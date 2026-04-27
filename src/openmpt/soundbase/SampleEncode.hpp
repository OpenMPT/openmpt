/* SPDX-License-Identifier: BSD-3-Clause */
/* SPDX-FileCopyrightText: OpenMPT Project Developers and Contributors */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/base/bit.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/memory.hpp"
#include "openmpt/base/Endian.hpp"
#include "openmpt/base/Types.hpp"

#include <algorithm>

#include <cmath>
#include <cstddef>
#include <cstdlib>


OPENMPT_NAMESPACE_BEGIN


namespace SC
{  // SC = _S_ample_C_onversion


struct EncodeuLaw
{
	using input_t = int16;
	using output_t = uint8;
	using encoded_t = std::byte;
	static constexpr std::size_t encoded_inc = 1;
	static constexpr uint8 exp_table[17] = {0, 7 << 4, 6 << 4, 5 << 4, 4 << 4, 3 << 4, 2 << 4, 1 << 4, 0 << 4, 0, 0, 0, 0, 0, 0, 0, 0};
	static constexpr uint8 mant_table[17] = {0, 10, 9, 8, 7, 6, 5, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3};
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE output_t operator()(input_t val)
	{
		uint16 x = static_cast<uint16>(val);
		uint8 out = static_cast<uint8>((x >> 8) & 0x80u);
		uint32 abs = x & 0x7fffu;
		if(x & 0x8000u)
		{
			abs ^= 0x7fffu;
			abs += 1u;
		}
		x = static_cast<uint16>(std::clamp(static_cast<uint32>(abs + (33 << 2)), static_cast<uint32>(0), static_cast<uint32>(0x7fff)));
		int index = mpt::countl_zero(x);
		out |= exp_table[index];
		out |= (x >> mant_table[index]) & 0x0fu;
		out ^= 0xffu;
		return out;
	}
};


struct EncodeALaw
{
	using input_t = int16;
	using output_t = uint8;
	using encoded_t = std::byte;
	static constexpr std::size_t encoded_inc = 1;
	static constexpr uint8 exp_table[17] = {0, 7 << 4, 6 << 4, 5 << 4, 4 << 4, 3 << 4, 2 << 4, 1 << 4, 0 << 4, 0, 0, 0, 0, 0, 0, 0, 0};
	static constexpr uint8 mant_table[17] = {0, 10, 9, 8, 7, 6, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE output_t operator()(input_t val)
	{
		int16 sx = std::clamp(val, static_cast<int16>(-32767), static_cast<int16>(32767));
		uint16 x = static_cast<uint16>(sx);
		uint8 out = static_cast<uint8>(((x & 0x8000u) ^ 0x8000u) >> 8);
		x = static_cast<uint16>(std::abs(sx));
		int index = mpt::countl_zero(x);
		out |= exp_table[index];
		out |= (x >> mant_table[index]) & 0x0fu;
		out ^= 0x55u;
		return out;
	}
};


template <mpt::endian endian, typename Tsample>
struct EncodeEndian
{
	using input_t = Tsample;
	using output_t = mpt::packed<Tsample, endian>;
	using encoded_t = std::byte;
	static constexpr std::size_t encoded_inc = sizeof(output_t);
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE output_t operator()(input_t val)
	{
		output_t out;
		out = val;
		return out;
	}
};


template <typename Func2, typename Func1>
struct EncodeChain
{
	using input_t = typename Func1::input_t;
	using output_t = typename Func2::output_t;
	using encoded_t = typename Func2::encoded_t;
	static constexpr std::size_t encoded_inc = Func2::encoded_inc;
	Func1 func1;
	Func2 func2;
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE output_t operator()(input_t in)
	{
		return func2(func1(in));
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE EncodeChain(Func2 f2 = Func2(), Func1 f1 = Func1())
		: func1(f1)
		, func2(f2)
	{
		return;
	}
};



}  // namespace SC


OPENMPT_NAMESPACE_END
