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
#include "OpenGLEditor.h"		//rewbs.fxvis
#include "PatternGotoDialog.h"
#include "PatternRandomizer.h"
#include "arrayutils.h"
#include "view_pat.h"
#include "View_gen.h"
#include "misc_util.h"
#include "midi.h"
#include <cmath>

#define	PLUGNAME_HEIGHT	16	//rewbs.patPlugName

#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

FindReplaceStruct CViewPattern::m_findReplace =
{
	{0,0,0,0,0,0}, {0,0,0,0,0,0},
	PATSEARCH_FULLSEARCH, PATSEARCH_REPLACEALL,
	0, 0,
	0,
	0, 0,
};

MODCOMMAND CViewPattern::m_cmdOld = {0,0,0,0,0,0};

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
	ON_COMMAND_RANGE(ID_CHANGE_PCNOTE_PARAM, ID_CHANGE_PCNOTE_PARAM + MODCOMMAND::maxColumnValue, OnSelectPCNoteParam)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO,			OnUpdateUndo)
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT+MAX_MIXPLUGINS, OnSelectPlugin) //rewbs.patPlugName


	//}}AFX_MSG_MAP
	ON_WM_INITMENU()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

static_assert(MODCOMMAND::maxColumnValue <= 999, "Command range for ID_CHANGE_PCNOTE_PARAM is designed for 999");

CViewPattern::CViewPattern()
//--------------------------
{
	m_pOpenGLEditor = NULL; //rewbs.fxvis
	m_pEffectVis = NULL; //rewbs.fxvis
	m_pRandomizer = NULL;
	m_bLastNoteEntryBlocked=false;

	m_nMenuOnChan = 0;
	m_nPattern = 0;
	m_nDetailLevel = 4;
	m_pEditWnd = NULL;
	m_pGotoWnd = NULL;
	m_Dib.Init(CMainFrame::bmpNotes);
	UpdateColors();
}

void CViewPattern::OnInitialUpdate()
//----------------------------------
{
	memset(ChnVUMeters, 0, sizeof(ChnVUMeters));
	memset(OldVUMeters, 0, sizeof(OldVUMeters));
// -> CODE#0012
// -> DESC="midi keyboard split"
	memset(splitActiveNoteChannel, 0xFF, sizeof(splitActiveNoteChannel));
	memset(activeNoteChannel, 0xFF, sizeof(activeNoteChannel));
	oldrow = -1;
	oldchn = -1;
	oldsplitchn = -1;
	m_nPlayPat = 0xFFFF;
	m_nPlayRow = 0;
	m_nMidRow = 0;
	m_nDragItem = 0;
	m_bDragging = false;
	m_bInItemRect = false;
	m_bContinueSearch = false;
	m_dwBeginSel = m_dwEndSel = m_dwCursor = m_dwStartSel = m_dwDragPos = 0;
	//m_dwStatus = 0;
	m_dwStatus = PATSTATUS_PLUGNAMESINHEADERS;
	m_nXScroll = m_nYScroll = 0;
	m_nPattern = m_nRow = 0;
	m_nSpacing = 0;
	m_nAccelChar = 0;
// -> CODE#0018
// -> DESC="route PC keyboard inputs to midi in mechanism"
	ignorekey = 0;
// -! BEHAVIOUR_CHANGE#0018
	CScrollView::OnInitialUpdate();
	UpdateSizes();
	UpdateScrollSize();
	SetCurrentPattern(0);
	m_nFoundInstrument = 0;
	m_nLastPlayedRow = 0;
	m_nLastPlayedOrder = 0;
}


BOOL CViewPattern::SetCurrentPattern(UINT npat, int nrow)
//-------------------------------------------------------
{
	CSoundFile *pSndFile;
	CModDoc *pModDoc = GetDocument();
	bool bUpdateScroll = false;
	pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;
	
	if ( (!pModDoc) || (!pSndFile) || (npat >= pSndFile->Patterns.Size()) ) return FALSE;
	if ((m_pEditWnd) && (m_pEditWnd->IsWindowVisible())) m_pEditWnd->ShowWindow(SW_HIDE);
	
	if ((npat + 1 < pSndFile->Patterns.Size()) && (!pSndFile->Patterns[npat])) npat = 0;
	while ((npat > 0) && (!pSndFile->Patterns[npat])) npat--;
	if (!pSndFile->Patterns[npat])
	{
		// Changed behaviour here. Previously, an empty pattern was inserted and the user most likely didn't notice that. Now, we just return an error.
		//pSndFile->Patterns.Insert(npat, 64);
		return FALSE;
	}
	
	m_nPattern = npat;
	if ((nrow >= 0) && (nrow != (int)m_nRow) && (nrow < (int)pSndFile->Patterns[m_nPattern].GetNumRows()))
	{
		m_nRow = nrow;
		bUpdateScroll = true;
	}
	if (m_nRow >= pSndFile->Patterns[m_nPattern].GetNumRows()) m_nRow = 0;
	int sel = CreateCursor(m_nRow) | m_dwCursor;
	SetCurSel(sel, sel);
	UpdateSizes();
	UpdateScrollSize();
	UpdateIndicator();

	if (m_bWholePatternFitsOnScreen) 		//rewbs.scrollFix
		SetScrollPos(SB_VERT, 0);
	else if (bUpdateScroll) //rewbs.fix3147
		SetScrollPos(SB_VERT, m_nRow * GetColumnHeight());

	UpdateScrollPos();
	InvalidatePattern(TRUE);
	SendCtrlMessage(CTRLMSG_PATTERNCHANGED, m_nPattern);

	return TRUE;
}


// This should be used instead of consecutive calls to SetCurrentRow() then SetCurrentColumn()
BOOL CViewPattern::SetCursorPosition(UINT nrow, UINT ncol, BOOL bWrap)
//--------------------------------------------------------------------------
{
	// Set row, but do not update scroll position yet
	// as there is another position update on the way:
	SetCurrentRow(nrow, bWrap, false);
	// Now set column and update scroll position:
	SetCurrentColumn(ncol);
	return TRUE; 
}


BOOL CViewPattern::SetCurrentRow(UINT row, BOOL bWrap, BOOL bUpdateHorizontalScrollbar)
//-------------------------------------------------------------------------------------
{
	CSoundFile *pSndFile;
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return FALSE;
	pSndFile = pModDoc->GetSoundFile();

	if ((bWrap) && (pSndFile->Patterns[m_nPattern].GetNumRows()))
	{
		if ((int)row < (int)0)
		{
			if (m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_MOUSEDRAGSEL))
			{
				row = 0;
			} else
			if (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL)
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
			if (CMainFrame::m_dwPatternSetup & PATTERN_WRAP)
			{
				if ((int)row < (int)0) row += pSndFile->Patterns[m_nPattern].GetNumRows();
				row %= pSndFile->Patterns[m_nPattern].GetNumRows();
			}
		} else //row >= 0
		if (row >= pSndFile->Patterns[m_nPattern].GetNumRows())
		{
			if (m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_MOUSEDRAGSEL))
			{
				row = pSndFile->Patterns[m_nPattern].GetNumRows()-1;
			} else
			if (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL)
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
			if (CMainFrame::m_dwPatternSetup & PATTERN_WRAP)
			{
				row %= pSndFile->Patterns[m_nPattern].GetNumRows();
			}
		}
	}

	//rewbs.fix3168
	if ( (static_cast<int>(row)<0) && !(CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL))
		row = 0;
	if (row >= pSndFile->Patterns[m_nPattern].GetNumRows() && !(CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL))
		row = pSndFile->Patterns[m_nPattern].GetNumRows()-1;
	//end rewbs.fix3168

	if ((row >= pSndFile->Patterns[m_nPattern].GetNumRows()) || (!m_szCell.cy)) return FALSE;
	// Fix: If cursor isn't on screen move both scrollbars to make it visible
	InvalidateRow();
	m_nRow = row;
	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	UpdateScrollbarPositions(bUpdateHorizontalScrollbar);
	InvalidateRow();
	int sel = CreateCursor(m_nRow) | m_dwCursor;
	int sel0 = sel;
	if ((m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_MOUSEDRAGSEL))
	 && (!(m_dwStatus & PATSTATUS_DRAGNDROPEDIT))) sel0 = m_dwStartSel;
	SetCurSel(sel0, sel);
	UpdateIndicator();

	Log("Row: %d; Chan: %d; ColType: %d; cursor&0xFFFF: %x; cursor>>16: %x;\n",
		GetRowFromCursor(sel0),
		GetChanFromCursor(sel0),
		GetColTypeFromCursor(sel0),
		(int)(sel0&0xFFFF),
		(int)(sel0>>16));


	return TRUE;
}


BOOL CViewPattern::SetCurrentColumn(UINT ncol)
//--------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	if (!pModDoc) return FALSE;
	pSndFile = pModDoc->GetSoundFile();
	if ((ncol & 0x07) == 7)
	{
		ncol = (ncol & ~0x07) + m_nDetailLevel;
	} else
	if ((ncol & 0x07) > m_nDetailLevel)
	{
		ncol += (((ncol & 0x07) - m_nDetailLevel) << 3) - (m_nDetailLevel+1);
	}
	if ((ncol >> 3) >= pSndFile->GetNumChannels()) return FALSE;
	m_dwCursor = ncol;
	int sel = CreateCursor(m_nRow) | m_dwCursor;
	int sel0 = sel;
	if ((m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_MOUSEDRAGSEL))
	 && (!(m_dwStatus & PATSTATUS_DRAGNDROPEDIT))) sel0 = m_dwStartSel;
	SetCurSel(sel0, sel);
	// Fix: If cursor isn't on screen move both scrollbars to make it visible
	UpdateScrollbarPositions();
	UpdateIndicator();
	return TRUE;
}

// Fix: If cursor isn't on screen move scrollbars to make it visible
// Fix: save pattern scrollbar position when switching to other tab
// Assume that m_nRow and m_dwCursor are valid
// When we switching to other tab the CViewPattern object is deleted
// and when switching back new one is created
BOOL CViewPattern::UpdateScrollbarPositions( BOOL UpdateHorizontalScrollbar )
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
			return TRUE;
		} else
		if( scroll.cx >= -1 )
		{
			scroll.cx = -2;
			pModDoc->SetOldPatternScrollbarsPos( scroll );
			return TRUE;
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

	if( UpdateHorizontalScrollbar )
	{
		UINT ncol = GetCurrentColumn();
		UINT xofs = GetXScrollPos();
		int nchn = ncol >> 3;
		if (nchn < xofs)
		{
			scroll.cx = (int)(nchn - xofs) * m_szCell.cx;
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
	return TRUE;
}

DWORD CViewPattern::GetDragItem(CPoint point, LPRECT lpRect)
//----------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	CRect rcClient, rect, plugRect; 	//rewbs.patPlugNames
	UINT n;
	int xofs, yofs;

	if (!pModDoc) return 0;
	GetClientRect(&rcClient);
	xofs = GetXScrollPos();
	yofs = GetYScrollPos();
	rect.SetRect(m_szHeader.cx, 0, m_szHeader.cx + GetColumnWidth() /*- 2*/, m_szHeader.cy);
	plugRect.SetRect(m_szHeader.cx, m_szHeader.cy-PLUGNAME_HEIGHT, m_szHeader.cx + GetColumnWidth() - 2, m_szHeader.cy);	//rewbs.patPlugNames
	pSndFile = pModDoc->GetSoundFile();
	const UINT nmax = pSndFile->GetNumChannels();
	// Checking channel headers
	//rewbs.patPlugNames
	if (m_dwStatus & PATSTATUS_PLUGNAMESINHEADERS)
	{
		n = xofs;
		for (UINT ihdr=0; n<nmax; ihdr++, n++)
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
	if ((pSndFile->Patterns[m_nPattern]) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)))
	{
		rect.SetRect(0, 0, m_szHeader.cx, m_szHeader.cy);
		if (rect.PtInRect(point))
		{
			if (lpRect) *lpRect = rect;
			return DRAGITEM_PATTERNHEADER;
		}
	}
	return 0;
}


// Drag a selection to position dwPos.
// If bScroll is true, the point dwPos is scrolled into the view if needed.
// If bNoMode if specified, the original selection points are not altered.
// Note that scrolling will only be executed if dwPos contains legal coordinates.
// This can be useful when selecting a whole row and specifying 0xFFFF as the end channel.
BOOL CViewPattern::DragToSel(DWORD dwPos, BOOL bScroll, BOOL bNoMove)
//-------------------------------------------------------------------
{
	CRect rect;
	CSoundFile *pSndFile;
	CModDoc *pModDoc = GetDocument();
	int yofs = GetYScrollPos(), xofs = GetXScrollPos();
	int row, col;

	if ((!pModDoc) || (!m_szCell.cy)) return FALSE;
	GetClientRect(&rect);
	pSndFile = pModDoc->GetSoundFile();
	if (!bNoMove) SetCurSel(m_dwStartSel, dwPos);
	if (!bScroll) return TRUE;
	// Scroll to row
	row = GetRowFromCursor(dwPos);
	if (row < (int)pSndFile->Patterns[m_nPattern].GetNumRows())
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
	col = GetChanFromCursor(dwPos);
	if (col < (int)pSndFile->m_nChannels)
	{
		int maxcol = (rect.right - m_szHeader.cx) - 4;
		maxcol -= GetColumnOffset(GetColTypeFromCursor(dwPos));
		maxcol /= GetColumnWidth();
		if (col < xofs)
		{
			CSize size(-m_szCell.cx,0);
			if (!bNoMove) size.cx = (col - xofs) * (int)m_szCell.cx;
			OnScrollBy(size, TRUE);
		} else
		if ((col > xofs+maxcol) && (maxcol > 0))
		{
			CSize size(m_szCell.cx,0);
			if (!bNoMove) size.cx = (col - maxcol + 1) * (int)m_szCell.cx;
			OnScrollBy(size, TRUE);
		}
	}
	UpdateWindow();
	return TRUE;
}


BOOL CViewPattern::SetPlayCursor(UINT nPat, UINT nRow)
//----------------------------------------------------
{
	UINT nOldPat = m_nPlayPat, nOldRow = m_nPlayRow;
	if ((nPat == m_nPlayPat) && (nRow == m_nPlayRow)) return TRUE;
	m_nPlayPat = nPat;
	m_nPlayRow = nRow;
	if (nOldPat == m_nPattern) InvalidateRow(nOldRow);
	if (m_nPlayPat == m_nPattern) InvalidateRow(m_nPlayRow);
	return TRUE;
}


UINT CViewPattern::GetCurrentInstrument() const
//---------------------------------------------
{
	return SendCtrlMessage(CTRLMSG_GETCURRENTINSTRUMENT);
}

BOOL CViewPattern::ShowEditWindow()
//---------------------------------
{
	if (!m_pEditWnd)
	{
		m_pEditWnd = new CEditCommand;
		if (m_pEditWnd) m_pEditWnd->SetParent(this, GetDocument());
	}
	if (m_pEditWnd)
	{
		m_pEditWnd->ShowEditWindow(m_nPattern, CreateCursor(m_nRow) | m_dwCursor);
		return TRUE;
	}
	return FALSE;
}


BOOL CViewPattern::PrepareUndo(DWORD dwBegin, DWORD dwEnd)
//--------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	UINT nChnBeg, nRowBeg, nChnEnd, nRowEnd;

	nChnBeg = GetChanFromCursor(dwBegin);
	nRowBeg = GetRowFromCursor(dwBegin);
	nChnEnd = GetChanFromCursor(dwEnd);
	nRowEnd = GetRowFromCursor(dwEnd);
	if( (nChnEnd < nChnBeg) || (nRowEnd < nRowBeg) ) return FALSE;
	if (pModDoc) return pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, nChnBeg, nRowBeg, nChnEnd-nChnBeg+1, nRowEnd-nRowBeg+1);
	return FALSE;
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
			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();

			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxViewPatterns + 1 + GetColTypeFromCursor(m_dwCursor));

			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT) != kcNull) {
				return true; // Mapped to a command, no need to pass message on.
			}
			//HACK: fold kCtxViewPatternsFX and kCtxViewPatternsFXparam so that all commands of 1 are active in the other
			else {
				if (ctx==kCtxViewPatternsFX) {
					if (ih->KeyEvent(kCtxViewPatternsFXparam, nChar, nRepCnt, nFlags, kT) != kcNull) {
						return true; // Mapped to a command, no need to pass message on.
					}
				}
				if (ctx==kCtxViewPatternsFXparam) {
					if (ih->KeyEvent(kCtxViewPatternsFX, nChar, nRepCnt, nFlags, kT) != kcNull) {
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
	if (m_pEffectVis)	{
		m_pEffectVis->DoClose();
		delete m_pEffectVis;
		m_pEffectVis = NULL;
	}

	if (m_pEditWnd)	{
		m_pEditWnd->DestroyWindow();
		delete m_pEditWnd;
		m_pEditWnd = NULL;
	}

	if (m_pGotoWnd)	{
		m_pGotoWnd->DestroyWindow();
		delete m_pGotoWnd;
		m_pGotoWnd = NULL;
	}

	if (m_pRandomizer) {
		delete m_pRandomizer;
		m_pRandomizer=NULL;
	}

	if (m_pOpenGLEditor)	{
		m_pOpenGLEditor->DoClose();
		delete m_pOpenGLEditor;
		m_pOpenGLEditor = NULL;
	}


	CModScrollView::OnDestroy();
}


void CViewPattern::OnSetFocus(CWnd *pOldWnd)
//------------------------------------------
{
	CScrollView::OnSetFocus(pOldWnd);
	m_dwStatus |= PATSTATUS_FOCUS;
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
	m_dwStatus &= ~PATSTATUS_KEYDRAGSEL;
	m_dwStatus &= ~PATSTATUS_CTRLDRAGSEL;
//	CMainFrame::GetMainFrame()->GetInputHandler()->SetModifierMask(0);
	//end rewbs.customKeys

	m_dwStatus &= ~PATSTATUS_FOCUS;
	InvalidateRow();
}

//rewbs.customKeys
void CViewPattern::OnGrowSelection()
//-----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	MODCOMMAND *p = pSndFile->Patterns[m_nPattern];
	if (!p) return;
	BeginWaitCursor();

	DWORD startSel = ((m_dwBeginSel>>16)<(m_dwEndSel>>16)) ? m_dwBeginSel : m_dwEndSel;
	DWORD endSel   = ((m_dwBeginSel>>16)<(m_dwEndSel>>16)) ? m_dwEndSel : m_dwBeginSel;
	pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, 0, 0, pSndFile->m_nChannels, pSndFile->Patterns[m_nPattern].GetNumRows());

	int finalDest = GetRowFromCursor(startSel) + (GetRowFromCursor(endSel) - GetRowFromCursor(startSel))*2;
	for (int row = finalDest; row > (int)GetRowFromCursor(startSel); row -= 2)
	{
		int offset = row - GetRowFromCursor(startSel);
		for (UINT i=(startSel & 0xFFFF); i<=(endSel & 0xFFFF); i++) if (GetColTypeFromCursor(i) <= LAST_COLUMN)
		{
			UINT chn = GetChanFromCursor(i);
			if ((chn >= pSndFile->GetNumChannels()) || (row >= pSndFile->Patterns[m_nPattern].GetNumRows())) continue;
			MODCOMMAND *dest = &p[row * pSndFile->GetNumChannels() + chn];
			MODCOMMAND *src  = &p[(row-offset/2) * pSndFile->GetNumChannels() + chn];
			MODCOMMAND *blank= &p[(row-1) * pSndFile->GetNumChannels() + chn];
			//memcpy(dest/*+(i%5)*/, src/*+(i%5)*/, /*sizeof(MODCOMMAND) - (i-chn)*/ sizeof(BYTE));
			//Log("dst: %d; src: %d; blk: %d\n", row, (row-offset/2), (row-1));
			switch(GetColTypeFromCursor(i))
			{
				case NOTE_COLUMN:	dest->note    = src->note;    blank->note = NOTE_NONE;		break;
				case INST_COLUMN:	dest->instr   = src->instr;   blank->instr = 0;				break;
				case VOL_COLUMN:	dest->vol     = src->vol;     blank->vol = 0;
									dest->volcmd  = src->volcmd;  blank->volcmd = VOLCMD_NONE;	break;
				case EFFECT_COLUMN:	dest->command = src->command; blank->command = 0;			break;
				case PARAM_COLUMN:	dest->param   = src->param;   blank->param = CMD_NONE;		break;
			}
		}
	}

	m_dwBeginSel = startSel;
	m_dwEndSel   = (min(finalDest,pSndFile->Patterns[m_nPattern].GetNumRows()-1)<<16) | (endSel&0xFFFF);

	InvalidatePattern(FALSE);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
	EndWaitCursor();
	SetFocus();
}


void CViewPattern::OnShrinkSelection()
//-----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	MODCOMMAND *p = pSndFile->Patterns[m_nPattern];
	if (!p) return;
	BeginWaitCursor();

	DWORD startSel = ((m_dwBeginSel>>16)<(m_dwEndSel>>16)) ? m_dwBeginSel : m_dwEndSel;
	DWORD endSel   = ((m_dwBeginSel>>16)<(m_dwEndSel>>16)) ? m_dwEndSel : m_dwBeginSel;
	pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, 0, 0, pSndFile->m_nChannels, pSndFile->Patterns[m_nPattern].GetNumRows());

	int finalDest = GetRowFromCursor(startSel) + (GetRowFromCursor(endSel) - GetRowFromCursor(startSel))/2;

	for (int row = GetRowFromCursor(startSel); row <= finalDest; row++)
	{
		int offset = row - GetRowFromCursor(startSel);
		int srcRow = GetRowFromCursor(startSel) + (offset * 2);

		for (UINT i = (startSel & 0xFFFF); i <= (endSel & 0xFFFF); i++) if (GetColTypeFromCursor(i) <= LAST_COLUMN)
		{
			const CHANNELINDEX chn = GetChanFromCursor(i);
			if ((chn >= pSndFile->GetNumChannels()) || (srcRow >= pSndFile->Patterns[m_nPattern].GetNumRows())
											   || (row    >= pSndFile->Patterns[m_nPattern].GetNumRows())) continue;
			MODCOMMAND *dest = pSndFile->Patterns[m_nPattern].GetpModCommand(row, chn);
			MODCOMMAND *src = pSndFile->Patterns[m_nPattern].GetpModCommand(srcRow, chn);
			// if source command is empty, try next source row.
			if(srcRow < pSndFile->Patterns[m_nPattern].GetNumRows() - 1)
			{
				const MODCOMMAND *srcNext = pSndFile->Patterns[m_nPattern].GetpModCommand(srcRow + 1, chn);
				if(src->note == NOTE_NONE) src->note = srcNext->note;
				if(src->instr == 0) src->instr = srcNext->instr;
				if(src->volcmd == VOLCMD_NONE)
				{
					src->volcmd = srcNext->volcmd;
					src->vol = srcNext->vol;
				}
				if(src->command == CMD_NONE)
				{
					src->command = srcNext->command;
					src->param = srcNext->param;
				}
			}
			
			//memcpy(dest/*+(i%5)*/, src/*+(i%5)*/, /*sizeof(MODCOMMAND) - (i-chn)*/ sizeof(BYTE));
			Log("dst: %d; src: %d\n", row, srcRow);
			switch(GetColTypeFromCursor(i))
			{
				case NOTE_COLUMN:	dest->note    = src->note;    break;
				case INST_COLUMN:	dest->instr   = src->instr;   break;
				case VOL_COLUMN:	dest->vol     = src->vol;      
									dest->volcmd  = src->volcmd;  break;
				case EFFECT_COLUMN:	dest->command = src->command; break;
				case PARAM_COLUMN:	dest->param   = src->param;   break;
			}
		}
	}
	for (int row = finalDest + 1; row <= GetRowFromCursor(endSel); row++)
	{
		for (UINT i = (startSel & 0xFFFF); i <= (endSel & 0xFFFF); i++) if (GetColTypeFromCursor(i) <= LAST_COLUMN)
		{
			UINT chn = GetChanFromCursor(i);
			MODCOMMAND *dest = pSndFile->Patterns[m_nPattern].GetpModCommand(row, chn);
			switch(GetColTypeFromCursor(i))
			{
				case NOTE_COLUMN:	dest->note    = NOTE_NONE;		break;
				case INST_COLUMN:	dest->instr   = 0;				break;
				case VOL_COLUMN:	dest->vol     = 0;
									dest->volcmd  = VOLCMD_NONE;	break;
				case EFFECT_COLUMN:	dest->command = CMD_NONE;		break;
				case PARAM_COLUMN:	dest->param   = 0;				break;
			}
		}
	}
	m_dwBeginSel = startSel;
	m_dwEndSel   = (min(finalDest,pSndFile->Patterns[m_nPattern].GetNumRows()-1)<<16) | (endSel& 0xFFFF);

	InvalidatePattern(FALSE);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
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
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc || !(IsEditingEnabled_bmsg())) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	MODCOMMAND *p = pSndFile->Patterns[m_nPattern];
	if (!p) return;

	BeginWaitCursor();

	if(ITStyle && GetColTypeFromCursor(m_dwEndSel) == NOTE_COLUMN) m_dwEndSel += 1;
	//If selection ends to a note column, in ITStyle extending it to instrument column since the instrument data is
	//removed with note data.

	PrepareUndo(m_dwBeginSel, m_dwEndSel);
	DWORD tmp = m_dwEndSel;
	bool invalidateAllCols = false;

	for (UINT row = GetRowFromCursor(m_dwBeginSel); row <= GetRowFromCursor(m_dwEndSel); row++) { // for all selected rows
		for (UINT i=(m_dwBeginSel & 0xFFFF); i<=(m_dwEndSel & 0xFFFF); i++) if (GetColTypeFromCursor(i) <= LAST_COLUMN) { // for all selected cols

			UINT chn = GetChanFromCursor(i);
			if ((chn >= pSndFile->m_nChannels) || (row >= pSndFile->Patterns[m_nPattern].GetNumRows())) continue;
			MODCOMMAND *m = &p[row * pSndFile->m_nChannels + chn];
			switch(GetColTypeFromCursor(i))
			{
			case NOTE_COLUMN:	// Clear note
				if (rm.note)
				{
					if(m->IsPcNote())
					{  // Clear whole cell if clearing PC note
						m->Clear();
						invalidateAllCols = true;
					}
					else
					{
						m->note = NOTE_NONE;
						if (ITStyle) m->instr = 0;
					}
				}
				break;
			case INST_COLUMN: // Clear instrument
				if (rm.instrument)
					m->instr = 0;
				break;
			case VOL_COLUMN:	// Clear volume
				if (rm.volume)
					m->volcmd = m->vol = 0;
				break;
			case EFFECT_COLUMN: // Clear Command
				if (rm.command)
				{
					m->command = 0;
					if(m->IsPcNote())
					{
						m->SetValueEffectCol(0);
						invalidateAllCols = true;
					}
				}
				break;
			case PARAM_COLUMN:	// Clear Command Param
				if (rm.parameter)
				{
					m->param = 0;
					if(m->IsPcNote())
					{
						m->SetValueEffectCol(0);
						// first column => update effect column char
						if(i == (m_dwBeginSel & 0xFFFF))
						{
							m_dwBeginSel--;
						}
					}
				}
				break;
			} //end switch
		} //end selected columns
	} //end selected rows

	// If selection ends on effect command column, extent invalidation area to
	// effect param column as well.
	if (GetColTypeFromCursor(tmp) == EFFECT_COLUMN)
		tmp++;

	// If invalidation on all columns is wanted, extent invalidation area.
	if(invalidateAllCols)
		tmp += LAST_COLUMN - GetColTypeFromCursor(tmp);

	InvalidateArea(m_dwBeginSel, tmp);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
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
		pModDoc->CopyPattern(m_nPattern, m_dwBeginSel, m_dwEndSel);
		SetFocus();
	}
}


void CViewPattern::OnLButtonDown(UINT nFlags, CPoint point)
//---------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (/*(m_bDragging) ||*/ (pModDoc == nullptr) || (pModDoc->GetSoundFile() == nullptr)) return;
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
			TogglePendingMute(GetChanFromCursor(GetPositionFromPoint(point)));
		}
	}
	else if ((point.x >= m_szHeader.cx) && (point.y > m_szHeader.cy))
	{
		/*if (ih->SelectionPressed()) {
			m_dwEndSel = GetPositionFromPoint(point);
			SetCurSel(m_dwStartSel, m_dwEndSel);
			SetCurrentRow(m_dwEndSel >> 16);
			SetCurrentColumn(m_dwEndSel & 0xFFFF);
			DragToSel(m_dwEndSel, TRUE);
		} else */
		{
			if(CMainFrame::GetInputHandler()->ShiftPressed() && m_dwStartSel == m_dwEndSel)
			{
				// Shift pressed -> set 2nd selection point
				DragToSel(GetPositionFromPoint(point), TRUE);
			} else
			{
				// Set first selection point
				m_dwStartSel = GetPositionFromPoint(point);
				if (GetChanFromCursor(m_dwStartSel) < pModDoc->GetNumChannels())
				{
					m_dwStatus |= PATSTATUS_MOUSEDRAGSEL;

					if (m_dwStatus & PATSTATUS_CTRLDRAGSEL)
					{
						SetCurSel(m_dwStartSel, m_dwStartSel);
					}
					if ((CMainFrame::m_dwPatternSetup & PATTERN_DRAGNDROPEDIT)
					&& ((m_dwBeginSel != m_dwEndSel) || (m_dwStatus & PATSTATUS_CTRLDRAGSEL))
					&& (GetRowFromCursor(m_dwStartSel) >= GetRowFromCursor(m_dwBeginSel))
					&& (GetRowFromCursor(m_dwStartSel) <= GetRowFromCursor(m_dwEndSel))
					&& ((m_dwStartSel & 0xFFFF) >= (m_dwBeginSel & 0xFFFF))
					&& ((m_dwStartSel & 0xFFFF) <= (m_dwEndSel & 0xFFFF)))
					{
						m_dwStatus |= PATSTATUS_DRAGNDROPEDIT;
					} else
					if (CMainFrame::m_dwPatternSetup & PATTERN_CENTERROW)
					{
						SetCurSel(m_dwStartSel, m_dwStartSel);
					} else
					{
						// Fix: Horizontal scrollbar pos screwed when selecting with mouse
						SetCursorPosition( GetRowFromCursor(m_dwStartSel), m_dwStartSel & 0xFFFF );
					}
				}
			}
		}
	} else if ((point.x < m_szHeader.cx) && (point.y > m_szHeader.cy))
	{
		// Mark row number => mark whole row (start)
		InvalidateSelection();
		DWORD dwPoint = GetPositionFromPoint(point);
		if(GetRowFromCursor(dwPoint) < pModDoc->GetSoundFile()->Patterns[m_nPattern].GetNumRows())
		{
			m_dwBeginSel = m_dwStartSel = dwPoint;
			m_dwEndSel = m_dwBeginSel | 0xFFFF;
			SetCurSel(m_dwStartSel, m_dwEndSel);
			m_dwStatus |= PATSTATUS_SELECTROW;
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
	DWORD dwPos = GetPositionFromPoint(point);
	if ((dwPos == (CreateCursor(m_nRow) | m_dwCursor)) && (point.y >= m_szHeader.cy))
	{
		if (ShowEditWindow()) return;
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
	m_dwStatus &= ~(PATSTATUS_MOUSEDRAGSEL|PATSTATUS_SELECTROW);
	// Drag & Drop Editing
	if (m_dwStatus & PATSTATUS_DRAGNDROPEDIT)
	{
		if (m_dwStatus & PATSTATUS_DRAGNDROPPING)
		{
			OnDrawDragSel();
			m_dwStatus &= ~PATSTATUS_DRAGNDROPPING;
			OnDropSelection();
		}
		DWORD dwPos = GetPositionFromPoint(point);
		if (dwPos == m_dwStartSel)
		{
			SetCurSel(dwPos, dwPos);
		}
		SetCursor(CMainFrame::curArrow);
		m_dwStatus &= ~PATSTATUS_DRAGNDROPEDIT;
	}
	if (((m_nDragItem & DRAGITEM_MASK) != DRAGITEM_CHNHEADER)
	 && ((m_nDragItem & DRAGITEM_MASK) != DRAGITEM_PATTERNHEADER)
	 && ((m_nDragItem & DRAGITEM_MASK) != DRAGITEM_PLUGNAME))
	{
		if ((m_nMidRow) && (m_dwBeginSel == m_dwEndSel))
		{
			DWORD dwPos = m_dwBeginSel;
			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition( GetRowFromCursor(dwPos), dwPos & 0xFFFF );
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

			const bool duplicate = (nFlags & MK_SHIFT) ? true : false;
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
					pModDoc->UpdateAllViews(this, HINT_MODCHANNELS);
					pModDoc->UpdateAllViews(this, HINT_MODTYPE);
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
	if (pModDoc && pModDoc->GetSoundFile() && pModDoc->GetSoundFile()->Patterns.IsValidPat(m_nPattern))
	{
		CPatternPropertiesDlg dlg(pModDoc, m_nPattern, this);
		if (dlg.DoModal())
		{
			UpdateScrollSize();
			InvalidatePattern(TRUE);
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
	if ((!pModDoc) || (pt.x < m_szHeader.cx)) {
		return;
	}

	// Handle drag n drop
	if (m_dwStatus & PATSTATUS_DRAGNDROPEDIT)	{
		if (m_dwStatus & PATSTATUS_DRAGNDROPPING)
		{
			OnDrawDragSel();
			m_dwStatus &= ~PATSTATUS_DRAGNDROPPING;
		}
		m_dwStatus &= ~(PATSTATUS_DRAGNDROPEDIT|PATSTATUS_MOUSEDRAGSEL);
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
	m_nMenuParam = GetPositionFromPoint(pt);

	// Right-click outside selection? Reposition cursor to the new location
	if ((GetRowFromCursor(m_nMenuParam) < GetRowFromCursor(m_dwBeginSel))
	 || (GetRowFromCursor(m_nMenuParam) > GetRowFromCursor(m_dwEndSel))
	 || ((m_nMenuParam & 0xFFFF) < (m_dwBeginSel & 0xFFFF))
	 || ((m_nMenuParam & 0xFFFF) > (m_dwEndSel & 0xFFFF)))
	{
		if (pt.y > m_szHeader.cy) { //ensure we're not clicking header
			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition( GetRowFromCursor(m_nMenuParam), m_nMenuParam & 0xFFFF );
		}
	}
	const CHANNELINDEX nChn = GetChanFromCursor(m_nMenuParam);
	if ((nChn < pSndFile->GetNumChannels()) && (pSndFile->Patterns[m_nPattern]))
	{
		CString MenuText;
		CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();

		//------ Plugin Header Menu --------- :
		if ((m_dwStatus & PATSTATUS_PLUGNAMESINHEADERS) && 
			(pt.y > m_szHeader.cy-PLUGNAME_HEIGHT) && (pt.y < m_szHeader.cy)) {
			BuildPluginCtxMenu(hMenu, nChn, pSndFile);
		}
		
		//------ Channel Header Menu ---------- :
		else if (pt.y < m_szHeader.cy){
			if (ih->ShiftPressed()) {
				//Don't bring up menu if shift is pressed, else we won't get button up msg.
			} else {
				if (BuildSoloMuteCtxMenu(hMenu, ih, nChn, pSndFile))
					AppendMenu(hMenu, MF_SEPARATOR, 0, "");
				BuildRecordCtxMenu(hMenu, nChn, pModDoc);
				BuildChannelControlCtxMenu(hMenu);
				BuildChannelMiscCtxMenu(hMenu, pSndFile);
			}
		}
		
		//------ Standard Menu ---------- :
		else if ((pt.x >= m_szHeader.cx) && (pt.y >= m_szHeader.cy))	{
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
	//BOOL bItemSelected = m_bInItemRect;
	if (!pModDoc) return;

	//CSoundFile *pSndFile = pModDoc->GetSoundFile();
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

		m_bShiftDragging = (nFlags & MK_SHIFT) ? true : false;
		m_nDropItem = GetDragItem(point, &m_rcDropItem);

		const bool b = (m_nDropItem == m_nDragItem) ? true : false;
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

	if ((m_dwStatus & PATSTATUS_SELECTROW) /*&& (point.x < m_szHeader.cx)*/ && (point.y > m_szHeader.cy))
	{
		// Mark row number => mark whole row (continue)
		InvalidateSelection();
		m_dwEndSel = GetPositionFromPoint(point) | 0xFFFF;
		DragToSel(m_dwEndSel, TRUE, FALSE);
	} else if (m_dwStatus & PATSTATUS_MOUSEDRAGSEL)
	{
		CModDoc *pModDoc = GetDocument();
		DWORD dwPos = GetPositionFromPoint(point);
		if ((pModDoc) && (m_nPattern < pModDoc->GetSoundFile()->Patterns.Size()))
		{
			UINT row = GetRowFromCursor(dwPos);
			UINT max = pModDoc->GetSoundFile()->Patterns[m_nPattern].GetNumRows();
			if ((row) && (row >= max)) row = max-1;
			dwPos &= 0xFFFF;
			dwPos |= CreateCursor(row);
		}
		// Drag & Drop editing
		if (m_dwStatus & PATSTATUS_DRAGNDROPEDIT)
		{
			if (!(m_dwStatus & PATSTATUS_DRAGNDROPPING)) SetCursor(CMainFrame::curDragging);
			if ((!(m_dwStatus & PATSTATUS_DRAGNDROPPING)) || ((m_dwDragPos & ~7) != (dwPos & ~7)))
			{
				if (m_dwStatus & PATSTATUS_DRAGNDROPPING) OnDrawDragSel();
				m_dwStatus &= ~PATSTATUS_DRAGNDROPPING;
				DragToSel(dwPos, TRUE, TRUE);
				m_dwDragPos = dwPos;
				m_dwStatus |= PATSTATUS_DRAGNDROPPING;
				OnDrawDragSel();
				pModDoc->SetModified();
			}
		} else
		// Default: selection
		if (CMainFrame::m_dwPatternSetup & PATTERN_CENTERROW)
		{
			DragToSel(dwPos, TRUE);
		} else
		{
			// Fix: Horizontal scrollbar pos screwed when selecting with mouse
			SetCursorPosition( GetRowFromCursor(dwPos), dwPos & 0xFFFF );
		}
	}
}


void CViewPattern::OnEditSelectAll()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		SetCurSel(CreateCursor(0), CreateCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, LAST_COLUMN));
	}
}


void CViewPattern::OnEditSelectColumn()
//-------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		SetCurSel(CreateCursor(0, GetChanFromCursor(m_nMenuParam), 0), CreateCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, GetChanFromCursor(m_nMenuParam), LAST_COLUMN));
	}
}


void CViewPattern::OnSelectCurrentColumn()
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		DWORD dwBeginSel = CreateCursor(0, GetChanFromCursor(m_dwCursor), 0);
		DWORD dwEndSel = CreateCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, GetChanFromCursor(m_dwCursor), LAST_COLUMN);
		// If column is already selected, select the current pattern
		if ((dwBeginSel == m_dwBeginSel) && (dwEndSel == m_dwEndSel))
		{
			dwBeginSel = CreateCursor(0);
			dwEndSel = CreateCursor(pSndFile->Patterns[m_nPattern].GetNumRows() - 1, pSndFile->GetNumChannels() - 1, LAST_COLUMN);
		}
		SetCurSel(dwBeginSel, dwEndSel);
	}
}

void CViewPattern::OnChannelReset()
//---------------------------------
{
	const CHANNELINDEX nChn = GetChanFromCursor(m_nMenuParam);
	CModDoc *pModDoc = GetDocument();
	CSoundFile* pSndFile;
	if (pModDoc == 0 || (pSndFile = pModDoc->GetSoundFile()) == 0) return;

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
		const CHANNELINDEX nChn = GetChanFromCursor(current ? m_dwCursor : m_nMenuParam);
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

	const CHANNELINDEX nChn = GetChanFromCursor(current ? m_dwCursor : m_nMenuParam);
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
		UINT nNumChn = pModDoc->GetNumChannels();
		UINT nChn = GetChanFromCursor(m_nMenuParam);
		if (nChn < nNumChn)
		{
//			MultiRecordMask[nChn>>3] ^= (1 << (nChn & 7));
// -> CODE#0012
// -> DESC="midi keyboard split"
			pModDoc->Record1Channel(nChn);
// -! NEW_FEATURE#0012
			InvalidateChannelsHeaders();
		}
	}
}

// -> CODE#0012
// -> DESC="midi keyboard split"
void CViewPattern::OnSplitRecordSelect()
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc){
		UINT nNumChn = pModDoc->GetNumChannels();
		UINT nChn = GetChanFromCursor(m_nMenuParam);
		if (nChn < nNumChn){
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


void CViewPattern::DeleteRows(UINT colmin, UINT colmax, UINT nrows)
//-----------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	UINT row, maxrow;

	if (!pModDoc || !( IsEditingEnabled_bmsg() )) return;
	pSndFile = pModDoc->GetSoundFile();
	if (!pSndFile->Patterns[m_nPattern]) return;
	row = GetRowFromCursor(m_dwBeginSel);
	maxrow = pSndFile->Patterns[m_nPattern].GetNumRows();
	if (colmax >= pSndFile->m_nChannels) colmax = pSndFile->m_nChannels-1;
	if (colmin > colmax) return;
	pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, 0,0, pSndFile->m_nChannels, maxrow);
	for (UINT r=row; r<maxrow; r++)
	{
		MODCOMMAND *m = pSndFile->Patterns[m_nPattern] + r * pSndFile->m_nChannels + colmin;
		for (UINT c=colmin; c<=colmax; c++, m++)
		{
			if (r + nrows >= maxrow)
			{
				m->Clear();
			} else
			{
				*m = *(m + pSndFile->m_nChannels * nrows);
			}
		}
	}
	//rewbs.customKeys
	DWORD finalPos = CreateCursor(GetSelectionStartRow()) | (m_dwEndSel & 0xFFFF);
	SetCurSel(finalPos, finalPos);
	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	SetCursorPosition( GetRowFromCursor(finalPos), finalPos & 0xFFFF );
	//end rewbs.customKeys
	
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
	InvalidatePattern(FALSE);
}


void CViewPattern::OnDeleteRows()
//-------------------------------
{
	UINT colmin = GetChanFromCursor(m_dwBeginSel);
	UINT colmax = GetChanFromCursor(m_dwEndSel);
	UINT nrows = GetRowFromCursor(m_dwEndSel) - GetRowFromCursor(m_dwBeginSel) + 1;
	DeleteRows(colmin, colmax, nrows);
	//m_dwEndSel = (m_dwEndSel & 0x0000FFFF) | (m_dwBeginSel & 0xFFFF0000);
}


void CViewPattern::OnDeleteRowsEx()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	UINT nrows = GetRowFromCursor(m_dwEndSel) - GetRowFromCursor(m_dwBeginSel) + 1;
	DeleteRows(0, pSndFile->GetNumChannels() - 1, nrows);
	m_dwEndSel = (m_dwEndSel & 0x0000FFFF) | (m_dwBeginSel & 0xFFFF0000);
}

void CViewPattern::InsertRows(UINT colmin, UINT colmax) //rewbs.customKeys: added args
//-----------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	UINT row, maxrow;

	if (!pModDoc || !(IsEditingEnabled_bmsg())) return;
	pSndFile = pModDoc->GetSoundFile();
	if (!pSndFile->Patterns[m_nPattern]) return;
	row = GetRowFromCursor(m_dwBeginSel);
	maxrow = pSndFile->Patterns[m_nPattern].GetNumRows();
	if (colmax >= pSndFile->GetNumChannels()) colmax = pSndFile->GetNumChannels() - 1;
	if (colmin > colmax) return;
	pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, 0,0, pSndFile->GetNumChannels(), maxrow);

	for (UINT r=maxrow; r>row; )
	{
		r--;
		MODCOMMAND *m = pSndFile->Patterns[m_nPattern] + r * pSndFile->GetNumChannels() + colmin;
		for (UINT c=colmin; c<=colmax; c++, m++)
		{
			if (r <= row)
			{
				m->Clear();
			} else
			{
				*m = *(m - pSndFile->GetNumChannels());
			}
		}
	}
	InvalidatePattern(FALSE);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
}


//rewbs.customKeys
void CViewPattern::OnInsertRows()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	UINT colmin, colmax, row, maxrow;

	if (!pModDoc) return;
	pSndFile = pModDoc->GetSoundFile();
	if (!pSndFile->Patterns[m_nPattern]) return;
	row = m_nRow;
	maxrow = pSndFile->Patterns[m_nPattern].GetNumRows();
	colmin = GetChanFromCursor(m_dwBeginSel);
	colmax = GetChanFromCursor(m_dwEndSel);

	InsertRows(colmin, colmax);
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
		pageFind.m_bPatSel = (m_dwBeginSel != m_dwEndSel) ? true : false;
		pageReplace.m_nNote = m_findReplace.cmdReplace.note;
		pageReplace.m_nInstr = m_findReplace.cmdReplace.instr;
		pageReplace.m_nVolCmd = m_findReplace.cmdReplace.volcmd;
		pageReplace.m_nVol = m_findReplace.cmdReplace.vol;
		pageReplace.m_nCommand = m_findReplace.cmdReplace.command;
		pageReplace.m_nParam = m_findReplace.cmdReplace.param;
		pageReplace.m_dwFlags = m_findReplace.dwReplaceFlags;
		pageReplace.cInstrRelChange = m_findReplace.cInstrRelChange;
		if(m_dwBeginSel != m_dwEndSel)
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
			m_findReplace.dwBeginSel = m_dwBeginSel;
			m_findReplace.dwEndSel = m_dwEndSel;
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
		CSoundFile* pSndFile = pModDoc->GetSoundFile();
		UINT nCurrentOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
		UINT nCurrentChannel = GetChanFromCursor(m_dwCursor) + 1;

		m_pGotoWnd->UpdatePos(m_nRow, nCurrentChannel, m_nPattern, nCurrentOrder, pSndFile);

		if (m_pGotoWnd->DoModal() == IDOK)
		{
			//Position should be valididated.

			if (m_pGotoWnd->m_nPattern != m_nPattern)
				SetCurrentPattern(m_pGotoWnd->m_nPattern);

			if (m_pGotoWnd->m_nOrder != nCurrentOrder)
				SendCtrlMessage(CTRLMSG_SETCURRENTORDER,  m_pGotoWnd->m_nOrder);

			if (m_pGotoWnd->m_nChannel != nCurrentChannel)
				SetCurrentColumn((m_pGotoWnd->m_nChannel-1) << 3);

			if (m_pGotoWnd->m_nRow != m_nRow)
				SetCurrentRow(m_pGotoWnd->m_nRow);

			pModDoc->SetElapsedTime(m_pGotoWnd->m_nOrder, m_pGotoWnd->m_nRow);
		}
	}
	return;
}

void CViewPattern::OnEditFindNext()
//---------------------------------
{
	CHAR s[512], szFind[64];
	UINT nFound = 0, nPatStart, nPatEnd;
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	BOOL bEffectEx;

	if (!pModDoc) return;
	if (!(m_findReplace.dwFindFlags & ~PATSEARCH_FULLSEARCH))
	{
		PostMessage(WM_COMMAND, ID_EDIT_FIND);
		return;
	}
	BeginWaitCursor();
	pSndFile = pModDoc->GetSoundFile();
	nPatStart = m_nPattern;
	nPatEnd = m_nPattern+1;
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
	bEffectEx = FALSE;
	if (m_findReplace.dwFindFlags & PATSEARCH_COMMAND)
	{
		UINT fxndx = pModDoc->GetIndexFromEffect(m_findReplace.cmdFind.command, m_findReplace.cmdFind.param);
		bEffectEx = pModDoc->IsExtendedEffect(fxndx);
	}
	for (UINT nPat=nPatStart; nPat<nPatEnd; nPat++)
	{
		LPMODCOMMAND m = pSndFile->Patterns[nPat];
		DWORD len = pSndFile->m_nChannels * pSndFile->Patterns[nPat].GetNumRows();
		if ((!m) || (!len)) continue;
		UINT n = 0;
		if ((m_bContinueSearch) && (nPat == nPatStart) && (nPat == m_nPattern))
		{
			n = GetCurrentRow() * pSndFile->m_nChannels + GetCurrentChannel() + 1;
			m += n;
		}
		for (; n<len; n++, m++)
		{
			bool bFound = true, bReplace = true;

			if (m_findReplace.dwFindFlags & PATSEARCH_CHANNEL)
			{
				// limit to given channels
				const CHANNELINDEX ch = n % pSndFile->m_nChannels;
				if ((ch < m_findReplace.nFindMinChn) || (ch > m_findReplace.nFindMaxChn)) bFound = false;
			}
			if (m_findReplace.dwFindFlags & PATSEARCH_PATSELECTION)
			{
				// limit to pattern selection
				const CHANNELINDEX ch = n % pSndFile->m_nChannels;
				const ROWINDEX row = n / pSndFile->m_nChannels;
				
				if ((ch < GetChanFromCursor(m_findReplace.dwBeginSel)) || (ch > GetChanFromCursor(m_findReplace.dwEndSel))) bFound = false;
				if ((row < GetRowFromCursor(m_findReplace.dwBeginSel)) || (row > GetRowFromCursor(m_findReplace.dwEndSel))) bFound = false;
			}
			if (((m_findReplace.dwFindFlags & PATSEARCH_NOTE) && ((m->note != m_findReplace.cmdFind.note) && ((m_findReplace.cmdFind.note != CFindReplaceTab::findAny) || (!m->note) || (m->note & 0x80))))
			 || ((m_findReplace.dwFindFlags & PATSEARCH_INSTR) && (m->instr != m_findReplace.cmdFind.instr))
			 || ((m_findReplace.dwFindFlags & PATSEARCH_VOLCMD) && (m->volcmd != m_findReplace.cmdFind.volcmd))
			 || ((m_findReplace.dwFindFlags & PATSEARCH_VOLUME) && (m->vol != m_findReplace.cmdFind.vol))
			 || ((m_findReplace.dwFindFlags & PATSEARCH_COMMAND) && (m->command != m_findReplace.cmdFind.command))
			 || ((m_findReplace.dwFindFlags & PATSEARCH_PARAM) && (m->param != m_findReplace.cmdFind.param)))
			{
				bFound = false;
			} 
			else
			{
				if (((m_findReplace.dwFindFlags & (PATSEARCH_COMMAND|PATSEARCH_PARAM)) == PATSEARCH_COMMAND) && (bEffectEx))
				{
					if ((m->param & 0xF0) != (m_findReplace.cmdFind.param & 0xF0)) bFound = false;
				}

				// Ignore modcommands with PC/PCS notes when searching from volume or effect column.
				if( (m->IsPcNote())
					&&
					m_findReplace.dwFindFlags & (PATSEARCH_VOLCMD|PATSEARCH_VOLUME|PATSEARCH_COMMAND|PATSEARCH_PARAM))
				{
					bFound = false;
				}
			}
			// Found!
			if (bFound)
			{
				bool bUpdPos = true;
				if ((m_findReplace.dwReplaceFlags & (PATSEARCH_REPLACEALL|PATSEARCH_REPLACE)) == (PATSEARCH_REPLACEALL|PATSEARCH_REPLACE)) bUpdPos = false;
				nFound++;

				if (bUpdPos)
				{
					// turn off "follow song"
					m_dwStatus &= ~PATSTATUS_FOLLOWSONG;
					SendCtrlMessage(CTRLMSG_PAT_FOLLOWSONG, 0);
					// go to place of finding
					SetCurrentPattern(nPat);
					SetCurrentRow(n / pSndFile->m_nChannels);
				}

				UINT ncurs = (n % pSndFile->m_nChannels) << 3;
				if (!(m_findReplace.dwFindFlags & PATSEARCH_NOTE))
				{
					ncurs++;
					if (!(m_findReplace.dwFindFlags & PATSEARCH_INSTR))
					{
						ncurs++;
						if (!(m_findReplace.dwFindFlags & (PATSEARCH_VOLCMD|PATSEARCH_VOLUME)))
						{
							ncurs++;
							if (!(m_findReplace.dwFindFlags & PATSEARCH_COMMAND)) ncurs++;
						}
					}
				}
				if (bUpdPos)
				{
					SetCurrentColumn(ncurs);
				}
				if (!(m_findReplace.dwReplaceFlags & PATSEARCH_REPLACE)) goto EndSearch;
				if (!(m_findReplace.dwReplaceFlags & PATSEARCH_REPLACEALL))
				{
					UINT ans = MessageBox("Replace this occurrence?", "Replace", MB_YESNOCANCEL);
					if (ans == IDYES) bReplace = true; else
					if (ans == IDNO) bReplace = false; else goto EndSearch;
				}
				if (bReplace)
				{
					// Just create one logic undo step when auto-replacing all occurences.
					const bool linkUndoBuffer = (nFound > 1) && (m_findReplace.dwReplaceFlags & PATSEARCH_REPLACEALL) != 0;
					pModDoc->GetPatternUndo()->PrepareUndo(nPat, n % pSndFile->m_nChannels, n / pSndFile->m_nChannels, 1, 1, linkUndoBuffer);

					if ((m_findReplace.dwReplaceFlags & PATSEARCH_NOTE))
					{
						// -1 octave
						if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNoteMinusOctave)
						{
							if (m->note > 12) m->note -= 12;
						} else
						// +1 octave
						if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNotePlusOctave)
						{
							if (m->note <= NOTE_MAX - 12) m->note += 12;
						} else
						// Note--
						if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNoteMinusOne)
						{
							if (m->note > 1) m->note--;
						} else
						// Note++
						if (m_findReplace.cmdReplace.note == CFindReplaceTab::replaceNotePlusOne)
						{
							if (m->note < NOTE_MAX) m->note++;
						} else
						// Replace with another note
						{
							// If we're going to remove a PC Note or replace a normal note by a PC note, wipe out the complete column.
							if(m->IsPcNote() != MODCOMMAND::IsPcNote(m_findReplace.cmdReplace.note))
							{
								m->Clear();
							}
							m->note = m_findReplace.cmdReplace.note;
						}
					}
					if ((m_findReplace.dwReplaceFlags & PATSEARCH_INSTR))
					{
						// Instr--
						if (m_findReplace.cInstrRelChange == -1 && m->instr > 1)
							m->instr--;
						// Instr++
						else if (m_findReplace.cInstrRelChange == 1 && m->instr > 0 && m->instr < (MAX_INSTRUMENTS - 1))
							m->instr++;
						else m->instr = m_findReplace.cmdReplace.instr;
					}
					if ((m_findReplace.dwReplaceFlags & PATSEARCH_VOLCMD))
					{
						m->volcmd = m_findReplace.cmdReplace.volcmd;
					}
					if ((m_findReplace.dwReplaceFlags & PATSEARCH_VOLUME))
					{
						m->vol = m_findReplace.cmdReplace.vol;
					}
					if ((m_findReplace.dwReplaceFlags & PATSEARCH_COMMAND))
					{
						m->command = m_findReplace.cmdReplace.command;
					}
					if ((m_findReplace.dwReplaceFlags & PATSEARCH_PARAM))
					{
						if ((bEffectEx) && (!(m_findReplace.dwReplaceFlags & PATSEARCH_COMMAND)))
							m->param = (BYTE)((m->param & 0xF0) | (m_findReplace.cmdReplace.param & 0x0F));
						else
							m->param = m_findReplace.cmdReplace.param;
					}
					pModDoc->SetModified();
					if (bUpdPos) InvalidateRow();
				}
			}
		}
	}
EndSearch:
	if( m_findReplace.dwFindFlags & PATSEARCH_PATSELECTION)
	{
		// Restore original selection
		m_dwBeginSel = m_findReplace.dwBeginSel;
		m_dwEndSel = m_findReplace.dwEndSel;
		InvalidatePattern();
	} else if (m_findReplace.dwReplaceFlags & PATSEARCH_REPLACEALL)
	{
		InvalidatePattern();
	}
	m_bContinueSearch = true;
	EndWaitCursor();
	// Display search results
	//m_findReplace.dwReplaceFlags &= ~PATSEARCH_REPLACEALL;
	if (!nFound)
	{
		if (m_findReplace.dwFindFlags & PATSEARCH_NOTE)
		{
			wsprintf(szFind, "%s", GetNoteStr(m_findReplace.cmdFind.note));
		} else strcpy(szFind, "???");
		strcat(szFind, " ");
		if (m_findReplace.dwFindFlags & PATSEARCH_INSTR)
		{
			if (m_findReplace.cmdFind.instr)
				wsprintf(&szFind[strlen(szFind)], "%03d", m_findReplace.cmdFind.instr);
			else
				strcat(szFind, " ..");
		} else strcat(szFind, " ??");
		strcat(szFind, " ");
		if (m_findReplace.dwFindFlags & PATSEARCH_VOLCMD)
		{
			if (m_findReplace.cmdFind.volcmd)
				wsprintf(&szFind[strlen(szFind)], "%c", pSndFile->GetModSpecifications().GetVolEffectLetter(m_findReplace.cmdFind.volcmd));
			else
				strcat(szFind, ".");
		} else strcat(szFind, "?");
		if (m_findReplace.dwFindFlags & PATSEARCH_VOLUME)
		{
			wsprintf(&szFind[strlen(szFind)], "%02d", m_findReplace.cmdFind.vol);
		} else strcat(szFind, "??");
		strcat(szFind, " ");
		if (m_findReplace.dwFindFlags & PATSEARCH_COMMAND)
		{
			if (m_findReplace.cmdFind.command)
			{
				wsprintf(&szFind[strlen(szFind)], "%c", pSndFile->GetModSpecifications().GetEffectLetter(m_findReplace.cmdFind.command));
			} else
			{
				strcat(szFind, ".");
			}
		} else strcat(szFind, "?");
		if (m_findReplace.dwFindFlags & PATSEARCH_PARAM)
		{
			wsprintf(&szFind[strlen(szFind)], "%02X", m_findReplace.cmdFind.param);
		} else strcat(szFind, "??");
		wsprintf(s, "Cannot find \"%s\"", szFind);
		MessageBox(s, "Find/Replace", MB_OK | MB_ICONINFORMATION);
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

	if ((pMainFrm) && (pModDoc))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if ((!pSndFile->Patterns.IsValidPat(m_nPattern)) || (!pSndFile->Patterns[m_nPattern].GetNumRows())) return;
		BEGIN_CRITICAL();
		// Cut instruments/samples in virtual channels
		for (CHANNELINDEX i = pSndFile->GetNumChannels(); i < MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		pSndFile->LoopPattern(m_nPattern);
		pSndFile->m_nNextRow = GetCurrentRow();
		pSndFile->m_dwSongFlags &= ~SONG_PAUSED;
		pSndFile->m_dwSongFlags |= SONG_STEP;
		END_CRITICAL();
		if (pMainFrm->GetModPlaying() != pModDoc)
		{
			pMainFrm->PlayMod(pModDoc, m_hWnd, MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);
		}
		CMainFrame::EnableLowLatencyMode();
		if(autoStep)
		{
			if (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL)
				SetCurrentRow(GetCurrentRow() + 1, TRUE);
			else
				SetCurrentRow((GetCurrentRow() + 1) % pSndFile->Patterns[m_nPattern].GetNumRows(), FALSE);
		}
		SetFocus();
	}
}


void CViewPattern::OnCursorCopy()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if ((!pSndFile->Patterns[m_nPattern].GetNumRows()) || (!pSndFile->Patterns[m_nPattern])) return;
		MODCOMMAND *m = pSndFile->Patterns[m_nPattern] + m_nRow * pSndFile->GetNumChannels() + GetChanFromCursor(m_dwCursor);
		switch(GetColTypeFromCursor(m_dwCursor))
		{
		case NOTE_COLUMN:
		case INST_COLUMN:
			m_cmdOld.note = m->note;
			m_cmdOld.instr = m->instr;
			SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, m_cmdOld.instr);
			break;
		case VOL_COLUMN:
			m_cmdOld.volcmd = m->volcmd;
			m_cmdOld.vol = m->vol;
			break;
		case EFFECT_COLUMN:
		case PARAM_COLUMN:
			m_cmdOld.command = m->command;
			m_cmdOld.param = m->param;
			break;
		}
	}
}

void CViewPattern::OnCursorPaste()
//--------------------------------
{
	//rewbs.customKeys
 	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

  	if ((pModDoc) && (pMainFrm) && (IsEditingEnabled_bmsg()))
	{
		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		UINT nChn = GetChanFromCursor(m_dwCursor);
		UINT nCursor = GetColTypeFromCursor(m_dwCursor);
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODCOMMAND *p = pSndFile->Patterns[m_nPattern] +  m_nRow*pSndFile->m_nChannels + nChn;

		switch(nCursor)
		{
			case NOTE_COLUMN:	p->note = m_cmdOld.note;
			case INST_COLUMN:	p->instr = m_cmdOld.instr; break;
			case VOL_COLUMN:	p->vol = m_cmdOld.vol; p->volcmd = m_cmdOld.volcmd; break;
			case EFFECT_COLUMN:
			case PARAM_COLUMN:	p->command = m_cmdOld.command; p->param = m_cmdOld.param; break;
		}
		pModDoc->SetModified();

		if (((pMainFrm->GetFollowSong(pModDoc) != m_hWnd) || (pSndFile->IsPaused()) || (!(m_dwStatus & PATSTATUS_FOLLOWSONG))))
		{
			DWORD sel = CreateCursor(m_nRow, nChn, 0);
			InvalidateArea(sel, sel + LAST_COLUMN);
			SetCurrentRow(m_nRow + m_nSpacing);
			sel = CreateCursor(m_nRow) | m_dwCursor;
			SetCurSel(sel, sel);
		}
	}
	//end rewbs.customKeys
}


void CViewPattern::OnInterpolateVolume()
//--------------------------------------
{
	Interpolate(VOL_COLUMN);
}

void CViewPattern::OnOpenRandomizer()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc) {
		if (m_pRandomizer) {
			if (m_pRandomizer->isGUIVisible()) { //window already there, update data
				//m_pRandomizer->UpdateSelection(rowStart, rowEnd, nchn, pModDoc, m_nPattern);
			} else {
				m_pRandomizer->showGUI();
			}
		}
		else {
			//Open window & send data
			m_pRandomizer = new CPatternRandomizer(this);
			if (m_pRandomizer) {
				m_pRandomizer->showGUI();
			}
		}
	}
}

//begin rewbs.fxvis
void CViewPattern::OnVisualizeEffect()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		//CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT row0 = GetRowFromCursor(m_dwBeginSel), row1 = GetRowFromCursor(m_dwEndSel), nchn = GetChanFromCursor(m_dwBeginSel);
		if (m_pEffectVis)
		{
			//window already there, update data
			m_pEffectVis->UpdateSelection(row0, row1, nchn, pModDoc, m_nPattern);
		}
		else
		{
			//Open window & send data
			m_pEffectVis = new CEffectVis(this, row0, row1, nchn, pModDoc, m_nPattern);
			if (m_pEffectVis)
			{
				m_pEffectVis->OpenEditor(CMainFrame::GetMainFrame());
				// HACK: to get status window set up; must create clear destinction between 
				// construction, 1st draw code and all draw code.
				m_pEffectVis->OnSize(0,0,0);

			}
			/*
			m_pOpenGLEditor = new COpenGLEditor(this);
			if (m_pOpenGLEditor) {
				m_pOpenGLEditor->OpenEditor(CMainFrame::GetMainFrame());
			}
			*/

		}
	}
}
//end rewbs.fxvis


void CViewPattern::OnInterpolateEffect()
//--------------------------------------
{
	Interpolate(EFFECT_COLUMN);
}

void CViewPattern::OnInterpolateNote()
//--------------------------------------
{
	Interpolate(NOTE_COLUMN);
}


//static void CArrayUtils<UINT>::Merge(CArray<UINT,UINT>& Dest, CArray<UINT,UINT>& Src);

void CViewPattern::Interpolate(PatternColumns type)
//-------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc)
		return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();

	bool changed = false;
	CArray<UINT,UINT> validChans;

	if (type==EFFECT_COLUMN || type==PARAM_COLUMN)
	{
		CArray<UINT,UINT> moreValidChans;
        ListChansWhereColSelected(EFFECT_COLUMN, validChans);
		ListChansWhereColSelected(PARAM_COLUMN, moreValidChans);
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
		UINT row0 = GetSelectionStartRow();
		UINT row1 = GetSelectionEndRow();
		
		if (!IsInterpolationPossible(row0, row1, nchn, type, pSndFile))
			continue; //skip chans where interpolation isn't possible

		if (!changed) //ensure we save undo buffer only before any channels are interpolated
			PrepareUndo(m_dwBeginSel, m_dwEndSel);

		bool doPCinterpolation = false;

		int vsrc, vdest, vcmd = 0, verr = 0, distance;
		distance = row1 - row0;

		const MODCOMMAND srcCmd = *pSndFile->Patterns[m_nPattern].GetpModCommand(row0, nchn);
		const MODCOMMAND destCmd = *pSndFile->Patterns[m_nPattern].GetpModCommand(row1, nchn);

		MODCOMMAND::NOTE PCnote = 0;
		uint16 PCinst = 0, PCparam = 0;

		switch(type)
		{
			case NOTE_COLUMN:
				vsrc = srcCmd.note;
				vdest = destCmd.note;
				vcmd = srcCmd.instr;
				verr = (distance * 59) / NOTE_MAX;
				break;
			case VOL_COLUMN:
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
			case PARAM_COLUMN:
			case EFFECT_COLUMN:
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
				}
				else
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

		MODCOMMAND* pcmd = pSndFile->Patterns[m_nPattern].GetpModCommand(row0, nchn);

		for (UINT i=0; i<=distance; i++, pcmd += pSndFile->m_nChannels)	{

			switch(type) {
				case NOTE_COLUMN:
					if ((!pcmd->note) || (pcmd->instr == vcmd))	{
						int note = vsrc + ((vdest - vsrc) * (int)i + verr) / distance;
						pcmd->note = (BYTE)note;
						pcmd->instr = vcmd;
					}
					break;
				case VOL_COLUMN:
					if ((!pcmd->volcmd) || (pcmd->volcmd == vcmd))	{
						int vol = vsrc + ((vdest - vsrc) * (int)i + verr) / distance;
						pcmd->vol = (BYTE)vol;
						pcmd->volcmd = vcmd;
					}
					break;
				case EFFECT_COLUMN:
					if(doPCinterpolation)
					{	// With PC/PCs notes, copy PCs note and plug index to all rows where
						// effect interpolation is done if no PC note with non-zero instrument is there.
						const uint16 val = static_cast<uint16>(vsrc + ((vdest - vsrc) * (int)i + verr) / distance);
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
						if ((!pcmd->command) || (pcmd->command == vcmd))
						{
							int val = vsrc + ((vdest - vsrc) * (int)i + verr) / distance;
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

	if (changed) {
		pModDoc->SetModified();
		InvalidatePattern(FALSE);
	}
}


BOOL CViewPattern::TransposeSelection(int transp)
//-----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		const UINT row0 = GetRowFromCursor(m_dwBeginSel), row1 = GetRowFromCursor(m_dwEndSel);
		const UINT col0 = ((m_dwBeginSel & 0xFFFF)+7) >> 3, col1 = GetChanFromCursor(m_dwEndSel);
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODCOMMAND *pcmd = pSndFile->Patterns[m_nPattern];
		const MODCOMMAND::NOTE noteMin = pSndFile->GetModSpecifications().noteMin;
		const MODCOMMAND::NOTE noteMax = pSndFile->GetModSpecifications().noteMax;

		if ((!pcmd) || (col0 > col1) || (col1 >= pSndFile->m_nChannels)
		 || (row0 > row1) || (row1 >= pSndFile->Patterns[m_nPattern].GetNumRows())) return FALSE;
		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		for (UINT row=row0; row <= row1; row++)
		{
			MODCOMMAND *m = &pcmd[row * pSndFile->m_nChannels];
			for (UINT col=col0; col<=col1; col++)
			{
				int note = m[col].note;
				if ((note >= NOTE_MIN) && (note <= NOTE_MAX))
				{
					note += transp;
					if (note < noteMin) note = noteMin;
					if (note > noteMax) note = noteMax;
					m[col].note = (BYTE)note;
				}
			}
		}
		pModDoc->SetModified();
		InvalidateSelection();
		return TRUE;
	}
	return FALSE;
}


void CViewPattern::OnDropSelection()
//----------------------------------
{
	MODCOMMAND *pOldPattern, *pNewPattern;
	CModDoc *pModDoc;
	CSoundFile *pSndFile;
	int x1, y1, x2, y2, c1, c2, xc1, xc2, xmc1, xmc2, ym1, ym2;
	int dx, dy, nChannels, nRows;

	if ((pModDoc = GetDocument()) == NULL || !(IsEditingEnabled_bmsg())) return;
	pSndFile = pModDoc->GetSoundFile();
	nChannels = pSndFile->GetNumChannels();
	nRows = pSndFile->Patterns[m_nPattern].GetNumRows();
	pOldPattern = pSndFile->Patterns[m_nPattern];
	if ((nChannels < 1) || (nRows < 1) || (!pOldPattern)) return;
	dx = (int)GetChanFromCursor(m_dwDragPos) - (int)GetChanFromCursor(m_dwStartSel);
	dy = (int)GetRowFromCursor(m_dwDragPos) - (int)GetRowFromCursor(m_dwStartSel);
	if ((!dx) && (!dy)) return;
	pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, 0,0, nChannels, nRows);
	pNewPattern = CPattern::AllocatePattern(nRows, nChannels);
	if (!pNewPattern) return;
	x1 = GetChanFromCursor(m_dwBeginSel);
	y1 = GetRowFromCursor(m_dwBeginSel);
	x2 = GetChanFromCursor(m_dwEndSel);
	y2 = GetRowFromCursor(m_dwEndSel);
	c1 = GetColTypeFromCursor(m_dwBeginSel);
	c2 = GetColTypeFromCursor(m_dwEndSel);
	if (c1 > EFFECT_COLUMN) c1 = EFFECT_COLUMN;
	if (c2 > EFFECT_COLUMN) c2 = EFFECT_COLUMN;
	xc1 = x1*4+c1;
	xc2 = x2*4+c2;
	xmc1 = xc1+dx*4;
	xmc2 = xc2+dx*4;
	ym1 = y1+dy;
	ym2 = y2+dy;
	BeginWaitCursor();
	MODCOMMAND *p = pNewPattern;
	for (int y=0; y<nRows; y++)
	{
		for (int x=0; x<nChannels; x++)
		{
			for (int c=0; c<4; c++)
			{
				int xsrc=x, ysrc=y, xc=x*4+c;

				// Destination is from destination selection
				if ((xc >= xmc1) && (xc <= xmc2) && (y >= ym1) && (y <= ym2))
				{
					xsrc -= dx;
					ysrc -= dy;
				} else
				// Destination is from source rectangle (clear)
				if ((xc >= xc1) && (xc <= xc2) && (y >= y1) && (y <= y2))
				{
					if (!(m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_CTRLDRAGSEL))) xsrc = -1;
				}
				// Copy the data
				if ((xsrc >= 0) && (xsrc < nChannels) && (ysrc >= 0) && (ysrc < nRows))
				{
					MODCOMMAND *psrc = &pOldPattern[ysrc*nChannels+xsrc];
					switch(c)
					{
					case 0:	p->note = psrc->note; break;
					case 1: p->instr = psrc->instr; break;
					case 2: p->vol = psrc->vol; p->volcmd = psrc->volcmd; break;
					default: p->command = psrc->command; p->param = psrc->param; break;
					}
				}
			}
			p++;
		}
	}
	
	BEGIN_CRITICAL();
	pSndFile->Patterns[m_nPattern] = pNewPattern;
	END_CRITICAL();


	x1 += dx;
	x2 += dx;
	y1 += dy;
	y2 += dy;
	if (x1<0) { x1=0; c1=0; }
	if (x1>=nChannels) x1=nChannels-1;
	if (x2<0) x2 = 0;
	if (x2>=nChannels) { x2=nChannels-1; c2=4; }
	if (y1<0) y1=0;
	if (y1>=nRows) y1=nRows-1;
	if (y2<0) y2=0;
	if (y2>=nRows) y2=nRows-1;
	if (c2 >= 3) c2 = 4;
	// Fix: Horizontal scrollbar pos screwed when selecting with mouse
	SetCursorPosition( y1, (x1<<3)|c1 );
	SetCurSel(CreateCursor(y1, x1, c1), CreateCursor(y2, x2, c2));
	InvalidatePattern();
	CPattern::FreePattern(pOldPattern);
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
	CSoundFile* pSndFile;
	if (pModDoc == nullptr || (pSndFile = pModDoc->GetSoundFile()) == nullptr) return;

	if(pSndFile->m_nChannels <= pSndFile->GetModSpecifications().channelsMin)
	{
		CMainFrame::GetMainFrame()->MessageBox("No channel removed - channel number already at minimum.", "Remove channel", MB_OK | MB_ICONINFORMATION);
		return;
	}

	CHANNELINDEX nChn = GetChanFromCursor(m_nMenuParam);
	const bool isEmpty = pModDoc->IsChannelUnused(nChn);

	CString str;
	str.Format("Remove channel %d? This channel still contains note data!\nNote: Operation affects all patterns and has no undo", nChn + 1);
	if(isEmpty || CMainFrame::GetMainFrame()->MessageBox(str , "Remove channel", MB_YESNO | MB_ICONQUESTION) == IDYES)
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

	if(AfxMessageBox(GetStrI18N(_TEXT("This affects all patterns, proceed?")), MB_YESNO) != IDYES)
		return;

	const CHANNELINDEX nDupChn = GetChanFromCursor(m_nMenuParam);
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
		pCmdUI->Enable(pModDoc->GetPatternUndo()->CanUndo());
	}
}


void CViewPattern::OnEditUndo()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc && IsEditingEnabled_bmsg())
	{
		PATTERNINDEX nPat = pModDoc->GetPatternUndo()->Undo();
		if (nPat < pModDoc->GetSoundFile()->Patterns.Size())
		{
			pModDoc->SetModified();
			if (nPat != m_nPattern)
				SetCurrentPattern(nPat);
			else
				InvalidatePattern(TRUE);
		}
	}
}


void CViewPattern::OnPatternAmplify()
//-----------------------------------
{
	static UINT snOldAmp = 100;
	CAmpDlg dlg(this, snOldAmp, 0);
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (dlg.DoModal() == IDOK))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		const bool useVolCol = (pSndFile->GetType() != MOD_TYPE_MOD);

		BeginWaitCursor();
		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		snOldAmp = static_cast<UINT>(dlg.m_nFactor);

		if (pSndFile->Patterns[m_nPattern])
		{
			MODCOMMAND *p = pSndFile->Patterns[m_nPattern];

			CHANNELINDEX firstChannel = GetSelectionStartChan(), lastChannel = GetSelectionEndChan();
			ROWINDEX firstRow = GetSelectionStartRow(), lastRow = GetSelectionEndRow();
			firstChannel = CLAMP(firstChannel, 0, pSndFile->m_nChannels - 1);
			lastChannel = CLAMP(lastChannel, firstChannel, pSndFile->m_nChannels - 1);
			firstRow = CLAMP(firstRow, 0, pSndFile->Patterns[m_nPattern].GetNumRows() - 1);
			lastRow = CLAMP(lastRow, firstRow, pSndFile->Patterns[m_nPattern].GetNumRows() - 1);

			// adjust min/max channel if they're only partly selected (i.e. volume column or effect column (when using .MOD) is not covered)
			if(((firstChannel << 3) | (useVolCol ? 2 : 3)) < (m_dwBeginSel & 0xFFFF))
			{
				if(firstChannel >= lastChannel)
				{
					// Selection too small!
					EndWaitCursor();
					return;
				}
				firstChannel++;
			}
			if(((lastChannel << 3) | (useVolCol ? 2 : 3)) > (m_dwEndSel & 0xFFFF))
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
				MODCOMMAND *m = p + nRow * pSndFile->m_nChannels + firstChannel;
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
						if (pSndFile->m_nInstruments)
						{
							if ((nSmp <= pSndFile->m_nInstruments) && (pSndFile->Instruments[nSmp]))
							{
								nSmp = pSndFile->Instruments[nSmp]->Keyboard[m->note];
								if(!nSmp) chvol[nChn] = 64;	// hack for instruments without samples
							} else
							{
								nSmp = 0;
							}
						}
						if ((nSmp) && (nSmp <= pSndFile->m_nSamples))
						{
							chvol[nChn] = (BYTE)(pSndFile->Samples[nSmp].nVolume >> 2);
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
				MODCOMMAND *m = p + nRow * pSndFile->m_nChannels + firstChannel;
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
								m->vol = (overrideSampleVol) ? 64 : pSndFile->Samples[nSmp].nVolume >> 2;
							} else
							{
								m->command = CMD_VOLUME;
								m->param = (overrideSampleVol) ? 64 : pSndFile->Samples[nSmp].nVolume >> 2;
							}
						}
					}
					if (m->volcmd == VOLCMD_VOLUME) chvol[nChn] = (BYTE)m->vol;
					if (((dlg.m_bFadeIn) || (dlg.m_bFadeOut)) && (m->command != CMD_VOLUME) && (!m->volcmd))
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
					if ((((nChn << 3) | 3) >= (m_dwBeginSel & 0xFFFF)) && (((nChn << 3) | 3) <= (m_dwEndSel & 0xFFFF)))
					{
						if ((m->command == CMD_VOLUME) && (m->param <= 64))
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
		pModDoc->SetModified();
		InvalidateSelection();
		EndWaitCursor();
	}
}


LRESULT CViewPattern::OnPlayerNotify(MPTNOTIFICATION *pnotify)
//------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((!pModDoc) || (!pnotify)) return 0;
	if (pnotify->dwType & MPTNOTIFY_POSITION)
	{
		UINT nOrd = pnotify->nOrder;
		UINT nRow = pnotify->nRow;
		//UINT nPat = 0xFFFF;
		UINT nPat = pnotify->nPattern; //get player pattern
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		bool updateOrderList = false;

		if(m_nLastPlayedOrder != nOrd)
		{
			updateOrderList = true;
			m_nLastPlayedOrder = nOrd;
		}

		if (nRow < m_nLastPlayedRow) {
			InvalidateChannelsHeaders();
		}
		m_nLastPlayedRow = nRow;

		if (pSndFile->m_dwSongFlags & (SONG_PAUSED|SONG_STEP)) return 0;

		//rewbs.toCheck: is it safe to remove this? What if a pattern has no order?
		/*
		if (pSndFile->m_dwSongFlags & SONG_PATTERNLOOP)
		{
			nPat = pSndFile->m_nPattern;
			nOrd = 0xFFFF;
		} else
		*/
		/*
		if (nOrd < MAX_ORDERS) {
			nPat = pSndFile->Order[nOrd];
		}
		*/

		if (nOrd >= pSndFile->Order.GetLength() || pSndFile->Order[nOrd] != nPat) {
			//order doesn't correlate with pattern, so mark it as invalid
			nOrd = 0xFFFF;
		}

		if (m_pEffectVis && m_pEffectVis->m_hWnd) {
			m_pEffectVis->SetPlayCursor(nPat, nRow);
		}

		if ((m_dwStatus & (PATSTATUS_FOLLOWSONG|PATSTATUS_DRAGVSCROLL|PATSTATUS_DRAGHSCROLL|PATSTATUS_MOUSEDRAGSEL|PATSTATUS_SELECTROW)) == PATSTATUS_FOLLOWSONG)
		{
			if (nPat < pSndFile->Patterns.Size())
			{
				if (nPat != m_nPattern || updateOrderList)
				{
					if(nPat != m_nPattern) SetCurrentPattern(nPat, nRow);
					if (nOrd < pSndFile->Order.size()) SendCtrlMessage(CTRLMSG_SETCURRENTORDER, nOrd);
					updateOrderList = false;
				}
				if (nRow != m_nRow)	SetCurrentRow((nRow < pSndFile->Patterns[nPat].GetNumRows()) ? nRow : 0, FALSE, FALSE);
			}
			SetPlayCursor(0xFFFF, 0);
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

	if ((pnotify->dwType & (MPTNOTIFY_VUMETERS|MPTNOTIFY_STOP)) && (m_dwStatus & PATSTATUS_VUMETERS))
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
	//if (!m_bRecord || !pModDoc) {
	if (!IsEditingEnabled() || !pModDoc)
	{
		return 0;
	}
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if (!pSndFile)
	{
		return 0;
	}

	//Work out where to put the new data
	const UINT nChn = GetChanFromCursor(m_dwCursor);
	const bool bUsePlaybackPosition = IsLiveRecord(*pModDoc, *pSndFile);
	ROWINDEX nRow = m_nRow;
	PATTERNINDEX nPattern = m_nPattern;
	if(bUsePlaybackPosition == true)
		SetEditPos(*pSndFile, nRow, nPattern, pSndFile->m_nRow, pSndFile->m_nPattern);

	MODCOMMAND *pRow = pSndFile->Patterns[nPattern].GetpModCommand(nRow, nChn);

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
			pModDoc->GetPatternUndo()->PrepareUndo(nPattern, nChn, nRow, 1, 1);

			pRow->Set(NOTE_PCS, plugSlot + 1, paramIndex, static_cast<uint16>(pPlug->GetParameter(paramIndex) * MODCOMMAND::maxColumnValue));
			InvalidateRow(nRow);
		}
	} else if(pSndFile->GetModSpecifications().HasCommand(CMD_SMOOTHMIDI))
	{
		// Other formats: Use MIDI macros

		//Figure out which plug param (if any) is controllable using the active macro on this channel.
		long activePlugParam  = -1;
		BYTE activeMacro      = pSndFile->Chn[nChn].nActiveMacro;
		CString activeMacroString = pSndFile->m_MidiCfg.szMidiSFXExt[activeMacro];
		if (pModDoc->GetMacroType(activeMacroString) == sfx_plug)
		{
			activePlugParam = pModDoc->MacroToPlugParam(activeMacroString);
		}
		//If the wrong macro is active, see if we can find the right one.
		//If we can, activate it for this chan by writing appropriate SFx command it.
		if (activePlugParam != paramIndex)
		{ 
			int foundMacro = pModDoc->FindMacroForParam(paramIndex);
			if (foundMacro >= 0)
			{
				pSndFile->Chn[nChn].nActiveMacro = foundMacro;
				if (pRow->command == CMD_NONE || pRow->command == CMD_SMOOTHMIDI || pRow->command == CMD_MIDI) //we overwrite existing Zxx and \xx only.
				{
					pModDoc->GetPatternUndo()->PrepareUndo(nPattern, nChn, nRow, 1, 1);

					pRow->command = (pSndFile->m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) ? CMD_S3MCMDEX : CMD_MODCMDEX;;
					pRow->param = 0xF0 + (foundMacro&0x0F);
					InvalidateRow(nRow);
				}

			}
		}

		//Write the data, but we only overwrite if the command is a macro anyway.
		if (pRow->command == CMD_NONE || pRow->command == CMD_SMOOTHMIDI || pRow->command == CMD_MIDI)
		{
			pModDoc->GetPatternUndo()->PrepareUndo(nPattern, nChn, nRow, 1, 1);

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
		SetEditPos(rSf, editpos.nRow, editpos.nPat, rSf.m_nRow, rSf.m_nPattern);
	else
		{editpos.nPat = m_nPattern; editpos.nRow = m_nRow;}
	editpos.nChn = GetChanFromCursor(m_dwCursor);
	return editpos;
}


MODCOMMAND* CViewPattern::GetModCommand(CSoundFile& rSf, const ModCommandPos& pos)
//--------------------------------------------------------------------------------
{
	static MODCOMMAND m;
	if (rSf.Patterns.IsValidPat(pos.nPat) && pos.nRow < rSf.Patterns[pos.nPat].GetNumRows() && pos.nChn < rSf.GetNumChannels())
		return rSf.Patterns[pos.nPat].GetpModCommand(pos.nRow, pos.nChn);
	else
		return &m;
}


LRESULT CViewPattern::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
//--------------------------------------------------------
{
	const DWORD dwMidiData = dwMidiDataParam;
	static BYTE midivolume = 127;

	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((!pModDoc) || (!pMainFrm)) return 0;

	CSoundFile* pSndFile = pModDoc->GetSoundFile();
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
		MODCOMMAND* p = GetModCommand(*pSndFile, editpos);
		pModDoc->GetPatternUndo()->PrepareUndo(editpos.nPat, editpos.nChn, editpos.nRow, 1, 1);
		p->Set(NOTE_PCS, mappedIndex, static_cast<uint16>(paramIndex), static_cast<uint16>((paramValue * MODCOMMAND::maxColumnValue)/127));
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
			TempStopNote(nNote, ((CMainFrame::m_dwMidiSetup & MIDISETUP_RECORDNOTEOFF) != 0));
		break;

		case MIDIEVENT_NOTEON: // Note On
			nVol = ApplyVolumeRelatedMidiSettings(dwMidiData, midivolume);
			if(nVol < 0) nVol = -1;
			else nVol = (nVol + 3) / 4; //Value from [0,256] to [0,64]
			TempEnterNote(nNote, true, nVol);
			
			// continue playing as soon as MIDI notes are being received (http://forum.openmpt.org/index.php?topic=2813.0)
			if(pSndFile->IsPaused() && (CMainFrame::m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN))
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
			if((paramValue == uint8_max || pSndFile->GetType() != MOD_TYPE_MPT) && IsEditingEnabled() && (CMainFrame::m_dwMidiSetup & MIDISETUP_MIDIMACROCONTROL))
			{  
				const bool bLiveRecord = IsLiveRecord(*pModDoc, *pSndFile);
				ModCommandPos editpos = GetEditPos(*pSndFile, bLiveRecord);
				MODCOMMAND* p = GetModCommand(*pSndFile, editpos);
				if(p->command == CMD_NONE || p->command == CMD_SMOOTHMIDI || p->command == CMD_MIDI)
				{   // Write command only if there's no existing command or already a midi macro command.
					pModDoc->GetPatternUndo()->PrepareUndo(editpos.nPat, editpos.nChn, editpos.nRow, 1, 1);
					p->command = CMD_SMOOTHMIDI;
					p->param = nByte2;
					pMainFrm->ThreadSafeSetModified(pModDoc);

					// Update GUI only if not recording live.
					if(bLiveRecord == false)
						InvalidateRow(editpos.nRow);
				}
			}

		default:
			if(CMainFrame::m_dwMidiSetup & MIDISETUP_RESPONDTOPLAYCONTROLMSGS)
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

			if(CMainFrame::m_dwMidiSetup & MIDISETUP_MIDITOPLUG
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
		return (m_nPattern << 16) | (m_nRow);

	case VIEWMSG_FOLLOWSONG:
		m_dwStatus &= ~PATSTATUS_FOLLOWSONG;
		if (lParam)
		{
			CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
			CModDoc *pModDoc = GetDocument();
			m_dwStatus |= PATSTATUS_FOLLOWSONG;
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
		if (lParam) m_dwStatus |= PATSTATUS_RECORD; else m_dwStatus &= ~PATSTATUS_RECORD;
		break;

	case VIEWMSG_SETSPACING:
		m_nSpacing = lParam;
		break;

	case VIEWMSG_PATTERNPROPERTIES:
		OnPatternProperties();
		GetParentFrame()->SetActiveView(this);
		break;

	case VIEWMSG_SETVUMETERS:
		if (lParam) m_dwStatus |= PATSTATUS_VUMETERS; else m_dwStatus &= ~PATSTATUS_VUMETERS;
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(TRUE);
		break;

	case VIEWMSG_SETPLUGINNAMES:
		if (lParam) m_dwStatus |= PATSTATUS_PLUGNAMESINHEADERS; else m_dwStatus &= ~PATSTATUS_PLUGNAMESINHEADERS;
		UpdateSizes();
		UpdateScrollSize();
		InvalidatePattern(TRUE);
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
				if ((!(m_dwStatus & PATSTATUS_FOLLOWSONG))
				 || (pMainFrm->GetFollowSong(pModDoc) != m_hWnd)
				 || (pModDoc->GetSoundFile()->IsPaused()))
				{
					SetCurrentRow(m_nRow + m_nSpacing);
				}
				m_dwStatus &= ~PATSTATUS_MIDISPACINGPENDING;
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
			m_dwStatus &= ~PATSTATUS_MIDISPACINGPENDING;
		}
		break;

	case VIEWMSG_LOADSTATE:
		if (lParam)
		{
			PATTERNVIEWSTATE *pState = (PATTERNVIEWSTATE *)lParam;
			if (pState->nDetailLevel) m_nDetailLevel = pState->nDetailLevel;
			if (/*(pState->nPattern == m_nPattern) && */(pState->cbStruct == sizeof(PATTERNVIEWSTATE)))
			{
				SetCurrentPattern(pState->nPattern);
				// Fix: Horizontal scrollbar pos screwed when selecting with mouse
				SetCursorPosition( pState->nRow, pState->nCursor );
				SetCurSel(pState->dwBeginSel, pState->dwEndSel);
			}
		}
		break;

	case VIEWMSG_SAVESTATE:
		if (lParam)
		{
			PATTERNVIEWSTATE *pState = (PATTERNVIEWSTATE *)lParam;
			pState->cbStruct = sizeof(PATTERNVIEWSTATE);
			pState->nPattern = m_nPattern;
			pState->nRow = m_nRow;
			pState->nCursor = m_dwCursor;
			pState->dwBeginSel = m_dwBeginSel;
			pState->dwEndSel = m_dwEndSel;
			pState->nDetailLevel = m_nDetailLevel;
			pState->nOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER); //rewbs.playSongFromCursor
		}
		break;

	case VIEWMSG_EXPANDPATTERN:
		{
			CModDoc *pModDoc = GetDocument();
			if (pModDoc->ExpandPattern(m_nPattern)) { m_nRow *= 2; SetCurrentPattern(m_nPattern); }
		}
		break;

	case VIEWMSG_SHRINKPATTERN:
		{
			CModDoc *pModDoc = GetDocument();
			if (pModDoc->ShrinkPattern(m_nPattern)) { m_nRow /= 2; SetCurrentPattern(m_nPattern); }
		}
		break;

	case VIEWMSG_COPYPATTERN:
		{
			CModDoc *pModDoc = GetDocument();
			if (pModDoc)
			{
				CSoundFile *pSndFile = pModDoc->GetSoundFile();
				pModDoc->CopyPattern(m_nPattern, 0, ((pSndFile->Patterns[m_nPattern].GetNumRows()-1)<<16)|(pSndFile->m_nChannels<<3));
			}
		}
		break;

	case VIEWMSG_PASTEPATTERN:
		{
			CModDoc *pModDoc = GetDocument();
			if (pModDoc) pModDoc->PastePattern(m_nPattern, 0, pm_overwrite);
			InvalidatePattern();
		}
		break;

	case VIEWMSG_AMPLIFYPATTERN:
		OnPatternAmplify();
		break;

	case VIEWMSG_SETDETAIL:
		if (lParam != (LONG)m_nDetailLevel)
		{
			m_nDetailLevel = lParam;
			UpdateSizes();
			UpdateScrollSize();
			SetCurrentColumn(m_dwCursor);
			InvalidatePattern(TRUE);
		}
		break;
	case VIEWMSG_DOSCROLL: 
		{
			CPoint p(0,0);  //dummy point;
			CModScrollView::OnMouseWheel(0, lParam, p);
		}
		break;


	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}

//rewbs.customKeys
void CViewPattern::CursorJump(DWORD distance, bool direction, bool snap)
//-----------------------------------------------------------------------
{											  //up is true
	switch(snap)
	{
		case false: //no snap
			SetCurrentRow(m_nRow + ((int)(direction?-1:1))*distance, TRUE); 
			break;

		case true: //snap
			SetCurrentRow((((m_nRow+(int)(direction?-1:0))/distance)+(int)(direction?0:1))*distance, TRUE);
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
		case kcToggleChanMuteOnPatTransition: TogglePendingMute(GetChanFromCursor(m_dwCursor)); return wParam;
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
		case kcNavigateDown:	SetCurrentRow(m_nRow+1, TRUE); return wParam;
		case kcNavigateUpSelect:
		case kcNavigateUp:		SetCurrentRow(m_nRow-1, TRUE); return wParam;

		case kcNavigateDownBySpacingSelect:
		case kcNavigateDownBySpacing:	SetCurrentRow(m_nRow+m_nSpacing, TRUE); return wParam;
		case kcNavigateUpBySpacingSelect:
		case kcNavigateUpBySpacing:		SetCurrentRow(m_nRow-m_nSpacing, TRUE); return wParam;

		case kcNavigateLeftSelect:
		case kcNavigateLeft:	if ((CMainFrame::m_dwPatternSetup & PATTERN_WRAP) && (!m_dwCursor))
									SetCurrentColumn((((pSndFile->GetNumChannels() - 1) << 3) | 4));
								else
									SetCurrentColumn(m_dwCursor - 1);
								return wParam;
		case kcNavigateRightSelect:
		case kcNavigateRight:	if ((CMainFrame::m_dwPatternSetup & PATTERN_WRAP) && (m_dwCursor >= (((pSndFile->m_nChannels-1) << 3) | 4)))
									SetCurrentColumn(0);
								else
									SetCurrentColumn(m_dwCursor + 1);
								return wParam;
		case kcNavigateNextChanSelect:
		case kcNavigateNextChan: SetCurrentColumn((((GetChanFromCursor(m_dwCursor) + 1) % pSndFile->m_nChannels) << 3) | GetColTypeFromCursor(m_dwCursor)); return wParam;
		case kcNavigatePrevChanSelect:
		case kcNavigatePrevChan:{if(GetChanFromCursor(m_dwCursor) > 0)
									SetCurrentColumn((((GetChanFromCursor(m_dwCursor) - 1) % pSndFile->m_nChannels) << 3) | GetColTypeFromCursor(m_dwCursor));
								else
									SetCurrentColumn(GetColTypeFromCursor(m_dwCursor) | ((pSndFile->m_nChannels-1) << 3));
								UINT n = CreateCursor(m_nRow) | m_dwCursor;
								SetCurSel(n, n);  return wParam;}

		case kcHomeHorizontalSelect:
		case kcHomeHorizontal:	if (m_dwCursor) SetCurrentColumn(0);
								else if (m_nRow > 0) SetCurrentRow(0);
								return wParam;
		case kcHomeVerticalSelect:
		case kcHomeVertical:	if (m_nRow > 0) SetCurrentRow(0);
								else if (m_dwCursor) SetCurrentColumn(0);
								return wParam;
		case kcHomeAbsoluteSelect:
		case kcHomeAbsolute:	if (m_dwCursor) SetCurrentColumn(0); if (m_nRow > 0) SetCurrentRow(0); return wParam;

		case kcEndHorizontalSelect:
		case kcEndHorizontal:	if (m_dwCursor!=(((pSndFile->GetNumChannels() - 1) << 3) | 4)) SetCurrentColumn(((pSndFile->m_nChannels-1) << 3) | 4);
								else if (m_nRow < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								return wParam;
		case kcEndVerticalSelect:
		case kcEndVertical:		if (m_nRow < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
								else if (m_dwCursor!=(((pSndFile->GetNumChannels() - 1) << 3) | 4)) SetCurrentColumn(((pSndFile->m_nChannels-1) << 3) | 4);
								return wParam;
		case kcEndAbsoluteSelect:
		case kcEndAbsolute:		SetCurrentColumn(((pSndFile->GetNumChannels() - 1) << 3) | 4); if (m_nRow < pModDoc->GetPatternSize(m_nPattern) - 1) SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1); return wParam;

		case kcNextPattern:	{	UINT n = m_nPattern + 1;
            					while ((n < pSndFile->Patterns.Size()) && (!pSndFile->Patterns[n])) n++;
								SetCurrentPattern((n < pSndFile->Patterns.Size()) ? n : 0);
								ORDERINDEX currentOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
								ORDERINDEX newOrder = pSndFile->FindOrder(m_nPattern, currentOrder, true);
								SendCtrlMessage(CTRLMSG_SETCURRENTORDER, newOrder);
								return wParam; }
		case kcPrevPattern: {	UINT n = (m_nPattern) ? m_nPattern - 1 : pSndFile->Patterns.Size()-1;
								while ((n > 0) && (!pSndFile->Patterns[n])) n--;
								SetCurrentPattern(n);
								ORDERINDEX currentOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
								ORDERINDEX newOrder = pSndFile->FindOrder(m_nPattern, currentOrder, false);
								SendCtrlMessage(CTRLMSG_SETCURRENTORDER, newOrder);
								return wParam; }
		case kcSelectWithCopySelect:
		case kcSelectWithNav:
		case kcSelect:			if (!(m_dwStatus & (PATSTATUS_DRAGNDROPEDIT|PATSTATUS_SELECTROW))) m_dwStartSel = CreateCursor(m_nRow) | m_dwCursor;
									m_dwStatus |= PATSTATUS_KEYDRAGSEL;	return wParam;
		case kcSelectOffWithCopySelect:
		case kcSelectOffWithNav:
		case kcSelectOff:		m_dwStatus &= ~PATSTATUS_KEYDRAGSEL; return wParam;
		case kcCopySelectWithSelect:
		case kcCopySelectWithNav:
		case kcCopySelect:		if (!(m_dwStatus & (PATSTATUS_DRAGNDROPEDIT|PATSTATUS_SELECTROW))) m_dwStartSel = CreateCursor(m_nRow) | m_dwCursor;
									m_dwStatus |= PATSTATUS_CTRLDRAGSEL; return wParam;
		case kcCopySelectOffWithSelect:
		case kcCopySelectOffWithNav:
		case kcCopySelectOff:	m_dwStatus &= ~PATSTATUS_CTRLDRAGSEL; return wParam;

		case kcSelectBeat:
		case kcSelectMeasure:
			SelectBeatOrMeasure(wParam == kcSelectBeat); return wParam;

		case kcClearRow:		OnClearField(-1, false);	return wParam;
		case kcClearField:		OnClearField(GetColTypeFromCursor(m_dwCursor), false);	return wParam;
		case kcClearFieldITStyle: OnClearField(GetColTypeFromCursor(m_dwCursor), false, true);	return wParam;
		case kcClearRowStep:	OnClearField(-1, true);	return wParam;
		case kcClearFieldStep:	OnClearField(GetColTypeFromCursor(m_dwCursor), true);	return wParam;
		case kcClearFieldStepITStyle:	OnClearField(GetColTypeFromCursor(m_dwCursor), true, true);	return wParam;
		case kcDeleteRows:		OnDeleteRows(); return wParam;
		case kcDeleteAllRows:	DeleteRows(0, pSndFile->GetNumChannels() - 1, 1); return wParam;
		case kcInsertRow:		OnInsertRows(); return wParam;
		case kcInsertAllRows:	InsertRows(0, pSndFile->GetNumChannels() - 1); return wParam;

		case kcShowNoteProperties: ShowEditWindow(); return wParam;
		case kcShowPatternProperties: OnPatternProperties(); return wParam;
		case kcShowMacroConfig:	SendCtrlMessage(CTRLMSG_SETUPMACROS); return wParam;
		case kcShowSplitKeyboardSettings:	SetSplitKeyboardSettings(); return wParam;
		case kcShowEditMenu:	{CPoint pt =	GetPointFromPosition(CreateCursor(m_nRow) | m_dwCursor);
								OnRButtonDown(0, pt); }
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
								int sel = CreateCursor(m_nRow) | m_dwCursor;
								SetCurSel(sel, sel);
								return wParam;
								}
		case kcEditPaste:		OnEditPaste(); return wParam;
		case kcEditMixPaste:	OnEditMixPaste(); return wParam;
		case kcEditMixPasteITStyle:	OnEditMixPasteITStyle(); return wParam;
		case kcEditPasteFlood:	OnEditPasteFlood(); return wParam;
		case kcEditPushForwardPaste: OnEditPushForwardPaste(); return wParam;
		case kcEditSelectAll:	OnEditSelectAll(); return wParam;
		case kcTogglePluginEditor: TogglePluginEditor(GetChanFromCursor(m_dwCursor)); return wParam;
		case kcToggleFollowSong: SendCtrlMessage(CTRLMSG_PAT_FOLLOWSONG, 1); return wParam;
		case kcChangeLoopStatus: SendCtrlMessage(CTRLMSG_PAT_LOOP, -1); return wParam;
		case kcNewPattern:		 SendCtrlMessage(CTRLMSG_PAT_NEWPATTERN); return wParam;
		case kcDuplicatePattern: SendCtrlMessage(CTRLMSG_PAT_DUPPATTERN); return wParam;
		case kcSwitchToOrderList: OnSwitchToOrderList();
		case kcSwitchOverflowPaste:	CMainFrame::m_dwPatternSetup ^= PATTERN_OVERFLOWPASTE; return wParam;
		case kcPatternEditPCNotePlugin: OnTogglePCNotePluginEditor(); return wParam;

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
			uint16 val = p->Get##method##(); \
			/* Move existing digits to left, drop out leftmost digit and */ \
			/* push new digit to the least meaning digit. */ \
			val = (val % 100) * 10 + v; \
			if(val > MODCOMMAND::maxColumnValue) val = MODCOMMAND::maxColumnValue; \
			p->Set##method##(val); \
		} \
	} 


// Enter volume effect / number in the pattern.
void CViewPattern::TempEnterVol(int v)
//------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (pMainFrm))
	{

		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		
		PrepareUndo(m_dwBeginSel, m_dwEndSel);

		MODCOMMAND* p = pSndFile->Patterns[m_nPattern].GetpModCommand(m_nRow, GetChanFromCursor(m_dwCursor));
		MODCOMMAND oldcmd = *p; // This is the command we are about to overwrite

		if(p->IsPcNote())
		{
			ENTER_PCNOTE_VALUE(v, ValueVolCol);
		}
		else
		{
			UINT volcmd = p->volcmd;
			UINT vol = p->vol;
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
			//if ((pSndFile->m_nType & MOD_TYPE_MOD) && (volcmd > VOLCMD_PANNING)) volcmd = vol = 0;

			UINT max = 64;
			if (volcmd > VOLCMD_PANNING)
			{
				max = (pSndFile->m_nType == MOD_TYPE_XM) ? 0x0F : 9;
			}

			if (vol > max) vol %= 10;
			if(pSndFile->GetModSpecifications().HasVolCommand(volcmd))
			{
				p->volcmd = volcmd;
				p->vol = vol;
			}
		}

		if (IsEditingEnabled_bmsg())
		{
			DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
			SetCurSel(sel, sel);
			sel &= ~7;
			if(oldcmd != *p)
			{
				pModDoc->SetModified();
				InvalidateArea(sel, sel + LAST_COLUMN);
				UpdateIndicator();
			}
		}
		else
		{
			// recording disabled
			*p = oldcmd;
		}
	}
}

void CViewPattern::SetSpacing(int n)
//----------------------------------
{
	if (n != m_nSpacing)
	{
		m_nSpacing = n;
		PostCtrlMessage(CTRLMSG_SETSPACING, m_nSpacing);
	}
}


// Enter an effect letter in the pattenr
void CViewPattern::TempEnterFX(int c, int v)
//------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if(!IsEditingEnabled_bmsg()) return;

	if ((pModDoc) && (pMainFrm))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();	
		
		MODCOMMAND *p = pSndFile->Patterns[m_nPattern].GetpModCommand(m_nRow, GetChanFromCursor(m_dwCursor));
		MODCOMMAND oldcmd = *p; // This is the command we are about to overwrite

		PrepareUndo(m_dwBeginSel, m_dwEndSel);

		if(p->IsPcNote())
		{
			ENTER_PCNOTE_VALUE(c, ValueEffectCol);
		}
		else if(pSndFile->GetModSpecifications().HasCommand(c))
		{

			if (c)
			{
				if ((c == m_cmdOld.command) && (!p->param) && (!p->command)) p->param = m_cmdOld.param;
				else m_cmdOld.param = 0;
				m_cmdOld.command = c;
			}
			p->command = c;
			if(v >= 0)
			{
				p->param = v;
			}

			// Check for MOD/XM Speed/Tempo command
			if ((pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
			&& ((p->command == CMD_SPEED) || (p->command == CMD_TEMPO)))
			{
				p->command = (p->param <= pSndFile->GetModSpecifications().speedMax) ? CMD_SPEED : CMD_TEMPO;
			}
		}

		DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
		SetCurSel(sel, sel);
		sel &= ~7;
		if(oldcmd != *p)
		{
			pModDoc->SetModified();
			InvalidateArea(sel, sel + LAST_COLUMN);
			UpdateIndicator();
		}
	}	// end if mainframe & moddoc exist
}


// Enter an effect param in the pattenr
void CViewPattern::TempEnterFXparam(int v)
//----------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (pMainFrm) && (IsEditingEnabled_bmsg()))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		
		MODCOMMAND *p = pSndFile->Patterns[m_nPattern].GetpModCommand(m_nRow, GetChanFromCursor(m_dwCursor));
		MODCOMMAND oldcmd = *p; // This is the command we are about to overwrite

		PrepareUndo(m_dwBeginSel, m_dwEndSel);

		if(p->IsPcNote())
		{
			ENTER_PCNOTE_VALUE(v, ValueEffectCol);
		}
		else
		{

			p->param = (p->param << 4) | v;
			if (p->command == m_cmdOld.command) m_cmdOld.param = p->param;

			// Check for MOD/XM Speed/Tempo command
			if ((pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
			 && ((p->command == CMD_SPEED) || (p->command == CMD_TEMPO)))
			{
				p->command = (p->param <= pSndFile->GetModSpecifications().speedMax) ? CMD_SPEED : CMD_TEMPO;
			}
		}

		DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
		SetCurSel(sel, sel);
		sel &= ~7;
		if(*p != oldcmd)
		{
			pModDoc->SetModified();
			InvalidateArea(sel, sel + LAST_COLUMN);
			UpdateIndicator();
		}
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

	const bool isSplit = (pModDoc->GetSplitKeyboardSettings()->IsSplitActive()) && (note <= pModDoc->GetSplitKeyboardSettings()->splitNote);
	UINT ins = 0;
	if (pModDoc)
	{
		if (isSplit)
		{
			ins = pModDoc->GetSplitKeyboardSettings()->splitInstrument;
			if (pModDoc->GetSplitKeyboardSettings()->octaveLink) note += 12 *pModDoc->GetSplitKeyboardSettings()->octaveModifier;
			if (note > NOTE_MAX && note < NOTE_MIN_SPECIAL) note = NOTE_MAX;
			if (note < 0) note = 1;
		}
		if (!ins)    ins = GetCurrentInstrument();
		if (!ins)	 ins = m_nFoundInstrument;
		if(bChordMode == true)
		{
			m_dwStatus &= ~PATSTATUS_CHORDPLAYING;
			pModDoc->NoteOff(0, TRUE, ins, m_dwCursor & 0xFFFF);
		}
		else
		{
			pModDoc->NoteOff(note, ((CMainFrame::m_dwPatternSetup & PATTERN_NOTEFADE) || pSndFile->GetNumInstruments() == 0) ? TRUE : FALSE, ins, GetChanFromCursor(m_dwCursor));
		}
	}

	//Enter note off in pattern?
	if ((note < NOTE_MIN) || (note > NOTE_MAX))
		return;
	if (GetColTypeFromCursor(m_dwCursor) > INST_COLUMN && (bChordMode || !fromMidi))
		return;
	if (!pModDoc || !pMainFrm || !(IsEditingEnabled()))
		return;

	const bool bIsLiveRecord = IsLiveRecord(*pMainFrm, *pModDoc, *pSndFile);

	const CHANNELINDEX nChnCursor = GetChanFromCursor(m_dwCursor);

	BYTE* activeNoteMap = isSplit ? splitActiveNoteChannel : activeNoteChannel;
	const CHANNELINDEX nChn = (activeNoteMap[note] < pSndFile->GetNumChannels()) ? activeNoteMap[note] : nChnCursor;

	activeNoteMap[note] = 0xFF;	//unlock channel

	if (!((CMainFrame::m_dwPatternSetup & PATTERN_KBDNOTEOFF) || fromMidi))
	{
		// We don't want to write the note-off into the pattern if this feature is disabled and we're not recording from MIDI.
		return;
	}

	// -- write sdx if playing live
	const bool usePlaybackPosition = (!bChordMode) && (bIsLiveRecord && (CMainFrame::m_dwPatternSetup & PATTERN_AUTODELAY));

	//Work out where to put the note off
	ROWINDEX nRow = m_nRow;
	PATTERNINDEX nPat = m_nPattern;

	if(usePlaybackPosition)
		SetEditPos(*pSndFile, nRow, nPat, nRowPlayback, nPatPlayback);

	MODCOMMAND* p = pSndFile->Patterns[nPat].GetpModCommand(nRow, nChn);

	//don't overwrite:
	if (p->note || p->instr || p->volcmd)
	{
		//if there's a note in the current location and the song is playing and following,
		//the user probably just tapped the key - let's try the next row down.
		nRow++;
		if (p->note==note && bIsLiveRecord && pSndFile->Patterns[nPat].IsValidRow(nRow))
		{
			p = pSndFile->Patterns[nPat].GetpModCommand(nRow, nChn);
			if (p->note || (!bChordMode && (p->instr || p->volcmd)) )
				return;
		}
		else
			return;
	}

	// Create undo-point.
	pModDoc->GetPatternUndo()->PrepareUndo(nPat, nChn, nRow, 1, 1);

	// -- write sdx if playing live
	if (usePlaybackPosition && nTick) {	// avoid SD0 which will be mis-interpreted
		if (p->command == 0) {	//make sure we don't overwrite any existing commands.
			p->command = (pSndFile->TypeIsS3M_IT_MPT()) ? CMD_S3MCMDEX : CMD_MODCMDEX;
			p->param   = 0xD0 | min(0xF, nTick);
		}
	}

	//Enter note off
	if(pModDoc->GetSoundFile()->GetModSpecifications().hasNoteOff) // ===
		p->note = NOTE_KEYOFF;
	else if(pModDoc->GetSoundFile()->GetModSpecifications().hasNoteCut) // ^^^
		p->note = NOTE_NOTECUT;
	else { // we don't have anything to cut (MOD format) - use volume or ECx
		if(usePlaybackPosition && nTick) // ECx
		{
			p->command = (pSndFile->TypeIsS3M_IT_MPT()) ? CMD_S3MCMDEX : CMD_MODCMDEX;
			p->param   = 0xC0 | min(0xF, nTick);
		} else // C00
		{
			p->note = NOTE_NONE;
			p->command = CMD_VOLUME;
			p->param = 0;
		}
	}
	p->instr = (bChordMode) ? 0 : ins; //p->instr = 0; 
	//Writing the instrument as well - probably someone finds this annoying :)
	p->volcmd	= 0;
	p->vol		= 0;

	pModDoc->SetModified();

	// Update only if not recording live.
	if(bIsLiveRecord == false)
	{
		DWORD sel = CreateCursor(nRow, nChn, 0);
		InvalidateArea(sel, sel + LAST_COLUMN);
		UpdateIndicator();
	}

	return;

}


// Enter an octave number in the pattern
void CViewPattern::TempEnterOctave(int val)
//-----------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (pMainFrm))
	{

		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nChn = GetChanFromCursor(m_dwCursor);
		PrepareUndo(m_dwBeginSel, m_dwEndSel);

		MODCOMMAND* p = pSndFile->Patterns[m_nPattern].GetpModCommand(m_nRow, nChn);
		MODCOMMAND oldcmd = *p; // This is the command we are about to overwrite
		if (oldcmd.note)
			TempEnterNote(((oldcmd.note-1)%12)+val*12+1);
	}

}


// Enter an instrument number in the pattern
void CViewPattern::TempEnterIns(int val)
//--------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (pMainFrm))
	{

		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		
		UINT nChn = GetChanFromCursor(m_dwCursor);
		PrepareUndo(m_dwBeginSel, m_dwEndSel);

		MODCOMMAND *p = pSndFile->Patterns[m_nPattern].GetpModCommand(m_nRow, nChn);
		MODCOMMAND oldcmd = *p;		// This is the command we are about to overwrite

		UINT instr = p->instr, nTotalMax, nTempMax;
		if(p->IsPcNote())	// this is a plugin index
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
		p->instr = instr;

		if (IsEditingEnabled_bmsg())
		{
			DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
			SetCurSel(sel, sel);
			sel &= ~7;
			if(*p != oldcmd)
			{
				pModDoc->SetModified();
				InvalidateArea(sel, sel + LAST_COLUMN);
				UpdateIndicator();
			}
		}
		else
		{
			// recording disabled
			*p = oldcmd;
		}
	}
}


// Enter a note in the pattern
void CViewPattern::TempEnterNote(int note, bool oldStyle, int vol)
//----------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (pMainFrm))
	{
		bool modified = false;
		ROWINDEX nRow = m_nRow;
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		const ROWINDEX nRowPlayback = pSndFile->m_nRow;
		const UINT nTick = pSndFile->m_nTickCount;
		const PATTERNINDEX nPatPlayback = pSndFile->m_nPattern;

		const bool bRecordEnabled = IsEditingEnabled();
		const UINT nChn = GetChanFromCursor(m_dwCursor);

		if(note > pSndFile->GetModSpecifications().noteMax && note < NOTE_MIN_SPECIAL)
			note = pSndFile->GetModSpecifications().noteMax;
		else if( note < pSndFile->GetModSpecifications().noteMin)
			note = pSndFile->GetModSpecifications().noteMin;

		// Special case: Convert note off commands to C00 for MOD files
		if((pSndFile->GetType() == MOD_TYPE_MOD) && (note == NOTE_NOTECUT || note == NOTE_FADE || note == NOTE_KEYOFF))
		{
			TempEnterFX(CMD_VOLUME, 0);
			return;
		}

		// Check whether the module format supports the note.
		if( pSndFile->GetModSpecifications().HasNote(note) == false )
			return;

		BYTE recordGroup = pModDoc->IsChannelRecord(nChn);
		const bool bIsLiveRecord = IsLiveRecord(*pMainFrm, *pModDoc, *pSndFile);
		const bool usePlaybackPosition = (bIsLiveRecord && (CMainFrame::m_dwPatternSetup & PATTERN_AUTODELAY) && !(pSndFile->m_dwSongFlags & SONG_STEP));
		//Param control 'note'
		if(MODCOMMAND::IsPcNote(note) && bRecordEnabled)
		{
			pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, nChn, nRow, 1, 1);
			pSndFile->Patterns[m_nPattern].GetpModCommand(nRow, nChn)->note = note;
			const DWORD sel = CreateCursor(nRow) | m_dwCursor;
			pModDoc->SetModified();
			InvalidateArea(sel, sel + LAST_COLUMN);
			UpdateIndicator();
			return;
		}


		// -- Chord autodetection: step back if we just entered a note
		if ((bRecordEnabled) && (recordGroup) && !bIsLiveRecord)
		{
			if ((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING))
			{
				if ((timeGetTime() - m_dwLastNoteEntryTime < CMainFrame::gnAutoChordWaitTime)
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
		MODCOMMAND *pTarget = pSndFile->Patterns[nPat].GetpModCommand(nRow, nChn);
		MODCOMMAND newcmd = *pTarget;
		
		// If record is enabled, create undo point.
		if(bRecordEnabled)
		{
			pModDoc->GetPatternUndo()->PrepareUndo(nPat, nChn, nRow, 1, 1);
		}

		// We're overwriting a PC note here.
		if(pTarget->IsPcNote() && !MODCOMMAND::IsPcNote(note))
		{
			newcmd.Clear();
		}

		// -- write note and instrument data.
		const bool isSplit = HandleSplit(&newcmd, note);

		// Nice idea actually: Use lower section of the keyboard to play chords (but it won't work 100% correctly this way...)
		/*if(isSplit)
		{
			TempEnterChord(note);
			return;
		}*/

		// -- write vol data
		int volWrite = -1;
		if (vol >= 0 && vol <= 64 && !(isSplit && pModDoc->GetSplitKeyboardSettings()->splitVolume))	//write valid volume, as long as there's no split volume override.
		{
			volWrite = vol;
		} else if (isSplit && pModDoc->GetSplitKeyboardSettings()->splitVolume)	//cater for split volume override.
		{
			if (pModDoc->GetSplitKeyboardSettings()->splitVolume > 0 && pModDoc->GetSplitKeyboardSettings()->splitVolume <= 64)
			{
				volWrite = pModDoc->GetSplitKeyboardSettings()->splitVolume;
			}
		}

		if(volWrite != -1)
		{
			if(pSndFile->GetModSpecifications().HasVolCommand(VOLCMD_VOLUME))
			{
				newcmd.volcmd = VOLCMD_VOLUME;
				newcmd.vol = (MODCOMMAND::VOL)volWrite;
			} else
			{
				newcmd.command = CMD_VOLUME;
				newcmd.param = (MODCOMMAND::PARAM)volWrite;
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
		

		// -- if recording, write out modified command.
		if (bRecordEnabled && *pTarget != newcmd)
		{
			*pTarget = newcmd;
			modified = true;
		}


		// -- play note
		if ((CMainFrame::m_dwPatternSetup & (PATTERN_PLAYNEWNOTE|PATTERN_PLAYEDITROW)) || (!bRecordEnabled))
		{
			const bool playWholeRow = ((CMainFrame::m_dwPatternSetup & PATTERN_PLAYEDITROW) && !bIsLiveRecord);
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

				// just play the newly inserted note using the already specified instrument...
				UINT nPlayIns = newcmd.instr;
				if(!nPlayIns && (note <= NOTE_MAX))
				{
					// ...or one that can be found on a previous row of this pattern.
					MODCOMMAND *search = pTarget;
					UINT srow = nRow;
					while (srow-- > 0)
					{
						search -= pSndFile->m_nChannels;
						if (search->instr)
						{
							nPlayIns = search->instr;
							m_nFoundInstrument = nPlayIns;  //used to figure out which instrument to stop on key release.
							break;
						}
					}
				}
				BOOL bNotPlaying = ((pMainFrm->GetModPlaying() == pModDoc) && (pMainFrm->IsPlaying())) ? FALSE : TRUE;
				//pModDoc->PlayNote(p->note, nPlayIns, 0, bNotPlaying, -1, 0, 0, nChn);	//rewbs.vstiLive - added extra args
				pModDoc->PlayNote(newcmd.note, nPlayIns, 0, bNotPlaying, 4*vol, 0, 0, nChn);	//rewbs.vstiLive - added extra args
			}
		}


		// -- if recording, handle post note entry behaviour (move cursor etc..)
		if(bRecordEnabled)
		{
			DWORD sel = CreateCursor(nRow) | m_dwCursor;
			if(bIsLiveRecord == false)
			{   // Update only when not recording live.
				SetCurSel(sel, sel);
				sel &= ~7;
			}

			if(modified) //has it really changed?
			{
				pModDoc->SetModified();
				if(bIsLiveRecord == false)
				{   // Update only when not recording live.
					InvalidateArea(sel, sel + LAST_COLUMN);
					UpdateIndicator();
				}
			}

			// Set new cursor position (row spacing)
			if (!bIsLiveRecord)
			{
				if((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING))
				{
					if(nRow + m_nSpacing < pSndFile->Patterns[nPat].GetNumRows() || (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL))
					{
						SetCurrentRow(nRow + m_nSpacing, (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL) ? true: false);
						m_bLastNoteEntryBlocked = false;
					} else
					{
						m_bLastNoteEntryBlocked = true;  // if the cursor is block by the end of the pattern here,
					}								     // we must remember to not step back should the next note form a chord.

				}
				DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
				SetCurSel(sel, sel);
			}

			BYTE* activeNoteMap = isSplit ? splitActiveNoteChannel : activeNoteChannel;
			if (newcmd.note <= NOTE_MAX)
				activeNoteMap[newcmd.note] = nChn;

			//Move to next channel if required
			if (recordGroup)
			{
				bool channelLocked;

				UINT n = nChn;
				for (UINT i=1; i<pSndFile->m_nChannels; i++)
				{
					if (++n >= pSndFile->m_nChannels) n = 0; //loop around

					channelLocked = false;
					for (int k=0; k<NOTE_MAX; k++)
					{
						if (activeNoteChannel[k]==n  || splitActiveNoteChannel[k]==n)
						{
							channelLocked = true;
							break;
						}
					}

					if (pModDoc->IsChannelRecord(n)==recordGroup && !channelLocked)
					{
						SetCurrentColumn(n<<3);
						break;
					}
				}
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
	UINT nPlayChord = 0;
	BYTE chordplaylist[3];

	if ((pModDoc) && (pMainFrm))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		const CHANNELINDEX nChn = GetChanFromCursor(m_dwCursor);
		UINT nPlayIns = 0;
		// Simply backup the whole row.
		pModDoc->GetPatternUndo()->PrepareUndo(m_nPattern, nChn, m_nRow, pSndFile->GetNumChannels(), 1);

		const PatternRow prowbase = pSndFile->Patterns[m_nPattern].GetRow(m_nRow);
		MODCOMMAND* pTarget = &prowbase[nChn];
		// Save old row contents		
		vector<MODCOMMAND> newrow(pSndFile->GetNumChannels());
		for(CHANNELINDEX n = 0; n < pSndFile->GetNumChannels(); n++)
		{
			newrow[n] = prowbase[n];
		}
		// This is being written to
		MODCOMMAND* p = &newrow[nChn];


		const bool bIsLiveRecord = IsLiveRecord(*pMainFrm, *pModDoc, *pSndFile);
		const bool bRecordEnabled = IsEditingEnabled();

		// -- establish note data
		//const bool isSplit = HandleSplit(p, note);
		HandleSplit(p, note);

		PMPTCHORD pChords = pMainFrm->GetChords();
		UINT baseoctave = pMainFrm->GetBaseOctave();
		UINT nchord = note - baseoctave * 12 - 1;
		UINT nNote;
		bool modified = false;
		if (nchord < 3 * 12)
		{
			UINT nchordnote = pChords[nchord].key + baseoctave * 12 + 1;
			// Rev. 244, commented the isSplit conditions below, can't see the point in it.
            //if (isSplit)
			//	nchordnote = pChords[nchord].key + baseoctave*(p->note%12) + 1;
			//else
			//	nchordnote = pChords[nchord].key + baseoctave*12 + 1;
			if (nchordnote <= NOTE_MAX)
			{
				UINT nchordch = nChn, nchno = 0;
				nNote = nchordnote;
				p->note = nNote;
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
							if(prowbase[nchordch] != newrow[nchordch])
							{
								modified = true;
							}

							nchno++;
							if (CMainFrame::m_dwPatternSetup & PATTERN_PLAYNEWNOTE)
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
			DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
			SetCurSel(sel, sel);
			sel = GetRowFromCursor(sel) | GetChanFromCursor(sel);
			if(modified)
			{
				for(CHANNELINDEX n = 0; n < pSndFile->GetNumChannels(); n++)
				{
					prowbase[n] = newrow[n];
				}
				pModDoc->SetModified();
				InvalidateRow();
				UpdateIndicator();
			}
		}


		// -- play note
		if ((CMainFrame::m_dwPatternSetup & (PATTERN_PLAYNEWNOTE|PATTERN_PLAYEDITROW)) || (!bRecordEnabled))
		{
			const bool playWholeRow = ((CMainFrame::m_dwPatternSetup & PATTERN_PLAYEDITROW) && !bIsLiveRecord);
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
					MODCOMMAND *search = pTarget;
					UINT srow = m_nRow;
					while (srow-- > 0)
					{
						search -= pSndFile->m_nChannels;
						if (search->instr)
						{
							nPlayIns = search->instr;
							m_nFoundInstrument = nPlayIns;  //used to figure out which instrument to stop on key release.
							break;
						}
					}
				}
				BOOL bNotPlaying = ((pMainFrm->GetModPlaying() == pModDoc) && (pMainFrm->IsPlaying())) ? FALSE : TRUE;
				pModDoc->PlayNote(p->note, nPlayIns, 0, bNotPlaying, -1, 0, 0, nChn);	//rewbs.vstiLive - added extra args
				for (UINT kplchrd=0; kplchrd<nPlayChord; kplchrd++)
				{
					if (chordplaylist[kplchrd])
					{
						pModDoc->PlayNote(chordplaylist[kplchrd], nPlayIns, 0, FALSE, -1, 0, 0, nChn);	//rewbs.vstiLive - 	- added extra args
						m_dwStatus |= PATSTATUS_CHORDPLAYING;
					}
				}
			}
		} // end play note


		// Set new cursor position (row spacing) - only when not recording live
		if (bRecordEnabled && !bIsLiveRecord)
		{
			if ((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING)) 
				SetCurrentRow(m_nRow + m_nSpacing);
			DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
			SetCurSel(sel, sel);
		}

	} // end mainframe and moddoc exist
}


void CViewPattern::OnClearField(int field, bool step, bool ITStyle)
//-----------------------------------------------------------------
{
 	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (pMainFrm))
	{
        //if we have a selection we want to do something different
		if (m_dwBeginSel != m_dwEndSel)
		{
			OnClearSelection(ITStyle);
			return;
		}

		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		UINT nChn = GetChanFromCursor(m_dwCursor);
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODCOMMAND *p = pSndFile->Patterns[m_nPattern] +  m_nRow*pSndFile->m_nChannels + nChn;
		MODCOMMAND oldcmd = *p;

		switch(field)
		{
			case NOTE_COLUMN:	if(p->IsPcNote()) p->Clear(); else {p->note = NOTE_NONE; if (ITStyle) p->instr = 0;}  break;		//Note
			case INST_COLUMN:	p->instr = 0; break;				//instr
			case VOL_COLUMN:	p->vol = 0; p->volcmd = 0; break;	//Vol
			case EFFECT_COLUMN:	p->command = 0;	break;				//Effect
			case PARAM_COLUMN:	p->param = 0; break;				//Param
			default:			p->Clear();							//If not specified, delete them all! :)
		}
		if((field == 3 || field == 4) && (p->IsPcNote()))
			p->SetValueEffectCol(0);

		if(IsEditingEnabled_bmsg())
		{
			DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
			SetCurSel(sel, sel);
			sel = GetRowFromCursor(sel) | GetChanFromCursor(sel);
			if(*p != oldcmd)
			{
				pModDoc->SetModified();
				InvalidateRow();
				UpdateIndicator();
			}
			if (step && (((pMainFrm->GetFollowSong(pModDoc) != m_hWnd) || (pSndFile->IsPaused())
			 || (!(m_dwStatus & PATSTATUS_FOLLOWSONG)))))
			{
				if ((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING)) 
					SetCurrentRow(m_nRow+m_nSpacing);
				DWORD sel = CreateCursor(m_nRow) | m_dwCursor;
				SetCurSel(sel, sel);
			}
		} else
		{
			*p = oldcmd;
		}
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
	CModDoc *pModDoc = GetDocument(); if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile(); if (!pSndFile) return;

	int plug = pSndFile->ChnSettings[chan].nMixPlugin;
	if (plug > 0)
		pModDoc->TogglePluginEditor(plug-1);

	return;
}


void CViewPattern::OnSelectInstrument(UINT nID)
//---------------------------------------------
{
	const UINT newIns = nID - ID_CHANGE_INSTRUMENT;

	if (newIns == 0)
	{
		RowMask sp = {false, true, false, false, false};    // Setup mask to only clear instrument data in OnClearSelection
		OnClearSelection(false, sp);	// Clears instrument selection from pattern
	} else
	{
		SetSelectionInstrument(newIns);
	}
}

void CViewPattern::OnSelectPCNoteParam(UINT nID)
//----------------------------------------------
{
	CModDoc *pModDoc = GetDocument(); if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile(); if (!pSndFile) return;

	UINT paramNdx = nID - ID_CHANGE_PCNOTE_PARAM;
	bool bModified = false;
	MODCOMMAND *p;
	for(ROWINDEX nRow = GetSelectionStartRow(); nRow <= GetSelectionEndRow(); nRow++)
	{
		for(CHANNELINDEX nChn = GetSelectionStartChan(); nChn <= GetSelectionEndChan(); nChn++)
		{
			p = pSndFile->Patterns[m_nPattern] + nRow * pSndFile->GetNumChannels() + nChn;
			if(p && p->IsPcNote() && (p->GetValueVolCol() != paramNdx))
			{
				bModified = true;
				p->SetValueVolCol(paramNdx);
			}
		}
	}
	if (bModified)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
	}
}


void CViewPattern::OnSelectPlugin(UINT nID)
//-----------------------------------------
{
	CModDoc *pModDoc = GetDocument(); if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile(); if (!pSndFile) return;

	if (m_nMenuOnChan)
	{
		UINT newPlug = nID-ID_PLUGSELECT;
		if (newPlug <= MAX_MIXPLUGINS && newPlug != pSndFile->ChnSettings[m_nMenuOnChan-1].nMixPlugin)
		{
			pSndFile->ChnSettings[m_nMenuOnChan-1].nMixPlugin = newPlug;
			if(pSndFile->GetModSpecifications().supportsPlugins)
				pModDoc->SetModified();
			InvalidateChannelsHeaders();
		}
	}
}

//rewbs.merge
bool CViewPattern::HandleSplit(MODCOMMAND* p, int note)
//-----------------------------------------------------
{
	CModDoc *pModDoc = GetDocument(); if (!pModDoc) return false;

	MODCOMMAND::INSTR ins = GetCurrentInstrument();
	bool isSplit = false;

	if(pModDoc->GetSplitKeyboardSettings()->IsSplitActive())
	{
		if(note <= pModDoc->GetSplitKeyboardSettings()->splitNote)
		{
			isSplit = true;

			if (pModDoc->GetSplitKeyboardSettings()->octaveLink && note <= NOTE_MAX)
			{
				note += 12 * pModDoc->GetSplitKeyboardSettings()->octaveModifier;
				note = CLAMP(note, NOTE_MIN, NOTE_MAX);
			}
			if (pModDoc->GetSplitKeyboardSettings()->splitInstrument)
			{
				p->instr = pModDoc->GetSplitKeyboardSettings()->splitInstrument;
			}
		}
	}
	
	p->note = note;	
	if(ins) 
		p->instr = ins;

	return isSplit;
}

bool CViewPattern::BuildPluginCtxMenu(HMENU hMenu, UINT nChn, CSoundFile* pSndFile)
//---------------------------------------------------------------------------------
{
	CHAR s[512];
	bool itemFound;
	for (UINT plug=0; plug<=MAX_MIXPLUGINS; plug++)	{

		itemFound=false;
		s[0] = 0;

		if (!plug) {
			strcpy(s, "No plugin");
			itemFound=true;
		} else	{
			PSNDMIXPLUGIN p = &(pSndFile->m_MixPlugins[plug-1]);
			if (p->Info.szLibraryName[0]) {
				wsprintf(s, "FX%d: %s", plug, p->Info.szName);
				itemFound=true;
			}
		}

		if (itemFound){
			m_nMenuOnChan=nChn+1;
			if (plug == pSndFile->ChnSettings[nChn].nMixPlugin) {
				AppendMenu(hMenu, (MF_STRING|MF_CHECKED), ID_PLUGSELECT+plug, s);
			} else {
				AppendMenu(hMenu, MF_STRING, ID_PLUGSELECT+plug, s);
			}
		}
	}
	return true;
}

bool CViewPattern::BuildSoloMuteCtxMenu(HMENU hMenu, CInputHandler* ih, UINT nChn, CSoundFile* pSndFile)
//------------------------------------------------------------------------------------------------------
{
	AppendMenu(hMenu, (pSndFile->ChnSettings[nChn].dwFlags & CHN_MUTE) ? 
					   (MF_STRING|MF_CHECKED) : MF_STRING, ID_PATTERN_MUTE, 
					   "Mute Channel\t" + ih->GetKeyTextFromCommand(kcChannelMute));
	bool bSolo = false, bUnmuteAll = false;
	bool bSoloPending = false, bUnmuteAllPending = false; // doesn't work perfectly yet

	for (CHANNELINDEX i = 0; i < pSndFile->m_nChannels; i++) {
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
			pSndFile->m_bChannelMuteTogglePending[nChn] ? 
					(MF_STRING|MF_CHECKED) : MF_STRING,
			 ID_PATTERN_TRANSITIONMUTE,
			(pSndFile->ChnSettings[nChn].dwFlags & CHN_MUTE) ?
			"On transition: Unmute\t" + ih->GetKeyTextFromCommand(kcToggleChanMuteOnPatTransition) :
			"On transition: Mute\t" + ih->GetKeyTextFromCommand(kcToggleChanMuteOnPatTransition));

	if (bUnmuteAllPending) AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITION_UNMUTEALL, "On transition: Unmute all\t" + ih->GetKeyTextFromCommand(kcUnmuteAllChnOnPatTransition));
	if (bSoloPending) AppendMenu(hMenu, MF_STRING, ID_PATTERN_TRANSITIONSOLO, "On transition: Solo\t" + ih->GetKeyTextFromCommand(kcSoloChnOnPatTransition));

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_CHNRESET, "Reset Channel\t" + ih->GetKeyTextFromCommand(kcChannelReset));
	
	return true;
}

bool CViewPattern::BuildRecordCtxMenu(HMENU hMenu, UINT nChn, CModDoc* pModDoc)
//-----------------------------------------------------------------------------
{
	AppendMenu(hMenu, pModDoc->IsChannelRecord1(nChn) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_EDIT_RECSELECT, "Record select");
	AppendMenu(hMenu, pModDoc->IsChannelRecord2(nChn) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_EDIT_SPLITRECSELECT, "Split Record select");
	return true;
}



bool CViewPattern::BuildRowInsDelCtxMenu(HMENU hMenu, CInputHandler* ih)
//----------------------------------------------------------------------
{
	CString label = "";
	if (GetSelectionStartRow() != GetSelectionEndRow()) {
		label = "Rows";
	} else { 
		label = "Row";
	}

	AppendMenu(hMenu, MF_STRING, ID_PATTERN_INSERTROW, "Insert " + label + "\t" + ih->GetKeyTextFromCommand(kcInsertRow));
	AppendMenu(hMenu, MF_STRING, ID_PATTERN_DELETEROW, "Delete " + label + "\t" + ih->GetKeyTextFromCommand(kcDeleteRows) );
	return true;
}

bool CViewPattern::BuildMiscCtxMenu(HMENU hMenu, CInputHandler* ih)
//-----------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_SHOWTIMEATROW, "Show row play time\t" + ih->GetKeyTextFromCommand(kcTimeAtRow));
	return true;
}

bool CViewPattern::BuildSelectionCtxMenu(HMENU hMenu, CInputHandler* ih)
//----------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECTCOLUMN, "Select Column\t" + ih->GetKeyTextFromCommand(kcSelectColumn));
	AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECT_ALL, "Select Pattern\t" + ih->GetKeyTextFromCommand(kcEditSelectAll));
	return true;
}

bool CViewPattern::BuildGrowShrinkCtxMenu(HMENU hMenu, CInputHandler* ih)
//----------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_GROW_SELECTION, "Grow selection\t" + ih->GetKeyTextFromCommand(kcPatternGrowSelection));
	AppendMenu(hMenu, MF_STRING, ID_SHRINK_SELECTION, "Shrink selection\t" + ih->GetKeyTextFromCommand(kcPatternShrinkSelection));
	return true;
}

bool CViewPattern::BuildNoteInterpolationCtxMenu(HMENU hMenu, CInputHandler* ih, CSoundFile* pSndFile)
//----------------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = MF_GRAYED;

	UINT startRow = GetSelectionStartRow();
	UINT endRow   = GetSelectionEndRow();
	
	if (ListChansWhereColSelected(NOTE_COLUMN, validChans) > 0)
	{
		for (int valChnIdx=0; valChnIdx<validChans.GetCount(); valChnIdx++)
		{
			if (IsInterpolationPossible(startRow, endRow, 
									    validChans[valChnIdx], NOTE_COLUMN, pSndFile))
			{
				greyed=0;	//Can do interpolation.
				break;
			}
		}

	}
	if (!greyed || !(CMainFrame::m_dwPatternSetup&PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING|greyed, ID_PATTERN_INTERPOLATE_NOTE, "Interpolate Note\t" + ih->GetKeyTextFromCommand(kcPatternInterpolateNote));
		return true;
	}
	return false;
}

bool CViewPattern::BuildVolColInterpolationCtxMenu(HMENU hMenu, CInputHandler* ih, CSoundFile* pSndFile)
//------------------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = MF_GRAYED;

	UINT startRow = GetSelectionStartRow();
	UINT endRow   = GetSelectionEndRow();
	
	if (ListChansWhereColSelected(VOL_COLUMN, validChans) > 0)
	{
		for (int valChnIdx=0; valChnIdx<validChans.GetCount(); valChnIdx++)
		{
			if (IsInterpolationPossible(startRow, endRow, 
									    validChans[valChnIdx], VOL_COLUMN, pSndFile))
			{
				greyed = 0;	//Can do interpolation.
				break;
			}
		}
	}
	if (!greyed || !(CMainFrame::m_dwPatternSetup&PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING|greyed, ID_PATTERN_INTERPOLATE_VOLUME, "Interpolate Vol Col\t" + ih->GetKeyTextFromCommand(kcPatternInterpolateVol));
		return true;
	}
	return false;
}


bool CViewPattern::BuildEffectInterpolationCtxMenu(HMENU hMenu, CInputHandler* ih, CSoundFile* pSndFile)
//------------------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = MF_GRAYED;

	UINT startRow = GetSelectionStartRow();
	UINT endRow   = GetSelectionEndRow();
	
	if (ListChansWhereColSelected(EFFECT_COLUMN, validChans) > 0)
	{
		for (int valChnIdx=0; valChnIdx<validChans.GetCount(); valChnIdx++)
		{
			if  (IsInterpolationPossible(startRow, endRow, validChans[valChnIdx], EFFECT_COLUMN, pSndFile))
			{
				greyed=0;	//Can do interpolation.
				break;
			}
		}
	}

	if (ListChansWhereColSelected(PARAM_COLUMN, validChans) > 0)
	{
		for (int valChnIdx=0; valChnIdx<validChans.GetCount(); valChnIdx++)
		{
			if  (IsInterpolationPossible(startRow, endRow, validChans[valChnIdx], EFFECT_COLUMN, pSndFile))
			{
				greyed = 0;	//Can do interpolation.
				break;
			}
		}
	}


	if (!greyed || !(CMainFrame::m_dwPatternSetup&PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING|greyed, ID_PATTERN_INTERPOLATE_EFFECT, "Interpolate Effect\t" + ih->GetKeyTextFromCommand(kcPatternInterpolateEffect));
		return true;
	}
	return false;
}

bool CViewPattern::BuildEditCtxMenu(HMENU hMenu, CInputHandler* ih, CModDoc* pModDoc)
//-----------------------------------------------------------------------------------
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

	DWORD greyed = pModDoc->GetPatternUndo()->CanUndo() ? MF_ENABLED : MF_GRAYED;
	if (!greyed || !(CMainFrame::m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		AppendMenu(hMenu, MF_STRING | greyed, ID_EDIT_UNDO, "Undo\t" + ih->GetKeyTextFromCommand(kcEditUndo));
	}

	AppendMenu(hMenu, MF_STRING, ID_CLEAR_SELECTION, "Clear selection\t" + ih->GetKeyTextFromCommand(kcSampleDelete));

	return true;
}

bool CViewPattern::BuildVisFXCtxMenu(HMENU hMenu, CInputHandler* ih)
//------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(EFFECT_COLUMN, validChans)>0)?FALSE:MF_GRAYED;

	if (!greyed || !(CMainFrame::m_dwPatternSetup&PATTERN_OLDCTXMENUSTYLE)) {
		AppendMenu(hMenu, MF_STRING|greyed, ID_PATTERN_VISUALIZE_EFFECT, "Visualize Effect\t" + ih->GetKeyTextFromCommand(kcPatternVisualizeEffect));
		return true;
	}
	return false;
}

bool CViewPattern::BuildRandomCtxMenu(HMENU hMenu, CInputHandler* ih)
//------------------------------------------------------------------
{
	AppendMenu(hMenu, MF_STRING, ID_PATTERN_OPEN_RANDOMIZER, "Randomize...\t" + ih->GetKeyTextFromCommand(kcPatternOpenRandomizer));
	return true;
}

bool CViewPattern::BuildTransposeCtxMenu(HMENU hMenu, CInputHandler* ih)
//----------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(NOTE_COLUMN, validChans)>0)?FALSE:MF_GRAYED;

	//AppendMenu(hMenu, MF_STRING, ID_RUN_SCRIPT, "Run script");

	if (!greyed || !(CMainFrame::m_dwPatternSetup&PATTERN_OLDCTXMENUSTYLE)) {
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_UP, "Transpose +1\t" + ih->GetKeyTextFromCommand(kcTransposeUp));
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_DOWN, "Transpose -1\t" + ih->GetKeyTextFromCommand(kcTransposeDown));
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_OCTUP, "Transpose +12\t" + ih->GetKeyTextFromCommand(kcTransposeOctUp));
		AppendMenu(hMenu, MF_STRING|greyed, ID_TRANSPOSE_OCTDOWN, "Transpose -12\t" + ih->GetKeyTextFromCommand(kcTransposeOctDown));
		return true;
	}
	return false;
}

bool CViewPattern::BuildAmplifyCtxMenu(HMENU hMenu, CInputHandler* ih)
//--------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(VOL_COLUMN, validChans)>0)?FALSE:MF_GRAYED;

	if (!greyed || !(CMainFrame::m_dwPatternSetup&PATTERN_OLDCTXMENUSTYLE)) {
		AppendMenu(hMenu, MF_STRING|greyed, ID_PATTERN_AMPLIFY, "Amplify\t" + ih->GetKeyTextFromCommand(kcPatternAmplify));
		return true;
	}
	return false;
}


bool CViewPattern::BuildChannelControlCtxMenu(HMENU hMenu)
//--------------------------------------------------------
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


bool CViewPattern::BuildSetInstCtxMenu(HMENU hMenu, CInputHandler* ih, CSoundFile* pSndFile)
//------------------------------------------------------------------------------------------
{
	CArray<UINT, UINT> validChans;
	DWORD greyed = (ListChansWhereColSelected(INST_COLUMN, validChans)>0)?FALSE:MF_GRAYED;

	if (!greyed || !(CMainFrame::m_dwPatternSetup & PATTERN_OLDCTXMENUSTYLE))
	{
		bool isPcNote = false;
		MODCOMMAND *mSelStart = nullptr;
		if((pSndFile != nullptr) && (pSndFile->Patterns.IsValidPat(m_nPattern)))
		{
			mSelStart = pSndFile->Patterns[m_nPattern].GetpModCommand(GetSelectionStartRow(), GetSelectionStartChan());
			if(mSelStart != nullptr && mSelStart->IsPcNote())
			{
				isPcNote = true;
			}
		}
		if(isPcNote)
			return false;

		// Create the new menu and add it to the existing menu.
		HMENU instrumentChangeMenu = ::CreatePopupMenu();
		AppendMenu(hMenu, MF_POPUP|greyed, (UINT)instrumentChangeMenu, "Change Instrument\t" + ih->GetKeyTextFromCommand(kcPatternSetInstrument));

		if(pSndFile == nullptr || pSndFile->GetpModDoc() == nullptr)
			return false;

		CModDoc* const pModDoc = pSndFile->GetpModDoc();
	
		if(!greyed)
		{
			if (pSndFile->m_nInstruments)
			{
				for (UINT i=1; i<=pSndFile->m_nInstruments; i++)
				{
					if (pSndFile->Instruments[i] == NULL)
						continue;

					CString instString = pModDoc->GetPatternViewInstrumentName(i, true);
					if(instString.GetLength() > 0) AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT+i, pModDoc->GetPatternViewInstrumentName(i));
					//Adding the entry to the list only if it has some name, since if the name is empty,
					//it likely is some non-used instrument.
				}

			}
			else
			{
				CHAR s[256];
				UINT nmax = pSndFile->m_nSamples;
				while ((nmax > 1) && (pSndFile->Samples[nmax].pSample == NULL) && (!pSndFile->m_szNames[nmax][0])) nmax--;
				for (UINT i=1; i<=nmax; i++) if ((pSndFile->m_szNames[i][0]) || (pSndFile->Samples[i].pSample))
				{
					wsprintf(s, "%02d: %s", i, pSndFile->m_szNames[i]);
					AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT+i, s);
				}
			}

			//Add options to remove instrument from selection.
     		AppendMenu(instrumentChangeMenu, MF_SEPARATOR, 0, 0);
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT, "Remove instrument");
			AppendMenu(instrumentChangeMenu, MF_STRING, ID_CHANGE_INSTRUMENT + GetCurrentInstrument(), "Set to current instrument");
		}
		return true;
	}
	return false;
}


bool CViewPattern::BuildChannelMiscCtxMenu(HMENU hMenu, CSoundFile* pSndFile)
//---------------------------------------------------------------------------
{
	if((pSndFile->m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) == 0) return false;
	AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(hMenu, MF_STRING, ID_CHANNEL_RENAME, "Rename channel");
	return true;
}


// Context menu for Param Control notes
bool CViewPattern::BuildPCNoteCtxMenu(HMENU hMenu, CInputHandler* ih, CSoundFile* pSndFile)
//-----------------------------------------------------------------------------------------
{
	MODCOMMAND *mSelStart = nullptr;
	if((pSndFile == nullptr) || (!pSndFile->Patterns.IsValidPat(m_nPattern)))
		return false;
	mSelStart = pSndFile->Patterns[m_nPattern].GetpModCommand(GetSelectionStartRow(), GetSelectionStartChan());
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

			char sname[64];
			uint16 nThisParam = mSelStart->GetValueVolCol();
			UINT nParams = plug->GetNumParameters();
			for (UINT i = 0; i < nParams; i++)
			{
				plug->GetParamName(i, sname, sizeof(sname));
				wsprintf(s, "%02d: %s", i, sname);
				AppendMenu(paramChangeMenu, MF_STRING | (i == nThisParam) ? MF_CHECKED : 0, ID_CHANGE_PCNOTE_PARAM + i, s);
			}
		}				

		AppendMenu(hMenu, MF_STRING, ID_PATTERN_EDIT_PCNOTE_PLUGIN, "Toggle plugin editor\t" + ih->GetKeyTextFromCommand(kcPatternEditPCNotePlugin));
	}

	return true;
}


ROWINDEX CViewPattern::GetSelectionStartRow()
//-------------------------------------------
{
	return min(GetRowFromCursor(m_dwBeginSel), GetRowFromCursor(m_dwEndSel));
}

ROWINDEX CViewPattern::GetSelectionEndRow()
//-----------------------------------------
{
	return max(GetRowFromCursor(m_dwBeginSel), GetRowFromCursor(m_dwEndSel));
}

CHANNELINDEX CViewPattern::GetSelectionStartChan()
//------------------------------------------------
{
	return min(GetChanFromCursor(m_dwBeginSel), GetChanFromCursor(m_dwEndSel));
}

CHANNELINDEX CViewPattern::GetSelectionEndChan()
//----------------------------------------------
{
	return max(GetChanFromCursor(m_dwBeginSel), GetChanFromCursor(m_dwEndSel));
}

UINT CViewPattern::ListChansWhereColSelected(PatternColumns colType, CArray<UINT,UINT> &chans) {
//----------------------------------------------------------------------------------
	chans.RemoveAll();
	UINT startChan = GetSelectionStartChan();
	UINT endChan   = GetSelectionEndChan();

	if (GetColTypeFromCursor(m_dwBeginSel) <= colType) {
		chans.Add(startChan);	//first selected chan includes this col type
	}
	for (UINT chan=startChan+1; chan<endChan; chan++) {
		chans.Add(chan); //All chans between first & last must include this col type
	}
	if ((startChan != endChan) && colType <= GetColTypeFromCursor(m_dwEndSel)) {
		chans.Add(endChan);		//last selected chan includes this col type
	}

	return chans.GetCount();
}


bool CViewPattern::IsInterpolationPossible(ROWINDEX startRow, ROWINDEX endRow, CHANNELINDEX chan,
										   PatternColumns colType, CSoundFile* pSndFile)
//--------------------------------------------------------------------------------------
{
	if (startRow == endRow)
		return false;

	bool result = false;
	const MODCOMMAND startRowMC = *pSndFile->Patterns[m_nPattern].GetpModCommand(startRow, chan);
	const MODCOMMAND endRowMC = *pSndFile->Patterns[m_nPattern].GetpModCommand(endRow, chan);
	UINT startRowCmd, endRowCmd;

	if(colType == EFFECT_COLUMN && (startRowMC.IsPcNote() || endRowMC.IsPcNote()))
		return true;

	switch (colType)
	{
		case NOTE_COLUMN:
			startRowCmd = startRowMC.note;
			endRowCmd = endRowMC.note;
			result = (startRowCmd >= NOTE_MIN && endRowCmd >= NOTE_MIN);
			break;
		case EFFECT_COLUMN:
			startRowCmd = startRowMC.command;
			endRowCmd = endRowMC.command;
			result = (startRowCmd == endRowCmd && startRowCmd != CMD_NONE) || (startRowCmd != CMD_NONE && endRowCmd == CMD_NONE) || (startRowCmd == CMD_NONE && endRowCmd != CMD_NONE);
			break;
		case VOL_COLUMN:
			startRowCmd = startRowMC.volcmd;
			endRowCmd = endRowMC.volcmd;
			result = (startRowCmd == endRowCmd && startRowCmd != VOLCMD_NONE) || (startRowCmd != VOLCMD_NONE && endRowCmd == VOLCMD_NONE) || (startRowCmd == VOLCMD_NONE && endRowCmd != VOLCMD_NONE);
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

void CViewPattern::OnTogglePendingMuteFromClick()
//-----------------------------------------------
{
	TogglePendingMute(GetChanFromCursor(m_nMenuParam));
}

void CViewPattern::OnPendingSoloChnFromClick()
//-----------------------------------------------
{
	PendingSoloChn(GetChanFromCursor(m_nMenuParam));
}

void CViewPattern::OnPendingUnmuteAllChnFromClick()
//----------------------------------------------
{
	GetDocument()->GetSoundFile()->GetPlaybackEventer().PatternTransitionChnUnmuteAll();
	InvalidateChannelsHeaders();
}

void CViewPattern::PendingSoloChn(const CHANNELINDEX nChn)
//---------------------------------------------
{
	GetDocument()->GetSoundFile()->GetPlaybackEventer().PatternTranstionChnSolo(nChn);
	InvalidateChannelsHeaders();
}

void CViewPattern::TogglePendingMute(UINT nChn)
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	pSndFile = pModDoc->GetSoundFile();
	pSndFile->m_bChannelMuteTogglePending[nChn]=!pSndFile->m_bChannelMuteTogglePending[nChn];
	InvalidateChannelsHeaders();
}


bool CViewPattern::IsEditingEnabled_bmsg()
//----------------------------------------
{
	if(IsEditingEnabled()) return true;

	HMENU hMenu;

	if ( (hMenu = ::CreatePopupMenu()) == NULL) return false;
	
	CPoint pt = GetPointFromPosition(m_dwCursor);

	AppendMenu(hMenu, MF_STRING, IDC_PATTERN_RECORD, "Editing(record) is disabled; click here to enable it.");
	//To check: It seems to work the way it should, but still is it ok to use IDC_PATTERN_RECORD here since it is not
	//'aimed' to be used here.

	ClientToScreen(&pt);
	::TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, m_hWnd, NULL);

	::DestroyMenu(hMenu);

	return false;
}


void CViewPattern::OnShowTimeAtRow()
//----------------------------------
{
	CModDoc* pModDoc = GetDocument();
	CSoundFile* pSndFile = (pModDoc) ? pModDoc->GetSoundFile() : 0;
	if(!pSndFile) return;

	CString msg;
	ORDERINDEX currentOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
	if(pSndFile->Order[currentOrder] == m_nPattern)
	{
		const double t = pSndFile->GetPlaybackTimeAt(currentOrder, m_nRow, false);
		if(t < 0)
			msg.Format("Unable to determine the time. Possible cause: No order %d, row %d found from play sequence", currentOrder, m_nRow);
		else
		{
			const uint32 minutes = static_cast<uint32>(t/60.0);
			const float seconds = t - minutes*60;
			msg.Format("Estimate for playback time at order %d(pattern %d), row %d: %d minute%s %.2f seconds", currentOrder, m_nPattern, m_nRow, minutes, (minutes == 1) ? "" : "s", seconds);
		}
	}
	else
		msg.Format("Unable to determine the time: pattern at current order(=%d) does not correspond to pattern at pattern view(=pattern %d).", currentOrder, m_nPattern);
	
	MessageBox(msg);	
}



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
		iRow = m_nRow;
	}
}


void CViewPattern::OnRenameChannel()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return;

	CHANNELINDEX nChn = GetChanFromCursor(m_nMenuParam);
	CChannelRenameDlg dlg(this, pSndFile->ChnSettings[nChn].szName, nChn + 1);
	if(dlg.DoModal() != IDOK || dlg.bChanged == false) return;

	strcpy(pSndFile->ChnSettings[nChn].szName, dlg.m_sName);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS);
}


void CViewPattern::SetSplitKeyboardSettings()
//-------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return;

	CSplitKeyboadSettings dlg(CMainFrame::GetMainFrame(), pSndFile, pModDoc->GetSplitKeyboardSettings());
	if (dlg.DoModal() == IDOK)
		pModDoc->UpdateAllViews(NULL, HINT_INSNAMES|HINT_SMPNAMES);
}


void CViewPattern::ExecutePaste(enmPatternPasteModes pasteMode)
//-------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if (pModDoc && IsEditingEnabled_bmsg())
	{
		pModDoc->PastePattern(m_nPattern, m_dwBeginSel, pasteMode);
		InvalidatePattern(FALSE);
		SetFocus();
	}
}


void CViewPattern::OnTogglePCNotePluginEditor()
//---------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if(pModDoc == nullptr) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	if((pSndFile == nullptr) || (!pSndFile->Patterns.IsValidPat(m_nPattern)))
		return;

	MODCOMMAND *mSelStart = nullptr;
	mSelStart = pSndFile->Patterns[m_nPattern].GetpModCommand(GetSelectionStartRow(), GetSelectionStartChan());
	if((mSelStart == nullptr) || (!mSelStart->IsPcNote()))
		return;
	if(mSelStart->instr < 1 || mSelStart->instr > MAX_MIXPLUGINS)
		return;

	PLUGINDEX nPlg = (PLUGINDEX)(mSelStart->instr - 1);
	pModDoc->TogglePluginEditor(nPlg);
}


ROWINDEX CViewPattern::GetRowsPerBeat() const
//-------------------------------------------
{
	CSoundFile *pSndFile;
	if(GetDocument() == nullptr || (pSndFile = GetDocument()->GetSoundFile()) == nullptr)
		return 0;
	if(!pSndFile->Patterns[m_nPattern].GetOverrideSignature())
		return pSndFile->m_nDefaultRowsPerBeat;
	else
		return pSndFile->Patterns[m_nPattern].GetRowsPerBeat();
}


ROWINDEX CViewPattern::GetRowsPerMeasure() const
//----------------------------------------------
{
	CSoundFile *pSndFile;
	if(GetDocument() == nullptr || (pSndFile = GetDocument()->GetSoundFile()) == nullptr)
		return 0;
	if(!pSndFile->Patterns[m_nPattern].GetOverrideSignature())
		return pSndFile->m_nDefaultRowsPerMeasure;
	else
		return pSndFile->Patterns[m_nPattern].GetRowsPerMeasure();
}


void CViewPattern::SetSelectionInstrument(const INSTRUMENTINDEX nIns)
//-------------------------------------------------------------------
{
	CModDoc *pModDoc;
	CSoundFile *pSndFile;
	MODCOMMAND *p;
	bool bModified = false;

	if (!nIns) return;
	if ((pModDoc = GetDocument()) == nullptr) return;
	pSndFile = pModDoc->GetSoundFile();
	if(!pSndFile) return;
	p = pSndFile->Patterns[m_nPattern];
	if (!p) return;
	BeginWaitCursor();
	PrepareUndo(m_dwBeginSel, m_dwEndSel);

	//rewbs: re-written to work regardless of selection
	UINT startRow  = GetSelectionStartRow();
	UINT endRow    = GetSelectionEndRow();
	UINT startChan = GetSelectionStartChan();
	UINT endChan   = GetSelectionEndChan();

	for (UINT r=startRow; r<endRow+1; r++)
	{
		p = pSndFile->Patterns[m_nPattern] + r * pSndFile->m_nChannels + startChan;
		for (UINT c = startChan; c < endChan + 1; c++, p++)
		{
			// If a note or an instr is present on the row, do the change, if required.
			// Do not set instr if note and instr are both blank.
			// Do set instr if note is a PC note and instr is blank.
			if ( ((p->note > 0 && p->note < NOTE_MIN_SPECIAL) || p->IsPcNote() || p->instr) && (p->instr!=nIns) )
			{
				p->instr = nIns;
				bModified = true;
			}
		}
	}

	if (bModified)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, HINT_PATTERNDATA | (m_nPattern << HINT_SHIFT_PAT), NULL);
	}
	EndWaitCursor();
}


// Select a whole beat (selectBeat = true) or measure.
void CViewPattern::SelectBeatOrMeasure(bool selectBeat)
//-----------------------------------------------------
{
	const ROWINDEX adjust = selectBeat ? GetRowsPerBeat() : GetRowsPerMeasure();

	// Snap to start of beat / measure of upper-left corner of current selection
	const ROWINDEX startRow = GetSelectionStartRow() - (GetSelectionStartRow() % adjust);
	// Snap to end of beat / measure of lower-right corner of current selection
	const ROWINDEX endRow = GetSelectionEndRow() + adjust - (GetSelectionEndRow() % adjust) - 1;

	CHANNELINDEX startChannel = GetSelectionStartChan(), endChannel = GetSelectionEndChan();
	UINT startColumn = 0, endColumn = 0;
	
	if(m_dwBeginSel == m_dwEndSel)
	{
		// No selection has been made yet => expand selection to whole channel.
		endColumn = LAST_COLUMN;	// Extend to param column
	} else if(startRow == GetSelectionStartRow() && endRow == GetSelectionEndRow())
	{
		// Whole beat or measure is already selected
		if(GetColTypeFromCursor(m_dwBeginSel) == NOTE_COLUMN && GetColTypeFromCursor(m_dwEndSel) == LAST_COLUMN)
		{
			// Whole channel is already selected => expand selection to whole row.
			startChannel = startColumn = 0;
			endChannel = MAX_BASECHANNELS;
			endColumn = LAST_COLUMN;
		} else
		{
			// Channel is only partly selected => expand to whole channel first.
			endColumn = LAST_COLUMN;	// Extend to param column
		}
	}
	else
	{
		// Some arbitrary selection: Remember start / end column
		startColumn = GetColTypeFromCursor(m_dwBeginSel);
		endColumn = GetColTypeFromCursor(m_dwEndSel);
	}
	SetCurSel(CreateCursor(startRow, startChannel, startColumn), CreateCursor(endRow, endChannel, endColumn));
}
