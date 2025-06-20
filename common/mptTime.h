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

#include "mpt/chrono/system_clock.hpp"
#include "mpt/chrono/unix_clock.hpp"

#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <chrono>
#endif
#if MPT_CXX_AT_LEAST(20) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO_DATE)
#include <exception>
#endif



#if MPT_WINNT_AT_LEAST(MPT_WIN_8)
#define MPT_FALLBACK_TIMEZONE_WINDOWS_HISTORIC
#define MPT_FALLBACK_TIMEZONE_WINDOWS_CURRENT
#define MPT_FALLBACK_TIMEZONE_C
#elif MPT_WINNT_AT_LEAST(MPT_WIN_XP)
#define MPT_FALLBACK_TIMEZONE_WINDOWS_CURRENT
#define MPT_FALLBACK_TIMEZONE_C
#else
#define MPT_FALLBACK_TIMEZONE_C
#endif



#if defined(MODPLUG_TRACKER) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)

namespace mpt {
inline namespace MPT_INLINE_NS {
namespace chrono {
#if MPT_CXX_AT_LEAST(20)
using days = std::chrono::days;
using weeks = std::chrono::weeks;
using years = std::chrono::years;
using months = std::chrono::months;
#else
using days = std::chrono::duration<int, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;
using weeks = std::chrono::duration<int, std::ratio_multiply<std::ratio<7>, mpt::chrono::days::period>>;
using years = std::chrono::duration<int, std::ratio_multiply<std::ratio<146097, 400>, mpt::chrono::days::period>>;
using months = std::chrono::duration<int, std::ratio_divide<mpt::chrono::years::period, std::ratio<12>>>;
#endif
}
}
}

#endif // !MPT_LIBCXX_QUIRK_NO_CHRONO



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



namespace Date
{



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
	int          year        = 0;
	unsigned int month       = 0;
	unsigned int day         = 0;
	int32        hours       = 0;
	int32        minutes     = 0;
	int32        seconds     = 0;
	int32        nanoseconds = 0;
	friend bool operator==(const Gregorian<tz>& lhs, const Gregorian<tz>& rhs)
	{
		return std::tie(lhs.year, lhs.month, lhs.day, lhs.hours, lhs.minutes, lhs.seconds, lhs.nanoseconds)
			== std::tie(rhs.year, rhs.month, rhs.day, rhs.hours, rhs.minutes, rhs.seconds, rhs.nanoseconds);
	}
	friend bool operator!=(const Gregorian<tz>& lhs, const Gregorian<tz>& rhs)
	{
		return std::tie(lhs.year, lhs.month, lhs.day, lhs.hours, lhs.minutes, lhs.seconds, lhs.nanoseconds)
			!= std::tie(rhs.year, rhs.month, rhs.day, rhs.hours, rhs.minutes, rhs.seconds, rhs.nanoseconds);
	}
	friend bool operator<(const Gregorian<tz> &lhs, const Gregorian<tz> &rhs)
	{
		return std::tie(lhs.year, lhs.month, lhs.day, lhs.hours, lhs.minutes, lhs.seconds, lhs.nanoseconds)
			< std::tie(rhs.year, rhs.month, rhs.day, rhs.hours, rhs.minutes, rhs.seconds, rhs.nanoseconds);
	}
};

using AnyGregorian = Gregorian<LogicalTimezone::Unspecified>;

using UTC = Gregorian<LogicalTimezone::UTC>;

#if defined(MODPLUG_TRACKER)
using Local = Gregorian<LogicalTimezone::Local>;
#endif // MODPLUG_TRACKER

template <LogicalTimezone TZ>
inline Gregorian<TZ> interpret_as_timezone(AnyGregorian gregorian)
{
	Gregorian<TZ> result;
	result.year = gregorian.year;
	result.month = gregorian.month;
	result.day = gregorian.day;
	result.hours = gregorian.hours;
	result.minutes = gregorian.minutes;
	result.seconds = gregorian.seconds;
	result.nanoseconds = gregorian.nanoseconds;
	return result;
}

template <LogicalTimezone TZ>
inline Gregorian<LogicalTimezone::Unspecified> forget_timezone(Gregorian<TZ> gregorian)
{
	Gregorian<LogicalTimezone::Unspecified> result;
	result.year = gregorian.year;
	result.month = gregorian.month;
	result.day = gregorian.day;
	result.hours = gregorian.hours;
	result.minutes = gregorian.minutes;
	result.seconds = gregorian.seconds;
	result.nanoseconds = gregorian.nanoseconds;
	return result;
}



} // namespace Date



namespace Date
{



mpt::chrono::unix_clock::time_point unix_from_UTC(mpt::Date::UTC timeUtc);

mpt::Date::UTC UTC_from_unix(mpt::chrono::unix_clock::time_point tp);

#if defined(MODPLUG_TRACKER)

mpt::chrono::unix_clock::time_point unix_from_local(mpt::Date::Local timeLocal);

mpt::Date::Local local_from_unix(mpt::chrono::unix_clock::time_point tp);

#endif // MODPLUG_TRACKER



#if MPT_CXX_AT_LEAST(20) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO_DATE)

inline mpt::chrono::system_clock::time_point system_from_UTC(mpt::Date::UTC utc)
{
	try
	{
		return std::chrono::system_clock::time_point{
			std::chrono::sys_days {
				std::chrono::year{ utc.year } /
				std::chrono::month{ utc.month } /
				std::chrono::day{ utc.day }
			} +
			std::chrono::hours{ utc.hours } +
			std::chrono::minutes{ utc.minutes } +
			std::chrono::seconds{ utc.seconds } +
			std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds{ utc.nanoseconds })};
	} catch(const std::exception &)
	{
		return mpt::chrono::system_clock::from_unix_nanoseconds(mpt::chrono::unix_clock::to_unix_nanoseconds(mpt::Date::unix_from_UTC(utc)));
	}
}

inline mpt::Date::UTC UTC_from_system(mpt::chrono::system_clock::time_point tp)
{
	try
	{
		std::chrono::sys_days dp = std::chrono::floor<std::chrono::days>(tp);
		std::chrono::year_month_day ymd{dp};
		std::chrono::hh_mm_ss hms{tp - dp};
		mpt::Date::UTC result;
		result.year = static_cast<int>(ymd.year());
		result.month = static_cast<unsigned int>(ymd.month());
		result.day = static_cast<unsigned int>(ymd.day());
		result.hours = static_cast<int32>(hms.hours().count());
		result.minutes = static_cast<int32>(hms.minutes().count());
		result.seconds = static_cast<int32>(hms.seconds().count());
		result.nanoseconds = static_cast<int32>(std::chrono::duration_cast<std::chrono::nanoseconds>(hms.subseconds()).count());
		return result;
	} catch(const std::exception &)
	{
		return mpt::Date::UTC_from_unix(mpt::chrono::unix_clock::from_unix_nanoseconds(mpt::chrono::system_clock::to_unix_nanoseconds(tp)));
	}
}

#if defined(MODPLUG_TRACKER)

inline mpt::chrono::system_clock::time_point system_from_local(mpt::Date::Local local)
{
#if !defined(MPT_LIBCXX_QUIRK_CHRONO_DATE_NO_ZONED_TIME)
	try
	{
		std::chrono::time_point<std::chrono::local_t, std::chrono::system_clock::duration> local_tp =
			std::chrono::local_days {
				std::chrono::year{ local.year } /
				std::chrono::month{ local.month } /
				std::chrono::day{ local.day }
			} +
			std::chrono::hours{ local.hours } +
			std::chrono::minutes{ local.minutes } +
			std::chrono::seconds{ local.seconds } +
			std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds{ local.nanoseconds });
#if defined(MPT_LIBCXX_QUIRK_CHRONO_DATE_BROKEN_ZONED_TIME)
		return std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::current_zone()->to_sys(local_tp)}.get_sys_time();
#else
		return std::chrono::zoned_time{std::chrono::current_zone(), local_tp}.get_sys_time();
#endif
	} catch(const std::exception &)
#endif
	{
		return mpt::chrono::system_clock::from_unix_nanoseconds(mpt::chrono::unix_clock::to_unix_nanoseconds(mpt::Date::unix_from_local(local)));
	}
}

inline mpt::Date::Local local_from_system(mpt::chrono::system_clock::time_point tp)
{
#if !defined(MPT_LIBCXX_QUIRK_CHRONO_DATE_NO_ZONED_TIME)
	try
	{
		std::chrono::zoned_time local_tp{ std::chrono::current_zone(), tp };
		std::chrono::local_days dp = std::chrono::floor<std::chrono::days>(local_tp.get_local_time());
		std::chrono::year_month_day ymd{dp};
		std::chrono::hh_mm_ss hms{local_tp.get_local_time() - dp};
		mpt::Date::Local result;
		result.year = static_cast<int>(ymd.year());
		result.month = static_cast<unsigned int>(ymd.month());
		result.day = static_cast<unsigned int>(ymd.day());
		result.hours = static_cast<int32>(hms.hours().count());
		result.minutes = static_cast<int32>(hms.minutes().count());
		result.seconds = static_cast<int32>(hms.seconds().count());
		result.nanoseconds = static_cast<int32>(std::chrono::duration_cast<std::chrono::nanoseconds>(hms.subseconds()).count());
		return result;
	} catch(const std::exception &)
#endif
	{
		return mpt::Date::local_from_unix(mpt::chrono::unix_clock::from_unix_nanoseconds(mpt::chrono::system_clock::to_unix_nanoseconds(tp)));
	}
}

#endif // MODPLUG_TRACKER

#endif



#if MPT_CXX_AT_LEAST(20) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO) && !defined(MPT_LIBCXX_QUIRK_NO_CHRONO_DATE) && 0

inline mpt::chrono::default_system_clock::time_point default_from_UTC(mpt::Date::UTC timeUtc)
{
	return mpt::Date::system_from_UTC(timeUtc);
}
inline mpt::Date::UTC UTC_from_default(mpt::chrono::default_system_clock::time_point tp)
{
	return mpt::Date::UTC_from_system(tp);
}

#if defined(MODPLUG_TRACKER)

inline mpt::chrono::default_system_clock::time_point default_from_local(mpt::Date::Local timeLocal)
{
	return mpt::Date::system_from_local(timeLocal);
}
inline mpt::Date::Local local_from_default(mpt::chrono::default_system_clock::time_point tp)
{
	return mpt::Date::local_from_system(tp);
}

#endif // MODPLUG_TRACKER

#elif !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)

inline mpt::chrono::default_system_clock::time_point default_from_UTC(mpt::Date::UTC timeUtc)
{
	return mpt::chrono::default_system_clock::from_unix_nanoseconds(mpt::chrono::unix_clock::to_unix_nanoseconds(mpt::Date::unix_from_UTC(timeUtc)));
}
inline mpt::Date::UTC UTC_from_default(mpt::chrono::default_system_clock::time_point tp)
{
	return mpt::Date::UTC_from_unix(mpt::chrono::unix_clock::from_unix_nanoseconds(mpt::chrono::default_system_clock::to_unix_nanoseconds(tp)));
}

#if defined(MODPLUG_TRACKER)

inline mpt::chrono::default_system_clock::time_point default_from_local(mpt::Date::Local timeLocal)
{
	return mpt::chrono::default_system_clock::from_unix_nanoseconds(mpt::chrono::unix_clock::to_unix_nanoseconds(mpt::Date::unix_from_local(timeLocal)));
}
inline mpt::Date::Local local_from_default(mpt::chrono::default_system_clock::time_point tp)
{
	return mpt::Date::local_from_unix(mpt::chrono::unix_clock::from_unix_nanoseconds(mpt::chrono::default_system_clock::to_unix_nanoseconds(tp)));
}

#endif // MODPLUG_TRACKER

#else

inline mpt::chrono::default_system_clock::time_point default_from_UTC(mpt::Date::UTC timeUtc)
{
	return mpt::Date::unix_from_UTC(timeUtc);
}
inline mpt::Date::UTC UTC_from_default(mpt::chrono::default_system_clock::time_point tp)
{
	return mpt::Date::UTC_from_unix(tp);
}

#if defined(MODPLUG_TRACKER)

inline mpt::chrono::default_system_clock::time_point default_from_local(mpt::Date::Local timeLocal)
{
	return mpt::Date::unix_from_local(timeLocal);
}
inline mpt::Date::Local local_from_default(mpt::chrono::default_system_clock::time_point tp)
{
	return mpt::Date::local_from_unix(tp);
}

#endif // MODPLUG_TRACKER

#endif



} // namespace Date



namespace Date
{



mpt::ustring ToShortenedISO8601(AnyGregorian date); // i.e. 2015-01-15T18:32:01

mpt::ustring ToShortenedISO8601(UTC date); // i.e. 2015-01-15T18:32:01Z

#ifdef MODPLUG_TRACKER
mpt::ustring ToShortenedISO8601(Local date); // i.e. 2015-01-15T18:32:01
#endif // MODPLUG_TRACKER

mpt::ustring ToISO8601(AnyGregorian date);

mpt::ustring ToISO8601(UTC date);

#ifdef MODPLUG_TRACKER
mpt::ustring ToISO8601(Local date);
#endif // MODPLUG_TRACKER



} // namespace Date



} // namespace mpt



OPENMPT_NAMESPACE_END
