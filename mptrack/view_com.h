/*
 * view_com.h
 * ----------
 * Purpose: Song comments tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "CListCtrl.h"

OPENMPT_NAMESPACE_BEGIN

//========================================
class CViewComments: public CModScrollView
//========================================
{
public:
	CViewComments();
	DECLARE_SERIAL(CViewComments)

protected:
	CModControlBar m_ToolBar;
	CListCtrlEx m_ItemList;
	UINT m_nCurrentListId, m_nListId;

public:
	void RecalcLayout();
	void UpdateButtonState();

public:
	//{{AFX_VIRTUAL(CViewComments)
	void OnInitialUpdate() override;
	void UpdateView(UpdateHint hint, CObject *pObject = nullptr) override;
	LRESULT OnModViewMsg(WPARAM, LPARAM) override;
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewGlobals)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowSamples();
	afx_msg void OnShowInstruments();
	afx_msg void OnShowPatterns();
	afx_msg void OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnBeginLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnDblClickListItem(NMHDR *, LRESULT *);
	afx_msg LRESULT OnMidiMsg(WPARAM midiData, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
