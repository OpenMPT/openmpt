#ifndef _VIEW_INSTRUMENTS_H_
#define _VIEW_INSTRUMENTS_H_

#define INSSTATUS_DRAGGING		0x01
#define INSSTATUS_NCLBTNDOWN	0x02
#define INSSTATUS_SPLITCURSOR	0x04

// Non-Client toolbar buttons
#define ENV_LEFTBAR_BUTTONS		17

//==========================================
class CViewInstrument: public CModScrollView
//==========================================
{
protected:
	CImageList m_bmpEnvBar;
	POINT m_ptMenu;
	RECT m_rcClient;
	bool m_baPlayingNote[128]; //rewbs.instViewNNA
	INSTRUMENTINDEX m_nInstrument;
	UINT m_nEnv, m_nDragItem, m_nBtnMouseOver, m_nPlayingChannel;
	DWORD m_dwStatus;
	DWORD m_NcButtonState[ENV_LEFTBAR_BUTTONS];
	DWORD m_dwNotifyPos[MAX_CHANNELS];
	//rewbs.envRowGrid
	bool m_bGrid;			  
	bool m_bGridForceRedraw;
	CBitmap *m_pbmpOldGrid;
	CBitmap m_bmpGrid;
	CDC m_dcGrid;
	int m_GridScrollPos;
	int m_GridSpeed;
	CDC m_dcMemMain;
	CBitmap m_bmpMemMain;
	CBitmap* oldBitmap;
	//rewbs.envRowGrid

public:
	CViewInstrument();
	DECLARE_SERIAL(CViewInstrument)

public:
	void UpdateScrollSize();
	BOOL SetCurrentInstrument(INSTRUMENTINDEX nIns, UINT m_nEnv=0);
	INSTRUMENTENVELOPE *GetEnvelopePtr() const;
	UINT EnvGetTick(int nPoint) const;
	UINT EnvGetValue(int nPoint) const;
	UINT EnvGetLastPoint() const;
	UINT EnvGetNumPoints() const;
	UINT EnvGetLoopStart() const;
	UINT EnvGetLoopEnd() const;
	UINT EnvGetSustainStart() const;
	UINT EnvGetSustainEnd() const;
	bool EnvGetLoop() const;
	bool EnvGetSustain() const;
	bool EnvGetCarry() const;
	bool EnvGetVolEnv() const;
	bool EnvGetPanEnv() const;
	bool EnvGetPitchEnv() const;
	bool EnvGetFilterEnv() const;
	bool EnvSetValue(int nPoint, int nTick=-1, int nValue=-1);
	bool EnvSetLoopStart(int nPoint);
	bool EnvSetLoopEnd(int nPoint);
	bool EnvSetSustainStart(int nPoint);
	bool EnvSetSustainEnd(int nPoint);
	bool EnvSetLoop(bool bLoop);
	bool EnvSetSustain(bool bSustain);
	bool EnvSetCarry(bool bCarry);
	bool EnvSetVolEnv(bool bEnable);
	bool EnvSetPanEnv(bool bEnable);
	bool EnvSetPitchEnv(bool bEnable);
	bool EnvSetFilterEnv(bool bEnable);
	UINT EnvInsertPoint();
	bool EnvRemovePoint();
	int TickToScreen(int nTick) const;
	int PointToScreen(int nPoint) const;
	int ScreenToTick(int x) const;
	int QuickScreenToTick(int x, int cachedScrollPos) const;
	int ScreenToPoint(int x, int y) const;
	int ValueToScreen(int val) const { return m_rcClient.bottom - 1 - (val * (m_rcClient.bottom-1)) / 64; }
	int ScreenToValue(int y) const;
	void InvalidateEnvelope() { InvalidateRect(NULL, FALSE); }
	void DrawPositionMarks(HDC hdc);
	void DrawNcButton(CDC *pDC, UINT nBtn);
	BOOL GetNcButtonRect(UINT nBtn, LPRECT lpRect);
	void UpdateNcButtonState();
	void PlayNote(UINT note);				//rewbs.customKeys
	void DrawGrid(CDC *memDC, UINT speed);	//rewbs.envRowGrid

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
#if _MFC_VER > 0x0710
	afx_msg LRESULT OnNcHitTest(CPoint point);
#else
	afx_msg UINT OnNcHitTest(CPoint point);
#endif 
	afx_msg void OnNcPaint();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnMButtonDown(UINT, CPoint);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonDown(UINT, CPoint);
	afx_msg void OnNcLButtonUp(UINT, CPoint);
	afx_msg void OnNcLButtonDblClk(UINT, CPoint);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnvLoopChanged();
	afx_msg void OnEnvSustainChanged();
	afx_msg void OnEnvCarryChanged();
	afx_msg void OnEnvToggleReleasNode();
	afx_msg void OnEnvInsertPoint();
	afx_msg void OnEnvRemovePoint();
	afx_msg void OnSelectVolumeEnv();
	afx_msg void OnSelectPanningEnv();
	afx_msg void OnSelectPitchEnv();
	afx_msg void OnEnvVolChanged();
	afx_msg void OnEnvPanChanged();
	afx_msg void OnEnvPitchChanged();
	afx_msg void OnEnvFilterChanged();
	afx_msg void OnEnvToggleGrid(); //rewbs.envRowGrid
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSampleMap();
	afx_msg void OnEnvelopeScalepoints();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	virtual BOOL PreTranslateMessage(MSG *pMsg); //rewbs.customKeys
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BYTE EnvGetReleaseNode();
	WORD EnvGetReleaseNodeValue();
	WORD EnvGetReleaseNodeTick();
};


#endif

