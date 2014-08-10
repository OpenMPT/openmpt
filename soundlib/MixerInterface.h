/*
 * MixerInterface.h
 * ----------------
 * Purpose: The basic mixer interface and main mixer loop, completely agnostic of the actual sample input / output formats.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Snd_defs.h"
#include "ModChannel.h"

OPENMPT_NAMESPACE_BEGIN

class CResampler;

//////////////////////////////////////////////////////////////////////////
// Sample conversion traits

template<int channelsOut, int channelsIn, typename out, typename in>
struct MixerTraits
{
	static const int numChannelsIn = channelsIn;	// Number of channels in sample
	static const int numChannelsOut = channelsOut;	// Number of mixer output channels
	typedef out output_t;							// Output buffer sample type
	typedef in input_t;								// Input buffer sample type
	typedef out outbuf_t[channelsOut];				// Output buffer sampling point type
	// To perform sample conversion, add a function with the following signature to your derived classes:
	// static forceinline output_t Convert(const input_t x)
};


//////////////////////////////////////////////////////////////////////////
// Interpolation templates

template<class Traits>
struct NoInterpolation
{
	forceinline void Start(const ModChannel &, const CResampler &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] = Traits::Convert(inBuffer[i]);
		}
	}
};

// Other interpolation algorithms depend on the input format type (integer / float) and can thus be found in FloatMixer.h and IntMixer.h


//////////////////////////////////////////////////////////////////////////
// Main sample render loop template

// Template parameters:
// Traits: A class derived from MixerTraits that defines the number of channels, sample buffer types, etc..
// InterpolationFunc: Functor for reading the sample data and doing the SRC
// FilterFunc: Functor for applying the resonant filter
// MixFunc: Functor for mixing the computed sample data into the output buffer
template<class Traits, class InterpolationFunc, class FilterFunc, class MixFunc>
static void SampleLoop(ModChannel &chn, const CResampler &resampler, typename Traits::output_t * MPT_RESTRICT outBuffer, int numSamples)
{
	ModChannel &c = chn;
	const typename Traits::input_t * MPT_RESTRICT inSample = static_cast<const typename Traits::input_t *>(c.pCurrentSample) + c.nPos * Traits::numChannelsIn;

	int32 smpPos = c.nPosLo;	// 16.16 sample position relative to c.nPos

	InterpolationFunc interpolate;
	FilterFunc filter;
	MixFunc mix;

	// Do initialisation if necessary
	interpolate.Start(c, resampler);
	filter.Start(c);
	mix.Start(c);

	int samples = numSamples;
	while(samples--)
	{
		typename Traits::outbuf_t outSample;
		interpolate(outSample, inSample + (smpPos >> 16) * Traits::numChannelsIn, (smpPos & 0xFFFF));
		filter(outSample, c);
		mix(outSample, c, outBuffer);
		outBuffer += Traits::numChannelsOut;

		smpPos += c.nInc;
	}

	mix.End(c);
	filter.End(c);
	interpolate.End(c);

	c.nPos += smpPos >> 16;
	c.nPosLo = smpPos & 0xFFFF;
}

// Type of the SampleLoop function above
typedef void (*MixFuncInterface)(ModChannel &, const CResampler &, mixsample_t *, int);

OPENMPT_NAMESPACE_END
