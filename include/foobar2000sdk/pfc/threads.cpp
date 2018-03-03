#include "pfc.h"

#ifdef _WIN32
#include "pp-winapi.h"
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/stat.h>
#endif

#include <thread>

namespace pfc {
	t_size getOptimalWorkerThreadCount() {
		return std::thread::hardware_concurrency();
	}

	t_size getOptimalWorkerThreadCountEx(t_size taskCountLimit) {
		if (taskCountLimit <= 1) return 1;
		return pfc::min_t(taskCountLimit,getOptimalWorkerThreadCount());
	}
    
    
    void thread::entry() {
        try {
            threadProc();
        } catch(...) {}
    }

	void thread::couldNotCreateThread() {
		// Something terminally wrong, better crash leaving a good debug trace
		crash();
	}
    
#ifdef _WIN32
    thread::thread() : m_thread(INVALID_HANDLE_VALUE)
    {
    }
    
    void thread::close() {
        if (isActive()) {
            
			int ctxPriority = GetThreadPriority( GetCurrentThread() );
			if (ctxPriority > GetThreadPriority( m_thread ) ) SetThreadPriority( m_thread, ctxPriority );
            
            if (WaitForSingleObject(m_thread,INFINITE) != WAIT_OBJECT_0) couldNotCreateThread();
            CloseHandle(m_thread); m_thread = INVALID_HANDLE_VALUE;
        }
    }
    bool thread::isActive() const {
        return m_thread != INVALID_HANDLE_VALUE;
    }

	static HANDLE MyBeginThread( unsigned (__stdcall * proc)(void *) , void * arg, DWORD * outThreadID, int priority) {
		HANDLE thread;
#ifdef PFC_WINDOWS_DESKTOP_APP
		thread = (HANDLE)_beginthreadex(NULL, 0, proc, arg, CREATE_SUSPENDED, (unsigned int*)outThreadID);
#else
		thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, arg, CREATE_SUSPENDED, outThreadID);
#endif
		if (thread == NULL) thread::couldNotCreateThread();
		SetThreadPriority(thread, priority);
		ResumeThread(thread);
		return thread;
	}
	void thread::winStart(int priority, DWORD * outThreadID) {
		close();
		m_thread = MyBeginThread(g_entry, reinterpret_cast<void*>(this), outThreadID, priority);
	}
    void thread::startWithPriority(int priority) {
		winStart(priority, NULL);
    }
    void thread::setPriority(int priority) {
        PFC_ASSERT(isActive());
        SetThreadPriority(m_thread, priority);
    }
    void thread::start() {
        startWithPriority(GetThreadPriority(GetCurrentThread()));
    }
    
    unsigned CALLBACK thread::g_entry(void* p_instance) {
        reinterpret_cast<thread*>(p_instance)->entry(); return 0;
    }
    
    int thread::getPriority() {
        PFC_ASSERT(isActive());
        return GetThreadPriority( m_thread );
    }

    int thread::currentPriority() {
        return GetThreadPriority( GetCurrentThread() );
    }
#else
    thread::thread() : m_thread(), m_threadValid() {
    }
    
#ifndef __APPLE__ // Apple specific entrypoint in obj-c.mm
    void * thread::g_entry( void * arg ) {
        reinterpret_cast<thread*>( arg )->entry();
        return NULL;
    }
#endif
    
    void thread::startWithPriority(int priority) {
        close();
#ifdef __APPLE__
        appleStartThreadPrologue();
#endif
        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        
        if (pthread_create(&thread, &attr, g_entry, reinterpret_cast<void*>(this)) < 0) couldNotCreateThread();
        
        pthread_attr_destroy(&attr);
        m_threadValid = true;
        m_thread = thread;
    }
    
    void thread::setPriority(int priority) {
        PFC_ASSERT(isActive());
        // not avail
    }
    int thread::getPriority() {
        return 0; // not avail
    }
    int thread::currentPriority() {
        return 0; // not avail
    }
    
    void thread::start() {
        startWithPriority(currentPriority());
    }
    
    void thread::close() {
        if (m_threadValid) {
            void * rv = NULL;
            pthread_join( m_thread, & rv ); m_thread = 0;
            m_threadValid = false;
        }
    }
    
    bool thread::isActive() const {
        return m_threadValid;
    }
#endif
#ifdef _WIN32
    unsigned CALLBACK winSplitThreadProc(void* arg) {
		auto f = reinterpret_cast<std::function<void() > *>(arg);
		(*f)();
		delete f;
		return 0;
    }
#else
	void * nixSplitThreadProc(void * arg) {
		auto f = reinterpret_cast<std::function<void() > *>(arg);
#ifdef __APPLE__
		inAutoReleasePool([f] {
			(*f)();
			delete f;
		});
#else
		(*f)();
		delete f;
#endif
		return NULL;
	}
#endif

	void splitThread(std::function<void() > f) {
		ptrholder_t< std::function<void() > > arg ( new std::function<void() >(f) );
#ifdef _WIN32
		HANDLE h = MyBeginThread(winSplitThreadProc, arg.get_ptr(), NULL, GetThreadPriority(GetCurrentThread()));
		CloseHandle(h);
#else
#ifdef __APPLE__
		thread::appleStartThreadPrologue();
#endif
		pthread_t thread;
		
        if (pthread_create(&thread, NULL, nixSplitThreadProc, arg.get_ptr()) < 0) thread::couldNotCreateThread();

		pthread_detach(thread);
#endif
		arg.detach();
	}
#ifndef __APPLE__
	// Stub for non Apple
	void inAutoReleasePool(std::function<void()> f) { f(); }
#endif


	void thread2::startHereWithPriority(std::function<void()> e, int priority) {
		setEntry(e); startWithPriority(priority);
	}
	void thread2::startHere(std::function<void()> e) {
		setEntry(e); start();
	}
	void thread2::setEntry(std::function<void()> e) {
		PFC_ASSERT(!isActive());
		m_entryPoint = e;
	}

	void thread2::threadProc() {
		m_entryPoint();
	}
}
