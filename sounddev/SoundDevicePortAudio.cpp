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

CPortaudioDevice::CPortaudioDevice(PaHostApiIndex hostapi)
//--------------------------------------------------------
{
	m_HostApi = hostapi;
	MemsetZero(m_StreamParameters);
	m_Stream = 0;
	m_CurrentFrameCount = 0;
	m_CurrentRealLatencyMS = 0.0f;
}


CPortaudioDevice::~CPortaudioDevice()
//-----------------------------------
{
	Reset();
	Close();
}


bool CPortaudioDevice::InternalOpen(UINT nDevice)
//-----------------------------------------------
{
	MemsetZero(m_StreamParameters);
	m_Stream = 0;
	m_CurrentFrameBuffer = 0;
	m_CurrentFrameCount = 0;
	m_StreamParameters.device = HostApiOutputIndexToGlobalDeviceIndex(nDevice, m_HostApi);
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
	if((m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI)) && (m_Settings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE))
	{
		MemsetZero(m_WasapiStreamInfo);
		m_WasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
		m_WasapiStreamInfo.hostApiType = paWASAPI;
		m_WasapiStreamInfo.version = 1;
		m_WasapiStreamInfo.flags = paWinWasapiExclusive;
		m_StreamParameters.hostApiSpecificStreamInfo = &m_WasapiStreamInfo;
	}
	if(Pa_IsFormatSupported(NULL, &m_StreamParameters, m_Settings.Samplerate) != paFormatIsSupported) return false;
	if(Pa_OpenStream(&m_Stream, NULL, &m_StreamParameters, m_Settings.Samplerate, /*static_cast<long>(m_UpdateIntervalMS * pwfx->nSamplesPerSec / 1000.0f)*/ paFramesPerBufferUnspecified, paNoFlag, StreamCallbackWrapper, (void*)this) != paNoError) return false;
	if(!Pa_GetStreamInfo(m_Stream))
	{
		Pa_CloseStream(m_Stream);
		m_Stream = 0;
		return false;
	}
	m_RealLatencyMS = static_cast<float>(Pa_GetStreamInfo(m_Stream)->outputLatency) * 1000.0f;
	m_RealUpdateIntervalMS = static_cast<float>(m_Settings.UpdateIntervalMS);
	return true;
}


bool CPortaudioDevice::InternalClose()
//------------------------------------
{
	if(m_Stream)
	{
		Pa_AbortStream(m_Stream);
		Pa_CloseStream(m_Stream);
		if(Pa_GetDeviceInfo(m_StreamParameters.device)->hostApi == Pa_HostApiTypeIdToHostApiIndex(paWDMKS)) Pa_Sleep((long)(m_RealLatencyMS*2)); // wait for broken wdm drivers not closing the stream immediatly
		MemsetZero(m_StreamParameters);
		m_Stream = 0;
		m_CurrentFrameCount = 0;
		m_CurrentFrameBuffer = 0;
	}
	return true;
}


void CPortaudioDevice::InternalReset()
//------------------------------------
{
	Pa_AbortStream(m_Stream);
}


void CPortaudioDevice::InternalStart()
//------------------------------------
{
	Pa_StartStream(m_Stream);
}


void CPortaudioDevice::InternalStop()
//-----------------------------------
{
	Pa_StopStream(m_Stream);
}


void CPortaudioDevice::FillAudioBuffer()
//--------------------------------------
{
	if(m_CurrentFrameCount == 0) return;
	SourceAudioRead(m_CurrentFrameBuffer, m_CurrentFrameCount);
	SourceAudioDone(m_CurrentFrameCount, static_cast<ULONG>(m_CurrentRealLatencyMS * Pa_GetStreamInfo(m_Stream)->sampleRate / 1000.0f));
}


int64 CPortaudioDevice::GetStreamPositionSamples() const
//------------------------------------------------------
{
	if(!IsOpen()) return 0;
	if(Pa_IsStreamActive(m_Stream) != 1) return 0;
	return static_cast<int64>(Pa_GetStreamTime(m_Stream) * Pa_GetStreamInfo(m_Stream)->sampleRate);
}


float CPortaudioDevice::GetCurrentRealLatencyMS()
//-----------------------------------------------
{
	if(!IsOpen()) return 0.0f;
	return m_CurrentRealLatencyMS;
}


bool CPortaudioDevice::CanSampleRate(UINT nDevice, std::vector<UINT> &samplerates, std::vector<bool> &result)
//-----------------------------------------------------------------------------------------------------------
{
	result.clear();
	for(UINT n=0; n<samplerates.size(); n++)
	{
		PaStreamParameters StreamParameters;
		MemsetZero(StreamParameters);
		StreamParameters.device = HostApiOutputIndexToGlobalDeviceIndex(nDevice, m_HostApi);
		if(StreamParameters.device == -1)
		{
			result.assign(samplerates.size(), false);
			return false;
		}
		StreamParameters.channelCount = 2;
		StreamParameters.sampleFormat = paInt16;
		StreamParameters.suggestedLatency = 0.0;
		StreamParameters.hostApiSpecificStreamInfo = NULL;
		if((m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI)) && (m_Settings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE))
		{
			MemsetZero(m_WasapiStreamInfo);
			m_WasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
			m_WasapiStreamInfo.hostApiType = paWASAPI;
			m_WasapiStreamInfo.version = 1;
			m_WasapiStreamInfo.flags = paWinWasapiExclusive;
			m_StreamParameters.hostApiSpecificStreamInfo = &m_WasapiStreamInfo;
		}
		result.push_back(Pa_IsFormatSupported(NULL, &StreamParameters, samplerates[n]) == paFormatIsSupported);
	}
	return true;
}


int CPortaudioDevice::StreamCallback(
	const void *input, void *output,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags
	)
//-----------------------------------------
{
	UNREFERENCED_PARAMETER(input);
	UNREFERENCED_PARAMETER(statusFlags);
	if(!output) return paAbort;
	m_CurrentRealLatencyMS = static_cast<float>( timeInfo->outputBufferDacTime - timeInfo->currentTime ) * 1000.0f;
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


int CPortaudioDevice::HostApiToSndDevType(PaHostApiIndex hostapi)
//---------------------------------------------------------------
{
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI)) return SNDDEV_PORTAUDIO_WASAPI;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paWDMKS)) return SNDDEV_PORTAUDIO_WDMKS;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paMME)) return SNDDEV_PORTAUDIO_WMME;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paDirectSound)) return SNDDEV_PORTAUDIO_DS;
	if(hostapi == Pa_HostApiTypeIdToHostApiIndex(paASIO)) return SNDDEV_PORTAUDIO_ASIO;
	return -1;
}


PaHostApiIndex CPortaudioDevice::SndDevTypeToHostApi(int snddevtype)
//------------------------------------------------------------------
{
	if(snddevtype == SNDDEV_PORTAUDIO_WASAPI) return Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
	if(snddevtype == SNDDEV_PORTAUDIO_WDMKS) return Pa_HostApiTypeIdToHostApiIndex(paWDMKS);
	if(snddevtype == SNDDEV_PORTAUDIO_WMME) return Pa_HostApiTypeIdToHostApiIndex(paMME);
	if(snddevtype == SNDDEV_PORTAUDIO_DS) return Pa_HostApiTypeIdToHostApiIndex(paDirectSound);
	if(snddevtype == SNDDEV_PORTAUDIO_ASIO) return Pa_HostApiTypeIdToHostApiIndex(paASIO);
	return paInDevelopment;
}


BOOL CPortaudioDevice::EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize, PaHostApiIndex hostapi)
//-------------------------------------------------------------------------------------------------------------
{
	memset(pszDescription, 0, cbSize);
	PaDeviceIndex dev = HostApiOutputIndexToGlobalDeviceIndex(nIndex, hostapi);
	if(dev == -1)
		return false;
	if(!Pa_GetDeviceInfo(dev))
		return false;
	_snprintf(pszDescription, cbSize, "%s - %s%s (portaudio)",
		Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->name,
		Pa_GetDeviceInfo(dev)->name,
		Pa_GetHostApiInfo(Pa_GetDeviceInfo(dev)->hostApi)->defaultOutputDevice == (PaDeviceIndex)dev ? " (Default)" : ""
		);
	return true;
}


static bool g_PortaudioInitialized = false;


void SndDevPortaudioInitialize()
//------------------------------
{
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


#endif

