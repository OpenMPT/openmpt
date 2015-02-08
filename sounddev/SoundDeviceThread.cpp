/*
 * SoundDeviceThread.cpp
 * ---------------------
 * Purpose: Sound device threading driver helpers.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"
#include "SoundDeviceThread.h"

#include "../common/misc_util.h"

#include <algorithm>

#include <mmsystem.h>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


CAudioThread::CAudioThread(CSoundDeviceWithThread &SoundDevice) : m_SoundDevice(SoundDevice)
//------------------------------------------------------------------------------------------
{
	MPT_TRACE();
	m_WakeupInterval = 0.0;
	m_hPlayThread = NULL;
	m_dwPlayThreadId = 0;
	m_hAudioWakeUp = NULL;
	m_hAudioThreadTerminateRequest = NULL;
	m_hAudioThreadGoneIdle = NULL;
	m_hHardwareWakeupEvent = INVALID_HANDLE_VALUE;
	m_AudioThreadActive = 0;
	m_hAudioWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hAudioThreadTerminateRequest = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hAudioThreadGoneIdle = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hPlayThread = CreateThread(NULL, 0, AudioThreadWrapper, (LPVOID)this, 0, &m_dwPlayThreadId);
}


CAudioThread::~CAudioThread()
//---------------------------
{
	MPT_TRACE();
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
}


CPriorityBooster::CPriorityBooster(bool boostPriority)
//----------------------------------------------------
	: m_HasVista(false)
	, pAvSetMmThreadCharacteristics(nullptr)
	, pAvRevertMmThreadCharacteristics(nullptr)
	, m_BoostPriority(boostPriority)
	, task_idx(0)
	, hTask(NULL)
{

	MPT_TRACE();

	m_HasVista = mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinVista);

	if(m_HasVista)
	{
		m_AvRtDLL = mpt::Library(mpt::LibraryPath::System(MPT_PATHSTRING("avrt")));
		if(m_AvRtDLL.IsValid())
		{
			if(!m_AvRtDLL.Bind(pAvSetMmThreadCharacteristics, "AvSetMmThreadCharacteristicsA"))
			{
				m_HasVista = false;
			}
			if(!m_AvRtDLL.Bind(pAvRevertMmThreadCharacteristics, "AvRevertMmThreadCharacteristics"))
			{
				m_HasVista = false;
			}
		} else
		{
			m_HasVista = false;
		}
	}

	#ifdef _DEBUG
		m_BoostPriority = false;
	#endif

	if(m_BoostPriority)
	{
		if(m_HasVista)
		{
			hTask = pAvSetMmThreadCharacteristics(TEXT("Pro Audio"), &task_idx);
		} else
		{
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		}
	}

}


CPriorityBooster::~CPriorityBooster()
//-----------------------------------
{

	MPT_TRACE();

	if(m_BoostPriority)
	{
		if(m_HasVista)
		{
			pAvRevertMmThreadCharacteristics(hTask);
			hTask = NULL;
			task_idx = 0;
		} else
		{
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		}
	}

	pAvRevertMmThreadCharacteristics = nullptr;
	pAvSetMmThreadCharacteristics = nullptr;

}


class CPeriodicWaker
{
private:
	CAudioThread &self;

	double sleepSeconds;
	long sleepMilliseconds;
	int64 sleep100Nanoseconds;

	Util::MultimediaClock clock_noxp;

	bool period_nont_set;
	bool periodic_nt_timer;

	HANDLE sleepEvent;

public:

	CPeriodicWaker(CAudioThread &self_, double sleepSeconds_) : self(self_), sleepSeconds(sleepSeconds_)
	//--------------------------------------------------------------------------------------------------
	{

		MPT_TRACE();

		sleepMilliseconds = static_cast<long>(sleepSeconds * 1000.0);
		sleep100Nanoseconds = static_cast<int64>(sleepSeconds * 10000000.0);
		if(sleepMilliseconds < 1) sleepMilliseconds = 1;
		if(sleep100Nanoseconds < 1) sleep100Nanoseconds = 1;

		period_nont_set = false;
		periodic_nt_timer = (sleep100Nanoseconds >= 10000); // can be represented as a millisecond period, otherwise use non-periodic timers which allow higher precision but might me slower because we have to set them again in each period

		sleepEvent = NULL;

		if(mpt::Windows::Version::IsNT())
		{
			if(periodic_nt_timer)
			{
				sleepEvent = CreateWaitableTimer(NULL, FALSE, NULL);
				LARGE_INTEGER dueTime;
				dueTime.QuadPart = 0 - sleep100Nanoseconds; // negative time means relative
				SetWaitableTimer(sleepEvent, &dueTime, sleepMilliseconds, NULL, NULL, FALSE);
			} else
			{
				sleepEvent = CreateWaitableTimer(NULL, TRUE, NULL);
			}
		} else
		{
			sleepEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			clock_noxp.SetResolution(1); // increase resolution of multimedia timer
			period_nont_set = true;
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
		MPT_TRACE();
		if(mpt::Windows::Version::IsNT())
		{
			if(!periodic_nt_timer)
			{
				LARGE_INTEGER dueTime;
				dueTime.QuadPart = 0 - sleep100Nanoseconds; // negative time means relative
				SetWaitableTimer(sleepEvent, &dueTime, 0, NULL, NULL, FALSE);
			}
		} else
		{
			timeSetEvent(sleepMilliseconds, 1, (LPTIMECALLBACK)sleepEvent, NULL, TIME_ONESHOT | TIME_CALLBACK_EVENT_SET);
		}
	}

	CPeriodicWaker::~CPeriodicWaker()
	//-------------------------------
	{
		MPT_TRACE();
		if(mpt::Windows::Version::IsNT())
		{
			if(periodic_nt_timer)
			{
				CancelWaitableTimer(sleepEvent);
			}
			CloseHandle(sleepEvent);
			sleepEvent = NULL;
		} else
		{
			if(period_nont_set)
			{
				clock_noxp.SetResolution(0);
				period_nont_set = false;
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
	MPT_TRACE();

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

			CPriorityBooster priorityBooster(m_SoundDevice.m_Settings.BoostThreadPriority);
			CPeriodicWaker periodicWaker(*this, m_WakeupInterval);

			m_SoundDevice.StartFromSoundThread();

			while(!terminate && IsActive())
			{

				m_SoundDevice.FillAudioBufferLocked();

				periodicWaker.Retrigger();

				if(m_hHardwareWakeupEvent != INVALID_HANDLE_VALUE)
				{
					HANDLE waithandles[4] = {m_hAudioThreadTerminateRequest, m_hAudioWakeUp, m_hHardwareWakeupEvent, periodicWaker.GetWakeupEvent()};
					switch(WaitForMultipleObjects(4, waithandles, FALSE, periodicWaker.GetSleepMilliseconds()))
					{
					case WAIT_OBJECT_0:
						terminate = true;
						break;
					}
				} else
				{
					HANDLE waithandles[3] = {m_hAudioThreadTerminateRequest, m_hAudioWakeUp, periodicWaker.GetWakeupEvent()};
					switch(WaitForMultipleObjects(3, waithandles, FALSE, periodicWaker.GetSleepMilliseconds()))
					{
					case WAIT_OBJECT_0:
						terminate = true;
						break;
					}
				}

			}

			m_SoundDevice.StopFromSoundThread();

		}

	}

	SetEvent(m_hAudioThreadGoneIdle);

	return 0;

}


void CAudioThread::SetWakeupEvent(HANDLE ev)
//------------------------------------------
{
	MPT_TRACE();
	m_hHardwareWakeupEvent = ev;
}


void CAudioThread::SetWakeupInterval(double seconds)
//--------------------------------------------------
{
	MPT_TRACE();
	m_WakeupInterval = seconds;
}


void CAudioThread::Activate()
//---------------------------
{
	MPT_TRACE();
	if(InterlockedExchangeAdd(&m_AudioThreadActive, 0))
	{
		MPT_ASSERT_ALWAYS(false);
		return;
	}
	ResetEvent(m_hAudioThreadGoneIdle);
	InterlockedExchange(&m_AudioThreadActive, 1);
	SetEvent(m_hAudioWakeUp);
}


void CAudioThread::Deactivate()
//-----------------------------
{
	MPT_TRACE();
	if(!InterlockedExchangeAdd(&m_AudioThreadActive, 0))
	{
		MPT_ASSERT_ALWAYS(false);
		return;
	}
	InterlockedExchange(&m_AudioThreadActive, 0);
	WaitForSingleObject(m_hAudioThreadGoneIdle, INFINITE);
}


void CSoundDeviceWithThread::FillAudioBufferLocked()
//--------------------------------------------------
{
	MPT_TRACE();
	SourceFillAudioBufferLocked();
}


void CSoundDeviceWithThread::SetWakeupEvent(HANDLE ev)
//----------------------------------------------------
{
	MPT_TRACE();
	m_AudioThread.SetWakeupEvent(ev);
}


void CSoundDeviceWithThread::SetWakeupInterval(double seconds)
//------------------------------------------------------------
{
	MPT_TRACE();
	m_AudioThread.SetWakeupInterval(seconds);
}


bool CSoundDeviceWithThread::InternalStart()
//------------------------------------------
{
	MPT_TRACE();
	m_AudioThread.Activate();
	return true;
}


void CSoundDeviceWithThread::InternalStop()
//-----------------------------------------
{
	MPT_TRACE();
	m_AudioThread.Deactivate();
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
