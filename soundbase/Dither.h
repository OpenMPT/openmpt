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
		return prng_type{rd};
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


template <typename Tdither>
class MultiChannelDither
{
private:
	std::vector<Tdither> DitherChannels = std::vector<Tdither>(4);
	typename Tdither::prng_type prng;

public:
	template <typename Trd>
	MultiChannelDither(Trd &rd)
		: prng(Tdither::prng_init(rd))
	{
		return;
	}
	void SetChannels(std::size_t channels)
	{
		DitherChannels = std::vector<Tdither>(channels);
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

private:
	MultiChannelDither<Dither_None> ditherNone;
	MultiChannelDither<Dither_ModPlug> ditherModPlug;
	MultiChannelDither<Dither_Simple> ditherSimple;

	DitherMode mode = DitherDefault;

public:
	template <typename Trd>
	Dither(Trd &rd)
		: ditherNone(rd)
		, ditherModPlug(rd)
		, ditherSimple(rd)
	{
		return;
	}
	void SetChannels(std::size_t channels)
	{
		ditherNone.SetChannels(channels);
		ditherModPlug.SetChannels(channels);
		ditherSimple.SetChannels(channels);
	}
	void Reset()
	{
		ditherNone.Reset();
		ditherModPlug.Reset();
		ditherSimple.Reset();
	}

	MultiChannelDither<Dither_None> &NoDither()
	{
		MPT_ASSERT(mode == DitherNone);
		return ditherNone;
	}
	MultiChannelDither<Dither_Simple> &DefaultDither()
	{
		MPT_ASSERT(mode == DitherDefault);
		return ditherSimple;
	}
	MultiChannelDither<Dither_ModPlug> &ModPlugDither()
	{
		MPT_ASSERT(mode == DitherModPlug);
		return ditherModPlug;
	}
	MultiChannelDither<Dither_Simple> &SimpleDither()
	{
		MPT_ASSERT(mode == DitherSimple);
		return ditherSimple;
	}

	template <typename Tfn>
	auto WithDither(Tfn fn)
	{
		switch(GetMode())
		{
			case DitherNone: return fn(NoDither()); break;
			case DitherModPlug: return fn(ModPlugDither()); break;
			case DitherSimple: return fn(SimpleDither()); break;
			case DitherDefault: return fn(DefaultDither()); break;
			default: return fn(DefaultDither()); break;
		}
	}

	void SetMode(DitherMode mode_)
	{
		mode = mode_;
	}
	DitherMode GetMode() const
	{
		return mode;
	}
};


OPENMPT_NAMESPACE_END
