#pragma once

#include "CIconOverlayWindow.h"
#include <utility>

template<typename TBase>
class CMiddleDragImpl : public TBase {
private:
	typedef CMiddleDragImpl<TBase> TSelf;
	enum {
		KTimerID = 0x389675f8,
		KTimerPeriod = 15,
	};
public:	
	template<typename ... arg_t> CMiddleDragImpl( arg_t && ... arg ) : TBase(std::forward<arg_t>(arg) ... ) { m_active = false; m_accX = m_accY = 0; }

	BEGIN_MSG_MAP(TSelf)
		MESSAGE_HANDLER(WM_TIMER,OnTimer);
		MESSAGE_HANDLER(WM_CAPTURECHANGED,OnCaptureChanged);
		MESSAGE_HANDLER(WM_MBUTTONDOWN,OnMButtonDown);
		MESSAGE_HANDLER(WM_MBUTTONDBLCLK,OnMButtonDown);
		MESSAGE_HANDLER(WM_MBUTTONUP,OnMButtonUp);
		MESSAGE_HANDLER(WM_LBUTTONDOWN,OnMouseAction);
		MESSAGE_HANDLER(WM_RBUTTONDOWN,OnMouseAction);
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK,OnMouseAction);
		MESSAGE_HANDLER(WM_RBUTTONDBLCLK,OnMouseAction);
		MESSAGE_HANDLER(WM_LBUTTONUP,OnMouseAction);
		MESSAGE_HANDLER(WM_RBUTTONUP,OnMouseAction);
		MESSAGE_HANDLER(WM_XBUTTONDOWN,OnMouseAction);
		MESSAGE_HANDLER(WM_XBUTTONDBLCLK,OnMouseAction);
		MESSAGE_HANDLER(WM_XBUTTONUP,OnMouseAction);
		MESSAGE_HANDLER(WM_MOUSEMOVE,OnMouseMove);
		MESSAGE_HANDLER(WM_DESTROY,OnDestroyPassThru);
		CHAIN_MSG_MAP(TBase);
	END_MSG_MAP()
protected:
	bool CanSmoothScroll() const { 
		return !m_active;
	}
private:
	bool m_active, m_dragged;
	CPoint m_base;
	CIconOverlayWindow m_overlay;
	double m_accX, m_accY;

	LRESULT OnDestroyPassThru(UINT,WPARAM,LPARAM,BOOL& bHandled) {
		if (m_overlay != NULL) m_overlay.DestroyWindow();
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnMButtonUp(UINT,WPARAM,LPARAM p_lp,BOOL& bHandled) {
		if (m_active /*&& m_dragged*/) {
			EndDrag();
		}
		return 0;
	}

	LRESULT OnMButtonDown(UINT,WPARAM,LPARAM p_lp,BOOL& bHandled) {
		if (m_active) {
			EndDrag();
			return 0;
		}
		EndDrag();
		SetFocus();
		CPoint pt(p_lp);
		WIN32_OP_D( ClientToScreen(&pt) );
		StartDrag(pt);
		return 0;
	}

	LRESULT OnMouseMove(UINT,WPARAM,LPARAM p_lp,BOOL& bHandled) {
		if (m_active) {
			if (!m_dragged) {
				CPoint pt(p_lp);
				WIN32_OP_D( ClientToScreen(&pt) );
				if (pt != m_base) {
					m_dragged = true;
				}
			}
			bHandled = TRUE;
		} else {
			bHandled = FALSE;
		}
		return 0;
	}

	LRESULT OnMouseAction(UINT,WPARAM,LPARAM,BOOL& bHandled) {
		EndDrag();
		bHandled = FALSE;
		return 0;
	}

	void StartDrag(CPoint const & p_point) {
		::SetCapture(NULL);

		if (m_overlay == NULL) {
			PFC_ASSERT( m_hWnd != NULL );
			if (m_overlay.Create(*this) == NULL) {PFC_ASSERT(!"Should not get here!"); return;}
			HANDLE temp;
			WIN32_OP_D( (temp = LoadImage(core_api::get_my_instance(),MAKEINTRESOURCE(IDI_SCROLL),IMAGE_ICON,32,32,LR_DEFAULTCOLOR)) != NULL );
			m_overlay.AttachIcon((HICON) temp);
		}

		//todo sanity checks - don't drag when the entire content is visible, perhaps use a different icon when only vertical or horizontal drag is possible
		SetCursor(::LoadCursor(NULL,IDC_SIZEALL));
		m_active = true;
		m_dragged = false;
		m_base = p_point;
		m_accX = m_accY = 0;
		SetCapture();
		SetTimer(KTimerID,KTimerPeriod);
		
		{
			CSize radius(16,16);
			CPoint center (p_point);
			CRect rect(center - radius, center + radius);
			m_overlay.SetWindowPos(HWND_TOPMOST,rect,SWP_SHOWWINDOW | SWP_NOACTIVATE);
		}
	}

	void EndDrag() {
		if (m_active) ::SetCapture(NULL);
	}

	void HandleEndDrag() {
		if (m_active) {
			m_active = false;
			KillTimer(KTimerID);
			if (m_overlay != NULL) m_overlay.ShowWindow(SW_HIDE);
		}
	}

	LRESULT OnCaptureChanged(UINT,WPARAM,LPARAM,BOOL& bHandled) {
		HandleEndDrag();
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnTimer(UINT,WPARAM p_wp,LPARAM,BOOL& bHandled) {
		switch(p_wp) {
		case KTimerID:
			this->MoveViewOriginDelta(ProcessMiddleDragDelta(CPoint((LPARAM)GetMessagePos()) - m_base));
			return 0;
		default:
			bHandled = FALSE;
			return 0;
		}
	}

	static double myPow(double p_val,double p_exp) {
		double ex = pow(pfc::abs_t(p_val),p_exp);
		return p_val < 0 ? -ex : ex;
	}

	static double ProcessMiddleDragDeltaInternal(double p_delta) {
		p_delta *= (double) KTimerPeriod / 25; /*originally calculated for 25ms timer interval*/
		return myPow(p_delta * 0.05,2.0);
	}

	static double radiusHelper(double p_x,double p_y) {
		return sqrt(p_x * p_x + p_y * p_y);
	}

	CPoint ProcessMiddleDragDelta(CPoint p_delta) {
		const CSize dpi = QueryScreenDPIEx();
		if (dpi.cx <= 0 || dpi.cy <= 0 || p_delta == CPoint(0,0)) return CPoint(0,0);
		const double dpiMulX = 96.0 / (double) dpi.cx;
		const double dpiMulY = 96.0 / (double) dpi.cy;

		const double deltaTotal = ProcessMiddleDragDeltaInternal( radiusHelper( (double) p_delta.x * dpiMulX, (double) p_delta.y * dpiMulY ) );

		double xVal = 0, yVal = 0;

		if (p_delta.x == 0) {
			yVal = deltaTotal;
		} else if (p_delta.y == 0) {
			xVal = deltaTotal;
		} else {
			const double ratio = (double) p_delta.x / (double) p_delta.y;
			yVal = sqrt(pfc::sqr_t(deltaTotal) / (1.0 + pfc::sqr_t(ratio)));
			xVal = yVal * ratio;
		}

		xVal = pfc::sgn_t( p_delta.x ) * pfc::abs_t(xVal);
		yVal = pfc::sgn_t( p_delta.y ) * pfc::abs_t(yVal);

		return CPoint(Round(xVal / dpiMulX, m_accX), Round(yVal / dpiMulY, m_accY));
	}
	static t_int32 Round(double val, double & acc) {
		val += acc;
		t_int32 ret = pfc::rint32(val);
		acc = val - ret;
		return ret;
	}
};

template<typename TBase>
class CMiddleDragWrapper : public TBase {
private:
	typedef CMiddleDragWrapper<TBase> TSelf;
public:
	template<typename ... arg_t> CMiddleDragWrapper(arg_t && ... arg ) : TBase(std::forward<arg_t>(arg) ... ) { m_overflow = CPoint(0, 0); m_lineWidth = CSize(4, 16); }

	BEGIN_MSG_MAP(TSelf)
		MESSAGE_HANDLER(WM_CAPTURECHANGED,OnCaptureChanged);
		CHAIN_MSG_MAP(TBase)
	END_MSG_MAP()

	void MoveViewOriginDelta(CPoint p_delta) {
		MoveViewOriginDeltaLines( MiddleDrag_PixelsToLines( m_overflow, p_delta ) );
	}
	void MoveViewOriginDeltaLines(CPoint p_delta) {
		FireScrollMessage(WM_HSCROLL,p_delta.x);
		FireScrollMessage(WM_VSCROLL,p_delta.y);
	}
	void SetLineWidth(CSize p_width) {m_lineWidth = p_width;}
	
private:
	void FireScrollMessage(UINT p_msg,int p_delta) {
		UINT count = (UINT)pfc::abs_t(p_delta);
		const UINT which = (p_msg == WM_HSCROLL ? SB_HORZ : SB_VERT);
		SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_PAGE | SIF_RANGE;
		if (!this->GetScrollInfo(which, &si) || si.nPage <= 0) return;
		while(count >= si.nPage) {
			const WPARAM code = p_delta < 0 ? SB_PAGEUP : SB_PAGEDOWN;
			this->SendMessage(p_msg, code, 0);
			count -= si.nPage;
		}
		const WPARAM code = p_delta < 0 ? SB_LINEUP : SB_LINEDOWN;
		for(UINT walk = 0; walk < count; ++walk) this->SendMessage(p_msg, code, 0);
	}
	LRESULT OnCaptureChanged(UINT,WPARAM,LPARAM,BOOL& bHandled) {
		m_overflow = CPoint(0,0);
		bHandled = FALSE;
		return 0;
	}

	static LONG LineToPixelsHelper(LONG & p_overflow,LONG p_pixels,LONG p_dpi,LONG p_lineWidth) {
		const int lineWidth = MulDiv(p_lineWidth,p_dpi,96);
		if (lineWidth == 0) return 0;
		p_overflow += p_pixels;
		LONG ret = p_overflow / lineWidth;
		p_overflow -= ret * lineWidth;
		return ret;
	}

	CPoint MiddleDrag_PixelsToLines(CPoint & p_overflow,CPoint p_pixels) {
		const CSize dpi = QueryScreenDPIEx();
		if (dpi.cx <= 0 || dpi.cy <= 0) return CPoint(0,0);
		CPoint pt;
		pt.x = LineToPixelsHelper(p_overflow.x,p_pixels.x,dpi.cx,m_lineWidth.cx);
		pt.y = LineToPixelsHelper(p_overflow.y,p_pixels.y,dpi.cy,m_lineWidth.cy);
		return pt;
	}

	CSize m_lineWidth;

	CPoint m_overflow;
};

template<typename TBase = CWindow>
class CWindow_DummyMsgMap : public TBase , public CMessageMap {
public:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		LRESULT& lResult, DWORD dwMsgMapID = 0) {return FALSE;}
};

typedef CMiddleDragImpl<CMiddleDragWrapper<CWindow_DummyMsgMap<> > > CMiddleDragImplSimple;

template<typename TBase = CWindow>
class CContainedWindow_MsgMap : public CContainedWindowT<CWindow_DummyMsgMap<TBase> > {
protected:
	CContainedWindow_MsgMap() : CContainedWindowT<CWindow_DummyMsgMap<TBase> >(pfc::implicit_cast<CMessageMap*>(this)) {}
};

template<typename TBase>
class CMiddleDragImplCtrlHook : public CMiddleDragImpl<CMiddleDragWrapper<CContainedWindow_MsgMap<TBase> > > {
};

template<typename TBase> class CMiddleDragImplCtrlHookEx : public CMiddleDragImplCtrlHook<TBase> {
public:
	CMiddleDragImplCtrlHookEx(CMessageMap * hook, DWORD hookMapID = 0) : m_hook(*hook), m_hookMapID(hookMapID) {}
	
	BEGIN_MSG_MAP(CMiddleDragImplCtrlHookEx)
		CHAIN_MSG_MAP(CMiddleDragImplCtrlHook<TBase>)
		CHAIN_MSG_MAP_ALT_MEMBER(m_hook,m_hookMapID);
	END_MSG_MAP()
private:
	DWORD const m_hookMapID;
	CMessageMap & m_hook;
};
