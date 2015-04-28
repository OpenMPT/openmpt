#include "pfc.h"

#ifdef _WIN32

BOOL pfc::winFormatSystemErrorMessage(pfc::string_base & p_out,DWORD p_code) {
	switch(p_code) {
	case ERROR_CHILD_NOT_COMPLETE:
		p_out = "Application cannot be run in Win32 mode.";
		return TRUE;
	case ERROR_INVALID_ORDINAL:
		p_out = "Invalid ordinal.";
		return TRUE;
	case ERROR_INVALID_STARTING_CODESEG:
		p_out = "Invalid code segment.";
		return TRUE;
	case ERROR_INVALID_STACKSEG:
		p_out = "Invalid stack segment.";
		return TRUE;
	case ERROR_INVALID_MODULETYPE:
		p_out = "Invalid module type.";
		return TRUE;
	case ERROR_INVALID_EXE_SIGNATURE:
		p_out = "Invalid executable signature.";
		return TRUE;
	case ERROR_BAD_EXE_FORMAT:
		p_out = "Not a valid Win32 application.";
		return TRUE;
	case ERROR_EXE_MACHINE_TYPE_MISMATCH:
		p_out = "Machine type mismatch.";
		return TRUE;
	case ERROR_EXE_CANNOT_MODIFY_SIGNED_BINARY:
	case ERROR_EXE_CANNOT_MODIFY_STRONG_SIGNED_BINARY:
		p_out = "Unable to modify a signed binary.";
		return TRUE;
	default:
		{
			TCHAR temp[512];
			if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,p_code,0,temp,_countof(temp),0) == 0) return FALSE;
			for(t_size n=0;n<_countof(temp);n++) {
				switch(temp[n]) {
				case '\n':
				case '\r':
					temp[n] = ' ';
					break;
				}
			}
			p_out = stringcvt::string_utf8_from_os(temp,_countof(temp));
			return TRUE;
		}
		break;
	}
}
void pfc::winPrefixPath(pfc::string_base & out, const char * p_path) {
	const char * prepend_header = "\\\\?\\";
	const char * prepend_header_net = "\\\\?\\UNC\\";
	if (pfc::strcmp_partial( p_path, prepend_header ) == 0) { out = p_path; return; }
	out.reset();
	if (pfc::strcmp_partial(p_path,"\\\\") != 0) {
		out << prepend_header << p_path;
	} else {
		out << prepend_header_net << (p_path+2);
	}
};

void pfc::winUnPrefixPath(pfc::string_base & out, const char * p_path) {
	const char * prepend_header = "\\\\?\\";
	const char * prepend_header_net = "\\\\?\\UNC\\";
	if (pfc::strcmp_partial(p_path, prepend_header_net) == 0) {
		out = pfc::string_formatter() << "\\\\" << (p_path + strlen(prepend_header_net) );
		return;
	}
	if (pfc::strcmp_partial(p_path, prepend_header) == 0) {
		out = (p_path + strlen(prepend_header));
		return;
	}
	out = p_path;
}

format_win32_error::format_win32_error(DWORD p_code) {
	LastErrorRevertScope revert;
	if (p_code == 0) m_buffer = "Undefined error";
	else if (!pfc::winFormatSystemErrorMessage(m_buffer,p_code)) m_buffer << "Unknown error code (" << (unsigned)p_code << ")";
}

format_hresult::format_hresult(HRESULT p_code) {
	if (!pfc::winFormatSystemErrorMessage(m_buffer,(DWORD)p_code)) m_buffer = "Unknown error code";
	stamp_hex(p_code);
}
format_hresult::format_hresult(HRESULT p_code, const char * msgOverride) {
	m_buffer = msgOverride;
	stamp_hex(p_code);
}

void format_hresult::stamp_hex(HRESULT p_code) {
	m_buffer << " (0x" << pfc::format_hex((t_uint32)p_code, 8) << ")";
}


void uAddWindowStyle(HWND p_wnd,LONG p_style) {
	SetWindowLong(p_wnd,GWL_STYLE, GetWindowLong(p_wnd,GWL_STYLE) | p_style);
}

void uRemoveWindowStyle(HWND p_wnd,LONG p_style) {
	SetWindowLong(p_wnd,GWL_STYLE, GetWindowLong(p_wnd,GWL_STYLE) & ~p_style);
}

void uAddWindowExStyle(HWND p_wnd,LONG p_style) {
	SetWindowLong(p_wnd,GWL_EXSTYLE, GetWindowLong(p_wnd,GWL_EXSTYLE) | p_style);
}

void uRemoveWindowExStyle(HWND p_wnd,LONG p_style) {
	SetWindowLong(p_wnd,GWL_EXSTYLE, GetWindowLong(p_wnd,GWL_EXSTYLE) & ~p_style);
}

unsigned MapDialogWidth(HWND p_dialog,unsigned p_value) {
	RECT temp;
	temp.left = 0; temp.right = p_value; temp.top = temp.bottom = 0;
	if (!MapDialogRect(p_dialog,&temp)) return 0;
	return temp.right;
}

bool IsKeyPressed(unsigned vk) {
	return (GetKeyState(vk) & 0x8000) ? true : false;
}

//! Returns current modifier keys pressed, using win32 MOD_* flags.
unsigned GetHotkeyModifierFlags() {
	unsigned ret = 0;
	if (IsKeyPressed(VK_CONTROL)) ret |= MOD_CONTROL;
	if (IsKeyPressed(VK_SHIFT)) ret |= MOD_SHIFT;
	if (IsKeyPressed(VK_MENU)) ret |= MOD_ALT;
	if (IsKeyPressed(VK_LWIN) || IsKeyPressed(VK_RWIN)) ret |= MOD_WIN;
	return ret;
}



bool CClipboardOpenScope::Open(HWND p_owner) {
	Close();
	if (OpenClipboard(p_owner)) {
		m_open = true;
		return true;
	} else {
		return false;
	}
}
void CClipboardOpenScope::Close() {
	if (m_open) {
		m_open = false;
		CloseClipboard();
	}
}


CGlobalLockScope::CGlobalLockScope(HGLOBAL p_handle) : m_handle(p_handle), m_ptr(GlobalLock(p_handle)) {
	if (m_ptr == NULL) throw std::bad_alloc();
}
CGlobalLockScope::~CGlobalLockScope() {
	if (m_ptr != NULL) GlobalUnlock(m_handle);
}

bool IsPointInsideControl(const POINT& pt, HWND wnd) {
	HWND walk = WindowFromPoint(pt);
	for(;;) {
		if (walk == NULL) return false;
		if (walk == wnd) return true;
		if (GetWindowLong(walk,GWL_STYLE) & WS_POPUP) return false;
		walk = GetParent(walk);
	}
}

bool IsWindowChildOf(HWND child, HWND parent) {
	HWND walk = child;
	while(walk != parent && walk != NULL && (GetWindowLong(walk,GWL_STYLE) & WS_CHILD) != 0) {
		walk = GetParent(walk);
	}
	return walk == parent;
}

void win32_menu::release() {
	if (m_menu != NULL) {
		DestroyMenu(m_menu);
		m_menu = NULL;
	}
}

void win32_menu::create_popup() {
	release();
	SetLastError(NO_ERROR);
	m_menu = CreatePopupMenu();
	if (m_menu == NULL) throw exception_win32(GetLastError());
}

void win32_event::create(bool p_manualreset,bool p_initialstate) {
	release();
	SetLastError(NO_ERROR);
	m_handle = CreateEvent(NULL,p_manualreset ? TRUE : FALSE, p_initialstate ? TRUE : FALSE,NULL);
	if (m_handle == NULL) throw exception_win32(GetLastError());
}

void win32_event::release() {
	HANDLE temp = detach();
	if (temp != NULL) CloseHandle(temp);
}


DWORD win32_event::g_calculate_wait_time(double p_seconds) {
	DWORD time = 0;
	if (p_seconds> 0) {
		time = pfc::rint32(p_seconds * 1000.0);
		if (time == 0) time = 1;
	} else if (p_seconds < 0) {
		time = INFINITE;
	}
	return time;
}

//! Returns true when signaled, false on timeout
bool win32_event::g_wait_for(HANDLE p_event,double p_timeout_seconds) {
	SetLastError(NO_ERROR);
 	DWORD status = WaitForSingleObject(p_event,g_calculate_wait_time(p_timeout_seconds));
	switch(status) {
	case WAIT_FAILED:
		throw exception_win32(GetLastError());
	default:
		throw pfc::exception_bug_check();
	case WAIT_OBJECT_0:
		return true;
	case WAIT_TIMEOUT:
		return false;
	}
}

void win32_event::set_state(bool p_state) {
	PFC_ASSERT(m_handle != NULL);
	if (p_state) SetEvent(m_handle);
	else ResetEvent(m_handle);
}

int win32_event::g_twoEventWait( HANDLE ev1, HANDLE ev2, double timeout ) {
    HANDLE h[2] = {ev1, ev2};
    switch(WaitForMultipleObjects(2, h, FALSE, g_calculate_wait_time( timeout ) )) {
		default:
			pfc::crash();
        case WAIT_TIMEOUT:
            return 0;
		case WAIT_OBJECT_0:
			return 1;
		case WAIT_OBJECT_0 + 1:
            return 2;
    }
}

int win32_event::g_twoEventWait( win32_event & ev1, win32_event & ev2, double timeout ) {
    return g_twoEventWait( ev1.get_handle(), ev2.get_handle(), timeout );
}

void win32_icon::release() {
	HICON temp = detach();
	if (temp != NULL) DestroyIcon(temp);
}


void win32_accelerator::load(HINSTANCE p_inst,const TCHAR * p_id) {
	release();
	SetLastError(NO_ERROR);
	m_accel = LoadAccelerators(p_inst,p_id);
	if (m_accel == NULL) {
		throw exception_win32(GetLastError());
	}
}
	
void win32_accelerator::release() {
	if (m_accel != NULL) {
		DestroyAcceleratorTable(m_accel);
		m_accel = NULL;
	}
}

void uSleepSeconds(double p_time,bool p_alertable) {
	SleepEx(win32_event::g_calculate_wait_time(p_time),p_alertable ? TRUE : FALSE);
}


WORD GetWindowsVersionCode() throw() {
	const DWORD ver = GetVersion();
	return (WORD)HIBYTE(LOWORD(ver)) | ((WORD)LOBYTE(LOWORD(ver)) << 8);
}


namespace pfc {
    bool isShiftKeyPressed() {
        return IsKeyPressed(VK_SHIFT);
    }
    bool isCtrlKeyPressed() {
        return IsKeyPressed(VK_CONTROL);
    }
    bool isAltKeyPressed() {
        return IsKeyPressed(VK_MENU);
    }
}
#endif
