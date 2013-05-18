/*
 * mutex.h
 * -------
 * Purpose: Partially implement c++ mutexes as far as openmpt needs them. Can eventually go away when we only support c++11 compilers some time.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#ifdef MODPLUG_TRACKER

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>

namespace Util {

// compatible with c++11 std::mutex, can eventually be replaced without touching any usage site
class mutex {
private:
	CRITICAL_SECTION impl;
public:
	mutex() { InitializeCriticalSection(&impl); }
	~mutex() { DeleteCriticalSection(&impl); }
	void lock() { EnterCriticalSection(&impl); }
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
	void unlock() { LeaveCriticalSection(&impl); }
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

#endif // MODPLUG_TRACKER
