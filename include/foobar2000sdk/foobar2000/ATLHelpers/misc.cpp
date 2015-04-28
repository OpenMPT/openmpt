#include "stdafx.h"

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
		WIN32_OP_D( dc.DrawText(buffer,txLen,rcText,DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER | DT_LEFT ) > 0);
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