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
#include "../common/StringFixer.h"


OPENMPT_NAMESPACE_BEGIN

#define CM_BT_LEFT		1
#define CM_BT_RIGHT		2
#define CM_NB_COLS		8
#define CM_BT_HEIGHT	22

///////////////////////////////////////////////////////////
// CChannelManagerDlg

BEGIN_MESSAGE_MAP(CChannelManagerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOVE()
	ON_WM_SIZE()
	ON_WM_ACTIVATE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_CLOSE()
	//{{AFX_MSG_MAP(CRemoveChannelsDlg)
	ON_COMMAND(IDC_BUTTON1,	OnApply)
	ON_COMMAND(IDC_BUTTON2,	OnClose)
	ON_COMMAND(IDC_BUTTON3,	OnSelectAll)
	ON_COMMAND(IDC_BUTTON4,	OnInvert)
	ON_COMMAND(IDC_BUTTON5,	OnAction1)
	ON_COMMAND(IDC_BUTTON6,	OnAction2)
	ON_COMMAND(IDC_BUTTON7,	OnStore)
	ON_COMMAND(IDC_BUTTON8,	OnRestore)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1,	OnTabSelchange)
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
END_MESSAGE_MAP()

CChannelManagerDlg * CChannelManagerDlg::sharedInstance_ = nullptr;

CChannelManagerDlg * CChannelManagerDlg::sharedInstance(bool autoCreate)
{
	if(CChannelManagerDlg::sharedInstance_ == nullptr && autoCreate) CChannelManagerDlg::sharedInstance_ = new CChannelManagerDlg();
	return CChannelManagerDlg::sharedInstance_;
}

void CChannelManagerDlg::SetDocument(CModDoc *modDoc)
{
	if(modDoc != m_ModDoc)
	{
		m_ModDoc = modDoc;
		nChannelsOld = 0;
		InvalidateRect(NULL, FALSE);
	}
}

BOOL CChannelManagerDlg::IsDisplayed(void)
{
	return show;
}

void CChannelManagerDlg::Update(void)
{
	if(!m_hWnd || show == false) return;

	nChannelsOld = 0;

	InvalidateRect(NULL, FALSE);
}

BOOL CChannelManagerDlg::Show(void)
{
	if(m_hWnd != NULL && show == false)
	{
		ShowWindow(SW_SHOW);
		show = true;
	}

	return show;
}

BOOL CChannelManagerDlg::Hide(void)
{
	if(this->m_hWnd != NULL && show == true)
	{
		ResetState(true, true, true, true, true);
		ShowWindow(SW_HIDE);
		show = false;
	}

	return show;
}


CChannelManagerDlg::CChannelManagerDlg(void)
	: m_ModDoc(nullptr)
	, leftButton(false)
	, rightButton(false)
	, moveRect(false)
	, nChannelsOld(0)
	, bkgnd(nullptr)
	, show(false)
{
	Create(IDD_CHANNELMANAGER, nullptr);
	ShowWindow(SW_HIDE);
}

CChannelManagerDlg::~CChannelManagerDlg(void)
{
	if(this == CChannelManagerDlg::sharedInstance_) CChannelManagerDlg::sharedInstance_ = nullptr;
	if(bkgnd) DeleteObject(bkgnd);
}

BOOL CChannelManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_ModDoc = CMainFrame::GetMainFrame()->GetActiveDoc();

	HWND menu = ::GetDlgItem(m_hWnd,IDC_TAB1);

	TCITEM tie;
	tie.mask = TCIF_TEXT | TCIF_IMAGE;
	tie.iImage = -1;
	tie.pszText = _T("Solo/Mute");
	TabCtrl_InsertItem(menu, 0, &tie);
	tie.pszText = _T("Record select");
	TabCtrl_InsertItem(menu, 1, &tie);
	tie.pszText = _T("Plugins");
	TabCtrl_InsertItem(menu, 2, &tie);
	tie.pszText = _T("Reorder/Remove");
	TabCtrl_InsertItem(menu, 3, &tie);
	currentTab = 0;

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

	buttonHeight = MulDiv(CM_BT_HEIGHT, Util::GetDPIy(m_hWnd), 96);
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);

	return TRUE;
}

void CChannelManagerDlg::OnApply()
{
	if(!m_ModDoc) return;

	CHANNELINDEX nChannels, newMemory[4][MAX_BASECHANNELS];
	std::vector<CHANNELINDEX> newChnOrder;
	newChnOrder.reserve(m_ModDoc->GetNumChannels());

	// Count new number of channels , copy pattern pointers & manager internal store memory
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

		ResetState(true, true, true, true, true);

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

	// Update document & player
	m_ModDoc->SetModified();
	m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels().ModType(), this); //refresh channel headers

	// Redraw channel manager window
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnClose()
{
	if(bkgnd) DeleteObject((HBITMAP)bkgnd);
	ResetState(true, true, true, true, true);
	bkgnd = nullptr;
	show = false;

	CDialog::OnCancel();
}

void CChannelManagerDlg::OnSelectAll()
{
	if(m_ModDoc)
		for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			select[nChn] = true;

	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnInvert()
{
	if(m_ModDoc)
		for(CHANNELINDEX nChn = 0 ; nChn < m_ModDoc->GetNumChannels() ; nChn++)
			select[nChn] = !select[nChn];
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnAction1()
{
	if(m_ModDoc)
	{
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab)
		{
			case 0:
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
			case 1:
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
			case 2:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn]) m_ModDoc->NoFxChannel(nThisChn, false);
				}
				break;
			case 3:
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
		InvalidateRect(NULL, FALSE);
	}
}

void CChannelManagerDlg::OnAction2()
{
	if(m_ModDoc)
	{
		
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab)
		{
			case 0:
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
			case 1:
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
			case 2:
				for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn]) m_ModDoc->NoFxChannel(nThisChn, true);
				}
				break;
			case 3:
				ResetState(false, false, false, false, true);
				break;
			default:
				break;
		}

		if(currentTab != 3) ResetState();

		m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
		InvalidateRect(NULL, FALSE);
	}
}

void CChannelManagerDlg::OnStore(void)
{
	if(!show || m_ModDoc == nullptr)
	{
		return;
	}

	switch(currentTab)
	{
		case 0:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			{
				CHANNELINDEX nThisChn = pattern[nChn];
				memory[0][nThisChn] = 0;
				if(m_ModDoc->IsChannelMuted(nThisChn)) memory[0][nChn] |= 1;
				if(m_ModDoc->IsChannelSolo(nThisChn))  memory[0][nChn] |= 2;
			}
			break;
		case 1:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				memory[1][nChn] = m_ModDoc->IsChannelRecord(pattern[nChn]);
			break;
		case 2:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				memory[2][nChn] = m_ModDoc->IsChannelNoFx(pattern[nChn]);
			break;
		case 3:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				memory[3][nChn] = pattern[nChn];
			break;
		default:
			break;
	}
}

void CChannelManagerDlg::OnRestore(void)
{
	if(!show || m_ModDoc == nullptr)
	{
		return;
	}

	switch(currentTab)
	{
		case 0:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			{
				CHANNELINDEX nThisChn = pattern[nChn];
				m_ModDoc->MuteChannel(nThisChn, (memory[0][nChn] & 1) != 0);
				m_ModDoc->SoloChannel(nThisChn, (memory[0][nChn] & 2) != 0);
			}
			break;
		case 1:
			m_ModDoc->ReinitRecordState(true);
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
			{
				if(memory[1][nChn] != 2) m_ModDoc->Record1Channel(pattern[nChn], memory[1][nChn] == 1);
				if(memory[1][nChn] != 1) m_ModDoc->Record2Channel(pattern[nChn], memory[1][nChn] == 2);
			}
			break;
		case 2:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				m_ModDoc->NoFxChannel(pattern[nChn], memory[2][nChn] != 0);
			break;
		case 3:
			for(CHANNELINDEX nChn = 0; nChn < m_ModDoc->GetNumChannels(); nChn++)
				pattern[nChn] = memory[3][nChn];
			ResetState(false, false, false, false, true);
			break;
		default:
			break;
	}

	if(currentTab != 3) ResetState();

	m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnTabSelchange(NMHDR* /*header*/, LRESULT* /*pResult*/)
{
	if(!show) return;

	int sel = TabCtrl_GetCurFocus(::GetDlgItem(m_hWnd,IDC_TAB1));
	currentTab = sel;

	switch(currentTab)
	{
		case 0:
			SetDlgItemText(IDC_BUTTON5, "Solo");
			SetDlgItemText(IDC_BUTTON6, "Mute");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case 1:
			SetDlgItemText(IDC_BUTTON5, "Instrument 1");
			SetDlgItemText(IDC_BUTTON6, "Instrument 2");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case 2:
			SetDlgItemText(IDC_BUTTON5, "Enable FX");
			SetDlgItemText(IDC_BUTTON6, "Disable FX");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case 3:
			SetDlgItemText(IDC_BUTTON5, "Remove");
			SetDlgItemText(IDC_BUTTON6, "Cancel all");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_SHOW);
			break;
		default:
			break;
	}

	InvalidateRect(NULL, FALSE);
}


void CChannelManagerDlg::DrawChannelButton(HDC hdc, LPRECT lpRect, LPCSTR lpszText, bool activate, bool enable, DWORD dwFlags)
{
	CRect rect = (*lpRect);

	DrawEdge(hdc, rect, enable ? EDGE_RAISED : EDGE_SUNKEN, BF_RECT | BF_MIDDLE | BF_ADJUST);
	if(activate)
	{
		::FillRect(hdc, rect, CMainFrame::brushWindow);
	}


	rect.left += Util::ScalePixels(13, m_hWnd);
	rect.right -= Util::ScalePixels(5, m_hWnd);

	::SetBkMode(hdc, TRANSPARENT);
	HGDIOBJ oldfont = ::SelectObject(hdc, CMainFrame::GetGUIFont());

	::SetTextColor(hdc, GetSysColor(enable || activate ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
	::DrawText(hdc, lpszText, -1, &rect, dwFlags | DT_SINGLELINE | DT_NOPREFIX);
	::SelectObject(hdc, oldfont);
}


void CChannelManagerDlg::OnSize(UINT nType,int cx,int cy)
{
	CWnd::OnSize(nType,cx,cy);
	if(!m_hWnd || !show) return;

	CRect wnd;
	GetClientRect(&wnd);

	const int dpiX = Util::GetDPIx(m_hWnd);
	const int dpiY = Util::GetDPIy(m_hWnd);

	// Move butttons to bottom of the window
	for(auto id : { IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5, IDC_BUTTON6 })
	{
		CWnd *button = GetDlgItem(id);
		if(button != nullptr)
		{
			CRect btn;
			button->GetClientRect(&btn);
			button->MapWindowPoints(this, &btn);
			button->SetWindowPos(nullptr, btn.left, wnd.Height() - btn.Height() - MulDiv(3, dpiY, 96), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
	}

	wnd.DeflateRect(MulDiv(10, dpiX, 96), MulDiv(38, dpiY, 96), MulDiv(8, dpiX, 96), MulDiv(30, dpiY, 96));
	m_drawableArea = wnd;
	buttonHeight = MulDiv(CM_BT_HEIGHT, dpiY, 96);

	if(bkgnd) DeleteObject(bkgnd);
	HDC dc = ::GetDC(m_hWnd);
	bkgnd = ::CreateCompatibleBitmap(dc, wnd.Width(), wnd.Height());
	if(!moveRect && bkgnd)
	{
		HDC bdc = ::CreateCompatibleDC(dc);
		::SelectObject(bdc,bkgnd);
		::BitBlt(bdc,0,0,wnd.Width(),wnd.Height(), dc,wnd.left,wnd.top,SRCCOPY);
		::SelectObject(bdc,(HBITMAP)NULL);
		::DeleteDC(bdc);
	}
	::ReleaseDC(m_hWnd, dc);

	nChannelsOld = 0;
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnActivate(UINT nState,CWnd* pWndOther,BOOL bMinimized)
{
	CDialog::OnActivate(nState,pWndOther,bMinimized);

	if(show && !bMinimized)
	{
		m_ModDoc = CMainFrame::GetMainFrame()->GetActiveDoc();
		ResetState(true, true, true, true, false);
		nChannelsOld = 0;
		InvalidateRect(NULL,TRUE);
	}
}

BOOL CChannelManagerDlg::OnEraseBkgnd(CDC* pDC)
{
	nChannelsOld = 0;
	return CDialog::OnEraseBkgnd(pDC);
}

void CChannelManagerDlg::OnPaint()
{
	if(!m_hWnd || show == false || m_ModDoc == nullptr)
	{
		ValidateRect(nullptr);
		return;
	}
	MPT_ASSERT(m_ModDoc == CMainFrame::GetMainFrame()->GetActiveDoc());

	const int dpiX = Util::GetDPIx(m_hWnd);
	const int dpiY = Util::GetDPIy(m_hWnd);

	CHAR s[256];
	UINT c=0,l=0;
	CHANNELINDEX nChannels = m_ModDoc->GetNumChannels();
	UINT nLines = nChannels / CM_NB_COLS + (nChannels % CM_NB_COLS ? 1 : 0);
	CRect client,btn;

	GetWindowRect(&btn);
	client = m_drawableArea;

	int chnSizeX = client.Width() / CM_NB_COLS;
	int chnSizeY = client.Height() / (int)nLines;

	if(chnSizeY != buttonHeight)
	{
		// Window height is not sufficient => resize window
		CWnd::SetWindowPos(NULL, 0, 0, btn.Width(), btn.Height() + (buttonHeight - chnSizeY) * nLines, SWP_NOMOVE | SWP_NOZORDER);
		return;
	}

	PAINTSTRUCT pDC;
	::BeginPaint(m_hWnd, &pDC);

	chnSizeY = buttonHeight;

	if(currentTab == 3 && moveRect && bkgnd)
	{
		// Only draw channels to be moved around
		HDC bdc = ::CreateCompatibleDC(pDC.hdc);
		::SelectObject(bdc, bkgnd);
		::BitBlt(pDC.hdc, client.left, client.top, client.Width(), client.Height(), bdc, 0, 0, SRCCOPY);

		BLENDFUNCTION ftn;
		ftn.BlendOp = AC_SRC_OVER;
		ftn.BlendFlags = 0;
		ftn.SourceConstantAlpha = 192;
		ftn.AlphaFormat = 0;

		for(CHANNELINDEX nChn = 0; nChn < nChannels; nChn++)
		{
			CHANNELINDEX nThisChn = pattern[nChn];
			if(select[nThisChn])
			{
				btn = move[nThisChn];
				btn.DeflateRect(3, 3, 0, 0);

				AlphaBlend(pDC.hdc, btn.left + mx - omx, btn.top + my - omy, btn.Width(), btn.Height(), bdc,
					btn.left - client.left, btn.top - client.top, btn.Width(), btn.Height(), ftn);
			}
		}
		::SelectObject(bdc, (HBITMAP)NULL);
		::DeleteDC(bdc);

		::EndPaint(m_hWnd,&pDC);
		return;
	}

	GetClientRect(&client);
	client.SetRect(client.left + MulDiv(2, dpiX, 96), client.top + MulDiv(32, dpiY, 96), client.right - MulDiv(2, dpiX, 96), client.bottom - MulDiv(24, dpiY, 96));

	CRect intersection;
	if(pDC.fErase || (intersection.IntersectRect(&pDC.rcPaint, &client) && nChannelsOld != nChannels))
	{
		FillRect(pDC.hdc, &intersection, CMainFrame::brushHighLight);
		FrameRect(pDC.hdc, &client, CMainFrame::brushBlack);
		nChannelsOld = nChannels;
	}

	client.SetRect(client.left + 8,client.top + 6,client.right - 6,client.bottom - 6);

	HBRUSH red = CreateSolidBrush(RGB(192, 96, 96));
	HBRUSH green = CreateSolidBrush(RGB(96, 192, 96));

	CSoundFile &sndFile = m_ModDoc->GetrSoundFile();
	for(CHANNELINDEX nChn = 0; nChn < nChannels; nChn++)
	{
		CHANNELINDEX nThisChn = pattern[nChn];

		if(sndFile.ChnSettings[nThisChn].szName[0] != '\0')
			wsprintf(s, "%d: %s", (nThisChn + 1), sndFile.ChnSettings[nThisChn].szName);
		else
			wsprintf(s, "Channel %d", nThisChn + 1);

		const int borderX = MulDiv(3, dpiX, 96), borderY = MulDiv(3, dpiY, 96);
		btn.left = client.left + c * chnSizeX + borderX;
		btn.right = btn.left + chnSizeX - borderX;
		btn.top = client.top + l * chnSizeY + borderY;
		btn.bottom = btn.top + chnSizeY - borderY;

		if(intersection.IntersectRect(&pDC.rcPaint, &client))
			DrawChannelButton(pDC.hdc, btn, s, select[nThisChn], !removed[nThisChn], DT_RIGHT | DT_VCENTER);

		btn.right = btn.left + chnSizeX / 7;

		btn.DeflateRect(borderX, borderY, borderX, borderY);

		// Draw red/green markers
		switch(currentTab)
		{
			case 0:
				FillRect(pDC.hdc, btn, sndFile.ChnSettings[nThisChn].dwFlags[CHN_MUTE] ? red : green);
				break;
			case 1:
			{
				const HBRUSH brushes[] = { CMainFrame::brushHighLight, green, red };
				auto r = m_ModDoc->IsChannelRecord(nThisChn);
				FillRect(pDC.hdc, btn, brushes[r % MPT_ARRAY_COUNT(brushes)]);
				break;
			}
			case 2:
				FillRect(pDC.hdc, btn, sndFile.ChnSettings[nThisChn].dwFlags[CHN_NOFX] ? red : green);
				break;
			case 3:
				FillRect(pDC.hdc, btn, removed[nThisChn] ? red : green);
				break;
		}
		// Draw border around marker
		FrameRect(pDC.hdc, btn, CMainFrame::brushBlack);

		c++;
		if(c >= CM_NB_COLS) { c = 0; l++; }
	}

	DeleteBrush(green);
	DeleteBrush(red);

	::EndPaint(m_hWnd,&pDC);
}

void CChannelManagerDlg::OnMove(int x, int y)
{
	CWnd::OnMove(x,y);
	nChannelsOld = 0;
}

bool CChannelManagerDlg::ButtonHit(CPoint point, CHANNELINDEX * id, CRect * invalidate)
{
	const CRect &client = m_drawableArea;

	if(PtInRect(client, point) && m_ModDoc != nullptr)
	{
		UINT nColns = CM_NB_COLS;

		int x = point.x - client.left;
		int y = point.y - client.top;

		int dx = client.Width() / (int)nColns;
		int dy = buttonHeight;

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
	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
	{
		if(bSelection)
			select[pattern[nChn]] = false;
		if(bButton)
			state[pattern[nChn]]  = false;
		if(bOrder)
		{
			pattern[nChn] = nChn;
			removed[nChn] = false;
		}
	}
	if(bMove || bInternal)
	{
		leftButton = false;
		rightButton = false;
	}
	if(move) moveRect = false;

	if(bOrder) nChannelsOld = 0;
}


void CChannelManagerDlg::OnMouseMove(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == false) return;

	if(!leftButton && !rightButton)
	{
		mx = point.x;
		my = point.y;
		return;
	}
	MouseEvent(nFlags, point, moveRect ? 0 : (leftButton ? CM_BT_LEFT : CM_BT_RIGHT));
}

void CChannelManagerDlg::OnLButtonUp(UINT /*nFlags*/,CPoint point)
{
	ReleaseCapture();
	if(!m_hWnd || show == false) return;

	if(moveRect && m_ModDoc)
	{
		CHANNELINDEX n, i, k;
		CHANNELINDEX newpat[MAX_BASECHANNELS];

		k = CHANNELINDEX_INVALID;
		bool hit = ButtonHit(point,&n,NULL);
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

		moveRect = false;
		nChannelsOld = 0;
		InvalidateRect(NULL, FALSE);
		if(m_ModDoc) m_ModDoc->UpdateAllViews(nullptr, GeneralHint().Channels(), this);
	}

	leftButton = false;

	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS ; nChn++)
		state[pattern[nChn]] = false;
}

void CChannelManagerDlg::OnLButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == false) return;
	SetCapture();

	if(!ButtonHit(point,NULL,NULL)) ResetState(true,  false, false, false);

	leftButton = true;
	m_buttonAction = kUndetermined;
	MouseEvent(nFlags,point,CM_BT_LEFT);
	omx = point.x;
	omy = point.y;
}

void CChannelManagerDlg::OnRButtonUp(UINT /*nFlags*/,CPoint /*point*/)
{
	ReleaseCapture();
	if(!m_hWnd || show == false) return;

	ResetState(false, false, true, false);

	rightButton = false;
}

void CChannelManagerDlg::OnRButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == false) return;
	SetCapture();

	rightButton = true;
	m_buttonAction = kUndetermined;
	if(moveRect)
	{
		ResetState(true, true, false, false, false);
		nChannelsOld = 0;
		InvalidateRect(NULL, FALSE);
	} else
	{
		MouseEvent(nFlags,point,CM_BT_RIGHT);
		omx = point.x;
		omy = point.y;
	}
}

void CChannelManagerDlg::OnMButtonDown(UINT /*nFlags*/, CPoint point)
{
	CHANNELINDEX chn;
	CRect rect;
	if(m_ModDoc != nullptr && (m_ModDoc->GetModType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)) && ButtonHit(point, &chn, &rect))
	{
		// Rename channel
		TCHAR s[64];
		wsprintf(s, _T("New name for channel %u:"), chn + 1);
		CInputDlg dlg(this, s, m_ModDoc->GetrSoundFile().ChnSettings[chn].szName);
		if(dlg.DoModal() == IDOK)
		{
			mpt::String::Copy(m_ModDoc->GetrSoundFile().ChnSettings[chn].szName, dlg.resultAsString.GetString());
			InvalidateRect(rect, FALSE);
			m_ModDoc->SetModified();
			m_ModDoc->UpdateAllViews(nullptr, GeneralHint(chn).Channels(), this);
		}
	}
}

void CChannelManagerDlg::MouseEvent(UINT nFlags,CPoint point,BYTE button)
{
	CHANNELINDEX n;
	CRect client,invalidate;
	bool hit = ButtonHit(point, &n, &invalidate);
	if(hit) n = pattern[n];

	mx = point.x;
	my = point.y;

	if(!m_ModDoc) return;

	if(hit && !state[n] && button)
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
		else if(!removed[n] || currentTab == 3)
		{
			switch(currentTab)
			{
				case 0:
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
				case 1:
					BYTE rec;
					rec = m_ModDoc->IsChannelRecord(n);
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
				case 2:
					if(button == CM_BT_LEFT) m_ModDoc->NoFxChannel(n, false);
					else m_ModDoc->NoFxChannel(n, true);
					m_ModDoc->SetModified();
					m_ModDoc->UpdateAllViews(nullptr, GeneralHint(n).Channels(), this);
					break;
				case 3:
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
						invalidate = client = m_drawableArea;
						HDC dc = ::GetDC(m_hWnd);
						if(!bkgnd) bkgnd = ::CreateCompatibleBitmap(dc,client.Width(),client.Height());
						if(!moveRect && bkgnd)
						{
							HDC bdc = ::CreateCompatibleDC(dc);
							::SelectObject(bdc,bkgnd);
							::BitBlt(bdc,0,0,client.Width(),client.Height(), dc,client.left,client.top,SRCCOPY);
							::SelectObject(bdc,(HBITMAP)NULL);
							::DeleteDC(bdc);
						}
						::ReleaseDC(m_hWnd, dc);
						moveRect = true;
					}
					break;
			}
		}

		state[n] = false;
		InvalidateRect(&invalidate, FALSE);
	} else
	{
		InvalidateRect(&m_drawableArea, FALSE);
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
