#pragma once

#include "ThreadUtils.h"
#include "rethrow.h"

namespace ThreadUtils {

	// OLD single thread wrapper class used only by old WMA input
	// Modern code should use cmdThread which is all kinds of prettier
	template<typename TBase, bool processMsgs = false>
	class CSingleThreadWrapper : protected CVerySimpleThread {
	private:
	protected:
		class command {
		protected:
			command() : m_abort(), m_completionEvent() {}
			virtual void executeImpl(TBase &) {}
			virtual ~command() {}
		public:
			void execute(TBase & obj) {
				m_rethrow.exec( [this, &obj] {executeImpl(obj); } );
				SetEvent(m_completionEvent);
			}
			void rethrow() const {
				m_rethrow.rethrow();
			}
			CRethrow m_rethrow;
			HANDLE m_completionEvent;
			abort_callback * m_abort;
		};

		typedef std::function<void (TBase&) > func_t;

		class command2 : public command {
		public:
			command2( func_t f ) : m_func(f) {}
			void executeImpl(TBase & obj) {
				m_func(obj);
			}
		private:

			func_t m_func;
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

		void invokeCommand2( func_t f, abort_callback & abort) {
			auto c = pfc::rcnew_t<command2>(f);
			invokeCommand( c, abort );
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