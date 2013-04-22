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

#ifdef MODPLUG_TRACKER
CRITICAL_SECTION g_csAudio;
int g_csAudioLockCount = 0;
#endif

