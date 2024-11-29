/*
 * PathConfigDlg.h
 * ---------------
 * Purpose: Default paths and auto save setup dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "AccessibleControls.h"

OPENMPT_NAMESPACE_BEGIN

class PathConfigDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(PathConfigDlg)

public:
	PathConfigDlg();

protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	void OnOK() override;
	BOOL OnInitDialog() override;

	afx_msg void OnAutosaveEnable();
	afx_msg void OnAutosaveUseOrigDir();
	afx_msg void OnAutosaveRetention();
	afx_msg void OnBrowseAutosavePath();
	afx_msg void OnBrowseSongs();
	afx_msg void OnBrowseSamples();
	afx_msg void OnBrowseInstruments();
	afx_msg void OnBrowsePlugins();
	afx_msg void OnBrowsePresets();

	void OnSettingsChanged();
	BOOL OnSetActive() override;

	void BrowseFolder(UINT nID);

	mpt::PathString GetPath(int id);

	DECLARE_MESSAGE_MAP()

	std::array<AccessibleEdit, 3> m_accessibleEdits;
	std::array<AccessibleButton, 6> m_browseButtons;
};

OPENMPT_NAMESPACE_END
