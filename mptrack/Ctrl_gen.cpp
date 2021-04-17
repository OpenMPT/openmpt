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
#include "../common/mptTime.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CCtrlGeneral, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlGeneral)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON1,					&CCtrlGeneral::OnTapTempo)
	ON_COMMAND(IDC_BUTTON_MODTYPE,			&CCtrlGeneral::OnSongProperties)
	ON_COMMAND(IDC_CHECK_LOOPSONG,			&CCtrlGeneral::OnLoopSongChanged)
	ON_EN_CHANGE(IDC_EDIT_SONGTITLE,		&CCtrlGeneral::OnTitleChanged)
	ON_EN_CHANGE(IDC_EDIT_ARTIST,			&CCtrlGeneral::OnArtistChanged)
	ON_EN_CHANGE(IDC_EDIT_TEMPO,			&CCtrlGeneral::OnTempoChanged)
	ON_EN_CHANGE(IDC_EDIT_SPEED,			&CCtrlGeneral::OnSpeedChanged)
	ON_EN_CHANGE(IDC_EDIT_GLOBALVOL,		&CCtrlGeneral::OnGlobalVolChanged)
	ON_EN_CHANGE(IDC_EDIT_RESTARTPOS,		&CCtrlGeneral::OnRestartPosChanged)
	ON_EN_CHANGE(IDC_EDIT_VSTIVOL,			&CCtrlGeneral::OnVSTiVolChanged)
	ON_EN_CHANGE(IDC_EDIT_SAMPLEPA,			&CCtrlGeneral::OnSamplePAChanged)
	ON_MESSAGE(WM_MOD_UPDATEPOSITION,		&CCtrlGeneral::OnUpdatePosition)
	ON_EN_SETFOCUS(IDC_EDIT_SONGTITLE,		&CCtrlGeneral::OnEnSetfocusEditSongtitle)
	ON_EN_KILLFOCUS(IDC_EDIT_RESTARTPOS,	&CCtrlGeneral::OnRestartPosDone)
	ON_CBN_SELCHANGE(IDC_COMBO1,			&CCtrlGeneral::OnResamplingChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCtrlGeneral::DoDataExchange(CDataExchange* pDX)
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
{
}


BOOL CCtrlGeneral::OnInitDialog()
{
	const auto &specs = m_sndFile.GetModSpecifications();
	CModControlDlg::OnInitDialog();
	// Song Title
	m_EditTitle.SetLimitText(specs.modNameLengthMax);

	m_SpinGlobalVol.SetRange(0, (short)(256 / GetGlobalVolumeFactor()));
	m_SpinSamplePA.SetRange(0, 2000);
	m_SpinVSTiVol.SetRange(0, 2000);
	m_SpinRestartPos.SetRange32(0, ORDERINDEX_MAX);
	
	m_SliderGlobalVol.SetRange(0, MAX_SLIDER_GLOBAL_VOL);
	m_SliderVSTiVol.SetRange(0, MAX_SLIDER_VSTI_VOL);
	m_SliderSamplePreAmp.SetRange(0, MAX_SLIDER_SAMPLE_VOL);

	m_SpinTempo.SetRange(-10, 10);
	m_SliderTempo.SetLineSize(1);
	m_SliderTempo.SetPageSize(10);
	m_EditTempo.SubclassDlgItem(IDC_EDIT_TEMPO, this);
	m_EditTempo.AllowNegative(false);
	
	m_editsLocked = false;
	UpdateView(GeneralHint().ModType());
	OnActivatePage(0);
	m_bInitialized = TRUE;
	
	return FALSE;
}


CRuntimeClass *CCtrlGeneral::GetAssociatedViewClass()
{
	return RUNTIME_CLASS(CViewGlobals);
}


void CCtrlGeneral::RecalcLayout()
{
}


void CCtrlGeneral::OnActivatePage(LPARAM)
{
	m_modDoc.SetNotifications(Notification::Default);
	m_modDoc.SetFollowWnd(m_hWnd);
	PostViewMessage(VIEWMSG_SETACTIVE, NULL);
	SetFocus();

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlGeneral::OnDeactivatePage()
{
	m_modDoc.SetFollowWnd(NULL);
	m_VuMeterLeft.SetVuMeter(0, true);
	m_VuMeterRight.SetVuMeter(0, true);
	m_tapTimer = nullptr;  // Reset high-precision clock if required
}


TEMPO CCtrlGeneral::TempoSliderRange() const
{
	return (TEMPO_SPLIT_THRESHOLD - m_tempoMin) + TEMPO((m_tempoMax - TEMPO_SPLIT_THRESHOLD).GetInt() / TEMPO_SPLIT_PRECISION, 0);
}


TEMPO CCtrlGeneral::SliderToTempo(int value) const
{
	if(m_tempoMax < TEMPO_SPLIT_THRESHOLD)
	{
		return m_tempoMax - TEMPO(value, 0);
	} else
	{
		auto tempo = TempoSliderRange() - TEMPO(value, 0) + m_tempoMin;
		if(tempo >= TEMPO_SPLIT_THRESHOLD)
			tempo = TEMPO((tempo - TEMPO_SPLIT_THRESHOLD).GetInt() * TEMPO_SPLIT_PRECISION, 0) + TEMPO_SPLIT_THRESHOLD;
		return tempo;
	}
}


int CCtrlGeneral::TempoToSlider(TEMPO tempo) const
{
	if(m_tempoMax < TEMPO_SPLIT_THRESHOLD)
	{
		return (m_tempoMax - tempo).GetInt();
	} else
	{
		if(tempo >= TEMPO_SPLIT_THRESHOLD)
			tempo = TEMPO((tempo - TEMPO_SPLIT_THRESHOLD).GetInt() / TEMPO_SPLIT_PRECISION + TEMPO_SPLIT_THRESHOLD.GetInt(), 0);
		const auto range = TempoSliderRange();
		return (range - std::min(tempo, range)).GetInt();
	}
}


void CCtrlGeneral::OnTapTempo()
{
	using TapType = decltype(m_tapTimer->Now());
	static std::array<TapType, 32> tapTime;
	static TapType lastTap = 0;
	static uint32 numTaps = 0;

	if(m_tapTimer == nullptr)
		m_tapTimer = std::make_unique<Util::MultimediaClock>(1);

	const uint32 now = m_tapTimer->Now();
	if(now - lastTap >= 2000)
		numTaps = 0;
	lastTap = now;

	if(static_cast<size_t>(numTaps) >= tapTime.size())
	{
		// Shift back the previously recorded tap history
		// cppcheck false-positive
		// cppcheck-suppress mismatchingContainers
		std::copy(tapTime.begin() + 1, tapTime.end(), tapTime.begin());
		numTaps = static_cast<uint32>(tapTime.size() - 1);
	}
	
	tapTime[numTaps++] = now;

	if(numTaps <= 1)
		return;

	// Now apply least squares to tap history
	double sum = 0.0, weightedSum = 0.0;
	for(uint32 i = 0; i < numTaps; i++)
	{
		const double tapMs = tapTime[i] / 1000.0;
		sum += tapMs;
		weightedSum += i * tapMs;
	}

	const double lengthSum = numTaps * (numTaps - 1) / 2;
	const double lengthSumSum = lengthSum * (2 * numTaps - 1) / 3.0;
	const double secondsPerBeat = (numTaps * weightedSum - lengthSum * sum) / (lengthSumSum * numTaps - lengthSum * lengthSum);

	double newTempo = 60.0 / secondsPerBeat;
	if(m_sndFile.m_nTempoMode != TempoMode::Modern)
		newTempo *= (m_sndFile.m_nDefaultSpeed * m_sndFile.m_nDefaultRowsPerBeat) / 24.0;
	if(!m_sndFile.GetModSpecifications().hasFractionalTempo)
		newTempo = std::round(newTempo);
	TEMPO t(newTempo);
	Limit(t, m_tempoMin, m_tempoMax);
	m_EditTempo.SetTempoValue(t);
}


void CCtrlGeneral::UpdateView(UpdateHint hint, CObject *pHint)
{
	if (pHint == this) return;
	FlagSet<HintType> hintType = hint.GetType();
	const bool updateAll = hintType[HINT_MODTYPE];

	const auto resamplingModes = Resampling::AllModes();

	if (hintType == HINT_MPTOPTIONS || updateAll)
	{
		CString defaultResampler;
		if(m_sndFile.m_SongFlags[SONG_ISAMIGA] && TrackerSettings::Instance().ResamplerEmulateAmiga != Resampling::AmigaFilter::Off)
			defaultResampler = _T("Amiga Resampler");
		else
			defaultResampler = CTrackApp::GetResamplingModeName(TrackerSettings::Instance().ResamplerMode, 1, false);

		m_CbnResampling.ResetContent();
		m_CbnResampling.SetItemData(m_CbnResampling.AddString(_T("Default (") + defaultResampler + _T(")")), SRCMODE_DEFAULT);
		for(auto mode : resamplingModes)
		{
			m_CbnResampling.SetItemData(m_CbnResampling.AddString(CTrackApp::GetResamplingModeName(mode, 2, true)), mode);
		}
		m_CbnResampling.Invalidate(FALSE);
	}

	if(updateAll)
	{
		const auto &specs = m_sndFile.GetModSpecifications();

		// S3M HACK: ST3 will ignore speed 255, even though it can be used with Axx.
		if(m_sndFile.GetType() == MOD_TYPE_S3M)
			m_SpinSpeed.SetRange32(1, 254);
		else
			m_SpinSpeed.SetRange32(specs.speedMin, specs.speedMax);

		m_tempoMin = specs.GetTempoMin();
		m_tempoMax = specs.GetTempoMax();
		// IT Hack: There are legacy OpenMPT-made ITs out there which use a higher default speed than 255.
		// Changing the upper tempo limit in the mod specs would break them, so do it here instead.
		if(m_sndFile.GetType() == MOD_TYPE_IT && m_sndFile.m_nDefaultTempo <= TEMPO(255, 0))
			m_tempoMax.Set(255);
		// Lower resolution for BPM above 256
		if(m_tempoMax >= TEMPO_SPLIT_THRESHOLD)
			m_SliderTempo.SetRange(0, TempoSliderRange().GetInt());
		else
			m_SliderTempo.SetRange(0, m_tempoMax.GetInt() - m_tempoMin.GetInt());
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
		m_SliderVSTiVol.EnableWindow(bIsNotMOD);
		m_EditVSTiVol.EnableWindow(bIsNotMOD);
		m_SpinVSTiVol.EnableWindow(bIsNotMOD);
		m_EditRestartPos.EnableWindow((specs.hasRestartPos || m_sndFile.Order().GetRestartPos() != 0));
		m_SpinRestartPos.EnableWindow(m_EditRestartPos.IsWindowEnabled());

		//Note: Sample volume slider is not disabled for MOD
		//on purpose(can be used to control play volume)
	}

	if(updateAll || (hint.GetCategory() == HINTCAT_GLOBAL && hintType[HINT_MODCHANNELS]))
	{
		// MOD Type
		mpt::ustring modType;
		switch(m_sndFile.GetType())
		{
		case MOD_TYPE_MOD:	modType = U_("MOD (ProTracker)"); break;
		case MOD_TYPE_S3M:	modType = U_("S3M (ScreamTracker)"); break;
		case MOD_TYPE_XM:	modType = U_("XM (FastTracker 2)"); break;
		case MOD_TYPE_IT:	modType = U_("IT (Impulse Tracker)"); break;
		case MOD_TYPE_MPT:	modType = U_("MPTM (OpenMPT)"); break;
		default:			modType = MPT_UFORMAT("{} ({})")(mpt::ToUpperCase(m_sndFile.m_modFormat.type), m_sndFile.m_modFormat.formatName); break;
		}
		CString s;
		s.Format(_T("%s, %u channel%s"), mpt::ToCString(modType).GetString(), m_sndFile.GetNumChannels(), (m_sndFile.GetNumChannels() != 1) ? _T("s") : _T(""));
		m_BtnModType.SetWindowText(s);
	}

	if (updateAll || (hint.GetCategory() == HINTCAT_SEQUENCE && hintType[HINT_MODSEQUENCE | HINT_RESTARTPOS]))
	{
		// Set max valid restart position
		m_SpinRestartPos.SetRange32(0, std::max(m_sndFile.Order().GetRestartPos(), static_cast<ORDERINDEX>(m_sndFile.Order().GetLengthTailTrimmed() - 1)));
		SetDlgItemInt(IDC_EDIT_RESTARTPOS, m_sndFile.Order().GetRestartPos(), FALSE);
	}
	if (updateAll || (hint.GetCategory() == HINTCAT_GENERAL && hintType[HINT_MODGENERAL]))
	{
		if (!m_editsLocked)
		{
			m_EditTitle.SetWindowText(mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.GetTitle()));
			m_EditArtist.SetWindowText(mpt::ToCString(m_sndFile.m_songArtist));
			m_EditTempo.SetTempoValue(m_sndFile.m_nDefaultTempo);
			SetDlgItemInt(IDC_EDIT_SPEED, m_sndFile.m_nDefaultSpeed, FALSE);
			SetDlgItemInt(IDC_EDIT_GLOBALVOL, m_sndFile.m_nDefaultGlobalVolume / GetGlobalVolumeFactor(), FALSE);
			SetDlgItemInt(IDC_EDIT_VSTIVOL, m_sndFile.m_nVSTiVolume, FALSE);
			SetDlgItemInt(IDC_EDIT_SAMPLEPA, m_sndFile.m_nSamplePreAmp, FALSE);
		}

		m_SliderGlobalVol.SetPos(MAX_SLIDER_GLOBAL_VOL - m_sndFile.m_nDefaultGlobalVolume);
		m_SliderVSTiVol.SetPos(MAX_SLIDER_VSTI_VOL - m_sndFile.m_nVSTiVolume);
		m_SliderSamplePreAmp.SetPos(MAX_SLIDER_SAMPLE_VOL - m_sndFile.m_nSamplePreAmp);
		m_SliderTempo.SetPos(TempoToSlider(m_sndFile.m_nDefaultTempo));
	}

	if(updateAll || hintType == HINT_MPTOPTIONS || (hint.GetCategory() == HINTCAT_GENERAL && hintType[HINT_MODGENERAL]))
	{
		for(int i = 0; i < m_CbnResampling.GetCount(); ++i)
		{
			if(m_sndFile.m_nResampling == static_cast<ResamplingMode>(m_CbnResampling.GetItemData(i)))
			{
				m_CbnResampling.SetCurSel(i);
				break;
			}
		}
	}

	CheckDlgButton(IDC_CHECK_LOOPSONG, (TrackerSettings::Instance().gbLoopSong) ? TRUE : FALSE);
	if (hintType[HINT_MPTOPTIONS])
	{
		m_VuMeterLeft.InvalidateRect(NULL, FALSE);
		m_VuMeterRight.InvalidateRect(NULL, FALSE);
	}
}


void CCtrlGeneral::OnVScroll(UINT code, UINT pos, CScrollBar *pscroll)
{
	CDialog::OnVScroll(code, pos, pscroll);

	if (m_bInitialized)
	{
		CSliderCtrl* pSlider = (CSliderCtrl*) pscroll;

		if (pSlider == &m_SliderTempo)
		{
			const TEMPO tempo = SliderToTempo(m_SliderTempo.GetPos());
			if ((tempo >= m_sndFile.GetModSpecifications().GetTempoMin()) && (tempo <= m_sndFile.GetModSpecifications().GetTempoMax()) && (tempo != m_sndFile.m_nDefaultTempo))
			{
				m_sndFile.m_nDefaultTempo = m_sndFile.m_PlayState.m_nMusicTempo = tempo;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				m_EditTempo.SetTempoValue(tempo);
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
				SetDlgItemInt(IDC_EDIT_GLOBALVOL, m_sndFile.m_nDefaultGlobalVolume / GetGlobalVolumeFactor(), FALSE);
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
				SetDlgItemInt(IDC_EDIT_SAMPLEPA, m_sndFile.m_nSamplePreAmp, FALSE);
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
				SetDlgItemInt(IDC_EDIT_VSTIVOL, m_sndFile.m_nVSTiVolume, FALSE);
			}
		}

		else if(pSlider == (CSliderCtrl*)&m_SpinTempo)
		{
			int pos32 = m_SpinTempo.GetPos32();
			if(pos32 != 0)
			{
				TEMPO newTempo;
				if(m_sndFile.GetModSpecifications().hasFractionalTempo)
				{
					pos32 *= TEMPO::fractFact;
					if(CMainFrame::GetMainFrame()->GetInputHandler()->CtrlPressed())
						pos32 /= 100;
					else if(CMainFrame::GetMainFrame()->GetInputHandler()->ShiftPressed())
						pos32 /= 10;
					newTempo.SetRaw(pos32);
				} else
				{
					newTempo = TEMPO(pos32, 0);
				}
				newTempo += m_sndFile.m_nDefaultTempo;
				Limit(newTempo, m_tempoMin, m_tempoMax);
				m_sndFile.m_nDefaultTempo = m_sndFile.m_PlayState.m_nMusicTempo = newTempo;
				m_modDoc.SetModified();
				LockControls();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UnlockControls();
				m_SliderTempo.SetPos(TempoToSlider(newTempo));
				m_EditTempo.SetTempoValue(newTempo);
			}
			m_SpinTempo.SetPos(0);
		}
	}
}


void CCtrlGeneral::OnTitleChanged()
{
	if (!m_EditTitle.m_hWnd || !m_EditTitle.GetModify()) return;

	CString title;
	m_EditTitle.GetWindowText(title);
	if(m_sndFile.SetTitle(mpt::ToCharset(m_sndFile.GetCharsetInternal(), title)))
	{
		m_EditTitle.SetModify(FALSE);
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
	}
}


void CCtrlGeneral::OnArtistChanged()
{
	if (!m_EditArtist.m_hWnd || !m_EditArtist.GetModify()) return;

	mpt::ustring artist = GetWindowTextUnicode(m_EditArtist);
	if(artist != m_sndFile.m_songArtist)
	{
		m_EditArtist.SetModify(FALSE);
		m_sndFile.m_songArtist = artist;
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(NULL, GeneralHint().General(), this);
	}
}


void CCtrlGeneral::OnTempoChanged()
{
	if (m_bInitialized && m_EditTempo.GetWindowTextLength() > 0)
	{
		TEMPO tempo = m_EditTempo.GetTempoValue();
		Limit(tempo, m_tempoMin, m_tempoMax);
		if(!m_sndFile.GetModSpecifications().hasFractionalTempo) tempo.Set(tempo.GetInt());
		if (tempo != m_sndFile.m_nDefaultTempo)
		{
			m_editsLocked = true;
			m_EditTempo.SetModify(FALSE);
			m_sndFile.m_nDefaultTempo = tempo;
			m_sndFile.m_PlayState.m_nMusicTempo = tempo;
			m_modDoc.SetModified();
			m_modDoc.UpdateAllViews(nullptr, GeneralHint().General());
			m_editsLocked = false;
		}
	}
}


void CCtrlGeneral::OnSpeedChanged()
{
	TCHAR s[16];
	if(m_bInitialized)
	{
		m_EditSpeed.GetWindowText(s, mpt::saturate_cast<int>(std::size(s)));
		if (s[0])
		{
			UINT n = ConvertStrTo<UINT>(s);
			n = Clamp(n, m_sndFile.GetModSpecifications().speedMin, m_sndFile.GetModSpecifications().speedMax);
			if (n != m_sndFile.m_nDefaultSpeed)
			{
				m_editsLocked = true;
				m_EditSpeed.SetModify(FALSE);
				m_sndFile.m_nDefaultSpeed = n;
				m_sndFile.m_PlayState.m_nMusicSpeed = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				// Update envelope grid view
				m_modDoc.UpdateAllViews(nullptr, InstrumentHint().Envelope(), this);
				m_editsLocked = false;
			}
		}
	}
}


void CCtrlGeneral::OnVSTiVolChanged()
{
	TCHAR s[16];
	if (m_bInitialized)
	{
		m_EditVSTiVol.GetWindowText(s, mpt::saturate_cast<int>(std::size(s)));
		if (s[0])
		{
			UINT n = ConvertStrTo<UINT>(s);
			Limit(n, 0u, 2000u);
			if (n != m_sndFile.m_nVSTiVolume)
			{
				m_editsLocked = true;
				m_sndFile.m_nVSTiVolume = n;
				m_sndFile.RecalculateGainForAllPlugs();
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UpdateView(GeneralHint().General());
				m_editsLocked = false;
			}
		}
	}
}

void CCtrlGeneral::OnSamplePAChanged()
{
	TCHAR s[16];
	if(m_bInitialized)
	{
		m_EditSamplePA.GetWindowText(s, mpt::saturate_cast<int>(std::size(s)));
		if (s[0])
		{
			UINT n = ConvertStrTo<UINT>(s);
			Limit(n, 0u, 2000u);
			if (n != m_sndFile.m_nSamplePreAmp)
			{
				m_editsLocked = true;
				m_sndFile.m_nSamplePreAmp = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UpdateView(GeneralHint().General());
				m_editsLocked = false;
			}
		}
	}
}

void CCtrlGeneral::OnGlobalVolChanged()
{
	TCHAR s[16];
	if(m_bInitialized)
	{
		m_EditGlobalVol.GetWindowText(s, mpt::saturate_cast<int>(std::size(s)));
		if (s[0])
		{
			UINT n = ConvertStrTo<ORDERINDEX>(s) * GetGlobalVolumeFactor();
			Limit(n, 0u, 256u);
			if (n != m_sndFile.m_nDefaultGlobalVolume)
			{ 
				m_editsLocked = true;
				m_EditGlobalVol.SetModify(FALSE);
				m_sndFile.m_nDefaultGlobalVolume = n;
				m_sndFile.m_PlayState.m_nGlobalVolume = n;
				m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, GeneralHint().General(), this);
				UpdateView(GeneralHint().General());
				m_editsLocked = false;
			}
		}
	}
}


void CCtrlGeneral::OnRestartPosChanged()
{
	if(!m_bInitialized)
		return;
	TCHAR s[32];
	m_EditRestartPos.GetWindowText(s, mpt::saturate_cast<int>(std::size(s)));
	if(!s[0])
		return;

	ORDERINDEX n = ConvertStrTo<ORDERINDEX>(s);
	LimitMax(n, m_sndFile.Order().GetLastIndex());
	while(n > 0 && n < m_sndFile.Order().GetLastIndex() && !m_sndFile.Order().IsValidPat(n))
		n++;

	if(n == m_sndFile.Order().GetRestartPos())
		return;
	
	m_EditRestartPos.SetModify(FALSE);
	m_sndFile.Order().SetRestartPos(n);
	m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, SequenceHint(m_sndFile.Order.GetCurrentSequenceIndex()).RestartPos(), this);
}


void CCtrlGeneral::OnRestartPosDone()
{
	if(m_bInitialized)
		SetDlgItemInt(IDC_EDIT_RESTARTPOS, m_sndFile.Order().GetRestartPos());
}


void CCtrlGeneral::OnSongProperties()
{
	m_modDoc.OnSongProperties();
}


void CCtrlGeneral::OnLoopSongChanged()
{
	m_modDoc.SetLoopSong(IsDlgButtonChecked(IDC_CHECK_LOOPSONG) != BST_UNCHECKED);
}


LRESULT CCtrlGeneral::OnUpdatePosition(WPARAM, LPARAM lParam)
{
	Notification *pnotify = (Notification *)lParam;
	if (pnotify)
	{
		m_VuMeterLeft.SetVuMeter(pnotify->masterVUout[0] & (~Notification::ClipVU), pnotify->type[Notification::Stop]);
		m_VuMeterRight.SetVuMeter(pnotify->masterVUout[1] & (~Notification::ClipVU), pnotify->type[Notification::Stop]);
	}
	return 0;
}


BOOL CCtrlGeneral::GetToolTipText(UINT uId, LPTSTR pszText)
{
	const TCHAR moreRecentMixModeNote[] = _T("Use a more recent mixmode to see dB offsets.");
	if ((pszText) && (uId))
	{
		const bool displayDBValues = m_sndFile.GetPlayConfig().getDisplayDBValues();

		switch(uId)
		{
		case IDC_BUTTON_MODTYPE:
			_tcscpy(pszText, _T("Song Properties"));
			{
				const auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(kcViewSongProperties, 0);
				if (!keyText.IsEmpty())
					_tcscat(pszText, MPT_TFORMAT(" ({})")(keyText).c_str());
			}
			return TRUE;
		case IDC_BUTTON1:
			_tcscpy(pszText, _T("Click button multiple times to tap in the desired tempo."));
			return TRUE;
		case IDC_SLIDER_SAMPLEPREAMP:
			_tcscpy(pszText, displayDBValues ? CModDoc::LinearToDecibels(m_sndFile.m_nSamplePreAmp, m_sndFile.GetPlayConfig().getNormalSamplePreAmp()).GetString() : moreRecentMixModeNote);
			return TRUE;
		case IDC_SLIDER_VSTIVOL:
			_tcscpy(pszText, displayDBValues ? CModDoc::LinearToDecibels(m_sndFile.m_nVSTiVolume, m_sndFile.GetPlayConfig().getNormalVSTiVol()).GetString() : moreRecentMixModeNote);
			return TRUE;
		case IDC_SLIDER_GLOBALVOL:
			_tcscpy(pszText, displayDBValues ? CModDoc::LinearToDecibels(m_sndFile.m_PlayState.m_nGlobalVolume, m_sndFile.GetPlayConfig().getNormalGlobalVol()).GetString() : moreRecentMixModeNote);
			return TRUE;
		}
	}
	return FALSE;
	
}


void CCtrlGeneral::OnEnSetfocusEditSongtitle()
{
	m_EditTitle.SetLimitText(m_sndFile.GetModSpecifications().modNameLengthMax);
}


void CCtrlGeneral::OnResamplingChanged()
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
{
	CRect rect;
	CPaintDC dc(this);
	GetClientRect(&rect);
	dc.FillSolidRect(rect.left, rect.top, rect.Width(), rect.Height(), RGB(0,0,0));
	m_lastDisplayedLevel = -1;
	DrawVuMeter(dc, true);
}


void CVuMeter::SetVuMeter(int level, bool force)
{
	level >>= 8;
	if (level != m_lastLevel)
	{
		DWORD curTime = timeGetTime();
		if(curTime - m_lastVuUpdateTime >= TrackerSettings::Instance().VuMeterUpdateInterval || force)
		{
			m_lastLevel = level;
			CClientDC dc(this);
			DrawVuMeter(dc);
			m_lastVuUpdateTime = curTime;
		}
	}
}


void CVuMeter::DrawVuMeter(CDC &dc, bool /*redraw*/)
{
	CRect rect;
	GetClientRect(&rect);
	int vu = (m_lastLevel * (rect.bottom-rect.top)) >> 8;
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
	m_lastDisplayedLevel = m_lastLevel;
}


OPENMPT_NAMESPACE_END
