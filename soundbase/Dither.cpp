/*
 * Dither.cpp
 * ----------
 * Purpose: Dithering when converting to lower resolution sample formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"

#include "Dither.h"

#include "../common/misc_util.h"


OPENMPT_NAMESPACE_BEGIN


mpt::ustring Dither::GetModeName(DitherMode mode)
{
	switch(mode)
	{
		case DitherNone   : return U_("no"     ); break;
		case DitherDefault: return U_("default"); break;
		case DitherModPlug: return U_("0.5 bit"); break;
		case DitherSimple : return U_("1 bit"  ); break;
		default           : return U_(""       ); break;
	}
}


static void Dither_ModPlug(MixSampleInt *pBuffer, std::size_t count, std::size_t channels, uint32 nBits, DitherModPlugState &state)
{
	if(nBits + MixSampleIntTraits::mix_headroom_bits() + 1 >= 32)
	{
		return;
	}
	count *= channels;
	while(count--)
	{
		*pBuffer += mpt::rshift_signed(static_cast<int32>(state.rng()), (nBits + MixSampleIntTraits::mix_headroom_bits() + 1));
		pBuffer++;
	}
}


template<int targetbits, int channels, int ditherdepth = 1, bool triangular = false, bool shaped = true>
struct Dither_SimpleTemplate
{
MPT_NOINLINE void operator () (MixSampleInt *mixbuffer, std::size_t count, DitherSimpleState &state, mpt::fast_prng &prng)
{
	STATIC_ASSERT(sizeof(MixSampleInt) == 4);
	constexpr int rshift = (32-targetbits) - MixSampleIntTraits::mix_headroom_bits();
	MPT_CONSTANT_IF(rshift <= 0)
	{
		// nothing to dither
		return;
	}
	const int round_mask = ~((1<<rshift)-1);
	const int round_offset = 1<<(rshift-1);
	const int noise_bits = rshift + (ditherdepth - 1);
	const int noise_bias = (1<<(noise_bits-1));
	DitherSimpleState s = state;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			unsigned int unoise = 0;
			MPT_CONSTANT_IF(triangular)
			{
				unoise = (mpt::random<unsigned int>(prng, noise_bits) + mpt::random<unsigned int>(prng, noise_bits)) >> 1;
			} else
			{
				unoise = mpt::random<unsigned int>(prng, noise_bits);
			}
			int noise = static_cast<int>(unoise) - noise_bias; // un-bias
			int val = *mixbuffer;
			MPT_CONSTANT_IF(shaped)
			{
				val += (s.error[channel] >> 1);
			}
			int rounded = (val + noise + round_offset) & round_mask;;
			s.error[channel] = val - rounded;
			*mixbuffer = rounded;
			mixbuffer++;
		}
	}
	state = s;
}
};

static void Dither_Simple(MixSampleInt *mixbuffer, std::size_t count, std::size_t channels, int bits, DitherSimpleState &state, mpt::fast_prng &prng)
{
	switch(bits)
	{
		case 8:
			switch(channels)
			{
				case 1:
					Dither_SimpleTemplate<8,1>()(mixbuffer, count, state, prng);
					break;
				case 2:
					Dither_SimpleTemplate<8,2>()(mixbuffer, count, state, prng);
					break;
				case 4:
					Dither_SimpleTemplate<8,4>()(mixbuffer, count, state, prng);
					break;
			}
			break;
		case 16:
			switch(channels)
			{
				case 1:
					Dither_SimpleTemplate<16,1>()(mixbuffer, count, state, prng);
					break;
				case 2:
					Dither_SimpleTemplate<16,2>()(mixbuffer, count, state, prng);
					break;
				case 4:
					Dither_SimpleTemplate<16,4>()(mixbuffer, count, state, prng);
					break;
			}
			break;
		case 24:
			switch(channels)
			{
				case 1:
					Dither_SimpleTemplate<24,1>()(mixbuffer, count, state, prng);
					break;
				case 2:
					Dither_SimpleTemplate<24,2>()(mixbuffer, count, state, prng);
					break;
				case 4:
					Dither_SimpleTemplate<24,4>()(mixbuffer, count, state, prng);
					break;
			}
			break;
	}
}


void Dither::Reset()
{
	state.Reset();
	}


void Dither::SetMode(DitherMode mode_)
{
	mode = mode_;
}


DitherMode Dither::GetMode() const
{
	return mode;
}


void Dither::Process(MixSampleInt *mixbuffer, std::size_t count, std::size_t channels, int bits)
{
	switch(mode)
	{
		case DitherNone:
			// nothing
			break;
		case DitherModPlug:
			Dither_ModPlug(mixbuffer, count, channels, bits, state.modplug);
			break;
		case DitherSimple:
			Dither_Simple(mixbuffer, count, channels, bits, state.simple, state.prng);
			break;
		case DitherDefault:
		default:
			Dither_ModPlug(mixbuffer, count, channels, bits, state.modplug);
			break;
	}
}


OPENMPT_NAMESPACE_END
