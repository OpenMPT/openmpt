/*
 * ComboBox.h
 * ----------
 * Purpose: Wrapper class for a Win32 combo box.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "WindowBase.h"


//================================
class ComboBox : public WindowBase
//================================
{ 
public:
	// Create a new combo box.
	void Create(HWND parent, int x, int y, int width, int height)
	{
		// Remove old instance, if necessary
		Destroy();

		hwnd = CreateWindow(_T("COMBOBOX"),
			nullptr,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
			x,
			y,
			width,
			height,
			parent,
			NULL,
			NULL,
			0);

		SendMessage(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
	}


	// Destroy the combo box.
	void Destroy()
	{
		if(hwnd != nullptr)
		{
			ResetContent();
			DestroyWindow(hwnd);
		}
		hwnd = nullptr;
	}


	// Remove all items in the combo box.
	void ResetContent()
	{
		if(hwnd != nullptr)
		{
			ComboBox_ResetContent(hwnd);
		}
	}


	// Add a string to the combo box.
	int AddString(const TCHAR *text, const void *data)
	{
		if(hwnd != nullptr)
		{
			int result = ComboBox_AddString(hwnd, text);
			if(result != CB_ERR)
			{
				ComboBox_SetItemData(hwnd, result, data);
				return result;
			}
		}
		return -1;
	}


	// Return number of items in combo box.
	int GetCount() const
	{
		if(hwnd != nullptr)
		{
			return ComboBox_GetCount(hwnd);
		}
		return 0;
	}


	// Select an item.
	void SetSelection(int index)
	{
		if(hwnd != nullptr)
		{
			ComboBox_SetCurSel(hwnd, index);
		}
	}


	// Get the currently selected item.
	LRESULT GetSelection() const
	{
		if(hwnd != nullptr)
		{
			return ComboBox_GetCurSel(hwnd);
		}
		return -1;
	}


	// Get the data associated with the selected item.
	void *GetSelectionData() const
	{
		return GetItemData(static_cast<int>(GetSelection()));
	}


	// Get item data associated with a list item.
	void *GetItemData(int index) const
	{
		if(hwnd != nullptr)
		{
			return reinterpret_cast<void *>(ComboBox_GetItemData(hwnd, index));
		}
		return 0;
	}

	// Dynamically resize the dropdown list to cover as many items as possible.
	void SizeDropdownList()
	{
		int itemHeight = ComboBox_GetItemHeight(hwnd);
		int numItems = GetCount();
		if(numItems < 2) numItems = 2;

		RECT rect;
		GetWindowRect(hwnd, &rect);

		SIZE sz;
		sz.cx = rect.right - rect.left;
		sz.cy = itemHeight * (numItems + 2);

		if(rect.top - sz.cy < 0 || rect.bottom + sz.cy > GetSystemMetrics(SM_CYSCREEN))
		{
			// Dropdown exceeds screen height - clamp it.
			int k = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / itemHeight;
			if(k < rect.top / itemHeight) k = rect.top / itemHeight;
			if(itemHeight * k < sz.cy) sz.cy = itemHeight * k;
		}

		SetWindowPos(hwnd, NULL, 0, 0, sz.cx, sz.cy, SWP_NOMOVE | SWP_NOZORDER);
	}

}; 
