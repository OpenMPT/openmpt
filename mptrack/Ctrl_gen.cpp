#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "dlg_misc.h"
#include "ctrl_gen.h"
#include "view_gen.h"

BEGIN_MESSAGE_MAP(CCtrlGeneral, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlGeneral)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON_MODTYPE,			OnChangeModType)
	ON_COMMAND(IDC_CHECK_LOOPSONG,			OnLoopSongChanged)
	ON_COMMAND(IDC_CHECK_BASS,				OnXBassChanged)
	ON_COMMAND(IDC_CHECK_SURROUND,			OnSurroundChanged)
	ON_COMMAND(IDC_CHECK_REVERB,			OnReverbChanged)
	ON_COMMAND(IDC_CHECK_EQ,				OnEqualizerChanged)
	ON_COMMAND(IDC_CHECK_AGC,				OnAGCChanged)
	ON_EN_CHANGE(IDC_EDIT_SONGTITLE,		OnTitleChanged)
	ON_EN_CHANGE(IDC_EDIT_TEMPO,			OnTempoChanged)
	ON_EN_CHANGE(IDC_EDIT_SPEED,			OnSpeedChanged)
	ON_EN_CHANGE(IDC_EDIT_GLOBALVOL,		OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT_RESTARTPOS,		OnRestartPosChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_RESAMPLING,	OnResamplingChanged)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		OnUpdatePosition)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCtrlGeneral::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------
{
	CModControlDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCtrlGeneral)
	DDX_Control(pDX, IDC_EDIT_SONGTITLE,	m_EditTitle);
	DDX_Control(pDX, IDC_EDIT_TEMPO,		m_EditTempo);
	DDX_Control(pDX, IDC_SPIN_TEMPO,		m_SpinTempo);
	DDX_Control(pDX, IDC_EDIT_SPEED,		m_EditSpeed);
	DDX_Control(pDX, IDC_SPIN_SPEED,		m_SpinSpeed);
	DDX_Control(pDX, IDC_EDIT_GLOBALVOL,	m_EditGlobalVol);
	DDX_Control(pDX, IDC_SPIN_GLOBALVOL,	m_SpinGlobalVol);
	DDX_Control(pDX, IDC_EDIT_RESTARTPOS,	m_EditRestartPos);
	DDX_Control(pDX, IDC_SPIN_RESTARTPOS,	m_SpinRestartPos);
	DDX_Control(pDX, IDC_SLIDER_SONGPREAMP,	m_SliderPreAmp);
	DDX_Control(pDX, IDC_EDIT_MODTYPE,		m_EditModType);
	DDX_Control(pDX, IDC_COMBO_RESAMPLING,	m_ComboResampling);
	DDX_Control(pDX, IDC_VUMETER_LEFT,		m_VuMeterLeft);
	DDX_Control(pDX, IDC_VUMETER_RIGHT,		m_VuMeterRight);
	//}}AFX_DATA_MAP
}


CCtrlGeneral::CCtrlGeneral()
//--------------------------
{
}


BOOL CCtrlGeneral::OnInitDialog()
//-------------------------------
{
	CModControlDlg::OnInitDialog();
	// Song Title
	m_EditTitle.SetLimitText(31);
	m_SpinTempo.SetRange(32, 255);
	m_SpinSpeed.SetRange(1, 31);
	m_SpinGlobalVol.SetRange(0, 128);
	m_SpinRestartPos.SetRange(0, 0);
	m_SliderPreAmp.SetRange(0, 100);
	m_SliderPreAmp.SetPos(100-48);
	m_ComboResampling.AddString("No Resampling");
	m_ComboResampling.AddString("Linear");
	m_ComboResampling.AddString("Cubic spline");
	m_ComboResampling.AddString("High quality");
	UpdateView(HINT_MODGENERAL|HINT_MODTYPE|HINT_MODSEQUENCE|HINT_MPTSETUP, NULL);
	OnActivatePage(0);
	m_bInitialized = TRUE;
	return FALSE;
}


CRuntimeClass *CCtrlGeneral::GetAssociatedViewClass()
//---------------------------------------------------
{
	return RUNTIME_CLASS(CViewGlobals);
}


void CCtrlGeneral::RecalcLayout()
//-------------------------------
{
}


void CCtrlGeneral::OnActivatePage(LPARAM)
//---------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (m_pModDoc) m_pModDoc->SetFollowWnd(m_hWnd, MPTNOTIFY_MASTERVU);
	if (pMainFrm) pMainFrm->SetFollowSong(m_pModDoc, m_hWnd, TRUE, MPTNOTIFY_MASTERVU);
	CMainFrame::EnableLowLatencyMode(FALSE);
}


void CCtrlGeneral::OnDeactivatePage()
//-----------------------------------
{
	if (m_pModDoc) m_pModDoc->SetFollowWnd(NULL, 0);
	m_VuMeterLeft.SetVuMeter(0);
	m_VuMeterRight.SetVuMeter(0);
}


void CCtrlGeneral::UpdateView(DWORD dwHint, CObject *pHint)
//---------------------------------------------------------
{
	CHAR s[256];
	if ((pHint == this) || (!m_pSndFile)) return;
	if (dwHint & HINT_MODSEQUENCE)
	{
		// Detecting max valid restart position
		for (UINT i=0; i<MAX_ORDERS; i++) if (m_pSndFile->Order[i] == 0xFF) break;
		m_SpinRestartPos.SetRange(0, i);
	}
	if (dwHint & HINT_MODGENERAL)
	{
		m_EditTitle.SetWindowText(m_pSndFile->m_szNames[0]);
		wsprintf(s, "%d", m_pSndFile->m_nDefaultTempo);
		m_EditTempo.SetWindowText(s);
		wsprintf(s, "%d", m_pSndFile->m_nDefaultSpeed);
		m_EditSpeed.SetWindowText(s);
		wsprintf(s, "%d", m_pSndFile->m_nDefaultGlobalVolume / 2);
		m_EditGlobalVol.SetWindowText(s);
		wsprintf(s, "%d", m_pSndFile->m_nRestartPos);
		m_EditRestartPos.SetWindowText(s);
		int n = 100 - m_pSndFile->m_nSongPreAmp;
		if (n > 100) n = 100;
		if (n < 0) n = 0;
		m_SliderPreAmp.SetPos(n);
	}
	if (dwHint & HINT_MODTYPE)
	{
		BOOL b = TRUE;
		if (m_pSndFile->m_nType == MOD_TYPE_MOD) b = FALSE;
		m_EditTempo.EnableWindow(b);
		m_SpinTempo.EnableWindow(b);
		m_EditSpeed.EnableWindow(b);
		m_SpinSpeed.EnableWindow(b);
		m_EditGlobalVol.EnableWindow(b);
		m_SpinGlobalVol.EnableWindow(b);
		// MOD Type
		LPCSTR pszModType = "MOD (ProTracker)";
		switch(m_pSndFile->m_nType)
		{
		case MOD_TYPE_S3M:	pszModType = "S3M (ScreamTracker)"; break;
		case MOD_TYPE_XM:	pszModType = "XM (FastTracker 2)"; break;
		case MOD_TYPE_IT:	pszModType = "IT (Impulse Tracker)"; break;
		}
		wsprintf(s, "%s, %d channels", pszModType, m_pSndFile->m_nChannels);
		m_EditModType.SetWindowText(s);
	}
	if (dwHint & HINT_MPTSETUP)
	{
		DWORD dwSetup = CMainFrame::m_dwQuality;
		m_ComboResampling.SetCurSel(CMainFrame::m_nSrcMode);
		CheckDlgButton(IDC_CHECK_LOOPSONG,	(CMainFrame::gbLoopSong) ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK_AGC,		(dwSetup & QUALITY_AGC) ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK_BASS,		(dwSetup & QUALITY_MEGABASS) ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK_REVERB,	(dwSetup & QUALITY_REVERB) ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK_SURROUND,	(dwSetup & QUALITY_SURROUND) ? TRUE : FALSE);
		CheckDlgButton(IDC_CHECK_EQ,		(dwSetup & QUALITY_EQ) ? TRUE : FALSE);
	}
	if (dwHint & HINT_MPTOPTIONS)
	{
		m_VuMeterLeft.InvalidateRect(NULL, FALSE);
		m_VuMeterRight.InvalidateRect(NULL, FALSE);
	}
}


void CCtrlGeneral::OnVScroll(UINT code, UINT pos, CScrollBar *pscroll)
//--------------------------------------------------------------------
{
	CDialog::OnVScroll(code, pos, pscroll);
	if ((m_pSndFile) && (m_pModDoc) && (m_bInitialized))
	{
		UINT n = 100 - m_SliderPreAmp.GetPos();
		if ((n > 0) && (n <= 100) && (n != m_pSndFile->m_nSongPreAmp))
		{
			m_pSndFile->m_nSongPreAmp = n;
			m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
		}
	}
}


void CCtrlGeneral::OnTitleChanged()
//---------------------------------
{
	CHAR s[80];
	if ((!m_pSndFile) || (!m_EditTitle.m_hWnd) || (!m_EditTitle.GetModify())) return;
	memset(s, 0, sizeof(s));
	m_EditTitle.GetWindowText(s, sizeof(s));
	s[31] = 0;
	if (strcmp(m_pSndFile->m_szNames[0], s))
	{
		memcpy(m_pSndFile->m_szNames[0], s, 32);
		if (m_pModDoc)
		{
			m_EditTitle.SetModify(FALSE);
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
		}
	}
}


void CCtrlGeneral::OnTempoChanged()
//---------------------------------
{
	CHAR s[32];
	if ((m_pSndFile) && (m_pModDoc) && (m_bInitialized))
	{
		m_EditTempo.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = atoi(s);
			if ((n >= 32) && (n <= 255) && (n != m_pSndFile->m_nDefaultTempo))
			{
				m_EditTempo.SetModify(FALSE);
				m_pSndFile->m_nDefaultTempo = n;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}
	}
}


void CCtrlGeneral::OnSpeedChanged()
//---------------------------------
{
	CHAR s[32];
	if ((m_pSndFile) && (m_pModDoc) && (m_bInitialized))
	{
		m_EditSpeed.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = atoi(s);
			if ((n >= 1) && (n < 32) && (n != m_pSndFile->m_nDefaultSpeed))
			{
				m_EditSpeed.SetModify(FALSE);
				m_pSndFile->m_nDefaultSpeed = n;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}
	}
}


void CCtrlGeneral::OnGlobalVolChanged()
//-------------------------------------
{
	CHAR s[32];
	if ((m_pSndFile) && (m_pModDoc) && (m_bInitialized))
	{
		m_EditGlobalVol.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = atoi(s);
			if (n <= 128)
			{
				UINT n0 = m_pSndFile->m_nDefaultGlobalVolume / 2;
				if (n != n0)
				{
					m_EditGlobalVol.SetModify(FALSE);
					m_pSndFile->m_nDefaultGlobalVolume = n << 1;
					m_pModDoc->SetModified();
					m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
				}
			}
		}
	}
}


void CCtrlGeneral::OnRestartPosChanged()
//--------------------------------------
{
	CHAR s[32];
	if ((m_pSndFile) && (m_pModDoc) && (m_bInitialized))
	{
		m_EditRestartPos.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = atoi(s);
			if (n < MAX_ORDERS)
			{
				for (UINT i=0; i<=n; i++) if (m_pSndFile->Order[i] == 0xFF) return;
				if (n != m_pSndFile->m_nRestartPos)
				{
					m_EditRestartPos.SetModify(FALSE);
					m_pSndFile->m_nRestartPos = n;
					m_pModDoc->SetModified();
					m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
				}
			}
		}
	}
}


void CCtrlGeneral::OnChangeModType()
//----------------------------------
{
	CModTypeDlg dlg(m_pSndFile, this);
	if (dlg.DoModal() == IDOK)
	{
		BOOL bShowLog = FALSE;
		m_pModDoc->ClearLog();
		if ((dlg.m_nType) && (dlg.m_nType != m_pSndFile->m_nType))
		{
			if (!m_pModDoc->ChangeModType(dlg.m_nType)) return;
			bShowLog = TRUE;
		}
		if ((dlg.m_nChannels >= 4) && (dlg.m_nChannels != m_pSndFile->m_nChannels))
		{
			if (!m_pModDoc->ChangeNumChannels(dlg.m_nChannels)) return;
			bShowLog = TRUE;
		}
		if (bShowLog) m_pModDoc->ShowLog("Conversion Status", this);
	}
}


void CCtrlGeneral::OnResamplingChanged()
//--------------------------------------
{
	DWORD n = m_ComboResampling.GetCurSel();
	if ((n < NUM_SRC_MODES) && (n != CMainFrame::m_nSrcMode))
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetupPlayer(CMainFrame::m_dwQuality, n);
	}
}


void CCtrlGeneral::OnLoopSongChanged()
//------------------------------------
{
	CMainFrame::gbLoopSong = IsDlgButtonChecked(IDC_CHECK_LOOPSONG);
	CModDoc *pModDoc = GetDocument();
	if (pModDoc)
	{
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		if (pSndFile) pSndFile->SetRepeatCount((CMainFrame::gbLoopSong) ? -1 : 0);
	}
}


void CCtrlGeneral::OnAGCChanged()
//-------------------------------
{
	BOOL b = IsDlgButtonChecked(IDC_CHECK_AGC);
	DWORD dwQuality = CMainFrame::m_dwQuality & ~QUALITY_AGC;
	if (b) dwQuality |= QUALITY_AGC;
	if (dwQuality != CMainFrame::m_dwQuality)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetupPlayer(dwQuality, CMainFrame::m_nSrcMode);
	}
}


void CCtrlGeneral::OnXBassChanged()
//---------------------------------
{
	BOOL b = IsDlgButtonChecked(IDC_CHECK_BASS);
	DWORD dwQuality = CMainFrame::m_dwQuality & ~QUALITY_MEGABASS;
	if (b) dwQuality |= QUALITY_MEGABASS;
	if (dwQuality != CMainFrame::m_dwQuality)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetupPlayer(dwQuality, CMainFrame::m_nSrcMode);
	}
}


void CCtrlGeneral::OnReverbChanged()
//----------------------------------
{
	BOOL b = IsDlgButtonChecked(IDC_CHECK_REVERB);
	DWORD dwQuality = CMainFrame::m_dwQuality & ~QUALITY_REVERB;
	if (b) dwQuality |= QUALITY_REVERB;
	if (dwQuality != CMainFrame::m_dwQuality)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetupPlayer(dwQuality, CMainFrame::m_nSrcMode);
	}
}


void CCtrlGeneral::OnSurroundChanged()
//------------------------------------
{
	BOOL b = IsDlgButtonChecked(IDC_CHECK_SURROUND);
	DWORD dwQuality = CMainFrame::m_dwQuality & ~QUALITY_SURROUND;
	if (b) dwQuality |= QUALITY_SURROUND;
	if (dwQuality != CMainFrame::m_dwQuality)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetupPlayer(dwQuality, CMainFrame::m_nSrcMode);
	}
}


void CCtrlGeneral::OnEqualizerChanged()
//-------------------------------------
{
	BOOL b = IsDlgButtonChecked(IDC_CHECK_EQ);
	DWORD dwQuality = CMainFrame::m_dwQuality & ~QUALITY_EQ;
	if (b) dwQuality |= QUALITY_EQ;
	if (dwQuality != CMainFrame::m_dwQuality)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->SetupPlayer(dwQuality, CMainFrame::m_nSrcMode);
	}
}


LRESULT CCtrlGeneral::OnUpdatePosition(WPARAM, LPARAM lParam)
//-----------------------------------------------------------
{
	MPTNOTIFICATION *pnotify = (MPTNOTIFICATION *)lParam;
	if (pnotify)
	{
		if (pnotify->dwType & MPTNOTIFY_MASTERVU)
		{
			m_VuMeterLeft.SetVuMeter(pnotify->dwPos[0]);
			m_VuMeterRight.SetVuMeter(pnotify->dwPos[1]);
		} else
		{
			m_VuMeterLeft.SetVuMeter(0);
			m_VuMeterRight.SetVuMeter(0);
		}
	}
	return 0;
}



////////////////////////////////////////////////////////////////////////////////
//
// CVuMeter
//

BEGIN_MESSAGE_MAP(CVuMeter, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()


void CVuMeter::OnPaint()
//----------------------
{
	CRect rect;
	CPaintDC dc(this);
	HDC hdc = dc.m_hDC;
	GetClientRect(&rect);
	FillRect(hdc, &rect, CMainFrame::brushBlack);
	m_nDisplayedVu = -1;
	DrawVuMeter(hdc);
}


VOID CVuMeter::SetVuMeter(LONG lVuMeter)
//--------------------------------------
{
	lVuMeter >>= 8;
	if (lVuMeter != m_nVuMeter)
	{
		m_nVuMeter = lVuMeter;
		CClientDC dc(this);
		DrawVuMeter(dc.m_hDC);
	}
}


VOID CVuMeter::DrawVuMeter(HDC hdc)
//---------------------------------
{
	LONG vu;
	CRect rect;

	GetClientRect(&rect);
	HGDIOBJ oldpen = SelectObject(hdc, CMainFrame::penBlack);
	vu = (m_nVuMeter * (rect.bottom-rect.top)) >> 8;
	int cy = rect.bottom - rect.top;
	if (cy < 1) cy = 1;
	for (int ry=rect.bottom-1; ry>rect.top; ry-=2)
	{
		int y0 = rect.bottom - ry;
		int n = (y0 * NUM_VUMETER_PENS) / cy;
		if (n < 0) n = 0;
		if (n >= NUM_VUMETER_PENS) n = NUM_VUMETER_PENS-1;
		if (vu < y0)
		{
			n += NUM_VUMETER_PENS;
		}
		SelectObject(hdc, CMainFrame::gpenVuMeter[n]);
		MoveToEx(hdc, rect.left, ry, NULL);
		LineTo(hdc, rect.right, ry);
	}
	SelectObject(hdc, oldpen);
	m_nDisplayedVu = m_nVuMeter;
}
