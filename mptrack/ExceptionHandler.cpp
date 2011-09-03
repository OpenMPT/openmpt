/*
 *
 * ExceptionHandler.cpp
 * --------------------
 * Purpose: Code for handling crashes in OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "snddev.h"
#include "Moddoc.h"
#include <shlwapi.h>
#include "ExceptionHandler.h"
#include "dbghelp.h"


LONG ExceptionHandler::UnhandledExceptionFilterMain(_EXCEPTION_POINTERS *pExceptionInfo)
//--------------------------------------------------------------------------------------
{
	return UnhandledExceptionFilter(pExceptionInfo, "main");
}


LONG ExceptionHandler::UnhandledExceptionFilterAudio(_EXCEPTION_POINTERS *pExceptionInfo)
//---------------------------------------------------------------------------------------
{
	return UnhandledExceptionFilter(pExceptionInfo, "audio");
}


LONG ExceptionHandler::UnhandledExceptionFilterNotify(_EXCEPTION_POINTERS *pExceptionInfo)
//----------------------------------------------------------------------------------------
{
	return UnhandledExceptionFilter(pExceptionInfo, "notify");
}


typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

// Try to close the audio device and rescue unsaved work if an unhandled exception occours...
LONG ExceptionHandler::UnhandledExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo, const CString threadName)
//------------------------------------------------------------------------------------------------------------
{
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();
	const HWND window = (pMainFrame ? pMainFrame->m_hWnd : NULL);

	// Shut down audio device...
	if(pMainFrame)
	{
		if(pMainFrame->gpSoundDevice) pMainFrame->gpSoundDevice->Reset();
		pMainFrame->audioCloseDevice();
	}

	CString errorMessage;
	errorMessage.Format("Unhandled exception 0x%X at address %p occoured in the %s thread.", pExceptionInfo->ExceptionRecord->ExceptionCode, pExceptionInfo->ExceptionRecord->ExceptionAddress, threadName);

	const CString timestampDir = (CTime::GetCurrentTime()).Format("%Y-%m-%d %H.%M.%S\\");
	CString baseRescuePath;
	{
		// Create a crash directory
		TCHAR tempPath[_MAX_PATH];
		GetTempPath(CountOf(tempPath), tempPath);
		baseRescuePath.Format("%sOpenMPT Crash Files\\", tempPath);
		if(!PathIsDirectory(baseRescuePath))
		{
			CreateDirectory(baseRescuePath, nullptr);
		}
		baseRescuePath.Append(timestampDir);
		if(!PathIsDirectory(baseRescuePath) && !CreateDirectory(baseRescuePath, nullptr))
		{
			errorMessage.AppendFormat("\n\nCould not create the following directory for saving debug information and modified files to:\n%s", baseRescuePath);
		}
	}

	// Create minidump...
	HMODULE hDll = ::LoadLibrary("DBGHELP.DLL");
	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
		if (pDump)
		{
			const CString filename = baseRescuePath + "crash-" + threadName + ".dmp";

			HANDLE hFile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = pExceptionInfo;
				ExInfo.ClientPointers = NULL;

				pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
				::CloseHandle(hFile);

				errorMessage.AppendFormat("\n\nDebug information have been saved to\n%s", baseRescuePath);
			}
		}
		::FreeLibrary(hDll);
	}

	// Rescue modified files...
	CDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if(pDocTmpl)
	{
		POSITION pos = pDocTmpl->GetFirstDocPosition();
		CDocument *pDoc;

		int numFiles = 0;

		while((pos != NULL) && ((pDoc = pDocTmpl->GetNextDoc(pos)) != NULL))
		{
			CModDoc *pModDoc = (CModDoc *)pDoc;
			if(pModDoc->IsModified() && pModDoc->GetSoundFile() != nullptr)
			{
				if(numFiles == 0)
				{
					// Show the rescue directory in Explorer...
					CTrackApp::OpenDirectory(baseRescuePath);
				}
				CString filename;
				filename.Format("%s%d_%s.%s", baseRescuePath, ++numFiles, pModDoc->GetTitle(), pModDoc->GetSoundFile()->GetModSpecifications().fileExtension);
				pModDoc->OnSaveDocument(filename);
			}
		}

		if(numFiles > 0)
		{
			errorMessage.AppendFormat("\n\n%d modified file%s been rescued, but it cannot be guaranteed that %s still intact.", numFiles, (numFiles == 1 ? " has" : "s have"), (numFiles == 1 ? "it is" : "they are"));
		}
	}
	
	Reporting::Notification(errorMessage, "OpenMPT Crash", MB_ICONERROR, window);

	// Let Windows handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;
}
