#pragma once

#include <list>
#include "synchro.h"

namespace pfc {

	template<typename obj_t>
	class waitQueue {
	public:
		waitQueue() : m_eof() {}
		
		void put( obj_t const & obj ) {
			mutexScope guard( m_mutex );
			m_list.push_back( obj );
			if ( m_list.size() == 1 ) m_canRead.set_state( true );
		}
		void put( obj_t && obj ) {
			mutexScope guard( m_mutex );
			m_list.push_back( std::move(obj) );
			if ( m_list.size() == 1 ) m_canRead.set_state( true );
		}

		void set_eof() {
			mutexScope guard(m_mutex);
			m_eof = true;
			m_canRead.set_state(true);
		}
        
        eventHandle_t get_event_handle() {
            return m_canRead.get_handle();
        }

		bool get( obj_t & out ) {
			for ( ;; ) {
				m_canRead.wait_for(-1);
				mutexScope guard(m_mutex);
				auto i = m_list.begin();
				if ( i == m_list.end() ) {
					if (m_eof) return false;
					continue;
				}
				out = std::move(*i);
				m_list.erase( i );
				didGet();
				return true;
			}
		}

		bool get( obj_t & out, pfc::eventHandle_t hAbort, bool * didAbort = nullptr ) {
			if (didAbort != nullptr) * didAbort = false;
			for ( ;; ) {
				if (pfc::event::g_twoEventWait( hAbort, m_canRead.get_handle(), -1) == 1) {
					if (didAbort != nullptr) * didAbort = true;
					return false;
				}
				mutexScope guard(m_mutex);
				auto i = m_list.begin();
				if ( i == m_list.end() ) {
					if (m_eof) return false;
					continue;
				}
				out = std::move(*i);
				m_list.erase( i );
				didGet();
				return true;
			}
		}
		void clear() {
			mutexScope guard(m_mutex);
			m_eof = false;
			m_list.clear();
			m_canRead.set_state(false);
		}
	private:
		void didGet() {
			if (!m_eof && m_list.size() == 0) m_canRead.set_state(false);
		}
		bool m_eof;
		std::list<obj_t> m_list;
		mutex m_mutex;
		event m_canRead;
	};

	template<typename obj_t_>
	class waitQueue2 {
	protected:
		typedef obj_t_ obj_t;
		typedef std::list<obj_t_> list_t;
		virtual bool canWriteCheck(list_t const &) { return true; }

	public:
		void waitWrite() {
			m_canWrite.wait_for(-1);
		}
		bool waitWrite(pfc::eventHandle_t hAbort) {
			return event::g_twoEventWait( hAbort, m_canWrite.get_handle(), -1) == 2;
		}
		eventHandle_t waitWriteHandle() {
			return m_canWrite.get_handle();
		}

		waitQueue2() : m_eof() {
			m_canWrite.set_state(true);
		}

		void put(obj_t const & obj) {
			mutexScope guard(m_mutex);
			m_list.push_back(obj);
			if (m_list.size() == 1) m_canRead.set_state(true);
			refreshCanWrite();
		}
		void put(obj_t && obj) {
			mutexScope guard(m_mutex);
			m_list.push_back(std::move(obj));
			if (m_list.size() == 1) m_canRead.set_state(true);
			refreshCanWrite();
		}

		void set_eof() {
			mutexScope guard(m_mutex);
			m_eof = true;
			m_canRead.set_state(true);
			m_canWrite.set_state(false);
		}

		bool get(obj_t & out) {
			for (;; ) {
				m_canRead.wait_for(-1);
				mutexScope guard(m_mutex);
				auto i = m_list.begin();
				if (i == m_list.end()) {
					if (m_eof) return false;
					continue;
				}
				out = std::move(*i);
				m_list.erase(i);
				didGet();
				return true;
			}
		}

		bool get(obj_t & out, pfc::eventHandle_t hAbort, bool * didAbort = nullptr) {
			if (didAbort != nullptr) * didAbort = false;
			for (;; ) {
				if (pfc::event::g_twoEventWait(hAbort, m_canRead.get_handle(), -1) == 1) {
					if (didAbort != nullptr) * didAbort = true;
					return false;
				}
				mutexScope guard(m_mutex);
				auto i = m_list.begin();
				if (i == m_list.end()) {
					if (m_eof) return false;
					continue;
				}
				out = std::move(*i);
				m_list.erase(i);
				didGet();
				return true;
			}
		}

		void clear() {
			mutexScope guard(m_mutex);
			m_list.clear();
			m_eof = false;
			m_canRead.set_state(false);
			m_canWrite.set_state(true);
		}
	private:
		void didGet() {
			if (m_eof) return;
			if (m_list.size() == 0) {
				m_canRead.set_state(false);
				m_canWrite.set_state(true);
			} else {
				m_canWrite.set_state(canWriteCheck(m_list));
			}
		}
		void refreshCanWrite() {
			m_canWrite.set_state( !m_eof && canWriteCheck(m_list));
		}
		bool m_eof;
		std::list<obj_t> m_list;
		mutex m_mutex;
		event m_canRead, m_canWrite;
	};
}
