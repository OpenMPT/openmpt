#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#define nullptr		0

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

const int8 int8_min	    = -127-1;
const int16 int16_min   = -32767-1;
const int32 int32_min   = -2147483647-1;
const int64 int64_min   = -9223372036854775807-1;

const int8 int8_max     = 127;
const int16 int16_max   = 32767;
const int32 int32_max   = 2147483647;
const int64 int64_max   = 9223372036854775807;

const uint8 uint8_max   = 255;
const uint16 uint16_max = 65535;
const uint32 uint32_max = 4294967295;
const uint64 uint64_max = 18446744073709551615;

typedef float float32;


typedef uint32 ROWINDEX;
	const ROWINDEX ROWINDEX_MAX = uint32_max;
typedef uint16 CHANNELINDEX;
typedef uint16 ORDERINDEX;
	const ORDERINDEX ORDERINDEX_MAX	= uint16_max;
	const ORDERINDEX ORDERINDEX_INVALID	= ORDERINDEX_MAX;
typedef uint16 PATTERNINDEX;
	const PATTERNINDEX PATTERNINDEX_MAX	= uint16_max;
	const PATTERNINDEX PATTERNINDEX_INVALID	= PATTERNINDEX_MAX;
typedef uint8  PLUGINDEX;
typedef uint16 TEMPO;
typedef uint16 SAMPLEINDEX;
typedef uint16 INSTRUMENTINDEX;
typedef uint32 MODTYPE;


#endif
