/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_ALIGNED_ARRAY_HPP
#define MPT_BASE_ALIGNED_ARRAY_HPP



#include "mpt/base/namespace.hpp"

#include <array>
#include <new>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {


template <typename T, std::size_t count, std::align_val_t alignment>
struct alignas(static_cast<std::size_t>(alignment)) aligned_array
	: std::array<T, count> {
	static_assert(static_cast<std::size_t>(alignment) >= alignof(T));
	static_assert(((count * sizeof(T)) % static_cast<std::size_t>(alignment)) == 0);
	static_assert(sizeof(std::array<T, count>) == (sizeof(T) * count));
};

static_assert(sizeof(mpt::aligned_array<float, 4, std::align_val_t{sizeof(float) * 4}>) == sizeof(std::array<float, 4>));



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_ALIGNED_ARRAY_HPP
