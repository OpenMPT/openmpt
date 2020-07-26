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
		case 8: result = RTAUDIO_SINT8; break;
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
		m_RtAudio = std::make_unique<RtAudio>(GetApi(info));
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
		m_StreamOptions.streamName = mpt::ToCharset(mpt::Charset::UTF8, m_AppInfo.GetName());
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
		if(m_RtAudio->getCurrentApi() == RtAudio::Api::WINDOWS_WASAPI)
		{
			m_Flags.NeedsClippedFloat = true;
		} else if(m_RtAudio->getCurrentApi() == RtAudio::Api::WINDOWS_DS)
		{
			m_Flags.NeedsClippedFloat = GetSysInfo().IsOriginal();
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
	SourceLockedAudioReadPrepare(m_CurrentFrameBufferCount, m_FramesPerChunk * m_StreamOptions.numberOfBuffers);
	SourceLockedAudioRead(m_CurrentFrameBufferOutput, m_CurrentFrameBufferInput, m_CurrentFrameBufferCount);
	m_StatisticLatencyFrames.store(m_CurrentFrameBufferCount * m_StreamOptions.numberOfBuffers);
	m_StatisticPeriodFrames.store(m_CurrentFrameBufferCount);
	SourceLockedAudioReadDone();
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
	mpt::append(caps.supportedSampleRates, rtinfo.sampleRates);
	std::reverse(caps.supportedSampleRates.begin(), caps.supportedSampleRates.end());
	mpt::append(caps.supportedExclusiveSampleRates, rtinfo.sampleRates);
	std::reverse(caps.supportedExclusiveSampleRates.begin(), caps.supportedExclusiveSampleRates.end());
	caps.supportedSampleFormats = { SampleFormatFloat32 };
	caps.supportedExclusiveModeSampleFormats.clear();
	if(rtinfo.nativeFormats & RTAUDIO_SINT8)
	{
		caps.supportedExclusiveModeSampleFormats.push_back(SampleFormatInt8);
	}
	if(rtinfo.nativeFormats & RTAUDIO_SINT16)
	{
		caps.supportedExclusiveModeSampleFormats.push_back(SampleFormatInt16);
	}
	if(rtinfo.nativeFormats & RTAUDIO_SINT24)
	{
		caps.supportedExclusiveModeSampleFormats.push_back(SampleFormatInt24);
	}
	if(rtinfo.nativeFormats & RTAUDIO_SINT32)
	{
		caps.supportedExclusiveModeSampleFormats.push_back(SampleFormatInt32);
	}
	if(rtinfo.nativeFormats & RTAUDIO_FLOAT32)
	{
		caps.supportedExclusiveModeSampleFormats.push_back(SampleFormatFloat32);
	}
	if(rtinfo.nativeFormats & RTAUDIO_FLOAT64)
	{
		caps.supportedExclusiveModeSampleFormats.push_back(SampleFormatFloat64);
	}
	for(unsigned int channel = 0; channel < rtinfo.outputChannels; ++channel)
	{
		caps.channelNames.push_back(MPT_UFORMAT("Output Channel {}")(channel));
	}
	for(unsigned int channel = 0; channel < rtinfo.inputChannels; ++channel)
	{
		caps.inputSourceNames.push_back(std::make_pair(static_cast<uint32>(channel), MPT_UFORMAT("Input Channel {}")(channel)));
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
	SendDeviceMessage(level, mpt::ToUnicode(mpt::Charset::UTF8, e.getMessage()));
}


RtAudio::Api CRtAudioDevice::GetApi(SoundDevice::Info info)
{
	std::vector<mpt::ustring> apidev = mpt::String::Split<mpt::ustring>(info.internalID, U_(","));
	if(apidev.size() != 2)
	{
		return RtAudio::UNSPECIFIED;
	}
	return RtAudio::getCompiledApiByName(mpt::ToCharset(mpt::Charset::UTF8, apidev[0]));
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


std::vector<SoundDevice::Info> CRtAudioDevice::EnumerateDevices(SoundDevice::SysInfo sysInfo)
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
				info.type = U_("RtAudio") + U_("-") + mpt::ToUnicode(mpt::Charset::UTF8, RtAudio::getApiName(rtaudio.getCurrentApi()));
				std::vector<mpt::ustring> apidev;
				apidev.push_back(mpt::ToUnicode(mpt::Charset::UTF8, RtAudio::getApiName(rtaudio.getCurrentApi())));
				apidev.push_back(mpt::ufmt::val(device));
				info.internalID = mpt::String::Combine(apidev, U_(","));
				info.name = mpt::ToUnicode(mpt::Charset::UTF8, rtinfo.name);
				info.apiName = mpt::ToUnicode(mpt::Charset::UTF8, RtAudio::getApiDisplayName(rtaudio.getCurrentApi()));
				info.extraData[U_("RtAudio-ApiDisplayName")] = mpt::ToUnicode(mpt::Charset::UTF8, RtAudio::getApiDisplayName(rtaudio.getCurrentApi())); 
				info.apiPath.push_back(U_("RtAudio"));
				info.useNameAsIdentifier = true;
				switch(rtaudio.getCurrentApi())
				{
				case RtAudio::LINUX_ALSA:
					info.apiName = U_("ALSA");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Named : Info::Default::None);
					info.flags = {
						sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Usability::Usable : Info::Usability::Experimental,
						Info::Level::Secondary,
						Info::Compatible::No,
						sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Api::Native : Info::Api::Emulated,
						Info::Io::FullDuplex,
						sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Mixing::Hardware : Info::Mixing::Software,
						Info::Implementor::External
					};
					break;
				case RtAudio::LINUX_PULSE:
					info.apiName = U_("PulseAudio");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Managed : Info::Default::None);
					info.flags = {
						sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Usability::Usable : Info::Usability::Experimental,
						Info::Level::Secondary,
						Info::Compatible::No,
						sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Api::Native : Info::Api::Emulated,
						Info::Io::FullDuplex,
						Info::Mixing::Server,
						Info::Implementor::External
					};
					break;
				case RtAudio::LINUX_OSS:
					info.apiName = U_("OSS");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Named : Info::Default::None);
					info.flags = {
						sysInfo.SystemClass == mpt::OS::Class::BSD ? Info::Usability::Usable : sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Usability::Deprecated : Info::Usability::NotAvailable,
						Info::Level::Secondary,
						Info::Compatible::No,
						sysInfo.SystemClass == mpt::OS::Class::BSD ? Info::Api::Native : sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Api::Emulated : Info::Api::Emulated,
						Info::Io::FullDuplex,
						sysInfo.SystemClass == mpt::OS::Class::BSD ? Info::Mixing::Hardware : sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Mixing::Software : Info::Mixing::Software,
						Info::Implementor::External
					};
					break;
				case RtAudio::UNIX_JACK:
					info.apiName = U_("JACK");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Managed : Info::Default::None);
					info.flags = {
						sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Usability::Usable : sysInfo.SystemClass == mpt::OS::Class::Darwin ? Info::Usability::Usable : Info::Usability::Experimental,
						Info::Level::Primary,
						Info::Compatible::Yes,
						sysInfo.SystemClass == mpt::OS::Class::Linux ? Info::Api::Native : Info::Api::Emulated,
						Info::Io::FullDuplex,
						Info::Mixing::Server,
						Info::Implementor::External
					};
					break;
				case RtAudio::MACOSX_CORE:
					info.apiName = U_("CoreAudio");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Named : Info::Default::None);
					info.flags = {
						sysInfo.SystemClass == mpt::OS::Class::Darwin ? Info::Usability::Usable : Info::Usability::NotAvailable,
						Info::Level::Primary,
						Info::Compatible::Yes,
						sysInfo.SystemClass == mpt::OS::Class::Darwin ? Info::Api::Native : Info::Api::Emulated,
						Info::Io::FullDuplex,
						Info::Mixing::Server,
						Info::Implementor::External
					};
					break;
				case RtAudio::WINDOWS_WASAPI:
					info.apiName = U_("WASAPI");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Named : Info::Default::None);
					info.flags = {
						sysInfo.SystemClass == mpt::OS::Class::Windows ? Info::Usability::Usable : Info::Usability::NotAvailable,
						Info::Level::Secondary,
						Info::Compatible::No,
						sysInfo.SystemClass == mpt::OS::Class::Windows ? Info::Api::Native : Info::Api::Emulated,
						Info::Io::FullDuplex,
						Info::Mixing::Server,
						Info::Implementor::External
					};
					break;
				case RtAudio::WINDOWS_ASIO:
					info.apiName = U_("ASIO");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Named : Info::Default::None);
					info.flags = {
						sysInfo.SystemClass == mpt::OS::Class::Windows ? sysInfo.IsWindowsOriginal() ? Info::Usability::Usable : Info::Usability::Experimental : Info::Usability::NotAvailable,
						Info::Level::Secondary,
						Info::Compatible::No,
						sysInfo.SystemClass == mpt::OS::Class::Windows && sysInfo.IsWindowsOriginal() ? Info::Api::Native : Info::Api::Emulated,
						Info::Io::FullDuplex,
						Info::Mixing::Hardware,
						Info::Implementor::External
					};
					break;
				case RtAudio::WINDOWS_DS:
					info.apiName = U_("DirectSound");
					info.default_ = (rtinfo.isDefaultOutput ? Info::Default::Managed : Info::Default::None);
					info.flags = {
						Info::Usability::Broken, // sysInfo.SystemClass == mpt::OS::Class::Windows ? Info::Usability::Deprecated : Info::Usability::NotAvailable,
						Info::Level::Secondary,
						Info::Compatible::No,
						Info::Api::Emulated,
						Info::Io::FullDuplex,
						Info::Mixing::Software,
						Info::Implementor::External
					};
					break;
				default:
					// nothing
					break;
				}

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
