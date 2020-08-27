/*
 * Childfrm.h
 * ----------
 * Purpose: Implementation of tab interface class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "PatternCursor.h"

#include "../common/FileReaderFwd.h"

OPENMPT_NAMESPACE_BEGIN

class CModControlView;
class CModControlDlg;

struct GENERALVIEWSTATE
{
	PlugParamIndex nParam = 0;
	CHANNELINDEX nTab = 0;
	PLUGINDEX nPlugin = 0;
	bool initialized = false;
};


struct PATTERNVIEWSTATE
{
	PATTERNINDEX nPattern = 0;
	PatternCursor cursor = 0;
	PatternRect selection;
	PatternCursor::Columns nDetailLevel = PatternCursor::firstColumn;
	ORDERINDEX nOrder = 0;
	ORDERINDEX initialOrder = ORDERINDEX_INVALID;
	bool initialized = false;
};

struct SAMPLEVIEWSTATE
{
	SmpLength dwScrollPos = 0;
	SmpLength dwBeginSel = 0;
	SmpLength dwEndSel = 0;
	SAMPLEINDEX nSample = 0;
	SAMPLEINDEX initialSample = 0;
};


struct INSTRUMENTVIEWSTATE
{
	EnvelopeType nEnv = ENV_VOLUME;
	INSTRUMENTINDEX initialInstrument = 0;
	bool bGrid = false;
	bool initialized = false;
};

struct COMMENTVIEWSTATE
{
	UINT nId = 0;
	bool initialized = false;
};



class CChildFrame: public CMDIChildWnd
{
	friend class CModControlDlg;
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

protected:
	static CChildFrame *m_lastActiveFrame;
	static int glMdiOpenCount;

// Attributes
protected:
	CSplitterWnd m_wndSplitter;
	HWND m_hWndCtrl, m_hWndView;
	GENERALVIEWSTATE m_ViewGeneral;
	PATTERNVIEWSTATE m_ViewPatterns;
	SAMPLEVIEWSTATE m_ViewSamples;
	INSTRUMENTVIEWSTATE m_ViewInstruments;
	COMMENTVIEWSTATE m_ViewComments;
	CHAR m_szCurrentViewClassName[256];
	bool m_bMaxWhenClosed;
	bool m_bInitialActivation;

// Operations
public:
	CModControlView *GetModControlView() const { return (CModControlView *)m_wndSplitter.GetPane(0, 0); }
	BOOL ChangeViewClass(CRuntimeClass* pNewViewClass, CCreateContext* pContext=NULL);
	void ForceRefresh();
	void SavePosition(BOOL bExit=FALSE);
	const char *GetCurrentViewClassName() const;
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

	static CChildFrame *LastActiveFrame() { return m_lastActiveFrame; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
	public:
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	void ActivateFrame(int nCmdShow) override;
	void OnUpdateFrameTitle(BOOL bAddToTitle) override;
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChildFrame();

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	afx_msg void OnDestroy();
	afx_msg BOOL OnNcActivate(BOOL bActivate);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd *pActivateWnd, CWnd *pDeactivateWnd);
	afx_msg LRESULT OnChangeViewClass(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnInstrumentSelected(WPARAM, LPARAM lParam);
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


OPENMPT_NAMESPACE_END
