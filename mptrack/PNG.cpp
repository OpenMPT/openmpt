/*
 * PNG.cpp
 * -------
 * Purpose: Extremely minimalistic PNG loader (only for internal data resources)
 * Notes  : Currently implemented: 8-bit, 24-bit and 32-bit images (8 bits per component), no filters, no interlaced pictures.
 *          Paletted pictures are automatically depalettized.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MPTrackUtil.h"
#include "PNG.h"
#include "../common/FileReader.h"
#if defined(MPT_WITH_ZLIB)
#include <zlib.h>
#elif defined(MPT_WITH_MINIZ)
#define MINIZ_HEADER_FILE_ONLY
#include "miniz/miniz.c"
#endif


OPENMPT_NAMESPACE_BEGIN


PNG::Bitmap *PNG::ReadPNG(FileReader &file)
//-----------------------------------------
{
	file.Rewind();
	if(!file.ReadMagic("\211PNG\r\n\032\n"))
	{
		return nullptr;
	}

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t bitDepth;
	uint8_t colorType;
	uint8_t compressionMethod;
	uint8_t filterMethod;
	uint8_t interlaceMethod;

	std::vector<uint8_t> dataIn;
	std::vector<Pixel> palette;

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
			strm.next_in = (Bytef *)(chunk.GetRawData());
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
				palette[i] = Pixel(p[0], p[1], p[2], 255);
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
		return nullptr;
	}

	Bitmap *bitmap = new (std::nothrow) Bitmap(width, height);

	Pixel *pixelOut = bitmap->GetPixels();
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


PNG::Bitmap *PNG::ReadPNG(const TCHAR *resource)
//----------------------------------------------
{
	const char *pData = nullptr;
	HGLOBAL hglob = nullptr;
	size_t nSize = 0;
	if(LoadResource(resource, TEXT("PNG"), pData, nSize, hglob) == nullptr)
	{
		return nullptr;
	}

	FileReader file(pData, nSize);
	return ReadPNG(file);
}


// Create a DIB for the current device from our PNG.
bool PNG::Bitmap::ToDIB(CBitmap &bitmap, CDC *dc) const
//-----------------------------------------------------
{
	BITMAPINFOHEADER bi;
	MemsetZero(bi);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -(int32_t)height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = width * height * 4;

	if(dc == nullptr) dc = CDC::FromHandle(GetDC(NULL));
	return bitmap.CreateCompatibleBitmap(dc, width, height)
		&& SetDIBits(dc->GetSafeHdc(), bitmap, 0, height, GetPixels(), reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);
}

OPENMPT_NAMESPACE_END
