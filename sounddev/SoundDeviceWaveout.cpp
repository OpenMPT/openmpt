/*
 * SoundDeviceWaveout.cpp
 * ----------------------
 * Purpose: Waveout sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"
#include "SoundDeviceThread.h"

#include "SoundDeviceWaveout.h"

#include "../common/misc_util.h"

#include <mmreg.h>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {

	
bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat, const SoundDevice::Settings &m_Settings);


///////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut Device
//


static const std::size_t WAVEOUT_MINBUFFERS = 3;
static const std::size_t WAVEOUT_MAXBUFFERS = 256;
static const std::size_t WAVEOUT_MINBUFFERSIZE = 1024;
static const std::size_t WAVEOUT_MAXBUFFERSIZE = 65536;


CWaveDevice::CWaveDevice(SoundDevice::Info info)
//----------------------------------------------
	: CSoundDeviceWithThread(info)
{
	MPT_TRACE();
	m_ThreadWakeupEvent;
	m_hWaveOut = NULL;
	m_nWaveBufferSize = 0;
	m_JustStarted = false;
	m_nPreparedHeaders = 0;
}


CWaveDevice::~CWaveDevice()
//-------------------------
{
	MPT_TRACE();
	Close();
}


SoundDevice::Caps CWaveDevice::InternalGetDeviceCaps()
//--------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Caps caps;
	caps.Available = true;
	caps.CanUpdateInterval = true;
	caps.CanSampleFormat = true;
	caps.CanExclusiveMode = (GetDeviceIndex() > 0); // no direct mode for WAVE_MAPPER, makes no sense there
	caps.CanBoostThreadPriority = true;
	caps.CanKeepDeviceRunning = false;
	caps.CanUseHardwareTiming = false;
	caps.CanChannelMapping = false;
	caps.CanDriverPanel = false;
	caps.HasInternalDither = false;
	caps.ExclusiveModeDescription = MPT_USTRING("Use direct mode");
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
	return caps;
}


SoundDevice::DynamicCaps CWaveDevice::GetDeviceDynamicCaps(const std::vector<uint32> & /*baseSampleRates*/ )
//--------------------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::DynamicCaps caps;
	WAVEOUTCAPSW woc;
	MemsetZero(woc);
	if(GetDeviceIndex() > 0)
	{
		if(waveOutGetDevCapsW(GetDeviceIndex() - 1, &woc, sizeof(woc)) == MMSYSERR_NOERROR)
		{
			if(woc.dwFormats & (WAVE_FORMAT_96M08 | WAVE_FORMAT_96M16	| WAVE_FORMAT_96S08 | WAVE_FORMAT_96S16))
			{
				caps.supportedExclusiveSampleRates.push_back(96000);
			}
			if(woc.dwFormats & (WAVE_FORMAT_48M08 | WAVE_FORMAT_48M16	| WAVE_FORMAT_48S08 | WAVE_FORMAT_48S16))
			{
				caps.supportedExclusiveSampleRates.push_back(48000);
			}
			if(woc.dwFormats & (WAVE_FORMAT_4M08 | WAVE_FORMAT_4M16	| WAVE_FORMAT_4S08 | WAVE_FORMAT_4S16))
			{
				caps.supportedExclusiveSampleRates.push_back(44100);
			}
			if(woc.dwFormats & (WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16	| WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16))
			{
				caps.supportedExclusiveSampleRates.push_back(22050);
			}
			if(woc.dwFormats & (WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16	| WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16))
			{
				caps.supportedExclusiveSampleRates.push_back(11025);
			}
		}
	}
	return caps;
}


bool CWaveDevice::InternalOpen()
//------------------------------
{
	MPT_TRACE();
	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext, m_Settings))
	{
		return false;
	}
	WAVEFORMATEX *pwfx = &wfext.Format;
	UINT_PTR nWaveDev = GetDeviceIndex();
	nWaveDev = (nWaveDev > 0) ? nWaveDev - 1 : WAVE_MAPPER;
	m_ThreadWakeupEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(m_ThreadWakeupEvent == INVALID_HANDLE_VALUE)
	{
		InternalClose();
		return false;
	}
	m_hWaveOut = NULL;
	if(waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD_PTR)WaveOutCallBack, (DWORD_PTR)this, CALLBACK_FUNCTION | (m_Settings.ExclusiveMode ? WAVE_FORMAT_DIRECT : 0)) != MMSYSERR_NOERROR)
	{
		InternalClose();
		return false;
	}
	if(waveOutPause(m_hWaveOut) != MMSYSERR_NOERROR)
	{
		InternalClose();
		return false;
	}
	m_nWaveBufferSize = Util::Round<int32>(m_Settings.UpdateInterval * pwfx->nAvgBytesPerSec);
	m_nWaveBufferSize = Clamp(m_nWaveBufferSize, WAVEOUT_MINBUFFERSIZE, WAVEOUT_MAXBUFFERSIZE);
	m_nWaveBufferSize = ((m_nWaveBufferSize + pwfx->nBlockAlign - 1) / pwfx->nBlockAlign) * pwfx->nBlockAlign;
	std::size_t numBuffers = Util::Round<int32>(m_Settings.Latency * pwfx->nAvgBytesPerSec / m_nWaveBufferSize);
	numBuffers = Clamp(numBuffers, WAVEOUT_MINBUFFERS, WAVEOUT_MAXBUFFERS);
	m_nPreparedHeaders = 0;
	m_WaveBuffers.resize(numBuffers);
	m_WaveBuffersData.resize(numBuffers);
	for(std::size_t buf = 0; buf < numBuffers; ++buf)
	{
		MemsetZero(m_WaveBuffers[buf]);
		m_WaveBuffersData[buf].resize(m_nWaveBufferSize);
		m_WaveBuffers[buf].dwFlags = 0;
		m_WaveBuffers[buf].lpData = &m_WaveBuffersData[buf][0];
		m_WaveBuffers[buf].dwBufferLength = m_nWaveBufferSize;
		if(waveOutPrepareHeader(m_hWaveOut, &m_WaveBuffers[buf], sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
			break;
		}
		m_WaveBuffers[buf].dwFlags |= WHDR_DONE;
		m_nPreparedHeaders++;
	}
	if(!m_nPreparedHeaders)
	{
		InternalClose();
		return false;
	}
	m_nBuffersPending = 0;
	m_nWriteBuffer = 0;
	SetWakeupEvent(m_ThreadWakeupEvent);
	SetWakeupInterval(m_nWaveBufferSize * 1.0 / m_Settings.GetBytesPerSecond());
	m_Flags.NeedsClippedFloat = mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista);
	return true;
}


bool CWaveDevice::InternalClose()
//-------------------------------
{
	MPT_TRACE();
	if(m_hWaveOut)
	{
		waveOutReset(m_hWaveOut);
		m_JustStarted = false;
		InterlockedExchange(&m_nBuffersPending, 0);
		m_nWriteBuffer = 0;
		while(m_nPreparedHeaders > 0)
		{
			m_nPreparedHeaders--;
			waveOutUnprepareHeader(m_hWaveOut, &m_WaveBuffers[m_nPreparedHeaders], sizeof(WAVEHDR));
		}
		MMRESULT err = waveOutClose(m_hWaveOut);
		MPT_ASSERT_ALWAYS(err == MMSYSERR_NOERROR);
		m_hWaveOut = NULL;
	}
	if(m_ThreadWakeupEvent)
	{
		CloseHandle(m_ThreadWakeupEvent);
		m_ThreadWakeupEvent = NULL;
	}
	return true;
}


void CWaveDevice::StartFromSoundThread()
//--------------------------------------
{
	MPT_TRACE();
	if(m_hWaveOut)
	{
		m_JustStarted = true;
		// Actual starting is done in InternalFillAudioBuffer to avoid crackling with tiny buffers.
	}
}


void CWaveDevice::StopFromSoundThread()
//-------------------------------------
{
	MPT_TRACE();
	if(m_hWaveOut)
	{
		waveOutPause(m_hWaveOut);
		m_JustStarted = false;
	}
}


void CWaveDevice::InternalFillAudioBuffer()
//-----------------------------------------
{
	MPT_TRACE();
	if(!m_hWaveOut)
	{
		return;
	}
	
	const std::size_t bytesPerFrame = m_Settings.GetBytesPerFrame();

	ULONG oldBuffersPending = InterlockedExchangeAdd(&m_nBuffersPending, 0); // read
	ULONG nLatency = oldBuffersPending * m_nWaveBufferSize;

	ULONG nBytesWritten = 0;
	while(oldBuffersPending < m_nPreparedHeaders)
	{
		SourceAudioPreRead(m_nWaveBufferSize / bytesPerFrame);
		SourceAudioRead(m_WaveBuffers[m_nWriteBuffer].lpData, m_nWaveBufferSize / bytesPerFrame);
		nLatency += m_nWaveBufferSize;
		nBytesWritten += m_nWaveBufferSize;
		m_WaveBuffers[m_nWriteBuffer].dwBufferLength = m_nWaveBufferSize;
		InterlockedIncrement(&m_nBuffersPending);
		oldBuffersPending++; // increment separately to avoid looping without leaving at all when rendering takes more than 100% CPU
		waveOutWrite(m_hWaveOut, &m_WaveBuffers[m_nWriteBuffer], sizeof(WAVEHDR));
		m_nWriteBuffer++;
		m_nWriteBuffer %= m_nPreparedHeaders;
		SourceAudioDone(m_nWaveBufferSize / bytesPerFrame, nLatency / bytesPerFrame);
	}

	if(m_JustStarted)
	{
		// Fill the buffers completely before starting the stream.
		// This avoids buffer underruns which result in audible crackling with small buffers.
		m_JustStarted = false;
		waveOutRestart(m_hWaveOut);
	}

}


int64 CWaveDevice::InternalGetStreamPositionFrames() const
//---------------------------------------------------------
{
	MMTIME mmtime;
	MemsetZero(mmtime);
	mmtime.wType = TIME_SAMPLES;
	if(waveOutGetPosition(m_hWaveOut, &mmtime, sizeof(mmtime)) != MMSYSERR_NOERROR) return 0;
	switch(mmtime.wType)
	{
		case TIME_BYTES:   return mmtime.u.cb / m_Settings.GetBytesPerFrame(); break;
		case TIME_MS:      return mmtime.u.ms * m_Settings.GetBytesPerSecond() / (1000 * m_Settings.GetBytesPerFrame()); break;
		case TIME_SAMPLES: return mmtime.u.sample; break;
		default: return 0; break;
	}
}


void CWaveDevice::HandleWaveoutDone()
//-----------------------------------
{
	MPT_TRACE();
	InterlockedDecrement(&m_nBuffersPending);
	SetEvent(m_ThreadWakeupEvent);
}


void CWaveDevice::WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR)
//--------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	if((uMsg == MM_WOM_DONE) && (dwUser))
	{
		CWaveDevice *that = (CWaveDevice *)dwUser;
		that->HandleWaveoutDone();
	}
}


SoundDevice::BufferAttributes CWaveDevice::InternalGetEffectiveBufferAttributes() const
//-------------------------------------------------------------------------------------
{
	SoundDevice::BufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_nWaveBufferSize * m_nPreparedHeaders * 1.0 / m_Settings.GetBytesPerSecond();
	bufferAttributes.UpdateInterval = m_nWaveBufferSize * 1.0 / m_Settings.GetBytesPerSecond();
	bufferAttributes.NumBuffers = m_nPreparedHeaders;
	return bufferAttributes;
}


SoundDevice::Statistics CWaveDevice::GetStatistics() const
//--------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	result.InstantaneousLatency = InterlockedExchangeAdd(&m_nBuffersPending, 0) * m_nWaveBufferSize * 1.0 / m_Settings.GetBytesPerSecond();
	result.LastUpdateInterval = GetLastUpdateInterval();
	result.text = mpt::ustring();
	return result;
}


std::vector<SoundDevice::Info> CWaveDevice::EnumerateDevices()
//----------------------------------------------------------
{
	MPT_TRACE();
	std::vector<SoundDevice::Info> devices;
	UINT numDevs = waveOutGetNumDevs();
	for(UINT index = 0; index <= numDevs; ++index)
	{
		if(!SoundDevice::IndexIsValid(index))
		{
			break;
		}
		SoundDevice::Info info;
		info.id = SoundDevice::ID(TypeWAVEOUT, static_cast<SoundDevice::Index>(index));
		info.apiName = SoundDevice::TypeToString(TypeWAVEOUT);
		WAVEOUTCAPSW woc;
		MemsetZero(woc);
		if(index == 0)
		{
			if(waveOutGetDevCapsW(WAVE_MAPPER, &woc, sizeof(woc)) == MMSYSERR_NOERROR)
			{
				info.name = mpt::ToUnicode(woc.szPname);
			} else
			{
				info.name = MPT_USTRING("Auto (Wave Mapper)");
			}
			info.isDefault = true;
		} else
		{
			if(waveOutGetDevCapsW(index-1, &woc, sizeof(woc)) == MMSYSERR_NOERROR)
			{
				info.name = mpt::ToUnicode(woc.szPname);
			}
		}
		devices.push_back(info);
	}
	return devices;
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
