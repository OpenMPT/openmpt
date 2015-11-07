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

namespace mpt {
void CreateGlobalCriticalSectionMutex();
void DestroyGlobalCriticalSectionMutex();
} // namespace mpt

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
	CriticalSection();
	explicit CriticalSection(InitialState state);
	void Enter();
	void Leave();
	~CriticalSection();
public:
	static bool IsLockedByCurrentThread(); // DEBUGGING only
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
