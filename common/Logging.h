/*
 * Logging.h
 * ---------
 * Purpose: General logging and user confirmation support
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once


OPENMPT_NAMESPACE_BEGIN


enum LogLevel
{
	/*LogDebug        = 1,*/
	LogNotification = 2,
	LogInformation  = 3,
	LogWarning      = 4,
	LogError        = 5
};


inline mpt::ustring LogLevelToString(LogLevel level)
{
	switch(level)
	{
	case LogError:        return MPT_USTRING("error");   break;
	case LogWarning:      return MPT_USTRING("warning"); break;
	case LogInformation:  return MPT_USTRING("info");    break;
	case LogNotification: return MPT_USTRING("notify");  break;
	}
	return MPT_USTRING("unknown");
}


class ILog
{
public:
	virtual	void AddToLog(LogLevel level, const mpt::ustring &text) const = 0;
};



namespace mpt
{
namespace log
{


struct Context
{
	const char * const file;
	const int line;
	const char * const function;
	forceinline Context(const char *file, int line, const char *function)
		: file(file)
		, line(line)
		, function(function)
	{
		return;
	}
	forceinline Context(const Context &c)
		: file(c.file)
		, line(c.line)
		, function(c.function)
	{
		return;
	}
}; // class Context

#define MPT_LOG_CURRENTCONTEXT() mpt::log::Context( __FILE__ , __LINE__ , __FUNCTION__ )


#ifndef NO_LOGGING

class Logger
{
private:
	const Context &context;
public:
	Logger(const Context &context) : context(context) {}
	void MPT_PRINTF_FUNC(2,3) operator () (const char *format, ...);
	void operator () (LogLevel level, const mpt::ustring &text);
	void operator () (const AnyStringLocale &text);
};

#define Log mpt::log::Logger(MPT_LOG_CURRENTCONTEXT())

#else // !NO_LOGGING

class Logger
{
public:
	inline void MPT_PRINTF_FUNC(2,3) operator () (const char * /*format*/, ...) {}
	inline void operator () (LogLevel /*level*/ , const mpt::ustring & /*text*/ ) {}
	inline void operator () (const AnyStringLocale & /*text*/ ) {}
};

#define Log if(true) {} else mpt::log::Logger() // completely compile out arguments to Log() so that they do not even get evaluated

#endif // NO_LOGGING



#if defined(MODPLUG_TRACKER)

namespace Trace {

// This is not strictly thread safe in all corner cases because of missing barriers.
// We do not care in order to not harm the fast path with additional barriers.
// Enabled tracing incurs a runtime overhead with multiple threads as a global atomic variable
//  gets modified.
// This cacheline bouncing does not matter at all
//  if there are not multiple thread adding trace points at high frequency (way greater than 1000Hz),
//  which, in OpenMPT, is only ever the case for just a single thread (the audio thread), if at all.
extern bool volatile g_Enabled;
static inline bool IsEnabled() { return g_Enabled; }

noinline void Trace(const mpt::log::Context & contexxt);

enum ThreadKind {
	ThreadKindGUI,
	ThreadKindAudio,
	ThreadKindNotify,
};

void Enable(std::size_t numEntries);
void Disable();

void SetThreadId(mpt::log::Trace::ThreadKind kind, uint32 id);

void Seal();
bool Dump(const mpt::PathString &filename);

#define MPT_TRACE() do { if(mpt::log::Trace::g_Enabled) { mpt::log::Trace::Trace(MPT_LOG_CURRENTCONTEXT()); } } while(0)

} // namespace Trace

#else // !MODPLUG_TRACKER

#define MPT_TRACE() do { } while(0)

#endif // MODPLUG_TRACKER



} // namespace log
} // namespace mpt


OPENMPT_NAMESPACE_END
