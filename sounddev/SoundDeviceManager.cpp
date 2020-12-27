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
#include "SoundDevicePortAudio.h"
#include "SoundDeviceRtAudio.h"
#include "SoundDeviceWaveout.h"
#include "SoundDeviceStub.h"
#if defined(MPT_ENABLE_PULSEAUDIO_FULL)
#include "SoundDevicePulseaudio.h"
#endif // MPT_ENABLE_PULSEAUDIO_FULL
#include "SoundDevicePulseSimple.h"


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
void Manager::EnumerateDevices(SoundDevice::SysInfo sysInfo)
{
	const auto infos = Tdevice::EnumerateDevices(sysInfo);
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
SoundDevice::IBase* Manager::ConstructSoundDevice(const SoundDevice::Info &info, SoundDevice::SysInfo sysInfo)
{
	return new Tdevice(info, sysInfo);
}


void Manager::ReEnumerate()
{
	MPT_TRACE_SCOPE();
	m_SoundDevices.clear();
	m_DeviceUnavailable.clear();
	m_DeviceFactoryMethods.clear();
	m_DeviceCaps.clear();
	m_DeviceDynamicCaps.clear();

#ifdef MPT_WITH_PORTAUDIO
	m_PortAudio.Reload();
#endif // MPT_WITH_PORTAUDIO

#if defined(MPT_ENABLE_PULSEAUDIO_FULL)
#if defined(MPT_WITH_PULSEAUDIO)
	if(IsComponentAvailable(m_Pulseaudio))
	{
		EnumerateDevices<Pulseaudio>(GetSysInfo());
	}
#endif // MPT_WITH_PULSEAUDIO
#endif // MPT_ENABLE_PULSEAUDIO_FULL

#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
	if(IsComponentAvailable(m_PulseaudioSimple))
	{
		EnumerateDevices<PulseaudioSimple>(GetSysInfo());
	}
#endif // MPT_WITH_PULSEAUDIO && MPT_WITH_PULSEAUDIOSIMPLE

#if MPT_OS_WINDOWS
	if(IsComponentAvailable(m_WaveOut))
	{
		EnumerateDevices<CWaveDevice>(GetSysInfo());
	}
#endif // MPT_OS_WINDOWS

#ifdef MPT_WITH_ASIO
	if(IsComponentAvailable(m_ASIO))
	{
		EnumerateDevices<CASIODevice>(GetSysInfo());
	}
#endif // MPT_WITH_ASIO

#ifdef MPT_WITH_PORTAUDIO
	if(IsComponentAvailable(m_PortAudio))
	{
		EnumerateDevices<CPortaudioDevice>(GetSysInfo());
	}
#endif // MPT_WITH_PORTAUDIO

#ifdef MPT_WITH_RTAUDIO
	if(IsComponentAvailable(m_RtAudio))
	{
		EnumerateDevices<CRtAudioDevice>(GetSysInfo());
	}
#endif // MPT_WITH_RTAUDIO

#ifndef MPT_BUILD_WINESUPPORT
	{
		EnumerateDevices<SoundDeviceStub>(GetSysInfo());
	}
#endif // !MPT_BUILD_WINESUPPORT

	struct Default
	{
		SoundDevice::Info::DefaultFor value = SoundDevice::Info::DefaultFor::None;
	};

	std::map<SoundDevice::Type, Default> typeDefault;
#ifdef MPT_BUILD_WINESUPPORT
	if(GetSysInfo().SystemClass == mpt::OS::Class::Linux)
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
	} else if(GetSysInfo().SystemClass == mpt::OS::Class::Darwin)
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
	} else if(GetSysInfo().SystemClass == mpt::OS::Class::BSD)
	{
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paOSS)].value = Info::DefaultFor::System;
#endif
#if defined(MPT_WITH_RTAUDIO)
		typeDefault[MPT_UFORMAT("RtAudio-{}")(U_("oss"))].value = Info::DefaultFor::System;
#endif
	} else if(GetSysInfo().SystemClass == mpt::OS::Class::Haiku)
	{
#if defined(MPT_WITH_PORTAUDIO)
		typeDefault[MPT_UFORMAT("PortAudio-{}")(paBeOS)].value = Info::DefaultFor::System;
#endif
	} else
#endif
	if(GetSysInfo().SystemClass == mpt::OS::Class::Windows && GetSysInfo().IsWindowsWine() && GetSysInfo().WineHostClass == mpt::OS::Class::Linux)
	{ // Wine on Linux
		typeDefault[SoundDevice::TypePORTAUDIO_WASAPI].value = Info::DefaultFor::System;
	} else if(GetSysInfo().SystemClass == mpt::OS::Class::Windows && GetSysInfo().IsWindowsWine())
	{ // Wine
		typeDefault[SoundDevice::TypePORTAUDIO_WASAPI].value = Info::DefaultFor::System;
		typeDefault[SoundDevice::TypeDSOUND].value = Info::DefaultFor::LowLevel;
	} else if(GetSysInfo().SystemClass == mpt::OS::Class::Windows && GetSysInfo().WindowsVersion.IsBefore(mpt::OS::Windows::Version::WinVista))
	{ // WinXP
		typeDefault[SoundDevice::TypeWAVEOUT].value = Info::DefaultFor::System;
		typeDefault[SoundDevice::TypeASIO].value = Info::DefaultFor::ProAudio;
		typeDefault[SoundDevice::TypePORTAUDIO_WDMKS].value = Info::DefaultFor::LowLevel;
	} else if(GetSysInfo().SystemClass == mpt::OS::Class::Windows && GetSysInfo().WindowsVersion.IsBefore(mpt::OS::Windows::Version::Win7))
	{ // Vista
		typeDefault[SoundDevice::TypeWAVEOUT].value = Info::DefaultFor::System;
		typeDefault[SoundDevice::TypeASIO].value = Info::DefaultFor::ProAudio;
		typeDefault[SoundDevice::TypePORTAUDIO_WDMKS].value = Info::DefaultFor::LowLevel;
	} else if(GetSysInfo().SystemClass == mpt::OS::Class::Windows)
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

	MPT_LOG(LogDebug, "sounddev", MPT_UFORMAT("Sound Devices enumerated:")());
	for(const auto &device : m_SoundDevices)
	{
		MPT_LOG(LogDebug, "sounddev", MPT_UFORMAT(" Identifier : {}")(device.GetIdentifier()));
		MPT_LOG(LogDebug, "sounddev", MPT_UFORMAT("  Type      : {}")(device.type));
		MPT_LOG(LogDebug, "sounddev", MPT_UFORMAT("  InternalID: {}")(device.internalID));
		MPT_LOG(LogDebug, "sounddev", MPT_UFORMAT("  API Name  : {}")(device.apiName));
		MPT_LOG(LogDebug, "sounddev", MPT_UFORMAT("  Name      : {}")(device.name));
		for(const auto &extra : device.extraData)
		{
			MPT_LOG(LogDebug, "sounddev", MPT_UFORMAT("  Extra Data: {} = {}")(extra.first, extra.second));
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
	MPT_TRACE_SCOPE();
	if(id > m_SoundDevices.size())
	{
		return SoundDevice::Info();
	}
	return m_SoundDevices[id];
}


SoundDevice::Info Manager::FindDeviceInfo(SoundDevice::Identifier identifier) const
{
	MPT_TRACE_SCOPE();
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
	MPT_TRACE_SCOPE();
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
	MPT_TRACE_SCOPE();
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
	MPT_TRACE_SCOPE();
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
	MPT_TRACE_SCOPE();
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
	MPT_TRACE_SCOPE();
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
	SoundDevice::IBase *result = m_DeviceFactoryMethods[identifier](info, GetSysInfo());
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


Manager::Manager(SoundDevice::SysInfo sysInfo, SoundDevice::AppInfo appInfo)
	: m_SysInfo(sysInfo)
	, m_AppInfo(appInfo)
{
	ReEnumerate();
}


Manager::~Manager()
{
	return;
}


namespace Legacy
{
SoundDevice::Info FindDeviceInfo(SoundDevice::Manager &manager, SoundDevice::Legacy::ID id)
{
	if(manager.GetDeviceInfos().empty())
	{
		return SoundDevice::Info();
	}
	SoundDevice::Type type = SoundDevice::Type();
	switch((id & SoundDevice::Legacy::MaskType) >> SoundDevice::Legacy::ShiftType)
	{
		case SoundDevice::Legacy::TypeWAVEOUT:
			type = SoundDevice::TypeWAVEOUT;
			break;
		case SoundDevice::Legacy::TypeDSOUND:
			type = SoundDevice::TypeDSOUND;
			break;
		case SoundDevice::Legacy::TypeASIO:
			type = SoundDevice::TypeASIO;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_WASAPI:
			type = SoundDevice::TypePORTAUDIO_WASAPI;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_WDMKS:
			type = SoundDevice::TypePORTAUDIO_WDMKS;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_WMME:
			type = SoundDevice::TypePORTAUDIO_WMME;
			break;
		case SoundDevice::Legacy::TypePORTAUDIO_DS:
			type = SoundDevice::TypePORTAUDIO_DS;
			break;
	}
	if(type.empty())
	{	// fallback to first device
		return *manager.begin();
	}
	std::size_t index = static_cast<uint8>((id & SoundDevice::Legacy::MaskIndex) >> SoundDevice::Legacy::ShiftIndex);
	std::size_t seenDevicesOfDesiredType = 0;
	for(const auto &info : manager)
	{
		if(info.type == type)
		{
			if(seenDevicesOfDesiredType == index)
			{
				if(!info.IsValid())
				{	// fallback to first device
					return *manager.begin();
				}
				return info;
			}
			seenDevicesOfDesiredType++;
		}
	}
	// default to first device
	return *manager.begin();
}
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
