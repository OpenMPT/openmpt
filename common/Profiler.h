/*
 * Profiler.h
 * ----------
 * Purpose: Performance measuring
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


#include "mpt/mutex/mutex.hpp"

#include "mptCPU.h"

#include <chrono>
#include <optional>
#include <ratio>
#include <string>
#include <vector>

#if (defined(MPT_ENABLE_ARCH_X86) || defined(MPT_ENABLE_ARCH_AMD64)) && defined(MPT_ARCH_X86_TSC)
#if MPT_COMPILER_CLANG
#include <x86intrin.h>
#elif MPT_COMPILER_GCC
#include <x86intrin.h>
#elif MPT_COMPILER_MSVC
#include <intrin.h>
#endif
#include <immintrin.h>
#elif MPT_OS_WINDOWS
#include <windows.h>
#endif


OPENMPT_NAMESPACE_BEGIN


#if defined(MODPLUG_TRACKER)

//#define USE_PROFILER

#endif


namespace mpt
{

namespace profiler
{


#if (defined(MPT_ENABLE_ARCH_X86) || defined(MPT_ENABLE_ARCH_AMD64)) && defined(MPT_ARCH_X86_TSC)

#define MPT_PROFILER_TSC_CLOCK 1

struct tsc_clock {
	using rep = uint64;
	//using period = std::ratio;
	//using duration = std::chrono::duration<rep, period>;
	//using time_point = std::chrono::time_point<tsc_clock>;
	static inline constexpr bool is_steady = false;
	static inline constexpr bool is_dynamic = true;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_period() noexcept {
		return std::nullopt;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return std::nullopt;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return __rdtsc();
	}
	//[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept;
};

using highres_clock = tsc_clock;
using fast_clock = tsc_clock;

using default_clock = tsc_clock;

#elif MPT_OS_WINDOWS

#define MPT_PROFILER_TSC_CLOCK 0

struct QueryPerformanceCounter_clock {
	using rep = uint64;
	//using period = std::ratio;
	//using duration = std::chrono::duration<rep, period>;
	//using time_point = std::chrono::time_point<QueryPerformanceCounter_clock>;
	static inline constexpr bool is_steady = false;
	static inline constexpr bool is_dynamic = true;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_period() noexcept {
		LARGE_INTEGER result{};
		QueryPerformanceFrequency(&result);
		return 1.0 / static_cast<double>(result.QuadPart);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		LARGE_INTEGER result{};
		QueryPerformanceFrequency(&result);
		return static_cast<double>(result.QuadPart);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		LARGE_INTEGER result{};
		QueryPerformanceCounter(&result);
		return result.QuadPart;
	}
	//[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept;
};

struct GetTickCount_clock {
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	using rep = uint64;
#else
	using rep = uint32;
#endif
	using period = std::milli;
	using duration = std::chrono::duration<rep, period>;
	using time_point = std::chrono::time_point<GetTickCount_clock>;
	static inline constexpr bool is_steady = true;
	static inline constexpr bool is_dynamic = false;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_period() noexcept {
		return 0.001;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return 1000.0;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
		return GetTickCount64();
#else
		return GetTickCount();
#endif
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return time_point{duration{std::chrono::milliseconds{now_raw()}}};
	}
};
#if MPT_CXX_AT_LEAST(20)
static_assert(std::chrono::is_clock<GetTickCount_clock>::value);
#endif

using highres_clock = QueryPerformanceCounter_clock;
using fast_clock = GetTickCount_clock;

using default_clock = QueryPerformanceCounter_clock;

#else

#define MPT_PROFILER_TSC_CLOCK 0

struct high_resolution_clock {
	using rep = std::chrono::high_resolution_clock::rep;
	using period = std::chrono::high_resolution_clock::period;
	using duration = std::chrono::high_resolution_clock::duration;
	using time_point = std::chrono::high_resolution_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::high_resolution_clock::is_steady;
	static inline constexpr bool is_dynamic = false;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_period() noexcept {
		return static_cast<double>(period::num) / static_cast<double>(period::den);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return static_cast<double>(period::den) / static_cast<double>(period::num);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return std::chrono::high_resolution_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return std::chrono::high_resolution_clock::now().time_since_epoch().count();
	}
};
#if MPT_CXX_AT_LEAST(20)
static_assert(std::chrono::is_clock<high_resolution_clock>::value);
#endif

struct steady_clock {
	using rep = std::chrono::steady_clock::rep;
	using period = std::chrono::steady_clock::period;
	using duration = std::chrono::steady_clock::duration;
	using time_point = std::chrono::steady_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::steady_clock::is_steady;
	static inline constexpr bool is_dynamic = false;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_period() noexcept {
		return static_cast<double>(period::num) / static_cast<double>(period::den);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return static_cast<double>(period::den) / static_cast<double>(period::num);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return std::chrono::steady_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return std::chrono::steady_clock::now().time_since_epoch().count();
	}
};
#if MPT_CXX_AT_LEAST(20)
static_assert(std::chrono::is_clock<steady_clock>::value);
#endif

using highres_clock = high_resolution_clock;
using fast_clock = steady_clock;

using default_clock = high_resolution_clock;

#endif // MPT_OS_WINDOWS


} // namespace profiler

} // namespace mpt


#ifdef USE_PROFILER

class Profiler
{
public:
	enum Category
	{
		GUI,
		Audio,
		Notify,
		Settings,
		CategoriesCount
	};
	static std::vector<std::string> GetCategoryNames()
	{
		std::vector<std::string> ret;
		ret.push_back("GUI");
		ret.push_back("Audio");
		ret.push_back("Notify");
		ret.push_back("Settings");
		return ret;
	}
public:
	static void Update();
	static std::string DumpProfiles();
	static std::vector<double> DumpCategories();
};


class Profile
{
private:
	mutable mpt::mutex datamutex;
public:
	struct Data
	{
		uint64 Calls;
		uint64 Sum;
		int64  Overhead;
		uint64 Start;
	};
public:
	Data data;
	uint64 EnterTime;
	Profiler::Category Category;
	const char * const Name;
	uint64 GetTime() const;
	uint64 GetFrequency() const;
public:
	Profile(Profiler::Category category, const char *name);
	~Profile();
	void Reset();
	void Enter();
	void Leave();
	class Scope
	{
	private:
		Profile &profile;
	public:
		Scope(Profile &p) : profile(p) { profile.Enter(); }
		~Scope() { profile.Leave(); }
	};
public:
	Data GetAndResetData();
};


#define OPENMPT_PROFILE_SCOPE(cat, name) \
	static Profile OPENMPT_PROFILE_VAR(cat, name);\
	Profile::Scope OPENMPT_PROFILE_SCOPE_VAR(OPENMPT_PROFILE_VAR); \
/**/


#define OPENMPT_PROFILE_FUNCTION(cat) OPENMPT_PROFILE_SCOPE(cat, __func__)


#else // !USE_PROFILER


class Profiler
{
public:
	enum Category
	{
		CategoriesCount
	};
	static std::vector<std::string> GetCategoryNames() { return std::vector<std::string>(); } 
public:
	static void Update() { }
	static std::string DumpProfiles() { return std::string(); }
	static std::vector<double> DumpCategories() { return std::vector<double>(); }
};
#define OPENMPT_PROFILE_SCOPE(cat, name) do { } while(0)
#define OPENMPT_PROFILE_FUNCTION(cat) do { } while(0)


#endif // USE_PROFILER


OPENMPT_NAMESPACE_END
