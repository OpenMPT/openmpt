#ifndef _VIEW_PATTERNS_H_
#define _VIEW_PATTERNS_H_

class CModDoc;
class CEditCommand;
class CEffectVis;	//rewbs.fxvis

// Drag & Drop info
#define DRAGITEM_MASK			0xFF0000
#define DRAGITEM_CHNHEADER		0x010000
#define DRAGITEM_PATTERNHEADER	0x020000
#define DRAGITEM_PLUGNAME		0x040000

#define PATSTATUS_MOUSEDRAGSEL			0x01
#define PATSTATUS_KEYDRAGSEL			0x02
#define PATSTATUS_FOCUS					0x04
#define PATSTATUS_FOLLOWSONG			0x08
#define PATSTATUS_RECORD				0x10
#define PATSTATUS_DRAGHSCROLL			0x20
#define PATSTATUS_DRAGVSCROLL			0x40
#define PATSTATUS_VUMETERS				0x80
#define PATSTATUS_CHORDPLAYING			0x100
#define PATSTATUS_DRAGNDROPEDIT			0x200
#define PATSTATUS_DRAGNDROPPING			0x400
#define PATSTATUS_MIDISPACINGPENDING	0x800
#define PATSTATUS_CTRLDRAGSEL			0x1000
#define PATSTATUS_PLUGNAMESINHEADERS		0x2000


//////////////////////////////////////////////////////////////////
// Pattern editing class


//=======================================
class CViewPattern: public CModScrollView
//=======================================
{
public:
	// Find/Replace
	static MODCOMMAND m_cmdFind, m_cmdReplace, m_cmdOld;
	static DWORD m_dwFindFlags, m_dwReplaceFlags;
	static UINT m_nFindMinChn, m_nFindMaxChn;

protected:
	CFastBitmap m_Dib;
	CEditCommand *m_pEditWnd;
	SIZE m_szHeader, m_szCell;
	UINT m_nPattern, m_nRow, m_nMidRow, m_nPlayPat, m_nPlayRow, m_nSpacing, m_nAccelChar;
	int m_nXScroll, m_nYScroll;
	DWORD m_nDragItem, m_nMenuParam, m_nDetailLevel;
	BOOL m_bDragging, m_bInItemRect, m_bRecord, m_bContinueSearch, m_bWholePatternFitsOnScreen;
	RECT m_rcDragItem;
	DWORD m_dwStatus, m_dwCursor;
	DWORD m_dwBeginSel, m_dwEndSel, m_dwStartSel, m_dwDragPos;
	WORD ChnVUMeters[MAX_BASECHANNELS];
	WORD OldVUMeters[MAX_BASECHANNELS];
	CListBox *ChnEffectList[MAX_BASECHANNELS];
	BYTE MultiRecordMask[(MAX_CHANNELS+7)/8];
	UINT m_nFoundInstrument;
	UINT m_nMenuOnChan;
	
public:
	CEffectVis *m_pEffectVis;	//rewbs.fxVis

	CViewPattern();
	DECLARE_SERIAL(CViewPattern)

public:

	BOOL UpdateSizes();
	void UpdateScrollSize();
	void UpdateScrollPos();
	void UpdateIndicator();
	void UpdateColors();
	int GetXScrollPos() const { return m_nXScroll; }
	int GetYScrollPos() const { return m_nYScroll; }
	int GetColumnWidth() const { return m_szCell.cx; }
	int GetColumnHeight() const { return m_szCell.cy; }
	UINT GetCurrentPattern() const { return m_nPattern; }
	UINT GetCurrentRow() const { return m_nRow; }
	UINT GetCurrentColumn() const { return m_dwCursor; }
	UINT GetCurrentChannel() const { return (m_dwCursor >> 3); }
	UINT GetColumnOffset(DWORD dwPos) const;
	POINT GetPointFromPosition(DWORD dwPos);
	DWORD GetPositionFromPoint(POINT pt);
	DWORD GetDragItem(CPoint point, LPRECT lpRect);
	void InvalidatePattern(BOOL bHdr=FALSE);
	void InvalidateRow(int n=-1);
	void InvalidateArea(DWORD dwBegin, DWORD dwEnd);
	void InvalidateSelection() { InvalidateArea(m_dwBeginSel, m_dwEndSel); }
	void InvalidateChannelsHeaders();
	void SetCurSel(DWORD dwBegin, DWORD dwEnd);
	BOOL SetCurrentPattern(UINT npat, int nrow=-1);
	BOOL SetCurrentRow(UINT nrow, BOOL bWrap=FALSE);
	BOOL SetCurrentColumn(UINT ncol);
	BOOL DragToSel(DWORD dwPos, BOOL bScroll, BOOL bNoMove=FALSE);
	BOOL SetPlayCursor(UINT nPat, UINT nRow);
	BOOL EnterNote(UINT nNote, UINT nIns=0, BOOL bCheck=FALSE, int vol=-1, BOOL bMultiCh=FALSE);
	BOOL ShowEditWindow();
	UINT GetCurrentInstrument() const;
	BOOL TransposeSelection(int transp);
	BOOL PrepareUndo(DWORD dwBegin, DWORD dwEnd);
	void DeleteRows(UINT colmin, UINT colmax, UINT nrows);
	void OnDropSelection();
	void ProcessChar(UINT nChar, UINT nFlags);
	BOOL CheckCustomKeys(UINT nChar, DWORD dwFlags);	//soon to be removed
	
public:
	void DrawPatternData(HDC, CSoundFile *, UINT, BOOL, BOOL, UINT, UINT, UINT, CRect&, int *);
	void DrawLetter(int x, int y, char letter, int sizex=10, int ofsx=0);
	void DrawNote(int x, int y, UINT note);
	void DrawInstrument(int x, int y, UINT instr);
	void DrawVolumeCommand(int x, int y, UINT volcmd, UINT vol);
	void DrawChannelVUMeter(HDC hdc, int x, int y, UINT nChn);
	void UpdateAllVUMeters(MPTNOTIFICATION *pnotify);
	void DrawDragSel(HDC hdc);
	void OnDrawDragSel();
	
	BOOL ExecuteCommand(CommandID command);
	void CursorJump(DWORD distance, bool direction, bool snap);
	void TempEnterNote(int n);
	void TempStopNote(int note);
	void TempEnterChord(int n);
	void TempStopChord(int note);
	void TempEnterIns(int val);
	void TempEnterOctave(int val);
	void TempEnterVol(int v);
	void TempEnterFX(int v);
	void TempEnterFXparam(int v);
	void SetSpacing(int n);
	void OnClearField(int, bool, bool=false);
	void InsertRows(UINT colmin, UINT colmax);
	void TogglePluginEditor(int chan);

public:
	//{{AFX_VIRTUAL(CViewPattern)
	virtual void OnDraw(CDC *);
	virtual void OnInitialUpdate();
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	virtual void UpdateView(DWORD dwHintMask=0, CObject *pObj=NULL);
	virtual LRESULT OnModViewMsg(WPARAM, LPARAM);
	virtual LRESULT OnPlayerNotify(MPTNOTIFICATION *);
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CViewPattern)




protected:

	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);   
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);  
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonDblClk(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditMixPaste();		//rewbs.mixPaste
	afx_msg void OnClearSelection(bool ITStyle=false);
	afx_msg void OnGrowSelection();
	afx_msg void OnShrinkSelection();
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditSelectColumn();
	afx_msg void OnSelectCurrentColumn();
	afx_msg void OnEditFind();
	afx_msg void OnEditFindNext();
	afx_msg void OnEditUndo();
	afx_msg void OnMuteFromClick();
	afx_msg void OnSoloFromClick();
	afx_msg void OnUnmuteAll();
	afx_msg void OnRecordSelect();
	afx_msg void OnDeleteRows();
	afx_msg void OnDeleteRowsEx();
	afx_msg void OnInsertRows();
	afx_msg void OnPatternStep();
	afx_msg void OnSwitchToOrderList();
	afx_msg void OnPrevOrder();
	afx_msg void OnNextOrder();
	afx_msg void OnPrevInstrument() { PostCtrlMessage(CTRLMSG_PAT_PREVINSTRUMENT); }
	afx_msg void OnNextInstrument() { PostCtrlMessage(CTRLMSG_PAT_NEXTINSTRUMENT); }
	afx_msg void OnPatternRestart() {/* PostCtrlMessage(CTRLMSG_PLAYPATTERN, -1); */}
	afx_msg void OnPatternPlay()	{/* PostCtrlMessage(CTRLMSG_PLAYPATTERN, 0); */}
	afx_msg void OnPatternPlayNoLoop()	{/* PostCtrlMessage(CTRLMSG_PLAYPATTERN, -2); */}
	afx_msg void OnPatternRecord()	{ PostCtrlMessage(CTRLMSG_SETRECORD, -1); }
	afx_msg void OnInterpolateVolume();
	afx_msg void OnInterpolateEffect();
	afx_msg void OnVisualizeEffect();		//rewbs.fxvis
	afx_msg void OnTransposeUp();
	afx_msg void OnTransposeDown();
	afx_msg void OnTransposeOctUp();
	afx_msg void OnTransposeOctDown();
	afx_msg void OnSetSelInstrument();
	afx_msg void OnPatternProperties();
	afx_msg void OnCursorCopy();
	afx_msg void OnCursorPaste();
	afx_msg void OnPatternAmplify();
	afx_msg void OnUpdateUndo(CCmdUI *pCmdUI);
	afx_msg void OnSelectPlugin(UINT nID);
	afx_msg LRESULT OnUpdatePosition(WPARAM nOrd, LPARAM nRow);
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void OnSoloChannel(BOOL current);
	void OnMuteChannel(BOOL current);
public:
	afx_msg void OnInitMenu(CMenu* pMenu);
};



#endif

