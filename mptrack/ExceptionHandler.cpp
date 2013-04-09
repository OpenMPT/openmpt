/*
 * ExceptionHandler.cpp
 * --------------------
 * Purpose: Code for handling crashes (unhandled exceptions) in OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "snddev.h"
#include "Moddoc.h"
#include <shlwapi.h>
#include "ExceptionHandler.h"
#include "dbghelp.h"


typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);


static void GenerateDump(CString &errorMessage, _EXCEPTION_POINTERS *pExceptionInfo=NULL)
//---------------------------------------------------------------------------------------
{
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();

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
			const CString filename = baseRescuePath + "crash.dmp";

			HANDLE hFile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				if(pExceptionInfo)
				{
					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;
				}

				pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, pExceptionInfo ? &ExInfo : NULL, NULL, NULL);
				::CloseHandle(hFile);

				errorMessage.AppendFormat("\n\nDebug information has been saved to\n%s", baseRescuePath);
			}
		}
		::FreeLibrary(hDll);
	}

	// Rescue modified files...
	int numFiles = 0;
	vector<CModDoc *> documents = theApp.GetOpenDocuments();
	for(vector<CModDoc *>::iterator doc = documents.begin(); doc != documents.end(); doc++)
	{
		CModDoc *pModDoc = *doc;
		if(pModDoc->IsModified() && pModDoc->GetSoundFile() != nullptr)
		{
			if(numFiles == 0)
			{
				// Show the rescue directory in Explorer...
				CTrackApp::OpenDirectory(baseRescuePath);
			}
			CString filename;
			filename.Format("%s%d_%s.%s", baseRescuePath, ++numFiles, pModDoc->GetTitle(), pModDoc->GetSoundFile()->GetModSpecifications().fileExtension);

			try
			{
				pModDoc->OnSaveDocument(filename);
			} catch(...)
			{
				continue;
			}
		}
	}

	if(numFiles > 0)
	{
		errorMessage.AppendFormat("\n\n%d modified file%s been rescued, but it cannot be guaranteed that %s still intact.", numFiles, (numFiles == 1 ? " has" : "s have"), (numFiles == 1 ? "it is" : "they are"));
	}
	
	Reporting::Error(errorMessage, "OpenMPT Crash", pMainFrame);

}


// Try to close the audio device and rescue unsaved work if an unhandled exception occours...
LONG ExceptionHandler::UnhandledExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo)
//----------------------------------------------------------------------------------
{
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();

	// Shut down audio device...
	if(pMainFrame)
	{
		try
		{
			if(pMainFrame->gpSoundDevice) pMainFrame->gpSoundDevice->Reset();
			pMainFrame->audioCloseDevice();
		} catch(...)
		{
		}
	}

	CString errorMessage;
	errorMessage.Format("Unhandled exception 0x%X at address %p occoured.", pExceptionInfo->ExceptionRecord->ExceptionCode, pExceptionInfo->ExceptionRecord->ExceptionAddress);

	GenerateDump(errorMessage, pExceptionInfo);

	// Let Windows handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;
}


void AlwaysAssertHandler(const char *file, int line, const char *function, const char *expr)
//------------------------------------------------------------------------------------------
{
	CString errorMessage;
	errorMessage.Format("Internal error occured at %s(%d): ASSERT(%s) failed in [%s].", file, line, expr, function);
	GenerateDump(errorMessage);
}
