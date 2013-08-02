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
#include "SoundDevices.h"
#include "../common/misc_util.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Reporting.h"
#endif
#include "../common/StringFixer.h"

// DEBUG:
#include "../common/AudioCriticalSection.h"


///////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice base class
//

ISoundDevice::ISoundDevice()
//--------------------------
{
	m_Source = nullptr;

	m_RealLatencyMS = static_cast<float>(m_Setttings.LatencyMS);
	m_RealUpdateIntervalMS = static_cast<float>(m_Setttings.UpdateIntervalMS);

	m_IsPlaying = false;
}


ISoundDevice::~ISoundDevice()
//---------------------------
{
	return;
}


bool ISoundDevice::FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat)
//---------------------------------------------------------------------------
{
	MemsetZero(WaveFormat);
	UINT bytespersample = (m_Setttings.BitsPerSample/8) * m_Setttings.Channels;
	WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.Format.nChannels = (WORD)m_Setttings.Channels;
	WaveFormat.Format.nSamplesPerSec = m_Setttings.Samplerate;
	WaveFormat.Format.nAvgBytesPerSec = m_Setttings.Samplerate * bytespersample;
	WaveFormat.Format.nBlockAlign = (WORD)bytespersample;
	WaveFormat.Format.wBitsPerSample = (WORD)m_Setttings.BitsPerSample;
	WaveFormat.Format.cbSize = 0;
	if((WaveFormat.Format.wBitsPerSample > 16) || (WaveFormat.Format.nChannels > 2))
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
		WaveFormat.SubFormat = guid_MEDIASUBTYPE_PCM;
	}
	return true;
}


bool ISoundDevice::Open(UINT device, const SoundDeviceSettings &settings)
//-----------------------------------------------------------------------
{
	m_Setttings = settings;
	if(m_Setttings.LatencyMS < SNDDEV_MINLATENCY_MS) m_Setttings.LatencyMS = SNDDEV_MINLATENCY_MS;
	if(m_Setttings.LatencyMS > SNDDEV_MAXLATENCY_MS) m_Setttings.LatencyMS = SNDDEV_MAXLATENCY_MS;
	if(m_Setttings.UpdateIntervalMS < SNDDEV_MINUPDATEINTERVAL_MS) m_Setttings.UpdateIntervalMS = SNDDEV_MINUPDATEINTERVAL_MS;
	if(m_Setttings.UpdateIntervalMS > SNDDEV_MAXUPDATEINTERVAL_MS) m_Setttings.UpdateIntervalMS = SNDDEV_MAXUPDATEINTERVAL_MS;
	m_RealLatencyMS = static_cast<float>(m_Setttings.LatencyMS);
	m_RealUpdateIntervalMS = static_cast<float>(m_Setttings.UpdateIntervalMS);
	return InternalOpen(device);
}


bool ISoundDevice::Close()
//------------------------
{
	return InternalClose();
}


void ISoundDevice::SourceFillAudioBufferLocked()
//----------------------------------------------
{
	if(m_Source)
	{
		m_Source->FillAudioBufferLocked(*this);
	}
}


void ISoundDevice::SourceAudioRead(void* pData, ULONG NumSamples)
//---------------------------------------------------------------
{
	m_Source->AudioRead(pData, NumSamples);
}


void ISoundDevice::SourceAudioDone(ULONG NumSamples, ULONG SamplesLatency)
//------------------------------------------------------------------------
{
	if(HasGetStreamPosition())
	{
		m_Source->AudioDone(NumSamples);
	} else
	{
		m_Source->AudioDone(NumSamples, SamplesLatency);
	}
}


void ISoundDevice::Start()
//------------------------
{
	if(!IsOpen()) return; 
	if(!IsPlaying())
	{
		InternalStart();
		m_IsPlaying = true;
	}
}


void ISoundDevice::Stop()
//-----------------------
{
	if(!IsOpen()) return;
	if(IsPlaying())
	{
		InternalStop();
		m_IsPlaying = false;
	}
}


void ISoundDevice::Reset()
//------------------------
{
	if(!IsOpen()) return;
	Stop();
	InternalReset();
}


CAudioThread::CAudioThread(CSoundDeviceWithThread &SoundDevice) : m_SoundDevice(SoundDevice)
//-----------------------------------------------------------------------------------
{

	OSVERSIONINFO versioninfo;
	MemsetZero(versioninfo);
	versioninfo.dwOSVersionInfoSize = sizeof(versioninfo);
	GetVersionEx(&versioninfo);
	m_HasXP = versioninfo.dwMajorVersion >= 6 || (versioninfo.dwMajorVersion == 5 && versioninfo.dwMinorVersion >= 1);
	m_HasVista = versioninfo.dwMajorVersion >= 6;

	m_hKernel32DLL = NULL;
	m_hAvRtDLL = NULL;

	pCreateWaitableTimer = nullptr;
	pSetWaitableTimer = nullptr;
	pCancelWaitableTimer = nullptr;
	m_hKernel32DLL = LoadLibrary("kernel32.dll");
	#if _WIN32_WINNT >= _WIN32_WINNT_WINXP
		m_HasXP = true;
		pCreateWaitableTimer = &CreateWaitableTimer;
		pSetWaitableTimer = &SetWaitableTimer;
		pCancelWaitableTimer = &CancelWaitableTimer;
	#else
		if(m_HasXP && m_hKernel32DLL)
		{
			pCreateWaitableTimer = (FCreateWaitableTimer)GetProcAddress(m_hKernel32DLL, "CreateWaitableTimerA");
			pSetWaitableTimer = (FSetWaitableTimer)GetProcAddress(m_hKernel32DLL, "SetWaitableTimer");
			pCancelWaitableTimer = (FCancelWaitableTimer)GetProcAddress(m_hKernel32DLL, "CancelWaitableTimer");
			if(!pCreateWaitableTimer || !pSetWaitableTimer || !pCancelWaitableTimer)
			{
				m_HasXP = false;
			}
		}
	#endif

	pAvSetMmThreadCharacteristics = nullptr;
	pAvRevertMmThreadCharacteristics = nullptr;
	if(m_HasVista)
	{
		m_hAvRtDLL = LoadLibrary("avrt.dll");
		if(m_hAvRtDLL)
		{
			pAvSetMmThreadCharacteristics = (FAvSetMmThreadCharacteristics)GetProcAddress(m_hAvRtDLL, "AvSetMmThreadCharacteristicsA");
			pAvRevertMmThreadCharacteristics = (FAvRevertMmThreadCharacteristics)GetProcAddress(m_hAvRtDLL, "AvRevertMmThreadCharacteristics");
		}
		if(!pAvSetMmThreadCharacteristics || !pAvRevertMmThreadCharacteristics)
		{
			m_HasVista = false;
		}
	}

	m_hPlayThread = NULL;
	m_dwPlayThreadId = 0;
	m_hAudioWakeUp = NULL;
	m_hAudioThreadTerminateRequest = NULL;
	m_hAudioThreadGoneIdle = NULL;
	m_AudioThreadActive = 0;
	m_hAudioWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hAudioThreadTerminateRequest = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hAudioThreadGoneIdle = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hPlayThread = CreateThread(NULL, 0, AudioThreadWrapper, (LPVOID)this, 0, &m_dwPlayThreadId);
}


CAudioThread::~CAudioThread()
//---------------------------
{
	if(m_hPlayThread != NULL)
	{
		SetEvent(m_hAudioThreadTerminateRequest);
		WaitForSingleObject(m_hPlayThread, INFINITE);
		m_dwPlayThreadId = 0;
		m_hPlayThread = NULL;
	}
	if(m_hAudioThreadTerminateRequest)
	{
		CloseHandle(m_hAudioThreadTerminateRequest);
		m_hAudioThreadTerminateRequest = 0;
	}
	if(m_hAudioThreadGoneIdle != NULL)
	{
		CloseHandle(m_hAudioThreadGoneIdle);
		m_hAudioThreadGoneIdle = 0;
	}
	if(m_hAudioWakeUp != NULL)
	{
		CloseHandle(m_hAudioWakeUp);
		m_hAudioWakeUp = NULL;
	}

	pAvRevertMmThreadCharacteristics = nullptr;
	pAvSetMmThreadCharacteristics = nullptr;
	pCreateWaitableTimer = nullptr;
	pSetWaitableTimer = nullptr;
	pCancelWaitableTimer = nullptr;

	if(m_hAvRtDLL)
	{
		FreeLibrary(m_hAvRtDLL);
		m_hAvRtDLL = NULL;
	}

	if(m_hKernel32DLL)
	{
		FreeLibrary(m_hKernel32DLL);
		m_hKernel32DLL = NULL;
	}

}


class CPriorityBooster
{
private:
	CAudioThread &self;
	bool m_BoostPriority;
	DWORD task_idx;
	HANDLE hTask;
public:

	CPriorityBooster(CAudioThread &self_, bool boostPriority) : self(self_), m_BoostPriority(boostPriority)
	//-----------------------------------------------------------------------------------------------------
	{
		#ifdef _DEBUG
			m_BoostPriority = false;
		#endif

		task_idx = 0;
		hTask = NULL;

		if(m_BoostPriority)
		{
			if(self.m_HasVista)
			{
				hTask = self.pAvSetMmThreadCharacteristics("Pro Audio", &task_idx);
			} else
			{
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
			}
		}

	}

	CPriorityBooster::~CPriorityBooster()
	//-----------------------------------
	{

		if(m_BoostPriority)
		{
			if(self.m_HasVista)
			{
				self.pAvRevertMmThreadCharacteristics(hTask);
				hTask = NULL;
				task_idx = 0;
			} else
			{
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
			}
		}

	}

};


class CPeriodicWaker
{
private:
	CAudioThread &self;

	double sleepSeconds;
	long sleepMilliseconds;
	int64 sleep100Nanoseconds;

	bool period_noxp_set;
	bool periodic_xp_timer;

	HANDLE sleepEvent;

public:

	CPeriodicWaker(CAudioThread &self_, double sleepSeconds_) : self(self_), sleepSeconds(sleepSeconds_)
	//--------------------------------------------------------------------------------------------------
	{

		sleepMilliseconds = static_cast<long>(sleepSeconds * 1000.0);
		sleep100Nanoseconds = static_cast<int64>(sleepSeconds * 10000000.0);
		if(sleepMilliseconds < 1) sleepMilliseconds = 1;
		if(sleep100Nanoseconds < 1) sleep100Nanoseconds = 1;

		period_noxp_set = false;
		periodic_xp_timer = (sleep100Nanoseconds >= 10000); // can be represented as a millisecond period, otherwise use non-periodic timers which allow higher precision but might me slower because we have to set them again in each period

		sleepEvent = NULL;

		if(self.m_HasXP)
		{
			if(periodic_xp_timer)
			{
				sleepEvent = self.pCreateWaitableTimer(NULL, FALSE, NULL);
				LARGE_INTEGER dueTime;
				dueTime.QuadPart = 0 - sleep100Nanoseconds; // negative time means relative
				self.pSetWaitableTimer(sleepEvent, &dueTime, sleepMilliseconds, NULL, NULL, FALSE);
			} else
			{
				sleepEvent = self.pCreateWaitableTimer(NULL, TRUE, NULL);
			}
		} else
		{
			sleepEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			period_noxp_set = (timeBeginPeriod(1) == TIMERR_NOERROR); // increase resolution of multimedia timer
		}

	}

	long GetSleepMilliseconds() const
	//-------------------------------
	{
		return sleepMilliseconds;
	}

	HANDLE GetWakeupEvent() const
	//---------------------------
	{
		return sleepEvent;
	}

	void Retrigger()
	//--------------
	{
		if(self.m_HasXP)
		{
			if(!periodic_xp_timer)
			{
				LARGE_INTEGER dueTime;
				dueTime.QuadPart = 0 - sleep100Nanoseconds; // negative time means relative
				self.pSetWaitableTimer(sleepEvent, &dueTime, 0, NULL, NULL, FALSE);
			}
		} else
		{
			timeSetEvent(sleepMilliseconds, 1, (LPTIMECALLBACK)sleepEvent, NULL, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
		}
	}

	CPeriodicWaker::~CPeriodicWaker()
	//-------------------------------
	{
		if(self.m_HasXP)
		{
			if(periodic_xp_timer)
			{
				self.pCancelWaitableTimer(sleepEvent);
			}
			CloseHandle(sleepEvent);
			sleepEvent = NULL;
		} else
		{
			if(period_noxp_set)
			{
				timeEndPeriod(1);
				period_noxp_set = false;
			}
			CloseHandle(sleepEvent);
			sleepEvent = NULL;
		}
	}

};


DWORD WINAPI CAudioThread::AudioThreadWrapper(LPVOID user)
{
	return ((CAudioThread*)user)->AudioThread();
}
DWORD CAudioThread::AudioThread()
//-------------------------------
{

	bool terminate = false;
	while(!terminate)
	{

		bool idle = true;
		while(!terminate && idle)
		{
			HANDLE waithandles[2] = {m_hAudioThreadTerminateRequest, m_hAudioWakeUp};
			SetEvent(m_hAudioThreadGoneIdle);
			switch(WaitForMultipleObjects(2, waithandles, FALSE, INFINITE))
			{
			case WAIT_OBJECT_0:
				terminate = true;
				break;
			case WAIT_OBJECT_0+1:
				idle = false;
				break;
			}
		}

		if(!terminate)
		{

			CPriorityBooster priorityBooster(*this, (m_SoundDevice.m_Setttings.fulCfgOptions & SNDDEV_OPTIONS_BOOSTTHREADPRIORITY)?true:false);
			CPeriodicWaker periodicWaker(*this, 0.001 * m_SoundDevice.GetRealUpdateIntervalMS());

			m_SoundDevice.StartFromSoundThread();

			while(!terminate && IsActive())
			{

				m_SoundDevice.FillAudioBufferLocked();

				periodicWaker.Retrigger();

				HANDLE waithandles[3] = {m_hAudioThreadTerminateRequest, m_hAudioWakeUp, periodicWaker.GetWakeupEvent()};
				switch(WaitForMultipleObjects(3, waithandles, FALSE, periodicWaker.GetSleepMilliseconds()))
				{
				case WAIT_OBJECT_0:
					terminate = true;
					break;
				}

			}

			m_SoundDevice.StopFromSoundThread();

		}

	}

	SetEvent(m_hAudioThreadGoneIdle);

	return 0;

}


void CAudioThread::Activate()
//---------------------------
{
	if(InterlockedExchangeAdd(&m_AudioThreadActive, 0))
	{
		ALWAYS_ASSERT(false);
		return;
	}
	ResetEvent(m_hAudioThreadGoneIdle);
	InterlockedExchange(&m_AudioThreadActive, 1);
	SetEvent(m_hAudioWakeUp);
}


void CAudioThread::Deactivate()
//-----------------------------
{
	if(!InterlockedExchangeAdd(&m_AudioThreadActive, 0))
	{
		ALWAYS_ASSERT(false);
		return;
	}
	InterlockedExchange(&m_AudioThreadActive, 0);
	WaitForSingleObject(m_hAudioThreadGoneIdle, INFINITE);
}


void CSoundDeviceWithThread::FillAudioBufferLocked()
//--------------------------------------------------
{
	SourceFillAudioBufferLocked();
}


void CSoundDeviceWithThread::InternalStart()
//------------------------------------------
{
	m_AudioThread.Activate();
}


void CSoundDeviceWithThread::InternalStop()
//-----------------------------------------
{
	m_AudioThread.Deactivate();
}


void CSoundDeviceWithThread::InternalReset()
//------------------------------------------
{
	ResetFromOutsideSoundThread();
}


///////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut Device
//

static UINT gnNumWaveDevs = 0;

CWaveDevice::CWaveDevice()
//------------------------
{
	m_hWaveOut = NULL;
	m_nWaveBufferSize = 0;
	m_nPreparedHeaders = 0;
	m_nBytesPerSec = 0;
	m_BytesPerSample = 0;
}


CWaveDevice::~CWaveDevice()
//-------------------------
{
	Reset();
	Close();
}


bool CWaveDevice::InternalOpen(UINT nDevice)
//------------------------------------------
{
	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext)) return false;
	WAVEFORMATEX *pwfx = &wfext.Format;

	LONG nWaveDev;

	if (m_hWaveOut) Close();
	nWaveDev = (nDevice) ? nDevice-1 : WAVE_MAPPER;
	if (waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD_PTR)WaveOutCallBack, (DWORD_PTR)this, CALLBACK_FUNCTION))
	{
		sndPlaySound(NULL, 0);
		LONG err = waveOutOpen(&m_hWaveOut, nWaveDev, pwfx, (DWORD_PTR)WaveOutCallBack, (DWORD_PTR)this, CALLBACK_FUNCTION);
		if (err) return false;
	}
	m_nBytesPerSec = pwfx->nAvgBytesPerSec;
	m_BytesPerSample = (pwfx->wBitsPerSample/8) * pwfx->nChannels;
	m_nWaveBufferSize = (m_Setttings.UpdateIntervalMS * pwfx->nAvgBytesPerSec) / 1000;
	m_nWaveBufferSize = (m_nWaveBufferSize + 7) & ~7;
	if (m_nWaveBufferSize < WAVEOUT_MINBUFFERSIZE) m_nWaveBufferSize = WAVEOUT_MINBUFFERSIZE;
	if (m_nWaveBufferSize > WAVEOUT_MAXBUFFERSIZE) m_nWaveBufferSize = WAVEOUT_MAXBUFFERSIZE;
	ULONG NumBuffers = m_Setttings.LatencyMS * pwfx->nAvgBytesPerSec / ( m_nWaveBufferSize * 1000 );
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
	m_RealLatencyMS = m_nWaveBufferSize * m_nPreparedHeaders * 1000.0f / m_nBytesPerSec;
	m_RealUpdateIntervalMS = m_nWaveBufferSize * 1000.0f / m_nBytesPerSec;
	m_nBuffersPending = 0;
	m_nWriteBuffer = 0;
	return true;
}

bool CWaveDevice::InternalClose()
//-------------------------------
{
	Reset();
	if (m_hWaveOut)
	{
		ResetFromOutsideSoundThread(); // always reset so that waveOutClose does not fail if we did only P->Stop() (meaning waveOutPause()) before
		while (m_nPreparedHeaders > 0)
		{
			m_nPreparedHeaders--;
			waveOutUnprepareHeader(m_hWaveOut, &m_WaveBuffers[m_nPreparedHeaders], sizeof(WAVEHDR));
		}
		MMRESULT err = waveOutClose(m_hWaveOut);
		ALWAYS_ASSERT(err == MMSYSERR_NOERROR);
		m_hWaveOut = NULL;
		Sleep(1); // Linux WINE-friendly
	}
	return true;
}


void CWaveDevice::StartFromSoundThread()
//--------------------------------------
{
	if(m_hWaveOut)
	{
		waveOutRestart(m_hWaveOut);
	}
}


void CWaveDevice::StopFromSoundThread()
//-------------------------------------
{
	if(m_hWaveOut)
	{
		waveOutPause(m_hWaveOut);
	}
}


void CWaveDevice::ResetFromOutsideSoundThread()
//---------------------------------------------
{
	if(m_hWaveOut)
	{
		waveOutReset(m_hWaveOut);
	}
	InterlockedExchange(&m_nBuffersPending, 0);
	m_nWriteBuffer = 0;
}


void CWaveDevice::FillAudioBuffer()
//---------------------------------
{
	ULONG nBytesWritten;
	ULONG nLatency;
	LONG oldBuffersPending;
	if (!m_hWaveOut) return;
	nBytesWritten = 0;
	oldBuffersPending = InterlockedExchangeAdd(&m_nBuffersPending, 0); // read
	nLatency = oldBuffersPending * m_nWaveBufferSize;

	bool wasempty = false;
	if(oldBuffersPending == 0) wasempty = true;
	// When there were no pending buffers at all, pause the output, fill the buffers completely and then restart the output.
	// This avoids buffer underruns which result in audible crackling on stream start with small buffers.
	if(wasempty) waveOutPause(m_hWaveOut);

	while((ULONG)oldBuffersPending < m_nPreparedHeaders)
	{
		SourceAudioRead(m_WaveBuffers[m_nWriteBuffer].lpData, m_nWaveBufferSize/m_BytesPerSample);
		nLatency += m_nWaveBufferSize;
		nBytesWritten += m_nWaveBufferSize;
		m_WaveBuffers[m_nWriteBuffer].dwBufferLength = m_nWaveBufferSize;
		InterlockedIncrement(&m_nBuffersPending);
		oldBuffersPending++; // increment separately to avoid looping without leaving at all when rendering takes more than 100% CPU
		waveOutWrite(m_hWaveOut, &m_WaveBuffers[m_nWriteBuffer], sizeof(WAVEHDR));
		m_nWriteBuffer++;
		m_nWriteBuffer %= m_nPreparedHeaders;
		SourceAudioDone(m_nWaveBufferSize/m_BytesPerSample, nLatency/m_BytesPerSample);
	}

	if(wasempty) waveOutRestart(m_hWaveOut);

}


int64 CWaveDevice::GetStreamPositionSamples() const
//-------------------------------------------------
{
	if(!IsOpen()) return 0;
	MMTIME mmtime;
	MemsetZero(mmtime);
	mmtime.wType = TIME_SAMPLES;
	if(waveOutGetPosition(m_hWaveOut, &mmtime, sizeof(mmtime)) != MMSYSERR_NOERROR) return 0;
	switch(mmtime.wType)
	{
		case TIME_BYTES:   return mmtime.u.cb / m_BytesPerSample; break;
		case TIME_MS:      return mmtime.u.ms * m_nBytesPerSec / (1000 * m_BytesPerSample); break;
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


BOOL CWaveDevice::EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize)
//--------------------------------------------------------------------------------
{
	WAVEOUTCAPS woc;

	if (!gnNumWaveDevs)
	{
		gnNumWaveDevs = waveOutGetNumDevs();
	}
	if (nIndex > gnNumWaveDevs) return FALSE;
	if (nIndex)
	{
		MemsetZero(woc);
		waveOutGetDevCaps(nIndex-1, &woc, sizeof(woc));
		if (pszDescription) lstrcpyn(pszDescription, woc.szPname, cbSize);
	} else
	{
		if (pszDescription) lstrcpyn(pszDescription, "Auto (Wave Mapper)", cbSize);
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound device
//

#ifndef NO_DSOUND

#ifndef DSBCAPS_GLOBALFOCUS
#define DSBCAPS_GLOBALFOCUS		0x8000
#endif

#define MAX_DSOUND_DEVICES		16

typedef BOOL (WINAPI * LPDSOUNDENUMERATE)(LPDSENUMCALLBACK lpDSEnumCallback, LPVOID lpContext);
typedef HRESULT (WINAPI * LPDSOUNDCREATE)(GUID * lpGuid, LPDIRECTSOUND * ppDS, IUnknown * pUnkOuter);

static HINSTANCE ghDSoundDLL = NULL;
static LPDSOUNDENUMERATE gpDSoundEnumerate = NULL;
static LPDSOUNDCREATE gpDSoundCreate = NULL;
static BOOL gbDSoundEnumerated = FALSE;
static UINT gnDSoundDevices = 0;
static GUID *glpDSoundGUID[MAX_DSOUND_DEVICES];
static CHAR gszDSoundDrvNames[MAX_DSOUND_DEVICES][64];


static BOOL WINAPI DSEnumCallback(GUID * lpGuid, LPCSTR lpstrDescription, LPCSTR, LPVOID)
//---------------------------------------------------------------------------------------
{
	if (gnDSoundDevices >= MAX_DSOUND_DEVICES) return FALSE;
	if ((lpstrDescription))
	{
		if (lpGuid)
		{
			//if ((gnDSoundDevices) && (!glpDSoundGUID[gnDSoundDevices-1])) gnDSoundDevices--;
			glpDSoundGUID[gnDSoundDevices] = new GUID;
			*glpDSoundGUID[gnDSoundDevices] = *lpGuid;
		} else glpDSoundGUID[gnDSoundDevices] = NULL;
		lstrcpyn(gszDSoundDrvNames[gnDSoundDevices], lpstrDescription, 64);
		gnDSoundDevices++;
		gbDSoundEnumerated = TRUE;
	}
	return TRUE;
}


BOOL CDSoundDevice::EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize)
//----------------------------------------------------------------------------------
{
	if (!gpDSoundEnumerate) return FALSE;
	if (!gbDSoundEnumerated)
	{
		gpDSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, NULL);
	}
	if (nIndex >= gnDSoundDevices) return FALSE;
	lstrcpyn(pszDescription, gszDSoundDrvNames[nIndex], cbSize);
	return TRUE;
}


CDSoundDevice::CDSoundDevice()
//----------------------------
{
	m_piDS = NULL;
	m_pPrimary = NULL;
	m_pMixBuffer = NULL;
	m_bMixRunning = FALSE;
	m_nBytesPerSec = 0;
	m_BytesPerSample = 0;
}


CDSoundDevice::~CDSoundDevice()
//-----------------------------
{
	Reset();
	Close();
}


bool CDSoundDevice::InternalOpen(UINT nDevice)
//--------------------------------------------
{
	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext)) return false;
	WAVEFORMATEX *pwfx = &wfext.Format;

	DSBUFFERDESC dsbd;
	DSBCAPS dsc;
	UINT nPriorityLevel = (m_Setttings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE) ? DSSCL_WRITEPRIMARY : DSSCL_PRIORITY;

	if (m_piDS) return true;
	if (!gpDSoundEnumerate) return false;
	if (!gbDSoundEnumerated) gpDSoundEnumerate((LPDSENUMCALLBACK)DSEnumCallback, NULL);
	if ((nDevice >= gnDSoundDevices) || (!gpDSoundCreate)) return false;
	if (gpDSoundCreate(glpDSoundGUID[nDevice], &m_piDS, NULL) != DS_OK) return false;
	if (!m_piDS) return false;
	m_piDS->SetCooperativeLevel(m_Setttings.hWnd, nPriorityLevel);
	m_bMixRunning = FALSE;
	m_nDSoundBufferSize = (m_Setttings.LatencyMS * pwfx->nAvgBytesPerSec) / 1000;
	m_nDSoundBufferSize = (m_nDSoundBufferSize + 15) & ~15;
	if(m_nDSoundBufferSize < DSOUND_MINBUFFERSIZE) m_nDSoundBufferSize = DSOUND_MINBUFFERSIZE;
	if(m_nDSoundBufferSize > DSOUND_MAXBUFFERSIZE) m_nDSoundBufferSize = DSOUND_MAXBUFFERSIZE;
	m_nBytesPerSec = pwfx->nAvgBytesPerSec;
	m_BytesPerSample = (pwfx->wBitsPerSample/8) * pwfx->nChannels;
	if(!(m_Setttings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE))
	{
		// Set the format of the primary buffer
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
		dsbd.dwBufferBytes = 0;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = NULL;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pPrimary, NULL) != DS_OK)
		{
			Close();
			return false;
		}
		m_pPrimary->SetFormat(pwfx);
		///////////////////////////////////////////////////
		// Create the secondary buffer
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = m_nDSoundBufferSize;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = pwfx;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pMixBuffer, NULL) != DS_OK)
		{
			Close();
			return false;
		}
	} else
	{
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = 0;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = NULL;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pPrimary, NULL) != DS_OK)
		{
			Close();
			return false;
		}
		if (m_pPrimary->SetFormat(pwfx) != DS_OK)
		{
			Close();
			return false;
		}
		dsc.dwSize = sizeof(dsc);
		if (m_pPrimary->GetCaps(&dsc) != DS_OK)
		{
			Close();
			return false;
		}
		m_nDSoundBufferSize = dsc.dwBufferBytes;
		m_pMixBuffer = m_pPrimary;
		m_pMixBuffer->AddRef();
	}
	LPVOID lpBuf1, lpBuf2;
	DWORD dwSize1, dwSize2;
	if (m_pMixBuffer->Lock(0, m_nDSoundBufferSize, &lpBuf1, &dwSize1, &lpBuf2, &dwSize2, 0) == DS_OK)
	{
		UINT zero = (pwfx->wBitsPerSample == 8) ? 0x80 : 0x00;
		if ((lpBuf1) && (dwSize1)) memset(lpBuf1, zero, dwSize1);
		if ((lpBuf2) && (dwSize2)) memset(lpBuf2, zero, dwSize2);
		m_pMixBuffer->Unlock(lpBuf1, dwSize1, lpBuf2, dwSize2);
	} else
	{
		DWORD dwStat = 0;
		m_pMixBuffer->GetStatus(&dwStat);
		if (dwStat & DSBSTATUS_BUFFERLOST) m_pMixBuffer->Restore();
	}
	m_RealLatencyMS = m_nDSoundBufferSize * 1000.0f / m_nBytesPerSec;
	m_RealUpdateIntervalMS = CLAMP(static_cast<float>(m_Setttings.UpdateIntervalMS), 1.0f, m_nDSoundBufferSize * 1000.0f / ( 2.0f * m_nBytesPerSec ) );
	m_dwWritePos = 0xFFFFFFFF;
	return true;
}


bool CDSoundDevice::InternalClose()
//---------------------------------
{
	if (m_pMixBuffer)
	{
		m_pMixBuffer->Release();
		m_pMixBuffer = NULL;
	}
	if (m_pPrimary)
	{
		m_pPrimary->Release();
		m_pPrimary = NULL;
	}
	if (m_piDS)
	{
		m_piDS->Release();
		m_piDS = NULL;
	}
	m_bMixRunning = FALSE;
	return true;
}


void CDSoundDevice::StartFromSoundThread()
//----------------------------------------
{
	if(!m_pMixBuffer) return;
	// done in FillAudioBuffer for now
}


void CDSoundDevice::StopFromSoundThread()
//---------------------------------------
{
	if(m_pMixBuffer)
	{
		m_pMixBuffer->Stop();
	}
	m_bMixRunning = FALSE;
}


void CDSoundDevice::ResetFromOutsideSoundThread()
//-----------------------------------------------
{
	if(m_pMixBuffer)
	{
		m_pMixBuffer->Stop();
	}
	m_bMixRunning = FALSE;
}


DWORD CDSoundDevice::LockBuffer(DWORD dwBytes, LPVOID *lpBuf1, LPDWORD lpSize1, LPVOID *lpBuf2, LPDWORD lpSize2)
//--------------------------------------------------------------------------------------------------------------
{
	DWORD d, dwPlay = 0, dwWrite = 0;

	if ((!m_pMixBuffer) || (!m_nDSoundBufferSize)) return 0;
	if (m_pMixBuffer->GetCurrentPosition(&dwPlay, &dwWrite) != DS_OK) return 0;
	if ((m_dwWritePos >= m_nDSoundBufferSize) || (!m_bMixRunning))
	{
		m_dwWritePos = dwWrite;
		m_dwLatency = 0;
		d = m_nDSoundBufferSize/2;
	} else
	{
		dwWrite = m_dwWritePos;
		m_dwLatency = (m_nDSoundBufferSize + dwWrite - dwPlay) % m_nDSoundBufferSize;
		if (dwPlay >= dwWrite)
			d = dwPlay - dwWrite;
		else
			d = m_nDSoundBufferSize + dwPlay - dwWrite;
	}
	if (d > m_nDSoundBufferSize) return FALSE;
	if (d > m_nDSoundBufferSize/2) d = m_nDSoundBufferSize/2;
	if (dwBytes)
	{
		if (m_dwLatency > dwBytes) return 0;
		if (m_dwLatency+d > dwBytes) d = dwBytes - m_dwLatency;
	}
	d &= ~0x0f;
	if (d <= 16) return 0;
	HRESULT hr = m_pMixBuffer->Lock(m_dwWritePos, d, lpBuf1, lpSize1, lpBuf2, lpSize2, 0);
	if(hr == DSERR_BUFFERLOST)
	{
		// buffer lost, restore buffer and try again, fail if it fails again
		if(m_pMixBuffer->Restore() != DS_OK) return 0;
		if(m_pMixBuffer->Lock(m_dwWritePos, d, lpBuf1, lpSize1, lpBuf2, lpSize2, 0) != DS_OK) return 0;
		return d;
	} else if(hr != DS_OK)
	{
		return 0;
	}
	return d;
}


BOOL CDSoundDevice::UnlockBuffer(LPVOID lpBuf1, DWORD dwSize1, LPVOID lpBuf2, DWORD dwSize2)
//------------------------------------------------------------------------------------------
{
	if (!m_pMixBuffer) return FALSE;
	if (m_pMixBuffer->Unlock(lpBuf1, dwSize1, lpBuf2, dwSize2) == DS_OK)
	{
		m_dwWritePos += dwSize1 + dwSize2;
		if (m_dwWritePos >= m_nDSoundBufferSize) m_dwWritePos -= m_nDSoundBufferSize;
		return TRUE;
	}
	return FALSE;
}


void CDSoundDevice::FillAudioBuffer()
//-----------------------------------
{
	LPVOID lpBuf1=NULL, lpBuf2=NULL;
	DWORD dwSize1=0, dwSize2=0;
	DWORD dwBytes;

	if (!m_pMixBuffer) return;
	dwBytes = LockBuffer(m_nDSoundBufferSize, &lpBuf1, &dwSize1, &lpBuf2, &dwSize2);
	if (dwBytes)
	{
		if ((lpBuf1) && (dwSize1)) SourceAudioRead(lpBuf1, dwSize1/m_BytesPerSample);
		if ((lpBuf2) && (dwSize2)) SourceAudioRead(lpBuf2, dwSize2/m_BytesPerSample);
		UnlockBuffer(lpBuf1, dwSize1, lpBuf2, dwSize2);
		DWORD dwStatus = 0;
		m_pMixBuffer->GetStatus(&dwStatus);
		if(!m_bMixRunning || !(dwStatus & DSBSTATUS_PLAYING))
		{
			HRESULT hr;
			if(!(dwStatus & DSBSTATUS_BUFFERLOST))
			{
				// start playing
				hr = m_pMixBuffer->Play(0, 0, DSBPLAY_LOOPING);
			} else
			{
				// buffer lost flag is set, do not try start playing, we know it will fail with DSERR_BUFFERLOST.
				hr = DSERR_BUFFERLOST;
			}
			if(hr == DSERR_BUFFERLOST)
			{
				// buffer lost, restore buffer and try again, fail if it fails again
				if(m_pMixBuffer->Restore() != DS_OK) return;
				if(m_pMixBuffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK) return;
			} else if(hr != DS_OK)
			{
				return;
			}
			m_bMixRunning = TRUE; 
		}
		SourceAudioDone((dwSize1+dwSize2)/m_BytesPerSample, m_dwLatency/m_BytesPerSample);
	}
}

#endif // NO_DIRECTSOUND


///////////////////////////////////////////////////////////////////////////////////////
//
// ASIO Device implementation
//

#ifndef NO_ASIO

#define ASIO_MAX_DRIVERS	8
#define ASIO_MAXDRVNAMELEN	128

typedef struct _ASIODRIVERDESC
{
	CLSID clsid; 
	CHAR name[80];
} ASIODRIVERDESC;

CASIODevice *CASIODevice::gpCurrentAsio = NULL;
LONG CASIODevice::gnFillBuffers = 0;
int CASIODevice::baseChannel = 0;
static UINT gnNumAsioDrivers = 0;
static BOOL gbAsioEnumerated = FALSE;
static ASIODRIVERDESC gAsioDrivers[ASIO_MAX_DRIVERS];

static DWORD g_dwBuffer = 0;

static int g_asio_startcount = 0;


BOOL CASIODevice::EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize)
//--------------------------------------------------------------------------------
{
	if (!gbAsioEnumerated)
	{
		HKEY hkEnum = NULL;
		CHAR keyname[ASIO_MAXDRVNAMELEN];
		CHAR s[256];
		WCHAR w[100];
		LONG cr;
		DWORD index;

		cr = RegOpenKey(HKEY_LOCAL_MACHINE, "software\\asio", &hkEnum);
		index = 0;
		while ((cr == ERROR_SUCCESS) && (gnNumAsioDrivers < ASIO_MAX_DRIVERS))
		{
			if ((cr = RegEnumKey(hkEnum, index, (LPTSTR)keyname, ASIO_MAXDRVNAMELEN)) == ERROR_SUCCESS)
			{
			#ifdef ASIO_LOG
				Log("ASIO: Found \"%s\":\n", keyname);
			#endif
				HKEY hksub;

				if ((RegOpenKeyEx(hkEnum, (LPCTSTR)keyname, 0, KEY_READ, &hksub)) == ERROR_SUCCESS)
				{
					DWORD datatype = REG_SZ;
					DWORD datasize = 64;
					
					if (ERROR_SUCCESS == RegQueryValueEx(hksub, "description", 0, &datatype, (LPBYTE)gAsioDrivers[gnNumAsioDrivers].name, &datasize))
					{
					#ifdef ASIO_LOG
						Log("  description =\"%s\":\n", gAsioDrivers[gnNumAsioDrivers].name);
					#endif
					} else
					{
						lstrcpyn(gAsioDrivers[gnNumAsioDrivers].name, keyname, 64);
					}
					datatype = REG_SZ;
					datasize = sizeof(s);
					if (ERROR_SUCCESS == RegQueryValueEx(hksub, "clsid", 0, &datatype, (LPBYTE)s, &datasize))
					{
						MultiByteToWideChar(CP_ACP, 0, (LPCSTR)s,-1,(LPWSTR)w,100);
						if (CLSIDFromString((LPOLESTR)w, (LPCLSID)&gAsioDrivers[gnNumAsioDrivers].clsid) == S_OK)
						{
						#ifdef ASIO_LOG
							Log("  clsid=\"%s\"\n", s);
						#endif
							gnNumAsioDrivers++;
						}
					}
					RegCloseKey(hksub);
				}
			}
			index++;
		}
		if (hkEnum) RegCloseKey(hkEnum);
		gbAsioEnumerated = TRUE;
	}
	if (nIndex < gnNumAsioDrivers)
	{
		if (pszDescription) lstrcpyn(pszDescription, gAsioDrivers[nIndex].name, cbSize);
		return TRUE;
	}
	return FALSE;
}


CASIODevice::CASIODevice()
//------------------------
{
	m_pAsioDrv = NULL;
	m_Callbacks.bufferSwitch = BufferSwitch;
	m_Callbacks.sampleRateDidChange = SampleRateDidChange;
	m_Callbacks.asioMessage = AsioMessage;
	m_Callbacks.bufferSwitchTimeInfo = BufferSwitchTimeInfo;
	m_nBitsPerSample = 0; // Unknown
	m_nCurrentDevice = (ULONG)-1;
	m_bMixRunning = FALSE;
	InterlockedExchange(&m_RenderSilence, 0);
	InterlockedExchange(&m_RenderingSilence, 0);
}


CASIODevice::~CASIODevice()
//-------------------------
{
	Reset();
	Close();
}


bool CASIODevice::InternalOpen(UINT nDevice)
//------------------------------------------
{
	bool bOk = false;

	if (IsOpen()) Close();
	if (!gbAsioEnumerated) EnumerateDevices(nDevice, NULL, 0);
	if (nDevice >= gnNumAsioDrivers) return FALSE;
	if (nDevice != m_nCurrentDevice)
	{
		m_nCurrentDevice = nDevice;
		m_nBitsPerSample = 0;
	}
#ifdef ASIO_LOG
	Log("CASIODevice::Open(%d:\"%s\"): %d-bit, %d channels, %dHz\n",
		nDevice, gAsioDrivers[nDevice].name, m_Setttings.BitsPerSample, m_Setttings.Channels, m_Setttings.Samplerate);
#endif
	OpenDevice(nDevice);

	if (IsOpen())
	{
		long nInputChannels = 0, nOutputChannels = 0;
		long minSize = 0, maxSize = 0, preferredSize = 0, granularity = 0;

		if ((m_Setttings.Channels > ASIO_MAX_CHANNELS)
		 || ((m_Setttings.BitsPerSample != 16) && (m_Setttings.BitsPerSample != 32))) goto abort;
		m_nChannels = m_Setttings.Channels;
		m_pAsioDrv->getChannels(&nInputChannels, &nOutputChannels);
	#ifdef ASIO_LOG
		Log("  getChannels: %d inputs, %d outputs\n", nInputChannels, nOutputChannels);
	#endif
		if (m_Setttings.Channels > nOutputChannels) goto abort;
		if (m_pAsioDrv->setSampleRate(m_Setttings.Samplerate) != ASE_OK)
		{
		#ifdef ASIO_LOG
			Log("  setSampleRate(%d) failed (sample rate not supported)!\n", m_Setttings.Samplerate);
		#endif
			goto abort;
		}
		m_nBitsPerSample = m_Setttings.BitsPerSample;
		for (UINT ich=0; ich<m_Setttings.Channels; ich++)
		{
			m_ChannelInfo[ich].channel = ich;
			m_ChannelInfo[ich].isInput = ASIOFalse;
			m_pAsioDrv->getChannelInfo(&m_ChannelInfo[ich]);
		#ifdef ASIO_LOG
			Log("  getChannelInfo(%d): isActive=%d channelGroup=%d type=%d name=\"%s\"\n",
				ich, m_ChannelInfo[ich].isActive, m_ChannelInfo[ich].channelGroup, m_ChannelInfo[ich].type, m_ChannelInfo[ich].name);
		#endif
			m_BufferInfo[ich].isInput = ASIOFalse;
			m_BufferInfo[ich].channelNum = ich + CASIODevice::baseChannel;		// map MPT channel i to ASIO channel i
			m_BufferInfo[ich].buffers[0] = NULL;
			m_BufferInfo[ich].buffers[1] = NULL;
			if ((m_ChannelInfo[ich].type & 0x0f) == ASIOSTInt16MSB)
			{
				if (m_nBitsPerSample < 16)
				{
					m_nBitsPerSample = 16;
					goto abort;
				}
			} else
			{
				if (m_nBitsPerSample != 32)
				{
					m_nBitsPerSample = 32;
					goto abort;
				}
			}
			switch(m_ChannelInfo[ich].type & 0x0f)
			{
			case ASIOSTInt16MSB:	m_nAsioSampleSize = 2; break;
			case ASIOSTInt24MSB:	m_nAsioSampleSize = 3; break;
			case ASIOSTFloat64MSB:	m_nAsioSampleSize = 8; break;
			default:				m_nAsioSampleSize = 4;
			}
		}
		m_pAsioDrv->getBufferSize(&minSize, &maxSize, &preferredSize, &granularity);
	#ifdef ASIO_LOG
		Log("  getBufferSize(): minSize=%d maxSize=%d preferredSize=%d granularity=%d\n",
				minSize, maxSize, preferredSize, granularity);
	#endif
		m_nAsioBufferLen = ((m_Setttings.LatencyMS * m_Setttings.Samplerate) / 2000);
		if (m_nAsioBufferLen < (UINT)minSize) m_nAsioBufferLen = minSize; else
		if (m_nAsioBufferLen > (UINT)maxSize) m_nAsioBufferLen = maxSize; else
		if (granularity < 0)
		{
			//rewbs.ASIOfix:
			/*UINT n = (minSize < 32) ? 32 : minSize;
			if (n % granularity) n = (n + granularity - 1) - (n % granularity);
			while ((n+(n>>1) < m_nAsioBufferLen) && (n*2 <= (UINT)maxSize))
			{
				n *= 2;
			}
			m_nAsioBufferLen = n;*/
			//end rewbs.ASIOfix
			m_nAsioBufferLen = preferredSize;

		} else
		if (granularity > 0)
		{
			int n = (minSize < 32) ? 32 : minSize;
			n = (n + granularity-1);
			n -= (n % granularity);
			while ((n+(granularity>>1) < (int)m_nAsioBufferLen) && (n+granularity <= maxSize))
			{
				n += granularity;
			}
			m_nAsioBufferLen = n;
		}
		m_RealLatencyMS = m_nAsioBufferLen * 2 * 1000.0f / m_Setttings.Samplerate;
		m_RealUpdateIntervalMS = m_nAsioBufferLen * 1000.0f / m_Setttings.Samplerate;
	#ifdef ASIO_LOG
		Log("  Using buffersize=%d samples\n", m_nAsioBufferLen);
	#endif
		if (m_pAsioDrv->createBuffers(m_BufferInfo, m_nChannels, m_nAsioBufferLen, &m_Callbacks) == ASE_OK)
		{
			for (UINT iInit=0; iInit<m_nChannels; iInit++)
			{
				if (m_BufferInfo[iInit].buffers[0])
				{
					memset(m_BufferInfo[iInit].buffers[0], 0, m_nAsioBufferLen * m_nAsioSampleSize);
				}
				if (m_BufferInfo[iInit].buffers[1])
				{
					memset(m_BufferInfo[iInit].buffers[1], 0, m_nAsioBufferLen * m_nAsioSampleSize);
				}
			}
			m_bPostOutput = (m_pAsioDrv->outputReady() == ASE_OK) ? TRUE : FALSE;
			bOk = true;
		}
	#ifdef ASIO_LOG
		else Log("  createBuffers failed!\n");
	#endif
	}
abort:
	if (bOk)
	{
		gpCurrentAsio = this;
		gnFillBuffers = 2;
	} else
	{
	#ifdef ASIO_LOG
		Log("Error opening ASIO device!\n");
	#endif
		CloseDevice();
	}
	return bOk;
}


void CASIODevice::SetRenderSilence(bool silence, bool wait)
//---------------------------------------------------------
{
	InterlockedExchange(&m_RenderSilence, silence?1:0);
	if(!wait)
	{
		return;
	}
	DWORD pollingstart = GetTickCount();
	while(InterlockedExchangeAdd(&m_RenderingSilence, 0) != (silence?1:0))
	{
		if(GetTickCount() - pollingstart > 250)
		{
			if(silence)
			{
				if(CriticalSection::IsLocked())
				{
					ASSERT_WARN_MESSAGE(false, "AudioCriticalSection locked while stopping ASIO");
				} else
				{
					ASSERT_WARN_MESSAGE(false, "waiting for asio failed in Stop()");
				}
			} else
			{
				if(CriticalSection::IsLocked())
				{
					ASSERT_WARN_MESSAGE(false, "AudioCriticalSection locked while starting ASIO");
				} else
				{
					ASSERT_WARN_MESSAGE(false, "waiting for asio failed in Start()");
				}
			}
			break;
		}
		Sleep(1);
	}
}


void CASIODevice::InternalStart()
//-------------------------------
{
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while starting ASIO");

		ALWAYS_ASSERT(g_asio_startcount==0);
		g_asio_startcount++;

		if(!m_bMixRunning)
		{
			SetRenderSilence(false);
			m_bMixRunning = TRUE;
			try
			{
				m_pAsioDrv->start();
			} catch(...)
			{
				CASIODevice::ReportASIOException("ASIO crash in start()\n");
				m_bMixRunning = FALSE;
			}
		} else
		{
			SetRenderSilence(false, true);
		}
}


void CASIODevice::InternalStop()
//------------------------------
{
	ALWAYS_ASSERT(g_asio_startcount==1);
	ALWAYS_ASSERT_WARN_MESSAGE(!CriticalSection::IsLocked(), "AudioCriticalSection locked while stopping ASIO");

		SetRenderSilence(true, true);
		g_asio_startcount--;
		ALWAYS_ASSERT(g_asio_startcount==0);
}


bool CASIODevice::InternalClose()
//-------------------------------
{
	if (IsOpen())
	{
		Stop();
		if (m_bMixRunning)
		{
			m_bMixRunning = FALSE;
			ALWAYS_ASSERT(g_asio_startcount==0);
			try
			{
				m_pAsioDrv->stop();
			} catch(...)
			{
				CASIODevice::ReportASIOException("ASIO crash in stop()\n");
			}
		}
		g_asio_startcount = 0;
		SetRenderSilence(false);
		try
		{
			m_pAsioDrv->disposeBuffers();
		} catch(...)
		{
			CASIODevice::ReportASIOException("ASIO crash in disposeBuffers()\n");
		}
		CloseDevice();
	}
	if (gpCurrentAsio == this)
	{
		gpCurrentAsio = NULL;
	}
	return true;
}


void CASIODevice::InternalReset()
//-------------------------------
{
	if(IsOpen())
	{
		Stop();
		if(m_bMixRunning)
		{
			m_bMixRunning = FALSE;
			ALWAYS_ASSERT(g_asio_startcount==0);
			try
			{
				m_pAsioDrv->stop();
			} catch(...)
			{
				CASIODevice::ReportASIOException("ASIO crash in stop()\n");
			}
			g_asio_startcount = 0;
			SetRenderSilence(false);
		}
	}
}


void CASIODevice::OpenDevice(UINT nDevice)
//----------------------------------------
{
	if (IsOpen())
	{
		return;
	}

	CLSID clsid = gAsioDrivers[nDevice].clsid;
	if (CoCreateInstance(clsid,0,CLSCTX_INPROC_SERVER, clsid, (void **)&m_pAsioDrv) == S_OK)
	{
		m_pAsioDrv->init((void *)m_Setttings.hWnd);
	} else
	{
#ifdef ASIO_LOG
		Log("  CoCreateInstance failed!\n");
#endif
		m_pAsioDrv = NULL;
	}
}


void CASIODevice::CloseDevice()
//-----------------------------
{
	if (IsOpen())
	{
		try
		{
			m_pAsioDrv->Release();
		} catch(...)
		{
			CASIODevice::ReportASIOException("ASIO crash in Release()\n");
		}
		m_pAsioDrv = NULL;
	}
}

void CASIODevice::FillAudioBuffer()
//---------------------------------
{
	bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);

	DWORD dwSampleSize = m_nChannels*(m_nBitsPerSample>>3);
	DWORD dwSamplesLeft = m_nAsioBufferLen;
	DWORD dwFrameLen = (ASIO_BLOCK_LEN*sizeof(int)) / dwSampleSize;
	DWORD dwBufferOffset = 0;
	
	g_dwBuffer &= 1;
	//Log("FillAudioBuffer(%d): dwSampleSize=%d dwSamplesLeft=%d dwFrameLen=%d\n", g_dwBuffer, dwSampleSize, dwSamplesLeft, dwFrameLen);
	while ((LONG)dwSamplesLeft > 0)
	{
		UINT n = (dwSamplesLeft < dwFrameLen) ? dwSamplesLeft : dwFrameLen;
		if(rendersilence)
		{
			memset(m_FrameBuffer, 0, n*dwSampleSize);
		} else
		{
			SourceAudioRead(m_FrameBuffer, n);
		}
		dwSamplesLeft -= n;
		for (UINT ich=0; ich<m_nChannels; ich++)
		{
			char *psrc = (char *)m_FrameBuffer;
			char *pbuffer = (char *)m_BufferInfo[ich].buffers[g_dwBuffer];
			switch(m_ChannelInfo[ich].type)
			{
			case ASIOSTInt16MSB:
				if (m_nBitsPerSample == 32)
					Cvt32To16msb(pbuffer+dwBufferOffset*2, psrc+ich*4, m_nChannels*4, n);
				else
					Cvt16To16msb(pbuffer+dwBufferOffset*2, psrc+ich*2, m_nChannels*2, n);
				break;
			case ASIOSTInt16LSB:
				if (m_nBitsPerSample == 32)
					Cvt32To16(pbuffer+dwBufferOffset*2, psrc+ich*4, m_nChannels*4, n);
				else
					Cvt16To16(pbuffer+dwBufferOffset*2, psrc+ich*2, m_nChannels*2, n);
				break;
			case ASIOSTInt24MSB:
				Cvt32To24msb(pbuffer+dwBufferOffset*3, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTInt24LSB:
				Cvt32To24(pbuffer+dwBufferOffset*3, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTInt32MSB:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 0);
				break;
			case ASIOSTInt32LSB:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 0);
				break;
			case ASIOSTFloat32MSB:
				Cvt32To32f(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n);
				EndianSwap32(pbuffer+dwBufferOffset*4, n);
				break;
			case ASIOSTFloat32LSB:
				Cvt32To32f(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTFloat64MSB:
				Cvt32To64f(pbuffer+dwBufferOffset*8, psrc+ich*4, m_nChannels*4, n);
				EndianSwap64(pbuffer+dwBufferOffset*4, n);
				break;
			case ASIOSTFloat64LSB:
				Cvt32To64f(pbuffer+dwBufferOffset*8, psrc+ich*4, m_nChannels*4, n);
				break;
			case ASIOSTInt32MSB16:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 16);
				break;
			case ASIOSTInt32LSB16:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 16);
				break;
			case ASIOSTInt32MSB18:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 14);
				break;
			case ASIOSTInt32LSB18:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 14);
				break;
			case ASIOSTInt32MSB20:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 12);
				break;
			case ASIOSTInt32LSB20:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 12);
				break;
			case ASIOSTInt32MSB24:
				Cvt32To32msb(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 8);
				break;
			case ASIOSTInt32LSB24:
				Cvt32To32(pbuffer+dwBufferOffset*4, psrc+ich*4, m_nChannels*4, n, 8);
				break;
			}
		}
		dwBufferOffset += n;
	}
	if (m_bPostOutput) m_pAsioDrv->outputReady();
	if(!rendersilence)
	{
		SourceAudioDone(dwBufferOffset, m_nAsioBufferLen);
	}
	return;
}


void CASIODevice::BufferSwitch(long doubleBufferIndex)
//----------------------------------------------------
{
	g_dwBuffer = doubleBufferIndex;
	bool rendersilence = (InterlockedExchangeAdd(&m_RenderSilence, 0) == 1);
	InterlockedExchange(&m_RenderingSilence, rendersilence ? 1 : 0 );
	if(rendersilence)
	{
		FillAudioBuffer();
	} else
	{
		SourceFillAudioBufferLocked();
	}
}


void CASIODevice::BufferSwitch(long doubleBufferIndex, ASIOBool directProcess)
//----------------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(directProcess);
	if(gpCurrentAsio)
	{
		gpCurrentAsio->BufferSwitch(doubleBufferIndex);
	} else
	{
		ALWAYS_ASSERT(false && "gpCurrentAsio");
	}
}


void CASIODevice::SampleRateDidChange(ASIOSampleRate sRate)
//---------------------------------------------------------
{
	UNREFERENCED_PARAMETER(sRate);
}


long CASIODevice::AsioMessage(long selector, long value, void* message, double* opt)
//----------------------------------------------------------------------------------
{
	UNREFERENCED_PARAMETER(value);
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(opt);
#ifdef ASIO_LOG
	// Log("AsioMessage(%d, %d)\n", selector, value);
#endif
	switch(selector)
	{
	case kAsioEngineVersion: return 2;
	}
	return 0;
}


ASIOTime* CASIODevice::BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
//-----------------------------------------------------------------------------------------------------------
{
	BufferSwitch(doubleBufferIndex, directProcess);
	return params;
}


void CASIODevice::EndianSwap64(void *pbuffer, UINT nSamples)
//----------------------------------------------------------
{
	_asm {
	mov edx, pbuffer
	mov ecx, nSamples
swaploop:
	mov eax, dword ptr [edx]
	mov ebx, dword ptr [edx+4]
	add edx, 8
	bswap eax
	bswap ebx
	dec ecx
	mov dword ptr [edx-8], ebx
	mov dword ptr [edx-4], eax
	jnz swaploop
	}
}


void CASIODevice::EndianSwap32(void *pbuffer, UINT nSamples)
//----------------------------------------------------------
{
	_asm {
	mov edx, pbuffer
	mov ecx, nSamples
swaploop:
	mov eax, dword ptr [edx]
	add edx, 4
	bswap eax
	dec ecx
	mov dword ptr [edx-4], eax
	jnz swaploop
	}
}


void CASIODevice::Cvt16To16(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	movsx eax, word ptr [ebx]
	add ebx, esi
	add edi, 2
	dec ecx
	mov word ptr [edi-2], ax
	jnz cvtloop
	}
}


void CASIODevice::Cvt16To16msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	movsx eax, word ptr [ebx]
	add ebx, esi
	add edi, 2
	dec ecx
	mov byte ptr [edi-2], ah
	mov byte ptr [edi-1], al
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To24(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 3
	mov edx, eax
	shr eax, 8
	shr edx, 24
	dec ecx
	mov byte ptr [edi-3], al
	mov byte ptr [edi-2], ah
	mov byte ptr [edi-1], dl
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To24msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 3
	mov edx, eax
	shr eax, 8
	shr edx, 24
	dec ecx
	mov byte ptr [edi-3], dl
	mov byte ptr [edi-2], ah
	mov byte ptr [edi-1], al
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To32(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples, UINT nShift)
//-----------------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	mov ecx, nShift
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 4
	sar eax, cl
	dec edx
	mov dword ptr [edi-4], eax
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To32msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples, UINT nShift)
//--------------------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	mov ecx, nShift
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 4
	sar eax, cl
	bswap eax
	dec edx
	mov dword ptr [edi-4], eax
	jnz cvtloop
	}
}


const float _pow2_31 = 1.0f / 2147483648.0f;

void CASIODevice::Cvt32To32f(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	fld _pow2_31
cvtloop:
	fild dword ptr [ebx]
	add ebx, esi
	add edi, 4
	fmul st(0), st(1)
	dec edx
	fstp dword ptr [edi-4]
	jnz cvtloop
	fstp st(1)
	}
}


void CASIODevice::Cvt32To64f(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov edx, nSamples
	fld _pow2_31
cvtloop:
	fild dword ptr [ebx]
	add ebx, esi
	add edi, 8
	fmul st(0), st(1)
	dec edx
	fstp qword ptr [edi-8]
	jnz cvtloop
	fstp st(1)
	}
}


void CASIODevice::Cvt32To16(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//----------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 2
	sar eax, 16
	dec ecx
	mov word ptr [edi-2], ax
	jnz cvtloop
	}
}


void CASIODevice::Cvt32To16msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples)
//-------------------------------------------------------------------------------------
{
	_asm {
	mov ebx, psrc
	mov edi, pdst
	mov esi, nSampleSize
	mov ecx, nSamples
cvtloop:
	mov eax, dword ptr [ebx]
	add ebx, esi
	add edi, 2
	bswap eax
	dec ecx
	mov word ptr [edi-2], ax
	jnz cvtloop
	}
}

BOOL CASIODevice::ReportASIOException(LPCSTR format,...)
//------------------------------------------------------
{
	CHAR cBuf[1024];
	va_list va;
	va_start(va, format);
	wvsprintf(cBuf, format, va);
	Reporting::Notification(cBuf);
	Log(cBuf);
	va_end(va);
	
	return TRUE;
}


bool CASIODevice::CanSampleRate(UINT nDevice, std::vector<UINT> &samplerates, std::vector<bool> &result)
//------------------------------------------------------------------------------------------------------
{
	const bool wasOpen = (m_pAsioDrv != NULL);
	if(!wasOpen)
	{
		OpenDevice(nDevice);
		if(m_pAsioDrv == NULL)
		{
			return false;
		}
	}

	bool foundSomething = false;	// is at least one sample rate supported by the device?
	result.clear();
	for(size_t i = 0; i < samplerates.size(); i++)
	{
		result.push_back((m_pAsioDrv->canSampleRate((ASIOSampleRate)samplerates[i]) == ASE_OK));
		if(result.back())
		{
			foundSomething = true;
		}
	}

	if(!wasOpen)
	{
		CloseDevice();
	}

	return foundSomething;
}


// If the device is open, this returns the current sample rate. If it's not open, it returns some sample rate supported by the device.
UINT CASIODevice::GetCurrentSampleRate(UINT nDevice)
//--------------------------------------------------
{
	const bool wasOpen = (m_pAsioDrv != NULL);
	if(!wasOpen)
	{
		OpenDevice(nDevice);
		if(m_pAsioDrv == NULL)
		{
			return 0;
		}
	}

	ASIOSampleRate samplerate;
	if(m_pAsioDrv->getSampleRate(&samplerate) != ASE_OK)
	{
		samplerate = 0;
	}

	if(!wasOpen)
	{
		CloseDevice();
	}

	return (UINT)samplerate;
}

#endif // NO_ASIO


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
	m_StreamParameters.channelCount = m_Setttings.Channels;
	switch(m_Setttings.BitsPerSample)
	{
		case 8: m_StreamParameters.sampleFormat = paUInt8; break;
		case 16: m_StreamParameters.sampleFormat = paInt16; break;
		case 24: m_StreamParameters.sampleFormat = paInt24; break;
		case 32: m_StreamParameters.sampleFormat = paInt32; break;
		default: return false; break;
	}
	m_StreamParameters.suggestedLatency = m_Setttings.LatencyMS / 1000.0;
	m_StreamParameters.hostApiSpecificStreamInfo = NULL;
	if((m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI)) && (m_Setttings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE))
	{
		MemsetZero(m_WasapiStreamInfo);
		m_WasapiStreamInfo.size = sizeof(PaWasapiStreamInfo);
		m_WasapiStreamInfo.hostApiType = paWASAPI;
		m_WasapiStreamInfo.version = 1;
		m_WasapiStreamInfo.flags = paWinWasapiExclusive;
		m_StreamParameters.hostApiSpecificStreamInfo = &m_WasapiStreamInfo;
	}
	if(Pa_IsFormatSupported(NULL, &m_StreamParameters, m_Setttings.Samplerate) != paFormatIsSupported) return false;
	if(Pa_OpenStream(&m_Stream, NULL, &m_StreamParameters, m_Setttings.Samplerate, /*static_cast<long>(m_UpdateIntervalMS * pwfx->nSamplesPerSec / 1000.0f)*/ paFramesPerBufferUnspecified, paNoFlag, StreamCallbackWrapper, (void*)this) != paNoError) return false;
	if(!Pa_GetStreamInfo(m_Stream))
	{
		Pa_CloseStream(m_Stream);
		m_Stream = 0;
		return false;
	}
	m_RealLatencyMS = static_cast<float>(Pa_GetStreamInfo(m_Stream)->outputLatency) * 1000.0f;
	m_RealUpdateIntervalMS = static_cast<float>(m_Setttings.UpdateIntervalMS);
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
		if((m_HostApi == Pa_HostApiTypeIdToHostApiIndex(paWASAPI)) && (m_Setttings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE))
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

#endif


///////////////////////////////////////////////////////////////////////////////////////
//
// Global Functions
//


static bool g_PortaudioInitialized = false;


BOOL EnumerateSoundDevices(UINT nType, UINT nIndex, LPSTR pszDesc, UINT cbSize)
//-----------------------------------------------------------------------------
{
	switch(nType)
	{
	case SNDDEV_WAVEOUT:	return CWaveDevice::EnumerateDevices(nIndex, pszDesc, cbSize);
#ifndef NO_DSOUND
	case SNDDEV_DSOUND:		return CDSoundDevice::EnumerateDevices(nIndex, pszDesc, cbSize);
#endif // NO_DIRECTSOUND
#ifndef NO_ASIO
	case SNDDEV_ASIO:		return CASIODevice::EnumerateDevices(nIndex, pszDesc, cbSize);
#endif // NO_ASIO
#ifndef NO_PORTAUDIO
	case SNDDEV_PORTAUDIO_WASAPI:
	case SNDDEV_PORTAUDIO_WDMKS:
	case SNDDEV_PORTAUDIO_WMME:
	case SNDDEV_PORTAUDIO_DS:
	case SNDDEV_PORTAUDIO_ASIO:
		return g_PortaudioInitialized ? CPortaudioDevice::EnumerateDevices(nIndex, pszDesc, cbSize, CPortaudioDevice::SndDevTypeToHostApi(nType)) : FALSE;
		break;
#endif
	}
	return FALSE;
}


ISoundDevice *CreateSoundDevice(UINT nType)
//-----------------------------------------
{
	switch(nType)
	{
	case SNDDEV_WAVEOUT:	return new CWaveDevice(); break;
#ifndef NO_DSOUND
	case SNDDEV_DSOUND:		return new CDSoundDevice(); break;
#endif // NO_DIRECTSOUND
#ifndef NO_ASIO
	case SNDDEV_ASIO:		return new CASIODevice(); break;
#endif // NO_ASIO
#ifndef NO_PORTAUDIO
	case SNDDEV_PORTAUDIO_WASAPI:
	case SNDDEV_PORTAUDIO_WDMKS:
	case SNDDEV_PORTAUDIO_WMME:
	case SNDDEV_PORTAUDIO_DS:
	case SNDDEV_PORTAUDIO_ASIO:
		return g_PortaudioInitialized ? new CPortaudioDevice(CPortaudioDevice::SndDevTypeToHostApi(nType)) : nullptr;
		break;
#endif
	}
	return nullptr;
}


#ifndef NO_PORTAUDIO

static void SndDevPortaudioInitialize()
//-------------------------------------
{
	// try loading delay-loaded dll, if it fails, portaudio gets disabled automatically
	if(Pa_Initialize() != paNoError) return;
	g_PortaudioInitialized = true;
}


static void SndDevPortaudioUnnitialize()
//--------------------------------------
{
	if(!g_PortaudioInitialized) return;
	Pa_Terminate();
	g_PortaudioInitialized = false;
}

#endif // NO_PORTAUDIO


#ifndef NO_DSOUND

static BOOL SndDevDSoundInitialize()
//----------------------------------
{
	if (ghDSoundDLL) return TRUE;
	if ((ghDSoundDLL = LoadLibrary("dsound.dll")) == NULL) return FALSE;
	static_assert(sizeof(TCHAR) == 1, "Check DirectSoundEnumerateA below");
	if ((gpDSoundEnumerate = (LPDSOUNDENUMERATE)GetProcAddress(ghDSoundDLL, "DirectSoundEnumerateA")) == NULL) return FALSE;
	if ((gpDSoundCreate = (LPDSOUNDCREATE)GetProcAddress(ghDSoundDLL, "DirectSoundCreate")) == NULL) return FALSE;
	MemsetZero(glpDSoundGUID);
	return TRUE;
}


static BOOL SndDevDSoundUninitialize()
//------------------------------------
{
	gpDSoundEnumerate = NULL;
	gpDSoundCreate = NULL;
	if (ghDSoundDLL)
	{
		FreeLibrary(ghDSoundDLL);
		ghDSoundDLL = NULL;
	}
	for (UINT i=0; i<MAX_DSOUND_DEVICES; i++)
	{
		if (glpDSoundGUID[i])
		{
			delete glpDSoundGUID[i];
			glpDSoundGUID[i] = NULL;
		}
	}
	gbDSoundEnumerated = FALSE;
	gnDSoundDevices = 0;
	return TRUE;
}

#endif // NO_DSOUND


BOOL SndDevInitialize()
//---------------------
{
#ifndef NO_DSOUND
	SndDevDSoundInitialize();
#endif // NO_DSOUND
#ifndef NO_PORTAUDIO
	SndDevPortaudioInitialize();
#endif // NO_PORTAUDIO
	return TRUE;
}


BOOL SndDevUninitialize()
//-----------------------
{
#ifndef NO_PORTAUDIO
	SndDevPortaudioUnnitialize();
#endif // NO_PORTAUDIO
#ifndef NO_DSOUND
	SndDevDSoundUninitialize();
#endif // NO_DSOUND
	return TRUE;
}
