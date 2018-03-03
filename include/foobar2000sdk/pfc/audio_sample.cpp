#include "pfc.h"

namespace pfc {
    audio_sample audio_math::decodeFloat24ptr(const void * sourcePtr) {
		PFC_STATIC_ASSERT(pfc::byte_order_is_little_endian);
		union {
			uint8_t bytes[4];
			float v;
		} u;
		const uint8_t * s = reinterpret_cast<const uint8_t*>(sourcePtr);
		u.bytes[0] = 0;
		u.bytes[1] = s[0];
		u.bytes[2] = s[1];
		u.bytes[3] = s[2];
		return u.v;
	}
	audio_sample audio_math::decodeFloat24ptrbs(const void * sourcePtr) {
		PFC_STATIC_ASSERT(pfc::byte_order_is_little_endian);
		union {
			uint8_t bytes[4];
			float v;
		} u;
		const uint8_t * s = reinterpret_cast<const uint8_t*>(sourcePtr);
		u.bytes[0] = 0;
		u.bytes[1] = s[2];
		u.bytes[2] = s[1];
		u.bytes[3] = s[0];
		return u.v;
	}

	audio_sample audio_math::decodeFloat16(uint16_t source) {
		const unsigned fractionBits = 10;
		const unsigned widthBits = 16;
		typedef uint16_t source_t;

	/*	typedef uint64_t out_t; typedef double retval_t;
		enum { 
			outExponent = 11, 
			outFraction = 52, 
			outExponentShift = (1 << (outExponent-1))-1 
		};*/

		typedef uint32_t out_t; typedef float retval_t;
		enum { 
			outExponent = 8, 
			outFraction = 23, 
			outExponentShift = (1 << (outExponent-1))-1 
		};

		const unsigned exponentBits = widthBits - fractionBits - 1;
		// 1 bit sign | exponent | fraction
		source_t fraction = source & (((source_t)1 << fractionBits)-1);
		source >>= fractionBits;
		int exponent = (int)( source & (((source_t)1 << exponentBits)-1) ) - (int)((1 << (exponentBits-1))-1);
		source >>= exponentBits;

		if (outExponent + outExponentShift <= 0) return 0;

		out_t output = (out_t)( source&1 );
		output <<= outExponent;
		output |= (unsigned) (exponent + outExponentShift) & ( (1<<outExponent) - 1 );
		output <<= outFraction;
		int shift = (int) outFraction - (int) fractionBits;
		if (shift < 0) output |= (out_t) (fraction >> -shift);
		else output |= (out_t) (fraction << shift);
		return *(retval_t*)&output / pfc::audio_math::float16scale;
	}

	unsigned audio_math::bitrate_kbps(uint64_t fileSize, double duration) {
		if (fileSize > 0 && duration > 0) return (unsigned)floor((double)fileSize * 8 / (duration * 1000) + 0.5);
		return 0;
	}
}
