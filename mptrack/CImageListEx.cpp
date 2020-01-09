/*
 * CImageListEx.cpp
 * ----------------
 * Purpose: A class that extends MFC's CImageList to handle alpha-blended images properly.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "CImageListEx.h"
#include "Image.h"

#include "Mptrack.h"


OPENMPT_NAMESPACE_BEGIN


bool CImageListEx::Create(UINT resourceID, int cx, int cy, int nInitial, int nGrow, CDC *dc, double scaling, bool disabled)
{
	std::unique_ptr<RawGDIDIB> bitmap;
	try
	{
		bitmap = LoadPixelImage(GetResource(MAKEINTRESOURCE(resourceID), _T("PNG")), scaling, cx, cy);
		cx = mpt::saturate_round<int>(cx * scaling);
		cy = mpt::saturate_round<int>(cy * scaling);
	} catch(...)
	{
		return false;
	}

	if(disabled)
	{
		// Grayed out icons
		for(auto &pixel : bitmap->Pixels())
		{
			if(pixel.a != 0)
			{
				uint8 y = static_cast<uint8>(pixel.r * 0.299f + pixel.g * 0.587f + pixel.b * 0.114f);
				pixel.r = pixel.g = pixel.b = y;
				pixel.a /= 2;
			}
		}
	}

	bool result;

	if(dc == nullptr) dc = CDC::FromHandle(GetDC(NULL));

	// Use 1-bit transperency when there is no alpha channel.
	if(GetDeviceCaps(dc->GetSafeHdc(), BITSPIXEL) * GetDeviceCaps(dc->GetSafeHdc(), PLANES) < 32)
	{
		uint32 rowSize = (bitmap->Width() + 31u) / 32u * 4u;
		std::vector<uint8> bitmapMask(rowSize * bitmap->Height());

		const COLORREF buttonColor = GetSysColor(COLOR_BTNFACE);
		const uint8 r = GetRValue(buttonColor), g = GetGValue(buttonColor), b = GetBValue(buttonColor);

		RawGDIDIB::Pixel *pixel = bitmap->Pixels().data();
		for(uint32 y = 0; y < bitmap->Height(); y++)
		{
			uint8 *mask = bitmapMask.data() + rowSize * y;
			for(uint32 x = 0; x < bitmap->Width(); x++, pixel++)
			{
				if(pixel->a != 0)
				{
					// Pixel not fully transparent - multiply with default background colour
#define MIXCOLOR(c) (((pixel-> c ) * pixel->a + (c) * (255 - pixel->a)) >> 8);
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

		CBitmap ddb, dibMask;
		CopyToCompatibleBitmap(ddb, *dc, *bitmap);

		struct
		{
			BITMAPINFOHEADER bi;
			RGBQUAD col[2];
		} bi;
		MemsetZero(bi);
		bi.bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.bi.biWidth = bitmap->Width();
		bi.bi.biHeight = -(int32)bitmap->Height();
		bi.bi.biPlanes = 1;
		bi.bi.biBitCount = 1;
		bi.bi.biCompression = BI_RGB;
		bi.bi.biSizeImage = static_cast<DWORD>(bitmapMask.size() * sizeof(decltype(bitmapMask[0])));
		bi.col[1].rgbBlue = bi.col[1].rgbGreen = bi.col[1].rgbRed = 255;

		dibMask.CreateCompatibleBitmap(dc, bitmap->Width(), bitmap->Height());
		SetDIBits(dc->GetSafeHdc(), dibMask, 0, bitmap->Height(), bitmapMask.data(), reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

		result = CImageList::Create(cx, cy, ILC_COLOR24 | ILC_MASK, nInitial, nGrow)
			&& CImageList::Add(&ddb, &dibMask);
	} else
	{
		// 32-bit image on modern system

		// Make fully transparent pixels use the mask color. This should hopefully make the icons look "somewhat" okay
		// on system where the alpha channel is magically missing in 32-bit mode (https://bugs.openmpt.org/view.php?id=520)
		for(auto &pixel : bitmap->Pixels())
		{
			if(pixel.a == 0)
			{
				pixel.r = 255;
				pixel.g = 0;
				pixel.b = 255;
			}
		}

		CBitmap ddb;
		CopyToCompatibleBitmap(ddb, *dc, *bitmap);
		result = CImageList::Create(cx, cy, ILC_COLOR32 | ILC_MASK, nInitial, nGrow)
			&& CImageList::Add(&ddb, RGB(255, 0, 255));
	}

	return result;
}


OPENMPT_NAMESPACE_END
