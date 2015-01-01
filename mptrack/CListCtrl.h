/*
 * CListCtrl.h
 * -----------
 * Purpose: A class that extends MFC's CListCtrl with some more functionality and to handle unicode strings in ANSI builds.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "MPTrackUtil.h"

OPENMPT_NAMESPACE_BEGIN

class CListCtrlEx : public CListCtrl
{
public:
	struct Header
	{
		const TCHAR *text;
		int width;
		UINT mask;
	};
	template<size_t numItems>
	void SetHeaders(const Header (&header)[numItems])
	{
		for(int i = 0; i < numItems; i++)
		{
			InsertColumn(i, header[i].text, header[i].mask, Util::ScalePixels(header[i].width, m_hWnd));
		}
	}

	// Unicode strings in ANSI builds
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
