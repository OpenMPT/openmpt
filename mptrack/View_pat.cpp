#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "dlg_misc.h"
#include "globals.h"
#include "view_pat.h"
#include "ctrl_pat.h"
#include "dlsbank.h"

#define MAX_SPACING		16

#pragma warning(disable:4244)

MODCOMMAND CViewPattern::m_cmdOld = { 0,0, 0,0, 0,0};
MODCOMMAND CViewPattern::m_cmdFind = { 0,0,0,0,0,0 };
MODCOMMAND CViewPattern::m_cmdReplace = { 0,0,0,0,0,0 };
DWORD CViewPattern::m_dwFindFlags = 0;
DWORD CViewPattern::m_dwReplaceFlags = 0;
UINT CViewPattern::m_nFindMinChn = 0;
UINT CViewPattern::m_nFindMaxChn = 0;


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
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_SYSKEYDOWN()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_MOD_MIDIMSG,		OnMidiMsg)
	ON_COMMAND(ID_EDIT_CUT,			OnEditCut)
	ON_COMMAND(ID_EDIT_COPY,		OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE,		OnEditPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL,	OnEditSelectAll)
	ON_COMMAND(ID_EDIT_SELECTCOLUMN,OnEditSelectColumn)
	ON_COMMAND(ID_EDIT_SELECTCOLUMN2,OnSelectCurrentColumn)
	ON_COMMAND(ID_EDIT_FIND,		OnEditFind)
	ON_COMMAND(ID_EDIT_FINDNEXT,	OnEditFindNext)
	ON_COMMAND(ID_EDIT_RECSELECT,	OnRecordSelect)
	ON_COMMAND(ID_EDIT_UNDO,		OnEditUndo)
	ON_COMMAND(ID_PATTERN_MUTE,		OnMuteChannel)
	ON_COMMAND(ID_PATTERN_SOLO,		OnSolo)
	ON_COMMAND(ID_PATTERN_UNMUTEALL,OnUnmuteAll)
	ON_COMMAND(ID_PATTERN_DELETEROW,OnDeleteRows)
	ON_COMMAND(ID_PATTERN_DELETEALLROW,OnDeleteRowsEx)
	ON_COMMAND(ID_PATTERN_INSERTROW,OnInsertRows)
	ON_COMMAND(ID_PATTERN_PLAY,		OnPatternPlay)
	ON_COMMAND(ID_PATTERN_RESTART,	OnPatternRestart)
	ON_COMMAND(ID_NEXTINSTRUMENT,	OnNextInstrument)
	ON_COMMAND(ID_PREVINSTRUMENT,	OnPrevInstrument)
	ON_COMMAND(ID_PATTERN_PLAYROW,	OnPatternStep)
	ON_COMMAND(ID_CONTROLENTER,		OnPatternStep)
	ON_COMMAND(ID_CONTROLTAB,		OnSwitchToOrderList)
	ON_COMMAND(ID_PREVORDER,		OnPrevOrder)
	ON_COMMAND(ID_NEXTORDER,		OnNextOrder)
	ON_COMMAND(IDC_PATTERN_RECORD,	OnPatternRecord)
	ON_COMMAND(ID_TRANSPOSE_UP,					OnTransposeUp)
	ON_COMMAND(ID_TRANSPOSE_DOWN,				OnTransposeDown)
	ON_COMMAND(ID_TRANSPOSE_OCTUP,				OnTransposeOctUp)
	ON_COMMAND(ID_TRANSPOSE_OCTDOWN,			OnTransposeOctDown)
	ON_COMMAND(ID_PATTERN_PROPERTIES,			OnPatternProperties)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_VOLUME,	OnInterpolateVolume)
	ON_COMMAND(ID_PATTERN_INTERPOLATE_EFFECT,	OnInterpolateEffect)
	ON_COMMAND(ID_PATTERN_SETINSTRUMENT,		OnSetSelInstrument)
	ON_COMMAND(ID_CURSORCOPY,					OnCursorCopy)
	ON_COMMAND(ID_CURSORPASTE,					OnCursorPaste)
	ON_COMMAND(ID_PATTERN_AMPLIFY,				OnPatternAmplify)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO,			OnUpdateUndo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CViewPattern::CViewPattern()
//--------------------------
{
	m_nPattern = 0;
	m_nDetailLevel = 4;
	m_pEditWnd = NULL;
	m_Dib.Init(CMainFrame::bmpNotes);
	UpdateColors();
}


void CViewPattern::OnInitialUpdate()
//----------------------------------
{
	memset(ChnVUMeters, 0, sizeof(ChnVUMeters));
	memset(OldVUMeters, 0, sizeof(OldVUMeters));
	memset(MultiRecordMask, 0, sizeof(MultiRecordMask));
	m_nPlayPat = 0xFFFF;
	m_nPlayRow = 0;
	m_nMidRow = 0;
	m_nDragItem = 0;
	m_bDragging = FALSE;
	m_bInItemRect = FALSE;
	m_bRecord = TRUE;
	m_bContinueSearch = FALSE;
	m_dwBeginSel = m_dwEndSel = m_dwCursor = m_dwStartSel = m_dwDragPos = 0;
	m_dwStatus = 0;
	m_nXScroll = m_nYScroll = 0;
	m_nPattern = m_nRow = 0;
	m_nSpacing = 0;
	m_nAccelChar = 0;
	CScrollView::OnInitialUpdate();
	UpdateSizes();
	UpdateScrollSize();
	SetCurrentPattern(0);
}


BOOL CViewPattern::SetCurrentPattern(UINT npat, int nrow)
//-------------------------------------------------------
{
	CSoundFile *pSndFile;
	CModDoc *pModDoc = GetDocument();
	BOOL bUpdateScroll;
	
	if ((npat >= MAX_PATTERNS) || (!pModDoc)) return FALSE;
	if ((m_pEditWnd) && (m_pEditWnd->IsWindowVisible())) m_pEditWnd->ShowWindow(SW_HIDE);
	pSndFile = pModDoc->GetSoundFile();
	if ((npat < MAX_PATTERNS-1) && (!pSndFile->Patterns[npat])) npat = 0;
	while ((npat > 0) && (!pSndFile->Patterns[npat])) npat--;
	if (!pSndFile->Patterns[npat])
	{
		pSndFile->PatternSize[npat] = 64;
		pSndFile->Patterns[npat] = CSoundFile::AllocatePattern(64, pSndFile->m_nChannels);
	}
	bUpdateScroll = FALSE;
	m_nPattern = npat;
	if ((nrow >= 0) && (nrow != (int)m_nRow) && (nrow < (int)pSndFile->PatternSize[m_nPattern]))
	{
		m_nRow = nrow;
		bUpdateScroll = TRUE;
	}
	if (m_nRow >= pSndFile->PatternSize[m_nPattern]) m_nRow = 0;
	int sel = m_dwCursor | (m_nRow << 16);
	SetCurSel(sel, sel);
	UpdateSizes();
	UpdateScrollSize();
	UpdateIndicator();
	if (bUpdateScroll) SetScrollPos(SB_VERT, m_nRow * GetColumnHeight());
	UpdateScrollPos();
	InvalidatePattern(TRUE);
	SendCtrlMessage(CTRLMSG_PATTERNCHANGED, m_nPattern);
	return TRUE;
}


BOOL CViewPattern::SetCurrentRow(UINT row, BOOL bWrap)
//----------------------------------------------------
{
	CSoundFile *pSndFile;
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return FALSE;
	pSndFile = pModDoc->GetSoundFile();
	CRect rect;
	UINT yofs = GetYScrollPos();
	
	if ((bWrap) && (pSndFile->PatternSize[m_nPattern]))
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
				if ((nCurOrder > 0) && (nCurOrder < MAX_ORDERS) && (m_nPattern == pSndFile->Order[nCurOrder]))
				{
					UINT nPrevPat = pSndFile->Order[nCurOrder-1];
					if ((nPrevPat < MAX_PATTERNS) && (pSndFile->PatternSize[nPrevPat]))
					{
						SendCtrlMessage(CTRLMSG_SETCURRENTORDER, nCurOrder-1);
						if (SetCurrentPattern(nPrevPat)) return SetCurrentRow(pSndFile->PatternSize[nPrevPat]-1);
					}
				}
				row = 0;
			} else
			if (CMainFrame::m_dwPatternSetup & PATTERN_WRAP)
			{
				if ((int)row < (int)0) row += pSndFile->PatternSize[m_nPattern];
				row %= pSndFile->PatternSize[m_nPattern];
			}
		} else
		if (row >= pSndFile->PatternSize[m_nPattern])
		{
			if (m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_MOUSEDRAGSEL))
			{
				row = pSndFile->PatternSize[m_nPattern]-1;
			} else
			if (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL)
			{
				UINT nCurOrder = SendCtrlMessage(CTRLMSG_GETCURRENTORDER);
				if ((nCurOrder+1 < MAX_ORDERS) && (m_nPattern == pSndFile->Order[nCurOrder]))
				{
					UINT nNextPat = pSndFile->Order[nCurOrder+1];
					if ((nNextPat < MAX_PATTERNS) && (pSndFile->PatternSize[nNextPat]))
					{
						SendCtrlMessage(CTRLMSG_SETCURRENTORDER, nCurOrder+1);
						if (SetCurrentPattern(nNextPat)) return SetCurrentRow(0);
					}
				}
				row = pSndFile->PatternSize[m_nPattern]-1;
			} else
			if (CMainFrame::m_dwPatternSetup & PATTERN_WRAP)
			{
				row %= pSndFile->PatternSize[m_nPattern];
			}
		}
	}
	if ((row >= pSndFile->PatternSize[m_nPattern]) || (!m_szCell.cy)) return FALSE;
	GetClientRect(&rect);
	rect.top += m_szHeader.cy;
	int numrows = (rect.bottom - rect.top - 1) / m_szCell.cy;
	if (numrows < 1) numrows = 1;
	if (m_nMidRow)
	{
		UINT newofs = row;
		InvalidateRow();
		if (newofs != yofs)
		{
			CSize sz;
			sz.cx = 0;
			sz.cy = (int)(newofs - yofs) * m_szCell.cy;
			OnScrollBy(sz, TRUE);
		}
		m_nRow = row;
		InvalidateRow();
	} else
	{
		InvalidateRow();
		if (row < yofs)
		{
			CSize sz;
			sz.cx = 0;
			sz.cy = (int)(row - yofs) * m_szCell.cy;
			OnScrollBy(sz, TRUE);
		} else
		if (row > yofs + (UINT)numrows - 1)
		{
			CSize sz;
			sz.cx = 0;
			sz.cy = (int)(row - yofs - numrows + 1) * m_szCell.cy;
			OnScrollBy(sz, TRUE);
		}
		m_nRow = row;
		InvalidateRow();
	}
	int sel = m_dwCursor | (m_nRow << 16);
	int sel0 = sel;
	if ((m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_MOUSEDRAGSEL))
	 && (!(m_dwStatus & PATSTATUS_DRAGNDROPEDIT))) sel0 = m_dwStartSel;
	SetCurSel(sel0, sel);
	UpdateIndicator();
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
	if ((ncol >> 3) >= pSndFile->m_nChannels) return FALSE;
	m_dwCursor = ncol;
	int sel = m_dwCursor | (m_nRow << 16);
	int sel0 = sel;
	if ((m_dwStatus & (PATSTATUS_KEYDRAGSEL|PATSTATUS_MOUSEDRAGSEL))
	 && (!(m_dwStatus & PATSTATUS_DRAGNDROPEDIT))) sel0 = m_dwStartSel;
	SetCurSel(sel0, sel);
	int xofs = GetXScrollPos();
	int nchn = ncol >> 3;
	if (nchn < xofs)
	{
		CSize size(0,0);
		size.cx = nchn * m_szCell.cx - GetScrollPos(SB_HORZ);
		UpdateWindow();
		if (OnScrollBy(size, TRUE)) UpdateWindow();
	} else
	if (nchn > xofs)
	{
		CRect rect;
		GetClientRect(&rect);
		UINT nw = GetColumnWidth();
		int maxcol = (rect.right - m_szHeader.cx) / nw;
		if ((nchn >= (xofs+maxcol)) && (maxcol >= 0))
		{
			CSize size(0,0);
			size.cx = (nchn - maxcol + 1) * m_szCell.cx - GetScrollPos(SB_HORZ);
			UpdateWindow();
			if (OnScrollBy(size, TRUE)) UpdateWindow();
		}
	}
	UpdateIndicator();
	return TRUE;
}


DWORD CViewPattern::GetDragItem(CPoint point, LPRECT lpRect)
//----------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CSoundFile *pSndFile;
	CRect rcClient, rect;
	UINT n, nmax;
	int xofs, yofs;

	if (!pModDoc) return 0;
	GetClientRect(&rcClient);
	xofs = GetXScrollPos();
	yofs = GetYScrollPos();
	rect.SetRect(m_szHeader.cx, 0, m_szHeader.cx + GetColumnWidth() - 2, m_szHeader.cy);
	pSndFile = pModDoc->GetSoundFile();
	nmax = pSndFile->m_nChannels;
	// Checking channel headers
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
	if ((pSndFile->Patterns[m_nPattern]) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)))
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
	row = dwPos >> 16;
	if (row < (int)pSndFile->PatternSize[m_nPattern])
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
	col = (dwPos & 0xFFFF) >> 3;
	if (col < (int)pSndFile->m_nChannels)
	{
		int maxcol = (rect.right - m_szHeader.cx) - 4;
		maxcol -= GetColumnOffset(dwPos&7);
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


BOOL CViewPattern::EnterNote(UINT nNote, UINT nIns, BOOL bNoOvr, int vol, BOOL bMultiCh)
//--------------------------------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		UINT nChn = (m_dwCursor & 0xFFFF) >> 3;
		if ((m_nPattern < MAX_PATTERNS) && (pSndFile->Patterns[m_nPattern])
		 && (m_nRow < pSndFile->PatternSize[m_nPattern]) && (nChn < pSndFile->m_nChannels))
		{
			MODCOMMAND *pbase = pSndFile->Patterns[m_nPattern] + m_nRow * pSndFile->m_nChannels;
			if ((bMultiCh) && (pbase[nChn].note) && (nNote < 0x80))
			{
				for (UINT i=0; i<pSndFile->m_nChannels; i++) if (i != nChn)
				{
					if ((MultiRecordMask[i/8] & (1 << (i&7))) && (!pbase[i].note))
					{
						nChn = i;
						break;
					}
				}
			}
			if ((nNote < 0xFE) && (nNote & 0x80) && (bMultiCh))
			{
				for (UINT i=0; i<pSndFile->m_nChannels; i++)
				{
					BOOL bFound = FALSE;
					if ((MultiRecordMask[i/8] & (1 << (i&7))) && (!pbase[i].note))
					{
						for (int row = m_nRow; row>=0; row--)
						{
							MODCOMMAND *m = pSndFile->Patterns[m_nPattern] + row*pSndFile->m_nChannels + i;
							if (m->note)
							{
								if (m->note == (nNote & 0x7F))
								{
									bFound = TRUE;
									nChn = i;
								}
								break;
							}
						}
					}
					if (bFound) break;
				}
				nNote = 0xFF;
			}
			MODCOMMAND *p = pbase + nChn;
			if ((bNoOvr) && (p->note)) return FALSE;
			p->note = nNote;
			p->instr = nIns;
			if ((vol > 0) && ((!bNoOvr) || (!p->volcmd)))
			{
				if (vol > 64) vol = 64;
				if ((!p->volcmd) || (p->volcmd == VOLCMD_VOLUME))
				{
					p->volcmd = VOLCMD_VOLUME;
					p->vol = vol;
				}
			}
			DWORD sel = (m_nRow << 16) | (nChn << 3);
			InvalidateArea(sel, sel+5);
			pModDoc->SetModified();
			return TRUE;
		}
	}
	return FALSE;
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
		m_pEditWnd->ShowEditWindow(m_nPattern, (m_nRow << 16) | m_dwCursor);
		return TRUE;
	}
	return FALSE;
}


BOOL CViewPattern::PrepareUndo(DWORD dwBegin, DWORD dwEnd)
//--------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	int x,y,cx,cy;

	x = (dwBegin & 0xFFFF) >> 3;
	y = (dwBegin >> 16);
	cx = ((dwEnd & 0xFFFF) >> 3) - x + 1;
	cy = (dwEnd >> 16) - y + 1;
	if ((x < 0) || (y < 0) || (cx < 1) || (cy < 1)) return FALSE;
	if (pModDoc) return pModDoc->PrepareUndo(m_nPattern, x, y, cx, cy);
	return FALSE;
}


BOOL CViewPattern::PreTranslateMessage(MSG *pMsg)
//-----------------------------------------------
{
	if (pMsg)
	{
		if ((pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
		{
			UINT nChar = pMsg->wParam;
			if (CheckCustomKeys(nChar, pMsg->lParam))
			{
				m_nAccelChar = nChar;
				m_dwStatus &= ~PATSTATUS_CTRLDRAGSEL;
				return TRUE;
			}
			switch(nChar)
			{
			// Spacing shortcuts: Alt+0..9
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (CMainFrame::gnHotKeyMask == HOTKEYF_ALT)
				{
					UINT n = nChar - '0';
					if (n != m_nSpacing)
					{
						m_nSpacing = n;
						PostCtrlMessage(CTRLMSG_SETSPACING, m_nSpacing);
						return TRUE;
					}
				}
				if (pMsg->message == WM_SYSKEYDOWN) return TRUE;
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
	if (m_pEditWnd)
	{
		delete m_pEditWnd;
		m_pEditWnd = NULL;
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
	m_dwStatus &= ~PATSTATUS_FOCUS;
	InvalidateRow();
}


void CViewPattern::OnEditDelete()
//-------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	MODCOMMAND *p = pSndFile->Patterns[m_nPattern];
	if (!p) return;
	BeginWaitCursor();
	PrepareUndo(m_dwBeginSel, m_dwEndSel);
	DWORD tmp = m_dwEndSel;
	for (UINT row=(m_dwBeginSel >> 16); row<=(m_dwEndSel >> 16); row++)
	{
		for (UINT i=(m_dwBeginSel & 0xFFFF); i<=(m_dwEndSel & 0xFFFF); i++) if ((i & 7) < 5)
		{
			UINT chn = i >> 3;
			if ((chn >= pSndFile->m_nChannels) || (row >= pSndFile->PatternSize[m_nPattern])) continue;
			MODCOMMAND *m = &p[row * pSndFile->m_nChannels + chn];
			switch(i & 7)
			{
			// Clear note
			case 0:	m->note = 0; if (m_dwBeginSel == m_dwEndSel) { m->instr = 0; tmp = m_dwEndSel+1; } break;
			// Clear instrument
			case 1: m->instr = 0; break;
			// Clear Volume Column
			case 2:	m->volcmd = m->vol = 0; break;
			// Clear Command
			case 3:	m->command = 0; break;
			// Clear Command Param
			case 4:	m->param = 0; break;
			}
		}
	}
	if ((tmp & 7) == 3) tmp++;
	InvalidateArea(m_dwBeginSel, tmp);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << 24), NULL);
	EndWaitCursor();
	SetFocus();
}


void CViewPattern::OnEditCut()
//----------------------------
{
	OnEditCopy();
	OnEditDelete();
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


void CViewPattern::OnEditPaste()
//------------------------------
{
	CModDoc *pModDoc = GetDocument();
	
	if (pModDoc)
	{
		pModDoc->PastePattern(m_nPattern, m_dwBeginSel);
		InvalidatePattern(FALSE);
		SetFocus();
	}
}


BOOL CViewPattern::CheckCustomKeys(UINT nChar, DWORD dwFlags)
//-----------------------------------------------------------
{
	DWORD dwKey = nChar | (CMainFrame::gnHotKeyMask << 16);
	UINT nId = CMainFrame::IsHotKey(dwKey);
	if (nId != NULL)
	{
		if (!(dwKey & 0xFFFF0000))
		{
			if ((CMainFrame::GetNoteFromKey(nChar, HIWORD(dwFlags)))
			 || ((nChar >= '0') && (nChar <= '9') && (m_dwCursor & 0x07))
			 || ((nChar >= 'A') && (nChar <= 'F') && ((m_dwCursor & 0x07) > 2)))
				return FALSE;			 
		}
		m_nMenuParam = (m_nRow << 16) | m_dwCursor;
		if (((nId == ID_CURSORPASTE) && (CMainFrame::m_dwPatternSetup & PATTERN_AUTOSPACEBAR))
		 || (!(dwFlags & 0x40000000))) PostMessage(WM_COMMAND, nId);
		return TRUE;
	}
	return FALSE;
}


void CViewPattern::ProcessChar(UINT nChar, UINT nFlags)
//-----------------------------------------------------
{
	UINT nPlayChord = 0;
	BYTE chordplaylist[3];
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	
	if (nChar == VK_BACK) return;
	if ((nChar == ' ') && (CMainFrame::m_dwPatternSetup & PATTERN_AUTOSPACEBAR)) nFlags &= ~0x4000;
	if ((pModDoc) && (pMainFrm) && (!(nFlags & 0x4000)))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODCOMMAND *p = pSndFile->Patterns[m_nPattern], *prowbase;
		MODCOMMAND oldcmd;
		UINT nChn = (m_dwCursor & 0xFFFF) >> 3;
		UINT nCursor = m_dwCursor & 0x07;
		BOOL bNewNote = FALSE, bNoteEntered = FALSE, bChordEntered = FALSE;
		UINT nPlayNote = 0, nPlayIns = 0;

		if ((nChar >= 'a') && (nChar <= 'z')) nChar -= (UINT)('a' - 'A');
		if ((!p) || (m_nRow >= pSndFile->PatternSize[m_nPattern]) || (nChn >= pSndFile->m_nChannels)) return;
		prowbase = p + m_nRow * pSndFile->m_nChannels;
		p = prowbase + nChn;
		oldcmd = *p;
		// Editing note
		if ((nCursor == 0) || ((nCursor == 1) && ((nChar > '9') || (nChar == ' ')))
			|| (!(m_dwStatus & PATSTATUS_RECORD)))
		{
			UINT nNote = pMainFrm->GetNoteFromKey(nChar, nFlags);
			BOOL bSpecial = FALSE;
			if ((!nNote) && (nChar >= '0') && (nChar <= '9') && (oldcmd.note > 0) && (oldcmd.note < 128))
			{
				nNote = ((oldcmd.note-1)%12)+(nChar-'0')*12+1;
				bSpecial = TRUE;
			}
			if (nChar == ' ')
			{
				if (nCursor != 1) p->note = m_cmdOld.note;
				p->instr = m_cmdOld.instr;
				bNewNote = TRUE;
			} else
			if (nNote)
			{
				p->note = nNote;
				if ((nNote < 128) && (!bSpecial))
				{
					UINT nins = GetCurrentInstrument();
					if (nins) p->instr = nins;
					// Check for chords
					if (m_dwStatus & PATSTATUS_KEYDRAGSEL)
					{
						PMPTCHORD pChords = pMainFrm->GetChords();
						UINT baseoctave = pMainFrm->GetBaseOctave();
						UINT nchord = nNote - baseoctave * 12 - 1;
						if (nchord < 3*12)
						{
							UINT nchordnote = pChords[nchord].key + baseoctave*12 + 1;
							if (nchordnote <= 120)
							{
								UINT nchordch = nChn, nchno = 0;
								nNote = nchordnote;
								p->note = nNote;
								for (UINT kchrd=1; kchrd<pSndFile->m_nChannels; kchrd++)
								{
									if ((nchno > 2) || (!pChords[nchord].notes[nchno])) break;
									if (++nchordch >= pSndFile->m_nChannels) nchordch = 0;
									UINT n = ((nchordnote-1)/12) * 12 + pChords[nchord].notes[nchno];
									if (m_dwStatus & PATSTATUS_RECORD)
									{
										if ((nchordch != nChn) && (MultiRecordMask[nchordch>>3] & (1 << (nchordch&7))) && (n <= 120))
										{
											prowbase[nchordch].note = n;
											if (nins) prowbase[nchordch].instr = nins;
											bChordEntered = TRUE;
											nchno++;
											if (CMainFrame::m_dwPatternSetup & PATTERN_PLAYNEWNOTE)
											{
												if ((n) && (n<=120)) chordplaylist[nPlayChord++] = n;
											}
										}
									} else
									{
										nchno++;
										if ((n) && (n<=120)) chordplaylist[nPlayChord++] = n;
									}
								}
							}
						}
					} // chords
				}
				if ((!p->instr) && (nNote < 128))
				{
					MODCOMMAND *search = p;
					UINT srow = m_nRow;
					while (srow > 0)
					{
						srow--;
						search -= pSndFile->m_nChannels;
						if (search->instr)
						{
							nPlayIns = search->instr;
							break;
						}
					}
				}
				if ((CMainFrame::m_dwPatternSetup & PATTERN_PLAYNEWNOTE) || (!(m_dwStatus & PATSTATUS_RECORD)))
				{
					nPlayNote = p->note;
					if (p->instr) nPlayIns = p->instr;
				}
				bNewNote = TRUE;
				bNoteEntered = TRUE;
			} else
			if (nChar == '.')
			{
				p->note = 0;
				bNewNote = TRUE;
			}
		}
		// Editing Instrument
		if (nCursor == 1)
		{
			if ((nChar >= '0') && (nChar <= '9'))
			{
				UINT instr  = p->instr;
				instr = ((instr * 10) + nChar - '0') % 1000;
				if (instr > MAX_SAMPLES) instr = instr % 100;
				if ((pSndFile->m_nSamples < 100) && (pSndFile->m_nInstruments < 100) && (instr >= 100)) instr = instr % 100;
				p->instr = instr;
			} else
			if (nChar == '.')
			{
				p->instr = 0;
			}
		}
		// Editing Volume
		if (nCursor == 2)
		{
			UINT volcmd = p->volcmd;
			UINT vol = p->vol;
			if ((nChar >= '0') && (nChar <= '9'))
			{
				vol = ((vol * 10) + nChar - '0') % 100;
				if (!volcmd) volcmd = VOLCMD_VOLUME;
			} else
			switch(nChar)
			{
			case ' ':	vol = m_cmdOld.vol; volcmd = m_cmdOld.volcmd; bNewNote = TRUE; break;
			case 'V':	volcmd = VOLCMD_VOLUME; break;
			case 'P':	volcmd = VOLCMD_PANNING; break;
			case 'C':	volcmd = VOLCMD_VOLSLIDEUP; break;
			case 'D':	volcmd = VOLCMD_VOLSLIDEDOWN; break;
			case 'A':	volcmd = VOLCMD_FINEVOLUP; break;
			case 'B':	volcmd = VOLCMD_FINEVOLDOWN; break;
			case 'U':	volcmd = VOLCMD_VIBRATOSPEED; break;
			case 'H':	volcmd = VOLCMD_VIBRATO; break;
			case 'L':	if (pSndFile->m_nType & MOD_TYPE_XM) volcmd = VOLCMD_PANSLIDELEFT; break;
			case 'R':	if (pSndFile->m_nType & MOD_TYPE_XM) volcmd = VOLCMD_PANSLIDERIGHT; break;
			case 'G':	volcmd = VOLCMD_TONEPORTAMENTO; break;
			case 'F':	if (pSndFile->m_nType & MOD_TYPE_IT) volcmd = VOLCMD_PORTAUP; break;
			case 'E':	if (pSndFile->m_nType & MOD_TYPE_IT) volcmd = VOLCMD_PORTADOWN; break;
			case '.':	volcmd = vol = 0; break;
			}
			if ((pSndFile->m_nType & MOD_TYPE_MOD) && (volcmd > VOLCMD_PANNING)) volcmd = vol = 0;
			UINT max = 64;
			if (volcmd > VOLCMD_PANNING)
			{
				max = (pSndFile->m_nType == MOD_TYPE_XM) ? 0x0F : 9;
			}
			if (vol > max) vol %= 10;
			p->volcmd = volcmd;
			p->vol = vol;
		}
		// Editing Command
		if ((nCursor == 3) || ((nCursor == 4) && (nChar > 'F')))
		{
			if (nChar == ' ')
			{
				p->command = m_cmdOld.command;
				p->param = m_cmdOld.param;
				bNewNote = TRUE;
			} else
			if (nChar == '.')
			{
				p->command = p->param = 0;
			} else
			{
				LPCSTR lpcmd = (pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) ? gszModCommands : gszS3mCommands;
				for (UINT i=0; lpcmd[i]; i++) if (lpcmd[i] != '?')
				{
					if (nChar == (UINT)lpcmd[i])
					{
						if (i)
						{
							if ((i == m_cmdOld.command) && (!p->param) && (!p->command)) p->param = m_cmdOld.param;
							else m_cmdOld.param = 0;
							m_cmdOld.command = i;
						}
						p->command = i;
						break;
					}
				}
			}
		}
		// Editing Effect Value
		if ((nCursor == 4) || ((nCursor == 3) && (pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT)) && (nChar <= '9')))
		{
			if (nChar == ' ')
			{
				p->command = m_cmdOld.command;
				p->param = m_cmdOld.param;
				bNewNote = TRUE;
			} else
			if ((nChar >= '0') && (nChar <= '9'))
			{
				p->param = (p->param << 4) | (nChar - '0');
				if (p->command == m_cmdOld.command) m_cmdOld.param = p->param;
			} else
			if ((nChar >= 'A') && (nChar <= 'F'))
			{
				p->param = (p->param << 4) | (nChar - 'A' + 0x0A);
				if (p->command == m_cmdOld.command) m_cmdOld.param = p->param;
			}
		}
		// Check for MOD/XM Speed/Tempo command
		if ((pSndFile->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))
		 && ((p->command == CMD_SPEED) || (p->command == CMD_TEMPO)))
		{
			UINT maxspd = (pSndFile->m_nType & MOD_TYPE_XM) ? 0x1F : 0x20;
			p->command = (p->param <= maxspd) ? CMD_SPEED : CMD_TEMPO;
		}
		// Check for note to play
		if (nPlayNote)
		{
			BOOL bNotPlaying = ((pMainFrm->GetModPlaying() == pModDoc) && (pMainFrm->IsPlaying())) ? FALSE : TRUE;
			pModDoc->PlayNote(nPlayNote, nPlayIns, 0, bNotPlaying);
			for (UINT kplchrd=0; kplchrd<nPlayChord; kplchrd++)
			{
				if (chordplaylist[kplchrd])
				{
					pModDoc->PlayNote(chordplaylist[kplchrd], nPlayIns, 0, FALSE);
					m_dwStatus |= PATSTATUS_CHORDPLAYING;
				}
			}
		}
		// Done
		if (m_dwStatus & PATSTATUS_RECORD)
		{
			DWORD sel = (m_nRow << 16) | m_dwCursor;
			SetCurSel(sel, sel);
			sel &= ~7;
			if ((memcmp(&oldcmd, p, sizeof(MODCOMMAND))) || (bChordEntered))
			{
				pModDoc->SetModified();
				if (bChordEntered) InvalidateRow();
				else InvalidateArea(sel, sel+5);
				UpdateIndicator();
			}
			if ((bNewNote) && ((pMainFrm->GetFollowSong(pModDoc) != m_hWnd) || (pSndFile->IsPaused())
			 || (!(m_dwStatus & PATSTATUS_FOLLOWSONG))))
			{
				if ((m_nSpacing > 0) && (m_nSpacing <= MAX_SPACING)) SetCurrentRow(m_nRow+m_nSpacing);
				if ((bChordEntered) || (bNoteEntered) || (nChar == ' '))
				{
					DWORD sel = m_dwCursor | (m_nRow << 16);
					SetCurSel(sel, sel);
				}
			}
			if ((bNoteEntered) && (MultiRecordMask[nChn>>3] & (1 << (nChn&7))) && (!bChordEntered))
			{
				UINT n = nChn;
				for (UINT i=0; i<pSndFile->m_nChannels; i++)
				{
					if (++n > pSndFile->m_nChannels) n = 0;
					if (MultiRecordMask[n>>3] & (1 << (n&7)))
					{
						SetCurrentColumn(n<<3);
						break;
					}
				}
			}
		} else
		{
			// recording disabled
			*p = oldcmd;
		}
	}
}


void CViewPattern::OnLButtonDown(UINT, CPoint point)
//--------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if ((m_bDragging) || (!pModDoc)) return;
	SetFocus();
	m_nDragItem = GetDragItem(point, &m_rcDragItem);
	m_bDragging = TRUE;
	m_bInItemRect = TRUE;
	SetCapture();
	if ((point.x >= m_szHeader.cx) && (point.y > m_szHeader.cy))
	{
		m_dwStartSel = GetPositionFromPoint(point);
		if (((m_dwStartSel & 0xFFFF) >> 3) < pModDoc->GetNumChannels())
		{
			m_dwStatus |= PATSTATUS_MOUSEDRAGSEL;

			if (m_dwStatus & PATSTATUS_CTRLDRAGSEL)
			{
				SetCurSel(m_dwStartSel, m_dwStartSel);
			}
			if ((CMainFrame::m_dwPatternSetup & PATTERN_DRAGNDROPEDIT)
			 && ((m_dwBeginSel != m_dwEndSel) || (m_dwStatus & PATSTATUS_CTRLDRAGSEL))
			 && ((m_dwStartSel >> 16) >= (m_dwBeginSel >> 16))
			 && ((m_dwStartSel >> 16) <= (m_dwEndSel >> 16))
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
				SetCurrentRow(m_dwStartSel >> 16);
				SetCurrentColumn(m_dwStartSel & 0xFFFF);
			}
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
	if ((dwPos == ((m_nRow << 16) | m_dwCursor)) && (point.y >= m_szHeader.cy))
	{
		if (ShowEditWindow()) return;
	}
	OnLButtonDown(uFlags, point);
}


void CViewPattern::OnLButtonUp(UINT nFlags, CPoint point)
//-------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	BOOL bItemSelected = m_bInItemRect;
	if ((!m_bDragging) || (!pModDoc)) return;
	m_bDragging = FALSE;
	m_bInItemRect = FALSE;
	ReleaseCapture();
	m_dwStatus &= ~PATSTATUS_MOUSEDRAGSEL;
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
	 && ((m_nDragItem & DRAGITEM_MASK) != DRAGITEM_PATTERNHEADER))
	{
		if ((m_nMidRow) && (m_dwBeginSel == m_dwEndSel))
		{
			DWORD dwPos = m_dwBeginSel;
			SetCurrentRow(dwPos >> 16);
			SetCurrentColumn(dwPos & 0xFFFF);
		}
	}
	if ((!bItemSelected) || (!m_nDragItem)) return;
	InvalidateRect(&m_rcDragItem, FALSE);
	DWORD nItemNo = m_nDragItem & 0xFFFF;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	switch(m_nDragItem & DRAGITEM_MASK)
	{
	case DRAGITEM_CHNHEADER:
		if (nFlags & MK_SHIFT)
		{
			if (nItemNo < MAX_CHANNELS)
			{
				MultiRecordMask[nItemNo>>3] ^= (1 << (nItemNo & 7));
				InvalidateChannelsHeaders();
			}
		} else
		{
			pModDoc->MuteChannel(nItemNo, (pSndFile->ChnSettings[nItemNo].dwFlags & CHN_MUTE) ? FALSE : TRUE);
		}
		break;
	case DRAGITEM_PATTERNHEADER:
		OnPatternProperties();
		break;
	}
}


void CViewPattern::OnPatternProperties()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
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

	if ((!pModDoc) || (pt.x < m_szHeader.cx)) return;
	if (m_dwStatus & PATSTATUS_DRAGNDROPEDIT)
	{
		if (m_dwStatus & PATSTATUS_DRAGNDROPPING)
		{
			OnDrawDragSel();
			m_dwStatus &= ~PATSTATUS_DRAGNDROPPING;
		}
		m_dwStatus &= ~(PATSTATUS_DRAGNDROPEDIT|PATSTATUS_MOUSEDRAGSEL);
		if (m_bDragging)
		{
			m_bDragging = FALSE;
			m_bInItemRect = FALSE;
			ReleaseCapture();
		}
		SetCursor(CMainFrame::curArrow);
		return;
	}
	if ((hMenu = ::CreatePopupMenu()) == NULL) return;
	pSndFile = pModDoc->GetSoundFile();
	m_nMenuParam = GetPositionFromPoint(pt);
	// Right-click outside selection ?
	if (((m_nMenuParam >> 16) < (m_dwBeginSel >> 16))
	 || ((m_nMenuParam >> 16) > (m_dwEndSel >> 16))
	 || ((m_nMenuParam & 0xFFFF) < (m_dwBeginSel & 0xFFFF))
	 || ((m_nMenuParam & 0xFFFF) > (m_dwEndSel & 0xFFFF)))
	{
		SetCurrentRow(m_nMenuParam >> 16);
		SetCurrentColumn(m_nMenuParam & 0xFFFF);
	}
	UINT nChn = (m_nMenuParam & 0xFFFF) >> 3;
	if ((nChn < pSndFile->m_nChannels) && (pSndFile->Patterns[m_nPattern]))
	{
		BOOL bSep = FALSE;
		{
			BOOL b, bAll;
			if (pt.y <= m_szHeader.cy) AppendMenu(hMenu, (pSndFile->ChnSettings[nChn].dwFlags & CHN_MUTE) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_PATTERN_MUTE, "Mute Channel");
			b = FALSE;
			bAll = FALSE;
			for (UINT i=0; i<pSndFile->m_nChannels; i++)
			{
				if (i != nChn)
				{
					if (!(pSndFile->ChnSettings[i].dwFlags & CHN_MUTE)) b = TRUE;
				} else
				{
					if (pSndFile->ChnSettings[i].dwFlags & CHN_MUTE) b = TRUE;
				}
				if (pSndFile->ChnSettings[i].dwFlags & CHN_MUTE) bAll = TRUE;
			}
			if (b) AppendMenu(hMenu, MF_STRING, ID_PATTERN_SOLO, "Solo");
			if (bAll) AppendMenu(hMenu, MF_STRING, ID_PATTERN_UNMUTEALL, "Unmute All");
			if (pt.y <= m_szHeader.cy) AppendMenu(hMenu, (MultiRecordMask[nChn>>3] & (1 << (nChn & 7))) ? (MF_STRING|MF_CHECKED) : MF_STRING, ID_EDIT_RECSELECT, "Record select");
			bSep = TRUE;
		}
		if ((pt.x >= m_szHeader.cx) && (pt.y > m_szHeader.cy))
		{
			AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECTCOLUMN, "Select Column\tCtrl+L");
			AppendMenu(hMenu, MF_STRING, ID_EDIT_SELECT_ALL, "Select Pattern\tCtrl+5");
			AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			bSep = FALSE;
			// Interpolate ?
			if (((m_dwBeginSel & 0xFFFF0000) <  (m_dwEndSel & 0xFFFF0000))
			 && ((m_dwBeginSel & 0x0000FFF8) == (m_dwEndSel & 0x0000FFF8)))
			{
				UINT row0 = m_dwBeginSel >> 16, row1 = m_dwEndSel >> 16, nch = (m_dwBeginSel & 0xFFFF) >> 3;
				UINT ncc0 = m_dwBeginSel & 7, ncc1 = m_dwEndSel & 7;
				MODCOMMAND *pcmd = pSndFile->Patterns[m_nPattern];
				if ((row1 < pSndFile->PatternSize[m_nPattern]) && (nch < pSndFile->m_nChannels))
				{
					row0 *= pSndFile->m_nChannels;
					row1 *= pSndFile->m_nChannels;
					// Volume Column ?
					if ((pcmd[row0+nch].volcmd == pcmd[row1+nch].volcmd) && (ncc0 < 3))
					{
						if (pcmd[row0+nch].volcmd == VOLCMD_VOLUME)
						{
							AppendMenu(hMenu, MF_STRING, ID_PATTERN_INTERPOLATE_VOLUME, "Interpolate Volume\tCtrl+J");
							bSep = TRUE;
						} else
						if (pcmd[row0+nch].volcmd == VOLCMD_PANNING)
						{
							AppendMenu(hMenu, MF_STRING, ID_PATTERN_INTERPOLATE_VOLUME, "Interpolate Panning\tCtrl+J");
							bSep = TRUE;
						}
					}
					// Effect Column ?
					if ((pcmd[row0+nch].command == pcmd[row1+nch].command) && (pcmd[row0+nch].command) && (ncc1 >= 3))
					{
						AppendMenu(hMenu, MF_STRING, ID_PATTERN_INTERPOLATE_EFFECT, "Interpolate Effect\tCtrl+K");
						bSep = TRUE;
					}
				}
			}
			// Change Instrument
			UINT ninscol = (m_dwBeginSel & 0xFFF8) | 1;
			if (ninscol < (m_dwBeginSel & 0xFFFF)) ninscol += 8;
			if ((ninscol >= (m_dwBeginSel & 0xFFFF)) && (ninscol <= (m_dwEndSel & 0xFFFF)))
			{
				if (GetCurrentInstrument())
				{
					AppendMenu(hMenu, MF_STRING, ID_PATTERN_SETINSTRUMENT, "Change Instrument\tCtrl+I");
					bSep = TRUE;
				}
			}
			// Transpose
			if ((!(m_dwBeginSel & 7)) || ((m_dwEndSel & 0xFFFF) - (m_dwBeginSel & 0xFFF8) >= 8))
			{
				AppendMenu(hMenu, MF_STRING, ID_TRANSPOSE_UP, "Transpose +1\tCtrl+Q");
				AppendMenu(hMenu, MF_STRING, ID_TRANSPOSE_DOWN, "Transpose -1\tCtrl+A");
				AppendMenu(hMenu, MF_STRING, ID_TRANSPOSE_OCTUP, "Transpose +12\tCtrl+Shift+Q");
				AppendMenu(hMenu, MF_STRING, ID_TRANSPOSE_OCTDOWN, "Transpose -12\tCtrl+Shift+A");
				bSep = TRUE;
			}
			// Amplify
			if (((m_dwBeginSel & 7) > 1) || ((m_dwEndSel & 0xFFFF) - (m_dwBeginSel & 0xFFF8) >= 4))
			{
				AppendMenu(hMenu, MF_STRING, ID_PATTERN_AMPLIFY, "Amplify\tCtrl+M");
				bSep = TRUE;
			}
			if (bSep) AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			AppendMenu(hMenu, MF_STRING, ID_EDIT_CUT, "Cut\tCtrl+X");
			AppendMenu(hMenu, MF_STRING, ID_EDIT_COPY, "Copy\tCtrl+C");
			AppendMenu(hMenu, MF_STRING, ID_EDIT_PASTE, "Paste\tCtrl+V");
			if (pModDoc->CanUndo()) AppendMenu(hMenu, MF_STRING, ID_EDIT_UNDO, "Undo\tCtrl+Z");
			bSep = TRUE;
		}
		if ((m_nMenuParam >> 16) == m_nRow)
		{
			if (bSep) AppendMenu(hMenu, MF_SEPARATOR, 0, "");
			AppendMenu(hMenu, MF_STRING, ID_PATTERN_INSERTROW, "Insert Row\tIns");
			AppendMenu(hMenu, MF_STRING, ID_PATTERN_DELETEROW, "Delete Row\tBack");
		}
		ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
	}
	::DestroyMenu(hMenu);
}


void CViewPattern::OnMouseMove(UINT, CPoint point)
//------------------------------------------------
{
	if (!m_bDragging) return;
	if (m_nDragItem)
	{
		DWORD nItem = GetDragItem(point, NULL);
		BOOL b = (nItem == m_nDragItem) ? TRUE : FALSE;
		if (b != m_bInItemRect)
		{
			m_bInItemRect = b;
			InvalidateRect(&m_rcDragItem, FALSE);
			UpdateWindow();
		}
	}
	if (m_dwStatus & PATSTATUS_MOUSEDRAGSEL)
	{
		CModDoc *pModDoc = GetDocument();
		DWORD dwPos = GetPositionFromPoint(point);
		if ((pModDoc) && (m_nPattern < MAX_PATTERNS))
		{
			UINT row = dwPos >> 16;
			UINT max = pModDoc->GetSoundFile()->PatternSize[m_nPattern];
			if ((row) && (row >= max)) row = max-1;
			dwPos &= 0xFFFF;
			dwPos |= (row << 16);
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
			}
		} else
		// Default: selection
		if (CMainFrame::m_dwPatternSetup & PATTERN_CENTERROW)
		{
			DragToSel(dwPos, TRUE);
		} else
		{
			SetCurrentRow(dwPos >> 16);
			SetCurrentColumn(dwPos & 0xFFFF);
		}
	}
}


void CViewPattern::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//-----------------------------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();

	switch(nChar)
	{
	// Row
	case VK_UP:
		SetCurrentRow(m_nRow - 1, TRUE);
		return;
	case VK_DOWN:
		SetCurrentRow(m_nRow + 1, TRUE);
		return;
	case VK_PRIOR:
		SetCurrentRow(m_nRow - 4, TRUE);
		return;
	case VK_NEXT:
		SetCurrentRow(m_nRow + 4, TRUE);
		return;
	case VK_HOME:
		if (m_dwStatus & PATSTATUS_CTRLDRAGSEL)
		{
			if (m_nRow > 0) SetCurrentRow(0);
		} else
		{
			if (m_dwCursor) SetCurrentColumn(0);
		}
		return;
	case VK_END:
		if (m_dwStatus & PATSTATUS_CTRLDRAGSEL)
		{
			if (m_nRow < pModDoc->GetPatternSize(m_nPattern) - 1)
				SetCurrentRow(pModDoc->GetPatternSize(m_nPattern) - 1);
		} else
		{
			SetCurrentColumn(((pSndFile->m_nChannels-1) << 3) | 4);
		}
		return;
	// Column
	case VK_LEFT:
		if ((CMainFrame::m_dwPatternSetup & PATTERN_WRAP) && (!m_dwCursor))
			SetCurrentColumn((((pSndFile->m_nChannels-1) << 3) | 4));
		else
			SetCurrentColumn(m_dwCursor - 1);
		return;
	case VK_RIGHT:
		if ((CMainFrame::m_dwPatternSetup & PATTERN_WRAP) && (m_dwCursor >= (((pSndFile->m_nChannels-1) << 3) | 4)))
			SetCurrentColumn(0);
		else
			SetCurrentColumn(m_dwCursor + 1);
		return;
	// Tab
	case VK_TAB:
		if (m_dwStatus & PATSTATUS_KEYDRAGSEL)
		{
			if (m_dwCursor >> 3)
			{
				SetCurrentColumn(((((m_dwCursor >> 3) - 1) % pSndFile->m_nChannels) << 3) | (m_dwCursor & 0x07));
			} else
			{
				SetCurrentColumn((m_dwCursor & 0x07) | ((pSndFile->m_nChannels-1) << 3));
			}
			UINT n = (m_nRow << 16) | (m_dwCursor);
			SetCurSel(n, n);
		} else
		{
			SetCurrentColumn(((((m_dwCursor >> 3) + 1) % pSndFile->m_nChannels) << 3) | (m_dwCursor & 0x07));
		}
		return;
	// Shift
	case VK_SHIFT:
	case VK_RSHIFT:
	case VK_LSHIFT:
		if (!(m_dwStatus & PATSTATUS_DRAGNDROPEDIT)) m_dwStartSel = (m_nRow << 16) | m_dwCursor;
		m_dwStatus |= PATSTATUS_KEYDRAGSEL;
		return;
	case VK_CONTROL:
	case VK_LCONTROL:
		if (!(m_dwStatus & PATSTATUS_DRAGNDROPEDIT)) m_dwStartSel = (m_nRow << 16) | m_dwCursor;
		m_dwStatus |= PATSTATUS_CTRLDRAGSEL;
		break;
	case VK_DELETE:
		OnEditDelete();
		return;
	case VK_RETURN:
		return;
	case VK_ADD:
		{
			UINT n = m_nPattern + 1;
			while ((n < MAX_PATTERNS) && (!pSndFile->Patterns[n])) n++;
			SetCurrentPattern((n < MAX_PATTERNS) ? n : 0);
		}
		return;
	case VK_SUBTRACT:
		{
			UINT n = (m_nPattern) ? m_nPattern - 1 : MAX_PATTERNS-1;
			while ((n > 0) && (!pSndFile->Patterns[n])) n--;
			SetCurrentPattern(n);
		}
		return;
	case VK_APPS:
		ShowEditWindow();
		return;
	case VK_BACK:
		OnDeleteRows();
		return;
	case VK_LWIN:
	case VK_RWIN:
		{
			CPoint pt =	GetPointFromPosition((m_nRow << 16) | m_dwCursor);
			OnRButtonDown(0, pt);
		}
		return;
	case VK_CAPITAL:
		if (CMainFrame::m_nKeyboardCfg & KEYBOARD_FT2KEYS) OnChar(nChar, nRepCnt, nFlags);
		break;
	}
	CScrollView::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CViewPattern::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
//---------------------------------------------------------------
{
	switch(nChar)
	{
	// Shift
	case VK_SHIFT:
	case VK_RSHIFT:
	case VK_LSHIFT:
		m_dwStatus &= ~PATSTATUS_KEYDRAGSEL;
		return;

	case VK_CONTROL:
	case VK_LCONTROL:
		m_dwStatus &= ~PATSTATUS_CTRLDRAGSEL;
		break;

	case VK_LWIN:
	case VK_RWIN:
		return;

	default:
		{
			// Key off playing notes
			UINT note = CMainFrame::GetNoteFromKey(nChar, nFlags);
			if (((note) && (note < 120)) || ((nChar >= '0') && (nChar <= '9')))
			{
				CModDoc *pModDoc = GetDocument();
				if (pModDoc)
				{
					if (m_dwStatus & PATSTATUS_CHORDPLAYING)
					{
						m_dwStatus &= ~PATSTATUS_CHORDPLAYING;
						pModDoc->NoteOff(0, TRUE);
					} else
					{
						pModDoc->NoteOff(note, TRUE);
					}
				}
				if ((CMainFrame::m_dwPatternSetup & PATTERN_KBDNOTEOFF) && (note) && (note < 120) && (nChar != m_nAccelChar))
				{
					if ((m_dwCursor & 7) < 2) EnterNote(note|0x80, 0, TRUE, -1, TRUE);
				}
			}
		}
	}
	CScrollView::OnKeyUp(nChar, nRepCnt, nFlags);
}


void CViewPattern::OnChar(UINT nChar, UINT, UINT nFlags)
//------------------------------------------------------
{
	m_nAccelChar = 0;
	ProcessChar(nChar, nFlags);
}


void CViewPattern::OnEditSelectAll()
//----------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		SetCurSel(0, ((pSndFile->PatternSize[m_nPattern]-1) << 16)
		 | ((pSndFile->m_nChannels - 1) << 3)
		 | (4));
	}
}


void CViewPattern::OnEditSelectColumn()
//-------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		SetCurSel(m_nMenuParam & 0xFFF8,
		 ((pSndFile->PatternSize[m_nPattern]-1) << 16)
		 | ((m_nMenuParam & 0xFFF8)+4));
	}
}


void CViewPattern::OnSelectCurrentColumn()
//----------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		DWORD dwBeginSel = m_dwCursor & 0xFFF8;
		DWORD dwEndSel = ((pSndFile->PatternSize[m_nPattern]-1) << 16) | ((m_dwCursor & 0xFFF8)+4);
		// If column is already selected, select the current pattern
		if ((dwBeginSel == m_dwBeginSel) && (dwEndSel == m_dwEndSel))
		{
			dwBeginSel = 0;
			dwEndSel &= 0xFFFF0000;
			dwEndSel |= ((pSndFile->m_nChannels-1) << 3) + 4;
		}
		SetCurSel(dwBeginSel, dwEndSel);
	}
}


void CViewPattern::OnMuteChannel()
//--------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		UINT nChn = (m_nMenuParam & 0xFFFF) >> 3;
		pModDoc->MuteChannel(nChn, !pModDoc->IsChannelMuted(nChn));
		InvalidateChannelsHeaders();
	}
}


void CViewPattern::OnSolo()
//-------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		UINT nNumChn = pModDoc->GetNumChannels();
		UINT nChn = (m_nMenuParam & 0xFFFF) >> 3;
		if (nChn < nNumChn)
		{
			BOOL bSolo = TRUE;
			for (UINT j=0; j<nNumChn; j++)
			{
				BOOL bMuted = pModDoc->IsChannelMuted(j);
				if (j == nChn)
				{
					if (bMuted) bSolo = FALSE;
				} else
				{
					if (!bMuted) bSolo = FALSE;
				}
			}
			for (UINT i=0; i<nNumChn; i++)
			{
				BOOL bMute = (i == nChn) ? FALSE : TRUE;
				if (bSolo) bMute = FALSE; // Unmute All
				pModDoc->MuteChannel(i, bMute);
			}
			InvalidateChannelsHeaders();
		}
	}
}


void CViewPattern::OnRecordSelect()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		UINT nNumChn = pModDoc->GetNumChannels();
		UINT nChn = (m_nMenuParam & 0xFFFF) >> 3;
		if (nChn < nNumChn)
		{
			MultiRecordMask[nChn>>3] ^= (1 << (nChn & 7));
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
		UINT nChns = pModDoc->GetNumChannels();
		for (UINT i=0; i<nChns; i++)
		{
			pModDoc->MuteChannel(i, FALSE);
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

	if (!pModDoc) return;
	pSndFile = pModDoc->GetSoundFile();
	if (!pSndFile->Patterns[m_nPattern]) return;
	row = m_dwBeginSel >> 16;
	maxrow = pSndFile->PatternSize[m_nPattern];
	if (colmax >= pSndFile->m_nChannels) colmax = pSndFile->m_nChannels-1;
	if (colmin > colmax) return;
	pModDoc->PrepareUndo(m_nPattern, 0,0, pSndFile->m_nChannels, maxrow);
	for (UINT r=row; r<maxrow; r++)
	{
		MODCOMMAND *m = pSndFile->Patterns[m_nPattern] + r * pSndFile->m_nChannels + colmin;
		for (UINT c=colmin; c<=colmax; c++, m++)
		{
			if (r + nrows >= maxrow)
			{
				m->note = m->instr = 0;
				m->volcmd = m->vol = 0;
				m->command = m->param = 0;
			} else
			{
				*m = *(m+pSndFile->m_nChannels*nrows);
			}
		}
	}
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << 24), NULL);
	InvalidatePattern(FALSE);
}


void CViewPattern::OnDeleteRows()
//-------------------------------
{
	UINT colmin = (m_dwBeginSel & 0xFFFF) >> 3;
	UINT colmax = (m_dwEndSel & 0xFFFF) >> 3;
	UINT nrows = (m_dwEndSel >> 16) - (m_dwBeginSel >> 16) + 1;
	DeleteRows(colmin, colmax, nrows);
	m_dwEndSel = (m_dwEndSel & 0x0000FFFF) | (m_dwBeginSel & 0xFFFF0000);
}


void CViewPattern::OnDeleteRowsEx()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (!pModDoc) return;
	CSoundFile *pSndFile = pModDoc->GetSoundFile();
	UINT nrows = (m_dwEndSel >> 16) - (m_dwBeginSel >> 16) + 1;
	DeleteRows(0, pSndFile->m_nChannels-1, nrows);
	m_dwEndSel = (m_dwEndSel & 0x0000FFFF) | (m_dwBeginSel & 0xFFFF0000);
}


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
	maxrow = pSndFile->PatternSize[m_nPattern];
	colmin = (m_dwBeginSel & 0xFFFF) >> 3;
	colmax = (m_dwEndSel & 0xFFFF) >> 3;
	for (UINT r=maxrow; r>row; )
	{
		r--;
		MODCOMMAND *m = pSndFile->Patterns[m_nPattern] + r * pSndFile->m_nChannels + colmin;
		for (UINT c=colmin; c<=colmax; c++, m++)
		{
			if (r <= row)
			{
				m->note = m->instr = 0;
				m->volcmd = m->vol = 0;
				m->command = m->param = 0;
			} else
			{
				*m = *(m-pSndFile->m_nChannels);
			}
		}
	}
	InvalidatePattern(FALSE);
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(this, HINT_PATTERNDATA | (m_nPattern << 24), NULL);
}


void CViewPattern::OnEditFind()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CFindReplaceTab pageFind(IDD_EDIT_FIND, FALSE, pModDoc);
		CFindReplaceTab pageReplace(IDD_EDIT_REPLACE, TRUE, pModDoc);
		CPropertySheet dlg("Find/Replace");

		pageFind.m_nNote = m_cmdFind.note;
		pageFind.m_nInstr = m_cmdFind.instr;
		pageFind.m_nVolCmd = m_cmdFind.volcmd;
		pageFind.m_nVol = m_cmdFind.vol;
		pageFind.m_nCommand = m_cmdFind.command;
		pageFind.m_nParam = m_cmdFind.param;
		pageFind.m_dwFlags = m_dwFindFlags;
		pageFind.m_nMinChannel = m_nFindMinChn;
		pageFind.m_nMaxChannel = m_nFindMaxChn;
		pageReplace.m_nNote = m_cmdReplace.note;
		pageReplace.m_nInstr = m_cmdReplace.instr;
		pageReplace.m_nVolCmd = m_cmdReplace.volcmd;
		pageReplace.m_nVol = m_cmdReplace.vol;
		pageReplace.m_nCommand = m_cmdReplace.command;
		pageReplace.m_nParam = m_cmdReplace.param;
		pageReplace.m_dwFlags = m_dwReplaceFlags;
		dlg.AddPage(&pageFind);
		dlg.AddPage(&pageReplace);
		if (dlg.DoModal() == IDOK)
		{
			m_cmdFind.note = pageFind.m_nNote;
			m_cmdFind.instr = pageFind.m_nInstr;
			m_cmdFind.volcmd = pageFind.m_nVolCmd;
			m_cmdFind.vol = pageFind.m_nVol;
			m_cmdFind.command = pageFind.m_nCommand;
			m_cmdFind.param = pageFind.m_nParam;
			m_nFindMinChn = pageFind.m_nMinChannel;
			m_nFindMaxChn = pageFind.m_nMaxChannel;
			m_dwFindFlags = pageFind.m_dwFlags;
			m_cmdReplace.note = pageReplace.m_nNote;
			m_cmdReplace.instr = pageReplace.m_nInstr;
			m_cmdReplace.volcmd = pageReplace.m_nVolCmd;
			m_cmdReplace.vol = pageReplace.m_nVol;
			m_cmdReplace.command = pageReplace.m_nCommand;
			m_cmdReplace.param = pageReplace.m_nParam;
			m_dwReplaceFlags = pageReplace.m_dwFlags;
			m_bContinueSearch = FALSE;
			OnEditFindNext();
		}
	}
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
	if (!(m_dwFindFlags & ~PATSEARCH_FULLSEARCH))
	{
		PostMessage(WM_COMMAND, ID_EDIT_FIND);
		return;
	}
	BeginWaitCursor();
	pSndFile = pModDoc->GetSoundFile();
	nPatStart = m_nPattern;
	nPatEnd = m_nPattern+1;
	if (m_dwFindFlags & PATSEARCH_FULLSEARCH)
	{
		nPatStart = 0;
		nPatEnd = MAX_PATTERNS;
	}
	if (m_bContinueSearch)
	{
		nPatStart = m_nPattern;
	}
	bEffectEx = FALSE;
	if (m_dwFindFlags & PATSEARCH_COMMAND)
	{
		UINT fxndx = pModDoc->GetIndexFromEffect(m_cmdFind.command, m_cmdFind.param);
		bEffectEx = pModDoc->IsExtendedEffect(fxndx);
	}
	for (UINT nPat=nPatStart; nPat<nPatEnd; nPat++)
	{
		LPMODCOMMAND m = pSndFile->Patterns[nPat];
		DWORD len = pSndFile->m_nChannels * pSndFile->PatternSize[nPat];
		if ((!m) || (!len)) continue;
		UINT n = 0;
		if ((m_bContinueSearch) && (nPat == nPatStart) && (nPat == m_nPattern))
		{
			n = GetCurrentRow() * pSndFile->m_nChannels + GetCurrentChannel() + 1;
			m += n;
		}
		for (; n<len; n++, m++)
		{
			BOOL bFound = TRUE, bReplace = TRUE;

			if (m_dwFindFlags & PATSEARCH_CHANNEL)
			{
				UINT ch = n % pSndFile->m_nChannels;
				if ((ch < m_nFindMinChn) || (ch > m_nFindMaxChn)) bFound = FALSE;
			}
			if (((m_dwFindFlags & PATSEARCH_NOTE) && ((m->note != m_cmdFind.note) && ((m_cmdFind.note != 0xFD) || (!m->note) || (m->note & 0x80))))
			 || ((m_dwFindFlags & PATSEARCH_INSTR) && (m->instr != m_cmdFind.instr))
			 || ((m_dwFindFlags & PATSEARCH_VOLCMD) && (m->volcmd != m_cmdFind.volcmd))
			 || ((m_dwFindFlags & PATSEARCH_VOLUME) && (m->vol != m_cmdFind.vol))
			 || ((m_dwFindFlags & PATSEARCH_COMMAND) && (m->command != m_cmdFind.command))
			 || ((m_dwFindFlags & PATSEARCH_PARAM) && (m->param != m_cmdFind.param)))
			{
				bFound = FALSE;
			} else
			if (((m_dwFindFlags & (PATSEARCH_COMMAND|PATSEARCH_PARAM)) == PATSEARCH_COMMAND) && (bEffectEx))
			{
				if ((m->param & 0xF0) != (m_cmdFind.param & 0xF0)) bFound = FALSE;
			}
			// Found!
			if (bFound)
			{
				BOOL bUpdPos = TRUE;
				if ((m_dwReplaceFlags & (PATSEARCH_REPLACEALL|PATSEARCH_REPLACE)) == (PATSEARCH_REPLACEALL|PATSEARCH_REPLACE)) bUpdPos = FALSE;
				nFound++;
				if (bUpdPos)
				{
					SetCurrentPattern(nPat);
					SetCurrentRow(n / pSndFile->m_nChannels);
				}
				UINT ncurs = (n % pSndFile->m_nChannels) << 3;
				if (!(m_dwFindFlags & PATSEARCH_NOTE))
				{
					ncurs++;
					if (!(m_dwFindFlags & PATSEARCH_INSTR))
					{
						ncurs++;
						if (!(m_dwFindFlags & (PATSEARCH_VOLCMD|PATSEARCH_VOLUME)))
						{
							ncurs++;
							if (!(m_dwFindFlags & PATSEARCH_COMMAND)) ncurs++;
						}
					}
				}
				if (bUpdPos)
				{
					SetCurrentColumn(ncurs);
				}
				if (!(m_dwReplaceFlags & PATSEARCH_REPLACE)) goto EndSearch;
				if (!(m_dwReplaceFlags & PATSEARCH_REPLACEALL))
				{
					UINT ans = MessageBox("Replace this occurrence ?", "Replace", MB_YESNOCANCEL);
					if (ans == IDYES) bReplace = TRUE; else
					if (ans == IDNO) bReplace = FALSE; else goto EndSearch;
				}
				if (bReplace)
				{
					if ((m_dwReplaceFlags & PATSEARCH_NOTE))
					{
						// -1 octave
						if (m_cmdReplace.note == 0xFA)
						{
							if (m->note > 12) m->note -= 12;
						} else
						// +1 octave
						if (m_cmdReplace.note == 0xFB)
						{
							if (m->note <= 108) m->note += 12;
						} else
						// Note--
						if (m_cmdReplace.note == 0xFC)
						{
							if (m->note > 1) m->note--;
						} else
						// Note++
						if (m_cmdReplace.note == 0xFD)
						{
							if (m->note < 120) m->note++;
						} else m->note = m_cmdReplace.note;
					}
					if ((m_dwReplaceFlags & PATSEARCH_INSTR))
					{
						// Instr--
						if (m_cmdReplace.instr == 0xFC)
						{
							if (m->instr > 1) m->instr--;
						} else
						// Instr++
						if (m_cmdReplace.instr == 0xFD)
						{
							if (m->instr < MAX_INSTRUMENTS-1) m->instr++;
						} else m->instr = m_cmdReplace.instr;
					}
					if ((m_dwReplaceFlags & PATSEARCH_VOLCMD))
					{
						m->volcmd = m_cmdReplace.volcmd;
					}
					if ((m_dwReplaceFlags & PATSEARCH_VOLUME))
					{
						m->vol = m_cmdReplace.vol;
					}
					if ((m_dwReplaceFlags & PATSEARCH_COMMAND))
					{
						m->command = m_cmdReplace.command;
					}
					if ((m_dwReplaceFlags & PATSEARCH_PARAM))
					{
						if ((bEffectEx) && (!(m_dwReplaceFlags & PATSEARCH_COMMAND)))
							m->param = (BYTE)((m->param & 0xF0) | (m_cmdReplace.param & 0x0F));
						else
							m->param = m_cmdReplace.param;
					}
					pModDoc->SetModified();
					if (bUpdPos) InvalidateRow();
				}
			}
		}
	}
EndSearch:
	if (m_dwReplaceFlags & PATSEARCH_REPLACEALL) InvalidatePattern();
	m_bContinueSearch = TRUE;
	EndWaitCursor();
	// Display search results
	m_dwReplaceFlags &= ~PATSEARCH_REPLACEALL;
	if (!nFound)
	{
		if (m_dwFindFlags & PATSEARCH_NOTE)
		{
			UINT note = m_cmdFind.note;
			if (note == 0) strcpy(szFind, "..."); else
			if (note == 0xFE) strcpy(szFind, "^^ "); else
			if (note == 0xFF) strcpy(szFind, "== "); else
			wsprintf(szFind, "%s%d", szNoteNames[(note-1) % 12], (note-1)/12);
		} else strcpy(szFind, "???");
		strcat(szFind, " ");
		if (m_dwFindFlags & PATSEARCH_INSTR)
		{
			if (m_cmdFind.instr)
				wsprintf(&szFind[strlen(szFind)], "%03d", m_cmdFind.instr);
			else
				strcat(szFind, " ..");
		} else strcat(szFind, " ??");
		strcat(szFind, " ");
		if (m_dwFindFlags & PATSEARCH_VOLCMD)
		{
			if (m_cmdFind.volcmd)
				wsprintf(&szFind[strlen(szFind)], "%c", gszVolCommands[m_cmdFind.volcmd]);
			else
				strcat(szFind, ".");
		} else strcat(szFind, "?");
		if (m_dwFindFlags & PATSEARCH_VOLUME)
		{
			wsprintf(&szFind[strlen(szFind)], "%02d", m_cmdFind.vol);
		} else strcat(szFind, "??");
		strcat(szFind, " ");
		if (m_dwFindFlags & PATSEARCH_COMMAND)
		{
			if (m_cmdFind.command)
			{
				if (pSndFile->m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT))
					wsprintf(&szFind[strlen(szFind)], "%c", gszS3mCommands[m_cmdFind.command]);
				else
					wsprintf(&szFind[strlen(szFind)], "%c", gszModCommands[m_cmdFind.command]);
			} else
				strcat(szFind, ".");
		} else strcat(szFind, "?");
		if (m_dwFindFlags & PATSEARCH_PARAM)
		{
			wsprintf(&szFind[strlen(szFind)], "%02X", m_cmdFind.param);
		} else strcat(szFind, "??");
		wsprintf(s, "Cannot find \"%s\"", szFind);
		MessageBox(s, "Find/Replace", MB_OK | MB_ICONINFORMATION);
	}
}


void CViewPattern::OnPatternStep()
//--------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();

	if ((pMainFrm) && (pModDoc))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if ((!pSndFile->PatternSize[m_nPattern]) || (!pSndFile->Patterns[m_nPattern])) return;
		// Cut instrument/sample
		BEGIN_CRITICAL();
		for (UINT i=pSndFile->m_nChannels; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
			pSndFile->m_dwSongFlags &= ~SONG_PAUSED;
			pSndFile->LoopPattern(m_nPattern);
			pSndFile->m_nNextRow = GetCurrentRow();
			pSndFile->m_dwSongFlags |= SONG_STEP;
		}
		END_CRITICAL();
		if (pMainFrm->GetModPlaying() != pModDoc)
		{
			pMainFrm->PlayMod(pModDoc, m_hWnd, MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);
		}
		CMainFrame::EnableLowLatencyMode();
		if (CMainFrame::m_dwPatternSetup & PATTERN_CONTSCROLL)
			SetCurrentRow(GetCurrentRow()+1, TRUE);
		else
			SetCurrentRow((GetCurrentRow()+1) % pSndFile->PatternSize[m_nPattern], FALSE);
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
		if ((!pSndFile->PatternSize[m_nPattern]) || (!pSndFile->Patterns[m_nPattern])) return;
		MODCOMMAND *m = pSndFile->Patterns[m_nPattern] + m_nRow * pSndFile->m_nChannels + ((m_dwCursor & 0xFFFF) >> 3);
		switch(m_dwCursor & 7)
		{
		case 0:
		case 1:
			m_cmdOld.note = m->note;
			m_cmdOld.instr = m->instr;
			SendCtrlMessage(CTRLMSG_SETCURRENTINSTRUMENT, m_cmdOld.instr);
			break;
		case 2:
			m_cmdOld.volcmd = m->volcmd;
			m_cmdOld.vol = m->vol;
			break;
		case 3:
		case 4:
			m_cmdOld.command = m->command;
			m_cmdOld.param = m->param;
			break;
		}
	}
}


void CViewPattern::OnCursorPaste()
//--------------------------------
{
	ProcessChar(' ', 0);
}


void CViewPattern::OnInterpolateVolume()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		UINT row0 = m_dwBeginSel >> 16, row1 = m_dwEndSel >> 16, nchn = (m_dwBeginSel & 0xFFFF) >> 3;
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODCOMMAND *pcmd = pSndFile->Patterns[m_nPattern];
		int vsrc, vdest;
		BYTE vcmd;

		if ((!pcmd) || (nchn >= pSndFile->m_nChannels)
		 || (row0 >= row1) || (row1 >= pSndFile->PatternSize[m_nPattern])) return;
		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		vsrc = pcmd[row0 * pSndFile->m_nChannels + nchn].vol;
		vdest = pcmd[row1 * pSndFile->m_nChannels + nchn].vol;
		vcmd = pcmd[row0 * pSndFile->m_nChannels + nchn].volcmd;
		if (!vcmd)
		{
			vcmd = pcmd[row1 * pSndFile->m_nChannels + nchn].volcmd;
			vsrc = vdest;
		} else
		if (!pcmd[row1 * pSndFile->m_nChannels + nchn].volcmd)
		{
			vdest = vsrc;
		}
		if (vcmd)
		{
			int verr;
			pcmd += row0 * pSndFile->m_nChannels + nchn;
			row1 -= row0;
			verr = (row1 * 63) / 128;
			if (vdest < vsrc) verr = -verr;
			for (UINT i=0; i<=row1; i++, pcmd += pSndFile->m_nChannels)
			{
				if ((!pcmd->volcmd) || (pcmd->volcmd == vcmd))
				{
					int vol = vsrc + ((vdest - vsrc) * (int)i + verr) / (int)row1;
					pcmd->vol = (BYTE)vol;
					pcmd->volcmd = vcmd;
				}
			}
			pModDoc->SetModified();
			InvalidatePattern(FALSE);
		}
	}
}


void CViewPattern::OnInterpolateEffect()
//--------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		UINT row0 = m_dwBeginSel >> 16, row1 = m_dwEndSel >> 16, nchn = (m_dwBeginSel & 0xFFFF) >> 3;
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODCOMMAND *pcmd = pSndFile->Patterns[m_nPattern];
		int vsrc, vdest;
		BYTE vcmd;

		if ((!pcmd) || (nchn >= pSndFile->m_nChannels)
		 || (row0 >= row1) || (row1 >= pSndFile->PatternSize[m_nPattern])) return;
		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		vsrc = pcmd[row0 * pSndFile->m_nChannels + nchn].param;
		vdest = pcmd[row1 * pSndFile->m_nChannels + nchn].param;
		vcmd = pcmd[row0 * pSndFile->m_nChannels + nchn].command;
		if (!vcmd)
		{
			vcmd = pcmd[row1 * pSndFile->m_nChannels + nchn].command;
			vsrc = vdest;
		} else
		if (!pcmd[row1 * pSndFile->m_nChannels + nchn].command)
		{
			vdest = vsrc;
		}
		if (vcmd)
		{
			int verr;
			pcmd += row0 * pSndFile->m_nChannels + nchn;
			row1 -= row0;
			verr = (row1 * 63) / 128;
			if (vdest < vsrc) verr = -verr;
			for (UINT i=0; i<=row1; i++, pcmd += pSndFile->m_nChannels)
			{
				if ((!pcmd->command) || (pcmd->command == vcmd))
				{
					int val = vsrc + ((vdest - vsrc) * (int)i + verr) / (int)row1;
					pcmd->param = (BYTE)val;
					pcmd->command = vcmd;
				}
			}
			pModDoc->SetModified();
			InvalidatePattern(FALSE);
		}
	}
}


BOOL CViewPattern::TransposeSelection(int transp)
//-----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		UINT row0 = m_dwBeginSel >> 16, row1 = m_dwEndSel >> 16;
		UINT col0 = ((m_dwBeginSel & 0xFFFF)+7) >> 3, col1 = (m_dwEndSel & 0xFFFF) >> 3;
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		MODCOMMAND *pcmd = pSndFile->Patterns[m_nPattern];

		if ((!pcmd) || (col0 > col1) || (col1 >= pSndFile->m_nChannels)
		 || (row0 > row1) || (row1 >= pSndFile->PatternSize[m_nPattern])) return FALSE;
		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		for (UINT row=row0; row <= row1; row++)
		{
			MODCOMMAND *m = &pcmd[row * pSndFile->m_nChannels];
			for (UINT col=col0; col<=col1; col++)
			{
				int note = m[col].note;
				if ((note) && (note <= 120))
				{
					note += transp;
					if (note < 1) note = 1;
					if (note > 120) note = 120;
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

	if ((pModDoc = GetDocument()) == NULL) return;
	pSndFile = pModDoc->GetSoundFile();
	nChannels = pSndFile->m_nChannels;
	nRows = pSndFile->PatternSize[m_nPattern];
	pOldPattern = pSndFile->Patterns[m_nPattern];
	if ((nChannels < 1) || (nRows < 1) || (!pOldPattern)) return;
	dx = (int)((m_dwDragPos & 0xFFF8) >> 3) - (int)((m_dwStartSel & 0xFFF8) >> 3);
	dy = (int)(m_dwDragPos >> 16) - (int)(m_dwStartSel >> 16);
	if ((!dx) && (!dy)) return;
	pModDoc->PrepareUndo(m_nPattern, 0,0, nChannels, nRows);
	pNewPattern = CSoundFile::AllocatePattern(nRows, nChannels);
	if (!pNewPattern) return;
	x1 = (m_dwBeginSel & 0xFFF8) >> 3;
	y1 = (m_dwBeginSel) >> 16;
	x2 = (m_dwEndSel & 0xFFF8) >> 3;
	y2 = (m_dwEndSel) >> 16;
	c1 = (m_dwBeginSel&7);
	c2 = (m_dwEndSel&7);
	if (c1 > 3) c1 = 3;
	if (c2 > 3) c2 = 3;
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
	SetCurrentRow(y1);
	SetCurrentColumn((x1<<3)|c1);
	SetCurSel((y1<<16)|(x1<<3)|c1, (y2<<16)|(x2<<3)|c2);
	InvalidatePattern();
	CSoundFile::FreePattern(pOldPattern);
	pModDoc->SetModified();
	EndWaitCursor();
}


void CViewPattern::OnSetSelInstrument()
//-------------------------------------
{
	CModDoc *pModDoc;
	CSoundFile *pSndFile;
	UINT nIns = GetCurrentInstrument();
	MODCOMMAND *p;
	BOOL bModified;
	
	if (!nIns) return;
	if ((pModDoc = GetDocument()) == NULL) return;
	pSndFile = pModDoc->GetSoundFile();
	p = pSndFile->Patterns[m_nPattern];
	if (!p) return;
	BeginWaitCursor();
	PrepareUndo(m_dwBeginSel, m_dwEndSel);
	bModified = FALSE;
	for (UINT y=0; y<pSndFile->PatternSize[m_nPattern]; y++)
	{
		if ((y >= (m_dwBeginSel >> 16)) && (y <= (m_dwEndSel >> 16)))
		{
			for (UINT x=0; x<pSndFile->m_nChannels; x++)
			{
				UINT col = (x<<3) | 1;
				if ((col >= (m_dwBeginSel & 0xFFFF)) && (col <= (m_dwEndSel & 0xFFFF)) && (p[x].instr))
				{
					if (p[x].instr != (BYTE)nIns)
					{
						p[x].instr = (BYTE)nIns;
						bModified = TRUE;
					}
				}
			}
		}
		p += pSndFile->m_nChannels;
	}
	if (bModified)
	{
		pModDoc->SetModified();
		pModDoc->UpdateAllViews(NULL, HINT_PATTERNDATA | (m_nPattern << 24), NULL);
	}
	EndWaitCursor();
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
		pCmdUI->Enable(pModDoc->CanUndo());
	}
}


void CViewPattern::OnEditUndo()
//-----------------------------
{
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		UINT nPat = pModDoc->DoUndo();
		if (nPat < MAX_PATTERNS)
		{
			pModDoc->SetModified();
			SetCurrentPattern(nPat);
		}
	}
}


void CViewPattern::OnPatternAmplify()
//-----------------------------------
{
	static UINT snOldAmp = 100;
	CAmpDlg dlg(this, snOldAmp);
	CModDoc *pModDoc = GetDocument();
	BYTE chvol[MAX_BASECHANNELS];

	if ((pModDoc) && (dlg.DoModal() == IDOK))
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		BeginWaitCursor();
		PrepareUndo(m_dwBeginSel, m_dwEndSel);
		snOldAmp = dlg.m_nFactor;
		memset(chvol, 64, sizeof(chvol));
		if (pSndFile->Patterns[m_nPattern])
		{
			MODCOMMAND *p = pSndFile->Patterns[m_nPattern];

			for (UINT j=0; j<pSndFile->m_nChannels; j++)
			{
				for (UINT i=0; i<pSndFile->PatternSize[m_nPattern]; i++)
				{
					if ((i >= (m_dwBeginSel >> 16)) && (i <= (m_dwEndSel >> 16)))
					{
						MODCOMMAND *m = p + i*pSndFile->m_nChannels+j;
						if ((m->command == CMD_VOLUME) && (m->param <= 64))
						{
							chvol[j] = m->param;
							break;
						}
						if (m->volcmd == VOLCMD_VOLUME)
						{
							chvol[j] = m->vol;
							break;
						}
						if ((m->note) && (m->note < 0x80) && (m->instr))
						{
							UINT nSmp = m->instr;
							if (pSndFile->m_nInstruments)
							{
								if ((nSmp <= pSndFile->m_nInstruments) && (pSndFile->Headers[nSmp]))
								{
									nSmp = pSndFile->Headers[nSmp]->Keyboard[m->note];
								} else
								{
									nSmp = 0;
								}
							}
							if ((nSmp) && (nSmp <= pSndFile->m_nSamples))
							{
								chvol[j] = (BYTE)(pSndFile->Ins[nSmp].nVolume >> 2);
								break;
							}
						}
					}
				}
			}
			for (UINT y=0; y<pSndFile->PatternSize[m_nPattern]; y++)
			{
				if ((y >= (m_dwBeginSel >> 16)) && (y <= (m_dwEndSel >> 16)))
				{
					int cy = (m_dwEndSel>>16) - (m_dwBeginSel>>16) + 1;
					int y0 = (m_dwBeginSel>>16);
					for (UINT x=0; x<pSndFile->m_nChannels; x++)
					{
						UINT col = (x<<3) | 2;
						if ((col >= (m_dwBeginSel & 0xFFFF)) && (col <= (m_dwEndSel & 0xFFFF)))
						{
							if ((!p[x].volcmd) && (p[x].command != CMD_VOLUME)
							 && (p[x].note) && (p[x].note < 0x80) && (p[x].instr))
							{
								UINT nSmp = p[x].instr;
								if (pSndFile->m_nInstruments)
								{
									if ((nSmp <= pSndFile->m_nInstruments) && (pSndFile->Headers[nSmp]))
									{
										nSmp = pSndFile->Headers[nSmp]->Keyboard[p[x].note];
									} else
									{
										nSmp = 0;
									}
								}
								if ((nSmp) && (nSmp <= pSndFile->m_nSamples))
								{
									p[x].volcmd = VOLCMD_VOLUME;
									p[x].vol = pSndFile->Ins[nSmp].nVolume >> 2;
								}
							}
							if (p[x].volcmd == VOLCMD_VOLUME) chvol[x] = (BYTE)p[x].vol;
							if (((dlg.m_bFadeIn) || (dlg.m_bFadeOut)) && (p[x].command != CMD_VOLUME) && (!p[x].volcmd))
							{
								p[x].volcmd = VOLCMD_VOLUME;
								p[x].vol = chvol[x];
							}
							if (p[x].volcmd == VOLCMD_VOLUME)
							{
								int vol = p[x].vol * dlg.m_nFactor;
								if (dlg.m_bFadeIn) vol = (vol * (y+1-y0)) / cy;
								if (dlg.m_bFadeOut) vol = (vol * (cy+y0-y)) / cy;
								vol = (vol+50) / 100;
								if (vol > 64) vol = 64;
								p[x].vol = (BYTE)vol;
							}
						}
						col = (x<<3) | 3;
						if ((col >= (m_dwBeginSel & 0xFFFF)) && (col <= (m_dwEndSel & 0xFFFF)))
						{
							if ((p[x].command == CMD_VOLUME) && (p[x].param <= 64))
							{
								int vol = p[x].param * dlg.m_nFactor;
								if (dlg.m_bFadeIn) vol = (vol * (y+1-y0)) / cy;
								if (dlg.m_bFadeOut) vol = (vol * (cy+y0-y)) / cy;
								vol = (vol+50) / 100;
								if (vol > 64) vol = 64;
								p[x].param = (BYTE)vol;
							}
						}
					}
				}
				p += pSndFile->m_nChannels;
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
		UINT nPat = 0xFFFF;
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if (pSndFile->m_dwSongFlags & (SONG_PAUSED|SONG_STEP)) return 0;
		if (pSndFile->m_dwSongFlags & SONG_PATTERNLOOP)
		{
			nPat = pSndFile->m_nPattern;
			nOrd = 0xFFFF;
		} else
		if (nOrd < MAX_ORDERS)
		{
			nPat = pSndFile->Order[nOrd];
		}
		if ((m_dwStatus & (PATSTATUS_FOLLOWSONG|PATSTATUS_DRAGVSCROLL|PATSTATUS_DRAGHSCROLL|PATSTATUS_MOUSEDRAGSEL)) == PATSTATUS_FOLLOWSONG)
		{
			if (nPat < MAX_PATTERNS)
			{
				if (nPat != m_nPattern) SetCurrentPattern(nPat, nRow);
				if (nRow != m_nRow)	SetCurrentRow((nRow < pSndFile->PatternSize[nPat]) ? nRow : 0);
			}
			SetPlayCursor(0xFFFF, 0);
			if (nOrd < MAX_ORDERS) SendCtrlMessage(CTRLMSG_SETCURRENTORDER, nOrd);
		} else
		{
			SetPlayCursor(nPat, nRow);
		}
	}
	if ((pnotify->dwType & (MPTNOTIFY_VUMETERS|MPTNOTIFY_STOP)) && (m_dwStatus & PATSTATUS_VUMETERS))
	{
		UpdateAllVUMeters(pnotify);
	}
	return 0;
}


LRESULT CViewPattern::OnMidiMsg(WPARAM dwMidiData, LPARAM)
//--------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = GetDocument();
	DWORD dwMidiByte1 = (dwMidiData >> 8) & 0xFF;
	DWORD dwMidiByte2 = (dwMidiData >> 16) & 0xFF;
	UINT nIns = 0;

	if ((!pModDoc) || (!pMainFrm)) return 0;
	switch(dwMidiData & 0xF0)
	{
	// Note Off
	case 0x80:
		dwMidiByte2 = 0;
	// Note On
	case 0x90:
		dwMidiByte1 &= 0x7F;
		dwMidiByte2 &= 0x7F;
		if ((!dwMidiByte2) && (!(CMainFrame::m_dwMidiSetup & MIDISETUP_RECORDNOTEOFF))) break;
		nIns = GetCurrentInstrument();
		if (m_dwStatus & PATSTATUS_RECORD)
		{
			if (dwMidiByte2)
			{
				int vol = -1;
				if (CMainFrame::m_dwMidiSetup & MIDISETUP_RECORDVELOCITY)
				{
					vol = (CDLSBank::DLSMidiVolumeToLinear(dwMidiByte2)+1023) >> 10;
					if (CMainFrame::m_dwMidiSetup & MIDISETUP_AMPLIFYVELOCITY) vol *= 2;
					if (vol < 1) vol = 1;
					if (vol > 64) vol = 64;
				}
				EnterNote(dwMidiByte1+1, nIns, FALSE, vol, TRUE);
				if (((m_nSpacing) && (!(m_dwStatus & PATSTATUS_MIDISPACINGPENDING)))
				 && ((!(m_dwStatus & PATSTATUS_FOLLOWSONG))
				  || (pMainFrm->GetFollowSong(pModDoc) != m_hWnd)
				  || (pModDoc->GetSoundFile()->IsPaused())))
				{
					m_dwStatus |= PATSTATUS_MIDISPACINGPENDING;
					PostMessage(WM_MOD_VIEWMSG, VIEWMSG_DOMIDISPACING, timeGetTime());
				}
			} else
			{
				EnterNote((dwMidiByte1+1)|0x80, 0, TRUE, -1, TRUE);
			}
		}
		if ((!(m_dwStatus & PATSTATUS_RECORD)) || (CMainFrame::m_dwPatternSetup & PATTERN_PLAYNEWNOTE))
		{
			if (dwMidiByte2)
			{
				UINT nVol = 0;
				
				if (CMainFrame::m_dwMidiSetup & MIDISETUP_RECORDVELOCITY)
				{
					nVol = (CDLSBank::DLSMidiVolumeToLinear(dwMidiByte2)+255) >> 8;
					if (CMainFrame::m_dwMidiSetup & MIDISETUP_AMPLIFYVELOCITY) nVol *= 2;
					if (nVol < 1) nVol = 1;
					if (nVol > 256) nVol = 256;
				}
				//Log("note=%d vol=%d\n", dwMidiByte1, dwMidiByte2);
				pModDoc->PlayNote(dwMidiByte1+1, nIns, 0, FALSE, nVol);
			} else
			{
				pModDoc->NoteOff(dwMidiByte1+1);
			}
		}
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

	case VIEWMSG_DOMIDISPACING:
		if (m_nSpacing)
		{
			if (timeGetTime() - lParam >= 10)
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
				Sleep(1);
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
			if ((pState->nPattern == m_nPattern) && (pState->cbStruct == sizeof(PATTERNVIEWSTATE)))
			{
				SetCurrentRow(pState->nRow);
				SetCurrentColumn(pState->nCursor);
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
				pModDoc->CopyPattern(m_nPattern, 0, ((pSndFile->PatternSize[m_nPattern]-1)<<16)|(pSndFile->m_nChannels<<3));
			}
		}
		break;

	case VIEWMSG_PASTEPATTERN:
		{
			CModDoc *pModDoc = GetDocument();
			if (pModDoc) pModDoc->PastePattern(m_nPattern, 0);
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

	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}


