/*
 * view_pat.h
 * ----------
 * Purpose: Pattern tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "globals.h"
#include "PatternCursor.h"
#include "PatternClipboard.h"
#include "PatternEditorDialogs.h"

class CModDoc;
class CEditCommand;
class CEffectVis;	//rewbs.fxvis
class CPatternGotoDialog;
class CPatternRandomizer;
class COpenGLEditor;

// Drag & Drop info
#define DRAGITEM_VALUEMASK		0x00FFFF
#define DRAGITEM_MASK			0xFF0000
#define DRAGITEM_CHNHEADER		0x010000
#define DRAGITEM_PATTERNHEADER	0x020000
#define DRAGITEM_PLUGNAME		0x040000	//rewbs.patPlugName


// Row Spacing
const ROWINDEX MAX_SPACING = 64; // MAX_PATTERN_ROWS


// Struct for controlling selection clearing. This is used to define which data fields should be cleared.
struct RowMask
{
	bool note;
	bool instrument;
	bool volume;
	bool command;
	bool parameter;

	// Default row mask (all rows are selected)
	RowMask()
	{
		note = instrument = volume = command = parameter = true;
	};

	// Construct mask from list
	RowMask(bool n, bool i, bool v, bool c, bool p)
	{
		note = n;
		instrument = i;
		volume = v;
		command = c;
		parameter = p;
	}

	// Construct mask from column index
	RowMask(const PatternCursor &cursor)
	{
		const PatternCursor::Columns column = cursor.GetColumnType();

		note = (column == PatternCursor::noteColumn);
		instrument = (column == PatternCursor::instrColumn);
		volume = (column == PatternCursor::volumeColumn);
		command = (column == PatternCursor::effectColumn);
		parameter = (column == PatternCursor::paramColumn);
	}

	void Clear()
	{
		note = instrument = volume = command = parameter = false;
	}
};


// Find/Replace data
struct FindReplaceStruct
{
	ModCommand cmdFind, cmdReplace;			// Find/replace notes/instruments/effects
	DWORD dwFindFlags, dwReplaceFlags;		// PATSEARCH_XXX flags (=> PatternEditorDialogs.h)
	CHANNELINDEX nFindMinChn, nFindMaxChn;	// Find in these channels (if PATSEARCH_CHANNEL is set)
	signed char cInstrRelChange;			// relative instrument change (quick'n'dirty fix, this should be implemented in a less cryptic way)
	PatternRect selection;					// Find in this selection (if PATSEARCH_PATSELECTION is set)
};


//////////////////////////////////////////////////////////////////
// Pattern editing class


//=======================================
class CViewPattern: public CModScrollView
//=======================================
{
protected:

	// Pattern status flags
	enum PatternStatus
	{
		psMouseDragSelect		= 0x01,		// Creating a selection using the mouse
		psKeyboardDragSelect	= 0x02,		// Creating a selection using shortcuts
		psFocussed				= 0x04,		// Is the pattern editor focussed
		psFollowSong			= 0x08,		// Does the cursor follow playback
		psRecordingEnabled		= 0x10,		// Recording enabled
		psDragHScroll			= 0x20,		// Some weird dragging stuff (?) - unused
		psDragVScroll			= 0x40,		// Some weird dragging stuff (?)
		psShowVUMeters			= 0x80,		// Display channel VU meters
		psChordPlaying			= 0x100,	// Is a chord playing? (pretty much unused)
		psDragnDropEdit			= 0x200,	// Drag & Drop editing (?)
		psDragnDropping			= 0x400,	// Dragging a selection around
		psShiftSelect			= 0x800,	// User has made at least one selection using Shift-Click since the Shift key has been pressed.
		psCtrlDragSelect		= 0x1000,	// Creating a selection using Ctrl
		psShowPluginNames		= 0x2000,	// Show plugin names in channel headers
		psRowSelection			= 0x4000,	// Selecting a whole pattern row by clicking the row numbers
		psChannelSelection		= 0x8000,	// Double-clicked pattern to select a whole channel

		// All possible drag flags, to test if user is dragging a selection or a scrollbar
		psDragActive			= psDragVScroll | psDragHScroll | psMouseDragSelect | psRowSelection | psChannelSelection,
	};

	struct ModCommandPos
	{
		PATTERNINDEX nPat;
		ROWINDEX nRow;
		CHANNELINDEX nChn;
	};

	CFastBitmap m_Dib;
	CEditCommand *m_pEditWnd;
	CPatternGotoDialog *m_pGotoWnd;
	SIZE m_szHeader, m_szCell;
	PATTERNINDEX m_nPattern;
	UINT m_nMidRow, m_nSpacing, m_nAccelChar, m_nLastPlayedRow, m_nLastPlayedOrder;
	PATTERNINDEX m_nPlayPat;
	ROWINDEX m_nPlayRow;

	int m_nXScroll, m_nYScroll;
	PatternCursor::Columns m_nDetailLevel;		// Visible Columns

	// Cursor and selection positions
	PatternCursor m_Cursor;					// Current cursor position in pattern.
	PatternCursor m_StartSel, m_DragPos;	// Point where selection was started.
	PatternCursor m_MenuCursor;				// Position at which context menu was opened.
	PatternRect m_Selection;				// Upper-left / Lower-right corners of selection.

	// Drag&Drop
	DWORD m_nDragItem;	// Currently dragged item
	DWORD m_nDropItem;	// Currently hovered item during dragondrop
	bool m_bDragging, m_bInItemRect, m_bShiftDragging;
	RECT m_rcDragItem, m_rcDropItem;

	bool m_bContinueSearch, m_bWholePatternFitsOnScreen;
	DWORD m_dwStatus;
	WORD ChnVUMeters[MAX_BASECHANNELS];
	WORD OldVUMeters[MAX_BASECHANNELS];
	UINT m_nFoundInstrument;
	DWORD m_dwLastNoteEntryTime; //rewbs.customkeys
	UINT m_nLastPlayedChannel; //rewbs.customkeys
	bool m_bLastNoteEntryBlocked;

	ModCommand m_PCNoteEditMemory;			// PC Note edit memory
	static ModCommand m_cmdOld;				// Quick cursor copy/paste data
	static FindReplaceStruct m_findReplace;	// Find/replace data

	// Internal pattern clipboard
	static PatternClipboard patternClipboard;

// -> CODE#0012
// -> DESC="midi keyboard split"
	BYTE activeNoteChannel[NOTE_MAX + 1];
	BYTE splitActiveNoteChannel[NOTE_MAX + 1];
	vector<ModCommand::NOTE> octaveKeyMemory;
	int oldrow, oldchn, oldsplitchn;
// -! NEW_FEATURE#0012

	QuickChannelProperties quickChannelProperties;

public:
	CEffectVis *m_pEffectVis;	//rewbs.fxVis

	CViewPattern();
	DECLARE_SERIAL(CViewPattern)

public:

	const CSoundFile *GetSoundFile() const { return (GetDocument() != nullptr) ? GetDocument()->GetSoundFile() : nullptr; };
	CSoundFile *GetSoundFile() { return (GetDocument() != nullptr) ? GetDocument()->GetSoundFile() : nullptr; };

	void SetModified(bool updateAllViews = true);

	static PatternClipboard &GetPatternClipboard() { return patternClipboard; }

	bool UpdateSizes();
	void UpdateScrollSize();
	void UpdateScrollPos();
	void UpdateIndicator();
	void UpdateXInfoText(); //rewbs.xinfo
	void UpdateColors();

	int GetXScrollPos() const { return m_nXScroll; }
	int GetYScrollPos() const { return m_nYScroll; }
	int GetColumnWidth() const { return m_szCell.cx; }
	int GetColumnHeight() const { return m_szCell.cy; }

	PATTERNINDEX GetCurrentPattern() const { return m_nPattern; }
	ROWINDEX GetCurrentRow() const { return m_Cursor.GetRow(); }
	CHANNELINDEX GetCurrentChannel() const { return m_Cursor.GetChannel(); }
	// Get ModCommand at the pattern cursor position.
	ModCommand &GetCursorCommand() { return GetModCommand(m_Cursor); };

	UINT GetColumnOffset(PatternCursor::Columns column) const;
	POINT GetPointFromPosition(PatternCursor cursor);
	PatternCursor GetPositionFromPoint(POINT pt);

	DWORD GetDragItem(CPoint point, LPRECT lpRect);

	ROWINDEX GetRowsPerBeat() const;
	ROWINDEX GetRowsPerMeasure() const;

	// Invalidate functions (for redrawing areas of the pattern)
	void InvalidatePattern(bool invalidateHeader = false);
	void InvalidateRow(ROWINDEX n = ROWINDEX_INVALID);
	void InvalidateArea(const PatternRect &rect) { InvalidateArea(rect.GetUpperLeft(), rect.GetLowerRight()); };
	void InvalidateArea(PatternCursor begin, PatternCursor end);
	void InvalidateSelection() { InvalidateArea(m_Selection); }
	void InvalidateCell(PatternCursor cursor);
	void InvalidateChannelsHeaders();

	// Selection functions
	void SetCurSel(const PatternRect &rect) { SetCurSel(rect.GetUpperLeft(), rect.GetLowerRight()); };
	void SetCurSel(const PatternCursor &point) { SetCurSel(point, point); };
	void SetCurSel(const PatternCursor &beginSel, const PatternCursor &endSel);
	void SetSelToCursor() { SetCurSel(m_Cursor); };

	bool SetCurrentPattern(PATTERNINDEX pat, ROWINDEX row = ROWINDEX_INVALID);
	bool SetCurrentRow(ROWINDEX row, bool wrap = false, bool updateHorizontalScrollbar = true);
	bool SetCurrentColumn(const PatternCursor &cursor) { return SetCurrentColumn(cursor.GetChannel(), cursor.GetColumnType()); };
	bool SetCurrentColumn(CHANNELINDEX channel, PatternCursor::Columns column = PatternCursor::firstColumn);
	// This should be used instead of consecutive calls to SetCurrentRow() then SetCurrentColumn()
	bool SetCursorPosition(const PatternCursor &cursor, bool wrap = false);
	bool DragToSel(const PatternCursor &cursor, bool scrollHorizontal, bool scrollVertical, bool noMove = false);
	bool SetPlayCursor(PATTERNINDEX nPat, ROWINDEX nRow);
	bool UpdateScrollbarPositions(bool updateHorizontalScrollbar = true);
	BYTE EnterNote(UINT nNote, UINT nIns=0, BOOL bCheck=FALSE, int vol=-1, BOOL bMultiCh=FALSE);
	bool ShowEditWindow();
	UINT GetCurrentInstrument() const;
	void SelectBeatOrMeasure(bool selectBeat);
	// Move pattern cursor to left or right, respecting invisible columns.
	void MoveCursor(bool moveRight);

	bool TransposeSelection(int transp);
	bool DataEntry(bool up, bool coarse);

	bool PrepareUndo(const PatternRect &selection) { return PrepareUndo(selection.GetUpperLeft(), selection.GetLowerRight()); };
	bool PrepareUndo(const PatternCursor &beginSel, const PatternCursor &endSel);

	void DeleteRows(CHANNELINDEX colmin, CHANNELINDEX colmax, ROWINDEX nrows);
	void OnDropSelection();
	void ProcessChar(UINT nChar, UINT nFlags);

public:
	void DrawPatternData(HDC hdc, const CSoundFile *pSndFile, PATTERNINDEX nPattern, bool selEnable, bool isPlaying, ROWINDEX startRow, ROWINDEX numRows, CHANNELINDEX startChan, CRect &rcClient, int *pypaint);
	void DrawLetter(int x, int y, char letter, int sizex=10, int ofsx=0);
	void DrawNote(int x, int y, UINT note, CTuning* pTuning = NULL);
	void DrawInstrument(int x, int y, UINT instr);
	void DrawVolumeCommand(int x, int y, const ModCommand &mc, bool drawDefaultVolume);
	void DrawChannelVUMeter(HDC hdc, int x, int y, UINT nChn);
	void UpdateAllVUMeters(MPTNOTIFICATION *pnotify);
	void DrawDragSel(HDC hdc);
	void OnDrawDragSel();
	// True if default volume should be drawn for a given cell.
	static bool DrawDefaultVolume(const ModCommand *m) { return (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_SHOWDEFAULTVOLUME) && m->volcmd == VOLCMD_NONE && m->command != CMD_VOLUME && m->instr != 0 && m->IsNote(); }

	void CursorJump(DWORD distance, bool upwards, bool snap);

	void TempEnterNote(int n, bool oldStyle = false, int vol = -1, bool fromMidi = false);
	void TempStopNote(int note, bool fromMidi = false, const bool bChordMode = false);
	void TempEnterChord(int n);
	void TempStopChord(int note) {TempStopNote(note, false, true);}
	void TempEnterIns(int val);
	void TempEnterOctave(int val);
	void TempStopOctave(int val);
	void TempEnterVol(int v);
	void TempEnterFX(int c, int v = -1);
	void TempEnterFXparam(int v);
	void EnterAftertouch(int note, int atValue);

	void QuantizeRow(PATTERNINDEX &pat, ROWINDEX &row) const;
	PATTERNINDEX GetNextPattern() const;

	void SetSpacing(int n);
	void OnClearField(const RowMask &mask, bool step, bool ITStyle = false);
	void InsertRows(CHANNELINDEX colmin, CHANNELINDEX colmax);
	void SetSelectionInstrument(const INSTRUMENTINDEX nIns);

	void FindInstrument();

	void TogglePluginEditor(int chan); //rewbs.patPlugName

	void ExecutePaste(PatternClipboard::PasteModes mode);

	// Reset all channel variables
	void ResetChannel(CHANNELINDEX chn);

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

protected:
	//{{AFX_MSG(CViewPattern)
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT, CPoint);
	afx_msg void OnLButtonUp(UINT, CPoint);
	afx_msg void OnLButtonDown(UINT, CPoint);
	afx_msg void OnLButtonDblClk(UINT, CPoint);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();

	afx_msg void OnEditPaste() { ExecutePaste(PatternClipboard::pmOverwrite); };
	afx_msg void OnEditMixPaste() { ExecutePaste(PatternClipboard::pmMixPaste); };
	afx_msg void OnEditMixPasteITStyle() { ExecutePaste(PatternClipboard::pmMixPasteIT); };
	afx_msg void OnEditPasteFlood() { ExecutePaste(PatternClipboard::pmPasteFlood); };
	afx_msg void OnEditPushForwardPaste() { ExecutePaste(PatternClipboard::pmPushForward); };

	afx_msg void OnClearSelection(bool ITStyle = false, RowMask sb = RowMask()); //rewbs.customKeys
	afx_msg void OnGrowSelection();   //rewbs.customKeys
	afx_msg void OnShrinkSelection(); //rewbs.customKeys
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditSelectColumn();
	afx_msg void OnSelectCurrentColumn();
	afx_msg void OnEditFind();
	afx_msg void OnEditGoto();
	afx_msg void OnEditFindNext();
	afx_msg void OnEditUndo();
	afx_msg void OnChannelReset();
	afx_msg void OnMuteFromClick(); //rewbs.customKeys
	afx_msg void OnSoloFromClick(); //rewbs.customKeys
	afx_msg void OnTogglePendingMuteFromClick(); //rewbs.customKeys
	afx_msg void OnPendingSoloChnFromClick();
	afx_msg void OnPendingUnmuteAllChnFromClick();
	afx_msg void OnSoloChannel(bool current); //rewbs.customKeys
	afx_msg void OnMuteChannel(bool current); //rewbs.customKeys
	afx_msg void OnUnmuteAll();
	afx_msg void OnRecordSelect();
// -> CODE#0012
// -> DESC="midi keyboard split"
	afx_msg void OnSplitRecordSelect();
// -! NEW_FEATURE#0012
	afx_msg void OnDeleteRows();
	afx_msg void OnDeleteRowsEx();
	afx_msg void OnInsertRows();
	afx_msg void OnPatternStep();
	afx_msg void OnSwitchToOrderList();
	afx_msg void OnPrevOrder();
	afx_msg void OnNextOrder();
	afx_msg void OnPrevInstrument() { PostCtrlMessage(CTRLMSG_PAT_PREVINSTRUMENT); }
	afx_msg void OnNextInstrument() { PostCtrlMessage(CTRLMSG_PAT_NEXTINSTRUMENT); }
//rewbs.customKeys - now implemented at ModDoc level
/*	afx_msg void OnPatternRestart() {}
	afx_msg void OnPatternPlay()	{}
	afx_msg void OnPatternPlayNoLoop()	{} */
//end rewbs.customKeys
	afx_msg void OnPatternRecord()	{ PostCtrlMessage(CTRLMSG_SETRECORD, -1); }
	afx_msg void OnInterpolateVolume() { Interpolate(PatternCursor::volumeColumn); }
	afx_msg void OnInterpolateEffect() { Interpolate(PatternCursor::effectColumn); }
	afx_msg void OnInterpolateNote() { Interpolate(PatternCursor::noteColumn); }
	afx_msg void OnVisualizeEffect();		//rewbs.fxvis
	afx_msg void OnTransposeUp() { TransposeSelection(1); }
	afx_msg void OnTransposeDown() { TransposeSelection(-1); }
	afx_msg void OnTransposeOctUp() { TransposeSelection(12); }
	afx_msg void OnTransposeOctDown() { TransposeSelection(-12); }
	afx_msg void OnTransposeCustom();
	afx_msg void OnSetSelInstrument();
	afx_msg void OnAddChannelFront() { AddChannelBefore(m_MenuCursor.GetChannel()); }
	afx_msg void OnAddChannelAfter() { AddChannelBefore(m_MenuCursor.GetChannel() + 1); };
	afx_msg void OnDuplicateChannel();
	afx_msg void OnRemoveChannel();
	afx_msg void OnRemoveChannelDialog();
	afx_msg void OnPatternProperties();
	afx_msg void OnCursorCopy();
	afx_msg void OnCursorPaste();
	afx_msg void OnPatternAmplify();
	afx_msg void OnUpdateUndo(CCmdUI *pCmdUI);
	afx_msg void OnSelectPlugin(UINT nID);  //rewbs.patPlugName
	afx_msg LRESULT OnUpdatePosition(WPARAM nOrd, LPARAM nRow);
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	afx_msg LRESULT OnRecordPlugParamChange(WPARAM, LPARAM);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys
	afx_msg void OnClearSelectionFromMenu();
	afx_msg void OnSelectInstrument(UINT nid);
	afx_msg void OnSelectPCNoteParam(UINT nid);
	afx_msg void OnRunScript();
	afx_msg void OnShowTimeAtRow();
	afx_msg void OnTogglePCNotePluginEditor();
	afx_msg void OnSetQuantize();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


public:
	afx_msg void OnInitMenu(CMenu* pMenu);

private:

	// Copy&Paste
	bool CopyPattern(PATTERNINDEX nPattern, const PatternRect &selection);
	bool PastePattern(PATTERNINDEX nPattern, const PatternCursor &pastePos, PatternClipboard::PasteModes mode);

	void SetSplitKeyboardSettings();
	bool HandleSplit(ModCommand *p, int note);
	bool IsNoteSplit(int note) const;

	CHANNELINDEX FindGroupRecordChannel(BYTE recordGroup, bool forceFreeChannel, CHANNELINDEX startChannel = 0) const;

	bool BuildChannelControlCtxMenu(HMENU hMenu) const;
	bool BuildPluginCtxMenu(HMENU hMenu, UINT nChn, CSoundFile *pSndFile) const;
	bool BuildRecordCtxMenu(HMENU hMenu, CInputHandler *ih, UINT nChn, CModDoc *pModDoc) const;
	bool BuildSoloMuteCtxMenu(HMENU hMenu, CInputHandler *ih, UINT nChn, CSoundFile *pSndFile) const;
	bool BuildRowInsDelCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildMiscCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildSelectionCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildGrowShrinkCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildInterpolationCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildInterpolationCtxMenu(HMENU hMenu, PatternCursor::Columns colType, CString label, UINT command) const;
	bool BuildEditCtxMenu(HMENU hMenu, CInputHandler *ih,  CModDoc* pModDoc) const;
	bool BuildVisFXCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildRandomCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildTransposeCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildSetInstCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildAmplifyCtxMenu(HMENU hMenu, CInputHandler *ih) const;
	bool BuildPCNoteCtxMenu(HMENU hMenu, CInputHandler *ih) const;

	// Returns an ordered list of all channels in which a given column type is selected.
	CHANNELINDEX ListChansWhereColSelected(PatternCursor::Columns colType, vector<CHANNELINDEX> &chans) const;
	// Check if a column type is selected on any channel in the current selection.
	bool IsColumnSelected(PatternCursor::Columns colType) const;

	bool IsInterpolationPossible(PatternCursor::Columns colType) const;
	bool IsInterpolationPossible(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX chan, PatternCursor::Columns colType) const;
	void Interpolate(PatternCursor::Columns type);

	// Return true if recording live (i.e. editing while following playback).
	bool IsLiveRecord() const
	{
		const CMainFrame *mainFrm = CMainFrame::GetMainFrame();
		const CSoundFile *sndFile = GetSoundFile();
		if(mainFrm == nullptr || sndFile == nullptr)
		{
			return false;
		}
		//           (following song)      &&       (following in correct document)           &&    (playback is on)
		return (m_dwStatus & psFollowSong) && mainFrm->GetFollowSong(GetDocument()) == m_hWnd && !sndFile->IsPaused();
	};

	// If given edit positions are valid, sets them to iRow and iPat.
	// If not valid, set edit cursor position.
	void SetEditPos(const CSoundFile &rSndFile, 
					ROWINDEX &iRow, PATTERNINDEX &iPat,
					const ROWINDEX iRowCandidate, const PATTERNINDEX iPatCandidate) const;

	// Returns edit position.
	ModCommandPos GetEditPos(CSoundFile &rSf, const bool bLiveRecord) const;

	// Returns pointer to modcommand at given position.
	// If the position is not valid, a pointer to a dummy command is returned.
	ModCommand &GetModCommand(PatternCursor cursor);
	ModCommand &GetModCommand(CSoundFile &sndFile, const ModCommandPos &pos);

	// Returns true if pattern editing is enabled.
	bool IsEditingEnabled() const { return ((m_dwStatus & psRecordingEnabled) != 0); }

	// Like IsEditingEnabled(), but shows some notification when editing is not enabled.
	bool IsEditingEnabled_bmsg();

	// Play one pattern row and stop ("step mode")
	void PatternStep(bool autoStep);

	// Add a channel.
	void AddChannelBefore(CHANNELINDEX nBefore);

public:
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
private:

	void TogglePendingMute(CHANNELINDEX nChn);
	void PendingSoloChn(CHANNELINDEX nChn);
	void PendingUnmuteAllChn();

public:
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};
