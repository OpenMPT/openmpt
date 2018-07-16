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

#include <memory>
#include <utility>
#include <vector>



OPENMPT_NAMESPACE_BEGIN



namespace mpt {



template <typename T> inline span<T> as_span(std::vector<T> & cont) { return span<T>(cont); }

template <typename T> inline span<const T> as_span(const std::vector<T> & cont) { return span<const T>(cont); }



template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * beg, T * end) { return std::vector<typename std::remove_const<T>::type>(beg, end); }

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(T * data, std::size_t size) { return std::vector<typename std::remove_const<T>::type>(data, data + size); }

template <typename T> inline std::vector<typename std::remove_const<T>::type> make_vector(mpt::span<T> data) { return std::vector<typename std::remove_const<T>::type>(data.data(), data.data() + data.size()); }

template <typename T, std::size_t N> inline std::vector<typename std::remove_const<T>::type> make_vector(T (&arr)[N]) { return std::vector<typename std::remove_const<T>::type>(std::begin(arr), std::end(arr)); }



template <typename T>
struct GetRawBytesFunctor<std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const mpt::byte *>(v.data()), v.size() * sizeof(T));
	}
	inline mpt::byte_span operator () (std::vector<T> & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<mpt::byte *>(v.data()), v.size() * sizeof(T));
	}
};

template <typename T>
struct GetRawBytesFunctor<const std::vector<T>>
{
	inline mpt::const_byte_span operator () (const std::vector<T> & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename std::remove_const<T>::type>::value);
		return mpt::as_span(reinterpret_cast<const mpt::byte *>(v.data()), v.size() * sizeof(T));
	}
};



} // namespace mpt



#if MPT_CXX_AT_LEAST(14)
namespace mpt {
using std::make_unique;
} // namespace mpt
#else
namespace mpt {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
} // namespace mpt
#endif



OPENMPT_NAMESPACE_END
