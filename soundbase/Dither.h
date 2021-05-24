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
	std::vector<Tdither> DitherChannels;
	typename Tdither::prng_type prng;

public:
	template <typename Trd>
	MultiChannelDither(Trd &rd, std::size_t channels)
		: DitherChannels(channels)
		, prng(Tdither::prng_init(rd))
	{
		return;
	}
	void Reset()
	{
		for(std::size_t channel = 0; channel < DitherChannels.size(); ++channel)
		{
			DitherChannels[channel] = Tdither{};
		}
	}
	std::size_t GetChannels() const
	{
		return DitherChannels.size();
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


class DitherNamesOpenMPT
{
public:
	static mpt::ustring GetModeName(std::size_t mode)
	{
		mpt::ustring result;
		switch(mode)
		{
			case 0:
				// no dither
				result = U_("no");
				break;
			case 1:
				// chosen by OpenMPT code, might change
				result = U_("default");
				break;
			case 2:
				// rectangular, 0.5 bit depth, no noise shaping (original ModPlug Tracker)
				result = U_("0.5 bit");
				break;
			case 3:
				// rectangular, 1 bit depth, simple 1st order noise shaping
				result = U_("1 bit");
				break;
			default:
				result = U_("");
				break;
		}
		return result;
	}
};


template <typename AvailableDithers, typename DitherNames, std::size_t defaultChannels, std::size_t defaultDither, std::size_t noDither>
class Dithers
	: public DitherNames
{

public:
	static constexpr std::size_t NoDither = noDither;
	static constexpr std::size_t DefaultDither = defaultDither;
	static constexpr std::size_t DefaultChannels = defaultChannels;

private:
	mpt::good_prng m_PRNG;
	AvailableDithers m_Dithers;

public:
	template <typename Trd>
	Dithers(Trd &rd, std::size_t mode = defaultDither, std::size_t channels = defaultChannels)
		: m_PRNG(mpt::make_prng<mpt::good_prng>(rd))
		, m_Dithers(std::in_place_index<defaultDither>, m_PRNG, channels)
	{
		SetMode(mode, channels);
	}

	AvailableDithers &Variant()
	{
		return m_Dithers;
	}

	static std::size_t GetNumDithers()
	{
		return std::variant_size<AvailableDithers>::value;
	}

	static std::size_t GetDefaultDither()
	{
		return defaultDither;
	}

	static std::size_t GetNoDither()
	{
		return noDither;
	}

private:
	template <std::size_t i = 0>
	void set_mode(std::size_t mode, std::size_t channels)
	{
		if constexpr(i < std::variant_size<AvailableDithers>::value)
		{
			if(mode == i)
			{
				m_Dithers.template emplace<i>(m_PRNG, channels);
			} else
			{
				this->template set_mode<i + 1>(mode, channels);
			}
		} else
		{
			MPT_UNUSED(mode);
			m_Dithers.template emplace<defaultDither>(m_PRNG, channels);
		}
	}

public:
	void SetMode(std::size_t mode, std::size_t channels)
	{
		if((mode == GetMode()) && (channels == GetChannels()))
		{
			std::visit([](auto &dither)
					   { return dither.Reset(); },
					   m_Dithers);
			return;
		}
		set_mode(mode, channels);
	}
	void SetMode(std::size_t mode)
	{
		if(mode == GetMode())
		{
			std::visit([](auto &dither)
					   { return dither.Reset(); },
					   m_Dithers);
			return;
		}
		set_mode(mode, GetChannels());
	}
	void SetChannels(std::size_t channels)
	{
		if(channels == GetChannels())
		{
			return;
		}
		set_mode(GetMode(), channels);
	}
	void Reset()
	{
		std::visit([](auto &dither)
				   { return dither.Reset(); },
				   m_Dithers);
	}
	std::size_t GetMode() const
	{
		return m_Dithers.index();
	}
	std::size_t GetChannels() const
	{
		return std::visit([](auto &dither)
						  { return dither.GetChannels(); },
						  m_Dithers);
	}
};


using DithersOpenMPT =
	Dithers<std::variant<MultiChannelDither<Dither_None>, MultiChannelDither<Dither_Default>, MultiChannelDither<Dither_ModPlug>, MultiChannelDither<Dither_Simple>>, DitherNamesOpenMPT, 4, 1, 0>;


struct DithersWrapperOpenMPT
	: DithersOpenMPT
{
	template <typename Trd>
	DithersWrapperOpenMPT(Trd &rd, std::size_t mode = DithersOpenMPT::DefaultDither, std::size_t channels = DithersOpenMPT::DefaultChannels)
		: DithersOpenMPT(rd, mode, channels)
	{
		return;
	}
};


OPENMPT_NAMESPACE_END
