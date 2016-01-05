/*
 * IntMixer.h
 * ----------
 * Purpose: Fixed point mixer classes
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Resampler.h"
#include "MixerInterface.h"

OPENMPT_NAMESPACE_BEGIN

template<int channelsOut, int channelsIn, typename out, typename in, size_t mixPrecision>
struct IntToIntTraits : public MixerTraits<channelsOut, channelsIn, out, in>
{
	typedef MixerTraits<channelsOut, channelsIn, out, in> base_t;
	typedef typename base_t::input_t input_t;
	typedef typename base_t::output_t output_t;

	static forceinline output_t Convert(const input_t x)
	{
		static_assert(std::numeric_limits<input_t>::is_integer, "Input must be integer");
		static_assert(std::numeric_limits<output_t>::is_integer, "Output must be integer");
		static_assert(sizeof(out) * 8 >= mixPrecision, "Mix precision is higher than output type can handle");
		static_assert(sizeof(in) * 8 <= mixPrecision, "Mix precision is lower than input type");
		return static_cast<output_t>(x) << (mixPrecision - sizeof(in) * 8);
	}
};

typedef IntToIntTraits<2, 1, mixsample_t, int8,  16> Int8MToIntS;
typedef IntToIntTraits<2, 1, mixsample_t, int16, 16> Int16MToIntS;
typedef IntToIntTraits<2, 2, mixsample_t, int8,  16> Int8SToIntS;
typedef IntToIntTraits<2, 2, mixsample_t, int16, 16> Int16SToIntS;


//////////////////////////////////////////////////////////////////////////
// Interpolation templates

template<class Traits>
struct LinearInterpolation
{
	forceinline void Start(const ModChannel &, const CResampler &) { }

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const typename Traits::output_t fract = posLo >> 8;

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			typename Traits::output_t srcVol = Traits::Convert(inBuffer[i]);
			typename Traits::output_t destVol = Traits::Convert(inBuffer[i + Traits::numChannelsIn]);

			outSample[i] = srcVol + ((fract * (destVol - srcVol)) >> 8);
		}
	}
};


template<class Traits>
struct FastSincInterpolation
{
	forceinline void Start(const ModChannel &, const CResampler &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const int16 *lut = CResampler::FastSincTable + ((posLo >> 6) & 0x3FC);

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				 (lut[0] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i])
				+ lut[2] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn])) >> 14;
		}
	}
};


template<class Traits>
struct PolyphaseInterpolation
{
	const SINC_TYPE *sinc;

	forceinline void Start(const ModChannel &chn, const CResampler &resampler)
	{
		#ifdef MODPLUG_TRACKER
			// Otherwise causes "warning C4100: 'resampler' : unreferenced formal parameter"
			// because all 3 tables are static members.
			// #pragma warning fails with this templated case for unknown reasons.
			MPT_UNREFERENCED_PARAMETER(resampler);
		#endif // MODPLUG_TRACKER
		sinc = (((chn.nInc > 0x13000) || (chn.nInc < -0x13000)) ?
			(((chn.nInc > 0x18000) || (chn.nInc < -0x18000)) ? resampler.gDownsample2x : resampler.gDownsample13x) : resampler.gKaiserSinc);
	}

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const SINC_TYPE *lut = sinc + ((posLo >> (16 - SINC_PHASES_BITS)) & SINC_MASK) * SINC_WIDTH;

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				 (lut[0] * Traits::Convert(inBuffer[i - 3 * Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i - 2 * Traits::numChannelsIn])
				+ lut[2] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i])
				+ lut[4] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[5] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn])
				+ lut[6] * Traits::Convert(inBuffer[i + 3 * Traits::numChannelsIn])
				+ lut[7] * Traits::Convert(inBuffer[i + 4 * Traits::numChannelsIn])) >> SINC_QUANTSHIFT;
		}
	}
};


template<class Traits>
struct FIRFilterInterpolation
{
	const int16 *WFIRlut;

	forceinline void Start(const ModChannel &, const CResampler &resampler)
	{
		WFIRlut = resampler.m_WindowedFIR.lut;
	}

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const int16 * const lut = WFIRlut + (((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK);

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			typename Traits::output_t vol1 =
				  (lut[0] * Traits::Convert(inBuffer[i - 3 * Traits::numChannelsIn]))
				+ (lut[1] * Traits::Convert(inBuffer[i - 2 * Traits::numChannelsIn]))
				+ (lut[2] * Traits::Convert(inBuffer[i - Traits::numChannelsIn]))
				+ (lut[3] * Traits::Convert(inBuffer[i]));
			typename Traits::output_t vol2 =
				  (lut[4] * Traits::Convert(inBuffer[i + 1 * Traits::numChannelsIn]))
				+ (lut[5] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn]))
				+ (lut[6] * Traits::Convert(inBuffer[i + 3 * Traits::numChannelsIn]))
				+ (lut[7] * Traits::Convert(inBuffer[i + 4 * Traits::numChannelsIn]));
			outSample[i] = ((vol1 >> 1) + (vol2 >> 1)) >> (WFIR_16BITSHIFT - 1);
		}
	}
};


//////////////////////////////////////////////////////////////////////////
// Mixing templates (add sample to stereo mix)

template<class Traits>
struct NoRamp
{
	typename Traits::output_t lVol, rVol;

	forceinline void Start(const ModChannel &chn)
	{
		lVol = chn.leftVol;
		rVol = chn.rightVol;
	}

	forceinline void End(const ModChannel &) { }
};


struct Ramp
{
	int32 lRamp, rRamp;

	forceinline void Start(const ModChannel &chn)
	{
		lRamp = chn.rampLeftVol;
		rRamp = chn.rampRightVol;
	}

	forceinline void End(ModChannel &chn)
	{
		chn.rampLeftVol = lRamp; chn.leftVol = lRamp >> VOLUMERAMPPRECISION;
		chn.rampRightVol = rRamp; chn.rightVol = rRamp >> VOLUMERAMPPRECISION;
	}
};


// Legacy optimization: If chn.nLeftVol == chn.nRightVol, save one multiplication instruction
template<class Traits>
struct MixMonoFastNoRamp : public NoRamp<Traits>
{
	typedef NoRamp<Traits> base_t;
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &, typename Traits::output_t * const outBuffer)
	{
		typename Traits::output_t vol = outSample[0] * base_t::lVol;
		for(int i = 0; i < Traits::numChannelsOut; i++)
		{
			outBuffer[i] += vol;
		}
	}
};


template<class Traits>
struct MixMonoNoRamp : public NoRamp<Traits>
{
	typedef NoRamp<Traits> base_t;
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &, typename Traits::output_t * const outBuffer)
	{
		outBuffer[0] += outSample[0] * base_t::lVol;
		outBuffer[1] += outSample[0] * base_t::rVol;
	}
};


template<class Traits>
struct MixMonoRamp : public Ramp
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		lRamp += chn.leftRamp;
		rRamp += chn.rightRamp;
		outBuffer[0] += outSample[0] * (lRamp >> VOLUMERAMPPRECISION);
		outBuffer[1] += outSample[0] * (rRamp >> VOLUMERAMPPRECISION);
	}
};


template<class Traits>
struct MixStereoNoRamp : public NoRamp<Traits>
{
	typedef NoRamp<Traits> base_t;
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &, typename Traits::output_t * const outBuffer)
	{
		outBuffer[0] += outSample[0] * base_t::lVol;
		outBuffer[1] += outSample[1] * base_t::rVol;
	}
};


template<class Traits>
struct MixStereoRamp : public Ramp
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		lRamp += chn.leftRamp;
		rRamp += chn.rightRamp;
		outBuffer[0] += outSample[0] * (lRamp >> VOLUMERAMPPRECISION);
		outBuffer[1] += outSample[1] * (rRamp >> VOLUMERAMPPRECISION);
	}
};


//////////////////////////////////////////////////////////////////////////
// Filter templates


template<class Traits>
struct NoFilter
{
	forceinline void Start(const ModChannel &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (const typename Traits::outbuf_t &, const ModChannel &) { }
};


// Resonant filter
template<class Traits>
struct ResonantFilter
{
	// Filter history
	typename Traits::output_t fy[Traits::numChannelsIn][2];

	forceinline void Start(const ModChannel &chn)
	{
		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			fy[i][0] = chn.nFilter_Y[i][0];
			fy[i][1] = chn.nFilter_Y[i][1];
		}
	}

	forceinline void End(ModChannel &chn)
	{
		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			chn.nFilter_Y[i][0] = fy[i][0];
			chn.nFilter_Y[i][1] = fy[i][1];
		}
	}

	// Filter values are clipped to double the input range
#define ClipFilter(x) Clamp<typename Traits::output_t, typename Traits::output_t>(x, int16_min << 1, int16_max << 1)

	forceinline void operator() (typename Traits::outbuf_t &outSample, const ModChannel &chn)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			typename Traits::output_t val = (outSample[i] * chn.nFilter_A0 + ClipFilter(fy[i][0]) * chn.nFilter_B0 + ClipFilter(fy[i][1]) * chn.nFilter_B1
				+ (1 << (MIXING_FILTER_PRECISION - 1))) >> MIXING_FILTER_PRECISION;
			fy[i][1] = fy[i][0];
			fy[i][0] = val - (outSample[i] & chn.nFilter_HP);
			outSample[i] = val;
		}
	}

#undef ClipFilter
};


OPENMPT_NAMESPACE_END
