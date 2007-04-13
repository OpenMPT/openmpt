#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <limits>

#ifdef WIN32
	typedef __int8 int8;
	typedef __int16 int16;
	typedef __int32 int32;
	typedef __int64 int64;

	typedef unsigned __int8 uint8;
	typedef unsigned __int16 uint16;
	typedef unsigned __int32 uint32;
	typedef size_t uint64;

	typedef float float32;
#endif //End WIN32 specific.

typedef uint32 ROWINDEX;
typedef uint16 CHANNELINDEX;
typedef uint16 ORDERINDEX;
typedef uint16 PATTERNINDEX;
typedef uint16 TEMPO;
typedef uint16 SAMPLEINDEX;
typedef uint16 INSTRUMENTINDEX;

const ORDERINDEX ORDERINDEX_MAX = (std::numeric_limits<ORDERINDEX>::max)();
const ROWINDEX ROWINDEX_MAX = (std::numeric_limits<ROWINDEX>::max)();


#endif
