/*
 * PNG.h
 * -----
 * Purpose: Extremely minimalistic PNG loader.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/FileReaderFwd.h"

OPENMPT_NAMESPACE_BEGIN

namespace PNG
{
	struct Pixel
	{
		uint8_t b, g, r, a;	// Component order must be compatible with Microsoft DIBs!
		Pixel(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 0) : b(b), g(g), r(r), a(a) { }
	};

	struct Bitmap
	{
		const uint32_t width, height;
		Pixel *pixels;

		Bitmap(uint32_t width, uint32_t height) : width(width), height(height), pixels(new Pixel[width * height]) { }
		~Bitmap() { delete[] pixels; }

		Pixel *GetPixels() { ASSERT(width && height && pixels); return pixels; }
		const Pixel *GetPixels() const { ASSERT(width && height && pixels); return pixels; }
		uint32_t GetNumPixels() const { return width * height; }

		// Create a DIB from our PNG.
		bool ToDIB(CBitmap &bitmap, CDC *dc) const;
	};

	Bitmap *ReadPNG(FileReader &file);
	Bitmap *ReadPNG(const TCHAR *resource);
}

OPENMPT_NAMESPACE_END
