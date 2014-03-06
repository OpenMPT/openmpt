/*
 * Logging.cpp
 * -----------
 * Purpose: General logging and user confirmation support
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "Logging.h"

#include <iostream>
#include <cstring>


//#define LOG_TO_FILE


namespace mpt
{
namespace log
{


#ifndef NO_LOGGING


static const std::size_t LOGBUF_SIZE = 1024;


Context::Context(const char *file, int line, const char *function)
//----------------------------------------------------------------
	: file(file)
	, line(line)
	, function(function)
{
	return;
}


Context::Context(const Context &c)
//--------------------------------
	: file(c.file)
	, line(c.line)
	, function(c.function)
{
	return;
}


#if defined(MODPLUG_TRACKER)


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


static std::wstring TimeDiffAsStringW(uint64 ms)
//----------------------------------------------
{
	return mpt::wfmt::dec<6>(ms);
}


#endif // MODPLUG_TRACKER


static noinline void DoLog(const mpt::log::Context &context, std::wstring message)
//--------------------------------------------------------------------------------
{
	// remove eol if already present
	message = mpt::String::RTrim(message, L"\r\n");
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
				fprintf(s_logfile, "%s+%s %s(%i): %s [%s]\n", TimeAsAsString(cur).c_str(), TimeDiffAsString(diff).c_str(), context.file, context.line, mpt::To(mpt::CharsetUTF8, message).c_str(), context.function);
				fflush(s_logfile);
			}
		}
		#endif // LOG_TO_FILE
		{
			OutputDebugStringW(mpt::String::PrintW(L"%1(%2): +%3 %4 [%5]\n", mpt::ToWide(mpt::CharsetASCII, context.file), context.line, TimeDiffAsStringW(diff), message, mpt::ToWide(mpt::CharsetASCII, context.function)).c_str());
		}
	#else // !MODPLUG_TRACKER
		std::clog
			<< "openmpt: "
			<< context.file << "(" << context.line << ")" << ": "
#if defined(MPT_WITH_CHARSET_LOCALE)
			<< mpt::ToLocale(message)
#else
			<< mpt::To(mpt::CharsetUTF8, message)
#endif
			<< " [" << context.function << "]"
			<< std::endl;
	#endif // MODPLUG_TRACKER
}


static noinline void DoLog(const mpt::log::Context &context, const char *format, va_list args)
//--------------------------------------------------------------------------------------------
{
	char message[LOGBUF_SIZE];
	va_list va;
	va_copy(va, args);
	vsnprintf(message, LOGBUF_SIZE, format, va);
	message[LOGBUF_SIZE - 1] = '\0';
	va_end(va);
#if defined(MPT_WITH_CHARSET_LOCALE)
	DoLog(context, mpt::ToWide(mpt::CharsetLocale, message));
#else
	DoLog(context, mpt::ToWide(mpt::CharsetUTF8, message));
#endif
}


void Logger::operator () (const char *format, ...)
//------------------------------------------------
{
	va_list va;
	va_start(va, format);
	DoLog(context, format, va);
	va_end(va);
}

void Logger::operator () (const std::string &text)
//------------------------------------------------
{
#if defined(MPT_WITH_CHARSET_LOCALE)
	DoLog(context, mpt::ToWide(mpt::CharsetLocale, text));
#else
	DoLog(context, mpt::ToWide(mpt::CharsetUTF8, text));
#endif
}

void Logger::operator () (const std::wstring &text)
//-------------------------------------------------
{
	DoLog(context, text);
}


#endif // !NO_LOGGING


} // namespace log
} // namespace mpt
