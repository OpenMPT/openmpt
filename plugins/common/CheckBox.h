/*
 * CheckBox.h
 * ----------
 * Purpose: Wrapper class for a Win32 check box.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "WindowBase.h"


//================================
class CheckBox : public WindowBase
//================================
{ 
public:
	// Create a new check box.
	void Create(HWND parent, const TCHAR *text, int x, int y, int width, int height)
	{
		// Remove old instance, if necessary
		Destroy();

		hwnd = CreateWindow(_T("BUTTON"),
			text,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX | BS_AUTOCHECKBOX,
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


	// Select an item.
	void SetState(bool state)
	{
		if(hwnd != nullptr)
		{
			SendMessage(hwnd,
				BM_SETCHECK,
				state ? BST_CHECKED : BST_UNCHECKED,
				0);
		}
	}


	// Get the currently selected item.
	bool GetState() const
	{
		if(hwnd != nullptr)
		{
			return SendMessage(hwnd,
				BM_GETCHECK,
				0,
				0) != BST_UNCHECKED;
		}
		return false;
	}

};
