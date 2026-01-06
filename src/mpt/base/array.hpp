/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_ARRAY_HPP
#define MPT_BASE_ARRAY_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"

#include <array>
#include <type_traits>
#if MPT_CXX_BEFORE(20)
#include <utility>
#endif // <C++20

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {


template <typename T>
struct stdarray_extent : std::integral_constant<std::size_t, 0> { };

template <typename T, std::size_t N>
struct stdarray_extent<std::array<T, N>> : std::integral_constant<std::size_t, N> { };

template <typename T>
struct is_stdarray : std::false_type { };

template <typename T, std::size_t N>
struct is_stdarray<std::array<T, N>> : std::true_type { };

// mpt::extent is the same as std::extent,
// but also works for std::array,
// and asserts that the given type is actually an array type instead of returning 0.
// use as:
// mpt::extent<decltype(expr)>()
// mpt::extent<decltype(variable)>()
// mpt::extent<decltype(type)>()
// mpt::extent<type>()
template <typename T>
constexpr std::size_t extent() noexcept {
	using Tarray = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
	static_assert(std::is_array<Tarray>::value || mpt::is_stdarray<Tarray>::value);
	if constexpr (mpt::is_stdarray<Tarray>::value) {
		return mpt::stdarray_extent<Tarray>();
	} else {
		return std::extent<Tarray>();
	}
}

template <typename>
struct array_size;

template <typename T, std::size_t N>
struct array_size<std::array<T, N>> {
	static constexpr std::size_t size = N;
};

template <typename T, std::size_t N>
struct array_size<T[N]> {
	static constexpr std::size_t size = N;
};


template <typename T, std::size_t N, typename Tx>
constexpr std::array<T, N> init_array(const Tx & x) {
	std::array<T, N> result{};
	for (std::size_t i = 0; i < N; ++i) {
		result[i] = x;
	}
	return result;
}


template <typename T, std::size_t N, typename Fgen>
constexpr std::array<T, N> generate_array(Fgen generator) {
	std::array<T, N> result{};
	for (std::size_t i = 0; i < N; ++i) {
		result[i] = generator(i);
	}
	return result;
}


#if MPT_CXX_AT_LEAST(20)

using std::to_array;

#else // !C++20

namespace detail {
template <typename T, std::size_t N, std::size_t... I>
constexpr std::array<typename std::remove_cv<T>::type, N> to_array_impl(T (&a)[N], std::index_sequence<I...>) {
	return {{a[I]...}};
}
} // namespace detail

template <typename T, std::size_t N>
constexpr std::array<typename std::remove_cv<T>::type, N> to_array(T (&a)[N]) {
	return detail::to_array_impl(a, std::make_index_sequence<N>{});
}

namespace detail {
template <typename T, std::size_t N, std::size_t... I>
constexpr std::array<typename std::remove_cv<T>::type, N> to_array_impl(T (&&a)[N], std::index_sequence<I...>) {
	return {{std::move(a[I])...}};
}
} // namespace detail

template <typename T, std::size_t N>
constexpr std::array<typename std::remove_cv<T>::type, N> to_array(T (&&a)[N]) {
	return detail::to_array_impl(std::move(a), std::make_index_sequence<N>{});
}

#endif // C++20


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_ARRAY_HPP
