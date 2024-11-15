/*
 * CListCtrl.h
 * -----------
 * Purpose: A class that extends MFC's CListCtrl with some more functionality and to handle unicode strings in ANSI builds.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "HighDPISupport.h"

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
			InsertColumn(i, header[i].text, header[i].mask, width >= 0 ? HighDPISupport::ScalePixels(width, m_hWnd) : 16);
			if(width < 0)
				SetColumnWidth(i, width);
		}
	}

	void SetColumnWidths(const mpt::span<const Header> &header)
	{
		for(int i = 0; i < static_cast<int>(header.size()); i++)
		{
			if(int width = header[i].width; width > 0)
				SetColumnWidth(i, HighDPISupport::ScalePixels(width, m_hWnd));
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

	static bool EnableGroupView(CListCtrl &listCtrl)
	{
		ListView_EnableGroupView(listCtrl.m_hWnd, FALSE);  // try to set known state
		int enableGroupsResult1 = static_cast<int>(ListView_EnableGroupView(listCtrl.m_hWnd, TRUE));
		int enableGroupsResult2 = static_cast<int>(ListView_EnableGroupView(listCtrl.m_hWnd, TRUE));
		// Looks like we have to check enabling and check that a second enabling does
		// not change anything.
		// Just checking if enabling fails with -1 does not work for older control
		// versions because they just do not know the window message at all and return
		// 0, always. At least Wine does behave this way.
		if(enableGroupsResult1 == 1 && enableGroupsResult2 == 0)
		{
			return true;
		} else
		{
			// Did not behave as documented or expected, the actual state of the
			// control is unknown by now.
			// Play safe and set and assume the traditional ungrouped mode again.
			ListView_EnableGroupView(listCtrl.m_hWnd, FALSE);
			return false;
		}
	}

	bool EnableGroupView() { return EnableGroupView(*this); }

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


#ifndef _AFX_NO_MFC_CONTROLS_IN_DIALOGS

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
			InsertColumn(i, header[i].text, header[i].mask, HighDPISupport::ScalePixels(header[i].width, m_hWnd));
		}
	}

	bool EnableGroupView() { return CListCtrlEx::EnableGroupView(*this); }

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

#endif // !_AFX_NO_MFC_CONTROLS_IN_DIALOGS


OPENMPT_NAMESPACE_END
