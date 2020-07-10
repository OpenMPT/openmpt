/*
 * mptThread.h
 * -----------
 * Purpose: Helper class for running threads, with a more or less platform-independent interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#if defined(MPT_ENABLE_THREAD)

#include <thread>

#if defined(MODPLUG_TRACKER)
#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS
#endif // MODPLUG_TRACKER

#endif // MPT_ENABLE_THREAD


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_THREAD)

namespace mpt
{


#if defined(MODPLUG_TRACKER)

#if MPT_OS_WINDOWS && (MPT_COMPILER_MSVC || MPT_COMPILER_CLANG)

enum ThreadPriority
{
	ThreadPriorityLowest  = THREAD_PRIORITY_LOWEST,
	ThreadPriorityLower   = THREAD_PRIORITY_BELOW_NORMAL,
	ThreadPriorityNormal  = THREAD_PRIORITY_NORMAL,
	ThreadPriorityHigh    = THREAD_PRIORITY_ABOVE_NORMAL,
	ThreadPriorityHighest = THREAD_PRIORITY_HIGHEST
};

inline void SetCurrentThreadPriority(mpt::ThreadPriority priority)
{
	::SetThreadPriority(GetCurrentThread(), priority);
}

#else // !MPT_OS_WINDOWS

enum ThreadPriority
{
	ThreadPriorityLowest  = -2,
	ThreadPriorityLower   = -1,
	ThreadPriorityNormal  =  0,
	ThreadPriorityHigh    =  1,
	ThreadPriorityHighest =  2
};

inline void SetCurrentThreadPriority(mpt::ThreadPriority /*priority*/ )
{
	// nothing
}

#endif // MPT_OS_WINDOWS && (MPT_COMPILER_MSVC || MPT_COMPILER_CLANG)

#endif // MODPLUG_TRACKER


}	// namespace mpt

#endif // MPT_ENABLE_THREAD

OPENMPT_NAMESPACE_END
