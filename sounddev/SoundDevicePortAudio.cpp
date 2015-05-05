/*
 * SoundDevicePortAudio.cpp
 * ------------------------
 * Purpose: PortAudio sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"

#include "SoundDevicePortAudio.h"

#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {

	
///////////////////////////////////////////////////////////////////////////////////////
//
// Portaudio Device implementation
//

#ifndef NO_PORTAUDIO

#define PALOG(x, ...) do { } while(0)
//#define PALOG Log

#include "../include/portaudio/src/common/pa_debugprint.h"


CPortaudioDevice::CPortaudioDevice(SoundDevice::Info info)
//--------------------------------------------------------
	: SoundDevice::Base(info)
	, m_StatisticPeriodFrames(0)
{
	m_DeviceIndex = ConvertStrTo<PaDeviceIndex>(GetDeviceInternalID());
	m_HostApiType = Pa_GetHostApiInfo(Pa_GetDeviceInfo(m_DeviceIndex)->hostApi)->type;
	MemsetZero(m_StreamParameters);
	m_Stream = 0;
	m_StreamInfo = 0;
	m_CurrentFrameCount = 0;
	m_CurrentRealLatency = 0.0;
}


CPortaudioDevice::~CPortaudioDevice()
//-----------------------------------
{
	Close();
}


bool CPortaudioDevice::InternalOpen()
//-----------------------------------
{
	MemsetZero(m_StreamParameters);
	m_Stream = 0;
	m_StreamInfo = 0;
	m_CurrentFrameBuffer = 0;
	m_CurrentFrameCount = 0;
	m_StreamParameters.device = m_DeviceIndex;
	if(m_StreamParameters.device == -1) return false;
	m_StreamParameters.channelCount = m_Settings.Channels;
	if(m_Settings.sampleFormat.IsFloat())
	{
		if(m_Settings.sampleFormat.GetBitsPerSample() != 32) return false;
		m_StreamParameters.sampleFormat = paFloat32;
	} else
	{
		switch(m_Settings.sampleFormat.GetBitsPerSample())
		{
		case 8: m_StreamParameters.sampleFormat = paUInt8; break;
		case 16: m_StreamParameters.sampleFormat = paInt16; break;
		case 24: m_StreamParameters.sampleFormat = paInt24; break;
		case 32: m_StreamParameters.sampleFormat = paInt32; break;
		default: return false; break;
		}
	}
	m_StreamParameters.suggestedLatency = m_Settings.Latency;
	m_StreamParameters.hostApiSpecificStreamInfo = NULL;
	unsigned long framesPerBuffer = static_cast<long>(m_Settings.UpdateInterval * m_Settings.Samplerate);
	if(m_HostApiType == paWASAPI)
	{
		if(m_Settings.ExclusiveMode)
		{
			m_Flags.NeedsClippedFloat = false;
			m_StreamParameters.suggestedLatency = 0.0; // let portaudio choose
			framesPerBuffer = paFramesPerBufferUnspecified; // let portaudio choose
			MemsetZero(m_WasapiStreamInfo);
			m_WasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
			m_WasapiStreamInfo.hostApiType = paWASAPI;
			m_WasapiStreamInfo.version = 1;
			m_WasapiStreamInfo.flags = paWinWasapiExclusive;
			m_StreamParameters.hostApiSpecificStreamInfo = &m_WasapiStreamInfo;
		} else
		{
			m_Flags.NeedsClippedFloat = true;
		}
	} else if(m_HostApiType == paWDMKS)
	{
		m_Flags.NeedsClippedFloat = false;
		framesPerBuffer = paFramesPerBufferUnspecified; // let portaudio choose
	} else if(m_HostApiType == paMME)
	{
		m_Flags.NeedsClippedFloat = mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista);
	} else if(m_HostApiType == paDirectSound)
	{
		m_Flags.NeedsClippedFloat = mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista);
	} else
	{
		m_Flags.NeedsClippedFloat = false;
	}
	if(Pa_IsFormatSupported(NULL, &m_StreamParameters, m_Settings.Samplerate) != paFormatIsSupported) return false;
	PaStreamFlags flags = paNoFlag;
	if(m_Settings.DitherType == 0)
	{
		flags |= paDitherOff;
	}
	if(Pa_OpenStream(&m_Stream, NULL, &m_StreamParameters, m_Settings.Samplerate, framesPerBuffer, flags, StreamCallbackWrapper, (void*)this) != paNoError) return false;
	m_StreamInfo = Pa_GetStreamInfo(m_Stream);
	if(!m_StreamInfo)
	{
		Pa_CloseStream(m_Stream);
		m_Stream = 0;
		return false;
	}
	return true;
}


bool CPortaudioDevice::InternalClose()
//------------------------------------
{
	if(m_Stream)
	{
		const SoundDevice::BufferAttributes bufferAttributes = GetEffectiveBufferAttributes();
		Pa_AbortStream(m_Stream);
		Pa_CloseStream(m_Stream);
		if(Pa_GetDeviceInfo(m_StreamParameters.device)->hostApi == Pa_HostApiTypeIdToHostApiIndex(paWDMKS))
		{
			Pa_Sleep(Util::Round<long>(bufferAttributes.Latency * 2.0 * 1000.0 + 0.5)); // wait for broken wdm drivers not closing the stream immediatly
		}
		MemsetZero(m_StreamParameters);
		m_StreamInfo = 0;
		m_Stream = 0;
		m_CurrentFrameCount = 0;
		m_CurrentFrameBuffer = 0;
	}
	return true;
}


bool CPortaudioDevice::InternalStart()
//------------------------------------
{
	return Pa_StartStream(m_Stream) == paNoError;
}


void CPortaudioDevice::InternalStop()
//-----------------------------------
{
	Pa_StopStream(m_Stream);
}


void CPortaudioDevice::InternalFillAudioBuffer()
//----------------------------------------------
{
	if(m_CurrentFrameCount == 0) return;
	SourceAudioPreRead(m_CurrentFrameCount, Util::Round<std::size_t>(m_CurrentRealLatency * m_StreamInfo->sampleRate));
	SourceAudioRead(m_CurrentFrameBuffer, m_CurrentFrameCount);
	m_StatisticPeriodFrames.store(m_CurrentFrameCount);
	SourceAudioDone();
}


int64 CPortaudioDevice::InternalGetStreamPositionFrames() const
//--------------------------------------------------------------
{
	if(Pa_IsStreamActive(m_Stream) != 1) return 0;
	return static_cast<int64>(Pa_GetStreamTime(m_Stream) * m_StreamInfo->sampleRate);
}


SoundDevice::BufferAttributes CPortaudioDevice::InternalGetEffectiveBufferAttributes() const
//------------------------------------------------------------------------------------------
{
	SoundDevice::BufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_StreamInfo->outputLatency;
	bufferAttributes.UpdateInterval = m_Settings.UpdateInterval;
	bufferAttributes.NumBuffers = 1;
	return bufferAttributes;
}


SoundDevice::Statistics CPortaudioDevice::GetStatistics() const
//-------------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	result.InstantaneousLatency = m_CurrentRealLatency;
	result.LastUpdateInterval = 1.0 * m_StatisticPeriodFrames / m_Settings.Samplerate;
	result.text = mpt::ustring();
	return result;
}


SoundDevice::Caps CPortaudioDevice::InternalGetDeviceCaps()
//-------------------------------------------------------
{
	SoundDevice::Caps caps;
	caps.Available = true;
	caps.CanUpdateInterval = true;
	caps.CanSampleFormat = true;
	caps.CanExclusiveMode = false;
	caps.CanBoostThreadPriority = false;
	caps.CanUseHardwareTiming = false;
	caps.CanChannelMapping = false;
	caps.CanDriverPanel = false;
	caps.HasInternalDither = true;
	const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(m_DeviceIndex);
	if(deviceInfo)
	{
		caps.DefaultSettings.Latency = deviceInfo->defaultLowOutputLatency;
	}
	caps.DefaultSettings.sampleFormat = SampleFormatFloat32;
	if(m_HostApiType == paWASAPI)
	{
		caps.CanExclusiveMode = true;
		caps.CanDriverPanel = true;
		caps.DefaultSettings.sampleFormat = SampleFormatFloat32;
	} else if(m_HostApiType == paWDMKS)
	{
		caps.CanUpdateInterval = false;
		caps.DefaultSettings.sampleFormat = SampleFormatInt32;
	} else if(m_HostApiType == paDirectSound)
	{
		caps.DefaultSettings.sampleFormat = SampleFormatInt16;
	} else if(m_HostApiType == paMME)
	{
		if(mpt::Windows::Version::IsWine())
		{
			caps.DefaultSettings.sampleFormat = SampleFormatInt16;
		} else if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista))
		{
			caps.DefaultSettings.sampleFormat = SampleFormatFloat32;
		} else
		{
			caps.DefaultSettings.sampleFormat = SampleFormatInt16;
		}
	} else if(m_HostApiType == paASIO)
	{
		caps.DefaultSettings.sampleFormat = SampleFormatInt32;
	}
	return caps;
}


SoundDevice::DynamicCaps CPortaudioDevice::GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates)
//-------------------------------------------------------------------------------------------------------
{
	SoundDevice::DynamicCaps caps;
	PaDeviceIndex device = m_DeviceIndex;
	if(device == -1)
	{
		return caps;
	}
	for(UINT n=0; n<baseSampleRates.size(); n++)
	{
		PaStreamParameters StreamParameters;
		MemsetZero(StreamParameters);
		StreamParameters.device = device;
		StreamParameters.channelCount = 2;
		StreamParameters.sampleFormat = paInt16;
		StreamParameters.suggestedLatency = 0.0;
		StreamParameters.hostApiSpecificStreamInfo = NULL;
		if((m_HostApiType == paWASAPI) && m_Settings.ExclusiveMode)
		{
			MemsetZero(m_WasapiStreamInfo);
			m_WasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
			m_WasapiStreamInfo.hostApiType = paWASAPI;
			m_WasapiStreamInfo.version = 1;
			m_WasapiStreamInfo.flags = paWinWasapiExclusive;
			m_StreamParameters.hostApiSpecificStreamInfo = &m_WasapiStreamInfo;
		}
		if(Pa_IsFormatSupported(NULL, &StreamParameters, baseSampleRates[n]) == paFormatIsSupported)
		{
			caps.supportedSampleRates.push_back(baseSampleRates[n]);
			caps.supportedExclusiveSampleRates.push_back(baseSampleRates[n]);
		}
	}
	return caps;
}


bool CPortaudioDevice::OpenDriverSettings()
//-----------------------------------------
{
	if(m_HostApiType != paWASAPI)
	{
		return false;
	}
	bool hasVista = mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista);
	mpt::PathString controlEXE;
	WCHAR systemDir[MAX_PATH];
	MemsetZero(systemDir);
	if(GetSystemDirectoryW(systemDir, CountOf(systemDir)) > 0)
	{
		controlEXE += mpt::PathString::FromNative(systemDir);
		controlEXE += MPT_PATHSTRING("\\");
	}
	controlEXE += MPT_PATHSTRING("control.exe");
	return ((int)ShellExecuteW(NULL, L"open", controlEXE.AsNative().c_str(), (hasVista ? L"/name Microsoft.Sound" : L"mmsys.cpl"), NULL, SW_SHOW) > 32);
}


int CPortaudioDevice::StreamCallback(
	const void *input, void *output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags
	)
//-----------------------------------------
{
	MPT_UNREFERENCED_PARAMETER(input);
	MPT_UNREFERENCED_PARAMETER(statusFlags);
	if(!output) return paAbort;
	if(m_HostApiType == paWDMKS)
	{
		// For WDM-KS, timeInfo->outputBufferDacTime seems to contain bogus values.
		// Work-around it by using the slightly less accurate per-stream latency estimation.
		m_CurrentRealLatency = m_StreamInfo->outputLatency;
	} else if(m_HostApiType == paWASAPI)
	{
		// PortAudio latency calculation appears to miss the current period or chunk for WASAPI. Compensate it.
		m_CurrentRealLatency = timeInfo->outputBufferDacTime - timeInfo->currentTime + ((double)frameCount / (double)m_Settings.Samplerate);
	} else if(m_HostApiType == paDirectSound)
	{
		// PortAudio latency calculation appears to miss the buffering latency.
		// The current chunk, however, appears to be compensated for.
		// Repair the confusion.
		m_CurrentRealLatency = timeInfo->outputBufferDacTime - timeInfo->currentTime + m_StreamInfo->outputLatency - ((double)frameCount / (double)m_Settings.Samplerate);
	} else
	{
		m_CurrentRealLatency = timeInfo->outputBufferDacTime - timeInfo->currentTime;
	}
	m_CurrentFrameBuffer = output;
	m_CurrentFrameCount = frameCount;
	SourceFillAudioBufferLocked();
	m_CurrentFrameCount = 0;
	m_CurrentFrameBuffer = 0;
	return paContinue;
}


int CPortaudioDevice::StreamCallbackWrapper(
	const void *input, void *output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData
	)
//------------------------------------------
{
	return ((CPortaudioDevice*)userData)->StreamCallback(input, output, frameCount, timeInfo, statusFlags);
}


std::vector<SoundDevice::Info> CPortaudioDevice::EnumerateDevices()
//-----------------------------------------------------------------
{
	std::vector<SoundDevice::Info> devices;
	for(PaDeviceIndex dev = 0; dev < Pa_GetDeviceCount(); ++dev)
	{
		if(!Pa_GetDeviceInfo(dev))
		{
			continue;
		}
		if(Pa_GetDeviceInfo(dev)->hostApi < 0)
		{
			continue;
		}
		if(!Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi))
		{
			continue;
		}
		if(!Pa_GetDeviceInfo(dev)->name)
		{
			continue;
		}
		if(!Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->name)
		{
			continue;
		}
		if(Pa_GetDeviceInfo(dev)->maxOutputChannels <= 0)
		{
			continue;
		}
		SoundDevice::Info result = SoundDevice::Info();
		switch((Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->type))
		{
			case paWASAPI:
				result.type = TypePORTAUDIO_WASAPI;
				break;
			case paWDMKS:
				result.type = TypePORTAUDIO_WDMKS;
				break;
			case paMME:
				result.type = TypePORTAUDIO_WMME;
				break;
			case paDirectSound:
				result.type = TypePORTAUDIO_DS;
				break;
			default:
				result.type = MPT_USTRING("PortAudio") + MPT_USTRING("-") + mpt::ufmt::dec(static_cast<int>(Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->type));
				break;
		}
		result.internalID = mpt::ufmt::dec(dev);
		result.name = mpt::ToUnicode(mpt::CharsetUTF8, Pa_GetDeviceInfo(dev)->name);
		switch(Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->type)
		{
			case paWASAPI:
				result.apiName = MPT_USTRING("WASAPI");
				break;
			case paWDMKS:
				if(mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista))
				{
					result.apiName = MPT_USTRING("WaveRT");
				} else
				{
					result.apiName = MPT_USTRING("WDM-KS");
				}
				break;
			default:
				result.apiName = mpt::ToUnicode(mpt::CharsetUTF8, Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->name);
				break;
		}
		result.apiPath.push_back(MPT_USTRING("PortAudio"));
		result.isDefault = (Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->defaultOutputDevice == (PaDeviceIndex)dev);
		result.useNameAsIdentifier = true;
		PALOG(mpt::String::Print("PortAudio: %1, %2, %3, %4", result.id.GetIdRaw(), mpt::ToLocale(result.name), mpt::ToLocale(result.apiName), result.isDefault));
		PALOG(mpt::String::Print(" low  : %1", Pa_GetDeviceInfo(dev)->defaultLowOutputLatency));
		PALOG(mpt::String::Print(" high : %1", Pa_GetDeviceInfo(dev)->defaultHighOutputLatency));
		devices.push_back(result);
	}
	return devices;
}


static void PortaudioLog(const char *text)
//----------------------------------------
{
	if(text)
	{
		PALOG(mpt::String::Print("PortAudio: %1", text));
	}
}


MPT_REGISTERED_COMPONENT(ComponentPortAudio)


ComponentPortAudio::ComponentPortAudio()
{
	return;
}


bool ComponentPortAudio::DoInitialize()
{
	PaUtil_SetDebugPrintFunction(PortaudioLog);
	return (Pa_Initialize() == paNoError);
}


ComponentPortAudio::~ComponentPortAudio()
{
	if(IsAvailable())
	{
		Pa_Terminate();
	}
}


bool ComponentPortAudio::ReInit()
{
	if(!IsAvailable())
	{
		return false;
	}
	Pa_Terminate();
	return DoInitialize();
}


#endif // NO_PORTAUDIO


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
