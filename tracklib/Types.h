/*
 * Types.h
 * -------
 * Purpose:  Type definitions commonly used in tracklib / tracker code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/numbers.hpp"
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
	using Func = double (*)(double pos);

	inline double LinearFunc(double pos)
		{ return pos; }
	inline double PowFunc(double pos)
		{ return pos * pos; }
	inline double SqrtFunc(double pos)
		{ return std::sqrt(pos); }
	inline double LogFunc(double pos)
		{ return std::log10(1.0 + pos * 99.0) * 0.5; }
	inline double QuarterSineFunc(double pos)
		{ return std::sin((0.5 * mpt::numbers::pi) * pos); }
	inline double HalfSineFunc(double pos)
		{ return (1.0 + std::cos(mpt::numbers::pi + mpt::numbers::pi * pos)) * 0.5; }

	inline Func GetFadeFunc(Law fadeLaw)
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


enum class AmplificationUnit : uint8
{
	Percent = 0,
	Decibels,
};


enum class SampleLengthUnit : uint8
{
	Samples = 0,
	Milliseconds,
};


enum class SampleGridMode : uint8
{
	NoGrid = 0,
	DivideIntoSegments,
	DivideEveryN,
};


enum class SampleChannelSelection : uint8
{
	None = 0,
	Left = 1,
	Right = 2,
	Both = 1 | 2,
};

namespace SampleEdit
{
	static constexpr double SILENCE_DB = -96.0;

	static constexpr uint8 ChannelSelectionToMask(SampleChannelSelection channelSel) noexcept
	{
		return static_cast<uint8>(channelSel);
	}
}  // namespace SampleEdit


OPENMPT_NAMESPACE_END
