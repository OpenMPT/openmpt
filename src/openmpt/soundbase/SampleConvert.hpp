/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/arithmetic_shift.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/math.hpp"
#include "mpt/base/saturate_cast.hpp"
#include "openmpt/base/Int24.hpp"
#include "openmpt/base/Types.hpp"
#include "openmpt/soundbase/SampleConvert.hpp"

#include <algorithm>
#include <limits>
#include <type_traits>

#include <cmath>



OPENMPT_NAMESPACE_BEGIN



namespace SC
{  // SC = _S_ample_C_onversion



#if MPT_COMPILER_MSVC
template <typename Tfloat>
MPT_FORCEINLINE Tfloat fastround(Tfloat x)
{
	static_assert(std::is_floating_point<Tfloat>::value);
	return std::floor(x + static_cast<Tfloat>(0.5));
}
#else
template <typename Tfloat>
MPT_FORCEINLINE Tfloat fastround(Tfloat x)
{
	static_assert(std::is_floating_point<Tfloat>::value);
	return mpt::round(x);
}
#endif



// Shift input_t down by shift and saturate to output_t.
template <typename Tdst, typename Tsrc, int shift>
struct ConvertShift
{
	typedef Tsrc input_t;
	typedef Tdst output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return mpt::saturate_cast<output_t>(mpt::rshift_signed(val, shift));
	}
};



// Shift input_t up by shift and saturate to output_t.
template <typename Tdst, typename Tsrc, int shift>
struct ConvertShiftUp
{
	typedef Tsrc input_t;
	typedef Tdst output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return mpt::saturate_cast<output_t>(mpt::lshift_signed(val, shift));
	}
};




// Every sample conversion functor has to typedef its input_t and output_t.
// The input_t argument is taken by value because we only deal with per-single-sample conversions here.


// straight forward type conversions, clamping when converting from floating point.
template <typename Tdst, typename Tsrc>
struct Convert;

template <typename Tid>
struct Convert<Tid, Tid>
{
	typedef Tid input_t;
	typedef Tid output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val;
	}
};

template <>
struct Convert<uint8, int8>
{
	typedef int8 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<uint8>(val + 0x80);
	}
};

template <>
struct Convert<uint8, int16>
{
	typedef int16 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(mpt::rshift_signed(val, 8)) + 0x80);
	}
};

template <>
struct Convert<uint8, int24>
{
	typedef int24 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(mpt::rshift_signed(static_cast<int>(val), 16)) + 0x80);
	}
};

template <>
struct Convert<uint8, int32>
{
	typedef int32 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(mpt::rshift_signed(val, 24)) + 0x80);
	}
};

template <>
struct Convert<uint8, int64>
{
	typedef int64 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(mpt::rshift_signed(val, 56)) + 0x80);
	}
};

template <>
struct Convert<uint8, float32>
{
	typedef float32 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0f, 1.0f);
		val *= 128.0f;
		return static_cast<uint8>(mpt::saturate_cast<int8>(static_cast<int>(SC::fastround(val))) + 0x80);
	}
};

template <>
struct Convert<uint8, double>
{
	typedef double input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0, 1.0);
		val *= 128.0;
		return static_cast<uint8>(mpt::saturate_cast<int8>(static_cast<int>(SC::fastround(val))) + 0x80);
	}
};

template <>
struct Convert<int8, uint8>
{
	typedef uint8 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int8>(static_cast<int>(val) - 0x80);
	}
};

template <>
struct Convert<int8, int16>
{
	typedef int16 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int8>(mpt::rshift_signed(val, 8));
	}
};

template <>
struct Convert<int8, int24>
{
	typedef int24 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int8>(mpt::rshift_signed(static_cast<int>(val), 16));
	}
};

template <>
struct Convert<int8, int32>
{
	typedef int32 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int8>(mpt::rshift_signed(val, 24));
	}
};

template <>
struct Convert<int8, int64>
{
	typedef int64 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int8>(mpt::rshift_signed(val, 56));
	}
};

template <>
struct Convert<int8, float32>
{
	typedef float32 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0f, 1.0f);
		val *= 128.0f;
		return mpt::saturate_cast<int8>(static_cast<int>(SC::fastround(val)));
	}
};

template <>
struct Convert<int8, double>
{
	typedef double input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0, 1.0);
		val *= 128.0;
		return mpt::saturate_cast<int8>(static_cast<int>(SC::fastround(val)));
	}
};

template <>
struct Convert<int16, uint8>
{
	typedef uint8 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int16>(mpt::lshift_signed(static_cast<int>(val) - 0x80, 8));
	}
};

template <>
struct Convert<int16, int8>
{
	typedef int8 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int16>(mpt::lshift_signed(val, 8));
	}
};

template <>
struct Convert<int16, int24>
{
	typedef int24 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int16>(mpt::rshift_signed(static_cast<int>(val), 8));
	}
};

template <>
struct Convert<int16, int32>
{
	typedef int32 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int16>(mpt::rshift_signed(val, 16));
	}
};

template <>
struct Convert<int16, int64>
{
	typedef int64 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int16>(mpt::rshift_signed(val, 48));
	}
};

template <>
struct Convert<int16, float32>
{
	typedef float32 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0f, 1.0f);
		val *= 32768.0f;
		return mpt::saturate_cast<int16>(static_cast<int>(SC::fastround(val)));
	}
};

template <>
struct Convert<int16, double>
{
	typedef double input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0, 1.0);
		val *= 32768.0;
		return mpt::saturate_cast<int16>(static_cast<int>(SC::fastround(val)));
	}
};

template <>
struct Convert<int24, uint8>
{
	typedef uint8 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int24>(mpt::lshift_signed(static_cast<int>(val) - 0x80, 16));
	}
};

template <>
struct Convert<int24, int8>
{
	typedef int8 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int24>(mpt::lshift_signed(val, 16));
	}
};

template <>
struct Convert<int24, int16>
{
	typedef int16 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int24>(mpt::lshift_signed(val, 8));
	}
};

template <>
struct Convert<int24, int32>
{
	typedef int32 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int24>(mpt::rshift_signed(val, 8));
	}
};

template <>
struct Convert<int24, int64>
{
	typedef int64 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int24>(mpt::rshift_signed(val, 40));
	}
};

template <>
struct Convert<int24, float32>
{
	typedef float32 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0f, 1.0f);
		val *= 2147483648.0f;
		return static_cast<int24>(mpt::rshift_signed(mpt::saturate_cast<int32>(static_cast<int64>(SC::fastround(val))), 8));
	}
};

template <>
struct Convert<int24, double>
{
	typedef double input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0, 1.0);
		val *= 2147483648.0;
		return static_cast<int24>(mpt::rshift_signed(mpt::saturate_cast<int32>(static_cast<int64>(SC::fastround(val))), 8));
	}
};

template <>
struct Convert<int32, uint8>
{
	typedef uint8 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int32>(mpt::lshift_signed(static_cast<int>(val) - 0x80, 24));
	}
};

template <>
struct Convert<int32, int8>
{
	typedef int8 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int32>(mpt::lshift_signed(val, 24));
	}
};

template <>
struct Convert<int32, int16>
{
	typedef int16 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int32>(mpt::lshift_signed(val, 16));
	}
};

template <>
struct Convert<int32, int24>
{
	typedef int24 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int32>(mpt::lshift_signed(static_cast<int>(val), 8));
	}
};

template <>
struct Convert<int32, int64>
{
	typedef int64 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<int32>(mpt::rshift_signed(val, 32));
	}
};

template <>
struct Convert<int32, float32>
{
	typedef float32 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0f, 1.0f);
		val *= 2147483648.0f;
		return mpt::saturate_cast<int32>(static_cast<int64>(SC::fastround(val)));
	}
};

template <>
struct Convert<int32, double>
{
	typedef double input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0, 1.0);
		val *= 2147483648.0;
		return mpt::saturate_cast<int32>(static_cast<int64>(SC::fastround(val)));
	}
};

template <>
struct Convert<int64, uint8>
{
	typedef uint8 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return mpt::lshift_signed(static_cast<int64>(val) - 0x80, 56);
	}
};

template <>
struct Convert<int64, int8>
{
	typedef int8 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return mpt::lshift_signed(static_cast<int64>(val), 56);
	}
};

template <>
struct Convert<int64, int16>
{
	typedef int16 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return mpt::lshift_signed(static_cast<int64>(val), 48);
	}
};

template <>
struct Convert<int64, int24>
{
	typedef int24 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return mpt::lshift_signed(static_cast<int64>(val), 40);
	}
};

template <>
struct Convert<int64, int32>
{
	typedef int32 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return mpt::lshift_signed(static_cast<int64>(val), 32);
	}
};

template <>
struct Convert<int64, float32>
{
	typedef float32 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0f, 1.0f);
		val *= static_cast<float>(uint64(1) << 63);
		return mpt::saturate_cast<int64>(SC::fastround(val));
	}
};

template <>
struct Convert<int64, double>
{
	typedef double input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		val = std::clamp(val, -1.0, 1.0);
		val *= static_cast<double>(uint64(1) << 63);
		return mpt::saturate_cast<int64>(SC::fastround(val));
	}
};

template <>
struct Convert<float32, uint8>
{
	typedef uint8 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return (static_cast<int>(val) - 0x80) * (1.0f / static_cast<float32>(static_cast<unsigned int>(1) << 7));
	}
};

template <>
struct Convert<float32, int8>
{
	typedef int8 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1) << 7));
	}
};

template <>
struct Convert<float32, int16>
{
	typedef int16 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1) << 15));
	}
};

template <>
struct Convert<float32, int24>
{
	typedef int24 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1) << 23));
	}
};

template <>
struct Convert<float32, int32>
{
	typedef int32 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1) << 31));
	}
};

template <>
struct Convert<float32, int64>
{
	typedef int64 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<uint64>(1) << 63));
	}
};

template <>
struct Convert<double, uint8>
{
	typedef uint8 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return (static_cast<int>(val) - 0x80) * (1.0 / static_cast<double>(static_cast<unsigned int>(1) << 7));
	}
};

template <>
struct Convert<double, int8>
{
	typedef int8 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1) << 7));
	}
};

template <>
struct Convert<double, int16>
{
	typedef int16 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1) << 15));
	}
};

template <>
struct Convert<double, int24>
{
	typedef int24 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1) << 23));
	}
};

template <>
struct Convert<double, int32>
{
	typedef int32 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1) << 31));
	}
};

template <>
struct Convert<double, int64>
{
	typedef int64 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<uint64>(1) << 63));
	}
};

template <>
struct Convert<double, float>
{
	typedef float input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<double>(val);
	}
};

template <>
struct Convert<float, double>
{
	typedef double input_t;
	typedef float output_t;
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return static_cast<float>(val);
	}
};



template <typename Tdst, typename Tsrc>
MPT_FORCEINLINE Tdst sample_cast(Tsrc src)
{
	return SC::Convert<Tdst, Tsrc>{}(src);
}



}  // namespace SC



OPENMPT_NAMESPACE_END
