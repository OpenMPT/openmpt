/*
 * Endianness.h
 * ------------
 * Purpose: Code for deadling with endianness.
 * Notes  : VC++ didn't like my compile-time endianness check - or rather, it didn't decide at compile time. ;_;
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

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
#ifdef PLATFORM_BIG_ENDIAN
// PPC
inline uint32 LittleEndian(uint32 x)	{ return bswap32(x); }
inline uint16 LittleEndianW(uint16 x)	{ return bswap16(x); }
#define BigEndian(x)					(x)
#define BigEndianW(x)					(x)
#else
// x86
inline uint32 BigEndian(uint32 x)	{ return bswap32(x); }
inline uint16 BigEndianW(uint16 x)	{ return bswap16(x); }
#define LittleEndian(x)				(x)
#define LittleEndianW(x)			(x)
#endif
//#pragma deprecated(BigEndian, BigEndianW, LittleEndian, LittleEndianW)

typedef uint32 ALIGN(1) unaligned_uint32;
typedef uint16 ALIGN(1) unaligned_uint16;
typedef  int32 ALIGN(1) unaligned_int32;
typedef  int16 ALIGN(1) unaligned_int16;

#ifdef PLATFORM_BIG_ENDIAN
// PPC
inline uint32 SwapBytesBE_(unaligned_uint32 *value) { return *value; }
inline uint16 SwapBytesBE_(unaligned_uint16 *value) { return *value; }
inline uint32 SwapBytesLE_(unaligned_uint32 *value) { return *value = bswap32(*value); }
inline uint16 SwapBytesLE_(unaligned_uint16 *value) { return *value = bswap16(*value); }
inline int32 SwapBytesBE_(unaligned_int32 *value)  { return *value; }
inline int16 SwapBytesBE_(unaligned_int16 *value)  { return *value; }
inline int32 SwapBytesLE_(unaligned_int32 *value)  { return *value = bswap32(*value); }
inline int16 SwapBytesLE_(unaligned_int16 *value)  { return *value = bswap16(*value); }
#else
// x86
inline uint32 SwapBytesBE_(unaligned_uint32 *value) { return *value = bswap32(*value); }
inline uint16 SwapBytesBE_(unaligned_uint16 *value) { return *value = bswap16(*value); }
inline uint32 SwapBytesLE_(unaligned_uint32 *value) { return *value; }
inline uint16 SwapBytesLE_(unaligned_uint16 *value) { return *value; }
inline int32 SwapBytesBE_(unaligned_int32 *value)  { return *value = bswap32(*value); }
inline int16 SwapBytesBE_(unaligned_int16 *value)  { return *value = bswap16(*value); }
inline int32 SwapBytesLE_(unaligned_int32 *value)  { return *value; }
inline int16 SwapBytesLE_(unaligned_int16 *value)  { return *value; }
#endif
// GCC will not bind references to members of packed structures, workaround it by using a raw pointer.
// This is a temporary solution as this pointer might of course be unaligned which GCC seems to not care about in that case.
#define SwapBytesBE(value) SwapBytesBE_(&(value))
#define SwapBytesLE(value) SwapBytesLE_(&(value))

#undef bswap16
#undef bswap32
