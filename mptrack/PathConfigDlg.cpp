/*
 * PathConfigDlg.cpp
 * -----------------
 * Purpose: Default paths and auto save setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "resource.h"
#include "PathConfigDlg.h"
#include "FileDialog.h"
#include "Mainfrm.h"

OPENMPT_NAMESPACE_BEGIN

static constexpr std::pair<ConfigurableDirectory TrackerSettings::*, int> PathSettings[] =
{
	{ &TrackerSettings::PathSongs, IDC_OPTIONS_DIR_MODS },
	{ &TrackerSettings::PathSamples, IDC_OPTIONS_DIR_SAMPS },
	{ &TrackerSettings::PathInstruments, IDC_OPTIONS_DIR_INSTS },
	{ &TrackerSettings::PathPlugins, IDC_OPTIONS_DIR_VSTS },
	{ &TrackerSettings::PathPluginPresets, IDC_OPTIONS_DIR_VSTPRESETS },
	{ &TrackerSettings::AutosavePath, IDC_AUTOSAVE_PATH },
};

IMPLEMENT_DYNAMIC(PathConfigDlg, CPropertyPage)

PathConfigDlg::PathConfigDlg()
	: CPropertyPage(IDD_OPTIONS_AUTOSAVE)
{
}


void PathConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(PathConfigDlg, CPropertyPage)
	// Paths
	ON_EN_CHANGE(IDC_OPTIONS_DIR_MODS,			OnSettingsChanged)
	ON_EN_CHANGE(IDC_OPTIONS_DIR_SAMPS,			OnSettingsChanged)
	ON_EN_CHANGE(IDC_OPTIONS_DIR_INSTS,			OnSettingsChanged)
	ON_EN_CHANGE(IDC_OPTIONS_DIR_VSTPRESETS,	OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON_CHANGE_MODDIR,		OnBrowseSongs)
	ON_COMMAND(IDC_BUTTON_CHANGE_SAMPDIR,		OnBrowseSamples)
	ON_COMMAND(IDC_BUTTON_CHANGE_INSTRDIR,		OnBrowseInstruments)
	ON_COMMAND(IDC_BUTTON_CHANGE_VSTDIR,		OnBrowsePlugins)
	ON_COMMAND(IDC_BUTTON_CHANGE_VSTPRESETSDIR,	OnBrowsePresets)

	// Autosave
	ON_COMMAND(IDC_CHECK1,						OnSettingsChanged)
	ON_BN_CLICKED(IDC_AUTOSAVE_BROWSE,			OnBrowseAutosavePath)
	ON_BN_CLICKED(IDC_AUTOSAVE_ENABLE,			OnAutosaveEnable)
	ON_BN_CLICKED(IDC_AUTOSAVE_USEORIGDIR,		OnAutosaveUseOrigDir)
	ON_BN_CLICKED(IDC_AUTOSAVE_USECUSTOMDIR,	OnAutosaveUseOrigDir)
	ON_EN_UPDATE(IDC_AUTOSAVE_PATH,				OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_HISTORY,			OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_INTERVAL,			OnSettingsChanged)
END_MESSAGE_MAP()


BOOL PathConfigDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	const auto &settings = TrackerSettings::Instance();
	// Default paths
	for(const auto &path : PathSettings)
	{
		::SetDlgItemTextW(m_hWnd, path.second, (settings.*path.first).GetDefaultDir().AsNative().c_str());
	}

	// Autosave
	CheckDlgButton(IDC_CHECK1, settings.CreateBackupFiles ? BST_CHECKED : BST_UNCHECKED);

	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1))->SetRange32(1, int32_max);
	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN2))->SetRange32(1, int32_max);
	CheckDlgButton(IDC_AUTOSAVE_ENABLE, settings.AutosaveEnabled ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_AUTOSAVE_HISTORY, settings.AutosaveHistoryDepth);
	SetDlgItemInt(IDC_AUTOSAVE_INTERVAL, settings.AutosaveIntervalMinutes);
	CheckDlgButton(IDC_AUTOSAVE_USEORIGDIR, settings.AutosaveUseOriginalPath ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_AUTOSAVE_USECUSTOMDIR, settings.AutosaveUseOriginalPath ? BST_UNCHECKED : BST_CHECKED);
	//enable/disable stuff as appropriate
	OnAutosaveEnable();
	OnAutosaveUseOrigDir();

	return TRUE;
}


static mpt::PathString GetPath(HWND hwnd, int id)
{
	return mpt::PathString::FromUnicode(GetWindowTextUnicode(::GetDlgItem(hwnd, id))).EnsureTrailingSlash();
}


void PathConfigDlg::OnOK()
{
	auto &settings = TrackerSettings::Instance();
	// Default paths
	for(const auto &path : PathSettings)
	{
		(settings.*path.first).SetDefaultDir(GetPath(m_hWnd, path.second));
	}

	// Autosave
	settings.CreateBackupFiles = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;

	settings.AutosaveEnabled = (IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) != BST_UNCHECKED);
	settings.AutosaveHistoryDepth = (GetDlgItemInt(IDC_AUTOSAVE_HISTORY));
	settings.AutosaveIntervalMinutes = (GetDlgItemInt(IDC_AUTOSAVE_INTERVAL));
	settings.AutosaveUseOriginalPath = (IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR) == BST_CHECKED);

	CPropertyPage::OnOK();
}


void PathConfigDlg::BrowseFolder(UINT nID)
{
	TCHAR *prompt = (nID == IDC_AUTOSAVE_PATH)
		? _T("Select a folder to store autosaved files in...")
		: _T("Select a default folder...");
	BrowseForFolder dlg(GetPath(m_hWnd, nID), prompt);
	if(dlg.Show(this))
	{
		::SetDlgItemTextW(m_hWnd, nID, dlg.GetDirectory().AsNative().c_str());
		OnSettingsChanged();
	}
}


void PathConfigDlg::OnAutosaveEnable()
{
	BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE);
	GetDlgItem(IDC_AUTOSAVE_INTERVAL)->EnableWindow(enabled);
	GetDlgItem(IDC_SPIN1)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_HISTORY)->EnableWindow(enabled);
	GetDlgItem(IDC_SPIN2)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_USEORIGDIR)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_USECUSTOMDIR)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_PATH)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_BROWSE)->EnableWindow(enabled);
	OnSettingsChanged();
	return;
}


void PathConfigDlg::OnAutosaveUseOrigDir()
{
	if (IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE))
	{
		BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR);
		GetDlgItem(IDC_AUTOSAVE_PATH)->EnableWindow(!enabled);
		GetDlgItem(IDC_AUTOSAVE_BROWSE)->EnableWindow(!enabled);
		OnSettingsChanged();
	}
	return;
}


void PathConfigDlg::OnSettingsChanged()
{
	SetModified(TRUE);
}


BOOL PathConfigDlg::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_PATHS;
	return CPropertyPage::OnSetActive();
}


BOOL PathConfigDlg::OnKillActive()
{
	mpt::PathString path = GetPath(m_hWnd, IDC_AUTOSAVE_PATH);

	if (!path.IsDirectory() && IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) && !IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR))
	{
		Reporting::Error("Backup path does not exist.");
		GetDlgItem(IDC_AUTOSAVE_PATH)->SetFocus();
		return 0;
	}

	return CPropertyPage::OnKillActive();
}

OPENMPT_NAMESPACE_END
