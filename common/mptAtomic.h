/*
 * mptAtomic.h
 * -----------
 * Purpose: Subset of C++11 std::atomic implementation as mpt::atomic_* and MPT_ATOMIC_PTR<T*>
 * Notes  : Only 32 bit signed and unsigned integer and pointers are supported.
 *          The interface (as far as implemented) is syntax-compatbile with C++11 
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#if defined(MPT_ENABLE_ATOMIC)

#if !(MPT_MSVC_BEFORE(2012,0) || MPT_GCC_BEFORE(4,4,0) || MPT_CLANG_BEFORE(3,1,0))
#include <atomic>
#endif // MPT_COMPILER

#endif // MPT_ENABLE_ATOMIC


OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_ATOMIC)


namespace mpt
{


#if MPT_GCC_BEFORE(4,4,0) || MPT_CLANG_BEFORE(3,1,0)

template < typename T >
class atomic_impl {

private:
	mutable T Data;

private: // disabled
	atomic_impl( const atomic_impl<T> & src );
	atomic_impl<T> & operator = ( const atomic_impl<T> & src );

public:
	
	atomic_impl() {
		return;
	}
	atomic_impl( T init ) {
		T old = __sync_fetch_and_add( &Data, 0 );
		while ( old != __sync_val_compare_and_swap( &Data, old, init ) ) {
			// nothing
		}
	}
	T operator = ( T src ) {
		T old = __sync_fetch_and_add( &Data, 0 );
		while ( old != __sync_val_compare_and_swap( &Data, old, src ) ) {
			// nothing
		}
		return src;
	}

	operator T () const {
		return __sync_fetch_and_add( &Data, 0 );
	}

	bool is_lock_free() const {
		return true;
	}

	T load() const {
		return __sync_fetch_and_add( &Data, 0 );
	}
	void store( T val ) {
		T old = __sync_fetch_and_add( &Data, 0 );
		while ( old != __sync_val_compare_and_swap( &Data, old, val ) ) {
			// nothing
		}
	}
	T exchange( T val ) {
		T old = __sync_fetch_and_add( &Data, 0 );
		while ( old != __sync_val_compare_and_swap( &Data, old, val ) ) {
			// nothing
		}
		return old;
	}

	bool compare_exchange_strong( T & expected, T new_value ) {
		return __sync_bool_compare_and_swap( &Data, expected, new_value );
	}
	bool compare_exchange_weak( T & expected, T new_value ) {
		return __sync_bool_compare_and_swap( &Data, expected, new_value );
	}

	T fetch_add( T val ) {
		return __sync_fetch_and_add( &Data, val );
	}
	T fetch_sub( T val ) {
		return __sync_fetch_and_sub( &Data, val );
	}

	T fetch_and( T val ) {
		return __sync_fetch_and_and( &Data, val );
	}
	T fetch_or( T val ) {
		return __sync_fetch_and_or( &Data, val );
	}
	T fetch_xor( T val ) {
		return __sync_fetch_and_xor( &Data, val );
	}

}; // class atomic

#elif MPT_MSVC_BEFORE(2012,0)
	
template < typename T >
class atomic_impl_32 {

private:
	mutable long Data;

private: // disabled
	atomic_impl_32( const atomic_impl_32<T> & src );
	atomic_impl_32<T> & operator = ( const atomic_impl_32<T> & src );

public:
	
	atomic_impl_32() {
		return;
	}
	atomic_impl_32( T init ) {
		_InterlockedExchange( &Data, init );
	}
	T operator = ( T src ) {
		_InterlockedExchange( &Data, src );
		return src;
	}

	operator T () const {
		return _InterlockedExchangeAdd( &Data, 0 );
	}

	bool is_lock_free() const {
		return true;
	}

	T load() const {
		return _InterlockedExchangeAdd( &Data, 0 );
	}
	void store( T val ) {
		_InterlockedExchange( &Data, val );
	}
	T exchange( T val ) {
		return _InterlockedExchange( &Data, val );
	}

	bool compare_exchange_strong( T & expected, T new_value ) {
		return _InterlockedCompareExchange( &Data, new_value, expected ) == expected;
	}
	bool compare_exchange_weak( T & expected, T new_value ) {
		return _InterlockedCompareExchange( &Data, new_value, expected ) == expected;
	}

	T fetch_add( T val ) {
		return _InterlockedExchangeAdd( &Data, val );
	}
	T fetch_sub( T val ) {
		return _InterlockedExchangeAdd( &Data, -val );
	}

	T fetch_and( T val ) {
		return _InterlockedAnd( &Data, val );
	}
	T fetch_or( T val ) {
		return _InterlockedOr( &Data, val );
	}
	T fetch_xor( T val ) {
		return _InterlockedXor( &Data, val );
	}

}; // class atomic_impl_32

#if defined(_M_AMD64)

template < typename T >
class atomic_impl_ptr {

private:
	mutable __int64 Data;

private: // disabled
	atomic_impl_ptr( const atomic_impl_ptr<T> & src );
	atomic_impl_ptr<T> & operator = ( const atomic_impl_ptr<T> & src );

public:
	
	atomic_impl_ptr() {
		return;
	}
	atomic_impl_ptr( T init ) {
		_InterlockedExchange64( &Data, reinterpret_cast<std::uintptr_t>( init ) );
	}
	T operator = ( T src ) {
		_InterlockedExchange64( &Data, reinterpret_cast<std::uintptr_t>( src ) );
		return src;
	}

	operator T () const {
		return reinterpret_cast<T>( _InterlockedExchangeAdd64( &Data, reinterpret_cast<std::uintptr_t>( 0 ) ) );
	}

	bool is_lock_free() const {
		return true;
	}
	
	T load() const {
		return reinterpret_cast<T>( _InterlockedExchangeAdd64( &Data, reinterpret_cast<std::uintptr_t>( 0 ) ) );
	}
	void store( T val ) {
		_InterlockedExchange64( &Data, reinterpret_cast<std::uintptr_t>( val ) );
	}
	T exchange( T val ) {
		return reinterpret_cast<T>( _InterlockedExchange64( &Data, reinterpret_cast<std::uintptr_t>( val ) ) );
	}

	bool compare_exchange_strong( T & expected, T new_value ) {
		return reinterpret_cast<T>( _InterlockedCompareExchange64( &Data, new_value, reinterpret_cast<std::uintptr_t>( expected ) ) ) == expected;
	}
	bool compare_exchange_weak( T & expected, T new_value ) {
		return reinterpret_cast<T>( _InterlockedCompareExchange64( &Data, new_value, reinterpret_cast<std::uintptr_t>( expected ) ) ) == expected;
	}

}; // class atomic_impl_ptr

#elif defined(_M_X86)

template < typename T >
class atomic_impl_ptr {

private:
	mutable long Data;

private: // disabled
	atomic_impl_ptr( const atomic_impl_ptr<T> & src );
	atomic_impl_ptr<T> & operator = ( const atomic_impl_ptr<T> & src );

public:
	
	atomic_impl_ptr() {
		return;
	}
	atomic_impl_ptr( T init ) {
		_InterlockedExchange( &Data, reinterpret_cast<std::uintptr_t>( init ) );
	}
	T operator = ( T src ) {
		_InterlockedExchange( &Data, reinterpret_cast<std::uintptr_t>( src ) );
		return src;
	}

	operator T () const {
		return reinterpret_cast<T>( _InterlockedExchangeAdd( &Data, reinterpret_cast<std::uintptr_t>( 0 ) ) );
	}

	bool is_lock_free() const {
		return true;
	}
	
	T load() const {
		return reinterpret_cast<T>( _InterlockedExchangeAdd( &Data, reinterpret_cast<std::uintptr_t>( 0 ) ) );
	}
	void store( T val ) {
		_InterlockedExchange( &Data, reinterpret_cast<std::uintptr_t>( val ) );
	}
	T exchange( T val ) {
		return reinterpret_cast<T>( _InterlockedExchange( &Data, reinterpret_cast<std::uintptr_t>( val ) ) );
	}

	bool compare_exchange_strong( T & expected, T new_value ) {
		return reinterpret_cast<T>( _InterlockedCompareExchange( &Data, new_value, reinterpret_cast<std::uintptr_t>( expected ) ) ) == expected;
	}
	bool compare_exchange_weak( T & expected, T new_value ) {
		return reinterpret_cast<T>( _InterlockedCompareExchange( &Data, new_value, reinterpret_cast<std::uintptr_t>( expected ) ) ) == expected;
	}

}; // class atomic_impl_ptr

#endif // X86 || AMD64

#endif // MPT_COMPILER

#if MPT_GCC_BEFORE(4,4,0) || MPT_CLANG_BEFORE(3,1,0)
typedef mpt::atomic_impl<uint32> atomic_uint32_t;
typedef mpt::atomic_impl<int32> atomic_int32_t;
#define MPT_ATOMIC_PTR mpt::atomic_impl // use as MPT_ATOMIC_PTR<T*>
#elif MPT_MSVC_BEFORE(2012,0)
typedef mpt::atomic_impl_32<uint32> atomic_uint32_t;
typedef mpt::atomic_impl_32<int32> atomic_int32_t;
#define MPT_ATOMIC_PTR mpt::atomic_impl_ptr // use as MPT_ATOMIC_PTR<T*>
#else // MPT_COMPILER_GENERIC
typedef std::atomic<uint32> atomic_uint32_t;
typedef std::atomic<int32> atomic_int32_t;
#define MPT_ATOMIC_PTR std::atomic // use as MPT_ATOMIC_PTR<T*>
#endif // MPT_COMPILER


} // namespace mpt


#endif // MPT_ENABLE_ATOMIC


OPENMPT_NAMESPACE_END
