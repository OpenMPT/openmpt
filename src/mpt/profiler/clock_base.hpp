/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_PROFILER_CLOCK_BASE_HPP
#define MPT_PROFILER_CLOCK_BASE_HPP



#include "mpt/base/float.hpp"
#include "mpt/base/namespace.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace profiler {

namespace clock {



using namespace mpt::float_literals;



enum class frequency_mode {
	fast,
	slow,
	optional,
};



} // namespace clock

} // namespace profiler



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_PROFILER_CLOCK_BASE_HPP
