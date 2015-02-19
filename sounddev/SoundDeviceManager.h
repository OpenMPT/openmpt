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


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


class ComponentPortAudio;


//===========
class Manager
//===========
{
private:

	const SoundDevice::AppInfo m_AppInfo;

#ifndef NO_PORTAUDIO
	ComponentHandle<ComponentPortAudio> m_PortAudio;
#endif // NO_PORTAUDIO

	std::vector<SoundDevice::Info> m_SoundDevices;
	std::map<SoundDevice::Identifier, bool> m_DeviceUnavailable;
	std::map<SoundDevice::Identifier, SoundDevice::Caps> m_DeviceCaps;
	std::map<SoundDevice::Identifier, SoundDevice::DynamicCaps> m_DeviceDynamicCaps;

public:
	Manager(SoundDevice::AppInfo appInfo, SoundDevice::TypesSet enabledTypes);
	~Manager();

public:

	void ReEnumerate(SoundDevice::TypesSet enabledTypes);

	std::vector<SoundDevice::Info>::const_iterator begin() const { return m_SoundDevices.begin(); }
	std::vector<SoundDevice::Info>::const_iterator end() const { return m_SoundDevices.end(); }
	const std::vector<SoundDevice::Info> & GetDeviceInfos() const { return m_SoundDevices; }

	SoundDevice::Info FindDeviceInfo(SoundDevice::ID id) const;
	SoundDevice::Info FindDeviceInfo(SoundDevice::Identifier identifier) const;
	SoundDevice::Info FindDeviceInfoBestMatch(SoundDevice::Identifier identifier, bool preferSameType);

	bool OpenDriverSettings(SoundDevice::Identifier identifier, SoundDevice::IMessageReceiver *messageReceiver = nullptr, SoundDevice::IBase *currentSoundDevice = nullptr);

	void SetDeviceUnavailable(SoundDevice::Identifier identifier) { m_DeviceUnavailable[identifier] = true; }
	bool IsDeviceUnavailable(SoundDevice::Identifier identifier) { return m_DeviceUnavailable[identifier]; }

	SoundDevice::Caps GetDeviceCaps(SoundDevice::Identifier identifier, SoundDevice::IBase *currentSoundDevice = nullptr);
	SoundDevice::DynamicCaps GetDeviceDynamicCaps(SoundDevice::Identifier identifier, const std::vector<uint32> &baseSampleRates, SoundDevice::IMessageReceiver *messageReceiver = nullptr, SoundDevice::IBase *currentSoundDevice = nullptr, bool update = false);

	SoundDevice::IBase * CreateSoundDevice(SoundDevice::Identifier identifier);

};


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
