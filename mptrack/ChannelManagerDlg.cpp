/*
 * ChannelManagerDlg.cpp
 * ---------------------
 * Purpose: Dialog class for moving, removing, managing channels
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Moddoc.h"
#include "Mainfrm.h"
#include "ChannelManagerDlg.h"
#include "dlg_misc.h"
#include "../common/mptStringBuffer.h"


OPENMPT_NAMESPACE_BEGIN

#define CM_NB_COLS		8
#define CM_BT_HEIGHT	22

///////////////////////////////////////////////////////////
// CChannelManagerDlg

BEGIN_MESSAGE_MAP(CChannelManagerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_CLOSE()

	ON_COMMAND(IDC_BUTTON1,	&CChannelManagerDlg::OnApply)
	ON_COMMAND(IDC_BUTTON2,	&CChannelManagerDlg::OnClose)
	ON_COMMAND(IDC_BUTTON3,	&CChannelManagerDlg::OnSelectAll)
	ON_COMMAND(IDC_BUTTON4,	&CChannelManagerDlg::OnInvert)
	ON_COMMAND(IDC_BUTTON5,	&CChannelManagerDlg::OnAction1)
	ON_COMMAND(IDC_BUTTON6,	&CChannelManagerDlg::OnAction2)
	ON_COMMAND(IDC_BUTTON7,	&CChannelManagerDlg::OnStore)
	ON_COMMAND(IDC_BUTTON8,	&CChannelManagerDlg::OnRestore)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1,	&CChannelManagerDlg::OnTabSelchange)
	
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
END_MESSAGE_MAP()

CChannelManagerDlg * CChannelManagerDlg::sharedInstance_ = nullptr;

CChannelManagerDlg * CChannelManagerDlg::sharedInstanceCreate()
{
	try
	{
		if(sharedInstance_ == nullptr)
			sharedInstance_ = new CChannelManagerDlg();
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
	}
	return sharedInstance_;
}

void CChannelManagerDlg::SetDocument(CModDoc *modDoc)
{
	if(modDoc != m_ModDoc)
	{
		m_ModDoc = modDoc;
		ResetState(true, true, true, true, false);
		if(m_show)
		{
			if(m_ModDoc)
			{
				ResizeWindow();
				ShowWindow(SW_SHOWNOACTIVATE);	// In case the window was hidden because no module was loaded
				InvalidateRect(m_drawableArea, FALSE);
			} else
			{
				ShowWindow(SW_HIDE);
			}
		}
	}
}

bool CChannelManagerDlg::IsDisplayed() const
{
	return m_show;
}

void CChannelManagerDlg::Update()
{
	if(!m_hWnd || m_show == false) return;
	ResizeWindow();
	InvalidateRect(nullptr, FALSE);
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
		ResetState(true, true, true, true, true);
		ShowWindow(SW_HIDE);
		m_show = false;
	}
}


CChannelManagerDlg::CChannelManagerDlg()
	: m_drawableArea(0, 0, 0, 0)
	, m_buttonHeight(CM_BT_HEIGHT)
	, m_leftButton(false)
	, m_rightButton(false)
	, m_moveRect(false)
	, m_show(false)
{
	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
	{
		pattern[nChn] = nChn;
		removed[nChn] = false;
		select[nChn] = false;
		state[nChn] = false;
		memory[0][nChn] = 0;
		memory[1][nChn] = 0;
		memory[2][nChn] = 0;
		memory[3][nChn] = nChn;
	}
}

CChannelManagerDlg::~CChannelManagerDlg()
{
	if(this == sharedInstance_) sharedInstance_ = nullptr;
	if(m_bkgnd) DeleteBitmap(m_bkgnd);
	DestroyWindow();
}

BOOL CChannelManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	HWND menu = ::GetDlgItem(m_hWnd, IDC_TAB1);

	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
	tie.pszText = const_cast<LPTSTR>(_T("Solo/Mute"));
	TabCtrl_InsertItem(menu, kSoloMute, &tie);
	tie.pszText = const_cast<LPTSTR>(_T("Record select"));
	TabCtrl_InsertItem(menu, kRecordSelect, &tie);
	tie.pszText = const_cast<LPTSTR>(_T("Plugins"));
	TabCtrl_InsertItem(menu, kPluginState, &tie);
	tie.pszText = const_cast<LPTSTR>(_T("Reorder/Remove"));
	TabCtrl_InsertItem(menu, kReorderRemove, &tie);
	m_currentTab = kSoloMute;

	m_buttonHeight = MulDiv(CM_BT_HEIGHT, Util::GetDPIy(m_hWnd), 96);
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1), SW_HIDE);

	return TRUE;
}

void CChannelManagerDlg::OnApply()
{
	if(!m_ModDoc) return;

	CHANNELINDEX nChannels, newMemory[4][MAX_BASECHANNELS];
	std::vector<CHANNELINDEX> newChnOrder;
	newChnOrder.reserve(m_ModDoc->GetNumChannels());

	// Count new number of channels, copy pattern pointers & manager internal store memory
	nChannels = 0;
	for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
	{
		if(!removed[pattern[nChn]])
		{
			newMemory[0][nChannels] = memory[0][nChannels];
			newMemory[1][nChannels] = memory[1][nChannels];
			newMemory[2][nChannels] = memory[2][nChannels];
			newChnOrder.push_back(pattern[nChn]);
			nChannels++;
		}
	}

	BeginWaitCursor();

	//Creating new order-vector for ReArrangeChannels.
	CriticalSection cs;
	if(m_ModDoc->ReArrangeChannels(newChnOrder) != nChannels)
	{
		cs.Leave();
		EndWaitCursor();
		return;
	}
	

	// Update manager internal store memory
	for(CHANNELINDEX nChn = 0; nChn < nChannels; nChn++)
	{
		CHANNELINDEX newChn = newChnOrder[nChn];
		if(nChn != newChn)
		{
			memory[0][nChn] = newMemory[0][newChn];
			memory[1][nChn] = newMemory[1][newChn];
			memory[2][nChn] = newMemory[2][newChn];
		}
		memory[3][nChn] = nChn;
	}


	cs.Leave();
	EndWaitCursor();

	ResetState(true, true, true, true, true);

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
	ResetState(true, true, true, true, true);
	m_bkgnd = nullptr;
	m_show = false;

	CDialog::OnCancel();
}

void CChannelManagerDlg::OnSelectAll()
{
	if(m_ModDoc)
		for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			select[nChn] = true;

	InvalidateRect(m_drawableArea, FALSE);
}

void CChannelManagerDlg::OnInvert()
{
	if(m_ModDoc)
		for(CHANNELINDEX nChn = 0 ; nChn < m_ModDoc->GetNumChannels() ; nChn++)
			select[nChn] = !select[nChn];
	InvalidateRect(m_drawableArea, FALSE);
}

void CChannelManagerDlg::OnAction1()
{
	if(m_ModDoc)
	{
		int nbOk = 0, nbSelect = 0;
		
		switch(m_currentTab)
		{
			case kSoloMute:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						if(select[nThisChn] && m_ModDoc->IsChannelSolo(nThisChn)) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn])
					{
						if(m_ModDoc->IsChannelMuted(nThisChn)) m_ModDoc->MuteChannel(nThisChn, false);
						if(nbSelect == nbOk) m_ModDoc->SoloChannel(nThisChn, !m_ModDoc->IsChannelSolo(nThisChn));
						else m_ModDoc->SoloChannel(nThisChn, true);
					}
					else if(!m_ModDoc->IsChannelSolo(nThisChn)) m_ModDoc->MuteChannel(nThisChn, true);
				}
				break;
			case kRecordSelect:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						BYTE rec = m_ModDoc->IsChannelRecord(nThisChn);
						if(select[nThisChn] && rec == 1) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn] && select[nThisChn])
					{
						if(select[nThisChn] && nbSelect != nbOk && m_ModDoc->IsChannelRecord(nThisChn) != 1) m_ModDoc->Record1Channel(nThisChn);
						else if(nbSelect == nbOk) m_ModDoc->Record1Channel(nThisChn, false);
					}
				}
				break;
			case kPluginState:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn]) m_ModDoc->NoFxChannel(nThisChn, false);
				}
				break;
			case kReorderRemove:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn]) removed[nThisChn] = !removed[nThisChn];
				}
				break;
			default:
				break;
		}


		ResetState();

		m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
		InvalidateRect(m_drawableArea, FALSE);
	}
}

void CChannelManagerDlg::OnAction2()
{
	if(m_ModDoc)
	{
		
		int nbOk = 0, nbSelect = 0;
		
		switch(m_currentTab)
		{
			case kSoloMute:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						if(select[nThisChn] && m_ModDoc->IsChannelMuted(nThisChn)) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn])
					{
						if(m_ModDoc->IsChannelSolo(nThisChn)) m_ModDoc->SoloChannel(nThisChn, false);
						if(nbSelect == nbOk) m_ModDoc->MuteChannel(nThisChn, !m_ModDoc->IsChannelMuted(nThisChn));
						else m_ModDoc->MuteChannel(nThisChn, true);
					}
				}
				break;
			case kRecordSelect:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						BYTE rec = m_ModDoc->IsChannelRecord(nThisChn);
						if(select[nThisChn] && rec == 2) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn] && select[nThisChn])
					{
						if(select[nThisChn] && nbSelect != nbOk && m_ModDoc->IsChannelRecord(nThisChn) != 2) m_ModDoc->Record2Channel(nThisChn);
						else if(nbSelect == nbOk) m_ModDoc->Record2Channel(nThisChn, false);
					}
				}
				break;
			case kPluginState:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn]) m_ModDoc->NoFxChannel(nThisChn, true);
				}
				break;
			case kReorderRemove:
				ResetState(false, false, false, false, true);
				break;
			default:
				break;
		}

		if(m_currentTab != 3) ResetState();

		m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
		InvalidateRect(m_drawableArea, FALSE);
	}
}

void CChannelManagerDlg::OnStore(void)
{
	if(!m_show || m_ModDoc == nullptr)
	{
		return;
	}

	switch(m_currentTab)
	{
		case kSoloMute:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			{
				CHANNELINDEX nThisChn = pattern[nChn];
				memory[0][nThisChn] = 0;
				if(m_ModDoc->IsChannelMuted(nThisChn)) memory[0][nChn] |= 1;
				if(m_ModDoc->IsChannelSolo(nThisChn))  memory[0][nChn] |= 2;
			}
			break;
		case kRecordSelect:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				memory[1][nChn] = m_ModDoc->IsChannelRecord(pattern[nChn]);
			break;
		case kPluginState:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				memory[2][nChn] = m_ModDoc->IsChannelNoFx(pattern[nChn]);
			break;
		case kReorderRemove:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				memory[3][nChn] = pattern[nChn];
			break;
		default:
			break;
	}
}

void CChannelManagerDlg::OnRestore(void)
{
	if(!m_show || m_ModDoc == nullptr)
	{
		return;
	}

	switch(m_currentTab)
	{
		case kSoloMute:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			{
				CHANNELINDEX nThisChn = pattern[nChn];
				m_ModDoc->MuteChannel(nThisChn, (memory[0][nChn] & 1) != 0);
				m_ModDoc->SoloChannel(nThisChn, (memory[0][nChn] & 2) != 0);
			}
			break;
		case kRecordSelect:
			m_ModDoc->ReinitRecordState(true);
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			{
				if(memory[1][nChn] != 2) m_ModDoc->Record1Channel(pattern[nChn], memory[1][nChn] == 1);
				if(memory[1][nChn] != 1) m_ModDoc->Record2Channel(pattern[nChn], memory[1][nChn] == 2);
			}
			break;
		case kPluginState:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				m_ModDoc->NoFxChannel(pattern[nChn], memory[2][nChn] != 0);
			break;
		case kReorderRemove:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				pattern[nChn] = memory[3][nChn];
			ResetState(false, false, false, false, true);
			break;
		default:
			break;
	}

	if(m_currentTab != 3) ResetState();

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
			SetDlgItemText(IDC_BUTTON5, _T("Solo"));
			SetDlgItemText(IDC_BUTTON6, _T("Mute"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case kRecordSelect:
			SetDlgItemText(IDC_BUTTON5, _T("Instrument 1"));
			SetDlgItemText(IDC_BUTTON6, _T("Instrument 2"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case kPluginState:
			SetDlgItemText(IDC_BUTTON5, _T("Enable FX"));
			SetDlgItemText(IDC_BUTTON6, _T("Disable FX"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case kReorderRemove:
			SetDlgItemText(IDC_BUTTON5, _T("Remove"));
			SetDlgItemText(IDC_BUTTON6, _T("Cancel All"));
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_SHOW);
			break;
		default:
			break;
	}

	InvalidateRect(m_drawableArea, FALSE);
}


void CChannelManagerDlg::DrawChannelButton(HDC hdc, CRect rect, const TCHAR *text, bool activate, bool enable, DWORD dwFlags)
{
	DrawEdge(hdc, rect, enable ? EDGE_RAISED : EDGE_SUNKEN, BF_RECT | BF_MIDDLE | BF_ADJUST);
	if(activate)
	{
		::FillRect(hdc, rect, GetSysColorBrush(COLOR_WINDOW));
	}

	rect.left += Util::ScalePixels(13, m_hWnd);
	rect.right -= Util::ScalePixels(5, m_hWnd);

	::SetBkMode(hdc, TRANSPARENT);
	HGDIOBJ oldfont = ::SelectObject(hdc, CMainFrame::GetGUIFont());

	::SetTextColor(hdc, GetSysColor(enable || activate ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
	::DrawText(hdc, text, -1, &rect, dwFlags | DT_SINGLELINE | DT_NOPREFIX);
	::SelectObject(hdc, oldfont);
}


void CChannelManagerDlg::ResizeWindow()
{
	if(!m_hWnd || !m_ModDoc) return;

	const int dpiX = Util::GetDPIx(m_hWnd);
	const int dpiY = Util::GetDPIy(m_hWnd);

	m_buttonHeight = MulDiv(CM_BT_HEIGHT, dpiY, 96);

	CHANNELINDEX channels = m_ModDoc->GetNumChannels();
	int lines = channels / CM_NB_COLS + (channels % CM_NB_COLS ? 1 : 0);

	CRect window;
	GetWindowRect(window);

	CRect client;
	GetClientRect(client);
	m_drawableArea = client;
	m_drawableArea.DeflateRect(MulDiv(10, dpiX, 96), MulDiv(38, dpiY, 96), MulDiv(8, dpiX, 96), MulDiv(30, dpiY, 96));

	int chnSizeY = m_drawableArea.Height() / lines;

	if(chnSizeY != m_buttonHeight)
	{
		SetWindowPos(nullptr, 0, 0, window.Width(), window.Height() + (m_buttonHeight - chnSizeY) * lines, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

		GetClientRect(client);

		// Move butttons to bottom of the window
		for(auto id : { IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5, IDC_BUTTON6 })
		{
			CWnd *button = GetDlgItem(id);
			if(button != nullptr)
			{
				CRect btn;
				button->GetClientRect(btn);
				button->MapWindowPoints(this, btn);
				button->SetWindowPos(nullptr, btn.left, client.Height() - btn.Height() - MulDiv(3, dpiY, 96), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			}
		}

		if(m_bkgnd)
		{
			DeleteObject(m_bkgnd);
			m_bkgnd = nullptr;
		}

		m_drawableArea = client;
		m_drawableArea.DeflateRect(MulDiv(10, dpiX, 96), MulDiv(38, dpiY, 96), MulDiv(8, dpiX, 96), MulDiv(30, dpiY, 96));
		InvalidateRect(nullptr, FALSE);
	}
}


void CChannelManagerDlg::OnPaint()
{
	if(!m_hWnd || !m_show || m_ModDoc == nullptr)
	{
		CDialog::OnPaint();
		ShowWindow(SW_HIDE);
		return;
	}
	if(IsIconic())
	{
		CDialog::OnPaint();
		return;
	}

	const int dpiX = Util::GetDPIx(m_hWnd);
	const int dpiY = Util::GetDPIy(m_hWnd);
	const CHANNELINDEX channels = m_ModDoc->GetNumChannels();

	PAINTSTRUCT pDC;
	::BeginPaint(m_hWnd, &pDC);
	const CRect &rcPaint = pDC.rcPaint;

	const int chnSizeX = m_drawableArea.Width() / CM_NB_COLS;
	const int chnSizeY = m_buttonHeight;

	if(m_currentTab == 3 && m_moveRect && m_bkgnd)
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

		for(CHANNELINDEX nChn = 0; nChn < channels; nChn++)
		{
			CHANNELINDEX nThisChn = pattern[nChn];
			if(select[nThisChn])
			{
				CRect btn = move[nThisChn];
				btn.DeflateRect(3, 3, 0, 0);

				AlphaBlend(pDC.hdc, btn.left + m_moveX - m_downX, btn.top + m_moveY - m_downY, btn.Width(), btn.Height(), bdc,
					btn.left, btn.top, btn.Width(), btn.Height(), ftn);
			}
		}
		::SelectObject(bdc, (HBITMAP)NULL);
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
	const auto blackBrush = (HBRUSH)::GetStockObject(BLACK_BRUSH);

	client.SetRect(client.left + MulDiv(2, dpiX, 96), client.top + MulDiv(32, dpiY, 96), client.right - MulDiv(2, dpiX, 96), client.bottom - MulDiv(24, dpiY, 96));
	// Draw background
	{
		FillRect(dc, &pDC.rcPaint, GetSysColorBrush(COLOR_BTNFACE));
		FillRect(dc, &client, GetSysColorBrush(COLOR_HIGHLIGHT));
		FrameRect(dc, &client, blackBrush);
	}

	client.SetRect(client.left + 8,client.top + 6,client.right - 6,client.bottom - 6);

	auto highlight = GetSysColorBrush(COLOR_HIGHLIGHT), red = CreateSolidBrush(RGB(192, 96, 96)), green = CreateSolidBrush(RGB(96, 192, 96));
	const HBRUSH brushes[] = {highlight, green, red};

	UINT c = 0, l = 0;
	const CSoundFile &sndFile = m_ModDoc->GetSoundFile();
	CString s;
	for(CHANNELINDEX nChn = 0; nChn < channels; nChn++)
	{
		CHANNELINDEX nThisChn = pattern[nChn];

		CString fmt = !sndFile.ChnSettings[nThisChn].szName.empty() ? _T("%1: %2") : _T("Channel %1");
		s = mpt::cformat(fmt)(nThisChn + 1, mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.ChnSettings[nThisChn].szName));

		const int borderX = MulDiv(3, dpiX, 96), borderY = MulDiv(3, dpiY, 96);
		CRect btn;
		btn.left = client.left + c * chnSizeX + borderX;
		btn.right = btn.left + chnSizeX - borderX;
		btn.top = client.top + l * chnSizeY + borderY;
		btn.bottom = btn.top + chnSizeY - borderY;

		CRect intersection;
		if(intersection.IntersectRect(&pDC.rcPaint, &btn))
			DrawChannelButton(dc, btn, s, select[nThisChn], !removed[nThisChn], DT_RIGHT | DT_VCENTER);

		btn.right = btn.left + chnSizeX / 7;

		btn.DeflateRect(borderX, borderY, borderX, borderY);

		// Draw red/green markers
		switch(m_currentTab)
		{
			case kSoloMute:
				FillRect(dc, btn, sndFile.ChnSettings[nThisChn].dwFlags[CHN_MUTE] ? red : green);
				break;
			case kRecordSelect:
			{
				auto r = m_ModDoc->IsChannelRecord(nThisChn);
				FillRect(dc, btn, brushes[r % std::size(brushes)]);
				break;
			}
			case kPluginState:
				FillRect(dc, btn, sndFile.ChnSettings[nThisChn].dwFlags[CHN_NOFX] ? red : green);
				break;
			case kReorderRemove:
				FillRect(dc, btn, removed[nThisChn] ? red : green);
				break;
		}
		// Draw border around marker
		FrameRect(dc, btn, blackBrush);

		c++;
		if(c >= CM_NB_COLS) { c = 0; l++; }
	}

	::BitBlt(pDC.hdc, rcPaint.left, rcPaint.top, rcPaint.Width(), rcPaint.Height(), dc, rcPaint.left, rcPaint.top, SRCCOPY);
	::SelectObject(dc, oldBmp);
	::DeleteDC(dc);

	DeleteBrush(green);
	DeleteBrush(red);

	::EndPaint(m_hWnd, &pDC);
}


bool CChannelManagerDlg::ButtonHit(CPoint point, CHANNELINDEX *id, CRect *invalidate) const
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
			if(id) *id = n;
			if(invalidate)
			{
				invalidate->left = client.left + x * dx;
				invalidate->right = invalidate->left + dx;
				invalidate->top = client.top + y * dy;
				invalidate->bottom = invalidate->top + dy;
			}
			return true;
		}
	}
	return false;
}


void CChannelManagerDlg::ResetState(bool bSelection, bool bMove, bool bButton, bool bInternal, bool bOrder)
{
	for(CHANNELINDEX chn = 0; chn < MAX_BASECHANNELS; chn++)
	{
		if(bSelection)
			select[pattern[chn]] = false;
		if(bButton)
			state[pattern[chn]]  = false;
		if(bOrder)
		{
			pattern[chn] = chn;
			removed[chn] = false;
		}
	}
	if(bMove || bInternal)
	{
		m_leftButton = false;
		m_rightButton = false;
	}
	if(bMove) m_moveRect = false;
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
	if(!m_hWnd || m_show == false) return;

	if(m_moveRect && m_ModDoc)
	{
		CHANNELINDEX n, i, k;
		CHANNELINDEX newpat[MAX_BASECHANNELS];

		k = CHANNELINDEX_INVALID;
		bool hit = ButtonHit(point, &n, nullptr);
		for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			if(k == CHANNELINDEX_INVALID && select[pattern[nChn]]) k = nChn;

		if(hit && k != CHANNELINDEX_INVALID)
		{
			i = 0;
			k = 0;
			while(i < n)
			{
				while(i < n && select[pattern[i]]) i++;
				if(i < n && !select[pattern[i]])
				{
					newpat[k] = pattern[i];
					pattern[i] = CHANNELINDEX_INVALID;
					k++;
					i++;
				}
			}
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels() ; nChn++)
			{
				if(pattern[nChn] != CHANNELINDEX_INVALID && select[pattern[nChn]])
				{
					newpat[k] = pattern[nChn];
					pattern[nChn] = CHANNELINDEX_INVALID;
					k++;
				}
			}
			i = 0;
			while(i < m_ModDoc->GetNumChannels())
			{
				while(i < m_ModDoc->GetNumChannels() && pattern[i] == CHANNELINDEX_INVALID) i++;
				if(i < m_ModDoc->GetNumChannels() && pattern[i] != CHANNELINDEX_INVALID)
				{
					newpat[k] = pattern[i];
					k++;
					i++;
				}
			}
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			{
				pattern[nChn] = newpat[nChn];
				select[nChn] = false;
			}
		} else
		{
			ResetState(true, false, false, false, false);
		}

		m_moveRect = false;
		InvalidateRect(m_drawableArea, FALSE);
		if(m_ModDoc) m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
	}

	m_leftButton = false;

	for(CHANNELINDEX chn : pattern)
		state[chn] = false;
}

void CChannelManagerDlg::OnLButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || m_show == false) return;
	SetCapture();

	if(!ButtonHit(point, nullptr, nullptr)) ResetState(true,  false, false, false);

	m_leftButton = true;
	m_buttonAction = kUndetermined;
	MouseEvent(nFlags,point,CM_BT_LEFT);
	m_downX = point.x;
	m_downY = point.y;
}

void CChannelManagerDlg::OnRButtonUp(UINT /*nFlags*/,CPoint /*point*/)
{
	ReleaseCapture();
	if(!m_hWnd || m_show == false) return;

	ResetState(false, false, true, false);

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
		ResetState(true, true, false, false, false);
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
	CHANNELINDEX chn;
	CRect rect;
	if(m_ModDoc != nullptr && (m_ModDoc->GetModType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)) && ButtonHit(point, &chn, &rect))
	{
		// Rename channel
		CString s = mpt::cformat(_T("New name for channel %1:"))(chn +1);
		CInputDlg dlg(this, s, mpt::ToCString(m_ModDoc->GetSoundFile().GetCharsetInternal(), m_ModDoc->GetSoundFile().ChnSettings[chn].szName));
		if(dlg.DoModal() == IDOK)
		{
			m_ModDoc->GetSoundFile().ChnSettings[chn].szName = mpt::ToCharset(m_ModDoc->GetSoundFile().GetCharsetInternal(), dlg.resultAsString);
			InvalidateRect(rect, FALSE);
			m_ModDoc->SetModified();
			m_ModDoc->UpdateAllViews(nullptr, GeneralHint(chn).Channels(), this);
		}
	}
}

void CChannelManagerDlg::MouseEvent(UINT nFlags,CPoint point, MouseButton button)
{
	CHANNELINDEX n;
	CRect client, invalidate;
	bool hit = ButtonHit(point, &n, &invalidate);
	if(hit) n = pattern[n];

	m_moveX = point.x;
	m_moveY = point.y;

	if(!m_ModDoc) return;

	if(hit && !state[n] && button != CM_BT_NONE)
	{
		if(nFlags & MK_CONTROL)
		{
			if(button == CM_BT_LEFT)
			{
				if(!select[n] && !removed[n]) move[n] = invalidate;
				select[n] = true;
			}
			else if(button == CM_BT_RIGHT) select[n] = false;
		}
		else if(!removed[n] || m_currentTab == 3)
		{
			switch(m_currentTab)
			{
				case kSoloMute:
					if(button == CM_BT_LEFT)
					{
						if(m_buttonAction == kUndetermined)
							m_buttonAction = (!m_ModDoc->IsChannelSolo(n) || m_ModDoc->IsChannelMuted(n)) ? kAction1 : kAction2;
						if(m_buttonAction == kAction1)
						{
							m_ModDoc->MuteChannel(n, false);
							m_ModDoc->SoloChannel(n, true);
							for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++) if(nChn != n) m_ModDoc->MuteChannel(nChn, true);
							invalidate = client = m_drawableArea;
						}
						else m_ModDoc->SoloChannel(n, false);
					} else
					{
						if(m_ModDoc->IsChannelSolo(n)) m_ModDoc->SoloChannel(n, false);
						if(m_buttonAction == kUndetermined)
							m_buttonAction = m_ModDoc->IsChannelMuted(n) ? kAction1 : kAction2;
						m_ModDoc->MuteChannel(n, m_buttonAction == kAction2);
					}
					m_ModDoc->SetModified();
					m_ModDoc->UpdateAllViews(nullptr, GeneralHint(n).Channels(), this);
					break;
				case kRecordSelect:
				{
					auto rec = m_ModDoc->IsChannelRecord(n);
					if(m_buttonAction == kUndetermined)
						m_buttonAction = (!rec || rec != (button == CM_BT_LEFT ? 1 : 2)) ? kAction1 : kAction2;

					m_ModDoc->Record1Channel(n, false);
					m_ModDoc->Record2Channel(n, false);
					if(m_buttonAction == kAction1)
					{
						if(button == CM_BT_LEFT) m_ModDoc->Record1Channel(n);
						else m_ModDoc->Record2Channel(n);
					}
					m_ModDoc->UpdateAllViews(nullptr, GeneralHint(n).Channels(), this);
					break;
				}
				case kPluginState:
					if(button == CM_BT_LEFT) m_ModDoc->NoFxChannel(n, false);
					else m_ModDoc->NoFxChannel(n, true);
					m_ModDoc->SetModified();
					m_ModDoc->UpdateAllViews(nullptr, GeneralHint(n).Channels(), this);
					break;
				case kReorderRemove:
					if(button == CM_BT_LEFT)
					{
						move[n] = invalidate;
						select[n] = true;
					}
					if(button == CM_BT_RIGHT)
					{
						if(m_buttonAction == kUndetermined)
							m_buttonAction = removed[n] ? kAction1 : kAction2;
							select[n] = false;
						removed[n] = (m_buttonAction == kAction2);
					}

					if(select[n] || button == 0)
					{
						m_moveRect = true;
					}
					break;
			}
		}

		state[n] = false;
		InvalidateRect(invalidate, FALSE);
	} else
	{
		InvalidateRect(m_drawableArea, FALSE);
	}
}


void CChannelManagerDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	OnLButtonDown(nFlags, point);
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CChannelManagerDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	OnRButtonDown(nFlags, point);
	CDialog::OnRButtonDblClk(nFlags, point);
}


OPENMPT_NAMESPACE_END
