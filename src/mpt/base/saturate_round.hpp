/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_SATURATE_ROUND_HPP
#define MPT_BASE_SATURATE_ROUND_HPP



#include "mpt/base/namespace.hpp"

#include "mpt/base/math.hpp"

#include <limits>



namespace mpt {
inline namespace MPT_INLINE_NS {



template <typename Tdst>
constexpr Tdst saturate_trunc(double src) {
	if (src >= static_cast<double>(std::numeric_limits<Tdst>::max())) {
		return std::numeric_limits<Tdst>::max();
	}
	if (src <= static_cast<double>(std::numeric_limits<Tdst>::min())) {
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}

template <typename Tdst>
constexpr Tdst saturate_trunc(float src) {
	if (src >= static_cast<float>(std::numeric_limits<Tdst>::max())) {
		return std::numeric_limits<Tdst>::max();
	}
	if (src <= static_cast<float>(std::numeric_limits<Tdst>::min())) {
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}


// Rounds given double value to nearest integer value of type T.
// Out-of-range values are saturated to the specified integer type's limits.

template <typename T>
inline T saturate_round(float val) {
	static_assert(std::numeric_limits<T>::is_integer);
	return mpt::saturate_trunc<T>(mpt::round(val));
}

template <typename T>
inline T saturate_round(double val) {
	static_assert(std::numeric_limits<T>::is_integer);
	return mpt::saturate_trunc<T>(mpt::round(val));
}

template <typename T>
inline T saturate_round(long double val) {
	static_assert(std::numeric_limits<T>::is_integer);
	return mpt::saturate_trunc<T>(mpt::round(val));
}


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_SATURATE_ROUND_HPP
