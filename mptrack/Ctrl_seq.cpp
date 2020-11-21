/*
 * Ctrl_seq.cpp
 * ------------
 * Purpose: Order list for the pattern editor upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Moddoc.h"
#include "../soundlib/mod_specifications.h"
#include "Globals.h"
#include "Ctrl_pat.h"
#include "PatternClipboard.h"
#include "../common/mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN

enum SequenceAction : SEQUENCEINDEX
{
	kAddSequence = MAX_SEQUENCES,
	kDuplicateSequence,
	kDeleteSequence,
	kSplitSequence,

	kMaxSequenceActions
};

// Little helper function to avoid copypasta
static bool IsSelectionKeyPressed() { return CMainFrame::GetInputHandler()->SelectionPressed(); }
static bool IsCtrlKeyPressed() { return CMainFrame::GetInputHandler()->CtrlPressed(); }


//////////////////////////////////////////////////////////////
// CPatEdit

BOOL CPatEdit::PreTranslateMessage(MSG *pMsg)
{
	if(((pMsg->message == WM_KEYDOWN) || (pMsg->message == WM_KEYUP)) && (pMsg->wParam == VK_TAB))
	{
		if((pMsg->message == WM_KEYUP) && (m_pParent))
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

	ON_COMMAND(ID_ORDERLIST_INSERT,				&COrderList::OnInsertOrder)
	ON_COMMAND(ID_ORDERLIST_INSERT_SEPARATOR,	&COrderList::OnInsertSeparatorPattern)
	ON_COMMAND(ID_ORDERLIST_DELETE,				&COrderList::OnDeleteOrder)
	ON_COMMAND(ID_ORDERLIST_RENDER,				&COrderList::OnRenderOrder)
	ON_COMMAND(ID_ORDERLIST_EDIT_COPY,			&COrderList::OnEditCopy)
	ON_COMMAND(ID_ORDERLIST_EDIT_CUT,			&COrderList::OnEditCut)
	ON_COMMAND(ID_ORDERLIST_EDIT_COPY_ORDERS,	&COrderList::OnEditCopyOrders)
	
	ON_COMMAND(ID_PATTERN_PROPERTIES,			&COrderList::OnPatternProperties)
	ON_COMMAND(ID_PLAYER_PLAY,					&COrderList::OnPlayerPlay)
	ON_COMMAND(ID_PLAYER_PAUSE,					&COrderList::OnPlayerPause)
	ON_COMMAND(ID_PLAYER_PLAYFROMSTART,			&COrderList::OnPlayerPlayFromStart)
	ON_COMMAND(IDC_PATTERN_PLAYFROMSTART,		&COrderList::OnPatternPlayFromStart)
	ON_COMMAND(ID_ORDERLIST_NEW,				&COrderList::OnCreateNewPattern)
	ON_COMMAND(ID_ORDERLIST_COPY,				&COrderList::OnDuplicatePattern)
	ON_COMMAND(ID_ORDERLIST_MERGE,				&COrderList::OnMergePatterns)
	ON_COMMAND(ID_PATTERNCOPY,					&COrderList::OnPatternCopy)
	ON_COMMAND(ID_PATTERNPASTE,					&COrderList::OnPatternPaste)
	ON_COMMAND(ID_SETRESTARTPOS,				&COrderList::OnSetRestartPos)
	ON_COMMAND(ID_ORDERLIST_LOCKPLAYBACK,		&COrderList::OnLockPlayback)
	ON_COMMAND(ID_ORDERLIST_UNLOCKPLAYBACK,		&COrderList::OnUnlockPlayback)
	ON_COMMAND_RANGE(ID_SEQUENCE_ITEM, ID_SEQUENCE_ITEM + kMaxSequenceActions - 1, &COrderList::OnSelectSequence)
	ON_MESSAGE(WM_MOD_DRAGONDROPPING,			&COrderList::OnDragonDropping)
	ON_MESSAGE(WM_HELPHITTEST,					&COrderList::OnHelpHitTest)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,				&COrderList::OnCustomKeyMsg)
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0,				&COrderList::OnToolTipText)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


COrderList::COrderList(CCtrlPatterns &parent, CModDoc &document)
    : m_nOrderlistMargins(TrackerSettings::Instance().orderlistMargins)
    , m_modDoc(document)
    , m_pParent(parent)
{
	EnableActiveAccessibility();
}


bool COrderList::EnsureEditable(ORDERINDEX ord)
{
	auto &sndFile = m_modDoc.GetSoundFile();
	if(ord >= Order().size())
	{
		if(ord < sndFile.GetModSpecifications().ordersMax)
		{
			try
			{
				Order().resize(ord + 1);
			} catch(mpt::out_of_memory e)
			{
				mpt::delete_out_of_memory(e);
				return false;
			}
		} else
		{
			return false;
		}
	}
	return true;
}


ModSequence &COrderList::Order() { return m_modDoc.GetSoundFile().Order(); }
const ModSequence &COrderList::Order() const { return m_modDoc.GetSoundFile().Order(); }


void COrderList::SetScrollPos(int pos)
{
	// Work around 16-bit limitations of WM_HSCROLL
	SCROLLINFO si;
	MemsetZero(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_TRACKPOS;
	GetScrollInfo(SB_HORZ, &si);
	si.nPos = pos;
	SetScrollInfo(SB_HORZ, &si);
}


int COrderList::GetScrollPos(bool getTrackPos)
{
	// Work around 16-bit limitations of WM_HSCROLL
	SCROLLINFO si;
	MemsetZero(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_TRACKPOS;
	GetScrollInfo(SB_HORZ, &si);
	return getTrackPos ? si.nTrackPos : si.nPos;
}


bool COrderList::IsOrderInMargins(int order, int startOrder)
{
	const ORDERINDEX nMargins = GetMargins();
	return ((startOrder != 0 && order - startOrder < nMargins) ||
		order - startOrder >= GetLength() - nMargins);
}


void COrderList::EnsureVisible(ORDERINDEX order)
{
	// nothing needs to be done
	if(!IsOrderInMargins(order, m_nXScroll) || order == ORDERINDEX_INVALID)
		return;

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


bool COrderList::IsPlaying() const
{
	return (CMainFrame::GetMainFrame()->GetModPlaying() == &m_modDoc);
}


ORDERINDEX COrderList::GetOrderFromPoint(const CRect &rect, const CPoint &pt) const
{
	if(m_cxFont)
		return static_cast<ORDERINDEX>(m_nXScroll + (pt.x - rect.left) / m_cxFont);
	return 0;
}


BOOL COrderList::Init(const CRect &rect, HFONT hFont)
{
	CreateEx(WS_EX_STATICEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, &m_pParent, IDC_ORDERLIST);
	m_hFont = hFont;
	SendMessage(WM_SETFONT, (WPARAM)m_hFont);
	SetScrollPos(0);
	EnableScrollBarCtrl(SB_HORZ, TRUE);
	SetCurSel(0);
	EnableToolTips();
	return TRUE;
}


void COrderList::UpdateScrollInfo()
{
	CRect rcClient;

	GetClientRect(&rcClient);
	if((m_cxFont > 0) && (rcClient.right > 0))
	{
		CRect rect;
		SCROLLINFO info;
		UINT nPage;

		int nMax = Order().GetLengthTailTrimmed();

		GetScrollInfo(SB_HORZ, &info, SIF_PAGE | SIF_RANGE);
		info.fMask = SIF_PAGE | SIF_RANGE;
		info.nMin = 0;
		nPage = rcClient.right / m_cxFont;
		if(nMax <= (int)nPage)
			nMax = nPage + 1;
		if((nMax != info.nMax) || (nPage != info.nPage))
		{
			info.nPage = nPage;
			info.nMax = nMax;
			SetScrollInfo(SB_HORZ, &info, TRUE);
		}
	}
}


int COrderList::GetFontWidth()
{
	if((m_cxFont <= 0) && (m_hWnd) && (m_hFont))
	{
		CClientDC dc(this);
		HGDIOBJ oldfont = dc.SelectObject(m_hFont);
		CSize sz = dc.GetTextExtent(_T("000+"), 4);
		if(oldfont)
			dc.SelectObject(oldfont);
		return sz.cx;
	}
	return m_cxFont;
}


void COrderList::InvalidateSelection()
{
	ORDERINDEX ordLo = m_nScrollPos, count = 1;
	static ORDERINDEX m_nScrollPos2Old = m_nScrollPos2nd;
	if(m_nScrollPos2Old != ORDERINDEX_INVALID)
	{
		// there were multiple orders selected - remove them all
		ORDERINDEX ordHi = m_nScrollPos;
		if(m_nScrollPos2Old < m_nScrollPos)
			ordLo = m_nScrollPos2Old;
		else
			ordHi = m_nScrollPos2Old;
		count = ordHi - ordLo + 1;
	}
	m_nScrollPos2Old = m_nScrollPos2nd;
	CRect rcClient, rect;
	GetClientRect(&rcClient);
	rect.left = rcClient.left + (ordLo - m_nXScroll) * m_cxFont;
	rect.top = rcClient.top;
	rect.right = rect.left + m_cxFont * count;
	rect.bottom = rcClient.bottom;
	rect &= rcClient;
	if(rect.right > rect.left)
		InvalidateRect(rect, FALSE);
	if(m_playPos != ORDERINDEX_INVALID)
	{
		rect.left = rcClient.left + (m_playPos - m_nXScroll) * m_cxFont;
		rect.top = rcClient.top;
		rect.right = rect.left + m_cxFont;
		rect &= rcClient;
		if(rect.right > rect.left)
			InvalidateRect(rect, FALSE);
		m_playPos = ORDERINDEX_INVALID;
	}
}


ORDERINDEX COrderList::GetLength()
{
	CRect rcClient;
	GetClientRect(&rcClient);
	if(m_cxFont > 0)
		return static_cast<ORDERINDEX>(rcClient.right / m_cxFont);
	else
	{
		const int fontWidth = GetFontWidth();
		return (fontWidth > 0) ? static_cast<ORDERINDEX>(rcClient.right / fontWidth) : 0;
	}
}


OrdSelection COrderList::GetCurSel(bool ignoreSelection) const
{
	// returns the currently selected order(s)
	OrdSelection result;
	result.firstOrd = result.lastOrd = m_nScrollPos;
	// ignoreSelection: true if only first selection marker is important.
	if(!ignoreSelection && m_nScrollPos2nd != ORDERINDEX_INVALID)
	{
		if(m_nScrollPos2nd < m_nScrollPos)  // ord2 < ord1
			result.firstOrd = m_nScrollPos2nd;
		else
			result.lastOrd = m_nScrollPos2nd;
	}
	ORDERINDEX lastIndex = std::max(Order().GetLengthTailTrimmed(), m_modDoc.GetSoundFile().GetModSpecifications().ordersMax) - 1u;
	LimitMax(result.firstOrd, lastIndex);
	LimitMax(result.lastOrd, lastIndex);
	return result;
}


void COrderList::SetSelection(ORDERINDEX firstOrd, ORDERINDEX lastOrd)
{
	SetCurSel(firstOrd, true, false, true);
	SetCurSel(lastOrd != ORDERINDEX_INVALID ? lastOrd : firstOrd, false, true, true);
}


bool COrderList::SetCurSel(ORDERINDEX sel, bool setPlayPos, bool shiftClick, bool ignoreCurSel)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	ORDERINDEX &ord = shiftClick ? m_nScrollPos2nd : m_nScrollPos;
	const ORDERINDEX lastIndex = std::max(Order().GetLength(), sndFile.GetModSpecifications().ordersMax) - 1u;

	if((sel < 0) || (sel > lastIndex) || (!m_pParent) || (!pMainFrm))
		return false;
	if(!ignoreCurSel && sel == ord && (sel == sndFile.m_PlayState.m_nCurrentOrder))
		return true;
	const ORDERINDEX shownLength = GetLength();
	InvalidateSelection();
	ord = sel;
	if(!EnsureEditable(ord))
		return false;

	if(!m_bScrolling)
	{
		const ORDERINDEX margins = GetMargins(GetMarginsMax(shownLength));
		if(ord < m_nXScroll + margins)
		{
			// Must move first shown sequence item to left in order to show the new active order.
			m_nXScroll = (ord > margins) ? (ord - margins) : 0;
			SetScrollPos(m_nXScroll);
			Invalidate(FALSE);
		} else
		{
			ORDERINDEX maxsel = shownLength;
			if(maxsel)
				maxsel--;
			if(ord - m_nXScroll >= maxsel - margins)
			{
				// Must move first shown sequence item to right in order to show the new active order.
				m_nXScroll = ord - (maxsel - margins);
				SetScrollPos(m_nXScroll);
				Invalidate(FALSE);
			}
		}
	}
	InvalidateSelection();
	PATTERNINDEX n = Order()[m_nScrollPos];
	if(setPlayPos && !shiftClick && sndFile.Patterns.IsValidPat(n))
	{
		CriticalSection cs;
		const bool isPlaying = IsPlaying();
		bool changedPos = false;

		if(isPlaying && sndFile.m_SongFlags[SONG_PATTERNLOOP])
		{
			pMainFrm->ResetNotificationBuffer();

			// Update channel parameters and play time
			m_modDoc.SetElapsedTime(m_nScrollPos, 0, !sndFile.m_SongFlags[SONG_PAUSED | SONG_STEP]);

			changedPos = true;
		} else if(m_pParent.GetFollowSong())
		{
			FlagSet<SongFlags> pausedFlags = sndFile.m_SongFlags & (SONG_PAUSED | SONG_STEP | SONG_PATTERNLOOP);
			// Update channel parameters and play time
			sndFile.SetCurrentOrder(m_nScrollPos);
			m_modDoc.SetElapsedTime(m_nScrollPos, 0, !sndFile.m_SongFlags[SONG_PAUSED | SONG_STEP]);
			sndFile.m_SongFlags.set(pausedFlags);

			if(isPlaying)
				pMainFrm->ResetNotificationBuffer();
			changedPos = true;
		}

		if(changedPos && Order().IsPositionLocked(m_nScrollPos))
		{
			// Users wants to go somewhere else, so let them do that.
			OnUnlockPlayback();
		}

		m_pParent.SetCurrentPattern(n);
	}
	UpdateInfoText();
	if(m_nScrollPos == m_nScrollPos2nd)
		m_nScrollPos2nd = ORDERINDEX_INVALID;
	return true;
}


PATTERNINDEX COrderList::GetCurrentPattern() const
{
	const ModSequence &order = Order();
	if(m_nScrollPos < order.size())
	{
		return order[m_nScrollPos];
	}
	return 0;
}


BOOL COrderList::PreTranslateMessage(MSG *pMsg)
{
	//handle Patterns View context keys that we want to take effect in the orderlist.
	if((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) || 
	   (pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN))
	{
		CInputHandler *ih = CMainFrame::GetInputHandler();

		//Translate message manually
		UINT nChar = (UINT)pMsg->wParam;
		UINT nRepCnt = LOWORD(pMsg->lParam);
		UINT nFlags = HIWORD(pMsg->lParam);
		KeyEventType kT = ih->GetKeyEventType(nFlags);

		if(ih->KeyEvent(kCtxCtrlOrderlist, nChar, nRepCnt, nFlags, kT) != kcNull)
			return true;  // Mapped to a command, no need to pass message on.

		//HACK: masquerade as kCtxViewPatternsNote context until we implement appropriate
		//      command propagation to kCtxCtrlOrderlist context.

		if(ih->KeyEvent(kCtxViewPatternsNote, nChar, nRepCnt, nFlags, kT) != kcNull)
			return true;  // Mapped to a command, no need to pass message on.
	}

	return CWnd::PreTranslateMessage(pMsg);
}


LRESULT COrderList::OnCustomKeyMsg(WPARAM wParam, LPARAM lParam)
{
	bool isPlaying = IsPlaying();
	switch(wParam)
	{
	case kcEditCopy:
		OnEditCopy(); return wParam;
	case kcEditCut:
		OnEditCut(); return wParam;
	case kcEditPaste:
		OnPatternPaste(); return wParam;
	case kcOrderlistEditCopyOrders:
		OnEditCopyOrders(); return wParam;

	// Orderlist navigation
	case kcOrderlistNavigateLeftSelect:
	case kcOrderlistNavigateLeft:
		SetCurSelTo2ndSel(wParam == kcOrderlistNavigateLeftSelect); SetCurSel(m_nScrollPos - 1, wParam == kcOrderlistNavigateLeft || !isPlaying); return wParam;
	case kcOrderlistNavigateRightSelect:
	case kcOrderlistNavigateRight:
		SetCurSelTo2ndSel(wParam == kcOrderlistNavigateRightSelect); SetCurSel(m_nScrollPos + 1, wParam == kcOrderlistNavigateRight || !isPlaying); return wParam;
	case kcOrderlistNavigateFirstSelect:
	case kcOrderlistNavigateFirst:
		SetCurSelTo2ndSel(wParam == kcOrderlistNavigateFirstSelect); SetCurSel(0, wParam == kcOrderlistNavigateFirst || !isPlaying); return wParam;
	case kcEditSelectAll:
		SetCurSel(0, !isPlaying);
		[[fallthrough]];
	case kcOrderlistNavigateLastSelect:
	case kcOrderlistNavigateLast:
		{
			SetCurSelTo2ndSel(wParam == kcOrderlistNavigateLastSelect || wParam == kcEditSelectAll);
			ORDERINDEX nLast = Order().GetLengthTailTrimmed();
			if(nLast > 0) nLast--;
			SetCurSel(nLast, wParam == kcOrderlistNavigateLast || !isPlaying);
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
		OnLButtonDblClk(0, CPoint(0, 0)); OnSwitchToView(); return wParam;

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
		EnterPatternNum(static_cast<UINT>(wParam) - kcOrderlistPat0); return wParam;
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
		return m_pParent.SendMessage(WM_MOD_KEYCOMMAND, wParam, lParam);

	case kcOrderlistLockPlayback:
		OnLockPlayback(); return wParam;
	case kcOrderlistUnlockPlayback:
		OnUnlockPlayback(); return wParam;

	case kcDuplicatePattern:
		OnDuplicatePattern(); return wParam;
	case kcMergePatterns:
		OnMergePatterns(); return wParam;
	case kcNewPattern:
		OnCreateNewPattern(); return wParam;
	}

	return kcNull;
}


// Helper function to enter pattern index into the orderlist.
// Call with param 0...9 (enter digit), 10 (decrease) or 11 (increase).
void COrderList::EnterPatternNum(int enterNum)
{
	CSoundFile &sndFile = m_modDoc.GetSoundFile();

	if(!EnsureEditable(m_nScrollPos))
		return;

	PATTERNINDEX curIndex = Order()[m_nScrollPos];
	const PATTERNINDEX maxIndex = std::max(PATTERNINDEX(1), sndFile.Patterns.GetNumPatterns()) - 1;
	const PATTERNINDEX firstInvalid = sndFile.GetModSpecifications().hasIgnoreIndex ? sndFile.Order.GetIgnoreIndex() : sndFile.Order.GetInvalidPatIndex();

	if(enterNum >= 0 && enterNum <= 9)  // enter 0...9
	{
		if(curIndex >= sndFile.Patterns.Size())
			curIndex = 0;

		curIndex = curIndex * 10 + static_cast<PATTERNINDEX>(enterNum);
		static_assert(MAX_PATTERNS < 10000);
		if((curIndex >= 1000) && (curIndex > maxIndex)) curIndex %= 1000;
		if((curIndex >= 100) && (curIndex > maxIndex)) curIndex %= 100;
		if((curIndex >= 10) && (curIndex > maxIndex)) curIndex %= 10;
	} else if(enterNum == 10) // decrease pattern index
	{
		if(curIndex == 0)
		{
			curIndex = sndFile.Order.GetInvalidPatIndex();
		} else if(curIndex > maxIndex && curIndex <= firstInvalid)
		{
			curIndex = maxIndex;
		} else
		{
			do
			{
				curIndex--;
			} while(curIndex > 0 && curIndex < firstInvalid && !sndFile.Patterns.IsValidPat(curIndex));
		}
	} else if(enterNum == 11)  // increase pattern index
	{
		if(curIndex >= sndFile.Order.GetInvalidPatIndex())
		{
			curIndex = 0;
		} else if(curIndex >= maxIndex && curIndex < firstInvalid)
		{
			curIndex = firstInvalid;
		} else
		{
			do
			{
				curIndex++;
			} while(curIndex <= maxIndex && !sndFile.Patterns.IsValidPat(curIndex));
		}
	} else if(enterNum == 12)  // ignore index (+++)
	{
		if(sndFile.GetModSpecifications().hasIgnoreIndex)
		{
			curIndex = sndFile.Order.GetIgnoreIndex();
		}
	} else if(enterNum == 13)  // invalid index (---)
	{
		curIndex = sndFile.Order.GetInvalidPatIndex();
	}
	// apply
	if(curIndex != Order()[m_nScrollPos])
	{
		Order()[m_nScrollPos] = curIndex;
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
		InvalidateSelection();
	}
}


void COrderList::OnEditCut()
{
	OnEditCopy();
	OnDeleteOrder();
}


void COrderList::OnCopy(bool onlyOrders)
{
	const OrdSelection ordsel = GetCurSel();
	BeginWaitCursor();
	PatternClipboard::Copy(m_modDoc.GetSoundFile(), ordsel.firstOrd, ordsel.lastOrd, onlyOrders);
	PatternClipboardDialog::UpdateList();
	EndWaitCursor();
}


void COrderList::UpdateView(UpdateHint hint, CObject *pObj)
{
	if(pObj != this && hint.ToType<SequenceHint>().GetType()[HINT_MODTYPE | HINT_MODSEQUENCE])
	{
		Invalidate(FALSE);
		UpdateInfoText();
	}
	if(hint.GetType()[HINT_MPTOPTIONS])
	{
		m_nOrderlistMargins = TrackerSettings::Instance().orderlistMargins;
	}
}


void COrderList::OnSwitchToView()
{
	m_pParent.PostViewMessage(VIEWMSG_SETFOCUS);
}


void COrderList::UpdateInfoText()
{
	if(::GetFocus() != m_hWnd)
		return;

	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	const auto &order = Order();

	const ORDERINDEX length = order.GetLengthTailTrimmed();
	CString s;
	if(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_HEXDISPLAY)
		s.Format(_T("Position %02Xh of %02Xh"), m_nScrollPos, length);
	else
		s.Format(_T("Position %u of %u (%02Xh of %02Xh)"), m_nScrollPos, length, m_nScrollPos, length);

	if(order.IsValidPat(m_nScrollPos))
	{
		if(const auto patName = sndFile.Patterns[order[m_nScrollPos]].GetName(); !patName.empty())
			s += _T(": ") + mpt::ToCString(sndFile.GetCharsetInternal(), patName);
	}
	CMainFrame::GetMainFrame()->SetInfoText(s);

	CMainFrame::GetMainFrame()->NotifyAccessibilityUpdate(*this);
}


// Accessible description for screen readers
HRESULT COrderList::get_accName(VARIANT, BSTR *pszName)
{
	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	const auto &order = Order();

	CString s;
	const bool singleSel = m_nScrollPos2nd == ORDERINDEX_INVALID || m_nScrollPos2nd == m_nScrollPos;
	const auto firstOrd = singleSel ? m_nScrollPos : std::min(m_nScrollPos, m_nScrollPos2nd), lastOrd = singleSel ? m_nScrollPos : std::max(m_nScrollPos, m_nScrollPos2nd);
	if(singleSel)
		s = MPT_CFORMAT("Order {}, ")(m_nScrollPos);
	else
		s = MPT_CFORMAT("Order selection {} to {}: ")(firstOrd, lastOrd);
	bool first = true;
	for(ORDERINDEX o = firstOrd; o <= lastOrd; o++)
	{
		if(!first)
			s += _T(", ");
		first = false;
		PATTERNINDEX pat = order[o];
		if(pat == ModSequence::GetIgnoreIndex())
			s += _T(" Skip");
		else if(pat == ModSequence::GetInvalidPatIndex())
			s += _T(" Stop");
		else
			s += MPT_CFORMAT("Pattern {}")(pat);
		if(sndFile.Patterns.IsValidPat(pat))
		{
			if(const auto patName = sndFile.Patterns[pat].GetName(); !patName.empty())
				s += _T(" (") + mpt::ToCString(sndFile.GetCharsetInternal(), patName) + _T(")");
		}
	}
	
	*pszName = s.AllocSysString();
	return S_OK;
}


/////////////////////////////////////////////////////////////////
// COrderList messages

void COrderList::OnPaint()
{
	TCHAR s[64];
	CPaintDC dc(this);
	HGDIOBJ oldfont = dc.SelectObject(m_hFont);
	HGDIOBJ oldpen = dc.SelectStockObject(DC_PEN);
	const auto separatorColor = GetSysColor(COLOR_WINDOW) ^ 0x808080;
	const auto colorText = GetSysColor(COLOR_WINDOWTEXT), colorInvalid = GetSysColor(COLOR_GRAYTEXT), colorTextSel = GetSysColor(COLOR_HIGHLIGHTTEXT);
	const auto windowBrush = GetSysColorBrush(COLOR_WINDOW), highlightBrush = GetSysColorBrush(COLOR_HIGHLIGHT), faceBrush = GetSysColorBrush(COLOR_BTNFACE);

	SetDCPenColor(dc, separatorColor);

	// First time?
	if(m_cxFont <= 0 || m_cyFont <= 0)
	{
		CSize sz = dc.GetTextExtent(_T("000+"), 4);
		m_cxFont = sz.cx;
		m_cyFont = sz.cy;
	}

	if(m_cxFont > 0 && m_cyFont > 0)
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		CRect rect = rcClient;

		UpdateScrollInfo();
		dc.SetBkMode(TRANSPARENT);
		const OrdSelection selection = GetCurSel();

		const int lineWidth1 = Util::ScalePixels(1, m_hWnd);
		const int lineWidth2 = Util::ScalePixels(2, m_hWnd);
		const bool isFocussed = (::GetFocus() == m_hWnd);

		const auto &order = Order();
		CSoundFile &sndFile = m_modDoc.GetSoundFile();
		ORDERINDEX maxEntries = sndFile.GetModSpecifications().ordersMax;
		if(order.size() > maxEntries)
		{
			// Only computed if potentially needed.
			maxEntries = std::max(maxEntries, order.GetLengthTailTrimmed());
		}

		// Scrolling the shown orders(the showns rectangles)?
		for(ORDERINDEX ord = m_nXScroll; rect.left < rcClient.right; ord++, rect.left += m_cxFont)
		{
			dc.SetTextColor(colorText);
			const bool inSelection = (ord >= selection.firstOrd && ord <= selection.lastOrd);
			const bool highLight = (isFocussed && inSelection);
			if((rect.right = rect.left + m_cxFont) > rcClient.right)
				rect.right = rcClient.right;
			rect.right--;

			HBRUSH background;
			if(highLight)
				background = highlightBrush;  // Currently selected order item
			else if(order.IsPositionLocked(ord))
				background = faceBrush;  // "Playback lock" indicator - grey out all order items which aren't played.
			else
				background = windowBrush;  // Normal, unselected item.
			::FillRect(dc, &rect, background);

			// Drawing the shown pattern-indicator or drag position.
			if(ord == (m_bDragging ? m_nDropPos : m_nScrollPos))
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			MoveToEx(dc, rect.right, rect.top, NULL);
			LineTo(dc, rect.right, rect.bottom);

			// Drawing the 'ctrl-transition' indicator
			if(ord == sndFile.m_PlayState.m_nSeqOverride)
			{
				dc.FillSolidRect(CRect{rect.left + 4, rect.bottom - 4 - lineWidth1, rect.right - 4, rect.bottom - 4}, separatorColor);
			}

			// Drawing 'playing'-indicator.
			if(ord == sndFile.GetCurrentOrder() && CMainFrame::GetMainFrame()->IsPlaying())
			{
				dc.FillSolidRect(CRect{rect.left + 4, rect.top + 2, rect.right - 4, rect.top + 2 + lineWidth1}, separatorColor);
				m_playPos = ord;
			}

			// Drawing drop indicator
			if(m_bDragging && ord == m_nDropPos && !inSelection)
			{
				const bool dropLeft = (m_nDropPos < selection.firstOrd) || TrackerSettings::Instance().orderListOldDropBehaviour;
				dc.FillSolidRect(CRect{dropLeft ? (rect.left + 2) : (rect.right - 2 - lineWidth2), rect.top + 2, dropLeft ? (rect.left + 2 + lineWidth2) : (rect.right - 2), rect.bottom - 2}, separatorColor);
			}

			s[0] = _T('\0');
			const PATTERNINDEX pat = (ord < order.size()) ? order[ord] : PATTERNINDEX_INVALID;
			if(ord < maxEntries && (rect.left + m_cxFont - 4) <= rcClient.right)
			{
				if(pat == order.GetInvalidPatIndex())
					_tcscpy(s, _T("---"));
				else if(pat == order.GetIgnoreIndex())
					_tcscpy(s, _T("+++"));
				else
					wsprintf(s, _T("%u"), pat);
			}

			COLORREF textCol;
			if(highLight)
				textCol = colorTextSel;  // Highlighted pattern
			else if(sndFile.Patterns.IsValidPat(pat))
				textCol = colorText;  // Normal pattern
			else
				textCol = colorInvalid;  // Non-existent pattern
			dc.SetTextColor(textCol);
			dc.DrawText(s, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		}
	}
	if(oldpen)
		dc.SelectObject(oldpen);
	if(oldfont)
		dc.SelectObject(oldfont);
}


void COrderList::OnSetFocus(CWnd *pWnd)
{
	CWnd::OnSetFocus(pWnd);
	InvalidateSelection();
	UpdateInfoText();
	CMainFrame::GetMainFrame()->m_pOrderlistHasFocus = this;
}


void COrderList::OnKillFocus(CWnd *pWnd)
{
	CWnd::OnKillFocus(pWnd);
	InvalidateSelection();
	CMainFrame::GetMainFrame()->m_pOrderlistHasFocus = nullptr;
}


void COrderList::OnLButtonDown(UINT nFlags, CPoint pt)
{
	CRect rect;
	GetClientRect(&rect);
	if(pt.y < rect.bottom)
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

			ORDERINDEX ord = GetOrderFromPoint(rect, pt);
			OrdSelection selection = GetCurSel();

			// check if cursor is in selection - if it is, only react on MouseUp as the user might want to drag those orders
			if(m_nScrollPos2nd == ORDERINDEX_INVALID || ord < selection.firstOrd || ord > selection.lastOrd)
			{
				m_nScrollPos2nd = ORDERINDEX_INVALID;
				SetCurSel(ord, true, IsSelectionKeyPressed());
			}
			m_bDragging = !IsOrderInMargins(m_nScrollPos, oldXScroll) || !IsOrderInMargins(m_nScrollPos2nd, oldXScroll);

			m_nMouseDownPos = ord;
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
{
	CRect rect;
	GetClientRect(&rect);

	// Copy or move orders?
	const bool copyOrders = IsSelectionKeyPressed();

	if(m_bDragging)
	{
		m_bDragging = false;
		ReleaseCapture();
		if(rect.PtInRect(pt))
		{
			ORDERINDEX n = GetOrderFromPoint(rect, pt);
			const OrdSelection selection = GetCurSel();
			if(n != ORDERINDEX_INVALID && n == m_nDropPos && (n < selection.firstOrd || n > selection.lastOrd))
			{
				const bool multiSelection = (selection.firstOrd != selection.lastOrd);
				const bool moveBack = m_nDropPos < m_nDragOrder;
				ORDERINDEX moveCount = (selection.lastOrd - selection.firstOrd), movePos = selection.firstOrd;

				if(!moveBack && !TrackerSettings::Instance().orderListOldDropBehaviour)
					m_nDropPos++;

				bool modified = false;
				for(int i = 0; i <= moveCount; i++)
				{
					if(!m_modDoc.MoveOrder(movePos, m_nDropPos, true, copyOrders))
						break;
					modified = true;
					if(moveBack != copyOrders && multiSelection)
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
				if(modified)
					m_modDoc.SetModified();
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
		Invalidate(FALSE);
	} else
	{
		CWnd::OnLButtonUp(nFlags, pt);
	}
}


void COrderList::OnMouseMove(UINT nFlags, CPoint pt)
{
	if((m_bDragging) && (m_cxFont))
	{
		CRect rect;

		GetClientRect(&rect);
		ORDERINDEX n = ORDERINDEX_INVALID;
		if(rect.PtInRect(pt))
		{
			CSoundFile &sndFile = m_modDoc.GetSoundFile();
			n = GetOrderFromPoint(rect, pt);
			if(n >= Order().size() && n >= sndFile.GetModSpecifications().ordersMax)
				n = ORDERINDEX_INVALID;
		}
		if(n != m_nDropPos)
		{
			if(n != ORDERINDEX_INVALID)
			{
				m_nMouseDownPos = ORDERINDEX_INVALID;
				m_nDropPos = n;
				Invalidate(FALSE);
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
{
	SelectSequence(static_cast<SEQUENCEINDEX>(nid - ID_SEQUENCE_ITEM));
}


void COrderList::OnRButtonDown(UINT nFlags, CPoint pt)
{
	CRect rect;
	GetClientRect(&rect);
	if(m_bDragging)
	{
		m_nDropPos = ORDERINDEX_INVALID;
		OnLButtonUp(nFlags, pt);
	}
	if(pt.y >= rect.bottom)
		return;

	bool multiSelection = (m_nScrollPos2nd != ORDERINDEX_INVALID);

	if(!multiSelection)
		SetCurSel(GetOrderFromPoint(rect, pt), false, false, false);
	SetFocus();
	HMENU hMenu = ::CreatePopupMenu();
	if(!hMenu)
		return;

	CSoundFile &sndFile = m_modDoc.GetSoundFile();

	// Check if at least one pattern in the current selection exists
	bool patExists = false;
	OrdSelection selection = GetCurSel();
	LimitMax(selection.lastOrd, Order().GetLastIndex());
	for(ORDERINDEX ord = selection.firstOrd; ord <= selection.lastOrd && !patExists; ord++)
	{
		patExists = Order().IsValidPat(ord);
	}

	const DWORD greyed = patExists ? 0 : MF_GRAYED;

	CInputHandler *ih = CMainFrame::GetInputHandler();

	if(multiSelection)
	{
		// Several patterns are selected.
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_INSERT, ih->GetKeyTextFromCommand(kcOrderlistEditInsert, _T("&Insert Patterns")));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_DELETE, ih->GetKeyTextFromCommand(kcOrderlistEditDelete, _T("&Remove Patterns")));
		AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_EDIT_COPY, ih->GetKeyTextFromCommand(kcEditCopy, _T("&Copy Patterns")));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_EDIT_COPY_ORDERS, ih->GetKeyTextFromCommand(kcOrderlistEditCopyOrders, _T("&Copy Orders")));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_EDIT_CUT, ih->GetKeyTextFromCommand(kcEditCut, _T("&C&ut Patterns")));
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERNPASTE, ih->GetKeyTextFromCommand(kcEditPaste, _T("P&aste Patterns")));
		AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
		AppendMenu(hMenu, MF_STRING | greyed, ID_ORDERLIST_COPY, ih->GetKeyTextFromCommand(kcDuplicatePattern, _T("&Duplicate Patterns")));
		AppendMenu(hMenu, MF_STRING | greyed, ID_ORDERLIST_MERGE, ih->GetKeyTextFromCommand(kcMergePatterns, _T("&Merge Patterns")));
	} else
	{
		// Only one pattern is selected
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_INSERT, ih->GetKeyTextFromCommand(kcOrderlistEditInsert, _T("&Insert Pattern")));
		if(sndFile.GetModSpecifications().hasIgnoreIndex)
		{
			AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_INSERT_SEPARATOR, ih->GetKeyTextFromCommand(kcOrderlistPatIgnore, _T("&Insert Separator")));
		}
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_DELETE, ih->GetKeyTextFromCommand(kcOrderlistEditDelete, _T("&Remove Pattern")));
		AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
		AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_NEW, ih->GetKeyTextFromCommand(kcNewPattern, _T("Create &New Pattern")));
		AppendMenu(hMenu, MF_STRING | greyed, ID_ORDERLIST_COPY, ih->GetKeyTextFromCommand(kcDuplicatePattern, _T("&Duplicate Pattern")));
		AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERNCOPY, _T("&Copy Pattern"));
		AppendMenu(hMenu, MF_STRING, ID_PATTERNPASTE, ih->GetKeyTextFromCommand(kcEditPaste, _T("P&aste Pattern")));
		const bool hasPatternProperties = sndFile.GetModSpecifications().patternRowsMin != sndFile.GetModSpecifications().patternRowsMax;
		if(hasPatternProperties || sndFile.GetModSpecifications().hasRestartPos)
		{
			AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
			if(hasPatternProperties)
				AppendMenu(hMenu, MF_STRING | greyed, ID_PATTERN_PROPERTIES, _T("&Pattern Properties..."));
			if(sndFile.GetModSpecifications().hasRestartPos)
				AppendMenu(hMenu, MF_STRING | greyed | ((Order().GetRestartPos() == m_nScrollPos) ? MF_CHECKED : 0), ID_SETRESTARTPOS, _T("R&estart Position"));
		}
		if(sndFile.GetModSpecifications().sequencesMax > 1)
		{
			AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));

			HMENU menuSequence = ::CreatePopupMenu();
			AppendMenu(hMenu, MF_POPUP, (UINT_PTR)menuSequence, _T("&Sequences"));

			const SEQUENCEINDEX numSequences = sndFile.Order.GetNumSequences();
			for(SEQUENCEINDEX i = 0; i < numSequences; i++)
			{
				CString str;
				if(sndFile.Order(i).GetName().empty())
					str = MPT_CFORMAT("Sequence {}")(i + 1);
				else
					str = MPT_CFORMAT("{}: {}")(i + 1, mpt::ToCString(sndFile.Order(i).GetName()));
				const UINT flags = (sndFile.Order.GetCurrentSequenceIndex() == i) ? MF_STRING | MF_CHECKED : MF_STRING;
				AppendMenu(menuSequence, flags, ID_SEQUENCE_ITEM + i, str);
			}
			if(sndFile.Order.GetNumSequences() < sndFile.GetModSpecifications().sequencesMax)
			{
				AppendMenu(menuSequence, MF_STRING, ID_SEQUENCE_ITEM + kDuplicateSequence, _T("&Duplicate current sequence"));
				AppendMenu(menuSequence, MF_STRING, ID_SEQUENCE_ITEM + kAddSequence, _T("&Create empty sequence"));
			}
			if(sndFile.Order.GetNumSequences() > 1)
				AppendMenu(menuSequence, MF_STRING, ID_SEQUENCE_ITEM + kDeleteSequence, _T("D&elete current sequence"));
			else
				AppendMenu(menuSequence, MF_STRING, ID_SEQUENCE_ITEM + kSplitSequence, _T("&Split sub songs into sequences"));
		}
	}
	AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
	AppendMenu(hMenu, ((selection.firstOrd == sndFile.m_lockOrderStart && selection.lastOrd == sndFile.m_lockOrderEnd) ? (MF_STRING | MF_CHECKED) : MF_STRING), ID_ORDERLIST_LOCKPLAYBACK, ih->GetKeyTextFromCommand(kcOrderlistLockPlayback, _T("&Lock Playback to Selection")));
	AppendMenu(hMenu, (sndFile.m_lockOrderStart == ORDERINDEX_INVALID ? (MF_STRING | MF_GRAYED) : MF_STRING), ID_ORDERLIST_UNLOCKPLAYBACK, ih->GetKeyTextFromCommand(kcOrderlistUnlockPlayback, _T("&Unlock Playback")));

	AppendMenu(hMenu, MF_SEPARATOR, NULL, _T(""));
	AppendMenu(hMenu, MF_STRING | greyed, ID_ORDERLIST_RENDER, _T("Render to &Wave"));

	ClientToScreen(&pt);
	::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
	::DestroyMenu(hMenu);
}


void COrderList::OnLButtonDblClk(UINT, CPoint)
{
	auto &sndFile = m_modDoc.GetSoundFile();
	m_nScrollPos2nd = ORDERINDEX_INVALID;
	SetFocus();
	if(!EnsureEditable(m_nScrollPos))
		return;
	PATTERNINDEX pat = Order()[m_nScrollPos];
	if(sndFile.Patterns.IsValidPat(pat))
		m_pParent.SetCurrentPattern(pat);
	else if(pat != sndFile.Order.GetIgnoreIndex())
		OnCreateNewPattern();
}


void COrderList::OnMButtonDown(UINT nFlags, CPoint pt)
{
	MPT_UNREFERENCED_PARAMETER(nFlags);
	QueuePattern(pt);
}


void COrderList::OnHScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar *)
{
	UINT nNewPos = m_nXScroll;
	UINT smin, smax;

	GetScrollRange(SB_HORZ, (LPINT)&smin, (LPINT)&smax);
	m_bScrolling = true;
	switch(nSBCode)
	{
	case SB_LINELEFT:		if (nNewPos) nNewPos--; break;
	case SB_LINERIGHT:		if (nNewPos < smax) nNewPos++; break;
	case SB_PAGELEFT:		if (nNewPos > 4) nNewPos -= 4; else nNewPos = 0; break;
	case SB_PAGERIGHT:		if (nNewPos + 4 < smax) nNewPos += 4; else nNewPos = smax; break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:		nNewPos = GetScrollPos(true); break;
	case SB_LEFT:			nNewPos = 0; break;
	case SB_RIGHT:			nNewPos = smax; break;
	case SB_ENDSCROLL:		m_bScrolling = false; break;
	}
	if (nNewPos > smax) nNewPos = smax;
	if (nNewPos != m_nXScroll)
	{
		m_nXScroll = static_cast<ORDERINDEX>(nNewPos);
		SetScrollPos(m_nXScroll);
		Invalidate(FALSE);
	}
}


void COrderList::OnSize(UINT nType, int cx, int cy)
{
	int nPos;
	int smin, smax;

	CWnd::OnSize(nType, cx, cy);
	UpdateScrollInfo();
	GetScrollRange(SB_HORZ, &smin, &smax);
	nPos = GetScrollPos();
	if(nPos > smax)
		nPos = smax;
	if(m_nXScroll != nPos)
	{
		m_nXScroll = static_cast<ORDERINDEX>(nPos);
		SetScrollPos(m_nXScroll);
		Invalidate(FALSE);
	}
}


void COrderList::OnInsertOrder()
{
	// insert the same order(s) after the currently selected order(s)
	ModSequence &order = Order();

	const OrdSelection selection = GetCurSel();
	const ORDERINDEX insertCount = order.insert(selection.lastOrd + 1, selection.lastOrd - selection.firstOrd + 1);
	if(!insertCount)
		return;

	std::copy(order.begin() + selection.firstOrd, order.begin() + selection.firstOrd + insertCount, order.begin() + selection.lastOrd + 1);

	InsertUpdatePlaystate(selection.firstOrd, selection.lastOrd);

	m_nScrollPos = std::min(ORDERINDEX(selection.lastOrd + 1), order.GetLastIndex());
	if(insertCount > 1)
		m_nScrollPos2nd = std::min(ORDERINDEX(m_nScrollPos + insertCount - 1), order.GetLastIndex());
	else
		m_nScrollPos2nd = ORDERINDEX_INVALID;

	InvalidateSelection();
	EnsureVisible(m_nScrollPos2nd);
	// first inserted order has higher priority than the last one
	EnsureVisible(m_nScrollPos);

	Invalidate(FALSE);
	m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
}


void COrderList::OnInsertSeparatorPattern()
{
	// Insert a separator pattern after the current pattern, don't move order list cursor
	ModSequence &order = Order();

	const OrdSelection selection = GetCurSel(true);
	ORDERINDEX insertPos = selection.firstOrd;

	if(!EnsureEditable(insertPos))
		return;
	if(order[insertPos] != order.GetInvalidPatIndex())
	{
		// If we're not inserting at a stop (---) index, we move on by one position.
		insertPos++;
		order.insert(insertPos, 1, order.GetIgnoreIndex());
	} else
	{
		order[insertPos] = order.GetIgnoreIndex();
	}

	InsertUpdatePlaystate(insertPos, insertPos);

	Invalidate(FALSE);
	m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
}


void COrderList::OnRenderOrder()
{
	OrdSelection selection = GetCurSel();
	m_modDoc.OnFileWaveConvert(selection.firstOrd, selection.lastOrd);
}


void COrderList::OnDeleteOrder()
{
	OrdSelection selection = GetCurSel();
	// remove selection
	m_nScrollPos2nd = ORDERINDEX_INVALID;

	Order().Remove(selection.firstOrd, selection.lastOrd);

	m_modDoc.SetModified();
	Invalidate(FALSE);
	m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);

	DeleteUpdatePlaystate(selection.firstOrd, selection.lastOrd);

	SetCurSel(selection.firstOrd, true, false, true);
}


void COrderList::OnPatternProperties()
{
	ModSequence &order = Order();
	const auto ord = GetCurSel(true).firstOrd;
	if(order.IsValidPat(ord))
		m_pParent.PostViewMessage(VIEWMSG_PATTERNPROPERTIES, order[ord]);
}


void COrderList::OnPlayerPlay()
{
	m_pParent.PostMessage(WM_COMMAND, ID_PLAYER_PLAY);
}


void COrderList::OnPlayerPause()
{
	m_pParent.PostMessage(WM_COMMAND, ID_PLAYER_PAUSE);
}


void COrderList::OnPlayerPlayFromStart()
{
	m_pParent.PostMessage(WM_COMMAND, ID_PLAYER_PLAYFROMSTART);
}


void COrderList::OnPatternPlayFromStart()
{
	m_pParent.PostMessage(WM_COMMAND, IDC_PATTERN_PLAYFROMSTART);
}


void COrderList::OnCreateNewPattern()
{
	m_pParent.PostMessage(WM_COMMAND, ID_ORDERLIST_NEW);
}


void COrderList::OnDuplicatePattern()
{
	m_pParent.PostMessage(WM_COMMAND, ID_ORDERLIST_COPY);
}


void COrderList::OnMergePatterns()
{
	m_pParent.PostMessage(WM_COMMAND, ID_ORDERLIST_MERGE);
}


void COrderList::OnPatternCopy()
{
	m_pParent.PostMessage(WM_COMMAND, ID_PATTERNCOPY);
}


void COrderList::OnPatternPaste()
{
	m_pParent.PostMessage(WM_COMMAND, ID_PATTERNPASTE);
}


void COrderList::OnSetRestartPos()
{
	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	bool modified = false;
	if(m_nScrollPos == Order().GetRestartPos())
	{
		// Unset position
		modified = (m_nScrollPos != 0);
		Order().SetRestartPos(0);
	} else if(sndFile.GetModSpecifications().hasRestartPos)
	{
		// Set new position
		modified = true;
		Order().SetRestartPos(m_nScrollPos);
	}
	if(modified)
	{
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, SequenceHint().RestartPos(), this);
	}
}


LRESULT COrderList::OnHelpHitTest(WPARAM, LPARAM)
{
	return HID_BASE_COMMAND + IDC_ORDERLIST;
}


LRESULT COrderList::OnDragonDropping(WPARAM doDrop, LPARAM lParam)
{
	const DRAGONDROP *pDropInfo = (const DRAGONDROP *)lParam;
	CPoint pt;

	if((!pDropInfo) || (&m_modDoc.GetSoundFile() != pDropInfo->sndFile) || (!m_cxFont))
		return FALSE;
	BOOL canDrop = FALSE;
	switch(pDropInfo->dropType)
	{
	case DRAGONDROP_ORDER:
		if(pDropInfo->dropItem >= Order().size())
			break;
	case DRAGONDROP_PATTERN:
		canDrop = TRUE;
		break;
	}
	if(!canDrop || !doDrop)
		return canDrop;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	if(pt.x < 0)
		pt.x = 0;
	ORDERINDEX posDest = static_cast<ORDERINDEX>(m_nXScroll + (pt.x / m_cxFont));
	if(posDest >= Order().size())
		return FALSE;
	switch(pDropInfo->dropType)
	{
	case DRAGONDROP_PATTERN:
		Order()[posDest] = static_cast<PATTERNINDEX>(pDropInfo->dropItem);
		break;

	case DRAGONDROP_ORDER:
		Order()[posDest] = Order()[pDropInfo->dropItem];
		break;
	}
	if(canDrop)
	{
		Invalidate(FALSE);
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
		SetCurSel(posDest, true);
	}
	return canDrop;
}


ORDERINDEX COrderList::SetMargins(int i)
{
	m_nOrderlistMargins = i;
	return GetMargins();
}


void COrderList::SelectSequence(const SEQUENCEINDEX seq)
{
	CriticalSection cs;

	CMainFrame::GetMainFrame()->ResetNotificationBuffer();
	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	const bool editSequence = seq >= sndFile.Order.GetNumSequences();
	if(seq == kSplitSequence)
	{
		if(!sndFile.Order.CanSplitSubsongs())
		{
			Reporting::Information(U_("No sub songs have been found in this sequence."));
			return;
		}
		if(Reporting::Confirm(U_("The order list contains separator items.\nDo you want to split the sequence at the separators into multiple song sequences?")) != cnfYes)
			return;
		if(!sndFile.Order.SplitSubsongsToMultipleSequences())
			return;
	} else if(seq == kDeleteSequence)
	{
		SEQUENCEINDEX curSeq = sndFile.Order.GetCurrentSequenceIndex();
		mpt::ustring str = MPT_UFORMAT("Remove sequence {}: {}?")(curSeq + 1, mpt::ToUnicode(Order().GetName()));
		if(Reporting::Confirm(str) == cnfYes)
			sndFile.Order.RemoveSequence(curSeq);
		else
			return;
	} else if(seq == kAddSequence || seq == kDuplicateSequence)
	{
		const bool duplicate = (seq == kDuplicateSequence);
		const SEQUENCEINDEX newIndex = sndFile.Order.GetCurrentSequenceIndex() + 1u;
		std::vector<SEQUENCEINDEX> newOrder(sndFile.Order.GetNumSequences());
		std::iota(newOrder.begin(), newOrder.end(), SEQUENCEINDEX(0));
		newOrder.insert(newOrder.begin() + newIndex, duplicate ? sndFile.Order.GetCurrentSequenceIndex() : SEQUENCEINDEX_INVALID);
		if(m_modDoc.ReArrangeSequences(newOrder))
		{
			sndFile.Order.SetSequence(newIndex);
			if(const auto name = sndFile.Order().GetName(); duplicate && !name.empty())
				sndFile.Order().SetName(name + U_(" (Copy)"));
			m_modDoc.UpdateAllViews(nullptr, SequenceHint(SEQUENCEINDEX_INVALID).Names().Data());
		}
	} else if(seq == sndFile.Order.GetCurrentSequenceIndex())
		return;
	else if(seq < sndFile.Order.GetNumSequences())
		sndFile.Order.SetSequence(seq);
	ORDERINDEX posCandidate = Order().GetLengthTailTrimmed() - 1;
	SetCurSel(std::min(m_nScrollPos, posCandidate), true, false, true);
	m_pParent.SetCurrentPattern(Order()[m_nScrollPos]);

	UpdateScrollInfo();
	// This won't make sense anymore in the new sequence.
	OnUnlockPlayback();

	cs.Leave();

	if(editSequence)
		m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), nullptr);
}


void COrderList::QueuePattern(CPoint pt)
{
	CRect rect;
	GetClientRect(&rect);

	if(!rect.PtInRect(pt))
		return;
	CSoundFile &sndFile = m_modDoc.GetSoundFile();

	const PATTERNINDEX ignoreIndex = sndFile.Order.GetIgnoreIndex();
	const PATTERNINDEX stopIndex = sndFile.Order.GetInvalidPatIndex();
	const ORDERINDEX length = Order().GetLength();
	ORDERINDEX order = GetOrderFromPoint(rect, pt);

	// If this is not a playable order item, find the next valid item.
	while(order < length && (Order()[order] == ignoreIndex || Order()[order] == stopIndex))
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
			if(Order().IsPositionLocked(order))
			{
				// Users wants to go somewhere else, so let them do that.
				OnUnlockPlayback();
			}

			sndFile.m_PlayState.m_nSeqOverride = order;
		}
		Invalidate(FALSE);
	}
}


void COrderList::OnLockPlayback()
{
	CSoundFile &sndFile = m_modDoc.GetSoundFile();

	OrdSelection selection = GetCurSel();
	if(selection.firstOrd == sndFile.m_lockOrderStart && selection.lastOrd == sndFile.m_lockOrderEnd)
	{
		OnUnlockPlayback();
	} else
	{
		sndFile.m_lockOrderStart = selection.firstOrd;
		sndFile.m_lockOrderEnd = selection.lastOrd;
		Invalidate(FALSE);
	}
}


void COrderList::OnUnlockPlayback()
{
	CSoundFile &sndFile = m_modDoc.GetSoundFile();
	sndFile.m_lockOrderStart = sndFile.m_lockOrderEnd = ORDERINDEX_INVALID;
	Invalidate(FALSE);
}


void COrderList::InsertUpdatePlaystate(ORDERINDEX first, ORDERINDEX last)
{
	auto &sndFile = m_modDoc.GetSoundFile();
	Util::InsertItem(first, last, sndFile.m_PlayState.m_nNextOrder);
	if(sndFile.m_PlayState.m_nSeqOverride != ORDERINDEX_INVALID)
		Util::InsertItem(first, last, sndFile.m_PlayState.m_nSeqOverride);
	// Adjust order lock position
	if(sndFile.m_lockOrderStart != ORDERINDEX_INVALID)
		Util::InsertRange(first, last, sndFile.m_lockOrderStart, sndFile.m_lockOrderEnd);
}


void COrderList::DeleteUpdatePlaystate(ORDERINDEX first, ORDERINDEX last)
{
	auto &sndFile = m_modDoc.GetSoundFile();
	Util::DeleteItem(first, last, sndFile.m_PlayState.m_nNextOrder);
	if(sndFile.m_PlayState.m_nSeqOverride != ORDERINDEX_INVALID)
		Util::DeleteItem(first, last, sndFile.m_PlayState.m_nSeqOverride);
	// Adjust order lock position
	if(sndFile.m_lockOrderStart != ORDERINDEX_INVALID)
		Util::DeleteRange(first, last, sndFile.m_lockOrderStart, sndFile.m_lockOrderEnd);
}


INT_PTR COrderList::OnToolHitTest(CPoint point, TOOLINFO *pTI) const
{
	CRect rect;
	GetClientRect(&rect);

	pTI->hwnd = m_hWnd;
	pTI->uId = GetOrderFromPoint(rect, point);
	pTI->rect = rect;
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	return pTI->uId;
}


BOOL COrderList::OnToolTipText(UINT, NMHDR *pNMHDR, LRESULT *)
{
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
	if(!(pTTT->uFlags & TTF_IDISHWND))
	{
		CString text;
		const CSoundFile &sndFile = m_modDoc.GetSoundFile();
		const ModSequence &order = Order();
		const ORDERINDEX ord = static_cast<ORDERINDEX>(pNMHDR->idFrom), ordLen = order.GetLengthTailTrimmed();
		text.Format(_T("Position %u of %u [%02Xh of %02Xh]"), ord, ordLen, ord, ordLen);
		if(order.IsValidPat(ord))
		{
			PATTERNINDEX pat = order[ord];
			const std::string name = sndFile.Patterns[pat].GetName();
			if(!name.empty())
			{
				::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, int32_max);  // Allow multiline tooltip
				text += _T("\r\n") + mpt::ToCString(sndFile.GetCharsetInternal(), name);
			}
		}
		mpt::String::WriteCStringBuf(pTTT->szText) = text;
		return TRUE;
	}
	return FALSE;
}


OPENMPT_NAMESPACE_END
