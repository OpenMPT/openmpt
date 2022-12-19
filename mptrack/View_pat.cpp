/*
 * View_pat.cpp
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
#include "SampleEditorDialogs.h"  // For amplification dialog (which is re-used from sample editor)
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
int32 CViewPattern::m_nTransposeAmount = 1;

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
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	&CViewPattern::OnCustomKeyMsg)
	ON_MESSAGE(WM_MOD_MIDIMSG,		&CViewPattern::OnMidiMsg)
	ON_MESSAGE(WM_MOD_RECORDPARAM,	&CViewPattern::OnRecordPlugParamChange)
	ON_COMMAND(ID_EDIT_CUT,			&CViewPattern::OnEditCut)
	ON_COMMAND(ID_EDIT_COPY,		&CViewPattern::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,		&CViewPattern::OnEditPaste)
	ON_COMMAND(ID_EDIT_MIXPASTE,	&CViewPattern::OnEditMixPaste)
	ON_COMMAND(ID_EDIT_MIXPASTE_ITSTYLE,&CViewPattern::OnEditMixPasteITStyle)
	ON_COMMAND(ID_EDIT_PASTEFLOOD,	&CViewPattern::OnEditPasteFlood)
	ON_COMMAND(ID_EDIT_PUSHFORWARDPASTE,&CViewPattern::OnEditPushForwardPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL,	&CViewPattern::OnEditSelectAll)
	ON_COMMAND(ID_EDIT_SELECTCOLUMN,&CViewPattern::OnEditSelectChannel)
	ON_COMMAND(ID_EDIT_SELECTCOLUMN2,&CViewPattern::OnSelectCurrentChannel)
	ON_COMMAND(ID_EDIT_FIND,		&CViewPattern::OnEditFind)
	ON_COMMAND(ID_EDIT_GOTO_MENU,	&CViewPattern::OnEditGoto)
	ON_COMMAND(ID_EDIT_FINDNEXT,	&CViewPattern::OnEditFindNext)
	ON_COMMAND(ID_EDIT_RECSELECT,	&CViewPattern::OnRecordSelect)
	ON_COMMAND(ID_EDIT_SPLITRECSELECT,	&CViewPattern::OnSplitRecordSelect)
	ON_COMMAND(ID_EDIT_SPLITKEYBOARDSETTINGS,	&CViewPattern::SetSplitKeyboardSettings)
	ON_COMMAND(ID_EDIT_UNDO,		&CViewPattern::OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO,		&CViewPattern::OnEditRedo)
	ON_COMMAND(ID_PATTERN_CHNRESET,	&CViewPattern::OnChannelReset)
	ON_COMMAND(ID_PATTERN_MUTE,		&CViewPattern::OnMuteFromClick)
	ON_COMMAND(ID_PATTERN_SOLO,		&CViewPattern::OnSoloFromClick)
	ON_COMMAND(ID_PATTERN_TRANSITIONMUTE, &CViewPattern::OnTogglePendingMuteFromClick)
	ON_COMMAND(ID_PATTERN_TRANSITIONSOLO, &CViewPattern::OnPendingSoloChnFromClick)
	ON_COMMAND(ID_PATTERN_TRANSITION_UNMUTEALL, &CViewPattern::OnPendingUnmuteAllChnFromClick)
	ON_COMMAND(ID_PATTERN_UNMUTEALL,&CViewPattern::OnUnmuteAll)
	ON_COMMAND(ID_PATTERN_SPLIT,	&CViewPattern::OnSplitPattern)
	ON_COMMAND(ID_NEXTINSTRUMENT,	&CViewPattern::OnNextInstrument)
	ON_COMMAND(ID_PREVINSTRUMENT,	&CViewPattern::OnPrevInstrument)
	ON_COMMAND(ID_PATTERN_PLAYROW,	&CViewPattern::OnPatternStep)
	ON_COMMAND(IDC_PATTERN_RECORD,	&CViewPattern::OnPatternRecord)
	ON_COMMAND(ID_PATTERN_DELETEROW,			&CViewPattern::OnDeleteRow)
	ON_COMMAND(ID_PATTERN_DELETEALLROW,			&CViewPattern::OnDeleteWholeRow)
	ON_COMMAND(ID_PATTERN_DELETEROWGLOBAL,		&CViewPattern::OnDeleteRowGlobal)
	ON_COMMAND(ID_PATTERN_DELETEALLROWGLOBAL,	&CViewPattern::OnDeleteWholeRowGlobal)
	ON_COMMAND(ID_PATTERN_INSERTROW,			&CViewPattern::OnInsertRow)
	ON_COMMAND(ID_PATTERN_INSERTALLROW,			&CViewPattern::OnInsertWholeRow)
	ON_COMMAND(ID_PATTERN_INSERTROWGLOBAL,		&CViewPattern::OnInsertRowGlobal)
	ON_COMMAND(ID_PATTERN_INSERTALLROWGLOBAL,	&CViewPattern::OnInsertWholeRowGlobal)
	ON_COMMAND(ID_RUN_SCRIPT,					&CViewPattern::OnRunScript)
	ON_COMMAND(ID_TRANSPOSE_UP,					&CViewPattern::OnTransposeUp)
	ON_COMMAND(ID_TRANSPOSE_DOWN,				&CViewPattern::OnTransposeDown)
	ON_COMMAND(ID_TRANSPOSE_OCTUP,				&CViewPattern::OnTransposeOctUp)
	ON_COMMAND(ID_TRANSPOSE_OCTDOWN,			&CViewPattern::OnTransposeOctDown)
	ON_COMMAND(ID_TRANSPOSE_CUSTOM,				&CViewPattern::OnTransposeCustom)
	ON_COMMAND(ID_PATTERN_PROPERTIES,			&CViewPattern::OnPatternProperties)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_VOLUME,	&CViewPattern::OnInterpolateVolume)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_EFFECT,	&CViewPattern::OnInterpolateEffect)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_NOTE,		&CViewPattern::OnInterpolateNote)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_INSTR,	&CViewPattern::OnInterpolateInstr)
	ON_COMMAND(ID_PATTERN_VISUALIZE_EFFECT,		&CViewPattern::OnVisualizeEffect)
	ON_COMMAND(ID_GROW_SELECTION,				&CViewPattern::OnGrowSelection)
	ON_COMMAND(ID_SHRINK_SELECTION,				&CViewPattern::OnShrinkSelection)
	ON_COMMAND(ID_PATTERN_SETINSTRUMENT,		&CViewPattern::OnSetSelInstrument)
	ON_COMMAND(ID_PATTERN_ADDCHANNEL_FRONT,		&CViewPattern::OnAddChannelFront)
	ON_COMMAND(ID_PATTERN_ADDCHANNEL_AFTER,		&CViewPattern::OnAddChannelAfter)
	ON_COMMAND(ID_PATTERN_RESETCHANNELCOLORS,	&CViewPattern::OnResetChannelColors)
	ON_COMMAND(ID_PATTERN_TRANSPOSECHANNEL,		&CViewPattern::OnTransposeChannel)
	ON_COMMAND(ID_PATTERN_DUPLICATECHANNEL,		&CViewPattern::OnDuplicateChannel)
	ON_COMMAND(ID_PATTERN_REMOVECHANNEL,		&CViewPattern::OnRemoveChannel)
	ON_COMMAND(ID_PATTERN_REMOVECHANNELDIALOG,	&CViewPattern::OnRemoveChannelDialog)
	ON_COMMAND(ID_PATTERN_AMPLIFY,				&CViewPattern::OnPatternAmplify)
	ON_COMMAND(ID_CLEAR_SELECTION,				&CViewPattern::OnClearSelectionFromMenu)
	ON_COMMAND(ID_SHOWTIMEATROW,				&CViewPattern::OnShowTimeAtRow)
	ON_COMMAND(ID_PATTERN_EDIT_PCNOTE_PLUGIN,	&CViewPattern::OnTogglePCNotePluginEditor)
	ON_COMMAND(ID_SETQUANTIZE,					&CViewPattern::OnSetQuantize)
	ON_COMMAND(ID_LOCK_PATTERN_ROWS,			&CViewPattern::OnLockPatternRows)
	ON_COMMAND_RANGE(ID_CHANGE_INSTRUMENT, ID_CHANGE_INSTRUMENT+MAX_INSTRUMENTS, &CViewPattern::OnSelectInstrument)
	ON_COMMAND_RANGE(ID_CHANGE_PCNOTE_PARAM, ID_CHANGE_PCNOTE_PARAM + ModCommand::maxColumnValue, &CViewPattern::OnSelectPCNoteParam)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO,			&CViewPattern::OnUpdateUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO,			&CViewPattern::OnUpdateRedo)
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT+MAX_MIXPLUGINS, &CViewPattern::OnSelectPlugin)


	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

static_assert(ModCommand::maxColumnValue <= 999, "Command range for ID_CHANGE_PCNOTE_PARAM is designed for 999");

const CSoundFile *CViewPattern::GetSoundFile() const { return (GetDocument() != nullptr) ? &GetDocument()->GetSoundFile() : nullptr; };
CSoundFile *CViewPattern::GetSoundFile() { return (GetDocument() != nullptr) ? &GetDocument()->GetSoundFile() : nullptr; };

const ModSequence &CViewPattern::Order() const { return GetSoundFile()->Order(); }
ModSequence &CViewPattern::Order() { return GetSoundFile()->Order(); }


CViewPattern::CViewPattern()
{
	EnableActiveAccessibility();

	m_Dib.Init(CMainFrame::bmpNotes);
	UpdateColors();
	m_PCNoteEditMemory = ModCommand::Empty();
	m_octaveKeyMemory.fill(NOTE_NONE);
}


CViewPattern::~CViewPattern()
{
	m_offScreenBitmap.DeleteObject();
	m_offScreenDC.DeleteDC();
}


void CViewPattern::OnInitialUpdate()
{
	CModScrollView::OnInitialUpdate();
	EnableToolTips();
	ChnVUMeters.fill(0);
	OldVUMeters.fill(0);
	m_previousNote.fill(NOTE_NONE);
	m_splitActiveNoteChannel.fill(NOTE_CHANNEL_MAP_INVALID);
	m_activeNoteChannel.fill(NOTE_CHANNEL_MAP_INVALID);
	m_nPlayPat = PATTERNINDEX_INVALID;
	m_nPlayRow = m_nNextPlayRow = 0;
	m_nPlayTick = 0;
	m_nTicksOnRow = 1;
	m_nMidRow = 0;
	m_nDragItem = {};
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
	m_fallbackInstrument = 0;
	m_nLastPlayedRow = 0;
	m_nLastPlayedOrder = ORDERINDEX_INVALID;
	m_prevChordNote = NOTE_NONE;
	m_previousPCevent.fill({PLUGINDEX_INVALID, 0});
}


bool CViewPattern::SetCurrentPattern(PATTERNINDEX pat, ROWINDEX row)
{
	const CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr)
		return false;
	if(pat == pSndFile->Order.GetIgnoreIndex() || pat == pSndFile->Order.GetInvalidPatIndex())
		return false;
	if(m_pEditWnd && m_pEditWnd->IsWindowVisible())
		m_pEditWnd->ShowWindow(SW_HIDE);

	m_nPattern = pat;
	bool updateScroll = false;

	if(pSndFile->Patterns.IsValidPat(pat))
	{
		if(row != ROWINDEX_INVALID && row != GetCurrentRow() && row < pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			m_Cursor.SetRow(row);
			updateScroll = true;
		}
		if(GetCurrentRow() >= pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			m_Cursor.SetRow(0);
			updateScroll = true;
		}
	}

	SetSelToCursor();

	UpdateSizes();
	UpdateScrollSize();
	UpdateIndicator();

	if(m_bWholePatternFitsOnScreen)
		SetScrollPos(SB_VERT, 0);
	else if(updateScroll)
		SetScrollPos(SB_VERT, (int)GetCurrentRow() * GetRowHeight());

	UpdateScrollPos();
	InvalidatePattern(true, true);
	SendCtrlMessage(CTRLMSG_PATTERNCHANGED, m_nPattern);

	return true;
}


// This should be used instead of consecutive calls to SetCurrentRow() then SetCurrentColumn().
bool CViewPattern::SetCursorPosition(const PatternCursor &cursor, bool wrap)
{
	// Set row, but do not update scroll position yet
	// as there is another position update on the way:
	SetCurrentRow(cursor.GetRow(), wrap, false);
	// Now set column and update scroll position:
	SetCurrentColumn(cursor);
	return true;
}


ROWINDEX CViewPattern::SetCurrentRow(ROWINDEX row, bool wrap, bool updateHorizontalScrollbar)
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidIndex(m_nPattern))
		return ROWINDEX_INVALID;

	if(wrap && pSndFile->Patterns[m_nPattern].GetNumRows())
	{
		const auto &order = Order();
		if((int)row < 0)
		{
			if(m_Status[psKeyboardDragSelect | psMouseDragSelect])
			{
				row = 0;
			} else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL)
			{
				ORDERINDEX curOrder = GetCurrentOrder();
				const ORDERINDEX prevOrd = order.GetPreviousOrderIgnoringSkips(curOrder);
				if(prevOrd < curOrder && m_nPattern == order[curOrder])
				{
					const PATTERNINDEX nPrevPat = order[prevOrd];
					if((nPrevPat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[nPrevPat].GetNumRows()))
					{
						SetCurrentOrder(prevOrd);
						if(SetCurrentPattern(nPrevPat))
							return SetCurrentRow(pSndFile->Patterns[nPrevPat].GetNumRows() + (int)row);
					}
				}
				row = 0;
			} else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_WRAP)
			{
				if((int)row < (int)0)
					row += pSndFile->Patterns[m_nPattern].GetNumRows();
				row %= pSndFile->Patterns[m_nPattern].GetNumRows();
			}
		} else  //row >= 0
		    if(row >= pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			if(m_Status[psKeyboardDragSelect | psMouseDragSelect])
			{
				row = pSndFile->Patterns[m_nPattern].GetNumRows() - 1;
			} else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL)
			{
				ORDERINDEX curOrder = GetCurrentOrder();
				ORDERINDEX nextOrder = order.GetNextOrderIgnoringSkips(curOrder);
				if(nextOrder > curOrder && m_nPattern == order[curOrder])
				{
					PATTERNINDEX nextPat = order[nextOrder];
					if((nextPat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[nextPat].GetNumRows()))
					{
						const ROWINDEX newRow = row - pSndFile->Patterns[m_nPattern].GetNumRows();
						SetCurrentOrder(nextOrder);
						if(SetCurrentPattern(nextPat))
							return SetCurrentRow(newRow);
					}
				}
				row = pSndFile->Patterns[m_nPattern].GetNumRows() - 1;
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

	if((row >= pSndFile->Patterns[m_nPattern].GetNumRows()) || (!m_szCell.cy))
		return false;
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

	return row;
}


bool CViewPattern::SetCurrentColumn(CHANNELINDEX channel, PatternCursor::Columns column)
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
	return true;
}


// Set document as modified and optionally update all pattern views.
void CViewPattern::SetModified(bool updateAllViews)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc != nullptr)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(this, PatternHint(m_nPattern).Data(), updateAllViews ? nullptr : this);
	}
	CMainFrame::GetMainFrame()->NotifyAccessibilityUpdate(*this);
}


// Fix: If cursor isn't on screen move scrollbars to make it visible
// Fix: save pattern scrollbar position when switching to other tab
// Assume that m_nRow and m_dwCursor are valid
// When we switching to other tab the CViewPattern object is deleted
// and when switching back new one is created
bool CViewPattern::UpdateScrollbarPositions(bool updateHorizontalScrollbar)
{
	// HACK - after new CViewPattern object created SetCurrentRow() and SetCurrentColumn() are called -
	// just skip first two calls of UpdateScrollbarPositions() if pModDoc->GetOldPatternScrollbarsPos() is valid
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
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
	CSize scroll(0, 0);
	UINT row = GetCurrentRow();
	UINT yofs = GetYScrollPos();
	CRect rect;
	GetClientRect(&rect);
	rect.top += m_szHeader.cy;
	int numrows = (rect.bottom - rect.top - 1) / m_szCell.cy;
	if(numrows < 1)
		numrows = 1;
	if(m_nMidRow)
	{
		if(row != yofs)
		{
			scroll.cy = (int)(row - yofs) * m_szCell.cy;
		}
	} else
	{
		if(row < yofs)
		{
			scroll.cy = (int)(row - yofs) * m_szCell.cy;
		} else if(row > yofs + (UINT)numrows - 1)
		{
			scroll.cy = (int)(row - yofs - numrows + 1) * m_szCell.cy;
		}
	}

	if(updateHorizontalScrollbar)
	{
		UINT xofs = GetXScrollPos();
		const CHANNELINDEX nchn = GetCurrentChannel();
		if(nchn < xofs)
		{
			scroll.cx = (int)(xofs - nchn) * m_szCell.cx;
			scroll.cx *= -1;
		} else if(nchn > xofs)
		{
			int maxcol = (rect.right - m_szHeader.cx) / m_szCell.cx;
			if((nchn >= (xofs + maxcol)) && (maxcol >= 0))
			{
				scroll.cx = (int)(nchn - xofs - maxcol + 1) * m_szCell.cx;
			}
		}
	}
	if(scroll.cx != 0 || scroll.cy != 0)
	{
		OnScrollBy(scroll, TRUE);
	}
	return true;
}

DragItem CViewPattern::GetDragItem(CPoint point, RECT &outRect) const
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
		return {};

	CRect rcClient, rect, plugRect;

	GetClientRect(&rcClient);
	rect.SetRect(m_szHeader.cx, 0, m_szHeader.cx + GetChannelWidth(), m_szHeader.cy);
	plugRect.SetRect(m_szHeader.cx, m_szHeader.cy - m_szPluginHeader.cy, m_szHeader.cx + GetChannelWidth(), m_szHeader.cy);

	const auto xOffset = static_cast<CHANNELINDEX>(GetXScrollPos());
	const CHANNELINDEX numChannels = pSndFile->GetNumChannels();

	// Checking channel headers
	if(m_Status[psShowPluginNames])
	{
		for(CHANNELINDEX n = xOffset; n < numChannels; n++)
		{
			if(plugRect.PtInRect(point))
			{
				outRect = plugRect;
				return {DragItem::PluginName, n};
			}
			plugRect.OffsetRect(GetChannelWidth(), 0);
		}
	}
	for(CHANNELINDEX n = xOffset; n < numChannels; n++)
	{
		if(rect.PtInRect(point))
		{
			outRect = rect;
			return {DragItem::ChannelHeader, n};
		}
		rect.OffsetRect(GetChannelWidth(), 0);
	}
	if(pSndFile->Patterns.IsValidPat(m_nPattern) && (pSndFile->GetType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)))
	{
		// Clicking on upper-left corner with pattern number (for pattern properties)
		rect.SetRect(0, 0, m_szHeader.cx, m_szHeader.cy);
		if(rect.PtInRect(point))
		{
			outRect = rect;
			return {DragItem::PatternHeader, 0};
		}
	}
	return {};
}


// Drag a selection to position "cursor".
// If scrollHorizontal is true, the point's channel is ensured to be visible.
// Likewise, if scrollVertical is true, the point's row is ensured to be visible.
// If noMode if specified, the original selection points are not altered.
bool CViewPattern::DragToSel(const PatternCursor &cursor, bool scrollHorizontal, bool scrollVertical, bool noMove)
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
		return false;

	CRect rect;
	int yofs = GetYScrollPos(), xofs = GetXScrollPos();
	int row, col;

	if(!m_szCell.cy)
		return false;
	GetClientRect(&rect);
	if(!noMove)
		SetCurSel(m_StartSel, cursor);
	if(!scrollHorizontal && !scrollVertical)
		return true;

	// Scroll to row
	row = cursor.GetRow();
	if(scrollVertical && row < (int)pSndFile->Patterns[m_nPattern].GetNumRows())
	{
		row += m_nMidRow;
		rect.top += m_szHeader.cy;
		int numrows = (rect.bottom - rect.top - 1) / m_szCell.cy;
		if(numrows < 1)
			numrows = 1;
		if(row < yofs)
		{
			CSize sz;
			sz.cx = 0;
			sz.cy = (int)(row - yofs) * m_szCell.cy;
			OnScrollBy(sz, TRUE);
		} else if(row > yofs + numrows - 1)
		{
			CSize sz;
			sz.cx = 0;
			sz.cy = (int)(row - yofs - numrows + 1) * m_szCell.cy;
			OnScrollBy(sz, TRUE);
		}
	}

	// Scroll to column
	col = cursor.GetChannel();
	if(scrollHorizontal && col < (int)pSndFile->GetNumChannels())
	{
		int maxcol = (rect.right - m_szHeader.cx) - 4;
		maxcol -= GetColumnOffset(cursor.GetColumnType());
		maxcol /= GetChannelWidth();
		if(col < xofs)
		{
			CSize size(-m_szCell.cx, 0);
			if(!noMove)
				size.cx = (col - xofs) * (int)m_szCell.cx;
			OnScrollBy(size, TRUE);
		} else if((col > xofs + maxcol) && (maxcol > 0))
		{
			CSize size(m_szCell.cx, 0);
			if(!noMove)
				size.cx = (col - maxcol + 1) * (int)m_szCell.cx;
			OnScrollBy(size, TRUE);
		}
	}
	UpdateWindow();
	return true;
}


bool CViewPattern::SetPlayCursor(PATTERNINDEX pat, ROWINDEX row, uint32 tick)
{
	PATTERNINDEX oldPat = m_nPlayPat;
	ROWINDEX oldRow = m_nPlayRow;
	uint32 oldTick = m_nPlayTick;

	m_nPlayPat = pat;
	m_nPlayRow = row;
	m_nPlayTick = tick;

	if(m_nPlayTick != oldTick && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SMOOTHSCROLL))
		InvalidatePattern(true, true);
	else if(oldPat == m_nPattern)
		InvalidateRow(oldRow);
	else if(m_nPlayPat == m_nPattern)
		InvalidateRow(m_nPlayRow);

	return true;
}


UINT CViewPattern::GetCurrentInstrument() const
{
	return static_cast<UINT>(SendCtrlMessage(CTRLMSG_GETCURRENTINSTRUMENT));
}


bool CViewPattern::ShowEditWindow()
{
	if(!m_pEditWnd)
	{
		m_pEditWnd = new CEditCommand(*GetSoundFile());
	}
	if(m_pEditWnd)
	{
		m_pEditWnd->ShowEditWindow(m_nPattern, m_Cursor, this);
		return true;
	}
	return false;
}


bool CViewPattern::PrepareUndo(const PatternCursor &beginSel, const PatternCursor &endSel, const char *description)
{
	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX chnBegin = beginSel.GetChannel(), chnEnd = endSel.GetChannel();
	const ROWINDEX rowBegin = beginSel.GetRow(), rowEnd = endSel.GetRow();

	if((chnEnd < chnBegin) || (rowEnd < rowBegin) || pModDoc == nullptr)
		return false;
	return pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, chnBegin, rowBegin, chnEnd - chnBegin + 1, rowEnd - rowBegin + 1, description);
}


BOOL CViewPattern::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
		   (pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = CMainFrame::GetInputHandler();

			InputTargetContext ctx = (InputTargetContext)(kCtxViewPatterns + 1 + m_Cursor.GetColumnType());
			// If editing is disabled, preview notes no matter which column we are in
			if(!IsEditingEnabled() && TrackerSettings::Instance().patternNoEditPopup)
				ctx = kCtxViewPatternsNote;

			const auto event = ih->Translate(*pMsg);
			if(ih->KeyEvent(ctx, event) != kcNull)
			{
				return true;  // Mapped to a command, no need to pass message on.
			}
			//HACK: fold kCtxViewPatternsFX and kCtxViewPatternsFXparam so that all commands of 1 are active in the other
			else
			{
				if(ctx == kCtxViewPatternsFX)
				{
					if(ih->KeyEvent(kCtxViewPatternsFXparam, event) != kcNull)
						return true;  // Mapped to a command, no need to pass message on.
				} else if(ctx == kCtxViewPatternsFXparam)
				{
					if(ih->KeyEvent(kCtxViewPatternsFX, event) != kcNull)
						return true;  // Mapped to a command, no need to pass message on.
				} else if(ctx == kCtxViewPatternsIns)
				{
					// Do the same with instrument->note column
					if(ih->KeyEvent(kCtxViewPatternsNote, event) != kcNull)
						return true;  // Mapped to a command, no need to pass message on.
				}
			}
			//end HACK.

			// Handle Application (menu) key
			if(pMsg->message == WM_KEYDOWN && event.key == VK_APPS)
			{
				OnRButtonDown(0, GetPointFromPosition(m_Cursor));
			}
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
					m_quickChannelProperties.Show(GetDocument(), cursor.GetChannel(), point);
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
{
	// Fix: save pattern scrollbar position when switching to other tab
	// When we switching to other tab the CViewPattern object is deleted
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
	{
		pModDoc->SetOldPatternScrollbarsPos(CSize(m_nXScroll * m_szCell.cx, m_nYScroll * m_szCell.cy));
	}
	if(m_pEffectVis)
	{
		m_pEffectVis->DoClose();
		m_pEffectVis = nullptr;
	}

	if(m_pEditWnd)
	{
		m_pEditWnd->DestroyWindow();
		delete m_pEditWnd;
		m_pEditWnd = NULL;
	}

	CModScrollView::OnDestroy();
}


void CViewPattern::OnSetFocus(CWnd *pOldWnd)
{
	CScrollView::OnSetFocus(pOldWnd);
	m_Status.set(psFocussed);
	InvalidateRow();
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
	{
		pModDoc->SetNotifications(Notification::Position | Notification::VUMeters);
		pModDoc->SetFollowWnd(m_hWnd);
		UpdateIndicator();
	}
}


void CViewPattern::OnKillFocus(CWnd *pNewWnd)
{
	CScrollView::OnKillFocus(pNewWnd);

	m_Status.reset(psKeyboardDragSelect | psCtrlDragSelect | psFocussed);
	InvalidateRow();
}


void CViewPattern::OnGrowSelection()
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

				ModCommand *dest  = pSndFile->Patterns[m_nPattern].GetpModCommand(row, chn);
				ModCommand *src   = pSndFile->Patterns[m_nPattern].GetpModCommand(row - offset / 2, chn);
				ModCommand *blank = pSndFile->Patterns[m_nPattern].GetpModCommand(row - 1, chn);  // Row "in between"

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
	m_Selection = PatternRect(startSel, PatternCursor(std::min(finalDest, static_cast<ROWINDEX>(pSndFile->Patterns[m_nPattern].GetNumRows() - 1)), endSel));

	InvalidatePattern();
	SetModified();
	EndWaitCursor();
	SetFocus();
}


void CViewPattern::OnShrinkSelection()
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
			ModCommand *dest = pSndFile->Patterns[m_nPattern].GetpModCommand(row, chn);
			ModCommand src;

			if(row <= finalDest)
			{
				// Normal shrink operation
				src = *pSndFile->Patterns[m_nPattern].GetpModCommand(srcRow, chn);

				// If source command is empty, try next source row (so we don't lose all the stuff that's on odd rows).
				if(srcRow < pSndFile->Patterns[m_nPattern].GetNumRows() - 1)
				{
					const ModCommand &srcNext = *pSndFile->Patterns[m_nPattern].GetpModCommand(srcRow + 1, chn);
					if(src.note == NOTE_NONE)
						src.note = srcNext.note;
					if(src.instr == 0)
						src.instr = srcNext.instr;
					if(src.volcmd == VOLCMD_NONE)
					{
						src.volcmd = srcNext.volcmd;
						src.vol = srcNext.vol;
					}
					if(src.command == CMD_NONE)
					{
						src.command = srcNext.command;
						src.param = srcNext.param;
					}
				}
			} else
			{
				// Clean up rows that are now supposed to be empty.
				src = ModCommand::Empty();
			}

			for(int i = PatternCursor::firstColumn; i <= PatternCursor::lastColumn; i++)
			{
				PatternCursor cell(row, chn, static_cast<PatternCursor::Columns>(i));
				if(!m_Selection.ContainsHorizontal(cell))
				{
					// We might have to skip the first / last few entries.
					continue;
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
	m_Selection = PatternRect(startSel, PatternCursor(std::min(finalDest, static_cast<ROWINDEX>(pSndFile->Patterns[m_nPattern].GetNumRows() - 1)), endSel));

	InvalidatePattern();
	SetModified();
	EndWaitCursor();
	SetFocus();
}


void CViewPattern::OnClearSelectionFromMenu()
{
	OnClearSelection();
}

void CViewPattern::OnClearSelection(bool ITStyle, RowMask rm)  //Default RowMask: all elements enabled
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

	ApplyToSelection([&] (ModCommand &m, ROWINDEX row, CHANNELINDEX chn)
	{
		for(int i = PatternCursor::firstColumn; i <= PatternCursor::lastColumn; i++)
		{
			PatternCursor cell(row, chn, static_cast<PatternCursor::Columns>(i));
			if(!m_Selection.ContainsHorizontal(cell))
			{
				// We might have to skip the first / last few entries.
				continue;
			}

			switch(i)
			{
			case PatternCursor::noteColumn:  // Clear note
				if(rm.note)
				{
					if(m.IsPcNote())
					{  // Clear whole cell if clearing PC note
						m.Clear();
					} else
					{
						m.note = NOTE_NONE;
						if(ITStyle)
							m.instr = 0;
					}
				}
				break;

			case PatternCursor::instrColumn:  // Clear instrument
				if(rm.instrument)
				{
					m.instr = 0;
				}
				break;

			case PatternCursor::volumeColumn:  // Clear volume
				if(rm.volume)
				{
					m.volcmd = VOLCMD_NONE;
					m.vol = 0;
				}
				break;

			case PatternCursor::effectColumn:  // Clear Command
				if(rm.command)
				{
					m.command = CMD_NONE;
					if(m.IsPcNote())
					{
						m.SetValueEffectCol(0);
					}
				}
				break;

			case PatternCursor::paramColumn:  // Clear Command Param
				if(rm.parameter)
				{
					m.param = 0;
					if(m.IsPcNote())
					{
						m.SetValueEffectCol(0);

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
	});

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
{
	OnEditCopy();
	OnClearSelection(false);
}


void CViewPattern::OnEditCopy()
{
	CModDoc *pModDoc = GetDocument();

	if(pModDoc)
	{
		CopyPattern(m_nPattern, m_Selection);
		SetFocus();
	}
}


void CViewPattern::StartRecordGroupDragging(const DragItem source)
{
	// Drag-select record channels
	const auto *modDoc = GetDocument();
	if(modDoc == nullptr)
		return;

	m_initialDragRecordStatus.resize(modDoc->GetNumChannels());
	for(CHANNELINDEX chn = 0; chn < modDoc->GetNumChannels(); chn++)
	{
		m_initialDragRecordStatus[chn] = modDoc->GetChannelRecordGroup(chn);
	}
	m_Status.reset(psDragging);
	m_nDropItem = m_nDragItem = source;
}


void CViewPattern::OnLButtonDown(UINT nFlags, CPoint point)
{
	const auto *modDoc = GetDocument();
	if(modDoc == nullptr)
		return;
	const auto &sndFile = modDoc->GetSoundFile();

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
		if(nFlags & MK_CONTROL)
			TogglePendingMute(pointCursor.GetChannel());
		if(nFlags & MK_SHIFT)
		{
			// Drag-select record channels
			StartRecordGroupDragging(m_nDragItem);
		}
	} else if(point.x >= m_szHeader.cx && point.y > m_szHeader.cy)
	{
		// Click on pattern data
		if(IsLiveRecord() && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_NOFOLLOWONCLICK))
		{
			SendCtrlMessage(CTRLMSG_PAT_FOLLOWSONG, 0);
		}

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
			if(m_StartSel.GetChannel() < sndFile.GetNumChannels())
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
				} else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CENTERROW)
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
		if(pointCursor.GetRow() < sndFile.Patterns[m_nPattern].GetNumRows())
		{
			m_StartSel.Set(pointCursor.GetRow(), 0);
			SetCurSel(m_StartSel, PatternCursor(pointCursor.GetRow(), sndFile.GetNumChannels() - 1, PatternCursor::lastColumn));
			m_Status.set(psRowSelection);
		}
	}

	if(m_nDragItem.IsValid())
	{
		InvalidateRect(&m_rcDragItem, FALSE);
		UpdateWindow();
	}
}


void CViewPattern::OnLButtonDblClk(UINT uFlags, CPoint point)
{
	PatternCursor cursor = GetPositionFromPoint(point);
	if(cursor == m_Cursor && point.y >= m_szHeader.cy)
	{
		// Double-click pattern cell: Select whole column or show cell properties.
		if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_DBLCLICKSELECT))
		{
			OnSelectCurrentChannel();
			m_Status.set(psChannelSelection | psDragging);
			return;
		} else
		{
			if(ShowEditWindow())
				return;
		}
	}

	OnLButtonDown(uFlags, point);
}


void CViewPattern::OnLButtonUp(UINT nFlags, CPoint point)
{
	CModDoc *modDoc = GetDocument();
	if(modDoc == nullptr)
		return;

	const auto dragType = m_nDragItem.Type();
	const bool wasDraggingRecordGroup = IsDraggingRecordGroup();
	const bool itemSelected = m_bInItemRect || (dragType == DragItem::ChannelHeader);
	m_bInItemRect = false;
	ResetRecordGroupDragging();
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
			SetCursorPosition(m_StartSel);
		}
		SetCursor(CMainFrame::curArrow);
		m_Status.reset(psDragnDropEdit);
	}
	if(dragType != DragItem::ChannelHeader
	   && dragType != DragItem::PatternHeader
	   && dragType != DragItem::PluginName)
	{
		if((m_nMidRow) && (m_Selection.GetUpperLeft() == m_Selection.GetLowerRight()))
		{
			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition(m_Selection.GetUpperLeft());
			//UpdateIndicator();
		}
	}
	if(!itemSelected || !m_nDragItem.IsValid())
		return;
	InvalidateRect(&m_rcDragItem, FALSE);
	const CHANNELINDEX sourceChn = static_cast<CHANNELINDEX>(m_nDragItem.Value());
	const CHANNELINDEX targetChn = m_nDropItem.IsValid() ? static_cast<CHANNELINDEX>(m_nDropItem.Value()) : CHANNELINDEX_INVALID;

	switch(m_nDragItem.Type())
	{
	case DragItem::ChannelHeader:
		if(sourceChn == targetChn && targetChn < modDoc->GetNumChannels())
		{
			// Just clicked a channel header...
			if(nFlags & MK_SHIFT)
			{
				// Toggle record state
				modDoc->ToggleChannelRecordGroup(sourceChn, RecordGroup::Group1);
				InvalidateChannelsHeaders(sourceChn);
			} else if(CMainFrame::GetInputHandler()->AltPressed())
			{
				// Solo / Unsolo
				OnSoloChannel(sourceChn);
			} else if(!(nFlags & MK_CONTROL))
			{
				// Mute / Unmute
				OnMuteChannel(sourceChn);
			}
		} else if(!wasDraggingRecordGroup && targetChn < modDoc->GetNumChannels() && m_nDropItem.Type() == DragItem::ChannelHeader)
		{
			// Dragged to other channel header => move or copy channel

			InvalidateRect(&m_rcDropItem, FALSE);

			const bool duplicate = (nFlags & MK_SHIFT) != 0;
			DragChannel(sourceChn, targetChn, 1, duplicate);
		}
		break;

	case DragItem::PatternHeader:
		OnPatternProperties();
		break;

	case DragItem::PluginName:
		if(sourceChn < MAX_BASECHANNELS)
			TogglePluginEditor(sourceChn);
		break;
	}

	m_nDropItem = {};
}


void CViewPattern::DragChannel(CHANNELINDEX source, CHANNELINDEX target, CHANNELINDEX numChannels, bool duplicate)
{
	auto modDoc = GetDocument();
	const CHANNELINDEX newChannels = modDoc->GetNumChannels() + (duplicate ? numChannels : 0);
	std::vector<CHANNELINDEX> channels(newChannels, 0);
	bool modified = duplicate;

	for(CHANNELINDEX chn = 0, fromChn = 0; chn < newChannels; chn++)
	{
		if(chn >= target && chn < target + numChannels)
		{
			channels[chn] = source + chn - target;
		} else
		{
			if(fromChn == source && !duplicate)  // Don't want the source channels twice if we're just moving
			{
				fromChn += numChannels;
			}
			channels[chn] = fromChn++;
		}
		if(channels[chn] != chn)
		{
			modified = true;
		}
	}
	if(modified && modDoc->ReArrangeChannels(channels) != CHANNELINDEX_INVALID)
	{
		modDoc->UpdateAllViews(this, GeneralHint().Channels().ModType(), this);
		if(duplicate)
		{
			// Number of channels changed: Update channel headers and other information.
			SetCurrentPattern(m_nPattern);
		}

		if(!duplicate)
		{
			const auto oldSel = m_Selection;
			if(auto chn = m_Cursor.GetChannel(); (chn >= source && chn < source + numChannels))
				SetCurrentColumn(target + chn - source, m_Cursor.GetColumnType());
			if(oldSel.GetStartChannel() >= source && oldSel.GetEndChannel() < source + numChannels)
			{
				const auto diff = static_cast<int>(target) - source;
				auto upperLeft = oldSel.GetUpperLeft(), lowerRight = oldSel.GetLowerRight();
				upperLeft.Move(0, diff, 0);
				lowerRight.Move(0, diff, 0);
				SetCurSel(upperLeft, lowerRight);
			}
		}

		InvalidatePattern(true, false);
		SetModified(false);
	}
}


void CViewPattern::ShowPatternProperties(PATTERNINDEX pat)
{
	CModDoc *pModDoc = GetDocument();
	if(pat == PATTERNINDEX_INVALID)
		pat = m_nPattern;
	if(pModDoc && pModDoc->GetSoundFile().Patterns.IsValidPat(pat))
	{
		CPatternPropertiesDlg dlg(*pModDoc, pat, this);
		if(dlg.DoModal() == IDOK)
		{
			UpdateScrollSize();
			InvalidatePattern(true, true);
			SanitizeCursor();
			pModDoc->UpdateAllViews(this, PatternHint(pat).Data(), this);
		}
	}
}


void CViewPattern::OnRButtonDown(UINT flags, CPoint pt)
{
	CModDoc *modDoc = GetDocument();
	HMENU hMenu;

	// Too far left to get a ctx menu:
	if(!modDoc || pt.x < m_szHeader.cx)
	{
		return;
	}

	// Handle drag n drop
	if(m_Status[psDragnDropEdit])
	{
		if(m_Status[psDragnDropping])
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

	if((hMenu = ::CreatePopupMenu()) == NULL)
	{
		return;
	}

	CSoundFile &sndFile = modDoc->GetSoundFile();
	m_MenuCursor = GetPositionFromPoint(pt);

	// Right-click outside single-point selection? Reposition cursor to the new location
	if(!m_Selection.Contains(m_MenuCursor) && m_Selection.GetUpperLeft() == m_Selection.GetLowerRight())
	{
		if(pt.y > m_szHeader.cy)
		{
			//ensure we're not clicking header

			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition(m_MenuCursor);
		}
	}
	const CHANNELINDEX nChn = m_MenuCursor.GetChannel();
	const bool inChannelHeader = (pt.y < m_szHeader.cy);

	if((flags & MK_CONTROL) && nChn < sndFile.GetNumChannels() && inChannelHeader)
	{
		// Ctrl+Right-Click: Open quick channel properties.
		ClientToScreen(&pt);
		m_quickChannelProperties.Show(GetDocument(), nChn, pt);
	} else if((flags & MK_SHIFT) && inChannelHeader)
	{
		// Drag-select record channels
		StartRecordGroupDragging(GetDragItem(pt, m_rcDragItem));
	} else if(nChn < sndFile.GetNumChannels() && sndFile.Patterns.IsValidPat(m_nPattern) && !(flags & (MK_CONTROL | MK_SHIFT)))
	{
		CInputHandler *ih = CMainFrame::GetInputHandler();

		//------ Plugin Header Menu --------- :
		if(m_Status[psShowPluginNames] &&
			inChannelHeader && (pt.y > m_szHeader.cy - m_szPluginHeader.cy))
		{
			BuildPluginCtxMenu(hMenu, nChn, sndFile);
		}

		//------ Channel Header Menu ---------- :
		else if(inChannelHeader)
		{
			if(ih->ShiftPressed())
			{
				//Don't bring up menu if shift is pressed, else we won't get button up msg.
			} else
			{
				if(BuildSoloMuteCtxMenu(hMenu, ih, nChn, sndFile))
					AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
				BuildRecordCtxMenu(hMenu, ih, nChn);
				BuildChannelControlCtxMenu(hMenu, ih);
			}
		}

		//------ Standard Menu ---------- :
		else if((pt.x >= m_szHeader.cx) && (pt.y >= m_szHeader.cy))
		{
			// When combining menus, use bitwise ORs to avoid shortcuts
			if(BuildSelectionCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));
			if(BuildEditCtxMenu(hMenu, ih, modDoc))
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
				s += MPT_CFORMAT("(Currently: {} Row{})")(rows, CString(rows == 1 ? _T("") : _T("s")));
			} else
			{
				s += _T("Settings...");
			}
			AppendMenu(hMenu, MF_STRING | (TrackerSettings::Instance().recordQuantizeRows != 0 ? MF_CHECKED : 0), ID_SETQUANTIZE, ih->GetKeyTextFromCommand(kcQuantizeSettings, s));
		}

		ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
	} else if(nChn >= sndFile.GetNumChannels() && sndFile.GetNumChannels() < sndFile.GetModSpecifications().channelsMax && !(flags & (MK_CONTROL | MK_SHIFT)))
	{
		// Click outside of pattern: Offer easy way to add more channels
		m_MenuCursor.Set(0, sndFile.GetNumChannels() - 1);
		AppendMenu(hMenu, MF_STRING, ID_PATTERN_ADDCHANNEL_AFTER, _T("&Add Channel"));
		ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
	}
	::DestroyMenu(hMenu);
}

void CViewPattern::OnRButtonUp(UINT nFlags, CPoint point)
{
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc)
		return;

	ResetRecordGroupDragging();
	const CHANNELINDEX sourceChn = static_cast<CHANNELINDEX>(m_nDragItem.Value());
	const CHANNELINDEX targetChn = m_nDropItem.IsValid() ? static_cast<CHANNELINDEX>(m_nDropItem.Value()) : CHANNELINDEX_INVALID;
	switch(m_nDragItem.Type())
	{
	case DragItem::ChannelHeader:
		if(nFlags & MK_SHIFT)
		{
			if(sourceChn < MAX_BASECHANNELS && sourceChn == targetChn)
			{
				pModDoc->ToggleChannelRecordGroup(sourceChn, RecordGroup::Group2);
				InvalidateChannelsHeaders(sourceChn);
			}
		}
		break;
	}

	CModScrollView::OnRButtonUp(nFlags, point);
}


BOOL CViewPattern::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
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
		CursorJump(-mpt::signum(zDelta), false);
		return TRUE;
	}
	return CModScrollView::OnMouseWheel(nFlags, zDelta, pt);
}


void CViewPattern::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
{
	if(nButton == XBUTTON1)
		OnPrevOrder();
	else if(nButton == XBUTTON2)
		OnNextOrder();
	CModScrollView::OnXButtonUp(nFlags, nButton, point);
}


void CViewPattern::OnMouseMove(UINT nFlags, CPoint point)
{
	CModScrollView::OnMouseMove(nFlags, point);

	const bool isDraggingRecordGroup = IsDraggingRecordGroup();
	if(!m_Status[psDragging] && !isDraggingRecordGroup)
		return;

	// Drag&Drop actions
	if(m_nDragItem.IsValid())
	{
		const CRect oldDropRect = m_rcDropItem;
		const auto oldDropItem = m_nDropItem;

		if(isDraggingRecordGroup)
		{
			// When drag-selecting record channels, ignore y position
			point.y = m_rcDragItem.top;
		}

		m_Status.set(psShiftDragging, (nFlags & MK_SHIFT) != 0);
		m_nDropItem = GetDragItem(point, m_rcDropItem);

		const bool b = (m_nDropItem == m_nDragItem);
		const bool dragChannel = m_nDragItem.Type() == DragItem::ChannelHeader;

		if(b != m_bInItemRect || (m_nDropItem != oldDropItem && dragChannel))
		{
			m_bInItemRect = b;
			InvalidateRect(&m_rcDragItem, FALSE);

			// Drag-select record channels
			if(isDraggingRecordGroup && m_nDropItem.Type() == DragItem::ChannelHeader)
			{
				auto modDoc = GetDocument();
				auto startChn = static_cast<CHANNELINDEX>(m_nDragItem.Value());
				auto endChn = static_cast<CHANNELINDEX>(m_nDropItem.Value());

				RecordGroup setRecord = RecordGroup::NoGroup;
				if(m_initialDragRecordStatus[startChn] != RecordGroup::Group1 && (nFlags & MK_LBUTTON))
					setRecord = RecordGroup::Group1;
				else if (m_initialDragRecordStatus[startChn] != RecordGroup::Group2 && (nFlags & MK_RBUTTON))
					setRecord = RecordGroup::Group2;

				if(startChn > endChn)
					std::swap(startChn, endChn);

				CHANNELINDEX numChannels = std::min(modDoc->GetNumChannels(), static_cast<CHANNELINDEX>(m_initialDragRecordStatus.size()));
				for(CHANNELINDEX chn = 0; chn < numChannels; chn++)
				{
					auto oldState = modDoc->GetChannelRecordGroup(chn);
					if(chn >= startChn && chn <= endChn)
						GetDocument()->SetChannelRecordGroup(chn, setRecord);
					else
						GetDocument()->SetChannelRecordGroup(chn, m_initialDragRecordStatus[chn]);
					if(oldState != modDoc->GetChannelRecordGroup(chn))
						InvalidateChannelsHeaders(chn);
				}
			} else
			{
				// Dragging around channel headers? Update move indicator...
				if(m_nDropItem.Type() == DragItem::ChannelHeader)
					InvalidateRect(&m_rcDropItem, FALSE);
				if(oldDropItem.Type() == DragItem::ChannelHeader)
					InvalidateRect(&oldDropRect, FALSE);
			}

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
		if(m_Status[psDragnDropEdit])
		{
			const bool moved = m_DragPos.GetChannel() != cursor.GetChannel() || m_DragPos.GetRow() != cursor.GetRow();

			if(!m_Status[psDragnDropping])
			{
				SetCursor(CMainFrame::curDragging);
			}
			if(!m_Status[psDragnDropping] || moved)
			{
				if(m_Status[psDragnDropping])
					OnDrawDragSel();
				m_Status.reset(psDragnDropping);
				DragToSel(cursor, true, true, true);
				m_DragPos = cursor;
				m_Status.set(psDragnDropping);
				OnDrawDragSel();
			}
		} else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CENTERROW)
		{
			// Default: selection
			DragToSel(cursor, true, true);
		} else
		{
			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition(cursor);
		}
	}
}


void CViewPattern::OnEditSelectAll()
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		SetCurSel(PatternCursor(0), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn));
	}
}


void CViewPattern::OnEditSelectChannel()
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		SetCurSel(PatternCursor(0, m_MenuCursor.GetChannel()), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, m_MenuCursor.GetChannel(), PatternCursor::lastColumn));
	}
}


void CViewPattern::OnSelectCurrentChannel()
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


void CViewPattern::OnSelectCurrentColumn()
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		SetCurSel(PatternCursor(0, m_Cursor), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, m_Cursor));
	}
}


void CViewPattern::OnChannelReset()
{
	ResetChannel(m_MenuCursor.GetChannel());
}


// Reset all channel variables
void CViewPattern::ResetChannel(CHANNELINDEX chn)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	CSoundFile &sndFile = pModDoc->GetSoundFile();

	CriticalSection cs;
	if(!pModDoc->IsChannelMuted(chn))
	{
		// Cut playing notes
		sndFile.ChnSettings[chn].dwFlags.set(CHN_MUTE);
		pModDoc->UpdateChannelMuteStatus(chn);
		sndFile.ChnSettings[chn].dwFlags.reset(CHN_MUTE);
	}
	sndFile.m_PlayState.Chn[chn].Reset(ModChannel::resetTotal, sndFile, chn, CSoundFile::GetChannelMuteFlag());
}


void CViewPattern::OnMuteFromClick()
{
	OnMuteChannel(m_MenuCursor.GetChannel());
}


void CViewPattern::OnMuteChannel(CHANNELINDEX chn)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
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
{
	OnSoloChannel(m_MenuCursor.GetChannel());
}


// When trying to solo a channel that is already the only unmuted channel,
// this will result in unmuting all channels, in order to satisfy user habits.
// In all other cases, soloing a channel unsoloes all and mutes all except this channel
void CViewPattern::OnSoloChannel(CHANNELINDEX chn)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;

	if(chn >= pModDoc->GetNumChannels())
	{
		return;
	}

	if(pModDoc->IsChannelSolo(chn))
	{
		bool nChnIsOnlyUnMutedChan = true;
		for(CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)  //check status of all other chans
		{
			if(i != chn && !pModDoc->IsChannelMuted(i))
			{
				nChnIsOnlyUnMutedChan = false;  //found a channel that isn't muted!
				break;
			}
		}
		if(nChnIsOnlyUnMutedChan)  // this is the only playable channel and it is already soloed ->  Unmute all
		{
			OnUnmuteAll();
			return;
		}
	}
	for(CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)
	{
		pModDoc->MuteChannel(i, !(i == chn));  //mute all chans except nChn, unmute nChn
		pModDoc->SoloChannel(i, (i == chn));   //unsolo all chans except nChn, solo nChn
	}
	InvalidateChannelsHeaders();
	pModDoc->UpdateAllViews(this, GeneralHint(chn).Channels());
}


void CViewPattern::OnRecordSelect()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
	{
		CHANNELINDEX chn = m_MenuCursor.GetChannel();
		if(chn < pModDoc->GetNumChannels())
		{
			pModDoc->ToggleChannelRecordGroup(chn, RecordGroup::Group1);
			InvalidateChannelsHeaders(chn);
		}
	}
}


void CViewPattern::OnSplitRecordSelect()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
	{
		CHANNELINDEX chn = m_MenuCursor.GetChannel();
		if(chn < pModDoc->GetNumChannels())
		{
			pModDoc->ToggleChannelRecordGroup(chn, RecordGroup::Group2);
			InvalidateChannelsHeaders(chn);
		}
	}
}


void CViewPattern::OnUnmuteAll()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc)
	{
		const CHANNELINDEX numChannels = pModDoc->GetNumChannels();
		for(CHANNELINDEX chn = 0; chn < numChannels; chn++)
		{
			pModDoc->MuteChannel(chn, false);
			pModDoc->SoloChannel(chn, false);
		}
		InvalidateChannelsHeaders();
	}
}


bool CViewPattern::InsertOrDeleteRows(CHANNELINDEX firstChn, CHANNELINDEX lastChn, bool globalEdit, bool deleteRows)
{
	CModDoc &modDoc = *GetDocument();
	CSoundFile &sndFile = *GetSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern) || !IsEditingEnabled_bmsg())
		return false;

	LimitMax(lastChn, CHANNELINDEX(sndFile.GetNumChannels() - 1));
	if(firstChn > lastChn)
		return false;

	const auto selection = (firstChn != lastChn || m_Selection.GetNumRows() > 1) ? PatternRect{{m_Selection.GetStartRow(), firstChn, PatternCursor::firstColumn}, {m_Selection.GetEndRow(), lastChn, PatternCursor::lastColumn}} : m_Selection;
	const ROWINDEX numRows = selection.GetNumRows();

	const char *undoDescription = "";
	if(deleteRows)
		undoDescription = numRows != 1 ? "Delete Rows" : "Delete Row";
	else
		undoDescription = numRows != 1 ? "Insert Rows" : "Insert Row";

	const ROWINDEX startRow = selection.GetStartRow();
	const CHANNELINDEX numChannels = lastChn - firstChn + 1;

	std::vector<PATTERNINDEX> patterns;
	if(globalEdit)
	{
		auto &order = Order();
		const auto start = order.begin() + GetCurrentOrder();
		const auto end = std::find(start, order.end(), order.GetInvalidPatIndex());

		// As this is a global operation, ensure that all modified patterns are unique
		bool orderListChanged = false;
		const ORDERINDEX ordEnd = GetCurrentOrder() + static_cast<ORDERINDEX>(std::distance(start, end));
		for(ORDERINDEX ord = GetCurrentOrder(); ord < ordEnd; ord++)
		{
			const auto pat = order[ord];
			if(pat != order.EnsureUnique(ord))
				orderListChanged = true;
		}
		if(orderListChanged)
			modDoc.UpdateAllViews(this, SequenceHint().Data(), nullptr);

		patterns.assign(start, end);
	} else
	{
		patterns = {m_nPattern};
	}

	// Backup source data and create undo points
	std::vector<ModCommand> patternData;
	if(!deleteRows)
		patternData.insert(patternData.begin(), numRows * numChannels, ModCommand{});

	bool first = true;
	for(auto pat : patterns)
	{
		if(!sndFile.Patterns.IsValidPat(pat))
			continue;
		const auto &pattern = sndFile.Patterns[pat];
		const ROWINDEX firstRow = first ? startRow : 0;
		for(ROWINDEX row = firstRow; row < pattern.GetNumRows(); row++)
		{
			const auto *m = pattern.GetpModCommand(row, firstChn);
			patternData.insert(patternData.end(), m, m + numChannels);
		}
		modDoc.GetPatternUndo().PrepareUndo(pat, firstChn, firstRow, numChannels, pattern.GetNumRows(), undoDescription, !first);
		first = false;
	}

	if(deleteRows)
		patternData.insert(patternData.end(), numRows * numChannels, ModCommand{});

	// Now do the actual shifting
	auto src = patternData.cbegin();
	if(deleteRows)
		src += numRows * numChannels;

	PATTERNINDEX firstNewPattern = m_nPattern;
	first = true;
	for(auto pat : patterns)
	{
		if(!sndFile.Patterns.IsValidPat(pat))
			continue;
		auto &pattern = sndFile.Patterns[pat];
		for(ROWINDEX row = first ? startRow : 0; row < pattern.GetNumRows(); row++, src += numChannels)
		{
			ModCommand *dest = pattern.GetpModCommand(row, firstChn);
			std::copy(src, src + numChannels, dest);
		}
		if(first)
			firstNewPattern = pat;
		first = false;
		modDoc.UpdateAllViews(this, PatternHint(pat).Data(), this);
	}

	SetModified();
	SetCurrentPattern(firstNewPattern);
	InvalidatePattern();

	SetCursorPosition(selection.GetUpperLeft());
	SetCurSel(selection);

	return true;
}


void CViewPattern::DeleteRows(CHANNELINDEX firstChn, CHANNELINDEX lastChn, bool globalEdit)
{
	InsertOrDeleteRows(firstChn, lastChn, globalEdit, true);
}


void CViewPattern::OnDeleteRow()
{
	DeleteRows(m_Selection.GetStartChannel(), m_Selection.GetEndChannel());
}


void CViewPattern::OnDeleteWholeRow()
{
	DeleteRows(0, GetSoundFile()->GetNumChannels() - 1);
}


void CViewPattern::OnDeleteRowGlobal()
{
	DeleteRows(m_Selection.GetStartChannel(), m_Selection.GetEndChannel(), true);
}


void CViewPattern::OnDeleteWholeRowGlobal()
{
	DeleteRows(0, GetSoundFile()->GetNumChannels() - 1, true);
}


void CViewPattern::InsertRows(CHANNELINDEX firstChn, CHANNELINDEX lastChn, bool globalEdit)
{
	InsertOrDeleteRows(firstChn, lastChn, globalEdit, false);
}


void CViewPattern::OnInsertRow()
{
	InsertRows(m_Selection.GetStartChannel(), m_Selection.GetEndChannel());
}


void CViewPattern::OnInsertWholeRow()
{
	InsertRows(0, GetSoundFile()->GetNumChannels() - 1);
}


void CViewPattern::OnInsertRowGlobal()
{
	InsertRows(m_Selection.GetStartChannel(), m_Selection.GetEndChannel(), true);
}


void CViewPattern::OnInsertWholeRowGlobal()
{
	InsertRows(0, GetSoundFile()->GetNumChannels() - 1, true);
}

void CViewPattern::OnSplitPattern()
{
	COrderList &orderList = static_cast<CCtrlPatterns *>(GetControlDlg())->GetOrderList();
	CSoundFile &sndFile = *GetSoundFile();
	const auto &specs = sndFile.GetModSpecifications();
	const PATTERNINDEX sourcePat = m_nPattern;
	const ROWINDEX splitRow = m_MenuCursor.GetRow();
	if(splitRow < 1 || !sndFile.Patterns.IsValidPat(sourcePat) || !sndFile.Patterns[sourcePat].IsValidRow(splitRow))
	{
		MessageBeep(MB_ICONWARNING);
		return;
	}

	// Create a new pattern (ignore if it's too big for this format - if it is, then the source pattern already was too big, too)
	CriticalSection cs;
	const ROWINDEX numSplitRows = sndFile.Patterns[sourcePat].GetNumRows() - splitRow;
	const PATTERNINDEX newPat = sndFile.Patterns.InsertAny(std::max(specs.patternRowsMin, numSplitRows), false);
	if(newPat == PATTERNINDEX_INVALID)
	{
		cs.Leave();
		Reporting::Error(MPT_UFORMAT("Pattern limit of the {} format ({} patterns) has been reached.")(specs.GetFileExtensionUpper(), specs.patternsMax), U_("Split Pattern"));
		return;
	}
	auto &sourcePattern = sndFile.Patterns[sourcePat];
	auto &newPattern = sndFile.Patterns[newPat];

	auto &undo = GetDocument()->GetPatternUndo();
	undo.PrepareUndo(sourcePat, 0, splitRow, sourcePattern.GetNumChannels(), numSplitRows, "Split Pattern");
	undo.PrepareUndo(newPat, 0, 0, newPattern.GetNumChannels(), newPattern.GetNumRows(), "Split Pattern", true);

	auto copyStart = sourcePattern.begin() + sourcePattern.GetNumChannels() * splitRow;
	std::copy(copyStart, sourcePattern.end(), newPattern.begin());

	// Reduce the row number or insert pattern breaks, if the patterns are too small for the format
	sourcePattern.Resize(std::max(specs.patternRowsMin, splitRow));
	if(splitRow != sourcePattern.GetNumRows())
	{
		std::fill(copyStart, sourcePattern.end(), ModCommand::Empty());
		sourcePattern.WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(splitRow - 1).RetryNextRow());
	}
	if(numSplitRows != newPattern.GetNumRows())
	{
		newPattern.WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(numSplitRows - 1).RetryNextRow());
	}

	// Update every occurrence of the split pattern in all order lists
	auto editOrd = GetCurrentOrder();
	for(SEQUENCEINDEX seq = 0; seq < sndFile.Order.GetNumSequences(); seq++)
	{
		const bool isCurrentSeq = (seq == sndFile.Order.GetCurrentSequenceIndex());
		bool editedSeq = false;
		auto &order = sndFile.Order(seq);
		for(ORDERINDEX i = 0; i < order.GetLength(); i++)
		{
			if(order[i] == sourcePat)
			{
				if(!order.insert(i + 1, 1, newPat))
					continue;
				editedSeq = true;
				if(isCurrentSeq)
					orderList.InsertUpdatePlaystate(i, i + 1);
				i++;

				// Slide the current selection accordingly so it doesn't end up in the wrong id
				if(i < editOrd && isCurrentSeq)
					editOrd++;
			}
		}
		if(editedSeq)
			GetDocument()->UpdateAllViews(nullptr, SequenceHint(seq).Data(), this);
	}

	orderList.SetSelection(editOrd + 1);
	SetCurrentRow(0);

	SetModified(true);
	GetDocument()->UpdateAllViews(nullptr, PatternHint(newPat).Names().Data(), this);
}


void CViewPattern::OnEditGoto()
{
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc)
		return;

	ORDERINDEX curOrder = GetCurrentOrder();
	CHANNELINDEX curChannel = GetCurrentChannel() + 1;
	CPatternGotoDialog dlg(this, GetCurrentRow(), curChannel, m_nPattern, curOrder, pModDoc->GetSoundFile());

	if(dlg.DoModal() == IDOK)
	{
		if(dlg.m_nPattern != m_nPattern)
			SetCurrentPattern(dlg.m_nPattern);
		if(dlg.m_nOrder != curOrder)
			SetCurrentOrder(dlg.m_nOrder);
		if(dlg.m_nChannel != curChannel)
			SetCurrentColumn(dlg.m_nChannel - 1);
		if(dlg.m_nRow != GetCurrentRow())
			SetCurrentRow(dlg.m_nRow);
		CriticalSection cs;
		pModDoc->SetElapsedTime(dlg.m_nOrder, dlg.m_nRow, false);
	}
	return;
}


void CViewPattern::OnPatternStep()
{
	PatternStep();
}


void CViewPattern::PatternStep(ROWINDEX row)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if(pMainFrm != nullptr && pModDoc != nullptr)
	{
		CSoundFile &sndFile = pModDoc->GetSoundFile();
		if(!sndFile.Patterns.IsValidPat(m_nPattern))
			return;

		CriticalSection cs;

		// In case we were previously in smooth scrolling mode during live playback, the pattern might be misaligned.
		if(GetSmoothScrollOffset() != 0)
			InvalidatePattern(true, true);

		// Cut instruments/samples in virtual channels
		for(CHANNELINDEX i = sndFile.GetNumChannels(); i < MAX_CHANNELS; i++)
		{
			sndFile.m_PlayState.Chn[i].dwFlags.set(CHN_NOTEFADE | CHN_KEYOFF);
		}
		sndFile.LoopPattern(m_nPattern);
		sndFile.m_PlayState.m_nNextRow = row == ROWINDEX_INVALID ? GetCurrentRow() : row;
		sndFile.m_SongFlags.reset(SONG_PAUSED);
		sndFile.m_SongFlags.set(SONG_STEP);

		SetPlayCursor(m_nPattern, sndFile.m_PlayState.m_nNextRow, 0);
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
			              (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) ||  // Wrap around to next pattern if continous scroll is enabled...
			                  (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_WRAP));     // ...or otherwise if cursor wrap is enabled.
		}
		SetFocus();
	}
}


// Copy cursor to internal clipboard
void CViewPattern::OnCursorCopy()
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
		[[fallthrough]];
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


void CViewPattern::OnVisualizeEffect()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc != nullptr && pModDoc->GetSoundFile().Patterns.IsValidPat(m_nPattern))
	{
		const ROWINDEX row0 = m_Selection.GetStartRow(), row1 = m_Selection.GetEndRow();
		const CHANNELINDEX nchn = m_Selection.GetStartChannel();
		if(m_pEffectVis)
		{
			// Window already there, update data
			m_pEffectVis->UpdateSelection(row0, row1, nchn, m_nPattern);
		} else
		{
			// Open window & send data
			CriticalSection cs;
			try
			{
				m_pEffectVis = std::make_unique<CEffectVis>(this, row0, row1, nchn, *pModDoc, m_nPattern);
				m_pEffectVis->OpenEditor(CMainFrame::GetMainFrame());
				// HACK: to get status window set up; must create clear destinction between
				// construction, 1st draw code and all draw code.
				m_pEffectVis->OnSize(0, 0, 0);
			} catch(mpt::out_of_memory e)
			{
				mpt::delete_out_of_memory(e);
			}
		}
	}
}


// Helper function for sweeping the pattern up and down to find suitable start and end points for interpolation.
// startCond must return true for the start row, endCond must return true for the end row.
PatternRect CViewPattern::SweepPattern(bool(*startCond)(const ModCommand &), bool(*endCond)(const ModCommand &, const ModCommand &)) const
{
	const auto &pattern = GetSoundFile()->Patterns[m_nPattern];
	const ROWINDEX numRows = pattern.GetNumRows();
	const ROWINDEX cursorRow = m_Selection.GetStartRow();
	if(cursorRow >= numRows)
		return {};

	const ModCommand *start = pattern.GetpModCommand(cursorRow, m_Selection.GetStartChannel()), *end = start;

	// Sweep up
	ROWINDEX startRow = ROWINDEX_INVALID;
	for(ROWINDEX row = 0; row <= cursorRow; row++, start -= pattern.GetNumChannels())
	{
		if(startCond(*start))
		{
			startRow = cursorRow - row;
			break;
		}
	}
	if(startRow == ROWINDEX_INVALID)
		return {};

	// Sweep down
	ROWINDEX endRow = ROWINDEX_INVALID;
	for(ROWINDEX row = cursorRow; row < numRows; row++, end += pattern.GetNumChannels())
	{
		if(endCond(*start, *end))
		{
			endRow = row;
			break;
		}
	}
	
	if(endRow == ROWINDEX_INVALID)
		return {};
	
	return {PatternCursor(startRow, m_Selection.GetUpperLeft()), PatternCursor(endRow, m_Selection.GetUpperLeft())};
}


void CViewPattern::Interpolate(PatternCursor::Columns type)
{
	CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern) || !IsEditingEnabled())
		return;

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
		PatternRect sweepSelection;

		switch(type)
		{
		case PatternCursor::noteColumn:
			// Allow note-to-note interpolation only.
			sweepSelection = SweepPattern(
				[](const ModCommand &start) { return start.note != NOTE_NONE; },
				[](const ModCommand &start, const ModCommand &end) { return start.IsNote() && end.IsNote(); });
			break;
		case PatternCursor::instrColumn:
			// Allow interpolation between same instrument, as long as it's not a PC note.
			sweepSelection = SweepPattern(
				[](const ModCommand &start) { return start.instr != 0 && !start.IsPcNote(); },
				[](const ModCommand &start, const ModCommand &end) { return end.instr == start.instr; });
			break;
		case PatternCursor::volumeColumn:
			// Allow interpolation between same volume effect, as long as it's not a PC note.
			sweepSelection = SweepPattern(
				[](const ModCommand &start) { return start.volcmd != VOLCMD_NONE && !start.IsPcNote(); },
				[](const ModCommand &start, const ModCommand &end) { return end.volcmd == start.volcmd && !end.IsPcNote(); });
			break;
		case PatternCursor::effectColumn:
		case PatternCursor::paramColumn:
			// Allow interpolation between same effect, or anything if it's a PC note.
			sweepSelection = SweepPattern(
				[](const ModCommand &start) { return start.command != CMD_NONE || start.IsPcNote(); },
				[](const ModCommand &start, const ModCommand &end) { return (end.command == start.command || start.IsPcNote()) && (!start.IsPcNote() || end.IsPcNote()); });
			break;
		}

		if(sweepSelection.GetNumRows() > 1)
		{
			// Found usable end and start commands: Extend selection.
			SetCurSel(sweepSelection);
		}
	}

	const ROWINDEX row0 = m_Selection.GetStartRow(), row1 = m_Selection.GetEndRow();

	//for all channels where type is selected
	for(auto nchn : validChans)
	{
		if(!IsInterpolationPossible(row0, row1, nchn, type))
			continue;  //skip chans where interpolation isn't possible

		if(!changed)  //ensure we save undo buffer only before any channels are interpolated
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
				vcmd = destCmd.volcmd;
				if(vcmd == VOLCMD_VOLUME && srcCmd.IsNote() && srcCmd.instr)
					vsrc = GetDefaultVolume(srcCmd);
				else
					vsrc = vdest;
			} else if(destCmd.volcmd == VOLCMD_NONE)
			{
				if(vcmd == VOLCMD_VOLUME && destCmd.IsNote() && destCmd.instr)
					vdest = GetDefaultVolume(srcCmd);
				else
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
				if((PCparam == 0 && destCmd.IsPcNote()) || !srcCmd.IsPcNote())
					PCparam = destCmd.GetValueVolCol();
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
			MPT_ASSERT(false);
			return;
		}

		if(vdest < vsrc)
			verr = -verr;

		ModCommand *pcmd = sndFile->Patterns[m_nPattern].GetpModCommand(row0, nchn);

		for(int i = 0; i <= distance; i++, pcmd += sndFile->GetNumChannels())
		{
			switch(type)
			{
			case PatternCursor::noteColumn:
				if((pcmd->note == NOTE_NONE || pcmd->instr == vcmd) && !pcmd->IsPcNote())
				{
					int note = vsrc + ((vdest - vsrc) * i + verr) / distance;
					pcmd->note = static_cast<ModCommand::NOTE>(note);
					if(pcmd->instr == 0)
						pcmd->instr = static_cast<ModCommand::VOLCMD>(vcmd);
				}
				break;

			case PatternCursor::instrColumn:
				if(pcmd->instr == 0)
				{
					int instr = vsrc + ((vdest - vsrc) * i + verr) / distance;
					pcmd->instr = static_cast<ModCommand::INSTR>(instr);
				}
				break;

			case PatternCursor::volumeColumn:
				if((pcmd->volcmd == VOLCMD_NONE || pcmd->volcmd == vcmd) && !pcmd->IsPcNote())
				{
					int vol = vsrc + ((vdest - vsrc) * i + verr) / distance;
					pcmd->vol = static_cast<ModCommand::VOL>(vol);
					pcmd->volcmd = static_cast<ModCommand::VOLCMD>(vcmd);
				}
				break;

			case PatternCursor::effectColumn:
				if(doPCinterpolation)
				{  // With PC/PCs notes, copy PCs note and plug index to all rows where
					// effect interpolation is done if no PC note with non-zero instrument is there.
					const uint16 val = static_cast<uint16>(vsrc + ((vdest - vsrc) * i + verr) / distance);
					if(!pcmd->IsPcNote() || pcmd->instr == 0)
					{
						pcmd->note = PCnote;
						pcmd->instr = static_cast<ModCommand::INSTR>(PCinst);
					}
					pcmd->SetValueVolCol(PCparam);
					pcmd->SetValueEffectCol(val);
				} else if(!pcmd->IsPcNote())
				{
					if((pcmd->command == CMD_NONE) || (pcmd->command == vcmd))
					{
						int val = vsrc + ((vdest - vsrc) * i + verr) / distance;
						pcmd->param = static_cast<ModCommand::PARAM>(val);
						pcmd->command = static_cast<ModCommand::COMMAND>(vcmd);
					}
				}
				break;

			default:
				MPT_ASSERT(false);
			}
		}

		changed = true;

	}  //end for all channels where type is selected

	if(changed)
	{
		SetModified(false);
		InvalidatePattern(false);
	}
}


void CViewPattern::OnResetChannelColors()
{
	CModDoc &modDoc = *GetDocument();
	const CSoundFile &sndFile = *GetSoundFile();
	modDoc.GetPatternUndo().PrepareChannelUndo(0, sndFile.GetNumChannels(), "Reset Channel Colours");
	if(modDoc.SetDefaultChannelColors())
	{
		if(modDoc.SupportsChannelColors())
			modDoc.SetModified();
		modDoc.UpdateAllViews(nullptr, GeneralHint().Channels(), nullptr);
	} else
	{
		modDoc.GetPatternUndo().RemoveLastUndoStep();
	}
}


void CViewPattern::OnTransposeChannel()
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
{
	CInputDlg dlg(this, _T("Enter transpose amount:"), -(NOTE_MAX - NOTE_MIN), (NOTE_MAX - NOTE_MIN), m_nTransposeAmount);
	if(dlg.DoModal() == IDOK)
	{
		m_nTransposeAmount = dlg.resultAsInt;
		TransposeSelection(dlg.resultAsInt);
	}
}


void CViewPattern::OnTransposeCustomQuick()
{
	if(m_nTransposeAmount != 0)
		TransposeSelection(m_nTransposeAmount);
	else
		OnTransposeCustom();
}


bool CViewPattern::TransposeSelection(int transp)
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return false;
	}

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());

	// Don't allow notes outside our supported note range.
	const ModCommand::NOTE noteMin = pSndFile->GetModSpecifications().noteMin;
	const ModCommand::NOTE noteMax = pSndFile->GetModSpecifications().noteMax;

	PrepareUndo(m_Selection, "Transpose");

	std::vector<int> lastGroupSize(pSndFile->GetNumChannels(), 12);
	ApplyToSelection([&] (ModCommand &m, ROWINDEX, CHANNELINDEX chn)
	{
		if(chn == m_Selection.GetStartChannel() && m_Selection.GetStartColumn() > PatternCursor::noteColumn)
			return;

		if(m.IsNote())
		{
			if(m.instr > 0)
			{
				lastGroupSize[chn] = GetDocument()->GetInstrumentGroupSize(m.instr);
			}
			int transpose = transp;
			if(transpose == 12000 || transpose == -12000)
			{
				// Transpose one octave
				transpose = lastGroupSize[chn] * mpt::signum(transpose);
			}
			int note = m.note + transpose;
			Limit(note, noteMin, noteMax);
			m.note = static_cast<ModCommand::NOTE>(note);
		}
	});
	SetModified(false);
	InvalidateSelection();

	if(m_Selection.GetNumChannels() == 1 && m_Selection.GetNumRows() == 1 && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYTRANSPOSE))
	{
		// Preview a single transposed note
		PreviewNote(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
	}

	return true;
}


bool CViewPattern::DataEntry(bool up, bool coarse)
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return false;
	}

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());

	const PatternCursor::Columns column = m_Selection.GetStartColumn();

	// Don't allow notes outside our supported note range.
	const ModCommand::NOTE noteMin = pSndFile->GetModSpecifications().noteMin;
	const ModCommand::NOTE noteMax = pSndFile->GetModSpecifications().noteMax;
	const int instrMax = std::min(static_cast<int>(Util::MaxValueOfType(ModCommand::INSTR())), static_cast<int>(pSndFile->GetNumInstruments() ? pSndFile->GetNumInstruments() : pSndFile->GetNumSamples()));
	const EffectInfo effectInfo(*pSndFile);
	const int offset = up ? 1 : -1;

	PrepareUndo(m_Selection, "Data Entry");

	// Notes per octave for non-TET12 tunings and coarse note steps
	std::vector<int> lastGroupSize(pSndFile->GetNumChannels(), 12);

	bool applyToSpecialNotes = true;
	if(column == PatternCursor::noteColumn)
	{
		const CPattern &pattern = pSndFile->Patterns[m_nPattern];
		const CHANNELINDEX startChn = m_Selection.GetStartChannel(), endChn = m_Selection.GetEndChannel();
		const ROWINDEX endRow = m_Selection.GetEndRow();
		for(ROWINDEX row = m_Selection.GetStartRow(); row <= endRow && applyToSpecialNotes; row++)
		{
			const ModCommand *m = pattern.GetpModCommand(row, startChn);
			for(CHANNELINDEX chn = startChn; chn <= endChn; chn++, m++)
			{
				if(!m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::noteColumn)))
					continue;
				if(m->IsNote())
				{
					applyToSpecialNotes = false;
					break;
				}
			}
		}
	}

	ApplyToSelection([&] (ModCommand &m, ROWINDEX, CHANNELINDEX chn)
	{
		if(column == PatternCursor::noteColumn && m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::noteColumn)))
		{
			// Increase / decrease note
			if(m.IsNote() && !applyToSpecialNotes)
			{
				if(m.instr > 0)
				{
					lastGroupSize[chn] = GetDocument()->GetInstrumentGroupSize(m.instr);
				}
				int note = m.note + offset * (coarse ? lastGroupSize[chn] : 1);
				Limit(note, noteMin, noteMax);
				m.note = (ModCommand::NOTE)note;
			} else if(m.IsSpecialNote() && applyToSpecialNotes)
			{
				ModCommand::NOTE note = m.note;
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
					if(m.IsPcNote() != ModCommand::IsPcNote(note))
					{
						m.Clear();
					}
					m.note = (ModCommand::NOTE)note;
				}
			}
		}
		if(column == PatternCursor::instrColumn && m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::instrColumn)) && m.instr != 0)
		{
			// Increase / decrease instrument
			int instr = m.instr + offset * (coarse ? 10 : 1);
			Limit(instr, 1, m.IsInstrPlug() ? MAX_MIXPLUGINS : instrMax);
			m.instr = (ModCommand::INSTR)instr;
		}
		if(column == PatternCursor::volumeColumn && m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::volumeColumn)))
		{
			// Increase / decrease volume parameter
			if(m.IsPcNote())
			{
				int val = m.GetValueVolCol() + offset * (coarse ? 10 : 1);
				Limit(val, 0, ModCommand::maxColumnValue);
				m.SetValueVolCol(static_cast<uint16>(val));
			} else
			{
				int vol = m.vol + offset * (coarse ? 10 : 1);
				if(m.volcmd == VOLCMD_NONE && m.IsNote() && m.instr)
				{
					m.volcmd = VOLCMD_VOLUME;
					vol = GetDefaultVolume(m);
				}
				ModCommand::VOL minValue = 0, maxValue = 64;
				effectInfo.GetVolCmdInfo(effectInfo.GetIndexFromVolCmd(m.volcmd), nullptr, &minValue, &maxValue);
				Limit(vol, (int)minValue, (int)maxValue);
				m.vol = (ModCommand::VOL)vol;
			}
		}
		if((column == PatternCursor::effectColumn || column == PatternCursor::paramColumn) && (m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::effectColumn)) || m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::paramColumn))))
		{
			// Increase / decrease effect parameter
			if(m.IsPcNote())
			{
				int val = m.GetValueEffectCol() + offset * (coarse ? 10 : 1);
				Limit(val, 0, ModCommand::maxColumnValue);
				m.SetValueEffectCol(static_cast<uint16>(val));
			} else
			{
				int param = m.param + offset * (coarse ? 16 : 1);
				ModCommand::PARAM minValue = 0x00, maxValue = 0xFF;
				if(!m.IsSlideUpDownCommand())
				{
					const auto effectIndex = effectInfo.GetIndexFromEffect(m.command, m.param);
					effectInfo.GetEffectInfo(effectIndex, nullptr, false, &minValue, &maxValue);
					minValue = static_cast<ModCommand::PARAM>(effectInfo.MapPosToValue(effectIndex, minValue));
					maxValue = static_cast<ModCommand::PARAM>(effectInfo.MapPosToValue(effectIndex, maxValue));
				}
				m.param = static_cast<ModCommand::PARAM>(Clamp(param, minValue, maxValue));
			}
		}
	});

	SetModified(false);
	InvalidatePattern();

	if(column == PatternCursor::noteColumn && m_Selection.GetNumChannels() == 1 && m_Selection.GetNumRows() == 1 && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYTRANSPOSE))
	{
		// Preview a single transposed note
		PreviewNote(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
	}

	return true;
}


// Get the velocity at which a given note would be played
int CViewPattern::GetDefaultVolume(const ModCommand &m, ModCommand::INSTR lastInstr) const
{
	const CSoundFile &sndFile = *GetSoundFile();
	SAMPLEINDEX sample = GetDocument()->GetSampleIndex(m, lastInstr);
	if(sample)
		return std::min(sndFile.GetSample(sample).nVolume, uint16(256)) / 4u;
	else if(m.instr > 0 && m.instr <= sndFile.GetNumInstruments() && sndFile.Instruments[m.instr] != nullptr && sndFile.Instruments[m.instr]->HasValidMIDIChannel())
		return std::min(sndFile.Instruments[m.instr]->nGlobalVol, uint32(64));  // For instrument plugins
	else
		return 64;
}


int CViewPattern::GetBaseNote() const
{
	const CModDoc *modDoc = GetDocument();
	INSTRUMENTINDEX instr = static_cast<INSTRUMENTINDEX>(GetCurrentInstrument());
	if(!instr && !IsLiveRecord())
		instr = GetCursorCommand().instr;
	return modDoc->GetBaseNote(instr);
}


ModCommand::NOTE CViewPattern::GetNoteWithBaseOctave(int note) const
{
	const CModDoc *modDoc = GetDocument();
	INSTRUMENTINDEX instr = static_cast<INSTRUMENTINDEX>(GetCurrentInstrument());
	if(!instr && !IsLiveRecord())
		instr = GetCursorCommand().instr;
	return modDoc->GetNoteWithBaseOctave(note, instr);
}


void CViewPattern::OnDropSelection()
{
	CModDoc *pModDoc;
	if((pModDoc = GetDocument()) == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetSoundFile();
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
	CPattern &pattern = sndFile.Patterns[m_nPattern];
	auto origPattern = pattern.GetData();

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
	begin.Sanitize(pattern.GetNumRows(), pattern.GetNumChannels());
	end.Sanitize(pattern.GetNumRows(), pattern.GetNumChannels());
	PatternRect destination(begin, end);

	const bool moveSelection = !m_Status[psKeyboardDragSelect | psCtrlDragSelect];

	BeginWaitCursor();
	pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, 0, 0, sndFile.GetNumChannels(), pattern.GetNumRows(), moveSelection ? "Move Selection" : "Copy Selection");

	const ModCommand empty = ModCommand::Empty();
	auto p = pattern.begin();
	for(ROWINDEX row = 0; row < pattern.GetNumRows(); row++)
	{
		for(CHANNELINDEX chn = 0; chn < sndFile.GetNumChannels(); chn++, p++)
		{
			for(int c = PatternCursor::firstColumn; c <= PatternCursor::lastColumn; c++)
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
				} else
				{
					continue;
				}

				// Copy the data
				const ModCommand &src = (xsrc >= 0 && xsrc < (int)sndFile.GetNumChannels() && ysrc >= 0 && ysrc < (int)sndFile.Patterns[m_nPattern].GetNumRows()) ? origPattern[ysrc * sndFile.GetNumChannels() + xsrc] : empty;
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

	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	SetCursorPosition(begin);
	SetCurSel(destination);
	InvalidatePattern();
	SetModified(false);
	EndWaitCursor();
}


void CViewPattern::OnSetSelInstrument()
{
	SetSelectionInstrument(static_cast<INSTRUMENTINDEX>(GetCurrentInstrument()), false);
}


void CViewPattern::OnRemoveChannelDialog()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	pModDoc->ChangeNumChannels(0);
	SetCurrentPattern(m_nPattern);  //Updating the screen.
}


void CViewPattern::OnRemoveChannel()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	const CSoundFile &sndFile = pModDoc->GetSoundFile();

	if(sndFile.GetNumChannels() <= sndFile.GetModSpecifications().channelsMin)
	{
		Reporting::Error("No channel removed - channel number already at minimum.", "Remove channel");
		return;
	}

	CHANNELINDEX nChn = m_MenuCursor.GetChannel();
	const bool isEmpty = pModDoc->IsChannelUnused(nChn);

	CString str;
	str.Format(_T("Remove channel %d? This channel still contains note data!"), nChn + 1);
	if(isEmpty || Reporting::Confirm(str, "Remove channel") == cnfYes)
	{
		std::vector<bool> keepMask(pModDoc->GetNumChannels(), true);
		keepMask[nChn] = false;
		pModDoc->RemoveChannels(keepMask, true);
		SetCurrentPattern(m_nPattern);  //Updating the screen.
		pModDoc->UpdateAllViews(nullptr, GeneralHint().General().Channels(), this);
	}
}


void CViewPattern::AddChannel(CHANNELINDEX parent, bool afterCurrent)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;

	BeginWaitCursor();
	// Create new channel order, with channel nBefore being an invalid (and thus empty) channel.
	std::vector<CHANNELINDEX> channels(pModDoc->GetNumChannels() + 1, CHANNELINDEX_INVALID);
	CHANNELINDEX i = 0;
	for(CHANNELINDEX nChn = 0; nChn < pModDoc->GetNumChannels() + 1; nChn++)
	{
		if(nChn != (parent + (afterCurrent ? 1 : 0)))
		{
			channels[nChn] = i++;
		}
	}

	if(pModDoc->ReArrangeChannels(channels) != CHANNELINDEX_INVALID)
	{
		auto &chnSettings = pModDoc->GetSoundFile().ChnSettings;
		chnSettings[parent + (afterCurrent ? 1 : 0)].color = chnSettings[parent + (afterCurrent ? 0 : 1)].color;
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(nullptr, GeneralHint().General().Channels(), this);  //refresh channel headers
		SetCurrentPattern(m_nPattern);
	}
	EndWaitCursor();
}


void CViewPattern::OnDuplicateChannel()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;

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
		pModDoc->UpdateAllViews(nullptr, GeneralHint().General().Channels(), this);  //refresh channel headers
		SetCurrentPattern(m_nPattern);
	}
	EndWaitCursor();
}


void CViewPattern::OnRunScript()
{
	;
}



void CViewPattern::OnSwitchToOrderList()
{
	PostCtrlMessage(CTRLMSG_SETFOCUS);
}


void CViewPattern::OnPrevOrder()
{
	PostCtrlMessage(CTRLMSG_PREVORDER);
}


void CViewPattern::OnNextOrder()
{
	PostCtrlMessage(CTRLMSG_NEXTORDER);
}


void CViewPattern::OnUpdateUndo(CCmdUI *pCmdUI)
{
	CModDoc *pModDoc = GetDocument();
	if((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetPatternUndo().CanUndo());
		pCmdUI->SetText(CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditUndo, _T("Undo ") + pModDoc->GetPatternUndo().GetUndoName()));
	}
}


void CViewPattern::OnUpdateRedo(CCmdUI *pCmdUI)
{
	CModDoc *pModDoc = GetDocument();
	if((pCmdUI) && (pModDoc))
	{
		pCmdUI->Enable(pModDoc->GetPatternUndo().CanRedo());
		pCmdUI->SetText(CMainFrame::GetInputHandler()->GetKeyTextFromCommand(kcEditRedo, _T("Redo ") + pModDoc->GetPatternUndo().GetRedoName()));
	}
}


void CViewPattern::OnEditUndo()
{
	UndoRedo(true);
}


void CViewPattern::OnEditRedo()
{
	UndoRedo(false);
}


void CViewPattern::UndoRedo(bool undo)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc && IsEditingEnabled_bmsg())
	{
		CHANNELINDEX oldNumChannels = pModDoc->GetNumChannels();
		PATTERNINDEX pat = undo ? pModDoc->GetPatternUndo().Undo() : pModDoc->GetPatternUndo().Redo();
		const CSoundFile &sndFile = pModDoc->GetSoundFile();
		if(pat < sndFile.Patterns.Size())
		{
			if(pat != m_nPattern)
			{
				// Find pattern in sequence.
				ORDERINDEX matchingOrder = Order().FindOrder(pat, GetCurrentOrder());
				if(matchingOrder != ORDERINDEX_INVALID)
				{
					SetCurrentOrder(matchingOrder);
				}
				SetCurrentPattern(pat);
			} else
			{
				InvalidatePattern(true, true);
			}
			SetModified(false);
			SanitizeCursor();
			UpdateScrollSize();
		}
		if(oldNumChannels != pModDoc->GetNumChannels())
		{
			pModDoc->UpdateAllViews(this, GeneralHint().Channels().ModType(), this);
		}
	}
}


// Apply amplification and fade function to volume
static void AmplifyFade(int &vol, int amp, ROWINDEX row, ROWINDEX numRows, int fadeIn, int fadeOut, Fade::Func &fadeFunc)
{
	const bool doFadeIn = fadeIn != amp, doFadeOut = fadeOut != amp;
	const double fadeStart = fadeIn / 100.0, fadeStartDiff = (amp - fadeIn) / 100.0;
	const double fadeEnd = fadeOut / 100.0, fadeEndDiff = (amp - fadeOut) / 100.0;

	double l;
	if(doFadeIn && doFadeOut)
	{
		ROWINDEX numRows2 = numRows / 2;
		if(row < numRows2)
			l = fadeStart + fadeFunc(static_cast<double>(row) / numRows2) * fadeStartDiff;
		else
			l = fadeEnd + fadeFunc(static_cast<double>(numRows - row) / (numRows - numRows2)) * fadeEndDiff;
	} else if(doFadeIn)
	{
		l = fadeStart + fadeFunc(static_cast<double>(row + 1) / numRows) * fadeStartDiff;
	} else if(doFadeOut)
	{
		l = fadeEnd + fadeFunc(static_cast<double>(numRows - row) / numRows) * fadeEndDiff;
	} else
	{
		l = amp / 100.0;
	}
	vol = mpt::saturate_round<int>(vol * l);
	Limit(vol, 0, 64);
}


void CViewPattern::OnPatternAmplify()
{
	static CAmpDlg::AmpSettings settings{Fade::kLinear, 0, 0, 100, false, false};

	CAmpDlg dlg(this, settings, 0);
	if(dlg.DoModal() != IDOK)
	{
		return;
	}

	CSoundFile &sndFile = *GetSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
		return;

	const bool useVolCol = sndFile.GetModSpecifications().HasVolCommand(VOLCMD_VOLUME);

	BeginWaitCursor();
	PrepareUndo(m_Selection, "Amplify");

	m_Selection.Sanitize(sndFile.Patterns[m_nPattern].GetNumRows(), sndFile.GetNumChannels());
	const CHANNELINDEX firstChannel = m_Selection.GetStartChannel(), lastChannel = m_Selection.GetEndChannel();
	const ROWINDEX firstRow = m_Selection.GetStartRow(), lastRow = m_Selection.GetEndRow();

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
		firstChannelValid = true;  // We cannot start "too far right" in the channel, since this is the last column.
		lastChannelValid = m_Selection.GetLowerRight().CompareColumn(PatternCursor(0, lastChannel, PatternCursor::effectColumn)) >= 0;
	}

	// Adjust min/max channel if they're only partly selected (i.e. volume column or effect column (when using .MOD) is not covered)
	// XXX if only the effect column is marked in the XM format, we cannot amplify volume commands there. Does anyone use that?
	if((!firstChannelValid && firstChannel >= lastChannel) || (!lastChannelValid && lastChannel <= firstChannel))
	{
		EndWaitCursor();
		return;
	}

	// Volume memory for each channel.
	std::vector<ModCommand::VOL> chvol(lastChannel + 1, 64);

	// First, fill the volume memory in case we start the selection before some note
	ApplyToSelection([&] (ModCommand &m, ROWINDEX, CHANNELINDEX chn)
	{
		if((chn == firstChannel && !firstChannelValid) || (chn == lastChannel && !lastChannelValid))
			return;

		if(m.command == CMD_VOLUME)
			chvol[chn] = std::min(m.param, ModCommand::PARAM(64));
		else if(m.volcmd == VOLCMD_VOLUME)
			chvol[chn] = m.vol;
		else if(m.instr != 0)
			chvol[chn] = static_cast<ModCommand::VOL>(GetDefaultVolume(m));
	});

	Fade::Func fadeFunc = GetFadeFunc(settings.fadeLaw);

	// Now do the actual amplification
	const int cy = lastRow - firstRow + 1;  // total rows (for fading)
	ApplyToSelection([&] (ModCommand &m, ROWINDEX nRow, CHANNELINDEX chn)
	{
		if((chn == firstChannel && !firstChannelValid) || (chn == lastChannel && !lastChannelValid))
			return;

		if(m.command == CMD_VOLUME)
			chvol[chn] = std::min(m.param, ModCommand::PARAM(64));
		else if(m.volcmd == VOLCMD_VOLUME)
			chvol[chn] = m.vol;
		else if(m.instr != 0)
			chvol[chn] = static_cast<ModCommand::VOL>(GetDefaultVolume(m));

		if(settings.fadeIn || settings.fadeOut || (m.IsNote() && m.instr != 0))
		{
			// Insert new volume commands where necessary
			if(useVolCol && m.volcmd == VOLCMD_NONE)
			{
				m.volcmd = VOLCMD_VOLUME;
				m.vol = chvol[chn];
			} else if(!useVolCol && m.command == CMD_NONE)
			{
				m.command = CMD_VOLUME;
				m.param = chvol[chn];
			}
		}

		if(m.volcmd == VOLCMD_VOLUME)
		{
			int vol = m.vol;
			AmplifyFade(vol, settings.factor, nRow - firstRow, cy, settings.fadeIn ? settings.fadeInStart : settings.factor, settings.fadeOut ? settings.fadeOutEnd : settings.factor, fadeFunc);
			m.vol = static_cast<ModCommand::VOL>(vol);
		}

		if(m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::effectColumn)) || m_Selection.ContainsHorizontal(PatternCursor(0, chn, PatternCursor::paramColumn)))
		{
			if(m.command == CMD_VOLUME && m.param <= 64)
			{
				int vol = m.param;
				AmplifyFade(vol, settings.factor, nRow - firstRow, cy, settings.fadeIn ? settings.fadeInStart : settings.factor, settings.fadeOut ? settings.fadeOutEnd : settings.factor, fadeFunc);
				m.param = static_cast<ModCommand::PARAM>(vol);
			}
		}
	});
	SetModified(false);
	InvalidateSelection();
	EndWaitCursor();
}


LRESULT CViewPattern::OnPlayerNotify(Notification *pnotify)
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || pnotify == nullptr)
	{
		return 0;
	}

	if(pnotify->type[Notification::Position])
	{
		ORDERINDEX ord = pnotify->order;
		ROWINDEX row = pnotify->row;
		PATTERNINDEX pat = pnotify->pattern;
		bool updateOrderList = false;

		if(m_nLastPlayedOrder != ord)
		{
			updateOrderList = true;
			m_nLastPlayedOrder = ord;
		}

		if(row < m_nLastPlayedRow)
		{
			InvalidateChannelsHeaders();
		}
		m_nLastPlayedRow = row;

		if(!pSndFile->m_SongFlags[SONG_PAUSED | SONG_STEP])
		{
			const auto &order = Order();
			if(ord >= order.GetLength() || order[ord] != pat)
			{
				//order doesn't correlate with pattern, so mark it as invalid
				ord = ORDERINDEX_INVALID;
			}

			if(m_pEffectVis && m_pEffectVis->m_hWnd)
			{
				m_pEffectVis->SetPlayCursor(pat, row);
			}

			// Simple detection of backwards-going patterns to avoid jerky animation
			m_nNextPlayRow = ROWINDEX_INVALID;
			if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SMOOTHSCROLL) && pSndFile->Patterns.IsValidPat(pat) && pSndFile->Patterns[pat].IsValidRow(row))
			{
				for(const ModCommand &m : pSndFile->Patterns[pat].GetRow(row))
				{
					if(m.command == CMD_PATTERNBREAK)
						m_nNextPlayRow = m.param;
					else if(m.command == CMD_POSITIONJUMP && (m_nNextPlayRow == ROWINDEX_INVALID || pSndFile->GetType() == MOD_TYPE_XM))
						m_nNextPlayRow = 0;
				}
			}
			if(m_nNextPlayRow == ROWINDEX_INVALID)
				m_nNextPlayRow = row + 1;

			m_nTicksOnRow = pnotify->ticksOnRow;
			SetPlayCursor(pat, row, pnotify->tick);
			// Don't follow song if user drags selections or scrollbars.
			if((m_Status & (psFollowSong | psDragActive)) == psFollowSong)
			{
				if(pat < pSndFile->Patterns.Size())
				{
					if(pat != m_nPattern || ord != m_nOrder || updateOrderList)
					{
						if(pat != m_nPattern)
							SetCurrentPattern(pat, row);
						else if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_SHOWPREVIOUS)
							InvalidatePattern(true, true);  // Redraw previous / next pattern

						if(ord < order.GetLength())
						{
							m_nOrder = ord;
							SendCtrlMessage(CTRLMSG_NOTIFYCURRENTORDER, ord);
						}
						updateOrderList = false;
					}
					if(row != GetCurrentRow())
					{
						SetCurrentRow((row < pSndFile->Patterns[pat].GetNumRows()) ? row : 0, false, false);
					}
				}
			} else
			{
				if(updateOrderList)
				{
					SendCtrlMessage(CTRLMSG_FORCEREFRESH);  //force orderlist refresh
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
		ChnVUMeters.fill(0);  // Also zero all non-visible VU meters
		SetPlayCursor(PATTERNINDEX_INVALID, ROWINDEX_INVALID, 0);
	}

	UpdateIndicator(false);

	return 0;
}

CHANNELINDEX CViewPattern::GetRecordChannelForPCEvent(PLUGINDEX plugSlot, PlugParamIndex paramIndex) const
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return 0;

	CSoundFile &sndFile = pModDoc->GetSoundFile();
	const auto plugParam = std::make_pair(static_cast<PLUGINDEX>(plugSlot), static_cast<PlugParamIndex>(paramIndex));
	const PatternEditPos editPos = GetEditPos(sndFile, IsLiveRecord());
	const CHANNELINDEX editChn = editPos.channel;
	const ROWINDEX row = editPos.row;
	const PATTERNINDEX pattern = editPos.pattern;
	const auto editGroup = pModDoc->GetChannelRecordGroup(editChn);
	CHANNELINDEX candidateChn = CHANNELINDEX_INVALID;

	for(CHANNELINDEX c = 0; c < sndFile.GetNumChannels(); c++)
	{
		if(pModDoc->GetChannelRecordGroup(c) != editGroup)
			continue;

		const ModCommand &m = *sndFile.Patterns[pattern].GetpModCommand(row, c);
		if((m_previousPCevent[c] == plugParam && m.IsEmpty()) || (m.IsPcNote() && m.instr == plugSlot + 1 && m.GetValueVolCol() == paramIndex))
		{
			return c;
			break;
		}
		if(m.IsEmpty() && (candidateChn == CHANNELINDEX_INVALID || c == editChn))
		{
			candidateChn = c;
		}
	}
	if(candidateChn != CHANNELINDEX_INVALID)
		return candidateChn;
	else
		return editChn;
}

// record plugin parameter changes into current pattern
LRESULT CViewPattern::OnRecordPlugParamChange(WPARAM plugSlot, LPARAM paramIndex)
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr || !IsEditingEnabled())
		return 0;
	
	CSoundFile &sndFile = pModDoc->GetSoundFile();

	IMixPlugin *pPlug = sndFile.m_MixPlugins[plugSlot].pMixPlugin;
	if(pPlug == nullptr)
		return 0;

	// Work out where and how to put the new data
	const bool usePCevent = sndFile.GetModSpecifications().HasNote(NOTE_PCS);
	const PatternEditPos editPos = GetEditPos(sndFile, IsLiveRecord());
	const ROWINDEX row = editPos.row;
	const PATTERNINDEX pattern = editPos.pattern;
	CHANNELINDEX chn = editPos.channel;
	const auto plugParam = std::make_pair(static_cast<PLUGINDEX>(plugSlot), static_cast<PlugParamIndex>(paramIndex));
	const bool doMultiChannelRecording = pModDoc->GetChannelRecordGroup(chn) != RecordGroup::NoGroup;

	if(usePCevent && doMultiChannelRecording)
		chn = GetRecordChannelForPCEvent(plugParam.first, plugParam.second);

	ModCommand &mSrc = *sndFile.Patterns[pattern].GetpModCommand(row, chn);
	ModCommand m = mSrc;

	if(usePCevent)
	{
		// MPTM: Use PC Notes

		// only overwrite existing PC Notes
		if(m.IsEmpty() || m.IsPcNote())
		{
			m.Set(NOTE_PCS, static_cast<ModCommand::INSTR>(plugSlot + 1), static_cast<uint16>(paramIndex), static_cast<uint16>(pPlug->GetParameter(static_cast<PlugParamIndex>(paramIndex)) * ModCommand::maxColumnValue));
			m_previousPCevent[chn] = plugParam;
		}
	} else if(sndFile.GetModSpecifications().HasCommand(CMD_SMOOTHMIDI))
	{
		// Other formats: Use MIDI macros

		// Figure out which plug param (if any) is controllable using the active macro on this channel.
		int activePlugParam = -1;
		auto activeMacro = sndFile.m_PlayState.Chn[chn].nActiveMacro;

		if(sndFile.m_MidiCfg.GetParameteredMacroType(activeMacro) == kSFxPlugParam)
			activePlugParam = sndFile.m_MidiCfg.MacroToPlugParam(activeMacro);

		// If the wrong macro is active, see if we can find the right one.
		// If we can, activate it for this chan by writing appropriate SFx command it.
		if(activePlugParam != paramIndex)
		{
			int foundMacro = sndFile.m_MidiCfg.FindMacroForParam(static_cast<PlugParamIndex>(paramIndex));
			if(foundMacro >= 0)
			{
				sndFile.m_PlayState.Chn[chn].nActiveMacro = static_cast<uint8>(foundMacro);
				if(m.command == CMD_NONE || m.command == CMD_SMOOTHMIDI || m.command == CMD_MIDI)  //we overwrite existing Zxx and \xx only.
				{
					m.command = CMD_S3MCMDEX;
					if(!sndFile.GetModSpecifications().HasCommand(CMD_S3MCMDEX))
						m.command = CMD_MODCMDEX;
					m.param = 0xF0 | (foundMacro & 0x0F);
				}
			}
		}

		// Write the data, but we only overwrite if the command is a macro anyway.
		if(m.command == CMD_NONE || m.command == CMD_SMOOTHMIDI || m.command == CMD_MIDI)
		{
			m.command = CMD_SMOOTHMIDI;
			PlugParamValue param = pPlug->GetParameter(static_cast<PlugParamIndex>(paramIndex));
			Limit(param, 0.0f, 1.0f);
			m.param = static_cast<ModCommand::PARAM>(param * 127.0f);
		}
	}

	if(m != mSrc)
	{
		pModDoc->GetPatternUndo().PrepareUndo(pattern, chn, row, 1, 1, "Automation Entry");
		mSrc = m;
		InvalidateCell(PatternCursor(row, chn));
		SetModified(false);
	}

	return 0;
}


PatternEditPos CViewPattern::GetEditPos(const CSoundFile &sndFile, const bool liveRecord) const
{
	PatternEditPos editPos;
	if(liveRecord)
	{
		if(m_nPlayPat != PATTERNINDEX_INVALID)
		{
			editPos.row = m_nPlayRow;
			editPos.order = GetCurrentOrder();
			editPos.pattern = m_nPlayPat;
		} else
		{
			editPos.row = sndFile.m_PlayState.m_nRow;
			editPos.order = sndFile.m_PlayState.m_nCurrentOrder;
			editPos.pattern = sndFile.m_PlayState.m_nPattern;
		}

		if(!sndFile.Patterns.IsValidPat(editPos.pattern) || !sndFile.Patterns[editPos.pattern].IsValidRow(editPos.row))
		{
			editPos.row = GetCurrentRow();
			editPos.order = GetCurrentOrder();
			editPos.pattern = m_nPattern;
		}
		const auto &order = Order();
		if(!order.IsValidPat(editPos.order) || order[editPos.order] != editPos.pattern)
		{
			ORDERINDEX realOrder = order.FindOrder(editPos.pattern, editPos.order);
			if(realOrder != ORDERINDEX_INVALID)
				editPos.order = realOrder;
		}
	} else
	{
		editPos.row = GetCurrentRow();
		editPos.order = GetCurrentOrder();
		editPos.pattern = m_nPattern;
	}
	editPos.channel = GetCurrentChannel();
	return editPos;
}


// Return ModCommand at the given cursor position of the current pattern.
// If the position is not valid, a pointer to a dummy command is returned.
ModCommand &CViewPattern::GetModCommand(PatternCursor cursor)
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(GetCurrentPattern()) && pSndFile->Patterns[GetCurrentPattern()].IsValidRow(cursor.GetRow()))
	{
		return *pSndFile->Patterns[GetCurrentPattern()].GetpModCommand(cursor.GetRow(), cursor.GetChannel());
	}
	// Failed.
	static ModCommand dummy;
	return dummy;
}


// Sanitize cursor so that it can't point to an invalid position in the current pattern.
void CViewPattern::SanitizeCursor()
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(GetCurrentPattern()))
	{
		const auto &pattern = GetSoundFile()->Patterns[m_nPattern];
		m_Cursor.Sanitize(pattern.GetNumRows(), pattern.GetNumChannels());
		m_Selection.Sanitize(pattern.GetNumRows(), pattern.GetNumChannels());
	}
};


// Returns pointer to modcommand at given position.
// If the position is not valid, a pointer to a dummy command is returned.
ModCommand &CViewPattern::GetModCommand(CSoundFile &sndFile, const PatternEditPos &pos)
{
	static ModCommand dummy;
	if(sndFile.Patterns.IsValidPat(pos.pattern) && pos.row < sndFile.Patterns[pos.pattern].GetNumRows() && pos.channel < sndFile.GetNumChannels())
		return *sndFile.Patterns[pos.pattern].GetpModCommand(pos.row, pos.channel);
	else
		return dummy;
}


LRESULT CViewPattern::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
{
	const uint32 midiData = static_cast<uint32>(dwMidiDataParam);
	static uint8 midiVolume = 127;

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if(pModDoc == nullptr || pMainFrm == nullptr)
		return 0;

	CSoundFile &sndFile = pModDoc->GetSoundFile();

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

	const uint8 midiByte1 = MIDIEvents::GetDataByte1FromEvent(midiData);
	const uint8 midiByte2 = MIDIEvents::GetDataByte2FromEvent(midiData);
	const uint8 channel = MIDIEvents::GetChannelFromEvent(midiData);

	const uint8 nNote = midiByte1 + NOTE_MIN;
	int vol = midiByte2;  // At this stage nVol is a non linear value in [0;127]
	                      // Need to convert to linear in [0;64] - see below
	MIDIEvents::EventType event = MIDIEvents::GetTypeFromEvent(midiData);

	if((event == MIDIEvents::evNoteOn) && !vol)
		event = MIDIEvents::evNoteOff;  //Convert event to note-off if req'd

	// Handle MIDI mapping.
	PLUGINDEX mappedIndex = uint8_max;
	PlugParamIndex paramIndex = 0;
	uint16 paramValue = uint16_max;
	bool captured = sndFile.GetMIDIMapper().OnMIDImsg(midiData, mappedIndex, paramIndex, paramValue);

	// Handle MIDI messages assigned to shortcuts
	CInputHandler *ih = CMainFrame::GetInputHandler();
	if(ih->HandleMIDIMessage(static_cast<InputTargetContext>(kCtxViewPatterns + 1 + m_Cursor.GetColumnType()), midiData) != kcNull
	   || ih->HandleMIDIMessage(kCtxAllContexts, midiData) != kcNull)
	{
		// Mapped to a command, no need to pass message on.
		captured = true;
	}

	// Write parameter control commands if needed.
	if(paramValue != uint16_max && IsEditingEnabled() && sndFile.GetType() == MOD_TYPE_MPT)
	{
		const bool liveRecord = IsLiveRecord();

		PatternEditPos editPos = GetEditPos(sndFile, liveRecord);
		if(pModDoc->GetChannelRecordGroup(editPos.channel) != RecordGroup::NoGroup)
			editPos.channel = GetRecordChannelForPCEvent(mappedIndex - 1, paramIndex);

		ModCommand &m = GetModCommand(sndFile, editPos);
		pModDoc->GetPatternUndo().PrepareUndo(editPos.pattern, editPos.channel, editPos.row, 1, 1, "MIDI Mapping Record");
		m.Set(NOTE_PCS, mappedIndex, static_cast<uint16>(paramIndex), static_cast<uint16>((paramValue * ModCommand::maxColumnValue) / 16383));
		m_previousPCevent[editPos.channel] = std::make_pair(static_cast<PLUGINDEX>(mappedIndex - 1), paramIndex);
		if(!liveRecord)
			InvalidateRow(editPos.row);
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(this, PatternHint(editPos.pattern).Data(), this);
	}

	if(captured)
	{
		// Event captured by MIDI mapping or shortcut, no need to pass message on.
		return 1;
	}

	const auto &modSpecs = sndFile.GetModSpecifications();
	bool recordParamAsZxx = false;

	switch(event)
	{
	case MIDIEvents::evNoteOff:  // Note Off
		if(m_midiSustainActive[channel])
		{
			m_midiSustainBuffer[channel].push_back(midiData);
			return 1;
		}
		// The following method takes care of:
		// . Silencing specific active notes (just setting nNote to 255 as was done before is not acceptible)
		// . Entering a note off in pattern if required
		TempStopNote(nNote, ((TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_RECORDNOTEOFF) != 0));
		break;

	case MIDIEvents::evNoteOn:  // Note On
		// Continue playing as soon as MIDI notes are being received
		if((pMainFrm->GetSoundFilePlaying() != &sndFile || sndFile.IsPaused()) && (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN))
			pModDoc->OnPatternPlayNoLoop();

		vol = CMainFrame::ApplyVolumeRelatedSettings(midiData, midiVolume);
		if(vol < 0)
			vol = -1;
		else
			vol = (vol + 3) / 4;  //Value from [0,256] to [0,64]
		TempEnterNote(nNote, vol, true);
		break;

	case MIDIEvents::evPolyAftertouch:  // Polyphonic aftertouch
		EnterAftertouch(nNote, vol);
		break;

	case MIDIEvents::evChannelAftertouch:  // Channel aftertouch
		EnterAftertouch(NOTE_NONE, midiByte1);
		break;

	case MIDIEvents::evPitchBend:  // Pitch wheel
		recordParamAsZxx = (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDIMACROPITCHBEND) != 0 || modSpecs.HasCommand(CMD_FINETUNE);
		break;

	case MIDIEvents::evControllerChange:  //Controller change
		// Checking whether to record MIDI controller change as MIDI macro change.
		// Don't write this if command was already written by MIDI mapping.
		if((paramValue == uint16_max || sndFile.GetType() != MOD_TYPE_MPT)
		   && (TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_MIDIMACROCONTROL)
		   && !TrackerSettings::Instance().midiIgnoreCCs.Get()[midiByte1 & 0x7F])
		{
			recordParamAsZxx = true;
		}

		switch(midiByte1)
		{
		case MIDIEvents::MIDICC_Volume_Coarse:
			midiVolume = midiByte2;
			break;

		case MIDIEvents::MIDICC_HoldPedal_OnOff:
			m_midiSustainActive[channel] = (midiByte2 >= 0x40);
			if(!m_midiSustainActive[channel])
			{
				// Release all notes
				for(const auto offEvent : m_midiSustainBuffer[channel])
				{
					OnMidiMsg(offEvent, 0);
				}
				m_midiSustainBuffer[channel].clear();
			}
			recordParamAsZxx = false;
			break;
		}
		break;

	case MIDIEvents::evSystem:
		if(TrackerSettings::Instance().m_dwMidiSetup & MIDISETUP_RESPONDTOPLAYCONTROLMSGS)
		{
			// Respond to MIDI song messages
			switch(channel)
			{
			case MIDIEvents::sysStart:  //Start song
				pModDoc->OnPlayerPlayFromStart();
				break;

			case MIDIEvents::sysContinue:  //Continue song
				pModDoc->OnPlayerPlay();
				break;

			case MIDIEvents::sysStop:  //Stop song
				pModDoc->OnPlayerStop();
				break;
			}
		}
		break;
	}

	// Write CC or pitch bend message as MIDI macro change.
	if(recordParamAsZxx && IsEditingEnabled())
	{
		const bool liveRecord = IsLiveRecord();

		const auto editpos = GetEditPos(sndFile, liveRecord);
		ModCommand &m = GetModCommand(sndFile, editpos);
		bool update = false;

		if(event == MIDIEvents::evPitchBend && (m.command == CMD_NONE || m.command == CMD_FINETUNE || m.command == CMD_FINETUNE_SMOOTH) && modSpecs.HasCommand(CMD_FINETUNE))
		{
			pModDoc->GetPatternUndo().PrepareUndo(editpos.pattern, editpos.channel, editpos.row, 1, 1, "MIDI Record Entry");
			m.command = (m.command == CMD_NONE) ? CMD_FINETUNE : CMD_FINETUNE_SMOOTH;
			m.param = (midiByte2 << 1) | (midiByte1 >> 7);
			update = true;
			if(IsLiveRecord())
			{
				CriticalSection cs;
				sndFile.ProcessFinetune(editpos.pattern, editpos.row, editpos.channel, false);
			}
		} else if(m.IsPcNote())
		{
			pModDoc->GetPatternUndo().PrepareUndo(editpos.pattern, editpos.channel, editpos.row, 1, 1, "MIDI Record Entry");
			m.SetValueEffectCol(static_cast<decltype(m.GetValueEffectCol())>(Util::muldivr(midiByte2, ModCommand::maxColumnValue, 127)));
			update = true;
		} else if((m.command == CMD_NONE || m.command == CMD_SMOOTHMIDI || m.command == CMD_MIDI)
			&& (modSpecs.HasCommand(CMD_SMOOTHMIDI) || modSpecs.HasCommand(CMD_MIDI)))
		{
			// Write command only if there's no existing command or already a midi macro command.
			pModDoc->GetPatternUndo().PrepareUndo(editpos.pattern, editpos.channel, editpos.row, 1, 1, "MIDI Record Entry");
			m.command = modSpecs.HasCommand(CMD_SMOOTHMIDI) ? CMD_SMOOTHMIDI : CMD_MIDI;
			m.param = midiByte2;
			update = true;
		}
		if(update)
		{
			pModDoc->SetModified();
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
		IMixPlugin *plug = sndFile.GetInstrumentPlugin(instr);
		if(plug)
		{
			plug->MidiSend(midiData);
			// Sending MIDI may modify the plugin. For now, if MIDI data
			// is not active sensing, set modified.
			if(midiData != MIDIEvents::System(MIDIEvents::sysActiveSense))
				pModDoc->SetModified();
		}
	}

	return 1;
}


LRESULT CViewPattern::OnModViewMsg(WPARAM wParam, LPARAM lParam)
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
		m_nOrder = static_cast<ORDERINDEX>(SendCtrlMessage(CTRLMSG_GETCURRENTORDER));
		SetCurrentPattern(static_cast<PATTERNINDEX>(lParam));
		break;

	case VIEWMSG_GETCURRENTPOS:
		return (m_nPattern << 16) | GetCurrentRow();

	case VIEWMSG_FOLLOWSONG:
		m_Status.reset(psFollowSong);
		if(lParam)
		{
			CModDoc *pModDoc = GetDocument();
			m_Status.set(psFollowSong);
			if(pModDoc)
				pModDoc->SetNotifications(Notification::Position | Notification::VUMeters);
			if(pModDoc)
				pModDoc->SetFollowWnd(m_hWnd);
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
		m_nSpacing = static_cast<UINT>(lParam);
		break;

	case VIEWMSG_PATTERNPROPERTIES:
		ShowPatternProperties(static_cast<PATTERNINDEX>(lParam));
		GetParentFrame()->SetActiveView(this);
		break;

	case VIEWMSG_SETVUMETERS:
		m_Status.set(psShowVUMeters, !!lParam);
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true, true);
		break;

	case VIEWMSG_SETPLUGINNAMES:
		m_Status.set(psShowPluginNames, !!lParam);
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true, true);
		break;

	case VIEWMSG_DOMIDISPACING:
		if(m_nSpacing)
		{
			int temp = timeGetTime();
			if(temp - lParam >= 60)
			{
				CModDoc *pModDoc = GetDocument();
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if(!m_Status[psFollowSong]
				   || (pMainFrm->GetFollowSong(pModDoc) != m_hWnd)
				   || (pModDoc->GetSoundFile().IsPaused()))
				{
					SetCurrentRow(GetCurrentRow() + m_nSpacing);
				}
			} else
			{
				Sleep(0);
				PostMessage(WM_MOD_VIEWMSG, VIEWMSG_DOMIDISPACING, lParam);
			}
		}
		break;

	case VIEWMSG_LOADSTATE:
		if(lParam)
		{
			PATTERNVIEWSTATE *pState = (PATTERNVIEWSTATE *)lParam;
			if(pState->nDetailLevel != PatternCursor::firstColumn)
				m_nDetailLevel = pState->nDetailLevel;
			if(pState->initialized)
			{
				SetCurrentPattern(pState->nPattern);
				// Fix: Horizontal scrollbar pos screwed when selecting with mouse
				SetCursorPosition(pState->cursor);
				SetCurSel(pState->selection);
			}
		}
		break;

	case VIEWMSG_SAVESTATE:
		if(lParam)
		{
			PATTERNVIEWSTATE *pState = (PATTERNVIEWSTATE *)lParam;
			pState->initialized = true;
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
		if(pModDoc->ExpandPattern(m_nPattern))
		{
			m_Cursor.SetRow(m_Cursor.GetRow() * 2);
			SetCurrentPattern(m_nPattern);
		}
		break;
	}

	case VIEWMSG_SHRINKPATTERN:
	{
		CModDoc *pModDoc = GetDocument();
		if(pModDoc->ShrinkPattern(m_nPattern))
		{
			m_Cursor.SetRow(m_Cursor.GetRow() / 2);
			SetCurrentPattern(m_nPattern);
		}
		break;
	}

	case VIEWMSG_COPYPATTERN:
	{
		const CSoundFile *pSndFile = GetSoundFile();
		if(pSndFile != nullptr && pSndFile->Patterns.IsValidPat(m_nPattern))
		{
			CopyPattern(m_nPattern, PatternRect(PatternCursor(0, 0), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn)));
		}
		break;
	}

	case VIEWMSG_PASTEPATTERN:
		PastePattern(m_nPattern, PatternCursor(0), PatternClipboard::pmOverwrite);
		InvalidatePattern();
		break;

	case VIEWMSG_AMPLIFYPATTERN:
		OnPatternAmplify();
		break;

	case VIEWMSG_SETDETAIL:
		if(lParam != m_nDetailLevel)
		{
			m_nDetailLevel = static_cast<PatternCursor::Columns>(lParam);
			UpdateSizes();
			UpdateScrollSize();
			SetCurrentColumn(m_Cursor);
			InvalidatePattern(true, true);
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
{
	ROWINDEX row = GetCurrentRow();
	const bool upwards = distance < 0;
	const int distanceAbs = std::abs(distance);

	if(snap && distanceAbs)
		// cppcheck false-positive
		// cppcheck-suppress signConversion
		row = (((row + (upwards ? -1 : 0)) / distanceAbs) + (upwards ? 0 : 1)) * distanceAbs;
	else
		row += distance;
	row = SetCurrentRow(row, true);

	if(IsLiveRecord() && !m_Status[psDragActive])
	{
		CriticalSection cs;
		CSoundFile &sndFile = GetDocument()->GetSoundFile();
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
	} else
	{
		if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYNAVIGATEROW)
		{
			PatternStep(row);
		}
	}
}


LRESULT CViewPattern::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
{
	CModDoc *pModDoc = GetDocument();
	if(!pModDoc)
		return kcNull;

	CSoundFile &sndFile = pModDoc->GetSoundFile();

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
												pModDoc->ToggleChannelRecordGroup(c, RecordGroup::Group1);
											InvalidateChannelsHeaders(); return wParam;
		case kcChannelSplitRecordSelect:	for(CHANNELINDEX c = m_Selection.GetStartChannel(); c <= m_Selection.GetEndChannel(); c++)
												pModDoc->ToggleChannelRecordGroup(c, RecordGroup::Group2);
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
		case kcTransposeCustomQuick:		OnTransposeCustomQuick(); return wParam;
		case kcDataEntryUp:					DataEntry(true, false); return wParam;
		case kcDataEntryDown:				DataEntry(false, false); return wParam;
		case kcDataEntryUpCoarse:			DataEntry(true, true); return wParam;
		case kcDataEntryDownCoarse:			DataEntry(false, true); return wParam;
		case kcSelectChannel:				OnSelectCurrentChannel(); return wParam;
		case kcSelectColumn:				OnSelectCurrentColumn(); return wParam;
		case kcPatternAmplify:				OnPatternAmplify(); return wParam;
		case kcPatternSetInstrumentNotEmpty:
		case kcPatternSetInstrument:		SetSelectionInstrument(static_cast<INSTRUMENTINDEX>(GetCurrentInstrument()), wParam == kcPatternSetInstrument); return wParam;
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
		case kcNavigateNextChan: SetCurrentColumn((GetCurrentChannel() + 1) % sndFile.GetNumChannels(), m_Cursor.GetColumnType()); return wParam;
		case kcNavigatePrevChanSelect:
		case kcNavigatePrevChan:{if(GetCurrentChannel() > 0)
									SetCurrentColumn((GetCurrentChannel() - 1) % sndFile.GetNumChannels(), m_Cursor.GetColumnType());
								else
									SetCurrentColumn(sndFile.GetNumChannels() - 1, m_Cursor.GetColumnType());
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
		case kcEndHorizontal:	if (m_Cursor.CompareColumn(PatternCursor(0, sndFile.GetNumChannels() - 1, m_nDetailLevel)) < 0) SetCurrentColumn(sndFile.GetNumChannels() - 1, m_nDetailLevel);
								else if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								return wParam;
		case kcEndVerticalSelect:
		case kcEndVertical:		if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								else if (m_Cursor.CompareColumn(PatternCursor(0, sndFile.GetNumChannels() - 1, m_nDetailLevel)) < 0) SetCurrentColumn(sndFile.GetNumChannels() - 1, m_nDetailLevel);
								return wParam;
		case kcEndAbsoluteSelect:
		case kcEndAbsolute:		SetCurrentColumn(sndFile.GetNumChannels() - 1, m_nDetailLevel);
								if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								return wParam;

		case kcPrevEntryInColumn:
		case kcNextEntryInColumn:
			JumpToPrevOrNextEntry(wParam == kcNextEntryInColumn, false);
			return wParam;
		case kcPrevEntryInColumnSelect:
		case kcNextEntryInColumnSelect:
			JumpToPrevOrNextEntry(wParam == kcNextEntryInColumnSelect, true);
			return wParam;

		case kcNextPattern:	{	PATTERNINDEX n = m_nPattern + 1;
								while ((n < sndFile.Patterns.Size()) && !sndFile.Patterns.IsValidPat(n)) n++;
								SetCurrentPattern((n < sndFile.Patterns.Size()) ? n : 0);
								ORDERINDEX currentOrder = GetCurrentOrder();
								ORDERINDEX newOrder = Order().FindOrder(m_nPattern, currentOrder, true);
								if(newOrder != ORDERINDEX_INVALID)
									SetCurrentOrder(newOrder);
								return wParam;
							}
		case kcPrevPattern: {	PATTERNINDEX n = (m_nPattern) ? m_nPattern - 1 : sndFile.Patterns.Size() - 1;
								while (n > 0 && !sndFile.Patterns.IsValidPat(n)) n--;
								SetCurrentPattern(n);
								ORDERINDEX currentOrder = GetCurrentOrder();
								ORDERINDEX newOrder = Order().FindOrder(m_nPattern, currentOrder, false);
								if(newOrder != ORDERINDEX_INVALID)
									SetCurrentOrder(newOrder);
								return wParam;
							}
		case kcPrevSequence:
		case kcNextSequence:
			SendCtrlMessage(CTRLMSG_PAT_SETSEQUENCE, mpt::wrapping_modulo(sndFile.Order.GetCurrentSequenceIndex() + (wParam == kcPrevSequence ? -1 : 1), sndFile.Order.GetNumSequences()));
			return wParam;
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
									PatternCursor(m_Selection.GetEndRow(), sndFile.GetNumChannels(), PatternCursor::lastColumn));
								return wParam;

		case kcClearRow:		OnClearField(RowMask(), false);	return wParam;
		case kcClearField:		OnClearField(RowMask(m_Cursor), false);	return wParam;
		case kcClearFieldITStyle: OnClearField(RowMask(m_Cursor), false, true);	return wParam;
		case kcClearRowStep:	OnClearField(RowMask(), true);	return wParam;
		case kcClearFieldStep:	OnClearField(RowMask(m_Cursor), true);	return wParam;
		case kcClearFieldStepITStyle:	OnClearField(RowMask(m_Cursor), true, true);	return wParam;

		case kcDeleteRow:				OnDeleteRow(); return wParam;
		case kcDeleteWholeRow:			OnDeleteWholeRow(); return wParam;
		case kcDeleteRowGlobal:			OnDeleteRowGlobal(); return wParam;
		case kcDeleteWholeRowGlobal:	OnDeleteWholeRowGlobal(); return wParam;

		case kcInsertRow:				OnInsertRow(); return wParam;
		case kcInsertWholeRow:			OnInsertWholeRow(); return wParam;
		case kcInsertRowGlobal:			OnInsertRowGlobal(); return wParam;
		case kcInsertWholeRowGlobal:	OnInsertWholeRowGlobal(); return wParam;

		case kcShowNoteProperties: ShowEditWindow(); return wParam;
		case kcShowPatternProperties: OnPatternProperties(); return wParam;
		case kcShowSplitKeyboardSettings:	SetSplitKeyboardSettings(); return wParam;
		case kcShowEditMenu:
			{
				CPoint pt = GetPointFromPosition(m_Cursor);
				pt.x += GetChannelWidth() / 2;
				pt.y += GetRowHeight() / 2;
				OnRButtonDown(0, pt);
			}
			return wParam;
		case kcShowChannelCtxMenu:
			{
				CPoint pt = GetPointFromPosition(m_Cursor);
				pt.x += GetChannelWidth() / 2;
				pt.y = (m_szHeader.cy - m_szPluginHeader.cy) / 2;
				OnRButtonDown(0, pt);
			}
			return wParam;
		case kcShowChannelPluginCtxMenu:
			{
				CPoint pt = GetPointFromPosition(m_Cursor);
				pt.x += GetChannelWidth() / 2;
				pt.y = m_szHeader.cy - m_szPluginHeader.cy / 2;
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
			OnEditCopy();
			[[fallthrough]];
		case kcLoseSelection:
			SetSelToCursor();
			return wParam;
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
		case kcSwitchToOrderList: OnSwitchToOrderList(); return wParam;
		case kcToggleOverflowPaste:	TrackerSettings::Instance().m_dwPatternSetup ^= PATTERN_OVERFLOWPASTE; return wParam;
		case kcToggleNoteOffRecordPC: TrackerSettings::Instance().m_dwPatternSetup ^= PATTERN_KBDNOTEOFF; return wParam;
		case kcToggleNoteOffRecordMIDI: TrackerSettings::Instance().m_dwMidiSetup ^= MIDISETUP_RECORDNOTEOFF; return wParam;
		case kcPatternEditPCNotePlugin: OnTogglePCNotePluginEditor(); return wParam;
		case kcQuantizeSettings: OnSetQuantize(); return wParam;
		case kcLockPlaybackToRows: OnLockPatternRows(); return wParam;
		case kcFindInstrument: FindInstrument(); return wParam;
		case kcChannelSettings:
			{
				// Open centered Quick Channel Settings dialog.
				CRect windowPos;
				GetWindowRect(windowPos);
				m_quickChannelProperties.Show(GetDocument(), m_Cursor.GetChannel(), CPoint(windowPos.left + windowPos.Width() / 2, windowPos.top + windowPos.Height() / 2));
				return wParam;
			}
		case kcChannelTranspose: m_MenuCursor = m_Cursor; OnTransposeChannel(); return wParam;
		case kcChannelDuplicate: m_MenuCursor = m_Cursor; OnDuplicateChannel(); return wParam;
		case kcChannelAddBefore: m_MenuCursor = m_Cursor; OnAddChannelFront(); return wParam;
		case kcChannelAddAfter: m_MenuCursor = m_Cursor; OnAddChannelAfter(); return wParam;
		case kcChannelRemove: m_MenuCursor = m_Cursor; OnRemoveChannel(); return wParam;
		case kcChannelMoveLeft:
			if(CHANNELINDEX chn = m_Selection.GetStartChannel(); chn > 0)
				DragChannel(chn, chn - 1u, m_Selection.GetNumChannels(), false);
			return wParam;
		case kcChannelMoveRight:
			if (CHANNELINDEX chn = m_Selection.GetStartChannel(); chn < sndFile.GetNumChannels() - m_Selection.GetNumChannels())
				DragChannel(chn, chn + 1u, m_Selection.GetNumChannels(), false);
			return wParam;

		case kcSplitPattern: m_MenuCursor = m_Cursor; OnSplitPattern(); return wParam;

		case kcDecreaseSpacing:
			if(m_nSpacing > 0) SetSpacing(m_nSpacing - 1);
			return wParam;
		case kcIncreaseSpacing:
			if(m_nSpacing < MAX_SPACING) SetSpacing(m_nSpacing + 1);
			return wParam;

		case kcChordEditor:
			{
				CChordEditor dlg(this);
				dlg.DoModal();
				return wParam;
			}

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

		case kcCutPatternChannel:
			PatternClipboard::Copy(sndFile, GetCurrentPattern(), GetCurrentChannel());
			OnEditSelectChannel();
			OnClearSelection(false);
			return wParam;
		case kcCutPattern:
			PatternClipboard::Copy(sndFile, GetCurrentPattern());
			OnEditSelectAll();
			OnClearSelection(false);
			return wParam;
		case kcCopyPatternChannel:
			PatternClipboard::Copy(sndFile, GetCurrentPattern(), GetCurrentChannel());
			return wParam;
		case kcCopyPattern:
			PatternClipboard::Copy(sndFile, GetCurrentPattern());
			return wParam;
		case kcPastePatternChannel:
		case kcPastePattern:
			if(PatternClipboard::Paste(sndFile, GetCurrentPattern(), wParam == kcPastePatternChannel ? GetCurrentChannel() : CHANNELINDEX_INVALID))
			{
				SetModified();
				InvalidatePattern();
				GetDocument()->UpdateAllViews(this, PatternHint(GetCurrentPattern()).Data(), this);
			}
			return wParam;
		case kcTogglePatternPlayRow:
			TrackerSettings::Instance().m_dwPatternSetup ^= PATTERN_PLAYNAVIGATEROW;
			CMainFrame::GetMainFrame()->SetHelpText((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYNAVIGATEROW)
				? _T("Play whole row when navigatin was turned is now enabled.") : _T("Play whole row when navigatin was turned is now disabled."));
			return wParam;
	}

	// Ignore note entry if it is on key hold and user is in key-jazz mode or edit step is 0 (so repeated entry would be useless)
	const auto keyCombination = KeyCombination::FromLPARAM(lParam);
	const bool enterNote = keyCombination.EventType() != kKeyEventRepeat || (IsEditingEnabled() && m_nSpacing != 0);

	// Ranges:
	if(wParam >= kcVPStartNotes && wParam <= kcVPEndNotes)
	{
		if(enterNote)
			TempEnterNote(GetNoteWithBaseOctave(static_cast<int>(wParam - kcVPStartNotes)));
		return wParam;
	} else if(wParam >= kcVPStartChords && wParam <= kcVPEndChords)
	{
		if(enterNote)
			TempEnterChord(GetNoteWithBaseOctave(static_cast<int>(wParam - kcVPStartChords)));
		return wParam;
	}

	if(wParam >= kcVPStartNoteStops && wParam <= kcVPEndNoteStops)
	{
		TempStopNote(GetNoteWithBaseOctave(static_cast<int>(wParam - kcVPStartNoteStops)));
		return wParam;
	} else if(wParam >= kcVPStartChordStops && wParam <= kcVPEndChordStops)
	{
		TempStopChord(GetNoteWithBaseOctave(static_cast<int>(wParam - kcVPStartChordStops)));
		return wParam;
	}

	if(wParam >= kcSetSpacing0 && wParam <= kcSetSpacing9)
	{
		SetSpacing(static_cast<int>(wParam) - kcSetSpacing0);
		return wParam;
	}

	if(wParam >= kcSetIns0 && wParam <= kcSetIns9)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterIns(static_cast<int>(wParam) - kcSetIns0);
		return wParam;
	}

	if(wParam >= kcSetOctave0 && wParam <= kcSetOctave9)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterOctave(static_cast<int>(wParam) - kcSetOctave0);
		return wParam;
	}

	if(wParam >= kcSetOctaveStop0 && wParam <= kcSetOctaveStop9)
	{
		TempStopOctave(static_cast<int>(wParam) - kcSetOctaveStop0);
		return wParam;
	}

	if(wParam >= kcSetVolumeStart && wParam <= kcSetVolumeEnd)
	{
		if(IsEditingEnabled_bmsg())
			TempEnterVol(static_cast<int>(wParam) - kcSetVolumeStart);
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
			TempEnterFXparam(static_cast<int>(wParam) - kcSetFXParam0);
		return wParam;
	}

	return kcNull;
}


// Move pattern cursor to left or right, respecting invisible columns.
void CViewPattern::MoveCursor(bool moveRight)
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


static bool EnterPCNoteValue(int v, ModCommand &m, uint16 (ModCommand::*getMethod)() const, void (ModCommand::*setMethod)(uint16))
{
	if(v < 0 || v > 9)
		return false;

	uint16 val = (m.*getMethod)();
	// Move existing digits to left, drop out leftmost digit and push new digit to the least significant digit.
	val = static_cast<uint16>((val % 100) * 10 + v);
	LimitMax(val, static_cast<uint16>(ModCommand::maxColumnValue));
	(m.*setMethod)(val);
	return true;
}


// Enter volume effect / number in the pattern.
void CViewPattern::TempEnterVol(int v)
{
	CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
		return;

	PrepareUndo(m_Cursor, m_Cursor, "Volume Entry");

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target;  // This is the command we are about to overwrite
	const bool isDigit = (v >= 0) && (v <= 9);

	if(target.IsPcNote())
	{
		if(EnterPCNoteValue(v, target, &ModCommand::GetValueVolCol, &ModCommand::SetValueVolCol))
			m_PCNoteEditMemory = target;
	} else
	{
		ModCommand::VOLCMD volcmd = target.volcmd;
		uint16 vol = target.vol;
		if(isDigit)
		{
			vol = ((vol * 10) + v) % 100;
			if(!volcmd)
				volcmd = VOLCMD_VOLUME;
		} else
		{
			switch(v + kcSetVolumeStart)
			{
			case kcSetVolumeVol:          volcmd = VOLCMD_VOLUME; break;
			case kcSetVolumePan:          volcmd = VOLCMD_PANNING; break;
			case kcSetVolumeVolSlideUp:   volcmd = VOLCMD_VOLSLIDEUP; break;
			case kcSetVolumeVolSlideDown: volcmd = VOLCMD_VOLSLIDEDOWN; break;
			case kcSetVolumeFineVolUp:    volcmd = VOLCMD_FINEVOLUP; break;
			case kcSetVolumeFineVolDown:  volcmd = VOLCMD_FINEVOLDOWN; break;
			case kcSetVolumeVibratoSpd:   volcmd = VOLCMD_VIBRATOSPEED; break;
			case kcSetVolumeVibrato:      volcmd = VOLCMD_VIBRATODEPTH; break;
			case kcSetVolumeXMPanLeft:    volcmd = VOLCMD_PANSLIDELEFT; break;
			case kcSetVolumeXMPanRight:   volcmd = VOLCMD_PANSLIDERIGHT; break;
			case kcSetVolumePortamento:   volcmd = VOLCMD_TONEPORTAMENTO; break;
			case kcSetVolumeITPortaUp:    volcmd = VOLCMD_PORTAUP; break;
			case kcSetVolumeITPortaDown:  volcmd = VOLCMD_PORTADOWN; break;
			case kcSetVolumeITOffset:     volcmd = VOLCMD_OFFSET; break;
			}
			if(target.volcmd == VOLCMD_NONE && volcmd == m_cmdOld.volcmd)
			{
				vol = m_cmdOld.vol;
			}
		}

		uint16 max;
		switch(volcmd)
		{
		case VOLCMD_VOLUME:
		case VOLCMD_PANNING:
			max = 64;
			break;
		default:
			max = (pSndFile->GetType() == MOD_TYPE_XM) ? 0x0F : 9;
			break;
		}

		if(vol > max)
			vol %= 10;
		if(pSndFile->GetModSpecifications().HasVolCommand(volcmd))
		{
			m_cmdOld.volcmd = target.volcmd = volcmd;
			m_cmdOld.vol = target.vol = static_cast<ModCommand::VOL>(vol);
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
	if(!target.IsPcNote() && !isDigit && m_nSpacing > 0 && !IsLiveRecord() && TrackerSettings::Instance().patternStepCommands)
	{
		if(m_Cursor.GetRow() + m_nSpacing < pSndFile->Patterns[m_nPattern].GetNumRows() || (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL))
		{
			SetCurrentRow(m_Cursor.GetRow() + m_nSpacing, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) != 0);
		}
	}
}


void CViewPattern::SetSpacing(int n)
{
	if(static_cast<UINT>(n) != m_nSpacing)
	{
		m_nSpacing = static_cast<UINT>(n);
		PostCtrlMessage(CTRLMSG_SETSPACING, m_nSpacing);
	}
}


// Enter an effect letter in the pattern
void CViewPattern::TempEnterFX(ModCommand::COMMAND c, int v)
{
	CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target;  // This is the command we are about to overwrite

	PrepareUndo(m_Cursor, m_Cursor, "Effect Entry");

	if(target.IsPcNote())
	{
		if(EnterPCNoteValue(c, target, &ModCommand::GetValueEffectCol, &ModCommand::SetValueEffectCol))
			m_PCNoteEditMemory = target;
	} else if(pSndFile->GetModSpecifications().HasCommand(c))
	{
		if(c != CMD_NONE)
		{
			if((c == m_cmdOld.command) && (!target.param) && (target.command == CMD_NONE))
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
	if(!target.IsPcNote() && m_nSpacing > 0 && !IsLiveRecord() && TrackerSettings::Instance().patternStepCommands)
	{
		if(m_Cursor.GetRow() + m_nSpacing < pSndFile->Patterns[m_nPattern].GetNumRows() || (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL))
		{
			SetCurrentRow(m_Cursor.GetRow() + m_nSpacing, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) != 0);
		}
	}
}


// Enter an effect param in the pattenr
void CViewPattern::TempEnterFXparam(int v)
{
	CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target;  // This is the command we are about to overwrite

	PrepareUndo(m_Cursor, m_Cursor, "Parameter Entry");

	if(target.IsPcNote())
	{
		if(EnterPCNoteValue(v, target, &ModCommand::GetValueEffectCol, &ModCommand::SetValueEffectCol))
			m_PCNoteEditMemory = target;
	} else
	{

		target.param = static_cast<ModCommand::PARAM>((target.param << 4) | v);
		if(target.command == m_cmdOld.command)
		{
			m_cmdOld.param = target.param;
		}

		// Check for MOD/XM Speed/Tempo command
		if((pSndFile->GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM))
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
void CViewPattern::TempStopNote(ModCommand::NOTE note, const bool fromMidi, bool chordMode)
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pModDoc == nullptr || pMainFrm == nullptr || !ModCommand::IsNote(note))
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
	{
		return;
	}
	const CModSpecifications &specs = sndFile.GetModSpecifications();
	Limit(note, specs.noteMin, specs.noteMax);

	const bool liveRecord = IsLiveRecord();
	const bool isSplit = IsNoteSplit(note);
	UINT ins = 0;
	chordMode = chordMode && (m_prevChordNote != NOTE_NONE);

	auto &activeNoteMap = isSplit ? m_splitActiveNoteChannel : m_activeNoteChannel;
	const CHANNELINDEX nChnCursor = GetCurrentChannel();
	const CHANNELINDEX nChn = chordMode ? m_chordPatternChannels[0] : (activeNoteMap[note] < sndFile.GetNumChannels() ? activeNoteMap[note] : nChnCursor);

	CHANNELINDEX noteChannels[MPTChord::notesPerChord] = {nChn};
	ModCommand::NOTE notes[MPTChord::notesPerChord] = {note};
	int numNotes = 1;

	if(pModDoc)
	{
		if(isSplit)
		{
			ins = pModDoc->GetSplitKeyboardSettings().splitInstrument;
			if(pModDoc->GetSplitKeyboardSettings().octaveLink)
			{
				int trNote = note + 12 * pModDoc->GetSplitKeyboardSettings().octaveModifier;
				Limit(trNote, specs.noteMin, specs.noteMax);
				note = static_cast<ModCommand::NOTE>(trNote);
			}
		}
		if(!ins)
			ins = GetCurrentInstrument();
		if(!ins)
			ins = m_fallbackInstrument;

		if(chordMode)
		{
			m_Status.reset(psChordPlaying);

			numNotes = ConstructChord(note, notes, m_prevChordBaseNote);
			if(!numNotes)
			{
				return;
			}
			for(int i = 0; i < numNotes; i++)
			{
				pModDoc->NoteOff(notes[i], true, static_cast<INSTRUMENTINDEX>(ins), m_noteChannel[notes[i] - NOTE_MIN]);
				m_noteChannel[notes[i] - NOTE_MIN] = CHANNELINDEX_INVALID;
				m_baPlayingNote.reset(notes[i]);
				noteChannels[i] = m_chordPatternChannels[i];
			}
			m_prevChordNote = NOTE_NONE;
		} else
		{
			m_baPlayingNote.reset(note);
			pModDoc->NoteOff(note, ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_NOTEFADE) || sndFile.GetNumInstruments() == 0), static_cast<INSTRUMENTINDEX>(ins), m_noteChannel[note - NOTE_MIN]);
			m_noteChannel[note - NOTE_MIN] = CHANNELINDEX_INVALID;
		}
	}

	// Enter note off in pattern?
	if(!ModCommand::IsNote(note))
		return;
	if(m_Cursor.GetColumnType() > PatternCursor::instrColumn && (chordMode || !fromMidi))
		return;
	if(!pModDoc || !pMainFrm || !(IsEditingEnabled()))
		return;

	activeNoteMap[note] = NOTE_CHANNEL_MAP_INVALID;  //unlock channel

	if(!((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_KBDNOTEOFF) || fromMidi))
	{
		// We don't want to write the note-off into the pattern if this feature is disabled and we're not recording from MIDI.
		return;
	}

	// -- write sdx if playing live
	const bool usePlaybackPosition = (!chordMode) && (liveRecord && (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_AUTODELAY));

	//Work out where to put the note off
	PatternEditPos editPos = GetEditPos(sndFile, usePlaybackPosition);

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
			if(pTarget->note != NOTE_NONE || (!chordMode && (pTarget->instr || pTarget->volcmd)))
				return;
		} else
		{
			return;
		}
	}

	bool modified = false;
	for(int i = 0; i < numNotes; i++)
	{
		if(m_previousNote[noteChannels[i]] != notes[i])
		{
			// This might be a note-off from a past note, but since we already hit a new note on this channel, we ignore it.
			continue;
		}

		if(!modified)
		{
			pModDoc->GetPatternUndo().PrepareUndo(editPos.pattern, nChn, editPos.row, noteChannels[numNotes - 1] - nChn + 1, 1, "Note Stop Entry");
			modified = true;
		}

		pTarget = sndFile.Patterns[editPos.pattern].GetpModCommand(editPos.row, noteChannels[i]);

		// -- write sdx if playing live
		if(usePlaybackPosition && m_nPlayTick && pTarget->command == CMD_NONE && !doQuantize)
		{
			pTarget->command = CMD_S3MCMDEX;
			if(!specs.HasCommand(CMD_S3MCMDEX))
				pTarget->command = CMD_MODCMDEX;
			pTarget->param = static_cast<ModCommand::PARAM>(0xD0 | std::min(uint8(0xF), mpt::saturate_cast<uint8>(m_nPlayTick)));
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
			if(usePlaybackPosition && m_nPlayTick && !doQuantize)  // ECx
			{
				pTarget->command = CMD_S3MCMDEX;
				if(!specs.HasCommand(CMD_S3MCMDEX))
					pTarget->command = CMD_MODCMDEX;
				pTarget->param = static_cast<ModCommand::PARAM>(0xC0 | std::min(uint8(0xF), mpt::saturate_cast<uint8>(m_nPlayTick)));
			} else  // C00
			{
				pTarget->note = NOTE_NONE;
				pTarget->command = CMD_VOLUME;
				pTarget->param = 0;
			}
		}
		pTarget->instr = 0;  // Instrument numbers next to note-offs can do all kinds of weird things in XM files, and they are pointless anyway.
		pTarget->volcmd = VOLCMD_NONE;
		pTarget->vol = 0;
	}
	if(!modified)
		return;

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
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	const ModCommand &target = GetCursorCommand();
	if(target.IsNote())
	{
		int groupSize = GetDocument()->GetInstrumentGroupSize(target.instr);
		// The following might look a bit convoluted... This is mostly because the "middle-C" in
		// custom tunings always has octave 5, no matter how many octaves the tuning actually has.
		int note = mpt::wrapping_modulo(target.note - NOTE_MIDDLEC, groupSize) + (val - 5) * groupSize + NOTE_MIDDLEC;
		Limit(note, NOTE_MIN, NOTE_MAX);
		TempEnterNote(static_cast<ModCommand::NOTE>(note));
		// Memorize note for key-up
		ASSERT(size_t(val) < m_octaveKeyMemory.size());
		m_octaveKeyMemory[val] = target.note;
	}
}


// Stop note that has been triggered by entering an octave in the pattern.
void CViewPattern::TempStopOctave(int val)
{
	ASSERT(size_t(val) < m_octaveKeyMemory.size());
	if(m_octaveKeyMemory[val] != NOTE_NONE)
	{
		TempStopNote(m_octaveKeyMemory[val]);
		m_octaveKeyMemory[val] = NOTE_NONE;
	}
}


// Enter an instrument number in the pattern
void CViewPattern::TempEnterIns(int val)
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	PrepareUndo(m_Cursor, m_Cursor, "Instrument Entry");

	ModCommand &target = GetCursorCommand();
	ModCommand oldcmd = target;  // This is the command we are about to overwrite

	UINT instr = target.instr, nTotalMax, nTempMax;
	if(target.IsPcNote())  // this is a plugin index
	{
		nTotalMax = MAX_MIXPLUGINS + 1;
		nTempMax = MAX_MIXPLUGINS + 1;
	} else if(pSndFile->GetNumInstruments() > 0)  // this is an instrument index
	{
		nTotalMax = MAX_INSTRUMENTS;
		nTempMax = pSndFile->GetNumInstruments();
	} else
	{
		nTotalMax = MAX_SAMPLES;
		nTempMax = pSndFile->GetNumSamples();
	}

	instr = ((instr * 10) + val) % 1000;
	if(instr >= nTotalMax)
		instr = instr % 100;
	if(nTempMax < 100)        // if we're using samples & have less than 100 samples
		instr = instr % 100;  // or if we're using instruments and have less than 100 instruments
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
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if(pMainFrm == nullptr || pModDoc == nullptr)
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

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

	PatternEditPos editPos = GetEditPos(sndFile, usePlaybackPosition);

	const bool recordEnabled = IsEditingEnabled();
	CHANNELINDEX nChn = GetCurrentChannel();

	auto recordGroup = pModDoc->GetChannelRecordGroup(nChn);

	if(!isSpecial && pModDoc->GetSplitKeyboardSettings().IsSplitActive()
	   && ((recordGroup == RecordGroup::Group1 && isSplit) || (recordGroup == RecordGroup::Group2 && !isSplit)))
	{
		// Record group 1 should be used for normal notes, record group 2 for split notes.
		// If there are any channels assigned to the "other" record group, we switch to another channel.
		auto otherGroup = (recordGroup == RecordGroup::Group1) ? RecordGroup::Group2 : RecordGroup::Group1;
		const CHANNELINDEX newChannel = FindGroupRecordChannel(otherGroup, true);
		if(newChannel != CHANNELINDEX_INVALID)
		{
			// Found a free channel, switch to other record group.
			nChn = newChannel;
			recordGroup = otherGroup;
		}
	}

	// -- Chord autodetection: step back if we just entered a note
	if(recordEnabled && recordGroup != RecordGroup::NoGroup && !liveRecord && !ModCommand::IsPcNote(note) && m_nSpacing > 0)
	{
		const auto &order = Order();
		if((timeGetTime() - m_autoChordStartTime) < TrackerSettings::Instance().gnAutoChordWaitTime
		   && order.IsValidPat(m_autoChordStartOrder)
		   && sndFile.Patterns[order[m_autoChordStartOrder]].IsValidRow(m_autoChordStartRow))
		{
			const auto pattern = order[m_autoChordStartOrder];
			if(pattern != editPos.pattern)
			{
				SetCurrentOrder(m_autoChordStartOrder);
				SetCurrentPattern(pattern, m_autoChordStartRow);
			}
			editPos.pattern = pattern;
			editPos.row = m_autoChordStartRow;
		} else
		{
			m_autoChordStartRow = ROWINDEX_INVALID;
			m_autoChordStartOrder = ORDERINDEX_INVALID;
		}
		m_autoChordStartTime = timeGetTime();
		if(m_autoChordStartOrder == ORDERINDEX_INVALID || m_autoChordStartRow == ROWINDEX_INVALID)
		{
			m_autoChordStartOrder = editPos.order;
			m_autoChordStartRow = editPos.row;
		}
	}

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
			if((pTarget->command == CMD_MIDI || pTarget->command == CMD_SMOOTHMIDI) && pTarget->param < 128)
			{
				newcmd.SetValueEffectCol(static_cast<decltype(newcmd.GetValueEffectCol())>(Util::muldivr(pTarget->param, ModCommand::maxColumnValue, 127)));
				if(!newcmd.instr)
					newcmd.instr = sndFile.ChnSettings[nChn].nMixPlugin;
				auto activeMacro = sndFile.m_PlayState.Chn[nChn].nActiveMacro;
				if(!newcmd.GetValueVolCol() && sndFile.m_MidiCfg.GetParameteredMacroType(activeMacro) == kSFxPlugParam)
				{
					PlugParamIndex plugParam = sndFile.m_MidiCfg.MacroToPlugParam(sndFile.m_PlayState.Chn[nChn].nActiveMacro);
					if(plugParam < ModCommand::maxColumnValue)
						newcmd.SetValueVolCol(static_cast<decltype(newcmd.GetValueVolCol())>(plugParam));
				}
			}
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
		if(vol >= 0 && vol <= 64 && !(isSplit && pModDoc->GetSplitKeyboardSettings().splitVolume))  //write valid volume, as long as there's no split volume override.
		{
			volWrite = vol;
		} else if(isSplit && pModDoc->GetSplitKeyboardSettings().splitVolume)  //cater for split volume override.
		{
			if(pModDoc->GetSplitKeyboardSettings().splitVolume > 0 && pModDoc->GetSplitKeyboardSettings().splitVolume <= 64)
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
		if(usePlaybackPosition && m_nPlayTick && !doQuantize)  // avoid SD0 which will be mis-interpreted
		{
			if(newcmd.command == CMD_NONE)  //make sure we don't overwrite any existing commands.
			{
				newcmd.command = CMD_S3MCMDEX;
				if(!sndFile.GetModSpecifications().HasCommand(CMD_S3MCMDEX))
					newcmd.command = CMD_MODCMDEX;
				uint8 maxSpeed = 0x0F;
				if(m_nTicksOnRow > 0)
					maxSpeed = std::min(uint8(0x0F), mpt::saturate_cast<uint8>(m_nTicksOnRow - 1));
				newcmd.param = static_cast<ModCommand::PARAM>(0xD0 | std::min(maxSpeed, mpt::saturate_cast<uint8>(m_nPlayTick)));
			}
		}

		// Note cut/off/fade: erase instrument number
		if(newcmd.note >= NOTE_MIN_SPECIAL)
			newcmd.instr = 0;
	}

	// -- if recording, create undo point and write out modified command.
	const bool modified = (recordEnabled && *pTarget != newcmd);
	if(modified)
	{
		pModDoc->GetPatternUndo().PrepareUndo(editPos.pattern, nChn, editPos.row, 1, 1, "Note Entry");
		*pTarget = newcmd;
	}

	// -- play note
	if(((TrackerSettings::Instance().m_dwPatternSetup & (PATTERN_PLAYNEWNOTE | PATTERN_PLAYEDITROW)) || !recordEnabled) && !newcmd.IsPcNote())
	{
		const bool playWholeRow = ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !liveRecord);
		if(playWholeRow)
		{
			// play the whole row in "step mode"
			PatternStep(editPos.row);
			if(recordEnabled && newcmd.IsNote())
				m_noteChannel[newcmd.note - NOTE_MIN] = nChn;
		}
		if(!playWholeRow || !recordEnabled)
		{
			// NOTE: This code is *also* used for the PATTERN_PLAYEDITROW edit mode because of some unforseeable race conditions when modifying pattern data.
			// We have to use this code when editing is disabled or else we will get some stupid hazards, because we would first have to write the new note
			// data to the pattern and then remove it again - but often, it is actually removed before the row is parsed by the soundlib.

			// just play the newly inserted note using the already specified instrument...
			ModCommand::INSTR playIns = newcmd.instr;
			if(!playIns && ModCommand::IsNoteOrEmpty(note))
			{
				// ...or one that can be found on a previous row of this pattern.
				ModCommand *search = pTarget;
				ROWINDEX srow = editPos.row;
				while(srow-- > 0)
				{
					search -= sndFile.GetNumChannels();
					if(search->instr && !search->IsPcNote())
					{
						playIns = search->instr;
						m_fallbackInstrument = playIns;  //used to figure out which instrument to stop on key release.
						break;
					}
				}
			}
			PlayNote(newcmd.note, playIns, 4 * vol, nChn);
		}
	}

	if(newcmd.IsNote())
	{
		m_previousNote[nChn] = note;
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

		if(modified)  // Has it really changed?
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
			if(m_nSpacing > 0)
			{
				if(editPos.row + m_nSpacing < sndFile.Patterns[editPos.pattern].GetNumRows() || (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL))
				{
					SetCurrentRow(editPos.row + m_nSpacing, (TrackerSettings::Instance().m_dwPatternSetup & PATTERN_CONTSCROLL) != 0);
				}
			}

			SetSelToCursor();
		}

		if(newcmd.IsPcNote())
		{
			// Nothing to do here anymore.
			return;
		}

		auto &activeNoteMap = isSplit ? m_splitActiveNoteChannel : m_activeNoteChannel;
		if(newcmd.note <= NOTE_MAX)
			activeNoteMap[newcmd.note] = static_cast<decltype(m_activeNoteChannel)::value_type>(nChn);

		if(recordGroup != RecordGroup::NoGroup)
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


void CViewPattern::PlayNote(ModCommand::NOTE note, ModCommand::INSTR instr, int volume, CHANNELINDEX channel)
{
	CModDoc *modDoc = GetDocument();
	modDoc->PlayNote(PlayNoteParam(note).Instrument(instr).Volume(volume).Channel(channel).CheckNNA(m_baPlayingNote), &m_noteChannel);
}


void CViewPattern::PreviewNote(ROWINDEX row, CHANNELINDEX channel)
{
	const ModCommand &m = *GetSoundFile()->Patterns[m_nPattern].GetpModCommand(row, channel);
	if(m.IsNote() && m.instr)
	{
		int vol = -1;
		if(m.command == CMD_VOLUME)
			vol = m.param * 4u;
		else if(m.volcmd == VOLCMD_VOLUME)
			vol = m.vol * 4u;
		// Note-off any previews from this channel first
		ModCommand::NOTE note = NOTE_MIN;
		const auto &channels = GetSoundFile()->m_PlayState.Chn;
		for(auto &chn : m_noteChannel)
		{
			if(chn != CHANNELINDEX_INVALID && channels[chn].isPreviewNote && channels[chn].nMasterChn == channel + 1)
			{
				GetDocument()->NoteOff(note, false, m.instr, chn);
			}
			note++;
		}
		PlayNote(m.note, m.instr, vol, channel);
	}
}


// Construct a chord from the chord presets. Returns number of notes in chord.
int CViewPattern::ConstructChord(int note, ModCommand::NOTE (&outNotes)[MPTChord::notesPerChord], ModCommand::NOTE baseNote)
{
	const MPTChords &chords = TrackerSettings::GetChords();
	UINT chordNum = note - GetBaseNote();

	if(chordNum >= chords.size())
	{
		return 0;
	}
	const MPTChord &chord = chords[chordNum];

	const bool relativeMode = (chord.key == MPTChord::relativeMode);  // Notes are relative to a previously entered note in the pattern
	ModCommand::NOTE key;
	if(relativeMode)
	{
		// Relative mode: Use pattern note as base note.
		// If there is no valid note in the pattern: Use shortcut note as relative base note
		key = ModCommand::IsNote(baseNote) ? baseNote : static_cast<ModCommand::NOTE>(note);
	} else
	{
		// Default mode: Use base key
		key = GetNoteWithBaseOctave(chord.key);
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

	int32 baseKey = key - NOTE_MIN;
	if(!relativeMode)
	{
		// Only use octave information from the base key
		baseKey = (baseKey / 12) * 12;
	}

	for(auto cnote : chord.notes)
	{
		if(cnote != MPTChord::noNote)
		{
			int32 chordNote = baseKey + cnote + NOTE_MIN;
			if(chordNote >= NOTE_MIN && chordNote <= NOTE_MAX && specs.HasNote(static_cast<ModCommand::NOTE>(chordNote)))
			{
				outNotes[numNotes++] = static_cast<ModCommand::NOTE>(chordNote);
			}
		}
	}
	return numNotes;
}


// Enter a chord in the pattern
void CViewPattern::TempEnterChord(ModCommand::NOTE note)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	if(pMainFrm == nullptr || pModDoc == nullptr)
	{
		return;
	}
	CSoundFile &sndFile = pModDoc->GetSoundFile();
	if(!sndFile.Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	const CHANNELINDEX chn = GetCurrentChannel();
	auto rowBase = sndFile.Patterns[m_nPattern].GetRow(GetCurrentRow());

	ModCommand::NOTE chordNotes[MPTChord::notesPerChord], baseNote = rowBase[chn].note;
	if(!ModCommand::IsNote(baseNote))
	{
		baseNote = m_prevChordBaseNote;
	}
	int numNotes = ConstructChord(note, chordNotes, baseNote);
	if(!numNotes)
	{
		return;
	}

	// Save old row contents
	std::vector<ModCommand> newRow(rowBase.begin(), rowBase.end());

	const bool liveRecord = IsLiveRecord();
	const bool recordEnabled = IsEditingEnabled();
	bool modified = false;

	// -- establish note data
	HandleSplit(newRow[chn], note);
	const auto recordGroup = pModDoc->GetChannelRecordGroup(chn);

	CHANNELINDEX curChn = chn;
	for(int i = 0; i < numNotes; i++)
	{
		// Find appropriate channel
		while(curChn < sndFile.GetNumChannels() && pModDoc->GetChannelRecordGroup(curChn) != recordGroup)
		{
			curChn++;
		}
		if(curChn >= sndFile.GetNumChannels())
		{
			numNotes = i;
			break;
		}

		m_chordPatternChannels[i] = curChn;

		ModCommand &m = newRow[curChn];
		m_previousNote[curChn] = m.note = chordNotes[i];
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
		if(m_prevChordNote != NOTE_NONE)
		{
			TempStopChord(m_prevChordNote);
		}

		const bool playWholeRow = ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !liveRecord);
		if(playWholeRow)
		{
			// play the whole row in "step mode"
			PatternStep(GetCurrentRow());
			if(recordEnabled)
			{
				for(int i = 0; i < numNotes; i++)
				{
					m_noteChannel[chordNotes[i] - NOTE_MIN] = m_chordPatternChannels[i];
				}
			}
		}
		if(!playWholeRow || !recordEnabled)
		{
			// NOTE: This code is *also* used for the PATTERN_PLAYEDITROW edit mode because of some unforseeable race conditions when modifying pattern data.
			// We have to use this code when editing is disabled or else we will get some stupid hazards, because we would first have to write the new note
			// data to the pattern and then remove it again - but often, it is actually removed before the row is parsed by the soundlib.
			// just play the newly inserted notes...

			const ModCommand &firstNote = rowBase[chn];
			ModCommand::INSTR playIns = 0;
			if(firstNote.instr)
			{
				// ...using the already specified instrument
				playIns = firstNote.instr;
			} else if(!firstNote.instr)
			{
				// ...or one that can be found on a previous row of this pattern.
				const ModCommand *search = &firstNote;
				ROWINDEX srow = GetCurrentRow();
				while(srow-- > 0)
				{
					search -= sndFile.GetNumChannels();
					if(search->instr)
					{
						playIns = search->instr;
						m_fallbackInstrument = playIns;  //used to figure out which instrument to stop on key release.
						break;
					}
				}
			}
			for(int i = 0; i < numNotes; i++)
			{
				pModDoc->PlayNote(PlayNoteParam(chordNotes[i]).Instrument(playIns).Channel(chn).CheckNNA(m_baPlayingNote), &m_noteChannel);
			}
		}
	}  // end play note

	m_prevChordNote = note;
	m_prevChordBaseNote = baseNote;

	// Set new cursor position (edit step aka row spacing) - only when not recording live
	if(recordEnabled && !liveRecord)
	{
		if(m_nSpacing > 0)
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
{
	if(TrackerSettings::Instance().aftertouchBehaviour == atDoNotRecord || !IsEditingEnabled())
		return;

	const CHANNELINDEX numChannels = GetSoundFile()->GetNumChannels();
	std::set<CHANNELINDEX> channels;

	if(ModCommand::IsNote(note))
	{
		// For polyphonic aftertouch, map the aftertouch note to the correct pattern channel.
		const auto &activeNoteMap = IsNoteSplit(note) ? m_splitActiveNoteChannel : m_activeNoteChannel;
		if(activeNoteMap[note] < numChannels)
		{
			channels.insert(activeNoteMap[note]);
		} else
		{
			// Couldn't find the channel that belongs to this note... Don't bother writing aftertouch messages.
			// This is actually necessary, because it is possible that the last aftertouch message for a note
			// is received after the key-off event, in which case OpenMPT won't know anymore on which channel
			// that particular note was, so it will just put the message on some other channel. We don't want that!
			return;
		}
	} else
	{
		for(const auto &noteMap : { m_activeNoteChannel, m_splitActiveNoteChannel })
		{
			for(const auto chn : noteMap)
			{
				if(chn < numChannels)
					channels.insert(chn);
			}
		}
		if(channels.empty())
			channels.insert(m_Cursor.GetChannel());
	}

	Limit(atValue, 0, 127);

	const PatternCursor endOfRow{ m_Cursor.GetRow(), static_cast<CHANNELINDEX>(numChannels - 1u), PatternCursor::lastColumn };
	const auto &specs = GetSoundFile()->GetModSpecifications();
	bool first = true, modified = false;
	for(const auto chn : channels)
	{
		const PatternCursor cursor{ m_Cursor.GetRow(), chn };
		ModCommand &target = GetModCommand(cursor);
		ModCommand newCommand = target;

		if(target.IsPcNote())
			continue;

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
				auto cmd = 
					specs.HasCommand(CMD_SMOOTHMIDI) ? CMD_SMOOTHMIDI :
					specs.HasCommand(CMD_MIDI) ? CMD_MIDI :
					CMD_NONE;

				if(cmd != CMD_NONE)
				{
					newCommand.command = static_cast<ModCommand::COMMAND>(cmd);
					newCommand.param = static_cast<ModCommand::PARAM>(atValue);
				}
			}
			break;
		}

		if(target != newCommand)
		{
			if(first)
				PrepareUndo(cursor, endOfRow, "Aftertouch Entry");
			first = false;
			modified = true;
			target = newCommand;

			InvalidateCell(cursor);
		}
	}
	if(modified)
	{
		SetModified(false);
		UpdateIndicator();
	}
}


// Apply quantization factor to given row.
void CViewPattern::QuantizeRow(PATTERNINDEX &pat, ROWINDEX &row) const
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
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile != nullptr)
	{
		const auto &order = Order();
		const ORDERINDEX curOrder = GetCurrentOrder();
		if(curOrder > 0 && m_nPattern == order[curOrder])
		{
			const ORDERINDEX nextOrder = order.GetPreviousOrderIgnoringSkips(curOrder);
			const PATTERNINDEX nextPat = order[nextOrder];
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
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile != nullptr)
	{
		const auto &order = Order();
		const ORDERINDEX curOrder = GetCurrentOrder();
		if(curOrder + 1 < order.GetLength() && m_nPattern == order[curOrder])
		{
			const ORDERINDEX nextOrder = order.GetNextOrderIgnoringSkips(curOrder);
			const PATTERNINDEX nextPat = order[nextOrder];
			if(sndFile->Patterns.IsValidPat(nextPat) && sndFile->Patterns[nextPat].GetNumRows())
			{
				return nextPat;
			}
		}
	}
	return PATTERNINDEX_INVALID;
}


void CViewPattern::OnSetQuantize()
{
	CInputDlg dlg(this, _T("Quantize amount in rows for live recording (0 to disable):"), 0, MAX_PATTERN_ROWS, TrackerSettings::Instance().recordQuantizeRows);
	if(dlg.DoModal())
	{
		TrackerSettings::Instance().recordQuantizeRows = static_cast<ROWINDEX>(dlg.resultAsInt);
	}
}


void CViewPattern::OnLockPatternRows()
{
	CSoundFile &sndFile = *GetSoundFile();
	if(m_Selection.GetUpperLeft() != m_Selection.GetLowerRight())
	{
		sndFile.m_lockRowStart = m_Selection.GetStartRow();
		sndFile.m_lockRowEnd = m_Selection.GetEndRow();
	} else
	{
		sndFile.m_lockRowStart = sndFile.m_lockRowEnd = ROWINDEX_INVALID;
	}
	InvalidatePattern(true, true);
}


// Find a free channel for a record group, starting search from a given channel.
// If forceFreeChannel is true and all channels in the specified record group are active, some channel is picked from the specified record group.
CHANNELINDEX CViewPattern::FindGroupRecordChannel(RecordGroup recordGroup, bool forceFreeChannel, CHANNELINDEX startChannel) const
{
	const CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return CHANNELINDEX_INVALID;

	CHANNELINDEX chn = startChannel;
	CHANNELINDEX foundChannel = CHANNELINDEX_INVALID;

	for(CHANNELINDEX i = 1; i < pModDoc->GetNumChannels(); i++, chn++)
	{
		if(chn >= pModDoc->GetNumChannels())
			chn = 0;  // loop around

		if(pModDoc->GetChannelRecordGroup(chn) == recordGroup)
		{
			// Check if any notes are playing on this channel
			bool channelLocked = false;
			for(size_t k = 0; k < m_activeNoteChannel.size(); k++)
			{
				if(m_activeNoteChannel[k] == chn || m_splitActiveNoteChannel[k] == chn)
				{
					channelLocked = true;
					break;
				}
			}

			if(!channelLocked)
			{
				// Channel belongs to correct record group and no note is currently playing.
				return chn;
			}

			if(forceFreeChannel)
			{
				// If all channels are active, we might still pick a random channel from the specified group.
				foundChannel = chn;
			}
		}
	}
	return foundChannel;
}


void CViewPattern::OnClearField(const RowMask &mask, bool step, bool ITStyle)
{
	CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || !IsEditingEnabled_bmsg())
		return;

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

	if(step && (sndFile->IsPaused() || !m_Status[psFollowSong] ||
		(CMainFrame::GetMainFrame() != nullptr && CMainFrame::GetMainFrame()->GetFollowSong(GetDocument()) != m_hWnd)))
	{
		// Preview Row
		if((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !IsLiveRecord())
		{
			PatternStep(GetCurrentRow());
		}

		if(m_nSpacing > 0)
			SetCurrentRow(GetCurrentRow() + m_nSpacing);

		SetSelToCursor();
	}
}



void CViewPattern::OnInitMenu(CMenu *pMenu)
{
	CModScrollView::OnInitMenu(pMenu);
}

void CViewPattern::TogglePluginEditor(int chan)
{
	CModDoc *modDoc = GetDocument();
	if(!modDoc)
		return;

	int plug = modDoc->GetSoundFile().ChnSettings[chan].nMixPlugin;
	if(plug > 0)
		modDoc->TogglePluginEditor(plug - 1);

	return;
}


void CViewPattern::OnSelectInstrument(UINT nID)
{
	SetSelectionInstrument(static_cast<INSTRUMENTINDEX>(nID - ID_CHANGE_INSTRUMENT), true);
}


void CViewPattern::OnSelectPCNoteParam(UINT nID)
{
	CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern))
		return;

	uint16 paramNdx = static_cast<uint16>(nID - ID_CHANGE_PCNOTE_PARAM);
	bool modified = false;
	ApplyToSelection([paramNdx, &modified] (ModCommand &m, ROWINDEX, CHANNELINDEX)
	{
		if(m.IsPcNote() && (m.GetValueVolCol() != paramNdx))
		{
			m.SetValueVolCol(paramNdx);
			modified = true;
		}
	});
	if(modified)
	{
		SetModified();
		InvalidatePattern();
	}
}


void CViewPattern::OnSelectPlugin(UINT nID)
{
	CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr)
		return;

	const CHANNELINDEX plugChannel = m_MenuCursor.GetChannel();
	if(plugChannel < sndFile->GetNumChannels())
	{
		PLUGINDEX newPlug = static_cast<PLUGINDEX>(nID - ID_PLUGSELECT);
		if(newPlug <= MAX_MIXPLUGINS && newPlug != sndFile->ChnSettings[plugChannel].nMixPlugin)
		{
			sndFile->ChnSettings[plugChannel].nMixPlugin = newPlug;
			if(sndFile->GetModSpecifications().supportsPlugins)
			{
				SetModified(false);
			}
			InvalidateChannelsHeaders();
		}
	}
}


bool CViewPattern::HandleSplit(ModCommand &m, int note)
{
	ModCommand::INSTR ins = static_cast<ModCommand::INSTR>(GetCurrentInstrument());
	const bool isSplit = IsNoteSplit(note);

	if(isSplit)
	{
		CModDoc *modDoc = GetDocument();
		if(modDoc == nullptr)
			return false;
		const CSoundFile &sndFile = modDoc->GetSoundFile();

		if(modDoc->GetSplitKeyboardSettings().octaveLink && note <= NOTE_MAX)
		{
			note += 12 * modDoc->GetSplitKeyboardSettings().octaveModifier;
			Limit(note, sndFile.GetModSpecifications().noteMin, sndFile.GetModSpecifications().noteMax);
		}
		if(modDoc->GetSplitKeyboardSettings().splitInstrument)
		{
			ins = modDoc->GetSplitKeyboardSettings().splitInstrument;
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
{
	CModDoc *pModDoc = GetDocument();
	return (pModDoc != nullptr
	        && pModDoc->GetSplitKeyboardSettings().IsSplitActive()
	        && note <= pModDoc->GetSplitKeyboardSettings().splitNote);
}


bool CViewPattern::BuildPluginCtxMenu(HMENU hMenu, UINT nChn, const CSoundFile &sndFile) const
{
	for(PLUGINDEX plug = 0; plug <= MAX_MIXPLUGINS; plug++)
	{
		bool itemFound = false;

		CString s;
		if(!plug)
		{
			s = _T("No Plugin");
			itemFound = true;
		} else
		{
			const SNDMIXPLUGIN &plugin = sndFile.m_MixPlugins[plug - 1];
			if(plugin.IsValidPlugin())
			{
				s.Format(_T("FX%u: "), plug);
				s += mpt::ToCString(plugin.GetName());
				itemFound = true;
			}
		}

		if(itemFound)
		{
			UINT flags = MF_STRING | ((plug == sndFile.ChnSettings[nChn].nMixPlugin) ? MF_CHECKED : 0);
			AppendMenu(hMenu, flags, ID_PLUGSELECT + plug, s);
		}
	}
	return true;
}


bool CViewPattern::BuildSoloMuteCtxMenu(HMENU hMenu, CInputHandler *ih, UINT nChn, const CSoundFile &sndFile) const
{
	AppendMenu(hMenu, sndFile.ChnSettings[nChn].dwFlags[CHN_MUTE] ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_PATTERN_MUTE, ih->GetKeyTextFromCommand(kcChannelMute, _T("&Mute Channel")));
	bool solo = false, unmuteAll = false;
	bool soloPending = false, unmuteAllPending = false;  // doesn't work perfectly yet

	for(CHANNELINDEX i = 0; i < sndFile.GetNumChannels(); i++)
	{
		if(i != nChn)
		{
			if(!sndFile.ChnSettings[i].dwFlags[CHN_MUTE])
				solo = soloPending = true;
			if(sndFile.ChnSettings[i].dwFlags[CHN_MUTE] && sndFile.m_bChannelMuteTogglePending[i])
				soloPending = true;
		} else
		{
			if(sndFile.ChnSettings[i].dwFlags[CHN_MUTE])
				solo = soloPending = true;
			if(!sndFile.ChnSettings[i].dwFlags[CHN_MUTE] && sndFile.m_bChannelMuteTogglePending[i])
				soloPending = true;
		}
		if(sndFile.ChnSettings[i].dwFlags[CHN_MUTE])
			unmuteAll = unmuteAllPending = true;
		if(!sndFile.ChnSettings[i].dwFlags[CHN_MUTE] && sndFile.m_bChannelMuteTogglePending[i])
			unmuteAllPending = true;
	}
	if(solo)
		AppendMenu(hMenu, MF_STRING, ID_PATTERN_SOLO, ih->GetKeyTextFromCommand(kcChannelSolo, _T("&Solo Channel")));
	if(unmuteAll)
		AppendMenu(hMenu, MF_STRING, ID_PATTERN_UNMUTEALL, ih->GetKeyTextFromCommand(kcChannelUnmuteAll, _T("&Unmute All")));

	AppendMenu(hMenu, sndFile.m_bChannelMuteTogglePending[nChn] ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_PATTERN_TRANSITIONMUTE, ih->GetKeyTextFromCommand(kcToggleChanMuteOnPatTransition, sndFile.ChnSettings[nChn].dwFlags[CHN_MUTE] ? _T("On Transition: Unmute\t") : _T("On Transition: Mute\t")));

	if(unmuteAllPending)
		AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITION_UNMUTEALL, ih->GetKeyTextFromCommand(kcUnmuteAllChnOnPatTransition, _T("On Transition: Unmute All")));
	if(soloPending)
		AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITIONSOLO, ih->GetKeyTextFromCommand(kcSoloChnOnPatTransition, _T("On Transition: Solo")));

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_CHNRESET, ih->GetKeyTextFromCommand(kcChannelReset, _T("&Reset Channel")));

	return true;
}

bool CViewPattern::BuildRecordCtxMenu(HMENU hMenu, CInputHandler *ih, CHANNELINDEX nChn) const
{
	const auto recordGroup = GetDocument()->GetChannelRecordGroup(nChn);
	AppendMenu(hMenu, (recordGroup == RecordGroup::Group1) ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_EDIT_RECSELECT, ih->GetKeyTextFromCommand(kcChannelRecordSelect, _T("R&ecord Select")));
	AppendMenu(hMenu, (recordGroup == RecordGroup::Group2) ? (MF_STRING | MF_CHECKED) : MF_STRING, ID_EDIT_SPLITRECSELECT, ih->GetKeyTextFromCommand(kcChannelSplitRecordSelect, _T("S&plit Record Select")));
	return true;
}



bool CViewPattern::BuildRowInsDelCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	HMENU subMenuInsert = CreatePopupMenu();
	HMENU subMenuDelete = CreatePopupMenu();

	const auto numRows = m_Selection.GetNumRows();
	const CString label = (numRows != 1) ? MPT_CFORMAT("{} Rows")(numRows) : CString(_T("Row"));

	AppendMenu(subMenuInsert, MF_STRING, ID_PATTERN_INSERTROW, ih->GetKeyTextFromCommand(kcInsertRow, _T("Insert ") + label + _T(" (&Selection)")));
	AppendMenu(subMenuInsert, MF_STRING, ID_PATTERN_INSERTALLROW, ih->GetKeyTextFromCommand(kcInsertWholeRow, _T("Insert ") + label + _T(" (&All Channels)")));
	AppendMenu(subMenuInsert, MF_STRING, ID_PATTERN_INSERTROWGLOBAL, ih->GetKeyTextFromCommand(kcInsertRowGlobal, _T("Insert ") + label + _T(" (Selection, &Global)")));
	AppendMenu(subMenuInsert, MF_STRING, ID_PATTERN_INSERTALLROWGLOBAL, ih->GetKeyTextFromCommand(kcInsertWholeRowGlobal, _T("Insert ") + label + _T(" (All &Channels, Global)")));
	AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(subMenuInsert), _T("&Insert ") + label);

	AppendMenu(subMenuDelete, MF_STRING, ID_PATTERN_DELETEROW, ih->GetKeyTextFromCommand(kcDeleteRow, _T("Delete ") + label + _T(" (&Selection)")));
	AppendMenu(subMenuDelete, MF_STRING, ID_PATTERN_DELETEALLROW, ih->GetKeyTextFromCommand(kcDeleteWholeRow, _T("Delete ") + label + _T(" (&All Channels)")));
	AppendMenu(subMenuDelete, MF_STRING, ID_PATTERN_DELETEROWGLOBAL, ih->GetKeyTextFromCommand(kcDeleteRowGlobal, _T("Delete ") + label + _T(" (Selection, &Global)")));
	AppendMenu(subMenuDelete, MF_STRING, ID_PATTERN_DELETEALLROWGLOBAL, ih->GetKeyTextFromCommand(kcDeleteWholeRowGlobal, _T("Delete ") + label + _T(" (All &Channels, Global)")));
	AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(subMenuDelete), _T("&Delete ") + label);
	return true;
}

bool CViewPattern::BuildMiscCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	AppendMenu(hMenu, MF_STRING, ID_SHOWTIMEATROW, ih->GetKeyTextFromCommand(kcTimeAtRow, _T("Show Row Play Time")));

	if(m_Selection.GetStartRow() == m_Selection.GetEndRow())
	{
		CString s;
		s.Format(_T("Split Pattern at Ro&w %u"), m_Selection.GetStartRow());
		AppendMenu(hMenu, MF_STRING | (m_Selection.GetStartRow() < 1 ? MF_GRAYED : 0), ID_PATTERN_SPLIT, ih->GetKeyTextFromCommand(kcSplitPattern, s));
	}

	const CSoundFile &sndFile = *GetSoundFile();
	CString lockStr;
	bool lockActive = (sndFile.m_lockRowStart != ROWINDEX_INVALID);
	if(m_Selection.GetUpperLeft() != m_Selection.GetLowerRight())
	{
		lockStr = _T("&Lock Playback to Selection");
		if(lockActive)
		{
			lockStr.AppendFormat(_T(" (Current: %u-%u)"), sndFile.m_lockRowStart, sndFile.m_lockRowEnd);
		}
	} else if(lockActive)
	{
		lockStr = _T("Reset Playback &Lock");
	} else
	{
		return true;
	}
	AppendMenu(hMenu, MF_STRING | (lockActive ? MF_CHECKED : 0), ID_LOCK_PATTERN_ROWS, ih->GetKeyTextFromCommand(kcLockPlaybackToRows, lockStr));
	return true;
}

bool CViewPattern::BuildSelectionCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECTCOLUMN, ih->GetKeyTextFromCommand(kcSelectChannel, _T("Select &Channel")));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECT_ALL, ih->GetKeyTextFromCommand(kcEditSelectAll, _T("Select &Pattern")));
	return true;
}

bool CViewPattern::BuildGrowShrinkCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	AppendMenu(hMenu, MF_STRING, ID_GROW_SELECTION, ih->GetKeyTextFromCommand(kcPatternGrowSelection, _T("&Grow selection")));
	AppendMenu(hMenu, MF_STRING, ID_SHRINK_SELECTION, ih->GetKeyTextFromCommand(kcPatternShrinkSelection, _T("&Shrink selection")));
	return true;
}


bool CViewPattern::BuildInterpolationCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	const CSoundFile *sndFile = GetSoundFile();
	const bool isPCNote = sndFile->Patterns.IsValidPat(m_nPattern) && sndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel())->IsPcNote();

	HMENU subMenu = CreatePopupMenu();
	bool possible = BuildInterpolationCtxMenu(subMenu, PatternCursor::noteColumn, ih->GetKeyTextFromCommand(kcPatternInterpolateNote, _T("&Note Column")), ID_PATTERN_INTERPOLATE_NOTE)
	                | BuildInterpolationCtxMenu(subMenu, PatternCursor::instrColumn, ih->GetKeyTextFromCommand(kcPatternInterpolateInstr, isPCNote ? _T("&Plugin Column") : _T("&Instrument Column")), ID_PATTERN_INTERPOLATE_INSTR)
	                | BuildInterpolationCtxMenu(subMenu, PatternCursor::volumeColumn, ih->GetKeyTextFromCommand(kcPatternInterpolateVol, isPCNote ? _T("&Parameter Column") : _T("&Volume Column")), ID_PATTERN_INTERPOLATE_VOLUME)
	                | BuildInterpolationCtxMenu(subMenu, PatternCursor::effectColumn, ih->GetKeyTextFromCommand(kcPatternInterpolateEffect, isPCNote ? _T("&Value Column") : _T("&Effect Column")), ID_PATTERN_INTERPOLATE_EFFECT);
	if(possible || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_POPUP | (possible ? 0 : MF_GRAYED), reinterpret_cast<UINT_PTR>(subMenu), _T("I&nterpolate..."));
		return true;
	}
	return false;
}


bool CViewPattern::BuildInterpolationCtxMenu(HMENU hMenu, PatternCursor::Columns colType, CString label, UINT command) const
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


bool CViewPattern::BuildEditCtxMenu(HMENU hMenu, CInputHandler *ih, CModDoc *pModDoc) const
{
	HMENU pasteSpecialMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_EDIT_CUT, ih->GetKeyTextFromCommand(kcEditCut, _T("Cu&t")));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_COPY, ih->GetKeyTextFromCommand(kcEditCopy, _T("&Copy")));
	AppendMenu(hMenu, MF_STRING | (PatternClipboard::CanPaste() ? 0 : MF_GRAYED), ID_EDIT_PASTE, ih->GetKeyTextFromCommand(kcEditPaste, _T("&Paste")));
	AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(pasteSpecialMenu), _T("Paste Special"));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_MIXPASTE, ih->GetKeyTextFromCommand(kcEditMixPaste, _T("&Mix Paste")));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_MIXPASTE_ITSTYLE, ih->GetKeyTextFromCommand(kcEditMixPasteITStyle, _T("M&ix Paste (IT Style)")));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_PASTEFLOOD, ih->GetKeyTextFromCommand(kcEditPasteFlood, _T("Paste Fl&ood")));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_PUSHFORWARDPASTE, ih->GetKeyTextFromCommand(kcEditPushForwardPaste, _T("&Push Forward Paste (Insert)")));

	DWORD greyed = pModDoc->GetPatternUndo().CanUndo() ? MF_ENABLED : MF_GRAYED;
	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_EDIT_UNDO, ih->GetKeyTextFromCommand(kcEditUndo, _T("&Undo")));
	}
	greyed = pModDoc->GetPatternUndo().CanRedo() ? MF_ENABLED : MF_GRAYED;
	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_EDIT_REDO, ih->GetKeyTextFromCommand(kcEditRedo, _T("&Redo")));
	}

	AppendMenu(hMenu, MF_STRING, ID_CLEAR_SELECTION, ih->GetKeyTextFromCommand(kcSampleDelete, _T("Clear Selection")));

	return true;
}

bool CViewPattern::BuildVisFXCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	DWORD greyed = (IsColumnSelected(PatternCursor::effectColumn) || IsColumnSelected(PatternCursor::paramColumn)) ? FALSE : MF_GRAYED;

	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_VISUALIZE_EFFECT, ih->GetKeyTextFromCommand(kcPatternVisualizeEffect, _T("&Visualize Effect")));
		return true;
	}
	return false;
}

bool CViewPattern::BuildTransposeCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	HMENU transMenu = CreatePopupMenu();

	std::vector<CHANNELINDEX> validChans;
	DWORD greyed = IsColumnSelected(PatternCursor::noteColumn) ? FALSE : MF_GRAYED;

	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_UP, ih->GetKeyTextFromCommand(kcTransposeUp, _T("Transpose +&1")));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_DOWN, ih->GetKeyTextFromCommand(kcTransposeDown, _T("Transpose -&1")));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_OCTUP, ih->GetKeyTextFromCommand(kcTransposeOctUp, _T("Transpose +1&2")));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_OCTDOWN, ih->GetKeyTextFromCommand(kcTransposeOctDown, _T("Transpose -1&2")));
		AppendMenu(transMenu, MF_STRING | greyed, ID_TRANSPOSE_CUSTOM, ih->GetKeyTextFromCommand(kcTransposeCustom, _T("&Custom...")));
		AppendMenu(hMenu, MF_POPUP | greyed, reinterpret_cast<UINT_PTR>(transMenu), _T("&Transpose..."));
		return true;
	}
	return false;
}

bool CViewPattern::BuildAmplifyCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	std::vector<CHANNELINDEX> validChans;
	DWORD greyed = IsColumnSelected(PatternCursor::volumeColumn) ? 0 : MF_GRAYED;

	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_AMPLIFY, ih->GetKeyTextFromCommand(kcPatternAmplify, _T("&Amplify")));
		return true;
	}
	return false;
}


bool CViewPattern::BuildChannelControlCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	const CModSpecifications &specs = GetDocument()->GetSoundFile().GetModSpecifications();
	CHANNELINDEX numChannels = GetDocument()->GetNumChannels();
	DWORD canAddChannels = (numChannels < specs.channelsMax) ? 0 : MF_GRAYED;
	DWORD canRemoveChannels = (numChannels > specs.channelsMin) ? 0 : MF_GRAYED;

	AppendMenu(hMenu, MF_SEPARATOR, 0, _T(""));

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSPOSECHANNEL, ih->GetKeyTextFromCommand(kcChannelTranspose, _T("&Transpose Channel")));
	AppendMenu(hMenu, MF_STRING | canAddChannels, ID_PATTERN_DUPLICATECHANNEL, ih->GetKeyTextFromCommand(kcChannelDuplicate, _T("&Duplicate Channel")));

	HMENU addChannelMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP | canAddChannels, reinterpret_cast<UINT_PTR>(addChannelMenu), _T("&Add Channel\t"));
	AppendMenu(addChannelMenu, MF_STRING, ID_PATTERN_ADDCHANNEL_FRONT, ih->GetKeyTextFromCommand(kcChannelAddBefore, _T("&Before this channel")));
	AppendMenu(addChannelMenu, MF_STRING, ID_PATTERN_ADDCHANNEL_AFTER, ih->GetKeyTextFromCommand(kcChannelAddAfter, _T("&After this channel")));

	HMENU removeChannelMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP | canRemoveChannels, reinterpret_cast<UINT_PTR>(removeChannelMenu), _T("Remo&ve Channel\t"));
	AppendMenu(removeChannelMenu, MF_STRING, ID_PATTERN_REMOVECHANNEL, ih->GetKeyTextFromCommand(kcChannelRemove, _T("&Remove this channel\t")));
	AppendMenu(removeChannelMenu, MF_STRING, ID_PATTERN_REMOVECHANNELDIALOG, _T("&Choose channels to remove...\t"));

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_RESETCHANNELCOLORS, _T("Reset Channel &Colours"));

	return false;
}


bool CViewPattern::BuildSetInstCtxMenu(HMENU hMenu, CInputHandler *ih) const
{
	const CSoundFile *sndFile = GetSoundFile();
	const CModDoc *modDoc;
	if(sndFile == nullptr || (modDoc = sndFile->GetpModDoc()) == nullptr)
	{
		return false;
	}

	std::vector<CHANNELINDEX> validChans;
	DWORD greyed = IsColumnSelected(PatternCursor::instrColumn) ? 0 : MF_GRAYED;

	if(!greyed || !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
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
		AppendMenu(hMenu, MF_POPUP | greyed, reinterpret_cast<UINT_PTR>(instrumentChangeMenu), ih->GetKeyTextFromCommand(kcPatternSetInstrument, _T("Change Instrument")));

		if(!greyed)
		{
			bool addSeparator = false;
			if(sndFile->GetNumInstruments())
			{
				for(INSTRUMENTINDEX i = 1; i <= sndFile->GetNumInstruments(); i++)
				{
					if(sndFile->Instruments[i] == nullptr)
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
				CString s;
				for(SAMPLEINDEX i = 1; i <= sndFile->GetNumSamples(); i++) if (sndFile->GetSample(i).HasSampleData())
				{
					s.Format(_T("%02d: "), i);
					s += mpt::ToCString(sndFile->GetCharsetInternal(), sndFile->GetSampleName(i));
					AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + i, s);
					addSeparator = true;
				}
			}

			// Add options to remove instrument from selection.
			if(addSeparator)
			{
				AppendMenu(instrumentChangeMenu, MF_SEPARATOR, 0, 0);
			}
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT, _T("&Remove Instrument"));
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + GetCurrentInstrument(), ih->GetKeyTextFromCommand(kcPatternSetInstrument, _T("&Current Instrument")));
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_PATTERN_SETINSTRUMENT, ih->GetKeyTextFromCommand(kcPatternSetInstrumentNotEmpty, _T("Current Instrument (&only change existing)")));
		}
		return BuildTogglePlugEditorCtxMenu(hMenu, ih);
	}
	return false;
}


// Context menu for Param Control notes
bool CViewPattern::BuildPCNoteCtxMenu(HMENU hMenu, CInputHandler *ih) const
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

	CString s;

	// Create sub menu for "change plugin"
	HMENU pluginChangeMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(pluginChangeMenu), ih->GetKeyTextFromCommand(kcPatternSetInstrument, _T("Change Plugin")));
	for(PLUGINDEX nPlg = 0; nPlg < MAX_MIXPLUGINS; nPlg++)
	{
		if(sndFile->m_MixPlugins[nPlg].pMixPlugin != nullptr)
		{
			s = MPT_CFORMAT("{}: {}")(mpt::cfmt::dec0<2>(nPlg + 1), mpt::ToCString(sndFile->m_MixPlugins[nPlg].GetName()));
			AppendMenu(pluginChangeMenu, MF_STRING | (((nPlg + 1) == selStart.instr) ? MF_CHECKED : 0), ID_CHANGE_INSTRUMENT + nPlg + 1, s);
		}
	}

	if(selStart.instr >= 1 && selStart.instr <= MAX_MIXPLUGINS)
	{
		const SNDMIXPLUGIN &plug = sndFile->m_MixPlugins[selStart.instr - 1];
		if(plug.pMixPlugin != nullptr)
		{

			// Create sub menu for "change plugin param"
			HMENU paramChangeMenu = ::CreatePopupMenu();
			AppendMenu(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(paramChangeMenu), _T("Change Plugin Parameter\t"));

			const PlugParamIndex curParam = selStart.GetValueVolCol(), nParams = plug.pMixPlugin->GetNumParameters();

			for(PlugParamIndex i = 0; i < nParams; i++)
			{
				AppendMenu(paramChangeMenu, MF_STRING | ((i == curParam) ? MF_CHECKED : 0), ID_CHANGE_PCNOTE_PARAM + i, plug.pMixPlugin->GetFormattedParamName(i));
			}
		}
	}

	return BuildTogglePlugEditorCtxMenu(hMenu, ih);
}


bool CViewPattern::BuildTogglePlugEditorCtxMenu(HMENU hMenu, CInputHandler *ih) const
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
		AppendMenu(hMenu, MF_STRING, ID_PATTERN_EDIT_PCNOTE_PLUGIN, ih->GetKeyTextFromCommand(kcPatternEditPCNotePlugin, _T("Toggle Plugin &Editor")));
		return true;
	}
	return false;
}

// Returns an ordered list of all channels in which a given column type is selected.
CHANNELINDEX CViewPattern::ListChansWhereColSelected(PatternCursor::Columns colType, std::vector<CHANNELINDEX> &chans) const
{
	CHANNELINDEX startChan = m_Selection.GetStartChannel();
	CHANNELINDEX endChan = m_Selection.GetEndChannel();
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
{
	return m_Selection.ContainsHorizontal(PatternCursor(0, m_Selection.GetStartChannel(), colType))
	       || m_Selection.ContainsHorizontal(PatternCursor(0, m_Selection.GetEndChannel(), colType));
}


// Check if the given interpolation type is actually possible in the current selection.
bool CViewPattern::IsInterpolationPossible(PatternCursor::Columns colType) const
{
	std::vector<CHANNELINDEX> validChans;
	ListChansWhereColSelected(colType, validChans);

	ROWINDEX startRow = m_Selection.GetStartRow();
	ROWINDEX endRow = m_Selection.GetEndRow();
	for(auto chn : validChans)
	{
		if(IsInterpolationPossible(startRow, endRow, chn, colType))
		{
			return true;
		}
	}
	return false;
}


// Check if the given interpolation type is actually possible in a given channel.
bool CViewPattern::IsInterpolationPossible(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX chan, PatternCursor::Columns colType) const
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

	switch(colType)
	{
	case PatternCursor::noteColumn:
		startRowCmd = startRowMC.note;
		endRowCmd = endRowMC.note;
		result = (startRowCmd == endRowCmd && startRowCmd != NOTE_NONE)   // Interpolate between two identical notes or Cut / Fade / etc...
		         || (startRowCmd != NOTE_NONE && endRowCmd == NOTE_NONE)  // Fill in values from the first row
		         || (startRowCmd == NOTE_NONE && endRowCmd != NOTE_NONE)  // Fill in values from the last row
		         || (ModCommand::IsNoteOrEmpty(startRowMC.note) && ModCommand::IsNoteOrEmpty(endRowMC.note) && !(startRowCmd == NOTE_NONE && endRowCmd == NOTE_NONE));  // Interpolate between two notes of which one may be empty
		break;

	case PatternCursor::instrColumn:
		startRowCmd = startRowMC.instr;
		endRowCmd = endRowMC.instr;
		result = startRowCmd != 0 || endRowCmd != 0;
		break;

	case PatternCursor::volumeColumn:
		startRowCmd = startRowMC.volcmd;
		endRowCmd = endRowMC.volcmd;
		result = (startRowCmd == endRowCmd && startRowCmd != VOLCMD_NONE)      // Interpolate between two identical commands
		         || (startRowCmd != VOLCMD_NONE && endRowCmd == VOLCMD_NONE)   // Fill in values from the first row
		         || (startRowCmd == VOLCMD_NONE && endRowCmd != VOLCMD_NONE);  // Fill in values from the last row
		break;

	case PatternCursor::effectColumn:
	case PatternCursor::paramColumn:
		startRowCmd = startRowMC.command;
		endRowCmd = endRowMC.command;
		result = (startRowCmd == endRowCmd && startRowCmd != CMD_NONE)   // Interpolate between two identical commands
		         || (startRowCmd != CMD_NONE && endRowCmd == CMD_NONE)   // Fill in values from the first row
		         || (startRowCmd == CMD_NONE && endRowCmd != CMD_NONE);  // Fill in values from the last row
		break;

	default:
		result = false;
	}
	return result;
}


void CViewPattern::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	OnRButtonDown(nFlags, point);
	CModScrollView::OnRButtonDblClk(nFlags, point);
}


// Toggle pending mute status for channel from context menu.
void CViewPattern::OnTogglePendingMuteFromClick()
{
	TogglePendingMute(m_MenuCursor.GetChannel());
}


// Toggle pending solo status for channel from context menu.
void CViewPattern::OnPendingSoloChnFromClick()
{
	PendingSoloChn(m_MenuCursor.GetChannel());
}


// Set pending unmute status for all channels.
void CViewPattern::OnPendingUnmuteAllChnFromClick()
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
{
	if(IsEditingEnabled())
		return true;
	if(TrackerSettings::Instance().patternNoEditPopup)
		return false;

	HMENU hMenu;

	if((hMenu = ::CreatePopupMenu()) == nullptr)
		return false;

	CPoint pt = GetPointFromPosition(m_Cursor);

	// We add an mnemonic for an unbreakable space to avoid activating edit mode accidentally.
	AppendMenuW(hMenu, MF_STRING, IDC_PATTERN_RECORD, L"Editing (recording) is disabled;&\u00A0 click here to enable it.");

	ClientToScreen(&pt);
	::TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, NULL);

	::DestroyMenu(hMenu);

	return false;
}


// Show playback time at a given pattern position.
void CViewPattern::OnShowTimeAtRow()
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	CString msg;
	const auto &order = Order();
	ORDERINDEX currentOrder = GetCurrentOrder();
	if(currentOrder < order.size() && order[currentOrder] == m_nPattern)
	{
		const double t = pSndFile->GetPlaybackTimeAt(currentOrder, GetCurrentRow(), false, false);
		if(t < 0)
			msg.Format(_T("Unable to determine the time. Possible cause: No order %d, row %u found in play sequence."), currentOrder, GetCurrentRow());
		else
		{
			const uint32 minutes = static_cast<uint32>(t / 60.0);
			const double seconds = t - (minutes * 60);
			msg.Format(_T("Estimate for playback time at order %d (pattern %d), row %u: %u minute%s %.2f seconds."), currentOrder, m_nPattern, GetCurrentRow(), minutes, (minutes == 1) ? _T("") : _T("s"), seconds);
		}
	} else
	{
		msg.Format(_T("Unable to determine the time: pattern at current order (%d) does not correspond to pattern in pattern view (pattern %d)."), currentOrder, m_nPattern);
	}

	Reporting::Notification(msg);
}


// Set up split keyboard
void CViewPattern::SetSplitKeyboardSettings()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;

	CSplitKeyboardSettings dlg(CMainFrame::GetMainFrame(), pModDoc->GetSoundFile(), pModDoc->GetSplitKeyboardSettings());
	if(dlg.DoModal() == IDOK)
	{
		// Update split keyboard settings in other pattern views
		pModDoc->UpdateAllViews(NULL, SampleHint().Names());
	}
}


// Paste pattern data using the given paste mode.
void CViewPattern::ExecutePaste(PatternClipboard::PasteModes mode)
{
	if(IsEditingEnabled_bmsg() && PastePattern(m_nPattern, m_Selection.GetUpperLeft(), mode))
	{
		InvalidatePattern(false);
		SetFocus();
	}
}


// Show plugin editor for plugin assigned to PC Event at the cursor position.
void CViewPattern::OnTogglePCNotePluginEditor()
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr)
		return;
	CSoundFile &sndFile = pModDoc->GetSoundFile();
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

	if(plug > 0 && plug <= MAX_MIXPLUGINS)
		pModDoc->TogglePluginEditor(plug - 1);
}


// Get the active pattern's rows per beat, or, if they are not overriden, the song's default rows per beat.
ROWINDEX CViewPattern::GetRowsPerBeat() const
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
void CViewPattern::SetSelectionInstrument(const INSTRUMENTINDEX instr, bool setEmptyInstrument)
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	BeginWaitCursor();
	PrepareUndo(m_Selection, "Set Instrument");

	bool modified = false;
	ApplyToSelection([instr, setEmptyInstrument, &modified] (ModCommand &m, ROWINDEX, CHANNELINDEX)
	{
		// If a note or an instr is present on the row, do the change, if required.
		// Do not set instr if note and instr are both blank,
		// but set instr if note is a PC note and instr is blank.
		if(((setEmptyInstrument && (m.IsNote() || m.IsPcNote())) || m.instr != 0)
		   && (m.instr != instr))
		{
			m.instr = static_cast<ModCommand::INSTR>(instr);
			modified = true;
		}
	});

	if(modified)
	{
		SetModified();
		InvalidatePattern();
	}
	EndWaitCursor();
}


// Select a whole beat (selectBeat = true) or measure.
void CViewPattern::SelectBeatOrMeasure(bool selectBeat)
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
		endColumn = PatternCursor::lastColumn;  // Extend to param column
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
			endColumn = PatternCursor::lastColumn;  // Extend to param column
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
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr)
	{
		return;
	}
	const auto &order = Order();
	ORDERINDEX ord = GetCurrentOrder();
	PATTERNINDEX pat = m_nPattern;
	ROWINDEX row = m_Cursor.GetRow();

	while(sndFile->Patterns.IsValidPat(pat))
	{
		// Seek upwards
		do
		{
			auto &m = *sndFile->Patterns[pat].GetpModCommand(row, m_Cursor.GetChannel());
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
		ord = order.GetPreviousOrderIgnoringSkips(ord);
		pat = order[ord];
		if(!sndFile->Patterns.IsValidPat(pat))
		{
			return;
		}
		row = sndFile->Patterns[pat].GetNumRows() - 1;
	}
}


// Find previous or next column entry (note, instrument, ...) on this channel
void CViewPattern::JumpToPrevOrNextEntry(bool nextEntry, bool select)
{
	const CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || GetCurrentOrder() >= Order().size())
	{
		return;
	}
	const auto &order = Order();
	ORDERINDEX ord = GetCurrentOrder();
	PATTERNINDEX pat = m_nPattern;
	CHANNELINDEX chn = m_Cursor.GetChannel();
	PatternCursor::Columns column = m_Cursor.GetColumnType();
	int32 row = m_Cursor.GetRow();

	int direction = nextEntry ? 1 : -1;
	row += direction;  // Don't want to find the cell we're already in
	while(sndFile->Patterns.IsValidPat(pat))
	{
		while(sndFile->Patterns[pat].IsValidRow(row))
		{
			auto &m = *sndFile->Patterns[pat].GetpModCommand(row, chn);
			bool found;
			switch(column)
			{
			case PatternCursor::noteColumn:
				found = m.note != NOTE_NONE;
				break;
			case PatternCursor::instrColumn:
				found = m.instr != 0;
				break;
			case PatternCursor::volumeColumn:
				found = m.volcmd != VOLCMD_NONE;
				break;
			case PatternCursor::effectColumn:
			case PatternCursor::paramColumn:
				found = m.command != CMD_NONE;
				break;
			default:
				found = false;
			}

			if(found)
			{
				if(select)
				{
					CursorJump(static_cast<int>(row) - m_Cursor.GetRow(), false);
				} else
				{
					SetCurrentOrder(ord);
					SetCurrentPattern(pat, row);
					if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_PLAYNAVIGATEROW)
					{
						PatternStep(row);
					}
				}
				return;
			}
			row += direction;
		}

		// Continue search in prev/next pattern (unless we also select - selections cannot span multiple patterns)
		if(select)
			return;
		ORDERINDEX nextOrd = nextEntry ? order.GetNextOrderIgnoringSkips(ord) : order.GetPreviousOrderIgnoringSkips(ord);
		pat = order[nextOrd];
		if(nextOrd == ord || !sndFile->Patterns.IsValidPat(pat))
			return;
		ord = nextOrd;
		row = nextEntry ? 0 : (sndFile->Patterns[pat].GetNumRows() - 1);
	}
}


// Copy to clipboard
bool CViewPattern::CopyPattern(PATTERNINDEX nPattern, const PatternRect &selection)
{
	BeginWaitCursor();
	bool result = PatternClipboard::Copy(*GetSoundFile(), nPattern, selection);
	EndWaitCursor();
	PatternClipboardDialog::UpdateList();
	return result;
}


// Paste from clipboard
bool CViewPattern::PastePattern(PATTERNINDEX nPattern, const PatternCursor &pastePos, PatternClipboard::PasteModes mode)
{
	BeginWaitCursor();
	PatternEditPos pos;
	pos.pattern = nPattern;
	pos.row = pastePos.GetRow();
	pos.channel = pastePos.GetChannel();
	pos.order = GetCurrentOrder();
	PatternRect rect;
	const bool patternExisted = GetSoundFile()->Patterns.IsValidPat(nPattern);
	bool orderChanged = false;
	bool result = PatternClipboard::Paste(*GetSoundFile(), pos, mode, rect, orderChanged);
	EndWaitCursor();

	PatternHint updateHint = PatternHint(PATTERNINDEX_INVALID).Data();
	if(pos.pattern != nPattern)
	{
		// Multipaste: Switch to pasted pattern.
		SetCurrentPattern(pos.pattern);
		SetCurrentOrder(pos.order);
	}
	if(orderChanged || (patternExisted != GetSoundFile()->Patterns.IsValidPat(nPattern)))
	{
		updateHint.Names();
		GetDocument()->UpdateAllViews(nullptr, SequenceHint(GetSoundFile()->Order.GetCurrentSequenceIndex()).Data(), nullptr);
	}

	if(result)
	{
		SetCurSel(rect);
		GetDocument()->SetModified();
		GetDocument()->UpdateAllViews(nullptr, updateHint, nullptr);
	}

	return result;
}


template<typename Func>
void CViewPattern::ApplyToSelection(Func func)
{
	CSoundFile *sndFile = GetSoundFile();
	if(sndFile == nullptr || !sndFile->Patterns.IsValidPat(m_nPattern))
		return;
	auto &pattern = sndFile->Patterns[m_nPattern];
	m_Selection.Sanitize(pattern.GetNumRows(), pattern.GetNumChannels());
	const CHANNELINDEX startChn = m_Selection.GetStartChannel(), endChn = m_Selection.GetEndChannel();
	const ROWINDEX endRow = m_Selection.GetEndRow();
	for(ROWINDEX row = m_Selection.GetStartRow(); row <= endRow; row++)
	{
		ModCommand *m = pattern.GetpModCommand(row, startChn);
		for(CHANNELINDEX chn = startChn; chn <= endChn; chn++, m++)
		{
			func(*m, row, chn);
		}
	}
}


INT_PTR CViewPattern::OnToolHitTest(CPoint point, TOOLINFO *pTI) const
{
	CRect rect;
	const auto item = GetDragItem(point, rect);
	const auto value = item.Value();
	const CSoundFile &sndFile = *GetSoundFile();

	mpt::winstring text;
	switch(item.Type())
	{
	case DragItem::PatternHeader:
	{
		text = _T("Show Pattern Properties");
		auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(kcShowPatternProperties, 0);
		if(!keyText.IsEmpty())
			text += MPT_CFORMAT(" ({})")(keyText);
		break;
	}
	case DragItem::ChannelHeader:
		if(value < sndFile.GetNumChannels())
		{
			if(!sndFile.ChnSettings[value].szName.empty())
				text = MPT_TFORMAT("{}: {}")(value + 1, mpt::ToWin(sndFile.GetCharsetInternal(), sndFile.ChnSettings[value].szName));
			else
				text = MPT_TFORMAT("Channel {}")(value + 1);
		}
		break;
	case DragItem::PluginName:
		if(value < sndFile.GetNumChannels())
		{
			PLUGINDEX mixPlug = sndFile.ChnSettings[value].nMixPlugin;
			if(mixPlug && mixPlug <= MAX_MIXPLUGINS)
				text = MPT_TFORMAT("{}: {}")(mixPlug, mpt::ToWin(sndFile.m_MixPlugins[mixPlug - 1].GetName()));
			else
				text = _T("No Plugin");
		}
		break;
	}

	if(text.empty())
		return CScrollView::OnToolHitTest(point, pTI);

	pTI->hwnd = m_hWnd;
	pTI->uId = item.ToIntPtr();
	pTI->rect = rect;
	// MFC will free() the text
	TCHAR *textP = static_cast<TCHAR *>(calloc(text.size() + 1, sizeof(TCHAR)));
	std::copy(text.begin(), text.end(), textP);
	pTI->lpszText = textP;

	return item.ToIntPtr();
}


// Accessible description for screen readers
HRESULT CViewPattern::get_accName(VARIANT varChild, BSTR *pszName)
{
	const ModCommand &m = GetCursorCommand();
	const size_t columnIndex = m_Cursor.GetColumnType();
	const TCHAR *column = _T("");
	static constexpr const TCHAR *regularColumns[] = {_T("Note"), _T("Instrument"), _T("Volume"), _T("Effect"), _T("Parameter")};
	static constexpr const TCHAR *pcColumns[] = {_T("Note"), _T("Plugin"), _T("Plugin Parameter"), _T("Parameter Value"), _T("Parameter Value")};
	static_assert(PatternCursor::lastColumn + 1 == std::size(regularColumns));
	static_assert(PatternCursor::lastColumn + 1 == std::size(pcColumns));

	if(m.IsPcNote() && columnIndex < std::size(pcColumns))
		column = pcColumns[columnIndex];
	else if(!m.IsPcNote() && columnIndex < std::size(regularColumns))
		column = regularColumns[columnIndex];

	const CSoundFile *sndFile = GetSoundFile();
	const CHANNELINDEX chn = m_Cursor.GetChannel();

	const auto channelNumber = mpt::cfmt::val(chn + 1);
	CString channelName = channelNumber;
	if(chn < sndFile->GetNumChannels() && !sndFile->ChnSettings[chn].szName.empty())
		channelName += _T(": ") + mpt::ToCString(sndFile->GetCharsetInternal(), sndFile->ChnSettings[chn].szName);

	CString str = TrackerSettings::Instance().patternAccessibilityFormat;
	str.Replace(_T("%sequence%"), mpt::cfmt::val(sndFile->Order.GetCurrentSequenceIndex()));
	str.Replace(_T("%order%"), mpt::cfmt::val(GetCurrentOrder()));
	str.Replace(_T("%pattern%"), mpt::cfmt::val(GetCurrentPattern()));
	str.Replace(_T("%row%"), mpt::cfmt::val(m_Cursor.GetRow()));
	str.Replace(_T("%channel%"), channelNumber);
	str.Replace(_T("%column_type%"), column);
	str.Replace(_T("%column_description%"), GetCursorDescription());
	str.Replace(_T("%channel_name%"), channelName);

	if(str.IsEmpty())
		return CModScrollView::get_accName(varChild, pszName);

	*pszName = str.AllocSysString();
	return S_OK;
}

OPENMPT_NAMESPACE_END
