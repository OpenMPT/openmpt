/*
 * CImageListEx.h
 * --------------
 * Purpose: A class that extends MFC's CImageList to handle alpha-blended images properly. Also provided 1-bit transparency fallback for legacy Win9x systems.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

class CImageListEx : public CImageList
{
public:
	bool Create(UINT resourceID, int cx, int cy, int nInitial, int nGrow, bool disabled = false)
	{
		CBitmap *bitmap = ReadPNG(MAKEINTRESOURCE(resourceID));
		if(bitmap == nullptr)
		{
			return false;
		}

		BITMAP bm;
		bitmap->GetBitmap(&bm);
		const uint32_t bitmapSize = bm.bmWidthBytes * bm.bmHeight;

		std::vector<RGBQUAD> bitmapBytes(bitmapSize / sizeof(RGBQUAD));
		bitmap->GetBitmapBits(bitmapSize, &bitmapBytes[0]);

		if(disabled)
		{
			// Grayed out icons
			RGBQUAD *pixel = &bitmapBytes[0];
			for(size_t i = bitmapBytes.size(); i != 0; i--, pixel++)
			{
				if(pixel->rgbReserved != 0)
				{
					uint8_t y = static_cast<uint8_t>(pixel->rgbRed * 0.299f + pixel->rgbGreen * 0.587f + pixel->rgbBlue * 0.114f);
					pixel->rgbRed = pixel->rgbGreen = pixel->rgbBlue = y;
					pixel->rgbReserved /= 2;
				}
			}
			bitmap->SetBitmapBits(bitmapSize, &bitmapBytes[0]);
		}

		// Icons with alpha transparency screw up in Windows 2000 and older as well as Wine 1.4 and older.
		// They all support 1-bit transparency, though, so we pre-multiply all pixels with the default button face colour and create a transparency mask.
		if(!mpt::Windows::Version::IsAtLeast(mpt::Windows::Version::WinXP) || mpt::Windows::Version::IsWine())
		{
			CBitmap bitmapMask;
			bitmapMask.CreateBitmapIndirect(&bm);
			std::vector<RGBQUAD> maskBytes(bitmapSize / sizeof(RGBQUAD));

			const COLORREF buttonColor = GetSysColor(COLOR_BTNFACE);
			const uint8_t r = GetRValue(buttonColor), g = GetGValue(buttonColor), b = GetBValue(buttonColor);

			RGBQUAD *pixel = &bitmapBytes[0], *mask = &maskBytes[0];
			for(size_t i = bitmapBytes.size(); i != 0; i--, pixel++, mask++)
			{
				if(pixel->rgbReserved != 0)
				{
					// Pixel not fully transparent - multiply with default background colour
#define MIXCOLOR(a, b) (((a) * pixel->rgbReserved + (b) * (255 - pixel->rgbReserved)) >> 8);
					pixel->rgbRed = MIXCOLOR(pixel->rgbRed, r);
					pixel->rgbGreen = MIXCOLOR(pixel->rgbGreen, g);
					pixel->rgbBlue = MIXCOLOR(pixel->rgbBlue, b);
					pixel->rgbReserved = 255;
#undef MIXCOLOR
					mask->rgbRed = 0;
					mask->rgbGreen = 0;
					mask->rgbBlue = 0;
					mask->rgbReserved = 255;
				} else
				{
					// Transparent pixel
					mask->rgbRed = 255;
					mask->rgbGreen = 255;
					mask->rgbBlue = 255;
					mask->rgbReserved = 255;
				}
			}
			bitmap->SetBitmapBits(bitmapSize, &bitmapBytes[0]);
			bitmapMask.SetBitmapBits(bitmapSize, &maskBytes[0]);

			if(!CImageList::Create(cx, cy, ILC_COLOR32 | ILC_MASK, nInitial, nGrow))
			{
				delete bitmap;
				return false;
			}
			CImageList::Add(bitmap, &bitmapMask);
			bitmapMask.Detach();
		} else
		{
			if(!CImageList::Create(cx, cy, ILC_COLOR32, nInitial, nGrow))
			{
				delete bitmap;
				return false;
			}
			CImageList::Add(bitmap, RGB(0, 0, 0));
		}
		bitmap->Detach();
		delete bitmap;
		return true;
	}
};