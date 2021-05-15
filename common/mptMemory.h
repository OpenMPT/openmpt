/*
 * mptMemory.h
 * -----------
 * Purpose: Raw memory manipulation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "mptBuildSettings.h"


#include "mpt/base/bit.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/span.hpp"

#include "mptAssert.h"
#include "mptBaseTypes.h"

#include <utility>
#include <type_traits>

#include <cstring>

#include <string.h>



OPENMPT_NAMESPACE_BEGIN



#define MPT_BINARY_STRUCT(type, size) \
	constexpr bool declare_binary_safe(const type &) { return true; } \
	static_assert(mpt::check_binary_size<type>(size)); \
/**/



template <typename T>
struct value_initializer
{
	inline void operator () (T & x)
	{
		x = T();
	}
};

template <typename T, std::size_t N>
struct value_initializer<T[N]>
{
	inline void operator () (T (& a)[N])
	{
		for(auto & e : a)
		{
			value_initializer<T>()(e);
		}
	}
};

template <typename T>
inline void Clear(T & x)
{
	static_assert(!std::is_pointer<T>::value);
	value_initializer<T>()(x);
}


// Memset given object to zero.
template <class T>
inline void MemsetZero(T &a)
{
	static_assert(std::is_pointer<T>::value == false, "Won't memset pointers.");
	static_assert(std::is_standard_layout<T>::value);
	static_assert((std::is_trivially_default_constructible<T>::value && std::is_trivially_copyable<T>::value) || mpt::is_binary_safe<T>::value);
	std::memset(&a, 0, sizeof(T));
}



OPENMPT_NAMESPACE_END
