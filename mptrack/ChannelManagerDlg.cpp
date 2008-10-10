#include "stdafx.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "view_gen.h"
#include "ChannelManagerDlg.h"
//#include "mptrack.h"
//#include "childfrm.h"
//#include "globals.h"
//#include "dlg_misc.h"
//#include "ctrl_pat.h"
//#include "view_pat.h"


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
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);
	nChannelsOld = 0;
	LeaveCriticalSection(&applying);

	InvalidateRect(NULL, FALSE);
}

BOOL CChannelManagerDlg::Show(void)
{
	if(this->m_hWnd != NULL && show == FALSE){
		ShowWindow(SW_SHOW);
		show = TRUE;
	}

	return show;
}

BOOL CChannelManagerDlg::Hide(void)
{
	if(this->m_hWnd != NULL && show == TRUE){
		ResetState(TRUE,TRUE,TRUE,TRUE,TRUE);
		ShowWindow(SW_HIDE);
		show = FALSE;
	}

	return show;
}

CChannelManagerDlg::CChannelManagerDlg(void)
{
	InitializeCriticalSection(&applying);
	mouseTracking = FALSE;
	rightButton = FALSE;
	leftButton = FALSE;
	parentCtrl = NULL;
	moveRect = FALSE;
	nChannelsOld = 0;
	bkgnd = NULL;
	show = FALSE;

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

	for(int i = 0 ; i < MAX_BASECHANNELS ; i++){
		pattern[i] = i;
		removed[i] = FALSE;
		select[i] = FALSE;
		state[i] = FALSE;
		memory[0][i] = 0;
		memory[1][i] = 0;
		memory[2][i] = 0;
		memory[3][i] = i;
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


	// Stop player (is this necessary?)
	CModDoc *pActiveMod = NULL;
	HWND followSong = pMainFrm->GetFollowSong(pModDoc);
	if(pMainFrm->IsPlaying()){
		if((m_pSndFile) && (!m_pSndFile->IsPaused())) pActiveMod = pMainFrm->GetModPlaying();
		pMainFrm->PauseMod();
	}

	EnterCriticalSection(&applying);

	UINT i,nChannels,newpat[MAX_BASECHANNELS],newMemory[4][MAX_BASECHANNELS];

	// Count new number of channels , copy pattern pointers & manager internal store memory
	nChannels = 0;
	for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
		if(!removed[pattern[i]]){
			newMemory[0][nChannels] = memory[0][nChannels];
			newMemory[1][nChannels] = memory[1][nChannels];
			newMemory[2][nChannels] = memory[2][nChannels];
			newpat[nChannels++] = pattern[i];
		}
	}

	BeginWaitCursor();
	BEGIN_CRITICAL();

	//Creating new order-vector for ReArrangeChannels.
	vector<CHANNELINDEX> newChnOrder; newChnOrder.reserve(nChannels);
	for(i = 0; i<nChannels; i++)
	{
		newChnOrder.push_back(newpat[i]);
	}
	if(m_pSndFile->ReArrangeChannels(newChnOrder) != nChannels)
	{
		MessageBox("Rearranging channels failed");
		END_CRITICAL();
		EndWaitCursor();

		ResetState(TRUE,TRUE,TRUE,TRUE,TRUE);
		LeaveCriticalSection(&applying);

		return;
	}
	

	// Update manager internal store memory
	for(i = 0 ; i < nChannels ; i++){
		if(i != newpat[i]){
			memory[0][i] = newMemory[0][newpat[i]];
			memory[1][i] = newMemory[1][newpat[i]];
			memory[2][i] = newMemory[2][newpat[i]];
		}
		memory[3][i] = i;
	}

	/*
	if(pActiveMod == pModDoc){
		i = m_pSndFile->GetCurrentPos();
		m_pSndFile->m_dwSongFlags &= ~SONG_STEP;
		m_pSndFile->SetCurrentPos(0);
		m_pSndFile->SetCurrentPos(i);
	}
	*/	

	END_CRITICAL();
	EndWaitCursor();

	ResetState(TRUE,TRUE,TRUE,TRUE,TRUE);
	LeaveCriticalSection(&applying);

	// Update document & player
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(NULL,0xff,NULL);
	pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS ); //refresh channel headers
	pMainFrm->PlayMod(pActiveMod, followSong, MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS); //rewbs.fix2977

	// Redraw channel manager window
	InvalidateRect(NULL,TRUE);
}

void CChannelManagerDlg::OnClose()
{
	EnterCriticalSection(&applying);

	if(bkgnd) DeleteObject((HBITMAP)bkgnd);
	ResetState(TRUE,TRUE,TRUE,TRUE,TRUE);
	bkgnd = NULL;
	show = FALSE;

	LeaveCriticalSection(&applying);

	CDialog::OnCancel();
}

void CChannelManagerDlg::OnSelectAll()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(m_pSndFile) for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) select[i] = TRUE;

	LeaveCriticalSection(&applying);
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnInvert()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(m_pSndFile) for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) select[i] = !select[i];

	LeaveCriticalSection(&applying);
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnAction1()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile){
		
		UINT ii,i,r;
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab){
			case 0:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						if(select[ii] && pModDoc->IsChannelSolo(ii)) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]){
						if(pModDoc->IsChannelMuted(ii)) pModDoc->MuteChannel(ii,FALSE);
						if(nbSelect == nbOk) pModDoc->SoloChannel(ii,!pModDoc->IsChannelSolo(ii));
						else pModDoc->SoloChannel(ii,TRUE);
					}
					else if(!pModDoc->IsChannelSolo(ii)) pModDoc->MuteChannel(ii,TRUE);
				}
				break;
			case 1:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						r = pModDoc->IsChannelRecord(ii);
						if(select[ii] && r == 1) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii] && select[ii]){
						if(select[ii] && nbSelect != nbOk && pModDoc->IsChannelRecord(ii) != 1) pModDoc->Record1Channel(ii);
						else if(nbSelect == nbOk) pModDoc->Record1Channel(ii,FALSE);
					}
				}
				break;
			case 2:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]) pModDoc->NoFxChannel(ii,FALSE);
				}
				break;
			case 3:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii]) removed[ii] = !removed[ii];
				}
				break;
			default:
				break;
		}


		ResetState();
		LeaveCriticalSection(&applying);

		pModDoc->UpdateAllViews(NULL,0xff,NULL);
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

	if(pModDoc && m_pSndFile){
		
		UINT i,ii,r;
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab){
			case 0:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						if(select[ii] && pModDoc->IsChannelMuted(ii)) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]){
						if(pModDoc->IsChannelSolo(ii)) pModDoc->SoloChannel(ii,FALSE);
						if(nbSelect == nbOk) pModDoc->MuteChannel(ii,!pModDoc->IsChannelMuted(ii));
						else pModDoc->MuteChannel(ii,TRUE);
					}
				}
				break;
			case 1:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						r = pModDoc->IsChannelRecord(ii);
						if(select[ii] && r == 2) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii] && select[ii]){
						if(select[ii] && nbSelect != nbOk && pModDoc->IsChannelRecord(ii) != 2) pModDoc->Record2Channel(ii);
						else if(nbSelect == nbOk) pModDoc->Record2Channel(ii,FALSE);
					}
				}
				break;
			case 2:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]) pModDoc->NoFxChannel(ii,TRUE);
				}
				break;
			case 3:
				ResetState(FALSE,FALSE,FALSE,FALSE,TRUE);
				break;
			default:
				break;
		}

		if(currentTab !=3) ResetState();
		LeaveCriticalSection(&applying);

		pModDoc->UpdateAllViews(NULL,0xff,NULL);
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

	switch(currentTab){
		case 0:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				UINT ii = pattern[i];
				memory[0][i] = 0;
				if(pModDoc->IsChannelMuted(ii)) memory[0][i] |= 1;
				if(pModDoc->IsChannelSolo(ii))  memory[0][i] |= 2;
			}
			break;
		case 1:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) memory[1][i] = pModDoc->IsChannelRecord(pattern[i]);
			break;
		case 2:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) memory[2][i] = pModDoc->IsChannelNoFx(pattern[i]);
			break;
		case 3:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) memory[3][i] = pattern[i];
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

	switch(currentTab){
		case 0:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				UINT ii = pattern[i];
				pModDoc->MuteChannel(ii,memory[0][i] & 1);
				pModDoc->SoloChannel(ii,memory[0][i] & 2);
			}
			break;
		case 1:
			pModDoc->ReinitRecordState(TRUE);
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				if(memory[1][i] != 2) pModDoc->Record1Channel(pattern[i],memory[1][i] == 1);
				if(memory[1][i] != 1) pModDoc->Record2Channel(pattern[i],memory[1][i] == 2);
			}
			break;
		case 2:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) pModDoc->NoFxChannel(pattern[i],memory[2][i]);
			break;
		case 3:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) pattern[i] = memory[3][i];
			ResetState(FALSE,FALSE,FALSE,FALSE,TRUE);
			break;
		default:
			break;
	}

	if(currentTab !=3) ResetState();
	LeaveCriticalSection(&applying);

	pModDoc->UpdateAllViews(NULL,0xff,NULL);
	InvalidateRect(NULL,FALSE);
}

void CChannelManagerDlg::OnTabSelchange(NMHDR* /*header*/, LRESULT* /*pResult*/)
{
	if(!show) return;

	int sel = TabCtrl_GetCurFocus(::GetDlgItem(m_hWnd,IDC_TAB1));
	currentTab = sel;

	switch(currentTab){
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

	if ((lpszText) && (lpszText[0])){
		rect.left += 13;
		rect.right -= 5;

		::SetBkMode(hdc, TRANSPARENT);
		HGDIOBJ oldfont = ::SelectObject(hdc, CMainFrame::GetGUIFont());

		::SetTextColor(hdc, GetSysColor(bEnable || !bActivate ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
		::DrawText(hdc, lpszText, -1, &rect, dwFlags | DT_SINGLELINE);
		::SelectObject(hdc, oldfont);
	}
}

void CChannelManagerDlg::OnSize(UINT nType,int cx,int cy)
{
	CWnd::OnSize(nType,cx,cy);
	if(!m_hWnd || show == FALSE) return;

	CWnd * button;
	CRect wnd,btn;
	GetWindowRect(&wnd);

	if((button = GetDlgItem(IDC_BUTTON1)) != 0){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON2)) != 0){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON3)) != 0){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON4)) != 0){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON5)) != 0){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if((button = GetDlgItem(IDC_BUTTON6)) != 0){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}

	GetClientRect(&wnd);
	wnd.SetRect(wnd.left + 10,wnd.top + 38,wnd.right - 8,wnd.bottom - 30);
	if(bkgnd) DeleteObject(bkgnd);
	bkgnd = ::CreateCompatibleBitmap(::GetDC(m_hWnd),wnd.Width(),wnd.Height());
	if(!moveRect && bkgnd){
		HDC bdc = ::CreateCompatibleDC(::GetDC(m_hWnd));
		::SelectObject(bdc,bkgnd);
		::BitBlt(bdc,0,0,wnd.Width(),wnd.Height(),::GetDC(m_hWnd),wnd.left,wnd.top,SRCCOPY);
		::SelectObject(bdc,(HBITMAP)NULL);
		::DeleteDC(bdc);
	}

	nChannelsOld = 0;
	InvalidateRect(NULL,FALSE);
}

void CChannelManagerDlg::OnActivate(UINT nState,CWnd* pWndOther,BOOL bMinimized)
{
	CDialog::OnActivate(nState,pWndOther,bMinimized);

	if(show && !bMinimized){
		ResetState(TRUE,TRUE,TRUE,TRUE,FALSE);
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
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	PAINTSTRUCT pDC;
	::BeginPaint(m_hWnd,&pDC);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(!pModDoc || !m_pSndFile){
		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		return;
	}

	CHAR s[256];
	UINT i,ii,c=0,l=0;
	UINT nColns = CM_NB_COLS;
	UINT nChannels = m_pSndFile->m_nChannels;
	UINT nLines = nChannels / nColns + (nChannels % nColns ? 1 : 0);
	CRect client,btn;

	GetWindowRect(&btn);
	GetClientRect(&client);
	client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);

	UINT chnSizeX = (client.right - client.left) / (int)nColns;
	UINT chnSizeY = (client.bottom - client.top) / (int)nLines;

	if(chnSizeY != CM_BT_HEIGHT){
		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		CWnd::SetWindowPos(NULL,0,0,btn.Width(),btn.Height()+(CM_BT_HEIGHT-chnSizeY)*nLines,SWP_NOMOVE | SWP_NOZORDER);
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
		for(i = 0 ; i < nChannels ; i++){
			ii = pattern[i];
			if(select[ii]){
				btn = move[ii];
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

	if(pDC.fErase || (ok && nChannelsOld != nChannels)){
		FillRect(pDC.hdc,&intersection,CMainFrame::brushHighLight);
		FrameRect(pDC.hdc,&client,CMainFrame::brushBlack);
		nChannelsOld = nChannels;
	}

	client.SetRect(client.left + 8,client.top + 6,client.right - 6,client.bottom - 6);

	HBRUSH red = CreateSolidBrush(RGB(192,96,96));
	HBRUSH green = CreateSolidBrush(RGB(96,192,96));

	for(i = 0 ; i < nChannels ; i++){

		ii = pattern[i];

		if(m_pSndFile->ChnSettings[ii].szName[0] > 0x20)
	        wsprintf(s, "%d: %s", (ii+1), m_pSndFile->ChnSettings[ii].szName);
		else
			wsprintf(s, "Channel %d", ii+1);

		btn.left = client.left + c * chnSizeX + 3;
		btn.right = btn.left + chnSizeX - 3;
		btn.top = client.top + l * chnSizeY + 3;
		btn.bottom = btn.top + chnSizeY - 3;

		ok = intersection.IntersectRect(&pDC.rcPaint,&client);
		if(ok) DrawChannelButton(pDC.hdc, &btn, s, select[ii], removed[ii] ? FALSE : TRUE, DT_RIGHT | DT_VCENTER, NULL);

		btn.right = btn.left + chnSizeX / 7;

		btn.left += 3;
		btn.right -= 3;
		btn.top += 3;
		btn.bottom -= 3;

		switch(currentTab){
			case 0:
				if(m_pSndFile->ChnSettings[ii].dwFlags & CHN_MUTE) FillRect(pDC.hdc,&btn,red);
				else if(m_pSndFile->ChnSettings[ii].dwFlags & CHN_SOLO) FillRect(pDC.hdc,&btn,green);
				else FillRect(pDC.hdc,&btn,CMainFrame::brushHighLight);
				break;
			case 1:
				UINT r;
				if(pModDoc) r = pModDoc->IsChannelRecord(ii);
				else r = 0;
				if(r == 1) FillRect(pDC.hdc,&btn,green);
				else if(r == 2) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,CMainFrame::brushHighLight);
				break;
			case 2:
				if(m_pSndFile->ChnSettings[ii].dwFlags & CHN_NOFX) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,green);
				break;
			case 3:
				if(removed[ii]) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,green);
				break;
		}

		if(!removed[ii]){
			HGDIOBJ oldpen = ::SelectObject(pDC.hdc, CMainFrame::penLightGray);
			::MoveToEx(pDC.hdc, btn.right, btn.top-2, NULL);
			::LineTo(pDC.hdc, btn.right, btn.bottom+1);
			::SelectObject(pDC.hdc, oldpen);
		}
		FrameRect(pDC.hdc,&btn,CMainFrame::brushBlack);

		c++;
		if(c >= nColns) { c = 0; l++; }
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

BOOL CChannelManagerDlg::ButtonHit(CPoint point, UINT * id, CRect * invalidate)
{
	CRect client;
	GetClientRect(&client);
	client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);

	if(!PtInRect(client,point)) return FALSE;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile){
		//UINT nChannels = m_pSndFile->m_nChannels;
		UINT nColns = CM_NB_COLS;
		//UINT nLines = nChannels / nColns + (nChannels % nColns ? 1 : 0);

		int x = point.x - client.left;
		int y = point.y - client.top;

		int dx = (client.right - client.left) / (int)nColns;
		int dy = CM_BT_HEIGHT;

		x = x / dx;
		y = y / dy;
		int n = y * nColns + x;
		if(n >= 0 && n < (int)m_pSndFile->m_nChannels){
			if(id) *id = n;
			if(invalidate){
				invalidate->left = client.left + x * dx;
				invalidate->right = invalidate->left + dx;
				invalidate->top = client.top + y * dy;
				invalidate->bottom = invalidate->top + dy;
			}
			return TRUE;
		}
	}
	return FALSE;
}

void CChannelManagerDlg::ResetState(BOOL selection, BOOL move, BOOL button, BOOL internal, BOOL order)
{
	for(int i = 0 ; i < MAX_BASECHANNELS ; i++){
		if(selection)
			select[pattern[i]] = FALSE;
		if(button)
			state[pattern[i]]  = FALSE;
		if(order){
			pattern[i] = i;
			removed[i] = FALSE;
		}
	}
	if(move || internal){
		leftButton = FALSE;
		rightButton = FALSE;
	}
	if(move) moveRect = FALSE;
	if(internal) mouseTracking = FALSE;

	if(order) nChannelsOld = 0;
}

LRESULT CChannelManagerDlg::OnMouseLeave(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	if(!m_hWnd || show == FALSE) return 0;

	EnterCriticalSection(&applying);
	mouseTracking = FALSE;
	ResetState(FALSE,TRUE,FALSE,TRUE);
	LeaveCriticalSection(&applying);

	return 0;
}

LRESULT CChannelManagerDlg::OnMouseHover(WPARAM /*wparam*/, LPARAM /*lparam*/)
{
	if(!m_hWnd || show == FALSE) return 0;

	EnterCriticalSection(&applying);
	mouseTracking = FALSE;
	LeaveCriticalSection(&applying);

	return 0;
}

void CChannelManagerDlg::OnMouseMove(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	if(!mouseTracking){
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE|TME_HOVER;
		tme.dwHoverTime = 1;
		mouseTracking = _TrackMouseEvent(&tme); 
	}

	if(!leftButton && !rightButton){
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
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(moveRect && m_pSndFile){
		UINT n,i,k,newpat[MAX_BASECHANNELS];

		k = 0xffff;
		BOOL hit = ButtonHit(point,&n,NULL);
		for(i = 0 ; i < m_pSndFile->m_nChannels ; i++) if(k == 0xffff && select[pattern[i]]) k = i;

		if(hit && m_pSndFile && k != 0xffff){
			i = 0;
			k = 0;
			while(i < n){
				while(i < n && select[pattern[i]]) i++;
				if(i < n && !select[pattern[i]]){
					newpat[k] = pattern[i];
					pattern[i] = 0xffff;
					k++;
					i++;
				}
			}
			for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				if(pattern[i] != 0xffff && select[pattern[i]]){
					newpat[k] = pattern[i];
					pattern[i] = 0xffff;
					k++;
				}
			}
			i = 0;
			while(i < m_pSndFile->m_nChannels){
				while(i < m_pSndFile->m_nChannels && pattern[i] == 0xffff) i++;
				if(i < m_pSndFile->m_nChannels && pattern[i] != 0xffff){
					newpat[k] = pattern[i];
					k++;
					i++;
				}
			}
			for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				pattern[i] = newpat[i];
				select[i] = FALSE;
			}
		}

		moveRect = FALSE;
		nChannelsOld = 0;
		InvalidateRect(NULL,FALSE);
	}

	leftButton = FALSE;

	for(int i = 0 ; i < MAX_BASECHANNELS ; i++) state[pattern[i]] = FALSE;
	if(pModDoc) pModDoc->UpdateAllViews(NULL,0xff,NULL);

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnLButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	if(!ButtonHit(point,NULL,NULL)) ResetState(TRUE,FALSE,FALSE,FALSE);

	leftButton = TRUE;
	MouseEvent(nFlags,point,CM_BT_LEFT);
	omx = point.x;
	omy = point.y;

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnRButtonUp(UINT /*nFlags*/,CPoint /*point*/)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	ResetState(FALSE,FALSE,TRUE,FALSE);

	rightButton = FALSE;
	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm->GetActiveDoc();
	if(pModDoc) pModDoc->UpdateAllViews(NULL,0xff,NULL);

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnRButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	if(moveRect){
		ResetState(TRUE,TRUE,FALSE,FALSE,FALSE);
		nChannelsOld = 0;
		InvalidateRect(NULL, FALSE);
		rightButton = TRUE;
	}
	else{
		rightButton = TRUE;
		MouseEvent(nFlags,point,CM_BT_RIGHT);
		omx = point.x;
		omy = point.y;
	}

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::MouseEvent(UINT nFlags,CPoint point,BYTE button)
{
	UINT n;
	CRect client,invalidate;
	BOOL hit = ButtonHit(point,&n,&invalidate);
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
				select[n] = TRUE;
			}
			else if(button == CM_BT_RIGHT) select[n] = FALSE;
		}
		else if(!removed[n] || currentTab == 3){
			switch(currentTab){
				case 0:
					if(button == CM_BT_LEFT){
						if(!pModDoc->IsChannelSolo(n) || pModDoc->IsChannelMuted(n)){
							GetClientRect(&client);
							pModDoc->MuteChannel(n,FALSE);
							pModDoc->SoloChannel(n,TRUE);
							for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) if(i != n) pModDoc->MuteChannel(i,TRUE);
							client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);
							invalidate = client;
						}
						else pModDoc->SoloChannel(n,FALSE);
					}
					else{
						if(pModDoc->IsChannelSolo(n)) pModDoc->SoloChannel(n,FALSE);
						pModDoc->MuteChannel(n,!pModDoc->IsChannelMuted(n));
					}
					pModDoc->SetModified();
					pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | ((n/CHANNELS_IN_TAB) << HINT_SHIFT_CHNTAB));
					break;
				case 1:
					UINT r;
					r = pModDoc->IsChannelRecord(n);
					if(!r || r != (button == CM_BT_LEFT ? 1 : 2)){
						if(button == CM_BT_LEFT) pModDoc->Record1Channel(n);
						else pModDoc->Record2Channel(n);
					}
					else{
						pModDoc->Record1Channel(n,FALSE);
						pModDoc->Record2Channel(n,FALSE);
					}
					break;
				case 2:
					if(button == CM_BT_LEFT) pModDoc->NoFxChannel(n,FALSE);
					else pModDoc->NoFxChannel(n,TRUE);
					pModDoc->SetModified();
					pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | ((n/CHANNELS_IN_TAB) << HINT_SHIFT_CHNTAB));
					break;
				case 3:
					if(button == CM_BT_LEFT){
						move[n] = invalidate;
						select[n] = TRUE;
					}
					if(button == CM_BT_RIGHT){
						select[n] = FALSE;
						removed[n] = !removed[n];
					}

					if(select[n] || button == 0){
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
						moveRect = TRUE;
					}
					break;
			}
		}

		state[n] = TRUE;
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
