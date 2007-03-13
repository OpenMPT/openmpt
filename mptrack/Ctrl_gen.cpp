#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "dlg_misc.h"
#include "ctrl_gen.h"
#include "view_gen.h"
#include "math.h"

// -> CODE#0015
// -> DESC="channels management dlg"
#include "Ctrl_pat.h"
// -! NEW_FEATURE#0015

BEGIN_MESSAGE_MAP(CCtrlGeneral, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlGeneral)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON_MODTYPE,			OnSongProperties)
	ON_COMMAND(IDC_BUTTON_PLAYERPROPS,		OnPlayerProperties)
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
	ON_EN_CHANGE(IDC_EDIT_VSTIVOL,			OnVSTiVolChanged)
	ON_EN_CHANGE(IDC_EDIT_SAMPLEPA,			OnSamplePAChanged)
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
	DDX_Control(pDX, IDC_EDIT_VSTIVOL,		m_EditVSTiVol);
	DDX_Control(pDX, IDC_SPIN_VSTIVOL,		m_SpinVSTiVol);
	DDX_Control(pDX, IDC_EDIT_SAMPLEPA,		m_EditSamplePA);
	DDX_Control(pDX, IDC_SPIN_SAMPLEPA,		m_SpinSamplePA);
	DDX_Control(pDX, IDC_EDIT_RESTARTPOS,	m_EditRestartPos);
	DDX_Control(pDX, IDC_SPIN_RESTARTPOS,	m_SpinRestartPos);


	DDX_Control(pDX, IDC_SLIDER_SONGTEMPO,	m_SliderTempo);
	DDX_Control(pDX, IDC_SLIDER_VSTIVOL,	m_SliderVSTiVol);
	DDX_Control(pDX, IDC_SLIDER_GLOBALVOL,	m_SliderGlobalVol);
	DDX_Control(pDX, IDC_SLIDER_SAMPLEPREAMP,	m_SliderSamplePreAmp);

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
// -> CODE#0016
// -> DESC="default tempo update"
//	m_SpinTempo.SetRange(32, 255);	// 255 bpm max
	m_SpinTempo.SetRange(32, 512);
	m_SpinSpeed.SetRange(1, 64);
// -! BEHAVIOUR_CHANGE#0016
	m_SpinGlobalVol.SetRange(0, 128);
	m_SpinSamplePA.SetRange(0, 2000);
	m_SpinVSTiVol.SetRange(0, 2000);
	m_SpinRestartPos.SetRange(0, 255);
	
	m_SliderTempo.SetRange(0, 480);
	m_SliderGlobalVol.SetRange(0, MAX_SLIDER_GLOBAL_VOL);
	m_SliderVSTiVol.SetRange(0, MAX_SLIDER_VSTI_VOL);
	m_SliderSamplePreAmp.SetRange(0, MAX_SLIDER_SAMPLE_VOL);
	

	
	// -! BEHAVIOUR_CHANGE#0016
	m_ComboResampling.AddString("None");
	m_ComboResampling.AddString("Linear");
	m_ComboResampling.AddString("Cubic spline");
	//rewbs.resamplerConf
	m_ComboResampling.AddString("Polyphase");
	m_ComboResampling.AddString("XMMS-Modplug");	
	//end rewbs.resamplerConf
	m_bEditsLocked=false;
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
	PostViewMessage(VIEWMSG_SETACTIVE, NULL);
	SetFocus();
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
		UINT i = 0;
		for (i=0; i<MAX_ORDERS; i++) if (m_pSndFile->Order[i] == 0xFF) break;
		m_SpinRestartPos.SetRange(0, i);
	}
	if (dwHint & HINT_MODGENERAL)
	{
		if (!m_bEditsLocked) {
			m_EditTitle.SetWindowText(m_pSndFile->m_szNames[0]);
			wsprintf(s, "%d", m_pSndFile->m_nDefaultTempo);
			m_EditTempo.SetWindowText(s);
			wsprintf(s, "%d", m_pSndFile->m_nDefaultSpeed);
			m_EditSpeed.SetWindowText(s);
			wsprintf(s, "%d", m_pSndFile->m_nDefaultGlobalVolume / 2);
			m_EditGlobalVol.SetWindowText(s);
			wsprintf(s, "%d", m_pSndFile->m_nRestartPos);
			m_EditRestartPos.SetWindowText(s);
			wsprintf(s, "%d", m_pSndFile->m_nVSTiVolume);
			m_EditVSTiVol.SetWindowText(s);
			wsprintf(s, "%d", m_pSndFile->m_nSamplePreAmp);
			m_EditSamplePA.SetWindowText(s);
			wsprintf(s, "%d", m_pSndFile->m_nRestartPos);
			m_EditRestartPos.SetWindowText(s);
		}

		m_SliderGlobalVol.SetPos(MAX_SLIDER_GLOBAL_VOL-m_pSndFile->m_nDefaultGlobalVolume);
		m_SliderVSTiVol.SetPos(MAX_SLIDER_VSTI_VOL-m_pSndFile->m_nVSTiVolume);
		m_SliderSamplePreAmp.SetPos(MAX_SLIDER_SAMPLE_VOL-m_pSndFile->m_nSamplePreAmp);
		m_SliderTempo.SetPos(480 - m_pSndFile->m_nDefaultTempo + 32);



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
		m_EditVSTiVol.EnableWindow(b);
		m_SpinVSTiVol.EnableWindow(b);
		m_EditSamplePA.EnableWindow(b);
		m_SpinSamplePA.EnableWindow(b);
		m_SliderSamplePreAmp.EnableWindow(b);
		m_SliderVSTiVol.EnableWindow(b);

		// MOD Type
		LPCSTR pszModType = "MOD (ProTracker)";
		switch(m_pSndFile->m_nType)
		{
		case MOD_TYPE_S3M:	pszModType = "S3M (ScreamTracker)"; break;
		case MOD_TYPE_XM:	pszModType = "XM (FastTracker 2)"; break;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//		case MOD_TYPE_IT:	pszModType = "IT (Impulse Tracker)"; break;
		case MOD_TYPE_IT:	pszModType = m_pSndFile->m_dwSongFlags & SONG_ITPROJECT ? "ITP (IT Project)" : "IT (Impulse Tracker)"; break;
// -! NEW_FEATURE#0023
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
		CSliderCtrl* pSlider = (CSliderCtrl*) pscroll;

		if (pSlider==&m_SliderTempo) {
			int tempo = 480 - m_SliderTempo.GetPos() + 32;
			if ((tempo >= 32) && (tempo <= 512) && (tempo != m_pSndFile->m_nDefaultTempo)) {
				m_pSndFile->m_nDefaultTempo = tempo;
				m_pSndFile->m_nMusicTempo = tempo;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		else if (pSlider==&m_SliderGlobalVol) {
			int gv = MAX_SLIDER_GLOBAL_VOL - m_SliderGlobalVol.GetPos();
			if ((gv >= 0) && (gv <= MAX_SLIDER_GLOBAL_VOL) && (gv != m_pSndFile->m_nDefaultGlobalVolume)) {
				m_pSndFile->m_nGlobalVolume = gv;
				m_pSndFile->m_nDefaultGlobalVolume = gv;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		else if (pSlider==&m_SliderSamplePreAmp) {
			int spa = MAX_SLIDER_SAMPLE_VOL - m_SliderSamplePreAmp.GetPos();
			if ((spa >= 0) && (spa <= MAX_SLIDER_SAMPLE_VOL) && (spa != m_pSndFile->m_nSamplePreAmp)) {
				m_pSndFile->m_nSamplePreAmp = spa;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		else if (pSlider==&m_SliderVSTiVol) {
			int vv = MAX_SLIDER_VSTI_VOL - m_SliderVSTiVol.GetPos();
			if ((vv >= 0) && (vv <= MAX_SLIDER_VSTI_VOL) && (vv != m_pSndFile->m_nVSTiVolume)) {
				m_pSndFile->m_nVSTiVolume = vv;
				m_pSndFile->RecalculateGainForAllPlugs();
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		UpdateView(HINT_MODGENERAL, NULL);
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
			if ((n >= 32) && (n <= 512) && (n != m_pSndFile->m_nDefaultTempo))
			{
				m_bEditsLocked=true;
				m_EditTempo.SetModify(FALSE);
				m_pSndFile->m_nDefaultTempo = n;
				m_pSndFile->m_nMusicTempo = n;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
				UpdateView(HINT_MODGENERAL, NULL); 
				m_bEditsLocked=false;
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
			if ((n >= 1) && (n <= 64) && (n != m_pSndFile->m_nDefaultSpeed)) {
				m_bEditsLocked=true;
				m_EditSpeed.SetModify(FALSE);
				m_pSndFile->m_nDefaultSpeed = n;
				m_pSndFile->m_nMusicSpeed = n;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
				m_bEditsLocked=false;
			}
		}
	}
}


void CCtrlGeneral::OnVSTiVolChanged()
//-------------------------------------
{
	CHAR s[32];
	if ((m_pSndFile) && (m_pModDoc) && (m_bInitialized)) {
		m_EditVSTiVol.GetWindowText(s, sizeof(s));
		if (s[0]) {
			int n = atoi(s);
			if ((n >= 0) && (n <= 2000) && (n != m_pSndFile->m_nVSTiVolume)) {
				m_bEditsLocked=true;
				m_pSndFile->m_nVSTiVolume = n;
				m_pSndFile->RecalculateGainForAllPlugs();
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
				UpdateView(HINT_MODGENERAL, NULL); 
				m_bEditsLocked=false;
			}
		}
	}
}

void CCtrlGeneral::OnSamplePAChanged()
//-------------------------------------
{
	CHAR s[32];
	if ((m_pSndFile) && (m_pModDoc) && (m_bInitialized)) {
		m_EditSamplePA.GetWindowText(s, sizeof(s));
		if (s[0]) {
			int n = atoi(s);
			if ((n >= 0) && (n <= 2000) && (n != m_pSndFile->m_nSamplePreAmp)) {
				m_bEditsLocked=true;
				m_pSndFile->m_nSamplePreAmp = n;
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
				UpdateView(HINT_MODGENERAL, NULL); 
				m_bEditsLocked=false;
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
					m_bEditsLocked=true;
					m_EditGlobalVol.SetModify(FALSE);
					m_pSndFile->m_nDefaultGlobalVolume = n << 1;
					m_pSndFile->m_nGlobalVolume = n << 1;
					m_pModDoc->SetModified();
					m_pModDoc->UpdateAllViews(NULL, HINT_MODGENERAL, this);
					UpdateView(HINT_MODGENERAL, NULL);
					m_bEditsLocked=false;
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

void CCtrlGeneral::OnPlayerProperties()
//-------------------------------------
{
	CMainFrame::m_nLastOptionsPage = 2; //OPTIONS_PAGE_PLAYER
	CMainFrame::GetMainFrame()->OnViewOptions();
}

void CCtrlGeneral::OnSongProperties()
//----------------------------------
{
	m_pModDoc->SongProperties();
	/*
	CModTypeDlg dlg(m_pSndFile, this);
	if (dlg.DoModal() == IDOK)
	{
		BOOL bShowLog = FALSE;
		m_pModDoc->ClearLog();
		if(dlg.m_nType)	{
			if (!m_pModDoc->ChangeModType(dlg.m_nType)) return;
			bShowLog = TRUE;
		}
		if ((dlg.m_nChannels >= 4) && (dlg.m_nChannels != m_pSndFile->m_nChannels)) {
			if(m_pModDoc->ChangeNumChannels(dlg.m_nChannels)) bShowLog = TRUE;
			if(CChannelManagerDlg::sharedInstance(FALSE) && CChannelManagerDlg::sharedInstance()->IsDisplayed())
				CChannelManagerDlg::sharedInstance()->Update();
		}
		if (bShowLog) m_pModDoc->ShowLog("Conversion Status", this);
		m_pModDoc->SetModified();
	}
	*/
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


BOOL CCtrlGeneral::GetToolTipText(UINT uId, LPSTR pszText) {
//----------------------------------------------------------
	if ((pszText) && (uId))
	{
		if (!m_pSndFile->m_pConfig->getDisplayDBValues()) {
			wsprintf(pszText, "Use a more recent mixmode to see dB offsets.");
			return TRUE;
		}
		

		switch(uId) 	{
			case IDC_SLIDER_SAMPLEPREAMP:
				setAsDecibels(pszText, m_pSndFile->m_nSamplePreAmp, m_pSndFile->m_pConfig->getNormalSamplePreAmp());
				return TRUE;
				break;
			case IDC_SLIDER_VSTIVOL:
				setAsDecibels(pszText, m_pSndFile->m_nVSTiVolume, m_pSndFile->m_pConfig->getNormalVSTiVol());
				return TRUE;
				break;
			case IDC_SLIDER_GLOBALVOL:
				setAsDecibels(pszText, m_pSndFile->m_nGlobalVolume, m_pSndFile->m_pConfig->getNormalGlobalVol());
				return TRUE;
				break;
		}
	}
	return FALSE;
	
}

void CCtrlGeneral::setAsDecibels(LPSTR stringToSet, double value, double valueAtZeroDB) {
//-------------------------------------------------------------------------------------

	if (value==0) {
		wsprintf(stringToSet, "-inf");
		return;
	}
	
	double changeFactor = value / valueAtZeroDB;
    double dB = 10*log(changeFactor);

	char sign = (dB>=0)?'+':' ';
	sprintf(stringToSet, "%c%.2f dB", sign, dB);
	return;

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


