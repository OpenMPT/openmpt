/*
 * SoundDevicePulseaudio.h
 * -----------------------
 * Purpose: PulseAudio sound device driver class.
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDeviceBase.h"
#include "SoundDeviceUtilities.h"

#include "../common/ComponentManager.h"

#if defined(MPT_ENABLE_PULSEAUDIO_FULL)
#if defined(MPT_WITH_PULSEAUDIO)
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#endif // MPT_WITH_PULSEAUDIO
#endif // MPT_ENABLE_PULSEAUDIO_FULL


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if defined(MPT_ENABLE_PULSEAUDIO_FULL)

#if defined(MPT_WITH_PULSEAUDIO)


class ComponentPulseaudio
	: public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentPulseaudio() { }
	virtual ~ComponentPulseaudio() { }
	virtual bool DoInitialize() { return true; }
};


//==============
class Pulseaudio
//==============
	: public ThreadBase
{
private:
	static mpt::ustring PulseErrorString(int error);
public:
	static std::vector<SoundDevice::Info> EnumerateDevices(SoundDevice::SysInfo sysInfo);
public:
	Pulseaudio(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	SoundDevice::Caps InternalGetDeviceCaps();
	SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates);
	bool InternalIsOpen() const;
	bool InternalOpen();
	void InternalStartFromSoundThread();
	void InternalFillAudioBuffer();
	void InternalWaitFromSoundThread();
	SoundDevice::BufferAttributes InternalGetEffectiveBufferAttributes() const;
	SoundDevice::Statistics GetStatistics() const;
	void InternalStopFromSoundThread();
	bool InternalClose();
	~Pulseaudio();
private:
	pa_simple * m_PA_SimpleOutput;
	SoundDevice::BufferAttributes m_EffectiveBufferAttributes;
	std::vector<float32> m_OutputBuffer;
	std::atomic<uint32> m_StatisticLastLatencyFrames;
};


#endif // MPT_WITH_PULSEAUDIO

#endif // MPT_ENABLE_PULSEAUDIO_FULL


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
