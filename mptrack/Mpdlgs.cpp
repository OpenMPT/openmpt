#include "stdafx.h"
#include "mptrack.h"
#include "sndfile.h"
#include "mainfrm.h"
#include "dlsbank.h"
#include "mpdlgs.h"
#include "moptions.h"
#include "moddoc.h"
#include "snddev.h"

#pragma warning(disable:4244)

LPCSTR szCPUNames[8] =
{
	"133MHz",
	"166MHz",
	"200MHz",
	"233MHz",
	"266MHz",
	"300MHz",
	"333MHz",
	"400+MHz"
};


UINT nCPUMix[8] =
{
	16,
	24,
	32,
	40,
	64,
	96,
	128,
	MAX_CHANNELS
};


LPCSTR gszChnCfgNames[3] =
{
	"Mono",
	"Stereo",
	"Quad"
};



BEGIN_MESSAGE_MAP(COptionsSoundcard, CPropertyPage)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_CHECK1,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK3,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,	OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnDeviceChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5, OnSettingsChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO2, OnSettingsChanged)
END_MESSAGE_MAP()


UINT nMixingRates[NUMMIXRATE] =
{
	16000,
	19800,
	20000,
	22050,
	24000,
	32000,
	33075,
	37800,
	40000,
	44100,
	48000,
	64000,
	88200,
	96000
};


void COptionsSoundcard::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_COMBO1,		m_CbnDevice);
	DDX_Control(pDX, IDC_COMBO2,		m_CbnBufferLength);
	DDX_Control(pDX, IDC_COMBO3,		m_CbnMixingFreq);
	DDX_Control(pDX, IDC_COMBO4,		m_CbnPolyphony);
	DDX_Control(pDX, IDC_COMBO5,		m_CbnQuality);
	DDX_Control(pDX, IDC_SLIDER1,		m_SliderStereoSep);
	DDX_Control(pDX, IDC_SLIDER_PREAMP,	m_SliderPreAmp);
	//}}AFX_DATA_MAP
}


BOOL COptionsSoundcard::OnInitDialog()
//------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	BOOL bAsio = FALSE;
	CHAR s[128];
	
	CPropertyPage::OnInitDialog();
	if (m_dwSoundSetup & SOUNDSETUP_STREVERSE) CheckDlgButton(IDC_CHECK1, MF_CHECKED);
	if (m_dwSoundSetup & SOUNDSETUP_SOFTPANNING) CheckDlgButton(IDC_CHECK2, MF_CHECKED);
	if (m_dwSoundSetup & SOUNDSETUP_ENABLEMMX) CheckDlgButton(IDC_CHECK3, MF_CHECKED);
	if (m_dwSoundSetup & SOUNDSETUP_SECONDARY) CheckDlgButton(IDC_CHECK4, MF_CHECKED);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK3), (CSoundFile::GetSysInfo() & SYSMIX_ENABLEMMX) ? TRUE : FALSE);
	// Sampling Rate
	{
		UINT n = 1;
		for (UINT i=0; i<NUMMIXRATE; i++)
		{
			wsprintf(s, "%u Hz", nMixingRates[i]);
			m_CbnMixingFreq.AddString(s);
			if (m_dwRate == nMixingRates[i]) n = i;
		}
		m_CbnMixingFreq.SetCurSel(n);
	}
	// Max Mixing Channels
	{
		for (UINT n=0; n<8; n++)
		{
			wsprintf(s, "%d (%s)", nCPUMix[n], szCPUNames[n]);
			m_CbnPolyphony.AddString(s);
			if (CSoundFile::m_nMaxMixChannels == nCPUMix[n]) m_CbnPolyphony.SetCurSel(n);
		}
	}
	// Sound Buffer Length
	{
		wsprintf(s, "%d ms", m_nBufferLength);
		m_CbnBufferLength.SetWindowText(s);
// -> CODE#0006
// -> DESC="misc quantity changes"
		m_CbnBufferLength.AddString("10 ms");
		m_CbnBufferLength.AddString("20 ms");
// -! BEHAVIOUR_CHANGE#0006
		m_CbnBufferLength.AddString("30 ms");
		m_CbnBufferLength.AddString("50 ms");
		m_CbnBufferLength.AddString("75 ms");
		m_CbnBufferLength.AddString("100 ms");
		m_CbnBufferLength.AddString("125 ms");
		m_CbnBufferLength.AddString("150 ms");
// -> CODE#0006
// -> DESC="misc quantity changes"
		m_CbnBufferLength.AddString("200 ms");
// -! BEHAVIOUR_CHANGE#0006
	}
	// Stereo Separation
	{
		m_SliderStereoSep.SetRange(0, 4);
		m_SliderStereoSep.SetPos(2);
		for (int n=0; n<=4; n++)
		{
			if ((int)CSoundFile::m_nStereoSeparation <= (int)(32 << n))
			{
				m_SliderStereoSep.SetPos(n);
				break;
			}
		}
		UpdateStereoSep();
	}
	// Pre-Amplification
	{
		int n = (CMainFrame::m_nPreAmp - 64) / 8;
		if ((n < 0) || (n > 40)) n = 16;
		m_SliderPreAmp.SetTicFreq(5);
		m_SliderPreAmp.SetRange(0, 40);
		m_SliderPreAmp.SetPos(40 - n);
	}
	// Sound Device
	{
		if (pMainFrm)
		{
			m_CbnDevice.SetImageList(pMainFrm->GetImageList());
		}
		COMBOBOXEXITEM cbi;
		UINT iItem = 0;
		for (UINT nDevType=0; nDevType<SNDDEV_NUM_DEVTYPES; nDevType++)
		{
			UINT nDev = 0;

			while (EnumerateSoundDevices(nDevType, nDev, s, sizeof(s)))
			{
				cbi.mask = CBEIF_IMAGE | CBEIF_LPARAM | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
				cbi.iItem = iItem;
				cbi.cchTextMax = 0;
				switch(nDevType)
				{
				case SNDDEV_DSOUND:
					cbi.iImage = IMAGE_DIRECTX;
					break;
				case SNDDEV_ASIO:
					bAsio = TRUE;
					cbi.iImage = IMAGE_ASIO;
					break;
				default:
					cbi.iImage = IMAGE_WAVEOUT;
				}
				cbi.iSelectedImage = cbi.iImage;
				cbi.iOverlay = cbi.iImage;
				cbi.iIndent = 0;
				cbi.lParam = (nDevType<<8)|(nDev&0xff);
				cbi.pszText = s;
				int pos = m_CbnDevice.InsertItem(&cbi);
				if (cbi.lParam == (LONG)m_nSoundDevice) m_CbnDevice.SetCurSel(pos);
				iItem++;
				nDev++;
			}
		}
		GetDlgItem(IDC_CHECK4)->EnableWindow(((m_nSoundDevice>>8)==SNDDEV_DSOUND) ? TRUE : FALSE);
	}
	// Sample Format
	{
		UINT n = 0;
		for (UINT i=0; i<3*3; i++)
		{
			UINT j = 3*3-1-i;
			UINT nBits = 8 << (j % 3);
			UINT nChannels = 1 << (j/3);
			if ((((nChannels <= 2) && (nBits <= 16)) || (theApp.IsWaveExEnabled()) || (bAsio))
			 && ((nBits >= 16) || (nChannels <= 2)))
			{
				wsprintf(s, "%s, %d Bit", gszChnCfgNames[j/3], nBits);
				UINT ndx = m_CbnQuality.AddString(s);
				m_CbnQuality.SetItemData( ndx, (nChannels << 8) | nBits );
				if ((nBits == m_nBitsPerSample) && (nChannels == m_nChannels)) n = ndx;
			}
		}
		m_CbnQuality.SetCurSel(n);
	}
	return TRUE;
}


void COptionsSoundcard::OnHScroll(UINT n, UINT pos, CScrollBar *p)
//----------------------------------------------------------------
{
	CPropertyPage::OnHScroll(n, pos, p);
	UpdateStereoSep();
}


void COptionsSoundcard::UpdateStereoSep()
//---------------------------------------
{
	CHAR s[64];
	CSoundFile::m_nStereoSeparation = 32 << m_SliderStereoSep.GetPos();
	wsprintf(s, "%d%%", (CSoundFile::m_nStereoSeparation * 100) / 128);
	SetDlgItemText(IDC_TEXT1, s);
	
}


void COptionsSoundcard::OnVScroll(UINT n, UINT pos, CScrollBar *p)
//----------------------------------------------------------------
{
	CPropertyPage::OnVScroll(n, pos, p);
	// PreAmp
	{
		int n = 40 - m_SliderPreAmp.GetPos();
		if ((n >= 0) && (n <= 40)) // approximately +/- 10dB
		{
			CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
			if (pMainFrm) pMainFrm->SetPreAmp(64 + (n * 8));
		}
	}
}


void COptionsSoundcard::OnDeviceChanged()
//---------------------------------------
{
	int n = m_CbnDevice.GetCurSel();
	if (n >= 0)
	{
		int dev = m_CbnDevice.GetItemData(n);
		GetDlgItem(IDC_CHECK4)->EnableWindow(((dev>>8)==SNDDEV_DSOUND) ? TRUE : FALSE);
		OnSettingsChanged();
	}
}


BOOL COptionsSoundcard::OnSetActive()
//-----------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_SOUNDCARD;
	return CPropertyPage::OnSetActive();
}


void COptionsSoundcard::OnOK()
//----------------------------
{
	m_dwSoundSetup &= ~(SOUNDSETUP_ENABLEMMX | SOUNDSETUP_SECONDARY | SOUNDSETUP_STREVERSE | SOUNDSETUP_SOFTPANNING);
	if (IsDlgButtonChecked(IDC_CHECK1)) m_dwSoundSetup |= SOUNDSETUP_STREVERSE;
	if (IsDlgButtonChecked(IDC_CHECK2)) m_dwSoundSetup |= SOUNDSETUP_SOFTPANNING;
	if (IsDlgButtonChecked(IDC_CHECK3)) m_dwSoundSetup |= SOUNDSETUP_ENABLEMMX;
	if (IsDlgButtonChecked(IDC_CHECK4)) m_dwSoundSetup |= SOUNDSETUP_SECONDARY;
	// Mixing Freq
	{
		UINT n = m_CbnMixingFreq.GetCurSel();
		if (n >= NUMMIXRATE) n = 3;
		m_dwRate = nMixingRates[n];
	}
	// Quality
	{
		UINT n = m_CbnQuality.GetItemData( m_CbnQuality.GetCurSel() );
		m_nChannels = n >> 8;
		m_nBitsPerSample = n & 0xFF;
		if ((m_nChannels != 1) && (m_nChannels != 4)) m_nChannels = 2;
		if ((m_nBitsPerSample != 8) && (m_nBitsPerSample != 32)) m_nBitsPerSample = 16;
	}
	// Polyphony
	{
		int nmmx = m_CbnPolyphony.GetCurSel();
		if ((nmmx >= 0) && (nmmx < sizeof(nCPUMix)/sizeof(nCPUMix[0]))) CSoundFile::m_nMaxMixChannels = nCPUMix[nmmx];
	}
	// Sound Device
	{
		int n = m_CbnDevice.GetCurSel();
		if (n >= 0) m_nSoundDevice = m_CbnDevice.GetItemData(n);
	}
	// Buffer Length
	{
		CHAR s[32];
		m_CbnBufferLength.GetWindowText(s, sizeof(s));
		m_nBufferLength = atoi(s);
		if ((m_nBufferLength < 10) || (m_nBufferLength > 200)) m_nBufferLength = 100;
	}
	// Soft Panning
	if (m_dwSoundSetup & SOUNDSETUP_SOFTPANNING)
		CSoundFile::gdwSoundSetup |= SNDMIX_SOFTPANNING;
	else
		CSoundFile::gdwSoundSetup &= ~SNDMIX_SOFTPANNING;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->SetupSoundCard(m_dwSoundSetup, m_dwRate, m_nBitsPerSample, m_nChannels, m_nBufferLength, m_nSoundDevice);
	CPropertyPage::OnOK();
}


//////////////////////////////////////////////////////////
// COptionsPlayer

BEGIN_MESSAGE_MAP(COptionsPlayer, CPropertyPage)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO1,OnResamplerChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,OnSettingsChanged)
	//rewbs.resamplerConf
	ON_CBN_SELCHANGE(IDC_WFIRTYPE,OnWFIRTypeChanged)
	ON_EN_UPDATE(IDC_WFIRCUTOFF,OnSettingsChanged)
	ON_EN_UPDATE(IDC_RAMPING,	OnSettingsChanged)
	//end rewbs.resamplerConf
	ON_COMMAND(IDC_CHECK1,		OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,		OnSettingsChanged)
	ON_COMMAND(IDC_CHECK3,		OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,		OnSettingsChanged)
	ON_COMMAND(IDC_CHECK5,		OnSettingsChanged)
	ON_COMMAND(IDC_CHECK6,		OnSettingsChanged)
	ON_COMMAND(IDC_CHECK7,		OnSettingsChanged)
END_MESSAGE_MAP()


void COptionsPlayer::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsPlayer)
	DDX_Control(pDX, IDC_COMBO1,		m_CbnResampling);
	//rewbs.resamplerConf
	DDX_Control(pDX, IDC_WFIRTYPE,		m_CbnWFIRType);
	DDX_Control(pDX, IDC_WFIRCUTOFF,	m_CEditWFIRCutoff);
	DDX_Control(pDX, IDC_RAMPING,		m_CEditRamping);
	//end rewbs.resamplerConf
	DDX_Control(pDX, IDC_COMBO2,		m_CbnReverbPreset);
	DDX_Control(pDX, IDC_SLIDER1,		m_SbXBassDepth);
	DDX_Control(pDX, IDC_SLIDER2,		m_SbXBassRange);
	DDX_Control(pDX, IDC_SLIDER3,		m_SbReverbDepth);
	DDX_Control(pDX, IDC_SLIDER5,		m_SbSurroundDepth);
	DDX_Control(pDX, IDC_SLIDER6,		m_SbSurroundDelay);
	//}}AFX_DATA_MAP
}


BOOL COptionsPlayer::OnInitDialog()
//---------------------------------
{
	DWORD dwQuality;
	
	CPropertyPage::OnInitDialog();
	dwQuality = CMainFrame::m_dwQuality;
	// Resampling type
	{
		m_CbnResampling.AddString("No Resampling");
		m_CbnResampling.AddString("Linear");
		m_CbnResampling.AddString("Cubic spline");
		//rewbs.resamplerConf
		m_CbnResampling.AddString("Polyphase");
		m_CbnResampling.AddString("XMMS-Modplug");
		//end rewbs.resamplerConf
		m_CbnResampling.SetCurSel(CMainFrame::m_nSrcMode);
	}
	// Effects
	if (dwQuality & QUALITY_MEGABASS) CheckDlgButton(IDC_CHECK1, MF_CHECKED);
	if (dwQuality & QUALITY_AGC) CheckDlgButton(IDC_CHECK2, MF_CHECKED);
	if (dwQuality & QUALITY_SURROUND) CheckDlgButton(IDC_CHECK4, MF_CHECKED);
	if (dwQuality & QUALITY_NOISEREDUCTION) CheckDlgButton(IDC_CHECK5, MF_CHECKED);
	if (CSoundFile::GetSysInfo() & SYSMIX_SLOWCPU)
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK3), FALSE);
	else if (dwQuality & QUALITY_EQ) CheckDlgButton(IDC_CHECK3, MF_CHECKED);
	if (CSoundFile::gdwSoundSetup & SNDMIX_EMULATE_MIX_BUGS) CheckDlgButton(IDC_CHECK7, MF_CHECKED); //rewbs.emulateMixBugs

	// Bass Expansion
	m_SbXBassDepth.SetRange(0,4);
	m_SbXBassDepth.SetPos(8-CSoundFile::m_nXBassDepth);
	m_SbXBassRange.SetRange(0,4);
	m_SbXBassRange.SetPos(4 - (CSoundFile::m_nXBassRange - 1) / 5);
	// Reverb
	m_SbReverbDepth.SetRange(1, 16);
	m_SbReverbDepth.SetPos(CSoundFile::m_nReverbDepth);
	UINT nSel = 0;
	for (UINT iRvb=0; iRvb<NUM_REVERBTYPES; iRvb++)
	{
		LPCSTR pszName = GetReverbPresetName(iRvb);
		if (pszName)
		{
			UINT n = m_CbnReverbPreset.AddString(pszName);
			m_CbnReverbPreset.SetItemData(n, iRvb);
			if (iRvb == CSoundFile::gnReverbType) nSel = n;
		}
	}
	m_CbnReverbPreset.SetCurSel(nSel);
	if (!(CSoundFile::gdwSysInfo & SYSMIX_ENABLEMMX))
	{
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK6), FALSE);
		m_SbReverbDepth.EnableWindow(FALSE);
		m_CbnReverbPreset.EnableWindow(FALSE);
	} else
	{
		if (dwQuality & QUALITY_REVERB) CheckDlgButton(IDC_CHECK6, MF_CHECKED);
	}
	// Surround
	{
		UINT n = CSoundFile::m_nProLogicDepth;
		if (n < 1) n = 1;
		if (n > 16) n = 16;
		m_SbSurroundDepth.SetRange(1, 16);
		m_SbSurroundDepth.SetPos(n);
		m_SbSurroundDelay.SetRange(0, 8);
		m_SbSurroundDelay.SetPos((CSoundFile::m_nProLogicDelay-5)/5);
	}
	//rewbs.resamplerConf
	OnResamplerChanged();
	char s[20] = "";
	ltoa(CMainFrame::glVolumeRampSamples,s, 10);
	m_CEditRamping.SetWindowText(s);
	//end rewbs.resamplerConf
	return TRUE;
}


BOOL COptionsPlayer::OnSetActive()
//--------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_PLAYER;
	return CPropertyPage::OnSetActive();
}


void COptionsPlayer::OnHScroll(UINT nSBCode, UINT, CScrollBar *psb)
//-----------------------------------------------------------------
{
	if (nSBCode == SB_ENDSCROLL) return;
	UINT n = m_SbReverbDepth.GetPos();
	if ((psb) && (psb->m_hWnd == m_SbReverbDepth.m_hWnd))
	{
		if (n != CSoundFile::m_nReverbDepth)
		{
			if ((n) && (n <= 16)) CSoundFile::m_nReverbDepth = n;
		}
	} else
	{
		OnSettingsChanged();
	}
}

//rewbs.resamplerConf
void COptionsPlayer::OnWFIRTypeChanged()
{
	CMainFrame::gbWFIRType = m_CbnWFIRType.GetCurSel();
	OnSettingsChanged();
}

void COptionsPlayer::OnResamplerChanged()
{
	DWORD dwSrcMode = m_CbnResampling.GetCurSel();
	m_CbnWFIRType.ResetContent();

	char s[10] = "";
	switch (dwSrcMode)
	{
		case SRCMODE_POLYPHASE:
		m_CbnWFIRType.AddString("Kaiser 8 Tap");
		m_CbnWFIRType.SetCurSel(0);
		m_CbnWFIRType.EnableWindow(FALSE);
		m_CEditWFIRCutoff.EnableWindow(TRUE);
		wsprintf(s, "%d", static_cast<int>((CMainFrame::gdWFIRCutoff*100)));
		break;
	case SRCMODE_FIRFILTER:
		m_CbnWFIRType.AddString("Hann");
		m_CbnWFIRType.AddString("Hamming");
		m_CbnWFIRType.AddString("Blackman Exact");
		m_CbnWFIRType.AddString("Blackman 3 Tap 61");
		m_CbnWFIRType.AddString("Blackman 3 Tap 67");
		m_CbnWFIRType.AddString("Blackman 4 Tap 92");
		m_CbnWFIRType.AddString("Blackman 4 Tap 74");
		m_CbnWFIRType.AddString("Kaiser 4 Tap");
		m_CbnWFIRType.SetCurSel(CMainFrame::gbWFIRType);
		m_CbnWFIRType.EnableWindow(TRUE);
		m_CEditWFIRCutoff.EnableWindow(TRUE);
		wsprintf(s, "%d", static_cast<int>((CMainFrame::gdWFIRCutoff*100)));
		break;
	default: 
		m_CbnWFIRType.AddString("None");
		m_CEditWFIRCutoff.EnableWindow(FALSE);
		m_CbnWFIRType.EnableWindow(FALSE);
	}
	
	m_CEditWFIRCutoff.SetWindowText(s);
	OnSettingsChanged();
}

extern VOID SndMixInitializeTables();
//end rewbs.resamplerConf
void COptionsPlayer::OnOK()
//-------------------------
{
	DWORD dwQuality = 0;
	DWORD dwSrcMode = 0;

	if (IsDlgButtonChecked(IDC_CHECK1)) dwQuality |= QUALITY_MEGABASS;
	if (IsDlgButtonChecked(IDC_CHECK2)) dwQuality |= QUALITY_AGC;
	if (IsDlgButtonChecked(IDC_CHECK3)) dwQuality |= QUALITY_EQ;
	if (IsDlgButtonChecked(IDC_CHECK4)) dwQuality |= QUALITY_SURROUND;
	if (IsDlgButtonChecked(IDC_CHECK5)) dwQuality |= QUALITY_NOISEREDUCTION;
	if (IsDlgButtonChecked(IDC_CHECK6)) dwQuality |= QUALITY_REVERB;
	dwSrcMode = m_CbnResampling.GetCurSel();

	//rewbs.emulateMixBugs
	if (IsDlgButtonChecked(IDC_CHECK7)) {
		CSoundFile::gdwSoundSetup |= SNDMIX_EMULATE_MIX_BUGS;
	} else {
		CSoundFile::gdwSoundSetup &= ~SNDMIX_EMULATE_MIX_BUGS;
	}
	//end rewbs.emulateMixBugs

	// Bass Expansion
	{
		UINT nXBassDepth = 8-m_SbXBassDepth.GetPos();
		if (nXBassDepth < 4) nXBassDepth = 4;
		if (nXBassDepth > 8) nXBassDepth = 8;
		UINT nXBassRange = (4-m_SbXBassRange.GetPos()) * 5 + 1;
		if (nXBassRange < 5) nXBassRange = 5;
		if (nXBassRange > 21) nXBassRange = 21;
		CSoundFile::m_nXBassDepth = nXBassDepth;
		CSoundFile::m_nXBassRange = nXBassRange;
	}
	// Reverb
	{
		// Reverb depth is dynamically changed
		UINT nReverbType = m_CbnReverbPreset.GetItemData(m_CbnReverbPreset.GetCurSel());
		if (nReverbType < NUM_REVERBTYPES) CSoundFile::gnReverbType = nReverbType;
	}
	// Surround
	{
		UINT nProLogicDepth = m_SbSurroundDepth.GetPos();
		UINT nProLogicDelay = 5 + (m_SbSurroundDelay.GetPos() * 5);
		CSoundFile::m_nProLogicDepth = nProLogicDepth;
		CSoundFile::m_nProLogicDelay = nProLogicDelay;
	}
	// Notify CMainFrame
	CMainFrame *pParent = CMainFrame::GetMainFrame();
	//rewbs.resamplerConf
	CString s;
	m_CEditWFIRCutoff.GetWindowText(s);
	if (s != "")
		CMainFrame::gdWFIRCutoff = atoi(s)/100.0;
	//CMainFrame::gbWFIRType set in OnWFIRTypeChange
	m_CEditRamping.GetWindowText(s);
	CMainFrame::glVolumeRampSamples = atol(s);
	SndMixInitializeTables(); //regenerate resampling tables
	//end rewbs.resamplerConf
	if (pParent) pParent->SetupPlayer(dwQuality, dwSrcMode, TRUE);
	CPropertyPage::OnOK();
}


////////////////////////////////////////////////////////////////////////////////
//
// EQ Globals
//

enum {
	EQPRESET_FLAT=0,
	EQPRESET_JAZZ,
	EQPRESET_POP,
	EQPRESET_ROCK,
	EQPRESET_CONCERT,
	EQPRESET_CLEAR,
};


#define EQ_MAX_FREQS	5

const UINT gEqBandFreqs[EQ_MAX_FREQS*MAX_EQ_BANDS] =
{
	100, 125, 150, 200, 250,
	300, 350, 400, 450, 500,
	600, 700, 800, 900, 1000,
	1250, 1500, 1750, 2000, 2500,
	3000, 3500, 4000, 4500, 5000,
	6000, 7000, 8000, 9000, 10000
};


const EQPRESET gEQPresets[6] =
{
	{ "Flat",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },	// Flat
	{ "Jazz",	{16,16,24,20,20,14}, { 125, 300, 600, 1250, 4000, 8000 } },	// Jazz
	{ "Pop",	{24,16,16,21,16,26}, { 125, 300, 600, 1250, 4000, 8000 } },	// Pop
	{ "Rock",	{24,16,24,16,24,22}, { 125, 300, 600, 1250, 4000, 8000 } },	// Rock
	{ "Concert",{22,18,26,16,22,16}, { 125, 300, 600, 1250, 4000, 8000 } },	// Concert
	{ "Clear",	{20,16,16,22,24,26}, { 125, 300, 600, 1250, 4000, 8000 } }	// Clear
};


EQPRESET CEQSetupDlg::gUserPresets[4] =
{
	{ "User1",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },	// User1
	{ "User2",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },	// User2
	{ "User3",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },	// User3
	{ "User4",	{16,16,16,16,16,16}, { 150, 500, 1000, 2500, 5000, 10000 } }	// User4
};


#define ID_EQSLIDER_BASE	41000
#define ID_EQMENU_BASE		41100


////////////////////////////////////////////////////////////////////////////////
//
// CEQSavePresetDlg
//

//====================================
class CEQSavePresetDlg: public CDialog
//====================================
{
protected:
	PEQPRESET m_pEq;

public:
	CEQSavePresetDlg(PEQPRESET pEq, CWnd *parent=NULL):CDialog(IDD_SAVEPRESET, parent) { m_pEq = pEq; }
	BOOL OnInitDialog();
	VOID OnOK();
};


BOOL CEQSavePresetDlg::OnInitDialog()
//-----------------------------------
{
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (pCombo)
	{
		int ndx = 0;
		for (UINT i=0; i<4; i++)
		{
			int n = pCombo->AddString(CEQSetupDlg::gUserPresets[i].szName);
			pCombo->SetItemData( n, i);
			if (!lstrcmpi(CEQSetupDlg::gUserPresets[i].szName, m_pEq->szName)) ndx = n;
		}
		pCombo->SetCurSel(ndx);
	}
	SetDlgItemText(IDC_EDIT1, m_pEq->szName);
	return TRUE;
}


VOID CEQSavePresetDlg::OnOK()
//---------------------------
{
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (pCombo)
	{
		int n = pCombo->GetCurSel();
		if ((n < 0) || (n >= 4)) n = 0;
		GetDlgItemText(IDC_EDIT1, m_pEq->szName, sizeof(m_pEq->szName));
		m_pEq->szName[sizeof(m_pEq->szName)-1] = 0;
		CEQSetupDlg::gUserPresets[n] = *m_pEq;
	}
	CDialog::OnOK();
}


////////////////////////////////////////////////////////////////////////////////
//
// CEQSetupDlg
//

VOID CEQSetupDlg::LoadEQ(HKEY key, LPCSTR pszName, PEQPRESET pEqSettings)
//-----------------------------------------------------------------------
{
	DWORD dwType = REG_BINARY;
	DWORD dwSize = sizeof(EQPRESET);
	RegQueryValueEx(key, pszName, NULL, &dwType, (LPBYTE)pEqSettings, &dwSize);
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		if (pEqSettings->Gains[i] > 32) pEqSettings->Gains[i] = 16;
		if ((pEqSettings->Freqs[i] < 100) || (pEqSettings->Freqs[i] > 10000)) pEqSettings->Freqs[i] = gEQPresets[0].Freqs[i];
	}
	pEqSettings->szName[sizeof(pEqSettings->szName)-1] = 0;
}


VOID CEQSetupDlg::SaveEQ(HKEY key, LPCSTR pszName, PEQPRESET pEqSettings)
//-----------------------------------------------------------------------
{
	RegSetValueEx(key, pszName, NULL, REG_BINARY, (LPBYTE)pEqSettings, sizeof(EQPRESET));
}


VOID CEQSlider::Init(UINT nID, UINT n, CWnd *parent)
//--------------------------------------------------
{
	m_nSliderNo = n;
	m_pParent = parent;
	SubclassDlgItem(nID, parent);
}


BOOL CEQSlider::PreTranslateMessage(MSG *pMsg)
//--------------------------------------------
{
	if ((pMsg) && (pMsg->message == WM_RBUTTONDOWN) && (m_pParent))
	{
		m_x = LOWORD(pMsg->lParam);
		m_y = HIWORD(pMsg->lParam);
		m_pParent->PostMessage(WM_COMMAND, ID_EQSLIDER_BASE+m_nSliderNo, 0);
	}
	return CSliderCtrl::PreTranslateMessage(pMsg);
}


// CEQSetupDlg
BEGIN_MESSAGE_MAP(CEQSetupDlg, CDialog)
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON1,	OnEqFlat)
	ON_COMMAND(IDC_BUTTON2,	OnEqJazz)
	ON_COMMAND(IDC_BUTTON5,	OnEqPop)
	ON_COMMAND(IDC_BUTTON6,	OnEqRock)
	ON_COMMAND(IDC_BUTTON7,	OnEqConcert)
	ON_COMMAND(IDC_BUTTON8,	OnEqClear)
	ON_COMMAND(IDC_BUTTON3,	OnEqUser1)
	ON_COMMAND(IDC_BUTTON4,	OnEqUser2)
	ON_COMMAND(IDC_BUTTON9,	OnEqUser3)
	ON_COMMAND(IDC_BUTTON10,OnEqUser4)
	ON_COMMAND(IDC_BUTTON13,OnSavePreset)
	ON_COMMAND_RANGE(ID_EQSLIDER_BASE, ID_EQSLIDER_BASE+MAX_EQ_BANDS, OnSliderMenu)
	ON_COMMAND_RANGE(ID_EQMENU_BASE, ID_EQMENU_BASE+EQ_MAX_FREQS, OnSliderFreq)
END_MESSAGE_MAP()


BOOL CEQSetupDlg::OnInitDialog()
//------------------------------
{
	CDialog::OnInitDialog();
	m_Sliders[0].Init(IDC_SLIDER1, 0, this);
	m_Sliders[1].Init(IDC_SLIDER3, 1, this);
	m_Sliders[2].Init(IDC_SLIDER5, 2, this);
	m_Sliders[3].Init(IDC_SLIDER7, 3, this);
	m_Sliders[4].Init(IDC_SLIDER8, 4, this);
	m_Sliders[5].Init(IDC_SLIDER9, 5, this);
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		m_Sliders[i].SetRange(0, 32);
		m_Sliders[i].SetTicFreq(4);
	}
	UpdateDialog();
	return TRUE;
}


static void f2s(UINT f, LPSTR s)
//------------------------------
{
	if (f < 1000)
	{
		wsprintf(s, "%dHz", f);
	} else
	{
		UINT fHi = f / 1000;
		UINT fLo = f % 1000;
		if (fLo)
		{
			wsprintf(s, "%d.%dkHz", fHi, fLo/100);
		} else
		{
			wsprintf(s, "%dkHz", fHi);
		}
	}
}


void CEQSetupDlg::UpdateDialog()
//------------------------------
{
	const USHORT uTextIds[MAX_EQ_BANDS] = {IDC_TEXT1, IDC_TEXT2, IDC_TEXT3, IDC_TEXT4, IDC_TEXT5, IDC_TEXT6};
	CHAR s[32];
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		int n = 32 - m_pEqPreset->Gains[i];
		if (n < 0) n = 0;
		if (n > 32) n = 32;
		if (n != (m_Sliders[i].GetPos() & 0xFFFF)) m_Sliders[i].SetPos(n);
		f2s(m_pEqPreset->Freqs[i], s);
		SetDlgItemText(uTextIds[i], s);
		SetDlgItemText(IDC_BUTTON3,	gUserPresets[0].szName);
		SetDlgItemText(IDC_BUTTON4,	gUserPresets[1].szName);
		SetDlgItemText(IDC_BUTTON9,	gUserPresets[2].szName);
		SetDlgItemText(IDC_BUTTON10,gUserPresets[3].szName);
	}
}


void CEQSetupDlg::UpdateEQ(BOOL bReset)
//-------------------------------------
{
	BEGIN_CRITICAL();
	CSoundFile::SetEQGains(	m_pEqPreset->Gains, MAX_EQ_BANDS, m_pEqPreset->Freqs, bReset);
	END_CRITICAL();
}


void CEQSetupDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
//--------------------------------------------------------------------------
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		int n = 32 - m_Sliders[i].GetPos();
		if ((n >= 0) && (n <= 32)) m_pEqPreset->Gains[i] = n;
	}
	UpdateEQ(FALSE);
}


void CEQSetupDlg::OnEqFlat()
//--------------------------
{
	*m_pEqPreset = gEQPresets[EQPRESET_FLAT];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqJazz()
//--------------------------
{
	*m_pEqPreset = gEQPresets[EQPRESET_JAZZ];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqPop()
//-------------------------
{
	*m_pEqPreset = gEQPresets[EQPRESET_POP];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqRock()
//--------------------------
{
	*m_pEqPreset = gEQPresets[EQPRESET_ROCK];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqConcert()
//-----------------------------
{
	*m_pEqPreset = gEQPresets[EQPRESET_CONCERT];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqClear()
//---------------------------
{
	*m_pEqPreset = gEQPresets[EQPRESET_CLEAR];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqUser1()
//---------------------------
{
	*m_pEqPreset = gUserPresets[0];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqUser2()
//---------------------------
{
	*m_pEqPreset = gUserPresets[1];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqUser3()
//---------------------------
{
	*m_pEqPreset = gUserPresets[2];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnEqUser4()
//---------------------------
{
	*m_pEqPreset = gUserPresets[3];
	UpdateEQ(TRUE);
	UpdateDialog();
}


void CEQSetupDlg::OnSavePreset()
//------------------------------
{
	CEQSavePresetDlg dlg(m_pEqPreset, this);
	if (dlg.DoModal() == IDOK)
	{
		UpdateDialog();
	}
}


void CEQSetupDlg::OnSliderMenu(UINT nID)
//--------------------------------------
{
	UINT n = nID - ID_EQSLIDER_BASE;
	if (n < MAX_EQ_BANDS)
	{
		CHAR s[32];
		HMENU hMenu = ::CreatePopupMenu();
		m_nSliderMenu = n;
		if (!hMenu)	return;
		const UINT *pFreqs = &gEqBandFreqs[m_nSliderMenu*EQ_MAX_FREQS];
		for (UINT i=0; i<EQ_MAX_FREQS; i++)
		{
			DWORD d = MF_STRING;
			if (m_pEqPreset->Freqs[m_nSliderMenu] == pFreqs[i]) d |= MF_CHECKED;
			f2s(pFreqs[i], s);
			::AppendMenu(hMenu, d, ID_EQMENU_BASE+i, s);
		}
		CPoint pt(m_Sliders[m_nSliderMenu].m_x, m_Sliders[m_nSliderMenu].m_y);
		m_Sliders[m_nSliderMenu].ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
		::DestroyMenu(hMenu);
	}
}


void CEQSetupDlg::OnSliderFreq(UINT nID)
//--------------------------------------
{
	UINT n = nID - ID_EQMENU_BASE;
	if ((m_nSliderMenu < MAX_EQ_BANDS) && (n < EQ_MAX_FREQS))
	{
		UINT f = gEqBandFreqs[m_nSliderMenu*EQ_MAX_FREQS+n];
		if (f != m_pEqPreset->Freqs[m_nSliderMenu])
		{
			m_pEqPreset->Freqs[m_nSliderMenu] = f;
			UpdateEQ(TRUE);
			UpdateDialog();
		}
	}
}


BOOL CEQSetupDlg::OnSetActive()
//-----------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_EQ;
	return CPropertyPage::OnSetActive();
}


//////////////////////////////////////////////////////////////
// CRawSampleDlg


BOOL CRawSampleDlg::OnInitDialog()
//--------------------------------
{
	CDialog::OnInitDialog();
	UpdateDialog();
	return TRUE;
}


void CRawSampleDlg::OnOK()
//------------------------
{
	m_nFormat = 0;
	if (IsDlgButtonChecked(IDC_RADIO2)) m_nFormat |= 1;
	if (IsDlgButtonChecked(IDC_RADIO4)) m_nFormat |= 2;
	if (IsDlgButtonChecked(IDC_RADIO6)) m_nFormat |= 4;
	CDialog::OnOK();
}


void CRawSampleDlg::UpdateDialog()
//--------------------------------
{
	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, (m_nFormat & 1) ? IDC_RADIO2 : IDC_RADIO1);
	CheckRadioButton(IDC_RADIO3, IDC_RADIO4, (m_nFormat & 2) ? IDC_RADIO4 : IDC_RADIO3);
	CheckRadioButton(IDC_RADIO5, IDC_RADIO6, (m_nFormat & 4) ? IDC_RADIO6 : IDC_RADIO5);
}


/////////////////////////////////////////////////////////////
// CMidiSetupDlg

extern UINT gnMidiImportSpeed;
extern UINT gnMidiPatternLen;

BEGIN_MESSAGE_MAP(CMidiSetupDlg, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK3,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,			OnSettingsChanged)
END_MESSAGE_MAP()


void CMidiSetupDlg::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_SPIN1,		m_SpinSpd);
	DDX_Control(pDX, IDC_SPIN2,		m_SpinPat);
	//}}AFX_DATA_MAP
}
	

BOOL CMidiSetupDlg::OnInitDialog()
//--------------------------------
{
	MIDIINCAPS mic;
	CComboBox *combo;

	CPropertyPage::OnInitDialog();
	// Flags
	if (m_dwMidiSetup & MIDISETUP_RECORDVELOCITY) CheckDlgButton(IDC_CHECK1, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_RECORDNOTEOFF) CheckDlgButton(IDC_CHECK2, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_AMPLIFYVELOCITY) CheckDlgButton(IDC_CHECK3, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_TRANSPOSEKEYBOARD) CheckDlgButton(IDC_CHECK4, MF_CHECKED);
	// Midi In Device
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		UINT ndevs = midiInGetNumDevs();
		for (UINT i=0; i<ndevs; i++)
		{
			mic.szPname[0] = 0;
			if (midiInGetDevCaps(i, &mic, sizeof(mic)) == MMSYSERR_NOERROR)
				combo->SetItemData(combo->AddString(mic.szPname), i);
		}
		combo->SetCurSel((m_nMidiDevice == MIDI_MAPPER) ? 0 : m_nMidiDevice);
	}
	// Midi Import settings
	SetDlgItemInt(IDC_EDIT1, gnMidiImportSpeed);
	SetDlgItemInt(IDC_EDIT2, gnMidiPatternLen);
	m_SpinSpd.SetRange(2, 6);
	m_SpinPat.SetRange(64, 256);
	return TRUE;
}


void CMidiSetupDlg::OnOK()
//------------------------
{
	CComboBox *combo;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	m_dwMidiSetup = 0;
	m_nMidiDevice = MIDI_MAPPER;
	if (IsDlgButtonChecked(IDC_CHECK1)) m_dwMidiSetup |= MIDISETUP_RECORDVELOCITY;
	if (IsDlgButtonChecked(IDC_CHECK2)) m_dwMidiSetup |= MIDISETUP_RECORDNOTEOFF;
	if (IsDlgButtonChecked(IDC_CHECK3)) m_dwMidiSetup |= MIDISETUP_AMPLIFYVELOCITY;
	if (IsDlgButtonChecked(IDC_CHECK4)) m_dwMidiSetup |= MIDISETUP_TRANSPOSEKEYBOARD;
	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		int n = combo->GetCurSel();
		if (n >= 0) m_nMidiDevice = combo->GetItemData(n);
	}
	gnMidiImportSpeed = GetDlgItemInt(IDC_EDIT1);
	gnMidiPatternLen = GetDlgItemInt(IDC_EDIT2);
	if (pMainFrm) pMainFrm->SetupMidi(m_dwMidiSetup, m_nMidiDevice);
	CPropertyPage::OnOK();
}


BOOL CMidiSetupDlg::OnSetActive()
//-------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIDI;
	return CPropertyPage::OnSetActive();
}



