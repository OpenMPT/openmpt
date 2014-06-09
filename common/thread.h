/*
 * thread.h
 * --------
 * Purpose: Helper class for running threads, with a more or less platform-independent interface.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

OPENMPT_NAMESPACE_BEGIN

namespace mpt
{

#if defined(WIN32)

// Default WinAPI thread
class thread
{
protected:
	HANDLE threadHandle;

public:

	enum Priority
	{
		lowest = THREAD_PRIORITY_LOWEST,
		lower = THREAD_PRIORITY_BELOW_NORMAL,
		normal = THREAD_PRIORITY_NORMAL,
		high = THREAD_PRIORITY_ABOVE_NORMAL,
		highest = THREAD_PRIORITY_HIGHEST
	};

	operator HANDLE& () { return threadHandle; }
	operator bool () const { return threadHandle != nullptr; }

	thread() : threadHandle(nullptr) { }
	thread(LPTHREAD_START_ROUTINE function, void *userData = nullptr, Priority priority = normal)
	{
		DWORD dummy;	// For Win9x
		threadHandle = CreateThread(NULL, 0, function, userData, 0, &dummy);
		SetThreadPriority(threadHandle, priority);
	}
};


// Thread that operates on a member function
template<typename T, void (T::*Fun)()>
class thread_member : public thread
{
protected:
	static DWORD WINAPI wrapperFunc(LPVOID param)
	{
		(static_cast<T *>(param)->*Fun)();
		return 0;
	}

public:

	thread_member(T *instance, Priority priority = normal) : thread(wrapperFunc, instance, priority) { }
};

#else // !WIN32

#error "thread.h is unimplemented on non-WIN32"

#endif // !WIN32

}	// namespace mpt

OPENMPT_NAMESPACE_END
