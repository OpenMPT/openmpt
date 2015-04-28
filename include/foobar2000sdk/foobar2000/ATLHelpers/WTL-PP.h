// Various WTL extensions that are not fb2k specific and can be reused in other WTL based software

#define ATLASSERT_SUCCESS(X) {auto RetVal = (X); ATLASSERT( RetVal ); }

class NoRedrawScope {
public:
	NoRedrawScope(HWND p_wnd) throw() : m_wnd(p_wnd) {
		m_wnd.SetRedraw(FALSE);
	}
	~NoRedrawScope() throw() {
		m_wnd.SetRedraw(TRUE);
	}
private:
	CWindow m_wnd;
};

class NoRedrawScopeEx {
public:
	NoRedrawScopeEx(HWND p_wnd) throw() : m_wnd(p_wnd), m_active() {
		if (m_wnd.IsWindowVisible()) {
			m_active = true;
			m_wnd.SetRedraw(FALSE);
		}
	}
	~NoRedrawScopeEx() throw() {
		if (m_active) {
			m_wnd.SetRedraw(TRUE);
			m_wnd.RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
		}
	}
private:
	bool m_active;
	CWindow m_wnd;
};


#define MSG_WM_TIMER_EX(timerId, func) \
	if (uMsg == WM_TIMER && (UINT_PTR)wParam == timerId) \
	{ \
		SetMsgHandled(TRUE); \
		func(); \
		lResult = 0; \
		if(IsMsgHandled()) \
			return TRUE; \
	}

#define MESSAGE_HANDLER_SIMPLE(msg, func) \
	if(uMsg == msg) \
	{ \
		SetMsgHandled(TRUE); \
		func(); \
		lResult = 0; \
		if(IsMsgHandled()) \
			return TRUE; \
	}

// void OnSysCommandHelp()
#define MSG_WM_SYSCOMMAND_HELP(func) \
	if (uMsg == WM_SYSCOMMAND && wParam == SC_CONTEXTHELP) \
	{ \
		SetMsgHandled(TRUE); \
		func(); \
		lResult = 0; \
		if(IsMsgHandled()) \
			return TRUE; \
	}

//BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID)
#define END_MSG_MAP_HOOK() \
			break; \
		default: \
			return __super::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID); \
		} \
		return FALSE; \
	}


class CImageListContainer : public CImageList {
public:
	CImageListContainer() {}
	~CImageListContainer() {Destroy();}
private:
	const CImageListContainer & operator=(const CImageListContainer&);
	CImageListContainer(const CImageListContainer&);
};


template<bool managed> class CThemeT {
public:
	CThemeT(HTHEME source = NULL) : m_theme(source) {}

	~CThemeT() {
		Release();
	}

	HTHEME OpenThemeData(HWND wnd,LPCWSTR classList) {
		Release();
		return m_theme = ::OpenThemeData(wnd, classList);
	}

	void Release() {
		HTHEME releaseme = pfc::replace_null_t(m_theme);
		if (managed && releaseme != NULL) CloseThemeData(releaseme);
	}

	operator HTHEME() const {return m_theme;}
	HTHEME m_theme;
};
typedef CThemeT<false> CThemeHandle;
typedef CThemeT<true> CTheme;


class CCheckBox : public CButton {
public:
	void ToggleCheck(bool state) {SetCheck(state ? BST_CHECKED : BST_UNCHECKED);}
	bool IsChecked() const {return GetCheck() == BST_CHECKED;}

	CCheckBox(HWND hWnd = NULL) : CButton(hWnd) { }
	CCheckBox & operator=(HWND wnd) {m_hWnd = wnd; return *this; }
};


class CEditNoEscSteal : public CContainedWindowT<CEdit>, private CMessageMap {
public:
	CEditNoEscSteal() : CContainedWindowT<CEdit>(this, 0) {}
	BEGIN_MSG_MAP_EX(CEditNoEscSteal)
		MSG_WM_GETDLGCODE(OnEditGetDlgCode)
	END_MSG_MAP()
private:
	UINT OnEditGetDlgCode(LPMSG lpMsg) {
		if (lpMsg == NULL) {
			SetMsgHandled(FALSE); return 0;
		} else {
			switch(lpMsg->message) {
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					switch(lpMsg->wParam) {
						case VK_ESCAPE:
							return 0;
						default:
							SetMsgHandled(FALSE); return 0;
					}
				default:
					SetMsgHandled(FALSE); return 0;

			}
		}
	}
};

class CEditNoEnterEscSteal : public CContainedWindowT<CEdit>, private CMessageMap {
public:
	CEditNoEnterEscSteal() : CContainedWindowT<CEdit>(this, 0) {}
	BEGIN_MSG_MAP_EX(CEditNoEscSteal)
		MSG_WM_GETDLGCODE(OnEditGetDlgCode)
	END_MSG_MAP()
private:
	UINT OnEditGetDlgCode(LPMSG lpMsg) {
		if (lpMsg == NULL) {
			SetMsgHandled(FALSE); return 0;
		} else {
			switch(lpMsg->message) {
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					switch(lpMsg->wParam) {
						case VK_ESCAPE:
						case VK_RETURN:
							return 0;
						default:
							SetMsgHandled(FALSE); return 0;
					}
				default:
					SetMsgHandled(FALSE); return 0;

			}
		}
	}
};



class CWindowClassUnregisterScope {
public:
	CWindowClassUnregisterScope() : name() {}
	const TCHAR * name;
	void Set(const TCHAR * n) {ATLASSERT( name == NULL ); name = n; }
	bool IsActive() const {return name != NULL;}
	void CleanUp() {
		const TCHAR * n = name; name = NULL;
		if (n != NULL) ATLASSERT_SUCCESS( UnregisterClass(n, (HINSTANCE)&__ImageBase) );
	}
	~CWindowClassUnregisterScope() {CleanUp();}
};


// CWindowRegisteredT
// Minimalistic wrapper for registering own window classes that can be created by class name, included in dialogs and such.
// Usage:
// class myClass : public CWindowRegisteredT<myClass> {...};
// Call myClass::Register() before first use
template<typename TClass, typename TBaseClass = CWindow>
class CWindowRegisteredT : public TBaseClass {
public:
	static UINT GetClassStyle() {
		return CS_VREDRAW | CS_HREDRAW;
	}
	static HCURSOR GetCursor() {
		return ::LoadCursor(NULL, IDC_ARROW);
	}

	BEGIN_MSG_MAP_EX(CWindowRegisteredT)
	END_MSG_MAP()


	static void Register() {
		static CWindowClassUnregisterScope scope;
		if (!scope.IsActive()) {
			WNDCLASS wc = {};
			wc.style = TClass::GetClassStyle();
			wc.cbWndExtra = sizeof(void*);
			wc.lpszClassName = TClass::GetClassName();
			wc.lpfnWndProc = myWindowProc;
			wc.hInstance = (HINSTANCE)&__ImageBase;
			wc.hCursor = TClass::GetCursor();
			ATLASSERT_SUCCESS( RegisterClass(&wc) != 0 );
			scope.Set(wc.lpszClassName);
		}
	}
protected:
	virtual ~CWindowRegisteredT() {}
private:
	static LRESULT CALLBACK myWindowProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
		TClass * i = NULL;
		if (msg == WM_NCCREATE) {
			TClass * i = new TClass;
			i->Attach(wnd);
			::SetWindowLongPtr(wnd, 0, reinterpret_cast<LONG_PTR>(i));
		} else {
			i = reinterpret_cast<TClass*>( ::GetWindowLongPtr(wnd, 0) );
		}
		LRESULT r;
		if (i == NULL || !i->ProcessWindowMessage(wnd, msg, wp, lp, r)) r = ::DefWindowProc(wnd, msg, wp, lp);
		if (msg == WM_NCDESTROY) {
			::SetWindowLongPtr(wnd, 0, 0);
			delete i;
		}
		return r;
	}
};




class CSRWlock {
public:
	CSRWlock() : theLock() {
#if _WIN32_WINNT < 0x600
		auto dll = GetModuleHandle(_T("kernel32"));
		Bind(AcquireSRWLockExclusive, dll, "AcquireSRWLockExclusive");
		Bind(AcquireSRWLockShared, dll, "AcquireSRWLockShared");
		Bind(ReleaseSRWLockExclusive, dll, "ReleaseSRWLockExclusive");
		Bind(ReleaseSRWLockShared, dll, "ReleaseSRWLockShared");
#endif
	}
	
	bool HaveAPI() {
#if _WIN32_WINNT < 0x600
		return AcquireSRWLockExclusive != NULL;
#else
		return true;
#endif
	}

	void EnterShared() {
		AcquireSRWLockShared( & theLock );
	}
	void EnterExclusive() {
		AcquireSRWLockExclusive( & theLock );
	}
	void LeaveShared() {
		ReleaseSRWLockShared( & theLock );
	}
	void LeaveExclusive() {
		ReleaseSRWLockExclusive( &theLock );
	}

private:
	CSRWlock(const CSRWlock&);
	void operator=(const CSRWlock&);

	SRWLOCK theLock;
#if _WIN32_WINNT < 0x600
	template<typename func_t> static void Bind(func_t & func, HMODULE dll, const char * name) {
		func = reinterpret_cast<func_t>(GetProcAddress( dll, name ) );
	}

	VOID (WINAPI * AcquireSRWLockExclusive)(PSRWLOCK SRWLock);
	VOID (WINAPI * AcquireSRWLockShared)(PSRWLOCK SRWLock);
	VOID (WINAPI * ReleaseSRWLockExclusive)(PSRWLOCK SRWLock);
	VOID (WINAPI * ReleaseSRWLockShared)(PSRWLOCK SRWLock);
#endif
};

#if _WIN32_WINNT < 0x600
class CSRWorCS {
public:
	CSRWorCS() : cs() {
		if (!srw.HaveAPI()) InitializeCriticalSection(&cs);
	}
	~CSRWorCS() {
		if (!srw.HaveAPI()) DeleteCriticalSection(& cs );
	}
	void EnterShared() {
		if (srw.HaveAPI()) srw.EnterShared();
		else EnterCriticalSection(&cs);
	}
	void EnterExclusive() {
		if (srw.HaveAPI()) srw.EnterExclusive();
		else EnterCriticalSection(&cs);
	}
	void LeaveShared() {
		if (srw.HaveAPI()) srw.LeaveShared();
		else LeaveCriticalSection(&cs);
	}
	void LeaveExclusive() {
		if (srw.HaveAPI()) srw.LeaveExclusive();
		else LeaveCriticalSection(&cs);
	}
private:
	CSRWorCS(const CSRWorCS&);
	void operator=(const CSRWorCS&);

	CSRWlock srw;
	CRITICAL_SECTION cs;
};
#else
typedef CSRWlock CSRWorCS;
#endif
