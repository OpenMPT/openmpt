/*
 * PNG.cpp
 * -------
 * Purpose: Extremely minimalistic PNG loader (only for internal data resources)
 * Notes  : Currenlty implemented: 8-bit, 24-bit and 32-bit images (8 bits per component), no filters, no interlaced pictures.
 *          Paletted pictures are automatically depalettized.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "MPTrackUtil.h"
#include "../soundlib/FileReader.h"
#if !defined(NO_ZLIB)
#include <zlib.h>
#elif !defined(NO_MINIZ)
#define MINIZ_HEADER_FILE_ONLY
#include "miniz/miniz.c"
#endif


CBitmap *ReadPNG(FileReader &file)
//--------------------------------
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
	std::vector<RGBQUAD> palette;

	while(file.AreBytesLeft())
	{
		uint32_t chunkLength = file.ReadUint32BE();
		char magic[4];
		file.ReadArray(magic);
		FileReader chunk = file.ReadChunk(chunkLength);
		file.Skip(4);
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
				RGBQUAD col;
				col.rgbRed = chunk.ReadUint8();
				col.rgbGreen = chunk.ReadUint8();
				col.rgbBlue = chunk.ReadUint8();
				col.rgbReserved = 255;
				palette[i] = col;
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
		|| dataIn.size() < (bitsPerPixel * width * height) / 8 + height)			// Enough data present?
	{
		return nullptr;
	}

	CBitmap *bitmap = new CBitmap;
	if(!bitmap->CreateBitmap(width, height, 1, 32, nullptr))
	{
		delete bitmap;
		return nullptr;
	}

	const uint32_t imageSize = width * height * sizeof(RGBQUAD);
	std::vector<RGBQUAD> dataOut(width * height);
	bitmap->GetBitmapBits(imageSize, &dataOut[0]);

	FileReader imageData(&dataIn[0], dataIn.size());

	RGBQUAD *pixelOut = &dataOut[0];
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
			struct PNGPixel32
			{
				uint8_t r, g, b, a;
			};

			const PNGPixel32 &inPixel = reinterpret_cast<const PNGPixel32 &>(dataIn[offset]);
			pixelOut->rgbRed = inPixel.r;
			pixelOut->rgbGreen = inPixel.g;
			pixelOut->rgbBlue = inPixel.b;
			pixelOut->rgbReserved= inPixel.a;
			offset += 4;
		} else if(colorType == 2)
		{
			// RGB
			struct PNGPixel24
			{
				uint8_t r, g, b;
			};

			const PNGPixel24 &inPixel = reinterpret_cast<const PNGPixel24 &>(dataIn[offset]);
			pixelOut->rgbRed = inPixel.r;
			pixelOut->rgbGreen = inPixel.g;
			pixelOut->rgbBlue = inPixel.b;
			pixelOut->rgbReserved= 255;
			offset += 3;
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

	bitmap->SetBitmapBits(imageSize, &dataOut[0]);
	return bitmap;
}


CBitmap *ReadPNG(const TCHAR *resource)
//-------------------------------------
{
	const char *pData = nullptr;
	HGLOBAL hglob = nullptr;
	size_t nSize = 0;
	if(LoadResource(resource, TEXT("PNG"), pData, nSize, hglob) == nullptr)
	{
		return nullptr;
	}

	FileReader file(pData, nSize);

	CBitmap *bitmap = ReadPNG(file);

	FreeResource(hglob);
	return bitmap;
}
