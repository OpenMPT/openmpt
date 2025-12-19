/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_FREQUENCY_HPP
#define MPT_PROFILER_FREQUENCY_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/float.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/chrono/system_clock.hpp"
#include "mpt/profiler/clock.hpp"
#include "mpt/profiler/clock_base.hpp"

#include <atomic>
#include <new>
#include <numeric>
#include <optional>

#include <cassert>
#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace profiler {



#if defined(MPT_LIBCXX_QUIRK_NO_HARDWARE_INTERFERENCE_SIZE)

// guess
#if MPT_OS_MACOSX_OR_IOS
#if MPT_ARCH_AARCH64
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
#elif MPT_COMPILER_GENERIC && defined(__GNUC__) && !defined(__clang__)
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
#elif MPT_COMPILER_GENERIC && defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif // MPT_COMPILER_GCC

#endif

#endif



namespace detail {



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
		, valid(false) { }
};
template <typename T>
struct alignas(cacheline_size_max) data_base {
	std::atomic<mpt::somefloat64> frequency;
	std::atomic<measurement_base<T>> old;
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE constexpr data_base() noexcept
		: frequency(0.0_sf64)
		, old(measurement_base<T>{std::chrono::system_clock::time_point{}, T{}, false}) {
		return;
	}
};

template <typename Clock>
class frequency_estimator {
private:
#if defined(MPT_ARCH_X86_CX8)
	static_assert(std::atomic<mpt::somefloat64>::is_always_lock_free);
#endif
	using measurement = measurement_base<typename Clock::rep>;
	using data = data_base<typename Clock::rep>;
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
		mpt::somefloat64 frequency = 1'000'000'000.0_sf64 * static_cast<mpt::somefloat64>(now.perfclock - old.perfclock) / static_cast<mpt::somefloat64>(mpt::chrono::system_clock::to_unix_nanoseconds(now.wallclock) - mpt::chrono::system_clock::to_unix_nanoseconds(old.wallclock));
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
		alignas(cacheline_size_max) static std::optional<mpt::somefloat64> s_frequency = Clock::get_frequency();
		return s_frequency;
	}
};



} // namespace detail



template <typename Clock>
MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE void estimate_frequency() {
	if constexpr (Clock::frequency_mode == clock::frequency_mode::optional) {
		detail::frequency_estimator<Clock>::estimate();
	}
}



MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE void estimate_all_frequencies() {
	if constexpr (std::is_same<highres_clock, fast_clock>::value) {
		assert((std::is_same<default_clock, highres_clock>::value));
		assert((std::is_same<default_clock, fast_clock>::value));
		if constexpr (default_clock::frequency_mode == clock::frequency_mode::optional) {
			estimate_frequency<default_clock>();
		}
	} else {
		assert((!std::is_same<highres_clock, fast_clock>::value));
		assert((std::is_same<default_clock, highres_clock>::value || std::is_same<default_clock, fast_clock>::value));
		if constexpr (highres_clock::frequency_mode == clock::frequency_mode::optional) {
			estimate_frequency<highres_clock>();
		}
		if constexpr (fast_clock::frequency_mode == clock::frequency_mode::optional) {
			estimate_frequency<fast_clock>();
		}
	}
}


template <typename Clock>
[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE std::optional<mpt::somefloat64> get_frequency() noexcept {
	std::optional<mpt::somefloat64> frequency;
	if constexpr (Clock::frequency_mode == clock::frequency_mode::slow) {
		frequency = detail::frequency_cache<Clock>::get_frequency();
	} else {
		frequency = Clock::get_frequency();
	}
	if (!frequency.has_value()) {
		frequency = detail::frequency_estimator<Clock>::get_frequency();
	}
	return frequency;
}



} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_FREQUENCY_HPP
