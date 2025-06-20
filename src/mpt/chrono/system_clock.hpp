/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CHRONO_SYSTEM_CLOCK_HPP
#define MPT_CHRONO_SYSTEM_CLOCK_HPP



#include "mpt/base/detect.hpp"

#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"
#if defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include "mpt/chrono/unix_clock.hpp"
#endif

#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <chrono>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace chrono {



#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)

struct system_clock {

	using time_point = std::chrono::system_clock::time_point;

	using duration = std::chrono::system_clock::duration;

	static int64 to_unix_seconds(time_point tp) {
		return static_cast<int64>(std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count());
	}

	static int64 to_unix_nanoseconds(time_point tp) {
		return static_cast<int64>(std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count());
	}

	static time_point from_unix_seconds(int64 seconds) {
		return std::chrono::system_clock::time_point{std::chrono::seconds{seconds}};
	}

	static time_point from_unix_nanoseconds(int64 nanoseconds) {
		return std::chrono::system_clock::time_point{std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds{nanoseconds})};
	}

	static time_point now() {
		return std::chrono::system_clock::now();
	}

}; // system_clock

#endif



#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
using default_system_clock = system_clock;
#else
using default_system_clock = unix_clock;
#endif



} // namespace chrono



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_CHRONO_SYSTEM_CLOCK_HPP
