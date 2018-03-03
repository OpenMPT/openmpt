#pragma once

#include <functional>
#include "WTL-PP.h"

typedef CWinTraits<WS_CHILD|WS_TABSTOP,0> CButtonLiteTraits;

class CButtonLite : public CWindowImpl<CButtonLite, CWindow, CButtonLiteTraits > {
public:
	CButtonLite() : m_pressed(), m_captured(), m_focused() {}
	BEGIN_MSG_MAP_EX(CButtonLite)
		MSG_WM_SETTEXT(OnSetText)
		MSG_WM_PAINT( OnPaint )
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_LBUTTONUP(OnLButtonUp)
		MSG_WM_SETFOCUS(OnSetFocus)
		MSG_WM_KILLFOCUS(OnKillFocus)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_KEYUP(OnKeyUp)
		MSG_WM_CHAR(OnChar)
		MSG_WM_ENABLE(OnEnable)
		MESSAGE_HANDLER_EX(WM_GETDLGCODE, OnGetDlgCode)
		MSG_WM_SETFONT(OnSetFont)
		MSG_WM_GETFONT(OnGetFont)
		MSG_WM_CREATE(OnCreate)
	END_MSG_MAP()
	std::function<void () > ClickHandler;

	unsigned Measure() {
		auto font = myGetFont();
		LOGFONT lf;
		font.GetLogFont( lf );
		MakeBoldFont( lf );
		CFont bold;
		bold.CreateFontIndirect(&lf);
		CWindowDC dc(*this);
		auto oldFont = dc.SelectFont( bold );
		CSize size (0,0);
		CSize sizeSpace (0,0);


		dc.GetTextExtent(m_textDrawMe, m_textDrawMe.GetLength(), &size);
		dc.GetTextExtent(L" ", 1, &sizeSpace);

		dc.SelectFont( oldFont );

		return size.cx + sizeSpace.cx;
	}
	std::function< void (HWND) > TabCycleHandler;
	std::function< HBRUSH (CDCHandle) > CtlColorHandler;
	std::function< bool (HWND) > WantTabCheck;
	CWindow WndCtlColorTarget;

	// Rationale: sometimes you want a different text to be presented to accessibility APIs than actually drawn
	// For an example, a clear button looks best with a multiplication sign, but the narrator should say "clear" or so when focused
	void DrawAlternateText( const wchar_t * textDrawMe ) {
		m_textDrawMe = textDrawMe;
	}

protected:
	CFontHandle m_font;
	void OnSetFont(HFONT font, BOOL bRedraw) {
		m_font = font; if (bRedraw) Invalidate();
	}
	HFONT OnGetFont() {
		return m_font;
	}
	LRESULT OnGetDlgCode(UINT, WPARAM wp, LPARAM lp) {
		if ( wp == VK_TAB && TabCycleHandler != NULL) {
			if ( WantTabCheck == NULL || WantTabCheck(m_hWnd) ) {
				TabCycleHandler( m_hWnd );
				return DLGC_WANTTAB;
			}
		}
		SetMsgHandled(FALSE); return 0;
	}
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
	}
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
		switch(nChar) {
		case VK_SPACE:
		case VK_RETURN:
			TogglePressed(true); break;
		}
	}
	void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) {
		switch(nChar) {
		case VK_SPACE:
		case VK_RETURN:
			TogglePressed(false);
			OnClicked(); 
			break;
		}
	}
	void OnSetFocus(CWindow wndOld) {
		m_focused = true; Invalidate();
	}
	void OnKillFocus(CWindow wndFocus) {
		m_focused = false; Invalidate();
	}
	CFontHandle myGetFont() {
		auto f = GetFont();
		if ( f == NULL ) f = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
		return f;
	}
	static void MakeBoldFont(LOGFONT & lf ) {
		lf.lfWeight += 300;
		if (lf.lfWeight > 1000 ) lf.lfWeight = 1000;
	}
	virtual void DrawBackground( CDCHandle dc, CRect rcClient ) {
		HBRUSH brush;
		if ( CtlColorHandler ) {
			brush = CtlColorHandler( dc );
		} else {
			CWindow target = WndCtlColorTarget;
			if ( target == NULL ) target = GetParent();
			brush = (HBRUSH) target.SendMessage(WM_CTLCOLORBTN, (WPARAM) dc.m_hDC, (LPARAM) this->m_hWnd );
		}
		if ( brush != NULL ) {
			dc.FillRect( rcClient, brush );
			dc.SetBkMode( TRANSPARENT );
		}
	}
	virtual void OnPaint(CDCHandle) {
		CPaintDC pdc(*this);

		CRect rcClient;
		if (! GetClientRect( &rcClient ) ) return;

		auto font = myGetFont();
		CFont fontOverride;
		if ( IsPressed() ) {
			LOGFONT lf;
			font.GetLogFont( lf );
			MakeBoldFont( lf );
			fontOverride.CreateFontIndirect( & lf );
			font = fontOverride;
		}
		HFONT oldFont = pdc.SelectFont( font );

		DrawBackground( pdc.m_hDC, rcClient );

		if ( !IsWindowEnabled() ) {
			pdc.SetTextColor( ::GetSysColor(COLOR_GRAYTEXT) );
		} else if ( m_focused ) {
			pdc.SetTextColor( ::GetSysColor(COLOR_HIGHLIGHT) );
		}
		pdc.DrawText( m_textDrawMe, m_textDrawMe.GetLength(), &rcClient, DT_VCENTER | DT_CENTER | DT_SINGLELINE | DT_NOPREFIX );

		pdc.SelectFont( oldFont );
	}
	virtual void OnClicked() { 
		if ( ClickHandler ) {
			ClickHandler();
		} else {
			GetParent().PostMessage( WM_COMMAND, MAKEWPARAM( this->GetDlgCtrlID(), BN_CLICKED ), (LPARAM) m_hWnd );
		}
	}
	bool IsPressed() {return m_pressed; }
private:
	int OnCreate(LPCREATESTRUCT lpCreateStruct) {
		if ( lpCreateStruct->lpszName != nullptr ) this->m_textDrawMe = lpCreateStruct->lpszName;
		SetMsgHandled(FALSE); return 0;
	}
	void OnEnable(BOOL bEnable) {
		Invalidate(); SetMsgHandled(FALSE);
	}
	void OnLButtonDown(UINT nFlags, CPoint point) {
		ToggleCapture( true );
		TogglePressed( true );
	}
	void OnLButtonUp(UINT nFlags, CPoint point) {
		if ( m_pressed ) {
			OnClicked();
		}
		ToggleCapture( false );
		TogglePressed( false );
	}
	void OnMouseMove(UINT nFlags, CPoint point) {
		if ( m_captured ) {
			CRect rcClient;
			if (GetClientRect( rcClient )) {
				TogglePressed( !! rcClient.PtInRect(point) );
			}
		}
	}
	void TogglePressed( bool bPressed ) {
		if ( bPressed != m_pressed ) {
			m_pressed = bPressed; Invalidate();
		}
	}
	void ToggleCapture( bool bCapture ) {
		if ( bCapture != m_captured ) {
			if ( bCapture ) SetCapture();
			else ReleaseCapture();
			m_captured = bCapture;
		}
	}
	int OnSetText(LPCTSTR lpstrText) {
		m_textDrawMe = lpstrText;
		Invalidate(); SetMsgHandled(FALSE);
		return 0;
	}
	bool m_pressed, m_captured, m_focused;
	CString m_textDrawMe;
};
