/*
 * ChannelManagerDlg.cpp
 * ---------------------
 * Purpose: Dialog class for moving, removing, managing channels
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "ChannelManagerDlg.h"
#include "HighDPISupport.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "PatternEditorDialogs.h"
#include "resource.h"
#include "UpdateHints.h"
#include "../common/mptStringBuffer.h"

#include <functional>

OPENMPT_NAMESPACE_BEGIN

#define CM_NB_COLS		8
#define CM_BT_HEIGHT	22

///////////////////////////////////////////////////////////
// CChannelManagerDlg

BEGIN_MESSAGE_MAP(CChannelManagerDlg, DialogBase)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_CLOSE()

	ON_COMMAND(IDC_BUTTON1, &CChannelManagerDlg::OnApply)
	ON_COMMAND(IDC_BUTTON2, &CChannelManagerDlg::OnClose)
	ON_COMMAND(IDC_BUTTON3, &CChannelManagerDlg::OnSelectAll)
	ON_COMMAND(IDC_BUTTON4, &CChannelManagerDlg::OnInvert)
	ON_COMMAND(IDC_BUTTON5, &CChannelManagerDlg::OnAction1)
	ON_COMMAND(IDC_BUTTON6, &CChannelManagerDlg::OnAction2)
	ON_COMMAND(IDC_BUTTON7, &CChannelManagerDlg::OnStore)
	ON_COMMAND(IDC_BUTTON8, &CChannelManagerDlg::OnRestore)

	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CChannelManagerDlg::OnTabSelchange)
END_MESSAGE_MAP()

CChannelManagerDlg * CChannelManagerDlg::sharedInstance_ = nullptr;

CChannelManagerDlg * CChannelManagerDlg::sharedInstanceCreate()
{
	try
	{
		if(sharedInstance_ == nullptr)
			sharedInstance_ = new CChannelManagerDlg();
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
	}
	return sharedInstance_;
}

void CChannelManagerDlg::SetDocument(CModDoc *modDoc)
{
	if(modDoc == m_ModDoc)
		return;
	
	m_ModDoc = modDoc;
	ResetState(true, true, true);
	if(m_show)
	{
		if(m_ModDoc)
		{
			ResizeWindow();
			ShowWindow(SW_SHOWNOACTIVATE);  // In case the window was hidden because no module was loaded
			InvalidateRect(m_drawableArea, FALSE);
		} else
		{
			ShowWindow(SW_HIDE);
		}
	}
}

bool CChannelManagerDlg::IsDisplayed() const
{
	return m_show;
}

void CChannelManagerDlg::Update(UpdateHint hint, CObject *pHint)
{
	if(!m_hWnd || !m_show)
		return;
	if(!hint.ToType<GeneralHint>().GetType()[HINT_MODCHANNELS | HINT_MODGENERAL | HINT_MODTYPE | HINT_MPTOPTIONS])
		return;

	const size_t oldNumChannels = m_states.size();
	m_states.resize(m_ModDoc->GetNumChannels());
	for(size_t chn = oldNumChannels; chn < m_states.size(); chn++)
	{
		m_states[chn].sourceChn = static_cast<CHANNELINDEX>(chn);
	}

	ResizeWindow();
	InvalidateRect(nullptr, FALSE);
	if(hint.ToType<GeneralHint>().GetType()[HINT_MODCHANNELS] && m_quickChannelProperties->m_hWnd && pHint != m_quickChannelProperties.get())
		m_quickChannelProperties->UpdateDisplay();
}

void CChannelManagerDlg::Show()
{
	if(!m_hWnd)
	{
		Create(IDD_CHANNELMANAGER, nullptr);
	}
	ResizeWindow();
	ShowWindow(SW_SHOW);
	m_show = true;
}

void CChannelManagerDlg::Hide()
{
	if(m_hWnd != nullptr && m_show)
	{
		ResetState(true, true, true);
		ShowWindow(SW_HIDE);
		m_show = false;
	}
}


CChannelManagerDlg::CChannelManagerDlg()
	: m_quickChannelProperties{std::make_unique<QuickChannelProperties>()}
	, m_buttonHeight{CM_BT_HEIGHT}
{
}

CChannelManagerDlg::~CChannelManagerDlg()
{
	if(this == sharedInstance_)
		sharedInstance_ = nullptr;
	if(m_bkgnd)
		DeleteBitmap(m_bkgnd);
	DestroyWindow();
}

BOOL CChannelManagerDlg::OnInitDialog()
{
	DialogBase::OnInitDialog();

	HWND menu = ::GetDlgItem(m_hWnd, IDC_TAB1);

	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
	tie.pszText = const_cast<LPTSTR>(_T("Solo/Mute"));
	TabCtrl_InsertItem(menu, kSoloMute, &tie);
	tie.pszText = const_cast<LPTSTR>(_T("Record Select"));
	TabCtrl_InsertItem(menu, kRecordSelect, &tie);
	tie.pszText = const_cast<LPTSTR>(_T("Plugins"));
	TabCtrl_InsertItem(menu, kPluginState, &tie);
	tie.pszText = const_cast<LPTSTR>(_T("Reorder/Remove"));
	TabCtrl_InsertItem(menu, kReorderRemove, &tie);
	m_currentTab = kSoloMute;

	m_buttonHeight = HighDPISupport::ScalePixels(CM_BT_HEIGHT, m_hWnd);
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1), SW_HIDE);

	ResetState(true, true, true, true);

	return TRUE;
}


void CChannelManagerDlg::OnDPIChanged()
{
	m_font.DeleteObject();
	ResizeWindow();
	DialogBase::OnDPIChanged();
}


INT_PTR CChannelManagerDlg::OnToolHitTest(CPoint point, TOOLINFO *pTI) const
{
	CRect rect;
	pTI->hwnd = m_hWnd;
	pTI->uId = ButtonHit(point, &rect);
	pTI->rect = rect;
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	if(pTI->uId == CHANNELINDEX_INVALID)
		return -1;
	return pTI->uId;
}


CString CChannelManagerDlg::GetToolTipText(UINT id, HWND hwnd) const
{
	CString text;
	if(hwnd || !m_ModDoc || id >= m_states.size())
		return text;

	const CHANNELINDEX chn = m_states[id].sourceChn;
	const auto &chnSettings = m_ModDoc->GetSoundFile().ChnSettings[chn];
	if(!chnSettings.szName.empty())
		text = MPT_CFORMAT("{}: {}")(chn + 1, mpt::ToWin(m_ModDoc->GetSoundFile().GetCharsetInternal(), chnSettings.szName));
	else
		text = MPT_CFORMAT("Channel {}")(chn + 1);

	switch(m_currentTab)
	{
	case kSoloMute:
		text += chnSettings.dwFlags[CHN_MUTE] ? _T(" (Muted)") : _T(" (Unmuted)");
		break;
	case kRecordSelect:
		switch(m_ModDoc->GetChannelRecordGroup(chn))
		{
		case RecordGroup::NoGroup: text += _T(" (No Record Group)"); break;
		case RecordGroup::Group1: text += _T(" (Record Group 1)"); break;
		case RecordGroup::Group2: text += _T(" (Record Group 2)"); break;
		}
		break;
	case kPluginState:
		text += chnSettings.dwFlags[CHN_NOFX] ? _T(" (Plugins Bypassed)") : _T(" (Plugins Enabled)");
		break;
	case kReorderRemove:
		if(m_states[id].removed)
			text += _T(" (Marked for Removal)");
		break;
	case kNumTabs:
		MPT_ASSERT_NOTREACHED();
		break;
	}
	return text;
}


void CChannelManagerDlg::OnApply()
{
	if(!m_ModDoc) return;

	std::vector<State> newStates;
	std::vector<CHANNELINDEX> newChnOrder;
	newStates.reserve(m_ModDoc->GetNumChannels());
	newChnOrder.reserve(m_ModDoc->GetNumChannels());

	// Count new number of channels, copy pattern pointers & manager internal store memory
	for(const auto &state : m_states)
	{
		if(!state.removed)
		{
			newStates.push_back(state);
			newChnOrder.push_back(state.sourceChn);
		}
	}

	BeginWaitCursor();

	CriticalSection cs;
	if(m_ModDoc->ReArrangeChannels(newChnOrder) != newChnOrder.size())
	{
		cs.Leave();
		EndWaitCursor();
		return;
	}

	// Update manager internal store memory
	m_states = std::move(newStates);
	for(CHANNELINDEX chn = 0; chn < m_states.size(); chn++)
	{
		m_states[chn].sourceChn = chn;
	}

	cs.Leave();
	EndWaitCursor();

	ResetState(true, true, true, true);

	// Update document & windows
	m_ModDoc->SetModified();
	m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels().ModType(), this); //refresh channel headers

	// Redraw channel manager window
	ResizeWindow();
	InvalidateRect(nullptr, FALSE);
}

void CChannelManagerDlg::OnClose()
{
	if(m_bkgnd) DeleteBitmap(m_bkgnd);
	ResetState(true, true, true, true);
	m_bkgnd = nullptr;
	m_show = false;

	DialogBase::OnCancel();
}

void CChannelManagerDlg::OnSelectAll()
{
	for(auto &state : m_states)
		state.select = true;
	InvalidateRect(m_drawableArea, FALSE);
}

void CChannelManagerDlg::OnInvert()
{
	for(auto &state : m_states)
		state.select = !state.select;
	InvalidateRect(m_drawableArea, FALSE);
}

void CChannelManagerDlg::OnAction(uint8 action)
{
	if(!m_show || m_ModDoc == nullptr)
		return;

	int numSelectedAndMatching = 0, numSelected = 0;
	const auto recordGroup = (action == 1 ? RecordGroup::Group1 : RecordGroup::Group2);

	switch(m_currentTab)
	{
		case kSoloMute:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
			{
				CHANNELINDEX sourceChn = m_states[chn].sourceChn;
				if(action == 1)
					m_ModDoc->MuteChannel(sourceChn, !m_states[chn].select);
				else if(m_states[chn].select)
					m_ModDoc->MuteChannel(sourceChn, !m_ModDoc->IsChannelMuted(sourceChn));
			}
			break;
		case kRecordSelect:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
			{
				CHANNELINDEX sourceChn = m_states[chn].sourceChn;
				if(m_states[chn].select)
					numSelected++;
				if(m_states[chn].select && m_ModDoc->GetChannelRecordGroup(sourceChn) == recordGroup)
					numSelectedAndMatching++;
			}
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
			{
				if(m_states[chn].select)
				{
					CHANNELINDEX sourceChn = m_states[chn].sourceChn;
					if(numSelected != numSelectedAndMatching && m_ModDoc->GetChannelRecordGroup(sourceChn) != recordGroup)
						m_ModDoc->SetChannelRecordGroup(sourceChn, recordGroup);
					else if(numSelected == numSelectedAndMatching)
						m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::NoGroup);
				}
			}
			break;
		case kPluginState:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
			{
				CHANNELINDEX sourceChn = m_states[chn].sourceChn;
				if(m_states[chn].select)
					m_ModDoc->NoFxChannel(sourceChn, action == 2);
			}
			break;
		case kReorderRemove:
			if(action == 1)
			{
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					if(m_states[chn].select)
						m_states[chn].removed = !m_states[chn].removed;
				}
			} else
			{
				ResetState(false, false, false, true);
			}
			break;
		case kNumTabs:
			MPT_ASSERT_NOTREACHED();
			break;
	}

	ResetState();

	m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
	InvalidateRect(m_drawableArea, FALSE);
}


void CChannelManagerDlg::OnStore()
{
	if(!m_show || m_ModDoc == nullptr)
		return;

	switch(m_currentTab)
	{
		case kSoloMute:
			for(auto &state : m_states)
				state.memoryMute = m_ModDoc->IsChannelMuted(state.sourceChn);
			break;
		case kRecordSelect:
			for(auto &state : m_states)
				state.memoryRecordGroup = static_cast<uint8>(m_ModDoc->GetChannelRecordGroup(state.sourceChn));
			break;
		case kPluginState:
			for(auto &state : m_states)
				state.memoryNoFx = m_ModDoc->IsChannelNoFx(state.sourceChn);
			break;
		case kReorderRemove:
		default:
			break;
	}
}

void CChannelManagerDlg::OnRestore()
{
	if(!m_show || m_ModDoc == nullptr)
		return;

	switch(m_currentTab)
	{
		case kSoloMute:
			for(auto &state : m_states)
				m_ModDoc->MuteChannel(state.sourceChn, state.memoryMute);
			break;
		case kRecordSelect:
			m_ModDoc->ReinitRecordState();
			for(auto &state : m_states)
				m_ModDoc->SetChannelRecordGroup(state.sourceChn, static_cast<RecordGroup>(state.memoryRecordGroup));
			break;
		case kPluginState:
			for(auto &state : m_states)
				m_ModDoc->NoFxChannel(state.sourceChn, state.memoryNoFx);
			break;
		case kReorderRemove:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				m_states[chn].sourceChn = chn;
			ResetState(false, false, false, true);
			break;
		default:
			break;
	}

	if(m_currentTab != kReorderRemove)
		ResetState();

	m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
	InvalidateRect(m_drawableArea, FALSE);
}

void CChannelManagerDlg::OnTabSelchange(NMHDR* /*header*/, LRESULT* /*pResult*/)
{
	if(!m_show) return;

	m_currentTab = static_cast<Tab>(TabCtrl_GetCurFocus(::GetDlgItem(m_hWnd, IDC_TAB1)));

	switch(m_currentTab)
	{
		case kSoloMute:
			SetDlgItemText(IDC_BUTTON5, _T("S&olo"));
			SetDlgItemText(IDC_BUTTON6, _T("M&ute"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case kRecordSelect:
			SetDlgItemText(IDC_BUTTON5, _T("Instrument &1"));
			SetDlgItemText(IDC_BUTTON6, _T("Instrument &2"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case kPluginState:
			SetDlgItemText(IDC_BUTTON5, _T("&Enable FX"));
			SetDlgItemText(IDC_BUTTON6, _T("&Disable FX"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case kReorderRemove:
			SetDlgItemText(IDC_BUTTON5, _T("R&emove"));
			SetDlgItemText(IDC_BUTTON6, _T("Ca&ncel All"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_SHOW);
			break;
		default:
			break;
	}

	InvalidateRect(m_drawableArea, FALSE);
}


void CChannelManagerDlg::ResizeWindow()
{
	if(!m_hWnd || !m_ModDoc)
		return;

	const int dpi = HighDPISupport::GetDpiForWindow(m_hWnd);

	const auto oldButtonHeight = m_buttonHeight;
	m_buttonHeight = MulDiv(CM_BT_HEIGHT, dpi, 96);

	CHANNELINDEX channels = m_ModDoc->GetNumChannels();
	int lines = channels / CM_NB_COLS + (channels % CM_NB_COLS ? 1 : 0);

	CRect window;
	GetWindowRect(window);

	CRect client;
	GetClientRect(client);
	m_drawableArea = client;
	m_drawableArea.DeflateRect(MulDiv(10, dpi, 96), MulDiv(38, dpi, 96), MulDiv(8, dpi, 96), MulDiv(30, dpi, 96));

	int chnSizeY = m_drawableArea.Height() / lines;

	if(chnSizeY != m_buttonHeight || oldButtonHeight != m_buttonHeight)
	{
		SetWindowPos(nullptr, 0, 0, window.Width(), window.Height() + (m_buttonHeight - chnSizeY) * lines, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

		GetClientRect(client);

		// Move butttons to bottom of the window
		auto dwp = ::BeginDeferWindowPos(6);
		for(auto id : {IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5, IDC_BUTTON6})
		{
			CWnd *button = GetDlgItem(id);
			if(button != nullptr)
			{
				CRect btn;
				button->GetClientRect(btn);
				button->MapWindowPoints(this, btn);
				::DeferWindowPos(dwp, *button, nullptr, btn.left, client.Height() - btn.Height() - MulDiv(3, dpi, 96), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}

		::EndDeferWindowPos(dwp);

		if(m_bkgnd)
		{
			DeleteObject(m_bkgnd);
			m_bkgnd = nullptr;
		}

		m_drawableArea = client;
		m_drawableArea.DeflateRect(MulDiv(10, dpi, 96), MulDiv(38, dpi, 96), MulDiv(8, dpi, 96), MulDiv(30, dpi, 96));
		Invalidate(FALSE);
	}
}


void CChannelManagerDlg::OnPaint()
{
	if(!m_hWnd || !m_show || m_ModDoc == nullptr)
	{
		DialogBase::OnPaint();
		ShowWindow(SW_HIDE);
		return;
	}
	if(IsIconic())
	{
		DialogBase::OnPaint();
		return;
	}

	if(!m_font.m_hObject)
	{
		HighDPISupport::CreateGUIFont(m_font, m_hWnd);
	}

	PAINTSTRUCT pDC;
	::BeginPaint(m_hWnd, &pDC);
	const CRect &rcPaint = pDC.rcPaint;

	const int chnSizeX = m_drawableArea.Width() / CM_NB_COLS;
	const int chnSizeY = m_buttonHeight;

	if(m_currentTab == kReorderRemove && m_moveRect && m_bkgnd)
	{
		// Only draw channels to be moved around
		HDC bdc = ::CreateCompatibleDC(pDC.hdc);
		::SelectObject(bdc, m_bkgnd);
		::BitBlt(pDC.hdc, rcPaint.left, rcPaint.top, rcPaint.Width(), rcPaint.Height(), bdc, rcPaint.left, rcPaint.top, SRCCOPY);

		BLENDFUNCTION ftn;
		ftn.BlendOp = AC_SRC_OVER;
		ftn.BlendFlags = 0;
		ftn.SourceConstantAlpha = 192;
		ftn.AlphaFormat = 0;

		for(const auto &state : m_states)
		{
			if(state.select)
			{
				CRect btn = state.move;
				btn.DeflateRect(3, 3, 0, 0);

				AlphaBlend(pDC.hdc, btn.left + m_moveX - m_downX, btn.top + m_moveY - m_downY, btn.Width(), btn.Height(), bdc,
					btn.left, btn.top, btn.Width(), btn.Height(), ftn);
			}
		}
		::SelectObject(bdc, nullptr);
		::DeleteDC(bdc);

		::EndPaint(m_hWnd, &pDC);
		return;
	}

	CRect client;
	GetClientRect(&client);

	HDC dc = ::CreateCompatibleDC(pDC.hdc);
	if(!m_bkgnd)
		m_bkgnd = ::CreateCompatibleBitmap(pDC.hdc, client.Width(), client.Height());
	HGDIOBJ oldBmp = ::SelectObject(dc, m_bkgnd);
	HGDIOBJ oldFont = ::SelectObject(dc, m_font);

	const auto dcBrush = GetStockBrush(DC_BRUSH);

	const int dpi = HighDPISupport::GetDpiForWindow(m_hWnd);
	client.SetRect(client.left + MulDiv(2, dpi, 96), client.top + MulDiv(32, dpi, 96), client.right - MulDiv(2, dpi, 96), client.bottom - MulDiv(24, dpi, 96));
	// Draw background
	{
		const auto bgIntersected = client & pDC.rcPaint;  // In case of partial redraws, FillRect may still draw into areas that are not part of the redraw area and thus make some buttons disappear
		::FillRect(dc, &pDC.rcPaint, GetSysColorBrush(COLOR_BTNFACE));
		::FillRect(dc, &bgIntersected, GetSysColorBrush(COLOR_HIGHLIGHT));
		::SetDCBrushColor(dc, RGB(20, 20, 20));
		::FrameRect(dc, &client, dcBrush);
	}

	client.SetRect(client.left + 8,client.top + 6,client.right - 6,client.bottom - 6);

	const COLORREF highlight = GetSysColor(COLOR_HIGHLIGHT), red = RGB(192, 96, 96), green = RGB(96, 192, 96), redBright = RGB(218, 163, 163), greenBright = RGB(163, 218, 163);
	const COLORREF brushColors[] = { highlight, green, red };
	const COLORREF brushColorsBright[] = { highlight, greenBright, redBright };
	const auto buttonFaceColor = GetSysColor(COLOR_BTNFACE), windowColor = GetSysColor(COLOR_WINDOW);

	uint32 col = 0, row = 0;
	const CSoundFile &sndFile = m_ModDoc->GetSoundFile();
	CString s;
	for(const auto &state : m_states)
	{
		const CHANNELINDEX sourceChn = state.sourceChn;
		const auto &chnSettings = sndFile.ChnSettings[sourceChn];

		if(!chnSettings.szName.empty())
			s = MPT_CFORMAT("{}: {}")(sourceChn + 1, mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.ChnSettings[sourceChn].szName));
		else
			s = MPT_CFORMAT("Channel {}")(sourceChn + 1);

		const int border = HighDPISupport::ScalePixels(3, m_hWnd);
		CRect btn;
		btn.left = client.left + col * chnSizeX + border;
		btn.right = btn.left + chnSizeX - border;
		btn.top = client.top + row * chnSizeY + border;
		btn.bottom = btn.top + chnSizeY - border;
		col++;
		if(col >= CM_NB_COLS)
		{
			col = 0;
			row++;
		}

		if(!CRect{}.IntersectRect(&pDC.rcPaint, &btn))
			continue;

		// Button
		const bool activate = state.select;
		const bool enable = !state.removed;
		auto btnAdjusted = btn;  // Without border
		::DrawEdge(dc, btnAdjusted, enable ? EDGE_RAISED : EDGE_SUNKEN, BF_RECT | BF_MIDDLE | BF_ADJUST);
		if(activate)
			::FillRect(dc, btnAdjusted, GetSysColorBrush(COLOR_WINDOW));

		if(chnSettings.color != ModChannelSettings::INVALID_COLOR)
		{
			// Channel color
			const auto startColor = chnSettings.color;
			const auto endColor = activate ? windowColor : buttonFaceColor;
			const auto width = btnAdjusted.Width() / 2;
			auto rect = btnAdjusted;
			rect.left = rect.right - width;
			rect.right = rect.left + 1;
			for(int i = 0; i < width; i++)
			{
				auto blend = static_cast<double>(i) / width, blendInv = 1.0 - blend;
				auto blendColor = RGB(mpt::saturate_round<uint8>(GetRValue(startColor) * blend + GetRValue(endColor) * blendInv),
				                      mpt::saturate_round<uint8>(GetGValue(startColor) * blend + GetGValue(endColor) * blendInv),
				                      mpt::saturate_round<uint8>(GetBValue(startColor) * blend + GetBValue(endColor) * blendInv));
				::SetDCBrushColor(dc, blendColor);
				::FillRect(dc, &rect, dcBrush);
				rect.left++;
				rect.right++;
			}
		}

		// Text
		{
			auto rect = btnAdjusted;
			rect.left += HighDPISupport::ScalePixels(9, m_hWnd);
			rect.right -= HighDPISupport::ScalePixels(3, m_hWnd);

			::SetBkMode(dc, TRANSPARENT);
			::SetTextColor(dc, GetSysColor(enable || activate ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
			::DrawText(dc, s, -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}

		// Draw red/green markers
		{
			const int margin = HighDPISupport::ScalePixels(1, m_hWnd);
			auto rect = btnAdjusted;
			rect.DeflateRect(margin, margin);
			rect.right = rect.left + HighDPISupport::ScalePixels(7, m_hWnd);
			const auto &brushes = activate ? brushColorsBright : brushColors;
			const auto redBrush = brushes[2], greenBrush = brushes[1];
			COLORREF color = 0;
			switch(m_currentTab)
			{
			case kSoloMute:
				color = chnSettings.dwFlags[CHN_MUTE] ? redBrush : greenBrush;
				break;
			case kRecordSelect:
				color = brushColors[static_cast<size_t>(m_ModDoc->GetChannelRecordGroup(sourceChn)) % std::size(brushColors)];
				break;
			case kPluginState:
				color = chnSettings.dwFlags[CHN_NOFX] ? redBrush : greenBrush;
				break;
			case kReorderRemove:
				color = state.removed ? redBrush : greenBrush;
				break;
			case kNumTabs:
				MPT_ASSERT_NOTREACHED();
				break;
			}
			::SetDCBrushColor(dc, color);
			::FillRect(dc, rect, dcBrush);
			// Draw border around marker
			::SetDCBrushColor(dc, RGB(20, 20, 20));
			::FrameRect(dc, rect, dcBrush);
		}
	}

	::BitBlt(pDC.hdc, rcPaint.left, rcPaint.top, rcPaint.Width(), rcPaint.Height(), dc, rcPaint.left, rcPaint.top, SRCCOPY);
	::SelectObject(dc, oldFont);
	::SelectObject(dc, oldBmp);
	::DeleteDC(dc);

	::EndPaint(m_hWnd, &pDC);
}


CHANNELINDEX CChannelManagerDlg::ButtonHit(CPoint point, CRect *invalidate) const
{
	const CRect &client = m_drawableArea;

	if(PtInRect(client, point) && m_ModDoc != nullptr)
	{
		UINT nColns = CM_NB_COLS;

		int x = point.x - client.left;
		int y = point.y - client.top;

		int dx = client.Width() / (int)nColns;
		int dy = m_buttonHeight;

		x = x / dx;
		y = y / dy;
		CHANNELINDEX n = static_cast<CHANNELINDEX>(y * nColns + x);
		if(n < m_ModDoc->GetNumChannels())
		{
			if(invalidate)
			{
				invalidate->left = client.left + x * dx;
				invalidate->right = invalidate->left + dx;
				invalidate->top = client.top + y * dy;
				invalidate->bottom = invalidate->top + dy;
			}
			return n;
		}
	}
	return CHANNELINDEX_INVALID;
}


void CChannelManagerDlg::ResetState(bool bSelection, bool bMove, bool bInternal, bool bOrder)
{
	size_t oldSize = m_states.size();
	m_states.resize(m_ModDoc ? m_ModDoc->GetNumChannels() : 0);
	for(CHANNELINDEX chn = 0; chn < m_states.size(); chn++)
	{
		if(bSelection)
			m_states[chn].select = false;
		if(bOrder || chn > oldSize)
		{
			m_states[chn].sourceChn = chn;
			m_states[chn].removed = false;
		}
	}
	if(bMove || bInternal)
	{
		m_leftButton = false;
		m_rightButton = false;
	}
	if(bMove)
		m_moveRect = false;
}


void CChannelManagerDlg::OnMouseMove(UINT nFlags,CPoint point)
{
	if(!m_hWnd || m_show == false) return;

	if(!m_leftButton && !m_rightButton)
	{
		m_moveX = point.x;
		m_moveY = point.y;
		return;
	}
	MouseEvent(nFlags, point, m_moveRect ? CM_BT_NONE : (m_leftButton ? CM_BT_LEFT : CM_BT_RIGHT));
}

void CChannelManagerDlg::OnLButtonUp(UINT /*nFlags*/,CPoint point)
{
	ReleaseCapture();
	if(!m_hWnd || !m_show)
		return;

	if(m_moveRect && m_ModDoc)
	{
		CRect dropRect;
		CHANNELINDEX dropChn = ButtonHit(point, &dropRect);
		if(dropChn != CHANNELINDEX_INVALID)
		{
			// Rearrange channels
			const auto numChannels = m_ModDoc->GetNumChannels();
			if(point.x > dropRect.left + dropRect.Width() / 2 && dropChn < numChannels)
				dropChn++;

			std::vector<State> states;
			CHANNELINDEX selectedBeforeDropChn = 0;
			for(CHANNELINDEX chn = 0; chn < numChannels; chn++)
			{
				if(m_states[chn].select)
				{
					states.push_back(m_states[chn]);
					states.back().select = false;
					if(chn < dropChn)
						selectedBeforeDropChn++;
				}
			}

			// Remove all selected channels from the order
			const auto IsSelected = [](const State &state) { return state.select; };
			m_states.erase(std::remove_if(m_states.begin(), m_states.end(), IsSelected), m_states.end());
			// Then insert them at the drop position
			m_states.insert(m_states.begin() + dropChn - selectedBeforeDropChn, states.begin(), states.end());
		} else
		{
			ResetState(true, false, false, false);
		}

		m_moveRect = false;
		InvalidateRect(m_drawableArea, FALSE);
		if(m_ModDoc) m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
	}

	m_leftButton = false;
}

void CChannelManagerDlg::OnLButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || m_show == false) return;
	SetCapture();

	if(ButtonHit(point) == CHANNELINDEX_INVALID)
		ResetState(true, false, false);

	m_leftButton = true;
	m_buttonAction = kUndetermined;
	MouseEvent(nFlags,point,CM_BT_LEFT);
	m_downX = point.x;
	m_downY = point.y;
}

void CChannelManagerDlg::OnRButtonUp(UINT /*nFlags*/,CPoint /*point*/)
{
	ReleaseCapture();
	m_rightButton = false;
}

void CChannelManagerDlg::OnRButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || m_show == false) return;
	SetCapture();

	m_rightButton = true;
	m_buttonAction = kUndetermined;
	if(m_moveRect)
	{
		ResetState(true, true, false, false);
		InvalidateRect(m_drawableArea, FALSE);
	} else
	{
		MouseEvent(nFlags, point, CM_BT_RIGHT);
		m_downX = point.x;
		m_downY = point.y;
	}
}

void CChannelManagerDlg::OnMButtonDown(UINT /*nFlags*/, CPoint point)
{
	CRect rect;
	CHANNELINDEX chn = ButtonHit(point, &rect);
	if(m_ModDoc != nullptr && chn != CHANNELINDEX_INVALID)
	{
		ClientToScreen(&point);
		m_quickChannelProperties->Show(m_ModDoc, m_states[chn].sourceChn, point);
	}
}

void CChannelManagerDlg::MouseEvent(UINT nFlags,CPoint point, MouseButton button)
{
	if(!m_ModDoc)
		return;

	m_moveX = point.x;
	m_moveY = point.y;

	CRect client, invalidate;
	const CHANNELINDEX n = ButtonHit(point, &invalidate);
	if(n != CHANNELINDEX_INVALID && button != CM_BT_NONE)
	{
		const CHANNELINDEX sourceChn = m_states[n].sourceChn;
		if(nFlags & MK_CONTROL)
		{
			if(button == CM_BT_LEFT)
			{
				if(!m_states[n].select && !m_states[n].removed)
					m_states[n].move = invalidate;
				m_states[n].select = true;
			} else if(button == CM_BT_RIGHT)
			{
				m_states[n].select = false;
			}
		} else
		{
			switch(m_currentTab)
			{
			case kSoloMute:
				if(button == CM_BT_LEFT)
				{
					if(m_buttonAction == kUndetermined)
					{
						bool isAlreadySolo = true;
						for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
						{
							if((chn == sourceChn) == m_ModDoc->IsChannelMuted(chn))
							{
								isAlreadySolo = false;
								break;
							}
						}
						m_buttonAction = isAlreadySolo ? kAction2 : kAction1;
						for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
						{
							m_ModDoc->MuteChannel(chn, m_buttonAction == kAction1);
						}
					}
					if(m_buttonAction == kAction1)
						m_ModDoc->MuteChannel(sourceChn, false);
					invalidate = client = m_drawableArea;
				} else
				{
					if(m_buttonAction == kUndetermined)
						m_buttonAction = m_ModDoc->IsChannelMuted(sourceChn) ? kAction1 : kAction2;
					m_ModDoc->MuteChannel(sourceChn, m_buttonAction == kAction2);
				}
				m_ModDoc->SetModified();
				m_ModDoc->UpdateAllViews(nullptr, GeneralHint(sourceChn).Channels(), this);
				break;
			case kRecordSelect:
				if(m_buttonAction == kUndetermined)
				{
					auto rec = m_ModDoc->GetChannelRecordGroup(sourceChn);
					m_buttonAction = (rec == RecordGroup::NoGroup || rec != (button == CM_BT_LEFT ? RecordGroup::Group1 : RecordGroup::Group2)) ? kAction1 : kAction2;
				}

				if(m_buttonAction == kAction1 && button == CM_BT_LEFT)
					m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::Group1);
				else if(m_buttonAction == kAction1 && button == CM_BT_RIGHT)
					m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::Group2);
				else
					m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::NoGroup);
				m_ModDoc->UpdateAllViews(nullptr, GeneralHint(sourceChn).Channels(), this);
				break;
			case kPluginState:
				m_ModDoc->NoFxChannel(sourceChn, (button != CM_BT_LEFT));
				m_ModDoc->SetModified();
				m_ModDoc->UpdateAllViews(nullptr, GeneralHint(sourceChn).Channels(), this);
				break;
			case kReorderRemove:
				if(button == CM_BT_LEFT)
				{
					m_states[n].move = invalidate;
					m_states[n].select = true;
				} else if(button == CM_BT_RIGHT)
				{
					if(m_buttonAction == kUndetermined)
						m_buttonAction = m_states[n].removed ? kAction1 : kAction2;
					m_states[n].select = false;
					m_states[n].removed = (m_buttonAction == kAction2);
				}

				if(m_states[n].select || button == CM_BT_NONE)
				{
					m_moveRect = true;
				}
				break;
			case kNumTabs:
				MPT_ASSERT_NOTREACHED();
				break;
			}
		}

		InvalidateRect(invalidate, FALSE);
	} else
	{
		InvalidateRect(m_drawableArea, FALSE);
	}
}


void CChannelManagerDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);
	DialogBase::OnLButtonDblClk(nFlags, point);
}

void CChannelManagerDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	OnRButtonDown(nFlags, point);
	DialogBase::OnRButtonDblClk(nFlags, point);
}


OPENMPT_NAMESPACE_END
