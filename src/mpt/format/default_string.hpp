/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FORMAT_DEFAULT_STRING_HPP
#define MPT_FORMAT_DEFAULT_STRING_HPP



#include "mpt/base/namespace.hpp"
#include "mpt/string_convert/convert.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {


template <typename Tstring, typename T>
inline auto format_value_default(const T & x) -> decltype(mpt::convert<Tstring>(x)) {
	return mpt::convert<Tstring>(x);
}


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FORMAT_DEFAULT_STRING_HPP
