/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_CLOCK_QUERY_PERFORMANCE_COUNTER_HPP
#define MPT_PROFILER_CLOCK_QUERY_PERFORMANCE_COUNTER_HPP



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

#define MPT_PROFILER_CLOCK_QUERY_PERFORMANCE_COUNTER 1

struct query_performance_counter {
	using rep = uint64;
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//using period = std::ratio;
	//using duration = std::chrono::duration<rep, period>;
	//using time_point = std::chrono::time_point<query_performance_counter>;
#endif
	static inline constexpr bool is_steady = false;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::fast;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		LARGE_INTEGER result{};
		QueryPerformanceFrequency(&result);
		return static_cast<double>(result.QuadPart);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw(uint32 * core_id = nullptr) noexcept {
		LARGE_INTEGER result{};
		if (core_id) {
#if MPT_WINNT_AT_LEAST(MPT_WIN_7)
			PROCESSOR_NUMBER pn{};
			GetCurrentProcessorNumberEx(&pn);
			*core_id = (static_cast<uint32>(static_cast<uint16>(pn.Group)) << 8) | static_cast<uint8>(pn.Number);
#else
			*core_id = GetCurrentProcessorNumber();
#endif
		}
		QueryPerformanceCounter(&result);
		return result.QuadPart;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter(uint32 * core_id = nullptr) noexcept {
		LARGE_INTEGER result{};
		if (core_id) {
#if MPT_WINNT_AT_LEAST(MPT_WIN_7)
			PROCESSOR_NUMBER pn{};
			GetCurrentProcessorNumberEx(&pn);
			*core_id = (static_cast<uint32>(static_cast<uint16>(pn.Group)) << 8) | static_cast<uint8>(pn.Number);
#else
			*core_id = GetCurrentProcessorNumber();
#endif
		}
		QueryPerformanceCounter(&result);
		return result.QuadPart;
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave(uint32 * core_id = nullptr) noexcept {
		LARGE_INTEGER result{};
		QueryPerformanceCounter(&result);
		if (core_id) {
#if MPT_WINNT_AT_LEAST(MPT_WIN_7)
			PROCESSOR_NUMBER pn{};
			GetCurrentProcessorNumberEx(&pn);
			*core_id = (static_cast<uint32>(static_cast<uint16>(pn.Group)) << 8) | static_cast<uint8>(pn.Number);
#else
			*core_id = GetCurrentProcessorNumber();
#endif
		}
		return result.QuadPart;
	}
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept;
#endif
};

#else // !MPT_OS_WINDOWS

#define MPT_PROFILER_CLOCK_QUERY_PERFORMANCE_COUNTER 0

#endif // MPT_OS_WINDOWS



} // namespace clock

} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_CLOCK_QUERY_PERFORMANCE_COUNTER_HPP
