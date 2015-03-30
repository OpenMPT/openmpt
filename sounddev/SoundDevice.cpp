/*
 * SoundDevice.cpp
 * ---------------
 * Purpose: Actual sound device driver classes.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"

#include "../common/misc_util.h"
#include "../common/StringFixer.h"



OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


mpt::ustring TypeToString(SoundDevice::Type type, bool verbose)
//-------------------------------------------------------------
{
	switch(type)
	{
	case TypeWAVEOUT: return MPT_USTRING("WaveOut"); break;
	case TypeDSOUND: return MPT_USTRING("DirectSound"); break;
	case TypeASIO: return MPT_USTRING("ASIO"); break;
	case TypePORTAUDIO_WASAPI: return (verbose ? MPT_USTRING("PortAudio") : MPT_USTRING("")) + MPT_USTRING("WASAPI"); break;
	case TypePORTAUDIO_WDMKS: return (verbose ? MPT_USTRING("PortAudio") : MPT_USTRING("")) + MPT_USTRING("WDM-KS"); break;
	case TypePORTAUDIO_WMME: return (verbose ? MPT_USTRING("PortAudio") : MPT_USTRING("")) + MPT_USTRING("MME"); break;
	case TypePORTAUDIO_DS: return (verbose ? MPT_USTRING("PortAudio") : MPT_USTRING("")) + MPT_USTRING("DS"); break;
	case TypePORTAUDIO_ASIO: return (verbose ? MPT_USTRING("PortAudio") : MPT_USTRING("")) + MPT_USTRING("ASIO"); break;
	}
	return mpt::ustring();
}


ChannelMapping::ChannelMapping()
//------------------------------
{
	return;
}


ChannelMapping::ChannelMapping(uint32 numHostChannels)
//----------------------------------------------------
{
	ChannelToDeviceChannel.resize(numHostChannels);
	for(uint32 channel = 0; channel < numHostChannels; ++channel)
	{
		ChannelToDeviceChannel[channel] = channel;
	}
}


ChannelMapping::ChannelMapping(const std::vector<int32> &mapping)
//---------------------------------------------------------------
{
	if(IsValid(mapping))
	{
		ChannelToDeviceChannel = mapping;
	}
}


ChannelMapping ChannelMapping::BaseChannel(uint32 channels, int32 baseChannel)
//----------------------------------------------------------------------------
{
	SoundDevice::ChannelMapping result;
	result.ChannelToDeviceChannel.resize(channels);
	for(uint32 channel = 0; channel < channels; ++channel)
	{
		result.ChannelToDeviceChannel[channel] = channel + baseChannel;
	}
	return result;
}


bool ChannelMapping::IsValid(const std::vector<int32> &mapping)
//-------------------------------------------------------------
{
	if(mapping.empty())
	{
		return true;
	}
	std::map<int32, uint32> inverseMapping;
	for(uint32 hostChannel = 0; hostChannel < mapping.size(); ++hostChannel)
	{
		int32 deviceChannel = mapping[hostChannel];
		if(deviceChannel < 0)
		{
			return false;
		}
		if(deviceChannel > MaxDeviceChannel)
		{
			return false;
		}
		inverseMapping[deviceChannel] = hostChannel;
	}
	if(inverseMapping.size() != mapping.size())
	{
		return false;
	}
	return true;
}


mpt::ustring ChannelMapping::ToString() const
//-------------------------------------------
{
	return mpt::String::Combine<int32>(ChannelToDeviceChannel);
}


ChannelMapping ChannelMapping::FromString(const mpt::ustring &str)
//----------------------------------------------------------------
{
	return SoundDevice::ChannelMapping(mpt::String::Split<int32>(str));
}


///////////////////////////////////////////////////////////////////////////////////////
//
// SoundDevice::Base base class
//

Base::Base(SoundDevice::Info info)
//--------------------------------
	: m_Source(nullptr)
	, m_MessageReceiver(nullptr)
	, m_Info(info)
{
	MPT_TRACE();

	m_DeviceUnavailableOnOpen = false;

	m_IsPlaying = false;
	m_CurrentUpdateInterval = 0.0;
	m_StreamPositionRenderFrames = 0;
	m_StreamPositionOutputFrames = 0;

	m_RequestFlags.store(0);
}


Base::~Base()
//-----------
{
	MPT_TRACE();
	return;
}


SoundDevice::DynamicCaps Base::GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates)
//---------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::DynamicCaps result;
	result.supportedSampleRates = baseSampleRates;
	return result;
}


void Base::UpdateTimeInfo(SoundDevice::TimeInfo timeInfo)
//-------------------------------------------------------
{
	MPT_TRACE();
	m_TimeInfo = timeInfo;
}


bool Base::Init(const SoundDevice::AppInfo &appInfo)
//--------------------------------------------------
{
	MPT_TRACE();
	if(IsInited())
	{
		return true;
	}
	m_AppInfo = appInfo;
	m_Caps = InternalGetDeviceCaps();
	return m_Caps.Available;
}


bool Base::Open(const SoundDevice::Settings &settings)
//----------------------------------------------------
{
	MPT_TRACE();
	if(IsOpen())
	{
		Close();
	}
	m_Settings = settings;
	if(m_Settings.Latency == 0.0) m_Settings.Latency = m_Caps.DefaultSettings.Latency;
	if(m_Settings.UpdateInterval == 0.0) m_Settings.UpdateInterval = m_Caps.DefaultSettings.UpdateInterval;
	m_Settings.Latency = Clamp(m_Settings.Latency, m_Caps.LatencyMin, m_Caps.LatencyMax);
	m_Settings.UpdateInterval = Clamp(m_Settings.UpdateInterval, m_Caps.UpdateIntervalMin, m_Caps.UpdateIntervalMax);
	if(m_Caps.CanChannelMapping)
	{
		if(m_Settings.ChannelMapping.GetNumHostChannels() == 0)
		{
			// default mapping
			m_Settings.ChannelMapping = SoundDevice::ChannelMapping(m_Settings.Channels);
		}
		if(m_Settings.ChannelMapping.GetNumHostChannels() != m_Settings.Channels)
		{
			return false;
		}
	} else
	{
		m_Settings.ChannelMapping = SoundDevice::ChannelMapping(m_Settings.Channels);
	}
	m_Flags = SoundDevice::Flags();
	m_DeviceUnavailableOnOpen = false;
	m_RequestFlags.store(0);
	return InternalOpen();
}


bool Base::Close()
//----------------
{
	MPT_TRACE();
	if(!IsOpen()) return true;
	Stop();
	bool result = InternalClose();
	m_RequestFlags.store(0);
	return result;
}


void Base::FillAudioBuffer()
//--------------------------
{
	MPT_TRACE();
	InternalFillAudioBuffer();
}


uint64 Base::SourceGetReferenceClockNowNanoseconds() const
//--------------------------------------------------------
{
	MPT_TRACE();
	if(!m_Source)
	{
		return 0;
	}
	return m_Source->SoundSourceGetReferenceClockNowNanoseconds();
}


void Base::SourceNotifyPreStart()
//-------------------------------
{
	MPT_TRACE();
	if(m_Source)
	{
		m_Source->SoundSourcePreStartCallback();
	}
}


void Base::SourceNotifyPostStop()
//-------------------------------
{
	MPT_TRACE();
	if(m_Source)
	{
		m_Source->SoundSourcePostStopCallback();
	}
}


bool Base::SourceIsLockedByCurrentThread() const
//----------------------------------------------
{
	MPT_TRACE();
	if(!m_Source)
	{
		return false;
	}
	return m_Source->SoundSourceIsLockedByCurrentThread();
}


void Base::SourceFillAudioBufferLocked()
//--------------------------------------
{
	MPT_TRACE();
	if(m_Source)
	{
		m_Source->FillAudioBufferLocked(*this);
	}
}


void Base::SourceAudioPreRead(std::size_t numFrames)
//--------------------------------------------------
{
	MPT_TRACE();
	if(!InternalHasTimeInfo())
	{
		if(InternalHasGetStreamPosition())
		{
			SoundDevice::TimeInfo timeInfo;
			timeInfo.StreamFrames = InternalHasGetStreamPosition();
			timeInfo.SystemTimestamp = SourceGetReferenceClockNowNanoseconds();
			timeInfo.Speed = 1.0;
			UpdateTimeInfo(timeInfo);
		} else
		{
			SoundDevice::TimeInfo timeInfo;
			{
				Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
				timeInfo.StreamFrames = m_StreamPositionRenderFrames + numFrames;
			}
			timeInfo.SystemTimestamp = SourceGetReferenceClockNowNanoseconds() + Util::Round<int64>(GetEffectiveBufferAttributes().Latency * 1000000000.0);
			timeInfo.Speed = 1.0;
			UpdateTimeInfo(timeInfo);
		}
	}
}


void Base::SourceAudioRead(void *buffer, std::size_t numFrames)
//-------------------------------------------------------------
{
	if(numFrames <= 0)
	{
		return;
	}
	m_Source->AudioRead(m_Settings, m_Flags, GetEffectiveBufferAttributes(), m_TimeInfo, numFrames, buffer);
}


void Base::SourceAudioDone(std::size_t numFrames, std::size_t framesLatency)
//--------------------------------------------------------------------------
{
	MPT_TRACE();
	if(numFrames <= 0)
	{
		return;
	}
	int64 framesRendered = 0;
	{
		Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
		m_CurrentUpdateInterval = (double)numFrames / (double)m_Settings.Samplerate;
		m_StreamPositionRenderFrames += numFrames;
		m_StreamPositionOutputFrames = m_StreamPositionRenderFrames - framesLatency;
		framesRendered = m_StreamPositionRenderFrames;
	}
	m_Source->AudioDone(m_Settings, m_Flags, GetEffectiveBufferAttributes(), m_TimeInfo, numFrames, framesRendered);
}


void Base::SendDeviceMessage(LogLevel level, const mpt::ustring &str)
//-------------------------------------------------------------------
{
	MPT_TRACE();
	Log(level, str);
	if(m_MessageReceiver)
	{
		m_MessageReceiver->SoundDeviceMessage(level, str);
	}
}


bool Base::Start()
//----------------
{
	MPT_TRACE();
	if(!IsOpen()) return false; 
	if(!IsPlaying())
	{
		{
			Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
			m_CurrentUpdateInterval = 0.0;
			m_StreamPositionRenderFrames = 0;
			m_StreamPositionOutputFrames = 0;
		}
		SourceNotifyPreStart();
		m_RequestFlags.fetch_and((~RequestFlagRestart).as_bits());
		if(!InternalStart())
		{
			SourceNotifyPostStop();
			return false;
		}
		m_IsPlaying = true;
	}
	return true;
}


void Base::Stop(bool force)
//-------------------------
{
	MPT_TRACE();
	if(!IsOpen()) return;
	if(IsPlaying())
	{
		if(force)
		{
			InternalStopForce();
		} else
		{
			InternalStop();
		}
		m_RequestFlags.fetch_and((~RequestFlagRestart).as_bits());
		SourceNotifyPostStop();
		m_IsPlaying = false;
		{
			Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
			m_CurrentUpdateInterval = 0.0;
			m_StreamPositionRenderFrames = 0;
			m_StreamPositionOutputFrames = 0;
		}
	}
}


double Base::GetLastUpdateInterval() const
//----------------------------------------
{
	MPT_TRACE();
	Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
	return m_CurrentUpdateInterval;
}


int64 Base::GetStreamPositionFrames() const
//-----------------------------------------
{
	MPT_TRACE();
	if(!IsOpen()) return 0;
	if(InternalHasGetStreamPosition())
	{
		return InternalGetStreamPositionFrames();
	} else
	{
		Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
		return m_StreamPositionOutputFrames;
	}
}


SoundDevice::Statistics Base::GetStatistics() const
//-------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	result.InstantaneousLatency = m_Settings.Latency;
	result.LastUpdateInterval = GetLastUpdateInterval();
	result.text = mpt::ustring();
	return result;
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
