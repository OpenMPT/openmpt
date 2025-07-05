/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/macros.hpp"
#include "openmpt/soundbase/MixSample.hpp"
#include "openmpt/soundbase/SampleConvert.hpp"
#include "openmpt/soundbase/SampleConvertFixedPoint.hpp"


OPENMPT_NAMESPACE_BEGIN


template <typename Tdst, typename Tsrc>
struct ConvertMixSample;

template <>
struct ConvertMixSample<MixSampleInt, MixSampleInt>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE MixSampleInt conv(MixSampleInt src)
	{
		return src;
	}
};

template <>
struct ConvertMixSample<MixSampleFloat, MixSampleFloat>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE MixSampleFloat conv(MixSampleFloat src)
	{
		return src;
	}
};

template <typename Tsrc>
struct ConvertMixSample<MixSampleInt, Tsrc>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE MixSampleInt conv(Tsrc src)
	{
		return SC::ConvertToFixedPoint<MixSampleInt, Tsrc, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};

template <typename Tdst>
struct ConvertMixSample<Tdst, MixSampleInt>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE Tdst conv(MixSampleInt src)
	{
		return SC::ConvertFixedPoint<Tdst, MixSampleInt, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};

template <typename Tsrc>
struct ConvertMixSample<MixSampleFloat, Tsrc>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE MixSampleFloat conv(Tsrc src)
	{
		return SC::Convert<MixSampleFloat, Tsrc>{}(src);
	}
};

template <typename Tdst>
struct ConvertMixSample<Tdst, MixSampleFloat>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE Tdst conv(MixSampleFloat src)
	{
		return SC::Convert<Tdst, MixSampleFloat>{}(src);
	}
};

template <>
struct ConvertMixSample<MixSampleInt, MixSampleFloat>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE MixSampleInt conv(MixSampleFloat src)
	{
		return SC::ConvertToFixedPoint<MixSampleInt, MixSampleFloat, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};

template <>
struct ConvertMixSample<MixSampleFloat, MixSampleInt>
{
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE MixSampleFloat conv(MixSampleInt src)
	{
		return SC::ConvertFixedPoint<MixSampleFloat, MixSampleInt, MixSampleIntTraits::mix_fractional_bits>{}(src);
	}
};


template <typename Tdst, typename Tsrc>
MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE Tdst mix_sample_cast(Tsrc src)
{
	return ConvertMixSample<Tdst, Tsrc>{}.conv(src);
}


OPENMPT_NAMESPACE_END
