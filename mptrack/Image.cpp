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

#ifdef MPT_WITH_GDIPLUS
#include <atlbase.h>
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#pragma warning(push)
#pragma warning(disable:4458) // declaration of 'x' hides class member
#include <gdiplus.h>
#pragma warning(pop)
#else // !MPT_WITH_GDIPLUS
#if defined(MPT_WITH_ZLIB)
#include <zlib.h>
#elif defined(MPT_WITH_MINIZ)
#include <miniz/miniz.h>
#endif
#endif // MPT_WITH_GDIPLUS


OPENMPT_NAMESPACE_BEGIN


#ifndef MPT_WITH_GDIPLUS

static std::unique_ptr<RawGDIDIB> ReadPNG(FileReader file)
{
	file.Rewind();
	if(!file.ReadMagic("\211PNG\r\n\032\n"))
	{
		throw bad_image();
	}

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t bitDepth;
	uint8_t colorType;
	uint8_t compressionMethod;
	uint8_t filterMethod;
	uint8_t interlaceMethod;

	std::vector<uint8_t> dataIn;
	std::vector<RawGDIDIB::Pixel> palette;

	while(file.CanRead(12))
	{
		uint32_t chunkLength = file.ReadUint32BE();
		char magic[4];
		file.ReadArray(magic);
		FileReader chunk = file.ReadChunk(chunkLength);
		file.Skip(4);	// CRC32
		if(!memcmp(magic, "IHDR", 4))
		{
			// Image header
			width = chunk.ReadUint32BE();
			height = chunk.ReadUint32BE();
			bitDepth = chunk.ReadUint8();
			colorType = chunk.ReadUint8();
			compressionMethod = chunk.ReadUint8();
			filterMethod = chunk.ReadUint8();
			interlaceMethod = chunk.ReadUint8();
			ASSERT(!filterMethod && !interlaceMethod);
		} else if(!memcmp(magic, "IDAT", 4))
		{
			// Data block(s)
			z_stream strm;
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = static_cast<uInt>(chunk.GetLength());
			strm.next_in = const_cast<Bytef*>(chunk.GetRawData<Bytef>());
			if(inflateInit2(&strm, 15) != Z_OK)
			{
				break;
			}
			int retVal;
			do
			{
				dataIn.resize(dataIn.size() + 4096);
				strm.avail_out = 4096;
				strm.next_out = (Bytef *)&dataIn[dataIn.size() - 4096];
				retVal = inflate(&strm, Z_NO_FLUSH);
			} while(retVal == Z_OK);
			inflateEnd(&strm);
		} else if(!memcmp(magic, "PLTE", 4))
		{
			// Palette for <= 8-bit images
			palette.resize(256);
			size_t numEntries = std::min<size_t>(256u, chunk.GetLength() / 3u);
			for(size_t i = 0; i < numEntries; i++)
			{
				uint8_t p[3];
				chunk.ReadArray(p);
				palette[i] = RawGDIDIB::Pixel(p[0], p[1], p[2], 255);
			}
		}
	}

	// LUT for translating the color type into a number of color samples
	const uint32_t sampleTable[] =
	{
		1,	// 0: Grayscale
		0,
		3,	// 2: RGB
		1,	// 3: Palette bitmap
		2,	// 4: Grayscale + Alpha
		0,
		4	// 6: RGBA
	};
	const uint32_t bitsPerPixel = colorType < CountOf(sampleTable) ? sampleTable[colorType] * bitDepth : 0;

	if(!width || !height || !bitsPerPixel
		|| (colorType != 2  && colorType != 3 && colorType != 6) || bitDepth != 8	// Only RGB(A) and 8-bit palette PNGs for now.
		|| compressionMethod || interlaceMethod
		|| (colorType == 3 && palette.empty())
		|| dataIn.size() < (bitsPerPixel * width * height) / 8 + height)			// Enough data present?
	{
		throw bad_image();
	}

	std::unique_ptr<RawGDIDIB> bitmap = mpt::make_unique<RawGDIDIB>(width, height);

	RawGDIDIB::Pixel *pixelOut = bitmap->Pixels().data();
	uint32_t x = 0, y = 0;
	size_t offset = 0;
	while(y < height)
	{
		if(x == 0)
		{
			filterMethod = dataIn[offset++];
			ASSERT(!filterMethod);
		}

		if(colorType == 6)
		{
			// RGBA
			pixelOut->r = dataIn[offset++];
			pixelOut->g = dataIn[offset++];
			pixelOut->b = dataIn[offset++];
			pixelOut->a = dataIn[offset++];
		} else if(colorType == 2)
		{
			// RGB
			pixelOut->r = dataIn[offset++];
			pixelOut->g = dataIn[offset++];
			pixelOut->b = dataIn[offset++];
			pixelOut->a = 255;
		} else if(colorType == 3)
		{
			// Palette
			*pixelOut = palette[dataIn[offset++]];
		}
		pixelOut++;
		x++;

		if(x == width)
		{
			y++;
			x = 0;
		}
	}

	return bitmap;
}

#endif // !MPT_WITH_GDIPLUS


#ifdef MPT_WITH_GDIPLUS


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


#endif // MPT_WITH_GDIPLUS


RawGDIDIB::RawGDIDIB(uint32 width, uint32 height)
	: width(width)
	, height(height)
	, pixels(width * height)
{
	MPT_ASSERT(width > 0);
	MPT_ASSERT(height > 0);
}


#ifdef MPT_WITH_GDIPLUS


namespace GDIP
{


static CComPtr<IStream> GetStream(mpt::const_byte_span data)
{
	CComPtr<IStream> stream;
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
	stream.Attach(SHCreateMemStream(data.data(), mpt::saturate_cast<UINT>(data.size())));
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


} // namespace GDPI


#endif // MPT_WITH_GDIPLUS


#ifdef MPT_WITH_GDIPLUS

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
			*dst = GDIP::TORawGDIDIB(*src);
			src++;
			dst++;
		}
	}
	bitmap.UnlockBits(&bitmapData);
	return result;
}

#endif // MPT_WITH_GDIPLUS


std::unique_ptr<RawGDIDIB> LoadPixelImage(mpt::const_byte_span file)
{
	#ifdef MPT_WITH_GDIPLUS
		return ToRawGDIDIB(*GDIP::LoadPixelImage(file));
	#else // !MPT_WITH_GDIPLUS
		return ReadPNG(file);
	#endif // MPT_WITH_GDIPLUS
}


std::unique_ptr<RawGDIDIB> LoadPixelImage(FileReader file)
{
	#ifdef MPT_WITH_GDIPLUS
		return ToRawGDIDIB(*GDIP::LoadPixelImage(file));
	#else // !MPT_WITH_GDIPLUS
		return ReadPNG(file);
	#endif // MPT_WITH_GDIPLUS
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


#ifdef MPT_WITH_GDIPLUS

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

#endif // MPT_WITH_GDIPLUS



bool LoadCompatibleBitmapFromPixelImage(CBitmap &dst, CDC &dc, mpt::const_byte_span file)
{
	try
	{
		#ifdef MPT_WITH_GDIPLUS
			std::unique_ptr<Gdiplus::Bitmap> pBitmap = GDIP::LoadPixelImage(file);
		#else // !MPT_WITH_GDIPLUS
			std::unique_ptr<RawGDIDIB> pBitmap = ReadPNG(file);
		#endif // MPT_WITH_GDIPLUS
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
		#ifdef MPT_WITH_GDIPLUS
			std::unique_ptr<Gdiplus::Bitmap> pBitmap = GDIP::LoadPixelImage(file);
		#else // !MPT_WITH_GDIPLUS
			std::unique_ptr<RawGDIDIB> pBitmap = ReadPNG(file);
		#endif // MPT_WITH_GDIPLUS
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
