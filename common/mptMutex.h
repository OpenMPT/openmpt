/*
 * mptMutex.h
 * ----------
 * Purpose: Partially implement c++ mutexes as far as openmpt needs them. Can eventually go away when we only support c++11 compilers some time.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#if defined(MPT_ENABLE_THREADSAFE)
#if MPT_COMPILER_GENERIC || MPT_MSVC_AT_LEAST(2012,0) || MPT_GCC_AT_LEAST(4,4,0) || MPT_CLANG_AT_LEAST(3,6,0)
#define MPT_STD_MUTEX 1
#else
#define MPT_STD_MUTEX 0
#endif
#if MPT_STD_MUTEX
#include <mutex>
#else // !MPT_STD_MUTEX
#if MPT_OS_WINDOWS
#include <windows.h>
#else // !MPT_OS_WINDOWS
#include <pthread.h>
#endif // MPT_OS_WINDOWS
#endif // MPT_STD_MUTEX
#endif // MPT_ENABLE_THREADSAFE

OPENMPT_NAMESPACE_BEGIN

namespace mpt {

#if defined(MPT_ENABLE_THREADSAFE)

#if MPT_STD_MUTEX

typedef std::_Mutex mutex;
typedef std::recursive_mutex recursive_mutex;

#else // MPT_STD_MUTEX

#if MPT_OS_WINDOWS

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

#else // !MPT_OS_WINDOWS

class mutex {
private:
	pthread_mutex_t hLock;
public:
	mutex()
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
		pthread_mutex_init(&hLock, &attr);
		pthread_mutexattr_destroy(&attr);
	}
	~mutex() { pthread_mutex_destroy(&hLock); }
	void lock() { pthread_mutex_lock(&hLock); }
	bool try_lock() { return (pthread_mutex_trylock(&hLock) == 0); }
	void unlock() { pthread_mutex_unlock(&hLock); }
};

class recursive_mutex {
private:
	pthread_mutex_t hLock;
public:
	recursive_mutex()
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&hLock, &attr);
		pthread_mutexattr_destroy(&attr);
	}
	~recursive_mutex() { pthread_mutex_destroy(&hLock); }
	void lock() { pthread_mutex_lock(&hLock); }
	bool try_lock() { return (pthread_mutex_trylock(&hLock) == 0); }
	void unlock() { pthread_mutex_unlock(&hLock); }
};

#endif // MPT_OS_WINDOWS

#endif // MPT_STD_MUTEX

#else // !MPT_ENABLE_THREADSAFE

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

#endif // MPT_ENABLE_THREADSAFE

#if defined(MPT_ENABLE_THREADSAFE) && MPT_STD_MUTEX

#define MPT_LOCK_GUARD std::lock_guard

#else //!(MPT_ENABLE_THREADSAFE && MPT_STD_MUTEX)

// compatible with c++11 std::lock_guard, can eventually be replaced without touching any usage site
template< typename mutex_type >
class lock_guard {
private:
	mutex_type & mutex;
public:
	lock_guard( mutex_type & m ) : mutex(m) { mutex.lock(); }
	~lock_guard() { mutex.unlock(); }
};

#define MPT_LOCK_GUARD mpt::lock_guard

#endif // MPT_ENABLE_THREADSAFE && MPT_STD_MUTEX

#if defined(MPT_ENABLE_THREADSAFE)

#ifdef MODPLUG_TRACKER

class recursive_mutex_with_lock_count {
private:
	mpt::recursive_mutex mutex;
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
	void lock()
	{
		mutex.lock();
		lockCount++;
	}
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

#endif // MPT_ENABLE_THREADSAFE

} // namespace mpt

OPENMPT_NAMESPACE_END
