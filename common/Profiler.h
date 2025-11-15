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
#include "mptTime.h"

#include <algorithm>
#include <atomic>
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <chrono>
#endif
#include <new>
#include <optional>
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <ratio>
#endif
#include <string>
#include <vector>

#include <cstddef>

#if (defined(MPT_ENABLE_ARCH_X86) || defined(MPT_ENABLE_ARCH_AMD64)) && defined(MPT_ARCH_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_TSC)
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



namespace mpt {

namespace profiler {



#if defined(MPT_LIBCXX_QUIRK_NO_HARDWARE_INTERFERENCE_SIZE)

// guess
#if MPT_OS_MACOSX_OR_IOS
#if MPT_ARCH_ARM64
inline constexpr std::size_t cacheline_size_min = std::max(alignof(std::max_align_t), static_cast<std::size_t>(64));
inline constexpr std::size_t cacheline_size_max = std::max(alignof(std::max_align_t), static_cast<std::size_t>(128));
#else
inline constexpr std::size_t cacheline_size_min = std::max(alignof(std::max_align_t), static_cast<std::size_t>(64));
inline constexpr std::size_t cacheline_size_max = std::max(alignof(std::max_align_t), static_cast<std::size_t>(64));
#endif
#elif MPT_OS_DJGPP
// work-around maximum alignment in object file
inline constexpr std::size_t cacheline_size_min = std::min(static_cast<std::size_t>(16), std::max(alignof(std::max_align_t), static_cast<std::size_t>(64)));
inline constexpr std::size_t cacheline_size_max = std::min(static_cast<std::size_t>(16), std::max(alignof(std::max_align_t), static_cast<std::size_t>(64)));
#else
inline constexpr std::size_t cacheline_size_min = std::max(alignof(std::max_align_t), static_cast<std::size_t>(64));
inline constexpr std::size_t cacheline_size_max = std::max(alignof(std::max_align_t), static_cast<std::size_t>(64));
#endif

#else

#if MPT_OS_DJGPP

// work-around maximum alignment in object file
inline constexpr std::size_t cacheline_size_min = std::min(static_cast<std::size_t>(16), std::max(alignof(std::max_align_t), static_cast<std::size_t>(64)));
inline constexpr std::size_t cacheline_size_max = std::min(static_cast<std::size_t>(16), std::max(alignof(std::max_align_t), static_cast<std::size_t>(64)));

#else

#if MPT_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#elif MPT_COMPILER_GENERIC && defined(__GNUC__)
// GCC stupidly warns for any use of std::hardware_destructive_interference_size
// unless the warning is explicitly disabled or the value is set on the command line
// via `--param hardware_destructive_interference_size`.
// We cannot do that in standard-compliant builds because we explicitly opt-out of
// any compiler-specific knowledge.
// Special casse GCC even in generic mode here because we are left with no choice.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winterference-size"
#endif // MPT_COMPILER_GCC
inline constexpr std::size_t cacheline_size_min = std::hardware_constructive_interference_size;
inline constexpr std::size_t cacheline_size_max = std::hardware_destructive_interference_size;
#if MPT_COMPILER_GCC
#pragma GCC diagnostic pop
#elif MPT_COMPILER_GENERIC && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // MPT_COMPILER_GCC

#endif

#endif



enum class clock_frequency_mode {
	fast,
	slow,
	optional,
};



#if (defined(MPT_ENABLE_ARCH_X86) || defined(MPT_ENABLE_ARCH_AMD64)) && defined(MPT_ARCH_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_TSC)

struct tsc_clock {
	using rep = uint64;
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//using period = std::ratio;
	//using duration = std::chrono::duration<rep, period>;
	//using time_point = std::chrono::time_point<tsc_clock>;
#endif
	static inline constexpr bool is_steady = false;
	static inline constexpr clock_frequency_mode frequency_mode = clock_frequency_mode::optional;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		uint64 frequency = mpt::arch::get_cpu_info().get_tsc_frequency();
		if (frequency == 0) {
			return std::nullopt;
		}
		return static_cast<double>(frequency);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return __rdtsc();
	}
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept;
#endif
};

#endif


#if MPT_OS_WINDOWS

struct QueryPerformanceCounter_clock {
	using rep = uint64;
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//using period = std::ratio;
	//using duration = std::chrono::duration<rep, period>;
	//using time_point = std::chrono::time_point<QueryPerformanceCounter_clock>;
#endif
	static inline constexpr bool is_steady = false;
	static inline constexpr clock_frequency_mode frequency_mode = clock_frequency_mode::fast;
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
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept;
#endif
};

struct GetTickCount_clock {
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	using rep = uint64;
#else
	using rep = uint32;
#endif
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	using period = std::milli;
	using duration = std::chrono::duration<rep, period>;
	using time_point = std::chrono::time_point<GetTickCount_clock>;
#endif
	static inline constexpr bool is_steady = true;
	static inline constexpr clock_frequency_mode frequency_mode = clock_frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return 1000.0_sf64;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
		return GetTickCount64();
#else
		return GetTickCount();
#endif
	}
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return time_point{duration{std::chrono::milliseconds{now_raw()}}};
	}
#endif
};
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<GetTickCount_clock>::value);
#endif

#endif


#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)

struct high_resolution_clock {
	using rep = std::chrono::high_resolution_clock::rep;
	using period = std::chrono::high_resolution_clock::period;
	using duration = std::chrono::high_resolution_clock::duration;
	using time_point = std::chrono::high_resolution_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::high_resolution_clock::is_steady;
	static inline constexpr clock_frequency_mode frequency_mode = clock_frequency_mode::fast;
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
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<high_resolution_clock>::value);
#endif

struct steady_clock {
	using rep = std::chrono::steady_clock::rep;
	using period = std::chrono::steady_clock::period;
	using duration = std::chrono::steady_clock::duration;
	using time_point = std::chrono::steady_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::steady_clock::is_steady;
	static inline constexpr clock_frequency_mode frequency_mode = clock_frequency_mode::fast;
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
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<steady_clock>::value);
#endif

struct system_clock {
	using rep = std::chrono::system_clock::rep;
	using period = std::chrono::system_clock::period;
	using duration = std::chrono::system_clock::duration;
	using time_point = std::chrono::system_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::system_clock::is_steady;
	static inline constexpr clock_frequency_mode frequency_mode = clock_frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return static_cast<double>(period::den) / static_cast<double>(period::num);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return std::chrono::system_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return std::chrono::system_clock::now().time_since_epoch().count();
	}
};
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<system_clock>::value);
#endif

#endif


struct default_system_clock {
	using rep = mpt::chrono::default_system_clock::rep;
	//using period = mpt::chrono::default_system_clock::period;
	using duration = mpt::chrono::default_system_clock::duration;
	using time_point = mpt::chrono::default_system_clock::time_point;
	static inline constexpr bool is_steady = mpt::chrono::default_system_clock::is_steady;
	static inline constexpr clock_frequency_mode frequency_mode = clock_frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return 1'000'000'000.0;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return mpt::chrono::default_system_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return mpt::chrono::default_system_clock::to_unix_nanoseconds(mpt::chrono::default_system_clock::now());
	}
};


#if (defined(MPT_ENABLE_ARCH_X86) || defined(MPT_ENABLE_ARCH_AMD64)) && defined(MPT_ARCH_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_TSC)

using highres_clock = tsc_clock;
using fast_clock = tsc_clock;
using default_clock = tsc_clock;

#elif MPT_OS_WINDOWS

using highres_clock = QueryPerformanceCounter_clock;
using fast_clock = GetTickCount_clock;
using default_clock = QueryPerformanceCounter_clock;

#elif !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)

using highres_clock = high_resolution_clock;
using fast_clock = steady_clock;
using default_clock = high_resolution_clock;

#else

using highres_clock = default_system_clock;
using fast_clock = default_system_clock;
using default_clock = default_system_clock;

#endif



namespace detail {


#if 1
template <typename T>
struct measurement_base {
	mpt::chrono::system_clock::time_point wallclock;
	T perfclock;
	bool valid;
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE constexpr measurement_base(std::chrono::system_clock::time_point wc, T pc, bool b) noexcept
		: wallclock(wc)
		, perfclock(pc)
		, valid(b) {
		return;
	}
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE constexpr measurement_base() noexcept
	: wallclock(std::chrono::system_clock::time_point{})
	, perfclock(T{})
	, valid(false)
	{}
};
template <typename T>
struct alignas(mpt::profiler::cacheline_size_max) data_base {
	std::atomic<mpt::somefloat64> frequency;
	std::atomic<measurement_base<T>> old;
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE constexpr data_base() noexcept
		: frequency(0.0_sf64)
		, old(measurement_base<T>{std::chrono::system_clock::time_point{}, T{}, false}) {
		return;
	}
};
#endif

template <typename Clock>
class frequency_estimator {
private:
	#if defined(MPT_ARCH_X86_CX8)
		static_assert(std::atomic<mpt::somefloat64>::is_always_lock_free);
	#endif
#if 0
	struct measurement {
		mpt::chrono::system_clock::time_point wallclock{};
		typename Clock::rep perfclock{};
		bool valid = false;
	};
	struct alignas(mpt::profiler::cacheline_size_max) data {
		std::atomic<mpt::somefloat64> frequency = 0.0_sf64;
		std::atomic<measurement> old = measurement{};
	};
#else
	using measurement = measurement_base<typename Clock::rep>;
	using data = data_base<typename Clock::rep>;
#endif
	inline static data g_data{};
public:
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE static void estimate() {
		measurement now{};
		typename Clock::rep beg = Clock::now_raw();
		now.wallclock = mpt::chrono::system_clock::now();
		typename Clock::rep end = Clock::now_raw();
		if (end < beg) {
			return;
		}
#if MPT_CXX_AT_LEAST(20)
		now.perfclock = std::midpoint(beg, end);
#else
		now.perfclock = (beg / 2) + (end / 2);
#endif
		now.valid = true;
		measurement old = g_data.old.exchange(now, std::memory_order_relaxed);
		if (!old.valid) {
			return;
		}
		if (now.wallclock <= old.wallclock) {
			return;
		}
		mpt::somefloat64 frequency = 1'000'000'000.0_sf64 * (now.perfclock - old.perfclock) / static_cast<mpt::somefloat64>(mpt::chrono::system_clock::to_unix_nanoseconds(now.wallclock) - mpt::chrono::system_clock::to_unix_nanoseconds(old.wallclock));
		if (frequency > 0.0_sf64) {
			g_data.frequency.store(frequency, std::memory_order_relaxed);
		}
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static mpt::somefloat64 get_frequency() noexcept {
		return g_data.frequency.load(std::memory_order_relaxed);
	}
};


template <typename Clock>
class frequency_cache {
public:
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		alignas(mpt::profiler::cacheline_size_max) static std::optional<mpt::somefloat64> s_frequency = Clock::get_frequency();
		return s_frequency;
	}
};


} // namespace detail



template <typename Clock>
MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE void estimate_frequency() {
	if constexpr (Clock::frequency_mode == clock_frequency_mode::optional) {
		mpt::profiler::detail::frequency_estimator<Clock>::estimate();
	}
}



MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE void estimate_all_frequencies() {
	if constexpr (std::is_same<mpt::profiler::highres_clock, mpt::profiler::fast_clock>::value) {
		assert((std::is_same<mpt::profiler::default_clock, mpt::profiler::highres_clock>::value));
		assert((std::is_same<mpt::profiler::default_clock, mpt::profiler::fast_clock>::value));
		if constexpr (mpt::profiler::default_clock::frequency_mode == mpt::profiler::clock_frequency_mode::optional) {
			mpt::profiler::estimate_frequency<mpt::profiler::default_clock>();
		}
	} else {
		assert((!std::is_same<mpt::profiler::highres_clock, mpt::profiler::fast_clock>::value));
		assert((std::is_same<mpt::profiler::default_clock, mpt::profiler::highres_clock>::value || std::is_same<mpt::profiler::default_clock, mpt::profiler::fast_clock>::value));
		if constexpr (mpt::profiler::highres_clock::frequency_mode == mpt::profiler::clock_frequency_mode::optional) {
			mpt::profiler::estimate_frequency<mpt::profiler::highres_clock>();
		}
		if constexpr (mpt::profiler::fast_clock::frequency_mode == mpt::profiler::clock_frequency_mode::optional) {
			mpt::profiler::estimate_frequency<mpt::profiler::fast_clock>();
		}
	}
}


template <typename Clock>
[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE std::optional<mpt::somefloat64> get_frequency() noexcept {
	std::optional<mpt::somefloat64> frequency;
	if constexpr (Clock::frequency_mode == clock_frequency_mode::slow) {
		frequency = mpt::profiler::detail::frequency_cache<Clock>::get_frequency();
	} else {
		frequency = Clock::get_frequency();
	}
	if (!frequency.has_value()) {
		frequency = mpt::profiler::detail::frequency_estimator<Clock>::get_frequency();
	}
	return frequency;
}



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
