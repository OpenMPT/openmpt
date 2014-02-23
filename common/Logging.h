/*
 * Logging.h
 * ---------
 * Purpose: General logging and user confirmation support
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


enum LogLevel
{
	/*LogDebug        = 1,*/
	LogNotification = 2,
	LogInformation  = 3,
	LogWarning      = 4,
	LogError        = 5
};


inline std::string LogLevelToString(LogLevel level)
{
	switch(level)
	{
	case LogError:        return "error";   break;
	case LogWarning:      return "warning"; break;
	case LogInformation:  return "info";    break;
	case LogNotification: return "notify";  break;
	}
	return "unknown";
}


class ILog
{
public:
	virtual void AddToLog(const std::string &text) const { AddToLog(LogInformation, text); }
	virtual	void AddToLog(LogLevel level, const std::string &text) const = 0;
};


#ifndef NO_LOGGING
void MPT_PRINTF_FUNC(1,2) Log(const char *format, ...);
void Log(const std::string &text);
void Log(const std::wstring &text);
class Logger
{
private:
	const char * const file;
	int const line;
	const char * const function;
public:
	Logger(const char *file_, int line_, const char *function_) : file(file_), line(line_), function(function_) {}
	void MPT_PRINTF_FUNC(2,3) operator () (const char *format, ...);
	void operator () (const std::string &text);
	void operator () (const std::wstring &text);
};
#define Log Logger(__FILE__, __LINE__, __FUNCTION__)
#else // !NO_LOGGING
static inline void MPT_PRINTF_FUNC(1,2) Log(const char * /*format*/, ...) {}
static inline void Log(const std::string & /*text*/ ) {}
static inline void Log(const std::wstring & /*text*/ ) {}
class Logger
{
public:
	inline void MPT_PRINTF_FUNC(2,3) operator () (const char * /*format*/, ...) {}
	inline void operator () (const std::string & /*text*/ ) {}
	inline void operator () (const std::wstring & /*text*/ ) {}
};
#define Log if(true) {} else Logger() // completely compile out arguments to Log() so that they do not even get evaluated
#endif // NO_LOGGING

// just #undef Log in files, where this Log redefinition causes problems
//#undef Log

