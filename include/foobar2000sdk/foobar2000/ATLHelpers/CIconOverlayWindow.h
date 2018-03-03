#pragma once

#include "../ATLHelpers/misc.h"

typedef CWinTraits<WS_POPUP,WS_EX_LAYERED> __COverlayWindowTraits;

class CIconOverlayWindow : public CWindowImpl<CIconOverlayWindow,CWindow,__COverlayWindowTraits> {
public:
	DECLARE_WND_CLASS_EX(TEXT("{384298D0-4370-4f9b-9C36-49FC1A396DC7}"),0,(-1));

	void AttachIcon(HICON p_icon) {m_icon = p_icon;}
	bool HaveIcon() const {return m_icon != NULL;}

	enum {
		ColorKey = 0xc0ffee
	};

	BEGIN_MSG_MAP(CIconOverlayWindow)
		MESSAGE_HANDLER(WM_CREATE,OnCreate);
		MESSAGE_HANDLER(WM_PAINT,OnPaint);
		MESSAGE_HANDLER(WM_ERASEBKGND,OnEraseBkgnd);
	END_MSG_MAP()
private:
	LRESULT OnCreate(UINT,WPARAM,LPARAM,BOOL&) {
		::SetLayeredWindowAttributes(*this,ColorKey,0,LWA_COLORKEY);
		return 0;
	}
	LRESULT OnEraseBkgnd(UINT,WPARAM p_wp,LPARAM,BOOL& bHandled) {
		CRect rcClient;
		WIN32_OP_D( GetClientRect(rcClient) );
		CDCHandle((HDC)p_wp).FillSolidRect(rcClient,ColorKey);
		return 1;
	}
	LRESULT OnPaint(UINT,WPARAM,LPARAM,BOOL& bHandled) {
		if (m_icon != NULL) {
			CPaintDC dc(*this);
			CRect client;
			WIN32_OP_D( GetClientRect(&client) );
			dc.DrawIconEx(0,0,m_icon,client.right,client.bottom);
			//CDCHandle(ps.hdc).DrawIcon(0,0,m_icon);
		} else {
			bHandled = FALSE;
		}
		return 0;
	}
	CIcon m_icon;
};
