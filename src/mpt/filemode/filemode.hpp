/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_FILEMODE_HPP
#define MPT_FILEMODE_FILEMODE_HPP



#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



enum class stdio {
	input,
	output,
	error,
	log,
};

enum class mode {
	binary,
	text,
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_FILEMODE_HPP
