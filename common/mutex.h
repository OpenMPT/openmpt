/*
 * mutex.h
 * -------
 * Purpose: Partially implement c++ mutexes as far as openmpt needs them. Can eventually go away when we only support c++11 compilers some time.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#if defined(MPT_ENABLE_THREAD)
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#else
#include <pthread.h>
#endif
#endif // MPT_ENABLE_THREAD

OPENMPT_NAMESPACE_BEGIN

#if defined(MPT_ENABLE_THREAD)

namespace Util {

#ifdef _WIN32

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

#else // !_WIN32

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

#endif // _WIN32

class recursive_mutex_with_lock_count {
private:
	Util::recursive_mutex mutex;
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

// compatible with c++11 std::lock_guard, can eventually be replaced without touching any usage site
template< typename mutex_type >
class lock_guard {
private:
	mutex_type & mutex;
public:
	lock_guard( mutex_type & m ) : mutex(m) { mutex.lock(); }
	~lock_guard() { mutex.unlock(); }
};

} // namespace Util

#endif // MPT_ENABLE_THREAD

OPENMPT_NAMESPACE_END
