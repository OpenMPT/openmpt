/*
 * mptAlloc.h
 * ----------
 * Purpose: Dynamic memory allocation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "mptBuildSettings.h"



#include "mpt/base/alloc.hpp"
#include "mpt/base/span.hpp"

#include "mptBaseMacros.h"
#include "mptMemory.h"

#if MPT_CXX_AT_LEAST(20)
#include <version>
#else // !C++20
#include <array>
#endif // C++20

#include <array>
#include <memory>
#include <new>
#include <vector>



OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_ALIGNED_ALLOC)



namespace mpt
{



template <typename T, std::size_t count, std::align_val_t alignment>
struct alignas(static_cast<std::size_t>(alignment)) aligned_array
	: std::array<T, count>
{
	static_assert(static_cast<std::size_t>(alignment) >= alignof(T));
	static_assert(((count * sizeof(T)) % static_cast<std::size_t>(alignment)) == 0);
	static_assert(sizeof(std::array<T, count>) == (sizeof(T) * count));
};

static_assert(sizeof(mpt::aligned_array<float, 4, std::align_val_t{sizeof(float) * 4}>) == sizeof(std::array<float, 4>));



} // namespace mpt



#endif // MPT_ENABLE_ALIGNED_ALLOC



OPENMPT_NAMESPACE_END
