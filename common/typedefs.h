/*
 * typedefs.h
 * ----------
 * Purpose: Basic data type definitions and assorted compiler-related helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once



#include "mptAlloc.h"
#include "mptAssert.h"
#include "mptBaseMacros.h"
#include "mptBaseTypes.h"
#include "mptException.h"



OPENMPT_NAMESPACE_BEGIN



// legacy
#if MPT_COMPILER_MSVC
OPENMPT_NAMESPACE_END
#include <cstdlib>
OPENMPT_NAMESPACE_BEGIN
#define MPT_ARRAY_COUNT(x) _countof(x)
#else
#define MPT_ARRAY_COUNT(x) (sizeof((x))/sizeof((x)[0]))
#endif
#define CountOf(x) MPT_ARRAY_COUNT(x)



OPENMPT_NAMESPACE_END
