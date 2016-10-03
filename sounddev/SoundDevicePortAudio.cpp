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


#ifdef MPT_WITH_PORTAUDIO
#if defined(MODPLUG_TRACKER) && MPT_COMPILER_MSVC
#include "../include/portaudio/src/common/pa_debugprint.h"
#endif
#if MPT_OS_WINDOWS
#include <shellapi.h>
#endif
#endif


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {

	
#ifdef MPT_WITH_PORTAUDIO

#define PALOG(x, ...) do { } while(0)
//#define PALOG Log



CPortaudioDevice::CPortaudioDevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
//--------------------------------------------------------------------------------------
	: SoundDevice::Base(info, sysInfo)
	, m_StatisticPeriodFrames(0)
{
	m_DeviceIndex = ConvertStrTo<PaDeviceIndex>(GetDeviceInternalID());
	m_HostApiType = Pa_GetHostApiInfo(Pa_GetDeviceInfo(m_DeviceIndex)->hostApi)->type;
	MemsetZero(m_StreamParameters);
	MemsetZero(m_InputStreamParameters);
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
	MemsetZero(m_InputStreamParameters);
	m_Stream = 0;
	m_StreamInfo = 0;
	m_CurrentFrameBuffer = 0;
	m_CurrentFrameBufferInput = 0;
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
#if MPT_OS_WINDOWS
			MemsetZero(m_WasapiStreamInfo);
			m_WasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
			m_WasapiStreamInfo.hostApiType = paWASAPI;
			m_WasapiStreamInfo.version = 1;
			m_WasapiStreamInfo.flags = paWinWasapiExclusive;
			m_StreamParameters.hostApiSpecificStreamInfo = &m_WasapiStreamInfo;
#endif // MPT_OS_WINDOWS
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
		m_Flags.NeedsClippedFloat = GetSysInfo().WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista);
	} else if(m_HostApiType == paDirectSound)
	{
		m_Flags.NeedsClippedFloat = GetSysInfo().WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista);
	} else
	{
		m_Flags.NeedsClippedFloat = false;
	}
	m_InputStreamParameters = m_StreamParameters;
	if(!HasInputChannelsOnSameDevice())
	{
		m_InputStreamParameters.device = static_cast<PaDeviceIndex>(m_Settings.InputSourceID);
	}
	m_InputStreamParameters.channelCount = m_Settings.InputChannels;
	if(Pa_IsFormatSupported((m_Settings.InputChannels > 0) ? &m_InputStreamParameters : NULL, &m_StreamParameters, m_Settings.Samplerate) != paFormatIsSupported) return false;
	PaStreamFlags flags = paNoFlag;
	if(m_Settings.DitherType == 0)
	{
		flags |= paDitherOff;
	}
	if(Pa_OpenStream(&m_Stream, (m_Settings.InputChannels > 0) ? &m_InputStreamParameters : NULL, &m_StreamParameters, m_Settings.Samplerate, framesPerBuffer, flags, StreamCallbackWrapper, reinterpret_cast<void*>(this)) != paNoError) return false;
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
		MemsetZero(m_InputStreamParameters);
		m_StreamInfo = 0;
		m_Stream = 0;
		m_CurrentFrameCount = 0;
		m_CurrentFrameBuffer = 0;
		m_CurrentFrameBufferInput = 0;
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
	SourceLockedAudioPreRead(m_CurrentFrameCount, Util::Round<std::size_t>(m_CurrentRealLatency * m_StreamInfo->sampleRate));
	SourceLockedAudioRead(m_CurrentFrameBuffer, m_CurrentFrameBufferInput, m_CurrentFrameCount);
	m_StatisticPeriodFrames.store(m_CurrentFrameCount);
	SourceLockedAudioDone();
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
	if(m_HostApiType == paWASAPI && m_Settings.ExclusiveMode)
	{
		// WASAPI exclusive mode streams only account for a single period of latency in PortAudio
		// (i.e. the same way as Steinerg ASIO defines latency).
		// That does not match our definition of latency, repair it.
		bufferAttributes.Latency *= 2.0;
	}
	return bufferAttributes;
}


bool CPortaudioDevice::OnIdle()
//-----------------------------
{
	if(!IsPlaying())
	{
		return false;
	}
	if(m_Stream)
	{
		if(m_HostApiType == paWDMKS)
		{
			// Catch timeouts in PortAudio threading code that cause the thread to exit.
			// Restore our desired playback state by resetting the whole sound device.
			if(Pa_IsStreamActive(m_Stream) <= 0)
			{
				// Hung state tends to be caused by an overloaded system.
				// Sleeping too long would freeze the UI,
				// but at least sleep a tiny bit of time to let things settle down.
				const SoundDevice::BufferAttributes bufferAttributes = GetEffectiveBufferAttributes();
				Pa_Sleep(Util::Round<long>(bufferAttributes.Latency * 2.0 * 1000.0 + 0.5));
				RequestReset();
				return true;
			}
		}
	}
	return false;
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
	caps.CanInput = false;
	caps.HasNamedInputSources = false;
	caps.CanDriverPanel = false;
	caps.HasInternalDither = true;
	caps.DefaultSettings.sampleFormat = SampleFormatFloat32;
	const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(m_DeviceIndex);
	if(deviceInfo)
	{
		caps.DefaultSettings.Latency = deviceInfo->defaultLowOutputLatency;
	}
	if(HasInputChannelsOnSameDevice())
	{
		caps.CanInput = true;
		caps.HasNamedInputSources = false;
	} else
	{
		caps.CanInput = (EnumerateInputOnlyDevices(m_HostApiType).size() > 0);
		caps.HasNamedInputSources = caps.CanInput;
	}
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
		if(GetSysInfo().IsWine)
		{
			caps.DefaultSettings.sampleFormat = SampleFormatInt16;
		} else if(GetSysInfo().WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista))
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
	if(device == paNoDevice)
	{
		return caps;
	}
	for(std::size_t n = 0; n<baseSampleRates.size(); n++)
	{
		PaStreamParameters StreamParameters;
		MemsetZero(StreamParameters);
		StreamParameters.device = device;
		StreamParameters.channelCount = 2;
		StreamParameters.sampleFormat = paInt16;
		StreamParameters.suggestedLatency = 0.0;
		StreamParameters.hostApiSpecificStreamInfo = NULL;
#if MPT_OS_WINDOWS
		if((m_HostApiType == paWASAPI) && m_Settings.ExclusiveMode)
		{
			MemsetZero(m_WasapiStreamInfo);
			m_WasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
			m_WasapiStreamInfo.hostApiType = paWASAPI;
			m_WasapiStreamInfo.version = 1;
			m_WasapiStreamInfo.flags = paWinWasapiExclusive;
			m_StreamParameters.hostApiSpecificStreamInfo = &m_WasapiStreamInfo;
		}
#endif // MPT_OS_WINDOWS
		if(Pa_IsFormatSupported(NULL, &StreamParameters, baseSampleRates[n]) == paFormatIsSupported)
		{
			caps.supportedSampleRates.push_back(baseSampleRates[n]);
			caps.supportedExclusiveSampleRates.push_back(baseSampleRates[n]);
		}
	}

	if(!HasInputChannelsOnSameDevice())
	{
		caps.inputSourceNames.clear();
		auto inputDevices = EnumerateInputOnlyDevices(m_HostApiType);
		for(auto it = inputDevices.cbegin(); it != inputDevices.cend(); ++it)
		{
			caps.inputSourceNames.push_back(std::make_pair(static_cast<uint32>(it->first), it->second));
		}
	}

	return caps;
}


bool CPortaudioDevice::OpenDriverSettings()
//-----------------------------------------
{
#if MPT_OS_WINDOWS
	if(m_HostApiType != paWASAPI)
	{
		return false;
	}
	bool hasVista = GetSysInfo().WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista);
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
#else // !MPT_OS_WINDOWS
	return false;
#endif // MPT_OS_WINDOWS
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
	if(!output) return paAbort;
	if(m_HostApiType == paWDMKS)
	{
		// For WDM-KS, timeInfo->outputBufferDacTime seems to contain bogus values.
		// Work-around it by using the slightly less accurate per-stream latency estimation.
		m_CurrentRealLatency = m_StreamInfo->outputLatency;
	} else if(m_HostApiType == paWASAPI)
	{
		// PortAudio latency calculation appears to miss the current period or chunk for WASAPI. Compensate it.
		m_CurrentRealLatency = timeInfo->outputBufferDacTime - timeInfo->currentTime + (static_cast<double>(frameCount) / static_cast<double>(m_Settings.Samplerate));
	} else if(m_HostApiType == paDirectSound)
	{
		// PortAudio latency calculation appears to miss the buffering latency.
		// The current chunk, however, appears to be compensated for.
		// Repair the confusion.
		m_CurrentRealLatency = timeInfo->outputBufferDacTime - timeInfo->currentTime + m_StreamInfo->outputLatency - (static_cast<double>(frameCount) / static_cast<double>(m_Settings.Samplerate));
	} else if(m_HostApiType == paALSA)
	{
		// PortAudio latency calculation appears to miss the buffering latency.
		// The current chunk, however, appears to be compensated for.
		// Repair the confusion.
		m_CurrentRealLatency = timeInfo->outputBufferDacTime - timeInfo->currentTime + m_StreamInfo->outputLatency - (static_cast<double>(frameCount) / static_cast<double>(m_Settings.Samplerate));
	} else
	{
		m_CurrentRealLatency = timeInfo->outputBufferDacTime - timeInfo->currentTime;
	}
	m_CurrentFrameBuffer = output;
	m_CurrentFrameBufferInput = input;
	m_CurrentFrameCount = frameCount;
	SourceFillAudioBufferLocked();
	m_CurrentFrameCount = 0;
	m_CurrentFrameBuffer = 0;
	m_CurrentFrameBufferInput = 0;
	if((m_HostApiType == paALSA) && (statusFlags & paOutputUnderflow))
	{
		// PortAudio ALSA does not recover well from buffer underruns
		RequestRestart();
	}
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
	return reinterpret_cast<CPortaudioDevice*>(userData)->StreamCallback(input, output, frameCount, timeInfo, statusFlags);
}


std::vector<SoundDevice::Info> CPortaudioDevice::EnumerateDevices(SoundDevice::SysInfo sysInfo)
//---------------------------------------------------------------------------------------------
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
				if(sysInfo.WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista))
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
		result.isDefault = (Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->defaultOutputDevice == static_cast<PaDeviceIndex>(dev));
		result.useNameAsIdentifier = true;
		PALOG(mpt::format(MPT_USTRING("PortAudio: %1, %2, %3, %4"))(result.id.GetIdRaw(), result.name, result.apiName, result.isDefault));
		PALOG(mpt::format(MPT_USTRING(" low  : %1"))(mpt::ToUnicode(mpt::CharsetUTF8, Pa_GetDeviceInfo(dev)->defaultLowOutputLatency)));
		PALOG(mpt::format(MPT_USTRING(" high : %1"))(mpt::ToUnicode(mpt::CharsetUTF8, Pa_GetDeviceInfo(dev)->defaultHighOutputLatency)));
		devices.push_back(result);
	}
	return devices;
}


std::vector<std::pair<PaDeviceIndex, mpt::ustring> > CPortaudioDevice::EnumerateInputOnlyDevices(PaHostApiTypeId hostApiType)
//---------------------------------------------------------------------------------------------------------------------------
{
	std::vector<std::pair<PaDeviceIndex, mpt::ustring> > result;
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
		if(Pa_GetDeviceInfo(dev)->maxInputChannels <= 0)
		{
			continue;
		}
		if(Pa_GetDeviceInfo(dev)->maxOutputChannels > 0)
		{ // only find devices with only input channels
			continue;
		}
		if(Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->type != hostApiType)
		{
			continue;
		}
		result.push_back(std::make_pair(dev, mpt::ToUnicode(mpt::CharsetUTF8, Pa_GetDeviceInfo(dev)->name)));
	}
	return result;
}


bool CPortaudioDevice::HasInputChannelsOnSameDevice() const
//---------------------------------------------------------
{
	if(m_DeviceIndex == paNoDevice)
	{
		return false;
	}
	const PaDeviceInfo *deviceinfo = Pa_GetDeviceInfo(m_DeviceIndex);
	if(!deviceinfo)
	{
		return false;
	}
	return (deviceinfo->maxInputChannels > 0);
}


#if MPT_COMPILER_MSVC
static void PortaudioLog(const char *text)
//----------------------------------------
{
	if(!text)
	{
		return;
	}
	PALOG(mpt::format(MPT_USTRING("PortAudio: %1"))(mpt::ToUnicode(mpt::CharsetUTF8, text)));
}
#endif // MPT_COMPILER_MSVC


MPT_REGISTERED_COMPONENT(ComponentPortAudio, "PortAudio")


ComponentPortAudio::ComponentPortAudio()
{
	return;
}


bool ComponentPortAudio::DoInitialize()
{
	#if defined(MODPLUG_TRACKER) && MPT_COMPILER_MSVC
		PaUtil_SetDebugPrintFunction(PortaudioLog);
	#endif
	return (Pa_Initialize() == paNoError);
}


ComponentPortAudio::~ComponentPortAudio()
{
	if(IsAvailable())
	{
		Pa_Terminate();
	}
}


#endif // MPT_WITH_PORTAUDIO


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
