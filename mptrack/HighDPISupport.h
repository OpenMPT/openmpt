/*
 * HighDPISupport.h
 * ----------------
 * Purpose: Various helpers for making high-DPI and mixed-DPI support easier.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

#if !defined(WM_DPICHANGED)
#define WM_DPICHANGED 0x02E0
#endif
#if !defined(WM_DPICHANGED_BEFOREPARENT)
#define WM_DPICHANGED_BEFOREPARENT 0x02E2
#endif
#if !defined(WM_DPICHANGED_AFTERPARENT)
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

namespace HighDPISupport
{
	enum class Mode
	{
		LowDpi,
		LowDpiUpscaled,
		HighDpiSystem,
		HighDpiPerMonitor,
	};

	void SetDPIAwareness(Mode mode);

	uint32 GetDpiForWindow(HWND hwnd);
	int GetSystemMetrics(int index, uint32 dpi);
	int GetSystemMetrics(int index, HWND hwnd);
	BOOL SystemParametersInfo(UINT uiAction, UINT uiParam, void *pvParam, UINT fWinIni, uint32 dpi);
	BOOL SystemParametersInfo(UINT uiAction, UINT uiParam, void *pvParam, UINT fWinIni, HWND hwnd);
	BOOL AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, uint32 dpi);

	void CreateGUIFont(CFont &font, HWND hwnd);

	// Normalized to 96 DPI to avoid issues in mixed-DPI contexts
	bool SetWindowPlacement(HWND hwnd, const WINDOWPLACEMENT &wpl);
	// Normalized to 96 DPI to avoid issues in mixed-DPI contexts
	bool GetWindowPlacement(HWND hwnd, WINDOWPLACEMENT &wpl);

	// Applies DPI scaling factor to some given size
	MPT_FORCEINLINE int ScalePixels(int pixels, HWND hwnd)
	{
		return MulDiv(pixels, HighDPISupport::GetDpiForWindow(hwnd), 96);
	}

	// Removes DPI scaling factor from some given size
	MPT_FORCEINLINE int ScalePixelsInv(int pixels, HWND hwnd)
	{
		return MulDiv(pixels, 96, HighDPISupport::GetDpiForWindow(hwnd));
	}

	class DPIAwarenessBypass
	{
	public:
		DPIAwarenessBypass(Mode forceMode = Mode::HighDpiSystem);
		~DPIAwarenessBypass();

	private:
		HANDLE m_previous = {};
	};
};

OPENMPT_NAMESPACE_END
