/*
 * PathConfigDlg.h
 * ---------------
 * Purpose: Default paths and auto save setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

class PathConfigDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(PathConfigDlg)

public:
	PathConfigDlg();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL OnKillActive();

protected:
	afx_msg void OnAutosaveEnable();
	afx_msg void OnAutosaveUseOrigDir();
	afx_msg void OnBrowseAutosavePath()	{ BrowseFolder(IDC_AUTOSAVE_PATH); }
	afx_msg void OnBrowseSongs()		{ BrowseFolder(IDC_OPTIONS_DIR_MODS); }
	afx_msg void OnBrowseSamples()		{ BrowseFolder(IDC_OPTIONS_DIR_SAMPS); }
	afx_msg void OnBrowseInstruments()	{ BrowseFolder(IDC_OPTIONS_DIR_INSTS); }
	afx_msg void OnBrowsePlugins()		{ BrowseFolder(IDC_OPTIONS_DIR_VSTS); }
	afx_msg void OnBrowsePresets()		{ BrowseFolder(IDC_OPTIONS_DIR_VSTPRESETS); }

	void OnSettingsChanged();
	BOOL OnSetActive();

	void BrowseFolder(UINT nID);
};

OPENMPT_NAMESPACE_END
