/*
 * view_pat.cpp
 * ------------
 * Purpose: Pattern tab, lower panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "PatternEditorDialogs.h"
#include "SampleEditorDialogs.h" // For amplification dialog (which is re-used from sample editor)
#include "globals.h"
#include "view_pat.h"
#include "ctrl_pat.h"
#include "vstplug.h"	// for writing plug params to pattern

#include "EffectVis.h"		//rewbs.fxvis
#include "PatternGotoDialog.h"
#include "PatternRandomizer.h"
#include "arrayutils.h"
#include "view_pat.h"
#include "View_gen.h"
#include "MIDIMacros.h"
#include "../common/misc_util.h"
#include "midi.h"
#include <cmath>

#define	PLUGNAME_HEIGHT	16	//rewbs.patPlugName

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

FindReplaceStruct CViewPattern::m_findReplace =
{
	ModCommand::Empty(), ModCommand::Empty(),
	PATSEARCH_FULLSEARCH, PATSEARCH_REPLACEALL,
	0, 0,
	0,
	PatternRect()
};

ModCommand CViewPattern::m_cmdOld = ModCommand::Empty();

IMPLEMENT_SERIAL(CViewPattern, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewPattern, CModScrollView)
	//{{AFX_MSG_MAP(CViewPattern)
	ON_WM_ERASEBKGND()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
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
	ON_MESSAGE(WM_MOD_RECORDPARAM,  OnRecordPlugParamChange)
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
// -> CODE#0012
// -> DESC="midi keyboard split"
	ON_COMMAND(ID_EDIT_SPLITRECSELECT,	OnSplitRecordSelect)
	ON_COMMAND(ID_EDIT_SPLITKEYBOARDSETTINGS,	SetSplitKeyboardSettings)
// -! NEW_FEATURE#0012
	ON_COMMAND(ID_EDIT_UNDO,		OnEditUndo)
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
	ON_COMMAND(ID_CONTROLENTER,		OnPatternStep)
	ON_COMMAND(ID_CONTROLTAB,		OnSwitchToOrderList)
	ON_COMMAND(ID_PREVORDER,		OnPrevOrder)
	ON_COMMAND(ID_NEXTORDER,		OnNextOrder)
	ON_COMMAND(IDC_PATTERN_RECORD,	OnPatternRecord)
	ON_COMMAND(ID_RUN_SCRIPT,					OnRunScript)
	ON_COMMAND(ID_TRANSPOSE_UP,					OnTransposeUp)
	ON_COMMAND(ID_TRANSPOSE_DOWN,				OnTransposeDown)
	ON_COMMAND(ID_TRANSPOSE_OCTUP,				OnTransposeOctUp)
	ON_COMMAND(ID_TRANSPOSE_OCTDOWN,			OnTransposeOctDown)
	ON_COMMAND(ID_PATTERN_PROPERTIES,			OnPatternProperties)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_VOLUME,	OnInterpolateVolume)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_EFFECT,	OnInterpolateEffect)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_NOTE,		OnInterpolateNote)
	ON_COMMAND(ID_PATTERN_VISUALIZE_EFFECT,		OnVisualizeEffect)		//rewbs.fxvis
	ON_COMMAND(ID_PATTERN_OPEN_RANDOMIZER,		OnOpenRandomizer)
	ON_COMMAND(ID_GROW_SELECTION,				OnGrowSelection)
	ON_COMMAND(ID_SHRINK_SELECTION,				OnShrinkSelection)
	ON_COMMAND(ID_PATTERN_SETINSTRUMENT,		OnSetSelInstrument)
	ON_COMMAND(ID_PATTERN_ADDCHANNEL_FRONT,		OnAddChannelFront)
	ON_COMMAND(ID_PATTERN_ADDCHANNEL_AFTER,		OnAddChannelAfter)
	ON_COMMAND(ID_PATTERN_DUPLICATECHANNEL,		OnDuplicateChannel)
	ON_COMMAND(ID_PATTERN_REMOVECHANNEL,		OnRemoveChannel)
	ON_COMMAND(ID_PATTERN_REMOVECHANNELDIALOG,	OnRemoveChannelDialog)
	ON_COMMAND(ID_CURSORCOPY,					OnCursorCopy)
	ON_COMMAND(ID_CURSORPASTE,					OnCursorPaste)
	ON_COMMAND(ID_PATTERN_AMPLIFY,				OnPatternAmplify)
	ON_COMMAND(ID_CLEAR_SELECTION,				OnClearSelectionFromMenu)
	ON_COMMAND(ID_SHOWTIMEATROW,				OnShowTimeAtRow)
	ON_COMMAND(ID_CHANNEL_RENAME,				OnRenameChannel)
	ON_COMMAND(ID_PATTERN_EDIT_PCNOTE_PLUGIN,	OnTogglePCNotePluginEditor)
	ON_COMMAND_RANGE(ID_CHANGE_INSTRUMENT, ID_CHANGE_INSTRUMENT+MAX_INSTRUMENTS, OnSelectInstrument)
	ON_COMMAND_RANGE(ID_CHANGE_PCNOTE_PARAM, ID_CHANGE_PCNOTE_PARAM + ModCommand::maxColumnValue, OnSelectPCNoteParam)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO,			OnUpdateUndo)
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT+MAX_MIXPLUGINS, OnSelectPlugin) //rewbs.patPlugName


	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

static_assert(ModCommand::maxColumnValue <= 999, "Command range for ID_CHANGE_PCNOTE_PARAM is designed for 999");

CViewPattern::CViewPattern()
//--------------------------
{
	m_pEffectVis = NULL; //rewbs.fxvis
	m_pRandomizer = NULL;
	m_bLastNoteEntryBlocked=false;

	m_nPattern = 0;
	m_nDetailLevel = PatternCursor::lastColumn;
	m_pEditWnd = NULL;
	m_pGotoWnd = NULL;
	m_Dib.Init(CMainFrame::bmpNotes);
	UpdateColors();
	m_PCNoteEditMemory = ModCommand::Empty();
}


void CViewPattern::OnInitialUpdate()
//----------------------------------
{
	MemsetZero(ChnVUMeters);
	MemsetZero(OldVUMeters);
// -> CODE#0012
// -> DESC="midi keyboard split"
	memset(splitActiveNoteChannel, 0xFF, sizeof(splitActiveNoteChannel));
	memset(activeNoteChannel, 0xFF, sizeof(activeNoteChannel));
	oldrow = -1;
	oldchn = -1;
	oldsplitchn = -1;
	m_nPlayPat = PATTERNINDEX_INVALID;
	m_nPlayRow = 0;
	m_nMidRow = 0;
	m_nDragItem = 0;
	m_bDragging = false;
	m_bInItemRect = false;
	m_bContinueSearch = false;
	//m_dwStatus = 0;
	m_dwStatus = psShowPluginNames;
	m_nXScroll = m_nYScroll = 0;
	m_nPattern = 0;
	m_nSpacing = 0;
	m_nAccelChar = 0;
	CScrollView::OnInitialUpdate();
	UpdateSizes();
	UpdateScrollSize();
	SetCurrentPattern(0);
	m_nFoundInstrument = 0;
	m_nLastPlayedRow = 0;
	m_nLastPlayedOrder = 0;
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

	if(m_bWholePatternFitsOnScreen) 		//rewbs.scrollFix
		SetScrollPos(SB_VERT, 0);
	else if(updateScroll) //rewbs.fix3147
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


bool CViewPattern::SetCurrentRow(ROWINDEX row, bool wrap, bool updateHorizontalScrollbar)
//---------------------------------------------------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
		return false;

	if(wrap && pSndFile->Patterns[m_nPattern].GetNumRows())
	{
		if ((int)row < 0)
		{
			if (m_dwStatus & (psKeyboardDragSelect|psMouseDragSelect))
			{
				row = 0;
			} else
			if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CONTSCROLL)
			{
				UINT nCurOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
				if ((nCurOrder > 0) && (nCurOrder < pSndFile->Order.size()) && (m_nPattern == pSndFile->Order[nCurOrder]))
				{
					const ORDERINDEX prevOrd = pSndFile->Order.GetPreviousOrderIgnoringSkips(nCurOrder);
					const PATTERNINDEX nPrevPat = pSndFile->Order[prevOrd];
					if ((nPrevPat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[nPrevPat].GetNumRows()))
					{
						SendCtrlMessage(CTRLMSG_SETCURRENTORDER, prevOrd);
						if (SetCurrentPattern(nPrevPat))
							return SetCurrentRow(pSndFile->Patterns[nPrevPat].GetNumRows() + (int)row);
					}
				}
				row = 0;
			} else
			if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_WRAP)
			{
				if ((int)row < (int)0) row += pSndFile->Patterns[m_nPattern].GetNumRows();
				row %= pSndFile->Patterns[m_nPattern].GetNumRows();
			}
		} else //row >= 0
		if (row >= pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			if (m_dwStatus & (psKeyboardDragSelect|psMouseDragSelect))
			{
				row = pSndFile->Patterns[m_nPattern].GetNumRows()-1;
			} else
			if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CONTSCROLL)
			{
				UINT nCurOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
				if ((nCurOrder+1 < pSndFile->Order.size()) && (m_nPattern == pSndFile->Order[nCurOrder]))
				{
					const ORDERINDEX nextOrder = pSndFile->Order.GetNextOrderIgnoringSkips(nCurOrder);
					const PATTERNINDEX nextPat = pSndFile->Order[nextOrder];
					if ((nextPat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[nextPat].GetNumRows()))
					{
						const ROWINDEX newRow = row - pSndFile->Patterns[m_nPattern].GetNumRows();
						SendCtrlMessage(CTRLMSG_SETCURRENTORDER, nextOrder);
						if (SetCurrentPattern(nextPat))
							return SetCurrentRow(newRow);
					}
				}
				row = pSndFile->Patterns[m_nPattern].GetNumRows()-1;
			} else
			if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_WRAP)
			{
				row %= pSndFile->Patterns[m_nPattern].GetNumRows();
			}
		}
	}

	//rewbs.fix3168
	if(static_cast<int>(row) < 0 && !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CONTSCROLL))
		row = 0;
	if(row >= pSndFile->Patterns[m_nPattern].GetNumRows() && !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CONTSCROLL))
		row = pSndFile->Patterns[m_nPattern].GetNumRows() - 1;
	//end rewbs.fix3168

	if ((row >= pSndFile->Patterns[m_nPattern].GetNumRows()) || (!m_szCell.cy)) return false;
	// Fix: If cursor isn't on screen move both scrollbars to make it visible
	InvalidateRow();
	m_Cursor.SetRow(row);
	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	UpdateScrollbarPositions(updateHorizontalScrollbar);
	InvalidateRow();

	PatternCursor selStart(m_Cursor);
	if((m_dwStatus & (psKeyboardDragSelect | psMouseDragSelect)) && (!(m_dwStatus & psDragnDropEdit)))
	{
		selStart.Set(m_StartSel);
	}
	SetCurSel(selStart, m_Cursor);
	UpdateIndicator();

	Log("Row: %d; Chan: %d; ColType: %d;\n",
		selStart.GetRow(),
		selStart.GetChannel(),
		selStart.GetColumnType());

	return true;
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

	if((m_dwStatus & (psKeyboardDragSelect | psMouseDragSelect)) && (!(m_dwStatus & psDragnDropEdit)))
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
		if(updateAllViews)
		{
			pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
		}
	}
}


// Fix: If cursor isn't on screen move scrollbars to make it visible
// Fix: save pattern scrollbar position when switching to other tab
// Assume that m_nRow and m_dwCursor are valid
// When we switching to other tab the CViewPattern object is deleted
// and when switching back new one is created
bool CViewPattern::UpdateScrollbarPositions(bool updateHorizontalScrollbar)
//---------------------------------------------------------------------------
{
// HACK - after new CViewPattern object created SetCurrentRow() and SetCurrentColumn() are called -
// just skip first two calls of UpdateScrollbarPositions() if pModDoc->GetOldPatternScrollbarsPos() is valid
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSize scroll = pModDoc->GetOldPatternScrollbarsPos();
		if( scroll.cx >= 0 )
		{
			OnScrollBy( scroll );
			scroll.cx = -1;
			pModDoc->SetOldPatternScrollbarsPos( scroll );
			return true;
		} else
		if( scroll.cx >= -1 )
		{
			scroll.cx = -2;
			pModDoc->SetOldPatternScrollbarsPos( scroll );
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

DWORD CViewPattern::GetDragItem(CPoint point, LPRECT lpRect)
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
	rect.SetRect(m_szHeader.cx, 0, m_szHeader.cx + GetColumnWidth() /*- 2*/, m_szHeader.cy);
	plugRect.SetRect(m_szHeader.cx, m_szHeader.cy-PLUGNAME_HEIGHT, m_szHeader.cx + GetColumnWidth() - 2, m_szHeader.cy);	//rewbs.patPlugNames

	const CHANNELINDEX nmax = pSndFile->GetNumChannels();
	// Checking channel headers
	//rewbs.patPlugNames
	if (m_dwStatus & psShowPluginNames)
	{
		n = xofs;
		for(UINT ihdr=0; n<nmax; ihdr++, n++)
		{
			if (plugRect.PtInRect(point))
			{
				if (lpRect) *lpRect = plugRect;
				return (DRAGITEM_PLUGNAME | n);
			}
			plugRect.OffsetRect(GetColumnWidth(), 0);
		}
	}
	//end rewbs.patPlugNames
	n = xofs;
	for (UINT ihdr=0; n<nmax; ihdr++, n++)
	{
		if (rect.PtInRect(point))
		{
			if (lpRect) *lpRect = rect;
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
			if (lpRect) *lpRect = rect;
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
	if(pSndFile == nullptr)
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


bool CViewPattern::SetPlayCursor(PATTERNINDEX nPat, ROWINDEX nRow)
//----------------------------------------------------------------
{
	PATTERNINDEX nOldPat = m_nPlayPat;
	ROWINDEX nOldRow = m_nPlayRow;

	if ((nPat == m_nPlayPat) && (nRow == m_nPlayRow))
		return true;
	
	m_nPlayPat = nPat;
	m_nPlayRow = nRow;
	if (nOldPat == m_nPattern)
		InvalidateRow(nOldRow);
	if (m_nPlayPat == m_nPattern)
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
		m_pEditWnd = new CEditCommand;
		if (m_pEditWnd) m_pEditWnd->SetParent(this, GetDocument());
	}
	if (m_pEditWnd)
	{
		m_pEditWnd->ShowEditWindow(m_nPattern, m_Cursor);
		return true;
	}
	return false;
}


bool CViewPattern::PrepareUndo(const PatternCursor &beginSel, const PatternCursor &endSel)
//----------------------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	const CHANNELINDEX nChnBeg = beginSel.GetChannel(), nChnEnd = endSel.GetChannel();
	const ROWINDEX nRowBeg = beginSel.GetRow(), nRowEnd = endSel.GetRow();

	if((nChnEnd < nChnBeg) || (nRowEnd < nRowBeg) || pModDoc == nullptr) return false;
	return pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, nChnBeg, nRowBeg, nChnEnd - nChnBeg + 1, nRowEnd-nRowBeg + 1);
}


BOOL CViewPattern::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if (pMsg)
	{
		//rewbs.customKeys
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) ||
			(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			CInputHandler *ih = (CMainFrame::GetMainFrame())->GetInputHandler();

			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxViewPatterns + 1 + m_Cursor.GetColumnType());

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
				}
				if (ctx==kCtxViewPatternsFXparam)
				{
					if (ih->KeyEvent(kCtxViewPatternsFX, nChar, nRepCnt, nFlags, kT) != kcNull)
					{
						return true; // Mapped to a command, no need to pass message on.
					}
				}
			}
			//end HACK.
		}
		//end rewbs.customKeys

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

	if (m_pRandomizer)
	{
		delete m_pRandomizer;
		m_pRandomizer=NULL;
	}

	CModScrollView::OnDestroy();
}


void CViewPattern::OnSetFocus(CWnd *pOldWnd)
//------------------------------------------
{
	CScrollView::OnSetFocus(pOldWnd);
	m_dwStatus |= psFocussed;
	InvalidateRow();
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		pModDoc->SetFollowWnd(m_hWnd, MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);
		UpdateIndicator();
	}
}


void CViewPattern::OnKillFocus(CWnd *pNewWnd)
//-------------------------------------------
{
	CScrollView::OnKillFocus(pNewWnd);

	//rewbs.customKeys
	//Unset all selection
	m_dwStatus &= ~psKeyboardDragSelect;
	m_dwStatus &= ~psCtrlDragSelect;
//	CMainFrame::GetMainFrame()->GetInputHandler()->SetModifierMask(0);
	//end rewbs.customKeys

	m_dwStatus &= ~psFocussed;
	InvalidateRow();
}

//rewbs.customKeys
void CViewPattern::OnGrowSelection()
//-----------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	BeginWaitCursor();

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	const PatternCursor startSel = m_Selection.GetUpperLeft();
	const PatternCursor endSel = m_Selection.GetLowerRight();
	PrepareUndo(startSel, PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows(), endSel));

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
	m_Selection = PatternRect(startSel, PatternCursor(min(finalDest, pSndFile->Patterns[m_nPattern].GetNumRows() - 1), endSel));

	InvalidatePattern(false);
	SetModified();
	EndWaitCursor();
	SetFocus();
}


void CViewPattern::OnShrinkSelection()
//-----------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	BeginWaitCursor();

	m_Selection.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	const PatternCursor startSel = m_Selection.GetUpperLeft();
	const PatternCursor endSel = m_Selection.GetLowerRight();
	PrepareUndo(startSel, endSel);

	const ROWINDEX finalDest = m_Selection.GetStartRow() + (m_Selection.GetNumRows() - 1) / 2;
	for(ROWINDEX row = startSel.GetRow(); row <= endSel.GetRow(); row++)
	{
		const ROWINDEX offset = row - startSel.GetRow();
		const ROWINDEX srcRow = startSel.GetRow() + (offset * 2);

		for(CHANNELINDEX chn = startSel.GetChannel(); chn <= startSel.GetChannel(); chn++)
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
	m_Selection = PatternRect(startSel, PatternCursor(min(finalDest, pSndFile->Patterns[m_nPattern].GetNumRows() - 1), endSel));

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

	PrepareUndo(m_Selection);

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
	m_nDropItem = m_nDragItem = GetDragItem(point, &m_rcDragItem);
	m_bDragging = true;
	m_bInItemRect = true;
	m_bShiftDragging = false;

	SetCapture();
	if ((point.x >= m_szHeader.cx) && (point.y <= m_szHeader.cy))
	{
		if (nFlags & MK_CONTROL)
		{
			TogglePendingMute(GetPositionFromPoint(point).GetChannel());
		}
	}
	else if ((point.x >= m_szHeader.cx) && (point.y > m_szHeader.cy))
	{
		if(CMainFrame::GetInputHandler()->ShiftPressed() && m_StartSel == m_Selection.GetLowerRight())
		{
			// Shift pressed -> set 2nd selection point
			DragToSel(GetPositionFromPoint(point), true, true);
		} else
		{
			// Set first selection point
			m_StartSel = GetPositionFromPoint(point);
			if(m_StartSel.GetChannel() < pSndFile->GetNumChannels())
			{
				m_dwStatus |= psMouseDragSelect;

				if (m_dwStatus & psCtrlDragSelect)
				{
					SetCurSel(m_StartSel);
				}
				if((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_DRAGNDROPEDIT)
					&& ((m_Selection.GetUpperLeft() != m_Selection.GetLowerRight()) || (m_dwStatus & psCtrlDragSelect))
					&& m_Selection.Contains(m_StartSel))
				{
					m_dwStatus |= psDragnDropEdit;
				} else if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CENTERROW)
				{
					SetCurSel(m_StartSel);
				} else
				{
					// Fix: Horizontal scrollbar pos screwed when selecting with mouse
					SetCursorPosition(m_StartSel);
				}
			}
		}
	} else if((point.x < m_szHeader.cx) && (point.y > m_szHeader.cy))
	{
		// Mark row number => mark whole row (start)
		InvalidateSelection();
		PatternCursor cursor(GetPositionFromPoint(point));
		if(cursor.GetRow() < pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			m_StartSel.Set(cursor);
			SetCurSel(cursor, PatternCursor(cursor.GetRow(), pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn));
			m_dwStatus |= psRowSelection;
		}
	}

	if (m_nDragItem)
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
		if((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_DBLCLICKSELECT))
		{
			OnSelectCurrentColumn();
			return;
		} else
		{
			if (ShowEditWindow()) return;
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

	m_bDragging = false;
	m_bInItemRect = false;
	ReleaseCapture();
	m_dwStatus &= ~(psMouseDragSelect|psRowSelection);
	// Drag & Drop Editing
	if (m_dwStatus & psDragnDropEdit)
	{
		if (m_dwStatus & psDragnDropping)
		{
			OnDrawDragSel();
			m_dwStatus &= ~psDragnDropping;
			OnDropSelection();
		}

		if(GetPositionFromPoint(point) == m_StartSel)
		{
			SetCurSel(m_StartSel);
		}
		SetCursor(CMainFrame::curArrow);
		m_dwStatus &= ~psDragnDropEdit;
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
	const CHANNELINDEX nItemNo = (m_nDragItem & DRAGITEM_VALUEMASK);
	const CHANNELINDEX nTargetNo = (m_nDropItem != 0) ? (m_nDropItem & DRAGITEM_VALUEMASK) : CHANNELINDEX_INVALID;

	switch(m_nDragItem & DRAGITEM_MASK)
	{
	case DRAGITEM_CHNHEADER:
		if(nItemNo == nTargetNo)
		{
			// Just clicked a channel header...

			if (nFlags & MK_SHIFT)
			{
				if (nItemNo < MAX_BASECHANNELS)
				{
					pModDoc->Record1Channel(nItemNo);
					InvalidateChannelsHeaders();
				}
			}
			else if (!(nFlags & MK_CONTROL))
			{
				pModDoc->MuteChannel(nItemNo, !pModDoc->IsChannelMuted(nItemNo));
				pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | ((nItemNo / CHANNELS_IN_TAB) << HINT_SHIFT_CHNTAB));
			}
		} else if(nTargetNo < pModDoc->GetNumChannels() && (m_nDropItem & DRAGITEM_MASK) == DRAGITEM_CHNHEADER)
		{
			// Dragged to other channel header => move or copy channel

			InvalidateRect(&m_rcDropItem, FALSE);

			const bool duplicate = (nFlags & MK_SHIFT) != 0;
			const CHANNELINDEX newChannels = pModDoc->GetNumChannels() + (duplicate ? 1 : 0);
			vector<CHANNELINDEX> channels(newChannels, 0);
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
					pModDoc->UpdateAllViews(this, HINT_MODCHANNELS | HINT_MODTYPE);
					SetCurrentPattern(m_nPattern);
				}
				InvalidatePattern();
				pModDoc->SetModified();
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
	if(pModDoc && pModDoc->GetSoundFile() && pModDoc->GetSoundFile()->Patterns.IsValidPat(m_nPattern))
	{
		CPatternPropertiesDlg dlg(pModDoc, m_nPattern, this);
		if (dlg.DoModal())
		{
			UpdateScrollSize();
			InvalidatePattern(true);
		}
	}
}


void CViewPattern::OnRButtonDown(UINT, CPoint pt)
//-----------------------------------------------
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
	if (m_dwStatus & psDragnDropEdit)
	{
		if (m_dwStatus & psDragnDropping)
		{
			OnDrawDragSel();
			m_dwStatus &= ~psDragnDropping;
		}
		m_dwStatus &= ~(psDragnDropEdit|psMouseDragSelect);
		if (m_bDragging)
		{
			m_bDragging = false;
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

	// Right-click outside selection? Reposition cursor to the new location
	if(!m_Selection.Contains(m_MenuCursor))
	{
		if (pt.y > m_szHeader.cy)
		{
			//ensure we're not clicking header

			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition(m_MenuCursor);
		}
	}
	const CHANNELINDEX nChn = m_MenuCursor.GetChannel();
	if ((nChn < pSndFile->GetNumChannels()) && (pSndFile->Patterns[m_nPattern]))
	{
		CString MenuText;
		CInputHandler *ih = (CMainFrame::GetMainFrame())->GetInputHandler();

		//------ Plugin Header Menu --------- :
		if ((m_dwStatus & psShowPluginNames) && 
			(pt.y > m_szHeader.cy-PLUGNAME_HEIGHT) && (pt.y < m_szHeader.cy))
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
					AppendMenu(hMenu, MF_SEPARATOR, 0, "");
				BuildRecordCtxMenu(hMenu, nChn, pModDoc);
				BuildChannelControlCtxMenu(hMenu);
				BuildChannelMiscCtxMenu(hMenu, pSndFile);
			}
		}
		
		//------ Standard Menu ---------- :
		else if ((pt.x >= m_szHeader.cx) && (pt.y >= m_szHeader.cy))
		{
			/*if (BuildSoloMuteCtxMenu(hMenu, ih, nChn, pSndFile))
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");*/
			if (BuildSelectionCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			if (BuildEditCtxMenu(hMenu, ih, pModDoc))
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			if (BuildNoteInterpolationCtxMenu(hMenu, ih, pSndFile) |	//Use bitwise ORs to avoid shortcuts
				BuildVolColInterpolationCtxMenu(hMenu, ih, pSndFile) | 
				BuildEffectInterpolationCtxMenu(hMenu, ih, pSndFile) )
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			if (BuildTransposeCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			if (BuildVisFXCtxMenu(hMenu, ih)   | 	//Use bitwise ORs to avoid shortcuts
				BuildAmplifyCtxMenu(hMenu, ih) |
				BuildSetInstCtxMenu(hMenu, ih, pSndFile) )
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			if (BuildPCNoteCtxMenu(hMenu, ih, pSndFile))
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			if (BuildGrowShrinkCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			if(BuildMiscCtxMenu(hMenu, ih))
				AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			BuildRowInsDelCtxMenu(hMenu, ih);
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

	m_nDragItem = GetDragItem(point, &m_rcDragItem);
	DWORD nItemNo = m_nDragItem & DRAGITEM_VALUEMASK;
	switch(m_nDragItem & DRAGITEM_MASK)
	{
	case DRAGITEM_CHNHEADER:
		if (nFlags & MK_SHIFT)
		{
			if (nItemNo < MAX_BASECHANNELS)
			{
				pModDoc->Record2Channel(nItemNo);
				InvalidateChannelsHeaders();
			}
		}
		break;
	}

	CModScrollView::OnRButtonUp(nFlags, point);
}


void CViewPattern::OnMouseMove(UINT nFlags, CPoint point)
//-------------------------------------------------------
{
	if (!m_bDragging) return;

	if (m_nDragItem)
	{
		const CRect oldDropRect = m_rcDropItem;
		const DWORD oldDropItem = m_nDropItem;

		m_bShiftDragging = (nFlags & MK_SHIFT) != 0;
		m_nDropItem = GetDragItem(point, &m_rcDropItem);

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

	if ((m_dwStatus & psRowSelection) /*&& (point.x < m_szHeader.cx)*/ && (point.y > m_szHeader.cy))
	{
		// Mark row number => mark whole row (continue)
		InvalidateSelection();

		PatternCursor cursor(GetPositionFromPoint(point));
		cursor.SetColumn(GetDocument()->GetNumChannels() - 1, PatternCursor::lastColumn);
		DragToSel(cursor, false, true, false);
	} else if (m_dwStatus & psMouseDragSelect)
	{
		PatternCursor cursor(GetPositionFromPoint(point));

		const CSoundFile *pSndFile = GetSoundFile();
		if(pSndFile != nullptr && m_nPattern < pSndFile->Patterns.Size())
		{
			ROWINDEX row = cursor.GetRow();
			ROWINDEX max = pSndFile->Patterns[m_nPattern].GetNumRows();
			if((row) && (row >= max)) row = max - 1;
			cursor.SetRow(row);
		}

		// Drag & Drop editing
		if (m_dwStatus & psDragnDropEdit)
		{
			const bool moved = m_DragPos.GetChannel() != cursor.GetChannel() || m_DragPos.GetRow() != cursor.GetRow();

			if(!(m_dwStatus & psDragnDropping))
			{
				SetCursor(CMainFrame::curDragging);
			}
			if(!(m_dwStatus & psDragnDropping) || moved)
			{
				if(m_dwStatus & psDragnDropping) OnDrawDragSel();
				m_dwStatus &= ~psDragnDropping;
				DragToSel(cursor, true, true, true);
				m_DragPos = cursor;
				m_dwStatus |= psDragnDropping;
				OnDrawDragSel();
			}
		} else
		// Default: selection
		if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CENTERROW)
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
	if(pSndFile != nullptr)
	{
		SetCurSel(PatternCursor(0), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn));
	}
}


void CViewPattern::OnEditSelectColumn()
//-------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr)
	{
		SetCurSel(PatternCursor(0, m_MenuCursor.GetChannel()), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, m_MenuCursor.GetChannel(), PatternCursor::lastColumn));
	}
}


void CViewPattern::OnSelectCurrentColumn()
//----------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile != nullptr)
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
	const CHANNELINDEX nChn = m_MenuCursor.GetChannel();
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	if(pModDoc == nullptr || (pSndFile = pModDoc->GetSoundFile()) == nullptr) return;

	const bool bIsMuted = pModDoc->IsChannelMuted(nChn);
	if(!bIsMuted) pModDoc->MuteChannel(nChn, true);
	pSndFile->ResetChannelState(nChn, CHNRESET_TOTAL);
	if(!bIsMuted) pModDoc->MuteChannel(nChn, false);
}


void CViewPattern::OnMuteFromClick()
//----------------------------------
{
	OnMuteChannel(false);
}

void CViewPattern::OnMuteChannel(bool current)
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		const CHANNELINDEX nChn = current ? GetCurrentChannel() : m_MenuCursor.GetChannel();
		pModDoc->SoloChannel(nChn, false); //rewbs.merge: recover old solo/mute behaviour
		pModDoc->MuteChannel(nChn, !pModDoc->IsChannelMuted(nChn));

		//If we just unmuted a channel, make sure none are still considered "solo".
		if(!pModDoc->IsChannelMuted(nChn))
		{
			for(CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)
			{
				pModDoc->SoloChannel(i, false);
			}
		}

		InvalidateChannelsHeaders();
	}
}

void CViewPattern::OnSoloFromClick()
{
	OnSoloChannel(false);
}


// When trying to solo a channel that is already the only unmuted channel,
// this will result in unmuting all channels, in order to satisfy user habits.
// In all other cases, soloing a channel unsoloes all and mutes all except this channel
void CViewPattern::OnSoloChannel(bool current)
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc == nullptr) return;

	const CHANNELINDEX nChn = current ? GetCurrentChannel() : m_MenuCursor.GetChannel();
	if (nChn >= pModDoc->GetNumChannels())
	{
		return;
	}

	if (pModDoc->IsChannelSolo(nChn))
	{
		bool nChnIsOnlyUnMutedChan=true;
		for (CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)	//check status of all other chans
		{
			if (i != nChn && !pModDoc->IsChannelMuted(i))
			{
				nChnIsOnlyUnMutedChan=false;	//found a channel that isn't muted!
				break;					
			}
		}
		if (nChnIsOnlyUnMutedChan)	// this is the only playable channel and it is already soloed ->  uunMute all
		{
			OnUnmuteAll();
			return;
		} 
	} 
	for(CHANNELINDEX i = 0; i < pModDoc->GetNumChannels(); i++)
	{
		pModDoc->MuteChannel(i, !(i == nChn)); //mute all chans except nChn, unmute nChn
		pModDoc->SoloChannel(i, (i == nChn));  //unsolo all chans except nChn, solo nChn
	}
	InvalidateChannelsHeaders();
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

// -> CODE#0012
// -> DESC="midi keyboard split"
void CViewPattern::OnSplitRecordSelect()
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
// -! NEW_FEATURE#0012


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
			pModDoc->SoloChannel(i, false); //rewbs.merge: binary solo/mute behaviour 
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

	PrepareUndo(PatternCursor(0), PatternCursor(maxrow - 1, pSndFile->GetNumChannels() - 1));

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
	//rewbs.customKeys
	PatternCursor finalPos(m_Selection.GetStartRow(), m_Selection.GetLowerRight());
	SetCurSel(finalPos);
	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	SetCursorPosition(finalPos);
	//end rewbs.customKeys
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

	PrepareUndo(PatternCursor(0), PatternCursor(maxrow - 1, pSndFile->GetNumChannels() - 1));

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


//rewbs.customKeys
void CViewPattern::OnInsertRows()
//-------------------------------
{
	InsertRows(m_Selection.GetStartChannel(), m_Selection.GetEndChannel());
}
//end rewbs.customKeys


void CViewPattern::OnEditFind()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CFindReplaceTab pageFind(IDD_EDIT_FIND, false, pModDoc);
		CFindReplaceTab pageReplace(IDD_EDIT_REPLACE, true, pModDoc);
		CPropertySheet dlg("Find/Replace");

		pageFind.m_nNote = m_findReplace.cmdFind.note;
		pageFind.m_nInstr = m_findReplace.cmdFind.instr;
		pageFind.m_nVolCmd = m_findReplace.cmdFind.volcmd;
		pageFind.m_nVol = m_findReplace.cmdFind.vol;
		pageFind.m_nCommand = m_findReplace.cmdFind.command;
		pageFind.m_nParam = m_findReplace.cmdFind.param;
		pageFind.m_dwFlags = m_findReplace.dwFindFlags;
		pageFind.m_nMinChannel = m_findReplace.nFindMinChn;
		pageFind.m_nMaxChannel = m_findReplace.nFindMaxChn;
		pageFind.m_bPatSel = (m_Selection.GetUpperLeft() != m_Selection.GetLowerRight());
		pageReplace.m_nNote = m_findReplace.cmdReplace.note;
		pageReplace.m_nInstr = m_findReplace.cmdReplace.instr;
		pageReplace.m_nVolCmd = m_findReplace.cmdReplace.volcmd;
		pageReplace.m_nVol = m_findReplace.cmdReplace.vol;
		pageReplace.m_nCommand = m_findReplace.cmdReplace.command;
		pageReplace.m_nParam = m_findReplace.cmdReplace.param;
		pageReplace.m_dwFlags = m_findReplace.dwReplaceFlags;
		pageReplace.cInstrRelChange = m_findReplace.cInstrRelChange;
		if(m_Selection.GetUpperLeft() != m_Selection.GetLowerRight())
		{
			pageFind.m_dwFlags |= PATSEARCH_PATSELECTION;
			pageFind.m_dwFlags &= ~PATSEARCH_FULLSEARCH;
		}
		dlg.AddPage(&pageFind);
		dlg.AddPage(&pageReplace);
		if (dlg.DoModal() == IDOK)
		{
			m_findReplace.cmdFind.note = pageFind.m_nNote;
			m_findReplace.cmdFind.instr = pageFind.m_nInstr;
			m_findReplace.cmdFind.volcmd = pageFind.m_nVolCmd;
			m_findReplace.cmdFind.vol = pageFind.m_nVol;
			m_findReplace.cmdFind.command = pageFind.m_nCommand;
			m_findReplace.cmdFind.param = pageFind.m_nParam;
			m_findReplace.nFindMinChn = pageFind.m_nMinChannel;
			m_findReplace.nFindMaxChn = pageFind.m_nMaxChannel;
			m_findReplace.dwFindFlags = pageFind.m_dwFlags;
			m_findReplace.cmdReplace.note = pageReplace.m_nNote;
			m_findReplace.cmdReplace.instr = pageReplace.m_nInstr;
			m_findReplace.cmdReplace.volcmd = pageReplace.m_nVolCmd;
			m_findReplace.cmdReplace.vol = pageReplace.m_nVol;
			m_findReplace.cmdReplace.command = pageReplace.m_nCommand;
			m_findReplace.cmdReplace.param = pageReplace.m_nParam;
			m_findReplace.dwReplaceFlags = pageReplace.m_dwFlags;
			m_findReplace.cInstrRelChange = pageReplace.cInstrRelChange;
			m_findReplace.selection = m_Selection;
			m_bContinueSearch = false;
			OnEditFindNext();
		}
	}
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
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		ORDERINDEX nCurrentOrder = static_cast<ORDERINDEX>(SendCtrlMessage(CTRLMSG_GETCURRENTORDER));
		CHANNELINDEX nCurrentChannel = GetCurrentChannel() + 1;

		m_pGotoWnd->UpdatePos(GetCurrentRow(), nCurrentChannel, m_nPattern, nCurrentOrder, pSndFile);

		if (m_pGotoWnd->DoModal() == IDOK)
		{
			//Position should be valididated.

			if (m_pGotoWnd->m_nPattern != m_nPattern)
				SetCurrentPattern(m_pGotoWnd->m_nPattern);

			if (m_pGotoWnd->m_nOrder != nCurrentOrder)
				SendCtrlMessage(CTRLMSG_SETCURRENTORDER,  m_pGotoWnd->m_nOrder);

			if (m_pGotoWnd->m_nChannel != nCurrentChannel)
				SetCurrentColumn(m_pGotoWnd->m_nChannel - 1);

			if (m_pGotoWnd->m_nRow != GetCurrentRow())
				SetCurrentRow(m_pGotoWnd->m_nRow);

			pModDoc->SetElapsedTime(m_pGotoWnd->m_nOrder, m_pGotoWnd->m_nRow);
		}
	}
	return;
}


void CViewPattern::OnEditFindNext()
//---------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();

	UINT nFound = 0;

	if (pSndFile == nullptr)
	{
		return;
	}

	if(!(m_findReplace.dwFindFlags & ~PATSEARCH_FULLSEARCH))
	{
		PostMessage(WM_COMMAND, ID_EDIT_FIND);
		return;
	}
	BeginWaitCursor();

	EffectInfo effectInfo(*pSndFile);

	PATTERNINDEX nPatStart = m_nPattern;
	PATTERNINDEX nPatEnd = m_nPattern + 1;

	if (m_findReplace.dwFindFlags & PATSEARCH_FULLSEARCH)
	{
		nPatStart = 0;
		nPatEnd = pSndFile->Patterns.Size();
	} else if(m_findReplace.dwFindFlags & PATSEARCH_PATSELECTION)
	{
		nPatStart = m_nPattern;
		nPatEnd = nPatStart + 1;
	}

	if (m_bContinueSearch)
	{
		nPatStart = m_nPattern;
	}

	// Do we search for an extended effect?
	bool isExtendedEffect = false;
	if (m_findReplace.dwFindFlags & PATSEARCH_COMMAND)
	{
		UINT fxndx = effectInfo.GetIndexFromEffect(m_findReplace.cmdFind.command, m_findReplace.cmdFind.param);
		isExtendedEffect = effectInfo.IsExtendedEffect(fxndx);
	}

	CHANNELINDEX firstChannel = 0;
	CHANNELINDEX lastChannel = pSndFile->GetNumChannels() - 1;

	if(m_findReplace.dwFindFlags & PATSEARCH_CHANNEL)
	{
		// Limit search to given channels
		firstChannel = min(m_findReplace.nFindMinChn, lastChannel);
		lastChannel = min(m_findReplace.nFindMaxChn, lastChannel);
	}

	if(m_findReplace.dwFindFlags & PATSEARCH_PATSELECTION)
	{
		// Limit search to pattern selection
		firstChannel = min(m_findReplace.selection.GetStartChannel(), lastChannel);
		lastChannel = min(m_findReplace.selection.GetEndChannel(), lastChannel);
	}

	for(PATTERNINDEX pat = nPatStart; pat < nPatEnd; pat++)
	{
		if(!pSndFile->Patterns.IsValidPat(pat))
		{
			continue;
		}

		ROWINDEX row = 0;
		CHANNELINDEX chn = firstChannel;
		if(m_bContinueSearch && pat == nPatStart && pat == m_nPattern)
		{
			// Continue search from cursor position
			row = GetCurrentRow();
			chn = GetCurrentChannel() + 1;
			if(chn > lastChannel)
			{
				row++;
				chn = firstChannel;
			} else if(chn < firstChannel)
			{
				chn = firstChannel;
			}
		}

		for(; row < pSndFile->Patterns[pat].GetNumRows(); row++)
		{
			ModCommand *m = pSndFile->Patterns[pat].GetpModCommand(row, chn);

			for(; chn <= lastChannel; chn++, m++)
			{
				RowMask findWhere;

				if(m_findReplace.dwFindFlags & PATSEARCH_PATSELECTION)
				{
					// Limit search to pattern selection
					if((chn == m_findReplace.selection.GetStartChannel() || chn == m_findReplace.selection.GetEndChannel())
						&& row >= m_findReplace.selection.GetStartRow() && row <= m_findReplace.selection.GetEndRow())
					{
						// For channels that are on the left and right boundaries of the selection, we need to check
						// columns are actually selected a bit more thoroughly.
						for(int i = PatternCursor::firstColumn; i <= PatternCursor::lastColumn; i++)
						{
							PatternCursor cursor(row, chn, static_cast<PatternCursor::Columns>(i));
							if(!m_findReplace.selection.Contains(cursor))
							{
								switch(i)
								{
								case PatternCursor::noteColumn:		findWhere.note = false; break;
								case PatternCursor::instrColumn:	findWhere.instrument = false; break;
								case PatternCursor::volumeColumn:	findWhere.volume = false; break;
								case PatternCursor::effectColumn:	findWhere.command = false; break;
								case PatternCursor::paramColumn:	findWhere.parameter = false; break;
								}
							}
						}
					} else
					{
						// For channels inside the selection, we have an easier job to solve.
						if(!m_findReplace.selection.Contains(PatternCursor(row, chn)))
						{
							findWhere.Clear();
						}
					}
				}

				bool foundHere = true;

				if(((m_findReplace.dwFindFlags & PATSEARCH_NOTE) && (!findWhere.note || (m->note != m_findReplace.cmdFind.note && (m_findReplace.cmdFind.note != CFindReplaceTab::findAny || !m->IsNote()))))
					|| ((m_findReplace.dwFindFlags & PATSEARCH_INSTR) && (!findWhere.instrument || m->instr != m_findReplace.cmdFind.instr))
					|| ((m_findReplace.dwFindFlags & PATSEARCH_VOLCMD) && (!findWhere.volume || m->volcmd != m_findReplace.cmdFind.volcmd))
					|| ((m_findReplace.dwFindFlags & PATSEARCH_VOLUME) && (!findWhere.volume || m->vol != m_findReplace.cmdFind.vol))
					|| ((m_findReplace.dwFindFlags & PATSEARCH_COMMAND) && (!findWhere.command || m->command != m_findReplace.cmdFind.command))
					|| ((m_findReplace.dwFindFlags & PATSEARCH_PARAM) && (!findWhere.parameter || m->param != m_findReplace.cmdFind.param)))
				{
					foundHere = false;
				} else
				{
					if((m_findReplace.dwFindFlags & (PATSEARCH_COMMAND | PATSEARCH_PARAM)) == PATSEARCH_COMMAND && isExtendedEffect)
					{
						if((m->param & 0xF0) != (m_findReplace.cmdFind.param & 0xF0)) foundHere = false;
					}

					// Ignore modcommands with PC/PCS notes when searching from volume or effect column.
					if(m->IsPcNote() && m_findReplace.dwFindFlags & (PATSEARCH_VOLCMD | PATSEARCH_VOLUME | PATSEARCH_COMMAND | PATSEARCH_PARAM))
					{
						foundHere = false;
					}
				}

				// Found!
				if(foundHere)
				{
					// Do we want to jump to the finding in this pattern?
					const bool updatePos = ((m_findReplace.dwReplaceFlags & (PATSEARCH_REPLACEALL | PATSEARCH_REPLACE)) != (PATSEARCH_REPLACEALL | PATSEARCH_REPLACE));
					nFound++;

					if (updatePos)
					{
						// turn off "follow song"
						m_dwStatus &= ~psFollowSong;
						SendCtrlMessage(CTRLMSG_PAT_FOLLOWSONG, 0);
						// go to place of finding
						SetCurrentPattern(pat);
					}

					PatternCursor::Columns foundCol = PatternCursor::firstColumn;
					if((m_findReplace.dwFindFlags & PATSEARCH_NOTE))
						foundCol = PatternCursor::noteColumn;
					else if((m_findReplace.dwFindFlags & PATSEARCH_INSTR))
						foundCol = PatternCursor::instrColumn;
					else if((m_findReplace.dwFindFlags & (PATSEARCH_VOLCMD | PATSEARCH_VOLUME)))
						foundCol = PatternCursor::volumeColumn;
					else if((m_findReplace.dwFindFlags & PATSEARCH_COMMAND))
						foundCol = PatternCursor::effectColumn;
					else if((m_findReplace.dwFindFlags & PATSEARCH_PARAM))
						foundCol = PatternCursor::paramColumn;

					if(updatePos)
					{
						// Jump to pattern cell
						SetCursorPosition(PatternCursor(row, chn, foundCol));
					}

					if(!(m_findReplace.dwReplaceFlags & PATSEARCH_REPLACE)) goto EndSearch;

					bool replace = true;

					if(!(m_findReplace.dwReplaceFlags & PATSEARCH_REPLACEALL))
					{
						ConfirmAnswer result = Reporting::Confirm("Replace this occurrence?", "Replace", true);
						if(result == cnfCancel)
						{
							goto EndSearch;	// Yuck!
						} else
						{
							replace = (result == cnfYes);
						}
					}
					if(replace)
					{
						// Just create one logic undo step when auto-replacing all occurences.
						const bool linkUndoBuffer = (nFound > 1) && (m_findReplace.dwReplaceFlags & PATSEARCH_REPLACEALL) != 0;
						GetDocument()->GetPatternUndo().PrepareUndo(pat, chn, row, 1, 1, linkUndoBuffer);

						if ((m_findReplace.dwReplaceFlags & PATSEARCH_NOTE))
						{
							if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNoteMinusOctave)
							{
								// -1 octave
								if (m->note > (12 + NOTE_MIN - 1) && m->note <= NOTE_MAX) m->note -= 12;
							} else if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNotePlusOctave)
							{
								// +1 octave
								if (m->note <= NOTE_MAX - 12 && m->note >= NOTE_MIN) m->note += 12;
							} else if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNoteMinusOne)
							{
								// Note--
								if (m->note > NOTE_MIN && m->note <= NOTE_MAX) m->note--;
							} else if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNotePlusOne)
							{
								// Note++
								if (m->note < NOTE_MAX && m->note >= NOTE_MIN) m->note++;
							} else
							{
								// Replace with another note
								// If we're going to remove a PC Note or replace a normal note by a PC note, wipe out the complete column.
								if(m->IsPcNote() != m_findReplace.cmdReplace.IsPcNote())
								{
									m->Clear();
								}
								m->note = m_findReplace.cmdReplace.note;
							}
						}

						if ((m_findReplace.dwReplaceFlags & PATSEARCH_INSTR))
						{
							if (m_findReplace.cInstrRelChange == -1)
							{
								// Instr--
								if(m->instr > 1)
									m->instr--;
							} else if (m_findReplace.cInstrRelChange == 1)
							{
								// Instr++
								if(m->instr > 0 && m->instr < (MAX_INSTRUMENTS - 1))
									m->instr++;
							} else
							{
								m->instr = m_findReplace.cmdReplace.instr;
							}
						}

						if ((m_findReplace.dwReplaceFlags & PATSEARCH_VOLCMD))
						{
							m->volcmd = m_findReplace.cmdReplace.volcmd;
						}

						if ((m_findReplace.dwReplaceFlags & PATSEARCH_VOLUME))
						{
							m->vol = m_findReplace.cmdReplace.vol;
						}

						if ((m_findReplace.dwReplaceFlags & (PATSEARCH_VOLCMD | PATSEARCH_VOLUME)) && m->volcmd != VOLCMD_NONE)
						{
							// Fix volume command parameters if necessary. This is necesary f.e.
							// When there was a command "v24" and the user searched for v and replaced it by d.
							// In that case, d24 wouldn't be a valid command.
							DWORD minVal = 0, maxVal = 64;
							if(effectInfo.GetVolCmdInfo(effectInfo.GetIndexFromVolCmd(m->volcmd), nullptr, &minVal, &maxVal))
							{
								Limit(m->vol, (ModCommand::VOL)minVal, (ModCommand::VOL)maxVal);
							}
						}

						if ((m_findReplace.dwReplaceFlags & PATSEARCH_COMMAND))
						{
							m->command = m_findReplace.cmdReplace.command;
						}

						if ((m_findReplace.dwReplaceFlags & PATSEARCH_PARAM))
						{
							if ((isExtendedEffect) && (!(m_findReplace.dwReplaceFlags & PATSEARCH_COMMAND)))
								m->param = (BYTE)((m->param & 0xF0) | (m_findReplace.cmdReplace.param & 0x0F));
							else
								m->param = m_findReplace.cmdReplace.param;
						}
						SetModified(false);
						if(updatePos)
							InvalidateRow();
					}
				}
			}
			chn = firstChannel;
		}

	}
EndSearch:

	if(m_findReplace.dwReplaceFlags & PATSEARCH_REPLACEALL)
	{
		InvalidatePattern();
	}

	if((m_findReplace.dwFindFlags & PATSEARCH_PATSELECTION) && (nFound == 0 || (m_findReplace.dwReplaceFlags & (PATSEARCH_REPLACE | PATSEARCH_REPLACEALL)) == PATSEARCH_REPLACE))
	{
		// Restore original selection if we didn't find anything or just replaced stuff manually.
		m_Selection = m_findReplace.selection;
		InvalidatePattern();
	}

	m_bContinueSearch = true;

	EndWaitCursor();

	// Display search results
	if(nFound == 0)
	{
		CString result;
		result.Preallocate(14 + 16);
		result = "Cannot find \"";

		// Note
		if(m_findReplace.dwFindFlags & PATSEARCH_NOTE)
		{
			result.Append(GetNoteStr(m_findReplace.cmdFind.note));
		} else
		{
			result.Append("???");
		}
		result.AppendChar(' ');

		// Instrument
		if(m_findReplace.dwFindFlags & PATSEARCH_INSTR)
		{
			if (m_findReplace.cmdFind.instr)
			{
				result.AppendFormat("%03d", m_findReplace.cmdFind.instr);
			} else
			{
				result.Append(" ..");
			}
		} else
		{
			result.Append(" ??");
		}
		result.AppendChar(' ');

		// Volume Command
		if(m_findReplace.dwFindFlags & PATSEARCH_VOLCMD)
		{
			if(m_findReplace.cmdFind.volcmd)
			{
				result.AppendChar(pSndFile->GetModSpecifications().GetVolEffectLetter(m_findReplace.cmdFind.volcmd));
			} else
			{
				result.AppendChar('.');
			}
		} else
		{
			result.AppendChar('?');
		}

		// Volume Parameter
		if(m_findReplace.dwFindFlags & PATSEARCH_VOLUME)
		{
			result.AppendFormat("%02d", m_findReplace.cmdFind.vol);
		} else
		{
			result.AppendFormat("??");
		}
		result.AppendChar(' ');

		// Effect Command
		if(m_findReplace.dwFindFlags & PATSEARCH_COMMAND)
		{
			if(m_findReplace.cmdFind.command)
			{
				result.AppendChar(pSndFile->GetModSpecifications().GetEffectLetter(m_findReplace.cmdFind.command));
			} else
			{
				result.AppendChar('.');
			}
		} else
		{
			result.AppendChar('?');
		}

		// Effect Parameter
		if(m_findReplace.dwFindFlags & PATSEARCH_PARAM)
		{
			result.AppendFormat("%02X", m_findReplace.cmdFind.param);
		} else
		{
			result.AppendFormat("??");
		}

		result.AppendChar('"');

		Reporting::Information(result, "Find/Replace");
	}
}


void CViewPattern::OnPatternStep()
//--------------------------------
{
	PatternStep(true);
}


void CViewPattern::PatternStep(bool autoStep)
//-------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if(pMainFrm != nullptr && pModDoc != nullptr)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
			return;

		CriticalSection cs;

		// Cut instruments/samples in virtual channels
		for (CHANNELINDEX i = pSndFile->GetNumChannels(); i < MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		pSndFile->LoopPattern(m_nPattern);
		pSndFile->m_nNextRow = GetCurrentRow();
		pSndFile->m_dwSongFlags &= ~SONG_PAUSED;
		pSndFile->m_dwSongFlags |= SONG_STEP;

		cs.Leave();

		if (pMainFrm->GetModPlaying() != pModDoc)
		{
			pMainFrm->PlayMod(pModDoc, m_hWnd, MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);
		}
		CMainFrame::EnableLowLatencyMode();
		if(autoStep)
		{
			if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CONTSCROLL)
				SetCurrentRow(GetCurrentRow() + 1, TRUE);
			else
				SetCurrentRow((GetCurrentRow() + 1) % pSndFile->Patterns[m_nPattern].GetNumRows(), FALSE);
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

	const ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), GetCurrentChannel());
	switch(m_Cursor.GetColumnType())
	{
	case PatternCursor::noteColumn:
	case PatternCursor::instrColumn:
		m_cmdOld.note = m->note;
		m_cmdOld.instr = m->instr;
		SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, m_cmdOld.instr);
		break;

	case PatternCursor::volumeColumn:
		m_cmdOld.volcmd = m->volcmd;
		m_cmdOld.vol = m->vol;
		break;

	case PatternCursor::effectColumn:
	case PatternCursor::paramColumn:
		m_cmdOld.command = m->command;
		m_cmdOld.param = m->param;
		break;
	}
}


// Paste cursor from internal clipboard
void CViewPattern::OnCursorPaste()
//--------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern) || !IsEditingEnabled_bmsg())
	{
		return;
	}

	PrepareUndo(m_Selection);
	CHANNELINDEX nChn = GetCurrentChannel();
	PatternCursor::Columns nCursor = m_Cursor.GetColumnType();
	ModCommand *p = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), nChn);

	switch(nCursor)
	{
	case PatternCursor::noteColumn:
		p->note = m_cmdOld.note;
		// Intentional fall-through
	case PatternCursor::instrColumn:
		p->instr = m_cmdOld.instr;
		break;

	case PatternCursor::volumeColumn:
		p->vol = m_cmdOld.vol;
		p->volcmd = m_cmdOld.volcmd;
		break;

	case PatternCursor::effectColumn:
	case PatternCursor::paramColumn:
		p->command = m_cmdOld.command;
		p->param = m_cmdOld.param;
		break;
	}

	SetModified(false);

	if(pSndFile->IsPaused() || !(m_dwStatus & psFollowSong) || (CMainFrame::GetMainFrame() && CMainFrame::GetMainFrame()->GetFollowSong(GetDocument()) != m_hWnd))
	{
		InvalidateCell(m_Cursor);
		SetCurrentRow(GetCurrentRow() + m_nSpacing);
		SetSelToCursor();
	}
}


void CViewPattern::OnInterpolateVolume()
//--------------------------------------
{
	Interpolate(PatternCursor::volumeColumn);
}


void CViewPattern::OnOpenRandomizer()
//-----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		if (m_pRandomizer)
		{
			if (m_pRandomizer->isGUIVisible())
			{
				//window already there, update data
				//m_pRandomizer->UpdateSelection(rowStart, rowEnd, nchn, pModDoc, m_nPattern);
			} else
			{
				m_pRandomizer->showGUI();
			}
		} else
		{
			//Open window & send data
			m_pRandomizer = new CPatternRandomizer(this);
			if (m_pRandomizer)
			{
				m_pRandomizer->showGUI();
			}
		}
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


void CViewPattern::OnInterpolateEffect()
//--------------------------------------
{
	Interpolate(PatternCursor::effectColumn);
}

void CViewPattern::OnInterpolateNote()
//--------------------------------------
{
	Interpolate(PatternCursor::noteColumn);
}


//static void CArrayUtils<UINT>::Merge(CArray<UINT,UINT>& Dest, CArray<UINT,UINT>& Src);

void CViewPattern::Interpolate(PatternCursor::Columns type)
//---------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	bool changed = false;
	CArray<UINT,UINT> validChans;

	if(type == PatternCursor::effectColumn || type == PatternCursor::paramColumn)
	{
		CArray<UINT,UINT> moreValidChans;
		ListChansWhereColSelected(PatternCursor::effectColumn, validChans);
		ListChansWhereColSelected(PatternCursor::paramColumn, moreValidChans);
		//CArrayUtils<UINT>::Merge(validChans, moreValidChans); //Causes unresolved external, not sure why yet.
		validChans.Append(moreValidChans);						//for now we'll just interpolate the same data several times. :)
	} else
	{
		ListChansWhereColSelected(type, validChans);
	}
	
	int nValidChans = validChans.GetCount();

	//for all channels where type is selected
	for (int chnIdx=0; chnIdx<nValidChans; chnIdx++)
	{
		UINT nchn = validChans[chnIdx];
		ROWINDEX row0 = m_Selection.GetStartRow();
		ROWINDEX row1 = m_Selection.GetEndRow();
		
		if (!IsInterpolationPossible(row0, row1, nchn, type, pSndFile))
			continue; //skip chans where interpolation isn't possible

		if (!changed) //ensure we save undo buffer only before any channels are interpolated
			PrepareUndo(m_Selection);

		bool doPCinterpolation = false;

		int vsrc, vdest, vcmd = 0, verr = 0, distance = row1 - row0;

		const ModCommand srcCmd = *pSndFile->Patterns[m_nPattern].GetpModCommand(row0, nchn);
		const ModCommand destCmd = *pSndFile->Patterns[m_nPattern].GetpModCommand(row1, nchn);

		ModCommand::NOTE PCnote = 0;
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

		ModCommand* pcmd = pSndFile->Patterns[m_nPattern].GetpModCommand(row0, nchn);

		for (int i = 0; i <= distance; i++, pcmd += pSndFile->GetNumChannels())
		{

			switch(type)
			{
				case PatternCursor::noteColumn:
					if ((pcmd->note == NOTE_NONE) || (pcmd->instr == vcmd))
					{
						int note = vsrc + ((vdest - vsrc) * i + verr) / distance;
						pcmd->note = (BYTE)note;
						pcmd->instr = vcmd;
					}
					break;

				case PatternCursor::volumeColumn:
					if ((pcmd->volcmd == VOLCMD_NONE) || (pcmd->volcmd == vcmd))
					{
						int vol = vsrc + ((vdest - vsrc) * i + verr) / distance;
						pcmd->vol = (BYTE)vol;
						pcmd->volcmd = vcmd;
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
							pcmd->instr = PCinst;
						}
						pcmd->SetValueVolCol(PCparam);
						pcmd->SetValueEffectCol(val);
					}
					else
					{
						if ((pcmd->command == CMD_NONE) || (pcmd->command == vcmd))
						{
							int val = vsrc + ((vdest - vsrc) * i + verr) / distance;
							pcmd->param = (BYTE)val;
							pcmd->command = vcmd;
						}
					}
					break;

				default:
					ASSERT(false);
			}
		}

		changed=true;

	} //end for all channels where type is selected

	if(changed)
	{
		SetModified(false);
		InvalidatePattern(false);
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

	PrepareUndo(m_Selection);

	for (UINT row=startRow; row <= endRow; row++)
	{
		ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(row, 0);
		for (UINT col = startChan; col <= endChan; col++)
		{
			if (m[col].IsNote())
			{
				int note = m[col].note;
				note += transp;
				if (note < noteMin) note = noteMin;
				if (note > noteMax) note = noteMax;
				m[col].note = (BYTE)note;
			}
		}
	}
	SetModified(false);
	InvalidateSelection();
	return true;
}


void CViewPattern::OnDropSelection()
//----------------------------------
{
	CModDoc *pModDoc;
	CSoundFile *pSndFile;

	if((pModDoc = GetDocument()) == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}
	pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
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
	ModCommand *pNewPattern = CPattern::AllocatePattern(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	if(pNewPattern == nullptr)
	{
		return;
	}
	// Compute destination rect
	PatternCursor begin(m_Selection.GetUpperLeft()), end(m_Selection.GetLowerRight());
	begin.Move(dy, dx, 0);
	if(begin.GetChannel() >= pSndFile->GetNumChannels())
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
	begin.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	end.Sanitize(pSndFile->Patterns[m_nPattern].GetNumRows(), pSndFile->GetNumChannels());
	PatternRect destination(begin, end);

	BeginWaitCursor();
	pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, 0, 0, pSndFile->GetNumChannels(), pSndFile->Patterns[m_nPattern].GetNumRows());

	ModCommand *p = pNewPattern;
	for(ROWINDEX row = 0; row < pSndFile->Patterns[m_nPattern].GetNumRows(); row++)
	{
		for(CHANNELINDEX chn = 0; chn < pSndFile->GetNumChannels(); chn++, p++)
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
					if(!(m_dwStatus & (psKeyboardDragSelect|psCtrlDragSelect)))
					{
						xsrc = -1;
					}
				}

				if(xsrc >= 0 && xsrc < (int)pSndFile->GetNumChannels() && ysrc >= 0 && ysrc < (int)pSndFile->Patterns[m_nPattern].GetNumRows())
				{
					// Copy the data
					const ModCommand &src = *pSndFile->Patterns[m_nPattern].GetpModCommand(ysrc, xsrc);
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
	CPattern::FreePattern(pSndFile->Patterns[m_nPattern]);
	pSndFile->Patterns[m_nPattern] = pNewPattern;
	cs.Leave();

	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	SetCursorPosition(begin);
	SetCurSel(destination);
	InvalidatePattern();
	pModDoc->SetModified();
	EndWaitCursor();
}


void CViewPattern::OnSetSelInstrument()
//-------------------------------------
{
	SetSelectionInstrument(GetCurrentInstrument());
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
		vector<bool> keepMask(pModDoc->GetNumChannels(), true);
		keepMask[nChn] = false;
		pModDoc->RemoveChannels(keepMask);
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
	vector<CHANNELINDEX> channels(pModDoc->GetNumChannels() + 1, CHANNELINDEX_INVALID);
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
		pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS); //refresh channel headers
		pModDoc->UpdateAllViews(NULL, HINT_MODTYPE); //updates(?) the channel number to general tab display
		SetCurrentPattern(m_nPattern);
	}
	EndWaitCursor();
}


void CViewPattern::OnDuplicateChannel()
//------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc == nullptr) return;

	if(Reporting::Confirm(GetStrI18N(_TEXT("This affects all patterns, proceed?"))) != cnfYes)
		return;

	const CHANNELINDEX nDupChn = m_MenuCursor.GetChannel();
	if(nDupChn >= pModDoc->GetNumChannels())
		return;

	BeginWaitCursor();
	// Create new channel order, with channel nDupChn duplicated.
	vector<CHANNELINDEX> channels(pModDoc->GetNumChannels() + 1, 0);
	CHANNELINDEX i = 0;
	for(CHANNELINDEX nChn = 0; nChn < pModDoc->GetNumChannels() + 1; nChn++)
	{
		channels[nChn] = i;
		if(nChn != nDupChn)
		{
			i++;
		}
	}

	// Check that duplication happened and in that case update.
	if(pModDoc->ReArrangeChannels(channels) != CHANNELINDEX_INVALID)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS);
		pModDoc->UpdateAllViews(NULL, HINT_MODTYPE);
		SetCurrentPattern(m_nPattern);
	}
	EndWaitCursor();
}

void CViewPattern::OnRunScript()
//------------------------------
{
	;
}


void CViewPattern::OnTransposeUp()
//--------------------------------
{
	TransposeSelection(1);
}


void CViewPattern::OnTransposeDown()
//----------------------------------
{
	TransposeSelection(-1);
}


void CViewPattern::OnTransposeOctUp()
//-----------------------------------
{
	TransposeSelection(12);
}


void CViewPattern::OnTransposeOctDown()
//-------------------------------------
{
	TransposeSelection(-12);
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
	}
}


void CViewPattern::OnEditUndo()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc && IsEditingEnabled_bmsg())
	{
		PATTERNINDEX nPat = pModDoc->GetPatternUndo().Undo();
		if (nPat < pModDoc->GetSoundFile()->Patterns.Size())
		{
			pModDoc->SetModified();
			if (nPat != m_nPattern)
				SetCurrentPattern(nPat);
			else
				InvalidatePattern(true);
		}
	}
}


void CViewPattern::OnPatternAmplify()
//-----------------------------------
{
	static int16 snOldAmp = 100;
	CAmpDlg dlg(this, snOldAmp, 0);
	if(dlg.DoModal() != IDOK)
	{
		return;
	}

	CSoundFile *pSndFile = GetSoundFile();

	const bool useVolCol = (pSndFile->GetType() != MOD_TYPE_MOD);

	BeginWaitCursor();
	PrepareUndo(m_Selection);
	snOldAmp = dlg.m_nFactor;

	if(pSndFile->Patterns[m_nPattern])
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
			if(lastChannel == 0)
			{
				// Selection too small!
				EndWaitCursor();
				return;
			}
			lastChannel--;
		}

		// Volume memory for each channel.
		vector<BYTE> chvol(lastChannel + 1, 64);

		for (ROWINDEX nRow = firstRow; nRow <= lastRow; nRow++)
		{
			ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(nRow, firstChannel);
			for (CHANNELINDEX nChn = firstChannel; nChn <= lastChannel; nChn++, m++)
			{
				if ((m->command == CMD_VOLUME) && (m->param <= 64))
				{
					chvol[nChn] = m->param;
					break;
				}
				if (m->volcmd == VOLCMD_VOLUME)
				{
					chvol[nChn] = m->vol;
					break;
				}
				if ((m->note) && (m->note <= NOTE_MAX) && (m->instr))
				{
					UINT nSmp = m->instr;
					if (pSndFile->GetNumInstruments())
					{
						if ((nSmp <= pSndFile->GetNumInstruments()) && (pSndFile->Instruments[nSmp]))
						{
							nSmp = pSndFile->Instruments[nSmp]->Keyboard[m->note];
							if(!nSmp) chvol[nChn] = 64;	// hack for instruments without samples
						} else
						{
							nSmp = 0;
						}
					}
					if ((nSmp) && (nSmp <= pSndFile->GetNumSamples()))
					{
						chvol[nChn] = (BYTE)(pSndFile->GetSample(nSmp).nVolume / 4);
						break;
					}
					else
					{	//nonexistant sample and no volume present in patten? assume volume=64.
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

		for (ROWINDEX nRow = firstRow; nRow <= lastRow; nRow++)
		{
			ModCommand *m = pSndFile->Patterns[m_nPattern].GetpModCommand(nRow, firstChannel);
			const int cy = lastRow - firstRow + 1; // total rows (for fading)
			for (CHANNELINDEX nChn = firstChannel; nChn <= lastChannel; nChn++, m++)
			{
				if ((!m->volcmd) && (m->command != CMD_VOLUME)
					&& (m->note) && (m->note <= NOTE_MAX) && (m->instr))
				{
					UINT nSmp = m->instr;
					bool overrideSampleVol = false;
					if (pSndFile->m_nInstruments)
					{
						if ((nSmp <= pSndFile->m_nInstruments) && (pSndFile->Instruments[nSmp]))
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
					if ((nSmp) && (nSmp <= pSndFile->m_nSamples))
					{
						if(useVolCol)
						{
							m->volcmd = VOLCMD_VOLUME;
							m->vol = (overrideSampleVol) ? 64 : pSndFile->GetSample(nSmp).nVolume / 4;
						} else
						{
							m->command = CMD_VOLUME;
							m->param = (overrideSampleVol) ? 64 : pSndFile->GetSample(nSmp).nVolume / 4;
						}
					}
				}

				if (m->volcmd == VOLCMD_VOLUME) chvol[nChn] = (BYTE)m->vol;

				if (((dlg.m_bFadeIn) || (dlg.m_bFadeOut)) && m->command != CMD_VOLUME && !m->volcmd == VOLCMD_NONE)
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

				if (m->volcmd == VOLCMD_VOLUME)
				{
					int vol = m->vol * dlg.m_nFactor;
					if (dlg.m_bFadeIn) vol = (vol * (nRow+1-firstRow)) / cy;
					if (dlg.m_bFadeOut) vol = (vol * (cy+firstRow-nRow)) / cy;
					vol = (vol + 50) / 100;
					if (vol > 64) vol = 64;
					m->vol = (BYTE)vol;
				}

				if(m_Selection.ContainsHorizontal(PatternCursor(0, nChn, PatternCursor::effectColumn)) || m_Selection.ContainsHorizontal(PatternCursor(0, nChn, PatternCursor::paramColumn)))
				{
					if((m->command == CMD_VOLUME) && (m->param <= 64))
					{
						int vol = m->param * dlg.m_nFactor;
						if (dlg.m_bFadeIn) vol = (vol * (nRow + 1 - firstRow)) / cy;
						if (dlg.m_bFadeOut) vol = (vol * (cy + firstRow - nRow)) / cy;
						vol = (vol + 50) / 100;
						if (vol > 64) vol = 64;
						m->param = (BYTE)vol;
					}
				}
			}
		}
	}
	SetModified(false);
	InvalidateSelection();
	EndWaitCursor();
}


LRESULT CViewPattern::OnPlayerNotify(MPTNOTIFICATION *pnotify)
//------------------------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr || pnotify == nullptr)
	{
		return 0;
	}

	if (pnotify->dwType & MPTNOTIFY_POSITION)
	{
		ORDERINDEX nOrd = pnotify->nOrder;
		ROWINDEX nRow = pnotify->nRow;
		PATTERNINDEX nPat = pnotify->nPattern; //get player pattern
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

		if (pSndFile->m_dwSongFlags & (SONG_PAUSED|SONG_STEP)) return 0;


		if (nOrd >= pSndFile->Order.GetLength() || pSndFile->Order[nOrd] != nPat)
		{
			//order doesn't correlate with pattern, so mark it as invalid
			nOrd = ORDERINDEX_INVALID;
		}

		if (m_pEffectVis && m_pEffectVis->m_hWnd)
		{
			m_pEffectVis->SetPlayCursor(nPat, nRow);
		}

		if ((m_dwStatus & (psFollowSong|psDragVScroll|psDragHScroll|psMouseDragSelect|psRowSelection)) == psFollowSong)
		{
			if (nPat < pSndFile->Patterns.Size())
			{
				if (nPat != m_nPattern || updateOrderList)
				{
					if(nPat != m_nPattern) SetCurrentPattern(nPat, nRow);
					if (nOrd < pSndFile->Order.size()) SendCtrlMessage(CTRLMSG_SETCURRENTORDER, nOrd);
					updateOrderList = false;
				}
				if (nRow != GetCurrentRow())
				{
					SetCurrentRow((nRow < pSndFile->Patterns[nPat].GetNumRows()) ? nRow : 0, FALSE, FALSE);
				}
			}
			SetPlayCursor(PATTERNINDEX_INVALID, 0);
		} else
		{
			if(updateOrderList)
			{
				SendCtrlMessage(CTRLMSG_FORCEREFRESH); //force orderlist refresh
				updateOrderList = false;
			}
			SetPlayCursor(nPat, nRow);
		}



	} //Ends condition "if(pnotify->dwType & MPTNOTIFY_POSITION)"

	if((pnotify->dwType & (MPTNOTIFY_VUMETERS|MPTNOTIFY_STOP)) && (m_dwStatus & psShowVUMeters))
	{
		UpdateAllVUMeters(pnotify);
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
	const bool bUsePlaybackPosition = IsLiveRecord(*pModDoc, *pSndFile);
	ROWINDEX nRow = GetCurrentRow();
	PATTERNINDEX nPattern = m_nPattern;
	if(bUsePlaybackPosition == true)
		SetEditPos(*pSndFile, nRow, nPattern, pSndFile->m_nRow, pSndFile->m_nPattern);

	ModCommand *pRow = pSndFile->Patterns[nPattern].GetpModCommand(nRow, nChn);

	// TODO: Is the right plugin active? Move to a chan with the right plug
	// Probably won't do this - finish fluctuator implementation instead.

	CVstPlugin *pPlug = (CVstPlugin*)pSndFile->m_MixPlugins[plugSlot].pMixPlugin;
	if (pPlug == nullptr) return 0;

	if(pSndFile->GetType() == MOD_TYPE_MPT)
	{
		// MPTM: Use PC Notes

		// only overwrite existing PC Notes
		if(pRow->IsEmpty() || pRow->IsPcNote())
		{
			pModDoc->GetPatternUndo().PrepareUndo(nPattern, nChn, nRow, 1, 1);

			pRow->Set(NOTE_PCS, plugSlot + 1, paramIndex, static_cast<uint16>(pPlug->GetParameter(paramIndex) * ModCommand::maxColumnValue));
			InvalidateRow(nRow);
		}
	} else if(pSndFile->GetModSpecifications().HasCommand(CMD_SMOOTHMIDI))
	{
		// Other formats: Use MIDI macros

		//Figure out which plug param (if any) is controllable using the active macro on this channel.
		long activePlugParam  = -1;
		BYTE activeMacro      = pSndFile->Chn[nChn].nActiveMacro;

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
				pSndFile->Chn[nChn].nActiveMacro = foundMacro;
				if (pRow->command == CMD_NONE || pRow->command == CMD_SMOOTHMIDI || pRow->command == CMD_MIDI) //we overwrite existing Zxx and \xx only.
				{
					pModDoc->GetPatternUndo().PrepareUndo(nPattern, nChn, nRow, 1, 1);

					pRow->command = (pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) ? CMD_S3MCMDEX : CMD_MODCMDEX;;
					pRow->param = 0xF0 + (foundMacro & 0x0F);
					InvalidateRow(nRow);
				}

			}
		}

		//Write the data, but we only overwrite if the command is a macro anyway.
		if (pRow->command == CMD_NONE || pRow->command == CMD_SMOOTHMIDI || pRow->command == CMD_MIDI)
		{
			pModDoc->GetPatternUndo().PrepareUndo(nPattern, nChn, nRow, 1, 1);

			pRow->command = CMD_SMOOTHMIDI;
			pRow->param = pPlug->GetZxxParameter(paramIndex);
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
		SetEditPos(rSf, editpos.nRow, editpos.nPat, rSf.m_nRow, rSf.m_nPattern);
	} else
	{
		editpos.nPat = m_nPattern;
		editpos.nRow = GetCurrentRow();
	}
	editpos.nChn = GetCurrentChannel();

	return editpos;
}


ModCommand* CViewPattern::GetModCommand(CSoundFile& rSf, const ModCommandPos& pos)
//--------------------------------------------------------------------------------
{
	static ModCommand m;
	if (rSf.Patterns.IsValidPat(pos.nPat) && pos.nRow < rSf.Patterns[pos.nPat].GetNumRows() && pos.nChn < rSf.GetNumChannels())
		return rSf.Patterns[pos.nPat].GetpModCommand(pos.nRow, pos.nChn);
	else
		return &m;
}


LRESULT CViewPattern::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
//-------------------------------------------------------------
{
	const DWORD dwMidiData = dwMidiDataParam;
	static BYTE midivolume = 127;

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((!pModDoc) || (!pMainFrm)) return 0;

	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(!pSndFile) return 0;

//Midi message from our perspective:
//     +---------------------------+---------------------------+-------------+-------------+
//bit: | 24.23.22.21 | 20.19.18.17 | 16.15.14.13 | 12.11.10.09 | 08.07.06.05 | 04.03.02.01 |
//     +---------------------------+---------------------------+-------------+-------------+
//     |     Velocity (0-127)      |  Note (middle C is 60)    |   Event     |   Channel   |
//     +---------------------------+---------------------------+-------------+-------------+
//(http://www.borg.com/~jglatt/tech/midispec.htm)

	//Notes:
	//. Initial midi data handling is done in MidiInCallBack().
	//. If no event is received, previous event is assumed.
	//. A note-on (event=9) with velocity 0 is equivalent to a note off.
	//. Basing the event solely on the velocity as follows is incorrect,
	//  since a note-off can have a velocity too:
	//  BYTE event  = (dwMidiData>>16) & 0x64;
	//. Sample- and instrumentview handle midi mesages in their own methods.

	const BYTE nByte1 = GetFromMIDIMsg_DataByte1(dwMidiData);
	const BYTE nByte2 = GetFromMIDIMsg_DataByte2(dwMidiData);

	const BYTE nNote = nByte1 + 1;				// +1 is for MPT, where middle C is 61
	int nVol = nByte2;							// At this stage nVol is a non linear value in [0;127]
												// Need to convert to linear in [0;64] - see below
	BYTE event = GetFromMIDIMsg_Event(dwMidiData);

	if ((event == MIDIEVENT_NOTEON) && !nVol) event = MIDIEVENT_NOTEOFF;	//Convert event to note-off if req'd


	// Handle MIDI mapping.
	uint8 mappedIndex = uint8_max, paramValue = uint8_max;
	uint32 paramIndex = 0;
	const bool bCaptured = pSndFile->GetMIDIMapper().OnMIDImsg(dwMidiData, mappedIndex, paramIndex, paramValue); 

	// Write parameter control commands if needed.
	if (paramValue != uint8_max && IsEditingEnabled() && pSndFile->GetType() == MOD_TYPE_MPT)
	{
		const bool bLiveRecord = IsLiveRecord(*pModDoc, *pSndFile);
		ModCommandPos editpos = GetEditPos(*pSndFile, bLiveRecord);
		ModCommand* p = GetModCommand(*pSndFile, editpos);
		pModDoc->GetPatternUndo().PrepareUndo(editpos.nPat, editpos.nChn, editpos.nRow, 1, 1);
		p->Set(NOTE_PCS, mappedIndex, static_cast<uint16>(paramIndex), static_cast<uint16>((paramValue * ModCommand::maxColumnValue)/127));
		if(bLiveRecord == false)
			InvalidateRow(editpos.nRow);
		pMainFrm->ThreadSafeSetModified(pModDoc);
	}

	if (bCaptured)
		return 0;

	switch(event)
	{
		case MIDIEVENT_NOTEOFF: // Note Off
			// The following method takes care of:
			// . Silencing specific active notes (just setting nNote to 255 as was done before is not acceptible)
			// . Entering a note off in pattern if required
			TempStopNote(nNote, ((CMainFrame::GetSettings().m_dwMidiSetup & MIDISETUP_RECORDNOTEOFF) != 0));
		break;

		case MIDIEVENT_NOTEON: // Note On
			nVol = ApplyVolumeRelatedMidiSettings(dwMidiData, midivolume);
			if(nVol < 0) nVol = -1;
			else nVol = (nVol + 3) / 4; //Value from [0,256] to [0,64]
			TempEnterNote(nNote, true, nVol);
			
			// continue playing as soon as MIDI notes are being received (http://forum.openmpt.org/index.php?topic=2813.0)
			if(pSndFile->IsPaused() && (CMainFrame::GetSettings().m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN))
				pModDoc->OnPatternPlayNoLoop();
				
		break;

		case MIDIEVENT_POLYAFTERTOUCH:
		case MIDIEVENT_CHANAFTERTOUCH:
			break;

		case MIDIEVENT_CONTROLLERCHANGE: //Controller change
			switch(nByte1)
			{
				case MIDICC_Volume_Coarse: //Volume
					midivolume = nByte2;
				break;
			}

			// Checking whether to record MIDI controller change as MIDI macro change.
			// Don't write this if command was already written by MIDI mapping.
			if((paramValue == uint8_max || pSndFile->GetType() != MOD_TYPE_MPT) && IsEditingEnabled() && (CMainFrame::GetSettings().m_dwMidiSetup & MIDISETUP_MIDIMACROCONTROL))
			{  
				const bool bLiveRecord = IsLiveRecord(*pModDoc, *pSndFile);
				ModCommandPos editpos = GetEditPos(*pSndFile, bLiveRecord);
				ModCommand* p = GetModCommand(*pSndFile, editpos);
				if(p->command == CMD_NONE || p->command == CMD_SMOOTHMIDI || p->command == CMD_MIDI)
				{   // Write command only if there's no existing command or already a midi macro command.
					pModDoc->GetPatternUndo().PrepareUndo(editpos.nPat, editpos.nChn, editpos.nRow, 1, 1);
					p->command = CMD_SMOOTHMIDI;
					p->param = nByte2;
					pMainFrm->ThreadSafeSetModified(pModDoc);

					// Update GUI only if not recording live.
					if(bLiveRecord == false)
						InvalidateRow(editpos.nRow);
				}
			}

		default:
			if(CMainFrame::GetSettings().m_dwMidiSetup & MIDISETUP_RESPONDTOPLAYCONTROLMSGS)
			{
				switch(dwMidiData & 0xFF)
				{
				case 0xFA: //Start song
					if(GetDocument())
						GetDocument()->OnPlayerPlayFromStart();
					break;

				case 0xFB: //Continue song
					if(GetDocument())
						GetDocument()->OnPlayerPlay();
					break;

				case 0xFC: //Stop song
					if(GetDocument())
						GetDocument()->OnPlayerStop();
					break;
				}
			}

			if(CMainFrame::GetSettings().m_dwMidiSetup & MIDISETUP_MIDITOPLUG
				&& pMainFrm->GetModPlaying() == pModDoc)
			{
				const UINT instr = GetCurrentInstrument();
				IMixPlugin* plug = pSndFile->GetInstrumentPlugin(instr);
				if(plug)
				{	
					plug->MidiSend(dwMidiData);
					// Sending midi may modify the plug. For now, if MIDI data
					// is not active sensing, set modified.
					if(dwMidiData != MIDISTATUS_ACTIVESENSING)
						pMainFrm->ThreadSafeSetModified(pModDoc);
				}
				
			}
		break;
	}

	return 0;
}




LRESULT CViewPattern::OnModViewMsg(WPARAM wParam, LPARAM lParam)
//--------------------------------------------------------------
{
	switch(wParam)
	{

	case VIEWMSG_SETCTRLWND:
		m_hWndCtrl = (HWND)lParam;
		SetCurrentPattern(SendCtrlMessage(CTRLMSG_GETCURRENTPATTERN));
		break;

	case VIEWMSG_GETCURRENTPATTERN:
		return m_nPattern;

	case VIEWMSG_SETCURRENTPATTERN:
		if (m_nPattern != (UINT)lParam) SetCurrentPattern(lParam);
		break;

	case VIEWMSG_GETCURRENTPOS:
		return (m_nPattern << 16) | GetCurrentRow();

	case VIEWMSG_FOLLOWSONG:
		m_dwStatus &= ~psFollowSong;
		if (lParam)
		{
			CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
			CModDoc *pModDoc = GetDocument();
			m_dwStatus |= psFollowSong;
			if (pModDoc) pModDoc->SetFollowWnd(m_hWnd, MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);
			if (pMainFrm) pMainFrm->SetFollowSong(pModDoc, m_hWnd, TRUE, MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);
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
		if (lParam) m_dwStatus |= psRecordingEnabled; else m_dwStatus &= ~psRecordingEnabled;
		break;

	case VIEWMSG_SETSPACING:
		m_nSpacing = lParam;
		break;

	case VIEWMSG_PATTERNPROPERTIES:
		OnPatternProperties();
		GetParentFrame()->SetActiveView(this);
		break;

	case VIEWMSG_SETVUMETERS:
		if (lParam) m_dwStatus |= psShowVUMeters; else m_dwStatus &= ~psShowVUMeters;
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true);
		break;

	case VIEWMSG_SETPLUGINNAMES:
		if (lParam) m_dwStatus |= psShowPluginNames; else m_dwStatus &= ~psShowPluginNames;
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(true);
		break;

	case VIEWMSG_DOMIDISPACING:
		if (m_nSpacing)
		{
// -> CODE#0012
// -> DESC="midi keyboard split"
			//CModDoc *pModDoc = GetDocument();
			//CSoundFile * pSndFile = pModDoc->GetSoundFile();
//			if (timeGetTime() - lParam >= 10)
			int temp = timeGetTime();
			if (temp - lParam >= 60)
// -! NEW_FEATURE#0012
			{
				CModDoc *pModDoc = GetDocument();
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if ((!(m_dwStatus & psFollowSong))
				 || (pMainFrm->GetFollowSong(pModDoc) != m_hWnd)
				 || (pModDoc->GetSoundFile()->IsPaused()))
				{
					SetCurrentRow(GetCurrentRow() + m_nSpacing);
				}
				m_dwStatus &= ~psMIDISpacingPending;
			} else
			{
// -> CODE#0012
// -> DESC="midi keyboard split"
//				Sleep(1);
				Sleep(0);
				PostMessage(WM_MOD_VIEWMSG, VIEWMSG_DOMIDISPACING, lParam);
			}
		} else
		{
			m_dwStatus &= ~psMIDISpacingPending;
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
			pState->nOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER); //rewbs.playSongFromCursor
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
			if(pSndFile != nullptr)
			{
				CopyPattern(m_nPattern, PatternRect(PatternCursor(0, 0), PatternCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1)));
			}
		}
		break;

	case VIEWMSG_PASTEPATTERN:
		{
			PastePattern(m_nPattern, 0, PatternClipboard::pmOverwrite);
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
		CModScrollView::OnMouseWheel(0, lParam, CPoint(0, 0));
		break;


	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}

//rewbs.customKeys
void CViewPattern::CursorJump(DWORD distance, bool upwards, bool snap)
//--------------------------------------------------------------------
{
	switch(snap)
	{
		case false:
			//no snap
			SetCurrentRow(GetCurrentRow() + ((int)(upwards ? -1 : 1)) * distance, TRUE); 
			break;

		case true:
			//snap
			SetCurrentRow((((GetCurrentRow() + (int)(upwards ? -1 : 0)) / distance) + (int)(upwards ? 0 : 1)) * distance, TRUE);
			break;
	}

}


LRESULT CViewPattern::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//----------------------------------------------------------------
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
		case kcChannelMute:					OnMuteChannel(true); return wParam;
		case kcChannelSolo:					OnSoloChannel(true); return wParam;
		case kcChannelUnmuteAll:			OnUnmuteAll(); return wParam;
		case kcToggleChanMuteOnPatTransition: TogglePendingMute(GetCurrentChannel()); return wParam;
		case kcUnmuteAllChnOnPatTransition:	OnPendingUnmuteAllChnFromClick(); return wParam;
		case kcChannelReset:				OnChannelReset(); return wParam;
		case kcTimeAtRow:					OnShowTimeAtRow(); return wParam;
		case kcSoloChnOnPatTransition:		PendingSoloChn(GetCurrentChannel()); return wParam;
		case kcTransposeUp:					OnTransposeUp(); return wParam;
		case kcTransposeDown:				OnTransposeDown(); return wParam;
		case kcTransposeOctUp:				OnTransposeOctUp(); return wParam;
		case kcTransposeOctDown:			OnTransposeOctDown(); return wParam;
		case kcSelectColumn:				OnSelectCurrentColumn(); return wParam;
		case kcPatternAmplify:				OnPatternAmplify(); return wParam;
		case kcPatternSetInstrument:		OnSetSelInstrument(); return wParam;
		case kcPatternInterpolateNote:		OnInterpolateNote(); return wParam;
		case kcPatternInterpolateVol:		OnInterpolateVolume(); return wParam;
		case kcPatternInterpolateEffect:	OnInterpolateEffect(); return wParam;
		case kcPatternVisualizeEffect:		OnVisualizeEffect(); return wParam;
		case kcPatternOpenRandomizer:		OnOpenRandomizer(); return wParam;
		case kcPatternGrowSelection:		OnGrowSelection(); return wParam;
		case kcPatternShrinkSelection:		OnShrinkSelection(); return wParam;

		// Pattern navigation:
		case kcPatternJumpUph1Select:
		case kcPatternJumpUph1:			CursorJump(GetRowsPerMeasure(), true, false); return wParam;
		case kcPatternJumpDownh1Select:
		case kcPatternJumpDownh1:		CursorJump(GetRowsPerMeasure(), false, false);  return wParam;
		case kcPatternJumpUph2Select:
		case kcPatternJumpUph2:			CursorJump(GetRowsPerBeat(), true, false);  return wParam;
		case kcPatternJumpDownh2Select:
		case kcPatternJumpDownh2:		CursorJump(GetRowsPerBeat(), false, false);  return wParam;

		case kcPatternSnapUph1Select:
		case kcPatternSnapUph1:			CursorJump(GetRowsPerMeasure(), true, true); return wParam;
		case kcPatternSnapDownh1Select:
		case kcPatternSnapDownh1:		CursorJump(GetRowsPerMeasure(), false, true);  return wParam;
		case kcPatternSnapUph2Select:
		case kcPatternSnapUph2:			CursorJump(GetRowsPerBeat(), true, true);  return wParam;
		case kcPatternSnapDownh2Select:
		case kcPatternSnapDownh2:		CursorJump(GetRowsPerBeat(), false, true);  return wParam;

		case kcNavigateDownSelect:
		case kcNavigateDown:	SetCurrentRow(GetCurrentRow() + 1, TRUE); return wParam;
		case kcNavigateUpSelect:
		case kcNavigateUp:		SetCurrentRow(GetCurrentRow() - 1, TRUE); return wParam;

		case kcNavigateDownBySpacingSelect:
		case kcNavigateDownBySpacing:	SetCurrentRow(GetCurrentRow() + m_nSpacing, TRUE); return wParam;
		case kcNavigateUpBySpacingSelect:
		case kcNavigateUpBySpacing:		SetCurrentRow(GetCurrentRow() - m_nSpacing, TRUE); return wParam;

		case kcNavigateLeftSelect:
		case kcNavigateLeft:	if ((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_WRAP) && m_Cursor.IsInFirstColumn())
									SetCurrentColumn(pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn);
								else
								{
									m_Cursor.Move(0, 0, -1);
									SetCurrentColumn(m_Cursor);
								}
								return wParam;
		case kcNavigateRightSelect:
		case kcNavigateRight:	if ((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_WRAP) && (m_Cursor.CompareColumn(PatternCursor(0, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn)) >= 0))
									SetCurrentColumn(0);
								else
								{
									m_Cursor.Move(0, 0, 1);
									SetCurrentColumn(m_Cursor);
								}
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
		case kcEndHorizontal:	if (m_Cursor.CompareColumn(PatternCursor(0, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn)) < 0) SetCurrentColumn(pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn);
								else if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								return wParam;
		case kcEndVerticalSelect:
		case kcEndVertical:		if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								else if (m_Cursor.CompareColumn(PatternCursor(0, pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn)) < 0) SetCurrentColumn(pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn);
								return wParam;
		case kcEndAbsoluteSelect:
		case kcEndAbsolute:		SetCurrentColumn(pSndFile->GetNumChannels() - 1, PatternCursor::lastColumn);
								if (GetCurrentRow() < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								return wParam;

		case kcNextPattern:	{	PATTERNINDEX n = m_nPattern + 1;
            					while ((n < pSndFile->Patterns.Size()) && !pSndFile->Patterns.IsValidPat(n)) n++;
								SetCurrentPattern((n < pSndFile->Patterns.Size()) ? n : 0);
								ORDERINDEX currentOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
								ORDERINDEX newOrder = pSndFile->Order.FindOrder(m_nPattern, currentOrder, true);
								SendCtrlMessage(CTRLMSG_SETCURRENTORDER, newOrder);
								return wParam;
							}
		case kcPrevPattern: {	PATTERNINDEX n = (m_nPattern) ? m_nPattern - 1 : pSndFile->Patterns.Size() - 1;
								while (n > 0 && !pSndFile->Patterns.IsValidPat(n)) n--;
								SetCurrentPattern(n);
								ORDERINDEX currentOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
								ORDERINDEX newOrder = pSndFile->Order.FindOrder(m_nPattern, currentOrder, false);
								SendCtrlMessage(CTRLMSG_SETCURRENTORDER, newOrder);
								return wParam;
							}
		case kcSelectWithCopySelect:
		case kcSelectWithNav:
		case kcSelect:			if (!(m_dwStatus & (psDragnDropEdit|psRowSelection))) m_StartSel = m_Cursor;
									m_dwStatus |= psKeyboardDragSelect;	return wParam;
		case kcSelectOffWithCopySelect:
		case kcSelectOffWithNav:
		case kcSelectOff:		m_dwStatus &= ~psKeyboardDragSelect; return wParam;
		case kcCopySelectWithSelect:
		case kcCopySelectWithNav:
		case kcCopySelect:		if (!(m_dwStatus & (psDragnDropEdit|psRowSelection))) m_StartSel = m_Cursor;
									m_dwStatus |= psCtrlDragSelect; return wParam;
		case kcCopySelectOffWithSelect:
		case kcCopySelectOffWithNav:
		case kcCopySelectOff:	m_dwStatus &= ~psCtrlDragSelect; return wParam;

		case kcSelectBeat:
		case kcSelectMeasure:
			SelectBeatOrMeasure(wParam == kcSelectBeat); return wParam;

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
		case kcShowMacroConfig:	SendCtrlMessage(CTRLMSG_SETUPMACROS); return wParam;
		case kcShowSplitKeyboardSettings:	SetSplitKeyboardSettings(); return wParam;
		case kcShowEditMenu:	{
									CPoint pt = GetPointFromPosition(m_Cursor);
									OnRButtonDown(0, pt);
								}
								return wParam;
		case kcPatternGoto:		OnEditGoto(); return wParam;

		case kcNoteCut:			TempEnterNote(NOTE_NOTECUT, false); return wParam;
		case kcNoteCutOld:		TempEnterNote(NOTE_NOTECUT, true);  return wParam;
		case kcNoteOff:			TempEnterNote(NOTE_KEYOFF, false); return wParam;
		case kcNoteOffOld:		TempEnterNote(NOTE_KEYOFF, true);  return wParam;
		case kcNoteFade:		TempEnterNote(NOTE_FADE, false); return wParam;
		case kcNoteFadeOld:		TempEnterNote(NOTE_FADE, true);  return wParam;
		case kcNotePC:			TempEnterNote(NOTE_PC); return wParam;
		case kcNotePCS:			TempEnterNote(NOTE_PCS); return wParam;

		case kcEditUndo:		OnEditUndo(); return wParam;
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
		case kcSwitchOverflowPaste:	CMainFrame::GetSettings().m_dwPatternSetup ^= PATTERN_OVERFLOWPASTE; return wParam;
		case kcPatternEditPCNotePlugin: OnTogglePCNotePluginEditor(); return wParam;

		case kcDecreaseSpacing:
			if(m_nSpacing > 0) SetSpacing(m_nSpacing - 1);
			break;
		case kcIncreaseSpacing:
			if(m_nSpacing < MAX_SPACING) SetSpacing(m_nSpacing + 1);
			break;

	}
	//Ranges:
	if (wParam>=kcVPStartNotes && wParam<=kcVPEndNotes)
	{
		TempEnterNote(wParam-kcVPStartNotes+1+pMainFrm->GetBaseOctave()*12);
		return wParam;
	}
	if (wParam>=kcVPStartChords && wParam<=kcVPEndChords)
	{
		TempEnterChord(wParam-kcVPStartChords+1+pMainFrm->GetBaseOctave()*12);
		return wParam;
	}

	if (wParam>=kcVPStartNoteStops && wParam<=kcVPEndNoteStops)
	{
		TempStopNote(wParam-kcVPStartNoteStops+1+pMainFrm->GetBaseOctave()*12);
		return wParam;
	}
	if (wParam>=kcVPStartChordStops && wParam<=kcVPEndChordStops)
	{
		TempStopChord(wParam-kcVPStartChordStops+1+pMainFrm->GetBaseOctave()*12);
		return wParam;
	}

	if (wParam>=kcSetSpacing0 && wParam<kcSetSpacing9)
	{
		SetSpacing(wParam - kcSetSpacing0);
		return wParam;
	}

	if (wParam>=kcSetIns0 && wParam<=kcSetIns9)
	{
		if (IsEditingEnabled_bmsg())
			TempEnterIns(wParam-kcSetIns0);
		return wParam;
	}

	if (wParam>=kcSetOctave0 && wParam<=kcSetOctave9)
	{
		if (IsEditingEnabled())
			TempEnterOctave(wParam-kcSetOctave0);
		return wParam;
	}
	if (wParam>=kcSetVolumeStart && wParam<=kcSetVolumeEnd)
	{
		if (IsEditingEnabled_bmsg())
			TempEnterVol(wParam-kcSetVolumeStart);
		return wParam;
	}
	if (wParam>=kcSetFXStart && wParam<=kcSetFXEnd)
	{
		if (IsEditingEnabled_bmsg())
			TempEnterFX(wParam-kcSetFXStart+1);
		return wParam;
	}
	if (wParam>=kcSetFXParam0 && wParam<=kcSetFXParamF)
	{
		if (IsEditingEnabled_bmsg())
			TempEnterFXparam(wParam-kcSetFXParam0);
		return wParam;
	}

	return NULL;
}

#define ENTER_PCNOTE_VALUE(v, method) \
	{ \
		if((v >= 0) && (v <= 9)) \
		{	\
			uint16 val = pTarget->Get##method##(); \
			/* Move existing digits to left, drop out leftmost digit and */ \
			/* push new digit to the least meaning digit. */ \
			val = (val % 100) * 10 + v; \
			if(val > ModCommand::maxColumnValue) val = ModCommand::maxColumnValue; \
			pTarget->Set##method##(val); \
			m_PCNoteEditMemory = *pTarget; \
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
		
	PrepareUndo(m_Selection);

	ModCommand* pTarget = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), GetCurrentChannel());
	ModCommand oldcmd = *pTarget; // This is the command we are about to overwrite

	if(pTarget->IsPcNote())
	{
		ENTER_PCNOTE_VALUE(v, ValueVolCol);
	}
	else
	{
		UINT volcmd = pTarget->volcmd;
		UINT vol = pTarget->vol;
		if ((v >= 0) && (v <= 9))
		{
			vol = ((vol * 10) + v) % 100;
			if (!volcmd) volcmd = VOLCMD_VOLUME;
		}
		else
			switch(v+kcSetVolumeStart)
		{
			case kcSetVolumeVol:			volcmd = VOLCMD_VOLUME; break;
			case kcSetVolumePan:			volcmd = VOLCMD_PANNING; break;
			case kcSetVolumeVolSlideUp:		volcmd = VOLCMD_VOLSLIDEUP; break;
			case kcSetVolumeVolSlideDown:	volcmd = VOLCMD_VOLSLIDEDOWN; break;
			case kcSetVolumeFineVolUp:		volcmd = VOLCMD_FINEVOLUP; break;
			case kcSetVolumeFineVolDown:	volcmd = VOLCMD_FINEVOLDOWN; break;
			case kcSetVolumeVibratoSpd:		if (pSndFile->m_nType & MOD_TYPE_XM) volcmd = VOLCMD_VIBRATOSPEED; break;
			case kcSetVolumeVibrato:		volcmd = VOLCMD_VIBRATODEPTH; break;
			case kcSetVolumeXMPanLeft:		if (pSndFile->m_nType & MOD_TYPE_XM) volcmd = VOLCMD_PANSLIDELEFT; break;
			case kcSetVolumeXMPanRight:		if (pSndFile->m_nType & MOD_TYPE_XM) volcmd = VOLCMD_PANSLIDERIGHT; break;
			case kcSetVolumePortamento:		volcmd = VOLCMD_TONEPORTAMENTO; break;
			case kcSetVolumeITPortaUp:		if (pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) volcmd = VOLCMD_PORTAUP; break;
			case kcSetVolumeITPortaDown:	if (pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) volcmd = VOLCMD_PORTADOWN; break;
			case kcSetVolumeITOffset:		if (pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) volcmd = VOLCMD_OFFSET; break;		//rewbs.volOff
		}

		UINT max = 64;
		if (volcmd > VOLCMD_PANNING)
		{
			max = (pSndFile->m_nType == MOD_TYPE_XM) ? 0x0F : 9;
		}

		if (vol > max) vol %= 10;
		if(pSndFile->GetModSpecifications().HasVolCommand(volcmd))
		{
			pTarget->volcmd = volcmd;
			pTarget->vol = vol;
		}
	}

	if (IsEditingEnabled_bmsg())
	{
		SetSelToCursor();

		if(oldcmd != *pTarget)
		{
			SetModified(false);
			InvalidateCell(m_Cursor);
			UpdateIndicator();
		}
	}
	else
	{
		// recording disabled
		*pTarget = oldcmd;
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
void CViewPattern::TempEnterFX(int c, int v)
//------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();

	if(pSndFile == nullptr || !IsEditingEnabled_bmsg())
	{
		return;
	}

	ModCommand *pTarget = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), GetCurrentChannel());
	ModCommand oldcmd = *pTarget; // This is the command we are about to overwrite

	PrepareUndo(m_Selection);

	if(pTarget->IsPcNote())
	{
		ENTER_PCNOTE_VALUE(c, ValueEffectCol);
	}
	else if(pSndFile->GetModSpecifications().HasCommand(c))
	{

		if (c)
		{
			if ((c == m_cmdOld.command) && (!pTarget->param) && (!pTarget->command)) pTarget->param = m_cmdOld.param;
			else m_cmdOld.param = 0;
			m_cmdOld.command = c;
		}
		pTarget->command = c;
		if(v >= 0)
		{
			pTarget->param = v;
		}

		// Check for MOD/XM Speed/Tempo command
		if ((pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
			&& ((pTarget->command == CMD_SPEED) || (pTarget->command == CMD_TEMPO)))
		{
			pTarget->command = (pTarget->param <= pSndFile->GetModSpecifications().speedMax) ? CMD_SPEED : CMD_TEMPO;
		}
	}

	SetSelToCursor();

	if(oldcmd != *pTarget)
	{
		SetModified(false);
		InvalidateCell(m_Cursor);
		UpdateIndicator();
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

	ModCommand *pTarget = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), GetCurrentChannel());
	ModCommand oldcmd = *pTarget; // This is the command we are about to overwrite

	PrepareUndo(m_Selection);

	if(pTarget->IsPcNote())
	{
		ENTER_PCNOTE_VALUE(v, ValueEffectCol);
	} else
	{

		pTarget->param = (pTarget->param << 4) | v;
		if (pTarget->command == m_cmdOld.command) m_cmdOld.param = pTarget->param;

		// Check for MOD/XM Speed/Tempo command
		if ((pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
			&& ((pTarget->command == CMD_SPEED) || (pTarget->command == CMD_TEMPO)))
		{
			pTarget->command = (pTarget->param <= pSndFile->GetModSpecifications().speedMax) ? CMD_SPEED : CMD_TEMPO;
		}
	}

	SetSelToCursor();

	if(*pTarget != oldcmd)
	{
		SetModified(false);
		InvalidateCell(m_Cursor);
		UpdateIndicator();
	}
}


// Stop a note that has been entered
void CViewPattern::TempStopNote(int note, bool fromMidi, const bool bChordMode)
//-----------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	// Get playback edit positions from play engine here in case they are needed below.
	const ROWINDEX nRowPlayback = pSndFile->m_nRow;
	const UINT nTick = pSndFile->m_nTickCount;
	const PATTERNINDEX nPatPlayback = pSndFile->m_nPattern;

	const bool isSplit = (pModDoc->GetSplitKeyboardSettings().IsSplitActive()) && (note <= pModDoc->GetSplitKeyboardSettings().splitNote);
	UINT ins = 0;
	if (pModDoc)
	{
		if (isSplit)
		{
			ins = pModDoc->GetSplitKeyboardSettings().splitInstrument;
			if (pModDoc->GetSplitKeyboardSettings().octaveLink) note += 12 *pModDoc->GetSplitKeyboardSettings().octaveModifier;
			if (note > NOTE_MAX && note < NOTE_MIN_SPECIAL) note = NOTE_MAX;
			if (note < 0) note = NOTE_MIN;
		}
		if (!ins)    ins = GetCurrentInstrument();
		if (!ins)	 ins = m_nFoundInstrument;
		if(bChordMode == true)
		{
			m_dwStatus &= ~psChordPlaying;
			pModDoc->NoteOff(0, true, ins, GetCurrentChannel());	// XXX this doesn't stop VSTi notes!
		}
		else
		{
			pModDoc->NoteOff(note, ((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_NOTEFADE) || pSndFile->GetNumInstruments() == 0), ins, GetCurrentChannel());
		}
	}

	// Enter note off in pattern?
	if(!ModCommand::IsNote(note))
		return;
	if (m_Cursor.GetColumnType() > PatternCursor::instrColumn && (bChordMode || !fromMidi))
		return;
	if (!pModDoc || !pMainFrm || !(IsEditingEnabled()))
		return;

	const bool bIsLiveRecord = IsLiveRecord(*pMainFrm, *pModDoc, *pSndFile);

	const CHANNELINDEX nChnCursor = GetCurrentChannel();

	BYTE* activeNoteMap = isSplit ? splitActiveNoteChannel : activeNoteChannel;
	const CHANNELINDEX nChn = (activeNoteMap[note] < pSndFile->GetNumChannels()) ? activeNoteMap[note] : nChnCursor;

	activeNoteMap[note] = 0xFF;	//unlock channel

	if (!((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_KBDNOTEOFF) || fromMidi))
	{
		// We don't want to write the note-off into the pattern if this feature is disabled and we're not recording from MIDI.
		return;
	}

	// -- write sdx if playing live
	const bool usePlaybackPosition = (!bChordMode) && (bIsLiveRecord && (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_AUTODELAY));

	//Work out where to put the note off
	ROWINDEX nRow = GetCurrentRow();
	PATTERNINDEX nPat = m_nPattern;

	if(usePlaybackPosition)
		SetEditPos(*pSndFile, nRow, nPat, nRowPlayback, nPatPlayback);

	ModCommand* pTarget = pSndFile->Patterns[nPat].GetpModCommand(nRow, nChn);

	//don't overwrite:
	if (pTarget->note || pTarget->instr || pTarget->volcmd)
	{
		//if there's a note in the current location and the song is playing and following,
		//the user probably just tapped the key - let's try the next row down.
		nRow++;
		if (pTarget->note==note && bIsLiveRecord && pSndFile->Patterns[nPat].IsValidRow(nRow))
		{
			pTarget = pSndFile->Patterns[nPat].GetpModCommand(nRow, nChn);
			if (pTarget->note || (!bChordMode && (pTarget->instr || pTarget->volcmd)) )
				return;
		}
		else
			return;
	}

	// Create undo-point.
	pModDoc->GetPatternUndo().PrepareUndo(nPat, nChn, nRow, 1, 1);

	// -- write sdx if playing live
	if (usePlaybackPosition && nTick) {	// avoid SD0 which will be mis-interpreted
		if (pTarget->command == 0) {	//make sure we don't overwrite any existing commands.
			pTarget->command = (pSndFile->TypeIsS3M_IT_MPT()) ? CMD_S3MCMDEX : CMD_MODCMDEX;
			pTarget->param   = 0xD0 | min(0xF, nTick);
		}
	}

	//Enter note off
	if(pSndFile->GetModSpecifications().hasNoteOff && (pSndFile->GetNumInstruments() > 0 || !pSndFile->GetModSpecifications().hasNoteCut))
	{
		 // ===
		// Not used in sample (if module format supports ^^^ instead)
		pTarget->note = NOTE_KEYOFF;
	} else if(pSndFile->GetModSpecifications().hasNoteCut)
	{
		// ^^^
		pTarget->note = NOTE_NOTECUT;
	} else
	{
		// we don't have anything to cut (MOD format) - use volume or ECx
		if(usePlaybackPosition && nTick) // ECx
		{
			pTarget->command = (pSndFile->TypeIsS3M_IT_MPT()) ? CMD_S3MCMDEX : CMD_MODCMDEX;
			pTarget->param   = 0xC0 | min(0xF, nTick);
		} else // C00
		{
			pTarget->note = NOTE_NONE;
			pTarget->command = CMD_VOLUME;
			pTarget->param = 0;
		}
	}
	pTarget->instr = (bChordMode) ? 0 : ins; //p->instr = 0; 
	//Writing the instrument as well - probably someone finds this annoying :)
	pTarget->volcmd	= VOLCMD_NONE;
	pTarget->vol = 0;

	pModDoc->SetModified();

	// Update only if not recording live.
	if(bIsLiveRecord == false)
	{
		PatternCursor sel(nRow, nChn);
		InvalidateCell(sel);
		UpdateIndicator();
	}

	return;

}


// Enter an octave number in the pattern
void CViewPattern::TempEnterOctave(int val)
//-----------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	PrepareUndo(m_Selection);

	ModCommand* pTarget = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), GetCurrentChannel());
	ModCommand oldcmd = *pTarget; // This is the command we are about to overwrite
	if (oldcmd.note)
	{
		TempEnterNote(((oldcmd.note - 1) % 12) + val * 12 + 1);
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

	PrepareUndo(m_Selection);

	ModCommand *pTarget = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), GetCurrentChannel());
	ModCommand oldcmd = *pTarget;		// This is the command we are about to overwrite

	UINT instr = pTarget->instr, nTotalMax, nTempMax;
	if(pTarget->IsPcNote())	// this is a plugin index
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
	pTarget->instr = instr;

	SetSelToCursor();

	if(*pTarget != oldcmd)
	{
		SetModified(false);
		InvalidateCell(m_Cursor);
		UpdateIndicator();
	}

	if(pTarget->IsPcNote())
	{
		m_PCNoteEditMemory = *pTarget;
	}
}


// Enter a note in the pattern
void CViewPattern::TempEnterNote(int note, bool oldStyle, int vol)
//----------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	if(pMainFrm == nullptr || pModDoc == nullptr || (pSndFile = pModDoc->GetSoundFile()) == nullptr)
	{
		return;
	}

	ROWINDEX nRow = GetCurrentRow();
	const ROWINDEX nRowPlayback = pSndFile->m_nRow;
	const UINT nTick = pSndFile->m_nTickCount;
	const PATTERNINDEX nPatPlayback = pSndFile->m_nPattern;

	const bool recordEnabled = IsEditingEnabled();
	CHANNELINDEX nChn = GetCurrentChannel();

	if(note < NOTE_MIN_SPECIAL)
	{
		Limit(note, pSndFile->GetModSpecifications().noteMin, pSndFile->GetModSpecifications().noteMax);
	}

	// Special case: Convert note off commands to C00 for MOD files
	if((pSndFile->GetType() == MOD_TYPE_MOD) && (note == NOTE_NOTECUT || note == NOTE_FADE || note == NOTE_KEYOFF))
	{
		TempEnterFX(CMD_VOLUME, 0);
		return;
	}

	// Check whether the module format supports the note.
	if(pSndFile->GetModSpecifications().HasNote(note) == false)
	{
		return;
	}

	BYTE recordGroup = pModDoc->IsChannelRecord(nChn);
	const bool bIsLiveRecord = IsLiveRecord(*pMainFrm, *pModDoc, *pSndFile);
	const bool usePlaybackPosition = (bIsLiveRecord && (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_AUTODELAY) && !(pSndFile->m_dwSongFlags & SONG_STEP));
	const bool isSplit = IsNoteSplit(note);

	if(pModDoc->GetSplitKeyboardSettings().IsSplitActive()
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
	if (recordEnabled && recordGroup && !bIsLiveRecord && !ModCommand::IsPcNote(note))
	{
		if (m_nSpacing > 0 && m_nSpacing <= MAX_SPACING)
		{
			if ((timeGetTime() - m_dwLastNoteEntryTime < CMainFrame::GetSettings().gnAutoChordWaitTime)
				&& (nRow >= m_nSpacing) && (!m_bLastNoteEntryBlocked))
			{
				nRow -= m_nSpacing;
			}
		}
	}
	m_dwLastNoteEntryTime = timeGetTime();

	PATTERNINDEX nPat = m_nPattern;
	if(usePlaybackPosition)
		SetEditPos(*pSndFile, nRow, nPat, nRowPlayback, nPatPlayback);

	// -- Work out where to put the new note
	ModCommand *pTarget = pSndFile->Patterns[nPat].GetpModCommand(nRow, nChn);
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
		HandleSplit(&newcmd, note);

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

		if(volWrite != -1)
		{
			if(pSndFile->GetModSpecifications().HasVolCommand(VOLCMD_VOLUME))
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
		if (usePlaybackPosition && nTick)	// avoid SD0 which will be mis-interpreted
		{
			if (newcmd.command == CMD_NONE)	//make sure we don't overwrite any existing commands.
			{
				newcmd.command = (pSndFile->TypeIsS3M_IT_MPT()) ? CMD_S3MCMDEX : CMD_MODCMDEX;
				UINT maxSpeed = 0x0F;
				if(pSndFile->m_nMusicSpeed > 0) maxSpeed = min(0x0F, pSndFile->m_nMusicSpeed - 1);
				newcmd.param = 0xD0 + min(maxSpeed, nTick);
			}
		}

		// -- old style note cut/off/fade: erase instrument number
		if (oldStyle && newcmd.note >= NOTE_MIN_SPECIAL)
			newcmd.instr = 0;

	}

	// -- if recording, create undo point and write out modified command.
	const bool modified = (recordEnabled && *pTarget != newcmd);
	if (modified)
	{
		pModDoc->GetPatternUndo().PrepareUndo(nPat, nChn, nRow, 1, 1);
		*pTarget = newcmd;
	}

	// -- play note
	if (((CMainFrame::GetSettings().m_dwPatternSetup & (PATTERN_PLAYNEWNOTE|PATTERN_PLAYEDITROW)) || !recordEnabled) && !newcmd.IsPcNote())
	{
		const bool playWholeRow = ((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !bIsLiveRecord);
		if (playWholeRow)
		{
			// play the whole row in "step mode"
			PatternStep(false);
		}
		if (!playWholeRow || !recordEnabled)
		{
			// NOTE: This code is *also* used for the PATTERN_PLAYEDITROW edit mode because of some unforseeable race conditions when modifying pattern data.
			// We have to use this code when editing is disabled or else we will get some stupid hazards, because we would first have to write the new note
			// data to the pattern and then remove it again - but often, it is actually removed before the row is parsed by the soundlib.

			// just play the newly inserted note using the already specified instrument...
			UINT nPlayIns = newcmd.instr;
			if(!nPlayIns && ModCommand::IsNoteOrEmpty(note))
			{
				// ...or one that can be found on a previous row of this pattern.
				ModCommand *search = pTarget;
				ROWINDEX srow = nRow;
				while(srow-- > 0)
				{
					search -= pSndFile->GetNumChannels();
					if (search->instr && !search->IsPcNote())
					{
						nPlayIns = search->instr;
						m_nFoundInstrument = nPlayIns;  //used to figure out which instrument to stop on key release.
						break;
					}
				}
			}
			bool isPlaying = ((pMainFrm->GetModPlaying() == pModDoc) && (pMainFrm->IsPlaying()));
			pModDoc->PlayNote(newcmd.note, nPlayIns, 0, !isPlaying, 4 * vol, 0, 0, nChn);
		}
	}

	// -- if recording, handle post note entry behaviour (move cursor etc..)
	if(recordEnabled)
	{
		PatternCursor sel(nRow, nChn, m_Cursor.GetColumnType());
		if(bIsLiveRecord == false)
		{
			// Update only when not recording live.
			SetCurSel(sel);
		}

		if(modified) // Has it really changed?
		{
			pModDoc->SetModified();
			if(bIsLiveRecord == false)
			{
				// Update only when not recording live.
				InvalidateCell(sel);
				UpdateIndicator();
			}
		}

		// Set new cursor position (row spacing)
		if (!bIsLiveRecord)
		{
			if((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING))
			{
				if(nRow + m_nSpacing < pSndFile->Patterns[nPat].GetNumRows() || (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CONTSCROLL))
				{
					SetCurrentRow(nRow + m_nSpacing, (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_CONTSCROLL) ? true: false);
					m_bLastNoteEntryBlocked = false;
				} else
				{
					m_bLastNoteEntryBlocked = true;  // if the cursor is block by the end of the pattern here,
				}								     // we must remember to not step back should the next note form a chord.

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
			activeNoteMap[newcmd.note] = nChn;

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


// Enter a chord in the pattern
void CViewPattern::TempEnterChord(int note)
//-----------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	if(pMainFrm == nullptr || pModDoc == nullptr || (pSndFile = pModDoc->GetSoundFile()) == nullptr)
	{
		return;
	}

	UINT nPlayChord = 0;
	BYTE chordplaylist[3];

	const CHANNELINDEX nChn = GetCurrentChannel();
	UINT nPlayIns = 0;
	// Simply backup the whole row.
	pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, nChn, GetCurrentRow(), pSndFile->GetNumChannels(), 1);

	const PatternRow rowBase = pSndFile->Patterns[m_nPattern].GetRow(GetCurrentRow());
	ModCommand* pTarget = &rowBase[nChn];
	// Save old row contents		
	vector<ModCommand> newrow(pSndFile->GetNumChannels());
	for(CHANNELINDEX n = 0; n < pSndFile->GetNumChannels(); n++)
	{
		newrow[n] = rowBase[n];
	}
	// This is being written to
	ModCommand* p = &newrow[nChn];


	const bool bIsLiveRecord = IsLiveRecord(*pMainFrm, *pModDoc, *pSndFile);
	const bool bRecordEnabled = IsEditingEnabled();

	// -- establish note data
	HandleSplit(p, note);

	PMPTCHORD pChords = pMainFrm->GetChords();
	UINT baseoctave = pMainFrm->GetBaseOctave();
	UINT nchord = note - baseoctave * 12 - 1;
	UINT nNote;
	bool modified = false;
	if (nchord < 3 * 12)
	{
		UINT nchordnote = pChords[nchord].key + baseoctave * 12 + 1;

		if (nchordnote <= NOTE_MAX)
		{
			UINT nchordch = nChn, nchno = 0;
			nNote = nchordnote;
			p->note = nNote;
			if(rowBase[nChn] != *p)
			{
				modified = true;
			}

			BYTE recordGroup, currentRecordGroup = 0;

			recordGroup = pModDoc->IsChannelRecord(nChn);

			for (UINT kchrd=1; kchrd<pSndFile->m_nChannels; kchrd++)
			{
				if ((nchno > 2) || (!pChords[nchord].notes[nchno])) break;
				if (++nchordch >= pSndFile->m_nChannels) nchordch = 0;

				currentRecordGroup = pModDoc->IsChannelRecord(nchordch);
				if (!recordGroup)
					recordGroup = currentRecordGroup;  //record group found

				UINT n = ((nchordnote-1)/12) * 12 + pChords[nchord].notes[nchno];
				if(bRecordEnabled)
				{
					if ((nchordch != nChn) && recordGroup && (currentRecordGroup == recordGroup) && (n <= NOTE_MAX))
					{
						newrow[nchordch].note = n;
						if (p->instr) newrow[nchordch].instr = p->instr;
						if(rowBase[nchordch] != newrow[nchordch])
						{
							modified = true;
						}

						nchno++;
						if (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_PLAYNEWNOTE)
						{
							if ((n) && (n <= NOTE_MAX)) chordplaylist[nPlayChord++] = n;
						}
					}
				} else
				{
					nchno++;
					if ((n) && (n <= NOTE_MAX)) chordplaylist[nPlayChord++] = n;
				}
			}
		}
	}


	// -- write notedata
	if(bRecordEnabled)
	{
		SetSelToCursor();

		if(modified)
		{
			for(CHANNELINDEX n = 0; n < pSndFile->GetNumChannels(); n++)
			{
				rowBase[n] = newrow[n];
			}
			pModDoc->SetModified();
			InvalidateRow();
			UpdateIndicator();
		}
	}


	// -- play note
	if ((CMainFrame::GetSettings().m_dwPatternSetup & (PATTERN_PLAYNEWNOTE|PATTERN_PLAYEDITROW)) || (!bRecordEnabled))
	{
		const bool playWholeRow = ((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_PLAYEDITROW) && !bIsLiveRecord);
		if (playWholeRow)
		{
			// play the whole row in "step mode"
			PatternStep(false);
		}
		if (!playWholeRow || !bRecordEnabled)
		{
			// NOTE: This code is *also* used for the PATTERN_PLAYEDITROW edit mode because of some unforseeable race conditions when modifying pattern data.
			// We have to use this code when editing is disabled or else we will get some stupid hazards, because we would first have to write the new note
			// data to the pattern and then remove it again - but often, it is actually removed before the row is parsed by the soundlib.
			// just play the newly inserted notes...

			if (p->instr)
			{
				// ...using the already specified instrument
				nPlayIns = p->instr;
			} else if ((!p->instr) && (p->note <= NOTE_MAX))
			{
				// ...or one that can be found on a previous row of this pattern.
				ModCommand *search = pTarget;
				ROWINDEX srow = GetCurrentRow();
				while (srow-- > 0)
				{
					search -= pSndFile->GetNumChannels();
					if (search->instr)
					{
						nPlayIns = search->instr;
						m_nFoundInstrument = nPlayIns;  //used to figure out which instrument to stop on key release.
						break;
					}
				}
			}
			bool isPlaying = ((pMainFrm->GetModPlaying() == pModDoc) && (pMainFrm->IsPlaying()));
			pModDoc->PlayNote(p->note, nPlayIns, 0, !isPlaying, -1, 0, 0, nChn);	//rewbs.vstiLive - added extra args
			for (UINT kplchrd=0; kplchrd<nPlayChord; kplchrd++)
			{
				if (chordplaylist[kplchrd])
				{
					pModDoc->PlayNote(chordplaylist[kplchrd], nPlayIns, 0, false, -1, 0, 0, nChn);	//rewbs.vstiLive - 	- added extra args
					m_dwStatus |= psChordPlaying;
				}
			}
		}
	} // end play note


	// Set new cursor position (row spacing) - only when not recording live
	if (bRecordEnabled && !bIsLiveRecord)
	{
		if ((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING)) 
			SetCurrentRow(GetCurrentRow() + m_nSpacing);

		SetSelToCursor();
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

	PrepareUndo(m_Cursor, m_Cursor);

	ModCommand *p = pSndFile->Patterns[m_nPattern].GetpModCommand(GetCurrentRow(), GetCurrentChannel());
	ModCommand oldcmd = *p;

	if(mask.note)
	{
		// Clear note
		if(p->IsPcNote())
		{
			// Need to clear entire field if this is a PC Event.
			p->Clear();
		} else
		{
			p->note = NOTE_NONE;
			if(ITStyle)
			{
				p->instr = 0;
			}
		}
	}
	if(mask.instrument)
	{
		// Clear instrument
		p->instr = 0;
	}
	if(mask.volume)
	{
		// Clear volume effect
		p->volcmd = VOLCMD_NONE;
		p->vol = 0;
	}
	if(mask.command)
	{
		// Clear effect command
		p->command = CMD_NONE;
	}
	if(mask.parameter)
	{
		// Clear effect parameter
		p->param = 0;
	}

	if((mask.command || mask.parameter) && (p->IsPcNote()))
	{
		p->SetValueEffectCol(0);
	}

	SetSelToCursor();

	if(*p != oldcmd)
	{
		SetModified(false);
		InvalidateRow();
		UpdateIndicator();
	}

	if(step && (pSndFile->IsPaused() || !(m_dwStatus & psFollowSong) ||
		(CMainFrame::GetMainFrame() != nullptr && CMainFrame::GetMainFrame()->GetFollowSong(GetDocument()) != m_hWnd)))
	{
		if ((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING)) 
			SetCurrentRow(GetCurrentRow() + m_nSpacing);

		SetSelToCursor();
	}
}



void CViewPattern::OnInitMenu(CMenu* pMenu)
//-----------------------------------------
{
	CModScrollView::OnInitMenu(pMenu);

	//rewbs: ensure modifiers are reset when we go into menu
/*	m_dwStatus &= ~PATSTATUS_KEYDRAGSEL;	
	m_dwStatus &= ~PATSTATUS_CTRLDRAGSEL;
	CMainFrame::GetMainFrame()->GetInputHandler()->SetModifierMask(0);
*/	//end rewbs

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
	const UINT newIns = nID - ID_CHANGE_INSTRUMENT;

	if (newIns == 0)
	{
		RowMask sp(false, true, false, false, false);	// Setup mask to only clear instrument data in OnClearSelection
		OnClearSelection(false, sp);					// Clears instrument selection from pattern
	} else
	{
		SetSelectionInstrument(newIns);
	}
}


void CViewPattern::OnSelectPCNoteParam(UINT nID)
//----------------------------------------------
{
	CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
	{
		return;
	}

	UINT paramNdx = nID - ID_CHANGE_PCNOTE_PARAM;
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
	if (bModified)
	{
		SetModified();
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
		UINT newPlug = nID - ID_PLUGSELECT;
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

//rewbs.merge
bool CViewPattern::HandleSplit(ModCommand* p, int note)
//-----------------------------------------------------
{
	ModCommand::INSTR ins = GetCurrentInstrument();
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
	
	p->note = note;	
	if(ins)
	{
		p->instr = ins;
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
	AppendMenu(hMenu, (pSndFile->ChnSettings[nChn].dwFlags & CHN_MUTE) ? 
					   (MF_STRING|MF_CHECKED) : MF_STRING, ID_PATTERN_MUTE, 
					   "Mute Channel\t" + ih->GetKeyTextFromCommand(kcChannelMute));
	bool bSolo = false, bUnmuteAll = false;
	bool bSoloPending = false, bUnmuteAllPending = false; // doesn't work perfectly yet

	for (CHANNELINDEX i = 0; i < pSndFile->GetNumChannels(); i++)
	{
		if (i != nChn)
		{
			if (!(pSndFile->ChnSettings[i].dwFlags & CHN_MUTE)) bSolo = bSoloPending = true;
			if (!((~pSndFile->ChnSettings[i].dwFlags & CHN_MUTE) && pSndFile->m_bChannelMuteTogglePending[i])) bSoloPending = true;
		}
		else
		{
			if (pSndFile->ChnSettings[i].dwFlags & CHN_MUTE) bSolo = bSoloPending = true;
			if ((~pSndFile->ChnSettings[i].dwFlags & CHN_MUTE) && pSndFile->m_bChannelMuteTogglePending[i]) bSoloPending = true;
		}
		if (pSndFile->ChnSettings[i].dwFlags & CHN_MUTE) bUnmuteAll = bUnmuteAllPending = true;
		if ((~pSndFile->ChnSettings[i].dwFlags & CHN_MUTE) && pSndFile->m_bChannelMuteTogglePending[i]) bUnmuteAllPending = true;
	}
	if (bSolo) AppendMenu(hMenu, MF_STRING, ID_PATTERN_SOLO, "Solo Channel\t" + ih->GetKeyTextFromCommand(kcChannelSolo));
	if (bUnmuteAll) AppendMenu(hMenu, MF_STRING, ID_PATTERN_UNMUTEALL, "Unmute All\t" + ih->GetKeyTextFromCommand(kcChannelUnmuteAll));
	
	AppendMenu(hMenu, 
			pSndFile->m_bChannelMuteTogglePending[nChn] ? (MF_STRING|MF_CHECKED) : MF_STRING,
			 ID_PATTERN_TRANSITIONMUTE,
			(pSndFile->ChnSettings[nChn].dwFlags & CHN_MUTE) ?
			"On transition: Unmute\t" + ih->GetKeyTextFromCommand(kcToggleChanMuteOnPatTransition) :
			"On transition: Mute\t" + ih->GetKeyTextFromCommand(kcToggleChanMuteOnPatTransition));

	if (bUnmuteAllPending) AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITION_UNMUTEALL, "On transition: Unmute all\t" + ih->GetKeyTextFromCommand(kcUnmuteAllChnOnPatTransition));
	if (bSoloPending) AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITIONSOLO, "On transition: Solo\t" + ih->GetKeyTextFromCommand(kcSoloChnOnPatTransition));

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_CHNRESET, "Reset Channel\t" + ih->GetKeyTextFromCommand(kcChannelReset));
	
	return true;
}

bool CViewPattern::BuildRecordCtxMenu(HMENU hMenu, UINT nChn, CModDoc* pModDoc) const
//-----------------------------------------------------------------------------------
{
	AppendMenu(hMenu, pModDoc->IsChannelRecord1(nChn) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_EDIT_RECSELECT, "Record select");
	AppendMenu(hMenu, pModDoc->IsChannelRecord2(nChn) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_EDIT_SPLITRECSELECT, "Split Record select");
	return true;
}



bool CViewPattern::BuildRowInsDelCtxMenu(HMENU hMenu, CInputHandler *ih) const
//----------------------------------------------------------------------------
{
	const CString label = (m_Selection.GetStartRow() != m_Selection.GetEndRow() ? "Rows" : "Row");

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_INSERTROW, "Insert " + label + "\t" + ih->GetKeyTextFromCommand(kcInsertRow));
	AppendMenu(hMenu, MF_STRING, ID_PATTERN_DELETEROW, "Delete " + label + "\t" + ih->GetKeyTextFromCommand(kcDeleteRows) );
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
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECTCOLUMN, "Select Column\t" + ih->GetKeyTextFromCommand(kcSelectColumn));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECT_ALL, "Select Pattern\t" + ih->GetKeyTextFromCommand(kcEditSelectAll));
	return true;
}

bool CViewPattern::BuildGrowShrinkCtxMenu(HMENU hMenu, CInputHandler *ih) const
//-----------------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_GROW_SELECTION, "Grow selection\t" + ih->GetKeyTextFromCommand(kcPatternGrowSelection));
	AppendMenu(hMenu, MF_STRING, ID_SHRINK_SELECTION, "Shrink selection\t" + ih->GetKeyTextFromCommand(kcPatternShrinkSelection));
	return true;
}

bool CViewPattern::BuildNoteInterpolationCtxMenu(HMENU hMenu, CInputHandler *ih, CSoundFile *pSndFile) const
//----------------------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = MF_GRAYED;

	UINT startRow = m_Selection.GetStartRow();
	UINT endRow   = m_Selection.GetEndRow();
	
	if (ListChansWhereColSelected(PatternCursor::noteColumn, validChans) > 0)
	{
		for (int valChnIdx=0; valChnIdx<validChans.GetCount(); valChnIdx++)
		{
			if (IsInterpolationPossible(startRow, endRow, validChans[valChnIdx], PatternCursor::noteColumn, pSndFile))
			{
				greyed = 0;	// Can do interpolation.
				break;
			}
		}

	}
	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_INTERPOLATE_NOTE, "Interpolate Note\t" + ih->GetKeyTextFromCommand(kcPatternInterpolateNote));
		return true;
	}
	return false;
}

bool CViewPattern::BuildVolColInterpolationCtxMenu(HMENU hMenu, CInputHandler *ih, CSoundFile *pSndFile) const
//------------------------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = MF_GRAYED;

	UINT startRow = m_Selection.GetStartRow();
	UINT endRow   = m_Selection.GetEndRow();
	
	if (ListChansWhereColSelected(PatternCursor::volumeColumn, validChans) > 0)
	{
		for (int valChnIdx = 0; valChnIdx < validChans.GetCount(); valChnIdx++)
		{
			if (IsInterpolationPossible(startRow, endRow, validChans[valChnIdx], PatternCursor::volumeColumn, pSndFile))
			{
				greyed = 0;	// Can do interpolation.
				break;
			}
		}
	}
	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_INTERPOLATE_VOLUME, "Interpolate Vol Col\t" + ih->GetKeyTextFromCommand(kcPatternInterpolateVol));
		return true;
	}
	return false;
}


bool CViewPattern::BuildEffectInterpolationCtxMenu(HMENU hMenu, CInputHandler *ih, CSoundFile *pSndFile) const
//------------------------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = MF_GRAYED;

	UINT startRow = m_Selection.GetStartRow();
	UINT endRow   = m_Selection.GetEndRow();
	
	if (ListChansWhereColSelected(PatternCursor::effectColumn, validChans) > 0)
	{
		for (int valChnIdx = 0; valChnIdx < validChans.GetCount(); valChnIdx++)
		{
			if  (IsInterpolationPossible(startRow, endRow, validChans[valChnIdx], PatternCursor::effectColumn, pSndFile))
			{
				greyed = 0;	// Can do interpolation.
				break;
			}
		}
	}

	if (ListChansWhereColSelected(PatternCursor::paramColumn, validChans) > 0)
	{
		for (int valChnIdx=0; valChnIdx<validChans.GetCount(); valChnIdx++)
		{
			if  (IsInterpolationPossible(startRow, endRow, validChans[valChnIdx], PatternCursor::paramColumn, pSndFile))
			{
				greyed = 0;	// Can do interpolation.
				break;
			}
		}
	}


	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		// Figure out what we want to interpolate
		CString interpolateWhat = "Effect";
		const CSoundFile *pSndFile = GetSoundFile();
		if(pSndFile->Patterns.IsValidPat(m_nPattern) && pSndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel())->IsPcNote())
		{
			interpolateWhat = "Parameter";
		}

		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_INTERPOLATE_EFFECT, "Interpolate " + interpolateWhat + "\t" + ih->GetKeyTextFromCommand(kcPatternInterpolateEffect));
		return true;
	}
	return false;
}

bool CViewPattern::BuildEditCtxMenu(HMENU hMenu, CInputHandler *ih, CModDoc* pModDoc) const
//-----------------------------------------------------------------------------------------
{
	HMENU pasteSpecialMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_EDIT_CUT, "Cut\t" + ih->GetKeyTextFromCommand(kcEditCut));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_COPY, "Copy\t" + ih->GetKeyTextFromCommand(kcEditCopy));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_PASTE, "Paste\t" + ih->GetKeyTextFromCommand(kcEditPaste));
	AppendMenu(hMenu, MF_POPUP, (UINT)pasteSpecialMenu, "Paste Special");
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_MIXPASTE, "Mix Paste\t" + ih->GetKeyTextFromCommand(kcEditMixPaste));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_MIXPASTE_ITSTYLE, "Mix Paste (IT Style)\t" + ih->GetKeyTextFromCommand(kcEditMixPasteITStyle));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_PASTEFLOOD, "Paste Flood\t" + ih->GetKeyTextFromCommand(kcEditPasteFlood));
	AppendMenu(pasteSpecialMenu, MF_STRING, ID_EDIT_PUSHFORWARDPASTE, "Push Forward Paste (Insert)\t" + ih->GetKeyTextFromCommand(kcEditPushForwardPaste));

	DWORD greyed = pModDoc->GetPatternUndo().CanUndo() ? MF_ENABLED : MF_GRAYED;
	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_EDIT_UNDO, "Undo\t" + ih->GetKeyTextFromCommand(kcEditUndo));
	}

	AppendMenu(hMenu, MF_STRING, ID_CLEAR_SELECTION, "Clear selection\t" + ih->GetKeyTextFromCommand(kcSampleDelete));

	return true;
}

bool CViewPattern::BuildVisFXCtxMenu(HMENU hMenu, CInputHandler *ih) const
//------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(PatternCursor::effectColumn, validChans) > 0) ? FALSE:MF_GRAYED;

	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING|greyed, ID_PATTERN_VISUALIZE_EFFECT, "Visualize Effect\t" + ih->GetKeyTextFromCommand(kcPatternVisualizeEffect));
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
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(PatternCursor::noteColumn, validChans) > 0) ? FALSE:MF_GRAYED;

	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_UP, "Transpose +1\t" + ih->GetKeyTextFromCommand(kcTransposeUp));
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_DOWN, "Transpose -1\t" + ih->GetKeyTextFromCommand(kcTransposeDown));
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_OCTUP, "Transpose +12\t" + ih->GetKeyTextFromCommand(kcTransposeOctUp));
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_OCTDOWN, "Transpose -12\t" + ih->GetKeyTextFromCommand(kcTransposeOctDown));
		return true;
	}
	return false;
}

bool CViewPattern::BuildAmplifyCtxMenu(HMENU hMenu, CInputHandler *ih) const
//--------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(PatternCursor::volumeColumn, validChans) > 0) ? 0 : MF_GRAYED;

	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_AMPLIFY, "Amplify\t" + ih->GetKeyTextFromCommand(kcPatternAmplify));
		return true;
	}
	return false;
}


bool CViewPattern::BuildChannelControlCtxMenu(HMENU hMenu) const
//--------------------------------------------------------------
{
	AppendMenu(hMenu, MF_SEPARATOR, 0, "");

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_DUPLICATECHANNEL, "Duplicate this channel");

	HMENU addChannelMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP, (UINT)addChannelMenu, "Add channel\t");
	AppendMenu(addChannelMenu, MF_STRING, ID_PATTERN_ADDCHANNEL_FRONT, "Before this channel");
	AppendMenu(addChannelMenu, MF_STRING, ID_PATTERN_ADDCHANNEL_AFTER, "After this channel");
	
	HMENU removeChannelMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP, (UINT)removeChannelMenu, "Remove channel\t");
	AppendMenu(removeChannelMenu, MF_STRING, ID_PATTERN_REMOVECHANNEL, "Remove this channel\t");
	AppendMenu(removeChannelMenu, MF_STRING, ID_PATTERN_REMOVECHANNELDIALOG, "Choose channels to remove...\t");


	return false;
}


bool CViewPattern::BuildSetInstCtxMenu(HMENU hMenu, CInputHandler *ih, CSoundFile *pSndFile) const
//------------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(PatternCursor::instrColumn, validChans) > 0) ? 0 : MF_GRAYED;

	if (!greyed || !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		bool isPcNote = false;
		ModCommand *mSelStart = nullptr;
		if((pSndFile != nullptr) && (pSndFile->Patterns.IsValidPat(m_nPattern)))
		{
			mSelStart = pSndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
			if(mSelStart != nullptr && mSelStart->IsPcNote())
			{
				isPcNote = true;
			}
		}
		if(isPcNote)
			return false;

		// Create the new menu and add it to the existing menu.
		HMENU instrumentChangeMenu = ::CreatePopupMenu();
		AppendMenu(hMenu, MF_POPUP | greyed, (UINT)instrumentChangeMenu, "Change Instrument\t" + ih->GetKeyTextFromCommand(kcPatternSetInstrument));

		if(pSndFile == nullptr || pSndFile->GetpModDoc() == nullptr)
			return false;

		CModDoc* const pModDoc = pSndFile->GetpModDoc();
	
		if(!greyed)
		{
			bool addSeparator = false;
			if (pSndFile->GetNumInstruments())
			{
				for (UINT i = 1; i <= pSndFile->GetNumInstruments() ; i++)
				{
					if (pSndFile->Instruments[i] == NULL)
						continue;

					CString instString = pModDoc->GetPatternViewInstrumentName(i, true);
					if(!instString.IsEmpty())
					{
						AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + i, pModDoc->GetPatternViewInstrumentName(i));
						addSeparator = true;
					}
				}

			} else
			{
				CHAR s[64];
				for (UINT i = 1; i <= pSndFile->GetNumSamples(); i++) if (pSndFile->GetSample(i).pSample != nullptr)
				{
					wsprintf(s, "%02d: %s", i, pSndFile->m_szNames[i]);
					AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + i, s);
					addSeparator = true;
				}
			}

			// Add options to remove instrument from selection.
			if(addSeparator)
			{
				AppendMenu(instrumentChangeMenu, MF_SEPARATOR, 0, 0);
			}
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT, "Remove instrument");
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + GetCurrentInstrument(), "Set to current instrument");
		}
		return true;
	}
	return false;
}


bool CViewPattern::BuildChannelMiscCtxMenu(HMENU hMenu, CSoundFile *pSndFile) const
//---------------------------------------------------------------------------------
{
	if((pSndFile->GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT)) == 0) return false;
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, MF_STRING, ID_CHANNEL_RENAME, "Rename channel");
	return true;
}


// Context menu for Param Control notes
bool CViewPattern::BuildPCNoteCtxMenu(HMENU hMenu, CInputHandler *ih, CSoundFile *pSndFile) const
//-----------------------------------------------------------------------------------------------
{
	ModCommand *mSelStart = nullptr;
	if((pSndFile == nullptr) || (!pSndFile->Patterns.IsValidPat(m_nPattern)))
		return false;
	mSelStart = pSndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
	if((mSelStart == nullptr) || (!mSelStart->IsPcNote()))
		return false;
	
	char s[72];

	// Create sub menu for "change plugin"
	HMENU pluginChangeMenu = ::CreatePopupMenu();
	AppendMenu(hMenu, MF_POPUP, (UINT)pluginChangeMenu, "Change Plugin\t" + ih->GetKeyTextFromCommand(kcPatternSetInstrument));
	for(PLUGINDEX nPlg = 0; nPlg < MAX_MIXPLUGINS; nPlg++)
	{
		if(pSndFile->m_MixPlugins[nPlg].pMixPlugin != nullptr)
		{
			wsprintf(s, "%02d: %s", nPlg + 1, pSndFile->m_MixPlugins[nPlg].GetName());
			AppendMenu(pluginChangeMenu, MF_STRING | ((nPlg + 1) == mSelStart->instr) ? MF_CHECKED : 0, ID_CHANGE_INSTRUMENT + nPlg + 1, s);
		}
	}

	if(mSelStart->instr >= 1 && mSelStart->instr <= MAX_MIXPLUGINS)
	{
		CVstPlugin *plug = (CVstPlugin *)(pSndFile->m_MixPlugins[mSelStart->instr - 1].pMixPlugin);

		if(plug != nullptr)
		{

			// Create sub menu for "change plugin param"
			HMENU paramChangeMenu = ::CreatePopupMenu();
			AppendMenu(hMenu, MF_POPUP, (UINT)paramChangeMenu, "Change Plugin Parameter\t");

			uint16 nThisParam = mSelStart->GetValueVolCol();
			UINT nParams = plug->GetNumParameters();
			for (UINT i = 0; i < nParams; i++)
			{
				AppendMenu(paramChangeMenu, MF_STRING | (i == nThisParam) ? MF_CHECKED : 0, ID_CHANGE_PCNOTE_PARAM + i, plug->GetFormattedParamName(i));
			}
		}				

		AppendMenu(hMenu, MF_STRING, ID_PATTERN_EDIT_PCNOTE_PLUGIN, "Toggle plugin editor\t" + ih->GetKeyTextFromCommand(kcPatternEditPCNotePlugin));
	}

	return true;
}


// List all channels in which a given column type is selected.
UINT CViewPattern::ListChansWhereColSelected(PatternCursor::Columns colType, CArray<UINT, UINT> &chans) const
//-----------------------------------------------------------------------------------------------------------
{
	chans.RemoveAll();
	CHANNELINDEX startChan = m_Selection.GetStartChannel();
	CHANNELINDEX endChan   = m_Selection.GetEndChannel();

	// Check in which channels this column is selected.
	// Actually this check is only important for the first and last channel, but to keep things clean and simple, all channels are checked in the same manner.
	for(CHANNELINDEX i = startChan; i <= endChan; i++)
	{
		if(m_Selection.ContainsHorizontal(PatternCursor(0, i, colType)))
		{
			chans.Add(i);
		}
	}

	return chans.GetCount();
}


// Check if the given interpolation type is actually possible in a given channel.
bool CViewPattern::IsInterpolationPossible(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX chan,
										   PatternCursor::Columns colType, CSoundFile *pSndFile) const
//----------------------------------------------------------------------------------------------------
{
	if (startRow == endRow)
		return false;

	bool result = false;
	const ModCommand startRowMC = *pSndFile->Patterns[m_nPattern].GetpModCommand(startRow, chan);
	const ModCommand endRowMC = *pSndFile->Patterns[m_nPattern].GetpModCommand(endRow, chan);
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
				|| (ModCommand::IsNoteOrEmpty(startRowCmd) && ModCommand::IsNoteOrEmpty(endRowCmd) && !(startRowCmd == NOTE_NONE && endRowCmd == NOTE_NONE));	// Interpolate between two notes of which one may be empty
			break;

		case PatternCursor::effectColumn:
			startRowCmd = startRowMC.command;
			endRowCmd = endRowMC.command;
			result = (startRowCmd == endRowCmd && startRowCmd != CMD_NONE)		// Interpolate between two identical commands
				|| (startRowCmd != CMD_NONE && endRowCmd == CMD_NONE)			// Fill in values from the first row
				|| (startRowCmd == CMD_NONE && endRowCmd != CMD_NONE);			// Fill in values from the last row
			break;

		case PatternCursor::volumeColumn:
			startRowCmd = startRowMC.volcmd;
			endRowCmd = endRowMC.volcmd;
			result = (startRowCmd == endRowCmd && startRowCmd != VOLCMD_NONE)	// Interpolate between two identical commands
				|| (startRowCmd != VOLCMD_NONE && endRowCmd == VOLCMD_NONE)		// Fill in values from the first row
				|| (startRowCmd == VOLCMD_NONE && endRowCmd != VOLCMD_NONE);	// Fill in values from the last row
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
		GetSoundFile()->GetPlaybackEventer().PatternTransitionChnUnmuteAll();
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
		GetSoundFile()->GetPlaybackEventer().PatternTranstionChnSolo(nChn);
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

	HMENU hMenu;

	if ( (hMenu = ::CreatePopupMenu()) == NULL) return false;
	
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
	ORDERINDEX currentOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
	if(pSndFile->Order[currentOrder] == m_nPattern)
	{
		const double t = pSndFile->GetPlaybackTimeAt(currentOrder, GetCurrentRow(), false);
		if(t < 0)
			msg.Format("Unable to determine the time. Possible cause: No order %d, row %d found from play sequence.", currentOrder, GetCurrentRow());
		else
		{
			const uint32 minutes = static_cast<uint32>(t / 60.0);
			const float seconds = t - minutes*60;
			msg.Format("Estimate for playback time at order %d (pattern %d), row %d: %d minute%s %.2f seconds.", currentOrder, m_nPattern, GetCurrentRow(), minutes, (minutes == 1) ? "" : "s", seconds);
		}
	}
	else
		msg.Format("Unable to determine the time: pattern at current order (%d) does not correspond to pattern at pattern view (pattern %d).", currentOrder, m_nPattern);
	
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


// Set a channel's name
void CViewPattern::OnRenameChannel()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return;

	const CHANNELINDEX nChn = m_MenuCursor.GetChannel();
	CChannelRenameDlg dlg(this, pSndFile->ChnSettings[nChn].szName, nChn + 1);
	if(dlg.DoModal() != IDOK || dlg.bChanged == false) return;

	// Backup old name.
	pModDoc->GetPatternUndo().PrepareUndo(m_nPattern, 0, 0, 1, 1, false, true);

	strcpy(pSndFile->ChnSettings[nChn].szName, dlg.m_sName);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS);
}


// Set up split keyboard
void CViewPattern::SetSplitKeyboardSettings()
//-------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return;

	CSplitKeyboadSettings dlg(CMainFrame::GetMainFrame(), pSndFile, pModDoc->GetSplitKeyboardSettings());
	if (dlg.DoModal() == IDOK)
		pModDoc->UpdateAllViews(NULL, HINT_INSNAMES | HINT_SMPNAMES);
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
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
		return;

	ModCommand *mSelStart = pSndFile->Patterns[m_nPattern].GetpModCommand(m_Selection.GetStartRow(), m_Selection.GetStartChannel());
	if(!mSelStart->IsPcNote() || mSelStart->instr < 1 || mSelStart->instr > MAX_MIXPLUGINS)
		return;

	PLUGINDEX nPlg = (PLUGINDEX)(mSelStart->instr - 1);
	pModDoc->TogglePluginEditor(nPlg);
}


// Get the active pattern's rows per beat, or, if they are not overriden, the song's default rows per beat.
ROWINDEX CViewPattern::GetRowsPerBeat() const
//-------------------------------------------
{
	const CSoundFile *pSndFile = GetSoundFile();
	if(pSndFile == nullptr)
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
	if(pSndFile == nullptr)
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
	if(nIns == 0 || pSndFile == nullptr || !pSndFile->Patterns.IsValidPat(m_nPattern))
	{
		return;
	}

	bool modified = false;

	BeginWaitCursor();
	PrepareUndo(m_Selection);

	//rewbs: re-written to work regardless of selection
	ROWINDEX startRow  = m_Selection.GetStartRow();
	ROWINDEX endRow    = m_Selection.GetEndRow();
	CHANNELINDEX startChan = m_Selection.GetStartChannel();
	CHANNELINDEX endChan   = m_Selection.GetEndChannel();

	for (ROWINDEX r = startRow; r <= endRow; r++)
	{
		ModCommand *p = pSndFile->Patterns[m_nPattern].GetpModCommand(r, startChan);
		for (CHANNELINDEX c = startChan; c <= endChan; c++, p++)
		{
			// If a note or an instr is present on the row, do the change, if required.
			// Do not set instr if note and instr are both blank,
			// but set instr if note is a PC note and instr is blank.
			if ((p->IsNote() || p->IsPcNote() || p->instr) && (p->instr != nIns))
			{
				p->instr = nIns;
				modified = true;
			}
		}
	}

	if (modified)
	{
		SetModified();
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
	}
	else
	{
		// Some arbitrary selection: Remember start / end column
		startColumn = m_Selection.GetStartColumn();
		endColumn = m_Selection.GetEndColumn();
	}

	SetCurSel(PatternCursor(startRow, startChannel, startColumn), PatternCursor(endRow, endChannel, endColumn));
}


// Copy to clipboard
bool CViewPattern::CopyPattern(PATTERNINDEX nPattern, const PatternRect &selection)
//---------------------------------------------------------------------------------
{
	BeginWaitCursor();
	bool result = GetDocument()->GetPatternClipboard().Copy(nPattern, selection);
	EndWaitCursor();
	return result;
}


// Paste from clipboard
bool CViewPattern::PastePattern(PATTERNINDEX nPattern, const PatternCursor &pastePos, PatternClipboard::PasteModes mode)
//----------------------------------------------------------------------------------------------------------------------
{
	BeginWaitCursor();
	bool result = GetDocument()->GetPatternClipboard().Paste(nPattern, pastePos, mode);
	EndWaitCursor();

	if(result)
	{
		GetDocument()->SetModified();
		GetDocument()->UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << HINT_SHIFT_PAT), NULL);
	}

	return result;
}