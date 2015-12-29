/*
 * CImageListEx.cpp
 * ----------------
 * Purpose: A class that extends MFC's CImageList to handle alpha-blended images properly.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "stdafx.h"
#include "CImageListEx.h"
#include "PNG.h"


OPENMPT_NAMESPACE_BEGIN


bool CImageListEx::Create(UINT resourceID, int cx, int cy, int nInitial, int nGrow, CDC *dc, bool disabled)
{
	PNG::Bitmap *bitmap = PNG::ReadPNG(MAKEINTRESOURCE(resourceID));
	if(bitmap == nullptr)
	{
		return false;
	}

	if(disabled)
	{
		// Grayed out icons
		PNG::Pixel *pixel = bitmap->GetPixels();
		for(size_t i = bitmap->GetNumPixels(); i != 0; i--, pixel++)
		{
			if(pixel->a != 0)
			{
				uint8_t y = static_cast<uint8_t>(pixel->r * 0.299f + pixel->g * 0.587f + pixel->b * 0.114f);
				pixel->r = pixel->g = pixel->b = y;
				pixel->a /= 2;
			}
		}
	}

	bool result;

	// Icons with alpha transparency screw up in Windows 2000 and older as well as Wine 1.4 and older.
	// They all support 1-bit transparency, though, so we pre-multiply all pixels with the default button face colour and create a transparency mask.
	// Additionally, since we create a bitmap that fits the device's bit depth, we apply this fix when using a bit depth that doesn't have an alpha channel.
	if(mpt::Windows::Version::Current().IsBefore(mpt::Windows::Version::WinXP)
		|| mpt::Windows::IsWine()
		|| GetDeviceCaps(dc->GetSafeHdc(), BITSPIXEL) * GetDeviceCaps(dc->GetSafeHdc(), PLANES) < 32)
	{
		PNG::Bitmap bitmapMask(bitmap->width, bitmap->height);

		const COLORREF buttonColor = GetSysColor(COLOR_BTNFACE);
		const uint8_t r = GetRValue(buttonColor), g = GetGValue(buttonColor), b = GetBValue(buttonColor);

		PNG::Pixel *pixel = bitmap->GetPixels(), *mask = bitmapMask.GetPixels();
		for(size_t i = bitmap->GetNumPixels(); i != 0; i--, pixel++, mask++)
		{
			if(pixel->a != 0)
			{
				// Pixel not fully transparent - multiply with default background colour
#define MIXCOLOR(c) (((pixel->##c) * pixel->a + (c) * (255 - pixel->a)) >> 8);
				pixel->r = MIXCOLOR(r);
				pixel->g = MIXCOLOR(g);
				pixel->b = MIXCOLOR(b);
#undef MIXCOLOR
				mask->r = 0;
				mask->g = 0;
				mask->b = 0;
				mask->a = 255;
			} else
			{
				// Transparent pixel
				mask->r = 255;
				mask->g = 255;
				mask->b = 255;
				mask->a = 255;
			}
		}

		CBitmap dib, dibMask;
		bitmap->ToDIB(dib, dc);
		bitmapMask.ToDIB(dibMask, dc);

		result = CImageList::Create(cx, cy, ILC_COLOR24 | ILC_MASK, nInitial, nGrow)
			&& CImageList::Add(&dib, &dibMask);
	} else
	{
		// 32-bit image on modern system

		// Make fully transparent pixels use the mask color. This should hopefully make the icons look "somewhat" okay
		// on system where the alpha channel is magically missing in 32-bit mode (http://bugs.openmpt.org/view.php?id=520)
		PNG::Pixel *pixel = bitmap->GetPixels();
		for(size_t i = bitmap->GetNumPixels(); i != 0; i--, pixel++)
		{
			if(pixel->a == 0)
			{
				pixel->r = 255;
				pixel->g = 0;
				pixel->b = 255;
			}
		}

		CBitmap dib;
		bitmap->ToDIB(dib, dc);
		result = CImageList::Create(cx, cy, ILC_COLOR32 | ILC_MASK, nInitial, nGrow)
			&& CImageList::Add(&dib, RGB(255, 0, 255));
	}
	delete bitmap;
	return true;
}


OPENMPT_NAMESPACE_END
