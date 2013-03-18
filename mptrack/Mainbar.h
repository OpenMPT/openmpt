/*
 * mainbar.h
 * ---------
 * Purpose: Implementation of OpenMPT's window toolbar.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define MIN_BASEOCTAVE		0
#define MAX_BASEOCTAVE		8

class CSoundFile;
class CModDoc;
class CModTree;

//===============================
class CToolBarEx: public CToolBar
//===============================
{
protected:
	BOOL m_bVertical, m_bFlatButtons;

public:
	CToolBarEx() { m_bVertical = m_bFlatButtons = FALSE; }
	virtual ~CToolBarEx() {}

public:
	BOOL EnableControl(CWnd &wnd, UINT nIndex, UINT nHeight=0);
	void ChangeCtrlStyle(LONG lStyle, BOOL bSetStyle);
	void EnableFlatButtons(BOOL bFlat);

public:
	//{{AFX_VIRTUAL(CToolBarEx)
	virtual CSize CalcDynamicLayout(int nLength, DWORD dwMode);
	virtual BOOL SetHorizontal();
	virtual BOOL SetVertical();
	//}}AFX_VIRTUAL
};


//===================================
class CMainToolBar: public CToolBarEx
//===================================
{
protected:
	CStatic m_EditTempo, m_EditSpeed, m_EditOctave, m_EditRowsPerBeat;
	CStatic m_StaticTempo, m_StaticSpeed, m_StaticRowsPerBeat;
	CSpinButtonCtrl m_SpinTempo, m_SpinSpeed, m_SpinOctave, m_SpinRowsPerBeat;
	int nCurrentSpeed, nCurrentTempo, nCurrentOctave, nCurrentRowsPerBeat;

public:
	CMainToolBar() {}
	virtual ~CMainToolBar() {}

protected:
	void SetRowsPerBeat(ROWINDEX nNewRPB);

public:
	//{{AFX_VIRTUAL(CMainToolBar)
	virtual BOOL SetHorizontal();
	virtual BOOL SetVertical();
	//}}AFX_VIRTUAL

public:
	BOOL Create(CWnd *parent);
	void Init(CMainFrame *);
	UINT GetBaseOctave() const;
	BOOL SetBaseOctave(UINT nOctave);
	BOOL SetCurrentSong(CSoundFile *pModDoc);

protected:
	//{{AFX_MSG(CMainToolBar)
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#define MTB_VERTICAL	0x01
#define MTB_CAPTURE		0x02
#define MTB_DRAGGING	0x04
#define MTB_TRACKER		0x08

//==================================
class CModTreeBar: public CDialogBar
//==================================
{
protected:
	DWORD m_dwStatus; // MTB_XXXX
	UINT m_nCursorDrag;
	CPoint ptDragging;
	UINT m_cxOriginal, m_cyOriginal, m_nTrackPos;
	UINT m_nTreeSplitRatio;

public:
	CModTree *m_pModTree, *m_pModTreeData;

	CModTreeBar();
	virtual ~CModTreeBar();

public:
	VOID Init();
	VOID RecalcLayout();
	VOID DoMouseMove(CPoint point);
	VOID DoLButtonDown(CPoint point);
	VOID DoLButtonUp();
	VOID CancelTracking();
	VOID OnInvertTracker(UINT x);
	VOID RefreshDlsBanks();
	VOID RefreshMidiLibrary();
	VOID OnOptionsChanged();
	VOID OnDocumentCreated(CModDoc *pModDoc);
	VOID OnDocumentClosed(CModDoc *pModDoc);
	VOID OnUpdate(CModDoc *pModDoc, DWORD lHint=0, CObject *pHint=NULL);
	VOID UpdatePlayPos(CModDoc *pModDoc, MPTNOTIFICATION *pNotify);
	HWND GetModTreeHWND(); //rewbs.customKeys
	BOOL PostMessageToModTree(UINT cmdID, WPARAM wParam, LPARAM lParam); //rewbs.customKeys


protected:
	//{{AFX_VIRTUAL(CModTreeBar)
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CModTreeBar)
	afx_msg void OnNcPaint();
#if _MFC_VER > 0x0710
	afx_msg LRESULT OnNcHitTest(CPoint point);
#else
	afx_msg UINT OnNcHitTest(CPoint point);
#endif 
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnNcLButtonUp(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnNcRButtonDown(UINT, CPoint) { CancelTracking(); }
	afx_msg void OnRButtonDown(UINT, CPoint) { CancelTracking(); }
	afx_msg LRESULT OnInitDialog(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
