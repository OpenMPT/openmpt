#include "pfc.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/stat.h>
#endif

#ifndef _WINDOWS
#ifdef __ANDROID__
#include "cpu-features.h"
#else
#include <thread>
#endif
#endif

namespace pfc {
	t_size getOptimalWorkerThreadCount() {
#ifdef _WINDOWS
		DWORD_PTR mask,system;
		t_size ret = 0;
		GetProcessAffinityMask(GetCurrentProcess(),&mask,&system);
		for(t_size n=0;n<sizeof(mask)*8;n++) {
			if (mask & ((DWORD_PTR)1<<n)) ret++;
		}
		if (ret == 0) return 1;
		return ret;
#elif defined(__ANDROID__)
                return android_getCpuCount();
#else
		return std::thread::hardware_concurrency();
#endif


#if 0 // OSX
        size_t len;
        unsigned int ncpu;
        
        len = sizeof(ncpu);
        sysctlbyname ("hw.ncpu",&ncpu,&len,NULL,0);
        
        return ncpu;
#endif
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
    
#ifdef _WINDOWS
    thread::thread() : m_thread(INVALID_HANDLE_VALUE)
    {
    }
    
    void thread::close() {
        if (isActive()) {
            
			int ctxPriority = GetThreadPriority( GetCurrentThread() );
			if (ctxPriority > GetThreadPriority( m_thread ) ) SetThreadPriority( m_thread, ctxPriority );
            
            if (WaitForSingleObject(m_thread,INFINITE) != WAIT_OBJECT_0) crash();
            CloseHandle(m_thread); m_thread = INVALID_HANDLE_VALUE;
        }
    }
    bool thread::isActive() const {
        return m_thread != INVALID_HANDLE_VALUE;
    }
	void thread::winStart(int priority, DWORD * outThreadID) {
		close();
		HANDLE thread;
		thread = (HANDLE)_beginthreadex(NULL, 0, g_entry, reinterpret_cast<void*>(this), CREATE_SUSPENDED, (unsigned int*)outThreadID);
		if (thread == NULL) throw exception_creation();
		SetThreadPriority(thread, priority);
		ResumeThread(thread);
		m_thread = thread;
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
        
        if (pthread_create(&thread, &attr, g_entry, reinterpret_cast<void*>(this)) < 0) throw exception_creation();
        
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
}
