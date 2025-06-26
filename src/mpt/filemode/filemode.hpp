/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_FILEMODE_HPP
#define MPT_FILEMODE_FILEMODE_HPP



#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



enum class stdio : uint8 {
	input = 0,
	output = 1,
	error = 2,
	log = 3,
};

enum class mode : uint8 {
	binary = 0,
	text = 1,
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_FILEMODE_HPP
