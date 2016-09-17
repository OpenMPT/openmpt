/*
 * AdvancedConfigDlg.h
 * -------------------
 * Purpose: Implementation of the advanced settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "CListCtrl.h"
#if MPT_USTRING_MODE_WIDE
#include <unordered_map>
#else
#include <map>
#endif

OPENMPT_NAMESPACE_BEGIN

//==========================================
class COptionsAdvanced: public CPropertyPage
//==========================================
{
protected:
	CListCtrlEx m_List;
#if MPT_USTRING_MODE_WIDE
	typedef std::unordered_map<mpt::ustring, int> GroupMap;
#else
	typedef std::map<mpt::ustring, int> GroupMap;
#endif
	std::vector<SettingPath> m_indexToPath;
	GroupMap m_groups;
	bool m_listGrouped;

public:
	COptionsAdvanced():CPropertyPage(IDD_OPTIONS_ADVANCED), m_listGrouped(false) {}

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG *msg);
	afx_msg void OnOptionDblClick(NMHDR *, LRESULT *);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnFindStringChanged() { ReInit(); }
	afx_msg void OnSaveNow();

	void ReInit();

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
