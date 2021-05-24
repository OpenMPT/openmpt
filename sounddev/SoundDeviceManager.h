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

#include "openmpt/all/BuildSettings.hpp"

#include "openmpt/logging/Logger.hpp"

#include "SoundDevice.h"

#include <map>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


struct EnabledBackends
{
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_ENABLE_PULSEAUDIO_FULL)
	bool Pulseaudio = true;
#endif // MPT_WITH_PULSEAUDIO && MPT_ENABLE_PULSEAUDIO_FULL
#if defined(MPT_WITH_PULSEAUDIO) && defined(MPT_WITH_PULSEAUDIOSIMPLE)
	bool PulseaudioSimple = true;
#endif // MPT_WITH_PULSEAUDIO && MPT_WITH_PULSEAUDIOSIMPLE
#if MPT_OS_WINDOWS
	bool WaveOut = true;
#endif // MPT_OS_WINDOWS
#if defined(MPT_WITH_DIRECTSOUND)
	bool DirectSound = true;
#endif // MPT_WITH_DIRECTSOUND
#ifdef MPT_WITH_ASIO
	bool ASIO = true;
#endif // MPT_WITH_ASIO
#ifdef MPT_WITH_PORTAUDIO
	bool PortAudio = true;
#endif // MPT_WITH_PORTAUDIO
#ifdef MPT_WITH_RTAUDIO
	bool RtAudio = true;
#endif // MPT_WITH_RTAUDIO
};


#ifdef MPT_WITH_PORTAUDIO
class PortAudioInitializer;
#endif // MPT_WITH_PORTAUDIO


class Manager
{

public:

	typedef std::size_t GlobalID;

private:

	typedef SoundDevice::IBase* (*CreateSoundDeviceFunc)(ILogger &logger, const SoundDevice::Info &info, SoundDevice::SysInfo sysInfo);

private:

	ILogger &m_Logger;
	const SoundDevice::SysInfo m_SysInfo;
	const SoundDevice::AppInfo m_AppInfo;

	const EnabledBackends m_EnabledBackends;

#ifdef MPT_WITH_PORTAUDIO
	std::unique_ptr<PortAudioInitializer> m_PortAudioInitializer;
#endif // MPT_WITH_PORTAUDIO

	std::vector<SoundDevice::Info> m_SoundDevices;
	std::map<SoundDevice::Identifier, bool> m_DeviceUnavailable;
	std::map<SoundDevice::Identifier, CreateSoundDeviceFunc> m_DeviceFactoryMethods;
	std::map<SoundDevice::Identifier, SoundDevice::Caps> m_DeviceCaps;
	std::map<SoundDevice::Identifier, SoundDevice::DynamicCaps> m_DeviceDynamicCaps;

public:

	Manager(ILogger &logger, SoundDevice::SysInfo sysInfo, SoundDevice::AppInfo appInfo, EnabledBackends enabledBackends = EnabledBackends{});
	~Manager();

private:

	template <typename Tdevice> void EnumerateDevices(ILogger &logger, SoundDevice::SysInfo sysInfo);
	template <typename Tdevice> static SoundDevice::IBase* ConstructSoundDevice(ILogger &logger, const SoundDevice::Info &info, SoundDevice::SysInfo sysInfo);

public:

	ILogger &GetLogger() const { return m_Logger; }
	SoundDevice::SysInfo GetSysInfo() const { return m_SysInfo; }
	SoundDevice::AppInfo GetAppInfo() const { return m_AppInfo; }

	void ReEnumerate(bool firstRun = false);

	std::vector<SoundDevice::Info>::const_iterator begin() const { return m_SoundDevices.begin(); }
	std::vector<SoundDevice::Info>::const_iterator end() const { return m_SoundDevices.end(); }
	const std::vector<SoundDevice::Info> & GetDeviceInfos() const { return m_SoundDevices; }

	SoundDevice::Manager::GlobalID GetGlobalID(SoundDevice::Identifier identifier) const;

	SoundDevice::Info FindDeviceInfo(SoundDevice::Manager::GlobalID id) const;
	SoundDevice::Info FindDeviceInfo(SoundDevice::Identifier identifier) const;
	SoundDevice::Info FindDeviceInfoBestMatch(SoundDevice::Identifier identifier);

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
