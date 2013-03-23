/*
 * AudioCriticalSection.h
 * ---------
 * Purpose: Implementation of OpenMPT's critical section for access to CSoundFile.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <windows.h>

extern CRITICAL_SECTION g_csAudio;
#ifdef _DEBUG
extern int g_csAudioLockCount;
#endif

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
	CriticalSection()
	{
		inSection = false;
		Enter();
	};
	void Enter()
	{
		if(!inSection)
		{
			inSection = true;
			EnterCriticalSection(&g_csAudio);
#ifdef _DEBUG
			g_csAudioLockCount++;
#endif
		}
	};
	void Leave()
	{
		if(inSection)
		{
			inSection = false;
#ifdef _DEBUG
			g_csAudioLockCount--;
#endif
			LeaveCriticalSection(&g_csAudio);
		}
	};
	~CriticalSection()
	{
		Leave();
	};
	static void AssertUnlocked()
	{
		// asserts that the critical section is currently not hold by THIS thread
#ifdef _DEBUG
		if(TryEnterCriticalSection(&g_csAudio))
		{
			ASSERT(g_csAudioLockCount==0);
			LeaveCriticalSection(&g_csAudio);
		}
#endif
	}
};
