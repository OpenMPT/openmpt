/*
 * AudioCriticalSection.cpp
 * -----------
 * Purpose: Implementation of OpenMPT's critical section for access to CSoundFile.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "../common/AudioCriticalSection.h"

#include "misc_util.h"

OPENMPT_NAMESPACE_BEGIN

#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)

static CRITICAL_SECTION g_csAudio;
static int g_csAudioLockCount = 0;

namespace mpt {

void CreateGlobalCriticalSectionMutex()
//-------------------------------------
{
	MemsetZero(g_csAudio);
	InitializeCriticalSection(&g_csAudio);
}

void DestroyGlobalCriticalSectionMutex()
//--------------------------------------
{
	DeleteCriticalSection(&g_csAudio);
	MemsetZero(g_csAudio);
}

} // namespace mpt

CriticalSection::CriticalSection()
	: inSection(false)
{
	Enter();
}

CriticalSection::CriticalSection(InitialState state)
	: inSection(false)
{
	if(state == InitialLocked)
	{
		Enter();
	}
}

void CriticalSection::Enter()
{
	if(!inSection)
	{
		inSection = true;
		EnterCriticalSection(&g_csAudio);
		g_csAudioLockCount++;
	}
}

void CriticalSection::Leave()
{
	if(inSection)
	{
		inSection = false;
		g_csAudioLockCount--;
		LeaveCriticalSection(&g_csAudio);
	}
}
CriticalSection::~CriticalSection()
{
	Leave();
}

bool CriticalSection::IsLockedByCurrentThread()
{
	bool islocked = false;
	if(TryEnterCriticalSection(&g_csAudio))
	{
		islocked = (g_csAudioLockCount > 0);
		LeaveCriticalSection(&g_csAudio);
	}
	return islocked;
}

#else

MPT_MSVC_WORKAROUND_LNK4221(AudioCriticalSection)

#endif

OPENMPT_NAMESPACE_END
