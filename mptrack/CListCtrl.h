/*
 * CListCtrl.h
 * -----------
 * Purpose: A class that extends MFC's CListCtrl to handle unicode strings in ANSI builds.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


OPENMPT_NAMESPACE_BEGIN


class CListCtrlW : public CListCtrl
{
public:

#ifndef UNICODE
	BOOL SetItemText(int nItem, int nSubItem, const WCHAR *lpszText)
	{
		ASSERT(::IsWindow(m_hWnd));
		ASSERT((GetStyle() & LVS_OWNERDATA)==0);
		LVITEMW lvi;
		lvi.iSubItem = nSubItem;
		lvi.pszText = (LPWSTR) lpszText;
		return (BOOL) ::SendMessage(m_hWnd, LVM_SETITEMTEXTW, nItem, (LPARAM)&lvi);
	}

	using CListCtrl::SetItemText;
#endif
};


OPENMPT_NAMESPACE_END
