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
#include "../common/StringFixer.h"
#include "../sounddev/SoundDevice.h"
#include "../sounddev/SoundDeviceManager.h"


OPENMPT_NAMESPACE_BEGIN


static const TCHAR * const PolyphonyNames[] =
{
	_T("133MHz"),
	_T("166MHz"),
	_T("200MHz"),
	_T("233MHz"),
	_T("266MHz"),
	_T("300MHz"),
	_T("333MHz"),
	_T("400+MHz")
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


const TCHAR *gszChnCfgNames[3] =
{
	_T("Mono"),
	_T("Stereo"),
	_T("Quad")
};


static double ParseTime(CString str)
{
	return ConvertStrTo<double>(mpt::ToCharset(mpt::CharsetASCII, str)) / 1000.0;
}


static CString PrintTime(double seconds)
{
	int32 microseconds = Util::Round<int32>(seconds * 1000000.0);
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
	return mpt::ToCString(mpt::CharsetASCII, mpt::String::Print("%1 ms", mpt::fmt::fix(seconds * 1000.0, 0, precision)));
}


BEGIN_MESSAGE_MAP(COptionsSoundcard, CPropertyPage)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_CHECK4,	OnExclusiveModeChanged)
	ON_COMMAND(IDC_CHECK5,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK7,	OnSettingsChanged)
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
	ON_CBN_SELCHANGE(IDC_COMBO11, OnSettingsChanged)
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
	DDX_Control(pDX, IDC_COMBO11,		m_CbnStoppedMode);
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


COptionsSoundcard::COptionsSoundcard(SoundDevice::Identifier deviceIdentifier)
//----------------------------------------------------------------------------
	: CPropertyPage(IDD_OPTIONS_SOUNDCARD)
	, m_InitialDeviceIdentifier(deviceIdentifier)
{
	return;
}


void COptionsSoundcard::SetInitialDevice()
//----------------------------------------
{
	SetDevice(m_InitialDeviceIdentifier, true);
}


void COptionsSoundcard::SetDevice(SoundDevice::Identifier dev, bool forceReload)
//------------------------------------------------------------------------------
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
	SetDevice(m_CurrentDeviceInfo.GetIdentifier(), true);
	UpdateEverything();
}


BOOL COptionsSoundcard::OnInitDialog()
//------------------------------------
{
	CPropertyPage::OnInitDialog();
	SetInitialDevice();
	UpdateEverything();
	return TRUE;
}


void COptionsSoundcard::UpdateLatency()
//-------------------------------------
{
	{
		GetDlgItem(IDC_STATIC_LATENCY)->EnableWindow(TRUE);
		m_CbnLatencyMS.EnableWindow(TRUE);
	}
	// latency
	{
		static const double latencies [] = {
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
		for(std::size_t i = 0; i < CountOf(latencies); ++i)
		{
			if(m_CurrentDeviceCaps.LatencyMin <= latencies[i] && latencies[i] <= m_CurrentDeviceCaps.LatencyMax)
			{
				m_CbnLatencyMS.AddString(PrintTime(latencies[i]));
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
//--------------------------------------------
{
	{
		m_CbnUpdateIntervalMS.EnableWindow(TRUE);
	}
	// update interval
	{
		static const double updateIntervals [] = {
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
		for(std::size_t i = 0; i < CountOf(updateIntervals); ++i)
		{
			if(m_CurrentDeviceCaps.UpdateIntervalMin <= updateIntervals[i] && updateIntervals[i] <= m_CurrentDeviceCaps.UpdateIntervalMax)
			{
				m_CbnUpdateIntervalMS.AddString(PrintTime(updateIntervals[i]));
			}
		}
	}
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()) || !m_CurrentDeviceCaps.CanUpdateInterval)
	{
		m_CbnUpdateIntervalMS.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateGeneral()
//-------------------------------------
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
//----------------------------------------
{
	// Sound Device
	{
		m_CbnDevice.ResetContent();
		m_CbnDevice.SetImageList(&CMainFrame::GetMainFrame()->m_MiscIcons);

		UINT iItem = 0;

		for(std::vector<SoundDevice::Info>::const_iterator it = theApp.GetSoundDevicesManager()->begin(); it != theApp.GetSoundDevicesManager()->end(); ++it)
		{
			std::map<mpt::ustring, mpt::ustring> extraData = it->extraData;
			int priority = ConvertStrTo<int>(extraData[MPT_USTRING("priority")]);

			if(!TrackerSettings::Instance().m_MorePortaudio)
			{
				if(it->type == SoundDevice::TypePORTAUDIO_DS || it->type == SoundDevice::TypePORTAUDIO_WMME)
				{
					// skip those portaudio apis that are already implemented via our own SoundDevice class
					// can be overwritten via [Sound Settings]MorePortaudio=1
					continue;
				}
			}

			if(!TrackerSettings::Instance().m_SoundShowDeprecatedDevices)
			{
				if(priority < 0)
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
				cbi.lParam = theApp.GetSoundDevicesManager()->GetGlobalID(it->GetIdentifier());
				if(it->type == SoundDevice::TypeWAVEOUT || it->type == SoundDevice::TypePORTAUDIO_WMME)
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_WAVEOUT;
				} else if(it->type == SoundDevice::TypeDSOUND || it->type == SoundDevice::TypePORTAUDIO_DS)
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_DIRECTX;
				} else if(it->type == SoundDevice::TypeASIO)
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_ASIO;
				} else if(it->type == SoundDevice::TypePORTAUDIO_WASAPI)
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_SAMPLEMUTE; // // No real image available for now,
				} else if(it->type == SoundDevice::TypePORTAUDIO_WDMKS)
				{
					cbi.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE | CBEIF_OVERLAY;
					cbi.iImage = IMAGE_CHIP; // No real image available for now,
				} else
				{
					cbi.iImage = 0;
				}
				cbi.iSelectedImage = cbi.iImage;
				cbi.iOverlay = cbi.iImage;
				mpt::ustring name = it->name + (it->isDefault ? MPT_USTRING(" [default]") : MPT_USTRING(""));
				if(TrackerSettings::Instance().m_SoundShowNotRecommendedDeviceWarning && (priority < 0))
				{
					name += MPT_USTRING(" [not recommended]");
				}
				if(it->type == SoundDevice::TypeWAVEOUT || it->type == SoundDevice::TypeDSOUND || it->type == SoundDevice::TypeASIO)
				{
					// leave name alone
				} else if(it->type == SoundDevice::TypePORTAUDIO_WASAPI || it->type == SoundDevice::TypePORTAUDIO_WDMKS)
				{
					name = it->apiName + MPT_USTRING(" - ") + name;
				} else
				{
					name = ((it->apiPath.size() > 0) ? mpt::String::Combine(it->apiPath, MPT_USTRING(" - ")) + MPT_USTRING(" - ") : MPT_USTRING("")) + it->apiName + MPT_USTRING(" - ") + name;
				}
				TCHAR tmp[1024];
				MemsetZero(tmp);
				lstrcpyn(tmp, mpt::ToCString(name), 1023);
				cbi.pszText = tmp;
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
//------------------------------------
{
	UpdateGeneral();
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
	{
		m_CbnChannels.EnableWindow(TRUE);
	}
	m_CbnChannels.ResetContent();
	std::size_t maxChannels = 0;
	if(m_CurrentDeviceDynamicCaps.channelNames.size() > 0)
	{
		maxChannels = std::min<std::size_t>(4, m_CurrentDeviceDynamicCaps.channelNames.size());
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


void COptionsSoundcard::UpdateSampleFormat()
//------------------------------------------
{
	{
		m_CbnSampleFormat.EnableWindow(TRUE);
	}
	UINT n = 0;
	m_CbnSampleFormat.ResetContent();
	m_CbnSampleFormat.EnableWindow(m_CurrentDeviceCaps.CanSampleFormat ? TRUE : FALSE);
	for(UINT bits = 40; bits >= 8; bits -= 8)
	{
		if(bits == 40)
		{
			if(m_CurrentDeviceCaps.CanSampleFormat || (SampleFormatFloat32 == m_Settings.sampleFormat))
			{
				UINT ndx = m_CbnSampleFormat.AddString(_T("Float"));
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
				UINT ndx = m_CbnSampleFormat.AddString(mpt::ToCString(mpt::String::Print(MPT_USTRING("%1 Bit"), bits)));
				m_CbnSampleFormat.SetItemData(ndx, bits);
				if((SampleFormat)bits == m_Settings.sampleFormat)
				{
					n = ndx;
				}
			}
		}
	}
	m_CbnSampleFormat.SetCurSel(n);
	if(theApp.GetSoundDevicesManager()->IsDeviceUnavailable(m_CurrentDeviceInfo.GetIdentifier()))
	{
		m_CbnSampleFormat.EnableWindow(FALSE);
	}
}


void COptionsSoundcard::UpdateDither()
//------------------------------------
{
	{
		m_CbnDither.EnableWindow(TRUE);
	}
	m_CbnDither.ResetContent();
	SampleFormat sampleFormat = static_cast<SampleFormatEnum>(m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel()));
	if(sampleFormat.IsInt() && sampleFormat.GetBitsPerSample() < 32)
	{
		m_CbnDither.EnableWindow(TRUE);
		for(int i=0; i<NumDitherModes; ++i)
		{
			m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName((DitherMode)i) + MPT_USTRING(" dithering")));
		}
	} else if(m_CurrentDeviceCaps.HasInternalDither)
	{
		m_CbnDither.EnableWindow(TRUE);
		m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherNone) + MPT_USTRING(" dithering")));
		m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherDefault) + MPT_USTRING(" dithering")));
	} else
	{
		m_CbnDither.EnableWindow(FALSE);
		for(int i=0; i<NumDitherModes; ++i)
		{
			m_CbnDither.AddString(mpt::ToCString(Dither::GetModeName(DitherNone) + MPT_USTRING(" dithering")));
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
//--------------------------------------------
{
	{
		GetDlgItem(IDC_STATIC_CHANNELMAPPING)->EnableWindow(TRUE);
		for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)
		{
			CStatic *statictext = &m_StaticChannelMapping[mch];
			CComboBox *combo = &m_CbnChannelMapping[mch];
			statictext->EnableWindow(TRUE);
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
	for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)	// Host channels
	{
		CStatic *statictext = &m_StaticChannelMapping[mch];
		CComboBox *combo = &m_CbnChannelMapping[mch];
		statictext->EnableWindow((m_CurrentDeviceCaps.CanChannelMapping && mch < usedChannels) ? TRUE : FALSE);
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
		for(int mch = 0; mch < NUM_CHANNELCOMBOBOXES; mch++)
		{
			CStatic *statictext = &m_StaticChannelMapping[mch];
			CComboBox *combo = &m_CbnChannelMapping[mch];
			statictext->EnableWindow(FALSE);
			combo->EnableWindow(FALSE);
		}
	}
}


void COptionsSoundcard::OnDeviceChanged()
//---------------------------------------
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
//----------------------------------------------
{
	UpdateSampleRates();
	OnSettingsChanged();
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
	theApp.GetSoundDevicesManager()->OpenDriverSettings(
		theApp.GetSoundDevicesManager()->FindDeviceInfo(static_cast<SoundDevice::Manager::GlobalID>(m_CbnDevice.GetItemData(m_CbnDevice.GetCurSel()))).GetIdentifier(),
		CMainFrame::GetMainFrame(),
		CMainFrame::GetMainFrame()->gpSoundDevice
		);
}


void COptionsSoundcard::OnChannelChanged(int channel)
//---------------------------------------------------
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
//-----------------------------------------
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
		int pos = m_CbnMixingFreq.AddString(mpt::ToCString(mpt::String::Print(MPT_USTRING("%1 Hz"), samplerates[i])));
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
//--------------------------------------
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
//-----------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_SOUNDCARD;
	return CPropertyPage::OnSetActive();
}


void COptionsSoundcard::OnOK()
//----------------------------
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
		m_Settings.Channels = n;
		if((m_Settings.Channels != 1) && (m_Settings.Channels != 4))
		{
			m_Settings.Channels = 2;
		}
	}
	// SampleFormat
	{
		DWORD_PTR n = m_CbnSampleFormat.GetItemData(m_CbnSampleFormat.GetCurSel());
		m_Settings.sampleFormat = static_cast<SampleFormat>(static_cast<int>(n & 0xFF));
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
			int numChannels = std::min<int>(m_Settings.Channels, NUM_CHANNELCOMBOBOXES);
			std::vector<int32> channels(numChannels);
			for(int mch = 0; mch < numChannels; mch++)	// Host channels
			{
				CComboBox *combo = &m_CbnChannelMapping[mch];
				channels[mch] = static_cast<int32>(combo->GetItemData(combo->GetCurSel()));
			}
			m_Settings.Channels = channels;
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
//----------------------------------------
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
			s += mpt::String::Print(MPT_USTRING("Buffer: %1%% (%2/%3)\r\n"), (bufferAttributes.Latency > 0.0) ? Util::Round<int64>(stats.InstantaneousLatency / bufferAttributes.Latency * 100.0) : 0, (stats.LastUpdateInterval > 0.0) ? Util::Round<int64>(bufferAttributes.Latency / stats.LastUpdateInterval) : 0, bufferAttributes.NumBuffers);
		} else
		{
			s += mpt::String::Print(MPT_USTRING("Buffer: %1%%\r\n"), (bufferAttributes.Latency > 0.0) ? Util::Round<int64>(stats.InstantaneousLatency / bufferAttributes.Latency * 100.0) : 0);
		}
		s += mpt::String::Print(MPT_USTRING("Latency: %1 ms (current: %2 ms, %3 frames)\r\n"), mpt::ufmt::f("%4.1f", bufferAttributes.Latency * 1000.0), mpt::ufmt::f("%4.1f", stats.InstantaneousLatency * 1000.0), Util::Round<int64>(stats.InstantaneousLatency * samplerate));
		s += mpt::String::Print(MPT_USTRING("Period: %1 ms (current: %2 ms, %3 frames)\r\n"), mpt::ufmt::f("%4.1f", bufferAttributes.UpdateInterval * 1000.0), mpt::ufmt::f("%4.1f", stats.LastUpdateInterval * 1000.0), Util::Round<int64>(stats.LastUpdateInterval * samplerate));
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
	ON_CBN_SELCHANGE(IDC_COMBO_FILTER,			OnResamplerChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_FILTERWINDOW,	OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_POLYPHONY,		OnSettingsChanged)
	ON_EN_UPDATE(IDC_WFIRCUTOFF,				OnSettingsChanged)
	ON_EN_UPDATE(IDC_RAMPING_IN,				OnRampingChanged)
	ON_EN_UPDATE(IDC_RAMPING_OUT,				OnRampingChanged)
	ON_COMMAND(IDC_CHECK_SOFTPAN,				OnSettingsChanged)
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
		const ResamplingMode resamplingModes[] = { SRCMODE_NEAREST, SRCMODE_LINEAR, SRCMODE_SPLINE, SRCMODE_POLYPHASE, SRCMODE_FIRFILTER };
		for(uint32 i = 0; i < CountOf(resamplingModes); i++)
		{
			int index = m_CbnResampling.AddString(CTrackApp::GetResamplingModeName(resamplingModes[i], true));
			m_CbnResampling.SetItemData(index, resamplingModes[i]);
			if(TrackerSettings::Instance().ResamplerMode == resamplingModes[i])
				m_CbnResampling.SetCurSel(index);
		}
	}

	// Resampler bandwidth
	{
		m_CEditWFIRCutoff.SetWindowText(mpt::ToCString(mpt::ToUString(TrackerSettings::Instance().ResamplerCutoffPercent)));
		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1))->SetRange32(1, 99);
	}

	// Resampler filter window
	{
		// done in OnResamplerChanged()
	}

	// volume ramping
	{
		m_CEditRampUp.SetWindowText(mpt::ToCString(mpt::ToUString(TrackerSettings::Instance().GetMixerSettings().GetVolumeRampUpMicroseconds())));
		m_CEditRampDown.SetWindowText(mpt::ToCString(mpt::ToUString(TrackerSettings::Instance().GetMixerSettings().GetVolumeRampDownMicroseconds())));
		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN2))->SetRange32(0, int32_max);
		static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN3))->SetRange32(0, int32_max);
		UpdateRamping();
	}

	// Max Mixing Channels
	{
		m_CbnPolyphony.ResetContent();
		for(int n = 0; n < CountOf(PolyphonyChannels); ++n)
		{
			m_CbnPolyphony.AddString(mpt::ToCString(mpt::String::Print(MPT_USTRING("%1 (%2)"), PolyphonyChannels[n], mpt::ToUnicode(CString(PolyphonyNames[n])))));
			if(TrackerSettings::Instance().MixerMaxChannels == PolyphonyChannels[n])
			{
				m_CbnPolyphony.SetCurSel(n);
			}
		}
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

	OnResamplerChanged();
	m_initialized = true;

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
	ResamplingMode srcMode = static_cast<ResamplingMode>(m_CbnResampling.GetItemData(m_CbnResampling.GetCurSel()));
	switch(srcMode)
	{
		case SRCMODE_FIRFILTER:
			m_CbnWFIRType.ResetContent();
			m_CbnWFIRType.AddString(_T("Hann"));
			m_CbnWFIRType.AddString(_T("Hamming"));
			m_CbnWFIRType.AddString(_T("Blackman Exact"));
			m_CbnWFIRType.AddString(_T("Blackman 3 Tap 61"));
			m_CbnWFIRType.AddString(_T("Blackman 3 Tap 67"));
			m_CbnWFIRType.AddString(_T("Blackman Harris"));
			m_CbnWFIRType.AddString(_T("Blackman 4 Tap 74"));
			m_CbnWFIRType.AddString(_T("Kaiser a=7.5"));
			m_CbnWFIRType.SetCurSel(TrackerSettings::Instance().ResamplerSubMode);
			break;
		case SRCMODE_POLYPHASE:
			m_CbnWFIRType.ResetContent();
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.AddString(_T("Auto"));
			m_CbnWFIRType.SetCurSel(TrackerSettings::Instance().ResamplerSubMode);
			break;
		default:
			m_CbnWFIRType.ResetContent();
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.AddString(_T("none"));
			m_CbnWFIRType.SetCurSel(TrackerSettings::Instance().ResamplerSubMode);
			break;
	}
	CSpinButtonCtrl *spinWFIRCutoff = static_cast<CSpinButtonCtrl *>(GetDlgItem(IDC_SPIN1));
	switch(srcMode)
	{
		case SRCMODE_POLYPHASE:
			m_CEditWFIRCutoff.EnableWindow(FALSE);
			spinWFIRCutoff->EnableWindow(FALSE);
			m_CbnWFIRType.EnableWindow(FALSE);
			break;
		case SRCMODE_FIRFILTER:
			m_CEditWFIRCutoff.EnableWindow(TRUE);
			spinWFIRCutoff->EnableWindow(TRUE);
			m_CbnWFIRType.EnableWindow(TRUE);
			break;
		default:
			m_CEditWFIRCutoff.EnableWindow(FALSE);
			spinWFIRCutoff->EnableWindow(FALSE);
			m_CbnWFIRType.EnableWindow(FALSE);
			break;
	}
	OnSettingsChanged();
}


void COptionsMixer::OnRampingChanged()
//------------------------------------
{
	if(!m_initialized)
		return;
	UpdateRamping();
	OnSettingsChanged();
}


void COptionsMixer::OnScroll(UINT n, UINT pos, CScrollBar *p)
//-----------------------------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(n);
	MPT_UNREFERENCED_PARAMETER(pos);
	// stereo sep
	if(p == (CScrollBar *)&m_SliderStereoSep)
	{
		UpdateStereoSep();
		OnSettingsChanged();
	}
}


void COptionsMixer::UpdateRamping()
//---------------------------------
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
//-----------------------------------
{
	CString s;
	s.Format(_T("%d%%"), ((8 * m_SliderStereoSep.GetPos()) * 100) / 128);
	SetDlgItemText(IDC_TEXT_STEREOSEP, s);
}


void COptionsMixer::OnOK()
//------------------------
{
	// resampler mode
	{
		TrackerSettings::Instance().ResamplerMode = static_cast<ResamplingMode>(m_CbnResampling.GetItemData(m_CbnResampling.GetCurSel()));
	}

	// resampler bandwidth
	{
		CString s;
		m_CEditWFIRCutoff.GetWindowText(s);
		if(s != "")
		{
			int newCutoff = ConvertStrTo<int>(s);
			Limit(newCutoff, 0, 100);
			TrackerSettings::Instance().ResamplerCutoffPercent = newCutoff;
		}
		{
			s.Format(_T("%d"), TrackerSettings::Instance().ResamplerCutoffPercent.Get());
			m_CEditWFIRCutoff.SetWindowText(s);
		}
	}

	// resampler filter window
	{
		TrackerSettings::Instance().ResamplerSubMode = (uint8)m_CbnWFIRType.GetCurSel();
	}

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

	CPropertyPage::OnOK();
}


////////////////////////////////////////////////////////////////////////////////
//
// CEQSavePresetDlg
//

#ifndef NO_EQ

//====================================
class CEQSavePresetDlg: public CDialog
//====================================
{
protected:
	EQPreset &m_EQ;

public:
	CEQSavePresetDlg(EQPreset &eq, CWnd *parent = nullptr) : CDialog(IDD_SAVEPRESET, parent), m_EQ(eq) { }
	BOOL OnInitDialog();
	void OnOK();
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
			int n = pCombo->AddString(mpt::ToCString(mpt::CharsetLocale, TrackerSettings::Instance().m_EqUserPresets[i].szName));
			pCombo->SetItemData( n, i);
			if (!lstrcmpiA(TrackerSettings::Instance().m_EqUserPresets[i].szName, m_EQ.szName)) ndx = n;
		}
		pCombo->SetCurSel(ndx);
	}
	SetDlgItemText(IDC_EDIT1, mpt::ToCString(mpt::CharsetLocale, m_EQ.szName));
	return TRUE;
}


void CEQSavePresetDlg::OnOK()
//---------------------------
{
	CComboBox *pCombo = (CComboBox *)GetDlgItem(IDC_COMBO1);
	if (pCombo)
	{
		int n = pCombo->GetCurSel();
		if ((n < 0) || (n >= 4)) n = 0;
		CString s;
		GetDlgItemText(IDC_EDIT1, s);
		mpt::String::Copy(m_EQ.szName, mpt::ToCharset(mpt::CharsetLocale, s));
		mpt::String::SetNullTerminator(m_EQ.szName);
		TrackerSettings::Instance().m_EqUserPresets[n] = m_EQ;
	}
	CDialog::OnOK();
}


void CEQSlider::Init(UINT nID, UINT n, CWnd *parent)
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
	ON_COMMAND(IDC_CHECK3,	OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,	OnEqUser1)
	ON_COMMAND(IDC_BUTTON2,	OnEqUser2)
	ON_COMMAND(IDC_BUTTON3,	OnEqUser3)
	ON_COMMAND(IDC_BUTTON4,	OnEqUser4)
	ON_COMMAND(IDC_BUTTON5,	OnSavePreset)
	ON_COMMAND_RANGE(ID_EQSLIDER_BASE, ID_EQSLIDER_BASE + MAX_EQ_BANDS,	OnSliderMenu)
	ON_COMMAND_RANGE(ID_EQMENU_BASE, ID_EQMENU_BASE + EQ_MAX_FREQS,		OnSliderFreq)
#endif // !NO_EQ

	// DSP
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_COMBO2,	OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK4,			OnSettingsChanged)
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
		LPCSTR pszName = GetReverbPresetName(iRvb);
		if (pszName)
		{
			UINT n = m_CbnReverbPreset.AddString(mpt::ToCString(mpt::CharsetASCII, pszName));
			m_CbnReverbPreset.SetItemData(n, iRvb);
			if (iRvb == TrackerSettings::Instance().m_ReverbSettings.m_nReverbType) nSel = n;
		}
	}
	m_CbnReverbPreset.SetCurSel(nSel);
	if(!(GetProcSupport() & PROCSUPPORT_MMX))
	{
		GetDlgItem(IDC_CHECK6)->EnableWindow(FALSE);
		m_SbReverbDepth.EnableWindow(FALSE);
		m_CbnReverbPreset.EnableWindow(FALSE);
	} else
	{
		if (dwQuality & SNDDSP_REVERB) CheckDlgButton(IDC_CHECK6, BST_CHECKED);
	}
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
//--------------------------------
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
	if (IsDlgButtonChecked(IDC_CHECK4)) dwQuality |= SNDDSP_SURROUND;
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
		TrackerSettings::Instance().m_MegaBassSettings.m_nXBassDepth = nXBassDepth;
		TrackerSettings::Instance().m_MegaBassSettings.m_nXBassRange = nXBassRange;
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
//----------------------------------------
{
	CriticalSection cs;
	if(CMainFrame::GetMainFrame()->GetSoundFilePlaying())
		CMainFrame::GetMainFrame()->GetSoundFilePlaying()->SetEQGains(m_EQPreset.Gains, MAX_EQ_BANDS, m_EQPreset.Freqs, bReset);
}


void COptionsPlayer::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
//-----------------------------------------------------------------------------
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
//-------------------------------------------------------
{
	m_EQPreset = preset;
	UpdateEQ(TRUE);
	UpdateDialog();
}


void COptionsPlayer::OnSavePreset()
//---------------------------------
{
	CEQSavePresetDlg dlg(m_EQPreset, this);
	if (dlg.DoModal() == IDOK)
	{
		UpdateDialog();
	}
}


static void f2s(UINT f, TCHAR *s)
//-------------------------------
{
	if (f < 1000)
	{
		wsprintf(s, _T("%uHz"), f);
	} else
	{
		UINT fHi = f / 1000u;
		UINT fLo = f % 1000u;
		if (fLo)
		{
			wsprintf(s, _T("%u.%ukHz"), fHi, fLo/100);
		} else
		{
			wsprintf(s, _T("%ukHz"), fHi);
		}
	}
}


void COptionsPlayer::UpdateDialog()
//---------------------------------
{
	TCHAR s[32];
	for (UINT i=0; i<MAX_EQ_BANDS; i++)
	{
		int n = 32 - m_EQPreset.Gains[i];
		if (n < 0) n = 0;
		if (n > 32) n = 32;
		if (n != (m_Sliders[i].GetPos() & 0xFFFF)) m_Sliders[i].SetPos(n);
		f2s(m_EQPreset.Freqs[i], s);
		SetDlgItemText(IDC_TEXT1 + i, s);
	}
	for(int i = 0; i < CountOf(TrackerSettings::Instance().m_EqUserPresets); i++)
	{
		SetDlgItemText(IDC_BUTTON1 + i, TrackerSettings::Instance().m_EqUserPresets[i].szName);
	}
}


void COptionsPlayer::OnSliderMenu(UINT nID)
//-----------------------------------------
{
	UINT n = nID - ID_EQSLIDER_BASE;
	if (n < MAX_EQ_BANDS)
	{
		TCHAR s[32];
		HMENU hMenu = ::CreatePopupMenu();
		m_nSliderMenu = n;
		if (!hMenu) return;
		const UINT *pFreqs = gEqBandFreqs[m_nSliderMenu];
		for (UINT i = 0; i < EQ_MAX_FREQS; i++)
		{
			DWORD d = MF_STRING;
			if (m_EQPreset.Freqs[m_nSliderMenu] == pFreqs[i]) d |= MF_CHECKED;
			f2s(pFreqs[i], s);
			::AppendMenu(hMenu, d, ID_EQMENU_BASE+i, s);
		}
		CPoint pt(m_Sliders[m_nSliderMenu].m_x, m_Sliders[m_nSliderMenu].m_y);
		m_Sliders[m_nSliderMenu].ClientToScreen(&pt);
		::TrackPopupMenu(hMenu, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
		::DestroyMenu(hMenu);
	}
}


void COptionsPlayer::OnSliderFreq(UINT nID)
//-----------------------------------------
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
	ON_CBN_SELCHANGE(IDC_COMBO1,			OnSettingsChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,					OnSettingsChanged)
	ON_COMMAND(IDC_CHECK2,					OnSettingsChanged)
	ON_COMMAND(IDC_CHECK3,					OnSettingsChanged)
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
	DDX_Control(pDX, IDC_COMBO3,	m_Quantize);
	//}}AFX_DATA_MAP
}


BOOL CMidiSetupDlg::OnInitDialog()
//--------------------------------
{
	MIDIINCAPS mic;
	CComboBox *combo;

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
		const TCHAR *text;
		RecordAftertouchOptions option;
	} aftertouchOptions[] =
	{
		{ _T("Do not record Aftertouch"), atDoNotRecord },
		{ _T("Record as Volume Commands"), atRecordAsVolume },
		{ _T("Record as MIDI Macros"), atRecordAsMacro },
	};

	for(int i = 0; i < CountOf(aftertouchOptions); i++)
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
	SetDlgItemInt(IDC_EDIT1, TrackerSettings::Instance().midiImportTicks);
	SetDlgItemInt(IDC_EDIT2, TrackerSettings::Instance().midiImportPatternLen);
	for(uint32 i = 2; i < 7; i++)
	{
		if((1u << i) == TrackerSettings::Instance().midiImportQuantize)
		{
			m_Quantize.SetCurSel(i - 2);
		}
	}
	m_SpinSpd.SetRange(2, 16);
	m_SpinPat.SetRange(1, MAX_PATTERN_ROWS);
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
	if (IsDlgButtonChecked(IDC_CHECK3)) m_dwMidiSetup |= MIDISETUP_ENABLE_RECORD_DEFAULT;
	if (IsDlgButtonChecked(IDC_CHECK4)) m_dwMidiSetup |= MIDISETUP_TRANSPOSEKEYBOARD;
	if (IsDlgButtonChecked(IDC_MIDI_TO_PLUGIN)) m_dwMidiSetup |= MIDISETUP_MIDITOPLUG;
	if (IsDlgButtonChecked(IDC_MIDI_MACRO_CONTROL)) m_dwMidiSetup |= MIDISETUP_MIDIMACROCONTROL;
	if (IsDlgButtonChecked(IDC_MIDIVOL_TO_NOTEVOL)) m_dwMidiSetup |= MIDISETUP_MIDIVOL_TO_NOTEVOL;
	if (IsDlgButtonChecked(IDC_MIDIPLAYCONTROL)) m_dwMidiSetup |= MIDISETUP_RESPONDTOPLAYCONTROLMSGS;
	if (IsDlgButtonChecked(IDC_MIDIPLAYPATTERNONMIDIIN)) m_dwMidiSetup |= MIDISETUP_PLAYPATTERNONMIDIIN;
	if (IsDlgButtonChecked(IDC_CHECK5)) m_dwMidiSetup |= MIDISETUP_MIDIMACROPITCHBEND;

	if ((combo = (CComboBox *)GetDlgItem(IDC_COMBO1)) != NULL)
	{
		int n = combo->GetCurSel();
		if (n >= 0) m_nMidiDevice = static_cast<UINT>(combo->GetItemData(n));
	}

	TrackerSettings::Instance().aftertouchBehaviour = static_cast<RecordAftertouchOptions>(m_ATBehaviour.GetItemData(m_ATBehaviour.GetCurSel()));

	TrackerSettings::Instance().midiVelocityAmp = static_cast<uint16>(Clamp(GetDlgItemInt(IDC_EDIT3), 1u, 10000u));

	CString cc;
	GetDlgItemText(IDC_EDIT4, cc);
	TrackerSettings::Instance().midiIgnoreCCs = StringToIgnoredCCs(cc.GetString());

	TrackerSettings::Instance().midiImportTicks = static_cast<uint8>(Clamp(GetDlgItemInt(IDC_EDIT1), uint8(1), uint8(16)));
	TrackerSettings::Instance().midiImportPatternLen = Clamp(GetDlgItemInt(IDC_EDIT2), ROWINDEX(1), MAX_PATTERN_ROWS);
	if(m_Quantize.GetCurSel() != -1)
	{
		TrackerSettings::Instance().midiImportQuantize = 1 << (m_Quantize.GetCurSel() + 2);
	}

	if (pMainFrm) pMainFrm->SetupMidi(m_dwMidiSetup, m_nMidiDevice);
	CPropertyPage::OnOK();
}


BOOL CMidiSetupDlg::OnSetActive()
//-------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_MIDI;
	return CPropertyPage::OnSetActive();
}


OPENMPT_NAMESPACE_END
