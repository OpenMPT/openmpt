/*
 * MainFrm.cpp
 * -----------
 * Purpose: Implementation of OpenMPT's main window code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "MainFrm.h"
#include "snddev.h"
#include "moddoc.h"
#include "childfrm.h"
#include "dlsbank.h"
#include "mpdlgs.h"
#include "moptions.h"
#include "vstplug.h"
#include "KeyConfigDlg.h"
#include "AutoSaver.h"
#include "MainFrm.h"
// -> CODE#0015
// -> DESC="channels management dlg"
#include "globals.h"
#include "ChannelManagerDlg.h"
#include "MIDIMappingDialog.h"
// -! NEW_FEATURE#0015
#include <direct.h>
#include "../common/version.h"
#include "ctrl_pat.h"
#include "UpdateCheck.h"
#include "CloseMainDialog.h"
#include "SelectPluginDialog.h"
#include "ExceptionHandler.h"
#include "PatternClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MPTTIMER_PERIOD		200


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_VIEW_OPTIONS,				OnViewOptions)

	ON_COMMAND(ID_PLUGIN_SETUP,				OnPluginManager)
	ON_COMMAND(ID_CHANNEL_MANAGER,			OnChannelManager)
	ON_COMMAND(ID_CLIPBOARD_MANAGER,		OnClipboardManager)
	ON_COMMAND(ID_VIEW_MIDIMAPPING,			OnViewMIDIMapping)
	//ON_COMMAND(ID_HELP,					CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_VIEW_SONGPROPERTIES,		OnSongProperties)
	ON_COMMAND(ID_REPORT_BUG,				OnReportBug)	//rewbs.reportBug
	ON_COMMAND(ID_NEXTOCTAVE,				OnNextOctave)
	ON_COMMAND(ID_PREVOCTAVE,				OnPrevOctave)
	ON_COMMAND_RANGE(ID_FILE_OPENTEMPLATE, ID_FILE_OPENTEMPLATE_LASTINRANGE, OnOpenTemplateModule)
	ON_COMMAND(ID_ADD_SOUNDBANK,			OnAddDlsBank)
	ON_COMMAND(ID_IMPORT_MIDILIB,			OnImportMidiLib)
	ON_COMMAND(ID_MIDI_RECORD,				OnMidiRecord)
	ON_COMMAND(ID_PANIC,					OnPanic)
	ON_COMMAND(ID_PLAYER_PAUSE,				OnPlayerPause)
	ON_COMMAND_RANGE(ID_EXAMPLE_MODULES, ID_EXAMPLE_MODULES_LASTINRANGE, OnExampleSong)
	ON_COMMAND_EX(IDD_TREEVIEW,				OnBarCheck)
	ON_COMMAND_EX(ID_NETLINK_MODPLUG,		OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_TOP_PICKS,		OnInternetLink)
	ON_CBN_SELCHANGE(IDC_COMBO_BASEOCTAVE,	OnOctaveChanged)
	ON_UPDATE_COMMAND_UI(ID_MIDI_RECORD,	OnUpdateMidiRecord)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TIME,	OnUpdateTime)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_USER,	OnUpdateUser)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO,	OnUpdateInfo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_XINFO,OnUpdateXInfo) //rewbs.xinfo
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CPU,  OnUpdateCPU)
	ON_UPDATE_COMMAND_UI(IDD_TREEVIEW,		OnUpdateControlBarMenu)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		OnUpdatePosition)
	ON_MESSAGE(WM_MOD_UPDATEPOSITIONTHREADED,		OnUpdatePositionThreaded)
	ON_MESSAGE(WM_MOD_INVALIDATEPATTERNS,	OnInvalidatePatterns)
	ON_MESSAGE(WM_MOD_SPECIALKEY,			OnSpecialKey)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,			OnCustomKeyMsg) //rewbs.customKeys
	ON_COMMAND(ID_INTERNETUPDATE,			OnInternetUpdate)
	ON_COMMAND(ID_HELP_SHOWSETTINGSFOLDER,	OnShowSettingsFolder)
	ON_COMMAND(ID_HELPSHOW,					OnHelp)
	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_KILLFOCUS() //rewbs.fix3116
	ON_WM_MOUSEWHEEL()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// Globals
UINT CMainFrame::m_nLastOptionsPage = 0;
HHOOK CMainFrame::ghKbdHook = NULL;

std::vector<CString> CMainFrame::s_ExampleModulePaths;
std::vector<CString> CMainFrame::s_TemplateModulePaths;

LONG CMainFrame::gnLVuMeter = 0;
LONG CMainFrame::gnRVuMeter = 0;

// GDI
HICON CMainFrame::m_hIcon = NULL;
HFONT CMainFrame::m_hGUIFont = NULL;
HFONT CMainFrame::m_hFixedFont = NULL;
HFONT CMainFrame::m_hLargeFixedFont = NULL;
HPEN CMainFrame::penDarkGray = NULL;
HPEN CMainFrame::penScratch = NULL; //rewbs.fxVis
HPEN CMainFrame::penGray00 = NULL;
HPEN CMainFrame::penGray33 = NULL;
HPEN CMainFrame::penGray40 = NULL;
HPEN CMainFrame::penGray55 = NULL;
HPEN CMainFrame::penGray80 = NULL;
HPEN CMainFrame::penGray99 = NULL;
HPEN CMainFrame::penGraycc = NULL;
HPEN CMainFrame::penGrayff = NULL; //end rewbs.fxVis
HPEN CMainFrame::penLightGray = NULL;
HPEN CMainFrame::penBlack = NULL;
HPEN CMainFrame::penWhite = NULL;
HPEN CMainFrame::penHalfDarkGray = NULL;
HPEN CMainFrame::penSample = NULL;
HPEN CMainFrame::penEnvelope = NULL;
HPEN CMainFrame::penEnvelopeHighlight = NULL;
HPEN CMainFrame::penSeparator = NULL;
HBRUSH CMainFrame::brushGray = NULL;
HBRUSH CMainFrame::brushBlack = NULL;
HBRUSH CMainFrame::brushWhite = NULL;
HBRUSH CMainFrame::brushText = NULL;
//CBrush *CMainFrame::pbrushBlack = NULL;//rewbs.envRowGrid
//CBrush *CMainFrame::pbrushWhite = NULL;//rewbs.envRowGrid

HBRUSH CMainFrame::brushHighLight = NULL;
HBRUSH CMainFrame::brushHighLightRed = NULL;
HBRUSH CMainFrame::brushWindow = NULL;
HBRUSH CMainFrame::brushYellow = NULL;
HCURSOR CMainFrame::curDragging = NULL;
HCURSOR CMainFrame::curArrow = NULL;
HCURSOR CMainFrame::curNoDrop = NULL;
HCURSOR CMainFrame::curNoDrop2 = NULL;
HCURSOR CMainFrame::curVSplit = NULL;
LPMODPLUGDIB CMainFrame::bmpPatterns = NULL;
LPMODPLUGDIB CMainFrame::bmpNotes = NULL;
LPMODPLUGDIB CMainFrame::bmpVUMeters = NULL;
LPMODPLUGDIB CMainFrame::bmpVisNode = NULL;
LPMODPLUGDIB CMainFrame::bmpVisPcNode = NULL;
HPEN CMainFrame::gpenVuMeter[NUM_VUMETER_PENS*2];

CInputHandler *CMainFrame::m_InputHandler = nullptr; //rewbs.customKeys
CAutoSaver *CMainFrame::m_pAutoSaver = nullptr; //rewbs.autosave
//CPerformanceCounter *CMainFrame::m_pPerfCounter = nullptr;

static UINT indicators[] =
{
	ID_SEPARATOR,			// status line indicator
	ID_INDICATOR_XINFO,		//rewbs.xinfo
	ID_INDICATOR_INFO,
	ID_INDICATOR_USER,
	ID_INDICATOR_TIME,
	ID_INDICATOR_CPU
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction
//#include <direct.h>
CMainFrame::CMainFrame()
//----------------------
{

	m_hNotifyThread = NULL;
	m_dwNotifyThreadId = 0;
	m_hNotifyWakeUp = NULL;
	gpSoundDevice = NULL;
	m_IsPlaybackRunning = false;

	m_bModTreeHasFocus = false;	//rewbs.customKeys
	m_pNoteMapHasFocus = nullptr;	//rewbs.customKeys
	m_pOrderlistHasFocus = nullptr;
	m_bOptionsLocked = false;	//rewbs.customKeys

	m_SoundCardOptionsDialog = nullptr;

	m_pJustModifiedDoc = nullptr;
	m_hFollowSong = NULL;
	m_hWndMidi = NULL;
	m_pSndFile = nullptr;
	m_dwTimeSec = 0;
	m_dwNotifyType = 0;
	m_nTimer = 0;
	m_nAvgMixChn = m_nMixChn = 0;
	m_szUserText[0] = 0;
	m_szInfoText[0] = 0;
	m_szXInfoText[0]= 0;	//rewbs.xinfo

	m_TotalSamplesRendered = 0;
	m_PendingNotificationSempahore = NULL;

	MemsetZero(gpenVuMeter);

	// Create Audio Critical Section
	MemsetZero(g_csAudio);
	InitializeCriticalSection(&g_csAudio);

	TrackerSettings::Instance().LoadSettings();

	m_InputHandler = new CInputHandler(this); 	//rewbs.customKeys
	//m_pPerfCounter= new CPerformanceCounter();

	//Loading static tunings here - probably not the best place to do that but anyway.
	CSoundFile::LoadStaticTunings();
}


VOID CMainFrame::Initialize()
//---------------------------
{
	//Adding version number to the frame title
	CString title = GetTitle();
	title += CString(" ") + MptVersion::str;
	#ifdef DEBUG
		title += CString(" DEBUG");
	#endif
	#ifdef NO_VST
		title += " NO_VST";
	#endif
	#ifdef NO_ASIO
		title += " NO_ASIO";
	#endif
	#ifdef NO_DSOUND
		title += " NO_DSOUND";
	#endif
	SetTitle(title);
	OnUpdateFrameTitle(false);

	CSoundFile::gpSndMixHook = CalcStereoVuMeters;

	// Check for valid sound device
	if (!EnumerateSoundDevices(SNDDEV_GET_TYPE(TrackerSettings::Instance().m_nWaveDevice), SNDDEV_GET_NUMBER(TrackerSettings::Instance().m_nWaveDevice), nullptr, 0))
	{
		TrackerSettings::Instance().m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_DSOUND);
		if (!EnumerateSoundDevices(SNDDEV_GET_TYPE(TrackerSettings::Instance().m_nWaveDevice), SNDDEV_GET_NUMBER(TrackerSettings::Instance().m_nWaveDevice), nullptr, 0))
		{
			TrackerSettings::Instance().m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT);
		}
	}

	// Create Notify Thread
	m_PendingNotificationSempahore = CreateSemaphore(NULL, 0, 1, NULL);
	m_hNotifyWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hNotifyThread = CreateThread(NULL, 0, NotifyThreadWrapper, NULL, 0, &m_dwNotifyThreadId);
	// Setup timer
	OnUpdateUser(NULL);
	m_nTimer = SetTimer(1, MPTTIMER_PERIOD, NULL);

//rewbs: reduce to normal priority during debug for easier hang debugging
	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	// Setup Keyboard Hook
	ghKbdHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, AfxGetInstanceHandle(), GetCurrentThreadId());
	// Initialize Audio Mixer
	UpdateAudioParameters(TRUE);
	// Update the tree
	m_wndTree.Init();

	CreateExampleModulesMenu();
	CreateTemplateModulesMenu();
}


CMainFrame::~CMainFrame()
//-----------------------
{
	DeleteCriticalSection(&g_csAudio);
	delete m_InputHandler; 	//rewbs.customKeys
	delete m_pAutoSaver; //rewbs.autosaver
	//delete m_pPerfCounter;

	CChannelManagerDlg::DestroySharedInstance();
	CSoundFile::DeleteStaticdata();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
//-----------------------------------------------------
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1) return -1;
	// Load resources
	m_hIcon = theApp.LoadIcon(IDR_MAINFRAME);
	m_ImageList.Create(IDB_IMAGELIST, 16, 0, RGB(0,128,128));
	m_hGUIFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	m_hFixedFont = ::CreateFont(12,5, 0,0, 300,
							FALSE, FALSE, FALSE,
							OEM_CHARSET, OUT_RASTER_PRECIS,
							CLIP_DEFAULT_PRECIS, DRAFT_QUALITY,
							FIXED_PITCH | FF_MODERN, "");
	m_hLargeFixedFont = ::CreateFont(18,8, 0,0, 400,
							FALSE, FALSE, FALSE,
							OEM_CHARSET, OUT_RASTER_PRECIS,
							CLIP_DEFAULT_PRECIS, DRAFT_QUALITY,
							FIXED_PITCH | FF_MODERN, "");
	if (m_hGUIFont == NULL) m_hGUIFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	brushBlack = (HBRUSH)::GetStockObject(BLACK_BRUSH);
	brushWhite = (HBRUSH)::GetStockObject(WHITE_BRUSH);
	brushText = ::CreateSolidBrush(GetSysColor(COLOR_BTNTEXT));
	brushGray = ::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	penLightGray = ::CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNHIGHLIGHT));
	penDarkGray = ::CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
	penScratch = ::CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
	penGray00 = ::CreatePen(PS_SOLID,0, RGB(0x00, 0x00, 0x00));
	penGray33 = ::CreatePen(PS_SOLID,0, RGB(0x33, 0x33, 0x33));
	penGray40 = ::CreatePen(PS_SOLID,0, RGB(0x40, 0x40, 0x40));
	penGray55 = ::CreatePen(PS_SOLID,0, RGB(0x55, 0x55, 0x55));
	penGray80 = ::CreatePen(PS_SOLID,0, RGB(0x80, 0x80, 0x80));
	penGray99 = ::CreatePen(PS_SOLID,0, RGB(0x99, 0x99, 0x99));
	penGraycc = ::CreatePen(PS_SOLID,0, RGB(0xcc, 0xcc, 0xcc));
	penGrayff = ::CreatePen(PS_SOLID,0, RGB(0xff, 0xff, 0xff));

	penHalfDarkGray = ::CreatePen(PS_DOT, 0, GetSysColor(COLOR_BTNSHADOW));
	penBlack = (HPEN)::GetStockObject(BLACK_PEN);
	penWhite = (HPEN)::GetStockObject(WHITE_PEN);



	// Cursors
	curDragging = theApp.LoadCursor(IDC_DRAGGING);
	curArrow = theApp.LoadStandardCursor(IDC_ARROW);
	curNoDrop = theApp.LoadCursor(IDC_NODROP);
	curNoDrop2 = theApp.LoadCursor(IDC_NODRAG);
	curVSplit = theApp.LoadCursor(AFX_IDC_HSPLITBAR);
	// bitmaps
	bmpPatterns = LoadDib(MAKEINTRESOURCE(IDB_PATTERNS));
	bmpNotes = LoadDib(MAKEINTRESOURCE(IDB_PATTERNVIEW));
	bmpVUMeters = LoadDib(MAKEINTRESOURCE(IDB_VUMETERS));
	bmpVisNode = LoadDib(MAKEINTRESOURCE(IDB_VISNODE));
	bmpVisPcNode = LoadDib(MAKEINTRESOURCE(IDB_VISPCNODE));
	UpdateColors();
	// Toolbars
	EnableDocking(CBRS_ALIGN_ANY);
	if (!m_wndToolBar.Create(this)) return -1;
	if (!m_wndStatusBar.Create(this)) return -1;
	if (!m_wndTree.Create(this, IDD_TREEVIEW, CBRS_LEFT|CBRS_BORDER_RIGHT, IDD_TREEVIEW)) return -1;
	m_wndStatusBar.SetIndicators(indicators, CountOf(indicators));
	m_wndToolBar.Init(this);
	m_wndTree.RecalcLayout();

	RemoveControlBar(&m_wndStatusBar); //Removing statusbar because corrupted statusbar inifiledata can crash the program.
	m_wndTree.EnableDocking(0); //To prevent a crash when there's "Docking=1" for treebar in ini-settings.
	// Restore toobar positions
	LoadBarState("Toolbars");

	AddControlBar(&m_wndStatusBar); //Restore statusbar to mainframe.

	if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_MIDIRECORD) OnMidiRecord();

	return 0;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
//------------------------------------------------
{
	return CMDIFrameWnd::PreCreateWindow(cs);
}


BOOL CMainFrame::DestroyWindow()
//------------------------------
{
	CSoundFile::gpSndMixHook = nullptr;

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	// Uninstall Keyboard Hook
	if (ghKbdHook)
	{
		UnhookWindowsHookEx(ghKbdHook);
		ghKbdHook = NULL;
	}
	// Kill Timer
	if (m_nTimer)
	{
		KillTimer(m_nTimer);
		m_nTimer = 0;
	}
	if (shMidiIn) midiCloseDevice();
	if(m_hNotifyThread != NULL)
	{
		PostThreadMessage(m_dwNotifyThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(m_hNotifyThread, INFINITE);
		m_dwNotifyThreadId = 0;
		m_hNotifyThread = NULL;
	}
	if(m_hNotifyWakeUp != NULL)
	{
		CloseHandle(m_hNotifyWakeUp);
		m_hNotifyWakeUp = NULL;
	}
	if(m_PendingNotificationSempahore != NULL)
	{
		CloseHandle(m_PendingNotificationSempahore);
		m_PendingNotificationSempahore = NULL;
	}
	// Delete bitmaps
	if (bmpPatterns)
	{
		delete bmpPatterns;
		bmpPatterns = NULL;
	}
	if (bmpNotes)
	{
		delete bmpNotes;
		bmpNotes = NULL;
	}
	if (bmpVUMeters)
	{
		delete bmpVUMeters;
		bmpVUMeters = NULL;
	}
	if (bmpVisNode)
	{
		delete bmpVisNode;
		bmpVisNode = NULL;
	}
	if (bmpVisPcNode)
	{
		delete bmpVisPcNode;
		bmpVisPcNode = NULL;
	}

	// Kill GDI Objects
	DeleteGDIObject(brushGray);
	DeleteGDIObject(penLightGray);
	DeleteGDIObject(penDarkGray);
	DeleteGDIObject(penSample);
	DeleteGDIObject(penEnvelope);
	DeleteGDIObject(penEnvelopeHighlight);
	DeleteGDIObject(m_hFixedFont);
	DeleteGDIObject(m_hLargeFixedFont);
	DeleteGDIObject(penScratch);
	DeleteGDIObject(penGray00);
	DeleteGDIObject(penGray33);
	DeleteGDIObject(penGray40);
	DeleteGDIObject(penGray55);
	DeleteGDIObject(penGray80);
	DeleteGDIObject(penGray99);
	DeleteGDIObject(penGraycc);
	DeleteGDIObject(penGrayff);

	for (UINT i=0; i<NUM_VUMETER_PENS*2; i++)
	{
		if (gpenVuMeter[i])
		{
			DeleteObject(gpenVuMeter[i]);
			gpenVuMeter[i] = NULL;
		}
	}

	return CMDIFrameWnd::DestroyWindow();
}


void CMainFrame::OnClose()
//------------------------
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

	CChildFrame *pMDIActive = (CChildFrame *)MDIGetActive();

	BeginWaitCursor();
	if (IsPlaying()) PauseMod();
	if (pMDIActive) pMDIActive->SavePosition(TRUE);
	if (gpSoundDevice)
	{
		CriticalSection cs;
		//gpSoundDevice->Reset();
		//audioCloseDevice();
		delete gpSoundDevice;
		gpSoundDevice = NULL;
	}
	// Save Settings
	RemoveControlBar(&m_wndStatusBar); // Remove statusbar so that its state won't get saved.
	TrackerSettings::Instance().SaveSettings();
	AddControlBar(&m_wndStatusBar); // Restore statusbar to mainframe.

	if(m_InputHandler && m_InputHandler->activeCommandSet)
	{
		m_InputHandler->activeCommandSet->SaveFile(TrackerSettings::Instance().m_szKbdFile);
	}

	EndWaitCursor();
	CMDIFrameWnd::OnClose();
}


bool CMainFrame::WritePrivateProfileLong(const CString section, const CString key, const long value, const CString iniFile)
{
	CHAR valueBuffer[INIBUFFERSIZE];
	wsprintf(valueBuffer, "%li", value);
	return (WritePrivateProfileString(section, key, valueBuffer, iniFile) != 0);
}


long CMainFrame::GetPrivateProfileLong(const CString section, const CString key, const long defaultValue, const CString iniFile)
{
	CHAR defaultValueBuffer[INIBUFFERSIZE];
	wsprintf(defaultValueBuffer, "%li", defaultValue);

	CHAR valueBuffer[INIBUFFERSIZE];
	GetPrivateProfileString(section, key, defaultValueBuffer, valueBuffer, INIBUFFERSIZE, iniFile);

	return atol(valueBuffer);
}


bool CMainFrame::WritePrivateProfileDWord(const CString section, const CString key, const DWORD value, const CString iniFile)
{
	CHAR valueBuffer[INIBUFFERSIZE];
	wsprintf(valueBuffer, "%lu", value);
	return (WritePrivateProfileString(section, key, valueBuffer, iniFile) != 0);
}

DWORD CMainFrame::GetPrivateProfileDWord(const CString section, const CString key, const DWORD defaultValue, const CString iniFile)
{
	CHAR defaultValueBuffer[INIBUFFERSIZE];
	wsprintf(defaultValueBuffer, "%lu", defaultValue);

	CHAR valueBuffer[INIBUFFERSIZE];
	GetPrivateProfileString(section, key, defaultValueBuffer, valueBuffer, INIBUFFERSIZE, iniFile);
	return static_cast<DWORD>(atol(valueBuffer));
}

bool CMainFrame::WritePrivateProfileCString(const CString section, const CString key, const CString value, const CString iniFile)
{
	return (WritePrivateProfileString(section, key, value, iniFile) != 0);
}

CString CMainFrame::GetPrivateProfileCString(const CString section, const CString key, const CString defaultValue, const CString iniFile)
{
	CHAR defaultValueBuffer[INIBUFFERSIZE];
	strcpy(defaultValueBuffer, defaultValue);
	CHAR valueBuffer[INIBUFFERSIZE];
	GetPrivateProfileString(section, key, defaultValueBuffer, valueBuffer, INIBUFFERSIZE, iniFile);
	return valueBuffer;
}



LRESULT CALLBACK CMainFrame::KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------------------------
{
	if (code>=0)
	{
		//Check if textbox has focus
		bool textboxHasFocus = false;
		bool handledByTextBox = false;

		HWND hWnd = ::GetFocus();
		if(hWnd != NULL)
		{
			TCHAR activeWindowClassName[512];
			GetClassName(hWnd, activeWindowClassName, 6);
			textboxHasFocus = _tcsicmp(activeWindowClassName, _T("Edit")) == 0;
			if(textboxHasFocus)
			{
				handledByTextBox = m_InputHandler->isKeyPressHandledByTextBox(wParam);
			}
		}

		if(!handledByTextBox && m_InputHandler->GeneralKeyEvent(kCtxAllContexts, code, wParam, lParam) != kcNull)
		{
			if(wParam != VK_ESCAPE)
				return -1;	// We've handled the keypress. No need to take it further.
							// Unless it was esc, in which case we need it to close Windows
							// (there might be other special cases, we'll see.. )
		}
	}

	return CallNextHookEx(ghKbdHook, code, wParam, lParam);
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
//---------------------------------------------
{
	if((pMsg->message == WM_RBUTTONDOWN) || (pMsg->message == WM_NCRBUTTONDOWN))
	{
		CWnd* pWnd = CWnd::FromHandlePermanent(pMsg->hwnd);
		CControlBar* pBar = NULL;
		HWND hwnd = (pWnd) ? pWnd->m_hWnd : NULL;

		if ((hwnd) && (pMsg->message == WM_RBUTTONDOWN)) pBar = DYNAMIC_DOWNCAST(CControlBar, pWnd);
		if ((pBar != NULL) || ((pMsg->message == WM_NCRBUTTONDOWN) && (pMsg->wParam == HTMENU)))
		{
			CMenu Menu;
			CPoint pt;

			GetCursorPos(&pt);
			if (Menu.LoadMenu(IDR_TOOLBARS))
			{
				CMenu* pSubMenu = Menu.GetSubMenu(0);
				if (pSubMenu!=NULL) pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,this);
			}
		}
	}
	return CMDIFrameWnd::PreTranslateMessage(pMsg);
}


void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
//---------------------------------------------------
{
	if ((GetStyle() & FWS_ADDTOTITLE) == 0)	return;     // leave it alone!

	CMDIChildWnd* pActiveChild = NULL;
	CDocument* pDocument = GetActiveDocument();
	if (bAddToTitle &&
	  (pActiveChild = MDIGetActive()) != NULL &&
	  (pActiveChild->GetStyle() & WS_MAXIMIZE) == 0 &&
	  (pDocument != NULL ||
	   (pDocument = pActiveChild->GetActiveDocument()) != NULL))
	{
		TCHAR szText[256+_MAX_PATH];
		lstrcpy(szText, pDocument->GetTitle());
		if (pDocument->IsModified()) lstrcat(szText, "*");
		UpdateFrameTitleForDocument(szText);
	} else
	{
		LPCTSTR lpstrTitle = NULL;
		CString strTitle;

		if (pActiveChild != NULL)
		{
			strTitle = pActiveChild->GetTitle();
			if (!strTitle.IsEmpty())
				lpstrTitle = strTitle;
		}
		UpdateFrameTitleForDocument(lpstrTitle);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame Sound Library

// Notify thread
DWORD WINAPI CMainFrame::NotifyThreadWrapper(LPVOID)
{
	return ((CMainFrame*)theApp.m_pMainWnd)->NotifyThread();
}
DWORD CMainFrame::NotifyThread()
//-------------------------------------------
{
	// initialize thread message queue
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
#ifdef NDEBUG
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL ); // we shall not stall the audio thread while holding m_NotificationBufferMutex
#endif
	bool terminate = false;
	bool cansend = true;
	while(!terminate)
	{
		switch(MsgWaitForMultipleObjects(1, &m_hNotifyWakeUp, FALSE, 1000, QS_ALLEVENTS))
		{
			case WAIT_OBJECT_0 + 1:
			{
				while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if(msg.message == WM_QUIT) terminate = true;
				}
			}
			break;
			case WAIT_OBJECT_0:
				{
					const MPTNOTIFICATION * pnotify = nullptr;
					{
						int64 currenttotalsamples = 0;
						Util::lock_guard<Util::mutex> lock(m_NotificationBufferMutex);
						if(gpSoundDevice && gpSoundDevice->HasGetStreamPosition())
						{
							currenttotalsamples = gpSoundDevice->GetStreamPositionSamples(); 
						} else
						{
							currenttotalsamples = m_TotalSamplesRendered; 
						}
						// advance to the newest notification, drop the obsolete ones
						const MPTNOTIFICATION * p = m_NotifyBuffer.peek_p();
						if(p && currenttotalsamples >= p->TimestampSamples)
						{
							pnotify = p;
							while(m_NotifyBuffer.peek_next_p() && currenttotalsamples >= m_NotifyBuffer.peek_next_p()->TimestampSamples)
							{
								m_NotifyBuffer.pop();
								p = m_NotifyBuffer.peek_p();
								pnotify = p;
							}
						}
					}
					if(pnotify)
					{
						if(!cansend)
						{
							// poll the semaphore instead of waiting directly after sending the message to avoid deadlocks on termination of openmpt or when stopping audio rendering
							if(WaitForSingleObject(m_PendingNotificationSempahore, 0) == WAIT_OBJECT_0)
							{
								// last notification has been handled by gui thread, so we can pop the notify buffer
								cansend = true;
							}
						}
						if(cansend)
						{
							m_PendingNotification = *pnotify; // copy notification so that we can free the buffer
							{
								Util::lock_guard<Util::mutex> lock(m_NotificationBufferMutex);
								m_NotifyBuffer.pop();
							}
							if(PostMessage(WM_MOD_UPDATEPOSITIONTHREADED, 0, 0))
							{
								cansend = false;
							}
						}
					}
				}
			break;
		}
	}
	return 0;
}


void CMainFrame::SetAudioThreadActive(bool active)
//------------------------------------------------
{
	if(active)
	{
		if(m_IsPlaybackRunning) return;
		m_IsPlaybackRunning = true;
		gpSoundDevice->Start();
	} else
	{
		if(!m_IsPlaybackRunning) return;
		gpSoundDevice->Stop();
		m_IsPlaybackRunning = false;
	}
}


void CMainFrame::FillAudioBufferLocked(IFillAudioBuffer &callback)
//----------------------------------------------------------------
{
	CriticalSection cs;
	callback.FillAudioBuffer();
}


ULONG CMainFrame::AudioRead(PVOID pvData, ULONG MaxSamples)
//---------------------------------------------------------
{
		DWORD dwSamplesRead = m_pSndFile->Read(pvData, MaxSamples);
		//m_dTotalCPU = m_pPerfCounter->StartStop()/(static_cast<double>(dwSamplesRead)/m_dwRate);
		return dwSamplesRead;
}


void CMainFrame::AudioDone(ULONG SamplesWritten, ULONG SamplesLatency, bool end_of_stream)
//----------------------------------------------------------------------------------------
{
	if (SamplesWritten > 0)
	{
		DoNotification(SamplesWritten, SamplesLatency, end_of_stream);
	}
}


bool CMainFrame::audioTryOpeningDevice(UINT channels, UINT bits, UINT samplespersec)
//----------------------------------------------------------------------------------
{
	WAVEFORMATEXTENSIBLE WaveFormat;

	UINT bytespersample = (bits/8) * channels;
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.Format.nChannels = (unsigned short) channels;
	WaveFormat.Format.nSamplesPerSec = samplespersec;
	WaveFormat.Format.nAvgBytesPerSec = samplespersec * bytespersample;
	WaveFormat.Format.nBlockAlign = (unsigned short)bytespersample;
	WaveFormat.Format.wBitsPerSample = (unsigned short)bits;
	WaveFormat.Format.cbSize = 0;
	// MultiChannel configuration
	if ((WaveFormat.Format.wBitsPerSample == 32) || (WaveFormat.Format.nChannels > 2))
	{
		const GUID guid_MEDIASUBTYPE_PCM = {0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x0, 0xAA, 0x0, 0x38, 0x9B, 0x71};
		WaveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		WaveFormat.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		WaveFormat.Samples.wValidBitsPerSample = WaveFormat.Format.wBitsPerSample;
		switch(WaveFormat.Format.nChannels)
		{
		case 1:		WaveFormat.dwChannelMask = 0x0004; break; // FRONT_CENTER
		case 2:		WaveFormat.dwChannelMask = 0x0003; break; // FRONT_LEFT | FRONT_RIGHT
		case 3:		WaveFormat.dwChannelMask = 0x0103; break; // FRONT_LEFT|FRONT_RIGHT|BACK_CENTER
		case 4:		WaveFormat.dwChannelMask = 0x0033; break; // FRONT_LEFT|FRONT_RIGHT|BACK_LEFT|BACK_RIGHT
		default:	WaveFormat.dwChannelMask = 0; return false; break;
		}
		WaveFormat.SubFormat = guid_MEDIASUBTYPE_PCM;
	}
	UINT nDevType = SNDDEV_GET_TYPE(TrackerSettings::Instance().m_nWaveDevice);
	if ((gpSoundDevice) && (gpSoundDevice->GetDeviceType() != nDevType))
	{
		delete gpSoundDevice;
		gpSoundDevice = NULL;
	}
	if(!gpSoundDevice) gpSoundDevice = CreateSoundDevice(nDevType);
	if(!gpSoundDevice) return false;
	gpSoundDevice->SetSource(this);
	gpSoundDevice->Configure(m_hWnd, TrackerSettings::Instance().m_LatencyMS, TrackerSettings::Instance().m_UpdateIntervalMS, (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_SECONDARY) ? SNDDEV_OPTIONS_SECONDARY : 0);
	if (!gpSoundDevice->Open(SNDDEV_GET_NUMBER(TrackerSettings::Instance().m_nWaveDevice), &WaveFormat.Format)) return false;
	return true;
}


bool CMainFrame::IsAudioDeviceOpen() const
//----------------------------------------
{
	 return gpSoundDevice && gpSoundDevice->IsOpen();
}


bool CMainFrame::audioOpenDevice()
//--------------------------------
{
	if(IsAudioDeviceOpen()) return true;
	UINT nFixedBitsPerSample;
	bool err = false;

	if (!TrackerSettings::Instance().m_dwRate) err = true;
	if ((TrackerSettings::Instance().m_nChannels != 1) && (TrackerSettings::Instance().m_nChannels != 2) && (TrackerSettings::Instance().m_nChannels != 4)) err = true;
	if(!err)
	{
		err = !audioTryOpeningDevice(TrackerSettings::Instance().m_nChannels,
								TrackerSettings::Instance().m_nBitsPerSample,
								TrackerSettings::Instance().m_dwRate);
		nFixedBitsPerSample = (gpSoundDevice) ? gpSoundDevice->HasFixedBitsPerSample() : 0;
		if(err && (nFixedBitsPerSample && (nFixedBitsPerSample != TrackerSettings::Instance().m_nBitsPerSample)))
		{
			if(nFixedBitsPerSample) TrackerSettings::Instance().m_nBitsPerSample = nFixedBitsPerSample;
			err = !audioTryOpeningDevice(TrackerSettings::Instance().m_nChannels,
									TrackerSettings::Instance().m_nBitsPerSample,
									TrackerSettings::Instance().m_dwRate);
		}
	}
	// Display error message box
	if(err)
	{
		Reporting::Error("Unable to open sound device!");
		return false;
	}
	// Device is ready
	return true;
}


bool CMainFrame::audioReopenDevice()
//----------------------------------
{
	audioCloseDevice();
	return audioOpenDevice();
}


void CMainFrame::audioCloseDevice()
//---------------------------------
{
	if (gpSoundDevice)
	{
		CriticalSection cs;

		gpSoundDevice->Reset();
		gpSoundDevice->Close();

		// reset notify buffer as timestamps revert here
		{
			Util::lock_guard<Util::mutex> lock(m_NotificationBufferMutex);
			m_NotifyBuffer.clear();
			m_TotalSamplesRendered = 0;
		}
	}
}


void CMainFrame::CalcStereoVuMeters(int *pMix, unsigned long nSamples, unsigned long nChannels)
//---------------------------------------------------------------------------------------------
{
	const int * const p = pMix;
	int lmax = gnLVuMeter, rmax = gnRVuMeter;
	if (nChannels > 1)
	{
		for (UINT i=0; i<nSamples; i+=nChannels)
		{
			int vl = p[i];
			int vr = p[i+1];
			if (vl < 0) vl = -vl;
			if (vr < 0) vr = -vr;
			if (vl > lmax) lmax = vl;
			if (vr > rmax) rmax = vr;
		}
	} else
	{
		for (UINT i=0; i<nSamples; i++)
		{
			int vl = p[i];
			if (vl < 0) vl = -vl;
			if (vl > lmax) lmax = vl;
		}
		rmax = lmax;
	}
	gnLVuMeter = lmax;
	gnRVuMeter = rmax;
}


BOOL CMainFrame::DoNotification(DWORD dwSamplesRead, DWORD SamplesLatency, bool end_of_stream)
//-------------------------------------------------------------------
{
	int64 notificationtimestamp = 0;
	{
		Util::lock_guard<Util::mutex> lock(m_NotificationBufferMutex); // protect m_TotalSamplesRendered
		m_TotalSamplesRendered += dwSamplesRead;
		if(gpSoundDevice->HasGetStreamPosition())
		{
			notificationtimestamp = m_TotalSamplesRendered;
		} else
		{
			notificationtimestamp = m_TotalSamplesRendered + SamplesLatency;
		}
	}
	if (!m_pSndFile) return FALSE;
	if (m_nMixChn < m_pSndFile->m_nMixStat) m_nMixChn++;
	if (m_nMixChn > m_pSndFile->m_nMixStat) m_nMixChn--;
	if (!(m_dwNotifyType & MPTNOTIFY_TYPEMASK)) return FALSE;
	// Notify Client
	//if(m_NotifyBuffer.read_size() > 0)
	{
		SetEvent(m_hNotifyWakeUp);
	}
	// Add an entry to the notification history

	MPTNOTIFICATION notification;
	MemsetZero(notification);
	MPTNOTIFICATION *p = &notification;

			p->dwType = m_dwNotifyType | (end_of_stream ? MPTNOTIFY_EOS : 0);
			p->TimestampSamples = notificationtimestamp;
			p->nOrder = m_pSndFile->m_nCurrentOrder;
			p->nRow = m_pSndFile->m_nRow;
			p->nPattern = m_pSndFile->m_nPattern;
			p->nTick = m_pSndFile->m_nTickCount;
			if (m_dwNotifyType & MPTNOTIFY_SAMPLE)
			{
				SAMPLEINDEX nSmp = m_dwNotifyType & 0xFFFF;
				for (CHANNELINDEX k = 0; k < MAX_CHANNELS; k++)
				{
					ModChannel *pChn = &m_pSndFile->Chn[k];
					p->dwPos[k] = 0;
					if ((nSmp) && (nSmp <= m_pSndFile->GetNumSamples()) && (pChn->nLength)
					 && (pChn->pSample) && (pChn->pSample == m_pSndFile->GetSample(nSmp).pSample)
					 && ((!(pChn->dwFlags & CHN_NOTEFADE)) || (pChn->nFadeOutVol)))
					{
						p->dwPos[k] = MPTNOTIFY_POSVALID | (DWORD)(pChn->nPos);
					}
				}
			} else if (m_dwNotifyType & (MPTNOTIFY_VOLENV|MPTNOTIFY_PANENV|MPTNOTIFY_PITCHENV))
			{
				UINT nIns = m_dwNotifyType & 0xFFFF;
				for (CHANNELINDEX k = 0; k < MAX_CHANNELS; k++)
				{
					ModChannel *pChn = &m_pSndFile->Chn[k];
					p->dwPos[k] = 0;
					if(nIns != 0 && nIns <= m_pSndFile->GetNumInstruments()									// There is an instrument
						&& pChn->pModInstrument && pChn->pModInstrument == m_pSndFile->Instruments[nIns]	// And it's the correct instrument
						&& (pChn->nLength || pChn->pModInstrument->HasValidMIDIChannel())					// And it's playing something (sample or instrument)
						&& (!pChn->dwFlags[CHN_NOTEFADE] || pChn->nFadeOutVol))								// And it hasn't completely faded out yet, so it's still playing
					{
						enmEnvelopeTypes notifyEnv = ENV_VOLUME;
						if (m_dwNotifyType & MPTNOTIFY_PITCHENV)
							notifyEnv = ENV_PITCH;
						else if (m_dwNotifyType & MPTNOTIFY_PANENV)
							notifyEnv = ENV_PANNING;

						const ModChannel::EnvInfo &chnEnv = pChn->GetEnvelope(notifyEnv);

						if(chnEnv.flags[ENV_ENABLED])
						{
							uint32 pos = chnEnv.nEnvPosition;
							if(m_pSndFile->IsCompatibleMode(TRK_IMPULSETRACKER))
							{
								// Impulse Tracker envelope handling (see e.g. CSoundFile::IncrementEnvelopePosition in SndMix.cpp for details)
								if(pos > 0)
									pos--;
								else
									continue;
							}
							p->dwPos[k] = MPTNOTIFY_POSVALID | pos;
						}
					}
				}
			} else if (m_dwNotifyType & (MPTNOTIFY_VUMETERS))
			{
				for (UINT k=0; k<MAX_CHANNELS; k++)
				{
					ModChannel *pChn = &m_pSndFile->Chn[k];
					UINT vul = pChn->nLeftVU;
					UINT vur = pChn->nRightVU;
					p->dwPos[k] = (vul << 8) | (vur);
				}
			}
			{
				DWORD lVu = (gnLVuMeter >> 11);
				DWORD rVu = (gnRVuMeter >> 11);
				if (lVu > 0x10000) lVu = 0x10000;
				if (rVu > 0x10000) rVu = 0x10000;
				p->MasterVuLeft = lVu;
				p->MasterVuRight = rVu;
				DWORD dwVuDecay = Util::muldiv(dwSamplesRead, 120000, TrackerSettings::Instance().m_dwRate) + 1;
				if (lVu >= dwVuDecay) gnLVuMeter = (lVu - dwVuDecay) << 11; else gnLVuMeter = 0;
				if (rVu >= dwVuDecay) gnRVuMeter = (rVu - dwVuDecay) << 11; else gnRVuMeter = 0;
			}

	{
		Util::lock_guard<Util::mutex> lock(m_NotificationBufferMutex);
		if(m_NotifyBuffer.write_size() == 0) return FALSE; // drop notification
		m_NotifyBuffer.push(notification);
	}

	SetEvent(m_hNotifyWakeUp);

	return TRUE;

}


void CMainFrame::UpdateAudioParameters(BOOL bReset)
//-------------------------------------------------
{
	if ((TrackerSettings::Instance().m_nBitsPerSample != 8) && (TrackerSettings::Instance().m_nBitsPerSample != 32)) TrackerSettings::Instance().m_nBitsPerSample = 16;
	CSoundFile::SetWaveConfig(TrackerSettings::Instance().m_dwRate,
			TrackerSettings::Instance().m_nBitsPerSample,
			TrackerSettings::Instance().m_nChannels,
			(TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) ? TRUE : FALSE);
	if (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_STREVERSE)
		CSoundFile::gdwSoundSetup |= SNDMIX_REVERSESTEREO;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_REVERSESTEREO;

	// Soft panning
	if (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_SOFTPANNING)
		CSoundFile::gdwSoundSetup |= SNDMIX_SOFTPANNING;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_SOFTPANNING;
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_MUTECHNMODE)
		CSoundFile::gdwSoundSetup |= SNDMIX_MUTECHNMODE;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_MUTECHNMODE;
	CSoundFile::SetResamplingMode(TrackerSettings::Instance().m_nSrcMode);
	CSoundFile::UPDATEDSPEFFECTS();
	CSoundFile::SetAGC(TrackerSettings::Instance().m_dwQuality & QUALITY_AGC);
	CSoundFile::SetEQGains(	TrackerSettings::Instance().m_EqSettings.Gains, MAX_EQ_BANDS, TrackerSettings::Instance().m_EqSettings.Freqs, bReset );
	if (bReset)
	{
		CSoundFile::SetMixerSettings(TrackerSettings::Instance().m_MixerSettings);
		CSoundFile::InitPlayer(TRUE);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame static helpers


void CMainFrame::UpdateColors()
//-----------------------------
{
	COLORREF (&colors)[MAX_MODCOLORS] = TrackerSettings::Instance().rgbCustomColors;
	if (bmpPatterns)
	{
		bmpPatterns->bmiColors[7] = rgb2quad(GetSysColor(COLOR_BTNFACE));
	}
	if (bmpVUMeters)
	{
		bmpVUMeters->bmiColors[7] = rgb2quad(GetSysColor(COLOR_BTNFACE));
		bmpVUMeters->bmiColors[8] = rgb2quad(GetSysColor(COLOR_BTNSHADOW));
		bmpVUMeters->bmiColors[15] = rgb2quad(GetSysColor(COLOR_BTNHIGHLIGHT));
		bmpVUMeters->bmiColors[10] = rgb2quad(colors[MODCOLOR_VUMETER_LO]);
		bmpVUMeters->bmiColors[11] = rgb2quad(colors[MODCOLOR_VUMETER_MED]);
		bmpVUMeters->bmiColors[9] = rgb2quad(colors[MODCOLOR_VUMETER_HI]);
		bmpVUMeters->bmiColors[2] = rgb2quad((colors[MODCOLOR_VUMETER_LO] >> 1) & 0x7F7F7F);
		bmpVUMeters->bmiColors[3] = rgb2quad((colors[MODCOLOR_VUMETER_MED] >> 1) & 0x7F7F7F);
		bmpVUMeters->bmiColors[1] = rgb2quad((colors[MODCOLOR_VUMETER_HI] >> 1) & 0x7F7F7F);
	}
	if (penSample) DeleteObject(penSample);
	penSample = ::CreatePen(PS_SOLID, 0, colors[MODCOLOR_SAMPLE]);
	if (penEnvelope) DeleteObject(penEnvelope);
	penEnvelope = ::CreatePen(PS_SOLID, 0, colors[MODCOLOR_ENVELOPES]);
	if (penEnvelopeHighlight) DeleteObject(penEnvelopeHighlight);
	penEnvelopeHighlight = ::CreatePen(PS_SOLID, 0, RGB(0xFF, 0xFF, 0x00));

	// Generel tab VU meters
	for (UINT i=0; i<NUM_VUMETER_PENS*2; i++)
	{
		int r0,g0,b0, r1,g1,b1;
		int r, g, b;
		int y;

		if (gpenVuMeter[i])
		{
			DeleteObject(gpenVuMeter[i]);
			gpenVuMeter[i] = NULL;
		}
		y = (i >= NUM_VUMETER_PENS) ? (i-NUM_VUMETER_PENS) : i;
		if (y < (NUM_VUMETER_PENS/2))
		{
			r0 = GetRValue(colors[MODCOLOR_VUMETER_LO]);
			g0 = GetGValue(colors[MODCOLOR_VUMETER_LO]);
			b0 = GetBValue(colors[MODCOLOR_VUMETER_LO]);
			r1 = GetRValue(colors[MODCOLOR_VUMETER_MED]);
			g1 = GetGValue(colors[MODCOLOR_VUMETER_MED]);
			b1 = GetBValue(colors[MODCOLOR_VUMETER_MED]);
		} else
		{
			y -= (NUM_VUMETER_PENS/2);
			r0 = GetRValue(colors[MODCOLOR_VUMETER_MED]);
			g0 = GetGValue(colors[MODCOLOR_VUMETER_MED]);
			b0 = GetBValue(colors[MODCOLOR_VUMETER_MED]);
			r1 = GetRValue(colors[MODCOLOR_VUMETER_HI]);
			g1 = GetGValue(colors[MODCOLOR_VUMETER_HI]);
			b1 = GetBValue(colors[MODCOLOR_VUMETER_HI]);
		}
		r = r0 + ((r1 - r0) * y) / (NUM_VUMETER_PENS/2);
		g = g0 + ((g1 - g0) * y) / (NUM_VUMETER_PENS/2);
		b = b0 + ((b1 - b0) * y) / (NUM_VUMETER_PENS/2);
		if (i >= NUM_VUMETER_PENS)
		{
			r = (r*2)/5;
			g = (g*2)/5;
			b = (b*2)/5;
		}
		gpenVuMeter[i] = CreatePen(PS_SOLID, 0, RGB(r, g, b));
	}
	// Sequence window
	{
		COLORREF crBkgnd = GetSysColor(COLOR_WINDOW);
		if (brushHighLight) DeleteObject(brushHighLight);
		brushHighLight = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
		if (brushHighLightRed) DeleteObject(brushHighLightRed);
		brushHighLightRed = CreateSolidBrush(RGB(0xFF,0x00,0x00));
		if (brushYellow) DeleteObject(brushYellow);
		brushYellow = CreateSolidBrush(RGB(0xFF,0xFF,0x00));

		if (brushWindow) DeleteObject(brushWindow);
		brushWindow = CreateSolidBrush(crBkgnd);
		if (penSeparator) DeleteObject(penSeparator);
		penSeparator = CreatePen(PS_SOLID, 0, RGB(GetRValue(crBkgnd)/2, GetGValue(crBkgnd)/2, GetBValue(crBkgnd)/2));
	}
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame operations

UINT CMainFrame::GetBaseOctave() const
//------------------------------------
{
	return m_wndToolBar.GetBaseOctave();
}


void CMainFrame::SetPreAmp(UINT n)
//--------------------------------
{
	TrackerSettings::Instance().m_nPreAmp = n;
	if (m_pSndFile) m_pSndFile->SetMasterVolume(TrackerSettings::Instance().m_nPreAmp, true);
}


void CMainFrame::ResetNotificationBuffer()
//----------------------------------------
{
	Util::lock_guard<Util::mutex> lock(m_NotificationBufferMutex);
	m_NotifyBuffer.clear();
}


void CMainFrame::ApplyTrackerSettings(CSoundFile *pSndFile)
//----------------------------------------------------------
{
	if(!pSndFile) return;
	if (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_STREVERSE) CSoundFile::gdwSoundSetup |= SNDMIX_REVERSESTEREO;
	else CSoundFile::gdwSoundSetup &= ~SNDMIX_REVERSESTEREO;
	pSndFile->SetWaveConfig(TrackerSettings::Instance().m_dwRate, TrackerSettings::Instance().m_nBitsPerSample, TrackerSettings::Instance().m_nChannels, (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) ? TRUE : FALSE);
	pSndFile->SetResamplingMode(TrackerSettings::Instance().m_nSrcMode);
	pSndFile->UPDATEDSPEFFECTS();
	pSndFile->SetAGC(TrackerSettings::Instance().m_dwQuality & QUALITY_AGC);
	pSndFile->SetMasterVolume(TrackerSettings::Instance().m_nPreAmp, true);
	pSndFile->SetMixerSettings(TrackerSettings::Instance().m_MixerSettings);
	CSoundFile::ResetAGC();
}


bool CMainFrame::PreparePlayback()
//--------------------------------
{
	// open the audio device to update needed TrackerSettings mixer parameters
	if(!audioOpenDevice()) return false;
	return true;
}


bool CMainFrame::StartPlayback()
//------------------------------
{
	if(!m_pSndFile) return false; // nothing to play
	if(!IsAudioDeviceOpen()) return false;
	SetAudioThreadActive(true);
	return true;
}


void CMainFrame::StopPlayback()
//-----------------------------
{
	if(!IsAudioDeviceOpen()) return;
	SetAudioThreadActive(false);
	audioCloseDevice();
}


bool CMainFrame::PausePlayback()
//------------------------------
{
	if(!IsAudioDeviceOpen()) return false;
	SetAudioThreadActive(false);
	return true;
}


void CMainFrame::GenerateStopNotification()
//-----------------------------------------
{
	MPTNOTIFICATION mn;
	MemsetZero(mn);
	mn.dwType = MPTNOTIFY_STOP;
	SendMessage(WM_MOD_UPDATEPOSITION, 0, (LPARAM)&mn);
}


void CMainFrame::UnsetPlaybackSoundFile()
//---------------------------------------
{
	if(m_pSndFile)
	{
		m_pSndFile->SuspendPlugins();
		m_nMixChn = 0;
		m_nAvgMixChn = 0;
		if(m_pSndFile->GetpModDoc())
		{
			m_wndTree.UpdatePlayPos(m_pSndFile->GetpModDoc(), NULL);
		}
		m_pSndFile->m_SongFlags.reset(SONG_PAUSED);
		if(m_pSndFile == &m_WaveFile)
		{
			// Unload previewed instrument
			m_WaveFile.Destroy();
		} else
		{
			// Stop sample preview channels
			for(CHANNELINDEX i = m_pSndFile->m_nChannels; i < MAX_CHANNELS; i++)
			{
				if(!(m_pSndFile->Chn[i].nMasterChn))
				{
					m_pSndFile->Chn[i].nPos = m_pSndFile->Chn[i].nPosLo = m_pSndFile->Chn[i].nLength = 0;
				}
			}
		}
	}
	m_pSndFile = nullptr;
	m_hFollowSong = NULL;
	m_wndToolBar.SetCurrentSong(nullptr);
	{
		Util::lock_guard<Util::mutex> lock(m_NotificationBufferMutex);
		m_NotifyBuffer.clear();
	}
}


void CMainFrame::SetPlaybackSoundFile(CSoundFile *pSndFile, HWND hPat, DWORD dwNotifyType)
//----------------------------------------------------------------------------------------
{
	m_pSndFile = pSndFile;
	m_hFollowSong = hPat;
	m_dwNotifyType = dwNotifyType;
}


bool CMainFrame::PlayMod(CModDoc *pModDoc, HWND hPat, DWORD dwNotifyType)
//-----------------------------------------------------------------------
{
	CriticalSection::AssertUnlocked();
	if(!pModDoc) return false;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(!IsValidSoundFile(pSndFile)) return false;

	// if something is playing, pause it
	PausePlayback();
	GenerateStopNotification();

	UnsetPlaybackSoundFile();

	// open audio device if not already open
	if (!PreparePlayback()) return false;

	// set mixing parameters in CSoundFile
	ApplyTrackerSettings(pSndFile);

	SetPlaybackSoundFile(pSndFile, hPat, dwNotifyType);

	const bool bPaused = m_pSndFile->IsPaused();
	const bool bPatLoop = m_pSndFile->m_SongFlags[SONG_PATTERNLOOP];

	m_pSndFile->ResetChannels();
	// Select correct bidi loop mode when playing a module.
	m_pSndFile->SetupITBidiMode();

	if(!bPatLoop && bPaused) pSndFile->m_SongFlags.set(SONG_PAUSED);
	pSndFile->SetRepeatCount((TrackerSettings::Instance().gbLoopSong) ? -1 : 0);

	m_pSndFile->InitPlayer(TRUE);

	m_wndToolBar.SetCurrentSong(m_pSndFile);

	gnLVuMeter = 0;
	gnRVuMeter = 0;
	m_nMixChn = 0;
	m_nAvgMixChn = 0;

	if(!StartPlayback())
	{
		UnsetPlaybackSoundFile();
		return false;
	}

	return true;
}


bool CMainFrame::PauseMod(CModDoc *pModDoc)
//-----------------------------------------
{
	CriticalSection::AssertUnlocked();
	if(pModDoc && (pModDoc != GetModPlaying())) return false;
	if(!IsPlaying()) return true;

	PausePlayback();
	GenerateStopNotification();

	UnsetPlaybackSoundFile();

	StopPlayback();

	return true;
}


bool CMainFrame::StopMod(CModDoc *pModDoc)
//----------------------------------------
{
	CriticalSection::AssertUnlocked();
	if(pModDoc && (pModDoc != GetModPlaying())) return false;
	if(!IsPlaying()) return true;

	PausePlayback();
	GenerateStopNotification();

	m_pSndFile->SetCurrentPos(0);
	UnsetPlaybackSoundFile();

	StopPlayback();

	return true;
}


bool CMainFrame::StopSoundFile(CSoundFile *pSndFile)
//--------------------------------------------------
{
	CriticalSection::AssertUnlocked();
	if(!IsValidSoundFile(pSndFile)) return false;
	if(pSndFile != m_pSndFile) return false;
	if(!IsPlaying()) return true;

	PausePlayback();
	GenerateStopNotification();

	UnsetPlaybackSoundFile();

	StopPlayback();

	return true;
}


bool CMainFrame::PlaySoundFile(CSoundFile *pSndFile)
//--------------------------------------------------
{
	CriticalSection::AssertUnlocked();
	if(!IsValidSoundFile(pSndFile)) return false;

	PausePlayback();
	GenerateStopNotification();

	if(m_pSndFile != pSndFile)
	{
		UnsetPlaybackSoundFile();
	}

	if(!PreparePlayback())
	{
		UnsetPlaybackSoundFile();
		return false;
	}

	ApplyTrackerSettings(pSndFile);

	SetPlaybackSoundFile(pSndFile);

	m_pSndFile->InitPlayer(TRUE);

	if(!StartPlayback())
	{
		UnsetPlaybackSoundFile();
		return false;
	}

	return true;
}


BOOL CMainFrame::PlayDLSInstrument(UINT nDLSBank, UINT nIns, UINT nRgn, ModCommand::NOTE note)
//--------------------------------------------------------------------------------------------
{
	if(nDLSBank >= CTrackApp::gpDLSBanks.size() || !CTrackApp::gpDLSBanks[nDLSBank]) return FALSE;
	bool ok = false;
	BeginWaitCursor();
	{
		CriticalSection cs;
		InitPreview();
		if(CTrackApp::gpDLSBanks[nDLSBank]->ExtractInstrument(&m_WaveFile, 1, nIns, nRgn))
		{
			PreparePreview(note);
			ok = true;
		}
	}
	EndWaitCursor();
	if(!ok)
	{
		PausePlayback();
		UnsetPlaybackSoundFile();
		StopPlayback();
		return false;
	}
	return PlaySoundFile(&m_WaveFile);
}


BOOL CMainFrame::PlaySoundFile(LPCSTR lpszFileName, ModCommand::NOTE note)
//------------------------------------------------------------------------
{
	bool ok = false;
	BeginWaitCursor();
	{
		CriticalSection cs;
		static CString prevFile;
		// Did we already load this file for previewing? Don't load it again if the preview is still running.
		ok = (prevFile == lpszFileName && m_pSndFile == &m_WaveFile);

		if(!ok && lpszFileName)
		{
			CMappedFile f;

			if(f.Open(lpszFileName))
			{
				DWORD dwLen = f.GetLength();
				if(dwLen)
				{
					LPBYTE p = f.Lock();
					if(p)
					{
						InitPreview();
						m_WaveFile.m_SongFlags.set(SONG_PAUSED);
						// Avoid hanging audio while reading file - we have removed all sample and instrument references before,
						// so it's safe to replace the sample / instrument now.
						cs.Leave();
						ok = m_WaveFile.ReadInstrumentFromFile(1, p, dwLen);
						if(!ok)
						{
							// Try reading as sample if reading as instrument fails
							ok = m_WaveFile.ReadSampleFromFile(1, p, dwLen);
							m_WaveFile.AllocateInstrument(1, 1);
						}
						f.Unlock();
					}
				}
				f.Close();
			}
		}
		if(ok)
		{
			// Write notes to pattern. Also done if we have previously loaded this file, since we might be previewing another note now.
			PreparePreview(note);
			prevFile = lpszFileName;
		}
	}
	EndWaitCursor();
	if(!ok)
	{
		PausePlayback();
		UnsetPlaybackSoundFile();
		StopPlayback();
		return false;
	}
	return PlaySoundFile(&m_WaveFile);
}


BOOL CMainFrame::PlaySoundFile(CSoundFile *pSong, INSTRUMENTINDEX nInstrument, SAMPLEINDEX nSample, ModCommand::NOTE note)
//------------------------------------------------------------------------------------------------------------------------
{
	bool ok = false;
	BeginWaitCursor();
	{
		CriticalSection cs;
		InitPreview();
		m_WaveFile.m_nType = pSong->m_nType;
		if ((nInstrument) && (nInstrument <= pSong->GetNumInstruments()))
		{
			m_WaveFile.m_nInstruments = 1;
			m_WaveFile.m_nSamples = 32;
		} else
		{
			m_WaveFile.m_nInstruments = 0;
			m_WaveFile.m_nSamples = 1;
		}
		if (nInstrument != INSTRUMENTINDEX_INVALID && nInstrument <= pSong->GetNumInstruments())
		{
			m_WaveFile.ReadInstrumentFromSong(1, pSong, nInstrument);
		} else if(nSample != SAMPLEINDEX_INVALID && nSample <= pSong->GetNumSamples())
		{
			m_WaveFile.ReadSampleFromSong(1, pSong, nSample);
		}
		PreparePreview(note);
		ok = true;
	}
	EndWaitCursor();
	if(!ok)
	{
		PausePlayback();
		UnsetPlaybackSoundFile();
		StopPlayback();
		return false;
	}
	return PlaySoundFile(&m_WaveFile);
}


void CMainFrame::InitPreview()
//----------------------------
{
	m_WaveFile.Destroy();
	m_WaveFile.Create(NULL, 0);
	// Avoid global volume ramping when trying samples in the treeview.
	m_WaveFile.m_pConfig->setGlobalVolumeAppliesToMaster(false);
	m_WaveFile.m_nDefaultGlobalVolume = m_WaveFile.m_nGlobalVolume = MAX_GLOBAL_VOLUME;
	m_WaveFile.m_nSamplePreAmp = 48;
	m_WaveFile.m_nDefaultTempo = 125;
	m_WaveFile.m_nDefaultSpeed = 6;
	m_WaveFile.m_nType = MOD_TYPE_IT;
	m_WaveFile.m_nChannels = 4;
	m_WaveFile.m_nInstruments = 1;
	m_WaveFile.m_nTempoMode = tempo_mode_classic;
	m_WaveFile.m_nMixLevels = mixLevels_compatible;
	m_WaveFile.Order.resize(2);
	m_WaveFile.Order[0] = 0;
	m_WaveFile.Order[1] = 1;
	m_WaveFile.Patterns.Insert(0, 64);
	m_WaveFile.Patterns.Insert(1, 1);
}


void CMainFrame::PreparePreview(ModCommand::NOTE note)
//----------------------------------------------------
{
	m_WaveFile.m_SongFlags.reset(SONG_PAUSED);
	m_WaveFile.SetRepeatCount(-1);
	m_WaveFile.SetCurrentPos(0);

	ModCommand *m = m_WaveFile.Patterns[0];
	if(m)
	{
		m[0].note = note;
		m[0].instr = 1;
		m[1].note = note;
		m[1].instr = 1;

		if(m_WaveFile.m_nSamples > 1 || m_WaveFile.GetSample(1).uFlags[CHN_LOOP])
		{
			m[32 * 4].note = NOTE_KEYOFF;
			m[32 * 4 + 1].note = NOTE_KEYOFF;
			m[63 * 4].note = NOTE_NOTECUT;
			m[63 * 4 + 1].note = NOTE_NOTECUT;
		}
	}

	m = m_WaveFile.Patterns[1];
	if(m)
	{
		m->command = CMD_POSITIONJUMP;
		m->param = 1;
	}
}


BOOL CMainFrame::SetFollowSong(CModDoc *pDoc, HWND hwnd, BOOL bFollowSong, DWORD dwType)
//--------------------------------------------------------------------------------------
{
	if ((!pDoc) || (pDoc != GetModPlaying())) return FALSE;
	if (bFollowSong)
	{
		m_hFollowSong = hwnd;
	} else
	{
		if (hwnd == m_hFollowSong) m_hFollowSong = NULL;
	}
	if (dwType) m_dwNotifyType = dwType;
	return TRUE;
}


BOOL CMainFrame::SetupSoundCard(DWORD q, DWORD rate, UINT nBits, UINT nChns, UINT latency_ms, UINT updateinterval_ms, LONG wd)
//-------------------------------------------------------------------------------------------------
{
	const bool isPlaying = (IsPlaying() != 0);
	if ((TrackerSettings::Instance().m_dwRate != rate) || ((TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_RESTARTMASK) != (q & SOUNDSETUP_RESTARTMASK))
	 || (TrackerSettings::Instance().m_nWaveDevice != wd) || (TrackerSettings::Instance().m_LatencyMS != latency_ms) || (TrackerSettings::Instance().m_UpdateIntervalMS != updateinterval_ms) || (nBits != TrackerSettings::Instance().m_nBitsPerSample)
	 || (TrackerSettings::Instance().m_nChannels != nChns))
	{
		CModDoc *pActiveMod = NULL;
		HWND hFollow = m_hFollowSong;
		if (isPlaying)
		{
			if ((m_pSndFile) && (!m_pSndFile->IsPaused())) pActiveMod = GetModPlaying();
			PauseMod();
		}
		TrackerSettings::Instance().m_nWaveDevice = wd;
		TrackerSettings::Instance().m_dwRate = rate;
		TrackerSettings::Instance().m_dwSoundSetup = q;
		TrackerSettings::Instance().m_LatencyMS = latency_ms;
		TrackerSettings::Instance().m_UpdateIntervalMS = updateinterval_ms;
		TrackerSettings::Instance().m_nBitsPerSample = nBits;
		TrackerSettings::Instance().m_nChannels = nChns;
		{
			CriticalSection cs;
			UpdateAudioParameters(FALSE);
		}
		if (pActiveMod) PlayMod(pActiveMod, hFollow, m_dwNotifyType);
		UpdateWindow();
	} else
	{
		// No need to restart playback
		TrackerSettings::Instance().m_dwSoundSetup = q;
		CSoundFile::EnableMMX((TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) != 0);
		if (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_STREVERSE)
			CSoundFile::gdwSoundSetup |= SNDMIX_REVERSESTEREO;
		else
			CSoundFile::gdwSoundSetup &= ~SNDMIX_REVERSESTEREO;
	}
	return TRUE;
}


BOOL CMainFrame::SetupPlayer(DWORD q, DWORD srcmode, BOOL bForceUpdate)
//---------------------------------------------------------------------
{
	if ((q != TrackerSettings::Instance().m_dwQuality) || (srcmode != TrackerSettings::Instance().m_nSrcMode) || (bForceUpdate))
	{
		TrackerSettings::Instance().m_nSrcMode = srcmode;
		TrackerSettings::Instance().m_dwQuality = q;
		{
			CriticalSection cs;
			CSoundFile::SetMixerSettings(TrackerSettings::Instance().m_MixerSettings);
			CSoundFile::SetResamplingMode(TrackerSettings::Instance().m_nSrcMode);
			CSoundFile::UPDATEDSPEFFECTS();
			CSoundFile::SetAGC(TrackerSettings::Instance().m_dwQuality & QUALITY_AGC);
		}
		PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTSETUP);
	}
	return TRUE;
}


BOOL CMainFrame::SetupDirectories(LPCTSTR szModDir, LPCTSTR szSampleDir, LPCTSTR szInstrDir, LPCTSTR szVstDir, LPCTSTR szPresetDir)
//---------------------------------------------------------------------------------------------------------------------------------
{
	// will also set working directory
	TrackerSettings::Instance().SetDefaultDirectory(szModDir, DIR_MODS);
	TrackerSettings::Instance().SetDefaultDirectory(szSampleDir, DIR_SAMPLES);
	TrackerSettings::Instance().SetDefaultDirectory(szInstrDir, DIR_INSTRUMENTS);
	TrackerSettings::Instance().SetDefaultDirectory(szVstDir, DIR_PLUGINS);
	TrackerSettings::Instance().SetDefaultDirectory(szPresetDir, DIR_PLUGINPRESETS);
	return TRUE;
}

BOOL CMainFrame::SetupMiscOptions()
//---------------------------------
{
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_MUTECHNMODE)
		CSoundFile::gdwSoundSetup |= SNDMIX_MUTECHNMODE;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_MUTECHNMODE;

	m_wndToolBar.EnableFlatButtons(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS);

	UpdateTree(NULL, HINT_MPTOPTIONS);
	UpdateAllViews(HINT_MPTOPTIONS, NULL);
	return true;
}


BOOL CMainFrame::SetupMidi(DWORD d, LONG n)
//-----------------------------------------
{
	TrackerSettings::Instance().m_dwMidiSetup = d;
	TrackerSettings::Instance().m_nMidiDevice = n;
	return TRUE;
}


void CMainFrame::UpdateAllViews(DWORD dwHint, CObject *pHint)
//-----------------------------------------------------------
{
	CDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if (pDocTmpl)
	{
		POSITION pos = pDocTmpl->GetFirstDocPosition();
		CDocument *pDoc;
		while ((pos != NULL) && ((pDoc = pDocTmpl->GetNextDoc(pos)) != NULL))
		{
			pDoc->UpdateAllViews(NULL, dwHint, pHint);
		}
	}
}


VOID CMainFrame::SetUserText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szUserText[0])
	{
		strcpy(m_szUserText, lpszText);
		OnUpdateUser(NULL);
	}
}


VOID CMainFrame::SetInfoText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szInfoText[0])
	{
		strcpy(m_szInfoText, lpszText);
		OnUpdateInfo(NULL);
	}
}

//rewbs.xinfo
VOID CMainFrame::SetXInfoText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szXInfoText[0])
	{
		strcpy(m_szXInfoText, lpszText);
		OnUpdateInfo(NULL);
	}
}
//end rewbs.xinfo

VOID CMainFrame::SetHelpText(LPCSTR lpszText)
//-------------------------------------------
{
	m_wndStatusBar.SetPaneText(0, lpszText);
}


VOID CMainFrame::OnDocumentCreated(CModDoc *pModDoc)
//--------------------------------------------------
{
	m_wndTree.OnDocumentCreated(pModDoc);
}


VOID CMainFrame::OnDocumentClosed(CModDoc *pModDoc)
//-------------------------------------------------
{
	if (pModDoc == GetModPlaying()) PauseMod();

	// Make sure that OnTimer() won't try to set the closed document modified anymore.
	if (pModDoc == m_pJustModifiedDoc) m_pJustModifiedDoc = nullptr;

	m_wndTree.OnDocumentClosed(pModDoc);
}


VOID CMainFrame::UpdateTree(CModDoc *pModDoc, DWORD lHint, CObject *pHint)
//------------------------------------------------------------------------
{
	m_wndTree.OnUpdate(pModDoc, lHint, pHint);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnViewOptions()
//------------------------------
{
	if (m_bOptionsLocked)	//rewbs.customKeys
		return;

	CPropertySheet dlg("OpenMPT Setup", this, m_nLastOptionsPage);
	COptionsGeneral general;
	COptionsSoundcard sounddlg(TrackerSettings::Instance().m_dwRate, TrackerSettings::Instance().m_dwSoundSetup, TrackerSettings::Instance().m_nBitsPerSample, TrackerSettings::Instance().m_nChannels, TrackerSettings::Instance().m_LatencyMS, TrackerSettings::Instance().m_UpdateIntervalMS, TrackerSettings::Instance().m_nWaveDevice);
	COptionsKeyboard keyboard;
	COptionsColors colors;
	COptionsPlayer playerdlg;
	CMidiSetupDlg mididlg(TrackerSettings::Instance().m_dwMidiSetup, TrackerSettings::Instance().m_nMidiDevice);
	CEQSetupDlg eqdlg(&TrackerSettings::Instance().m_EqSettings);
	CAutoSaverGUI autosavedlg(m_pAutoSaver); //rewbs.AutoSaver
	CUpdateSetupDlg updatedlg;
	dlg.AddPage(&general);
	dlg.AddPage(&sounddlg);
	dlg.AddPage(&playerdlg);
	dlg.AddPage(&eqdlg);
	dlg.AddPage(&keyboard);
	dlg.AddPage(&colors);
	dlg.AddPage(&mididlg);
	dlg.AddPage(&autosavedlg);
	dlg.AddPage(&updatedlg);
	m_bOptionsLocked=true;	//rewbs.customKeys
	m_SoundCardOptionsDialog = &sounddlg;
	dlg.DoModal();
	m_SoundCardOptionsDialog = nullptr;
	m_bOptionsLocked=false;	//rewbs.customKeys
	m_wndTree.OnOptionsChanged();
}


void CMainFrame::OnSongProperties()
//---------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	if(pModDoc) pModDoc->SongProperties();
}


// -> CODE#0002
// -> DESC="list box to choose VST plugin presets (programs)"
void CMainFrame::OnPluginManager()
//--------------------------------
{
#ifndef NO_VST
	int nPlugslot=-1;
	CModDoc* pModDoc = GetActiveDoc();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		//Find empty plugin slot
		for (PLUGINDEX nPlug = 0; nPlug < MAX_MIXPLUGINS; nPlug++)
		{
			if (pSndFile->m_MixPlugins[nPlug].pMixPlugin == NULL)
			{
				nPlugslot = nPlug;
				break;
			}
		}
	}

	CSelectPluginDlg dlg(GetActiveDoc(), nPlugslot, this);
	if(dlg.DoModal() == IDOK && pModDoc)
	{
		pModDoc->SetModified();
		//Refresh views
		pModDoc->UpdateAllViews(NULL, HINT_MIXPLUGINS|HINT_MODTYPE);
		//Refresh Controls
		CChildFrame *pActiveChild = (CChildFrame *)MDIGetActive();
		pActiveChild->ForceRefresh();
	}
#endif // NO_VST
}
// -! NEW_FEATURE#0002


void CMainFrame::OnChannelManager()
//---------------------------------
{
	if(GetActiveDoc() && CChannelManagerDlg::sharedInstance())
	{
		if(CChannelManagerDlg::sharedInstance()->IsDisplayed())
			CChannelManagerDlg::sharedInstance()->Hide();
		else
		{
			CChannelManagerDlg::sharedInstance()->SetDocument(NULL);
			CChannelManagerDlg::sharedInstance()->Show();
		}
	}
}


void CMainFrame::OnClipboardManager()
//-----------------------------------
{
	PatternClipboardDialog::Show();
}


void CMainFrame::OnAddDlsBank()
//-----------------------------
{
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "dls", "",
		"All Sound Banks|*.dls;*.sbk;*.sf2;*.mss|"
		"Downloadable Sounds Banks (*.dls)|*.dls;*.mss|"
		"SoundFont 2.0 Banks (*.sf2)|*.sbk;*.sf2|"
		"All Files (*.*)|*.*||",
		"",
		true);
	if(files.abort) return;

	BeginWaitCursor();
	for(size_t counter = 0; counter < files.filenames.size(); counter++)
	{
		CTrackApp::AddDLSBank(files.filenames[counter].c_str());
	}
	m_wndTree.RefreshDlsBanks();
	EndWaitCursor();
}


void CMainFrame::OnImportMidiLib()
//--------------------------------
{
	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "", "",
		"Text and INI files (*.txt,*.ini)|*.txt;*.ini;*.dls;*.sf2;*.sbk|"
		"Downloadable Sound Banks (*.dls)|*.dls;*.mss|"
		"SoundFont 2.0 banks (*.sf2)|*.sbk;*.sf2|"
		"Gravis UltraSound (ultrasnd.ini)|ultrasnd.ini|"
		"All Files (*.*)|*.*||");
	if(files.abort) return;

	BeginWaitCursor();
	CTrackApp::ImportMidiConfig(files.first_file.c_str());
	m_wndTree.RefreshMidiLibrary();
	EndWaitCursor();
}


void CMainFrame::OnTimer(UINT)
//----------------------------
{
	// Display Time in status bar
	CSoundFile::samplecount_t time = 0;
	if(m_pSndFile != nullptr && m_pSndFile->GetSampleRate() != 0)
	{
		time = m_pSndFile->GetTotalSampleCount() / m_pSndFile->GetSampleRate();
	}

	if (time != m_dwTimeSec)
	{
		m_dwTimeSec = time;
		m_nAvgMixChn = m_nMixChn;
		OnUpdateTime(NULL);
	}
	// Idle Time Check
	DWORD curTime = timeGetTime();

	m_wndToolBar.SetCurrentSong(m_pSndFile);

	if (m_pAutoSaver && m_pAutoSaver->IsEnabled())
	{
		bool success = m_pAutoSaver->DoSave(curTime);
		if (!success)		// autosave failure; bring up options.
		{
			CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_AUTOSAVE;
			OnViewOptions();
		}
	}

	// Ensure the modified flag gets set in the WinMain thread, even if modification
	// originated from Audio Thread (access to CWnd is not thread safe).
	// Flaw: if 2 docs are modified in between Timer ticks (very rare), one mod will be lost.
	/*CModDoc* pModDoc = GetActiveDoc();
	if (pModDoc && pModDoc->m_bModifiedChanged) {
		pModDoc->SetModifiedFlag(pModDoc->m_bDocModified);
		pModDoc->m_bModifiedChanged=false;
	}*/
	if (m_pJustModifiedDoc)
	{
		m_pJustModifiedDoc->SetModified(true);
		m_pJustModifiedDoc = NULL;
	}

	if(m_SoundCardOptionsDialog)
	{
		m_SoundCardOptionsDialog->UpdateStatistics();
	}

}


CModDoc *CMainFrame::GetActiveDoc()
//---------------------------------
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if (pView) return (CModDoc *)pView->GetDocument();
	}
	return NULL;
}

//rewbs.customKeys
CView *CMainFrame::GetActiveView()
//---------------------------------
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		return pMDIActive->GetActiveView();
	}

	return NULL;
}
//end rewbs.customKeys

void CMainFrame::SwitchToActiveView()
//-----------------------------------
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if (pView)
		{
			pMDIActive->SetActiveView(pView);
			pView->SetFocus();
		}
	}
}


void CMainFrame::OnUpdateTime(CCmdUI *)
//-------------------------------------
{
	CHAR s[64];
	wsprintf(s, "%d:%02d:%02d",
		m_dwTimeSec / 3600, (m_dwTimeSec / 60) % 60, m_dwTimeSec % 60);

	if(m_pSndFile != nullptr && m_pSndFile != &m_WaveFile && !m_pSndFile->IsPaused())
	{
		PATTERNINDEX nPat = m_pSndFile->m_nPattern;
		if(m_pSndFile->Patterns.IsValidIndex(nPat))
		{
			if(nPat < 10) strcat(s, " ");
			if(nPat < 100) strcat(s, " ");
			wsprintf(s + strlen(s), " [%d]", nPat);
		}
		wsprintf(s + strlen(s), " %dch", m_nAvgMixChn);
	}
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_TIME), s, TRUE);
}


void CMainFrame::OnUpdateUser(CCmdUI *)
//-------------------------------------
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_USER), m_szUserText, TRUE);
}


void CMainFrame::OnUpdateInfo(CCmdUI *)
//-------------------------------------
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_INFO), m_szInfoText, TRUE);
}


void CMainFrame::OnUpdateCPU(CCmdUI *)
//-------------------------------------
{
/*	CString s;
	double totalCPUPercent = m_dTotalCPU*100;
	UINT intPart = static_cast<int>(totalCPUPercent);
	UINT decPart = static_cast<int>(totalCPUPercent-intPart)*100;
	s.Format("%d.%d%%", intPart, decPart);
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_CPU), s, TRUE);*/
}

//rewbs.xinfo
void CMainFrame::OnUpdateXInfo(CCmdUI *)
//-------------------------------------
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_XINFO), m_szXInfoText, TRUE);
}
//end rewbs.xinfo

void CMainFrame::OnPlayerPause()
//------------------------------
{
	if (GetModPlaying())
	{
		GetModPlaying()->OnPlayerPause();
	} else
	{
		PauseMod();
	}
}


void CMainFrame::OpenMenuItemFile(const UINT nId, const bool bTemplateFile)
//-------------------------------------------------------------------------
{
	const UINT nIdBegin = (bTemplateFile) ? ID_FILE_OPENTEMPLATE : ID_EXAMPLE_MODULES;
	const std::vector<CString>& vecFilePaths = (bTemplateFile) ? s_TemplateModulePaths : s_ExampleModulePaths;

	const UINT nIndex = nId - nIdBegin;
	if (nIndex < vecFilePaths.size())
	{
		const CString& sPath = vecFilePaths[nIndex];
		const bool bAvailable = Util::sdOs::IsPathFileAvailable(sPath, Util::sdOs::FileModeRead);
		if (bAvailable)
		{
			CDocument* pDoc = theApp.OpenDocumentFile(sPath, bTemplateFile ? FALSE : TRUE);
			if (pDoc != nullptr)
			{
				ASSERT(pDoc->IsKindOf(RUNTIME_CLASS(CModDoc)) == TRUE);
				CModDoc* pModDoc = static_cast<CModDoc*>(pDoc);
				pModDoc->ClearFilePath(); // Clear path so that saving will not take place in templates/examples folder.
				// Remove extension from title, so that saving the file will not suggest a filename like e.g. "template.mptm.mptm".
				const CString title = pModDoc->GetTitle();
				const int dotPos = title.ReverseFind('.');
				if(dotPos >= 0)
				{
					pModDoc->SetTitle(title.Left(dotPos));
				}

				if (bTemplateFile)
				{
					pModDoc->GetFileHistory().clear();	// Reset edit history for template files
					pModDoc->GetSoundFile()->m_dwCreatedWithVersion = MptVersion::num;
					pModDoc->GetSoundFile()->m_dwLastSavedWithVersion = 0;
				}
			}
		}
		else
		{
			const bool bExists = Util::sdOs::IsPathFileAvailable(sPath, Util::sdOs::FileModeExists);
			CString str;
			if (bExists)
				AfxFormatString1(str, IDS_FILE_EXISTS_BUT_IS_NOT_READABLE, (LPCTSTR)sPath);
			else
				AfxFormatString1(str, IDS_FILE_DOES_NOT_EXIST, (LPCTSTR)sPath);
			Reporting::Notification(str);
		}
	}
	else
		ASSERT(false);
}


void CMainFrame::OnOpenTemplateModule(UINT nId)
//---------------------------------------------
{
	OpenMenuItemFile(nId, true/*open template menu file*/);
}


void CMainFrame::OnExampleSong(UINT nId)
//--------------------------------------
{
	OpenMenuItemFile(nId, false/*open example menu file*/);

}


LRESULT CMainFrame::OnInvalidatePatterns(WPARAM wParam, LPARAM)
//-------------------------------------------------------------
{
	UpdateAllViews(wParam, NULL);
	return TRUE;
}


LRESULT CMainFrame::OnUpdatePositionThreaded(WPARAM, LPARAM)
//-----------------------------------------------------------------
{
	MPTNOTIFICATION * pnotify = &m_PendingNotification;
	LRESULT retval = OnUpdatePosition(0, (LPARAM)pnotify);
	// for all notifications which were delivered via the notification thread, release the semaphore
	ReleaseSemaphore(m_PendingNotificationSempahore, 1, NULL);
	return retval;
}

LRESULT CMainFrame::OnUpdatePosition(WPARAM, LPARAM lParam)
//---------------------------------------------------------
{
	MPTNOTIFICATION *pnotify = (MPTNOTIFICATION *)lParam;
	if (pnotify)
	{
		if(pnotify->dwType & MPTNOTIFY_EOS)
		{
			PostMessage(WM_COMMAND, ID_PLAYER_STOP);
		}
		//Log("OnUpdatePosition: row=%d time=%lu\n", pnotify->nRow, pnotify->TimestampSamples);
		if (GetModPlaying())
		{
			m_wndTree.UpdatePlayPos(GetModPlaying(), pnotify);
			if (m_hFollowSong)
				::SendMessage(m_hFollowSong, WM_MOD_UPDATEPOSITION, 0, lParam);
		}
	}
	return 0;
}


void CMainFrame::OnPanic()
//------------------------
{
	// "Panic button." At the moment, it just resets all VSTi and sample notes.
	if(GetModPlaying())
		GetModPlaying()->OnPanic();
}


void CMainFrame::OnPrevOctave()
//-----------------------------
{
	UINT n = GetBaseOctave();
	if (n > MIN_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n-1);
// -> CODE#0009
// -> DESC="instrument editor note play & octave change"
//	SwitchToActiveView();
// -! BEHAVIOUR_CHANGE#0009
}


void CMainFrame::OnNextOctave()
//-----------------------------
{
	UINT n = GetBaseOctave();
	if (n < MAX_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n+1);
// -> CODE#0009
// -> DESC="instrument editor note play & octave change"
//	SwitchToActiveView();
// -! BEHAVIOUR_CHANGE#0009
}


void CMainFrame::OnOctaveChanged()
//--------------------------------
{
	SwitchToActiveView();
}

//rewbs.reportBug
void CMainFrame::OnReportBug()
//----------------------------
{
	CTrackApp::OpenURL("http://bugs.openmpt.org/");
	return;
}
//end rewbs.reportBug

BOOL CMainFrame::OnInternetLink(UINT nID)
//---------------------------------------
{
	LPCSTR pszURL = nullptr;

	switch(nID)
	{
	case ID_NETLINK_MODPLUG:	pszURL = "http://openmpt.org/"; break;
	case ID_NETLINK_TOP_PICKS:	pszURL = "http://openmpt.org/top_picks"; break;
	}
	if(pszURL != nullptr)
	{
		return CTrackApp::OpenURL(pszURL) ? TRUE : FALSE;
	}
	return FALSE;
}


void CMainFrame::OnRButtonDown(UINT, CPoint pt)
//---------------------------------------------
{
	CMenu Menu;

	ClientToScreen(&pt);
	if (Menu.LoadMenu(IDR_TOOLBARS))
	{
		CMenu* pSubMenu = Menu.GetSubMenu(0);
		if (pSubMenu!=NULL) pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,this);
	}
}


LRESULT CMainFrame::OnSpecialKey(WPARAM /*vKey*/, LPARAM)
//---------------------------------------------------
{
/*	CMDIChildWnd *pMDIActive = MDIGetActive();
	CView *pView = NULL;
	if (pMDIActive) pView = pMDIActive->GetActiveView();
	switch(vKey)
	{
	case VK_RMENU:
		if (pView) pView->PostMessage(WM_COMMAND, ID_PATTERN_RESTART, 0);
		break;
	case VK_RCONTROL:
		if (pView) pView->PostMessage(WM_COMMAND, ID_PLAYER_PLAY, 0);
		break;
	}
*/
	return 0;
}

//rewbs.customKeys
LRESULT CMainFrame::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;

	switch(wParam)
	{
		case kcViewTree: OnBarCheck(IDD_TREEVIEW); break;
		case kcViewOptions: OnViewOptions(); break;
		case kcViewMain: OnBarCheck(59392 /* MAINVIEW */); break;
	 	case kcFileImportMidiLib: OnImportMidiLib(); break;
		case kcFileAddSoundBank: OnAddDlsBank(); break;
		case kcPauseSong:	OnPlayerPause(); break;
		case kcPrevOctave:	OnPrevOctave(); break;
		case kcNextOctave:	OnNextOctave(); break;
		case kcFileNew:		theApp.OnFileNew(); break;
		case kcFileOpen:	theApp.OnFileOpen(); break;
		case kcMidiRecord:	OnMidiRecord(); break;
		case kcHelp: 		OnHelp(); break;
		case kcViewAddPlugin: OnPluginManager(); break;
		case kcViewChannelManager: OnChannelManager(); break;
		case kcViewMIDImapping: OnViewMIDIMapping(); break;
		case kcViewEditHistory:	OnViewEditHistory(); break;
		case kcNextDocument:	MDINext(); break;
		case kcPrevDocument:	MDIPrev(); break;
		case kcFileCloseAll:	theApp.OnFileCloseAll(); break;


		//D'oh!! moddoc isn't a CWnd so we have to handle its messages and pass them on.

		case kcFileSaveAs:
		case kcFileSaveAsWave:
		case kcFileSaveAsMP3:
		case kcFileSaveMidi:
		case kcFileExportCompat:
		case kcFileClose:
		case kcFileSave:
		case kcViewGeneral:
		case kcViewPattern:
		case kcViewSamples:
		case kcViewInstruments:
		case kcViewComments:
		case kcViewGraph: //rewbs.graph
		case kcViewSongProperties:
		case kcPlayPatternFromCursor:
		case kcPlayPatternFromStart:
		case kcPlaySongFromCursor:
		case kcPlaySongFromStart:
		case kcPlayPauseSong:
		case kcPlaySongFromPattern:
		case kcStopSong:
		case kcEstimateSongLength:
		case kcApproxRealBPM:
			{	CModDoc* pModDoc = GetActiveDoc();
				if (pModDoc)
					return GetActiveDoc()->OnCustomKeyMsg(wParam, lParam);
				else if(wParam == kcPlayPauseSong  || wParam == kcStopSong)
					StopPreview();
				break;
			}

		//if handled neither by MainFrame nor by ModDoc...
		default:
			//If the treeview has focus, post command to the tree view
			if (m_bModTreeHasFocus)
				return m_wndTree.PostMessageToModTree(WM_MOD_KEYCOMMAND, wParam, lParam);
			if (m_pNoteMapHasFocus)
				return m_pNoteMapHasFocus->PostMessage(WM_MOD_KEYCOMMAND, wParam, lParam);
			if (m_pOrderlistHasFocus)
				return m_pOrderlistHasFocus->PostMessage(WM_MOD_KEYCOMMAND, wParam, lParam);

			//Else send it to the active view
			CView* pView = GetActiveView();
			if (pView)
				return pView->PostMessage(WM_MOD_KEYCOMMAND, wParam, lParam);
	}

	return wParam;
}
//end rewbs.customKeys

void CMainFrame::OnInitMenu(CMenu* pMenu)
//---------------------------------------
{
	m_InputHandler->SetModifierMask(0);
	if (m_InputHandler->noAltMenu())
		return;

	CMDIFrameWnd::OnInitMenu(pMenu);

}

//end rewbs.VSTTimeInfo
long CMainFrame::GetSampleRate()
//------------------------------
{
	return CSoundFile::GetSampleRate();
}


double CMainFrame::GetApproxBPM()
//-------------------------------
{
	CSoundFile *pSndFile = NULL;

	pSndFile = GetActiveDoc()->GetSoundFile();
	if (pSndFile)
	{
		pSndFile->RecalculateSamplesPerTick();
		return pSndFile->GetCurrentBPM();
	}
	return 0;
}

BOOL CMainFrame::InitRenderer(CSoundFile* pSndFile)
//-------------------------------------------------
{
	CriticalSection cs;
	pSndFile->m_bIsRendering=true;
	pSndFile->SuspendPlugins();
	pSndFile->ResumePlugins();
	return true;
}

BOOL CMainFrame::StopRenderer(CSoundFile* pSndFile)
//-------------------------------------------------
{
	CriticalSection cs;
	pSndFile->SuspendPlugins();
	pSndFile->m_bIsRendering=false;
	return true;
}
//end rewbs.VSTTimeInfo

//rewbs.customKeys
// We have swicthed focus to a new module - might need to update effect keys to reflect module type
bool CMainFrame::UpdateEffectKeys()
//---------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	if (pModDoc)
	{
		CSoundFile* pSndFile = pModDoc->GetSoundFile();
		if (pSndFile)
		{
			return m_InputHandler->SetEffectLetters(pSndFile->GetModSpecifications());
		}
	}

	return false;
}
//end rewbs.customKeys


//rewbs.fix3116
void CMainFrame::OnKillFocus(CWnd* pNewWnd)
//-----------------------------------------
{
	CMDIFrameWnd::OnKillFocus(pNewWnd);

	//rewbs: ensure modifiers are reset when we leave the window (e.g. alt-tab)
	CMainFrame::GetMainFrame()->GetInputHandler()->SetModifierMask(0);
	//end rewbs
}
//end rewbs.fix3116

void CMainFrame::OnShowWindow(BOOL bShow, UINT /*nStatus*/)
//---------------------------------------------------------
{
	static bool firstShow = true;
	if (bShow && !IsWindowVisible() && firstShow)
	{
		firstShow = false;
		WINDOWPLACEMENT wpl;
		if (GetPrivateProfileStruct("Display", "WindowPlacement", &wpl, sizeof(WINDOWPLACEMENT), theApp.GetConfigFileName()))
		{
			SetWindowPlacement(&wpl);
		}
	}
}


void CMainFrame::OnViewMIDIMapping()
//----------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	CSoundFile* pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : nullptr;
	if(!pSndFile) return;

	const HWND oldMIDIRecondWnd = GetMidiRecordWnd();
	CMIDIMappingDialog dlg(this, *pSndFile);
	dlg.DoModal();
	SetMidiRecordWnd(oldMIDIRecondWnd);
}


void CMainFrame::OnViewEditHistory()
//----------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	if(pModDoc != nullptr)
	{
		pModDoc->OnViewEditHistory();
	}
}


void CMainFrame::OnInternetUpdate()
//---------------------------------
{
	CUpdateCheck *updateCheck = CUpdateCheck::Create(false);
	updateCheck->DoUpdateCheck();
}


void CMainFrame::OnShowSettingsFolder()
//-------------------------------------
{
	theApp.OpenDirectory(theApp.GetConfigPath());
}


void CMainFrame::OnHelp()
//-----------------------
{
	CString helpFile = theApp.GetAppDirPath();
	helpFile += "OpenMPT Manual.pdf";
	if(!theApp.OpenFile(helpFile))
	{
		helpFile = "Could not find help file:\n" + helpFile;
		Reporting::Error(helpFile);
	}
}


HMENU CMainFrame::CreateFileMenu(const size_t nMaxCount, std::vector<CString>& vPaths, const LPCTSTR pszFolderName, const uint16 nIdRangeBegin)
//---------------------------------------------------------------------------------------------------------------------------------------------
{
	vPaths.clear();
	HMENU hMenu = ::CreatePopupMenu();
	ASSERT(hMenu != NULL);
	if (hMenu != NULL)
	{
		UINT_PTR nAddCounter = 0;
		for(size_t i = 0; i < 2; i++) // 0: app items, 1: user items
		{
			// To avoid duplicates, check whether app path and config path are the same.
			if (i == 1 && _tcsicmp(CTrackApp::GetAppDirPath(), theApp.GetConfigPath()) == 0)
				break;
			CFileFind fileFind;
			CFixedStringT<CString, MAX_PATH> sPath;
			sPath = (i == 0) ? CTrackApp::GetAppDirPath() : theApp.GetConfigPath();
			sPath += pszFolderName;
			if (Util::sdOs::IsPathFileAvailable(sPath, Util::sdOs::FileModeExists) == false)
				continue;
			sPath += _T("*");

			BOOL bWorking = fileFind.FindFile(sPath);
			// Note: The order in which the example files appears in the menu is unspecified.
			while (bWorking && nAddCounter < nMaxCount)
			{
				bWorking = fileFind.FindNextFile();
				const CString fn = fileFind.GetFileName();
				if (fileFind.IsDirectory() == FALSE)
				{
					vPaths.push_back(fileFind.GetFilePath());
					AppendMenu(hMenu, MF_STRING, nIdRangeBegin + nAddCounter, fileFind.GetFileName());
					++nAddCounter;
				}
			}
			fileFind.Close();
		}

		if (nAddCounter == 0)
			AppendMenu(hMenu, MF_STRING | MF_GRAYED | MF_DISABLED, 0, _T("No items found"));
	}

	return hMenu;
}


void CMainFrame::CreateExampleModulesMenu()
//-----------------------------------------
{
	static_assert(nMaxItemsInExampleModulesMenu == ID_EXAMPLE_MODULES_LASTINRANGE - ID_EXAMPLE_MODULES + 1,
				  "Make sure that there's a proper range for menu commands in resources.");
	HMENU hMenu = CreateFileMenu(nMaxItemsInExampleModulesMenu, s_ExampleModulePaths, _T("ExampleSongs\\"), ID_EXAMPLE_MODULES);
	CMenu* const pMainMenu = GetMenu();
	if (hMenu && pMainMenu && m_InputHandler)
		VERIFY(pMainMenu->ModifyMenu(ID_EXAMPLE_MODULES, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)hMenu, m_InputHandler->GetMenuText(ID_EXAMPLE_MODULES)));
	else
		ASSERT(false);
}


void CMainFrame::CreateTemplateModulesMenu()
//------------------------------------------
{
	static_assert(nMaxItemsInTemplateModulesMenu == ID_FILE_OPENTEMPLATE_LASTINRANGE - ID_FILE_OPENTEMPLATE + 1,
				  "Make sure that there's a proper range for menu commands in resources.");
	HMENU hMenu = CreateFileMenu(nMaxItemsInTemplateModulesMenu, s_TemplateModulePaths, _T("TemplateModules\\"), ID_FILE_OPENTEMPLATE);
	CMenu* const pMainMenu = GetMenu();
	CMenu* pFileMenu = (pMainMenu) ? pMainMenu->GetSubMenu(0) : nullptr;
	if (hMenu && pFileMenu && m_InputHandler)
	{
		if (pFileMenu->GetMenuItemID(1) != ID_FILE_OPEN)
			pFileMenu = pMainMenu->GetSubMenu(1);
		ASSERT(pFileMenu->GetMenuItemID(1) == ID_FILE_OPEN);
		VERIFY(pFileMenu->RemoveMenu(2, MF_BYPOSITION));
		VERIFY(pFileMenu->InsertMenu(2, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hMenu, m_InputHandler->GetMenuText(ID_FILE_OPENTEMPLATE)));
	}
	else
		ASSERT(false);
}


/////////////////////////////////////////////
//Misc helper functions
/////////////////////////////////////////////

void AddPluginNamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN* plugarray, const bool librarynames)
//----------------------------------------------------------------------------------------------
{
	for (PLUGINDEX iPlug = 0; iPlug < MAX_MIXPLUGINS; iPlug++)
	{
		const SNDMIXPLUGIN &plugin = plugarray[iPlug];
		CString str;
		str.Preallocate(80);
		str.Format(_T("FX%d: "), iPlug + 1);
		const int size0 = str.GetLength();
		str += (librarynames) ? plugin.GetLibraryName() : plugin.GetName();
		if(str.GetLength() <= size0) str += _T("undefined");

		CBox.SetItemData(CBox.AddString(str), iPlug + 1);
	}
}


void AddPluginParameternamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN& plug)
//-------------------------------------------------------------------------
{
	if(plug.pMixPlugin)
		AddPluginParameternamesToCombobox(CBox, *(CVstPlugin *)plug.pMixPlugin);
}


void AddPluginParameternamesToCombobox(CComboBox& CBox, CVstPlugin& plug)
//-----------------------------------------------------------------------
{
	const PlugParamIndex nParams = plug.GetNumParameters();
	for (PlugParamIndex i = 0; i < nParams; i++)
	{
		CBox.SetItemData(CBox.AddString(plug.GetFormattedParamName(i)), i);
	}
}
