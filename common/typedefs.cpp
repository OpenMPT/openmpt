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


static const std::size_t LOGBUF_SIZE = 1024;

static void DoLog(const char *file, int line, const char *function, const char *format, va_list args)
//---------------------------------------------------------------------------------------------------
{
#if !defined(MODPLUG_TRACKER) || defined(_DEBUG)
	char message[LOGBUF_SIZE];
	va_list va;
	va_copy(va, args);
	vsnprintf(message, LOGBUF_SIZE, format, va);
	message[LOGBUF_SIZE - 1] = '\0';
	va_end(va);
	#if defined(MODPLUG_TRACKER)
		char buf2[LOGBUF_SIZE];
		char *verbose_message = nullptr;
		if(file || function)
		{
			snprintf(buf2, LOGBUF_SIZE, "%s(%i): %s", file?file:"", line, message);
			buf2[LOGBUF_SIZE - 1] = '\0';
			verbose_message = buf2;
		} else
		{
			verbose_message = message;
		}
		OutputDebugString(verbose_message);
		#ifdef LOG_TO_FILE
			FILE *f = fopen("c:\\mptrack.log", "a");
			if(f)
			{
				fwrite(verbose_message, 1, strlen(verbose_message), f);
				fclose(f);
			}
		#endif //LOG_TO_FILE
	#else // !MODPLUG_TRACKER
		std::size_t len = std::strlen(message);
		// remove eol if already present
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
		if(file || function)
		{
			std::clog
				<< "openmpt: DEBUG: "
				<< (file?file:"") << "(" << line << ")" << ": "
				<< std::string(message)
				<< " [" << (function?function:"") << "]"
				<< std::endl;
		} else
		{
			std::clog
				<< "openmpt: DEBUG: "
				<< std::string(message)
				<< std::endl;
		}
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
