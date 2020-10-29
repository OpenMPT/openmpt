/*
 * mptColor.cpp
 * ------------
 * Purpose: Color space conversion and other color-related code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptColor.h"


OPENMPT_NAMESPACE_BEGIN


namespace mpt::Color
{

uint8 GetLuma(uint8 r, uint8 g, uint8 b) noexcept
{
	return mpt::saturate_cast<uint8>(r * 0.299f + g * 0.587f + b * 0.114f);
}


HSV RGB::ToHSV() const noexcept
{
	const auto min = std::min({r, g, b});
	const auto max = std::max({r, g, b});
	const auto delta = max - min;

	HSV hsv;
	hsv.v = max;
	if(delta < 0.00001f)
	{
		hsv.s = 0;
		hsv.h = 0;
		return hsv;
	}
	if(max > 0.0f)
	{
		hsv.s = (delta / max);
	} else
	{
		// black
		hsv.s = 0.0f;
		hsv.h = 0.0f;
		return hsv;
	}
	if(r >= max)
		hsv.h = (g - b) / delta;
	else if(g >= max)
		hsv.h = 2.0f + (b - r) / delta;
	else
		hsv.h = 4.0f + (r - g) / delta;

	if(hsv.h < 0.0f)
		hsv.h += 6.0f;

	hsv.h *= 60.0f;

	return hsv;
}


RGB HSV::ToRGB() const noexcept
{
	// Optimization for greyscale
	if(s <= 0.0f)
		return {v, v, v};

	const float hh = h / 60.0f;
	const int region = static_cast<int>(hh);
	const float fract = hh - region;
	const float p = v * (1.0f - s);
	const float q = v * (1.0f - (s * fract));
	const float t = v * (1.0f - (s * (1.0f - fract)));

	switch(region % 6)
	{
	default:
	case 0: return {v, t, p};
	case 1: return {q, v, p};
	case 2: return {p, v, t};
	case 3: return {p, q, v};
	case 4: return {t, p, v};
	case 5: return {v, p, q};
	}
}


}  // namespace mpt::Color

OPENMPT_NAMESPACE_END
