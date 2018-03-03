#include "stdafx.h"
#include "Controls.h"
#include "../helpers/win32_misc.h"

void PaintSeparatorControl(CWindow wnd) {
	CPaintDC dc(wnd);
	TCHAR buffer[512] = {};
	wnd.GetWindowText(buffer, _countof(buffer));
	const int txLen = pfc::strlen_max_t(buffer, _countof(buffer));
	CRect contentRect;
	WIN32_OP_D( wnd.GetClientRect(contentRect) );

	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	dc.SetBkMode(TRANSPARENT);

	{
		CBrushHandle brush = (HBRUSH) wnd.GetParent().SendMessage(WM_CTLCOLORSTATIC, (WPARAM) (HDC) dc, (LPARAM) wnd.m_hWnd);
		if (brush != NULL) dc.FillRect(contentRect, brush);
	}
	SelectObjectScope scopeFont(dc, wnd.GetFont());
		
	if (txLen > 0) {
		CRect rcText(contentRect);
		if ( !wnd.IsWindowEnabled() ) {
			dc.SetTextColor( GetSysColor(COLOR_GRAYTEXT) );
		}
		WIN32_OP_D( dc.DrawText(buffer,txLen,rcText,DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER | DT_LEFT ) > 0);
		//			WIN32_OP_D( dc.GrayString(NULL, NULL, (LPARAM) buffer, txLen, rcText.left, rcText.top, rcText.Width(), rcText.Height() ) );		
	}

	SIZE txSize, probeSize;
	const TCHAR probe[] = _T("#");
	if (dc.GetTextExtent(buffer,txLen,&txSize) && dc.GetTextExtent(probe, _countof(probe), &probeSize)) {
		int spacing = txSize.cx > 0 ? (probeSize.cx / 4) : 0;
		if (txSize.cx + spacing < contentRect.Width()) {
			const CPoint center = contentRect.CenterPoint();
			CRect rcEdge(contentRect.left + txSize.cx + spacing, center.y, contentRect.right, contentRect.bottom);
			WIN32_OP_D( dc.DrawEdge(rcEdge, EDGE_ETCHED, BF_TOP) );
		}
	}
}



void ui_element_instance_standard_context_menu(service_ptr_t<ui_element_instance> p_elem, LPARAM p_pt) {
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
void ui_element_instance_standard_context_menu_eh(service_ptr_t<ui_element_instance> p_elem, LPARAM p_pt) {
	try {
		ui_element_instance_standard_context_menu(p_elem, p_pt);
	} catch(std::exception const & e) {
		console::complain("Context menu failure", e);
	}
}


#if _WIN32_WINNT >= 0x501
void HeaderControl_SetSortIndicator(CHeaderCtrl header, int column, bool isUp) {
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
