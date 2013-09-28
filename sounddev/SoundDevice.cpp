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
#include "../common/StringFixer.h"

#include "SoundDeviceASIO.h"
#include "SoundDeviceDirectSound.h"
#include "SoundDevicePortAudio.h"
#include "SoundDeviceWaveout.h"

#include <algorithm>
#include <iterator>


///////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice base class
//

ISoundDevice::ISoundDevice()
//--------------------------
{
	m_Source = nullptr;

	m_Index = 0;

	m_RealLatencyMS = static_cast<float>(m_Settings.LatencyMS);
	m_RealUpdateIntervalMS = static_cast<float>(m_Settings.UpdateIntervalMS);

	m_IsPlaying = false;
	m_SamplesRendered = 0;
}


ISoundDevice::~ISoundDevice()
//---------------------------
{
	return;
}


void ISoundDevice::SetDevice(UINT index, const std::wstring &internalID)
//----------------------------------------------------------------------
{
	m_Index = index;
	m_InternalID = internalID;
}


bool ISoundDevice::FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat)
//---------------------------------------------------------------------------
{
	MemsetZero(WaveFormat);
	if(!m_Settings.sampleFormat.IsValid()) return false;
	UINT bytespersample = (m_Settings.sampleFormat.GetBitsPerSample()/8) * m_Settings.Channels;
	WaveFormat.Format.wFormatTag = m_Settings.sampleFormat.IsFloat() ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	WaveFormat.Format.nChannels = (WORD)m_Settings.Channels;
	WaveFormat.Format.nSamplesPerSec = m_Settings.Samplerate;
	WaveFormat.Format.nAvgBytesPerSec = m_Settings.Samplerate * bytespersample;
	WaveFormat.Format.nBlockAlign = (WORD)bytespersample;
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


bool ISoundDevice::Open(const SoundDeviceSettings &settings)
//----------------------------------------------------------
{
	m_Settings = settings;
	if(m_Settings.LatencyMS < SNDDEV_MINLATENCY_MS) m_Settings.LatencyMS = SNDDEV_MINLATENCY_MS;
	if(m_Settings.LatencyMS > SNDDEV_MAXLATENCY_MS) m_Settings.LatencyMS = SNDDEV_MAXLATENCY_MS;
	if(m_Settings.UpdateIntervalMS < SNDDEV_MINUPDATEINTERVAL_MS) m_Settings.UpdateIntervalMS = SNDDEV_MINUPDATEINTERVAL_MS;
	if(m_Settings.UpdateIntervalMS > SNDDEV_MAXUPDATEINTERVAL_MS) m_Settings.UpdateIntervalMS = SNDDEV_MAXUPDATEINTERVAL_MS;
	m_RealLatencyMS = static_cast<float>(m_Settings.LatencyMS);
	m_RealUpdateIntervalMS = static_cast<float>(m_Settings.UpdateIntervalMS);
	return InternalOpen();
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
	if(NumSamples <= 0)
	{
		return;
	}
	m_Source->AudioRead(pData, NumSamples, m_Settings.sampleFormat);
}


void ISoundDevice::SourceAudioDone(ULONG NumSamples, ULONG SamplesLatency)
//------------------------------------------------------------------------
{
	if(NumSamples <= 0)
	{
		return;
	}
	int64 samplesRendered = 0;
	{
		Util::lock_guard<Util::mutex> lock(m_SamplesRenderedMutex);
		m_SamplesRendered += NumSamples;
		samplesRendered = m_SamplesRendered;
	}
	if(InternalHasGetStreamPosition())
	{
		m_Source->AudioDone(NumSamples, samplesRendered);
	} else
	{
		m_Source->AudioDone(NumSamples, samplesRendered + SamplesLatency);
	}
}


void ISoundDevice::Start()
//------------------------
{
	if(!IsOpen()) return; 
	if(!IsPlaying())
	{
		{
			Util::lock_guard<Util::mutex> lock(m_SamplesRenderedMutex);
			m_SamplesRendered = 0;
		}
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
		{
			Util::lock_guard<Util::mutex> lock(m_SamplesRenderedMutex);
			m_SamplesRendered = 0;
		}
	}
}


void ISoundDevice::Reset()
//------------------------
{
	if(!IsOpen()) return;
	Stop();
	InternalReset();
}


int64 ISoundDevice::GetStreamPositionSamples() const
//--------------------------------------------------
{
	if(InternalHasGetStreamPosition())
	{
		return InternalGetStreamPositionSamples();
	} else
	{
		Util::lock_guard<Util::mutex> lock(m_SamplesRenderedMutex);
		return m_SamplesRendered;
	}
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

			CPriorityBooster priorityBooster(*this, (m_SoundDevice.m_Settings.fulCfgOptions & SNDDEV_OPTIONS_BOOSTTHREADPRIORITY)?true:false);
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
// Global Functions
//


void SoundDevicesManager::ReEnumerate()
//-------------------------------------
{
	m_SoundDevices.clear();
	for(int type = 0; type < SNDDEV_NUM_DEVTYPES; ++type)
	{
		std::vector<SoundDeviceInfo> infos;
		switch(type)
		{

		case SNDDEV_WAVEOUT:
			infos = CWaveDevice::EnumerateDevices();
			break;

#ifndef NO_DSOUND
		case SNDDEV_DSOUND:
			infos = CDSoundDevice::EnumerateDevices();
			break;
#endif // NO_DSOUND

#ifndef NO_ASIO
		case SNDDEV_ASIO:
			infos = CASIODevice::EnumerateDevices();
			break;
#endif // NO_ASIO

#ifndef NO_PORTAUDIO
		case SNDDEV_PORTAUDIO_WASAPI:
		case SNDDEV_PORTAUDIO_WDMKS:
		case SNDDEV_PORTAUDIO_WMME:
		case SNDDEV_PORTAUDIO_DS:
		case SNDDEV_PORTAUDIO_ASIO:
			infos = CPortaudioDevice::EnumerateDevices(type);
			break;
#endif // NO_PORTAUDIO

		}
		std::copy(infos.begin(), infos.end(), std::back_inserter(m_SoundDevices));
	}
}


const SoundDeviceInfo * SoundDevicesManager::FindDeviceInfo(UINT type, UINT index) const
//--------------------------------------------------------------------------------------
{
	for(std::vector<SoundDeviceInfo>::const_iterator it = begin(); it != end(); ++it)
	{
		if(it->type == type && it->index == index)
		{
			return &(*it);
		}
	}
	return nullptr;
}


ISoundDevice * SoundDevicesManager::CreateSoundDevice(UINT type, UINT index)
//--------------------------------------------------------------------------
{
	const SoundDeviceInfo *info = FindDeviceInfo(type, index);
	if(!info)
	{
		return nullptr;
	}
	ISoundDevice *result = nullptr;
	switch(type)
	{
	case SNDDEV_WAVEOUT: result = new CWaveDevice(); break;
#ifndef NO_DSOUND
	case SNDDEV_DSOUND: result = new CDSoundDevice(); break;
#endif // NO_DSOUND
#ifndef NO_ASIO
	case SNDDEV_ASIO: result = new CASIODevice(); break;
#endif // NO_ASIO
#ifndef NO_PORTAUDIO
	case SNDDEV_PORTAUDIO_WASAPI:
	case SNDDEV_PORTAUDIO_WDMKS:
	case SNDDEV_PORTAUDIO_WMME:
	case SNDDEV_PORTAUDIO_DS:
	case SNDDEV_PORTAUDIO_ASIO:
		result = SndDevPortaudioIsInitialized() ? new CPortaudioDevice(CPortaudioDevice::SndDevTypeToHostApi(type)) : nullptr;
		break;
#endif // NO_PORTAUDIO
	}
	if(result)
	{
		result->SetDevice(index, info->internalID);
	}
	return result;
}


SoundDevicesManager::SoundDevicesManager()
//----------------------------------------
{
#ifndef NO_PORTAUDIO
	SndDevPortaudioInitialize();
#endif // NO_PORTAUDIO
	ReEnumerate();
}


SoundDevicesManager::~SoundDevicesManager()
//-----------------------------------------
{
#ifndef NO_PORTAUDIO
	SndDevPortaudioUnnitialize();
#endif // NO_PORTAUDIO
}
