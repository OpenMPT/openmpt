#pragma once
#include <pfc/wait_queue.h>
#include <pfc/pool.h>
#include <functional>
#include "rethrow.h"

namespace ThreadUtils {

	// Serialize access to some resource to a single thread
	// Execute blocking/nonabortable methods in with proper abortability (detach on abort and move on)
	class cmdThread {
	public:
		typedef std::function<void () > func_t;
		typedef pfc::waitQueue<func_t> queue_t;
		typedef std::function<void (abort_callback&) > funcAbortable_t;
		cmdThread() {
			auto q = std::make_shared<queue_t>();
			m_queue = q;
			auto x = std::make_shared<atExit_t>();
			m_atExit = x;
			pfc::splitThread( [q, x] {
				for ( ;; ) {
					func_t f;
					if (!q->get(f)) break;
					try { f(); } catch(...) {}
				}
				// No guard for atExit access, as nobody is supposed to be still able to call host object methods by the time we get here
				for( auto i = x->begin(); i != x->end(); ++ i ) {
					auto f = *i;
					try { f(); } catch(...) {}
				}
			} );
		}
		void atExit( func_t f ) {
			m_atExit->push_back(f);
		}
		~cmdThread() {
			m_queue->set_eof();
		}
		void runSynchronously( func_t f, abort_callback & abort ) {
			auto evt = m_eventPool.make();
			evt->set_state(false);
			auto rethrow = std::make_shared<ThreadUtils::CRethrow>();
			auto worker2 = [f, rethrow, evt] {
				rethrow->exec(f);
				evt->set_state( true );
			};

			add ( worker2 );

			abort.waitForEvent( * evt, -1 );

			m_eventPool.put( evt );

			rethrow->rethrow();
		}
		void runSynchronously2( funcAbortable_t f, abort_callback & abort ) {
			auto subAbort = m_abortPool.make();
			subAbort->reset();
			auto worker = [subAbort, f] {
				f(*subAbort);
			};

			try {
				runSynchronously( worker, abort );
			} catch(...) {
				subAbort->set(); throw;
			}

			m_abortPool.put( subAbort );
		}

		void add( func_t f ) { m_queue->put( f ); }
	private:
		pfc::objPool<pfc::event> m_eventPool;
		pfc::objPool<abort_callback_impl> m_abortPool;
		std::shared_ptr<queue_t> m_queue;
		typedef std::list<func_t> atExit_t;
		std::shared_ptr<atExit_t> m_atExit;
	};
}