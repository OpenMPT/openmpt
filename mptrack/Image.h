/*
 * Image.h
 * -------
 * Purpose: Bitmap and Vector image file handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../common/FileReaderFwd.h"

#ifdef MPT_WITH_GDIPLUS
namespace Gdiplus {
#include <gdipluspixelformats.h>
class Image;
class Bitmap;
class Metafile;
}
#endif // MPT_WITH_GDIPLUS


OPENMPT_NAMESPACE_BEGIN


class bad_image : public std::runtime_error { public: bad_image() : std::runtime_error("") { } };


class RawGDIDIB
{
public:
	struct Pixel
	{
		// Component order must be compatible with Microsoft DIBs!
		uint8 b;
		uint8 g;
		uint8 r;
		uint8 a;
		MPT_CONSTEXPR11_FUN Pixel() noexcept : b(0), g(0), r(0), a(0) {}
		MPT_CONSTEXPR11_FUN Pixel(uint8 r, uint8 g, uint8 b, uint8 a) noexcept : b(b), g(g), r(r), a(a) {}
	};
private:
	uint32 width;
	uint32 height;
	std::vector<Pixel> pixels;
public:
	RawGDIDIB(uint32 width, uint32 height);
public:
	MPT_CONSTEXPR11_FUN uint32 Width() const noexcept { return width; }
	MPT_CONSTEXPR11_FUN uint32 Height() const noexcept { return height; }
	MPT_FORCEINLINE Pixel & operator()(uint32 x, uint32 y) noexcept { return pixels[y*width + x]; }
	MPT_FORCEINLINE const Pixel & operator()(uint32 x, uint32 y) const noexcept { return pixels[y*width + x]; }
	std::vector<Pixel> &Pixels() { return pixels; }
	const std::vector<Pixel> &Pixels() const { return pixels; }
};


#ifdef MPT_WITH_GDIPLUS


class GdiplusRAII
{
private:
	ULONG_PTR gdiplusToken;
public:
	GdiplusRAII();
	~GdiplusRAII();
};


namespace GDIP
{

	std::unique_ptr<Gdiplus::Bitmap> LoadPixelImage(mpt::const_byte_span file);
	std::unique_ptr<Gdiplus::Bitmap> LoadPixelImage(FileReader file);

	std::unique_ptr<Gdiplus::Metafile> LoadVectorImage(mpt::const_byte_span file);
	std::unique_ptr<Gdiplus::Metafile> LoadVectorImage(FileReader file);

	typedef Gdiplus::ARGB Pixel;

	template <typename TBitmapData>
	inline Pixel * GetScanline(const TBitmapData &bitmapData, std::size_t y) noexcept
	{
		if(bitmapData.Stride >= 0)
		{
			return reinterpret_cast<Pixel*>(mpt::void_cast<void*>(mpt::void_cast<mpt::byte*>(bitmapData.Scan0) + y * bitmapData.Stride));
		} else
		{
			return reinterpret_cast<Pixel*>(mpt::void_cast<void*>(mpt::void_cast<mpt::byte*>(bitmapData.Scan0) + (bitmapData.Height - 1 - y) * (-bitmapData.Stride)));
		}
	}

	MPT_CONSTEXPR11_FUN Pixel AsPixel(uint8 r, uint8 g, uint8 b, uint8 a) noexcept
	{
		return Pixel(0)
			| (static_cast<Pixel>(r) << RED_SHIFT)
			| (static_cast<Pixel>(g) << GREEN_SHIFT)
			| (static_cast<Pixel>(b) << BLUE_SHIFT)
			| (static_cast<Pixel>(a) << ALPHA_SHIFT)
			;
	}

	MPT_CONSTEXPR11_FUN uint8 R(Pixel p) noexcept { return static_cast<uint8>(p >> RED_SHIFT); }
	MPT_CONSTEXPR11_FUN uint8 G(Pixel p) noexcept { return static_cast<uint8>(p >> GREEN_SHIFT); }
	MPT_CONSTEXPR11_FUN uint8 B(Pixel p) noexcept { return static_cast<uint8>(p >> BLUE_SHIFT); }
	MPT_CONSTEXPR11_FUN uint8 A(Pixel p) noexcept { return static_cast<uint8>(p >> ALPHA_SHIFT); }

	MPT_CONSTEXPR11_FUN RawGDIDIB::Pixel TORawGDIDIB(Pixel p) noexcept
	{
		return RawGDIDIB::Pixel(GDIP::R(p), GDIP::G(p), GDIP::B(p), GDIP::A(p));
	}

} // namespace GDIP


#endif // MPT_WITH_GDIPLUS


#ifdef MPT_WITH_GDIPLUS
std::unique_ptr<RawGDIDIB> ToRawGDIDIB(Gdiplus::Bitmap &bitmap);
#endif // MPT_WITH_GDIPLUS

bool CopyToCompatibleBitmap(CBitmap &dst, CDC &dc, const RawGDIDIB &src);

#ifdef MPT_WITH_GDIPLUS
bool CopyToCompatibleBitmap(CBitmap &dst, CDC &dc, Gdiplus::Image &src);
#endif // MPT_WITH_GDIPLUS

std::unique_ptr<RawGDIDIB> LoadPixelImage(mpt::const_byte_span file);
std::unique_ptr<RawGDIDIB> LoadPixelImage(FileReader file);

bool LoadCompatibleBitmapFromPixelImage(CBitmap &dst, CDC &dc, mpt::const_byte_span file);
bool LoadCompatibleBitmapFromPixelImage(CBitmap &dst, CDC &dc, FileReader file);


OPENMPT_NAMESPACE_END
