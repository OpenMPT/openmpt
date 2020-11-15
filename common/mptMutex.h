/*
 * mptMutex.h
 * ----------
 * Purpose: Partially implement c++ mutexes as far as openmpt needs them. Can eventually go away when we only support c++11 compilers some time.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#if MPT_CXX_AT_LEAST(20)
#include <version>
#else // !C++20
#include <array>
#endif // C++20

#if !MPT_PLATFORM_MULTITHREADED
#define MPT_MUTEX_NONE 1
#elif MPT_COMPILER_GENERIC
#define MPT_MUTEX_STD 1
#elif (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(_GLIBCXX_HAS_GTHREADS) && defined(MPT_WITH_MINGWSTDTHREADS)
#define MPT_MUTEX_STD 1
#elif (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(_GLIBCXX_HAS_GTHREADS)
#define MPT_MUTEX_WIN32 1
#else
#define MPT_MUTEX_STD 1
#endif

#ifndef MPT_MUTEX_STD
#define MPT_MUTEX_STD 0
#endif
#ifndef MPT_MUTEX_WIN32
#define MPT_MUTEX_WIN32 0
#endif
#ifndef MPT_MUTEX_NONE
#define MPT_MUTEX_NONE 0
#endif

#if defined(MODPLUG_TRACKER) && MPT_MUTEX_NONE
#error "OpenMPT requires mutexes."
#endif

#if MPT_MUTEX_STD
#if !MPT_COMPILER_GENERIC && (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(_GLIBCXX_HAS_GTHREADS) && defined(MPT_WITH_MINGWSTDTHREADS)
#include <mingw.mutex.h>
#else
#include <mutex>
#endif
#elif MPT_MUTEX_WIN32
#include <windows.h>
#endif // MPT_MUTEX

OPENMPT_NAMESPACE_BEGIN

namespace mpt {

#if MPT_MUTEX_STD

typedef std::mutex mutex;
typedef std::recursive_mutex recursive_mutex;

#elif MPT_MUTEX_WIN32

// compatible with c++11 std::mutex, can eventually be replaced without touching any usage site
class mutex {
private:
	CRITICAL_SECTION impl;
public:
	mutex() { InitializeCriticalSection(&impl); }
	~mutex() { DeleteCriticalSection(&impl); }
	void lock() { EnterCriticalSection(&impl); }
	bool try_lock() { return TryEnterCriticalSection(&impl) ? true : false; }
	void unlock() { LeaveCriticalSection(&impl); }
};

// compatible with c++11 std::recursive_mutex, can eventually be replaced without touching any usage site
class recursive_mutex {
private:
	CRITICAL_SECTION impl;
public:
	recursive_mutex() { InitializeCriticalSection(&impl); }
	~recursive_mutex() { DeleteCriticalSection(&impl); }
	void lock() { EnterCriticalSection(&impl); }
	bool try_lock() { return TryEnterCriticalSection(&impl) ? true : false; }
	void unlock() { LeaveCriticalSection(&impl); }
};

#else // MPT_MUTEX_NONE

class mutex {
public:
	mutex() { }
	~mutex() { }
	void lock() { }
	bool try_lock() { return true; }
	void unlock() { }
};

class recursive_mutex {
public:
	recursive_mutex() { }
	~recursive_mutex() { }
	void lock() { }
	bool try_lock() { return true; }
	void unlock() { }
};

#endif // MPT_MUTEX

#if MPT_MUTEX_STD

template <typename T> using lock_guard = std::lock_guard<T>;

#else // !MPT_MUTEX_STD

// compatible with c++11 std::lock_guard, can eventually be replaced without touching any usage site
template< typename mutex_type >
class lock_guard {
private:
	mutex_type & mutex;
public:
	lock_guard( mutex_type & m ) : mutex(m) { mutex.lock(); }
	~lock_guard() { mutex.unlock(); }
};

#endif // MPT_MUTEX_STD

#ifdef MODPLUG_TRACKER

class recursive_mutex_with_lock_count {

private:

	mpt::recursive_mutex mutex;

#if MPT_COMPILER_MSVC
	_Guarded_by_(mutex)
#endif // MPT_COMPILER_MSVC
	long lockCount;

public:

	recursive_mutex_with_lock_count()
		: lockCount(0)
	{
		return;
	}

	~recursive_mutex_with_lock_count()
	{
		return;
	}

#if MPT_COMPILER_MSVC
	_Acquires_lock_(mutex)
#endif // MPT_COMPILER_MSVC
	void lock()
	{
		mutex.lock();
		lockCount++;
	}

#if MPT_COMPILER_MSVC
	_Requires_lock_held_(mutex) _Releases_lock_(mutex)
#endif // MPT_COMPILER_MSVC
	void unlock()
	{
		lockCount--;
		mutex.unlock();
	}

public:

	bool IsLockedByCurrentThread() // DEBUGGING only
	{
		bool islocked = false;
		if(mutex.try_lock())
		{
			islocked = (lockCount > 0);
			mutex.unlock();
		}
		return islocked;
	}

};

#endif // MODPLUG_TRACKER

} // namespace mpt

OPENMPT_NAMESPACE_END

