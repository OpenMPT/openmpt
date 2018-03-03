#pragma once

inline static t_size GetOptimalWorkerThreadCount() throw() {
	return pfc::getOptimalWorkerThreadCount();
}

//! IMPORTANT: all classes derived from CVerySimpleThread must call WaitTillThreadDone() in their destructor, to avoid object destruction during a virtual function call!
class CVerySimpleThread : private pfc::thread {
public:
	void StartThread(int priority) {
		this->pfc::thread::startWithPriority( priority );
	}
	void StartThread() {
		this->StartThread( pfc::thread::currentPriority() );
	}

	bool IsThreadActive() const {
		return this->pfc::thread::isActive();
	}
	void WaitTillThreadDone() {
		this->pfc::thread::waitTillDone();
	}
protected:
	CVerySimpleThread() {}
	virtual void ThreadProc() = 0;
private:

	void threadProc() {
		this->ThreadProc();
	}

	PFC_CLASS_NOT_COPYABLE_EX(CVerySimpleThread)
};

//! IMPORTANT: all classes derived from CSimpleThread must call AbortThread()/WaitTillThreadDone() in their destructors, to avoid object destruction during a virtual function call!
class CSimpleThread : private completion_notify_receiver, private pfc::thread {
public:
	void StartThread(int priority) {
		AbortThread();
		m_abort.reset();
		m_ownNotify = create_task(0);
		this->pfc::thread::startWithPriority( priority );
	}
	void StartThread() {
		this->StartThread( pfc::thread::currentPriority () );
	}
	void AbortThread() {
		m_abort.abort();
		CloseThread();
	}
	bool IsThreadActive() const {
		return this->pfc::thread::isActive();
	}
	void WaitTillThreadDone() {
		CloseThread();
	}
protected:
	CSimpleThread() {}
	~CSimpleThread() {AbortThread();}

	virtual unsigned ThreadProc(abort_callback & p_abort) = 0;
	//! Called when the thread has completed normally, with p_code equal to ThreadProc retval. Not called when AbortThread() or WaitTillThreadDone() was used to abort the thread / wait for the thread to finish.
	virtual void ThreadDone(unsigned p_code) {};
private:
	void CloseThread() {
		this->pfc::thread::waitTillDone();
		orphan_all_tasks();
	}

	void on_task_completion(unsigned p_id,unsigned p_status) {
		if (IsThreadActive()) {
			CloseThread();
			ThreadDone(p_status);
		}
	}
	void threadProc() {
		unsigned code = ~0;
		try {
			code = ThreadProc(m_abort);
		} catch(...) {}
		if (!m_abort.is_aborting()) m_ownNotify->on_completion_async(code);
	}
	abort_callback_impl m_abort;
	completion_notify_ptr m_ownNotify;

	PFC_CLASS_NOT_COPYABLE_EX(CSimpleThread);
};

