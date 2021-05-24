/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/arithmetic_shift.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/saturate_cast.hpp"
#include "openmpt/base/Int24.hpp"
#include "openmpt/base/Types.hpp"
#include "openmpt/soundbase/SampleConvert.hpp"

#include <algorithm>
#include <limits>


OPENMPT_NAMESPACE_BEGIN


namespace SC
{  // SC = _S_ample_C_onversion


template <typename Tdst, typename Tsrc, int fractionalBits>
struct ConvertFixedPoint;

template <int fractionalBits>
struct ConvertFixedPoint<uint8, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef uint8 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		val = mpt::rshift_signed((val + (1 << (shiftBits - 1))), shiftBits);  // round
		if(val < std::numeric_limits<int8>::min()) val = std::numeric_limits<int8>::min();
		if(val > std::numeric_limits<int8>::max()) val = std::numeric_limits<int8>::max();
		return static_cast<uint8>(val + 0x80);  // unsigned
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int8, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int8 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		val = mpt::rshift_signed((val + (1 << (shiftBits - 1))), shiftBits);  // round
		if(val < std::numeric_limits<int8>::min()) val = std::numeric_limits<int8>::min();
		if(val > std::numeric_limits<int8>::max()) val = std::numeric_limits<int8>::max();
		return static_cast<int8>(val);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int16, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int16 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		val = mpt::rshift_signed((val + (1 << (shiftBits - 1))), shiftBits);  // round
		if(val < std::numeric_limits<int16>::min()) val = std::numeric_limits<int16>::min();
		if(val > std::numeric_limits<int16>::max()) val = std::numeric_limits<int16>::max();
		return static_cast<int16>(val);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int24, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int24 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		val = mpt::rshift_signed((val + (1 << (shiftBits - 1))), shiftBits);  // round
		if(val < std::numeric_limits<int24>::min()) val = std::numeric_limits<int24>::min();
		if(val > std::numeric_limits<int24>::max()) val = std::numeric_limits<int24>::max();
		return static_cast<int24>(val);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int32, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		return static_cast<int32>(std::clamp(val, static_cast<int32>(-((1 << fractionalBits) - 1)), static_cast<int32>(1 << fractionalBits) - 1)) << (sizeof(input_t) * 8 - 1 - fractionalBits);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<float32, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef float32 output_t;
	const float factor;
	MPT_FORCEINLINE ConvertFixedPoint()
		: factor(1.0f / static_cast<float>(1 << fractionalBits))
	{
		return;
	}
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		return val * factor;
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<float64, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef float64 output_t;
	const double factor;
	MPT_FORCEINLINE ConvertFixedPoint()
		: factor(1.0 / static_cast<double>(1 << fractionalBits))
	{
		return;
	}
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		return val * factor;
	}
};


template <typename Tdst, typename Tsrc, int fractionalBits>
struct ConvertToFixedPoint;

template <int fractionalBits>
struct ConvertToFixedPoint<int32, uint8, fractionalBits>
{
	typedef uint8 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		return mpt::lshift_signed(static_cast<output_t>(static_cast<int>(val) - 0x80), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int8, fractionalBits>
{
	typedef int8 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		return mpt::lshift_signed(static_cast<output_t>(val), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int16, fractionalBits>
{
	typedef int16 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		return mpt::lshift_signed(static_cast<output_t>(val), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int24, fractionalBits>
{
	typedef int24 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t) * 8 - 1);
		static_assert(shiftBits >= 1);
		return mpt::lshift_signed(static_cast<output_t>(val), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t) * 8 - 1);
		return mpt::rshift_signed(static_cast<output_t>(val), (sizeof(input_t) * 8 - 1 - fractionalBits));
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, float32, fractionalBits>
{
	typedef float32 input_t;
	typedef int32 output_t;
	const float factor;
	MPT_FORCEINLINE ConvertToFixedPoint()
		: factor(static_cast<float>(1 << fractionalBits))
	{
		return;
	}
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		return mpt::saturate_cast<output_t>(SC::fastround(val * factor));
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, float64, fractionalBits>
{
	typedef float64 input_t;
	typedef int32 output_t;
	const double factor;
	MPT_FORCEINLINE ConvertToFixedPoint()
		: factor(static_cast<double>(1 << fractionalBits))
	{
		return;
	}
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t) * 8 - 1);
		return mpt::saturate_cast<output_t>(SC::fastround(val * factor));
	}
};


}  // namespace SC


OPENMPT_NAMESPACE_END
