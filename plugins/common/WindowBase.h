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


OPENMPT_NAMESPACE_BEGIN


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
	
	// Scale X coordinate for DPI-awareness
	static int ScaleX(HWND hwnd, int x)
	{
		HDC dc = ::GetDC(hwnd);
		int dpi = MulDiv(x, ::GetDeviceCaps(dc, LOGPIXELSX), 96);
		::ReleaseDC(hwnd, dc);
		return dpi;
	}
	// Scale Y coordinate for DPI-awareness
	static int ScaleY(HWND hwnd, int y)
	{
		HDC dc = ::GetDC(hwnd);
		int dpi = MulDiv(y, ::GetDeviceCaps(dc, LOGPIXELSY), 96);
		::ReleaseDC(hwnd, dc);
		return dpi;
	}
};


OPENMPT_NAMESPACE_END
