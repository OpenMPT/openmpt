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

namespace Util { namespace sdOs
{
	/// Checks whether file or folder exists and whether it has the given mode.
	enum FileMode {FileModeExists = 0, FileModeRead = 4, FileModeWrite = 2, FileModeReadWrite = 6};
	bool IsPathFileAvailable(const mpt::PathString &pszFilePath, FileMode fm);

}} // namespace Util::sdOs

namespace Util
{
	// Insert a range of items [insStart,  insEnd], and possibly shift item fix to the left.
	template<typename T>
	void InsertItem(const T insStart, const T insEnd, T &fix)
	{
		ASSERT(insEnd >= insStart);
		if(fix >= insStart)
		{
			fix += (insEnd - insStart + 1);
		}
	}

	// Insert a range of items [insStart,  insEnd], and possibly shift items in range [fixStart, fixEnd] to the right.
	template<typename T>
	void InsertRange(const T insStart, const T insEnd, T &fixStart, T &fixEnd)
	{
		ASSERT(insEnd >= insStart);
		const T insLength = insEnd - insStart + 1;
		if(fixStart >= insEnd)
		{
			fixStart += insLength;
		}
		if(fixEnd >= insEnd)
		{
			fixEnd += insLength;
		}
	}

	// Delete a range of items [delStart,  delEnd], and possibly shift item fix to the left.
	template<typename T>
	void DeleteItem(const T delStart, const T delEnd, T &fix)
	{
		ASSERT(delEnd >= delStart);
		if(fix > delEnd)
		{
			fix -= (delEnd - delStart + 1);
		}
	}

	// Delete a range of items [delStart,  delEnd], and possibly shift items in range [fixStart, fixEnd] to the left.
	template<typename T>
	void DeleteRange(const T delStart, const T delEnd, T &fixStart, T &fixEnd)
	{
		ASSERT(delEnd >= delStart);
		const T delLength = delEnd - delStart + 1;
		if(delStart < fixStart  && delEnd < fixStart)
		{
			// cut part is before loop start
			fixStart -= delLength;
			fixEnd -= delLength;
		} else if(delStart < fixStart  && delEnd < fixEnd)
		{
			// cut part is partly before loop start
			fixStart = delStart;
			fixEnd -= delLength;
		} else if(delStart >= fixStart && delEnd < fixEnd)
		{
			// cut part is in the loop
			fixEnd -= delLength;
		} else if(delStart >= fixStart && delStart < fixEnd && delEnd > fixEnd)
		{
			// cut part is partly before loop end
			fixEnd = delStart;
		}
	}

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
