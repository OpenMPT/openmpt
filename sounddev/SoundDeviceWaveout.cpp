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
#include "SoundDevices.h"

#include "SoundDeviceWaveout.h"

#include "../common/misc_util.h"


///////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut Device
//


#define WAVEOUT_MAXBUFFERS           256
#define WAVEOUT_MINBUFFERSIZE        1024
#define WAVEOUT_MAXBUFFERSIZE        65536


CWaveDevice::CWaveDevice(SoundDeviceID id, const std::wstring &internalID)
//------------------------------------------------------------------------
	: CSoundDeviceWithThread(id, internalID)
{
	m_hWaveOut = NULL;
	m_nWaveBufferSize = 0;
	m_JustStarted = false;
	m_nPreparedHeaders = 0;
}


CWaveDevice::~CWaveDevice()
//-------------------------
{
	Close();
}


bool CWaveDevice::InternalOpen()
//------------------------------
{
	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext)) return false;
	WAVEFORMATEX *pwfx = &wfext.Format;

	LONG nWaveDev;

	if (m_hWaveOut) Close();
	nWaveDev = GetDeviceIndex();
	nWaveDev = (nWaveDev) ? nWaveDev-1 : WAVE_MAPPER;
	if (waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD_PTR)WaveOutCallBack, (DWORD_PTR)this, CALLBACK_FUNCTION))
	{
		sndPlaySound(NULL, 0);
		LONG err = waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD_PTR)WaveOutCallBack, (DWORD_PTR)this, CALLBACK_FUNCTION);
		if (err) return false;
	}
	m_nWaveBufferSize = (m_Settings.UpdateIntervalMS * pwfx->nAvgBytesPerSec) / 1000;
	m_nWaveBufferSize = (m_nWaveBufferSize + 7) & ~7;
	if (m_nWaveBufferSize < WAVEOUT_MINBUFFERSIZE) m_nWaveBufferSize = WAVEOUT_MINBUFFERSIZE;
	if (m_nWaveBufferSize > WAVEOUT_MAXBUFFERSIZE) m_nWaveBufferSize = WAVEOUT_MAXBUFFERSIZE;
	ULONG NumBuffers = m_Settings.LatencyMS * pwfx->nAvgBytesPerSec / ( m_nWaveBufferSize * 1000 );
	NumBuffers = CLAMP(NumBuffers, 3, WAVEOUT_MAXBUFFERS);
	m_nPreparedHeaders = 0;
	m_WaveBuffers.resize(NumBuffers);
	m_WaveBuffersData.resize(NumBuffers);
	for(UINT iBuf=0; iBuf<NumBuffers; iBuf++)
	{
		MemsetZero(m_WaveBuffers[iBuf]);
		m_WaveBuffersData[iBuf].resize(m_nWaveBufferSize);
		m_WaveBuffers[iBuf].dwFlags = WHDR_DONE;
		m_WaveBuffers[iBuf].lpData = &m_WaveBuffersData[iBuf][0];
		m_WaveBuffers[iBuf].dwBufferLength = m_nWaveBufferSize;
		if (waveOutPrepareHeader(m_hWaveOut, &m_WaveBuffers[iBuf], sizeof(WAVEHDR)) != 0)
		{
			break;
		}
		m_nPreparedHeaders++;
	}
	if (!m_nPreparedHeaders)
	{
		Close();
		return false;
	}
	m_RealLatencyMS = m_nWaveBufferSize * m_nPreparedHeaders * 1000.0f / m_Settings.GetBytesPerSecond();
	m_RealUpdateIntervalMS = m_nWaveBufferSize * 1000.0f / m_Settings.GetBytesPerSecond();
	m_nBuffersPending = 0;
	m_nWriteBuffer = 0;
	return true;
}


bool CWaveDevice::InternalClose()
//-------------------------------
{
	if(m_hWaveOut)
	{
		waveOutReset(m_hWaveOut);
		m_JustStarted = false;
		InterlockedExchange(&m_nBuffersPending, 0);
		m_nWriteBuffer = 0;
		while (m_nPreparedHeaders > 0)
		{
			m_nPreparedHeaders--;
			waveOutUnprepareHeader(m_hWaveOut, &m_WaveBuffers[m_nPreparedHeaders], sizeof(WAVEHDR));
		}
		MMRESULT err = waveOutClose(m_hWaveOut);
		ALWAYS_ASSERT(err == MMSYSERR_NOERROR);
		m_hWaveOut = NULL;
	}
	return true;
}


void CWaveDevice::StartFromSoundThread()
//--------------------------------------
{
	if(m_hWaveOut)
	{
		m_JustStarted = true;
		// Actual starting is done in FillAudioBuffer to avoid crackling with tiny buffers.
	}
}


void CWaveDevice::StopFromSoundThread()
//-------------------------------------
{
	if(m_hWaveOut)
	{
		waveOutPause(m_hWaveOut);
		m_JustStarted = false;
	}
}


void CWaveDevice::FillAudioBuffer()
//---------------------------------
{
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
		SourceAudioRead(m_WaveBuffers[m_nWriteBuffer].lpData, m_nWaveBufferSize/bytesPerFrame);
		nLatency += m_nWaveBufferSize;
		nBytesWritten += m_nWaveBufferSize;
		m_WaveBuffers[m_nWriteBuffer].dwBufferLength = m_nWaveBufferSize;
		InterlockedIncrement(&m_nBuffersPending);
		oldBuffersPending++; // increment separately to avoid looping without leaving at all when rendering takes more than 100% CPU
		waveOutWrite(m_hWaveOut, &m_WaveBuffers[m_nWriteBuffer], sizeof(WAVEHDR));
		m_nWriteBuffer++;
		m_nWriteBuffer %= m_nPreparedHeaders;
		SourceAudioDone(m_nWaveBufferSize/bytesPerFrame, nLatency/bytesPerFrame);
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


void CWaveDevice::WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR)
//--------------------------------------------------------------------------------------------
{
	if ((uMsg == MM_WOM_DONE) && (dwUser))
	{
		CWaveDevice *that = (CWaveDevice *)dwUser;
		InterlockedDecrement(&that->m_nBuffersPending);
	}
}


std::vector<SoundDeviceInfo> CWaveDevice::EnumerateDevices()
//----------------------------------------------------------
{
	std::vector<SoundDeviceInfo> devices;
	UINT numDevs = waveOutGetNumDevs();
	for(UINT index = 0; index <= numDevs; ++index)
	{
		if(!SoundDeviceIndexIsValid(index))
		{
			break;
		}
		SoundDeviceInfo info;
		info.id = SoundDeviceID(SNDDEV_WAVEOUT, static_cast<SoundDeviceIndex>(index));
		info.apiName = L"WaveOut";
		if(index == 0)
		{
			info.name = L"Auto (Wave Mapper)";
		} else
		{
			WAVEOUTCAPSW woc;
			MemsetZero(woc);
			waveOutGetDevCapsW(index-1, &woc, sizeof(woc));
			info.name = woc.szPname;
		}
		devices.push_back(info);
	}
	return devices;
}
