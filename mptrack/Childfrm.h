/*
 * ChildFrm.h
 * ----------
 * Purpose: Implementation of tab interface class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "PatternCursor.h"

OPENMPT_NAMESPACE_BEGIN

class CModControlView;
class CModControlDlg;
class FileReader;
class CChildFrame;

typedef struct _GENERALVIEWSTATE
{
	DWORD cbStruct;
	PlugParamIndex nParam;
	CHANNELINDEX nTab;
	PLUGINDEX nPlugin;
} GENERALVIEWSTATE;


typedef struct PATTERNVIEWSTATE
{
	DWORD cbStruct;
	PATTERNINDEX nPattern;
	PatternCursor cursor;
	PatternRect selection;
	PatternCursor::Columns nDetailLevel;
	ORDERINDEX nOrder;		//rewbs.playSongFromCursor
	ORDERINDEX initialOrder;
} PATTERNVIEWSTATE;

typedef struct SAMPLEVIEWSTATE
{
	SmpLength dwScrollPos;
	SmpLength dwBeginSel;
	SmpLength dwEndSel;
	SAMPLEINDEX nSample;
	SAMPLEINDEX initialSample;
} SAMPLEVIEWSTATE;


typedef struct INSTRUMENTVIEWSTATE
{
	DWORD cbStruct;
	enmEnvelopeTypes nEnv;
	INSTRUMENTINDEX initialInstrument;
	bool bGrid;
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
	//CWnd* GetActivePane(int* pRow = NULL, int* pCol = NULL);
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
	static int glMdiOpenCount;

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
	bool m_bInitialActivation; //rewbs.fix3185

// Operations
public:
	CModControlView *GetModControlView() const { return (CModControlView *)m_wndSplitter.GetPane(0, 0); }
	BOOL ChangeViewClass(CRuntimeClass* pNewViewClass, CCreateContext* pContext=NULL);
	void ForceRefresh();
	void SavePosition(BOOL bExit=FALSE);
	CHAR* GetCurrentViewClassName();	//rewbs.varWindowSize
	LRESULT SendViewMessage(UINT uMsg, LPARAM lParam=0) const;
	LRESULT ActivateView(UINT nId, LPARAM lParam) { return ::SendMessage(m_hWndCtrl, WM_MOD_ACTIVATEVIEW, nId, lParam); }
	HWND GetHwndCtrl() const { return m_hWndCtrl; }
	HWND GetHwndView() const { return m_hWndView; }
	GENERALVIEWSTATE &GetGeneralViewState() { return m_ViewGeneral; }
	PATTERNVIEWSTATE &GetPatternViewState() { return m_ViewPatterns; }
	SAMPLEVIEWSTATE &GetSampleViewState() { return m_ViewSamples; }
	INSTRUMENTVIEWSTATE &GetInstrumentViewState() { return m_ViewInstruments; }
	COMMENTVIEWSTATE &GetCommentViewState() { return m_ViewComments; }

	void SetSplitterHeight(int x);
	int GetSplitterHeight();

	std::string SerializeView() const;
	void DeserializeView(FileReader &file);

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


OPENMPT_NAMESPACE_END
