/*
 * CTreeCtrl.h
 * -----------
 * Purpose: A class that extends MFC's CTreeCtrl to handle unicode strings in ANSI builds.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


OPENMPT_NAMESPACE_BEGIN


class CTreeCtrlW : public CTreeCtrl
{
public:

#ifndef UNICODE
	BOOL GetItem(TVITEMW *pItem) const
	{
		return (BOOL) ::SendMessage(m_hWnd, TVM_GETITEMW, 0, (LPARAM)pItem);
	}

	BOOL SetItem(TVITEMW *pItem)
	{
		return (BOOL) ::SendMessage(m_hWnd, TVM_SETITEMW, 0, (LPARAM)pItem);
	}
	BOOL SetItem(HTREEITEM hItem, UINT nMask, const WCHAR *lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam)
	{
		TVITEMW tvi;
		MemsetZero(tvi);
		tvi.hItem = hItem;
		tvi.mask = nMask;
		tvi.state = nState;
		tvi.stateMask = nStateMask;
		tvi.iImage = nImage;
		tvi.iSelectedImage = nSelectedImage;
		tvi.pszText = const_cast<WCHAR *>(lpszItem);
		tvi.lParam = lParam;
		return SetItem(&tvi);
	}

	HTREEITEM InsertItem(TVINSERTSTRUCTW &tvi)
	{
		return (HTREEITEM)::SendMessage(m_hWnd, TVM_INSERTITEMW, 0, (LPARAM)&tvi);
	}
	HTREEITEM InsertItem(UINT nMask, const WCHAR *lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam, HTREEITEM hParent, HTREEITEM hInsertAfter)
	{
		TVINSERTSTRUCTW tvi;
		MemsetZero(tvi);
		tvi.hInsertAfter = hInsertAfter;
		tvi.hParent = hParent;
		tvi.item.mask = nMask;
		tvi.item.state = nState;
		tvi.item.stateMask = nStateMask;
		tvi.item.iImage = nImage;
		tvi.item.iSelectedImage = nSelectedImage;
		tvi.item.pszText = const_cast<WCHAR *>(lpszItem);
		tvi.item.lParam = lParam;
		return InsertItem(tvi);
	}
	HTREEITEM InsertItem(const WCHAR *lpszItem, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST)
	{
		TVINSERTSTRUCTW tvi;
		MemsetZero(tvi);
		tvi.hInsertAfter = hInsertAfter;
		tvi.hParent = hParent;
		tvi.item.mask = TVIF_TEXT;
		tvi.item.pszText = const_cast<WCHAR *>(lpszItem);
		return InsertItem(tvi);
	}
	HTREEITEM InsertItem(const WCHAR *lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST)
	{
		TVINSERTSTRUCTW tvi;
		MemsetZero(tvi);
		tvi.hInsertAfter = hInsertAfter;
		tvi.hParent = hParent;
		tvi.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.item.iImage = nImage;
		tvi.item.iSelectedImage = nSelectedImage;
		tvi.item.pszText = const_cast<WCHAR *>(lpszItem);
		return InsertItem(tvi);
	}

	void SetItemText(HTREEITEM item, const WCHAR *text)
	{
		TVITEMW tvi;
		MemsetZero(tvi);
		tvi.hItem = item;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = const_cast<WCHAR *>(text);
		::SendMessage(m_hWnd, TVM_SETITEMW, 0, (LPARAM)&tvi);
	}

#endif // UNICODE

	CStringW GetItemTextW(HTREEITEM item) const
	{
#ifdef UNICODE
		return GetItemText(item);
#else
		TVITEMW tvi;
		tvi.hItem = item;
		tvi.mask = TVIF_TEXT;
		CStringW str;
		int nLen = 128;
		size_t nRes;
		do
		{
			nLen *= 2;
			tvi.pszText = str.GetBufferSetLength(nLen);
			tvi.cchTextMax = nLen;
			::SendMessage(m_hWnd, TVM_GETITEMW, 0, (LPARAM)&tvi);
			nRes = wcslen(tvi.pszText);
		} while (nRes >= size_t(nLen - 1));
		str.ReleaseBuffer();
		return str;
#endif // UNICODE
	}

	using CTreeCtrl::GetItem;
	using CTreeCtrl::SetItem;
	using CTreeCtrl::SetItemText;
	using CTreeCtrl::InsertItem;
};


OPENMPT_NAMESPACE_END
