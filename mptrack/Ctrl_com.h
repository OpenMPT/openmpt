/*
 * ctrl_com.h
 * ----------
 * Purpose: Song comments tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

//========================================
class CCtrlComments: public CModControlDlg
//========================================
{
protected:
	int charWidth;
	bool m_Reformatting;
	UINT m_nLockCount;

public:
	CCtrlComments(CModControlView &parent, CModDoc &document);
	Setting<LONG> &GetSplitPosRef() {return TrackerSettings::Instance().glCommentsWindowHeight;} 	//rewbs.varWindowSize

public:
	//{{AFX_DATA(CCtrlComments)
	CEdit m_EditComments;
	//}}AFX_DATA
	//{{AFX_VIRTUAL(CCtrlComments)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void RecalcLayout();
	virtual void UpdateView(UpdateHint hint, CObject *pObj = nullptr);
	virtual CRuntimeClass *GetAssociatedViewClass();
	virtual void OnActivatePage(LPARAM);
	virtual void OnDeactivatePage();
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CCtrlComments)
	afx_msg void OnCommentsUpdated();
	afx_msg void OnCommentsChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
