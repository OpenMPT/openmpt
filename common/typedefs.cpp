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


OPENMPT_NAMESPACE_BEGIN


#if !defined(MODPLUG_TRACKER) && defined(MPT_ASSERT_HANDLER_NEEDED)

MPT_NOINLINE void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//------------------------------------------------------------------------------------------------------------------
{
	if(msg)
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, msg) + MPT_USTRING(" (") + mpt::ToUnicode(mpt::CharsetASCII, expr) + MPT_USTRING(")")
			);
	} else
	{
		mpt::log::Logger().SendLogMessage(mpt::log::Context(file, line, function), LogError, "ASSERT",
			MPT_USTRING("ASSERTION FAILED: ") + mpt::ToUnicode(mpt::CharsetASCII, expr)
			);
	}
}

#endif // !MODPLUG_TRACKER &&  MPT_ASSERT_HANDLER_NEEDED


OPENMPT_NAMESPACE_END
