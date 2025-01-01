/*
 * GeneralConfigDlg.cpp
 * --------------------
 * Purpose: Implementation of the general settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "GeneralConfigDlg.h"
#include "FileDialog.h"
#include "FolderScanner.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "resource.h"
#include "Settings.h"
#include "TrackerSettings.h"
#include "../common/mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(COptionsGeneral, CPropertyPage)
	ON_LBN_SELCHANGE(IDC_LIST1,   &COptionsGeneral::OnOptionSelChanged)
	ON_CLBN_CHKCHANGE(IDC_LIST1,  &COptionsGeneral::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO1,        &COptionsGeneral::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,        &COptionsGeneral::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,        &COptionsGeneral::OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT1,       &COptionsGeneral::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,  &COptionsGeneral::OnDefaultTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,  &COptionsGeneral::OnTemplateChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO2, &COptionsGeneral::OnTemplateChanged)
	ON_COMMAND(IDC_BUTTON1,       &COptionsGeneral::OnBrowseTemplate)
END_MESSAGE_MAP()


COptionsGeneral::COptionsGeneral() : CPropertyPage{IDD_OPTIONS_GENERAL}
{
	m_templateBrowseButton.SetAccessibleText(_T("Browse for template file..."));
}


static constexpr struct GeneralOptionsDescriptions
{
	PatternSetup flag;
	const char *name, *description;
} generalOptionsList[] =
{
	{PatternSetup::PlayNewNotesWhileRecording, "Play new notes while recording", "When this option is enabled, notes entered in the pattern editor will always be played (If not checked, notes won't be played in record mode)."},
	{PatternSetup::PlayRowOnNoteEntry,         "Play whole row while recording", "When this option is enabled, all notes on the current row are played when entering notes in the pattern editor."},
	{PatternSetup::PlayRowOnNavigate,          "Play whole row when navigating", "When this option is enabled, all notes on the current row are played when navigating vertically in the pattern editor."},
	{PatternSetup::PreviewNoteTransposition,   "Play notes when transposing",    "When transposing a single note, the new note is previewed."},
	{PatternSetup::CenterActiveRow,            "Always center active row",       "Turn on this option to have the active row always centered in the pattern editor."},
	{PatternSetup::SmoothScrolling,            "Smooth pattern scrolling",       "Scroll patterns tick by tick rather than row by row at the expense of an increased CPU load."},
	{PatternSetup::RowAndOrderNumbersHex,      "Display rows / orders in hex",   "With this option enabled, row numbers and sequence numbers will be displayed in hexadecimal."},
	{PatternSetup::CursorWrap,                 "Cursor wrap in pattern editor",  "When this option is active, going past the end of a pattern row or channel will move the cursor to the beginning. When \"Continuous scroll\"-option is enabled, row wrap is disabled."},
	{PatternSetup::DragNDropEdit,              "Drag and Drop Editing",          "Enable moving a selection in the pattern editor (copying if pressing shift while dragging)"},
	{PatternSetup::FlatToolbarButtons,         "Flat Buttons",                   "Use flat buttons in toolbars"},
	{PatternSetup::SingleClickToExpand,        "Single click to expand tree",    "Single-clicking in the left tree view will expand a node."},
	{PatternSetup::IgnoreMutedChannels,        "Ignored muted channels",         "Notes will not be played on muted channels (unmuting will only start on a new note)."},
	{PatternSetup::NoLoudSamplePreview,        "No loud sample preview",         "Disable loud playback of samples in the sample/instrument editor. Sample volume depends on the sample volume slider on the general tab when activated (if disabled, samples are previewed at 0 dB)."},
	{PatternSetup::ShowPrevNextPattern,        "Show Prev/Next patterns",        "Displays grayed-out version of the previous/next patterns in the pattern editor. Does not work if \"always center active row\" is disabled."},
	{PatternSetup::ContinuousScrolling,        "Continuous scroll",              "Jumps to the next pattern when moving past the end of a pattern"},
	{PatternSetup::RecordNoteOff,              "Record note off",                "Record note off when a key is released on the PC keyboard."},
	{PatternSetup::FollowSongOffByDefault,     "Follow Song off by default",     "Ensure Follow Song is off when opening or starting a new song."},
	{PatternSetup::DisableFollowOnClick,       "Disable Follow Song on click",   "Follow Song is deactivated when clicking into the pattern."},
	{PatternSetup::HideUnavailableMenuEntries, "Old style pattern context menu", "Check this option to hide unavailable items in the pattern editor context menu. Uncheck to grey-out unavailable items instead."},
	{PatternSetup::SyncMute,                   "Maintain sample sync on mute",   "Samples continue to be processed when channels are muted (like in IT2 and FT2)"},
	{PatternSetup::SampleSyncOnSeek,           "Maintain sample sync on seek",   "Sample that are still active from previous patterns are continued to be played after seeking.\nNote: Some pattern commands may prevent samples from being synced. This feature may slow down seeking."},
	{PatternSetup::AutoDelayCommands,          "Automatic delay commands",       "Automatically insert appropriate note-delay commands when recording notes during live playback."},
	{PatternSetup::NoteFadeOnKeyUp,            "Note fade on key up",            "Enable to fade / stop notes on key up in pattern tab."},
	{PatternSetup::OverflowPaste,              "Overflow paste mode",            "Wrap pasted pattern data into next pattern. This is useful for creating echo channels."},
	{PatternSetup::ResetChannelsOnLoop,        "Reset channels on loop",         "If enabled, channels will be reset to their initial state when song looping is enabled.\nNote: This does not affect manual song loops (i.e. triggered by pattern commands) and is not recommended to be enabled."},
	{PatternSetup::LiveUpdateTreeView,         "Update sample status in tree",   "If enabled, active samples and instruments will be indicated by a different icon in the treeview."},
	{PatternSetup::NoCustomCloseDialog,        "Disable modern close dialog",    "When closing the main window, a confirmation window is shown for every unsaved document instead of one single window with a list of unsaved documents."},
	{PatternSetup::DblClickSelectsChannel,     "Double-click to select channel", "Instead of showing the note properties, double-clicking a pattern cell selects the whole channel."},
	{PatternSetup::ShowDefaultVolume,          "Show default volume commands",   "If there is no volume command next to a note + instrument combination, the sample's default volume is shown."},
};


void COptionsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsGeneral)
	DDX_Control(pDX, IDC_LIST1,   m_CheckList);
	DDX_Control(pDX, IDC_EDIT1,   m_defaultArtist);
	DDX_Control(pDX, IDC_COMBO2,  m_defaultTemplate);
	DDX_Control(pDX, IDC_COMBO1,  m_defaultFormat);
	DDX_Control(pDX, IDC_BUTTON1, m_templateBrowseButton);
	//}}AFX_DATA_MAP
}


BOOL COptionsGeneral::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_defaultArtist.SetWindowText(mpt::ToCString(TrackerSettings::Instance().defaultArtist.Get()));

	static constexpr struct
	{
		MODTYPE type;
		const TCHAR *str;
	} formats[] =
	{
		{ MOD_TYPE_MOD, _T("MOD (Amiga)") },
		{ MOD_TYPE_MOD_PC, _T("MOD (PC)")},
		{ MOD_TYPE_XM, _T("XM") },
		{ MOD_TYPE_S3M, _T("S3M") },
		{ MOD_TYPE_IT, _T("IT") },
		{ MOD_TYPE_MPT, _T("MPTM") },
	};
	m_defaultFormat.SetCurSel(0);
	for(const auto &fmt : formats)
	{
		auto idx = m_defaultFormat.AddString(fmt.str);
		m_defaultFormat.SetItemData(idx, fmt.type);
		if(fmt.type == TrackerSettings::Instance().defaultModType)
		{
			m_defaultFormat.SetCurSel(idx);
		}
	}

	const mpt::PathString basePath = theApp.GetUserTemplatesPath();
	FolderScanner scanner(basePath, FolderScanner::kOnlyFiles | FolderScanner::kFindInSubDirectories);
	mpt::PathString file;
	while(scanner.Next(file))
	{
		mpt::RawPathString fileW = file.AsNative();
		fileW = fileW.substr(basePath.Length());
		::SendMessage(m_defaultTemplate.m_hWnd, CB_ADDSTRING, 0, (LPARAM)fileW.c_str());
	}
	file = TrackerSettings::Instance().defaultTemplateFile;
	if(file.GetDirectoryWithDrive() == basePath)
	{
		file = file.GetFilename();
	}
	m_defaultTemplate.SetWindowText(file.AsNative().c_str());

	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1 + TrackerSettings::Instance().defaultNewFileAction);

	const FlagSet<PatternSetup> patternSetup = TrackerSettings::Instance().patternSetup;
	for(const auto &opt : generalOptionsList)
	{
		auto idx = m_CheckList.AddString(mpt::ToCString(mpt::Charset::ASCII, opt.name));
		const int check = patternSetup[opt.flag] ? BST_CHECKED : BST_UNCHECKED;
		m_CheckList.SetCheck(idx, check);
	}
	m_CheckList.SetCurSel(0);
	m_CheckList.SetItemHeight(0, 0);  // Workaround to force MFC to correctly compute the height of the first list item, in particular on high-DPI setups
	OnOptionSelChanged();

	return TRUE;
}


void COptionsGeneral::OnOK()
{
	TrackerSettings::Instance().defaultArtist = GetWindowTextUnicode(m_defaultArtist);
	TrackerSettings::Instance().defaultModType = static_cast<MODTYPE>(m_defaultFormat.GetItemData(m_defaultFormat.GetCurSel()));
	TrackerSettings::Instance().defaultTemplateFile = mpt::PathString::FromCString(GetWindowTextString(m_defaultTemplate));

	NewFileAction action = nfDefaultFormat;
	int newActionRadio = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3);
	if(newActionRadio == IDC_RADIO2) action = nfSameAsCurrent;
	if(newActionRadio == IDC_RADIO3) action = nfDefaultTemplate;
	if(action == nfDefaultTemplate && TrackerSettings::Instance().defaultTemplateFile.Get().empty())
	{
		action = nfDefaultFormat;
		::MessageBeep(MB_ICONWARNING);
	}
	TrackerSettings::Instance().defaultNewFileAction = action;

	FlagSet<PatternSetup> patternSetup = TrackerSettings::Instance().patternSetup;
	for(int i = 0; i < mpt::saturate_cast<int>(std::size(generalOptionsList)); i++)
	{
		const bool check = (m_CheckList.GetCheck(i) != BST_UNCHECKED);

		patternSetup.set(generalOptionsList[i].flag, check);
	}
	TrackerSettings::Instance().patternSetup = patternSetup;

	CMainFrame::GetMainFrame()->SetupMiscOptions();

	CPropertyPage::OnOK();
}


BOOL COptionsGeneral::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_GENERAL;
	return CPropertyPage::OnSetActive();
}


void COptionsGeneral::OnOptionSelChanged()
{
	const char *desc = "";
	const int sel = m_CheckList.GetCurSel();
	if ((sel >= 0) && (sel < mpt::saturate_cast<int>(std::size(generalOptionsList))))
	{
		desc = generalOptionsList[sel].description;
	}
	SetDlgItemText(IDC_TEXT1, mpt::ToCString(mpt::Charset::ASCII, desc));
}


void COptionsGeneral::OnBrowseTemplate()
{
	mpt::PathString basePath = theApp.GetUserTemplatesPath();
	mpt::PathString defaultFile = mpt::PathString::FromCString(GetWindowTextString(m_defaultTemplate));
	if(defaultFile.empty()) defaultFile = TrackerSettings::Instance().defaultTemplateFile;

	OpenFileDialog dlg;
	if(defaultFile.empty())
	{
		dlg.WorkingDirectory(basePath);
	} else
	{
		if(defaultFile.AsNative().find_first_of(_T("/\\")) == mpt::RawPathString::npos)
		{
			// Relative path
			defaultFile = basePath + defaultFile;
		}
		dlg.DefaultFilename(defaultFile);
	}
	if(dlg.Show(this))
	{
		defaultFile = dlg.GetFirstFile();
		if(defaultFile.GetDirectoryWithDrive() == basePath)
		{
			defaultFile = defaultFile.GetFilename();
		}
		m_defaultTemplate.SetWindowText(defaultFile.AsNative().c_str());
		OnTemplateChanged();
	}
}


void COptionsGeneral::OnDefaultTypeChanged() { CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1); OnSettingsChanged(); }
void COptionsGeneral::OnTemplateChanged() { CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO3); OnSettingsChanged(); }


OPENMPT_NAMESPACE_END
