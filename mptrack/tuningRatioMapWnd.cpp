/*
 * tuningRatioMapWnd.cpp
 * ---------------------
 * Purpose: Alternative sample tuning configuration dialog - ratio map edit control.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "../soundlib/tuning.h"
#include "tuningRatioMapWnd.h"
#include "tuningdialog.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CTuningRatioMapWnd, CStatic)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()



void CTuningRatioMapWnd::OnPaint()
//-------------------------------
{
	HGDIOBJ oldfont = NULL;
	CRect rcClient;
	CPaintDC dc(this);
	HDC hdc;

	if(!m_pTuning) return;

	GetClientRect(&rcClient);
	if (!m_hFont)
	{
		m_hFont = CMainFrame::GetGUIFont();
		colorText = GetSysColor(COLOR_WINDOWTEXT);
		colorTextSel = GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	hdc = dc.m_hDC;
	oldfont = ::SelectObject(hdc, m_hFont);
	dc.SetBkMode(TRANSPARENT);
	if ((m_cxFont <= 0) || (m_cyFont <= 0))
	{
		CSize sz;
		sz = dc.GetTextExtent(CString("123456789"));
		m_cyFont = sz.cy + 2;
		m_cxFont = rcClient.right / 2;
	}
	dc.IntersectClipRect(&rcClient);
	if ((m_cxFont > 0) && (m_cyFont > 0))
	{
		BOOL bFocus = (::GetFocus() == m_hWnd) ? TRUE : FALSE;
		CHAR s[64];
		const size_t sizeofS = sizeof(s) / sizeof(s[0]);
		CRect rect;

		NOTEINDEXTYPE nNotes = static_cast<NOTEINDEXTYPE>((rcClient.bottom + m_cyFont - 1) / m_cyFont);
		//if(!m_nNote) m_nNote = m_nNoteCentre;
		NOTEINDEXTYPE nPos = m_nNote - (nNotes/2);
		int ypaint = 0;


		for (int ynote=0; ynote<nNotes; ynote++, ypaint+=m_cyFont, nPos++)
		{
			BOOL bHighLight;
			// Note
			NOTEINDEXTYPE noteToDraw = nPos - m_nNoteCentre;
			s[0] = 0;

			const bool isValidNote = m_pTuning->IsValidNote(noteToDraw);
			std::string temp;
			if(isValidNote)
			{
				temp = "(" + mpt::ToString(noteToDraw) + ")   " + m_pTuning->GetNoteName(noteToDraw);
			}

			if(isValidNote && temp.size()+1 < sizeofS)
				wsprintf(s, "%s", temp.c_str());
			else
				wsprintf(s, "%s", "...");


			rect.SetRect(0, ypaint, m_cxFont, ypaint+m_cyFont);
			DrawButtonRect(hdc, &rect, s, FALSE, FALSE);
			// Mapped Note
			bHighLight = ((bFocus) && (nPos == (int)m_nNote) ) ? TRUE : FALSE;
			rect.left = rect.right;
			rect.right = m_cxFont*2-1;
			strcpy(s, "...");
			FillRect(hdc, &rect, (bHighLight) ? CMainFrame::brushHighLight : CMainFrame::brushWindow);
			if (nPos == (int)m_nNote)
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			dc.SetTextColor((bHighLight) ? colorTextSel : colorText);
			std::string str = mpt::ToString(m_pTuning->GetRatio(noteToDraw));
			dc.DrawText(str.c_str(), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		}
		rect.SetRect(rcClient.left+m_cxFont*2-1, rcClient.top, rcClient.left+m_cxFont*2+3, ypaint);
		DrawButtonRect(hdc, &rect, "", FALSE, FALSE);
		if (ypaint < rcClient.bottom)
		{
			rect.SetRect(rcClient.left, ypaint, rcClient.right, rcClient.bottom);
			FillRect(hdc, &rect, CMainFrame::brushGray);
		}
	}
	if (oldfont) ::SelectObject(hdc, oldfont);
}

void CTuningRatioMapWnd::OnSetFocus(CWnd *pOldWnd)
//-----------------------------------------
{
	CWnd::OnSetFocus(pOldWnd);
	InvalidateRect(NULL, FALSE);
}


void CTuningRatioMapWnd::OnKillFocus(CWnd *pNewWnd)
//------------------------------------------
{
	CWnd::OnKillFocus(pNewWnd);
	InvalidateRect(NULL, FALSE);
}


BOOL CTuningRatioMapWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//-------------------------------------------------------------------------
{
	NOTEINDEXTYPE note = static_cast<NOTEINDEXTYPE>(m_nNote - sgn(zDelta));
	if(m_pTuning->IsValidNote(note - m_nNoteCentre))
	{
		m_nNote = note;
		InvalidateRect(NULL, FALSE);
		if(m_pParent)
			m_pParent->UpdateRatioMapEdits(GetShownCentre());
	}

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}


void CTuningRatioMapWnd::OnLButtonDown(UINT, CPoint pt)
//-----------------------------------------------------
{
	if ((pt.x >= m_cxFont) && (pt.x < m_cxFont*2))
	{
		InvalidateRect(NULL, FALSE);
	}
	if ((pt.x > m_cxFont*2) && (pt.x <= m_cxFont*3))
	{
		InvalidateRect(NULL, FALSE);
	}
	if ((pt.x >= 0) && (m_cyFont))
	{
		CRect rcClient;
		GetClientRect(&rcClient);
		int nNotes = (rcClient.bottom + m_cyFont - 1) / m_cyFont;
		const int n = (pt.y / m_cyFont) + m_nNote - (nNotes/2);
		const NOTEINDEXTYPE note = static_cast<NOTEINDEXTYPE>(n - m_nNoteCentre);
		if(m_pTuning->IsValidNote(note))
		{
			m_nNote = static_cast<NOTEINDEXTYPE>(n);
			InvalidateRect(NULL, FALSE);
			if(m_pParent)
				m_pParent->UpdateRatioMapEdits(GetShownCentre());
		}

	}
	SetFocus();
}



NOTEINDEXTYPE CTuningRatioMapWnd::GetShownCentre() const
//------------------------------------------------------
{
	return m_nNote - m_nNoteCentre;
}


OPENMPT_NAMESPACE_END