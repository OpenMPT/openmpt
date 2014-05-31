/*
 * SoundDeviceThread.h
 * -------------------
 * Purpose: Sound device threading driver helpers.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDevice.h"

#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


class CSoundDeviceWithThread;


class CPriorityBooster
{
private:
	typedef HANDLE (WINAPI *FAvSetMmThreadCharacteristics)(LPCTSTR, LPDWORD);
	typedef BOOL (WINAPI *FAvRevertMmThreadCharacteristics)(HANDLE);
private:
	bool m_HasVista;
	mpt::Library m_AvRtDLL;
	FAvSetMmThreadCharacteristics pAvSetMmThreadCharacteristics;
	FAvRevertMmThreadCharacteristics pAvRevertMmThreadCharacteristics;
	bool m_BoostPriority;
	DWORD task_idx;
	HANDLE hTask;
public:
	CPriorityBooster(bool boostPriority);
	~CPriorityBooster();
};


class CAudioThread
{
	friend class CPeriodicWaker;
private:
	CSoundDeviceWithThread & m_SoundDevice;

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


class CSoundDeviceWithThread : public ISoundDevice
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
	CSoundDeviceWithThread(SoundDeviceID id, const std::wstring &internalID) : ISoundDevice(id, internalID), m_AudioThread(*this) {}
	virtual ~CSoundDeviceWithThread() {}
	bool InternalStart();
	void InternalStop();
	virtual void StartFromSoundThread() = 0;
	virtual void StopFromSoundThread() = 0;
};


OPENMPT_NAMESPACE_END
