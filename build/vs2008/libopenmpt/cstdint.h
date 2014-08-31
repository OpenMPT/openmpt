#pragma once
#if (_MSC_VER >= 1500) && (_MSC_VER < 1600) // VS2008
#include "stdint.h"
namespace std {
	typedef int8_t   int8_t;
	typedef int16_t  int16_t;
	typedef int32_t  int32_t;
	typedef int64_t  int64_t;
	typedef uint8_t  uint8_t;
	typedef uint16_t uint16_t;
	typedef uint32_t uint32_t;
	typedef uint64_t uint64_t;
}
#endif
