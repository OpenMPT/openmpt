/*
 * GeneralConfigDlg.h
 * ------------------
 * Purpose: Implementation of the general settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "AccessibleControls.h"

OPENMPT_NAMESPACE_BEGIN

class COptionsGeneral: public CPropertyPage
{
protected:
	CEdit m_defaultArtist;
	CComboBox m_defaultTemplate, m_defaultFormat;
	CCheckListBox m_CheckList;
	AccessibleButton m_templateBrowseButton;

public:
	COptionsGeneral();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;

	afx_msg void OnOptionSelChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnBrowseTemplate();
	afx_msg void OnDefaultTypeChanged();
	afx_msg void OnTemplateChanged();

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
