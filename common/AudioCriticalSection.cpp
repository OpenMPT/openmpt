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

OPENMPT_NAMESPACE_BEGIN

#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)
CRITICAL_SECTION g_csAudio;
int g_csAudioLockCount = 0;
#else
MPT_MSVC_WORKAROUND_LNK4221(AudioCriticalSection)
#endif

OPENMPT_NAMESPACE_END
