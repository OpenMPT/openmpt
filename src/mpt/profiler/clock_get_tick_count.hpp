/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_CLOCK_GET_TICK_COUNT_HPP
#define MPT_PROFILER_CLOCK_GET_TICK_COUNT_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/float.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/profiler/clock_base.hpp"

#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <chrono>
#endif
#include <optional>

#if MPT_OS_WINDOWS
#include <windows.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace profiler {

namespace clock {



#if MPT_OS_WINDOWS

#define MPT_PROFILER_CLOCK_GET_TICK_COUNT 1

struct get_tick_count {
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	using rep = uint64;
#else
	using rep = uint32;
#endif
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	using period = std::milli;
	using duration = std::chrono::duration<rep, period>;
	using time_point = std::chrono::time_point<get_tick_count>;
#endif
	static inline constexpr bool is_steady = true;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::fast;
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
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter() noexcept {
		return now_raw();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave() noexcept {
		return now_raw();
	}
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return time_point{duration{std::chrono::milliseconds{now_raw()}}};
	}
#endif
};
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<get_tick_count>::value);
#endif

#else // !MPT_OS_WINDOWS

#define MPT_PROFILER_CLOCK_GET_TICK_COUNT 0

#endif // MPT_OS_WINDOWS



} // namespace clock

} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_CLOCK_GET_TICK_COUNT_HPP
