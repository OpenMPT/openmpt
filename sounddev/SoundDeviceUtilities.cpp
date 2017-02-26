/*
 * SoundDeviceUtilities.cpp
 * ------------------------
 * Purpose: Sound device utilities.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"
#include "SoundDeviceUtilities.h"

#include "../common/misc_util.h"

#include <algorithm>

#if MPT_OS_WINDOWS
#include <mmsystem.h>
#endif // MPT_OS_WINDOWS

#if !MPT_OS_WINDOWS
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#ifdef _POSIX_PRIORITY_SCHEDULING // from unistd.h
#include <sched.h>
#endif
#endif

#if defined(MPT_WITH_DBUS)
#include <dbus/dbus.h>
#endif
#if defined(MPT_WITH_RTKIT)
#include "rtkit/rtkit.h"
#endif


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if MPT_OS_WINDOWS

bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat, const SoundDevice::Settings &m_Settings)
//------------------------------------------------------------------------------------------------------
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

#endif // MPT_OS_WINDOWS


#if MPT_OS_WINDOWS

CAudioThread::CAudioThread(CSoundDeviceWithThread &SoundDevice)
//-------------------------------------------------------------
	: m_SoundDevice(SoundDevice)
{
	MPT_TRACE();
	m_MMCSSClass = mpt::ToWide(m_SoundDevice.m_AppInfo.BoostedThreadMMCSSClassVista);
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


MPT_REGISTERED_COMPONENT(ComponentAvRt, "AvRt")

ComponentAvRt::ComponentAvRt()
//----------------------------
	: ComponentLibrary(ComponentTypeSystem)
	, AvSetMmThreadCharacteristicsW(nullptr)
	, AvRevertMmThreadCharacteristics(nullptr)
{
	return;
}

bool ComponentAvRt::DoInitialize()
//--------------------------------
{
	if(!mpt::Windows::Version::Current().IsAtLeast(mpt::Windows::Version::WinVista))
	{
		return false;
	}
	AddLibrary("avrt", mpt::LibraryPath::System(MPT_PATHSTRING("avrt")));
	MPT_COMPONENT_BIND("avrt", AvSetMmThreadCharacteristicsW);
	MPT_COMPONENT_BIND("avrt", AvRevertMmThreadCharacteristics);
	if(HasBindFailed())
	{
		return false;
	}
	return true;
}

ComponentAvRt::~ComponentAvRt()
//-----------------------------
{
	return;
}


CPriorityBooster::CPriorityBooster(SoundDevice::SysInfo sysInfo, ComponentHandle<ComponentAvRt> & avrt, bool boostPriority, const std::wstring & priorityClass, int priority)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	: m_SysInfo(sysInfo)
	, m_AvRt(avrt)
	, m_BoostPriority(boostPriority)
	, m_Priority(priority)
	, task_idx(0)
	, hTask(NULL)
	, oldPriority(THREAD_PRIORITY_NORMAL)
{
	MPT_TRACE();
	#ifdef _DEBUG
		m_BoostPriority = false;
	#endif
	if(m_BoostPriority)
	{
		if(m_SysInfo.WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista) && IsComponentAvailable(m_AvRt))
		{
			if(!priorityClass.empty())
			{
				hTask = m_AvRt->AvSetMmThreadCharacteristicsW(priorityClass.c_str(), &task_idx);
			}
		} else
		{
			oldPriority = GetThreadPriority(GetCurrentThread());
			SetThreadPriority(GetCurrentThread(), priority);
		}
	}
}


CPriorityBooster::~CPriorityBooster()
//-----------------------------------
{
	MPT_TRACE();
	if(m_BoostPriority)
	{
		if(m_SysInfo.WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista) && IsComponentAvailable(m_AvRt))
		{
			if(hTask)
			{
				m_AvRt->AvRevertMmThreadCharacteristics(hTask);
			}
			hTask = NULL;
			task_idx = 0;
		} else
		{
			SetThreadPriority(GetCurrentThread(), oldPriority);
		}
	}
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

	CPeriodicWaker(CAudioThread &self_, double sleepSeconds_)
	//-------------------------------------------------------
		: self(self_)
		, sleepSeconds(sleepSeconds_)
	{

		MPT_TRACE();

		sleepMilliseconds = static_cast<long>(sleepSeconds * 1000.0);
		sleep100Nanoseconds = static_cast<int64>(sleepSeconds * 10000000.0);
		if(sleepMilliseconds < 1) sleepMilliseconds = 1;
		if(sleep100Nanoseconds < 1) sleep100Nanoseconds = 1;

		period_nont_set = false;
		periodic_nt_timer = (sleep100Nanoseconds >= 10000); // can be represented as a millisecond period, otherwise use non-periodic timers which allow higher precision but might me slower because we have to set them again in each period

		sleepEvent = NULL;

		if(self.m_SoundDevice.GetSysInfo().WindowsVersion.IsNT())
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
		if(self.m_SoundDevice.GetSysInfo().WindowsVersion.IsNT())
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
		if(self.m_SoundDevice.GetSysInfo().WindowsVersion.IsNT())
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

			CPriorityBooster priorityBooster(m_SoundDevice.GetSysInfo(), m_AvRt, m_SoundDevice.m_Settings.BoostThreadPriority, m_MMCSSClass, m_SoundDevice.m_AppInfo.BoostedThreadPriorityXP);
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


bool CAudioThread::IsActive()
//---------------------------
{
	return InterlockedExchangeAdd(&m_AudioThreadActive, 0) ? true : false;
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


CSoundDeviceWithThread::CSoundDeviceWithThread(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
//--------------------------------------------------------------------------------------------------
	: SoundDevice::Base(info, sysInfo), m_AudioThread(*this)
{
	return;
}


CSoundDeviceWithThread::~CSoundDeviceWithThread()
//-----------------------------------------------
{
	return;
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

#endif // MPT_OS_WINDOWS


#if defined(MODPLUG_TRACKER) && defined(MPT_BUILD_WINESUPPORT) && !MPT_OS_WINDOWS


class ThreadPriorityGuardImpl
{

private:

	bool active;
	bool successfull;
	bool realtime;
	int niceness;
	int rt_priority;
#if defined(MPT_WITH_DBUS) && defined(MPT_WITH_RTKIT)
	DBusConnection * bus;
#endif // MPT_WITH_DBUS && MPT_WITH_RTKIT

public:

	ThreadPriorityGuardImpl(bool active, bool realtime, int niceness, int rt_priority)
		: active(active)
		, successfull(false)
		, realtime(realtime)
		, niceness(niceness)
		, rt_priority(rt_priority)
#if defined(MPT_WITH_DBUS) && defined(MPT_WITH_RTKIT)
		, bus(NULL)
#endif // MPT_WITH_DBUS && MPT_WITH_RTKIT
	{
		if(active)
		{
			if(realtime)
			{
				#ifdef _POSIX_PRIORITY_SCHEDULING
					sched_param p;
					MemsetZero(p);
					p.sched_priority = rt_priority;
					#if MPT_OS_LINUX
						if(sched_setscheduler(0, SCHED_RR|SCHED_RESET_ON_FORK, &p) == 0)
						{
							successfull = true;
						} else
						{
							#if defined(MPT_WITH_DBUS) && defined(MPT_WITH_RTKIT)
								MPT_LOG(LogNotification, "sounddev", mpt::format(MPT_USTRING("sched_setscheduler: %1"))(errno));
							#else
								MPT_LOG(LogError, "sounddev", mpt::format(MPT_USTRING("sched_setscheduler: %1"))(errno));
							#endif
						}
					#else
						if(sched_setscheduler(0, SCHED_RR, &p) == 0)
						{
							successfull = true;
						} else
						{
							#if defined(MPT_WITH_DBUS) && defined(MPT_WITH_RTKIT)
								MPT_LOG(LogNotification, "sounddev", mpt::format(MPT_USTRING("sched_setscheduler: %1"))(errno));
							#else
								MPT_LOG(LogError, "sounddev", mpt::format(MPT_USTRING("sched_setscheduler: %1"))(errno));
							#endif
						}
					#endif
				#endif
			} else
			{
				if(setpriority(PRIO_PROCESS, 0, niceness) == 0)
				{
					successfull = true;
				} else
				{
					#if defined(MPT_WITH_DBUS) && defined(MPT_WITH_RTKIT)
						MPT_LOG(LogNotification, "sounddev", mpt::format(MPT_USTRING("setpriority: %1"))(errno));
					#else
						MPT_LOG(LogError, "sounddev", mpt::format(MPT_USTRING("setpriority: %1"))(errno));
					#endif
				}
			}
			if(!successfull)
			{
				#if defined(MPT_WITH_DBUS) && defined(MPT_WITH_RTKIT)
					DBusError error;
					dbus_error_init(&error);
					bus = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
					if(!bus)
					{
						MPT_LOG(LogError, "sounddev", mpt::format(MPT_USTRING("DBus: dbus_bus_get: %1"))(mpt::ToUnicode(mpt::CharsetUTF8, error.message)));
					}
					dbus_error_free(&error);
					if(bus)
					{
						if(realtime)
						{
							int e = rtkit_make_realtime(bus, 0, rt_priority);
							if(e != 0) {
								MPT_LOG(LogError, "sounddev", mpt::format(MPT_USTRING("RtKit: rtkit_make_realtime: %1"))(e));
							} else
							{
								successfull = true;
							}
						} else
						{
							int e = rtkit_make_high_priority(bus, 0, niceness);
							if(e != 0) {
								MPT_LOG(LogError, "sounddev", mpt::format(MPT_USTRING("RtKit: rtkit_make_high_priority: %1"))(e));
							} else
							{
								successfull = true;
							}
						}
					}
				#endif // MPT_WITH_DBUS && MPT_WITH_RTKIT
			}
		}
	}

	~ThreadPriorityGuardImpl()
	{
		if(active)
		{
			#if defined(MPT_WITH_DBUS) && defined(MPT_WITH_RTKIT)
				if(bus)
				{
					// TODO: Do we want to reset priorities here?
					dbus_connection_unref(bus);
					bus = NULL;
				}
			#endif // MPT_WITH_DBUS && MPT_WITH_RTKIT
		}
	}

};


ThreadPriorityGuard::ThreadPriorityGuard(bool active, bool realtime, int niceness, int rt_priority)
	: impl(mpt::make_unique<ThreadPriorityGuardImpl>(active, realtime, niceness, rt_priority))
{
	return;
}


ThreadPriorityGuard::~ThreadPriorityGuard()
{
	return;
}


ThreadBase::ThreadBase(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
	: Base(info, sysInfo)
	, m_ThreadStopRequest(false)
{
	return;
}

bool ThreadBase::InternalStart()
{
	m_ThreadStopRequest.store(false);
	m_Thread = std::move(std::thread(&ThreadProcStatic, this));
	m_ThreadStarted.wait();
	m_ThreadStarted.post();
	return true;
}

void ThreadBase::ThreadProcStatic(ThreadBase * this_)
{
	this_->ThreadProc();
}

void ThreadBase::ThreadProc()
{
	ThreadPriorityGuard priorityGuard(m_Settings.BoostThreadPriority, m_AppInfo.BoostedThreadRealtimePosix, m_AppInfo.BoostedThreadNicenessPosix, m_AppInfo.BoostedThreadRealtimePosix);
	m_ThreadStarted.post();
	InternalStartFromSoundThread();
	while(!m_ThreadStopRequest.load())
	{
		SourceFillAudioBufferLocked();
		InternalWaitFromSoundThread();
	}
	InternalStopFromSoundThread();
}

void ThreadBase::InternalStop()
{
	m_ThreadStopRequest.store(true);
	m_Thread.join();
	m_Thread = std::move(std::thread());
	m_ThreadStopRequest.store(false);
}

ThreadBase::~ThreadBase()
{
	return;
}


#endif // MODPLUG_TRACKER && MPT_BUILD_WINESUPPORT && !MPT_OS_WINDOWS


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
