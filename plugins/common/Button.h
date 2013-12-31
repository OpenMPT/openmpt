/*
 * Button.h
 * --------
 * Purpose: Wrapper class for a Win32 button.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "WindowBase.h"


//==============================
class Button : public WindowBase
//==============================
{ 
protected:
	HWND hwnd;

public:
	// Create a new button.
	void Create(HWND parent, const TCHAR *text, int x, int y, int width, int height)
	{
		// Remove old instance, if necessary
		Destroy();

		hwnd = CreateWindow(_T("BUTTON"),
			text,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
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
};
