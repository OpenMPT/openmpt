/*
 * MPTrack.cpp
 * -----------
 * Purpose: OpenMPT core application class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "MainFrm.h"
#include "InputHandler.h"
#include "ChildFrm.h"
#include "moddoc.h"
#include "globals.h"
#include "Dlsbank.h"
#include "commctrl.h"
#include "../common/version.h"
#include "../test/test.h"
#include <afxadv.h>
#include <shlwapi.h>
#include "UpdateCheck.h"
#include "../common/StringFixer.h"
#include "ExceptionHandler.h"
#include "CloseMainDialog.h"
#include "AboutDialog.h"
#include "AutoSaver.h"
#include "FileDialog.h"
#include "PNG.h"
#include "BuildVariants.h"
#include "../common/ComponentManager.h"
#include "WelcomeDialog.h"
#include "../sounddev/SoundDeviceManager.h"
#include "../soundlib/plugins/PluginManager.h"

// rewbs.memLeak
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
//end  rewbs.memLeak

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


OPENMPT_NAMESPACE_BEGIN


/////////////////////////////////////////////////////////////////////////////
// The one and only CTrackApp object

CTrackApp theApp;

const TCHAR *szSpecialNoteNamesMPT[] = {_T("PCs"), _T("PC"), _T("~~ (Note Fade)"), _T("^^ (Note Cut)"), _T("== (Note Off)")};
const TCHAR *szSpecialNoteShortDesc[] = {_T("Param Control (Smooth)"), _T("Param Control"), _T("Note Fade"), _T("Note Cut"), _T("Note Off")};

// Make sure that special note arrays include string for every note.
STATIC_ASSERT(NOTE_MAX_SPECIAL - NOTE_MIN_SPECIAL + 1 == CountOf(szSpecialNoteNamesMPT));
STATIC_ASSERT(CountOf(szSpecialNoteShortDesc) == CountOf(szSpecialNoteNamesMPT));

const char *szHexChar = "0123456789ABCDEF";

CDocument *CModDocTemplate::OpenDocumentFile(const mpt::PathString &filename, BOOL addToMru, BOOL makeVisible)
//------------------------------------------------------------------------------------------------------------
{
	if(filename.IsDirectory())
	{
		CDocument *pDoc = nullptr;
		mpt::PathString path = filename;
		path.EnsureTrailingSlash();
		HANDLE hFind;
		WIN32_FIND_DATAW wfd;
		MemsetZero(wfd);
		if((hFind = FindFirstFileW((path + MPT_PATHSTRING("*.*")).AsNative().c_str(), &wfd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(wcscmp(wfd.cFileName, L"..") && wcscmp(wfd.cFileName, L"."))
				{
					pDoc = OpenDocumentFile(path + mpt::PathString::FromNative(wfd.cFileName), addToMru, makeVisible);
				}
			} while (FindNextFileW(hFind, &wfd));
			FindClose(hFind);
		}
		return pDoc;
	}

	if(!mpt::PathString::CompareNoCase(filename.GetFileExt(), MPT_PATHSTRING(".dll")))
	{
		CVstPluginManager *pPluginManager = theApp.GetPluginManager();
		if(pPluginManager && pPluginManager->AddPlugin(filename) != nullptr)
		{
			return nullptr;
		}
	}

	// First, remove document from MRU list.
	if(addToMru)
	{
		theApp.RemoveMruItem(filename);
	}

	CDocument *pDoc = CMultiDocTemplate::OpenDocumentFile(filename.empty() ? NULL : mpt::PathString::TunnelIntoCString(filename).GetString(), addToMru, makeVisible);
	if(pDoc)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->OnDocumentCreated(static_cast<CModDoc *>(pDoc));
	} else //Case: pDoc == 0, opening document failed.
	{
		if(!filename.empty())
		{
			if(CMainFrame::GetMainFrame() && addToMru)
			{
				CMainFrame::GetMainFrame()->UpdateMRUList();
			}
			if(!filename.IsFile())
			{
				Reporting::Error(L"Unable to open \"" + filename.ToWide() + L"\": file does not exist.");
			}
			else //Case: Valid path but opening fails.
			{
				const int nOdc = theApp.GetOpenDocumentCount();
				Reporting::Notification(mpt::String::Print(L"Opening \"%1\" failed. This can happen if "
					L"no more documents can be opened or if the file type was not "
					L"recognised. If the former is true, it is "
					L"recommended to close some documents as otherwise a crash is likely"
					L"(currently there %2 %3 document%4 open).",
					filename, (nOdc == 1) ? L"is" : L"are", nOdc, (nOdc == 1) ? L"" : L"s"));
			}
		}
	}
	return pDoc;
}


CDocument* CModDocTemplate::OpenTemplateFile(const mpt::PathString &filename, bool isExampleTune)
//-----------------------------------------------------------------------------------------------
{
	CDocument *doc = OpenDocumentFile(filename, isExampleTune ? TRUE : FALSE, TRUE);
	if(doc)
	{
		CModDoc *modDoc = static_cast<CModDoc *>(doc);
		// Clear path so that saving will not take place in templates/examples folder.
		modDoc->ClearFilePath();
		if(!isExampleTune)
		{
			CMultiDocTemplate::SetDefaultTitle(modDoc);
			m_nUntitledCount++;
			// Name has changed...
			CMainFrame::GetMainFrame()->UpdateTree(modDoc, GeneralHint().General());

			// Reset edit history for template files
			CSoundFile &sndFile = modDoc->GetrSoundFile();
			sndFile.GetFileHistory().clear();
			sndFile.m_dwCreatedWithVersion = MptVersion::num;
			sndFile.m_dwLastSavedWithVersion = 0;
			sndFile.m_madeWithTracker.clear();
			sndFile.m_songArtist = TrackerSettings::Instance().defaultArtist;
			sndFile.m_playBehaviour = sndFile.GetDefaultPlaybackBehaviour(sndFile.GetType());
			doc->UpdateAllViews(nullptr, UpdateHint().ModType().AsLPARAM());
		} else
		{
			// Remove extension from title, so that saving the file will not suggest a filename like e.g. "example.it.it".
			const CString title = modDoc->GetTitle();
			const int dotPos = title.ReverseFind('.');
			if(dotPos >= 0)
			{
				modDoc->SetTitle(title.Left(dotPos));
			}
		}
	}
	return doc;
}


#ifdef _DEBUG
#define DDEDEBUG
#endif


//======================================
class CModDocManager: public CDocManager
//======================================
{
public:
	CModDocManager() {}
	virtual BOOL OnDDECommand(LPTSTR lpszCommand);
	MPT_DEPRECATED_PATH virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName)
	{
		return OpenDocumentFile(lpszFileName ? mpt::PathString::TunnelOutofCString(lpszFileName) : mpt::PathString());
	}
	virtual CDocument* OpenDocumentFile(const mpt::PathString &filename)
	{
		return CDocManager::OpenDocumentFile(filename.empty() ? NULL : mpt::PathString::TunnelIntoCString(filename).GetString());
	}
};


BOOL CModDocManager::OnDDECommand(LPTSTR lpszCommand)
//---------------------------------------------------
{
	BOOL bResult, bActivate;
#ifdef DDEDEBUG
	Log("OnDDECommand: %s\n", lpszCommand);
#endif
	// Handle any DDE commands recognized by your application
	// and return TRUE.  See implementation of CWinApp::OnDDEComand
	// for example of parsing the DDE command string.
	bResult = FALSE;
	bActivate = FALSE;
	if ((lpszCommand) && (*lpszCommand) && (theApp.m_pMainWnd))
	{
		CHAR s[_MAX_PATH], *pszCmd, *pszData;
		std::size_t len;

		mpt::String::CopyN(s, lpszCommand);
		len = strlen(s) - 1;
		while ((len > 0) && (strchr("(){}[]\'\" ", s[len]))) s[len--] = 0;
		pszCmd = s;
		while (pszCmd[0] == '[') pszCmd++;
		pszData = pszCmd;
		while ((pszData[0] != '(') && (pszData[0]))
		{
			if (((BYTE)pszData[0]) <= (BYTE)0x20) *pszData = 0;
			pszData++;
		}
		while ((*pszData) && (strchr("(){}[]\'\" ", *pszData)))
		{
			*pszData = 0;
			pszData++;
		}
		// Edit/Open
		if ((!lstrcmpi(pszCmd, "Edit"))
		 || (!lstrcmpi(pszCmd, "Open")))
		{
			if (pszData[0])
			{
				bResult = TRUE;
				bActivate = TRUE;
				OpenDocumentFile(mpt::PathString::FromCString(pszData));
			}
		} else
		// New
		if (!lstrcmpi(pszCmd, "New"))
		{
			OpenDocumentFile(mpt::PathString());
			bResult = TRUE;
			bActivate = TRUE;
		}
	#ifdef DDEDEBUG
		Log("%s(%s)\n", pszCmd, pszData);
	#endif
		if ((bActivate) && (theApp.m_pMainWnd->m_hWnd))
		{
			if (theApp.m_pMainWnd->IsIconic()) theApp.m_pMainWnd->ShowWindow(SW_RESTORE);
			theApp.m_pMainWnd->SetActiveWindow();
		}
	}
	// Return FALSE for any DDE commands you do not handle.
#ifdef DDEDEBUG
	if (!bResult)
	{
		Log("WARNING: failure in CModDocManager::OnDDECommand()\n");
	}
#endif
	return bResult;
}


void CTrackApp::OnFileCloseAll()
//------------------------------
{
	if(!(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_NOCLOSEDIALOG))
	{
		// Show modified documents window
		CloseMainDialog dlg;
		if(dlg.DoModal() != IDOK)
		{
			return;
		}
	}

	std::vector<CModDoc *> documents = theApp.GetOpenDocuments();
	for(auto doc = documents.begin(); doc != documents.end(); doc++)
	{
		(*doc)->SafeFileClose();
	}
}


int CTrackApp::GetOpenDocumentCount() const
//-----------------------------------------
{
	return AfxGetApp()->m_pDocManager->GetOpenDocumentCount();
}


// Retrieve a list of all open modules.
std::vector<CModDoc *> CTrackApp::GetOpenDocuments() const
//--------------------------------------------------------
{
	std::vector<CModDoc *> documents;

	CDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if(pDocTmpl)
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


/////////////////////////////////////////////////////////////////////////////
// MPTRACK Command Line options

//================================================
class CMPTCommandLineInfo: public CCommandLineInfo
//================================================
{
public:
	bool m_bNoDls, m_bNoPlugins, m_bNoAssembly, m_bNoSysCheck, m_bNoWine,
		 m_bPortable;
#ifdef _DEBUG
	bool m_bNoTests;
#endif

public:
	CMPTCommandLineInfo()
	{
		m_bNoDls = m_bNoPlugins = m_bNoAssembly = m_bNoSysCheck = m_bNoWine =
		m_bPortable = false;
#ifdef _DEBUG
		m_bNoTests = false;
#endif
	}
	virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
	{
		if ((lpszParam) && (bFlag))
		{
			if (!lstrcmpi(lpszParam, _T("nologo"))) { m_bShowSplash = FALSE; return; }
			if (!lstrcmpi(lpszParam, _T("nodls"))) { m_bNoDls = true; return; }
			if (!lstrcmpi(lpszParam, _T("noplugs"))) { m_bNoPlugins = true; return; }
			if (!lstrcmpi(lpszParam, _T("portable"))) { m_bPortable = true; return; }
			if (!lstrcmpi(lpszParam, _T("fullMemDump"))) { ExceptionHandler::fullMemDump = true; return; }
			if (!lstrcmpi(lpszParam, _T("noAssembly"))) { m_bNoAssembly = true; return; }
			if (!lstrcmpi(lpszParam, _T("noSysCheck"))) { m_bNoSysCheck = true; return; }
			if (!lstrcmpi(lpszParam, _T("noWine"))) { m_bNoWine = true; return; }
#ifdef _DEBUG
			if (!lstrcmpi(lpszParam, _T("noTests"))) { m_bNoTests = true; return; }
#endif
		}
		CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
	}
};


// Splash Screen

static void StartSplashScreen();
static void StopSplashScreen();
static void TimeoutSplashScreen();


/////////////////////////////////////////////////////////////////////////////
// Midi Library

MIDILIBSTRUCT CTrackApp::midiLibrary;

BOOL CTrackApp::ImportMidiConfig(const mpt::PathString &filename, BOOL bNoWarn)
//-----------------------------------------------------------------------------
{
	if(filename.empty()) return FALSE;

	if (CDLSBank::IsDLSBank(filename))
	{
		ConfirmAnswer result = cnfYes;
		if (!bNoWarn)
		{
			result = Reporting::Confirm("You are about to replace the current MIDI library:\n"
				"Do you want to replace only the missing instruments? (recommended)",
				"Warning", true);
		}
		if (result == cnfCancel) return FALSE;
		const bool bReplaceAll = (result == cnfNo);
		CDLSBank dlsbank;
		if (dlsbank.Open(filename))
		{
			for (UINT iIns=0; iIns<256; iIns++)
			{
				if((bReplaceAll) || midiLibrary.MidiMap[iIns].empty())
				{
					DWORD dwProgram = (iIns < 128) ? iIns : 0xFF;
					DWORD dwKey = (iIns < 128) ? 0xFF : iIns & 0x7F;
					DWORD dwBank = (iIns < 128) ? 0 : F_INSTRUMENT_DRUMS;
					if (dlsbank.FindInstrument((iIns < 128) ? FALSE : TRUE,	dwBank, dwProgram, dwKey))
					{
						midiLibrary.MidiMap[iIns] = filename;
					}
				}
			}
		}
		return TRUE;
	}

	IniFileSettingsContainer file(filename);
	return ImportMidiConfig(file);
}

BOOL CTrackApp::ImportMidiConfig(SettingsContainer &file, bool forgetSettings)
//----------------------------------------------------------------------------
{
	mpt::PathString UltraSndPath;

	UltraSndPath = file.Read<mpt::PathString>("Ultrasound", "PatchDir", mpt::PathString());
	if(forgetSettings) file.Forget("Ultrasound", "PatchDir");
	if(UltraSndPath == MPT_PATHSTRING(".\\")) UltraSndPath = mpt::PathString();
	if(UltraSndPath.empty())
	{
		WCHAR curDir[MAX_PATH];
		GetCurrentDirectoryW(CountOf(curDir), curDir);
		UltraSndPath = mpt::PathString::FromNative(curDir);
	}
	for (uint32 iMidi=0; iMidi<256; iMidi++)
	{
		mpt::PathString filename;
		char section[32];
		sprintf(section, (iMidi < 128) ? "Midi%d" : "Perc%d", iMidi & 0x7f);
		filename = file.Read<mpt::PathString>("Midi Library", section, mpt::PathString());
		// Check for ULTRASND.INI
		if(filename.empty())
		{
			const char *pszSection = (iMidi < 128) ? "Melodic Patches" : "Drum Patches";
			sprintf(section, "%d", iMidi & 0x7f);
			filename = file.Read<mpt::PathString>(pszSection, section, mpt::PathString());
			if(forgetSettings) file.Forget(pszSection, section);
			if(filename.empty())
			{
				pszSection = (iMidi < 128) ? "Melodic Bank 0" : "Drum Bank 0";
				filename = file.Read<mpt::PathString>(pszSection, section, mpt::PathString());
				if(forgetSettings) file.Forget(pszSection, section);
			}
			if(!filename.empty())
			{
				mpt::PathString tmp;
				if(!UltraSndPath.empty())
				{
					tmp = UltraSndPath;
					tmp.EnsureTrailingSlash();
				}
				tmp += filename;
				tmp += MPT_PATHSTRING(".pat");
				filename = tmp;
			}
		}
		if(!filename.empty())
		{
			filename = theApp.RelativePathToAbsolute(filename);
			midiLibrary.MidiMap[iMidi] = filename;
		}
	}
	return FALSE;
}


BOOL CTrackApp::ExportMidiConfig(const mpt::PathString &filename)
//---------------------------------------------------------------
{
	if(filename.empty()) return FALSE;
	IniFileSettingsContainer file(filename);
	return ExportMidiConfig(file);
}

BOOL CTrackApp::ExportMidiConfig(SettingsContainer &file)
//-------------------------------------------------------
{
	for(uint32 iMidi = 0; iMidi < 256; iMidi++) if (!midiLibrary.MidiMap[iMidi].empty())
	{
		mpt::PathString szFileName = midiLibrary.MidiMap[iMidi];

		if(!szFileName.empty())
		{
			if(theApp.IsPortableMode())
				szFileName = theApp.AbsolutePathToRelative(szFileName);

			char s[16];
			if (iMidi < 128)
				sprintf(s, "Midi%d", iMidi);
			else
				sprintf(s, "Perc%d", iMidi & 0x7F);

			file.Write<mpt::PathString>("Midi Library", s, szFileName);
		}
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// DLS Banks support

std::vector<CDLSBank *> CTrackApp::gpDLSBanks;


BOOL CTrackApp::LoadDefaultDLSBanks()
//-----------------------------------
{
	mpt::PathString filename;

	UINT numBanks = theApp.GetSettings().Read<int32>("DLS Banks", "NumBanks", 0);
	for(size_t i = 0; i < numBanks; i++)
	{
		mpt::PathString path = theApp.GetSettings().Read<mpt::PathString>("DLS Banks", mpt::format("Bank%1")(i + 1), mpt::PathString());
		path = theApp.RelativePathToAbsolute(path);
		AddDLSBank(path);
	}

	SaveDefaultDLSBanks(); // This will avoid a crash the next time if we crash while loading the bank

	WCHAR szFileNameW[MAX_PATH];
	szFileNameW[0] = 0;
	GetSystemDirectoryW(szFileNameW, CountOf(szFileNameW));
	filename = mpt::PathString::FromNative(szFileNameW);
	filename += MPT_PATHSTRING("\\GM.DLS");
	if(!AddDLSBank(filename))
	{
		szFileNameW[0] = 0;
		GetWindowsDirectoryW(szFileNameW, CountOf(szFileNameW));
		filename = mpt::PathString::FromNative(szFileNameW);
		filename += MPT_PATHSTRING("\\SYSTEM32\\DRIVERS\\GM.DLS");
		if(!AddDLSBank(filename))
		{
			HKEY key;
			if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\DirectMusic", 0, KEY_READ, &key) == ERROR_SUCCESS)
			{
				WCHAR szFileName[MAX_PATH];
				DWORD dwRegType = REG_SZ;
				DWORD dwSize = sizeof(szFileName);
				szFileName[0] = 0;
				if(RegQueryValueExW(key, L"GMFilePath", NULL, &dwRegType, (LPBYTE)&szFileName, &dwSize) == ERROR_SUCCESS)
				{
					AddDLSBank(mpt::PathString::FromNative(szFileName));
				}
				RegCloseKey(key);
			}
		}
	}
	ImportMidiConfig(filename, TRUE);

	return TRUE;
}


BOOL CTrackApp::SaveDefaultDLSBanks()
//-----------------------------------
{
	DWORD nBanks = 0;
	for(size_t i = 0; i < gpDLSBanks.size(); i++)
	{

		if(!gpDLSBanks[i] || gpDLSBanks[i]->GetFileName().empty())
			continue;

		mpt::PathString path = gpDLSBanks[i]->GetFileName();
		if(theApp.IsPortableMode())
		{
			path = theApp.AbsolutePathToRelative(path);
		}

		char s[16];
		sprintf(s, "Bank%d", nBanks + 1);
		theApp.GetSettings().Write<mpt::PathString>("DLS Banks", s, path);
		nBanks++;

	}
	theApp.GetSettings().Write<int32>("DLS Banks", "NumBanks", nBanks);
	return TRUE;
}


BOOL CTrackApp::RemoveDLSBank(UINT nBank)
//---------------------------------------
{
	if(nBank >= gpDLSBanks.size() || !gpDLSBanks[nBank]) return FALSE;
	delete gpDLSBanks[nBank];
	gpDLSBanks[nBank] = nullptr;
	//gpDLSBanks.erase(gpDLSBanks.begin() + nBank);
	return TRUE;
}


BOOL CTrackApp::AddDLSBank(const mpt::PathString &filename)
//---------------------------------------------------------
{
	if(filename.empty() || !CDLSBank::IsDLSBank(filename)) return FALSE;
	// Check for dupes
	for(size_t i = 0; i < gpDLSBanks.size(); i++)
	{
		if(gpDLSBanks[i] && !mpt::PathString::CompareNoCase(filename, gpDLSBanks[i]->GetFileName())) return TRUE;
	}
	CDLSBank *bank = new CDLSBank;
	if(bank->Open(filename))
	{
		gpDLSBanks.push_back(bank);
		return TRUE;
	} else
	{
		delete bank;
		return FALSE;
	}
}


/////////////////////////////////////////////////////////////////////////////
// CTrackApp

MODTYPE CTrackApp::m_nDefaultDocType = MOD_TYPE_IT;

BEGIN_MESSAGE_MAP(CTrackApp, CWinApp)
	//{{AFX_MSG_MAP(CTrackApp)
	ON_COMMAND(ID_FILE_NEW,		OnFileNew)
	ON_COMMAND(ID_FILE_NEWMOD,	OnFileNewMOD)
	ON_COMMAND(ID_FILE_NEWS3M,	OnFileNewS3M)
	ON_COMMAND(ID_FILE_NEWXM,	OnFileNewXM)
	ON_COMMAND(ID_FILE_NEWIT,	OnFileNewIT)
	ON_COMMAND(ID_NEW_MPT,		OnFileNewMPT)
	ON_COMMAND(ID_FILE_OPEN,	OnFileOpen)
	ON_COMMAND(ID_FILE_CLOSEALL, OnFileCloseAll)
	ON_COMMAND(ID_APP_ABOUT,	OnAppAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrackApp construction

CTrackApp::CTrackApp()
//--------------------
	: m_GuiThreadId(0)
	, m_pSettingsIniFile(nullptr)
	, m_pSongSettings(nullptr)
	, m_pSettings(nullptr)
	, m_pDebugSettings(nullptr)
	, m_pTrackerSettings(nullptr)
	, m_pComponentManagerSettings(nullptr)
	, m_pPluginCache(nullptr)
{
	m_bPortableMode = false;
	m_pModTemplate = NULL;
	m_pPluginManager = NULL;
	m_pSoundDevicesManager = nullptr;
}


void CTrackApp::AddToRecentFileList(LPCTSTR lpszPathName)
//-------------------------------------------------------
{
	AddToRecentFileList(mpt::PathString::TunnelOutofCString(lpszPathName));
}


void CTrackApp::AddToRecentFileList(const mpt::PathString path)
//-------------------------------------------------------------
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
//----------------------------------------------
{
	if(item < TrackerSettings::Instance().mruFiles.size())
	{
		TrackerSettings::Instance().mruFiles.erase(TrackerSettings::Instance().mruFiles.begin() + item);
		CMainFrame::GetMainFrame()->UpdateMRUList();
	}
}


void CTrackApp::RemoveMruItem(const mpt::PathString &path)
//--------------------------------------------------------
{
	for(auto i = TrackerSettings::Instance().mruFiles.begin(); i != TrackerSettings::Instance().mruFiles.end(); i++)
	{
		if(!mpt::PathString::CompareNoCase(*i, path))
		{
			TrackerSettings::Instance().mruFiles.erase(i);
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


//============================
class ComponentManagerSettings
//============================
	: public IComponentManagerSettings
{
private:
	TrackerSettings &conf;
public:
	ComponentManagerSettings(TrackerSettings &conf)
		: conf(conf)
	{
		return;
	}
	virtual bool LoadOnStartup() const
	{
		return conf.ComponentsLoadOnStartup;
	}
	virtual bool KeepLoaded() const
	{
		return conf.ComponentsKeepLoaded;
	}
	virtual bool IsBlocked(const std::string &key) const
	{
		return conf.IsComponentBlocked(key);
	}
};


MPT_REGISTERED_COMPONENT(ComponentUXTheme, "UXTheme")


// Move a config file called sFileName from the App's directory (or one of its sub directories specified by sSubDir) to
// %APPDATA%. If specified, it will be renamed to sNewFileName. Existing files are never overwritten.
// Returns true on success.
bool CTrackApp::MoveConfigFile(mpt::PathString sFileName, mpt::PathString sSubDir, mpt::PathString sNewFileName)
//--------------------------------------------------------------------------------------------------------------
{
	// copy a config file from the exe directory to the new config dirs
	mpt::PathString sOldPath;
	mpt::PathString sNewPath;
	sOldPath = GetAppDirPath();
	sOldPath += sSubDir;
	sOldPath += sFileName;

	sNewPath = m_szConfigDirectory;
	sNewPath += sSubDir;
	if(!sNewFileName.empty())
	{
		sNewPath += sNewFileName;
	} else
	{
		sNewPath += sFileName;
	}

	if(!sNewPath.IsFile() && sOldPath.IsFile())
	{
		return (MoveFileW(sOldPath.AsNative().c_str(), sNewPath.AsNative().c_str()) != 0);
	}
	return false;
}


// Set up paths were configuration data is written to. Set overridePortable to true if application's own directory should always be used.
void CTrackApp::SetupPaths(bool overridePortable)
//-----------------------------------------------
{
	WCHAR dir[MAX_PATH] = { 0 };

	// Determine paths, portable mode, first run. Do not yet update any state.

	mpt::PathString configPathApp = mpt::GetAppPath(); // config path in portable mode
	mpt::PathString configPathGlobal; // config path in default non-portable mode
	{
		// Try to find a nice directory where we should store our settings (default: %APPDATA%)
		if((SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK)
			|| (SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK))
		{
			// Store our app settings in %APPDATA% or "My Documents"
			configPathGlobal = mpt::PathString::FromNative(dir) + MPT_PATHSTRING("\\OpenMPT\\");
		}
	}

	bool portableMode = overridePortable;

	if(configPathGlobal.empty())
	{
		// no usable global directory found
		portableMode = true;
	}

	// Check if the user prefers to use the app's directory
	bool configAppPortable = (GetPrivateProfileIntW(L"Paths", L"UseAppDataDirectory", 1, (configPathApp + MPT_PATHSTRING("mptrack.ini")).AsNative().c_str()) == 0);
	if(configAppPortable)
	{
		portableMode = true;
	}

	// chose config directory
	mpt::PathString configPath = portableMode ? configPathApp : configPathGlobal;

	// Set current directory to My Documents (normal) or OpenMPT directory (portable).
	// If no sample / mod / etc. paths are set up by the user, this will be the default location for browsing files.
	if(portableMode || SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, dir) != S_OK)
	{
		SetCurrentDirectoryW(configPathApp.AsNative().c_str());
	} else
	{
		SetCurrentDirectoryW(dir);
	}

	// Update state.

	// store exe path
	m_szExePath = mpt::GetAppPath();

	// store the config path
	m_szConfigDirectory = configPath;

	// Set up default file locations
	m_szConfigFileName = m_szConfigDirectory + MPT_PATHSTRING("mptrack.ini"); // config file
	m_szPluginCacheFileName = m_szConfigDirectory + MPT_PATHSTRING("plugin.cache"); // plugin cache

	// Force use of custom ini file rather than windowsDir\executableName.ini
	if(m_pszProfileName)
	{
		free((void *)m_pszProfileName);
	}
	m_pszProfileName = _tcsdup(m_szConfigFileName.ToCString());

	m_bPortableMode = portableMode;

	// Create missing diretories
	if(!m_szConfigDirectory.IsDirectory())
	{
		CreateDirectoryW(m_szConfigDirectory.AsNative().c_str(), 0);
	}


	// Handle updates from old versions.

	if(!portableMode)
	{

		// Move the config files if they're still in the old place.
		MoveConfigFile(MPT_PATHSTRING("mptrack.ini"));
		MoveConfigFile(MPT_PATHSTRING("plugin.cache"));
	
		// Import old tunings
		mpt::PathString sOldTunings;
		sOldTunings = GetAppDirPath();
		sOldTunings += MPT_PATHSTRING("tunings\\");

		if(sOldTunings.IsDirectory())
		{
			mpt::PathString sSearchPattern;
			sSearchPattern = sOldTunings;
			sSearchPattern += MPT_PATHSTRING("*.*");
			WIN32_FIND_DATAW FindFileData;
			HANDLE hFind;
			hFind = FindFirstFileW(sSearchPattern.AsNative().c_str(), &FindFileData);
			if(hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					MoveConfigFile(mpt::PathString::FromNative(FindFileData.cFileName), MPT_PATHSTRING("tunings\\"));
				} while(FindNextFileW(hFind, &FindFileData) != 0);
			}
			FindClose(hFind);
			RemoveDirectoryW(sOldTunings.AsNative().c_str());
		}
	}

}


CString CTrackApp::SuggestModernBuildText()
{
	CString updateText;
	const CString url = mpt::ToCString(MptVersion::GetDownloadURL());
	if(BuildVariants::IsKnownSystem())
	{
		if(!BuildVariants::CurrentBuildIsModern())
		{
			if(BuildVariants::SystemCanRunModernBuilds())
			{
				updateText += CString(_T("You are running a '" + mpt::ToCString(BuildVariants::GuessCurrentBuildName()) + "' build of OpenMPT.")) + _T("\r\n");
				updateText += CString(_T("However, OpenMPT detected that your system is capable of running the standard 'Win32' build as well, which provides better support for your system.")) + _T("\r\n");
				updateText += CString(_T("You may want to visit ")) + url + CString(_T(" and upgrade.")) + _T("\r\n");
			}
		}
	}
	return updateText;
}


bool CTrackApp::CheckSystemSupport()
//----------------------------------
{
	const mpt::ustring lf = MPT_USTRING("\n");
	const mpt::ustring url = MptVersion::GetDownloadURL();
	if(!BuildVariants::ProcessorCanRunCurrentBuild())
	{
		mpt::ustring text = mpt::ustring();
		text += MPT_USTRING("Your CPU is too old to run this variant of OpenMPT.") + lf;
		if(BuildVariants::GetRecommendedBuilds().size() > 0)
		{
			text += MPT_USTRING("The following OpenMPT variants are supported on your system:") + lf;
			text += mpt::String::Combine(BuildVariants::GetBuildNames(BuildVariants::GetRecommendedBuilds()), lf);
			text += MPT_USTRING("OpenMPT will exit now.") + lf;
			text += MPT_USTRING("Do you want to visit ") + url + MPT_USTRING(" to download a suitable OpenMPT variant now?") + lf;
			if(Reporting::CustomNotification(text, "OpenMPT", MB_YESNO | MB_ICONERROR, CMainFrame::GetMainFrame()) == IDYES)
			{
				OpenURL(url);
			}
			return false;
		} else if(BuildVariants::IsKnownSystem())
		{
			text += MPT_USTRING("There is no OpenMPT variant available for your system.") + lf;
			text += MPT_USTRING("OpenMPT will exit now.") + lf;
			Reporting::Error(text, "OpenMPT");
			return false;
		} else
		{
			text += MPT_USTRING("OpenMPT will exit now.") + lf;
			Reporting::Error(text, "OpenMPT");
			return false;
		}
	}
	if(BuildVariants::IsKnownSystem())
	{
		if(!BuildVariants::CanRunBuild(BuildVariants::GetCurrentBuildVariant()))
		{
			mpt::ustring text = mpt::ustring();
			text += MPT_USTRING("Your system does not meet the minimum requirements for this variant of OpenMPT.") + lf;
			if(BuildVariants::GetRecommendedBuilds().size() > 0)
			{
				text += MPT_USTRING("The following OpenMPT variants are supported on your system:") + lf;
				text += mpt::String::Combine(BuildVariants::GetBuildNames(BuildVariants::GetRecommendedBuilds()), lf) + lf;
				text += MPT_USTRING("OpenMPT will exit now.") + lf;
				text += MPT_USTRING("Do you want to visit ") + url + MPT_USTRING(" to download aq suitable OpenMPT variant now?") + lf;
				if(Reporting::CustomNotification(text, "OpenMPT", MB_YESNO | MB_ICONERROR, CMainFrame::GetMainFrame()) == IDYES)
				{
					OpenURL(url);
				}
				return true; // may work though
			} else
			{
				text += MPT_USTRING("There is no OpenMPT variant available for your system.") + lf;
				text += MPT_USTRING("OpenMPT will exit now.") + lf;
				Reporting::Error(text, "OpenMPT");
				return true; // may work though
			}
		} else if(!BuildVariants::CurrentBuildIsModern())
		{
			if(BuildVariants::SystemCanRunModernBuilds())
			{
				if(TrackerSettings::Instance().UpdateSuggestDifferentBuildVariant)
				{
					mpt::ustring text = mpt::ustring();
					text += MPT_USTRING("You are running an OpenMPT variant which does not support your system in the most optimal way.") + lf;
					text += MPT_USTRING("The following OpenMPT variants are more suitable for your system:") + lf;
					text += mpt::String::Combine(BuildVariants::GetBuildNames(BuildVariants::GetRecommendedBuilds()), lf) + lf;
					text += MPT_USTRING("Do you want to visit ") + url + MPT_USTRING(" now to upgrade?") + lf;
					if(Reporting::CustomNotification(text, "OpenMPT", MB_YESNO | MB_ICONINFORMATION, CMainFrame::GetMainFrame()) == IDYES)
					{
						OpenURL(url);
					}
				}
			}
		}
	}
	return true;
}


BOOL CTrackApp::InitInstance()
//----------------------------
{

	// We probably should call the base class version here,
	// but we historically did not do that.
	//if(!CWinApp::InitInstance())
	//{
	//	return FALSE;
	//}

	#if MPT_COMPILER_MSVC
		_CrtSetDebugFillThreshold(0); // Disable buffer filling in secure enhanced CRT functions.
	#endif

	// Initialize OLE MFC support
	BOOL oleinit = AfxOleInit();
	ASSERT(oleinit != FALSE); // no MPT_ASSERT here!

	// Standard initialization

	// Start loading
	BeginWaitCursor();

	MPT_LOG(LogInformation, "", MPT_USTRING("OpenMPT Start"));

	// Initialize DocManager (for DDE)
	// requires mpt::PathString
	ASSERT(nullptr == m_pDocManager); // no MPT_ASSERT here!
	m_pDocManager = new CModDocManager();

	// Parse command line for standard shell commands, DDE, file open
	CMPTCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	m_GuiThreadId = GetCurrentThreadId();

	mpt::log::Trace::SetThreadId(mpt::log::Trace::ThreadKindGUI, m_GuiThreadId);

	ExceptionHandler::Register();

	// create the tracker-global random device
	m_RD = mpt::make_unique<mpt::random_device>();
	// make the device available to non-tracker-only code
	mpt::set_global_random_device(m_RD.get());
	// create and seed the traker-global best PRNG with the random device
	m_BestPRNG = mpt::make_unique<mpt::thread_safe_prng<mpt::best_prng> >(mpt::make_prng<mpt::best_prng>(RandomDevice()));
	// make the best PRNG available to non-tracker-only code
	mpt::set_global_prng(m_BestPRNG.get());
	// create and seed the traker-global PRNG with the random device
	m_PRNG = mpt::make_unique<mpt::thread_safe_prng<mpt::prng> >(mpt::make_prng<mpt::prng>(RandomDevice()));
	// additionally, seed the C rand() PRNG, just in case any third party library calls rand()
	mpt::rng::crand::reseed(RandomDevice());

	if(cmdInfo.m_bNoWine)
	{
		mpt::Windows::PreventWineDetection();
	}

	#ifdef ENABLE_ASM
		if(cmdInfo.m_bNoAssembly)
		{
			ProcSupport = 0;
		} else
		{
			InitProcSupport();
		}
	#endif

	if(mpt::Windows::IsWine())
	{
		SetWineVersion(std::make_shared<mpt::Wine::VersionContext>());
	}

	// Set up paths to store configuration in
	SetupPaths(cmdInfo.m_bPortable);

	m_pSettingsIniFile = new IniFileSettingsBackend(m_szConfigFileName);
	m_pSettings = new SettingsContainer(m_pSettingsIniFile);

	m_pDebugSettings = new DebugSettings(*m_pSettings);

	m_pTrackerSettings = new TrackerSettings(*m_pSettings);

	MPT_LOG(LogInformation, "", MPT_USTRING("OpenMPT settings initialized."));

	// Create missing diretories
	if(!TrackerSettings::Instance().PathTunings.GetDefaultDir().IsDirectory())
	{
		CreateDirectoryW(TrackerSettings::Instance().PathTunings.GetDefaultDir().AsNative().c_str(), 0);
	}

	m_pSongSettingsIniFile = new IniFileSettingsBackend(m_szConfigDirectory + MPT_PATHSTRING("SongSettings.ini"));
	m_pSongSettings = new SettingsContainer(m_pSongSettingsIniFile);

	m_pComponentManagerSettings = new ComponentManagerSettings(TrackerSettings::Instance());

	m_pPluginCache = new IniFileSettingsContainer(m_szPluginCacheFileName);

	// Load standard INI file options (without MRU)
	// requires SetupPaths called
	LoadStdProfileSettings(0);

	// Set process priority class
	#ifndef _DEBUG
		SetPriorityClass(GetCurrentProcess(), TrackerSettings::Instance().MiscProcessPriorityClass);
	#endif

	// Dynamic DPI-awareness. Some users might want to disable DPI-awareness because of their DPI-unaware VST plugins.
	bool setDPI = false;
	// For Windows 8.1 and newer
	{
		mpt::Library shcore(mpt::LibraryPath::System(MPT_PATHSTRING("SHCore")));
		if(shcore.IsValid())
		{
			typedef HRESULT (WINAPI * PSETPROCESSDPIAWARENESS)(int);
			PSETPROCESSDPIAWARENESS SetProcessDPIAwareness = nullptr;
			if(shcore.Bind(SetProcessDPIAwareness, "SetProcessDpiAwareness"))
			{
				setDPI = (SetProcessDPIAwareness(TrackerSettings::Instance().highResUI ? 2 : 0) == S_OK);
			}
		}
	}
	// For Vista and newer
	if(!setDPI && TrackerSettings::Instance().highResUI)
	{
		mpt::Library user32(mpt::LibraryPath::System(MPT_PATHSTRING("user32")));
		if(user32.IsValid())
		{
			typedef BOOL (WINAPI * PSETPROCESSDPIAWARE)();
			PSETPROCESSDPIAWARE SetProcessDPIAware = nullptr;
			if(user32.Bind(SetProcessDPIAware, "SetProcessDPIAware"))
			{
				SetProcessDPIAware();
			}
		}
	}

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame();
	if(!pMainFrame->LoadFrame(IDR_MAINFRAME)) return FALSE;
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

	// Register document templates
	m_pModTemplate = new CModDocTemplate(
		IDR_MODULETYPE,
		RUNTIME_CLASS(CModDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CModControlView));
	AddDocTemplate(m_pModTemplate);

	// Load Midi Library
	ImportMidiConfig(theApp.GetSettings(), true);

	// Enable DDE Execute open
	// requires m_pDocManager
	EnableShellOpen();

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Load sound APIs
	// requires TrackerSettings
	SoundDevice::SysInfo sysInfo = SoundDevice::SysInfo::Current();
	SoundDevice::AppInfo appInfo;
	appInfo.SetName(MPT_USTRING("OpenMPT"));
	appInfo.SetHWND(*m_pMainWnd);
	appInfo.BoostedThreadPriorityXP = TrackerSettings::Instance().SoundBoostedThreadPriority;
	appInfo.BoostedThreadMMCSSClassVista = TrackerSettings::Instance().SoundBoostedThreadMMCSSClass;
	m_pSoundDevicesManager = new SoundDevice::Manager(sysInfo, appInfo);
	m_pTrackerSettings->MigrateOldSoundDeviceSettings(*m_pSoundDevicesManager);

	// Load static tunings
	CSoundFile::LoadStaticTunings();
	CSoundFile::SetDefaultNoteNames();

	// Load DLS Banks
	if (!cmdInfo.m_bNoDls) LoadDefaultDLSBanks();

	// Initialize Plugins
	if (!cmdInfo.m_bNoPlugins) InitializeDXPlugins();

	// Initialize CMainFrame
	pMainFrame->Initialize();
	InitCommonControls();
	m_dwLastPluginIdleCall = 0;
	pMainFrame->m_InputHandler->UpdateMainMenu();

	// Dispatch commands specified on the command line
	if(cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
	{
		// When not asked to open any existing file,
		// we do not want to open an empty new one on startup.
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	}
	if(!ProcessShellCommand(cmdInfo))
	{
		EndWaitCursor();
		StopSplashScreen();
		return FALSE;
	}

	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	EndWaitCursor();


	// Perform startup tasks.

	// Check whether we are running the best build for the given system.
	if(!cmdInfo.m_bNoSysCheck)
	{
		if(!CheckSystemSupport())
		{
			StopSplashScreen();
			return FALSE;
		}
	}

	if(TrackerSettings::Instance().FirstRun)
	{
		
		// On high-DPI devices, automatically upscale pattern font
		FontSetting font = TrackerSettings::Instance().patternFont;
		font.size = Clamp(Util::GetDPIy(m_pMainWnd->m_hWnd) / 96 - 1, 0, 9);
		TrackerSettings::Instance().patternFont = font;
		new WelcomeDlg(m_pMainWnd);

	} else
	{

		// Update check
		CUpdateCheck::DoAutoUpdateCheck();

		// Open settings if the previous execution was with an earlier version.
		if(TrackerSettings::Instance().ShowSettingsOnNewVersion && (TrackerSettings::Instance().PreviousSettingsVersion < MptVersion::num))
		{
			m_pMainWnd->PostMessage(WM_COMMAND, ID_VIEW_OPTIONS);
		}

	}

#ifdef _DEBUG
	if(!cmdInfo.m_bNoTests)
		Test::DoTests();
#endif

	if(TrackerSettings::Instance().m_SoundSettingsOpenDeviceAtStartup)
	{
		pMainFrame->InitPreview();
		pMainFrame->PreparePreview(NOTE_NOTECUT);
		pMainFrame->PlayPreview();
	}


	return TRUE;
}


int CTrackApp::ExitInstance()
//---------------------------
{
	delete m_pSoundDevicesManager;
	m_pSoundDevicesManager = nullptr;
	ExportMidiConfig(theApp.GetSettings());
	SaveDefaultDLSBanks();
	for(size_t i = 0; i < gpDLSBanks.size(); i++)
	{
		delete gpDLSBanks[i];
	}
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

	SetWineVersion(MPT_SHARED_PTR_NULL(mpt::Wine::VersionContext));

	m_PRNG.reset();
	mpt::set_global_prng(nullptr);
	m_BestPRNG.reset();
	mpt::set_global_random_device(nullptr);
	m_RD.reset();
	
	return CWinApp::ExitInstance();
}


////////////////////////////////////////////////////////////////////////////////
// App Messages


void CTrackApp::OnFileNew()
//-------------------------
{

	// Build from template
	const mpt::PathString templateFile = TrackerSettings::Instance().defaultTemplateFile;
	if(TrackerSettings::Instance().defaultNewFileAction == nfDefaultTemplate && !templateFile.empty())
	{
		const mpt::PathString dirs[] = { GetConfigPath() + MPT_PATHSTRING("TemplateModules\\"), GetAppDirPath() + MPT_PATHSTRING("TemplateModules\\"), mpt::PathString() };
		for(size_t i = 0; i < CountOf(dirs); i++)
		{
			if((dirs[i] + templateFile).IsFile())
			{
				if(m_pModTemplate->OpenTemplateFile(dirs[i] + templateFile) != nullptr)
				{
					return;
				}
			}
		}
	}


	// Default module type
	MODTYPE newType = TrackerSettings::Instance().defaultModType;

	// Get active document to make the new module of the same type
	CModDoc *pModDoc = CMainFrame::GetMainFrame()->GetActiveDoc();
	if(pModDoc != nullptr && TrackerSettings::Instance().defaultNewFileAction == nfSameAsCurrent)
	{
		newType = pModDoc->GetrSoundFile().GetBestSaveFormat();
	}

	switch(newType)
	{
	case MOD_TYPE_MOD:
		OnFileNewMOD();
		break;
	case MOD_TYPE_S3M:
		OnFileNewS3M();
		break;
	case MOD_TYPE_XM:
		OnFileNewXM();
		break;
	case MOD_TYPE_IT:
		OnFileNewIT();
		break;
	case MOD_TYPE_MPT:
	default:
		OnFileNewMPT();
		break;
	}
}


void CTrackApp::OnFileNewMOD()
//----------------------------
{
	SetDefaultDocType(MOD_TYPE_MOD);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}


void CTrackApp::OnFileNewS3M()
//----------------------------
{
	SetDefaultDocType(MOD_TYPE_S3M);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}


void CTrackApp::OnFileNewXM()
//---------------------------
{
	SetDefaultDocType(MOD_TYPE_XM);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}


void CTrackApp::OnFileNewIT()
//---------------------------
{
	SetDefaultDocType(MOD_TYPE_IT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}

void CTrackApp::OnFileNewMPT()
//---------------------------
{
	SetDefaultDocType(MOD_TYPE_MPT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}


void CTrackApp::OpenModulesDialog(std::vector<mpt::PathString> &files, const mpt::PathString &overridePath)
//---------------------------------------------------------------------------------------------------------
{
	files.clear();

	std::vector<const char *> modExtensions = CSoundFile::GetSupportedExtensions(true);
	std::string exts;
	for(size_t i = 0; i < modExtensions.size(); i++)
	{
		exts += std::string("*.") + modExtensions[i] + std::string(";");
	}

	static int nFilterIndex = 0;
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.ExtensionFilter("All Modules|" + exts + ";mod.*"
		"|"
		"Compressed Modules (*.mdz;*.s3z;*.xmz;*.itz"
#ifdef MPT_ENABLE_MO3
		";*.mo3"
#endif
		")|*.mdz;*.s3z;*.xmz;*.itz;*.mdr;*.zip;*.rar;*.lha;*.pma;*.lzs;*.gz"
#ifdef MPT_ENABLE_MO3
		";*.mo3"
#endif
		"|"
		"ProTracker Modules (*.mod,*.nst)|*.mod;mod.*;*.mdz;*.nst;*.m15;*.stk;*.pt36|"
		"ScreamTracker Modules (*.s3m,*.stm)|*.s3m;*.stm;*.s3z|"
		"FastTracker Modules (*.xm)|*.xm;*.xmz|"
		"Impulse Tracker Modules (*.it)|*.it;*.itz|"
		"OpenMPT Modules (*.mptm)|*.mptm;*.mptmz|"
		"Other Modules (mtm,okt,mdl,669,far,...)|*.mtm;*.669;*.ult;*.wow;*.far;*.mdl;*.okt;*.dmf;*.ptm;*.med;*.ams;*.dbm;*.digi;*.dsm;*.umx;*.amf;*.psm;*.mt2;*.gdm;*.imf;*.itp;*.j2b;*.ice;*.st26;*.plm;*.sfx;*.sfx2;*.mms|"
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
//--------------------------
{
	FileDialog::PathList files;
	OpenModulesDialog(files);
	for(auto file = files.cbegin(); file != files.cend(); file++)
	{
		OpenDocumentFile(*file);
	}
}


// App command to run the dialog
void CTrackApp::OnAppAbout()
//--------------------------
{
	if (CAboutDlg::instance) return;
	CAboutDlg::instance = new CAboutDlg();
	CAboutDlg::instance->Create(IDD_ABOUTBOX, m_pMainWnd);
}


/////////////////////////////////////////////////////////////////////////////
// Splash Screen

//=================================
class CSplashScreen: public CDialog
//=================================
{
protected:
	CBitmap m_Bitmap;
	PNG::Bitmap *bitmap;

public:
	CSplashScreen();
	~CSplashScreen();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel() { OnOK(); }
	virtual void OnPaint();
	virtual BOOL OnEraseBkgnd(CDC *) { return TRUE; }

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CSplashScreen, CDialog)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

static CSplashScreen *gpSplashScreen = NULL;

static DWORD gSplashScreenStartTime = 0;


CSplashScreen::CSplashScreen()
//----------------------------
{
	bitmap = PNG::ReadPNG(MAKEINTRESOURCE(IDB_SPLASHNOFOLDFIN));
}


CSplashScreen::~CSplashScreen()
//-----------------------------
{
	gpSplashScreen = nullptr;
	delete bitmap;
}


void CSplashScreen::OnPaint()
//---------------------------
{
	CPaintDC dc(this);

	CDC hdcMem;
	hdcMem.CreateCompatibleDC(&dc);
	CBitmap *oldBitmap = hdcMem.SelectObject(&m_Bitmap);
	dc.BitBlt(0, 0, bitmap->width, bitmap->height, &hdcMem, 0, 0, SRCCOPY);
	hdcMem.SelectObject(oldBitmap);
	hdcMem.DeleteDC();

	CDialog::OnPaint();
}


BOOL CSplashScreen::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();

	CDC *dc = GetDC();
	bitmap->ToDIB(m_Bitmap, dc);
	ReleaseDC(dc);

	CRect rect;
	GetWindowRect(&rect);
	SetWindowPos(nullptr,
		rect.left - ((static_cast<int32>(bitmap->width) - rect.Width()) / 2),
		rect.top - ((static_cast<int32>(bitmap->height) - rect.Height()) / 2),
		bitmap->width,
		bitmap->height,
		SWP_NOZORDER | SWP_NOCOPYBITS);

	return TRUE;
}


void CSplashScreen::OnOK()
//------------------------
{
	StopSplashScreen();
}


static void StartSplashScreen()
//-----------------------------
{
	if(!gpSplashScreen)
	{
		gpSplashScreen = new CSplashScreen();
		gpSplashScreen->Create(IDD_SPLASHSCREEN, theApp.m_pMainWnd);
		gpSplashScreen->ShowWindow(SW_SHOW);
		gpSplashScreen->UpdateWindow();
		gpSplashScreen->BeginWaitCursor();
		gSplashScreenStartTime = GetTickCount();
	}
}


static void StopSplashScreen()
//----------------------------
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
//-------------------------------
{
	if(gpSplashScreen)
	{
		if(GetTickCount() - gSplashScreenStartTime > 100)
		{
			StopSplashScreen();
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// Idle-time processing

BOOL CTrackApp::OnIdle(LONG lCount)
//---------------------------------
{
	BOOL b = CWinApp::OnIdle(lCount);

	TimeoutSplashScreen();

	if(CMainFrame::GetMainFrame())
	{
		CMainFrame::GetMainFrame()->IdleHandlerSounddevice();
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
//--------------------------
{
	RGBQUAD r;
	r.rgbBlue = GetBValue(c);
	r.rgbGreen = GetGValue(c);
	r.rgbRed = GetRValue(c);
	r.rgbReserved = 0;
	return r;
}


void DibBlt(HDC hdc, int x, int y, int sizex, int sizey, int srcx, int srcy, MODPLUGDIB *lpdib)
//---------------------------------------------------------------------------------------------
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
//-----------------------------------
{
	HINSTANCE hInstance = AfxGetInstanceHandle();
	HRSRC hrsrc = FindResource(hInstance, lpszName, RT_BITMAP);
	HGLOBAL hglb = LoadResource(hInstance, hrsrc);
	LPBITMAPINFO p = (LPBITMAPINFO)LockResource(hglb);
	if (p)
	{
		MODPLUGDIB *pmd = new MODPLUGDIB;
		pmd->bmiHeader = p->bmiHeader;
		for (int i=0; i<16; i++) pmd->bmiColors[i] = p->bmiColors[i];
		LPBYTE lpDibBits = (LPBYTE)p;
		lpDibBits += p->bmiHeader.biSize + 16 * sizeof(RGBQUAD);
		pmd->lpDibBits = lpDibBits;
		return pmd;
	} else return NULL;
}


void DrawButtonRect(HDC hdc, LPRECT lpRect, LPCSTR lpszText, BOOL bDisabled, BOOL bPushed, DWORD dwFlags)
//-------------------------------------------------------------------------------------------------------
{
	CRect rect = *lpRect;

	int width = Util::ScalePixels(1, WindowFromDC(hdc));
	rect.DeflateRect(width, width);
	if(width != 1)
	{
		// Draw "real" buttons in Hi-DPI mode
		DrawFrameControl(hdc, lpRect, DFC_BUTTON, bPushed ? (DFCS_PUSHED | DFCS_BUTTONPUSH) : DFCS_BUTTONPUSH);
	} else
	{
		HGDIOBJ oldpen = SelectPen(hdc, (bPushed) ? CMainFrame::penDarkGray : CMainFrame::penLightGray);
		::MoveToEx(hdc, lpRect->left, lpRect->bottom-1, NULL);
		::LineTo(hdc, lpRect->left, lpRect->top);
		::LineTo(hdc, lpRect->right-1, lpRect->top);
		SelectPen(hdc, (bPushed) ? CMainFrame::penLightGray : CMainFrame::penDarkGray);
		::LineTo(hdc, lpRect->right-1, lpRect->bottom-1);
		::LineTo(hdc, lpRect->left, lpRect->bottom-1);
		::FillRect(hdc, rect, CMainFrame::brushGray);
		SelectPen(hdc, oldpen);
	}
	
	if ((lpszText) && (lpszText[0]))
	{
		if (bPushed)
		{
			rect.top += width;
			rect.left += width;
		}
		::SetTextColor(hdc, GetSysColor((bDisabled) ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
		::SetBkMode(hdc, TRANSPARENT);
		HGDIOBJ oldfont = ::SelectObject(hdc, CMainFrame::GetGUIFont());
		::DrawTextA(hdc, lpszText, -1, &rect, dwFlags | DT_SINGLELINE | DT_NOPREFIX);
		::SelectObject(hdc, oldfont);
	}
}


//////////////////////////////////////////////////////////////////////////////////
// Misc functions


UINT MsgBox(UINT nStringID, CWnd *parent, LPCSTR lpszTitle, UINT n)
//-----------------------------------------------------------------
{
	CString str;
	str.LoadString(nStringID);
	return Reporting::CustomNotification(str, CString(lpszTitle), n, parent);
}


void ErrorBox(UINT nStringID, CWnd *parent)
//-----------------------------------------
{
	MsgBox(nStringID, parent, "Error!", MB_OK | MB_ICONERROR);
}


std::wstring GetWindowTextW(CWnd &wnd)
//------------------------------------
{
	HWND hwnd = wnd.m_hWnd;
	int len = ::GetWindowTextLengthW(hwnd);
	std::wstring str(len, L' ');
	if(len)
	{
		::GetWindowTextW(hwnd, &str[0], len + 1);
	}
	return str;
}


////////////////////////////////////////////////////////////////////////////////
// CFastBitmap 8-bit output / 4-bit input
// useful for lots of small blits with color mapping
// combined in one big blit

void CFastBitmap::Init(MODPLUGDIB *lpTextDib)
//-------------------------------------------
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
//-----------------------------------------------------------
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
//--------------------------------------------------
{
	if (nIndex < 256)
	{
		m_Dib.bmiColors[nIndex].rgbRed = GetRValue(cr);
		m_Dib.bmiColors[nIndex].rgbGreen = GetGValue(cr);
		m_Dib.bmiColors[nIndex].rgbBlue = GetBValue(cr);
	}
}


void CFastBitmap::SetAllColors(UINT nBaseIndex, UINT nColors, COLORREF *pcr)
//--------------------------------------------------------------------------
{
	for (UINT i=0; i<nColors; i++)
	{
		SetColor(nBaseIndex+i, pcr[i]);
	}
}


void CFastBitmap::SetBlendColor(COLORREF cr)
//------------------------------------------
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
//--------------------------------------------------------------------------------------------
{
	const uint8_t *psrc;
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
		uint8_t *p = pdest;
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
//-------------------------------------
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
// DirectX Plugins
//

BOOL CTrackApp::InitializeDXPlugins()
//-----------------------------------
{
	m_pPluginManager = new CVstPluginManager;
	if(!m_pPluginManager) return FALSE;
	const size_t numPlugins = GetSettings().Read<int32>("VST Plugins", "NumPlugins", 0);

	std::wstring nonFoundPlugs;
	const mpt::PathString failedPlugin = GetSettings().Read<mpt::PathString>("VST Plugins", "FailedPlugin", MPT_PATHSTRING(""));

	CDialog pluginScanDlg;
	DWORD scanStart = GetTickCount();
	bool dialogShown = false;

	// Read tags for built-in plugins
	for(auto plug = m_pPluginManager->begin(); plug != m_pPluginManager->end(); plug++)
	{
		char tmp[32];
		sprintf(tmp, "Plugin%08X%08X.Tags", (**plug).pluginId1, (**plug).pluginId2);
		(**plug).tags = GetSettings().Read<mpt::ustring>("VST Plugins", tmp, mpt::ustring());
	}

	// Restructured plugin cache
	if(TrackerSettings::Instance().PreviousSettingsVersion < MAKE_VERSION_NUMERIC(1,27,00,15))
	{
		DeleteFileW(m_szPluginCacheFileName.AsNative().c_str());
		GetPluginCache().ForgetAll();
	}

	m_pPluginManager->reserve(numPlugins);
	for(size_t plug = 0; plug < numPlugins; plug++)
	{
		mpt::PathString plugPath = GetSettings().Read<mpt::PathString>("VST Plugins", mpt::format("Plugin%1")(plug), MPT_PATHSTRING(""));
		if(!plugPath.empty())
		{
			plugPath = RelativePathToAbsolute(plugPath);

			if(plugPath == failedPlugin)
			{
				GetSettings().Remove("VST Plugins", "FailedPlugin");
				const std::wstring text = L"The following plugin has previously crashed OpenMPT during initialisation:\n\n" + failedPlugin.ToWide() + L"\n\nDo you still want to load it?";
				if(Reporting::Confirm(text, false, true) == cnfNo)
				{
					continue;
				}
			}

			mpt::ustring plugTags = GetSettings().Read<mpt::ustring>("VST Plugins", mpt::format("Plugin%1.Tags")(plug), mpt::ustring());

			VSTPluginLib *lib = m_pPluginManager->AddPlugin(plugPath, plugTags, true, true, &nonFoundPlugs);
			if(lib != nullptr && lib->libraryName == MPT_PATHSTRING("MIDI Input Output") && lib->pluginId1 == 'VstP' && lib->pluginId2 == 'MMID')
			{
				// This appears to be an old version of our MIDI I/O plugin, which is not built right into the main executable.
				m_pPluginManager->RemovePlugin(lib);
			}
		}

		if(!dialogShown && GetTickCount() >= scanStart + 2000)
		{
			// If this is taking too long, show the user what they're waiting for.
			dialogShown = true;
			pluginScanDlg.Create(IDD_SCANPLUGINS, gpSplashScreen);
			pluginScanDlg.ShowWindow(SW_SHOW);
			pluginScanDlg.CenterWindow(gpSplashScreen);
		} else if(dialogShown)
		{
			CWnd *text = pluginScanDlg.GetDlgItem(IDC_SCANTEXT);
			std::wstring scanStr = mpt::String::Print(L"Scanning Plugin %1 / %2...\n%3", plug, numPlugins, plugPath);
			SetWindowTextW(text->m_hWnd, scanStr.c_str());
			MSG msg;
			while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
	}
	GetPluginCache().Flush();
	if(!nonFoundPlugs.empty())
	{
		Reporting::Notification(L"Problems were encountered with plugins:\n" + nonFoundPlugs, L"OpenMPT", CWnd::GetDesktopWindow());
	}
	return FALSE;
}


BOOL CTrackApp::UninitializeDXPlugins()
//-------------------------------------
{
	if(!m_pPluginManager) return FALSE;

#ifndef NO_PLUGINS

	size_t plug = 0;
	for(auto pPlug = m_pPluginManager->begin(); pPlug != m_pPluginManager->end(); pPlug++)
	{
		if(!(**pPlug).isBuiltIn)
		{
			mpt::PathString plugPath = (**pPlug).dllPath;
			if(theApp.IsPortableMode())
			{
				plugPath = AbsolutePathToRelative(plugPath);
			}
			theApp.GetSettings().Write<mpt::PathString>("VST Plugins", mpt::format("Plugin%1")(plug), plugPath);

			theApp.GetSettings().Write("VST Plugins", mpt::format("Plugin%1.Tags")(plug), (**pPlug).tags);

			plug++;
		} else
		{
			char tmp[32];
			sprintf(tmp, "Plugin%08X%08X.Tags", (**pPlug).pluginId1, (**pPlug).pluginId2);
			theApp.GetSettings().Write("VST Plugins", tmp, (**pPlug).tags);
		}
	}
	theApp.GetSettings().Write<int32>("VST Plugins", "NumPlugins", plug);
#endif // NO_PLUGINS

	delete m_pPluginManager;
	m_pPluginManager = nullptr;
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////
// Internet-related functions

bool CTrackApp::OpenURL(const char *url)
//--------------------------------------
{
	if(!url) return false;
	return OpenURL(mpt::PathString::FromUTF8(url));
}

bool CTrackApp::OpenURL(const std::string &url)
//---------------------------------------------
{
	return OpenURL(mpt::PathString::FromUTF8(url));
}

bool CTrackApp::OpenURL(const CString &url)
//-----------------------------------------
{
	return OpenURL(mpt::ToUnicode(url));
}

bool CTrackApp::OpenURL(const mpt::ustring &url)
//----------------------------------------------
{
	return OpenURL(mpt::PathString::FromUnicode(url));
}

bool CTrackApp::OpenURL(const mpt::PathString &lpszURL)
//-----------------------------------------------------
{
	if(!lpszURL.empty() && theApp.m_pMainWnd)
	{
		if(reinterpret_cast<INT_PTR>(ShellExecuteW(
			theApp.m_pMainWnd->m_hWnd,
			L"open",
			lpszURL.AsNative().c_str(),
			NULL,
			NULL,
			SW_SHOW)) >= 32)
		{
			return true;
		}
	}
	return false;
}


const TCHAR *CTrackApp::GetResamplingModeName(ResamplingMode mode, bool addTaps)
//------------------------------------------------------------------------------
{
	switch(mode)
	{
	case SRCMODE_NEAREST:
		return addTaps ? _T("No Interpolation (1 tap)") : _T("No Interpolation");
	case SRCMODE_LINEAR:
		return addTaps ? _T("Linear (2 tap)"):_T("Linear");
	case SRCMODE_SPLINE:
		return addTaps ? _T("Cubic Spline (4 tap)"): _T("Cubic Spline");
	case SRCMODE_POLYPHASE:
		return addTaps ? _T("Polyphase (8 tap)"): _T("Polyphase");
	case SRCMODE_FIRFILTER:
		return addTaps ? _T("XMMS-ModPlug (8 tap)"): _T("XMMS-ModPlug");
	default:
		MPT_ASSERT_NOTREACHED();
	}
	return _T("");
}

OPENMPT_NAMESPACE_END
