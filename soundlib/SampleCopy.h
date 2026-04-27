/*
 * SampleCopy.h
 * ------------
 * Purpose: Functions for copying sample data.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "openmpt/soundbase/SampleConvert.hpp"
#include "openmpt/soundbase/SampleDecode.hpp"

#include <cstddef>


OPENMPT_NAMESPACE_BEGIN


// Copy a sample data buffer.
// targetBuffer: Buffer in which the sample should be copied into.
// numSamples: Number of samples of size T that should be copied. targetBuffer is expected to be able to hold "numSamples * incTarget" samples.
// incTarget: Number of samples by which the target data pointer is increased each time.
// sourceBuffer: Buffer from which the samples should be read.
// sourceSize: Size of source buffer, in bytes.
// incSource: Number of samples by which the source data pointer is increased each time.
//
// Template arguments:
// SampleConversion: Functor of type SampleConversionFunctor to apply sample conversion (see above for existing functors).
template <typename SampleConversion>
std::size_t CopySample(typename SampleConversion::output_t *MPT_RESTRICT outBuf, std::size_t numSamples, std::size_t incTarget, const typename SampleConversion::input_t *MPT_RESTRICT inBuf, std::size_t sourceSize, std::size_t incSource, SampleConversion conv = SampleConversion())
{
	const std::size_t sampleSize = incSource * SampleConversion::input_inc * sizeof(typename SampleConversion::input_t);
	LimitMax(numSamples, sourceSize / sampleSize);
	const std::size_t copySize = numSamples * sampleSize;

	SampleConversion sampleConv(conv);
	while(numSamples--)
	{
		*outBuf = sampleConv(inBuf);
		outBuf += incTarget;
		inBuf += incSource * SampleConversion::input_inc;
	}

	return copySize;
}


// SampleConversion template parameter shortcuts for pure copying of native sample data
namespace SC
{  // SC = _S_ample_C_onversion
	using CopyNative8 = ConversionChain<Convert<int8, int8>, DecodeIdentity<int8>>;
	using CopyNative16 = ConversionChain<Convert<int16, int16>, DecodeIdentity<int16>>;
}


OPENMPT_NAMESPACE_END
