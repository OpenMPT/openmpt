/*
 * ComboBox.h
 * ----------
 * Purpose: Wrapper class for a Win32 static label.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "WindowBase.h"


OPENMPT_NAMESPACE_BEGIN


//=============================
class Label : public WindowBase
//=============================
{ 
public:
	// Create a new label
	void Create(HWND parent, const TCHAR *text, int x, int y, int width, int height)
	{
		// Remove old instance, if necessary
		Destroy();

		hwnd = CreateWindow(_T("STATIC"),
			text,
			WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
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


OPENMPT_NAMESPACE_END
