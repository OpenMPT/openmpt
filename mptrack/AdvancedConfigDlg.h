/*
 * AdvancedConfigDlg.h
 * -------------------
 * Purpose: Implementation of the advanced settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#if defined(MPT_SETTINGS_CACHE)

OPENMPT_NAMESPACE_BEGIN

//==========================================
class COptionsAdvanced: public CPropertyPage
//==========================================
{
protected:
	CListBox m_List;
	std::map<int, SettingPath> m_IndexToPath;

public:
	COptionsAdvanced():CPropertyPage(IDD_OPTIONS_ADVANCED) {}

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG *msg);
	afx_msg void OnOptionDblClick();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnFindStringChanged() { ReInit(); }

	void ReInit();

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END

#endif // MPT_SETTINGS_CACHE
