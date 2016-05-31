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


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#if MPT_OS_WINDOWS
bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat, const SoundDevice::Settings &m_Settings);
#endif // MPT_OS_WINDOWS


#if MPT_OS_WINDOWS

class CSoundDeviceWithThread;


class ComponentAvRt : public ComponentLibrary
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
	bool IsActive() { return InterlockedExchangeAdd(&m_AudioThreadActive, 0)?true:false; }
public:
	CAudioThread(CSoundDeviceWithThread &SoundDevice);
	~CAudioThread();
	void Activate();
	void Deactivate();
	void SetWakeupEvent(HANDLE ev);
	void SetWakeupInterval(double seconds);
};


class CSoundDeviceWithThread : public SoundDevice::Base
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
	CSoundDeviceWithThread(SoundDevice::Info info, SoundDevice::SysInfo sysInfo) : SoundDevice::Base(info, sysInfo), m_AudioThread(*this) {}
	virtual ~CSoundDeviceWithThread() {}
	bool InternalStart();
	void InternalStop();
	virtual void StartFromSoundThread() = 0;
	virtual void StopFromSoundThread() = 0;
};

#endif // MPT_OS_WINDOWS


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
