/*
 * typedefs.cpp
 * ------------
 * Purpose: Basic data type definitions and assorted compiler-related helpers.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "typedefs.h"
#include "Logging.h"


#if !defined(MODPLUG_TRACKER) && defined(MPT_ASSERT_HANDLER_NEEDED)

noinline void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//--------------------------------------------------------------------------------------------------------------
{
	if(msg)
	{
		mpt::log::Logger(mpt::log::Context(file, line, function))("ASSERTION FAILED: %s (%s)", msg, expr);
	} else
	{
		mpt::log::Logger(mpt::log::Context(file, line, function))("ASSERTION FAILED: %s", expr);
	}
}

#endif // !MODPLUG_TRACKER &&  MPT_ASSERT_HANDLER_NEEDED

