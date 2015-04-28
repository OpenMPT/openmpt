#include "stdafx.h"

bool CDialogResizeHelper::EvalRect(UINT id, CRect & out) const {
	for(t_size walk = 0; walk < m_table.get_size(); ++walk) {
		if (m_table[walk].id == id) {
			CRect client; WIN32_OP_D( m_thisWnd.GetClientRect(client) );
			return _EvalRect(walk, client.Size(), out);
		}
	}
	return false;
}

bool CDialogResizeHelper::_EvalRect(t_size index, CSize wndSize, CRect & out) const {
	CRect rc;
	const Param & e = m_table[index];
	if (m_origRects.query(e.id, rc)) {
		int delta_x = wndSize.cx - m_rcOrigClient.right,
			delta_y = wndSize.cy - m_rcOrigClient.bottom;

		rc.left += pfc::rint32( e.snapLeft * delta_x );
		rc.right += pfc::rint32( e.snapRight * delta_x );
		rc.top += pfc::rint32(e.snapTop * delta_y );
		rc.bottom += pfc::rint32(e.snapBottom * delta_y );

		out = rc;
		return true;
	}
	return false;
}

void CDialogResizeHelper::OnSize(UINT, CSize newSize)
{
	if (m_thisWnd != NULL) {
		HDWP hWinPosInfo = BeginDeferWindowPos( m_table.get_size() + (m_sizeGrip != NULL ? 1 : 0) );
		for(t_size n = 0; n < m_table.get_size(); ++n) {
			CRect rc;
			if (_EvalRect(n, newSize, rc)) {
				hWinPosInfo = DeferWindowPos(hWinPosInfo, m_thisWnd.GetDlgItem(m_table[n].id), 0, rc.left,rc.top,rc.Width(),rc.Height(),SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
			}
		}
		if (m_sizeGrip != NULL)
		{
			RECT rc, rc_grip;
			if (m_thisWnd.GetClientRect(&rc) && m_sizeGrip.GetWindowRect(&rc_grip)) {
				DWORD flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOCOPYBITS;
				if (IsZoomed(m_thisWnd)) flags |= SWP_HIDEWINDOW;
				else flags |= SWP_SHOWWINDOW;
				hWinPosInfo = DeferWindowPos(hWinPosInfo, m_sizeGrip, NULL, rc.right - (rc_grip.right - rc_grip.left), rc.bottom - (rc_grip.bottom - rc_grip.top), 0, 0, flags );
			}
		}
		EndDeferWindowPos(hWinPosInfo);
		//RedrawWindow(m_thisWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
	SetMsgHandled(FALSE);
}

void CDialogResizeHelper::OnGetMinMaxInfo(LPMINMAXINFO info) const {
	CRect r;
	const DWORD dwStyle = m_thisWnd.GetWindowLong(GWL_STYLE);
	const DWORD dwExStyle = m_thisWnd.GetWindowLong(GWL_EXSTYLE);
	if (max_x && max_y)
	{
		r.left = 0; r.right = max_x;
		r.top = 0; r.bottom = max_y;
		MapDialogRect(m_thisWnd,&r);
		AdjustWindowRectEx(&r, dwStyle, FALSE, dwExStyle);
		info->ptMaxTrackSize.x = r.right - r.left;
		info->ptMaxTrackSize.y = r.bottom - r.top;
	}
	if (min_x && min_y)
	{
		r.left = 0; r.right = min_x;
		r.top = 0; r.bottom = min_y;
		MapDialogRect(m_thisWnd,&r);
		AdjustWindowRectEx(&r, dwStyle, FALSE, dwExStyle);
		info->ptMinTrackSize.x = r.right - r.left;
		info->ptMinTrackSize.y = r.bottom - r.top;
	}
}

void CDialogResizeHelper::OnInitDialog(CWindow thisWnd) {
	m_origRects.remove_all();
	m_thisWnd = thisWnd;
	m_thisWnd.GetClientRect(&m_rcOrigClient);
	for(t_size n = 0; n < m_table.get_size(); n++) {
		CRect rc;
		const UINT id = m_table[n].id;
		if (GetChildWindowRect(m_thisWnd,id,&rc)) {
			m_origRects.set(id, rc);
		}
	}
	AddSizeGrip();
	SetMsgHandled(FALSE);
}
void CDialogResizeHelper::OnDestroy() {
	m_sizeGrip = NULL; m_thisWnd = NULL;
	SetMsgHandled(FALSE);
}

void CDialogResizeHelper::AddSizeGrip()
{
	if (m_thisWnd != NULL && m_sizeGrip == NULL)
	{
		if (m_thisWnd.GetWindowLong(GWL_STYLE) & WS_POPUP) {
			m_sizeGrip = CreateWindowEx(0, WC_SCROLLBAR, _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
				0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
				m_thisWnd, (HMENU)0, NULL, NULL);
			if (m_sizeGrip != NULL)
			{
				RECT rc, rc_grip;
				if (m_thisWnd.GetClientRect(&rc) && m_sizeGrip.GetWindowRect(&rc_grip)) {
					m_sizeGrip.SetWindowPos(NULL, rc.right - (rc_grip.right - rc_grip.left), rc.bottom - (rc_grip.bottom - rc_grip.top), 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				}
			}
		}
	}
}


void CDialogResizeHelper::InitTable(const Param* table, t_size tableSize) {
	m_table.set_data_fromptr(table, tableSize);
}
void CDialogResizeHelper::InitTable(const ParamOld * table, t_size tableSize) {
	m_table.set_size(tableSize);
	for(t_size walk = 0; walk < tableSize; ++walk) {
		const ParamOld in = table[walk];
		Param entry = {};
		entry.id = table[walk].id;
		if (in.flags & dialog_resize_helper::X_MOVE) entry.snapLeft = entry.snapRight = 1;
		else if (in.flags & dialog_resize_helper::X_SIZE) entry.snapRight = 1;
		if (in.flags & dialog_resize_helper::Y_MOVE) entry.snapTop = entry.snapBottom = 1;
		else if (in.flags & dialog_resize_helper::Y_SIZE) entry.snapBottom = 1;
		m_table[walk] = entry;
	}
}
void CDialogResizeHelper::InitMinMax(const CRect & range) {
	min_x = range.left; min_y = range.top; max_x = range.right; max_y = range.bottom;
}
