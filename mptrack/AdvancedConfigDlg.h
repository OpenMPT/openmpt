/*
 * AdvancedConfigDlg.h
 * -------------------
 * Purpose: Implementation of the advanced settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "CListCtrl.h"
#if MPT_USTRING_MODE_WIDE
#include <unordered_map>
#else
#include <map>
#endif

OPENMPT_NAMESPACE_BEGIN

class SettingPath;

#ifndef _AFX_NO_MFC_CONTROLS_IN_DIALOGS

class CAdvancedSettingsList : public CMFCListCtrlEx
{
private:
	std::vector<SettingPath> & m_indexToPath;
public:
	CAdvancedSettingsList(std::vector<SettingPath> & indexToPath) : m_indexToPath(indexToPath) {}
	COLORREF OnGetCellBkColor(int nRow, int nColumn) override;
	COLORREF OnGetCellTextColor(int nRow, int nColumn) override;
};

#endif // !_AFX_NO_MFC_CONTROLS_IN_DIALOGS


class COptionsAdvanced: public CPropertyPage
{
#ifndef _AFX_NO_MFC_CONTROLS_IN_DIALOGS
	using ListCtrl = CAdvancedSettingsList;
#else  // _AFX_NO_MFC_CONTROLS_IN_DIALOGS
	using ListCtrl = CListCtrlEx;
#endif // !_AFX_NO_MFC_CONTROLS_IN_DIALOGS

protected:
	ListCtrl m_List;
#if MPT_USTRING_MODE_WIDE
	using GroupMap = std::unordered_map<mpt::ustring, int>;
#else
	using GroupMap = std::map<mpt::ustring, int>;
#endif
	std::vector<SettingPath> m_indexToPath;
	GroupMap m_groups;
	bool m_listGrouped = false;

public:
	COptionsAdvanced();
	~COptionsAdvanced();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;
	BOOL PreTranslateMessage(MSG *msg) override;

	afx_msg void OnOptionDblClick(NMHDR *, LRESULT *);
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnFindStringChanged() { ReInit(); }
	afx_msg void OnSaveNow();
#ifdef _AFX_NO_MFC_CONTROLS_IN_DIALOGS
	afx_msg void OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult);
#endif // _AFX_NO_MFC_CONTROLS_IN_DIALOGS

	void ReInit();

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
