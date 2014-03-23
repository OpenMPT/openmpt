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
#include "vstplug.h"
#include "CreditStatic.h"
#include "commctrl.h"
#include "../common/version.h"
#include "../test/test.h"
#include <afxadv.h>
#include <shlwapi.h>
#include "UpdateCheck.h"
#include "../common/StringFixer.h"
#include "ExceptionHandler.h"
#include "CloseMainDialog.h"
#include "AutoSaver.h"
#include "FileDialog.h"

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

/////////////////////////////////////////////////////////////////////////////
// The one and only CTrackApp object

CTrackApp theApp;

const char *szSpecialNoteNamesMPT[] = {TEXT("PCs"), TEXT("PC"), TEXT("~~ (Note Fade)"), TEXT("^^ (Note Cut)"), TEXT("== (Note Off)")};
const char *szSpecialNoteShortDesc[] = {TEXT("Param Control (Smooth)"), TEXT("Param Control"), TEXT("Note Fade"), TEXT("Note Cut"), TEXT("Note Off")};

// Make sure that special note arrays include string for every note.
STATIC_ASSERT(NOTE_MAX_SPECIAL - NOTE_MIN_SPECIAL + 1 == CountOf(szSpecialNoteNamesMPT)); 
STATIC_ASSERT(CountOf(szSpecialNoteShortDesc) == CountOf(szSpecialNoteNamesMPT)); 

const char *szHexChar = "0123456789ABCDEF";

CDocument *CModDocTemplate::OpenDocumentFile(const mpt::PathString &filename, BOOL addToMru, BOOL makeVisible)
//------------------------------------------------------------------------------------------------------------
{
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

	#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
		CDocument *pDoc = CMultiDocTemplate::OpenDocumentFile(filename.empty() ? NULL : mpt::PathString::TunnelIntoCString(filename).GetString(), makeVisible);
	#else
		CDocument *pDoc = CMultiDocTemplate::OpenDocumentFile(filename.empty() ? NULL : mpt::PathString::TunnelIntoCString(filename).GetString(), addToMru, makeVisible);
	#endif
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
			if(PathFileExistsW(filename.AsNative().c_str()) == FALSE)
			{
				Reporting::Error(L"Unable to open \"" + filename.ToWide() + L"\": file does not exist.");
			}
			else //Case: Valid path but opening fails.
			{
				const int nOdc = theApp.GetOpenDocumentCount();
				Reporting::Notification(mpt::String::PrintW(L"Opening \"%1\" failed. This can happen if "
					L"no more documents can be opened or if the file type was not "
					L"recognised. If the former is true, it's "
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
			CMainFrame::GetMainFrame()->UpdateTree(modDoc, HINT_MODGENERAL);

			// Reset edit history for template files
			modDoc->GetrSoundFile().GetFileHistory().clear();
			modDoc->GetrSoundFile().m_dwCreatedWithVersion = MptVersion::num;
			modDoc->GetrSoundFile().m_dwLastSavedWithVersion = 0;
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
		int len;

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
	for(std::vector<CModDoc *>::iterator doc = documents.begin(); doc != documents.end(); doc++)
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


mpt::PathString CTrackApp::m_szExePath;

/////////////////////////////////////////////////////////////////////////////
// MPTRACK Command Line options

//================================================
class CMPTCommandLineInfo: public CCommandLineInfo
//================================================
{
public:
	bool m_bNoDls, m_bNoPlugins, m_bNoAssembly,
		 m_bPortable, m_bNoSettingsOnNewVersion;

public:
	CMPTCommandLineInfo()
	{
		m_bNoDls = m_bNoPlugins = m_bNoAssembly =
		m_bPortable = m_bNoSettingsOnNewVersion = false;
	}
	virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
	{
		if ((lpszParam) && (bFlag))
		{
			if (!lstrcmpi(lpszParam, _T("nologo"))) { m_bShowSplash = FALSE; return; }
			if (!lstrcmpi(lpszParam, _T("nodls"))) { m_bNoDls = true; return; }
			if (!lstrcmpi(lpszParam, _T("noplugs"))) { m_bNoPlugins = true; return; }
			if (!lstrcmpi(lpszParam, _T("portable"))) { m_bPortable = true; return; }
			if (!lstrcmpi(lpszParam, _T("noSettingsOnNewVersion"))) { m_bNoSettingsOnNewVersion = true; return; }
			if (!lstrcmpi(lpszParam, _T("fullMemDump"))) { ExceptionHandler::fullMemDump = true; return; }
			if (!lstrcmpi(lpszParam, _T("noAssembly"))) { m_bNoAssembly = true; return; }
		}
		CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
	}
};


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
	for (UINT iMidi=0; iMidi<256; iMidi++)
	{
		mpt::PathString filename;
		char section[32];
		sprintf(section, (iMidi < 128) ? "Midi%d" : "Perc%d", iMidi & 0x7f);
		filename = file.Read<mpt::PathString>("Midi Library", section, mpt::PathString());
		if(forgetSettings) file.Forget("Midi Library", section);
		// Check for ULTRASND.INI
		if(filename.empty())
		{
			const char *pszSection = (iMidi < 128) ? "Melodic Patches" : "Drum Patches";
			sprintf(section, _T("%d"), iMidi & 0x7f);
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
					if(!tmp.HasTrailingSlash())
					{
						tmp += MPT_PATHSTRING("\\");
					}
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
	for(size_t iMidi = 0; iMidi < 256; iMidi++) if (!midiLibrary.MidiMap[iMidi].empty())
	{
		mpt::PathString szFileName = midiLibrary.MidiMap[iMidi];

		if(!szFileName.empty())
		{
			if(theApp.IsPortableMode())
				szFileName = theApp.AbsolutePathToRelative(szFileName);

			char s[16];
			if (iMidi < 128)
				sprintf(s, _T("Midi%d"), iMidi);
			else
				sprintf(s, _T("Perc%d"), iMidi & 0x7F);

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
		char s[16];
		sprintf(s, _T("Bank%d"), i + 1);
		mpt::PathString path = theApp.GetSettings().Read<mpt::PathString>("DLS Banks", s, mpt::PathString());
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
		sprintf(s, _T("Bank%d"), nBanks + 1);
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

// -> CODE#0023
// -> DESC="IT project files (.itp)"
BOOL CTrackApp::m_nProject = FALSE;
// -! NEW_FEATURE#0023

BEGIN_MESSAGE_MAP(CTrackApp, CWinApp)
	//{{AFX_MSG_MAP(CTrackApp)
	ON_COMMAND(ID_FILE_NEW,		OnFileNew)
	ON_COMMAND(ID_FILE_NEWMOD,	OnFileNewMOD)
	ON_COMMAND(ID_FILE_NEWS3M,	OnFileNewS3M)
	ON_COMMAND(ID_FILE_NEWXM,	OnFileNewXM)
	ON_COMMAND(ID_FILE_NEWIT,	OnFileNewIT)
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	ON_COMMAND(ID_NEW_ITPROJECT,OnFileNewITProject)
// -! NEW_FEATURE#0023
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
	, m_pTrackerDirectories(nullptr)
	, m_pUXThemeDLL(nullptr)
	, m_pSettingsIniFile(nullptr)
	, m_pSettings(nullptr)
	, m_pTrackerSettings(nullptr)
	, m_pPluginCache(nullptr)
{
	#if MPT_COMPILER_MSVC
		_CrtSetDebugFillThreshold(0); // Disable buffer filling in secure enhanced CRT functions.
	#endif

	m_GuiThreadId = GetCurrentThreadId();

	ExceptionHandler::Register();

	m_bPortableMode = false;
	m_pModTemplate = NULL;
	m_pPluginManager = NULL;
	m_pSoundDevicesManager = nullptr;
	m_bInitialized = FALSE;
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
	for(std::vector<mpt::PathString>::iterator i = TrackerSettings::Instance().mruFiles.begin(); i != TrackerSettings::Instance().mruFiles.end(); i++)
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

#ifdef WIN32	// Legacy stuff
// Move a config file called sFileName from the App's directory (or one of its sub directories specified by sSubDir) to
// %APPDATA%. If specified, it will be renamed to sNewFileName. Existing files are never overwritten.
// Returns true on success.
bool CTrackApp::MoveConfigFile(mpt::PathString sFileName, mpt::PathString sSubDir, mpt::PathString sNewFileName)
//--------------------------------------------------------------------------------------------------------------
{
	// copy a config file from the exe directory to the new config dirs
	mpt::PathString sOldPath;
	mpt::PathString sNewPath;
	sOldPath = m_szExePath;
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

	if(PathFileExistsW(sNewPath.AsNative().c_str()) == 0 && PathFileExistsW(sOldPath.AsNative().c_str()) != 0)
	{
		return (MoveFileW(sOldPath.AsNative().c_str(), sNewPath.AsNative().c_str()) != 0);
	}
	return false;
}
#endif	// WIN32 Legacy Stuff


// Set up paths were configuration data is written to. Set overridePortable to true if application's own directory should always be used.
void CTrackApp::SetupPaths(bool overridePortable)
//-----------------------------------------------
{
	WCHAR tempExePath[MAX_PATH];
	if(GetModuleFileNameW(NULL, tempExePath, CountOf(tempExePath)))
	{
		mpt::String::SetNullTerminator(tempExePath);
		m_szExePath = mpt::PathString::FromNative(tempExePath).GetDrive();
		m_szExePath += mpt::PathString::FromNative(tempExePath).GetDir();

		WCHAR wcsDir[MAX_PATH];
		GetFullPathNameW(m_szExePath.AsNative().c_str(), CountOf(wcsDir), wcsDir, NULL);
		m_szExePath = mpt::PathString::FromNative(wcsDir);
		SetCurrentDirectoryW(wcsDir);
	}

	m_szConfigDirectory = mpt::PathString();
	// Try to find a nice directory where we should store our settings (default: %APPDATA%)
	bool bIsAppDir = overridePortable;
	WCHAR tempConfigDirectory[MAX_PATH];
	tempConfigDirectory[0] = 0;
	if(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, tempConfigDirectory) != S_OK)
	{
		if(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, tempConfigDirectory) != S_OK)
		{
			bIsAppDir = true;
		}
	}
	m_szConfigDirectory = mpt::PathString::FromNative(tempConfigDirectory);

	// Check if the user prefers to use the app's directory
	m_szConfigFileName = m_szExePath; // config file
	m_szConfigFileName += MPT_PATHSTRING("mptrack.ini");
	if(GetPrivateProfileIntW(L"Paths", L"UseAppDataDirectory", 1, m_szConfigFileName.AsNative().c_str()) == 0)
	{
		bIsAppDir = true;
	}

	if(!bIsAppDir)
	{
		// Store our app settings in %APPDATA% or "My Files"
		m_szConfigDirectory += MPT_PATHSTRING("\\OpenMPT\\");

		// Path doesn't exist yet, so it has to be created
		if(PathIsDirectoryW(m_szConfigDirectory.AsNative().c_str()) == 0)
		{
			CreateDirectoryW(m_szConfigDirectory.AsNative().c_str(), 0);
		}

		#ifdef WIN32	// Legacy stuff
		// Move the config files if they're still in the old place.
		MoveConfigFile(MPT_PATHSTRING("mptrack.ini"));
		MoveConfigFile(MPT_PATHSTRING("plugin.cache"));
		#endif	// WIN32 Legacy Stuff
	} else
	{
		m_szConfigDirectory = m_szExePath;
	}

	// Create tunings dir
	mpt::PathString sTuningPath = m_szConfigDirectory + MPT_PATHSTRING("tunings\\");
	TrackerDirectories::Instance().SetDefaultDirectory(sTuningPath, DIR_TUNING);

	if(PathIsDirectoryW(TrackerDirectories::Instance().GetDefaultDirectory(DIR_TUNING).AsNative().c_str()) == 0)
	{
		CreateDirectoryW(TrackerDirectories::Instance().GetDefaultDirectory(DIR_TUNING).AsNative().c_str(), 0);
	}

	if(!bIsAppDir)
	{
		// Import old tunings
		mpt::PathString sOldTunings;
		sOldTunings = m_szExePath;
		sOldTunings += MPT_PATHSTRING("tunings\\");

		if(PathIsDirectoryW(sOldTunings.AsNative().c_str()) != 0)
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

	// Set up default file locations
	m_szConfigFileName = m_szConfigDirectory; // config file
	m_szConfigFileName += MPT_PATHSTRING("mptrack.ini");

	m_szPluginCacheFileName = m_szConfigDirectory + MPT_PATHSTRING("plugin.cache"); // plugin cache

	mpt::PathString szTemplatePath;
	szTemplatePath = m_szConfigDirectory;
	szTemplatePath += MPT_PATHSTRING("TemplateModules\\");
	TrackerDirectories::Instance().SetDefaultDirectory(szTemplatePath, DIR_TEMPLATE_FILES_USER);

	//Force use of custom ini file rather than windowsDir\executableName.ini
	if(m_pszProfileName)
	{
		free((void *)m_pszProfileName);
	}
	m_pszProfileName = _tcsdup(m_szConfigFileName.ToCString());

	m_bPortableMode = bIsAppDir;
}

BOOL CTrackApp::InitInstance()
//----------------------------
{

	m_GuiThreadId = GetCurrentThreadId();

	// Initialize OLE MFC support
	AfxOleInit();
	// Standard initialization

	// Start loading
	BeginWaitCursor();

	Log("OpenMPT Start");

	ASSERT(nullptr == m_pDocManager);
	m_pDocManager = new CModDocManager();

	ASSERT((sizeof(ModChannel) & 7) == 0);

	// Parse command line for standard shell commands, DDE, file open
	CMPTCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

#ifdef ENABLE_ASM
	if(cmdInfo.m_bNoAssembly)
		ProcSupport = 0;
	else
		InitProcSupport();
#endif

	m_pTrackerDirectories = new TrackerDirectories();

	// Set up paths to store configuration in
	SetupPaths(cmdInfo.m_bPortable);

	// Load UXTheme.DLL
	m_pUXThemeDLL = new mpt::Library(mpt::LibraryPath::System(MPT_PATHSTRING("uxtheme")));
	m_pUXThemeDLL->Bind(m_pEnableThemeDialogTexture, "EnableThemeDialogTexture");

	// Construct auto saver instance, class TrackerSettings expects it being available.
	CMainFrame::m_pAutoSaver = new CAutoSaver();

	m_pSettingsIniFile = new IniFileSettingsBackend(m_szConfigFileName);
	
	m_pSettings = new SettingsContainer(m_pSettingsIniFile);

	m_pTrackerSettings = new TrackerSettings(*m_pSettings);

	m_pPluginCache = new IniFileSettingsContainer(m_szPluginCacheFileName);

	LoadStdProfileSettings(0);  // Load standard INI file options (without MRU)

	// Register document templates
	m_pModTemplate = new CModDocTemplate(
		IDR_MODULETYPE,
		RUNTIME_CLASS(CModDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CModControlView));
	AddDocTemplate(m_pModTemplate);

	// Load Midi Library
	ImportMidiConfig(theApp.GetSettings(), true);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame();
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME)) return FALSE;
	m_pMainWnd = pMainFrame;

	if (cmdInfo.m_bShowSplash && TrackerSettings::Instance().m_ShowSplashScreen)
	{
		StartSplashScreen();
	}
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();

	// Register MOD extensions
	//RegisterExtensions();

	// Load sound APIs
	m_pSoundDevicesManager = new SoundDevicesManager();
	if(TrackerSettings::Instance().m_SoundDeviceSettingsUseOldDefaults)
	{
		// get the old default device
		TrackerSettings::Instance().m_SoundDeviceIdentifier = m_pSoundDevicesManager->FindDeviceInfo(TrackerSettings::Instance().m_SoundDeviceID_DEPRECATED).GetIdentifier();
		// apply old global sound device settings to each found device
		for(std::vector<SoundDeviceInfo>::const_iterator it = m_pSoundDevicesManager->begin(); it != m_pSoundDevicesManager->end(); ++it)
		{
			TrackerSettings::Instance().SetSoundDeviceSettings(it->id, TrackerSettings::Instance().GetSoundDeviceSettingsDefaults());
		}
	}

	// Load DLS Banks
	if (!cmdInfo.m_bNoDls) LoadDefaultDLSBanks();

	// Initialize Plugins
	if (!cmdInfo.m_bNoPlugins) InitializeDXPlugins();

	// Initialize CMainFrame
	pMainFrame->Initialize();
	InitCommonControls();
	m_dwLastPluginIdleCall = 0;	//rewbs.VSTCompliance
	pMainFrame->m_InputHandler->UpdateMainMenu();	//rewbs.customKeys

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
	{
		EndWaitCursor();
		StopSplashScreen();
		return FALSE;
	}

	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	m_dwTimeStarted = timeGetTime();
	m_bInitialized = TRUE;

	if(CUpdateCheck::GetUpdateCheckPeriod() != 0)
	{
		CUpdateCheck::DoUpdateCheck(true);
	}

	// Open settings if the previous execution was with an earlier version.
	if (!cmdInfo.m_bNoSettingsOnNewVersion && TrackerSettings::Instance().gcsPreviousVersion < MptVersion::num)
	{
		StopSplashScreen();
		m_pMainWnd->PostMessage(WM_COMMAND, ID_VIEW_OPTIONS);
	}

	EndWaitCursor();

#ifdef ENABLE_TESTS
	MptTest::DoTests();
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

	delete m_pPluginCache;
	m_pPluginCache = nullptr;
	delete m_pTrackerSettings;
	m_pTrackerSettings = nullptr;
	delete m_pSettings;
	m_pSettings = nullptr;
	delete m_pSettingsIniFile;
	m_pSettingsIniFile = nullptr;
	m_pEnableThemeDialogTexture = nullptr;
	delete m_pUXThemeDLL;
	m_pUXThemeDLL = nullptr;
	delete m_pTrackerDirectories;
	m_pTrackerDirectories = nullptr;

	return CWinApp::ExitInstance();
}


////////////////////////////////////////////////////////////////////////////////
// App Messages


void CTrackApp::OnFileNew()
//-------------------------
{
	if (!m_bInitialized) return;

	// Build from template
	const mpt::PathString templateFile = TrackerSettings::Instance().defaultTemplateFile;
	if(!templateFile.empty())
	{
		const mpt::PathString dirs[] = { GetConfigPath(), GetAppDirPath(), mpt::PathString() };
		for(size_t i = 0; i < CountOf(dirs); i++)
		{
			if(Util::sdOs::IsPathFileAvailable(dirs[i] + templateFile, Util::sdOs::FileModeExists))
			{
				if(m_pModTemplate->OpenTemplateFile(dirs[i] + templateFile) != nullptr)
				{
					return;
				}
			}
		}
	}


	// Default module type
	MODTYPE nNewType = TrackerSettings::Instance().defaultModType;
	bool bIsProject = false;

	// Get active document to make the new module of the same type
	CModDoc *pModDoc = CMainFrame::GetMainFrame()->GetActiveDoc();
	if(pModDoc != nullptr)
	{
		nNewType = pModDoc->GetrSoundFile().GetBestSaveFormat();
		bIsProject = pModDoc->GetrSoundFile().m_SongFlags[SONG_ITPROJECT];
	}

	switch(nNewType)
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
		if(bIsProject)
			OnFileNewITProject();
		else
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
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_MOD);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}


void CTrackApp::OnFileNewS3M()
//----------------------------
{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_S3M);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}


void CTrackApp::OnFileNewXM()
//---------------------------
{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_XM);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}


void CTrackApp::OnFileNewIT()
//---------------------------
{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_IT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}

void CTrackApp::OnFileNewMPT()
//---------------------------
{
	SetAsProject(FALSE);
	SetDefaultDocType(MOD_TYPE_MPT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}



// -> CODE#0023
// -> DESC="IT project files (.itp)"
void CTrackApp::OnFileNewITProject()
//----------------------------------
{
	SetAsProject(TRUE);
	SetDefaultDocType(MOD_TYPE_IT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(mpt::PathString());
}
// -! NEW_FEATURE#0023


void CTrackApp::OpenModulesDialog(std::vector<mpt::PathString> &files)
//--------------------------------------------------------------------
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
		.ExtensionFilter("All Modules|" + exts +
		"|"
		"Compressed Modules (*.mdz;*.s3z;*.xmz;*.itz"
#ifndef NO_MO3
		";*.mo3"
#endif
		")|*.mdz;*.s3z;*.xmz;*.itz;*.mdr;*.zip;*.rar;*.lha;*.pma;*.lzs;*.gz"
#ifndef NO_MO3
		";*.mo3"
#endif
		"|"
		"ProTracker Modules (*.mod,*.nst)|*.mod;mod.*;*.mdz;*.nst;*.m15|"
		"ScreamTracker Modules (*.s3m,*.stm)|*.s3m;*.stm;*.s3z|"
		"FastTracker Modules (*.xm)|*.xm;*.xmz|"
		"Impulse Tracker Modules (*.it)|*.it;*.itz|"
		// -> CODE#0023
		// -> DESC="IT project files (.itp)"
		"Impulse Tracker Projects (*.itp)|*.itp;*.itpz|"
		// -! NEW_FEATURE#0023
		"OpenMPT Modules (*.mptm)|*.mptm;*.mptmz|"
		"Other Modules (mtm,okt,mdl,669,far,...)|*.mtm;*.669;*.ult;*.wow;*.far;*.mdl;*.okt;*.dmf;*.ptm;*.med;*.ams;*.dbm;*.digi;*.dsm;*.umx;*.amf;*.psm;*.mt2;*.gdm;*.imf;*.j2b|"
		"Wave Files (*.wav)|*.wav|"
		"Midi Files (*.mid,*.rmi)|*.mid;*.rmi;*.smf|"
		"All Files (*.*)|*.*||")
		.WorkingDirectory(TrackerDirectories::Instance().GetWorkingDirectory(DIR_MODS))
		.FilterIndex(&nFilterIndex);
	if(!dlg.Show()) return;

	TrackerDirectories::Instance().SetWorkingDirectory(dlg.GetWorkingDirectory(), DIR_MODS);

	files = dlg.GetFilenames();
}

void CTrackApp::OnFileOpen()
//--------------------------
{
	FileDialog::PathList files;
	OpenModulesDialog(files);
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		OpenDocumentFile(files[counter]);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

//===============================
class CPaletteBitmap: public CWnd
//===============================
{
protected:
	LPBITMAPINFO m_lpBmp, m_lpCopy;
	HPALETTE m_hPal;
	LPBYTE m_lpRotoZoom;
	DWORD m_dwStartTime, m_dwFrameTime;
	UINT m_nRotoWidth, m_nRotoHeight;
	BOOL m_bFirst;

public:
	CPaletteBitmap() { m_hPal = NULL; m_lpBmp = NULL; m_lpRotoZoom = NULL; m_lpCopy = NULL; m_bFirst = TRUE; }
	~CPaletteBitmap();
	void LoadBitmap(LPCSTR lpszResource);
	BOOL Animate();
	UINT GetWidth() const { return (m_lpBmp) ? m_lpBmp->bmiHeader.biWidth : 0; }
	UINT GetHeight() const { return (m_lpBmp) ? m_lpBmp->bmiHeader.biHeight : 0; }

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnLButtonDblClk(UINT, CPoint) { m_dwStartTime = timeGetTime(); }
	DECLARE_MESSAGE_MAP()
};

static CPaletteBitmap *gpRotoZoom = NULL;

BEGIN_MESSAGE_MAP(CPaletteBitmap, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


void CPaletteBitmap::LoadBitmap(LPCSTR lpszResource)
//--------------------------------------------------
{
	m_hPal = NULL;
	m_lpBmp = NULL;
	// True Color Mode ?
	HDC hdc = ::GetDC(AfxGetApp()->m_pMainWnd->m_hWnd);
	int nbits = GetDeviceCaps(hdc, BITSPIXEL);
	::ReleaseDC(AfxGetApp()->m_pMainWnd->m_hWnd, hdc);
	// Creating palette
	HINSTANCE hInstance = AfxGetInstanceHandle();
	HRSRC hrsrc = FindResource(hInstance, lpszResource, RT_BITMAP);
	HGLOBAL hglb = LoadResource(hInstance, hrsrc);
	LPBITMAPINFO p = (LPBITMAPINFO)LockResource(hglb);
	m_lpBmp = p;
	m_nRotoWidth = m_nRotoHeight = 0;
	if (p)
	{
		char pal_buf[sizeof(LOGPALETTE) + 512*sizeof(PALETTEENTRY)];
		LPLOGPALETTE pal = (LPLOGPALETTE)&pal_buf[0];
		for (int i=0; i<256; i++)
		{
			pal->palPalEntry[i].peRed = p->bmiColors[i].rgbRed;
			pal->palPalEntry[i].peGreen = p->bmiColors[i].rgbGreen;
			pal->palPalEntry[i].peBlue = p->bmiColors[i].rgbBlue;
			pal->palPalEntry[i].peFlags = 0;
		}
		pal->palVersion = 0x300;
		pal->palNumEntries = 256;
		if (nbits <= 8) m_hPal = CreatePalette(pal);
		if ((p->bmiHeader.biWidth == 256) && (p->bmiHeader.biHeight == 128))
		{
			UINT n;
			CRect rect;
			GetClientRect(&rect);
			m_nRotoWidth = (rect.Width() + 3) & ~3;
			m_nRotoHeight = rect.Height();
			if ((n = m_nRotoWidth - rect.Width()) > 0)
			{
				GetWindowRect(&rect);
				SetWindowPos(NULL, 0,0, rect.Width()+n, rect.Height(), SWP_NOMOVE | SWP_NOZORDER);
			}
			m_lpRotoZoom = new BYTE[m_nRotoWidth*m_nRotoHeight];
			if (m_lpRotoZoom)
			{
				memset(m_lpRotoZoom, 0, m_nRotoWidth*m_nRotoHeight);
				m_lpCopy = (LPBITMAPINFO)(new char [sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD)]);
				if (m_lpCopy)
				{
					memcpy(m_lpCopy, p, sizeof(BITMAPINFO) + 256*sizeof(RGBQUAD));
					m_lpCopy->bmiHeader.biWidth = m_nRotoWidth;
					m_lpCopy->bmiHeader.biHeight = m_nRotoHeight;
					m_lpCopy->bmiHeader.biSizeImage = m_nRotoWidth * m_nRotoHeight;
				}
				gpRotoZoom = this;
			}

		}
	}
	m_dwStartTime = timeGetTime();
	m_dwFrameTime = 0;
}


CPaletteBitmap::~CPaletteBitmap()
//-------------------------------
{
	if (gpRotoZoom == this) gpRotoZoom = NULL;
	if (m_hPal)
	{
		DeleteObject(m_hPal);
		m_hPal = NULL;
	}
	if (m_lpRotoZoom)
	{
		delete[] m_lpRotoZoom;
		m_lpRotoZoom = NULL;
	}
	if (m_lpCopy)
	{
		delete[] m_lpCopy;
		m_lpCopy = NULL;
	}
}


void CPaletteBitmap::OnPaint()
//----------------------------
{
	CPaintDC dc(this);
	HDC hdc = dc.m_hDC;
	HPALETTE oldpal = NULL;
	LPBITMAPINFO lpdib;
	if (m_hPal)
	{
		oldpal = SelectPalette(hdc, m_hPal, FALSE);
		RealizePalette(hdc);
	}
	if ((lpdib = m_lpBmp) != NULL)
	{
		if ((m_lpRotoZoom) && (m_lpCopy))
		{
			lpdib = m_lpCopy;
			SetDIBitsToDevice(hdc,
						0,
						0,
						m_nRotoWidth,
						m_nRotoHeight,
						0,
						0,
						0,
						lpdib->bmiHeader.biHeight,
						m_lpRotoZoom,
						lpdib,
						DIB_RGB_COLORS);
		} else
		{
			CRect rect;
			GetClientRect(&rect);
			StretchDIBits(hdc,
						0,
						0,
						rect.right,
						rect.bottom,
						0,
						0,
						lpdib->bmiHeader.biWidth,
						lpdib->bmiHeader.biHeight,
						&lpdib->bmiColors[256],
						lpdib,
						DIB_RGB_COLORS,
						SRCCOPY);
		}
	}
	if (oldpal) SelectPalette(hdc, oldpal, FALSE);
}

////////////////////////////////////////////////////////////////////
// RotoZoomer

const int __SinusTable[256] =
{
	   0,   6,  12,  18,  25,  31,  37,  43,  49,  56,  62,  68,  74,  80,  86,  92,
	  97, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176,
	 181, 185, 189, 193, 197, 201, 205, 209, 212, 216, 219, 222, 225, 228, 231, 234,
	 236, 238, 241, 243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 255,
	 256, 255, 255, 255, 254, 254, 253, 252, 251, 249, 248, 246, 244, 243, 241, 238,
	 236, 234, 231, 228, 225, 222, 219, 216, 212, 209, 205, 201, 197, 193, 189, 185,
	 181, 176, 171, 167, 162, 157, 152, 147, 142, 136, 131, 126, 120, 115, 109, 103,
	  97,  92,  86,  80,  74,  68,  62,  56,  49,  43,  37,  31,  25,  18,  12,   6,
	   0,  -6, -12, -18, -25, -31, -37, -43, -49, -56, -62, -68, -74, -80, -86, -92,
	 -97,-103,-109,-115,-120,-126,-131,-136,-142,-147,-152,-157,-162,-167,-171,-176,
	-181,-185,-189,-193,-197,-201,-205,-209,-212,-216,-219,-222,-225,-228,-231,-234,
	-236,-238,-241,-243,-244,-246,-248,-249,-251,-252,-253,-254,-254,-255,-255,-255,
	-256,-255,-255,-255,-254,-254,-253,-252,-251,-249,-248,-246,-244,-243,-241,-238,
	-236,-234,-231,-228,-225,-222,-219,-216,-212,-209,-205,-201,-197,-193,-189,-185,
	-181,-176,-171,-167,-162,-157,-152,-147,-142,-136,-131,-126,-120,-115,-109,-103,
	 -97, -92, -86, -80, -74, -68, -62, -56, -49, -43, -37, -31, -25, -18, -12,  -6
};

#define Sinus(x)	__SinusTable[(x)&0xFF]
#define Cosinus(x)	__SinusTable[((x)+0x40)&0xFF]

#define PI	3.14159265358979323f
BOOL CPaletteBitmap::Animate()
//----------------------------
{
	//included random hacking by rewbs to get funny animation.
	LPBYTE dest, src;
	DWORD t = (timeGetTime() - m_dwStartTime) / 10;
	LONG Dist, Phi, srcx, srcy, spdx, spdy, sizex, sizey;
	bool dir;

	if ((!m_lpRotoZoom) || (!m_lpBmp) || (!m_nRotoWidth) || (!m_nRotoHeight)) return FALSE;
	Sleep(2); 	//give away some CPU

	if (t > 256)
		m_bFirst = FALSE;

	dir = ((t/256) % 2 != 0); //change dir every 256 t
	t = t%256;
	if (!dir) t = (256-t);

	sizex = m_nRotoWidth;
	sizey = m_nRotoHeight;
	m_dwFrameTime = t;
	src = (LPBYTE)&m_lpBmp->bmiColors[256];
	dest = m_lpRotoZoom;
	Dist = t;
	Phi = t;
	spdx = 70000 + Sinus(Phi) * 10000 / 256;
	spdy = 0;
	spdx =(Cosinus(Phi)+Sinus(Phi<<2))*(Dist<<9)/sizex;
	spdy =(Sinus(Phi)+Cosinus(Phi>>2))*(Dist<<9)/sizey;
	srcx = 0x800000 - ((spdx * sizex) >> 1) + (spdy * sizey);
	srcy = 0x800000 - ((spdy * sizex) >> 1) + (spdx * sizey);
	for (UINT y=sizey; y; y--)
	{
		UINT oldx = srcx, oldy = srcy;
		for (UINT x=sizex; x; x--)
		{
			srcx += spdx;
			srcy += spdy;
			*dest++ = src[((srcy & 0x7F0000) >> 8) | ((srcx & 0xFF0000) >> 16)];
		}
		srcx=oldx-spdy;
		srcy=oldy+spdx;
	}
	InvalidateRect(NULL, FALSE);
	UpdateWindow();
	return TRUE;
}


//=============================
class CAboutDlg: public CDialog
//=============================
{
protected:
	CPaletteBitmap m_bmp;
	CCreditStatic m_static;

public:
	CAboutDlg() {}
	~CAboutDlg();

// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);
};

static CAboutDlg *gpAboutDlg = NULL;


CAboutDlg::~CAboutDlg()
//---------------------
{
	gpAboutDlg = NULL;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	//}}AFX_DATA_MAP
}

void CAboutDlg::OnOK()
//--------------------
{
	gpRotoZoom = NULL;
	gpAboutDlg = NULL;
	DestroyWindow();
	delete this;
}


void CAboutDlg::OnCancel()
//------------------------
{
	OnOK();
}


BOOL CAboutDlg::OnInitDialog()
//----------------------------
{
	CDialog::OnInitDialog();
	m_bmp.SubclassDlgItem(IDC_BITMAP1, this);
	m_bmp.LoadBitmap(MAKEINTRESOURCE(IDB_MPTRACK));
	SetDlgItemText(IDC_EDIT2, CString("Build Date: ") + MptVersion::GetBuildDateString().c_str());
	SetDlgItemText(IDC_EDIT3, CString("OpenMPT ") + MptVersion::GetVersionStringExtended().c_str());
	m_static.SubclassDlgItem(IDC_CREDITS,this);
	m_static.SetCredits((mpt::String::Replace(mpt::To(mpt::CharsetLocale, mpt::CharsetUTF8, MptVersion::GetFullCreditsString()), "\n", "|") + "|" + mpt::String::Replace(MptVersion::GetContactString(), "\n", "|" ) + "||||||").c_str());
	m_static.SetSpeed(DISPLAY_SLOW);
	m_static.SetColor(BACKGROUND_COLOR, RGB(138, 165, 219)); // Background Colour
	m_static.SetTransparent(); // Set parts of bitmaps with RGB(192,192,192) transparent
	m_static.SetGradient(GRADIENT_LEFT_DARK);  // Background goes from blue to black from left to right
	// m_static.SetBkImage(IDB_BITMAP1); // Background image
	m_static.StartScrolling();
	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE
}


// App command to run the dialog
void CTrackApp::OnAppAbout()
//--------------------------
{
	if (gpAboutDlg) return;
	gpAboutDlg = new CAboutDlg();
	gpAboutDlg->Create(IDD_ABOUTBOX, m_pMainWnd);
}


/////////////////////////////////////////////////////////////////////////////
// Splash Screen

//=================================
class CSplashScreen: public CDialog
//=================================
{
protected:
	CPaletteBitmap m_Bmp;

public:
	CSplashScreen() {}
	~CSplashScreen();
	BOOL Initialize(CWnd *);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel() { OnOK(); }
};

static CSplashScreen *gpSplashScreen = NULL;

CSplashScreen::~CSplashScreen()
//-----------------------------
{
	gpSplashScreen = NULL;
}


BOOL CSplashScreen::Initialize(CWnd *parent)
//------------------------------------------
{
	Create(IDD_SPLASHSCREEN, parent);
	return TRUE;
}


BOOL CSplashScreen::OnInitDialog()
//--------------------------------
{
	CRect rect;
	int cx, cy, newcx, newcy;

	CDialog::OnInitDialog();
	m_Bmp.SubclassDlgItem(IDC_SPLASH, this);
	m_Bmp.LoadBitmap(MAKEINTRESOURCE(IDB_SPLASHNOFOLDFIN));
	GetWindowRect(&rect);
	cx = rect.Width();
	cy = rect.Height();
	newcx = m_Bmp.GetWidth();
	newcy = m_Bmp.GetHeight();
	if ((newcx) && (newcy))
	{
		LONG ExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
		ExStyle |= WS_EX_TOPMOST;
		SetWindowLong(m_hWnd, GWL_EXSTYLE, ExStyle);
		rect.left -= (newcx - cx) / 2;
		rect.top -= (newcy - cy) / 2;
		SetWindowPos(&wndTop, rect.left, rect.top, newcx, newcy, 0);
		m_Bmp.SetWindowPos(NULL, 0,0, newcx, newcy, SWP_NOZORDER);
	}
	return TRUE;
}


void CSplashScreen::OnOK()
//------------------------
{
	if (gpSplashScreen)
	{
		EndWaitCursor();
		gpSplashScreen = NULL;
	}
	DestroyWindow();
	delete this;
}


VOID CTrackApp::StartSplashScreen()
//---------------------------------
{
	if (!gpSplashScreen)
	{
		gpSplashScreen = new CSplashScreen();
		if (gpSplashScreen)
		{
			gpSplashScreen->Initialize(m_pMainWnd);
			gpSplashScreen->ShowWindow(SW_SHOW);
			gpSplashScreen->UpdateWindow();
			gpSplashScreen->BeginWaitCursor();
		}
	}
}


VOID CTrackApp::StopSplashScreen()
//--------------------------------
{
	if (gpSplashScreen)
	{
		gpSplashScreen->EndWaitCursor();
		gpSplashScreen->DestroyWindow();
		if (gpSplashScreen)
		{
			delete gpSplashScreen;
			gpSplashScreen = NULL;
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// Idle-time processing

BOOL CTrackApp::OnIdle(LONG lCount)
//---------------------------------
{
	BOOL b = CWinApp::OnIdle(lCount);

	if(CMainFrame::GetMainFrame())
	{
		CMainFrame::GetMainFrame()->IdleHandlerSounddevice();
	}

	if ((gpSplashScreen) && (m_bInitialized))
	{
		if (timeGetTime() - m_dwTimeStarted > 1000)		//Set splash screen duration here -rewbs
		{
			StopSplashScreen();
		}
	}
	if (gpRotoZoom)
	{
		if (gpRotoZoom->Animate()) return TRUE;
	}

	// Call plugins idle routine for open editor
	DWORD curTime = timeGetTime();
	// TODO: is it worth the overhead of checking that 10ms have passed,
	//       or should we just do it on every idle message?
	if (m_pPluginManager)
	{
		//rewbs.vstCompliance: call @ 50Hz
		if (curTime - m_dwLastPluginIdleCall > 20) //20ms since last call?
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


void DibBlt(HDC hdc, int x, int y, int sizex, int sizey, int srcx, int srcy, LPMODPLUGDIB lpdib)
//----------------------------------------------------------------------------------------------
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


LPMODPLUGDIB LoadDib(LPCSTR lpszName)
//-----------------------------------
{
	HINSTANCE hInstance = AfxGetInstanceHandle();
	HRSRC hrsrc = FindResource(hInstance, lpszName, RT_BITMAP);
	HGLOBAL hglb = LoadResource(hInstance, hrsrc);
	LPBITMAPINFO p = (LPBITMAPINFO)LockResource(hglb);
	if (p)
	{
		LPMODPLUGDIB pmd = new MODPLUGDIB;
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
	RECT rect;
	HGDIOBJ oldpen = ::SelectObject(hdc, (bPushed) ? CMainFrame::penDarkGray : CMainFrame::penLightGray);
	::MoveToEx(hdc, lpRect->left, lpRect->bottom-1, NULL);
	::LineTo(hdc, lpRect->left, lpRect->top);
	::LineTo(hdc, lpRect->right-1, lpRect->top);
	::SelectObject(hdc, (bPushed) ? CMainFrame::penLightGray : CMainFrame::penDarkGray);
	::LineTo(hdc, lpRect->right-1, lpRect->bottom-1);
	::LineTo(hdc, lpRect->left, lpRect->bottom-1);
	rect.left = lpRect->left + 1;
	rect.top = lpRect->top + 1;
	rect.right = lpRect->right - 1;
	rect.bottom = lpRect->bottom - 1;
	::FillRect(hdc, &rect, CMainFrame::brushGray);
	::SelectObject(hdc, oldpen);
	if ((lpszText) && (lpszText[0]))
	{
		if (bPushed)
		{
			rect.top++;
			rect.left++;
		}
		::SetTextColor(hdc, GetSysColor((bDisabled) ? COLOR_GRAYTEXT : COLOR_BTNTEXT));
		::SetBkMode(hdc, TRANSPARENT);
		HGDIOBJ oldfont = ::SelectObject(hdc, CMainFrame::GetGUIFont());
		::DrawText(hdc, lpszText, -1, &rect, dwFlags | DT_SINGLELINE | DT_NOPREFIX);
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


////////////////////////////////////////////////////////////////////////////////
// CFastBitmap 8-bit output / 4-bit input
// useful for lots of small blits with color mapping
// combined in one big blit

void CFastBitmap::Init(LPMODPLUGDIB lpTextDib)
//--------------------------------------------
{
	m_nBlendOffset = 0;
	m_pTextDib = lpTextDib;
	MemsetZero(m_Dib);
	m_nTextColor = 0;
	m_nBkColor = 1;
	m_Dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_Dib.bmiHeader.biWidth = FASTBMP_MAXWIDTH;
	m_Dib.bmiHeader.biHeight = FASTBMP_MAXHEIGHT;
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
						FASTBMP_MAXHEIGHT - cy,
						0,
						FASTBMP_MAXHEIGHT,
						m_Dib.DibBits,
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
	for (UINT i=0; i<0x80; i++)
	{
		UINT m = (m_Dib.bmiColors[i].rgbRed >> 2)
				+ (m_Dib.bmiColors[i].rgbGreen >> 1)
				+ (m_Dib.bmiColors[i].rgbBlue >> 2);
		m_Dib.bmiColors[i|0x80].rgbRed = static_cast<BYTE>((m + r)>>1);
		m_Dib.bmiColors[i|0x80].rgbGreen = static_cast<BYTE>((m + g)>>1);
		m_Dib.bmiColors[i|0x80].rgbBlue = static_cast<BYTE>((m + b)>>1);
	}
}


// Monochrome 4-bit bitmap (0=text, !0 = back)
void CFastBitmap::TextBlt(int x, int y, int cx, int cy, int srcx, int srcy, LPMODPLUGDIB lpdib)
//---------------------------------------------------------------------------------------------
{
	const BYTE *psrc;
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
	if ((x >= FASTBMP_MAXWIDTH) || (y >= FASTBMP_MAXHEIGHT)) return;
	if (x+cx >= FASTBMP_MAXWIDTH) cx = FASTBMP_MAXWIDTH - x;
	if (y+cy >= FASTBMP_MAXHEIGHT) cy = FASTBMP_MAXHEIGHT - y;
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
	pdest = m_Dib.DibBits + ((FASTBMP_MAXHEIGHT - 1 - y) << FASTBMP_XSHIFT) + x;
	psrc = lpdib->lpDibBits + (srcx >> 1) + (srcy * srcwidth);
	for (int iy=0; iy<cy; iy++)
	{
		LPBYTE p = pdest;
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
		pdest -= FASTBMP_MAXWIDTH;
		psrc += srcinc;
	}
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
	const size_t numPlugins = theApp.GetSettings().Read<int32>("VST Plugins", "NumPlugins", 0);

	#ifndef NO_VST
		std::string buffer = theApp.GetSettings().Read<std::string>("VST Plugins", "HostProductString", CVstPluginManager::s_szHostProductString);

		// Version <= 1.19.03.00 had buggy handling of custom host information. If last open was from
		// such OpenMPT version, clear the related settings to get a clean start.
		if(TrackerSettings::Instance().gcsPreviousVersion != 0 && TrackerSettings::Instance().gcsPreviousVersion < MAKE_VERSION_NUMERIC(1, 19, 03, 01) && buffer == "OpenMPT")
		{
			theApp.GetSettings().Remove("VST Plugins", "HostProductString");
			theApp.GetSettings().Remove("VST Plugins", "HostVendorString");
			theApp.GetSettings().Remove("VST Plugins", "HostVendorVersion");
		}

		mpt::String::Copy(CVstPluginManager::s_szHostProductString, theApp.GetSettings().Read<std::string>("VST Plugins", "HostProductString", CVstPluginManager::s_szHostProductString));
		mpt::String::Copy(CVstPluginManager::s_szHostVendorString, theApp.GetSettings().Read<std::string>("VST Plugins", "HostVendorString", CVstPluginManager::s_szHostVendorString));
		CVstPluginManager::s_nHostVendorVersion = theApp.GetSettings().Read<int32>("VST Plugins", "HostVendorVersion", CVstPluginManager::s_nHostVendorVersion);
	#endif


	std::wstring nonFoundPlugs;
	const mpt::PathString failedPlugin = theApp.GetSettings().Read<mpt::PathString>("VST Plugins", "FailedPlugin", MPT_PATHSTRING(""));

	CDialog pluginScanDlg;
	DWORD scanStart = GetTickCount();
	bool dialogShown = false;

	m_pPluginManager->reserve(numPlugins);
	for(size_t plug = 0; plug < numPlugins; plug++)
	{
		char tmp[32];
		sprintf(tmp, "Plugin%d", plug);
		mpt::PathString plugPath = theApp.GetSettings().Read<mpt::PathString>("VST Plugins", tmp, MPT_PATHSTRING(""));
		if(!plugPath.empty())
		{
			plugPath = RelativePathToAbsolute(plugPath);

			if(plugPath == failedPlugin)
			{
				const std::wstring text = L"The following plugin has previously crashed OpenMPT during initialisation:\n\n" + failedPlugin.ToWide() + L"\n\nDo you still want to load it?";
				if(Reporting::Confirm(text, false, true) == cnfNo)
				{
					continue;
				}
			}
			m_pPluginManager->AddPlugin(plugPath, true, true, &nonFoundPlugs);
		}

		if(!dialogShown && GetTickCount() >= scanStart + 1000)
		{
			// If this is taking too long, show the user what he's waiting for.
			dialogShown = true;
			pluginScanDlg.Create(IDD_SCANPLUGINS, gpSplashScreen);
			pluginScanDlg.ShowWindow(SW_SHOW);
			pluginScanDlg.CenterWindow(gpSplashScreen);
		} else if(dialogShown)
		{
			CWnd *text = pluginScanDlg.GetDlgItem(IDC_SCANTEXT);
			std::wstring scanStr = mpt::String::PrintW(L"Scanning Plugin %1 / %2...\n%3", plug, numPlugins, plugPath);
			SetWindowTextW(text->m_hWnd, scanStr.c_str());
			MSG msg;
			while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
	}
	if(!nonFoundPlugs.empty())
	{
		Reporting::Notification(L"Problems were encountered with plugins:\n" + nonFoundPlugs);
	}
	return FALSE;
}


BOOL CTrackApp::UninitializeDXPlugins()
//-------------------------------------
{
	if(!m_pPluginManager) return FALSE;

#ifndef NO_VST

	size_t plug = 0;
	for(CVstPluginManager::const_iterator pPlug = m_pPluginManager->begin(); pPlug != m_pPluginManager->end(); pPlug++)
	{
		if((**pPlug).pluginId1 != kDmoMagic)
		{
			char tmp[32];
			sprintf(tmp, "Plugin%d", plug);
			mpt::PathString plugPath = (**pPlug).dllPath;
			if(theApp.IsPortableMode())
			{
				plugPath = AbsolutePathToRelative(plugPath);
			}
			theApp.GetSettings().Write<mpt::PathString>("VST Plugins", tmp, plugPath);
			plug++;
		}
	}
	theApp.GetSettings().Write<int32>("VST Plugins", "NumPlugins", plug);
#endif // NO_VST

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
	return OpenURL(mpt::PathString::FromCString(url));
}

bool CTrackApp::OpenURL(const mpt::PathString &lpszURL)
//-----------------------------------------------------
{
	if(!lpszURL.empty() && theApp.m_pMainWnd)
	{
		if(reinterpret_cast<int>(ShellExecuteW(
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
