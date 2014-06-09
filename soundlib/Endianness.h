/*
 * Endianness.h
 * ------------
 * Purpose: Code for deadling with endianness.
 * Notes  : VC++ didn't like my compile-time endianness check - or rather, it didn't decide at compile time. ;_;
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

// Ending swaps:
// BigEndian(x) may be used either to:
// -Convert DWORD x, which is in big endian format(for example read from file),
//		to endian format of current architecture.
// -Convert value x from endian format of current architecture to big endian format.
// Similarly LittleEndian(x) converts known little endian format to format of current
// endian architecture or value x in format of current architecture to little endian 
// format.

#if MPT_COMPILER_GCC
#if MPT_GCC_AT_LEAST(4,3,0)
#define bswap32 __builtin_bswap32
#endif
#elif MPT_COMPILER_MSVC
#include <intrin.h>
#define bswap16 _byteswap_ushort
#define bswap32 _byteswap_ulong
#endif

// No intrinsics available
#ifndef bswap16
#define bswap16(x) (((x >> 8) & 0xFF) | ((x << 8) & 0xFF00))
#endif
#ifndef bswap32
#define bswap32(x) (((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24))
#endif

// Deprecated. Use "SwapBytesXX" versions below.
#ifdef MPT_PLATFORM_BIG_ENDIAN
inline uint32 LittleEndian(uint32 x)	{ return bswap32(x); }
inline uint16 LittleEndianW(uint16 x)	{ return bswap16(x); }
#define BigEndian(x)					(x)
#define BigEndianW(x)					(x)
#else
inline uint32 BigEndian(uint32 x)	{ return bswap32(x); }
inline uint16 BigEndianW(uint16 x)	{ return bswap16(x); }
#define LittleEndian(x)				(x)
#define LittleEndianW(x)			(x)
#endif

#if defined(MPT_PLATFORM_BIG_ENDIAN)
#define bswap32le(x) bswap32(x)
#define bswap16le(x) bswap16(x)
#define bswap32be(x) (x)
#define bswap16be(x) (x)
#elif defined(MPT_PLATFORM_LITTLE_ENDIAN)
#define bswap32be(x) bswap32(x)
#define bswap16be(x) bswap16(x)
#define bswap32le(x) (x)
#define bswap16le(x) (x)
#endif

inline uint32 SwapBytesBE_(uint32 value) { return bswap32be(value); }
inline uint16 SwapBytesBE_(uint16 value) { return bswap16be(value); }
inline uint32 SwapBytesLE_(uint32 value) { return bswap32le(value); }
inline uint16 SwapBytesLE_(uint16 value) { return bswap16le(value); }
inline int32  SwapBytesBE_(int32  value) { return bswap32be(value); }
inline int16  SwapBytesBE_(int16  value) { return bswap16be(value); }
inline int32  SwapBytesLE_(int32  value) { return bswap32le(value); }
inline int16  SwapBytesLE_(int16  value) { return bswap16le(value); }

// Do NOT remove these overloads, even if they seem useless.
// We do not want risking to extend 8bit integers to int and then
// endian-converting and casting back to int.
// Thus these overloads.
inline uint8  SwapBytesLE_(uint8  value) { return value; }
inline int8   SwapBytesLE_(int8   value) { return value; }
inline char   SwapBytesLE_(char   value) { return value; }
inline uint8  SwapBytesBE_(uint8  value) { return value; }
inline int8   SwapBytesBE_(int8   value) { return value; }
inline char   SwapBytesBE_(char   value) { return value; }

// SwapBytesLE/SwapBytesBE is mostly used throughout the code with in-place
// argument-modifying semantics.
// As GCC will not bind references to members of packed structures,
// we implement reference semantics by explicitly assigning to a macro
// argument.

// In-place modifying version:
#define SwapBytesBE(value) do { (value) = SwapBytesBE_((value)); } while(0)
#define SwapBytesLE(value) do { (value) = SwapBytesLE_((value)); } while(0)

// Alternative, function-style syntax:
#define SwapBytesReturnBE(value) SwapBytesBE_((value))
#define SwapBytesReturnLE(value) SwapBytesLE_((value))

#undef bswap16le
#undef bswap32le
#undef bswap16be
#undef bswap32be
#undef bswap16
#undef bswap32


// 1.0f --> 0x3f800000u
static forceinline uint32 EncodeIEEE754binary32(float32 f)
{
#if MPT_PLATFORM_IEEE_FLOAT
	STATIC_ASSERT(sizeof(uint32) == sizeof(float32));
	#if MPT_COMPILER_UNION_TYPE_ALIASES
		union {
			float32 f;
			uint32 i;
		} conv;
		conv.f = f;
		return conv.i;
	#else
		uint32 i = 0;
		std::memcpy(&i, &f, sizeof(float32));
		return i;
	#endif
#else
	#error "IEEE754 single precision float support is required (for now)"
#endif
}

// 0x3f800000u --> 1.0f
static forceinline float32 DecodeIEEE754binary32(uint32 i)
{
#if MPT_PLATFORM_IEEE_FLOAT
	STATIC_ASSERT(sizeof(uint32) == sizeof(float32));
	#if MPT_COMPILER_UNION_TYPE_ALIASES
		union {
			uint32 i;
			float32 f;
		} conv;
		conv.i = i;
		return conv.f;
	#else
		float32 f = 0.0f;
		std::memcpy(&f, &i, sizeof(float32));
		return f;
	#endif
#else
	#error "IEEE754 single precision float support is required (for now)"
#endif
}


// template parameters are byte indices corresponding to the individual bytes of iee754 in memory
template<std::size_t hihi, std::size_t hilo, std::size_t lohi, std::size_t lolo>
struct IEEE754binary32Emulated
{
private:
	typedef IEEE754binary32Emulated<hihi,hilo,lohi,lolo> self_t;
	uint8 bytes[4];
public:
	forceinline uint8 GetByte(std::size_t i) const
	{
		return bytes[i];
	}
	forceinline IEEE754binary32Emulated() { }
	forceinline explicit IEEE754binary32Emulated(float32 f)
	{
		SetInt32(EncodeIEEE754binary32(f));
	}
	// b0...b3 are in memory order, i.e. depend on the endianness of this type
	// little endian: (0x00,0x00,0x80,0x3f)
	// big endian:    (0x3f,0x80,0x00,0x00)
	forceinline explicit IEEE754binary32Emulated(uint8 b0, uint8 b1, uint8 b2, uint8 b3)
	{
		bytes[0] = b0;
		bytes[1] = b1;
		bytes[2] = b2;
		bytes[3] = b3;
	}
	forceinline operator float32 () const
	{
		return DecodeIEEE754binary32(GetInt32());
	}
	forceinline self_t & SetInt32(uint32 i)
	{
		bytes[hihi] = static_cast<uint8>(i >> 24);
		bytes[hilo] = static_cast<uint8>(i >> 16);
		bytes[lohi] = static_cast<uint8>(i >>  8);
		bytes[lolo] = static_cast<uint8>(i >>  0);
		return *this;
	}
	forceinline uint32 GetInt32() const
	{
		return 0u
			| (static_cast<uint32>(bytes[hihi]) << 24)
			| (static_cast<uint32>(bytes[hilo]) << 16)
			| (static_cast<uint32>(bytes[lohi]) <<  8)
			| (static_cast<uint32>(bytes[lolo]) <<  0)
			;
	}
	forceinline bool operator == (const self_t &cmp) const
	{
		return true
			&& bytes[0] == cmp.bytes[0]
			&& bytes[1] == cmp.bytes[1]
			&& bytes[2] == cmp.bytes[2]
			&& bytes[3] == cmp.bytes[3]
			;
	}
	forceinline bool operator != (const self_t &cmp) const
	{
		return !(*this == cmp);
	}
};

#if MPT_PLATFORM_IEEE_FLOAT

struct IEEE754binary32Native
{
private:
	float32 value;
public:
	forceinline uint8 GetByte(std::size_t i) const
	{
		#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
			return static_cast<uint8>(EncodeIEEE754binary32(value) >> (i*8));
		#elif defined(MPT_PLATFORM_BIG_ENDIAN)
			return static_cast<uint8>(EncodeIEEE754binary32(value) >> ((4-1-i)*8));
		#else
			STATIC_ASSERT(false);
		#endif
	}
	forceinline IEEE754binary32Native() { }
	forceinline explicit IEEE754binary32Native(float32 f)
	{
		value = f;
	}
	// b0...b3 are in memory order, i.e. depend on the endianness of this type
	// little endian: (0x00,0x00,0x80,0x3f)
	// big endian:    (0x3f,0x80,0x00,0x00)
	forceinline explicit IEEE754binary32Native(uint8 b0, uint8 b1, uint8 b2, uint8 b3)
	{
		#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
			value = DecodeIEEE754binary32(0u
				| (static_cast<uint32>(b0) <<  0)
				| (static_cast<uint32>(b1) <<  8)
				| (static_cast<uint32>(b2) << 16)
				| (static_cast<uint32>(b3) << 24)
				);
		#elif defined(MPT_PLATFORM_BIG_ENDIAN)
			value = DecodeIEEE754binary32(0u
				| (static_cast<uint32>(b0) << 24)
				| (static_cast<uint32>(b1) << 16)
				| (static_cast<uint32>(b2) <<  8)
				| (static_cast<uint32>(b3) <<  0)
				);
		#else
			STATIC_ASSERT(false);
		#endif
	}
	forceinline operator float32 () const
	{
		return value;
	}
	forceinline IEEE754binary32Native & SetInt32(uint32 i)
	{
		value = DecodeIEEE754binary32(i);
		return *this;
	}
	forceinline uint32 GetInt32() const
	{
		return EncodeIEEE754binary32(value);
	}
	forceinline bool operator == (const IEEE754binary32Native &cmp) const
	{
		return value == cmp.value;
	}
	forceinline bool operator != (const IEEE754binary32Native &cmp) const
	{
		return value != cmp.value;
	}
};

#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
typedef IEEE754binary32Native            IEEE754binary32LE;
typedef IEEE754binary32Emulated<0,1,2,3> IEEE754binary32BE;
#elif defined(MPT_PLATFORM_BIG_ENDIAN)
typedef IEEE754binary32Emulated<3,2,1,0> IEEE754binary32LE;
typedef IEEE754binary32Native            IEEE754binary32BE;
#endif

#else // !MPT_PLATFORM_IEEE_FLOAT

typedef IEEE754binary32Emulated<3,2,1,0> IEEE754binary32LE;
typedef IEEE754binary32Emulated<0,1,2,3> IEEE754binary32BE;

#endif // MPT_PLATFORM_IEEE_FLOAT

STATIC_ASSERT(sizeof(IEEE754binary32LE) == 4);
STATIC_ASSERT(sizeof(IEEE754binary32BE) == 4);

OPENMPT_NAMESPACE_END
