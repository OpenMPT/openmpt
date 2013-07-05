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


#ifndef MODPLUG_TRACKER
void AlwaysAssertHandler(const char *file, int line, const char *function, const char *expr, const char *msg)
//-----------------------------------------------------------------------------------------------------------
{
	if(msg)
	{
		std::cerr
			<< "openmpt: ASSERTION FAILED: "
			<< file << "(" << line << ")" << ": "
			<< msg
			<< " (" << std::string(expr) << ") "
			<< " [" << function << "]"
			<< std::endl
			;
	} else
	{
		std::cerr
			<< "openmpt: ASSERTION FAILED: "
			<< file << "(" << line << ")" << ": "
			<< std::string(expr)
			<< " [" << function << "]"
			<< std::endl
			;
	}
}
#endif


#if !defined(_MFC_VER)
void AssertHandler(const char *file, int line, const char *function, const char *expr)
//------------------------------------------------------------------------------------
{
	std::cerr
		<< "openmpt: ASSERTION FAILED: "
		<< file << "(" << line << ")" << ": "
		<< std::string(expr)
		<< " [" << function << "]"
		<< std::endl
		;
}
#endif


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
	return mpt::String::Format("%6u", (unsigned)(ms));
}


#endif // MODPLUG_TRACKER


static void DoLog(const char *file, int line, const char *function, const char *format, va_list args)
//---------------------------------------------------------------------------------------------------
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
			char verbose_message[LOGBUF_SIZE];
			snprintf(verbose_message, LOGBUF_SIZE, "%s+%s %s(%i): %s [%s]\n", TimeAsAsString(cur).c_str(), TimeDiffAsString(diff).c_str(), file, line, message, function);
			verbose_message[LOGBUF_SIZE - 1] = '\0';
			if(!s_logfile)
			{
				s_logfile = fopen("mptrack.log", "a");
			}
			if(s_logfile)
			{
				fprintf(s_logfile, "%s", verbose_message);
				fflush(s_logfile);
			}
		}
		#endif // LOG_TO_FILE
		{
			char verbose_message[LOGBUF_SIZE];
			snprintf(verbose_message, LOGBUF_SIZE, "%s(%i): +%s %s [%s]\n", file, line, TimeDiffAsString(diff).c_str(), message, function);
			verbose_message[LOGBUF_SIZE - 1] = '\0';
			OutputDebugString(verbose_message);
		}
	#else // !MODPLUG_TRACKER
		std::clog
			<< "openmpt: DEBUG: "
			<< file << "(" << line << ")" << ": "
			<< std::string(message)
			<< " [" << function << "]"
			<< std::endl;
	#endif // MODPLUG_TRACKER
#else
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(line);
	UNREFERENCED_PARAMETER(function);
	UNREFERENCED_PARAMETER(format);
	UNREFERENCED_PARAMETER(args);
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

#endif
