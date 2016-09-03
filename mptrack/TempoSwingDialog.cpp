/*
 * TempoSwingDialog.cpp
 * --------------------
 * Purpose: Implementation of the tempo swing configuration dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "TempoSwingDialog.h"
#include "Mainfrm.h"


OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(CTempoSwingDlg, CDialog)
	//{{AFX_MSG_MAP(CTempoSwingDlg)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON1,	OnReset)
	ON_COMMAND(IDC_CHECK1,	OnToggleGroup)
	ON_EN_CHANGE(IDC_EDIT1, OnGroupChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CTempoSwingDlg::m_groupSize = 1;

CTempoSwingDlg::CTempoSwingDlg(CWnd *parent, const TempoSwing &currentTempoSwing, CSoundFile &sndFile, PATTERNINDEX pattern)
	: CDialog(IDD_TEMPO_SWING, parent)
	, m_container(*this)
	, m_scrollPos(0)
	, m_tempoSwing(currentTempoSwing)
	, m_origTempoSwing(pattern == PATTERNINDEX_INVALID ? sndFile.m_tempoSwing : sndFile.Patterns[pattern].GetTempoSwing())
	, m_sndFile(sndFile)
	, m_pattern(pattern)
{
	m_groupSize = std::min(m_groupSize, static_cast<int>(m_tempoSwing.size()));
}


void CTempoSwingDlg::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK1, m_checkGroup);
	DDX_Control(pDX, IDC_SCROLLBAR1, m_scrollBar);
	DDX_Control(pDX, IDC_CONTAINER, m_container);
}


BOOL CTempoSwingDlg::OnInitDialog()
//---------------------------------
{
	struct Measurements
	{
		enum
		{
			edRowLabelWidth = 64,	// Label "Row 999:"
			edSliderWidth = 220,	// Setting slider
			edSliderHeight = 20,	// Setting slider
			edValueLabelWidth = 64,	// Label "100%"
			edPaddingX = 8,			// Spacing between elements
			edPaddingY = 4,			// Spacing between elements
			edPaddingTop = 64,		// Spacing from top of dialog
			edRowHeight = edSliderHeight + edPaddingY, // Height of one set of controls
			edFooterHeight = 32,	// Buttons
			edScrollbarWidth = 16,	// Width of optional scrollbar
		};

		const int rowLabelWidth;
		const int sliderWidth;
		const int sliderHeight;
		const int valueLabelWidth;
		const int paddingX;
		const int paddingY;
		const int paddingTop;
		const int rowHeight;
		const int footerHeight;
		const int scrollbarWidth;

		Measurements(HWND hWnd)
			: rowLabelWidth(Util::ScalePixels(edRowLabelWidth, hWnd))
			, sliderWidth(Util::ScalePixels(edSliderWidth, hWnd))
			, sliderHeight(Util::ScalePixels(edSliderHeight, hWnd))
			, valueLabelWidth(Util::ScalePixels(edValueLabelWidth, hWnd))
			, paddingX(Util::ScalePixels(edPaddingX, hWnd))
			, paddingY(Util::ScalePixels(edPaddingY, hWnd))
			, paddingTop(Util::ScalePixels(edPaddingTop, hWnd))
			, rowHeight(Util::ScalePixels(edRowHeight, hWnd))
			, footerHeight(Util::ScalePixels(edFooterHeight, hWnd))
			, scrollbarWidth(Util::ScalePixels(edScrollbarWidth, hWnd))
		{ }
	};

	CDialog::OnInitDialog();
	Measurements m(m_hWnd);
	CRect windowRect, rect;
	GetWindowRect(windowRect);
	GetClientRect(rect);
	windowRect.bottom = windowRect.top + windowRect.Height() - rect.Height();

	CRect mainWindowRect;
	CMainFrame::GetMainFrame()->GetClientRect(mainWindowRect);

	const int realHeight = static_cast<int>(m_tempoSwing.size()) * m.rowHeight;
	const int displayHeight = std::min<int>(realHeight, mainWindowRect.bottom - windowRect.Height() - m.paddingTop - m.footerHeight);

	CRect containerRect;
	m_container.GetClientRect(containerRect);
	containerRect.bottom = displayHeight;
	m_container.SetWindowPos(nullptr, 0, m.paddingTop, rect.right - m.scrollbarWidth, containerRect.bottom, SWP_NOZORDER);
	m_container.ModifyStyleEx(0, WS_EX_CONTROLPARENT, 0);

	// Need scrollbar?
	if(realHeight > displayHeight)
	{
		SCROLLINFO info;
		info.cbSize = sizeof(info);
		info.fMask = SIF_ALL;
		info.nMin = 0;
		info.nMax = realHeight;
		info.nPage = displayHeight;
		info.nTrackPos = info.nPos = 0;
		m_scrollBar.SetScrollInfo(&info, FALSE);

		CRect scrollRect;
		m_scrollBar.GetClientRect(scrollRect);
		m_scrollBar.SetWindowPos(nullptr, containerRect.right, m.paddingTop, scrollRect.Width(), displayHeight, SWP_NOZORDER);
	} else
	{
		m_scrollBar.ShowWindow(SW_HIDE);
	}

	rect.DeflateRect(m.paddingX, 0/* m.paddingTop*/, m.paddingX + m.scrollbarWidth, 0);

	m_controls.resize(m_tempoSwing.size());
	for(size_t i = 0; i < m_controls.size(); i++)
	{
		RowCtls *r = m_controls[i] = new RowCtls;
		// Row label
		TCHAR s[16];
		wsprintf(s, "Row %u:", i + 1);
		r->rowLabel.Create(s, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, CRect(rect.left, rect.top, rect.right, rect.top + m.rowHeight), &m_container);
		r->rowLabel.SetFont(GetFont());

		// Value label
		r->valueLabel.Create(_T("100%"), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, CRect(rect.right - m.valueLabelWidth, rect.top, rect.right, rect.top + m.sliderHeight), &m_container);
		r->valueLabel.SetFont(GetFont());

		// Value slider
		r->valueSlider.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_TOOLTIPS | TBS_AUTOTICKS, CRect(rect.left + m.rowLabelWidth, rect.top, rect.right - m.valueLabelWidth, rect.top + m.sliderHeight), &m_container, 0xFFFF);
		r->valueSlider.SetFont(GetFont());
		r->valueSlider.SetRange(-SliderResolution / 2, SliderResolution / 2);
		r->valueSlider.SetTicFreq(SliderResolution / 8);
		r->valueSlider.SetPageSize(SliderResolution / 8);
		int32 val = Util::muldivr(static_cast<int32>(m_tempoSwing[i]) - TempoSwing::Unity, SliderUnity, TempoSwing::Unity);
		r->valueSlider.SetPos(val);
		rect.MoveToY(rect.top + m.rowHeight);
	}

	((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN1))->SetRange32(1, static_cast<int>(m_tempoSwing.size()));
	SetDlgItemInt(IDC_EDIT1, m_groupSize);
	OnToggleGroup();

	m_container.OnHScroll(0, 0, reinterpret_cast<CScrollBar *>(&(m_controls[0]->valueSlider)));
	rect.MoveToY(m.paddingTop + containerRect.bottom + m.paddingY);
	{
		CRect buttonRect;
		GetDlgItem(IDOK)->GetWindowRect(buttonRect);
		GetDlgItem(IDOK)->SetWindowPos(nullptr, buttonRect.left - windowRect.left - GetSystemMetrics(SM_CXEDGE), rect.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);
		GetDlgItem(IDCANCEL)->GetWindowRect(buttonRect);
		GetDlgItem(IDCANCEL)->SetWindowPos(nullptr, buttonRect.left - windowRect.left - GetSystemMetrics(SM_CXEDGE), rect.top, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER);
	}

	windowRect.bottom += displayHeight + m.paddingTop + m.footerHeight;
	SetWindowPos(nullptr, 0, 0, windowRect.Width(), windowRect.Height(), SWP_NOMOVE | SWP_NOOWNERZORDER);
	EnableToolTips();

	return TRUE;
}


void CTempoSwingDlg::OnOK()
//-------------------------
{
	CDialog::OnOK();
	// If this is the default setup, just clear the vector.
	if(static_cast<size_t>(std::count(m_tempoSwing.begin(), m_tempoSwing.end(), TempoSwing::Unity)) == m_tempoSwing.size())
	{
		m_tempoSwing.clear();
	}
	OnClose();
}


void CTempoSwingDlg::OnCancel()
//-----------------------------
{
	CDialog::OnCancel();
	OnClose();
}


void CTempoSwingDlg::OnClose()
//----------------------------
{
	for(size_t i = 0; i < m_controls.size(); i++)
	{
		delete m_controls[i];
	}
	// Restore original swing properties after preview
	if(m_pattern == PATTERNINDEX_INVALID)
	{
		m_sndFile.m_tempoSwing = m_origTempoSwing;
	} else
	{
		m_sndFile.Patterns[m_pattern].SetTempoSwing(m_origTempoSwing);
	}
}


void CTempoSwingDlg::OnReset()
//----------------------------
{
	for(size_t i = 0; i < m_controls.size(); i++)
	{
		m_controls[i]->valueSlider.SetPos(0);
	}
	m_container.OnHScroll(0, 0, reinterpret_cast<CScrollBar *>(&(m_controls[0]->valueSlider)));
}


void CTempoSwingDlg::OnToggleGroup()
//----------------------------------
{
	const BOOL checked = m_checkGroup.GetCheck() != BST_UNCHECKED;
	GetDlgItem(IDC_EDIT1)->EnableWindow(checked);
	GetDlgItem(IDC_SPIN1)->EnableWindow(checked);
}


void CTempoSwingDlg::OnGroupChanged()
//-----------------------------------
{
	int val = GetDlgItemInt(IDC_EDIT1);
	if(val > 0) m_groupSize = std::min(val, static_cast<int>(m_tempoSwing.size()));
}


void CTempoSwingDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
//-----------------------------------------------------------------------------
{
	if(pScrollBar == &m_scrollBar)
	{
		// Get the minimum and maximum scrollbar positions.
		int minpos;
		int maxpos;
		pScrollBar->GetScrollRange(&minpos, &maxpos);

		SCROLLINFO sbInfo;
		pScrollBar->GetScrollInfo(&sbInfo);

		// Get the current position of scroll box.
		int curpos = pScrollBar->GetScrollPos();

		// Determine the new position of scroll box.
		switch(nSBCode)
		{
		case SB_LEFT:			// Scroll to far left.
			curpos = minpos;
			break;

		case SB_RIGHT:			// Scroll to far right.
			curpos = maxpos;
			break;

		case SB_ENDSCROLL:		// End scroll.
			m_container.Invalidate();
			break;

		case SB_LINELEFT:		// Scroll left.
			if(curpos > minpos)
				curpos--;
			break;

		case SB_LINERIGHT:		// Scroll right.
			if(curpos < maxpos)
				curpos++;
			break;

		case SB_PAGELEFT:		// Scroll one page left.
			if(curpos > minpos)
			{
				curpos = MAX(minpos, curpos - (int)sbInfo.nPage);
			}
			break;

		case SB_PAGERIGHT:		// Scroll one page right.
			if(curpos < maxpos)
			{
				curpos = MIN(maxpos, curpos + (int)sbInfo.nPage);
			}
			break;

		case SB_THUMBPOSITION:	// Scroll to absolute position. nPos is the position
			curpos = nPos;		// of the scroll box at the end of the drag operation.
			break;

		case SB_THUMBTRACK:		// Drag scroll box to specified position. nPos is the
			curpos = nPos;		// position that the scroll box has been dragged to.
			break;
		}

		// Set the new position of the thumb (scroll box).
		pScrollBar->SetScrollPos(curpos);

		m_container.ScrollWindowEx(0, m_scrollPos - curpos, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
		m_scrollPos = curpos;
	}

	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}


// Scrollable container for the sliders
BEGIN_MESSAGE_MAP(CTempoSwingDlg::SliderContainer, CDialog)
	//{{AFX_MSG_MAP(CTempoSwingDlg::SliderContainer)
	ON_WM_HSCROLL()
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CTempoSwingDlg::SliderContainer::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
//----------------------------------------------------------------------------------------------
{
	if(m_parent.m_checkGroup.GetCheck() != BST_UNCHECKED)
	{
		// Edit groups
		size_t editedGroup = 0;
		int editedValue = reinterpret_cast<CSliderCtrl *>(pScrollBar)->GetPos();
		for(size_t i = 0; i < m_parent.m_controls.size(); i++)
		{
			if(m_parent.m_controls[i]->valueSlider.m_hWnd == pScrollBar->m_hWnd)
			{
				editedGroup = (i / m_parent.m_groupSize) % 2u;
				break;
			}
		}
		for(size_t i = 0; i < m_parent.m_controls.size(); i++)
		{
			if((i / m_parent.m_groupSize) % 2u == editedGroup)
			{
				m_parent.m_controls[i]->valueSlider.SetPos(editedValue);
			}
		}
	}

	for(size_t i = 0; i < m_parent.m_controls.size(); i++)
	{
		m_parent.m_tempoSwing[i] = Util::muldivr(m_parent.m_controls[i]->valueSlider.GetPos(), TempoSwing::Unity, SliderUnity) + TempoSwing::Unity;
	}
	m_parent.m_tempoSwing.Normalize();
	// Apply preview
	if(m_parent.m_pattern == PATTERNINDEX_INVALID)
	{
		m_parent.m_sndFile.m_tempoSwing = m_parent.m_tempoSwing;
	} else
	{
		m_parent.m_sndFile.Patterns[m_parent.m_pattern].SetTempoSwing(m_parent.m_tempoSwing);
	}

	for(size_t i = 0; i < m_parent.m_tempoSwing.size(); i++)
	{
		TCHAR s[32];
		wsprintf(s, _T("%u%%"), Util::muldivr(m_parent.m_tempoSwing[i], 100, TempoSwing::Unity));
		m_parent.m_controls[i]->valueLabel.SetWindowText(s);
	}

	CStatic::OnHScroll(nSBCode, nPos, pScrollBar);
}


BOOL CTempoSwingDlg::SliderContainer::OnToolTipNotify(UINT, NMHDR *pNMHDR, LRESULT *)
//-----------------------------------------------------------------------------------
{
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXTA*)pNMHDR;
	for(size_t i = 0; i < m_parent.m_controls.size(); i++)
	{
		if((HWND)pNMHDR->idFrom == m_parent.m_controls[i]->valueSlider.m_hWnd)
		{
			int32 val = Util::muldivr(m_parent.m_tempoSwing[i], 100, TempoSwing::Unity) - 100;
			wsprintf(pTTT->szText, _T("%s%d"), val > 0 ? _T("+") : _T(""), val);
			return TRUE;
		}
	}
	return FALSE;
}


OPENMPT_NAMESPACE_END
