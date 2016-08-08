/*
 * Endianness.h
 * ------------
 * Purpose: Code for deadling with endianness.
 * Notes  : (currently none)
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
// SwapBytesBE(x) and variants may be used either to:
// -Convert integer x, which is in big endian format (for example read from file),
//		to endian format of current architecture.
// -Convert value x from endian format of current architecture to big endian format.
// Similarly SwapBytesLE(x) converts known little endian format to format of current
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

namespace mpt { namespace detail {
// catch system macros
#ifndef MPT_bswap16
#ifdef bswap16
static forceinline uint16 mpt_bswap16(uint16 x) { return bswap16(x); }
#define MPT_bswap16 mpt::detail::mpt_bswap16
#endif
#endif
#ifndef MPT_bswap32
#ifdef bswap32
static forceinline uint32 mpt_bswap32(uint32 x) { return bswap32(x); }
#define MPT_bswap32 mpt::detail::mpt_bswap32
#endif
#endif
#ifndef MPT_bswap64
#ifdef bswap64
static forceinline uint64 mpt_bswap16(uint64 x) { return bswap64(x); }
#define MPT_bswap64 mpt::detail::mpt_bswap64
#endif
#endif
} } // namespace mpt::detail


// No intrinsics available
#ifndef MPT_bswap16
#define MPT_bswap16(x) \
	( uint16(0) \
		| ((static_cast<uint16>(x) >> 8) & 0x00FFu) \
		| ((static_cast<uint16>(x) << 8) & 0xFF00u) \
	) \
/**/
#endif
#ifndef MPT_bswap32
#define MPT_bswap32(x) \
	( uint32(0) \
		| ((static_cast<uint32>(x) & 0x000000FFu) << 24) \
		| ((static_cast<uint32>(x) & 0x0000FF00u) <<  8) \
		| ((static_cast<uint32>(x) & 0x00FF0000u) >>  8) \
		| ((static_cast<uint32>(x) & 0xFF000000u) >> 24) \
	) \
/**/
#endif
#ifndef MPT_bswap64
#define MPT_bswap64(x) \
	( uint64(0) \
		| (((static_cast<uint64>(x) >>  0) & 0xffu) << 56) \
		| (((static_cast<uint64>(x) >>  8) & 0xffu) << 48) \
		| (((static_cast<uint64>(x) >> 16) & 0xffu) << 40) \
		| (((static_cast<uint64>(x) >> 24) & 0xffu) << 32) \
		| (((static_cast<uint64>(x) >> 32) & 0xffu) << 24) \
		| (((static_cast<uint64>(x) >> 40) & 0xffu) << 16) \
		| (((static_cast<uint64>(x) >> 48) & 0xffu) <<  8) \
		| (((static_cast<uint64>(x) >> 56) & 0xffu) <<  0) \
	) \
/**/
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

inline uint64 SwapBytesBE(uint64 value) { return MPT_bswap64be(value); }
inline uint32 SwapBytesBE(uint32 value) { return MPT_bswap32be(value); }
inline uint16 SwapBytesBE(uint16 value) { return MPT_bswap16be(value); }
inline uint64 SwapBytesLE(uint64 value) { return MPT_bswap64le(value); }
inline uint32 SwapBytesLE(uint32 value) { return MPT_bswap32le(value); }
inline uint16 SwapBytesLE(uint16 value) { return MPT_bswap16le(value); }
inline int64  SwapBytesBE(int64  value) { return MPT_bswap64be(value); }
inline int32  SwapBytesBE(int32  value) { return MPT_bswap32be(value); }
inline int16  SwapBytesBE(int16  value) { return MPT_bswap16be(value); }
inline int64  SwapBytesLE(int64  value) { return MPT_bswap64le(value); }
inline int32  SwapBytesLE(int32  value) { return MPT_bswap32le(value); }
inline int16  SwapBytesLE(int16  value) { return MPT_bswap16le(value); }

// Do NOT remove these overloads, even if they seem useless.
// We do not want risking to extend 8bit integers to int and then
// endian-converting and casting back to int.
// Thus these overloads.
inline uint8  SwapBytesLE(uint8  value) { return value; }
inline int8   SwapBytesLE(int8   value) { return value; }
inline char   SwapBytesLE(char   value) { return value; }
inline uint8  SwapBytesBE(uint8  value) { return value; }
inline int8   SwapBytesBE(int8   value) { return value; }
inline char   SwapBytesBE(char   value) { return value; }

inline uint64 SwapBytes(uint64 value) { return MPT_bswap64(value); }
inline uint32 SwapBytes(uint32 value) { return MPT_bswap32(value); }
inline uint16 SwapBytes(uint16 value) { return MPT_bswap16(value); }
inline int64  SwapBytes(int64  value) { return MPT_bswap64(value); }
inline int32  SwapBytes(int32  value) { return MPT_bswap32(value); }
inline int16  SwapBytes(int16  value) { return MPT_bswap16(value); }
inline uint8  SwapBytes(uint8  value) { return value; }
inline int8   SwapBytes(int8   value) { return value; }
inline char   SwapBytes(char   value) { return value; }

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

typedef IEEE754binary32Emulated<0,1,2,3> IEEE754binary32EmulatedBE;
typedef IEEE754binary32Emulated<3,2,1,0> IEEE754binary32EmulatedLE;
typedef IEEE754binary64Emulated<0,1,2,3,4,5,6,7> IEEE754binary64EmulatedBE;
typedef IEEE754binary64Emulated<7,6,5,4,3,2,1,0> IEEE754binary64EmulatedLE;

MPT_BINARY_STRUCT(IEEE754binary32EmulatedBE, 4)
MPT_BINARY_STRUCT(IEEE754binary32EmulatedLE, 4)
MPT_BINARY_STRUCT(IEEE754binary64EmulatedBE, 8)
MPT_BINARY_STRUCT(IEEE754binary64EmulatedLE, 8)

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

MPT_BINARY_STRUCT(IEEE754binary32Native, 4)
MPT_BINARY_STRUCT(IEEE754binary64Native, 8)

#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
typedef IEEE754binary32Native                    IEEE754binary32LE;
typedef IEEE754binary32EmulatedBE                IEEE754binary32BE;
typedef IEEE754binary64Native                    IEEE754binary64LE;
typedef IEEE754binary64EmulatedBE                IEEE754binary64BE;
#elif defined(MPT_PLATFORM_BIG_ENDIAN)
typedef IEEE754binary32EmulatedLE                IEEE754binary32LE;
typedef IEEE754binary32Native                    IEEE754binary32BE;
typedef IEEE754binary64EmulatedLE                IEEE754binary64LE;
typedef IEEE754binary64Native                    IEEE754binary64BE;
#endif

#else // !MPT_PLATFORM_IEEE_FLOAT

typedef IEEE754binary32EmulatedLE IEEE754binary32LE;
typedef IEEE754binary32EmulatedBE IEEE754binary32BE;
typedef IEEE754binary64EmulatedLE IEEE754binary64LE;
typedef IEEE754binary64EmulatedBE IEEE754binary64BE;

#endif // MPT_PLATFORM_IEEE_FLOAT

STATIC_ASSERT(sizeof(IEEE754binary32LE) == 4);
STATIC_ASSERT(sizeof(IEEE754binary32BE) == 4);
STATIC_ASSERT(sizeof(IEEE754binary64LE) == 8);
STATIC_ASSERT(sizeof(IEEE754binary64BE) == 8);


///////////////////////////////////////////////////////////////////////////////
// On-disk integer types with defined endianness and no alignemnt requirements
// Note: To easily debug module loaders (and anything else that uses this
// wrapper struct), you can use the Debugger Visualizers available in
// build/vs/debug/ to conveniently view the wrapped contents.

namespace mpt { namespace detail {
enum Endianness
{
	BigEndian,
	LittleEndian,
#if defined(MPT_PLATFORM_BIG_ENDIAN)
	NativeEndian = BigEndian,
#else
	NativeEndian = LittleEndian,
#endif
};
} } // namespace mpt::detail

struct BigEndian_tag
{
	static const mpt::detail::Endianness Endianness = mpt::detail::BigEndian;
};

struct LittleEndian_tag
{
	static const mpt::detail::Endianness Endianness = mpt::detail::LittleEndian;
};

template<typename T, typename Tendian>
struct PACKED packed
{
public:
	typedef T base_type;
	typedef Tendian endian_type;
private:
	mpt::byte data[sizeof(base_type)];

public:
	forceinline void set(base_type val)
	{
		MPT_MAYBE_CONSTANT_IF(mpt::detail::NativeEndian != endian_type::Endianness)
		{
			val = SwapBytes(val);
		}
		std::memcpy(data, &val, sizeof(val));
	}
	forceinline base_type get() const
	{
		base_type val;
		std::memcpy(&val, data, sizeof(val));
		MPT_MAYBE_CONSTANT_IF(mpt::detail::NativeEndian != endian_type::Endianness)
		{
			val = SwapBytes(val);
		}
		return val;
	}
	forceinline packed & operator = (const base_type & val) { set(val); return *this; }
	forceinline operator base_type () const { return get(); }
public:
	packed & operator &= (base_type val) { set(get() & val); return *this; }
	packed & operator |= (base_type val) { set(get() | val); return *this; }
	packed & operator ^= (base_type val) { set(get() ^ val); return *this; }
	packed & operator += (base_type val) { set(get() + val); return *this; }
	packed & operator -= (base_type val) { set(get() - val); return *this; }
	packed & operator *= (base_type val) { set(get() * val); return *this; }
	packed & operator /= (base_type val) { set(get() / val); return *this; }
	packed & operator %= (base_type val) { set(get() % val); return *this; }
	packed & operator ++ () { set(get() + 1); return *this; } // prefix
	packed & operator -- () { set(get() - 1); return *this; } // prefix
	base_type operator ++ (int) { base_type old = get(); set(old + 1); return old; } // postfix
	base_type operator -- (int) { base_type old = get(); set(old - 1); return old; } // postfix
};

typedef packed< int64, LittleEndian_tag> int64le;
typedef packed< int32, LittleEndian_tag> int32le;
typedef packed< int16, LittleEndian_tag> int16le;
typedef packed< int8 , LittleEndian_tag> int8le;
typedef packed<uint64, LittleEndian_tag> uint64le;
typedef packed<uint32, LittleEndian_tag> uint32le;
typedef packed<uint16, LittleEndian_tag> uint16le;
typedef packed<uint8 , LittleEndian_tag> uint8le;

typedef packed< int64, BigEndian_tag> int64be;
typedef packed< int32, BigEndian_tag> int32be;
typedef packed< int16, BigEndian_tag> int16be;
typedef packed< int8 , BigEndian_tag> int8be;
typedef packed<uint64, BigEndian_tag> uint64be;
typedef packed<uint32, BigEndian_tag> uint32be;
typedef packed<uint16, BigEndian_tag> uint16be;
typedef packed<uint8 , BigEndian_tag> uint8be;

MPT_BINARY_STRUCT(int64le, 8)
MPT_BINARY_STRUCT(int32le, 4)
MPT_BINARY_STRUCT(int16le, 2)
MPT_BINARY_STRUCT(int8le , 1)
MPT_BINARY_STRUCT(uint64le, 8)
MPT_BINARY_STRUCT(uint32le, 4)
MPT_BINARY_STRUCT(uint16le, 2)
MPT_BINARY_STRUCT(uint8le , 1)

MPT_BINARY_STRUCT(int64be, 8)
MPT_BINARY_STRUCT(int32be, 4)
MPT_BINARY_STRUCT(int16be, 2)
MPT_BINARY_STRUCT(int8be , 1)
MPT_BINARY_STRUCT(uint64be, 8)
MPT_BINARY_STRUCT(uint32be, 4)
MPT_BINARY_STRUCT(uint16be, 2)
MPT_BINARY_STRUCT(uint8be , 1)


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

template <typename T>
class const_unaligned_ptr_be
{
public:
	typedef T value_type;
private:
	const mpt::byte *mem;
	value_type Read() const
	{
		mpt::byte bytes[sizeof(value_type)];
		std::memcpy(bytes, mem, sizeof(value_type));
		#if defined(MPT_PLATFORM_LITTLE_ENDIAN)
			std::reverse(bytes, bytes + sizeof(value_type));
		#endif
		value_type val = value_type();
		std::memcpy(&val, bytes, sizeof(value_type));
		return val;
	}
public:
	const_unaligned_ptr_be() : mem(nullptr) {}
	const_unaligned_ptr_be(const const_unaligned_ptr_be<value_type> & other) : mem(other.mem) {}
	const_unaligned_ptr_be & operator = (const const_unaligned_ptr_be<value_type> & other) { mem = other.mem; return *this; }
	explicit const_unaligned_ptr_be(const uint8 *mem) : mem(mem) {}
	explicit const_unaligned_ptr_be(const char *mem) : mem(mpt::byte_cast<const mpt::byte*>(mem)) {}
	const_unaligned_ptr_be & operator += (std::size_t count) { mem += count * sizeof(value_type); return *this; }
	const_unaligned_ptr_be & operator -= (std::size_t count) { mem -= count * sizeof(value_type); return *this; }
	const_unaligned_ptr_be & operator ++ () { mem += sizeof(value_type); return *this; }
	const_unaligned_ptr_be & operator -- () { mem -= sizeof(value_type); return *this; }
	const_unaligned_ptr_be operator ++ (int) { const_unaligned_ptr_be<value_type> result = *this; ++result; return result; }
	const_unaligned_ptr_be operator -- (int) { const_unaligned_ptr_be<value_type> result = *this; --result; return result; }
	const_unaligned_ptr_be operator + (std::size_t count) const { const_unaligned_ptr_be<value_type> result = *this; result += count; return result; }
	const_unaligned_ptr_be operator - (std::size_t count) const { const_unaligned_ptr_be<value_type> result = *this; result -= count; return result; }
	const value_type operator * () const { return Read(); }
	const value_type operator [] (std::size_t i) const { return *((*this) + i); }
	operator bool () const { return mem != nullptr; }
};

template <typename T>
class const_unaligned_ptr
{
public:
	typedef T value_type;
private:
	const mpt::byte *mem;
	value_type Read() const
	{
		value_type val = value_type();
		std::memcpy(&val, mem, sizeof(value_type));
		return val;
	}
public:
	const_unaligned_ptr() : mem(nullptr) {}
	const_unaligned_ptr(const const_unaligned_ptr<value_type> & other) : mem(other.mem) {}
	const_unaligned_ptr & operator = (const const_unaligned_ptr<value_type> & other) { mem = other.mem; return *this; }
	explicit const_unaligned_ptr(const uint8 *mem) : mem(mem) {}
	explicit const_unaligned_ptr(const char *mem) : mem(mpt::byte_cast<const mpt::byte*>(mem)) {}
	const_unaligned_ptr & operator += (std::size_t count) { mem += count * sizeof(value_type); return *this; }
	const_unaligned_ptr & operator -= (std::size_t count) { mem -= count * sizeof(value_type); return *this; }
	const_unaligned_ptr & operator ++ () { mem += sizeof(value_type); return *this; }
	const_unaligned_ptr & operator -- () { mem -= sizeof(value_type); return *this; }
	const_unaligned_ptr operator ++ (int) { const_unaligned_ptr<value_type> result = *this; ++result; return result; }
	const_unaligned_ptr operator -- (int) { const_unaligned_ptr<value_type> result = *this; --result; return result; }
	const_unaligned_ptr operator + (std::size_t count) const { const_unaligned_ptr<value_type> result = *this; result += count; return result; }
	const_unaligned_ptr operator - (std::size_t count) const { const_unaligned_ptr<value_type> result = *this; result -= count; return result; }
	const value_type operator * () const { return Read(); }
	const value_type operator [] (std::size_t i) const { return *((*this) + i); }
	operator bool () const { return mem != nullptr; }
};


OPENMPT_NAMESPACE_END

