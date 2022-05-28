/*
 * mptTime.h
 * ---------
 * Purpose: Various time utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#if MPT_CXX_AT_LEAST(20) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO_DATE)
#include <chrono>
#endif
#include <string>

#if MPT_CXX_BEFORE(20) || defined(MPT_LIBCXX_QUIRK_NO_CHRONO_DATE)
#include <time.h>
#endif

#define MPT_TIME_CTIME

#if defined(MPT_TIME_CTIME)
#include <time.h>
#endif


OPENMPT_NAMESPACE_BEGIN



namespace mpt
{
namespace Date
{

#if defined(MODPLUG_TRACKER)

#if MPT_OS_WINDOWS

namespace ANSI
{
// uint64 counts 100ns since 1601-01-01T00:00Z

uint64 Now();

mpt::ustring ToUString(uint64 time100ns); // i.e. 2015-01-15 18:32:01.718

} // namespacee ANSI

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER

enum class LogicalTimezone
{
	Unspecified,
	UTC,
#if defined(MODPLUG_TRACKER)
	Local,
#endif // MODPLUG_TRACKER
};

template <LogicalTimezone tz>
struct Gregorian
{
	int          year    = 0;
	unsigned int month   = 0;
	unsigned int day     = 0;
	int32        hours   = 0;
	int32        minutes = 0;
	int64        seconds = 0;
	friend bool operator==(const Gregorian<tz>& lhs, const Gregorian<tz>& rhs)
	{
		return true
			&& lhs.year == rhs.year
			&& lhs.month == rhs.month
			&& lhs.day == rhs.day
			&& lhs.hours == rhs.hours
			&& lhs.minutes == rhs.minutes
			&& lhs.seconds == rhs.seconds
			;
	}
	friend bool operator!=(const Gregorian<tz>& lhs, const Gregorian<tz>& rhs)
	{
		return false
			|| lhs.year != rhs.year
			|| lhs.month != rhs.month
			|| lhs.day != rhs.day
			|| lhs.hours != rhs.hours
			|| lhs.minutes != rhs.minutes
			|| lhs.seconds != rhs.seconds
			;
	}
};

using AnyGregorian = Gregorian<LogicalTimezone::Unspecified>;

#if defined(MPT_TIME_CTIME)
inline tm AsTm(AnyGregorian val)
{
	tm result{};
	result.tm_year = val.year - 1900;
	result.tm_mon = val.month - 1;
	result.tm_mday = val.day;
	result.tm_hour = val.hours;
	result.tm_min = val.minutes;
	result.tm_sec = static_cast<int>(val.seconds);
	return result;
}
inline AnyGregorian AsGregorian(tm val)
{
	AnyGregorian result{};
	result.year = val.tm_year + 1900;
	result.month = val.tm_mon + 1;
	result.day = val.tm_mday;
	result.hours = val.tm_hour;
	result.minutes = val.tm_min;
	result.seconds = val.tm_sec;
	return result;
}
#endif

using UTC = Gregorian<LogicalTimezone::UTC>;

#if defined(MODPLUG_TRACKER)
using Local = Gregorian<LogicalTimezone::Local>;
#endif // MODPLUG_TRACKER

#if MPT_CXX_AT_LEAST(20) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO_DATE)

using Unix = std::chrono::system_clock::time_point;

inline Unix UnixNow()
{
	return std::chrono::system_clock::now();
}

inline int64 UnixAsSeconds(Unix tp)
{
	return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

inline Unix UnixFromSeconds(int64 seconds)
{
	return std::chrono::system_clock::time_point{std::chrono::seconds{seconds}};
}

inline mpt::Date::Unix UnixFromUTC(UTC utc)
{
	std::chrono::year_month_day ymd =
		std::chrono::year{utc.year} /
		std::chrono::month{utc.month} /
		std::chrono::day{utc.day};
	std::chrono::hh_mm_ss<std::chrono::seconds> hms{
		std::chrono::hours{utc.hours} +
		std::chrono::minutes{utc.minutes} +
		std::chrono::seconds{utc.seconds}};
	return std::chrono::system_clock::time_point{static_cast<std::chrono::sys_days>(ymd)} + hms.to_duration();
}

inline mpt::Date::UTC UnixAsUTC(Unix tp)
{
	std::chrono::sys_days dp = std::chrono::floor<std::chrono::days>(tp);
	std::chrono::year_month_day ymd{dp};
	std::chrono::hh_mm_ss hms{tp - dp};
	mpt::Date::UTC result;
	result.year = static_cast<int>(ymd.year());
	result.month = static_cast<unsigned int>(ymd.month());
	result.day = static_cast<unsigned int>(ymd.day());
	result.hours = hms.hours().count();
	result.minutes = hms.minutes().count();
	result.seconds = hms.seconds().count();
	return result;
}

#else

// int64 counts 1s since 1970-01-01T00:00Z
struct Unix
{
	int64 value{};
	friend bool operator==(const Unix &a, const Unix &b)
	{
		return a.value == b.value;
	}
	friend bool operator!=(const Unix &a, const Unix &b)
	{
		return a.value != b.value;
	}
};

inline Unix UnixNow()
{
	return Unix{static_cast<int64>(time(nullptr))};
}

inline int64 UnixAsSeconds(Unix tp)
{
	return tp.value;
}

inline Unix UnixFromSeconds(int64 seconds)
{
	return Unix{seconds};
}

mpt::Date::Unix UnixFromUTC(UTC timeUtc);

mpt::Date::UTC UnixAsUTC(Unix tp);

#endif

mpt::ustring ToShortenedISO8601(AnyGregorian date); // i.e. 2015-01-15T18:32:01

mpt::ustring ToShortenedISO8601(UTC date); // i.e. 2015-01-15T18:32:01Z

#if defined(MPT_TIME_CTIME)

mpt::Date::Unix UnixFromUTCtm(tm timeUtc);

tm UnixAsUTCtm(mpt::Date::Unix unixtime);

mpt::ustring ToShortenedISO8601(tm date); // i.e. 2015-01-15T18:32:01Z

#endif

} // namespace Date
} // namespace mpt



#ifdef MODPLUG_TRACKER

namespace Util
{

#if MPT_OS_WINDOWS

// RAII wrapper around timeBeginPeriod/timeEndPeriod/timeGetTime (on Windows).
// This clock is monotonic, even across changing its resolution.
// This is needed to synchronize time in Steinberg APIs (ASIO and VST).
class MultimediaClock
{
private:
	uint32 m_CurrentPeriod;
private:
	void Init();
	void SetPeriod(uint32 ms);
	void Cleanup();
public:
	MultimediaClock();
	MultimediaClock(uint32 ms);
	~MultimediaClock();
public:
	// Sets the desired resolution in milliseconds, returns the obtained resolution in milliseconds.
	// A parameter of 0 causes the resolution to be reset to system defaults.
	// A return value of 0 means the resolution is unknown, but timestamps will still be valid.
	uint32 SetResolution(uint32 ms);
	// Returns obtained resolution in milliseconds.
	// A return value of 0 means the resolution is unknown, but timestamps will still be valid.
	uint32 GetResolution() const; 
	// Returns current instantaneous timestamp in milliseconds.
	// The epoch (offset) of the timestamps is undefined but constant until the next system reboot.
	// The resolution is the value returned from GetResolution().
	uint32 Now() const;
	// Returns current instantaneous timestamp in nanoseconds.
	// The epoch (offset) of the timestamps is undefined but constant until the next system reboot.
	// The resolution is the value returned from GetResolution() in milliseconds.
	uint64 NowNanoseconds() const;
};

#endif // MPT_OS_WINDOWS

} // namespace Util

#endif // MODPLUG_TRACKER



OPENMPT_NAMESPACE_END
