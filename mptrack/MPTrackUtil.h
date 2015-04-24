/*
 * MPTrackUtil.h
 * -------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include <string>


OPENMPT_NAMESPACE_BEGIN


LPCCH LoadResource(LPCTSTR lpName, LPCTSTR lpType, LPCCH& pData, size_t& nSize, HGLOBAL& hglob);

std::string GetErrorMessage(DWORD nErrorCode);

namespace Util
{
	// Get horizontal DPI resolution
	forceinline int GetDPIx(HWND hwnd)
	{
		HDC dc = ::GetDC(hwnd);
		int dpi = ::GetDeviceCaps(dc, LOGPIXELSX);
		::ReleaseDC(hwnd, dc);
		return dpi;
	}

	// Get vertical DPI resolution
	forceinline int GetDPIy(HWND hwnd)
	{
		HDC dc = ::GetDC(hwnd);
		int dpi = ::GetDeviceCaps(dc, LOGPIXELSY);
		::ReleaseDC(hwnd, dc);
		return dpi;
	}

	// Applies DPI scaling factor to some given size
	forceinline int ScalePixels(int pixels, HWND hwnd)
	{
		return MulDiv(pixels, GetDPIx(hwnd), 96);
	}

	// Removes DPI scaling factor from some given size
	forceinline int ScalePixelsInv(int pixels, HWND hwnd)
	{
		return MulDiv(pixels, 96, GetDPIx(hwnd));
	}
}


OPENMPT_NAMESPACE_END
