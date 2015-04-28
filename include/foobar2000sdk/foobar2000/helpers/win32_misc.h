PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL();
PFC_NORETURN PFC_NOINLINE void WIN32_OP_FAIL_CRITICAL(const char * what);

#ifdef _DEBUG
void WIN32_OP_D_FAIL(const wchar_t * _Message, const wchar_t *_File, unsigned _Line);
#endif

//Throws an exception when (OP) evaluates to false/zero.
#define WIN32_OP(OP)	\
	{	\
		SetLastError(NO_ERROR);	\
		if (!(OP)) WIN32_OP_FAIL();	\
	}

// Kills the application with appropriate debug info when (OP) evaluates to false/zero.
#define WIN32_OP_CRITICAL(WHAT, OP)	\
	{	\
		SetLastError(NO_ERROR);	\
		if (!(OP)) WIN32_OP_FAIL_CRITICAL(WHAT);	\
	}

//WIN32_OP_D() acts like an assert specialized for win32 operations in debug build, ignores the return value / error codes in release build.
//Use WIN32_OP_D() instead of WIN32_OP() on operations that are extremely unlikely to fail, so failure condition checks are performed in the debug build only, to avoid bloating release code with pointless error checks.
#ifdef _DEBUG
#define WIN32_OP_D(OP)	\
	{	\
		SetLastError(NO_ERROR); \
		if (!(OP)) WIN32_OP_D_FAIL(PFC_WIDESTRING(#OP), PFC_WIDESTRING(__FILE__), __LINE__); \
	}

#else
#define WIN32_OP_D(OP) (void)( (OP), 0);
#endif


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

bool SetClipboardDataBlock(UINT p_format,const void * p_block,t_size p_block_size);

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
	OleInitializeScope() {
		if (FAILED(OleInitialize(NULL))) throw pfc::exception("OleInitialize() failure");
	}
	~OleInitializeScope() {
		OleUninitialize();
	}

private:
	PFC_CLASS_NOT_COPYABLE(OleInitializeScope,OleInitializeScope);
};

class CoInitializeScope {
public:
	CoInitializeScope() {
		if (FAILED(CoInitialize(NULL))) throw pfc::exception("CoInitialize() failed");
	}
	CoInitializeScope(DWORD params) {
		if (FAILED(CoInitializeEx(NULL, params))) throw pfc::exception("CoInitialize() failed");
	}
	~CoInitializeScope() {
		CoUninitialize();
	}
	PFC_CLASS_NOT_COPYABLE_EX(CoInitializeScope)
};


unsigned QueryScreenDPI();

SIZE QueryScreenDPIEx();

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

class SetTextColorScope {
public:
	SetTextColorScope(HDC dc, COLORREF col) throw() : m_dc(dc) {
		m_oldCol = SetTextColor(dc,col);
	}
	~SetTextColorScope() throw() {
		SetTextColor(m_dc,m_oldCol);
	}
	PFC_CLASS_NOT_COPYABLE_EX(SetTextColorScope)
private:
	HDC m_dc;
	COLORREF m_oldCol;
};

class CloseHandleScope {
public:
	CloseHandleScope(HANDLE handle) throw() : m_handle(handle) {}
	~CloseHandleScope() throw() {CloseHandle(m_handle);}
	HANDLE Detach() throw() {return pfc::replace_t(m_handle,INVALID_HANDLE_VALUE);}
	HANDLE Get() const throw() {return m_handle;}
	void Close() throw() {CloseHandle(Detach());}
	PFC_CLASS_NOT_COPYABLE_EX(CloseHandleScope)
private:
	HANDLE m_handle;
};

class CModelessDialogEntry {
public:
	inline CModelessDialogEntry() : m_wnd() {}
	inline CModelessDialogEntry(HWND p_wnd) : m_wnd() {Set(p_wnd);}
	inline ~CModelessDialogEntry() {Set(NULL);}

	void Set(HWND p_new) {
		static_api_ptr_t<modeless_dialog_manager> api;
		if (m_wnd) api->remove(m_wnd);
		m_wnd = p_new;
		if (m_wnd) api->add(m_wnd);
	}
private:
	PFC_CLASS_NOT_COPYABLE_EX(CModelessDialogEntry);
	HWND m_wnd;
};

void GetOSVersionString(pfc::string_base & out);
void GetOSVersionStringAppend(pfc::string_base & out);



void SetDefaultMenuItem(HMENU p_menu,unsigned p_id);




class TypeFind {
public:
	static LRESULT Handler(NMHDR* hdr, int subItemFrom = 0, int subItemCnt = 1) {
		NMLVFINDITEM * info = reinterpret_cast<NMLVFINDITEM*>(hdr);
		const HWND wnd = hdr->hwndFrom;
		if (info->lvfi.flags & LVFI_NEARESTXY) return -1;
		const size_t count = _ItemCount(wnd);
		if (count == 0) return -1;
		const size_t base = (size_t) info->iStart % count;
		for(size_t walk = 0; walk < count; ++walk) {
			const size_t index = (walk + base) % count;
			for(int subItem = subItemFrom; subItem < subItemFrom + subItemCnt; ++subItem) {
				if (StringPrefixMatch(info->lvfi.psz, _ItemText(wnd, index, subItem))) return (LRESULT) index;
			}
		}
		for(size_t walk = 0; walk < count; ++walk) {
			const size_t index = (walk + base) % count;
			for(int subItem = subItemFrom; subItem < subItemFrom + subItemCnt; ++subItem) {
				if (StringPartialMatch(info->lvfi.psz, _ItemText(wnd, index, subItem))) return (LRESULT) index;
			}
		}
		return -1;
	}

	static wchar_t myCharLower(wchar_t c) {
		return (wchar_t) CharLower((wchar_t*)c);
	}
	static bool StringPrefixMatch(const wchar_t * part, const wchar_t * str) {
		unsigned walk = 0;
		for(;;) {
			wchar_t c1 = part[walk], c2 = str[walk];
			if (c1 == 0) return true;
			if (c2 == 0) return false;
			if (myCharLower(c1) != myCharLower(c2)) return false;
			++walk;
		}
	}

	static bool StringPartialMatch(const wchar_t * part, const wchar_t * str) {
		unsigned base = 0;
		for(;;) {
			unsigned walk = 0;
			for(;;) {
				wchar_t c1 = part[walk], c2 = str[base + walk];
				if (c1 == 0) return true;
				if (c2 == 0) return false;
				if (myCharLower(c1) != myCharLower(c2)) break;
				++walk;
			}
			++ base;
		}
	}

	static size_t _ItemCount(HWND wnd) {
		return ListView_GetItemCount(wnd);
	}
	static const wchar_t * _ItemText(HWND wnd, size_t index, int subItem = 0) {
		NMLVDISPINFO info = {};
		info.hdr.code = LVN_GETDISPINFO;
		info.hdr.idFrom = GetDlgCtrlID(wnd);
		info.hdr.hwndFrom = wnd;
		info.item.iItem = index;
		info.item.iSubItem = subItem;
		info.item.mask = LVIF_TEXT;
		::SendMessage(::GetParent(wnd), WM_NOTIFY, info.hdr.idFrom, reinterpret_cast<LPARAM>(&info));
		if (info.item.pszText == NULL) return L"";
		return info.item.pszText;
	}
	
};


class mutexScope {
public:
	mutexScope(HANDLE hMutex_, abort_callback & abort);
	~mutexScope();
private:
	PFC_CLASS_NOT_COPYABLE_EX(mutexScope);
	HANDLE hMutex;
};

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
	void open( const char * inPath, file::ptr inReader, abort_callback & aborter);
	void close();

	winLocalFileScope() : m_isTemp() {}
	winLocalFileScope( const char * inPath, file::ptr inReader, abort_callback & aborter ) : m_isTemp() {
		open( inPath, inReader, aborter );
	}

	~winLocalFileScope() {
		close();
	}

	const wchar_t * Path() const {return m_path.c_str();}
private:
	bool m_isTemp;
	std::wstring m_path;
};



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
