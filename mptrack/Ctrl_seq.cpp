#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "ctrl_pat.h"
#include "view_pat.h"

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
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_COMMAND(ID_CONTROLTAB,			OnSwitchToView)
	ON_COMMAND(ID_ORDERLIST_INSERT,		OnInsertOrder)
	ON_COMMAND(ID_ORDERLIST_DELETE,		OnDeleteOrder)
	ON_COMMAND(ID_PATTERN_PROPERTIES,	OnPatternProperties)
	ON_COMMAND(ID_PLAYER_PLAY,			OnPlayerPlay)
	ON_COMMAND(ID_PLAYER_PAUSE,			OnPlayerPause)
	ON_COMMAND(ID_PLAYER_PLAYFROMSTART,	OnPlayerPlayFromStart)
	ON_COMMAND(IDC_PATTERN_PLAYFROMSTART,OnPatternPlayFromStart)
	ON_COMMAND(ID_ORDERLIST_NEW,		OnCreateNewPattern)
	ON_COMMAND(ID_ORDERLIST_COPY,		OnDuplicatePattern)
	ON_COMMAND(ID_PATTERNCOPY,			OnPatternCopy)
	ON_COMMAND(ID_PATTERNPASTE,			OnPatternPaste)
	ON_MESSAGE(WM_MOD_DRAGONDROPPING,	OnDragonDropping)
	ON_MESSAGE(WM_HELPHITTEST,			OnHelpHitTest)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


COrderList::COrderList()
//----------------------
{
	m_hFont = NULL;
	m_pParent = NULL;
	m_cxFont = m_cyFont = 0;
	m_pModDoc = NULL;
	m_nScrollPos = m_nXScroll = 0; 
	m_bScrolling = FALSE;
	m_bDragging = FALSE;
	m_bShift = FALSE;
}


BOOL COrderList::Init(const CRect &rect, CCtrlPatterns *pParent, CModDoc *pModDoc, HFONT hFont)
//---------------------------------------------------------------------------------------------
{
	CreateEx(WS_EX_STATICEDGE, NULL, "", WS_CHILD|WS_VISIBLE, rect, pParent, IDC_ORDERLIST);
	m_pParent = pParent;
	m_pModDoc = pModDoc;
	m_hFont = hFont;
	colorText = GetSysColor(COLOR_WINDOWTEXT);
	colorTextSel = GetSysColor(COLOR_HIGHLIGHTTEXT);
	SendMessage(WM_SETFONT, (WPARAM)m_hFont);
	SetScrollPos(SB_HORZ, 0);
	EnableScrollBarCtrl(SB_HORZ, TRUE);
	return TRUE;
}


BOOL COrderList::UpdateScrollInfo()
//---------------------------------
{
	CRect rcClient;

	GetClientRect(&rcClient);
	if ((m_pModDoc) && (m_cxFont > 0) && (rcClient.right > 0))
	{
		CRect rect;
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		SCROLLINFO info;
		UINT nPage;
		int nMax=0;

		while ((nMax < MAX_ORDERS) && (pSndFile->Order[nMax] != 0xFF)) nMax++;
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
	CRect rcClient, rect;
	GetClientRect(&rcClient);
	rect.left = rcClient.left + (m_nScrollPos - m_nXScroll) * m_cxFont;
	rect.top = rcClient.top;
	rect.right = rect.left + m_cxFont;
	rect.bottom = rcClient.bottom;
	if (rect.right > rcClient.right) rect.right = rcClient.right;
	if (rect.left < rcClient.left) rect.left = rcClient.left;
	if (rect.right > rect.left) ::InvalidateRect(m_hWnd, &rect, FALSE);
}


BOOL COrderList::SetCurSel(int sel, BOOL bEdit)
//---------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CRect rcClient;

	if ((sel < 0) || (sel >= MAX_ORDERS) || (!m_pParent) || (!pMainFrm)) return FALSE;
	if (sel == m_nScrollPos) return TRUE;
	GetClientRect(&rcClient);
	InvalidateSelection();
	m_nScrollPos = sel;
	if (!m_bScrolling)
	{
		if ((m_nScrollPos < m_nXScroll) || (!m_cxFont) || (!m_cyFont))
		{
			m_nXScroll = m_nScrollPos;
			SetScrollPos(SB_HORZ, m_nXScroll);
			InvalidateRect(NULL, FALSE);
		} else
		{
			int maxsel = (rcClient.right / m_cxFont);
			if (maxsel) maxsel--;
			if (m_nScrollPos - m_nXScroll >= maxsel)
			{
				m_nXScroll = m_nScrollPos - maxsel;
				SetScrollPos(SB_HORZ, m_nXScroll);
				InvalidateRect(NULL, FALSE);
			}
		}
	}
	InvalidateSelection();
	if ((m_pParent) && (m_pModDoc) && (bEdit))
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		UINT n = pSndFile->Order[m_nScrollPos];
		if ((n < MAX_PATTERNS) && (pSndFile->Patterns[n]))
		{
			BOOL bIsPlaying = (pMainFrm->GetModPlaying() == m_pModDoc);
			if ((bIsPlaying) && (pSndFile->m_dwSongFlags & SONG_PATTERNLOOP))
			{
				BEGIN_CRITICAL();
				pSndFile->m_nPattern = n;
				pSndFile->m_nNextRow = 0;
				END_CRITICAL();
			} else
			if (m_pParent->GetFollowSong())
			{
				BEGIN_CRITICAL();
				DWORD dwPaused = pSndFile->m_dwSongFlags & (SONG_PAUSED|SONG_STEP|SONG_PATTERNLOOP);
				pSndFile->SetCurrentOrder(m_nScrollPos);
				pSndFile->m_dwSongFlags |= dwPaused;
				if (!(dwPaused & SONG_PATTERNLOOP)) pSndFile->GetLength(TRUE);
				if (bIsPlaying) pMainFrm->ResetNotificationBuffer();
				END_CRITICAL();
			}
			m_pParent->SetCurrentPattern(n);
		}
	}
	UpdateInfoText();
	return TRUE;
}


UINT COrderList::GetCurrentPattern() const
//----------------------------------------
{
	if ((m_pModDoc) && (m_nScrollPos < MAX_PATTERNS))
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		return pSndFile->Order[m_nScrollPos];
	}
	return 0;
}


BOOL COrderList::ProcessKeyDown(UINT nChar)
//-----------------------------------------
{
	switch(nChar)
	{
	case VK_UP:
	case VK_LEFT:	SetCurSel(m_nScrollPos-1); break;
	case VK_DOWN:
	case VK_RIGHT:	SetCurSel(m_nScrollPos+1); break;
	case VK_HOME:	SetCurSel(0); break;
	case VK_END:
		if (m_pModDoc)
		{
			CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
			for (int i=0; i<MAX_ORDERS-1; i++) if (pSndFile->Order[i+1] == 0xFF) break;
			SetCurSel(i);
		}
		break;
	case VK_DELETE:	OnDeleteOrder(); break;
	case VK_INSERT: OnInsertOrder(); break;
	case VK_TAB:	OnSwitchToView(); break;
	case VK_RETURN:	OnLButtonDblClk(0, CPoint(0,0)); OnSwitchToView(); break;
	case VK_F5:		OnPlayerPlay(); break;
	case VK_F6:		OnPlayerPlayFromStart(); break;
	case VK_F7:		OnPatternPlayFromStart(); break;
	case VK_ESCAPE:
	case VK_F8:		OnPlayerPause(); break;
	default:
		return FALSE;
	}
	return TRUE;
}


BOOL COrderList::ProcessChar(UINT nChar)
//--------------------------------------
{
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		int ord = pSndFile->Order[m_nScrollPos];
		int maxpat = 0;
		for (int i=0; i<MAX_PATTERNS; i++) if (pSndFile->Patterns[i]) maxpat = i;
		if ((nChar >= '0') && (nChar <= '9'))
		{
			if (ord >= MAX_PATTERNS) ord = 0;
			ord = ord * 10 + (nChar - '0');
			if ((ord >= 100) && (ord > maxpat)) ord %= 100;
			if ((ord >= 10) && (ord > maxpat)) ord %= 10;
		} else
		if (nChar == '+')
		{
			ord++;
			if (ord > 0xFF) ord = 0; else
			if ((ord > maxpat) && (ord < 0xFE)) ord = 0xFE;
		} else
		if (nChar == '-')
		{
			ord--;
			if (ord < 0) ord = 0xFF; else
			if ((ord > maxpat) && (ord < 0xFE)) ord = maxpat;
		}
		if (ord != pSndFile->Order[m_nScrollPos])
		{
			pSndFile->Order[m_nScrollPos] = (BYTE)ord;
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE, this);
			InvalidateSelection();
			return TRUE;
		}
	}
	return FALSE;
}


BOOL COrderList::PreTranslateMessage(MSG *pMsg)
//---------------------------------------------
{
	switch(pMsg->message)
	{
	case WM_KEYUP:
		if ((pMsg->wParam == VK_SHIFT) || (pMsg->wParam == VK_LSHIFT) || (pMsg->wParam == VK_RSHIFT)) m_bShift = FALSE;
		break;
	case WM_KEYDOWN:
		if ((pMsg->wParam == VK_SHIFT) || (pMsg->wParam == VK_LSHIFT) || (pMsg->wParam == VK_RSHIFT)) m_bShift = TRUE;
		if (ProcessKeyDown(pMsg->wParam)) return TRUE;
		break;
	case WM_CHAR:
		if (ProcessChar(pMsg->wParam)) return TRUE;
		break;
	}
	return CWnd::PreTranslateMessage(pMsg);
}


void COrderList::UpdateView(DWORD dwHintMask, CObject *pObj)
//----------------------------------------------------------
{
	if ((pObj != this) && (dwHintMask & HINT_MODSEQUENCE))
	{
		InvalidateRect(NULL, FALSE);
		UpdateInfoText();
	}
}


void COrderList::OnSwitchToView()
//-------------------------------
{
	if (m_pParent) m_pParent->PostViewMessage(VIEWMSG_SETFOCUS);
	m_bShift = FALSE;
}


void COrderList::UpdateInfoText()
//-------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((pMainFrm) && (m_pModDoc) && (::GetFocus() == m_hWnd))
	{
		CHAR s[128];
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();

		s[0] = 0;
		wsprintf(s, (CMainFrame::m_dwPatternSetup & PATTERN_HEXDISPLAY) ? "Position %02Xh of %02Xh" : "Position %d of %d",
			m_nScrollPos, pSndFile->GetNumPatterns());
		if (m_nScrollPos < MAX_ORDERS)
		{
			UINT nPat = pSndFile->Order[m_nScrollPos];
			if ((nPat < MAX_PATTERNS) && (nPat < pSndFile->m_nPatternNames))
			{
				CHAR szpat[40] = "";
				if (pSndFile->GetPatternName(nPat, szpat))
				{
					wsprintf(s+strlen(s), ": %s", szpat);
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
	if ((m_cxFont > 0) && (m_cyFont > 0) && (m_pModDoc))
	{
		CRect rcClient, rect;

		UpdateScrollInfo();
		BOOL bFocus = (::GetFocus() == m_hWnd);
		dc.SetBkMode(TRANSPARENT);
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		GetClientRect(&rcClient);
		rect = rcClient;
		int nIndex = m_nXScroll;
		while (rect.left < rcClient.right)
		{
			BOOL bHighLight = ((bFocus) && (nIndex == m_nScrollPos)) ? TRUE : FALSE;
			int nOrder = ((nIndex >= 0) && (nIndex < MAX_ORDERS)) ? pSndFile->Order[nIndex] : -1;
			if ((rect.right = rect.left + m_cxFont) > rcClient.right) rect.right = rcClient.right;
			rect.right--;
			FillRect(dc.m_hDC, &rect, (bHighLight) ? CMainFrame::brushHighLight : CMainFrame::brushWindow);
			if (nIndex == ((m_bDragging) ? (int)m_nDropPos : m_nScrollPos))
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			MoveToEx(dc.m_hDC, rect.right, rect.top, NULL);
			LineTo(dc.m_hDC, rect.right, rect.bottom);
			if (nIndex == (int)pSndFile->m_nSeqOverride-1)
			{
				MoveToEx(dc.m_hDC, rect.left+4, rect.bottom-4, NULL);
				LineTo(dc.m_hDC, rect.right-4, rect.bottom-4);
			}
			s[0] = 0;
			if ((nOrder >= 0) && (rect.left + m_cxFont - 4 <= rcClient.right))
			{
				if (nOrder == 0xFF) strcpy(s, "---"); else
				if (nOrder < MAX_PATTERNS) wsprintf(s, "%d", nOrder);
				else strcpy(s, "+++");
			}
			dc.SetTextColor((bHighLight) ? colorTextSel : colorText);
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
}


void COrderList::OnKillFocus(CWnd *pWnd)
//--------------------------------------
{
	CWnd::OnKillFocus(pWnd);
	InvalidateSelection();
	m_bShift = FALSE;
}


void COrderList::OnLButtonDown(UINT nFlags, CPoint pt)
//----------------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	if (pt.y < rect.bottom)
	{
		SetFocus();
		if (CMainFrame::gnHotKeyMask & HOTKEYF_CONTROL)
		{
			if (m_pModDoc)
			{
				CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
				int nOrder = m_nXScroll + (pt.x - rect.left) / m_cxFont;
				if ((nOrder >= 0) && (nOrder < MAX_ORDERS))
				{
					pSndFile->m_nSeqOverride = nOrder+1;
					InvalidateRect(NULL, FALSE);
				}
			}
		} else
		{
			SetCurSel(m_nXScroll + (pt.x - rect.left) / m_cxFont);
			m_bDragging = TRUE;
			m_nDragOrder = GetCurSel();
			m_nDropPos = m_nDragOrder;
			SetCapture();
		}
	} else
	{
		CWnd::OnLButtonDown(nFlags, pt);
	}
}


void COrderList::OnLButtonUp(UINT nFlags, CPoint pt)
//--------------------------------------------------
{
	if (m_bDragging)
	{
		CRect rect;
		
		m_bDragging = FALSE;
		ReleaseCapture();
		GetClientRect(&rect);
		if (rect.PtInRect(pt))
		{
			int n = m_nXScroll + (pt.x - rect.left) / m_cxFont;
			if ((n >= 0) && (n == m_nDropPos) && (m_pModDoc))
			{
				if (m_nDragOrder == (UINT)m_nDropPos) return;
				if (m_pModDoc->MoveOrder(m_nDragOrder, m_nDropPos, TRUE, m_bShift))
				{
					SetCurSel(((m_nDragOrder < (UINT)m_nDropPos) && (!m_bShift)) ? m_nDropPos-1 : m_nDropPos);
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
		int n = -1;
		if (rect.PtInRect(pt))
		{
			n = m_nXScroll + (pt.x - rect.left) / m_cxFont;
			if ((n < 0) || (n >= MAX_ORDERS)) n = -1;
		}
		if (n != (int)m_nDropPos)
		{
			if (n >= 0)
			{
				m_nDropPos = n;
				InvalidateRect(NULL, FALSE);
				SetCursor(CMainFrame::curDragging);
			} else
			{
				m_nDropPos = -1;
				SetCursor(CMainFrame::curNoDrop);
			}
		}
	} else
	{
		CWnd::OnMouseMove(nFlags, pt);
	}
}


void COrderList::OnRButtonDown(UINT nFlags, CPoint pt)
//----------------------------------------------------
{
	CRect rect;
	GetClientRect(&rect);
	if (m_bDragging)
	{
		m_nDropPos = -1;
		OnLButtonUp(nFlags, pt);
	}
	if (pt.y < rect.bottom)
	{
		SetCurSel(m_nXScroll + (pt.x - rect.left) / m_cxFont);
		SetFocus();
		HMENU hMenu = ::CreatePopupMenu();
		if (hMenu)
		{
			AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_INSERT, "&Insert Pattern\tIns");
			AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_DELETE, "&Remove Pattern\tDel");
			AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
			AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_NEW, "Create &New Pattern");
			AppendMenu(hMenu, MF_STRING, ID_ORDERLIST_COPY, "&Duplicate Pattern");
			AppendMenu(hMenu, MF_STRING, ID_PATTERNCOPY, "&Copy Pattern");
			AppendMenu(hMenu, MF_STRING, ID_PATTERNPASTE, "P&aste Pattern");
			if ((m_pModDoc) && (m_pModDoc->GetSoundFile()->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)))
			{
				AppendMenu(hMenu, MF_SEPARATOR, NULL, "");
				AppendMenu(hMenu, MF_STRING, ID_PATTERN_PROPERTIES, "&Properties...");
			}
			ClientToScreen(&pt);
			::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
			::DestroyMenu(hMenu);
		}
	}
}


void COrderList::OnLButtonDblClk(UINT, CPoint)
//--------------------------------------------
{
	if ((m_pModDoc) && (m_pParent))
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		m_pParent->SetCurrentPattern(pSndFile->Order[m_nScrollPos]);
	}
}


void COrderList::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*)
//--------------------------------------------------------------
{
	UINT nNewPos = m_nXScroll;
	UINT smin, smax;
	
	GetScrollRange(SB_HORZ, (LPINT)&smin, (LPINT)&smax);
	m_bScrolling = TRUE;
	switch(nSBCode)
	{
	case SB_LEFT:			nNewPos = 0; break;
	case SB_LINELEFT:		if (nNewPos) nNewPos--; break;
	case SB_LINERIGHT:		if (nNewPos < smax) nNewPos++; break;
	case SB_PAGELEFT:		if (nNewPos > 4) nNewPos -= 4; else nNewPos = 0; break;
	case SB_PAGERIGHT:		if (nNewPos+4 < smax) nNewPos += 4; else nNewPos = smax; break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:	nNewPos = nPos; if (nNewPos & 0xFFFF8000) nNewPos = smin; break;
	case SB_ENDSCROLL:		m_bScrolling = FALSE; break;
	}
	if (nNewPos > smax) nNewPos = smax;
	if (nNewPos != (UINT)m_nXScroll)
	{
		m_nXScroll = nNewPos;
		SetScrollPos(SB_HORZ, m_nXScroll);
		InvalidateRect(NULL, FALSE);
	}
}


void COrderList::OnSize(UINT nType, int cx, int cy)
//-------------------------------------------------
{
	int smin, smax, nPos;

	CWnd::OnSize(nType, cx, cy);
	UpdateScrollInfo();
	GetScrollRange(SB_HORZ, &smin, &smax);
	nPos = (short int)GetScrollPos(SB_HORZ);
	if (nPos > smax) nPos = smax;
	if (m_nXScroll != nPos)
	{
		m_nXScroll = nPos;
		SetScrollPos(SB_HORZ, m_nXScroll);
		InvalidateRect(NULL, FALSE);
	}
}


void COrderList::OnInsertOrder()
//------------------------------
{
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		for (int i=MAX_ORDERS-1; i>m_nScrollPos; i--) pSndFile->Order[i] = pSndFile->Order[i-1];
		InvalidateRect(NULL, FALSE);
		m_pModDoc->SetModified();
		m_pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE, this);
	}
}


void COrderList::OnDeleteOrder()
//------------------------------
{
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		for (int i=m_nScrollPos; i<MAX_ORDERS-1; i++) pSndFile->Order[i] = pSndFile->Order[i+1];
		pSndFile->Order[MAX_ORDERS-1] = 0xFF;
		InvalidateRect(NULL, FALSE);
		m_pModDoc->SetModified();
		m_pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE, this);
		UINT nNewOrd = pSndFile->Order[m_nScrollPos];
		if ((nNewOrd < MAX_PATTERNS) && (pSndFile->Patterns[nNewOrd]) && (m_pParent))
		{
			m_pParent->SetCurrentPattern(nNewOrd);
		}
	}
}


void COrderList::OnPatternProperties()
//------------------------------------
{
	if (m_pParent) m_pParent->PostViewMessage(VIEWMSG_PATTERNPROPERTIES);
}


void COrderList::OnPlayerPlay()
//-----------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_PLAYER_PLAY);
}


void COrderList::OnPlayerPause()
//------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_PLAYER_PAUSE);
}


void COrderList::OnPlayerPlayFromStart()
//--------------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_PLAYER_PLAYFROMSTART);
}


void COrderList::OnPatternPlayFromStart()
//---------------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, IDC_PATTERN_PLAYFROMSTART);
}


void COrderList::OnCreateNewPattern()
//-----------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_ORDERLIST_NEW);
}


void COrderList::OnDuplicatePattern()
//-----------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_ORDERLIST_COPY);
}


void COrderList::OnPatternCopy()
//------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_PATTERNCOPY);
}


void COrderList::OnPatternPaste()
//-------------------------------
{
	if (m_pParent) m_pParent->PostMessage(WM_COMMAND, ID_PATTERNPASTE);
}


LRESULT COrderList::OnHelpHitTest(WPARAM, LPARAM)
//-----------------------------------------------
{
	return HID_BASE_COMMAND + IDC_ORDERLIST;
}


LRESULT COrderList::OnDragonDropping(WPARAM bDoDrop, LPARAM lParam)
//-----------------------------------------------------------------
{
	LPDRAGONDROP pDropInfo = (LPDRAGONDROP)lParam;
	UINT posdest;
	BOOL bCanDrop;
	CSoundFile *pSndFile;
	CPoint pt;

	if ((!pDropInfo) || (!m_pModDoc) || (m_pModDoc != pDropInfo->pModDoc) || (!m_cxFont)) return FALSE;
	bCanDrop = FALSE;
	switch(pDropInfo->dwDropType)
	{
	case DRAGONDROP_ORDER:
		if (pDropInfo->dwDropItem >= MAX_ORDERS) break;
	case DRAGONDROP_PATTERN:
		bCanDrop = TRUE;
		break;
	}
	if ((!bCanDrop) || (!bDoDrop)) return bCanDrop;
	GetCursorPos(&pt);
	ScreenToClient(&pt);
	if (pt.x < 0) pt.x = 0;
	posdest = m_nXScroll + (pt.x / m_cxFont);
	if (posdest >= MAX_ORDERS) return FALSE;
	pSndFile = m_pModDoc->GetSoundFile();
	switch(pDropInfo->dwDropType)
	{
	case DRAGONDROP_PATTERN:
		pSndFile->Order[posdest] = (BYTE)pDropInfo->dwDropItem;
		break;

	case DRAGONDROP_ORDER:
		pSndFile->Order[posdest] = pSndFile->Order[pDropInfo->dwDropItem];
		break;
	}
	if (bCanDrop)
	{
		InvalidateRect(NULL, FALSE);
		m_pModDoc->SetModified();
		m_pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE, this);
		SetCurSel(posdest, TRUE);
	}
	return bCanDrop;
}
