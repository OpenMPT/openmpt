/*
 * mptTime.cpp
 * -----------
 * Purpose: Various time utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptTime.h"

#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS
#include "mpt/osinfo/windows_wine_version.hpp"
#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS

#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <chrono>
#endif

#if defined(MPT_FALLBACK_TIMEZONE_C)
#include <ctime>
#endif // MPT_FALLBACK_TIMEZONE_C

#if MPT_OS_WINDOWS
#include <windows.h>
#endif


OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



namespace Date
{



namespace detail
{


	
static int32 ToDaynum(int32 year, int32 month, int32 day)
{
	month = (month + 9) % 12;
	year = year - (month / 10);
	int32 daynum = year*365 + year/4 - year/100 + year/400 + (month*306 + 5)/10 + (day - 1);
	return daynum;
}

static void FromDaynum(int32 d, int32 & year, int32 & month, int32 & day)
{
	int64 g = d;
	int64 y,ddd,mi,mm,dd;

	y = (10000*g + 14780)/3652425;
	ddd = g - (365*y + y/4 - y/100 + y/400);
	if(ddd < 0)
	{
		y = y - 1;
		ddd = g - (365*y + y/4 - y/100 + y/400);
	}
	mi = (100*ddd + 52)/3060;
	mm = (mi + 2)%12 + 1;
	y = y + (mi + 2)/12;
	dd = ddd - (mi*306 + 5)/10 + 1;

	year = static_cast<int32>(y);
	month = static_cast<int32>(mm);
	day = static_cast<int32>(dd);
}

#if defined(MODPLUG_TRACKER)

struct tz_error
{
};

#endif // MODPLUG_TRACKER



} // namespace detail



mpt::chrono::unix_clock::time_point unix_from_UTC(mpt::Date::UTC timeUtc)
{
	int32 daynum = mpt::Date::detail::ToDaynum(timeUtc.year, timeUtc.month, timeUtc.day);
	int64 seconds = static_cast<int64>(daynum - mpt::Date::detail::ToDaynum(1970, 1, 1)) * 24 * 60 * 60 + timeUtc.hours * 60 * 60 + timeUtc.minutes * 60 + timeUtc.seconds;
	return mpt::chrono::unix_clock::time_point{seconds, timeUtc.nanoseconds};
}

mpt::Date::UTC UTC_from_unix(mpt::chrono::unix_clock::time_point tp)
{
	int64 tmp = tp.seconds;
	int64 seconds = tmp % 60; tmp /= 60;
	int64 minutes = tmp % 60; tmp /= 60;
	int64 hours   = tmp % 24; tmp /= 24;
	int32 year = 0, month = 0, day = 0;
	mpt::Date::detail::FromDaynum(static_cast<int32>(tmp) + mpt::Date::detail::ToDaynum(1970,1,1), year, month, day);
	mpt::Date::UTC result = {};
	result.year = year;
	result.month = month;
	result.day = day;
	result.hours = static_cast<int32>(hours);
	result.minutes = static_cast<int32>(minutes);
	result.seconds = static_cast<int32>(seconds);
	result.nanoseconds = tp.nanoseconds;
	return result;
}

#if defined(MODPLUG_TRACKER)

mpt::chrono::unix_clock::time_point unix_from_local(mpt::Date::Local timeLocal)
{
#if defined(MPT_FALLBACK_TIMEZONE_WINDOWS_HISTORIC)
	try
	{
		MPT_MAYBE_CONSTANT_IF(mpt::osinfo::windows::wine::current_is_wine())
		{
			throw mpt::Date::detail::tz_error{};
		}
		SYSTEMTIME sys_local{};
		sys_local.wYear = static_cast<uint16>(timeLocal.year);
		sys_local.wMonth = static_cast<uint16>(timeLocal.month);
		sys_local.wDay = static_cast<uint16>(timeLocal.day);
		sys_local.wHour = static_cast<uint16>(timeLocal.hours);
		sys_local.wMinute = static_cast<uint16>(timeLocal.minutes);
		sys_local.wSecond = static_cast<uint16>(timeLocal.seconds);
		sys_local.wMilliseconds = static_cast<uint16>(timeLocal.nanoseconds / 1'000'000);
		DYNAMIC_TIME_ZONE_INFORMATION dtzi{};
		if(GetDynamicTimeZoneInformation(&dtzi) == TIME_ZONE_ID_INVALID) // WinVista
		{
			throw mpt::Date::detail::tz_error{};
		}
		SYSTEMTIME sys_utc{};
		if(TzSpecificLocalTimeToSystemTimeEx(&dtzi, &sys_local, &sys_utc) == FALSE) // Win7/Win8
		{
			throw mpt::Date::detail::tz_error{};
		}
		FILETIME ft{};
		if(SystemTimeToFileTime(&sys_utc, &ft) == FALSE) // Win 2000
		{
			throw mpt::Date::detail::tz_error{};
		}
		ULARGE_INTEGER time_value{};
		time_value.LowPart = ft.dwLowDateTime;
		time_value.HighPart = ft.dwHighDateTime;
		return mpt::chrono::unix_clock::from_unix_nanoseconds(static_cast<int64>((time_value.QuadPart - 116444736000000000LL) * 100LL));
	} catch(const mpt::Date::detail::tz_error &)
	{
		// nothing
	}
#endif
#if defined(MPT_FALLBACK_TIMEZONE_WINDOWS_CURRENT)
	try
	{
		SYSTEMTIME sys_local{};
		sys_local.wYear = static_cast<uint16>(timeLocal.year);
		sys_local.wMonth = static_cast<uint16>(timeLocal.month);
		sys_local.wDay = static_cast<uint16>(timeLocal.day);
		sys_local.wHour = static_cast<uint16>(timeLocal.hours);
		sys_local.wMinute = static_cast<uint16>(timeLocal.minutes);
		sys_local.wSecond = static_cast<uint16>(timeLocal.seconds);
		sys_local.wMilliseconds = static_cast<uint16>(timeLocal.nanoseconds / 1'000'000);
		SYSTEMTIME sys_utc{};
		if(TzSpecificLocalTimeToSystemTime(NULL, &sys_local, &sys_utc) == FALSE) // WinXP
		{
			throw mpt::Date::detail::tz_error{};
		}
		FILETIME ft{};
		if(SystemTimeToFileTime(&sys_utc, &ft) == FALSE) // Win 2000
		{
			throw mpt::Date::detail::tz_error{};
		}
		ULARGE_INTEGER time_value{};
		time_value.LowPart = ft.dwLowDateTime;
		time_value.HighPart = ft.dwHighDateTime;
		return mpt::chrono::unix_clock::from_unix_nanoseconds(static_cast<int64>((time_value.QuadPart - 116444736000000000LL) * 100LL));
	} catch(const mpt::Date::detail::tz_error &)
	{
		// nothing
	}
#endif
#if defined(MPT_FALLBACK_TIMEZONE_C)
	std::tm tmp{};
	tmp.tm_year = timeLocal.year - 1900;
	tmp.tm_mon = timeLocal.month - 1;
	tmp.tm_mday = timeLocal.day;
	tmp.tm_hour = timeLocal.hours;
	tmp.tm_min = timeLocal.minutes;
	tmp.tm_sec = static_cast<int>(timeLocal.seconds);
	return mpt::chrono::unix_clock::from_unix_seconds(static_cast<int64>(std::mktime(&tmp)));
#endif
}

mpt::Date::Local local_from_unix(mpt::chrono::unix_clock::time_point tp)
{
#if defined(MPT_FALLBACK_TIMEZONE_WINDOWS_HISTORIC)
	try
	{
		MPT_MAYBE_CONSTANT_IF(mpt::osinfo::windows::wine::current_is_wine())
		{
			throw mpt::Date::detail::tz_error{};
		}
		ULARGE_INTEGER time_value{};
		time_value.QuadPart = (static_cast<int64>(mpt::chrono::unix_clock::to_unix_nanoseconds(tp)) / 100LL) + 116444736000000000LL;
		FILETIME ft{};
		ft.dwLowDateTime = time_value.LowPart;
		ft.dwHighDateTime = time_value.HighPart;
		SYSTEMTIME sys_utc{};
		if(FileTimeToSystemTime(&ft, &sys_utc) == FALSE) // WinXP
		{
			throw mpt::Date::detail::tz_error{};
		}
		DYNAMIC_TIME_ZONE_INFORMATION dtzi{};
		if(GetDynamicTimeZoneInformation(&dtzi) == TIME_ZONE_ID_INVALID) // WinVista
		{
			throw mpt::Date::detail::tz_error{};
		}
		SYSTEMTIME sys_local{};
		if(SystemTimeToTzSpecificLocalTimeEx(&dtzi, &sys_utc, &sys_local) == FALSE) // Win7/Win8
		{
			throw mpt::Date::detail::tz_error{};
		}
		mpt::Date::Local result{};
		result.year = sys_local.wYear;
		result.month = sys_local.wMonth;
		result.day = sys_local.wDay;
		result.hours = sys_local.wHour;
		result.minutes = sys_local.wMinute;
		result.seconds = sys_local.wSecond;
		result.nanoseconds = static_cast<int32>(sys_local.wMilliseconds) * 1'000'000;
		return result;
	} catch(const mpt::Date::detail::tz_error&)
	{
		// nothing
	}
#endif
#if defined(MPT_FALLBACK_TIMEZONE_WINDOWS_CURRENT)
	try
	{
		ULARGE_INTEGER time_value{};
		time_value.QuadPart = (static_cast<int64>(mpt::chrono::unix_clock::to_unix_nanoseconds(tp)) / 100LL) + 116444736000000000LL;
		FILETIME ft{};
		ft.dwLowDateTime = time_value.LowPart;
		ft.dwHighDateTime = time_value.HighPart;
		SYSTEMTIME sys_utc{};
		if(FileTimeToSystemTime(&ft, &sys_utc) == FALSE) // WinXP
		{
			throw mpt::Date::detail::tz_error{};
		}
		SYSTEMTIME sys_local{};
		if(SystemTimeToTzSpecificLocalTime(NULL, &sys_utc, &sys_local) == FALSE) // Win2000
		{
			throw mpt::Date::detail::tz_error{};
		}
		mpt::Date::Local result{};
		result.year = sys_local.wYear;
		result.month = sys_local.wMonth;
		result.day = sys_local.wDay;
		result.hours = sys_local.wHour;
		result.minutes = sys_local.wMinute;
		result.seconds = sys_local.wSecond;
		result.nanoseconds = static_cast<int32>(sys_local.wMilliseconds) * 1'000'000;
		return result;
	} catch(const mpt::Date::detail::tz_error&)
	{
		// nothing
	}
#endif
#if defined(MPT_FALLBACK_TIMEZONE_C)
	std::time_t time_tp = static_cast<std::time_t>(mpt::chrono::unix_clock::to_unix_seconds(tp));
	std::tm *tmp = std::localtime(&time_tp);
	if(!tmp)
	{
		return mpt::Date::Local{};
	}
	std::tm local = *tmp;
	mpt::Date::Local result{};
	result.year = local.tm_year + 1900;
	result.month = local.tm_mon + 1;
	result.day = local.tm_mday;
	result.hours = local.tm_hour;
	result.minutes = local.tm_min;
	result.seconds = local.tm_sec;
	result.nanoseconds = 0;
	return result;
#endif
}

#endif // MODPLUG_TRACKER



} // namespace Date



namespace Date
{



template <LogicalTimezone TZ>
static mpt::ustring ToISO8601Impl(mpt::Date::Gregorian<TZ> date, bool shorten)
{
	mpt::ustring result;
	mpt::ustring tz;
	if constexpr(TZ == LogicalTimezone::Unspecified)
	{
		tz = U_("");
	} else if constexpr(TZ == LogicalTimezone::UTC)
	{
		tz = U_("Z");
	} else
	{
		tz = U_("");
	}
	if(shorten && (date.year == 0))
	{
		return result;
	}
	result += mpt::ufmt::dec0<4>(date.year);
	result += U_("-") + mpt::ufmt::dec0<2>(date.month);
	result += U_("-") + mpt::ufmt::dec0<2>(date.day);
	if(shorten && (date.hours == 0 && date.minutes == 0 && date.seconds))
	{
		return result;
	}
	result += U_("T");
	result += mpt::ufmt::dec0<2>(date.hours) + U_(":") + mpt::ufmt::dec0<2>(date.minutes);
	if(shorten && (date.seconds == 0))
	{
		return result + tz;
	}
	result += U_(":") + mpt::ufmt::dec0<2>(date.seconds);
	if(shorten)
	{
		return result + tz;
	}
	result += U_(".") + mpt::ufmt::dec0<9>(date.nanoseconds);
	result += tz;
	return result;
}

mpt::ustring ToShortenedISO8601(mpt::Date::AnyGregorian date)
{
	return ToISO8601Impl(date, true);
}

mpt::ustring ToShortenedISO8601(mpt::Date::UTC date)
{
	return ToISO8601Impl(date, true);
}

#ifdef MODPLUG_TRACKER
mpt::ustring ToShortenedISO8601(Local date)
{
	return ToISO8601Impl(date, true);
}
#endif // MODPLUG_TRACKER

mpt::ustring ToISO8601(mpt::Date::AnyGregorian date)
{
	return ToISO8601Impl(date, false);
}

mpt::ustring ToISO8601(mpt::Date::UTC date)
{
	return ToISO8601Impl(date, false);
}

#ifdef MODPLUG_TRACKER
mpt::ustring ToISO8601(Local date)
{
	return ToISO8601Impl(date, false);
}
#endif // MODPLUG_TRACKER



} // namespace Date



} // namespace mpt



OPENMPT_NAMESPACE_END
