#pragma once

#ifdef _WIN32


#include "win32_op.h"

class CloseHandleScope {
public:
	CloseHandleScope(HANDLE handle) throw() : m_handle(handle) {}
	~CloseHandleScope() throw() { CloseHandle(m_handle); }
	HANDLE Detach() throw() { return pfc::replace_t(m_handle, INVALID_HANDLE_VALUE); }
	HANDLE Get() const throw() { return m_handle; }
	void Close() throw() { CloseHandle(Detach()); }
	PFC_CLASS_NOT_COPYABLE_EX(CloseHandleScope)
private:
	HANDLE m_handle;
};


class mutexScope {
public:
	mutexScope(HANDLE hMutex_, abort_callback & abort);
	~mutexScope();
private:
	PFC_CLASS_NOT_COPYABLE_EX(mutexScope);
	HANDLE hMutex;
};

#ifdef FOOBAR2000_DESKTOP_WINDOWS

class registerclass_scope_delayed {
public:
	registerclass_scope_delayed() : m_class(0) {}

	bool is_registered() const {return m_class != 0;}
	void toggle_on(UINT p_style,WNDPROC p_wndproc,int p_clsextra,int p_wndextra,HICON p_icon,HCURSOR p_cursor,HBRUSH p_background,const TCHAR * p_classname,const TCHAR * p_menuname);
	void toggle_off();
	ATOM get_class() const {return m_class;}

	~registerclass_scope_delayed() {toggle_off();}
private:
	registerclass_scope_delayed(const registerclass_scope_delayed &) {throw pfc::exception_not_implemented();}
	const registerclass_scope_delayed & operator=(const registerclass_scope_delayed &) {throw pfc::exception_not_implemented();}

	ATOM m_class;
};



typedef CGlobalLockScope CGlobalLock;//for compatibility, implementation moved elsewhere

bool SetClipboardDataBlock(UINT p_format, const void * p_block, t_size p_block_size);

template<typename t_array>
static bool SetClipboardDataBlock(UINT p_format,const t_array & p_array) {
	PFC_STATIC_ASSERT( sizeof(p_array[0]) == 1 );
	return SetClipboardDataBlock(p_format,p_array.get_ptr(),p_array.get_size());
}

template<typename t_array>
static bool GetClipboardDataBlock(UINT p_format,t_array & p_array) {
	PFC_STATIC_ASSERT( sizeof(p_array[0]) == 1 );
	if (OpenClipboard(NULL)) {
		HANDLE handle = GetClipboardData(p_format);
		if (handle == NULL) {
			CloseClipboard();
			return false;
		}
		{
			CGlobalLock lock(handle);
			const t_size size = lock.GetSize();
			try {
				p_array.set_size(size);
			} catch(...) {
				CloseClipboard();
				throw;
			}
			memcpy(p_array.get_ptr(),lock.GetPtr(),size);
		}
		CloseClipboard();
		return true;
	} else {
		return false;
	}
}


class OleInitializeScope {
public:
	OleInitializeScope();
	~OleInitializeScope();

private:
	PFC_CLASS_NOT_COPYABLE_EX(OleInitializeScope);
};

class CoInitializeScope {
public:
	CoInitializeScope();
	CoInitializeScope(DWORD params);
	~CoInitializeScope();
private:
	PFC_CLASS_NOT_COPYABLE_EX(CoInitializeScope)
};


unsigned QueryScreenDPI(HWND wnd = NULL);
unsigned QueryScreenDPI_X(HWND wnd = NULL);
unsigned QueryScreenDPI_Y(HWND wnd = NULL);

SIZE QueryScreenDPIEx(HWND wnd = NULL);

static WORD GetOSVersion() {
	const DWORD ver = GetVersion();
	return (WORD)HIBYTE(LOWORD(ver)) | ((WORD)LOBYTE(LOWORD(ver)) << 8);
}

#if _WIN32_WINNT >= 0x501
#define WS_EX_COMPOSITED_Safe() WS_EX_COMPOSITED
#else
static DWORD WS_EX_COMPOSITED_Safe() {
	return (GetOSVersion() < 0x501) ? 0 : 0x02000000L;
}
#endif





class EnableWindowScope {
public:
	EnableWindowScope(HWND p_window,BOOL p_state) throw() : m_window(p_window) {
		m_oldState = IsWindowEnabled(m_window);
		EnableWindow(m_window,p_state);
	}
	~EnableWindowScope() throw() {
		EnableWindow(m_window,m_oldState);
	}

private:
	BOOL m_oldState;
	HWND m_window;
};

bool IsMenuNonEmpty(HMENU menu);

class CModelessDialogEntry {
public:
	CModelessDialogEntry() : m_wnd() {}
	CModelessDialogEntry(HWND p_wnd) : m_wnd() {Set(p_wnd);}
	~CModelessDialogEntry() {Set(NULL);}

	void Set(HWND p_new);
private:
	PFC_CLASS_NOT_COPYABLE_EX(CModelessDialogEntry);
	HWND m_wnd;
};

void GetOSVersionString(pfc::string_base & out);
void GetOSVersionStringAppend(pfc::string_base & out);

void SetDefaultMenuItem(HMENU p_menu,unsigned p_id);


class CDLL {
public:
#ifdef _DEBUG
	static LPTOP_LEVEL_EXCEPTION_FILTER _GetEH() {
		LPTOP_LEVEL_EXCEPTION_FILTER rv = SetUnhandledExceptionFilter(NULL);
		SetUnhandledExceptionFilter(rv);
		return rv;
	}
#endif
	CDLL(const wchar_t * Name) : hMod() {
		Load(Name);
	}
	CDLL() : hMod() {}
	void Load(const wchar_t * Name) {
		PFC_ASSERT( hMod == NULL );
#ifdef _DEBUG
		auto handlerBefore = _GetEH();
#endif
		WIN32_OP( hMod = LoadLibrary(Name) );
#ifdef _DEBUG
		PFC_ASSERT( handlerBefore == _GetEH() );
#endif
	}


	~CDLL() {
		if (hMod) FreeLibrary(hMod);
	}
	template<typename funcptr_t> void Bind(funcptr_t & outFunc, const char * name) {
		WIN32_OP( outFunc = (funcptr_t)GetProcAddress(hMod, name) );
	}

	HMODULE hMod;

	PFC_CLASS_NOT_COPYABLE_EX(CDLL);
};

class winLocalFileScope {
public:
	void open(const char * inPath, file::ptr inReader, abort_callback & aborter);
	void close();

	winLocalFileScope() : m_isTemp() {}
	winLocalFileScope(const char * inPath, file::ptr inReader, abort_callback & aborter) : m_isTemp() {
		open(inPath, inReader, aborter);
	}

	~winLocalFileScope() {
		close();
	}

	const wchar_t * Path() const { return m_path.c_str(); }
private:
	bool m_isTemp;
	std::wstring m_path;
};

#endif // FOOBAR2000_DESKTOP_WINDOWS


class CMutex {
public:
	CMutex(const TCHAR * name = NULL);
	~CMutex();
	HANDLE Handle() {return m_hMutex;}
	static void AcquireByHandle( HANDLE hMutex, abort_callback & aborter );
	void Acquire( abort_callback& aborter );
	void Release();
private:
	CMutex(const CMutex&); void operator=(const CMutex&);
	HANDLE m_hMutex;
};

class CMutexScope {
public:
	CMutexScope(CMutex & mutex, DWORD timeOutMS, const char * timeOutBugMsg);
	CMutexScope(CMutex & mutex);
	CMutexScope(CMutex & mutex, abort_callback & aborter);
	~CMutexScope();
private:
	CMutexScope(const CMutexScope &); void operator=(const CMutexScope&);
	CMutex & m_mutex;
};

LRESULT RelayEraseBkgnd(HWND p_from, HWND p_to, HDC p_dc);

#endif // _WIN32
