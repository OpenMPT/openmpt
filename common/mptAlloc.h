/*
 * mptAlloc.h
 * ----------
 * Purpose: Dynamic memory allocation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptBaseMacros.h"
#include "mptMemory.h"
#include "mptSpan.h"

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



namespace mpt {



template <typename T> inline mpt::span<T> as_span(std::vector<T> & cont)
{
	return mpt::span<T>(cont.data(), cont.data() + cont.size());
}

template <typename T> inline mpt::span<const T> as_span(const std::vector<T> & cont)
{
	return mpt::span<const T>(cont.data(), cont.data() + cont.size());
}



template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * beg, T * end)
{
	return std::vector<typename std::remove_const<T>::type>(beg, end);
}

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * data, std::size_t size)
{
	return std::vector<typename std::remove_const<T>::type>(data, data + size);
}

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(mpt::span<T> data)
{
	return std::vector<typename std::remove_const<T>::type>(data.data(), data.data() + data.size());
}

template <typename T, std::size_t N> inline std::vector<typename std::remove_const<T>::type> make_vector(T (&arr)[N])
{
	return std::vector<typename std::remove_const<T>::type>(std::begin(arr), std::end(arr));
}



template <typename Tcont2, typename Tcont1> inline Tcont1 & append(Tcont1 & cont1, const Tcont2 & cont2)
{
	cont1.insert(cont1.end(), cont2.begin(), cont2.end());
	return cont1;
}

template <typename Tit2, typename Tcont1> inline Tcont1 & append(Tcont1 & cont1, Tit2 beg, Tit2 end)
{
	cont1.insert(cont1.end(), beg, end);
	return cont1;
}



template <typename Tdst, typename Tsrc>
struct buffer_cast_impl
{
	inline Tdst operator () (const Tsrc &src) const
	{
		return Tdst(mpt::byte_cast<const typename Tdst::value_type *>(src.data()), mpt::byte_cast<const typename Tdst::value_type *>(src.data()) + src.size());
	}
};

// casts between vector<->string of byte-castable types
template <typename Tdst, typename Tsrc>
inline Tdst buffer_cast(Tsrc src)
{
	return buffer_cast_impl<Tdst, Tsrc>()(src);
}



template <typename T>
struct GetRawBytesFunctor<std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		static_assert(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const std::byte *>(v.data()), v.size() * sizeof(T));
	}
	inline mpt::byte_span operator () (std::vector<T> & v) const
	{
		static_assert(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<std::byte *>(v.data()), v.size() * sizeof(T));
	}
};

template <typename T>
struct GetRawBytesFunctor<const std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		static_assert(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const std::byte *>(v.data()), v.size() * sizeof(T));
	}
};



} // namespace mpt



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
