/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_CLOCK_STD_CHRONO_HPP
#define MPT_PROFILER_CLOCK_STD_CHRONO_HPP



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



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace profiler {

namespace clock {



#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)

#define MPT_PROFILER_CLOCK_STD_CHRONO 1

struct chrono_high_resolution {
	using rep = std::chrono::high_resolution_clock::rep;
	using period = std::chrono::high_resolution_clock::period;
	using duration = std::chrono::high_resolution_clock::duration;
	using time_point = std::chrono::high_resolution_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::high_resolution_clock::is_steady;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return static_cast<double>(period::den) / static_cast<double>(period::num);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return std::chrono::high_resolution_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return std::chrono::high_resolution_clock::now().time_since_epoch().count();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter() noexcept {
		return now_raw();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave() noexcept {
		return now_raw();
	}
};
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<chrono_high_resolution>::value);
#endif

struct chrono_steady {
	using rep = std::chrono::steady_clock::rep;
	using period = std::chrono::steady_clock::period;
	using duration = std::chrono::steady_clock::duration;
	using time_point = std::chrono::steady_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::steady_clock::is_steady;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return static_cast<double>(period::den) / static_cast<double>(period::num);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return std::chrono::steady_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return std::chrono::steady_clock::now().time_since_epoch().count();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter() noexcept {
		return now_raw();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave() noexcept {
		return now_raw();
	}
};
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<chrono_steady>::value);
#endif

struct chrono_system {
	using rep = std::chrono::system_clock::rep;
	using period = std::chrono::system_clock::period;
	using duration = std::chrono::system_clock::duration;
	using time_point = std::chrono::system_clock::time_point;
	static inline constexpr bool is_steady = std::chrono::system_clock::is_steady;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return static_cast<double>(period::den) / static_cast<double>(period::num);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return std::chrono::system_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return std::chrono::system_clock::now().time_since_epoch().count();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter() noexcept {
		return now_raw();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave() noexcept {
		return now_raw();
	}
};
#if MPT_CXX_AT_LEAST(20) && !defined(LIBCXX_QUIRK_NO_CHRONO_IS_CLOCK)
static_assert(std::chrono::is_clock<chrono_system>::value);
#endif

#else // MPT_LIBCXX_QUIRK_NO_CHRONO

#define MPT_PROFILER_CLOCK_STD_CHRONO 0

#endif // !MPT_LIBCXX_QUIRK_NO_CHRONO



} // namespace clock

} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_CLOCK_STD_CHRONO_HPP
