#ifndef _VIEW_INSTRUMENTS_H_
#define _VIEW_INSTRUMENTS_H_

#define INSSTATUS_DRAGGING		0x01
#define INSSTATUS_NCLBTNDOWN	0x02
#define INSSTATUS_SPLITCURSOR	0x04

// Non-Client toolbar buttons
#define ENV_LEFTBAR_BUTTONS		15

//==========================================
class CViewInstrument: public CModScrollView
//==========================================
{
protected:
	CImageList m_bmpEnvBar;
	POINT m_ptMenu;
	RECT m_rcClient;
	UINT m_nInstrument, m_nEnv, m_nDragItem, m_nBtnMouseOver;
	DWORD m_dwStatus;
	DWORD m_NcButtonState[ENV_LEFTBAR_BUTTONS];
	DWORD m_dwNotifyPos[MAX_CHANNELS];

public:
	CViewInstrument();
	DECLARE_SERIAL(CViewInstrument)

public:
	void UpdateScrollSize();
	BOOL SetCurrentInstrument(UINT nIns, UINT m_nEnv=0);
	UINT EnvGetTick(int nPoint) const;
	UINT EnvGetValue(int nPoint) const;
	UINT EnvGetLastPoint() const;
	UINT EnvGetNumPoints() const;
	UINT EnvGetLoopStart() const;
	UINT EnvGetLoopEnd() const;
	UINT EnvGetSustainStart() const;
	UINT EnvGetSustainEnd() const;
	BOOL EnvGetLoop() const;
	BOOL EnvGetSustain() const;
	BOOL EnvGetCarry() const;
	BOOL EnvGetVolEnv() const;
	BOOL EnvGetPanEnv() const;
	BOOL EnvGetPitchEnv() const;
	BOOL EnvGetFilterEnv() const;
	BOOL EnvSetValue(int nPoint, int nTick=-1, int nValue=-1);
	BOOL EnvSetLoopStart(int nPoint);
	BOOL EnvSetLoopEnd(int nPoint);
	BOOL EnvSetSustainStart(int nPoint);
	BOOL EnvSetSustainEnd(int nPoint);
	BOOL EnvSetLoop(BOOL bLoop);
	BOOL EnvSetSustain(BOOL bSustain);
	BOOL EnvSetCarry(BOOL bCarry);
	BOOL EnvSetVolEnv(BOOL bEnable);
	BOOL EnvSetPanEnv(BOOL bEnable);
	BOOL EnvSetPitchEnv(BOOL bEnable);
	BOOL EnvSetFilterEnv(BOOL bEnable);
	int TickToScreen(int nTick) const;
	int PointToScreen(int nPoint) const;
	int ScreenToTick(int x) const;
	int ScreenToPoint(int x, int y) const;
	int ValueToScreen(int val) const { return m_rcClient.bottom - 1 - (val * (m_rcClient.bottom-1)) / 64; }
	int ScreenToValue(int y) const;
	void InvalidateEnvelope() { InvalidateRect(NULL, FALSE); }
	void DrawPositionMarks(HDC hdc);
	void DrawNcButton(CDC *pDC, UINT nBtn);
	BOOL GetNcButtonRect(UINT nBtn, LPRECT lpRect);
	void UpdateNcButtonState();

public:
	//{{AFX_VIRTUAL(CViewInstrument)
	virtual void OnDraw(CDC *);
	virtual void OnInitialUpdate();
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);
	virtual BOOL OnDragonDrop(BOOL, LPDRAGONDROP);
	virtual LRESULT OnPlayerNotify(MPTNOTIFICATION *);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewInstrument)
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnNcPaint();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonDown(UINT, CPoint);
	afx_msg void OnNcLButtonUp(UINT, CPoint);
	afx_msg void OnNcLButtonDblClk(UINT, CPoint);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnvLoopChanged();
	afx_msg void OnEnvSustainChanged();
	afx_msg void OnEnvCarryChanged();
	afx_msg void OnEnvInsertPoint();
	afx_msg void OnEnvRemovePoint();
	afx_msg void OnSelectVolumeEnv();
	afx_msg void OnSelectPanningEnv();
	afx_msg void OnSelectPitchEnv();
	afx_msg void OnEnvVolChanged();
	afx_msg void OnEnvPanChanged();
	afx_msg void OnEnvPitchChanged();
	afx_msg void OnEnvFilterChanged();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSampleMap();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif

