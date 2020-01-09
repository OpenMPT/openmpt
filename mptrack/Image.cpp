/*
 * Image.cpp
 * ---------
 * Purpose: Bitmap and Vector image file handling using GDI+.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MPTrackUtil.h"
#include "Image.h"
#include "../common/FileReader.h"
#include "../common/ComponentManager.h"

// GDI+
#include <atlbase.h>
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#pragma warning(push)
#pragma warning(disable : 4458)  // declaration of 'x' hides class member
#include <gdiplus.h>
#pragma warning(pop)
#undef min
#undef max


OPENMPT_NAMESPACE_BEGIN


GdiplusRAII::GdiplusRAII()
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}


GdiplusRAII::~GdiplusRAII()
{
	Gdiplus::GdiplusShutdown(gdiplusToken);
	gdiplusToken = 0;
}


RawGDIDIB::RawGDIDIB(uint32 width, uint32 height)
    : width(width)
    , height(height)
    , pixels(width * height)
{
	MPT_ASSERT(width > 0);
	MPT_ASSERT(height > 0);
}


namespace GDIP
{


static CComPtr<IStream> GetStream(mpt::const_byte_span data)
{
	CComPtr<IStream> stream;
	stream.Attach(SHCreateMemStream(mpt::byte_cast<const unsigned char *>(data.data()), mpt::saturate_cast<UINT>(data.size())));
	if(!stream)
	{
		throw bad_image();
	}
	return stream;
}


std::unique_ptr<Gdiplus::Bitmap> LoadPixelImage(mpt::const_byte_span file)
{
	CComPtr<IStream> stream = GetStream(file);
	std::unique_ptr<Gdiplus::Bitmap> result = std::make_unique<Gdiplus::Bitmap>(stream, FALSE);
	if(result->GetLastStatus() != Gdiplus::Ok)
	{
		throw bad_image();
	}
	if(result->GetWidth() == 0 || result->GetHeight() == 0)
	{
		throw bad_image();
	}
	return result;
}


std::unique_ptr<Gdiplus::Bitmap> LoadPixelImage(FileReader file)
{
	FileReader::PinnedRawDataView view = file.GetPinnedRawDataView();
	return LoadPixelImage(view.span());
}


std::unique_ptr<Gdiplus::Metafile> LoadVectorImage(mpt::const_byte_span file)
{
	CComPtr<IStream> stream = GetStream(file);
	std::unique_ptr<Gdiplus::Metafile> result = std::make_unique<Gdiplus::Metafile>(stream);
	if(result->GetLastStatus() != Gdiplus::Ok)
	{
		throw bad_image();
	}
	if(result->GetWidth() == 0 || result->GetHeight() == 0)
	{
		throw bad_image();
	}
	return result;
}


std::unique_ptr<Gdiplus::Metafile> LoadVectorImage(FileReader file)
{
	FileReader::PinnedRawDataView view = file.GetPinnedRawDataView();
	return LoadVectorImage(view.span());
}


static std::unique_ptr<Gdiplus::Bitmap> DoResize(Gdiplus::Image &src, double scaling, int spriteWidth, int spriteHeight)
{
	const int width = src.GetWidth(), height = src.GetHeight();
	int newWidth = 0, newHeight = 0, newSpriteWidth = 0, newSpriteHeight = 0;

	if(spriteWidth <= 0 || spriteHeight <= 0)
	{
		newWidth = mpt::saturate_round<int>(width * scaling);
		newHeight = mpt::saturate_round<int>(height * scaling);
	} else
	{
		// Sprite mode: Source images consists of several sprites / icons that should be scaled individually
		newSpriteWidth = mpt::saturate_round<int>(spriteWidth * scaling);
		newSpriteHeight = mpt::saturate_round<int>(spriteHeight * scaling);
		newWidth = width * newSpriteWidth / spriteWidth;
		newHeight = height * newSpriteHeight / spriteHeight;
	}

	std::unique_ptr<Gdiplus::Bitmap> resizedImage = std::make_unique<Gdiplus::Bitmap>(newWidth, newHeight, PixelFormat32bppARGB);
	std::unique_ptr<Gdiplus::Graphics> resizedGraphics(Gdiplus::Graphics::FromImage(resizedImage.get()));
	if(scaling >= 1.5)
	{
		// Prefer crisp look on real high-DPI devices
		resizedGraphics->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
		resizedGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
		resizedGraphics->SetSmoothingMode(Gdiplus::SmoothingModeNone);
	} else
	{
		resizedGraphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
		resizedGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
		resizedGraphics->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	}
	if(spriteWidth <= 0 || spriteHeight <= 0)
	{
		resizedGraphics->DrawImage(&src, 0, 0, newWidth, newHeight);
	} else
	{
		// Draw each source sprite individually into separate image to avoid neighbouring source sprites bleeding in
		std::unique_ptr<Gdiplus::Bitmap> spriteImage = std::make_unique<Gdiplus::Bitmap>(spriteWidth, spriteHeight, PixelFormat32bppARGB);
		std::unique_ptr<Gdiplus::Graphics> spriteGraphics(Gdiplus::Graphics::FromImage(spriteImage.get()));

		for(int srcY = 0, destY = 0; srcY < height; srcY += spriteHeight, destY += newSpriteHeight)
		{
			for(int srcX = 0, destX = 0; srcX < width; srcX += spriteWidth, destX += newSpriteWidth)
			{
				spriteGraphics->Clear({0, 0, 0, 0});
				spriteGraphics->DrawImage(&src, Gdiplus::Rect(0, 0, spriteWidth, spriteHeight), srcX, srcY, spriteWidth, spriteHeight, Gdiplus::UnitPixel);
				resizedGraphics->DrawImage(spriteImage.get(), destX, destY, newSpriteWidth, newSpriteHeight);
			}
		}
	}
	return resizedImage;
}


std::unique_ptr<Gdiplus::Image> ResizeImage(Gdiplus::Image &src, double scaling, int spriteWidth, int spriteHeight)
{
	return DoResize(src, scaling, spriteWidth, spriteHeight);
}


std::unique_ptr<Gdiplus::Bitmap> ResizeImage(Gdiplus::Bitmap &src, double scaling, int spriteWidth, int spriteHeight)
{
	return DoResize(src, scaling, spriteWidth, spriteHeight);
}


}  // namespace GDIP


std::unique_ptr<RawGDIDIB> ToRawGDIDIB(Gdiplus::Bitmap &bitmap)
{
	Gdiplus::BitmapData bitmapData;
	Gdiplus::Rect rect{Gdiplus::Point{0, 0}, Gdiplus::Size{static_cast<INT>(bitmap.GetWidth()), static_cast<INT>(bitmap.GetHeight())}};
	std::unique_ptr<RawGDIDIB> result = std::make_unique<RawGDIDIB>(bitmap.GetWidth(), bitmap.GetHeight());
	if(bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
	{
		throw bad_image();
	}
	RawGDIDIB::Pixel *dst = result->Pixels().data();
	for(uint32 y = 0; y < result->Height(); ++y)
	{
		const GDIP::Pixel *src = GDIP::GetScanline(bitmapData, y);
		for(uint32 x = 0; x < result->Width(); ++x)
		{
			*dst = GDIP::ToRawGDIDIB(*src);
			src++;
			dst++;
		}
	}
	bitmap.UnlockBits(&bitmapData);
	return result;
}


std::unique_ptr<RawGDIDIB> LoadPixelImage(mpt::const_byte_span file, double scaling, int spriteWidth, int spriteHeight)
{
	auto bitmap = GDIP::LoadPixelImage(file);
	if(scaling != 1.0)
		bitmap = GDIP::ResizeImage(*bitmap, scaling, spriteWidth, spriteHeight);
	return ToRawGDIDIB(*bitmap);
}


std::unique_ptr<RawGDIDIB> LoadPixelImage(FileReader file, double scaling, int spriteWidth, int spriteHeight)
{
	auto bitmap = GDIP::LoadPixelImage(file);
	if(scaling != 1.0)
		bitmap = GDIP::ResizeImage(*bitmap, scaling, spriteWidth, spriteHeight);
	return ToRawGDIDIB(*bitmap);
}


// Create a DIB for the current device from our PNG.
bool CopyToCompatibleBitmap(CBitmap &dst, CDC &dc, const RawGDIDIB &src)
{
	BITMAPINFOHEADER bi;
	MemsetZero(bi);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = src.Width();
	bi.biHeight = -static_cast<LONG>(src.Height());
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = src.Width() * src.Height() * 4;
	if(!dst.CreateCompatibleBitmap(&dc, src.Width(), src.Height()))
	{
		return false;
	}
	if(!SetDIBits(dc.GetSafeHdc(), dst, 0, src.Height(), src.Pixels().data(), reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS))
	{
		return false;
	}
	return true;
}


bool CopyToCompatibleBitmap(CBitmap &dst, CDC &dc, Gdiplus::Image &src)
{
	if(!dst.CreateCompatibleBitmap(&dc, src.GetWidth(), src.GetHeight()))
	{
		return false;
	}
	CDC memdc;
	if(!memdc.CreateCompatibleDC(&dc))
	{
		return false;
	}
	memdc.SelectObject(dst);
	Gdiplus::Graphics gfx(memdc);
	if(gfx.DrawImage(&src, 0, 0) != Gdiplus::Ok)
	{
		return false;
	}
	return true;
}



bool LoadCompatibleBitmapFromPixelImage(CBitmap &dst, CDC &dc, mpt::const_byte_span file)
{
	try
	{
		std::unique_ptr<Gdiplus::Bitmap> pBitmap = GDIP::LoadPixelImage(file);
		if(!CopyToCompatibleBitmap(dst, dc, *pBitmap))
		{
			return false;
		}
	} catch(...)
	{
		return false;
	}
	return true;
}


bool LoadCompatibleBitmapFromPixelImage(CBitmap &dst, CDC &dc, FileReader file)
{
	try
	{
		std::unique_ptr<Gdiplus::Bitmap> pBitmap = GDIP::LoadPixelImage(file);
		if(!CopyToCompatibleBitmap(dst, dc, *pBitmap))
		{
			return false;
		}
	} catch(...)
	{
		return false;
	}
	return true;
}


OPENMPT_NAMESPACE_END
