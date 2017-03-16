/*
 * dlg_misc.cpp
 * ------------
 * Purpose: Implementation of various OpenMPT dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Moddoc.h"
#include "Mainfrm.h"
#include "dlg_misc.h"
#include "Dlsbank.h"
#include "ChildFrm.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "ChannelManagerDlg.h"
#include "TempoSwingDialog.h"
#include "../soundlib/mod_specifications.h"
#include "../common/version.h"
#include "../common/StringFixer.h"


OPENMPT_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////
// CModTypeDlg


BEGIN_MESSAGE_MAP(CModTypeDlg, CDialog)
	//{{AFX_MSG_MAP(CModTypeDlg)
	ON_CBN_SELCHANGE(IDC_COMBO1,			UpdateDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_TEMPOMODE,	OnTempoModeChanged)
	ON_COMMAND(IDC_CHECK_PT1X,				OnPTModeChanged)
	ON_COMMAND(IDC_BUTTON1,					OnTempoSwing)
	ON_COMMAND(IDC_BUTTON2,					OnLegacyPlaybackSettings)
	ON_COMMAND(IDC_BUTTON3,					OnDefaultBehaviour)

	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)

	//}}AFX_MSG_MAP

END_MESSAGE_MAP()


void CModTypeDlg::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModTypeDlg)
	DDX_Control(pDX, IDC_COMBO1,		m_TypeBox);
	DDX_Control(pDX, IDC_COMBO2,		m_ChannelsBox);
	DDX_Control(pDX, IDC_COMBO_TEMPOMODE,	m_TempoModeBox);
	DDX_Control(pDX, IDC_COMBO_MIXLEVELS,	m_PlugMixBox);

	DDX_Control(pDX, IDC_CHECK1,		m_CheckBox1);
	DDX_Control(pDX, IDC_CHECK2,		m_CheckBox2);
	DDX_Control(pDX, IDC_CHECK3,		m_CheckBox3);
	DDX_Control(pDX, IDC_CHECK4,		m_CheckBox4);
	DDX_Control(pDX, IDC_CHECK5,		m_CheckBox5);
	DDX_Control(pDX, IDC_CHECK_PT1X,	m_CheckBoxPT1x);
	DDX_Control(pDX, IDC_CHECK_AMIGALIMITS,	m_CheckBoxAmigaLimits);

	//}}AFX_DATA_MAP
}


BOOL CModTypeDlg::OnInitDialog()
//------------------------------
{
	CDialog::OnInitDialog();
	m_nType = sndFile.GetType();
	m_nChannels = sndFile.GetNumChannels();
	m_tempoSwing = sndFile.m_tempoSwing;
	m_playBehaviour = sndFile.m_playBehaviour;
	initialized = false;

	// Mod types

	m_TypeBox.SetItemData(m_TypeBox.AddString(_T("ProTracker MOD")), MOD_TYPE_MOD);
	m_TypeBox.SetItemData(m_TypeBox.AddString(_T("ScreamTracker S3M")), MOD_TYPE_S3M);
	m_TypeBox.SetItemData(m_TypeBox.AddString(_T("FastTracker XM")), MOD_TYPE_XM);
	m_TypeBox.SetItemData(m_TypeBox.AddString(_T("Impulse Tracker IT")), MOD_TYPE_IT);
	m_TypeBox.SetItemData(m_TypeBox.AddString(_T("OpenMPT MPTM")), MOD_TYPE_MPT);
	switch(m_nType)
	{
	case MOD_TYPE_S3M:	m_TypeBox.SetCurSel(1); break;
	case MOD_TYPE_XM:	m_TypeBox.SetCurSel(2); break;
	case MOD_TYPE_IT:	m_TypeBox.SetCurSel(3); break;
	case MOD_TYPE_MPT:	m_TypeBox.SetCurSel(4); break;
	default:			m_TypeBox.SetCurSel(0); break;
	}

	// Time signature information

	SetDlgItemInt(IDC_ROWSPERBEAT, sndFile.m_nDefaultRowsPerBeat);
	SetDlgItemInt(IDC_ROWSPERMEASURE, sndFile.m_nDefaultRowsPerMeasure);

	// Version information

	if(sndFile.m_dwCreatedWithVersion) SetDlgItemText(IDC_EDIT_CREATEDWITH, _T("OpenMPT ") + FormatVersionNumber(sndFile.m_dwCreatedWithVersion));
	SetDlgItemText(IDC_EDIT_SAVEDWITH, sndFile.m_madeWithTracker.c_str());

	const int iconSize = Util::ScalePixels(32, m_hWnd);
	m_warnIcon = (HICON)::LoadImage(NULL, IDI_EXCLAMATION, IMAGE_ICON, iconSize, iconSize, LR_SHARED);

	UpdateDialog();

	initialized = true;
	EnableToolTips(TRUE);
	return TRUE;
}


CString CModTypeDlg::FormatVersionNumber(DWORD version)
//-----------------------------------------------------
{
	return std::string(MptVersion::ToStr(version) + (MptVersion::IsTestBuild(version) ? " (test build)" : "")).c_str();
}


void CModTypeDlg::UpdateChannelCBox()
//-----------------------------------
{
	const MODTYPE type = static_cast<MODTYPE>(m_TypeBox.GetItemData(m_TypeBox.GetCurSel()));
	CHANNELINDEX currChanSel = static_cast<CHANNELINDEX>(m_ChannelsBox.GetItemData(m_ChannelsBox.GetCurSel()));
	const CHANNELINDEX minChans = CSoundFile::GetModSpecifications(type).channelsMin;
	const CHANNELINDEX maxChans = CSoundFile::GetModSpecifications(type).channelsMax;
	
	if(m_ChannelsBox.GetCount() < 1 
		|| m_ChannelsBox.GetItemData(0) != minChans
		|| m_ChannelsBox.GetItemData(m_ChannelsBox.GetCount() - 1) != maxChans)
	{
		// Update channel list if number of supported channels has changed.
		if(m_ChannelsBox.GetCount() < 1) currChanSel = m_nChannels;
		m_ChannelsBox.ResetContent();

		CString s;
		for(CHANNELINDEX i = minChans; i <= maxChans; i++)
		{
			s.Format(_T("%u Channel%s"), i, (i != 1) ? _T("s") : _T(""));
			m_ChannelsBox.SetItemData(m_ChannelsBox.AddString(s), i);
		}

		Limit(currChanSel, minChans, maxChans);
		m_ChannelsBox.SetCurSel(currChanSel - minChans);
	}
}


void CModTypeDlg::UpdateDialog()
//------------------------------
{
	m_nType = static_cast<MODTYPE>(m_TypeBox.GetItemData(m_TypeBox.GetCurSel()));

	UpdateChannelCBox();

	m_CheckBox1.SetCheck(sndFile.m_SongFlags[SONG_LINEARSLIDES] ? BST_CHECKED : BST_UNCHECKED);
	m_CheckBox2.SetCheck(sndFile.m_SongFlags[SONG_FASTVOLSLIDES] ? BST_CHECKED : BST_UNCHECKED);
	m_CheckBox3.SetCheck(sndFile.m_SongFlags[SONG_ITOLDEFFECTS] ? BST_CHECKED : BST_UNCHECKED);
	m_CheckBox4.SetCheck(sndFile.m_SongFlags[SONG_ITCOMPATGXX] ? BST_CHECKED : BST_UNCHECKED);
	m_CheckBox5.SetCheck(sndFile.m_SongFlags[SONG_EXFILTERRANGE] ? BST_CHECKED : BST_UNCHECKED);
	m_CheckBoxPT1x.SetCheck(sndFile.m_SongFlags[SONG_PT_MODE] ? BST_CHECKED : BST_UNCHECKED);
	m_CheckBoxAmigaLimits.SetCheck(sndFile.m_SongFlags[SONG_AMIGALIMITS] ? BST_CHECKED : BST_UNCHECKED);

	const FlagSet<SongFlags> allowedFlags(sndFile.GetModSpecifications(m_nType).songFlags);
	m_CheckBox1.EnableWindow(allowedFlags[SONG_LINEARSLIDES]);
	m_CheckBox2.EnableWindow(allowedFlags[SONG_FASTVOLSLIDES]);
	m_CheckBox3.EnableWindow(allowedFlags[SONG_ITOLDEFFECTS]);
	m_CheckBox4.EnableWindow(allowedFlags[SONG_ITCOMPATGXX]);
	m_CheckBox5.EnableWindow(allowedFlags[SONG_EXFILTERRANGE]);
	m_CheckBoxPT1x.EnableWindow(allowedFlags[SONG_PT_MODE]);
	m_CheckBoxAmigaLimits.EnableWindow(allowedFlags[SONG_AMIGALIMITS]);

	// These two checkboxes are mutually exclusive and share the same screen space
	m_CheckBoxPT1x.ShowWindow(m_nType == MOD_TYPE_MOD ? SW_SHOW : SW_HIDE);
	m_CheckBox5.ShowWindow(m_nType != MOD_TYPE_MOD ? SW_SHOW : SW_HIDE);
	if(allowedFlags[SONG_PT_MODE]) OnPTModeChanged();

	// Tempo modes
	const TempoMode oldTempoMode = initialized ? static_cast<TempoMode>(m_TempoModeBox.GetItemData(m_TempoModeBox.GetCurSel())) : sndFile.m_nTempoMode;
	m_TempoModeBox.ResetContent();

	m_TempoModeBox.SetItemData(m_TempoModeBox.AddString(_T("Classic")), tempoModeClassic);
	if(m_nType == MOD_TYPE_MPT || (sndFile.GetType() != MOD_TYPE_MPT && sndFile.m_nTempoMode == tempoModeAlternative))
		m_TempoModeBox.SetItemData(m_TempoModeBox.AddString(_T("Alternative")), tempoModeAlternative);
	if(m_nType == MOD_TYPE_MPT || (sndFile.GetType() != MOD_TYPE_MPT && sndFile.m_nTempoMode == tempoModeModern))
		m_TempoModeBox.SetItemData(m_TempoModeBox.AddString(_T("Modern (accurate)")), tempoModeModern);
	m_TempoModeBox.SetCurSel(0);
	for(int i = m_TempoModeBox.GetCount(); i > 0; i--)
	{
		if(static_cast<TempoMode>(m_TempoModeBox.GetItemData(i)) == oldTempoMode)
		{
			m_TempoModeBox.SetCurSel(i);
			break;
		}
	}
	OnTempoModeChanged();

	// Mix levels
	const MixLevels oldMixLevels = initialized ? static_cast<MixLevels>(m_PlugMixBox.GetItemData(m_PlugMixBox.GetCurSel())) : sndFile.GetMixLevels();
	m_PlugMixBox.ResetContent();
	if(m_nType == MOD_TYPE_MPT || (sndFile.GetType() != MOD_TYPE_MPT && sndFile.GetMixLevels() == mixLevels1_17RC3))	// In XM/IT, this is only shown for backwards compatibility with existing tunes
		m_PlugMixBox.SetItemData(m_PlugMixBox.AddString(_T("OpenMPT 1.17RC3")),	mixLevels1_17RC3);
	if(sndFile.GetMixLevels() == mixLevels1_17RC2)	// Only shown for backwards compatibility with existing tunes
		m_PlugMixBox.SetItemData(m_PlugMixBox.AddString(_T("OpenMPT 1.17RC2")),	mixLevels1_17RC2);
	if(sndFile.GetMixLevels() == mixLevels1_17RC1)	// Ditto
		m_PlugMixBox.SetItemData(m_PlugMixBox.AddString(_T("OpenMPT 1.17RC1")),	mixLevels1_17RC1);
	if(sndFile.GetMixLevels() == mixLevelsOriginal)	// Ditto
		m_PlugMixBox.SetItemData(m_PlugMixBox.AddString(_T("Original (MPT 1.16)")),	mixLevelsOriginal);
	int compatMixMode = m_PlugMixBox.AddString(_T("Compatible"));
	m_PlugMixBox.SetItemData(compatMixMode,										mixLevelsCompatible);
	if(m_nType == MOD_TYPE_XM)
		m_PlugMixBox.SetItemData(m_PlugMixBox.AddString(_T("Compatible (FT2 Pan Law)")), mixLevelsCompatibleFT2);

	// Default to compatible mix mode
	m_PlugMixBox.SetCurSel(compatMixMode);
	int mixCount = m_PlugMixBox.GetCount();
	for(int i = 0; i < mixCount; i++)
	{
		if(static_cast<MixLevels>(m_PlugMixBox.GetItemData(i)) == oldMixLevels)
		{
			m_PlugMixBox.SetCurSel(i);
			break;
		}
	}

	const bool XMorITorMPT = (m_nType & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT));

	// Mixmode Box
	GetDlgItem(IDC_TEXT_MIXMODE)->EnableWindow(XMorITorMPT);
	m_PlugMixBox.EnableWindow(XMorITorMPT);
	
	// Tempo mode box
	m_TempoModeBox.EnableWindow(XMorITorMPT);
	GetDlgItem(IDC_ROWSPERBEAT)->EnableWindow(XMorITorMPT);
	GetDlgItem(IDC_ROWSPERMEASURE)->EnableWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_ROWSPERBEAT)->EnableWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_ROWSPERMEASURE)->EnableWindow(XMorITorMPT);
	GetDlgItem(IDC_TEXT_TEMPOMODE)->EnableWindow(XMorITorMPT);
	GetDlgItem(IDC_FRAME_TEMPOMODE)->EnableWindow(XMorITorMPT);

	// Compatibility settings
	PlayBehaviourSet defaultBehaviour = CSoundFile::GetDefaultPlaybackBehaviour(m_nType);
	bool usesDefaultBehaviour = true;
	const bool isMPTM = (m_nType == MOD_TYPE_MPT);
	if(m_nType & (MOD_TYPE_MPT | MOD_TYPE_IT | MOD_TYPE_XM))
	{
		for(size_t i = 0; i < m_playBehaviour.size(); i++)
		{
			// Some flags are not really important for "default" behaviour.
			if(defaultBehaviour[i] != m_playBehaviour[i]
				&& i != MSF_COMPATIBLE_PLAY
				&& i != kFT2VolumeRamping)
			{
				usesDefaultBehaviour = false;
				break;
			}
		}
	}
	static_cast<CStatic *>(GetDlgItem(IDC_STATIC1))->SetIcon((usesDefaultBehaviour || isMPTM) ? NULL : m_warnIcon);
	GetDlgItem(IDC_STATIC2)->SetWindowText((usesDefaultBehaviour || isMPTM)
		? _T("Compatibility settings are currently optimal. It is advised to not edit them.")
		: _T("Playback settings have been set to compatibility mode. Click \"Set Defaults\" to use the recommended settings instead."));
	GetDlgItem(IDC_BUTTON3)->EnableWindow(usesDefaultBehaviour ? FALSE : TRUE);
}


void CModTypeDlg::OnPTModeChanged()
//---------------------------------
{
	// PT1/2 mode enforces Amiga limits
	const bool ptMode = IsDlgButtonChecked(IDC_CHECK_PT1X) != BST_UNCHECKED;
	m_CheckBoxAmigaLimits.EnableWindow(!ptMode);
	if(ptMode) m_CheckBoxAmigaLimits.SetCheck(BST_CHECKED);
}


void CModTypeDlg::OnTempoModeChanged()
//------------------------------------
{
	GetDlgItem(IDC_BUTTON1)->EnableWindow(m_TempoModeBox.GetItemData(m_TempoModeBox.GetCurSel()) == tempoModeModern);
}


void CModTypeDlg::OnTempoSwing()
//------------------------------
{
	const ROWINDEX oldRPB = sndFile.m_nDefaultRowsPerBeat;
	const ROWINDEX oldRPM = sndFile.m_nDefaultRowsPerMeasure;
	const TempoMode oldMode = sndFile.m_nTempoMode;

	// Temporarily apply new tempo signature for preview
	ROWINDEX newRPB = std::max(1u, GetDlgItemInt(IDC_ROWSPERBEAT));
	ROWINDEX newRPM = std::max(newRPB, GetDlgItemInt(IDC_ROWSPERMEASURE));
	sndFile.m_nDefaultRowsPerBeat = newRPB;
	sndFile.m_nDefaultRowsPerMeasure = newRPM;
	sndFile.m_nTempoMode = tempoModeModern;

	m_tempoSwing.resize(GetDlgItemInt(IDC_ROWSPERBEAT), TempoSwing::Unity);
	CTempoSwingDlg dlg(this, m_tempoSwing, sndFile);
	if(dlg.DoModal() == IDOK)
	{
		m_tempoSwing = dlg.m_tempoSwing;
	}
	sndFile.m_nDefaultRowsPerBeat = oldRPB;
	sndFile.m_nDefaultRowsPerMeasure = oldRPM;
	sndFile.m_nTempoMode = oldMode;
}


void CModTypeDlg::OnLegacyPlaybackSettings()
//------------------------------------------
{
	CLegacyPlaybackSettingsDlg dlg(this, m_playBehaviour, m_nType);
	dlg.DoModal();
	UpdateDialog();
}


void CModTypeDlg::OnDefaultBehaviour()
//------------------------------------
{
	m_playBehaviour = CSoundFile::GetDefaultPlaybackBehaviour(m_nType);
	UpdateDialog();
}


bool CModTypeDlg::VerifyData()
//----------------------------
{

	int temp_nRPB = GetDlgItemInt(IDC_ROWSPERBEAT);
	int temp_nRPM = GetDlgItemInt(IDC_ROWSPERMEASURE);
	if(temp_nRPB > temp_nRPM)
	{
		Reporting::Warning("Error: Rows per measure must be greater than or equal to rows per beat.");
		GetDlgItem(IDC_ROWSPERMEASURE)->SetFocus();
		return false;
	}

	int sel = m_ChannelsBox.GetItemData(m_ChannelsBox.GetCurSel());
	MODTYPE type = static_cast<MODTYPE>(m_TypeBox.GetItemData(m_TypeBox.GetCurSel()));

	CHANNELINDEX maxChans = CSoundFile::GetModSpecifications(type).channelsMax;

	if(sel > maxChans)
	{
		CString error;
		error.Format(_T("Error: Maximum number of channels for this module type is %u."), maxChans);
		Reporting::Warning(error);
		return false;
	}

	if(maxChans < sndFile.GetNumChannels())
	{
		if(Reporting::Confirm("New module type supports less channels than currently used, and reducing channel number is required. Continue?") != cnfYes)
			return false;
	}

	return true;
}


void CModTypeDlg::OnOK()
//----------------------
{
	if (!VerifyData())
		return;

	int sel = m_TypeBox.GetCurSel();
	if (sel >= 0)
	{
		m_nType = static_cast<MODTYPE>(m_TypeBox.GetItemData(sel));
	}

	sndFile.m_SongFlags.set(SONG_LINEARSLIDES, m_CheckBox1.GetCheck() != BST_UNCHECKED);
	sndFile.m_SongFlags.set(SONG_FASTVOLSLIDES, m_CheckBox2.GetCheck() != BST_UNCHECKED);
	sndFile.m_SongFlags.set(SONG_ITOLDEFFECTS, m_CheckBox3.GetCheck() != BST_UNCHECKED);
	sndFile.m_SongFlags.set(SONG_ITCOMPATGXX, m_CheckBox4.GetCheck() != BST_UNCHECKED);
	sndFile.m_SongFlags.set(SONG_EXFILTERRANGE, m_CheckBox5.GetCheck() != BST_UNCHECKED);
	sndFile.m_SongFlags.set(SONG_PT_MODE, m_CheckBoxPT1x.GetCheck() != BST_UNCHECKED);
	sndFile.m_SongFlags.set(SONG_AMIGALIMITS, m_CheckBoxAmigaLimits.GetCheck() != BST_UNCHECKED);

	sel = m_ChannelsBox.GetCurSel();
	if (sel >= 0)
	{
		m_nChannels = static_cast<CHANNELINDEX>(m_ChannelsBox.GetItemData(sel));
	}
	
	sndFile.m_nDefaultRowsPerBeat    = GetDlgItemInt(IDC_ROWSPERBEAT);
	sndFile.m_nDefaultRowsPerMeasure = GetDlgItemInt(IDC_ROWSPERMEASURE);

	sel = m_TempoModeBox.GetCurSel();
	if (sel >= 0)
	{
		sndFile.m_nTempoMode = static_cast<TempoMode>(m_TempoModeBox.GetItemData(sel));
	}
	if(sndFile.m_nTempoMode == tempoModeModern)
	{
		sndFile.m_tempoSwing = m_tempoSwing;
		sndFile.m_tempoSwing.resize(sndFile.m_nDefaultRowsPerBeat);
	} else
	{
		sndFile.m_tempoSwing.clear();
	}

	sel = m_PlugMixBox.GetCurSel();
	if(sel >= 0)
	{
		sndFile.SetMixLevels(static_cast<MixLevels>(m_PlugMixBox.GetItemData(sel)));
	}

	PlayBehaviourSet allowedFlags = CSoundFile::GetSupportedPlaybackBehaviour(m_nType);
	for(size_t i = 0; i < kMaxPlayBehaviours; i++)
	{
		// Only set those flags which are supported by the new format or were already enabled previously
		sndFile.m_playBehaviour.set(i, m_playBehaviour[i] && (allowedFlags[i] || (sndFile.m_playBehaviour[i] && sndFile.GetType() == m_nType)));
	}

	DestroyIcon(m_warnIcon);
	CDialog::OnOK();
}


BOOL CModTypeDlg::OnToolTipNotify(UINT, NMHDR *pNMHDR, LRESULT *)
//---------------------------------------------------------------
{
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXTA*)pNMHDR;
	const TCHAR *text = _T("");
	UINT_PTR nID = pNMHDR->idFrom;
	if(pTTT->uFlags & TTF_IDISHWND)
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	switch(nID)
	{
	case IDC_CHECK1:
		text = _T("Note slides always slide the same amount, not depending on the sample frequency.");
		break;
	case IDC_CHECK2:
		text = _T("Old ScreamTracker 3 volume slide behaviour (not recommended).");
		break;
	case IDC_CHECK3:
		text = _T("Play some effects like in early versions of Impulse Tracker (not recommended).");
		break;
	case IDC_CHECK4:
		text = _T("Gxx and Exx/Fxx won't share effect memory. Gxx resets instrument envelopes.");
		break;
	case IDC_CHECK5:
		text = _T("The resonant filter's frequency range is increased from about 5kHz to 10kHz.");
		break;
	case IDC_CHECK_PT1X:
		text = _T("Enforce Amiga frequency limits, ProTracker offset bug emulation.");
		break;
	case IDC_COMBO_MIXLEVELS:
		text = _T("Mixing method of sample and instrument plugin levels.");
		break;
	case IDC_BUTTON1:
		if(!GetDlgItem(IDC_BUTTON1)->IsWindowEnabled())
		{
			text = _T("Tempo swing is only available in modern tempo mode.");
		} else
		{
			CString s = _T("Swing setting: ");
			if(m_tempoSwing.empty())
			{
				s += _T("Default");
			} else
			{
				for(size_t i = 0; i < m_tempoSwing.size(); i++)
				{
					if(i > 0) s += _T(" / ");
					s.AppendFormat(_T("%u%%"), Util::muldivr(m_tempoSwing[i], 100, TempoSwing::Unity));
				}
			}
			mpt::String::CopyN(pTTT->szText, s);
			return TRUE;
		}
	}

	mpt::String::CopyN(pTTT->szText, text);
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
// CLegacyPlaybackSettings

BEGIN_MESSAGE_MAP(CLegacyPlaybackSettingsDlg, CDialog)
	ON_COMMAND(IDC_BUTTON1,			OnSelectDefaults)
	ON_CLBN_CHKCHANGE(IDC_LIST1,	UpdateSelectDefaults)
END_MESSAGE_MAP()

void CLegacyPlaybackSettingsDlg::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1,	m_CheckList);
}


BOOL CLegacyPlaybackSettingsDlg::OnInitDialog()
//---------------------------------------------
{
	CDialog::OnInitDialog();

	PlayBehaviourSet allowedFlags = CSoundFile::GetSupportedPlaybackBehaviour(m_modType);
	for(size_t i = 0; i < kMaxPlayBehaviours; i++)
	{
		const TCHAR *desc = _T("");
		switch(i)
		{
		case MSF_COMPATIBLE_PLAY: continue;

		case kMPTOldSwingBehaviour: desc = _T("OpenMPT 1.17 compatible random variation behaviour for instruments"); break;
		case kMIDICCBugEmulation: desc = _T("Plugin volume MIDI CC bug emulation"); break;
		case kOldMIDIPitchBends: desc = _T("Old Pitch Wheel behaviour for instrument plugins"); break;
		case kFT2VolumeRamping: desc = _T("Use smooth Fasttracker 2 volume ramping"); break;
		case kMODVBlankTiming: desc = _T("VBlank timing: F21 and above sets speed instead of tempo"); break;

		case kSlidesAtSpeed1: desc = _T("Execute regular portamento slides at speed 1"); break;
		case kHertzInLinearMode: desc = _T("Compute note frequency in Hertz rather than periods"); break;
		case kTempoClamp: desc = _T("Clamp tempo to 32-255 range"); break;
		case kPerChannelGlobalVolSlide: desc = _T("Global volume slide memory is per-channel"); break;
		case kPanOverride: desc = _T("Panning commands override surround and random pan variation"); break;

		case kITInstrWithoutNote: desc = _T("Retrigger instrument envelopes on instrument change"); break;
		case kITVolColFinePortamento: desc = _T("Volume column portamento never does fine portamento"); break;
		case kITArpeggio: desc = _T("IT arpeggio algorithm"); break;
		case kITOutOfRangeDelay: desc = _T("Out-of-range delay commands queue new instrument"); break;
		case kITPortaMemoryShare: desc = _T("Gxx shares memory with Exx and Fxx"); break;
		case kITPatternLoopTargetReset: desc = _T("After finishing a pattern loop, set the pattern loop target to the next row"); break;
		case kITFT2PatternLoop: desc = _T("Nested pattern loop behaviour"); break;
		case kITPingPongNoReset: desc = _T("Do not reset ping pong direction with instrument numbers"); break;
		case kITEnvelopeReset: desc = _T("IT envelope reset behaviour"); break;
		case kITClearOldNoteAfterCut: desc = _T("Forget the previous note after cutting it"); break;
		case kITVibratoTremoloPanbrello: desc = _T("More IT-like Vibrato, Tremolo and Panbrello handling"); break;
		case kITTremor: desc = _T("Ixx behaves like in IT"); break;
		case kITRetrigger: desc = _T("Qxx behaves like in IT"); break;
		case kITMultiSampleBehaviour: desc = _T("Properly update C-5 frequency when changing note in multisampled instrument"); break;
		case kITPortaTargetReached: desc = _T("Clear portamento target after it has been reached"); break;
		case kITPatternLoopBreak: desc = _T("Do not reset loop count on pattern break."); break;
		case kITOffset: desc = _T("Offset after sample end is treated like in IT"); break;
		case kITSwingBehaviour: desc = _T("Volume and panning random variation work more like in IT"); break;
		case kITNNAReset: desc = _T("NNA is reset on every note change, not every instrument change"); break;
		case kITSCxStopsSample: desc = _T("SCx really stops the sample and does not just mute it"); break;
		case kITEnvelopePositionHandling: desc = _T("IT-style envelope position advance + enable/disable behaviour"); break;
		case kITPortamentoInstrument: desc = _T("More compatible instrument change + portamento"); break;
		case kITPingPongMode: desc = _T("Do not repeat last sample point in ping pong loop, like IT's software mixer"); break;
		case kITRealNoteMapping: desc = _T("Use triggered note rather than translated note for PPS and DNA note check"); break;
		case kITHighOffsetNoRetrig: desc = _T("SAx does not apply an offset effect to a note next to it"); break;
		case kITFilterBehaviour: desc = _T("User IT's filter coefficients (unless extended filter range is used) and behaviour"); break;
		case kITNoSurroundPan: desc = _T("Panning modulation is disabled on surround channels"); break;
		case kITShortSampleRetrig: desc = _T("Do not retrigger already stopped channels"); break;
		case kITPortaNoNote: desc = _T("Do not apply any portamento if no previous note is playing"); break;
		case kITDontResetNoteOffOnPorta: desc = _T("Only reset note-off status on portamento in IT Compatible Gxx mode"); break;
		case kITVolColMemory: desc = _T("Volume column effects share their memory with the effect column"); break;
		case kITPortamentoSwapResetsPos: desc = _T("Portamento with sample swap plays the new sample from the beginning"); break;
		case kITEmptyNoteMapSlot: desc = _T("Ignore instrument note map entries with no note completely"); break;
		case kITFirstTickHandling: desc = _T("IT-style first tick handling"); break;
		case kITSampleAndHoldPanbrello: desc = _T("IT-style sample&hold panbrello waveform"); break;
		case kITClearPortaTarget: desc = _T("New notes reset portamento target in IT"); break;
		case kITPanbrelloHold: desc = _T("Do not reset panbrello effect until next note or panning effect"); break;
		case kITPanningReset: desc = _T("Sample and instrument panning is only applied on note change, not instrument change"); break;
		case kITPatternLoopWithJumps: desc = _T("Bxx on the same row as SBx terminates the loop in IT"); break;
		case kITInstrWithNoteOff: desc = _T("Instrument number with note-off recalls default volume"); break;
		case kFT2Arpeggio: desc = _T("FT2 arpeggio algorithm"); break;
		case kFT2Retrigger: desc = _T("Rxx behaves like in FT2"); break;
		case kFT2VolColVibrato: desc = _T("Vibrato speed in volume column does not actually execute the vibrato effect"); break;
		case kFT2PortaNoNote: desc = _T("Do not play portamento-ed note if no previous note is playing"); break;
		case kFT2KeyOff: desc = _T("FT2-style Kxx handling"); break;
		case kFT2PanSlide: desc = _T("Volume-column pan slides are finer"); break;
		case kFT2OffsetOutOfRange: desc = _T("FT2-style 9xx edge case handling"); break;
		case kFT2RestrictXCommand: desc = _T("Do not allow ModPlug extensions to X command"); break;
		case kFT2RetrigWithNoteDelay: desc = _T("Retrigger envelopes if there is a note delay with no note"); break;
		case kFT2SetPanEnvPos: desc = _T("Lxx only sets the pan envelope position if the volume envelope's sustain flag is set"); break;
		case kFT2PortaIgnoreInstr: desc = _T("Portamento with instrument number applies volume settings of new sample, but not the new sample itself"); break;
		case kFT2VolColMemory: desc = _T("No volume column memory"); break;
		case kFT2LoopE60Restart: desc = _T("Next pattern starts on the same row as the last E60 command"); break;
		case kFT2ProcessSilentChannels: desc = _T("Keep processing faded channels for later portamento pickup"); break;
		case kFT2ReloadSampleSettings: desc = _T("Reload sample settings even if a note-off is placed next to an instrument number"); break;
		case kFT2PortaDelay: desc = _T("Portamento with note delay next to it is ignored"); break;
		case kFT2Transpose: desc = _T("Ignore out-of-range transposed notes"); break;
		case kFT2PatternLoopWithJumps: desc = _T("Bxx or Dxx on the same row as E6x terminates the loop"); break;
		case kFT2PortaTargetNoReset: desc = _T("Portamento target is not reset with new notes"); break;
		case kFT2EnvelopeEscape: desc = _T("Sustain point at end of envelope loop stops the loop after release"); break;
		case kFT2Tremor: desc = _T("Txx behaves like in FT2"); break;
		case kFT2OutOfRangeDelay: desc = _T("Do not trigger notes with out-of-range note delay"); break;
		case kFT2Periods: desc = _T("Use FT2's broken period handling"); break;
		case kFT2PanWithDelayedNoteOff: desc = _T("Panning command with delayed note-off is ignored"); break;
		case kFT2VolColDelay: desc = _T("FT2-style volume column handling if there is a note delay"); break;
		case kFT2FinetunePrecision: desc = _T("Round sample finetune to multiples of 16"); break;
		case kFT2NoteOffFlags: desc = _T("Fade instrument on note-off when there is no volume envelope; instrument numbers reset note-off status."); break;
		case kITMultiSampleInstrumentNumber: desc = _T("Lone instrument number after portamento within multi-sampled instrument sets the target sample's settings."); break;
		case kRowDelayWithNoteDelay: desc = _T("Note delays next to a row delay are repeated on every row repetition."); break;
		case kFT2TremoloRampWaveform: desc = _T("Emulate FT2's tremolo ramp down / triangle waveform"); break;
		case kFT2PortaUpDownMemory: desc = _T("Portamento Up and Down have separate effect memory"); break;
		case kST3NoMutedChannels: desc = _T("Do not process any effects on muted S3M channels"); break;
		case kST3EffectMemory: desc = _T("Most effects share the same memory"); break;
		case kST3PortaSampleChange: desc = _T("Portamento with instrument number applies volume settings of new sample, but not the new sample itself."); break;
		case kST3VibratoMemory: desc = _T("Do not remember vibrato type in effect memory"); break;
		case kST3LimitPeriod: desc = _T("ModPlug Tracker frequency limits"); break;
		case KST3PortaAfterArpeggio: desc = _T("Portamento immediately following an arpeggio effect continues at the last arpeggiated note."); break;
		case kMODOneShotLoops: desc = _T("ProTracker one-shot loops"); break;
		case kMODIgnorePanning: desc = _T("Ignore panning commands."); break;
		case kMODSampleSwap: desc = _T("Enable on-the-fly sample swapping"); break;

		default: MPT_ASSERT(0);
		}

		if(m_playBehaviour[i] || allowedFlags[i])
		{
			int item = m_CheckList.AddString(desc);
			m_CheckList.SetItemData(item, i);
			int check = m_playBehaviour[i] ? BST_CHECKED : BST_UNCHECKED;
			if(!allowedFlags[i]) check = BST_INDETERMINATE;	// Is checked but not supported by format -> grey out
			m_CheckList.SetCheck(item, check);
		}
	}
	UpdateSelectDefaults();
	return TRUE;
}


void CLegacyPlaybackSettingsDlg::OnOK()
//-------------------------------------
{
	CDialog::OnOK();

	const int count = m_CheckList.GetCount();
	for(int i = 0; i < count; i++)
	{
		m_playBehaviour.set(m_CheckList.GetItemData(i), m_CheckList.GetCheck(i) != BST_UNCHECKED);
	}
}


void CLegacyPlaybackSettingsDlg::OnSelectDefaults()
//-------------------------------------------------
{
	const int count = m_CheckList.GetCount();
	PlayBehaviourSet defaults = CSoundFile::GetDefaultPlaybackBehaviour(m_modType);
	for(int i = 0; i < count; i++)
	{
		m_CheckList.SetCheck(i, defaults[m_CheckList.GetItemData(i)] ? BST_CHECKED : BST_UNCHECKED);
	}
}


void CLegacyPlaybackSettingsDlg::UpdateSelectDefaults()
//-----------------------------------------------------
{
	bool usesDefaults = false;
	const int count = m_CheckList.GetCount();
	PlayBehaviourSet defaults = CSoundFile::GetDefaultPlaybackBehaviour(m_modType);
	for(int i = 0; i < count; i++)
	{
		if((m_CheckList.GetCheck(i) != BST_UNCHECKED) != defaults[m_CheckList.GetItemData(i)])
		{
			usesDefaults = true;
			break;
		}
	}
	GetDlgItem(IDC_BUTTON1)->EnableWindow(usesDefaults ? TRUE : FALSE);
}



//////////////////////////////////////////////////////////////////////////////
// CShowLogDlg

BOOL CShowLogDlg::OnInitDialog()
//------------------------------
{
	CDialog::OnInitDialog();
	if (m_lpszTitle) SetWindowText(m_lpszTitle);
	return FALSE;
}


UINT CShowLogDlg::ShowLog(LPCSTR pszLog, LPCSTR lpszTitle)
//--------------------------------------------------------
{
	m_lpszLog = pszLog;
	m_lpszTitle = lpszTitle;
	return DoModal();
}


///////////////////////////////////////////////////////////
// CRemoveChannelsDlg

void CRemoveChannelsDlg::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REMCHANSLIST,		m_RemChansList);
}


BEGIN_MESSAGE_MAP(CRemoveChannelsDlg, CDialog)
	ON_LBN_SELCHANGE(IDC_REMCHANSLIST,		OnChannelChanged)
END_MESSAGE_MAP()



BOOL CRemoveChannelsDlg::OnInitDialog()
//-------------------------------------
{
	CHAR label[MAX(100, 20 + MAX_CHANNELNAME)];
	CDialog::OnInitDialog();
	for (UINT n = 0; n < m_nChannels; n++)
	{
		if(sndFile.ChnSettings[n].szName[0] >= 0x20)
			wsprintf(label, "Channel %d: %s", (n + 1), sndFile.ChnSettings[n].szName);
		else
			wsprintf(label, "Channel %d", n + 1);

		m_RemChansList.SetItemData(m_RemChansList.AddString(label), n);
		if (!m_bKeepMask[n]) m_RemChansList.SetSel(n);
	}

	if (m_nRemove > 0)
	{
		wsprintf(label, "Select %u channel%s to remove:", m_nRemove, (m_nRemove != 1) ? "s" : "");
	} else
	{
		wsprintf(label, "Select channels to remove (the minimum number of remaining channels is %u)", sndFile.GetModSpecifications().channelsMin);
	}
	
	SetDlgItemText(IDC_QUESTION1, label);
	if(GetDlgItem(IDCANCEL)) GetDlgItem(IDCANCEL)->ShowWindow(m_ShowCancel);

	OnChannelChanged();
	return TRUE;
}


void CRemoveChannelsDlg::OnOK()
//-----------------------------
{
	int nCount = m_RemChansList.GetSelCount();
	CArray<int,int> aryListBoxSel;
	aryListBoxSel.SetSize(nCount);
	m_RemChansList.GetSelItems(nCount, aryListBoxSel.GetData()); 

	m_bKeepMask.assign(m_nChannels, true);
	for (int n = 0; n < nCount; n++)
	{
		m_bKeepMask[aryListBoxSel[n]] = false;
	}
	if ((static_cast<CHANNELINDEX>(nCount) == m_nRemove && nCount > 0)  || (m_nRemove == 0 && (sndFile.GetNumChannels() >= nCount + sndFile.GetModSpecifications().channelsMin)))
		CDialog::OnOK();
	else
		CDialog::OnCancel();
}


void CRemoveChannelsDlg::OnChannelChanged()
//-----------------------------------------
{
	UINT nr = 0;
	nr = m_RemChansList.GetSelCount();
	GetDlgItem(IDOK)->EnableWindow(((nr == m_nRemove && nr >0)  || (m_nRemove == 0 && (sndFile.GetNumChannels() >= nr + sndFile.GetModSpecifications().channelsMin) && nr > 0)) ? TRUE : FALSE);
}


////////////////////////////////////////////////////////////////////////////////
// Sound Bank Information

CSoundBankProperties::CSoundBankProperties(const CDLSBank &bank, CWnd *parent) : CDialog(IDD_SOUNDBANK_INFO, parent)
//------------------------------------------------------------------------------------------------------------------
{
	
	fileName = bank.GetFileName();

	const SOUNDBANKINFO &bi = bank.GetBankInfo();
	m_szInfo.reserve(128 + bi.szBankName.size() + bi.szDescription.size() + bi.szCopyRight.size() + bi.szEngineer.size() + bi.szSoftware.size() + bi.szComments.size());
	m_szInfo = "Type:\t" + std::string((bank.GetBankType() & SOUNDBANK_TYPE_SF2) ? "Sound Font (SF2)" : "Downloadable Sound (DLS)");
	if (bi.szBankName.size())
		m_szInfo += "\r\nName:\t" + bi.szBankName;
	if (bi.szDescription.size())
		m_szInfo += "\r\n\t" + bi.szDescription;
	if (bi.szCopyRight.size())
		m_szInfo += "\r\nCopyright:\t" + bi.szCopyRight;
	if (bi.szEngineer.size())
		m_szInfo += "\r\nAuthor:\t" + bi.szEngineer;
	if (bi.szSoftware.size())
		m_szInfo += "\r\nSoftware:\t" + bi.szSoftware;
	if (bi.szComments.size())
	{
		m_szInfo += "\r\n\r\nComments:\r\n" + bi.szComments;
	}
}


BOOL CSoundBankProperties::OnInitDialog()
//---------------------------------------
{
	CDialog::OnInitDialog();
	SetDlgItemTextA(IDC_EDIT1, m_szInfo.c_str());
	SetWindowTextW(m_hWnd, (fileName.AsNative() + L" - Sound Bank Information").c_str());
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard Control

static const uint8 whitetab[7] = {0,2,4,5,7,9,11};
static const uint8 blacktab[7] = {0xff,1,3,0xff,6,8,10};

BEGIN_MESSAGE_MAP(CKeyboardControl, CWnd)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


void CKeyboardControl::Init(HWND parent, UINT nOctaves, bool cursNotify)
//----------------------------------------------------------------------
{
	m_hParent = parent;
	m_nOctaves = nOctaves;
	m_bCursorNotify = cursNotify;
	MemsetZero(KeyFlags);
	MemsetZero(m_sampleNum);
	
	// Point size to pixels
	int fontSize = -MulDiv(60, Util::GetDPIy(m_hWnd), 720);
#if _WIN32_WINNT >= 0x0501
	m_font.CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_DONTCARE, "MS Shell Dlg");
#else
	m_font.CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, "MS Shell Dlg");
#endif
}


void CKeyboardControl::OnDestroy()
//--------------------------------
{
	m_font.DeleteObject();
}


void CKeyboardControl::OnPaint()
//------------------------------
{
	HGDIOBJ oldpen, oldbrush;
	CRect rcClient, rect;
	CPaintDC dc(this);
	HDC hdc = dc.m_hDC;
	HBRUSH brushDot[2];

	if (!m_nOctaves) m_nOctaves = 1;
	dc.SetBkMode(TRANSPARENT);
	GetClientRect(&rcClient);
	rect = rcClient;
	oldpen = ::SelectObject(hdc, CMainFrame::penBlack);
	oldbrush = ::SelectObject(hdc, CMainFrame::brushWhite);
	brushDot[0] = ::CreateSolidBrush(RGB(0xFF, 0, 0));
	brushDot[1] = ::CreateSolidBrush(RGB(0xFF, 0xC0, 0xC0));
	CFont *oldFont = dc.SelectObject(&m_font);
	// White notes
	for (UINT note=0; note<m_nOctaves*7; note++)
	{
		rect.right = ((note + 1) * rcClient.Width()) / (m_nOctaves * 7);
		int val = (note/7) * 12 + whitetab[note % 7];
		if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushGray);
		dc.Rectangle(&rect);
		if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushWhite);
		if (val < NOTE_MAX && KeyFlags[val] != KEYFLAG_NORMAL && KeyFlags[val] < KEYFLAG_MAX)
		{
			CRect ellipseRect(rect.left + 2, rect.bottom - (rect.right - rect.left) + 2, rect.right - 2, rect.bottom - 2);
			::SelectObject(hdc, brushDot[KeyFlags[val] - 1]);
			dc.Ellipse(ellipseRect);
			::SelectObject(hdc, CMainFrame::brushWhite);
			if(m_sampleNum[val] != 0)
			{
				dc.SetTextColor(KeyFlags[val] == KEYFLAG_REDDOT ? RGB(255, 255, 255) : RGB(0, 0, 0));
				TCHAR s[16];
				wsprintf(s, _T("%u"), m_sampleNum[val]);
				dc.DrawText(s, -1, ellipseRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
			}
		}
		rect.left = rect.right - 1;
	}
	// Black notes
	::SelectObject(hdc, CMainFrame::brushBlack);
	rect = rcClient;
	rect.bottom -= rcClient.Height() / 3;
	for (UINT nblack=0; nblack<m_nOctaves*7; nblack++)
	{
		switch(nblack % 7)
		{
		case 1:
		case 2:
		case 4:
		case 5:
		case 6:
			{
				rect.left = (nblack * rcClient.Width()) / (m_nOctaves * 7);
				rect.right = rect.left;
				int delta = rcClient.Width() / (m_nOctaves * 7 * 3);
				rect.left -= delta;
				rect.right += delta;
				int val = (nblack/7)*12 + blacktab[nblack%7];
				if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushGray);
				dc.Rectangle(&rect);
				if (val == m_nSelection) ::SelectObject(hdc, CMainFrame::brushBlack);
				if (val < NOTE_MAX && KeyFlags[val] != KEYFLAG_NORMAL && KeyFlags[val] < KEYFLAG_MAX)
				{
					CRect ellipseRect(rect.left, rect.bottom - (rect.right - rect.left), rect.right, rect.bottom);
					::SelectObject(hdc, brushDot[KeyFlags[val] - 1]);
					dc.Ellipse(ellipseRect);
					::SelectObject(hdc, CMainFrame::brushBlack);
					if(m_sampleNum[val] != 0)
					{
						dc.SetTextColor(KeyFlags[val] == KEYFLAG_REDDOT ? RGB(255, 255, 255) : RGB(0, 0, 0));
						TCHAR s[16];
						wsprintf(s, _T("%u"), m_sampleNum[val]);
						dc.DrawText(s, -1, ellipseRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
					}
				}
			}
			break;
		}
	}
	if (oldpen) ::SelectObject(hdc, oldpen);
	if (oldbrush) ::SelectObject(hdc, oldbrush);
	for(auto &brush : brushDot)
	{
		DeleteBrush(brush);
	}
	dc.SelectObject(oldFont);
}


void CKeyboardControl::OnMouseMove(UINT flags, CPoint point)
//----------------------------------------------------------
{
	int sel = -1, xmin, xmax;
	CRect rcClient, rect;
	if (!m_nOctaves) m_nOctaves = 1;
	GetClientRect(&rcClient);
	rect = rcClient;
	xmin = rcClient.right;
	xmax = rcClient.left;
	// White notes
	for (UINT note=0; note<m_nOctaves*7; note++)
	{
		int val = (note/7)*12 + whitetab[note % 7];
		rect.right = ((note + 1) * rcClient.Width()) / (m_nOctaves * 7);
		if (val == m_nSelection)
		{
			if (rect.left < xmin) xmin = rect.left;
			if (rect.right > xmax) xmax = rect.right;
		}
		if (rect.PtInRect(point))
		{
			sel = val;
			if (rect.left < xmin) xmin = rect.left;
			if (rect.right > xmax) xmax = rect.right;
		}
		rect.left = rect.right - 1;
	}
	// Black notes
	rect = rcClient;
	rect.bottom -= rcClient.Height() / 3;
	for (UINT nblack=0; nblack<m_nOctaves*7; nblack++)
	{
		switch(nblack % 7)
		{
		case 1:
		case 2:
		case 4:
		case 5:
		case 6:
			{
				int val = (nblack/7)*12 + blacktab[nblack % 7];
				rect.left = (nblack * rcClient.Width()) / (m_nOctaves * 7);
				rect.right = rect.left;
				int delta = rcClient.Width() / (m_nOctaves * 7 * 3);
				rect.left -= delta;
				rect.right += delta;
				if (val == m_nSelection)
				{
					if (rect.left < xmin) xmin = rect.left;
					if (rect.right > xmax) xmax = rect.right;
				}
				if (rect.PtInRect(point))
				{
					sel = val;
					if (rect.left < xmin) xmin = rect.left;
					if (rect.right > xmax) xmax = rect.right;
				}
			}
			break;
		}
	}
	// Check for selection change
	if (sel != m_nSelection)
	{
		m_nSelection = sel;
		rcClient.left = xmin;
		rcClient.right = xmax;
		InvalidateRect(&rcClient, FALSE);
		if ((m_bCursorNotify) && (m_hParent))
		{
			::PostMessage(m_hParent, WM_MOD_KBDNOTIFY, KBDNOTIFY_MOUSEMOVE, m_nSelection);
			if((flags & MK_LBUTTON))
			{
				::SendMessage(m_hParent, WM_MOD_KBDNOTIFY, KBDNOTIFY_LBUTTONDOWN, m_nSelection);
			}
		}
	}
	if (sel >= 0)
	{
		if (!m_bCapture)
		{
			m_bCapture = TRUE;
			SetCapture();
		}
	} else
	{
		if (m_bCapture)
		{
			m_bCapture = FALSE;
			ReleaseCapture();
		}
	}
}


void CKeyboardControl::OnLButtonDown(UINT, CPoint)
//------------------------------------------------
{
	if ((m_nSelection != -1) && (m_hParent))
	{
		::SendMessage(m_hParent, WM_MOD_KBDNOTIFY, KBDNOTIFY_LBUTTONDOWN, m_nSelection);
	}
}


void CKeyboardControl::OnLButtonUp(UINT, CPoint)
//----------------------------------------------
{
	if ((m_nSelection != -1) && (m_hParent))
	{
		::SendMessage(m_hParent, WM_MOD_KBDNOTIFY, KBDNOTIFY_LBUTTONUP, m_nSelection);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
// Sample Map
//

BEGIN_MESSAGE_MAP(CSampleMapDlg, CDialog)
	ON_MESSAGE(WM_MOD_KBDNOTIFY,	OnKeyboardNotify)
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_CHECK1,			OnUpdateSamples)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnUpdateKeyboard)
END_MESSAGE_MAP()

void CSampleMapDlg::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleMapDlg)
	DDX_Control(pDX, IDC_KEYBOARD1,		m_Keyboard);
	DDX_Control(pDX, IDC_COMBO1,		m_CbnSample);
	DDX_Control(pDX, IDC_SLIDER1,		m_SbOctave);
	//}}AFX_DATA_MAP
}


BOOL CSampleMapDlg::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		for (UINT i=0; i<NOTE_MAX; i++)
		{
			KeyboardMap[i] = pIns->Keyboard[i];
		}
	}
	m_Keyboard.Init(m_hWnd, 3, TRUE);
	m_SbOctave.SetRange(0, 7);
	m_SbOctave.SetPos(4);
	OnUpdateSamples();
	OnUpdateOctave();
	return TRUE;
}


void CSampleMapDlg::OnHScroll(UINT nCode, UINT nPos, CScrollBar *pBar)
//--------------------------------------------------------------------
{
	CDialog::OnHScroll(nCode, nPos, pBar);
	OnUpdateKeyboard();
	OnUpdateOctave();
}


void CSampleMapDlg::OnUpdateSamples()
//-----------------------------------
{
	UINT nOldPos = 0;
	UINT nNewPos = 0;
	bool showAll;
	
	if ((m_nInstrument >= MAX_INSTRUMENTS)) return;
	if (m_CbnSample.GetCount() > 0)
	{
		nOldPos = m_CbnSample.GetItemData(m_CbnSample.GetCurSel());
	}
	m_CbnSample.ResetContent();
	showAll = (IsDlgButtonChecked(IDC_CHECK1) != FALSE);
	
	UINT nInsertPos;
	nInsertPos = m_CbnSample.AddString("0: No sample");
	m_CbnSample.SetItemData(nInsertPos, 0);

	for (SAMPLEINDEX i = 1; i <= sndFile.GetNumSamples(); i++)
	{
		bool isUsed = showAll;

		if (!isUsed)
		{
			for (auto smp : KeyboardMap)
			{
				if (smp == i)
				{
					isUsed = true;
					break;
				}
			}
		}
		if (isUsed)
		{
			CString sampleName;
			sampleName.Format("%d: %s", i, sndFile.GetSampleName(i));
			nInsertPos = m_CbnSample.AddString(sampleName);
			
			m_CbnSample.SetItemData(nInsertPos, i);
			if (i == nOldPos) nNewPos = nInsertPos;
		}
	}
	m_CbnSample.SetCurSel(nNewPos);
	OnUpdateKeyboard();
}


void CSampleMapDlg::OnUpdateOctave()
//----------------------------------
{
	TCHAR s[64];

	UINT nBaseOctave = m_SbOctave.GetPos() & 7;
	wsprintf(s, _T("Octaves %u-%u"), nBaseOctave, nBaseOctave+2);
	SetDlgItemText(IDC_TEXT1, s);
}



void CSampleMapDlg::OnUpdateKeyboard()
//------------------------------------
{
	SAMPLEINDEX nSample = static_cast<SAMPLEINDEX>(m_CbnSample.GetItemData(m_CbnSample.GetCurSel()));
	UINT nBaseOctave = m_SbOctave.GetPos() & 7;
	BOOL bRedraw = FALSE;
	for (UINT iNote=0; iNote<3*12; iNote++)
	{
		uint8 nOld = m_Keyboard.GetFlags(iNote);
		SAMPLEINDEX oldSmp = m_Keyboard.GetSample(iNote);
		UINT ndx = nBaseOctave*12+iNote;
		uint8 nNew = CKeyboardControl::KEYFLAG_NORMAL;
		if(KeyboardMap[ndx] == nSample) nNew = CKeyboardControl::KEYFLAG_REDDOT;
		else if(KeyboardMap[ndx] != 0) nNew = CKeyboardControl::KEYFLAG_BRIGHTDOT;
		if (nNew != nOld || oldSmp != KeyboardMap[ndx])
		{
			m_Keyboard.SetFlags(iNote, nNew);
			m_Keyboard.SetSample(iNote, KeyboardMap[ndx]);
			bRedraw = TRUE;
		}
	}
	if (bRedraw) m_Keyboard.InvalidateRect(NULL, FALSE);
}


LRESULT CSampleMapDlg::OnKeyboardNotify(WPARAM wParam, LPARAM lParam)
//-------------------------------------------------------------------
{
	CHAR s[32] = "--";

	if ((lParam >= 0) && (lParam < 3*12))
	{
		SAMPLEINDEX nSample = static_cast<SAMPLEINDEX>(m_CbnSample.GetItemData(m_CbnSample.GetCurSel()));
		uint32 nBaseOctave = m_SbOctave.GetPos() & 7;
		
		const std::string temp = sndFile.GetNoteName(static_cast<ModCommand::NOTE>(lParam + 1 + 12 * nBaseOctave), m_nInstrument).c_str();
		if(temp.size() >= CountOf(s))
			wsprintf(s, "%s", "...");
		else
			wsprintf(s, "%s", temp.c_str());

		ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
		if ((wParam == KBDNOTIFY_LBUTTONDOWN) && (nSample < MAX_SAMPLES) && (pIns))
		{
			uint32 iNote = static_cast<uint32>(nBaseOctave * 12 + lParam);

			if(mouseAction == mouseUnknown)
			{
				// Mouse down -> decide if we are going to set or remove notes
				mouseAction = mouseSet;
				if(KeyboardMap[iNote] == nSample)
				{
					 mouseAction = (KeyboardMap[iNote] == pIns->Keyboard[iNote]) ? mouseZero : mouseUnset;
				}
			}

			switch(mouseAction)
			{
			case mouseSet:
				KeyboardMap[iNote] = nSample;
				break;
			case mouseUnset:
				KeyboardMap[iNote] = pIns->Keyboard[iNote];
				break;
			case mouseZero:
				if(KeyboardMap[iNote] == nSample)
				{
					KeyboardMap[iNote] = 0;
				}
				break;
			}
			OnUpdateKeyboard();
		}
	}
	if(wParam == KBDNOTIFY_LBUTTONUP)
	{
		mouseAction = mouseUnknown;
	}
	SetDlgItemText(IDC_TEXT2, s);
	return 0;
}


void CSampleMapDlg::OnOK()
//------------------------
{
	ModInstrument *pIns = sndFile.Instruments[m_nInstrument];
	if (pIns)
	{
		BOOL bModified = FALSE;
		for (UINT i=0; i<NOTE_MAX; i++)
		{
			if (KeyboardMap[i] != pIns->Keyboard[i])
			{
				pIns->Keyboard[i] = KeyboardMap[i];
				bModified = TRUE;
			}
		}
		if (bModified)
		{
			CDialog::OnOK();
			return;
		}
	}
	CDialog::OnCancel();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Edit history dialog

BEGIN_MESSAGE_MAP(CEditHistoryDlg, CDialog)
	ON_COMMAND(IDC_BTN_CLEAR,	OnClearHistory)
END_MESSAGE_MAP()


BOOL CEditHistoryDlg::OnInitDialog()
//----------------------------------
{
	CDialog::OnInitDialog();

	if(m_pModDoc == nullptr)
		return TRUE;

	CString s;
	uint64 totalTime = 0;
	const size_t num = m_pModDoc->GetrSoundFile().GetFileHistory().size();
	
	for(size_t n = 0; n < num; n++)
	{
		const FileHistory *hist = &(m_pModDoc->GetrSoundFile().GetFileHistory().at(n));
		totalTime += hist->openTime;

		// Date
		TCHAR szDate[32];
		if(hist->loadDate.tm_mday != 0)
			_tcsftime(szDate, CountOf(szDate), _T("%d %b %Y, %H:%M:%S"), &hist->loadDate);
		else
			_tcscpy(szDate, _T("<unknown date>"));
		// Time + stuff
		uint32 duration = (uint32)((double)(hist->openTime) / HISTORY_TIMER_PRECISION);
		s.AppendFormat(_T("Loaded %s, open for %luh %02lum %02lus\r\n"),
			szDate, duration / 3600, (duration / 60) % 60, duration % 60);
	}
	if(num == 0)
	{
		s = _T("No information available about the previous edit history of this module.");
	}
	SetDlgItemText(IDC_EDIT_HISTORY, s);

	// Total edit time
	s = "";
	if(totalTime)
	{
		totalTime = (uint64)((double)(totalTime) / HISTORY_TIMER_PRECISION);

		s.Format(_T("Total edit time: %lluh %02llum %02llus (%u session%s)"), totalTime / 3600, (totalTime / 60) % 60, totalTime % 60, num, (num != 1) ? _T("s") : _T(""));
		SetDlgItemText(IDC_TOTAL_EDIT_TIME, s);
		// Window title
		s.Format(_T("Edit history for %s"), m_pModDoc->GetTitle());
		SetWindowText(s);
	}
	// Enable or disable Clear button
	GetDlgItem(IDC_BTN_CLEAR)->EnableWindow((m_pModDoc->GetrSoundFile().GetFileHistory().empty()) ? FALSE : TRUE);

	return TRUE;

}


void CEditHistoryDlg::OnClearHistory()
//------------------------------------
{
	if(m_pModDoc != nullptr && !m_pModDoc->GetrSoundFile().GetFileHistory().empty())
	{
		m_pModDoc->GetrSoundFile().GetFileHistory().clear();
		m_pModDoc->SetModified();
		OnInitDialog();
	}
}


void CEditHistoryDlg::OnOK()
//--------------------------
{
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////
// Generic input dialog

void CInputDlg::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	if(m_minValueInt == m_maxValueInt && m_minValueDbl == m_maxValueDbl)
	{
		// Only need this for freeform text
		DDX_Control(pDX, IDC_EDIT1, m_edit);
	}
	DDX_Control(pDX, IDC_SPIN1, m_spin);
}


BOOL CInputDlg::OnInitDialog()
//----------------------------
{
	CDialog::OnInitDialog();
	SetDlgItemText(IDC_PROMPT, m_description);

	// Get all current control sizes and positions
	CRect windowRect, labelRect, inputRect, okRect, cancelRect;
	GetWindowRect(windowRect);
	GetDlgItem(IDC_PROMPT)->GetWindowRect(labelRect);
	GetDlgItem(IDC_EDIT1)->GetWindowRect(inputRect);
	GetDlgItem(IDOK)->GetWindowRect(okRect);
	GetDlgItem(IDCANCEL)->GetWindowRect(cancelRect);
	ScreenToClient(labelRect);
	ScreenToClient(inputRect);
	ScreenToClient(okRect);
	ScreenToClient(cancelRect);

	// Find out how big our label shall be
	HDC dc = ::GetDC(m_hWnd);
	CRect textRect(0,0,0,0);
	DrawText(dc, m_description, m_description.GetLength(), textRect, DT_CALCRECT);
	LPtoDP(dc, &textRect.BottomRight(), 1);
	::ReleaseDC(m_hWnd, dc);
	if(textRect.right < 320) textRect.right = 320;
	const int windowWidth = windowRect.Width() - labelRect.Width() + textRect.right;
	const int windowHeight = windowRect.Height() - labelRect.Height() + textRect.bottom;

	// Resize and move all controls
	GetDlgItem(IDC_PROMPT)->SetWindowPos(nullptr, 0, 0, textRect.right, textRect.bottom, SWP_NOMOVE | SWP_NOZORDER);
	GetDlgItem(IDC_EDIT1)->SetWindowPos(nullptr, inputRect.left, labelRect.top + textRect.bottom + (inputRect.top - labelRect.bottom), textRect.right, inputRect.Height(), SWP_NOZORDER);
	GetDlgItem(IDOK)->SetWindowPos(nullptr, windowWidth - (windowRect.Width() - okRect.left), windowHeight - (windowRect.Height() - okRect.top), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	GetDlgItem(IDCANCEL)->SetWindowPos(nullptr, windowWidth - (windowRect.Width() - cancelRect.left), windowHeight - (windowRect.Height() - cancelRect.top), 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(nullptr, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);

	if(m_minValueInt != m_maxValueInt)
	{
		// Numeric (int)
		m_spin.SetRange32(m_minValueInt, m_maxValueInt);
		m_edit.SubclassDlgItem(IDC_EDIT1, this);
		m_edit.ModifyStyle(0, ES_NUMBER);
		m_edit.AllowNegative(m_minValueInt < 0);
		m_edit.AllowFractions(false);
		SetDlgItemInt(IDC_EDIT1, resultAsInt);
		m_spin.SetBuddy(&m_edit);
	} else if(m_minValueDbl != m_maxValueDbl)
	{
		// Numeric (double)
		m_spin.SetRange32(static_cast<int32>(m_minValueDbl), static_cast<int32>(m_maxValueDbl));
		m_edit.SubclassDlgItem(IDC_EDIT1, this);
		m_edit.ModifyStyle(0, ES_NUMBER);
		m_edit.AllowNegative(m_minValueDbl < 0);
		m_edit.AllowFractions(true);
		m_edit.SetDecimalValue(resultAsDouble);
		m_spin.SetBuddy(&m_edit);
	} else
	{
		// Text
		m_spin.ShowWindow(SW_HIDE);
		SetDlgItemText(IDC_EDIT1, resultAsString);
	}

	return TRUE;
}


void CInputDlg::OnOK()
//--------------------
{
	CDialog::OnOK();
	GetDlgItemText(IDC_EDIT1, resultAsString);
	resultAsInt = static_cast<int32>(GetDlgItemInt(IDC_EDIT1));
	Limit(resultAsInt, m_minValueInt, m_maxValueInt);
	m_edit.GetDecimalValue(resultAsDouble);
	Limit(resultAsDouble, m_minValueDbl, m_maxValueDbl);
}


///////////////////////////////////////////////////////////////////////////////////////
// Messagebox with 'don't show again'-option.

//===================================
class CMsgBoxHidable : public CDialog
//===================================
{
public:
	CMsgBoxHidable(LPCTSTR strMsg, bool checkStatus = true, CWnd* pParent = NULL);
	enum { IDD = IDD_MSGBOX_HIDABLE };

	int m_nCheckStatus;
	LPCTSTR m_StrMsg;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support
	virtual BOOL OnInitDialog();
};


struct MsgBoxHidableMessage
//=========================
{
	LPCTSTR strMsg;
	uint32 nMask;
	bool bDefaultDontShowAgainStatus; // true for don't show again, false for show again.
};

const MsgBoxHidableMessage HidableMessages[] =
{
	{_T("Note: First two bytes of oneshot samples are silenced for ProTracker compatibility."), 1, true},
	{_T("Hint: To create IT-files without MPT-specific extensions included, try compatibility export from File-menu."), 1 << 1, true},
	{_T("Press OK to apply signed/unsigned conversion\n (note: this often significantly increases volume level)"), 1 << 2, false},
	{_T("Hint: To create XM-files without MPT-specific extensions included, try compatibility export from File-menu."), 1 << 3, true},
	{_T("Warning: The exported file will not contain any of MPT's file format hacks."), 1 << 4, true},
};

STATIC_ASSERT(CountOf(HidableMessages) == enMsgBoxHidableMessage_count);

// Messagebox with 'don't show this again'-checkbox. Uses parameter 'enMsg'
// to get the needed information from message array, and updates the variable that
// controls the show/don't show-flags.
void MsgBoxHidable(enMsgBoxHidableMessage enMsg)
//----------------------------------------------
{
	// Check whether the message should be shown.
	if((TrackerSettings::Instance().gnMsgBoxVisiblityFlags & HidableMessages[enMsg].nMask) == 0)
		return;

	const LPCTSTR strMsg = HidableMessages[enMsg].strMsg;
	const uint32 mask = HidableMessages[enMsg].nMask;
	const bool defaulCheckStatus = HidableMessages[enMsg].bDefaultDontShowAgainStatus;

	// Show dialog.
	CMsgBoxHidable dlg(strMsg, defaulCheckStatus);
	dlg.DoModal();

	// Update visibility flags.
	if(dlg.m_nCheckStatus == BST_CHECKED)
		TrackerSettings::Instance().gnMsgBoxVisiblityFlags &= ~mask;
	else
		TrackerSettings::Instance().gnMsgBoxVisiblityFlags |= mask;
}


CMsgBoxHidable::CMsgBoxHidable(LPCTSTR strMsg, bool checkStatus, CWnd* pParent)
	:	CDialog(CMsgBoxHidable::IDD, pParent),
		m_StrMsg(strMsg),
		m_nCheckStatus((checkStatus) ? BST_CHECKED : BST_UNCHECKED)
//----------------------------------------------------------------------------
{}

BOOL CMsgBoxHidable::OnInitDialog()
//----------------------------------
{
	CDialog::OnInitDialog();
	SetDlgItemText(IDC_MESSAGETEXT, m_StrMsg);
	SetWindowText(AfxGetAppName());
	return TRUE;
}

void CMsgBoxHidable::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_DONTSHOWAGAIN, m_nCheckStatus);
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


void AppendNotesToControl(CComboBox& combobox, ModCommand::NOTE noteStart, ModCommand::NOTE noteEnd)
//--------------------------------------------------------------------------------------------------
{
	const ModCommand::NOTE upperLimit = std::min(ModCommand::NOTE(NOTE_MAX), noteEnd);
	for(ModCommand::NOTE note = noteStart; note <= upperLimit; note++)
		combobox.SetItemData(combobox.AddString(CSoundFile::GetNoteName(note, CSoundFile::GetDefaultNoteNames()).c_str()), note);
}


void AppendNotesToControlEx(CComboBox& combobox, const CSoundFile &sndFile, INSTRUMENTINDEX nInstr, ModCommand::NOTE noteStart, ModCommand::NOTE noteEnd)
//-------------------------------------------------------------------------------------------------------------------------------------------------------
{
	bool addSpecial = noteStart == noteEnd;
	if(noteStart == noteEnd)
	{
		noteStart = sndFile.GetModSpecifications().noteMin;
		noteEnd = sndFile.GetModSpecifications().noteMax;
	}
	for(ModCommand::NOTE note = noteStart; note <= noteEnd; note++)
	{
		combobox.SetItemData(combobox.AddString(sndFile.GetNoteName(note, nInstr).c_str()), note);
	}
	if(addSpecial)
	{
		for(ModCommand::NOTE note = NOTE_MIN_SPECIAL - 1; note++ < NOTE_MAX_SPECIAL;)
		{
			if(sndFile.GetModSpecifications().HasNote(note))
				combobox.SetItemData(combobox.AddString(szSpecialNoteNamesMPT[note - NOTE_MIN_SPECIAL]), note);
		}
	}
}


OPENMPT_NAMESPACE_END
