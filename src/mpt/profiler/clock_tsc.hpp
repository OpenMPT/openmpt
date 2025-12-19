/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_CLOCK_TSC_HPP
#define MPT_PROFILER_CLOCK_TSC_HPP



#include "mpt/arch/arch.hpp"
#include "mpt/arch/feature_fence.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/float.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/chrono/system_clock.hpp"
#include "mpt/profiler/clock_base.hpp"

#include <atomic>
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
#include <chrono>
#endif
#include <optional>

#if (defined(MPT_ENABLE_ARCH_X86) || defined(MPT_ENABLE_ARCH_AMD64)) && defined(MPT_ARCH_INTRINSICS_X86_TSC)
#if MPT_COMPILER_CLANG
#include <x86intrin.h>
#elif MPT_COMPILER_GCC
#include <x86intrin.h>
#elif MPT_COMPILER_MSVC
#include <intrin.h>
#endif
#include <immintrin.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace profiler {

namespace clock {



#if (defined(MPT_ENABLE_ARCH_X86) || defined(MPT_ENABLE_ARCH_AMD64)) && defined(MPT_ARCH_INTRINSICS_X86_TSC)



#if defined(MPT_ARCH_X86_TSC) 



#define MPT_PROFILER_CLOCK_TSC 1



struct tsc {
	using rep = uint64;
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//using period = std::ratio;
	//using duration = std::chrono::duration<rep, period>;
	//using time_point = std::chrono::time_point<tsc>;
#endif
	static inline constexpr bool is_steady = false;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::optional;
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		uint64 frequency = mpt::arch::get_cpu_info().get_tsc_frequency();
		if (frequency == 0) {
			return std::nullopt;
		}
		return static_cast<double>(frequency);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw([[maybe_unused]] std::memory_order order = std::memory_order_relaxed, uint32 * core_id = nullptr) noexcept {
		std::atomic_signal_fence(order);
		#if defined(MPT_ARCH_X86_RDTSCP) && defined(MPT_ARCH_INTRINSICS_X86_RDTSCP)
			if (core_id) {
				#if defined(MPT_ARCH_X86_CPUID) && defined(MPT_ARCH_INTRINSICS_X86_CPUID) && defined(MPT_ARCH_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_TSC)
					if (order == std::memory_order_seq_cst) {
						MPT_DISCARD(mpt::arch::current::cpu_info::cpuid(0x0000'0000u));
					}
				#else
					order = std::memory_order_acq_rel;
				#endif
				#if defined(MPT_ARCH_X86_SSE2) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
					if (order == std::memory_order_acq_rel) {
						_mm_mfence();
						_mm_lfence();
					}
					if (order == std::memory_order_release) {
						_mm_mfence();
						_mm_lfence();
					}
					if (order == std::memory_order_acquire) {
						_mm_lfence();
					}
				#endif
				if constexpr (std::is_same<unsigned int, uint32>::value) {
					return __rdtscp(core_id);
				} else {
					unsigned int id;
					uint64 result = __rdtscp(&id);
					*core_id = id;
					return result;
				}
			}
		#endif
		#if defined(MPT_ARCH_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_TSC)
			#if defined(MPT_ARCH_X86_CPUID) && defined(MPT_ARCH_INTRINSICS_X86_CPUID)
				if (order == std::memory_order_seq_cst) {
					MPT_DISCARD(mpt::arch::current::cpu_info::cpuid(0x0000'0000u));
					return __rdtsc();
				}
			#else
				order = std::memory_order_acq_rel;
			#endif
			#if defined(MPT_ARCH_X86_SSE) && defined(MPT_ARCH_INTRINSICS_X86_SSE) && defined(MPT_ARCH_X86_SSE2) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
				if (order == std::memory_order_acq_rel) {
					_mm_mfence();
					_mm_lfence();
				}
				if (order == std::memory_order_release) {
					_mm_mfence();
					_mm_lfence();
				}
				if (order == std::memory_order_acquire) {
					_mm_lfence();
				}
				if (order == std::memory_order_consume) {
					_mm_sfence();
				}
			#elif defined(MPT_ARCH_X86_SSE) && defined(MPT_ARCH_INTRINSICS_X86_SSE)
				if (order == std::memory_order_acq_rel) {
					_mm_sfence();
				}
				if (order == std::memory_order_release) {
					_mm_sfence();
				}
				if (order == std::memory_order_acquire) {
					_mm_sfence();
				}
				if (order == std::memory_order_consume) {
					_mm_sfence();
				}
			#endif
			if (core_id) {
				*core_id = 0xffff'ffffu;
			}
			return __rdtsc();
		#else
			if (core_id) {
				*core_id = 0xffff'ffffu;
			}
			return 0xffff'ffff'ffff'ffffull;
		#endif
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter(std::memory_order order = std::memory_order_relaxed, uint32 * core_id = nullptr) noexcept {
		if (order == std::memory_order_acq_rel) {
			order = std::memory_order_acquire;
		}
		return now_raw(order, core_id);
	}
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave(std::memory_order order = std::memory_order_relaxed, uint32 * core_id = nullptr) noexcept {
		if (order == std::memory_order_acq_rel) {
			order = std::memory_order_release;
		}
		return now_raw(order, core_id);
	}
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept;
#endif
};

#endif // MPT_ARCH_X86_TSC 



#define MPT_PROFILER_CLOCK_TSC_RUNTIME 1



struct tsc_runtime {

	using rep = uint64;
#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//using period = std::ratio;
	//using duration = std::chrono::duration<rep, period>;
	//using time_point = std::chrono::time_point<tsc_runtime>;
#endif

	static inline constexpr bool is_steady = false;
	static inline constexpr clock::frequency_mode frequency_mode = clock::frequency_mode::optional;

	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<mpt::somefloat64> get_frequency() noexcept {
		uint64 frequency = mpt::arch::get_cpu_info().get_tsc_frequency();
		if (frequency == 0) {
			return std::nullopt;
		}
		return static_cast<double>(frequency);
	}

	inline static rep(* rdtsc_relaxed)() noexcept = nullptr;
	inline static rep(* rdtsc_consume)() noexcept = nullptr;
	inline static rep(* rdtsc_acquire)() noexcept = nullptr;
	inline static rep(* rdtsc_release)() noexcept = nullptr;
	inline static rep(* rdtsc_acq_rel)() noexcept = nullptr;
	inline static rep(* rdtsc_seq_cst)() noexcept = nullptr;
	inline static rep(* rdtscp_relaxed)(uint32 * core_id) noexcept = nullptr;
	inline static rep(* rdtscp_consume)(uint32 * core_id) noexcept = nullptr;
	inline static rep(* rdtscp_acquire)(uint32 * core_id) noexcept = nullptr;
	inline static rep(* rdtscp_release)(uint32 * core_id) noexcept = nullptr;
	inline static rep(* rdtscp_acq_rel)(uint32 * core_id) noexcept = nullptr;
	inline static rep(* rdtscp_seq_cst)(uint32 * core_id) noexcept = nullptr;

	static rep rdtsc_relaxed_dummy() noexcept {
		return 0xffff'ffff'ffff'ffffull;
	}

#if defined(MPT_ARCH_INTRINSICS_X86_TSC)
	static rep rdtsc_relaxed_tsc() noexcept {
		return __rdtsc();
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE)
	static rep rdtsc_consume_tsc_sse() noexcept {
		_mm_sfence();
		return __rdtsc();
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
	static rep rdtsc_consume_tsc_sse2() noexcept {
		_mm_sfence();
		return __rdtsc();
	}
	static rep rdtsc_acquire_tsc_sse2() noexcept {
		_mm_lfence();
		return __rdtsc();
	}
	static rep rdtsc_release_tsc_sse2() noexcept {
		_mm_mfence();
		_mm_lfence();
		return __rdtsc();
	}
	static rep rdtsc_acq_rel_tsc_sse2() noexcept {
		_mm_mfence();
		_mm_lfence();
		return __rdtsc();
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_CPUID)
	static rep rdtsc_seq_cst_tsc_cpuid() noexcept {
		MPT_DISCARD(mpt::arch::current::cpu_info::cpuid(0x0000'0000u));
		return __rdtsc();
	}
#endif

	static rep rdtscp_relaxed_dummy(uint32 * core_id) noexcept {
		*core_id = 0xffff'ffffu;
		return 0xffff'ffff'ffff'ffffull;
	}

#if defined(MPT_ARCH_INTRINSICS_X86_TSC)
	static rep rdtscp_relaxed_tsc(uint32 * core_id) noexcept {
		*core_id = 0xffff'ffffu;
		return __rdtsc();
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE)
	static rep rdtscp_consume_tsc_sse(uint32 * core_id) noexcept {
		_mm_sfence();
		*core_id = 0xffff'ffffu;
		return __rdtsc();
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
	static rep rdtscp_consume_tsc_sse2(uint32 * core_id) noexcept {
		_mm_sfence();
		*core_id = 0xffff'ffffu;
		return __rdtsc();
	}
	static rep rdtscp_acquire_tsc_sse2(uint32 * core_id) noexcept {
		_mm_lfence();
		*core_id = 0xffff'ffffu;
		return __rdtsc();
	}
	static rep rdtscp_release_tsc_sse2(uint32 * core_id) noexcept {
		_mm_mfence();
		_mm_lfence();
		*core_id = 0xffff'ffffu;
		return __rdtsc();
	}
	static rep rdtscp_acq_rel_tsc_sse2(uint32 * core_id) noexcept {
		_mm_mfence();
		_mm_lfence();
		*core_id = 0xffff'ffffu;
		return __rdtsc();
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_CPUID)
	static rep rdtscp_seq_cst_tsc_cpuid(uint32 * core_id) noexcept {
		MPT_DISCARD(mpt::arch::current::cpu_info::cpuid(0x0000'0000u));
		*core_id = 0xffff'ffffu;
		return __rdtsc();
	}
#endif

#if defined(MPT_ARCH_INTRINSICS_X86_RDTSCP)
	static rep rdtscp_relaxed_rdtscp(uint32 * core_id) noexcept {
		if constexpr (std::is_same<unsigned int, uint32>::value) {
			return __rdtscp(core_id);
		} else {
			unsigned int id;
			uint64 result = __rdtscp(&id);
			*core_id = id;
			return result;
		}
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_RDTSCP) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
	static rep rdtscp_consume_sse2_rdtscp(uint32 * core_id) noexcept {
		if constexpr (std::is_same<unsigned int, uint32>::value) {
			return __rdtscp(core_id);
		} else {
			unsigned int id;
			uint64 result = __rdtscp(&id);
			*core_id = id;
			return result;
		}
	}
	static rep rdtscp_acquire_sse2_rdtscp(uint32 * core_id) noexcept {
		_mm_lfence();
		if constexpr (std::is_same<unsigned int, uint32>::value) {
			return __rdtscp(core_id);
		} else {
			unsigned int id;
			uint64 result = __rdtscp(&id);
			*core_id = id;
			return result;
		}
	}
	static rep rdtscp_release_sse2_rdtscp(uint32 * core_id) noexcept {
		_mm_mfence();
		_mm_lfence();
		if constexpr (std::is_same<unsigned int, uint32>::value) {
			return __rdtscp(core_id);
		} else {
			unsigned int id;
			uint64 result = __rdtscp(&id);
			*core_id = id;
			return result;
		}
	}
	static rep rdtscp_acq_rel_sse2_rdtscp(uint32 * core_id) noexcept {
		_mm_mfence();
		_mm_lfence();
		if constexpr (std::is_same<unsigned int, uint32>::value) {
			return __rdtscp(core_id);
		} else {
			unsigned int id;
			uint64 result = __rdtscp(&id);
			*core_id = id;
			return result;
		}
	}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_RDTSCP) && defined(MPT_ARCH_INTRINSICS_X86_CPUID)
	static rep rdtscp_seq_cst_rdtscp_cpuid(uint32 * core_id) noexcept {
		MPT_DISCARD(mpt::arch::current::cpu_info::cpuid(0x0000'0000u));
		if constexpr (std::is_same<unsigned int, uint32>::value) {
			return __rdtscp(core_id);
		} else {
			unsigned int id;
			uint64 result = __rdtscp(&id);
			*core_id = id;
			return result;
		}
	}
#endif

	static void init() {

		const mpt::arch::cpu_info & cpu_info = mpt::arch::get_cpu_info();

		rdtsc_relaxed = &rdtsc_relaxed_dummy;
		rdtscp_relaxed = &rdtscp_relaxed_dummy;
		
#if defined(MPT_ARCH_INTRINSICS_X86_TSC)
		if (cpu_info.has_features(mpt::arch::current::feature::tsc)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtsc_relaxed = &rdtsc_relaxed_tsc;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE)
		if (cpu_info.has_features(mpt::arch::current::feature::tsc | mpt::arch::current::feature::sse)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtsc_consume = &rdtsc_consume_tsc_sse;
			rdtsc_acquire = &rdtsc_consume_tsc_sse;
			rdtsc_release = &rdtsc_consume_tsc_sse;
			rdtsc_acq_rel = &rdtsc_consume_tsc_sse;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
		if (cpu_info.has_features(mpt::arch::current::feature::tsc | mpt::arch::current::feature::sse | mpt::arch::current::feature::sse2)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtsc_consume = &rdtsc_consume_tsc_sse2;
			rdtsc_acquire = &rdtsc_acquire_tsc_sse2;
			rdtsc_release = &rdtsc_release_tsc_sse2;
			rdtsc_acq_rel = &rdtsc_acq_rel_tsc_sse2;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_CPUID)
		if (cpu_info.has_features(mpt::arch::current::feature::tsc | mpt::arch::current::feature::cpuid)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtsc_seq_cst = &rdtsc_seq_cst_tsc_cpuid;
		}
#endif

#if defined(MPT_ARCH_INTRINSICS_X86_TSC)
		if (!cpu_info.has_features(mpt::arch::current::feature::rdtscp) && cpu_info.has_features(mpt::arch::current::feature::tsc)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtscp_relaxed = &rdtscp_relaxed_tsc;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE)
		if (!cpu_info.has_features(mpt::arch::current::feature::rdtscp) && cpu_info.has_features(mpt::arch::current::feature::tsc | mpt::arch::current::feature::sse)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtscp_consume = &rdtscp_consume_tsc_sse;
			rdtscp_acquire = &rdtscp_consume_tsc_sse;
			rdtscp_release = &rdtscp_consume_tsc_sse;
			rdtscp_acq_rel = &rdtscp_consume_tsc_sse;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_SSE) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
		if (!cpu_info.has_features(mpt::arch::current::feature::rdtscp) && cpu_info.has_features(mpt::arch::current::feature::tsc | mpt::arch::current::feature::sse | mpt::arch::current::feature::sse2)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtscp_consume = &rdtscp_consume_tsc_sse2;
			rdtscp_acquire = &rdtscp_acquire_tsc_sse2;
			rdtscp_release = &rdtscp_release_tsc_sse2;
			rdtscp_acq_rel = &rdtscp_acq_rel_tsc_sse2;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_TSC) && defined(MPT_ARCH_INTRINSICS_X86_CPUID)
		if (!cpu_info.has_features(mpt::arch::current::feature::rdtscp) && cpu_info.has_features(mpt::arch::current::feature::tsc | mpt::arch::current::feature::cpuid)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtscp_seq_cst = &rdtscp_seq_cst_tsc_cpuid;
		}
#endif

#if defined(MPT_ARCH_INTRINSICS_X86_RDTSCP)
		if (cpu_info.has_features(mpt::arch::current::feature::rdtscp)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtscp_relaxed = &rdtscp_relaxed_rdtscp;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_RDTSCP) && defined(MPT_ARCH_INTRINSICS_X86_SSE2)
		if (cpu_info.has_features(mpt::arch::current::feature::rdtscp | mpt::arch::current::feature::sse2)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtscp_consume = &rdtscp_consume_tsc_sse2;
			rdtscp_acquire = &rdtscp_acquire_tsc_sse2;
			rdtscp_release = &rdtscp_release_tsc_sse2;
			rdtscp_acq_rel = &rdtscp_acq_rel_tsc_sse2;
		}
#endif
#if defined(MPT_ARCH_INTRINSICS_X86_RDTSCP) && defined(MPT_ARCH_INTRINSICS_X86_CPUID)
		if (cpu_info.has_features(mpt::arch::current::feature::rdtscp | mpt::arch::current::feature::cpuid)) {
			mpt::arch::feature_fence_guard arch_feature_guard;
			rdtscp_seq_cst = &rdtscp_seq_cst_rdtscp_cpuid;
		}
#endif

		if (!rdtsc_consume) {
			rdtsc_consume = rdtsc_relaxed;
		}
		if (!rdtsc_acquire) {
			rdtsc_acquire = rdtsc_consume;
		}
		if (!rdtsc_release) {
			rdtsc_release = rdtsc_acquire;
		}
		if (!rdtsc_acq_rel) {
			rdtsc_acq_rel = rdtsc_release;
		}
		if (!rdtsc_seq_cst) {
			rdtsc_seq_cst = rdtsc_acq_rel;
		}

		if (!rdtscp_consume) {
			rdtscp_consume = rdtscp_relaxed;
		}
		if (!rdtsc_acquire) {
			rdtscp_acquire = rdtscp_consume;
		}
		if (!rdtscp_release) {
			rdtscp_release = rdtscp_acquire;
		}
		if (!rdtscp_acq_rel) {
			rdtscp_acq_rel = rdtscp_release;
		}
		if (!rdtscp_seq_cst) {
			rdtscp_seq_cst = rdtscp_acq_rel;
		}

	}

	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw([[maybe_unused]] std::memory_order order = std::memory_order_relaxed, uint32 * core_id = nullptr) noexcept {
		std::atomic_signal_fence(order);
		if (core_id) {
			if (order == std::memory_order_relaxed) {
				return rdtscp_relaxed(core_id);
			}
			if (order == std::memory_order_consume) {
				return rdtscp_consume(core_id);
			}
			if (order == std::memory_order_acquire) {
				return rdtscp_acquire(core_id);
			}
			if (order == std::memory_order_release) {
				return rdtscp_release(core_id);
			}
			if (order == std::memory_order_acq_rel) {
				return rdtscp_acq_rel(core_id);
			}
			if (order == std::memory_order_seq_cst) {
				return rdtscp_seq_cst(core_id);
			}
		} else {
			if (order == std::memory_order_relaxed) {
				return rdtsc_relaxed();
			}
			if (order == std::memory_order_consume) {
				return rdtsc_consume();
			}
			if (order == std::memory_order_acquire) {
				return rdtsc_acquire();
			}
			if (order == std::memory_order_release) {
				return rdtsc_release();
			}
			if (order == std::memory_order_acq_rel) {
				return rdtsc_acq_rel();
			}
			if (order == std::memory_order_seq_cst) {
				return rdtsc_seq_cst();
			}
		}
		if (core_id) {
			*core_id = 0xffff'ffffu;
			return 0xffff'ffff'ffff'ffffull;
		} else {
			return 0xffff'ffff'ffff'ffffull;
		}
	}

	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_enter(std::memory_order order = std::memory_order_relaxed, uint32 * core_id = nullptr) noexcept {
		if (order == std::memory_order_acq_rel) {
			order = std::memory_order_acquire;
		}
		return now_raw(order, core_id);
	}

	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static rep now_raw_leave(std::memory_order order = std::memory_order_relaxed, uint32 * core_id = nullptr) noexcept {
		if (order == std::memory_order_acq_rel) {
			order = std::memory_order_release;
		}
		return now_raw(order, core_id);
	}

#if !defined(MPT_LIBCXX_QUIRK_NO_CHRONO)
	//[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static time_point now() noexcept;
#endif

};



namespace detail {

struct tsc_runtime_initializer {
	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE inline tsc_runtime_initializer() noexcept {
		tsc_runtime::init();
	}
};

#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif // MPT_COMPILER_CLANG
inline tsc_runtime_initializer g_tsc_runtime_initializer;
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG

} // namespace detail



#else



#define MPT_PROFILER_CLOCK_TSC 0

#define MPT_PROFILER_CLOCK_TSC_RUNTIME 0



#endif



} // namespace clock

} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_CLOCK_TSC_HPP
