#include "StdAfx.h"

#include "ThreadUtils.h"
#include "rethrow.h"

#include <exception>

namespace ThreadUtils {
	bool CRethrow::exec( std::function<void () > f ) throw() {
		m_rethrow = nullptr;
		bool rv = false;
		try {
			f();
			rv = true;
		} catch( ... ) {
			auto e = std::current_exception();
			m_rethrow = [e] { std::rethrow_exception(e); };
		}  

		return rv;
	}

	void CRethrow::rethrow() const {
		if ( m_rethrow ) m_rethrow();
	}
}
#ifdef _WIN32

#include "win32_misc.h"

#ifdef FOOBAR2000_MOBILE
#include <pfc/pp-winapi.h>
#endif


namespace ThreadUtils {
	bool WaitAbortable(HANDLE ev, abort_callback & abort, DWORD timeout) {
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
#ifdef FOOBAR2000_DESKTOP_WINDOWS
	void ProcessPendingMessagesWithDialog(HWND hDialog) {
		MSG msg = {};
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (!IsDialogMessage(hDialog, &msg)) {
				DispatchMessage(&msg);
			}
		}
	}
	void ProcessPendingMessages() {
		MSG msg = {};
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
	}
	void WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort) {
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
	t_size MultiWaitAbortable_MsgLoop(const HANDLE * ev, t_size evCount, abort_callback & abort) {
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

	void SleepAbortable_MsgLoop(abort_callback & abort, DWORD timeout /*must not be INFINITE*/) {
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

	bool WaitAbortable_MsgLoop(HANDLE ev, abort_callback & abort, DWORD timeout /*must not be INFINITE*/) {
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

#endif // FOOBAR2000_DESKTOP_WINDOWS

}
#endif