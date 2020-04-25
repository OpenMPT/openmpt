/*
 * CListCtrl.h
 * -----------
 * Purpose: A class that extends MFC's CListCtrl with some more functionality and to handle unicode strings in ANSI builds.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "MPTrackUtil.h"

OPENMPT_NAMESPACE_BEGIN

class CListCtrlEx : public CListCtrl
{
public:
	struct Header
	{
		const TCHAR *text = nullptr;
		int width = 0;
		UINT mask = 0;
	};
	void SetHeaders(const mpt::span<const Header> &header)
	{
		for(int i = 0; i < static_cast<int>(header.size()); i++)
		{
			int width = header[i].width;
			InsertColumn(i, header[i].text, header[i].mask, width >= 0 ? Util::ScalePixels(width, m_hWnd) : 16);
			if(width < 0)
				SetColumnWidth(i, width);
		}
	}

	void SetItemDataPtr(int item, void *value)
	{
		SetItemData(item, reinterpret_cast<DWORD_PTR>(value));
	}

	void *GetItemDataPtr(int item)
	{
		return reinterpret_cast<void *>(GetItemData(item));
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


#ifdef MPT_MFC_FULL

class CMFCListCtrlEx : public CMFCListCtrl
{
public:
	struct Header
	{
		const TCHAR *text = nullptr;
		int width = 0;
		UINT mask = 0;
	};
	void SetHeaders(const mpt::span<const Header> &header)
	{
		for(int i = 0; i < static_cast<int>(header.size()); i++)
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

#endif // MPT_MFC_FULL


OPENMPT_NAMESPACE_END
