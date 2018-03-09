/*
 * mptMutex.h
 * ----------
 * Purpose: std::mutex shim for non-multithreaded platforms
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#if MPT_PLATFORM_MULTITHREADED
#include <mutex>
#endif // MPT_PLATFORM_MULTITHREADED

OPENMPT_NAMESPACE_BEGIN

namespace mpt {

#if MPT_PLATFORM_MULTITHREADED

typedef std::mutex mutex;
typedef std::recursive_mutex recursive_mutex;

#define MPT_LOCK_GUARD std::lock_guard

#else // !MPT_PLATFORM_MULTITHREADED

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

#endif // MPT_PLATFORM_MULTITHREADED

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

} // namespace mpt

OPENMPT_NAMESPACE_END

