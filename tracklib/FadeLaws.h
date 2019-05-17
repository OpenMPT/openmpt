/*
 * FadeLaws.h
 * ----------
 * Purpose: Various fade law implementations for sample and pattern fading / interpolation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include <cmath>

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

	// Maps fade curve position in [0,1] to value in [0,1]
	typedef double (*Func) (double pos);

	static double LinearFunc(double pos)
		{ return pos; }
	static double PowFunc(double pos)
		{ return pos * pos; }
	static double SqrtFunc(double pos)
		{ return std::sqrt(pos); }
	static double LogFunc(double pos)
		{ return std::log10(1.0 + pos * 99.0) * 0.5; }
	static double QuarterSineFunc(double pos)
		{ return std::sin(M_PI_2 * pos); }
	static double HalfSineFunc(double pos)
		{ return (1.0 + std::cos(M_PI + M_PI * pos)) * 0.5; }

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
