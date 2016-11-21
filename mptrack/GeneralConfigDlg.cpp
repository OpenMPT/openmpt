/*
 * GeneralConfigDlg.cpp
 * --------------------
 * Purpose: Implementation of the general settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "GeneralConfigDlg.h"
#include "Settings.h"
#include "FileDialog.h"
#include "../common/StringFixer.h"
#include "FolderScanner.h"


OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(COptionsGeneral, CPropertyPage)
	ON_LBN_SELCHANGE(IDC_LIST1,		OnOptionSelChanged)
	ON_CLBN_CHKCHANGE(IDC_LIST1,	OnSettingsChanged)
	ON_COMMAND(IDC_RADIO1,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,			OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT1,			OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnDefaultTypeChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnTemplateChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO2,	OnTemplateChanged)
	ON_COMMAND(IDC_BUTTON1,			OnBrowseTemplate)
END_MESSAGE_MAP()


static const struct GeneralOptionsDescriptions
{
	uint32 flag;
	const char *name, *description;
} generalOptionsList[] =
{
	{PATTERN_PLAYNEWNOTE,	"Play new notes while recording",	"When this option is enabled, notes entered in the pattern editor will always be played (If not checked, notes won't be played in record mode)."},
	{PATTERN_PLAYEDITROW,	"Play whole row while recording",	"When this option is enabled, all notes on the current row are played when entering notes in the pattern editor."},
	{PATTERN_CENTERROW,		"Always center active row",			"Turn on this option to have the active row always centered in the pattern editor."},
	{PATTERN_SMOOTHSCROLL,	"Smooth pattern scrolling",			"Scroll patterns tick by tick rather than row by row at the expense of an increased CPU load."},
	{PATTERN_HEXDISPLAY,	"Display rows in hex",				"With this option enabled, row numbers and sequence numbers will be displayed in hexadecimal."},
	{PATTERN_WRAP,			"Cursor wrap in pattern editor",	"When this option is active, going past the end of a pattern row or channel will move the cursor to the beginning. When \"Continuous scroll\"-option is enabled, row wrap is disabled."},
	{PATTERN_DRAGNDROPEDIT,	"Drag and Drop Editing",			"Enable moving a selection in the pattern editor (copying if pressing shift while dragging)"},
	{PATTERN_FLATBUTTONS,	"Flat Buttons",						"Use flat buttons in toolbars"},
	{PATTERN_SINGLEEXPAND,	"Single click to expand tree",		"Single-clicking in the left tree view will expand a node."},
	{PATTERN_MUTECHNMODE,	"Ignored muted channels",			"Notes will not be played on muted channels (unmuting will only start on a new note)."},
	{PATTERN_NOEXTRALOUD,	"No loud sample preview",			"Disable loud playback of samples in the sample/instrument editor. Sample volume depends on the sample volume slider on the general tab when activated (if disabled, samples are previewed at 0 dB)."},
	{PATTERN_SHOWPREVIOUS,	"Show Prev/Next patterns",			"Displays grayed-out version of the previous/next patterns in the pattern editor. Does not work if \"always center active row\" is disabled."},
	{PATTERN_CONTSCROLL,	"Continuous scroll",				"Jumps to the next pattern when moving past the end of a pattern"},
	{PATTERN_KBDNOTEOFF,	"Record note off",					"Record note off when a key is released on the PC keyboard."},
	{PATTERN_FOLLOWSONGOFF,	"Follow song off by default",		"Ensure follow song is off when opening or starting a new song."},
	{PATTERN_OLDCTXMENUSTYLE, "Old style pattern context menu", "Check this option to hide unavailable items in the pattern editor context menu. Uncheck to grey-out unavailable items instead."},
	{PATTERN_SYNCMUTE,		"Maintain sample sync on mute",		"Samples continue to be processed when channels are muted (like in IT2 and FT2)"},
	{PATTERN_SYNCSAMPLEPOS,	"Maintain sample sync on seek",		"Sample that are still active from previous patterns are continued to be played after seeking.\nNote: Some pattern commands may prevent samples from being synced. This feature may slow down seeking."},
	{PATTERN_AUTODELAY,		"Automatic delay commands",			"Automatically insert appropriate note-delay commands when recording notes during live playback."},
	{PATTERN_NOTEFADE,		"Note fade on key up",				"Enable to fade / stop notes on key up in pattern tab."},
	{PATTERN_OVERFLOWPASTE,	"Overflow paste mode",				"Wrap pasted pattern data into next pattern. This is useful for creating echo channels."},
	{PATTERN_RESETCHANNELS,	"Reset channels on loop",			"If enabled, channels will be reset to their initial state when song looping is enabled.\nNote: This does not affect manual song loops (i.e. triggered by pattern commands) and is not recommended to be enabled."},
	{PATTERN_LIVEUPDATETREE,"Update sample status in tree",		"If enabled, active samples and instruments will be indicated by a different icon in the treeview."},
	{PATTERN_NOCLOSEDIALOG,	"Disable modern close dialog",		"When closing the main window, a confirmation window is shown for every unsaved document instead of one single window with a list of unsaved documents."},
	{PATTERN_DBLCLICKSELECT, "Double-click to select channel",	"Instead of showing the note properties, double-clicking a pattern cell selects the whole channel."},
	{PATTERN_SHOWDEFAULTVOLUME, "Show default volume commands",	"If there is no volume command next to a note + instrument combination, the sample's default volume is shown."},
};


void COptionsGeneral::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_LIST1,		m_CheckList);
	DDX_Control(pDX, IDC_EDIT1,		m_defaultArtist);
	DDX_Control(pDX, IDC_COMBO2,	m_defaultTemplate);
	DDX_Control(pDX, IDC_COMBO1,	m_defaultFormat);
	//}}AFX_DATA_MAP
}


BOOL COptionsGeneral::OnInitDialog()
//----------------------------------
{
	CPropertyPage::OnInitDialog();

	::SetWindowTextW(m_defaultArtist.m_hWnd, mpt::ToWide(TrackerSettings::Instance().defaultArtist).c_str());

	const struct
	{
		MODTYPE type;
		TCHAR *str;
	} formats[] =
	{
		{ MOD_TYPE_MOD, _T("MOD") },
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

	const mpt::PathString basePath = theApp.GetConfigPath() + MPT_PATHSTRING("TemplateModules\\");
	FolderScanner scanner(basePath, true);
	mpt::PathString file;
	while(scanner.NextFile(file))
	{
		std::wstring fileW = file.AsNative();
		fileW = fileW.substr(basePath.Length());
		::SendMessageW(m_defaultTemplate.m_hWnd, CB_ADDSTRING, 0, (LPARAM)fileW.c_str());
	}
	file = TrackerSettings::Instance().defaultTemplateFile;
	if(file.GetPath() == basePath)
	{
		file = file.GetFullFileName();
	}
	::SetWindowTextW(m_defaultTemplate.m_hWnd, file.AsNative().c_str());

	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1 + TrackerSettings::Instance().defaultNewFileAction);

	for(const auto &opt : generalOptionsList)
	{
		auto idx = m_CheckList.AddString(opt.name);
		const int check = (TrackerSettings::Instance().m_dwPatternSetup & opt.flag) != 0 ? BST_CHECKED : BST_UNCHECKED;
		m_CheckList.SetCheck(idx, check);
	}
	m_CheckList.SetCurSel(0);
	OnOptionSelChanged();

	return TRUE;
}


void COptionsGeneral::OnOK()
//--------------------------
{
	TrackerSettings::Instance().defaultArtist = mpt::ToUnicode(GetWindowTextW(m_defaultArtist));
	TrackerSettings::Instance().defaultModType = static_cast<MODTYPE>(m_defaultFormat.GetItemData(m_defaultFormat.GetCurSel()));
	TrackerSettings::Instance().defaultTemplateFile = mpt::PathString::FromNative(GetWindowTextW(m_defaultTemplate));

	NewFileAction action = nfDefaultFormat;
	if(IsDlgButtonChecked(IDC_RADIO2)) action = nfSameAsCurrent;
	if(IsDlgButtonChecked(IDC_RADIO3)) action = nfDefaultTemplate;
	if(action == nfDefaultTemplate && TrackerSettings::Instance().defaultTemplateFile.Get().empty())
	{
		action = nfDefaultFormat;
		::MessageBeep(MB_ICONWARNING);
	}
	TrackerSettings::Instance().defaultNewFileAction = action;

	for(int i = 0; i < CountOf(generalOptionsList); i++)
	{
		const bool check = (m_CheckList.GetCheck(i) != BST_UNCHECKED);

		if(check) TrackerSettings::Instance().m_dwPatternSetup |= generalOptionsList[i].flag;
		else TrackerSettings::Instance().m_dwPatternSetup &= ~generalOptionsList[i].flag;
	}

	CMainFrame::GetMainFrame()->SetupMiscOptions();

	CPropertyPage::OnOK();
}


BOOL COptionsGeneral::OnSetActive()
//---------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_GENERAL;
	return CPropertyPage::OnSetActive();
}


void COptionsGeneral::OnOptionSelChanged()
//----------------------------------------
{
	LPCSTR pszDesc = NULL;
	const int sel = m_CheckList.GetCurSel();
	if ((sel >= 0) && (sel < CountOf(generalOptionsList)))
	{
		pszDesc = generalOptionsList[sel].description;
	}
	SetDlgItemText(IDC_TEXT1, (pszDesc) ? pszDesc : "");
}


void COptionsGeneral::OnBrowseTemplate()
//--------------------------------------
{
	mpt::PathString basePath = theApp.GetAppDirPath() + MPT_PATHSTRING("TemplateModules\\");
	mpt::PathString defaultFile = mpt::PathString::FromNative(GetWindowTextW(m_defaultTemplate));
	if(defaultFile.empty()) defaultFile = TrackerSettings::Instance().defaultTemplateFile;

	OpenFileDialog dlg;
	if(defaultFile.empty())
	{
		dlg.WorkingDirectory(basePath);
	} else
	{
		if(defaultFile.ToWide().find_first_of(L"/\\") == std::wstring::npos)
		{
			// Relative path
			defaultFile = basePath + defaultFile;
		}
		dlg.DefaultFilename(defaultFile);
	}
	if(dlg.Show(this))
	{
		defaultFile = dlg.GetFirstFile();
		if(defaultFile.GetPath() == basePath)
		{
			defaultFile = defaultFile.GetFullFileName();
		}
		::SetWindowTextW(m_defaultTemplate.m_hWnd, defaultFile.AsNative().c_str());
		OnTemplateChanged();
	}
}

OPENMPT_NAMESPACE_END
