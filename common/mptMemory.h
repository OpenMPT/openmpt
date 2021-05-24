/*
 * mptMemory.h
 * -----------
 * Purpose: Raw memory manipulation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


#include "mpt/base/bit.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/span.hpp"
#include "mpt/base/utility.hpp"

#include "mptAssert.h"
#include "mptBaseTypes.h"

#include <utility>
#include <type_traits>



OPENMPT_NAMESPACE_BEGIN



#define MPT_BINARY_STRUCT(type, size) \
	constexpr bool declare_binary_safe(const type &) { return true; } \
	static_assert(mpt::check_binary_size<type>(size)); \
/**/


template <typename T>
inline void Clear(T & x)
{
	static_assert(!std::is_pointer<T>::value);
	mpt::reset(x);
}


// Memset given object to zero.
template <class T>
inline void MemsetZero(T &a)
{
	static_assert(std::is_pointer<T>::value == false, "Won't memset pointers.");
	mpt::memclear(a);
}



OPENMPT_NAMESPACE_END
