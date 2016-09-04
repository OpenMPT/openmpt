/*
 * view_pat.cpp
 * ------------
 * Purpose: Pattern tab, lower panel.
 * Notes  : Welcome to about 7000 lines of, err, very beautiful code.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Childfrm.h"
#include "Moddoc.h"
#include "SampleEditorDialogs.h" // For amplification dialog (which is re-used from sample editor)
#include "Globals.h"
#include "View_pat.h"
#include "Ctrl_pat.h"
#include "PatternFont.h"
#include "PatternFindReplace.h"
#include "PatternFindReplaceDlg.h"

#include "EffectVis.h"
#include "PatternGotoDialog.h"
#include "MIDIMacros.h"
#include "../common/misc_util.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"
#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


// Static initializers
ModCommand CViewPattern::m_cmdOld = ModCommand::Empty();

IMPLEMENT_SERIAL(CViewPattern, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewPattern, CModScrollView)
	//{{AFX_MSG_MAP(CViewPattern)
	ON_WM_ERASEBKGND()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_XBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SYSKEYDOWN()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg)		//rewbs.customKeys
	ON_MESSAGE(WM_MOD_MIDIMSG,		OnMidiMsg)
	ON_MESSAGE(WM_MOD_RECORDPARAM,	OnRecordPlugParamChange)
	ON_COMMAND(ID_EDIT_CUT,			OnEditCut)
	ON_COMMAND(ID_EDIT_COPY,		OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,		OnEditPaste)
	ON_COMMAND(ID_EDIT_MIXPASTE,	OnEditMixPaste)
	ON_COMMAND(ID_EDIT_MIXPASTE_ITSTYLE,OnEditMixPasteITStyle)
	ON_COMMAND(ID_EDIT_PASTEFLOOD,	OnEditPasteFlood)
	ON_COMMAND(ID_EDIT_PUSHFORWARDPASTE,OnEditPushForwardPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL,	OnEditSelectAll)
	ON_COMMAND(ID_EDIT_SELECTCOLUMN,OnEditSelectColumn)
	ON_COMMAND(ID_EDIT_SELECTCOLUMN2,OnSelectCurrentColumn)
	ON_COMMAND(ID_EDIT_FIND,		OnEditFind)
	ON_COMMAND(ID_EDIT_GOTO_MENU,	OnEditGoto)
	ON_COMMAND(ID_EDIT_FINDNEXT,	OnEditFindNext)
	ON_COMMAND(ID_EDIT_RECSELECT,	OnRecordSelect)
	ON_COMMAND(ID_EDIT_SPLITRECSELECT,	OnSplitRecordSelect)
	ON_COMMAND(ID_EDIT_SPLITKEYBOARDSETTINGS,	SetSplitKeyboardSettings)
	ON_COMMAND(ID_EDIT_UNDO,		OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO,		OnEditRedo)
	ON_COMMAND(ID_PATTERN_CHNRESET,	OnChannelReset)
	ON_COMMAND(ID_PATTERN_MUTE,		OnMuteFromClick) //rewbs.customKeys
	ON_COMMAND(ID_PATTERN_SOLO,		OnSoloFromClick) //rewbs.customKeys
	ON_COMMAND(ID_PATTERN_TRANSITIONMUTE, OnTogglePendingMuteFromClick)
	ON_COMMAND(ID_PATTERN_TRANSITIONSOLO, OnPendingSoloChnFromClick)
	ON_COMMAND(ID_PATTERN_TRANSITION_UNMUTEALL, OnPendingUnmuteAllChnFromClick)
	ON_COMMAND(ID_PATTERN_UNMUTEALL,OnUnmuteAll)
	ON_COMMAND(ID_PATTERN_DELETEROW,OnDeleteRows)
	ON_COMMAND(ID_PATTERN_DELETEALLROW,OnDeleteRowsEx)
	ON_COMMAND(ID_PATTERN_INSERTROW,OnInsertRows)
	ON_COMMAND(ID_NEXTINSTRUMENT,	OnNextInstrument)
	ON_COMMAND(ID_PREVINSTRUMENT,	OnPrevInstrument)
	ON_COMMAND(ID_PATTERN_PLAYROW,	OnPatternStep)
	ON_COMMAND(IDC_PATTERN_RECORD,	OnPatternRecord)
	ON_COMMAND(ID_RUN_SCRIPT,					OnRunScript)
	ON_COMMAND(ID_TRANSPOSE_UP,					OnTransposeUp)
	ON_COMMAND(ID_TRANSPOSE_DOWN,				OnTransposeDown)
	ON_COMMAND(ID_TRANSPOSE_OCTUP,				OnTransposeOctUp)
	ON_COMMAND(ID_TRANSPOSE_OCTDOWN,			OnTransposeOctDown)
	ON_COMMAND(ID_TRANSPOSE_CUSTOM,				OnTransposeCustom)
	ON_COMMAND(ID_PATTERN_PROPERTIES,			OnPatternProperties)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_VOLUME,	OnInterpolateVolume)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_EFFECT,	OnInterpolateEffect)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_NOTE,		OnInterpolateNote)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_INSTR,	OnInterpolateInstr)
	ON_COMMAND(ID_PATTERN_VISUALIZE_EFFECT,		OnVisualizeEffect)		//rewbs.fxvis
	ON_COMMAND(ID_GROW_SELECTION,				OnGrowSelection)
	ON_COMMAND(ID_SHRINK_SELECTION,				OnShrinkSelection)
	ON_COMMAND(ID_PATTERN_SETINSTRUMENT,		OnSetSelInstrument)
	ON_COMMAND(ID_PATTERN_ADDCHANNEL_FRONT,		OnAddChannelFront)
	ON_COMMAND(ID_PATTERN_ADDCHANNEL_AFTER,		OnAddChannelAfter)
	ON_COMMAND(ID_PATTERN_TRANSPOSECHANNEL,		OnTransposeChannel)
	ON_COMMAND(ID_PATTERN_DUPLICATECHANNEL,		OnDuplicateChannel)
	ON_COMMAND(ID_PATTERN_REMOVECHANNEL,		OnRemoveChannel)
	ON_COMMAND(ID_PATTERN_REMOVECHANNELDIALOG,	OnRemoveChannelDialog)
	ON_COMMAND(ID_PATTERN_AMPLIFY,				OnPatternAmplify)
	ON_COMMAND(ID_CLEAR_SELECTION,				OnClearSelectionFromMenu)
	ON_COMMAND(ID_SHOWTIMEATROW,				OnShowTimeAtRow)
	ON_COMMAND(ID_PATTERN_EDIT_PCNOTE_PLUGIN,	OnTogglePCNotePluginEditor)
	ON_COMMAND(ID_SETQUANTIZE,					OnSetQuantize)
	ON_COMMAND_RANGE(ID_CHANGE_INSTRUMENT, ID_CHANGE_INSTRUMENT+MAX_INSTRUMENTS, OnSelectInstrument)
	ON_COMMAND_RANGE(ID_CHANGE_PCNOTE_PARAM, ID_CHANGE_PCNOTE_PARAM + ModCommand::maxColumnValue, OnSelectPCNoteParam)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO,			OnUpdateUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO,			OnUpdateRedo)
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT+MAX_MIXPLUGINS, OnSelectPlugin) //rewbs.patPlugName


	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

static_assert(ModCommand::maxColumnValue <= 999, "Command range for ID_CHANGE_PCNOTE_PARAM is designed for 999");

const CSoundFile *CViewPattern::GetSoundFile() const { return (GetDocument() != nullptr) ? GetDocument()->GetSoundFile() : nullptr; };
CSoundFile *CViewPattern::GetSoundFile() { return (GetDocument() != nullptr) ? GetDocument()->GetSoundFile() : nullptr; };


CViewPattern::CViewPattern()
//--------------------------
{
	m_pEffectVis = nullptr; //rewbs.fxvis
	m_bLastNoteEntryBlocked = false;

	m_nTransposeAmount = 1;
	m_nPattern = 0;
	m_nOrder = 0;
	m_nDetailLevel = PatternCursor::lastColumn;
	m_pEditWnd = NULL;
	m_pGotoWnd = NULL;
	m_Dib.Init(CMainFrame::bmpNotes);
	UpdateColors();
	m_PCNoteEditMemory = ModCommand::Empty();

	for(size_t i = 0; i < CountOf(octaveKeyMemory); i++)
	{
		octaveKeyMemory[i] = NOTE_NONE;
	}
}


CViewPattern::~CViewPattern()
//---------------------------
{
	m_offScreenBitmap.DeleteObject();
	m_offScreenDC.DeleteDC();
}


void CViewPattern::OnInitialUpdate()
//----------------------------------
{
	CModScrollView::OnInitialUpdate();
	MemsetZero(ChnVUMeters);
	MemsetZero(OldVUMeters);
	memset(previousNote, NOTE_NONE, sizeof(previousNote));
	memset(splitActiveNoteChannel, 0xFF, sizeof(splitActiveNoteChannel));
	memset(activeNoteChannel, 0xFF, sizeof(activeNoteChannel));
	m_nPlayPat = PATTERNINDEX_INVALID;
	m_nPlayRow = m_nNextPlayRow = 0;
	m_nPlayTick = 0;
	m_nTicksOnRow = 1;
	m_nMidRow = 0;
	m_nDragItem = 0;
	m_bInItemRect = false;
	m_bContinueSearch = false;
	m_Status = psShowPluginNames;
	m_nXScroll = m_nYScroll = 0;
	m_nPattern = 0;
	m_nSpacing = 0;
	m_nAccelChar = 0;
	PatternFont::UpdateFont(m_hWnd);
	UpdateSizes();
	UpdateScrollSize();
	SetCurrentPattern(0);
	m_nFoundInstrument = 0;
	m_nLastPlayedRow = 0;
	m_nLastPlayedOrder = 0;
	prevChordNote = NOTE_NONE;
}


bool CViewPattern::SetCurrentPattern(PATTERNINDEX pat, ROWINDEX row)
//------------------------------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	
	if(pSndFile == nullptr || pat >= pSndFile->Patterns.Size())
		return false;
	if(m_pEditWnd && m_pEditWnd->IsWindowVisible())
		m_pEditWnd->ShowWindow(SW_HIDE);
	
	if(pat + 1 < pSndFile->Patterns.Size() && !pSndFile->Patterns.IsValidPat(pat))
		pat = 0;
	while(pat > 0 && !pSndFile->Patterns.IsValidPat(pat))
		pat--;

	if(!pSndFile->Patterns.IsValidPat(pat))
	{
		return false;
	}
	
	bool updateScroll = false;
	m_nPattern = pat;
	if(row != ROWINDEX_INVALID && row != GetCurrentRow() && row < pSndFile->Patterns[m_nPattern].GetNumRows())
	{
		m_Cursor.SetRow(row);
		updateScroll = true;
	}
	if(GetCurrentRow() >= pSndFile->Patterns[m_nPattern].GetNumRows())
	{
		m_Cursor.SetRow(0);
	}
	
	SetSelToCursor();

	UpdateSizes();
	UpdateScrollSize();
	UpdateIndicator();

	if(m_bWholePatternFitsOnScreen)
		SetScrollPos(SB_VERT, 0);
	else if(updateScroll)
		SetScrollPos(SB_VERT, (int)GetCurrentRow() * GetColumnHeight());

	UpdateScrollPos();
	InvalidatePattern(true);
	SendCtrlMessage(CTRLMSG_PATTERNCHANGED, m_nPattern);

	return true;
}


// This should be used instead of consecutive calls to SetCurrentRow() then SetCurrentColumn().
bool CViewPattern::SetCursorPosition(const PatternCursor &cursor, bool wrap)
//--------------------------------------------------------------------------
{
	// Set row, but do not update scroll position yet
	// as there is another position update on the way:
	SetCurrentRow(cursor.GetRow(), wrap, false);
	// Now set column and update scroll position:
	SetCurrentColumn(cursor);
	return true;
}


ROWINDEX CViewPattern::SetCurrentRow(ROWINDEX row, bool wrap, bool updateHorizontalScrollbar)
//-------------------------------------------------------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidIndex(m_nPattern))
		return ROWINDEX_INVALID;

	if(wrap && pSndFile->Patterns[m_nPattern].GetNumRows())
	{
		if ((int)row < 0)
		{
			if(m_Status[psKeyboardDragSelect | psMouseDragSelect])
			{
				row = 0;
			} else
			if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL)
			{
				ORDERINDEX curOrder = GetCurrentOrder();
				const ORDERINDEX prevOrd = pSndFile->Order.GetPreviousOrderIgnoringSkips(curOrder);
				if (prevOrd < curOrder && m_nPattern == pSndFile->Order[curOrder])
				{
					const PATTERNINDEX nPrevPat = pSndFile->Order[prevOrd];
					if ((nPrevPat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[nPrevPat].GetNumRows()))
					{
						SetCurrentOrder(prevOrd);
						if (SetCurrentPattern(nPrevPat))
							return SetCurrentRow(pSndFile->Patterns[nPrevPat].GetNumRows() + (int)row);
					}
				}
				row = 0;
			} else
			if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_WRAP)
			{
				if ((int)row < (int)0) row += pSndFile->Patterns[m_nPattern].GetNumRows();
				row %= pSndFile->Patterns[m_nPattern].GetNumRows();
			}
		} else //row >= 0
		if (row >= pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			if(m_Status[psKeyboardDragSelect | psMouseDragSelect])
			{
				row = pSndFile->Patterns[m_nPattern].GetNumRows()-1;
			} else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL)
			{
				ORDERINDEX curOrder = GetCurrentOrder();
				ORDERINDEX nextOrder = pSndFile->Order.GetNextOrderIgnoringSkips(curOrder);
				if(nextOrder > curOrder && m_nPattern == pSndFile->Order[curOrder])
				{
					PATTERNINDEX nextPat = pSndFile->Order[nextOrder];
					if ((nextPat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[nextPat].GetNumRows()))
					{
						const ROWINDEX newRow = row - pSndFile->Patterns[m_nPattern].GetNumRows();
						SetCurrentOrder(nextOrder);
						if (SetCurrentPattern(nextPat))
							return SetCurrentRow(newRow);
					}
				}
				row = pSndFile->Patterns[m_nPattern].GetNumRows()-1;
			} else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_WRAP)
			{
				row %= pSndFile->Patterns[m_nPattern].GetNumRows();
			}
		}
	}

	if(!(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL))
	{
		if(static_cast<int>(row) < 0)
			row = 0;
		if(row >= pSndFile->Patterns[m_nPattern].GetNumRows())
			row = pSndFile->Patterns[m_nPattern].GetNumRows() - 1;
	}

	if ((row >= pSndFile->Patterns[m_nPattern].GetNumRows()) || (!m_szCell.cy)) return false;
	// Fix: If cursor isn't on screen move both scrollbars to make it visible
	InvalidateRow();
	m_Cursor.SetRow(row);
	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	UpdateScrollbarPositions(updateHorizontalScrollbar);
	InvalidateRow();

	PatternCursor selStart(m_Cursor);
	if(m_Status[psKeyboardDragSelect | psMouseDragSelect] && !m_Status[psDragnDropEdit])
	{
		selStart.Set(m_StartSel);
	}
	SetCurSel(selStart, m_Cursor);
	UpdateIndicator();

// 	Log("Row: %d; Chan: %d; ColType: %d;\n",
// 		selStart.GetRow(),
// 		selStart.GetChannel(),
// 		selStart.GetColumnType());

	return row;
}


bool CViewPattern::SetCurrentColumn(CHANNELINDEX channel, PatternCursor::Columns column)
//--------------------------------------------------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return false;
	}

	LimitMax(column, m_nDetailLevel);
	m_Cursor.SetColumn(channel, column);

	PatternCursor selStart(m_Cursor);

	if(m_Status[psKeyboardDragSelect | psMouseDragSelect] && !m_Status[psDragnDropEdit])
	{
		selStart = m_StartSel;
	}
	SetCurSel(selStart, m_Cursor);
	// Fix: If cursor isn't on screen move both scrollbars to make it visible
	UpdateScrollbarPositions();
	UpdateIndicator();
	return true;
}


// Set document as modified and optionally update all pattern views.
void CViewPattern::SetModified(bool updateAllViews)
//-------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc != nullptr)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(this, PatternHint(m_nPattern).Data(), updateAllViews ? nullptr : this);
	}
}


// Fix: If cursor isn't on screen move scrollbars to make it visible
// Fix: save pattern scrollbar position when switching to other tab
// Assume that m_nRow and m_dwCursor are valid
// When we switching to other tab the CViewPattern object is deleted
// and when switching back new one is created
bool CViewPattern::UpdateScrollbarPositions(bool updateHorizontalScrollbar)
//-------------------------------------------------------------------------
{
// HACK - after new CViewPattern object created SetCurrentRow() and SetCurrentColumn() are called -
// just skip first two calls of UpdateScrollbarPositions() if pModDoc->GetOldPatternScrollbarsPos() is valid
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSize scroll = pModDoc->GetOldPatternScrollbarsPos();
		if(scroll.cx >= 0)
		{
			OnScrollBy(scroll);
			scroll.cx = -1;
			pModDoc->SetOldPatternScrollbarsPos(scroll);
			return true;
		} else if(scroll.cx >= -1)
		{
			scroll.cx = -2;
			pModDoc->SetOldPatternScrollbarsPos(scroll);
			return true;
		}
	}
	CSize scroll(0,0);
	UINT row = GetCurrentRow();
	UINT yofs = GetYScrollPos();
	CRect rect;
	GetClientRect(&rect);
	rect.top += m_szHeader.cy;
	int numrows = (rect.bottom - rect.top - 1) / m_szCell.cy;
	if (numrows < 1) numrows = 1;
	if (m_nMidRow)
	{
		if (row != yofs)
		{
			scroll.cy = (int)(row - yofs) * m_szCell.cy;
		}
	} else
	{
		if (row < yofs)
		{
			scroll.cy = (int)(row - yofs) * m_szCell.cy;
		} else
		if (row > yofs + (UINT)numrows - 1)
		{
			scroll.cy = (int)(row - yofs - numrows + 1) * m_szCell.cy;
		}
	}

	if(updateHorizontalScrollbar)
	{
		UINT xofs = GetXScrollPos();
		const CHANNELINDEX nchn = GetCurrentChannel();
		if (nchn < xofs)
		{
			scroll.cx = (int)(xofs - nchn) * m_szCell.cx;
			scroll.cx *= -1;
		} else
		if (nchn > xofs)
		{
			int maxcol = (rect.right - m_szHeader.cx) / m_szCell.cx;
			if ((nchn >= (xofs+maxcol)) && (maxcol >= 0))
			{
				scroll.cx = (int)(nchn - xofs - maxcol + 1) * m_szCell.cx;
			}
		}
	}
	if( scroll.cx != 0 || scroll.cy != 0 )
	{
		OnScrollBy(scroll, TRUE);
	}
	return true;
}

DWORD CViewPattern::GetDragItem(CPoint point, RECT &outRect)
//----------------------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return 0;
	}

	CRect rcClient, rect, plugRect; 	//rewbs.patPlugNames
	UINT n;
	int xofs, yofs;

	GetClientRect(&rcClient);
	xofs = GetXScrollPos();
	yofs = GetYScrollPos();
	rect.SetRect(m_szHeader.cx, 0, m_szHeader.cx + GetColumnWidth(), m_szHeader.cy);
	plugRect.SetRect(m_szHeader.cx, m_szHeader.cy - m_szPluginHeader.cy, m_szHeader.cx + GetColumnWidth(), m_szHeader.cy);	//rewbs.patPlugNames

	const CHANNELINDEX nmax = pSndFile->GetNumChannels();
	// Checking channel headers
	if (m_Status[psShowPluginNames])
	{
		n = xofs;
		for(CHANNELINDEX ihdr = 0; n < nmax; ihdr++, n++)
		{
			if (plugRect.PtInRect(point))
			{
				outRect = plugRect;
				return (DRAGITEM_PLUGNAME | n);
			}
			plugRect.OffsetRect(GetColumnWidth(), 0);
		}
	}
	n = xofs;
	for (CHANNELINDEX ihdr=0; n<nmax; ihdr++, n++)
	{
		if (rect.PtInRect(point))
		{
			outRect = rect;
			return (DRAGITEM_CHNHEADER | n);
		}
		rect.OffsetRect(GetColumnWidth(), 0);
	}
	if(pSndFile->Patterns.IsValidPat(m_nPattern) && (pSndFile->GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)))
	{
		// Clicking on upper-left corner with pattern number (for pattern properties)
		rect.SetRect(0, 0, m_szHeader.cx, m_szHeader.cy);
		if (rect.PtInRect(point))
		{
			outRect = rect;
			return DRAGITEM_PATTERNHEADER;
		}
	}
	return 0;
}


// Drag a selection to position "cursor".
// If scrollHorizontal is true, the point's channel is ensured to be visible.
// Likewise, if scrollVertical is true, the point's row is ensured to be visible.
// If noMode if specified, the original selection points are not altered.
bool CViewPattern::DragToSel(const PatternCursor &cursor, bool scrollHorizontal, bool scrollVertical, bool noMove)
//----------------------------------------------------------------------------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return false;
	}

	CRect rect;
	int yofs = GetYScrollPos(), xofs = GetXScrollPos();
	int row, col;

	if(!m_szCell.cy) return false;
	GetClientRect(&rect);
	if (!noMove) SetCurSel(m_StartSel, cursor);
	if (!scrollHorizontal && !scrollVertical) return true;

	// Scroll to row
	row = cursor.GetRow();
	if (scrollVertical && row < (int)pSndFile->Patterns[m_nPattern].GetNumRows())
	{
		row += m_nMidRow;
		rect.top += m_szHeader.cy;
		int numrows = (rect.bottom - rect.top - 1) / m_szCell.cy;
		if (numrows < 1) numrows = 1;
		if (row < yofs)
		{
			CSize sz;
			sz.cx = 0;
			sz.cy = (int)(row - yofs) * m_szCell.cy;
			OnScrollBy(sz, TRUE);
		} else
		if (row > yofs + numrows - 1)
		{
			CSize sz;
			sz.cx = 0;
			sz.cy = (int)(row - yofs - numrows + 1) * m_szCell.cy;
			OnScrollBy(sz, TRUE);
		}
	}

	// Scroll to column
	col = cursor.GetChannel();
	if (scrollHorizontal && col < (int)pSndFile->GetNumChannels())
	{
		int maxcol = (rect.right - m_szHeader.cx) - 4;
		maxcol -= GetColumnOffset(cursor.GetColumnType());
		maxcol /= GetColumnWidth();
		if (col < xofs)
		{
			CSize size(-m_szCell.cx,0);
			if (!noMove) size.cx = (col - xofs) * (int)m_szCell.cx;
			OnScrollBy(size, TRUE);
		} else
		if ((col > xofs+maxcol) && (maxcol > 0))
		{
			CSize size(m_szCell.cx,0);
			if (!noMove) size.cx = (col - maxcol + 1) * (int)m_szCell.cx;
			OnScrollBy(size, TRUE);
		}
	}
	UpdateWindow();
	return true;
}


bool CViewPattern::SetPlayCursor(PATTERNINDEX pat, ROWINDEX row, uint32 tick)
//---------------------------------------------------------------------------
{
	PATTERNINDEX oldPat = m_nPlayPat;
	ROWINDEX oldRow = m_nPlayRow;
	uint32 oldTick = m_nPlayTick;

	m_nPlayPat = pat;
	m_nPlayRow = row;
	m_nPlayTick = tick;

	if(m_nPlayTick != oldTick && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SMOOTHSCROLL))
		InvalidatePattern(true);
	else if (oldPat == m_nPattern)
		InvalidateRow(oldRow);
	else if (m_nPlayPat == m_nPattern)
		InvalidateRow(m_nPlayRow);

	return true;
}


UINT CViewPattern::GetCurrentInstrument() const
//---------------------------------------------
{
	return SendCtrlMessage(CTRLMSG_GETCURRENTINSTRUMENT);
}


bool CViewPattern::ShowEditWindow()
//---------------------------------
{
	if (!m_pEditWnd)
	{
		m_pEditWnd = new CEditCommand(*GetSoundFile());
	}
	if (m_pEditWnd)
	{
		m_pEditWnd->ShowEditWindow(m_nPattern, m_Cursor, this);
		return true;
	}
	return false;
}


bool CViewPattern::PrepareUndo(const PatternCursor &beginSel, const PatternCursor &endSel, const char *description)
//-----------------------------------------------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX nChnBeg = beginSel.GetChannel(), nChnEnd = endSel.GetChannel();
	const ROWINDEX nRowBeg = beginSel.GetRow(), nRowEnd = endSel.GetRow();

	if((nChnEnd < nChnBeg) || (nRowEnd < nRowBeg) || pModDoc == nullptr) return false;
	return pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, nChnBeg, nRowBeg, nChnEnd - nChnBeg + 1, nRowEnd-nRowBeg + 1, description);
}


BOOL CViewPattern::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = CMainFrame::GetInputHandler();

			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxViewPatterns + 1 + m_Cursor.GetColumnType());
			// If editing is disabled, preview notes no matter which column we are in
			if(!IsEditingEnabled() && TrackerSettings::Instance().patternNoEditPopup) ctx = kCtxViewPatternsNote;

			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull)
			{
				return true; // Mapped to a command, no need to pass message on.
			}
			//HACK: fold kCtxViewPatternsFX and kCtxViewPatternsFXparam so that all commands of 1 are active in the other
			else {
				if (ctx==kCtxViewPatternsFX)
				{
					if (ih->KeyEvent(kCtxViewPatternsFXparam, nChar, nRepCnt, nFlags, kT) != kcNull)
					{
						return true; // Mapped to a command, no need to pass message on.
					}
				} else if (ctx==kCtxViewPatternsFXparam)
				{
					if (ih->KeyEvent(kCtxViewPatternsFX, nChar, nRepCnt, nFlags, kT) != kcNull)
					{
						return true; // Mapped to a command, no need to pass message on.
					}
				} else if (ctx == kCtxViewPatternsIns)
				{
					// Do the same with instrument->note column
					if (ih->KeyEvent(kCtxViewPatternsNote, nChar, nRepCnt, nFlags, kT) != kcNull)
					{
						return true; // Mapped to a command, no need to pass message on.
					}
				}
			}
			//end HACK.
		} else if(pMsg->message == WM_MBUTTONDOWN)
		{
			// Open quick channel properties dialog if we're middle-clicking a channel header.
			CPoint point(GET_X_LPARAM(pMsg->lParam), GET_Y_LPARAM(pMsg->lParam));
			if(point.y < m_szHeader.cy - m_szPluginHeader.cy)
			{
				PatternCursor cursor = GetPositionFromPoint(point);
				if(cursor.GetChannel() < GetDocument()->GetNumChannels())
				{
					ClientToScreen(&point);
					quickChannelProperties.Show(GetDocument(), cursor.GetChannel(), m_nPattern, point);
					return true;
				}
			}
		}
	}

	return CModScrollView::PreTranslateMessage(pMsg);
}


////////////////////////////////////////////////////////////////////////
// CViewPattern message handlers

void CViewPattern::OnDestroy()
//----------------------------
{
// Fix: save pattern scrollbar position when switching to other tab
// When we switching to other tab the CViewPattern object is deleted
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		pModDoc->SetOldPatternScrollbarsPos(CSize(m_nXScroll*m_szCell.cx, m_nYScroll*m_szCell.cy));
	};
	if (m_pEffectVis)
	{
		m_pEffectVis->DoClose();
		delete m_pEffectVis;
		m_pEffectVis = NULL;
	}

	if (m_pEditWnd)
	{
		m_pEditWnd->DestroyWindow();
		delete m_pEditWnd;
		m_pEditWnd = NULL;
	}

	if (m_pGotoWnd)
	{
		m_pGotoWnd->DestroyWindow();
		delete m_pGotoWnd;
		m_pGotoWnd = NULL;
	}

	CModScrollView::OnDestroy();
}


void CViewPattern::OnSetFocus(CWnd *pOldWnd)
//------------------------------------------
{
	CScrollView::OnSetFocus(pOldWnd);
	m_Status.set(psFocussed);
	InvalidateRow();
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		pModDoc->SetNotifications(Notification::Position | Notification::VUMeters);
		pModDoc->SetFollowWnd(m_hWnd);
		UpdateIndicator();
	}
}


void CViewPattern::OnKillFocus(CWnd *pNewWnd)
//-------------------------------------------
{
	CScrollView::OnKillFocus(pNewWnd);

	m_Status.reset(psKeyboardDragSelect | psCtrlDragSelect | psFocussed);
	InvalidateRow();
}


//rewbs.customKeys
void CViewPattern::OnGrowSelection()
//----------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	BeginWaitCursor();

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	const PatternCursor startSel = m_Selection.GetUpperLeft();
	const PatternCursor endSel = m_Selection.GetLowerRight();
	PrepareUndo(startSel, PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows(), endSel), "Grow Selection");

	const ROWINDEX finalDest = m_Selection.GetStartRow() + (m_Selection.GetNumRows() - 1) * 2;
	for(int row = finalDest; row > (int)startSel.GetRow(); row -= 2)
	{
		if(ROWINDEX(row) >= pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			continue;
		}

		int offset = row - startSel.GetRow();

		for(CHANNELINDEX chn = startSel.GetChannel(); chn <= endSel.GetChannel(); chn++)
		{
			for(int i = PatternCursor::firstColumn; i <= PatternCursor::lastColumn; i++)
			{
				PatternCursor cell(row, chn, static_cast<PatternCursor::Columns>(i));
				if(!m_Selection.ContainsHorizontal(cell))
				{
					// We might have to skip the first / last few entries.
					continue;
				}

				ModCommand *dest = pSndFile->Patterns[m_nPattern].GetpModCommand(row, chn);
				ModCommand *src  = pSndFile->Patterns[m_nPattern].GetpModCommand(row - offset / 2, chn);
				ModCommand *blank= pSndFile->Patterns[m_nPattern].GetpModCommand(row - 1, chn);		// Row "in between"

				switch(i)
				{
				case PatternCursor::noteColumn:
					dest->note = src->note;
					blank->note = NOTE_NONE;
					break;

				case PatternCursor::instrColumn:
					dest->instr = src->instr;
					blank->instr = 0;
					break;

				case PatternCursor::volumeColumn:
					dest->volcmd = src->volcmd;
					blank->volcmd = VOLCMD_NONE;
					dest->vol = src->vol;
					blank->vol = 0;
					break;

				case PatternCursor::effectColumn:
					dest->command = src->command;
					blank->command = CMD_NONE;
					break;

				case PatternCursor::paramColumn:
					dest->param = src->param;
					blank->param = 0;
					break;
				}
			}
		}
	}

	// Adjust selection
	m_Selection = PatternRect(startSel, PatternCursor(MIN(finalDest, pSndFile->Patterns[m_nPattern].GetNumRows() - 1), endSel));

	InvalidatePattern(false);
	SetModified();
	EndWaitCursor();
	SetFocus();
}


void CViewPattern::OnShrinkSelection()
//-----------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	BeginWaitCursor();

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	const PatternCursor startSel = m_Selection.GetUpperLeft();
	const PatternCursor endSel = m_Selection.GetLowerRight();
	PrepareUndo(startSel, endSel, "Shrink Selection");

	const ROWINDEX finalDest = m_Selection.GetStartRow() + (m_Selection.GetNumRows() - 1) / 2;
	for(ROWINDEX row = startSel.GetRow(); row <= endSel.GetRow(); row++)
	{
		const ROWINDEX offset = row - startSel.GetRow();
		const ROWINDEX srcRow = startSel.GetRow() + (offset * 2);

		for(CHANNELINDEX chn = startSel.GetChannel(); chn <= endSel.GetChannel(); chn++)
		{
			for(int i = PatternCursor::firstColumn; i <= PatternCursor::lastColumn; i++)
			{
				PatternCursor cell(row, chn, static_cast<PatternCursor::Columns>(i));
				if(!m_Selection.ContainsHorizontal(cell))
				{
					// We might have to skip the first / last few entries.
					continue;
				}

				ModCommand *dest = pSndFile->Patterns[m_nPattern].GetpModCommand(row, chn);
				ModCommand src;

				if(row <= finalDest)
				{
					// Normal shrink operation
					src = *pSndFile->Patterns[m_nPattern].GetpModCommand(srcRow, chn);

					// If source command is empty, try next source row (so we don't lose all the stuff that's on odd rows).
					if(srcRow < pSndFile->Patterns[m_nPattern].GetNumRows() - 1)
					{
						const ModCommand *srcNext = pSndFile->Patterns[m_nPattern].GetpModCommand(srcRow + 1, chn);
						if(src.note == NOTE_NONE) src.note = srcNext->note;
						if(src.instr == 0) src.instr = srcNext->instr;
						if(src.volcmd == VOLCMD_NONE)
						{
							src.volcmd = srcNext->volcmd;
							src.vol = srcNext->vol;
						}
						if(src.command == CMD_NONE)
						{
							src.command = srcNext->command;
							src.param = srcNext->param;
						}
					}
				} else
				{
					// Clean up rows that are now supposed to be empty.
					src = ModCommand::Empty();
				}

				switch(i)
				{
				case PatternCursor::noteColumn:
					dest->note = src.note;
					break;

				case PatternCursor::instrColumn:
					dest->instr = src.instr;
					break;

				case PatternCursor::volumeColumn:
					dest->vol = src.vol;      
					dest->volcmd = src.volcmd;
					break;

				case PatternCursor::effectColumn:
					dest->command = src.command;
					break;

				case PatternCursor::paramColumn:
					dest->param = src.param;
					break;
				}
			}
		}
	}

	// Adjust selection
	m_Selection = PatternRect(startSel, PatternCursor(MIN(finalDest, pSndFile->Patterns[m_nPattern].GetNumRows() - 1), endSel));

	InvalidatePattern(false);
	SetModified();
	EndWaitCursor();
	SetFocus();
}
//rewbs.customKeys

void CViewPattern::OnClearSelectionFromMenu()
//-------------------------------------------
{
	OnClearSelection();
}

void CViewPattern::OnClearSelection(bool ITStyle, RowMask rm) //Default RowMask: all elements enabled
//-----------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern) || !IsEditingEnabled_bmsg())
	{
		return;
	}

	BeginWaitCursor();

	// If selection ends to a note column, in ITStyle extending it to instrument column since the instrument data is
	// removed with note data.
	if(ITStyle && m_Selection.GetEndColumn() == PatternCursor::noteColumn)
	{
		PatternCursor lower(m_Selection.GetLowerRight());
		lower.Move(0, 0, 1);
		m_Selection = PatternRect(m_Selection.GetUpperLeft(), lower);
	}

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());

	PrepareUndo(m_Selection, "Clear Selection");

	const ROWINDEX endRow = m_Selection.GetEndRow();
	for(ROWINDEX row = m_Selection.GetStartRow(); row <= endRow; row++)
	{
		for(CHANNELINDEX chn = m_Selection.GetStartChannel(); chn <= m_Selection.GetEndChannel(); chn++)
		{
			for(int i = PatternCursor::firstColumn; i <= PatternCursor::lastColumn; i++)
			{
				PatternCursor cell(row, chn, static_cast<PatternCursor::Columns>(i));
				if(!m_Selection.ContainsHorizontal(cell))
				{
					// We might have to skip the first / last few entries.
					continue;
				}

				ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(row, chn);

				switch(i)
				{
				case PatternCursor::noteColumn:		// Clear note
					if(rm.note)
					{
						if(m->IsPcNote())
						{  // Clear whole cell if clearing PC note
							m->Clear();
						}
						else
						{
							m->note = NOTE_NONE;
							if (ITStyle) m->instr = 0;
						}
					}
					break;

				case PatternCursor::instrColumn:	// Clear instrument
					if(rm.instrument)
					{
						m->instr = 0;
					}
					break;

				case PatternCursor::volumeColumn:	// Clear volume
					if(rm.volume)
					{
						m->volcmd = VOLCMD_NONE;
						m->vol = 0;
					}
					break;

				case PatternCursor::effectColumn:	// Clear Command
					if(rm.command)
					{
						m->command = CMD_NONE;
						if(m->IsPcNote())
						{
							m->SetValueEffectCol(0);
						}
					}
					break;

				case PatternCursor::paramColumn:	// Clear Command Param
					if(rm.parameter)
					{
						m->param = 0;
						if(m->IsPcNote())
						{
							m->SetValueEffectCol(0);

							if(cell.CompareColumn(m_Selection.GetUpperLeft()) == 0)
							{
								// If this is the first selected column, update effect column char as well
								PatternCursor upper(m_Selection.GetUpperLeft());
								upper.Move(0, 0, -1);
								m_Selection = PatternRect(upper, m_Selection.GetLowerRight());
							}
						}
					}
					break;
				}
			}
		}
	}

	// Expand invalidation to the whole column. Needed for:
	// - Last column is the effect character (parameter needs to be invalidated, too
	// - PC Notes
	// - Default volume display is enabled.
	PatternCursor endCursor(m_Selection.GetEndRow(), m_Selection.GetEndChannel() + 1);

	InvalidateArea(m_Selection.GetUpperLeft(), endCursor);
	SetModified();
	EndWaitCursor();
	SetFocus();
}


void CViewPattern::OnEditCut()
//----------------------------
{
	OnEditCopy();
	OnClearSelection(false);
}


void CViewPattern::OnEditCopy()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CopyPattern(m_nPattern, m_Selection);
		SetFocus();
	}
}


void CViewPattern::OnLButtonDown(UINT nFlags, CPoint point)
//---------------------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	SetFocus();
	m_nDropItem = m_nDragItem = GetDragItem(point, m_rcDragItem);
	m_Status.set(psDragging);
	m_bInItemRect = true;
	m_Status.reset(psShiftDragging);

	PatternCursor pointCursor(GetPositionFromPoint(point));

	SetCapture();
	if(point.x >= m_szHeader.cx && point.y <= m_szHeader.cy - m_szPluginHeader.cy)
	{
		// Click on channel header
		if (nFlags & MK_CONTROL)
		{
			TogglePendingMute(pointCursor.GetChannel());
		}
	} else if(point.x >= m_szHeader.cx && point.y > m_szHeader.cy)
	{
		// Click on pattern data

		if(CMainFrame::GetInputHandler()->SelectionPressed()
			&& (m_Status[psShiftSelect]
				|| m_Selection.GetUpperLeft() == m_Selection.GetLowerRight()
				|| !m_Selection.Contains(pointCursor)))
		{
			// Shift pressed -> set 2nd selection point
			// This behaviour is only used if:
			// * Shift-click has previously been used since the shift key has been pressed down (psShiftSelect flag is set),
			// * No selection has been made yet, or
			// * Shift-clicking outside the current selection.
			// This is necessary so that selections can still be moved properly while the shift button is pressed (for copy-move).
			DragToSel(pointCursor, true, true);
			m_Status.set(psShiftSelect);
		} else
		{
			// Set first selection point
			m_StartSel = pointCursor;
			if(m_StartSel.GetChannel() < pSndFile->GetNumChannels())
			{
				m_Status.set(psMouseDragSelect);

				if(m_Status[psCtrlDragSelect])
				{
					SetCurSel(m_StartSel);
				}
				if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_DRAGNDROPEDIT)
					&& ((m_Selection.GetUpperLeft() != m_Selection.GetLowerRight()) || m_Status[psCtrlDragSelect])
					&& m_Selection.Contains(m_StartSel))
				{
					m_Status.set(psDragnDropEdit);
				} else if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CENTERROW)
				{
					SetCurSel(m_StartSel);
				} else
				{
					// Fix: Horizontal scrollbar pos screwed when selecting with mouse
					SetCursorPosition(m_StartSel);
				}
			}
		}
	} else if(point.x < m_szHeader.cx && point.y > m_szHeader.cy)
	{
		// Mark row number => mark whole row (start)
		InvalidateSelection();
		if(pointCursor.GetRow() < pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			m_StartSel.Set(pointCursor);
			SetCurSel(pointCursor, PatternCursor(pointCursor.GetRow(), pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn));
			m_Status.set(psRowSelection);
		}
	}

	if(m_nDragItem)
	{
		InvalidateRect(&m_rcDragItem, FALSE);
		UpdateWindow();
	}
}


void CViewPattern::OnLButtonDblClk(UINT uFlags, CPoint point)
//-----------------------------------------------------------
{
	PatternCursor cursor = GetPositionFromPoint(point);
	if(cursor == m_Cursor && point.y >= m_szHeader.cy)
	{
		// Double-click pattern cell: Select whole column or show cell properties.
		if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_DBLCLICKSELECT))
		{
			OnSelectCurrentColumn();
			m_Status.set(psChannelSelection |psDragging);
			return;
		} else
		{
			if(ShowEditWindow()) return;
		}
	}

	OnLButtonDown(uFlags, point);
}


void CViewPattern::OnLButtonUp(UINT nFlags, CPoint point)
//-------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	const bool bItemSelected = m_bInItemRect || ((m_nDragItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER);
	if (/*(!m_bDragging) ||*/ (!pModDoc)) return;

	m_bInItemRect = false;
	ReleaseCapture();
	m_Status.reset(psMouseDragSelect | psRowSelection | psChannelSelection | psDragging);
	// Drag & Drop Editing
	if(m_Status[psDragnDropEdit])
	{
		if(m_Status[psDragnDropping])
		{
			OnDrawDragSel();
			m_Status.reset(psDragnDropping);
			OnDropSelection();
		}

		if(GetPositionFromPoint(point) == m_StartSel)
		{
			SetCurSel(m_StartSel);
		}
		SetCursor(CMainFrame::curArrow);
		m_Status.reset(psDragnDropEdit);
	}
	if (((m_nDragItem & DRAGITEM_MASK) != DRAGITEM_CHNHEADER)
	 && ((m_nDragItem & DRAGITEM_MASK) != DRAGITEM_PATTERNHEADER)
	 && ((m_nDragItem & DRAGITEM_MASK) != DRAGITEM_PLUGNAME))
	{
		if ((m_nMidRow) && (m_Selection.GetUpperLeft() == m_Selection.GetLowerRight()))
		{
			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition(m_Selection.GetUpperLeft());
			//UpdateIndicator();
		}
	}
	if ((!bItemSelected) || (!m_nDragItem)) return;
	InvalidateRect(&m_rcDragItem, FALSE);
	const CHANNELINDEX nItemNo = static_cast<CHANNELINDEX>(m_nDragItem & DRAGITEM_VALUEMASK);
	const CHANNELINDEX nTargetNo = (m_nDropItem != 0) ? static_cast<CHANNELINDEX>(m_nDropItem & DRAGITEM_VALUEMASK) : CHANNELINDEX_INVALID;

	switch(m_nDragItem & DRAGITEM_MASK)
	{
	case DRAGITEM_CHNHEADER:
		if(nItemNo == nTargetNo && nTargetNo < pModDoc->GetNumChannels())
		{
			// Just clicked a channel header...
			if(nFlags & MK_SHIFT)
			{
				// Toggle record state
				pModDoc->Record1Channel(nItemNo);
				InvalidateChannelsHeaders();
			} else if(CMainFrame::GetInputHandler()->AltPressed())
			{
				// Solo / Unsolo
				OnSoloChannel(nItemNo);
			} else if(!(nFlags & MK_CONTROL))
			{
				// Mute / Unmute
				OnMuteChannel(nItemNo);
			}
		} else if(nTargetNo < pModDoc->GetNumChannels() && (m_nDropItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER)
		{
			// Dragged to other channel header => move or copy channel

			InvalidateRect(&m_rcDropItem, FALSE);

			const bool duplicate = (nFlags & MK_SHIFT) != 0;
			const CHANNELINDEX newChannels = pModDoc->GetNumChannels() + (duplicate ? 1 : 0);
			std::vector<CHANNELINDEX> channels(newChannels, 0);
			CHANNELINDEX i = 0;
			bool modified = duplicate;

			for(CHANNELINDEX nChn = 0; nChn < newChannels; nChn++)
			{
				if(nChn == nTargetNo)
				{
					channels[nChn] = nItemNo;
				}
				else
				{
					if(i == nItemNo && !duplicate)	// Don't want that source channel twice if we're just moving
					{
						i++;
					}
					channels[nChn] = i++;
				}
				if(channels[nChn] != nChn)
				{
					modified = true;
				}
			}
			if(modified && pModDoc->ReArrangeChannels(channels) != CHANNELINDEX_INVALID)
			{
				if(duplicate)
				{
					// Number of channels changed: Update channel headers and other information.
					pModDoc->UpdateAllViews(this, GeneralHint().Channels().ModType());
					SetCurrentPattern(m_nPattern);
				}
				InvalidatePattern(true);
				SetModified(false);
			}
		}
		break;

	case DRAGITEM_PATTERNHEADER:
		OnPatternProperties();
		break;

	case DRAGITEM_PLUGNAME:			//rewbs.patPlugNames
		if (nItemNo < MAX_BASECHANNELS)
			TogglePluginEditor(nItemNo);
		break;
	}

	m_nDropItem = 0;
}


void CViewPattern::OnPatternProperties()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc && pModDoc->GetrSoundFile().Patterns.IsValidPat(m_nPattern))
	{
		CPatternPropertiesDlg dlg(*pModDoc, m_nPattern, this);
		if(dlg.DoModal() == IDOK)
		{
			UpdateScrollSize();
			InvalidatePattern(true);
			SanitizeCursor();
		}
	}
}


void CViewPattern::OnRButtonDown(UINT flags, CPoint pt)
//-----------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	HMENU hMenu;

	// Too far left to get a ctx menu:
	if ((!pModDoc) || (pt.x < m_szHeader.cx))
	{
		return;
	}

	// Handle drag n drop
	if (m_Status[psDragnDropEdit])
	{
		if (m_Status[psDragnDropping])
		{
			OnDrawDragSel();
			m_Status.reset(psDragnDropping);
		}
		m_Status.reset(psDragnDropEdit | psMouseDragSelect);
		if(m_Status[psDragging])
		{
			m_Status.reset(psDragging);
			m_bInItemRect = false;
			ReleaseCapture();
		}
		SetCursor(CMainFrame::curArrow);
		return;
	}

	if ((hMenu = ::CreatePopupMenu()) == NULL)
	{
		return;
	}

	pSndFile = pModDoc->GetSoundFile();
	m_MenuCursor = GetPositionFromPoint(pt);

	// Right-click outside single-point selection? Reposition cursor to the new location
	if(!m_Selection.Contains(m_MenuCursor) && m_Selection.GetUpperLeft() == m_Selection.GetLowerRight())
	{
		if (pt.y > m_szHeader.cy)
		{
			//ensure we're not clicking header

			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition(m_MenuCursor);
		}
	}
	const CHANNELINDEX nChn = m_MenuCursor.GetChannel();

	if((flags & MK_CONTROL) != 0 && nChn < pSndFile->GetNumChannels() && (pt.y < m_szHeader.cy))
	{
		// Ctrl+Right-Click: Open quick channel properties.
		ClientToScreen(&pt);
		quickChannelProperties.Show(GetDocument(), nChn, m_nPattern, pt);
	} else if(nChn < pSndFile->GetNumChannels() && pSndFile->Patterns.IsValidPat(m_nPattern) && !(flags & (MK_CONTROL | MK_SHIFT)))
	{
		CInputHandler *ih = CMainFrame::GetInputHandler();

		//------ Plugin Header Menu --------- :
		if(m_Status[psShowPluginNames] &&
			(pt.y > m_szHeader.cy - m_szPluginHeader.cy) && (pt.y < m_szHeader.cy))
		{
			BuildPluginCtxMenu(hMenu, nChn, pSndFile);
		}
		
		//------ Channel Header Menu ---------- :
		else if (pt.y < m_szHeader.cy)
		{
			if (ih->ShiftPressed())
			{
				//Don't bring up menu if shift is pressed, else we won't get button up msg.
			} else
			{
				if (BuildSoloMuteCtxMenu(hMenu, ih, nChn, pSndFile))
					AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
				BuildRecordCtxMenu(hMenu, ih, nChn, pModDoc);
				BuildChannelControlCtxMenu(hMenu, ih);
			}
		}
		
		//------ Standard Menu ---------- :
		else if ((pt.x >= m_szHeader.cx) && (pt.y >= m_szHeader.cy))
		{
			// When combining menus, use bitwise ORs to avoid shortcuts
			if(BuildSelectionCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildEditCtxMenu(hMenu, ih, pModDoc))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildInterpolationCtxMenu(hMenu, ih)
				| BuildTransposeCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildVisFXCtxMenu(hMenu, ih)
				| BuildAmplifyCtxMenu(hMenu, ih)
				| BuildSetInstCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildPCNoteCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildGrowShrinkCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildMiscCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildRowInsDelCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));

			CString s = _T("&Quantize ");
			if(TrackerSettings::Instance().recordQuantizeRows != 0)
			{
				uint32 rows = TrackerSettings::Instance().recordQuantizeRows.Get();
				s.AppendFormat(_T("(Currently: %d Row%s)"), rows, rows == 1 ? _T("") : _T("s"));
			} else
			{
				s.Append(_T("Settings..."));
			}
			s.Append(_T("\t") + ih->GetKeyTextFromCommand(kcQuantizeSettings));
			AppendMenu(hMenu, MF_STRING | (TrackerSettings::Instance().recordQuantizeRows != 0 ? MF_CHECKED : 0), ID_SETQUANTIZE, s);
		}

		ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
	}
	::DestroyMenu(hMenu);
}

void CViewPattern::OnRButtonUp(UINT nFlags, CPoint point)
//-------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;

	m_nDragItem = GetDragItem(point, m_rcDragItem);
	CHANNELINDEX nItemNo = static_cast<CHANNELINDEX>(m_nDragItem & DRAGITEM_VALUEMASK);
	switch(m_nDragItem & DRAGITEM_MASK)
	{
	case DRAGITEM_CHNHEADER:
		if(nFlags & MK_SHIFT)
		{
			if(nItemNo < MAX_BASECHANNELS)
			{
				pModDoc->Record2Channel(nItemNo);
				InvalidateChannelsHeaders();
			}
		}
		break;
	}

	CModScrollView::OnRButtonUp(nFlags, point);
}


BOOL CViewPattern::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//-------------------------------------------------------------------
{
	if(nFlags & MK_CONTROL)
	{
		// Ctrl + mouse wheel: Increment / decrement values
		DataEntry(zDelta > 0, (nFlags & MK_SHIFT) == MK_SHIFT);
		return TRUE;
	}
	if(IsLiveRecord() && !m_Status[psDragActive])
	{
		// During live playback with "follow song" enabled, the mouse wheel can be used to jump forwards and backwards.
		CursorJump(-sgn(zDelta), false);
		return TRUE;
	}
	return CModScrollView::OnMouseWheel(nFlags, zDelta, pt);
}


void CViewPattern::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
//---------------------------------------------------------------------
{
	if(nButton == XBUTTON1) OnPrevOrder();
	else if(nButton == XBUTTON2) OnNextOrder();
	CModScrollView::OnXButtonUp(nFlags, nButton, point);
}


void CViewPattern::OnMouseMove(UINT nFlags, CPoint point)
//-------------------------------------------------------
{
	if(!m_Status[psDragging])
	{
		return;
	}

	// Drag&Drop actions
	if (m_nDragItem)
	{
		const CRect oldDropRect = m_rcDropItem;
		const DWORD oldDropItem = m_nDropItem;

		m_Status.set(psShiftDragging, (nFlags & MK_SHIFT) != 0);
		m_nDropItem = GetDragItem(point, m_rcDropItem);

		const bool b = (m_nDropItem == m_nDragItem);
		const bool dragChannel = (m_nDragItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER;

		if (b != m_bInItemRect || (m_nDropItem != oldDropItem && dragChannel))
		{
			m_bInItemRect = b;
			InvalidateRect(&m_rcDragItem, FALSE);

			// Dragging around channel headers? Update move indicator...
			if((m_nDropItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER) InvalidateRect(&m_rcDropItem, FALSE);
			if((oldDropItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER) InvalidateRect(&oldDropRect, FALSE);

			UpdateWindow();
		}
	}

	if(m_Status[psChannelSelection])
	{
		// Double-clicked a pattern cell to select whole channel.
		// Continue dragging to select more channels.
		const CSoundFile *pSndFile = GetSoundFile();
		if(pSndFile->Patterns.IsValidPat(m_nPattern))
		{
			const ROWINDEX lastRow = pSndFile->Patterns[m_nPattern].GetNumRows() - 1;

			CHANNELINDEX startChannel = m_Cursor.GetChannel();
			CHANNELINDEX endChannel = GetPositionFromPoint(point).GetChannel();

			m_StartSel = PatternCursor(0, startChannel, (startChannel <= endChannel ? PatternCursor::firstColumn : PatternCursor::lastColumn));
			PatternCursor endSel = PatternCursor(lastRow, endChannel, (startChannel <= endChannel ? PatternCursor::lastColumn : PatternCursor::firstColumn));

			DragToSel(endSel, true, false, false);
		}
	} else if(m_Status[psRowSelection] && point.y > m_szHeader.cy)
	{
		// Mark row number => mark whole row (continue)
		InvalidateSelection();

		PatternCursor cursor(GetPositionFromPoint(point));
		cursor.SetColumn(GetDocument()->GetNumChannels() - 1, PatternCursor::lastColumn);
		DragToSel(cursor, false, true, false);

	} else if(m_Status[psMouseDragSelect])
	{
		PatternCursor cursor(GetPositionFromPoint(point));

		const CSoundFile *pSndFile = GetSoundFile();
		if(pSndFile != nullptr && m_nPattern < pSndFile->Patterns.Size())
		{
			ROWINDEX row = cursor.GetRow();
			LimitMax(row, pSndFile->Patterns[m_nPattern].GetNumRows() - 1);
			cursor.SetRow(row);
		}

		// Drag & Drop editing
		if (m_Status[psDragnDropEdit])
		{
			const bool moved = m_DragPos.GetChannel() != cursor.GetChannel() || m_DragPos.GetRow() != cursor.GetRow();

			if(!m_Status[psDragnDropping])
			{
				SetCursor(CMainFrame::curDragging);
			}
			if(!m_Status[psDragnDropping] || moved)
			{
				if(m_Status[psDragnDropping]) OnDrawDragSel();
				m_Status.reset(psDragnDropping);
				DragToSel(cursor, true, true, true);
				m_DragPos = cursor;
				m_Status.set(psDragnDropping);
				OnDrawDragSel();
			}
		} else
		// Default: selection
		if (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CENTERROW)
		{
			DragToSel(cursor, true, true);
		} else
		{
			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition(cursor);
		}
	}
}


void CViewPattern::OnEditSelectAll()
//----------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		SetCurSel(PatternCursor(0), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn));
	}
}


void CViewPattern::OnEditSelectColumn()
//-------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		SetCurSel(PatternCursor(0, m_MenuCursor.GetChannel()), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, m_MenuCursor.GetChannel(), PatternCursor::lastColumn));
	}
}


void CViewPattern::OnSelectCurrentColumn()
//----------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		PatternCursor beginSel(0, GetCurrentChannel());
		PatternCursor endSel(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, GetCurrentChannel(), PatternCursor::lastColumn);
		// If column is already selected, select the current pattern
		if((beginSel == m_Selection.GetUpperLeft()) && (endSel == m_Selection.GetLowerRight()))
		{
			beginSel.Set(0, 0);
			endSel.Set(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn);
		}
		SetCurSel(beginSel, endSel);
	}
}


void CViewPattern::OnChannelReset()
//---------------------------------
{
	ResetChannel(m_MenuCursor.GetChannel());
}


// Reset all channel variables
void CViewPattern::ResetChannel(CHANNELINDEX chn)
//-----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	if(pModDoc == nullptr || (pSndFile = pModDoc->GetSoundFile()) == nullptr) return;

	const bool isMuted = pModDoc->IsChannelMuted(chn);
	if(!isMuted) pModDoc->MuteChannel(chn, true);
	pSndFile->m_PlayState.Chn[chn].Reset(ModChannel::resetTotal, *pSndFile, chn);
	if(!isMuted) pModDoc->MuteChannel(chn, false);
}


void CViewPattern::OnMuteFromClick()
//----------------------------------
{
	OnMuteChannel(m_MenuCursor.GetChannel());
}


void CViewPattern::OnMuteChannel(CHANNELINDEX chn)
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		pModDoc->SoloChannel(chn, false);
		pModDoc->MuteChannel(chn, !pModDoc->IsChannelMuted(chn));

		//If we just unmuted a channel, make sure none are still considered "solo".
		if(!pModDoc->IsChannelMuted(chn))
		{
			for(CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)
			{
				pModDoc->SoloChannel(i, false);
			}
		}

		InvalidateChannelsHeaders();
		pModDoc->UpdateAllViews(this, GeneralHint(chn).Channels());
	}
}


void CViewPattern::OnSoloFromClick()
//----------------------------------
{
	OnSoloChannel(m_MenuCursor.GetChannel());
}


// When trying to solo a channel that is already the only unmuted channel,
// this will result in unmuting all channels, in order to satisfy user habits.
// In all other cases, soloing a channel unsoloes all and mutes all except this channel
void CViewPattern::OnSoloChannel(CHANNELINDEX chn)
//------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc == nullptr) return;

	if (chn >= pModDoc->GetNumChannels())
	{
		return;
	}

	if (pModDoc->IsChannelSolo(chn))
	{
		bool nChnIsOnlyUnMutedChan = true;
		for (CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)	//check status of all other chans
		{
			if (i != chn && !pModDoc->IsChannelMuted(i))
			{
				nChnIsOnlyUnMutedChan = false;	//found a channel that isn't muted!
				break;
			}
		}
		if (nChnIsOnlyUnMutedChan)	// this is the only playable channel and it is already soloed ->  Unmute all
		{
			OnUnmuteAll();
			return;
		}
	}
	for(CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)
	{
		pModDoc->MuteChannel(i, !(i == chn)); //mute all chans except nChn, unmute nChn
		pModDoc->SoloChannel(i, (i == chn));  //unsolo all chans except nChn, solo nChn
	}
	InvalidateChannelsHeaders();
	pModDoc->UpdateAllViews(this, GeneralHint(chn).Channels());
}


void CViewPattern::OnRecordSelect()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CHANNELINDEX nChn = m_MenuCursor.GetChannel();
		if (nChn < pModDoc->GetNumChannels())
		{
			pModDoc->Record1Channel(nChn);
			InvalidateChannelsHeaders();
		}
	}
}


void CViewPattern::OnSplitRecordSelect()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CHANNELINDEX nChn = m_MenuCursor.GetChannel();
		if (nChn < pModDoc->GetNumChannels())
		{
			pModDoc->Record2Channel(nChn);
			InvalidateChannelsHeaders();
		}
	}
}


void CViewPattern::OnUnmuteAll()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		const CHANNELINDEX nChns = pModDoc->GetNumChannels();
		for(CHANNELINDEX i = 0; i < nChns; i++)
		{
			pModDoc->MuteChannel(i, false);
			pModDoc->SoloChannel(i, false);
		}
		InvalidateChannelsHeaders();
	}
}


void CViewPattern::DeleteRows(CHANNELINDEX colmin, CHANNELINDEX colmax, ROWINDEX nrows)
//-------------------------------------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern) || !IsEditingEnabled_bmsg())
	{
		return;
	}

	ROWINDEX row = m_Selection.GetStartRow();
	ROWINDEX maxrow = pSndFile->Patterns[m_nPattern].GetNumRows();

	LimitMax(colmax, CHANNELINDEX(pSndFile->GetNumChannels() - 1));
	if(colmin > colmax) return;

	PrepareUndo(PatternCursor(0), PatternCursor(maxrow - 1, pSndFile->GetNumChannels() - 1), nrows != 1 ? "Delete Rows" : "Delete Row");

	for(ROWINDEX r = row; r < maxrow; r++)
	{
		ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(r, colmin);
		for(CHANNELINDEX c = colmin; c <= colmax; c++, m++)
		{
			if (r + nrows >= maxrow)
			{
				m->Clear();
			} else
			{
				*m = *(m + pSndFile->GetNumChannels() * nrows);
			}
		}
	}
	PatternRect sel = m_Selection;
	PatternCursor finalPos(m_Selection.GetStartRow(), m_Selection.GetLowerRight());
	//SetCurSel(finalPos);
	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	SetCursorPosition(finalPos);
	SetCurSel(sel);
	SetModified();
	InvalidatePattern(false);
}


void CViewPattern::OnDeleteRows()
//-------------------------------
{
	DeleteRows(m_Selection.GetStartChannel(), m_Selection.GetEndChannel(), m_Selection.GetNumRows());
}


void CViewPattern::OnDeleteRowsEx()
//---------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	DeleteRows(0, pSndFile->GetNumChannels() - 1, m_Selection.GetNumRows());
}


void CViewPattern::InsertRows(CHANNELINDEX colmin, CHANNELINDEX colmax)
//---------------------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern) || !IsEditingEnabled_bmsg())
	{
		return;
	}

	ROWINDEX row = m_Selection.GetStartRow();
	ROWINDEX maxrow = pSndFile->Patterns[m_nPattern].GetNumRows();

	LimitMax(colmax, CHANNELINDEX(pSndFile->GetNumChannels() - 1));
	if(colmin > colmax) return;

	PrepareUndo(PatternCursor(0), PatternCursor(maxrow - 1, pSndFile->GetNumChannels() - 1), "Insert Row");

	for(ROWINDEX r = maxrow; r > row; )
	{
		r--;
		ModCommand *m = pSndFile->Patterns[m_nPattern] + r * pSndFile->GetNumChannels() + colmin;
		for(CHANNELINDEX c = colmin; c <= colmax; c++, m++)
		{
			if(r <= row)
			{
				m->Clear();
			} else
			{
				*m = *(m - pSndFile->GetNumChannels());
			}
		}
	}
	InvalidatePattern(false);
	SetModified();
}


void CViewPattern::OnInsertRows()
//-------------------------------
{
	InsertRows(m_Selection.GetStartChannel(), m_Selection.GetEndChannel());
}


void CViewPattern::OnEditGoto()
//-----------------------------
{
	CModDoc* pModDoc = GetDocument();
	if (!pModDoc)
		return;


	if (!m_pGotoWnd)
	{
		m_pGotoWnd = new CPatternGotoDialog(this);
		//if (m_pGotoWnd) m_pGotoWnd->SetParent(this/*, GetDocument()*/);
	}
	if (m_pGotoWnd)
	{
		CSoundFile &sndFile = pModDoc->GetrSoundFile();
		ORDERINDEX curOrder = GetCurrentOrder();
		CHANNELINDEX curChannel = GetCurrentChannel() + 1;

		m_pGotoWnd->UpdatePos(GetCurrentRow(), curChannel, m_nPattern, curOrder, sndFile);

		if (m_pGotoWnd->DoModal() == IDOK)
		{
			//Position should be valididated.

			if (m_pGotoWnd->m_nPattern != m_nPattern)
				SetCurrentPattern(m_pGotoWnd->m_nPattern);

			if (m_pGotoWnd->m_nOrder != curOrder)
				SetCurrentOrder( m_pGotoWnd->m_nOrder);

			if (m_pGotoWnd->m_nChannel != curChannel)
				SetCurrentColumn(m_pGotoWnd->m_nChannel - 1);

			if (m_pGotoWnd->m_nRow != GetCurrentRow())
				SetCurrentRow(m_pGotoWnd->m_nRow);

			pModDoc->SetElapsedTime(m_pGotoWnd->m_nOrder, m_pGotoWnd->m_nRow, false);
		}
	}
	return;
}


void CViewPattern::OnPatternStep()
//--------------------------------
{
	PatternStep();
}


void CViewPattern::PatternStep(ROWINDEX row)
//------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if(pMainFrm != nullptr && pModDoc != nullptr)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
			return;

		CriticalSection cs;

		// In case we were previously in smooth scrolling mode during live playback, the pattern might be misaligned.
		if(GetSmoothScrollOffset() != 0)
			InvalidatePattern(true);

		// Cut instruments/samples in virtual channels
		for(CHANNELINDEX i = pSndFile->GetNumChannels(); i < MAX_CHANNELS; i++)
		{
			pSndFile->m_PlayState.Chn[i].dwFlags.set(CHN_NOTEFADE | CHN_KEYOFF);
		}
		pSndFile->LoopPattern(m_nPattern);
		pSndFile->m_PlayState.m_nNextRow = row == ROWINDEX_INVALID ? GetCurrentRow() : row;
		pSndFile->m_SongFlags.reset(SONG_PAUSED);
		pSndFile->m_SongFlags.set(SONG_STEP);

		SetPlayCursor(m_nPattern, pSndFile->m_PlayState.m_nNextRow, 0);
		cs.Leave();

		if(pMainFrm->GetModPlaying() != pModDoc)
		{
			pModDoc->SetFollowWnd(m_hWnd);
			pMainFrm->PlayMod(pModDoc);
		}
		pModDoc->SetNotifications(Notification::Position | Notification::VUMeters);
		if(row == ROWINDEX_INVALID)
		{
			SetCurrentRow(GetCurrentRow() + 1,
				(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) ||	// Wrap around to next pattern if continous scroll is enabled...
				(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_WRAP));			// ...or otherwise if cursor wrap is enabled.
		}
		SetFocus();
	}
}


// Copy cursor to internal clipboard
void CViewPattern::OnCursorCopy()
//-------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	const ModCommand &m = GetCursorCommand();
	switch(m_Cursor.GetColumnType())
	{
	case PatternCursor::noteColumn:
	case PatternCursor::instrColumn:
		m_cmdOld.note = m.note;
		m_cmdOld.instr = m.instr;
		SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, m_cmdOld.instr);
		break;

	case PatternCursor::volumeColumn:
		m_cmdOld.volcmd = m.volcmd;
		m_cmdOld.vol = m.vol;
		break;

	case PatternCursor::effectColumn:
	case PatternCursor::paramColumn:
		m_cmdOld.command = m.command;
		m_cmdOld.param = m.param;
		break;
	}
}


// Paste cursor from internal clipboard
void CViewPattern::OnCursorPaste()
//--------------------------------
{
	if(!IsEditingEnabled_bmsg())
	{
		return;
	}

	PrepareUndo(m_Cursor, m_Cursor, "Cursor Paste");
	PatternCursor::Columns column = m_Cursor.GetColumnType();

	ModCommand &m = GetCursorCommand();

	switch(column)
	{
	case PatternCursor::noteColumn:
		m.note = m_cmdOld.note;
		MPT_FALLTHROUGH;
	case PatternCursor::instrColumn:
		m.instr = m_cmdOld.instr;
		break;

	case PatternCursor::volumeColumn:
		m.vol = m_cmdOld.vol;
		m.volcmd = m_cmdOld.volcmd;
		break;

	case PatternCursor::effectColumn:
	case PatternCursor::paramColumn:
		m.command = m_cmdOld.command;
		m.param = m_cmdOld.param;
		break;
	}

	SetModified(false);
	// Preview Row
	if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !IsLiveRecord())
	{
		PatternStep(GetCurrentRow());
	}

	if(GetSoundFile()->IsPaused() || !m_Status[psFollowSong] || (CMainFrame::GetMainFrame() && CMainFrame::GetMainFrame()->GetFollowSong(GetDocument()) != m_hWnd))
	{
		InvalidateCell(m_Cursor);
		SetCurrentRow(GetCurrentRow() + m_nSpacing);
		SetSelToCursor();
	}
}


//begin rewbs.fxvis
void CViewPattern::OnVisualizeEffect()
//------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		const ROWINDEX row0 = m_Selection.GetStartRow(), row1 = m_Selection.GetEndRow();
		const CHANNELINDEX nchn = m_Selection.GetStartChannel();
		if (m_pEffectVis)
		{
			//window already there, update data
			m_pEffectVis->UpdateSelection(row0, row1, nchn, pModDoc, m_nPattern);
		}
		else
		{
			//Open window & send data
			CriticalSection cs;

			m_pEffectVis = new CEffectVis(this, row0, row1, nchn, pModDoc, m_nPattern);
			if (m_pEffectVis)
			{
				m_pEffectVis->OpenEditor(CMainFrame::GetMainFrame());
				// HACK: to get status window set up; must create clear destinction between 
				// construction, 1st draw code and all draw code.
				m_pEffectVis->OnSize(0, 0, 0);

			}
		}
	}
}
//end rewbs.fxvis


void CViewPattern::Interpolate(PatternCursor::Columns type)
//---------------------------------------------------------
{
	CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	bool changed = false;
	std::vector<CHANNELINDEX> validChans;

	if(type == PatternCursor::effectColumn || type == PatternCursor::paramColumn)
	{
		std::vector<CHANNELINDEX> effectChans;
		std::vector<CHANNELINDEX> paramChans;
		ListChansWhereColSelected(PatternCursor::effectColumn, effectChans);
		ListChansWhereColSelected(PatternCursor::paramColumn, paramChans);

		validChans.resize(effectChans.size() + paramChans.size());
		validChans.resize(std::set_union(effectChans.begin(), effectChans.end(), paramChans.begin(), paramChans.end(), validChans.begin()) - validChans.begin());
	} else
	{
		ListChansWhereColSelected(type, validChans);
	}

	if(m_Selection.GetUpperLeft() == m_Selection.GetLowerRight() && !validChans.empty())
	{
		// No selection has been made: Interpolate between closest non-zero values in this column.
		const ModCommand *mStart = sndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel()), *mEnd = mStart;
		const ROWINDEX maxRow = sndFile->Patterns[m_nPattern].GetNumRows() - 1;
		ROWINDEX startRow = m_Selection.GetStartRow(), endRow = startRow;

		// Shortcut macro for sweeping the pattern up and down to find suitable start and end points for interpolation.
		// While startCond / endCond evaluates to true, sweeping is continued upwards / downwards.
		#define SweepPattern(startCond, endCond) \
			while(startRow >= 0 && startRow <= maxRow && (startCond)) \
				{ startRow--; mStart -= sndFile->GetNumChannels(); } \
			while(endRow <= maxRow && (endCond)) \
				{ endRow++; mEnd += sndFile->GetNumChannels(); }

		switch(type)
		{
		case PatternCursor::noteColumn:
			// Allow note-to-note interpolation only.
			SweepPattern(mStart->note == NOTE_NONE, !mStart->IsNote() || !mEnd->IsNote());
			break;
		case PatternCursor::instrColumn:
			// Allow interpolation between same instrument, as long as it's not a PC note.
			SweepPattern(mStart->instr == 0 || mStart->IsPcNote(), mEnd->instr != mStart->instr);
			break;
		case PatternCursor::volumeColumn:
			// Allow interpolation between same volume effect, as long as it's not a PC note.
			SweepPattern(mStart->volcmd == VOLCMD_NONE || mStart->IsPcNote(), mEnd->volcmd != mStart->volcmd);
			break;
		case PatternCursor::effectColumn:
		case PatternCursor::paramColumn:
			// Allow interpolation between same effect, or anything if it's a PC note.
			SweepPattern(mStart->command == CMD_NONE && !mStart->IsPcNote(), (mEnd->command != mStart->command && !mStart->IsPcNote()) || (mStart->IsPcNote() && !mEnd->IsPcNote()));
			break;
		}
		
		#undef SweepPattern

		if(startRow >= 0 && startRow < endRow && endRow <= maxRow)
		{
			// Found usable end and start commands: Extend selection.
			SetCurSel(PatternCursor(startRow, m_Selection.GetUpperLeft()), PatternCursor(endRow, m_Selection.GetUpperLeft()));
		}
	}
	
	const ROWINDEX row0 = m_Selection.GetStartRow(), row1 = m_Selection.GetEndRow();
	
	//for all channels where type is selected
	for(auto iter = validChans.begin(); iter != validChans.end(); iter++)
	{
		CHANNELINDEX nchn = *iter;
		
		if (!IsInterpolationPossible(row0, row1, nchn, type))
			continue; //skip chans where interpolation isn't possible

		if (!changed) //ensure we save undo buffer only before any channels are interpolated
		{
			const char *description = "";
			switch(type)
			{
			case PatternCursor::noteColumn:
				description = "Interpolate Note Column";
				break;
			case PatternCursor::instrColumn:
				description = "Interpolate Instrument Column";
				break;
			case PatternCursor::volumeColumn:
				description = "Interpolate Volume Column";
				break;
			case PatternCursor::effectColumn:
			case PatternCursor::paramColumn:
				description = "Interpolate Effect Column";
				break;
			}
			PrepareUndo(m_Selection, description);
		}

		bool doPCinterpolation = false;

		int vsrc, vdest, vcmd = 0, verr = 0, distance = row1 - row0;

		const ModCommand srcCmd = *sndFile->Patterns[m_nPattern].GetpModCommand(row0, nchn);
		const ModCommand destCmd = *sndFile->Patterns[m_nPattern].GetpModCommand(row1, nchn);

		ModCommand::NOTE PCnote = NOTE_NONE;
		uint16 PCinst = 0, PCparam = 0;

		switch(type)
		{
			case PatternCursor::noteColumn:
				vsrc = srcCmd.note;
				vdest = destCmd.note;
				vcmd = srcCmd.instr;
				verr = (distance * (NOTE_MAX - 1)) / NOTE_MAX;
				if(srcCmd.note == NOTE_NONE)
				{
					vsrc = vdest;
					vcmd = destCmd.note;
				} else if(destCmd.note == NOTE_NONE)
				{
					vdest = vsrc;
				}
				break;

			case PatternCursor::instrColumn:
				vsrc = srcCmd.instr;
				vdest = destCmd.instr;
				verr = (distance * 63) / 128;
				if(srcCmd.instr == 0)
				{
					vsrc = vdest;
					vcmd = destCmd.instr;
				} else if(destCmd.instr == 0)
				{
					vdest = vsrc;
				}
				break;

			case PatternCursor::volumeColumn:
				vsrc = srcCmd.vol;
				vdest = destCmd.vol;
				vcmd = srcCmd.volcmd;
				verr = (distance * 63) / 128;
				if(srcCmd.volcmd == VOLCMD_NONE)
				{
					vsrc = vdest;
					vcmd = destCmd.volcmd;
				} else if(destCmd.volcmd == VOLCMD_NONE)
				{
					vdest = vsrc;
				}
				break;

			case PatternCursor::paramColumn:
			case PatternCursor::effectColumn:
				if(srcCmd.IsPcNote() || destCmd.IsPcNote())
				{
					doPCinterpolation = true;
					PCnote = (srcCmd.IsPcNote()) ? srcCmd.note : destCmd.note;
					vsrc = srcCmd.GetValueEffectCol();
					vdest = destCmd.GetValueEffectCol();
					PCparam = srcCmd.GetValueVolCol();
					if(PCparam == 0) PCparam = destCmd.GetValueVolCol();
					PCinst = srcCmd.instr;
					if(PCinst == 0)
						PCinst = destCmd.instr;
				} else
				{
					vsrc = srcCmd.param;
					vdest = destCmd.param;
					vcmd = srcCmd.command;
					if(srcCmd.command == CMD_NONE)
					{
						vsrc = vdest;
						vcmd = destCmd.command;
					} else if(destCmd.command == CMD_NONE)
					{
						vdest = vsrc;
					}
				}
				verr = (distance * 63) / 128;
				break;

			default:
				ASSERT(false);
				return;
		}

		if (vdest < vsrc) verr = -verr;

		ModCommand* pcmd = sndFile->Patterns[m_nPattern].GetpModCommand(row0, nchn);

		for (int i = 0; i <= distance; i++, pcmd += sndFile->GetNumChannels())
		{

			switch(type)
			{
				case PatternCursor::noteColumn:
					if ((pcmd->note == NOTE_NONE) || (pcmd->instr == vcmd))
					{
						int note = vsrc + ((vdest - vsrc) * i + verr) / distance;
						pcmd->note = static_cast<ModCommand::NOTE>(note);
						pcmd->instr = static_cast<ModCommand::VOLCMD>(vcmd);
					}
					break;

				case PatternCursor::instrColumn:
					if (pcmd->instr == 0)
					{
						int instr = vsrc + ((vdest - vsrc) * i + verr) / distance;
						pcmd->instr = static_cast<ModCommand::INSTR>(instr);
					}
					break;

				case PatternCursor::volumeColumn:
					if ((pcmd->volcmd == VOLCMD_NONE) || (pcmd->volcmd == vcmd))
					{
						int vol = vsrc + ((vdest - vsrc) * i + verr) / distance;
						pcmd->vol = static_cast<ModCommand::VOL>(vol);
						pcmd->volcmd = static_cast<ModCommand::VOLCMD>(vcmd);
					}
					break;

				case PatternCursor::effectColumn:
					if(doPCinterpolation)
					{	// With PC/PCs notes, copy PCs note and plug index to all rows where
						// effect interpolation is done if no PC note with non-zero instrument is there.
						const uint16 val = static_cast<uint16>(vsrc + ((vdest - vsrc) * i + verr) / distance);
						if (pcmd->IsPcNote() == false || pcmd->instr == 0)
						{
							pcmd->note = PCnote;
							pcmd->instr = static_cast<ModCommand::INSTR>(PCinst);
						}
						pcmd->SetValueVolCol(PCparam);
						pcmd->SetValueEffectCol(val);
					}
					else
					{
						if ((pcmd->command == CMD_NONE) || (pcmd->command == vcmd))
						{
							int val = vsrc + ((vdest - vsrc) * i + verr) / distance;
							pcmd->param = static_cast<ModCommand::PARAM>(val);
							pcmd->command = static_cast<ModCommand::COMMAND>(vcmd);
						}
					}
					break;

				default:
					ASSERT(false);
			}
		}

		changed = true;

	} //end for all channels where type is selected

	if(changed)
	{
		SetModified(false);
		InvalidatePattern(false);
	}
}


void CViewPattern::OnTransposeChannel()
//-------------------------------------
{
	CInputDlg dlg(this, _T("Enter transpose amount (affects all patterns):"), -(NOTE_MAX - NOTE_MIN), (NOTE_MAX - NOTE_MIN), m_nTransposeAmount);
	if(dlg.DoModal() == IDOK)
	{
		m_nTransposeAmount = dlg.resultAsInt;

		CSoundFile &sndFile = *GetSoundFile();
		bool changed = false;
		// Don't allow notes outside our supported note range.
		const ModCommand::NOTE noteMin = sndFile.GetModSpecifications().noteMin;
		const ModCommand::NOTE noteMax = sndFile.GetModSpecifications().noteMax;

		for(PATTERNINDEX pat = 0; pat < sndFile.Patterns.Size(); pat++)
		{
			bool changedThisPat = false;
			if(sndFile.Patterns.IsValidPat(pat))
			{
				ModCommand *m = sndFile.Patterns[pat].GetpModCommand(0, m_MenuCursor.GetChannel());
				const ROWINDEX numRows = sndFile.Patterns[pat].GetNumRows();
				for(ROWINDEX row = 0; row < numRows; row++)
				{
					if(m->IsNote())
					{
						if(!changedThisPat)
						{
							GetDocument()->GetPatternUndo().PrepareUndo(pat, m_MenuCursor.GetChannel(), 0, 1, numRows, "Transpose Channel", changed);
							changed = changedThisPat = true;
						}
						int note = m->note + m_nTransposeAmount;
						Limit(note, noteMin, noteMax);
						m->note = static_cast<ModCommand::NOTE>(note);
					}
					m += sndFile.Patterns[pat].GetNumChannels();
				}
			}
		}
		if(changed)
		{
			SetModified(true);
			InvalidatePattern(false);
		}
	}
}


void CViewPattern::OnTransposeCustom()
//------------------------------------
{
	CInputDlg dlg(this, _T("Enter transpose amount:"), -(NOTE_MAX - NOTE_MIN), (NOTE_MAX - NOTE_MIN), m_nTransposeAmount);
	if(dlg.DoModal() == IDOK)
	{
		m_nTransposeAmount = dlg.resultAsInt;
		TransposeSelection(dlg.resultAsInt);
	}
}


bool CViewPattern::TransposeSelection(int transp)
//-----------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return false;
	}

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());

	const ROWINDEX startRow = m_Selection.GetStartRow(), endRow = m_Selection.GetEndRow();
	const CHANNELINDEX startChan = m_Selection.GetStartChannel() + (m_Selection.GetStartColumn() > PatternCursor::noteColumn ? 1 : 0), endChan = m_Selection.GetEndChannel();

	// Don't allow notes outside our supported note range.
	const ModCommand::NOTE noteMin = pSndFile->GetModSpecifications().noteMin;
	const ModCommand::NOTE noteMax = pSndFile->GetModSpecifications().noteMax;

	PrepareUndo(m_Selection, "Transpose");

	std::vector<int> lastGroupSize(pSndFile->GetNumChannels(), 12);
	for(ROWINDEX row = startRow; row <= endRow; row++)
	{
		PatternRow m = pSndFile->Patterns[m_nPattern].GetRow(row);
		for(CHANNELINDEX chn = startChan; chn <= endChan; chn++)
		{
			if (m[chn].IsNote())
			{
				if(m[chn].instr > 0 && m[chn].instr <= pSndFile->GetNumInstruments())
				{
					const ModInstrument *pIns = pSndFile->Instruments[m[chn].instr];
					if(pIns != nullptr && pIns->pTuning != nullptr)
					{
						lastGroupSize[chn] = pIns->pTuning->GetGroupSize();
					}
				}
				int transpose = transp;
				if(transpose == 12000 || transpose == -12000)
				{
					// Transpose one octave
					transpose = lastGroupSize[chn] * sgn(transpose);
				}
				int note = m[chn].note + transpose;
				Limit(note, noteMin, noteMax);
				m[chn].note = static_cast<ModCommand::NOTE>(note);
			}
		}
	}
	SetModified(false);
	InvalidateSelection();
	return true;
}


bool CViewPattern::DataEntry(bool up, bool coarse)
//------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return false;
	}

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());

	const ROWINDEX startRow = m_Selection.GetStartRow(), endRow = m_Selection.GetEndRow();
	const CHANNELINDEX startChan = m_Selection.GetStartChannel(), endChan = m_Selection.GetEndChannel();
	const PatternCursor::Columns column = m_Selection.GetStartColumn();

	// Don't allow notes outside our supported note range.
	const ModCommand::NOTE noteMin = pSndFile->GetModSpecifications().noteMin;
	const ModCommand::NOTE noteMax = pSndFile->GetModSpecifications().noteMax;
	const int instrMax = std::min<int>(Util::MaxValueOfType(ModCommand::INSTR()), pSndFile->GetNumInstruments() ? pSndFile->GetNumInstruments() : pSndFile->GetNumSamples());
	const EffectInfo effectInfo(*pSndFile);
	const int offset = up ? 1 : -1;

	PrepareUndo(m_Selection, "Data Entry");

	// Notes per octave for non-TET12 tunings and coarse note steps
	std::vector<int> lastGroupSize(pSndFile->GetNumChannels(), 12);
	for(ROWINDEX row = startRow; row <= endRow; row++)
	{
		PatternRow m = pSndFile->Patterns[m_nPattern].GetRow(row);
		for(CHANNELINDEX chn = startChan; chn <= endChan; chn++)
		{
			if(column == PatternCursor::noteColumn && m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::noteColumn)))
			{
				// Increase / decrease note
				if(m[chn].IsNote())
				{
					if(m[chn].instr > 0 && m[chn].instr <= pSndFile->GetNumInstruments())
					{
						lastGroupSize[chn] = 12;
						const ModInstrument *pIns = pSndFile->Instruments[m[chn].instr];
						if(pIns != nullptr && pIns->pTuning != nullptr)
						{
							lastGroupSize[chn] = pIns->pTuning->GetGroupSize();
						}
					}
					int note = m[chn].note + offset * (coarse ? lastGroupSize[chn] : 1);
					Limit(note, noteMin, noteMax);
					m[chn].note = (ModCommand::NOTE)note;
				} else if(m[chn].IsSpecialNote())
				{
					ModCommand::NOTE note = m[chn].note;
					do
					{
						note = static_cast<ModCommand::NOTE>(note + offset);
						if(!ModCommand::IsSpecialNote(note))
						{
							break;
						}
					} while(!pSndFile->GetModSpecifications().HasNote(note));
					if(ModCommand::IsSpecialNote(note))
					{
						if(m[chn].IsPcNote() != ModCommand::IsPcNote(note))
						{
							m[chn].Clear();
						}
						m[chn].note = (ModCommand::NOTE)note;
					}
				}
			}
			if(column == PatternCursor::instrColumn && m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::instrColumn)) && m[chn].instr != 0)
			{
				// Increase / decrease instrument
				int instr = m[chn].instr + offset * (coarse ? 10 : 1);
				Limit(instr, 1, m[chn].IsInstrPlug() ? MAX_MIXPLUGINS : instrMax);
				m[chn].instr = (ModCommand::INSTR)instr;
			}
			if(column == PatternCursor::volumeColumn && m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::volumeColumn)))
			{
				// Increase / decrease volume parameter
				if(m[chn].IsPcNote())
				{
					int val = m[chn].GetValueVolCol() + offset * (coarse ? 10 : 1);
					Limit(val, 0, int(ModCommand::maxColumnValue));
					m[chn].SetValueVolCol(static_cast<uint16>(val));
				} else
				{
					int vol = m[chn].vol + offset * (coarse ? 10 : 1);
					if(m[chn].volcmd == VOLCMD_NONE && m[chn].IsNote() && m[chn].instr)
					{
						m[chn].volcmd = VOLCMD_VOLUME;
						vol = GetDefaultVolume(m[chn]);
					}
					ModCommand::VOL minValue = 0, maxValue = 64;
					effectInfo.GetVolCmdInfo(effectInfo.GetIndexFromVolCmd(m[chn].volcmd), nullptr, &minValue, &maxValue);
					Limit(vol, (int)minValue, (int)maxValue);
					m[chn].vol = (ModCommand::VOL)vol;
				}
			}
			if((column == PatternCursor::effectColumn || column == PatternCursor::paramColumn) && (m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::effectColumn)) || m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::paramColumn))))
			{
				// Increase / decrease effect parameter
				if(m[chn].IsPcNote())
				{
					int val = m[chn].GetValueEffectCol() + offset * (coarse ? 10 : 1);
					Limit(val, 0, int(ModCommand::maxColumnValue));
					m[chn].SetValueEffectCol(static_cast<uint16>(val));
				} else
				{
					int param = m[chn].param + offset * (coarse ? 16 : 1);
					ModCommand::PARAM minValue = 0, maxValue = 0xFF;
					effectInfo.GetEffectInfo(effectInfo.GetIndexFromEffect(m[chn].command, m[chn].param), nullptr, false, &minValue, &maxValue);
					Limit(param, (int)minValue, (int)maxValue);
					m[chn].param = (ModCommand::PARAM)param;
				}
			}
		}
	}

	SetModified(false);
	InvalidatePattern();
	return true;
}


// Get the velocity at which a given note would be played
int CViewPattern::GetDefaultVolume(const ModCommand &m, ModCommand::INSTR lastInstr) const
//----------------------------------------------------------------------------------------
{
	const CSoundFile &sndFile = *GetSoundFile();
	SAMPLEINDEX sample = GetDocument()->GetSampleIndex(m, lastInstr);
	if(sample)
		return sndFile.GetSample(sample).nVolume / 4;
	else if(m.instr > 0 && m.instr <= sndFile.GetNumInstruments() && sndFile.Instruments[m.instr] != nullptr && sndFile.Instruments[m.instr]->HasValidMIDIChannel())
		return sndFile.Instruments[m.instr]->nGlobalVol;	// For instrument plugins
	else
		return 64;
}


void CViewPattern::OnDropSelection()
//----------------------------------
{
	CModDoc *pModDoc;
	if((pModDoc = GetDocument()) == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	// Compute relative movement
	int dx = (int)m_DragPos.GetChannel() - (int)m_StartSel.GetChannel();
	int dy = (int)m_DragPos.GetRow() - (int)m_StartSel.GetRow();
	if((!dx) && (!dy))
	{
		return;
	}

	// Allocate replacement pattern
	ModCommand *pNewPattern = CPattern::AllocatePattern(sndFile.Patterns[m_nPattern].GetNumRows(), sndFile.GetNumChannels());
	if(pNewPattern == nullptr)
	{
		return;
	}
	// Compute destination rect
	PatternCursor begin(m_Selection.GetUpperLeft()), end(m_Selection.GetLowerRight());
	begin.Move(dy, dx, 0);
	if(begin.GetChannel() >= sndFile.GetNumChannels())
	{
		// Moved outside pattern range.
		return;
	}
	end.Move(dy, dx, 0);
	if(end.GetColumnType() == PatternCursor::effectColumn)
	{
		// Extend to parameter column
		end.Move(0, 0, 1);
	}
	begin.Sanitize(sndFile.Patterns[m_nPattern].GetNumRows(), sndFile.GetNumChannels());
	end.Sanitize(sndFile.Patterns[m_nPattern].GetNumRows(), sndFile.GetNumChannels());
	PatternRect destination(begin, end);

	const bool moveSelection = !m_Status[psKeyboardDragSelect | psCtrlDragSelect];

	BeginWaitCursor();
	pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, 0, 0, sndFile.GetNumChannels(), sndFile.Patterns[m_nPattern].GetNumRows(), moveSelection ? "Move Selection" : "Copy Selection");

	ModCommand *p = pNewPattern;
	for(ROWINDEX row = 0; row < sndFile.Patterns[m_nPattern].GetNumRows(); row++)
	{
		for(CHANNELINDEX chn = 0; chn < sndFile.GetNumChannels(); chn++, p++)
		{
			for (int c = PatternCursor::firstColumn; c <= PatternCursor::lastColumn; c++)
			{
				PatternCursor cell(row, chn, static_cast<PatternCursor::Columns>(c));
				int xsrc = chn, ysrc = row;

				if(destination.Contains(cell))
				{
					// Current cell is from destination selection
					xsrc -= dx;
					ysrc -= dy;
				} else if(m_Selection.Contains(cell))
				{
					// Current cell is from source rectangle (clear)
					if(moveSelection)
					{
						xsrc = -1;
					}
				}

				if(xsrc >= 0 && xsrc < (int)sndFile.GetNumChannels() && ysrc >= 0 && ysrc < (int)sndFile.Patterns[m_nPattern].GetNumRows())
				{
					// Copy the data
					const ModCommand &src = *sndFile.Patterns[m_nPattern].GetpModCommand(static_cast<ROWINDEX>(ysrc), static_cast<CHANNELINDEX>(xsrc));
					switch(c)
					{
					case PatternCursor::noteColumn:
						p->note = src.note;
						break;
					case PatternCursor::instrColumn:
						p->instr = src.instr;
						break;
					case PatternCursor::volumeColumn:
						p->vol = src.vol;
						p->volcmd = src.volcmd;
						break;
					case PatternCursor::effectColumn:
						p->command = src.command;
						p->param = src.param;
						break;
					}
				}
			}
		}
	}
	
	CriticalSection cs;
	CPattern::FreePattern(sndFile.Patterns[m_nPattern]);
	sndFile.Patterns[m_nPattern] = pNewPattern;
	cs.Leave();

	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	SetCursorPosition(begin);
	SetCurSel(destination);
	InvalidatePattern();
	SetModified(false);
	EndWaitCursor();
}


void CViewPattern::OnSetSelInstrument()
//-------------------------------------
{
	SetSelectionInstrument(static_cast<INSTRUMENTINDEX>(GetCurrentInstrument()));
}


void CViewPattern::OnRemoveChannelDialog()
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;
	pModDoc->ChangeNumChannels(0);
	SetCurrentPattern(m_nPattern); //Updating the screen.
}


void CViewPattern::OnRemoveChannel()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	if(pModDoc == nullptr || (pSndFile = pModDoc->GetSoundFile()) == nullptr) return;

	if(pSndFile->GetNumChannels() <= pSndFile->GetModSpecifications().channelsMin)
	{
		Reporting::Error("No channel removed - channel number already at minimum.", "Remove channel");
		return;
	}

	CHANNELINDEX nChn = m_MenuCursor.GetChannel();
	const bool isEmpty = pModDoc->IsChannelUnused(nChn);

	CString str;
	str.Format("Remove channel %d? This channel still contains note data!", nChn + 1);
	if(isEmpty || Reporting::Confirm(str , "Remove channel") == cnfYes)
	{
		std::vector<bool> keepMask(pModDoc->GetNumChannels(), true);
		keepMask[nChn] = false;
		pModDoc->RemoveChannels(keepMask, true);
		SetCurrentPattern(m_nPattern); //Updating the screen.
	}
}


void CViewPattern::AddChannelBefore(CHANNELINDEX nBefore)
//-------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;

	BeginWaitCursor();
	// Create new channel order, with channel nBefore being an invalid (and thus empty) channel.
	std::vector<CHANNELINDEX> channels(pModDoc->GetNumChannels() + 1, CHANNELINDEX_INVALID);
	CHANNELINDEX i = 0;
	for(CHANNELINDEX nChn = 0; nChn < pModDoc->GetNumChannels() + 1; nChn++)
	{
		if(nChn != nBefore)
		{
			channels[nChn] = i++;
		}
	}

	if (pModDoc->ReArrangeChannels(channels) != CHANNELINDEX_INVALID)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, GeneralHint().General().Channels().ModType()); //refresh channel headers
		SetCurrentPattern(m_nPattern);
	}
	EndWaitCursor();
}


void CViewPattern::OnDuplicateChannel()
//------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc == nullptr) return;

	const CHANNELINDEX dupChn = m_MenuCursor.GetChannel();
	if(dupChn >= pModDoc->GetNumChannels())
		return;

	if(!pModDoc->IsChannelUnused(dupChn) && Reporting::Confirm(_T("This affects all patterns, proceed?"), _T("Duplicate Channel")) != cnfYes)
		return;

	BeginWaitCursor();
	// Create new channel order, with channel nDupChn duplicated.
	std::vector<CHANNELINDEX> channels(pModDoc->GetNumChannels() + 1, 0);
	CHANNELINDEX i = 0;
	for(CHANNELINDEX nChn = 0; nChn < pModDoc->GetNumChannels() + 1; nChn++)
	{
		channels[nChn] = i;
		if(nChn != dupChn)
			i++;
	}

	// Check that duplication happened and in that case update.
	if(pModDoc->ReArrangeChannels(channels) != CHANNELINDEX_INVALID)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(nullptr, GeneralHint().General().Channels().ModType()); //refresh channel headers
		SetCurrentPattern(m_nPattern);
	}
	EndWaitCursor();
}


void CViewPattern::OnRunScript()
//------------------------------
{
	;
}



void CViewPattern::OnSwitchToOrderList()
//--------------------------------------
{
	PostCtrlMessage(CTRLMSG_SETFOCUS);
}


void CViewPattern::OnPrevOrder()
//------------------------------
{
	PostCtrlMessage(CTRLMSG_PREVORDER);
}


void CViewPattern::OnNextOrder()
//------------------------------
{
	PostCtrlMessage(CTRLMSG_NEXTORDER);
}


void CViewPattern::OnUpdateUndo(CCmdUI *pCmdUI)
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetPatternUndo().CanUndo());
		pCmdUI->SetText(CString("Undo ") + CString(pModDoc->GetPatternUndo().GetUndoName())
			+ CString("\t") + CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditUndo));
	}
}


void CViewPattern::OnUpdateRedo(CCmdUI *pCmdUI)
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetPatternUndo().CanRedo());
		pCmdUI->SetText(CString("Redo ") + CString(pModDoc->GetPatternUndo().GetRedoName())
			+ CString("\t") + CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditRedo));
	}
}


void CViewPattern::OnEditUndo()
//-----------------------------
{
	UndoRedo(true);
}


void CViewPattern::OnEditRedo()
//-----------------------------
{
	UndoRedo(false);
}


void CViewPattern::UndoRedo(bool undo)
//------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc && IsEditingEnabled_bmsg())
	{
		PATTERNINDEX pat = undo ? pModDoc->GetPatternUndo().Undo() : pModDoc->GetPatternUndo().Redo();
		if(pat < pModDoc->GetSoundFile()->Patterns.Size())
		{
			if(pat != m_nPattern)
			{
				// Find pattern in sequence.
				ORDERINDEX matchingOrder = GetSoundFile()->Order.FindOrder(pat, GetCurrentOrder());
				if(matchingOrder != ORDERINDEX_INVALID)
				{
					SetCurrentOrder(matchingOrder);
				}
				SetCurrentPattern(pat);
			} else
			{
				InvalidatePattern(true);
			}
			SetModified(false);
			SanitizeCursor();
			UpdateScrollSize();
		}
	}
}


// Apply amplification and fade function to volume
static void AmplifyFade(int &vol, int amp, ROWINDEX row, ROWINDEX numRows, bool fadeIn, bool fadeOut, Fade::Func &fadeFunc)
//-------------------------------------------------------------------------------------------------------------------------
{
	vol *= amp;
	if(fadeIn && fadeOut)
	{
		ROWINDEX numRows2 = numRows / 2;
		if(row < numRows2)
			vol = fadeFunc(vol, row + 1, numRows2);
		else
			vol = fadeFunc(vol, numRows - row, numRows - numRows2);
	} else if(fadeIn)
	{
		vol = fadeFunc(vol, row + 1, numRows);
	} else if(fadeOut)
	{
		vol = fadeFunc(vol, numRows - row, numRows);
	}
	vol = (vol + 50) / 100;
	LimitMax(vol, 64);
}


void CViewPattern::OnPatternAmplify()
//-----------------------------------
{
	static int16 oldAmp = 100;
	static Fade::Law fadeLaw = Fade::kLinear;
	CAmpDlg dlg(this, oldAmp, fadeLaw, 0);
	if(dlg.DoModal() != IDOK)
	{
		return;
	}

	CSoundFile *pSndFile = GetSoundFile();

	const bool useVolCol = pSndFile->GetModSpecifications().HasVolCommand(VOLCMD_VOLUME);

	BeginWaitCursor();
	PrepareUndo(m_Selection, "Amplify");
	oldAmp = dlg.m_nFactor;
	fadeLaw = dlg.m_fadeLaw;
	Fade::Func fadeFunc = GetFadeFunc(fadeLaw);

	if(pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
		CHANNELINDEX firstChannel = m_Selection.GetStartChannel(), lastChannel = m_Selection.GetEndChannel();
		ROWINDEX firstRow = m_Selection.GetStartRow(), lastRow = m_Selection.GetEndRow();

		// For partically selected start and end channels, we check if the start and end columns contain the relevant columns.
		bool firstChannelValid, lastChannelValid;
		if(useVolCol)
		{
			// Volume column
			firstChannelValid = m_Selection.ContainsHorizontal(PatternCursor(0, firstChannel, PatternCursor::volumeColumn));
			lastChannelValid = m_Selection.ContainsHorizontal(PatternCursor(0, lastChannel, PatternCursor::volumeColumn));
		} else
		{
			// Effect column
			firstChannelValid = true;	// We cannot start "too far right" in the channel, since this is the last column.
			lastChannelValid = m_Selection.GetLowerRight().CompareColumn(PatternCursor(0, lastChannel, PatternCursor::effectColumn)) >= 0;
		}

		// adjust min/max channel if they're only partly selected (i.e. volume column or effect column (when using .MOD) is not covered)
		// XXX if only the effect column is marked in the XM format, we cannot amplify volume commands there. Does anyone use that?
		if(!firstChannelValid)
		{
			if(firstChannel >= lastChannel)
			{
				// Selection too small!
				EndWaitCursor();
				return;
			}
			firstChannel++;
		}
		if(!lastChannelValid)
		{
			if(lastChannel <= firstChannel)
			{
				// Selection too small!
				EndWaitCursor();
				return;
			}
			lastChannel--;
		}

		// Volume memory for each channel.
		std::vector<uint8> chvol(lastChannel + 1, 64);

		for(ROWINDEX nRow = firstRow; nRow <= lastRow; nRow++)
		{
			ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(nRow, firstChannel);
			for(CHANNELINDEX nChn = firstChannel; nChn <= lastChannel; nChn++, m++)
			{
				if(m->command == CMD_VOLUME && m->param <= 64)
				{
					chvol[nChn] = m->param;
					break;
				}
				if(m->volcmd == VOLCMD_VOLUME)
				{
					chvol[nChn] = m->vol;
					break;
				}
				if(m->IsNote() && m->instr != 0)
				{
					SAMPLEINDEX nSmp = m->instr;
					if(pSndFile->GetNumInstruments())
					{
						if(nSmp <= pSndFile->GetNumInstruments() && pSndFile->Instruments[nSmp] != nullptr)
						{
							nSmp = pSndFile->Instruments[nSmp]->Keyboard[m->note];
							if(!nSmp) chvol[nChn] = 64;	// hack for instruments without samples
						} else
						{
							nSmp = 0;
						}
					}
					if(nSmp > 0 && nSmp <= pSndFile->GetNumSamples())
					{
						chvol[nChn] = (uint8)(pSndFile->GetSample(nSmp).nVolume / 4);
						break;
					} else
					{
						//nonexistant sample and no volume present in patten? assume volume=64.
						if(useVolCol)
						{
							m->volcmd = VOLCMD_VOLUME;
							m->vol = 64;
						} else
						{
							m->command = CMD_VOLUME;
							m->param = 64;
						}
						chvol[nChn] = 64;
						break;
					}
				}
			}
		}

		for(ROWINDEX nRow = firstRow; nRow <= lastRow; nRow++)
		{
			ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(nRow, firstChannel);
			const int cy = lastRow - firstRow + 1; // total rows (for fading)
			for(CHANNELINDEX nChn = firstChannel; nChn <= lastChannel; nChn++, m++)
			{
				if(m->volcmd == VOLCMD_NONE && m->command != CMD_VOLUME
					&& m->IsNote() && m->instr)
				{
					SAMPLEINDEX nSmp = m->instr;
					bool overrideSampleVol = false;
					if(pSndFile->GetNumInstruments())
					{
						if(nSmp <= pSndFile->GetNumInstruments() && pSndFile->Instruments[nSmp] != nullptr)
						{
							nSmp = pSndFile->Instruments[nSmp]->Keyboard[m->note];
							// hack for instruments without samples
							if(!nSmp)
							{
								nSmp = 1;
								overrideSampleVol = true;
							}
						} else
						{
							nSmp = 0;
						}
					}
					if(nSmp > 0 && (nSmp <= pSndFile->GetNumSamples()))
					{
						if(useVolCol)
						{
							m->volcmd = VOLCMD_VOLUME;
							m->vol = static_cast<ModCommand::VOL>((overrideSampleVol) ? 64 : pSndFile->GetSample(nSmp).nVolume / 4u);
						} else
						{
							m->command = CMD_VOLUME;
							m->param = static_cast<ModCommand::PARAM>((overrideSampleVol) ? 64 : pSndFile->GetSample(nSmp).nVolume / 4u);
						}
					}
				}

				if(m->volcmd == VOLCMD_VOLUME) chvol[nChn] = m->vol;

				if((dlg.m_bFadeIn || dlg.m_bFadeOut) && m->command != CMD_VOLUME && m->volcmd == VOLCMD_NONE)
				{
					if(useVolCol)
					{
						m->volcmd = VOLCMD_VOLUME;
						m->vol = chvol[nChn];
					} else
					{
						m->command = CMD_VOLUME;
						m->param = chvol[nChn];
					}
				}

				if(m->volcmd == VOLCMD_VOLUME)
				{
					int vol = m->vol;
					AmplifyFade(vol, dlg.m_nFactor, nRow - firstRow, cy, dlg.m_bFadeIn, dlg.m_bFadeOut, fadeFunc);
					m->vol = static_cast<ModCommand::VOL>(vol);
				}

				if(m_Selection.ContainsHorizontal(PatternCursor(0, nChn, PatternCursor::effectColumn)) || m_Selection.ContainsHorizontal(PatternCursor(0, nChn, PatternCursor::paramColumn)))
				{
					if(m->command == CMD_VOLUME && m->param <= 64)
					{
						int vol = m->param;
						AmplifyFade(vol, dlg.m_nFactor, nRow - firstRow, cy, dlg.m_bFadeIn, dlg.m_bFadeOut, fadeFunc);
						m->param = static_cast<ModCommand::PARAM>(vol);
					}
				}
			}
		}
	}
	SetModified(false);
	InvalidateSelection();
	EndWaitCursor();
}


LRESULT CViewPattern::OnPlayerNotify(Notification *pnotify)
//---------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || pnotify == nullptr)
	{
		return 0;
	}

	if(pnotify->type[Notification::Position])
	{
		ORDERINDEX nOrd = pnotify->order;
		ROWINDEX nRow = pnotify->row;
		PATTERNINDEX nPat = pnotify->pattern; //get player pattern
		bool updateOrderList = false;

		if(m_nLastPlayedOrder != nOrd)
		{
			updateOrderList = true;
			m_nLastPlayedOrder = nOrd;
		}

		if (nRow < m_nLastPlayedRow)
		{
			InvalidateChannelsHeaders();
		}
		m_nLastPlayedRow = nRow;

		if(!pSndFile->m_SongFlags[SONG_PAUSED | SONG_STEP])
		{
			if (nOrd >= pSndFile->Order.GetLength() || pSndFile->Order[nOrd] != nPat)
			{
				//order doesn't correlate with pattern, so mark it as invalid
				nOrd = ORDERINDEX_INVALID;
			}

			if (m_pEffectVis && m_pEffectVis->m_hWnd)
			{
				m_pEffectVis->SetPlayCursor(nPat, nRow);
			}

			// Simple detection of backwards-going patterns to avoid jerky animation
			m_nNextPlayRow = ROWINDEX_INVALID;
			if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SMOOTHSCROLL) && pSndFile->Patterns.IsValidPat(nPat) && pSndFile->Patterns[nPat].IsValidRow(nRow))
			{
				for(const ModCommand *m = pSndFile->Patterns[nPat].GetRow(nRow), *mEnd = m + pSndFile->GetNumChannels(); m != mEnd; m++)
				{
					if(m->command == CMD_PATTERNBREAK) m_nNextPlayRow = m->param;
					else if(m->command == CMD_POSITIONJUMP && (m_nNextPlayRow == ROWINDEX_INVALID || pSndFile->GetType() == MOD_TYPE_XM)) m_nNextPlayRow = 0;
				}
			}
			if(m_nNextPlayRow == ROWINDEX_INVALID) m_nNextPlayRow = nRow + 1;

			m_nTicksOnRow = pnotify->ticksOnRow;
			SetPlayCursor(nPat, nRow, pnotify->tick);
			// Don't follow song if user drags selections or scrollbars.
			if((m_Status & (psFollowSong | psDragActive)) == psFollowSong)
			{
				if (nPat < pSndFile->Patterns.Size())
				{
					if (nPat != m_nPattern || updateOrderList)
					{
						if(nPat != m_nPattern)
							SetCurrentPattern(nPat, nRow);
						else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SHOWPREVIOUS)
							InvalidatePattern(true);	// Redraw previous / next pattern

						if (nOrd < pSndFile->Order.size())
						{
							m_nOrder = nOrd;
							SendCtrlMessage(CTRLMSG_NOTIFYCURRENTORDER, nOrd);
						}
						updateOrderList = false;
					}
					if (nRow != GetCurrentRow())
					{
						SetCurrentRow((nRow < pSndFile->Patterns[nPat].GetNumRows()) ? nRow : 0, false, false);
					}
				}
			} else
			{
				if(updateOrderList)
				{
					SendCtrlMessage(CTRLMSG_FORCEREFRESH); //force orderlist refresh
					updateOrderList = false;
				}
			}
		}
	}

	if(pnotify->type[Notification::VUMeters | Notification::Stop] && m_Status[psShowVUMeters])
	{
		UpdateAllVUMeters(pnotify);
	}

	if(pnotify->type[Notification::Stop])
	{
		m_baPlayingNote.reset();
		MemsetZero(ChnVUMeters);	// Also zero all non-visible VU meters
		SetPlayCursor(PATTERNINDEX_INVALID, ROWINDEX_INVALID, 0);
	}

	UpdateIndicator();

	return 0;


}

// record plugin parameter changes into current pattern
LRESULT CViewPattern::OnRecordPlugParamChange(WPARAM plugSlot, LPARAM paramIndex)
//-------------------------------------------------------------------------------
{	
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr || !IsEditingEnabled())
	{
		return 0;
	}
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr)
	{
		return 0;
	}

	//Work out where to put the new data
	const CHANNELINDEX nChn = GetCurrentChannel();
	ROWINDEX nRow = GetCurrentRow();
	PATTERNINDEX nPattern = m_nPattern;
	if(IsLiveRecord())
		SetEditPos(*pSndFile, nRow, nPattern, pSndFile->m_PlayState.m_nRow, pSndFile->m_PlayState.m_nPattern);

	ModCommand *pRow = pSndFile->Patterns[nPattern].GetpModCommand(nRow, nChn);

	// TODO: Is the right plugin active? Move to a chan with the right plug
	// Probably won't do this - finish fluctuator implementation instead.

	IMixPlugin *pPlug = pSndFile->m_MixPlugins[plugSlot].pMixPlugin;
	if (pPlug == nullptr) return 0;

	if(pSndFile->GetType() == MOD_TYPE_MPT)
	{
		// MPTM: Use PC Notes

		// only overwrite existing PC Notes
		if(pRow->IsEmpty() || pRow->IsPcNote())
		{
			pModDoc->GetPatternUndo().PrepareUndo(nPattern, nChn, nRow, 1, 1, "Automation Entry");

			pRow->Set(NOTE_PCS, static_cast<ModCommand::INSTR>(plugSlot + 1), static_cast<uint16>(paramIndex), static_cast<uint16>(pPlug->GetParameter(paramIndex) * ModCommand::maxColumnValue));
			InvalidateRow(nRow);
		}
	} else if(pSndFile->GetModSpecifications().HasCommand(CMD_SMOOTHMIDI))
	{
		// Other formats: Use MIDI macros

		//Figure out which plug param (if any) is controllable using the active macro on this channel.
		int activePlugParam  = -1;
		BYTE activeMacro      = pSndFile->m_PlayState.Chn[nChn].nActiveMacro;

		if (pSndFile->m_MidiCfg.GetParameteredMacroType(activeMacro) == sfx_plug)
		{
			activePlugParam = pSndFile->m_MidiCfg.MacroToPlugParam(activeMacro);
		}

		//If the wrong macro is active, see if we can find the right one.
		//If we can, activate it for this chan by writing appropriate SFx command it.
		if (activePlugParam != paramIndex)
		{ 
			int foundMacro = pSndFile->m_MidiCfg.FindMacroForParam(paramIndex);
			if (foundMacro >= 0)
			{
				pSndFile->m_PlayState.Chn[nChn].nActiveMacro = static_cast<uint8>(foundMacro);
				if (pRow->command == CMD_NONE || pRow->command == CMD_SMOOTHMIDI || pRow->command == CMD_MIDI) //we overwrite existing Zxx and \xx only.
				{
					pModDoc->GetPatternUndo().PrepareUndo(nPattern, nChn, nRow, 1, 1, "Automation Entry");

					pRow->command = CMD_S3MCMDEX;
					if(!pSndFile->GetModSpecifications().HasCommand(CMD_S3MCMDEX)) pRow->command = CMD_MODCMDEX;
					pRow->param = 0xF0 + (foundMacro & 0x0F);
					InvalidateRow(nRow);
				}

			}
		}

		// Write the data, but we only overwrite if the command is a macro anyway.
		if(pRow->command == CMD_NONE || pRow->command == CMD_SMOOTHMIDI || pRow->command == CMD_MIDI)
		{
			pModDoc->GetPatternUndo().PrepareUndo(nPattern, nChn, nRow, 1, 1, "Automation Entry");

			pRow->command = CMD_SMOOTHMIDI;
			PlugParamValue param = pPlug->GetParameter(paramIndex);
			Limit(param, 0.0f, 1.0f);
			pRow->param = static_cast<ModCommand::PARAM>(param * 127.0f);
			InvalidateRow(nRow);
		}

	}

	return 0;
}


ModCommandPos CViewPattern::GetEditPos(CSoundFile& rSf, const bool bLiveRecord) const
//-----------------------------------------------------------------------------------
{
	ModCommandPos editpos;
	if(bLiveRecord)
	{
		if(m_nPlayPat != PATTERNINDEX_INVALID)
			SetEditPos(rSf, editpos.row, editpos.pattern, m_nPlayRow, m_nPlayPat);
		else
			SetEditPos(rSf, editpos.row, editpos.pattern, rSf.m_PlayState.m_nRow, rSf.m_PlayState.m_nPattern);
	} else
	{
		editpos.pattern = m_nPattern;
		editpos.row = GetCurrentRow();
	}
	editpos.channel = GetCurrentChannel();

	return editpos;
}


// Return ModCommand at the given cursor position of the current pattern.
// If the position is not valid, a pointer to a dummy command is returned.
ModCommand &CViewPattern::GetModCommand(PatternCursor cursor)
//-----------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(GetCurrentPattern() && pSndFile->Patterns[GetCurrentPattern()].IsValidRow(cursor.GetRow())))
	{
		return *pSndFile->Patterns[GetCurrentPattern()].GetpModCommand(cursor.GetRow(), cursor.GetChannel());
	}
	// Failed.
	static ModCommand dummy;
	return dummy;
}


// Sanitize cursor so that it can't point to an invalid position in the current pattern.
void CViewPattern::SanitizeCursor()
//---------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(GetCurrentPattern()))
	{
		m_Cursor.Sanitize(GetSoundFile()->Patterns[m_nPattern].GetNumRows(), GetSoundFile()->Patterns[m_nPattern].GetNumChannels());
	}
};


// Returns pointer to modcommand at given position.
// If the position is not valid, a pointer to a dummy command is returned.
ModCommand &CViewPattern::GetModCommand(CSoundFile &sndFile, const ModCommandPos &pos)
//------------------------------------------------------------------------------------
{
	static ModCommand dummy;
	if(sndFile.Patterns.IsValidPat(pos.pattern) && pos.row < sndFile.Patterns[pos.pattern].GetNumRows() && pos.channel < sndFile.GetNumChannels())
		return *sndFile.Patterns[pos.pattern].GetpModCommand(pos.row, pos.channel);
	else
		return dummy;
}


LRESULT CViewPattern::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
//-------------------------------------------------------------
{
	const DWORD dwMidiData = dwMidiDataParam;
	static BYTE midivolume = 127;

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if(pModDoc == nullptr || pMainFrm == nullptr) return 0;

	CSoundFile &sndFile = pModDoc->GetrSoundFile();

//Midi message from our perspective:
//     +---------------------------+---------------------------+-------------+-------------+
//bit: | 24.23.22.21 | 20.19.18.17 | 16.15.14.13 | 12.11.10.09 | 08.07.06.05 | 04.03.02.01 |
//     +---------------------------+---------------------------+-------------+-------------+
//     |     Velocity (0-127)      |  Note (middle C is 60)    |   Event     |   Channel   |
//     +---------------------------+---------------------------+-------------+-------------+
//(http://home.roadrunner.com/~jgglatt/tech/midispec.htm)

	//Notes:
	//. Initial midi data handling is done in MidiInCallBack().
	//. If no event is received, previous event is assumed.
	//. A note-on (event=9) with velocity 0 is equivalent to a note off.
	//. Basing the event solely on the velocity as follows is incorrect,
	//  since a note-off can have a velocity too:
	//  BYTE event  = (dwMidiData>>16) & 0x64;
	//. Sample- and instrumentview handle midi mesages in their own methods.

	const uint8 nByte1 = MIDIEvents::GetDataByte1FromEvent(dwMidiData);
	const uint8 nByte2 = MIDIEvents::GetDataByte2FromEvent(dwMidiData);

	const uint8 nNote = nByte1 + NOTE_MIN;
	int nVol = nByte2;							// At this stage nVol is a non linear value in [0;127]
												// Need to convert to linear in [0;64] - see below
	MIDIEvents::EventType event = MIDIEvents::GetTypeFromEvent(dwMidiData);
	uint8 channel = MIDIEvents::GetChannelFromEvent(dwMidiData);

	if ((event == MIDIEvents::evNoteOn) && !nVol) event = MIDIEvents::evNoteOff;	//Convert event to note-off if req'd


	// Handle MIDI mapping.
	PLUGINDEX mappedIndex = uint8_max;
	PlugParamIndex paramIndex = 0;
	uint16 paramValue = uint16_max;
	bool captured = sndFile.GetMIDIMapper().OnMIDImsg(dwMidiData, mappedIndex, paramIndex, paramValue);


	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetInputHandler();
	if(ih->HandleMIDIMessage(static_cast<InputTargetContext>(kCtxViewPatterns + 1 + m_Cursor.GetColumnType()), dwMidiData) != kcNull
		|| ih->HandleMIDIMessage(kCtxAllContexts, dwMidiData) != kcNull)
	{
		// Mapped to a command, no need to pass message on.
		captured = true;
	}

	// Write parameter control commands if needed.
	if(paramValue != uint16_max && IsEditingEnabled() && sndFile.GetType() == MOD_TYPE_MPT)
	{
		const bool liveRecord = IsLiveRecord();

		ModCommandPos editpos = GetEditPos(sndFile, liveRecord);
		ModCommand &m = GetModCommand(sndFile, editpos);
		pModDoc->GetPatternUndo().PrepareUndo(editpos.pattern, editpos.channel, editpos.row, 1, 1, "MIDI Mapping Record");
		m.Set(NOTE_PCS, mappedIndex, static_cast<uint16>(paramIndex), static_cast<uint16>((paramValue * ModCommand::maxColumnValue) / 16383));
		if(!liveRecord)
			InvalidateRow(editpos.row);
		pMainFrm->ThreadSafeSetModified(pModDoc);
		pModDoc->UpdateAllViews(this, PatternHint(editpos.pattern).Data(), this);
	}

	if(captured)
	{
		// Event captured by MIDI mapping or shortcut, no need to pass message on.
		return 1;
	}

	bool recordParamAsZxx = false;

	switch(event)
	{
	case MIDIEvents::evNoteOff: // Note Off
		// The following method takes care of:
		// . Silencing specific active notes (just setting nNote to 255 as was done before is not acceptible)
		// . Entering a note off in pattern if required
		TempStopNote(nNote, ((TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_RECORDNOTEOFF) != 0));
		break;

	case MIDIEvents::evNoteOn: // Note On
		// continue playing as soon as MIDI notes are being received (http://forum.openmpt.org/index.php?topic=2813.0)
		if(!IsLiveRecord() && (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN))
			pModDoc->OnPatternPlayNoLoop();

		nVol = CMainFrame::ApplyVolumeRelatedSettings(dwMidiData, midivolume);
		if(nVol < 0) nVol = -1;
		else nVol = (nVol + 3) / 4; //Value from [0,256] to [0,64]
		TempEnterNote(nNote, nVol, true);
		break;

	case MIDIEvents::evPolyAftertouch:	// Polyphonic aftertouch
		EnterAftertouch(nNote, nVol);
		break;

	case MIDIEvents::evChannelAftertouch:	// Channel aftertouch
		EnterAftertouch(NOTE_NONE, nByte1);
		break;

	case MIDIEvents::evPitchBend:	// Pitch wheel
		recordParamAsZxx = (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDIMACROPITCHBEND) != 0;
		break;

	case MIDIEvents::evControllerChange:	//Controller change
		switch(nByte1)
		{
			case MIDIEvents::MIDICC_Volume_Coarse: //Volume
				midivolume = nByte2;
			break;
		}

		// Checking whether to record MIDI controller change as MIDI macro change.
		// Don't write this if command was already written by MIDI mapping.
		if((paramValue == uint16_max || sndFile.GetType() != MOD_TYPE_MPT)
			&& (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDIMACROCONTROL)
			&& !TrackerSettings::Instance().midiIgnoreCCs.Get()[nByte1 & 0x7F])
		{
			recordParamAsZxx = true;
		}
		break;

	case MIDIEvents::evSystem:
		if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_RESPONDTOPLAYCONTROLMSGS)
		{
			// Respond to MIDI song messages
			switch(channel)
			{
			case MIDIEvents::sysStart: //Start song
				if(GetDocument())
					GetDocument()->OnPlayerPlayFromStart();
				break;

			case MIDIEvents::sysContinue: //Continue song
				if(GetDocument())
					GetDocument()->OnPlayerPlay();
				break;

			case MIDIEvents::sysStop: //Stop song
				if(GetDocument())
					GetDocument()->OnPlayerStop();
				break;
			}
		}
		break;
	}

	// Write CC or pitch bend message as MIDI macro change.
	if(recordParamAsZxx && IsEditingEnabled())
	{
		const bool liveRecord = IsLiveRecord();

		ModCommandPos editpos = GetEditPos(sndFile, liveRecord);
		ModCommand &m = GetModCommand(sndFile, editpos);

		if(m.command == CMD_NONE || m.command == CMD_SMOOTHMIDI || m.command == CMD_MIDI)
		{
			// Write command only if there's no existing command or already a midi macro command.
			pModDoc->GetPatternUndo().PrepareUndo(editpos.pattern, editpos.channel, editpos.row, 1, 1, "MIDI Record Entry");
			m.command = CMD_SMOOTHMIDI;
			m.param = nByte2;
			pMainFrm->ThreadSafeSetModified(pModDoc);
			pModDoc->UpdateAllViews(this, PatternHint(editpos.pattern).Data(), this);

			// Update GUI only if not recording live.
			if(!liveRecord)
				InvalidateRow(editpos.row);
		}
	}

	// Pass MIDI to plugin
	if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDITOPLUG
		&& pMainFrm->GetModPlaying() == pModDoc
		&& event != MIDIEvents::evNoteOn
		&& event != MIDIEvents::evNoteOff)
	{
		const INSTRUMENTINDEX instr = static_cast<INSTRUMENTINDEX>(GetCurrentInstrument());
		IMixPlugin* plug = sndFile.GetInstrumentPlugin(instr);
		if(plug)
		{	
			plug->MidiSend(dwMidiData);
			// Sending MIDI may modify the plugin. For now, if MIDI data
			// is not active sensing, set modified.
			if(dwMidiData != MIDIEvents::System(MIDIEvents::sysActiveSense))
				pMainFrm->ThreadSafeSetModified(pModDoc);
		}

	}

	return 1;
}


LRESULT CViewPattern::OnModViewMsg(WPARAM wParam, LPARAM lParam)
//--------------------------------------------------------------
{
	switch(wParam)
	{

	case VIEWMSG_SETCTRLWND:
		m_hWndCtrl = (HWND)lParam;
		m_nOrder = static_cast<ORDERINDEX>(SendCtrlMessage(CTRLMSG_GETCURRENTORDER));
		SetCurrentPattern(static_cast<PATTERNINDEX>(SendCtrlMessage(CTRLMSG_GETCURRENTPATTERN)));
		break;

	case VIEWMSG_GETCURRENTPATTERN:
		return m_nPattern;

	case VIEWMSG_SETCURRENTPATTERN:
		if(GetSoundFile()->Patterns.IsValidPat(static_cast<PATTERNINDEX>(lParam)))
		{
			m_nOrder = static_cast<ORDERINDEX>(SendCtrlMessage(CTRLMSG_GETCURRENTORDER));
			SetCurrentPattern(static_cast<PATTERNINDEX>(lParam));
		}
		break;

	case VIEWMSG_GETCURRENTPOS:
		return (m_nPattern << 16) | GetCurrentRow();

	case VIEWMSG_FOLLOWSONG:
		m_Status.reset(psFollowSong);
		if (lParam)
		{
			CModDoc *pModDoc = GetDocument();
			m_Status.set(psFollowSong);
			if(pModDoc) pModDoc->SetNotifications(Notification::Position | Notification::VUMeters);
			if(pModDoc) pModDoc->SetFollowWnd(m_hWnd);
			SetFocus();
		} else
		{
			InvalidateRow();
		}
		break;

	case VIEWMSG_PATTERNLOOP:
		SendCtrlMessage(CTRLMSG_PAT_LOOP, lParam);
		break;

	case VIEWMSG_SETRECORD:
		m_Status.set(psRecordingEnabled, !!lParam);
		break;

	case VIEWMSG_SETSPACING:
		m_nSpacing = lParam;
		break;

	case VIEWMSG_PATTERNPROPERTIES:
		OnPatternProperties();
		GetParentFrame()->SetActiveView(this);
		break;

	case VIEWMSG_SETVUMETERS:
		m_Status.set(psShowVUMeters, !!lParam);
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true);
		break;

	case VIEWMSG_SETPLUGINNAMES:
		m_Status.set(psShowPluginNames, !!lParam);
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true);
		break;

	case VIEWMSG_DOMIDISPACING:
		if (m_nSpacing)
		{
			int temp = timeGetTime();
			if (temp - lParam >= 60)
			{
				CModDoc *pModDoc = GetDocument();
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if(!m_Status[psFollowSong]
					|| (pMainFrm->GetFollowSong(pModDoc) != m_hWnd)
					|| (pModDoc->GetSoundFile()->IsPaused()))
				{
					SetCurrentRow(GetCurrentRow() + m_nSpacing);
				}
			} else
			{
//				Sleep(1);
				Sleep(0);
				PostMessage(WM_MOD_VIEWMSG, VIEWMSG_DOMIDISPACING, lParam);
			}
		}
		break;

	case VIEWMSG_LOADSTATE:
		if (lParam)
		{
			PATTERNVIEWSTATE *pState = (PATTERNVIEWSTATE *)lParam;
			if (pState->nDetailLevel != PatternCursor::firstColumn) m_nDetailLevel = pState->nDetailLevel;
			if (/*(pState->nPattern == m_nPattern) && */(pState->cbStruct == sizeof(PATTERNVIEWSTATE)))
			{
				SetCurrentPattern(pState->nPattern);
				// Fix: Horizontal scrollbar pos screwed when selecting with mouse
				SetCursorPosition(pState->cursor);
				SetCurSel(pState->selection);
			}
		}
		break;

	case VIEWMSG_SAVESTATE:
		if (lParam)
		{
			PATTERNVIEWSTATE *pState = (PATTERNVIEWSTATE *)lParam;
			pState->cbStruct = sizeof(PATTERNVIEWSTATE);
			pState->nPattern = m_nPattern;
			pState->cursor = m_Cursor;
			pState->selection = m_Selection;
			pState->nDetailLevel = m_nDetailLevel;
			pState->nOrder = GetCurrentOrder();
		}
		break;

	case VIEWMSG_EXPANDPATTERN:
		{
			CModDoc *pModDoc = GetDocument();
			if (pModDoc->ExpandPattern(m_nPattern))
			{
				m_Cursor.SetRow(m_Cursor.GetRow() * 2);
				SetCurrentPattern(m_nPattern);
			}
		}
		break;

	case VIEWMSG_SHRINKPATTERN:
		{
			CModDoc *pModDoc = GetDocument();
			if (pModDoc->ShrinkPattern(m_nPattern))
			{
				m_Cursor.SetRow(m_Cursor.GetRow() / 2);
				SetCurrentPattern(m_nPattern);
			}
		}
		break;

	case VIEWMSG_COPYPATTERN:
		{
			const CSoundFile *pSndFile = GetSoundFile();
			if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
			{
				CopyPattern(m_nPattern, PatternRect(PatternCursor(0, 0), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn)));
			}
		}
		break;

	case VIEWMSG_PASTEPATTERN:
		{
			PastePattern(m_nPattern, PatternCursor(0), PatternClipboard::pmOverwrite);
			InvalidatePattern();
		}
		break;

	case VIEWMSG_AMPLIFYPATTERN:
		OnPatternAmplify();
		break;

	case VIEWMSG_SETDETAIL:
		if (lParam != m_nDetailLevel)
		{
			m_nDetailLevel = static_cast<PatternCursor::Columns>(lParam);
			UpdateSizes();
			UpdateScrollSize();
			SetCurrentColumn(m_Cursor);
			InvalidatePattern(true);
		}
		break;
	case VIEWMSG_DOSCROLL:
		OnMouseWheel(0, static_cast<short>(lParam), CPoint(0, 0));
		break;


	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}


void CViewPattern::CursorJump(int distance, bool snap)
//----------------------------------------------------
{
	ROWINDEX row = GetCurrentRow();
	const bool upwards = distance < 0;
	const int distanceAbs = mpt::abs(distance);

	if(snap)
		row = (((row + (upwards ? -1 : 0)) / distanceAbs) + (upwards ? 0 : 1)) * distanceAbs;
	else
		row += distance;
	row = SetCurrentRow(row, true);

	if(IsLiveRecord() && !m_Status[psDragActive])
	{
		CriticalSection cs;
		CSoundFile &sndFile = GetDocument()->GetrSoundFile();
		if(m_nOrder != sndFile.m_PlayState.m_nCurrentOrder)
		{
			// We jumped to a different order
			sndFile.ResetChannels();
			sndFile.StopAllVsti();
		}

		sndFile.m_PlayState.m_nCurrentOrder = sndFile.m_PlayState.m_nNextOrder = GetCurrentOrder();
		sndFile.m_PlayState.m_nPattern = m_nPattern;
		sndFile.m_PlayState.m_nRow = m_nPlayRow = row;
		sndFile.m_PlayState.m_nNextRow = m_nNextPlayRow = row + 1;
		// Queue the correct follow-up pattern if we just jumped to the last row.
		if(sndFile.Patterns.IsValidPat(m_nPattern) && m_nNextPlayRow >= sndFile.Patterns[m_nPattern].GetNumRows())
		{
			sndFile.m_PlayState.m_nNextOrder++;
		}
		CMainFrame::GetMainFrame()->ResetNotificationBuffer();
	}
}


LRESULT CViewPattern::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//--------------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return NULL;
	
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	switch(wParam)
	{
		case kcPrevInstrument:				OnPrevInstrument(); return wParam;
		case kcNextInstrument:				OnNextInstrument(); return wParam;
		case kcPrevOrder:					OnPrevOrder(); return wParam;
		case kcNextOrder:					OnNextOrder(); return wParam;
		case kcPatternPlayRow:				OnPatternStep(); return wParam;
		case kcPatternRecord:				OnPatternRecord(); return wParam;
		case kcCursorCopy:					OnCursorCopy(); return wParam;
		case kcCursorPaste:					OnCursorPaste(); return wParam;
		case kcChannelMute:					for(CHANNELINDEX c = m_Selection.GetStartChannel(); c <= m_Selection.GetEndChannel(); c++)
												OnMuteChannel(c);
											return wParam;
		case kcChannelSolo:					OnSoloChannel(GetCurrentChannel()); return wParam;
		case kcChannelUnmuteAll:			OnUnmuteAll(); return wParam;
		case kcToggleChanMuteOnPatTransition: for(CHANNELINDEX c = m_Selection.GetStartChannel(); c <= m_Selection.GetEndChannel(); c++)
												TogglePendingMute(c);
											return wParam;
		case kcUnmuteAllChnOnPatTransition:	OnPendingUnmuteAllChnFromClick(); return wParam;
		case kcChannelRecordSelect:			for(CHANNELINDEX c = m_Selection.GetStartChannel(); c <= m_Selection.GetEndChannel(); c++)
												pModDoc->Record1Channel(c);
											InvalidateChannelsHeaders(); return wParam;
		case kcChannelSplitRecordSelect:	for(CHANNELINDEX c = m_Selection.GetStartChannel(); c <= m_Selection.GetEndChannel(); c++)
												pModDoc->Record2Channel(c);
											InvalidateChannelsHeaders(); return wParam;
		case kcChannelReset:				for(CHANNELINDEX c = m_Selection.GetStartChannel(); c <= m_Selection.GetEndChannel(); c++)
												ResetChannel(m_Cursor.GetChannel());
											return wParam;
		case kcTimeAtRow:					OnShowTimeAtRow(); return wParam;
		case kcSoloChnOnPatTransition:		PendingSoloChn(GetCurrentChannel()); return wParam;
		case kcTransposeUp:					OnTransposeUp(); return wParam;
		case kcTransposeDown:				OnTransposeDown(); return wParam;
		case kcTransposeOctUp:				OnTransposeOctUp(); return wParam;
		case kcTransposeOctDown:			OnTransposeOctDown(); return wParam;
		case kcTransposeCustom:				OnTransposeCustom(); return wParam;
		case kcDataEntryUp:					DataEntry(true, false); return wParam;
		case kcDataEntryDown:				DataEntry(false, false); return wParam;
		case kcDataEntryUpCoarse:			DataEntry(true, true); return wParam;
		case kcDataEntryDownCoarse:			DataEntry(false, true); return wParam;
		case kcSelectColumn:				OnSelectCurrentColumn(); return wParam;
		case kcPatternAmplify:				OnPatternAmplify(); return wParam;
		case kcPatternSetInstrument:		OnSetSelInstrument(); return wParam;
		case kcPatternInterpolateNote:		OnInterpolateNote(); return wParam;
		case kcPatternInterpolateInstr:		OnInterpolateInstr(); return wParam;
		case kcPatternInterpolateVol:		OnInterpolateVolume(); return wParam;
		case kcPatternInterpolateEffect:	OnInterpolateEffect(); return wParam;
		case kcPatternVisualizeEffect:		OnVisualizeEffect(); return wParam;
		//case kcPatternOpenRandomizer:		OnOpenRandomizer(); return wParam;
		case kcPatternGrowSelection:		OnGrowSelection(); return wParam;
		case kcPatternShrinkSelection:		OnShrinkSelection(); return wParam;

		// Pattern navigation:
		case kcPatternJumpUph1Select:
		case kcPatternJumpUph1:			CursorJump(-(int)GetRowsPerMeasure(), false); return wParam;
		case kcPatternJumpDownh1Select:
		case kcPatternJumpDownh1:		CursorJump(GetRowsPerMeasure(), false);  return wParam;
		case kcPatternJumpUph2Select:
		case kcPatternJumpUph2:			CursorJump(-(int)GetRowsPerBeat(), false);  return wParam;
		case kcPatternJumpDownh2Select:
		case kcPatternJumpDownh2:		CursorJump(GetRowsPerBeat(), false);  return wParam;

		case kcPatternSnapUph1Select:
		case kcPatternSnapUph1:			CursorJump(-(int)GetRowsPerMeasure(), true); return wParam;
		case kcPatternSnapDownh1Select:
		case kcPatternSnapDownh1:		CursorJump(GetRowsPerMeasure(), true);  return wParam;
		case kcPatternSnapUph2Select:
		case kcPatternSnapUph2:			CursorJump(-(int)GetRowsPerBeat(), true);  return wParam;
		case kcPatternSnapDownh2Select:
		case kcPatternSnapDownh2:		CursorJump(GetRowsPerBeat(), true);  return wParam;

		case kcNavigateDownSelect:
		case kcNavigateDown:	CursorJump(1, false); return wParam;
		case kcNavigateUpSelect:
		case kcNavigateUp:		CursorJump(-1, false); return wParam;

		case kcNavigateDownBySpacingSelect:
		case kcNavigateDownBySpacing:	CursorJump(m_nSpacing, false); return wParam;
		case kcNavigateUpBySpacingSelect:
		case kcNavigateUpBySpacing:		CursorJump(-(int)m_nSpacing, false); return wParam;

		case kcNavigateLeftSelect:
		case kcNavigateLeft:
			MoveCursor(false);
			return wParam;
		case kcNavigateRightSelect:
		case kcNavigateRight:
			MoveCursor(true);
			return wParam;

		case kcNavigateNextChanSelect:
		case kcNavigateNextChan: SetCurrentColumn((GetCurrentChannel() + 1) % pSndFile->GetNumChannels(), m_Cursor.GetColumnType()); return wParam;
		case kcNavigatePrevChanSelect:
		case kcNavigatePrevChan:{if(GetCurrentChannel() > 0)
									SetCurrentColumn((GetCurrentChannel() - 1) % pSndFile->GetNumChannels(), m_Cursor.GetColumnType());
								else
									SetCurrentColumn(pSndFile->GetNumChannels() - 1, m_Cursor.GetColumnType());
								SetSelToCursor();
								return wParam;}

		case kcHomeHorizontalSelect:
		case kcHomeHorizontal:	if (!m_Cursor.IsInFirstColumn()) SetCurrentColumn(0);
								else if (GetCurrentRow() > 0) SetCurrentRow(0);
								return wParam;
		case kcHomeVerticalSelect:
		case kcHomeVertical:	if (GetCurrentRow() > 0) SetCurrentRow(0);
								else if (!m_Cursor.IsInFirstColumn()) SetCurrentColumn(0);
								return wParam;
		case kcHomeAbsoluteSelect:
		case kcHomeAbsolute:	if (!m_Cursor.IsInFirstColumn()) SetCurrentColumn(0);
								if (GetCurrentRow() > 0) SetCurrentRow(0);
								return wParam;

		case kcEndHorizontalSelect:
		case kcEndHorizontal:	if (m_Cursor.CompareColumn(PatternCursor(0, pSndFile->GetNumChannels() - 1, m_nDetailLevel)) < 0) SetCurrentColumn(pSndFile->GetNumChannels() - 1, m_nDetailLevel);
								else if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								return wParam;
		case kcEndVerticalSelect:
		case kcEndVertical:		if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								else if (m_Cursor.CompareColumn(PatternCursor(0, pSndFile->GetNumChannels() - 1, m_nDetailLevel)) < 0) SetCurrentColumn(pSndFile->GetNumChannels() - 1, m_nDetailLevel);
								return wParam;
		case kcEndAbsoluteSelect:
		case kcEndAbsolute:		SetCurrentColumn(pSndFile->GetNumChannels() - 1, m_nDetailLevel);
								if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								return wParam;

		case kcNextPattern:	{	PATTERNINDEX n = m_nPattern + 1;
								while ((n < pSndFile->Patterns.Size()) && !pSndFile->Patterns.IsValidPat(n)) n++;
								SetCurrentPattern((n < pSndFile->Patterns.Size()) ? n : 0);
								ORDERINDEX currentOrder = GetCurrentOrder();
								ORDERINDEX newOrder = pSndFile->Order.FindOrder(m_nPattern, currentOrder, true);
								SetCurrentOrder(newOrder);
								return wParam;
							}
		case kcPrevPattern: {	PATTERNINDEX n = (m_nPattern) ? m_nPattern - 1 : pSndFile->Patterns.Size() - 1;
								while (n > 0 && !pSndFile->Patterns.IsValidPat(n)) n--;
								SetCurrentPattern(n);
								ORDERINDEX currentOrder = GetCurrentOrder();
								ORDERINDEX newOrder = pSndFile->Order.FindOrder(m_nPattern, currentOrder, false);
								SetCurrentOrder(newOrder);
								return wParam;
							}
		case kcSelectWithCopySelect:
		case kcSelectWithNav:
		case kcSelect:			if(!m_Status[psDragnDropEdit | psRowSelection | psChannelSelection | psMouseDragSelect]) m_StartSel = m_Cursor;
									m_Status.set(psKeyboardDragSelect);
								return wParam;
		case kcSelectOffWithCopySelect:
		case kcSelectOffWithNav:
		case kcSelectOff:		m_Status.reset(psKeyboardDragSelect | psShiftSelect);
								return wParam;
		case kcCopySelectWithSelect:
		case kcCopySelectWithNav:
		case kcCopySelect:		if(!m_Status[psDragnDropEdit | psRowSelection | psChannelSelection | psMouseDragSelect]) m_StartSel = m_Cursor;
									m_Status.set(psCtrlDragSelect); return wParam;
		case kcCopySelectOffWithSelect:
		case kcCopySelectOffWithNav:
		case kcCopySelectOff:	m_Status.reset(psCtrlDragSelect); return wParam;

		case kcSelectBeat:
		case kcSelectMeasure:
			SelectBeatOrMeasure(wParam == kcSelectBeat); return wParam;

		case kcSelectEvent:		SetCurSel(PatternCursor(m_Selection.GetStartRow(), m_Selection.GetStartChannel(), PatternCursor::firstColumn),
									PatternCursor(m_Selection.GetEndRow(), m_Selection.GetEndChannel(), PatternCursor::lastColumn));
								return wParam;
		case kcSelectRow:		SetCurSel(PatternCursor(m_Selection.GetStartRow(), 0, PatternCursor::firstColumn),
									PatternCursor(m_Selection.GetEndRow(), pSndFile->GetNumChannels(), PatternCursor::lastColumn));
								return wParam;

		case kcClearRow:		OnClearField(RowMask(), false);	return wParam;
		case kcClearField:		OnClearField(RowMask(m_Cursor), false);	return wParam;
		case kcClearFieldITStyle: OnClearField(RowMask(m_Cursor), false, true);	return wParam;
		case kcClearRowStep:	OnClearField(RowMask(), true);	return wParam;
		case kcClearFieldStep:	OnClearField(RowMask(m_Cursor), true);	return wParam;
		case kcClearFieldStepITStyle:	OnClearField(RowMask(m_Cursor), true, true);	return wParam;
		case kcDeleteRows:		OnDeleteRows(); return wParam;
		case kcDeleteAllRows:	OnDeleteRowsEx(); return wParam;
		case kcInsertRow:		OnInsertRows(); return wParam;
		case kcInsertAllRows:	InsertRows(0, pSndFile->GetNumChannels() - 1); return wParam;

		case kcShowNoteProperties: ShowEditWindow(); return wParam;
		case kcShowPatternProperties: OnPatternProperties(); return wParam;
		case kcShowSplitKeyboardSettings:	SetSplitKeyboardSettings(); return wParam;
		case kcShowEditMenu:	{
									CPoint pt = GetPointFromPosition(m_Cursor);
									OnRButtonDown(0, pt);
								}
								return wParam;
		case kcPatternGoto:		OnEditGoto(); return wParam;

		case kcNoteCut:			TempEnterNote(NOTE_NOTECUT); return wParam;
		case kcNoteOff:			TempEnterNote(NOTE_KEYOFF); return wParam;
		case kcNoteFade:		TempEnterNote(NOTE_FADE); return wParam;
		case kcNotePC:			TempEnterNote(NOTE_PC); return wParam;
		case kcNotePCS:			TempEnterNote(NOTE_PCS); return wParam;

		case kcEditUndo:		OnEditUndo(); return wParam;
		case kcEditRedo:		OnEditRedo(); return wParam;
		case kcEditFind:		OnEditFind(); return wParam;
		case kcEditFindNext:	OnEditFindNext(); return wParam;
		case kcEditCut:			OnEditCut(); return wParam;
		case kcEditCopy:		OnEditCopy(); return wParam;
		case kcCopyAndLoseSelection:
								{
								OnEditCopy();
								SetSelToCursor();
								return wParam;
								}
		case kcEditPaste:		OnEditPaste(); return wParam;
		case kcEditMixPaste:	OnEditMixPaste(); return wParam;
		case kcEditMixPasteITStyle:	OnEditMixPasteITStyle(); return wParam;
		case kcEditPasteFlood:	OnEditPasteFlood(); return wParam;
		case kcEditPushForwardPaste: OnEditPushForwardPaste(); return wParam;
		case kcEditSelectAll:	OnEditSelectAll(); return wParam;
		case kcTogglePluginEditor: TogglePluginEditor(GetCurrentChannel()); return wParam;
		case kcToggleFollowSong: SendCtrlMessage(CTRLMSG_PAT_FOLLOWSONG, 1); return wParam;
		case kcChangeLoopStatus: SendCtrlMessage(CTRLMSG_PAT_LOOP, -1); return wParam;
		case kcNewPattern:		 SendCtrlMessage(CTRLMSG_PAT_NEWPATTERN); return wParam;
		case kcDuplicatePattern: SendCtrlMessage(CTRLMSG_PAT_DUPPATTERN); return wParam;
		case kcSwitchToOrderList: OnSwitchToOrderList();
		case kcToggleOverflowPaste:	TrackerSettings::Instance().m_dwPatternSetup ^= PATTERN_OVERFLOWPASTE; return wParam;
		case kcToggleNoteOffRecordPC: TrackerSettings::Instance().m_dwPatternSetup ^= PATTERN_KBDNOTEOFF; return wParam;
		case kcToggleNoteOffRecordMIDI: TrackerSettings::Instance().m_dwMidiSetup ^= MIDISETUP_RECORDNOTEOFF; return wParam;
		case kcPatternEditPCNotePlugin: OnTogglePCNotePluginEditor(); return wParam;
		case kcQuantizeSettings: OnSetQuantize(); return wParam;
		case kcFindInstrument: FindInstrument(); return wParam;
		case kcChannelSettings:
			{
				// Open centered Quick Channel Settings dialog.
				CRect windowPos;
				GetWindowRect(windowPos);
				quickChannelProperties.Show(GetDocument(), m_Cursor.GetChannel(), m_nPattern, CPoint(windowPos.left + windowPos.Width() / 2, windowPos.top + windowPos.Height() / 2));
				return wParam;
			}
		case kcChannelTranspose: m_MenuCursor = m_Cursor; OnTransposeChannel(); return wParam;
		case kcChannelDuplicate: m_MenuCursor = m_Cursor; OnDuplicateChannel(); return wParam;

		case kcDecreaseSpacing:
			if(m_nSpacing > 0) SetSpacing(m_nSpacing - 1);
			return wParam;
		case kcIncreaseSpacing:
			if(m_nSpacing < MAX_SPACING) SetSpacing(m_nSpacing + 1);
			return wParam;

		// Clipboard Manager
		case kcToggleClipboardManager:
			PatternClipboardDialog::Toggle();
			return wParam;
		case kcClipboardPrev:
			PatternClipboard::CycleBackward();
			PatternClipboardDialog::UpdateList();
			return wParam;
		case kcClipboardNext:
			PatternClipboard::CycleForward();
			PatternClipboardDialog::UpdateList();
			return wParam;

	}
	//Ranges:
	if(wParam >= kcVPStartNotes && wParam <= kcVPEndNotes)
	{
		TempEnterNote(static_cast<ModCommand::NOTE>(wParam - kcVPStartNotes + 1 + pMainFrm->GetBaseOctave() * 12));
		return wParam;
	}
	if(wParam >= kcVPStartChords && wParam <= kcVPEndChords)
	{
		TempEnterChord(static_cast<ModCommand::NOTE>(wParam - kcVPStartChords + 1 + pMainFrm->GetBaseOctave() * 12));
		return wParam;
	}

	if(wParam >= kcVPStartNoteStops && wParam <= kcVPEndNoteStops)
	{
		TempStopNote(static_cast<ModCommand::NOTE>(wParam - kcVPStartNoteStops + 1 + pMainFrm->GetBaseOctave() * 12));
		return wParam;
	}
	if(wParam >= kcVPStartChordStops && wParam <= kcVPEndChordStops)
	{
		TempStopChord(static_cast<ModCommand::NOTE>(wParam - kcVPStartChordStops + 1 + pMainFrm->GetBaseOctave() * 12));
		return wParam;
	}

	if(wParam >= kcSetSpacing0 && wParam <= kcSetSpacing9)
	{
		SetSpacing(wParam - kcSetSpacing0);
		return wParam;
	}

	if(wParam >= kcSetIns0 && wParam <= kcSetIns9)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterIns(wParam - kcSetIns0);
		return wParam;
	}

	if(wParam >= kcSetOctave0 && wParam <= kcSetOctave9)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterOctave(wParam - kcSetOctave0);
		return wParam;
	}

	if(wParam >= kcSetOctaveStop0 && wParam <= kcSetOctaveStop9)
	{
		TempStopOctave(wParam - kcSetOctaveStop0);
		return wParam;
	}

	if(wParam >= kcSetVolumeStart && wParam <= kcSetVolumeEnd)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterVol(wParam - kcSetVolumeStart);
		return wParam;
	}

	if(wParam >= kcSetFXStart && wParam <= kcSetFXEnd)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterFX(static_cast<ModCommand::COMMAND>(wParam - kcSetFXStart + 1));
		return wParam;
	}

	if(wParam >= kcSetFXParam0 && wParam <= kcSetFXParamF)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterFXparam(wParam - kcSetFXParam0);
		return wParam;
	}

	return NULL;
}


// Move pattern cursor to left or right, respecting invisible columns.
void CViewPattern::MoveCursor(bool moveRight)
//-------------------------------------------
{
	if(!moveRight)
	{
		// Move cursor one column to the left
		if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_WRAP) && m_Cursor.IsInFirstColumn())
		{
			// Wrap around to last channel
			SetCurrentColumn(GetDocument()->GetNumChannels() - 1, m_nDetailLevel);
		} else if(!m_Cursor.IsInFirstColumn())
		{
			m_Cursor.Move(0, 0, -1);
			SetCurrentColumn(m_Cursor);
		}
	} else
	{
		// Move cursor one column to the right
		const PatternCursor rightmost(0, GetDocument()->GetNumChannels() - 1, m_nDetailLevel);
		if(m_Cursor.CompareColumn(rightmost) >= 0)
		{
			if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_WRAP))
			{
				// Wrap around to first channel.
				SetCurrentColumn(0);
			} else
			{
				SetCurrentColumn(rightmost);
			}
		} else
		{
			do
			{
				m_Cursor.Move(0, 0, 1);
			} while(m_Cursor.GetColumnType() > m_nDetailLevel);
			SetCurrentColumn(m_Cursor);
		}
	}
}

#define ENTER_PCNOTE_VALUE(v, method) \
	{ \
		if((v >= 0) && (v <= 9)) \
		{	\
			uint16 val = target.Get##method##(); \
			/* Move existing digits to left, drop out leftmost digit and */ \
			/* push new digit to the least meaning digit. */ \
			val = (val % 100) * 10 + v; \
			if(val > ModCommand::maxColumnValue) val = ModCommand::maxColumnValue; \
			target.Set##method##(val); \
			m_PCNoteEditMemory = target; \
		} \
	} 


// Enter volume effect / number in the pattern.
void CViewPattern::TempEnterVol(int v)
//------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}
		
	PrepareUndo(m_Cursor, m_Cursor, "Volume Entry");

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target; // This is the command we are about to overwrite
	const bool isDigit = (v >= 0) && (v <= 9);

	if(target.IsPcNote())
	{
		ENTER_PCNOTE_VALUE(static_cast<uint16>(v), ValueVolCol);
	} else
	{
		ModCommand::VOLCMD volcmd = target.volcmd;
		uint16 vol = target.vol;
		if (isDigit)
		{
			vol = ((vol * 10) + v) % 100;
			if (!volcmd) volcmd = VOLCMD_VOLUME;
		} else
		{
			switch(v+kcSetVolumeStart)
			{
			case kcSetVolumeVol:			volcmd = VOLCMD_VOLUME; break;
			case kcSetVolumePan:			volcmd = VOLCMD_PANNING; break;
			case kcSetVolumeVolSlideUp:		volcmd = VOLCMD_VOLSLIDEUP; break;
			case kcSetVolumeVolSlideDown:	volcmd = VOLCMD_VOLSLIDEDOWN; break;
			case kcSetVolumeFineVolUp:		volcmd = VOLCMD_FINEVOLUP; break;
			case kcSetVolumeFineVolDown:	volcmd = VOLCMD_FINEVOLDOWN; break;
			case kcSetVolumeVibratoSpd:		volcmd = VOLCMD_VIBRATOSPEED; break;
			case kcSetVolumeVibrato:		volcmd = VOLCMD_VIBRATODEPTH; break;
			case kcSetVolumeXMPanLeft:		volcmd = VOLCMD_PANSLIDELEFT; break;
			case kcSetVolumeXMPanRight:		volcmd = VOLCMD_PANSLIDERIGHT; break;
			case kcSetVolumePortamento:		volcmd = VOLCMD_TONEPORTAMENTO; break;
			case kcSetVolumeITPortaUp:		volcmd = VOLCMD_PORTAUP; break;
			case kcSetVolumeITPortaDown:	volcmd = VOLCMD_PORTADOWN; break;
			case kcSetVolumeITOffset:		volcmd = VOLCMD_OFFSET; break;
			}
		}

		UINT max = 64;
		if(volcmd > VOLCMD_PANNING)
		{
			max = (pSndFile->GetType() == MOD_TYPE_XM) ? 0x0F : 9;
		}

		if(vol > max) vol %= 10;
		if(pSndFile->GetModSpecifications().HasVolCommand(volcmd))
		{
			target.volcmd = volcmd;
			target.vol = static_cast<ModCommand::VOL>(vol);
		}
	}

	SetSelToCursor();

	if(oldcmd != target)
	{
		SetModified(false);
		InvalidateCell(m_Cursor);
		UpdateIndicator();
	}

	// Cursor step for command letter
	if(!target.IsPcNote() && !isDigit && m_nSpacing > 0 && m_nSpacing <= MAX_SPACING && !IsLiveRecord() && TrackerSettings::Instance().patternStepCommands)
	{
		if(m_Cursor.GetRow() + m_nSpacing < pSndFile->Patterns[m_nPattern].GetNumRows() || (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL))
		{
			SetCurrentRow(m_Cursor.GetRow() + m_nSpacing, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) != 0);
		}
	}

}


void CViewPattern::SetSpacing(int n)
//----------------------------------
{
	if (static_cast<UINT>(n) != m_nSpacing)
	{
		m_nSpacing = static_cast<UINT>(n);
		PostCtrlMessage(CTRLMSG_SETSPACING, m_nSpacing);
	}
}


// Enter an effect letter in the pattenr
void CViewPattern::TempEnterFX(ModCommand::COMMAND c, int v)
//----------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target; // This is the command we are about to overwrite

	PrepareUndo(m_Cursor, m_Cursor, "Effect Entry");

	if(target.IsPcNote())
	{
		ENTER_PCNOTE_VALUE(static_cast<uint16>(c), ValueEffectCol);
	} else if(pSndFile->GetModSpecifications().HasCommand(c))
	{
		if (c != CMD_NONE)
		{
			if ((c == m_cmdOld.command) && (!target.param) && (target.command == CMD_NONE))
			{
				target.param = m_cmdOld.param;
			} else
			{
				m_cmdOld.param = 0;
			}
			m_cmdOld.command = c;
		}
		target.command = c;
		if(v >= 0)
		{
			target.param = static_cast<ModCommand::PARAM>(v);
		}

		// Check for MOD/XM Speed/Tempo command
		if((pSndFile->GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM))
			&& (target.command == CMD_SPEED || target.command == CMD_TEMPO))
		{
			target.command = static_cast<ModCommand::COMMAND>((target.param <= pSndFile->GetModSpecifications().speedMax) ? CMD_SPEED : CMD_TEMPO);
		}
	}

	SetSelToCursor();

	if(oldcmd != target)
	{
		SetModified(false);
		InvalidateCell(m_Cursor);
		UpdateIndicator();
	}

	// Cursor step for command letter
	if(!target.IsPcNote() && m_nSpacing > 0 && m_nSpacing <= MAX_SPACING && !IsLiveRecord() && TrackerSettings::Instance().patternStepCommands)
	{
		if(m_Cursor.GetRow() + m_nSpacing < pSndFile->Patterns[m_nPattern].GetNumRows() || (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL))
		{
			SetCurrentRow(m_Cursor.GetRow() + m_nSpacing, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) != 0);
		}
	}
}


// Enter an effect param in the pattenr
void CViewPattern::TempEnterFXparam(int v)
//----------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target; // This is the command we are about to overwrite

	PrepareUndo(m_Cursor, m_Cursor, "Parameter Entry");

	if(target.IsPcNote())
	{
		ENTER_PCNOTE_VALUE(static_cast<uint16>(v), ValueEffectCol);
	} else
	{

		target.param = static_cast<ModCommand::PARAM>((target.param << 4) | v);
		if (target.command == m_cmdOld.command)
		{
			m_cmdOld.param = target.param;
		}

		// Check for MOD/XM Speed/Tempo command
		if((pSndFile->GetType() & (MOD_TYPE_MOD|MOD_TYPE_XM))
			&& (target.command == CMD_SPEED || target.command == CMD_TEMPO))
		{
			target.command = static_cast<ModCommand::COMMAND>((target.param <= pSndFile->GetModSpecifications().speedMax) ? CMD_SPEED : CMD_TEMPO);
		}
	}

	SetSelToCursor();

	if(target != oldcmd)
	{
		SetModified(false);
		InvalidateCell(m_Cursor);
		UpdateIndicator();
	}
}


// Stop a note that has been entered
void CViewPattern::TempStopNote(ModCommand::NOTE note, bool fromMidi, const bool bChordMode)
//------------------------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pModDoc == nullptr || pMainFrm == nullptr)
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	const CModSpecifications &specs = sndFile.GetModSpecifications();

	if(!ModCommand::IsSpecialNote(note))
	{
		Limit(note, specs.noteMin, specs.noteMax);
	}

	const bool liveRecord = IsLiveRecord();
	const bool isSplit = IsNoteSplit(note);
	UINT ins = 0;

	BYTE *activeNoteMap = isSplit ? splitActiveNoteChannel : activeNoteChannel;
	const CHANNELINDEX nChnCursor = GetCurrentChannel();
	const CHANNELINDEX nChn = bChordMode ? chordPatternChannels[0] : (activeNoteMap[note] < sndFile.GetNumChannels() ? activeNoteMap[note] : nChnCursor);

	CHANNELINDEX noteChannels[MPTChord::notesPerChord] = { nChn };
	ModCommand::NOTE notes[MPTChord::notesPerChord] = { note };
	int numNotes = 1;

	if(pModDoc)
	{
		if(isSplit)
		{
			ins = pModDoc->GetSplitKeyboardSettings().splitInstrument;
			if(pModDoc->GetSplitKeyboardSettings().octaveLink)
			{
				int trNote = note + 12 *pModDoc->GetSplitKeyboardSettings().octaveModifier;
				Limit(trNote, specs.noteMin, specs.noteMax);
				note = static_cast<ModCommand::NOTE>(trNote);
			}
		}
		if(!ins)
			ins = GetCurrentInstrument();
		if(!ins)
			ins = m_nFoundInstrument;

		const bool playWholeRow = ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !liveRecord);
		if(bChordMode == true)
		{
			m_Status.reset(psChordPlaying);

			numNotes = ConstructChord(note, notes, prevChordBaseNote);
			if(!numNotes)
			{
				return;
			}
			for(int i = 0; i < numNotes; i++)
			{
				pModDoc->NoteOff(notes[i], true, static_cast<INSTRUMENTINDEX>(ins), GetCurrentChannel(), playWholeRow ? chordPatternChannels[i] : CHANNELINDEX_INVALID);
				m_baPlayingNote.reset(notes[i]);
				noteChannels[i] = chordPatternChannels[i];
			}
			prevChordNote = NOTE_NONE;
		} else
		{
			m_baPlayingNote.reset(note);
			pModDoc->NoteOff(note, ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_NOTEFADE) || sndFile.GetNumInstruments() == 0), static_cast<INSTRUMENTINDEX>(ins), nChnCursor, playWholeRow ? nChn : CHANNELINDEX_INVALID);
		}
	}

	// Enter note off in pattern?
	if(!ModCommand::IsNote(note))
		return;
	if (m_Cursor.GetColumnType() > PatternCursor::instrColumn && (bChordMode || !fromMidi))
		return;
	if (!pModDoc || !pMainFrm || !(IsEditingEnabled()))
		return;

	activeNoteMap[note] = 0xFF;	//unlock channel

	if(!((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_KBDNOTEOFF) || fromMidi))
	{
		// We don't want to write the note-off into the pattern if this feature is disabled and we're not recording from MIDI.
		return;
	}

	// -- write sdx if playing live
	const bool usePlaybackPosition = (!bChordMode) && (liveRecord && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_AUTODELAY));

	//Work out where to put the note off
	ModCommandPos editPos = GetEditPos(sndFile, usePlaybackPosition);

	const bool doQuantize = (liveRecord || (fromMidi && (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN))) && TrackerSettings::Instance().recordQuantizeRows != 0;
	if(doQuantize)
	{
		QuantizeRow(editPos.pattern, editPos.row);
	}

	ModCommand *pTarget = sndFile.Patterns[editPos.pattern].GetpModCommand(editPos.row, nChn);
	// Don't overwrite:
	if(pTarget->note != NOTE_NONE || pTarget->instr || pTarget->volcmd != VOLCMD_NONE)
	{
		// If there's a note in the current location and the song is playing and following,
		// the user probably just tapped the key - let's try the next row down.
		editPos.row++;
		if(pTarget->note == note && liveRecord && sndFile.Patterns[editPos.pattern].IsValidRow(editPos.row))
		{
			pTarget = sndFile.Patterns[editPos.pattern].GetpModCommand(editPos.row, nChn);
			if(pTarget->note != NOTE_NONE || (!bChordMode && (pTarget->instr || pTarget->volcmd)))
				return;
		} else
		{
			return;
		}
	}

	// Create undo-point.
	pModDoc->GetPatternUndo().PrepareUndo(editPos.pattern, nChn, editPos.row, noteChannels[numNotes - 1] - nChn + 1, 1, "Note Stop Entry");

	for(int i = 0; i < numNotes; i++)
	{
		if(previousNote[noteChannels[i]] != notes[i])
		{
			// This might be a note-off from a past note, but since we already hit a new note on this channel, we ignore it.
			continue;
		}
		ModCommand *pTarget = sndFile.Patterns[editPos.pattern].GetpModCommand(editPos.row, noteChannels[i]);

		// -- write sdx if playing live
		if(usePlaybackPosition && m_nPlayTick && pTarget->command == CMD_NONE && !doQuantize)
		{
			pTarget->command = CMD_S3MCMDEX;
			if(!specs.HasCommand(CMD_S3MCMDEX)) pTarget->command = CMD_MODCMDEX;
			pTarget->param = static_cast<ModCommand::PARAM>(0xD0 | MIN(0xF, m_nPlayTick));
		}

		//Enter note off
		if(sndFile.GetModSpecifications().hasNoteOff && (sndFile.GetNumInstruments() > 0 || !sndFile.GetModSpecifications().hasNoteCut))
		{
			// ===
			// Not used in sample (if module format supports ^^^ instead)
			pTarget->note = NOTE_KEYOFF;
		} else if(sndFile.GetModSpecifications().hasNoteCut)
		{
			// ^^^
			pTarget->note = NOTE_NOTECUT;
		} else
		{
			// we don't have anything to cut (MOD format) - use volume or ECx
			if(usePlaybackPosition && m_nPlayTick && !doQuantize) // ECx
			{
				pTarget->command = CMD_S3MCMDEX;
				if(!specs.HasCommand(CMD_S3MCMDEX)) pTarget->command = CMD_MODCMDEX;
				pTarget->param = static_cast<ModCommand::PARAM>(0xC0 | MIN(0xF, m_nPlayTick));
			} else // C00
			{
				pTarget->note = NOTE_NONE;
				pTarget->command = CMD_VOLUME;
				pTarget->param = 0;
			}
		}
		pTarget->instr = 0;	// Instrument numbers next to note-offs can do all kinds of weird things in XM files, and they are pointless anyway.
		pTarget->volcmd = VOLCMD_NONE;
		pTarget->vol = 0;
	}

	SetModified(false);

	if(editPos.pattern == m_nPattern)
	{
		InvalidateRow(editPos.row);
	} else
	{
		InvalidatePattern();
	}

	// Update only if not recording live.
	if(!liveRecord)
	{
		UpdateIndicator();
	}

	return;

}


// Enter an octave number in the pattern
void CViewPattern::TempEnterOctave(int val)
//-----------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	const ModCommand &target = GetCursorCommand();
	if(target.IsNote())
	{
		int groupSize = 12;
		if(target.instr > 0 && target.instr <= pSndFile->GetNumInstruments())
		{
			const ModInstrument *pIns = pSndFile->Instruments[target.instr];
			if(pIns != nullptr && pIns->pTuning != nullptr)
			{
				groupSize = pIns->pTuning->GetGroupSize();
			}
		}

		PrepareUndo(m_Cursor, m_Cursor, "Octave Entry");
		// The following might look a bit convoluted... This is mostly because the "middle-C" in
		// custom tunings always has octave 5, no matter how many octaves the tuning actually has.
		int note = ((target.note - NOTE_MIN) % groupSize) + (val - 5) * groupSize + NOTE_MIDDLEC;
		Limit(note, NOTE_MIN, NOTE_MAX);
		TempEnterNote(static_cast<ModCommand::NOTE>(note));
		// Memorize note for key-up
		ASSERT(size_t(val) < CountOf(octaveKeyMemory));
		octaveKeyMemory[val] = target.note;
	}
}


// Stop note that has been triggered by entering an octave in the pattern.
void CViewPattern::TempStopOctave(int val)
//----------------------------------------
{
	ASSERT(size_t(val) < CountOf(octaveKeyMemory));
	if(octaveKeyMemory[val] != NOTE_NONE)
	{
		TempStopNote(octaveKeyMemory[val]);
		octaveKeyMemory[val] = NOTE_NONE;
	}
}


// Enter an instrument number in the pattern
void CViewPattern::TempEnterIns(int val)
//--------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	PrepareUndo(m_Cursor, m_Cursor, "Instrument Entry");

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target;		// This is the command we are about to overwrite

	UINT instr = target.instr, nTotalMax, nTempMax;
	if(target.IsPcNote())	// this is a plugin index
	{
		nTotalMax = MAX_MIXPLUGINS + 1;
		nTempMax = MAX_MIXPLUGINS + 1;
	} else if(pSndFile->GetNumInstruments() > 0)	// this is an instrument index
	{
		nTotalMax = MAX_INSTRUMENTS;
		nTempMax = pSndFile->GetNumInstruments();
	} else
	{
		nTotalMax = MAX_SAMPLES;
		nTempMax = pSndFile->GetNumSamples();
	}

	instr = ((instr * 10) + val) % 1000;
	if (instr >= nTotalMax) instr = instr % 100;
	if (nTempMax < 100)			// if we're using samples & have less than 100 samples
		instr = instr % 100;	// or if we're using instruments and have less than 100 instruments
	// --> ensure the entered instrument value is less than 100.
	target.instr = static_cast<ModCommand::INSTR>(instr);

	SetSelToCursor();

	if(target != oldcmd)
	{
		SetModified(false);
		InvalidateCell(m_Cursor);
		UpdateIndicator();
	}

	if(target.IsPcNote())
	{
		m_PCNoteEditMemory = target;
	}
}


// Enter a note in the pattern
void CViewPattern::TempEnterNote(ModCommand::NOTE note, int vol, bool fromMidi)
//-----------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if(pMainFrm == nullptr || pModDoc == nullptr)
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetrSoundFile();

	if(note < NOTE_MIN_SPECIAL)
	{
		Limit(note, sndFile.GetModSpecifications().noteMin, sndFile.GetModSpecifications().noteMax);
	}

	// Special case: Convert note off commands to C00 for MOD files
	if((sndFile.GetType() == MOD_TYPE_MOD) && (note == NOTE_NOTECUT || note == NOTE_FADE || note == NOTE_KEYOFF))
	{
		TempEnterFX(CMD_VOLUME, 0);
		return;
	}

	// Check whether the module format supports the note.
	if(sndFile.GetModSpecifications().HasNote(note) == false)
	{
		return;
	}

	const bool liveRecord = IsLiveRecord();
	const bool usePlaybackPosition = (liveRecord && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_AUTODELAY) && !sndFile.m_SongFlags[SONG_STEP]);
	const bool isSpecial = note >= NOTE_MIN_SPECIAL;
	const bool isSplit = IsNoteSplit(note);

	ModCommandPos editPos = GetEditPos(sndFile, usePlaybackPosition);

	const bool recordEnabled = IsEditingEnabled();
	CHANNELINDEX nChn = GetCurrentChannel();

	BYTE recordGroup = pModDoc->IsChannelRecord(nChn);

	if(!isSpecial && pModDoc->GetSplitKeyboardSettings().IsSplitActive()
		&& ((recordGroup == 1 && isSplit) || (recordGroup == 2 && !isSplit)))
	{
		// Record group 1 should be used for normal notes, record group 2 for split notes.
		// If there are any channels assigned to the "other" record group, we switch to another channel.
		const CHANNELINDEX newChannel = FindGroupRecordChannel(3 - recordGroup, true);
		if(newChannel != CHANNELINDEX_INVALID)
		{
			// Found a free channel, switch to other record group.
			nChn = newChannel;
			recordGroup = 3 - recordGroup;
		}
	}

	// -- Chord autodetection: step back if we just entered a note
	if (recordEnabled && recordGroup && !liveRecord && !ModCommand::IsPcNote(note))
	{
		if (m_nSpacing > 0 && m_nSpacing <= MAX_SPACING)
		{
			if ((timeGetTime() - m_dwLastNoteEntryTime < TrackerSettings::Instance().gnAutoChordWaitTime)
				&& (editPos.row >= m_nSpacing) && (!m_bLastNoteEntryBlocked))
			{
				editPos.row -= m_nSpacing;
			}
		}
	}
	m_dwLastNoteEntryTime = timeGetTime();

	// Quantize
	const bool doQuantize = (liveRecord || (fromMidi && (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN))) && TrackerSettings::Instance().recordQuantizeRows != 0;
	if(doQuantize)
	{
		QuantizeRow(editPos.pattern, editPos.row);
		// "Grace notes" are stuffed into the next row, if possible
		if(sndFile.Patterns[editPos.pattern].GetpModCommand(editPos.row, nChn)->IsNote() && editPos.row < sndFile.Patterns[editPos.pattern].GetNumRows() - 1)
		{
			editPos.row++;
		}
	}

	// -- Work out where to put the new note
	ModCommand *pTarget = sndFile.Patterns[editPos.pattern].GetpModCommand(editPos.row, nChn);
	ModCommand newcmd = *pTarget;

	// Param control 'note'
	if(ModCommand::IsPcNote(note))
	{
		if(!pTarget->IsPcNote())
		{
			// We're overwriting a normal cell with a PC note.
			newcmd = m_PCNoteEditMemory;
		} else if(recordEnabled)
		{
			// Pick up current entry to update PC note edit memory.
			m_PCNoteEditMemory = newcmd;
		}

		newcmd.note = note;
	} else
	{

		// Are we overwriting a PC note here?
		if(pTarget->IsPcNote())
		{
			newcmd.Clear();
		}

		// -- write note and instrument data.
		HandleSplit(newcmd, note);

		// Nice idea actually: Use lower section of the keyboard to play chords (but it won't work 100% correctly this way...)
		/*if(isSplit)
		{
			TempEnterChord(note);
			return;
		}*/

		// -- write vol data
		int volWrite = -1;
		if (vol >= 0 && vol <= 64 && !(isSplit && pModDoc->GetSplitKeyboardSettings().splitVolume))	//write valid volume, as long as there's no split volume override.
		{
			volWrite = vol;
		} else if (isSplit && pModDoc->GetSplitKeyboardSettings().splitVolume)	//cater for split volume override.
		{
			if (pModDoc->GetSplitKeyboardSettings().splitVolume > 0 && pModDoc->GetSplitKeyboardSettings().splitVolume <= 64)
			{
				volWrite = pModDoc->GetSplitKeyboardSettings().splitVolume;
			}
		}

		if(volWrite != -1 && !isSpecial)
		{
			if(sndFile.GetModSpecifications().HasVolCommand(VOLCMD_VOLUME))
			{
				newcmd.volcmd = VOLCMD_VOLUME;
				newcmd.vol = (ModCommand::VOL)volWrite;
			} else
			{
				newcmd.command = CMD_VOLUME;
				newcmd.param = (ModCommand::PARAM)volWrite;
			}
		}

		// -- write sdx if playing live
		if(usePlaybackPosition && m_nPlayTick && !doQuantize)	// avoid SD0 which will be mis-interpreted
		{
			if(newcmd.command == CMD_NONE)	//make sure we don't overwrite any existing commands.
			{
				newcmd.command = CMD_S3MCMDEX;
				if(!sndFile.GetModSpecifications().HasCommand(CMD_S3MCMDEX)) newcmd.command = CMD_MODCMDEX;
				UINT maxSpeed = 0x0F;
				if(m_nTicksOnRow > 0) maxSpeed = MIN(0x0F, m_nTicksOnRow - 1);
				newcmd.param = static_cast<ModCommand::PARAM>(0xD0 | MIN(maxSpeed, m_nPlayTick));
			}
		}

		// Note cut/off/fade: erase instrument number
		if(newcmd.note >= NOTE_MIN_SPECIAL)
			newcmd.instr = 0;

	}

	// -- if recording, create undo point and write out modified command.
	const bool modified = (recordEnabled && *pTarget != newcmd);
	if (modified)
	{
		pModDoc->GetPatternUndo().PrepareUndo(editPos.pattern, nChn, editPos.row, 1, 1, "Note Entry");
		*pTarget = newcmd;
	}

	// -- play note
	if (((TrackerSettings::Instance().m_dwPatternSetup & (PATTERN_PLAYNEWNOTE|PATTERN_PLAYEDITROW)) || !recordEnabled) && !newcmd.IsPcNote())
	{
		const bool playWholeRow = ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !liveRecord);
		if (playWholeRow)
		{
			// play the whole row in "step mode"
			PatternStep(editPos.row);
		}
		if (!playWholeRow || !recordEnabled)
		{
			// NOTE: This code is *also* used for the PATTERN_PLAYEDITROW edit mode because of some unforseeable race conditions when modifying pattern data.
			// We have to use this code when editing is disabled or else we will get some stupid hazards, because we would first have to write the new note
			// data to the pattern and then remove it again - but often, it is actually removed before the row is parsed by the soundlib.

			// just play the newly inserted note using the already specified instrument...
			ModCommand::INSTR nPlayIns = newcmd.instr;
			if(!nPlayIns && ModCommand::IsNoteOrEmpty(note))
			{
				// ...or one that can be found on a previous row of this pattern.
				ModCommand *search = pTarget;
				ROWINDEX srow = editPos.row;
				while(srow-- > 0)
				{
					search -= sndFile.GetNumChannels();
					if (search->instr && !search->IsPcNote())
					{
						nPlayIns = search->instr;
						m_nFoundInstrument = nPlayIns;  //used to figure out which instrument to stop on key release.
						break;
					}
				}
			}
			bool isPlaying = ((pMainFrm->GetModPlaying() == pModDoc) && (pMainFrm->IsPlaying()));
			pModDoc->CheckNNA(newcmd.note, nPlayIns, m_baPlayingNote);
			pModDoc->PlayNote(newcmd.note, nPlayIns, 0, !isPlaying, 4 * vol, 0, 0, nChn);
		}
	}

	if(newcmd.IsNote())
	{
		previousNote[nChn] = note;
	}

	// -- if recording, handle post note entry behaviour (move cursor etc..)
	if(recordEnabled)
	{
		PatternCursor sel(editPos.row, nChn, m_Cursor.GetColumnType());
		if(!liveRecord)
		{
			// Update only when not recording live.
			SetCurSel(sel);
		}

		if(modified) // Has it really changed?
		{
			SetModified(false);
			if(editPos.pattern == m_nPattern)
				InvalidateCell(sel);
			else
				InvalidatePattern();
			if(!liveRecord)
			{
				// Update only when not recording live.
				UpdateIndicator();
			}
		}

		// Set new cursor position (edit step aka row spacing)
		if(!liveRecord)
		{
			if((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING))
			{
				if(editPos.row + m_nSpacing < sndFile.Patterns[editPos.pattern].GetNumRows() || (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL))
				{
					SetCurrentRow(editPos.row + m_nSpacing, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) != 0);
					m_bLastNoteEntryBlocked = false;
				} else
				{
					// if the cursor is block by the end of the pattern here,
					// we must remember to not step back should the next note form a chord.
					m_bLastNoteEntryBlocked = true;
				}

			}

			SetSelToCursor();
		}

		if(newcmd.IsPcNote())
		{
			// Nothing to do here anymore.
			return;
		}
			
		BYTE *activeNoteMap = isSplit ? splitActiveNoteChannel : activeNoteChannel;
		if (newcmd.note <= NOTE_MAX)
			activeNoteMap[newcmd.note] = static_cast<BYTE>(nChn);

		if (recordGroup)
		{
			// Move to next channel in record group
			nChn = FindGroupRecordChannel(recordGroup, false, nChn + 1);
			if(nChn != CHANNELINDEX_INVALID)
			{
				SetCurrentColumn(nChn);
			}
		}
	}
}


// Construct a chord from the chord presets. Returns number of notes in chord.
int CViewPattern::ConstructChord(int note, ModCommand::NOTE (&outNotes)[MPTChord::notesPerChord], ModCommand::NOTE baseNote)
//--------------------------------------------------------------------------------------------------------------------------
{
	const MPTChords &chords = TrackerSettings::GetChords();
	UINT baseOctave = CMainFrame::GetMainFrame()->GetBaseOctave();
	UINT chordNum = note - baseOctave * 12 - NOTE_MIN;

	if(chordNum >= CountOf(chords))
	{
		return 0;
	}
	const MPTChord &chord = chords[chordNum];

	const bool relativeMode = (chord.key == MPTChord::relativeMode);	// Notes are relative to a previously entered note in the pattern
	ModCommand::NOTE key;
	if(relativeMode)
	{
		// Relative mode: Use pattern note as base note.
		// If there is no valid note in the pattern: Use shortcut note as relative base note
		key = ModCommand::IsNote(baseNote) ? baseNote : static_cast<ModCommand::NOTE>(note);
	} else
	{
		// Default mode: Use base key
		key = static_cast<ModCommand::NOTE>(chord.key + baseOctave * 12 + NOTE_MIN);
	}
	if(!ModCommand::IsNote(key))
	{
		return 0;
	}

	int numNotes = 0;
	const CModSpecifications &specs = GetSoundFile()->GetModSpecifications();
	if(specs.HasNote(key))
	{
		outNotes[numNotes++] = key;
	}

	for(size_t i = 0; i < CountOf(chord.notes); i++)
	{
		if(chord.notes[i])
		{
			ModCommand::NOTE note = key - NOTE_MIN;
			if(!relativeMode)
			{
				// Only use octave information from the base key
				note = (note / 12) * 12;
			}
			note += chord.notes[i];
			if(specs.HasNote(note))
			{
				outNotes[numNotes++] = note;
			}
		}
	}
	return numNotes;
}


// Enter a chord in the pattern
void CViewPattern::TempEnterChord(ModCommand::NOTE note)
//------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if(pMainFrm == nullptr || pModDoc == nullptr)
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	const CHANNELINDEX chn = GetCurrentChannel();
	const PatternRow rowBase = sndFile.Patterns[m_nPattern].GetRow(GetCurrentRow());

	ModCommand::NOTE chordNotes[MPTChord::notesPerChord], baseNote = rowBase[chn].note;
	if(!ModCommand::IsNote(baseNote))
	{
		baseNote = prevChordBaseNote;
	}
	int numNotes = ConstructChord(note, chordNotes, baseNote);
	if(!numNotes)
	{
		return;
	}

	// Save old row contents
	std::vector<ModCommand> newRow(rowBase, rowBase + sndFile.GetNumChannels());

	const bool liveRecord = IsLiveRecord();
	const bool recordEnabled = IsEditingEnabled();
	bool modified = false;

	// -- establish note data
	HandleSplit(newRow[chn], note);
	const BYTE recordGroup = pModDoc->IsChannelRecord(chn);

	CHANNELINDEX curChn = chn;
	for(int i = 0; i < numNotes; i++)
	{
		// Find appropriate channel
		while(curChn < sndFile.GetNumChannels() && pModDoc->IsChannelRecord(curChn) != recordGroup)
		{
			curChn++;
		}
		if(curChn >= sndFile.GetNumChannels())
		{
			numNotes = i;
			break;
		}

		chordPatternChannels[i] = curChn;
		
		ModCommand &m = newRow[curChn];
		previousNote[curChn] = m.note = chordNotes[i];
		if(newRow[chn].instr)
		{
			m.instr = newRow[chn].instr;
		}

		if(rowBase[chn] != m)
		{
			modified = true;
		}
		curChn++;
	}

	m_Status.set(psChordPlaying);

	// -- write notedata
	if(recordEnabled)
	{
		SetSelToCursor();

		if(modified)
		{
			// Simply backup the whole row.
			pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, chn, GetCurrentRow(), sndFile.GetNumChannels(), 1, "Chord Entry");

			for(CHANNELINDEX n = 0; n < sndFile.GetNumChannels(); n++)
			{
				rowBase[n] = newRow[n];
			}
			SetModified(false);
			InvalidateRow();
			UpdateIndicator();
		}
	}

	// -- play note
	if((TrackerSettings::Instance().m_dwPatternSetup & (PATTERN_PLAYNEWNOTE | PATTERN_PLAYEDITROW)) || !recordEnabled)
	{
		if(prevChordNote != NOTE_NONE)
		{
			TempStopChord(prevChordNote);
		}

		const bool playWholeRow = ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !liveRecord);
		if(playWholeRow)
		{
			// play the whole row in "step mode"
			PatternStep(GetCurrentRow());
		}
		if(!playWholeRow || !recordEnabled)
		{
			// NOTE: This code is *also* used for the PATTERN_PLAYEDITROW edit mode because of some unforseeable race conditions when modifying pattern data.
			// We have to use this code when editing is disabled or else we will get some stupid hazards, because we would first have to write the new note
			// data to the pattern and then remove it again - but often, it is actually removed before the row is parsed by the soundlib.
			// just play the newly inserted notes...

			ModCommand &firstNote = rowBase[chn];
			ModCommand::INSTR nPlayIns = 0;
			if(firstNote.instr)
			{
				// ...using the already specified instrument
				nPlayIns = firstNote.instr;
			} else if(!firstNote.instr)
			{
				// ...or one that can be found on a previous row of this pattern.
				ModCommand *search = &firstNote;
				ROWINDEX srow = GetCurrentRow();
				while(srow-- > 0)
				{
					search -= sndFile.GetNumChannels();
					if(search->instr)
					{
						nPlayIns = search->instr;
						m_nFoundInstrument = nPlayIns;  //used to figure out which instrument to stop on key release.
						break;
					}
				}
			}
			const bool isPlaying = pMainFrm->GetModPlaying() == pModDoc && pMainFrm->IsPlaying();
			for(int i = 0; i < numNotes; i++)
			{
				pModDoc->CheckNNA(note, nPlayIns, m_baPlayingNote);
				pModDoc->PlayNote(chordNotes[i], nPlayIns, 0, !isPlaying && i == 0, -1, 0, 0, chn);
			}
		}
	} // end play note

	prevChordNote = note;
	prevChordBaseNote = baseNote;

	// Set new cursor position (edit step aka row spacing) - only when not recording live
	if(recordEnabled && !liveRecord)
	{
		if(m_nSpacing > 0 && m_nSpacing <= MAX_SPACING)
		{
			// Shift from entering chord may have triggered this flag, which will prevent us from wrapping to the next pattern.
			m_Status.reset(psKeyboardDragSelect);
			SetCurrentRow(GetCurrentRow() + m_nSpacing, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) != 0);
		}
		SetSelToCursor();
	}
}


// Translate incoming MIDI aftertouch messages to pattern commands
void CViewPattern::EnterAftertouch(ModCommand::NOTE note, int atValue)
//--------------------------------------------------------------------
{
	if(TrackerSettings::Instance().aftertouchBehaviour == atDoNotRecord || !IsEditingEnabled())
	{
		return;
	}

	PatternCursor cursor(m_Cursor);

	if(ModCommand::IsNote(note))
	{
		// For polyphonic aftertouch, map the aftertouch note to the correct pattern channel.
		BYTE *activeNoteMap = IsNoteSplit(note) ? splitActiveNoteChannel : activeNoteChannel;
		if(activeNoteMap[note] < GetSoundFile()->GetNumChannels())
		{
			cursor.SetColumn(activeNoteMap[note], PatternCursor::firstColumn);
		} else
		{
			// Couldn't find the channel that belongs to this note... Don't bother writing aftertouch messages.
			// This is actually necessary, because it is possible that the last aftertouch message for a note
			// is received after the key-off event, in which case OpenMPT won't know anymore on which channel
			// that particular note was, so it will just put the message on some other channel. We don't want that!
			return;
		}
	}

	Limit(atValue, 0, 127);

	ModCommand &target = GetModCommand(cursor);
	ModCommand newCommand = target;
	const CModSpecifications &specs = GetSoundFile()->GetModSpecifications();

	if(target.IsPcNote())
	{
		return;
	}

	switch(TrackerSettings::Instance().aftertouchBehaviour)
	{
	case atRecordAsVolume:
		// Record aftertouch messages as volume commands
		if(specs.HasVolCommand(VOLCMD_VOLUME))
		{
			if(newCommand.volcmd == VOLCMD_NONE || newCommand.volcmd == VOLCMD_VOLUME)
			{
				newCommand.volcmd = VOLCMD_VOLUME;
				newCommand.vol = static_cast<ModCommand::VOL>((atValue * 64 + 64) / 127);
			}
		} else if(specs.HasCommand(CMD_VOLUME))
		{
			if(newCommand.command == CMD_NONE || newCommand.command == CMD_VOLUME)
			{
				newCommand.command = CMD_VOLUME;
				newCommand.param = static_cast<ModCommand::PARAM>((atValue * 64 + 64) / 127);
			}
		}
		break;

	case atRecordAsMacro:
		// Record aftertouch messages as MIDI Macros
		if(newCommand.command == CMD_NONE || newCommand.command == CMD_SMOOTHMIDI || newCommand.command == CMD_MIDI)
		{
			ModCommand::COMMAND cmd = static_cast<ModCommand::COMMAND>(
				specs.HasCommand(CMD_SMOOTHMIDI) ? CMD_SMOOTHMIDI :
				specs.HasCommand(CMD_MIDI) ? CMD_MIDI :
				CMD_NONE);

			if(cmd != CMD_NONE)
			{
				newCommand.command = cmd;
				newCommand.param = static_cast<ModCommand::PARAM>(atValue);
			}
		}
		break;
	}

	if(target != newCommand)
	{
		PrepareUndo(cursor, cursor, "Aftertouch Entry");
		target = newCommand;

		SetModified(false);
		InvalidateCell(cursor);
		UpdateIndicator();
	}
}


// Apply quantization factor to given row.
void CViewPattern::QuantizeRow(PATTERNINDEX &pat, ROWINDEX &row) const
//--------------------------------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || TrackerSettings::Instance().recordQuantizeRows == 0)
	{
		return;
	}

	const ROWINDEX currentTick = m_nTicksOnRow * row + m_nPlayTick;
	const ROWINDEX ticksPerNote = TrackerSettings::Instance().recordQuantizeRows * m_nTicksOnRow;
	
	// Previous quantization step
	const ROWINDEX quantLow = (currentTick / ticksPerNote) * ticksPerNote;
	// Next quantization step
	const ROWINDEX quantHigh = (1 + (currentTick / ticksPerNote)) * ticksPerNote;

	if(currentTick - quantLow < quantHigh - currentTick)
	{
		row = quantLow / m_nTicksOnRow;
	} else
	{
		row = quantHigh / m_nTicksOnRow;
	}
	
	if(!sndFile->Patterns[pat].IsValidRow(row))
	{
		// Quantization exceeds current pattern, try stuffing note into next pattern instead.
		PATTERNINDEX nextPat = sndFile->m_SongFlags[SONG_PATTERNLOOP] ? m_nPattern : GetNextPattern();
		if(nextPat != PATTERNINDEX_INVALID)
		{
			pat = nextPat;
			row = 0;
		} else
		{
			row = sndFile->Patterns[pat].GetNumRows() - 1;
		}
	}
}


// Get previous pattern in order list
PATTERNINDEX CViewPattern::GetPrevPattern() const
//-----------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile != nullptr)
	{
		const ORDERINDEX curOrder = GetCurrentOrder();
		if(curOrder > 0 && m_nPattern == sndFile->Order[curOrder])
		{
			const ORDERINDEX nextOrder = sndFile->Order.GetPreviousOrderIgnoringSkips(curOrder);
			const PATTERNINDEX nextPat = sndFile->Order[nextOrder];
			if(sndFile->Patterns.IsValidPat(nextPat) && sndFile->Patterns[nextPat].GetNumRows())
			{
				return nextPat;
			}
		}
	}
	return PATTERNINDEX_INVALID;
}


// Get follow-up pattern in order list
PATTERNINDEX CViewPattern::GetNextPattern() const
//-----------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile != nullptr)
	{
		const ORDERINDEX curOrder = GetCurrentOrder();
		if(curOrder + 1 < sndFile->Order.size() && m_nPattern == sndFile->Order[curOrder])
		{
			const ORDERINDEX nextOrder = sndFile->Order.GetNextOrderIgnoringSkips(curOrder);
			const PATTERNINDEX nextPat = sndFile->Order[nextOrder];
			if(sndFile->Patterns.IsValidPat(nextPat) && sndFile->Patterns[nextPat].GetNumRows())
			{
				return nextPat;
			}
		}
	}
	return PATTERNINDEX_INVALID;
}


void CViewPattern::OnSetQuantize()
//--------------------------------
{
	CInputDlg dlg(this, _T("Quantize amount in rows for live recording (0 to disable):"), 0, MAX_PATTERN_ROWS, TrackerSettings::Instance().recordQuantizeRows);
	if(dlg.DoModal())
	{
		TrackerSettings::Instance().recordQuantizeRows = static_cast<ROWINDEX>(dlg.resultAsInt);
	}
}


// Find a free channel for a record group, starting search from a given channel.
// If forceFreeChannel is true and all channels in the specified record group are active, some channel is picked from the specified record group.
CHANNELINDEX CViewPattern::FindGroupRecordChannel(BYTE recordGroup, bool forceFreeChannel, CHANNELINDEX startChannel) const
//-------------------------------------------------------------------------------------------------------------------------
{
	const CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
	{
		return CHANNELINDEX_INVALID;
	}

	CHANNELINDEX nChn = startChannel;
	CHANNELINDEX foundChannel = CHANNELINDEX_INVALID;

	for(CHANNELINDEX i = 1; i < pModDoc->GetNumChannels(); i++, nChn++)
	{
		if(nChn >= pModDoc->GetNumChannels())
		{
			nChn = 0; // loop around
		}

		if(pModDoc->IsChannelRecord(nChn) == recordGroup)
		{
			// Check if any notes are playing on this channel
			bool channelLocked = false;
			for(size_t k = 0; k < CountOf(activeNoteChannel); k++)
			{
				if (activeNoteChannel[k] == nChn || splitActiveNoteChannel[k] == nChn)
				{
					channelLocked = true;
					break;
				}
			}

			if (!channelLocked)
			{
				// Channel belongs to correct record group and no note is currently playing.
				return nChn;
			}

			if(forceFreeChannel)
			{
				// If all channels are active, we might still pick a random channel from the specified group.
				foundChannel = nChn;
			}
		}
	}
	return foundChannel;
}


void CViewPattern::OnClearField(const RowMask &mask, bool step, bool ITStyle)
//---------------------------------------------------------------------------
{
 	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	// If we have a selection, we want to do something different
	if(m_Selection.GetUpperLeft() != m_Selection.GetLowerRight())
	{
		OnClearSelection(ITStyle);
		return;
	}

	PrepareUndo(m_Cursor, m_Cursor, "Clear Field");

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target;

	if(mask.note)
	{
		// Clear note
		if(target.IsPcNote())
		{
			// Need to clear entire field if this is a PC Event.
			target.Clear();
		} else
		{
			target.note = NOTE_NONE;
			if(ITStyle)
			{
				target.instr = 0;
			}
		}
	}
	if(mask.instrument)
	{
		// Clear instrument
		target.instr = 0;
	}
	if(mask.volume)
	{
		// Clear volume effect
		target.volcmd = VOLCMD_NONE;
		target.vol = 0;
	}
	if(mask.command)
	{
		// Clear effect command
		target.command = CMD_NONE;
	}
	if(mask.parameter)
	{
		// Clear effect parameter
		target.param = 0;
	}

	if((mask.command || mask.parameter) && (target.IsPcNote()))
	{
		target.SetValueEffectCol(0);
	}

	SetSelToCursor();

	if(target != oldcmd)
	{
		SetModified(false);
		InvalidateRow();
		UpdateIndicator();
	}

	if(step && (pSndFile->IsPaused() || !m_Status[psFollowSong] ||
		(CMainFrame::GetMainFrame() != nullptr && CMainFrame::GetMainFrame()->GetFollowSong(GetDocument()) != m_hWnd)))
	{
		// Preview Row
		if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !IsLiveRecord())
		{
			PatternStep(GetCurrentRow());
		}

		if ((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING)) 
			SetCurrentRow(GetCurrentRow() + m_nSpacing);

		SetSelToCursor();
	}
}



void CViewPattern::OnInitMenu(CMenu* pMenu)
//-----------------------------------------
{
	CModScrollView::OnInitMenu(pMenu);
}

void CViewPattern::TogglePluginEditor(int chan)
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(!pSndFile) return;

	int plug = pSndFile->ChnSettings[chan].nMixPlugin;
	if(plug > 0)
		pModDoc->TogglePluginEditor(plug - 1);

	return;
}


void CViewPattern::OnSelectInstrument(UINT nID)
//---------------------------------------------
{
	SetSelectionInstrument(static_cast<INSTRUMENTINDEX>(nID - ID_CHANGE_INSTRUMENT));
}


void CViewPattern::OnSelectPCNoteParam(UINT nID)
//----------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	uint16 paramNdx = static_cast<uint16>(nID - ID_CHANGE_PCNOTE_PARAM);
	bool bModified = false;
	for(ROWINDEX nRow = m_Selection.GetStartRow(); nRow <= m_Selection.GetEndRow(); nRow++)
	{
		ModCommand *p = pSndFile->Patterns[m_nPattern].GetRow(nRow);
		for(CHANNELINDEX nChn = m_Selection.GetStartChannel(); nChn <= m_Selection.GetEndChannel(); nChn++, p++)
		{
			if(p && p->IsPcNote() && (p->GetValueVolCol() != paramNdx))
			{
				bModified = true;
				p->SetValueVolCol(paramNdx);
			}
		}
	}
	if(bModified)
	{
		SetModified();
		InvalidatePattern();
	}
}


void CViewPattern::OnSelectPlugin(UINT nID)
//-----------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	const CHANNELINDEX plugChannel = m_MenuCursor.GetChannel();
	if(plugChannel < pSndFile->GetNumChannels())
	{
		PLUGINDEX newPlug = static_cast<PLUGINDEX>(nID - ID_PLUGSELECT);
		if(newPlug <= MAX_MIXPLUGINS && newPlug != pSndFile->ChnSettings[plugChannel].nMixPlugin)
		{
			pSndFile->ChnSettings[plugChannel].nMixPlugin = newPlug;
			if(pSndFile->GetModSpecifications().supportsPlugins)
			{
				SetModified(false);
			}
			InvalidateChannelsHeaders();
		}
	}
}


bool CViewPattern::HandleSplit(ModCommand &m, int note)
//-----------------------------------------------------
{
	ModCommand::INSTR ins = static_cast<ModCommand::INSTR>(GetCurrentInstrument());
	const bool isSplit = IsNoteSplit(note);

	if(isSplit)
	{
		CModDoc *pModDoc = GetDocument();
		CSoundFile *pSndFile;
		if (pModDoc == nullptr || (pSndFile = pModDoc->GetSoundFile()) == nullptr) return false;

		if (pModDoc->GetSplitKeyboardSettings().octaveLink && note <= NOTE_MAX)
		{
			note += 12 * pModDoc->GetSplitKeyboardSettings().octaveModifier;
			Limit(note, pSndFile->GetModSpecifications().noteMin, pSndFile->GetModSpecifications().noteMax);
		}
		if (pModDoc->GetSplitKeyboardSettings().splitInstrument)
		{
			ins = pModDoc->GetSplitKeyboardSettings().splitInstrument;
		}
	}
	
	m.note = static_cast<ModCommand::NOTE>(note);
	if(ins)
	{
		m.instr = ins;
	}

	return isSplit;
}


bool CViewPattern::IsNoteSplit(int note) const
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	return(pModDoc != nullptr
		&& pModDoc->GetSplitKeyboardSettings().IsSplitActive()
		&& note <= pModDoc->GetSplitKeyboardSettings().splitNote);
}


bool CViewPattern::BuildPluginCtxMenu(HMENU hMenu, UINT nChn, CSoundFile *pSndFile) const
//---------------------------------------------------------------------------------------
{
	for(PLUGINDEX plug = 0; plug <= MAX_MIXPLUGINS; plug++)
	{

		bool itemFound = false;

		CHAR s[64];

		if(!plug)
		{
			strcpy(s, "No plugin");
			itemFound = true;
		} else
		{
			const SNDMIXPLUGIN &plugin = pSndFile->m_MixPlugins[plug - 1];
			if(plugin.IsValidPlugin())
			{
				wsprintf(s, "FX%d: %s", plug, plugin.GetName());
				itemFound = true;
			}
		}

		if (itemFound)
		{
			if (plug == pSndFile->ChnSettings[nChn].nMixPlugin)
			{
				AppendMenu(hMenu, (MF_STRING | MF_CHECKED), ID_PLUGSELECT + plug, s);
			} else
			{
				AppendMenu(hMenu, MF_STRING, ID_PLUGSELECT + plug, s);
			}
		}
	}
	return true;
}

bool CViewPattern::BuildSoloMuteCtxMenu(HMENU hMenu, CInputHandler *ih, UINT nChn, CSoundFile *pSndFile) const
//------------------------------------------------------------------------------------------------------------
{
	AppendMenu(hMenu, pSndFile->ChnSettings[nChn].dwFlags[CHN_MUTE] ? 
						(MF_STRING|MF_CHECKED) : MF_STRING, ID_PATTERN_MUTE, 
						"&Mute Channel\t" + ih->GetKeyTextFromCommand(kcChannelMute));
	bool bSolo = false, bUnmuteAll = false;
	bool bSoloPending = false, bUnmuteAllPending = false; // doesn't work perfectly yet

	for (CHANNELINDEX i = 0; i < pSndFile->GetNumChannels(); i++)
	{
		if(i != nChn)
		{
			if(!pSndFile->ChnSettings[i].dwFlags[CHN_MUTE]) bSolo = bSoloPending = true;
			if(pSndFile->ChnSettings[i].dwFlags[CHN_MUTE] && pSndFile->m_bChannelMuteTogglePending[i]) bSoloPending = true;
		} else
		{
			if(pSndFile->ChnSettings[i].dwFlags[CHN_MUTE]) bSolo = bSoloPending = true;
			if(!pSndFile->ChnSettings[i].dwFlags[CHN_MUTE] && pSndFile->m_bChannelMuteTogglePending[i]) bSoloPending = true;
		}
		if(pSndFile->ChnSettings[i].dwFlags[CHN_MUTE]) bUnmuteAll = bUnmuteAllPending = true;
		if(!pSndFile->ChnSettings[i].dwFlags[CHN_MUTE] && pSndFile->m_bChannelMuteTogglePending[i]) bUnmuteAllPending = true;
	}
	if (bSolo) AppendMenu(hMenu, MF_STRING, ID_PATTERN_SOLO, "&Solo Channel\t" + ih->GetKeyTextFromCommand(kcChannelSolo));
	if (bUnmuteAll) AppendMenu(hMenu, MF_STRING, ID_PATTERN_UNMUTEALL, "&Unmute All\t" + ih->GetKeyTextFromCommand(kcChannelUnmuteAll));
	
	AppendMenu(hMenu, 
			pSndFile->m_bChannelMuteTogglePending[nChn] ? (MF_STRING|MF_CHECKED) : MF_STRING,
			 ID_PATTERN_TRANSITIONMUTE,
			pSndFile->ChnSettings[nChn].dwFlags[CHN_MUTE] ?
			"On transition: Unmute\t" + ih->GetKeyTextFromCommand(kcToggleChanMuteOnPatTransition) :
			"On transition: Mute\t" + ih->GetKeyTextFromCommand(kcToggleChanMuteOnPatTransition));

	if (bUnmuteAllPending) AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITION_UNMUTEALL, "On transition: Unmute all\t" + ih->GetKeyTextFromCommand(kcUnmuteAllChnOnPatTransition));
	if (bSoloPending) AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITIONSOLO, "On transition: Solo\t" + ih->GetKeyTextFromCommand(kcSoloChnOnPatTransition));

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_CHNRESET, "&Reset Channel\t" + ih->GetKeyTextFromCommand(kcChannelReset));
	
	return true;
}

bool CViewPattern::BuildRecordCtxMenu(HMENU hMenu, CInputHandler *ih, CHANNELINDEX nChn, CModDoc* pModDoc) const
//--------------------------------------------------------------------------------------------------------------
{
	AppendMenu(hMenu, pModDoc->IsChannelRecord1(nChn) ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_EDIT_RECSELECT, "R&ecord select\t" + ih->GetKeyTextFromCommand(kcChannelRecordSelect));
	AppendMenu(hMenu, pModDoc->IsChannelRecord2(nChn) ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_EDIT_SPLITRECSELECT, "S&plit Record select\t" + ih->GetKeyTextFromCommand(kcChannelSplitRecordSelect));
	return true;
}



bool CViewPattern::BuildRowInsDelCtxMenu(HMENU hMenu, CInputHandler *ih) const
//----------------------------------------------------------------------------
{
	const CString label = (m_Selection.GetStartRow() != m_Selection.GetEndRow() ? "Rows" : "Row");

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_INSERTROW, "&Insert " + label + "\t" + ih->GetKeyTextFromCommand(kcInsertRow));
	AppendMenu(hMenu, MF_STRING, ID_PATTERN_DELETEROW, "&Delete " + label + "\t" + ih->GetKeyTextFromCommand(kcDeleteRows) );
	return true;
}

bool CViewPattern::BuildMiscCtxMenu(HMENU hMenu, CInputHandler *ih) const
//-----------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_SHOWTIMEATROW, "Show row play time\t" + ih->GetKeyTextFromCommand(kcTimeAtRow));
	return true;
}

bool CViewPattern::BuildSelectionCtxMenu(HMENU hMenu, CInputHandler *ih) const
//----------------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECTCOLUMN, _T("Select &Column\t") + ih->GetKeyTextFromCommand(kcSelectColumn));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECT_ALL, _T("Select &Pattern\t") + ih->GetKeyTextFromCommand(kcEditSelectAll));
	return true;
}

bool CViewPattern::BuildGrowShrinkCtxMenu(HMENU hMenu, CInputHandler *ih) const
//-----------------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_GROW_SELECTION, _T("&Grow selection\t") + ih->GetKeyTextFromCommand(kcPatternGrowSelection));
	AppendMenu(hMenu, MF_STRING, ID_SHRINK_SELECTION, _T("&Shrink selection\t") + ih->GetKeyTextFromCommand(kcPatternShrinkSelection));
	return true;
}


bool CViewPattern::BuildInterpolationCtxMenu(HMENU hMenu, CInputHandler *ih) const
//--------------------------------------------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	const bool isPCNote = sndFile->Patterns.IsValidPat(m_nPattern) && sndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel())->IsPcNote();

	HMENU subMenu = CreatePopupMenu();
	bool possible = BuildInterpolationCtxMenu(subMenu, PatternCursor::noteColumn, _T("&Note Column\t") + ih->GetKeyTextFromCommand(kcPatternInterpolateNote), ID_PATTERN_INTERPOLATE_NOTE)
		| BuildInterpolationCtxMenu(subMenu, PatternCursor::instrColumn, (isPCNote ? _T("&Plugin Column\t") : _T("&Instrument Column\t")) + ih->GetKeyTextFromCommand(kcPatternInterpolateInstr), ID_PATTERN_INTERPOLATE_INSTR)
		| BuildInterpolationCtxMenu(subMenu, PatternCursor::volumeColumn, (isPCNote ? _T("&Parameter Column\t") : _T("&Volume Column\t")) + ih->GetKeyTextFromCommand(kcPatternInterpolateVol), ID_PATTERN_INTERPOLATE_VOLUME)
		| BuildInterpolationCtxMenu(subMenu, PatternCursor::effectColumn, (isPCNote ? _T("&Value Column\t") : _T("&Effect Column\t")) + ih->GetKeyTextFromCommand(kcPatternInterpolateEffect), ID_PATTERN_INTERPOLATE_EFFECT);
	if(possible || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_POPUP | (possible ? 0 : MF_GRAYED), reinterpret_cast<UINT_PTR>(subMenu), "I&nterpolate...");
		return true;
	}
	return false;
}


bool CViewPattern::BuildInterpolationCtxMenu(HMENU hMenu, PatternCursor::Columns colType, CString label, UINT command) const
//--------------------------------------------------------------------------------------------------------------------------
{
	bool possible = IsInterpolationPossible(colType);
	if(!possible && colType == PatternCursor::effectColumn)
	{
		// Extend search to param column
		possible = IsInterpolationPossible(PatternCursor::paramColumn);
	}

	if(possible || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | (possible ? 0 : MF_GRAYED), command, label);
	}

	return possible;
}


bool CViewPattern::BuildEditCtxMenu(HMENU hMenu, CInputHandler *ih, CModDoc* pModDoc) const
//-----------------------------------------------------------------------------------------
{
	HMENU pasteSpecialMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_EDIT_CUT, "Cu&t\t" + ih->GetKeyTextFromCommand(kcEditCut));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_COPY, "&Copy\t" + ih->GetKeyTextFromCommand(kcEditCopy));
	AppendMenu(hMenu, MF_STRING | (PatternClipboard::CanPaste() ? 0 : MF_GRAYED), ID_EDIT_PASTE, "&Paste\t" + ih->GetKeyTextFromCommand(kcEditPaste));
	AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(pasteSpecialMenu), "Paste Special");
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_MIXPASTE, "&Mix Paste\t" + ih->GetKeyTextFromCommand(kcEditMixPaste));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_MIXPASTE_ITSTYLE, "M&ix Paste (IT Style)\t" + ih->GetKeyTextFromCommand(kcEditMixPasteITStyle));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_PASTEFLOOD, "Paste Fl&ood\t" + ih->GetKeyTextFromCommand(kcEditPasteFlood));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_PUSHFORWARDPASTE, "&Push Forward Paste (Insert)\t" + ih->GetKeyTextFromCommand(kcEditPushForwardPaste));

	DWORD greyed = pModDoc->GetPatternUndo().CanUndo() ? MF_ENABLED : MF_GRAYED;
	if (!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_EDIT_UNDO, "&Undo\t" + ih->GetKeyTextFromCommand(kcEditUndo));
	}
	greyed = pModDoc->GetPatternUndo().CanRedo() ? MF_ENABLED : MF_GRAYED;
	if (!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_EDIT_REDO, "&Redo\t" + ih->GetKeyTextFromCommand(kcEditRedo));
	}

	AppendMenu(hMenu, MF_STRING, ID_CLEAR_SELECTION, "Clear selection\t" + ih->GetKeyTextFromCommand(kcSampleDelete));

	return true;
}

bool CViewPattern::BuildVisFXCtxMenu(HMENU hMenu, CInputHandler *ih) const
//------------------------------------------------------------------------
{
	DWORD greyed = (IsColumnSelected(PatternCursor::effectColumn) || IsColumnSelected(PatternCursor::paramColumn)) ? FALSE : MF_GRAYED;

	if (!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING|greyed, ID_PATTERN_VISUALIZE_EFFECT, "&Visualize Effect\t" + ih->GetKeyTextFromCommand(kcPatternVisualizeEffect));
		return true;
	}
	return false;
}

bool CViewPattern::BuildRandomCtxMenu(HMENU hMenu, CInputHandler *ih) const
//------------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_PATTERN_OPEN_RANDOMIZER, "Randomize...\t" + ih->GetKeyTextFromCommand(kcPatternOpenRandomizer));
	return true;
}

bool CViewPattern::BuildTransposeCtxMenu(HMENU hMenu, CInputHandler *ih) const
//----------------------------------------------------------------------------
{
	HMENU transMenu = CreatePopupMenu();

	std::vector<CHANNELINDEX> validChans;
	DWORD greyed = IsColumnSelected(PatternCursor::noteColumn) ? FALSE : MF_GRAYED;

	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_UP, _T("Transpose +&1\t") + ih->GetKeyTextFromCommand(kcTransposeUp));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_DOWN, _T("Transpose -&1\t") + ih->GetKeyTextFromCommand(kcTransposeDown));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_OCTUP, _T("Transpose +1&2\t") + ih->GetKeyTextFromCommand(kcTransposeOctUp));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_OCTDOWN, _T("Transpose -1&2\t") + ih->GetKeyTextFromCommand(kcTransposeOctDown));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_CUSTOM, _T("&Custom...\t") + ih->GetKeyTextFromCommand(kcTransposeCustom));
		AppendMenu(hMenu, MF_POPUP | greyed, reinterpret_cast<UINT_PTR>(transMenu), _T("&Transpose..."));
		return true;
	}
	return false;
}

bool CViewPattern::BuildAmplifyCtxMenu(HMENU hMenu, CInputHandler *ih) const
//--------------------------------------------------------------------------
{
	std::vector<CHANNELINDEX> validChans;
	DWORD greyed = IsColumnSelected(PatternCursor::volumeColumn) ? 0 : MF_GRAYED;

	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_AMPLIFY, "&Amplify\t" + ih->GetKeyTextFromCommand(kcPatternAmplify));
		return true;
	}
	return false;
}


bool CViewPattern::BuildChannelControlCtxMenu(HMENU hMenu, CInputHandler *ih) const
//---------------------------------------------------------------------------------
{
	const CModSpecifications &specs = GetDocument()->GetrSoundFile().GetModSpecifications();
	CHANNELINDEX numChannels = GetDocument()->GetNumChannels();
	DWORD canAddChannels = (numChannels < specs.channelsMax) ? 0 : MF_GRAYED;
	DWORD canRemoveChannels = (numChannels > specs.channelsMin) ? 0 : MF_GRAYED;

	AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSPOSECHANNEL, "&Transpose Channel\t" + ih->GetKeyTextFromCommand(kcChannelTranspose));
	AppendMenu(hMenu, MF_STRING | canAddChannels, ID_PATTERN_DUPLICATECHANNEL, "&Duplicate Channel\t" + ih->GetKeyTextFromCommand(kcChannelDuplicate));

	HMENU addChannelMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP | canAddChannels, reinterpret_cast<UINT_PTR>(addChannelMenu), "&Add Channel\t");
	AppendMenu(addChannelMenu, MF_STRING, ID_PATTERN_ADDCHANNEL_FRONT, "&Before this channel");
	AppendMenu(addChannelMenu, MF_STRING, ID_PATTERN_ADDCHANNEL_AFTER, "&After this channel");
	
	HMENU removeChannelMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP | canRemoveChannels, reinterpret_cast<UINT_PTR>(removeChannelMenu), "Remo&ve Channel\t");
	AppendMenu(removeChannelMenu, MF_STRING, ID_PATTERN_REMOVECHANNEL, "&Remove this channel\t");
	AppendMenu(removeChannelMenu, MF_STRING, ID_PATTERN_REMOVECHANNELDIALOG, "&Choose channels to remove...\t");


	return false;
}


bool CViewPattern::BuildSetInstCtxMenu(HMENU hMenu, CInputHandler *ih) const
//--------------------------------------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	const CModDoc *modDoc;
	if(sndFile == nullptr || (modDoc = sndFile->GetpModDoc()) == nullptr)
	{
		return false;
	}

	std::vector<CHANNELINDEX> validChans;
	DWORD greyed = IsColumnSelected(PatternCursor::instrColumn) ? 0 : MF_GRAYED;

	if (!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		if((sndFile->Patterns.IsValidPat(m_nPattern)))
		{
			if(sndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel())->IsPcNote())
			{
				// Don't build instrument menu for PC notes.
				return false;
			}
		}

		// Create the new menu and add it to the existing menu.
		HMENU instrumentChangeMenu = ::CreatePopupMenu();
		AppendMenu(hMenu, MF_POPUP | greyed, reinterpret_cast<UINT_PTR>(instrumentChangeMenu), "Change Instrument\t" + ih->GetKeyTextFromCommand(kcPatternSetInstrument));
	
		if(!greyed)
		{
			bool addSeparator = false;
			if (sndFile->GetNumInstruments())
			{
				for(INSTRUMENTINDEX i = 1; i <= sndFile->GetNumInstruments() ; i++)
				{
					if (sndFile->Instruments[i] == NULL)
						continue;

					CString instString = modDoc->GetPatternViewInstrumentName(i, true);
					if(!instString.IsEmpty())
					{
						AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + i, modDoc->GetPatternViewInstrumentName(i));
						addSeparator = true;
					}
				}

			} else
			{
				CHAR s[64];
				for(SAMPLEINDEX i = 1; i <= sndFile->GetNumSamples(); i++) if (sndFile->GetSample(i).pSample != nullptr)
				{
					wsprintf(s, "%02d: %s", i, sndFile->GetSampleName(i));
					AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + i, s);
					addSeparator = true;
				}
			}

			// Add options to remove instrument from selection.
			if(addSeparator)
			{
				AppendMenu(instrumentChangeMenu, MF_SEPARATOR, 0, 0);
			}
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT, _T("&Remove instrument"));
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + GetCurrentInstrument(), _T("&Set to current instrument"));
		}
		return BuildTogglePlugEditorCtxMenu(hMenu, ih);
	}
	return false;
}


// Context menu for Param Control notes
bool CViewPattern::BuildPCNoteCtxMenu(HMENU hMenu, CInputHandler *ih) const
//-------------------------------------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern))
	{
		return false;
	}

	const ModCommand &selStart = *sndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
	if(!selStart.IsPcNote())
	{
		return false;
	}
	
	char s[72];

	// Create sub menu for "change plugin"
	HMENU pluginChangeMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(pluginChangeMenu), "Change Plugin\t" + ih->GetKeyTextFromCommand(kcPatternSetInstrument));
	for(PLUGINDEX nPlg = 0; nPlg < MAX_MIXPLUGINS; nPlg++)
	{
		if(sndFile->m_MixPlugins[nPlg].pMixPlugin != nullptr)
		{
			wsprintf(s, "%02d: %s", nPlg + 1, sndFile->m_MixPlugins[nPlg].GetName());
			AppendMenu(pluginChangeMenu, MF_STRING | ((nPlg + 1) == selStart.instr) ? MF_CHECKED : 0, ID_CHANGE_INSTRUMENT + nPlg + 1, s);
		}
	}

	if(selStart.instr >= 1 && selStart.instr <= MAX_MIXPLUGINS)
	{
		const SNDMIXPLUGIN &plug = sndFile->m_MixPlugins[selStart.instr - 1];
		if(plug.pMixPlugin != nullptr)
		{

			// Create sub menu for "change plugin param"
			HMENU paramChangeMenu = ::CreatePopupMenu();
			AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(paramChangeMenu), "Change Plugin Parameter\t");

			const PlugParamIndex curParam = selStart.GetValueVolCol(), nParams = plug.pMixPlugin->GetNumParameters();

			for(PlugParamIndex i = 0; i < nParams; i++)
			{
				AppendMenu(paramChangeMenu, MF_STRING | (i == curParam) ? MF_CHECKED : 0, ID_CHANGE_PCNOTE_PARAM + i, plug.pMixPlugin->GetFormattedParamName(i));
			}
		}
	}

	return BuildTogglePlugEditorCtxMenu(hMenu, ih);
}


bool CViewPattern::BuildTogglePlugEditorCtxMenu(HMENU hMenu, CInputHandler *ih) const
//-----------------------------------------------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern))
	{
		return false;
	}

	PLUGINDEX plug = 0;
	const ModCommand &selStart = *sndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
	if(selStart.IsPcNote())
	{
		// PC Event
		plug = selStart.instr;
	} else if(selStart.instr > 0 && selStart.instr <= sndFile->GetNumInstruments()
		&& sndFile->Instruments[selStart.instr] != nullptr
		&& sndFile->Instruments[selStart.instr]->nMixPlug)
	{
		// Regular instrument
		plug = sndFile->Instruments[selStart.instr]->nMixPlug;
	}

	if(plug && plug <= MAX_MIXPLUGINS && sndFile->m_MixPlugins[plug - 1].pMixPlugin != nullptr)
	{
		AppendMenu(hMenu, MF_STRING, ID_PATTERN_EDIT_PCNOTE_PLUGIN, "Toggle Plugin &Editor\t" + ih->GetKeyTextFromCommand(kcPatternEditPCNotePlugin));
		return true;
	}
	return false;
}

// Returns an ordered list of all channels in which a given column type is selected.
CHANNELINDEX CViewPattern::ListChansWhereColSelected(PatternCursor::Columns colType, std::vector<CHANNELINDEX> &chans) const
//--------------------------------------------------------------------------------------------------------------------------
{
	CHANNELINDEX startChan = m_Selection.GetStartChannel();
	CHANNELINDEX endChan   = m_Selection.GetEndChannel();
	chans.clear();
	chans.reserve(endChan - startChan + 1);

	// Check in which channels this column is selected.
	// Actually this check is only important for the first and last channel, but to keep things clean and simple, all channels are checked in the same manner.
	for(CHANNELINDEX i = startChan; i <= endChan; i++)
	{
		if(m_Selection.ContainsHorizontal(PatternCursor(0, i, colType)))
		{
			chans.push_back(i);
		}
	}

	return static_cast<CHANNELINDEX>(chans.size());
}


// Check if a column type is selected on any channel in the current selection.
bool CViewPattern::IsColumnSelected(PatternCursor::Columns colType) const
//-----------------------------------------------------------------------
{
	return m_Selection.ContainsHorizontal(PatternCursor(0, m_Selection.GetStartChannel(), colType))
		|| m_Selection.ContainsHorizontal(PatternCursor(0, m_Selection.GetEndChannel(), colType));
}


// Check if the given interpolation type is actually possible in the current selection.
bool CViewPattern::IsInterpolationPossible(PatternCursor::Columns colType) const
//------------------------------------------------------------------------------
{
	std::vector<CHANNELINDEX> validChans;
	ListChansWhereColSelected(colType, validChans);

	ROWINDEX startRow = m_Selection.GetStartRow();
	ROWINDEX endRow   = m_Selection.GetEndRow();
	for(auto iter = validChans.begin(); iter != validChans.end(); iter++)
	{
		if(IsInterpolationPossible(startRow, endRow, *iter, colType))
		{
			return true;
		}
	}
	return false;
}


// Check if the given interpolation type is actually possible in a given channel.
bool CViewPattern::IsInterpolationPossible(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX chan, PatternCursor::Columns colType) const
//-------------------------------------------------------------------------------------------------------------------------------------
{
	const CSoundFile *sndFile = GetSoundFile();
	if(startRow == endRow || sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern))
		return false;

	bool result = false;
	const ModCommand &startRowMC = *sndFile->Patterns[m_nPattern].GetpModCommand(startRow, chan);
	const ModCommand &endRowMC = *sndFile->Patterns[m_nPattern].GetpModCommand(endRow, chan);
	UINT startRowCmd, endRowCmd;

	if(colType == PatternCursor::effectColumn && (startRowMC.IsPcNote() || endRowMC.IsPcNote()))
		return true;

	switch (colType)
	{
		case PatternCursor::noteColumn:
			startRowCmd = startRowMC.note;
			endRowCmd = endRowMC.note;
			result = (startRowCmd == endRowCmd && startRowCmd != NOTE_NONE)		// Interpolate between two identical notes or Cut / Fade / etc...
				|| (startRowCmd != NOTE_NONE && endRowCmd == NOTE_NONE)			// Fill in values from the first row
				|| (startRowCmd == NOTE_NONE && endRowCmd != NOTE_NONE)			// Fill in values from the last row
				|| (ModCommand::IsNoteOrEmpty(startRowMC.note) && ModCommand::IsNoteOrEmpty(endRowMC.note) && !(startRowCmd == NOTE_NONE && endRowCmd == NOTE_NONE));	// Interpolate between two notes of which one may be empty
			break;

		case PatternCursor::instrColumn:
			startRowCmd = startRowMC.instr;
			endRowCmd = endRowMC.instr;
			result = startRowCmd != 0 || endRowCmd != 0;
			break;

		case PatternCursor::volumeColumn:
			startRowCmd = startRowMC.volcmd;
			endRowCmd = endRowMC.volcmd;
			result = (startRowCmd == endRowCmd && startRowCmd != VOLCMD_NONE)	// Interpolate between two identical commands
				|| (startRowCmd != VOLCMD_NONE && endRowCmd == VOLCMD_NONE)		// Fill in values from the first row
				|| (startRowCmd == VOLCMD_NONE && endRowCmd != VOLCMD_NONE);	// Fill in values from the last row
			break;

		case PatternCursor::effectColumn:
		case PatternCursor::paramColumn:
			startRowCmd = startRowMC.command;
			endRowCmd = endRowMC.command;
			result = (startRowCmd == endRowCmd && startRowCmd != CMD_NONE)		// Interpolate between two identical commands
				|| (startRowCmd != CMD_NONE && endRowCmd == CMD_NONE)			// Fill in values from the first row
				|| (startRowCmd == CMD_NONE && endRowCmd != CMD_NONE);			// Fill in values from the last row
			break;

		default:
			result = false;
	}
	return result;
}


void CViewPattern::OnRButtonDblClk(UINT nFlags, CPoint point)
//-----------------------------------------------------------
{
	OnRButtonDown(nFlags, point);
	CModScrollView::OnRButtonDblClk(nFlags, point);
}


// Toggle pending mute status for channel from context menu.
void CViewPattern::OnTogglePendingMuteFromClick()
//-----------------------------------------------
{
	TogglePendingMute(m_MenuCursor.GetChannel());
}


// Toggle pending solo status for channel from context menu.
void CViewPattern::OnPendingSoloChnFromClick()
//--------------------------------------------
{
	PendingSoloChn(m_MenuCursor.GetChannel());
}


// Set pending unmute status for all channels.
void CViewPattern::OnPendingUnmuteAllChnFromClick()
//----------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr)
	{
		GetSoundFile()->PatternTransitionChnUnmuteAll();
		InvalidateChannelsHeaders();
	}
}


// Toggle pending solo status for a channel.
void CViewPattern::PendingSoloChn(CHANNELINDEX nChn)
//--------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr)
	{
		GetSoundFile()->PatternTranstionChnSolo(nChn);
		InvalidateChannelsHeaders();
	}
}


// Toggle pending mute status for a channel.
void CViewPattern::TogglePendingMute(CHANNELINDEX nChn)
//-----------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr)
	{
		pSndFile->m_bChannelMuteTogglePending[nChn] = !pSndFile->m_bChannelMuteTogglePending[nChn];
		InvalidateChannelsHeaders();
	}
}


// Check if editing is enabled, and if it's not, prompt the user to enable editing.
bool CViewPattern::IsEditingEnabled_bmsg()
//----------------------------------------
{
	if(IsEditingEnabled()) return true;
	if(TrackerSettings::Instance().patternNoEditPopup) return false;

	HMENU hMenu;

	if((hMenu = ::CreatePopupMenu()) == nullptr) return false;
	
	CPoint pt = GetPointFromPosition(m_Cursor);

	AppendMenu(hMenu, MF_STRING, IDC_PATTERN_RECORD, "Editing (recording) is disabled; click here to enable it.");

	ClientToScreen(&pt);
	::TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, NULL);

	::DestroyMenu(hMenu);

	return false;
}


// Show playback time at a given pattern position.
void CViewPattern::OnShowTimeAtRow()
//----------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	CString msg;
	ORDERINDEX currentOrder = GetCurrentOrder();
	if(pSndFile->Order[currentOrder] == m_nPattern)
	{
		const double t = pSndFile->GetPlaybackTimeAt(currentOrder, GetCurrentRow(), false, false);
		if(t < 0)
			msg.Format("Unable to determine the time. Possible cause: No order %d, row %d found from play sequence.", currentOrder, GetCurrentRow());
		else
		{
			const uint32 minutes = static_cast<uint32>(t / 60.0);
			const double seconds = t - (minutes * 60);
			msg.Format("Estimate for playback time at order %d (pattern %d), row %d: %d minute%s %.2f seconds.", currentOrder, m_nPattern, GetCurrentRow(), minutes, (minutes == 1) ? "" : "s", seconds);
		}
	} else
	{
		msg.Format("Unable to determine the time: pattern at current order (%d) does not correspond to pattern at pattern view (pattern %d).", currentOrder, m_nPattern);
	}
	
	Reporting::Notification(msg);
}


// Try setting the cursor position to given coordinates.
void CViewPattern::SetEditPos(const CSoundFile& rSndFile,
							  ROWINDEX& iRow, PATTERNINDEX& iPat,
							  const ROWINDEX iRowCandidate, const PATTERNINDEX iPatCandidate) const
//-------------------------------------------------------------------------------------------
{
	if(rSndFile.Patterns.IsValidPat(iPatCandidate) && rSndFile.Patterns[iPatCandidate].IsValidRow(iRowCandidate))
	{ // Case: Edit position candidates are valid -- use them.
		iPat = iPatCandidate;
		iRow = iRowCandidate;
	}
	else // Case: Edit position candidates are not valid -- set edit cursor position instead.
	{
		iPat = m_nPattern;
		iRow = GetCurrentRow();
	}
}


// Set up split keyboard
void CViewPattern::SetSplitKeyboardSettings()
//-------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;

	CSplitKeyboadSettings dlg(CMainFrame::GetMainFrame(), pModDoc->GetrSoundFile(), pModDoc->GetSplitKeyboardSettings());
	if (dlg.DoModal() == IDOK)
	{
		// Update split keyboard settings in other pattern views
		pModDoc->UpdateAllViews(NULL, SampleHint().Names());
	}
}


// Paste pattern data using the given paste mode.
void CViewPattern::ExecutePaste(PatternClipboard::PasteModes mode)
//----------------------------------------------------------------
{
	if(IsEditingEnabled_bmsg() && PastePattern(m_nPattern, m_Selection.GetUpperLeft(), mode))
	{
		InvalidatePattern(false);
		SetFocus();
	}
}


// Show plugin editor for plugin assigned to PC Event at the cursor position.
void CViewPattern::OnTogglePCNotePluginEditor()
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	CSoundFile &sndFile = pModDoc->GetrSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
		return;

	const ModCommand &m = *sndFile.Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
	PLUGINDEX plug = 0;
	if(!m.IsPcNote())
	{
		// No PC note: Toggle instrument's plugin editor
		if(m.instr && m.instr <= sndFile.GetNumInstruments() && sndFile.Instruments[m.instr])
		{
			plug = sndFile.Instruments[m.instr]->nMixPlug;
		}
	} else
	{
		plug = m.instr;
	}

	if(plug > 0 && plug <= MAX_MIXPLUGINS) pModDoc->TogglePluginEditor(plug - 1);
}


// Get the active pattern's rows per beat, or, if they are not overriden, the song's default rows per beat.
ROWINDEX CViewPattern::GetRowsPerBeat() const
//-------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
		return 0;
	if(!pSndFile->Patterns[m_nPattern].GetOverrideSignature())
		return pSndFile->m_nDefaultRowsPerBeat;
	else
		return pSndFile->Patterns[m_nPattern].GetRowsPerBeat();
}


// Get the active pattern's rows per measure, or, if they are not overriden, the song's default rows per measure.
ROWINDEX CViewPattern::GetRowsPerMeasure() const
//----------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
		return 0;
	if(!pSndFile->Patterns[m_nPattern].GetOverrideSignature())
		return pSndFile->m_nDefaultRowsPerMeasure;
	else
		return pSndFile->Patterns[m_nPattern].GetRowsPerMeasure();
}


// Set instrument 
void CViewPattern::SetSelectionInstrument(const INSTRUMENTINDEX nIns)
//-------------------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	bool modified = false;

	BeginWaitCursor();
	PrepareUndo(m_Selection, "Set Instrument");

	//rewbs: re-written to work regardless of selection
	ROWINDEX startRow  = m_Selection.GetStartRow();
	ROWINDEX endRow    = m_Selection.GetEndRow();
	CHANNELINDEX startChan = m_Selection.GetStartChannel();
	CHANNELINDEX endChan   = m_Selection.GetEndChannel();

	for(ROWINDEX r = startRow; r <= endRow; r++)
	{
		ModCommand *p = pSndFile->Patterns[m_nPattern].GetpModCommand(r, startChan);
		for(CHANNELINDEX c = startChan; c <= endChan; c++, p++)
		{
			// If a note or an instr is present on the row, do the change, if required.
			// Do not set instr if note and instr are both blank,
			// but set instr if note is a PC note and instr is blank.
			if((p->IsNote() || p->IsPcNote() || p->instr) && (p->instr != nIns))
			{
				p->instr = static_cast<ModCommand::INSTR>(nIns);
				modified = true;
			}
		}
	}

	if(modified)
	{
		SetModified();
		InvalidatePattern();
	}
	EndWaitCursor();
}


// Select a whole beat (selectBeat = true) or measure.
void CViewPattern::SelectBeatOrMeasure(bool selectBeat)
//-----------------------------------------------------
{
	const ROWINDEX adjust = selectBeat ? GetRowsPerBeat() : GetRowsPerMeasure();

	// Snap to start of beat / measure of upper-left corner of current selection
	const ROWINDEX startRow = m_Selection.GetStartRow() - (m_Selection.GetStartRow() % adjust);
	// Snap to end of beat / measure of lower-right corner of current selection
	const ROWINDEX endRow = m_Selection.GetEndRow() + adjust - (m_Selection.GetEndRow() % adjust) - 1;

	CHANNELINDEX startChannel = m_Selection.GetStartChannel(), endChannel = m_Selection.GetEndChannel();
	PatternCursor::Columns startColumn = PatternCursor::firstColumn, endColumn = PatternCursor::firstColumn;
	
	if(m_Selection.GetUpperLeft() == m_Selection.GetLowerRight())
	{
		// No selection has been made yet => expand selection to whole channel.
		endColumn = PatternCursor::lastColumn;	// Extend to param column
	} else if(startRow == m_Selection.GetStartRow() && endRow == m_Selection.GetEndRow())
	{
		// Whole beat or measure is already selected
		if(m_Selection.GetStartColumn() == PatternCursor::firstColumn && m_Selection.GetEndColumn() == PatternCursor::lastColumn)
		{
			// Whole channel is already selected => expand selection to whole row.
			startChannel = 0;
			startColumn = PatternCursor::firstColumn;
			endChannel = MAX_BASECHANNELS;
			endColumn = PatternCursor::lastColumn;
		} else
		{
			// Channel is only partly selected => expand to whole channel first.
			endColumn = PatternCursor::lastColumn;	// Extend to param column
		}
	} else
	{
		// Some arbitrary selection: Remember start / end column
		startColumn = m_Selection.GetStartColumn();
		endColumn = m_Selection.GetEndColumn();
	}

	SetCurSel(PatternCursor(startRow, startChannel, startColumn), PatternCursor(endRow, endChannel, endColumn));
}


// Sweep pattern channel to find instrument number to use
void CViewPattern::FindInstrument()
//---------------------------------
{
	CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr)
	{
		return;
	}
	ORDERINDEX ord = GetCurrentOrder();
	PATTERNINDEX pat = m_nPattern;
	ROWINDEX row = m_Cursor.GetRow();

	while(sndFile->Patterns.IsValidPat(pat) && sndFile->Patterns[pat].GetNumRows() != 0)
	{
		// Seek upwards
		do
		{
			ModCommand &m = *sndFile->Patterns[pat].GetpModCommand(row, m_Cursor.GetChannel());
			if(!m.IsPcNote() && m.instr != 0)
			{
				SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, m.instr);
				static_cast<CModControlView *>(CWnd::FromHandle(m_hWndCtrl))->InstrumentChanged(m.instr);
				return;
			}
		} while(row-- != 0);

		// Try previous pattern
		if(ord == 0)
		{
			return;
		}
		ord = sndFile->Order.GetPreviousOrderIgnoringSkips(ord);
		pat = sndFile->Order[ord];
		if(!sndFile->Patterns.IsValidPat(pat))
		{
			return;
		}
		row = sndFile->Patterns[pat].GetNumRows() - 1;
	}
}


// Copy to clipboard
bool CViewPattern::CopyPattern(PATTERNINDEX nPattern, const PatternRect &selection)
//---------------------------------------------------------------------------------
{
	BeginWaitCursor();
	bool result = PatternClipboard::Copy(*GetSoundFile(), nPattern, selection);
	EndWaitCursor();
	PatternClipboardDialog::UpdateList();
	return result;
}


// Paste from clipboard
bool CViewPattern::PastePattern(PATTERNINDEX nPattern, const PatternCursor &pastePos, PatternClipboard::PasteModes mode)
//----------------------------------------------------------------------------------------------------------------------
{
	BeginWaitCursor();
	ModCommandPos pos;
	pos.pattern = nPattern;
	pos.row = pastePos.GetRow();
	pos.channel = pastePos.GetChannel();
	ORDERINDEX curOrder = GetCurrentOrder();
	PatternRect rect;
	bool result = PatternClipboard::Paste(*GetSoundFile(), pos, mode, curOrder, rect);
	EndWaitCursor();

	PatternHint updateHint = PatternHint(PATTERNINDEX_INVALID).Data();
	if(pos.pattern != nPattern)
	{
		// Multipaste: Switch to pasted pattern.
		SetCurrentPattern(pos.pattern);
		curOrder = GetSoundFile()->Order.FindOrder(pos.pattern, curOrder);
		SetCurrentOrder(curOrder);

		updateHint.Names();
		GetDocument()->UpdateAllViews(NULL, SequenceHint(GetSoundFile()->Order.GetCurrentSequenceIndex()).Data(), nullptr);
	}

	if(result)
	{
		SetCurSel(rect);
		GetDocument()->SetModified();
		GetDocument()->UpdateAllViews(NULL, updateHint, nullptr);
	}

	return result;
}


OPENMPT_NAMESPACE_END
