/*
 * SampleFormatConverters.h
 * ------------------------
 * Purpose: Functions and functors for reading and converting pretty much any uncompressed sample format supported by OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

// Byte offsets, from lowest significant to highest significant byte (for various functor template parameters)
#define littleEndian32 0, 1, 2, 3
#define littleEndian24 0, 1, 2
#define littleEndian16 0, 1

#define bigEndian32 3, 2, 1, 0
#define bigEndian24 2, 1, 0
#define bigEndian16 1, 0


//////////////////////////////////////////////////////
// Sample conversion functors

enum SampleConversionState { conversionHasNoState, conversionHasState };

// Sample conversion functor base class, which contains all the basic properties of the conversion.
template <typename TIn, typename TOut, SampleConversionState state>
struct SampleConversionFunctor
{
	// The sample data type that is accepted as input by the functor
	typedef TIn input_t;
	// The sample data type that is output by the functor
	typedef TOut output_t;

	// Declares whether the sample conversion functor has a state, i.e. if there are
	// dependencies between two functor calls (e.g. when loading delta-encoded samples).
	// Sometimes there might be quicker ways to load a sample if it is known that there is
	// no state, for example in the case CopyStereoInterleavedSample: If there are no
	// dependencies between functor calls, we can load the whole interleaved stream in one
	// go, else we will have to load the left and right channel independently (example:
	// for delta-encoded samples, the delta value is usally a per-channel property)
	static const SampleConversionState hasState = state;

	// Now it's your job to add this to child classes :)
	// inline output_t operator() (const void *sourceBuffer) { ... }
};


// 7-Bit sample with unused high bit conversion
template <int8 offset>
struct ReadInt7to8PCM : SampleConversionFunctor<int8, int8, conversionHasNoState>
{
	inline int8 operator() (const void *sourceBuffer)
	{
		return static_cast<int8>(Clamp(static_cast<int16>((*static_cast<const int8 *>(sourceBuffer)) - offset) * 2, -128, 127));
	}
};


// 8-Bit sample conversion
template <int8 offset>
struct ReadInt8PCM : SampleConversionFunctor<int8, int8, conversionHasNoState>
{
	inline int8 operator() (const void *sourceBuffer)
	{
		return (*static_cast<const int8 *>(sourceBuffer)) - offset;
	}
};


// 8-Bit delta sample conversion
struct ReadInt8DeltaPCM : SampleConversionFunctor<int8, int8, conversionHasState>
{
	int8 delta;

	ReadInt8DeltaPCM() : delta(0) { }

	inline int8 operator() (const void *sourceBuffer)
	{
		delta += *static_cast<const int8 *>(sourceBuffer);
		return static_cast<int8>(delta);
	}
};


// 16-Bit sample conversion (for both little and big endian, depending on template parameters)
template <int16 offset, size_t loByteIndex, size_t hiByteIndex>
struct ReadInt16PCM : SampleConversionFunctor<int16, int16, conversionHasNoState>
{
	inline int16 operator() (const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		return (inBuf[loByteIndex] | (inBuf[hiByteIndex] << 8)) - offset;
	}
};


// 16-Bit delta sample conversion (for both little and big endian, depending on template parameters)
template <size_t loByteIndex, size_t hiByteIndex>
struct ReadInt16DeltaPCM : SampleConversionFunctor<int16, int16, conversionHasState>
{
	int16 delta;

	ReadInt16DeltaPCM() : delta(0) { }

	inline int16 operator() (const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		delta += inBuf[loByteIndex] | (inBuf[hiByteIndex] << 8);
		return static_cast<int16>(delta);
	}
};


// 8-Bit delta to 16-Bit sample conversion (PTM samples)
struct ReadInt8to16DeltaPCM : SampleConversionFunctor<int16, int16, conversionHasState>
{
	int16 delta;

	ReadInt8to16DeltaPCM() : delta(0) { }

	inline int16 operator() (const void *sourceBuffer)
	{
		const int8 *inBuf = static_cast<const int8 *>(sourceBuffer);
		delta += inBuf[0];
		int16 result = delta & 0xFF;
		delta += inBuf[1];
		result |= (delta << 8);
		return result;
	}
};


// 24-Bit sample conversion (for both little and big endian, depending on template parameters): Extend to 32-Bit
template <int32 offset, size_t loByteIndex, size_t midByteIndex, size_t hiByteIndex>
struct ReadInt24to32PCM : SampleConversionFunctor<char[3], int32, conversionHasNoState>
{
	inline int32 operator() (const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		return ((inBuf[loByteIndex] << 8) | (inBuf[midByteIndex] << 16) | (inBuf[hiByteIndex] << 24)) - offset;
	}
};


// 32-Bit sample conversion (for both little and big endian, depending on template parameters)
template <int32 offset, size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct ReadInt32PCM : SampleConversionFunctor<int32, int32, conversionHasNoState>
{
	inline int32 operator() (const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		return (inBuf[loLoByteIndex] | (inBuf[loHiByteIndex] << 8) | (inBuf[hiLoByteIndex] << 16) | (inBuf[hiHiByteIndex] << 24)) - offset;
	}
};


// Signed integer PCM (e.g. 24-Bit, 32-Bit) to 16-Bit signed sample conversion with truncation (for both little and big endian, depending on template parameters)
template <size_t inputTypeSize, size_t loByteIndex, size_t hiByteIndex>
struct ReadBigIntTo16PCM : SampleConversionFunctor<char[inputTypeSize], int16, conversionHasNoState>
{
	inline int16 operator() (const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		return inBuf[loByteIndex] | (inBuf[hiByteIndex] << 8);
	}
};


// 32-Bit floating point to 16-Bit signed PCM sample conversion with clipping (for both little and big endian, depending on template parameters)
template <size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct ReadFloat32toInt16PCM : SampleConversionFunctor<float, int16, conversionHasNoState>
{
	inline int16 operator() (const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		const uint32 in32 = inBuf[loLoByteIndex] | (inBuf[loHiByteIndex] << 8) | (inBuf[hiLoByteIndex] << 16) | (inBuf[hiHiByteIndex] << 24);
		const bool negative = (in32 & 0x80000000) != 0;

		float val = *reinterpret_cast<const float *>(&in32);
		Limit(val, -1.0f, 1.0f);
		return static_cast<int16>(val * (negative ? -int16_min : int16_max));
	}
};


//////////////////////////////////////////////////////
// Sample normalization + conversion functors

// Signed integer PCM (up to 32-Bit) to 16-Big signed sample conversion with normalization. A second sample conversion functor is used for actual sample reading.
template <typename SampleConversion>
struct ReadBigIntToInt16PCMandNormalize : SampleConversionFunctor<typename SampleConversion::input_t, int16, conversionHasNoState>
{
	uint32 maxCandidate;
	double maxVal;
	SampleConversion sampleConv;

	ReadBigIntToInt16PCMandNormalize() : maxCandidate(0)
	{
		static_assert(SampleConversion::hasState == false, "Implementation of this conversion functor is stateless");
		static_assert(sizeof(SampleConversion::output_t) <= 4, "Implementation of this conversion functor is only suitable for 32-Bit integers or smaller");
	}

	inline void FindMax(const void *sourceBuffer)
	{
		int32 val = sampleConv(sourceBuffer);

		if(val < 0)
		{
			if(val == int32_min)
			{
				maxCandidate = static_cast<uint32>(-int32_min);
				return;
			}
			val = -val;
		}

		if(static_cast<uint32>(val) > maxCandidate)
		{
			maxCandidate = static_cast<uint32>(val);
		}
	}

	bool IsSilent()
	{
		maxVal = static_cast<double>(maxCandidate);
		return (maxCandidate == 0);
	}

	inline int16 operator() (const void *sourceBuffer)
	{
		int32 val = sampleConv(sourceBuffer);
		const double NC = (val < 0) ? 32768.0 : 32767.0;	// Normalization Constant
		const double roundBias = (val < 0) ? -0.5 : 0.5;
		return static_cast<int16>((static_cast<double>(val) / maxVal * NC) + roundBias);
	}
};


// 32-Bit floating point to 16-Bit signed PCM sample conversion with normalization (for both little and big endian, depending on template parameters)
template <size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct ReadFloat32to16PCMandNormalize : SampleConversionFunctor<float, int16, conversionHasNoState>
{
	union
	{
		float f;
		uint32 i;
	} maxVal;

	ReadFloat32to16PCMandNormalize() { maxVal.i = 0; }

	inline void FindMax(const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		uint32 val = inBuf[loLoByteIndex] | (inBuf[loHiByteIndex] << 8) | (inBuf[hiLoByteIndex] << 16) | (inBuf[hiHiByteIndex] << 24);
		val &= ~0x80000000;	// Remove sign for absolute value

		// IEEE float values are lexicographically ordered and can be compared when interpreted as integers.
		// So we won't bother with loading the float into a floating point register here if we already have it in an integer register.
		if(val > maxVal.i)
		{
			ASSERT(*reinterpret_cast<float *>(&val) > maxVal.f);
			maxVal.i = val;
		}
	}

	bool IsSilent() const
	{
		return (maxVal.i == 0);
	}

	inline int16 operator() (const void *sourceBuffer)
	{
		const uint8 *inBuf = static_cast<const uint8 *>(sourceBuffer);
		const uint32 in32 = inBuf[loLoByteIndex] | (inBuf[loHiByteIndex] << 8) | (inBuf[hiLoByteIndex] << 16) | (inBuf[hiHiByteIndex] << 24);
		const bool negative = (in32 & 0x80000000) != 0;

		const float val = (*reinterpret_cast<const float *>(&in32) / maxVal.f) * (negative ? 32768.0f : 32767.0f);
		ASSERT(val >= -32768.0f && val <= 32767.0f);
		return static_cast<int16>(val);
	}
};


//////////////////////////////////////////////////////
// Actual sample conversion functions


// Copy a sample data buffer.
// targetBuffer: Buffer in which the sample should be copied into.
// numSamples: Number of samples of size T that should be copied. targetBuffer is expected to be able to hold "numSampels * incTarget" samples.
// incTarget: Number of samples by which the target data pointer is increased each time.
// sourceBuffer: Buffer from which the samples should be read.
// sourceSize: Size of source buffer, in bytes.
// incSource: Number of samples by which the source data pointer is increased each time.
//
// Template arguments:
// SampleConversion: Functor of type SampleConversionFunctor to apply sample conversion (see above for existing functors).
//
// Returns:
// Number of bytes read (including skipped bytes caused by "incSource").
template <typename SampleConversion>
size_t CopySample(void *targetBuffer, size_t numSamples, size_t incTarget, const void *sourceBuffer, size_t sourceSize, size_t incSource)
//---------------------------------------------------------------------------------------------------------------------------------------
{
	SampleConversion::output_t *outBuf = static_cast<SampleConversion::output_t *>(targetBuffer);
	const SampleConversion::input_t *inBuf = static_cast<const SampleConversion::input_t *>(sourceBuffer);

	const size_t sampleSize = incSource * sizeof(SampleConversion::input_t);
	LimitMax(numSamples, sourceSize / sampleSize);

	SampleConversion sampleConv;
	for(size_t i = numSamples; i != 0; i--)
	{
		*outBuf = sampleConv(inBuf);
		outBuf += incTarget;
		inBuf += incSource;
	}

	return numSamples * sampleSize;
}


// Copy a mono sample data buffer.
template <typename SampleConversion>
size_t CopyMonoSample(ModSample &sample, const uint8 *sourceBuffer, size_t sourceSize)
//------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 1);
	ASSERT(sample.GetElementarySampleSize() == sizeof(SampleConversion::output_t));

	return CopySample<SampleConversion>(sample.pSample, sample.nLength, 1, sourceBuffer, sourceSize, 1);
}


// Copy a stereo interleaved sample data buffer.
template <typename SampleConversion>
size_t CopyStereoInterleavedSample(ModSample &sample, const uint8 *sourceBuffer, size_t sourceSize)
//-------------------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 2);
	ASSERT(sample.GetElementarySampleSize() == sizeof(SampleConversion::output_t));

	if(SampleConversion::hasState == conversionHasState)
	{
		// Functor has state: We have to load left and right channel independently
		// or else the state of the two channels is mixed.
		const size_t rightOffset = sizeof(SampleConversion::input_t);
		if(rightOffset > sourceSize)
		{
			// Can't even load one sample of the right channel...
			return 0;
		}

		// Read left channel
		CopySample<SampleConversion>(sample.pSample, sample.nLength, 2, sourceBuffer, sourceSize, 2);
		// Read right channel
		return CopySample<SampleConversion>(sample.pSample + sizeof(SampleConversion::output_t), sample.nLength, 2, sourceBuffer + rightOffset, sourceSize - rightOffset, 2);
	} else
	{
		// This is quicker (and smaller), but only possible if the functor does't care about what it actually processes:
		// Read both interleaved channels at once.
		return CopySample<SampleConversion>(sample.pSample, sample.nLength * 2, 1, sourceBuffer, sourceSize, 1);
	}
}


// Copy a stereo split sample data buffer.
template <typename SampleConversion>
size_t CopyStereoSplitSample(ModSample &sample, const uint8 *sourceBuffer, size_t sourceSize)
//-------------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 2);
	ASSERT(sample.GetElementarySampleSize() == sizeof(SampleConversion::output_t));

	// Read left channel
	CopySample<SampleConversion>(sample.pSample, sample.nLength, 2, sourceBuffer, sourceSize, 1);

	const size_t rightOffset = sample.nLength * sizeof(SampleConversion::input_t);
	if(rightOffset > sourceSize)
	{
		// Can't even load one sample of the right channel...
		return sourceSize;
	} else
	{
		// Read right channel
		return rightOffset + CopySample<SampleConversion>(sample.pSample + sizeof(SampleConversion::output_t), sample.nLength, 2, sourceBuffer + rightOffset, sourceSize - rightOffset, 1);
	}
}


// Copy a sample data buffer and normalize it. Requires slightly advanced sample conversion functor.
template<typename SampleConversion>
size_t CopyAndNormalizeSample(ModSample &sample, const uint8 *sourceBuffer, size_t sourceSize)
//------------------------------------------------------------------------------------------
{
	static_assert(SampleConversion::hasState == false, "Implementation of this conversion function is stateless");
	const size_t inSize = sizeof(SampleConversion::input_t);
	const size_t outSize = sizeof(SampleConversion::output_t);

	ASSERT(sample.GetElementarySampleSize() == outSize);

	size_t numSamples = sample.nLength * sample.GetNumChannels();
	LimitMax(numSamples, sourceSize / inSize);

	const SampleConversion::input_t *inBuf = reinterpret_cast<const SampleConversion::input_t *>(sourceBuffer);

	// Finding max value
	SampleConversion sampleConv;
	for(size_t i = numSamples; i != 0; i--)
	{
		sampleConv.FindMax(inBuf);
		inBuf++;
	}

	// If buffer is silent (maximum is 0), don't bother normalizing the sample - just keep the already silent buffer.
	if(!sampleConv.IsSilent())
	{
		// Copying buffer.
		SampleConversion::output_t *outBuf = reinterpret_cast<SampleConversion::output_t *>(sample.pSample);
		inBuf = reinterpret_cast<const SampleConversion::input_t *>(sourceBuffer);

		for(size_t i = numSamples; i != 0; i--)
		{
			*outBuf = sampleConv(inBuf);
			outBuf++;
			inBuf++;
		}
	}

	return numSamples * inSize;
}
