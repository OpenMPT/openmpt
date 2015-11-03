/*
 * AudioCriticalSection.h
 * ---------
 * Purpose: Implementation of OpenMPT's critical section for access to CSoundFile.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

OPENMPT_NAMESPACE_BEGIN

#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)

extern CRITICAL_SECTION g_csAudio;
extern int g_csAudioLockCount;

// Critical section handling done in (safe) RAII style.
// Create a CriticalSection object whenever you need exclusive access to CSoundFile.
// One object = one lock / critical section.
// The critical section is automatically left when the object is destroyed, but
// Enter() and Leave() can also be called manually if needed.
class CriticalSection
{
protected:
	bool inSection;
public:
	enum InitialState
	{
		InitialLocked = 0,
		InitialUnlocked = 1
	};
public:
	CriticalSection()
		: inSection(false)
	{
		Enter();
	}
	explicit CriticalSection(InitialState state)
		: inSection(false)
	{
		if(state == InitialLocked)
		{
			Enter();
		}
	}
	void Enter()
	{
		if(!inSection)
		{
			inSection = true;
			EnterCriticalSection(&g_csAudio);
			g_csAudioLockCount++;
		}
	}
	void Leave()
	{
		if(inSection)
		{
			inSection = false;
			g_csAudioLockCount--;
			LeaveCriticalSection(&g_csAudio);
		}
	}
	~CriticalSection()
	{
		Leave();
	}
	static bool IsLockedByCurrentThread() // DEBUGGING only
	{
		bool islocked = false;
		if(TryEnterCriticalSection(&g_csAudio))
		{
			islocked = (g_csAudioLockCount > 0);
			LeaveCriticalSection(&g_csAudio);
		}
		return islocked;
	}
};

#else // !MODPLUG_TRACKER

class CriticalSection
{
public:
	CriticalSection() {}
	void Enter() {}
	void Leave() {}
	~CriticalSection() {}
};

#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_END
