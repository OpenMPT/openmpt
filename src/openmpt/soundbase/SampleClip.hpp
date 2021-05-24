/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/macros.hpp"
#include "openmpt/base/Int24.hpp"
#include "openmpt/base/Types.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace SC
{  // SC = _S_ample_C_onversion


template <typename Tsample, bool clipOutput>
struct Clip;

template <bool clipOutput>
struct Clip<uint8, clipOutput>
{
	typedef uint8 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE uint8 operator()(uint8 val)
	{
		return val;
	}
};

template <bool clipOutput>
struct Clip<int8, clipOutput>
{
	typedef int8 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE int8 operator()(int8 val)
	{
		return val;
	}
};

template <bool clipOutput>
struct Clip<int16, clipOutput>
{
	typedef int16 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE int16 operator()(int16 val)
	{
		return val;
	}
};

template <bool clipOutput>
struct Clip<int24, clipOutput>
{
	typedef int24 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE int24 operator()(int24 val)
	{
		return val;
	}
};

template <bool clipOutput>
struct Clip<int32, clipOutput>
{
	typedef int32 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE int32 operator()(int32 val)
	{
		return val;
	}
};

template <bool clipOutput>
struct Clip<int64, clipOutput>
{
	typedef int64 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE int64 operator()(int64 val)
	{
		return val;
	}
};

template <bool clipOutput>
struct Clip<float, clipOutput>
{
	typedef float input_t;
	typedef float output_t;
	MPT_FORCEINLINE float operator()(float val)
	{
		if constexpr(clipOutput)
		{
			if(val < -1.0f) val = -1.0f;
			if(val > 1.0f) val = 1.0f;
			return val;
		} else
		{
			return val;
		}
	}
};

template <bool clipOutput>
struct Clip<double, clipOutput>
{
	typedef double input_t;
	typedef double output_t;
	MPT_FORCEINLINE double operator()(double val)
	{
		if constexpr(clipOutput)
		{
			if(val < -1.0) val = -1.0;
			if(val > 1.0) val = 1.0;
			return val;
		} else
		{
			return val;
		}
	}
};


}  // namespace SC


OPENMPT_NAMESPACE_END
