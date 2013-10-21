/*
 * SampleFormat.h
 * ---------------
 * Purpose: Utility enum and funcion to describe sample formats.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


enum SampleFormatEnum
{
	SampleFormatUnsigned8 =  8,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt16     = 16,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt24     = 24,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt32     = 32,       // do not change value (for compatibility with old configuration settings)
	SampleFormatFloat32   = 32 + 128, // do not change value (for compatibility with old configuration settings)
	SampleFormatInvalid   =  0
};

template<typename Tsample> struct SampleFormatTraits;
template<> struct SampleFormatTraits<uint8>     { static const SampleFormatEnum sampleFormat = SampleFormatUnsigned8; };
template<> struct SampleFormatTraits<int16>     { static const SampleFormatEnum sampleFormat = SampleFormatInt16;     };
template<> struct SampleFormatTraits<int24>     { static const SampleFormatEnum sampleFormat = SampleFormatInt24;     };
template<> struct SampleFormatTraits<int32>     { static const SampleFormatEnum sampleFormat = SampleFormatInt32;     };
template<> struct SampleFormatTraits<float>     { static const SampleFormatEnum sampleFormat = SampleFormatFloat32;   };

template<SampleFormatEnum sampleFormat> struct SampleFormatToType;
template<> struct SampleFormatToType<SampleFormatUnsigned8> { typedef uint8     type; };
template<> struct SampleFormatToType<SampleFormatInt16>     { typedef int16     type; };
template<> struct SampleFormatToType<SampleFormatInt24>     { typedef int24     type; };
template<> struct SampleFormatToType<SampleFormatInt32>     { typedef int32     type; };
template<> struct SampleFormatToType<SampleFormatFloat32>   { typedef float     type; };


struct SampleFormat
{
	SampleFormatEnum value;
	SampleFormat(SampleFormatEnum v = SampleFormatInvalid) : value(v) { }
	bool operator == (SampleFormat other) const { return value == other.value; }
	bool operator != (SampleFormat other) const { return value != other.value; }
	bool operator == (SampleFormatEnum other) const { return value == other; }
	bool operator != (SampleFormatEnum other) const { return value != other; }
	operator SampleFormatEnum () const
	{
		return value;
	}
	bool IsValid() const
	{
		return value != SampleFormatInvalid;
	}
	bool IsUnsigned() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatUnsigned8;
	}
	bool IsFloat() const
	{
		if(!IsValid()) return false;
		return value == SampleFormatFloat32;
	}
	bool IsInt() const
	{
		if(!IsValid()) return false;
		return value != SampleFormatFloat32;
	}
	uint8 GetBitsPerSample() const
	{
		if(!IsValid()) return 0;
		switch(value)
		{
		case SampleFormatUnsigned8:
			return 8;
			break;
		case SampleFormatInt16:
			return 16;
			break;
		case SampleFormatInt24:
			return 24;
			break;
		case SampleFormatInt32:
			return 32;
			break;
		case SampleFormatFloat32:
			return 32;
			break;
		default:
			return 0;
			break;
		}
	}

	// backward compatibility, conversion to/from integers
	operator int () const { return value; }
	SampleFormat(int v) : value(SampleFormatEnum(v)) { }
	operator long () const { return value; }
	SampleFormat(long v) : value(SampleFormatEnum(v)) { }
	operator unsigned int () const { return value; }
	SampleFormat(unsigned int v) : value(SampleFormatEnum(v)) { }
	operator unsigned long () const { return value; }
	SampleFormat(unsigned long v) : value(SampleFormatEnum(v)) { }
};
