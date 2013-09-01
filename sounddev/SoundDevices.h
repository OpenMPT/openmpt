/*
 * SoundDevices.h
 * --------------
 * Purpose: Sound device driver helpers.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDevice.h"


class CSoundDeviceWithThread;

class CAudioThread
{
	friend class CPriorityBooster;
	friend class CPeriodicWaker;
private:
	CSoundDeviceWithThread & m_SoundDevice;

	bool m_HasXP;
	bool m_HasVista;
	HMODULE m_hKernel32DLL;
	HMODULE m_hAvRtDLL;
	typedef HANDLE (WINAPI *FCreateWaitableTimer)(LPSECURITY_ATTRIBUTES, BOOL, LPCTSTR);
	typedef BOOL (WINAPI *FSetWaitableTimer)(HANDLE, const LARGE_INTEGER *, LONG, PTIMERAPCROUTINE, LPVOID, BOOL);
	typedef BOOL (WINAPI *FCancelWaitableTimer)(HANDLE);
	typedef HANDLE (WINAPI *FAvSetMmThreadCharacteristics)(LPCTSTR, LPDWORD);
	typedef BOOL (WINAPI *FAvRevertMmThreadCharacteristics)(HANDLE);
	FCreateWaitableTimer pCreateWaitableTimer;
	FSetWaitableTimer pSetWaitableTimer;
	FCancelWaitableTimer pCancelWaitableTimer;
	FAvSetMmThreadCharacteristics pAvSetMmThreadCharacteristics;
	FAvRevertMmThreadCharacteristics pAvRevertMmThreadCharacteristics;

	HANDLE m_hAudioWakeUp;
	HANDLE m_hPlayThread;
	HANDLE m_hAudioThreadTerminateRequest;
	HANDLE m_hAudioThreadGoneIdle;
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
};


class CSoundDeviceWithThread : public ISoundDevice
{
	friend class CAudioThread;
protected:
	CAudioThread m_AudioThread;
private:
	void FillAudioBufferLocked();
public:
	CSoundDeviceWithThread() : m_AudioThread(*this) {}
	virtual ~CSoundDeviceWithThread() {}
	void InternalStart();
	void InternalStop();
	void InternalReset();
	virtual void StartFromSoundThread() = 0;
	virtual void StopFromSoundThread() = 0;
	virtual void ResetFromOutsideSoundThread() = 0;
};

