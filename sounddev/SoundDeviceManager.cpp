/*
 * SoundDeviceManager.cpp
 * ----------------------
 * Purpose: Sound device manager class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDeviceManager.h"
#include "SoundDevice.h"

#include "SoundDeviceASIO.h"
#include "SoundDeviceDirectSound.h"
#include "SoundDevicePortAudio.h"
#include "SoundDeviceRtAudio.h"
#include "SoundDeviceWaveout.h"
#include "SoundDevicePulseaudio.h"
#include "SoundDevicePulseSimple.h"
#include "SoundDeviceStub.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {



struct CompareInfo
{
	static int64 score(const SoundDevice::Info &x)
	{
		int64 score = 0;
		score *= 256;
		score += static_cast<int8>(x.managerFlags.defaultFor);
		score *= 256;
		score += static_cast<int8>(x.flags.usability);
		score *= 256;
		score += static_cast<int8>(x.flags.level);
		score *= 256;
		score += static_cast<int8>(x.flags.compatible);
		score *= 256;
		score += static_cast<int8>(x.flags.api);
		score *= 256;
		score += static_cast<int8>(x.flags.io);
		score *= 256;
		score += static_cast<int8>(x.flags.mixing);
		score *= 256;
		score += static_cast<int8>(x.flags.implementor);
		return score;
	}
	bool operator () (const SoundDevice::Info &x, const SoundDevice::Info &y)
	{
		const auto scorex = score(x);
		const auto scorey = score(y);
		return (scorex > scorey)
			|| ((scorex == scorey) && (x.type > y.type))
			|| ((scorex == scorey) && (x.type == y.type) && (x.default_ > y.default_))
			|| ((scorex == scorey) && (x.type == y.type) && (x.default_ == y.default_) && (x.name < y.name))
			;
	}
};


template <typename Tdevice>
void Manager::EnumerateDevices(ILogger &logger, SoundDevice::SysInfo sysInfo)
{
	const auto infos = Tdevice::EnumerateDevices(logger, sysInfo);
	mpt::append(m_SoundDevices, infos);
	for(const auto &info : infos)
	{
		SoundDevice::Identifier identifier = info.GetIdentifier();
		if(!identifier.empty())
		{
			m_DeviceFactoryMethods[identifier] = ConstructSoundDevice<Tdevice>;
		}
	}
}


template <typename Tdevice>
SoundDevice::IBase* Manager::ConstructSoundDevice(ILogger &logger, const SoundDevice::Info &info, SoundDevice::SysInfo sysInfo)
{
	return new Tdevice(logger, info, sysInfo);
}


void Manager::ReEnumerate(bool firstRun)
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	m_SoundDevices.clear();
	m_DeviceUnavailable.clear();
	m_DeviceFactoryMethods.clear();
	m_DeviceCaps.clear();
	m_DeviceDynamicCaps.clear();

#ifdef MPT_WITH_PORTAUDIO
	if(!firstRun)
	{
		if(m_EnabledBackends.PortAudio)
		{
			m_PortAudioInitializer->Reload();
		}
	}
#endif // MPT_WITH_PORTAUDIO

#if defined(MPT_ENABLE_PULSEAUDIO_FULL)
#if defined(MPT_WITH_PULSEAUDIO)
	if(m_EnabledBackends.Pulseaudio)
	{
		EnumerateDevices<Pulseaudio>(GetLogger(), GetSysInfo());
	}
#endif // MPT_WITH_PULSEAUDIO
#endif // MPT_ENABLE_PULSEAUDIO_FULL

#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
	if(m_EnabledBackends.PulseaudioSimple)
	{
		EnumerateDevices<PulseaudioSimple>(GetLogger(), GetSysInfo());
	}
#endif // MPT_WITH_PULSEAUDIO && MPT_WITH_PULSEAUDIOSIMPLE

#if MPT_OS_WINDOWS
	if(m_EnabledBackends.WaveOut)
	{
		EnumerateDevices<CWaveDevice>(GetLogger(), GetSysInfo());
	}
#endif // MPT_OS_WINDOWS

#if defined(MPT_WITH_DIRECTSOUND)
	// kind of deprecated by now
	if(m_EnabledBackends.DirectSound)
	{
		EnumerateDevices<CDSoundDevice>(GetLogger(), GetSysInfo());
	}
#endif // MPT_WITH_DIRECTSOUND

#ifdef MPT_WITH_ASIO
	if(m_EnabledBackends.ASIO)
	{
		EnumerateDevices<CASIODevice>(GetLogger(), GetSysInfo());
	}
#endif // MPT_WITH_ASIO

#ifdef MPT_WITH_PORTAUDIO
	if(m_EnabledBackends.PortAudio)
	{
		EnumerateDevices<CPortaudioDevice>(GetLogger(), GetSysInfo());
	}
#endif // MPT_WITH_PORTAUDIO

#ifdef MPT_WITH_RTAUDIO
	if(m_EnabledBackends.RtAudio)
	{
		EnumerateDevices<CRtAudioDevice>(GetLogger(), GetSysInfo());
	}
#endif // MPT_WITH_RTAUDIO

#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)
	{
		EnumerateDevices<SoundDeviceStub>(GetLogger(), GetSysInfo());
	}
#endif

	struct Default
	{
		SoundDevice::Info::DefaultFor value = SoundDevice::Info::DefaultFor::None;
	};

	std::map<SoundDevice::Type, Default> typeDefault;
	if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Linux)
	{
#if defined(MPT_WITH_PULSEAUDIO)
		typeDefault[U_("PulseAudio")].value = Info::DefaultFor::System;
#endif
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
		typeDefault[U_("PulseAudio-Simple")].value = Info::DefaultFor::System;
#endif
#if defined(MPT_WITH_RTAUDIO)
		typeDefault[MPT_UFORMAT("RtAudio-{}")(U_("pulse"))].value = Info::DefaultFor::System;
#endif
#if defined(MPT_WITH_RTAUDIO)
		typeDefault[MPT_UFORMAT("RtAudio-{}")(U_("alsa"))].value = Info::DefaultFor::LowLevel;
#endif
#if defined(MPT_WITH_RTAUDIO)
		typeDefault[MPT_UFORMAT("RtAudio-{}")(U_("jack"))].value = Info::DefaultFor::ProAudio;
#endif
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paALSA)].value = Info::DefaultFor::LowLevel;
#endif
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paJACK)].value = Info::DefaultFor::ProAudio;
#endif
	} else if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Darwin)
	{
#if defined(MPT_WITH_RTAUDIO)
		typeDefault[MPT_UFORMAT("RtAudio-{}")(U_("core"))].value = Info::DefaultFor::System;
#endif
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paCoreAudio)].value = Info::DefaultFor::System;
#endif
#if defined(MPT_WITH_RTAUDIO)
		typeDefault[MPT_UFORMAT("RtAudio-{}")(U_("jack"))].value = Info::DefaultFor::ProAudio;
#endif
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paJACK)].value = Info::DefaultFor::ProAudio;
#endif
	} else if(GetSysInfo().SystemClass == mpt::osinfo::osclass::BSD)
	{
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paOSS)].value = Info::DefaultFor::System;
#endif
#if defined(MPT_WITH_RTAUDIO)
		typeDefault[MPT_UFORMAT("RtAudio-{}")(U_("oss"))].value = Info::DefaultFor::System;
#endif
	} else if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Haiku)
	{
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paBeOS)].value = Info::DefaultFor::System;
#endif
	} else
	if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Windows && GetSysInfo().IsWindowsWine() && GetSysInfo().WineHostClass == mpt::osinfo::osclass::Linux)
	{ // Wine on Linux
		typeDefault[SoundDevice::TypePORTAUDIO_WASAPI].value = Info::DefaultFor::System;
	} else if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Windows && GetSysInfo().IsWindowsWine())
	{ // Wine
		typeDefault[SoundDevice::TypePORTAUDIO_WASAPI].value = Info::DefaultFor::System;
		typeDefault[SoundDevice::TypeDSOUND].value = Info::DefaultFor::LowLevel;
	} else if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Windows && GetSysInfo().WindowsVersion.IsBefore(mpt::osinfo::windows::Version::WinVista))
	{ // WinXP
		typeDefault[SoundDevice::TypeWAVEOUT].value = Info::DefaultFor::System;
		typeDefault[SoundDevice::TypeASIO].value = Info::DefaultFor::ProAudio;
		typeDefault[SoundDevice::TypePORTAUDIO_WDMKS].value = Info::DefaultFor::LowLevel;
	} else if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Windows && GetSysInfo().WindowsVersion.IsBefore(mpt::osinfo::windows::Version::Win7))
	{ // Vista
		typeDefault[SoundDevice::TypeWAVEOUT].value = Info::DefaultFor::System;
		typeDefault[SoundDevice::TypeASIO].value = Info::DefaultFor::ProAudio;
		typeDefault[SoundDevice::TypePORTAUDIO_WDMKS].value = Info::DefaultFor::LowLevel;
	} else if(GetSysInfo().SystemClass == mpt::osinfo::osclass::Windows)
	{ // >=Win7
		typeDefault[SoundDevice::TypePORTAUDIO_WASAPI].value = Info::DefaultFor::System;
		typeDefault[SoundDevice::TypeASIO].value = Info::DefaultFor::ProAudio;
		typeDefault[SoundDevice::TypePORTAUDIO_WDMKS].value = Info::DefaultFor::LowLevel;
	} else
	{ // unknown
		typeDefault[SoundDevice::TypePORTAUDIO_WASAPI].value = Info::DefaultFor::System;
	}
	for(auto & deviceInfo : m_SoundDevices)
	{
		if(typeDefault[deviceInfo.type].value != Info::DefaultFor::None)
		{
			deviceInfo.managerFlags.defaultFor = typeDefault[deviceInfo.type].value;
		}
	}
	std::stable_sort(m_SoundDevices.begin(), m_SoundDevices.end(), CompareInfo());

	MPT_LOG(GetLogger(), LogDebug, "sounddev", MPT_UFORMAT("Sound Devices enumerated:")());
	for(const auto &device : m_SoundDevices)
	{
		MPT_LOG(GetLogger(), LogDebug, "sounddev", MPT_UFORMAT(" Identifier : {}")(device.GetIdentifier()));
		MPT_LOG(GetLogger(), LogDebug, "sounddev", MPT_UFORMAT("  Type      : {}")(device.type));
		MPT_LOG(GetLogger(), LogDebug, "sounddev", MPT_UFORMAT("  InternalID: {}")(device.internalID));
		MPT_LOG(GetLogger(), LogDebug, "sounddev", MPT_UFORMAT("  API Name  : {}")(device.apiName));
		MPT_LOG(GetLogger(), LogDebug, "sounddev", MPT_UFORMAT("  Name      : {}")(device.name));
		for(const auto &extra : device.extraData)
		{
			MPT_LOG(GetLogger(), LogDebug, "sounddev", MPT_UFORMAT("  Extra Data: {} = {}")(extra.first, extra.second));
		}
	}
	
}


SoundDevice::Manager::GlobalID Manager::GetGlobalID(SoundDevice::Identifier identifier) const
{
	for(std::size_t i = 0; i < m_SoundDevices.size(); ++i)
	{
		if(m_SoundDevices[i].GetIdentifier() == identifier)
		{
			return i;
		}
	}
	return ~SoundDevice::Manager::GlobalID();
}


SoundDevice::Info Manager::FindDeviceInfo(SoundDevice::Manager::GlobalID id) const
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	if(id > m_SoundDevices.size())
	{
		return SoundDevice::Info();
	}
	return m_SoundDevices[id];
}


SoundDevice::Info Manager::FindDeviceInfo(SoundDevice::Identifier identifier) const
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	if(m_SoundDevices.empty())
	{
		return SoundDevice::Info();
	}
	if(identifier.empty())
	{
		return SoundDevice::Info();
	}
	for(const auto &info : *this)
	{
		if(info.GetIdentifier() == identifier)
		{
			return info;
		}
	}
	return SoundDevice::Info();
}


SoundDevice::Info Manager::FindDeviceInfoBestMatch(SoundDevice::Identifier identifier)
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	if(m_SoundDevices.empty())
	{
		return SoundDevice::Info();
	}
	if(!identifier.empty())
	{ // valid identifier
		for(const auto &info : *this)
		{
			if((info.GetIdentifier() == identifier) && !IsDeviceUnavailable(info.GetIdentifier()))
			{ // exact match
				return info;
			}
		}
	}
	for(const auto &info : *this)
	{ // find first available device
		if(!IsDeviceUnavailable(info.GetIdentifier()))
		{
			return info;
		}
	}
	// default to first device
	return *begin();
}


bool Manager::OpenDriverSettings(SoundDevice::Identifier identifier, SoundDevice::IMessageReceiver *messageReceiver, SoundDevice::IBase *currentSoundDevice)
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	bool result = false;
	if(currentSoundDevice && FindDeviceInfo(identifier).IsValid() && (currentSoundDevice->GetDeviceInfo().GetIdentifier() == identifier))
	{
		result = currentSoundDevice->OpenDriverSettings();
	} else
	{
		SoundDevice::IBase *dummy = CreateSoundDevice(identifier);
		if(dummy)
		{
			dummy->SetMessageReceiver(messageReceiver);
			result = dummy->OpenDriverSettings();
		}
		delete dummy;
	}
	return result;
}


SoundDevice::Caps Manager::GetDeviceCaps(SoundDevice::Identifier identifier, SoundDevice::IBase *currentSoundDevice)
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	if(m_DeviceCaps.find(identifier) == m_DeviceCaps.end())
	{
		if(currentSoundDevice && FindDeviceInfo(identifier).IsValid() && (currentSoundDevice->GetDeviceInfo().GetIdentifier() == identifier))
		{
			m_DeviceCaps[identifier] = currentSoundDevice->GetDeviceCaps();
		} else
		{
			SoundDevice::IBase *dummy = CreateSoundDevice(identifier);
			if(dummy)
			{
				m_DeviceCaps[identifier] = dummy->GetDeviceCaps();
			} else
			{
				SetDeviceUnavailable(identifier);
			}
			delete dummy;
		}
	}
	return m_DeviceCaps[identifier];
}


SoundDevice::DynamicCaps Manager::GetDeviceDynamicCaps(SoundDevice::Identifier identifier, const std::vector<uint32> &baseSampleRates, SoundDevice::IMessageReceiver *messageReceiver, SoundDevice::IBase *currentSoundDevice, bool update)
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	if((m_DeviceDynamicCaps.find(identifier) == m_DeviceDynamicCaps.end()) || update)
	{
		if(currentSoundDevice && FindDeviceInfo(identifier).IsValid() && (currentSoundDevice->GetDeviceInfo().GetIdentifier() == identifier))
		{
			m_DeviceDynamicCaps[identifier] = currentSoundDevice->GetDeviceDynamicCaps(baseSampleRates);
			if(!currentSoundDevice->IsAvailable())
			{
				SetDeviceUnavailable(identifier);
			}
		} else
		{
			SoundDevice::IBase *dummy = CreateSoundDevice(identifier);
			if(dummy)
			{
				dummy->SetMessageReceiver(messageReceiver);
				m_DeviceDynamicCaps[identifier] = dummy->GetDeviceDynamicCaps(baseSampleRates);
				if(!dummy->IsAvailable())
				{
					SetDeviceUnavailable(identifier);
				}
			} else
			{
				SetDeviceUnavailable(identifier);
			}
			delete dummy;
		}
	}
	return m_DeviceDynamicCaps[identifier];
}


SoundDevice::IBase * Manager::CreateSoundDevice(SoundDevice::Identifier identifier)
{
	MPT_SOUNDDEV_TRACE_SCOPE();
	const SoundDevice::Info info = FindDeviceInfo(identifier);
	if(!info.IsValid())
	{
		return nullptr;
	}
	if(m_DeviceFactoryMethods.find(identifier) == m_DeviceFactoryMethods.end())
	{
		return nullptr;
	}
	if(!m_DeviceFactoryMethods[identifier])
	{
		return nullptr;
	}
	SoundDevice::IBase *result = m_DeviceFactoryMethods[identifier](GetLogger(), info, GetSysInfo());
	if(!result)
	{
		return nullptr;
	}
	if(!result->Init(m_AppInfo))
	{
		delete result;
		result = nullptr;
		return nullptr;
	}
	m_DeviceCaps[identifier] = result->GetDeviceCaps(); // update cached caps
	return result;
}


Manager::Manager(ILogger &logger, SoundDevice::SysInfo sysInfo, SoundDevice::AppInfo appInfo, EnabledBackends enabledBackends)
	: m_Logger(logger)
	, m_SysInfo(sysInfo)
	, m_AppInfo(appInfo)
	, m_EnabledBackends(enabledBackends)
#ifdef MPT_WITH_PORTAUDIO
	, m_PortAudioInitializer(m_EnabledBackends.PortAudio ? std::make_unique<PortAudioInitializer>() : nullptr)
#endif // MPT_WITH_PORTAUDIO
{
	ReEnumerate(true);
}


Manager::~Manager()
{
	return;
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
