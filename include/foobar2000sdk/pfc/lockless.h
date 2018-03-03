#pragma once

#include <functional>
#ifdef _MSC_VER
#include <intrin.h>
#endif
namespace pfc {

	class threadSafeInt {
	public:
		typedef long t_val;

		threadSafeInt(t_val p_val = 0) : m_val(p_val) {}
		long operator++() throw() { return inc(); }
		long operator--() throw() { return dec(); }
		long operator++(int) throw() { return inc() - 1; }
		long operator--(int) throw() { return dec() + 1; }
		operator t_val() const throw() { return m_val; }

		t_val exchange(t_val newVal) {
#ifdef _MSC_VER
			return InterlockedExchange(&m_val, newVal);
#else
			return __sync_lock_test_and_set(&m_val, newVal);
#endif
		}
	private:
		t_val inc() {
#ifdef _MSC_VER
			return _InterlockedIncrement(&m_val);
#else
			return __sync_add_and_fetch(&m_val, 1);
#endif
		}
		t_val dec() {
#ifdef _MSC_VER
			return _InterlockedDecrement(&m_val);
#else
			return __sync_sub_and_fetch(&m_val, 1);
#endif
		}

		volatile t_val m_val;
	};

	typedef threadSafeInt counter;
	typedef threadSafeInt refcounter;

	void yield(); // forward declaration

	//! Minimalist class to call some function only once. \n
	//! Presumes low probability of concurrent run() calls actually happening, \n
	//! but frequent calls once already initialized, hence only using basic volatile bool check. \n
	//! If using a modern compiler you might want to use std::call_once instead. \n
	//! The called function is not expected to throw exceptions.
	class runOnceLock {
	public:
		void run(std::function<void()> f) {
			if (m_done) return;
			if (m_once.exchange(1) == 0) {
				f();
				m_done = true;
			} else {
				while (!m_done) yield();
			}
		}
	private:
		threadSafeInt m_once;
		volatile bool m_done;

	};
}
