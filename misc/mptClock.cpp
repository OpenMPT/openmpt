/*
 * mptClock.cpp
 * ------------
 * Purpose: Various clock utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptClock.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#if defined(MODPLUG_TRACKER)
#include <mmsystem.h>
#endif
#endif


OPENMPT_NAMESPACE_BEGIN



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
	TIMECAPS caps = {};
	if(timeGetDevCaps(&caps, sizeof(caps)) != MMSYSERR_NOERROR)
	{
		return;
	}
	if((caps.wPeriodMax == 0) || (caps.wPeriodMin > caps.wPeriodMax))
	{
		return;
	}
	ms = std::clamp(mpt::saturate_cast<UINT>(ms), caps.wPeriodMin, caps.wPeriodMax);
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
	if(ms != 0)
	{
		SetPeriod(ms);
	}
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
