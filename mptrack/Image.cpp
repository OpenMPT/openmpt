/*
 * Image.cpp
 * ---------
 * Purpose: Bitmap and Vector image file handling.
 * Notes  : Either uses GDI+ or a simple custom PNG loader.
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
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#pragma warning(push)
#pragma warning(disable:4458) // declaration of 'x' hides class member
#include <gdiplus.h>
#pragma warning(pop)
#undef min
#undef max


OPENMPT_NAMESPACE_BEGIN


GdiplusRAII::GdiplusRAII()
	: gdiplusToken(0)
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
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
	stream.Attach(SHCreateMemStream(mpt::byte_cast<const unsigned char*>(data.data()), mpt::saturate_cast<UINT>(data.size())));
#else
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, data.size());
	if(hGlobal == NULL)
	{
		throw bad_image();
	}
	void * mem = GlobalLock(hGlobal);
	if(!mem)
	{
		hGlobal = GlobalFree(hGlobal);
		throw bad_image();
	}
	std::memcpy(mem, data.data(), data.size());
	GlobalUnlock(hGlobal);
	if(CreateStreamOnHGlobal(hGlobal, TRUE, &stream) != S_OK)
	{
		hGlobal = GlobalFree(hGlobal);
		throw bad_image();
	}
	hGlobal = NULL;
#endif
	if(!stream)
	{
		throw bad_image();
	}
	return stream;
}


std::unique_ptr<Gdiplus::Bitmap> LoadPixelImage(mpt::const_byte_span file)
{
	CComPtr<IStream> stream = GetStream(file);
	std::unique_ptr<Gdiplus::Bitmap> result = mpt::make_unique<Gdiplus::Bitmap>(stream, FALSE);
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
	std::unique_ptr<Gdiplus::Metafile> result = mpt::make_unique<Gdiplus::Metafile>(stream);
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


std::unique_ptr<Gdiplus::Image> ResizeImage(Gdiplus::Image &src, double scaling)
{
	const int newWidth = mpt::saturate_round<int>(src.GetWidth() * scaling), newHeight = mpt::saturate_round<int>(src.GetHeight() * scaling);
	std::unique_ptr<Gdiplus::Image> resizedImage = mpt::make_unique<Gdiplus::Bitmap>(newWidth, newHeight, PixelFormat32bppARGB);
	std::unique_ptr<Gdiplus::Graphics> resizedGraphics(Gdiplus::Graphics::FromImage(resizedImage.get()));

	resizedGraphics->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
	resizedGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	resizedGraphics->SetSmoothingMode(Gdiplus::SmoothingModeNone);
	resizedGraphics->DrawImage(&src, 0, 0, newWidth, newHeight);
	return resizedImage;
}


std::unique_ptr<Gdiplus::Bitmap> ResizeImage(Gdiplus::Bitmap &src, double scaling)
{
	const int newWidth = mpt::saturate_round<int>(src.GetWidth() * scaling), newHeight = mpt::saturate_round<int>(src.GetHeight() * scaling);
	std::unique_ptr<Gdiplus::Bitmap> resizedImage = mpt::make_unique<Gdiplus::Bitmap>(newWidth, newHeight, PixelFormat32bppARGB);
	std::unique_ptr<Gdiplus::Graphics> resizedGraphics(Gdiplus::Graphics::FromImage(resizedImage.get()));

	resizedGraphics->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
	resizedGraphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	resizedGraphics->SetSmoothingMode(Gdiplus::SmoothingModeNone);
	resizedGraphics->DrawImage(&src, 0, 0, newWidth, newHeight);
	return resizedImage;
}


} // namespace GDPI


std::unique_ptr<RawGDIDIB> ToRawGDIDIB(Gdiplus::Bitmap &bitmap)
{
	Gdiplus::BitmapData bitmapData;
	Gdiplus::Rect rect{Gdiplus::Point{0, 0}, Gdiplus::Size{static_cast<INT>(bitmap.GetWidth()), static_cast<INT>(bitmap.GetHeight())}};
	std::unique_ptr<RawGDIDIB> result = mpt::make_unique<RawGDIDIB>(bitmap.GetWidth(), bitmap.GetHeight());
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


std::unique_ptr<RawGDIDIB> LoadPixelImage(mpt::const_byte_span file, double scaling)
{
	auto bitmap = GDIP::LoadPixelImage(file);
	if(scaling != 1.0)
		bitmap = GDIP::ResizeImage(*bitmap, scaling);
	return ToRawGDIDIB(*bitmap);
}


std::unique_ptr<RawGDIDIB> LoadPixelImage(FileReader file, double scaling)
{
	auto bitmap = GDIP::LoadPixelImage(file);
	if(scaling != 1.0)
		bitmap = GDIP::ResizeImage(*bitmap, scaling);
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
