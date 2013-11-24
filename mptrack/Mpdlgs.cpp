/*
 * MPDlgs.cpp
 * ----------
 * Purpose: Implementation of various player setup dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "Sndfile.h"
#include "mainfrm.h"
#include "Dlsbank.h"
#include "mpdlgs.h"
#include "moptions.h"
#include "moddoc.h"
#include "../sounddev/SoundDevice.h"
#include "mpdlgs.h"
#include "../common/StringFixer.h"

#define str_preampChangeNote GetStrI18N(_TEXT("Note: The Pre-Amp setting affects sample volume only. Changing it may cause undesired effects on volume balance between sample based instruments and plugin instruments.\nIn other words: Don't touch this slider unless you know what you are doing."))

//#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"

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


UINT nCPUMix[] =
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


const char *gszChnCfgNames[3] =
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
	ON_COMMAND(IDC_CHECK5,	OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnDeviceChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATEINTERVAL, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO9, OnSettingsChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO2, OnSettingsChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO_UPDATEINTERVAL, OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,	OnSoundCardRescan)
	ON_COMMAND(IDC_BUTTON2,	OnSoundCardDriverPanel)
END_MESSAGE_MAP()


void COptionsSoundcard::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_COMBO1,		m_CbnDevice);
	DDX_Control(pDX, IDC_COMBO2,		m_CbnLatencyMS);
	DDX_Control(pDX, IDC_COMBO_UPDATEINTERVAL, m_CbnUpdateIntervalMS);
	DDX_Control(pDX, IDC_COMBO3,		m_CbnMixingFreq);
	DDX_Control(pDX, IDC_COMBO4,		m_CbnPolyphony);
	DDX_Control(pDX, IDC_COMBO5,		m_CbnChannels);
	DDX_Control(pDX, IDC_COMBO6,		m_CbnSampleFormat);
	DDX_Control(pDX, IDC_COMBO9,		m_CbnBaseChannel);
	DDX_Control(pDX, IDC_BUTTON2,		m_BtnDriverPanel);
	DDX_Control(pDX, IDC_SLIDER1,		m_SliderStereoSep);
	DDX_Control(pDX, IDC_SLIDER_PREAMP,	m_SliderPreAmp);
	DDX_Control(pDX, IDC_EDIT_STATISTICS,	m_EditStatistics);
	//}}AFX_DATA_MAP
}


COptionsSoundcard::COptionsSoundcard(const SoundDeviceSettings &settings, SoundDeviceID sd)
//-----------------------------------------------------------------------------------------
	: CPropertyPage(IDD_OPTIONS_SOUNDCARD)
	, m_Settings(settings)
{
	m_PreAmpNoteShowed = false;
	SetDevice(sd);
}


void COptionsSoundcard::SetDevice(SoundDeviceID dev)
//--------------------------------------------------
{
	m_CurrentDeviceInfo = theApp.GetSoundDevicesManager()->FindDeviceInfo(dev);
	m_CurrentDeviceCaps = theApp.GetSoundDevicesManager()->GetDeviceCaps(dev, TrackerSettings::Instance().GetSampleRates(), CMainFrame::GetMainFrame(), CMainFrame::GetMainFrame()->gpSoundDevice, true);
}


void COptionsSoundcard::OnSoundCardRescan()
//-----------------------------------------
{
	{
		// Close sound device because IDs might change when re-enumerating which could cause all kinds of havoc.
		CMainFrame::GetMainFrame()->audioCloseDevice();
		delete CMainFrame::GetMainFrame()->gpSoundDevice;
		CMainFrame::GetMainFrame()->gpSoundDevice = nullptr;
	}
	theApp.GetSoundDevicesManager()->ReEnumerate();
	UpdateEverything();
}


BOOL COptionsSoundcard::OnInitDialog()
//------------------------------------
{
	CPropertyPage::OnInitDialog();
	UpdateEverything();
	return TRUE;
}


void COptionsSoundcard::UpdateEverything()
//----------------------------------------
{
	
	CHAR s[128];

	CheckDlgButton(IDC_CHECK2, (TrackerSettings::Instance().MixerFlags & SNDMIX_SOFTPANNING) ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK4, m_Settings.ExclusiveMode ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK5, m_Settings.BoostThreadPriority ? MF_CHECKED : MF_UNCHECKED);

	// Sampling Rate
	UpdateSampleRates();

	// Max Mixing Channels
	{
		m_CbnPolyphony.ResetContent();
		for (UINT n = 0; n < CountOf(nCPUMix); n++)
		{
			wsprintf(s, "%d (%s)", nCPUMix[n], szCPUNames[n]);
			m_CbnPolyphony.AddString(s);
			if (TrackerSettings::Instance().MixerMaxChannels == nCPUMix[n]) m_CbnPolyphony.SetCurSel(n);
		}
	}
	// latency
	{
		m_CbnLatencyMS.ResetContent();
		wsprintf(s, "%d ms", m_Settings.LatencyMS);
		m_CbnLatencyMS.SetWindowText(s);
		m_CbnLatencyMS.AddString("1 ms");
		m_CbnLatencyMS.AddString("2 ms");
		m_CbnLatencyMS.AddString("3 ms");
		m_CbnLatencyMS.AddString("4 ms");
		m_CbnLatencyMS.AddString("5 ms");
		m_CbnLatencyMS.AddString("10 ms");
		m_CbnLatencyMS.AddString("15 ms");
		m_CbnLatencyMS.AddString("20 ms");
		m_CbnLatencyMS.AddString("25 ms");
		m_CbnLatencyMS.AddString("30 ms");
		m_CbnLatencyMS.AddString("40 ms");
		m_CbnLatencyMS.AddString("50 ms");
		m_CbnLatencyMS.AddString("75 ms");
		m_CbnLatencyMS.AddString("100 ms");
		m_CbnLatencyMS.AddString("150 ms");
		m_CbnLatencyMS.AddString("200 ms");
		m_CbnLatencyMS.AddString("250 ms");
	}
	// update interval
	{
		m_CbnUpdateIntervalMS.ResetContent();
		wsprintf(s, "%d ms", m_Settings.UpdateIntervalMS);
		m_CbnUpdateIntervalMS.SetWindowText(s);
		m_CbnUpdateIntervalMS.AddString("1 ms");
		m_CbnUpdateIntervalMS.AddString("2 ms");
		m_CbnUpdateIntervalMS.AddString("5 ms");
		m_CbnUpdateIntervalMS.AddString("10 ms");
		m_CbnUpdateIntervalMS.AddString("15 ms");
		m_CbnUpdateIntervalMS.AddString("20 ms");
		m_CbnUpdateIntervalMS.AddString("25 ms");
		m_CbnUpdateIntervalMS.AddString("50 ms");
	}
	// Stereo Separation
	{
		m_SliderStereoSep.SetRange(0, 4);
		m_SliderStereoSep.SetPos(2);
		for (int n=0; n<=4; n++)
		{
			if ((int)TrackerSettings::Instance().MixerStereoSeparation <= (int)(32 << n))
			{
				m_SliderStereoSep.SetPos(n);
				break;
			}
		}
		UpdateStereoSep();
	}
	// Pre-Amplification
	{
		m_SliderPreAmp.SetTicFreq(5);
		m_SliderPreAmp.SetRange(0, 40);
		SetPreAmpSliderPosition();
	}
	// Sound Device
	{
		m_CbnDevice.ResetContent();
		m_CbnDevice.SetImageList(CMainFrame::GetMainFrame()->GetImageList());

		COMBOBOXEXITEM cbi;
		UINT iItem = 0;

		for(std::vector<SoundDeviceInfo>::const_iterator it = theApp.GetSoundDevicesManager()->begin(); it != theApp.GetSoundDevicesManager()->end(); ++it)
		{

			if(!TrackerSettings::Instance().m_MorePortaudio)
			{
				if(it->id.GetType() == SNDDEV_PORTAUDIO_ASIO || it->id.GetType() == SNDDEV_PORTAUDIO_DS || it->id.GetType() == SNDDEV_PORTAUDIO_WMME)
				{
					// skip those portaudio apis that are already implemented via our own ISoundDevice class
					// can be overwritten via [Sound Settings]MorePortaudio=1
					continue;
				}
			}

			{
				CString name = mpt::ToCString(it->name);
				cbi.mask = CBEIF_IMAGE | CBEIF_LPARAM | CBEIF_TEXT | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
				cbi.iItem = iItem;
				cbi.cchTextMax = 0;
				switch(it->id.GetType())
				{
				case SNDDEV_WAVEOUT:
				case SNDDEV_PORTAUDIO_WMME:
					cbi.iImage = IMAGE_WAVEOUT;
					break;
				case SNDDEV_DSOUND:
				case SNDDEV_PORTAUDIO_DS:
					cbi.iImage = IMAGE_DIRECTX;
					break;
				case SNDDEV_ASIO:
				case SNDDEV_PORTAUDIO_ASIO:
					cbi.iImage = IMAGE_ASIO;
					break;
				case SNDDEV_PORTAUDIO_WASAPI:
				case SNDDEV_PORTAUDIO_WDMKS:
					cbi.iImage = IMAGE_WAVEOUT;
					// No image available for now,
					// prepend API name to name.
					name = mpt::ToCString(it->apiName) + TEXT(" - ") + name;
					break;
				default:
					cbi.iImage = IMAGE_WAVEOUT;
					break;
				}
				if(it->isDefault)
				{
					name += " (Default)";
				}
				cbi.iSelectedImage = cbi.iImage;
				cbi.iOverlay = cbi.iImage;
				cbi.iIndent = 0;
				cbi.lParam = it->id.GetIdRaw();
				TCHAR tmp[256];
				_tcscpy(tmp, name);
				cbi.pszText = tmp;
				int pos = m_CbnDevice.InsertItem(&cbi);
				if(cbi.lParam == m_CurrentDeviceInfo.id.GetIdRaw())
				{
					m_CbnDevice.SetCurSel(pos);
				}
				iItem++;
			}
		}
		UpdateControls();
	}

	UpdateChannels();
	UpdateSampleFormat();
	
}


void COptionsSoundcard::UpdateChannels()
//--------------------------------------
{
	CHAR s[128];
	UINT n = 0;
	m_CbnChannels.ResetContent();
	UINT maxChannels = 0;
	if(m_CurrentDeviceCaps.channelNames.size() > 0)
	{
		maxChannels = std::min<std::size_t>(4, m_CurrentDeviceCaps.channelNames.size());
	} else
	{
		maxChannels = 4;
	}
	for(UINT channels = maxChannels; channels >= 1; channels /= 2)
	{
		wsprintf(s, "%s", gszChnCfgNames[(channels+2)/2-1]);
		UINT ndx = m_CbnChannels.AddString(s);
		m_CbnChannels.SetItemData(ndx, channels);
		if(channels == m_Settings.Channels)
		{
			n = ndx;
		}
	}
	m_CbnChannels.SetCurSel(n);
	if(m_CurrentDeviceCaps.CanChannelMapping)
	{
		m_CbnBaseChannel.ResetContent();
		m_CbnBaseChannel.EnableWindow(TRUE);
		int sel = 0;
		for(std::size_t channel = 0; channel < m_CurrentDeviceCaps.channelNames.size(); ++channel)
		{
			int ndx = m_CbnBaseChannel.AddString(mpt::ToCString(m_CurrentDeviceCaps.channelNames[channel]));
			m_CbnBaseChannel.SetItemData(ndx, channel);
			if(channel == m_Settings.ChannelMapping.GetBaseChannel())
			{
				sel = ndx;
			}
		}
		m_CbnBaseChannel.SetCurSel(sel);
	} else
	{
		m_CbnBaseChannel.ResetContent();
		m_CbnBaseChannel.EnableWindow(FALSE);
	}
	m_BtnDriverPanel.ShowWindow(m_CurrentDeviceCaps.CanDriverPanel ? SW_SHOW : SW_HIDE);
}


void COptionsSoundcard::UpdateSampleFormat()
//------------------------------------------
{
	UINT n = 0;
	m_CbnSampleFormat.ResetContent();
	if(m_CurrentDeviceInfo.id.GetType() == SNDDEV_ASIO)
	{
		m_Settings.sampleFormat = TrackerSettings::Instance().m_SampleFormat;
	}
	m_CbnSampleFormat.EnableWindow(m_CurrentDeviceCaps.CanSampleFormat ? TRUE : FALSE);
	for(UINT bits = 40; bits >= 8; bits -= 8)
	{
		if(bits == 40)
		{
			if(m_CurrentDeviceCaps.CanSampleFormat || (SampleFormatFloat32 == m_Settings.sampleFormat))
			{
				UINT ndx = m_CbnSampleFormat.AddString("Floating Point");
				m_CbnSampleFormat.SetItemData(ndx, (32+128));
				if(SampleFormatFloat32 == m_Settings.sampleFormat)
				{
					n = ndx;
				}
			}
		} else
		{
			if(m_CurrentDeviceCaps.CanSampleFormat || ((SampleFormat)bits == m_Settings.sampleFormat))
			{
				UINT ndx = m_CbnSampleFormat.AddString(mpt::String::Format("%d Bit", bits).c_str());
				m_CbnSampleFormat.SetItemData(ndx, bits);
				if((SampleFormat)bits == m_Settings.sampleFormat)
				{
					n = ndx;
				}
			}
		}
	}
	m_CbnSampleFormat.SetCurSel(n);
}


void COptionsSoundcard::OnHScroll(UINT n, UINT pos, CScrollBar *p)
//----------------------------------------------------------------
{
	CPropertyPage::OnHScroll(n, pos, p);
	// stereo sep
	{
		UpdateStereoSep();
	}
	// PreAmp
	if(p == (CScrollBar *)(&m_SliderPreAmp))
	{
		if(m_PreAmpNoteShowed == true)
		{
			int n = m_SliderPreAmp.GetPos();
			if ((n >= 0) && (n <= 40)) // approximately +/- 10dB
			{
				CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
				if (pMainFrm) pMainFrm->SetPreAmp(64 + (n * 8));
			}
		} else
		{
			m_PreAmpNoteShowed = true;
			Reporting::Information(str_preampChangeNote);
			SetPreAmpSliderPosition();
		}
	}
}


void COptionsSoundcard::UpdateStereoSep()
//---------------------------------------
{
	CHAR s[64];
	TrackerSettings::Instance().MixerStereoSeparation = 32 << m_SliderStereoSep.GetPos();
	CMainFrame::GetMainFrame()->SetupPlayer();
	wsprintf(s, "%d%%", (TrackerSettings::Instance().MixerStereoSeparation * 100) / 128);
	SetDlgItemText(IDC_TEXT1, s);

}


void COptionsSoundcard::SetPreAmpSliderPosition()
//-----------------------------------------------
{
	int n = (TrackerSettings::Instance().MixerPreAmp - 64) / 8;
	if ((n < 0) || (n > 40)) n = 16;
	m_SliderPreAmp.SetPos(n);
}


void COptionsSoundcard::OnVScroll(UINT n, UINT pos, CScrollBar *p)
//----------------------------------------------------------------
{
	CPropertyPage::OnVScroll(n, pos, p);
}


void COptionsSoundcard::OnDeviceChanged()
//---------------------------------------
{
	int n = m_CbnDevice.GetCurSel();
	if(n >= 0)
	{
		SetDevice(SoundDeviceID::FromIdRaw(m_CbnDevice.GetItemData(n)));
		UpdateControls();
		UpdateSampleRates();
		UpdateChannels();
		UpdateSampleFormat();
		OnSettingsChanged();
	}
}


void COptionsSoundcard::OnSoundCardDriverPanel()
//----------------------------------------------
{
	theApp.GetSoundDevicesManager()->OpenDriverSettings(SoundDeviceID::FromIdRaw(m_CbnDevice.GetItemData(m_CbnDevice.GetCurSel())), CMainFrame::GetMainFrame(), CMainFrame::GetMainFrame()->gpSoundDevice);
}


// Fill the dropdown box with a list of valid sample rates, depending on the selected sound device.
void COptionsSoundcard::UpdateSampleRates()
//-----------------------------------------
{
	m_CbnMixingFreq.ResetContent();

	std::vector<uint32> samplerates = m_CurrentDeviceCaps.supportedSampleRates;

	if(samplerates.empty())
	{
		// We have no valid list of supported playback rates! Assume all rates supported by OpenMPT are possible...
		samplerates = TrackerSettings::Instance().GetSampleRates();
	}

	int n = 0;
	for(size_t i = 0; i < samplerates.size(); i++)
	{
		CHAR s[16];
		wsprintf(s, "%i Hz", samplerates[i]);
		int pos = m_CbnMixingFreq.AddString(s);
		m_CbnMixingFreq.SetItemData(pos, samplerates[i]);
		if(m_Settings.Samplerate == samplerates[i])
		{
			n = pos;
		}
	}
	m_CbnMixingFreq.SetCurSel(n);
}


void COptionsSoundcard::UpdateControls()
//--------------------------------------
{
	GetDlgItem(IDC_CHECK4)->EnableWindow(m_CurrentDeviceCaps.CanExclusiveMode ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK5)->EnableWindow(m_CurrentDeviceCaps.CanBoostThreadPriority ? TRUE : FALSE);
	GetDlgItem(IDC_STATIC_UPDATEINTERVAL)->EnableWindow(m_CurrentDeviceCaps.CanUpdateInterval ? TRUE : FALSE);
	GetDlgItem(IDC_COMBO_UPDATEINTERVAL)->EnableWindow(m_CurrentDeviceCaps.CanUpdateInterval ? TRUE : FALSE);
	if(m_CurrentDeviceInfo.id.GetType() == SNDDEV_DSOUND)
	{
		GetDlgItem(IDC_CHECK4)->SetWindowText("Use primary buffer");
	} else
	{
		GetDlgItem(IDC_CHECK4)->SetWindowText("Use device exclusively");
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
	if(IsDlgButtonChecked(IDC_CHECK2))
		TrackerSettings::Instance().MixerFlags = TrackerSettings::Instance().MixerFlags | SNDMIX_SOFTPANNING;
	else
		TrackerSettings::Instance().MixerFlags = TrackerSettings::Instance().MixerFlags & ~SNDMIX_SOFTPANNING;
	m_Settings.ExclusiveMode = IsDlgButtonChecked(IDC_CHECK4) ? true : false;
	m_Settings.BoostThreadPriority = IsDlgButtonChecked(IDC_CHECK5) ? true : false;
	// Mixing Freq
	{
		m_Settings.Samplerate = m_CbnMixingFreq.GetItemData(m_CbnMixingFreq.GetCurSel());
	}
	// Channels
	{
		UINT n = m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel());
		m_Settings.Channels = static_cast<uint8>(n);
		if((m_Settings.Channels != 1) && (m_Settings.Channels != 4))
		{
			m_Settings.Channels = 2;
		}
	}
	// SampleFormat
	{
		UINT n = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
		m_Settings.sampleFormat = (SampleFormat)(n & 0xFF);
	}
	// Polyphony
	{
		int nmmx = m_CbnPolyphony.GetCurSel();
		if ((nmmx >= 0) && (nmmx < CountOf(nCPUMix)))
		{
			TrackerSettings::Instance().MixerMaxChannels = nCPUMix[nmmx];
			CMainFrame::GetMainFrame()->SetupPlayer();
		}
	}
	// Sound Device
	{
		int n = m_CbnDevice.GetCurSel();
		if(n >= 0)
		{
			SetDevice(SoundDeviceID::FromIdRaw(m_CbnDevice.GetItemData(n)));
		}
	}
	const SoundDeviceID dev = m_CurrentDeviceInfo.id;
	// Latency
	{
		CHAR s[32];
		m_CbnLatencyMS.GetWindowText(s, sizeof(s));
		m_Settings.LatencyMS = atoi(s);
		//Check given value.
		m_Settings.LatencyMS = CLAMP(m_Settings.LatencyMS, SNDDEV_MINLATENCY_MS, SNDDEV_MAXLATENCY_MS);
		wsprintf(s, "%d ms", m_Settings.LatencyMS);
		m_CbnLatencyMS.SetWindowText(s);
	}
	// Update Interval
	{
		CHAR s[32];
		m_CbnUpdateIntervalMS.GetWindowText(s, sizeof(s));
		m_Settings.UpdateIntervalMS = atoi(s);
		//Check given value.
		m_Settings.UpdateIntervalMS = CLAMP(m_Settings.UpdateIntervalMS, SNDDEV_MINUPDATEINTERVAL_MS, SNDDEV_MAXUPDATEINTERVAL_MS);
		wsprintf(s, "%d ms", m_Settings.UpdateIntervalMS);
		m_CbnUpdateIntervalMS.SetWindowText(s);
	}
	// Base Channel
	{
		if(m_CurrentDeviceInfo.id.GetType() == SNDDEV_ASIO)
		{
			m_Settings.ChannelMapping = SoundChannelMapping::BaseChannel(m_Settings.Channels, m_CbnBaseChannel.GetItemData(m_CbnBaseChannel.GetCurSel()));
		}
	}
	CMainFrame::GetMainFrame()->SetupSoundCard(m_Settings, m_CurrentDeviceInfo.id);
	SetDevice(m_CurrentDeviceInfo.id); // Poll changed ASIO channel names
	UpdateSampleFormat();
	UpdateChannels();
	UpdateStatistics();
	CPropertyPage::OnOK();
}


void COptionsSoundcard::UpdateStatistics()
//----------------------------------------
{
	if (!m_EditStatistics) return;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm->gpSoundDevice && pMainFrm->IsPlaying())
	{
		const SoundBufferAttributes bufferAttributes = pMainFrm->gpSoundDevice->GetBufferAttributes();
		const double currentLatency = pMainFrm->gpSoundDevice->GetCurrentLatency();
		const double currentUpdateInterval = pMainFrm->gpSoundDevice->GetCurrentUpdateInterval();
		const uint32 samplerate = pMainFrm->gpSoundDevice->GetSettings().Samplerate;
		std::string s;
		s += mpt::String::Print("Buffer: %1%%\r\n", (bufferAttributes.Latency > 0.0) ? Util::Round<int64>(currentLatency / bufferAttributes.Latency * 100.0) : 0);
		s += mpt::String::Print("Buffers: %1 (current: %2)\r\n", bufferAttributes.NumBuffers, (currentUpdateInterval > 0.0) ? Util::Round<int64>(bufferAttributes.Latency / currentUpdateInterval) : 0);
		s += mpt::String::Print("Latency: %1 ms (current: %2 ms, %3 frames)\r\n", mpt::Format("%4.1f").ToString(bufferAttributes.Latency * 1000.0), mpt::Format("%4.1f").ToString(currentLatency * 1000.0), Util::Round<int64>(currentLatency * samplerate));
		s += mpt::String::Print("Update Interval: %1 ms (current: %2 ms, %3 frames)\r\n", mpt::Format("%4.1f").ToString(bufferAttributes.UpdateInterval * 1000.0), mpt::Format("%4.1f").ToString(currentUpdateInterval * 1000.0), Util::Round<int64>(currentUpdateInterval * samplerate));
		m_EditStatistics.SetWindowText(s.c_str());
	}	else
	{
		m_EditStatistics.SetWindowText("");
	}
}


//////////////////////////////////////////////////////////
// COptionsPlayer

BEGIN_MESSAGE_MAP(COptionsPlayer, CPropertyPage)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO1,	OnResamplerChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnSettingsChanged)
	//rewbs.resamplerConf
	ON_CBN_SELCHANGE(IDC_WFIRTYPE,	OnWFIRTypeChanged)
	ON_EN_UPDATE(IDC_WFIRCUTOFF,	OnSettingsChanged)
	ON_EN_UPDATE(IDC_RAMPING_IN,	OnSettingsChanged)
	ON_EN_UPDATE(IDC_RAMPING_OUT,	OnSettingsChanged)
	//end rewbs.resamplerConf
	ON_COMMAND(IDC_CHECK1,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK3,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK5,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK6,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK7,			OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON_DEFAULT_RESAMPLING,	OnDefaultResampling)
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
	DDX_Control(pDX, IDC_RAMPING_IN,	m_CEditRampUp);
	DDX_Control(pDX, IDC_RAMPING_OUT,	m_CEditRampDown);
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
	dwQuality = TrackerSettings::Instance().MixerDSPMask;
	// Resampling type
	{
		m_CbnResampling.AddString("No Interpolation");
		m_CbnResampling.AddString("Linear");
		m_CbnResampling.AddString("Cubic spline");
		//rewbs.resamplerConf
		m_CbnResampling.AddString("Polyphase");
		m_CbnResampling.AddString("XMMS-ModPlug");
		//end rewbs.resamplerConf
		m_CbnResampling.SetCurSel(TrackerSettings::Instance().ResamplerMode);
	}
	// Effects
#ifndef NO_DSP
	if (dwQuality & SNDDSP_MEGABASS) CheckDlgButton(IDC_CHECK1, MF_CHECKED);
#else
	GetDlgItem(IDC_CHECK1)->ShowWindow(SW_HIDE);
#endif
#ifndef NO_AGC
	if (dwQuality & SNDDSP_AGC) CheckDlgButton(IDC_CHECK2, MF_CHECKED);
#else
	GetDlgItem(IDC_CHECK2)->ShowWindow(SW_HIDE);
#endif
#ifndef NO_DSP
	if (dwQuality & SNDDSP_SURROUND) CheckDlgButton(IDC_CHECK4, MF_CHECKED);
	if (dwQuality & SNDDSP_NOISEREDUCTION) CheckDlgButton(IDC_CHECK5, MF_CHECKED);
#else
	GetDlgItem(IDC_CHECK4)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK5)->ShowWindow(SW_HIDE);
#endif
#ifndef NO_EQ
	if (dwQuality & SNDDSP_EQ) CheckDlgButton(IDC_CHECK3, MF_CHECKED);
#else
	GetDlgItem(IDC_CHECK3)->ShowWindow(SW_HIDE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK3), FALSE);
#endif

#ifndef NO_DSP
	// Bass Expansion
	m_SbXBassDepth.SetRange(0,4);
	m_SbXBassDepth.SetPos(8-TrackerSettings::Instance().m_DSPSettings.m_nXBassDepth);
	m_SbXBassRange.SetRange(0,4);
	m_SbXBassRange.SetPos(4 - (TrackerSettings::Instance().m_DSPSettings.m_nXBassRange - 1) / 5);
#else
	m_SbXBassDepth.ShowWindow(SW_HIDE);
	m_SbXBassRange.ShowWindow(SW_HIDE);
#endif

#ifndef NO_REVERB
	// Reverb
	m_SbReverbDepth.SetRange(1, 16);
	m_SbReverbDepth.SetPos(TrackerSettings::Instance().m_ReverbSettings.m_nReverbDepth);
	UINT nSel = 0;
	for (UINT iRvb=0; iRvb<NUM_REVERBTYPES; iRvb++)
	{
		LPCSTR pszName = GetReverbPresetName(iRvb);
		if (pszName)
		{
			UINT n = m_CbnReverbPreset.AddString(pszName);
			m_CbnReverbPreset.SetItemData(n, iRvb);
			if (iRvb == TrackerSettings::Instance().m_ReverbSettings.m_nReverbType) nSel = n;
		}
	}
	m_CbnReverbPreset.SetCurSel(nSel);
	if(!(GetProcSupport() & PROCSUPPORT_MMX))
	{
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK6), FALSE);
		m_SbReverbDepth.EnableWindow(FALSE);
		m_CbnReverbPreset.EnableWindow(FALSE);
	} else
	{
		if (dwQuality & SNDDSP_REVERB) CheckDlgButton(IDC_CHECK6, MF_CHECKED);
	}
#else
	GetDlgItem(IDC_CHECK6)->ShowWindow(SW_HIDE);
	m_SbReverbDepth.ShowWindow(SW_HIDE);
	m_CbnReverbPreset.ShowWindow(SW_HIDE);
#endif

#ifndef NO_DSP
	// Surround
	{
		UINT n = TrackerSettings::Instance().m_DSPSettings.m_nProLogicDepth;
		if (n < 1) n = 1;
		if (n > 16) n = 16;
		m_SbSurroundDepth.SetRange(1, 16);
		m_SbSurroundDepth.SetPos(n);
		m_SbSurroundDelay.SetRange(0, 8);
		m_SbSurroundDelay.SetPos((TrackerSettings::Instance().m_DSPSettings.m_nProLogicDelay-5)/5);
	}
#else
	m_SbSurroundDepth.ShowWindow(SW_HIDE);
	m_SbSurroundDelay.ShowWindow(SW_HIDE);
#endif
	//rewbs.resamplerConf
	OnResamplerChanged();

	char s[16] = "";
	_ltoa(TrackerSettings::Instance().MixerVolumeRampUpSamples, s, 10);
	m_CEditRampUp.SetWindowText(s);

	_ltoa(TrackerSettings::Instance().MixerVolumeRampDownSamples, s, 10);
	m_CEditRampDown.SetWindowText(s);

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
	if ((psb) && (psb->m_hWnd == m_SbReverbDepth.m_hWnd))
	{
#ifndef NO_REVERB
		UINT n = m_SbReverbDepth.GetPos();
		if ((n) && (n <= 16)) TrackerSettings::Instance().m_ReverbSettings.m_nReverbDepth = n;
		//if ((n) && (n <= 16)) CSoundFile::m_Reverb.m_Settings.m_nReverbDepth = n;
		CMainFrame::GetMainFrame()->SetupPlayer();
#endif
	} else
	{
		OnSettingsChanged();
	}
}

//rewbs.resamplerConf
void COptionsPlayer::OnWFIRTypeChanged()
{
	TrackerSettings::Instance().ResamplerSubMode = static_cast<BYTE>(m_CbnWFIRType.GetCurSel());
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
		wsprintf(s, "%d", static_cast<int>((TrackerSettings::Instance().ResamplerCutoffPercent)));
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
		m_CbnWFIRType.SetCurSel(TrackerSettings::Instance().ResamplerSubMode);
		m_CbnWFIRType.EnableWindow(TRUE);
		m_CEditWFIRCutoff.EnableWindow(TRUE);
		wsprintf(s, "%d", static_cast<int>((TrackerSettings::Instance().ResamplerCutoffPercent)));
		break;
	default:
		m_CbnWFIRType.AddString("None");
		m_CEditWFIRCutoff.EnableWindow(FALSE);
		m_CbnWFIRType.EnableWindow(FALSE);
	}

	m_CEditWFIRCutoff.SetWindowText(s);
	OnSettingsChanged();
}


void COptionsPlayer::OnDefaultResampling()
//----------------------------------------
{
	CResamplerSettings resamplerDefaults;
	m_CbnResampling.SetCurSel(resamplerDefaults.SrcMode);
	OnResamplerChanged();
	m_CbnWFIRType.SetCurSel(resamplerDefaults.gbWFIRType);
	m_CEditWFIRCutoff.SetWindowText(Stringify(static_cast<int>(resamplerDefaults.gdWFIRCutoff * 100)).c_str());
	MixerSettings mixerDefaults;
	m_CEditRampUp.SetWindowText(Stringify(mixerDefaults.glVolumeRampUpSamples).c_str());
	m_CEditRampDown.SetWindowText(Stringify(mixerDefaults.glVolumeRampDownSamples).c_str());
	OnSettingsChanged();
}


void COptionsPlayer::OnOK()
//-------------------------
{
	DWORD dwQuality = 0;
	DWORD dwSrcMode = 0;

#ifndef NO_DSP
	if (IsDlgButtonChecked(IDC_CHECK1)) dwQuality |= SNDDSP_MEGABASS;
#endif
#ifndef NO_AGC
	if (IsDlgButtonChecked(IDC_CHECK2)) dwQuality |= SNDDSP_AGC;
#endif
#ifndef NO_EQ
	if (IsDlgButtonChecked(IDC_CHECK3)) dwQuality |= SNDDSP_EQ;
#endif
#ifndef NO_DSP
	if (IsDlgButtonChecked(IDC_CHECK4)) dwQuality |= SNDDSP_SURROUND;
	if (IsDlgButtonChecked(IDC_CHECK5)) dwQuality |= SNDDSP_NOISEREDUCTION;
#endif
#ifndef NO_REVERB
	if (IsDlgButtonChecked(IDC_CHECK6)) dwQuality |= SNDDSP_REVERB;
#endif
	dwSrcMode = m_CbnResampling.GetCurSel();

#ifndef NO_DSP
	// Bass Expansion
	{
		UINT nXBassDepth = 8-m_SbXBassDepth.GetPos();
		if (nXBassDepth < 4) nXBassDepth = 4;
		if (nXBassDepth > 8) nXBassDepth = 8;
		UINT nXBassRange = (4-m_SbXBassRange.GetPos()) * 5 + 1;
		if (nXBassRange < 5) nXBassRange = 5;
		if (nXBassRange > 21) nXBassRange = 21;
		TrackerSettings::Instance().m_DSPSettings.m_nXBassDepth = nXBassDepth;
		TrackerSettings::Instance().m_DSPSettings.m_nXBassRange = nXBassRange;
		CMainFrame::GetMainFrame()->SetupPlayer();
	}
#endif
#ifndef NO_REVERB
	// Reverb
	{
		// Reverb depth is dynamically changed
		UINT nReverbType = m_CbnReverbPreset.GetItemData(m_CbnReverbPreset.GetCurSel());
		if (nReverbType < NUM_REVERBTYPES) TrackerSettings::Instance().m_ReverbSettings.m_nReverbType = nReverbType;
		CMainFrame::GetMainFrame()->SetupPlayer();
	}
#endif
#ifndef NO_DSP
	// Surround
	{
		UINT nProLogicDepth = m_SbSurroundDepth.GetPos();
		UINT nProLogicDelay = 5 + (m_SbSurroundDelay.GetPos() * 5);
		TrackerSettings::Instance().m_DSPSettings.m_nProLogicDepth = nProLogicDepth;
		TrackerSettings::Instance().m_DSPSettings.m_nProLogicDelay = nProLogicDelay;
		CMainFrame::GetMainFrame()->SetupPlayer();
	}
#endif
	//rewbs.resamplerConf
	CString s;
	m_CEditWFIRCutoff.GetWindowText(s);
	if(s != "")
	{
		int newCutoff = atoi(s);
		Limit(newCutoff, 0, 100);
		TrackerSettings::Instance().ResamplerCutoffPercent = newCutoff;
	}
	{
		CHAR s[64];
		wsprintf(s, "%d", TrackerSettings::Instance().ResamplerCutoffPercent.Get());
		m_CEditWFIRCutoff.SetWindowText(s);
	}
	//TrackerSettings::Instance().m_ResamplerSettings.gbWFIRType set in OnWFIRTypeChange

	m_CEditRampUp.GetWindowText(s);
	TrackerSettings::Instance().MixerVolumeRampUpSamples = atol(s);
	m_CEditRampDown.GetWindowText(s);
	TrackerSettings::Instance().MixerVolumeRampDownSamples = atol(s);
	TrackerSettings::Instance().ResamplerMode = (ResamplingMode)dwSrcMode;
	TrackerSettings::Instance().MixerDSPMask = dwQuality;
	CMainFrame::GetMainFrame()->SetupPlayer();
	CPropertyPage::OnOK();
}


////////////////////////////////////////////////////////////////////////////////
//
// EQ Globals
//

#define EQ_MAX_FREQS	5

const UINT gEqBandFreqs[MAX_EQ_BANDS][EQ_MAX_FREQS] =
{
	{ 100, 125, 150, 200, 250 },
	{ 300, 350, 400, 450, 500 },
	{ 600, 700, 800, 900, 1000 },
	{ 1250, 1500, 1750, 2000, 2500 },
	{ 3000, 3500, 4000, 4500, 5000 },
	{ 6000, 7000, 8000, 9000, 10000 },
};


const EQPreset CEQSetupDlg::gEQPresets[] =
{
	{ "Flat",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },	// Flat
	{ "Jazz",	{16,16,24,20,20,14}, { 125, 300, 600, 1250, 4000, 8000 } },	// Jazz
	{ "Pop",	{24,16,16,21,16,26}, { 125, 300, 600, 1250, 4000, 8000 } },	// Pop
	{ "Rock",	{24,16,24,16,24,22}, { 125, 300, 600, 1250, 4000, 8000 } },	// Rock
	{ "Concert",{22,18,26,16,22,16}, { 125, 300, 600, 1250, 4000, 8000 } },	// Concert
	{ "Clear",	{20,16,16,22,24,26}, { 125, 300, 600, 1250, 4000, 8000 } }	// Clear
};


EQPreset CEQSetupDlg::gUserPresets[] =
{
	{ "User1",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },		// User1
	{ "User2",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },		// User2
	{ "User3",	{16,16,16,16,16,16}, { 125, 300, 600, 1250, 4000, 8000 } },		// User3
	{ "User4",	{16,16,16,16,16,16}, { 150, 500, 1000, 2500, 5000, 10000 } }	// User4
};


////////////////////////////////////////////////////////////////////////////////
//
// CEQSavePresetDlg
//

//====================================
class CEQSavePresetDlg: public CDialog
//====================================
{
protected:
	EQPreset *m_pEq;

public:
	CEQSavePresetDlg(EQPreset *pEq, CWnd *parent=NULL):CDialog(IDD_SAVEPRESET, parent) { m_pEq = pEq; }
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
#ifndef NO_EQ
	CriticalSection cs;
	if(CMainFrame::GetMainFrame()->GetSoundFilePlaying())
		CMainFrame::GetMainFrame()->GetSoundFilePlaying()->SetEQGains( m_pEqPreset->Gains, MAX_EQ_BANDS, m_pEqPreset->Freqs, bReset);
#endif
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


void CEQSetupDlg::LoadEQPreset(const EQPreset &preset)
//----------------------------------------------------
{
	*m_pEqPreset = preset;
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
		const UINT *pFreqs = gEqBandFreqs[m_nSliderMenu];
		for (UINT i = 0; i < EQ_MAX_FREQS; i++)
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
		UINT f = gEqBandFreqs[m_nSliderMenu][n];
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
	SetDlgItemText(IDC_EQ_WARNING,
		"Note: This EQ, when enabled from Player tab, is applied to "
		"any and all of the modules "
		"that you load in OpenMPT; its settings are stored globally, "
		"rather than in each file. This means that you should avoid "
		"using it as part of your production process, and instead only "
		"use it to correct deficiencies in your audio hardware.");
	return CPropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////
// CMidiSetupDlg

BEGIN_MESSAGE_MAP(CMidiSetupDlg, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO1,			OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,					OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,					OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,					OnSettingsChanged)
	ON_COMMAND(IDC_MIDI_TO_PLUGIN,			OnSettingsChanged)
	ON_COMMAND(IDC_MIDI_MACRO_CONTROL,		OnSettingsChanged)
	ON_COMMAND(IDC_MIDIVOL_TO_NOTEVOL,		OnSettingsChanged)
	ON_COMMAND(IDC_MIDIPLAYCONTROL,			OnSettingsChanged)
	ON_COMMAND(IDC_MIDIPLAYPATTERNONMIDIIN,	OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT3,					OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT4,					OnSettingsChanged)
END_MESSAGE_MAP()


void CMidiSetupDlg::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_SPIN1,		m_SpinSpd);
	DDX_Control(pDX, IDC_SPIN2,		m_SpinPat);
	DDX_Control(pDX, IDC_SPIN3,		m_SpinAmp);
	DDX_Control(pDX, IDC_COMBO2,	m_ATBehaviour);
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
	if (m_dwMidiSetup & MIDISETUP_TRANSPOSEKEYBOARD) CheckDlgButton(IDC_CHECK4, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_MIDITOPLUG) CheckDlgButton(IDC_MIDI_TO_PLUGIN, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_MIDIMACROCONTROL) CheckDlgButton(IDC_MIDI_MACRO_CONTROL, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_MIDIVOL_TO_NOTEVOL) CheckDlgButton(IDC_MIDIVOL_TO_NOTEVOL, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_RESPONDTOPLAYCONTROLMSGS) CheckDlgButton(IDC_MIDIPLAYCONTROL, MF_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN) CheckDlgButton(IDC_MIDIPLAYPATTERNONMIDIIN, MF_CHECKED);

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

	// Aftertouch behaviour
	m_ATBehaviour.ResetContent();
	static const struct
	{
		const char *text;
		RecordAftertouchOptions option;
	} aftertouchOptions[] =
	{
		{ "Do not record Aftertouch", atDoNotRecord },
		{ "Record as Volume Commands", atRecordAsVolume },
		{ "Record as MIDI Macros", atRecordAsMacro },
	};

	for(size_t i = 0; i < CountOf(aftertouchOptions); i++)
	{
		int item = m_ATBehaviour.AddString(aftertouchOptions[i].text);
		m_ATBehaviour.SetItemData(item, aftertouchOptions[i].option);
		if(aftertouchOptions[i].option == TrackerSettings::Instance().aftertouchBehaviour)
		{
			m_ATBehaviour.SetCurSel(i);
		}
	}

	// Note Velocity amp
	SetDlgItemInt(IDC_EDIT3, TrackerSettings::Instance().midiVelocityAmp);
	m_SpinAmp.SetRange(1, 10000);

	SetDlgItemText(IDC_EDIT4, IgnoredCCsToString(TrackerSettings::Instance().midiIgnoreCCs).c_str());

	// Midi Import settings
	SetDlgItemInt(IDC_EDIT1, TrackerSettings::Instance().midiImportSpeed);
	SetDlgItemInt(IDC_EDIT2, TrackerSettings::Instance().midiImportPatternLen);
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
	if (IsDlgButtonChecked(IDC_CHECK4)) m_dwMidiSetup |= MIDISETUP_TRANSPOSEKEYBOARD;
	if (IsDlgButtonChecked(IDC_MIDI_TO_PLUGIN)) m_dwMidiSetup |= MIDISETUP_MIDITOPLUG;
	if (IsDlgButtonChecked(IDC_MIDI_MACRO_CONTROL)) m_dwMidiSetup |= MIDISETUP_MIDIMACROCONTROL;
	if (IsDlgButtonChecked(IDC_MIDIVOL_TO_NOTEVOL)) m_dwMidiSetup |= MIDISETUP_MIDIVOL_TO_NOTEVOL;
	if (IsDlgButtonChecked(IDC_MIDIPLAYCONTROL)) m_dwMidiSetup |= MIDISETUP_RESPONDTOPLAYCONTROLMSGS;
	if (IsDlgButtonChecked(IDC_MIDIPLAYPATTERNONMIDIIN)) m_dwMidiSetup |= MIDISETUP_PLAYPATTERNONMIDIIN;

	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		int n = combo->GetCurSel();
		if (n >= 0) m_nMidiDevice = combo->GetItemData(n);
	}

	TrackerSettings::Instance().aftertouchBehaviour = static_cast<RecordAftertouchOptions>(m_ATBehaviour.GetItemData(m_ATBehaviour.GetCurSel()));

	TrackerSettings::Instance().midiImportSpeed = GetDlgItemInt(IDC_EDIT1);
	TrackerSettings::Instance().midiImportPatternLen = GetDlgItemInt(IDC_EDIT2);
	TrackerSettings::Instance().midiVelocityAmp = static_cast<uint16>(Clamp(GetDlgItemInt(IDC_EDIT3), 1u, 10000u));

	CString cc;
	GetDlgItemText(IDC_EDIT4, cc);
	TrackerSettings::Instance().midiIgnoreCCs = StringToIgnoredCCs(cc.GetString());

	if (pMainFrm) pMainFrm->SetupMidi(m_dwMidiSetup, m_nMidiDevice);
	CPropertyPage::OnOK();
}


BOOL CMidiSetupDlg::OnSetActive()
//-------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIDI;
	return CPropertyPage::OnSetActive();
}
