/*
 * ctrl_seq.cpp
 * ------------
 * Purpose: Order list for the pattern editor upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "View_tre.h"
#include "InputHandler.h"
#include "Moddoc.h"
#include "../soundlib/mod_specifications.h"
#include "Globals.h"
#include "Ctrl_pat.h"
#include "PatternClipboard.h"


OPENMPT_NAMESPACE_BEGIN


// Little helper function to avoid copypasta
static bool IsSelectionKeyPressed() { return CMainFrame::GetInputHandler()->SelectionPressed(); }
static bool IsCtrlKeyPressed() { return CMainFrame::GetInputHandler()->CtrlPressed(); }


//////////////////////////////////////////////////////////////
// CPatEdit

BOOL CPatEdit::PreTranslateMessage(MSG *pMsg)
//-------------------------------------------
{
	if (((pMsg->message == WM_KEYDOWN) || (pMsg->message == WM_KEYUP)) && (pMsg->wParam == VK_TAB))
	{
		if ((pMsg->message == WM_KEYUP) && (m_pParent))
		{
			m_pParent->SwitchToView();
		}
		return TRUE;
	}
	return CEdit::PreTranslateMessage(pMsg);
}


//////////////////////////////////////////////////////////////
// COrderList

BEGIN_MESSAGE_MAP(COrderList, CWnd)
	//{{AFX_MSG_MAP(COrderList)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_HSCROLL()
	ON_WM_SIZE()

	ON_COMMAND(ID_ORDERLIST_INSERT,				OnInsertOrder)
	ON_COMMAND(ID_ORDERLIST_INSERT_SEPARATOR,	OnInsertSeparatorPattern)
	ON_COMMAND(ID_ORDERLIST_DELETE,				OnDeleteOrder)
	ON_COMMAND(ID_ORDERLIST_RENDER,				OnRenderOrder)
	ON_COMMAND(ID_ORDERLIST_EDIT_COPY,			OnEditCopy)
	ON_COMMAND(ID_ORDERLIST_EDIT_CUT,			OnEditCut)
	ON_COMMAND(ID_ORDERLIST_EDIT_COPY_ORDERS,	OnEditCopyOrders)
	
	ON_COMMAND(ID_PATTERN_PROPERTIES,			OnPatternProperties)
	ON_COMMAND(ID_PLAYER_PLAY,					OnPlayerPlay)
	ON_COMMAND(ID_PLAYER_PAUSE,					OnPlayerPause)
	ON_COMMAND(ID_PLAYER_PLAYFROMSTART,			OnPlayerPlayFromStart)
	ON_COMMAND(IDC_PATTERN_PLAYFROMSTART,		OnPatternPlayFromStart)
	//ON_COMMAND(ID_PATTERN_RESTART,			OnPatternPlayFromStart)
	ON_COMMAND(ID_ORDERLIST_NEW,				OnCreateNewPattern)
	ON_COMMAND(ID_ORDERLIST_COPY,				OnDuplicatePattern)
	ON_COMMAND(ID_PATTERNCOPY,					OnPatternCopy)
	ON_COMMAND(ID_PATTERNPASTE,					OnPatternPaste)
	ON_COMMAND(ID_ORDERLIST_LOCKPLAYBACK,		OnLockPlayback)
	ON_COMMAND(ID_ORDERLIST_UNLOCKPLAYBACK,		OnUnlockPlayback)
	ON_COMMAND_RANGE(ID_SEQUENCE_ITEM, ID_SEQUENCE_ITEM + MAX_SEQUENCES + 2, OnSelectSequence)
	ON_MESSAGE(WM_MOD_DRAGONDROPPING,			OnDragonDropping)
	ON_MESSAGE(WM_HELPHITTEST,					OnHelpHitTest)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,				OnCustomKeyMsg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


bool COrderList::IsOrderInMargins(int order, int startOrder)
//----------------------------------------------------------
{
	const ORDERINDEX nMargins = GetMargins();
	return ((startOrder != 0 && order - startOrder < nMargins) || 
		order - startOrder >= GetLength() - nMargins);
}


void COrderList::EnsureVisible(ORDERINDEX order)
//----------------------------------------------
{
	// nothing needs to be done
	if(!IsOrderInMargins(order, m_nXScroll) || order == ORDERINDEX_INVALID) return;

	if(order < m_nXScroll)
	{
		if(order < GetMargins())
			m_nXScroll = 0;
		else
			m_nXScroll = order - GetMargins();
	} else
	{
		m_nXScroll = order + 2 * GetMargins() - 1;
		if(m_nXScroll < GetLength())
			m_nXScroll = 0;
		else
			m_nXScroll -= GetLength();
	}
}


COrderList::COrderList(CCtrlPatterns &parent, CModDoc &document) : m_pParent(parent), m_pModDoc(document)
//-------------------------------------------------------------------------------------------------------
{
	m_hFont = nullptr;
	m_cxFont = m_cyFont = 0;
	m_nScrollPos = m_nXScroll = 0;
	m_nScrollPos2nd = ORDERINDEX_INVALID;
	m_nOrderlistMargins = TrackerSettings::Instance().orderlistMargins;
	m_bScrolling = false;
	m_bDragging = false;
}


ORDERINDEX COrderList::GetOrderFromPoint(const CRect& rect, const CPoint& pt) const
//---------------------------------------------------------------------------------
{
	return static_cast<ORDERINDEX>(m_nXScroll + (pt.x - rect.left) / m_cxFont);
}


BOOL COrderList::Init(const CRect &rect, HFONT hFont)
//---------------------------------------------------
{
	CreateEx(WS_EX_STATICEDGE, NULL, "", WS_CHILD|WS_VISIBLE, rect, &m_pParent, IDC_ORDERLIST);
	m_hFont = hFont;
	colorText = GetSysColor(COLOR_WINDOWTEXT);
	colorInvalid = GetSysColor(COLOR_GRAYTEXT);
	colorTextSel = GetSysColor(COLOR_HIGHLIGHTTEXT);
	SendMessage(WM_SETFONT, (WPARAM)m_hFont);
	SetScrollPos(SB_HORZ, 0);
	EnableScrollBarCtrl(SB_HORZ, TRUE);
	SetCurSel(0);
	return TRUE;
}


BOOL COrderList::UpdateScrollInfo()
//---------------------------------
{
	CRect rcClient;

	GetClientRect(&rcClient);
	if ((m_cxFont > 0) && (rcClient.right > 0))
	{
		CRect rect;
		CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
		SCROLLINFO info;
		UINT nPage;

		int nMax = sndFile.Order.GetLengthTailTrimmed();

		GetScrollInfo(SB_HORZ, &info, SIF_PAGE|SIF_RANGE);
		info.fMask = SIF_PAGE|SIF_RANGE;
		info.nMin = 0;
		nPage = rcClient.right / m_cxFont;
		if (nMax <= (int)nPage) nMax = nPage + 1;
		if ((nMax != info.nMax) || (nPage != info.nPage))
		{
			info.nPage = nPage;
			info.nMax = nMax;
			SetScrollInfo(SB_HORZ, &info, TRUE);
		}
	}
	return FALSE;
}


int COrderList::GetFontWidth()
//----------------------------
{
	if ((m_cxFont <= 0) && (m_hWnd) && (m_hFont))
	{
		CClientDC dc(this);
		HGDIOBJ oldfont = ::SelectObject(dc.m_hDC, m_hFont);
		CSize sz = dc.GetTextExtent("000+", 4);
		if (oldfont) ::SelectObject(dc.m_hDC, oldfont);
		return sz.cx;
	}
	return m_cxFont;
}


void COrderList::InvalidateSelection() const
//------------------------------------------
{
	ORDERINDEX nOrdLo = m_nScrollPos, nCount = 1;
	static ORDERINDEX m_nScrollPos2Old = m_nScrollPos2nd;
	if(m_nScrollPos2Old != ORDERINDEX_INVALID)
	{
		// there were multiple orders selected - remove them all
		ORDERINDEX nOrdHi = m_nScrollPos;
		if(m_nScrollPos2Old < m_nScrollPos)
		{
			nOrdLo = m_nScrollPos2Old;
		} else
		{
			nOrdHi = m_nScrollPos2Old;
		}
		nCount = nOrdHi - nOrdLo + 1;
	}
	m_nScrollPos2Old = m_nScrollPos2nd;
	CRect rcClient, rect;
	GetClientRect(&rcClient);
	rect.left = rcClient.left + (nOrdLo - m_nXScroll) * m_cxFont;
	rect.top = rcClient.top;
	rect.right = rect.left + m_cxFont * nCount;
	rect.bottom = rcClient.bottom;
	if (rect.right > rcClient.right) rect.right = rcClient.right;
	if (rect.left < rcClient.left) rect.left = rcClient.left;
	if (rect.right > rect.left) ::InvalidateRect(m_hWnd, &rect, FALSE);
}


ORDERINDEX COrderList::GetLength()
//--------------------------------
{
	CRect rcClient;
	GetClientRect(&rcClient);
	if(m_cxFont > 0)
		return static_cast<ORDERINDEX>(rcClient.right / m_cxFont);
	else
	{
		const int nFontWidth = GetFontWidth();
		return (nFontWidth > 0) ? static_cast<ORDERINDEX>(rcClient.right / nFontWidth) : 0;
	}
}


OrdSelection COrderList::GetCurSel(bool bIgnoreSelection) const
//-------------------------------------------------------------
{
	// returns the currently selected order(s)
	OrdSelection result;
	result.firstOrd = result.lastOrd = m_nScrollPos;
	// bIgnoreSelection: true if only first selection marker is important.
	if(!bIgnoreSelection && m_nScrollPos2nd != ORDERINDEX_INVALID)
	{
		if(m_nScrollPos2nd < m_nScrollPos) // ord2 < ord1
			result.firstOrd = m_nScrollPos2nd;
		else
			result.lastOrd = m_nScrollPos2nd;
	}
	LimitMax(result.firstOrd, m_pModDoc.GetrSoundFile().Order.GetLastIndex());
	LimitMax(result.lastOrd, m_pModDoc.GetrSoundFile().Order.GetLastIndex());
	return result;
}


bool COrderList::SetCurSel(ORDERINDEX sel, bool bEdit, bool bShiftClick, bool bIgnoreCurSel, bool setPlayPos)
//-----------------------------------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
	ORDERINDEX &nOrder = bShiftClick ? m_nScrollPos2nd : m_nScrollPos;

	if ((sel < 0) || (sel >= sndFile.Order.GetLength()) || (!m_pParent) || (!pMainFrm)) return false;
	if (!bIgnoreCurSel && sel == nOrder && (sel == sndFile.m_PlayState.m_nCurrentOrder || !setPlayPos)) return true;
	const ORDERINDEX nShownLength = GetLength();
	InvalidateSelection();
	nOrder = sel;

	if (!m_bScrolling)
	{
		const ORDERINDEX nMargins = GetMargins(GetMarginsMax(nShownLength));
		if(nOrder < m_nXScroll + nMargins)
		{
			// Must move first shown sequence item to left in order to show
			// the new active order.
			m_nXScroll = static_cast<ORDERINDEX>(std::max(0, static_cast<int>(nOrder - nMargins)));
			SetScrollPos(SB_HORZ, m_nXScroll);
			InvalidateRect(NULL, FALSE);
		} else
		{
			ORDERINDEX maxsel = nShownLength;
			if (maxsel) maxsel--;
			if (nOrder - m_nXScroll >= maxsel - nMargins)
			{
				// Must move first shown sequence item to right in order to show
				// the new active order.
				m_nXScroll = nOrder - (maxsel - nMargins);
				SetScrollPos(SB_HORZ, m_nXScroll);
				InvalidateRect(NULL, FALSE);
			}
		}
	}
	InvalidateSelection();
	if (bEdit)
	{
		PATTERNINDEX n = sndFile.Order[m_nScrollPos];
		if (sndFile.Patterns.IsValidPat(n) && setPlayPos && !bShiftClick)
		{
			CriticalSection cs;

			bool isPlaying = (pMainFrm->GetModPlaying() == &m_pModDoc);
			bool changedPos = false;

			if(isPlaying && sndFile.m_SongFlags[SONG_PATTERNLOOP])
			{
				pMainFrm->ResetNotificationBuffer();

				// update channel parameters and play time
				m_pModDoc.SetElapsedTime(m_nScrollPos, 0, !sndFile.m_SongFlags[SONG_PAUSED | SONG_STEP]);

				changedPos = true;
			} else if(m_pParent.GetFollowSong())
			{
				FlagSet<SongFlags> pausedFlags = sndFile.m_SongFlags & (SONG_PAUSED | SONG_STEP | SONG_PATTERNLOOP);
				// update channel parameters and play time
				sndFile.SetCurrentOrder(m_nScrollPos);
				m_pModDoc.SetElapsedTime(m_nScrollPos, 0, !sndFile.m_SongFlags[SONG_PAUSED | SONG_STEP]);
				sndFile.m_SongFlags.set(pausedFlags);

				if(isPlaying) pMainFrm->ResetNotificationBuffer();
				changedPos = true;
			}

			if(changedPos && sndFile.Order.IsPositionLocked(m_nScrollPos))
			{
				// Users wants to go somewhere else, so let him do that.
				OnUnlockPlayback();
			}

			m_pParent.SetCurrentPattern(n);
		}
	}
	UpdateInfoText();
	if(m_nScrollPos == m_nScrollPos2nd) m_nScrollPos2nd = ORDERINDEX_INVALID;
	return true;
}


UINT COrderList::GetCurrentPattern() const
//----------------------------------------
{
	if(m_nScrollPos < m_pModDoc.GetrSoundFile().Order.GetLength())
	{
		return m_pModDoc.GetrSoundFile().Order[m_nScrollPos];
	}
	return 0;
}


BOOL COrderList::PreTranslateMessage(MSG *pMsg)
//---------------------------------------------
{
	//handle Patterns View context keys that we want to take effect in the orderlist.
	if ((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) || 
		(pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
	{
		CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();

		//Translate message manually
		UINT nChar = (UINT)pMsg->wParam;
		UINT nRepCnt = LOWORD(pMsg->lParam);
		UINT nFlags = HIWORD(pMsg->lParam);
		KeyEventType kT = ih->GetKeyEventType(nFlags);

		if (ih->KeyEvent(kCtxCtrlOrderlist, nChar, nRepCnt, nFlags, kT) != kcNull)
			return true; // Mapped to a command, no need to pass message on.

		//HACK: masquerade as kCtxViewPatternsNote context until we implement appropriate
		//      command propagation to kCtxCtrlOrderlist context.

		if (ih->KeyEvent(kCtxViewPatternsNote, nChar, nRepCnt, nFlags, kT) != kcNull)
			return true; // Mapped to a command, no need to pass message on.

	}

	return CWnd::PreTranslateMessage(pMsg);
}


LRESULT COrderList::OnCustomKeyMsg(WPARAM wParam, LPARAM)
//-------------------------------------------------------
{
	if (wParam == kcNull)
		return 0;

	switch(wParam)
	{
	case kcEditCopy:
		OnEditCopy(); return wParam;
	case kcEditCut:
		OnEditCut(); return wParam;
	case kcEditPaste:
		OnPatternPaste(); return wParam;

	// Orderlist navigation
	case kcOrderlistNavigateLeftSelect:
	case kcOrderlistNavigateLeft:
		SetCurSelTo2ndSel(wParam == kcOrderlistNavigateLeftSelect); SetCurSel(m_nScrollPos - 1); return wParam;
	case kcOrderlistNavigateRightSelect:
	case kcOrderlistNavigateRight:
		SetCurSelTo2ndSel(wParam == kcOrderlistNavigateRightSelect); SetCurSel(m_nScrollPos + 1); return wParam;
	case kcOrderlistNavigateFirstSelect:
	case kcOrderlistNavigateFirst:
		SetCurSelTo2ndSel(wParam == kcOrderlistNavigateFirstSelect); SetCurSel(0); return wParam;
	case kcEditSelectAll:
		SetCurSel(0);
		MPT_FALLTHROUGH;
	case kcOrderlistNavigateLastSelect:
	case kcOrderlistNavigateLast:
		{
			SetCurSelTo2ndSel(wParam == kcOrderlistNavigateLastSelect || wParam == kcEditSelectAll);
			ORDERINDEX nLast = m_pModDoc.GetrSoundFile().Order.GetLengthTailTrimmed();
			if(nLast > 0) nLast--;
			SetCurSel(nLast);
		}
		return wParam;

	// Orderlist edit
	case kcOrderlistEditDelete:
		OnDeleteOrder(); return wParam;
	case kcOrderlistEditInsert:
		OnInsertOrder(); return wParam;
	case kcOrderlistSwitchToPatternView:
		OnSwitchToView(); return wParam;
	case kcOrderlistEditPattern:
		OnLButtonDblClk(0, CPoint(0,0)); OnSwitchToView(); return wParam;

	// Enter pattern number
	case kcOrderlistPat0:
	case kcOrderlistPat1:
	case kcOrderlistPat2:
	case kcOrderlistPat3:
	case kcOrderlistPat4:
	case kcOrderlistPat5:
	case kcOrderlistPat6:
	case kcOrderlistPat7:
	case kcOrderlistPat8:
	case kcOrderlistPat9:
		EnterPatternNum(wParam - kcOrderlistPat0); return wParam;
	case kcOrderlistPatMinus:
		EnterPatternNum(10); return wParam;
	case kcOrderlistPatPlus:
		EnterPatternNum(11); return wParam;
	case kcOrderlistPatIgnore:
		EnterPatternNum(12); return wParam;
	case kcOrderlistPatInvalid:
		EnterPatternNum(13); return wParam;

	// kCtxViewPatternsNote messages
	case kcSwitchToOrderList:
		OnSwitchToView();
		return wParam;
	case kcChangeLoopStatus:
		m_pParent.OnModCtrlMsg(CTRLMSG_PAT_LOOP, -1); return wParam;
	case kcToggleFollowSong:
		m_pParent.OnModCtrlMsg(CTRLMSG_PAT_FOLLOWSONG, 1); return wParam;

	case kcChannelUnmuteAll:
	case kcUnmuteAllChnOnPatTransition:
		::PostMessage(m_pParent.GetViewWnd(), WM_MOD_KEYCOMMAND, wParam, 0); return wParam;

	case kcOrderlistLockPlayback:
		OnLockPlayback(); return wParam;
	case kcOrderlistUnlockPlayback:
		OnUnlockPlayback(); return wParam;

	case kcDuplicatePattern:
		OnDuplicatePattern(); return wParam;
	case kcNewPattern:
		OnCreateNewPattern(); return wParam;
	}

	return 0;
}


// Helper function to enter pattern index into the orderlist.
// Call with param 0...9 (enter digit), 10 (decrease) or 11 (increase).
void COrderList::EnterPatternNum(int enterNum)
//--------------------------------------------
{
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();

	PATTERNINDEX nCurNdx = (m_nScrollPos < sndFile.Order.GetLength()) ? sndFile.Order[m_nScrollPos] : sndFile.Order.GetInvalidPatIndex();
	PATTERNINDEX nMaxNdx = 0;
	for(PATTERNINDEX nPat = 0; nPat < sndFile.Patterns.Size(); nPat++)
		if (sndFile.Patterns.IsValidPat(nPat)) nMaxNdx = nPat;

	if (enterNum >= 0 && enterNum <= 9) // enter 0...9
	{
		if (nCurNdx >= sndFile.Patterns.Size()) nCurNdx = 0;

		nCurNdx = nCurNdx * 10 + static_cast<PATTERNINDEX>(enterNum);
		STATIC_ASSERT(MAX_PATTERNS < 10000);
		if ((nCurNdx >= 1000) && (nCurNdx > nMaxNdx)) nCurNdx %= 1000;
		if ((nCurNdx >= 100) && (nCurNdx > nMaxNdx)) nCurNdx %= 100;
		if ((nCurNdx >= 10) && (nCurNdx > nMaxNdx)) nCurNdx %= 10;
	} else if (enterNum == 10) // decrease pattern index
	{
		const PATTERNINDEX nFirstInvalid = sndFile.GetModSpecifications().hasIgnoreIndex ? sndFile.Order.GetIgnoreIndex() : sndFile.Order.GetInvalidPatIndex();
		if (nCurNdx == 0)
			nCurNdx = sndFile.Order.GetInvalidPatIndex();
		else
		{
			nCurNdx--;
			if ((nCurNdx > nMaxNdx) && (nCurNdx < nFirstInvalid)) nCurNdx = nMaxNdx;
		}
	} else if (enterNum == 11) // increase pattern index
	{
		if(nCurNdx >= sndFile.Order.GetInvalidPatIndex())
		{
			nCurNdx = 0;
		}
		else
		{
			nCurNdx++;
			const PATTERNINDEX nFirstInvalid = sndFile.GetModSpecifications().hasIgnoreIndex ? sndFile.Order.GetIgnoreIndex() : sndFile.Order.GetInvalidPatIndex();
			if(nCurNdx > nMaxNdx && nCurNdx < nFirstInvalid)
				nCurNdx = nFirstInvalid;
		}
	} else if (enterNum == 12) // ignore index (+++)
	{
		if (sndFile.GetModSpecifications().hasIgnoreIndex)
		{
			nCurNdx = sndFile.Order.GetIgnoreIndex();
		}
	} else if (enterNum == 13) // invalid index (---)
	{
		nCurNdx = sndFile.Order.GetInvalidPatIndex();
	}
	// apply
	if (nCurNdx != sndFile.Order[m_nScrollPos])
	{
		sndFile.Order[m_nScrollPos] = nCurNdx;
		m_pModDoc.SetModified();
		m_pModDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
		InvalidateSelection();
	}
}


void COrderList::OnEditCut()
//--------------------------
{
	OnEditCopy();
	OnDeleteOrder();
}


void COrderList::OnCopy(bool onlyOrders)
//--------------------------------------
{
	const OrdSelection ordsel = GetCurSel(false);
	BeginWaitCursor();
	PatternClipboard::Copy(m_pModDoc.GetrSoundFile(), ordsel.firstOrd, ordsel.lastOrd, onlyOrders);
	PatternClipboardDialog::UpdateList();
	EndWaitCursor();
}


void COrderList::UpdateView(UpdateHint hint, CObject *pObj)
//---------------------------------------------------------
{
	if(pObj != this && hint.ToType<SequenceHint>().GetType()[HINT_MODTYPE | HINT_MODSEQUENCE])
	{
		InvalidateRect(NULL, FALSE);
		UpdateInfoText();
	}
	if(hint.GetType()[HINT_MPTOPTIONS])
	{
		m_nOrderlistMargins = TrackerSettings::Instance().orderlistMargins;
	}
}


void COrderList::OnSwitchToView()
//-------------------------------
{
	m_pParent.PostViewMessage(VIEWMSG_SETFOCUS);
}


void COrderList::UpdateInfoText()
//-------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm != nullptr && ::GetFocus() == m_hWnd)
	{
		CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
		TCHAR s[128] = _T("");

		const ORDERINDEX nLength = sndFile.Order.GetLengthTailTrimmed();

		if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_HEXDISPLAY)
		{
			wsprintf(s, _T("Position %02Xh of %02Xh"), m_nScrollPos, nLength);
		}
		else
		{
			wsprintf(s, _T("Position %u of %u (%02Xh of %02Xh)"), m_nScrollPos, nLength, m_nScrollPos, nLength);
		}
		
		if (m_nScrollPos < sndFile.Order.GetLength())
		{
			PATTERNINDEX nPat = sndFile.Order[m_nScrollPos];
			if (nPat < sndFile.Patterns.Size())
			{
				std::string patName = sndFile.Patterns[nPat].GetName();
				if (!patName.empty())
				{
					_tcscat(s, _T(": "));
					_tcscat(s, patName.c_str());
				}
			}
		}
		pMainFrm->SetInfoText(s);
	}
}


/////////////////////////////////////////////////////////////////
// COrderList messages

void COrderList::OnPaint()
//------------------------
{
	CHAR s[64];
	CPaintDC dc(this);
	HGDIOBJ oldfont = ::SelectObject(dc.m_hDC, m_hFont);
	HGDIOBJ oldpen = ::SelectObject(dc.m_hDC, CMainFrame::penSeparator);
	// First time ?
	if ((m_cxFont <= 0) || (m_cyFont <= 0))
	{
		CSize sz = dc.GetTextExtent("000+", 4);
		m_cxFont = sz.cx;
		m_cyFont = sz.cy;
	}

	if ((m_cxFont > 0) && (m_cyFont > 0))
	{
		CRect rcClient, rect;

		UpdateScrollInfo();
		BOOL bFocus = (::GetFocus() == m_hWnd);
		dc.SetBkMode(TRANSPARENT);
		GetClientRect(&rcClient);
		rect = rcClient;
		ORDERINDEX nIndex = m_nXScroll;
		OrdSelection selection = GetCurSel(false);

		CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
		ORDERINDEX maxEntries = sndFile.GetModSpecifications().ordersMax;
		if(sndFile.Order.GetLength() > maxEntries)
		{
			// Only computed if potentially needed.
			maxEntries = std::max(maxEntries, sndFile.Order.GetLengthTailTrimmed());
		}

		// Scrolling the shown orders(the showns rectangles)?
		while (rect.left < rcClient.right)
		{
			dc.SetTextColor(colorText);
			bool bHighLight = ((bFocus) && (nIndex >= selection.firstOrd && nIndex <= selection.lastOrd));
			const PATTERNINDEX nPat = (nIndex < sndFile.Order.GetLength()) ? sndFile.Order[nIndex] : PATTERNINDEX_INVALID;
			if ((rect.right = rect.left + m_cxFont) > rcClient.right) rect.right = rcClient.right;
			rect.right--;

			if(bHighLight)
			{
				// Currently selected order item
				FillRect(dc.m_hDC, &rect, CMainFrame::brushHighLight);
			} else if(sndFile.Order.IsPositionLocked(nIndex))
			{
				// "Playback lock" indicator - grey out all order items which aren't played.
				FillRect(dc.m_hDC, &rect, CMainFrame::brushGray);
			} else
			{
				// Normal, unselected item.
				FillRect(dc.m_hDC, &rect, CMainFrame::brushWindow);
			}

			// Drawing the shown pattern-indicator or drag position.
			if (nIndex == ((m_bDragging) ? m_nDropPos : m_nScrollPos))
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			MoveToEx(dc.m_hDC, rect.right, rect.top, NULL);
			LineTo(dc.m_hDC, rect.right, rect.bottom);

			// Drawing the 'ctrl-transition' indicator
			if(nIndex == sndFile.m_PlayState.m_nSeqOverride)
			{
				MoveToEx(dc.m_hDC, rect.left + 4, rect.bottom - 4, NULL);
				LineTo(dc.m_hDC, rect.right - 4, rect.bottom - 4);
			}

			// Drawing 'playing'-indicator.
			if(nIndex == sndFile.GetCurrentOrder() && CMainFrame::GetMainFrame()->IsPlaying())
			{
				MoveToEx(dc.m_hDC, rect.left + 4, rect.top + 2, NULL);
				LineTo(dc.m_hDC, rect.right - 4, rect.top + 2);
			}

			s[0] = '\0';
			if(nIndex < maxEntries && (rect.left + m_cxFont - 4) <= rcClient.right)
			{
				if(nPat == sndFile.Order.GetInvalidPatIndex()) strcpy(s, "---");
				else if(nPat == sndFile.Order.GetIgnoreIndex()) strcpy(s, "+++");
				else wsprintf(s, _T("%u"), nPat);
			}

			const COLORREF &textCol =
				(bHighLight
					? colorTextSel			// Highlighted pattern
					: (sndFile.Patterns.IsValidPat(nPat)
						? colorText			// Normal pattern
						: colorInvalid));	// Non-existent pattern
			dc.SetTextColor(textCol);
			dc.DrawText(s, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			rect.left += m_cxFont;
			nIndex++;
		}
	}
	if (oldpen) ::SelectObject(dc.m_hDC, oldpen);
	if (oldfont) ::SelectObject(dc.m_hDC, oldfont);
}


void COrderList::OnSetFocus(CWnd *pWnd)
//-------------------------------------
{
	CWnd::OnSetFocus(pWnd);
	InvalidateSelection();
	UpdateInfoText();
	CMainFrame::GetMainFrame()->m_pOrderlistHasFocus = this;
}


void COrderList::OnKillFocus(CWnd *pWnd)
//--------------------------------------
{
	CWnd::OnKillFocus(pWnd);
	InvalidateSelection();
	CMainFrame::GetMainFrame()->m_pOrderlistHasFocus = nullptr;
}


void COrderList::OnLButtonDown(UINT nFlags, CPoint pt)
//----------------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	if (pt.y < rect.bottom)
	{
		SetFocus();

		if(IsCtrlKeyPressed())
		{
			// Queue pattern
			QueuePattern(pt);
		} else
		{
			// mark pattern (+skip to)
			const int oldXScroll = m_nXScroll;

			ORDERINDEX nOrder = GetOrderFromPoint(rect, pt);
			OrdSelection selection = GetCurSel(false);

			// check if cursor is in selection - if it is, only react on MouseUp as the user might want to drag those orders
			if(m_nScrollPos2nd == ORDERINDEX_INVALID || nOrder < selection.firstOrd || nOrder > selection.lastOrd)
			{
				m_nScrollPos2nd = ORDERINDEX_INVALID;
				SetCurSel(nOrder, true, IsSelectionKeyPressed());
			}
			m_bDragging = !IsOrderInMargins(m_nScrollPos, oldXScroll) || !IsOrderInMargins(m_nScrollPos2nd, oldXScroll);

			m_nMouseDownPos = nOrder;
			if(m_bDragging)
			{
				m_nDragOrder = m_nDropPos = GetCurSel(true).firstOrd;
				SetCapture();
			}
		}
	} else
	{
		CWnd::OnLButtonDown(nFlags, pt);
	}
}


void COrderList::OnLButtonUp(UINT nFlags, CPoint pt)
//--------------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);

	// Copy or move orders?
	const bool copyOrders = IsSelectionKeyPressed();

	if (m_bDragging)
	{
		m_bDragging = false;
		ReleaseCapture();
		if (rect.PtInRect(pt))
		{
			ORDERINDEX n = GetOrderFromPoint(rect, pt);
			if (n != ORDERINDEX_INVALID && n == m_nDropPos && n != m_nMouseDownPos)
			{
				// drag multiple orders (not quite as easy...)
				OrdSelection selection = GetCurSel(false);
				// move how many orders from where?
				ORDERINDEX moveCount = (selection.lastOrd - selection.firstOrd), movePos = selection.firstOrd;
				// drop before or after the selection
				bool moveBack = !(m_nDragOrder < m_nDropPos);
				// don't do anything if drop position is inside the selection
				if((m_nDropPos >= selection.firstOrd && m_nDropPos <= selection.lastOrd) || m_nDragOrder == m_nDropPos) return;
				// drag one order or multiple orders?
				bool multiSelection = (selection.firstOrd != selection.lastOrd);

				for(int i = 0; i <= moveCount; i++)
				{
					if(!m_pModDoc.MoveOrder(movePos, m_nDropPos, true, copyOrders)) return;
					if((moveBack ^ copyOrders) == true && multiSelection)
					{
						movePos++;
						m_nDropPos++;
					}
					if(moveBack && copyOrders && multiSelection)
					{
						movePos += 2;
						m_nDropPos++;
					}
				}

				if(multiSelection)
				{
					// adjust selection
					m_nScrollPos2nd = m_nDropPos - 1;
					m_nDropPos -= moveCount + (moveBack ? 0 : 1);
					SetCurSel((moveBack && !copyOrders) ? m_nDropPos - 1 : m_nDropPos);
				} else
				{
					SetCurSel((m_nDragOrder < m_nDropPos && !copyOrders) ? m_nDropPos - 1 : m_nDropPos);
				}
				// Did we actually change anything?
				if(n != selection.lastOrd + 1 || copyOrders)
				{
					m_pModDoc.SetModified();
				}
			} else
			{
				if(pt.y < rect.bottom && n == m_nMouseDownPos && !copyOrders)
				{
					// Remove selection if we didn't drag anything but multiselect was active
					m_nScrollPos2nd = ORDERINDEX_INVALID;
					SetFocus();
					SetCurSel(n);
				}
			}
		}
		InvalidateRect(NULL, FALSE);
	} else
	{
		CWnd::OnLButtonUp(nFlags, pt);
	}
}


void COrderList::OnMouseMove(UINT nFlags, CPoint pt)
//--------------------------------------------------
{
	if ((m_bDragging) && (m_cxFont))
	{
		CRect rect;

		GetClientRect(&rect);
		ORDERINDEX n = ORDERINDEX_INVALID;
		if (rect.PtInRect(pt))
		{
			CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
			n = GetOrderFromPoint(rect, pt);
			if (n >= sndFile.Order.GetLength() || n >= sndFile.GetModSpecifications().ordersMax) n = ORDERINDEX_INVALID;
		}
		if (n != m_nDropPos)
		{
			if (n != ORDERINDEX_INVALID)
			{
				m_nMouseDownPos = ORDERINDEX_INVALID;
				m_nDropPos = n;
				InvalidateRect(NULL, FALSE);
				SetCursor(CMainFrame::curDragging);
			} else
			{
				m_nDropPos = ORDERINDEX_INVALID;
				SetCursor(CMainFrame::curNoDrop);
			}
		}
	} else
	{
		CWnd::OnMouseMove(nFlags, pt);
	}
}


void COrderList::OnSelectSequence(UINT nid)
//-----------------------------------------
{
	SelectSequence(static_cast<SEQUENCEINDEX>(nid - ID_SEQUENCE_ITEM));
}


void COrderList::OnRButtonDown(UINT nFlags, CPoint pt)
//----------------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	if (m_bDragging)
	{
		m_nDropPos = ORDERINDEX_INVALID;
		OnLButtonUp(nFlags, pt);
	}
	if (pt.y >= rect.bottom) return;

	bool multiSelection = (m_nScrollPos2nd != ORDERINDEX_INVALID);

	if(!multiSelection) SetCurSel(GetOrderFromPoint(rect, pt), true, false, false, false);
	SetFocus();
	HMENU hMenu = ::CreatePopupMenu();
	if(!hMenu) return;

	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();

	// Check if at least one pattern in the current selection exists
	bool patExists = false;
	OrdSelection selection = GetCurSel(false);
	for(ORDERINDEX ord = selection.firstOrd; ord <= selection.lastOrd && !patExists; ord++)
	{
		patExists = sndFile.Patterns.IsValidPat(sndFile.Order[ord]);
	}

	const DWORD greyed = patExists ? 0 : MF_GRAYED;

	CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();

	if(multiSelection)
	{
		// Several patterns are selected.
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_INSERT, "&Insert Patterns\t" + ih->GetKeyTextFromCommand(kcOrderlistEditInsert));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_DELETE, "&Remove Patterns\t" + ih->GetKeyTextFromCommand(kcOrderlistEditDelete));
		AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_EDIT_COPY, "&Copy Patterns\t" + ih->GetKeyTextFromCommand(kcEditCopy));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_EDIT_COPY_ORDERS, "&Copy Orders\t" + ih->GetKeyTextFromCommand(kcOrderlistEditCopyOrders));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_EDIT_CUT, "&C&ut Patterns\t" + ih->GetKeyTextFromCommand(kcEditCut));
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERNPASTE, "P&aste Patterns\t" + ih->GetKeyTextFromCommand(kcEditPaste));
		AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
		AppendMenu(hMenu, MF_STRING | greyed, ID_ORDERLIST_COPY, "&Duplicate Patterns\t" + ih->GetKeyTextFromCommand(kcDuplicatePattern));
	} else
	{
		// Only one pattern is selected
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_INSERT, "&Insert Pattern\t" + ih->GetKeyTextFromCommand(kcOrderlistEditInsert));
		if(sndFile.GetModSpecifications().hasIgnoreIndex)
		{
			AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_INSERT_SEPARATOR, "&Insert Separator\t" + ih->GetKeyTextFromCommand(kcOrderlistPatIgnore));
		}
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_DELETE, "&Remove Pattern\t" + ih->GetKeyTextFromCommand(kcOrderlistEditDelete));
		AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_NEW, "Create &New Pattern\t" + ih->GetKeyTextFromCommand(kcNewPattern));
		AppendMenu(hMenu, MF_STRING | greyed, ID_ORDERLIST_COPY, "&Duplicate Pattern\t" + ih->GetKeyTextFromCommand(kcDuplicatePattern));
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERNCOPY, "&Copy Pattern");
		AppendMenu(hMenu, MF_STRING, ID_PATTERNPASTE, "P&aste Pattern\t" + ih->GetKeyTextFromCommand(kcEditPaste));
		if (sndFile.TypeIsIT_MPT_XM())
		{
			AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
			AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_PROPERTIES, "&Pattern Properties...");
		}
		if (sndFile.GetModSpecifications().sequencesMax > 1)
		{
			AppendMenu(hMenu, MF_SEPARATOR, NULL, "");

			HMENU menuSequence = ::CreatePopupMenu();
			AppendMenu(hMenu, MF_POPUP, (UINT_PTR)menuSequence, TEXT("Sequences"));
			
			const SEQUENCEINDEX numSequences = sndFile.Order.GetNumSequences();
			for(SEQUENCEINDEX i = 0; i < numSequences; i++)
			{
				CString str;
				if(sndFile.Order.GetSequence(i).GetName().empty())
					str.Format(TEXT("Sequence %u"), i);
				else
					str.Format(TEXT("%u: %s"), i, sndFile.Order.GetSequence(i).GetName().c_str());
				const UINT flags = (sndFile.Order.GetCurrentSequenceIndex() == i) ? MF_STRING|MF_CHECKED : MF_STRING;
				AppendMenu(menuSequence, flags, ID_SEQUENCE_ITEM + i, str);
			}
			if (sndFile.Order.GetNumSequences() < MAX_SEQUENCES)
			{
				AppendMenu(menuSequence, MF_STRING, ID_SEQUENCE_ITEM + MAX_SEQUENCES, TEXT("&Duplicate current sequence"));
				AppendMenu(menuSequence, MF_STRING, ID_SEQUENCE_ITEM + MAX_SEQUENCES + 1, TEXT("&Create empty sequence"));
			}
			if (sndFile.Order.GetNumSequences() > 1)
				AppendMenu(menuSequence, MF_STRING, ID_SEQUENCE_ITEM + MAX_SEQUENCES + 2, TEXT("D&elete current sequence"));
		}
	}
	AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
	AppendMenu(hMenu, ((selection.firstOrd == sndFile.m_lockOrderStart && selection.lastOrd == sndFile.m_lockOrderEnd) ? (MF_STRING | MF_CHECKED) : MF_STRING), ID_ORDERLIST_LOCKPLAYBACK, "&Lock Playback to Selection\t" + ih->GetKeyTextFromCommand(kcOrderlistLockPlayback));
	AppendMenu(hMenu, (sndFile.m_lockOrderStart == ORDERINDEX_INVALID ? (MF_STRING | MF_GRAYED) : MF_STRING), ID_ORDERLIST_UNLOCKPLAYBACK, "&Unlock Playback\t" + ih->GetKeyTextFromCommand(kcOrderlistUnlockPlayback));

	AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
	AppendMenu(hMenu, MF_STRING | greyed, ID_ORDERLIST_RENDER, "Render to &Wave");

	ClientToScreen(&pt);
	::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
	::DestroyMenu(hMenu);
}


void COrderList::OnLButtonDblClk(UINT, CPoint)
//--------------------------------------------
{
	m_nScrollPos2nd = ORDERINDEX_INVALID;
	SetFocus();
	m_pParent.SetCurrentPattern(m_pModDoc.GetrSoundFile().Order[m_nScrollPos]);
}


void COrderList::OnMButtonDown(UINT nFlags, CPoint pt)
//----------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(nFlags);
	QueuePattern(pt);
}


void COrderList::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *)
//---------------------------------------------------------------
{
	UINT nNewPos = m_nXScroll;
	UINT smin, smax;
	
	GetScrollRange(SB_HORZ, (LPINT)&smin, (LPINT)&smax);
	ASSERT(smin >= 0);	// This should never be negative. Otherwise, nPos may be negative too, which we don't account for, because it shouldn't happen... :)
	m_bScrolling = true;
	switch(nSBCode)
	{
	case SB_LINELEFT:		if (nNewPos) nNewPos--; break;
	case SB_LINERIGHT:		if (nNewPos < smax) nNewPos++; break;
	case SB_PAGELEFT:		if (nNewPos > 4) nNewPos -= 4; else nNewPos = 0; break;
	case SB_PAGERIGHT:		if (nNewPos + 4 < smax) nNewPos += 4; else nNewPos = smax; break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:		nNewPos = nPos; break;
	case SB_LEFT:			nNewPos = 0; break;
	case SB_RIGHT:			nNewPos = smax; break;
	case SB_ENDSCROLL:		m_bScrolling = false; break;
	}
	if (nNewPos > smax) nNewPos = smax;
	if (nNewPos != m_nXScroll)
	{
		m_nXScroll = static_cast<ORDERINDEX>(nNewPos);
		SetScrollPos(SB_HORZ, m_nXScroll);
		InvalidateRect(NULL, FALSE);
	}
}


void COrderList::OnSize(UINT nType, int cx, int cy)
//-------------------------------------------------
{
	int nPos;
	int smin, smax;

	CWnd::OnSize(nType, cx, cy);
	UpdateScrollInfo();
	GetScrollRange(SB_HORZ, &smin, &smax);
	nPos = GetScrollPos(SB_HORZ);
	if (nPos > smax) nPos = smax;
	if (m_nXScroll != nPos)
	{
		m_nXScroll = static_cast<ORDERINDEX>(nPos);
		SetScrollPos(SB_HORZ, m_nXScroll);
		InvalidateRect(NULL, FALSE);
	}
}


void COrderList::OnInsertOrder()
//------------------------------
{
	// insert the same order(s) after the currently selected order(s)
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();

	const OrdSelection selection = GetCurSel(false);
	const ORDERINDEX insertCount = selection.lastOrd - selection.firstOrd, insertEnd = selection.lastOrd;

	for(ORDERINDEX i = 0; i <= insertCount; i++)
	{
		// Checking whether there is some pattern at the end of orderlist.
		if (sndFile.Order.GetLength() < 1 || sndFile.Order.Last() < sndFile.Patterns.Size())
		{
			if(sndFile.Order.GetLength() < sndFile.GetModSpecifications().ordersMax)
				sndFile.Order.Append();
		}
		for(int j = sndFile.Order.GetLastIndex(); j > insertEnd; j--)
			sndFile.Order[j] = sndFile.Order[j - 1];
	}
	// now that there is enough space in the order list, overwrite the orders
	for(ORDERINDEX i = 0; i <= insertCount; i++)
	{
		if(insertEnd + i + 1 < sndFile.GetModSpecifications().ordersMax
			&& insertEnd + i + 1 < sndFile.Order.GetLength())
		{
			sndFile.Order[insertEnd + i + 1] = sndFile.Order[insertEnd - insertCount + i];
		}
	}

	Util::InsertItem(selection.firstOrd, selection.lastOrd, sndFile.m_PlayState.m_nNextOrder);
	if(sndFile.m_PlayState.m_nSeqOverride != ORDERINDEX_INVALID)
	{
		Util::InsertItem(selection.firstOrd, selection.lastOrd, sndFile.m_PlayState.m_nSeqOverride);
	}
	// Adjust order lock position
	if(sndFile.m_lockOrderStart != ORDERINDEX_INVALID)
	{
		Util::InsertRange(selection.firstOrd, selection.lastOrd, sndFile.m_lockOrderStart, sndFile.m_lockOrderEnd);
	}

	m_nScrollPos = std::min(ORDERINDEX(insertEnd + 1), sndFile.Order.GetLastIndex());
	if(insertCount > 0)
		m_nScrollPos2nd = std::min(ORDERINDEX(m_nScrollPos + insertCount), sndFile.Order.GetLastIndex());
	else
		m_nScrollPos2nd = ORDERINDEX_INVALID;

	InvalidateSelection();
	EnsureVisible(m_nScrollPos2nd);
	// first inserted order has higher priority than the last one
	EnsureVisible(m_nScrollPos);

	InvalidateRect(NULL, FALSE);
	m_pModDoc.SetModified();
	m_pModDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
}


void COrderList::OnInsertSeparatorPattern()
//-----------------------------------------
{
	// Insert a separator pattern after the current pattern, don't move order list cursor
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();

	const OrdSelection selection = GetCurSel(true);
	ORDERINDEX insertPos = selection.firstOrd;
		
	if(sndFile.Order[insertPos] != sndFile.Order.GetInvalidPatIndex())
	{
		// If we're not inserting on a stop (---) index, we move on by one position.
		insertPos++;
		// Checking whether there is some pattern at the end of orderlist.
		if (sndFile.Order.GetLength() < 1 || sndFile.Order.Last() < sndFile.Patterns.Size())
		{
			if(sndFile.Order.GetLength() < sndFile.GetModSpecifications().ordersMax)
				sndFile.Order.Append();
		}
		for(int j = sndFile.Order.GetLastIndex(); j > insertPos; j--)
			sndFile.Order[j] = sndFile.Order[j - 1];

	}

	sndFile.Order[insertPos] = sndFile.Order.GetIgnoreIndex();

	Util::InsertItem(insertPos, insertPos, sndFile.m_PlayState.m_nNextOrder);
	if(sndFile.m_PlayState.m_nSeqOverride != ORDERINDEX_INVALID)
	{
		Util::InsertItem(insertPos, insertPos, sndFile.m_PlayState.m_nSeqOverride);
	}
	// Adjust order lock position
	if(sndFile.m_lockOrderStart != ORDERINDEX_INVALID)
	{
		Util::InsertRange(insertPos, insertPos, sndFile.m_lockOrderStart, sndFile.m_lockOrderEnd);
	}

	InvalidateRect(NULL, FALSE);
	m_pModDoc.SetModified();
	m_pModDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
}


void COrderList::OnRenderOrder()
//------------------------------
{
	OrdSelection selection = GetCurSel(false);
	m_pModDoc.OnFileMP3Convert(selection.firstOrd, selection.lastOrd);
}


void COrderList::OnDeleteOrder()
//------------------------------
{
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();

	OrdSelection selection = GetCurSel(false);
	// remove selection
	m_nScrollPos2nd = ORDERINDEX_INVALID;

	sndFile.Order.Remove(selection.firstOrd, selection.lastOrd);

	m_pModDoc.SetModified();
	InvalidateRect(NULL, FALSE);
	m_pModDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);

	Util::DeleteItem(selection.firstOrd, selection.lastOrd, sndFile.m_PlayState.m_nNextOrder);
	if(sndFile.m_PlayState.m_nSeqOverride != ORDERINDEX_INVALID)
	{
		Util::DeleteItem(selection.firstOrd, selection.lastOrd, sndFile.m_PlayState.m_nSeqOverride);
	}

	// Adjust order lock position
	if(sndFile.m_lockOrderStart != ORDERINDEX_INVALID)
	{
		Util::DeleteRange(selection.firstOrd, selection.lastOrd, sndFile.m_lockOrderStart, sndFile.m_lockOrderEnd);
	}

	SetCurSel(selection.firstOrd, true, false, true);
}


void COrderList::OnPatternProperties()
//------------------------------------
{
	m_pParent.PostViewMessage(VIEWMSG_PATTERNPROPERTIES);
}


void COrderList::OnPlayerPlay()
//-----------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_PLAYER_PLAY);
}


void COrderList::OnPlayerPause()
//------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_PLAYER_PAUSE);
}


void COrderList::OnPlayerPlayFromStart()
//--------------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_PLAYER_PLAYFROMSTART);
}


void COrderList::OnPatternPlayFromStart()
//---------------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, IDC_PATTERN_PLAYFROMSTART);
}


void COrderList::OnCreateNewPattern()
//-----------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_ORDERLIST_NEW);
}


void COrderList::OnDuplicatePattern()
//-----------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_ORDERLIST_COPY);
}


void COrderList::OnPatternCopy()
//------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_PATTERNCOPY);
}


void COrderList::OnPatternPaste()
//-------------------------------
{
	m_pParent.PostMessage(WM_COMMAND, ID_PATTERNPASTE);
}


LRESULT COrderList::OnHelpHitTest(WPARAM, LPARAM)
//-----------------------------------------------
{
	return HID_BASE_COMMAND + IDC_ORDERLIST;
}


LRESULT COrderList::OnDragonDropping(WPARAM doDrop, LPARAM lParam)
//-----------------------------------------------------------------
{
	const DRAGONDROP *pDropInfo = (const DRAGONDROP *)lParam;
	ORDERINDEX posdest;
	BOOL canDrop;
	CPoint pt;

	if ((!pDropInfo) || (&m_pModDoc != pDropInfo->pModDoc) || (!m_cxFont)) return FALSE;
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
	canDrop = FALSE;
	switch(pDropInfo->dwDropType)
	{
	case DRAGONDROP_ORDER:
		if (pDropInfo->dwDropItem >= sndFile.Order.size()) break;
	case DRAGONDROP_PATTERN:
		canDrop = TRUE;
		break;
	}
	if(!canDrop || !doDrop) return canDrop;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	if (pt.x < 0) pt.x = 0;
	posdest = static_cast<ORDERINDEX>(m_nXScroll + (pt.x / m_cxFont));
	if (posdest >= sndFile.Order.GetLength()) return FALSE;
	switch(pDropInfo->dwDropType)
	{
	case DRAGONDROP_PATTERN:
		sndFile.Order[posdest] = static_cast<PATTERNINDEX>(pDropInfo->dwDropItem);
		break;

	case DRAGONDROP_ORDER:
		sndFile.Order[posdest] = sndFile.Order[pDropInfo->dwDropItem];
		break;
	}
	if (canDrop)
	{
		InvalidateRect(NULL, FALSE);
		m_pModDoc.SetModified();
		m_pModDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
		SetCurSel(posdest, true);
	}
	return canDrop;
}


ORDERINDEX COrderList::SetMargins(int i)
//--------------------------------------
{
	m_nOrderlistMargins = i;
	return GetMargins();
}

void COrderList::SelectSequence(const SEQUENCEINDEX nSeq)
//-------------------------------------------------------
{
	CriticalSection cs;

	CMainFrame::GetMainFrame()->ResetNotificationBuffer();
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
	const bool editSequence = nSeq >= sndFile.Order.GetNumSequences();
	if(nSeq == MAX_SEQUENCES + 2)
	{
		std::wstring str = L"Delete sequence " + mpt::ToWString(sndFile.Order.GetCurrentSequenceIndex()) + L": " + mpt::ToWide(mpt::CharsetLocale, sndFile.Order.GetName()) + L"?";
		if (Reporting::Confirm(str) == cnfYes)
			sndFile.Order.RemoveSequence();
		else
			return;
	}
	else if(nSeq == sndFile.Order.GetCurrentSequenceIndex())
		return;
	else if(nSeq == MAX_SEQUENCES || nSeq == MAX_SEQUENCES + 1)
		sndFile.Order.AddSequence((nSeq == MAX_SEQUENCES));
	else if(nSeq < sndFile.Order.GetNumSequences())
		sndFile.Order.SetSequence(nSeq);
	ORDERINDEX nPosCandidate = sndFile.Order.GetLengthTailTrimmed() - 1;
	SetCurSel(std::min(m_nScrollPos, nPosCandidate), true, false, true);
	m_pParent.SetCurrentPattern(sndFile.Order[m_nScrollPos]);

	UpdateScrollInfo();
	// This won't make sense anymore in the new sequence.
	OnUnlockPlayback();

	cs.Leave();

	if(editSequence)
		m_pModDoc.SetModified();
	m_pModDoc.UpdateAllViews(nullptr, SequenceHint().Data(), nullptr);
}


void COrderList::QueuePattern(CPoint pt)
//--------------------------------------
{
	CRect rect;
	GetClientRect(&rect);

	if(!rect.PtInRect(pt)) return;
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();

	const PATTERNINDEX ignoreIndex = sndFile.Order.GetIgnoreIndex();
	const PATTERNINDEX stopIndex = sndFile.Order.GetInvalidPatIndex();
	const ORDERINDEX length = sndFile.Order.GetLength();
	ORDERINDEX order = GetOrderFromPoint(rect, pt);

	// If this is not a playable order item, find the next valid item.
	while(order < length && (sndFile.Order[order] == ignoreIndex || sndFile.Order[order] == stopIndex))
	{
		order++;
	}

	if(order < length)
	{
		if(sndFile.m_PlayState.m_nSeqOverride == order)
		{
			// This item is already queued: Dequeue it.
			sndFile.m_PlayState.m_nSeqOverride = ORDERINDEX_INVALID;
		} else
		{
			if(sndFile.Order.IsPositionLocked(order))
			{
				// Users wants to go somewhere else, so let him do that.
				OnUnlockPlayback();
			}

			sndFile.m_PlayState.m_nSeqOverride = order;
		}
		InvalidateRect(NULL, FALSE);
	}
}


void COrderList::OnLockPlayback()
//-------------------------------
{
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();

	OrdSelection selection = GetCurSel(false);
	if(selection.firstOrd == sndFile.m_lockOrderStart && selection.lastOrd == sndFile.m_lockOrderEnd)
	{
		OnUnlockPlayback();
	} else
	{
		sndFile.m_lockOrderStart = selection.firstOrd;
		sndFile.m_lockOrderEnd = selection.lastOrd;
		InvalidateRect(NULL, FALSE);
	}
}


void COrderList::OnUnlockPlayback()
//----------------------------------
{
	CSoundFile &sndFile = m_pModDoc.GetrSoundFile();
	sndFile.m_lockOrderStart = sndFile.m_lockOrderEnd = ORDERINDEX_INVALID;
	InvalidateRect(NULL, FALSE);
}


OPENMPT_NAMESPACE_END
