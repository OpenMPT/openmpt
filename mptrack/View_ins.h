/*
 * view_ins.h
 * ----------
 * Purpose: Instrument tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

#define INSSTATUS_DRAGGING		0x01
#define INSSTATUS_NCLBTNDOWN	0x02
#define INSSTATUS_SPLITCURSOR	0x04

// Non-Client toolbar buttons
#define ENV_LEFTBAR_BUTTONS		22

enum DragPoints
{
	ENV_DRAGLOOPSTART		= (MAX_ENVPOINTS + 1),
	ENV_DRAGLOOPEND			= (MAX_ENVPOINTS + 2),
	ENV_DRAGSUSTAINSTART	= (MAX_ENVPOINTS + 3),
	ENV_DRAGSUSTAINEND		= (MAX_ENVPOINTS + 4),
	ENV_DRAGPREVIOUS		= (MAX_ENVPOINTS + 5),
	ENV_DRAGNEXT			= (MAX_ENVPOINTS + 6),
};

class CViewInstrument: public CModScrollView
{
protected:
	CImageList m_bmpEnvBar;
	CPoint m_ptMenu;
	CRect m_rcClient, m_rcOldClient;

	CBitmap m_bmpGrid;
	CBitmap m_bmpMemMain;
	HBITMAP m_pbmpOldGrid = nullptr;
	HBITMAP oldBitmap = nullptr;

	EnvelopeType m_nEnv = ENV_VOLUME;
	uint32 m_nDragItem = 1, m_nBtnMouseOver = 0xFFFF;
	DWORD m_dwStatus = 0;
	DWORD m_NcButtonState[ENV_LEFTBAR_BUTTONS];

	INSTRUMENTINDEX m_nInstrument = 1;

	CDC m_dcMemMain;
	CDC m_dcGrid;
	int m_GridScrollPos = -1;
	int m_GridSpeed = -1;

	float m_zoom = 4;
	int m_envPointSize = 4;

	bool m_bGrid = true;
	bool m_bGridForceRedraw = false;
	bool m_mouseMoveModified = false;

	std::bitset<128> m_baPlayingNote;
	CModDoc::NoteToChannelMap m_noteChannel;	// Note -> Preview channel assignment
	std::array<uint32, MAX_CHANNELS> m_dwNotifyPos;

public:
	CViewInstrument();
	DECLARE_SERIAL(CViewInstrument)

protected:
	void PrepareUndo(const char *description);

	////////////////////////
	// Envelope get stuff

	// Flags
	bool EnvGetFlag(const EnvelopeFlags dwFlag) const;
	bool EnvGetLoop() const { return EnvGetFlag(ENV_LOOP); };
	bool EnvGetSustain() const { return EnvGetFlag(ENV_SUSTAIN); };
	bool EnvGetCarry() const { return EnvGetFlag(ENV_CARRY); };

	// Misc.
	uint32 EnvGetTick(int nPoint) const;
	uint32 EnvGetValue(int nPoint) const;
	uint32 EnvGetLastPoint() const;
	uint32 EnvGetNumPoints() const;

	// Get loop points
	uint32 EnvGetLoopStart() const;
	uint32 EnvGetLoopEnd() const;
	uint32 EnvGetSustainStart() const;
	uint32 EnvGetSustainEnd() const;

	// Get envelope status
	bool EnvGetVolEnv() const;
	bool EnvGetPanEnv() const;
	bool EnvGetPitchEnv() const;
	bool EnvGetFilterEnv() const;

	////////////////////////
	// Envelope set stuff

	// Flags
	bool EnvSetFlag(EnvelopeFlags flag, bool enable);
	bool EnvSetLoop(bool enable) {return EnvSetFlag(ENV_LOOP, enable);};
	bool EnvSetSustain(bool enable) {return EnvSetFlag(ENV_SUSTAIN, enable);};
	bool EnvSetCarry(bool enable) {return EnvSetFlag(ENV_CARRY, enable);};

	// Misc.
	bool EnvSetValue(int nPoint, int32 nTick = int32_min, int32 nValue = int32_min, bool moveTail = false);
	bool CanMovePoint(uint32 envPoint, int step);

	// Set loop points
	bool EnvSetLoopStart(int nPoint);
	bool EnvSetLoopEnd(int nPoint);
	bool EnvSetSustainStart(int nPoint);
	bool EnvSetSustainEnd(int nPoint);
	bool EnvToggleReleaseNode(int nPoint);

	// Set envelope status
	bool EnvToggleEnv(EnvelopeType envelope, CSoundFile &sndFile, ModInstrument &ins, bool enable, EnvelopeNode::value_t defaultValue, EnvelopeFlags extraFlags = EnvelopeFlags(0));
	bool EnvSetVolEnv(bool enable);
	bool EnvSetPanEnv(bool enable);
	bool EnvSetPitchEnv(bool enable);
	bool EnvSetFilterEnv(bool enable);

	// Keyboard envelope control
	void EnvKbdSelectPoint(DragPoints point);
	void EnvKbdMovePointLeft(int stepsize);
	void EnvKbdMovePointRight(int stepsize);
	void EnvKbdMovePointVertical(int stepsize);
	void EnvKbdInsertPoint();
	void EnvKbdRemovePoint();
	void EnvKbdSetLoopStart();
	void EnvKbdSetLoopEnd();
	void EnvKbdSetSustainStart();
	void EnvKbdSetSustainEnd();
	void EnvKbdToggleReleaseNode();

	bool IsDragItemEnvPoint() const { return m_nDragItem >= 1 && m_nDragItem <= EnvGetNumPoints(); }

	////////////////////////
	// Misc stuff
	void UpdateScrollSize();
	void SetModified(InstrumentHint hint, bool updateAll);
	BOOL SetCurrentInstrument(INSTRUMENTINDEX nIns, EnvelopeType m_nEnv = ENV_VOLUME);
	ModInstrument *GetInstrumentPtr() const;
	InstrumentEnvelope *GetEnvelopePtr() const;
	bool InsertAtPoint(CPoint pt);
	uint32 EnvInsertPoint(int nTick, int nValue);
	bool EnvRemovePoint(uint32 nPoint);
	uint32 DragItemToEnvPoint() const;
	int TickToScreen(int nTick) const;
	int PointToScreen(int nPoint) const;
	int ScreenToTick(int x) const;
	int ScreenToPoint(int x, int y) const;
	int ValueToScreen(int val) const { return m_rcClient.bottom - 1 - (val * (m_rcClient.bottom - 1)) / 64; }
	int ScreenToValue(int y) const;
	void InvalidateEnvelope() { InvalidateRect(NULL, FALSE); }
	void DrawPositionMarks();
	void DrawNcButton(CDC *pDC, UINT nBtn);
	bool GetNcButtonRect(UINT button, CRect &rect) const;
	UINT GetNcButtonAtPoint(CPoint point, CRect *outRect = nullptr) const;
	void UpdateNcButtonState();
	void PlayNote(ModCommand::NOTE note);
	void DrawGrid(CDC *memDC, uint32 speed);
	void UpdateIndicator();
	void UpdateIndicator(int tick, int val);
	CString EnvValueToString(int tick, int val) const;

	void OnEnvZoomIn() { EnvSetZoom(m_zoom + 1); };
	void OnEnvZoomOut() { EnvSetZoom(m_zoom - 1); };
	void EnvSetZoom(float fNewZoom);

public:
	//{{AFX_VIRTUAL(CViewInstrument)
	void OnDraw(CDC *) override;
	void OnInitialUpdate() override;
	void UpdateView(UpdateHint hint, CObject *pObj = nullptr) override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	BOOL OnDragonDrop(BOOL, const DRAGONDROP *) override;
	LRESULT OnModViewMsg(WPARAM, LPARAM) override;
	LRESULT OnPlayerNotify(Notification *) override;
	HRESULT get_accName(VARIANT varChild, BSTR *pszName) override;
	INT_PTR OnToolHitTest(CPoint point, TOOLINFO *pTI) const override;
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CViewInstrument)
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnDPIChanged(WPARAM = 0, LPARAM = 0);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnNcPaint();
	afx_msg void OnPrevInstrument();
	afx_msg void OnNextInstrument();
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnLButtonDblClk(UINT /*nFlags*/, CPoint point) { InsertAtPoint(point); }
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnMButtonDown(UINT, CPoint);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonDown(UINT, CPoint);
	afx_msg void OnNcLButtonUp(UINT, CPoint);
	afx_msg void OnNcLButtonDblClk(UINT, CPoint);
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
	afx_msg void OnEnvToggleGrid();
	afx_msg void OnEnvLoad();
	afx_msg void OnEnvSave();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSampleMap();
	afx_msg void OnEnvelopeScalePoints();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnXButtonUp(UINT nFlags, UINT nButton, CPoint point);
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateUndo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRedo(CCmdUI *pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	uint8 EnvGetReleaseNode();
};


OPENMPT_NAMESPACE_END
