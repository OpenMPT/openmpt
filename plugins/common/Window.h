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


OPENMPT_NAMESPACE_BEGIN


//==============================
class Window : public WindowBase
//==============================
{
protected:
	const TCHAR *windowClassName;

public:
	Window() : windowClassName(nullptr) { }

	// Register window callback function and initialize window
	void Create(HWND parent, const ERect &rect, const TCHAR *className)
	{
		Destroy();

		WNDCLASSEX windowClass;
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WndProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = GetModuleHandle(NULL);
		windowClass.hIcon = NULL;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = windowClassName = className;
		windowClass.hIconSm = NULL;
		RegisterClassEx(&windowClass);

		hwnd = CreateWindow(className, nullptr, WS_VISIBLE | WS_CHILD, ScaleX(parent, rect.left), ScaleY(parent, rect.top), ScaleX(parent, rect.right - rect.left), ScaleY(parent, rect.bottom - rect.top), parent, NULL, windowClass.hInstance, NULL);

		// Store pointer to the window object in the window's user data, so we have access to the custom callback function and original window processing function later.
		SetPropA(hwnd, "ptr", reinterpret_cast<HANDLE>(this));
	}


	// Destroy the window.
	void Destroy()
	{
		DestroyWindow(hwnd);
		UnregisterClass(windowClassName, GetModuleHandle(NULL));
		hwnd = nullptr;
	}

protected:
	// Window processing callback function (to be implemented by user)
	virtual intptr_t WindowCallback(int message, void *param1, void *param2)
	{
		return DefWindowProc(hwnd, message, reinterpret_cast<WPARAM>(param1), reinterpret_cast<LPARAM>(param2));
	}

private:
	// Window processing callback function
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		Window *owner = reinterpret_cast<Window *>(GetPropA(hWnd, "ptr"));

		if(owner != nullptr)
		{
			// Custom callback function
			return owner->WindowCallback(static_cast<int>(msg), reinterpret_cast<void *>(wParam), reinterpret_cast<void *>(lParam));
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

}; 


OPENMPT_NAMESPACE_END
