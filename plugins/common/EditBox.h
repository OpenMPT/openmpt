/*
 * EditBox.h
 * ---------
 * Purpose: Wrapper class for a Win32 edit box.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "WindowBase.h"


//===============================
class EditBox : public WindowBase
//===============================
{ 
public:
	// Create a new edit box.
	void Create(HWND parent, const TCHAR *text, int x, int y, int width, int height, bool numbersOnly = false)
	{
		// Remove old instance, if necessary
		Destroy();

		hwnd = CreateWindow(_T("EDIT"),
			text,
			WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_AUTOVSCROLL | (numbersOnly ? ES_NUMBER : 0),
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


	void SetText(const TCHAR *text)
	{
		Edit_SetText(hwnd, text);
	}


	template<size_t length>
	void GetText(TCHAR (&text)[length])
	{
		Edit_GetText(hwnd, text, length);
	}
};
