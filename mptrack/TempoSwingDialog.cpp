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
#include "HighDPISupport.h"
#include "Mainfrm.h"
#include "resource.h"


OPENMPT_NAMESPACE_BEGIN

void CTempoSwingDlg::RowCtls::SetValue(TempoSwing::value_type v)
{
	int32 val = Util::muldivr(static_cast<int32>(v) - TempoSwing::Unity, CTempoSwingDlg::SliderUnity, TempoSwing::Unity);
	valueSlider.SetPos(val);
}


TempoSwing::value_type CTempoSwingDlg::RowCtls::GetValue() const
{
	return Util::muldivr(valueSlider.GetPos(), TempoSwing::Unity, SliderUnity) + TempoSwing::Unity;
}


struct TempoSwingMeasurements
{
	enum
	{
		edRowLabelWidth = 64,                       // Label "Row 999:"
		edSliderWidth = 220,                        // Setting slider
		edSliderHeight = 20,                        // Setting slider
		edValueLabelWidth = 64,                     // Label "100%"
		edPaddingX = 8,                             // Spacing between elements
		edPaddingY = 4,                             // Spacing between elements
		edPaddingTop = 64,                          // Spacing from top of dialog
		edRowHeight = edSliderHeight + edPaddingY,  // Height of one set of controls
		edFooterHeight = 32,                        // Buttons
		edScrollbarWidth = 16,                      // Width of optional scrollbar
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

	TempoSwingMeasurements(HWND hWnd)
		: rowLabelWidth(HighDPISupport::ScalePixels(edRowLabelWidth, hWnd))
		, sliderWidth(HighDPISupport::ScalePixels(edSliderWidth, hWnd))
		, sliderHeight(HighDPISupport::ScalePixels(edSliderHeight, hWnd))
		, valueLabelWidth(HighDPISupport::ScalePixels(edValueLabelWidth, hWnd))
		, paddingX(HighDPISupport::ScalePixels(edPaddingX, hWnd))
		, paddingY(HighDPISupport::ScalePixels(edPaddingY, hWnd))
		, paddingTop(HighDPISupport::ScalePixels(edPaddingTop, hWnd))
		, rowHeight(HighDPISupport::ScalePixels(edRowHeight, hWnd))
		, footerHeight(HighDPISupport::ScalePixels(edFooterHeight, hWnd))
		, scrollbarWidth(HighDPISupport::ScalePixels(edScrollbarWidth, hWnd))
	{
	}


	CRect RowLabelRect(const CRect rect) const noexcept
	{
		return CRect{rect.left, rect.top, rect.right, rect.top + rowHeight};
	}

	CRect ValueLabelRect(const CRect rect) const noexcept
	{
		return CRect{rect.right - valueLabelWidth, rect.top, rect.right, rect.top + sliderHeight};
	}

	CRect ValueSliderRect(const CRect rect) const noexcept
	{
		return CRect{rect.left + rowLabelWidth, rect.top, rect.right - valueLabelWidth, rect.top + sliderHeight};
	}
};



BEGIN_MESSAGE_MAP(CTempoSwingDlg, DialogBase)
	//{{AFX_MSG_MAP(CTempoSwingDlg)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON1, &CTempoSwingDlg::OnReset)
	ON_COMMAND(IDC_BUTTON2, &CTempoSwingDlg::OnUseGlobal)
	ON_COMMAND(IDC_CHECK1,  &CTempoSwingDlg::OnToggleGroup)
	ON_EN_CHANGE(IDC_EDIT1, &CTempoSwingDlg::OnGroupChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CTempoSwingDlg::m_groupSize = 1;

CTempoSwingDlg::CTempoSwingDlg(CWnd *parent, const TempoSwing &currentTempoSwing, CSoundFile &sndFile, PATTERNINDEX pattern)
	: DialogBase(IDD_TEMPO_SWING, parent)
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
{
	DialogBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK1, m_checkGroup);
	DDX_Control(pDX, IDC_SCROLLBAR1, m_scrollBar);
	DDX_Control(pDX, IDC_CONTAINER, m_container);
}


BOOL CTempoSwingDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();
	
	TempoSwingMeasurements m{m_hWnd};
	CRect windowRect, rect;
	GetWindowRect(windowRect);
	GetClientRect(rect);
	windowRect.bottom = windowRect.top + windowRect.Height() - rect.Height();

	CRect mainWindowRect;
	CMainFrame::GetMainFrame()->GetClientRect(mainWindowRect);

	const int realHeight = static_cast<int>(m_tempoSwing.size()) * m.rowHeight;
	const int displayHeight = std::min(realHeight, static_cast<int>(mainWindowRect.bottom - windowRect.Height() - m.paddingTop - m.footerHeight));

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

	rect.DeflateRect(m.paddingX, 0, m.paddingX + m.scrollbarWidth, 0);

	GetDlgItem(IDC_BUTTON2)->ShowWindow((m_pattern != PATTERNINDEX_INVALID) ? SW_SHOW : SW_HIDE);

	m_controls.resize(m_tempoSwing.size());
	CFont *font = GetFont();
	for(size_t i = 0; i < m_controls.size(); i++)
	{
		m_controls[i] = std::make_unique<RowCtls>();
		auto &r = m_controls[i];
		// Row label
		r->rowLabel.Create(MPT_CFORMAT("Row {}:")(i + 1), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, m.RowLabelRect(rect), &m_container);
		r->rowLabel.SetFont(font);

		// Value label
		r->valueLabel.Create(_T("100%"), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, m.ValueLabelRect(rect), &m_container);
		r->valueLabel.SetFont(font);

		// Value slider
		r->valueSlider.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_TOOLTIPS | TBS_AUTOTICKS, m.ValueSliderRect(rect), &m_container, 0xFFFF);
		r->valueSlider.SetFont(font);
		r->valueSlider.SetRange(-SliderResolution / 2, SliderResolution / 2);
		r->valueSlider.SetTicFreq(SliderResolution / 8);
		r->valueSlider.SetPageSize(SliderResolution / 8);
		r->valueSlider.SetPos(1);	// Work around https://bugs.winehq.org/show_bug.cgi?id=41909
		SetWindowLongPtr(r->valueSlider, GWLP_USERDATA, i);
		r->SetValue(m_tempoSwing[i]);
		rect.MoveToY(rect.top + m.rowHeight);
	}

	static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1))->SetRange32(1, static_cast<int>(m_tempoSwing.size()));
	SetDlgItemInt(IDC_EDIT1, m_groupSize);
	OnToggleGroup();

	m_container.OnHScroll(0, 0, reinterpret_cast<CScrollBar *>(&(m_controls[0]->valueSlider)));
	rect.MoveToY(m.paddingTop + containerRect.bottom + m.paddingY);
	{
		// Buttons at dialog bottom
		const int cxEdge = HighDPISupport::GetSystemMetrics(SM_CYHSCROLL, GetDPI());
		CRect buttonRect;
		for(auto i : { IDOK, IDCANCEL, IDC_BUTTON2 })
		{
			auto wnd = GetDlgItem(i);
			wnd->GetWindowRect(buttonRect);
			wnd->SetWindowPos(nullptr, buttonRect.left - windowRect.left - cxEdge, rect.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
		}
	}

	windowRect.bottom += displayHeight + m.paddingTop + m.footerHeight;
	SetWindowPos(nullptr, 0, 0, windowRect.Width(), windowRect.Height(), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

	return TRUE;
}


void CTempoSwingDlg::OnDPIChanged()
{
	DialogBase::OnDPIChanged();

	// For some reason, these controls are not sized automatically and lose their intended font because they are parented in the static text field...
	TempoSwingMeasurements m{m_hWnd};
	CFont *font = GetFont();
	auto dwp = ::BeginDeferWindowPos(static_cast<int>(m_controls.size() * 3));
	CRect rect;
	m_container.GetClientRect(rect);
	rect.DeflateRect(m.paddingX, 0, m.paddingX + m.scrollbarWidth, 0);
	for(auto &r: m_controls)
	{
		r->rowLabel.SetFont(font);
		r->valueLabel.SetFont(font);
		r->valueSlider.SetFont(font);

		const CRect rowLabelRect = m.RowLabelRect(rect);
		const CRect valueLabelRect = m.ValueLabelRect(rect);
		const CRect valueSliderRect = m.ValueSliderRect(rect);
		::DeferWindowPos(dwp, r->rowLabel, nullptr, rowLabelRect.left, rowLabelRect.top, rowLabelRect.Width(), rowLabelRect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS);
		::DeferWindowPos(dwp, r->valueLabel, nullptr, valueLabelRect.left, valueLabelRect.top, valueLabelRect.Width(), valueLabelRect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS);
		::DeferWindowPos(dwp, r->valueSlider, nullptr, valueSliderRect.left, valueSliderRect.top, valueSliderRect.Width(), valueSliderRect.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS);
		rect.MoveToY(rect.top + m.rowHeight);
	}
	::EndDeferWindowPos(dwp);
}


void CTempoSwingDlg::OnOK()
{
	DialogBase::OnOK();
	// If this is the default setup, just clear the vector.
	if(m_pattern == PATTERNINDEX_INVALID)
	{
		if(static_cast<size_t>(std::count(m_tempoSwing.begin(), m_tempoSwing.end(), static_cast<TempoSwing::value_type>(TempoSwing::Unity))) == m_tempoSwing.size())
		{
			m_tempoSwing.clear();
		}
	} else
	{
		if(m_tempoSwing == m_sndFile.m_tempoSwing)
		{
			m_tempoSwing.clear();
		}
	}
	OnClose();
}


void CTempoSwingDlg::OnCancel()
{
	DialogBase::OnCancel();
	OnClose();
}


void CTempoSwingDlg::OnClose()
{
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
{
	for(auto &control : m_controls)
	{
		control->valueSlider.SetPos(0);
	}
	m_container.OnHScroll(0, 0, reinterpret_cast<CScrollBar *>(&(m_controls[0]->valueSlider)));
}


void CTempoSwingDlg::OnUseGlobal()
{
	if(m_sndFile.m_tempoSwing.empty())
	{
		OnReset();
		return;
	}
	for(size_t i = 0; i < m_controls.size(); i++)
	{
		m_controls[i]->SetValue(m_sndFile.m_tempoSwing[i % m_sndFile.m_tempoSwing.size()]);
	}
	m_container.OnHScroll(0, 0, reinterpret_cast<CScrollBar *>(&(m_controls[0]->valueSlider)));
}


void CTempoSwingDlg::OnToggleGroup()
{
	const BOOL checked = m_checkGroup.GetCheck() != BST_UNCHECKED;
	GetDlgItem(IDC_EDIT1)->EnableWindow(checked);
	GetDlgItem(IDC_SPIN1)->EnableWindow(checked);
}


void CTempoSwingDlg::OnGroupChanged()
{
	int val = GetDlgItemInt(IDC_EDIT1);
	if(val > 0) m_groupSize = std::min(val, static_cast<int>(m_tempoSwing.size()));
}


void CTempoSwingDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
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
				curpos = std::max(minpos, curpos - static_cast<int>(sbInfo.nPage));
			}
			break;

		case SB_PAGERIGHT:		// Scroll one page right.
			if(curpos < maxpos)
			{
				curpos = std::min(maxpos, curpos + static_cast<int>(sbInfo.nPage));
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

	DialogBase::OnVScroll(nSBCode, nPos, pScrollBar);
}


// Scrollable container for the sliders
BEGIN_MESSAGE_MAP(CTempoSwingDlg::SliderContainer, CStatic)
	//{{AFX_MSG_MAP(CTempoSwingDlg::SliderContainer)
	ON_WM_HSCROLL()
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, &CTempoSwingDlg::SliderContainer::OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CTempoSwingDlg::SliderContainer::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
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
		m_parent.m_tempoSwing[i] = m_parent.m_controls[i]->GetValue();
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
		wsprintf(s, _T("%i%%"), Util::muldivr(m_parent.m_tempoSwing[i], 100, TempoSwing::Unity));
		m_parent.m_controls[i]->valueLabel.SetWindowText(s);
	}

	CStatic::OnHScroll(nSBCode, nPos, pScrollBar);
}


BOOL CTempoSwingDlg::SliderContainer::OnToolTipNotify(UINT, NMHDR *pNMHDR, LRESULT *)
{
	TOOLTIPTEXT *pTTT = reinterpret_cast<TOOLTIPTEXT *>(pNMHDR);
	if(!(pTTT->uFlags & TTF_IDISHWND))
		return FALSE;
	CSliderCtrl *slider = dynamic_cast<CSliderCtrl *>(CWnd::FromHandlePermanent(reinterpret_cast<HWND>(pNMHDR->idFrom)));
	if(slider != nullptr)
	{
		int32 val = Util::muldivr(m_parent.m_tempoSwing[GetWindowLongPtr(*slider, GWLP_USERDATA)], 100, TempoSwing::Unity) - 100;
		wsprintf(pTTT->szText, _T("%s%d"), val > 0 ? _T("+") : _T(""), val);
		return TRUE;
	}
	return FALSE;
}


OPENMPT_NAMESPACE_END
