/*
 * tuningRatioMapWnd.cpp
 * ---------------------
 * Purpose: Alternative sample tuning configuration dialog - ratio map edit control.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "../soundlib/tuning.h"
#include "tuningRatioMapWnd.h"
#include "TuningDialog.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CTuningRatioMapWnd, CStatic)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


void CTuningRatioMapWnd::Init(CTuningDialog* const pParent, CTuning* const tuning)
{
	m_pParent = pParent;
	m_pTuning = tuning;
}

void CTuningRatioMapWnd::OnPaint()
{
	CPaintDC dc(this);

	if(!m_pTuning) return;

	CRect rcClient;
	GetClientRect(&rcClient);
	
	const auto colorText = GetSysColor(COLOR_WINDOWTEXT);
	const auto colorTextSel = GetSysColor(COLOR_HIGHLIGHTTEXT);
	const auto highlightBrush = GetSysColorBrush(COLOR_HIGHLIGHT), windowBrush = GetSysColorBrush(COLOR_WINDOW);

	auto oldFont = dc.SelectObject(CMainFrame::GetGUIFont());
	dc.SetBkMode(TRANSPARENT);
	if ((m_cxFont <= 0) || (m_cyFont <= 0))
	{
		CSize sz;
		sz = dc.GetTextExtent(_T("123456789"));
		m_cyFont = sz.cy + 2;
		m_cxFont = rcClient.right / 4;
	}
	dc.IntersectClipRect(&rcClient);
	if ((m_cxFont > 0) && (m_cyFont > 0))
	{
		const bool focus = (::GetFocus() == m_hWnd);
		CRect rect;

		NOTEINDEXTYPE nNotes = static_cast<NOTEINDEXTYPE>((rcClient.bottom + m_cyFont - 1) / m_cyFont);
		//if(!m_nNote) m_nNote = m_nNoteCentre;
		NOTEINDEXTYPE nPos = m_nNote - (nNotes/2);
		int ypaint = 0;

		for (int ynote=0; ynote<nNotes; ynote++, ypaint+=m_cyFont, nPos++)
		{
			// Note
			NOTEINDEXTYPE noteToDraw = nPos - m_nNoteCentre;
			const bool isValidNote = m_pTuning->IsValidNote(noteToDraw);

			rect.SetRect(0, ypaint, m_cxFont, ypaint + m_cyFont);
			const auto noteStr = isValidNote ? mpt::tfmt::val(noteToDraw) : mpt::tstring(_T("..."));
			DrawButtonRect(dc, &rect, noteStr.c_str(), FALSE, FALSE);

			// Mapped Note
			const bool highLight = focus && (nPos == (int)m_nNote);
			rect.left = rect.right;
			rect.right = m_cxFont*4-1;
			FillRect(dc, &rect, highLight ? highlightBrush : windowBrush);
			if(nPos == (int)m_nNote)
			{
				rect.InflateRect(-1, -1);
				dc.DrawFocusRect(&rect);
				rect.InflateRect(1, 1);
			}
			dc.SetTextColor(highLight ? colorTextSel : colorText);

			rect.SetRect(m_cxFont * 1, ypaint, m_cxFont * 2 - 1, ypaint + m_cyFont);
			dc.DrawText(mpt::ToCString(m_pTuning->GetNoteName(noteToDraw)), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

			rect.SetRect(m_cxFont * 2, ypaint, m_cxFont * 3 - 1, ypaint + m_cyFont);
			dc.DrawText(mpt::cfmt::flt(m_pTuning->GetRatio(noteToDraw), 6), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

			rect.SetRect(m_cxFont * 3, ypaint, m_cxFont * 4 - 1, ypaint + m_cyFont);
			dc.DrawText(mpt::cfmt::fix(std::log2(static_cast<double>(m_pTuning->GetRatio(noteToDraw))) * 1200.0, 1), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);

		}
		rect.SetRect(rcClient.left + m_cxFont * 4 - 1, rcClient.top, rcClient.left + m_cxFont * 4 + 3, ypaint);
		DrawButtonRect(dc, &rect, _T(""));
		if (ypaint < rcClient.bottom)
		{
			rect.SetRect(rcClient.left, ypaint, rcClient.right, rcClient.bottom);
			FillRect(dc, &rect, GetSysColorBrush(COLOR_BTNFACE));
		}
	}
	dc.SelectObject(oldFont);
}

void CTuningRatioMapWnd::OnSetFocus(CWnd *pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	InvalidateRect(NULL, FALSE);
}


void CTuningRatioMapWnd::OnKillFocus(CWnd *pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	InvalidateRect(NULL, FALSE);
}


BOOL CTuningRatioMapWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	NOTEINDEXTYPE note = static_cast<NOTEINDEXTYPE>(m_nNote - mpt::signum(zDelta));
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
{
	return m_nNote - m_nNoteCentre;
}


OPENMPT_NAMESPACE_END
