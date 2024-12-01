/*
 * HighDPISupport.cpp
 * ------------------
 * Purpose: Various helpers for making high-DPI and mixed-DPI support easier.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "HighDPISupport.h"
#include "../misc/mptLibrary.h"


OPENMPT_NAMESPACE_BEGIN


struct HighDPISupportData
{
	HighDPISupportData()
		: m_user32{mpt::LibraryPath::System(P_("user32"))}
	{
		m_user32.Bind(m_SetThreadDpiAwarenessContext, "SetThreadDpiAwarenessContext");
		m_user32.Bind(m_GetDpiForWindow, "GetDpiForWindow");
		m_user32.Bind(m_GetSystemMetricsForDpi, "GetSystemMetricsForDpi");
		m_user32.Bind(m_SystemParametersInfoForDpi, "SystemParametersInfoForDpi");
		m_user32.Bind(m_AdjustWindowRectExForDpi, "AdjustWindowRectExForDpi");
	}

	mpt::Library m_user32;

	using PSETTHREADDPIAWARENESSCONTEXT = HANDLE(WINAPI *)(HANDLE);
	PSETTHREADDPIAWARENESSCONTEXT m_SetThreadDpiAwarenessContext = nullptr;

	using PGETDPIFORWINDOW = UINT(WINAPI *)(HWND);
	PGETDPIFORWINDOW m_GetDpiForWindow = nullptr;

	using PGETSYSTEMMETRICSFORPDI = int(WINAPI *)(int, UINT);
	PGETSYSTEMMETRICSFORPDI m_GetSystemMetricsForDpi = nullptr;

	using PSYSTEMPARAMETERSINFOFORPDI = BOOL(WINAPI *)(UINT, UINT, void *, UINT, UINT);
	PSYSTEMPARAMETERSINFOFORPDI m_SystemParametersInfoForDpi = nullptr;

	using PADJUSTWINDOWRECTEXFORDPI = BOOL(WINAPI *)(LPRECT, DWORD, BOOL, DWORD, UINT);
	PADJUSTWINDOWRECTEXFORDPI m_AdjustWindowRectExForDpi = nullptr;
};


enum MPT_DPI_AWARENESS_CONTEXT : intptr_t
{
	MPT_DPI_AWARENESS_CONTEXT_UNAWARE = -1,
	MPT_DPI_AWARENESS_CONTEXT_SYSTEM_AWARE = -2,
	MPT_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE = -3,
	MPT_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4,  // Windows 10 version 1703 and newer
	MPT_DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED = -5,     // Windows 10 version 1809 update and newer
};


static HANDLE DPIModeToContext(HighDPISupport::Mode mode)
{
	switch (mode)
	{
	case HighDPISupport::Mode::LowDpi: return HANDLE(MPT_DPI_AWARENESS_CONTEXT_UNAWARE);
	case HighDPISupport::Mode::LowDpiUpscaled: return HANDLE(MPT_DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
	case HighDPISupport::Mode::HighDpiSystem: return HANDLE(MPT_DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
	case HighDPISupport::Mode::HighDpiPerMonitor: return HANDLE(MPT_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	}
	MPT_ASSERT_NOTREACHED();
	return HANDLE(MPT_DPI_AWARENESS_CONTEXT_UNAWARE);
}


static HighDPISupportData *GetHighDPISupportData()
{
	static std::unique_ptr<HighDPISupportData> highDPISupportData;

	if(!highDPISupportData)
		highDPISupportData = std::make_unique<HighDPISupportData>();
	return highDPISupportData.get();
}


void HighDPISupport::SetDPIAwareness(Mode mode)
{
	auto instance = GetHighDPISupportData();
	// For Windows 10, Creators Update (1703) and newer
	if(instance->m_user32.IsValid())
	{
		using PSETPROCESSDPIAWARENESSCONTEXT = BOOL(WINAPI *)(HANDLE);
		PSETPROCESSDPIAWARENESSCONTEXT SetProcessDpiAwarenessContext = nullptr;
		if(instance->m_user32.Bind(SetProcessDpiAwarenessContext, "SetProcessDpiAwarenessContext"))
		{
			if(SetProcessDpiAwarenessContext(DPIModeToContext(mode)) == TRUE)
				return;
		}
	}

	// For Windows 8.1 and newer
	const bool highDPI = (mode == Mode::HighDpiSystem || mode == Mode::HighDpiPerMonitor);
	mpt::Library shcore(mpt::LibraryPath::System(P_("SHCore")));
	if(shcore.IsValid())
	{
		using PSETPROCESSDPIAWARENESS = HRESULT(WINAPI *)(int);
		PSETPROCESSDPIAWARENESS SetProcessDPIAwareness = nullptr;
		if(shcore.Bind(SetProcessDPIAwareness, "SetProcessDpiAwareness"))
		{
			if(SetProcessDPIAwareness(highDPI ? 2 : 0) == S_OK)
				return;
		}
	}
	
	// For Vista and newer
	if(highDPI && instance->m_user32.IsValid())
	{
		using PSETPROCESSDPIAWARE = BOOL(WINAPI *)();
		PSETPROCESSDPIAWARE SetProcessDPIAware = nullptr;
		if(instance->m_user32.Bind(SetProcessDPIAware, "SetProcessDPIAware"))
			SetProcessDPIAware();
	}
}


uint32 HighDPISupport::GetDpiForWindow(HWND hwnd)
{
	if(auto instance = GetHighDPISupportData(); instance->m_GetDpiForWindow)
		return instance->m_GetDpiForWindow(hwnd);

	HDC dc = ::GetDC(hwnd);
	uint32 dpi = ::GetDeviceCaps(dc, LOGPIXELSX);
	::ReleaseDC(hwnd, dc);
	return dpi;
}


int HighDPISupport::GetSystemMetrics(int index, uint32 dpi)
{
	if(auto instance = GetHighDPISupportData(); instance->m_GetSystemMetricsForDpi)
		return instance->m_GetSystemMetricsForDpi(index, dpi);
	else
		return ::GetSystemMetrics(index);
}


int HighDPISupport::GetSystemMetrics(int index, HWND hwnd)
{
	if(auto instance = GetHighDPISupportData(); instance->m_GetSystemMetricsForDpi)
		return instance->m_GetSystemMetricsForDpi(index, HighDPISupport::GetDpiForWindow(hwnd));
	else
		return ::GetSystemMetrics(index);
}


BOOL HighDPISupport::SystemParametersInfo(UINT uiAction, UINT uiParam, void *pvParam, UINT fWinIni, uint32 dpi)
{
	if(auto instance = GetHighDPISupportData(); instance->m_SystemParametersInfoForDpi)
		return instance->m_SystemParametersInfoForDpi(uiAction, uiParam, pvParam, fWinIni, dpi);
	else
		return ::SystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}


BOOL HighDPISupport::SystemParametersInfo(UINT uiAction, UINT uiParam, void *pvParam, UINT fWinIni, HWND hwnd)
{
	if(auto instance = GetHighDPISupportData(); instance->m_SystemParametersInfoForDpi)
		return instance->m_SystemParametersInfoForDpi(uiAction, uiParam, pvParam, fWinIni, HighDPISupport::GetDpiForWindow(hwnd));
	else
		return ::SystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}


BOOL HighDPISupport::AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, uint32 dpi)
{
	if(auto instance = GetHighDPISupportData(); instance->m_AdjustWindowRectExForDpi)
		return instance->m_AdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, dpi);
	else
		return ::AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}


void HighDPISupport::CreateGUIFont(CFont &font, HWND hwnd)
{
	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof(metrics);
	HighDPISupport::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, hwnd);
	font.DeleteObject();
	font.CreateFontIndirect(&metrics.lfMessageFont);
}


bool HighDPISupport::SetWindowPlacement(HWND hwnd, const WINDOWPLACEMENT &wpl)
{
	// https://blogs.windows.com/windowsdeveloper/2016/10/24/high-dpi-scaling-improvements-for-desktop-applications-and-mixed-mode-dpi-scaling-in-the-windows-10-anniversary-update/
	// What is not mentioned there: The first SetWindowPlacement call will restore the window to the wrong dimensions if it's not on the main screen...
	HighDPISupport::DPIAwarenessBypass bypass{HighDPISupport::Mode::LowDpi};
	::SetWindowPlacement(hwnd, &wpl);
	return ::SetWindowPlacement(hwnd, &wpl) != FALSE;
}


bool HighDPISupport::GetWindowPlacement(HWND hwnd, WINDOWPLACEMENT &wpl)
{
	// https://blogs.windows.com/windowsdeveloper/2016/10/24/high-dpi-scaling-improvements-for-desktop-applications-and-mixed-mode-dpi-scaling-in-the-windows-10-anniversary-update/
	HighDPISupport::DPIAwarenessBypass bypass{HighDPISupport::Mode::LowDpi};
	wpl.length = sizeof(WINDOWPLACEMENT);
	return ::GetWindowPlacement(hwnd, &wpl) != FALSE;
}


HighDPISupport::DPIAwarenessBypass::DPIAwarenessBypass(Mode forceMode)
{
	if(auto instance = GetHighDPISupportData(); instance->m_SetThreadDpiAwarenessContext)
		m_previous = instance->m_SetThreadDpiAwarenessContext(DPIModeToContext(forceMode));
}


HighDPISupport::DPIAwarenessBypass::~DPIAwarenessBypass()
{
	if(auto instance = GetHighDPISupportData(); instance->m_SetThreadDpiAwarenessContext)
		instance->m_SetThreadDpiAwarenessContext(m_previous);
}


OPENMPT_NAMESPACE_END
