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
#include "AutoSaver.h"
#include "PathConfigDlg.h"
#include "FileDialog.h"
#include "Mainfrm.h"

OPENMPT_NAMESPACE_BEGIN

IMPLEMENT_DYNAMIC(PathConfigDlg, CPropertyPage)

PathConfigDlg::PathConfigDlg(CAutoSaver &autoSaver)
	: CPropertyPage(IDD_OPTIONS_AUTOSAVE)
	, m_AutoSaver(autoSaver)
{
}


void PathConfigDlg::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
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
	ON_BN_CLICKED(IDC_AUTOSAVE_BROWSE,			OnBrowseAutosavePath)
	ON_BN_CLICKED(IDC_AUTOSAVE_ENABLE,			OnAutosaveEnable)
	ON_BN_CLICKED(IDC_AUTOSAVE_USEORIGDIR,		OnAutosaveUseOrigDir)
	ON_BN_CLICKED(IDC_AUTOSAVE_USECUSTOMDIR,	OnAutosaveUseOrigDir)
	ON_EN_UPDATE(IDC_AUTOSAVE_PATH,				OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_HISTORY,			OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_INTERVAL,			OnSettingsChanged)
END_MESSAGE_MAP()


BOOL PathConfigDlg::OnInitDialog()
//--------------------------------
{
	CPropertyPage::OnInitDialog();

	// Paths
	::SetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_MODS, TrackerSettings::Instance().PathSongs.GetDefaultDir().AsNative().c_str());
	::SetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_SAMPS, TrackerSettings::Instance().PathSamples.GetDefaultDir().AsNative().c_str());
	::SetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_INSTS, TrackerSettings::Instance().PathInstruments.GetDefaultDir().AsNative().c_str());
	::SetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_VSTS, TrackerSettings::Instance().PathPlugins.GetDefaultDir().AsNative().c_str());
	::SetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_VSTPRESETS,	TrackerSettings::Instance().PathPluginPresets.GetDefaultDir().AsNative().c_str());

	// Autosave
	CheckDlgButton(IDC_AUTOSAVE_ENABLE, m_AutoSaver.IsEnabled() ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemInt(IDC_AUTOSAVE_HISTORY, m_AutoSaver.GetHistoryDepth());
	::SetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, m_AutoSaver.GetPath().AsNative().c_str());
	SetDlgItemInt(IDC_AUTOSAVE_INTERVAL, m_AutoSaver.GetSaveInterval());
	CheckDlgButton(IDC_AUTOSAVE_USEORIGDIR, m_AutoSaver.GetUseOriginalPath() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_AUTOSAVE_USECUSTOMDIR, m_AutoSaver.GetUseOriginalPath() ? BST_UNCHECKED : BST_CHECKED);
	//enable/disable stuff as appropriate
	OnAutosaveEnable();
	OnAutosaveUseOrigDir();

	return TRUE;
}


void PathConfigDlg::OnOK()
//------------------------
{
	// Default paths
	WCHAR szModDir[MAX_PATH], szSmpDir[MAX_PATH], szInsDir[MAX_PATH], szVstDir[MAX_PATH], szPresetDir[MAX_PATH];
	szModDir[0] = szInsDir[0] = szSmpDir[0] = szVstDir[0] = szPresetDir[0] = 0;
	::GetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_MODS, szModDir, MAX_PATH);
	::GetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_SAMPS, szSmpDir, MAX_PATH);
	::GetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_INSTS, szInsDir, MAX_PATH);
	::GetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_VSTS, szVstDir, MAX_PATH);
	::GetDlgItemTextW(m_hWnd, IDC_OPTIONS_DIR_VSTPRESETS, szPresetDir, MAX_PATH);
	CMainFrame::GetMainFrame()->SetupDirectories(
			mpt::PathString::FromNative(szModDir),
			mpt::PathString::FromNative(szSmpDir),
			mpt::PathString::FromNative(szInsDir),
			mpt::PathString::FromNative(szVstDir),
			mpt::PathString::FromNative(szPresetDir));

	WCHAR tempPath[MAX_PATH];
	m_AutoSaver.SetEnabled(IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) != BST_UNCHECKED);
	m_AutoSaver.SetHistoryDepth(GetDlgItemInt(IDC_AUTOSAVE_HISTORY));
	m_AutoSaver.SetSaveInterval(GetDlgItemInt(IDC_AUTOSAVE_INTERVAL));
	m_AutoSaver.SetUseOriginalPath(IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR) == BST_CHECKED);
	::GetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, tempPath, CountOf(tempPath));
	mpt::PathString path = mpt::PathString::FromNative(tempPath).EnsureTrailingSlash();
	m_AutoSaver.SetPath(path);

	CPropertyPage::OnOK();
}


void PathConfigDlg::BrowseFolder(UINT nID)
//----------------------------------------
{
	WCHAR szPath[MAX_PATH] = L"";
	::GetDlgItemTextW(m_hWnd, nID, szPath, CountOf(szPath));

	TCHAR *prompt = (nID == IDC_AUTOSAVE_PATH)
		? _T("Select a folder to store autosaved files in...")
		: _T("Select a default folder...");
	BrowseForFolder dlg(mpt::PathString::FromNative(szPath), prompt);
	if(dlg.Show(this))
	{
		::SetDlgItemTextW(m_hWnd, nID, dlg.GetDirectory().AsNative().c_str());
		OnSettingsChanged();
	}
}


void PathConfigDlg::OnAutosaveEnable()
//------------------------------------
{
	BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE);
	GetDlgItem(IDC_AUTOSAVE_INTERVAL)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_HISTORY)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_USEORIGDIR)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_USECUSTOMDIR)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_PATH)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_BROWSE)->EnableWindow(enabled);
	OnSettingsChanged();
	return;
}


void PathConfigDlg::OnAutosaveUseOrigDir()
//----------------------------------------
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
//-------------------------------------
{
	SetModified(TRUE);
}


BOOL PathConfigDlg::OnSetActive()
//--------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_PATHS;
	return CPropertyPage::OnSetActive();
}


BOOL PathConfigDlg::OnKillActive()
//--------------------------------
{
	WCHAR szPath[MAX_PATH] = L"";
	::GetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, szPath, CountOf(szPath));

	if (!::PathIsDirectoryW(szPath) && IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) && !IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR))
	{
		Reporting::Error("Backup path does not exist.");
		GetDlgItem(IDC_AUTOSAVE_PATH)->SetFocus();
		return 0;
	}

	return CPropertyPage::OnKillActive();
}

OPENMPT_NAMESPACE_END
