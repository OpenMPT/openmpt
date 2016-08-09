/*
 * mptTypeTraits.h
 * ---------------
 * Purpose: C++11 similar type_traits header plus some OpenMPT specific traits.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#if MPT_COMPILER_HAS_TYPE_TRAITS
#include <type_traits>
#endif // MPT_COMPILER_HAS_TYPE_TRAITS



OPENMPT_NAMESPACE_BEGIN



namespace mpt {



#if MPT_COMPILER_HAS_TYPE_TRAITS

typedef std::true_type true_type;
typedef std::false_type false_type;

#else // !MPT_COMPILER_HAS_TYPE_TRAITS

struct true_type {
	typedef true_type type;
	typedef bool value_type;
	static const value_type value = true;
	operator value_type () const { return value; }
	value_type operator () () const { return value; }
};

struct false_type {
	typedef true_type type;
	typedef bool value_type;
	static const value_type value = false;
	operator value_type () const { return value; }
	value_type operator () () const { return value; }
};

#endif // MPT_COMPILER_HAS_TYPE_TRAITS


template <std::size_t size> struct int_of_size { };
template <> struct int_of_size<1> { typedef int8  type; };
template <> struct int_of_size<2> { typedef int16 type; };
template <> struct int_of_size<3> { typedef int32 type; };
template <> struct int_of_size<4> { typedef int32 type; };
template <> struct int_of_size<5> { typedef int64 type; };
template <> struct int_of_size<6> { typedef int64 type; };
template <> struct int_of_size<7> { typedef int64 type; };
template <> struct int_of_size<8> { typedef int64 type; };

template <std::size_t size> struct uint_of_size { };
template <> struct uint_of_size<1> { typedef uint8  type; };
template <> struct uint_of_size<2> { typedef uint16 type; };
template <> struct uint_of_size<3> { typedef uint32 type; };
template <> struct uint_of_size<4> { typedef uint32 type; };
template <> struct uint_of_size<5> { typedef uint64 type; };
template <> struct uint_of_size<6> { typedef uint64 type; };
template <> struct uint_of_size<7> { typedef uint64 type; };
template <> struct uint_of_size<8> { typedef uint64 type; };


#if MPT_COMPILER_HAS_TYPE_TRAITS

using std::make_signed;
using std::make_unsigned;

#else // !MPT_COMPILER_HAS_TYPE_TRAITS

// Simplified version of C++11 std::make_signed and std::make_unsigned:
//  - we do not require a C++11 <type_traits> header
//  - no support fr CV-qualifiers
//  - does not fail for non-integral types

template <typename T> struct make_signed { typedef typename mpt::int_of_size<sizeof(T)>::type type; };
template <> struct make_signed<char> { typedef signed char type; };
template <> struct make_signed<signed char> { typedef signed char type; };
template <> struct make_signed<unsigned char> { typedef signed char type; };
template <> struct make_signed<signed short> { typedef signed short type; };
template <> struct make_signed<unsigned short> { typedef signed short type; };
template <> struct make_signed<signed int> { typedef signed int type; };
template <> struct make_signed<unsigned int> { typedef signed int type; };
template <> struct make_signed<signed long> { typedef signed long type; };
template <> struct make_signed<unsigned long> { typedef signed long type; };
template <> struct make_signed<signed long long> { typedef signed long long type; };
template <> struct make_signed<unsigned long long> { typedef signed long long type; };

template <typename T> struct make_unsigned { typedef typename mpt::uint_of_size<sizeof(T)>::type type; };
template <> struct make_unsigned<char> { typedef unsigned char type; };
template <> struct make_unsigned<signed char> { typedef unsigned char type; };
template <> struct make_unsigned<unsigned char> { typedef unsigned char type; };
template <> struct make_unsigned<signed short> { typedef unsigned short type; };
template <> struct make_unsigned<unsigned short> { typedef unsigned short type; };
template <> struct make_unsigned<signed int> { typedef unsigned int type; };
template <> struct make_unsigned<unsigned int> { typedef unsigned int type; };
template <> struct make_unsigned<signed long> { typedef unsigned long type; };
template <> struct make_unsigned<unsigned long> { typedef unsigned long type; };
template <> struct make_unsigned<signed long long> { typedef unsigned long long type; };
template <> struct make_unsigned<unsigned long long> { typedef unsigned long long type; };

#endif // MPT_COMPILER_HAS_TYPE_TRAITS


#if MPT_COMPILER_HAS_TYPE_TRAITS

using std::remove_const;

#else // !MPT_COMPILER_HAS_TYPE_TRAITS

template <typename T> struct remove_const { typedef T type; };
template <typename T> struct remove_const<const T> { typedef T type; };

#endif // MPT_COMPILER_HAS_TYPE_TRAITS


// Tell which types are safe for mpt::byte_cast.
// signed char is actually not allowed to alias into an object representation,
// which means that, if the actual type is not itself signed char but char or
// unsigned char instead, dereferencing the signed char pointer is undefined
// behaviour.
template <typename T> struct is_byte_castable : public mpt::false_type { };
template <> struct is_byte_castable<char>                : public mpt::true_type { };
template <> struct is_byte_castable<unsigned char>       : public mpt::true_type { };
template <> struct is_byte_castable<const char>          : public mpt::true_type { };
template <> struct is_byte_castable<const unsigned char> : public mpt::true_type { };


// Tell which types are safe to binary write into files.
// By default, no types are safe.
// When a safe type gets defined,
// also specialize this template so that IO functions will work.
template <typename T> struct is_binary_safe : public mpt::false_type { }; 

// Specialization for byte types.
template <> struct is_binary_safe<char>  : public mpt::true_type { };
template <> struct is_binary_safe<uint8> : public mpt::true_type { };
template <> struct is_binary_safe<int8>  : public mpt::true_type { };

// Generic Specialization for arrays.
template <typename T, std::size_t N> struct is_binary_safe<T[N]> : public is_binary_safe<T> { };

template <typename T>
struct GetRawBytesFunctor
{
	inline const mpt::byte * operator () (const T & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename mpt::remove_const<T>::type>::value);
		return reinterpret_cast<const mpt::byte *>(&v);
	}
	inline mpt::byte * operator () (T & v) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename mpt::remove_const<T>::type>::value);
		return reinterpret_cast<mpt::byte *>(&v);
	}
};

template <typename T, std::size_t N>
struct GetRawBytesFunctor<T[N]>
{
	inline const mpt::byte * operator () (const T (&v)[N]) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename mpt::remove_const<T>::type>::value);
		return reinterpret_cast<const mpt::byte *>(v);
	}
	inline mpt::byte * operator () (T (&v)[N]) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename mpt::remove_const<T>::type>::value);
		return reinterpret_cast<mpt::byte *>(v);
	}
};

template <typename T, std::size_t N>
struct GetRawBytesFunctor<const T[N]>
{
	inline const mpt::byte * operator () (const T (&v)[N]) const
	{
		STATIC_ASSERT(mpt::is_binary_safe<typename mpt::remove_const<T>::type>::value);
		return reinterpret_cast<const mpt::byte *>(v);
	}
};

// In order to be able to partially specialize it,
// as_raw_memory is implemented via a class template.
// Do not overload or specialize as_raw_memory directly.
// Using a wrapper (by default just around a cast to const mpt::byte *),
// allows for implementing raw memory access
// via on-demand generating a cached serialized representation.
template <typename T> inline const mpt::byte * as_raw_memory(const T & v)
{
	STATIC_ASSERT(mpt::is_binary_safe<typename mpt::remove_const<T>::type>::value);
	return mpt::GetRawBytesFunctor<T>()(v);
}
template <typename T> inline mpt::byte * as_raw_memory(T & v)
{
	STATIC_ASSERT(mpt::is_binary_safe<typename mpt::remove_const<T>::type>::value);
	return mpt::GetRawBytesFunctor<T>()(v);
}

} // namespace mpt

#define MPT_BINARY_STRUCT(type, size) \
	STATIC_ASSERT(sizeof( type ) == (size) ); \
	STATIC_ASSERT(MPT_ALIGNOF( type ) == 1); \
	namespace mpt { \
		template <> struct is_binary_safe< type > : public mpt::true_type { }; \
	} \
/**/


namespace mpt
{



} // namespace mpt



OPENMPT_NAMESPACE_END
