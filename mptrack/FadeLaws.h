/*
 * FadeLaws.h
 * ----------
 * Purpose: Various fade law implementations for sample and pattern fading / interpolation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <cmath>
#include "../common/misc_util.h"

OPENMPT_NAMESPACE_BEGIN

namespace Fade
{
	enum Law
	{
		kLinear,
		kPow,
		kSqrt,
		kLog,
		kQuarterSine,
		kHalfSine,
	};

	typedef int32 (*Func) (int32 val, int64 i, int64 max);

	static int32 LinearFunc(int32 val, int64 i, int64 max)
		{ return static_cast<int32>((val * i) / max); }
	static int32 PowFunc(int32 val, int64 i, int64 max)
		{ return static_cast<int32>((val * i * i) / (max * max)); }
	static int32 SqrtFunc(int32 val, int64 i, int64 max)
		{ return Util::Round<int32>(val * std::sqrt(static_cast<double>(i) / max)); }
	static int32 LogFunc(int32 val, int64 i, int64 max)
		{ return Util::Round<int32>(val * std::log10(1.0 + (static_cast<double>(i) / max) * 99.0) * 0.5); }
	static int32 QuarterSineFunc(int32 val, int64 i, int64 max)
		{ return Util::Round<int32>(val * std::sin(M_PI_2 * static_cast<double>(i) / max)); }
	static int32 HalfSineFunc(int32 val, int64 i, int64 max)
		{ return Util::Round<int32>(val * (1.0 + std::cos(M_PI + M_PI * static_cast<double>(i) / max)) * 0.5); }

	static Func GetFadeFunc(Law fadeLaw)
	{
		switch(fadeLaw)
		{
		default:
		case Fade::kLinear:      return LinearFunc;
		case Fade::kPow:         return PowFunc;
		case Fade::kSqrt:        return SqrtFunc;
		case Fade::kLog:         return LogFunc;
		case Fade::kQuarterSine: return QuarterSineFunc;
		case Fade::kHalfSine:    return HalfSineFunc;
		}
	}
}

OPENMPT_NAMESPACE_END
