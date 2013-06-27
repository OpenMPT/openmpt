/*
 * ChannelManagerDlg.cpp
 * ---------------------
 * Purpose: Dialog class for moving, removing, managing channels
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "view_gen.h"
#include "ChannelManagerDlg.h"


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
	ON_WM_CLOSE()
	//{{AFX_MSG_MAP(CRemoveChannelsDlg)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
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

CChannelManagerDlg * CChannelManagerDlg::sharedInstance_ = NULL;

CChannelManagerDlg * CChannelManagerDlg::sharedInstance(BOOL autoCreate)
{
	if(CChannelManagerDlg::sharedInstance_ == NULL && autoCreate) CChannelManagerDlg::sharedInstance_ = new CChannelManagerDlg();
	return CChannelManagerDlg::sharedInstance_;
}

void CChannelManagerDlg::SetDocument(void * parent)
{
	if(parent && parentCtrl != parent){

		EnterCriticalSection(&applying);

		parentCtrl = parent;
		nChannelsOld = 0;

		LeaveCriticalSection(&applying);
		InvalidateRect(NULL, FALSE);
	}
}

BOOL CChannelManagerDlg::IsOwner(void * ctrl)
{
	return ctrl == parentCtrl;
}

BOOL CChannelManagerDlg::IsDisplayed(void)
{
	return show;
}

void CChannelManagerDlg::Update(void)
{
	if(!m_hWnd || show == false) return;

	EnterCriticalSection(&applying);
	nChannelsOld = 0;
	LeaveCriticalSection(&applying);

	InvalidateRect(NULL, FALSE);
}

BOOL CChannelManagerDlg::Show(void)
{
	if(this->m_hWnd != NULL && show == false){
		ShowWindow(SW_SHOW);
		show = true;
	}

	return show;
}

BOOL CChannelManagerDlg::Hide(void)
{
	if(this->m_hWnd != NULL && show == true){
		ResetState(true, true, true, true, true);
		ShowWindow(SW_HIDE);
		show = false;
	}

	return show;
}

CChannelManagerDlg::CChannelManagerDlg(void)
{
	InitializeCriticalSection(&applying);
	mouseTracking = false;
	rightButton = false;
	leftButton = false;
	parentCtrl = NULL;
	moveRect = false;
	nChannelsOld = 0;
	bkgnd = NULL;
	show = false;

	Create(IDD_CHANNELMANAGER, NULL);
	ShowWindow(SW_HIDE);
}

CChannelManagerDlg::~CChannelManagerDlg(void)
{
	if(this == CChannelManagerDlg::sharedInstance_) CChannelManagerDlg::sharedInstance_ = NULL;
	DeleteCriticalSection(&applying);
	if(bkgnd) DeleteObject(bkgnd);
}

BOOL CChannelManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	HWND menu = ::GetDlgItem(m_hWnd,IDC_TAB1);

    TCITEM tie; 
    tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 
    tie.pszText = "Solo/Mute"; 
    TabCtrl_InsertItem(menu, 0, &tie); 
    tie.pszText = "Record select"; 
    TabCtrl_InsertItem(menu, 1, &tie); 
    tie.pszText = "Fx plugins"; 
    TabCtrl_InsertItem(menu, 2, &tie); 
    tie.pszText = "Reorder/Remove"; 
    TabCtrl_InsertItem(menu, 3, &tie); 
	currentTab = 0;

	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++){
		pattern[nChn] = nChn;
		removed[nChn] = false;
		select[nChn] = false;
		state[nChn] = false;
		memory[0][nChn] = 0;
		memory[1][nChn] = 0;
		memory[2][nChn] = 0;
		memory[3][nChn] = nChn;
	}

	::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);

	return TRUE;
}

void CChannelManagerDlg::OnApply()
{
	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(!m_pSndFile || !pModDoc) return;

	EnterCriticalSection(&applying);

	CHANNELINDEX nChannels, newpat[MAX_BASECHANNELS], newMemory[4][MAX_BASECHANNELS];

	// Count new number of channels , copy pattern pointers & manager internal store memory
	nChannels = 0;
	for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
	{
		if(!removed[pattern[nChn]])
		{
			newMemory[0][nChannels] = memory[0][nChannels];
			newMemory[1][nChannels] = memory[1][nChannels];
			newMemory[2][nChannels] = memory[2][nChannels];
			newpat[nChannels++] = pattern[nChn];
		}
	}

	BeginWaitCursor();
	CriticalSection cs;

	//Creating new order-vector for ReArrangeChannels.
	std::vector<CHANNELINDEX> newChnOrder;
	for(CHANNELINDEX nChn = 0; nChn < nChannels; nChn++)
	{
		newChnOrder.push_back(newpat[nChn]);
	}
	if(pModDoc->ReArrangeChannels(newChnOrder) != nChannels)
	{
		cs.Leave();
		EndWaitCursor();
		Reporting::Error("Rearranging channels failed");

		ResetState(true, true, true, true, true);
		LeaveCriticalSection(&applying);

		return;
	}
	

	// Update manager internal store memory
	for(CHANNELINDEX nChn = 0; nChn < nChannels; nChn++)
	{
		if(nChn != newpat[nChn])
		{
			memory[0][nChn] = newMemory[0][newpat[nChn]];
			memory[1][nChn] = newMemory[1][newpat[nChn]];
			memory[2][nChn] = newMemory[2][newpat[nChn]];
		}
		memory[3][nChn] = nChn;
	}


	cs.Leave();
	EndWaitCursor();

	ResetState(true, true, true, true, true);
	LeaveCriticalSection(&applying);

	// Update document & player
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(NULL, HINT_MODTYPE | HINT_MODCHANNELS, NULL); //refresh channel headers

	// Redraw channel manager window
	InvalidateRect(NULL,TRUE);
}

void CChannelManagerDlg::OnClose()
{
	EnterCriticalSection(&applying);

	if(bkgnd) DeleteObject((HBITMAP)bkgnd);
	ResetState(true, true, true, true, true);
	bkgnd = NULL;
	show = false;

	LeaveCriticalSection(&applying);

	CDialog::OnCancel();
}

void CChannelManagerDlg::OnSelectAll()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(m_pSndFile)
		for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
			select[nChn] = true;

	LeaveCriticalSection(&applying);
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnInvert()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(m_pSndFile)
		for(CHANNELINDEX nChn = 0 ; nChn < m_pSndFile->m_nChannels ; nChn++)
			select[nChn] = !select[nChn];

	LeaveCriticalSection(&applying);
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnAction1()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile)
	{
		
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab)
		{
			case 0:
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						if(select[nThisChn] && pModDoc->IsChannelSolo(nThisChn)) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn]){
						if(pModDoc->IsChannelMuted(nThisChn)) pModDoc->MuteChannel(nThisChn, false);
						if(nbSelect == nbOk) pModDoc->SoloChannel(nThisChn, !pModDoc->IsChannelSolo(nThisChn));
						else pModDoc->SoloChannel(nThisChn, true);
					}
					else if(!pModDoc->IsChannelSolo(nThisChn)) pModDoc->MuteChannel(nThisChn, true);
				}
				break;
			case 1:
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						BYTE rec = pModDoc->IsChannelRecord(nThisChn);
						if(select[nThisChn] && rec == 1) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn] && select[nThisChn])
					{
						if(select[nThisChn] && nbSelect != nbOk && pModDoc->IsChannelRecord(nThisChn) != 1) pModDoc->Record1Channel(nThisChn);
						else if(nbSelect == nbOk) pModDoc->Record1Channel(nThisChn, false);
					}
				}
				break;
			case 2:
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn]) pModDoc->NoFxChannel(nThisChn, false);
				}
				break;
			case 3:
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn]) removed[nThisChn] = !removed[nThisChn];
				}
				break;
			default:
				break;
		}


		ResetState();
		LeaveCriticalSection(&applying);

		pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS, NULL);
		InvalidateRect(NULL,FALSE);
	}
	else LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnAction2()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile)
	{
		
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab){
			case 0:
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						if(select[nThisChn] && pModDoc->IsChannelMuted(nThisChn)) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn])
					{
						if(pModDoc->IsChannelSolo(nThisChn)) pModDoc->SoloChannel(nThisChn, false);
						if(nbSelect == nbOk) pModDoc->MuteChannel(nThisChn, !pModDoc->IsChannelMuted(nThisChn));
						else pModDoc->MuteChannel(nThisChn, true);
					}
				}
				break;
			case 1:
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn])
					{
						if(select[nThisChn]) nbSelect++;
						BYTE rec = pModDoc->IsChannelRecord(nThisChn);
						if(select[nThisChn] && rec == 2) nbOk++;
					}
				}
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(!removed[nThisChn] && select[nThisChn])
					{
						if(select[nThisChn] && nbSelect != nbOk && pModDoc->IsChannelRecord(nThisChn) != 2) pModDoc->Record2Channel(nThisChn);
						else if(nbSelect == nbOk) pModDoc->Record2Channel(nThisChn, false);
					}
				}
				break;
			case 2:
				for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				{
					CHANNELINDEX nThisChn = pattern[nChn];
					if(select[nThisChn] && !removed[nThisChn]) pModDoc->NoFxChannel(nThisChn, true);
				}
				break;
			case 3:
				ResetState(false, false, false, false, true);
				break;
			default:
				break;
		}

		if(currentTab != 3) ResetState();
		LeaveCriticalSection(&applying);

		pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS, NULL);
		InvalidateRect(NULL,FALSE);
	}
	else LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnStore(void)
{
	if(!show) return;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	switch(currentTab)
	{
		case 0:
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
			{
				CHANNELINDEX nThisChn = pattern[nChn];
				memory[0][nThisChn] = 0;
				if(pModDoc->IsChannelMuted(nThisChn)) memory[0][nChn] |= 1;
				if(pModDoc->IsChannelSolo(nThisChn))  memory[0][nChn] |= 2;
			}
			break;
		case 1:
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				memory[1][nChn] = pModDoc->IsChannelRecord(pattern[nChn]);
			break;
		case 2:
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				memory[2][nChn] = pModDoc->IsChannelNoFx(pattern[nChn]);
			break;
		case 3:
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				memory[3][nChn] = pattern[nChn];
			break;
		default:
			break;
	}
}

void CChannelManagerDlg::OnRestore(void)
{
	if(!show) return;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	EnterCriticalSection(&applying);

	switch(currentTab)
	{
		case 0:
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
			{
				CHANNELINDEX nThisChn = pattern[nChn];
				pModDoc->MuteChannel(nThisChn, (memory[0][nChn] & 1) != 0);
				pModDoc->SoloChannel(nThisChn, (memory[0][nChn] & 2) != 0);
			}
			break;
		case 1:
			pModDoc->ReinitRecordState(true);
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
			{
				if(memory[1][nChn] != 2) pModDoc->Record1Channel(pattern[nChn], memory[1][nChn] == 1);
				if(memory[1][nChn] != 1) pModDoc->Record2Channel(pattern[nChn], memory[1][nChn] == 2);
			}
			break;
		case 2:
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				pModDoc->NoFxChannel(pattern[nChn], memory[2][nChn] != 0);
			break;
		case 3:
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
				pattern[nChn] = memory[3][nChn];
			ResetState(false, false, false, false, true);
			break;
		default:
			break;
	}

	if(currentTab != 3) ResetState();
	LeaveCriticalSection(&applying);

	pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS, NULL);
	InvalidateRect(NULL,FALSE);
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
			SetDlgItemText(IDC_BUTTON5, "Enable fx");
			SetDlgItemText(IDC_BUTTON6, "Disable fx");
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

void DrawChannelButton(HDC hdc, LPRECT lpRect, LPCSTR lpszText, BOOL bActivate, BOOL bEnable, DWORD dwFlags, HBRUSH /*markBrush*/)
{
	RECT rect;
	rect = (*lpRect);
	::FillRect(hdc,&rect,bActivate ? CMainFrame::brushWindow : (bEnable ? CMainFrame::brushGray : CMainFrame::brushHighLight));

	if(bEnable){
		HGDIOBJ oldpen = ::SelectObject(hdc, CMainFrame::penLightGray);
		::MoveToEx(hdc, lpRect->left, lpRect->bottom-1, NULL);
		::LineTo(hdc, lpRect->left, lpRect->top);
		::LineTo(hdc, lpRect->right-1, lpRect->top);

		::MoveToEx(hdc, lpRect->left+1, lpRect->bottom-1, NULL);
		::LineTo(hdc, lpRect->left+1, lpRect->top+1);
		::LineTo(hdc, lpRect->right-1, lpRect->top+1);

		::SelectObject(hdc, CMainFrame::penBlack);
		::MoveToEx(hdc, lpRect->right-1, lpRect->top, NULL);
		::LineTo(hdc, lpRect->right-1, lpRect->bottom-1);
		::LineTo(hdc, lpRect->left, lpRect->bottom-1);

		::SelectObject(hdc, CMainFrame::penDarkGray);
		::MoveToEx(hdc, lpRect->right-2, lpRect->top+1, NULL);
		::LineTo(hdc, lpRect->right-2, lpRect->bottom-2);
		::LineTo(hdc, lpRect->left+1, lpRect->bottom-2);

		::SelectObject(hdc, oldpen);
	}
	else ::FrameRect(hdc,&rect,CMainFrame::brushBlack);

	if ((lpszText) && (lpszText[0]))
	{
		rect.left += 13;
		rect.right -= 5;

		::SetBkMode(hdc, TRANSPARENT);
		HGDIOBJ oldfont = ::SelectObject(hdc, CMainFrame::GetGUIFont());

		::SetTextColor(hdc, GetSysColor(bEnable || !bActivate ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
		::DrawText(hdc, lpszText, -1, &rect, dwFlags | DT_SINGLELINE | DT_NOPREFIX);
		::SelectObject(hdc, oldfont);
	}
}

void CChannelManagerDlg::OnSize(UINT nType,int cx,int cy)
{
	CWnd::OnSize(nType,cx,cy);
	if(!m_hWnd || !show) return;

	CWnd *button;
	CRect wnd,btn;
	GetWindowRect(&wnd);

	if((button = GetDlgItem(IDC_BUTTON1)) != nullptr)
	{
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL, btn.left - wnd.left - 3, wnd.Height() - btn.Height() * 2 - 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON2)) != nullptr)
	{
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL, btn.left - wnd.left - 3, wnd.Height() - btn.Height() * 2 - 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON3)) != nullptr)
	{
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL, btn.left - wnd.left - 3, wnd.Height() - btn.Height() * 2 - 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON4)) != nullptr)
	{
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL, btn.left - wnd.left - 3, wnd.Height() - btn.Height() * 2 - 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON5)) != nullptr)
	{
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL, btn.left - wnd.left - 3, wnd.Height() - btn.Height() * 2 - 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON6)) != nullptr)
	{
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL, btn.left - wnd.left - 3, wnd.Height() - btn.Height() * 2 - 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	GetClientRect(&wnd);
	wnd.DeflateRect(10, 38, 8, 30);
	if(bkgnd) DeleteObject(bkgnd);
	bkgnd = ::CreateCompatibleBitmap(::GetDC(m_hWnd), wnd.Width(), wnd.Height());
	if(!moveRect && bkgnd)
	{
		HDC bdc = ::CreateCompatibleDC(::GetDC(m_hWnd));
		::SelectObject(bdc,bkgnd);
		::BitBlt(bdc,0,0,wnd.Width(),wnd.Height(),::GetDC(m_hWnd),wnd.left,wnd.top,SRCCOPY);
		::SelectObject(bdc,(HBITMAP)NULL);
		::DeleteDC(bdc);
	}

	nChannelsOld = 0;
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnActivate(UINT nState,CWnd* pWndOther,BOOL bMinimized)
{
	CDialog::OnActivate(nState,pWndOther,bMinimized);

	if(show && !bMinimized){
		ResetState(true, true, true, true, false);
		EnterCriticalSection(&applying);
		nChannelsOld = 0;
		LeaveCriticalSection(&applying);
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
	if(!m_hWnd || show == false) return;

	EnterCriticalSection(&applying);

	PAINTSTRUCT pDC;
	::BeginPaint(m_hWnd,&pDC);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : nullptr;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : nullptr;

	if(!pModDoc || !m_pSndFile)
	{
		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		return;
	}

	CHAR s[256];
	UINT c=0,l=0;
	CHANNELINDEX nChannels = m_pSndFile->m_nChannels;
	UINT nLines = nChannels / CM_NB_COLS + (nChannels % CM_NB_COLS ? 1 : 0);
	CRect client,btn;

	GetWindowRect(&btn);
	GetClientRect(&client);
	client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);

	UINT chnSizeX = (client.right - client.left) / CM_NB_COLS;
	UINT chnSizeY = (client.bottom - client.top) / (int)nLines;

	if(chnSizeY != CM_BT_HEIGHT)
	{
		// Window height is not sufficient => resize window
		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		CWnd::SetWindowPos(NULL, 0, 0, btn.Width(), btn.Height() + (CM_BT_HEIGHT - chnSizeY) * nLines, SWP_NOMOVE | SWP_NOZORDER);
		return;
	}

	chnSizeY = CM_BT_HEIGHT;

	if(currentTab == 3 && moveRect && bkgnd){

		HDC bdc = ::CreateCompatibleDC(pDC.hdc);
		::SelectObject(bdc,bkgnd);
		::BitBlt(pDC.hdc,client.left,client.top,client.Width(),client.Height(),bdc,0,0,SRCCOPY);
		::SelectObject(bdc,(HBITMAP)NULL);
		::DeleteDC(bdc);
/*
		UINT n;
		POINT p;
		CRect r;
		p.x = mx;
		p.y = my;
		BOOL hit = ButtonHit(p,&n,&r);
		if(hit && !select[n]){
			r.top += 3;
			r.left += 3;
			FrameRect(pDC.hdc,&r,CMainFrame::brushBlack);
			r.top += 3;
			r.left += 3;
			r.bottom -= 3;
			r.right = r.left + chnSizeX / 7 - 6;
			FillRect(pDC.hdc,&r,CMainFrame::brushWhite);
			FrameRect(pDC.hdc,&r,CMainFrame::brushBlack);
		}
*/
		for(CHANNELINDEX nChn = 0; nChn < nChannels; nChn++)
		{
			CHANNELINDEX nThisChn = pattern[nChn];
			if(select[nThisChn]){
				btn = move[nThisChn];
				btn.left += mx - omx + 2;
				btn.right += mx - omx + 1;
				btn.top += my - omy + 2;
				btn.bottom += my - omy + 1;
				FrameRect(pDC.hdc,&btn,CMainFrame::brushBlack);
			}
		}

		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		return;
	}

	GetClientRect(&client);
	client.SetRect(client.left + 2,client.top + 32,client.right - 2,client.bottom - 24);

	CRect intersection;
	BOOL ok = intersection.IntersectRect(&pDC.rcPaint,&client);

	if(pDC.fErase || (ok && nChannelsOld != nChannels))
	{
		FillRect(pDC.hdc,&intersection,CMainFrame::brushHighLight);
		FrameRect(pDC.hdc,&client,CMainFrame::brushBlack);
		nChannelsOld = nChannels;
	}

	client.SetRect(client.left + 8,client.top + 6,client.right - 6,client.bottom - 6);

	HBRUSH red = CreateSolidBrush(RGB(192,96,96));
	HBRUSH green = CreateSolidBrush(RGB(96,192,96));

	for(CHANNELINDEX nChn = 0; nChn < nChannels; nChn++)
	{
		CHANNELINDEX nThisChn = pattern[nChn];

		if(m_pSndFile->ChnSettings[nThisChn].szName[0] != '\0')
	        wsprintf(s, "%d: %s", (nThisChn + 1), m_pSndFile->ChnSettings[nThisChn].szName);
		else
			wsprintf(s, "Channel %d", nThisChn + 1);

		btn.left = client.left + c * chnSizeX + 3;
		btn.right = btn.left + chnSizeX - 3;
		btn.top = client.top + l * chnSizeY + 3;
		btn.bottom = btn.top + chnSizeY - 3;

		ok = intersection.IntersectRect(&pDC.rcPaint, &client);
		if(ok) DrawChannelButton(pDC.hdc, &btn, s, select[nThisChn], removed[nThisChn] ? FALSE : TRUE, DT_RIGHT | DT_VCENTER, NULL);

		btn.right = btn.left + chnSizeX / 7;

		btn.DeflateRect(3, 3, 3, 3);

		switch(currentTab)
		{
			case 0:
				if(m_pSndFile->ChnSettings[nThisChn].dwFlags[CHN_MUTE]) FillRect(pDC.hdc,&btn,red);
				else if(m_pSndFile->ChnSettings[nThisChn].dwFlags[CHN_SOLO]) FillRect(pDC.hdc,&btn,green);
				else FillRect(pDC.hdc,&btn,CMainFrame::brushHighLight);
				break;
			case 1:
				BYTE r;
				if(pModDoc) r = pModDoc->IsChannelRecord(nThisChn);
				else r = 0;
				if(r == 1) FillRect(pDC.hdc,&btn,green);
				else if(r == 2) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,CMainFrame::brushHighLight);
				break;
			case 2:
				if(m_pSndFile->ChnSettings[nThisChn].dwFlags[CHN_NOFX]) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,green);
				break;
			case 3:
				if(removed[nThisChn]) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,green);
				break;
		}

		if(!removed[nThisChn])
		{
			HGDIOBJ oldpen = ::SelectObject(pDC.hdc, CMainFrame::penLightGray);
			::MoveToEx(pDC.hdc, btn.right, btn.top-2, NULL);
			::LineTo(pDC.hdc, btn.right, btn.bottom+1);
			::SelectObject(pDC.hdc, oldpen);
		}
		FrameRect(pDC.hdc,&btn,CMainFrame::brushBlack);

		c++;
		if(c >= CM_NB_COLS) { c = 0; l++; }
	}

	DeleteObject((HBRUSH)green);
	DeleteObject((HBRUSH)red);

	::EndPaint(m_hWnd,&pDC);
	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnMove(int x, int y)
{
	CWnd::OnMove(x,y);
	nChannelsOld = 0;
}

bool CChannelManagerDlg::ButtonHit( CPoint point, CHANNELINDEX * id, CRect * invalidate )
{
	CRect client;
	GetClientRect(&client);
	client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);

	if(!PtInRect(client,point)) return false;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile)
	{
		//UINT nChannels = m_pSndFile->m_nChannels;
		UINT nColns = CM_NB_COLS;
		//UINT nLines = nChannels / nColns + (nChannels % nColns ? 1 : 0);

		int x = point.x - client.left;
		int y = point.y - client.top;

		int dx = (client.right - client.left) / (int)nColns;
		int dy = CM_BT_HEIGHT;

		x = x / dx;
		y = y / dy;
		CHANNELINDEX n = static_cast<CHANNELINDEX>(y * nColns + x);
		if(n >= 0 && n < (int)m_pSndFile->m_nChannels)
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
		if(bOrder){
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
	if(bInternal) mouseTracking = false;

	if(bOrder) nChannelsOld = 0;
}

LRESULT CChannelManagerDlg::OnMouseLeave(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	if(!m_hWnd || show == false) return 0;

	EnterCriticalSection(&applying);
	mouseTracking = false;
	ResetState(false, true, false, true);
	LeaveCriticalSection(&applying);

	return 0;
}

LRESULT CChannelManagerDlg::OnMouseHover(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	if(!m_hWnd || show == false) return 0;

	EnterCriticalSection(&applying);
	mouseTracking = false;
	LeaveCriticalSection(&applying);

	return 0;
}

void CChannelManagerDlg::OnMouseMove(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == false) return;

	EnterCriticalSection(&applying);

	if(!mouseTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE|TME_HOVER;
		tme.dwHoverTime = 1;
		mouseTracking = _TrackMouseEvent(&tme) != FALSE; 
	}

	if(!leftButton && !rightButton)
	{
		mx = point.x;
		my = point.y;
		LeaveCriticalSection(&applying);
		return;
	}
	MouseEvent(nFlags,point,moveRect ? 0 : (leftButton ? CM_BT_LEFT : CM_BT_RIGHT));

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnLButtonUp(UINT /*nFlags*/,CPoint point)
{
	if(!m_hWnd || show == false) return;

	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(moveRect && m_pSndFile)
	{
		CHANNELINDEX n, i, k;
		CHANNELINDEX newpat[MAX_BASECHANNELS];

		k = CHANNELINDEX_INVALID;
		bool hit = ButtonHit(point,&n,NULL);
		for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
			if(k == CHANNELINDEX_INVALID && select[pattern[nChn]]) k = nChn;

		if(hit && m_pSndFile && k != CHANNELINDEX_INVALID){
			i = 0;
			k = 0;
			while(i < n){
				while(i < n && select[pattern[i]]) i++;
				if(i < n && !select[pattern[i]]){
					newpat[k] = pattern[i];
					pattern[i] = CHANNELINDEX_INVALID;
					k++;
					i++;
				}
			}
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels ; nChn++)
			{
				if(pattern[nChn] != CHANNELINDEX_INVALID && select[pattern[nChn]]){
					newpat[k] = pattern[nChn];
					pattern[nChn] = CHANNELINDEX_INVALID;
					k++;
				}
			}
			i = 0;
			while(i < m_pSndFile->m_nChannels){
				while(i < m_pSndFile->m_nChannels && pattern[i] == CHANNELINDEX_INVALID) i++;
				if(i < m_pSndFile->m_nChannels && pattern[i] != CHANNELINDEX_INVALID){
					newpat[k] = pattern[i];
					k++;
					i++;
				}
			}
			for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++)
			{
				pattern[nChn] = newpat[nChn];
				select[nChn] = false;
			}
		}

		moveRect = false;
		nChannelsOld = 0;
		InvalidateRect(NULL,FALSE);
	}

	leftButton = false;

	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS ; nChn++)
		state[pattern[nChn]] = false;

	if(pModDoc) pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS, NULL);

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnLButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == false) return;

	EnterCriticalSection(&applying);

	if(!ButtonHit(point,NULL,NULL)) ResetState(true,  false, false, false);

	leftButton = true;
	MouseEvent(nFlags,point,CM_BT_LEFT);
	omx = point.x;
	omy = point.y;

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnRButtonUp(UINT /*nFlags*/,CPoint /*point*/)
{
	if(!m_hWnd || show == false) return;

	EnterCriticalSection(&applying);

	ResetState(false, false, true, false);

	rightButton = false;
	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm->GetActiveDoc();
	if(pModDoc) pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS, NULL);

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnRButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == false) return;

	EnterCriticalSection(&applying);

	rightButton = true;
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

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::MouseEvent(UINT nFlags,CPoint point,BYTE button)
{
	CHANNELINDEX n;
	CRect client,invalidate;
	bool hit = ButtonHit(point, &n, &invalidate);
	if(hit) n = pattern[n];

	mx = point.x;
	my = point.y;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(!pModDoc || !m_pSndFile) return;

	if(hit && !state[n] && button){
		if(nFlags & MK_CONTROL){
			if(button == CM_BT_LEFT){
				if(!select[n] && !removed[n]) move[n] = invalidate;
				select[n] = true;
			}
			else if(button == CM_BT_RIGHT) select[n] = false;
		}
		else if(!removed[n] || currentTab == 3){
			switch(currentTab){
				case 0:
					if(button == CM_BT_LEFT)
					{
						if(!pModDoc->IsChannelSolo(n) || pModDoc->IsChannelMuted(n)){
							GetClientRect(&client);
							pModDoc->MuteChannel(n, false);
							pModDoc->SoloChannel(n, true);
							for(CHANNELINDEX nChn = 0; nChn < m_pSndFile->m_nChannels; nChn++) if(nChn != n) pModDoc->MuteChannel(nChn, true);
							client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);
							invalidate = client;
						}
						else pModDoc->SoloChannel(n, false);
					} else
					{
						if(pModDoc->IsChannelSolo(n)) pModDoc->SoloChannel(n, false);
						pModDoc->MuteChannel(n,!pModDoc->IsChannelMuted(n));
					}
					pModDoc->SetModified();
					pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | ((n/CHANNELS_IN_TAB) << HINT_SHIFT_CHNTAB));
					break;
				case 1:
					BYTE rec;
					rec = pModDoc->IsChannelRecord(n);
					if(!rec || rec != (button == CM_BT_LEFT ? 1 : 2)){
						if(button == CM_BT_LEFT) pModDoc->Record1Channel(n);
						else pModDoc->Record2Channel(n);
					} else
					{
						pModDoc->Record1Channel(n, false);
						pModDoc->Record2Channel(n, false);
					}
					break;
				case 2:
					if(button == CM_BT_LEFT) pModDoc->NoFxChannel(n, false);
					else pModDoc->NoFxChannel(n, true);
					pModDoc->SetModified();
					pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | ((n/CHANNELS_IN_TAB) << HINT_SHIFT_CHNTAB));
					break;
				case 3:
					if(button == CM_BT_LEFT)
					{
						move[n] = invalidate;
						select[n] = true;
					}
					if(button == CM_BT_RIGHT)
					{
						select[n] = false;
						removed[n] = !removed[n];
					}

					if(select[n] || button == 0)
					{
						GetClientRect(&client);
						client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);
						if(!bkgnd) bkgnd = ::CreateCompatibleBitmap(::GetDC(m_hWnd),client.Width(),client.Height());
						if(!moveRect && bkgnd){
							HDC bdc = ::CreateCompatibleDC(::GetDC(m_hWnd));
							::SelectObject(bdc,bkgnd);
							::BitBlt(bdc,0,0,client.Width(),client.Height(),::GetDC(m_hWnd),client.left,client.top,SRCCOPY);
							::SelectObject(bdc,(HBITMAP)NULL);
							::DeleteDC(bdc);
						}
						invalidate = client;
						moveRect = true;
					}
					break;
			}
		}

		state[n] = false;
		InvalidateRect(&invalidate, FALSE);
	}
	else{
		GetClientRect(&client);
		client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);
		InvalidateRect(&client, FALSE);
	}
}

// -! NEW_FEATURE#0015


void CChannelManagerDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	OnLButtonDown(nFlags, point);
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CChannelManagerDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	OnRButtonDown(nFlags, point);
	CDialog::OnRButtonDblClk(nFlags, point);
}
