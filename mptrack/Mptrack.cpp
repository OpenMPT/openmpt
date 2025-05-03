/*
 * MPTrack.cpp
 * -----------
 * Purpose: OpenMPT core application class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "AboutDialog.h"
#include "AutoSaver.h"
#include "Childfrm.h"
#include "CloseMainDialog.h"
#include "DialogBase.h"
#include "ExceptionHandler.h"
#include "FileDialog.h"
#include "FolderScanner.h"
#include "Globals.h"
#include "Image.h"
#include "InputHandler.h"
#include "IPCWindow.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "ModDocTemplate.h"
#include "MPTrackUtil.h"
#include "MPTrackWine.h"
#include "PlugNotFoundDlg.h"
#include "Reporting.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "UpdateCheck.h"
#include "WelcomeDialog.h"
#include "WineSoundDeviceStub.h"
#include "../common/ComponentManager.h"
#include "../common/mptStringBuffer.h"
#include "../common/version.h"
#include "../misc/mptOS.h"
#include "../soundlib/Dlsbank.h"
#include "../soundlib/plugins/PluginManager.h"
#include "../test/test.h"
#include "mpt/arch/arch.hpp"
#include "mpt/fs/common_directories.hpp"
#include "mpt/fs/fs.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "mpt/random/seed.hpp"
#include "mpt/string/utility.hpp"
#include "openmpt/sounddevice/SoundDeviceManager.hpp"

#if MPT_MSVC_AT_LEAST(2022, 2) && MPT_MSVC_BEFORE(2022, 3)
// Work-around <https://developercommunity.visualstudio.com/t/warning-C4311-in-MFC-header-afxrecovery/10041328>,
// see <https://developercommunity.visualstudio.com/t/Compiler-warnings-after-upgrading-to-17/10036311#T-N10061908>.
template <class ARG_KEY>
AFX_INLINE UINT AFXAPI HashKey(ARG_KEY key);
template <>
AFX_INLINE UINT AFXAPI HashKey<CDocument*>(CDocument *key)
{
	// (algorithm copied from STL hash in xfunctional)
#pragma warning(suppress: 4302) // 'type cast' : truncation
#pragma warning(suppress: 4311) // pointer truncation
	ldiv_t HashVal = ldiv((long)(CDocument*)key, 127773);
	HashVal.rem = 16807 * HashVal.rem - 2836 * HashVal.quot;
	if(HashVal.rem < 0)
		HashVal.rem += 2147483647;
	return ((UINT)HashVal.rem);
}
#endif
#include <afxdatarecovery.h>

// GDI+
#include <atlbase.h>
#if MPT_MSVC_BEFORE(2019, 11)  // really < Windows 10 SDK 2104 (10.0.20348.0)
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4458)  // declaration of 'x' hides class member
#endif
#include <gdiplus.h>
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif
#if MPT_MSVC_BEFORE(2019, 11)  // really < Windows 10 SDK 2104 (10.0.20348.0)
#undef min
#undef max
#endif

#if MPT_COMPILER_MSVC
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif


OPENMPT_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////////////////
// The one and only CTrackApp object

CTrackApp theApp;

const TCHAR *szSpecialNoteNamesMPT[] = {_T("PCs"), _T("PC"), _T("~~ (Note Fade)"), _T("^^ (Note Cut)"), _T("== (Note Off)")};
const TCHAR *szSpecialNoteShortDesc[] = {_T("Param Control (Smooth)"), _T("Param Control"), _T("Note Fade"), _T("Note Cut"), _T("Note Off")};

// Make sure that special note arrays include string for every note.
static_assert(NOTE_MAX_SPECIAL - NOTE_MIN_SPECIAL + 1 == mpt::array_size<decltype(szSpecialNoteNamesMPT)>::size);
static_assert(mpt::array_size<decltype(szSpecialNoteShortDesc)>::size == mpt::array_size<decltype(szSpecialNoteNamesMPT)>::size);

const char *szHexChar = "0123456789ABCDEF";


#ifdef MPT_WITH_ASIO
class ComponentASIO
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentASIO, "ASIO")
public:
	ComponentASIO() = default;
	virtual ~ComponentASIO() = default;
};
#endif // MPT_WITH_ASIO

#if defined(MPT_WITH_DIRECTSOUND)
class ComponentDirectSound 
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentDirectSound, "DirectSound")
public:
	ComponentDirectSound() = default;
	virtual ~ComponentDirectSound() = default;
};
#endif // MPT_WITH_DIRECTSOUND

#if defined(MPT_WITH_PORTAUDIO)
class ComponentPortAudio
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentPortAudio, "PortAudio")
public:
	ComponentPortAudio() = default;
	virtual ~ComponentPortAudio() = default;
};
#endif // MPT_WITH_PORTAUDIO

#if defined(MPT_WITH_PULSEAUDIO)
class ComponentPulseaudio
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentPulseaudio, "Pulseaudio")
public:
	ComponentPulseaudio() = default;
	virtual ~ComponentPulseaudio() = default;
};
#endif // MPT_WITH_PULSEAUDIO

#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
class ComponentPulseaudioSimple
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentPulseaudioSimple, "PulseaudioSimple")
public:
	ComponentPulseaudioSimple() = default;
	virtual ~ComponentPulseaudioSimple() = default;
};
#endif // MPT_WITH_PULSEAUDIO && MPT_WITH_PULSEAUDIOSIMPLE

#if defined(MPT_WITH_RTAUDIO)
class ComponentRtAudio
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentRtAudio, "RtAudio")
public:
	ComponentRtAudio() = default;
	virtual ~ComponentRtAudio() = default;
};
#endif // MPT_WITH_RTAUDIO

#if MPT_OS_WINDOWS
class ComponentWaveOut
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS(ComponentWaveOut, "WaveOut")
public:
	ComponentWaveOut() = default;
	virtual ~ComponentWaveOut() = default;
};
#endif // MPT_OS_WINDOWS

struct AllSoundDeviceComponents
{
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_ENABLE_PULSEAUDIO_FULL)
	ComponentHandle<ComponentPulseaudio> m_Pulseaudio;
#endif // MPT_WITH_PULSEAUDIO && MPT_ENABLE_PULSEAUDIO_FULL
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
	ComponentHandle<ComponentPulseaudioSimple> m_PulseaudioSimple;
#endif // MPT_WITH_PULSEAUDIO && MPT_WITH_PULSEAUDIOSIMPLE
#if MPT_OS_WINDOWS
	ComponentHandle<ComponentWaveOut> m_WaveOut;
#endif // MPT_OS_WINDOWS
#if defined(MPT_WITH_DIRECTSOUND)
	ComponentHandle<ComponentDirectSound> m_DirectSound;
#endif // MPT_WITH_DIRECTSOUND
#ifdef MPT_WITH_ASIO
	ComponentHandle<ComponentASIO> m_ASIO;
#endif // MPT_WITH_ASIO
#ifdef MPT_WITH_PORTAUDIO
	ComponentHandle<ComponentPortAudio> m_PortAudio;
#endif // MPT_WITH_PORTAUDIO
#ifdef MPT_WITH_RTAUDIO
	ComponentHandle<ComponentRtAudio> m_RtAudio;
#endif // MPT_WITH_RTAUDIO
	operator SoundDevice::EnabledBackends() const
	{
		SoundDevice::EnabledBackends result;
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_ENABLE_PULSEAUDIO_FULL)
		result.Pulseaudio = IsComponentAvailable(m_PulseAudio);
#endif // MPT_WITH_PULSEAUDIO && MPT_ENABLE_PULSEAUDIO_FULL
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
		result.PulseaudioSimple = IsComponentAvailable(m_PulseAudioSimple);
#endif // MPT_WITH_PULSEAUDIO && MPT_WITH_PULSEAUDIOSIMPLE
#if MPT_OS_WINDOWS
		result.WaveOut = IsComponentAvailable(m_WaveOut);
#endif // MPT_OS_WINDOWS
#if defined(MPT_WITH_DIRECTSOUND)
		result.DirectSound = IsComponentAvailable(m_DirectSound);
#endif // MPT_WITH_DIRECTSOUND
#ifdef MPT_WITH_ASIO
		result.ASIO = IsComponentAvailable(m_ASIO);
#endif // MPT_WITH_ASIO
#ifdef MPT_WITH_PORTAUDIO
		result.PortAudio = IsComponentAvailable(m_PortAudio);
#endif // MPT_WITH_PORTAUDIO
#ifdef MPT_WITH_RTAUDIO
		result.RtAudio = IsComponentAvailable(m_RtAudio);
#endif // MPT_WITH_RTAUDIO
		return result;
	}
};


void CTrackApp::OnFileCloseAll()
{
	if(!(TrackerSettings::Instance().patternSetup & PatternSetup::NoCustomCloseDialog))
	{
		// Show modified documents window
		CloseMainDialog dlg;
		if(dlg.DoModal() != IDOK)
		{
			return;
		}
	}

	for(auto &doc : GetOpenDocuments())
	{
		doc->SafeFileClose();
	}
}


void CTrackApp::OnUpdateAnyDocsOpen(CCmdUI *cmd)
{
	cmd->Enable(!GetModDocTemplate()->empty());
}


int CTrackApp::GetOpenDocumentCount() const
{
	return static_cast<int>(GetModDocTemplate()->size());
}


// Retrieve a list of all open modules.
std::vector<CModDoc *> CTrackApp::GetOpenDocuments() const
{
	std::vector<CModDoc *> documents;
	if(auto *pDocTmpl = GetModDocTemplate())
	{
		POSITION pos = pDocTmpl->GetFirstDocPosition();
		CDocument *pDoc;
		while((pos != nullptr) && ((pDoc = pDocTmpl->GetNextDoc(pos)) != nullptr))
		{
			documents.push_back(dynamic_cast<CModDoc *>(pDoc));
		}
	}

	return documents;
}


void CTrackApp::UpdateAllViews(UpdateHint hint, CObject *pHint)
{
	if(auto *pDocTmpl = GetModDocTemplate())
	{
		for(auto &doc : *pDocTmpl)
		{
			doc->UpdateAllViews(nullptr, hint, pHint);
		}
	}
}


void CTrackApp::PostMessageToAllViews(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(auto *pDocTmpl = GetModDocTemplate())
	{
		for(auto &doc : *pDocTmpl)
		{
			doc->PostMessageToAllViews(uMsg, wParam, lParam);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// Command Line options

class CMPTCommandLineInfo: public CCommandLineInfo
{
public:
	std::vector<mpt::PathString> m_fileNames;
	bool m_noDls = false, m_noPlugins = false, m_noAssembly = false, m_noSysCheck = false, m_noWine = false,
		m_portable = false, m_noCrashHandler = false, m_debugCrashHandler = false, m_sharedInstance = false,
		m_autoPlay = false;
#ifdef ENABLE_TESTS
	bool m_noTests = false;
#endif

public:
	CMPTCommandLineInfo()
	{
		std::vector<TCHAR> curDir(::GetCurrentDirectory(0, nullptr), _T('\0'));
		::GetCurrentDirectory(static_cast<DWORD>(curDir.size()), curDir.data());
		m_workingDir = mpt::PathString::FromNative(curDir.data());
	}

	void ParseParam(LPCTSTR param, BOOL isFlag, BOOL isLast) override
	{
		if(isFlag)
		{
			if(!lstrcmpi(param, _T("nologo"))) { m_bShowSplash = FALSE; return; }
			if(!lstrcmpi(param, _T("nodls"))) { m_noDls = true; return; }
			if(!lstrcmpi(param, _T("noplugs"))) { m_noPlugins = true; return; }
			if(!lstrcmpi(param, _T("portable"))) { m_portable = true; return; }
			if(!lstrcmpi(param, _T("fullMemDump"))) { ExceptionHandler::fullMemDump = true; return; }
			if(!lstrcmpi(param, _T("noAssembly"))) { m_noAssembly = true; return; }
			if(!lstrcmpi(param, _T("noSysCheck"))) { m_noSysCheck = true; return; }
			if(!lstrcmpi(param, _T("noWine"))) { m_noWine = true; return; }
			if(!lstrcmpi(param, _T("noCrashHandler"))) { m_noCrashHandler = true; return; }
			if(!lstrcmpi(param, _T("DebugCrashHandler"))) { m_debugCrashHandler = true; return; }
			if(!lstrcmpi(param, _T("shared"))) { m_sharedInstance = true; return; }
			if(!lstrcmpi(param, _T("play"))) { m_autoPlay = true; return; }
#ifdef ENABLE_TESTS
			if (!lstrcmpi(param, _T("noTests"))) { m_noTests = true; return; }
#endif
		} else
		{
			m_fileNames.push_back(mpt::RelativePathToAbsolute(mpt::PathString::FromNative(param), m_workingDir));
			if(m_nShellCommand == FileNew) m_nShellCommand = FileOpen;
		}
		CCommandLineInfo::ParseParam(param, isFlag, isLast);
	}

protected:
	mpt::PathString m_workingDir;
};


// Splash Screen

static void StartSplashScreen();
static void StopSplashScreen();
static void TimeoutSplashScreen();


/////////////////////////////////////////////////////////////////////////////
// Midi Library

MidiLibrary CTrackApp::midiLibrary;

void CTrackApp::ImportMidiConfig(const mpt::PathString &filename, bool hideWarning)
{
	if(filename.empty()) return;

	if(CDLSBank::IsDLSBank(filename))
	{
		ConfirmAnswer result = cnfYes;
		if(!hideWarning)
		{
			result = Reporting::Confirm("You are about to replace the current MIDI library:\n"
				"Do you want to replace only the missing instruments? (recommended)",
				"Warning", true);
		}
		if(result == cnfCancel) return;
		const bool replaceAll = (result == cnfNo);
		CDLSBank dlsbank;
		if (dlsbank.Open(filename))
		{
			for(uint32 ins = 0; ins < 256; ins++)
			{
				if(replaceAll || !midiLibrary[ins] || midiLibrary[ins]->empty())
				{
					uint32 prog = (ins < 128) ? ins : 0xFF;
					uint32 key = (ins < 128) ? 0xFF : ins & 0x7F;
					if(dlsbank.FindInstrument(ins >= 128, 0xFFFF, prog, key))
					{
						midiLibrary[ins] = filename;
					}
				}
			}
		}
		return;
	}

	IniFileSettingsContainer file(filename);
	ImportMidiConfig(file, filename.GetDirectoryWithDrive());
}


static mpt::PathString GetUltraSoundPatchDir(SettingsContainer &file, const mpt::ustring &iniSection, const mpt::PathString &path, bool forgetSettings)
{
	mpt::PathString patchDir = file.Read<mpt::PathString>(iniSection, U_("PatchDir"), {});
	if(forgetSettings)
		file.Forget(U_("Ultrasound"), U_("PatchDir"));
	if(patchDir.empty() || patchDir == P_(".\\"))
		patchDir = path;
	if(!patchDir.empty())
		patchDir = patchDir.WithTrailingSlash();
	return patchDir;
}

void CTrackApp::ImportMidiConfig(SettingsContainer &file, const mpt::PathString &path, bool forgetSettings)
{
	const mpt::PathString patchDir = GetUltraSoundPatchDir(file, U_("Ultrasound"), path, forgetSettings);
	for(uint32 prog = 0; prog < 256; prog++)
	{
		mpt::ustring key = MPT_UFORMAT("{}{}")((prog < 128) ? U_("Midi") : U_("Perc"), prog & 0x7F);
		mpt::PathString filename = file.Read<mpt::PathString>(U_("Midi Library"), key, mpt::PathString());
		// Check for ULTRASND.INI
		if(filename.empty())
		{
			mpt::ustring section = (prog < 128) ? UL_("Melodic Patches") : UL_("Drum Patches");
			key = mpt::ufmt::val(prog & 0x7f);
			filename = file.Read<mpt::PathString>(section, key, mpt::PathString());
			if(forgetSettings) file.Forget(section, key);
			if(filename.empty())
			{
				section = (prog < 128) ? UL_("Melodic Bank 0") : UL_("Drum Bank 0");
				filename = file.Read<mpt::PathString>(section, key, mpt::PathString());
				if(forgetSettings) file.Forget(section, key);
			}
			const mpt::PathString localPatchDir = GetUltraSoundPatchDir(file, section, patchDir, forgetSettings);
			if(!filename.empty())
			{
				filename = localPatchDir + filename + P_(".pat");
			}
		}

		if(filename == P_("*empty"))
			midiLibrary[prog] = P_("");
		else if(!filename.empty())
			midiLibrary[prog] = theApp.PathInstallRelativeToAbsolute(filename);
	}
}


void CTrackApp::ExportMidiConfig(const mpt::PathString &filename)
{
	if(filename.empty())
		return;
	IniFileSettingsContainer file(filename);
	ExportMidiConfig(file);
}

void CTrackApp::ExportMidiConfig(SettingsContainer &file)
{
	for(uint32 prog = 0; prog < 256; prog++)
	{
		const mpt::ustring key = MPT_UFORMAT("{}{}")((prog < 128) ? U_("Midi") : U_("Perc"), prog & 0x7F);
		if(!midiLibrary[prog])
		{
			file.Forget(U_("Midi Library"), key);
			continue;
		}

		mpt::PathString fileName = *midiLibrary[prog];
		if(midiLibrary[prog]->empty())
			fileName = P_("*empty");
		else if(theApp.IsPortableMode())
			fileName = theApp.PathAbsoluteToInstallRelative(fileName);
		file.Write<mpt::PathString>(U_("Midi Library"), key, fileName);
	}
}


/////////////////////////////////////////////////////////////////////////////
// DLS Banks support

std::vector<std::unique_ptr<CDLSBank>> CTrackApp::gpDLSBanks;


struct CompareLessPathStringNoCase
{
	inline bool operator()(const mpt::PathString &l, const mpt::PathString &r) const
	{
		return mpt::PathCompareNoCase(l, r) < 0;
	}
};

std::future<std::vector<std::unique_ptr<CDLSBank>>> CTrackApp::LoadDefaultDLSBanks()
{
	std::set<mpt::PathString, CompareLessPathStringNoCase> paths;

	uint32 numBanks = theApp.GetSettings().Read<uint32>(U_("DLS Banks"), U_("NumBanks"), 0);
	for(uint32 i = 0; i < numBanks; i++)
	{
		mpt::PathString path = theApp.GetSettings().Read<mpt::PathString>(U_("DLS Banks"), MPT_UFORMAT("Bank{}")(i + 1), mpt::PathString());
		paths.insert(theApp.PathInstallRelativeToAbsolute(path));
	}

	HKEY key;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\DirectMusic"), 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		DWORD dwRegType = REG_SZ;
		DWORD dwSize = 0;
		if(RegQueryValueEx(key, _T("GMFilePath"), NULL, &dwRegType, nullptr, &dwSize) == ERROR_SUCCESS && dwSize > 0)
		{
			std::vector<TCHAR> filenameT(dwSize / sizeof(TCHAR));
			if(RegQueryValueEx(key, _T("GMFilePath"), NULL, &dwRegType, reinterpret_cast<LPBYTE>(filenameT.data()), &dwSize) == ERROR_SUCCESS)
			{
				mpt::winstring filenamestr = ParseMaybeNullTerminatedStringFromBufferWithSizeInBytes<mpt::winstring>(filenameT.data(), dwSize);
				std::vector<TCHAR> filenameExpanded(::ExpandEnvironmentStrings(filenamestr.c_str(), nullptr, 0));
				::ExpandEnvironmentStrings(filenamestr.c_str(), filenameExpanded.data(), static_cast<DWORD>(filenameExpanded.size()));
				auto filename = mpt::PathString::FromNative(filenameExpanded.data());
				ImportMidiConfig(filename, true);
				paths.insert(std::move(filename));
			}
		}
		RegCloseKey(key);
	}

	if(paths.empty())
		return {};

	return std::async(std::launch::async, [paths = std::move(paths)]()
	{
		std::vector<std::unique_ptr<CDLSBank>> banks;
		banks.reserve(paths.size());
		for(const auto &filename : paths)
		{
			if(filename.empty() || !CDLSBank::IsDLSBank(filename))
				continue;
			try
			{
				auto bank = std::make_unique<CDLSBank>();
				if(bank->Open(filename))
				{
					banks.push_back(std::move(bank));
					continue;
				}
			} catch(mpt::out_of_memory e)
			{
				mpt::delete_out_of_memory(e);
			} catch(const std::exception &)
			{
			}
		}
		// Avoid the overhead of future::wait_for(0) until future::is_ready is finally non-experimental
		theApp.m_scannedDlsBanksAvailable = true;
		return banks;
	});
}


void CTrackApp::SaveDefaultDLSBanks()
{
	uint32 nBanks = 0;
	for(const auto &bank : gpDLSBanks)
	{
		if(!bank || bank->GetFileName().empty())
			continue;

		mpt::PathString path = bank->GetFileName();
		if(theApp.IsPortableMode())
		{
			path = theApp.PathAbsoluteToInstallRelative(path);
		}

		mpt::ustring key = MPT_UFORMAT("Bank{}")(nBanks + 1);
		theApp.GetSettings().Write<mpt::PathString>(U_("DLS Banks"), key, path);
		nBanks++;

	}
	theApp.GetSettings().Write<uint32>(U_("DLS Banks"), U_("NumBanks"), nBanks);
}


void CTrackApp::RemoveDLSBank(UINT nBank)
{
	if(nBank < gpDLSBanks.size())
		gpDLSBanks[nBank] = nullptr;
}


bool CTrackApp::AddDLSBank(const mpt::PathString &filename)
{
	if(filename.empty() || !CDLSBank::IsDLSBank(filename)) return false;
	// Check for dupes
	for(const auto &bank : gpDLSBanks)
	{
		if(bank && !mpt::PathCompareNoCase(filename, bank->GetFileName()))
			return true;
	}
	try
	{
		auto bank = std::make_unique<CDLSBank>();
		if(bank->Open(filename))
		{
			gpDLSBanks.push_back(std::move(bank));
			return true;
		}
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
	} catch(const std::exception &)
	{
	}
	return false;
}


size_t CTrackApp::AddScannedDLSBanks()
{
	if(!m_scannedDlsBanks.valid())
		return 0;

	size_t numAdded = 0;
	auto scannedBanks = m_scannedDlsBanks.get();
	gpDLSBanks.reserve(gpDLSBanks.size() + scannedBanks.size());
	const size_t existingBanks = gpDLSBanks.size();
	for(auto &bank : scannedBanks)
	{
		if(std::find_if(gpDLSBanks.begin(), gpDLSBanks.begin() + existingBanks, [&bank](const auto &other) { return other && *bank == *other; }) == gpDLSBanks.begin() + existingBanks)
		{
			gpDLSBanks.push_back(std::move(bank));
			numAdded++;
		}
	}
	return numAdded;
}


/////////////////////////////////////////////////////////////////////////////
// CTrackApp

MODTYPE CTrackApp::m_nDefaultDocType = MOD_TYPE_IT;

BEGIN_MESSAGE_MAP(CTrackApp, CWinApp)
	//{{AFX_MSG_MAP(CTrackApp)
	ON_COMMAND(ID_FILE_NEW,       &CTrackApp::OnFileNew)
	ON_COMMAND(ID_FILE_NEWMOD,    &CTrackApp::OnFileNewMOD_Amiga)
	ON_COMMAND(ID_FILE_NEWMOD_PC, &CTrackApp::OnFileNewMOD_PC)
	ON_COMMAND(ID_FILE_NEWS3M,    &CTrackApp::OnFileNewS3M)
	ON_COMMAND(ID_FILE_NEWXM,     &CTrackApp::OnFileNewXM)
	ON_COMMAND(ID_FILE_NEWIT,     &CTrackApp::OnFileNewIT)
	ON_COMMAND(ID_NEW_MPT,        &CTrackApp::OnFileNewMPT)
	ON_COMMAND(ID_FILE_OPEN,      &CTrackApp::OnFileOpen)
	ON_COMMAND(ID_FILE_CLOSEALL,  &CTrackApp::OnFileCloseAll)
	ON_COMMAND(ID_APP_ABOUT,      &CTrackApp::OnAppAbout)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEALL, &CTrackApp::OnUpdateAnyDocsOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrackApp construction

CTrackApp::CTrackApp()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART | AFX_RESTART_MANAGER_REOPEN_PREVIOUS_FILES;
}


CTrackApp::~CTrackApp()
{
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO_DATE) && defined(MPT_LIBCXX_QUIRK_CHRONO_TZ_MEMLEAK)
	// Work-around memleak (see <https://github.com/microsoft/STL/issues/2504#issuecomment-1068008937>)
	try
	{
		std::chrono::get_tzdb_list().~tzdb_list();
	} catch(const std::exception &)
	{
		// nothing
	} catch(...)
	{
		// nothing
	}
#endif
}


class OpenMPTDataRecoveryHandler
	: public CDataRecoveryHandler
{
public:
	OpenMPTDataRecoveryHandler(_In_ DWORD dwRestartManagerSupportFlags, _In_ int nAutosaveInterval)
		: CDataRecoveryHandler(dwRestartManagerSupportFlags, nAutosaveInterval)
	{
		return;
	}
	~OpenMPTDataRecoveryHandler() override = default;

	BOOL SaveOpenDocumentList() override
	{
		BOOL bRet = TRUE;  // return FALSE if document list non-empty and not saved

		POSITION posAutosave = m_mapDocNameToAutosaveName.GetStartPosition();
		if (posAutosave != NULL)
		{
			bRet = FALSE;

			// Save the open document list and associated autosave info to the registry
			IniFileSettingsBackend ini(theApp.GetConfigPath() + P_("restart.") + mpt::PathString::FromCString(GetRestartIdentifier()) + P_(".ini"));
			ini.ConvertToUnicode();
			int32 count = 0;
			while (posAutosave != NULL)
			{
				CString strDocument, strAutosave;
				m_mapDocNameToAutosaveName.GetNextAssoc(posAutosave, strDocument, strAutosave);

				ini.WriteSetting({ U_("RestartDocument"), mpt::ufmt::val(count) }, SettingValue(mpt::ToUnicode(strDocument)));
				ini.WriteSetting({ U_("RestartAutosave"), mpt::ufmt::val(count) }, SettingValue(mpt::ToUnicode(strAutosave)));
				count++;
			}
			ini.WriteSetting({ U_("Restart"), U_("Count") }, SettingValue(count));

			return TRUE;
		}

		return bRet;
	}

	BOOL ReopenPreviousDocuments() override
	{
		// In case no previously open documents are to be re-opened,
		// MFC RestartManager integration assumes the default action to be
		// opening a new empty document (see CWinApp::ProcessShellCommand()
		// and CWinApp::RestartInstance().
		// We do not want that, so we always return TRUE.
		CDataRecoveryHandler::ReopenPreviousDocuments();
		return TRUE;
	}

	BOOL ReadOpenDocumentList() override
	{
		{
			IniFileSettingsBackend ini(theApp.GetConfigPath() + P_("restart.") + mpt::PathString::FromCString(GetRestartIdentifier()) + P_(".ini"));
			int32 count = ini.ReadSetting({ U_("Restart"), U_("Count") }, SettingValue(0));

			for(int32 index = 0; index < count; ++index)
			{
				mpt::ustring document = ini.ReadSetting({ U_("RestartDocument"), mpt::ufmt::val(index) }, SettingValue(U_("")));
				mpt::ustring autosave = ini.ReadSetting({ U_("RestartAutosave"), mpt::ufmt::val(index) }, SettingValue(U_("")));
				if(!document.empty())
				{
					m_mapDocNameToAutosaveName[mpt::ToCString(document)] = mpt::ToCString(autosave);
				}
			}

			ini.RemoveSection(U_("Restart"));
			ini.RemoveSection(U_("RestartDocument"));
			ini.RemoveSection(U_("RestartAutosave"));

		}

		DeleteFile((theApp.GetConfigPath() + P_("restart.") + mpt::PathString::FromCString(GetRestartIdentifier()) + P_(".ini")).AsNative().c_str());

		// In case no previously open documents are found,
		// MFC RestartManager integration assumes the default action to be
		// opening a new empty document (see CWinApp::RestartInstance() and
		// after the call to RestartInstance()).
		// We do not want that, so we always return TRUE.
		return TRUE;

	}

};


CDataRecoveryHandler *CTrackApp::GetDataRecoveryHandler()
{
	static BOOL bTriedOnce = FALSE;

	// Since the application restart and application recovery are supported only on Windows
	// Vista and above, we don't need a recovery handler on Windows versions less than Vista.
	if (SupportsRestartManager() || SupportsApplicationRecovery())
	{
		if (!bTriedOnce && m_pDataRecoveryHandler == NULL)
		{
			m_pDataRecoveryHandler = new OpenMPTDataRecoveryHandler(m_dwRestartManagerSupportFlags, m_nAutosaveInterval);
			if (!m_pDataRecoveryHandler->Initialize())
			{
				delete m_pDataRecoveryHandler;
				m_pDataRecoveryHandler = NULL;
			}
		}
	}

	bTriedOnce = TRUE;
	return m_pDataRecoveryHandler;
}


CDocument *CTrackApp::OpenTemplateFile(const mpt::PathString &file) const
{
	return GetModDocTemplate()->OpenTemplateFile(file);
}


void CTrackApp::AddToRecentFileList(LPCTSTR lpszPathName)
{
	AddToRecentFileList(mpt::PathString::FromCString(lpszPathName));
}


void CTrackApp::AddToRecentFileList(const mpt::PathString &path)
{
	RemoveMruItem(path);
	TrackerSettings::Instance().mruFiles.insert(TrackerSettings::Instance().mruFiles.begin(), path);
	if(TrackerSettings::Instance().mruFiles.size() > TrackerSettings::Instance().mruListLength)
	{
		TrackerSettings::Instance().mruFiles.resize(TrackerSettings::Instance().mruListLength);
	}
	CMainFrame::GetMainFrame()->UpdateMRUList();
}


void CTrackApp::RemoveMruItem(const size_t item)
{
	if(item < TrackerSettings::Instance().mruFiles.size())
	{
		TrackerSettings::Instance().mruFiles.erase(TrackerSettings::Instance().mruFiles.begin() + item);
		CMainFrame::GetMainFrame()->UpdateMRUList();
	}
}


void CTrackApp::RemoveMruItem(const mpt::PathString &path)
{
	auto &mruFiles = TrackerSettings::Instance().mruFiles;
	for(auto i = mruFiles.begin(); i != mruFiles.end(); i++)
	{
		if(!mpt::PathCompareNoCase(*i, path))
		{
			mruFiles.erase(i);
			break;
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CTrackApp initialization


namespace Tracker
{
mpt::recursive_mutex_with_lock_count & GetGlobalMutexRef()
{
	return theApp.GetGlobalMutexRef();
}
} // namespace Tracker


class ComponentManagerSettings
	: public IComponentManagerSettings
{
private:
	TrackerSettings &conf;
	mpt::PathString configPath;
public:
	ComponentManagerSettings(TrackerSettings &conf, const mpt::PathString &configPath)
		: conf(conf)
		, configPath(configPath)
	{
		return;
	}
	bool LoadOnStartup() const override
	{
		return conf.ComponentsLoadOnStartup;
	}
	bool KeepLoaded() const override
	{
		return conf.ComponentsKeepLoaded;
	}
	bool IsBlocked(const std::string &key) const override
	{
		return conf.IsComponentBlocked(key);
	}
};


SettingsContainer &CTrackApp::GetPluginCache()
{
	ASSERT(m_pPluginCache);
	return *m_pPluginCache;
}


SettingsContainer &CTrackApp::GetSongSettings()
{
	ASSERT(m_pSongSettings);
	return *m_pSongSettings;
}


const mpt::PathString &CTrackApp::GetSongSettingsFilename() const
{
	return m_pSongSettingsIniFile->GetFilename();
}


// Move a config file called fileName from the App's directory (or one of its sub directories specified by subDir) to
// %APPDATA%. If specified, it will be renamed to newFileName. Existing files are never overwritten.
// Returns true on success.
bool CTrackApp::MoveConfigFile(const mpt::PathString &fileName, mpt::PathString subDir, mpt::PathString newFileName)
{
	const mpt::PathString oldPath = GetInstallPath() + subDir + fileName;
	mpt::PathString newPath = GetConfigPath() + subDir;
	if(!newFileName.empty())
		newPath += newFileName;
	else
		newPath += fileName;

	if(!mpt::native_fs{}.is_file(newPath) && mpt::native_fs{}.is_file(oldPath))
	{
		return MoveFile(oldPath.AsNative().c_str(), newPath.AsNative().c_str()) != 0;
	}
	return false;
}


// Set up paths were configuration data is written to. Set overridePortable to true if application's own directory should always be used.
void CTrackApp::SetupPaths(bool overridePortable)
{

	// First, determine if the executable is installed in multi-arch mode or in the old standard mode.
	bool modeMultiArch = false;
	bool modeSourceProject = false;
	const mpt::PathString exePath = mpt::common_directories::get_application_directory();
	auto exePathComponents = mpt::split(exePath.GetDirectory().WithoutTrailingSlash().ToUnicode(), P_("\\").ToUnicode());
	if(exePathComponents.size() >= 2)
	{
		if(exePathComponents[exePathComponents.size()-1] == mpt::OS::Windows::Name(mpt::OS::Windows::GetProcessArchitecture()))
		{
			if(exePathComponents[exePathComponents.size()-2] == U_("bin"))
			{
				modeMultiArch = true;
			}
		}
	}
	// Check if we are running from the source tree.
	if(!modeMultiArch && exePathComponents.size() >= 4)
	{
		if(exePathComponents[exePathComponents.size()-1] == mpt::OS::Windows::Name(mpt::OS::Windows::GetProcessArchitecture()))
		{
			if(exePathComponents[exePathComponents.size()-4] == U_("bin"))
			{
				modeSourceProject = true;
			}
		}
	}
	if(modeSourceProject)
	{
		mpt::native_fs fs;
		m_InstallPath = fs.absolute(exePath + P_("..\\") + P_("..\\") + P_("..\\") + P_("..\\"));
		m_InstallBinPath = fs.absolute(exePath + P_("..\\"));
		m_InstallBinArchPath = exePath;
		m_InstallPkgPath = fs.absolute(exePath + P_("..\\") + P_("..\\") + P_("..\\") + P_("..\\packageTemplate\\"));
	} else if(modeMultiArch)
	{
		mpt::native_fs fs;
		m_InstallPath = fs.absolute(exePath + P_("..\\") + P_("..\\"));
		m_InstallBinPath = fs.absolute(exePath + P_("..\\"));
		m_InstallBinArchPath = exePath;
		m_InstallPkgPath = fs.absolute(exePath + P_("..\\") + P_("..\\"));
	} else
	{
		m_InstallPath = exePath;
		m_InstallBinPath = exePath;
		m_InstallBinArchPath = exePath;
		m_InstallPkgPath = exePath;
	}

	// Determine paths, portable mode, first run. Do not yet update any state.
	mpt::PathString configPathPortable = (modeSourceProject ? exePath : m_InstallPath); // config path in portable mode
	mpt::PathString configPathUser; // config path in default non-portable mode
	{
		// Try to find a nice directory where we should store our settings (default: %APPDATA%)
		TCHAR dir[MAX_PATH] = { 0 };
		if((SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK)
			|| (SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK))
		{
			// Store our app settings in %APPDATA% or "My Documents"
			configPathUser = mpt::PathString::FromNative(dir) + P_("\\OpenMPT\\");
		}
	}

	// Check if the user has configured portable mode.
	bool configInstallPortable = false;
	mpt::PathString portableFlagFilename = (configPathPortable + P_("OpenMPT.portable"));
	bool configPortableFlag = mpt::native_fs{}.is_file(portableFlagFilename);
	configInstallPortable = configInstallPortable || configPortableFlag;
	// before 1.29.00.13:
	configInstallPortable = configInstallPortable || (GetPrivateProfileInt(_T("Paths"), _T("UseAppDataDirectory"), 1, (configPathPortable + P_("mptrack.ini")).AsNative().c_str()) == 0);
	// convert to new style
	if(configInstallPortable && !configPortableFlag)
	{
		mpt::IO::SafeOutputFile f(portableFlagFilename);
	}

	// Determine portable mode.
	bool portableMode = overridePortable || configInstallPortable || configPathUser.empty();

	// Update config dir
	m_ConfigPath = portableMode ? configPathPortable : configPathUser;

	// Set up default file locations
	m_szConfigFileName = m_ConfigPath + P_("mptrack.ini"); // config file
	m_szPluginCacheFileName = m_ConfigPath + P_("plugin.cache"); // plugin cache

	// Force use of custom ini file rather than windowsDir\executableName.ini
	if(m_pszProfileName)
	{
		free((void *)m_pszProfileName);
	}
	m_pszProfileName = _tcsdup(m_szConfigFileName.ToCString());

	m_bInstallerMode = !modeSourceProject && !portableMode;
	m_bPortableMode = portableMode;
	m_bSourceTreeMode = modeSourceProject;

}


void CTrackApp::CreatePaths()
{
	// Create missing directories
	if(!IsPortableMode())
	{
		if(!mpt::native_fs{}.is_directory(m_ConfigPath))
		{
			CreateDirectory(m_ConfigPath.AsNative().c_str(), 0);
		}
	}

	// Handle updates from old versions.
	if(!IsPortableMode())
	{
		// Move the config files if they're still in the old place.
		MoveConfigFile(P_("mptrack.ini"));
		MoveConfigFile(P_("plugin.cache"));
	
		// Import old tunings
		const mpt::PathString oldTunings = GetInstallPath() + P_("tunings\\");

		if(mpt::native_fs{}.is_directory(oldTunings))
		{
			FolderScanner scanner{oldTunings, FolderScanner::kOnlyFiles};
			mpt::PathString fileName;
			while(scanner.Next(fileName))
			{
				MoveConfigFile(fileName.GetFilename(), P_("tunings\\"));
			}
			RemoveDirectory(oldTunings.AsNative().c_str());
		}
	}
}


mpt::PathString CTrackApp::GetUserTemplatesPath() const
{
	return GetConfigPath() + P_("TemplateModules\\");
}


mpt::PathString CTrackApp::GetExampleSongsPath() const
{
	return GetInstallPath() + P_("ExampleSongs\\");
}


#if !defined(MPT_BUILD_RETRO)

static bool ProcessorCanRunCurrentBuild()
{
#ifdef MPT_ENABLE_ARCH_INTRINSICS
	if(!mpt::arch::get_cpu_info().has_features(mpt::arch::current::assumed_features()))
	{
		return false;
	}
#endif // MPT_ENABLE_ARCH_INTRINSICS
	return true;
}

static bool SystemCanRunCurrentBuild()
{
	if(mpt::OS::Windows::IsOriginal())
	{
		if(mpt::osinfo::windows::Version::Current().IsBefore(mpt::OS::Windows::Version::GetMinimumKernelLevel()))
		{
			return false;
		}
		if(mpt::osinfo::windows::Version::Current().IsBefore(mpt::OS::Windows::Version::GetMinimumAPILevel()))
		{
			return false;
		}
	} else if(mpt::OS::Windows::IsWine() && theApp.GetWineVersion()->Version().IsValid())
	{
		if(theApp.GetWineVersion()->Version().IsBefore(mpt::OS::Wine::GetMinimumWineVersion()))
		{
			return false;
		}
	}
	return true;
}

bool CTrackApp::CheckSystemSupport()
{
	const mpt::ustring lf = U_("\n");
	const mpt::ustring url = Build::GetURL(Build::Url::Download);
	if(!ProcessorCanRunCurrentBuild())
	{
		mpt::ustring text;
		text += U_("Your CPU is too old to run this variant of OpenMPT.") + lf;
		text += U_("OpenMPT will exit now.") + lf;
		Reporting::Error(text, "OpenMPT");
		return false;
	}
	if(!SystemCanRunCurrentBuild())
	{
		if(mpt::OS::Windows::IsOriginal())
		{
			mpt::ustring text;
			text += U_("Your system does not meet the minimum requirements for this variant of OpenMPT.") + lf;
			text += U_("OpenMPT will exit now.") + lf;
			Reporting::Error(text, "OpenMPT");
			return false;
		} else
		{
			mpt::ustring text;
			text += U_("Your system does not meet the minimum requirements for this variant of OpenMPT.") + lf;
			Reporting::Error(text, "OpenMPT");
			// may work though
		}
	}
	return true;
}

#endif // !MPT_BUILD_RETRO


BOOL CTrackApp::InitInstanceEarly(CMPTCommandLineInfo &cmdInfo)
{
	// The first step of InitInstance, always executed without any crash handler.

	#ifndef UNICODE
		if(MessageBox(NULL,
			_T("STOP!!!") _T("\n")
			_T("This is an ANSI (as opposed to a UNICODE) build of OpenMPT.") _T("\n")
			_T("\n")
			_T("ANSI builds are NOT SUPPORTED and WILL CAUSE CORRUPTION of the OpenMPT configuration and exhibit other unintended behaviour.") _T("\n")
			_T("\n")
			_T("Please use an official build of OpenMPT or compile 'OpenMPT.sln' instead of 'OpenMPT-ANSI.sln'.") _T("\n")
			_T("\n")
			_T("Continue starting OpenMPT anyway?") _T("\n"),
			_T("OpenMPT"), MB_ICONSTOP | MB_YESNO| MB_DEFBUTTON2)
			!= IDYES)
		{
			ExitProcess(1);
		}
	#endif

	// Call the base class.
	// This is required for MFC RestartManager integration.
	if(!CWinApp::InitInstance())
	{
		return FALSE;
	}

	#if MPT_COMPILER_MSVC
		_CrtSetDebugFillThreshold(0); // Disable buffer filling in secure enhanced CRT functions.
	#endif

	// Avoid e.g. audio APIs trying to load wdmaud.drv from arbitrary working directory
	::SetCurrentDirectory(mpt::common_directories::get_application_directory().AsNative().c_str());

	// Initialize OLE MFC support
	[[maybe_unused]] BOOL oleinit = AfxOleInit();
	ASSERT(oleinit != FALSE); // no MPT_ASSERT here!

	// Parse command line for standard shell commands, DDE, file open
	ParseCommandLine(cmdInfo);

	// Set up paths to store configuration in
	SetupPaths(cmdInfo.m_portable);

	if(cmdInfo.m_sharedInstance && IPCWindow::SendToIPC(cmdInfo.m_fileNames, cmdInfo.m_autoPlay))
	{
		ExitProcess(0);
	}

	// Initialize DocManager (for DDE)
	// requires mpt::PathString
	ASSERT(nullptr == m_pDocManager); // no MPT_ASSERT here!
	m_pDocManager = new CModDocManager();

	if(IsDebuggerPresent() && cmdInfo.m_debugCrashHandler)
	{
		ExceptionHandler::useAnyCrashHandler = true;
		ExceptionHandler::useImplicitFallbackSEH = false;
		ExceptionHandler::useExplicitSEH = true;
		ExceptionHandler::handleStdTerminate = true;
		ExceptionHandler::handleMfcExceptions = true;
		ExceptionHandler::debugExceptionHandler = true;
	} else if(IsDebuggerPresent() || cmdInfo.m_noCrashHandler)
	{
		ExceptionHandler::useAnyCrashHandler = false;
		ExceptionHandler::useImplicitFallbackSEH = false;
		ExceptionHandler::useExplicitSEH = false;
		ExceptionHandler::handleStdTerminate = false;
		ExceptionHandler::handleMfcExceptions = false;
		ExceptionHandler::debugExceptionHandler = false;
	} else
	{
		ExceptionHandler::useAnyCrashHandler = true;
		ExceptionHandler::useImplicitFallbackSEH = true;
		ExceptionHandler::useExplicitSEH = true;
		ExceptionHandler::handleStdTerminate = true;
		ExceptionHandler::handleMfcExceptions = true;
		ExceptionHandler::debugExceptionHandler = false;
	}

	return TRUE;
}


BOOL CTrackApp::InitInstanceImpl(CMPTCommandLineInfo &cmdInfo)
{

	m_GuiThreadId = GetCurrentThreadId();

	mpt::log::Trace::SetThreadId(mpt::log::Trace::ThreadKindGUI, m_GuiThreadId);

	if(ExceptionHandler::useAnyCrashHandler)
	{
		ExceptionHandler::Register();
	}

	// Start loading
	BeginWaitCursor();

	MPT_LOG_GLOBAL(LogInformation, "", U_("OpenMPT Start"));

	// create the tracker-global random device
	m_RD = std::make_unique<mpt::random_device>();
	// make the device available to non-tracker-only code
	mpt::set_global_random_device(m_RD.get());
	// create and seed the traker-global best PRNG with the random device
	m_PRNG = std::make_unique<mpt::thread_safe_prng<mpt::default_prng> >(mpt::make_prng<mpt::default_prng>(RandomDevice()));
	// make the best PRNG available to non-tracker-only code
	mpt::set_global_prng(m_PRNG.get());
	// additionally, seed the C rand() PRNG, just in case any third party library calls rand()
	mpt::crand::reseed(RandomDevice());

	m_Gdiplus = std::make_unique<GdiplusRAII>();

	if(cmdInfo.m_noWine)
	{
		mpt::OS::Windows::PreventWineDetection();
	}

	#ifdef MPT_ENABLE_ARCH_INTRINSICS
		if(!cmdInfo.m_noAssembly)
		{
			CPU::EnableAvailableFeatures();
		}
	#endif // MPT_ENABLE_ARCH_INTRINSICS

	if(mpt::OS::Windows::IsWine())
	{
		SetWineVersion(std::make_shared<mpt::OS::Wine::VersionContext>());
	}

	// Create paths to store configuration in
	CreatePaths();

	m_pSettingsIniFile = new IniFileSettingsBackend(m_szConfigFileName);
	m_pSettings = new SettingsContainer(m_pSettingsIniFile);

	m_pDebugSettings = new DebugSettings(*m_pSettings);

	m_pTrackerSettings = new TrackerSettings(*m_pSettings);

	MPT_LOG_GLOBAL(LogInformation, "", U_("OpenMPT settings initialized."));

	if(ExceptionHandler::useAnyCrashHandler)
	{
		ExceptionHandler::ConfigureSystemHandler();
	}

	if(TrackerSettings::Instance().MiscUseSingleInstance && IPCWindow::SendToIPC(cmdInfo.m_fileNames, cmdInfo.m_autoPlay))
	{
		ExitProcess(0);
	}

	IPCWindow::Open(m_hInstance);

	m_pSongSettingsIniFile = new IniFileSettingsBackend(GetConfigPath() + P_("SongSettings.ini"));
	m_pSongSettings = new SettingsContainer(m_pSongSettingsIniFile);

	m_pComponentManagerSettings = new ComponentManagerSettings(TrackerSettings::Instance(), GetConfigPath());

	m_pPluginCache = new IniFileSettingsContainer(m_szPluginCacheFileName);

	// Load standard INI file options (without MRU)
	// requires SetupPaths+CreatePaths called
	LoadStdProfileSettings(0);

// Set process priority class
#ifndef _DEBUG
	SetPriorityClass(GetCurrentProcess(), TrackerSettings::Instance().MiscProcessPriorityClass);
#endif

	// Dynamic DPI-awareness. Some users might want to disable DPI-awareness because of their DPI-unaware VST plugins.
	switch(TrackerSettings::Instance().dpiAwareness)
	{
	case DPIAwarenessMode::NoDPIAwareness:
		HighDPISupport::SetDPIAwareness(HighDPISupport::Mode::LowDpi);
		break;
	case DPIAwarenessMode::NoDPIAwarenessGDIUpscaled:
		HighDPISupport::SetDPIAwareness(HighDPISupport::Mode::LowDpiUpscaled);
		break;
	case DPIAwarenessMode::SystemDPIAware:
		HighDPISupport::SetDPIAwareness(HighDPISupport::Mode::HighDpiSystem);
		break;
	case DPIAwarenessMode::PerMonitorDPIAware:
		HighDPISupport::SetDPIAwareness(HighDPISupport::Mode::HighDpiPerMonitor);
		break;
	default:
		// Malformed entry in INI file
		TrackerSettings::Instance().dpiAwareness = DPIAwarenessMode::PerMonitorDPIAware;
		HighDPISupport::SetDPIAwareness(HighDPISupport::Mode::HighDpiPerMonitor);
		break;
	}

	// create main MDI Frame window
	CMainFrame *pMainFrame = new CMainFrame();
	if(!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Show splash screen
	if(cmdInfo.m_bShowSplash && TrackerSettings::Instance().m_ShowSplashScreen)
	{
		StartSplashScreen();
	}

	// create component manager
	ComponentManager::Init(*m_pComponentManagerSettings);

	// load components
	ComponentManager::Instance()->Startup();

	// Wine Support
	if(mpt::OS::Windows::IsWine())
	{
		WineIntegration::Initialize();
		WineIntegration::Load();
	}

	// Register document templates
	m_pModTemplate = new CModDocTemplate(
		IDR_MODULETYPE,
		RUNTIME_CLASS(CModDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CModControlView));
	AddDocTemplate(m_pModTemplate);

	// Enable DDE Execute open
	// requires m_pDocManager
	EnableShellOpen();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Load sound APIs
	// requires TrackerSettings
	m_pAllSoundDeviceComponents = std::make_unique<AllSoundDeviceComponents>();
	auto GetSysInfo = [&]()
	{
		if(mpt::OS::Windows::IsWine())
		{
			return SoundDevice::SysInfo(mpt::osinfo::get_class(), mpt::osinfo::windows::Version::Current(), mpt::OS::Windows::IsWine(), GetWineVersion()->HostClass(), GetWineVersion()->Version());
		}
		return SoundDevice::SysInfo(mpt::osinfo::get_class(), mpt::osinfo::windows::Version::Current(), mpt::OS::Windows::IsWine(), mpt::osinfo::osclass::Unknown, mpt::osinfo::windows::wine::version());
	};
	SoundDevice::SysInfo sysInfo = GetSysInfo();
	SoundDevice::AppInfo appInfo;
	appInfo.SetName(U_("OpenMPT"));
	appInfo.SetHWND(*m_pMainWnd);
	appInfo.BoostedThreadPriorityXP = TrackerSettings::Instance().SoundBoostedThreadPriority;
	appInfo.BoostedThreadMMCSSClassVista = TrackerSettings::Instance().SoundBoostedThreadMMCSSClass;
	appInfo.BoostedThreadRealtimePosix = TrackerSettings::Instance().SoundBoostedThreadRealtimePosix;
	appInfo.BoostedThreadNicenessPosix = TrackerSettings::Instance().SoundBoostedThreadNicenessPosix;
	appInfo.BoostedThreadRtprioPosix = TrackerSettings::Instance().SoundBoostedThreadRtprioPosix;
	appInfo.MaskDriverCrashes = TrackerSettings::Instance().SoundMaskDriverCrashes;
	appInfo.AllowDeferredProcessing = TrackerSettings::Instance().SoundAllowDeferredProcessing;
	std::vector<std::shared_ptr<SoundDevice::IDevicesEnumerator>> deviceEnumerators = SoundDevice::Manager::GetEnabledEnumerators(*m_pAllSoundDeviceComponents);
	deviceEnumerators.push_back(std::static_pointer_cast<SoundDevice::IDevicesEnumerator>(std::make_shared<SoundDevice::DevicesEnumerator<SoundDevice::SoundDeviceStub>>()));
	m_pSoundDevicesManager = std::make_unique<SoundDevice::Manager>(m_GlobalLogger, sysInfo, appInfo, std::move(deviceEnumerators));
	m_pTrackerSettings->MigrateOldSoundDeviceSettings(*m_pSoundDevicesManager);

	// Set default note names
	CSoundFile::SetDefaultNoteNames();

	// Load Soundfonts and default MIDI Library
	if (!cmdInfo.m_noDls)
		m_scannedDlsBanks = LoadDefaultDLSBanks();
	// Load user-defined MIDI Library
	ImportMidiConfig(theApp.GetSettings(), {}, true);

	// Initialize Plugins
	if (!cmdInfo.m_noPlugins) InitializeDXPlugins();

	// Initialize CMainFrame
	pMainFrame->Initialize();

	// Dispatch commands specified on the command line
	if(cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
	{
		// When not asked to open any existing file,
		// we do not want to open an empty new one on startup.
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	}
	bool shellSuccess = false;
	if(cmdInfo.m_fileNames.empty())
	{
		shellSuccess = ProcessShellCommand(cmdInfo) != FALSE;
	} else
	{
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileOpen;
		for(const auto &filename : cmdInfo.m_fileNames)
		{
			cmdInfo.m_strFileName = filename.ToCString();
			shellSuccess |= ProcessShellCommand(cmdInfo) != FALSE;
		}
	}
	if(!shellSuccess)
	{
		EndWaitCursor();
		StopSplashScreen();
		return FALSE;
	}

	pMainFrame->UpdateDocumentCount();
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	EndWaitCursor();


	// Perform startup tasks.

#if !defined(MPT_BUILD_RETRO)
	// Check whether we are running the best build for the given system.
	if(!cmdInfo.m_noSysCheck)
	{
		if(!CheckSystemSupport())
		{
			StopSplashScreen();
			return FALSE;
		}
	}
#endif // !MPT_BUILD_RETRO

	if(TrackerSettings::Instance().FirstRun)
	{
		new WelcomeDlg(m_pMainWnd);
	} else
	{
#if !defined(MPT_BUILD_RETRO)
		bool deprecatedSoundDevice = GetSoundDevicesManager()->FindDeviceInfo(TrackerSettings::Instance().GetSoundDeviceIdentifier()).IsDeprecated();
		bool showSettings = deprecatedSoundDevice && !TrackerSettings::Instance().m_SoundDeprecatedDeviceWarningShown && (Reporting::Confirm(
			U_("You have currently selected a sound device which is deprecated. MME/WaveOut support will be removed in a future OpenMPT version.\n") +
			U_("The recommended sound device type is WASAPI.\n") +
			U_("Do you want to change your sound device settings now?"),
			U_("OpenMPT - Deprecated sound device")
			) == cnfYes);
		if(showSettings)
		{
			TrackerSettings::Instance().m_SoundDeprecatedDeviceWarningShown = true;
			m_pMainWnd->PostMessage(WM_COMMAND, ID_VIEW_OPTIONS);
		}
#endif // !MPT_BUILD_RETRO
	}

#ifdef ENABLE_TESTS
	if(!cmdInfo.m_noTests)
		Test::DoTests();
#endif

	if(TrackerSettings::Instance().m_SoundSettingsOpenDeviceAtStartup)
	{
		pMainFrame->InitPreview();
		pMainFrame->PreparePreview(NOTE_NOTECUT, 0);
		pMainFrame->PlayPreview();
	}

	if(cmdInfo.m_autoPlay)
	{
		pMainFrame->PlayMod(pMainFrame->GetActiveDoc());
	}

	if(!TrackerSettings::Instance().FirstRun)
	{
#if defined(MPT_ENABLE_UPDATE)
		if(CUpdateCheck::IsSuitableUpdateMoment())
		{
			CUpdateCheck::DoAutoUpdateCheck();
		}
#endif // MPT_ENABLE_UPDATE
	}

	return TRUE;
}


BOOL CTrackApp::InitInstance()
{
	CMPTCommandLineInfo cmdInfo;
	if(!InitInstanceEarly(cmdInfo))
	{
		return FALSE;
	}
	return InitInstanceLate(cmdInfo);
}


BOOL CTrackApp::InitInstanceLate(CMPTCommandLineInfo &cmdInfo)
{
	BOOL result = FALSE;
	if(ExceptionHandler::useExplicitSEH)
	{
		// https://support.microsoft.com/en-us/kb/173652
		__try
		{
			result = InitInstanceImpl(cmdInfo);
		} __except(ExceptionHandler::ExceptionFilter(GetExceptionInformation()))
		{
			std::abort();
		}
	} else
	{
		result = InitInstanceImpl(cmdInfo);
	}
	return result;
}


int CTrackApp::Run()
{
	int result = 255;
	if(ExceptionHandler::useExplicitSEH)
	{
		// https://support.microsoft.com/en-us/kb/173652
		__try
		{
			result = CWinApp::Run();
		} __except(ExceptionHandler::ExceptionFilter(GetExceptionInformation()))
		{
			std::abort();
		}
	} else
	{
		result = CWinApp::Run();
	}
	return result;
}


LRESULT CTrackApp::ProcessWndProcException(CException * e, const MSG * pMsg)
{
	if(ExceptionHandler::handleMfcExceptions)
	{
		LRESULT result = 0L; // as per documentation
		if(pMsg)
		{
			if(pMsg->message == WM_COMMAND)
			{
				result = (LRESULT)TRUE; // as per documentation
			}
		}
		if(dynamic_cast<CMemoryException*>(e))
		{
			e->ReportError();
			//ExceptionHandler::UnhandledMFCException(e, pMsg);
		} else
		{
			ExceptionHandler::UnhandledMFCException(e, pMsg);
		}
		return result;
	} else
	{
		return CWinApp::ProcessWndProcException(e, pMsg);
	}
}


int CTrackApp::ExitInstance()
{
	int result = 0;
	if(ExceptionHandler::useExplicitSEH)
	{
		// https://support.microsoft.com/en-us/kb/173652
		__try
		{
			result = ExitInstanceImpl();
		} __except(ExceptionHandler::ExceptionFilter(GetExceptionInformation()))
		{
			std::abort();
		}
	} else
	{
		result = ExitInstanceImpl();
	}
	return result;
}


int CTrackApp::ExitInstanceImpl()
{
	IPCWindow::Close();

	m_pSoundDevicesManager = nullptr;
	m_pAllSoundDeviceComponents = nullptr;
	ExportMidiConfig(theApp.GetSettings());
	AddScannedDLSBanks();
	SaveDefaultDLSBanks();
	gpDLSBanks.clear();

	// Uninitialize Plugins
	UninitializeDXPlugins();

	ComponentManager::Release();

	delete m_pPluginCache;
	m_pPluginCache = nullptr;
	delete m_pComponentManagerSettings;
	m_pComponentManagerSettings = nullptr;
	delete m_pTrackerSettings;
	m_pTrackerSettings = nullptr;
	delete m_pDebugSettings;
	m_pDebugSettings = nullptr;
	delete m_pSettings;
	m_pSettings = nullptr;
	delete m_pSettingsIniFile;
	m_pSettingsIniFile = nullptr;
	delete m_pSongSettings;
	m_pSongSettings = nullptr;
	delete m_pSongSettingsIniFile;
	m_pSongSettingsIniFile = nullptr;

	if(mpt::OS::Windows::IsWine())
	{
		SetWineVersion(nullptr);
	}

	m_Gdiplus.reset();

	mpt::set_global_prng(nullptr);
	m_PRNG.reset();
	mpt::set_global_random_device(nullptr);
	m_RD.reset();

	if(ExceptionHandler::useAnyCrashHandler)
	{
		ExceptionHandler::UnconfigureSystemHandler();
		ExceptionHandler::Unregister();
	}

	return CWinApp::ExitInstance();
}


////////////////////////////////////////////////////////////////////////////////
// App Messages


CModDoc *CTrackApp::NewDocument(MODTYPE newType)
{
	// Build from template
	if(newType == MOD_TYPE_NONE)
	{
		const mpt::PathString templateFile = TrackerSettings::Instance().defaultTemplateFile;
		if(TrackerSettings::Instance().defaultNewFileAction == nfDefaultTemplate && !templateFile.empty())
		{
			// Template file can be either a filename inside one of the preset and user TemplateModules folders, or a full path.
			const mpt::PathString dirs[] = { GetUserTemplatesPath(), GetInstallPath() + P_("TemplateModules\\"), mpt::PathString() };
			for(const auto &dir : dirs)
			{
				if(mpt::native_fs{}.is_file(dir + templateFile))
				{
					if(CModDoc *modDoc = static_cast<CModDoc *>(m_pModTemplate->OpenTemplateFile(dir + templateFile)))
					{
						return modDoc;
					}
				}
			}
		}


		// Default module type
		newType = TrackerSettings::Instance().defaultModType;

		// Get active document to make the new module of the same type
		CModDoc *pModDoc = CMainFrame::GetMainFrame()->GetActiveDoc();
		if(pModDoc != nullptr && TrackerSettings::Instance().defaultNewFileAction == nfSameAsCurrent)
		{
			newType = pModDoc->GetSoundFile().GetBestSaveFormat();
		}
	}

	SetDefaultDocType(newType);
	return static_cast<CModDoc *>(m_pModTemplate->OpenDocumentFile(_T("")));
}


void CTrackApp::OpenModulesDialog(std::vector<mpt::PathString> &files, const mpt::PathString &overridePath)
{
	files.clear();

	static constexpr std::string_view commonExts[] = {"mod", "s3m", "xm", "it", "mptm", "mo3", "oxm", "nst", "stk", "m15", "pt36", "mid", "rmi", "smf", "wav", "mdz", "s3z", "xmz", "itz", "mdr"};
	std::string exts, extsWithoutCommon;
	for(const auto &ext : CSoundFile::GetSupportedExtensions(true))
	{
		const auto filter = std::string("*.") + ext + std::string(";");
		exts += filter;
		if(!mpt::contains(commonExts, ext))
			extsWithoutCommon += filter;
	}

	static int nFilterIndex = 0;
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.ExtensionFilter("All Modules (*.mptm,*.mod,*.xm,*.s3m,*.it,...)|" + exts + ";mod.*"
		"|"
		"Compressed Modules (*.mdz,*.s3z,*.xmz,*.itz,*.mo3,*.oxm,...)|*.mdz;*.s3z;*.xmz;*.itz;*.mdr;*.zip;*.rar;*.lha;*.pma;*.lzs;*.gz;*.mo3;*.oxm"
		"|"
		"ProTracker Modules (*.mod,*.nst)|*.mod;mod.*;*.mdz;*.nst;*.m15;*.stk;*.pt36|"
		"Scream Tracker Modules (*.s3m,*.stm)|*.s3m;*.stm;*.s3z;*.stx|"
		"FastTracker Modules (*.xm)|*.xm;*.xmz|"
		"Impulse Tracker Modules (*.it)|*.it;*.itz|"
		"OpenMPT Modules (*.mptm)|*.mptm;*.mptmz|"
		"Other Modules (*.mtm,*.okt,*.mdl,*.669,*.far,...)|" + extsWithoutCommon + "|"
		"Wave Files (*.wav)|*.wav|"
		"MIDI Files (*.mid,*.rmi)|*.mid;*.rmi;*.smf|"
		"All Files (*.*)|*.*||")
		.WorkingDirectory(overridePath.empty() ? TrackerSettings::Instance().PathSongs.GetWorkingDir() : overridePath)
		.FilterIndex(&nFilterIndex);
	if(!dlg.Show()) return;

	if(overridePath.empty())
		TrackerSettings::Instance().PathSongs.SetWorkingDir(dlg.GetWorkingDirectory());

	files = dlg.GetFilenames();
}


void CTrackApp::OnFileOpen()
{
	FileDialog::PathList files;
	OpenModulesDialog(files);
	for(const auto &file : files)
	{
		OpenDocumentFile(file.ToCString());
	}
}


// App command to run the dialog
void CTrackApp::OnAppAbout()
{
	if (CAboutDlg::instance) return;
	CAboutDlg::instance = new CAboutDlg();
	CAboutDlg::instance->Create(IDD_ABOUTBOX, m_pMainWnd);
}


/////////////////////////////////////////////////////////////////////////////
// Splash Screen

class CSplashScreen : public DialogBase
{
protected:
	std::unique_ptr<Gdiplus::Image> m_Image;

public:
	~CSplashScreen();
	BOOL OnInitDialog() override;
	void OnOK() override;
	void OnCancel() override { OnOK(); }
	void OnPaint();
	BOOL OnEraseBkgnd(CDC *) { return TRUE; }

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CSplashScreen, DialogBase)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

static CSplashScreen *gpSplashScreen = nullptr;

static DWORD64 gSplashScreenStartTime = 0;


CSplashScreen::~CSplashScreen()
{
	gpSplashScreen = nullptr;
}


void CSplashScreen::OnPaint()
{
	CPaintDC dc(this);
	Gdiplus::Graphics gfx(dc);

	CRect rect;
	GetClientRect(&rect);
	gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQuality);
	gfx.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	gfx.DrawImage(m_Image.get(), 0, 0, rect.right, rect.bottom);

	DialogBase::OnPaint();
}


BOOL CSplashScreen::OnInitDialog()
{
	DialogBase::OnInitDialog();

	try
	{
		m_Image = GDIP::LoadPixelImage(GetResource(MAKEINTRESOURCE(IDB_SPLASHNOFOLDFIN), _T("PNG")));
	} catch(const bad_image &)
	{
		return FALSE;
	}

	CRect rect;
	GetWindowRect(&rect);
	const int width = HighDPISupport::ScalePixels(m_Image->GetWidth(), m_hWnd) / 2;
	const int height = HighDPISupport::ScalePixels(m_Image->GetHeight(), m_hWnd) / 2;
	SetWindowPos(nullptr,
		rect.left - ((width - rect.Width()) / 2),
		rect.top - ((height - rect.Height()) / 2),
		width,
		height,
		SWP_NOZORDER | SWP_NOCOPYBITS);

	return TRUE;
}


void CSplashScreen::OnOK()
{
	StopSplashScreen();
}


static void StartSplashScreen()
{
	if(!gpSplashScreen)
	{
		gpSplashScreen = new CSplashScreen();
		gpSplashScreen->Create(IDD_SPLASHSCREEN, theApp.m_pMainWnd);
		gpSplashScreen->ShowWindow(SW_SHOW);
		gpSplashScreen->UpdateWindow();
		gpSplashScreen->BeginWaitCursor();
		gSplashScreenStartTime = Util::GetTickCount64();
	}
}


static void StopSplashScreen()
{
	if(gpSplashScreen)
	{
		gpSplashScreen->EndWaitCursor();
		gpSplashScreen->DestroyWindow();
		delete gpSplashScreen;
		gpSplashScreen = nullptr;
	}
}


static void TimeoutSplashScreen()
{
	if(gpSplashScreen)
	{
		if(Util::GetTickCount64() - gSplashScreenStartTime > 100)
		{
			StopSplashScreen();
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// Idle-time processing

BOOL CTrackApp::OnIdle(LONG lCount)
{
	BOOL b = CWinApp::OnIdle(lCount);

	TimeoutSplashScreen();

	if(CMainFrame::GetMainFrame())
	{
		CMainFrame::GetMainFrame()->IdleHandlerSounddevice();

		if(m_scannedDlsBanksAvailable)
		{
			if(AddScannedDLSBanks())
				CMainFrame::GetMainFrame()->RefreshDlsBanks();
		}
	}

	// Call plugins idle routine for open editor
	if (m_pPluginManager)
	{
		DWORD curTime = timeGetTime();
		//rewbs.vstCompliance: call @ 50Hz
		if (curTime - m_dwLastPluginIdleCall > 20 || curTime < m_dwLastPluginIdleCall)
		{
			m_pPluginManager->OnIdle();
			m_dwLastPluginIdleCall = curTime;
		}
	}

	return b;
}


/////////////////////////////////////////////////////////////////////////////
// DIB


RGBQUAD rgb2quad(COLORREF c)
{
	RGBQUAD r;
	r.rgbBlue = GetBValue(c);
	r.rgbGreen = GetGValue(c);
	r.rgbRed = GetRValue(c);
	r.rgbReserved = 0;
	return r;
}


void DibBlt(HDC hdc, int x, int y, int sizex, int sizey, int srcx, int srcy, MODPLUGDIB *lpdib)
{
	if (!lpdib) return;
	SetDIBitsToDevice(	hdc,
						x,
						y,
						sizex,
						sizey,
						srcx,
						lpdib->bmiHeader.biHeight - srcy - sizey,
						0,
						lpdib->bmiHeader.biHeight,
						lpdib->lpDibBits,
						(LPBITMAPINFO)lpdib,
						DIB_RGB_COLORS);
}


MODPLUGDIB *LoadDib(LPCTSTR lpszName)
{
	mpt::const_byte_span data = GetResource(lpszName, RT_BITMAP);
	if(!data.data())
	{
		return nullptr;
	}
	LPBITMAPINFO p = (LPBITMAPINFO)data.data();
	MODPLUGDIB *pmd = new MODPLUGDIB;
	pmd->bmiHeader = p->bmiHeader;
	for (int i=0; i<16; i++) pmd->bmiColors[i] = p->bmiColors[i];
	LPBYTE lpDibBits = (LPBYTE)p;
	lpDibBits += p->bmiHeader.biSize + 16 * sizeof(RGBQUAD);
	pmd->lpDibBits = lpDibBits;
	return pmd;
}

int DrawTextT(HDC hdc, const wchar_t *lpchText, int cchText, LPRECT lprc, UINT format)
{
	return ::DrawTextW(hdc, lpchText, cchText, lprc, format);
}

int DrawTextT(HDC hdc, const char *lpchText, int cchText, LPRECT lprc, UINT format)
{
	return ::DrawTextA(hdc, lpchText, cchText, lprc, format);
}

template<typename Tchar>
static void DrawButtonRectImpl(HDC hdc, int lineWidth, HFONT font, CRect rect, const Tchar *text, bool disabled, bool pushed, DWORD textFlags, uint32 topMargin)
{
	if(lineWidth != 1)
	{
		// Draw "real" buttons in Hi-DPI mode
		DrawFrameControl(hdc, rect, DFC_BUTTON, pushed ? (DFCS_PUSHED | DFCS_BUTTONPUSH) : DFCS_BUTTONPUSH);
	} else
	{
		const auto colorHighlight = GetSysColor(COLOR_BTNHIGHLIGHT), colorShadow = GetSysColor(COLOR_BTNSHADOW);
		auto oldpen = SelectPen(hdc, GetStockObject(DC_PEN));
		::SetDCPenColor(hdc, pushed ? colorShadow : colorHighlight);
		::FillRect(hdc, rect, GetSysColorBrush(COLOR_BTNFACE));
		::MoveToEx(hdc, rect.left, rect.bottom - 1, nullptr);
		::LineTo(hdc, rect.left, rect.top);
		::LineTo(hdc, rect.right - 1, rect.top);
		::SetDCPenColor(hdc, pushed ? colorHighlight : colorShadow);
		::LineTo(hdc, rect.right - 1, rect.bottom - 1);
		::LineTo(hdc, rect.left, rect.bottom - 1);
		SelectPen(hdc, oldpen);
	}
	
	if(text && text[0])
	{
		rect.DeflateRect(lineWidth, lineWidth);
		if(pushed)
		{
			rect.top += lineWidth;
			rect.left += lineWidth;
		}
		::SetTextColor(hdc, GetSysColor(disabled ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
		::SetBkMode(hdc, TRANSPARENT);
		rect.top += topMargin;
		auto oldFont = SelectFont(hdc, font);
		DrawTextT(hdc, text, -1, &rect, textFlags | DT_SINGLELINE | DT_NOPREFIX);
		SelectFont(hdc, oldFont);
	}
}


void DrawButtonRect(HDC hdc, int lineWidth, const CRect &rect, const char *text, bool disabled, bool pushed, DWORD dwFlags, uint32 topMargin)
{
	DrawButtonRectImpl(hdc, lineWidth, CMainFrame::GetGUIFont(), rect, text, disabled, pushed, dwFlags, topMargin);
}


void DrawButtonRect(HDC hdc, int lineWidth, const CRect &rect, const wchar_t *text, bool disabled, bool pushed, DWORD dwFlags, uint32 topMargin)
{
	DrawButtonRectImpl(hdc, lineWidth, CMainFrame::GetGUIFont(), rect, text, disabled, pushed, dwFlags, topMargin);
}


void DrawButtonRect(HDC hdc, int lineWidth, HFONT font, const CRect &rect, const char *text, bool disabled, bool pushed, DWORD dwFlags, uint32 topMargin)
{
	DrawButtonRectImpl(hdc, lineWidth, font, rect, text, disabled, pushed, dwFlags, topMargin);
}


void DrawButtonRect(HDC hdc, int lineWidth, HFONT font, const CRect &rect, const wchar_t *text, bool disabled, bool pushed, DWORD dwFlags, uint32 topMargin)
{
	DrawButtonRectImpl(hdc, lineWidth, font, rect, text, disabled, pushed, dwFlags, topMargin);
}


//////////////////////////////////////////////////////////////////////////////////
// Misc functions


void ErrorBox(UINT nStringID, CWnd *parent)
{
	CString str;
	BOOL resourceLoaded = str.LoadString(nStringID);
	if(!resourceLoaded)
	{
		str.Format(_T("Resource string %u not found."), nStringID);
	}
	MPT_ASSERT(resourceLoaded);
	Reporting::CustomNotification(str, _T("Error!"), MB_OK | MB_ICONERROR, parent);
}


CString GetWindowTextString(const CWnd &wnd)
{
	CString result;
	wnd.GetWindowText(result);
	return result;
}


mpt::ustring GetWindowTextUnicode(const CWnd &wnd)
{
	return mpt::ToUnicode(GetWindowTextString(wnd));
}


CString FormatFileSize(uint64 fileSize)
{
	static constexpr std::array<const TCHAR *, 4> Unit = {_T(" B"), _T(" KB"), _T(" MB"), _T(" GB")};
	double size = static_cast<double>(fileSize);
	for(int i = 0; i < 4; i++)
	{
		if(size < 1024.0 || i == 3)
		{
			// Variable-length formatting may decide on a whim to switch to scientific formatting, so used a fixed width and trim manually...
			CString s = mpt::cfmt::fix(size, 2);
			if(i == 0)
			{
				if(int pos = s.ReverseFind(_T('.')); pos >= 0)
					s.Delete(pos, s.GetLength() - pos);
			}
			return s + Unit[i];
		}
		size /= 1024.0;
	}
	return _T("");
}


CString FormatOrderRow(uint32 value)
{
	const bool rowAndOrderNumbersHex = TrackerSettings::Instance().patternSetup & PatternSetup::RowAndOrderNumbersHex;
	if(rowAndOrderNumbersHex)
		return mpt::cfmt::HEX0<2>(value);
	else
		return mpt::cfmt::val(value);
}


////////////////////////////////////////////////////////////////////////////////
// CFastBitmap 8-bit output / 4-bit input
// useful for lots of small blits with color mapping
// combined in one big blit

void CFastBitmap::Init(MODPLUGDIB *lpTextDib)
{
	m_nBlendOffset = 0;
	m_pTextDib = lpTextDib;
	MemsetZero(m_Dib.bmiHeader);
	m_nTextColor = 0;
	m_nBkColor = 1;
	m_Dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_Dib.bmiHeader.biWidth = 0;	// Set later
	m_Dib.bmiHeader.biHeight = 0;	// Ditto
	m_Dib.bmiHeader.biPlanes = 1;
	m_Dib.bmiHeader.biBitCount = 8;
	m_Dib.bmiHeader.biCompression = BI_RGB;
	m_Dib.bmiHeader.biSizeImage = 0;
	m_Dib.bmiHeader.biXPelsPerMeter = 96;
	m_Dib.bmiHeader.biYPelsPerMeter = 96;
	m_Dib.bmiHeader.biClrUsed = 0;
	m_Dib.bmiHeader.biClrImportant = 256; // MAX_MODPALETTECOLORS;
	m_n4BitPalette[0] = (BYTE)m_nTextColor;
	m_n4BitPalette[4] = MODCOLOR_SEPSHADOW;
	m_n4BitPalette[12] = MODCOLOR_SEPFACE;
	m_n4BitPalette[14] = MODCOLOR_SEPHILITE;
	m_n4BitPalette[15] = (BYTE)m_nBkColor;
}


void CFastBitmap::Blit(HDC hdc, int x, int y, int cx, int cy)
{
	SetDIBitsToDevice(	hdc,
						x,
						y,
						cx,
						cy,
						0,
						m_Dib.bmiHeader.biHeight - cy,
						0,
						m_Dib.bmiHeader.biHeight,
						&m_Dib.DibBits[0],
						(LPBITMAPINFO)&m_Dib,
						DIB_RGB_COLORS);
}


void CFastBitmap::SetColor(UINT nIndex, COLORREF cr)
{
	if (nIndex < 256)
	{
		m_Dib.bmiColors[nIndex].rgbRed = GetRValue(cr);
		m_Dib.bmiColors[nIndex].rgbGreen = GetGValue(cr);
		m_Dib.bmiColors[nIndex].rgbBlue = GetBValue(cr);
	}
}


void CFastBitmap::SetAllColors(UINT nBaseIndex, UINT nColors, COLORREF *pcr)
{
	for (UINT i=0; i<nColors; i++)
	{
		SetColor(nBaseIndex+i, pcr[i]);
	}
}


void CFastBitmap::SetBlendColor(COLORREF cr)
{
	UINT r = GetRValue(cr);
	UINT g = GetGValue(cr);
	UINT b = GetBValue(cr);
	for (UINT i=0; i<BLEND_OFFSET; i++)
	{
		UINT m = (m_Dib.bmiColors[i].rgbRed >> 2)
				+ (m_Dib.bmiColors[i].rgbGreen >> 1)
				+ (m_Dib.bmiColors[i].rgbBlue >> 2);
		m_Dib.bmiColors[i|BLEND_OFFSET].rgbRed = static_cast<BYTE>((m + r)>>1);
		m_Dib.bmiColors[i|BLEND_OFFSET].rgbGreen = static_cast<BYTE>((m + g)>>1);
		m_Dib.bmiColors[i|BLEND_OFFSET].rgbBlue = static_cast<BYTE>((m + b)>>1);
	}
}


// Monochrome 4-bit bitmap (0=text, !0 = back)
void CFastBitmap::TextBlt(int x, int y, int cx, int cy, int srcx, int srcy, MODPLUGDIB *lpdib)
{
	const uint8 *psrc;
	BYTE *pdest;
	UINT x1, x2;
	int srcwidth, srcinc;

	m_n4BitPalette[0] = (BYTE)m_nTextColor;
	m_n4BitPalette[15] = (BYTE)m_nBkColor;
	if (x < 0)
	{
		cx += x;
		x = 0;
	}
	if (y < 0)
	{
		cy += y;
		y = 0;
	}
	if ((x >= m_Dib.bmiHeader.biWidth) || (y >= m_Dib.bmiHeader.biHeight)) return;
	if (x+cx >= m_Dib.bmiHeader.biWidth) cx = m_Dib.bmiHeader.biWidth - x;
	if (y+cy >= m_Dib.bmiHeader.biHeight) cy = m_Dib.bmiHeader.biHeight - y;
	if (!lpdib) lpdib = m_pTextDib;
	if ((cx <= 0) || (cy <= 0) || (!lpdib)) return;
	srcwidth = (lpdib->bmiHeader.biWidth+1) >> 1;
	srcinc = srcwidth;
	if (((int)lpdib->bmiHeader.biHeight) > 0)
	{
		srcy = lpdib->bmiHeader.biHeight - 1 - srcy;
		srcinc = -srcinc;
	}
	x1 = srcx & 1;
	x2 = x1 + cx;
	pdest = &m_Dib.DibBits[((m_Dib.bmiHeader.biHeight - 1 - y) << m_nXShiftFactor) + x];
	psrc = lpdib->lpDibBits + (srcx >> 1) + (srcy * srcwidth);
	for (int iy=0; iy<cy; iy++)
	{
		uint8 *p = pdest;
		UINT ix = x1;
		if (ix&1)
		{
			UINT b = psrc[ix >> 1];
			*p++ = m_n4BitPalette[b & 0x0F]+m_nBlendOffset;
			ix++;
		}
		while (ix+1 < x2)
		{
			UINT b = psrc[ix >> 1];
			p[0] = m_n4BitPalette[b >> 4]+m_nBlendOffset;
			p[1] = m_n4BitPalette[b & 0x0F]+m_nBlendOffset;
			ix+=2;
			p+=2;
		}
		if (x2&1)
		{
			UINT b = psrc[ix >> 1];
			*p++ = m_n4BitPalette[b >> 4]+m_nBlendOffset;
		}
		pdest -= m_Dib.bmiHeader.biWidth;
		psrc += srcinc;
	}
}


void CFastBitmap::SetSize(int x, int y)
{
	if(x > 4)
	{
		// Compute the required shift factor for obtaining a power-of-two bitmap width
		m_nXShiftFactor = 1;
		x--;
		while(x >>= 1)
		{
			m_nXShiftFactor++;
		}
	} else
	{
		// Bitmaps rows are aligned to 4 bytes, so let this bitmap be exactly 4 pixels wide.
		m_nXShiftFactor = 2;
	}

	x = (1 << m_nXShiftFactor);
	if(m_Dib.DibBits.size() != static_cast<size_t>(y << m_nXShiftFactor)) m_Dib.DibBits.resize(y << m_nXShiftFactor);
	m_Dib.bmiHeader.biWidth = x;
	m_Dib.bmiHeader.biHeight = y;
}


///////////////////////////////////////////////////////////////////////////////////
//
// Restore / save plugin list
//

static const auto PLUGFORMAT_FILENAME = MPT_UFORMAT("Plugin{}");
static const auto PLUGFORMAT_TAGS = MPT_UFORMAT("Plugin{}.Tags");
static const auto PLUGFORMAT_TAGS_BUILTIN = MPT_UFORMAT("Plugin{}{}.Tags");
static const auto PLUGFORMAT_LIBNAME = MPT_UFORMAT("Plugin{}.LibraryName");
static const auto PLUGFORMAT_SHELLID = MPT_UFORMAT("Plugin{}.ShellPluginID");

void CTrackApp::InitializeDXPlugins()
{
	m_pPluginManager = new CVstPluginManager;
	const size_t numPlugins = GetSettings().Read<int32>(U_("VST Plugins"), U_("NumPlugins"), 0);

	bool maskCrashes = TrackerSettings::Instance().BrokenPluginsWorkaroundVSTMaskAllCrashes;

	std::vector<VSTPluginLib *> nonFoundPlugs;
	const mpt::PathString failedPlugin = GetSettings().Read<mpt::PathString>(U_("VST Plugins"), U_("FailedPlugin"));
	ConfirmAnswer skipFailed = cnfCancel;

	CDialog pluginScanDlg;
	CWnd *textWnd = nullptr;
	DWORD64 scanStart = Util::GetTickCount64();

	// Read tags for built-in plugins
	for(auto &plug : *m_pPluginManager)
	{
		plug->tags = GetSettings().Read<mpt::ustring>(U_("VST Plugins"), PLUGFORMAT_TAGS_BUILTIN(mpt::ufmt::HEX0<8>(plug->pluginId1), mpt::ufmt::HEX0<8>(plug->pluginId2)));
	}

	// Restructured plugin cache
	if(TrackerSettings::Instance().PreviousSettingsVersion < MPT_V("1.27.00.15"))
	{
		DeleteFile(m_szPluginCacheFileName.AsNative().c_str());
		GetPluginCache().ForgetAll();
	}

	m_pPluginManager->reserve(numPlugins);
	auto scanFormat = MPT_CFORMAT("Scanning Plugin {} / {}...\n{}");
	for(size_t plug = 0; plug < numPlugins; plug++)
	{
		const mpt::PathString plugPath = PathInstallRelativeToAbsolute(GetSettings().Read<mpt::PathString>(U_("VST Plugins"), PLUGFORMAT_FILENAME(plug)));
		if(plugPath.empty())
			continue;

		if(!pluginScanDlg.m_hWnd && Util::GetTickCount64() >= scanStart + 2000)
		{
			// If this is taking too long, show the user what they're waiting for.
			pluginScanDlg.Create(IDD_SCANPLUGINS, gpSplashScreen);
			pluginScanDlg.ShowWindow(SW_SHOW);
			pluginScanDlg.CenterWindow(gpSplashScreen);
			textWnd = pluginScanDlg.GetDlgItem(IDC_SCANTEXT);
		} else if(pluginScanDlg.m_hWnd && Util::GetTickCount64() >= scanStart + 30)
		{
			textWnd->SetWindowText(scanFormat(plug + 1, numPlugins + 1, plugPath));
			MSG msg;
			while(::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			scanStart = Util::GetTickCount64();
		}

		if(plugPath == failedPlugin)
		{
			GetSettings().Remove(U_("VST Plugins"), U_("FailedPlugin"));
			if(skipFailed == cnfCancel)
			{
				const CString text = MPT_CFORMAT("The following plugin has previously crashed OpenMPT during initialisation:\n\n{}\n\nDo you still want to load it?")
					(failedPlugin.ToCString());
				skipFailed = Reporting::Confirm(text, false, true, &pluginScanDlg);
			}

			if(skipFailed == cnfNo)
				continue;
		}

		const uint32 shellPluginID = mpt::parse_hex<uint32>(GetSettings().Read<mpt::ustring>(U_("VST Plugins"), PLUGFORMAT_SHELLID(plug)));
		bool plugFound = true;
		for(VSTPluginLib *lib : m_pPluginManager->AddPlugin(plugPath, maskCrashes, true, &plugFound, shellPluginID))
		{
			if(lib->libraryName == P_("MIDI Input Output") && lib->pluginId1 == PLUGMAGIC('V', 's', 't', 'P') && lib->pluginId2 == PLUGMAGIC('M', 'M', 'I', 'D') && !lib->shellPluginID)
			{
				// This appears to be an old version of our MIDI I/O plugin, which is now built right into the main executable.
				m_pPluginManager->RemovePlugin(lib);
				continue;
			}
			if(!plugFound)
				nonFoundPlugs.push_back(lib);
			if(shellPluginID && lib->shellPluginID != shellPluginID)
				continue;

			lib->tags = GetSettings().Read<mpt::ustring>(U_("VST Plugins"), PLUGFORMAT_TAGS(plug));
			if(shellPluginID != 0)
			{
				if(mpt::PathString libName = GetSettings().Read<mpt::PathString>(U_("VST Plugins"), PLUGFORMAT_LIBNAME(plug)); !libName.empty())
					lib->libraryName = std::move(libName);
			}

		}
	}
	GetPluginCache().Flush();
	if(pluginScanDlg.m_hWnd)
	{
		pluginScanDlg.DestroyWindow();
	}
	if(!nonFoundPlugs.empty())
	{
		PlugNotFoundDialog(nonFoundPlugs, nullptr).DoModal();
	}
}


void CTrackApp::UninitializeDXPlugins()
{
	if(!m_pPluginManager) return;

#ifndef NO_PLUGINS

	size_t plugIndex = 0;
	for(auto &plug : *m_pPluginManager)
	{
		if(!plug->isBuiltIn)
		{
			mpt::PathString plugPath;
			if(theApp.IsPortableMode())
				plugPath = PathAbsoluteToInstallRelative(plug->dllPath);
			else
				plugPath = plug->dllPath;
			
			const auto libName = PLUGFORMAT_LIBNAME(plugIndex), shellID = PLUGFORMAT_SHELLID(plugIndex);
			if(plug->shellPluginID != 0)
			{
				theApp.GetSettings().Write(U_("VST Plugins"), libName, plug->libraryName);
				theApp.GetSettings().Write(U_("VST Plugins"), shellID, mpt::ufmt::HEX0<8>(plug->shellPluginID));
			} else
			{
				theApp.GetSettings().Remove(U_("VST Plugins"), libName);
				theApp.GetSettings().Remove(U_("VST Plugins"), shellID);
			}

			theApp.GetSettings().Write<mpt::PathString>(U_("VST Plugins"), PLUGFORMAT_FILENAME(plugIndex), plugPath);
			theApp.GetSettings().Write(U_("VST Plugins"), PLUGFORMAT_TAGS(plugIndex), plug->tags);

			plugIndex++;
		} else
		{
			theApp.GetSettings().Write(U_("VST Plugins"), PLUGFORMAT_TAGS_BUILTIN(mpt::ufmt::HEX0<8>(plug->pluginId1), mpt::ufmt::HEX0<8>(plug->pluginId2)), plug->tags);
		}
	}
	theApp.GetSettings().Write(U_("VST Plugins"), U_("NumPlugins"), static_cast<uint32>(plugIndex));
#endif // NO_PLUGINS

	delete m_pPluginManager;
	m_pPluginManager = nullptr;
}


///////////////////////////////////////////////////////////////////////////////////
// Internet-related functions

bool CTrackApp::OpenURL(const char *url)
{
	if(!url) return false;
	return OpenURL(mpt::PathString::FromUTF8(url));
}

bool CTrackApp::OpenURL(const std::string &url)
{
	return OpenURL(mpt::PathString::FromUTF8(url));
}

bool CTrackApp::OpenURL(const CString &url)
{
	return OpenURL(mpt::ToUnicode(url));
}

bool CTrackApp::OpenURL(const mpt::ustring &url)
{
	return OpenURL(mpt::PathString::FromUnicode(url));
}

bool CTrackApp::OpenURL(const mpt::PathString &lpszURL, const mpt::tstring &param)
{
	if(!lpszURL.empty() && theApp.m_pMainWnd)
	{
		if(reinterpret_cast<INT_PTR>(ShellExecute(
			theApp.m_pMainWnd->m_hWnd,
			_T("open"),
			lpszURL.AsNative().c_str(),
			param.empty() ? nullptr : param.c_str(),
			nullptr,
			SW_SHOW)) >= 32)
		{
			return true;
		}
	}
	return false;
}

bool CTrackApp::OpenDirectory(const mpt::PathString &directory)
{
	if(mpt::native_fs{}.is_file(directory))
		return OpenURL(P_("explorer.exe"), MPT_TFORMAT("/select,\"{}\"")(directory.AsNative()));
	else
		return OpenURL(directory);
}


CString CTrackApp::GetResamplingModeName(ResamplingMode mode, int length, bool addTaps)
{
	CString result;
	switch(mode)
	{
	case SRCMODE_NEAREST:
		result = (length > 1) ? _T("No Interpolation") : _T("None") ;
		break;
	case SRCMODE_LINEAR:
		result = _T("Linear");
		break;
	case SRCMODE_CUBIC:
		result = _T("Cubic");
		break;
	case SRCMODE_SINC8:
		result = _T("Sinc");
		break;
	case SRCMODE_SINC8LP:
		result = _T("Sinc");
		break;
	default:
		MPT_ASSERT_NOTREACHED();
		break;
	}
	if(Resampling::HasAA(mode))
	{
		result += (length > 1) ? _T(" + Low-Pass") : _T(" + LP");
	}
	if(addTaps)
	{
		result += MPT_CFORMAT(" ({} tap{})")(Resampling::Length(mode), (Resampling::Length(mode) != 1) ? CString(_T("s")) : CString(_T("")));
	}
	return result;
}


mpt::ustring CTrackApp::GetFriendlyMIDIPortName(const mpt::ustring &deviceName, bool isInputPort, bool addDeviceName)
{
	auto friendlyName = GetSettings().Read<mpt::ustring>(isInputPort ? U_("MIDI Input Ports") : U_("MIDI Output Ports"), deviceName, deviceName);
	if(friendlyName.empty())
		return deviceName;
	else if(addDeviceName && friendlyName != deviceName)
		return friendlyName + UL_(" (") + deviceName + UL_(")");
	else
		return friendlyName;
}


CString CTrackApp::GetFriendlyMIDIPortName(const CString &deviceName, bool isInputPort, bool addDeviceName)
{
	return mpt::ToCString(GetFriendlyMIDIPortName(mpt::ToUnicode(deviceName), isInputPort, addDeviceName));
}


bool ValidateMacroString(CEdit &wnd, const std::string_view prevMacro, bool isParametric, bool allowVariables, bool allowMultiline)
{
	CString macroStrT;
	wnd.GetWindowText(macroStrT);
	std::string macroStr = mpt::ToCharset(mpt::Charset::ASCII, macroStrT);

	bool allowed = true, caseChange = false;
	for(char &c : macroStr)
	{
		if(c >= 'G' && c <= 'Z')  // Potentially an allowed variable; lowercase it
		{
			caseChange = true;
			c = c - 'A' + 'a';
		}

		if(c == 'k')  // Previously, 'K' was used for MIDI channel
		{
			caseChange = true;
			c = 'c';
		} else if(c >= 'a' && c <= 'c' && !allowVariables)
		{
			caseChange = true;
			c = c - 'a' + 'A';
		} else if(c >= 'd' && c <= 'f')  // abc can be variables, but def can be fixed
		{
			caseChange = true;
			c = c - 'a' + 'A';
		} else if((c >= 'a' && c <= 'c') || c == 'h' || c == 'm' || c == 'n' || c == 'o' || c == 'p' || c == 's' || c == 'u' || c == 'v' || c == 'x' || c == 'y')
		{
			if(!allowVariables)
			{
				allowed = false;
				break;
			}
		} else if(c == 'z')
		{
			if(!isParametric || !allowVariables)
			{
				allowed = false;
				break;
			}
		} else if(c == '\r' || c == '\n')
		{
			if(!allowMultiline)
			{
				allowed = false;
				break;
			}
		} else if(!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || c == ' '))
		{
			allowed = false;
			break;
		}
	}

	if(!allowed)
	{
		// Replace text and keep cursor position if we just typed in an invalid character
		if(prevMacro != std::string_view{macroStr})
		{
			int start, end;
			wnd.GetSel(start, end);
			wnd.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, static_cast<std::string>(prevMacro)));
			wnd.SetSel(start - 1, end - 1, true);
			MessageBeep(MB_OK);
		}
		return false;
	} else
	{
		if(caseChange)
		{
			// Replace text and keep cursor position if there was a case conversion
			int start, end;
			wnd.GetSel(start, end);
			wnd.SetWindowText(mpt::ToCString(mpt::Charset::ASCII, static_cast<std::string>(macroStr)));
			wnd.SetSel(start, end, true);
		}
		return true;
	}
}


static constexpr std::pair<const mpt::uchar*, const mpt::uchar*> SampleFormats[]
{
	{ UL_("Wave Files (*.wav)"), UL_("*.wav") },
#ifdef MPT_WITH_FLAC
	{ UL_("FLAC Files (*.flac,*.oga)"), UL_("*.flac;*.oga") },
#endif  // MPT_WITH_FLAC
#if defined(MPT_WITH_OPUSFILE)
	{ UL_("Opus Files (*.opus,*.oga)"), UL_("*.opus;*.oga") },
#endif  // MPT_WITH_OPUSFILE
#if defined(MPT_WITH_VORBISFILE) || defined(MPT_WITH_STBVORBIS)
	{ UL_("Ogg Vorbis Files (*.ogg,*.oga)"), UL_("*.ogg;*.oga") },
#endif  // VORBIS
#if defined(MPT_ENABLE_MP3_SAMPLES)
	{ UL_("MPEG Files (*.mp1,*.mp2,*.mp3)"), UL_("*.mp1;*.mp2;*.mp3") },
#endif  // MPT_ENABLE_MP3_SAMPLES
	{ UL_("XI Samples (*.xi)"), UL_("*.xi") },
	{ UL_("Impulse Tracker Samples (*.its)"), UL_("*.its") },
	{ UL_("Scream Tracker Samples (*.s3i,*.smp)"), UL_("*.s3i;*.smp") },
	{ UL_("OPL Instruments (*.sb0,*.sb2,*.sbi)"), UL_("*.sb0;*.sb2;*.sbi") },
	{ UL_("GF1 Patches (*.pat)"), UL_("*.pat") },
	{ UL_("Wave64 Files (*.w64)"), UL_("*.w64") },
	{ UL_("CAF Files (*.wav)"), UL_("*.caf") },
	{ UL_("AIFF Files (*.aiff,*.8svx)"), UL_("*.aif;*.aiff;*.iff;*.8sv;*.8svx;*.svx") },
	{ UL_("Sun Audio (*.au,*.snd)"), UL_("*.au;*.snd") },
	{ UL_("SNES BRR Files (*.brr)"), UL_("*.brr") },
};


mpt::ustring ConstructSampleFormatFileFilter(bool includeRaw)
{
	mpt::ustring s = U_("All Samples (*.wav,*.flac,*.xi,*.its,*.s3i,*.sbi,...)|");
	bool first = true;
	for (const auto& [name, exts] : SampleFormats)
	{
		if (!first)
			s += U_(";");
		else
			first = false;
		s += exts;
	}
#if defined(MPT_WITH_MEDIAFOUNDATION)
	std::vector<FileType> mediaFoundationTypes = CSoundFile::GetMediaFoundationFileTypes();
	s += ToFilterOnlyString(mediaFoundationTypes, true).ToUnicode();
#endif
	if (includeRaw)
	{
		s += U_(";*.raw;*.snd;*.pcm;*.sam");
	}
	s += U_("|");
	for (const auto& [name, exts] : SampleFormats)
	{
		s += name + U_("|");
		s += exts + U_("|");
	}
#if defined(MPT_WITH_MEDIAFOUNDATION)
	s += ToFilterString(mediaFoundationTypes, FileTypeFormatShowExtensions).ToUnicode();
#endif
	if (includeRaw)
	{
		s += U_("Raw Samples (*.raw,*.snd,*.pcm,*.sam)|*.raw;*.snd;*.pcm;*.sam|");
	}
	s += U_("All Files (*.*)|*.*||");
	return s;
}



OPENMPT_NAMESPACE_END
