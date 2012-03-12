/*
 * ComboBox.h
 * ----------
 * Purpose: Wrapper class for a Win32 static label.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include "windows.h"


//=========
class Label
//=========
{ 
protected:
	HWND label;

public:
	Label()
	{
		label = nullptr;
	}


	~Label()
	{
		Destroy();
	}


	// Create a new label
	void Create(HWND parent, const char *text, int x, int y, int width, int height)
	{
		// Remove old instance, if necessary
		Destroy();

		label = CreateWindow("STATIC",
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

		SendMessage(label, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
	}


	// Destroy the combo box.
	void Destroy()
	{
		if(label != nullptr)
		{
			DestroyWindow(label);
		}
		label = nullptr;
	}


	// Get the HWND associated with this combo box.
	HWND GetHwnd()
	{
		return label;
	}

}; 
