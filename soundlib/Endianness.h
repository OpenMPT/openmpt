#ifndef ENDIANNESS_H
#define ENDIANNESS_H

// Ending swaps:
// BigEndian(x) may be used either to:
// -Convert DWORD x, which is in big endian format(for example read from file),
//		to endian format of current architecture.
// -Convert value x from endian format of current architecture to big endian format.
// Similarly LittleEndian(x) converts known little endian format to format of current
// endian architecture or value x in format of current architecture to little endian 
// format.
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

#endif // ENDIANNESS_H

