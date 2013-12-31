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
			SendMessage(hwnd,
				CB_RESETCONTENT,
				0,
				0);
		}
	}


	// Add a string to the combo box.
	int AddString(const TCHAR *text, const void *data)
	{
		if(hwnd != nullptr)
		{
			LRESULT result = SendMessage(hwnd,
				CB_ADDSTRING,
				0,
				reinterpret_cast<LPARAM>(text));

			if(result != CB_ERR)
			{
				SendMessage(hwnd,
					CB_SETITEMDATA,
					result,
					reinterpret_cast<LPARAM>(data)
					);
				return static_cast<int>(result);
			}
		}
		return -1;
	}


	// Return number of items in combo box.
	int GetCount() const
	{
		if(hwnd != nullptr)
		{
			return static_cast<int>(SendMessage(hwnd,
				CB_GETCOUNT,
				0,
				0));
		}
		return 0;
	}


	// Select an item.
	void SetSelection(int index)
	{
		if(hwnd != nullptr)
		{
			SendMessage(hwnd,
				CB_SETCURSEL,
				index,
				0);
		}
	}


	// Get the currently selected item.
	LRESULT GetSelection() const
	{
		if(hwnd != nullptr)
		{
			return SendMessage(hwnd,
				CB_GETCURSEL,
				0,
				0);
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
			return reinterpret_cast<void *>(SendMessage(hwnd,
				CB_GETITEMDATA,
				index,
				0));
		}
		return 0;
	}

}; 
