#ifndef _VIEW_SAMPLES_H_
#define _VIEW_SAMPLES_H_

#define SMPSTATUS_MOUSEDRAG		0x01
#define SMPSTATUS_KEYDOWN		0x02
#define SMPSTATUS_NCLBTNDOWN	0x04

#define SMP_LEFTBAR_BUTTONS		8

//======================================
class CViewSample: public CModScrollView
//======================================
{
protected:
	CImageList m_bmpEnvBar;
	CRect m_rcClient;
	SIZE m_sizeTotal;
	SAMPLEINDEX m_nSample;
	UINT m_nZoom, m_nScrollPos, m_nScrollFactor, m_nBtnMouseOver;
	DWORD m_dwStatus, m_dwBeginSel, m_dwEndSel, m_dwBeginDrag, m_dwEndDrag;
	DWORD m_dwMenuParam;
	DWORD m_NcButtonState[SMP_LEFTBAR_BUTTONS];
	DWORD m_dwNotifyPos[MAX_CHANNELS];
	int m_nGridSegments;

	bool m_bDrawingEnabled;	// sample drawing mode enabled?
	CPoint m_lastDrawPoint;	// for drawing horizontal lines

public:
	CViewSample();
	DECLARE_SERIAL(CViewSample)

protected:
	void UpdateScrollSize() {UpdateScrollSize(m_nZoom);}
	void UpdateScrollSize(const UINT nZoomOld);
	BOOL SetCurrentSample(SAMPLEINDEX nSmp);
	BOOL SetZoom(UINT nZoom);
	LONG SampleToScreen(LONG n) const;
	DWORD ScreenToSample(LONG x) const;
	void PlayNote(UINT note, const uint32 nStartPos = uint32_max); //rewbs.customKeys
	void InvalidateSample();
	void SetCurSel(DWORD nBegin, DWORD nEnd);
	void ScrollToPosition(int x);
	void DrawPositionMarks(HDC hdc);
	void DrawSampleData1(HDC hdc, int ymed, int cx, int cy, int len, int uFlags, PVOID pSampleData);
	void DrawSampleData2(HDC hdc, int ymed, int cx, int cy, int len, int uFlags, PVOID pSampleData);
	void DrawNcButton(CDC *pDC, UINT nBtn);
	BOOL GetNcButtonRect(UINT nBtn, LPRECT lpRect);
	void UpdateNcButtonState();

	// Sets sample data on sample draw.
	template<class T, class uT>
	void SetSampleData(void* pSample, const CPoint& point, const DWORD old);

	// Sets initial draw point on sample draw.
	template<class T, class uT>
	void SetInitialDrawPoint(void* pSample, const CPoint& point);

	// Returns sample value corresponding given point in the sample view.
	template<class T, class uT>
	T GetSampleValueFromPoint(const CPoint& point);

	// Returns auto-zoom level compared to other zoom levels.
	// If auto-zoom gives bigger zoom than zoom level N but smaller than zoom level N-1,
	// return value is N. If zoom is bigger than the biggest zoom, returns MIN_ZOOM + 1 and
	// if smaller than the smallest zoom, returns value >= MAX_ZOOM + 1.
	UINT GetAutoZoomLevel(const MODSAMPLE& smp);

	UINT ScrollPosToSamplePos() const {return ScrollPosToSamplePos(m_nZoom);}
	UINT ScrollPosToSamplePos(UINT nZoom) const {return (nZoom > 0) ? (m_nScrollPos << (nZoom - 1)) : 0;}

	void AdjustLoopPoints(UINT &loopStart, UINT &loopEnd, UINT length) const;

public:
	//{{AFX_VIRTUAL(CViewSample)
	virtual void OnDraw(CDC *);
	virtual void OnInitialUpdate();
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll=TRUE);
	virtual BOOL OnDragonDrop(BOOL, LPDRAGONDROP);
	virtual LRESULT OnPlayerNotify(MPTNOTIFICATION *);
	virtual BOOL PreTranslateMessage(MSG *pMsg); //rewbs.customKeys
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewSample)
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
#if _MFC_VER > 0x0710
	afx_msg LRESULT OnNcHitTest(CPoint point);
#else
	afx_msg UINT OnNcHitTest(CPoint point);
#endif 
	afx_msg void OnNcPaint();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
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
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditDelete();
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditUndo();
	afx_msg void OnSetLoop();
	afx_msg void OnSetSustainLoop();
	afx_msg void On8BitConvert();
	afx_msg void OnMonoConvert();
	afx_msg void OnSampleTrim();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnZoomOnSel();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnSetLoopStart();
	afx_msg void OnSetLoopEnd();
	afx_msg void OnSetSustainStart();
	afx_msg void OnSetSustainEnd();
	afx_msg void OnZoomUp();
	afx_msg void OnZoomDown();
	afx_msg void OnDrawingToggle();
	afx_msg void OnAddSilence();
	afx_msg void OnChangeGridSize();
	afx_msg void OnQuickFade() { PostCtrlMessage(IDC_SAMPLE_QUICKFADE); };
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif

