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


static const char * const PolyphonyNames[] =
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

static const CHANNELINDEX PolyphonyChannels[] =
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

STATIC_ASSERT(CountOf(PolyphonyNames) == CountOf(PolyphonyChannels));


const char *gszChnCfgNames[3] =
{
	"Mono",
	"Stereo",
	"Quad"
};


BEGIN_MESSAGE_MAP(COptionsSoundcard, CPropertyPage)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_CHECK4,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK5,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK6,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK7,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK8,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK9,	OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnDeviceChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATEINTERVAL, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4, OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5, OnChannelsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6, OnSampleFormatChanged)
	ON_CBN_SELCHANGE(IDC_COMBO10, OnSettingsChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO2, OnSettingsChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO_UPDATEINTERVAL, OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,	OnSoundCardRescan)
	ON_COMMAND(IDC_BUTTON2,	OnSoundCardDriverPanel)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_FRONTLEFT, OnChannel1Changed)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_FRONTRIGHT, OnChannel2Changed)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_REARLEFT, OnChannel3Changed)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_REARRIGHT, OnChannel4Changed)
END_MESSAGE_MAP()


void COptionsSoundcard::OnSampleFormatChanged()
//---------------------------------------------
{
	OnSettingsChanged();
	UpdateDither();
}


void COptionsSoundcard::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_COMBO1,		m_CbnDevice);
	DDX_Control(pDX, IDC_COMBO2,		m_CbnLatencyMS);
	DDX_Control(pDX, IDC_COMBO_UPDATEINTERVAL, m_CbnUpdateIntervalMS);
	DDX_Control(pDX, IDC_COMBO3,		m_CbnMixingFreq);
	DDX_Control(pDX, IDC_COMBO5,		m_CbnChannels);
	DDX_Control(pDX, IDC_COMBO6,		m_CbnSampleFormat);
	DDX_Control(pDX, IDC_COMBO10,		m_CbnDither);
	DDX_Control(pDX, IDC_BUTTON2,		m_BtnDriverPanel);
	DDX_Control(pDX, IDC_COMBO6,		m_CbnSampleFormat);
	DDX_Control(pDX, IDC_STATIC_CHANNEL_FRONTLEFT , m_StaticChannelMapping[0]);
	DDX_Control(pDX, IDC_STATIC_CHANNEL_FRONTRIGHT, m_StaticChannelMapping[1]);
	DDX_Control(pDX, IDC_STATIC_CHANNEL_REARLEFT  , m_StaticChannelMapping[2]);
	DDX_Control(pDX, IDC_STATIC_CHANNEL_REARRIGHT , m_StaticChannelMapping[3]);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_FRONTLEFT , m_CbnChannelMapping[0]);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_FRONTRIGHT, m_CbnChannelMapping[1]);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_REARLEFT  , m_CbnChannelMapping[2]);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_REARRIGHT , m_CbnChannelMapping[3]);
	DDX_Control(pDX, IDC_EDIT_STATISTICS,	m_EditStatistics);
	//}}AFX_DATA_MAP
}


COptionsSoundcard::COptionsSoundcard(SoundDeviceID dev)
//-----------------------------------------------------
	: CPropertyPage(IDD_OPTIONS_SOUNDCARD)
	, m_CurrentDeviceInfo(theApp.GetSoundDevicesManager()->FindDeviceInfo(dev))
	, m_CurrentDeviceCaps(theApp.GetSoundDevicesManager()->GetDeviceCaps(dev, TrackerSettings::Instance().GetSampleRates(), CMainFrame::GetMainFrame(), CMainFrame::GetMainFrame()->gpSoundDevice, true))
	, m_Settings(TrackerSettings::Instance().GetSoundDeviceSettings(dev))
{
	return;
}


void COptionsSoundcard::SetDevice(SoundDeviceID dev, bool forceReload)
//--------------------------------------------------------------------
{
	bool deviceChanged = (dev != m_CurrentDeviceInfo.id);
	m_CurrentDeviceInfo = theApp.GetSoundDevicesManager()->FindDeviceInfo(dev);
	m_CurrentDeviceCaps = theApp.GetSoundDevicesManager()->GetDeviceCaps(dev, TrackerSettings::Instance().GetSampleRates(), CMainFrame::GetMainFrame(), CMainFrame::GetMainFrame()->gpSoundDevice, true);
	if(deviceChanged || forceReload)
	{
		m_Settings = TrackerSettings::Instance().GetSoundDeviceSettings(dev);
	}
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


void COptionsSoundcard::UpdateLatency()
//-------------------------------------
{
	// latency
	{
		CHAR s[128];
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
}


void COptionsSoundcard::UpdateUpdateInterval()
//--------------------------------------------
{
	// update interval
	{
		CHAR s[128];
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
}


void COptionsSoundcard::UpdateEverything()
//----------------------------------------
{
	// General
	{
		CheckDlgButton(IDC_CHECK6, TrackerSettings::Instance().m_SoundSettingsKeepDeviceOpen ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(IDC_CHECK7, TrackerSettings::Instance().m_SoundSettingsOpenDeviceAtStartup ? BST_CHECKED : BST_UNCHECKED);
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
					// No real image available for now,
					// prepend API name to name and misuse another icon
					cbi.iImage = IMAGE_SAMPLEMUTE;
					name = mpt::ToCString(it->apiName) + TEXT(" - ") + name;
					break;
				case SNDDEV_PORTAUDIO_WDMKS:
					// No real image available for now,
					// prepend API name to name and misuse another icon.
					cbi.iImage = IMAGE_RAMDRIVE;
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
	}

	UpdateDevice();

}


void COptionsSoundcard::UpdateDevice()
//------------------------------------
{
	UpdateControls();
	UpdateLatency();
	UpdateUpdateInterval();
	UpdateSampleRates();
	UpdateChannels();
	UpdateSampleFormat();
	UpdateDither();
	UpdateChannelMapping();
}


void COptionsSoundcard::UpdateChannels()
//--------------------------------------
{
	m_CbnChannels.ResetContent();
	UINT maxChannels = 0;
	if(m_CurrentDeviceCaps.channelNames.size() > 0)
	{
		maxChannels = std::min<std::size_t>(4, m_CurrentDeviceCaps.channelNames.size());
	} else
	{
		maxChannels = 4;
	}
	int sel = 0;
	for(UINT channels = maxChannels; channels >= 1; channels /= 2)
	{
		int ndx = m_CbnChannels.AddString(gszChnCfgNames[(channels+2)/2-1]);
		m_CbnChannels.SetItemData(ndx, channels);
		if(channels == m_Settings.Channels)
		{
			sel = ndx;
		}
	}
	m_CbnChannels.SetCurSel(sel);
}


void COptionsSoundcard::UpdateSampleFormat()
//------------------------------------------
{
	UINT n = 0;
	m_CbnSampleFormat.ResetContent();
	m_CbnSampleFormat.EnableWindow(m_CurrentDeviceCaps.CanSampleFormat ? TRUE : FALSE);
	for(UINT bits = 40; bits >= 8; bits -= 8)
	{
		if(bits == 40)
		{
			if(m_CurrentDeviceCaps.CanSampleFormat || (SampleFormatFloat32 == m_Settings.sampleFormat))
			{
				UINT ndx = m_CbnSampleFormat.AddString("Float");
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


void COptionsSoundcard::UpdateDither()
//------------------------------------
{
	m_CbnDither.ResetContent();
	SampleFormat sampleFormat = static_cast<SampleFormatEnum>(m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel()));
	if(sampleFormat.IsInt() && sampleFormat.GetBitsPerSample() < 32)
	{
		m_CbnDither.EnableWindow(TRUE);
		for(int i=0; i<NumDitherModes; ++i)
		{
			m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName((DitherMode)i) + L" dithering"));
		}
	} else if(m_CurrentDeviceCaps.HasInternalDither)
	{
		m_CbnDither.EnableWindow(TRUE);
		m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherNone) + L" dithering"));
		m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherDefault) + L" dithering"));
	} else
	{
		m_CbnDither.EnableWindow(FALSE);
		for(int i=0; i<NumDitherModes; ++i)
		{
			m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherNone) + L" dithering"));
		}
	}
	if(m_Settings.DitherType < 0 || m_Settings.DitherType >= m_CbnDither.GetCount())
	{
		m_CbnDither.SetCurSel(1);
	} else
	{
		m_CbnDither.SetCurSel(m_Settings.DitherType);
	}
}


void COptionsSoundcard::UpdateChannelMapping()
//--------------------------------------------
{
	int usedChannels = m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel());
	for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)	// Host channels
	{
		CStatic *statictext = &m_StaticChannelMapping[mch];
		CComboBox *combo = &m_CbnChannelMapping[mch];
		statictext->EnableWindow((m_CurrentDeviceCaps.CanChannelMapping && mch < usedChannels) ? TRUE : FALSE);
		combo->EnableWindow((m_CurrentDeviceCaps.CanChannelMapping && mch < usedChannels) ? TRUE : FALSE);
		combo->ResetContent();
		if(m_CurrentDeviceCaps.CanChannelMapping)
		{
			combo->SetItemData(combo->AddString("Unassigned"), (DWORD_PTR)-1);
			combo->SetCurSel(0);
			if(mch < usedChannels)
			{
				for(size_t dch = 0; dch < m_CurrentDeviceCaps.channelNames.size(); dch++)	// Device channels
				{
					const int pos = (int)::SendMessageW(combo->m_hWnd, CB_ADDSTRING, 0, (LPARAM)m_CurrentDeviceCaps.channelNames[dch].c_str());
					combo->SetItemData(pos, (DWORD_PTR)dch);
					if(dch == m_Settings.ChannelMapping.ToDevice(mch))
					{
						combo->SetCurSel(pos);
					}
				}
			}
		}
	}
}


void COptionsSoundcard::OnDeviceChanged()
//---------------------------------------
{
	int n = m_CbnDevice.GetCurSel();
	if(n >= 0)
	{
		SetDevice(SoundDeviceID::FromIdRaw(m_CbnDevice.GetItemData(n)));
		UpdateDevice();
		OnSettingsChanged();
	}
}


void COptionsSoundcard::OnChannelsChanged()
//-----------------------------------------
{
	UpdateChannelMapping();
	OnSettingsChanged();
}


void COptionsSoundcard::OnSoundCardDriverPanel()
//----------------------------------------------
{
	theApp.GetSoundDevicesManager()->OpenDriverSettings(SoundDeviceID::FromIdRaw(m_CbnDevice.GetItemData(m_CbnDevice.GetCurSel())), CMainFrame::GetMainFrame(), CMainFrame::GetMainFrame()->gpSoundDevice);
}


void COptionsSoundcard::OnChannelChanged(int channel)
//---------------------------------------------------
{
	CComboBox *combo = &m_CbnChannelMapping[channel];
	const int newChn = combo->GetItemData(combo->GetCurSel());
	if(newChn == -1)
	{
		return;
	}
	// Ensure that no channel is used twice
	for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)	// Host channels
	{
		if(mch != channel)
		{
			combo = &m_CbnChannelMapping[mch];
			if((int)combo->GetItemData(combo->GetCurSel()) == newChn)
			{
				// find an unused channel
				bool found = false;
				std::size_t deviceChannel = 0;
				for(; deviceChannel < m_CurrentDeviceCaps.channelNames.size(); ++deviceChannel)
				{
					bool used = false;
					for(int hostChannel = 0; hostChannel < NUM_CHANNELCOMBOBOXES; ++hostChannel)
					{
						if((int)m_CbnChannelMapping[hostChannel].GetItemData(m_CbnChannelMapping[hostChannel].GetCurSel()) == (int)deviceChannel)
						{
							used = true;
							break;
						}
					}
					if(!used)
					{
						found = true;
						break;
					}
				}
				if(found)
				{
					combo->SetCurSel(deviceChannel+1);
				} else
				{
					combo->SetCurSel(0);
				}
				break;
			}
		}
	}
	OnSettingsChanged();
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
	if(!m_CurrentDeviceCaps.CanKeepDeviceRunning)
	{
		m_Settings.KeepDeviceRunning = false;
	}
	m_BtnDriverPanel.EnableWindow(m_CurrentDeviceCaps.CanDriverPanel ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK4)->EnableWindow(m_CurrentDeviceCaps.CanExclusiveMode ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK5)->EnableWindow(m_CurrentDeviceCaps.CanBoostThreadPriority ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK8)->EnableWindow(m_CurrentDeviceCaps.CanKeepDeviceRunning ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK9)->EnableWindow(m_CurrentDeviceCaps.CanUseHardwareTiming ? TRUE : FALSE);
	GetDlgItem(IDC_STATIC_UPDATEINTERVAL)->EnableWindow(m_CurrentDeviceCaps.CanUpdateInterval ? TRUE : FALSE);
	GetDlgItem(IDC_COMBO_UPDATEINTERVAL)->EnableWindow(m_CurrentDeviceCaps.CanUpdateInterval ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK4)->SetWindowText(mpt::ToCString(m_CurrentDeviceCaps.ExclusiveModeDescription));
	CheckDlgButton(IDC_CHECK4, m_CurrentDeviceCaps.CanExclusiveMode && m_Settings.ExclusiveMode ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK5, m_CurrentDeviceCaps.CanBoostThreadPriority && m_Settings.BoostThreadPriority ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK8, m_CurrentDeviceCaps.CanKeepDeviceRunning && m_Settings.KeepDeviceRunning ? MF_CHECKED : MF_UNCHECKED);
	CheckDlgButton(IDC_CHECK9, m_CurrentDeviceCaps.CanUseHardwareTiming && m_Settings.UseHardwareTiming ? MF_CHECKED : MF_UNCHECKED);
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
	// General
	{
		TrackerSettings::Instance().m_SoundSettingsKeepDeviceOpen = IsDlgButtonChecked(IDC_CHECK6) ? true : false;
		TrackerSettings::Instance().m_SoundSettingsOpenDeviceAtStartup = IsDlgButtonChecked(IDC_CHECK7) ? true : false;
	}
	m_Settings.ExclusiveMode = IsDlgButtonChecked(IDC_CHECK4) ? true : false;
	m_Settings.BoostThreadPriority = IsDlgButtonChecked(IDC_CHECK5) ? true : false;
	m_Settings.KeepDeviceRunning = IsDlgButtonChecked(IDC_CHECK8) ? true : false;
	m_Settings.UseHardwareTiming = IsDlgButtonChecked(IDC_CHECK9) ? true : false;
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
	// Dither
	{
		UINT n = m_CbnDither.GetCurSel();
		m_Settings.DitherType = (DitherMode)(n);
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
	// Channel Mapping
	{
		if(m_CurrentDeviceCaps.CanChannelMapping)
		{
			int numChannels = std::min<int>(m_Settings.Channels, NUM_CHANNELCOMBOBOXES);
			std::vector<uint32> channels(numChannels);
			for(int mch = 0; mch < numChannels; mch++)	// Host channels
			{
				CComboBox *combo = &m_CbnChannelMapping[mch];
				channels[mch] = combo->GetItemData(combo->GetCurSel());
			}
			m_Settings.ChannelMapping = SoundChannelMapping(channels);
		} else
		{
			m_Settings.ChannelMapping = SoundChannelMapping();
		}
	}
	CMainFrame::GetMainFrame()->SetupSoundCard(m_Settings, m_CurrentDeviceInfo.id);
	SetDevice(m_CurrentDeviceInfo.id, true); // Poll changed ASIO sample format and channel names
	UpdateDevice();
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
		if(bufferAttributes.NumBuffers > 2)
		{
			s += mpt::String::Print("Buffer: %1%% (%2/%3)\r\n", (bufferAttributes.Latency > 0.0) ? Util::Round<int64>(currentLatency / bufferAttributes.Latency * 100.0) : 0, (currentUpdateInterval > 0.0) ? Util::Round<int64>(bufferAttributes.Latency / currentUpdateInterval) : 0, bufferAttributes.NumBuffers);
		} else
		{
			s += mpt::String::Print("Buffer: %1%%\r\n", (bufferAttributes.Latency > 0.0) ? Util::Round<int64>(currentLatency / bufferAttributes.Latency * 100.0) : 0);
		}
		s += mpt::String::Print("Latency: %1 ms (current: %2 ms, %3 frames)\r\n", mpt::Format("%4.1f").ToString(bufferAttributes.Latency * 1000.0), mpt::Format("%4.1f").ToString(currentLatency * 1000.0), Util::Round<int64>(currentLatency * samplerate));
		s += mpt::String::Print("Period: %1 ms (current: %2 ms, %3 frames)\r\n", mpt::Format("%4.1f").ToString(bufferAttributes.UpdateInterval * 1000.0), mpt::Format("%4.1f").ToString(currentUpdateInterval * 1000.0), Util::Round<int64>(currentUpdateInterval * samplerate));
		s += pMainFrm->gpSoundDevice->GetStatistics();
		m_EditStatistics.SetWindowText(s.c_str());
	}	else
	{
		m_EditStatistics.SetWindowText("");
	}
}


//////////////////
// COptionsMixer

BEGIN_MESSAGE_MAP(COptionsMixer, CPropertyPage)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER, OnResamplerChanged)
	ON_EN_UPDATE(IDC_WFIRCUTOFF, OnSettingsChanged)
	ON_EN_UPDATE(IDC_COMBO_FILTERWINDOW, OnSettingsChanged)
	ON_EN_UPDATE(IDC_RAMPING_IN, OnResamplerChanged)
	ON_EN_UPDATE(IDC_RAMPING_OUT,	OnResamplerChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_POLYPHONY, OnSettingsChanged)
	// slider stereo sep
	ON_COMMAND(IDC_CHECK_SOFTPAN, OnSettingsChanged)
	// slider preamp
END_MESSAGE_MAP()


void COptionsMixer::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_COMBO_FILTER, m_CbnResampling);
	DDX_Control(pDX, IDC_WFIRCUTOFF, m_CEditWFIRCutoff);
	DDX_Control(pDX, IDC_COMBO_FILTERWINDOW, m_CbnWFIRType);
	DDX_Control(pDX, IDC_RAMPING_IN, m_CEditRampUp);
	DDX_Control(pDX, IDC_RAMPING_OUT, m_CEditRampDown);
	DDX_Control(pDX, IDC_EDIT_VOLRAMP_SAMPLES_UP, m_CInfoRampUp);
	DDX_Control(pDX, IDC_EDIT_VOLRAMP_SAMPLES_DOWN, m_CInfoRampDown);
	DDX_Control(pDX, IDC_COMBO_POLYPHONY, m_CbnPolyphony);
	DDX_Control(pDX, IDC_SLIDER_STEREOSEP, m_SliderStereoSep);
	// check box soft pan
	DDX_Control(pDX, IDC_SLIDER_PREAMP, m_SliderPreAmp);
	//}}AFX_DATA_MAP
}


BOOL COptionsMixer::OnInitDialog()
//--------------------------------
{
	CPropertyPage::OnInitDialog();

	// Resampling type
	{
		m_CbnResampling.AddString("No Interpolation (1 tap)");
		m_CbnResampling.AddString("Linear (2 tap)");
		m_CbnResampling.AddString("Cubic spline (4 tap)");
		m_CbnResampling.AddString("Polyphase (8 tap)");
		m_CbnResampling.AddString("XMMS-ModPlug (8 tap)");
		m_CbnResampling.SetCurSel(TrackerSettings::Instance().ResamplerMode);
	}

	// Resampler bandwidth
	{
		m_CEditWFIRCutoff.SetWindowText(mpt::ToString(TrackerSettings::Instance().ResamplerCutoffPercent).c_str());
	}

	// Resampler filter window
	{
		// done in OnResamplerChanged()
	}

	// volume ramping
	{
#if 0
		m_CEditRampUp.SetWindowText(mpt::ToString(TrackerSettings::Instance().MixerVolumeRampUpSamples).c_str());
		m_CEditRampDown.SetWindowText(mpt::ToString(TrackerSettings::Instance().MixerVolumeRampDownSamples).c_str());
#else
		m_CEditRampUp.SetWindowText(mpt::ToString(TrackerSettings::Instance().GetMixerSettings().GetVolumeRampUpMicroseconds()).c_str());
		m_CEditRampDown.SetWindowText(mpt::ToString(TrackerSettings::Instance().GetMixerSettings().GetVolumeRampDownMicroseconds()).c_str());
#endif
		UpdateRamping();
	}

	// Max Mixing Channels
	{
		m_CbnPolyphony.ResetContent();
		for(std::size_t n = 0; n < CountOf(PolyphonyChannels); ++n)
		{
			m_CbnPolyphony.AddString(mpt::String::Print("%1 (%2)", PolyphonyChannels[n], PolyphonyNames[n]).c_str());
			if(TrackerSettings::Instance().MixerMaxChannels == PolyphonyChannels[n])
			{
				m_CbnPolyphony.SetCurSel(n);
			}
		}
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

	// soft pan
	{
		CheckDlgButton(IDC_CHECK_SOFTPAN, (TrackerSettings::Instance().MixerFlags & SNDMIX_SOFTPANNING) ? BST_CHECKED : BST_UNCHECKED);
	}

	// Pre-Amplification
	{
		m_SliderPreAmp.SetTicFreq(5);
		m_SliderPreAmp.SetRange(0, 40);
		int n = (TrackerSettings::Instance().MixerPreAmp - 64) / 8;
		if ((n < 0) || (n > 40)) n = 16;
		m_SliderPreAmp.SetPos(n);
	}

	OnResamplerChanged();

	return TRUE;
}


BOOL COptionsMixer::OnSetActive()
//-------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIXER;
	return CPropertyPage::OnSetActive();
}


void COptionsMixer::OnResamplerChanged()
//--------------------------------------
{
	int dwSrcMode = m_CbnResampling.GetCurSel();
	switch(dwSrcMode)
	{
		case SRCMODE_FIRFILTER:
			m_CbnWFIRType.ResetContent();
			m_CbnWFIRType.AddString("Hann");
			m_CbnWFIRType.AddString("Hamming");
			m_CbnWFIRType.AddString("Blackman Exact");
			m_CbnWFIRType.AddString("Blackman 3 Tap 61");
			m_CbnWFIRType.AddString("Blackman 3 Tap 67");
			m_CbnWFIRType.AddString("Blackman Harris");
			m_CbnWFIRType.AddString("Blackman 4 Tap 74");
			m_CbnWFIRType.AddString("Kaiser a=7.5");
			m_CbnWFIRType.SetCurSel(TrackerSettings::Instance().ResamplerSubMode);
			break;
		case SRCMODE_POLYPHASE:
			m_CbnWFIRType.ResetContent();
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.AddString("Auto");
			m_CbnWFIRType.SetCurSel(TrackerSettings::Instance().ResamplerSubMode);
			break;
		default:
			m_CbnWFIRType.ResetContent();
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.AddString("none");
			m_CbnWFIRType.SetCurSel(TrackerSettings::Instance().ResamplerSubMode);
			break;
	}
	switch(dwSrcMode)
	{
		case SRCMODE_POLYPHASE:
			m_CEditWFIRCutoff.EnableWindow(TRUE);
			m_CbnWFIRType.EnableWindow(FALSE);
			break;
		case SRCMODE_FIRFILTER:
			m_CEditWFIRCutoff.EnableWindow(TRUE);
			m_CbnWFIRType.EnableWindow(TRUE);
			break;
		default:
			m_CEditWFIRCutoff.EnableWindow(FALSE);
			m_CbnWFIRType.EnableWindow(FALSE);
			break;
	}
	OnSettingsChanged();
}


void COptionsMixer::OnRampingChanged()
//------------------------------------
{
	UpdateRamping();
	OnSettingsChanged();
}


void COptionsMixer::OnScroll(UINT n, UINT pos, CScrollBar *p)
//-----------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(n);
	MPT_UNREFERENCED_PARAMETER(pos);
	MPT_UNREFERENCED_PARAMETER(p);
	// stereo sep
	{
		UpdateStereoSep();
	}
}


void COptionsMixer::UpdateRamping()
//---------------------------------
{
	MixerSettings settings = TrackerSettings::Instance().GetMixerSettings();
	CString s;
	m_CEditRampUp.GetWindowText(s);
	settings.SetVolumeRampUpMicroseconds(atol(s));
	m_CEditRampDown.GetWindowText(s);
	settings.SetVolumeRampDownMicroseconds(atol(s));
	s.Format("%i samples at %i Hz", (int)settings.GetVolumeRampUpSamples(), (int)settings.gdwMixingFreq);
	m_CInfoRampUp.SetWindowText(s);
	s.Format("%i samples at %i Hz", (int)settings.GetVolumeRampDownSamples(), (int)settings.gdwMixingFreq);
	m_CInfoRampDown.SetWindowText(s);
}


void COptionsMixer::UpdateStereoSep()
//-----------------------------------
{
	CString s;
	s.Format("%d%%", ((32 << m_SliderStereoSep.GetPos()) * 100) / 128);
	SetDlgItemText(IDC_TEXT_STEREOSEP, s);
}


void COptionsMixer::OnOK()
//------------------------
{
	// resampler mode
	{
		TrackerSettings::Instance().ResamplerMode = (ResamplingMode)m_CbnResampling.GetCurSel();
	}

	// resampler bandwidth
	{
		CString s;
		m_CEditWFIRCutoff.GetWindowText(s);
		if(s != "")
		{
			int newCutoff = atoi(s);
			Limit(newCutoff, 0, 100);
			TrackerSettings::Instance().ResamplerCutoffPercent = newCutoff;
		}
		{
			s.Format("%d", TrackerSettings::Instance().ResamplerCutoffPercent.Get());
			m_CEditWFIRCutoff.SetWindowText(s);
		}
	}

	// resampler filter window
	{
		TrackerSettings::Instance().ResamplerSubMode = (uint8)m_CbnWFIRType.GetCurSel();
	}

	// volume ramping
	{
#if 0
		CString s;
		m_CEditRampUp.GetWindowText(s);
		TrackerSettings::Instance().MixerVolumeRampUpSamples = atol(s);
		m_CEditRampDown.GetWindowText(s);
		TrackerSettings::Instance().MixerVolumeRampDownSamples = atol(s);
#else
		MixerSettings settings = TrackerSettings::Instance().GetMixerSettings();
		CString s;
		m_CEditRampUp.GetWindowText(s);
		settings.SetVolumeRampUpMicroseconds(atol(s));
		m_CEditRampDown.GetWindowText(s);
		settings.SetVolumeRampDownMicroseconds(atol(s));
		TrackerSettings::Instance().SetMixerSettings(settings);
#endif
	}

	// Polyphony
	{
		int polyphony = m_CbnPolyphony.GetCurSel();
		if(polyphony >= 0 && polyphony < CountOf(PolyphonyChannels))
		{
			TrackerSettings::Instance().MixerMaxChannels = PolyphonyChannels[polyphony];
		}
	}

	// stereo sep
	{
		TrackerSettings::Instance().MixerStereoSeparation = 32 << m_SliderStereoSep.GetPos();
	}

	// soft pan
	{
		if(IsDlgButtonChecked(IDC_CHECK2))
		{
			TrackerSettings::Instance().MixerFlags = TrackerSettings::Instance().MixerFlags | SNDMIX_SOFTPANNING;
		} else
		{
			TrackerSettings::Instance().MixerFlags = TrackerSettings::Instance().MixerFlags & ~SNDMIX_SOFTPANNING;
		}
	}

	// pre amp
	{
		int n = m_SliderPreAmp.GetPos();
		if ((n >= 0) && (n <= 40)) // approximately +/- 10dB
		{
			TrackerSettings::Instance().MixerPreAmp = 64 + (n * 8);
		}
	}

	CMainFrame::GetMainFrame()->SetupPlayer();

	CPropertyPage::OnOK();
}


//////////////////////////////////////////////////////////
// COptionsPlayer

BEGIN_MESSAGE_MAP(COptionsPlayer, CPropertyPage)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK3,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK5,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK6,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK7,			OnSettingsChanged)
END_MESSAGE_MAP()


void COptionsPlayer::DoDataExchange(CDataExchange* pDX)
//-----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsPlayer)
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


void COptionsPlayer::OnOK()
//-------------------------
{
	DWORD dwQuality = 0;

	DWORD dwQualityMask = 0;

#ifndef NO_DSP
	dwQualityMask |= SNDDSP_MEGABASS;
	if (IsDlgButtonChecked(IDC_CHECK1)) dwQuality |= SNDDSP_MEGABASS;
#endif
#ifndef NO_AGC
	dwQualityMask |= SNDDSP_AGC;
	if (IsDlgButtonChecked(IDC_CHECK2)) dwQuality |= SNDDSP_AGC;
#endif
#ifndef NO_EQ
	dwQualityMask |= SNDDSP_EQ;
	if (IsDlgButtonChecked(IDC_CHECK3)) dwQuality |= SNDDSP_EQ;
#endif
#ifndef NO_DSP
	dwQualityMask |= SNDDSP_SURROUND;
	dwQualityMask |= SNDDSP_NOISEREDUCTION;
	if (IsDlgButtonChecked(IDC_CHECK4)) dwQuality |= SNDDSP_SURROUND;
	if (IsDlgButtonChecked(IDC_CHECK5)) dwQuality |= SNDDSP_NOISEREDUCTION;
#endif
#ifndef NO_REVERB
	dwQualityMask |= SNDDSP_REVERB;
	if (IsDlgButtonChecked(IDC_CHECK6)) dwQuality |= SNDDSP_REVERB;
#endif

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
	}
#endif
#ifndef NO_REVERB
	// Reverb
	{
		// Reverb depth is dynamically changed
		UINT nReverbType = m_CbnReverbPreset.GetItemData(m_CbnReverbPreset.GetCurSel());
		if (nReverbType < NUM_REVERBTYPES) TrackerSettings::Instance().m_ReverbSettings.m_nReverbType = nReverbType;
	}
#endif
#ifndef NO_DSP
	// Surround
	{
		UINT nProLogicDepth = m_SbSurroundDepth.GetPos();
		UINT nProLogicDelay = 5 + (m_SbSurroundDelay.GetPos() * 5);
		TrackerSettings::Instance().m_DSPSettings.m_nProLogicDepth = nProLogicDepth;
		TrackerSettings::Instance().m_DSPSettings.m_nProLogicDelay = nProLogicDelay;
	}
#endif

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
