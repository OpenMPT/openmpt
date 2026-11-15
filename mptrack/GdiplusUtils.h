/*
 * GdiplusUtils.h
 * --------------
 * Purpose: GDI+ includes and various tiny helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#if MPT_MSVC_BEFORE(2019, 11)  // really < Windows 10 SDK 2104 (10.0.20348.0)
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4458)  // declaration of 'x' hides class member
#endif
#include <gdiplus.h>
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif
#if MPT_MSVC_BEFORE(2019, 11)  // really < Windows 10 SDK 2104 (10.0.20348.0)
#undef min
#undef max
#endif


OPENMPT_NAMESPACE_BEGIN

namespace GDIP
{

inline Gdiplus::Color ToColor(COLORREF color)
{
	return Gdiplus::Color{GetRValue(color), GetGValue(color), GetBValue(color)};
}


inline void SetupGraphics(Gdiplus::Graphics &graphics)
{
	// Pixel (x, y) covers the area from (x, y) to (x + 1, y + 1), just like with classic GDI
	graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
}


// First pixel of a line with the given width that should be centered around pixel pos
// Assumes Gdiplus::PixelOffsetModeHalf, as configured in SetupGraphics.
inline int LineStart(int pos, int width)
{
	return pos - (width - 1) / 2;
}

// Pen coordinate for a line with the given width centered around pixel pos
// Assumes Gdiplus::PixelOffsetModeHalf, as configured in SetupGraphics.
inline Gdiplus::REAL LineCenter(int pos, int width)
{
	return static_cast<Gdiplus::REAL>(LineStart(pos, width)) + width * 0.5f;
}


}  // namespace GdiplusUtils

OPENMPT_NAMESPACE_END
