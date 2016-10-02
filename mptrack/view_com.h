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
	CModDoc* GetDocument() const { return (CModDoc *)m_pDocument; }
	VOID RecalcLayout();
	VOID UpdateButtonState();

public:
	//{{AFX_VIRTUAL(CViewComments)
// -> CODE#0015
// -> DESC="channels management dlg"
//	virtual void OnDraw(CDC *) {}
	virtual void OnDraw(CDC *);
// -! NEW_FEATURE#0015
	virtual void OnInitialUpdate();
	virtual void UpdateView(UpdateHint hint, CObject *pObject = nullptr);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);
	LRESULT OnMidiMsg(WPARAM midiData, LPARAM);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewGlobals)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowSamples();
	afx_msg void OnShowInstruments();
	afx_msg void OnShowPatterns();
	afx_msg VOID OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg VOID OnBeginLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnDblClickListItem(NMHDR *, LRESULT *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
