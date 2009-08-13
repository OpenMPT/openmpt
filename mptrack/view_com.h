#ifndef _VIEW_COMMENTS_H_
#define _VIEW_COMMENTS_H_

enum { 
	LINE_LENGTH = 128,	//was 81. must be larger than visible comment area.
};

//===============================
class CViewComments: public CModScrollView
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
// -> CODE#0015
// -> DESC="channels management dlg"
//	virtual void OnDraw(CDC *) {}
	virtual void OnDraw(CDC *);
// -! NEW_FEATURE#0015
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewGlobals)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowSamples();
	afx_msg void OnShowInstruments();
	afx_msg void OnShowPatterns();
	afx_msg VOID OnEndLabelEdit(LPNMHDR pnmhdr, LRESULT *pLResult);
	afx_msg void OnDblClickListItem(NMHDR *, LRESULT *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // _VIEW_COMMENTS_H_
