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
		PFC_DECLARE_EXCEPTION(exception_creation, exception, "Could not create thread");
		thread();
		~thread() {PFC_ASSERT(!isActive()); waitTillDone();}
		void startWithPriority(int priority);
		void setPriority(int priority);
        int getPriority();
		void start();
		bool isActive() const;
		void waitTillDone() {close();}
        static int currentPriority();
#ifdef _WINDOWS
		void winStart(int priority, DWORD * outThreadID); 
		HANDLE winThreadHandle() { return m_thread; }
#endif
	protected:
		virtual void threadProc() {PFC_ASSERT(!"Stub thread entry - should not get here");}
	private:
		void close();
#ifdef _WINDOWS
		static unsigned CALLBACK g_entry(void* p_instance);
#else
		static void * g_entry( void * arg );
#endif
        void entry();
        
#ifdef _WINDOWS
		HANDLE m_thread;
#else
        pthread_t m_thread;
        bool m_threadValid; // there is no invalid pthread_t, so we keep a separate 'valid' flag
#endif

#ifdef __APPLE__
        static void appleStartThreadPrologue();
#endif
		PFC_CLASS_NOT_COPYABLE_EX(thread)
	};
}
