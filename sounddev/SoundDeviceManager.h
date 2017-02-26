/*
 * SoundDeviceManager.h
 * --------------------
 * Purpose: Sound device manager class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDevice.h"

#include "../common/ComponentManager.h"

#include <map>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if defined(MPT_ENABLE_PULSEAUDIO_FULL)
class ComponentPulseaudio;
#endif // MPT_ENABLE_PULSEAUDIO_FULL
class ComponentPulseaudioSimple;
class ComponentWaveOut;
class ComponentDirectSound;
class ComponentASIO;
class ComponentPortAudio;


//===========
class Manager
//===========
{

public:

	typedef std::size_t GlobalID;

private:

	typedef SoundDevice::IBase* (*CreateSoundDeviceFunc)(const SoundDevice::Info &info, SoundDevice::SysInfo sysInfo);

private:

	const SoundDevice::SysInfo m_SysInfo;
	const SoundDevice::AppInfo m_AppInfo;

#if defined(MPT_ENABLE_PULSEAUDIO_FULL)
#if defined(MPT_WITH_PULSEAUDIO)
	ComponentHandle<ComponentPulseaudio> m_Pulseaudio;
#endif // MPT_WITH_PULSEAUDIO
#endif // MPT_ENABLE_PULSEAUDIO_FULL
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
	ComponentHandle<ComponentPulseaudioSimple> m_PulseaudioSimple;
#endif // MPT_WITH_PULSEAUDIO && MPT_WITH_PULSEAUDIOSIMPLE
#if MPT_OS_WINDOWS
	ComponentHandle<ComponentWaveOut> m_WaveOut;
#endif // MPT_OS_WINDOWS
#ifdef MPT_WITH_DSOUND
	ComponentHandle<ComponentDirectSound> m_DirectSound;
#endif // MPT_WITH_DSOUND
#ifdef MPT_WITH_ASIO
	ComponentHandle<ComponentASIO> m_ASIO;
#endif // MPT_WITH_ASIO
#ifdef MPT_WITH_PORTAUDIO
	ComponentHandle<ComponentPortAudio> m_PortAudio;
#endif // MPT_WITH_PORTAUDIO

	std::vector<SoundDevice::Info> m_SoundDevices;
	std::map<SoundDevice::Identifier, bool> m_DeviceUnavailable;
	std::map<SoundDevice::Identifier, CreateSoundDeviceFunc> m_DeviceFactoryMethods;
	std::map<SoundDevice::Identifier, SoundDevice::Caps> m_DeviceCaps;
	std::map<SoundDevice::Identifier, SoundDevice::DynamicCaps> m_DeviceDynamicCaps;

public:

	Manager(SoundDevice::SysInfo sysInfo, SoundDevice::AppInfo appInfo);
	~Manager();

private:

	template <typename Tdevice> void EnumerateDevices(SoundDevice::SysInfo sysInfo);
	template <typename Tdevice> static SoundDevice::IBase* ConstructSoundDevice(const SoundDevice::Info &info, SoundDevice::SysInfo sysInfo);

public:

	SoundDevice::SysInfo GetSysInfo() const { return m_SysInfo; }
	SoundDevice::AppInfo GetAppInfo() const { return m_AppInfo; }

	void ReEnumerate();

	std::vector<SoundDevice::Info>::const_iterator begin() const { return m_SoundDevices.begin(); }
	std::vector<SoundDevice::Info>::const_iterator end() const { return m_SoundDevices.end(); }
	const std::vector<SoundDevice::Info> & GetDeviceInfos() const { return m_SoundDevices; }

	SoundDevice::Manager::GlobalID GetGlobalID(SoundDevice::Identifier identifier) const;

	SoundDevice::Info FindDeviceInfo(SoundDevice::Manager::GlobalID id) const;
	SoundDevice::Info FindDeviceInfo(SoundDevice::Identifier identifier) const;
	SoundDevice::Info FindDeviceInfoBestMatch(SoundDevice::Identifier identifier, bool preferSameType);

	bool OpenDriverSettings(SoundDevice::Identifier identifier, SoundDevice::IMessageReceiver *messageReceiver = nullptr, SoundDevice::IBase *currentSoundDevice = nullptr);

	void SetDeviceUnavailable(SoundDevice::Identifier identifier) { m_DeviceUnavailable[identifier] = true; }
	bool IsDeviceUnavailable(SoundDevice::Identifier identifier) { return m_DeviceUnavailable[identifier]; }

	SoundDevice::Caps GetDeviceCaps(SoundDevice::Identifier identifier, SoundDevice::IBase *currentSoundDevice = nullptr);
	SoundDevice::DynamicCaps GetDeviceDynamicCaps(SoundDevice::Identifier identifier, const std::vector<uint32> &baseSampleRates, SoundDevice::IMessageReceiver *messageReceiver = nullptr, SoundDevice::IBase *currentSoundDevice = nullptr, bool update = false);

	SoundDevice::IBase * CreateSoundDevice(SoundDevice::Identifier identifier);

};


namespace Legacy
{
SoundDevice::Info FindDeviceInfo(SoundDevice::Manager &manager, SoundDevice::Legacy::ID id);
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
