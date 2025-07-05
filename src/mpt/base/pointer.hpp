/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_POINTER_HPP
#define MPT_BASE_POINTER_HPP



#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"

#include <cstddef>
#include <cstdint>



namespace mpt {
inline namespace MPT_INLINE_NS {


inline constexpr int arch_bits = sizeof(void *) * 8;
inline constexpr std::size_t pointer_size = sizeof(void *);


template <typename Tdst, typename Tsrc>
struct pointer_cast_helper;

template <typename Tdst, typename Tptr>
struct pointer_cast_helper<Tdst, Tptr *> {
	static constexpr Tdst cast(Tptr * const & src) noexcept {
		static_assert(sizeof(Tdst) == sizeof(Tptr *));
		return static_cast<Tdst>(reinterpret_cast<std::uintptr_t>(src));
	}
};

template <typename Tptr, typename Tsrc>
struct pointer_cast_helper<Tptr *, Tsrc> {
	static constexpr Tptr * cast(const Tsrc & src) noexcept {
		static_assert(sizeof(Tptr *) == sizeof(Tsrc));
		return reinterpret_cast<Tptr *>(static_cast<std::uintptr_t>(src));
	}
};

template <typename Tdst, typename Tsrc>
constexpr Tdst pointer_cast(const Tsrc & src) noexcept {
	return pointer_cast_helper<Tdst, Tsrc>::cast(src);
}


template <typename T>
class void_ptr {
private:
	T * m_ptr = nullptr;
public:
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE explicit void_ptr(void * ptr)
		: m_ptr(reinterpret_cast<T *>(ptr)) {
		return;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE T & operator*() {
		return *m_ptr;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE T * operator->() {
		return m_ptr;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE operator void *() {
		return m_ptr;
	}
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE operator T *() {
		return m_ptr;
	}
};


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_POINTER_HPP
