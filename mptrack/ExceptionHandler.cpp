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

bool ExceptionHandler::stopSoundDeviceOnCrash = true;


enum DumpMode
{
	DumpModeCrash   = 0,  // crash
	DumpModeWarning = 1,  // assert
	DumpModeDebug   = 2,  // debug output (e.g. trace log)
};


struct CrashOutputDirectory
{
	bool valid;
	mpt::PathString path;
	CrashOutputDirectory()
		: valid(true)
	{
		const mpt::PathString timestampDir = mpt::PathString::FromCStringSilent((CTime::GetCurrentTime()).Format("%Y-%m-%d %H.%M.%S\\"));
		// Create a crash directory
		path = Util::GetTempDirectory() + MPT_PATHSTRING("OpenMPT Crash Files\\");
		if(!path.IsDirectory())
		{
			CreateDirectoryW(path.AsNative().c_str(), nullptr);
		}
		path += timestampDir;
		if(!path.IsDirectory())
		{
			if(!CreateDirectoryW(path.AsNative().c_str(), nullptr))
			{
				valid = false;
			}
		}
	}
};


class DebugReporter
{
private:
	bool stateFrozen;
	const DumpMode mode;
	const CrashOutputDirectory crashDirectory;
	bool writtenMiniDump;
	bool writtenTraceLog;
	int rescuedFiles;
private:
	static bool FreezeState(DumpMode mode);
	bool GenerateDump(_EXCEPTION_POINTERS *pExceptionInfo);
	bool GenerateTraceLog();
	int RescueFiles();
	bool HasWrittenDebug() const { return writtenMiniDump || writtenTraceLog; }
public:
	DebugReporter(DumpMode mode, _EXCEPTION_POINTERS *pExceptionInfo);
	void ReportError(CString errorMessage);
};


DebugReporter::DebugReporter(DumpMode mode, _EXCEPTION_POINTERS *pExceptionInfo)
//------------------------------------------------------------------------------
	: stateFrozen(FreezeState(mode))
	, mode(mode)
	, writtenMiniDump(false)
	, writtenTraceLog(false)
	, rescuedFiles(0)
{
	if(mode == DumpModeCrash || mode == DumpModeWarning)
	{
		writtenMiniDump = GenerateDump(pExceptionInfo);
	}
	if(mode == DumpModeCrash || mode == DumpModeWarning || mode == DumpModeDebug)
	{
		writtenTraceLog = GenerateTraceLog();
	}
	if(mode == DumpModeCrash || mode == DumpModeWarning)
	{
		rescuedFiles = RescueFiles();
	}
}


bool DebugReporter::GenerateDump(_EXCEPTION_POINTERS *pExceptionInfo)
//-------------------------------------------------------------------
{
	return WriteMemoryDump(pExceptionInfo, (crashDirectory.path + MPT_PATHSTRING("crash.dmp")).AsNative().c_str(), ExceptionHandler::fullMemDump);
}


bool DebugReporter::GenerateTraceLog()
//------------------------------------
{
	return mpt::log::Trace::Dump(crashDirectory.path + MPT_PATHSTRING("trace.log"));
}


// Rescue modified files...
int DebugReporter::RescueFiles()
//------------------------------
{
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
				CTrackApp::OpenDirectory(crashDirectory.path);
			}

			mpt::PathString filename;
			filename += crashDirectory.path;
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
	return numFiles;
}


void DebugReporter::ReportError(CString errorMessage)
//---------------------------------------------------
{

	if(!crashDirectory.valid)
	{
		errorMessage += "\n\nCould not create the following directory for saving debug information and modified files to:\n"
			+ mpt::ToCString(crashDirectory.path.ToWide());
	}

	if(HasWrittenDebug())
	{
		errorMessage += "\n\nDebug information has been saved to\n"
			+ mpt::ToCString(crashDirectory.path.ToWide());
	}

	if(rescuedFiles > 0)
	{
		errorMessage.AppendFormat("\n\n%d modified file%s been rescued, but it cannot be guaranteed that %s still intact.", rescuedFiles, (rescuedFiles == 1 ? " has" : "s have"), (rescuedFiles == 1 ? "it is" : "they are"));
	}

	errorMessage.AppendFormat("\n\nOpenMPT %s (%s)\n",
		MptVersion::GetVersionStringExtended().c_str(),
		MptVersion::GetVersionUrlString().c_str()
		);

	Reporting::Error(errorMessage, (mode == DumpModeWarning) ? "OpenMPT Warning" : "OpenMPT Crash", CMainFrame::GetMainFrame());

}


// Freezes the state as much aspossible in order to avoid further confusion by
// other (possibly still running) threads
bool DebugReporter::FreezeState(DumpMode mode)
//--------------------------------------------
{
	// seal the trace log as early as possible
	mpt::log::Trace::Seal();

	if(mode == DumpModeCrash || mode == DumpModeWarning)
	{
		// Shut down audio device...
		if(ExceptionHandler::stopSoundDeviceOnCrash)
		{
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
		}
	}

	return true;
}



// Different entry points for different situations in which we want to dump some information


LONG ExceptionHandler::UnhandledExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo)
//----------------------------------------------------------------------------------
{
	DebugReporter report(DumpModeCrash, pExceptionInfo);

	CString errorMessage;
	errorMessage.Format("Unhandled exception 0x%X at address %p occurred.", pExceptionInfo->ExceptionRecord->ExceptionCode, pExceptionInfo->ExceptionRecord->ExceptionAddress);
	report.ReportError(errorMessage);

	// Let Windows handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;
}


#if defined(MPT_ASSERT_HANDLER_NEEDED)

noinline void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//--------------------------------------------------------------------------------------------------------------
{
	DebugReporter report(msg ? DumpModeWarning : DumpModeCrash, nullptr);
	if(IsDebuggerPresent())
	{
		OutputDebugString("ASSERT(");
		OutputDebugString(expr);
		OutputDebugString(") failed\n");
		DebugBreak();
	} else
	{
		CString errorMessage;
		if(msg)
		{
			errorMessage.Format("Internal state inconsistency detected at %s(%d). This is just a warning that could potentially lead to a crash later on: %s [%s].", file, line, msg, function);
		} else
		{
			errorMessage.Format("Internal error occured at %s(%d): ASSERT(%s) failed in [%s].", file, line, expr, function);
		}
		report.ReportError(errorMessage);
	}
}

#endif MPT_ASSERT_HANDLER_NEEDED


void DebugTraceDump()
//-------------------
{
	DebugReporter report(DumpModeDebug, nullptr);
}


OPENMPT_NAMESPACE_END
