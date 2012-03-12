/*
 * Window.h
 * --------
 * Purpose: Wrapper class for a Win32 window callback function.
 * Notes  : (currently none)
 * Authors: Johannes Schultz (OpenMPT Devs)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include "windows.h"


//==========
class Window
//==========
{ 
private:
	HWND window;
	WNDPROC originalWinProc;

public:
	Window() : window(nullptr), originalWinProc(nullptr) { };


	~Window()
	{
		DestroyWindow();
	}


	// Get system pointer to window
	HWND GetHwnd() const
	{
		return static_cast<HWND>(window);
	}


	// Register window callback function and initialize window
	void InitializeWindow(void *windowHandle)
	{
		DestroyWindow();

		window = static_cast<HWND>(windowHandle);

		// Inject our own window processing function into the window.
		originalWinProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

		// Store pointer to the window object in the window's user data, so we have access to the custom callback function and original window processing function later.
		SetWindowLongPtr(window, GWL_USERDATA, reinterpret_cast<LONG_PTR>(this));

		// Set background colour (or else, the plugin's background is black in Renoise)
		SetClassLongPtr(window, GCL_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE))); 
	}


	// Destroy the combo box.
	void DestroyWindow()
	{
		// Restore original window processing function.
		if(window != nullptr)
		{
			SetWindowLongPtr(window, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(originalWinProc));
		}
		window = nullptr;
		originalWinProc = nullptr;
	}

protected:
	// Window processing callback function (to be implemented by user)
	virtual void WindowCallback(int message, void *param1, void *param2) = 0;

private:
	// Window processing callback function
	static INT_PTR CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		Window *owner = reinterpret_cast<Window *>(GetWindowLongPtr(hWnd, GWL_USERDATA));

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
