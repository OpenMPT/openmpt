/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_CLASS_HPP
#define MPT_OSINFO_CLASS_HPP



#include "mpt/base/namespace.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

enum class osclass
{
	Unknown,
	Windows,
	Linux,
	Darwin,
	BSD,
	Haiku,
	DOS,
};

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_CLASS_HPP
