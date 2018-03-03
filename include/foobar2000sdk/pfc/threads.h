#pragma once

#include <functional>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif
namespace pfc {
	t_size getOptimalWorkerThreadCount();
	t_size getOptimalWorkerThreadCountEx(t_size taskCountLimit);

	//! IMPORTANT: all classes derived from thread must call waitTillDone() in their destructor, to avoid object destruction during a virtual function call!
	class thread {
	public:

		//! Critical error handler function, never returns
		PFC_NORETURN static void couldNotCreateThread();

		thread();
		~thread() {PFC_ASSERT(!isActive()); waitTillDone();}
		void startWithPriority(int priority);
		void setPriority(int priority);
        int getPriority();
		void start();
		bool isActive() const;
		void waitTillDone() {close();}
        static int currentPriority();
#ifdef _WIN32
		void winStart(int priority, DWORD * outThreadID); 
		HANDLE winThreadHandle() { return m_thread; }
#else
		pthread_t posixThreadHandle() { return m_thread; }
#endif
#ifdef __APPLE__
		static void appleStartThreadPrologue();
#endif
	protected:
		virtual void threadProc() {PFC_ASSERT(!"Stub thread entry - should not get here");}
	private:
		void close();
#ifdef _WIN32
		static unsigned CALLBACK g_entry(void* p_instance);
#else
		static void * g_entry( void * arg );
#endif
        void entry();
        
#ifdef _WIN32
		HANDLE m_thread;
#else
        pthread_t m_thread;
        bool m_threadValid; // there is no invalid pthread_t, so we keep a separate 'valid' flag
#endif
		PFC_CLASS_NOT_COPYABLE_EX(thread)
	};

	//! Thread class using lambda entrypoint rather than function override
	class thread2 : public thread {
	public:
		void startHereWithPriority(std::function<void()> e, int priority);
		void startHere(std::function<void()> e);
		void setEntry(std::function<void()> e);
	private:
		void threadProc();

		std::function<void()> m_entryPoint;
	};

	void splitThread(std::function<void() > f);

	//! Apple specific; executes the function in a release pool scope. \n
	//! On non Apple platforms it just invokes the function.
	void inAutoReleasePool(std::function<void()> f);
}
