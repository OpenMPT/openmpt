/*
 * AdvancedConfigDlg.h
 * -------------------
 * Purpose: Implementation of the advanced settings dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "CListCtrl.h"
#if MPT_USTRING_MODE_WIDE
#include <unordered_map>
#else
#include <map>
#endif

OPENMPT_NAMESPACE_BEGIN


#ifdef MPT_MFC_FULL

class CAdvancedSettingsList : public CMFCListCtrlEx
{
private:
	std::vector<SettingPath> & m_indexToPath;
public:
	CAdvancedSettingsList(std::vector<SettingPath> & indexToPath) : m_indexToPath(indexToPath) {}
	COLORREF OnGetCellBkColor(int nRow, int nColumn) override;
	COLORREF OnGetCellTextColor(int nRow, int nColumn) override;
};

#endif // MPT_MFC_FULL


class COptionsAdvanced: public CPropertyPage
{
protected:
#ifdef MPT_MFC_FULL
	CAdvancedSettingsList m_List;
#else // MPT_MFC_FULL
	CListCtrlEx m_List;
#endif // !MPT_MFC_FULL
#if MPT_USTRING_MODE_WIDE
	typedef std::unordered_map<mpt::ustring, int> GroupMap;
#else
	typedef std::map<mpt::ustring, int> GroupMap;
#endif
	std::vector<SettingPath> m_indexToPath;
	GroupMap m_groups;
	bool m_listGrouped = false;

public:
#ifdef MPT_MFC_FULL
	COptionsAdvanced():CPropertyPage(IDD_OPTIONS_ADVANCED), m_List(m_indexToPath) {}
#else // !MPT_MFC_FULL
	COptionsAdvanced():CPropertyPage(IDD_OPTIONS_ADVANCED) {}
#endif // MPT_MFC_FULL

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
#ifndef MPT_MFC_FULL
	afx_msg void OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult);
#endif // !MPT_MFC_FULL

	void ReInit();

	DECLARE_MESSAGE_MAP();
};

OPENMPT_NAMESPACE_END
