#ifndef _VIEW_COMMENTS_H_
#define _VIEW_COMMENTS_H_


//===============================
class CViewComments: public CView
//===============================
{
public:
	CViewComments();
	DECLARE_SERIAL(CViewComments)

protected:
	CModControlBar m_ToolBar;
	CListCtrl m_ItemList;
	UINT m_nCurrentListId, m_nListId;

public:
	CModDoc* GetDocument() const { return (CModDoc *)m_pDocument; }
	VOID RecalcLayout();
	VOID UpdateButtonState();

public:
	//{{AFX_VIRTUAL(CViewComments)
	virtual void OnDraw(CDC *) {}
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewGlobals)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowSamples();
	afx_msg void OnShowInstruments();
	afx_msg void OnShowPatterns();
	afx_msg VOID OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // _VIEW_COMMENTS_H_
