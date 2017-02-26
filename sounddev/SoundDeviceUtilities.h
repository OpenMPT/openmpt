/*
 * SoundDeviceUtilities.h
 * ----------------------
 * Purpose: Sound device utilities.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDeviceBase.h"

#include "../common/misc_util.h"
#include "../common/ComponentManager.h"

#if MPT_OS_WINDOWS
#include <mmreg.h>
#endif // MPT_OS_WINDOWS

#if defined(MODPLUG_TRACKER) && defined(MPT_BUILD_WINESUPPORT) && !MPT_OS_WINDOWS
// we use c++11 in native support library
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#endif // MODPLUG_TRACKER && MPT_BUILD_WINESUPPORT && !MPT_OS_WINDOWS


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if MPT_OS_WINDOWS
bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat, const SoundDevice::Settings &m_Settings);
#endif // MPT_OS_WINDOWS


#if MPT_OS_WINDOWS


class CSoundDeviceWithThread;


class ComponentAvRt
	: public ComponentLibrary
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	typedef HANDLE (WINAPI *pAvSetMmThreadCharacteristicsW)(LPCWSTR, LPDWORD);
	typedef BOOL (WINAPI *pAvRevertMmThreadCharacteristics)(HANDLE);
	pAvSetMmThreadCharacteristicsW AvSetMmThreadCharacteristicsW;
	pAvRevertMmThreadCharacteristics AvRevertMmThreadCharacteristics;
public:
	ComponentAvRt();
	virtual ~ComponentAvRt();
	virtual bool DoInitialize();
};


class CPriorityBooster
{
private:
	SoundDevice::SysInfo m_SysInfo;
	ComponentHandle<ComponentAvRt> & m_AvRt;
	bool m_BoostPriority;
	int m_Priority;
	DWORD task_idx;
	HANDLE hTask;
	int oldPriority;
public:
	CPriorityBooster(SoundDevice::SysInfo sysInfo, ComponentHandle<ComponentAvRt> & avrt, bool boostPriority, const std::wstring & priorityClass, int priority);
	~CPriorityBooster();
};


class CAudioThread
{
	friend class CPeriodicWaker;
private:
	CSoundDeviceWithThread & m_SoundDevice;
	ComponentHandle<ComponentAvRt> m_AvRt;
	std::wstring m_MMCSSClass;
	double m_WakeupInterval;
	HANDLE m_hAudioWakeUp;
	HANDLE m_hPlayThread;
	HANDLE m_hAudioThreadTerminateRequest;
	HANDLE m_hAudioThreadGoneIdle;
	HANDLE m_hHardwareWakeupEvent;
	DWORD m_dwPlayThreadId;
	LONG m_AudioThreadActive;
	static DWORD WINAPI AudioThreadWrapper(LPVOID user);
	DWORD AudioThread();
	bool IsActive();
public:
	CAudioThread(CSoundDeviceWithThread &SoundDevice);
	~CAudioThread();
	void Activate();
	void Deactivate();
	void SetWakeupEvent(HANDLE ev);
	void SetWakeupInterval(double seconds);
};


class CSoundDeviceWithThread
	: public SoundDevice::Base
{
	friend class CAudioThread;
protected:
	CAudioThread m_AudioThread;
private:
	void FillAudioBufferLocked();
protected:
	void SetWakeupEvent(HANDLE ev);
	void SetWakeupInterval(double seconds);
public:
	CSoundDeviceWithThread(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	virtual ~CSoundDeviceWithThread();
	bool InternalStart();
	void InternalStop();
	virtual void StartFromSoundThread() = 0;
	virtual void StopFromSoundThread() = 0;
};


#endif // MPT_OS_WINDOWS


#if defined(MODPLUG_TRACKER) && defined(MPT_BUILD_WINESUPPORT) && !MPT_OS_WINDOWS


class semaphore {
private:
	unsigned int count;
	unsigned int waiters_count;
	std::mutex mutex;
	std::condition_variable count_nonzero;
public:
	semaphore( unsigned int initial_count = 0 )
		: count(initial_count)
		, waiters_count(0)
	{
		return;
	}
	~semaphore() {
		return;
	}
	void wait() {
		std::unique_lock<std::mutex> l(mutex);
		waiters_count++;
		while ( count == 0 ) {
			count_nonzero.wait( l );
		}
		waiters_count--;
		count--;
	}
	void post() {
		std::unique_lock<std::mutex> l(mutex);
		if ( waiters_count > 0 ) {
			count_nonzero.notify_one();
		}
		count++;
	}
	void lock() {
		wait();
	}
	void unlock() {
		post();
	}
};


class ThreadPriorityGuardImpl;

class ThreadPriorityGuard
{
private:
	std::unique_ptr<ThreadPriorityGuardImpl> impl;
public:
	ThreadPriorityGuard(bool active, bool realtime, int niceness, int rt_priority);
	~ThreadPriorityGuard();
};


class ThreadBase
	: public SoundDevice::Base
{
private:
	semaphore m_ThreadStarted;
	std::atomic<bool> m_ThreadStopRequest;
	std::thread m_Thread;
private:
	static void ThreadProcStatic(ThreadBase * this_);
	void ThreadProc();
public:
	ThreadBase(SoundDevice::Info info, SoundDevice::SysInfo sysInfo);
	virtual ~ThreadBase();
	bool InternalStart();
	void InternalStop();
	virtual void InternalStartFromSoundThread() = 0;
	virtual void InternalWaitFromSoundThread() = 0;
	virtual void InternalStopFromSoundThread() = 0;
};


#endif // MODPLUG_TRACKER && MPT_BUILD_WINESUPPORT && !MPT_OS_WINDOWS


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
