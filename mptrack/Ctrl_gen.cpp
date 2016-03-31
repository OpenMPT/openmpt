/*
 * Ctrl_gen.cpp
 * ------------
 * Purpose: General tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "Moddoc.h"
#include "Globals.h"
#include "dlg_misc.h"
#include "Ctrl_gen.h"
#include "View_gen.h"
#include "../common/misc_util.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CCtrlGeneral, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlGeneral)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON1,					OnTapTempo)
	ON_COMMAND(IDC_BUTTON_MODTYPE,			OnSongProperties)
	ON_COMMAND(IDC_CHECK_LOOPSONG,			OnLoopSongChanged)
	ON_EN_CHANGE(IDC_EDIT_SONGTITLE,		OnTitleChanged)
	ON_EN_CHANGE(IDC_EDIT_ARTIST,			OnArtistChanged)
	ON_EN_CHANGE(IDC_EDIT_TEMPO,			OnTempoChanged)
	ON_EN_CHANGE(IDC_EDIT_SPEED,			OnSpeedChanged)
	ON_EN_CHANGE(IDC_EDIT_GLOBALVOL,		OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT_RESTARTPOS,		OnRestartPosChanged)
	ON_EN_CHANGE(IDC_EDIT_VSTIVOL,			OnVSTiVolChanged)
	ON_EN_CHANGE(IDC_EDIT_SAMPLEPA,			OnSamplePAChanged)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		OnUpdatePosition)
	ON_EN_SETFOCUS(IDC_EDIT_SONGTITLE,		OnEnSetfocusEditSongtitle)
	ON_CBN_SELCHANGE(IDC_COMBO1,			OnResamplingChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCtrlGeneral::DoDataExchange(CDataExchange* pDX)
//---------------------------------------------------
{
	CModControlDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCtrlGeneral)
	DDX_Control(pDX, IDC_EDIT_SONGTITLE,	m_EditTitle);
	DDX_Control(pDX, IDC_EDIT_ARTIST,		m_EditArtist);
	//DDX_Control(pDX, IDC_EDIT_TEMPO,		m_EditTempo);
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

	DDX_Control(pDX, IDC_BUTTON_MODTYPE,	m_BtnModType);
	DDX_Control(pDX, IDC_VUMETER_LEFT,		m_VuMeterLeft);
	DDX_Control(pDX, IDC_VUMETER_RIGHT,		m_VuMeterRight);

	DDX_Control(pDX, IDC_COMBO1,			m_CbnResampling);
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

	m_SpinGlobalVol.SetRange(0, (short)(256 / GetGlobalVolumeFactor()));
	m_SpinSamplePA.SetRange(0, 2000);
	m_SpinVSTiVol.SetRange(0, 2000);
	m_SpinRestartPos.SetRange(0, 255);
	
	m_SliderGlobalVol.SetRange(0, MAX_SLIDER_GLOBAL_VOL);
	m_SliderVSTiVol.SetRange(0, MAX_SLIDER_VSTI_VOL);
	m_SliderSamplePreAmp.SetRange(0, MAX_SLIDER_SAMPLE_VOL);

	m_SpinTempo.SetRange(-10, 10);
	m_SliderTempo.SetLineSize(1);
	m_SliderTempo.SetPageSize(10);
	m_EditTempo.SubclassDlgItem(IDC_EDIT_TEMPO, this);
	m_EditTempo.AllowNegative(false);
	
	m_bEditsLocked = false;
	UpdateView(GeneralHint().ModType());
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


void CCtrlGeneral::OnTapTempo()
//-----------------------------
{
	static uint32 tapLength[16], lastTap = 0;
	// Shift back the previously recorded tap history
	for(size_t i = CountOf(tapLength) - 1; i >= 1; i--)
	{
		tapLength[i] = tapLength[i - 1];
	}
	const uint32 now = timeGetTime();
	tapLength[0] = now - lastTap;
	lastTap = now;

	// Now average over complete tap history
	uint32 numSamples = 0, delay = 0;
	for(uint32 i = 0; i < CountOf(tapLength); i++)
	{
		if(tapLength[i] < 2000)
		{
			numSamples++;
			delay += tapLength[i];
		} else
		{
			break;
		}
	}
	if(delay)
	{
		uint32 newTempo = 60000 * numSamples;
		if(m_sndFile.m_nTempoMode != tempoModeModern)
		{
			newTempo = Util::muldiv(newTempo, m_sndFile.m_nDefaultSpeed * m_sndFile.m_nDefaultRowsPerBeat, 24);
		}
		newTempo = Util::muldivr(newTempo, TEMPO::fractFact, delay);
		TEMPO t;
		t.SetRaw(newTempo);

		Limit(t, tempoMin, tempoMax);
		if(!m_sndFile.GetModSpecifications().hasFractionalTempo) t.Set(t.GetInt(), 0);
		m_EditTempo.SetTempoValue(t);
	}
}


void CCtrlGeneral::UpdateView(UpdateHint hint, CObject *pHint)
//------------------------------------------------------------
{
	if (pHint == this) return;
	FlagSet<HintType> hintType = hint.GetType();
	const bool updateAll = hintType[HINT_MODTYPE];

	const struct
	{
		const TCHAR *name;
		ResamplingMode mode;
	} interpolationTypes[] =
	{
		{ _T("No Interpolation"), SRCMODE_NEAREST },
		{ _T("Linear"), SRCMODE_LINEAR },
		{ _T("Cubic Spline"), SRCMODE_SPLINE },
		{ _T("Polyphase"), SRCMODE_POLYPHASE },
		{ _T("XMMS-ModPlug"), SRCMODE_FIRFILTER },
	};

	if (hintType == HINT_MPTOPTIONS || updateAll)
	{
		m_CbnResampling.ResetContent();
		m_CbnResampling.SetItemData(m_CbnResampling.AddString(_T("Default (") + CString(interpolationTypes[TrackerSettings::Instance().ResamplerMode].name) + _T(")")), SRCMODE_DEFAULT);
		for(int i = 0; i < CountOf(interpolationTypes); i++)
		{
			m_CbnResampling.SetItemData(m_CbnResampling.AddString(interpolationTypes[i].name), interpolationTypes[i].mode);
		}
		m_CbnResampling.Invalidate(FALSE);
	}

	if (updateAll)
	{
		CModSpecifications specs = m_sndFile.GetModSpecifications();
		
		// S3M HACK: ST3 will ignore speed 255, even though it can be used with Axx.
		if(m_sndFile.GetType() == MOD_TYPE_S3M)
			m_SpinSpeed.SetRange32(1, 254);
		else
			m_SpinSpeed.SetRange32(specs.speedMin, specs.speedMax);
		
		tempoMin = specs.tempoMin;
		tempoMax = specs.tempoMax;
		// IT Hack: There are legacy OpenMPT-made ITs out there which use a higher default speed than 255.
		// Changing the upper tempo limit in the mod specs would break them, so do it here instead.
		if(m_sndFile.GetType() == MOD_TYPE_IT && m_sndFile.m_nDefaultTempo <= TEMPO(255, 0))
			tempoMax.Set(255);
		m_SliderTempo.SetRange(0, tempoMax.GetInt() - tempoMin.GetInt());
		m_EditTempo.AllowFractions(specs.hasFractionalTempo);

		const BOOL bIsNotMOD = (m_sndFile.GetType() != MOD_TYPE_MOD);
		const BOOL bIsNotMOD_S3M = ((bIsNotMOD) && (m_sndFile.GetType() != MOD_TYPE_S3M));
		const BOOL bIsNotMOD_XM = ((bIsNotMOD) && (m_sndFile.GetType() != MOD_TYPE_XM));
		m_EditArtist.EnableWindow(specs.hasArtistName);
		m_EditTempo.EnableWindow(bIsNotMOD);
		m_SpinTempo.EnableWindow(bIsNotMOD);
		GetDlgItem(IDC_BUTTON1)->EnableWindow(bIsNotMOD);
		m_SliderTempo.EnableWindow(bIsNotMOD);
		m_EditSpeed.EnableWindow(bIsNotMOD);
		m_SpinSpeed.EnableWindow(bIsNotMOD);
		const BOOL globalVol = bIsNotMOD_XM || m_sndFile.m_nDefaultGlobalVolume != MAX_GLOBAL_VOLUME;
		m_SliderGlobalVol.EnableWindow(globalVol);
		m_EditGlobalVol.EnableWindow(globalVol);
		m_SpinGlobalVol.EnableWindow(globalVol);
		m_EditSamplePA.EnableWindow(bIsNotMOD);
		m_SpinSamplePA.EnableWindow(bIsNotMOD);
		//m_SliderSamplePreAmp.EnableWindow(bIsNotMOD);
		m_SliderVSTiVol.EnableWindow(bIsNotMOD_S3M);
		m_EditVSTiVol.EnableWindow(bIsNotMOD_S3M);
		m_SpinVSTiVol.EnableWindow(bIsNotMOD_S3M);
		m_EditRestartPos.EnableWindow((specs.hasRestartPos || m_sndFile.Order.GetRestartPos() != 0));
		m_SpinRestartPos.EnableWindow(m_EditRestartPos.IsWindowEnabled());
		
		//Note: Sample volume slider is not disabled for MOD
		//on purpose(can be used to control play volume)

		// MOD Type
		const TCHAR *modType = _T("");
		switch(m_sndFile.GetType())
		{
		case MOD_TYPE_MOD:	modType = _T("MOD (ProTracker)"); break;
		case MOD_TYPE_S3M:	modType = _T("S3M (ScreamTracker)"); break;
		case MOD_TYPE_XM:	modType = _T("XM (FastTracker 2)"); break;
		case MOD_TYPE_IT:	modType = _T("IT (Impulse Tracker)"); break;
		case MOD_TYPE_MPT:	modType = _T("MPTM (OpenMPT)"); break;
		default:			modType = CSoundFile::ModTypeToString(m_sndFile.GetType()); break;
		}
		TCHAR s[256];
		wsprintf(s, _T("%s, %u channel%s"), modType, m_sndFile.GetNumChannels(), (m_sndFile.GetNumChannels() != 1) ? _T("s") : _T(""));
		m_BtnModType.SetWindowText(s);
	}

	if (updateAll || (hint.GetCategory() == HINTCAT_SEQUENCE && hintType[HINT_MODSEQUENCE | HINT_RESTARTPOS]))
	{
		// Set max valid restart position
		m_SpinRestartPos.SetRange32(0, m_sndFile.Order.GetLengthTailTrimmed() - 1);
		SetDlgItemInt(IDC_EDIT_RESTARTPOS, m_sndFile.Order.GetRestartPos(), FALSE);
	}
	if (updateAll || (hint.GetCategory() == HINTCAT_GENERAL && hintType[HINT_MODGENERAL]))
	{
		if (!m_bEditsLocked)
		{
			m_EditTitle.SetWindowText(m_sndFile.GetTitle().c_str());
			::SetWindowTextW(m_EditArtist.m_hWnd, mpt::ToWide(m_sndFile.m_songArtist).c_str());
			m_EditTempo.SetTempoValue(m_sndFile.m_nDefaultTempo);
			SetDlgItemInt(IDC_EDIT_SPEED, m_sndFile.m_nDefaultSpeed, FALSE);
			SetDlgItemInt(IDC_EDIT_GLOBALVOL, m_sndFile.m_nDefaultGlobalVolume / GetGlobalVolumeFactor(), FALSE);
			SetDlgItemInt(IDC_EDIT_VSTIVOL, m_sndFile.m_nVSTiVolume, FALSE);
			SetDlgItemInt(IDC_EDIT_SAMPLEPA, m_sndFile.m_nSamplePreAmp, FALSE);
		}

		m_SliderGlobalVol.SetPos(MAX_SLIDER_GLOBAL_VOL - m_sndFile.m_nDefaultGlobalVolume);
		m_SliderVSTiVol.SetPos(MAX_SLIDER_VSTI_VOL - m_sndFile.m_nVSTiVolume);
		m_SliderSamplePreAmp.SetPos(MAX_SLIDER_SAMPLE_VOL - m_sndFile.m_nSamplePreAmp);
		m_SliderTempo.SetPos((tempoMax - m_sndFile.m_nDefaultTempo).GetInt());
	}

	if(updateAll || hintType == HINT_MPTOPTIONS || (hint.GetCategory() == HINTCAT_GENERAL && hintType[HINT_MODGENERAL]))
	{
		int srcMode = 0;
		for(int i = 0; i < CountOf(interpolationTypes); i++)
		{
			if(m_sndFile.m_nResampling == interpolationTypes[i].mode) srcMode = i + 1;
		}
		m_CbnResampling.SetCurSel(srcMode);
	}

	CheckDlgButton(IDC_CHECK_LOOPSONG, (TrackerSettings::Instance().gbLoopSong) ? TRUE : FALSE);
	if (hintType[HINT_MPTOPTIONS])
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

		if (pSlider == &m_SliderTempo)
		{
			const TEMPO tempo = tempoMax - TEMPO(m_SliderTempo.GetPos(), 0);
			if ((tempo >= m_sndFile.GetModSpecifications().tempoMin) && (tempo <= m_sndFile.GetModSpecifications().tempoMax) && (tempo != m_sndFile.m_nDefaultTempo))
			{
				m_sndFile.m_nDefaultTempo = m_sndFile.m_PlayState.m_nMusicTempo = tempo;
				m_modDoc.SetModified();

				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
			}
		}

		else if (pSlider == &m_SliderGlobalVol)
		{
			const UINT gv = MAX_SLIDER_GLOBAL_VOL - m_SliderGlobalVol.GetPos();
			if ((gv >= 0) && (gv <= MAX_SLIDER_GLOBAL_VOL) && (gv != m_sndFile.m_nDefaultGlobalVolume))
			{
				m_sndFile.m_PlayState.m_nGlobalVolume = gv;
				m_sndFile.m_nDefaultGlobalVolume = gv;
				m_modDoc.SetModified();

				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
			}
		}

		else if (pSlider == &m_SliderSamplePreAmp)
		{
			const UINT spa = MAX_SLIDER_SAMPLE_VOL - m_SliderSamplePreAmp.GetPos();
			if ((spa >= 0) && (spa <= MAX_SLIDER_SAMPLE_VOL) && (spa != m_sndFile.m_nSamplePreAmp))
			{
				m_sndFile.m_nSamplePreAmp = spa;
				if(m_sndFile.GetType() != MOD_TYPE_MOD)
					m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
			}
		}

		else if (pSlider == &m_SliderVSTiVol)
		{
			const UINT vv = MAX_SLIDER_VSTI_VOL - m_SliderVSTiVol.GetPos();
			if ((vv >= 0) && (vv <= MAX_SLIDER_VSTI_VOL) && (vv != m_sndFile.m_nVSTiVolume))
			{
				m_sndFile.m_nVSTiVolume = vv;
				m_sndFile.RecalculateGainForAllPlugs();
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
			}
		}

		else if(pSlider == (CSliderCtrl*)&m_SpinTempo)
		{
			int pos = m_SpinTempo.GetPos32();
			if(pos != 0)
			{
				TEMPO newTempo;
				if(m_sndFile.GetModSpecifications().hasFractionalTempo)
				{
					pos *= TEMPO::fractFact;
					if(CMainFrame::GetMainFrame()->GetInputHandler()->CtrlPressed())
						pos /= 100;
					else
						pos /= 10;
					newTempo.SetRaw(pos);
				} else
				{
					newTempo = TEMPO(pos, 0);
				}
				newTempo += m_sndFile.m_nDefaultTempo;
				Limit(newTempo, tempoMin, tempoMax);
				m_sndFile.m_nDefaultTempo = m_sndFile.m_PlayState.m_nMusicTempo = newTempo;
				m_modDoc.SetModified();
				LockControls();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UnlockControls();
			}
			m_SpinTempo.SetPos(0);
		}

		UpdateView(GeneralHint().General());
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
		m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
	}
}


void CCtrlGeneral::OnArtistChanged()
//----------------------------------
{
	if (!m_EditArtist.m_hWnd || !m_EditArtist.GetModify()) return;

	mpt::ustring artist = mpt::ToUnicode(GetWindowTextW(m_EditArtist));
	if(artist != m_sndFile.m_songArtist)
	{
		m_EditArtist.SetModify(FALSE);
		m_sndFile.m_songArtist = artist;
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(NULL, GeneralHint().General(), this);
	}
}


void CCtrlGeneral::OnTempoChanged()
//---------------------------------
{
	if (m_bInitialized && m_EditTempo.GetWindowTextLength() > 0)
	{
		TEMPO tempo = m_EditTempo.GetTempoValue();
		Limit(tempo, tempoMin, tempoMax);
		if(!m_sndFile.GetModSpecifications().hasFractionalTempo) tempo.Set(tempo.GetInt());
		if (tempo != m_sndFile.m_nDefaultTempo)
		{
			m_bEditsLocked=true;
			m_EditTempo.SetModify(FALSE);
			m_sndFile.m_nDefaultTempo = tempo;
			m_sndFile.m_PlayState.m_nMusicTempo = tempo;
			m_modDoc.SetModified();
			m_modDoc.UpdateAllViews(nullptr, GeneralHint().General());
			m_bEditsLocked=false;
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
			n = Clamp(n, m_sndFile.GetModSpecifications().speedMin, m_sndFile.GetModSpecifications().speedMax);
			if (n != m_sndFile.m_nDefaultSpeed)
			{
				m_bEditsLocked=true;
				m_EditSpeed.SetModify(FALSE);
				m_sndFile.m_nDefaultSpeed = n;
				m_sndFile.m_PlayState.m_nMusicSpeed = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				// Update envelope grid view
				m_modDoc.UpdateAllViews(nullptr, InstrumentHint().Envelope(), this);
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
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UpdateView(GeneralHint().General());
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
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UpdateView(GeneralHint().General());
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
				m_sndFile.m_PlayState.m_nGlobalVolume = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UpdateView(GeneralHint().General());
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

			if (n != m_sndFile.Order.GetRestartPos())
			{
				m_EditRestartPos.SetModify(FALSE);
				m_sndFile.Order.SetRestartPos(n);
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
			}
		}
	}
}


void CCtrlGeneral::OnSongProperties()
//-----------------------------------
{
	m_modDoc.OnSongProperties();
}


void CCtrlGeneral::OnLoopSongChanged()
//------------------------------------
{
	TrackerSettings::Instance().gbLoopSong = (IsDlgButtonChecked(IDC_CHECK_LOOPSONG) != 0);
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
	const TCHAR moreRecentMixModeNote[] = _T("Use a more recent mixmode to see dB offsets.");
	if ((pszText) && (uId))
	{
		const bool displayDBValues = m_sndFile.GetPlayConfig().getDisplayDBValues();

		switch(uId)
		{
		case IDC_BUTTON_MODTYPE:
			_tcscpy(pszText, _T("Song Properties"));
			return TRUE;
		case IDC_BUTTON1:
			_tcscpy(pszText, _T("Click button multiple times to tap in the desired tempo."));
			return TRUE;
		case IDC_SLIDER_SAMPLEPREAMP:
			_tcscpy(pszText, displayDBValues ? CModDoc::LinearToDecibels(m_sndFile.m_nSamplePreAmp, m_sndFile.GetPlayConfig().getNormalSamplePreAmp()) : moreRecentMixModeNote);
			return TRUE;
		case IDC_SLIDER_VSTIVOL:
			_tcscpy(pszText, displayDBValues ? CModDoc::LinearToDecibels(m_sndFile.m_nVSTiVolume, m_sndFile.GetPlayConfig().getNormalVSTiVol()) : moreRecentMixModeNote);
			return TRUE;
		case IDC_SLIDER_GLOBALVOL:
			_tcscpy(pszText, displayDBValues ? CModDoc::LinearToDecibels(m_sndFile.m_PlayState.m_nGlobalVolume, m_sndFile.GetPlayConfig().getNormalGlobalVol()) : moreRecentMixModeNote);
			return TRUE;
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


void CCtrlGeneral::OnResamplingChanged()
//--------------------------------------
{
	int sel = m_CbnResampling.GetCurSel();
	if(sel >= 0)
	{
		m_sndFile.m_nResampling = static_cast<ResamplingMode>(m_CbnResampling.GetItemData(sel));
		if(m_sndFile.GetModSpecifications().hasDefaultResampling)
		{
			m_modDoc.SetModified();
			m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
		}
	}
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


OPENMPT_NAMESPACE_END
