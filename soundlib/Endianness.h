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

// Deprecated. Use "SwapBytesXX" versions below.
#ifdef PLATFORM_BIG_ENDIAN
// PPC
inline uint32 LittleEndian(uint32 x)	{ return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24); }
inline uint16 LittleEndianW(uint16 x)	{ return (uint16)(((x >> 8) & 0xFF) | ((x << 8) & 0xFF00)); }
#define BigEndian(x)					(x)
#define BigEndianW(x)					(x)
#else
// x86
inline uint32 BigEndian(uint32 x)	{ return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24); }
inline uint16 BigEndianW(uint16 x)	{ return (uint16)(((x >> 8) & 0xFF) | ((x << 8) & 0xFF00)); }
#define LittleEndian(x)				(x)
#define LittleEndianW(x)			(x)
#endif
//#pragma deprecated(BigEndian, BigEndianW, LittleEndian, LittleEndianW)

#ifdef PLATFORM_BIG_ENDIAN
// PPC
inline uint32 SwapBytesBE(uint32 &value)	{ return value; }
inline uint16 SwapBytesBE(uint16 &value)	{ return value; }
inline uint32 SwapBytesLE(uint32 &value)	{ return (value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24)); }
inline uint16 SwapBytesLE(uint16 &value)	{ return (value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00)); }
inline int32 SwapBytesBE(int32 &value)		{ return value; }
inline int16 SwapBytesBE(int16 &value)		{ return value; }
inline int32 SwapBytesLE(int32 &value)		{ return (value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24)); }
inline int16 SwapBytesLE(int16 &value)		{ return (value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00))); }
#else
// x86
inline uint32 SwapBytesBE(uint32 &value)	{ return (value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24)); }
inline uint16 SwapBytesBE(uint16 &value)	{ return (value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00))); }
inline uint32 SwapBytesLE(uint32 &value)	{ return value; }
inline uint16 SwapBytesLE(uint16 &value)	{ return value; }
inline int32 SwapBytesBE(int32 &value)		{ return (value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24)); }
inline int16 SwapBytesBE(int16 &value)		{ return (value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00))); }
inline int32 SwapBytesLE(int32 &value)		{ return value; }
inline int16 SwapBytesLE(int16 &value)		{ return value; }
#endif

