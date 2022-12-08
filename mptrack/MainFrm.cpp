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
#include "Mainfrm.h"
#include "ModDocTemplate.h"
#include "openmpt/sounddevice/SoundDevice.hpp"
#include "openmpt/sounddevice/SoundDeviceBuffer.hpp"
#include "openmpt/sounddevice/SoundDeviceManager.hpp"
#include "../soundlib/AudioReadTarget.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "Childfrm.h"
#include "Dlsbank.h"
#include "Mpdlgs.h"
#include "FolderScanner.h"
#include "KeyConfigDlg.h"
#include "PathConfigDlg.h"
#include "GeneralConfigDlg.h"
#include "ColorConfigDlg.h"
#include "SampleConfigDlg.h"
#include "AdvancedConfigDlg.h"
#include "AutoSaver.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Globals.h"
#include "ChannelManagerDlg.h"
#include "../common/version.h"
#include "UpdateCheck.h"
#include "CloseMainDialog.h"
#include "SelectPluginDialog.h"
#include "PatternClipboard.h"
#include "PatternFont.h"
#include "../common/mptFileIO.h"
#include "mpt/fs/fs.hpp"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "../common/FileReader.h"
#include "../common/Profiler.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/plugins/PluginManager.h"
#include "Vstplug.h"
#include "FileDialog.h"
#include "ProgressDialog.h"
#include <HtmlHelp.h>
#include <Dbt.h>  // device change messages
#include "mpt/audio/span.hpp"


OPENMPT_NAMESPACE_BEGIN

#define TIMERID_GUI 1
#define TIMERID_NOTIFY 2

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
	ON_WM_DEVICECHANGE()
	ON_WM_DROPFILES()
	ON_WM_QUERYENDSESSION()
	ON_COMMAND(ID_VIEW_OPTIONS,				&CMainFrame::OnViewOptions)

	ON_COMMAND(ID_PLUGIN_SETUP,				&CMainFrame::OnPluginManager)
	ON_COMMAND(ID_CLIPBOARD_MANAGER,		&CMainFrame::OnClipboardManager)
	//ON_COMMAND(ID_HELP,					CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_REPORT_BUG,				&CMainFrame::OnReportBug)
	ON_COMMAND(ID_NEXTOCTAVE,				&CMainFrame::OnNextOctave)
	ON_COMMAND(ID_PREVOCTAVE,				&CMainFrame::OnPrevOctave)
	ON_COMMAND_RANGE(ID_FILE_OPENTEMPLATE, ID_FILE_OPENTEMPLATE_LASTINRANGE, &CMainFrame::OnOpenTemplateModule)
	ON_COMMAND(ID_ADD_SOUNDBANK,			&CMainFrame::OnAddDlsBank)
	ON_COMMAND(ID_IMPORT_MIDILIB,			&CMainFrame::OnImportMidiLib)
	ON_COMMAND(ID_MIDI_RECORD,				&CMainFrame::OnMidiRecord)
	ON_COMMAND(ID_PANIC,					&CMainFrame::OnPanic)
	ON_COMMAND(ID_PLAYER_PAUSE,				&CMainFrame::OnPlayerPause)
	ON_COMMAND_RANGE(ID_EXAMPLE_MODULES, ID_EXAMPLE_MODULES_LASTINRANGE, &CMainFrame::OnExampleSong)
	ON_COMMAND_EX(IDD_TREEVIEW,				&CMainFrame::OnBarCheck)
	ON_COMMAND_EX(ID_NETLINK_MODPLUG,		&CMainFrame::OnInternetLink)
	ON_COMMAND_EX(ID_NETLINK_TOP_PICKS,		&CMainFrame::OnInternetLink)
	ON_UPDATE_COMMAND_UI(ID_MIDI_RECORD,	&CMainFrame::OnUpdateMidiRecord)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TIME,	&CMainFrame::OnUpdateTime)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_USER,	&CMainFrame::OnUpdateUser)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INFO,	&CMainFrame::OnUpdateInfo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_XINFO,&CMainFrame::OnUpdateXInfo)
	ON_UPDATE_COMMAND_UI(IDD_TREEVIEW,		&CMainFrame::OnUpdateControlBarMenu)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		&CMainFrame::OnUpdatePosition)
	ON_MESSAGE(WM_MOD_INVALIDATEPATTERNS,	&CMainFrame::OnInvalidatePatterns)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,			&CMainFrame::OnCustomKeyMsg)
	ON_MESSAGE(WM_MOD_MIDIMAPPING,			&CMainFrame::OnViewMIDIMapping)
	ON_MESSAGE(WM_MOD_UPDATEVIEWS,			&CMainFrame::OnUpdateViews)
	ON_MESSAGE(WM_MOD_SETMODIFIED,			&CMainFrame::OnSetModified)
#if defined(MPT_ENABLE_UPDATE)
	ON_MESSAGE(WM_MOD_UPDATENOTIFY,			&CMainFrame::OnToolbarUpdateIndicatorClick)
#endif // MPT_ENABLE_UPDATE
	ON_COMMAND(ID_INTERNETUPDATE,			&CMainFrame::OnInternetUpdate)
	ON_COMMAND(ID_UPDATE_AVAILABLE,			&CMainFrame::OnUpdateAvailable)
	ON_COMMAND(ID_HELP_SHOWSETTINGSFOLDER,	&CMainFrame::OnShowSettingsFolder)
#if defined(MPT_ENABLE_UPDATE)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_START, &CMainFrame::OnUpdateCheckStart)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_PROGRESS, &CMainFrame::OnUpdateCheckProgress)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_CANCELED, &CMainFrame::OnUpdateCheckCanceled)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_FAILURE, &CMainFrame::OnUpdateCheckFailure)
	ON_MESSAGE(MPT_WM_APP_UPDATECHECK_SUCCESS, &CMainFrame::OnUpdateCheckSuccess)
#endif // MPT_ENABLE_UPDATE
	ON_COMMAND(ID_HELPSHOW,					&CMainFrame::OnHelp)

	ON_COMMAND_RANGE(ID_MRU_LIST_FIRST, ID_MRU_LIST_LAST, &CMainFrame::OnOpenMRUItem)
	ON_UPDATE_COMMAND_UI(ID_MRU_LIST_FIRST,	&CMainFrame::OnUpdateMRUItem)
	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEWHEEL()
	ON_WM_SHOWWINDOW()
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

// Globals
OptionsPage CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_DEFAULT;
HHOOK CMainFrame::ghKbdHook = NULL;

// GDI
HICON CMainFrame::m_hIcon = NULL;
HFONT CMainFrame::m_hGUIFont = NULL;
HFONT CMainFrame::m_hFixedFont = NULL;
HPEN CMainFrame::penDarkGray = NULL;
HPEN CMainFrame::penGray99 = NULL;
HPEN CMainFrame::penHalfDarkGray = NULL;

HCURSOR CMainFrame::curDragging = NULL;
HCURSOR CMainFrame::curArrow = NULL;
HCURSOR CMainFrame::curNoDrop = NULL;
HCURSOR CMainFrame::curNoDrop2 = NULL;
HCURSOR CMainFrame::curVSplit = NULL;
MODPLUGDIB *CMainFrame::bmpNotes = nullptr;
MODPLUGDIB *CMainFrame::bmpVUMeters = nullptr;
MODPLUGDIB *CMainFrame::bmpPluginVUMeters = nullptr;
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
	: SoundDevice::CallbackBufferHandler<DithersOpenMPT>(theApp.PRNG())
	, m_SoundDeviceFillBufferCriticalSection(CriticalSection::InitialState::Unlocked)
{
	m_szUserText[0] = 0;
	m_szInfoText[0] = 0;
	m_szXInfoText[0]= 0;

	MemsetZero(gcolrefVuMeter);

	m_InputHandler = new CInputHandler(this);

}


void CMainFrame::Initialize()
{
	//Adding version number to the frame title
	CString title = GetTitle();
	title += _T(" ") + mpt::cfmt::val(Version::Current());
	#if defined(MPT_BUILD_RETRO)
		title += _T(" RETRO");
	#endif // MPT_BUILD_RETRO
	if(Build::IsDebugBuild())
	{
		title += _T(" DEBUG");
	}
	#ifndef MPT_WITH_VST
		title += _T(" NO_VST");
	#endif
	#ifndef MPT_WITH_DMO
		title += _T(" NO_DMO");
	#endif
	#ifdef NO_PLUGINS
		title += _T(" NO_PLUGINS");
	#endif
	SetTitle(title);
	OnUpdateFrameTitle(false);

	// Check for valid sound device
	SoundDevice::Identifier dev = TrackerSettings::Instance().GetSoundDeviceIdentifier();
	if(!theApp.GetSoundDevicesManager()->FindDeviceInfo(dev).IsValid())
	{
		dev = theApp.GetSoundDevicesManager()->FindDeviceInfoBestMatch(dev).GetIdentifier();
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
{
	delete m_InputHandler;
	CChannelManagerDlg::DestroySharedInstance();
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	// Load resources
	m_hIcon = theApp.LoadIcon(IDR_MAINFRAME);

	// Toolbar and other icons
	CDC *dc = GetDC();
	const double scaling = Util::GetDPIx(m_hWnd) / 96.0;
	static constexpr int miscIconsInvert[] = {IMAGE_PATTERNS, IMAGE_OPLINSTRACTIVE, IMAGE_OPLINSTRMUTE};
	static constexpr int patternIconsInvert[] = {TIMAGE_PREVIEW, TIMAGE_MACROEDITOR, TIMAGE_PATTERN_OVERFLOWPASTE, TIMAGE_PATTERN_DETAIL_LO, TIMAGE_PATTERN_DETAIL_MED, TIMAGE_PATTERN_DETAIL_HI, TIMAGE_PATTERN_PLUGINS, TIMAGE_SAMPLE_UNSIGN};
	static constexpr int envelopeIconsInvert[] = {IIMAGE_CHECKED, IIMAGE_VOLSWITCH, IIMAGE_PANSWITCH, IIMAGE_PITCHSWITCH, IIMAGE_FILTERSWITCH, IIMAGE_NOPITCHSWITCH, IIMAGE_NOFILTERSWITCH};
	m_MiscIcons.Create(IDB_IMAGELIST, 16, 16, IMGLIST_NUMIMAGES, 1, dc, scaling, false, miscIconsInvert);
	m_MiscIconsDisabled.Create(IDB_IMAGELIST, 16, 16, IMGLIST_NUMIMAGES, 1, dc, scaling, true, miscIconsInvert);
	m_PatternIcons.Create(IDB_PATTERNS, 16, 16, PATTERNIMG_NUMIMAGES, 1, dc, scaling, false, patternIconsInvert);
	m_PatternIconsDisabled.Create(IDB_PATTERNS, 16, 16, PATTERNIMG_NUMIMAGES, 1, dc, scaling, true, patternIconsInvert);
	m_EnvelopeIcons.Create(IDB_ENVTOOLBAR, 20, 18, ENVIMG_NUMIMAGES, 1, dc, scaling, false, envelopeIconsInvert);
	m_SampleIcons.Create(IDB_SMPTOOLBAR, 20, 18, SAMPLEIMG_NUMIMAGES, 1, dc, scaling, false);
	ReleaseDC(dc);

	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(metrics);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
	m_hGUIFont = CreateFontIndirect(&metrics.lfMessageFont);

	penDarkGray = ::CreatePen(PS_SOLID, 0, GetSysColor(COLOR_BTNSHADOW));
	penGray99 = ::CreatePen(PS_SOLID,0, RGB(0x99, 0x99, 0x99));
	penHalfDarkGray = ::CreatePen(PS_DOT, 0, GetSysColor(COLOR_BTNSHADOW));

	// Cursors
	curDragging = theApp.LoadCursor(IDC_DRAGGING);
	curArrow = theApp.LoadStandardCursor(IDC_ARROW);
	curNoDrop = theApp.LoadStandardCursor(IDC_NO);
	curNoDrop2 = theApp.LoadCursor(IDC_NODRAG);
	curVSplit = theApp.LoadCursor(AFX_IDC_HSPLITBAR);
	// bitmaps
	bmpNotes = LoadDib(MAKEINTRESOURCE(IDB_PATTERNVIEW));
	bmpVUMeters = LoadDib(MAKEINTRESOURCE(IDB_VUMETERS));
	bmpPluginVUMeters = LoadDib(MAKEINTRESOURCE(IDB_VUMETERS));
	// Toolbars
	EnableDocking(CBRS_ALIGN_ANY);
	if (!m_wndToolBar.Create(this)) return -1;
	if (!m_wndStatusBar.Create(this)) return -1;
	if (!m_wndTree.Create(this, IDD_TREEVIEW, CBRS_LEFT|CBRS_BORDER_RIGHT, IDD_TREEVIEW)) return -1;
	m_wndStatusBar.SetIndicators(indicators, mpt::saturate_cast<int>(std::size(indicators)));
	m_wndStatusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 256);
	m_wndToolBar.Init(this);
	m_wndTree.RecalcLayout();

	RemoveControlBar(&m_wndStatusBar); //Removing statusbar because corrupted statusbar inifiledata can crash the program.
	m_wndTree.EnableDocking(0); //To prevent a crash when there's "Docking=1" for treebar in ini-settings.
	// Restore toobar positions
	LoadBarState(_T("Toolbars"));

	AddControlBar(&m_wndStatusBar); //Restore statusbar to mainframe.

	UpdateColors();

	if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_ENABLE_RECORD_DEFAULT) midiOpenDevice(false);

#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:6387) // '_Param_(2)' could be '0':  this does not adhere to the specification for the function 'HtmlHelpW'
#endif
	::HtmlHelp(m_hWnd, nullptr, HH_INITIALIZE, reinterpret_cast<DWORD_PTR>(&helpCookie));
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif

	return 0;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	return CMDIFrameWnd::PreCreateWindow(cs);
}


BOOL CMainFrame::DestroyWindow()
{
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:6387) // '_Param_(2)' could be '0':  this does not adhere to the specification for the function 'HtmlHelpW'
#endif
	::HtmlHelp(m_hWnd, nullptr, HH_UNINITIALIZE, reinterpret_cast<DWORD_PTR>(&helpCookie));
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif

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
	delete bmpNotes;
	bmpNotes = nullptr;
	
	delete bmpVUMeters;
	bmpVUMeters = nullptr;
	
	delete bmpPluginVUMeters;
	bmpPluginVUMeters = nullptr;

	PatternFont::DeleteFontData();

	// Kill GDI Objects
#define DeleteGDIObject(h) ::DeleteObject(h); h = NULL;
	DeleteGDIObject(penDarkGray);
	DeleteGDIObject(m_hGUIFont);
	DeleteGDIObject(m_hFixedFont);
	DeleteGDIObject(penGray99);
#undef DeleteGDIObject

	return CMDIFrameWnd::DestroyWindow();
}


void CMainFrame::OnClose()
{
	MPT_TRACE_SCOPE();
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
	SaveBarState(_T("Toolbars"));
	AddControlBar(&m_wndStatusBar); // Restore statusbar to mainframe.
	TrackerSettings::Instance().SaveSettings();

	if(m_InputHandler && m_InputHandler->m_activeCommandSet)
	{
		m_InputHandler->m_activeCommandSet->SaveFile(TrackerSettings::Instance().m_szKbdFile);
	}

	EndWaitCursor();
	CMDIFrameWnd::OnClose();
}


BOOL CMainFrame::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	if(nEventType == DBT_DEVNODES_CHANGED && shMidiIn)
	{
		// Calling this (or most other MIDI input related functions) makes the MIDI driver realize
		// that the connection to USB MIDI devices was lost and send a MIM_CLOSE message.
		// Otherwise, after disconnecting a USB MIDI device, the MIDI callback will stay alive forever but return no data.
		midiInGetNumDevs();
	}
	return CMDIFrameWnd::OnDeviceChange(nEventType, dwData);
}


// Drop files from Windows
void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	const UINT nFiles = ::DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0);
	CMainFrame::GetMainFrame()->SetForegroundWindow();
	for(UINT f = 0; f < nFiles; f++)
	{
		UINT size = ::DragQueryFile(hDropInfo, f, nullptr, 0) + 1;
		std::vector<TCHAR> fileName(size, L'\0');
		if(::DragQueryFile(hDropInfo, f, fileName.data(), size))
		{
			const mpt::PathString file = mpt::PathString::FromNative(fileName.data());
#ifdef MPT_BUILD_DEBUG
			// Debug Hack: Quickly scan a folder containing module files (without running out of window handles ;)
			if(m_InputHandler->CtrlPressed() && m_InputHandler->AltPressed() && m_InputHandler->ShiftPressed() && mpt::native_fs{}.is_directory(file))
			{
				FolderScanner scanner(file, FolderScanner::kOnlyFiles | FolderScanner::kFindInSubDirectories);
				mpt::PathString scanName;
				size_t failed = 0, total = 0;
				while(scanner.Next(scanName))
				{
					mpt::IO::InputFile inputFile(scanName, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
					if(!inputFile.IsValid())
						continue;
					auto sndFile = std::make_unique<CSoundFile>();
					MPT_LOG_GLOBAL(LogDebug, "info", U_("Loading ") + scanName.ToUnicode());
					if(!sndFile->Create(GetFileReader(inputFile), CSoundFile::loadCompleteModule, nullptr))
					{
						MPT_LOG_GLOBAL(LogDebug, "info", U_("FAILED: ") + scanName.ToUnicode());
						failed++;
					}
					total++;
				}
				Reporting::Information(MPT_UFORMAT("Scanned {} files, {} failed")(total, failed));
				break;
			}
#endif
			theApp.OpenDocumentFile(file.ToCString());
		}
	}
	::DragFinish(hDropInfo);
}


void CMainFrame::OnActivateApp(BOOL active, DWORD /*threadID*/)
{
	// Ensure modifiers are reset when we leave the window (e.g. Alt-Tab)
	if(!active)
		m_InputHandler->SetModifierMask(ModNone);
}


LRESULT CALLBACK CMainFrame::KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{

	static bool s_KeyboardHookReentryFlag = false; // work-around for https://bugs.openmpt.org/view.php?id=713

	if(mpt::OS::Windows::IsWine()) // work-around for https://bugs.openmpt.org/view.php?id=713
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
				// Exit early without calling our hook when re-entering.
				// We still need to call other hooks though.
				return CallNextHookEx(ghKbdHook, code, wParam, lParam); // required by spec
			}
			s_KeyboardHookReentryFlag = true;
		}
	}

	bool skipFurtherProcessing = false;

	if(code >= 0)
	{
		// Check if textbox has focus
		const bool handledByTextBox = m_InputHandler->IsKeyPressHandledByTextBox(static_cast<DWORD>(wParam), ::GetFocus());
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

	if(mpt::OS::Windows::IsWine()) // work-around for https://bugs.openmpt.org/view.php?id=713
	{
		if(theApp.InGuiThread())
		{
			s_KeyboardHookReentryFlag = false;
		}
	}

	return result;
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
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
{
	if ((GetStyle() & FWS_ADDTOTITLE) == 0)	return;     // leave it alone!

	CMDIChildWnd* pActiveChild = nullptr;
	CDocument* pDocument = GetActiveDocument();
	if (bAddToTitle &&
	  (pActiveChild = MDIGetActive()) != nullptr &&
	  (pActiveChild->GetStyle() & WS_MAXIMIZE) == 0 &&
	  (pDocument != nullptr ||
	   (pDocument = pActiveChild->GetActiveDocument()) != nullptr))
	{
		CString title = pDocument->GetTitle();
		if(pDocument->IsModified())
			title.AppendChar(_T('*'));
		UpdateFrameTitleForDocument(title);
	} else
	{
		LPCTSTR lpstrTitle = nullptr;
		CString strTitle;

		if (pActiveChild != nullptr)
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
{
	MPT_TRACE_SCOPE();
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
		mpt::lock_guard<mpt::mutex> lock(m_NotificationBufferMutex);
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
{
	MPT_TRACE();
	Reporting::Message(level, str);
}


void CMainFrame::SoundCallbackPreStart()
{
	MPT_TRACE();
	m_SoundDeviceClock.SetResolution(1);
}


void CMainFrame::SoundCallbackPostStop()
{
	MPT_TRACE();
	m_SoundDeviceClock.SetResolution(0);
}


uint64 CMainFrame::SoundCallbackGetReferenceClockNowNanoseconds() const
{
	MPT_TRACE();
	MPT_ASSERT(!InAudioThread());
	return m_SoundDeviceClock.NowNanoseconds();
}


uint64 CMainFrame::SoundCallbackLockedGetReferenceClockNowNanoseconds() const
{
	MPT_TRACE();
	MPT_ASSERT(InAudioThread());
	return m_SoundDeviceClock.NowNanoseconds();
}


bool CMainFrame::SoundCallbackIsLockedByCurrentThread() const
{
	MPT_TRACE();
	return theApp.GetGlobalMutexRef().IsLockedByCurrentThread();
}


void CMainFrame::SoundCallbackLock()
{
	MPT_TRACE_SCOPE();
	m_SoundDeviceFillBufferCriticalSection.Enter();
	MPT_ASSERT_ALWAYS(m_pSndFile != nullptr);
	m_AudioThreadId = GetCurrentThreadId();
	mpt::log::Trace::SetThreadId(mpt::log::Trace::ThreadKindAudio, m_AudioThreadId);
}


void CMainFrame::SoundCallbackUnlock()
{
	MPT_TRACE_SCOPE();
	MPT_ASSERT_ALWAYS(m_pSndFile != nullptr);
	m_AudioThreadId = 0;
	m_SoundDeviceFillBufferCriticalSection.Leave();
}


class BufferInputWrapper
	: public IAudioSource
{
private:
	SoundDevice::CallbackBuffer<DithersOpenMPT> &callbackBuffer;
public:
	inline BufferInputWrapper(SoundDevice::CallbackBuffer<DithersOpenMPT> &callbackBuffer_)
		: callbackBuffer(callbackBuffer_)
	{
		return;
	}
	inline void Process(mpt::audio_span_planar<MixSampleInt> buffer) override
	{
		callbackBuffer.template ReadFixedPoint<MixSampleIntTraits::mix_fractional_bits>(buffer);
	}
	inline void Process(mpt::audio_span_planar<MixSampleFloat> buffer) override
	{
		callbackBuffer.Read(buffer);
	}
};


class BufferOutputWrapper
	: public IAudioTarget
{
private:
	SoundDevice::CallbackBuffer<DithersOpenMPT> &callbackBuffer;
public:
	inline BufferOutputWrapper(SoundDevice::CallbackBuffer<DithersOpenMPT> &callbackBuffer_)
		: callbackBuffer(callbackBuffer_)
	{
		return;
	}
	inline void Process(mpt::audio_span_interleaved<MixSampleInt> buffer) override
	{
		callbackBuffer.template WriteFixedPoint<MixSampleIntTraits::mix_fractional_bits>(buffer);
	}
	inline void Process(mpt::audio_span_interleaved<MixSampleFloat> buffer) override
	{
		callbackBuffer.Write(buffer);
	}
};


void CMainFrame::SoundCallbackLockedProcessPrepare(SoundDevice::TimeInfo timeInfo)
{
	MPT_TRACE_SCOPE();
	MPT_ASSERT(InAudioThread());
	TimingInfo timingInfo;
	timingInfo.OutputLatency = timeInfo.Latency;
	timingInfo.StreamFrames = timeInfo.SyncPointStreamFrames;
	timingInfo.SystemTimestamp = timeInfo.SyncPointSystemTimestamp;
	timingInfo.Speed = timeInfo.Speed;
	m_pSndFile->m_TimingInfo = timingInfo;
}


void CMainFrame::SoundCallbackLockedCallback(SoundDevice::CallbackBuffer<DithersOpenMPT> &buffer)
{
	MPT_TRACE_SCOPE();
	MPT_ASSERT(InAudioThread());
	OPENMPT_PROFILE_FUNCTION(Profiler::Audio);
	BufferInputWrapper source(buffer);
	BufferOutputWrapper target(buffer);
	MPT_ASSERT(buffer.GetNumFrames() <= std::numeric_limits<CSoundFile::samplecount_t>::max());
	CSoundFile::samplecount_t framesToRender = static_cast<CSoundFile::samplecount_t>(buffer.GetNumFrames());
	MPT_ASSERT(framesToRender > 0);
	CSoundFile::samplecount_t renderedFrames = m_pSndFile->Read(framesToRender, target, source, std::ref(m_VUMeterOutput), std::ref(m_VUMeterInput));
	MPT_ASSERT(renderedFrames <= framesToRender);
	[[maybe_unused]] CSoundFile::samplecount_t remainingFrames = framesToRender - renderedFrames;
	MPT_ASSERT(remainingFrames >= 0); // remaining buffer is filled with silence automatically
}


void CMainFrame::SoundCallbackLockedProcessDone(SoundDevice::TimeInfo timeInfo)
{
	MPT_TRACE_SCOPE();
	MPT_ASSERT(InAudioThread());
	OPENMPT_PROFILE_FUNCTION(Profiler::Notify);
	MPT_ASSERT((timeInfo.RenderStreamPositionAfter.Frames - timeInfo.RenderStreamPositionBefore.Frames) < std::numeric_limits<CSoundFile::samplecount_t>::max());
	CSoundFile::samplecount_t framesRendered = static_cast<CSoundFile::samplecount_t>(timeInfo.RenderStreamPositionAfter.Frames - timeInfo.RenderStreamPositionBefore.Frames);
	int64 streamPosition = timeInfo.RenderStreamPositionAfter.Frames;
	DoNotification(framesRendered, streamPosition);
	//m_pSndFile->m_TimingInfo = TimingInfo(); // reset
}


bool CMainFrame::IsAudioDeviceOpen() const
{
	MPT_TRACE_SCOPE();
	return gpSoundDevice && gpSoundDevice->IsOpen();
}


bool CMainFrame::audioOpenDevice()
{
	MPT_TRACE_SCOPE();
	const SoundDevice::Identifier deviceIdentifier = TrackerSettings::Instance().GetSoundDeviceIdentifier();
	if(!TrackerSettings::Instance().GetMixerSettings().IsValid())
	{
		Reporting::Error(MPT_UFORMAT("Unable to open sound device '{}': Invalid mixer settings.")(deviceIdentifier));
		return false;
	}
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
		Reporting::Error(MPT_UFORMAT("Unable to open sound device '{}': Could not find sound device.")(deviceIdentifier));
		return false;
	}
	gpSoundDevice->SetMessageReceiver(this);
	gpSoundDevice->SetCallback(this);
	SoundDevice::Settings deviceSettings = TrackerSettings::Instance().GetSoundDeviceSettings(deviceIdentifier);
	if(!gpSoundDevice->Open(deviceSettings))
	{
		if(!gpSoundDevice->IsAvailable())
		{
			Reporting::Error(MPT_UFORMAT("Unable to open sound device '{}': Device not available.")(gpSoundDevice->GetDeviceInfo().GetDisplayName()));
		} else
		{
			Reporting::Error(MPT_UFORMAT("Unable to open sound device '{}'.")(gpSoundDevice->GetDeviceInfo().GetDisplayName()));
		}
		return false;
	}
	SampleFormat actualSampleFormat = gpSoundDevice->GetActualSampleFormat();
	deviceSettings.sampleFormat = actualSampleFormat;
	Dithers().SetMode(deviceSettings.DitherType, deviceSettings.Channels);
	TrackerSettings::Instance().MixerSamplerate = gpSoundDevice->GetSettings().Samplerate;
	TrackerSettings::Instance().SetSoundDeviceSettings(deviceIdentifier, deviceSettings);
	return true;
}


void CMainFrame::audioCloseDevice()
{
	MPT_TRACE_SCOPE();
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


void VUMeter::Process(Channel &c, MixSampleInt sample)
{
	c.peak = std::max(c.peak, std::abs(sample));
	if(sample < MixSampleIntTraits::mix_clip_min || MixSampleIntTraits::mix_clip_max < sample)
	{
		c.clipped = true;
	}
}


void VUMeter::Process(Channel &c, MixSampleFloat sample)
{
	Process(c, SC::ConvertToFixedPoint<MixSampleInt, MixSampleFloat, MixSampleIntTraits::mix_fractional_bits>()(sample));
}


void VUMeter::Process(mpt::audio_span_interleaved<const MixSampleInt> buffer)
{
	for(std::size_t frame = 0; frame < buffer.size_frames(); ++frame)
	{
		for(std::size_t channel = 0; channel < std::min(buffer.size_channels(), maxChannels); ++channel)
		{
			Process(channels[channel], buffer(channel, frame));
		}
	}
	for(std::size_t channel = std::min(buffer.size_channels(), maxChannels); channel < maxChannels; ++channel)
	{
		channels[channel] = Channel();
	}
}


void VUMeter::Process(mpt::audio_span_interleaved<const MixSampleFloat> buffer)
{
	for(std::size_t frame = 0; frame < buffer.size_frames(); ++frame)
	{
		for(std::size_t channel = 0; channel < std::min(buffer.size_channels(), maxChannels); ++channel)
		{
			Process(channels[channel], buffer(channel, frame));
		}
	}
	for(std::size_t channel = std::min(buffer.size_channels(), maxChannels); channel < maxChannels; ++channel)
	{
		channels[channel] = Channel();
	}
}


void VUMeter::Process(mpt::audio_span_planar<const MixSampleInt> buffer)
{
	for(std::size_t frame = 0; frame < buffer.size_frames(); ++frame)
	{
		for(std::size_t channel = 0; channel < std::min(buffer.size_channels(), maxChannels); ++channel)
		{
			Process(channels[channel], buffer(channel, frame));
		}
	}
	for(std::size_t channel = std::min(buffer.size_channels(), maxChannels); channel < maxChannels; ++channel)
	{
		channels[channel] = Channel();
	}
}


void VUMeter::Process(mpt::audio_span_planar<const MixSampleFloat> buffer)
{
	for(std::size_t frame = 0; frame < buffer.size_frames(); ++frame)
	{
		for(std::size_t channel = 0; channel < std::min(buffer.size_channels(), maxChannels); ++channel)
		{
			Process(channels[channel], buffer(channel, frame));
		}
	}
	for(std::size_t channel = std::min(buffer.size_channels(), maxChannels); channel < maxChannels; ++channel)
	{
		channels[channel] = Channel();
	}
}


const float VUMeter::dynamicRange = 48.0f; // corresponds to the current implementation of the UI widget displaying the result


void VUMeter::SetDecaySpeedDecibelPerSecond(float decibelPerSecond)
{
	float linearDecayRate = decibelPerSecond / dynamicRange;
	decayParam = mpt::saturate_round<int32>(linearDecayRate * static_cast<float>(MixSampleIntTraits::mix_clip_max));
}


void VUMeter::Decay(int32 secondsNum, int32 secondsDen)
{
	int32 decay = Util::muldivr(decayParam, secondsNum, secondsDen);
	for(std::size_t channel = 0; channel < maxChannels; ++channel)
	{
		channels[channel].peak = std::max(channels[channel].peak - decay, 0);
	}
}


void VUMeter::ResetClipped()
{
	for(std::size_t channel = 0; channel < maxChannels; ++channel)
	{
		channels[channel].clipped = false;
	}
}


static void SetVUMeter(std::array<uint32, 4> &masterVU, const VUMeter &vumeter)
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
{
	MPT_TRACE_SCOPE();
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

	Notification notification(notifyType, notifyItem, streamPosition, m_pSndFile->m_PlayState.m_nRow, m_pSndFile->m_PlayState.m_nTickCount, m_pSndFile->m_PlayState.TicksOnRow(), m_pSndFile->m_PlayState.m_nCurrentOrder, m_pSndFile->m_PlayState.m_nPattern, m_pSndFile->GetMixStat(), static_cast<uint8>(m_pSndFile->m_MixerSettings.gnChannels), static_cast<uint8>(m_pSndFile->m_MixerSettings.NumInputChannels));

	m_pSndFile->ResetMixStat();

	if(m_pSndFile->m_SongFlags[SONG_ENDREACHED]) notification.type.set(Notification::EOS);

	if(notifyType[Notification::Sample])
	{
		// Sample positions
		const SAMPLEINDEX smp = notifyItem;
		if(smp > 0 && smp <= m_pSndFile->GetNumSamples() && m_pSndFile->GetSample(smp).HasSampleData())
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
		SetVUMeter(notification.masterVUin, m_VUMeterInput);
		SetVUMeter(notification.masterVUout, m_VUMeterOutput);
		m_VUMeterInput.Decay(dwSamplesRead, m_pSndFile->m_MixerSettings.gdwMixingFreq);
		m_VUMeterOutput.Decay(dwSamplesRead, m_pSndFile->m_MixerSettings.gdwMixingFreq);

	}

	{
		mpt::lock_guard<mpt::mutex> lock(m_NotificationBufferMutex);
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
	sndFile.SetEQGains(TrackerSettings::Instance().m_EqSettings.Gains, TrackerSettings::Instance().m_EqSettings.Freqs, reset);
#endif
#ifndef NO_DSP
	sndFile.m_BitCrush.m_Settings = TrackerSettings::Instance().m_BitCrushSettings;
#endif
	sndFile.SetDspEffects(TrackerSettings::Instance().MixerDSPMask);
	sndFile.InitPlayer(reset);
}


void CMainFrame::UpdateAudioParameters(CSoundFile &sndFile, bool reset)
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
{
	const auto &colors = TrackerSettings::Instance().rgbCustomColors;
	const struct { MODPLUGDIB *bitmap; uint32 lo, med, hi; } meters[] =
	{
		{ bmpVUMeters, MODCOLOR_VUMETER_LO, MODCOLOR_VUMETER_MED, MODCOLOR_VUMETER_HI },
		{ bmpPluginVUMeters, MODCOLOR_VUMETER_LO_VST, MODCOLOR_VUMETER_MED_VST, MODCOLOR_VUMETER_HI_VST },
	};
	for(auto &meter : meters) if(meter.bitmap != nullptr)
	{
		meter.bitmap->bmiColors[7] = rgb2quad(GetSysColor(COLOR_BTNFACE));
		meter.bitmap->bmiColors[8] = rgb2quad(GetSysColor(COLOR_BTNSHADOW));
		meter.bitmap->bmiColors[15] = rgb2quad(GetSysColor(COLOR_BTNHIGHLIGHT));
		meter.bitmap->bmiColors[10] = rgb2quad(colors[meter.lo]);
		meter.bitmap->bmiColors[11] = rgb2quad(colors[meter.med]);
		meter.bitmap->bmiColors[9] = rgb2quad(colors[meter.hi]);
		meter.bitmap->bmiColors[2] = rgb2quad((colors[meter.lo] >> 1) & 0x7F7F7F);
		meter.bitmap->bmiColors[3] = rgb2quad((colors[meter.med] >> 1) & 0x7F7F7F);
		meter.bitmap->bmiColors[1] = rgb2quad((colors[meter.hi] >> 1) & 0x7F7F7F);
	}

	// Generel tab VU meters
	for(UINT i = 0; i < NUM_VUMETER_PENS * 2; i++)
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
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame operations


UINT CMainFrame::GetBaseOctave() const
{
	return m_wndToolBar.GetBaseOctave();
}


void CMainFrame::ResetNotificationBuffer()
{
	MPT_TRACE();
	mpt::lock_guard<mpt::mutex> lock(m_NotificationBufferMutex);
	m_NotifyBuffer.clear();
}


bool CMainFrame::PreparePlayback()
{
	MPT_TRACE_SCOPE();
	// open the audio device to update needed TrackerSettings mixer parameters
	if(!audioOpenDevice()) return false;
	return true;
}


bool CMainFrame::StartPlayback()
{
	MPT_TRACE_SCOPE();
	if(!m_pSndFile) return false; // nothing to play
	if(!IsAudioDeviceOpen()) return false;
	Dithers().Reset();
	if(!gpSoundDevice->Start()) return false;
	if(!m_NotifyTimer)
	{
		if(TrackerSettings::Instance().GUIUpdateInterval.Get() > 0)
		{
			m_NotifyTimer = SetTimer(TIMERID_NOTIFY, TrackerSettings::Instance().GUIUpdateInterval, NULL);
		} else
		{
			m_NotifyTimer = SetTimer(TIMERID_NOTIFY, std::max(int(1), mpt::saturate_round<int>(gpSoundDevice->GetEffectiveBufferAttributes().UpdateInterval * 1000.0)), NULL);
		}
	}
	return true;
}


void CMainFrame::StopPlayback()
{
	MPT_TRACE_SCOPE();
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
{
	MPT_TRACE_SCOPE();
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
{
	MPT_TRACE_SCOPE();
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
{
	Notification mn(Notification::Stop);
	SendMessage(WM_MOD_UPDATEPOSITION, 0, (LPARAM)&mn);
}


void CMainFrame::UnsetPlaybackSoundFile()
{
	MPT_ASSERT_ALWAYS(!gpSoundDevice || !gpSoundDevice->IsPlaying());
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
				if(m_pSndFile->m_PlayState.Chn[i].isPreviewNote)
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
{
	MPT_ASSERT_ALWAYS(pSndFile);
	m_pSndFile = pSndFile;
}


bool CMainFrame::PlayMod(CModDoc *pModDoc)
{
	MPT_ASSERT_ALWAYS(!theApp.GetGlobalMutexRef().IsLockedByCurrentThread());
	if(!pModDoc) return false;
	CSoundFile &sndFile = pModDoc->GetSoundFile();
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

	m_VUMeterInput = VUMeter();
	m_VUMeterOutput = VUMeter();

	if(!StartPlayback())
	{
		UnsetPlaybackSoundFile();
		return false;
	}

	return true;
}


bool CMainFrame::PauseMod(CModDoc *pModDoc)
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
{
	MPT_ASSERT_ALWAYS(!theApp.GetGlobalMutexRef().IsLockedByCurrentThread());
	if(pModDoc && (pModDoc != GetModPlaying())) return false;
	if(!IsPlaying()) return true;

	PausePlayback();
	SetElapsedTime(0);
	GenerateStopNotification();

	m_pSndFile->ResetPlayPos();
	m_pSndFile->ResetChannels();
	UnsetPlaybackSoundFile();

	StopPlayback();

	return true;
}


bool CMainFrame::StopSoundFile(CSoundFile *pSndFile)
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


bool CMainFrame::PlayDLSInstrument(const CDLSBank &bank, UINT instr, UINT region, ModCommand::NOTE note, int volume)
{
	bool ok = false;
	BeginWaitCursor();
	{
		CriticalSection cs;
		InitPreview();
		if(bank.ExtractInstrument(m_WaveFile, 1, instr, region))
		{
			PreparePreview(note, volume);
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
	if(IsPlaying() && (m_pSndFile == &m_WaveFile))
	{
		return true;
	}
	return PlaySoundFile(&m_WaveFile);
}


bool CMainFrame::PlaySoundFile(const mpt::PathString &filename, ModCommand::NOTE note, int volume)
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
			mpt::IO::InputFile f(filename, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
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
			PreparePreview(note, volume);
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
	if(IsPlaying() && (m_pSndFile == &m_WaveFile))
	{
		return true;
	}
	return PlaySoundFile(&m_WaveFile);
}


bool CMainFrame::PlaySoundFile(CSoundFile &sndFile, INSTRUMENTINDEX nInstrument, SAMPLEINDEX nSample, ModCommand::NOTE note, int volume)
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
		PreparePreview(note, volume);
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
	if(IsPlaying() && (m_pSndFile == &m_WaveFile))
	{
		return true;
	}
	return PlaySoundFile(&m_WaveFile);
}


void CMainFrame::InitPreview()
{
	m_WaveFile.Destroy();
	m_WaveFile.Create(FileReader());
	m_WaveFile.m_nDefaultTempo.Set(125);
	m_WaveFile.m_nDefaultSpeed = 6;
	m_WaveFile.m_nType = MOD_TYPE_MPT;
	m_WaveFile.m_nChannels = 2;
	m_WaveFile.m_nInstruments = 1;
	m_WaveFile.m_nTempoMode = TempoMode::Classic;
	m_WaveFile.Order().assign(1, 0);
	m_WaveFile.Patterns.Insert(0, 2);
	m_WaveFile.m_SongFlags = SONG_LINEARSLIDES;
}


void CMainFrame::PreparePreview(ModCommand::NOTE note, int volume)
{
	m_WaveFile.m_SongFlags.reset(SONG_PAUSED);
	m_WaveFile.SetRepeatCount(-1);
	m_WaveFile.ResetPlayPos();

	const CModDoc *activeDoc = GetActiveDoc();
	if(activeDoc != nullptr && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_NOEXTRALOUD))
	{
		m_WaveFile.SetMixLevels(activeDoc->GetSoundFile().GetMixLevels());
		m_WaveFile.m_nSamplePreAmp = activeDoc->GetSoundFile().m_nSamplePreAmp;
	} else
	{
		// Preview at 0dB
		m_WaveFile.SetMixLevels(MixLevels::v1_17RC3);
		m_WaveFile.m_nSamplePreAmp = static_cast<uint32>(m_WaveFile.GetPlayConfig().getNormalSamplePreAmp());
	}

	// Avoid global volume ramping when trying samples in the treeview.
	m_WaveFile.m_nDefaultGlobalVolume = m_WaveFile.m_PlayState.m_nGlobalVolume = (volume > 0) ? volume : MAX_GLOBAL_VOLUME;

	if(m_WaveFile.Patterns.IsValidPat(0))
	{
		auto m = m_WaveFile.Patterns[0].GetRow(0);
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
	m_WaveFile.InitPlayer(true);
}


HWND CMainFrame::GetFollowSong() const
{
	return GetModPlaying() ? GetModPlaying()->GetFollowWnd() : NULL;
}


void CMainFrame::IdleHandlerSounddevice()
{
	MPT_TRACE_SCOPE();
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
{
	MPT_TRACE_SCOPE();
	return CMainFrame::SetupSoundCard(TrackerSettings::Instance().GetSoundDeviceSettings(TrackerSettings::Instance().GetSoundDeviceIdentifier()), TrackerSettings::Instance().GetSoundDeviceIdentifier(), TrackerSettings::Instance().m_SoundSettingsStopMode, true);
}


BOOL CMainFrame::SetupSoundCard(SoundDevice::Settings deviceSettings, SoundDevice::Identifier deviceIdentifier, SoundDeviceStopMode stoppedMode, bool forceReset)
{
	MPT_TRACE_SCOPE();
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
		TrackerSettings::Instance().MixerNumInputChannels = deviceSettings.InputChannels;
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
{
	CriticalSection cs;
	if(GetSoundFilePlaying()) UpdateAudioParameters(*GetSoundFilePlaying(), FALSE);
	return TRUE;
}


BOOL CMainFrame::SetupMiscOptions()
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


void CMainFrame::SetupMidi(DWORD d, UINT n)
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
{
	CModDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if (pDocTmpl)
	{
		for(auto &doc : *pDocTmpl)
		{
			doc->UpdateAllViews(nullptr, hint, pHint);
		}
	}
}


void CMainFrame::SetUserText(LPCTSTR lpszText)
{
	if (lpszText[0] | m_szUserText[0])
	{
		_tcscpy(m_szUserText, lpszText);
		OnUpdateUser(nullptr);
	}
}


void CMainFrame::SetInfoText(LPCTSTR lpszText)
{
	if (lpszText[0] | m_szInfoText[0])
	{
		_tcscpy(m_szInfoText, lpszText);
		OnUpdateInfo(nullptr);
	}
}


void CMainFrame::SetXInfoText(LPCTSTR lpszText)
{
	if (lpszText[0] | m_szXInfoText[0])
	{
		_tcscpy(m_szXInfoText, lpszText);
		OnUpdateInfo(nullptr);
	}
}


void CMainFrame::SetHelpText(LPCTSTR lpszText)
{
	m_wndStatusBar.SetPaneText(0, lpszText);
}


void CMainFrame::OnDocumentCreated(CModDoc *pModDoc)
{
	m_wndTree.OnDocumentCreated(pModDoc);
	UpdateMRUList();
}


void CMainFrame::OnDocumentClosed(CModDoc *pModDoc)
{
	if (pModDoc == GetModPlaying()) PauseMod();

	m_wndTree.OnDocumentClosed(pModDoc);
}


void CMainFrame::UpdateTree(CModDoc *pModDoc, UpdateHint hint, CObject *pHint)
{
	m_wndTree.OnUpdate(pModDoc, hint, pHint);
}


void CMainFrame::RefreshDlsBanks()
{
	m_wndTree.RefreshDlsBanks();
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnViewOptions()
{
	if (m_bOptionsLocked)
		return;

	CPropertySheet dlg(_T("OpenMPT Setup"), this, m_nLastOptionsPage);
	COptionsGeneral general;
	COptionsSoundcard sounddlg(TrackerSettings::Instance().m_SoundDeviceIdentifier);
	COptionsSampleEditor smpeditor;
	COptionsKeyboard keyboard;
	COptionsColors colors;
	COptionsMixer mixerdlg;
	CMidiSetupDlg mididlg(TrackerSettings::Instance().m_dwMidiSetup, TrackerSettings::Instance().GetCurrentMIDIDevice());
	PathConfigDlg pathsdlg;
#if defined(MPT_ENABLE_UPDATE)
	CUpdateSetupDlg updatedlg;
#endif // MPT_ENABLE_UPDATE
	COptionsAdvanced advanced;
	COptionsWine winedlg;
	dlg.AddPage(&general);
	dlg.AddPage(&sounddlg);
	dlg.AddPage(&mixerdlg);
#if !defined(NO_REVERB) || !defined(NO_DSP) || !defined(NO_EQ) || !defined(NO_AGC)
	COptionsPlayer dspdlg;
	dlg.AddPage(&dspdlg);
#endif
	dlg.AddPage(&smpeditor);
	dlg.AddPage(&keyboard);
	dlg.AddPage(&colors);
	dlg.AddPage(&mididlg);
	dlg.AddPage(&pathsdlg);
#if defined(MPT_ENABLE_UPDATE)
	dlg.AddPage(&updatedlg);
#endif // MPT_ENABLE_UPDATE
	dlg.AddPage(&advanced);
	if(mpt::OS::Windows::IsWine()) dlg.AddPage(&winedlg);
	m_bOptionsLocked = true;
	m_SoundCardOptionsDialog = &sounddlg;
#if defined(MPT_ENABLE_UPDATE)
	m_UpdateOptionsDialog = &updatedlg;
#endif // MPT_ENABLE_UPDATE
	dlg.DoModal();
	m_SoundCardOptionsDialog = nullptr;
#if defined(MPT_ENABLE_UPDATE)
	m_UpdateOptionsDialog = nullptr;
#endif // MPT_ENABLE_UPDATE
	m_bOptionsLocked = false;
	m_wndTree.OnOptionsChanged();
}


void CMainFrame::OnPluginManager()
{
#ifndef NO_PLUGINS
	PLUGINDEX nPlugslot = PLUGINDEX_INVALID;
	CModDoc* pModDoc = GetActiveDoc();

	if (pModDoc)
	{
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		//Find empty plugin slot
		for (PLUGINDEX nPlug = 0; nPlug < MAX_MIXPLUGINS; nPlug++)
		{
			if (sndFile.m_MixPlugins[nPlug].pMixPlugin == nullptr)
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
{
	PatternClipboardDialog::Show();
}


void CMainFrame::OnAddDlsBank()
{
	FileDialog dlg = OpenFileDialog()
		.AllowMultiSelect()
		.ExtensionFilter("All Sound Banks|*.dls;*.sbk;*.sf2;*.sf3;*.sf4;*.mss|"
			"Downloadable Sounds Banks (*.dls)|*.dls;*.mss|"
			"SoundFont 2.0 Banks (*.sf2)|*.sbk;*.sf2;*.sf3;*.sf4|"
			"All Files (*.*)|*.*||");
	if(!dlg.Show()) return;

	BeginWaitCursor();
	bool ok = true;
	for(const auto &file : dlg.GetFilenames())
	{
		ok &= CTrackApp::AddDLSBank(file);
	}
	if(!ok)
	{
		Reporting::Error("At least one selected file was not a valid sound bank.");
	}
	m_wndTree.RefreshDlsBanks();
	EndWaitCursor();
}


void CMainFrame::OnImportMidiLib()
{
	FileDialog dlg = OpenFileDialog()
		.ExtensionFilter("Text and INI files (*.txt,*.ini)|*.txt;*.ini;*.dls;*.sf2;*.sf3;*.sf4;*.sbk|"
			"Downloadable Sound Banks (*.dls)|*.dls;*.mss|"
			"SoundFont 2.0 banks (*.sf2)|*.sbk;*.sf2;*.sf3;*.sf4|"
			"Gravis UltraSound (ultrasnd.ini)|ultrasnd.ini|"
			"All Files (*.*)|*.*||");
	if(!dlg.Show()) return;

	BeginWaitCursor();
	CTrackApp::ImportMidiConfig(dlg.GetFirstFile());
	m_wndTree.RefreshMidiLibrary();
	EndWaitCursor();
}


void CMainFrame::OnTimer(UINT_PTR timerID)
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

	if(m_AutoSaver.IsEnabled())
	{
		bool success = m_AutoSaver.DoSave(curTime);
		if (!success)		// autosave failure; bring up options.
		{
			CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_PATHS;
			OnViewOptions();
		}
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
			auto s = mpt::ToCString(mpt::Charset::ASCII, mpt::afmt::right(6, mpt::afmt::fix(cats[i] * 100.0, 3)) + "% " + catnames[i]);
			dc.DrawText(s, s.GetLength(), &rect, DT_LEFT);
		}

		RECT rect;
		cwnd->GetClientRect(&rect);
		rect.top += Profiler::CategoriesCount * height;
		auto s = mpt::ToCString(mpt::Charset::ASCII, Profiler::DumpProfiles());
		dc.DrawText(s, s.GetLength(), &rect, DT_LEFT);

		cwnd->Detach();
	}
#endif

}


CModDoc *CMainFrame::GetActiveDoc() const
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		return static_cast<CModDoc *>(pMDIActive->GetActiveDocument());
	}
	return nullptr;
}


CView *CMainFrame::GetActiveView() const
{
	CMDIChildWnd *pMDIActive = MDIGetActive();
	if (pMDIActive)
	{
		return pMDIActive->GetActiveView();
	}

	return nullptr;
}


void CMainFrame::SwitchToActiveView()
{
	CWnd *wnd = GetActiveView();
	if (wnd)
	{
		// Hack: If the upper view is active, we only get the "container" (the dialog view with the tabs), not the view itself.
		if(!strcmp(wnd->GetRuntimeClass()->m_lpszClassName, "CModControlView"))
		{
			wnd = static_cast<CModControlView *>(wnd)->GetCurrentControlDlg();
		}
		wnd->SetFocus();
	}
}


void CMainFrame::OnUpdateTime(CCmdUI *)
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
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_USER), m_szUserText, TRUE);
}


void CMainFrame::OnUpdateInfo(CCmdUI *)
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_INFO), m_szInfoText, TRUE);
}


void CMainFrame::OnUpdateXInfo(CCmdUI *)
{
	m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_XINFO), m_szXInfoText, TRUE);
}

void CMainFrame::OnPlayerPause()
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
{
	const UINT nIdBegin = (isTemplateFile) ? ID_FILE_OPENTEMPLATE : ID_EXAMPLE_MODULES;
	const std::vector<mpt::PathString> &vecFilePaths = (isTemplateFile) ? m_TemplateModulePaths : m_ExampleModulePaths;

	const UINT nIndex = nId - nIdBegin;
	if (nIndex < vecFilePaths.size())
	{
		const mpt::PathString& sPath = vecFilePaths[nIndex];
		const bool bExists = mpt::native_fs{}.is_file(sPath);
		CDocument *pDoc = nullptr;
		if(bExists)
		{
			pDoc = theApp.GetModDocTemplate()->OpenTemplateFile(sPath, !isTemplateFile);
		}
		if(!pDoc)
		{
			Reporting::Notification(_T("The file '") + sPath.ToCString() + _T("' ") + (bExists ? _T("exists but can't be read.") : _T("does not exist.")));
		}
	} else
	{
		MPT_ASSERT(nId == UINT(isTemplateFile ? ID_FILE_OPENTEMPLATE_LASTINRANGE : ID_EXAMPLE_MODULES_LASTINRANGE));
		FileDialog::PathList files;
		theApp.OpenModulesDialog(files, isTemplateFile ? theApp.GetConfigPath() + P_("TemplateModules") : theApp.GetInstallPath() + P_("ExampleSongs"));
		for(const auto &file : files)
		{
			theApp.OpenDocumentFile(file.ToCString());
		}
	}
}


void CMainFrame::OnOpenTemplateModule(UINT nId)
{
	OpenMenuItemFile(nId, true/*open template menu file*/);
}


void CMainFrame::OnExampleSong(UINT nId)
{
	OpenMenuItemFile(nId, false/*open example menu file*/);
}


void CMainFrame::OnOpenMRUItem(UINT nId)
{
	theApp.OpenDocumentFile(TrackerSettings::Instance().mruFiles[nId - ID_MRU_LIST_FIRST].ToCString());
}


void CMainFrame::OnUpdateMRUItem(CCmdUI *cmd)
{
	cmd->Enable(!TrackerSettings::Instance().mruFiles.empty());
}


LRESULT CMainFrame::OnInvalidatePatterns(WPARAM, LPARAM)
{
	UpdateAllViews(UpdateHint().MPTOptions());
	return TRUE;
}


LRESULT CMainFrame::OnUpdatePosition(WPARAM, LPARAM lParam)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::GUI);
	m_VUMeterOutput.SetDecaySpeedDecibelPerSecond(TrackerSettings::Instance().VuMeterDecaySpeedDecibelPerSecond); // update in notification update in order to avoid querying the settings framework from inside audio thread
	m_VUMeterInput.SetDecaySpeedDecibelPerSecond(TrackerSettings::Instance().VuMeterDecaySpeedDecibelPerSecond); // update in notification update in order to avoid querying the settings framework from inside audio thread
	Notification *pnotify = (Notification *)lParam;
	if (pnotify)
	{
		if(pnotify->type[Notification::EOS])
		{
			PostMessage(WM_COMMAND, ID_PLAYER_STOP);
		}
		//Log("OnUpdatePosition: row=%d time=%lu\n", pnotify->nRow, pnotify->TimestampSamples);
		if(CModDoc *modDoc = GetModPlaying(); modDoc != nullptr)
		{
			m_wndTree.UpdatePlayPos(modDoc, pnotify);
			if (GetFollowSong())
				::SendMessage(GetFollowSong(), WM_MOD_UPDATEPOSITION, 0, lParam);
			if(m_pSndFile->m_pluginDryWetRatioChanged.any())
			{
				for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
				{
					if(m_pSndFile->m_pluginDryWetRatioChanged[i])
						modDoc->PostMessageToAllViews(WM_MOD_PLUGINDRYWETRATIOCHANGED, i);
				}
				m_pSndFile->m_pluginDryWetRatioChanged.reset();
			}
		}
		m_nMixChn = pnotify->mixedChannels;

		bool duplicateMono = false;
		if(pnotify->masterVUinChannels == 1 && pnotify->masterVUoutChannels > 1)
		{
			duplicateMono = true;
		} else if(pnotify->masterVUoutChannels == 1 && pnotify->masterVUinChannels > 1)
		{
			duplicateMono = true;
		}
		uint8 countChan = 0;
		uint32 vu[VUMeter::maxChannels * 2];
		MemsetZero(vu);
		std::copy(pnotify->masterVUin.begin(), pnotify->masterVUin.begin() + pnotify->masterVUinChannels, vu + countChan);

		countChan += pnotify->masterVUinChannels;
		if(pnotify->masterVUinChannels == 1 && duplicateMono)
		{
			std::copy(pnotify->masterVUin.begin(), pnotify->masterVUin.begin() + 1, vu + countChan);
			countChan += 1;
		}

		std::copy(pnotify->masterVUout.begin(), pnotify->masterVUout.begin() + pnotify->masterVUoutChannels, vu + countChan);
		countChan += pnotify->masterVUoutChannels;
		if(pnotify->masterVUoutChannels == 1 && duplicateMono)
		{
			std::copy(pnotify->masterVUout.begin(), pnotify->masterVUout.begin() + 1, vu + countChan);
			countChan += 1;
		}

		m_wndToolBar.m_VuMeter.SetVuMeter(countChan, vu, pnotify->type[Notification::Stop]);

		m_wndToolBar.SetCurrentSong(m_pSndFile);
	}
	return 0;
}


LRESULT CMainFrame::OnUpdateViews(WPARAM modDoc, LPARAM hint)
{
	CModDoc *doc = reinterpret_cast<CModDoc *>(modDoc);
	CModDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if(pDocTmpl && pDocTmpl->DocumentExists(doc))
	{
		// Since this message is potentially posted, we first need to verify if the document still exists
		doc->UpdateAllViews(nullptr, UpdateHint::FromLPARAM(hint));
	}
	return 0;
}


LRESULT CMainFrame::OnSetModified(WPARAM modDoc, LPARAM)
{
	CModDoc *doc = reinterpret_cast<CModDoc *>(modDoc);
	CModDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if(pDocTmpl && pDocTmpl->DocumentExists(doc))
	{
		// Since this message is potentially posted, we first need to verify if the document still exists
		doc->UpdateFrameCounts();
	}
	return 0;
}


void CMainFrame::OnPanic()
{
	// "Panic button." At the moment, it just resets all VSTi and sample notes.
	if(GetModPlaying())
		GetModPlaying()->OnPanic();
}


void CMainFrame::OnPrevOctave()
{
	UINT n = GetBaseOctave();
	if (n > MIN_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n-1);
}


void CMainFrame::OnNextOctave()
{
	UINT n = GetBaseOctave();
	if (n < MAX_BASEOCTAVE) m_wndToolBar.SetBaseOctave(n+1);
}


void CMainFrame::OnReportBug()
{
	CTrackApp::OpenURL(Build::GetURL(Build::Url::Bugtracker));
}


BOOL CMainFrame::OnInternetLink(UINT nID)
{
	mpt::ustring url;
	switch(nID)
	{
	case ID_NETLINK_MODPLUG:	url = Build::GetURL(Build::Url::Website); break;
	case ID_NETLINK_TOP_PICKS:	url = Build::GetURL(Build::Url::TopPicks); break;
	}
	if(!url.empty())
	{
		return CTrackApp::OpenURL(url) ? TRUE : FALSE;
	}
	return FALSE;
}


void CMainFrame::OnRButtonDown(UINT, CPoint pt)
{
	CMenu Menu;
	ClientToScreen(&pt);
	if (Menu.LoadMenu(IDR_TOOLBARS))
	{
		CMenu *pSubMenu = Menu.GetSubMenu(0);
		if (pSubMenu != nullptr) pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);
	}
}


LRESULT CMainFrame::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
{
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
		case kcFileSaveCopy:
		case kcFileSaveAsWave:
		case kcFileSaveMidi:
		case kcFileSaveOPL:
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
		case kcViewTempoSwing:
		case kcViewMIDImapping:
		case kcViewEditHistory:
		case kcViewChannelManager:
		case kcPlayPatternFromCursor:
		case kcPlayPatternFromStart:
		case kcPlaySongFromCursor:
		case kcPlaySongFromStart:
		case kcPlayPauseSong:
		case kcPlayStopSong:
		case kcPlaySongFromPattern:
		case kcStopSong:
		case kcToggleLoopSong:
		case kcPanic:
		case kcEstimateSongLength:
		case kcApproxRealBPM:
		case kcTempoIncrease:
		case kcTempoDecrease:
		case kcTempoIncreaseFine:
		case kcTempoDecreaseFine:
		case kcSpeedIncrease:
		case kcSpeedDecrease:
		case kcViewToggle:
			{
				CModDoc *modDoc = GetActiveDoc();
				if (modDoc)
					return GetActiveDoc()->OnCustomKeyMsg(wParam, lParam);
				else if(wParam == kcPlayPauseSong || wParam == kcPlayStopSong|| wParam == kcStopSong)
					StopPreview();
				break;
			}

		case kcSwitchToInstrLibrary:
			if(m_bModTreeHasFocus)
				SwitchToActiveView();
			else
				m_wndTree.SetFocus();
			break;

		//if handled neither by MainFrame nor by ModDoc...
		default:
			//If the treeview has focus, post command to the tree view
			if (m_bModTreeHasFocus)
				return m_wndTree.SendMessageToModTree(WM_MOD_KEYCOMMAND, wParam, lParam);
			if (m_pNoteMapHasFocus)
				return m_pNoteMapHasFocus->SendMessage(WM_MOD_KEYCOMMAND, wParam, lParam);
			if (m_pOrderlistHasFocus)
				return m_pOrderlistHasFocus->SendMessage(WM_MOD_KEYCOMMAND, wParam, lParam);

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
			if(wnd)
				return wnd->SendMessage(WM_MOD_KEYCOMMAND, wParam, lParam);
			return kcNull;
	}

	return wParam;
}


void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	m_InputHandler->SetModifierMask(ModNone);
	CMDIFrameWnd::OnInitMenu(pMenu);

}


BOOL CMainFrame::InitRenderer(CSoundFile* pSndFile)
{
	CriticalSection cs;
	pSndFile->m_bIsRendering = true;
	pSndFile->SuspendPlugins();
	pSndFile->ResumePlugins();
	return true;
}


BOOL CMainFrame::StopRenderer(CSoundFile* pSndFile)
{
	CriticalSection cs;
	pSndFile->SuspendPlugins();
	pSndFile->m_bIsRendering = false;
	return true;
}


// We have switched focus to a new module - might need to update effect keys to reflect module type
bool CMainFrame::UpdateEffectKeys(const CModDoc *modDoc)
{
	if(modDoc != nullptr)
	{
		return m_InputHandler->SetEffectLetters(modDoc->GetSoundFile().GetModSpecifications());
	}
	return false;
}


void CMainFrame::OnKillFocus(CWnd* pNewWnd)
{
	CMDIFrameWnd::OnKillFocus(pNewWnd);

	//rewbs: ensure modifiers are reset when we leave the window (e.g. alt-tab)
	m_InputHandler->SetModifierMask(ModNone);
}


void CMainFrame::OnShowWindow(BOOL bShow, UINT /*nStatus*/)
{
	static bool firstShow = true;
	if (bShow && !IsWindowVisible() && firstShow)
	{
		firstShow = false;
		WINDOWPLACEMENT wpl;
		GetWindowPlacement(&wpl);
		wpl = theApp.GetSettings().Read<WINDOWPLACEMENT>(U_("Display"), U_("WindowPlacement"), wpl);
		SetWindowPlacement(&wpl);
	}
}


void CMainFrame::OnInternetUpdate()
{
#if defined(MPT_ENABLE_UPDATE)
	CUpdateCheck::DoManualUpdateCheck();
#endif // MPT_ENABLE_UPDATE
}


void CMainFrame::OnUpdateAvailable()
{
#if defined(MPT_ENABLE_UPDATE)
	if(m_updateCheckResult)
		CUpdateCheck::ShowSuccessGUI(false, *m_updateCheckResult);
	else
		CUpdateCheck::DoManualUpdateCheck();
#endif  // MPT_ENABLE_UPDATE
}


void CMainFrame::OnShowSettingsFolder()
{
	theApp.OpenDirectory(theApp.GetConfigPath());
}



class CUpdateCheckProgressDialog
	: public CProgressDialog
{
public:
	CUpdateCheckProgressDialog(CWnd *parent)
		: CProgressDialog(parent)
	{
		return;
	}
	void Run() override
	{
	}
};

static std::unique_ptr<CUpdateCheckProgressDialog> g_UpdateCheckProgressDialog = nullptr;


#if defined(MPT_ENABLE_UPDATE)


bool CMainFrame::ShowUpdateIndicator(const UpdateCheckResult &result, const CString &releaseVersion, const CString &infoURL, bool showHighlight)
{
	m_updateCheckResult = std::make_unique<UpdateCheckResult>(result);
	if(m_wndToolBar.IsVisible())
	{
		return m_wndToolBar.ShowUpdateInfo(releaseVersion, infoURL, showHighlight);
	} else
	{
		GetMenu()->RemoveMenu(ID_UPDATE_AVAILABLE, MF_BYCOMMAND);
		GetMenu()->AppendMenu(MF_STRING, ID_UPDATE_AVAILABLE, _T("[Update Available]"));
		DrawMenuBar();
		return true;
	}
}


LRESULT CMainFrame::OnUpdateCheckStart(WPARAM wparam, LPARAM lparam)
{
	GetMenu()->RemoveMenu(ID_UPDATE_AVAILABLE, MF_BYCOMMAND);
	m_wndToolBar.RemoveUpdateInfo();

	const bool isAutoUpdate = CUpdateCheck::IsAutoUpdateFromMessage(wparam, lparam);
	CString updateText = _T("Checking for updates...");
	if(isAutoUpdate)
	{
		SetHelpText(updateText);
	} else if(m_UpdateOptionsDialog)
	{
		m_UpdateOptionsDialog->SetDlgItemText(IDC_LASTUPDATE, updateText);
	} else
	{
		if(!g_UpdateCheckProgressDialog)
		{
			g_UpdateCheckProgressDialog = std::make_unique<CUpdateCheckProgressDialog>(CMainFrame::GetMainFrame());
			g_UpdateCheckProgressDialog->Create(IDD_PROGRESS, CMainFrame::GetMainFrame());
			g_UpdateCheckProgressDialog->SetTitle(_T("Checking for updates..."));
			g_UpdateCheckProgressDialog->SetText(_T("Checking for updates..."));
			g_UpdateCheckProgressDialog->SetAbortText(_T("&Cancel"));
			g_UpdateCheckProgressDialog->SetRange(0, 100);
			g_UpdateCheckProgressDialog->ShowWindow(SW_SHOWDEFAULT);
		}
		if(g_UpdateCheckProgressDialog)
		{
			g_UpdateCheckProgressDialog->SetProgress(0);
		}
	}
	return TRUE;
}


LRESULT CMainFrame::OnUpdateCheckProgress(WPARAM wparam, LPARAM lparam)
{
	bool isAutoUpdate = wparam != 0;
	CString updateText = MPT_CFORMAT("Checking for updates... {}%")(lparam);
	if(isAutoUpdate)
	{
		SetHelpText(updateText);
	} else if(m_UpdateOptionsDialog)
	{
		m_UpdateOptionsDialog->SetDlgItemText(IDC_LASTUPDATE, updateText);
	} else
	{
		if(g_UpdateCheckProgressDialog)
		{
			g_UpdateCheckProgressDialog->SetProgress(lparam);
			if(g_UpdateCheckProgressDialog->m_abort)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}


LRESULT CMainFrame::OnUpdateCheckCanceled(WPARAM wparam, LPARAM lparam)
{
	m_updateCheckResult.reset();
	bool isAutoUpdate = CUpdateCheck::IsAutoUpdateFromMessage(wparam, lparam);
	CString updateText = _T("Checking for updates... Canceled.");
	if(isAutoUpdate)
	{
		SetHelpText(updateText);
	} else if(m_UpdateOptionsDialog)
	{
		m_UpdateOptionsDialog->SetDlgItemText(IDC_LASTUPDATE, updateText);
		m_UpdateOptionsDialog->SettingChanged(TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath());
	} else
	{
		if(g_UpdateCheckProgressDialog)
		{
			g_UpdateCheckProgressDialog->DestroyWindow();
			g_UpdateCheckProgressDialog = nullptr;
		}
	}
	if(isAutoUpdate)
	{
		SetHelpText(_T(""));
	}
	return TRUE;
}


LRESULT CMainFrame::OnUpdateCheckFailure(WPARAM wparam, LPARAM lparam)
{
	m_updateCheckResult.reset();
	const bool isAutoUpdate = CUpdateCheck::IsAutoUpdateFromMessage(wparam, lparam);
	const CUpdateCheck::Error &error = CUpdateCheck::MessageAsError(wparam, lparam);
	CString updateText = MPT_CFORMAT("Checking for updates failed: {}")(mpt::get_exception_text<mpt::ustring>(error));
	if(isAutoUpdate)
	{
		SetHelpText(updateText);
	} else if(m_UpdateOptionsDialog)
	{
		m_UpdateOptionsDialog->SetDlgItemText(IDC_LASTUPDATE, updateText);
		m_UpdateOptionsDialog->SettingChanged(TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath());
	} else
	{
		if(g_UpdateCheckProgressDialog)
		{
			g_UpdateCheckProgressDialog->DestroyWindow();
			g_UpdateCheckProgressDialog = nullptr;
		}
	}
	CUpdateCheck::ShowFailureGUI(isAutoUpdate, error);
	if(isAutoUpdate)
	{
		SetHelpText(_T(""));
	}
	return TRUE;
}


LRESULT CMainFrame::OnUpdateCheckSuccess(WPARAM wparam, LPARAM lparam)
{
	m_updateCheckResult.reset();
	const bool isAutoUpdate = CUpdateCheck::IsAutoUpdateFromMessage(wparam, lparam);
	const UpdateCheckResult &result = CUpdateCheck::MessageAsResult(wparam, lparam);
	CUpdateCheck::AcknowledgeSuccess(result);
	if(result.CheckTime != mpt::Date::Unix{})
	{
		TrackerSettings::Instance().UpdateLastUpdateCheck = result.CheckTime;
	}
	if(!isAutoUpdate)
	{
		if(m_UpdateOptionsDialog)
		{
			m_UpdateOptionsDialog->SetDlgItemText(IDC_LASTUPDATE, _T("Checking for updates... Done."));
		} else
		{
			if(g_UpdateCheckProgressDialog)
			{
				g_UpdateCheckProgressDialog->DestroyWindow();
				g_UpdateCheckProgressDialog = nullptr;
			}
			SetHelpText(_T(""));
		}
	}
	CUpdateCheck::ShowSuccessGUI(isAutoUpdate, result);
	if(isAutoUpdate)
	{
		SetHelpText(_T(""));
	}
	return TRUE;
}


LPARAM CMainFrame::OnToolbarUpdateIndicatorClick(WPARAM action, LPARAM)
{
	const auto popAction = static_cast<UpdateToolTip::PopAction>(action);
	if(popAction != UpdateToolTip::PopAction::ClickBubble)
		return 0;

	if(m_updateCheckResult)
		CUpdateCheck::ShowSuccessGUI(false, *m_updateCheckResult);
	else
		CUpdateCheck::DoManualUpdateCheck();
	return 0;
}


#endif // MPT_ENABLE_UPDATE


void CMainFrame::OnHelp()
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
			case OPTIONS_PAGE_WINE:			page = "::/Setup_Wine.html"; break;
		}
	} else if(view != nullptr)
	{
		const char *className = view->GetRuntimeClass()->m_lpszClassName;
		if(!strcmp("CViewGlobals", className))
			page = "::/General.html";
		else if(!strcmp("CViewPattern", className))
			page = "::/Patterns.html";
		else if(!strcmp("CViewSample", className))
			page = "::/Samples.html";
		else if(!strcmp("CViewInstrument", className))
			page = "::/Instruments.html";
		else if(!strcmp("CViewComments", className))
			page = "::/Comments.html";
		else if(!strcmp("CModControlView", className))
		{
			switch(static_cast<CModControlView*>(view)->GetActivePage())
			{
				case CModControlView::VIEW_GLOBALS: page = "::/General.html"; break;
				case CModControlView::VIEW_PATTERNS: page = "::/Patterns.html"; break;
				case CModControlView::VIEW_SAMPLES: page = "::/Samples.html"; break;
				case CModControlView::VIEW_INSTRUMENTS: page = "::/Instruments.html"; break;
				case CModControlView::VIEW_COMMENTS: page = "::/Comments.html"; break;
			}
		}
	}

	const mpt::PathString helpFile = theApp.GetInstallPath() + P_("OpenMPT Manual.chm") + mpt::PathString::FromUTF8(page) + P_(">OpenMPT");
	if(!::HtmlHelp(m_hWnd, helpFile.AsNative().c_str(), strcmp(page, "") ? HH_DISPLAY_TOC : HH_DISPLAY_TOPIC, NULL))
	{
		Reporting::Error(_T("Could not find help file:\n") + helpFile.ToCString());
		return;
	}
	//::ShowWindow(hwndHelp, SW_SHOWMAXIMIZED);
}


LRESULT CMainFrame::OnViewMIDIMapping(WPARAM wParam, LPARAM lParam)
{
	static bool inMapper = false;
	if(!inMapper)
	{
		inMapper = true;
		CModDoc *doc = GetActiveDoc();
		if(doc != nullptr)
			doc->ViewMIDIMapping(static_cast<PLUGINDEX>(wParam), static_cast<PlugParamIndex>(lParam));
		inMapper = false;
	}
	return 0;
}


HMENU CMainFrame::CreateFileMenu(const size_t maxCount, std::vector<mpt::PathString>& paths, const mpt::PathString &folderName, const uint16 idRangeBegin)
{
	paths.clear();
	HMENU hMenu = ::CreatePopupMenu();
	ASSERT(hMenu != NULL);
	if (hMenu != NULL)
	{
		UINT_PTR filesAdded = 0;
		for(size_t i = 0; i < 2; i++) // 0: app items, 1: user items
		{
			// To avoid duplicates, check whether app path and config path are the same.
			if (i == 1 && mpt::PathCompareNoCase(theApp.GetInstallPath(), theApp.GetConfigPath()) == 0)
				break;

			mpt::PathString basePath;
			basePath = (i == 0) ? theApp.GetInstallPath() : theApp.GetConfigPath();
			basePath += folderName;
			if(!mpt::native_fs{}.is_directory(basePath))
				continue;

			FolderScanner scanner(basePath, FolderScanner::kOnlyFiles);
			mpt::PathString fileName;
			while(filesAdded < maxCount && scanner.Next(fileName))
			{
				paths.push_back(fileName);
				CString file = fileName.GetFilename().ToCString();
				file.Replace(_T("&"), _T("&&"));
				AppendMenu(hMenu, MF_STRING, idRangeBegin + filesAdded, file);
				filesAdded++;
			}
		}

		if(filesAdded == 0)
		{
			AppendMenu(hMenu, MF_STRING | MF_GRAYED | MF_DISABLED, 0, _T("No items found"));
		} else
		{
			AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			AppendMenu(hMenu, MF_STRING, idRangeBegin + maxCount, _T("Browse..."));
		}
	}

	return hMenu;
}


void CMainFrame::CreateExampleModulesMenu()
{
	static_assert(nMaxItemsInExampleModulesMenu == ID_EXAMPLE_MODULES_LASTINRANGE - ID_EXAMPLE_MODULES,
				  "Make sure that there's a proper range for menu commands in resources.");
	HMENU hMenu = CreateFileMenu(nMaxItemsInExampleModulesMenu, m_ExampleModulePaths, P_("ExampleSongs\\"), ID_EXAMPLE_MODULES);
	CMenu* const pMainMenu = GetMenu();
	if (hMenu && pMainMenu && m_InputHandler)
		VERIFY(pMainMenu->ModifyMenu(ID_EXAMPLE_MODULES, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)hMenu, m_InputHandler->GetMenuText(ID_EXAMPLE_MODULES)));
	else
		ASSERT(false);
}


// Hack-ish way to get the file menu (this is necessary because the MDI document icon next to the File menu is a sub menu, too).
CMenu *CMainFrame::GetFileMenu() const
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
{
	static_assert(nMaxItemsInTemplateModulesMenu == ID_FILE_OPENTEMPLATE_LASTINRANGE - ID_FILE_OPENTEMPLATE,
				  "Make sure that there's a proper range for menu commands in resources.");
	HMENU hMenu = CreateFileMenu(nMaxItemsInTemplateModulesMenu, m_TemplateModulePaths, P_("TemplateModules\\"), ID_FILE_OPENTEMPLATE);
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
{
	CMenu *pMenu = GetFileMenu();
	if(!pMenu) return;

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
		const int entries = mpt::saturate_cast<int>(TrackerSettings::Instance().mruFiles.size());
		for(int i = 0; i < entries; i++)
		{
			mpt::winstring s = mpt::tfmt::val(i + 1) + _T(" ");
			// Add mnemonics
			if(i < 9)
			{
				s = _T("&") + s;
			} else if(i == 9)
			{
				s = _T("1&0 ");
			}

			const mpt::PathString &pathMPT = TrackerSettings::Instance().mruFiles[i];
			mpt::winstring path = pathMPT.AsNative();
			if(!mpt::PathCompareNoCase(workDir, pathMPT.GetDirectoryWithDrive()))
			{
				// Only show filename
				path = path.substr(workDir.AsNative().length());
			} else if(path.length() > 30)	// Magic number experimentally determined to be equal to MFC's behaviour
			{
				// Shorten path ("C:\Foo\VeryLongString...\Bar.it" => "C:\Foo\...\Bar.it")
				size_t start = path.find_first_of(_T("\\/"), path.find_first_of(_T("\\/")) + 1);
				size_t end = path.find_last_of(_T("\\/"));
				if(start < end)
				{
					path = path.substr(0, start + 1) + _T("...") + path.substr(end);
				}
			}
			path = mpt::String::Replace(path, mpt::winstring(_T("&")), mpt::winstring(_T("&&")));
			s += path;
			pMenu->InsertMenu(firstMenu + i, MF_STRING | MF_BYPOSITION, ID_MRU_LIST_FIRST + i, mpt::ToCString(s));
		}
	}
}


BOOL CMainFrame::OnQueryEndSession()
{
	int modifiedCount = 0;
	for(const auto &modDoc : theApp.GetOpenDocuments())
	{
		if(modDoc->IsModified())
			modifiedCount++;
	}
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
	ShutdownBlockReasonDestroy(m_hWnd);
	if(modifiedCount > 0)
	{
		ShutdownBlockReasonCreate(m_hWnd,
			MPT_WFORMAT("There {} {} unsaved file{}.")(modifiedCount == 1 ? L"is" : L"are", modifiedCount, modifiedCount == 1 ? L"" : L"s").c_str());
	}
#endif
	return modifiedCount ? FALSE : TRUE;
}


void CMainFrame::NotifyAccessibilityUpdate(CWnd &source)
{
	if(!IsPlaying() || m_pSndFile->m_SongFlags[SONG_PAUSED])
		source.NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, OBJID_CLIENT, CHILDID_SELF);
}

// ITfLanguageProfileNotifySink implementation

TfLanguageProfileNotifySink::TfLanguageProfileNotifySink()
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
			MPT_ASSERT(SUCCEEDED(hr));
			MPT_ASSERT(m_dwCookie != TF_INVALID_COOKIE);
		}
	}
}


TfLanguageProfileNotifySink::~TfLanguageProfileNotifySink()
{
	if(mpt::OS::Windows::IsWine())
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


HRESULT TfLanguageProfileNotifySink::OnLanguageChange(LANGID /*langid*/, __RPC__out BOOL *pfAccept)
{
	*pfAccept = TRUE;
	return ResultFromScode(S_OK);
}


HRESULT TfLanguageProfileNotifySink::OnLanguageChanged()
{
	// Input language has changed, so key positions might have changed too.
	CMainFrame *mainFrm = CMainFrame::GetMainFrame();
	if(mainFrm != nullptr)
	{
		mainFrm->UpdateEffectKeys(mainFrm->GetActiveDoc());
		mainFrm->m_InputHandler->SetModifierMask(ModNone);
	}
	return ResultFromScode(S_OK);
}


HRESULT TfLanguageProfileNotifySink::QueryInterface(REFIID riid, void **ppvObject)
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
{
	// Don't let COM do anything to this object
	return 1;
}

ULONG TfLanguageProfileNotifySink::Release()
{
	// Don't let COM do anything to this object
	return 1;
}


/////////////////////////////////////////////
//Misc helper functions
/////////////////////////////////////////////

void AddPluginNamesToCombobox(CComboBox &CBox, const std::array<SNDMIXPLUGIN, MAX_MIXPLUGINS> &plugins, const bool libraryName, const PLUGINDEX updatePlug)
{
#ifndef NO_PLUGINS
	int insertAt = CBox.GetCount();
	if(updatePlug != PLUGINDEX_INVALID)
	{
		const int items = insertAt;
		for(insertAt = 0; insertAt < items; insertAt++)
		{
			auto thisPlugin = static_cast<PLUGINDEX>(CBox.GetItemData(insertAt));
			if(thisPlugin == (updatePlug + 1))
				CBox.DeleteString(insertAt);
			if(thisPlugin >= (updatePlug + 1))
				break;
		}
	}

	mpt::tstring str;
	str.reserve(80);
	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		if(updatePlug != PLUGINDEX_INVALID && plug != updatePlug)
			continue;
		const SNDMIXPLUGIN &plugin = plugins[plug];
		str.clear();
		str += MPT_TFORMAT("FX{}: ")(plug + 1);
		const auto plugName = plugin.GetName(), libName = plugin.GetLibraryName();
		str += mpt::ToWin(plugName);
		if(libraryName && plugName != libName && !libName.empty())
			str += _T(" (") + mpt::ToWin(libName) + _T(")");
		else if(plugName.empty() && (!libraryName || libName.empty()))
			str += _T("--");
#ifdef MPT_WITH_VST
		auto *vstPlug = dynamic_cast<const CVstPlugin *>(plugin.pMixPlugin);
		if(vstPlug != nullptr && vstPlug->isBridged)
		{
			VSTPluginLib &lib = vstPlug->GetPluginFactory();
			str += MPT_TFORMAT(" ({} Bridged)")(lib.GetDllArchNameUser());
		}
#endif // MPT_WITH_VST

		insertAt = CBox.InsertString(insertAt, str.c_str());
		CBox.SetItemData(insertAt, plug + 1);
		insertAt++;
	}
#endif // NO_PLUGINS
}


void AddPluginParameternamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN& plug)
{
#ifndef NO_PLUGINS
	if(plug.pMixPlugin)
		AddPluginParameternamesToCombobox(CBox, *plug.pMixPlugin);
#endif // NO_PLUGINS
}


void AddPluginParameternamesToCombobox(CComboBox& CBox, IMixPlugin& plug)
{
#ifndef NO_PLUGINS
	const PlugParamIndex numParams = plug.GetNumParameters();
	plug.CacheParameterNames(0, numParams);
	for(PlugParamIndex i = 0; i < numParams; i++)
	{
		CBox.SetItemData(CBox.AddString(plug.GetFormattedParamName(i)), i);
	}
#endif // NO_PLUGINS
}


OPENMPT_NAMESPACE_END
