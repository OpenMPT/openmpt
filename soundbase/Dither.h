/*
 * Dither.h
 * --------
 * Purpose: Dithering when converting to lower resolution sample formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "mptBuildSettings.h"


#include "MixSample.h"
#include "SampleFormatConverters.h"
#include "MixSampleConvert.h"
#include "../common/mptRandom.h"

#include <variant>


OPENMPT_NAMESPACE_BEGIN


enum DitherMode
{
	DitherNone = 0,
	DitherDefault = 1,  // chosen by OpenMPT code, might change
	DitherModPlug = 2,  // rectangular, 0.5 bit depth, no noise shaping (original ModPlug Tracker)
	DitherSimple = 3,   // rectangular, 1 bit depth, simple 1st order noise shaping
	NumDitherModes
};


struct Dither_None
{
public:
	using prng_type = struct
	{
	};
	template <typename Trd>
	static prng_type prng_init(Trd &)
	{
		return prng_type{};
	}

public:
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleInt process(MixSampleInt sample, Trng &)
	{
		return sample;
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleFloat process(MixSampleFloat sample, Trng &)
	{
		return sample;
	}
};


struct Dither_ModPlug
{
public:
	using prng_type = mpt::rng::modplug_dither;
	template <typename Trd>
	static prng_type prng_init(Trd &)
	{
		return prng_type{0, 0};
	}

public:
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleInt process(MixSampleInt sample, Trng &rng)
	{
		if constexpr(targetbits == 0)
		{
			MPT_UNREFERENCED_PARAMETER(rng);
			return sample;
		} else if constexpr(targetbits + MixSampleIntTraits::mix_headroom_bits + 1 >= 32)
		{
			MPT_UNREFERENCED_PARAMETER(rng);
			return sample;
		} else
		{
			sample += mpt::rshift_signed(static_cast<int32>(mpt::random<uint32>(rng)), (targetbits + MixSampleIntTraits::mix_headroom_bits + 1));
			return sample;
		}
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleFloat process(MixSampleFloat sample, Trng &prng)
	{
		return mix_sample_cast<MixSampleFloat>(process<targetbits>(mix_sample_cast<MixSampleInt>(sample), prng));
	}
};


template <int ditherdepth = 1, bool triangular = false, bool shaped = true>
struct Dither_SimpleImpl
{
public:
	using prng_type = mpt::fast_prng;
	template <typename Trd>
	static prng_type prng_init(Trd &rd)
	{
		return mpt::make_prng<prng_type>(rd);
	}

private:
	int32 error = 0;

public:
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleInt process(MixSampleInt sample, Trng &prng)
	{
		if constexpr(targetbits == 0)
		{
			MPT_UNREFERENCED_PARAMETER(prng);
			return sample;
		} else
		{
			static_assert(sizeof(MixSampleInt) == 4);
			constexpr int rshift = (32 - targetbits) - MixSampleIntTraits::mix_headroom_bits;
			if constexpr(rshift <= 1)
			{
				MPT_UNREFERENCED_PARAMETER(prng);
				// nothing to dither
				return sample;
			} else
			{
				constexpr int rshiftpositive = (rshift > 1) ? rshift : 1;  // work-around warnings about negative shift with C++14 compilers
				constexpr int round_mask = ~((1 << rshiftpositive) - 1);
				constexpr int round_offset = 1 << (rshiftpositive - 1);
				constexpr int noise_bits = rshiftpositive + (ditherdepth - 1);
				constexpr int noise_bias = (1 << (noise_bits - 1));
				int32 e = error;
				unsigned int unoise = 0;
				if constexpr(triangular)
				{
					unoise = (mpt::random<unsigned int>(prng, noise_bits) + mpt::random<unsigned int>(prng, noise_bits)) >> 1;
				} else
				{
					unoise = mpt::random<unsigned int>(prng, noise_bits);
				}
				int noise = static_cast<int>(unoise) - noise_bias;  // un-bias
				int val = sample;
				if constexpr(shaped)
				{
					val += (e >> 1);
				}
				int rounded = (val + noise + round_offset) & round_mask;
				e = val - rounded;
				sample = rounded;
				error = e;
				return sample;
			}
		}
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleFloat process(MixSampleFloat sample, Trng &prng)
	{
		return mix_sample_cast<MixSampleFloat>(process<targetbits>(mix_sample_cast<MixSampleInt>(sample), prng));
	}
};

using Dither_Simple = Dither_SimpleImpl<>;

using Dither_Default = Dither_Simple;


template <typename Tdither>
class MultiChannelDither
{
private:
	std::vector<Tdither> DitherChannels = std::vector<Tdither>(4);
	typename Tdither::prng_type prng;

public:
	template <typename Trd>
	MultiChannelDither(Trd &rd, std::size_t channels = 4)
		: prng(Tdither::prng_init(rd))
		, DitherChannels(channels)
	{
		return;
	}
	void SetChannels(std::size_t channels)
	{
		DitherChannels = std::vector<Tdither>(channels);
	}
	std::size_t GetChannels() const
	{
		return DitherChannels.size();
	}
	void Reset()
	{
		DitherChannels = std::vector<Tdither>(DitherChannels.size());
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleInt process(std::size_t channel, MixSampleInt sample)
	{
		return DitherChannels[channel].template process<targetbits>(sample, prng);
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleFloat process(std::size_t channel, MixSampleFloat sample)
	{
		return DitherChannels[channel].template process<targetbits>(sample, prng);
	}
};


class DitherNames
{
public:
	static mpt::ustring GetModeName(DitherMode mode)
	{
		mpt::ustring result;
		switch(mode)
		{
			case DitherNone: result = U_("no"); break;
			case DitherDefault: result = U_("default"); break;
			case DitherModPlug: result = U_("0.5 bit"); break;
			case DitherSimple: result = U_("1 bit"); break;
			default: result = U_(""); break;
		}
		return result;
	}
};


class Dither
	: public DitherNames
{

public:
	using TAllDithers = std::variant<
		MultiChannelDither<Dither_None>,
		MultiChannelDither<Dither_Default>,
		MultiChannelDither<Dither_ModPlug>,
		MultiChannelDither<Dither_Simple>>;

private:
	TAllDithers m_Dithers;
	mpt::good_prng m_PRNG;

public:
	template <typename Trd>
	Dither(Trd &rd)
		: m_PRNG(mpt::make_prng<mpt::good_prng>(rd))
		, m_Dithers(std::in_place_index<static_cast<std::size_t>(DitherDefault)>, m_PRNG)
	{
		return;
	}

	void SetChannels(std::size_t channels)
	{
		std::visit(
			[&](auto &dither)
			{
				dither.SetChannels(channels);
			},
			m_Dithers);
	}
	std::size_t GetChannels() const
	{
		return std::visit(
			[&](auto &dither)
			{
				return dither.GetChannels();
			},
			m_Dithers);
	}

	void Reset()
	{
		std::visit(
			[&](auto &dither)
			{
				dither.Reset();
			},
			m_Dithers);
	}

	template <typename Tfn>
	auto visit(Tfn fn)
	{
		std::visit(fn, m_Dithers);
	}

	void SetMode(DitherMode mode)
	{
		if(mode == GetMode())
		{
			return;
		}
		const std::size_t oldChannels = GetChannels();
		switch(mode)
		{
			case DitherNone:
				m_Dithers.template emplace<static_cast<std::size_t>(DitherNone)>(m_PRNG, oldChannels);
				break;
			case DitherDefault:
				m_Dithers.template emplace<static_cast<std::size_t>(DitherDefault)>(m_PRNG, oldChannels);
				break;
			case DitherModPlug:
				m_Dithers.template emplace<static_cast<std::size_t>(DitherModPlug)>(m_PRNG, oldChannels);
				break;
			case DitherSimple:
				m_Dithers.template emplace<static_cast<std::size_t>(DitherSimple)>(m_PRNG, oldChannels);
				break;
		}
	}
	DitherMode GetMode() const
	{
		return static_cast<DitherMode>(m_Dithers.index());
	}
};


OPENMPT_NAMESPACE_END
