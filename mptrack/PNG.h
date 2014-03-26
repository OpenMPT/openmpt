/*
 * PNG.h
 * -----
 * Purpose: Extremely minimalistic PNG loader.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

class FileReader;

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
		std::vector<Pixel> pixels;

		Bitmap(uint32_t width, uint32_t height) : width(width), height(height), pixels(width * height) { }

		Pixel *GetPixels() { ASSERT(!pixels.empty()); return &pixels[0]; }
		const Pixel *GetPixels() const { ASSERT(!pixels.empty()); return &pixels[0]; }
		uint32_t GetNumPixels() const { return width * height; }

		// Create a DIB for the current device from our PNG.
		bool ToDIB(CBitmap &bitmap, CDC *dc = nullptr) const;
	};

	Bitmap *ReadPNG(FileReader &file);
	Bitmap *ReadPNG(const TCHAR *resource);
}
