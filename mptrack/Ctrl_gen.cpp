/*
 * ctrl_gen.cpp
 * ------------
 * Purpose: General tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "dlg_misc.h"
#include "ctrl_gen.h"
#include "view_gen.h"
#include "math.h"
#include "../common/misc_util.h"

// -> CODE#0015
// -> DESC="channels management dlg"
#include "Ctrl_pat.h"
#include "ctrl_gen.h"
// -! NEW_FEATURE#0015

BEGIN_MESSAGE_MAP(CCtrlGeneral, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlGeneral)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON_MODTYPE,			OnSongProperties)
	ON_COMMAND(IDC_BUTTON_PLAYERPROPS,		OnPlayerProperties)
	ON_COMMAND(IDC_CHECK_LOOPSONG,			OnLoopSongChanged)
	ON_EN_CHANGE(IDC_EDIT_SONGTITLE,		OnTitleChanged)
	ON_EN_CHANGE(IDC_EDIT_TEMPO,			OnTempoChanged)
	ON_EN_CHANGE(IDC_EDIT_SPEED,			OnSpeedChanged)
	ON_EN_CHANGE(IDC_EDIT_GLOBALVOL,		OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT_RESTARTPOS,		OnRestartPosChanged)
	ON_EN_CHANGE(IDC_EDIT_VSTIVOL,			OnVSTiVolChanged)
	ON_EN_CHANGE(IDC_EDIT_SAMPLEPA,			OnSamplePAChanged)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		OnUpdatePosition)
	ON_EN_SETFOCUS(IDC_EDIT_SONGTITLE,		OnEnSetfocusEditSongtitle)
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
	DDX_Control(pDX, IDC_VUMETER_LEFT,		m_VuMeterLeft);
	DDX_Control(pDX, IDC_VUMETER_RIGHT,		m_VuMeterRight);
	//}}AFX_DATA_MAP
}


CCtrlGeneral::CCtrlGeneral(CModControlView &parent, CModDoc &document) : CModControlDlg(parent, document)
//-------------------------------------------------------------------------------------------------------
{
}


BOOL CCtrlGeneral::OnInitDialog()
//-------------------------------
{
	const CModSpecifications specs = m_sndFile.GetModSpecifications();
	CModControlDlg::OnInitDialog();
	// Song Title
	m_EditTitle.SetLimitText(specs.modNameLengthMax);

// -> CODE#0016
// -> DESC="default tempo update"
//	m_SpinTempo.SetRange(32, 255);	// 255 bpm max

	if(m_sndFile.GetType() & MOD_TYPE_S3M)
	{
		// S3M HACK: ST3 will ignore speed 255, even though it can be used with Axx.
		m_SpinSpeed.SetRange(1, 254);
	} else
	{
		m_SpinSpeed.SetRange((short)specs.speedMin, (short)specs.speedMax);
	}
	m_SpinTempo.SetRange((short)specs.tempoMin, (short)specs.tempoMax);

// -! BEHAVIOUR_CHANGE#0016

	m_SpinGlobalVol.SetRange(0, (short)(256 / GetGlobalVolumeFactor()));
	m_SpinSamplePA.SetRange(0, 2000);
	m_SpinVSTiVol.SetRange(0, 2000);
	m_SpinRestartPos.SetRange(0, 255);
	
	m_SliderTempo.SetRange(0, specs.tempoMax - specs.tempoMin);
	m_SliderGlobalVol.SetRange(0, MAX_SLIDER_GLOBAL_VOL);
	m_SliderVSTiVol.SetRange(0, MAX_SLIDER_VSTI_VOL);
	m_SliderSamplePreAmp.SetRange(0, MAX_SLIDER_SAMPLE_VOL);
	
	m_bEditsLocked=false;
	UpdateView(HINT_MODGENERAL|HINT_MODTYPE|HINT_MODSEQUENCE, NULL);
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
	m_modDoc.SetNotifications(Notification::Default);
	m_modDoc.SetFollowWnd(m_hWnd);
	PostViewMessage(VIEWMSG_SETACTIVE, NULL);
	SetFocus();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlGeneral::OnDeactivatePage()
//-----------------------------------
{
	m_modDoc.SetFollowWnd(NULL);
	m_VuMeterLeft.SetVuMeter(0, true);
	m_VuMeterRight.SetVuMeter(0, true);
}


void CCtrlGeneral::UpdateView(DWORD dwHint, CObject *pHint)
//---------------------------------------------------------
{
	CHAR s[256];
	if (pHint == this) return;
	if (dwHint & HINT_MODSEQUENCE)
	{
		// Set max valid restart position
		m_SpinRestartPos.SetRange32(0, m_sndFile.Order.GetLengthTailTrimmed() - 1);
	}
	if (dwHint & HINT_MODGENERAL)
	{
		if (!m_bEditsLocked)
		{
			m_EditTitle.SetWindowText(m_sndFile.GetTitle().c_str());
			wsprintf(s, "%d", m_sndFile.m_nDefaultTempo);
			m_EditTempo.SetWindowText(s);
			wsprintf(s, "%d", m_sndFile.m_nDefaultSpeed);
			m_EditSpeed.SetWindowText(s);
			wsprintf(s, "%d", m_sndFile.m_nDefaultGlobalVolume / GetGlobalVolumeFactor());
			m_EditGlobalVol.SetWindowText(s);
			wsprintf(s, "%d", m_sndFile.m_nRestartPos);
			m_EditRestartPos.SetWindowText(s);
			wsprintf(s, "%d", m_sndFile.m_nVSTiVolume);
			m_EditVSTiVol.SetWindowText(s);
			wsprintf(s, "%d", m_sndFile.m_nSamplePreAmp);
			m_EditSamplePA.SetWindowText(s);
			wsprintf(s, "%d", m_sndFile.m_nRestartPos);
			m_EditRestartPos.SetWindowText(s);
		}

		m_SliderGlobalVol.SetPos(MAX_SLIDER_GLOBAL_VOL-m_sndFile.m_nDefaultGlobalVolume);
		m_SliderVSTiVol.SetPos(MAX_SLIDER_VSTI_VOL-m_sndFile.m_nVSTiVolume);
		m_SliderSamplePreAmp.SetPos(MAX_SLIDER_SAMPLE_VOL-m_sndFile.m_nSamplePreAmp);
		m_SliderTempo.SetPos(m_sndFile.GetModSpecifications().tempoMax - m_sndFile.m_nDefaultTempo);
	}

	if (dwHint & HINT_MODTYPE)
	{
		CModSpecifications specs = m_sndFile.GetModSpecifications();
		m_SpinTempo.SetRange(specs.tempoMin, specs.tempoMax);
		m_SliderTempo.SetRange(0, specs.tempoMax - specs.tempoMin);

		const BOOL bIsNotMOD = (m_sndFile.GetType() != MOD_TYPE_MOD);
		const BOOL bIsNotMOD_S3M = ((bIsNotMOD) && (m_sndFile.GetType() != MOD_TYPE_S3M));
		const BOOL bIsNotMOD_XM = ((bIsNotMOD) && (m_sndFile.GetType() != MOD_TYPE_XM));
		m_EditTempo.EnableWindow(bIsNotMOD);
		m_SpinTempo.EnableWindow(bIsNotMOD);
		m_SliderTempo.EnableWindow(bIsNotMOD);
		m_EditSpeed.EnableWindow(bIsNotMOD);
		m_SpinSpeed.EnableWindow(bIsNotMOD);
		m_SliderGlobalVol.EnableWindow(bIsNotMOD_XM);
		m_EditGlobalVol.EnableWindow(bIsNotMOD_XM);
		m_SpinGlobalVol.EnableWindow(bIsNotMOD_XM);
		m_EditSamplePA.EnableWindow(bIsNotMOD);
		m_SpinSamplePA.EnableWindow(bIsNotMOD);
		//m_SliderSamplePreAmp.EnableWindow(bIsNotMOD);
		m_SliderVSTiVol.EnableWindow(bIsNotMOD_S3M);
		m_EditVSTiVol.EnableWindow(bIsNotMOD_S3M);
		m_SpinVSTiVol.EnableWindow(bIsNotMOD_S3M);
		m_EditRestartPos.EnableWindow((specs.hasRestartPos || m_sndFile.m_nRestartPos != 0));
		m_SpinRestartPos.EnableWindow(m_EditRestartPos.IsWindowEnabled());
		
		//Note: Sample volume slider is not disabled for MOD
		//on purpose(can be used to control play volume)

		// MOD Type
		LPCSTR pszModType = "MOD (ProTracker)";
		switch(m_sndFile.GetType())
		{
		case MOD_TYPE_S3M:	pszModType = "S3M (ScreamTracker)"; break;
		case MOD_TYPE_XM:	pszModType = "XM (FastTracker 2)"; break;
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//		case MOD_TYPE_IT:	pszModType = "IT (Impulse Tracker)"; break;
		case MOD_TYPE_IT:	pszModType = m_sndFile.m_SongFlags[SONG_ITPROJECT] ? "ITP (IT Project)" : "IT (Impulse Tracker)"; break;
		case MOD_TYPE_MPT:	pszModType = "MPTM (OpenMPT)"; break;

// -! NEW_FEATURE#0023
		}
		wsprintf(s, "%s, %d channel%s", pszModType, m_sndFile.GetNumChannels(), (m_sndFile.GetNumChannels() != 1) ? "s" : "");
		m_EditModType.SetWindowText(s);
	}
	CheckDlgButton(IDC_CHECK_LOOPSONG,	(TrackerSettings::Instance().gbLoopSong) ? TRUE : FALSE);
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

	if (m_bInitialized)
	{
		CSliderCtrl* pSlider = (CSliderCtrl*) pscroll;

		if (pSlider==&m_SliderTempo)
		{
			int min, max;
			m_SpinTempo.GetRange(min, max);
			const UINT tempo = max - m_SliderTempo.GetPos();
			if ((tempo >= m_sndFile.GetModSpecifications().tempoMin) && (tempo <= m_sndFile.GetModSpecifications().tempoMax) && (tempo != m_sndFile.m_nDefaultTempo))
			{
				m_sndFile.m_nDefaultTempo = tempo;
				m_sndFile.m_nMusicTempo = tempo;
				m_modDoc.SetModified();

				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		else if (pSlider==&m_SliderGlobalVol)
		{
			const UINT gv = MAX_SLIDER_GLOBAL_VOL - m_SliderGlobalVol.GetPos();
			if ((gv >= 0) && (gv <= MAX_SLIDER_GLOBAL_VOL) && (gv != m_sndFile.m_nDefaultGlobalVolume))
			{
				m_sndFile.m_nGlobalVolume = gv;
				m_sndFile.m_nDefaultGlobalVolume = gv;
				m_modDoc.SetModified();

				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		else if (pSlider==&m_SliderSamplePreAmp)
		{
			const UINT spa = MAX_SLIDER_SAMPLE_VOL - m_SliderSamplePreAmp.GetPos();
			if ((spa >= 0) && (spa <= MAX_SLIDER_SAMPLE_VOL) && (spa != m_sndFile.m_nSamplePreAmp))
			{
				m_sndFile.m_nSamplePreAmp = spa;
				if(m_sndFile.GetType() != MOD_TYPE_MOD)
					m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		else if (pSlider==&m_SliderVSTiVol)
		{
			const UINT vv = MAX_SLIDER_VSTI_VOL - m_SliderVSTiVol.GetPos();
			if ((vv >= 0) && (vv <= MAX_SLIDER_VSTI_VOL) && (vv != m_sndFile.m_nVSTiVolume))
			{
				m_sndFile.m_nVSTiVolume = vv;
				m_sndFile.RecalculateGainForAllPlugs();
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
			}
		}

		UpdateView(HINT_MODGENERAL, NULL);
	}
}


void CCtrlGeneral::OnTitleChanged()
//---------------------------------
{
	if (!m_EditTitle.m_hWnd || !m_EditTitle.GetModify()) return;

	CString title;
	m_EditTitle.GetWindowText(title);
	if(m_sndFile.SetTitle(title.GetString()))
	{
		m_EditTitle.SetModify(FALSE);
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
	}
}


void CCtrlGeneral::OnTempoChanged()
//---------------------------------
{
	CHAR s[16];
	if (m_bInitialized)
	{
		m_EditTempo.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = atoi(s);
			n = CLAMP(n, m_sndFile.GetModSpecifications().tempoMin, m_sndFile.GetModSpecifications().tempoMax);
			if (n != m_sndFile.m_nDefaultTempo)
			{
				m_bEditsLocked=true;
				m_EditTempo.SetModify(FALSE);
				m_sndFile.m_nDefaultTempo = n;
				m_sndFile.m_nMusicTempo = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
				UpdateView(HINT_MODGENERAL, NULL); 
				m_bEditsLocked=false;
			}
		}
	}
}


void CCtrlGeneral::OnSpeedChanged()
//---------------------------------
{
	CHAR s[16];
	if(m_bInitialized)
	{
		m_EditSpeed.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = atoi(s);
			n = CLAMP(n, m_sndFile.GetModSpecifications().speedMin, m_sndFile.GetModSpecifications().speedMax);
			if (n != m_sndFile.m_nDefaultSpeed)
			{
				m_bEditsLocked=true;
				m_EditSpeed.SetModify(FALSE);
				m_sndFile.m_nDefaultSpeed = n;
				m_sndFile.m_nMusicSpeed = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
				m_bEditsLocked=false;
			}
		}
	}
}


void CCtrlGeneral::OnVSTiVolChanged()
//-----------------------------------
{
	CHAR s[16];
	if (m_bInitialized)
	{
		m_EditVSTiVol.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = ConvertStrTo<UINT>(s);
			Limit(n, 0u, 2000u);
			if (n != m_sndFile.m_nVSTiVolume)
			{
				m_bEditsLocked=true;
				m_sndFile.m_nVSTiVolume = n;
				m_sndFile.RecalculateGainForAllPlugs();
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
				UpdateView(HINT_MODGENERAL, NULL); 
				m_bEditsLocked=false;
			}
		}
	}
}

void CCtrlGeneral::OnSamplePAChanged()
//------------------------------------
{
	CHAR s[16];
	if(m_bInitialized)
	{
		m_EditSamplePA.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = ConvertStrTo<UINT>(s);
			Limit(n, 0u, 2000u);
			if (n != m_sndFile.m_nSamplePreAmp)
			{
				m_bEditsLocked=true;
				m_sndFile.m_nSamplePreAmp = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
				UpdateView(HINT_MODGENERAL, NULL); 
				m_bEditsLocked=false;
			}
		}
	}
}

void CCtrlGeneral::OnGlobalVolChanged()
//-------------------------------------
{
	CHAR s[16];
	if(m_bInitialized)
	{
		m_EditGlobalVol.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			UINT n = atoi(s) * GetGlobalVolumeFactor();
			Limit(n, 0u, 256u);
			if (n != m_sndFile.m_nDefaultGlobalVolume)
			{ 
				m_bEditsLocked = true;
				m_EditGlobalVol.SetModify(FALSE);
				m_sndFile.m_nDefaultGlobalVolume = n;
				m_sndFile.m_nGlobalVolume = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
				UpdateView(HINT_MODGENERAL, NULL);
				m_bEditsLocked = false;
			}
		}
	}
}


void CCtrlGeneral::OnRestartPosChanged()
//--------------------------------------
{
	CHAR s[32];
	if(m_bInitialized)
	{
		m_EditRestartPos.GetWindowText(s, sizeof(s));
		if (s[0])
		{
			ORDERINDEX n = (ORDERINDEX)atoi(s);
			Limit(n, (ORDERINDEX)0, m_sndFile.Order.size());
			for (ORDERINDEX i = 0; i <= n; i++)
				if (m_sndFile.Order[i] == m_sndFile.Order.GetInvalidPatIndex()) return;

			if (n != m_sndFile.m_nRestartPos)
			{
				m_EditRestartPos.SetModify(FALSE);
				m_sndFile.m_nRestartPos = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, HINT_MODGENERAL, this);
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
	m_modDoc.SongProperties();
}


void CCtrlGeneral::OnLoopSongChanged()
//------------------------------------
{
	TrackerSettings::Instance().gbLoopSong = IsDlgButtonChecked(IDC_CHECK_LOOPSONG);
	m_sndFile.SetRepeatCount((TrackerSettings::Instance().gbLoopSong) ? -1 : 0);
}


LRESULT CCtrlGeneral::OnUpdatePosition(WPARAM, LPARAM lParam)
//-----------------------------------------------------------
{
	Notification *pnotify = (Notification *)lParam;
	if (pnotify)
	{
		m_VuMeterLeft.SetVuMeter(pnotify->masterVU[0] & (~Notification::ClipVU), pnotify->type[Notification::Stop]);
		m_VuMeterRight.SetVuMeter(pnotify->masterVU[1] & (~Notification::ClipVU), pnotify->type[Notification::Stop]);
	}
	return 0;
}


BOOL CCtrlGeneral::GetToolTipText(UINT uId, LPSTR pszText)
//--------------------------------------------------------
{
	const char moreRecentMixModeNote[] = "Use a more recent mixmode to see dB offsets.";
	if ((pszText) && (uId))
	{
		const bool displayDBValues = m_sndFile.GetPlayConfig().getDisplayDBValues();

		switch(uId)
		{
			case IDC_SLIDER_SAMPLEPREAMP:
				(displayDBValues) ? setAsDecibels(pszText, m_sndFile.m_nSamplePreAmp, m_sndFile.GetPlayConfig().getNormalSamplePreAmp()) : wsprintf(pszText, moreRecentMixModeNote);
				return TRUE;
				break;
			case IDC_SLIDER_VSTIVOL:
				(displayDBValues) ? setAsDecibels(pszText, m_sndFile.m_nVSTiVolume, m_sndFile.GetPlayConfig().getNormalVSTiVol()) : wsprintf(pszText, moreRecentMixModeNote);
				return TRUE;
				break;
			case IDC_SLIDER_GLOBALVOL:
				(displayDBValues) ? setAsDecibels(pszText, m_sndFile.m_nGlobalVolume, m_sndFile.GetPlayConfig().getNormalGlobalVol()) : wsprintf(pszText, moreRecentMixModeNote);
				return TRUE;
				break;
		}
	}
	return FALSE;
	
}

void CCtrlGeneral::setAsDecibels(LPSTR stringToSet, double value, double valueAtZeroDB)
//-------------------------------------------------------------------------------------
{
	if (value == 0)
	{
		wsprintf(stringToSet, "-inf");
		return;
	}
	
	double changeFactor = value / valueAtZeroDB;
	double dB = 10 * log(changeFactor);

	char sign = (dB>=0) ? '+' : ' ';
	sprintf(stringToSet, "%c%.2f dB", sign, dB);
	return;

}


void CCtrlGeneral::OnEnSetfocusEditSongtitle()
//--------------------------------------------
{
	m_EditTitle.SetLimitText(m_sndFile.GetModSpecifications().modNameLengthMax);
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
	GetClientRect(&rect);
	dc.FillSolidRect(rect.left, rect.top, rect.Width(), rect.Height(), RGB(0,0,0));
	m_nDisplayedVu = -1;
	DrawVuMeter(dc, true);
}


VOID CVuMeter::SetVuMeter(LONG lVuMeter, bool force)
//--------------------------------------------------
{
	lVuMeter >>= 8;
	if (lVuMeter != m_nVuMeter)
	{
		DWORD curTime = timeGetTime();
		if(curTime - lastVuUpdateTime >= TrackerSettings::Instance().VuMeterUpdateInterval || force)
		{
			m_nVuMeter = lVuMeter;
			CClientDC dc(this);
			DrawVuMeter(dc);
			lastVuUpdateTime = curTime;
		}
	}
}


VOID CVuMeter::DrawVuMeter(CDC &dc, bool /*redraw*/)
//--------------------------------------------------
{
	LONG vu;
	LONG lastvu;
	CRect rect;

	GetClientRect(&rect);
	vu = (m_nVuMeter * (rect.bottom-rect.top)) >> 8;
	lastvu = (m_nDisplayedVu * (rect.bottom-rect.top)) >> 8;
	int cy = rect.bottom - rect.top;
	if (cy < 1) cy = 1;
	for (int ry=rect.bottom-1; ry>rect.top; ry-=2)
	{
		int y0 = rect.bottom - ry;
		int n = Clamp((y0 * NUM_VUMETER_PENS) / cy, 0, NUM_VUMETER_PENS - 1);
		if (vu < y0)
			n += NUM_VUMETER_PENS;
		dc.FillSolidRect(rect.left, ry, rect.Width(), 1, CMainFrame::gcolrefVuMeter[n]);
	}
	m_nDisplayedVu = m_nVuMeter;
}

