/*
 * PathConfigDlg.cpp
 * -----------------
 * Purpose: Default paths and auto save setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "PathConfigDlg.h"
#include "FileDialog.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "Reporting.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "mpt/fs/fs.hpp"

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
	: CPropertyPage{IDD_OPTIONS_AUTOSAVE}
{
	m_accessibleEdits[0].SetAccessibleSuffix(_T("minutes"));
	m_accessibleEdits[1].SetAccessibleSuffix(_T("backups"));
	m_accessibleEdits[2].SetAccessibleSuffix(_T("days"));
	m_browseButtons[0].SetAccessibleText(_T("Browse for song folder..."));
	m_browseButtons[1].SetAccessibleText(_T("Browse for sample folder..."));
	m_browseButtons[2].SetAccessibleText(_T("Browse for instrument folder..."));
	m_browseButtons[3].SetAccessibleText(_T("Browse for VST plugin folder..."));
	m_browseButtons[4].SetAccessibleText(_T("Browse for VST preset folder..."));
	m_browseButtons[5].SetAccessibleText(_T("Browse for auto save folder..."));
}


void PathConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AUTOSAVE_INTERVAL, m_accessibleEdits[0]);
	DDX_Control(pDX, IDC_AUTOSAVE_HISTORY, m_accessibleEdits[1]);
	DDX_Control(pDX, IDC_EDIT1, m_accessibleEdits[2]);
	DDX_Control(pDX, IDC_BUTTON_CHANGE_MODDIR, m_browseButtons[0]);
	DDX_Control(pDX, IDC_BUTTON_CHANGE_SAMPDIR, m_browseButtons[1]);
	DDX_Control(pDX, IDC_BUTTON_CHANGE_INSTRDIR, m_browseButtons[2]);
	DDX_Control(pDX, IDC_BUTTON_CHANGE_VSTDIR, m_browseButtons[3]);
	DDX_Control(pDX, IDC_BUTTON_CHANGE_VSTPRESETSDIR, m_browseButtons[4]);
	DDX_Control(pDX, IDC_AUTOSAVE_BROWSE, m_browseButtons[5]);
}

BEGIN_MESSAGE_MAP(PathConfigDlg, CPropertyPage)
	// Paths
	ON_EN_CHANGE(IDC_OPTIONS_DIR_MODS,          &PathConfigDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_OPTIONS_DIR_SAMPS,         &PathConfigDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_OPTIONS_DIR_INSTS,         &PathConfigDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_OPTIONS_DIR_VSTPRESETS,    &PathConfigDlg::OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON_CHANGE_MODDIR,        &PathConfigDlg::OnBrowseSongs)
	ON_COMMAND(IDC_BUTTON_CHANGE_SAMPDIR,       &PathConfigDlg::OnBrowseSamples)
	ON_COMMAND(IDC_BUTTON_CHANGE_INSTRDIR,      &PathConfigDlg::OnBrowseInstruments)
	ON_COMMAND(IDC_BUTTON_CHANGE_VSTDIR,        &PathConfigDlg::OnBrowsePlugins)
	ON_COMMAND(IDC_BUTTON_CHANGE_VSTPRESETSDIR, &PathConfigDlg::OnBrowsePresets)

	// Autosave
	ON_COMMAND(IDC_CHECK1,                &PathConfigDlg::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,                &PathConfigDlg::OnAutosaveRetention)
	ON_COMMAND(IDC_CHECK3,                &PathConfigDlg::OnSettingsChanged)
	ON_COMMAND(IDC_AUTOSAVE_BROWSE,       &PathConfigDlg::OnBrowseAutosavePath)
	ON_COMMAND(IDC_AUTOSAVE_ENABLE,       &PathConfigDlg::OnAutosaveEnable)
	ON_COMMAND(IDC_AUTOSAVE_USEORIGDIR,   &PathConfigDlg::OnAutosaveUseOrigDir)
	ON_COMMAND(IDC_AUTOSAVE_USECUSTOMDIR, &PathConfigDlg::OnAutosaveUseOrigDir)
	ON_EN_UPDATE(IDC_AUTOSAVE_PATH,       &PathConfigDlg::OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_HISTORY,    &PathConfigDlg::OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_INTERVAL,   &PathConfigDlg::OnSettingsChanged)
	ON_EN_UPDATE(IDC_EDIT1,               &PathConfigDlg::OnSettingsChanged)
END_MESSAGE_MAP()


BOOL PathConfigDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	const auto &settings = TrackerSettings::Instance();
	// Default paths
	for(const auto & [path, id] : PathSettings)
	{
		SetDlgItemText(id, (settings.*path).GetDefaultDir().AsNative().c_str());
	}

	// Autosave
	CheckDlgButton(IDC_CHECK1, settings.CreateBackupFiles ? BST_CHECKED : BST_UNCHECKED);

	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1))->SetRange32(1, int32_max);
	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN2))->SetRange32(1, int32_max);
	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN3))->SetRange32(1, int32_max);
	CheckDlgButton(IDC_AUTOSAVE_ENABLE, settings.AutosaveEnabled ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_AUTOSAVE_HISTORY, settings.AutosaveHistoryDepth);
	SetDlgItemInt(IDC_AUTOSAVE_INTERVAL, settings.AutosaveIntervalMinutes);
	SetDlgItemInt(IDC_EDIT1, settings.AutosaveRetentionTimeDays ? settings.AutosaveRetentionTimeDays : 30);
	CheckDlgButton(IDC_CHECK2, settings.AutosaveRetentionTimeDays > 0);
	CheckDlgButton(IDC_AUTOSAVE_USEORIGDIR, settings.AutosaveUseOriginalPath ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_AUTOSAVE_USECUSTOMDIR, settings.AutosaveUseOriginalPath ? BST_UNCHECKED : BST_CHECKED);
	CheckDlgButton(IDC_CHECK3, settings.AutosaveDeletePermanently ? BST_CHECKED : BST_UNCHECKED);
	//enable/disable stuff as appropriate
	OnAutosaveEnable();
	OnAutosaveUseOrigDir();
	OnAutosaveRetention();

	return TRUE;
}


mpt::PathString PathConfigDlg::GetPath(int id)
{
	return mpt::PathString::FromCString(GetWindowTextString(*GetDlgItem(id))).WithTrailingSlash();
}


void PathConfigDlg::OnOK()
{
	auto &settings = TrackerSettings::Instance();
	// Default paths
	for(const auto &path : PathSettings)
	{
		(settings.*path.first).SetDefaultDir(GetPath(path.second));
	}

	// Autosave
	settings.CreateBackupFiles = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;

	settings.AutosaveEnabled = IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) != BST_UNCHECKED;
	settings.AutosaveHistoryDepth = GetDlgItemInt(IDC_AUTOSAVE_HISTORY);
	settings.AutosaveIntervalMinutes = GetDlgItemInt(IDC_AUTOSAVE_INTERVAL);
	if(IsDlgButtonChecked(IDC_CHECK2))
		settings.AutosaveRetentionTimeDays = std::max(GetDlgItemInt(IDC_EDIT1), UINT(1));
	else
		settings.AutosaveRetentionTimeDays = 0;
	settings.AutosaveUseOriginalPath = IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR) != BST_UNCHECKED;
	settings.AutosaveDeletePermanently = IsDlgButtonChecked(IDC_CHECK3) != BST_UNCHECKED;

	CPropertyPage::OnOK();
}


void PathConfigDlg::BrowseFolder(UINT nID)
{
	const TCHAR *prompt = (nID == IDC_AUTOSAVE_PATH)
		? _T("Select a folder to store autosaved files in...")
		: _T("Select a default folder...");
	BrowseForFolder dlg(GetPath(nID), prompt);
	if(dlg.Show(this))
	{
		SetDlgItemText(nID, dlg.GetDirectory().AsNative().c_str());
		OnSettingsChanged();
	}
}


void PathConfigDlg::OnAutosaveEnable()
{
	const BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE);
	static constexpr UINT AutoSaveDlgItems[] =
	{
		IDC_AUTOSAVE_INTERVAL, IDC_AUTOSAVE_HISTORY, IDC_AUTOSAVE_USEORIGDIR, IDC_AUTOSAVE_USECUSTOMDIR,
		IDC_AUTOSAVE_PATH, IDC_AUTOSAVE_BROWSE, IDC_CHECK2, IDC_CHECK3, IDC_EDIT1, IDC_SPIN1, IDC_SPIN2, IDC_SPIN3
	};
	for(UINT id : AutoSaveDlgItems)
	{
		GetDlgItem(id)->EnableWindow(enabled);
	}
	OnSettingsChanged();
	return;
}


void PathConfigDlg::OnAutosaveUseOrigDir()
{
	if(IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE))
	{
		const BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR) ? FALSE : TRUE;
		static constexpr UINT AutoSaveDirDlgItems[] = {IDC_AUTOSAVE_PATH, IDC_AUTOSAVE_BROWSE, IDC_CHECK2, IDC_EDIT1, IDC_SPIN3};
		for(UINT id : AutoSaveDirDlgItems)
		{
			GetDlgItem(id)->EnableWindow(enabled);
		}
		OnSettingsChanged();
	}
}


void PathConfigDlg::OnAutosaveRetention()
{
	const BOOL enabled = (IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) && IsDlgButtonChecked(IDC_CHECK2)) ? TRUE : FALSE;
	GetDlgItem(IDC_EDIT1)->EnableWindow(enabled);
	GetDlgItem(IDC_SPIN3)->EnableWindow(enabled);
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


void PathConfigDlg::OnBrowseAutosavePath() { BrowseFolder(IDC_AUTOSAVE_PATH); }
void PathConfigDlg::OnBrowseSongs() { BrowseFolder(IDC_OPTIONS_DIR_MODS); }
void PathConfigDlg::OnBrowseSamples() { BrowseFolder(IDC_OPTIONS_DIR_SAMPS); }
void PathConfigDlg::OnBrowseInstruments() { BrowseFolder(IDC_OPTIONS_DIR_INSTS); }
void PathConfigDlg::OnBrowsePlugins() { BrowseFolder(IDC_OPTIONS_DIR_VSTS); }
void PathConfigDlg::OnBrowsePresets() { BrowseFolder(IDC_OPTIONS_DIR_VSTPRESETS); }


OPENMPT_NAMESPACE_END
