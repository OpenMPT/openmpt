/*
 * mptColor.h
 * ----------
 * Purpose: Color space conversion and other color-related code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


OPENMPT_NAMESPACE_BEGIN


namespace mpt::Color
{

uint8 GetLuma(uint8 r, uint8 g, uint8 b) noexcept;

struct HSV;

struct RGB
{
	float r;  // 0...1
	float g;  // 0...1
	float b;  // 0...1

	HSV ToHSV() const noexcept;
};

struct HSV
{
	float h;  // angle in degrees
	float s;  // 0...1
	float v;  // 0...1

	RGB ToRGB() const noexcept;
};

}  // namespace mpt::Color


OPENMPT_NAMESPACE_END
