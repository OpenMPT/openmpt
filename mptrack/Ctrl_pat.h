#ifndef _CONTROL_PATTERNS_H_
#define _CONTROL_PATTERNS_H_


class COrderList;
class CCtrlPatterns;

//===========================
class COrderList: public CWnd
//===========================
{
	friend class CCtrlPatterns;
protected:
	HFONT m_hFont;
	COLORREF colorText, colorTextSel;
	int m_cxFont, m_cyFont, m_nXScroll, m_nScrollPos, m_nDropPos;
	UINT m_nDragOrder;
	BOOL m_bScrolling, m_bDragging, m_bShift;
	CModDoc *m_pModDoc;
	CCtrlPatterns *m_pParent;

public:
	COrderList();
	virtual ~COrderList() {}

public:
	BOOL Init(const CRect&, CCtrlPatterns *pParent, CModDoc *, HFONT hFont);
	void InvalidateSelection() const;
	int GetCurSel() const { return m_nScrollPos; }
	UINT GetCurrentPattern() const;
	BOOL SetCurSel(int sel, BOOL bEdit=TRUE);
	BOOL ProcessKeyDown(UINT nChar);
	BOOL ProcessChar(UINT nChar);
	BOOL UpdateScrollInfo();
	void UpdateInfoText();
	int GetFontWidth();

public:
	//{{AFX_VIRTUAL(COrderList)
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(COrderList)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSetFocus(CWnd *);
	afx_msg void OnKillFocus(CWnd *);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonDblClk(UINT, CPoint);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSwitchToView();
	afx_msg void OnInsertOrder();
	afx_msg void OnDeleteOrder();
	afx_msg void OnPatternProperties();
	afx_msg void OnPlayerPlay();
	afx_msg void OnPlayerPause();
	afx_msg void OnPlayerPlayFromStart();
	afx_msg void OnPatternPlayFromStart();
	afx_msg void OnCreateNewPattern();
	afx_msg void OnDuplicatePattern();
	afx_msg void OnPatternCopy();
	afx_msg void OnPatternPaste();
	afx_msg LRESULT OnDragonDropping(WPARAM bDoDrop, LPARAM lParam);
	afx_msg LRESULT OnHelpHitTest(WPARAM, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//==========================
class CPatEdit: public CEdit
//==========================
{
protected:
	CCtrlPatterns *m_pParent;

public:
	CPatEdit() { m_pParent = NULL; }
	void SetParent(CCtrlPatterns *parent) { m_pParent = parent; }
	virtual BOOL PreTranslateMessage(MSG *pMsg);
};


//========================================
class CCtrlPatterns: public CModControlDlg
//========================================
{
	friend class COrderList;
protected:
	COrderList m_OrderList;
	CButton m_BtnPrev, m_BtnNext;
	CComboBox m_CbnInstrument;
	CPatEdit m_EditSpacing, m_EditPatName;
	CSpinButtonCtrl m_SpinInstrument, m_SpinSpacing;
	CModControlBar m_ToolBar;
	UINT m_nInstrument, m_nDetailLevel;
	BOOL m_bRecord, m_bVUMeters;

public:
	CCtrlPatterns();
	LONG* GetSplitPosRef() {return &CMainFrame::glPatternWindowHeight;} 	//rewbs.varWindowSize

public:
	void SetCurrentPattern(UINT nPat);
	BOOL SetCurrentInstrument(UINT nIns);
	BOOL GetFollowSong() { return IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG); }
	//{{AFX_VIRTUAL(CCtrlPatterns)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void RecalcLayout();
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual CRuntimeClass *GetAssociatedViewClass();
	virtual LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam);
	virtual void OnActivatePage(LPARAM);
	virtual void OnDeactivatePage();
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CCtrlPatterns)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSequenceNext();
	afx_msg void OnSequencePrev();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPlayerPause();
	afx_msg void OnPatternNew();
	afx_msg void OnPatternDuplicate();
	afx_msg void OnPatternStop();
	afx_msg void OnPatternPlay();
	afx_msg void OnPatternPlayRow();
	afx_msg void OnPatternPlayFromStart();
	afx_msg void OnPatternRecord();
	afx_msg void OnPatternVUMeters();
	afx_msg void OnPatternProperties();
	afx_msg void OnPatternExpand();
	afx_msg void OnPatternShrink();
	afx_msg void OnPatternAmplify();
	afx_msg void OnPatternCopy();
	afx_msg void OnPatternPaste();
	afx_msg void OnFollowSong();
	afx_msg void OnSwitchToView();
	afx_msg void OnInstrumentChanged();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnSpacingChanged();
	afx_msg void OnPatternNameChanged();
	afx_msg void OnSetupZxxMacros();
	afx_msg void OnChordEditor();
	afx_msg void OnDetailLo();
	afx_msg void OnDetailMed();
	afx_msg void OnDetailHi();
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateRecord(CCmdUI *pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif
