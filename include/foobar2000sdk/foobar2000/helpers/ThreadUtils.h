#pragma once
#include "fb2k_threads.h"

#ifdef _WIN32
#include <functional>

namespace ThreadUtils {
	bool WaitAbortable(HANDLE ev, abort_callback & abort, DWORD timeout = INFINITE);

	void ProcessPendingMessages();
	void ProcessPendingMessagesWithDialog(HWND hDialog);
	
	void WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort);
	
	t_size MultiWaitAbortable_MsgLoop(const HANDLE * ev, t_size evCount, abort_callback & abort);
	void SleepAbortable_MsgLoop(abort_callback & abort, DWORD timeout /*must not be INFINITE*/);
	bool WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort, DWORD timeout /*must not be INFINITE*/);

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
			auto iter = m_content.cfirst();
			FB2K_DYNAMIC_ASSERT( iter.is_valid() );
			out = *iter;
			m_content.remove(iter);
			if (m_content.get_count() == 0) m_event.set_state(false);
		}
		win32_event m_event;
		critical_section m_sync;
		pfc::chain_list_v2_t<TWhat> m_content;
	};

}
#endif // _WIN32
