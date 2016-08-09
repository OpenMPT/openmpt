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

#include <time.h>

#if MPT_OS_WINDOWS
#include <windows.h>
#if defined(MODPLUG_TRACKER)
#include <mmsystem.h>
#endif
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

uint64 Now()
//----------
{
	FILETIME filetime;
	GetSystemTimeAsFileTime(&filetime);
	return ((uint64)filetime.dwHighDateTime << 32 | filetime.dwLowDateTime);
}

mpt::ustring ToString(uint64 time100ns)
//-------------------------------------
{
	static const std::size_t bufsize = 256;

	mpt::ustring result;

	FILETIME filetime;
	SYSTEMTIME systime;
	filetime.dwHighDateTime = (DWORD)(((uint64)time100ns) >> 32);
	filetime.dwLowDateTime = (DWORD)((uint64)time100ns);
	FileTimeToSystemTime(&filetime, &systime);

	WCHAR buf[bufsize];

	GetDateFormatW(LOCALE_SYSTEM_DEFAULT, 0, &systime, L"yyyy-MM-dd", buf, bufsize);
	result.append(mpt::ToUnicode(buf));

	result.append(MPT_USTRING(" "));

	GetTimeFormatW(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &systime, L"HH:mm:ss", buf, bufsize);
	result.append(mpt::ToUnicode(buf));

	result.append(MPT_USTRING("."));

	result.append(mpt::ufmt::dec0<3>((unsigned)systime.wMilliseconds));

	return result;

}

} // namespace ANSI

#endif // MPT_OS_WINDOWS

#endif // MODPLUG_TRACKER

Unix::Unix()
//----------
	: Value(0)
{
	return;
}

Unix::Unix(time_t unixtime)
//-------------------------
	: Value(unixtime)
{
	return;
}

Unix::operator time_t () const
//----------------------------
{
	return Value;
}

mpt::Date::Unix Unix::FromUTC(tm timeUtc)
//---------------------------------------
{
	#if MPT_COMPILER_MSVC || MPT_COMPILER_MSVCCLANGC2
		return mpt::Date::Unix(_mkgmtime(&timeUtc));
	#else // !MPT_COMPILER_MSVC
		// There is no portable way in C/C++ to convert between time_t and struct tm in UTC.
		// Approximate it as good as possible without implementing full date handling logic.
		// NOTE:
		// This can be wrong for dates during DST switch.
		tm t = timeUtc;
		time_t localSinceEpoch = mktime(&t);
		const tm * tmpLocal = localtime(&localSinceEpoch);
		if(!tmpLocal)
		{
			return mpt::Date::Unix(localSinceEpoch);
		}
		tm localTM = *tmpLocal;
		const tm * tmpUTC = gmtime(&localSinceEpoch);
		if(!tmpUTC)
		{
			return mpt::Date::Unix(localSinceEpoch);
		}
		tm utcTM = *tmpUTC;
		double offset = difftime(mktime(&localTM), mktime(&utcTM));
		double timeScaleFactor = difftime(2, 1);
		time_t utcSinceEpoch = localSinceEpoch + Util::Round<time_t>(offset / timeScaleFactor);
		return mpt::Date::Unix(utcSinceEpoch);
	#endif // MPT_COMPILER_MSVC
}

mpt::ustring ToShortenedISO8601(tm date)
//--------------------------------------
{
	// We assume date in UTC here.
	// There are too many differences in supported format specifiers in strftime()
	// and strftime does not support reduced precision ISO8601 at all.
	// Just do the formatting ourselves.
	mpt::ustring result;
	mpt::ustring tz = MPT_USTRING("Z");
	if(date.tm_year == 0)
	{
		return result;
	}
	result += mpt::ufmt::dec0<4>(date.tm_year + 1900);
	if(date.tm_mon < 0 || date.tm_mon > 11)
	{
		return result;
	}
	result += MPT_USTRING("-") + mpt::ufmt::dec0<2>(date.tm_mon + 1);
	if(date.tm_mday < 1 || date.tm_mday > 31)
	{
		return result;
	}
	result += MPT_USTRING("-") + mpt::ufmt::dec0<2>(date.tm_mday);
	if(date.tm_hour == 0 && date.tm_min == 0 && date.tm_sec == 0)
	{
		return result;
	}
	if(date.tm_hour < 0 || date.tm_hour > 23)
	{
		return result;
	}
	if(date.tm_min < 0 || date.tm_min > 59)
	{
		return result;
	}
	result += MPT_USTRING("T");
	if(date.tm_isdst > 0)
	{
		tz = MPT_USTRING("+01:00");
	}
	result += mpt::ufmt::dec0<2>(date.tm_hour) + MPT_USTRING(":") + mpt::ufmt::dec0<2>(date.tm_min);
	if(date.tm_sec < 0 || date.tm_sec > 61)
	{
		return result + tz;
	}
	result += MPT_USTRING(":") + mpt::ufmt::dec0<2>(date.tm_sec);
	result += tz;
	return result;
}

} // namespace Date
} // namespace mpt



#ifdef MODPLUG_TRACKER

namespace Util
{

#if MPT_OS_WINDOWS

void MultimediaClock::Init()
{
	m_CurrentPeriod = 0;
}

void MultimediaClock::SetPeriod(uint32 ms)
{
	TIMECAPS caps;
	MemsetZero(caps);
	if(timeGetDevCaps(&caps, sizeof(caps)) != MMSYSERR_NOERROR)
	{
		return;
	}
	if((caps.wPeriodMax == 0) || (caps.wPeriodMin > caps.wPeriodMax))
	{
		return;
	}
	ms = Clamp<uint32>(ms, caps.wPeriodMin, caps.wPeriodMax);
	if(timeBeginPeriod(ms) != MMSYSERR_NOERROR)
	{
		return;
	}
	m_CurrentPeriod = ms;
}

void MultimediaClock::Cleanup()
{
	if(m_CurrentPeriod > 0)
	{
		if(timeEndPeriod(m_CurrentPeriod) != MMSYSERR_NOERROR)
		{
			// should not happen
			MPT_ASSERT_NOTREACHED();
		}
		m_CurrentPeriod = 0;
	}
}

MultimediaClock::MultimediaClock()
{
	Init();
}

MultimediaClock::MultimediaClock(uint32 ms)
{
	Init();
	SetResolution(ms);
}

MultimediaClock::~MultimediaClock()
{
	Cleanup();
}

uint32 MultimediaClock::SetResolution(uint32 ms)
{
	if(m_CurrentPeriod == ms)
	{
		return m_CurrentPeriod;
	}
	Cleanup();
	SetPeriod(ms);
	return GetResolution();
}

uint32 MultimediaClock::GetResolution() const
{
	return m_CurrentPeriod;
}

uint32 MultimediaClock::Now() const
{
	return timeGetTime();
}

uint64 MultimediaClock::NowNanoseconds() const
{
	return (uint64)timeGetTime() * (uint64)1000000;
}

#endif // MPT_OS_WINDOWS

} // namespace Util

#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
