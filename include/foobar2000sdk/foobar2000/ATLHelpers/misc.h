class CMenuSelectionReceiver : public CWindowImpl<CMenuSelectionReceiver> {
public:
	CMenuSelectionReceiver(HWND p_parent) {
		WIN32_OP( Create(p_parent) != NULL );
	}
	~CMenuSelectionReceiver() {
		if (m_hWnd != NULL) DestroyWindow();
	}
	typedef CWindowImpl<CMenuSelectionReceiver> _baseClass;
	DECLARE_WND_CLASS_EX(TEXT("{DF0087DB-E765-4283-BBAB-6AB2E8AB64A1}"),0,0);

	BEGIN_MSG_MAP(CMenuSelectionReceiver)
		MESSAGE_HANDLER(WM_MENUSELECT,OnMenuSelect)
	END_MSG_MAP()
protected:
	virtual bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return false;
	}
private:
	LRESULT OnMenuSelect(UINT,WPARAM p_wp,LPARAM p_lp,BOOL&) {
		if (p_lp != 0) {
			if (HIWORD(p_wp) & MF_POPUP) {
				m_status.release();
			} else {
				pfc::string8 msg;
				if (!QueryHint(LOWORD(p_wp),msg)) {
					m_status.release();
				} else {
					if (m_status.is_empty()) {
						if (!static_api_ptr_t<ui_control>()->override_status_text_create(m_status)) m_status.release();
					}
					if (m_status.is_valid()) {
						m_status->override_text(msg);
					}
				}
			}
		} else {
			m_status.release();
		}
		return 0;
	}

	service_ptr_t<ui_status_text_override> m_status;

	PFC_CLASS_NOT_COPYABLE(CMenuSelectionReceiver,CMenuSelectionReceiver);
};

class CMenuDescriptionMap : public CMenuSelectionReceiver {
public:
	CMenuDescriptionMap(HWND p_parent) : CMenuSelectionReceiver(p_parent) {}
	void Set(unsigned p_id,const char * p_description) {m_content.set(p_id,p_description);}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return m_content.query(p_id,p_out);
	}
private:
	pfc::map_t<unsigned,pfc::string8> m_content;
};

class CMenuDescriptionHybrid : public CMenuSelectionReceiver {
public:
	CMenuDescriptionHybrid(HWND parent) : CMenuSelectionReceiver(parent) {}
	void Set(unsigned id, const char * desc) {m_content.set(id, desc);}

	void SetCM(contextmenu_manager::ptr mgr, unsigned base, unsigned max) {
		m_cmMgr = mgr; m_cmMgr_base = base; m_cmMgr_max = max;
	}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		if (m_cmMgr.is_valid() && p_id >= m_cmMgr_base && p_id < m_cmMgr_max) {
			return m_cmMgr->get_description_by_id(p_id - m_cmMgr_base,p_out);
		}
		return m_content.query(p_id,p_out);
	}
private:
	pfc::map_t<unsigned,pfc::string8> m_content;
	contextmenu_manager::ptr m_cmMgr; unsigned m_cmMgr_base, m_cmMgr_max;
};

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,const CPoint & p_point) {
	return p_fmt << "(" << p_point.x << "," << p_point.y << ")";
}

inline pfc::string_base & operator<<(pfc::string_base & p_fmt,const CRect & p_rect) {
	return p_fmt << "(" << p_rect.left << "," << p_rect.top << "," << p_rect.right << "," << p_rect.bottom << ")";
}

template<typename TClass>
class CAddDummyMessageMap : public TClass {
public:
	BEGIN_MSG_MAP(CAddDummyMessageMap<TClass>)
	END_MSG_MAP()
};

template<typename _parentClass> class CWindowFixSEH : public _parentClass { public:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) {
		__try {
			return _parentClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID);
		} __except(uExceptFilterProc(GetExceptionInformation())) { return FALSE; /* should not get here */ }
	}
	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(CWindowFixSEH, _parentClass);
};

template<typename TClass>
class CWindowAutoLifetime : public CWindowFixSEH<TClass> {
public:
	typedef CWindowFixSEH<TClass> TBase;
	CWindowAutoLifetime(HWND parent) : TBase() {Init(parent);}
	template<typename TParam1> CWindowAutoLifetime(HWND parent, const TParam1 & p1) : TBase(p1) {Init(parent);}
	template<typename TParam1,typename TParam2> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2) : TBase(p1,p2) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3) : TBase(p1,p2,p3) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3, typename TParam4> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3,const TParam4 & p4) : TBase(p1,p2,p3,p4) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3, typename TParam4,typename TParam5> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3,const TParam4 & p4, const TParam5 & p5) : TBase(p1,p2,p3,p4,p5) {Init(parent);}
	template<typename TParam1,typename TParam2, typename TParam3, typename TParam4,typename TParam5,typename TParam6> CWindowAutoLifetime(HWND parent, const TParam1 & p1,const TParam2 & p2,const TParam3 & p3,const TParam4 & p4, const TParam5 & p5, const TParam6 & p6) : TBase(p1,p2,p3,p4,p5,p6) {Init(parent);}
private:
	void Init(HWND parent) {WIN32_OP(this->Create(parent) != NULL);}
	void OnFinalMessage(HWND wnd) {PFC_ASSERT_NO_EXCEPTION( TBase::OnFinalMessage(wnd) ); PFC_ASSERT_NO_EXCEPTION(delete this);}
};

template<typename TClass>
class ImplementModelessTracking : public TClass {
public:
	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD(ImplementModelessTracking, TClass);
	
	BEGIN_MSG_MAP_EX(ImplementModelessTracking)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(TClass)
	END_MSG_MAP_HOOK()
private:
	BOOL OnInitDialog(CWindow, LPARAM) {m_modeless.Set( m_hWnd ); SetMsgHandled(FALSE); return FALSE; }
	void OnDestroy() {m_modeless.Set(NULL); SetMsgHandled(FALSE); }
	CModelessDialogEntry m_modeless;
};

class CMenuSelectionReceiver_UiElement : public CMenuSelectionReceiver {
public:
	CMenuSelectionReceiver_UiElement(service_ptr_t<ui_element_instance> p_owner,unsigned p_id_base) : CMenuSelectionReceiver(p_owner->get_wnd()), m_owner(p_owner), m_id_base(p_id_base) {}
protected:
	bool QueryHint(unsigned p_id,pfc::string_base & p_out) {
		return m_owner->edit_mode_context_menu_get_description(p_id,m_id_base,p_out);
	}
private:
	const unsigned m_id_base;
	const service_ptr_t<ui_element_instance> m_owner;
};

static void ui_element_instance_standard_context_menu(service_ptr_t<ui_element_instance> p_elem, LPARAM p_pt) {
	CPoint pt;
	bool fromKeyboard;
	if (p_pt == -1) {
		fromKeyboard = true;
		if (!p_elem->edit_mode_context_menu_get_focus_point(pt)) {
			CRect rc;
			WIN32_OP_D( GetWindowRect(p_elem->get_wnd(), rc) );
			pt = rc.CenterPoint();
		}
	} else {
		fromKeyboard = false;
		pt = p_pt;
	}
	if (p_elem->edit_mode_context_menu_test(pt,fromKeyboard)) {
		const unsigned idBase = 1;
		CMenu menu;
		WIN32_OP( menu.CreatePopupMenu() );
		p_elem->edit_mode_context_menu_build(pt,fromKeyboard,menu,idBase);
		
		int cmd;
		{
			CMenuSelectionReceiver_UiElement receiver(p_elem,idBase);
			cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,receiver);
		}
		if (cmd > 0) p_elem->edit_mode_context_menu_command(pt,fromKeyboard,cmd,idBase);
	}
}
static void ui_element_instance_standard_context_menu_eh(service_ptr_t<ui_element_instance> p_elem, LPARAM p_pt) {
	try {
		ui_element_instance_standard_context_menu(p_elem, p_pt);
	} catch(std::exception const & e) {
		console::complain("Context menu failure", e);
	}
}


#if _WIN32_WINNT >= 0x501
static void HeaderControl_SetSortIndicator(CHeaderCtrl header, int column, bool isUp) {
	const int total = header.GetItemCount();
	for(int walk = 0; walk < total; ++walk) {
		HDITEM item = {}; item.mask = HDI_FORMAT;
		if (header.GetItem(walk,&item)) {
			DWORD newFormat = item.fmt;
			newFormat &= ~( HDF_SORTUP | HDF_SORTDOWN );
			if (walk == column) {
				newFormat |= isUp ? HDF_SORTUP : HDF_SORTDOWN;
			}
			if (newFormat != item.fmt) {
				item.fmt = newFormat;
				header.SetItem(walk,&item);
			}
		}
	}
}
#endif

typedef CWinTraits<WS_POPUP,WS_EX_TRANSPARENT|WS_EX_LAYERED|WS_EX_TOPMOST|WS_EX_TOOLWINDOW> CFlashWindowTraits;

class CFlashWindow : public CWindowImpl<CFlashWindow,CWindow,CFlashWindowTraits> {
public:
	void Activate(CWindow parent) {
		ShowAbove(parent);
		m_tickCount = 0;
		SetTimer(KTimerID, 500);
	}
	void Deactivate() throw() {
		ShowWindow(SW_HIDE); KillTimer(KTimerID);
	}

	void ShowAbove(CWindow parent) {
		if (m_hWnd == NULL) {
			WIN32_OP( Create(NULL) != NULL );
		}
		CRect rect;
		WIN32_OP_D( parent.GetWindowRect(rect) );
		WIN32_OP_D( SetWindowPos(NULL,rect,SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW) );
		m_parent = parent;
	}

	void CleanUp() throw() {
		if (m_hWnd != NULL) DestroyWindow();
	}

	BEGIN_MSG_MAP_EX(CFlashWindow)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_TIMER(OnTimer)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

	DECLARE_WND_CLASS_EX(TEXT("{2E124D52-131F-4004-A569-2316615BE63F}"),0,COLOR_HIGHLIGHT);
private:
	void OnDestroy() throw() {
		KillTimer(KTimerID);
	}
	enum {
		KTimerID = 0x47f42dd0
	};
	void OnTimer(WPARAM id) {
		if (id == KTimerID) {
			switch(++m_tickCount) {
				case 1:
					ShowWindow(SW_HIDE);
					break;
				case 2:
					ShowAbove(m_parent);
					break;
				case 3:
					ShowWindow(SW_HIDE);
					KillTimer(KTimerID);
					break;
			}
		}
	}
	LRESULT OnCreate(LPCREATESTRUCT) throw() {
		SetLayeredWindowAttributes(*this,0,128,LWA_ALPHA);
		return 0;
	}
	CWindow m_parent;
	t_uint32 m_tickCount;
};

class CTypableWindowScope {
public:
	CTypableWindowScope() : m_wnd() {}
	~CTypableWindowScope() {Set(NULL);}
	void Set(HWND wnd) {
		try {
			if (m_wnd != NULL) {
				static_api_ptr_t<ui_element_typable_window_manager>()->remove(m_wnd);
			}
			m_wnd = wnd;
			if (m_wnd != NULL) {
				static_api_ptr_t<ui_element_typable_window_manager>()->add(m_wnd);
			}
		} catch(exception_service_not_found) {
			m_wnd = NULL;
		}
	}

private:
	HWND m_wnd;
	PFC_CLASS_NOT_COPYABLE_EX(CTypableWindowScope);
};


template<typename TBase> class CContainedWindowSimpleT : public CContainedWindowT<TBase>, public CMessageMap {
public:
	CContainedWindowSimpleT() : CContainedWindowT<TBase>(this) {}
	BEGIN_MSG_MAP(CContainedWindowSimpleT)
	END_MSG_MAP()
};


static bool window_service_trait_defer_destruction(const service_base *) {return true;}


//! Special service_impl_t replacement for service classes that also implement ATL/WTL windows.
template<typename _t_base>
class window_service_impl_t : public CWindowFixSEH<_t_base> {
private:
	typedef window_service_impl_t<_t_base> t_self;
	typedef CWindowFixSEH<_t_base> t_base;
public:
	BEGIN_MSG_MAP_EX(window_service_impl_t)
		MSG_WM_DESTROY(OnDestroyPassThru)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP_HOOK()

	int FB2KAPI service_release() throw() {
		int ret = --m_counter; 
		if (ret == 0) {
			if (window_service_trait_defer_destruction(this) && !InterlockedExchange(&m_delayedDestroyInProgress,1)) {
				PFC_ASSERT_NO_EXCEPTION( service_impl_helper::release_object_delayed(this); );
			} else if (m_hWnd != NULL) {
				if (!m_destroyWindowInProgress) { // don't double-destroy in weird scenarios
					PFC_ASSERT_NO_EXCEPTION( ::DestroyWindow(m_hWnd) );
				}
			} else {
				PFC_ASSERT_NO_EXCEPTION( delete this );
			}
		}
		return ret;
	}
	int FB2KAPI service_add_ref() throw() {return ++m_counter;}

	TEMPLATE_CONSTRUCTOR_FORWARD_FLOOD_WITH_INITIALIZER(window_service_impl_t,t_base,{m_destroyWindowInProgress = false; m_delayedDestroyInProgress = 0; })
private:
	void OnDestroyPassThru() {
		SetMsgHandled(FALSE); m_destroyWindowInProgress = true;
	}
	void OnFinalMessage(HWND p_wnd) {
		t_base::OnFinalMessage(p_wnd);
		service_ptr_t<service_base> bump(this);
	}
	volatile bool m_destroyWindowInProgress;
	volatile LONG m_delayedDestroyInProgress;
	pfc::refcounter m_counter;
};


static void AppendMenuPopup(HMENU menu, UINT flags, CMenu & popup, const TCHAR * label) {
	PFC_ASSERT( flags & MF_POPUP );
	WIN32_OP( CMenuHandle(menu).AppendMenu(flags, popup, label) );
	popup.Detach();
}

class CMessageMapDummy : public CMessageMap { 
public:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		LRESULT& lResult, DWORD dwMsgMapID) {return FALSE;}
};

class CPopupTooltipMessage {
public:
	CPopupTooltipMessage(DWORD style = TTS_BALLOON | TTS_NOPREFIX) : m_style(style | WS_POPUP), m_toolinfo(), m_shutDown() {}
	void ShowFocus(const TCHAR * message, CWindow wndParent) {
		Show(message, wndParent); wndParent.SetFocus();
	}
	void Show(const TCHAR * message, CWindow wndParent) {
		if (m_shutDown || (message == NULL && m_tooltip.m_hWnd == NULL)) return;
		Initialize();
		Hide();
		
		if (message != NULL) {
			CRect rect;
			WIN32_OP_D( wndParent.GetWindowRect(rect) );
			ShowInternal(message, wndParent, rect);
		}
	}
	void ShowEx(const TCHAR * message, CWindow wndParent, CRect rect) {
		if (m_shutDown) return;
		Initialize();
		Hide();
		ShowInternal(message, wndParent, rect);
	}
	void Hide() {
		if (m_tooltip.m_hWnd != NULL && m_tooltip.GetToolCount() > 0) {
			m_tooltip.TrackActivate(&m_toolinfo,FALSE);
			m_tooltip.DelTool(&m_toolinfo);
		}
	}

	void CleanUp() {
		if (m_tooltip.m_hWnd != NULL) {
			m_tooltip.DestroyWindow();
		}
	}
	void ShutDown() {
		m_shutDown = true; CleanUp();
	}
private:
	void ShowInternal(const TCHAR * message, CWindow wndParent, CRect rect) {
		PFC_ASSERT( !m_shutDown );
		PFC_ASSERT( message != NULL );
		PFC_ASSERT( wndParent != NULL );
		m_toolinfo.cbSize = sizeof(m_toolinfo);
		m_toolinfo.uFlags = TTF_TRACK|TTF_IDISHWND|TTF_ABSOLUTE|TTF_TRANSPARENT|TTF_CENTERTIP;
		m_toolinfo.hwnd = wndParent;
		m_toolinfo.uId = 0;
		m_toolinfo.lpszText = const_cast<TCHAR*>(message);
		m_toolinfo.hinst = NULL; //core_api::get_my_instance();
		if (m_tooltip.AddTool(&m_toolinfo)) {
			m_tooltip.TrackPosition(rect.CenterPoint().x,rect.bottom);
			m_tooltip.TrackActivate(&m_toolinfo,TRUE);
		}
	}
	void Initialize() {
		if (m_tooltip.m_hWnd == NULL) {
			WIN32_OP( m_tooltip.Create( NULL , NULL, NULL, m_style) );
		}
	}
	CContainedWindowSimpleT<CToolTipCtrl> m_tooltip;
	TOOLINFO m_toolinfo;
	const DWORD m_style;
	bool m_shutDown;
};


template<typename T> class CDialogWithTooltip : public CDialogImpl<T> {
public:
	BEGIN_MSG_MAP(CDialogWithTooltip)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

	void ShowTip(UINT id, const TCHAR * label) {
		m_tip.Show(label, GetDlgItem(id));
	}
	void ShowTip(HWND child, const TCHAR * label) {
		m_tip.Show(label, child);
	}

	void ShowTipF(UINT id, const TCHAR * label) {
		m_tip.ShowFocus(label, GetDlgItem(id));
	}
	void ShowTipF(HWND child, const TCHAR * label) {
		m_tip.ShowFocus(label, child);
	}
	void HideTip() {m_tip.Hide();}
private:
	void OnDestroy() {m_tip.ShutDown(); SetMsgHandled(FALSE); }
	CPopupTooltipMessage m_tip;
};








static void ListView_FixContextMenuPoint(CListViewCtrl list,CPoint & coords) {
	if (coords == CPoint(-1,-1)) {
		int selWalk = -1;
		CRect rcClient; WIN32_OP_D(list.GetClientRect(rcClient));
		for(;;) {
			selWalk = list.GetNextItem(selWalk, LVNI_SELECTED);
			if (selWalk < 0) {
				CRect rc;
				WIN32_OP_D( list.GetWindowRect(&rc) );
				coords = rc.CenterPoint();
				return;
			}
			CRect rcItem, rcVisible;
			WIN32_OP_D( list.GetItemRect(selWalk, &rcItem, LVIR_BOUNDS) );
			if (rcVisible.IntersectRect(rcItem, rcClient)) {
				coords = rcVisible.CenterPoint();
				WIN32_OP_D( list.ClientToScreen(&coords) );
				return;
			}
		}
	}
}


template<typename TDialog> class preferences_page_instance_impl : public TDialog {
public:
	preferences_page_instance_impl(HWND parent, preferences_page_callback::ptr callback) : TDialog(callback) {WIN32_OP(this->Create(parent) != NULL);}
	HWND get_wnd() {return this->m_hWnd;}
};
static bool window_service_trait_defer_destruction(const preferences_page_instance *) {return false;}
template<typename TDialog> class preferences_page_impl : public preferences_page_v3 {
public:
	preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback) {
		return new window_service_impl_t<preferences_page_instance_impl<TDialog> >(parent, callback);
	}
};

class CEmbeddedDialog : public CDialogImpl<CEmbeddedDialog> {
public:
	CEmbeddedDialog(CMessageMap * owner, DWORD msgMapID, UINT dialogID) : m_owner(*owner), IDD(dialogID), m_msgMapID(msgMapID) {}
	
	BEGIN_MSG_MAP(CEmbeddedDialog)
		CHAIN_MSG_MAP_ALT_MEMBER(m_owner, m_msgMapID)
	END_MSG_MAP()

	const DWORD m_msgMapID;
	const UINT IDD;
	CMessageMap & m_owner;
};


void PaintSeparatorControl(CWindow wnd);

class CStaticSeparator : public CContainedWindowT<CStatic>, private CMessageMap {
public:
	CStaticSeparator() : CContainedWindowT<CStatic>(this, 0) {}
	BEGIN_MSG_MAP_EX(CSeparator)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_SETTEXT(OnSetText)
	END_MSG_MAP()
private:
	int OnSetText(LPCTSTR lpstrText) {
		Invalidate();
		SetMsgHandled(FALSE);
		return 0;
	}
	void OnPaint(CDCHandle) {
		PaintSeparatorControl(*this);
	}
};



template<typename TClass>
class CTextControl : public CWindowRegisteredT<TClass> {
public:
	BEGIN_MSG_MAP_EX(CTextControl)
		MSG_WM_SETFONT(OnSetFont)
		MSG_WM_GETFONT(OnGetFont)
		MSG_WM_SETTEXT(OnSetText)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP()
private:
	HFONT OnGetFont() {
		return m_font;
	}
	void OnSetFont(HFONT font, BOOL bRedraw) {
		m_font = font;
		if (bRedraw) Invalidate();
	}
	int OnSetText(LPCTSTR lpstrText) {
		Invalidate();SetMsgHandled(FALSE); return 0;
	}
	CFontHandle m_font;
};

#ifndef VSCLASS_TEXTSTYLE
//
//  TEXTSTYLE class parts and states 
//
#define VSCLASS_TEXTSTYLE	L"TEXTSTYLE"

enum TEXTSTYLEPARTS {
	TEXT_MAININSTRUCTION = 1,
	TEXT_INSTRUCTION = 2,
	TEXT_BODYTITLE = 3,
	TEXT_BODYTEXT = 4,
	TEXT_SECONDARYTEXT = 5,
	TEXT_HYPERLINKTEXT = 6,
	TEXT_EXPANDED = 7,
	TEXT_LABEL = 8,
	TEXT_CONTROLLABEL = 9,
};

enum HYPERLINKTEXTSTATES {
	TS_HYPERLINK_NORMAL = 1,
	TS_HYPERLINK_HOT = 2,
	TS_HYPERLINK_PRESSED = 3,
	TS_HYPERLINK_DISABLED = 4,
};

enum CONTROLLABELSTATES {
	TS_CONTROLLABEL_NORMAL = 1,
	TS_CONTROLLABEL_DISABLED = 2,
};

#endif


class CStaticThemed : public CContainedWindowT<CStatic>, private CMessageMap {
public:
	CStaticThemed() : CContainedWindowT<CStatic>(this, 0), m_id(), m_fallback() {}
	BEGIN_MSG_MAP_EX(CStaticThemed)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_THEMECHANGED(OnThemeChanged)
		MSG_WM_SETTEXT(OnSetText)
	END_MSG_MAP()

	void SetThemePart(int id) {m_id = id; if (m_hWnd != NULL) Invalidate();}
private:
	int OnSetText(LPCTSTR lpstrText) {
		Invalidate();
		SetMsgHandled(FALSE);
		return 0;
	}
	void OnThemeChanged() {
		m_theme.Release();
		m_fallback = false;
	}
	void OnPaint(CDCHandle) {
		if (m_fallback) {
			SetMsgHandled(FALSE); return;
		}
		if (m_theme == NULL) {
			m_theme.OpenThemeData(*this, L"TextStyle");
			if (m_theme == NULL) {
				m_fallback = true; SetMsgHandled(FALSE); return;
			}
		}
		CPaintDC dc(*this);
		TCHAR buffer[512] = {};
		GetWindowText(buffer, _countof(buffer));
		const int txLen = pfc::strlen_max_t(buffer, _countof(buffer));
		CRect contentRect;
		WIN32_OP_D( GetClientRect(contentRect) );
		SelectObjectScope scopeFont(dc, GetFont());
		dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
		dc.SetBkMode(TRANSPARENT);
		
		if (txLen > 0) {
			CRect rcText(contentRect);
			DWORD flags = 0;
			DWORD style = GetStyle();
			if (style & SS_LEFT) flags |= DT_LEFT;
			else if (style & SS_RIGHT) flags |= DT_RIGHT;
			else if (style & SS_CENTER) flags |= DT_CENTER;
			if (style & SS_ENDELLIPSIS) flags |= DT_END_ELLIPSIS;

			HRESULT retval = DrawThemeText(m_theme, dc, m_id, 0, buffer, txLen, flags, 0, rcText);
			PFC_ASSERT( SUCCEEDED( retval ) );
		}		
	}
	int m_id;
	CTheme m_theme;
	bool m_fallback;
};
class CStaticMainInstruction : public CStaticThemed {
public:
	CStaticMainInstruction() { SetThemePart(TEXT_MAININSTRUCTION); }
};



class CSeparator : public CTextControl<CSeparator> {
public:
	BEGIN_MSG_MAP_EX(CSeparator)
		MSG_WM_PAINT(OnPaint)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP()

	static const TCHAR * GetClassName() {
		return _T("foobar2000:separator");
	}
private:
	void OnPaint(CDCHandle dc) {
		PaintSeparatorControl(*this);
	}
};

