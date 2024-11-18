/*
 * Mainbar.h
 * ---------
 * Purpose: Implementation of OpenMPT's window toolbar and parent container of the tree view.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "CDecimalSupport.h"
#include "CImageListEx.h"
#include "UpdateHints.h"
#include "UpdateToolTip.h"
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

enum class MainToolBarItem : uint8;
class CMainToolBar;

class CStereoVU: public CStatic
{
protected:
	uint8 numChannels = 2;
	uint32 vuMeter[4] = {{}};
	DWORD lastVuUpdateTime;
	int lastV[4] = {{}};
	bool lastClip[4] = {{}};
	bool horizontal = true;
	bool allowRightToLeft = false;

public:
	CStereoVU() { lastVuUpdateTime = timeGetTime(); }
	void SetVuMeter(uint8 validChannels, const uint32 channels[4], bool force = false);
	void SetOrientation(bool h) { horizontal = h; }

protected:
	void DrawVuMeters(CDC &dc, bool redraw = false);
	void DrawVuMeter(CDC &dc, const CRect &rect, int index, bool redraw = false);

protected:
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT, CPoint);
	DECLARE_MESSAGE_MAP();
};

class COctaveEdit : public CEdit
{
public:
	COctaveEdit(CMainToolBar &owner) : m_owner{owner} { }

protected:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

	DECLARE_MESSAGE_MAP()

	CMainToolBar &m_owner;
};

#define MIN_BASEOCTAVE		0
#define MAX_BASEOCTAVE		8

class CSoundFile;
class CModDoc;
class CModTree;
class CMainFrame;
struct Notification;

class CToolBarEx: public CToolBar
{
protected:
	bool m_bVertical = false;

public:
	CToolBarEx() {}
	~CToolBarEx() override {}

public:
	void UpdateControl(bool show, CWnd &wnd, int index, int id, int height = 0);
	void EnableFlatButtons(bool flat);
	void SetButtonVisibility(int index, bool visible);

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
	CFont m_font;
	COctaveEdit m_EditOctave;
	CNumberEdit m_EditTempo;
	CEdit m_EditSpeed, m_EditRowsPerBeat, m_EditGlobalVolume;
	CStatic m_StaticTempo, m_StaticSpeed, m_StaticRowsPerBeat, m_StaticGlobalVolume;
	CSpinButtonCtrl m_SpinTempo, m_SpinSpeed, m_SpinOctave, m_SpinRowsPerBeat, m_SpinGlobalVolume;
	int m_currentSpeed = 0, m_currentOctave = -1, m_currentRowsPerBeat = 0, m_currentGlobalVolume = 0;
	TEMPO m_currentTempo{1, 0};
	bool m_updating = false;
public:
	CStereoVU m_VuMeter;

public:
	CMainToolBar() : m_EditOctave{*this} { }

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
	void SetBaseOctave(UINT nOctave);
	void SetCurrentSong(CSoundFile *pModDoc);

	bool ShowUpdateInfo(const CString &newVersion, const CString &infoURL, bool showHighLight);
	void RemoveUpdateInfo();

	bool ToggleVisibility(MainToolBarItem item);

protected:
	void RefreshToolbar();
	void UpdateSizes();
	void UpdateControls();

	//{{AFX_MSG(CMainToolBar)
	afx_msg LRESULT OnDPIChangedAfterParent(WPARAM, LPARAM);
	afx_msg void OnVScroll(UINT, UINT, CScrollBar *);
	afx_msg void OnTbnDropDownToolBar(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectMIDIDevice(UINT id);

	afx_msg void OnSpeedChanged();
	afx_msg void OnTempoChanged();
	afx_msg void OnRPBChanged();
	afx_msg void OnGlobalVolChanged();

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

	void DelayShow(BOOL show) override;

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
	bool SetTreeSoundfile(FileReader &file);

	void StartTreeFilter(CModTree &source);

	void SetBarOnLeft(const bool left);
	bool BarOnLeft() { return (GetBarStyle() & CBRS_ALIGN_LEFT); }

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
