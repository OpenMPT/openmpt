/*
 * View_smp.h
 * ----------
 * Purpose: Sample tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "Globals.h"
#include "../soundlib/modsmp_ctrl.h"


OPENMPT_NAMESPACE_BEGIN

#define SMP_LEFTBAR_BUTTONS		8
class OPLInstrDlg;

class CViewSample: public CModScrollView
{
public:
	enum Flags
	{
		SMPSTATUS_MOUSEDRAG		= 0x01,
		SMPSTATUS_KEYDOWN		= 0x02,
		SMPSTATUS_NCLBTNDOWN	= 0x04,
		SMPSTATUS_DRAWING		= 0x08,
	};

protected:
	enum class PasteMode
	{
		Replace,
		MixPaste,
		Insert
	};

	enum class HitTestItem
	{
		Nothing,
		SampleData,
		SelectionStart,
		SelectionEnd,
		LoopStart,
		LoopEnd,
		SustainStart,
		SustainEnd,
		CuePointFirst,
		CuePointLast = CuePointFirst + mpt::array_size<decltype(ModSample::cues)>::size - 1,
	};

	std::unique_ptr<OPLInstrDlg> m_oplEditor;
	CImageList m_bmpEnvBar;
	CRect m_rcClient;
	CDC m_offScreenDC, m_waveformDC;
	CBitmap m_offScreenBitmap, m_waveformBitmap;
	CFont m_timelineFont;
	SIZE m_sizeTotal;
	UINT m_nBtnMouseOver = 0xFFFF;
	int m_nZoom = 0;	// < 0: Zoom into sample (2^x:1 ratio), 0: Auto zoom, > 0: Zoom out (1:2^x ratio)
	int m_timelineHeight = 0;
	int m_timelineUnit = 0;
	int m_timelineInterval = 0;
	decltype(ModSample::nC5Speed) m_cachedSampleRate = 8363;
	FlagSet<Flags> m_dwStatus;
	SmpLength m_dwBeginSel, m_dwEndSel, m_dwBeginDrag, m_dwEndDrag;
	SmpLength m_dwMenuParam;
	SmpLength m_nGridSegments = 0;
	SAMPLEINDEX m_nSample = 1;
	HitTestItem m_dragItem = HitTestItem::Nothing;
	CPoint m_startDragPoint;
	SmpLength m_startDragValue = MAX_SAMPLE_LENGTH;
	bool m_dragPreparedUndo = false, m_fineDrag = false, m_forceRedrawWaveform = true, m_scrolledSinceLastMouseMove = false;

	// Sample drawing
	CPoint m_lastDrawPoint;		// For drawing horizontal lines
	int m_drawChannel;			// Which sample channel are we drawing on?

	// Note-off event buffer for MIDI sustain pedal
	std::array<std::vector<uint32>, 16> m_midiSustainBuffer;
	std::bitset<16> m_midiSustainActive;

	DWORD m_NcButtonState[SMP_LEFTBAR_BUTTONS];
	std::array<SmpLength, MAX_CHANNELS> m_dwNotifyPos;
	CModDoc::NoteToChannelMap m_noteChannel;	// Note -> Preview channel assignment

public:
	CViewSample();
	DECLARE_SERIAL(CViewSample)

	static std::pair<int, int> FindMinMax(const int8 *p, SmpLength numSamples, int numChannels);
	static std::pair<int, int> FindMinMax(const int16 *p, SmpLength numSamples, int numChannels);

protected:
	MPT_NOINLINE void SetModified(SampleHint hint, bool updateAll, bool waveformModified);
	void UpdateScrollSize() { UpdateScrollSize(m_nZoom, true); }
	void UpdateScrollSize(int newZoom, bool forceRefresh, SmpLength centeredSample = SmpLength(-1));
	void UpdateOPLEditor();
	void SetCurrentSample(SAMPLEINDEX nSmp);
	bool IsOPLInstrument() const;
	void SetZoom(int nZoom, SmpLength centeredSample = SmpLength(-1));
	int32 SampleToScreen(SmpLength pos, bool ignoreScrollPos = false) const;
	SmpLength ScreenToSample(int32 x, bool ignoreSampleLength = false) const;
	int32 SecondsToScreen(double x) const;
	double ScreenToSeconds(int32 x, bool ignoreSampleLength = false) const;
	std::pair<HitTestItem, SmpLength> PointToItem(CPoint point, CRect *rect = nullptr) const;
	void PlayNote(ModCommand::NOTE note, const SmpLength nStartPos = 0, int volume = -1);
	void NoteOff(ModCommand::NOTE note);
	void InvalidateSample(bool invalidateWaveform = true);
	void InvalidateTimeline();
	void SetCurSel(SmpLength nBegin, SmpLength nEnd);
	void ScrollToPosition(int x);
	void DrawPositionMarks();
	void DrawSampleData1(HDC hdc, int ymed, int cx, int cy, SmpLength len, SampleFlags uFlags, const void *pSampleData);
	void DrawSampleData2(HDC hdc, int ymed, int cx, int cy, SmpLength len, SampleFlags uFlags, const void *pSampleData);
	void DrawNcButton(CDC *pDC, UINT nBtn);
	bool GetNcButtonRect(UINT button, CRect &rect) const;
	UINT GetNcButtonAtPoint(CPoint point, CRect *outRect = nullptr) const;
	void UpdateNcButtonState();
	void DoPaste(PasteMode pasteMode);

	// Sets sample data on sample draw.
	template<class T, class uT>
	void SetSampleData(ModSample &smp, const CPoint &point, const SmpLength old);

	// Sets initial draw point on sample draw.
	template<class T, class uT>
	void SetInitialDrawPoint(ModSample &smp, const CPoint &point);

	// Returns sample value corresponding given point in the sample view.
	template<class T, class uT>
	T GetSampleValueFromPoint(const ModSample &smp, const CPoint &point) const;

	int GetZoomLevel(SmpLength length) const;
	void DoZoom(int direction, const CPoint &zoomPoint = CPoint(-1, -1));
	bool CanZoomSelection() const;
	void ScrollToSample(SmpLength sample, bool refresh = true, bool centerSample = true);

	SmpLength ScrollPosToSamplePos() const {return ScrollPosToSamplePos(m_nZoom);}
	SmpLength ScrollPosToSamplePos(int nZoom) const;

	void OnMonoConvert(ctrlSmp::StereoToMonoMode convert);
	void TrimSample(bool trimToLoopEnd);

	int CalcScroll(int &currentPos, int amount, int style, int bar);

	SmpLength SnapToGrid(const SmpLength pos) const;

public:
	//{{AFX_VIRTUAL(CViewSample)
	void OnDraw(CDC *) override;
	void OnInitialUpdate() override;
	void UpdateView(UpdateHint hint, CObject *pObj = nullptr) override;
	LRESULT OnModViewMsg(WPARAM, LPARAM) override;
	BOOL OnDragonDrop(BOOL, const DRAGONDROP *) override;
	LRESULT OnPlayerNotify(Notification *) override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE) override;
	INT_PTR OnToolHitTest(CPoint point, TOOLINFO *pTI) const override;
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewSample)
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnNcPaint();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonDown(UINT, CPoint);
	afx_msg void OnNcLButtonUp(UINT, CPoint);
	afx_msg void OnNcLButtonDblClk(UINT, CPoint);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message);
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditDelete();
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditMixPaste();
	afx_msg void OnEditInsertPaste();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnSetLoop();
	afx_msg void OnSetSustainLoop();
	afx_msg void On8BitConvert();
	afx_msg void On16BitConvert();
	afx_msg void OnMonoConvertMix() { OnMonoConvert(ctrlSmp::mixChannels); }
	afx_msg void OnMonoConvertLeft() { OnMonoConvert(ctrlSmp::onlyLeft); }
	afx_msg void OnMonoConvertRight() { OnMonoConvert(ctrlSmp::onlyRight); }
	afx_msg void OnMonoConvertSplit() { OnMonoConvert(ctrlSmp::splitSample); }
	afx_msg void OnSampleTrim() { TrimSample(false); }
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnZoomOnSel();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnSetLoopStart();
	afx_msg void OnSetLoopEnd();
	afx_msg void OnConvertPingPongLoop();
	afx_msg void OnSetSustainStart();
	afx_msg void OnSetSustainEnd();
	afx_msg void OnConvertPingPongSustain();
	afx_msg void OnSetCuePoint(UINT nID);
	afx_msg void OnZoomUp();
	afx_msg void OnZoomDown();
	afx_msg void OnDrawingToggle();
	afx_msg void OnAddSilence();
	afx_msg void OnChangeGridSize();
	afx_msg void OnQuickFade();
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnXButtonUp(UINT nFlags, UINT nButton, CPoint point);
	afx_msg void OnUpdateUndo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRedo(CCmdUI *pCmdUI);
	afx_msg void OnSampleSlice();
	afx_msg void OnSampleInsertCuePoint();
	afx_msg void OnSampleDeleteCuePoint();
	afx_msg void OnTimelineFormatSeconds() { SetTimelineFormat(TimelineFormat::Seconds); }
	afx_msg void OnTimelineFormatSamples() { SetTimelineFormat(TimelineFormat::Samples); }
	afx_msg void OnTimelineFormatSamplesPow2() { SetTimelineFormat(TimelineFormat::SamplesPow2); }
	void SetTimelineFormat(TimelineFormat fmt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL OnGestureZoom(CPoint ptCenter, long lDelta) override
	{
		DoZoom(lDelta / 10, ptCenter);
		return TRUE;
	}

	static bool IsCuePoint(HitTestItem item) { return item >= HitTestItem::CuePointFirst && item <= HitTestItem::CuePointLast; }
	static int CuePointFromItem(HitTestItem item) { return static_cast<int>(item) - static_cast<int>(HitTestItem::CuePointFirst); }
};

DECLARE_FLAGSET(CViewSample::Flags)


OPENMPT_NAMESPACE_END
