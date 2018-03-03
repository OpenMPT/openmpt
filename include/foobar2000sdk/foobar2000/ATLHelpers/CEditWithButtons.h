#pragma once

#include <list>
#include <string>
#include <functional>
#include <memory>
#include "WTL-PP.h"

#include "CButtonLite.h"

class CEditWithButtons : public CEditPPHooks {
public:
	CEditWithButtons(::ATL::CMessageMap * hookMM = nullptr, int hookMMID = 0) : CEditPPHooks(hookMM, hookMMID), m_fixedWidth() {}

	enum {
		MSG_CHECKCONDITIONS = WM_USER+13,
		MSG_CHECKCONDITIONS_MAGIC1 = 0xaec66f0c,
		MSG_CHECKCONDITIONS_MAGIC2 = 0x180c2f35,

	};
	BEGIN_MSG_MAP_EX(CEditWithButtons)
		MSG_WM_SETFONT(OnSetFont)
		MSG_WM_WINDOWPOSCHANGED(OnPosChanged)
		MSG_WM_CTLCOLORBTN(OnColorBtn)
		MESSAGE_HANDLER_EX(WM_GETDLGCODE, OnGetDlgCode)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_CHAR(OnChar)
		// MSG_WM_SETFOCUS( OnSetFocus )
		// MSG_WM_KILLFOCUS( OnKillFocus )
		MSG_WM_ENABLE(OnEnable)
		MSG_WM_SETTEXT( OnSetText )
		MESSAGE_HANDLER_EX(WM_PAINT, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(WM_CUT, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(WM_PASTE, CheckConditionsTrigger)
		MESSAGE_HANDLER_EX(MSG_CHECKCONDITIONS, OnCheckConditions)
		CHAIN_MSG_MAP(CEditPPHooks)
	END_MSG_MAP()

	void SubclassWindow( HWND wnd ) {
		CEditPPHooks::SubclassWindow( wnd );
		this->ModifyStyle(0, WS_CLIPCHILDREN);
		RefreshButtons();
	}
	typedef std::function<void () > handler_t;
	typedef std::function<bool (const wchar_t*) > condition_t;

	void AddClearButton() {
		auto handler = [this] {
			this->SetWindowText(L"");
		};
		auto condition = [] ( const wchar_t * txt ) -> bool {
			return * txt != 0;
		};
		AddButton(L"x", handler, condition);
	}

	void AddButton( const wchar_t * str, handler_t handler, condition_t condition = nullptr ) {
		Button_t btn;
		btn.handler = handler;
		btn.title = str;
		btn.condition = condition;
		btn.visible = EvalCondition( btn, nullptr );
		m_buttons.push_back(std::move(btn) );
		RefreshButtons();
	}

	void SetFixedWidth(unsigned fw = GetSystemMetrics(SM_CXVSCROLL) ) {
		m_fixedWidth = fw;
		RefreshButtons();
	}
	CRect RectOfButton( const wchar_t * text ) {
		for ( auto i = m_buttons.begin(); i != m_buttons.end(); ++ i ) {
			if ( i->title == text && i->wnd != NULL ) {
				CRect rc;
				if (i->wnd.GetWindowRect ( rc )) return rc;
			}
		}
		return CRect();
	}
	void Invalidate() {
		__super::Invalidate();
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++i ) {
			if (i->wnd != NULL) i->wnd.Invalidate();
		}
	}
private:
	LRESULT OnCheckConditions( UINT msg, WPARAM wp, LPARAM lp ) {
		if ( msg == MSG_CHECKCONDITIONS && wp == MSG_CHECKCONDITIONS_MAGIC1 && lp == MSG_CHECKCONDITIONS_MAGIC2 ) {
			this->RefreshConditions(nullptr);
		} else {
			SetMsgHandled(0);
		}		
		return 0;
	}
	bool HaveConditions() {
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++i ) {
			if ( i->condition ) return true;
		}
		return false;
	}
	void PostCheckConditions() {
		if ( HaveConditions() ) {
			PostMessage( MSG_CHECKCONDITIONS, MSG_CHECKCONDITIONS_MAGIC1, MSG_CHECKCONDITIONS_MAGIC2 );
		}
	}
	LRESULT CheckConditionsTrigger( UINT, WPARAM, LPARAM ) {
		PostCheckConditions();
		SetMsgHandled(FALSE);
		return 0;
	}
	int OnSetText(LPCTSTR lpstrText) {
		PostCheckConditions();
		SetMsgHandled(FALSE);
		return 0;
	}
	void OnEnable(BOOL bEnable) {
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++ i ) {
			if ( i->wnd != NULL ) {
				i->wnd.EnableWindow( bEnable );
			}
		}
		SetMsgHandled(FALSE); 
	}
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
		if (nChar == VK_TAB ) {
			return;
		}
		PostCheckConditions();
		SetMsgHandled(FALSE);
	}
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
		if ( nChar == VK_TAB ) {
			return;
		}
		SetMsgHandled(FALSE);
	}
	UINT OnGetDlgCode(UINT, WPARAM wp, LPARAM lp) {
		if ( wp == VK_TAB && !IsShiftPressed() && m_buttons.size() > 0 ) {
			auto i = m_buttons.begin();
			if ( i != m_buttons.end() ) {
				TabFocusThis(i->wnd);
			}
			return DLGC_WANTTAB;
		}
		SetMsgHandled(FALSE); return 0;
	}
	void OnSetFocus(HWND) {
		this->ModifyStyleEx(0, WS_EX_CONTROLPARENT ); SetMsgHandled(FALSE);
	}
	void OnKillFocus(HWND) {
		this->ModifyStyleEx(WS_EX_CONTROLPARENT, 0 ); SetMsgHandled(FALSE);

	}
	HBRUSH OnColorBtn(CDCHandle dc, CButton button) {
		if ( (this->GetStyle() & ES_READONLY) != 0 || !this->IsWindowEnabled() ) {
			return (HBRUSH) GetParent().SendMessage( WM_CTLCOLORSTATIC, (WPARAM) dc.m_hDC, (LPARAM) m_hWnd );
		} else {
			return (HBRUSH) GetParent().SendMessage( WM_CTLCOLOREDIT, (WPARAM) dc.m_hDC, (LPARAM) m_hWnd );
		}
	}
	void OnPosChanged(LPWINDOWPOS lpWndPos) {
		Layout(); SetMsgHandled(FALSE);
	}
	
	struct Button_t {
		std::wstring title;
		handler_t handler;
		std::shared_ptr<CButtonLite> buttonImpl;
		CWindow wnd;
		bool visible;
		condition_t condition;
	};
	void OnSetFont(CFontHandle font, BOOL bRedraw) {
		CRect rc;
		if (GetClientRect(&rc)) {
			Layout(rc.Size(), font);
		}
		SetMsgHandled(FALSE);
	}

	void RefreshButtons() {
		if ( m_hWnd != NULL && m_buttons.size() > 0 ) {
			Layout();
		}
	}
	void Layout( ) {
		CRect rc;
		if (GetClientRect(&rc)) {
			Layout(rc.Size(), NULL);
		}
	}
	static bool IsShiftPressed() {
		return (GetKeyState(VK_SHIFT) & 0x8000) ? true : false;
	}
	CWindow FindDialog() {
		return GetParent(); 
	}
	void TabFocusThis(HWND wnd) {
		FindDialog().PostMessage(WM_NEXTDLGCTL, (WPARAM) wnd, TRUE );
	}
	void TabFocusPrevNext(bool bPrev) {
		FindDialog().PostMessage(WM_NEXTDLGCTL, bPrev ? TRUE : FALSE, FALSE);
	}
	void TabCycleButtons(HWND wnd) {
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++ i ) {
			if ( i->wnd == wnd ) {
				if (IsShiftPressed()) {
					// back
					if ( i == m_buttons.begin() ) {
						TabFocusThis(m_hWnd);
					} else {
						--i;
						TabFocusThis(i->wnd);
					}
				} else {
					// forward
					++i;
					if ( i == m_buttons.end() ) {
						TabFocusThis(m_hWnd);
						TabFocusPrevNext(false);
					} else {
						TabFocusThis( i->wnd);
					}
				}

				return;
			}
		}
	}
	bool ButtonWantTab( HWND wnd ) {
		if ( IsShiftPressed() ) return true;
		if ( m_buttons.size() == 0 ) return false; // should not be possible
		auto last = m_buttons.rbegin();
		if ( wnd == last->wnd ) return false; // not for last button
		return true;
	}
	bool EvalCondition( Button_t & btn, const wchar_t * newText ) {
		if (!btn.condition) return true;
		if ( newText != nullptr ) return btn.condition( newText );
		TCHAR text[256] = {};
		GetWindowText(text, 256);
		text[255] = 0;
		return btn.condition( text );
	}
	void RefreshConditions( const wchar_t * newText ) {
		bool changed = false;
		for( auto i = m_buttons.begin(); i != m_buttons.end(); ++i ) {
			bool status = EvalCondition(*i, newText);
			if ( status != i->visible ) {
				i->visible = status; changed = true;
			}
		}
		if ( changed ) {
			Layout();
		}
	}
	void Layout(CSize size, CFontHandle fontSetMe) {
		if ( m_buttons.size() == 0 ) return;

		int walk = size.cx;

		HDWP dwp = BeginDeferWindowPos( (int) m_buttons.size() );
		for( auto iter = m_buttons.rbegin(); iter != m_buttons.rend(); ++iter ) {
			if (! iter->visible ) {
				if (::GetFocus() == iter->wnd ) {
					this->SetFocus();
				}
				::DeferWindowPos( dwp, iter->wnd, NULL, 0,0,0,0, SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE );
				continue;
			}
				
			if (iter->wnd == NULL) {
				auto b = std::make_shared< CButtonLite > ( );
				iter->buttonImpl = b;
				b->Create( *this, NULL, iter->title.c_str() );
				CFontHandle font = fontSetMe;
				if ( font == NULL ) font = GetFont();
				b->SetFont( font );
				b->ClickHandler = iter->handler;
				b->CtlColorHandler = [=] (CDCHandle dc) -> HBRUSH {
					return this->OnColorBtn( dc, NULL );
				};
				b->TabCycleHandler = [=] (HWND wnd) {
					TabCycleButtons(wnd);
				};
				b->WantTabCheck = [=] (HWND wnd) -> bool {
					return ButtonWantTab(wnd);
				};
				if (! IsWindowEnabled( ) ) b->EnableWindow(FALSE);
				iter->wnd = * b;
			} else if ( fontSetMe ) {
				iter->wnd.SetFont( fontSetMe );
			}

			unsigned delta = MeasureButton(*iter);
			int left = walk - delta;

			if ( iter->wnd != NULL ) {
				CRect rc;
				rc.top = 0;
				rc.bottom = size.cy;
				rc.left = left;
				rc.right = walk;
				::DeferWindowPos( dwp, iter->wnd, NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_SHOWWINDOW);
			}

			walk = left;
		}
		EndDeferWindowPos( dwp );
		this->SetMargins(0, size.cx - walk, EC_RIGHTMARGIN );
	}

	unsigned MeasureButton(Button_t const & button ) {
		if ( m_fixedWidth != 0 ) return m_fixedWidth;
		
		return button.buttonImpl->Measure();
	}
	unsigned m_fixedWidth;
	std::list< Button_t > m_buttons;
};
