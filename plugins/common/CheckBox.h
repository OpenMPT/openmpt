/*
 * CheckBox.h
 * ----------
 * Purpose: Wrapper class for a Win32 check box.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <tchar.h>


//============
class CheckBox
//============
{ 
protected:
	HWND check;

public:
	CheckBox()
	{
		check = nullptr;
	}


	~CheckBox()
	{
		Destroy();
	}


	// Create a new check box.
	void Create(HWND parent, const TCHAR *text, int x, int y, int width, int height)
	{
		// Remove old instance, if necessary
		Destroy();

		check = CreateWindow(_T("BUTTON"),
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

		SendMessage(check, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
	}


	// Destroy the combo box.
	void Destroy()
	{
		if(check != nullptr)
		{
			DestroyWindow(check);
		}
		check = nullptr;
	}


	// Get the HWND associated with this combo box.
	HWND GetHwnd()
	{
		return check;
	}


	// Select an item.
	void SetState(bool state)
	{
		if(check != nullptr)
		{
			SendMessage(check,
				BM_SETCHECK,
				state ? BST_CHECKED : BST_UNCHECKED,
				0);
		}
	}


	// Get the currently selected item.
	bool GetState() const
	{
		if(check != nullptr)
		{
			return SendMessage(check,
				BM_GETCHECK,
				0,
				0) != BST_UNCHECKED;
		}
		return false;
	}

};
