#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "../soundlib/tuning.h"
#include "tuningRatioMapWnd.h"
#include "tuningdialog.h"

BEGIN_MESSAGE_MAP(CTuningRatioMapWnd, CStatic)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
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

		CTuning::STEPTYPE nNotes = static_cast<CTuning::STEPTYPE>((rcClient.bottom + m_cyFont - 1) / m_cyFont);
		if(!m_nNote) m_nNote = m_nNoteCentre;
		CTuning::STEPTYPE nPos = m_nNote - (nNotes/2);
		int ypaint = 0;

		
		for (int ynote=0; ynote<nNotes; ynote++, ypaint+=m_cyFont, nPos++)
		{
			BOOL bHighLight;
			// Note
			s[0] = 0;
			const string temp = m_pTuning->GetNoteName(nPos - 61).c_str();
			if(temp.size() >= sizeofS)
				wsprintf(s, "%s", "...");
			else
				wsprintf(s, "%s", temp.c_str());

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
			string str = Stringify(m_pTuning->GetFrequencyRatio(nPos - m_nNoteCentre));
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

void CTuningRatioMapWnd::OnLButtonDown(UINT, CPoint pt)
//----------------------------------------------
{
	if ((pt.x >= m_cxFont) && (pt.x < m_cxFont*2)) {
		InvalidateRect(NULL, FALSE);
	}
	if ((pt.x > m_cxFont*2) && (pt.x <= m_cxFont*3)) {
		InvalidateRect(NULL, FALSE);
	}
	if ((pt.x >= 0) && (m_cyFont)) {
		CRect rcClient;
		GetClientRect(&rcClient);
		int nNotes = (rcClient.bottom + m_cyFont - 1) / m_cyFont;
		UINT n = (pt.y / m_cyFont) + m_nNote - (nNotes/2);
		m_nNote = static_cast<CTuning::STEPTYPE>(n);
		InvalidateRect(NULL, FALSE);
		if(m_pParent)
			m_pParent->UpdateRatioMapEdits(GetShownCentre());
	}
	SetFocus();
}



CTuning::STEPTYPE CTuningRatioMapWnd::GetShownCentre() const
//--------------------------------------
{
	return m_nNote - m_nNoteCentre;
}

