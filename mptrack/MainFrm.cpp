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


//========================================
class CMPTSoundSource: public ISoundSource
//========================================
{
public:
	CMPTSoundSource() {}
	ULONG AudioRead(PVOID pData, ULONG cbSize);
	VOID AudioDone(ULONG dwSize, ULONG dwLatency);
};


CMPTSoundSource gMPTSoundSource;

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

// Static
static int gdwLastLowLatencyTime = 0;
static int gdwLastMixActiveTime = 0;
static DWORD gsdwTotalSamples = 0;
static DWORD gdwPlayLatency = 0;

// Globals
DWORD CMainFrame::gdwNotificationType = MPTNOTIFY_DEFAULT;
UINT CMainFrame::m_nLastOptionsPage = 0;
HHOOK CMainFrame::ghKbdHook = NULL;

std::vector<CString> CMainFrame::s_ExampleModulePaths;
std::vector<CString> CMainFrame::s_TemplateModulePaths;

HANDLE CMainFrame::m_hPlayThread = NULL;
DWORD CMainFrame::m_dwPlayThreadId = 0;
HANDLE CMainFrame::m_hAudioWakeUp = NULL;
HANDLE CMainFrame::m_hNotifyThread = NULL;
DWORD CMainFrame::m_dwNotifyThreadId = 0;
HANDLE CMainFrame::m_hNotifyWakeUp = NULL;
ISoundDevice *CMainFrame::gpSoundDevice = NULL;
LONG CMainFrame::slSampleSize = 2;
LONG CMainFrame::sdwSamplesPerSec = 44100;
LONG CMainFrame::sdwAudioBufferSize = MAX_AUDIO_BUFFERSIZE;
UINT CMainFrame::gdwIdleTime = 0;
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
	m_bModTreeHasFocus = false;	//rewbs.customKeys
	m_pNoteMapHasFocus = nullptr;	//rewbs.customKeys
	m_pOrderlistHasFocus = nullptr;
	m_bOptionsLocked = false;	//rewbs.customKeys

	m_pJustModifiedDoc = nullptr;
	m_pModPlaying = nullptr;
	m_hFollowSong = NULL;
	m_hWndMidi = NULL;
	m_pSndFile = nullptr;
	m_dwStatus = 0;
	m_dwTimeSec = 0;
	m_dwNotifyType = 0;
	m_nTimer = 0;
	m_nAvgMixChn = m_nMixChn = 0;
	m_szUserText[0] = 0;
	m_szInfoText[0] = 0;
	m_szXInfoText[0]= 0;	//rewbs.xinfo

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

	// Check for valid sound device
	if (!EnumerateSoundDevices(SNDDEV_GET_TYPE(TrackerSettings::Instance().m_nWaveDevice), SNDDEV_GET_NUMBER(TrackerSettings::Instance().m_nWaveDevice), nullptr, 0))
	{
		TrackerSettings::Instance().m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_DSOUND);
		if (!EnumerateSoundDevices(SNDDEV_GET_TYPE(TrackerSettings::Instance().m_nWaveDevice), SNDDEV_GET_NUMBER(TrackerSettings::Instance().m_nWaveDevice), nullptr, 0))
		{
			TrackerSettings::Instance().m_nWaveDevice = SNDDEV_BUILD_ID(0, SNDDEV_WAVEOUT);
		}
	}

	// Create Audio Thread
	m_hAudioWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hNotifyWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hPlayThread = CreateThread(NULL, 0, AudioThread, NULL, 0, &m_dwPlayThreadId);
	m_hNotifyThread = CreateThread(NULL, 0, NotifyThread, NULL, 0, &m_dwNotifyThreadId);
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
	if (m_hPlayThread != NULL)
	{
		if(TerminateThread(m_hPlayThread, 0)) m_hPlayThread = NULL;
	}
	if (m_hNotifyThread != NULL)
	{
		if(TerminateThread(m_hNotifyThread, 0)) m_hNotifyThread = NULL;
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
	if (m_dwStatus & MODSTATUS_PLAYING) PauseMod();
	if (pMDIActive) pMDIActive->SavePosition(TRUE);
	if (gpSoundDevice)
	{
		CriticalSection cs;
		//gpSoundDevice->Reset();
		//audioCloseDevice();
		gpSoundDevice->Release();
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

static BOOL gbStopSent = FALSE;

// Sound Device Callback
BOOL SoundDeviceCallback(DWORD dwUser)
//------------------------------------
{
	BOOL bOk = FALSE;
	CMainFrame *pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
	if (gbStopSent) return FALSE;
	CriticalSection cs;
	if ((pMainFrm) && (pMainFrm->IsPlaying()) && (CMainFrame::gpSoundDevice))
	{
		bOk = CMainFrame::gpSoundDevice->FillAudioBuffer(&gMPTSoundSource, gdwPlayLatency, dwUser);
	}
	if (!bOk)
	{
		gbStopSent = TRUE;
		pMainFrm->PostMessage(WM_COMMAND, ID_PLAYER_STOP);
	}
	return bOk;
}


// Audio thread
DWORD WINAPI CMainFrame::AudioThread(LPVOID)
//------------------------------------------
{
	CMainFrame *pMainFrm;
	BOOL bWait;
	UINT nSleep;

	ExceptionHandler::RegisterAudioThread();

// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
	HANDLE sleepEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
// -! BEHAVIOUR_CHANGE#0021

	bWait = TRUE;
	nSleep = 50;
// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
//rewbs: reduce to normal priority during debug for easier hang debugging
#ifdef NDEBUG
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#endif
#ifdef _DEBUG
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif
//	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
// -! BEHAVIOUR_CHANGE#0021
	for (;;)
	{
		if (bWait)
		{
			WaitForSingleObject(CMainFrame::m_hAudioWakeUp, 250);
		} else
		{
// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
//			Sleep(nSleep);
			timeSetEvent(nSleep,1,(LPTIMECALLBACK)sleepEvent,NULL,TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
			WaitForSingleObject(sleepEvent,nSleep);
			ResetEvent(sleepEvent);
// -! BEHAVIOUR_CHANGE#0021
		}
		bWait = TRUE;
		pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
		CriticalSection cs;
		if ((!gbStopSent) && (pMainFrm) && (pMainFrm->IsPlaying()) && (CMainFrame::gpSoundDevice))
		{
			if (!CMainFrame::gpSoundDevice->Directcallback())
			{
				if (CMainFrame::gpSoundDevice->FillAudioBuffer(&gMPTSoundSource, gdwPlayLatency))
				{
					ULONG nMaxSleep = CMainFrame::gpSoundDevice->GetMaxFillInterval();
					bWait = FALSE;
					nSleep = TrackerSettings::Instance().m_nBufferLength / 8;
					if (nSleep > nMaxSleep) nSleep = nMaxSleep;
					if (nSleep < 10) nSleep = 10;
					if (nSleep > 40) nSleep = 40;
				} else
				{
					gbStopSent = TRUE;
					pMainFrm->PostMessage(WM_COMMAND, ID_PLAYER_STOP);
				}
			} else
			{
				nSleep = 50;
			}
		}
	}

// -> CODE#0021
// -> DESC="use multimedia timer instead of Sleep() in audio thread"
	// Commented as this caused "warning C4702: unreachable code"
	//CloseHandle(sleepEvent);
// -! BEHAVIOUR_CHANGE#0021

	// Commented the two lines below as those caused "warning C4702: unreachable code"
	//ExitThread(0);
	//return 0;
}


// Notify thread
DWORD WINAPI CMainFrame::NotifyThread(LPVOID)
//-------------------------------------------
{
	CMainFrame *pMainFrm;

	ExceptionHandler::RegisterNotifyThread();

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	for (;;)
	{
		WaitForSingleObject(CMainFrame::m_hNotifyWakeUp, 1000);
		pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
		if ((pMainFrm) && (pMainFrm->IsPlaying()))
		{
			MPTNOTIFICATION *pnotify = NULL;
			DWORD dwLatency = 0;

			for (UINT i=0; i<MAX_UPDATE_HISTORY; i++)
			{
				MPTNOTIFICATION *p = &pMainFrm->NotifyBuffer[i];
				if ((p->dwType & MPTNOTIFY_PENDING)
				 && (!(pMainFrm->m_dwStatus & MODSTATUS_BUSY)))
				{
					if (p->dwLatency >= dwLatency)
					{
						if (pnotify) pnotify->dwType = 0;
						pnotify = p;
					} else
					{
						p->dwType = 0;
					}
				}
			}
			if (pnotify)
			{
				pMainFrm->m_dwStatus |= MODSTATUS_BUSY;
				pMainFrm->PostMessage(WM_MOD_UPDATEPOSITION, 0, (LPARAM)pnotify);
			}
		}
	}
	// Commented the two lines below as those caused "warning C4702: unreachable code"
	//ExitThread(0);
	//return 0;
}




ULONG CMPTSoundSource::AudioRead(PVOID pData, ULONG cbSize)
//---------------------------------------------------------
{
	CMainFrame *pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
	if (pMainFrm) return pMainFrm->AudioRead(pData, cbSize);
	return 0;
}


VOID CMPTSoundSource::AudioDone(ULONG nBytesWritten, ULONG nLatency)
//------------------------------------------------------------------
{
	CMainFrame *pMainFrm = (CMainFrame *)theApp.m_pMainWnd;
	if (pMainFrm) pMainFrm->AudioDone(nBytesWritten, nLatency);
}



ULONG CMainFrame::AudioRead(PVOID pvData, ULONG ulSize)
//-----------------------------------------------------
{
	if ((IsPlaying()) && (m_pSndFile))
	{
		DWORD dwSamplesRead = m_pSndFile->Read(pvData, ulSize);
		//m_dTotalCPU = m_pPerfCounter->StartStop()/(static_cast<double>(dwSamplesRead)/m_dwRate);
		return dwSamplesRead * slSampleSize;
	}
	return 0;
}


VOID CMainFrame::AudioDone(ULONG nBytesWritten, ULONG nLatency)
//-------------------------------------------------------------
{
	if (nBytesWritten > (DWORD)slSampleSize)
	{
		DoNotification(nBytesWritten/CMainFrame::slSampleSize, nLatency);
	}
}


LONG CMainFrame::audioTryOpeningDevice(UINT channels, UINT bits, UINT samplespersec)
//----------------------------------------------------------------------------------
{
	WAVEFORMATEXTENSIBLE WaveFormat;
	UINT buflen = TrackerSettings::Instance().m_nBufferLength;

	if (!m_pSndFile) return -1;
	slSampleSize = (bits/8) * channels;
	sdwAudioBufferSize = ((samplespersec * buflen) / 1000) * slSampleSize;
	sdwAudioBufferSize = (sdwAudioBufferSize + 0x0F) & ~0x0F;
	if (sdwAudioBufferSize < MIN_AUDIO_BUFFERSIZE) sdwAudioBufferSize = MIN_AUDIO_BUFFERSIZE;
	if (sdwAudioBufferSize > MAX_AUDIO_BUFFERSIZE) sdwAudioBufferSize = MAX_AUDIO_BUFFERSIZE;
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.Format.nChannels = (unsigned short) channels;
	WaveFormat.Format.nSamplesPerSec = samplespersec;
	WaveFormat.Format.nAvgBytesPerSec = samplespersec * slSampleSize;
	WaveFormat.Format.nBlockAlign = (unsigned short) slSampleSize;
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
		default:	WaveFormat.dwChannelMask = 0; break;
		}
		WaveFormat.SubFormat = guid_MEDIASUBTYPE_PCM;
	}
	if (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_STREVERSE) CSoundFile::gdwSoundSetup |= SNDMIX_REVERSESTEREO;
	else CSoundFile::gdwSoundSetup &= ~SNDMIX_REVERSESTEREO;
	m_pSndFile->SetWaveConfig(samplespersec, bits, channels, (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) ? TRUE : FALSE);
	// Maybe we failed because someone is playing sound already.
	// Shut any sound off, and try once more before giving up.
	UINT nDevType = SNDDEV_GET_TYPE(TrackerSettings::Instance().m_nWaveDevice);
	UINT nDevNo = SNDDEV_GET_NUMBER(TrackerSettings::Instance().m_nWaveDevice);
	UINT fulOptions = (TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_SECONDARY) ? SNDDEV_OPTIONS_SECONDARY : 0;
	if ((gpSoundDevice) && (gpSoundDevice->GetDeviceType() != nDevType))
	{
		gpSoundDevice->Release();
		gpSoundDevice = NULL;
	}
	if (!gpSoundDevice)
	{
		if (!CreateSoundDevice(nDevType, &gpSoundDevice)) return -1;
	}
	gpSoundDevice->Configure(m_hWnd, NUM_AUDIO_BUFFERS, TrackerSettings::Instance().m_nBufferLength, fulOptions);
	gbStopSent = FALSE;
	m_pSndFile->SetResamplingMode(TrackerSettings::Instance().m_nSrcMode);
	m_pSndFile->UPDATEDSPEFFECTS();
	m_pSndFile->SetAGC(TrackerSettings::Instance().m_dwQuality & QUALITY_AGC);
	if (!gpSoundDevice->Open(nDevNo, &WaveFormat.Format)) return -1;
	return 0;
}


BOOL CMainFrame::audioOpenDevice()
//--------------------------------
{
	UINT nFixedBitsPerSample;
	LONG err;

	if ((!m_pSndFile) || (!m_pSndFile->GetType())) return FALSE;
	if (m_dwStatus & MODSTATUS_PLAYING) return TRUE;
	if (!TrackerSettings::Instance().m_dwRate) TrackerSettings::Instance().m_dwRate = 22050;
	if ((TrackerSettings::Instance().m_nChannels != 1) && (TrackerSettings::Instance().m_nChannels != 2) && (TrackerSettings::Instance().m_nChannels != 4)) TrackerSettings::Instance().m_nChannels = 2;
	err = audioTryOpeningDevice(TrackerSettings::Instance().m_nChannels,
								TrackerSettings::Instance().m_nBitsPerSample,
								TrackerSettings::Instance().m_dwRate);
	nFixedBitsPerSample = (gpSoundDevice) ? gpSoundDevice->HasFixedBitsPerSample() : 0;
	if ((err) && ((TrackerSettings::Instance().m_dwRate > 44100) || (TrackerSettings::Instance().m_nChannels > 2) || (TrackerSettings::Instance().m_nBitsPerSample > 16)
			   || ((nFixedBitsPerSample) && (nFixedBitsPerSample != TrackerSettings::Instance().m_nBitsPerSample))))
	{
		DWORD oldrate = TrackerSettings::Instance().m_dwRate;

		TrackerSettings::Instance().m_dwRate = 44100;
		if (TrackerSettings::Instance().m_nChannels > 2) TrackerSettings::Instance().m_nChannels = 2;
		if (nFixedBitsPerSample) TrackerSettings::Instance().m_nBitsPerSample = nFixedBitsPerSample;
		else if (TrackerSettings::Instance().m_nBitsPerSample > 16) TrackerSettings::Instance().m_nBitsPerSample = 16;
		err = audioTryOpeningDevice(TrackerSettings::Instance().m_nChannels,
									TrackerSettings::Instance().m_nBitsPerSample,
									TrackerSettings::Instance().m_dwRate);
		if (err) TrackerSettings::Instance().m_dwRate = oldrate;
	}
	// Display error message box
	if (err != 0)
	{
		Reporting::Error("Unable to open sound device!");
		return FALSE;
	}
	// Device is ready
	gdwLastMixActiveTime = timeGetTime();
	return TRUE;
}


void CMainFrame::audioCloseDevice()
//---------------------------------
{
	if (gpSoundDevice)
	{
		CriticalSection cs;

		gpSoundDevice->Reset();
		gpSoundDevice->Close();
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


BOOL CMainFrame::DoNotification(DWORD dwSamplesRead, DWORD dwLatency)
//-------------------------------------------------------------------
{
	gsdwTotalSamples += dwSamplesRead;
	if (!m_pSndFile) return FALSE;
	if (m_nMixChn < m_pSndFile->m_nMixStat) m_nMixChn++;
	if (m_nMixChn > m_pSndFile->m_nMixStat) m_nMixChn--;
	if (!(m_dwNotifyType & MPTNOTIFY_TYPEMASK)) return FALSE;
	// Notify Client
	for (UINT i=0; i<MAX_UPDATE_HISTORY; i++)
	{
		MPTNOTIFICATION *p = &NotifyBuffer[i];
		if ((p->dwType & MPTNOTIFY_TYPEMASK)
		 && (!(p->dwType & MPTNOTIFY_PENDING))
		 && (gsdwTotalSamples >= p->dwLatency))
		{
			p->dwType |= MPTNOTIFY_PENDING;
			SetEvent(m_hNotifyWakeUp);
		}
	}
	if (!m_pSndFile) return FALSE;
	// Add an entry to the notification history
	for (UINT j=0; j<MAX_UPDATE_HISTORY; j++)
	{
		MPTNOTIFICATION *p = &NotifyBuffer[j];
		if (!(p->dwType & MPTNOTIFY_TYPEMASK))
		{
			p->dwType = m_dwNotifyType;
			DWORD d = dwLatency / slSampleSize;
			p->dwLatency = gsdwTotalSamples + d;
			p->nOrder = m_pSndFile->m_nCurrentOrder;
			p->nRow = m_pSndFile->m_nRow;
			p->nPattern = m_pSndFile->m_nPattern;
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
			} else
			if (m_dwNotifyType & (MPTNOTIFY_VOLENV|MPTNOTIFY_PANENV|MPTNOTIFY_PITCHENV))
			{
				UINT nIns = m_dwNotifyType & 0xFFFF;
				for (CHANNELINDEX k = 0; k < MAX_CHANNELS; k++)
				{
					ModChannel *pChn = &m_pSndFile->Chn[k];
					p->dwPos[k] = 0;
					if ((nIns) && (nIns <= m_pSndFile->m_nInstruments) && (pChn->nLength)
					 && (pChn->pModInstrument) && (pChn->pModInstrument == m_pSndFile->Instruments[nIns])
					 && ((!(pChn->dwFlags & CHN_NOTEFADE)) || (pChn->nFadeOutVol)))
					{
						enmEnvelopeTypes notifyEnv = ENV_VOLUME;
						if (m_dwNotifyType & MPTNOTIFY_PITCHENV)
							notifyEnv = ENV_PITCH;
						else if (m_dwNotifyType & MPTNOTIFY_PANENV)
							notifyEnv = ENV_PANNING;

						const ModChannelEnvInfo &chnEnv = pChn->GetEnvelope(notifyEnv);

						if(chnEnv.flags[ENV_ENABLED])
						{
							uint32 pos = chnEnv.nEnvPosition;
							if(m_pSndFile->IsCompatibleMode(TRK_IMPULSETRACKER | TRK_FASTTRACKER2))
							{
								// Impulse Tracker / Fasttracker 2 envelope handling (see SndMix.cpp for details)
								if(pos > 0)
									pos--;
								else
									continue;
							}
							p->dwPos[k] = MPTNOTIFY_POSVALID | pos;
						}
					}
				}
			} else
			if (m_dwNotifyType & (MPTNOTIFY_VUMETERS))
			{
				for (UINT k=0; k<MAX_CHANNELS; k++)
				{
					ModChannel *pChn = &m_pSndFile->Chn[k];
					UINT vul = pChn->nLeftVU;
					UINT vur = pChn->nRightVU;
					p->dwPos[k] = (vul << 8) | (vur);
				}
			} else
			if (m_dwNotifyType & MPTNOTIFY_MASTERVU)
			{
				DWORD lVu = (gnLVuMeter >> 11);
				DWORD rVu = (gnRVuMeter >> 11);
				if (lVu > 0x10000) lVu = 0x10000;
				if (rVu > 0x10000) rVu = 0x10000;
				p->dwPos[0] = lVu;
				p->dwPos[1] = rVu;
				DWORD dwVuDecay = Util::muldiv(dwSamplesRead, 120000, TrackerSettings::Instance().m_dwRate) + 1;
				if (lVu >= dwVuDecay) gnLVuMeter = (lVu - dwVuDecay) << 11; else gnLVuMeter = 0;
				if (rVu >= dwVuDecay) gnRVuMeter = (rVu - dwVuDecay) << 11; else gnRVuMeter = 0;
			}
			return TRUE;
		}
	}

	return FALSE;
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


void CMainFrame::EnableLowLatencyMode(BOOL bOn)
//---------------------------------------------
{
	gdwPlayLatency = (bOn) ? sdwAudioBufferSize : 0;
	gdwLastLowLatencyTime = timeGetTime();
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

UINT CMainFrame::GetBaseOctave()
//------------------------------
{
	return m_wndToolBar.GetBaseOctave();
}


void CMainFrame::SetPreAmp(UINT n)
//--------------------------------
{
	TrackerSettings::Instance().m_nPreAmp = n;
	if (m_pSndFile) m_pSndFile->SetMasterVolume(TrackerSettings::Instance().m_nPreAmp, true);
}


BOOL CMainFrame::ResetNotificationBuffer(HWND hwnd)
//-------------------------------------------------
{
	if ((!hwnd) || (m_hFollowSong == hwnd))
	{
		MemsetZero(NotifyBuffer);
		gsdwTotalSamples = 0;
		return TRUE;
	}
	return FALSE;
}


BOOL CMainFrame::PlayMod(CModDoc *pModDoc, HWND hPat, DWORD dwNotifyType)
//-----------------------------------------------------------------------
{
	if (!pModDoc) return FALSE;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if ((!pSndFile) || (!pSndFile->GetType())) return FALSE;
	const bool bPaused = pSndFile->IsPaused();
	const bool bPatLoop = pSndFile->m_SongFlags[SONG_PATTERNLOOP];
	pSndFile->ResetChannels();
	// Select correct bidi loop mode when playing a module.
	pSndFile->SetupITBidiMode();

	if(m_pSndFile != nullptr || (m_dwStatus & MODSTATUS_PLAYING)) PauseMod();
	if(m_pSndFile != nullptr && (pSndFile != m_pSndFile || !m_pSndFile->GetTotalSampleCount())) CSoundFile::ResetAGC();
	m_pSndFile = pSndFile;
	m_pModPlaying = pModDoc;
	m_hFollowSong = hPat;
	m_dwNotifyType = dwNotifyType;
	if (m_dwNotifyType & MPTNOTIFY_MASTERVU)
	{
		gnLVuMeter = gnRVuMeter = 0;
		CSoundFile::gpSndMixHook = CalcStereoVuMeters;
	} else
	{
		CSoundFile::gpSndMixHook = NULL;
	}

	if (!audioOpenDevice())
	{
		m_pSndFile = NULL;
		m_pModPlaying = NULL;
		m_hFollowSong = NULL;
		return FALSE;
	}
	m_nMixChn = m_nAvgMixChn = 0;
	gsdwTotalSamples = 0;
	if (!bPatLoop)
	{
		if (bPaused)
		{
			pSndFile->m_SongFlags.set(SONG_PAUSED);
		} else
		{
			pModDoc->SetPause(FALSE);
		}
	}
	pSndFile->SetRepeatCount((TrackerSettings::Instance().gbLoopSong) ? -1 : 0);

	m_pSndFile->SetMasterVolume(TrackerSettings::Instance().m_nPreAmp, true);
	m_pSndFile->SetMixerSettings(TrackerSettings::Instance().m_MixerSettings);
	m_pSndFile->InitPlayer(TRUE);
	MemsetZero(NotifyBuffer);
	m_dwStatus |= MODSTATUS_PLAYING;
	m_wndToolBar.SetCurrentSong(m_pSndFile);
	if (gpSoundDevice) gpSoundDevice->Start();
	SetEvent(m_hAudioWakeUp);
	return TRUE;
}


BOOL CMainFrame::PauseMod(CModDoc *pModDoc)
//-----------------------------------------
{
	if ((pModDoc) && (pModDoc != m_pModPlaying)) return FALSE;
	if (m_dwStatus & MODSTATUS_PLAYING)
	{
		m_dwStatus &= ~MODSTATUS_PLAYING;

		if (gpSoundDevice) gpSoundDevice->Reset();
		audioCloseDevice();

		{
			CriticalSection cs;
			m_pSndFile->SuspendPlugins(); 	//rewbs.VSTCompliance
		}

		m_nMixChn = m_nAvgMixChn = 0;
		Sleep(1);
		if (m_hFollowSong)
		{
			MPTNOTIFICATION mn;
			MemsetZero(mn);
			mn.dwType = MPTNOTIFY_STOP;
			::SendMessage(m_hFollowSong, WM_MOD_UPDATEPOSITION, 0, (LPARAM)&mn);
		}
	}
	if (m_pModPlaying)
	{
		m_pModPlaying->SetPause(TRUE);
		m_wndTree.UpdatePlayPos(m_pModPlaying, NULL);
	}
	if (m_pSndFile)
	{
		m_pSndFile->m_SongFlags.reset(SONG_PAUSED);
		if (m_pSndFile == &m_WaveFile)
		{
			m_pSndFile = NULL;
			m_WaveFile.Destroy();
		} else
		{
			for(CHANNELINDEX i = m_pSndFile->m_nChannels; i < MAX_CHANNELS; i++)
			{
				if(!(m_pSndFile->Chn[i].nMasterChn))
				{
					m_pSndFile->Chn[i].nPos = m_pSndFile->Chn[i].nPosLo = m_pSndFile->Chn[i].nLength = 0;
				}
			}
		}
	}

	m_pModPlaying = NULL;
	m_pSndFile = NULL;
	m_hFollowSong = NULL;
	m_wndToolBar.SetCurrentSong(NULL);
	return TRUE;
}


BOOL CMainFrame::StopMod(CModDoc *pModDoc)
//----------------------------------------
{
	if ((pModDoc) && (pModDoc != m_pModPlaying)) return FALSE;
	CModDoc *pPlay = m_pModPlaying;
	CSoundFile *pSndFile = m_pSndFile;
	PauseMod();
	if (pPlay) pPlay->SetPause(FALSE);
	if (pSndFile) pSndFile->SetCurrentPos(0);
	return TRUE;
}


BOOL CMainFrame::PlaySoundFile(CSoundFile *pSndFile)
//--------------------------------------------------
{
	if ((!pSndFile) || (!pSndFile->GetType())) return FALSE;
	if (m_pSndFile && m_pSndFile != pSndFile) PauseMod(NULL);
	m_pSndFile = pSndFile;
	if (!audioOpenDevice())
	{
		m_pSndFile = NULL;
		return FALSE;
	}
	gsdwTotalSamples = 0;
	m_pSndFile->SetMasterVolume(TrackerSettings::Instance().m_nPreAmp, true);
	m_pSndFile->SetMixerSettings(TrackerSettings::Instance().m_MixerSettings);
	m_pSndFile->InitPlayer(TRUE);
	if(gpSoundDevice && !(m_dwStatus & MODSTATUS_PLAYING)) gpSoundDevice->Start();
	m_dwStatus |= MODSTATUS_PLAYING;
	SetEvent(m_hAudioWakeUp);
	return TRUE;
}


BOOL CMainFrame::PlayDLSInstrument(UINT nDLSBank, UINT nIns, UINT nRgn, ModCommand::NOTE note)
//--------------------------------------------------------------------------------------------
{
	if(nDLSBank >= CTrackApp::gpDLSBanks.size() || !CTrackApp::gpDLSBanks[nDLSBank]) return FALSE;
	BeginWaitCursor();
	InitPreview();
	if(CTrackApp::gpDLSBanks[nDLSBank]->ExtractInstrument(&m_WaveFile, 1, nIns, nRgn))
	{
		PreparePreview(note);
		PlaySoundFile(&m_WaveFile);
	}
	EndWaitCursor();
	return TRUE;
}


BOOL CMainFrame::PlaySoundFile(LPCSTR lpszFileName, ModCommand::NOTE note)
//------------------------------------------------------------------------
{
	static CString prevFile;
	bool bOk = (prevFile == lpszFileName && m_pSndFile == &m_WaveFile);

	CriticalSection cs;
	if(!bOk && lpszFileName)
	{
		CMappedFile f;

		BeginWaitCursor();
		if(!f.Open(lpszFileName))
		{
			EndWaitCursor();
			return FALSE;
		}

		m_WaveFile.Destroy();
		m_WaveFile.Create(NULL, 0);
		InitPreview();

		DWORD dwLen = f.GetLength();
		if(dwLen)
		{
			LPBYTE p = f.Lock();
			if(p)
			{
				m_WaveFile.m_SongFlags.set(SONG_PAUSED);
				cs.Leave();	// Avoid hanging audio while reading file
				bOk = m_WaveFile.ReadInstrumentFromFile(1, p, dwLen);
				if(!bOk)
				{
					bOk = m_WaveFile.ReadSampleFromFile(1, p, dwLen);
					m_WaveFile.AllocateInstrument(1, 1);
				}
				f.Unlock();
			}
		}
		f.Close();
		EndWaitCursor();
	}

	if(bOk)
	{
		cs.Enter();
		PreparePreview(note);
		bOk = PlaySoundFile(&m_WaveFile) != FALSE;

		prevFile = lpszFileName;
	}
	return bOk;
}


BOOL CMainFrame::PlaySoundFile(CSoundFile *pSong, INSTRUMENTINDEX nInstrument, SAMPLEINDEX nSample, ModCommand::NOTE note)
//------------------------------------------------------------------------------------------------------------------------
{
	CriticalSection cs;
	m_WaveFile.Destroy();
	m_WaveFile.Create(NULL, 0);
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
	return PlaySoundFile(&m_WaveFile);
}


void CMainFrame::InitPreview()
//----------------------------
{
	// Avoid global volume ramping when trying samples in the treeview.
	m_WaveFile.m_pConfig->setGlobalVolumeAppliesToMaster(false);
	m_WaveFile.m_nDefaultGlobalVolume = m_WaveFile.m_nGlobalVolume = MAX_GLOBAL_VOLUME;
	m_WaveFile.m_nDefaultTempo = 125;
	m_WaveFile.m_nDefaultSpeed = 6;
	m_WaveFile.m_nType = MOD_TYPE_IT;
	m_WaveFile.m_nChannels = 4;
	m_WaveFile.m_nInstruments = 1;
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


BOOL CMainFrame::StopSoundFile(CSoundFile *pSndFile)
//--------------------------------------------------
{
	if ((pSndFile) && (pSndFile != m_pSndFile)) return FALSE;
	PauseMod(NULL);
	return TRUE;
}


void CMainFrame::StopPreview()
//----------------------------
{
	StopSoundFile(&m_WaveFile);
	m_WaveFile.Destroy();
}


BOOL CMainFrame::SetFollowSong(CModDoc *pDoc, HWND hwnd, BOOL bFollowSong, DWORD dwType)
//--------------------------------------------------------------------------------------
{
	if ((!pDoc) || (pDoc != m_pModPlaying)) return FALSE;
	if (bFollowSong)
	{
		m_hFollowSong = hwnd;
	} else
	{
		if (hwnd == m_hFollowSong) m_hFollowSong = NULL;
	}
	if (dwType) m_dwNotifyType = dwType;
	if (m_dwNotifyType & MPTNOTIFY_MASTERVU)
	{
		CSoundFile::gpSndMixHook = CalcStereoVuMeters;
	} else
	{
		gnLVuMeter = gnRVuMeter = 0;
		CSoundFile::gpSndMixHook = NULL;
	}
	return TRUE;
}


BOOL CMainFrame::SetupSoundCard(DWORD q, DWORD rate, UINT nBits, UINT nChns, UINT bufsize, LONG wd)
//-------------------------------------------------------------------------------------------------
{
	const bool isPlaying = (m_dwStatus & MODSTATUS_PLAYING) != 0;
	if ((TrackerSettings::Instance().m_dwRate != rate) || ((TrackerSettings::Instance().m_dwSoundSetup & SOUNDSETUP_RESTARTMASK) != (q & SOUNDSETUP_RESTARTMASK))
	 || (TrackerSettings::Instance().m_nWaveDevice != wd) || (TrackerSettings::Instance().m_nBufferLength != bufsize) || (nBits != TrackerSettings::Instance().m_nBitsPerSample)
	 || (TrackerSettings::Instance().m_nChannels != nChns))
	{
		CModDoc *pActiveMod = NULL;
		HWND hFollow = m_hFollowSong;
		if (isPlaying)
		{
			if ((m_pSndFile) && (!m_pSndFile->IsPaused())) pActiveMod = m_pModPlaying;
			PauseMod();
		}
		TrackerSettings::Instance().m_nWaveDevice = wd;
		TrackerSettings::Instance().m_dwRate = rate;
		TrackerSettings::Instance().m_dwSoundSetup = q;
		TrackerSettings::Instance().m_nBufferLength = bufsize;
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
	if (pModDoc == m_pModPlaying) PauseMod();

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
	COptionsSoundcard sounddlg(TrackerSettings::Instance().m_dwRate, TrackerSettings::Instance().m_dwSoundSetup, TrackerSettings::Instance().m_nBitsPerSample, TrackerSettings::Instance().m_nChannels, TrackerSettings::Instance().m_nBufferLength, TrackerSettings::Instance().m_nWaveDevice);
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
	dlg.DoModal();
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


void CMainFrame::SetLastMixActiveTime()		//rewbs.LiveVSTi
//-------------------------------------
{
	gdwLastMixActiveTime = timeGetTime();
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
	if (IsPlaying())
	{
		gdwIdleTime = 0;
		if (curTime - gdwLastLowLatencyTime > 15000)
		{
			gdwPlayLatency = 0;
		}
		if ((m_pSndFile) && (m_pSndFile->IsPaused()) && (!m_pSndFile->m_nMixChannels))
		{
			//Log("%d (%d)\n", dwTime - gdwLastMixActiveTime, gdwLastMixActiveTime);
			if (curTime - gdwLastMixActiveTime > 5000)
			{
				//rewbs.instroVSTi: testing without shutting down audio device after 5s of idle time.
				//PauseMod();
			}
		} else
		{
			gdwLastMixActiveTime = curTime;
		}
	} else
	{
		gdwIdleTime += MPTTIMER_PERIOD;
		if (gdwIdleTime > 15000)
		{
			gdwIdleTime = 0;
			// After 15 seconds of inactivity, we reset the AGC
			CSoundFile::ResetAGC();
			gdwPlayLatency = 0;
		}
	}
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
	if (m_pModPlaying)
	{
		m_pModPlaying->OnPlayerPause();
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


LRESULT CMainFrame::OnUpdatePosition(WPARAM, LPARAM lParam)
//---------------------------------------------------------
{
	MPTNOTIFICATION *pnotify = (MPTNOTIFICATION *)lParam;
	if (pnotify)
	{
		//Log("OnUpdatePosition: row=%d time=%lu\n", pnotify->nRow, pnotify->dwLatency);
		if ((m_pModPlaying) && (m_pSndFile))
		{
			m_wndTree.UpdatePlayPos(m_pModPlaying, pnotify);
			if (m_hFollowSong) ::SendMessage(m_hFollowSong, WM_MOD_UPDATEPOSITION, 0, lParam);
		}
		pnotify->dwType = 0;
	}
	m_dwStatus &= ~MODSTATUS_BUSY;
	return 0;
}


void CMainFrame::OnPanic()
//------------------------
{
	// "Panic button." At the moment, it just resets all VSTi and sample notes.
	if(m_pModPlaying)
		m_pModPlaying->OnPanic();
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
	m_dwStatus |= MODSTATUS_RENDERING;
	m_pModPlaying = GetActiveDoc();
	return true;
}

BOOL CMainFrame::StopRenderer(CSoundFile* pSndFile)
//-------------------------------------------------
{
	CriticalSection cs;
	m_dwStatus &= ~MODSTATUS_RENDERING;
	m_pModPlaying = NULL;
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
