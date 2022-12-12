/*
 * Mainbar.h
 * ---------
 * Purpose: Implementation of OpenMPT's window toolbar.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "UpdateToolTip.h"

OPENMPT_NAMESPACE_BEGIN

class CStereoVU: public CStatic
{
protected:
	uint8 numChannels;
	uint32 vuMeter[4];
	DWORD lastVuUpdateTime;
	int lastV[4];
	bool lastClip[4];
	bool horizontal;
	bool allowRightToLeft;

public:
	CStereoVU() { numChannels = 2; MemsetZero(vuMeter); lastVuUpdateTime = timeGetTime(); horizontal = true; MemsetZero(lastV); MemsetZero(lastClip); allowRightToLeft = false; }
	void SetVuMeter(uint8 validChannels, const uint32 channels[4], bool force=false);
	void SetOrientation(bool h) { horizontal = h; }

protected:
	void DrawVuMeters(CDC &dc, bool redraw=false);
	void DrawVuMeter(CDC &dc, const CRect &rect, int index, bool redraw=false);

protected:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT, CPoint);
	DECLARE_MESSAGE_MAP();
};

#define MIN_BASEOCTAVE		0
#define MAX_BASEOCTAVE		8

class CSoundFile;
class CModDoc;
class CModTree;
class CMainFrame;

class CToolBarEx: public CToolBar
{
protected:
	bool m_bVertical = false, m_bFlatButtons = false;

public:
	CToolBarEx() {}
	~CToolBarEx() override {}

public:
	BOOL EnableControl(CWnd &wnd, UINT nIndex, UINT nHeight=0);
	void ChangeCtrlStyle(LONG lStyle, BOOL bSetStyle);
	void EnableFlatButtons(BOOL bFlat);

public:
	//{{AFX_VIRTUAL(CToolBarEx)
	CSize CalcDynamicLayout(int nLength, DWORD dwMode) override;
	virtual void SetHorizontal();
	virtual void SetVertical();
	//}}AFX_VIRTUAL
};


class CMainToolBar: public CToolBarEx
{
protected:
	UpdateToolTip m_tooltip;
	CImageListEx m_ImageList, m_ImageListDisabled;
	CStatic m_EditTempo, m_EditSpeed, m_EditOctave, m_EditRowsPerBeat;
	CStatic m_StaticTempo, m_StaticSpeed, m_StaticRowsPerBeat;
	CSpinButtonCtrl m_SpinTempo, m_SpinSpeed, m_SpinOctave, m_SpinRowsPerBeat;
	int nCurrentSpeed, nCurrentOctave, nCurrentRowsPerBeat;
	TEMPO nCurrentTempo;
public:
	CStereoVU m_VuMeter;

public:
	CMainToolBar() {}
	~CMainToolBar() override {}

protected:
	void SetRowsPerBeat(ROWINDEX nNewRPB);

public:
	//{{AFX_VIRTUAL(CMainToolBar)
	void SetHorizontal() override;
	void SetVertical() override;
	//}}AFX_VIRTUAL

public:
#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif // MPT_COMPILER_CLANG
	BOOL Create(CWnd *parent);
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG
	void Init(CMainFrame *);
	UINT GetBaseOctave() const;
	BOOL SetBaseOctave(UINT nOctave);
	BOOL SetCurrentSong(CSoundFile *pModDoc);

	bool ShowUpdateInfo(const CString &newVersion, const CString &infoURL, bool showHighLight);
	void RemoveUpdateInfo();

protected:
	//{{AFX_MSG(CMainToolBar)
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnTbnDropDownToolBar(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectMIDIDevice(UINT id);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


class CModTreeBar: public CDialogBar
{
protected:
	enum Status
	{
		MTB_VERTICAL = 0x01,
		MTB_CAPTURE = 0x02,
		MTB_DRAGGING = 0x04,
		MTB_TRACKER = 0x08,
	};

	DWORD m_dwStatus = 0; // MTB_XXXX
	UINT m_nCursorDrag = 0;
	CPoint ptDragging;
	UINT m_cxOriginal = 0, m_cyOriginal = 0, m_nTrackPos = 0;
	UINT m_nTreeSplitRatio = 0;

	CEdit m_filterEdit;
	CModTree *m_filterSource = nullptr;
	UINT_PTR m_filterTimer = 0;

public:
	CModTree *m_pModTree = nullptr, *m_pModTreeData = nullptr;

	CModTreeBar();
	~CModTreeBar() override;

public:
	void Init();
	void RecalcLayout();
	void DoMouseMove(CPoint point);
	void DoLButtonDown(CPoint point);
	void DoLButtonUp();
	void CancelTracking();
	void OnInvertTracker(UINT x);
	void RefreshDlsBanks();
	void RefreshMidiLibrary();
	void OnOptionsChanged();
	void OnDocumentCreated(CModDoc *pModDoc);
	void OnDocumentClosed(CModDoc *pModDoc);
	void OnUpdate(CModDoc *pModDoc, UpdateHint hint, CObject *pHint = nullptr);
	void UpdatePlayPos(CModDoc *pModDoc, Notification *pNotify);
	HWND GetModTreeHWND(); //rewbs.customKeys
	LRESULT SendMessageToModTree(UINT cmdID, WPARAM wParam, LPARAM lParam);
	bool SetTreeSoundfile(FileReader &file);

	void StartTreeFilter(CModTree &source);

protected:
	//{{AFX_VIRTUAL(CModTreeBar)
	CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz) override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	//}}AFX_VIRTUAL

	void CloseTreeFilter();
	void CancelTimer();

protected:
	//{{AFX_MSG(CModTreeBar)
	afx_msg void OnNcPaint();
	afx_msg LRESULT OnNcHitTest(CPoint point);
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
	afx_msg void OnFilterChanged();
	afx_msg void OnFilterLostFocus();
	afx_msg void OnTimer(UINT_PTR id);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


OPENMPT_NAMESPACE_END
