// ChildFrm.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDFRM_H__AE144DCA_DD0B_11D1_AF24_444553540000__INCLUDED_)
#define AFX_CHILDFRM_H__AE144DCA_DD0B_11D1_AF24_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CModControlDlg;
class CChildFrame;


typedef struct _GENERALVIEWSTATE
{
	DWORD cbStruct;
	DWORD nPlugin;
	DWORD nParam;
	DWORD nTab;
} GENERALVIEWSTATE;


typedef struct PATTERNVIEWSTATE
{
	DWORD cbStruct;
	UINT nPattern;
	UINT nRow;
	UINT nCursor;
	DWORD dwBeginSel;
	DWORD dwEndSel;
	UINT nDetailLevel;
	UINT nOrder;		//rewbs.playSongFromCursor
} PATTERNVIEWSTATE;

typedef struct SAMPLEVIEWSTATE
{
	DWORD cbStruct;
	DWORD dwScrollPos;
	DWORD dwBeginSel;
	DWORD dwEndSel;
	UINT nSample;
} SAMPLEVIEWSTATE;


typedef struct INSTRUMENTVIEWSTATE
{
	DWORD cbStruct;
	DWORD nEnv;
} INSTRUMENTVIEWSTATE;

typedef struct COMMENTVIEWSTATE
{
	DWORD cbStruct;
	UINT nId;
} COMMENTVIEWSTATE;



//========================================
class CViewExSplitWnd: public CSplitterWnd
//========================================
{
	DECLARE_DYNAMIC(CViewExSplitWnd)

// Implementation
public:
	CViewExSplitWnd() {}
	~CViewExSplitWnd() {}
	CWnd* GetActivePane(int* pRow = NULL, int* pCol = NULL);
};


//====================================
class CChildFrame: public CMDIChildWnd
//====================================
{
	friend class CModControlDlg;
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

protected:
	static LONG glMdiOpenCount;

// Attributes
protected:
	CViewExSplitWnd m_wndSplitter;
	HWND m_hWndCtrl, m_hWndView;
	BOOL m_bMaxWhenClosed;
	GENERALVIEWSTATE m_ViewGeneral;
	PATTERNVIEWSTATE m_ViewPatterns;
	SAMPLEVIEWSTATE m_ViewSamples;
	INSTRUMENTVIEWSTATE m_ViewInstruments;
	COMMENTVIEWSTATE m_ViewComments;
	CHAR m_szCurrentViewClassName[256];
	bool m_bInitialActivation; //rewbs.fix3185:

// Operations
public:
	BOOL ChangeViewClass(CRuntimeClass* pNewViewClass, CCreateContext* pContext=NULL);
	void SavePosition(BOOL bExit=FALSE);
	CHAR* GetCurrentViewClassName();	//rewbs.varWindowSize
	LRESULT SendViewMessage(UINT uMsg, LPARAM lParam=0) const;
	LRESULT ActivateView(UINT nId, LPARAM lParam) { return ::SendMessage(m_hWndCtrl, WM_MOD_ACTIVATEVIEW, nId, lParam); }
	HWND GetHwndCtrl() const { return m_hWndCtrl; }
	HWND GetHwndView() const { return m_hWndView; }
	GENERALVIEWSTATE *GetGeneralViewState() { return &m_ViewGeneral; }
	PATTERNVIEWSTATE *GetPatternViewState() { return &m_ViewPatterns; }
	SAMPLEVIEWSTATE *GetSampleViewState() { return &m_ViewSamples; }
	INSTRUMENTVIEWSTATE *GetInstrumentViewState() { return &m_ViewInstruments; }
	COMMENTVIEWSTATE *GetCommentViewState() { return &m_ViewComments; }

	void SetSplitterHeight(int x);		//rewbs.varWindowSize
	int GetSplitterHeight();	 	    //rewbs.varWindowSize

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow);
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	afx_msg void OnClose();
	afx_msg BOOL OnNcActivate(BOOL bActivate);
	afx_msg LRESULT OnChangeViewClass(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnInstrumentSelected(WPARAM, LPARAM lParam);
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSetFocus(CWnd* pOldWnd); //rewbs.customKeysAutoEffects
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__AE144DCA_DD0B_11D1_AF24_444553540000__INCLUDED_)
