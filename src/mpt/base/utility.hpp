/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_UTILITY_HPP
#define MPT_BASE_UTILITY_HPP



#include "mpt/base/detect_compiler.hpp"
#include "mpt/base/namespace.hpp"

#if MPT_CXX_BEFORE(20)
#include "mpt/base/saturate_cast.hpp"
#endif

#include <type_traits>
#include <utility>



namespace mpt {
inline namespace MPT_INLINE_NS {


#if MPT_CXX_AT_LEAST(20)

using std::in_range;

#else

// Returns true iff Tdst can represent the value val.
// Use as if(mpt::in_range<uint8>(-1)).
template <typename Tdst, typename Tsrc>
constexpr bool in_range(Tsrc val) {
	return (static_cast<Tsrc>(mpt::saturate_cast<Tdst>(val)) == val);
}

#endif


#if MPT_CXX_AT_LEAST(23)

using std::to_underlying;

#else

template <typename T>
constexpr std::underlying_type_t<T> to_underlying(T value) noexcept {
	return static_cast<typename std::underlying_type<T>::type>(value);
}

#endif


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_UTILITY_HPP
