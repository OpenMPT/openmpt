#ifndef _VIEW_SAMPLES_H_
#define _VIEW_SAMPLES_H_

#define SMPSTATUS_MOUSEDRAG		0x01
#define SMPSTATUS_KEYDOWN		0x02
#define SMPSTATUS_NCLBTNDOWN	0x04

#define SMP_LEFTBAR_BUTTONS		3

//======================================
class CViewSample: public CModScrollView
//======================================
{
protected:
	CImageList m_bmpEnvBar;
	CRect m_rcClient;
	SIZE m_sizeTotal;
	UINT m_nSample, m_nZoom, m_nScrollPos, m_nScrollFactor, m_nBtnMouseOver;
	DWORD m_dwStatus, m_dwBeginSel, m_dwEndSel, m_dwBeginDrag, m_dwEndDrag;
	DWORD m_dwMenuParam;
	DWORD m_NcButtonState[SMP_LEFTBAR_BUTTONS];
	DWORD m_dwNotifyPos[MAX_CHANNELS];

public:
	CViewSample();
	DECLARE_SERIAL(CViewSample)

public:
	void UpdateScrollSize();
	BOOL SetCurrentSample(UINT nSmp);
	BOOL SetZoom(UINT nZoom);
	LONG SampleToScreen(LONG n) const;
	DWORD ScreenToSample(LONG x) const;
	void InvalidateSample();
	void SetCurSel(DWORD nBegin, DWORD nEnd);
	void ScrollToPosition(int x);
	void DrawPositionMarks(HDC hdc);
	void DrawSampleData1(HDC hdc, int ymed, int cx, int cy, int len, int uFlags, PVOID pSampleData);
	void DrawSampleData2(HDC hdc, int ymed, int cx, int cy, int len, int uFlags, PVOID pSampleData);
	void DrawNcButton(CDC *pDC, UINT nBtn);
	BOOL GetNcButtonRect(UINT nBtn, LPRECT lpRect);
	void UpdateNcButtonState();

public:
	//{{AFX_VIRTUAL(CViewSample)
	virtual void OnDraw(CDC *);
	virtual void OnInitialUpdate();
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll=TRUE);
	virtual BOOL OnDragonDrop(BOOL, LPDRAGONDROP);
	virtual LRESULT OnPlayerNotify(MPTNOTIFICATION *);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewSample)
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg UINT OnNcHitTest(CPoint point);
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
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif

