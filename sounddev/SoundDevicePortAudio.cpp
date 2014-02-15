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
#include "SoundDevices.h"

#include "SoundDevicePortAudio.h"

#include "../common/misc_util.h"


///////////////////////////////////////////////////////////////////////////////////////
//
// Portaudio Device implementation
//

#ifndef NO_PORTAUDIO


#include "../include/portaudio/src/common/pa_debugprint.h"


CPortaudioDevice::CPortaudioDevice(SoundDeviceID id, const std::wstring &internalID)
//----------------------------------------------------------------------------------
	: ISoundDevice(id, internalID)
{
	m_HostApi = SndDevTypeToHostApi(id.GetType());
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
	m_StreamParameters.device = HostApiOutputIndexToGlobalDeviceIndex(GetDeviceIndex(), m_HostApi);
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
	m_StreamParameters.suggestedLatency = m_Settings.LatencyMS / 1000.0;
	m_StreamParameters.hostApiSpecificStreamInfo = NULL;
	unsigned long framesPerBuffer = static_cast<long>(m_Settings.UpdateIntervalMS * m_Settings.Samplerate / 1000.0f);
	if(m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI))
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
	} else if(m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWDMKS))
	{
		m_Flags.NeedsClippedFloat = false;
		framesPerBuffer = paFramesPerBufferUnspecified; // let portaudio choose
	} else if(m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paMME))
	{
		m_Flags.NeedsClippedFloat = (mpt::Windows::GetWinNTVersion() >= mpt::Windows::VerWinVista);
	} else if(m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paDirectSound))
	{
		m_Flags.NeedsClippedFloat = (mpt::Windows::GetWinNTVersion() >= mpt::Windows::VerWinVista);
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
	SoundBufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_StreamInfo->outputLatency;
	bufferAttributes.UpdateInterval = m_Settings.UpdateIntervalMS / 1000.0;
	bufferAttributes.NumBuffers = 1;
	UpdateBufferAttributes(bufferAttributes);
	return true;
}


bool CPortaudioDevice::InternalClose()
//------------------------------------
{
	if(m_Stream)
	{
		Pa_AbortStream(m_Stream);
		Pa_CloseStream(m_Stream);
		if(Pa_GetDeviceInfo(m_StreamParameters.device)->hostApi == Pa_HostApiTypeIdToHostApiIndex(paWDMKS))
		{
			Pa_Sleep(Util::Round<long>(GetBufferAttributes().Latency * 2.0 * 1000.0 + 0.5)); // wait for broken wdm drivers not closing the stream immediatly
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
	SourceAudioPreRead(m_CurrentFrameCount);
	SourceAudioRead(m_CurrentFrameBuffer, m_CurrentFrameCount);
	SourceAudioDone(m_CurrentFrameCount, Util::Round<int32>(m_CurrentRealLatency * m_StreamInfo->sampleRate));
}


int64 CPortaudioDevice::InternalGetStreamPositionFrames() const
//--------------------------------------------------------------
{
	if(Pa_IsStreamActive(m_Stream) != 1) return 0;
	return static_cast<int64>(Pa_GetStreamTime(m_Stream) * m_StreamInfo->sampleRate);
}


double CPortaudioDevice::GetCurrentLatency() const
//------------------------------------------------
{
	return m_CurrentRealLatency;
}


SoundDeviceCaps CPortaudioDevice::GetDeviceCaps(const std::vector<uint32> &baseSampleRates)
//-----------------------------------------------------------------------------------------
{
	SoundDeviceCaps caps;
	caps.CanUpdateInterval = true;
	caps.CanSampleFormat = true;
	caps.CanExclusiveMode = false;
	caps.CanBoostThreadPriority = false;
	caps.CanUseHardwareTiming = false;
	caps.CanChannelMapping = false;
	caps.CanDriverPanel = false;
	caps.HasInternalDither = true;
	if(m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI))
	{
		caps.CanExclusiveMode = true;
		caps.CanDriverPanel = true;
	} else if(m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWDMKS))
	{
		caps.CanUpdateInterval = false;
	}
	PaDeviceIndex device = HostApiOutputIndexToGlobalDeviceIndex(GetDeviceIndex(), m_HostApi);
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
		if((m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI)) && m_Settings.ExclusiveMode)
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
		}
	}
	return caps;
}


bool CPortaudioDevice::OpenDriverSettings()
//-----------------------------------------
{
	if(m_HostApi != Pa_HostApiTypeIdToHostApiIndex(paWASAPI))
	{
		return false;
	}
	OSVERSIONINFO versioninfo;
	MemsetZero(versioninfo);
	versioninfo.dwOSVersionInfoSize = sizeof(versioninfo);
	GetVersionEx(&versioninfo);
	const bool hasVista = (versioninfo.dwMajorVersion >= 6);
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
	if(Pa_GetHostApiInfo(m_HostApi)->type == paWDMKS)
	{
		// For WDM-KS, timeInfo->outputBufferDacTime seems to contain bogus values.
		// Work-around it by using the slightly less accurate per-stream latency estimation.
		m_CurrentRealLatency = m_StreamInfo->outputLatency;
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


PaDeviceIndex CPortaudioDevice::HostApiOutputIndexToGlobalDeviceIndex(int hostapioutputdeviceindex, PaHostApiIndex hostapi)
//-------------------------------------------------------------------------------------------------------------------
{
	if(hostapi < 0)
		return -1;
	if(hostapi >= Pa_GetHostApiCount())
		return -1;
	if(!Pa_GetHostApiInfo(hostapi))
		return -1;
	if(hostapioutputdeviceindex < 0)
		return -1;
	if(hostapioutputdeviceindex >= Pa_GetHostApiInfo(hostapi)->deviceCount)
		return -1;
	int dev = hostapioutputdeviceindex;
	for(int hostapideviceindex=0; hostapideviceindex<Pa_GetHostApiInfo(hostapi)->deviceCount; hostapideviceindex++)
	{
		if(!Pa_GetDeviceInfo(Pa_HostApiDeviceIndexToDeviceIndex(hostapi, hostapideviceindex)))
		{
			dev++; // skip this device
			continue;
		}
		if(Pa_GetDeviceInfo(Pa_HostApiDeviceIndexToDeviceIndex(hostapi, hostapideviceindex))->maxOutputChannels == 0)
		{
			dev++; // skip this device
			continue;
		}
		if(dev == hostapideviceindex)
		{
			break;
		}
	}
	if(dev >= Pa_GetHostApiInfo(hostapi)->deviceCount)
		return -1;
	return Pa_HostApiDeviceIndexToDeviceIndex(hostapi, dev);
}


SoundDeviceType CPortaudioDevice::HostApiToSndDevType(PaHostApiIndex hostapi)
//---------------------------------------------------------------------------
{
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI)) return SNDDEV_PORTAUDIO_WASAPI;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paWDMKS)) return SNDDEV_PORTAUDIO_WDMKS;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paMME)) return SNDDEV_PORTAUDIO_WMME;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paDirectSound)) return SNDDEV_PORTAUDIO_DS;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paASIO)) return SNDDEV_PORTAUDIO_ASIO;
	return SNDDEV_INVALID;
}


PaHostApiIndex CPortaudioDevice::SndDevTypeToHostApi(SoundDeviceType snddevtype)
//------------------------------------------------------------------------------
{
	if(snddevtype == SNDDEV_PORTAUDIO_WASAPI) return Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
	if(snddevtype == SNDDEV_PORTAUDIO_WDMKS) return Pa_HostApiTypeIdToHostApiIndex(paWDMKS);
	if(snddevtype == SNDDEV_PORTAUDIO_WMME) return Pa_HostApiTypeIdToHostApiIndex(paMME);
	if(snddevtype == SNDDEV_PORTAUDIO_DS) return Pa_HostApiTypeIdToHostApiIndex(paDirectSound);
	if(snddevtype == SNDDEV_PORTAUDIO_ASIO) return Pa_HostApiTypeIdToHostApiIndex(paASIO);
	return paInDevelopment;
}


std::wstring CPortaudioDevice::HostApiToString(PaHostApiIndex hostapi)
//--------------------------------------------------------------------
{
	SoundDeviceType type = HostApiToSndDevType(hostapi);
	if(type == SNDDEV_INVALID)
	{
		return L"PortAudio";
	}
	return SoundDeviceTypeToString(type);
}


bool CPortaudioDevice::EnumerateDevices(SoundDeviceInfo &result, SoundDeviceIndex index, PaHostApiIndex hostapi)
//--------------------------------------------------------------------------------------------------------------
{
	result = SoundDeviceInfo();
	PaDeviceIndex dev = HostApiOutputIndexToGlobalDeviceIndex(index, hostapi);
	if(dev == -1)
		return false;
	if(!Pa_GetDeviceInfo(dev))
		return false;
	result.id = SoundDeviceID(HostApiToSndDevType(hostapi), index);
	result.name = mpt::ToWide(mpt::CharsetUTF8, Pa_GetDeviceInfo(dev)->name);
	result.apiName = HostApiToString(Pa_GetDeviceInfo(dev)->hostApi);
	result.isDefault = (Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->defaultOutputDevice == (PaDeviceIndex)dev);
	Log(mpt::String::Print("PortAudio: %1, %2, %3, %4", result.id.GetIdRaw(), mpt::ToLocale(result.name), mpt::ToLocale(result.apiName), result.isDefault));
	Log(mpt::String::Print(" low  : %1", Pa_GetDeviceInfo(dev)->defaultLowOutputLatency));
	Log(mpt::String::Print(" high : %1", Pa_GetDeviceInfo(dev)->defaultHighOutputLatency));
	return true;
}


std::vector<SoundDeviceInfo> CPortaudioDevice::EnumerateDevices(SoundDeviceType type)
//-----------------------------------------------------------------------------------
{
	std::vector<SoundDeviceInfo> devices;
	if(!SndDevPortaudioIsInitialized())
	{
		return devices;
	}
	for(SoundDeviceIndex index = 0; ; ++index)
	{
		SoundDeviceInfo info;
		if(!EnumerateDevices(info, index, CPortaudioDevice::SndDevTypeToHostApi(type)))
		{
			break;
		}
		devices.push_back(info);
	}
	return devices;
}


static bool g_PortaudioInitialized = false;


static void PortaudioLog(const char *text) {
	if(text)
	{
		Log("PortAudio: %s", text);
	}
}


void SndDevPortaudioInitialize()
//------------------------------
{
	PaUtil_SetDebugPrintFunction(PortaudioLog);
	if(Pa_Initialize() != paNoError) return;
	g_PortaudioInitialized = true;
}


void SndDevPortaudioUnnitialize()
//-------------------------------
{
	if(!g_PortaudioInitialized) return;
	Pa_Terminate();
	g_PortaudioInitialized = false;
}


bool SndDevPortaudioIsInitialized()
//---------------------------------
{
	return g_PortaudioInitialized;
}


#endif // NO_PORTAUDIO
