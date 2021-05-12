/*
 * MixSampleConvert.h
 * ------------------
 * Purpose: Basic audio sample types.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "MixSample.h"
#include "SampleFormatConverters.h"


OPENMPT_NAMESPACE_BEGIN


template <typename Tdst, typename Tsrc>
struct ConvertMixSample;

template <typename Tsrc>
struct ConvertMixSample<MixSampleInt, Tsrc>
{
	MPT_FORCEINLINE MixSampleInt conv(Tsrc src)
	{
		return SC::ConvertToFixedPoint<MixSampleInt, Tsrc, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};

template <typename Tdst>
struct ConvertMixSample<Tdst, MixSampleInt>
{
	MPT_FORCEINLINE Tdst conv(MixSampleInt src)
	{
		return SC::ConvertFixedPoint<Tdst, MixSampleInt, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};

template <typename Tsrc>
struct ConvertMixSample<MixSampleFloat, Tsrc>
{
	MPT_FORCEINLINE MixSampleFloat conv(Tsrc src)
	{
		return SC::Convert<MixSampleFloat, Tsrc>{}(src);
	}
};

template <typename Tdst>
struct ConvertMixSample<Tdst, MixSampleFloat>
{
	MPT_FORCEINLINE Tdst conv(MixSampleFloat src)
	{
		return SC::Convert<Tdst, MixSampleFloat>{}(src);
	}
};

template <>
struct ConvertMixSample<MixSampleInt, MixSampleFloat>
{
	MPT_FORCEINLINE MixSampleInt conv(MixSampleFloat src)
	{
		return SC::ConvertToFixedPoint<MixSampleInt, MixSampleFloat, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};

template <>
struct ConvertMixSample<MixSampleFloat, MixSampleInt>
{
	MPT_FORCEINLINE MixSampleFloat conv(MixSampleInt src)
	{
		return SC::ConvertFixedPoint<MixSampleFloat, MixSampleInt, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};


template <typename Tdst, typename Tsrc>
MPT_FORCEINLINE Tdst mix_sample_cast(Tsrc src)
{
	return ConvertMixSample<Tdst, Tsrc>{}.conv(src);
}


OPENMPT_NAMESPACE_END
