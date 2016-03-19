/*
 * Endianness.h
 * ------------
 * Purpose: Code for deadling with endianness.
 * Notes  : VC++ didn't like my compile-time endianness check - or rather, it didn't decide at compile time. ;_;
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <algorithm>
#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif

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
#if MPT_GCC_AT_LEAST(4,8,0)
#define MPT_bswap16 __builtin_bswap16
#endif
#if MPT_GCC_AT_LEAST(4,3,0)
#define MPT_bswap32 __builtin_bswap32
#define MPT_bswap64 __builtin_bswap64
#endif
#elif MPT_COMPILER_MSVC
#define MPT_bswap16 _byteswap_ushort
#define MPT_bswap32 _byteswap_ulong
#define MPT_bswap64 _byteswap_uint64
#endif

// catch system macros
#ifndef MPT_bswap16
#ifdef bswap16
#define MPT_bswap16 bswap16
#endif
#endif
#ifndef MPT_bswap32
#ifdef bswap32
#define MPT_bswap32 bswap32
#endif
#endif
#ifndef MPT_bswap64
#ifdef bswap64
#define MPT_bswap64 bswap64
#endif
#endif

// No intrinsics available
#ifndef MPT_bswap16
#define MPT_bswap16(x) (((x >> 8) & 0xFF) | ((x << 8) & 0xFF00))
#endif
#ifndef MPT_bswap32
#define MPT_bswap32(x) (((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24))
#endif
#ifndef MPT_bswap64
#define MPT_bswap64(x) \
	( uint64(0) \
		| (((x >>  0) & 0xff) << 56) \
		| (((x >>  8) & 0xff) << 48) \
		| (((x >> 16) & 0xff) << 40) \
		| (((x >> 24) & 0xff) << 32) \
		| (((x >> 32) & 0xff) << 24) \
		| (((x >> 40) & 0xff) << 16) \
		| (((x >> 48) & 0xff) <<  8) \
		| (((x >> 56) & 0xff) <<  0) \
	) \
/**/
#endif


// Deprecated. Use "SwapBytesXX" versions below.
#ifdef MPT_PLATFORM_BIG_ENDIAN
inline uint32 LittleEndian(uint32 x)	{ return MPT_bswap32(x); }
inline uint16 LittleEndianW(uint16 x)	{ return MPT_bswap16(x); }
#define BigEndian(x)					(x)
#define BigEndianW(x)					(x)
#else
inline uint32 BigEndian(uint32 x)	{ return MPT_bswap32(x); }
inline uint16 BigEndianW(uint16 x)	{ return MPT_bswap16(x); }
#define LittleEndian(x)				(x)
#define LittleEndianW(x)			(x)
#endif

#if defined(MPT_PLATFORM_BIG_ENDIAN)
#define MPT_bswap64le(x) MPT_bswap64(x)
#define MPT_bswap32le(x) MPT_bswap32(x)
#define MPT_bswap16le(x) MPT_bswap16(x)
#define MPT_bswap64be(x) (x)
#define MPT_bswap32be(x) (x)
#define MPT_bswap16be(x) (x)
#elif defined(MPT_PLATFORM_LITTLE_ENDIAN)
#define MPT_bswap64be(x) MPT_bswap64(x)
#define MPT_bswap32be(x) MPT_bswap32(x)
#define MPT_bswap16be(x) MPT_bswap16(x)
#define MPT_bswap64le(x) (x)
#define MPT_bswap32le(x) (x)
#define MPT_bswap16le(x) (x)
#endif

inline uint64 SwapBytesBE_(uint64 value) { return MPT_bswap64be(value); }
inline uint32 SwapBytesBE_(uint32 value) { return MPT_bswap32be(value); }
inline uint16 SwapBytesBE_(uint16 value) { return MPT_bswap16be(value); }
inline uint64 SwapBytesLE_(uint64 value) { return MPT_bswap64le(value); }
inline uint32 SwapBytesLE_(uint32 value) { return MPT_bswap32le(value); }
inline uint16 SwapBytesLE_(uint16 value) { return MPT_bswap16le(value); }
inline int64  SwapBytesBE_(int64  value) { return MPT_bswap64be(value); }
inline int32  SwapBytesBE_(int32  value) { return MPT_bswap32be(value); }
inline int16  SwapBytesBE_(int16  value) { return MPT_bswap16be(value); }
inline int64  SwapBytesLE_(int64  value) { return MPT_bswap64le(value); }
inline int32  SwapBytesLE_(int32  value) { return MPT_bswap32le(value); }
inline int16  SwapBytesLE_(int16  value) { return MPT_bswap16le(value); }

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
#define SwapBytesBE(value) MPT_DO { (value) = SwapBytesBE_((value)); } MPT_WHILE_0
#define SwapBytesLE(value) MPT_DO { (value) = SwapBytesLE_((value)); } MPT_WHILE_0

// Alternative, function-style syntax:
#define SwapBytesReturnBE(value) SwapBytesBE_((value))
#define SwapBytesReturnLE(value) SwapBytesLE_((value))

#undef MPT_bswap16le
#undef MPT_bswap32le
#undef MPT_bswap64le
#undef MPT_bswap16be
#undef MPT_bswap32be
#undef MPT_bswap64be
#undef MPT_bswap16
#undef MPT_bswap32
#undef MPT_bswap64


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
static forceinline uint64 EncodeIEEE754binary64(float64 f)
{
#if MPT_PLATFORM_IEEE_FLOAT
	STATIC_ASSERT(sizeof(uint64) == sizeof(float64));
	#if MPT_COMPILER_UNION_TYPE_ALIASES
		union {
			float64 f;
			uint64 i;
		} conv;
		conv.f = f;
		return conv.i;
	#else
		uint64 i = 0;
		std::memcpy(&i, &f, sizeof(float64));
		return i;
	#endif
#else
	#error "IEEE754 double precision float support is required (for now)"
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
static forceinline float64 DecodeIEEE754binary64(uint64 i)
{
#if MPT_PLATFORM_IEEE_FLOAT
	STATIC_ASSERT(sizeof(uint64) == sizeof(float64));
	#if MPT_COMPILER_UNION_TYPE_ALIASES
		union {
			uint64 i;
			float64 f;
		} conv;
		conv.i = i;
		return conv.f;
	#else
		float64 f = 0.0;
		std::memcpy(&f, &i, sizeof(float64));
		return f;
	#endif
#else
	#error "IEEE754 double precision float support is required (for now)"
#endif
}


// template parameters are byte indices corresponding to the individual bytes of iee754 in memory
template<std::size_t hihi, std::size_t hilo, std::size_t lohi, std::size_t lolo>
struct IEEE754binary32Emulated
{
private:
	typedef IEEE754binary32Emulated<hihi,hilo,lohi,lolo> self_t;
	mpt::byte bytes[4];
public:
	forceinline mpt::byte GetByte(std::size_t i) const
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
	forceinline explicit IEEE754binary32Emulated(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3)
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
		bytes[hihi] = static_cast<mpt::byte>(i >> 24);
		bytes[hilo] = static_cast<mpt::byte>(i >> 16);
		bytes[lohi] = static_cast<mpt::byte>(i >>  8);
		bytes[lolo] = static_cast<mpt::byte>(i >>  0);
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
template<std::size_t hihihi, std::size_t hihilo, std::size_t hilohi, std::size_t hilolo, std::size_t lohihi, std::size_t lohilo, std::size_t lolohi, std::size_t lololo>
struct IEEE754binary64Emulated
{
private:
	typedef IEEE754binary64Emulated<hihihi,hihilo,hilohi,hilolo,lohihi,lohilo,lolohi,lololo> self_t;
	mpt::byte bytes[8];
public:
	forceinline mpt::byte GetByte(std::size_t i) const
	{
		return bytes[i];
	}
	forceinline IEEE754binary64Emulated() { }
	forceinline explicit IEEE754binary64Emulated(float64 f)
	{
		SetInt64(EncodeIEEE754binary64(f));
	}
	forceinline explicit IEEE754binary64Emulated(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3, mpt::byte b4, mpt::byte b5, mpt::byte b6, mpt::byte b7)
	{
		bytes[0] = b0;
		bytes[1] = b1;
		bytes[2] = b2;
		bytes[3] = b3;
		bytes[4] = b4;
		bytes[5] = b5;
		bytes[6] = b6;
		bytes[7] = b7;
	}
	forceinline operator float64 () const
	{
		return DecodeIEEE754binary64(GetInt64());
	}
	forceinline self_t & SetInt64(uint64 i)
	{
		bytes[hihihi] = static_cast<mpt::byte>(i >> 56);
		bytes[hihilo] = static_cast<mpt::byte>(i >> 48);
		bytes[hilohi] = static_cast<mpt::byte>(i >> 40);
		bytes[hilolo] = static_cast<mpt::byte>(i >> 32);
		bytes[lohihi] = static_cast<mpt::byte>(i >> 24);
		bytes[lohilo] = static_cast<mpt::byte>(i >> 16);
		bytes[lolohi] = static_cast<mpt::byte>(i >>  8);
		bytes[lololo] = static_cast<mpt::byte>(i >>  0);
		return *this;
	}
	forceinline uint64 GetInt64() const
	{
		return 0u
			| (static_cast<uint64>(bytes[hihihi]) << 56)
			| (static_cast<uint64>(bytes[hihilo]) << 48)
			| (static_cast<uint64>(bytes[hilohi]) << 40)
			| (static_cast<uint64>(bytes[hilolo]) << 32)
			| (static_cast<uint64>(bytes[lohihi]) << 24)
			| (static_cast<uint64>(bytes[lohilo]) << 16)
			| (static_cast<uint64>(bytes[lolohi]) <<  8)
			| (static_cast<uint64>(bytes[lololo]) <<  0)
			;
	}
	forceinline bool operator == (const self_t &cmp) const
	{
		return true
			&& bytes[0] == cmp.bytes[0]
			&& bytes[1] == cmp.bytes[1]
			&& bytes[2] == cmp.bytes[2]
			&& bytes[3] == cmp.bytes[3]
			&& bytes[4] == cmp.bytes[4]
			&& bytes[5] == cmp.bytes[5]
			&& bytes[6] == cmp.bytes[6]
			&& bytes[7] == cmp.bytes[7]
			;
	}
	forceinline bool operator != (const self_t &cmp) const
	{
		return !(*this == cmp);
	}
};

namespace mpt {

template <> struct is_binary_safe<IEEE754binary32Emulated<0,1,2,3> > : public mpt::true_type { };
template <> struct is_binary_safe<IEEE754binary32Emulated<3,2,1,0> > : public mpt::true_type { };
template <> struct is_binary_safe<IEEE754binary64Emulated<0,1,2,3,4,5,6,7> > : public mpt::true_type { };
template <> struct is_binary_safe<IEEE754binary64Emulated<7,6,5,4,3,2,1,0> > : public mpt::true_type { };

} // namespace mpt

#if MPT_PLATFORM_IEEE_FLOAT

struct IEEE754binary32Native
{
private:
	float32 value;
public:
	forceinline mpt::byte GetByte(std::size_t i) const
	{
		#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
			return static_cast<mpt::byte>(EncodeIEEE754binary32(value) >> (i*8));
		#elif defined(MPT_PLATFORM_BIG_ENDIAN)
			return static_cast<mpt::byte>(EncodeIEEE754binary32(value) >> ((4-1-i)*8));
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
	forceinline explicit IEEE754binary32Native(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3)
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

struct IEEE754binary64Native
{
private:
	float64 value;
public:
	forceinline mpt::byte GetByte(std::size_t i) const
	{
		#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
			return static_cast<mpt::byte>(EncodeIEEE754binary64(value) >> (i*8));
		#elif defined(MPT_PLATFORM_BIG_ENDIAN)
			return static_cast<mpt::byte>(EncodeIEEE754binary64(value) >> ((8-1-i)*8));
		#else
			STATIC_ASSERT(false);
		#endif
	}
	forceinline IEEE754binary64Native() { }
	forceinline explicit IEEE754binary64Native(float64 f)
	{
		value = f;
	}
	forceinline explicit IEEE754binary64Native(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3, mpt::byte b4, mpt::byte b5, mpt::byte b6, mpt::byte b7)
	{
		#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
			value = DecodeIEEE754binary64(0ull
				| (static_cast<uint64>(b0) <<  0)
				| (static_cast<uint64>(b1) <<  8)
				| (static_cast<uint64>(b2) << 16)
				| (static_cast<uint64>(b3) << 24)
				| (static_cast<uint64>(b4) << 32)
				| (static_cast<uint64>(b5) << 40)
				| (static_cast<uint64>(b6) << 48)
				| (static_cast<uint64>(b7) << 56)
				);
		#elif defined(MPT_PLATFORM_BIG_ENDIAN)
			value = DecodeIEEE754binary64(0ull
				| (static_cast<uint64>(b0) << 56)
				| (static_cast<uint64>(b1) << 48)
				| (static_cast<uint64>(b2) << 40)
				| (static_cast<uint64>(b3) << 32)
				| (static_cast<uint64>(b4) << 24)
				| (static_cast<uint64>(b5) << 16)
				| (static_cast<uint64>(b6) <<  8)
				| (static_cast<uint64>(b7) <<  0)
				);
		#else
			STATIC_ASSERT(false);
		#endif
	}
	forceinline operator float64 () const
	{
		return value;
	}
	forceinline IEEE754binary64Native & SetInt64(uint64 i)
	{
		value = DecodeIEEE754binary64(i);
		return *this;
	}
	forceinline uint64 GetInt64() const
	{
		return EncodeIEEE754binary64(value);
	}
	forceinline bool operator == (const IEEE754binary64Native &cmp) const
	{
		return value == cmp.value;
	}
	forceinline bool operator != (const IEEE754binary64Native &cmp) const
	{
		return value != cmp.value;
	}
};

namespace mpt {

template <> struct is_binary_safe<IEEE754binary32Native> : public mpt::true_type { };
template <> struct is_binary_safe<IEEE754binary64Native> : public mpt::true_type { };

} // namespace mpt

#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
typedef IEEE754binary32Native                    IEEE754binary32LE;
typedef IEEE754binary32Emulated<0,1,2,3>         IEEE754binary32BE;
typedef IEEE754binary64Native                    IEEE754binary64LE;
typedef IEEE754binary64Emulated<0,1,2,3,4,5,6,7> IEEE754binary64BE;
#elif defined(MPT_PLATFORM_BIG_ENDIAN)
typedef IEEE754binary32Emulated<3,2,1,0>         IEEE754binary32LE;
typedef IEEE754binary32Native                    IEEE754binary32BE;
typedef IEEE754binary64Emulated<7,6,5,4,3,2,1,0> IEEE754binary64LE;
typedef IEEE754binary64Native                    IEEE754binary64BE;
#endif

#else // !MPT_PLATFORM_IEEE_FLOAT

typedef IEEE754binary32Emulated<3,2,1,0> IEEE754binary32LE;
typedef IEEE754binary32Emulated<0,1,2,3> IEEE754binary32BE;
typedef IEEE754binary64Emulated<7,6,5,4,3,2,1,0> IEEE754binary64LE;
typedef IEEE754binary64Emulated<0,1,2,3,4,5,6,7> IEEE754binary64BE;

#endif // MPT_PLATFORM_IEEE_FLOAT

STATIC_ASSERT(sizeof(IEEE754binary32LE) == 4);
STATIC_ASSERT(sizeof(IEEE754binary32BE) == 4);
STATIC_ASSERT(sizeof(IEEE754binary64LE) == 8);
STATIC_ASSERT(sizeof(IEEE754binary64BE) == 8);


// Small helper class to support unaligned memory access on all platforms.
// This is only used to make old module loaders work everywhere.
// Do not use in new code.
template <typename T>
class const_unaligned_ptr_le
{
public:
	typedef T value_type;
private:
	const mpt::byte *mem;
	value_type Read() const
	{
		mpt::byte bytes[sizeof(value_type)];
		std::memcpy(bytes, mem, sizeof(value_type));
		#if defined(MPT_PLATFORM_BIG_ENDIAN)
			std::reverse(bytes, bytes + sizeof(value_type));
		#endif
		value_type val = value_type();
		std::memcpy(&val, bytes, sizeof(value_type));
		return val;
	}
public:
	const_unaligned_ptr_le() : mem(nullptr) {}
	const_unaligned_ptr_le(const const_unaligned_ptr_le<value_type> & other) : mem(other.mem) {}
	const_unaligned_ptr_le & operator = (const const_unaligned_ptr_le<value_type> & other) { mem = other.mem; return *this; }
	explicit const_unaligned_ptr_le(const uint8 *mem) : mem(mem) {}
	explicit const_unaligned_ptr_le(const char *mem) : mem(mpt::byte_cast<const mpt::byte*>(mem)) {}
	const_unaligned_ptr_le & operator += (std::size_t count) { mem += count * sizeof(value_type); return *this; }
	const_unaligned_ptr_le & operator -= (std::size_t count) { mem -= count * sizeof(value_type); return *this; }
	const_unaligned_ptr_le & operator ++ () { mem += sizeof(value_type); return *this; }
	const_unaligned_ptr_le & operator -- () { mem -= sizeof(value_type); return *this; }
	const_unaligned_ptr_le operator ++ (int) { const_unaligned_ptr_le<value_type> result = *this; ++result; return result; }
	const_unaligned_ptr_le operator -- (int) { const_unaligned_ptr_le<value_type> result = *this; --result; return result; }
	const_unaligned_ptr_le operator + (std::size_t count) const { const_unaligned_ptr_le<value_type> result = *this; result += count; return result; }
	const_unaligned_ptr_le operator - (std::size_t count) const { const_unaligned_ptr_le<value_type> result = *this; result -= count; return result; }
	const value_type operator * () const { return Read(); }
	const value_type operator [] (std::size_t i) const { return *((*this) + i); }
	operator bool () const { return mem != nullptr; }
};


OPENMPT_NAMESPACE_END
