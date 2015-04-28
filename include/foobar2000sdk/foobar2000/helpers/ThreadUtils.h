namespace ThreadUtils {
	static bool WaitAbortable(HANDLE ev, abort_callback & abort, DWORD timeout = INFINITE) {
		const HANDLE handles[2] = {ev, abort.get_abort_event()};
		SetLastError(0);
		const DWORD status = WaitForMultipleObjects(2, handles, FALSE, timeout);
		switch(status) {
			case WAIT_TIMEOUT:
				PFC_ASSERT( timeout != INFINITE );
				return false;
			case WAIT_OBJECT_0:
				return true;
			case WAIT_OBJECT_0 + 1:
				throw exception_aborted();
			case WAIT_FAILED:
				WIN32_OP_FAIL();
			default:
				uBugCheck();
		}
	}

	static void ProcessPendingMessages() {
		MSG msg = {};
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
	}

	
	static void WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort) {
		const HANDLE handles[2] = {ev, abort.get_abort_event()};
		for(;;) {
			SetLastError(0);
			const DWORD status = MsgWaitForMultipleObjects(2, handles, FALSE, INFINITE, QS_ALLINPUT);
			switch(status) {
				case WAIT_TIMEOUT:
					PFC_ASSERT(!"How did we get here?");
					uBugCheck();
				case WAIT_OBJECT_0:
					return;
				case WAIT_OBJECT_0 + 1:
					throw exception_aborted();
				case WAIT_OBJECT_0 + 2:
					ProcessPendingMessages();
					break;
				case WAIT_FAILED:
					WIN32_OP_FAIL();
				default:
					uBugCheck();
			}
		}
	}
	
	static t_size MultiWaitAbortable_MsgLoop(const HANDLE * ev, t_size evCount, abort_callback & abort) {
		pfc::array_t<HANDLE> handles; handles.set_size(evCount + 1);
		handles[0] = abort.get_abort_event();
		pfc::memcpy_t(handles.get_ptr() + 1, ev, evCount);
		for(;;) {
			SetLastError(0);
			const DWORD status = MsgWaitForMultipleObjects(handles.get_count(), handles.get_ptr(), FALSE, INFINITE, QS_ALLINPUT);
			switch(status) {
				case WAIT_TIMEOUT:
					PFC_ASSERT(!"How did we get here?");
					uBugCheck();
				case WAIT_OBJECT_0:
					throw exception_aborted();
				case WAIT_FAILED:
					WIN32_OP_FAIL();
				default:
					{
						t_size index = (t_size)(status - (WAIT_OBJECT_0 + 1));
						if (index == evCount) {
							ProcessPendingMessages();
						} else if (index < evCount) {
							return index;
						} else {
							uBugCheck();
						}
					}
			}
		}
	}

	static void SleepAbortable_MsgLoop(abort_callback & abort, DWORD timeout /*must not be INFINITE*/) {
		PFC_ASSERT( timeout != INFINITE );
		const DWORD entry = GetTickCount();
		const HANDLE handles[1] = {abort.get_abort_event()};
		for(;;) {
			const DWORD done = GetTickCount() - entry;
			if (done >= timeout) return;
			SetLastError(0);
			const DWORD status = MsgWaitForMultipleObjects(1, handles, FALSE, timeout - done, QS_ALLINPUT);
			switch(status) {
				case WAIT_TIMEOUT:
					return;
				case WAIT_OBJECT_0:
					throw exception_aborted();
				case WAIT_OBJECT_0 + 1:
					ProcessPendingMessages();
				default:
					throw exception_win32(GetLastError());
			}
		}
	}

	static bool WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort, DWORD timeout /*must not be INFINITE*/) {
		PFC_ASSERT( timeout != INFINITE );
		const DWORD entry = GetTickCount();
		const HANDLE handles[2] = {ev, abort.get_abort_event()};
		for(;;) {
			const DWORD done = GetTickCount() - entry;
			if (done >= timeout) return false;
			SetLastError(0);
			const DWORD status = MsgWaitForMultipleObjects(2, handles, FALSE, timeout - done, QS_ALLINPUT);
			switch(status) {
				case WAIT_TIMEOUT:
					return false;
				case WAIT_OBJECT_0:
					return true;
				case WAIT_OBJECT_0 + 1:
					throw exception_aborted();
				case WAIT_OBJECT_0 + 2:
					ProcessPendingMessages();
					break;
				case WAIT_FAILED:
					WIN32_OP_FAIL();
				default:
					uBugCheck();
			}
		}
	}

	template<typename TWhat>
	class CObjectQueue {
	public:
		CObjectQueue() { m_event.create(true,false); }

		template<typename TSource> void Add(const TSource & source) {
			insync(m_sync);
			m_content.add_item(source);
			if (m_content.get_count() == 1) m_event.set_state(true);
		}
		template<typename TDestination> void Get(TDestination & out, abort_callback & abort) {
			WaitAbortable(m_event.get(), abort);
			_Get(out);
		}

		template<typename TDestination> void Get_MsgLoop(TDestination & out, abort_callback & abort) {
			WaitAbortable_MsgLoop(m_event.get(), abort);
			_Get(out);
		}

	private:
		template<typename TDestination> void _Get(TDestination & out) {
			insync(m_sync);
			pfc::const_iterator<TWhat> iter = m_content.first();
			FB2K_DYNAMIC_ASSERT( iter.is_valid() );
			out = *iter;
			m_content.remove(iter);
			if (m_content.get_count() == 0) m_event.set_state(false);
		}
		win32_event m_event;
		critical_section m_sync;
		pfc::chain_list_v2_t<TWhat> m_content;
	};


	template<typename TBase, bool processMsgs = false>
	class CSingleThreadWrapper : protected CVerySimpleThread {
	private:
		enum status {
			success,
			fail,
			fail_io,
			fail_io_data,
			fail_abort,
		};
	protected:
		class command {
		protected:
			command() : m_status(success), m_abort(), m_completionEvent() {}
			virtual void executeImpl(TBase &) {}
			virtual ~command() {}
		public:
			void execute(TBase & obj) {
				try {
					executeImpl(obj);
					m_status = success;
				} catch(exception_aborted const & e) {
					m_status = fail_abort; m_statusMsg = e.what();
				} catch(exception_io_data const & e) {
					m_status = fail_io_data; m_statusMsg = e.what();
				} catch(exception_io const & e) {
					m_status = fail_io; m_statusMsg = e.what();
				} catch(std::exception const & e) {
					m_status = fail; m_statusMsg = e.what();
				}
				SetEvent(m_completionEvent);
			}
			void rethrow() const {
				switch(m_status) {
					case fail:
						throw pfc::exception(m_statusMsg);
					case fail_io:
						throw exception_io(m_statusMsg);
					case fail_io_data:
						throw exception_io_data(m_statusMsg);
					case fail_abort:
						throw exception_aborted();
					case success:
						break;
					default:
						uBugCheck();
				}
			}
			status m_status;
			pfc::string8 m_statusMsg;
			HANDLE m_completionEvent;
			abort_callback * m_abort;
		};
		
		typedef pfc::rcptr_t<command> command_ptr;

		CSingleThreadWrapper() {
			m_completionEvent.create(true,false);
			this->StartThread();
			//start();
		}

		~CSingleThreadWrapper() {
			m_threadAbort.abort();
			this->WaitTillThreadDone();
		}

		void invokeCommand(command_ptr cmd, abort_callback & abort) {
			abort.check();
			m_completionEvent.set_state(false);
			pfc::vartoggle_t<abort_callback*> abortToggle(cmd->m_abort, &abort);
			pfc::vartoggle_t<HANDLE> eventToggle(cmd->m_completionEvent, m_completionEvent.get() );
			m_commands.Add(cmd);
			m_completionEvent.wait_for(-1);
			//WaitAbortable(m_completionEvent.get(), abort);
			cmd->rethrow();
		}

	private:
		void ThreadProc() {
			TRACK_CALL_TEXT("CSingleThreadWrapper entry");
			try {
				TBase instance;
				for(;;) {
					command_ptr cmd;
					if (processMsgs) m_commands.Get_MsgLoop(cmd, m_threadAbort);
					else m_commands.Get(cmd, m_threadAbort);
					cmd->execute(instance);
				}
			} catch(...) {}
			if (processMsgs) ProcessPendingMessages();
		}
		win32_event m_completionEvent;
		CObjectQueue<command_ptr> m_commands;
		abort_callback_impl m_threadAbort;
	};
}
