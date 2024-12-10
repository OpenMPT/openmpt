/*
 * Childfrm.h
 * ----------
 * Purpose: Implementation of the MDI document child windows.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "PatternCursor.h"

#include "../common/FileReaderFwd.h"

OPENMPT_NAMESPACE_BEGIN

class CModControlView;
class CModControlDlg;

struct GeneralViewState
{
	PlugParamIndex nParam = 0;
	CHANNELINDEX nTab = 0;
	PLUGINDEX nPlugin = 0;
	bool initialized = false;

	std::string Serialize() const { return {}; }
	void Deserialize(FileReader &) { }
};


struct PatternViewState
{
	PATTERNINDEX nPattern = 0;
	PatternCursor cursor = 0;
	PatternRect selection;
	ORDERINDEX nOrder = 0;
	ORDERINDEX initialOrder = ORDERINDEX_INVALID;
	std::bitset<PatternCursor::numColumns> visibleColumns;
	bool initialized = false;

	std::string Serialize() const;
	void Deserialize(FileReader &f);
};

struct SampleViewState
{
	SmpLength dwScrollPos = 0;
	SmpLength dwBeginSel = 0;
	SmpLength dwEndSel = 0;
	SAMPLEINDEX nSample = 0;
	SAMPLEINDEX initialSample = 0;

	std::string Serialize() const;
	void Deserialize(FileReader &f);
};


struct InstrumentViewState
{
	float zoom = 4;
	EnvelopeType nEnv = ENV_VOLUME;
	INSTRUMENTINDEX initialInstrument = 0;
	INSTRUMENTINDEX instrument = 0;
	bool bGrid = false;
	bool initialized = false;

	std::string Serialize() const;
	void Deserialize(FileReader &f);
};

struct CommentsViewState
{
	UINT nId = 0;
	bool initialized = false;

	std::string Serialize() const { return {}; }
	void Deserialize(FileReader &) {}
};



class CChildFrame: public CMDIChildWnd
{
	friend class CModControlDlg;
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();
	~CChildFrame() override;

protected:
	static CChildFrame *m_lastActiveFrame;
	static int glMdiOpenCount;

// Attributes
protected:
	CSplitterWnd m_wndSplitter;
	HWND m_hWndCtrl = nullptr, m_hWndView = nullptr;
	GeneralViewState m_ViewGeneral;
	PatternViewState m_ViewPatterns;
	SampleViewState m_ViewSamples;
	InstrumentViewState m_ViewInstruments;
	CommentsViewState m_ViewComments;
	std::string m_currentViewClassName;
	int m_dpi = 0;
	bool m_maxWhenClosed = false;
	bool m_initialActivation = true;

// Operations
public:
	CModControlView *GetModControlView() const { return reinterpret_cast<CModControlView *>(m_wndSplitter.GetPane(0, 0)); }
	BOOL ChangeViewClass(CRuntimeClass *pNewViewClass, CCreateContext *pContext = nullptr);
	void ForceRefresh();
	void SavePosition(bool exit = false);
	LRESULT SendCtrlMessage(UINT uMsg, LPARAM lParam = 0) const;
	LRESULT SendViewMessage(UINT uMsg, LPARAM lParam = 0) const;
	LRESULT ActivateView(UINT nId, LPARAM lParam);
	HWND GetHwndCtrl() const { return m_hWndCtrl; }
	HWND GetHwndView() const { return m_hWndView; }
	GeneralViewState &GetGeneralViewState() { return m_ViewGeneral; }
	PatternViewState &GetPatternViewState() { return m_ViewPatterns; }
	SampleViewState &GetSampleViewState() { return m_ViewSamples; }
	InstrumentViewState &GetInstrumentViewState() { return m_ViewInstruments; }
	CommentsViewState &GetCommentViewState() { return m_ViewComments; }

	bool IsPatternView() const;

	void SetSplitterHeight(int x);
	int GetSplitterHeight();

	void SaveAllViewStates();
	std::string SerializeView();
	void DeserializeView(FileReader &file);

	void ToggleViews();

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

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	afx_msg LRESULT OnDPIChangedAfterParent(WPARAM, LPARAM);
	afx_msg void OnDestroy();
	afx_msg BOOL OnNcActivate(BOOL bActivate);
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd *pActivateWnd, CWnd *pDeactivateWnd);
	afx_msg LRESULT OnChangeViewClass(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnInstrumentSelected(WPARAM, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


OPENMPT_NAMESPACE_END
