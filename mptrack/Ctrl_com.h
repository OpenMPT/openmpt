#ifndef _CONTROL_COMMENTS_H_
#define _CONTROL_COMMENTS_H_


//========================================
class CCtrlComments: public CModControlDlg
//========================================
{
protected:
	HFONT m_hFont;
	UINT m_nLockCount;

public:
	CCtrlComments();
	LONG* GetSplitPosRef() {return &CMainFrame::glCommentsWindowHeight;} 	//rewbs.varWindowSize

public:
	//{{AFX_DATA(CCtrlComments)
	CEdit m_EditComments;
	//}}AFX_DATA
	//{{AFX_VIRTUAL(CCtrlComments)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void RecalcLayout();
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual CRuntimeClass *GetAssociatedViewClass();
	virtual void OnActivatePage(LPARAM);
	virtual void OnDeactivatePage();
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CCtrlComments)
	afx_msg void OnCommentsChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
