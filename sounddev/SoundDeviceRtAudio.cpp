/*
 * SoundDeviceRtAudio.cpp
 * ----------------------
 * Purpose: RtAudio sound device driver class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"

#include "SoundDeviceRtAudio.h"

#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#ifdef MPT_WITH_RTAUDIO


static mpt::ustring RtAudioApiToString(RtAudio::Api api)
{
	mpt::ustring result;
	switch(api)
	{
	case RtAudio::UNSPECIFIED: result = U_("UNSPECIFIED"); break;
	case RtAudio::LINUX_ALSA: result = U_("LINUX_ALSA"); break;
	case RtAudio::LINUX_PULSE: result = U_("LINUX_PULSE"); break;
	case RtAudio::LINUX_OSS: result = U_("LINUX_OSS"); break;
	case RtAudio::UNIX_JACK: result = U_("UNIX_JACK"); break;
	case RtAudio::MACOSX_CORE: result = U_("MACOSX_CORE"); break;
	case RtAudio::WINDOWS_WASAPI: result = U_("WINDOWS_WASAPI"); break;
	case RtAudio::WINDOWS_ASIO: result = U_("WINDOWS_ASIO"); break;
	case RtAudio::WINDOWS_DS: result = U_("WINDOWS_DS"); break;
	case RtAudio::RTAUDIO_DUMMY: result = U_("RTAUDIO_DUMMY"); break;
	default: result = U_(""); break;
	}
	return result;
}


static mpt::ustring RtAudioApiToDescription(RtAudio::Api api)
{
	mpt::ustring result;
	switch(api)
	{
	case RtAudio::UNSPECIFIED: result = U_("default"); break;
	case RtAudio::LINUX_ALSA: result = U_("ALSA"); break;
	case RtAudio::LINUX_PULSE: result = U_("PulseAudio"); break;
	case RtAudio::LINUX_OSS: result = U_("OSS"); break;
	case RtAudio::UNIX_JACK: result = U_("Jack"); break;
	case RtAudio::MACOSX_CORE: result = U_("CoreAudio"); break;
	case RtAudio::WINDOWS_WASAPI: result = U_("WASAPI"); break;
	case RtAudio::WINDOWS_ASIO: result = U_("ASIO"); break;
	case RtAudio::WINDOWS_DS: result = U_("DirectSound"); break;
	case RtAudio::RTAUDIO_DUMMY: result = U_("Dummy"); break;
	default: result = U_(""); break;
	}
	return result;
}


static RtAudio::Api StringToRtAudioApi(const mpt::ustring &str)
{
	RtAudio::Api result = RtAudio::RTAUDIO_DUMMY;
	if(str == U_("")) result = RtAudio::RTAUDIO_DUMMY;
	if(str == U_("UNSPECIFIED")) result = RtAudio::UNSPECIFIED;
	if(str == U_("LINUX_ALSA")) result = RtAudio::LINUX_ALSA;
	if(str == U_("LINUX_PULSE")) result = RtAudio::LINUX_PULSE;
	if(str == U_("LINUX_OSS")) result = RtAudio::LINUX_OSS;
	if(str == U_("UNIX_JACK")) result = RtAudio::UNIX_JACK;
	if(str == U_("MACOSX_CORE")) result = RtAudio::MACOSX_CORE;
	if(str == U_("WINDOWS_WASAPI")) result = RtAudio::WINDOWS_WASAPI;
	if(str == U_("WINDOWS_ASIO")) result = RtAudio::WINDOWS_ASIO;
	if(str == U_("WINDOWS_DS")) result = RtAudio::WINDOWS_DS;
	if(str == U_("RTAUDIO_DUMMY")) result = RtAudio::RTAUDIO_DUMMY;
	return result;
}


static RtAudioFormat SampleFormatToRtAudioFormat(SampleFormat sampleFormat)
{
	RtAudioFormat result = RtAudioFormat();
	if(sampleFormat.IsFloat())
	{
		switch(sampleFormat.GetBitsPerSample())
		{
		case 32: result = RTAUDIO_FLOAT32; break;
		case 64: result = RTAUDIO_FLOAT64; break;
		}
	} else if(sampleFormat.IsInt())
	{
		switch(sampleFormat.GetBitsPerSample())
		{
		case 16: result = RTAUDIO_SINT16; break;
		case 24: result = RTAUDIO_SINT24; break;
		case 32: result = RTAUDIO_SINT32; break;
		}
	}
	return result;
}


CRtAudioDevice::CRtAudioDevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
	: SoundDevice::Base(info, sysInfo)
	, m_RtAudio(std::unique_ptr<RtAudio>())
	, m_FramesPerChunk(0)
{
	m_CurrentFrameBufferOutput = nullptr;
	m_CurrentFrameBufferInput = nullptr;
	m_CurrentFrameBufferCount = 0;
	m_CurrentStreamTime = 0.0;
	m_StatisticLatencyFrames.store(0);
	m_StatisticPeriodFrames.store(0);
	try
	{
		m_RtAudio = mpt::make_unique<RtAudio>(GetApi(info));
	} catch (const RtAudioError &)
	{
		// nothing
	}
}


CRtAudioDevice::~CRtAudioDevice()
{
	Close();
}


bool CRtAudioDevice::InternalOpen()
{
	try
	{
		if(SampleFormatToRtAudioFormat(m_Settings.sampleFormat) == RtAudioFormat())
		{
			return false;
		}
		if(ChannelMapping::BaseChannel(m_Settings.Channels, m_Settings.Channels.ToDevice(0)) != m_Settings.Channels)
		{ // only simple base channel mappings are supported
			return false;
		}
		m_OutputStreamParameters.deviceId = GetDevice(GetDeviceInfo());
		m_OutputStreamParameters.nChannels = m_Settings.Channels;
		m_OutputStreamParameters.firstChannel = m_Settings.Channels.ToDevice(0);
		m_InputStreamParameters.deviceId = GetDevice(GetDeviceInfo());
		m_InputStreamParameters.nChannels = m_Settings.InputChannels;
		m_InputStreamParameters.firstChannel = m_Settings.InputSourceID;
		m_FramesPerChunk = mpt::saturate_round<int>(m_Settings.UpdateInterval * m_Settings.Samplerate);
		m_StreamOptions.flags = RtAudioStreamFlags();
		m_StreamOptions.numberOfBuffers = mpt::saturate_round<int>(m_Settings.Latency * m_Settings.Samplerate / m_FramesPerChunk);
		m_StreamOptions.priority = 0;
		m_StreamOptions.streamName = mpt::ToCharset(mpt::CharsetUTF8, m_AppInfo.GetName());
		if(m_Settings.BoostThreadPriority)
		{
			m_StreamOptions.flags |= RTAUDIO_SCHEDULE_REALTIME;
			m_StreamOptions.priority = m_AppInfo.BoostedThreadPriorityXP;
		}
		if(m_Settings.ExclusiveMode)
		{
			//m_FramesPerChunk = 0; // auto
			m_StreamOptions.flags |= RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_HOG_DEVICE;
			m_StreamOptions.numberOfBuffers = 2;
		}
		m_RtAudio->openStream((m_OutputStreamParameters.nChannels > 0) ? &m_OutputStreamParameters : nullptr, (m_InputStreamParameters.nChannels > 0) ? &m_InputStreamParameters : nullptr, SampleFormatToRtAudioFormat(m_Settings.sampleFormat), m_Settings.Samplerate, &m_FramesPerChunk, &RtAudioCallback, this, &m_StreamOptions, nullptr);
	} catch (const RtAudioError &e)
	{
		SendError(e);
		return false;
	}
	return true;
}


bool CRtAudioDevice::InternalClose()
{
	try
	{
		m_RtAudio->closeStream();
	} catch (const RtAudioError &e)
	{
		SendError(e);
		return false;
	}
	return true;
}


bool CRtAudioDevice::InternalStart()
{
	try
	{
		m_RtAudio->startStream();
	} catch (const RtAudioError &e)
	{
		SendError(e);
		return false;
	}
	return true;
}


void CRtAudioDevice::InternalStop()
{
	try
	{
		m_RtAudio->stopStream();
	} catch (const RtAudioError &e)
	{
		SendError(e);
		return;
	}
	return;
}


void CRtAudioDevice::InternalFillAudioBuffer()
{
	if(m_CurrentFrameBufferCount == 0)
	{
		return;
	}
	SourceLockedAudioPreRead(m_CurrentFrameBufferCount, m_FramesPerChunk * m_StreamOptions.numberOfBuffers);
	SourceLockedAudioRead(m_CurrentFrameBufferOutput, m_CurrentFrameBufferInput, m_CurrentFrameBufferCount);
	m_StatisticLatencyFrames.store(m_CurrentFrameBufferCount * m_StreamOptions.numberOfBuffers);
	m_StatisticPeriodFrames.store(m_CurrentFrameBufferCount);
	SourceLockedAudioDone();
}


int64 CRtAudioDevice::InternalGetStreamPositionFrames() const
{
	return mpt::saturate_round<int64>(m_RtAudio->getStreamTime() * m_RtAudio->getStreamSampleRate());
}


SoundDevice::BufferAttributes CRtAudioDevice::InternalGetEffectiveBufferAttributes() const
{
	SoundDevice::BufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_FramesPerChunk * m_StreamOptions.numberOfBuffers / static_cast<double>(m_Settings.Samplerate);
	bufferAttributes.UpdateInterval = m_FramesPerChunk / static_cast<double>(m_Settings.Samplerate);
	bufferAttributes.NumBuffers = m_StreamOptions.numberOfBuffers;
	return bufferAttributes;
}


int CRtAudioDevice::RtAudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	reinterpret_cast<CRtAudioDevice*>(userData)->AudioCallback(outputBuffer, inputBuffer, nFrames, streamTime, status);
	return 0; // continue
}


void CRtAudioDevice::AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status)
{
	m_CurrentFrameBufferOutput = outputBuffer;
	m_CurrentFrameBufferInput = inputBuffer;
	m_CurrentFrameBufferCount = nFrames;
	m_CurrentStreamTime = streamTime;
	SourceFillAudioBufferLocked();
	m_CurrentFrameBufferCount = 0;
	m_CurrentFrameBufferOutput = 0;
	m_CurrentFrameBufferInput = 0;
	if(status != RtAudioStreamStatus())
	{
		// maybe
		// RequestRestart();
	}
}


SoundDevice::Statistics CRtAudioDevice::GetStatistics() const
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	long latency = 0;
	try
	{
		if(m_RtAudio->isStreamOpen())
		{
			latency = m_RtAudio->getStreamLatency();
			if(m_Settings.InputChannels > 0 && m_Settings.Channels > 0)
			{
				latency /= 2;
			}
		}
	} catch(const RtAudioError &)
	{
		latency = 0;
	}
	if(latency > 0)
	{
		result.InstantaneousLatency = latency / static_cast<double>(m_Settings.Samplerate);
		result.LastUpdateInterval = m_StatisticPeriodFrames.load() / static_cast<double>(m_Settings.Samplerate);
	} else
	{
		result.InstantaneousLatency = m_StatisticLatencyFrames.load() / static_cast<double>(m_Settings.Samplerate);
		result.LastUpdateInterval = m_StatisticPeriodFrames.load() / static_cast<double>(m_Settings.Samplerate);
	}
	return result;
}


SoundDevice::Caps CRtAudioDevice::InternalGetDeviceCaps()
{
	MPT_TRACE();
	SoundDevice::Caps caps;
	if(!m_RtAudio)
	{
		return caps;
	}
	RtAudio::DeviceInfo rtinfo;
	try
	{
		rtinfo = m_RtAudio->getDeviceInfo(GetDevice(GetDeviceInfo()));
	} catch(const RtAudioError &)
	{
		return caps;
	}
	caps.Available = rtinfo.probed;
	caps.CanUpdateInterval = true;
	caps.CanSampleFormat = true;
	caps.CanExclusiveMode = true;
	caps.CanBoostThreadPriority = true;
	caps.CanKeepDeviceRunning = false;
	caps.CanUseHardwareTiming = false;
	caps.CanChannelMapping = false; // only base channel is supported, and that does not make too much sense for non-ASIO backends
	caps.CanInput = (rtinfo.inputChannels > 0);
	caps.HasNamedInputSources = true;
	caps.CanDriverPanel = false;
	caps.HasInternalDither = false;
	caps.ExclusiveModeDescription = U_("Exclusive Mode");
	return caps;
}


SoundDevice::DynamicCaps CRtAudioDevice::GetDeviceDynamicCaps(const std::vector<uint32> & /* baseSampleRates */ )
{
	MPT_TRACE();
	SoundDevice::DynamicCaps caps;
	RtAudio::DeviceInfo rtinfo;
	try
	{
		rtinfo = m_RtAudio->getDeviceInfo(GetDevice(GetDeviceInfo()));
	} catch(const RtAudioError &)
	{
		return caps;
	}
	if(!rtinfo.probed)
	{
		return caps;
	}
	caps.inputSourceNames.clear();
	for(unsigned int channel = 0; channel < rtinfo.inputChannels; ++channel)
	{
		caps.inputSourceNames.push_back(std::make_pair(channel, U_("Channel ") + mpt::ufmt::dec(channel + 1)));
	}
	caps.supportedSampleRates.insert(caps.supportedSampleRates.end(), rtinfo.sampleRates.begin(), rtinfo.sampleRates.end());
	caps.supportedExclusiveSampleRates.insert(caps.supportedExclusiveSampleRates.end(), rtinfo.sampleRates.begin(), rtinfo.sampleRates.end());
	for(unsigned int channel = 0; channel < rtinfo.outputChannels; ++channel)
	{
		caps.channelNames.push_back(mpt::format(U_("Output Channel %1"))(channel));
	}
	for(unsigned int channel = 0; channel < rtinfo.inputChannels; ++channel)
	{
		caps.inputSourceNames.push_back(std::make_pair(static_cast<uint32>(channel), mpt::format(U_("Input Channel %1"))(channel)));
	}
	return caps;
}


void CRtAudioDevice::SendError(const RtAudioError &e)
{
	LogLevel level = LogError;
	switch(e.getType())
	{
		case RtAudioError::WARNING:
			level = LogWarning;
			break;
		case RtAudioError::DEBUG_WARNING:
			level = LogDebug;
			break;
		case RtAudioError::UNSPECIFIED:
			level = LogError;
			break;
		case RtAudioError::NO_DEVICES_FOUND:
			level = LogError;
			break;
		case RtAudioError::INVALID_DEVICE:
			level = LogError;
			break;
		case RtAudioError::MEMORY_ERROR:
			level = LogError;
			break;
		case RtAudioError::INVALID_PARAMETER:
			level = LogError;
			break;
		case RtAudioError::INVALID_USE:
			level = LogError;
			break;
		case RtAudioError::DRIVER_ERROR:
			level = LogError;
			break;
		case RtAudioError::SYSTEM_ERROR:
			level = LogError;
			break;
		case RtAudioError::THREAD_ERROR:
			level = LogError;
			break;
		default:
			level = LogError;
			break;
	}
	SendDeviceMessage(level, mpt::ToUnicode(mpt::CharsetUTF8, e.getMessage()));
}


RtAudio::Api CRtAudioDevice::GetApi(SoundDevice::Info info)
{
	std::vector<mpt::ustring> apidev = mpt::String::Split<mpt::ustring>(info.internalID, U_(","));
	if(apidev.size() != 2)
	{
		return RtAudio::UNSPECIFIED;
	}
	return StringToRtAudioApi(apidev[0]);
}


unsigned int CRtAudioDevice::GetDevice(SoundDevice::Info info)
{
	std::vector<mpt::ustring> apidev = mpt::String::Split<mpt::ustring>(info.internalID, U_(","));
	if(apidev.size() != 2)
	{
		return 0;
	}
	return ConvertStrTo<unsigned int>(apidev[1]);
}


std::vector<SoundDevice::Info> CRtAudioDevice::EnumerateDevices(SoundDevice::SysInfo /* sysInfo */ )
{
	std::vector<SoundDevice::Info> devices;
	std::vector<RtAudio::Api> apis;
	RtAudio::getCompiledApi(apis);
	for(const auto &api : apis)
	{
		if(api == RtAudio::RTAUDIO_DUMMY)
		{
			continue;
		}
		if(api == RtAudio::WINDOWS_WASAPI)
		{ // WASAPI is broken in RtAudio 5.0.0
			continue;
		}
		try
		{
			RtAudio rtaudio(api);
			for(unsigned int device = 0; device < rtaudio.getDeviceCount(); ++device)
			{
				RtAudio::DeviceInfo rtinfo;
				try
				{
					rtinfo = rtaudio.getDeviceInfo(device);
				} catch(const RtAudioError &)
				{
					continue;
				}
				if(!rtinfo.probed)
				{
					continue;
				}
				SoundDevice::Info info = SoundDevice::Info();
				info.type = U_("RtAudio") + U_("-") + RtAudioApiToString(rtaudio.getCurrentApi());
				std::vector<mpt::ustring> apidev;
				apidev.push_back(RtAudioApiToString(rtaudio.getCurrentApi()));
				apidev.push_back(mpt::ufmt::val(device));
				info.internalID = mpt::String::Combine(apidev, U_(","));
				info.name = mpt::ToUnicode(mpt::CharsetUTF8, rtinfo.name);
				info.apiName = RtAudioApiToDescription(rtaudio.getCurrentApi());
				info.apiPath.push_back(U_("RtAudio"));
				info.isDefault = rtinfo.isDefaultOutput;
				info.useNameAsIdentifier = true;
				devices.push_back(info);
			}
		} catch(const RtAudioError &)
		{
			// nothing
		}
	}
	return devices;
}


MPT_REGISTERED_COMPONENT(ComponentRtAudio, "RtAudio")


ComponentRtAudio::ComponentRtAudio()
{
	return;
}


bool ComponentRtAudio::DoInitialize()
{
	return true;
}


ComponentRtAudio::~ComponentRtAudio()
{
	return;
}


#endif // MPT_WITH_RTAUDIO


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
