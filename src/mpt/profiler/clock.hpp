/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_CLOCK_HPP
#define MPT_PROFILER_CLOCK_HPP



#include "mpt/base/namespace.hpp"
#include "mpt/profiler/clock_get_tick_count.hpp"
#include "mpt/profiler/clock_query_performance_counter.hpp"
#include "mpt/profiler/clock_std_chrono.hpp"
#include "mpt/profiler/clock_system.hpp"
#include "mpt/profiler/clock_tsc.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace profiler {



//                                  performance precision  resolution monotonic walltime coreid
//
// rdtsc                                 +++    +          +++ 1clk   -         -        -      TSC          relaxed
// sfence;rdtsc                          +++    +          +++ 1clk   -         -        -      TSC+SSE      consume
// lfence;rdtsc                          ++     ++         +++ 1clk   -         -        -      TSC+SSE2     aquire
// lfence;rdtsc + mfence;lfence;rdtsc    ++     ++         +++ 1clk   -         -        -      TSC+SSE2     aquire+release
// cpuid;rdtsc                           -      -          +++ 1clk   -         -        -      CPUID+TSC    seq_cst
//
// rdtscp                                ++     ++         +++ 1clk   ~         -        +      RDTSCP       consume
// lfence;rdtscp                         ++     ++         +++ 1clk   ~         -        +      SSE2+RDTSCP  aquire
// lfence;rdtscp + mfence;lfence;rdtscp  ++     +++        +++ 1clk   ~         -        +      SSE2+RDTSCP  aquire+release
// cpuid;rdtscp                          -      -          +++ 1clk   ~         -        +      CPUID+RDTSCP seq_cst
//
// QueryPerformanceCounter               -      +   1us    ++  1us    ~         -
//
// clock_gettime(CLOCK_MONOTONIC)        ++     +++ ?      +   1ns    +         -
// clock_gettime(CLOCK_MONOTONIC_COARSE) +++    ++  ?      +   1ns    +         -
// timtGetTime                           +      +   1ms    -   1ms    +         -
// GetTickCount64                        ++     -   15ms   -   1ms    +         -
//
// clock_gettime(CLOCK_REALTIME)         ++     +++ ?      +   1ns    -         +
// clock_gettime(CLOCK_REALTIME_COARSE)  +++    ++  ?      +   1ns    -         +
// gettimeofday                          +      ++  ?      ++  1us    -         +
// GetSystemTimeAsFileTime               +      +   1us    +++ 100ns  -         +
// GetSystemTimePreciseAsFileTime        -      +++ 100ns  +++ 100ns  -         +
// std::time                             +      -   1s     --  1s     -         +
//
// std::chrono::system_clock             ?      -          ?          -         +
// std::chrono::steady_clock             ?      -          ?          +         -
// std::chrono::high_resolution_clock    ?      +          ?          -         -
//
// MSVC
// std::chrono::system_clock          GetSystemTimePreciseAsFileTime
// std::chrono::steady_clock          QueryPerformanceCounter
// std::chrono::high_resolution_clock steady_clock
//
// Clang-Windows
// std::chrono::system_clock          GetSystemTimePreciseAsFileTime / GetSystemTimeAsFileTime
// std::chrono::steady_clock          QueryPerformanceCounter
// std::chrono::high_resolution_clock steady_clock / system_clock
//
// Clang-Linux
// std::chrono::system_clock          clock_gettime(CLOCK_REALTIME) / gettimeofday
// std::chrono::steady_clock          clock_gettime(CLOCK_MONOTONIC)
// std::chrono::high_resolution_clock steady_clock / system_clock
//
// GCC
// std::chrono::system_clock          clock_gettime(CLOCK_REALTIME) / gettimeofday / std::time
// std::chrono::steady_clock          clock_gettime(CLOCK_MONOTONIC) / system_clock
// std::chrono::high_resolution_clock system_clock



#if MPT_PROFILER_CLOCK_TSC && MPT_PROFILER_CLOCK_TSC_RUNTIME

using highres_clock = profiler::clock::tsc_runtime;
using exact_clock = profiler::clock::tsc_runtime;
using fast_clock = profiler::clock::tsc;
using default_clock = profiler::clock::tsc;

#elif MPT_PROFILER_CLOCK_TSC

using highres_clock = profiler::clock::tsc;
using exact_clock = profiler::clock::tsc;
using fast_clock = profiler::clock::tsc;
using default_clock = profiler::clock::tsc;

#elif MPT_PROFILER_CLOCK_TSC_RUNTIME

using highres_clock = profiler::clock::tsc_runtime;
using exact_clock = profiler::clock::tsc_runtime;
using fast_clock = profiler::clock::tsc_runtime;
using default_clock = profiler::clock::tsc_runtime;

#elif MPT_PROFILER_CLOCK_GET_TICK_COUNT && MPT_PROFILER_CLOCK_QUERY_PERFORMANCE_COUNTER

using highres_clock = profiler::clock::query_performance_counter;
using exact_clock = profiler::clock::get_tick_count;
using fast_clock = profiler::clock::get_tick_count;
using default_clock = profiler::clock::query_performance_counter;

#elif MPT_PROFILER_CLOCK_STD_CHRONO

using highres_clock = profiler::clock::chrono_high_resolution;
using exact_clock = profiler::clock::chrono_steady;
using fast_clock = profiler::clock::chrono_steady;
using default_clock = profiler::clock::chrono_high_resolution;

#elif MPT_PROFILER_CLOCK_SYSTEM

using highres_clock = profiler::clock::system;
using exact_clock = profiler::clock::system;
using fast_clock = profiler::clock::system;
using default_clock = profiler::clock::system;

#endif



} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_CLOCK_HPP
