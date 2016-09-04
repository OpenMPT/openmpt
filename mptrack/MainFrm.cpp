/*
 * MainFrm.cpp
 * -----------
 * Purpose: Implementation of OpenMPT's main window code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "MainFrm.h"
#include "../sounddev/SoundDevice.h"
#include "../sounddev/SoundDeviceManager.h"
#include "../soundlib/AudioReadTarget.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "Childfrm.h"
#include "Dlsbank.h"
#include "Mpdlgs.h"
#include "KeyConfigDlg.h"
#include "PathConfigDlg.h"
#include "GeneralConfigDlg.h"
#include "ColorConfigDlg.h"
#include "SampleConfigDlg.h"
#include "AdvancedConfigDlg.h"
#include "AutoSaver.h"
#include "MainFrm.h"
#include "InputHandler.h"
#include "globals.h"
#include "ChannelManagerDlg.h"
#include <direct.h>
#include "../common/version.h"
#include "Ctrl_pat.h"
#include "UpdateCheck.h"
#include "CloseMainDialog.h"
#include "SelectPluginDialog.h"
#include "ExceptionHandler.h"
#include "PatternClipboard.h"
#include "PatternFont.h"
#include "../common/mptFileIO.h"
#include "../common/FileReader.h"
#include "../common/Profiler.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "FileDialog.h"
#include <HtmlHelp.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


OPENMPT_NAMESPACE_BEGIN


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
	ON_WM_DROPFILES()
	ON_COMMAND(ID_VIEW_OPTIONS,				OnViewOptions)

	ON_COMMAND(ID_PLUGIN_SETUP,				OnPluginManager)
	ON_COMMAND(ID_CLIPBOARD_MANAGER,		OnClipboardManager)
	//ON_COMMAND(ID_HELP,					CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_REPORT_BUG,				OnReportBug)
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
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_XINFO,OnUpdateXInfo)
	ON_UPDATE_COMMAND_UI(IDD_TREEVIEW,		OnUpdateControlBarMenu)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		OnUpdatePosition)
	ON_MESSAGE(WM_MOD_INVALIDATEPATTERNS,	OnInvalidatePatterns)
	ON_MESSAGE(WM_MOD_SPECIALKEY,			OnSpecialKey)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,			OnCustomKeyMsg)
	ON_MESSAGE(WM_MOD_MIDIMAPPING,			OnViewMIDIMapping)
	ON_COMMAND(ID_INTERNETUPDATE,			OnInternetUpdate)
	ON_COMMAND(ID_HELP_SHOWSETTINGSFOLDER,	OnShowSettingsFolder)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_PROGRESS, OnUpdateCheckProgress)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_SUCCESS, OnUpdateCheckSuccess)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_FAILURE, OnUpdateCheckFailure)
	ON_COMMAND(ID_HELPSHOW,					OnHelp)

	ON_COMMAND_RANGE(ID_MRU_LIST_FIRST, ID_MRU_LIST_LAST, OnOpenMRUItem)
	ON_UPDATE_COMMAND_UI(ID_MRU_LIST_FIRST,	OnUpdateMRUItem)
	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEWHEEL()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

// Globals
OptionsPage CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_DEFAULT;
HHOOK CMainFrame::ghKbdHook = NULL;

std::vector<mpt::PathString> CMainFrame::s_ExampleModulePaths;
std::vector<mpt::PathString> CMainFrame::s_TemplateModulePaths;

// GDI
HICON CMainFrame::m_hIcon = NULL;
HFONT CMainFrame::m_hGUIFont = NULL;
HFONT CMainFrame::m_hFixedFont = NULL;
HPEN CMainFrame::penDarkGray = NULL;
HPEN CMainFrame::penScratch = NULL;
HPEN CMainFrame::penGray00 = NULL;
HPEN CMainFrame::penGray33 = NULL;
HPEN CMainFrame::penGray40 = NULL;
HPEN CMainFrame::penGray55 = NULL;
HPEN CMainFrame::penGray80 = NULL;
HPEN CMainFrame::penGray99 = NULL;
HPEN CMainFrame::penGraycc = NULL;
HPEN CMainFrame::penGrayff = NULL;
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

HBRUSH CMainFrame::brushHighLight = NULL;
HBRUSH CMainFrame::brushHighLightRed = NULL;
HBRUSH CMainFrame::brushWindow = NULL;
HBRUSH CMainFrame::brushYellow = NULL;
HCURSOR CMainFrame::curDragging = NULL;
HCURSOR CMainFrame::curArrow = NULL;
HCURSOR CMainFrame::curNoDrop = NULL;
HCURSOR CMainFrame::curNoDrop2 = NULL;
HCURSOR CMainFrame::curVSplit = NULL;
MODPLUGDIB *CMainFrame::bmpNotes = nullptr;
MODPLUGDIB *CMainFrame::bmpVUMeters = nullptr;
COLORREF CMainFrame::gcolrefVuMeter[NUM_VUMETER_PENS*2];

CInputHandler *CMainFrame::m_InputHandler = nullptr;

static UINT indicators[] =
{
	ID_SEPARATOR,			// status line indicator
	ID_INDICATOR_XINFO,
	ID_INDICATOR_INFO,
	ID_INDICATOR_USER,
	ID_INDICATOR_TIME,
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction
CMainFrame::CMainFrame()
//----------------------
	: m_SoundDeviceFillBufferCriticalSection(CriticalSection::InitialUnlocked)
	, m_Dither(theApp.BestPRNG())
{
	m_NotifyTimer = 0;
	gpSoundDevice = NULL;

	m_AudioThreadId = 0;
	m_InNotifyHandler = false;

	m_bModTreeHasFocus = false;
	m_pNoteMapHasFocus = nullptr;
	m_pOrderlistHasFocus = nullptr;
	m_bOptionsLocked = false;

	m_SoundCardOptionsDialog = nullptr;

	m_pJustModifiedDoc = nullptr;
	m_hWndMidi = NULL;
	m_pSndFile = nullptr;
	m_dwTimeSec = 0;
	m_nTimer = 0;
	m_nAvgMixChn = m_nMixChn = 0;
	m_szUserText[0] = 0;
	m_szInfoText[0] = 0;
	m_szXInfoText[0]= 0;

	MemsetZero(gcolrefVuMeter);

	m_InputHandler = new CInputHandler(this);

}


void CMainFrame::Initialize()
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
	#ifdef NO_DMO
		title += " NO_DMO";
	#endif
	#ifdef NO_PLUGINS
		title += " NO_PLUGINS";
	#endif
	#ifndef MPT_WITH_ASIO
		title += " NO_ASIO";
	#endif
	#ifndef MPT_WITH_DSOUND
		title += " NO_DSOUND";
	#endif
	SetTitle(title);
	OnUpdateFrameTitle(false);

	// Check for valid sound device
	SoundDevice::Identifier dev = TrackerSettings::Instance().GetSoundDeviceIdentifier();
	if(!theApp.GetSoundDevicesManager()->FindDeviceInfo(dev).IsValid())
	{
		dev = theApp.GetSoundDevicesManager()->FindDeviceInfoBestMatch(dev, TrackerSettings::Instance().m_SoundDevicePreferSameTypeIfDeviceUnavailable).GetIdentifier();
		TrackerSettings::Instance().SetSoundDeviceIdentifier(dev);
	}

	// Setup timer
	OnUpdateUser(NULL);
	m_nTimer = SetTimer(TIMERID_GUI, MPTTIMER_PERIOD, NULL);

	// Setup Keyboard Hook
	ghKbdHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, AfxGetInstanceHandle(), GetCurrentThreadId());

	// Update the tree
	m_wndTree.Init();

	CreateExampleModulesMenu();
	CreateTemplateModulesMenu();
	UpdateMRUList();
}


CMainFrame::~CMainFrame()
//-----------------------
{

	delete m_InputHandler;

	CChannelManagerDlg::DestroySharedInstance();
	CSoundFile::DeleteStaticdata();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
//-----------------------------------------------------
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1) return -1;
	// Load resources
	m_hIcon = theApp.LoadIcon(IDR_MAINFRAME);

	// Toolbar and other icons
	CDC *dc = GetDC();
	m_MiscIcons.Create(IDB_IMAGELIST, 16, 16, IMGLIST_NUMIMAGES, 1, dc);
	m_MiscIconsDisabled.Create(IDB_IMAGELIST, 16, 16, IMGLIST_NUMIMAGES, 1, dc, true);
	m_PatternIcons.Create(IDB_PATTERNS, 16, 16, PATTERNIMG_NUMIMAGES, 1, dc);
	m_PatternIconsDisabled.Create(IDB_PATTERNS, 16, 16, PATTERNIMG_NUMIMAGES, 1, dc, true);
	m_EnvelopeIcons.Create(IDB_ENVTOOLBAR, 20, 18, ENVIMG_NUMIMAGES, 1, dc);
	m_SampleIcons.Create(IDB_SMPTOOLBAR, 20, 18, SAMPLEIMG_NUMIMAGES, 1, dc);
	ReleaseDC(dc);

	m_hGUIFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
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
	curNoDrop = theApp.LoadStandardCursor(IDC_NO);
	curNoDrop2 = theApp.LoadCursor(IDC_NODRAG);
	curVSplit = theApp.LoadCursor(AFX_IDC_HSPLITBAR);
	// bitmaps
	bmpNotes = LoadDib(MAKEINTRESOURCE(IDB_PATTERNVIEW));
	bmpVUMeters = LoadDib(MAKEINTRESOURCE(IDB_VUMETERS));
	// Toolbars
	EnableDocking(CBRS_ALIGN_ANY);
	if (!m_wndToolBar.Create(this)) return -1;
	if (!m_wndStatusBar.Create(this)) return -1;
	if (!m_wndTree.Create(this, IDD_TREEVIEW, CBRS_LEFT|CBRS_BORDER_RIGHT, IDD_TREEVIEW)) return -1;
	m_wndStatusBar.SetIndicators(indicators, CountOf(indicators));
	m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 256);
	m_wndToolBar.Init(this);
	m_wndTree.RecalcLayout();

	RemoveControlBar(&m_wndStatusBar); //Removing statusbar because corrupted statusbar inifiledata can crash the program.
	m_wndTree.EnableDocking(0); //To prevent a crash when there's "Docking=1" for treebar in ini-settings.
	// Restore toobar positions
	LoadBarState("Toolbars");

	AddControlBar(&m_wndStatusBar); //Restore statusbar to mainframe.

	UpdateColors();

	if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_ENABLE_RECORD_DEFAULT) midiOpenDevice(false);

	HtmlHelpW(m_hWnd, nullptr, HH_INITIALIZE, reinterpret_cast<DWORD_PTR>(&helpCookie));

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
	HtmlHelpW(m_hWnd, nullptr, HH_UNINITIALIZE, reinterpret_cast<DWORD_PTR>(&helpCookie));

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
	// Delete bitmaps
	if (bmpNotes)
	{
		delete bmpNotes;
		bmpNotes = nullptr;
	}
	if (bmpVUMeters)
	{
		delete bmpVUMeters;
		bmpVUMeters = nullptr;
	}

	PatternFont::DeleteFontData();

	// Kill GDI Objects
#define DeleteGDIObject(h) ::DeleteObject(h); h = NULL;
	DeleteGDIObject(brushGray);
	DeleteGDIObject(penLightGray);
	DeleteGDIObject(penDarkGray);
	DeleteGDIObject(penSample);
	DeleteGDIObject(penEnvelope);
	DeleteGDIObject(penEnvelopeHighlight);
	DeleteGDIObject(m_hFixedFont);
	DeleteGDIObject(penScratch);
	DeleteGDIObject(penGray00);
	DeleteGDIObject(penGray33);
	DeleteGDIObject(penGray40);
	DeleteGDIObject(penGray55);
	DeleteGDIObject(penGray80);
	DeleteGDIObject(penGray99);
	DeleteGDIObject(penGraycc);
	DeleteGDIObject(penGrayff);
#undef DeleteGDIObject

	return CMDIFrameWnd::DestroyWindow();
}


void CMainFrame::OnClose()
//------------------------
{
	MPT_TRACE();
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

	if(gpSoundDevice)
	{
		gpSoundDevice->Stop();
		gpSoundDevice->Close();
		delete gpSoundDevice;
		gpSoundDevice = nullptr;
	}

	// Save Settings
	RemoveControlBar(&m_wndStatusBar); // Remove statusbar so that its state won't get saved.
	SaveBarState("Toolbars");
	AddControlBar(&m_wndStatusBar); // Restore statusbar to mainframe.
	TrackerSettings::Instance().SaveSettings();

	if(m_InputHandler && m_InputHandler->activeCommandSet)
	{
		m_InputHandler->activeCommandSet->SaveFile(TrackerSettings::Instance().m_szKbdFile);
	}

	EndWaitCursor();
	CMDIFrameWnd::OnClose();
}


// Drop files from Windows
void CMainFrame::OnDropFiles(HDROP hDropInfo)
//-------------------------------------------
{
	const UINT nFiles = ::DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0);
	CMainFrame::GetMainFrame()->SetForegroundWindow();
	for(UINT f = 0; f < nFiles; f++)
	{
		UINT size = ::DragQueryFileW(hDropInfo, f, nullptr, 0) + 1;
		std::vector<WCHAR> fileName(size, L'\0');
		if(::DragQueryFileW(hDropInfo, f, fileName.data(), size))
		{
			const mpt::PathString file = mpt::PathString::FromNative(fileName.data());
			theApp.OpenDocumentFile(file);
		}
	}
	::DragFinish(hDropInfo);
}


LRESULT CALLBACK CMainFrame::KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------------------------
{

	static bool s_KeyboardHookReentryFlag = false; // work-around for https://bugs.openmpt.org/view.php?id=713

	if(mpt::Windows::IsWine()) // work-around for https://bugs.openmpt.org/view.php?id=713
	{
		// TODO: Properly fix this in same way or another.
		if(code < 0)
		{
			return CallNextHookEx(ghKbdHook, code, wParam, lParam); // required by spec
		}
		if(theApp.InGuiThread())
		{
			if(s_KeyboardHookReentryFlag)
			{
				return -1; // exit early without calling further hooks when re-entering
			}
			s_KeyboardHookReentryFlag = true;
		}
	}

	bool skipFurtherProcessing = false;

	if(code >= 0)
	{
		// Check if textbox has focus
		bool textboxHasFocus = false;
		bool handledByTextBox = false;

		HWND hWnd = ::GetFocus();
		if(hWnd != NULL)
		{
			TCHAR activeWindowClassName[6];
			GetClassName(hWnd, activeWindowClassName, CountOf(activeWindowClassName));
			textboxHasFocus = _tcsicmp(activeWindowClassName, _T("Edit")) == 0;
			if(textboxHasFocus)
			{
				handledByTextBox = m_InputHandler->isKeyPressHandledByTextBox(wParam);
			}
		}

		if(!handledByTextBox && m_InputHandler->GeneralKeyEvent(kCtxAllContexts, code, wParam, lParam) != kcNull)
		{
			if(wParam != VK_ESCAPE)
			{
				// We've handled the keypress. No need to take it further.
				// Unless it was esc, in which case we need it to close Windows
				// (there might be other special cases, we'll see.. )
				skipFurtherProcessing = true;
			}
		}
	}

	LRESULT result = 0;
	if(skipFurtherProcessing)
	{
		result = -1;
	} else
	{
		result = CallNextHookEx(ghKbdHook, code, wParam, lParam);
	}

	if(mpt::Windows::IsWine()) // work-around for https://bugs.openmpt.org/view.php?id=713
	{
		if(theApp.InGuiThread())
		{
			s_KeyboardHookReentryFlag = false;
		}
	}

	return result;
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


void CMainFrame::OnTimerNotify()
//------------------------------
{
	MPT_TRACE();
	ASSERT(InGuiThread());
	ASSERT(!InNotifyHandler());
	m_InNotifyHandler = true;
	Notification PendingNotification;
	bool found = false;
	int64 currenttotalsamples = 0;
	if(gpSoundDevice)
	{
		currenttotalsamples = gpSoundDevice->GetStreamPosition().Frames;
	}
	{
		// advance to the newest notification, drop the obsolete ones
		MPT_LOCK_GUARD<mpt::mutex> lock(m_NotificationBufferMutex);
		const Notification * pnotify = nullptr;
		const Notification * p = m_NotifyBuffer.peek_p();
		if(p && currenttotalsamples >= p->timestampSamples)
		{
			pnotify = p;
			while(m_NotifyBuffer.peek_next_p() && currenttotalsamples >= m_NotifyBuffer.peek_next_p()->timestampSamples)
			{
				m_NotifyBuffer.pop();
				p = m_NotifyBuffer.peek_p();
				pnotify = p;
			}
		}
		if(pnotify)
		{
			PendingNotification = *pnotify; // copy notification so that we can free the buffer
			found = true;
			{
				m_NotifyBuffer.pop();
			}
		}
	}
	if(found)
	{
		OnUpdatePosition(0, (LPARAM)&PendingNotification);
	}
	m_InNotifyHandler = false;
	ASSERT(!InNotifyHandler());
}


void CMainFrame::SoundDeviceMessage(LogLevel level, const mpt::ustring &str)
//--------------------------------------------------------------------------
{
	MPT_TRACE();
	Reporting::Message(level, str);
}


void CMainFrame::SoundSourcePreStartCallback()
//--------------------------------------------
{
	MPT_TRACE();
	m_SoundDeviceClock.SetResolution(1);
}


void CMainFrame::SoundSourcePostStopCallback()
//--------------------------------------------
{
	MPT_TRACE();
	m_SoundDeviceClock.SetResolution(0);
}


uint64 CMainFrame::SoundSourceGetReferenceClockNowNanoseconds() const
//-------------------------------------------------------------------
{
	MPT_TRACE();
	MPT_ASSERT(!InAudioThread());
	return m_SoundDeviceClock.NowNanoseconds();
}


uint64 CMainFrame::SoundSourceLockedGetReferenceClockNowNanoseconds() const
//-------------------------------------------------------------------------
{
	MPT_TRACE();
	MPT_ASSERT(InAudioThread());
	return m_SoundDeviceClock.NowNanoseconds();
}


bool CMainFrame::SoundSourceIsLockedByCurrentThread() const
//---------------------------------------------------------
{
	MPT_TRACE();
	return theApp.GetGlobalMutexRef().IsLockedByCurrentThread();
}


void CMainFrame::SoundSourceLock()
//--------------------------------
{
	MPT_TRACE();
	m_SoundDeviceFillBufferCriticalSection.Enter();
	MPT_ASSERT_ALWAYS(m_pSndFile != nullptr);
	m_AudioThreadId = GetCurrentThreadId();
	mpt::log::Trace::SetThreadId(mpt::log::Trace::ThreadKindAudio, m_AudioThreadId);
}


void CMainFrame::SoundSourceUnlock()
//----------------------------------
{
	MPT_TRACE();
	MPT_ASSERT_ALWAYS(m_pSndFile != nullptr);
	m_AudioThreadId = 0;
	m_SoundDeviceFillBufferCriticalSection.Leave();
}


//==============================
class StereoVuMeterTargetWrapper
//==============================
	: public AudioReadTargetBufferInterleavedDynamic
{
private:
	VUMeter &vumeter;
public:
	StereoVuMeterTargetWrapper(SampleFormat sampleFormat, bool clipFloat, Dither &dither, void *buffer, VUMeter &vumeter)
		: AudioReadTargetBufferInterleavedDynamic(sampleFormat, clipFloat, dither, buffer)
		, vumeter(vumeter)
	{
		return;
	}
	virtual void DataCallback(int *MixSoundBuffer, std::size_t channels, std::size_t countChunk)
	{
		vumeter.Process(MixSoundBuffer, channels, countChunk);
		AudioReadTargetBufferInterleavedDynamic::DataCallback(MixSoundBuffer, channels, countChunk);
	}
};


void CMainFrame::SoundSourceLockedRead(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo, std::size_t numFrames, void *buffer, const void *inputBuffer)
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	MPT_UNREFERENCED_PARAMETER(inputBuffer);
	MPT_ASSERT(InAudioThread());
	OPENMPT_PROFILE_FUNCTION(Profiler::Audio);
	TimingInfo timingInfo;
	timingInfo.OutputLatency = bufferAttributes.Latency;
	timingInfo.StreamFrames = timeInfo.SyncPointStreamFrames;
	timingInfo.SystemTimestamp = timeInfo.SyncPointSystemTimestamp;
	timingInfo.Speed = timeInfo.Speed;
	m_pSndFile->m_TimingInfo = timingInfo;
	m_Dither.SetMode((DitherMode)bufferFormat.DitherType);
	StereoVuMeterTargetWrapper target(bufferFormat.sampleFormat, bufferFormat.NeedsClippedFloat, m_Dither, buffer, m_VUMeter);
	CSoundFile::samplecount_t renderedFrames = m_pSndFile->Read(numFrames, target);
	ASSERT(renderedFrames <= numFrames);
	CSoundFile::samplecount_t remainingFrames = numFrames - renderedFrames;
	if(remainingFrames > 0)
	{
		// The sound device interface expects the whole buffer to be filled, always.
		// Clear remaining buffer if not enough samples got rendered.
		std::size_t frameSize = bufferFormat.Channels * (bufferFormat.sampleFormat.GetBitsPerSample()/8);
		if(bufferFormat.sampleFormat.IsUnsigned())
		{
			std::memset((char*)(buffer) + renderedFrames * frameSize, 0x80, remainingFrames * frameSize);
		} else
		{
			std::memset((char*)(buffer) + renderedFrames * frameSize, 0, remainingFrames * frameSize);
		}
	}
}


void CMainFrame::SoundSourceLockedDone(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo)
//------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	MPT_UNREFERENCED_PARAMETER(bufferFormat);
	MPT_UNREFERENCED_PARAMETER(bufferAttributes);
	MPT_ASSERT(InAudioThread());
	OPENMPT_PROFILE_FUNCTION(Profiler::Notify);
	std::size_t numFrames = static_cast<std::size_t>(timeInfo.RenderStreamPositionAfter.Frames - timeInfo.RenderStreamPositionBefore.Frames);
	int64 streamPosition = timeInfo.RenderStreamPositionAfter.Frames;
	DoNotification(numFrames, streamPosition);
	//m_pSndFile->m_TimingInfo = TimingInfo(); // reset
}


bool CMainFrame::IsAudioDeviceOpen() const
//----------------------------------------
{
	MPT_TRACE();
	return gpSoundDevice && gpSoundDevice->IsOpen();
}


bool CMainFrame::audioOpenDevice()
//--------------------------------
{
	MPT_TRACE();
	if(!TrackerSettings::Instance().GetMixerSettings().IsValid())
	{
		Reporting::Error("Unable to open sound device: Invalid mixer settings.");
		return false;
	}
	const SoundDevice::Identifier deviceIdentifier = TrackerSettings::Instance().GetSoundDeviceIdentifier();
	if(gpSoundDevice && (gpSoundDevice->GetDeviceInfo().GetIdentifier() != deviceIdentifier))
	{
		gpSoundDevice->Stop();
		gpSoundDevice->Close();
		delete gpSoundDevice;
		gpSoundDevice = nullptr;
	}
	if(IsAudioDeviceOpen())
	{
		return true;
	}
	if(!gpSoundDevice)
	{
		gpSoundDevice = theApp.GetSoundDevicesManager()->CreateSoundDevice(deviceIdentifier);
	}
	if(!gpSoundDevice)
	{
		Reporting::Error("Unable to open sound device: Could not find sound device.");
		return false;
	}
	gpSoundDevice->SetMessageReceiver(this);
	gpSoundDevice->SetSource(this);
	SoundDevice::Settings deviceSettings = TrackerSettings::Instance().GetSoundDeviceSettings(deviceIdentifier);
	if(!gpSoundDevice->Open(deviceSettings))
	{
		if(!gpSoundDevice->IsAvailable())
		{
			Reporting::Error("Unable to open sound device: Device not available.");
		} else
		{
			Reporting::Error("Unable to open sound device.");
		}
		return false;
	}
	SampleFormat actualSampleFormat = gpSoundDevice->GetActualSampleFormat();
	if(!actualSampleFormat.IsValid())
	{
		Reporting::Error("Unable to open sound device: Unknown sample format.");
		return false;
	}
	deviceSettings.sampleFormat = actualSampleFormat;
	TrackerSettings::Instance().MixerSamplerate = gpSoundDevice->GetSettings().Samplerate;
	TrackerSettings::Instance().SetSoundDeviceSettings(deviceIdentifier, deviceSettings);
	return true;
}


void CMainFrame::audioCloseDevice()
//---------------------------------
{
	MPT_TRACE();
	if(gpSoundDevice)
	{
		gpSoundDevice->Close();
	}
	if(m_NotifyTimer)
	{
		KillTimer(m_NotifyTimer);
		m_NotifyTimer = 0;
	}
	ResetNotificationBuffer();
}


void VUMeter::Process(const int *mixbuffer, std::size_t numChannels, std::size_t numFrames)
//-----------------------------------------------------------------------------------------
{
	for(std::size_t frame = 0; frame < numFrames; ++frame)
	{
		for(std::size_t channel = 0; channel < std::min(numChannels, maxChannels); ++channel)
		{
			Channel &c = channels[channel];
			const int sample = mixbuffer[frame*numChannels + channel];
			c.peak = std::max(c.peak, mpt::abs(sample));
			if(sample < MIXING_CLIPMIN || MIXING_CLIPMAX < sample)
			{
				c.clipped = true;
			}
		}
	}
	for(std::size_t channel = std::min(numChannels, maxChannels); channel < maxChannels; ++channel)
	{
		channels[channel] = Channel();
	}
}


const float VUMeter::dynamicRange = 48.0f; // corresponds to the current implementation of the UI widget displaying the result


void VUMeter::SetDecaySpeedDecibelPerSecond(float decibelPerSecond)
//-----------------------------------------------------------------
{
	float linearDecayRate = decibelPerSecond / dynamicRange;
	decayParam = Util::Round<int32>(linearDecayRate * MIXING_CLIPMAX);
}


void VUMeter::Decay(int32 secondsNum, int32 secondsDen)
//-----------------------------------------------------
{
	int32 decay = Util::muldivr(decayParam, secondsNum, secondsDen);
	for(std::size_t channel = 0; channel < maxChannels; ++channel)
	{
		channels[channel].peak = std::max(channels[channel].peak - decay, 0);
	}
}


void VUMeter::ResetClipped()
//--------------------------
{
	for(std::size_t channel = 0; channel < maxChannels; ++channel)
	{
		channels[channel].clipped = false;
	}
}


static void SetVUMeter(uint32 *masterVU, const VUMeter &vumeter)
//--------------------------------------------------------------
{
	for(std::size_t channel = 0; channel < VUMeter::maxChannels; ++channel)
	{
		masterVU[channel] = Clamp(vumeter[channel].peak >> 11, 0, 0x10000);
		if(vumeter[channel].clipped)
		{
			masterVU[channel] |= Notification::ClipVU;
		}
	}
}


bool CMainFrame::DoNotification(DWORD dwSamplesRead, int64 streamPosition)
//------------------------------------------------------------------------
{
	MPT_TRACE();
	ASSERT(InAudioThread());
	if(!m_pSndFile) return false;

	FlagSet<Notification::Type> notifyType(Notification::Default);
	Notification::Item notifyItem = 0;

	if(m_pSndFile->m_pModDoc)
	{
		notifyType = m_pSndFile->m_pModDoc->GetNotificationType();
		notifyItem = m_pSndFile->m_pModDoc->GetNotificationItem();
	}

	// Add an entry to the notification history

	Notification notification(notifyType, notifyItem, streamPosition, m_pSndFile->m_PlayState.m_nRow, m_pSndFile->m_PlayState.m_nTickCount, m_pSndFile->GetNumTicksOnCurrentRow(), m_pSndFile->m_PlayState.m_nCurrentOrder, m_pSndFile->m_PlayState.m_nPattern, m_pSndFile->GetMixStat(), static_cast<uint8>(m_pSndFile->m_MixerSettings.gnChannels));

	m_pSndFile->ResetMixStat();

	if(m_pSndFile->m_SongFlags[SONG_ENDREACHED]) notification.type.set(Notification::EOS);

	if(notifyType[Notification::Sample])
	{
		// Sample positions
		const SAMPLEINDEX smp = notifyItem;
		if(smp > 0 && smp <= m_pSndFile->GetNumSamples() && m_pSndFile->GetSample(smp).pSample != nullptr)
		{
			for(CHANNELINDEX k = 0; k < MAX_CHANNELS; k++)
			{
				const ModChannel &chn = m_pSndFile->m_PlayState.Chn[k];
				if(chn.pModSample == &m_pSndFile->GetSample(smp) && chn.nLength != 0	// Corrent sample is set up on this channel
					&& (!chn.dwFlags[CHN_NOTEFADE] || chn.nFadeOutVol))					// And it hasn't completely faded out yet, so it's still playing
				{
					notification.pos[k] = chn.position.GetInt();
				} else
				{
					notification.pos[k] = Notification::PosInvalid;
				}
			}
		} else
		{
			// Can't generate a valid notification.
			notification.type.reset(Notification::Sample);
		}
	} else if(notifyType[Notification::VolEnv | Notification::PanEnv | Notification::PitchEnv])
	{
		// Instrument envelopes
		const INSTRUMENTINDEX ins = notifyItem;

		EnvelopeType notifyEnv = ENV_VOLUME;
		if(notifyType[Notification::PitchEnv])
			notifyEnv = ENV_PITCH;
		else if(notifyType[Notification::PanEnv])
			notifyEnv = ENV_PANNING;

		if(ins > 0 && ins <= m_pSndFile->GetNumInstruments() && m_pSndFile->Instruments[ins] != nullptr)
		{
			for(CHANNELINDEX k = 0; k < MAX_CHANNELS; k++)
			{
				const ModChannel &chn = m_pSndFile->m_PlayState.Chn[k];
				SmpLength pos = Notification::PosInvalid;

				if(chn.pModInstrument == m_pSndFile->Instruments[ins]				// Correct instrument is set up on this channel
					&& (chn.nLength || chn.pModInstrument->HasValidMIDIChannel())	// And it's playing something (sample or instrument plugin)
					&& (!chn.dwFlags[CHN_NOTEFADE] || chn.nFadeOutVol))				// And it hasn't completely faded out yet, so it's still playing
				{
					const ModChannel::EnvInfo &chnEnv = chn.GetEnvelope(notifyEnv);
					if(chnEnv.flags[ENV_ENABLED])
					{
						pos = chnEnv.nEnvPosition;
						if(m_pSndFile->m_playBehaviour[kITEnvelopePositionHandling])
						{
							// Impulse Tracker envelope handling (see e.g. CSoundFile::IncrementEnvelopePosition in SndMix.cpp for details)
							if(pos > 0)
								pos--;
							else
								pos = Notification::PosInvalid;	// Envelope isn't playing yet (e.g. when disabling it right when triggering a note)
						}
					}
				}
				notification.pos[k] = pos;
			}
		} else
		{
			// Can't generate a valid notification.
			notification.type.reset(Notification::VolEnv | Notification::PanEnv | Notification::PitchEnv);
		}
	} else if(notifyType[Notification::VUMeters])
	{
		// Pattern channel VU meters
		for(CHANNELINDEX k = 0; k < m_pSndFile->GetNumChannels(); k++)
		{
			uint32 vul = m_pSndFile->m_PlayState.Chn[k].nLeftVU;
			uint32 vur = m_pSndFile->m_PlayState.Chn[k].nRightVU;
			notification.pos[k] = (vul << 8) | (vur);
		}
	}

	{
		// Master VU meter
		SetVUMeter(notification.masterVU, m_VUMeter);
		m_VUMeter.Decay(dwSamplesRead, m_pSndFile->m_MixerSettings.gdwMixingFreq);

	}

	{
		MPT_LOCK_GUARD<mpt::mutex> lock(m_NotificationBufferMutex);
		if(m_NotifyBuffer.write_size() == 0)
		{
			ASSERT(0);
			return FALSE; // drop notification
		}
		m_NotifyBuffer.push(notification);
	}

	return true;
}


void CMainFrame::UpdateDspEffects(CSoundFile &sndFile, bool reset)
//----------------------------------------------------------------
{
	CriticalSection cs;
#ifndef NO_REVERB
	sndFile.m_Reverb.m_Settings = TrackerSettings::Instance().m_ReverbSettings;
#endif
#ifndef NO_DSP
	sndFile.m_Surround.m_Settings = TrackerSettings::Instance().m_SurroundSettings;
#endif
#ifndef NO_DSP
	sndFile.m_MegaBass.m_Settings = TrackerSettings::Instance().m_MegaBassSettings;
#endif
#ifndef NO_EQ
	sndFile.SetEQGains(TrackerSettings::Instance().m_EqSettings.Gains, MAX_EQ_BANDS, TrackerSettings::Instance().m_EqSettings.Freqs, reset);
#endif
	sndFile.SetDspEffects(TrackerSettings::Instance().MixerDSPMask);
	sndFile.InitPlayer(reset);
}


void CMainFrame::UpdateAudioParameters(CSoundFile &sndFile, bool reset)
//---------------------------------------------------------------------
{
	CriticalSection cs;
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_MUTECHNMODE)
		TrackerSettings::Instance().MixerFlags |= SNDMIX_MUTECHNMODE;
	else
		TrackerSettings::Instance().MixerFlags &= ~SNDMIX_MUTECHNMODE;
	sndFile.SetMixerSettings(TrackerSettings::Instance().GetMixerSettings());
	sndFile.SetResamplerSettings(TrackerSettings::Instance().GetResamplerSettings());
	UpdateDspEffects(sndFile, false); // reset done in next line
	sndFile.InitPlayer(reset);
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
		gcolrefVuMeter[i] = RGB(r, g, b);
	}
	CMainFrame *mainFrm = GetMainFrame();
	if(mainFrm != nullptr)
	{
		mainFrm->m_wndToolBar.m_VuMeter.Invalidate();
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


void CMainFrame::ResetNotificationBuffer()
//----------------------------------------
{
	MPT_TRACE();
	MPT_LOCK_GUARD<mpt::mutex> lock(m_NotificationBufferMutex);
	m_NotifyBuffer.clear();
}


bool CMainFrame::PreparePlayback()
//--------------------------------
{
	MPT_TRACE();
	// open the audio device to update needed TrackerSettings mixer parameters
	if(!audioOpenDevice()) return false;
	return true;
}


bool CMainFrame::StartPlayback()
//------------------------------
{
	MPT_TRACE();
	if(!m_pSndFile) return false; // nothing to play
	if(!IsAudioDeviceOpen()) return false;
	if(!gpSoundDevice->Start()) return false;
	if(!m_NotifyTimer)
	{
		if(TrackerSettings::Instance().GUIUpdateInterval.Get() > 0)
		{
			m_NotifyTimer = SetTimer(TIMERID_NOTIFY, TrackerSettings::Instance().GUIUpdateInterval, NULL);
		} else
		{
			m_NotifyTimer = SetTimer(TIMERID_NOTIFY, std::max<int>(1, Util::Round<int>(gpSoundDevice->GetEffectiveBufferAttributes().UpdateInterval * 1000.0)), NULL);
		}
	}
	return true;
}


void CMainFrame::StopPlayback()
//-----------------------------
{
	MPT_TRACE();
	if(!IsAudioDeviceOpen()) return;
	gpSoundDevice->Stop();
	if(m_NotifyTimer)
	{
		KillTimer(m_NotifyTimer);
		m_NotifyTimer = 0;
	}
	ResetNotificationBuffer();
	if(!gpSoundDevice->GetDeviceCaps().CanKeepDeviceRunning || TrackerSettings::Instance().m_SoundSettingsStopMode == SoundDeviceStopModeClosed)
	{
		audioCloseDevice();
	}
}


bool CMainFrame::RestartPlayback()
//--------------------------------
{
	MPT_TRACE();
	if(!m_pSndFile) return false; // nothing to play
	if(!IsAudioDeviceOpen()) return false;
	if(!gpSoundDevice->IsPlaying()) return false;
	gpSoundDevice->StopAndAvoidPlayingSilence();
	if(m_NotifyTimer)
	{
		KillTimer(m_NotifyTimer);
		m_NotifyTimer = 0;
	}
	ResetNotificationBuffer();
	return StartPlayback();
}


bool CMainFrame::PausePlayback()
//------------------------------
{
	MPT_TRACE();
	if(!IsAudioDeviceOpen()) return false;
	gpSoundDevice->Stop();
	if(m_NotifyTimer)
	{
		KillTimer(m_NotifyTimer);
		m_NotifyTimer = 0;
	}
	ResetNotificationBuffer();
	return true;
}


void CMainFrame::GenerateStopNotification()
//-----------------------------------------
{
	Notification mn(Notification::Stop);
	SendMessage(WM_MOD_UPDATEPOSITION, 0, (LPARAM)&mn);
}


void CMainFrame::UnsetPlaybackSoundFile()
//---------------------------------------
{
	if(m_pSndFile)
	{
		m_pSndFile->SuspendPlugins();
		if(m_pSndFile->GetpModDoc())
		{
			m_wndTree.UpdatePlayPos(m_pSndFile->GetpModDoc(), nullptr);
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
				if(!(m_pSndFile->m_PlayState.Chn[i].nMasterChn))
				{
					m_pSndFile->m_PlayState.Chn[i].nLength = 0;
					m_pSndFile->m_PlayState.Chn[i].position.Set(0);
				}
			}
		}
	}
	m_pSndFile = nullptr;
	m_wndToolBar.SetCurrentSong(nullptr);
	ResetNotificationBuffer();
}


void CMainFrame::SetPlaybackSoundFile(CSoundFile *pSndFile)
//---------------------------------------------------------
{
	m_pSndFile = pSndFile;
}


bool CMainFrame::PlayMod(CModDoc *pModDoc)
//----------------------------------------
{
	MPT_ASSERT_ALWAYS(!theApp.GetGlobalMutexRef().IsLockedByCurrentThread());
	if(!pModDoc) return false;
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	if(!IsValidSoundFile(sndFile)) return false;

	// if something is playing, pause it
	PausePlayback();
	GenerateStopNotification();

	UnsetPlaybackSoundFile();

	// open audio device if not already open
	if (!PreparePlayback()) return false;

	// set mixing parameters in CSoundFile
	UpdateAudioParameters(sndFile);

	SetPlaybackSoundFile(&sndFile);

	const bool bPaused = m_pSndFile->IsPaused();
	const bool bPatLoop = m_pSndFile->m_SongFlags[SONG_PATTERNLOOP];

	m_pSndFile->m_SongFlags.reset(SONG_FADINGSONG | SONG_ENDREACHED);

	if(!bPatLoop && bPaused) sndFile.m_SongFlags.set(SONG_PAUSED);
	sndFile.SetRepeatCount((TrackerSettings::Instance().gbLoopSong) ? -1 : 0);

	sndFile.InitPlayer(true);
	sndFile.ResumePlugins();

	m_wndToolBar.SetCurrentSong(m_pSndFile);

	m_VUMeter = VUMeter();

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
	MPT_ASSERT_ALWAYS(!theApp.GetGlobalMutexRef().IsLockedByCurrentThread());
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
	MPT_ASSERT_ALWAYS(!theApp.GetGlobalMutexRef().IsLockedByCurrentThread());
	if(pModDoc && (pModDoc != GetModPlaying())) return false;
	if(!IsPlaying()) return true;

	PausePlayback();
	GenerateStopNotification();

	m_pSndFile->ResetPlayPos();
	m_pSndFile->ResetChannels();
	UnsetPlaybackSoundFile();

	StopPlayback();

	return true;
}


bool CMainFrame::StopSoundFile(CSoundFile *pSndFile)
//--------------------------------------------------
{
	MPT_ASSERT_ALWAYS(!theApp.GetGlobalMutexRef().IsLockedByCurrentThread());
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
	MPT_ASSERT_ALWAYS(!theApp.GetGlobalMutexRef().IsLockedByCurrentThread());
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

	UpdateAudioParameters(*pSndFile);

	SetPlaybackSoundFile(pSndFile);

	m_pSndFile->InitPlayer(true);

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
		if(CTrackApp::gpDLSBanks[nDLSBank]->ExtractInstrument(m_WaveFile, 1, nIns, nRgn))
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


BOOL CMainFrame::PlaySoundFile(const mpt::PathString &filename, ModCommand::NOTE note)
//------------------------------------------------------------------------------------
{
	bool ok = false;
	BeginWaitCursor();
	{
		CriticalSection cs;
		static mpt::PathString prevFile;
		// Did we already load this file for previewing? Don't load it again if the preview is still running.
		ok = (prevFile == filename && m_pSndFile == &m_WaveFile);

		if(!ok && !filename.empty())
		{
			InputFile f(filename);
			if(f.IsValid())
			{
				FileReader file = GetFileReader(f);
				if(file.IsValid())
				{
					InitPreview();
					m_WaveFile.m_SongFlags.set(SONG_PAUSED);
					// Avoid hanging audio while reading file - we have removed all sample and instrument references before,
					// so it's safe to replace the sample / instrument now.
					cs.Leave();
					ok = m_WaveFile.ReadInstrumentFromFile(1, file, TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad);
					cs.Enter();
					if(!ok)
					{
						// Try reading as sample if reading as instrument fails
						ok = m_WaveFile.ReadSampleFromFile(1, file, TrackerSettings::Instance().m_MayNormalizeSamplesOnLoad);
						m_WaveFile.AllocateInstrument(1, 1);
					}
				}
			}
		}
		if(ok)
		{
			// Write notes to pattern. Also done if we have previously loaded this file, since we might be previewing another note now.
			PreparePreview(note);
			prevFile = filename;
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


BOOL CMainFrame::PlaySoundFile(CSoundFile &sndFile, INSTRUMENTINDEX nInstrument, SAMPLEINDEX nSample, ModCommand::NOTE note)
//--------------------------------------------------------------------------------------------------------------------------
{
	bool ok = false;
	BeginWaitCursor();
	{
		CriticalSection cs;
		InitPreview();
		m_WaveFile.m_nType = sndFile.GetType();
		if ((nInstrument) && (nInstrument <= sndFile.GetNumInstruments()))
		{
			m_WaveFile.m_nInstruments = 1;
			m_WaveFile.m_nSamples = 32;
		} else
		{
			m_WaveFile.m_nInstruments = 0;
			m_WaveFile.m_nSamples = 1;
		}
		if (nInstrument != INSTRUMENTINDEX_INVALID && nInstrument <= sndFile.GetNumInstruments())
		{
			m_WaveFile.ReadInstrumentFromSong(1, sndFile, nInstrument);
		} else if(nSample != SAMPLEINDEX_INVALID && nSample <= sndFile.GetNumSamples())
		{
			m_WaveFile.ReadSampleFromSong(1, sndFile, nSample);
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
	const CModDoc *activeDoc = GetActiveDoc();
	m_WaveFile.Destroy();
	m_WaveFile.Create(FileReader());
	// Avoid global volume ramping when trying samples in the treeview.
	m_WaveFile.m_nDefaultGlobalVolume = m_WaveFile.m_PlayState.m_nGlobalVolume = MAX_GLOBAL_VOLUME;
	if(activeDoc != nullptr && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_NOEXTRALOUD))
	{
		m_WaveFile.SetMixLevels(activeDoc->GetrSoundFile().GetMixLevels());
		m_WaveFile.m_nSamplePreAmp = activeDoc->GetrSoundFile().m_nSamplePreAmp;
	} else
	{
		// Preview at 0dB
		m_WaveFile.SetMixLevels(mixLevels1_17RC3);
		m_WaveFile.m_nSamplePreAmp = static_cast<uint32>(m_WaveFile.GetPlayConfig().getNormalSamplePreAmp());
	}
	m_WaveFile.m_nDefaultTempo.Set(125);
	m_WaveFile.m_nDefaultSpeed = 6;
	m_WaveFile.m_nType = MOD_TYPE_MPT;
	m_WaveFile.m_nChannels = 2;
	m_WaveFile.m_nInstruments = 1;
	m_WaveFile.m_nTempoMode = tempoModeClassic;
	m_WaveFile.Order.resize(1);
	m_WaveFile.Order[0] = 0;
	m_WaveFile.Patterns.Insert(0, 2);
	m_WaveFile.m_SongFlags = SONG_LINEARSLIDES;
}


void CMainFrame::PreparePreview(ModCommand::NOTE note)
//----------------------------------------------------
{
	m_WaveFile.m_SongFlags.reset(SONG_PAUSED);
	m_WaveFile.SetRepeatCount(-1);
	m_WaveFile.ResetPlayPos();

	ModCommand *m = m_WaveFile.Patterns[0];
	if(m)
	{
		if(m_WaveFile.GetNumSamples() > 0)
		{
			m[0].note = note;
			m[0].instr = 1;

		}
		// Infinite loop on second row
		m[1 * 2].command = CMD_POSITIONJUMP;
		m[1 * 2 + 1].command = CMD_PATTERNBREAK;
		m[1 * 2 + 1].param = 1;
	}
}


HWND CMainFrame::GetFollowSong() const
//------------------------------------
{
	return GetModPlaying() ? GetModPlaying()->GetFollowWnd() : NULL;
}


void CMainFrame::IdleHandlerSounddevice()
//---------------------------------------
{
	MPT_TRACE();
	if(gpSoundDevice)
	{
		const FlagSet<SoundDevice::RequestFlags> requestFlags = gpSoundDevice->GetRequestFlags();
		if(requestFlags[SoundDevice::RequestFlagClose])
		{
			StopPlayback();
			audioCloseDevice();
		} else if(requestFlags[SoundDevice::RequestFlagReset])
		{
			ResetSoundCard();
		} else if(requestFlags[SoundDevice::RequestFlagRestart])
		{
			RestartPlayback();
		} else
		{
			gpSoundDevice->OnIdle();
		}
	}
}


BOOL CMainFrame::ResetSoundCard()
//-------------------------------
{
	MPT_TRACE();
	return CMainFrame::SetupSoundCard(TrackerSettings::Instance().GetSoundDeviceSettings(TrackerSettings::Instance().GetSoundDeviceIdentifier()), TrackerSettings::Instance().GetSoundDeviceIdentifier(), TrackerSettings::Instance().m_SoundSettingsStopMode, true);
}


BOOL CMainFrame::SetupSoundCard(SoundDevice::Settings deviceSettings, SoundDevice::Identifier deviceIdentifier, SoundDeviceStopMode stoppedMode, bool forceReset)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	if(forceReset
		|| (TrackerSettings::Instance().GetSoundDeviceIdentifier() != deviceIdentifier)
		|| (TrackerSettings::Instance().GetSoundDeviceSettings(deviceIdentifier) != deviceSettings)
		|| (TrackerSettings::Instance().m_SoundSettingsStopMode != stoppedMode)
		)
	{
		CModDoc *pActiveMod = nullptr;
		if(IsPlaying())
		{
			if ((m_pSndFile) && (!m_pSndFile->IsPaused())) pActiveMod = GetModPlaying();
			PauseMod();
		}
		if(gpSoundDevice)
		{
			gpSoundDevice->Close();
		}
		TrackerSettings::Instance().m_SoundSettingsStopMode = stoppedMode;
		switch(stoppedMode)
		{
			case SoundDeviceStopModeClosed:
				deviceSettings.KeepDeviceRunning = true;
				break;
			case SoundDeviceStopModeStopped:
				deviceSettings.KeepDeviceRunning = false;
				break;
			case SoundDeviceStopModePlaying:
				deviceSettings.KeepDeviceRunning = true;
				break;
		}
		TrackerSettings::Instance().SetSoundDeviceIdentifier(deviceIdentifier);
		TrackerSettings::Instance().SetSoundDeviceSettings(deviceIdentifier, deviceSettings);
		TrackerSettings::Instance().MixerOutputChannels = deviceSettings.Channels;
		TrackerSettings::Instance().MixerSamplerate = deviceSettings.Samplerate;
		if(pActiveMod)
		{
			PlayMod(pActiveMod);
		}
		UpdateWindow();
	} else
	{
		// No need to restart playback
		CriticalSection cs;
		if(GetSoundFilePlaying()) UpdateAudioParameters(*GetSoundFilePlaying(), FALSE);
	}
	return TRUE;
}


BOOL CMainFrame::SetupPlayer()
//----------------------------
{
	CriticalSection cs;
	if(GetSoundFilePlaying()) UpdateAudioParameters(*GetSoundFilePlaying(), FALSE);
	return TRUE;
}


BOOL CMainFrame::SetupDirectories(const mpt::PathString &szModDir, const mpt::PathString &szSampleDir, const mpt::PathString &szInstrDir, const mpt::PathString &szVstDir, const mpt::PathString &szPresetDir)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	// will also set working directory
	TrackerSettings::Instance().PathSongs.SetDefaultDir(szModDir);
	TrackerSettings::Instance().PathSamples.SetDefaultDir(szSampleDir);
	TrackerSettings::Instance().PathInstruments.SetDefaultDir(szInstrDir);
	TrackerSettings::Instance().PathPlugins.SetDefaultDir(szVstDir);
	TrackerSettings::Instance().PathPluginPresets.SetDefaultDir(szPresetDir);
	return TRUE;
}

BOOL CMainFrame::SetupMiscOptions()
//---------------------------------
{
	if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_MUTECHNMODE)
		TrackerSettings::Instance().MixerFlags |= SNDMIX_MUTECHNMODE;
	else
		TrackerSettings::Instance().MixerFlags &= ~SNDMIX_MUTECHNMODE;
	{
		CriticalSection cs;
		if(GetSoundFilePlaying()) UpdateAudioParameters(*GetSoundFilePlaying());
	}

	m_wndToolBar.EnableFlatButtons(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FLATBUTTONS);

	UpdateTree(nullptr, UpdateHint().MPTOptions());
	UpdateAllViews(UpdateHint().MPTOptions());
	return true;
}


void CMainFrame::SetupMidi(DWORD d, UINT_PTR n)
//---------------------------------------------
{
	bool deviceChanged = (TrackerSettings::Instance().m_nMidiDevice != n);
	TrackerSettings::Instance().m_dwMidiSetup = d;
	TrackerSettings::Instance().SetMIDIDevice(n);
	if(deviceChanged && shMidiIn)
	{
		// Device has changed, close the old one.
		midiCloseDevice();
		midiOpenDevice();
	}
}


void CMainFrame::UpdateAllViews(UpdateHint hint, CObject *pHint)
//-----------------------------------------------------------
{
	CDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if (pDocTmpl)
	{
		POSITION pos = pDocTmpl->GetFirstDocPosition();
		CModDoc *pDoc;
		while ((pos != NULL) && ((pDoc = dynamic_cast<CModDoc *>(pDocTmpl->GetNextDoc(pos))) != nullptr))
		{
			pDoc->UpdateAllViews(NULL, hint, pHint);
		}
	}
}


void CMainFrame::SetUserText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szUserText[0])
	{
		strcpy(m_szUserText, lpszText);
		OnUpdateUser(NULL);
	}
}


void CMainFrame::SetInfoText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szInfoText[0])
	{
		strcpy(m_szInfoText, lpszText);
		OnUpdateInfo(NULL);
	}
}


void CMainFrame::SetXInfoText(LPCSTR lpszText)
//-------------------------------------------
{
	if (lpszText[0] | m_szXInfoText[0])
	{
		strcpy(m_szXInfoText, lpszText);
		OnUpdateInfo(NULL);
	}
}


void CMainFrame::SetHelpText(LPCSTR lpszText)
//-------------------------------------------
{
	m_wndStatusBar.SetPaneText(0, lpszText);
}


void CMainFrame::OnDocumentCreated(CModDoc *pModDoc)
//--------------------------------------------------
{
	m_wndTree.OnDocumentCreated(pModDoc);
	UpdateMRUList();
}


void CMainFrame::OnDocumentClosed(CModDoc *pModDoc)
//-------------------------------------------------
{
	if (pModDoc == GetModPlaying()) PauseMod();

	// Make sure that OnTimer() won't try to set the closed document modified anymore.
	if (pModDoc == m_pJustModifiedDoc) m_pJustModifiedDoc = nullptr;

	m_wndTree.OnDocumentClosed(pModDoc);
}


void CMainFrame::UpdateTree(CModDoc *pModDoc, UpdateHint hint, CObject *pHint)
//----------------------------------------------------------------------------
{
	m_wndTree.OnUpdate(pModDoc, hint, pHint);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnViewOptions()
//------------------------------
{
	if (m_bOptionsLocked)
		return;

	CPropertySheet dlg("OpenMPT Setup", this, m_nLastOptionsPage);
	COptionsGeneral general;
	COptionsSoundcard sounddlg(TrackerSettings::Instance().m_SoundDeviceIdentifier);
	COptionsSampleEditor smpeditor;
	COptionsKeyboard keyboard;
	COptionsColors colors;
	COptionsMixer mixerdlg;
	CMidiSetupDlg mididlg(TrackerSettings::Instance().m_dwMidiSetup, TrackerSettings::Instance().GetCurrentMIDIDevice());
	PathConfigDlg pathsdlg;
	CUpdateSetupDlg updatedlg;
#if defined(MPT_SETTINGS_CACHE)
	COptionsAdvanced advanced;
#endif // MPT_SETTINGS_CACHE
	dlg.AddPage(&general);
	dlg.AddPage(&sounddlg);
	dlg.AddPage(&mixerdlg);
#if !defined(NO_REVERB) || !defined(NO_DSP) || !defined(NO_EQ) || !defined(NO_AGC) || !defined(NO_EQ)
	COptionsPlayer dspdlg;
	dlg.AddPage(&dspdlg);
#endif
	dlg.AddPage(&smpeditor);
	dlg.AddPage(&keyboard);
	dlg.AddPage(&colors);
	dlg.AddPage(&mididlg);
	dlg.AddPage(&pathsdlg);
	dlg.AddPage(&updatedlg);
#if defined(MPT_SETTINGS_CACHE)
	dlg.AddPage(&advanced);
#endif // MPT_SETTINGS_CACHE
	m_bOptionsLocked = true;
	m_SoundCardOptionsDialog = &sounddlg;
	dlg.DoModal();
	m_SoundCardOptionsDialog = nullptr;
	m_bOptionsLocked = false;
	m_wndTree.OnOptionsChanged();
}


void CMainFrame::OnPluginManager()
//--------------------------------
{
#ifndef NO_PLUGINS
	PLUGINDEX nPlugslot = PLUGINDEX_INVALID;
	CModDoc* pModDoc = GetActiveDoc();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		//Find empty plugin slot
		for (PLUGINDEX nPlug = 0; nPlug < MAX_MIXPLUGINS; nPlug++)
		{
			if (pSndFile->m_MixPlugins[nPlug].pMixPlugin == nullptr)
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
		pModDoc->UpdateAllViews(nullptr, PluginHint().Info().Names().ModType());
		//Refresh Controls
		CChildFrame *pActiveChild = (CChildFrame *)MDIGetActive();
		pActiveChild->ForceRefresh();
	}
#endif // NO_PLUGINS
}


void CMainFrame::OnClipboardManager()
//-----------------------------------
{
	PatternClipboardDialog::Show();
}


void CMainFrame::OnAddDlsBank()
//-----------------------------
{
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.ExtensionFilter("All Sound Banks|*.dls;*.sbk;*.sf2;*.mss|"
			"Downloadable Sounds Banks (*.dls)|*.dls;*.mss|"
			"SoundFont 2.0 Banks (*.sf2)|*.sbk;*.sf2|"
			"All Files (*.*)|*.*||");
	if(!dlg.Show()) return;

	BeginWaitCursor();
	const FileDialog::PathList &files = dlg.GetFilenames();
	for(size_t counter = 0; counter < files.size(); counter++)
	{
		CTrackApp::AddDLSBank(files[counter]);
	}
	m_wndTree.RefreshDlsBanks();
	EndWaitCursor();
}


void CMainFrame::OnImportMidiLib()
//--------------------------------
{
	FileDialog dlg = OpenFileDialog()
		.ExtensionFilter("Text and INI files (*.txt,*.ini)|*.txt;*.ini;*.dls;*.sf2;*.sbk|"
			"Downloadable Sound Banks (*.dls)|*.dls;*.mss|"
			"SoundFont 2.0 banks (*.sf2)|*.sbk;*.sf2|"
			"Gravis UltraSound (ultrasnd.ini)|ultrasnd.ini|"
			"All Files (*.*)|*.*||");
	if(!dlg.Show()) return;

	BeginWaitCursor();
	CTrackApp::ImportMidiConfig(dlg.GetFirstFile());
	m_wndTree.RefreshMidiLibrary();
	EndWaitCursor();
}


void CMainFrame::OnTimer(UINT_PTR timerID)
//----------------------------------------
{
	switch(timerID)
	{
		case TIMERID_GUI:
			OnTimerGUI();
			break;
		case TIMERID_NOTIFY:
			OnTimerNotify();
			break;
	}
}


void CMainFrame::OnTimerGUI()
//---------------------------
{

	IdleHandlerSounddevice();

	// Display Time in status bar
	CSoundFile::samplecount_t time = 0;
	if(m_pSndFile != nullptr && m_pSndFile->GetSampleRate() != 0)
	{
		time = m_pSndFile->GetTotalSampleCount() / m_pSndFile->GetSampleRate();
		if(time != m_dwTimeSec)
		{
			m_dwTimeSec = time;
			m_nAvgMixChn = m_nMixChn;
			OnUpdateTime(NULL);
		}
	}

	// Idle Time Check
	DWORD curTime = timeGetTime();

	m_wndToolBar.SetCurrentSong(m_pSndFile);

	if(m_AutoSaver.IsEnabled())
	{
		bool success = m_AutoSaver.DoSave(curTime);
		if (!success)		// autosave failure; bring up options.
		{
			CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_PATHS;
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

#ifdef USE_PROFILER
	{
		Profiler::Update();

		CWnd * cwnd = CWnd::FromHandle(this->m_hWndMDIClient);
		CClientDC dc(cwnd);

		int height = 16;
		int width = 256;

		std::vector<std::string> catnames = Profiler::GetCategoryNames();
		std::vector<double> cats = Profiler::DumpCategories();

		for(int i=0; i<Profiler::CategoriesCount; i++)
		{
			dc.FillSolidRect(0, i * height, (int)(width * cats[i]), height, RGB(255,0,0));
			dc.FillSolidRect((int)(width * cats[i]), i * height, width - (int)(width * cats[i]), height, RGB(192,192,192));
			RECT rect;
			cwnd->GetClientRect(&rect);
			rect.left += width;
			rect.top += i * height;
			char dummy[1024];
			sprintf(dummy, "%6.3f%% %s", cats[i] * 100.0, catnames[i].c_str());
			dc.DrawText(dummy, strlen(dummy), &rect, DT_LEFT);
		}

		std::string dummy = Profiler::DumpProfiles();
		RECT rect;
		cwnd->GetClientRect(&rect);
		rect.top += Profiler::CategoriesCount * height;
		dc.DrawText(dummy.c_str(), dummy.length(), &rect, DT_LEFT);

		cwnd->Detach();
	}
#endif

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


CView *CMainFrame::GetActiveView()
//---------------------------------
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		return pMDIActive->GetActiveView();
	}

	return nullptr;
}


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
	TCHAR s[64];
	wsprintf(s, _T("%u:%02u:%02u"),
		m_dwTimeSec / 3600, (m_dwTimeSec / 60) % 60, m_dwTimeSec % 60);

	if(m_pSndFile != nullptr && m_pSndFile != &m_WaveFile && !m_pSndFile->IsPaused())
	{
		PATTERNINDEX nPat = m_pSndFile->m_PlayState.m_nPattern;
		if(m_pSndFile->Patterns.IsValidIndex(nPat))
		{
			if(nPat < 10) _tcscat(s, _T(" "));
			if(nPat < 100) _tcscat(s, _T(" "));
			wsprintf(s + _tcslen(s), _T(" [%d]"), nPat);
		}
		wsprintf(s + _tcslen(s), _T(" %dch"), m_nAvgMixChn);
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


void CMainFrame::OnUpdateXInfo(CCmdUI *)
//-------------------------------------
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_XINFO), m_szXInfoText, TRUE);
}

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


void CMainFrame::OpenMenuItemFile(const UINT nId, const bool isTemplateFile)
//--------------------------------------------------------------------------
{
	const UINT nIdBegin = (isTemplateFile) ? ID_FILE_OPENTEMPLATE : ID_EXAMPLE_MODULES;
	const std::vector<mpt::PathString>& vecFilePaths = (isTemplateFile) ? s_TemplateModulePaths : s_ExampleModulePaths;

	const UINT nIndex = nId - nIdBegin;
	if (nIndex < vecFilePaths.size())
	{
		const mpt::PathString& sPath = vecFilePaths[nIndex];
		const bool bExists = sPath.IsFile();
		CDocument *pDoc = nullptr;
		if(bExists)
		{
			pDoc = theApp.GetModDocTemplate()->OpenTemplateFile(sPath, !isTemplateFile);
		}
		if(!pDoc)
		{
			Reporting::Notification(L"The file '" + sPath.ToWide() + L"' " + (bExists ? L"exists but can't be read" : L"does not exist"));
		}
	} else
	{
		MPT_ASSERT(nId == UINT(isTemplateFile ? ID_FILE_OPENTEMPLATE_LASTINRANGE : ID_EXAMPLE_MODULES_LASTINRANGE));
		FileDialog::PathList files;
		theApp.OpenModulesDialog(files, isTemplateFile ? theApp.GetConfigPath() + MPT_PATHSTRING("TemplateModules") : theApp.GetAppDirPath() + MPT_PATHSTRING("ExampleSongs"));
		for(auto file = files.cbegin(); file != files.cend(); file++)
		{
			theApp.OpenDocumentFile(*file);
		}
	}
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


void CMainFrame::OnOpenMRUItem(UINT nId)
//--------------------------------------
{
	mpt::PathString file = TrackerSettings::Instance().mruFiles[nId - ID_MRU_LIST_FIRST];
	theApp.OpenDocumentFile(file);
}


void CMainFrame::OnUpdateMRUItem(CCmdUI *cmd)
//-------------------------------------------
{
	cmd->Enable(!TrackerSettings::Instance().mruFiles.empty());
}


LRESULT CMainFrame::OnInvalidatePatterns(WPARAM, LPARAM)
//------------------------------------------------------
{
	UpdateAllViews(UpdateHint().MPTOptions());
	return TRUE;
}


LRESULT CMainFrame::OnUpdatePosition(WPARAM, LPARAM lParam)
//---------------------------------------------------------
{
	OPENMPT_PROFILE_FUNCTION(Profiler::GUI);
	m_VUMeter.SetDecaySpeedDecibelPerSecond(TrackerSettings::Instance().VuMeterDecaySpeedDecibelPerSecond); // update in notification update in order to avoid querying the settings framework from inside audio thread
	Notification *pnotify = (Notification *)lParam;
	if (pnotify)
	{
		if(pnotify->type[Notification::EOS])
		{
			PostMessage(WM_COMMAND, ID_PLAYER_STOP);
		}
		//Log("OnUpdatePosition: row=%d time=%lu\n", pnotify->nRow, pnotify->TimestampSamples);
		if (GetModPlaying())
		{
			m_wndTree.UpdatePlayPos(GetModPlaying(), pnotify);
			if (GetFollowSong())
				::SendMessage(GetFollowSong(), WM_MOD_UPDATEPOSITION, 0, lParam);
		}
		m_nMixChn = pnotify->mixedChannels;
		m_wndToolBar.m_VuMeter.SetVuMeter(pnotify->masterVUchannels, pnotify->masterVU,  pnotify->type[Notification::Stop]);
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


void CMainFrame::OnReportBug()
//----------------------------
{
	CTrackApp::OpenURL(MptVersion::GetURL("bugtracker"));
}


BOOL CMainFrame::OnInternetLink(UINT nID)
//---------------------------------------
{
	mpt::ustring url;
	switch(nID)
	{
	case ID_NETLINK_MODPLUG:	url = MptVersion::GetURL("website"); break;
	case ID_NETLINK_TOP_PICKS:	url = MptVersion::GetURL("top_picks"); break;
	}
	if(!url.empty())
	{
		return CTrackApp::OpenURL(url) ? TRUE : FALSE;
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
		case kcViewMIDImapping:
		case kcViewEditHistory:
		case kcViewChannelManager:
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
			CMDIChildWnd *pMDIActive = MDIGetActive();
			CWnd *wnd = nullptr;
			if(pMDIActive)
			{
				wnd = pMDIActive->GetActiveView();
				// Hack: If the upper view is active, we only get the "container" (the dialog view with the tabs), not the view itself.
				if(!strcmp(wnd->GetRuntimeClass()->m_lpszClassName, "CModControlView"))
				{
					wnd = static_cast<CModControlView *>(wnd)->GetCurrentControlDlg();
				}
			}
			if(wnd) return wnd->SendMessage(WM_MOD_KEYCOMMAND, wParam, lParam);
	}

	return wParam;
}


void CMainFrame::OnInitMenu(CMenu* pMenu)
//---------------------------------------
{
	m_InputHandler->SetModifierMask(0);
	if (m_InputHandler->noAltMenu())
		return;

	CMDIFrameWnd::OnInitMenu(pMenu);

}


BOOL CMainFrame::InitRenderer(CSoundFile* pSndFile)
//-------------------------------------------------
{
	CriticalSection cs;
	pSndFile->m_bIsRendering = true;
	pSndFile->SuspendPlugins();
	pSndFile->ResumePlugins();
	return true;
}


BOOL CMainFrame::StopRenderer(CSoundFile* pSndFile)
//-------------------------------------------------
{
	CriticalSection cs;
	pSndFile->SuspendPlugins();
	pSndFile->m_bIsRendering = false;
	return true;
}


// We have swicthed focus to a new module - might need to update effect keys to reflect module type
bool CMainFrame::UpdateEffectKeys()
//---------------------------------
{
	CModDoc* pModDoc = GetActiveDoc();
	if (pModDoc)
	{
		return m_InputHandler->SetEffectLetters(pModDoc->GetrSoundFile().GetModSpecifications());
	}

	return false;
}


void CMainFrame::OnKillFocus(CWnd* pNewWnd)
//-----------------------------------------
{
	CMDIFrameWnd::OnKillFocus(pNewWnd);

	//rewbs: ensure modifiers are reset when we leave the window (e.g. alt-tab)
	m_InputHandler->SetModifierMask(0);
}


void CMainFrame::OnShowWindow(BOOL bShow, UINT /*nStatus*/)
//---------------------------------------------------------
{
	static bool firstShow = true;
	if (bShow && !IsWindowVisible() && firstShow)
	{
		firstShow = false;
		WINDOWPLACEMENT wpl;
		GetWindowPlacement(&wpl);
		wpl = theApp.GetSettings().Read<WINDOWPLACEMENT>("Display", "WindowPlacement", wpl);
		SetWindowPlacement(&wpl);
	}
}


void CMainFrame::OnInternetUpdate()
//---------------------------------
{
	CUpdateCheck::DoManualUpdateCheck();
}


void CMainFrame::OnShowSettingsFolder()
//-------------------------------------
{
	theApp.OpenDirectory(theApp.GetConfigPath());
}


LRESULT CMainFrame::OnUpdateCheckProgress(WPARAM wparam, LPARAM lparam)
//---------------------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(wparam);
	MPT_UNREFERENCED_PARAMETER(lparam);
	return TRUE;
}


LRESULT CMainFrame::OnUpdateCheckSuccess(WPARAM wparam, LPARAM lparam)
//--------------------------------------------------------------------
{
	TrackerSettings::Instance().UpdateLastUpdateCheck = mpt::Date::Unix(CUpdateCheck::ResultFromMessage(wparam, lparam).CheckTime);
	CUpdateCheck::ShowSuccessGUI(wparam, lparam);
	return TRUE;
}


LRESULT CMainFrame::OnUpdateCheckFailure(WPARAM wparam, LPARAM lparam)
//--------------------------------------------------------------------
{
	CUpdateCheck::ShowFailureGUI(wparam, lparam);
	return TRUE;
}


void CMainFrame::OnHelp()
//-----------------------
{
	CView *view = GetActiveView();
	const char *page = "";
	if(m_bOptionsLocked)
	{
		switch(m_nLastOptionsPage)
		{
			case OPTIONS_PAGE_GENERAL:		page = "::/Setup_General.html"; break;
			case OPTIONS_PAGE_SOUNDCARD:	page = "::/Setup_Soundcard.html"; break;
			case OPTIONS_PAGE_MIXER:		page = "::/Setup_Mixer.html"; break;
			case OPTIONS_PAGE_PLAYER:		page = "::/Setup_DSP.html"; break;
			case OPTIONS_PAGE_SAMPLEDITOR:	page = "::/Setup_Samples.html"; break;
			case OPTIONS_PAGE_KEYBOARD:		page = "::/Setup_Keyboard.html"; break;
			case OPTIONS_PAGE_COLORS:		page = "::/Setup_Display.html"; break;
			case OPTIONS_PAGE_MIDI:			page = "::/Setup_MIDI.html"; break;
			case OPTIONS_PAGE_PATHS:		page = "::/Setup_Paths_Auto_Save.html"; break;
			case OPTIONS_PAGE_UPDATE:		page = "::/Setup_Update.html"; break;
			case OPTIONS_PAGE_ADVANCED:		page = "::/Setup_Advanced.html"; break;
		}
	} else if(view != nullptr)
	{
		const char *className = view->GetRuntimeClass()->m_lpszClassName;
		if(!strcmp("CViewGlobals", className))			page = "::/General.html";
		else if(!strcmp("CViewPattern", className))		page = "::/Patterns.html";
		else if(!strcmp("CViewSample", className))		page = "::/Samples.html";
		else if(!strcmp("CViewInstrument", className))	page = "::/Instruments.html";
		else if(!strcmp("CModControlView", className))	page = "::/Comments.html";
	}

	const mpt::PathString helpFile = theApp.GetAppDirPath() + MPT_PATHSTRING("OpenMPT Manual.chm") + mpt::PathString::FromUTF8(page) + MPT_PATHSTRING(">OpenMPT");
	if(!HtmlHelpW(m_hWnd, helpFile.AsNative().c_str(), strcmp(page, "") ? HH_DISPLAY_TOC : HH_DISPLAY_TOPIC, NULL))
	{
		Reporting::Error(L"Could not find help file:\n" + helpFile.ToWide());
		return;
	}
	//::ShowWindow(hwndHelp, SW_SHOWMAXIMIZED);
}


LRESULT CMainFrame::OnViewMIDIMapping(WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------
{
	CModDoc *doc = GetActiveDoc();
	if(doc != nullptr)
		doc->ViewMIDIMapping(static_cast<PLUGINDEX>(wParam), static_cast<PlugParamIndex>(lParam));
	return 0;
}


HMENU CMainFrame::CreateFileMenu(const size_t nMaxCount, std::vector<mpt::PathString>& vPaths, const mpt::PathString &pszFolderName, const uint16 nIdRangeBegin)
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
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
			if (i == 1 && mpt::PathString::CompareNoCase(theApp.GetAppDirPath(), theApp.GetConfigPath()) == 0)
				break;

			mpt::PathString basePath;
			basePath = (i == 0) ? theApp.GetAppDirPath() : theApp.GetConfigPath();
			basePath += pszFolderName;
			if(!basePath.IsDirectory())
				continue;
			mpt::PathString sPath = basePath + MPT_PATHSTRING("*");

			WIN32_FIND_DATAW findData;
			MemsetZero(findData);
			HANDLE hFindFile = FindFirstFileW(sPath.AsNative().c_str(), &findData);
			if(hFindFile != INVALID_HANDLE_VALUE)
			{
				while(nAddCounter < nMaxCount)
				{
					// Note: The order in which the example files appears in the menu is unspecified.
					if(!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						vPaths.push_back(basePath + mpt::PathString::FromNative(findData.cFileName));
						AppendMenuW(hMenu, MF_STRING, nIdRangeBegin + nAddCounter, findData.cFileName);
						++nAddCounter;
					}
					if(FindNextFileW(hFindFile, &findData) == FALSE)
					{
						break;
					}
				}
				FindClose(hFindFile);
				hFindFile = INVALID_HANDLE_VALUE;
			}

		}

		if(nAddCounter == 0)
		{
			AppendMenu(hMenu, MF_STRING | MF_GRAYED | MF_DISABLED, 0, _T("No items found"));
		} else
		{
			AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			AppendMenu(hMenu, MF_STRING, nIdRangeBegin + nMaxCount, _T("Browse..."));
		}
	}

	return hMenu;
}


void CMainFrame::CreateExampleModulesMenu()
//-----------------------------------------
{
	static_assert(nMaxItemsInExampleModulesMenu == ID_EXAMPLE_MODULES_LASTINRANGE - ID_EXAMPLE_MODULES,
				  "Make sure that there's a proper range for menu commands in resources.");
	HMENU hMenu = CreateFileMenu(nMaxItemsInExampleModulesMenu, s_ExampleModulePaths, MPT_PATHSTRING("ExampleSongs\\"), ID_EXAMPLE_MODULES);
	CMenu* const pMainMenu = GetMenu();
	if (hMenu && pMainMenu && m_InputHandler)
		VERIFY(pMainMenu->ModifyMenu(ID_EXAMPLE_MODULES, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)hMenu, m_InputHandler->GetMenuText(ID_EXAMPLE_MODULES)));
	else
		ASSERT(false);
}


// Hack-ish way to get the file menu (this is necessary because the MDI document icon next to the File menu is a sub menu, too).
CMenu *CMainFrame::GetFileMenu() const
//------------------------------------
{
	CMenu *mainMenu = GetMenu();
	CMenu *fileMenu = mainMenu ? mainMenu->GetSubMenu(0) : nullptr;
	if(fileMenu)
	{
		if(fileMenu->GetMenuItemID(1) != ID_FILE_OPEN)
			fileMenu = mainMenu->GetSubMenu(1);
		ASSERT(fileMenu->GetMenuItemID(1) == ID_FILE_OPEN);
	}
	ASSERT(fileMenu);
	return fileMenu;
}


void CMainFrame::CreateTemplateModulesMenu()
//------------------------------------------
{
	static_assert(nMaxItemsInTemplateModulesMenu == ID_FILE_OPENTEMPLATE_LASTINRANGE - ID_FILE_OPENTEMPLATE,
				  "Make sure that there's a proper range for menu commands in resources.");
	HMENU hMenu = CreateFileMenu(nMaxItemsInTemplateModulesMenu, s_TemplateModulePaths, MPT_PATHSTRING("TemplateModules\\"), ID_FILE_OPENTEMPLATE);
	CMenu *pFileMenu = GetFileMenu();
	if (hMenu && pFileMenu && m_InputHandler)
	{
		VERIFY(pFileMenu->RemoveMenu(2, MF_BYPOSITION));
		VERIFY(pFileMenu->InsertMenu(2, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hMenu, m_InputHandler->GetMenuText(ID_FILE_OPENTEMPLATE)));
	}
	else
		ASSERT(false);
}


void CMainFrame::UpdateMRUList()
//------------------------------
{
	CMenu *pMenu = GetFileMenu();
	static int firstMenu = -1;
	if(firstMenu == -1)
	{
		int numMenus = pMenu->GetMenuItemCount();
		for(int i = 0; i < numMenus; i++)
		{
			if(pMenu->GetMenuItemID(i) == ID_MRU_LIST_FIRST)
			{
				firstMenu = i;
				break;
			}
		}
	}

	for(int i = ID_MRU_LIST_FIRST; i <= ID_MRU_LIST_LAST; i++)
	{
		pMenu->DeleteMenu(i, MF_BYCOMMAND);
	}

	if(TrackerSettings::Instance().mruFiles.empty())
	{
		// MFC will automatically ignore if we set MF_GRAYED here because of CFrameWnd::m_bAutoMenuEnable.
		// So we will have to install a ON_UPDATE_COMMAND_UI callback...
		pMenu->InsertMenu(firstMenu, MF_STRING | MF_BYPOSITION, ID_MRU_LIST_FIRST, _T("Recent File"));
	} else
	{
		const mpt::PathString workDir = TrackerSettings::Instance().PathSongs.GetWorkingDir();

		for(size_t i = 0; i < TrackerSettings::Instance().mruFiles.size(); i++)
		{
			std::wstring s = mpt::ToWString(i + 1) + L" ";
			// Add mnemonics
			if(i < 9)
			{
				s = L"&" + s;
			} else if(i == 9)
			{
				s = L"1&0 ";
			}

			const mpt::PathString &pathMPT = TrackerSettings::Instance().mruFiles[i];
			std::wstring path = pathMPT.ToWide();
			if(!mpt::PathString::CompareNoCase(workDir, pathMPT.GetPath()))
			{
				// Only show filename
				path = path.substr(workDir.ToWide().length());
			} else if(path.length() > 30)	// Magic number experimentally determined to be equal to MFC's behaviour
			{
				// Shorten path ("C:\Foo\VeryLongString...\Bar.it" => "C:\Foo\...\Bar.it")
				size_t start = path.find_first_of(L"\\/", path.find_first_of(L"\\/") + 1);
				size_t end = path.find_last_of(L"\\/");
				if(start < end)
				{
					path = path.substr(0, start + 1) + L"..." + path.substr(end);
				}
			}
			path = mpt::String::Replace(path, L"&", L"&&");
			s += path;
			::InsertMenuW(pMenu->m_hMenu, firstMenu + i, MF_STRING | MF_BYPOSITION, ID_MRU_LIST_FIRST + i, s.c_str());
		}
	}
}


// ITfLanguageProfileNotifySink implementation

TfLanguageProfileNotifySink::TfLanguageProfileNotifySink()
//--------------------------------------------------------
	: m_pProfiles(nullptr)
	, m_pSource(nullptr)
	, m_dwCookie(TF_INVALID_COOKIE)
{
	HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfiles, (void**)&m_pProfiles);
	if(SUCCEEDED(hr))
	{
		hr = m_pProfiles->QueryInterface(IID_ITfSource, (void**)&m_pSource);
		if(SUCCEEDED(hr))
		{
			hr = m_pSource->AdviseSink(IID_ITfLanguageProfileNotifySink,
				static_cast<ITfLanguageProfileNotifySink *>(this),
				&m_dwCookie);
			ASSERT(SUCCEEDED(hr));
			ASSERT(m_dwCookie != TF_INVALID_COOKIE);
		}
	}
}


TfLanguageProfileNotifySink::~TfLanguageProfileNotifySink()
//---------------------------------------------------------
{
	if(mpt::Windows::IsWine())
	{
		// Calling UnadviseSink causes a random crash in Wine when computing its function pointer for some reason.
		// Probably a race condition I don't understand, and probably a bug in Wine.
		return;
	}
	if(m_pSource && (m_dwCookie != TF_INVALID_COOKIE))
	{
		m_pSource->UnadviseSink(m_dwCookie);
	}
	m_dwCookie = TF_INVALID_COOKIE;
	if(m_pSource)
	{
		m_pSource->Release();
	}
	m_pSource = nullptr;
	if(m_pProfiles) m_pProfiles->Release();
	m_pProfiles = nullptr;
}


HRESULT TfLanguageProfileNotifySink::OnLanguageChange(LANGID /*langid*/, BOOL *pfAccept)
//--------------------------------------------------------------------------------------
{
	*pfAccept = TRUE;
	return ResultFromScode(S_OK);
}


HRESULT TfLanguageProfileNotifySink::OnLanguageChanged()
//------------------------------------------------------
{
	// Input language has changed, so key positions might have changed too.
	CMainFrame *mainFrm = CMainFrame::GetMainFrame();
	if(mainFrm != nullptr)
	{
		mainFrm->UpdateEffectKeys();
		mainFrm->m_InputHandler->SetModifierMask(0);
	}
	return ResultFromScode(S_OK);
}


HRESULT TfLanguageProfileNotifySink::QueryInterface(REFIID riid, void **ppvObject)
//--------------------------------------------------------------------------------
{
	if(riid == IID_ITfLanguageProfileNotifySink || riid == IID_IUnknown)
	{
		*ppvObject = static_cast<ITfLanguageProfileNotifySink *>(this);
		AddRef();
		return ResultFromScode(S_OK);
	}
	*ppvObject = nullptr;
	return ResultFromScode(E_NOINTERFACE);
}


ULONG TfLanguageProfileNotifySink::AddRef()
//-----------------------------------------
{
	// Don't let COM do anything to this object
	return 1;
}

ULONG TfLanguageProfileNotifySink::Release()
//------------------------------------------
{
	// Don't let COM do anything to this object
	return 1;
}


/////////////////////////////////////////////
//Misc helper functions
/////////////////////////////////////////////

void AddPluginNamesToCombobox(CComboBox& CBox, const SNDMIXPLUGIN*plugarray, const bool librarynames)
//---------------------------------------------------------------------------------------------------
{
#ifndef NO_PLUGINS
	for (PLUGINDEX iPlug = 0; iPlug < MAX_MIXPLUGINS; iPlug++)
	{
		const SNDMIXPLUGIN &plugin = plugarray[iPlug];
		CString str;
		str.Preallocate(80);
		str.Format(_T("FX%u: "), iPlug + 1);
		const int size0 = str.GetLength();
		str += (librarynames) ? mpt::ToCString(mpt::CharsetUTF8, plugin.GetLibraryName()) : mpt::ToCString(mpt::CharsetLocale, plugin.GetName());
		if(str.GetLength() <= size0) str += _T("undefined");

		CBox.SetItemData(CBox.AddString(str), iPlug + 1);
	}
#endif // NO_PLUGINS
}


void AddPluginParameternamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN& plug)
//-------------------------------------------------------------------------
{
#ifndef NO_PLUGINS
	if(plug.pMixPlugin)
		AddPluginParameternamesToCombobox(CBox, *plug.pMixPlugin);
#endif // NO_PLUGINS
}


void AddPluginParameternamesToCombobox(CComboBox& CBox, IMixPlugin& plug)
//-----------------------------------------------------------------------
{
#ifndef NO_PLUGINS
	const PlugParamIndex nParams = plug.GetNumParameters();
	for (PlugParamIndex i = 0; i < nParams; i++)
	{
		CBox.SetItemData(CBox.AddString(plug.GetFormattedParamName(i)), i);
	}
#endif // NO_PLUGINS
}


OPENMPT_NAMESPACE_END
