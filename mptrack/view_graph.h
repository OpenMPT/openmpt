#pragma once
class CGraph;

class CViewGraph :
	public CModScrollView
{
public:
	CViewGraph(void);
	DECLARE_SERIAL(CViewGraph)

	virtual void OnDraw(CDC *);
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);

	~CViewGraph(void);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnDestroy();

protected:
	CGraph* m_pGraph;

};
