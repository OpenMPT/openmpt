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
#include "Mptrack.h"
#include "AboutDialog.h"
#include "../sounddev/SoundDevice.h"
#include "Moddoc.h"
#include <shlwapi.h>
#include "ExceptionHandler.h"
#include "../common/WriteMemoryDump.h"
#include "../common/version.h"
#include "../common/mptFileIO.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN


// Write full memory dump instead of minidump.
bool ExceptionHandler::fullMemDump = false;

bool ExceptionHandler::stopSoundDeviceOnCrash = true;

bool ExceptionHandler::stopSoundDeviceBeforeDump = false;

// Delegate to system-specific crash processing once our own crash handler is
// finished. This is useful to allow attaching a debugger.
bool ExceptionHandler::delegateToWindowsHandler = false;

// Allow debugging the unhandled exception filter. Normally, if a debugger is
// attached, no exceptions are unhandled because the debugger handles them. If
// debugExceptionHandler is true, an additional __try/__catch is inserted around
// InitInstance(), ExitInstance() and the main message loop, which will call our
// filter, which then can be stepped through in a debugger. 
bool ExceptionHandler::debugExceptionHandler = false;


bool ExceptionHandler::useAnyCrashHandler = false;
bool ExceptionHandler::useImplicitFallbackSEH = false;
bool ExceptionHandler::useExplicitSEH = false;
bool ExceptionHandler::handleStdTerminate = false;
bool ExceptionHandler::handleStdUnexpected = false;
bool ExceptionHandler::handleMfcExceptions = false;


static LPTOP_LEVEL_EXCEPTION_FILTER g_OriginalUnhandledExceptionFilter = nullptr;
static std::terminate_handler g_OriginalTerminateHandler = nullptr;
static std::unexpected_handler g_OriginalUnexpectedHandler = nullptr;

static UINT g_OriginalErrorMode = 0;


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
		path = mpt::GetTempDirectory() + MPT_PATHSTRING("OpenMPT Crash Files\\");
		if(!path.IsDirectory())
		{
			CreateDirectoryW(path.AsNative().c_str(), nullptr);
		}
		// Set compressed attribute in order to save disk space.
		// Debugging information should clutter the users computer as little as possible.
		// Performance is not important here.
		// Ignore any errors.
		SetFilesystemCompression(path);
		// Compression will be inherited by children directories and files automatically.
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
	static bool Cleanup(DumpMode mode);
	bool GenerateDump(_EXCEPTION_POINTERS *pExceptionInfo);
	bool GenerateTraceLog();
	int RescueFiles();
	bool HasWrittenDebug() const { return writtenMiniDump || writtenTraceLog; }
	static void StopSoundDevice();
public:
	DebugReporter(DumpMode mode, _EXCEPTION_POINTERS *pExceptionInfo);
	~DebugReporter();
	void ReportError(mpt::ustring errorMessage);
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


DebugReporter::~DebugReporter()
//-----------------------------
{
	Cleanup(mode);
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


static void SaveDocumentSafe(CModDoc *pModDoc, const mpt::PathString &filename)
//-----------------------------------------------------------------------------
{
	__try
	{
		pModDoc->OnSaveDocument(filename);
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		// nothing
	}
}


// Rescue modified files...
int DebugReporter::RescueFiles()
//------------------------------
{
	int numFiles = 0;
	auto documents = theApp.GetOpenDocuments();
	for(auto modDoc : documents)
	{
		if(modDoc->IsModified() && modDoc->GetSoundFile() != nullptr)
		{
			if(numFiles == 0)
			{
				// Show the rescue directory in Explorer...
				CTrackApp::OpenDirectory(crashDirectory.path);
			}

			mpt::PathString filename;
			filename += crashDirectory.path;
			filename += mpt::PathString::FromUnicode(mpt::ToUString(++numFiles));
			filename += MPT_PATHSTRING("_");
			filename += mpt::PathString::FromCStringSilent(modDoc->GetTitle()).SanitizeComponent();
			filename += MPT_PATHSTRING(".");
			filename += mpt::PathString::FromUTF8(modDoc->GetSoundFile()->GetModSpecifications().fileExtension);

			try
			{
				SaveDocumentSafe(modDoc, filename);
			} catch(...)
			{
				continue;
			}
		}
	}
	return numFiles;
}


void DebugReporter::ReportError(mpt::ustring errorMessage)
//--------------------------------------------------------
{

	if(!crashDirectory.valid)
	{
		errorMessage += MPT_ULITERAL("\n\n");
		errorMessage += MPT_ULITERAL("Could not create the following directory for saving debug information and modified files to:\n");
		errorMessage += crashDirectory.path.ToUnicode();
	}

	if(HasWrittenDebug())
	{
		errorMessage += MPT_ULITERAL("\n\n");
		errorMessage += MPT_ULITERAL("Debug information has been saved to\n");
		errorMessage += crashDirectory.path.ToUnicode();
	}

	if(rescuedFiles > 0)
	{
		errorMessage += MPT_ULITERAL("\n\n");
		if(rescuedFiles == 1)
		{
			errorMessage += MPT_ULITERAL("1 modified file has been rescued, but it cannot be guaranteed that it is still intact.");
		} else
		{
			errorMessage += mpt::String::Print(MPT_USTRING("%1 modified files have been rescued, but it cannot be guaranteed that they are still intact."), rescuedFiles);
		}
	}

	errorMessage += MPT_ULITERAL("\n\n");
	errorMessage += mpt::String::Print(MPT_USTRING("OpenMPT %1 (%2 (%3))\n")
		, mpt::ToUnicode(mpt::CharsetUTF8, MptVersion::GetVersionStringExtended())
		, mpt::ToUnicode(mpt::CharsetUTF8, MptVersion::GetSourceInfo().GetUrlWithRevision())
		, mpt::ToUnicode(mpt::CharsetUTF8, MptVersion::GetSourceInfo().GetStateString())
		);

	{
		mpt::ofstream f(crashDirectory.path + MPT_PATHSTRING("error.txt"), std::ios::binary);
		f.imbue(std::locale::classic());
		f << mpt::String::Replace(mpt::ToCharset(mpt::CharsetUTF8, errorMessage), "\n", "\r\n");
	}

	{
		mpt::ofstream f(crashDirectory.path + MPT_PATHSTRING("active-settings.txt"), std::ios::binary);
		f.imbue(std::locale::classic());
		if(&theApp.GetSettings())
		{
			SettingsContainer & settings = theApp.GetSettings();
			for(const auto &it : settings)
			{
				f
					<< mpt::ToCharset(mpt::CharsetUTF8, it.first.FormatAsString() + MPT_USTRING(" = ") + it.second.GetRefValue().FormatValueAsString())
					<< std::endl;
			}
		}
	}

	{
		CopyFileW
			( theApp.GetConfigFileName().AsNative().c_str()
			, (crashDirectory.path + MPT_PATHSTRING("stored-mptrack.ini")).AsNative().c_str()
			, FALSE
			);
	}

	/*
	// This is very slow, we instead write active-settings.txt above.
	{
		IniFileSettingsBackend f(crashDirectory.path + MPT_PATHSTRING("active-mptrack.ini"));
		if(&theApp.GetSettings())
		{
			if(&theApp.GetSettings())
			{
				SettingsContainer & settings = theApp.GetSettings();
				for(const auto &it : settings)
				{
					f.WriteSetting(it.first, it.second.GetRefValue());
				}
			}
		}
	}
	*/

	{
		mpt::ofstream f(crashDirectory.path + MPT_PATHSTRING("about-openmpt.txt"), std::ios::binary);
		f.imbue(std::locale::classic());
		f << mpt::ToCharset(mpt::CharsetUTF8, CAboutDlg::GetTabText(0));
	}

	{
		mpt::ofstream f(crashDirectory.path + MPT_PATHSTRING("about-components.txt"), std::ios::binary);
		f.imbue(std::locale::classic());
		f << mpt::ToCharset(mpt::CharsetUTF8, CAboutDlg::GetTabText(1));
	}

	Reporting::Error(errorMessage, (mode == DumpModeWarning) ? "OpenMPT Warning" : "OpenMPT Crash", CMainFrame::GetMainFrame());

}


// Freezes the state as much aspossible in order to avoid further confusion by
// other (possibly still running) threads
bool DebugReporter::FreezeState(DumpMode mode)
//--------------------------------------------
{
	MPT_TRACE();

	// seal the trace log as early as possible
	mpt::log::Trace::Seal();

	if(mode == DumpModeCrash || mode == DumpModeWarning)
	{
		if(CMainFrame::GetMainFrame() && CMainFrame::GetMainFrame()->gpSoundDevice && CMainFrame::GetMainFrame()->gpSoundDevice->DebugIsFragileDevice())
		{
			// For fragile devices, always stop the device. Stop before the dumping if not in realtime context.
			if(!CMainFrame::GetMainFrame()->gpSoundDevice->DebugInRealtimeCallback())
			{
				StopSoundDevice();
			}
		} else
		{
			if(ExceptionHandler::stopSoundDeviceOnCrash && ExceptionHandler::stopSoundDeviceBeforeDump)
			{
				StopSoundDevice();
			}
		}
	}

	return true;
}


static void StopSoundDeviceSafe(CMainFrame *pMainFrame)
//-----------------------------------------------------
{
	__try
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
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		// nothing
	}
}


void DebugReporter::StopSoundDevice()
//-----------------------------------
{
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();
	if(pMainFrame)
	{
		try
		{
			StopSoundDeviceSafe(pMainFrame);
		} catch(...)
		{
			// nothing
		}
	}
}


bool DebugReporter::Cleanup(DumpMode mode)
//----------------------------------------
{
	MPT_TRACE();

	if(mode == DumpModeCrash || mode == DumpModeWarning)
	{
		if(CMainFrame::GetMainFrame() && CMainFrame::GetMainFrame()->gpSoundDevice && CMainFrame::GetMainFrame()->gpSoundDevice->DebugIsFragileDevice())
		{
			// For fragile devices, always stop the device. Stop after the dumping if in realtime context.
			if(CMainFrame::GetMainFrame()->gpSoundDevice->DebugInRealtimeCallback())
			{
				StopSoundDevice();
			}
		} else
		{
			if(ExceptionHandler::stopSoundDeviceOnCrash && !ExceptionHandler::stopSoundDeviceBeforeDump)
			{
				StopSoundDevice();
			}
		}
	}

	return true;
}


// Different entry points for different situations in which we want to dump some information


static bool IsCxxException(_EXCEPTION_POINTERS *pExceptionInfo)
{
	if (!pExceptionInfo)
		return false;
	if (!pExceptionInfo->ExceptionRecord)
		return false;
	if (pExceptionInfo->ExceptionRecord->ExceptionCode != 0xE06D7363u)
		return false;
	return true;
}


template <typename E>
static const E * GetCxxException(_EXCEPTION_POINTERS *pExceptionInfo)
{
	// https://blogs.msdn.microsoft.com/oldnewthing/20100730-00/?p=13273
	struct info_a_t
	{
		DWORD  bitmask;              // Probably: 1=Const, 2=Volatile
		DWORD  destructor;           // RVA (Relative Virtual Address) of destructor for that exception object
		DWORD  unknown;
		DWORD  catchableTypesPtr;    // RVA of instance of type "B"
	};
	struct info_c_t
	{
		DWORD  someBitmask;
		DWORD  typeInfo;             // RVA of std::type_info for that type
		DWORD  memberDisplacement;   // Add to ExceptionInformation[1] in EXCEPTION_RECORD to obtain 'this' pointer.
		DWORD  virtBaseRelated1;     // -1 if no virtual base
		DWORD  virtBaseRelated2;     // ?
		DWORD  objectSize;           // Size of the object in bytes
		DWORD  probablyCopyCtr;      // RVA of copy constructor (?)
	};
	if(!pExceptionInfo)
		return nullptr;
	if (!pExceptionInfo->ExceptionRecord)
		return nullptr;
	if(pExceptionInfo->ExceptionRecord->ExceptionCode != 0xE06D7363u)
		return nullptr;
	#ifdef _WIN64
		if(pExceptionInfo->ExceptionRecord->NumberParameters != 4)
			return nullptr;
	#else
		if(pExceptionInfo->ExceptionRecord->NumberParameters != 3)
			return nullptr;
	#endif
	if(pExceptionInfo->ExceptionRecord->ExceptionInformation[0] != 0x19930520u)
		return nullptr;
	std::uintptr_t base_address = 0;
	#ifdef _WIN64
		base_address = pExceptionInfo->ExceptionRecord->ExceptionInformation[3];
	#else
		base_address = 0;
	#endif
	std::uintptr_t obj_address = pExceptionInfo->ExceptionRecord->ExceptionInformation[1];
	if(!obj_address)
		return nullptr;
	std::uintptr_t info_a_address = pExceptionInfo->ExceptionRecord->ExceptionInformation[2];
	if(!info_a_address)
		return nullptr;
	const info_a_t * info_a = reinterpret_cast<const info_a_t *>(info_a_address);
	std::uintptr_t info_b_offset = info_a->catchableTypesPtr;
	if(!info_b_offset)
		return nullptr;
	const DWORD * info_b = reinterpret_cast<const DWORD *>(base_address + info_b_offset);
	for(DWORD type = 1; type <= info_b[0]; ++type)
	{
		std::uintptr_t info_c_offset = info_b[type];
		if(!info_c_offset)
			continue;
		const info_c_t * info_c = reinterpret_cast<const info_c_t *>(base_address + info_c_offset);
		if(!info_c->typeInfo)
			continue;
		const std::type_info * ti = reinterpret_cast<const std::type_info *>(base_address + info_c->typeInfo);
		if(*ti != typeid(E))
			continue;
		const E * e = reinterpret_cast<const E *>(obj_address + info_c->memberDisplacement);
		return e;
	}
	return nullptr;
}


void ExceptionHandler::UnhandledMFCException(CException * e, const MSG * pMsg)
//----------------------------------------------------------------------------
{
	DebugReporter report(DumpModeCrash, nullptr);
	mpt::ustring errorMessage;
	if(e && dynamic_cast<CSimpleException*>(e))
	{
		TCHAR tmp[1024 + 1];
		MemsetZero(tmp);
		if(dynamic_cast<CSimpleException*>(e)->GetErrorMessage(tmp, MPT_ARRAY_COUNT(tmp) - 1) != 0)
		{
			tmp[1024] = 0;
			errorMessage = mpt::format(MPT_USTRING("Unhandled MFC exception occurred while processming window message '%1': %2."))
				(mpt::ufmt::dec(pMsg ? pMsg->message : 0)
				, mpt::ToUnicode(CString(tmp))
				);
		} else
		{
			errorMessage = mpt::format(MPT_USTRING("Unhandled MFC exception occurred while processming window message '%1': %2."))
				(mpt::ufmt::dec(pMsg ? pMsg->message : 0)
				, mpt::ToUnicode(CString(tmp))
				);
		}
	}
	else
	{
		errorMessage = mpt::format(MPT_USTRING("Unhandled MFC exception occurred while processming window message '%1'."))
			( mpt::ufmt::dec(pMsg ? pMsg->message : 0)
			);
	}
	report.ReportError(errorMessage);
}


static void UnhandledExceptionFilterImpl(_EXCEPTION_POINTERS *pExceptionInfo)
//---------------------------------------------------------------------------
{
	DebugReporter report(DumpModeCrash, pExceptionInfo);

	mpt::ustring errorMessage;
	const std::exception * pE = GetCxxException<std::exception>(pExceptionInfo);
	if(pE)
	{
		const std::exception & e = *pE;
		errorMessage = mpt::format(MPT_USTRING("Unhandled C++ exception '%1' occurred at address 0x%2: '%3'."))
			( mpt::ToUnicode(mpt::CharsetASCII, typeid(e).name())
			, mpt::ufmt::hex0<sizeof(void*)*2>(reinterpret_cast<std::uintptr_t>(pExceptionInfo->ExceptionRecord->ExceptionAddress))
			, mpt::ToUnicode(mpt::CharsetUTF8, e.what() ? e.what() : "")
			);
	} else
	{
		errorMessage = mpt::format(MPT_USTRING("Unhandled exception 0x%1 at address 0x%2 occurred."))
			( mpt::ufmt::HEX0<8>(pExceptionInfo->ExceptionRecord->ExceptionCode)
			, mpt::ufmt::hex0<sizeof(void*)*2>(reinterpret_cast<std::uintptr_t>(pExceptionInfo->ExceptionRecord->ExceptionAddress))
			);
	}
	report.ReportError(errorMessage);

}


LONG ExceptionHandler::UnhandledExceptionFilterContinue(_EXCEPTION_POINTERS *pExceptionInfo)
//------------------------------------------------------------------------------------------
{

	UnhandledExceptionFilterImpl(pExceptionInfo);

	// Disable the call to std::terminate() as that would re-renter the crash
	// handler another time, but with less information available.
#if 0
	// MSVC implements calling std::terminate by its own UnhandledExeptionFilter.
	// However, we do overwrite it here, thus we have to call std::terminate
	// ourselves.
	if (IsCxxException(pExceptionInfo))
	{
		std::terminate();
	}
#endif

	// Let a potential debugger handle the exception...
	return EXCEPTION_CONTINUE_SEARCH;

}


LONG ExceptionHandler::ExceptionFilter(_EXCEPTION_POINTERS *pExceptionInfo)
//-------------------------------------------------------------------------
{
	UnhandledExceptionFilterImpl(pExceptionInfo);
	// Let a potential debugger handle the exception...
	return EXCEPTION_EXECUTE_HANDLER;
}


static void mpt_unexpected_handler();
static void mpt_terminate_handler();


void ExceptionHandler::Register()
//-------------------------------
{
	if(useImplicitFallbackSEH)
	{
		g_OriginalUnhandledExceptionFilter = ::SetUnhandledExceptionFilter(&UnhandledExceptionFilterContinue);
	}
	if(handleStdTerminate)
	{
		g_OriginalTerminateHandler = std::set_terminate(&mpt_terminate_handler);
	}
	if(handleStdUnexpected)
	{
		g_OriginalUnexpectedHandler = std::set_unexpected(&mpt_unexpected_handler);
	}
}


void ExceptionHandler::ConfigureSystemHandler()
//---------------------------------------------
{
#if (_WIN32_WINNT >= 0x0600)
	if(delegateToWindowsHandler)
	{
		//SetErrorMode(0);
		g_OriginalErrorMode = ::GetErrorMode();
	}	else
	{
		g_OriginalErrorMode = ::SetErrorMode(::GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	}
#else // _WIN32_WINNT < 0x0600
	if(delegateToWindowsHandler)
	{
		g_OriginalErrorMode = ::SetErrorMode(0);
	} else
	{
		g_OriginalErrorMode = ::SetErrorMode(::SetErrorMode(0) | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	}
#endif // _WIN32_WINNT
}


void ExceptionHandler::UnconfigureSystemHandler()
//-----------------------------------------------
{
	::SetErrorMode(g_OriginalErrorMode);
	g_OriginalErrorMode = 0;
}


void ExceptionHandler::Unregister()
//---------------------------------
{
	if(handleStdUnexpected)
	{
		std::set_unexpected(g_OriginalUnexpectedHandler);
		g_OriginalUnexpectedHandler = nullptr;
	}
	if(handleStdTerminate)
	{
		std::set_terminate(g_OriginalTerminateHandler);
		g_OriginalTerminateHandler = nullptr;
	}
	if(useImplicitFallbackSEH)
	{
		::SetUnhandledExceptionFilter(g_OriginalUnhandledExceptionFilter);
		g_OriginalUnhandledExceptionFilter = nullptr;
	}
}


static void mpt_unexpected_handler()
{
	DebugReporter(DumpModeCrash, nullptr).ReportError(MPT_USTRING("An unexpected C++ exception occured: std::unexpected() called."));
	// We disable the call to std::terminate() as that would re-renter the crash
	// handler another time, but with less information available.
#if 1
	std::abort();
#else
	if(g_OriginalUnexpectedHandler)
	{
		g_OriginalUnexpectedHandler();
	} else
	{
		std::terminate();
	}
#endif
}


static void mpt_terminate_handler()
{
	DebugReporter(DumpModeCrash, nullptr).ReportError(MPT_USTRING("A C++ runtime crash occured: std::terminate() called."));
#if 1
	std::abort();
#else
	if(g_OriginalTerminateHandler)
	{
		g_OriginalTerminateHandler();
	} else
	{
		std::abort();
	}
#endif
}


#if defined(MPT_ASSERT_HANDLER_NEEDED)

MPT_NOINLINE void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//------------------------------------------------------------------------------------------------------------------
{
	DebugReporter report(msg ? DumpModeWarning : DumpModeCrash, nullptr);
	if(IsDebuggerPresent())
	{
		OutputDebugString(_T("ASSERT("));
		OutputDebugString(mpt::ToCString(mpt::CharsetASCII, expr));
		OutputDebugString(_T(") failed\n"));
		DebugBreak();
	} else
	{
		CStringA errorMessage;
		if(msg)
		{
			errorMessage.Format("Internal state inconsistency detected at %s(%d). This is just a warning that could potentially lead to a crash later on: %s [%s].", file, line, msg, function);
		} else
		{
			errorMessage.Format("Internal error occurred at %s(%d): ASSERT(%s) failed in [%s].", file, line, expr, function);
		}
		report.ReportError(mpt::ToUnicode(mpt::CharsetLocale, errorMessage.GetString()));
	}
}

#endif MPT_ASSERT_HANDLER_NEEDED


void DebugInjectCrash()
//---------------------
{
	DebugReporter(DumpModeCrash, nullptr).ReportError(MPT_USTRING("Injected crash."));
}


void DebugTraceDump()
//-------------------
{
	DebugReporter report(DumpModeDebug, nullptr);
}


OPENMPT_NAMESPACE_END
