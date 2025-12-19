/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_CLOCK_SYSTEM_HPP
#define MPT_PROFILER_CLOCK_SYSTEM_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/float.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/chrono/system_clock.hpp"
#include "mpt/profiler/clock_base.hpp"

#include <optional>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace profiler {

namespace clock {



#define MPT_PROFILER_CLOCK_SYSTEM 1

struct system {
	using rep = mpt::chrono::default_system_clock::rep;
	//using period = mpt::chrono::default_system_clock::period;
	using duration = mpt::chrono::default_system_clock::duration;
	using time_point = mpt::chrono::default_system_clock::time_point;
	static inline constexpr bool is_steady = mpt::chrono::default_system_clock::is_steady;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		return 1'000'000'000.0;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept {
		return mpt::chrono::default_system_clock::now();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw() noexcept {
		return mpt::chrono::default_system_clock::to_unix_nanoseconds(mpt::chrono::default_system_clock::now());
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter() noexcept {
		return now_raw();
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave() noexcept {
		return now_raw();
	}
};



} // namespace clock

} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_CLOCK_SYSTEM_HPP
