/*
 * mptLog.h
 * --------
 * Purpose: Logging interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "mptBuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

enum LogLevel
{
	LogDebug        = 5,
	LogInformation  = 4,
	LogNotification = 3,
	LogWarning      = 2,
	LogError        = 1
};

namespace mpt
{
namespace log
{

class ILogger
{
protected:
	virtual ~ILogger() = default;
public:
	virtual bool IsLevelActive(LogLevel level) const noexcept = 0;
	// facility:ASCII
	virtual bool IsFacilityActive(const char *facility) const noexcept = 0;
	// facility:ASCII
	virtual void SendLogMessage(const mpt::source_location &loc, LogLevel level, const char *facility, const mpt::ustring &text) const = 0;
};

#define MPT_LOG(logger, level, facility, text) \
	do \
	{ \
		if((logger).IsLevelActive((level))) \
		{ \
			if((logger).IsFacilityActive((facility))) \
			{ \
				(logger).SendLogMessage(MPT_SOURCE_LOCATION_CURRENT(), (level), (facility), (text)); \
			} \
		} \
	} while(0) \
/**/

#define MPT_LOG_GLOBAL(level, facility, text) MPT_LOG(mpt::log::GlobalLogger{}, (level), (facility), (text))

} // namespace log
} // namespace mpt

OPENMPT_NAMESPACE_END
