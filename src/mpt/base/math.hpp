/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_EMPTY_HPP
#define MPT_BASE_EMPTY_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"

#include <cmath>



namespace mpt {
inline namespace MPT_INLINE_NS {


#if MPT_OS_DJGPP

inline long double log2(const long double val) {
	return static_cast<long double>(::log2(static_cast<double>(val)));
}

inline double log2(const double val) {
	return ::log2(val);
}

inline float log2(const float val) {
	return ::log2f(val);
}

#else // !MPT_OS_DJGPP

// C++11 std::log2
using std::log2;

#endif // MPT_OS_DJGPP


#if MPT_OS_DJGPP

inline long double round(const long double val) {
	return ::roundl(val);
}

inline double round(const double val) {
	return ::round(val);
}

inline float round(const float val) {
	return ::roundf(val);
}

#else // !MPT_OS_DJGPP

// C++11 std::round
using std::round;

#endif // MPT_OS_DJGPP


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_EMPTY_HPP
