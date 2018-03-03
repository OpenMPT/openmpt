#pragma once

#include "misc.h"

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
		MSG_WM_ENABLE(OnEnable)
		CHAIN_MSG_MAP(__super)
	END_MSG_MAP()

	static const TCHAR * GetClassName() {
		return _T("foobar2000:separator");
	}
private:
	void OnEnable(BOOL bEnable) {
		Invalidate();
	}
	void OnPaint(CDCHandle dc) {
		PaintSeparatorControl(*this);
	}
};

