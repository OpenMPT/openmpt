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



namespace mpt
{
namespace log
{


#ifndef NO_LOGGING


class Context
{
public:
	const char * const file;
	const int line;
	const char * const function;
public:
	Context(const char *file, int line, const char *function);
	Context(const Context &c);
}; // class Context

#define MPT_LOG_CURRENTCONTEXT() ::mpt::log::Context( __FILE__ , __LINE__ , __FUNCTION__ )


class Logger
{
private:
	const Context &context;
public:
	Logger(const Context &context) : context(context) {}
	void MPT_PRINTF_FUNC(2,3) operator () (const char *format, ...);
	void operator () (const std::string &text);
	void operator () (const std::wstring &text);
};

#define Log ::mpt::log::Logger(MPT_LOG_CURRENTCONTEXT())


#else // !NO_LOGGING


class Logger
{
public:
	inline void MPT_PRINTF_FUNC(2,3) operator () (const char * /*format*/, ...) {}
	inline void operator () (const std::string & /*text*/ ) {}
	inline void operator () (const std::wstring & /*text*/ ) {}
};

#define Log if(true) {} else ::mpt::log::Logger() // completely compile out arguments to Log() so that they do not even get evaluated


#endif // NO_LOGGING


} // namespace log
} // namespace mpt
