/*
 * mptMutex.h
 * ----------
 * Purpose: Partially implement c++ mutexes as far as openmpt needs them. Can eventually go away when we only support c++11 compilers some time.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/mutex/mutex.hpp"

OPENMPT_NAMESPACE_BEGIN

namespace mpt {

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

} // namespace mpt

OPENMPT_NAMESPACE_END

