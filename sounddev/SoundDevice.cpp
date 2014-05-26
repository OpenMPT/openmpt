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

#include "SoundDeviceASIO.h"
#include "SoundDeviceDirectSound.h"
#include "SoundDevicePortAudio.h"
#include "SoundDeviceWaveout.h"

#include <algorithm>
#include <iterator>

#include <mmreg.h>


std::wstring SoundDeviceTypeToString(SoundDeviceType type)
//--------------------------------------------------------
{
	switch(type)
	{
	case SNDDEV_WAVEOUT: return L"WaveOut"; break;
	case SNDDEV_DSOUND: return L"DirectSound"; break;
	case SNDDEV_ASIO: return L"ASIO"; break;
	case SNDDEV_PORTAUDIO_WASAPI: return L"WASAPI"; break;
	case SNDDEV_PORTAUDIO_WDMKS: return L"WDM-KS"; break;
	case SNDDEV_PORTAUDIO_WMME: return L"MME"; break;
	case SNDDEV_PORTAUDIO_DS: return L"DS"; break;
	case SNDDEV_PORTAUDIO_ASIO: return L"ASIO"; break;
	}
	return std::wstring();
}


SoundChannelMapping::SoundChannelMapping()
//----------------------------------------
{
	return;
}


SoundChannelMapping::SoundChannelMapping(const std::vector<uint32> &mapping)
//--------------------------------------------------------------------------
	: ChannelToDeviceChannel(mapping)
{
	return;
}


SoundChannelMapping SoundChannelMapping::BaseChannel(uint32 channels, uint32 baseChannel)
//---------------------------------------------------------------------------------------
{
	SoundChannelMapping result;
	result.ChannelToDeviceChannel.clear();
	if(baseChannel == 0)
	{
		return result;
	}
	result.ChannelToDeviceChannel.resize(channels);
	for(uint32 channel = 0; channel < channels; ++channel)
	{
		result.ChannelToDeviceChannel[channel] = channel + baseChannel;
	}
	return result;
}


bool SoundChannelMapping::IsValid(uint32 channels) const
//------------------------------------------------------
{
	if(ChannelToDeviceChannel.empty())
	{
		return true;
	}
	if(ChannelToDeviceChannel.size() < channels)
	{
		return false;
	}
	std::map<uint32, uint32> inverseMapping;
	for(uint32 channel = 0; channel < channels; ++channel)
	{
		inverseMapping[ChannelToDeviceChannel[channel]] = channel;
	}
	if(inverseMapping.size() != channels)
	{
		return false;
	}
	return true;
}


std::string SoundChannelMapping::ToString() const
//-----------------------------------------------
{
	return mpt::String::Combine<uint32>(ChannelToDeviceChannel);
}


SoundChannelMapping SoundChannelMapping::FromString(const std::string &str)
//-------------------------------------------------------------------------
{
	return SoundChannelMapping(mpt::String::Split<uint32>(str));
}


///////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice base class
//

ISoundDevice::ISoundDevice(SoundDeviceID id, const std::wstring &internalID)
//--------------------------------------------------------------------------
	: m_Source(nullptr)
	, m_MessageReceiver(nullptr)
	, m_ID(id)
	, m_InternalID(internalID)
{

	m_BufferAttributes.Latency = m_Settings.LatencyMS / 1000.0;
	m_BufferAttributes.UpdateInterval = m_Settings.UpdateIntervalMS / 1000.0;
	m_BufferAttributes.NumBuffers = 0;

	m_IsPlaying = false;
	m_CurrentUpdateInterval = 0.0;
	m_StreamPositionRenderFrames = 0;
	m_StreamPositionOutputFrames = 0;

	InterlockedExchange(&m_RequestFlags, 0);
}


ISoundDevice::~ISoundDevice()
//---------------------------
{
	return;
}


SoundDeviceCaps ISoundDevice::GetDeviceCaps()
//-------------------------------------------
{
	SoundDeviceCaps result;
	return result;
}


SoundDeviceDynamicCaps ISoundDevice::GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates)
//---------------------------------------------------------------------------------------------------
{
	SoundDeviceDynamicCaps result;
	result.supportedSampleRates = baseSampleRates;
	return result;
}


bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat, const SoundDeviceSettings &m_Settings)
//----------------------------------------------------------------------------------------------------
{
	MemsetZero(WaveFormat);
	if(!m_Settings.sampleFormat.IsValid()) return false;
	WaveFormat.Format.wFormatTag = m_Settings.sampleFormat.IsFloat() ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	WaveFormat.Format.nChannels = (WORD)m_Settings.Channels;
	WaveFormat.Format.nSamplesPerSec = m_Settings.Samplerate;
	WaveFormat.Format.nAvgBytesPerSec = (DWORD)m_Settings.GetBytesPerSecond();
	WaveFormat.Format.nBlockAlign = (WORD)m_Settings.GetBytesPerFrame();
	WaveFormat.Format.wBitsPerSample = (WORD)m_Settings.sampleFormat.GetBitsPerSample();
	WaveFormat.Format.cbSize = 0;
	if((WaveFormat.Format.wBitsPerSample > 16 && m_Settings.sampleFormat.IsInt()) || (WaveFormat.Format.nChannels > 2))
	{
		WaveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		WaveFormat.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		WaveFormat.Samples.wValidBitsPerSample = WaveFormat.Format.wBitsPerSample;
		switch(WaveFormat.Format.nChannels)
		{
		case 1:  WaveFormat.dwChannelMask = SPEAKER_FRONT_CENTER; break;
		case 2:  WaveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT; break;
		case 3:  WaveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER; break;
		case 4:  WaveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT; break;
		default: WaveFormat.dwChannelMask = 0; return false; break;
		}
		const GUID guid_MEDIASUBTYPE_PCM = {0x00000001, 0x0000, 0x0010, 0x80, 0x00, 0x0, 0xAA, 0x0, 0x38, 0x9B, 0x71};
		const GUID guid_MEDIASUBTYPE_IEEE_FLOAT = {0x00000003, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
		WaveFormat.SubFormat = m_Settings.sampleFormat.IsFloat() ? guid_MEDIASUBTYPE_IEEE_FLOAT : guid_MEDIASUBTYPE_PCM;
	}
	return true;
}


void ISoundDevice::UpdateBufferAttributes(SoundBufferAttributes attributes)
//-------------------------------------------------------------------------
{
	m_BufferAttributes = attributes;
}


void ISoundDevice::UpdateTimeInfo(SoundTimeInfo timeInfo)
//-------------------------------------------------------
{
	m_TimeInfo = timeInfo;
}


bool ISoundDevice::Open(const SoundDeviceSettings &settings)
//----------------------------------------------------------
{
	if(IsOpen())
	{
		Close();
	}
	m_Settings = settings;
	if(m_Settings.LatencyMS < SNDDEV_MINLATENCY_MS) m_Settings.LatencyMS = SNDDEV_MINLATENCY_MS;
	if(m_Settings.LatencyMS > SNDDEV_MAXLATENCY_MS) m_Settings.LatencyMS = SNDDEV_MAXLATENCY_MS;
	if(m_Settings.UpdateIntervalMS < SNDDEV_MINUPDATEINTERVAL_MS) m_Settings.UpdateIntervalMS = SNDDEV_MINUPDATEINTERVAL_MS;
	if(m_Settings.UpdateIntervalMS > SNDDEV_MAXUPDATEINTERVAL_MS) m_Settings.UpdateIntervalMS = SNDDEV_MAXUPDATEINTERVAL_MS;
	if(!m_Settings.ChannelMapping.IsValid(m_Settings.Channels))
	{
		return false;
	}
	m_Flags = SoundDeviceFlags();
	m_BufferAttributes.Latency = m_Settings.LatencyMS / 1000.0;
	m_BufferAttributes.UpdateInterval = m_Settings.UpdateIntervalMS / 1000.0;
	m_BufferAttributes.NumBuffers = 0;
	InterlockedExchange(&m_RequestFlags, 0);
	return InternalOpen();
}


bool ISoundDevice::Close()
//------------------------
{
	if(!IsOpen()) return true;
	Stop();
	bool result = InternalClose();
	InterlockedExchange(&m_RequestFlags, 0);
	return result;
}


void ISoundDevice::FillAudioBuffer()
//----------------------------------
{
	InternalFillAudioBuffer();
}


void ISoundDevice::SourceFillAudioBufferLocked()
//----------------------------------------------
{
	if(m_Source)
	{
		m_Source->FillAudioBufferLocked(*this);
	}
}


void ISoundDevice::SourceAudioPreRead(std::size_t numFrames)
//----------------------------------------------------------
{
	if(!InternalHasTimeInfo())
	{
		if(InternalHasGetStreamPosition())
		{
			SoundTimeInfo timeInfo;
			timeInfo.StreamFrames = InternalHasGetStreamPosition();
			timeInfo.SystemTimestamp = Clock().NowNanoseconds();
			timeInfo.Speed = 1.0;
			UpdateTimeInfo(timeInfo);
		} else
		{
			SoundTimeInfo timeInfo;
			{
				Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
				timeInfo.StreamFrames = m_StreamPositionRenderFrames + numFrames;
			}
			timeInfo.SystemTimestamp = Clock().NowNanoseconds() + Util::Round<int64>(m_BufferAttributes.Latency * 1000000000.0);
			timeInfo.Speed = 1.0;
			UpdateTimeInfo(timeInfo);
		}
	}
}


void ISoundDevice::SourceAudioRead(void *buffer, std::size_t numFrames)
//---------------------------------------------------------------------
{
	if(numFrames <= 0)
	{
		return;
	}
	m_Source->AudioRead(m_Settings, m_Flags, m_BufferAttributes, m_TimeInfo, numFrames, buffer);
}


void ISoundDevice::SourceAudioDone(std::size_t numFrames, int32 framesLatency)
//----------------------------------------------------------------------------
{
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
	m_Source->AudioDone(m_Settings, m_Flags, m_BufferAttributes, m_TimeInfo, numFrames, framesRendered);
}


void ISoundDevice::AudioSendMessage(const std::string &str)
//---------------------------------------------------------
{
	if(m_MessageReceiver)
	{
		m_MessageReceiver->AudioMessage(str);
	}
}


bool ISoundDevice::Start()
//------------------------
{
	if(!IsOpen()) return false; 
	if(!IsPlaying())
	{
		{
			Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
			m_CurrentUpdateInterval = 0.0;
			m_StreamPositionRenderFrames = 0;
			m_StreamPositionOutputFrames = 0;
		}
		m_Clock.SetResolution(1);
		_InterlockedAnd(&m_RequestFlags, ~RequestFlagRestart);
		if(!InternalStart())
		{
			m_Clock.SetResolution(0);
			return false;
		}
		m_IsPlaying = true;
	}
	return true;
}


void ISoundDevice::Stop(bool force)
//---------------------------------
{
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
		_InterlockedAnd(&m_RequestFlags, ~RequestFlagRestart);
		m_Clock.SetResolution(0);
		m_IsPlaying = false;
		{
			Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
			m_CurrentUpdateInterval = 0.0;
			m_StreamPositionRenderFrames = 0;
			m_StreamPositionOutputFrames = 0;
		}
	}
}


double ISoundDevice::GetCurrentUpdateInterval() const
//---------------------------------------------------
{
	Util::lock_guard<Util::mutex> lock(m_StreamPositionMutex);
	return m_CurrentUpdateInterval;
}


int64 ISoundDevice::GetStreamPositionFrames() const
//-------------------------------------------------
{
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


///////////////////////////////////////////////////////////////////////////////////////
//
// Global Functions
//


void SoundDevicesManager::ReEnumerate()
//-------------------------------------
{
	m_SoundDevices.clear();
	m_DeviceCaps.clear();

#ifndef NO_PORTAUDIO
	SndDevPortaudioUnnitialize();
	SndDevPortaudioInitialize();
#endif // NO_PORTAUDIO

	{
		const std::vector<SoundDeviceInfo> infos = CWaveDevice::EnumerateDevices();
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
#ifndef NO_ASIO
	{
		const std::vector<SoundDeviceInfo> infos = CASIODevice::EnumerateDevices();
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
#endif // NO_ASIO
#ifndef NO_PORTAUDIO
	{
		const std::vector<SoundDeviceInfo> infos = CPortaudioDevice::EnumerateDevices(SNDDEV_PORTAUDIO_WASAPI);
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
	{
		const std::vector<SoundDeviceInfo> infos = CPortaudioDevice::EnumerateDevices(SNDDEV_PORTAUDIO_WDMKS);
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
#endif // NO_PORTAUDIO

	// kind of deprecated by now
#ifndef NO_DSOUND
	{
		const std::vector<SoundDeviceInfo> infos = CDSoundDevice::EnumerateDevices();
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
#endif // NO_DSOUND

	// duplicate devices, only used if [Sound Settings]MorePortaudio=1
#ifndef NO_PORTAUDIO
	{
		const std::vector<SoundDeviceInfo> infos = CPortaudioDevice::EnumerateDevices(SNDDEV_PORTAUDIO_WMME);
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
	{
		const std::vector<SoundDeviceInfo> infos = CPortaudioDevice::EnumerateDevices(SNDDEV_PORTAUDIO_ASIO);
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
	{
		const std::vector<SoundDeviceInfo> infos = CPortaudioDevice::EnumerateDevices(SNDDEV_PORTAUDIO_DS);
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
#endif // NO_PORTAUDIO

}


SoundDeviceInfo SoundDevicesManager::FindDeviceInfo(SoundDeviceID id) const
//-------------------------------------------------------------------------
{
	for(std::vector<SoundDeviceInfo>::const_iterator it = begin(); it != end(); ++it)
	{
		if(it->id == id)
		{
			return *it;
		}
	}
	return SoundDeviceInfo();
}


SoundDeviceInfo SoundDevicesManager::FindDeviceInfo(const std::wstring &identifier) const
//---------------------------------------------------------------------------------------
{
	if(m_SoundDevices.empty())
	{
		return SoundDeviceInfo();
	}
	if(identifier.empty())
	{
		return m_SoundDevices[0];
	}
	for(std::vector<SoundDeviceInfo>::const_iterator it = begin(); it != end(); ++it)
	{
		if(it->GetIdentifier() == identifier)
		{
			return *it;
		}
	}
	return SoundDeviceInfo();
}


static SoundDeviceType ParseType(const std::wstring &identifier)
//--------------------------------------------------------------
{
	for(int i = 0; i < SNDDEV_NUM_DEVTYPES; ++i)
	{
		const std::wstring api = SoundDeviceTypeToString(static_cast<SoundDeviceType>(i));
		if(identifier.find(api) == 0)
		{
			return static_cast<SoundDeviceType>(i);
		}
	}
	return SNDDEV_INVALID;
}


SoundDeviceInfo SoundDevicesManager::FindDeviceInfoBestMatch(const std::wstring &identifier) const
//------------------------------------------------------------------------------------------------
{
	if(m_SoundDevices.empty())
	{
		return SoundDeviceInfo();
	}
	if(identifier.empty())
	{
		return m_SoundDevices[0];
	}
	for(std::vector<SoundDeviceInfo>::const_iterator it = begin(); it != end(); ++it)
	{
		if(it->GetIdentifier() == identifier)
		{ // exact match
			return *it;
		}
	}
	const SoundDeviceType type = ParseType(identifier);
	switch(type)
	{
		case SNDDEV_PORTAUDIO_WASAPI:
			// WASAPI devices might change names if a different connector jack is used.
			// In order to avoid defaulting to wave mapper in that case,
			// just find the first WASAPI device.
			for(std::vector<SoundDeviceInfo>::const_iterator it = begin(); it != end(); ++it)
			{
				if(it->id.GetType() == SNDDEV_PORTAUDIO_WASAPI)
				{
					return *it;
				}
			}
			// default to first device
			return *begin();
			break;
		case SNDDEV_WAVEOUT:
		case SNDDEV_DSOUND:
		case SNDDEV_PORTAUDIO_WMME:
		case SNDDEV_PORTAUDIO_DS:
		case SNDDEV_ASIO:
		case SNDDEV_PORTAUDIO_WDMKS:
		case SNDDEV_PORTAUDIO_ASIO:
			// default to first device
			return *begin();
			break;
	}
	// invalid
	return SoundDeviceInfo();
}


bool SoundDevicesManager::OpenDriverSettings(SoundDeviceID id, ISoundMessageReceiver *messageReceiver, ISoundDevice *currentSoundDevice)
//--------------------------------------------------------------------------------------------------------------------------------------
{
	bool result = false;
	if(currentSoundDevice && FindDeviceInfo(id).IsValid() && (currentSoundDevice->GetDeviceID() == id) && (currentSoundDevice->GetDeviceInternalID() == FindDeviceInfo(id).internalID))
	{
		result = currentSoundDevice->OpenDriverSettings();
	} else
	{
		ISoundDevice *dummy = CreateSoundDevice(id);
		if(dummy)
		{
			dummy->SetMessageReceiver(messageReceiver);
			result = dummy->OpenDriverSettings();
		}
		delete dummy;
	}
	return result;
}


SoundDeviceCaps SoundDevicesManager::GetDeviceCaps(SoundDeviceID id, ISoundDevice *currentSoundDevice)
//----------------------------------------------------------------------------------------------------
{
	if(m_DeviceCaps.find(id) == m_DeviceCaps.end())
	{
		if(currentSoundDevice && FindDeviceInfo(id).IsValid() && (currentSoundDevice->GetDeviceID() == id) && (currentSoundDevice->GetDeviceInternalID() == FindDeviceInfo(id).internalID))
		{
			m_DeviceCaps[id] = currentSoundDevice->GetDeviceCaps();
		} else
		{
			ISoundDevice *dummy = CreateSoundDevice(id);
			if(dummy)
			{
				m_DeviceCaps[id] = dummy->GetDeviceCaps();
			}
			delete dummy;
		}
	}
	return m_DeviceCaps[id];
}


SoundDeviceDynamicCaps SoundDevicesManager::GetDeviceDynamicCaps(SoundDeviceID id, const std::vector<uint32> &baseSampleRates, ISoundMessageReceiver *messageReceiver, ISoundDevice *currentSoundDevice, bool update)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if((m_DeviceDynamicCaps.find(id) == m_DeviceDynamicCaps.end()) || update)
	{
		if(currentSoundDevice && FindDeviceInfo(id).IsValid() && (currentSoundDevice->GetDeviceID() == id) && (currentSoundDevice->GetDeviceInternalID() == FindDeviceInfo(id).internalID))
		{
			m_DeviceDynamicCaps[id] = currentSoundDevice->GetDeviceDynamicCaps(baseSampleRates);
		} else
		{
			ISoundDevice *dummy = CreateSoundDevice(id);
			if(dummy)
			{
				dummy->SetMessageReceiver(messageReceiver);
				m_DeviceDynamicCaps[id] = dummy->GetDeviceDynamicCaps(baseSampleRates);
			}
			delete dummy;
		}
	}
	return m_DeviceDynamicCaps[id];
}


ISoundDevice * SoundDevicesManager::CreateSoundDevice(SoundDeviceID id)
//---------------------------------------------------------------------
{
	const SoundDeviceInfo info = FindDeviceInfo(id);
	if(!info.IsValid())
	{
		return nullptr;
	}
	ISoundDevice *result = nullptr;
	switch(id.GetType())
	{
	case SNDDEV_WAVEOUT: result = new CWaveDevice(id, info.internalID); break;
#ifndef NO_DSOUND
	case SNDDEV_DSOUND: result = new CDSoundDevice(id, info.internalID); break;
#endif // NO_DSOUND
#ifndef NO_ASIO
	case SNDDEV_ASIO: result = new CASIODevice(id, info.internalID); break;
#endif // NO_ASIO
#ifndef NO_PORTAUDIO
	case SNDDEV_PORTAUDIO_WASAPI:
	case SNDDEV_PORTAUDIO_WDMKS:
	case SNDDEV_PORTAUDIO_WMME:
	case SNDDEV_PORTAUDIO_DS:
	case SNDDEV_PORTAUDIO_ASIO:
		result = SndDevPortaudioIsInitialized() ? new CPortaudioDevice(id, info.internalID) : nullptr;
		break;
#endif // NO_PORTAUDIO
	}
	return result;
}


SoundDevicesManager::SoundDevicesManager()
//----------------------------------------
{
	ReEnumerate();
}


SoundDevicesManager::~SoundDevicesManager()
//-----------------------------------------
{
#ifndef NO_PORTAUDIO
	SndDevPortaudioUnnitialize();
#endif // NO_PORTAUDIO
}
