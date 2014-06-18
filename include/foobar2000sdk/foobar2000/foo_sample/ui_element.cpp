#include "stdafx.h"


class CMyElemWindow : public ui_element_instance, public CWindowImpl<CMyElemWindow> {
public:
	// ATL window class declaration. Replace class name with your own when reusing code.
	DECLARE_WND_CLASS_EX(TEXT("{DC2917D5-1288-4434-A28C-F16CFCE13C4B}"),CS_VREDRAW | CS_HREDRAW,(-1));

	void initialize_window(HWND parent) {WIN32_OP(Create(parent,0,0,0,WS_EX_STATICEDGE) != NULL);}

	BEGIN_MSG_MAP(ui_element_dummy)
		MESSAGE_HANDLER(WM_LBUTTONDOWN,OnLButtonDown);
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_PAINT(OnPaint)
	END_MSG_MAP()

	CMyElemWindow(ui_element_config::ptr,ui_element_instance_callback_ptr p_callback);
	HWND get_wnd() {return *this;}
	void set_configuration(ui_element_config::ptr config) {m_config = config;}
	ui_element_config::ptr get_configuration() {return m_config;}
	static GUID g_get_guid() {
		// This is our GUID. Substitute with your own when reusing code.
		static const GUID guid_myelem = { 0xb46dc166, 0x88f3, 0x4b45, { 0x9f, 0x77, 0xab, 0x33, 0xf4, 0xc3, 0xf2, 0xe4 } };
		return guid_myelem;
	}
	static GUID g_get_subclass() {return ui_element_subclass_utility;}
	static void g_get_name(pfc::string_base & out) {out = "Sample UI Element";}
	static ui_element_config::ptr g_get_default_configuration() {return ui_element_config::g_create_empty(g_get_guid());}
	static const char * g_get_description() {return "This is a sample UI Element.";}
	
	void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);
private:
	LRESULT OnLButtonDown(UINT,WPARAM,LPARAM,BOOL&) {m_callback->request_replace(this);return 0;}
	void OnPaint(CDCHandle);
	BOOL OnEraseBkgnd(CDCHandle);
	ui_element_config::ptr m_config;
protected:
	// this must be declared as protected for ui_element_impl_withpopup<> to work.
	const ui_element_instance_callback_ptr m_callback;
};
void CMyElemWindow::notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size) {
	if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed) {
		// we use global colors and fonts - trigger a repaint whenever these change.
		Invalidate();
	}
}
CMyElemWindow::CMyElemWindow(ui_element_config::ptr config,ui_element_instance_callback_ptr p_callback) : m_callback(p_callback), m_config(config) {
}

BOOL CMyElemWindow::OnEraseBkgnd(CDCHandle dc) {
	CRect rc; WIN32_OP_D( GetClientRect(&rc) );
	CBrush brush;
	WIN32_OP_D( brush.CreateSolidBrush( m_callback->query_std_color(ui_color_background) ) != NULL );
	WIN32_OP_D( dc.FillRect(&rc, brush) );
	return TRUE;
}
void CMyElemWindow::OnPaint(CDCHandle) {
	CPaintDC dc(*this);
	dc.SetTextColor( m_callback->query_std_color(ui_color_text) );
	dc.SetBkMode(TRANSPARENT);
	SelectObjectScope fontScope(dc, (HGDIOBJ) m_callback->query_font_ex(ui_font_default) );
	const UINT format = DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE;
	CRect rc; 
	WIN32_OP_D( GetClientRect(&rc) );
	WIN32_OP_D( dc.DrawText(_T("This is a sample element."), -1, &rc, format) > 0 );
}

// ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands. Use ui_element_impl instead if you don't want that.
class ui_element_myimpl : public ui_element_impl_withpopup<CMyElemWindow> {};

static service_factory_single_t<ui_element_myimpl> g_ui_element_myimpl_factory;
