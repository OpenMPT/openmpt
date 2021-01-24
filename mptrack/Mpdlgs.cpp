/*
 * MPDlgs.cpp
 * ----------
 * Purpose: Implementation of various player setup dialogs.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Sndfile.h"
#include "Mainfrm.h"
#include "ImageLists.h"
#include "Moddoc.h"
#include "Mpdlgs.h"
#include "dlg_misc.h"
#include "../common/mptStringBuffer.h"
#include "../sounddev/SoundDevice.h"
#include "../sounddev/SoundDeviceManager.h"


OPENMPT_NAMESPACE_BEGIN


const TCHAR *gszChnCfgNames[3] =
{
	_T("Mono"),
	_T("Stereo"),
	_T("Quad")
};


static double ParseTime(CString str)
{
	return ConvertStrTo<double>(mpt::ToCharset(mpt::Charset::ASCII, str)) / 1000.0;
}


static CString PrintTime(double seconds)
{
	int32 microseconds = mpt::saturate_round<int32>(seconds * 1000000.0);
	int precision = 0;
	if(microseconds < 1000)
	{
		precision = 3;
	} else if(microseconds < 10000)
	{
		precision = 2;
	} else if(microseconds < 100000)
	{
		precision = 1;
	} else
	{
		precision = 0;
	}
	return MPT_CFORMAT("{} ms")(mpt::cfmt::fix(seconds * 1000.0, precision));
}


BEGIN_MESSAGE_MAP(COptionsSoundcard, CPropertyPage)
	ON_WM_HSCROLL()
	ON_COMMAND(IDC_CHECK4,	&COptionsSoundcard::OnExclusiveModeChanged)
	ON_COMMAND(IDC_CHECK5,	&COptionsSoundcard::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK7,	&COptionsSoundcard::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK9,	&COptionsSoundcard::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK_SOUNDCARD_SHOWALL, &COptionsSoundcard::OnSoundCardShowAll)
	ON_CBN_SELCHANGE(IDC_COMBO1, &COptionsSoundcard::OnDeviceChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, &COptionsSoundcard::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATEINTERVAL, &COptionsSoundcard::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, &COptionsSoundcard::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4, &COptionsSoundcard::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5, &COptionsSoundcard::OnChannelsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO6, &COptionsSoundcard::OnSampleFormatChanged)
	ON_CBN_SELCHANGE(IDC_COMBO10, &COptionsSoundcard::OnSettingsChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO2, &COptionsSoundcard::OnSettingsChanged)
	ON_CBN_EDITCHANGE(IDC_COMBO_UPDATEINTERVAL, &COptionsSoundcard::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO11, &COptionsSoundcard::OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,	&COptionsSoundcard::OnSoundCardRescan)
	ON_COMMAND(IDC_BUTTON2,	&COptionsSoundcard::OnSoundCardDriverPanel)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_FRONTLEFT, &COptionsSoundcard::OnChannel1Changed)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_FRONTRIGHT, &COptionsSoundcard::OnChannel2Changed)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_REARLEFT, &COptionsSoundcard::OnChannel3Changed)
	ON_CBN_SELCHANGE(IDC_COMBO_CHANNEL_REARRIGHT, &COptionsSoundcard::OnChannel4Changed)
	ON_CBN_SELCHANGE(IDC_COMBO_RECORDING_CHANNELS, &COptionsSoundcard::OnRecordingChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_RECORDING_SOURCE, &COptionsSoundcard::OnSettingsChanged)
END_MESSAGE_MAP()


void COptionsSoundcard::OnSampleFormatChanged()
{
	OnSettingsChanged();
	UpdateDither();
}


void COptionsSoundcard::OnRecordingChanged()
{
	DWORD_PTR inputChannels = m_CbnRecordingChannels.GetItemData(m_CbnRecordingChannels.GetCurSel());
	m_CbnRecordingSource.EnableWindow((m_CurrentDeviceCaps.HasNamedInputSources && inputChannels > 0) ? TRUE : FALSE);
	OnSettingsChanged();
}


void COptionsSoundcard::DoDataExchange(CDataExchange* pDX)
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
	DDX_Control(pDX, IDC_COMBO11,		m_CbnStoppedMode);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_FRONTLEFT , m_CbnChannelMapping[0]);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_FRONTRIGHT, m_CbnChannelMapping[1]);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_REARLEFT  , m_CbnChannelMapping[2]);
	DDX_Control(pDX, IDC_COMBO_CHANNEL_REARRIGHT , m_CbnChannelMapping[3]);
	DDX_Control(pDX, IDC_COMBO_RECORDING_CHANNELS,		m_CbnRecordingChannels);
	DDX_Control(pDX, IDC_COMBO_RECORDING_SOURCE,		m_CbnRecordingSource);
	DDX_Control(pDX, IDC_EDIT_STATISTICS,	m_EditStatistics);
	//}}AFX_DATA_MAP
}


COptionsSoundcard::COptionsSoundcard(SoundDevice::Identifier deviceIdentifier)
	: CPropertyPage(IDD_OPTIONS_SOUNDCARD)
	, m_InitialDeviceIdentifier(deviceIdentifier)
{
	return;
}


void COptionsSoundcard::SetInitialDevice()
{
	SetDevice(m_InitialDeviceIdentifier, true);
}


void COptionsSoundcard::SetDevice(SoundDevice::Identifier dev, bool forceReload)
{
	SoundDevice::Identifier olddev = m_CurrentDeviceInfo.GetIdentifier();
	SoundDevice::Info newInfo;
	SoundDevice::Caps newCaps;
	SoundDevice::DynamicCaps newDynamicCaps;
	SoundDevice::Settings newSettings;
	newInfo = theApp.GetSoundDevicesManager()->FindDeviceInfo(dev);
	newCaps = theApp.GetSoundDevicesManager()->GetDeviceCaps(dev, CMainFrame::GetMainFrame()->gpSoundDevice);
	newDynamicCaps = theApp.GetSoundDevicesManager()->GetDeviceDynamicCaps(dev, TrackerSettings::Instance().GetSampleRates(), CMainFrame::GetMainFrame(), CMainFrame::GetMainFrame()->gpSoundDevice, true);
	bool deviceChanged = (dev != olddev);
	if(deviceChanged || forceReload)
	{
		newSettings = TrackerSettings::Instance().GetSoundDeviceSettings(dev);
	} else
	{
		newSettings = m_Settings;
	}
	m_CurrentDeviceInfo = newInfo;
	m_CurrentDeviceCaps = newCaps;
	m_CurrentDeviceDynamicCaps = newDynamicCaps;
	m_Settings = newSettings;
}


void COptionsSoundcard::OnSoundCardShowAll()
{
	TrackerSettings::Instance().m_SoundShowDeprecatedDevices = (IsDlgButtonChecked(IDC_CHECK_SOUNDCARD_SHOWALL) == BST_CHECKED);
	SetDevice(m_CurrentDeviceInfo.GetIdentifier(), true);
	UpdateEverything();
}


void COptionsSoundcard::OnSoundCardRescan()
{
	{
		// Close sound device because IDs might change when re-enumerating which could cause all kinds of havoc.
		CMainFrame::GetMainFrame()->audioCloseDevice();
		delete CMainFrame::GetMainFrame()->gpSoundDevice;
		CMainFrame::GetMainFrame()->gpSoundDevice = nullptr;
	}
	theApp.GetSoundDevicesManager()->ReEnumerate();
	SetDevice(m_CurrentDeviceInfo.GetIdentifier(), true);
	UpdateEverything();
}


BOOL COptionsSoundcard::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	SetInitialDevice();
	UpdateEverything();
	return TRUE;
}


void COptionsSoundcard::UpdateLatency()
{
	{
		GetDlgItem(IDC_STATIC_LATENCY)->EnableWindow(TRUE);
		m_CbnLatencyMS.EnableWindow(TRUE);
	}
	// latency
	{
		static constexpr double latencies [] = {
			0.001,
			0.002,
			0.003,
			0.004,
			0.005,
			0.010,
			0.015,
			0.020,
			0.025,
			0.030,
			0.040,
			0.050,
			0.075,
			0.100,
			0.150,
			0.200,
			0.250
		};
		m_CbnLatencyMS.ResetContent();
		m_CbnLatencyMS.SetWindowText(PrintTime(m_Settings.Latency));
		for(auto lat : latencies)
		{
			if(m_CurrentDeviceCaps.LatencyMin <= lat && lat <= m_CurrentDeviceCaps.LatencyMax)
			{
				m_CbnLatencyMS.AddString(PrintTime(lat));
			}
		}
	}
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		GetDlgItem(IDC_STATIC_LATENCY)->EnableWindow(FALSE);
		m_CbnLatencyMS.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateUpdateInterval()
{
	{
		m_CbnUpdateIntervalMS.EnableWindow(TRUE);
	}
	// update interval
	{
		static constexpr double updateIntervals [] = {
			0.001,
			0.002,
			0.005,
			0.010,
			0.015,
			0.020,
			0.025,
			0.050
		};
		m_CbnUpdateIntervalMS.ResetContent();
		m_CbnUpdateIntervalMS.SetWindowText(PrintTime(m_Settings.UpdateInterval));
		for(auto upd : updateIntervals)
		{
			if(m_CurrentDeviceCaps.UpdateIntervalMin <= upd && upd <= m_CurrentDeviceCaps.UpdateIntervalMax)
			{
				m_CbnUpdateIntervalMS.AddString(PrintTime(upd));
			}
		}
	}
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()) || !m_CurrentDeviceCaps.CanUpdateInterval)
	{
		m_CbnUpdateIntervalMS.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateGeneral()
{
	// General
	{
		if(m_CurrentDeviceCaps.CanKeepDeviceRunning)
		{
			m_CbnStoppedMode.ResetContent();
			m_CbnStoppedMode.AddString(_T("Close driver"));
			m_CbnStoppedMode.AddString(_T("Pause driver"));
			m_CbnStoppedMode.AddString(_T("Play silence"));
			m_CbnStoppedMode.SetCurSel(TrackerSettings::Instance().m_SoundSettingsStopMode);
		} else
		{
			m_CbnStoppedMode.ResetContent();
			m_CbnStoppedMode.AddString(_T("Close driver"));
			m_CbnStoppedMode.AddString(_T("Close driver"));
			m_CbnStoppedMode.AddString(_T("Close driver"));
			m_CbnStoppedMode.SetCurSel(TrackerSettings::Instance().m_SoundSettingsStopMode);
		}
		CheckDlgButton(IDC_CHECK7, TrackerSettings::Instance().m_SoundSettingsOpenDeviceAtStartup ? BST_CHECKED : BST_UNCHECKED);
	}
	bool isUnavailble = theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier());
	m_CbnStoppedMode.EnableWindow(isUnavailble ? FALSE : (m_CurrentDeviceCaps.CanKeepDeviceRunning ? TRUE : FALSE));
	CPropertySheet *sheet = dynamic_cast<CPropertySheet *>(GetParent());
	if(sheet) sheet->GetDlgItem(IDOK)->EnableWindow(isUnavailble ? FALSE : TRUE);
}


void COptionsSoundcard::UpdateEverything()
{
	// Sound Device
	{
		if(m_CurrentDeviceInfo.IsDeprecated())
		{
			TrackerSettings::Instance().m_SoundShowDeprecatedDevices = true;
		}
		CheckDlgButton(IDC_CHECK_SOUNDCARD_SHOWALL, TrackerSettings::Instance().m_SoundShowDeprecatedDevices ? BST_CHECKED : BST_UNCHECKED);

		m_CbnDevice.ResetContent();
		m_CbnDevice.SetImageList(&CMainFrame::GetMainFrame()->m_MiscIcons);

		UINT iItem = 0;

		for(const auto &it : *theApp.GetSoundDevicesManager())
		{

			if(!TrackerSettings::Instance().m_SoundShowDeprecatedDevices)
			{
				if(it.IsDeprecated())
				{
					continue;
				}
			}

			{
				COMBOBOXEXITEM cbi;
				MemsetZero(cbi);
				cbi.iItem = iItem;
				cbi.cchTextMax = 0;
				cbi.mask = CBEIF_LPARAM | CBEIF_TEXT;
				cbi.lParam = theApp.GetSoundDevicesManager()->GetGlobalID(it.GetIdentifier());
				mpt::ustring TypeWineNative = U_("Wine-Native");
				if(it.type == SoundDevice::TypeWAVEOUT || it.type == SoundDevice::TypePORTAUDIO_WMME)
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_WAVEOUT;
				} else if(it.type == SoundDevice::TypeDSOUND || it.type == SoundDevice::TypePORTAUDIO_DS || it.type == U_("RtAudio-ds"))
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_DIRECTX;
				} else if(it.type == SoundDevice::TypeASIO || it.type == U_("RtAudio-asio"))
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_ASIO;
				} else if(it.type == SoundDevice::TypePORTAUDIO_WASAPI || it.type == U_("RtAudio-wasapi"))
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_SAMPLEMUTE; // // No real image available for now,
				} else if(it.type == SoundDevice::TypePORTAUDIO_WDMKS)
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_CHIP; // No real image available for now,
				} else if(it.type.find(TypeWineNative + U_("-")) == 0)
				{
					if(theApp.GetWineVersion() && (theApp.GetWineVersion()->HostClass() == mpt::OS::Class::Linux))
					{
						cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
						cbi.iImage = IMAGE_TUX;
					} else
					{
						cbi.iImage = 0;
					}
				} else
				{
					cbi.iImage = 0;
				}
				cbi.iSelectedImage = cbi.iImage;
				cbi.iOverlay = cbi.iImage;
				CString tmp = mpt::ToCString(it.GetDisplayName());
				cbi.pszText = const_cast<TCHAR *>(tmp.GetString());
				cbi.iIndent = 0;
				int pos = m_CbnDevice.InsertItem(&cbi);
				if(static_cast<SoundDevice::Manager::GlobalID>(cbi.lParam) == theApp.GetSoundDevicesManager()->GetGlobalID(m_CurrentDeviceInfo.GetIdentifier()))
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
{
	GetDlgItem(IDC_CHECK_SOUNDCARD_SHOWALL)->EnableWindow(m_CurrentDeviceInfo.IsDeprecated() ? FALSE : TRUE);
	UpdateGeneral();
	UpdateControls();
	UpdateLatency();
	UpdateUpdateInterval();
	UpdateSampleRates();
	UpdateChannels();
	UpdateSampleFormat();
	UpdateDither();
	UpdateChannelMapping();
	UpdateRecording();
}


void COptionsSoundcard::UpdateChannels()
{
	{
		m_CbnChannels.EnableWindow(TRUE);
	}
	m_CbnChannels.ResetContent();
	int maxChannels = 0;
	if(m_CurrentDeviceDynamicCaps.channelNames.size() > 0)
	{
		maxChannels = static_cast<int>(std::min(std::size_t(4), m_CurrentDeviceDynamicCaps.channelNames.size()));
	} else
	{
		maxChannels = 4;
	}
	int sel = 0;
	for(int channels = maxChannels; channels >= 1; channels /= 2)
	{
		int ndx = m_CbnChannels.AddString(gszChnCfgNames[(channels+2)/2-1]);
		m_CbnChannels.SetItemData(ndx, channels);
		if(channels == m_Settings.Channels)
		{
			sel = ndx;
		}
	}
	m_CbnChannels.SetCurSel(sel);
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		m_CbnChannels.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateRecording()
{
	GetDlgItem(IDC_STATIC_RECORDING)->ShowWindow(TrackerSettings::Instance().m_SoundShowRecordingSettings ? SW_SHOW : SW_HIDE);
	m_CbnRecordingChannels.ShowWindow(TrackerSettings::Instance().m_SoundShowRecordingSettings ? SW_SHOW : SW_HIDE);
	m_CbnRecordingSource.ShowWindow(TrackerSettings::Instance().m_SoundShowRecordingSettings ? SW_SHOW : SW_HIDE);
	m_CbnRecordingChannels.ResetContent();
	m_CbnRecordingSource.ResetContent();
	if(m_CurrentDeviceCaps.CanInput && ((m_CurrentDeviceCaps.HasNamedInputSources && m_CurrentDeviceDynamicCaps.inputSourceNames.size() > 0) || !m_CurrentDeviceCaps.HasNamedInputSources))
	{
		GetDlgItem(IDC_STATIC_RECORDING)->EnableWindow(TRUE);
		m_CbnRecordingChannels.EnableWindow(TRUE);
		int sel = 0;
		{
			int ndx = m_CbnRecordingChannels.AddString(_T("off"));
			m_CbnRecordingChannels.SetItemData(ndx, 0);
			if(0 == m_Settings.InputChannels)
			{
				sel = ndx;
			}
		}
		for(int channels = 4; channels >= 1; channels /= 2)
		{
			int ndx = m_CbnRecordingChannels.AddString(gszChnCfgNames[(channels+2)/2-1]);
			m_CbnRecordingChannels.SetItemData(ndx, channels);
			if(channels == m_Settings.InputChannels)
			{
				sel = ndx;
			}
		}
		m_CbnRecordingChannels.SetCurSel(sel);
		if(m_CurrentDeviceCaps.HasNamedInputSources)
		{
			m_CbnRecordingSource.EnableWindow((m_Settings.InputChannels > 0) ? TRUE : FALSE);
			sel = -1;
			for(size_t ch = 0; ch < m_CurrentDeviceDynamicCaps.inputSourceNames.size(); ch++)
			{
				const int pos = (int)::SendMessageW(m_CbnRecordingSource.m_hWnd, CB_ADDSTRING, 0, (LPARAM)m_CurrentDeviceDynamicCaps.inputSourceNames[ch].second.c_str());
				m_CbnRecordingSource.SetItemData(pos, (DWORD_PTR)m_CurrentDeviceDynamicCaps.inputSourceNames[ch].first);
				if(m_CurrentDeviceDynamicCaps.inputSourceNames[ch].first == m_Settings.InputSourceID)
				{
					sel = pos;
				}
			}
			if(sel == -1 ) sel = 0;
			m_CbnRecordingSource.SetCurSel(sel);
		} else
		{
			m_CbnRecordingSource.EnableWindow(FALSE);
		}
	} else
	{
		GetDlgItem(IDC_STATIC_RECORDING)->EnableWindow(FALSE);
		m_CbnRecordingChannels.EnableWindow(FALSE);
		int ndx = m_CbnRecordingChannels.AddString(_T("off"));
		m_CbnRecordingChannels.SetItemData(ndx, 0);
		m_CbnRecordingChannels.SetCurSel(ndx);
		m_CbnRecordingSource.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateSampleFormat()
{
	{
		m_CbnSampleFormat.EnableWindow(TRUE);
	}
	UINT n = 0;
	m_CbnSampleFormat.ResetContent();
	std::vector<SampleFormat> sampleformats;
	if(IsDlgButtonChecked(IDC_CHECK4))
	{
		sampleformats = m_CurrentDeviceDynamicCaps.supportedExclusiveModeSampleFormats;
	} else
	{
		sampleformats = m_CurrentDeviceDynamicCaps.supportedSampleFormats;
	}
	m_CbnSampleFormat.EnableWindow(m_CurrentDeviceCaps.CanSampleFormat && (sampleformats.size() != 1) ? TRUE : FALSE);
	const std::vector<SampleFormat> allSampleFormats = AllSampleFormats<std::vector<SampleFormat>>();
	for(const auto sampleFormat : allSampleFormats)
	{
		if(!sampleformats.empty() && !mpt::contains(sampleformats, sampleFormat))
		{
			continue;
		}
		CString name;
		if(sampleFormat.IsFloat())
		{
			name = MPT_CFORMAT("Float {} bit")(sampleFormat.GetBitsPerSample());
		} else if(sampleFormat.IsUnsigned())
		{
			name = MPT_CFORMAT("{} Bit uint")(sampleFormat.GetBitsPerSample());
		} else
		{
			name = MPT_CFORMAT("{} Bit")(sampleFormat.GetBitsPerSample());
		}
		UINT ndx = m_CbnSampleFormat.AddString(name);
		m_CbnSampleFormat.SetItemData(ndx, static_cast<int>(sampleFormat));
		if(sampleFormat == m_Settings.sampleFormat)
		{
			n = ndx;
		}
	}
	m_CbnSampleFormat.SetCurSel(n);
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		m_CbnSampleFormat.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateDither()
{
	{
		m_CbnDither.EnableWindow(TRUE);
	}
	m_CbnDither.ResetContent();
	SampleFormat sampleFormat = SampleFormat::FromInt(static_cast<int>(m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel())));
	if(sampleFormat.IsInt() && sampleFormat.GetBitsPerSample() < 32)
	{
		m_CbnDither.EnableWindow(TRUE);
		for(int i=0; i<NumDitherModes; ++i)
		{
			m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName((DitherMode)i) + U_(" dither")));
		}
	} else if(m_CurrentDeviceCaps.HasInternalDither)
	{
		m_CbnDither.EnableWindow(TRUE);
		m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherNone) + U_(" dither")));
		m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherDefault) + U_(" dither")));
	} else
	{
		m_CbnDither.EnableWindow(FALSE);
		for(int i=0; i<NumDitherModes; ++i)
		{
			m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherNone) + U_(" dither")));
		}
	}
	if(m_Settings.DitherType < 0 || m_Settings.DitherType >= m_CbnDither.GetCount())
	{
		m_CbnDither.SetCurSel(1);
	} else
	{
		m_CbnDither.SetCurSel(m_Settings.DitherType);
	}
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		m_CbnDither.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateChannelMapping()
{
	{
		GetDlgItem(IDC_STATIC_CHANNELMAPPING)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_CHANNEL_FRONT)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_CHANNEL_REAR)->EnableWindow(TRUE);
		for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)
		{
			CComboBox *combo = &m_CbnChannelMapping[mch];
			combo->EnableWindow(TRUE);
		}
	}
	int usedChannels = static_cast<int>(m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel()));
	if(m_Settings.Channels.GetNumHostChannels() != static_cast<uint32>(usedChannels))
	{
		// If the channel mapping is not valid for the selected number of channels, reset it to default identity mapping.
		m_Settings.Channels = SoundDevice::ChannelMapping(usedChannels);
	}
	GetDlgItem(IDC_STATIC_CHANNELMAPPING)->EnableWindow(m_CurrentDeviceCaps.CanChannelMapping ? TRUE : FALSE);
	if(m_CurrentDeviceCaps.CanChannelMapping && usedChannels > 2)
	{
		GetDlgItem(IDC_STATIC_CHANNEL_FRONT)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_CHANNEL_REAR)->EnableWindow(TRUE);
	} else
	{
		GetDlgItem(IDC_STATIC_CHANNEL_FRONT)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_CHANNEL_REAR)->EnableWindow(FALSE);
	}
	for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)	// Host channels
	{
		CComboBox *combo = &m_CbnChannelMapping[mch];
		combo->EnableWindow((m_CurrentDeviceCaps.CanChannelMapping && mch < usedChannels) ? TRUE : FALSE);
		combo->ResetContent();
		if(m_CurrentDeviceCaps.CanChannelMapping)
		{
			combo->SetItemData(combo->AddString(_T("Unassigned")), (DWORD_PTR)-1);
			combo->SetCurSel(0);
			if(mch < usedChannels)
			{
				for(size_t dch = 0; dch < m_CurrentDeviceDynamicCaps.channelNames.size(); dch++)	// Device channels
				{
					const int pos = (int)::SendMessageW(combo->m_hWnd, CB_ADDSTRING, 0, (LPARAM)m_CurrentDeviceDynamicCaps.channelNames[dch].c_str());
					combo->SetItemData(pos, (DWORD_PTR)dch);
					if(static_cast<int32>(dch) == m_Settings.Channels.ToDevice(mch))
					{
						combo->SetCurSel(pos);
					}
				}
			}
		}
	}
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		GetDlgItem(IDC_STATIC_CHANNELMAPPING)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_CHANNEL_FRONT)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_CHANNEL_REAR)->EnableWindow(FALSE);
		for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)
		{
			CComboBox *combo = &m_CbnChannelMapping[mch];
			combo->EnableWindow(FALSE);
		}
	}
}


void COptionsSoundcard::OnDeviceChanged()
{
	int n = m_CbnDevice.GetCurSel();
	if(n >= 0)
	{
		SetDevice(theApp.GetSoundDevicesManager()->FindDeviceInfo(static_cast<SoundDevice::Manager::GlobalID>(m_CbnDevice.GetItemData(n))).GetIdentifier());
		UpdateDevice();
		OnSettingsChanged();
	}
}


void COptionsSoundcard::OnExclusiveModeChanged()
{
	UpdateSampleRates();
	UpdateSampleFormat();
	UpdateDither();
	OnSettingsChanged();
}


void COptionsSoundcard::OnChannelsChanged()
{
	UpdateChannelMapping();
	OnSettingsChanged();
}


void COptionsSoundcard::OnSoundCardDriverPanel()
{
	theApp.GetSoundDevicesManager()->OpenDriverSettings(
		theApp.GetSoundDevicesManager()->FindDeviceInfo(static_cast<SoundDevice::Manager::GlobalID>(m_CbnDevice.GetItemData(m_CbnDevice.GetCurSel()))).GetIdentifier(),
		CMainFrame::GetMainFrame(),
		CMainFrame::GetMainFrame()->gpSoundDevice
		);
}


void COptionsSoundcard::OnChannelChanged(int channel)
{
	CComboBox *combo = &m_CbnChannelMapping[channel];
	const LONG_PTR newChn = combo->GetItemData(combo->GetCurSel());
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
				int deviceChannel = 0;
				for(; deviceChannel < static_cast<int>(m_CurrentDeviceDynamicCaps.channelNames.size()); ++deviceChannel)
				{
					bool used = false;
					for(int hostChannel = 0; hostChannel < NUM_CHANNELCOMBOBOXES; ++hostChannel)
					{
						if(static_cast<int>(m_CbnChannelMapping[hostChannel].GetItemData(m_CbnChannelMapping[hostChannel].GetCurSel())) == deviceChannel)
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
{
	{
		GetDlgItem(IDC_STATIC_FORMAT)->EnableWindow(TRUE);
		m_CbnMixingFreq.EnableWindow(TRUE);
	}

	m_CbnMixingFreq.ResetContent();

	std::vector<uint32> samplerates;

	if(IsDlgButtonChecked(IDC_CHECK4))
	{
		samplerates = m_CurrentDeviceDynamicCaps.supportedExclusiveSampleRates;
	} else
	{
		samplerates = m_CurrentDeviceDynamicCaps.supportedSampleRates;
	}

	if(samplerates.empty())
	{
		// We have no valid list of supported playback rates! Assume all rates supported by OpenMPT are possible...
		samplerates = TrackerSettings::Instance().GetSampleRates();
	}

	int n = 0;
	for(size_t i = 0; i < samplerates.size(); i++)
	{
		int pos = m_CbnMixingFreq.AddString(MPT_CFORMAT("{} Hz")(samplerates[i]));
		m_CbnMixingFreq.SetItemData(pos, samplerates[i]);
		if(m_Settings.Samplerate == samplerates[i])
		{
			n = pos;
		}
	}
	m_CbnMixingFreq.SetCurSel(n);
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		GetDlgItem(IDC_STATIC_FORMAT)->EnableWindow(FALSE);
		m_CbnMixingFreq.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateControls()
{
	{
		m_BtnDriverPanel.EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK4)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK5)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK9)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_UPDATEINTERVAL)->EnableWindow(TRUE);
		GetDlgItem(IDC_COMBO_UPDATEINTERVAL)->EnableWindow(TRUE);
	}
	if(!m_CurrentDeviceCaps.CanKeepDeviceRunning)
	{
		m_Settings.KeepDeviceRunning = false;
	}
	m_BtnDriverPanel.EnableWindow(m_CurrentDeviceCaps.CanDriverPanel ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK4)->EnableWindow(m_CurrentDeviceCaps.CanExclusiveMode ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK5)->EnableWindow(m_CurrentDeviceCaps.CanBoostThreadPriority ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK9)->EnableWindow(m_CurrentDeviceCaps.CanUseHardwareTiming ? TRUE : FALSE);
	GetDlgItem(IDC_STATIC_UPDATEINTERVAL)->EnableWindow(m_CurrentDeviceCaps.CanUpdateInterval ? TRUE : FALSE);
	GetDlgItem(IDC_COMBO_UPDATEINTERVAL)->EnableWindow(m_CurrentDeviceCaps.CanUpdateInterval ? TRUE : FALSE);
	GetDlgItem(IDC_CHECK4)->SetWindowText(mpt::ToCString(m_CurrentDeviceCaps.ExclusiveModeDescription));
	CheckDlgButton(IDC_CHECK4, m_CurrentDeviceCaps.CanExclusiveMode && m_Settings.ExclusiveMode ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK5, m_CurrentDeviceCaps.CanBoostThreadPriority && m_Settings.BoostThreadPriority ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_CHECK9, m_CurrentDeviceCaps.CanUseHardwareTiming && m_Settings.UseHardwareTiming ? BST_CHECKED : BST_UNCHECKED);
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		m_BtnDriverPanel.EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK5)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK9)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_UPDATEINTERVAL)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_UPDATEINTERVAL)->EnableWindow(FALSE);
	}
}


BOOL COptionsSoundcard::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_SOUNDCARD;
	return CPropertyPage::OnSetActive();
}


void COptionsSoundcard::OnOK()
{
	if(!theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{

	// General
	{
		TrackerSettings::Instance().m_SoundSettingsOpenDeviceAtStartup = IsDlgButtonChecked(IDC_CHECK7) != BST_UNCHECKED;
	}
	m_Settings.ExclusiveMode = IsDlgButtonChecked(IDC_CHECK4) != BST_UNCHECKED;
	m_Settings.BoostThreadPriority = IsDlgButtonChecked(IDC_CHECK5) != BST_UNCHECKED;
	m_Settings.UseHardwareTiming = IsDlgButtonChecked(IDC_CHECK9) != BST_UNCHECKED;
	// Mixing Freq
	{
		m_Settings.Samplerate = static_cast<uint32>(m_CbnMixingFreq.GetItemData(m_CbnMixingFreq.GetCurSel()));
	}
	// Channels
	{
		DWORD_PTR n = m_CbnChannels.GetItemData(m_CbnChannels.GetCurSel());
		m_Settings.Channels = static_cast<int>(n);
		if((m_Settings.Channels != 1) && (m_Settings.Channels != 4))
		{
			m_Settings.Channels = 2;
		}
	}
	// SampleFormat
	{
		DWORD_PTR n = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
		m_Settings.sampleFormat = SampleFormat::FromInt(static_cast<int>(n));
	}
	// Dither
	{
		UINT n = m_CbnDither.GetCurSel();
		m_Settings.DitherType = (DitherMode)(n);
	}
	// Latency
	{
		CString s;
		m_CbnLatencyMS.GetWindowText(s);
		m_Settings.Latency = ParseTime(s);
		//Check given value.
		if(m_Settings.Latency == 0.0) m_Settings.Latency = m_CurrentDeviceCaps.DefaultSettings.Latency;
		m_Settings.Latency = Clamp(m_Settings.Latency, m_CurrentDeviceCaps.LatencyMin, m_CurrentDeviceCaps.LatencyMax);
		m_CbnLatencyMS.SetWindowText(PrintTime(m_Settings.Latency));
	}
	// Update Interval
	{
		CString s;
		m_CbnUpdateIntervalMS.GetWindowText(s);
		m_Settings.UpdateInterval = ParseTime(s);
		//Check given value.
		if(m_Settings.UpdateInterval == 0.0) m_Settings.UpdateInterval = m_CurrentDeviceCaps.DefaultSettings.UpdateInterval;
		m_Settings.UpdateInterval = Clamp(m_Settings.UpdateInterval, m_CurrentDeviceCaps.UpdateIntervalMin, m_CurrentDeviceCaps.UpdateIntervalMax);
		m_CbnUpdateIntervalMS.SetWindowText(PrintTime(m_Settings.UpdateInterval));
	}
	// Channel Mapping
	{
		if(m_CurrentDeviceCaps.CanChannelMapping)
		{
			int numChannels = std::min(static_cast<int>(m_Settings.Channels), static_cast<int>(NUM_CHANNELCOMBOBOXES));
			std::vector<int32> channels(numChannels);
			for(int mch = 0; mch < numChannels; mch++)	// Host channels
			{
				CComboBox *combo = &m_CbnChannelMapping[mch];
				channels[mch] = static_cast<int32>(combo->GetItemData(combo->GetCurSel()));
			}
			m_Settings.Channels = channels;
		}
	}
	// Recording
	{
		if(TrackerSettings::Instance().m_SoundShowRecordingSettings && m_CurrentDeviceCaps.CanInput && ((m_CurrentDeviceCaps.HasNamedInputSources && m_CurrentDeviceDynamicCaps.inputSourceNames.size() > 0) || !m_CurrentDeviceCaps.HasNamedInputSources))
		{
			DWORD_PTR n = m_CbnRecordingChannels.GetItemData(m_CbnRecordingChannels.GetCurSel());
			m_Settings.InputChannels = static_cast<uint8>(n);
			if((m_Settings.InputChannels != 1) && (m_Settings.InputChannels != 2) && (m_Settings.InputChannels != 4))
			{
				m_Settings.InputChannels = 0;
			}
			if(m_CurrentDeviceCaps.HasNamedInputSources)
			{
				DWORD_PTR sourceID = m_CbnRecordingSource.GetItemData(m_CbnRecordingSource.GetCurSel());
				m_Settings.InputSourceID = static_cast<uint32>(sourceID);
			} else
			{
				m_Settings.InputSourceID = 0;
			}
		} else
		{
			m_Settings.InputChannels = 0;
			m_Settings.InputSourceID = 0;
		}
	}
	CMainFrame::GetMainFrame()->SetupSoundCard(m_Settings, m_CurrentDeviceInfo.GetIdentifier(), (SoundDeviceStopMode)m_CbnStoppedMode.GetCurSel());
	SetDevice(m_CurrentDeviceInfo.GetIdentifier(), true); // Poll changed ASIO sample format and channel names
	UpdateDevice();
	UpdateStatistics();

	} else
	{

		Reporting::Error("Sound card currently not available.");

	}

	CPropertyPage::OnOK();
}


void COptionsSoundcard::UpdateStatistics()
{
	if (!m_EditStatistics) return;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm->gpSoundDevice && pMainFrm->IsPlaying())
	{
		const SoundDevice::BufferAttributes bufferAttributes = pMainFrm->gpSoundDevice->GetEffectiveBufferAttributes();
		const SoundDevice::Statistics stats = pMainFrm->gpSoundDevice->GetStatistics();
		const uint32 samplerate = pMainFrm->gpSoundDevice->GetSettings().Samplerate;
		mpt::ustring s;
		if(bufferAttributes.NumBuffers > 2)
		{
			s += MPT_UFORMAT("Buffer: {}% ({}/{})\r\n")((bufferAttributes.Latency > 0.0) ? mpt::saturate_round<int64>(stats.InstantaneousLatency / bufferAttributes.Latency * 100.0) : 0, (stats.LastUpdateInterval > 0.0) ? mpt::saturate_round<int64>(bufferAttributes.Latency / stats.LastUpdateInterval) : 0, bufferAttributes.NumBuffers);
		} else
		{
			s += MPT_UFORMAT("Buffer: {}%\r\n")((bufferAttributes.Latency > 0.0) ? mpt::saturate_round<int64>(stats.InstantaneousLatency / bufferAttributes.Latency * 100.0) : 0);
		}
		s += MPT_UFORMAT("Latency: {} ms (current: {} ms, {} frames)\r\n")(mpt::ufmt::fix(bufferAttributes.Latency * 1000.0, 1), mpt::ufmt::fix(stats.InstantaneousLatency * 1000.0, 1), mpt::saturate_round<int64>(stats.InstantaneousLatency * samplerate));
		s += MPT_UFORMAT("Period: {} ms (current: {} ms, {} frames)\r\n")(mpt::ufmt::fix(bufferAttributes.UpdateInterval * 1000.0, 1), mpt::ufmt::fix(stats.LastUpdateInterval * 1000.0, 1), mpt::saturate_round<int64>(stats.LastUpdateInterval * samplerate));
		s += stats.text;
		m_EditStatistics.SetWindowText(mpt::ToCString(s));
	}	else
	{
		if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
		{
			m_EditStatistics.SetWindowText(_T("Device currently unavailable."));
		} else
		{
			m_EditStatistics.SetWindowText(_T(""));
		}
	}
}


//////////////////
// COptionsMixer

BEGIN_MESSAGE_MAP(COptionsMixer, CPropertyPage)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER,			&COptionsMixer::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_AMIGA_TYPE,		&COptionsMixer::OnSettingsChanged)
	ON_EN_UPDATE(IDC_RAMPING_IN,				&COptionsMixer::OnRampingChanged)
	ON_EN_UPDATE(IDC_RAMPING_OUT,				&COptionsMixer::OnRampingChanged)
	ON_COMMAND(IDC_CHECK_SOFTPAN,				&COptionsMixer::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,						&COptionsMixer::OnAmigaChanged)
END_MESSAGE_MAP()


void COptionsMixer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_COMBO_FILTER, m_CbnResampling);
	DDX_Control(pDX, IDC_COMBO_AMIGA_TYPE, m_CbnAmigaType);
	DDX_Control(pDX, IDC_RAMPING_IN, m_CEditRampUp);
	DDX_Control(pDX, IDC_RAMPING_OUT, m_CEditRampDown);
	DDX_Control(pDX, IDC_EDIT_VOLRAMP_SAMPLES_UP, m_CInfoRampUp);
	DDX_Control(pDX, IDC_EDIT_VOLRAMP_SAMPLES_DOWN, m_CInfoRampDown);
	DDX_Control(pDX, IDC_SLIDER_STEREOSEP, m_SliderStereoSep);
	// check box soft pan
	DDX_Control(pDX, IDC_SLIDER_PREAMP, m_SliderPreAmp);
	//}}AFX_DATA_MAP
}


BOOL COptionsMixer::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// Resampling type
	{
		const auto resamplingModes = Resampling::AllModes();
		for(auto mode : resamplingModes)
		{
			int index = m_CbnResampling.AddString(CTrackApp::GetResamplingModeName(mode, 2, true));
			m_CbnResampling.SetItemData(index, mode);
			if(TrackerSettings::Instance().ResamplerMode == mode)
			{
				m_CbnResampling.SetCurSel(index);
			}
		}
	}

	// Amiga Resampler
	const bool enableAmigaResampler = TrackerSettings::Instance().ResamplerEmulateAmiga != Resampling::AmigaFilter::Off;
	CheckDlgButton(IDC_CHECK1, enableAmigaResampler ? BST_CHECKED : BST_UNCHECKED);
	m_CbnAmigaType.EnableWindow(enableAmigaResampler ? TRUE : FALSE);
	static constexpr std::pair<const TCHAR *, Resampling::AmigaFilter> Filters[] =
	{
		{_T("A500 Filter"),  Resampling::AmigaFilter::A500},
		{_T("A1200 Filter"), Resampling::AmigaFilter::A1200},
		{_T("Unfiltered"),   Resampling::AmigaFilter::Unfiltered},
	};
	int sel = 0;
	for(const auto & [name, filter] : Filters)
	{
		const int item = m_CbnAmigaType.AddString(name);
		m_CbnAmigaType.SetItemData(item, static_cast<DWORD_PTR>(filter));
		if(filter == TrackerSettings::Instance().ResamplerEmulateAmiga)
			sel = item;
	}
	m_CbnAmigaType.SetCurSel(sel);

	// volume ramping
	{
		m_CEditRampUp.SetWindowText(mpt::ToCString(mpt::ufmt::val(TrackerSettings::Instance().GetMixerSettings().GetVolumeRampUpMicroseconds())));
		m_CEditRampDown.SetWindowText(mpt::ToCString(mpt::ufmt::val(TrackerSettings::Instance().GetMixerSettings().GetVolumeRampDownMicroseconds())));
		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN2))->SetRange32(0, int32_max);
		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN3))->SetRange32(0, int32_max);
		UpdateRamping();
	}

	// Stereo Separation
	{
		m_SliderStereoSep.SetRange(0, 32);
		m_SliderStereoSep.SetPos(16);
		for (int n = 0; n <= 32; n++)
		{
			if ((int)TrackerSettings::Instance().MixerStereoSeparation <= 8 * n)
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

	m_initialized = true;

	return TRUE;
}


BOOL COptionsMixer::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIXER;
	return CPropertyPage::OnSetActive();
}


void COptionsMixer::OnAmigaChanged()
{
	const bool enableAmigaResampler = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
	m_CbnAmigaType.EnableWindow(enableAmigaResampler ? TRUE : FALSE);
	OnSettingsChanged();
}


void COptionsMixer::OnRampingChanged()
{
	if(!m_initialized)
		return;
	UpdateRamping();
	OnSettingsChanged();
}


void COptionsMixer::OnHScroll(UINT n, UINT pos, CScrollBar *p)
{
	CPropertyPage::OnHScroll(n, pos, p);
	if(p == (CScrollBar *)&m_SliderStereoSep)
	{
		UpdateStereoSep();
		OnSettingsChanged();
	}
}


void COptionsMixer::UpdateRamping()
{
	MixerSettings settings = TrackerSettings::Instance().GetMixerSettings();
	CString s;
	m_CEditRampUp.GetWindowText(s);
	settings.SetVolumeRampUpMicroseconds(ConvertStrTo<int32>(s));
	m_CEditRampDown.GetWindowText(s);
	settings.SetVolumeRampDownMicroseconds(ConvertStrTo<int32>(s));
	s.Format(_T("%i samples at %i Hz"), (int)settings.GetVolumeRampUpSamples(), (int)settings.gdwMixingFreq);
	m_CInfoRampUp.SetWindowText(s);
	s.Format(_T("%i samples at %i Hz"), (int)settings.GetVolumeRampDownSamples(), (int)settings.gdwMixingFreq);
	m_CInfoRampDown.SetWindowText(s);
}


void COptionsMixer::UpdateStereoSep()
{
	CString s;
	s.Format(_T("%d%%"), ((8 * m_SliderStereoSep.GetPos()) * 100) / 128);
	SetDlgItemText(IDC_TEXT_STEREOSEP, s);
}


void COptionsMixer::OnOK()
{
	// resampler mode
	{
		TrackerSettings::Instance().ResamplerMode = static_cast<ResamplingMode>(m_CbnResampling.GetItemData(m_CbnResampling.GetCurSel()));
	}

	// Amiga Resampler
	if(IsDlgButtonChecked(IDC_CHECK1) == BST_UNCHECKED)
		TrackerSettings::Instance().ResamplerEmulateAmiga = Resampling::AmigaFilter::Off;
	else
		TrackerSettings::Instance().ResamplerEmulateAmiga = static_cast<Resampling::AmigaFilter>(m_CbnAmigaType.GetItemData(m_CbnAmigaType.GetCurSel()));

	// volume ramping
	{
		MixerSettings settings = TrackerSettings::Instance().GetMixerSettings();
		CString s;
		m_CEditRampUp.GetWindowText(s);
		settings.SetVolumeRampUpMicroseconds(ConvertStrTo<int>(s));
		m_CEditRampDown.GetWindowText(s);
		settings.SetVolumeRampDownMicroseconds(ConvertStrTo<int>(s));
		TrackerSettings::Instance().SetMixerSettings(settings);
	}

	// stereo sep
	{
		TrackerSettings::Instance().MixerStereoSeparation = 8 * m_SliderStereoSep.GetPos();
	}

	// soft pan
	{
		if(IsDlgButtonChecked(IDC_CHECK_SOFTPAN))
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
	CMainFrame::GetMainFrame()->PostMessage(WM_MOD_INVALIDATEPATTERNS, HINT_MPTOPTIONS);

	CPropertyPage::OnOK();
}


////////////////////////////////////////////////////////////////////////////////
//
// CEQSavePresetDlg
//

#ifndef NO_EQ

class CEQSavePresetDlg: public CDialog
{
protected:
	EQPreset &m_EQ;

public:
	CEQSavePresetDlg(EQPreset &eq, CWnd *parent = nullptr) : CDialog(IDD_SAVEPRESET, parent), m_EQ(eq) { }
	BOOL OnInitDialog();
	void OnOK();
};


BOOL CEQSavePresetDlg::OnInitDialog()
{
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (pCombo)
	{
		int ndx = 0;
		for (UINT i=0; i<4; i++)
		{
			int n = pCombo->AddString(mpt::ToCString(mpt::Charset::Locale, TrackerSettings::Instance().m_EqUserPresets[i].szName));
			pCombo->SetItemData( n, i);
			if (!lstrcmpiA(TrackerSettings::Instance().m_EqUserPresets[i].szName, m_EQ.szName)) ndx = n;
		}
		pCombo->SetCurSel(ndx);
	}
	SetDlgItemText(IDC_EDIT1, mpt::ToCString(mpt::Charset::Locale, m_EQ.szName));
	return TRUE;
}


void CEQSavePresetDlg::OnOK()
{
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (pCombo)
	{
		int n = pCombo->GetCurSel();
		if ((n < 0) || (n >= 4)) n = 0;
		CString s;
		GetDlgItemText(IDC_EDIT1, s);
		mpt::String::WriteAutoBuf(m_EQ.szName) = mpt::ToCharset(mpt::Charset::Locale, s);
		TrackerSettings::Instance().m_EqUserPresets[n] = m_EQ;
	}
	CDialog::OnOK();
}


void CEQSlider::Init(UINT nID, UINT n, CWnd *parent)
{
	m_nSliderNo = n;
	m_pParent = parent;
	SubclassDlgItem(nID, parent);
}


BOOL CEQSlider::PreTranslateMessage(MSG *pMsg)
{
	if ((pMsg) && (pMsg->message == WM_RBUTTONDOWN) && (m_pParent))
	{
		m_x = LOWORD(pMsg->lParam);
		m_y = HIWORD(pMsg->lParam);
		m_pParent->PostMessage(WM_COMMAND, ID_EQSLIDER_BASE+m_nSliderNo, 0);
	}
	return CSliderCtrl::PreTranslateMessage(pMsg);
}

#endif // !NO_EQ


//////////////////////////////////////////////////////////
// COptionsPlayer - DSP / EQ settings


#ifndef NO_EQ
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
#endif // !NO_EQ

BEGIN_MESSAGE_MAP(COptionsPlayer, CPropertyPage)
#ifndef NO_EQ
	// EQ
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_CHECK3,	&COptionsPlayer::OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,	&COptionsPlayer::OnEqUser1)
	ON_COMMAND(IDC_BUTTON2,	&COptionsPlayer::OnEqUser2)
	ON_COMMAND(IDC_BUTTON3,	&COptionsPlayer::OnEqUser3)
	ON_COMMAND(IDC_BUTTON4,	&COptionsPlayer::OnEqUser4)
	ON_COMMAND(IDC_BUTTON5,	&COptionsPlayer::OnSavePreset)
	ON_COMMAND_RANGE(ID_EQSLIDER_BASE, ID_EQSLIDER_BASE + MAX_EQ_BANDS,	&COptionsPlayer::OnSliderMenu)
	ON_COMMAND_RANGE(ID_EQMENU_BASE, ID_EQMENU_BASE + EQ_MAX_FREQS,		&COptionsPlayer::OnSliderFreq)
#endif // !NO_EQ

	// DSP
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO2,	&COptionsPlayer::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,			&COptionsPlayer::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,			&COptionsPlayer::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,			&COptionsPlayer::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK5,			&COptionsPlayer::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK6,			&COptionsPlayer::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK7,			&COptionsPlayer::OnSettingsChanged)
END_MESSAGE_MAP()


void COptionsPlayer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsPlayer)
	DDX_Control(pDX, IDC_COMBO2,		m_CbnReverbPreset);
	DDX_Control(pDX, IDC_SLIDER1,		m_SbXBassDepth);
	DDX_Control(pDX, IDC_SLIDER2,		m_SbXBassRange);
	DDX_Control(pDX, IDC_SLIDER3,		m_SbReverbDepth);
	DDX_Control(pDX, IDC_SLIDER5,		m_SbSurroundDepth);
	DDX_Control(pDX, IDC_SLIDER6,		m_SbSurroundDelay);
	DDX_Control(pDX, IDC_SLIDER4,		m_SbBitCrushBits);
	//}}AFX_DATA_MAP
}


BOOL COptionsPlayer::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	uint32 dwQuality = TrackerSettings::Instance().MixerDSPMask;

#ifndef NO_EQ
	for (UINT i = 0; i < MAX_EQ_BANDS; i++)
	{
		m_Sliders[i].Init(IDC_SLIDER7 + i, i, this);
		m_Sliders[i].SetRange(0, 32);
		m_Sliders[i].SetTicFreq(4);
	}

	UpdateDialog();

	if (dwQuality & SNDDSP_EQ) CheckDlgButton(IDC_CHECK3, BST_CHECKED);
#else
	GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
#endif

	// Effects
#ifndef NO_DSP
	if (dwQuality & SNDDSP_MEGABASS) CheckDlgButton(IDC_CHECK1, BST_CHECKED);
#else
	GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
#endif
#ifndef NO_AGC
	if (dwQuality & SNDDSP_AGC) CheckDlgButton(IDC_CHECK2, BST_CHECKED);
#else
	GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
#endif
#ifndef NO_DSP
	if (dwQuality & SNDDSP_SURROUND) CheckDlgButton(IDC_CHECK4, BST_CHECKED);
#else
	GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
#endif
#ifndef NO_DSP
	if (dwQuality & SNDDSP_BITCRUSH) CheckDlgButton(IDC_CHECK5, BST_CHECKED);
#else
	GetDlgItem(IDC_CHECK5)->EnableWindow(FALSE);
#endif

#ifndef NO_DSP
	m_SbBitCrushBits.SetRange(1, 24);
	m_SbBitCrushBits.SetPos(TrackerSettings::Instance().m_BitCrushSettings.m_Bits);
#else
	m_SbBitCurshBits.EnableWindow(FALSE);
#endif

#ifndef NO_DSP
	// Bass Expansion
	m_SbXBassDepth.SetRange(0,4);
	m_SbXBassDepth.SetPos(8-TrackerSettings::Instance().m_MegaBassSettings.m_nXBassDepth);
	m_SbXBassRange.SetRange(0,4);
	m_SbXBassRange.SetPos(4 - (TrackerSettings::Instance().m_MegaBassSettings.m_nXBassRange - 1) / 5);
#else
	m_SbXBassDepth.EnableWindow(FALSE);
	m_SbXBassRange.EnableWindow(FALSE);
#endif

#ifndef NO_REVERB
	// Reverb
	m_SbReverbDepth.SetRange(1, 16);
	m_SbReverbDepth.SetPos(TrackerSettings::Instance().m_ReverbSettings.m_nReverbDepth);
	UINT nSel = 0;
	for (UINT iRvb=0; iRvb<NUM_REVERBTYPES; iRvb++)
	{
		CString pszName = mpt::ToCString(GetReverbPresetName(iRvb));
		if(!pszName.IsEmpty())
		{
			UINT n = m_CbnReverbPreset.AddString(pszName);
			m_CbnReverbPreset.SetItemData(n, iRvb);
			if (iRvb == TrackerSettings::Instance().m_ReverbSettings.m_nReverbType) nSel = n;
		}
	}
	m_CbnReverbPreset.SetCurSel(nSel);
	if (dwQuality & SNDDSP_REVERB) CheckDlgButton(IDC_CHECK6, BST_CHECKED);
#else
	GetDlgItem(IDC_CHECK6)->EnableWindow(FALSE);
	m_SbReverbDepth.EnableWindow(FALSE);
	m_CbnReverbPreset.EnableWindow(FALSE);
#endif

#ifndef NO_DSP
	// Surround
	{
		UINT n = TrackerSettings::Instance().m_SurroundSettings.m_nProLogicDepth;
		if (n < 1) n = 1;
		if (n > 16) n = 16;
		m_SbSurroundDepth.SetRange(1, 16);
		m_SbSurroundDepth.SetPos(n);
		m_SbSurroundDelay.SetRange(0, 8);
		m_SbSurroundDelay.SetPos((TrackerSettings::Instance().m_SurroundSettings.m_nProLogicDelay-5)/5);
	}
#else
	m_SbSurroundDepth.EnableWindow(FALSE);
	m_SbSurroundDelay.EnableWindow(FALSE);
#endif

	return TRUE;
}


BOOL COptionsPlayer::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_PLAYER;

	SetDlgItemText(IDC_EQ_WARNING,
		_T("Note: This EQ is applied to any and all of the modules ")
		_T("that you load in OpenMPT; its settings are stored globally, ")
		_T("rather than in each file. This means that you should avoid ")
		_T("using it as part of your production process, and instead only ")
		_T("use it to correct deficiencies in your audio hardware."));

	return CPropertyPage::OnSetActive();
}


void COptionsPlayer::OnHScroll(UINT nSBCode, UINT, CScrollBar *psb)
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
	if (IsDlgButtonChecked(IDC_CHECK4)) dwQuality |= SNDDSP_SURROUND;
#endif
#ifndef NO_REVERB
	dwQualityMask |= SNDDSP_REVERB;
	if (IsDlgButtonChecked(IDC_CHECK6)) dwQuality |= SNDDSP_REVERB;
#endif
#ifndef NO_DSP
	dwQualityMask |= SNDDSP_BITCRUSH;
	if (IsDlgButtonChecked(IDC_CHECK5)) dwQuality |= SNDDSP_BITCRUSH;
#endif

#ifndef NO_DSP
	{
		TrackerSettings::Instance().m_BitCrushSettings.m_Bits = m_SbBitCrushBits.GetPos();
	}
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
		TrackerSettings::Instance().m_MegaBassSettings.m_nXBassDepth = nXBassDepth;
		TrackerSettings::Instance().m_MegaBassSettings.m_nXBassRange = nXBassRange;
	}
#endif
#ifndef NO_REVERB
	// Reverb
	{
		// Reverb depth is dynamically changed
		uint32 nReverbType = static_cast<uint32>(m_CbnReverbPreset.GetItemData(m_CbnReverbPreset.GetCurSel()));
		if (nReverbType < NUM_REVERBTYPES) TrackerSettings::Instance().m_ReverbSettings.m_nReverbType = nReverbType;
	}
#endif
#ifndef NO_DSP
	// Surround
	{
		UINT nProLogicDepth = m_SbSurroundDepth.GetPos();
		UINT nProLogicDelay = 5 + (m_SbSurroundDelay.GetPos() * 5);
		TrackerSettings::Instance().m_SurroundSettings.m_nProLogicDepth = nProLogicDepth;
		TrackerSettings::Instance().m_SurroundSettings.m_nProLogicDelay = nProLogicDelay;
	}
#endif

	TrackerSettings::Instance().MixerDSPMask = dwQuality;

	CMainFrame::GetMainFrame()->SetupPlayer();
	CPropertyPage::OnOK();
}


#ifndef NO_EQ

void COptionsPlayer::UpdateEQ(bool bReset)
{
	CriticalSection cs;
	if(CMainFrame::GetMainFrame()->GetSoundFilePlaying())
		CMainFrame::GetMainFrame()->GetSoundFilePlaying()->SetEQGains(m_EQPreset.Gains, MAX_EQ_BANDS, m_EQPreset.Freqs, bReset);
}


void COptionsPlayer::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		int n = 32 - m_Sliders[i].GetPos();
		if ((n >= 0) && (n <= 32)) m_EQPreset.Gains[i] = n;
	}
	UpdateEQ(FALSE);
}


void COptionsPlayer::LoadEQPreset(const EQPreset &preset)
{
	m_EQPreset = preset;
	UpdateEQ(TRUE);
	UpdateDialog();
}


void COptionsPlayer::OnSavePreset()
{
	CEQSavePresetDlg dlg(m_EQPreset, this);
	if (dlg.DoModal() == IDOK)
	{
		UpdateDialog();
	}
}


static CString f2s(UINT f)
{
	if (f < 1000)
	{
		return MPT_CFORMAT("{}Hz")(f);
	} else
	{
		UINT fHi = f / 1000u;
		UINT fLo = f % 1000u;
		if(fLo)
		{
			return MPT_CFORMAT("{}.{}kHz")(fHi, mpt::cfmt::dec0<1>(fLo/100));
		} else
		{
			return MPT_CFORMAT("{}kHz")(fHi);
		}
	}
}


void COptionsPlayer::UpdateDialog()
{
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		int n = 32 - m_EQPreset.Gains[i];
		if (n < 0) n = 0;
		if (n > 32) n = 32;
		if (n != (m_Sliders[i].GetPos() & 0xFFFF)) m_Sliders[i].SetPos(n);
		SetDlgItemText(IDC_TEXT1 + i, f2s(m_EQPreset.Freqs[i]));
	}
	for(unsigned int i = 0; i < std::size(TrackerSettings::Instance().m_EqUserPresets); i++)
	{
		SetDlgItemText(IDC_BUTTON1 + i, mpt::ToCString(mpt::Charset::Locale, TrackerSettings::Instance().m_EqUserPresets[i].szName));
	}
}


void COptionsPlayer::OnSliderMenu(UINT nID)
{
	UINT n = nID - ID_EQSLIDER_BASE;
	if (n < MAX_EQ_BANDS)
	{
		HMENU hMenu = ::CreatePopupMenu();
		m_nSliderMenu = n;
		if (!hMenu) return;
		const UINT *pFreqs = gEqBandFreqs[m_nSliderMenu];
		for (UINT i = 0; i < EQ_MAX_FREQS; i++)
		{
			DWORD d = MF_STRING;
			if (m_EQPreset.Freqs[m_nSliderMenu] == pFreqs[i]) d |= MF_CHECKED;
			::AppendMenu(hMenu, d, ID_EQMENU_BASE+i, f2s(pFreqs[i]));
		}
		CPoint pt(m_Sliders[m_nSliderMenu].m_x, m_Sliders[m_nSliderMenu].m_y);
		m_Sliders[m_nSliderMenu].ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
		::DestroyMenu(hMenu);
	}
}


void COptionsPlayer::OnSliderFreq(UINT nID)
{
	UINT n = nID - ID_EQMENU_BASE;
	if ((m_nSliderMenu < MAX_EQ_BANDS) && (n < EQ_MAX_FREQS))
	{
		UINT f = gEqBandFreqs[m_nSliderMenu][n];
		if (f != m_EQPreset.Freqs[m_nSliderMenu])
		{
			m_EQPreset.Freqs[m_nSliderMenu] = f;
			UpdateEQ(TRUE);
			UpdateDialog();
		}
	}
}

#endif // !NO_EQ


/////////////////////////////////////////////////////////////
// CMidiSetupDlg

BEGIN_MESSAGE_MAP(CMidiSetupDlg, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO1,			&CMidiSetupDlg::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,			&CMidiSetupDlg::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3,			&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,					&CMidiSetupDlg::OnRenameDevice)
	ON_COMMAND(IDC_CHECK1,					&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,					&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK3,					&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,					&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK5,					&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_MIDI_TO_PLUGIN,			&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_MIDI_MACRO_CONTROL,		&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_MIDIVOL_TO_NOTEVOL,		&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_MIDIPLAYCONTROL,			&CMidiSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_MIDIPLAYPATTERNONMIDIIN,	&CMidiSetupDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT1,					&CMidiSetupDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT2,					&CMidiSetupDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT3,					&CMidiSetupDlg::OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT4,					&CMidiSetupDlg::OnSettingsChanged)
END_MESSAGE_MAP()


void CMidiSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSoundcard)
	DDX_Control(pDX, IDC_SPIN1,		m_SpinSpd);
	DDX_Control(pDX, IDC_SPIN2,		m_SpinPat);
	DDX_Control(pDX, IDC_SPIN3,		m_SpinAmp);
	DDX_Control(pDX, IDC_COMBO1,	m_InputDevice);
	DDX_Control(pDX, IDC_COMBO2,	m_ATBehaviour);
	DDX_Control(pDX, IDC_COMBO3,	m_Quantize);
	//}}AFX_DATA_MAP
}


BOOL CMidiSetupDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	// Flags
	if (m_dwMidiSetup & MIDISETUP_RECORDVELOCITY) CheckDlgButton(IDC_CHECK1, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_RECORDNOTEOFF) CheckDlgButton(IDC_CHECK2, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_ENABLE_RECORD_DEFAULT) CheckDlgButton(IDC_CHECK3, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_TRANSPOSEKEYBOARD) CheckDlgButton(IDC_CHECK4, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_MIDITOPLUG) CheckDlgButton(IDC_MIDI_TO_PLUGIN, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_MIDIMACROCONTROL) CheckDlgButton(IDC_MIDI_MACRO_CONTROL, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_MIDIVOL_TO_NOTEVOL) CheckDlgButton(IDC_MIDIVOL_TO_NOTEVOL, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_RESPONDTOPLAYCONTROLMSGS) CheckDlgButton(IDC_MIDIPLAYCONTROL, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_PLAYPATTERNONMIDIIN) CheckDlgButton(IDC_MIDIPLAYPATTERNONMIDIIN, BST_CHECKED);
	if (m_dwMidiSetup & MIDISETUP_MIDIMACROPITCHBEND) CheckDlgButton(IDC_CHECK5, BST_CHECKED);

	// Midi In Device
	RefreshDeviceList(m_nMidiDevice);

	// Aftertouch behaviour
	m_ATBehaviour.ResetContent();
	static constexpr std::pair<const TCHAR *, RecordAftertouchOptions> aftertouchOptions[] =
	{
		{ _T("Do not record Aftertouch"), atDoNotRecord },
		{ _T("Record as Volume Commands"), atRecordAsVolume },
		{ _T("Record as MIDI Macros"), atRecordAsMacro },
	};

	for(const auto & [str, value] : aftertouchOptions)
	{
		int item = m_ATBehaviour.AddString(str);
		m_ATBehaviour.SetItemData(item, value);
		if(value == TrackerSettings::Instance().aftertouchBehaviour)
		{
			m_ATBehaviour.SetCurSel(item);
		}
	}

	// Note Velocity amp
	SetDlgItemInt(IDC_EDIT3, TrackerSettings::Instance().midiVelocityAmp);
	m_SpinAmp.SetRange(1, 10000);

	SetDlgItemText(IDC_EDIT4, mpt::ToCString(IgnoredCCsToString(TrackerSettings::Instance().midiIgnoreCCs)));

	// Midi Import settings
	SetDlgItemInt(IDC_EDIT1, TrackerSettings::Instance().midiImportTicks);
	SetDlgItemInt(IDC_EDIT2, TrackerSettings::Instance().midiImportPatternLen);

	// Note quantization
	m_Quantize.ResetContent();
	static constexpr std::pair<const TCHAR *, uint32> quantizeOptions[] =
	{
		{ _T("1/4th Notes"),  4 },  { _T("1/6th Notes"),  6 },
		{ _T("1/8th Notes"),  8 },  { _T("1/12th Notes"), 12 },
		{ _T("1/16th Notes"), 16 }, { _T("1/24th Notes"), 24 },
		{ _T("1/32nd Notes"), 32 }, { _T("1/48th Notes"), 48 },
		{ _T("1/64th Notes"), 64 }, { _T("1/96th Notes"), 96 },
	};

	for(const auto & [str, value]: quantizeOptions)
	{
		int item = m_Quantize.AddString(str);
		m_Quantize.SetItemData(item, value);
		if(value == TrackerSettings::Instance().midiImportQuantize)
		{
			m_Quantize.SetCurSel(item);
		}
	}
	m_SpinSpd.SetRange(2, 16);
	m_SpinPat.SetRange(1, MAX_PATTERN_ROWS);
	return TRUE;
}


void CMidiSetupDlg::RefreshDeviceList(UINT currentDevice)
{
	m_InputDevice.SetRedraw(FALSE);
	m_InputDevice.ResetContent();
	UINT ndevs = midiInGetNumDevs();
	for(UINT i = 0; i < ndevs; i++)
	{
		MIDIINCAPS mic;
		mic.szPname[0] = 0;
		if(midiInGetDevCaps(i, &mic, sizeof(mic)) == MMSYSERR_NOERROR)
		{
			int item = m_InputDevice.AddString(theApp.GetFriendlyMIDIPortName(mpt::ToCString(mpt::String::ReadWinBuf(mic.szPname)), true));
			m_InputDevice.SetItemData(item, i);
			if(i == currentDevice)
			{
				m_InputDevice.SetCurSel(item);
			}
		}
	}
	m_InputDevice.SetRedraw(TRUE);
	m_InputDevice.Invalidate(FALSE);
}


void CMidiSetupDlg::OnRenameDevice()
{
	int n = m_InputDevice.GetCurSel();
	if(n >= 0)
	{
		UINT device = static_cast<UINT>(m_InputDevice.GetItemData(n));
		MIDIINCAPS mic;
		mic.szPname[0] = 0;
		midiInGetDevCaps(device, &mic, sizeof(mic));
		CString name = mic.szPname;
		CString friendlyName = theApp.GetSettings().Read(U_("MIDI Input Ports"), mpt::ToUnicode(name), name);
		CInputDlg dlg(this, _T("New name for ") + name + _T(":"), friendlyName);
		if(dlg.DoModal() == IDOK)
		{
			if(dlg.resultAsString.IsEmpty() || dlg.resultAsString == name)
				theApp.GetSettings().Remove(U_("MIDI Input Ports"), mpt::ToUnicode(name));
			else
				theApp.GetSettings().Write(U_("MIDI Input Ports"), mpt::ToUnicode(name), dlg.resultAsString);
			RefreshDeviceList(device);
		}
	}
}


void CMidiSetupDlg::OnOK()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	m_dwMidiSetup = 0;
	m_nMidiDevice = MIDI_MAPPER;
	if (IsDlgButtonChecked(IDC_CHECK1)) m_dwMidiSetup |= MIDISETUP_RECORDVELOCITY;
	if (IsDlgButtonChecked(IDC_CHECK2)) m_dwMidiSetup |= MIDISETUP_RECORDNOTEOFF;
	if (IsDlgButtonChecked(IDC_CHECK3)) m_dwMidiSetup |= MIDISETUP_ENABLE_RECORD_DEFAULT;
	if (IsDlgButtonChecked(IDC_CHECK4)) m_dwMidiSetup |= MIDISETUP_TRANSPOSEKEYBOARD;
	if (IsDlgButtonChecked(IDC_MIDI_TO_PLUGIN)) m_dwMidiSetup |= MIDISETUP_MIDITOPLUG;
	if (IsDlgButtonChecked(IDC_MIDI_MACRO_CONTROL)) m_dwMidiSetup |= MIDISETUP_MIDIMACROCONTROL;
	if (IsDlgButtonChecked(IDC_MIDIVOL_TO_NOTEVOL)) m_dwMidiSetup |= MIDISETUP_MIDIVOL_TO_NOTEVOL;
	if (IsDlgButtonChecked(IDC_MIDIPLAYCONTROL)) m_dwMidiSetup |= MIDISETUP_RESPONDTOPLAYCONTROLMSGS;
	if (IsDlgButtonChecked(IDC_MIDIPLAYPATTERNONMIDIIN)) m_dwMidiSetup |= MIDISETUP_PLAYPATTERNONMIDIIN;
	if (IsDlgButtonChecked(IDC_CHECK5)) m_dwMidiSetup |= MIDISETUP_MIDIMACROPITCHBEND;

	int n = m_InputDevice.GetCurSel();
	if (n >= 0) m_nMidiDevice = static_cast<UINT>(m_InputDevice.GetItemData(n));

	TrackerSettings::Instance().aftertouchBehaviour = static_cast<RecordAftertouchOptions>(m_ATBehaviour.GetItemData(m_ATBehaviour.GetCurSel()));

	TrackerSettings::Instance().midiVelocityAmp = static_cast<uint16>(Clamp(GetDlgItemInt(IDC_EDIT3), 1u, 10000u));

	CString cc;
	GetDlgItemText(IDC_EDIT4, cc);
	TrackerSettings::Instance().midiIgnoreCCs = StringToIgnoredCCs(mpt::ToUnicode(cc));

	TrackerSettings::Instance().midiImportTicks = static_cast<uint8>(Clamp(GetDlgItemInt(IDC_EDIT1), uint8(2), uint8(16)));
	TrackerSettings::Instance().midiImportPatternLen = Clamp(GetDlgItemInt(IDC_EDIT2), ROWINDEX(1), MAX_PATTERN_ROWS);
	if(m_Quantize.GetCurSel() != -1)
	{
		TrackerSettings::Instance().midiImportQuantize = static_cast<uint32>(m_Quantize.GetItemData(m_Quantize.GetCurSel()));
	}

	if (pMainFrm) pMainFrm->SetupMidi(m_dwMidiSetup, m_nMidiDevice);
	CPropertyPage::OnOK();
}


BOOL CMidiSetupDlg::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIDI;
	return CPropertyPage::OnSetActive();
}


// Wine


BEGIN_MESSAGE_MAP(COptionsWine, CPropertyPage)
	ON_COMMAND(IDC_CHECK_WINE_ENABLE, &COptionsWine::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_WINE_PULSEAUDIO, &COptionsWine::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_WINE_PORTAUDIO, &COptionsWine::OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_WINE_RTAUDIO, &COptionsWine::OnSettingsChanged)
END_MESSAGE_MAP()


COptionsWine::COptionsWine()
	: CPropertyPage(IDD_OPTIONS_WINE)
{
	return;
}


void COptionsWine::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsWine)
	DDX_Control(pDX, IDC_COMBO_WINE_PULSEAUDIO, m_CbnPulseAudio);
	DDX_Control(pDX, IDC_COMBO_WINE_PORTAUDIO, m_CbnPortAudio);
	DDX_Control(pDX, IDC_COMBO_WINE_RTAUDIO, m_CbnRtAudio);
	//}}AFX_DATA_MAP
}


BOOL COptionsWine::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	GetDlgItem(IDC_CHECK_WINE_ENABLE)->EnableWindow(mpt::OS::Windows::IsWine() ? TRUE : FALSE);
	CheckDlgButton(IDC_CHECK_WINE_ENABLE, TrackerSettings::Instance().WineSupportEnabled ? BST_CHECKED : BST_UNCHECKED);
	int index;
	index = m_CbnPulseAudio.AddString(_T("Auto"    )); m_CbnPulseAudio.SetItemData(index, 1);
	index = m_CbnPulseAudio.AddString(_T("Enabled" )); m_CbnPulseAudio.SetItemData(index, 2);
	index = m_CbnPulseAudio.AddString(_T("Disabled")); m_CbnPulseAudio.SetItemData(index, 0);
	m_CbnPulseAudio.SetCurSel(0);
	for(index = 0; index < 3; ++index)
	{
		if(m_CbnPulseAudio.GetItemData(index) == static_cast<uint32>(TrackerSettings::Instance().WineSupportEnablePulseAudio))
		{
			m_CbnPulseAudio.SetCurSel(index);
		}
	}
	index = m_CbnPortAudio.AddString(_T("Auto"    )); m_CbnPortAudio.SetItemData(index, 1);
	index = m_CbnPortAudio.AddString(_T("Enabled" )); m_CbnPortAudio.SetItemData(index, 2);
	index = m_CbnPortAudio.AddString(_T("Disabled")); m_CbnPortAudio.SetItemData(index, 0);
	for(index = 0; index < 3; ++index)
	{
		if(m_CbnPortAudio.GetItemData(index) == static_cast<uint32>(TrackerSettings::Instance().WineSupportEnablePortAudio))
		{
			m_CbnPortAudio.SetCurSel(index);
		}
	}
	index = m_CbnRtAudio.AddString(_T("Auto"    )); m_CbnRtAudio.SetItemData(index, 1);
	index = m_CbnRtAudio.AddString(_T("Enabled" )); m_CbnRtAudio.SetItemData(index, 2);
	index = m_CbnRtAudio.AddString(_T("Disabled")); m_CbnRtAudio.SetItemData(index, 0);
	for(index = 0; index < 3; ++index)
	{
		if(m_CbnRtAudio.GetItemData(index) == static_cast<uint32>(TrackerSettings::Instance().WineSupportEnableRtAudio))
		{
			m_CbnRtAudio.SetCurSel(index);
		}
	}
	return TRUE;
}


void COptionsWine::OnSettingsChanged()
{
	SetModified(TRUE);
}


void COptionsWine::OnOK()
{
	TrackerSettings::Instance().WineSupportEnabled = IsDlgButtonChecked(IDC_CHECK_WINE_ENABLE) ? true : false;
	TrackerSettings::Instance().WineSupportEnablePulseAudio = static_cast<int32>(m_CbnPulseAudio.GetItemData(m_CbnPulseAudio.GetCurSel()));
	TrackerSettings::Instance().WineSupportEnablePortAudio = static_cast<int32>(m_CbnPortAudio.GetItemData(m_CbnPortAudio.GetCurSel()));
	TrackerSettings::Instance().WineSupportEnableRtAudio = static_cast<int32>(m_CbnRtAudio.GetItemData(m_CbnRtAudio.GetCurSel()));
	CPropertyPage::OnOK();
}


BOOL COptionsWine::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_WINE;
	return CPropertyPage::OnSetActive();
}


OPENMPT_NAMESPACE_END
