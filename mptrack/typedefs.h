#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <limits>

#define nullptr		0

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

const int8 int8_min	= (std::numeric_limits<int8>::min)();
const int16 int16_min = (std::numeric_limits<int16>::min)();
const int32 int32_min = (std::numeric_limits<int32>::min)();
const int64 int64_min = (std::numeric_limits<int64>::min)();

const int8 int8_max = (std::numeric_limits<int8>::max)();
const int16 int16_max = (std::numeric_limits<int16>::max)();
const int32 int32_max = (std::numeric_limits<int32>::max)();
const int64 int64_max = (std::numeric_limits<int64>::max)();

const uint8 uint8_max = (std::numeric_limits<uint8>::max)();
const uint16 uint16_max = (std::numeric_limits<uint16>::max)();
const uint32 uint32_max = (std::numeric_limits<uint32>::max)();
const uint64 uint64_max = (std::numeric_limits<uint64>::max)();

typedef float float32;


typedef uint32 ROWINDEX;
typedef uint16 CHANNELINDEX;
typedef uint16 ORDERINDEX;
typedef uint16 PATTERNINDEX;
typedef uint8  PLUGINDEX;
typedef uint16 TEMPO;
typedef uint16 SAMPLEINDEX;
typedef uint16 INSTRUMENTINDEX;
typedef uint32 MODTYPE;

const ORDERINDEX ORDERINDEX_MAX	= (std::numeric_limits<ORDERINDEX>::max)();
const ROWINDEX ROWINDEX_MAX = (std::numeric_limits<ROWINDEX>::max)();


#endif
