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
#include "../common/mptStringBuffer.h"

#include <functional>

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
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
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

void CChannelManagerDlg::Update(UpdateHint hint, CObject* pHint)
{
	if(!m_hWnd || !m_show)
		return;
	if(!hint.ToType<GeneralHint>().GetType()[HINT_MODCHANNELS | HINT_MODGENERAL | HINT_MODTYPE | HINT_MPTOPTIONS])
		return;
	ResizeWindow();
	InvalidateRect(nullptr, FALSE);
	if(hint.ToType<GeneralHint>().GetType()[HINT_MODCHANNELS] && m_quickChannelProperties.m_hWnd && pHint != &m_quickChannelProperties)
		m_quickChannelProperties.UpdateDisplay();
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
	: m_buttonHeight(CM_BT_HEIGHT)
{
	for(CHANNELINDEX chn = 0; chn < MAX_BASECHANNELS; chn++)
	{
		pattern[chn] = chn;
		memory[0][chn] = 0;
		memory[1][chn] = 0;
		memory[2][chn] = 0;
		memory[3][chn] = chn;
	}
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

	CHANNELINDEX numChannels, newMemory[4][MAX_BASECHANNELS];
	std::vector<CHANNELINDEX> newChnOrder;
	newChnOrder.reserve(m_ModDoc->GetNumChannels());

	// Count new number of channels, copy pattern pointers & manager internal store memory
	numChannels = 0;
	for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
	{
		if(!removed[pattern[chn]])
		{
			newMemory[0][numChannels] = memory[0][numChannels];
			newMemory[1][numChannels] = memory[1][numChannels];
			newMemory[2][numChannels] = memory[2][numChannels];
			newChnOrder.push_back(pattern[chn]);
			numChannels++;
		}
	}

	BeginWaitCursor();

	//Creating new order-vector for ReArrangeChannels.
	CriticalSection cs;
	if(m_ModDoc->ReArrangeChannels(newChnOrder) != numChannels)
	{
		cs.Leave();
		EndWaitCursor();
		return;
	}

	// Update manager internal store memory
	for(CHANNELINDEX chn = 0; chn < numChannels; chn++)
	{
		CHANNELINDEX newChn = newChnOrder[chn];
		if(chn != newChn)
		{
			memory[0][chn] = newMemory[0][newChn];
			memory[1][chn] = newMemory[1][newChn];
			memory[2][chn] = newMemory[2][newChn];
		}
		memory[3][chn] = chn;
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
	select.set();
	InvalidateRect(m_drawableArea, FALSE);
}

void CChannelManagerDlg::OnInvert()
{
	select.flip();
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
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(!removed[sourceChn])
					{
						if(select[sourceChn])
							nbSelect++;
						if(select[sourceChn] && m_ModDoc->IsChannelSolo(sourceChn))
							nbOk++;
					}
				}
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(select[sourceChn] && !removed[sourceChn])
					{
						if(m_ModDoc->IsChannelMuted(sourceChn))
							m_ModDoc->MuteChannel(sourceChn, false);
						if(nbSelect == nbOk)
							m_ModDoc->SoloChannel(sourceChn, !m_ModDoc->IsChannelSolo(sourceChn));
						else
							m_ModDoc->SoloChannel(sourceChn, true);
					}
					else if(!m_ModDoc->IsChannelSolo(sourceChn))
						m_ModDoc->MuteChannel(sourceChn, true);
				}
				break;
			case kRecordSelect:
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(!removed[sourceChn])
					{
						if(select[sourceChn])
							nbSelect++;
						if(select[sourceChn] && m_ModDoc->GetChannelRecordGroup(sourceChn) == RecordGroup::Group1)
							nbOk++;
					}
				}
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(!removed[sourceChn] && select[sourceChn])
					{
						if(select[sourceChn] && nbSelect != nbOk && m_ModDoc->GetChannelRecordGroup(sourceChn) != RecordGroup::Group1)
							m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::Group1);
						else if(nbSelect == nbOk)
							m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::NoGroup);
					}
				}
				break;
			case kPluginState:
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(select[sourceChn] && !removed[sourceChn])
						m_ModDoc->NoFxChannel(sourceChn, false);
				}
				break;
			case kReorderRemove:
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(select[sourceChn])
						removed[sourceChn] = !removed[sourceChn];
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
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(!removed[sourceChn])
					{
						if(select[sourceChn])
							nbSelect++;
						if(select[sourceChn] && m_ModDoc->IsChannelMuted(sourceChn))
							nbOk++;
					}
				}
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(select[sourceChn] && !removed[sourceChn])
					{
						if(m_ModDoc->IsChannelSolo(sourceChn))
							m_ModDoc->SoloChannel(sourceChn, false);
						if(nbSelect == nbOk)
							m_ModDoc->MuteChannel(sourceChn, !m_ModDoc->IsChannelMuted(sourceChn));
						else
							m_ModDoc->MuteChannel(sourceChn, true);
					}
				}
				break;
			case kRecordSelect:
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(!removed[sourceChn])
					{
						if(select[sourceChn])
							nbSelect++;
						if(select[sourceChn] && m_ModDoc->GetChannelRecordGroup(sourceChn) == RecordGroup::Group2)
							nbOk++;
					}
				}
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(!removed[sourceChn] && select[sourceChn])
					{
						if(select[sourceChn] && nbSelect != nbOk && m_ModDoc->GetChannelRecordGroup(sourceChn) != RecordGroup::Group2)
							m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::Group2);
						else if(nbSelect == nbOk)
							m_ModDoc->SetChannelRecordGroup(sourceChn, RecordGroup::NoGroup);
					}
				}
				break;
			case kPluginState:
				for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				{
					CHANNELINDEX sourceChn = pattern[chn];
					if(select[sourceChn] && !removed[sourceChn])
						m_ModDoc->NoFxChannel(sourceChn, true);
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
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
			{
				CHANNELINDEX sourceChn = pattern[chn];
				memory[0][sourceChn] = 0;
				if(m_ModDoc->IsChannelMuted(sourceChn)) memory[0][chn] |= 1;
				if(m_ModDoc->IsChannelSolo(sourceChn))  memory[0][chn] |= 2;
			}
			break;
		case kRecordSelect:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				memory[1][chn] = static_cast<uint8>(m_ModDoc->GetChannelRecordGroup(pattern[chn]));
			break;
		case kPluginState:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				memory[2][chn] = m_ModDoc->IsChannelNoFx(pattern[chn]);
			break;
		case kReorderRemove:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				memory[3][chn] = pattern[chn];
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
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
			{
				CHANNELINDEX sourceChn = pattern[chn];
				m_ModDoc->MuteChannel(sourceChn, (memory[0][chn] & 1) != 0);
				m_ModDoc->SoloChannel(sourceChn, (memory[0][chn] & 2) != 0);
			}
			break;
		case kRecordSelect:
			m_ModDoc->ReinitRecordState(true);
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
			{
				m_ModDoc->SetChannelRecordGroup(chn, static_cast<RecordGroup>(memory[1][chn]));
			}
			break;
		case kPluginState:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				m_ModDoc->NoFxChannel(pattern[chn], memory[2][chn] != 0);
			break;
		case kReorderRemove:
			for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
				pattern[chn] = memory[3][chn];
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

		for(CHANNELINDEX chn = 0; chn < channels; chn++)
		{
			CHANNELINDEX sourceChn = pattern[chn];
			if(select[sourceChn])
			{
				CRect btn = move[sourceChn];
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
	HGDIOBJ oldFont = ::SelectObject(dc, CMainFrame::GetGUIFont());

	const auto dcBrush = GetStockBrush(DC_BRUSH);

	client.SetRect(client.left + MulDiv(2, dpiX, 96), client.top + MulDiv(32, dpiY, 96), client.right - MulDiv(2, dpiX, 96), client.bottom - MulDiv(24, dpiY, 96));
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
	for(CHANNELINDEX chn = 0; chn < channels; chn++, col++)
	{
		if(col >= CM_NB_COLS)
		{
			col = 0;
			row++;
		}

		const CHANNELINDEX sourceChn = pattern[chn];
		const auto &chnSettings = sndFile.ChnSettings[sourceChn];

		if(!chnSettings.szName.empty())
			s = MPT_CFORMAT("{}: {}")(sourceChn + 1, mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.ChnSettings[sourceChn].szName));
		else
			s = MPT_CFORMAT("Channel {}")(sourceChn + 1);

		const int borderX = MulDiv(3, dpiX, 96), borderY = MulDiv(3, dpiY, 96);
		CRect btn;
		btn.left = client.left + col * chnSizeX + borderX;
		btn.right = btn.left + chnSizeX - borderX;
		btn.top = client.top + row * chnSizeY + borderY;
		btn.bottom = btn.top + chnSizeY - borderY;

		if(!CRect{}.IntersectRect(&pDC.rcPaint, &btn))
			continue;

		// Button
		const bool activate = select[sourceChn];
		const bool enable = !removed[sourceChn];
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
			rect.right = rect.left + 1;
			for(int i = 0; i < width; i++)
			{
				auto blend = static_cast<double>(i) / width, blendInv = 1.0 - blend;
				auto blendColor = RGB(mpt::saturate_round<uint8>(GetRValue(startColor) * blendInv + GetRValue(endColor) * blend),
				                      mpt::saturate_round<uint8>(GetGValue(startColor) * blendInv + GetGValue(endColor) * blend),
				                      mpt::saturate_round<uint8>(GetBValue(startColor) * blendInv + GetBValue(endColor) * blend));
				::SetDCBrushColor(dc, blendColor);
				::FillRect(dc, &rect, dcBrush);
				rect.left++;
				rect.right++;
			}
		}

		// Text
		{
			auto rect = btnAdjusted;
			rect.left += Util::ScalePixels(9, m_hWnd);
			rect.right -= Util::ScalePixels(3, m_hWnd);

			::SetBkMode(dc, TRANSPARENT);
			::SetTextColor(dc, GetSysColor(enable || activate ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
			::DrawText(dc, s, -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}

		// Draw red/green markers
		{
			const int margin = Util::ScalePixels(1, m_hWnd);
			auto rect = btnAdjusted;
			rect.DeflateRect(margin, margin);
			rect.right = rect.left + Util::ScalePixels(7, m_hWnd);
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
				color = removed[sourceChn] ? redBrush : greenBrush;
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
		CHANNELINDEX dropChn = 0;
		CRect dropRect;
		if(ButtonHit(point, &dropChn, &dropRect))
		{
			// Rearrange channels
			const auto IsSelected = std::bind(&decltype(select)::test, &select, std::placeholders::_1);

			const auto numChannels = m_ModDoc->GetNumChannels();
			if(point.x > dropRect.left + dropRect.Width() / 2 && dropChn < numChannels)
				dropChn++;

			std::vector<CHANNELINDEX> newOrder{ pattern.begin(), pattern.begin() + numChannels };
			// How many selected channels are there before the drop target?
			const CHANNELINDEX selectedBeforeDropChn = static_cast<CHANNELINDEX>(std::count_if(pattern.begin(), pattern.begin() + dropChn, IsSelected));
			dropChn -= selectedBeforeDropChn;
			// Remove all selected channels from the order
			newOrder.erase(std::remove_if(newOrder.begin(), newOrder.end(), IsSelected), newOrder.end());
			const CHANNELINDEX numSelected = static_cast<CHANNELINDEX>(numChannels - newOrder.size());
			// Then insert them at the drop position
			newOrder.insert(newOrder.begin() + dropChn, numSelected, PATTERNINDEX_INVALID);
			std::copy_if(pattern.begin(), pattern.begin() + numChannels, newOrder.begin() + dropChn, IsSelected);

			std::copy(newOrder.begin(), newOrder.begin() + numChannels, pattern.begin());
			select.reset();
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
		ClientToScreen(&point);
		m_quickChannelProperties.Show(m_ModDoc, pattern[chn], point);
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
							for(CHANNELINDEX chn = 0; chn < m_ModDoc->GetNumChannels(); chn++)
							{
								if(chn != n)
									m_ModDoc->MuteChannel(chn, true);
							}
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
					auto rec = m_ModDoc->GetChannelRecordGroup(n);
					if(m_buttonAction == kUndetermined)
						m_buttonAction = (rec == RecordGroup::NoGroup || rec != (button == CM_BT_LEFT ? RecordGroup::Group1 : RecordGroup::Group2)) ? kAction1 : kAction2;

					if(m_buttonAction == kAction1 && button == CM_BT_LEFT)
						m_ModDoc->SetChannelRecordGroup(n, RecordGroup::Group1);
					else if(m_buttonAction == kAction1 && button == CM_BT_RIGHT)
						m_ModDoc->SetChannelRecordGroup(n, RecordGroup::Group2);
					else
						m_ModDoc->SetChannelRecordGroup(n, RecordGroup::NoGroup);
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
