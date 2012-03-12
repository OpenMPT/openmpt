/*
 * ComboBox.h
 * ----------
 * Purpose: Wrapper class for a Win32 combo box.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include "windows.h"


//============
class ComboBox
//============
{ 
protected:
	HWND combo;

public:
	ComboBox()
	{
		combo = nullptr;
	}


	~ComboBox()
	{
		Destroy();
	}


	// Create a new combo box.
	void Create(HWND parent, int x, int y, int width, int height)
	{
		// Remove old instance, if necessary
		Destroy();

		combo = CreateWindow("COMBOBOX",
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

		SendMessage(combo, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
	}


	// Destroy the combo box.
	void Destroy()
	{
		if(combo != nullptr)
		{
			ResetContent();
			DestroyWindow(combo);
		}
		combo = nullptr;
	}


	// Remove all items in the combo box.
	void ResetContent()
	{
		if(combo != nullptr)
		{
			SendMessage(combo,
				CB_RESETCONTENT,
				0,
				0);
		}
	}


	// Get the HWND associated with this combo box.
	HWND GetHwnd()
	{
		return combo;
	}


	// Add a string to the combo box.
	int AddString(const char *text, const void *data)
	{
		if(combo != nullptr)
		{
			LRESULT result = SendMessage(combo,
				CB_ADDSTRING,
				0,
				reinterpret_cast<LPARAM>(text));

			if(result != CB_ERR)
			{
				SendMessage(combo,
					CB_SETITEMDATA,
					result,
					reinterpret_cast<LPARAM>(data)
					);
				return result;
			}
		}
		return -1;
	}


	// Return number of items in combo box.
	int GetCount() const
	{
		if(combo != nullptr)
		{
			return SendMessage(combo,
				CB_GETCOUNT,
				0,
				0);
		}
		return 0;
	}


	// Select an item.
	void SetSelection(int index)
	{
		if(combo != nullptr)
		{
			SendMessage(combo,
				CB_SETCURSEL,
				index,
				0);
		}
	}


	// Get the currently selected item.
	LRESULT GetSelection() const
	{
		if(combo != nullptr)
		{
			return SendMessage(combo,
				CB_GETCURSEL,
				0,
				0);
		}
		return -1;
	}


	// Get the data associated with the selected item.
	void *GetSelectionData() const
	{
		return GetItemData(GetSelection());
	}


	// Get item data associated with a list item.
	void *GetItemData(int index) const
	{
		if(combo != nullptr)
		{
			return reinterpret_cast<void *>(SendMessage(combo,
				CB_GETITEMDATA,
				index,
				0));
		}
		return 0;
	}

}; 
