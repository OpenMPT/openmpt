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
inline DWORD LittleEndian(DWORD x)	{ return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24); }
inline WORD LittleEndianW(WORD x)	{ return (WORD)(((x >> 8) & 0xFF) | ((x << 8) & 0xFF00)); }
#define BigEndian(x)				(x)
#define BigEndianW(x)				(x)
#else
// x86
inline DWORD BigEndian(DWORD x)	{ return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | ((x & 0xFF000000) >> 24); }
inline WORD BigEndianW(WORD x)	{ return (WORD)(((x >> 8) & 0xFF) | ((x << 8) & 0xFF00)); }
#define LittleEndian(x)			(x)
#define LittleEndianW(x)		(x)
#endif

#ifdef PLATFORM_BIG_ENDIAN
// PPC
inline void SwapBytesBE(uint32 &value)	{ value; }
inline void SwapBytesBE(uint16 &value)	{ value; }
inline void SwapBytesLE(uint32 &value)	{ value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24); }
inline void SwapBytesLE(uint16 &value)	{ value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00)); }
inline void SwapBytesBE(int32 &value)	{ value; }
inline void SwapBytesBE(int16 &value)	{ value; }
inline void SwapBytesLE(int32 &value)	{ value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24); }
inline void SwapBytesLE(int16 &value)	{ value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00)); }
#else
// x86
inline void SwapBytesBE(uint32 &value)	{ value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24); }
inline void SwapBytesBE(uint16 &value)	{ value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00)); }
inline void SwapBytesLE(uint32 &value)	{ value; }
inline void SwapBytesLE(uint16 &value)	{ value; }
inline void SwapBytesBE(int32 &value)	{ value = ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24); }
inline void SwapBytesBE(int16 &value)	{ value = (((value >> 8) & 0xFF) | ((value << 8) & 0xFF00)); }
inline void SwapBytesLE(int32 &value)	{ value; }
inline void SwapBytesLE(int16 &value)	{ value; }
#endif

