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
#include "ExceptionHandler.h"
#include "dbghelp.h"


void ExceptionHandler::RegisterMainThread()
//-----------------------------------------
{
	::SetUnhandledExceptionFilter(UnhandledExceptionFilterMain);
}


void ExceptionHandler::RegisterAudioThread()
//------------------------------------------
{
	::SetUnhandledExceptionFilter(UnhandledExceptionFilterAudio);
}


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
			::MessageBox(window, "A crash has been detected and OpenMPT will be closed.\nOpenMPT was unable to create a directory for saving debug information and modified files to.", "OpenMPT Crash", MB_ICONERROR);
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
			CString message;
			message.Format("A crash has been detected in the %s thread and OpenMPT will be closed.\n%d modified file%s been rescued to\n\n%s\n\nNote: It cannot be guaranteed that rescued files are still intact.", threadName, numFiles, (numFiles == 1 ? " has" : "s have"), baseRescuePath);
			::MessageBox(window, message, "OpenMPT Crash", MB_ICONERROR);
		}
	}

	// Let Windows handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;
}
