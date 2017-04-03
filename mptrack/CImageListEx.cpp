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
		|| (mpt::Windows::IsWine() && !mpt::Wine::VersionContext().Version().IsAtLeast(mpt::Wine::Version(1,6,0)))
		|| GetDeviceCaps(dc->GetSafeHdc(), BITSPIXEL) * GetDeviceCaps(dc->GetSafeHdc(), PLANES) < 32)
	{
		uint32 rowSize = (bitmap->width + 31u) / 32u * 4u;
		std::vector<uint8> bitmapMask(rowSize * bitmap->height);

		const COLORREF buttonColor = GetSysColor(COLOR_BTNFACE);
		const uint8_t r = GetRValue(buttonColor), g = GetGValue(buttonColor), b = GetBValue(buttonColor);

		PNG::Pixel *pixel = bitmap->GetPixels();
		for(uint32 y = 0; y < bitmap->height; y++)
		{
			uint8 *mask = bitmapMask.data() + rowSize * y;
			for(uint32 x = 0; x < bitmap->width; x++, pixel++)
			{
				if(pixel->a != 0)
				{
					// Pixel not fully transparent - multiply with default background colour
#define MIXCOLOR(c) (((pixel->##c) * pixel->a + (c) * (255 - pixel->a)) >> 8);
					pixel->r = MIXCOLOR(r);
					pixel->g = MIXCOLOR(g);
					pixel->b = MIXCOLOR(b);
#undef MIXCOLOR
				} else
				{
					// Transparent pixel
					mask[x / 8u] |= 1 << (7 - (x & 7));
				}
			}
		}

		CBitmap dib, dibMask;
		bitmap->ToDIB(dib, dc);

		struct
		{
			BITMAPINFOHEADER bi;
			RGBQUAD col[2];
		} bi;
		MemsetZero(bi);
		bi.bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.bi.biWidth = bitmap->width;
		bi.bi.biHeight = -(int32)bitmap->height;
		bi.bi.biPlanes = 1;
		bi.bi.biBitCount = 1;
		bi.bi.biCompression = BI_RGB;
		bi.bi.biSizeImage = static_cast<DWORD>(bitmapMask.size() * sizeof(decltype(bitmapMask[0])));
		bi.col[1].rgbBlue = bi.col[1].rgbGreen = bi.col[1].rgbRed = 255;

		if(dc == nullptr) dc = CDC::FromHandle(GetDC(NULL));
		dibMask.CreateCompatibleBitmap(dc, bitmap->width, bitmap->height);
		SetDIBits(dc->GetSafeHdc(), dibMask, 0, bitmap->height, bitmapMask.data(), reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

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
