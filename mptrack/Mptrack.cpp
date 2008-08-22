// mptrack.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "mptrack.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "moddoc.h"
#include "globals.h"
#include "dlsbank.h"
#include "snddev.h"
#include "vstplug.h"
#include "CreditStatic.h"
#include "hyperEdit.h"
#include "bladedll.h"
#include "commctrl.h"
#include "version.h"
#include "test/test.h"

// rewbs.memLeak
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include ".\mptrack.h"
//end  rewbs.memLeak

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only CTrackApp object

CTrackApp theApp;


/////////////////////////////////////////////////////////////////////////////
// Document Template

//=============================================
class CModDocTemplate: public CMultiDocTemplate
//=============================================
{
public:
	CModDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass):
		CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible = TRUE);
};


CDocument *CModDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName, BOOL bMakeVisible)
//-----------------------------------------------------------------------------------
{
	if (lpszPathName)
	{
		TCHAR s[_MAX_EXT];
		_tsplitpath(lpszPathName, NULL, NULL, NULL, s);
		if (!_tcsicmp(s, _TEXT(".dll")))
		{
			CVstPluginManager *pPluginManager = theApp.GetPluginManager();
			if (pPluginManager)
			{
				pPluginManager->AddPlugin(lpszPathName);
			}
			return NULL;
		}
	}
	CDocument *pDoc = CMultiDocTemplate::OpenDocumentFile(lpszPathName, bMakeVisible);
	if (pDoc)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->OnDocumentCreated((CModDoc *)pDoc);
	}
	return pDoc;
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

		lstrcpyn(s, lpszCommand, sizeof(s));
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
				OpenDocumentFile(pszData);
			}
		} else
		// New
		if (!lstrcmpi(pszCmd, "New"))
		{
			OpenDocumentFile(NULL);
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


/////////////////////////////////////////////////////////////////////////////
// Common Tables

LPCSTR szNoteNames[12] =
{
	"C-", "C#", "D-", "D#", "E-", "F-",
	"F#", "G-", "G#", "A-", "A#", "B-"
};

LPCSTR szDefaultNoteNames[NOTE_MAX] = {
	"C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
	"C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
	"C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
	"C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
	"C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
	"C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
	"C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
	"C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7",
	"C-8", "C#8", "D-8", "D#8", "E-8", "F-8", "F#8", "G-8", "G#8", "A-8", "A#8", "B-8",
	"C-9", "C#9", "D-9", "D#9", "E-9", "F-9", "F#9", "G-9", "G#9", "A-9", "A#9", "B-9",
};

LPCSTR szHexChar = "0123456789ABCDEF";
LPCSTR gszModCommands = " 0123456789ABCDRFFTE???GHK?YXPLZ\\:#"; //rewbs.smoothVST: added last \ (written as \\); rewbs.velocity: added last :
LPCSTR gszS3mCommands = " JFEGHLKRXODB?CQATI?SMNVW?UY?P?Z\\:#"; //rewbs.smoothVST: added last \ (written as \\); rewbs.velocity: added last :
LPCSTR gszVolCommands = " vpcdabuhlrgfe:o";	//rewbs.velocity: added last : ; rewbs.volOff added last o


BYTE gEffectColors[MAX_EFFECTS] =
{
	0,					0,					MODCOLOR_PITCH,		MODCOLOR_PITCH,
	MODCOLOR_PITCH,		MODCOLOR_PITCH,		MODCOLOR_VOLUME,	MODCOLOR_VOLUME,
	MODCOLOR_VOLUME,	MODCOLOR_PANNING,	0,					MODCOLOR_VOLUME,
	MODCOLOR_GLOBALS,	MODCOLOR_VOLUME,	MODCOLOR_GLOBALS,	0,
	MODCOLOR_GLOBALS,	MODCOLOR_GLOBALS,	0,					0,					
	0,					MODCOLOR_VOLUME,	MODCOLOR_VOLUME,	MODCOLOR_GLOBALS,	
	MODCOLOR_GLOBALS,	0,					MODCOLOR_PITCH,		MODCOLOR_PANNING,
	MODCOLOR_PITCH,		MODCOLOR_PANNING,	0,					0,
	//0,/*rewbs.smoothVST*/ ,0/*rewbs.velocity*/,
};

static void ShowChangesDialog()
//-----------------------------
{
	/*
	CString firstOpenMessage = "OpenMPT version " + CMainFrame::GetFullVersionString();
	firstOpenMessage +=	". This is a development build.\n\nChanges:\n\n"
	"TODO";

	CMainFrame::GetMainFrame()->MessageBox(firstOpenMessage, "OpenMPT v." + CMainFrame::GetFullVersionString(), MB_ICONINFORMATION);
	*/
}


/////////////////////////////////////////////////////////////////////////////
// MPTRACK Command Line options

//================================================
class CMPTCommandLineInfo: public CCommandLineInfo
//================================================
{
public:
	BOOL m_bNoAcm, m_bNoDls, m_bNoMp3, m_bSafeMode, m_bWavEx, m_bNoPlugins, m_bDebug,
		 m_bNoSettingsOnNewVersion;

	CString m_csExtension;

public:
	CMPTCommandLineInfo() { 
		m_bNoAcm = m_bNoDls = m_bNoMp3 = m_bSafeMode = m_bWavEx = 
		m_bNoPlugins = m_bDebug = m_bNoSettingsOnNewVersion = FALSE; 
		m_csExtension = "";
	}
	virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);
};


void CMPTCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
//-----------------------------------------------------------------------------
{
	if ((lpszParam) && (bFlag))
	{
		if (!lstrcmpi(lpszParam, "nologo")) { m_bShowSplash = FALSE; return; } else
		if (!lstrcmpi(lpszParam, "nodls")) { m_bNoDls = TRUE; return; } else
		if (!lstrcmpi(lpszParam, "noacm")) { m_bNoAcm = TRUE; return; } else
		if (!lstrcmpi(lpszParam, "nomp3")) { m_bNoMp3 = TRUE; return; } else
		if (!lstrcmpi(lpszParam, "wavex")) { m_bWavEx = TRUE; } else
		if (!lstrcmpi(lpszParam, "noplugs")) { m_bNoPlugins = TRUE; } else
		if (!lstrcmpi(lpszParam, "debug")) { m_bDebug = TRUE; } else
		if (!lstrcmpi(lpszParam, "noSettingsOnNewVersion")) { m_bNoSettingsOnNewVersion = TRUE; }
	}
	CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
}


/////////////////////////////////////////////////////////////////////////////
// Midi Library

LPMIDILIBSTRUCT CTrackApp::glpMidiLibrary = NULL;

BOOL CTrackApp::ImportMidiConfig(LPCSTR lpszConfigFile, BOOL bNoWarn)
//-------------------------------------------------------------------
{
	CHAR szFileName[_MAX_PATH], s[_MAX_PATH], szUltraSndPath[_MAX_PATH];
	
	if ((!lpszConfigFile) || (!lpszConfigFile[0])) return FALSE;
	if (!glpMidiLibrary)
	{
		glpMidiLibrary = new MIDILIBSTRUCT;
		if (!glpMidiLibrary) return FALSE;
		memset(glpMidiLibrary, 0, sizeof(MIDILIBSTRUCT));
	}
	if (CDLSBank::IsDLSBank(lpszConfigFile))
	{
		UINT id = IDYES;
		if (!bNoWarn)
		{
			id = CMainFrame::GetMainFrame()->MessageBox("You are about to replace the current midi library:\n"
												"Do you want to replace only the missing instruments? (recommended)",
												"Warning", MB_YESNOCANCEL|MB_ICONQUESTION );
		}
		if (id == IDCANCEL) return FALSE;
		BOOL bReplaceAll = (id == IDNO) ? TRUE : FALSE;
		CDLSBank dlsbank;
		if (dlsbank.Open(lpszConfigFile))
		{
			for (UINT iIns=0; iIns<256; iIns++)
			{
				if ((bReplaceAll) || (!glpMidiLibrary->MidiMap[iIns]) || (!glpMidiLibrary->MidiMap[iIns][0]))
				{
					DWORD dwProgram = (iIns < 128) ? iIns : 0xFF;
					DWORD dwKey = (iIns < 128) ? 0xFF : iIns & 0x7F;
					DWORD dwBank = (iIns < 128) ? 0 : F_INSTRUMENT_DRUMS;
					if (dlsbank.FindInstrument((iIns < 128) ? FALSE : TRUE,	dwBank, dwProgram, dwKey))
					{
						if (!glpMidiLibrary->MidiMap[iIns])
						{
							if ((glpMidiLibrary->MidiMap[iIns] = new CHAR[_MAX_PATH]) == NULL) break;
						}
						strcpy(glpMidiLibrary->MidiMap[iIns], lpszConfigFile);
					}
				}
			}
		}
		return TRUE;
	}
	GetPrivateProfileString("Ultrasound", "PatchDir", "", szUltraSndPath, sizeof(szUltraSndPath), lpszConfigFile);
	if (!strcmp(szUltraSndPath, ".\\")) szUltraSndPath[0] = 0;
	if (!szUltraSndPath[0]) GetCurrentDirectory(sizeof(szUltraSndPath), szUltraSndPath);
	for (UINT iMidi=0; iMidi<256; iMidi++)
	{
		szFileName[0] = 0;
		wsprintf(s, (iMidi < 128) ? "Midi%d" : "Perc%d", iMidi & 0x7f);
		GetPrivateProfileString("Midi Library", s, "", szFileName, sizeof(szFileName), lpszConfigFile);
		// Check for ULTRASND.INI
		if (!szFileName[0])
		{
			LPCSTR pszSection = (iMidi < 128) ? "Melodic Patches" : "Drum Patches";
			wsprintf(s, "%d", iMidi & 0x7f);
			GetPrivateProfileString(pszSection, s, "", szFileName, sizeof(szFileName), lpszConfigFile);
			if (!szFileName[0])
			{
				pszSection = (iMidi < 128) ? "Melodic Bank 0" : "Drum Bank 0";
				GetPrivateProfileString(pszSection, s, "", szFileName, sizeof(szFileName), lpszConfigFile);
			}
			if (szFileName[0])
			{
				s[0] = 0;
				if (szUltraSndPath[0])
				{
					strcpy(s, szUltraSndPath);
					int len = strlen(s)-1;
					if ((len) && (s[len-1] != '\\')) strcat(s, "\\");
				}
				strncat(s, szFileName, sizeof(s));
				strncat(s, ".pat", sizeof(s));
				strcpy(szFileName, s);
			}
		}
		if (szFileName[0])
		{
			if (!glpMidiLibrary->MidiMap[iMidi])
			{
				if ((glpMidiLibrary->MidiMap[iMidi] = new CHAR[_MAX_PATH]) == NULL) return FALSE;
			}
			if ((lpszConfigFile[1] == ':') && (szFileName[1] != ':'))
			{
				s[0] = lpszConfigFile[0];
				s[1] = lpszConfigFile[1];
				s[2] = 0;
				strncat(s, szFileName, sizeof(s));
				s[sizeof(s)-1] = 0;
				strcpy(szFileName, s);
			}
			strcpy(glpMidiLibrary->MidiMap[iMidi], szFileName);
		}
	}
	return FALSE;
}


BOOL CTrackApp::ExportMidiConfig(LPCSTR lpszConfigFile)
//-----------------------------------------------------
{
	CHAR szFileName[_MAX_PATH], s[128];
	
	if ((!glpMidiLibrary) || (!lpszConfigFile) || (!lpszConfigFile[0])) return FALSE;
	for (UINT iMidi=0; iMidi<256; iMidi++) if (glpMidiLibrary->MidiMap[iMidi])
	{
		if (iMidi < 128)
			wsprintf(s, "Midi%d", iMidi);
		else
			wsprintf(s, "Perc%d", iMidi & 0x7F);
		if ((glpMidiLibrary->MidiMap[iMidi][1] == ':') && (lpszConfigFile[1] == ':')
		 && ((glpMidiLibrary->MidiMap[iMidi][0]|0x20) == (lpszConfigFile[0]|0x20)))
		{
			strcpy(szFileName, glpMidiLibrary->MidiMap[iMidi]+2);
		} else
		{
			strcpy(szFileName, glpMidiLibrary->MidiMap[iMidi]);
		}
		if (szFileName[0])
		{
			if (!WritePrivateProfileString("Midi Library", s, szFileName, lpszConfigFile)) break;
		}
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// DLS Banks support

#define MPTRACK_REG_DLS		"Software\\Olivier Lapicque\\ModPlug Tracker\\DLS Banks"
CDLSBank *CTrackApp::gpDLSBanks[MAX_DLS_BANKS];


BOOL CTrackApp::LoadDefaultDLSBanks()
//-----------------------------------
{
	CHAR szFileName[MAX_PATH];
	HKEY key;

	CString storedVersion = CMainFrame::GetPrivateProfileCString("Version", "Version", "", theApp.GetConfigFileName());
	//If version number stored in INI is 1.17.02.40 or later, load DLS from INI file.
	//Else load DLS from Registry
	if (storedVersion >= "1.17.02.40") {
		CHAR s[MAX_PATH];
		UINT numBanks = CMainFrame::GetPrivateProfileLong("DLS Banks", "NumBanks", 0, theApp.GetConfigFileName());
		for (UINT i=0; i<numBanks; i++) {
			wsprintf(s, "Bank%d", i+1);
			CString dlsFileName = CMainFrame::GetPrivateProfileCString("DLS Banks", s, "", theApp.GetConfigFileName());
			AddDLSBank(dlsFileName);
		}
	} else {
		LoadRegistryDLS();
	}

	SaveDefaultDLSBanks(); // This will avoid a crash the next time if we crash while loading the bank

	szFileName[0] = 0;
	GetSystemDirectory(szFileName, sizeof(szFileName));
	lstrcat(szFileName, "\\GM.DLS");
	if (!AddDLSBank(szFileName))
	{
		GetWindowsDirectory(szFileName, sizeof(szFileName));
		lstrcat(szFileName, "\\SYSTEM32\\DRIVERS\\GM.DLS");
		if (!AddDLSBank(szFileName))
		{
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectMusic", 0, KEY_READ, &key) == ERROR_SUCCESS)
			{
				DWORD dwRegType = REG_SZ;
				DWORD dwSize = sizeof(szFileName);
				szFileName[0] = 0;
				if (RegQueryValueEx(key, "GMFilePath", NULL, &dwRegType, (LPBYTE)&szFileName, &dwSize) == ERROR_SUCCESS)
				{
					AddDLSBank(szFileName);
				}
				RegCloseKey(key);
			}
		}
	}
	if (glpMidiLibrary) ImportMidiConfig(szFileName, TRUE);

	return TRUE;
}

void CTrackApp::LoadRegistryDLS()
//-------------------------------
{
	CHAR szFileNameX[_MAX_PATH];
	HKEY keyX;

	if (RegOpenKeyEx(HKEY_CURRENT_USER,	MPTRACK_REG_DLS, 0, KEY_READ, &keyX) == ERROR_SUCCESS)
	{
		DWORD dwRegType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		DWORD d = 0;
		if (RegQueryValueEx(keyX, "NumBanks", NULL, &dwRegType, (LPBYTE)&d, &dwSize) == ERROR_SUCCESS)
		{
			CHAR s[64];
			for (UINT i=0; i<d; i++)
			{
				wsprintf(s, "Bank%d", i+1);
				szFileNameX[0] = 0;
				dwRegType = REG_SZ;
				dwSize = sizeof(szFileNameX);
				RegQueryValueEx(keyX, s, NULL, &dwRegType, (LPBYTE)szFileNameX, &dwSize);
				AddDLSBank(szFileNameX);
			}
		}
		RegCloseKey(keyX);
	}
}


BOOL CTrackApp::SaveDefaultDLSBanks()
//-----------------------------------
{
	CHAR s[64];
	DWORD nBanks = 0;
	for (UINT i=0; i<MAX_DLS_BANKS; i++) {
		
		if (!gpDLSBanks[i]) {
			continue;
		}
		
		LPCSTR pszBankName = gpDLSBanks[i]->GetFileName();
		if (!(pszBankName) || !(pszBankName[0])) {
			continue;
		}

		wsprintf(s, "Bank%d", nBanks+1);
		WritePrivateProfileString("DLS Banks", s, pszBankName, theApp.GetConfigFileName());
		nBanks++;

	}
	CMainFrame::WritePrivateProfileLong("DLS Banks", "NumBanks", nBanks, theApp.GetConfigFileName());
	return TRUE;
}


BOOL CTrackApp::RemoveDLSBank(UINT nBank)
//---------------------------------------
{
	if ((nBank >= MAX_DLS_BANKS) || (!gpDLSBanks[nBank])) return FALSE;
	delete gpDLSBanks[nBank];
	gpDLSBanks[nBank] = NULL;
	return TRUE;
}


BOOL CTrackApp::AddDLSBank(LPCSTR lpszFileName)
//---------------------------------------------
{
	if ((!lpszFileName) || (!lpszFileName[0]) || (!CDLSBank::IsDLSBank(lpszFileName))) return FALSE;
	for (UINT j=0; j<MAX_DLS_BANKS; j++) if (gpDLSBanks[j])
	{
		if (!lstrcmpi(lpszFileName, gpDLSBanks[j]->GetFileName())) return TRUE;
	}
	for (UINT i=0; i<MAX_DLS_BANKS; i++) if (!gpDLSBanks[i])
	{
		gpDLSBanks[i] = new CDLSBank;
		gpDLSBanks[i]->Open(lpszFileName);
		return TRUE;
	}
	return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CTrackApp

UINT CTrackApp::m_nDefaultDocType = MOD_TYPE_IT;
MEMORYSTATUS CTrackApp::gMemStatus;

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
	ON_COMMAND(ID_APP_ABOUT,	OnAppAbout)
	ON_COMMAND(ID_HELP_INDEX,	CWinApp::OnHelpIndex)
	ON_COMMAND(ID_HELP_FINDER,	CWinApp::OnHelpFinder)
	ON_COMMAND(ID_HELP_USING,	CWinApp::OnHelpUsing)
	ON_COMMAND(ID_HELP_SEARCH,	OnHelpSearch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrackApp construction

CTrackApp::CTrackApp()
//--------------------
{
	m_pModTemplate = NULL;
	m_pPluginManager = NULL;
	m_bInitialized = FALSE;
	m_bLayer3Present = FALSE;
	m_bExWaveSupport = FALSE;
	m_bDebugMode = FALSE;
	m_hBladeEnc = NULL;
	m_hLameEnc = NULL;
	m_hACMInst = NULL;
	m_hAlternateResourceHandle = NULL;
	m_szConfigFileName[0] = 0;
	for (UINT i=0; i<MAX_DLS_BANKS; i++) gpDLSBanks[i] = NULL;
	// Default macro config
	memset(&m_MidiCfg, 0, sizeof(m_MidiCfg));
	strcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_START*32], "FF");
	strcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_STOP*32], "FC");
	strcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_NOTEON*32], "9c n v");
	strcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_NOTEOFF*32], "9c n 0");
	strcpy(&m_MidiCfg.szMidiGlb[MIDIOUT_PROGRAM*32], "Cc p");
	strcpy(&m_MidiCfg.szMidiSFXExt[0], "F0F000z");
	for (int iz=0; iz<16; iz++) wsprintf(&m_MidiCfg.szMidiZXXExt[iz*32], "F0F001%02X", iz*8);

	#ifdef UPDATECHECKENABLED
		m_pRequestContext = NULL;
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// GetDSoundVersion

static DWORD GetDSoundVersion()
//-----------------------------
{
	DWORD dwVersion = 0x600;
	HKEY key = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\DirectX", 0, KEY_READ, &key) == ERROR_SUCCESS)
	{
		CHAR szVersion[32] = "";
		DWORD dwSize = sizeof(szVersion);
		DWORD dwType = REG_SZ;
		if (RegQueryValueEx(key, "Version", NULL, &dwType, (LPBYTE)szVersion, &dwSize) == ERROR_SUCCESS)
		{
			// "4.06.03.xxxx"
			dwVersion = ((szVersion[3] - '0') << 8) | ((szVersion[5] - '0') << 4) | ((szVersion[6] - '0'));
			if (dwVersion < 0x600) dwVersion = 0x600;
			if (dwVersion > 0x800) dwVersion = 0x800;
		}
		RegCloseKey(key);
	}
	return dwVersion;
}


/////////////////////////////////////////////////////////////////////////////
// CTrackApp initialization

void Terminate_AppThread()
//----------------------------------------------
{	
	//TODO: Why does this not get called.
	AfxMessageBox("Application thread terminated unexpectedly. Attempting to shut down audio device");
	CMainFrame* pMainFrame = CMainFrame::GetMainFrame();
	if (pMainFrame->gpSoundDevice) pMainFrame->gpSoundDevice->Reset();
	pMainFrame->audioCloseDevice();
	exit(-1);
}

BOOL CTrackApp::InitInstance()
//----------------------------
{

	set_terminate(Terminate_AppThread);

	// Initialize OLE MFC support
	AfxOleInit();
	// Standard initialization
/*
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
*/

	// Change the registry key under which our settings are stored.
	//SetRegistryKey(_T("Olivier Lapicque"));
	// Start loading
	BeginWaitCursor();

	//m_hAlternateResourceHandle = LoadLibrary("mpt_intl.dll");

	memset(&gMemStatus, 0, sizeof(gMemStatus));
	GlobalMemoryStatus(&gMemStatus);
#if 0
	Log("Physical: %lu\n", gMemStatus.dwTotalPhys);
	Log("Page File: %lu\n", gMemStatus.dwTotalPageFile);
	Log("Virtual: %lu\n", gMemStatus.dwTotalVirtual);
#endif
	// Allow allocations of at least 16MB
	if (gMemStatus.dwTotalPhys < 16*1024*1024) gMemStatus.dwTotalPhys = 16*1024*1024;

	ASSERT(NULL == m_pDocManager);
	m_pDocManager = new CModDocManager();

#ifdef _DEBUG
		ASSERT((sizeof(MODCHANNEL)&7) == 0);
	// Disabled by rewbs for smoothVST. Might cause minor perf issues due to increased cache misses?
#endif

	m_szConfigFileName[0] = 0;
	m_szStringsFileName[0] = 0;

	if (GetModuleFileName(NULL, m_szConfigFileName, sizeof(m_szConfigFileName)))
	{
		CHAR szDrive[_MAX_DRIVE]="", szDir[_MAX_PATH]="";
		_splitpath(m_szConfigFileName, szDrive, szDir, NULL, NULL);
		m_szConfigFileName[0] = 0;
		lstrcpyn(m_szConfigFileName, szDrive, sizeof(m_szConfigFileName));
		strncat(m_szConfigFileName, szDir, sizeof(m_szConfigFileName));
		m_szConfigFileName[sizeof(m_szConfigFileName)-1] = 0;
		strncat(m_szConfigFileName, "mptrack.ini", sizeof(m_szConfigFileName));
		m_szConfigFileName[sizeof(m_szConfigFileName)-1] = 0;

		m_szPluginCacheFileName[0] = 0;
		lstrcpyn(m_szPluginCacheFileName, szDrive, sizeof(m_szPluginCacheFileName));
		strncat(m_szPluginCacheFileName, szDir, sizeof(m_szPluginCacheFileName));
		m_szPluginCacheFileName[sizeof(m_szPluginCacheFileName)-1] = 0;
		strncat(m_szPluginCacheFileName, "plugin.cache", sizeof(m_szPluginCacheFileName));
		m_szPluginCacheFileName[sizeof(m_szPluginCacheFileName)-1] = 0;

		m_szStringsFileName[0] = 0;
		lstrcpyn(m_szStringsFileName, szDrive, sizeof(m_szStringsFileName));
		strncat(m_szStringsFileName, szDir, sizeof(m_szStringsFileName));
		m_szStringsFileName[sizeof(m_szStringsFileName)-1] = 0;
		strncat(m_szStringsFileName, "mpt_intl.ini", sizeof(m_szStringsFileName));
		m_szStringsFileName[sizeof(m_szStringsFileName)-1] = 0;
	}

	//Force use of custom ini file rather than windowsDir\executableName.ini
	if (m_pszProfileName) {
		free((void *)m_pszProfileName);
	}
	m_pszProfileName = _tcsdup(m_szConfigFileName); 


	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)

	// Register document templates
	m_pModTemplate = new CModDocTemplate(
		IDR_MODULETYPE,
		RUNTIME_CLASS(CModDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CModControlView));
	AddDocTemplate(m_pModTemplate);

	// Initialize Audio
	CSoundFile::InitSysInfo();
	if (CSoundFile::gdwSysInfo & SYSMIX_ENABLEMMX)
	{
		CMainFrame::m_dwSoundSetup |= SOUNDSETUP_ENABLEMMX;
		CMainFrame::m_nSrcMode = SRCMODE_SPLINE;
	}
	if (CSoundFile::gdwSysInfo & SYSMIX_MMXEX)
	{
		CMainFrame::m_nSrcMode = SRCMODE_POLYPHASE;
	}
	// Load Midi Library
	if (m_szConfigFileName[0]) ImportMidiConfig(m_szConfigFileName);

	// Load default macro configuration
	for (UINT isfx=0; isfx<16; isfx++)
	{
		CHAR s[64], snam[32];
		wsprintf(snam, "SF%X", isfx);
		GetPrivateProfileString("Zxx Macros", snam, &m_MidiCfg.szMidiSFXExt[isfx*32], s, sizeof(s), m_szConfigFileName);
		s[31] = 0;
		memcpy(&m_MidiCfg.szMidiSFXExt[isfx*32], s, 32);
	}
	for (UINT izxx=0; izxx<128; izxx++)
	{
		CHAR s[64], snam[32];
		wsprintf(snam, "Z%02X", izxx|0x80);
		GetPrivateProfileString("Zxx Macros", snam, &m_MidiCfg.szMidiZXXExt[izxx*32], s, sizeof(s), m_szConfigFileName);
		s[31] = 0;
		memcpy(&m_MidiCfg.szMidiZXXExt[izxx*32], s, 32);
	}

	// Parse command line for standard shell commands, DDE, file open
	CMPTCommandLineInfo cmdInfo;
	if (GetDSoundVersion() >= 0x0700) cmdInfo.m_bWavEx = TRUE;
	ParseCommandLine(cmdInfo);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame(/*cmdInfo.m_csExtension*/);
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME)) return FALSE;
	m_pMainWnd = pMainFrame;

	if (cmdInfo.m_bShowSplash)
	{
		StartSplashScreen();
	}
	m_bDebugMode = cmdInfo.m_bDebug;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();

	// Register MOD extensions
	RegisterExtensions();

	// Load DirectSound (if available)
	m_bExWaveSupport = cmdInfo.m_bWavEx;
	SndDevInitialize();

	// Load DLS Banks
	if (!cmdInfo.m_bNoDls) LoadDefaultDLSBanks();

	// Initialize ACM Support
	if (GetProfileInt("Settings", "DisableACM", 0)) cmdInfo.m_bNoAcm = TRUE;
	if (!cmdInfo.m_bNoMp3) InitializeACM(cmdInfo.m_bNoAcm);

	// Initialize DXPlugins
	if (!cmdInfo.m_bNoPlugins) InitializeDXPlugins();

	// Initialize localized strings
	ImportLocalizedStrings();

	// Initialize CMainFrame
	pMainFrame->Initialize();
	InitCommonControls();
	m_dwLastPluginIdleCall=0;	//rewbs.VSTCompliance
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

	/*
	if (CMainFrame::gnCheckForUpdates) {
		try {
			UpdateCheck();
		} catch (...) {
			// Could not do update check. Don't compain - do nothing.
			// Assuming winhttp.dll is set as delay-load in the project settings,
			// we will end up here if the dll cannot be foung (e.g. on win98).
		}
	}
	*/

	// Open settings if the previous execution was with an earlier version.
	if (!cmdInfo.m_bNoSettingsOnNewVersion && MptVersion::ToNum(CMainFrame::gcsPreviousVersion) < MptVersion::num) {
		StopSplashScreen();
		ShowChangesDialog();
		m_pMainWnd->PostMessage(WM_COMMAND, ID_VIEW_OPTIONS);
	}

	EndWaitCursor();

	#ifdef ENABLE_TESTS
		MptTest::DoTests();
	#endif

	return TRUE;
}


#ifdef UPDATECHECKENABLED
void __stdcall CTrackApp::InternetRequestCallback( HINTERNET /*hInternet*/, DWORD_PTR userData, DWORD dwInternetStatus,
                              LPVOID /*lpvStatusInformation*/,  DWORD /*dwStatusInformationLength*/)
//-----------------------------------------------------------------------------------------------------
{

	REQUEST_CONTEXT *pRequestContext = (REQUEST_CONTEXT*)userData;
	if (pRequestContext->hRequest == NULL) {
		return;
	}

	DWORD versionBytesToRead = 10;

	switch (dwInternetStatus) {
		case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
			if (!WinHttpReceiveResponse(pRequestContext->hRequest, NULL)) {
				CleanupInternetRequest(pRequestContext);
				return;
			}
			break;
		case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
			{
				DWORD dwDownloaded = 0;
				if (!WinHttpQueryDataAvailable(pRequestContext->hRequest, &dwDownloaded)) {
					Log("Error %u in WinHttpQueryDataAvailable.\n",GetLastError());
					CleanupInternetRequest(pRequestContext);
					return;
				}
				if (dwDownloaded<versionBytesToRead) {
					Log("Downloaded %d bytes, expected at least %d\n", dwDownloaded, versionBytesToRead);
					CleanupInternetRequest(pRequestContext);
					return;
				}
				break;
			}
		case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
			{
				DWORD dwRead = 0;
				pRequestContext->lpBuffer = new char[versionBytesToRead+1];
				ZeroMemory(pRequestContext->lpBuffer, versionBytesToRead+1);

				if (!WinHttpReadData(pRequestContext->hRequest, (LPVOID)pRequestContext->lpBuffer, versionBytesToRead, &dwRead)) {
					Log("Error %u in WinHttpReadData.\n", GetLastError());
					CleanupInternetRequest(pRequestContext);
					return;
				}
				if (dwRead<versionBytesToRead) {
					Log("Read %d bytes, expected at least %d\n", dwRead, versionBytesToRead);
					CleanupInternetRequest(pRequestContext);
					return;
				}
				break;
			}
		case WINHTTP_CALLBACK_STATUS_READ_COMPLETE: 
			{
				CString remoteVersion = pRequestContext->lpBuffer;
				CString localVersion = CMainFrame::GetFullVersionString();
				if (remoteVersion > localVersion) {
					CString message;
					message.Format("New version available: %s (you are using %s). Would you like more information?", remoteVersion, localVersion);
					if (AfxMessageBox(message, MB_ICONQUESTION|MB_YESNO ) == IDYES) {
						CString URL;
						URL.Format("http://openmpt.xwiki.com/xwiki/bin/view/Development/Builds?currentVersion=%s", localVersion);
						CTrackApp::OpenURL(URL);
					}
				}
				CleanupInternetRequest(pRequestContext);
				break;
			}
		default:
			Log("Unhandled callback - status %d given", dwInternetStatus);
			break;
	}

}

void CTrackApp::CleanupInternetRequest(REQUEST_CONTEXT *pRequestContext)
//-------------------------------------------------------------------------
{
	if (pRequestContext != NULL) {
		if (pRequestContext->lpBuffer != NULL) {
			delete[] pRequestContext->lpBuffer;
			pRequestContext->lpBuffer = NULL;
		}
		
		if (pRequestContext->postData != NULL) {
			delete[] pRequestContext->postData;
			pRequestContext->postData = NULL;
		}

		if (pRequestContext->hRequest != NULL) {
			WinHttpSetStatusCallback(pRequestContext->hRequest, NULL, NULL, NULL);
			WinHttpCloseHandle(pRequestContext->hRequest);
			pRequestContext->hRequest = NULL;
		}

		if (pRequestContext->hConnection != NULL) {
			WinHttpCloseHandle(pRequestContext->hConnection);
			pRequestContext->hConnection = NULL;
		}

		if (pRequestContext->hSession != NULL) {
			WinHttpCloseHandle(pRequestContext->hSession);
			pRequestContext->hSession = NULL;
		}
	}
}

void CTrackApp::UpdateCheck() 
//---------------------------
{
		m_pRequestContext = new REQUEST_CONTEXT();
		m_pRequestContext->hSession = NULL;
		m_pRequestContext->hConnection = NULL;
		m_pRequestContext->hRequest = NULL;
		m_pRequestContext->lpBuffer = NULL;
		m_pRequestContext->postData = NULL;

		// Prepare post data
		if (CMainFrame::gcsInstallGUID == "") {
			//No GUID found in INI file - generate one.
			GUID guid;
			CoCreateGuid(&guid);
			BYTE* Str;
			UuidToString((UUID*)&guid, &Str);
			CMainFrame::gcsInstallGUID.Format("%s", (LPTSTR)Str);
			RpcStringFree(&Str);
		}

		CString csPostData;
		csPostData.Format("install_id=%s&install_version=%s", CMainFrame::gcsInstallGUID, CMainFrame::GetFullVersionString());
		int length = csPostData.GetLength();
		m_pRequestContext->postData = new char[length+1];
		strcpy(m_pRequestContext->postData, csPostData);
		
		m_pRequestContext->hSession = WinHttpOpen( L"OpenMPT/1.17", 	WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
								WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
		if (m_pRequestContext->hSession==NULL) {
			CleanupInternetRequest(m_pRequestContext);
			return;
		}
	
		m_pRequestContext->hConnection = WinHttpConnect(m_pRequestContext->hSession, L"www.soal.org", INTERNET_DEFAULT_HTTP_PORT, 0);
		if (m_pRequestContext->hConnection==NULL) {
			CleanupInternetRequest(m_pRequestContext);
			return;
		}

		m_pRequestContext->hRequest = WinHttpOpenRequest(m_pRequestContext->hConnection, L"POST", L"openmpt/OpenMPTversionCheck.php5",
								NULL, NULL, NULL, 0);
		if (m_pRequestContext->hRequest==NULL) {
			CleanupInternetRequest(m_pRequestContext);
			return;
		}

		if (!WinHttpAddRequestHeaders(m_pRequestContext->hRequest, L"Content-Type:application/x-www-form-urlencoded\r\n\r\n", 
			-1L, WINHTTP_ADDREQ_FLAG_ADD)) {
			CleanupInternetRequest(m_pRequestContext);
			return;	
		}

		WINHTTP_STATUS_CALLBACK pCallback = WinHttpSetStatusCallback(m_pRequestContext->hRequest,
                        static_cast<WINHTTP_STATUS_CALLBACK>(InternetRequestCallback),
                        WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS,    
                        NULL);
		if (pCallback == WINHTTP_INVALID_STATUS_CALLBACK) {
			Log("Error %d in WinHttpSetStatusCallback.\n", WINHTTP_INVALID_STATUS_CALLBACK);
			CleanupInternetRequest(m_pRequestContext);
			return;
		}

		if (!WinHttpSendRequest(m_pRequestContext->hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, m_pRequestContext->postData, length, length, (DWORD_PTR)m_pRequestContext)) {
			CleanupInternetRequest(m_pRequestContext);
           return;
        }
}

#endif


int CTrackApp::ExitInstance()
//---------------------------
{
	//::MessageBox("Exiting/Crashing");
	SndDevUninitialize();
	if (glpMidiLibrary)
	{
		if (m_szConfigFileName[0]) ExportMidiConfig(m_szConfigFileName);
		for (UINT iMidi=0; iMidi<256; iMidi++) {
			if (glpMidiLibrary->MidiMap[iMidi]) {
				delete[] glpMidiLibrary->MidiMap[iMidi];
			}
		}
		delete glpMidiLibrary;
		glpMidiLibrary = NULL;
	}
	SaveDefaultDLSBanks();
	for (UINT i=0; i<MAX_DLS_BANKS; i++)
	{
		if (gpDLSBanks[i])
		{
			delete gpDLSBanks[i];
			gpDLSBanks[i] = NULL;
		}
	}
	// Save default macro configuration
	if (m_szConfigFileName[0])
	{
		for (UINT isfx=0; isfx<16; isfx++)
		{
			CHAR s[64], snam[32];
			wsprintf(snam, "SF%X", isfx);
			memcpy(s, &m_MidiCfg.szMidiSFXExt[isfx*32], 32);
			s[31] = 0;
			if (!WritePrivateProfileString("Zxx Macros", snam, s, m_szConfigFileName)) break;
		}
		for (UINT izxx=0; izxx<128; izxx++)
		{
			CHAR s[64], snam[32];
			wsprintf(snam, "Z%02X", izxx|0x80);
			memcpy(s, &m_MidiCfg.szMidiZXXExt[izxx*32], 32);
			s[31] = 0;
			if (!WritePrivateProfileString("Zxx Macros", snam, s, m_szConfigFileName)) break;
		}
	}
	// Uninitialize DX-Plugins
	UninitializeDXPlugins();

	// Uninitialize ACM
	UninitializeACM();

	// Cleanup the internet request, in case it is still active.
	#ifdef UPDATECHECKENABLED
	CleanupInternetRequest(m_pRequestContext);
	delete m_pRequestContext;
	#endif

	return CWinApp::ExitInstance();
}


////////////////////////////////////////////////////////////////////////////////
// Chords

void CTrackApp::LoadChords(PMPTCHORD pChords)
//-------------------------------------------
{	
	if (!m_szConfigFileName[0]) return;
	for (UINT i=0; i<3*12; i++)
	{
		LONG chord;
		if ((chord = GetPrivateProfileInt("Chords", szDefaultNoteNames[i], -1, m_szConfigFileName)) >= 0)
		{
			if ((chord & 0xFFFFFFC0) || (!pChords[i].notes[0]))
			{
				pChords[i].key = (BYTE)(chord & 0x3F);
				pChords[i].notes[0] = (BYTE)((chord >> 6) & 0x3F);
				pChords[i].notes[1] = (BYTE)((chord >> 12) & 0x3F);
				pChords[i].notes[2] = (BYTE)((chord >> 18) & 0x3F);
			}
		}
	}
}


void CTrackApp::SaveChords(PMPTCHORD pChords)
//-------------------------------------------
{
	CHAR s[64];
	
	if (!m_szConfigFileName[0]) return;
	for (UINT i=0; i<3*12; i++)
	{
		wsprintf(s, "%d", (pChords[i].key) | (pChords[i].notes[0] << 6) | (pChords[i].notes[1] << 12) | (pChords[i].notes[2] << 18));
		if (!WritePrivateProfileString("Chords", szDefaultNoteNames[i], s, m_szConfigFileName)) break;
	}
}


////////////////////////////////////////////////////////////////////////////////
// App Messages


void CTrackApp::OnFileNew()
//-------------------------
{
	if (m_bInitialized) OnFileNewIT();
}


void CTrackApp::OnFileNewMOD()
//----------------------------
{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_MOD);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(NULL);
}


void CTrackApp::OnFileNewS3M()
//----------------------------
{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_S3M);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(NULL);
}


void CTrackApp::OnFileNewXM()
//---------------------------
{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_XM);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(NULL);
}


void CTrackApp::OnFileNewIT()
//---------------------------
{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	SetAsProject(FALSE);
// -! NEW_FEATURE#0023

	SetDefaultDocType(MOD_TYPE_IT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(NULL);
}

void CTrackApp::OnFileNewMPT()
//---------------------------
{
	SetAsProject(FALSE);
	SetDefaultDocType(MOD_TYPE_MPT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(NULL);
}



// -> CODE#0023
// -> DESC="IT project files (.itp)"
void CTrackApp::OnFileNewITProject()
//----------------------------------
{
	SetAsProject(TRUE);
	SetDefaultDocType(MOD_TYPE_IT);
	if (m_pModTemplate) m_pModTemplate->OpenDocumentFile(NULL);
}
// -! NEW_FEATURE#0023


void CTrackApp::OnFileOpen()
//--------------------------
{
	CFileDialog dlg(TRUE,
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_FORCESHOWHIDDEN | OFN_ALLOWMULTISELECT,
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//					"All Modules|*.mod;*.nst;*.wow;*.s3m;*.stm;*.669;*.mtm;*.xm;*.it;*.ult;*.mdz;*.s3z;*.xmz;*.itz;mod.*;*.far;*.mdl;*.okt;*.dmf;*.ptm;*.mdr;*.med;*.ams;*.dbm;*.dsm;*.mid;*.rmi;*.smf;*.bak;*.umx;*.amf;*.psm;*.mt2|"
					"All Modules|*.mod;*.nst;*.wow;*.s3m;*.stm;*.669;*.mtm;*.xm;*.it;*.itp;*.mptm;*.ult;*.mdz;*.s3z;*.xmz;*.itz;mod.*;*.far;*.mdl;*.okt;*.dmf;*.ptm;*.mdr;*.med;*.ams;*.dbm;*.dsm;*.mid;*.rmi;*.smf;*.umx;*.amf;*.psm;*.mt2|"
// -! NEW_FEATURE#0023
					"Compressed Modules (*.mdz;*.s3z;*.xmz;*.itz)|*.mdz;*.s3z;*.xmz;*.itz;*.mdr;*.zip;*.rar;*.lha|"
					"ProTracker Modules (*.mod,*.nst)|*.mod;mod.*;*.mdz;*.nst;*.m15|"
					"ScreamTracker Modules (*.s3m,*.stm)|*.s3m;*.stm;*.s3z|"
					"FastTracker Modules (*.xm)|*.xm;*.xmz|"
					"Impulse Tracker Modules (*.it)|*.it;*.itz|"
// -> CODE#0023
// -> DESC="IT project files (.itp)"
					"Impulse Tracker Projects (*.itp)|*.itp;*.itpz|"
// -! NEW_FEATURE#0023
					"Open MPT Modules (*.mptm)|*.mptm;*.mptmz|"
					"Other Modules (mtm,okt,mdl,669,far,...)|*.mtm;*.669;*.ult;*.wow;*.far;*.mdl;*.okt;*.dmf;*.ptm;*.med;*.ams;*.dbm;*.dsm;*.umx;*.amf;*.psm;*.mt2|"
					"Wave Files (*.wav)|*.wav|"
					"Midi Files (*.mid,*.rmi)|*.mid;*.rmi;*.smf|"
					"All Files (*.*)|*.*||",
					NULL);
	dlg.m_ofn.nFilterIndex = CMainFrame::m_nFilterIndex;
	if (CMainFrame::m_szCurModDir[0])
	{
		dlg.m_ofn.lpstrInitialDir = CMainFrame::m_szCurModDir;
	}
	const size_t bufferSize = 2048; //Note: This is possibly the maximum buffer size in MFC 7(this note was written November 2006).
	vector<char> filenameBuffer(bufferSize, 0);
	dlg.GetOFN().lpstrFile = &filenameBuffer[0];
	dlg.GetOFN().nMaxFile = bufferSize;
    if (dlg.DoModal() == IDOK) 
	{
		POSITION pos = dlg.GetStartPosition();
		while(pos != NULL)
		{
			CHAR szDrive[_MAX_PATH], szDir[_MAX_PATH];
			CString pathName = dlg.GetNextPathName(pos);
			_splitpath(pathName, szDrive, szDir, NULL, NULL);
			strcpy(CMainFrame::m_szCurModDir, szDrive);
			strcat(CMainFrame::m_szCurModDir, szDir);
			CMainFrame::m_nFilterIndex = dlg.m_ofn.nFilterIndex;
			OpenDocumentFile(pathName);
		}
	}
	filenameBuffer.clear();
}


void CTrackApp::RegisterExtensions()
//----------------------------------
{
	HKEY key;
	CHAR s[512] = "";
	CHAR exename[512] = "";

	GetModuleFileName(AfxGetInstanceHandle(), s, sizeof(s));
	GetShortPathName(s, exename, sizeof(exename));
	if (RegCreateKey(HKEY_CLASSES_ROOT,
					"ModPlugPlayer\\shell\\Edit\\command",
					&key) == ERROR_SUCCESS)
	{
		strcpy(s, exename);
		strcat(s, " \"%1\"");
		RegSetValueEx(key, NULL, NULL, REG_SZ, (LPBYTE)s, strlen(s)+1);
		RegCloseKey(key);
	}
	if (RegCreateKey(HKEY_CLASSES_ROOT,
					"ModPlugPlayer\\shell\\Edit\\ddeexec",
					&key) == ERROR_SUCCESS)
	{
		strcpy(s, "[Edit(\"%1\")]");
		RegSetValueEx(key, NULL, NULL, REG_SZ, (LPBYTE)s, strlen(s)+1);
		RegCloseKey(key);
	}
}


void CTrackApp::OnHelpSearch()
//----------------------------
{
	CHAR s[80] = "";
	WinHelp((DWORD)&s, HELP_KEY);
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

	dir = (t/256) % 2; //change dir every 256 t
	t = t%256;
	if (!dir) t = (256-t);
	
	sizex = m_nRotoWidth;
	sizey = m_nRotoHeight;
	m_dwFrameTime = t;
	src = (LPBYTE)&m_lpBmp->bmiColors[256];
	dest = m_lpRotoZoom;
	Dist = t;
	Phi = t;
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
	CHyperEdit m_heContact;

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
	DDX_Control(pDX, IDC_EDIT1,			m_heContact);
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
	CHAR s[256];
	CDialog::OnInitDialog();
	m_bmp.SubclassDlgItem(IDC_BITMAP1, this);
	m_bmp.LoadBitmap(MAKEINTRESOURCE(IDB_MPTRACK));
	wsprintf(s, "Build Date: %s", gszBuildDate);
	SetDlgItemText(IDC_EDIT2, s);
	SetDlgItemText(IDC_EDIT3, CString("Open Modplug Tracker, version ") + MptVersion::str + " (development build)");

	m_heContact.SetWindowText(
"Contact / Discussion:\r\n\
http://modplug.sourceforge.net/forum\r\n\
\r\nUpdates:\r\n\
http://modplug.sourceforge.net/builds/#dev");

	char *pArrCredit = { 
		"OpenMPT / Modplug Tracker|"
		"Copyright © 2004-2007 GPL|"
		"Copyright © 1997-2003 Olivier Lapicque (olivier@modplug.com)|"
		"|"
		"Contributors:|"
		"Robin Fernandes:  robin@soal.org (2004-2007)|"
		"Ahti Leppänen: aaldery@dnainternet.net (2005-2007)|"
		"Sergiy Pylypenko: x.pelya.x@gmail.com (2007)|"
		"Eric Chavanon:  contact@ericus.org (2004-2005)|"
		"Trevor Nunes:  modplug@plastikskouser.com (2004)|"
		"Olivier Lapicque:  olivier@modplug.com (1997-2003)|"
		"|"
		"|"
		"Thanks to:||"
		"Konstanty for the XMMS-Modplug resampling implementation |"
		"http://modplug-xmms.sourceforge.net/|"
		"Stephan M. Bernsee for pitch shifting source code|"
		"http://www.dspdimension.com|"
		"Erik de Castro Lopo for his resampling library|"
		"http://www.mega-nerd.com/SRC/|"
		"Hermann Seib for his example VST Host implementation|"
		"http://www.hermannseib.com/english/vsthost.htm|"
		"Pel K. Txnder for the scrolling credits control :)|"
		"http://tinyurl.com/4yze8||"
		"...and to the following for ideas, testing and support:|"
		"LPChip, Ganja, Diamond, Nofold, Goor00,|"
		"Georg, Skilletaudio, Squirrel Havoc, Snu, Anboi,|"
		"Sam Zen, BooT-SectoR-ViruZ, 33, Waxhead, Jojo,|"
		"all at the MPC forums.|"
		"||||||" 
	};

    m_static.SubclassDlgItem(IDC_CREDITS,this);
    m_static.SetCredits(pArrCredit);
    m_static.SetSpeed(DISPLAY_SLOW);
    m_static.SetColor(BACKGROUND_COLOR, RGB(138, 165, 219)); // Background Colour
    m_static.SetTransparent(); // Set parts of bitmaps with RGB(192,192,192) transparent
    m_static.SetGradient(GRADIENT_LEFT_DARK);  // Background goes from blue to black from left to right
    // m_static.SetBkImage(IDB_BITMAP1); // Background image
    m_static.StartScrolling();
    return TRUE;  // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}


// App command to run the dialog
void CTrackApp::OnAppAbout()
//--------------------------
{
	if (gpAboutDlg) return;
	ShowChangesDialog();
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


void DrawBitmapButton(HDC hdc, LPRECT lpRect, LPMODPLUGDIB lpdib, int srcx, int srcy, BOOL bPushed)
//-------------------------------------------------------------------------------------------------
{
	RECT rect;
	int x = (lpRect->right + lpRect->left) / 2 - 8;
	int y = (lpRect->top + lpRect->bottom) / 2 - 8;
	HGDIOBJ oldpen = SelectObject(hdc, CMainFrame::penBlack);
	rect.left = lpRect->left + 1;
	rect.top = lpRect->top + 1;
	rect.right = lpRect->right - 1;
	rect.bottom = lpRect->bottom - 1;
	if (bPushed)
	{
		::MoveToEx(hdc, lpRect->left, lpRect->bottom-1, NULL);
		::LineTo(hdc, lpRect->left, lpRect->top);
		::LineTo(hdc, lpRect->right-1, lpRect->top);
		::SelectObject(hdc, CMainFrame::penLightGray);
		::LineTo(hdc, lpRect->right-1, lpRect->bottom-1);
		::LineTo(hdc, lpRect->left, lpRect->bottom-1);
		::MoveToEx(hdc, lpRect->left+1, lpRect->bottom-2, NULL);
		::SelectObject(hdc, CMainFrame::penDarkGray);
		::LineTo(hdc, lpRect->left+1, lpRect->top+1);
		::LineTo(hdc, lpRect->right-2, lpRect->top+1);
		rect.left++;
		rect.top++;
		x++;
		y++;
	} else
	{
		::MoveToEx(hdc, lpRect->right-1, lpRect->top, NULL);
		::LineTo(hdc, lpRect->right-1, lpRect->bottom-1);
		::LineTo(hdc, lpRect->left, lpRect->bottom-1);
		::SelectObject(hdc, CMainFrame::penLightGray);
		::LineTo(hdc, lpRect->left, lpRect->top);
		::LineTo(hdc, lpRect->right-1, lpRect->top);
		::SelectObject(hdc, CMainFrame::penDarkGray);
		::MoveToEx(hdc, lpRect->right-2, lpRect->top+1, NULL);
		::LineTo(hdc, lpRect->right-2, lpRect->bottom-2);
		::LineTo(hdc, lpRect->left+1, lpRect->bottom-2);
		rect.right--;
		rect.bottom--;
	}
	::FillRect(hdc, &rect, CMainFrame::brushGray);
	::SelectObject(hdc, oldpen);
	DibBlt(hdc, x, y, 16, 15, srcx, srcy, lpdib);
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
		::DrawText(hdc, lpszText, -1, &rect, dwFlags | DT_SINGLELINE);
		::SelectObject(hdc, oldfont);
	}
}


/////////////////////////////////////////////////////////////////////////////////////
// CButtonEx: button with custom bitmap

BEGIN_MESSAGE_MAP(CButtonEx, CButton)
	//{{AFX_MSG_MAP(CButtonEx)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CButtonEx::Init(const LPMODPLUGDIB pDib, COLORREF colorkey)
//--------------------------------------------------------------
{
	COLORREF btnface = GetSysColor(COLOR_BTNFACE);
	if (!pDib) return FALSE;
	m_Dib = *pDib;
	m_srcRect.left = 0;
	m_srcRect.top = 0;
	m_srcRect.right = 16;
	m_srcRect.bottom = 15;
	for (UINT i=0; i<16; i++)
	{
		COLORREF rgb = RGB(m_Dib.bmiColors[i].rgbRed, m_Dib.bmiColors[i].rgbGreen, m_Dib.bmiColors[i].rgbBlue);
		if (rgb == colorkey)
		{
			m_Dib.bmiColors[i].rgbRed = GetRValue(btnface);
			m_Dib.bmiColors[i].rgbGreen = GetGValue(btnface);
			m_Dib.bmiColors[i].rgbBlue = GetBValue(btnface);
		}
	}
	return TRUE;
}


void CButtonEx::SetPushState(BOOL bPushed)
//----------------------------------------
{
	m_bPushed = bPushed;
}


BOOL CButtonEx::SetSourcePos(int x, int y, int cx, int cy)
//--------------------------------------------------------
{
	m_srcRect.left = x;
	m_srcRect.top = y;
	m_srcRect.right = cx;
	m_srcRect.bottom = cy;
	return TRUE;
}


BOOL CButtonEx::AlignButton(HWND hwndPrev, int dx)
//------------------------------------------------
{
	HWND hwndParent = ::GetParent(m_hWnd);
	if (!hwndParent) return FALSE;
	if (hwndPrev)
	{
		POINT pt;
		RECT rect;
		SIZE sz;
		::GetWindowRect(hwndPrev, &rect);
		pt.x = rect.left;
		pt.y = rect.top;
		sz.cx = rect.right - rect.left;
		sz.cy = rect.bottom - rect.top;
		::ScreenToClient(hwndParent, &pt);
		SetWindowPos(NULL, pt.x + sz.cx + dx, pt.y, 0,0, SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
	}
	return FALSE;
}


BOOL CButtonEx::AlignButton(UINT nIdPrev, int dx)
//-----------------------------------------------
{
	return AlignButton(::GetDlgItem(::GetParent(m_hWnd), nIdPrev), dx);
}


void CButtonEx::DrawItem(LPDRAWITEMSTRUCT lpdis)
//----------------------------------------------
{
	DrawBitmapButton(lpdis->hDC, &lpdis->rcItem, &m_Dib, m_srcRect.left, m_srcRect.top,
						((lpdis->itemState & ODS_SELECTED) || (m_bPushed)) ? TRUE : FALSE);
}


void CButtonEx::OnLButtonDblClk(UINT nFlags, CPoint point)
//--------------------------------------------------------
{
	PostMessage(WM_LBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
}


//////////////////////////////////////////////////////////////////////////////////
// Misc functions


UINT MsgBox(UINT nStringID, CWnd *p, LPCSTR lpszTitle, UINT n)
//------------------------------------------------------------
{
	CString str;
	str.LoadString(nStringID);
	HWND hwnd = (p) ? p->m_hWnd : NULL;
	return ::MessageBox(hwnd, str, lpszTitle, n);
}


void ErrorBox(UINT nStringID, CWnd*p)
//-----------------------------------
{
	MsgBox(nStringID, p, "Error!", MB_OK|MB_ICONSTOP);
}


////////////////////////////////////////////////////////////////////////////////
// CFastBitmap 8-bit output / 4-bit input
// useful for lots of small blits with color mapping 
// combined in one big blit

void CFastBitmap::Init(LPMODPLUGDIB lpTextDib)
//--------------------------------------------
{
	m_nBlendOffset = 0;			// rewbs.buildfix for pattern display bug in debug builds
								// & release builds when ran directly from vs.net 

	m_pTextDib = lpTextDib;
	memset(&m_Dib, 0, sizeof(m_Dib));
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
	m_n4BitPalette[4] = MODSYSCOLOR_LO;
	m_n4BitPalette[12] = MODSYSCOLOR_MED;
	m_n4BitPalette[14] = MODSYSCOLOR_HI;
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
		m_Dib.bmiColors[i|0x80].rgbRed = (m + r)>>1;
		m_Dib.bmiColors[i|0x80].rgbGreen = (m + g)>>1;
		m_Dib.bmiColors[i|0x80].rgbBlue = (m + b)>>1;
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


/////////////////////////////////////////////////////////////////////////////////////
// CMappedFile

CMappedFile::CMappedFile()
//------------------------
{
	m_hFMap = NULL;
	m_lpData = NULL;
}


CMappedFile::~CMappedFile()
//-------------------------
{
}


BOOL CMappedFile::Open(LPCSTR lpszFileName)
//-----------------------------------------
{
	return m_File.Open(lpszFileName, CFile::modeRead|CFile::typeBinary);
}


void CMappedFile::Close()
//-----------------------
{
	if (m_lpData) Unlock();
	m_File.Close();
}


DWORD CMappedFile::GetLength()
//----------------------------
{
	return static_cast<DWORD>(m_File.GetLength());
}


LPBYTE CMappedFile::Lock(DWORD dwMaxLen)
//--------------------------------------
{
	DWORD dwLen = GetLength();
	LPBYTE lpStream;

	if (!dwLen) return NULL;
	if ((dwMaxLen) && (dwLen > dwMaxLen)) dwLen = dwMaxLen;
	HANDLE hmf = CreateFileMapping(
							(HANDLE)m_File.m_hFile,
							NULL,
							PAGE_READONLY,
							0, 0,
							NULL
							);
	if (hmf)
	{
		lpStream = (LPBYTE)MapViewOfFile(
								hmf,
								FILE_MAP_READ,
								0, 0,
								0
							);
		if (lpStream)
		{
			m_hFMap = hmf;
			m_lpData = lpStream;
			return lpStream;
		}
		CloseHandle(hmf);
	}
	if (dwLen > CTrackApp::gMemStatus.dwTotalPhys) return NULL;
	if ((lpStream = (LPBYTE)GlobalAllocPtr(GHND, dwLen)) == NULL) return NULL;
	m_File.Read(lpStream, dwLen);
	m_lpData = lpStream;
	return lpStream;
}


BOOL CMappedFile::Unlock()
//------------------------
{
	if (m_hFMap)
	{
		if (m_lpData)
		{
			UnmapViewOfFile(m_lpData);
			m_lpData = NULL;
		}
		CloseHandle(m_hFMap);
		m_hFMap = NULL;
	}
	if (m_lpData)
	{
		GlobalFreePtr(m_lpData);
		m_lpData = NULL;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////
// MPEG Layer-3 Functions (through ACM)

typedef struct BLADEENCSTREAMINFO
{
	BE_CONFIG beCfg;
	LONGLONG dummy;
	DWORD dwInputSamples;
	DWORD dwOutputSamples;
	HBE_STREAM hBeStream;
	SHORT *pPCMBuffer;
	DWORD dwReadPos;
} BLADEENCSTREAMINFO, *PBLADEENCSTREAMINFO;

static PBLADEENCSTREAMINFO gpbeBladeCfg = NULL;
static PBLADEENCSTREAMINFO gpbeLameCfg = NULL;

#define TRAP_ACM_FAULTS

#ifdef TRAP_ACM_FAULTS
void CTrackApp::AcmExceptionHandler()
{
	theApp.m_hACMInst = NULL;
	theApp.WriteProfileInt("Settings", "DisableACM", 1);
}
#endif

BOOL CTrackApp::InitializeACM(BOOL bNoAcm)
//----------------------------------------
{
	DWORD (ACMAPI *pfnAcmGetVersion)(void);
	DWORD dwVersion;
	UINT fuErrorMode;
	BOOL bOk = FALSE;

	fuErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	try {
		m_hBladeEnc = LoadLibrary(TEXT("BLADEENC.DLL"));
		m_hLameEnc = LoadLibrary(TEXT("LAME_ENC.DLL"));
	} catch(...) {}
	if (!bNoAcm)
	{
	#ifdef TRAP_ACM_FAULTS
		try {
	#endif
			m_hACMInst = LoadLibrary(TEXT("MSACM32.DLL"));
	#ifdef TRAP_ACM_FAULTS
		} catch(...) {}
	#endif
	}
	SetErrorMode(fuErrorMode);
	if (m_hBladeEnc != NULL)
	{
		BEVERSION pfnBeVersion = (BEVERSION)GetProcAddress(m_hBladeEnc, TEXT_BEVERSION);
		if (!pfnBeVersion)
		{
			FreeLibrary(m_hBladeEnc);
			m_hBladeEnc = NULL;
		} else
		{
			m_bLayer3Present = TRUE;
			bOk = TRUE;
		}
	}
	if (m_hLameEnc != NULL)
	{
		BEVERSION pfnBeVersion = (BEVERSION)GetProcAddress(m_hLameEnc, TEXT_BEVERSION);
		if (!pfnBeVersion)
		{
			FreeLibrary(m_hLameEnc);
			m_hLameEnc = NULL;
		} else
		{
			m_bLayer3Present = TRUE;
			bOk = TRUE;
		}
	}
    if ((m_hACMInst < (HINSTANCE)HINSTANCE_ERROR) || (!m_hACMInst))
	{
		m_hACMInst = NULL;
		return bOk;
	}
#ifdef TRAP_ACM_FAULTS
	try {
#endif
		*(FARPROC *)&pfnAcmGetVersion = GetProcAddress(m_hACMInst, "acmGetVersion");
		dwVersion = 0;
		if (pfnAcmGetVersion) dwVersion = pfnAcmGetVersion();
		if (HIWORD(dwVersion) < 0x0200)
		{
			FreeLibrary(m_hACMInst);
			m_hACMInst = NULL;
			return bOk;
		}
		// Load Function Pointers
		m_pfnAcmFormatEnum = (PFNACMFORMATENUM)GetProcAddress(m_hACMInst, "acmFormatEnumA");
		// Enumerate formats
		if (m_pfnAcmFormatEnum)
		{
			ACMFORMATDETAILS afd;
			BYTE wfx[256];
			WAVEFORMATEX *pwfx = (WAVEFORMATEX *)&wfx;

			memset(&afd, 0, sizeof(afd));
			memset(pwfx, 0, sizeof(wfx));
			afd.cbStruct = sizeof(ACMFORMATDETAILS);
			afd.dwFormatTag = WAVE_FORMAT_PCM;
			afd.pwfx = pwfx;
			afd.cbwfx = sizeof(wfx);
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->nChannels = 2;
			pwfx->nSamplesPerSec = 44100;
			pwfx->wBitsPerSample = 16;
			pwfx->nBlockAlign = (WORD)((pwfx->nChannels * pwfx->wBitsPerSample) / 8);
			pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
			m_pfnAcmFormatEnum(NULL, &afd, AcmFormatEnumCB, NULL, ACM_FORMATENUMF_CONVERT);
		}
#ifdef TRAP_ACM_FAULTS
	} catch(...){}
#endif
	return TRUE;
}


BOOL CTrackApp::UninitializeACM()
//-------------------------------
{
	if (m_hACMInst)
	{
		FreeLibrary(m_hACMInst);
		m_hACMInst = NULL;
	}
	if (m_hLameEnc)
	{
		FreeLibrary(m_hLameEnc);
		m_hLameEnc = NULL;
	}
	if (m_hBladeEnc)
	{
		FreeLibrary(m_hBladeEnc);
		m_hBladeEnc = NULL;
	}
	return TRUE;
}


MMRESULT CTrackApp::AcmFormatEnum(HACMDRIVER had, LPACMFORMATDETAILSA pafd, ACMFORMATENUMCBA fnCallback, DWORD dwInstance, DWORD fdwEnum)
//---------------------------------------------------------------------------------------------------------------------------------------
{
	MMRESULT err = MMSYSERR_INVALPARAM;
	if ((m_hBladeEnc) || (m_hLameEnc))
	{
		HACMDRIVER hBlade = (HACMDRIVER)&m_hBladeEnc;
		HACMDRIVER hLame = (HACMDRIVER)&m_hLameEnc;
		if (((had == hBlade) || (had == hLame) || (had == NULL)) && (pafd) && (fnCallback)
		 && (fdwEnum & ACM_FORMATENUMF_CONVERT) && (pafd->dwFormatTag == WAVE_FORMAT_PCM))
		{
			ACMFORMATDETAILS afd;
			MPEGLAYER3WAVEFORMAT wfx;

			afd.dwFormatIndex = 0;
			for (UINT iFmt=0; iFmt<0x40; iFmt++)
			{
				afd.cbStruct = sizeof(afd);
				afd.dwFormatTag = WAVE_FORMAT_MPEGLAYER3;
				afd.fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
				afd.pwfx = (LPWAVEFORMATEX)&wfx;
				afd.cbwfx = sizeof(wfx);
				wfx.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
				wfx.wfx.nChannels = (WORD)((iFmt & 0x20) ? 1 : 2);
				wfx.wfx.nSamplesPerSec = 0;
				wfx.wfx.nBlockAlign = 1;
				wfx.wfx.wBitsPerSample = 0;
				wfx.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
				wfx.wID = MPEGLAYER3_ID_MPEG;
				wfx.fdwFlags = MPEGLAYER3_FLAG_PADDING_ISO;
				wfx.nBlockSize = 384;
				wfx.nFramesPerBlock = 1;
				wfx.nCodecDelay = 1000;
				switch((iFmt >> 3) & 0x03)
				{
				case 0x00: wfx.wfx.nSamplesPerSec = 48000; break;
				case 0x01: wfx.wfx.nSamplesPerSec = 44100; break;
				case 0x02: wfx.wfx.nSamplesPerSec = 32000; break;
				}
				switch(iFmt & 7)
				{
				case 4:	wfx.wfx.nAvgBytesPerSec = 64/8; break;
				case 3:	wfx.wfx.nAvgBytesPerSec = 96/8; break;
				case 2:	wfx.wfx.nAvgBytesPerSec = 128/8; break;
				case 1:	if (wfx.wfx.nChannels == 2) { wfx.wfx.nAvgBytesPerSec = 192/8; break; }
				case 0:	if (wfx.wfx.nChannels == 2) { wfx.wfx.nAvgBytesPerSec = 256/8; break; }
				default: wfx.wfx.nSamplesPerSec = 0;
				}
				wsprintf(afd.szFormat, "%dkbps, %dHz, %s", wfx.wfx.nAvgBytesPerSec*8, wfx.wfx.nSamplesPerSec, (wfx.wfx.nChannels == 2) ? "stereo" : "mono");
				if (wfx.wfx.nSamplesPerSec)
				{
					if (m_hBladeEnc) fnCallback((HACMDRIVERID)hBlade, &afd, dwInstance, ACMDRIVERDETAILS_SUPPORTF_CODEC);
					if (m_hLameEnc) fnCallback((HACMDRIVERID)hLame, &afd, dwInstance, ACMDRIVERDETAILS_SUPPORTF_CODEC);
					afd.dwFormatIndex++;
				}
			}
			err = MMSYSERR_NOERROR;
		}
	}
	if (m_pfnAcmFormatEnum)
	{
		err = m_pfnAcmFormatEnum(had, pafd, fnCallback, dwInstance, fdwEnum);
	}
	return err;
}


BOOL CTrackApp::AcmFormatEnumCB(HACMDRIVERID, LPACMFORMATDETAILS pafd, DWORD, DWORD fdwSupport)
//---------------------------------------------------------------------------------------------
{
	if ((pafd) && (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC))
	{
		if (pafd->dwFormatTag == WAVE_FORMAT_MPEGLAYER3) theApp.m_bLayer3Present = TRUE;
	}
	return TRUE;
}


MMRESULT CTrackApp::AcmDriverOpen(LPHACMDRIVER phad, HACMDRIVERID hadid, DWORD fdwOpen)
//-------------------------------------------------------------------------------------
{
	if ((m_hBladeEnc) && (hadid == (HACMDRIVERID)&m_hBladeEnc))
	{
		*phad = (HACMDRIVER)&m_hBladeEnc;
		return MMSYSERR_NOERROR;
	}
	if ((m_hLameEnc) && (hadid == (HACMDRIVERID)&m_hLameEnc))
	{
		*phad = (HACMDRIVER)&m_hLameEnc;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMDRIVEROPEN pfnAcmDriverOpen = (PFNACMDRIVEROPEN)GetProcAddress(m_hACMInst, "acmDriverOpen");
		if (pfnAcmDriverOpen) return pfnAcmDriverOpen(phad, hadid, fdwOpen);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmDriverClose(HACMDRIVER had, DWORD fdwClose)
//----------------------------------------------------------------
{
	if ((m_hBladeEnc) && (had == (HACMDRIVER)&m_hBladeEnc))
	{
		return MMSYSERR_NOERROR;
	}
	if ((m_hLameEnc) && (had == (HACMDRIVER)&m_hLameEnc))
	{
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMDRIVERCLOSE pfnAcmDriverClose = (PFNACMDRIVERCLOSE)GetProcAddress(m_hACMInst, "acmDriverClose");
		if (pfnAcmDriverClose) return pfnAcmDriverClose(had, fdwClose);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmDriverDetails(HACMDRIVERID hadid, LPACMDRIVERDETAILS padd, DWORD fdwDetails)
//-------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (hadid == (HACMDRIVERID)(&m_hBladeEnc)))
	 || ((m_hLameEnc) && (hadid == (HACMDRIVERID)(&m_hLameEnc))))
	{
		if (!padd) return MMSYSERR_INVALPARAM;
		padd->cbStruct = sizeof(ACMDRIVERDETAILS);
		padd->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
		padd->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
		padd->wMid = 0;
		padd->wPid = 0;
		padd->vdwACM = 0x04020000;
		padd->vdwDriver = 0x04020000;
		padd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
		padd->cFormatTags = 1;
		padd->cFilterTags = 0;
		padd->hicon = NULL;
		strcpy(padd->szShortName, (hadid == (HACMDRIVERID)(&m_hBladeEnc)) ? "BladeEnc MP3" : "LAME Encoder");
		strcpy(padd->szLongName, padd->szShortName);
		padd->szCopyright[0] = 0;
		padd->szLicensing[0] = 0;
		padd->szFeatures[0] = 0;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMDRIVERDETAILS pfnAcmDriverDetails = (PFNACMDRIVERDETAILS)GetProcAddress(m_hACMInst, "acmDriverDetailsA");
		if (pfnAcmDriverDetails) return pfnAcmDriverDetails(hadid, padd, fdwDetails);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmStreamOpen(
	LPHACMSTREAM            phas,       // pointer to stream handle
    HACMDRIVER              had,        // optional driver handle
    LPWAVEFORMATEX          pwfxSrc,    // source format to convert
    LPWAVEFORMATEX          pwfxDst,    // required destination format
    LPWAVEFILTER            pwfltr,     // optional filter
    DWORD                   dwCallback, // callback
    DWORD                   dwInstance, // callback instance data
    DWORD                   fdwOpen)    // ACM_STREAMOPENF_* and CALLBACK_*
//-------------------------------------------------------------------------
{
	PBLADEENCSTREAMINFO *ppbeCfg = NULL;
	HINSTANCE hLib = NULL;

	if ((m_hBladeEnc) && (had == (HACMDRIVER)&m_hBladeEnc))
	{
		ppbeCfg = &gpbeBladeCfg;
		hLib = m_hBladeEnc;
	}
	if ((m_hLameEnc) && (had == (HACMDRIVER)&m_hLameEnc))
	{
		ppbeCfg = &gpbeLameCfg;
		hLib = m_hLameEnc;
	}
	if ((ppbeCfg) && (hLib))
	{
		BEINITSTREAM pfnbeInitStream = (BEINITSTREAM)GetProcAddress(hLib, TEXT_BEINITSTREAM);
		if (!pfnbeInitStream) return MMSYSERR_INVALPARAM;
		PBLADEENCSTREAMINFO pbeCfg = new BLADEENCSTREAMINFO;
		if ((pbeCfg) && (pwfxSrc) && (pwfxDst) && (pwfxSrc->nChannels == pwfxDst->nChannels)
		 && (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM) && (pwfxDst->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
		 && (pwfxSrc->wBitsPerSample == 16))
		{
			pbeCfg->dwInputSamples = 2048;
			pbeCfg->dwOutputSamples = 2048;
			pbeCfg->beCfg.dwConfig = BE_CONFIG_MP3;
			pbeCfg->beCfg.format.mp3.dwSampleRate = pwfxDst->nSamplesPerSec; // 48000, 44100 and 32000 allowed
			pbeCfg->beCfg.format.mp3.byMode = (BYTE)((pwfxSrc->nChannels == 2) ? BE_MP3_MODE_STEREO : BE_MP3_MODE_MONO);
			pbeCfg->beCfg.format.mp3.wBitrate = (WORD)(pwfxDst->nAvgBytesPerSec * 8);	// 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256 and 320 allowed
			pbeCfg->beCfg.format.mp3.bPrivate = FALSE;
			pbeCfg->beCfg.format.mp3.bCRC = FALSE;
			pbeCfg->beCfg.format.mp3.bCopyright = FALSE;
			pbeCfg->beCfg.format.mp3.bOriginal = TRUE;
			pbeCfg->hBeStream = NULL;
			if (pfnbeInitStream(&pbeCfg->beCfg, &pbeCfg->dwInputSamples, &pbeCfg->dwOutputSamples, &pbeCfg->hBeStream) == BE_ERR_SUCCESSFUL)
			{
				*phas = (HACMSTREAM)had;
				*ppbeCfg = pbeCfg;
				pbeCfg->pPCMBuffer = (SHORT *)GlobalAllocPtr(GHND, pbeCfg->dwInputSamples * sizeof(SHORT));
				pbeCfg->dwReadPos = 0;
				return MMSYSERR_NOERROR;
			}
		}
		if (pbeCfg) delete pbeCfg;
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMOPEN pfnAcmStreamOpen = (PFNACMSTREAMOPEN)GetProcAddress(m_hACMInst, "acmStreamOpen");
		if (pfnAcmStreamOpen) return pfnAcmStreamOpen(phas, had, pwfxSrc, pwfxDst, pwfltr, dwCallback, dwInstance, fdwOpen);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmStreamClose(HACMSTREAM has, DWORD fdwClose)
//----------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
	 || ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		HINSTANCE hLib = *(HINSTANCE *)has;
		PBLADEENCSTREAMINFO pbeCfg = (has == (HACMSTREAM)&m_hBladeEnc) ? gpbeBladeCfg : gpbeLameCfg;
		BECLOSESTREAM pfnbeCloseStream = (BECLOSESTREAM)GetProcAddress(hLib, TEXT_BECLOSESTREAM);
		if ((pbeCfg) && (pfnbeCloseStream))
		{
			pfnbeCloseStream(pbeCfg->hBeStream);
			if (pbeCfg->pPCMBuffer)
			{
				GlobalFreePtr(pbeCfg->pPCMBuffer);
				pbeCfg->pPCMBuffer = NULL;
			}
			delete pbeCfg;
			pbeCfg = NULL;
			return MMSYSERR_NOERROR;
		}
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCLOSE pfnAcmStreamClose = (PFNACMSTREAMCLOSE)GetProcAddress(m_hACMInst, "acmStreamClose");
		if (pfnAcmStreamClose) return pfnAcmStreamClose(has, fdwClose);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmStreamSize(HACMSTREAM has, DWORD cbInput, LPDWORD pdwOutputBytes, DWORD fdwSize)
//-----------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
	 || ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		PBLADEENCSTREAMINFO pbeCfg = (has == (HACMSTREAM)&m_hBladeEnc) ? gpbeBladeCfg : gpbeLameCfg;

		if ((pbeCfg) && (pdwOutputBytes))
		{
			if (fdwSize & ACM_STREAMSIZEF_DESTINATION)
			{
				*pdwOutputBytes = pbeCfg->dwInputSamples * sizeof(SHORT);
			} else
			if (fdwSize & ACM_STREAMSIZEF_SOURCE)
			{
				*pdwOutputBytes = pbeCfg->dwOutputSamples;
			}
			return MMSYSERR_NOERROR;
		}
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMSIZE pfnAcmStreamSize = (PFNACMSTREAMSIZE)GetProcAddress(m_hACMInst, "acmStreamSize");
		if (pfnAcmStreamSize) return pfnAcmStreamSize(has, cbInput, pdwOutputBytes, fdwSize);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmStreamPrepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare)
//--------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
	 || ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		pash->fdwStatus = ACMSTREAMHEADER_STATUSF_PREPARED;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCONVERT pfnAcmStreamPrepareHeader = (PFNACMSTREAMCONVERT)GetProcAddress(m_hACMInst, "acmStreamPrepareHeader");
		if (pfnAcmStreamPrepareHeader) return pfnAcmStreamPrepareHeader(has, pash, fdwPrepare);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmStreamUnprepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwUnprepare)
//------------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
	 || ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		pash->fdwStatus &= ~ACMSTREAMHEADER_STATUSF_PREPARED;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCONVERT pfnAcmStreamUnprepareHeader = (PFNACMSTREAMCONVERT)GetProcAddress(m_hACMInst, "acmStreamUnprepareHeader");
		if (pfnAcmStreamUnprepareHeader) return pfnAcmStreamUnprepareHeader(has, pash, fdwUnprepare);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT CTrackApp::AcmStreamConvert(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert)
//--------------------------------------------------------------------------------------------
{
	static BEENCODECHUNK pfnbeEncodeChunk = NULL;
	static BEDEINITSTREAM pfnbeDeinitStream = NULL;
	static HINSTANCE hInstEncode = NULL;

	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
	 || ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		PBLADEENCSTREAMINFO pbeCfg = (has == (HACMSTREAM)&m_hBladeEnc) ? gpbeBladeCfg : gpbeLameCfg;
		HINSTANCE hLib = *(HINSTANCE *)has;

		if (hInstEncode != hLib)
		{
			pfnbeEncodeChunk = (BEENCODECHUNK)GetProcAddress(hLib, TEXT_BEENCODECHUNK);
			pfnbeDeinitStream = (BEDEINITSTREAM)GetProcAddress(hLib, TEXT_BEDEINITSTREAM);
			hInstEncode = hLib;
		}
		pash->fdwStatus |= ACMSTREAMHEADER_STATUSF_DONE;
		if (fdwConvert & ACM_STREAMCONVERTF_END)
		{
			if ((pfnbeDeinitStream) && (pbeCfg))
			{
				pfnbeDeinitStream(pbeCfg->hBeStream, pash->pbDst, &pash->cbDstLengthUsed);
				return MMSYSERR_NOERROR;
			}
		} else
		{
			if ((pfnbeEncodeChunk) && (pbeCfg))
			{
				DWORD dwBytesLeft = pbeCfg->dwInputSamples*2 - pbeCfg->dwReadPos;
				if (!dwBytesLeft)
				{
					dwBytesLeft = pbeCfg->dwInputSamples*2;
					pbeCfg->dwReadPos = 0;
				}
				if (dwBytesLeft > pash->cbSrcLength) dwBytesLeft = pash->cbSrcLength;
				memcpy(&pbeCfg->pPCMBuffer[pbeCfg->dwReadPos/2], pash->pbSrc, dwBytesLeft);
				pbeCfg->dwReadPos += dwBytesLeft;
				pash->cbSrcLengthUsed = dwBytesLeft;
				pash->cbDstLengthUsed = 0;
				if (pbeCfg->dwReadPos == pbeCfg->dwInputSamples*2)
				{
					if (pfnbeEncodeChunk(pbeCfg->hBeStream, pbeCfg->dwInputSamples, pbeCfg->pPCMBuffer, pash->pbDst, &pash->cbDstLengthUsed) != 0) return MMSYSERR_INVALPARAM;
					pbeCfg->dwReadPos = 0;
				}
				return MMSYSERR_NOERROR;
			}
		}
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCONVERT pfnAcmStreamConvert = (PFNACMSTREAMCONVERT)GetProcAddress(m_hACMInst, "acmStreamConvert");
		if (pfnAcmStreamConvert) return pfnAcmStreamConvert(has, pash, fdwConvert);
	}
	return MMSYSERR_INVALPARAM;
}


///////////////////////////////////////////////////////////////////////////////////
//
// DirectX Plugins
//

BOOL CTrackApp::InitializeDXPlugins()
//-----------------------------------
{
	CHAR s[_MAX_PATH], tmp[32];
	LONG nPlugins;

	m_pPluginManager = new CVstPluginManager;
	if (!m_pPluginManager) return FALSE;
	nPlugins = GetPrivateProfileInt("VST Plugins", "NumPlugins", 0, m_szConfigFileName);
	CString nonFoundPlugs;
	for (LONG iPlug=0; iPlug<nPlugins; iPlug++)
	{
		s[0] = 0;
		wsprintf(tmp, "Plugin%d", iPlug);
		GetPrivateProfileString("VST Plugins", tmp, "", s, sizeof(s), m_szConfigFileName);
		if (s[0]) m_pPluginManager->AddPlugin(s, TRUE, true, &nonFoundPlugs);
	}
	if(nonFoundPlugs.GetLength() > 0)
	{
		nonFoundPlugs.Insert(0, "Problems were encountered with plugins:\n");
		MessageBox(NULL, nonFoundPlugs, "", MB_OK);
	}
	return FALSE;
}


BOOL CTrackApp::UninitializeDXPlugins()
//-------------------------------------
{
	CHAR s[_MAX_PATH], tmp[32];
	PVSTPLUGINLIB pPlug;
	UINT iPlug;

	if (!m_pPluginManager) return FALSE;
	pPlug = m_pPluginManager->GetFirstPlugin();
	iPlug = 0;
	while (pPlug)
	{
		if (pPlug->dwPluginId1 != kDmoMagic)
		{
			s[0] = 0;
			wsprintf(tmp, "Plugin%d", iPlug);
			strcpy(s, pPlug->szDllPath);
			WritePrivateProfileString("VST Plugins", tmp, s, m_szConfigFileName);
			iPlug++;
		}
		pPlug = pPlug->pNext;
	}
	wsprintf(s, "%d", iPlug);
	WritePrivateProfileString("VST Plugins", "NumPlugins", s, m_szConfigFileName);
	if (m_pPluginManager)
	{
		delete m_pPluginManager;
		m_pPluginManager = NULL;
	}
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////
// Internet-related functions

BOOL CTrackApp::OpenURL(LPCSTR lpszURL)
//-------------------------------------
{
	if ((lpszURL) && (lpszURL[0]) && (theApp.m_pMainWnd))
	{
		if (((ULONG)ShellExecute(
					theApp.m_pMainWnd->m_hWnd,
					"open",
					lpszURL,
					NULL,
					NULL,
					0)) >= 32) return TRUE;				
	}
	return FALSE;
}


///////////////////////////////////////////////////////////////////////////////////
// Debug

void Log(LPCSTR format,...)
//-------------------------
{
	#ifdef _DEBUG
		CHAR cBuf[1024];
		va_list va;
		va_start(va, format);
		wvsprintf(cBuf, format, va);
		OutputDebugString(cBuf);
		#ifdef LOG_TO_FILE
			FILE *f = fopen("c:\\mptrack.log", "a");
			if (f)
			{
				fwrite(cBuf, 1, strlen(cBuf), f);
				fclose(f);
			}
		#endif //LOG_TO_FILE
		va_end(va);
	#endif //_DEBUG
}


//////////////////////////////////////////////////////////////////////////////////
// Localized strings

VOID CTrackApp::ImportLocalizedStrings()
//--------------------------------------
{
	//DWORD dwLangId = ((DWORD)GetUserDefaultLangID()) & 0xfff;
	// TODO: look up [Strings.lcid], [Strings.(lcid&0xff)] & [Strings] in mpt_intl.ini
}


BOOL CTrackApp::GetLocalizedString(LPCSTR pszName, LPSTR pszStr, UINT cbSize)
//---------------------------------------------------------------------------
{
	CHAR s[32];
	DWORD dwLangId = ((DWORD)GetUserDefaultLangID()) & 0xffff;

	pszStr[0] = 0;
	if (!m_szStringsFileName[0]) return FALSE;
	wsprintf(s, "Strings.%04X", dwLangId);
	GetPrivateProfileString(s, pszName, "", pszStr, cbSize, m_szStringsFileName);
	if (pszStr[0]) return TRUE;
	wsprintf(s, "Strings.%04X", dwLangId&0xff);
	GetPrivateProfileString(s, pszName, "", pszStr, cbSize, m_szStringsFileName);
	if (pszStr[0]) return TRUE;
	wsprintf(s, "Strings", dwLangId&0xff);
	GetPrivateProfileString(s, pszName, "", pszStr, cbSize, m_szStringsFileName);
	if (pszStr[0]) return TRUE;
	return FALSE;
}

//rewbs.crashHandler
LRESULT CTrackApp::ProcessWndProcException(CException* e, const MSG* pMsg)
//-----------------------------------------------------------------------
{
	// TODO: Add your specialized code here and/or call the base class
	Log("Unhandled Exception\n");
	Log("Attempting to close sound device\n");
	
	if (CMainFrame::gpSoundDevice) {
		CMainFrame::gpSoundDevice->Reset(); 
		CMainFrame::gpSoundDevice->Close();
	}

	return CWinApp::ProcessWndProcException(e, pMsg);
}
//end rewbs.crashHandler
