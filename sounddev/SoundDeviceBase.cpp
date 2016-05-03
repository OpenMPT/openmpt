/*
 * SoundDeviceBase.cpp
 * -------------------
 * Purpose: Sound device drivers base class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDeviceBase.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


Base::Base(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
//--------------------------------------------------------------
	: m_Source(nullptr)
	, m_MessageReceiver(nullptr)
	, m_Info(info)
	, m_SysInfo(sysInfo)
{
	MPT_TRACE();

	m_DeviceUnavailableOnOpen = false;

	m_IsPlaying = false;
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


uint64 Base::SourceLockedGetReferenceClockNowNanoseconds() const
//--------------------------------------------------------------
{
	MPT_TRACE();
	if(!m_Source)
	{
		return 0;
	}
	return m_Source->SoundSourceLockedGetReferenceClockNowNanoseconds();
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
		ISource::Guard lock(*m_Source);
		InternalFillAudioBuffer();
	}
}


void Base::SourceLockedAudioPreRead(std::size_t numFrames, std::size_t framesLatency)
//-----------------------------------------------------------------------------------
{
	MPT_TRACE();
	if(!InternalHasTimeInfo())
	{
		SoundDevice::TimeInfo timeInfo;
		if(InternalHasGetStreamPosition())
		{
			timeInfo.SyncPointStreamFrames = InternalHasGetStreamPosition();
			timeInfo.SyncPointSystemTimestamp = SourceLockedGetReferenceClockNowNanoseconds();
			timeInfo.Speed = 1.0;
		} else
		{
			timeInfo.SyncPointStreamFrames = m_StreamPositionRenderFrames + numFrames;
			timeInfo.SyncPointSystemTimestamp = SourceLockedGetReferenceClockNowNanoseconds() + Util::Round<int64>(GetEffectiveBufferAttributes().Latency * 1000000000.0);
			timeInfo.Speed = 1.0;
		}
		timeInfo.RenderStreamPositionBefore = StreamPositionFromFrames(m_StreamPositionRenderFrames);
		timeInfo.RenderStreamPositionAfter = StreamPositionFromFrames(m_StreamPositionRenderFrames + numFrames);
		SetTimeInfo(timeInfo);
	}
	m_StreamPositionRenderFrames += numFrames;
	if(!InternalHasGetStreamPosition() && !InternalHasTimeInfo())
	{
		m_StreamPositionOutputFrames = m_StreamPositionRenderFrames - framesLatency;
	} else
	{
		// unused, no locking
		m_StreamPositionOutputFrames = 0;
	}
}


void Base::SourceLockedAudioRead(void *buffer, const void *inputBuffer, std::size_t numFrames)
//--------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	if(numFrames <= 0)
	{
		return;
	}
	m_Source->SoundSourceLockedRead(GetBufferFormat(), GetEffectiveBufferAttributes(), m_TimeInfo, numFrames, buffer, inputBuffer);
}


void Base::SourceLockedAudioDone()
//--------------------------------
{
	MPT_TRACE();
	m_Source->SoundSourceLockedDone(GetBufferFormat(), GetEffectiveBufferAttributes(), m_TimeInfo);
}


void Base::SendDeviceMessage(LogLevel level, const mpt::ustring &str)
//-------------------------------------------------------------------
{
	MPT_TRACE();
	MPT_LOG(level, "sounddev", str);
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
		m_StreamPositionRenderFrames = 0;
		{
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
			m_StreamPositionOutputFrames = 0;
		}
		m_StreamPositionRenderFrames = 0;
	}
}


SoundDevice::StreamPosition Base::GetStreamPosition() const
//---------------------------------------------------------
{
	MPT_TRACE();
	if(!IsOpen())
	{
		return StreamPosition();
	}
	int64 frames = 0;
	if(InternalHasGetStreamPosition())
	{
		frames = InternalGetStreamPositionFrames();
	} else if(InternalHasTimeInfo())
	{
		const uint64 now = SourceGetReferenceClockNowNanoseconds();
		const SoundDevice::TimeInfo timeInfo = GetTimeInfo();
		frames = Util::Round<int64>(
				timeInfo.SyncPointStreamFrames + (
					static_cast<double>(static_cast<int64>(now - timeInfo.SyncPointSystemTimestamp)) * timeInfo.Speed * m_Settings.Samplerate * (1.0 / (1000.0 * 1000.0))
				)
			);
	} else
	{
		frames = m_StreamPositionOutputFrames;
	}
	return StreamPositionFromFrames(frames);
}


SoundDevice::Statistics Base::GetStatistics() const
//-------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	result.InstantaneousLatency = m_Settings.Latency;
	result.LastUpdateInterval = m_Settings.UpdateInterval;
	result.text = mpt::ustring();
	return result;
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
