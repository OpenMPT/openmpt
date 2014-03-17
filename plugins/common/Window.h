/*
 * Window.h
 * --------
 * Purpose: Wrapper class for a Win32 window callback function.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "WindowBase.h"


//==============================
class Window : public WindowBase
//==============================
{ 
protected:
	WNDPROC originalWinProc;

public:
	Window() : originalWinProc(nullptr) { };

	// Register window callback function and initialize window
	void Create(void *windowHandle)
	{
		Destroy();

		hwnd = static_cast<HWND>(windowHandle);

		// Inject our own window processing function into the window.
		originalWinProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

		// Store pointer to the window object in the window's user data, so we have access to the custom callback function and original window processing function later.
		SetPropA(hwnd, "ptr", reinterpret_cast<HANDLE>(this));

		// Set background colour (or else, the plugin's background is black in Renoise)
		SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
	}


	// Destroy the window.
	void Destroy()
	{
		// Restore original window processing function.
		if(hwnd != nullptr)
		{
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWinProc));
		}
		hwnd = nullptr;
		originalWinProc = nullptr;
	}

protected:
	// Window processing callback function (to be implemented by user)
	virtual void WindowCallback(int message, void *param1, void *param2) = 0;

private:
	// Window processing callback function
	static INT_PTR CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		Window *owner = reinterpret_cast<Window *>(GetPropA(hWnd, "ptr"));

		if(owner != nullptr)
		{
			// Custom callback function
			owner->WindowCallback(static_cast<int>(msg), reinterpret_cast<void *>(wParam), reinterpret_cast<void *>(lParam));

			if(owner->originalWinProc != nullptr)
			{
				// Original callback function
				return owner->originalWinProc(hWnd, msg, wParam, lParam);
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

}; 
