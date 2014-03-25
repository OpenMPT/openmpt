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
	BOOL Create(UINT resourceID, int cx, int cy, int nInitial, int nGrow)
	{
		CBitmap *bitmap = ReadPNG(MAKEINTRESOURCE(resourceID));
		if(bitmap == nullptr)
		{
			return FALSE;
		}

#ifdef _M_IX86
		// This is only relevant for Win9x (testing for supported SDK version won't work here because we only support Win9x through KernelEx).
		// Win9x only supports 1-bit transparency, so we pre-multiply all pixels with the default button face colour and create a transparency mask.
		if(!mpt::Windows::IsWinNT())
		{
			BITMAP bm;
			bitmap->GetBitmap(&bm);
			uint32_t bitmapSize = bm.bmWidthBytes * bm.bmHeight;

			std::vector<RGBQUAD> bitmapBytes(bitmapSize / sizeof(RGBQUAD)), maskBytes(bitmapSize / sizeof(RGBQUAD));
			bitmap->GetBitmapBits(bitmapSize, &bitmapBytes[0]);

			CBitmap bitmapMask;
			bitmapMask.CreateBitmapIndirect(&bm);
			bitmapMask.GetBitmapBits(bitmapSize, &maskBytes[0]);

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
				return FALSE;
			}
			CImageList::Add(bitmap, &bitmapMask);
		} else
#endif
		{
			if(!CImageList::Create(cx, cy, ILC_COLOR32, nInitial, nGrow))
			{
				return FALSE;
			}
			CImageList::Add(bitmap, RGB(0, 0, 0));
		}
		bitmap->Detach();
		delete bitmap;
		return TRUE;
	}
};