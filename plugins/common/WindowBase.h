/*
 * WindowBase.h
 * ------------
 * Purpose: Base class for Win32 wrapper classes.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <WindowsX.h>
#include <tchar.h>


//==============
class WindowBase
//==============
{
protected:
	HWND hwnd;

public:
	WindowBase()
	{
		hwnd = nullptr;
	}

	~WindowBase()
	{
		Destroy();
	}

	// Destroy the window.
	void Destroy()
	{
		if(hwnd != nullptr)
		{
			DestroyWindow(hwnd);
		}
		hwnd = nullptr;
	}

	// Get the HWND associated with this window.
	HWND GetHwnd()
	{
		return hwnd;
	}
};
