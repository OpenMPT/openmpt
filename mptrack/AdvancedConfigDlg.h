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

#include "CListCtrl.h"
#include <unordered_map>
#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
#define UNORDERED_MAP std::tr1::unordered_map
#else
#define UNORDERED_MAP std::unordered_map
#endif

OPENMPT_NAMESPACE_BEGIN

//==========================================
class COptionsAdvanced: public CPropertyPage
//==========================================
{
protected:
	CListCtrlEx m_List;
	std::vector<SettingPath> m_indexToPath;
	UNORDERED_MAP<mpt::ustring, int> m_groups;
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

#endif // MPT_SETTINGS_CACHE
