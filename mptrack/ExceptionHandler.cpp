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
#include "../sounddev/SoundDevice.h"
#include "Moddoc.h"
#include <shlwapi.h>
#include "ExceptionHandler.h"
#include "../common/WriteMemoryDump.h"
#include "../common/version.h"


OPENMPT_NAMESPACE_BEGIN


bool ExceptionHandler::fullMemDump = false;

enum DumpMode
{
	DumpModeCrash   = 0,
	DumpModeWarning = 1,
};


static void GenerateDump(CString &errorMessage, _EXCEPTION_POINTERS *pExceptionInfo=NULL, DumpMode mode=DumpModeCrash)
//--------------------------------------------------------------------------------------------------------------------
{
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();

	const mpt::PathString timestampDir = mpt::PathString::FromCStringSilent((CTime::GetCurrentTime()).Format("%Y-%m-%d %H.%M.%S\\"));
	mpt::PathString baseRescuePath;
	{
		// Create a crash directory
		baseRescuePath = Util::GetTempDirectory() + MPT_PATHSTRING("OpenMPT Crash Files\\");
		if(!baseRescuePath.IsDirectory())
		{
			CreateDirectoryW(baseRescuePath.AsNative().c_str(), nullptr);
		}
		baseRescuePath += timestampDir;
		if(!baseRescuePath.IsDirectory() && !CreateDirectoryW(baseRescuePath.AsNative().c_str(), nullptr))
		{
			errorMessage += "\n\nCould not create the following directory for saving debug information and modified files to:\n"
				+ mpt::ToCString(baseRescuePath.ToWide());
		}
	}

	// Create minidump...
	const mpt::PathString filename = baseRescuePath + MPT_PATHSTRING("crash.dmp");
	if(WriteMemoryDump(pExceptionInfo, filename.AsNative().c_str(), ExceptionHandler::fullMemDump))
	{
		errorMessage += "\n\nDebug information has been saved to\n"
			+ mpt::ToCString(baseRescuePath.ToWide());
	}

	// Rescue modified files...
	int numFiles = 0;
	std::vector<CModDoc *> documents = theApp.GetOpenDocuments();
	for(std::vector<CModDoc *>::iterator doc = documents.begin(); doc != documents.end(); doc++)
	{
		CModDoc *pModDoc = *doc;
		if(pModDoc->IsModified() && pModDoc->GetSoundFile() != nullptr)
		{
			if(numFiles == 0)
			{
				// Show the rescue directory in Explorer...
				CTrackApp::OpenDirectory(baseRescuePath);
			}

			mpt::PathString filename;
			filename += baseRescuePath;
			filename += mpt::PathString::FromWide(mpt::ToWString(++numFiles));
			filename += MPT_PATHSTRING("_");
			filename += mpt::PathString::FromCStringSilent(pModDoc->GetTitle()).SanitizeComponent();
			filename += MPT_PATHSTRING(".");
			filename += mpt::PathString::FromUTF8(pModDoc->GetSoundFile()->GetModSpecifications().fileExtension);

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
	
	errorMessage.AppendFormat("\n\nOpenMPT %s (%s)\n",
		MptVersion::GetVersionStringExtended().c_str(),
		MptVersion::GetVersionUrlString().c_str()
		);

	if(mode == DumpModeWarning)
	{
		Reporting::Error(errorMessage, "OpenMPT Warning", pMainFrame);
	} else
	{
		Reporting::Error(errorMessage, "OpenMPT Crash", pMainFrame);
	}
}


// Try to close the audio device and rescue unsaved work if an unhandled exception occurs...
LONG ExceptionHandler::UnhandledExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo)
//----------------------------------------------------------------------------------
{
	// Shut down audio device...
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();
	if(pMainFrame)
	{
		try
		{
			if(pMainFrame->gpSoundDevice)
			{
				pMainFrame->gpSoundDevice->Close();
			}
			if(pMainFrame->m_NotifyTimer)
			{
				pMainFrame->KillTimer(pMainFrame->m_NotifyTimer);
				pMainFrame->m_NotifyTimer = 0;
			}
		} catch(...)
		{
		}
	}

	CString errorMessage;
	errorMessage.Format("Unhandled exception 0x%X at address %p occurred.", pExceptionInfo->ExceptionRecord->ExceptionCode, pExceptionInfo->ExceptionRecord->ExceptionAddress);

	GenerateDump(errorMessage, pExceptionInfo);

	// Let Windows handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;
}


#if defined(MPT_ASSERT_HANDLER_NEEDED)

noinline void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//--------------------------------------------------------------------------------------------------------------
{
	if(IsDebuggerPresent())
	{
		OutputDebugString("ASSERT(");
		OutputDebugString(expr);
		OutputDebugString(") failed\n");
		DebugBreak();
	} else
	{
		if(msg)
		{
			CString errorMessage;
			errorMessage.Format("Internal state inconsistency detected at %s(%d). This is just a warning that could potentially lead to a crash later on: %s [%s].", file, line, msg, function);
			GenerateDump(errorMessage, NULL, DumpModeWarning);
		} else
		{
			CString errorMessage;
			errorMessage.Format("Internal error occured at %s(%d): ASSERT(%s) failed in [%s].", file, line, expr, function);
			GenerateDump(errorMessage);
		}
	}
}

#endif MPT_ASSERT_HANDLER_NEEDED


OPENMPT_NAMESPACE_END
