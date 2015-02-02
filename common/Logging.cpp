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
#include "mptFileIO.h"
#if defined(MODPLUG_TRACKER)
#include "mptAtomic.h"
#endif
#include "version.h"

#include <iostream>
#include <cstring>


OPENMPT_NAMESPACE_BEGIN


//#define LOG_TO_FILE


namespace mpt
{
namespace log
{


static const std::size_t LOGBUF_SIZE = 1024;


#ifndef NO_LOGGING


static noinline void DoLog(const mpt::log::Context &context, mpt::ustring message)
//--------------------------------------------------------------------------------
{
	// remove eol if already present
	message = mpt::String::RTrim(message, MPT_USTRING("\r\n"));
	#if defined(MODPLUG_TRACKER)
		static uint64_t s_lastlogtime = 0;
		uint64 cur = mpt::Date::ANSI::Now();
		uint64 diff = cur/10000 - s_lastlogtime;
		s_lastlogtime = cur/10000;
		#ifdef LOG_TO_FILE
		{
			static FILE * s_logfile = nullptr;
			if(!s_logfile)
			{
				s_logfile = mpt_fopen(MPT_PATHSTRING("mptrack.log"), "a");
			}
			if(s_logfile)
			{
				fprintf(s_logfile, mpt::ToCharset(mpt::CharsetUTF8, mpt::String::Print(MPT_USTRING("%1+%2 %3(%4): %5 [%6]\n"
					, mpt::Date::ANSI::ToString(cur)
					, mpt::ufmt::dec<6>(diff)
					, mpt::ToUnicode(mpt::CharsetASCII, context.file)
					, context.line
					, message
					, mpt::ToUnicode(mpt::CharsetASCII, context.function)
					))).c_str());
				fflush(s_logfile);
			}
		}
		#endif // LOG_TO_FILE
		{
			OutputDebugStringW(mpt::String::PrintW(L"%1(%2): +%3 %4 [%5]\n"
				, mpt::ToWide(mpt::CharsetASCII, context.file)
				, context.line
				, mpt::wfmt::dec<6>(diff)
				, message
				, mpt::ToWide(mpt::CharsetASCII, context.function)
				).c_str());
		}
	#else // !MODPLUG_TRACKER
		std::clog
			<< "openmpt: "
			<< context.file << "(" << context.line << ")" << ": "
#if defined(MPT_WITH_CHARSET_LOCALE)
			<< mpt::ToLocale(message)
#else
			<< mpt::ToCharset(mpt::CharsetUTF8, message)
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
	DoLog(context, mpt::ToUnicode(mpt::CharsetLocale, message));
#else
	DoLog(context, mpt::ToUnicode(mpt::CharsetUTF8, message));
#endif
}


void Logger::operator () (LogLevel level, const mpt::ustring &text)
//-----------------------------------------------------------------
{
	DoLog(context, LogLevelToString(level) + MPT_USTRING(": ") + text);
}

void Logger::operator () (const AnyStringLocale &text)
//----------------------------------------------------
{
	DoLog(context, text);
}

void Logger::operator () (const char *format, ...)
//------------------------------------------------
{
	va_list va;
	va_start(va, format);
	DoLog(context, format, va);
	va_end(va);
}


#endif // !NO_LOGGING



#if defined(MODPLUG_TRACKER)

namespace Trace {

// Debugging functionality will use simple globals.

bool volatile g_Enabled = false;

static bool g_Sealed = false;

struct Entry {
	uint32       Index;
	uint32       ThreadId;
	uint64       Timestamp;
	const char * Function;
	const char * File;
	int          Line;
};

inline bool operator < (const Entry &a, const Entry &b)
{
/*
	return false
		|| (a.Timestamp < b.Timestamp)
		|| (a.ThreadID < b.ThreadID)
		|| (a.File < b.File)
		|| (a.Line < b.Line)
		|| (a.Function < b.Function)
		;
*/
	return false
		|| (a.Index < b.Index)
		;
}

static std::vector<mpt::log::Trace::Entry> Entries;

static mpt::atomic_uint32_t NextIndex = 0;

static uint32 ThreadIdGUI = 0;
static uint32 ThreadIdAudio = 0;
static uint32 ThreadIdNotify = 0;

void Enable(std::size_t numEntries)
{
	if(g_Sealed)
	{
		return;
	}
	Entries.clear();
	Entries.resize(numEntries);
	NextIndex.store(0);
	g_Enabled = true;
}

void Disable()
{
	if(g_Sealed)
	{
		return;
	}
	g_Enabled = false;
}

noinline void Trace(const mpt::log::Context & context)
{
	// This will get called in realtime contexts and hot paths.
	// No blocking allowed here.
	const uint32 index = NextIndex.fetch_add(1);
#if 1
	LARGE_INTEGER time;
	time.QuadPart = 0;
	QueryPerformanceCounter(&time);
	const uint64 timestamp = time.QuadPart;
#else
	FILETIME time = FILETIME();
	GetSystemTimeAsFileTime(&time);
	const uint64 timestamp = (static_cast<uint64>(time.dwHighDateTime) << 32) | (static_cast<uint64>(time.dwLowDateTime) << 0);
#endif
	const uint32 threadid = static_cast<uint32>(GetCurrentThreadId());
	mpt::log::Trace::Entry & entry = Entries[index % Entries.size()];
	entry.Index = index;
	entry.ThreadId = threadid;
	entry.Timestamp = timestamp;
	entry.Function = context.function;
	entry.File = context.file;
	entry.Line = context.line;
}

void Seal()
{
	if(!g_Enabled)
	{
		return;
	}
	g_Enabled = false;
	g_Sealed = true;
	uint32 count = NextIndex.fetch_add(0);
	if(count < Entries.size())
	{
		Entries.resize(count);
	}
}

bool Dump(const mpt::PathString &filename)
{
	if(!g_Sealed)
	{
		return false;
	}

	LARGE_INTEGER qpcNow;
	qpcNow.QuadPart = 0;
	QueryPerformanceCounter(&qpcNow);
	uint64 ftNow = mpt::Date::ANSI::Now();

	// sort according to index in case of overflows
	std::stable_sort(Entries.begin(), Entries.end());

	mpt::ofstream f(filename, std::ios::out);

	f << "Build: OpenMPT " << MptVersion::GetVersionStringExtended() << std::endl;

	bool qpcValid = false;

	LARGE_INTEGER qpcFreq;
	qpcFreq.QuadPart = 0;
	QueryPerformanceFrequency(&qpcFreq);
	if(qpcFreq.QuadPart > 0)
	{
		qpcValid = true;
	}

	f << "Dump: " << mpt::ToCharset(mpt::CharsetUTF8, mpt::Date::ANSI::ToString(ftNow)) << std::endl;
	f << "Captured events: " << Entries.size() << std::endl;
	if(qpcValid && (Entries.size() > 0))
	{
		double period = static_cast<double>(Entries[Entries.size() - 1].Timestamp - Entries[0].Timestamp) / static_cast<double>(qpcFreq.QuadPart);
		double eventsPerSecond = Entries.size() / period;
		f << "Period [s]: " << mpt::fmt::fix(period) << std::endl;
		f << "Events/second: " << mpt::fmt::fix(eventsPerSecond) << std::endl;
	}

	for(std::size_t i = 0; i < Entries.size(); ++i)
	{
		mpt::log::Trace::Entry & entry = Entries[i];
		if(!entry.Function) entry.Function = "";
		if(!entry.File) entry.File = "";
		std::string time;
		if(qpcValid)
		{
			time = mpt::ToCharset(mpt::CharsetUTF8, mpt::Date::ANSI::ToString( ftNow - static_cast<int64>( static_cast<double>(qpcNow.QuadPart - entry.Timestamp) * (10000000.0 / static_cast<double>(qpcFreq.QuadPart) ) ) ) );
		} else
		{
			time = mpt::String::Print<std::string>("0x%1", mpt::fmt::hex0<16>(entry.Timestamp));
		}
		f << time;
		if(entry.ThreadId == ThreadIdGUI)
		{
			f << " -----GUI ";
		} else if(entry.ThreadId == ThreadIdAudio)
		{
			f << " ---Audio ";
		} else if(entry.ThreadId == ThreadIdNotify)
		{
			f << " --Notify ";
		} else
		{
			f << " " << mpt::fmt::hex0<8>(entry.ThreadId) << " ";
		}
		f << entry.File << "(" << entry.Line << "): " << entry.Function;
		f << std::endl;
	}
	return true;
}

void SetThreadId(mpt::log::Trace::ThreadKind kind, uint32 id)
{
	if(id == 0)
	{
		return;
	}
	switch(kind)
	{
		case ThreadKindGUI:
			ThreadIdGUI = id;
			break;
		case ThreadKindAudio:
			ThreadIdAudio = id;
			break;
		case ThreadKindNotify:
			ThreadIdNotify = id;
			break;
	}
}

} // namespace Trace

#endif // MODPLUG_TRACKER


} // namespace log
} // namespace mpt


OPENMPT_NAMESPACE_END
