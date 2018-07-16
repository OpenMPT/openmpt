/*
 * SoundDeviceRtAudio.h
 * --------------------
 * Purpose: RtAudio sound device driver class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "SoundDeviceBase.h"

#include "../common/ComponentManager.h"

#ifdef MPT_WITH_RTAUDIO
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4244) // conversion from 'int' to 'unsigned char', possible loss of data
#endif
#include <RtAudio.h>
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif
#endif // MPT_WITH_RTAUDIO

OPENMPT_NAMESPACE_BEGIN

namespace SoundDevice {


#ifdef MPT_WITH_RTAUDIO


class CRtAudioDevice: public SoundDevice::Base
{

protected:

	std::unique_ptr<RtAudio> m_RtAudio;

	RtAudio::StreamParameters m_InputStreamParameters;
	RtAudio::StreamParameters m_OutputStreamParameters;
	unsigned int m_FramesPerChunk;
	RtAudio::StreamOptions m_StreamOptions;

	void * m_CurrentFrameBufferOutput;
	void * m_CurrentFrameBufferInput;
	unsigned int m_CurrentFrameBufferCount;
	double m_CurrentStreamTime;

	std::atomic<uint32> m_StatisticLatencyFrames;
	std::atomic<uint32> m_StatisticPeriodFrames;

public:

	CRtAudioDevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	~CRtAudioDevice();

public:

	bool InternalOpen();
	bool InternalClose();
	void InternalFillAudioBuffer();
	bool InternalStart();
	void InternalStop();
	bool InternalIsOpen() const { return m_RtAudio && m_RtAudio->isStreamOpen(); }
	bool InternalHasGetStreamPosition() const { return true; }
	int64 InternalGetStreamPositionFrames() const;
	SoundDevice::BufferAttributes InternalGetEffectiveBufferAttributes() const;
	SoundDevice::Statistics GetStatistics() const;
	SoundDevice::Caps InternalGetDeviceCaps();
	SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates);

private:

	void SendError(const RtAudioError &e);

	void AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status);

	static int RtAudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData);

	static RtAudio::Api GetApi(SoundDevice::Info info);
	static unsigned int GetDevice(SoundDevice::Info info);

public:

	static std::vector<SoundDevice::Info> EnumerateDevices(SoundDevice::SysInfo sysInfo);

};


class ComponentRtAudio : public ComponentBuiltin
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentRtAudio();
	virtual ~ComponentRtAudio();
	virtual bool DoInitialize();
};


#endif // MPT_WITH_RTAUDIO


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
