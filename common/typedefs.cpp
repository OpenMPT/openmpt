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
#include <iostream>
#include <cstring>


#if !defined(MODPLUG_TRACKER) && defined(MPT_ASSERT_HANDLER_NEEDED)

noinline void AssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//--------------------------------------------------------------------------------------------------------------
{
	if(msg)
	{
		Logger(file, line, function)("ASSERTION FAILED: %s (%s)", msg, expr);
	} else
	{
		Logger(file, line, function)("ASSERTION FAILED: %s", expr);
	}
}

#endif // !MODPLUG_TRACKER &&  MPT_ASSERT_HANDLER_NEEDED


//#define LOG_TO_FILE


static const std::size_t LOGBUF_SIZE = 1024;


#if defined(MODPLUG_TRACKER) && defined(_DEBUG)


static uint64 GetTimeMS()
//-----------------------
{
	FILETIME filetime;
	GetSystemTimeAsFileTime(&filetime);
	return ((uint64)filetime.dwHighDateTime << 32 | filetime.dwLowDateTime) / 10000;
}

static std::string TimeAsAsString(uint64 ms)
//------------------------------------------
{

	FILETIME filetime;
	SYSTEMTIME systime;
	filetime.dwHighDateTime = (DWORD)(((uint64)ms * 10000) >> 32);
	filetime.dwLowDateTime = (DWORD)((uint64)ms * 10000);
	FileTimeToSystemTime(&filetime, &systime);

	std::string result;
	TCHAR buf[LOGBUF_SIZE];

	GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &systime, "yyyy-MM-dd ", buf, LOGBUF_SIZE);
	result.append(buf);

	GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, &systime, "HH:mm:ss.", buf, LOGBUF_SIZE);
	result.append(buf);

	sprintf(buf, "%03u", (unsigned)systime.wMilliseconds);
	result.append(buf);

	return result;
}

static std::string TimeDiffAsString(uint64 ms)
//--------------------------------------------
{
	return mpt::fmt::dec<6>(ms);
}


#endif // MODPLUG_TRACKER


static noinline void DoLog(const char *file, int line, const char *function, const char *format, va_list args)
//------------------------------------------------------------------------------------------------------------
{
#if !defined(MODPLUG_TRACKER) || (defined(MODPLUG_TRACKER) && defined(_DEBUG))
	char message[LOGBUF_SIZE];
	va_list va;
	va_copy(va, args);
	vsnprintf(message, LOGBUF_SIZE, format, va);
	message[LOGBUF_SIZE - 1] = '\0';
	va_end(va);
	if(!file)
	{
		file = "unknown";
	}
	if(!function)
	{
		function = "";
	}
	// remove eol if already present
	std::size_t len = std::strlen(message);
	if(len > 0 && message[len-1] == '\n')
	{
		message[len-1] = '\0';
		len--;
	}
	if(len > 0 && message[len-1] == '\r')
	{
		message[len-1] = '\0';
		len--;
	}
	#if defined(MODPLUG_TRACKER)
		static uint64_t s_lastlogtime = 0;
		uint64 cur = GetTimeMS();
		uint64 diff = cur - s_lastlogtime;
		s_lastlogtime = cur;
		#ifdef LOG_TO_FILE
		{
			static FILE * s_logfile = nullptr;
			if(!s_logfile)
			{
				s_logfile = mpt_fopen(MPT_PATHSTRING("mptrack.log"), "a");
			}
			if(s_logfile)
			{
				fprintf(s_logfile, "%s+%s %s(%i): %s [%s]\n", TimeAsAsString(cur).c_str(), TimeDiffAsString(diff).c_str(), file, line, message, function);
				fflush(s_logfile);
			}
		}
		#endif // LOG_TO_FILE
		{
			OutputDebugString(mpt::String::Format("%s(%i): +%s %s [%s]\n", file, line, TimeDiffAsString(diff).c_str(), message, function).c_str());
		}
	#else // !MODPLUG_TRACKER
		std::clog
			<< "openmpt: "
			<< file << "(" << line << ")" << ": "
			<< std::string(message)
			<< " [" << function << "]"
			<< std::endl;
	#endif // MODPLUG_TRACKER
#else
	MPT_UNREFERENCED_PARAMETER(file);
	MPT_UNREFERENCED_PARAMETER(line);
	MPT_UNREFERENCED_PARAMETER(function);
	MPT_UNREFERENCED_PARAMETER(format);
	MPT_UNREFERENCED_PARAMETER(args);
#endif
}


#ifndef NO_LOGGING

void Logger::operator () (const char *format, ...)
//------------------------------------------------
{
	va_list va;
	va_start(va, format);
	DoLog(file, line, function, format, va);
	va_end(va);
}


#undef Log
void Log(const char * format, ...)
//--------------------------------
{
	va_list va;
	va_start(va, format);
	DoLog(nullptr, 0, nullptr, format, va);
	va_end(va);
}

#endif // !NO_LOGGING

