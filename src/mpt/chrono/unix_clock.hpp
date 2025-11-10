/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_CHRONO_UNIX_CLOCK_HPP
#define MPT_CHRONO_UNIX_CLOCK_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"

#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <chrono>
#endif

#if defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <ctime>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace chrono {



struct unix_clock {

	using rep = int64;

	// int64 counts 1s since 1970-01-01T00:00Z
	struct time_point {
		int64 seconds = 0;
		int32 nanoseconds = 0;
		friend bool operator==(const time_point & a, const time_point & b) {
			return a.seconds == b.seconds && a.nanoseconds == b.nanoseconds;
		}
		friend bool operator!=(const time_point & a, const time_point & b) {
			return a.seconds != b.seconds || a.nanoseconds != b.nanoseconds;
		}
	};

	using duration = int64;

	static inline constexpr bool is_steady = false;

	static int64 to_unix_seconds(time_point tp) {
		return tp.seconds;
	}

	static int64 to_unix_nanoseconds(time_point tp) {
		return (tp.seconds * 1'000'000'000) + tp.nanoseconds;
	}

	static time_point from_unix_seconds(int64 seconds) {
		return time_point{static_cast<int64>(seconds), static_cast<int32>(0)};
	}

	static time_point from_unix_nanoseconds(int64 nanoseconds) {
		return time_point{static_cast<int64>(nanoseconds / 1'000'000'000ll), static_cast<int32>(nanoseconds % 1'000'000'000ll)};
	}

	static time_point now() {
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
		return from_unix_nanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
#else
		return from_unix_seconds(static_cast<int64>(std::time(nullptr)));
#endif
	}

}; // unix_clock



} // namespace chrono



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_CHRONO_UNIX_CLOCK_HPP
