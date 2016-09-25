/*
 * ctrl_pat.h
 * ----------
 * Purpose: Pattern tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "globals.h"
#include "PatternCursor.h"

OPENMPT_NAMESPACE_BEGIN

class COrderList;
class CCtrlPatterns;

struct OrdSelection
{
	ORDERINDEX firstOrd;
	ORDERINDEX lastOrd;
	ORDERINDEX GetSelCount() const {return lastOrd - firstOrd + 1;}
};

//===========================
class COrderList: public CWnd
//===========================
{
	friend class CCtrlPatterns;
protected:
	HFONT m_hFont;
	COLORREF colorText, colorTextSel, colorInvalid;
	int m_cxFont, m_cyFont;
	//m_nXScroll  : The order at the beginning of shown orderlist
	//m_nScrollPos: The same as order
	//m_nScrollPos2nd: 2nd selection point if multiple orders are selected
	//	               (not neccessarily the higher order - GetCurSel() is taking care of that.)
	ORDERINDEX m_nXScroll, m_nScrollPos, m_nScrollPos2nd, m_nDropPos, m_nMouseDownPos;
	ORDERINDEX m_nDragOrder;
	//To tell how many orders('orderboxes') to show at least
	//on both sides of current order(when updating orderslist position).
	int m_nOrderlistMargins;
	CModDoc &m_pModDoc;
	CCtrlPatterns &m_pParent;
	bool m_bScrolling, m_bDragging;

public:
	COrderList(CCtrlPatterns &parent, CModDoc &document);
	virtual ~COrderList() {}

public:
	BOOL Init(const CRect&, HFONT hFont);
	void InvalidateSelection() const;
	PATTERNINDEX GetCurrentPattern() const;
	// make the current selection the secondary selection (used for keyboard orderlist navigation)
	inline void SetCurSelTo2ndSel(bool isSelectionKeyPressed)
	{
		if(isSelectionKeyPressed && m_nScrollPos2nd == ORDERINDEX_INVALID) m_nScrollPos2nd = m_nScrollPos;
		else if(!isSelectionKeyPressed && m_nScrollPos2nd != ORDERINDEX_INVALID) m_nScrollPos2nd = ORDERINDEX_INVALID;
	};
	// Why VC wants to inline this huge function is beyond my understanding...
	MPT_NOINLINE bool SetCurSel(ORDERINDEX sel, bool bEdit = true, bool bShiftClick = false, bool bIgnoreCurSel = false, bool setPlayPos = true);
	BOOL UpdateScrollInfo();
	void UpdateInfoText();
	int GetFontWidth();
	void QueuePattern(CPoint pt);

	ORDERINDEX GetOrderFromPoint(const CRect& rect, const CPoint& pt) const;

	// Get the currently selected pattern(s).
	// Set bIgnoreSelection to true if only the first selected point is important.
	OrdSelection GetCurSel(bool bIgnoreSelection) const;

	// Sets target margin value and returns the effective margin value.
	ORDERINDEX SetMargins(int);

	// Returns the effective margin value.
	ORDERINDEX GetMargins() { return GetMargins(GetMarginsMax()); }

	// Returns the effective margin value.
	ORDERINDEX GetMargins(const ORDERINDEX maxMargins) const { return std::min(maxMargins, static_cast<ORDERINDEX>(m_nOrderlistMargins)); }

	// Returns maximum margin value given current window width.
	ORDERINDEX GetMarginsMax() { return GetMarginsMax(GetLength()); }

	// Returns maximum margin value when shown sequence has nLength orders.
	// For example: If length is 4 orders -> maxMargins = 4/2 - 1 = 1;
	// if maximum is 5 -> maxMargins = (int)5/2 = 2
	ORDERINDEX GetMarginsMax(const ORDERINDEX length) const { return (length > 0 && length % 2 == 0) ? length / 2 - 1 : length / 2; }

	// Returns the number of sequence items visible in the list.
	ORDERINDEX GetLength();

	// Return true if given order is in margins given that first shown order
	// is 'startOrder'. Begin part of the whole sequence
	// is not interpreted to be in margins regardless of the margin value.
	bool IsOrderInMargins(int order, int startOrder);

	// Ensure that a given order index is visible in the orderlist view.
	void EnsureVisible(ORDERINDEX order);

	// Set given sqeuence and update orderlist display.
	void SelectSequence(const SEQUENCEINDEX nSeq);

	// Helper function for entering pattern number
	void EnterPatternNum(int enterNum);

	void OnCopy(bool onlyOrders);

	//{{AFX_VIRTUAL(COrderList)
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	virtual void UpdateView(UpdateHint hint, CObject *pObj = nullptr);
	//}}AFX_VIRTUAL

protected:
	void SetScrollPos(int pos);
	int GetScrollPos(bool getTrackPos = false);

	// Resizes the order list if the specified order is past the order list length
	bool EnsureEditable(ORDERINDEX ord);

	//{{AFX_MSG(COrderList)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSetFocus(CWnd *);
	afx_msg void OnKillFocus(CWnd *);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonDblClk(UINT, CPoint);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnMButtonDown(UINT, CPoint);
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSwitchToView();
	afx_msg void OnInsertOrder();
	afx_msg void OnInsertSeparatorPattern();
	afx_msg void OnDeleteOrder();
	afx_msg void OnRenderOrder();
	afx_msg void OnPatternProperties();
	afx_msg void OnPlayerPlay();
	afx_msg void OnPlayerPause();
	afx_msg void OnPlayerPlayFromStart();
	afx_msg void OnPatternPlayFromStart();
	afx_msg void OnCreateNewPattern();
	afx_msg void OnDuplicatePattern();
	afx_msg void OnPatternCopy();
	afx_msg void OnPatternPaste();
	afx_msg void OnSetRestartPos();
	afx_msg void OnEditCopy() { OnCopy(false); }
	afx_msg void OnEditCopyOrders() { OnCopy(true); }
	afx_msg void OnEditCut();
	afx_msg LRESULT OnDragonDropping(WPARAM bDoDrop, LPARAM lParam);
	afx_msg LRESULT OnHelpHitTest(WPARAM, LPARAM lParam);
	afx_msg void OnSelectSequence(UINT nid);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	afx_msg void OnLockPlayback();
	afx_msg void OnUnlockPlayback();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


// CPatEdit: Edit control that switches back to the pattern view if Tab key is pressed.

//==========================
class CPatEdit: public CEdit
//==========================
{
protected:
	CCtrlPatterns *m_pParent;

public:
	CPatEdit() { m_pParent = nullptr; }
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
	CPatEdit m_EditSpacing, m_EditPatName, m_EditSequence;
	CSpinButtonCtrl m_SpinInstrument, m_SpinSpacing, m_SpinSequence;
	CModControlBar m_ToolBar;
	INSTRUMENTINDEX m_nInstrument;
	PatternCursor::Columns m_nDetailLevel;			// Visible Columns
	BOOL m_bRecord, m_bVUMeters, m_bPluginNames;

public:
	CCtrlPatterns(CModControlView &parent, CModDoc &document);
	Setting<LONG> &GetSplitPosRef() {return TrackerSettings::Instance().glPatternWindowHeight;} 	//rewbs.varWindowSize

public:
	void SetCurrentPattern(PATTERNINDEX nPat);
	BOOL SetCurrentInstrument(UINT nIns);
	BOOL GetFollowSong() { return IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG); }
	BOOL GetLoopPattern() {return IsDlgButtonChecked(IDC_PATTERN_LOOP);}
	//{{AFX_VIRTUAL(CCtrlPatterns)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void RecalcLayout();
	virtual void UpdateView(UpdateHint hint = UpdateHint(), CObject *pObj = nullptr);
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
// -> CODE#0015
// -> DESC="channels management dlg"
	afx_msg void OnChannelManager();
// -! NEW_FEATURE#0015
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPlayerPause();
	afx_msg void OnPatternNew();
	afx_msg void OnPatternDuplicate();
	afx_msg void OnPatternStop();
	afx_msg void OnPatternPlay();
	afx_msg void OnPatternPlayNoLoop();		//rewbs.playSongFromCursor
	afx_msg void OnPatternPlayRow();
	afx_msg void OnPatternPlayFromStart();
	afx_msg void OnPatternRecord();
	afx_msg void OnPatternVUMeters();
	afx_msg void OnPatternViewPlugNames();	//rewbs.patPlugNames
	afx_msg void OnPatternProperties();
	afx_msg void OnPatternExpand();
	afx_msg void OnPatternShrink();
	afx_msg void OnPatternAmplify();
	afx_msg void OnPatternCopy();
	afx_msg void OnPatternPaste();
	afx_msg void OnFollowSong();
	afx_msg void OnChangeLoopStatus();
	afx_msg void OnSwitchToView();
	afx_msg void OnInstrumentChanged();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnSpacingChanged();
	afx_msg void OnPatternNameChanged();
	afx_msg void OnSequenceNameChanged();
	afx_msg void OnChordEditor();
	afx_msg void OnDetailLo();
	afx_msg void OnDetailMed();
	afx_msg void OnDetailHi();
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateRecord(CCmdUI *pCmdUI);
	afx_msg void TogglePluginEditor(); //rewbs.instroVST
	afx_msg void OnToggleOverflowPaste();
	afx_msg void OnSequenceNumChanged();
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	bool HasValidPlug(INSTRUMENTINDEX instr);
public:
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnXButtonUp(UINT nFlags, UINT nButton, CPoint point);
	afx_msg BOOL OnToolTip(UINT id, NMHDR *pTTTStruct, LRESULT *pResult);
};

OPENMPT_NAMESPACE_END
