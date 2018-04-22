/**
* @file mingw.mutex.h
* @brief std::mutex et al implementation for MinGW
** (c) 2013-2016 by Mega Limited, Auckland, New Zealand
* @author Alexander Vassilev
*
* @copyright Simplified (2-clause) BSD License.
* You should have received a copy of the license along with this
* program.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* @note
* This file may become part of the mingw-w64 runtime package. If/when this happens,
* the appropriate license will be added, i.e. this code will become dual-licensed,
* and the current BSD 2-clause license will stay.
*/

#ifndef WIN32STDMUTEX_H
#define WIN32STDMUTEX_H

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#error A C++11 compiler is required!
#endif
// Recursion checks on non-recursive locks have some performance penalty, so the user
// may want to disable the checks in release builds. In that case, make sure they
// are always enabled in debug builds.

#if defined(STDMUTEX_NO_RECURSION_CHECKS) && !defined(NDEBUG)
    #undef STDMUTEX_NO_RECURSION_CHECKS
#endif

#include <windows.h>
#include <chrono>
#include <system_error>
#include <cstdio>
#include <atomic>
#include <mutex> //need for call_once()
#include <assert.h>

//  Need for yield in spinlock and the implementation of invoke
#include "mingw.thread.h"


#ifndef EPROTO
    #define EPROTO 134
#endif
#ifndef EOWNERDEAD
    #define EOWNERDEAD 133
#endif

namespace mingw_stdthread
{
//    The _NonRecursive class has mechanisms that do not play nice with direct
//  manipulation of the native handle. This forward declaration is part of
//  a friend class declaration.
#ifndef STDMUTEX_NO_RECURSION_CHECKS
namespace vista
{
class condition_variable;
}
#endif
//    To make this namespace equivalent to the thread-related subset of std,
//  pull in the classes and class templates supplied by std but not by this
//  implementation.
using std::lock_guard;
using std::unique_lock;
using std::adopt_lock_t;
using std::defer_lock_t;
using std::try_to_lock_t;
using std::adopt_lock;
using std::defer_lock;
using std::try_to_lock;

class recursive_mutex
{
protected:
    CRITICAL_SECTION mHandle;
public:
    typedef LPCRITICAL_SECTION native_handle_type;
    native_handle_type native_handle() {return &mHandle;}
    recursive_mutex() noexcept : mHandle()
    {
        InitializeCriticalSection(&mHandle);
    }
    recursive_mutex (const recursive_mutex&) = delete;
    recursive_mutex& operator=(const recursive_mutex&) = delete;
    ~recursive_mutex() noexcept
    {
        DeleteCriticalSection(&mHandle);
    }
    void lock()
    {
        EnterCriticalSection(&mHandle);
    }
    void unlock()
    {
        LeaveCriticalSection(&mHandle);
    }
    bool try_lock()
    {
        return (TryEnterCriticalSection(&mHandle)!=0);
    }
};

struct _OwnerThread
{
//    If this is to be read before locking, then the owner-thread variable must
//  be atomic to prevent a torn read from spuriously causing errors.
    std::atomic<DWORD> mOwnerThread;
    constexpr _OwnerThread () noexcept : mOwnerThread(0) {}
    DWORD checkOwnerBeforeLock() const
    {
        DWORD self = GetCurrentThreadId();
        if (mOwnerThread.load(std::memory_order_relaxed) == self)
        {
            std::fprintf(stderr, "FATAL: Recursive locking of non-recursive mutex detected. Throwing system exception\n");
            std::fflush(stderr);
            throw std::system_error(EDEADLK, std::generic_category());
        }
        return self;
    }
    void setOwnerAfterLock(DWORD id)
    {
        mOwnerThread.store(id, std::memory_order_relaxed);
    }
    void checkSetOwnerBeforeUnlock()
    {
        DWORD self = GetCurrentThreadId();
        if (mOwnerThread.load(std::memory_order_relaxed) != self)
        {
            std::fprintf(stderr, "FATAL: Recursive unlocking of non-recursive mutex detected. Throwing system exception\n");
            std::fflush(stderr);
            throw std::system_error(EDEADLK, std::generic_category());
        }
        mOwnerThread.store(0, std::memory_order_relaxed);
    }
};

/*template <class B>
class _NonRecursive: protected B
{
protected:
#ifndef STDMUTEX_NO_RECURSION_CHECKS
//    Allow condition variable to unlock the native handle directly.
    friend class vista::condition_variable;
#endif
    typedef B base;
    _OwnerThread mOwnerThread;
public:
    using typename base::native_handle_type;
    using base::native_handle;
    constexpr _NonRecursive() noexcept :base(), mOwnerThread() {}
    _NonRecursive (const _NonRecursive<B>&) = delete;
    _NonRecursive& operator= (const _NonRecursive<B>&) = delete;
    void lock()
    {
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
        base::lock();
        mOwnerThread.setOwnerAfterLock(self);
    }
    void unlock()
    {
        mOwnerThread.checkSetOwnerBeforeUnlock();
        base::unlock();
    }
    bool try_lock()
    {
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
        bool ret = base::try_lock();
        if (ret)
            mOwnerThread.setOwnerAfterLock(self);
        return ret;
    }
};*/

//    Though the Slim Reader-Writer (SRW) locks used here are not complete until
//  Windows 7, implementing partial functionality in Vista will simplify the
//  interaction with condition variables.
#if defined(_WIN32) && (WINVER >= _WIN32_WINNT_VISTA)
namespace windows7
{
class mutex
{
    SRWLOCK mHandle;
//  Track locking thread for error checking.
#ifndef STDMUTEX_NO_RECURSION_CHECKS
    friend class vista::condition_variable;
    _OwnerThread mOwnerThread;
#endif
public:
    typedef PSRWLOCK native_handle_type;
    constexpr mutex () noexcept : mHandle(SRWLOCK_INIT)
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        , mOwnerThread()
#endif
    { }
    mutex (const mutex&) = delete;
    mutex & operator= (const mutex&) = delete;
    void lock (void)
    {
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
#endif
        AcquireSRWLockExclusive(&mHandle);
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        mOwnerThread.setOwnerAfterLock(self);
#endif
    }
    void unlock (void)
    {
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        mOwnerThread.checkSetOwnerBeforeUnlock();
#endif
        ReleaseSRWLockExclusive(&mHandle);
    }
//  TryAcquireSRW functions are a Windows 7 feature.
#if (WINVER >= _WIN32_WINNT_WIN7)
    bool try_lock (void)
    {
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
#endif
        BOOL ret = TryAcquireSRWLockExclusive(&mHandle);
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        if (ret)
            mOwnerThread.setOwnerAfterLock(self);
#endif
        return ret;
    }
#endif
    native_handle_type native_handle (void)
    {
        return &mHandle;
    }
};
} //  Namespace windows7
#endif  //  Compiling for Vista
namespace xp
{
/*
#ifndef STDMUTEX_NO_RECURSION_CHECKS
    typedef _NonRecursive<recursive_mutex> mutex;
#else
    typedef recursive_mutex mutex;
#endif*/
class mutex
{
    CRITICAL_SECTION mHandle;
    std::atomic_uchar mState;
//  Track locking thread for error checking.
#ifndef STDMUTEX_NO_RECURSION_CHECKS
    friend class vista::condition_variable;
    _OwnerThread mOwnerThread;
#endif
public:
    typedef PCRITICAL_SECTION native_handle_type;
    constexpr mutex () noexcept : mHandle(), mState(2)
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        , mOwnerThread()
#endif
    { }
    mutex (const mutex&) = delete;
    mutex & operator= (const mutex&) = delete;
    ~mutex() noexcept
    {
        DeleteCriticalSection(&mHandle);
    }
    void lock (void)
    {
        unsigned char state = mState.load(std::memory_order_acquire);
        while (state) {
            if ((state == 2) && mState.compare_exchange_weak(state, 1, std::memory_order_acquire))
            {
                InitializeCriticalSection(&mHandle);
                mState.store(0, std::memory_order_release);
                break;
            }
            if (state == 1)
                this_thread::yield();
        }
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
#endif
        EnterCriticalSection(&mHandle);
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        mOwnerThread.setOwnerAfterLock(self);
#endif
    }
    void unlock (void)
    {
        assert(mState.load(std::memory_order_relaxed) == 0);
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        mOwnerThread.checkSetOwnerBeforeUnlock();
#endif
        LeaveCriticalSection(&mHandle);
    }
    bool try_lock (void)
    {
        unsigned char state = mState.load(std::memory_order_acquire);
        if ((state == 2) && mState.compare_exchange_strong(state, 1, std::memory_order_acquire))
        {
            InitializeCriticalSection(&mHandle);
            mState.store(0, std::memory_order_release);
        }
        if (state == 1)
            return false;
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
#endif
        BOOL ret = TryEnterCriticalSection(&mHandle);
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        if (ret)
            mOwnerThread.setOwnerAfterLock(self);
#endif
        return ret;
    }
    native_handle_type native_handle (void)
    {
        return &mHandle;
    }
};
}
#if (WINVER >= _WIN32_WINNT_WIN7)
using windows7::mutex;
#else
using xp::mutex;
#endif

class recursive_timed_mutex
{
protected:
    HANDLE mHandle;
//    Track locking thread for error checking of non-recursive timed_mutex. For
//  standard compliance, this must be defined in same class and at the same
//  access-control level as every other variable in the timed_mutex.
#ifndef STDMUTEX_NO_RECURSION_CHECKS
    friend class vista::condition_variable;
    _OwnerThread mOwnerThread;
#endif
public:
    typedef HANDLE native_handle_type;
    native_handle_type native_handle() const {return mHandle;}
    recursive_timed_mutex(const recursive_timed_mutex&) = delete;
    recursive_timed_mutex& operator=(const recursive_timed_mutex&) = delete;
    recursive_timed_mutex(): mHandle(CreateMutex(NULL, FALSE, NULL))
#ifndef STDMUTEX_NO_RECURSION_CHECKS
        , mOwnerThread()
#endif
    {}
    ~recursive_timed_mutex()
    {
        CloseHandle(mHandle);
    }
    void lock()
    {
        DWORD ret = WaitForSingleObject(mHandle, INFINITE);
        if (ret != WAIT_OBJECT_0)
        {
            if (ret == WAIT_ABANDONED)
                throw std::system_error(EOWNERDEAD, std::generic_category());
            else
                throw std::system_error(EPROTO, std::generic_category());
        }
    }
    void unlock()
    {
        if (!ReleaseMutex(mHandle))
            throw std::system_error(EDEADLK, std::generic_category());
    }
    bool try_lock()
    {
        DWORD ret = WaitForSingleObject(mHandle, 0);
        if (ret == WAIT_TIMEOUT)
            return false;
        else if (ret == WAIT_OBJECT_0)
            return true;
        else if (ret == WAIT_ABANDONED)
            throw std::system_error(EOWNERDEAD, std::generic_category());
        else
            throw std::system_error(EPROTO, std::generic_category());
    }
    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep,Period>& dur)
    {
        DWORD timeout = (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

        DWORD ret = WaitForSingleObject(mHandle, timeout);
        if (ret == WAIT_TIMEOUT)
            return false;
        else if (ret == WAIT_OBJECT_0)
            return true;
        else if (ret == WAIT_ABANDONED)
            throw std::system_error(EOWNERDEAD, std::generic_category());
        else
            throw std::system_error(EPROTO, std::generic_category());
    }
    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time)
    {
        return try_lock_for(timeout_time - Clock::now());
    }
};

//  Override if, and only if, it is necessary for error-checking.
#ifndef STDMUTEX_NO_RECURSION_CHECKS
class timed_mutex: public recursive_timed_mutex
{
protected:
    typedef recursive_timed_mutex base;
public:
    using base::base;
    timed_mutex(const timed_mutex&) = delete;
    timed_mutex& operator=(const timed_mutex&) = delete;
    void lock()
    {
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
        base::lock();
        mOwnerThread.setOwnerAfterLock(self);
    }
    void unlock()
    {
        mOwnerThread.checkSetOwnerBeforeUnlock();
        base::unlock();
    }
    bool try_lock ()
    {
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
        bool ret = base::try_lock();
        if (ret)
            mOwnerThread.setOwnerAfterLock(self);
        return ret;
    }
    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep,Period>& dur)
    {
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
        bool ret = base::try_lock_for(dur);
        if (ret)
            mOwnerThread.setOwnerAfterLock(self);
        return ret;
    }
    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock,Duration>& timeout_time)
    {
        DWORD self = mOwnerThread.checkOwnerBeforeLock();
        bool ret = base::try_lock_until(timeout_time);
        if (ret)
            mOwnerThread.setOwnerAfterLock(self);
        return ret;
    }
};
#else
typedef recursive_timed_mutex timed_mutex;
#endif

class once_flag
{
    mutex mMutex;
    std::atomic_bool mHasRun;
    once_flag(const once_flag&) = delete;
    once_flag& operator=(const once_flag&) = delete;
    template<class Callable, class... Args>
    friend void call_once(once_flag& once, Callable&& f, Args&&... args);
public:
    constexpr once_flag() noexcept: mMutex(), mHasRun(false) {}
};

template<class Callable, class... Args>
void call_once(once_flag& flag, Callable&& func, Args&&... args)
{
    if (flag.mHasRun.load(std::memory_order_acquire))
        return;
    lock_guard<mutex> lock(flag.mMutex);
    if (flag.mHasRun.load(std::memory_order_acquire))
        return;
    detail::invoke(std::forward<Callable>(func),std::forward<Args>(args)...);
    flag.mHasRun.store(true, std::memory_order_release);
}
} //  Namespace mingw_stdthread

//  Push objects into std, but only if they are not already there.
namespace std
{
//    Because of quirks of the compiler, the common "using namespace std;"
//  directive would flatten the namespaces and introduce ambiguity where there
//  was none. Direct specification (std::), however, would be unaffected.
//    Take the safe option, and include only in the presence of MinGW's win32
//  implementation.
#if defined(__MINGW32__ ) && !defined(_GLIBCXX_HAS_GTHREADS)
using mingw_stdthread::recursive_mutex;
using mingw_stdthread::mutex;
using mingw_stdthread::recursive_timed_mutex;
using mingw_stdthread::timed_mutex;
using mingw_stdthread::once_flag;
using mingw_stdthread::call_once;
#elif !defined(MINGW_STDTHREAD_REDUNDANCY_WARNING)  //  Skip repetition
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#pragma message "This version of MinGW seems to include a win32 port of\
 pthreads, and probably already has C++11 std threading classes implemented,\
 based on pthreads. These classes, found in namespace std, are not overridden\
 by the mingw-std-thread library. If you would still like to use this\
 implementation (as it is more lightweight), use the classes provided in\
 namespace mingw_stdthread."
#endif
}
#endif // WIN32STDMUTEX_H
