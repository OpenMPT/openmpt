#include "stdafx.h"

void registerclass_scope_delayed::toggle_on(UINT p_style,WNDPROC p_wndproc,int p_clsextra,int p_wndextra,HICON p_icon,HCURSOR p_cursor,HBRUSH p_background,const TCHAR * p_class_name,const TCHAR * p_menu_name) {
	toggle_off();
	WNDCLASS wc = {};
	wc.style = p_style;
	wc.lpfnWndProc = p_wndproc;
	wc.cbClsExtra = p_clsextra;
	wc.cbWndExtra = p_wndextra;
	wc.hInstance = core_api::get_my_instance();
	wc.hIcon = p_icon;
	wc.hCursor = p_cursor;
	wc.hbrBackground = p_background;
	wc.lpszMenuName = p_menu_name;
	wc.lpszClassName = p_class_name;
	WIN32_OP( (m_class = RegisterClass(&wc)) != 0);
}

void registerclass_scope_delayed::toggle_off() {
	if (m_class != 0) {
		UnregisterClass((LPCTSTR)m_class,core_api::get_my_instance());
		m_class = 0;
	}
}


unsigned QueryScreenDPI() {
	HDC dc = GetDC(0);
	unsigned ret = GetDeviceCaps(dc,LOGPIXELSY);
	ReleaseDC(0,dc);
	return ret;
}


SIZE QueryScreenDPIEx() {
	HDC dc = GetDC(0);
	SIZE ret = { GetDeviceCaps(dc,LOGPIXELSY), GetDeviceCaps(dc,LOGPIXELSY) };
	ReleaseDC(0,dc);
	return ret;
}


bool IsMenuNonEmpty(HMENU menu) {
	unsigned n,m=GetMenuItemCount(menu);
	for(n=0;n<m;n++) {
		if (GetSubMenu(menu,n)) return true;
		if (!(GetMenuState(menu,n,MF_BYPOSITION)&MF_SEPARATOR)) return true;
	}
	return false;
}

PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL() {
	const DWORD code = GetLastError();
	PFC_ASSERT( code != NO_ERROR );
	pfc::string_fixed_t<32> debugMsg; debugMsg << "Win32 error #" << (t_uint32)code;
	TRACK_CODE( debugMsg, throw exception_win32(code) );
}

#ifdef _DEBUG
void WIN32_OP_D_FAIL(const wchar_t * _Message, const wchar_t *_File, unsigned _Line) {
	const DWORD code = GetLastError();
	pfc::array_t<wchar_t> msgFormatted; msgFormatted.set_size(pfc::strlen_t(_Message) + 64);
	wsprintfW(msgFormatted.get_ptr(), L"%s (code: %u)", _Message, code);
	if (IsDebuggerPresent()) {
		OutputDebugString(TEXT("WIN32_OP_D() failure:\n"));
		OutputDebugString(msgFormatted.get_ptr());
		OutputDebugString(TEXT("\n"));
		pfc::crash();
	}
	_wassert(msgFormatted.get_ptr(),_File,_Line);
}
#endif


void GetOSVersionString(pfc::string_base & out) {
	out.reset(); GetOSVersionStringAppend(out);
}

static bool running_under_wine(void) {
    HMODULE module = GetModuleHandle(_T("ntdll.dll"));
    if (!module) return false;
    return GetProcAddress(module, "wine_server_call") != NULL;
}
static bool FetchWineInfoAppend(pfc::string_base & out) {
	typedef const char *(__cdecl *t_wine_get_build_id)(void);
    typedef void (__cdecl *t_wine_get_host_version)( const char **sysname, const char **release );
	const HMODULE ntdll = GetModuleHandle(_T("ntdll.dll"));
	if (ntdll == NULL) return false;
	t_wine_get_build_id wine_get_build_id;
	t_wine_get_host_version wine_get_host_version;
    wine_get_build_id = (t_wine_get_build_id)GetProcAddress(ntdll, "wine_get_build_id");
    wine_get_host_version = (t_wine_get_host_version)GetProcAddress(ntdll, "wine_get_host_version");
	if (wine_get_build_id == NULL || wine_get_host_version == NULL) {
		if (GetProcAddress(ntdll, "wine_server_call") != NULL) {
			out << "wine (unknown version)";
			return true;
		}
		return false;
	}
	const char * sysname = NULL; const char * release = NULL;
	wine_get_host_version(&sysname, &release);
	out << wine_get_build_id() << ", on: " << sysname << " / " << release;
	return true;
}
void GetOSVersionStringAppend(pfc::string_base & out) {

	if (FetchWineInfoAppend(out)) return;

	OSVERSIONINFO ver = {}; ver.dwOSVersionInfoSize = sizeof(ver);
	WIN32_OP( GetVersionEx(&ver) );
	SYSTEM_INFO info = {};
	GetNativeSystemInfo(&info);
	
	out << "Windows " << (int)ver.dwMajorVersion << "." << (int)ver.dwMinorVersion << "." << (int)ver.dwBuildNumber;
	if (ver.szCSDVersion[0] != 0) out << " " << pfc::stringcvt::string_utf8_from_os(ver.szCSDVersion, PFC_TABSIZE(ver.szCSDVersion));
	
	switch(info.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_AMD64:
			out << " x64"; break;
		case PROCESSOR_ARCHITECTURE_IA64:
			out << " IA64"; break;
		case PROCESSOR_ARCHITECTURE_INTEL:
			out << " x86"; break;
	}
}


void SetDefaultMenuItem(HMENU p_menu,unsigned p_id) {
	MENUITEMINFO info = {sizeof(info)};
	info.fMask = MIIM_STATE;
	GetMenuItemInfo(p_menu,p_id,FALSE,&info);
	info.fState |= MFS_DEFAULT;
	SetMenuItemInfo(p_menu,p_id,FALSE,&info);
}
