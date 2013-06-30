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



namespace SC { // SC = _S_ample_C_onversion



// Every sample conversion functor has to typedef its input_t and output_t
// and has to provide a static const input_inc member
// which describes by how many input_t elements inBuf has to be incremented between invocations.
// input_inc is normally 1 except when decoding e.g. bigger sample values
// from multiple char values.
// Another possible use for input_inc could be reading only one channel of
// an interleaved multi-channel sample (although there is currently no functor to do this).



struct DecodeInt7
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return Clamp(int8(*inBuf), int8(-64), int8(63)) * 2;
	}
};

struct DecodeInt8
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int8(*inBuf);
	}
};

struct DecodeUint8
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int8(int(uint8(*inBuf)) - 128);
	}
};

struct DecodeInt8Delta
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	uint8 delta;
	DecodeInt8Delta() : delta(0) { }
	forceinline output_t operator() (const input_t *inBuf)
	{
		delta += uint8(*inBuf);
		return int8(delta);
	}
};

template <int16 offset, size_t loByteIndex, size_t hiByteIndex>
struct DecodeInt16
{
	typedef char input_t;
	typedef int16 output_t;
	static const int input_inc = 2;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return (uint8(inBuf[loByteIndex]) | (uint8(inBuf[hiByteIndex]) << 8)) - offset;
	}
};

template <size_t loByteIndex, size_t hiByteIndex>
struct DecodeInt16Delta
{
	typedef char input_t;
	typedef int16 output_t;
	static const int input_inc = 2;
	uint16 delta;
	DecodeInt16Delta() : delta(0) { }
	forceinline output_t operator() (const input_t *inBuf)
	{
		delta += uint8(inBuf[loByteIndex]) | (uint8(inBuf[hiByteIndex]) << 8);
		return int16(delta);
	}
};

struct DecodeInt16Delta8
{
	typedef char input_t;
	typedef int16 output_t;
	static const int input_inc = 2;
	uint16 delta;
	DecodeInt16Delta8() : delta(0) { }
	forceinline output_t operator() (const input_t *inBuf)
	{
		delta += uint8(inBuf[0]);
		int16 result = delta & 0xFF;
		delta += uint8(inBuf[1]);
		result |= (delta << 8);
		return result;
	}
};

template <int32 offset, size_t loByteIndex, size_t midByteIndex, size_t hiByteIndex>
struct DecodeInt24
{
	typedef char input_t;
	typedef int32 output_t;
	static const int input_inc = 3;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return ((uint8(inBuf[loByteIndex]) << 8) | (uint8(inBuf[midByteIndex]) << 16) | (uint8(inBuf[hiByteIndex]) << 24)) - offset;
	}
};

template <int32 offset, size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeInt32
{
	typedef char input_t;
	typedef int32 output_t;
	static const int input_inc = 4;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return (uint8(inBuf[loLoByteIndex]) | (uint8(inBuf[loHiByteIndex]) << 8) | (uint8(inBuf[hiLoByteIndex]) << 16) | (uint8(inBuf[hiHiByteIndex]) << 24)) - offset;
	}
};

template <size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeFloat32
{
	typedef char input_t;
	typedef float32 output_t;
	static const int input_inc = 4;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return DecodeFloatLE(uint8_4(uint8(inBuf[loLoByteIndex]), uint8(inBuf[loHiByteIndex]), uint8(inBuf[hiLoByteIndex]), uint8(inBuf[hiHiByteIndex])));
	}
};

template <typename Tsample>
struct ReadSample
{
	typedef Tsample input_t;
	typedef Tsample output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t * inBuf)
	{
		return *inBuf;
	}
};



template <typename Tdst, typename Tsrc, int shift>
struct ConvertShift
{
	typedef Tsrc input_t;
	typedef Tdst output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t * inBuf)
	{
		return mpt::saturate_cast<output_t>(*inBuf >> shift);
	}
};



template <typename Tdst, typename Tsrc>
struct Convert;

template <typename Tid>
struct Convert<Tid, Tid>
{
	typedef Tid input_t;
	typedef Tid output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t * inBuf)
	{
		return *inBuf;
	}
};

template <>
struct Convert<int8, int16>
{
	typedef int16 input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int8(*inBuf >> 8);
	}
};

template <>
struct Convert<int8, int32>
{
	typedef int32 input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int8(*inBuf >> 24);
	}
};

template <>
struct Convert<int8, float32>
{
	typedef float32 input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		float val = *inBuf;
		Limit(val, -1.0f, 1.0f);
		val *= 128.0f;
		// MSVC with x87 floating point math calls floor for the more intuitive version
		// return mpt::saturate_cast<int8>(static_cast<int>(std::floor(val + 0.5f)));
		return mpt::saturate_cast<int8>(static_cast<int>(val * 2.0f + 1.0f) >> 1);
	}
};

template <>
struct Convert<int16, int8>
{
	typedef int8 input_t;
	typedef int16 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int16(*inBuf << 8);
	}
};

template <>
struct Convert<int16, int32>
{
	typedef int32 input_t;
	typedef int16 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int16(*inBuf >> 16);
	}
};

template <>
struct Convert<int16, float32>
{
	typedef float32 input_t;
	typedef int16 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		float val = *inBuf;
		Limit(val, -1.0f, 1.0f);
		val *= 32768.0f;
		// MSVC with x87 floating point math calls floor for the more intuitive version
		// return mpt::saturate_cast<int16>(static_cast<int>(std::floor(val + 0.5f)));
		return mpt::saturate_cast<int16>(static_cast<int>(val * 2.0f + 1.0f) >> 1);
	}
};



// Reads sample data with Func and passes it directly to Func2.
// Func1::output_t and Func2::input_t must be identical
template <typename Func2, typename Func1>
struct ConversionChain
{
	typedef typename Func1::input_t input_t;
	typedef typename Func2::output_t output_t;
	static const int input_inc = Func1::input_inc;
	Func1 func1;
	Func2 func2;
	forceinline output_t operator() (const input_t *inBuf)
	{
		typename Func1::output_t tmp = func1(inBuf);
		return func2(&tmp);
	}
};



template <typename Tsample>
struct Normalize;

template <>
struct Normalize<int32>
{
	typedef int32 input_t;
	typedef int32 output_t;
	static const int input_inc = 1;
	uint32 maxVal;
	Normalize() : maxVal(0) { }
	forceinline void FindMax(const input_t *inBuf)
	{
		int32 val = *inBuf;
		if(val < 0)
		{
			if(val == int32_min)
			{
				maxVal = static_cast<uint32>(-static_cast<int64>(int32_min));
				return;
			}
			val = -val;
		}
		if(static_cast<uint32>(val) > maxVal)
		{
			maxVal = static_cast<uint32>(val);
		}
	}
	bool IsSilent() const
	{
		return maxVal == 0;
	}
	forceinline output_t operator() (const input_t *inBuf)
	{
		return Util::muldivrfloor(*inBuf, (uint32)1 << 31, maxVal);
	}
};

template <>
struct Normalize<float32>
{
	typedef float32 input_t;
	typedef float32 output_t;
	static const int input_inc = 1;
	uint32 intMaxVal;
	float maxValInv;
	Normalize() : intMaxVal(0), maxValInv(1.0f) { }
	forceinline void FindMax(const input_t *inBuf)
	{
		uint32 val = EncodeFloatNE(*inBuf);
		val &= ~0x80000000;	// Remove sign for absolute value

		// IEEE float values are lexicographically ordered and can be compared when interpreted as integers.
		// So we won't bother with loading the float into a floating point register here if we already have it in an integer register.
		if(val > intMaxVal)
		{
			ASSERT(*reinterpret_cast<float *>(&val) > DecodeFloatLE(uint8_4().SetLE(intMaxVal)));
			intMaxVal = val;
		}
	}
	bool IsSilent()
	{
		if(intMaxVal == 0)
		{
			return true;
		} else
		{
			maxValInv = 1.0f / DecodeFloatNE(intMaxVal);
			return false;
		}
	}
	forceinline output_t operator() (const input_t *inBuf)
	{
		return *inBuf * maxValInv;
	}
};



// Reads sample data with Func1, then normalizes the sample data, and then converts it with Func2.
// Func1::output_t and Func2::input_t must be identical.
// Func1 or Func2 can also be the identity transform (ReadSample<T>).
template <typename Func2, typename Func1>
struct NormalizationChain
{
	typedef typename Func1::input_t input_t;
	typedef typename Func2::output_t output_t;
	static const int input_inc = Func1::input_inc;
	Func1 func1;
	Normalize<typename Func1::output_t> normalize;
	Func2 func2;
	forceinline void FindMax(const input_t *inBuf)
	{
		typename Func1::output_t tmp = func1(inBuf);
		normalize.FindMax(&tmp);
	}
	bool IsSilent()
	{
		return normalize.IsSilent();
	}
	forceinline output_t operator() (const input_t *inBuf)
	{
		typename Func1::output_t tmp1 = func1(inBuf);
		typename Func1::output_t norm = normalize(&tmp1);
		typename Func2::output_t tmp2 = func2(&norm);
		return tmp2;
	}
};



} // namespace SC



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
template <typename SampleConversion>
size_t CopySample(typename SampleConversion::output_t *outBuf, size_t numSamples, size_t incTarget, const typename SampleConversion::input_t *inBuf, size_t sourceSize, size_t incSource)
//---------------------------------------------------------------------------------------------------------------------------------------
{
	const size_t sampleSize = incSource * SampleConversion::input_inc;
	LimitMax(numSamples, sourceSize / sampleSize);
	const size_t copySize = numSamples * sampleSize;

	SampleConversion sampleConv;
	while(numSamples--)
	{
		*outBuf = sampleConv(inBuf);
		outBuf += incTarget;
		inBuf += incSource * SampleConversion::input_inc;
	}

	return copySize;
}


// Copy a mono sample data buffer.
template <typename SampleConversion>
size_t CopyMonoSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize)
//------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 1);
	ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	const size_t frameSize =  SampleConversion::input_inc;
	const size_t countFrames = std::min<size_t>(sourceSize / frameSize, sample.nLength);
	size_t numFrames = countFrames;
	SampleConversion sampleConv;
	const char * inBuf = sourceBuffer;
	typename SampleConversion::output_t * outBuf = reinterpret_cast<typename SampleConversion::output_t *>(sample.pSample);
	while(numFrames--)
	{
		*outBuf = sampleConv(inBuf);
		inBuf += SampleConversion::input_inc;
		outBuf++;
	}
	return frameSize * countFrames;
}


// Copy a stereo interleaved sample data buffer.
template <typename SampleConversion>
size_t CopyStereoInterleavedSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize)
//-------------------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 2);
	ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	const size_t frameSize = 2 * SampleConversion::input_inc;
	const size_t countFrames = std::min<size_t>(sourceSize / frameSize, sample.nLength);
	size_t numFrames = countFrames;
	SampleConversion sampleConvLeft;
	SampleConversion sampleConvRight;
	const char * inBuf = sourceBuffer;
	typename SampleConversion::output_t * outBuf = reinterpret_cast<typename SampleConversion::output_t *>(sample.pSample);
	while(numFrames--)
	{
		*outBuf = sampleConvLeft(inBuf);
		inBuf += SampleConversion::input_inc;
		outBuf++;
		*outBuf = sampleConvRight(inBuf);
		inBuf += SampleConversion::input_inc;
		outBuf++;
	}
	return frameSize * countFrames;
}


// Copy a stereo split sample data buffer.
template <typename SampleConversion>
size_t CopyStereoSplitSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize)
//-------------------------------------------------------------------------------------------
{
	ASSERT(sample.GetNumChannels() == 2);
	ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	const size_t frameSize = 2 * SampleConversion::input_inc;
	const size_t countFrames = std::min<size_t>(sourceSize / frameSize, sample.nLength);
	size_t numFrames = countFrames;
	SampleConversion sampleConvLeft;
	SampleConversion sampleConvRight;
	const char * inBufLeft = sourceBuffer;
	const char * inBufRight = sourceBuffer + sample.nLength * SampleConversion::input_inc;
	typename SampleConversion::output_t * outBuf = reinterpret_cast<typename SampleConversion::output_t *>(sample.pSample);
	while(numFrames--)
	{
		*outBuf = sampleConvLeft(inBufLeft);
		inBufLeft += SampleConversion::input_inc;
		outBuf++;
		*outBuf = sampleConvRight(inBufRight);
		inBufRight += SampleConversion::input_inc;
		outBuf++;
	}
	return frameSize * countFrames;
}


// Copy a sample data buffer and normalize it. Requires slightly advanced sample conversion functor.
template<typename SampleConversion>
size_t CopyAndNormalizeSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize)
//--------------------------------------------------------------------------------------------
{
	const size_t inSize = sizeof(typename SampleConversion::input_t);

	ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	size_t numSamples = sample.nLength * sample.GetNumChannels();
	LimitMax(numSamples, sourceSize / inSize);

	const char * inBuf = sourceBuffer;
	// Finding max value
	SampleConversion sampleConv;
	for(size_t i = numSamples; i != 0; i--)
	{
		sampleConv.FindMax(inBuf);
		inBuf += SampleConversion::input_inc;
	}

	// If buffer is silent (maximum is 0), don't bother normalizing the sample - just keep the already silent buffer.
	if(!sampleConv.IsSilent())
	{
		const char * inBuf = sourceBuffer;
		// Copying buffer.
		typename SampleConversion::output_t *outBuf = static_cast<typename SampleConversion::output_t *>(sample.pSample);

		for(size_t i = numSamples; i != 0; i--)
		{
			*outBuf = sampleConv(inBuf);
			outBuf++;
			inBuf += SampleConversion::input_inc;
		}
	}

	return numSamples * inSize;
}
