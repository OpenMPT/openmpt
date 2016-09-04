/*
 * FlagSet.h
 * ---------
 * Purpose: A flexible and typesafe flag set class.
 * Notes  : Originally based on http://stackoverflow.com/questions/4226960/type-safer-bitflags-in-c .
 *          Rewritten to be standard-conforming.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <string>

OPENMPT_NAMESPACE_BEGIN


// Be aware of the required size when specializing this.
// We cannot assert the minimum size because some compilers always allocate an 'int',
// even for enums that would fit in smaller integral types.
template <typename Tenum>
struct enum_traits
{
	typedef typename mpt::make_unsigned<Tenum>::type store_type;
};


// Type-safe wrapper around an enum, that can represent all enum values and bitwise compositions thereof.
// Conversions to and from plain integers as well as conversions to the base enum are always explicit.
template <typename enum_t>
class enum_value_type
{
public:
	typedef enum_t enum_type;
	typedef enum_value_type value_type;
	typedef typename enum_traits<enum_t>::store_type store_type;
private:
	store_type bits;
public:
	MPT_FORCEINLINE enum_value_type() : bits(0) { }
	MPT_FORCEINLINE enum_value_type(const enum_value_type &x) : bits(x.bits) { }
	MPT_FORCEINLINE enum_value_type(enum_type x) : bits(static_cast<store_type>(x)) { }
private:
	explicit MPT_FORCEINLINE enum_value_type(store_type x) : bits(x) { } // private in order to prevent accidental conversions. use from_bits.
	MPT_FORCEINLINE operator store_type () const { return bits; }  // private in order to prevent accidental conversions. use as_bits.
public:
	static MPT_FORCEINLINE enum_value_type from_bits(store_type bits) { return value_type(bits); }
	MPT_FORCEINLINE enum_type as_enum() const { return static_cast<enum_t>(bits); }
	MPT_FORCEINLINE store_type as_bits() const { return bits; }
public:
	MPT_FORCEINLINE operator bool () const { return bits != store_type(); }
	MPT_FORCEINLINE bool operator ! () const { return bits == store_type(); }

	MPT_FORCEINLINE const enum_value_type operator ~ () const { return enum_value_type(~bits); }

	friend MPT_FORCEINLINE bool operator == (enum_value_type a, enum_value_type b) { return a.bits == b.bits; }
	friend MPT_FORCEINLINE bool operator != (enum_value_type a, enum_value_type b) { return a.bits != b.bits; }
	
	friend MPT_FORCEINLINE bool operator == (enum_value_type a, enum_t b) { return a == enum_value_type(b); }
	friend MPT_FORCEINLINE bool operator != (enum_value_type a, enum_t b) { return a != enum_value_type(b); }
	
	friend MPT_FORCEINLINE bool operator == (enum_t a, enum_value_type b) { return enum_value_type(a) == b; }
	friend MPT_FORCEINLINE bool operator != (enum_t a, enum_value_type b) { return enum_value_type(a) != b; }
	
	friend MPT_FORCEINLINE const enum_value_type operator | (enum_value_type a, enum_value_type b) { return enum_value_type(a.bits | b.bits); }
	friend MPT_FORCEINLINE const enum_value_type operator & (enum_value_type a, enum_value_type b) { return enum_value_type(a.bits & b.bits); }
	friend MPT_FORCEINLINE const enum_value_type operator ^ (enum_value_type a, enum_value_type b) { return enum_value_type(a.bits ^ b.bits); }
	
	friend MPT_FORCEINLINE const enum_value_type operator | (enum_value_type a, enum_t b) { return a | enum_value_type(b); }
	friend MPT_FORCEINLINE const enum_value_type operator & (enum_value_type a, enum_t b) { return a & enum_value_type(b); }
	friend MPT_FORCEINLINE const enum_value_type operator ^ (enum_value_type a, enum_t b) { return a ^ enum_value_type(b); }
	
	friend MPT_FORCEINLINE const enum_value_type operator | (enum_t a, enum_value_type b) { return enum_value_type(a) | b; }
	friend MPT_FORCEINLINE const enum_value_type operator & (enum_t a, enum_value_type b) { return enum_value_type(a) & b; }
	friend MPT_FORCEINLINE const enum_value_type operator ^ (enum_t a, enum_value_type b) { return enum_value_type(a) ^ b; }
	
	MPT_FORCEINLINE enum_value_type &operator |= (enum_value_type b) { *this = *this | b; return *this; }
	MPT_FORCEINLINE enum_value_type &operator &= (enum_value_type b) { *this = *this & b; return *this; }
	MPT_FORCEINLINE enum_value_type &operator ^= (enum_value_type b) { *this = *this ^ b; return *this; }

	MPT_FORCEINLINE enum_value_type &operator |= (enum_t b) { *this = *this | b; return *this; }
	MPT_FORCEINLINE enum_value_type &operator &= (enum_t b) { *this = *this & b; return *this; }
	MPT_FORCEINLINE enum_value_type &operator ^= (enum_t b) { *this = *this ^ b; return *this; }

};


// Type-safe enum wrapper that allows type-safe bitwise testing.
template <typename enum_t>
class Enum
{
public:
	typedef Enum self_type;
	typedef enum_t enum_type;
	typedef enum_value_type<enum_t> value_type;
	typedef typename value_type::store_type store_type;
private:
	enum_type value;
public:
	explicit MPT_FORCEINLINE Enum(enum_type val) : value(val) { }
	MPT_FORCEINLINE operator enum_type () const { return value; }
	MPT_FORCEINLINE Enum &operator = (enum_type val) { value = val; return *this; }
public:
	MPT_FORCEINLINE const value_type operator ~ () const { return ~value_type(value); }

	friend MPT_FORCEINLINE bool operator == (self_type a, self_type b) { return value_type(a) == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (self_type a, self_type b) { return value_type(a) != value_type(b); }

	friend MPT_FORCEINLINE bool operator == (self_type a, value_type b) { return value_type(a) == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (self_type a, value_type b) { return value_type(a) != value_type(b); }

	friend MPT_FORCEINLINE bool operator == (value_type a, self_type b) { return value_type(a) == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (value_type a, self_type b) { return value_type(a) != value_type(b); }

	friend MPT_FORCEINLINE bool operator == (self_type a, enum_type b) { return value_type(a) == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (self_type a, enum_type b) { return value_type(a) != value_type(b); }

	friend MPT_FORCEINLINE bool operator == (enum_type a, self_type b) { return value_type(a) == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (enum_type a, self_type b) { return value_type(a) != value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (self_type a, self_type b) { return value_type(a) | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (self_type a, self_type b) { return value_type(a) & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (self_type a, self_type b) { return value_type(a) ^ value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (self_type a, value_type b) { return value_type(a) | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (self_type a, value_type b) { return value_type(a) & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (self_type a, value_type b) { return value_type(a) ^ value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (value_type a, self_type b) { return value_type(a) | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (value_type a, self_type b) { return value_type(a) & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (value_type a, self_type b) { return value_type(a) ^ value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (self_type a, enum_type b) { return value_type(a) | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (self_type a, enum_type b) { return value_type(a) & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (self_type a, enum_type b) { return value_type(a) ^ value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (enum_type a, self_type b) { return value_type(a) | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (enum_type a, self_type b) { return value_type(a) & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (enum_type a, self_type b) { return value_type(a) ^ value_type(b); }

};


template <typename enum_t, typename store_t = typename enum_value_type<enum_t>::store_type >
class FlagSet
{
public:
	typedef FlagSet self_type;
	typedef enum_t enum_type;
	typedef enum_value_type<enum_t> value_type;
	typedef store_t store_type;
	
private:

	// support truncated store_type ... :
	store_type bits_;
	static MPT_FORCEINLINE store_type store_from_value(value_type bits) { return static_cast<store_type>(bits.as_bits()); }
	static MPT_FORCEINLINE value_type value_from_store(store_type bits) { return value_type::from_bits(static_cast<typename value_type::store_type>(bits)); }

	MPT_FORCEINLINE void store(value_type bits) { bits_ = store_from_value(bits); }
	MPT_FORCEINLINE value_type load() const { return value_from_store(bits_); }

public:

	// Default constructor (no flags set)
	MPT_FORCEINLINE FlagSet() : bits_(store_from_value(value_type()))
	{
		return;
	}

	// Value constructor
	MPT_FORCEINLINE FlagSet(value_type flags) : bits_(store_from_value(value_type(flags)))
	{
		return;
	}

	MPT_FORCEINLINE FlagSet(enum_type flag) : bits_(store_from_value(value_type(flag)))
	{
		return;
	}

	explicit MPT_FORCEINLINE FlagSet(store_type flags) : bits_(store_from_value(value_type::from_bits(flags)))
	{
		return;
	}
	
	MPT_FORCEINLINE operator bool () const
	{
		return load();
	}
	// In order to catch undesired conversions to bool in integer contexts,
	// add a deprecated conversion operator to store_type.
	// C++11 explicit conversion cast operators ('explicit operator bool ();')
	// would solve this in a better way and always fail at compile-time instead of this
	// solution which just warns in some cases.
	// The macro-based extended instrument fields writer in InstrumentExtensions.cpp currently needs this conversion,
	// so it is not marked deprecated (for now).
	/*MPT_DEPRECATED*/ MPT_FORCEINLINE operator store_type () const
	{
		return load().as_bits();
	}
	
	MPT_FORCEINLINE value_type value() const
	{
		return load();
	}
	
	MPT_FORCEINLINE operator value_type () const
	{
		return load();
	}

	// Test if one or more flags are set. Returns true if at least one of the given flags is set.
	MPT_FORCEINLINE bool operator[] (value_type flags) const
	{
		return test(flags);
	}

	// String representation of flag set
	std::string to_string() const
	{
		std::string str(size_bits(), '0');

		for(size_t x = 0; x < size_bits(); ++x)
		{
			str[size_bits() - x - 1] = (load() & (1 << x) ? '1' : '0');
		}

		return str;
	}
	
	// Set one or more flags.
	MPT_FORCEINLINE FlagSet &set(value_type flags)
	{
		store(load() | flags);
		return *this;
	}

	// Set or clear one or more flags.
	MPT_FORCEINLINE FlagSet &set(value_type flags, bool val)
	{
		store((val ? (load() | flags) : (load() & ~flags)));
		return *this;
	}

	// Clear or flags.
	MPT_FORCEINLINE FlagSet &reset()
	{
		store(value_type());
		return *this;
	}

	// Clear one or more flags.
	MPT_FORCEINLINE FlagSet &reset(value_type flags)
	{
		store(load() & ~flags);
		return *this;
	}

	// Toggle all flags.
	MPT_FORCEINLINE FlagSet &flip()
	{
		store(~load());
		return *this;
	}

	// Toggle one or more flags.
	MPT_FORCEINLINE FlagSet &flip(value_type flags)
	{
		store(load() ^ flags);
		return *this;
	}

	// Returns the size of the flag set in bytes
	MPT_FORCEINLINE std::size_t size() const
	{
		return sizeof(store_type);
	}

	// Returns the size of the flag set in bits
	MPT_FORCEINLINE std::size_t size_bits() const
	{
		return size() * 8;
	}
	
	// Test if one or more flags are set. Returns true if at least one of the given flags is set.
	MPT_FORCEINLINE bool test(value_type flags) const
	{
		return (load() & flags);
	}

	// Test if all specified flags are set.
	MPT_FORCEINLINE bool test_all(value_type flags) const
	{
		return (load() & flags) == flags;
	}

	// Test if any flag is set.
	MPT_FORCEINLINE bool any() const
	{
		return load();
	}

	// Test if no flags are set.
	MPT_FORCEINLINE bool none() const
	{
		return !load();
	}
	
	MPT_FORCEINLINE store_type GetRaw() const
	{
		return bits_;
	}

	MPT_FORCEINLINE void SetRaw(store_type flags)
	{
		bits_ = flags;
	}

	MPT_FORCEINLINE FlagSet &operator = (value_type flags)
	{
		store(flags);
		return *this;
	}
	
	MPT_FORCEINLINE FlagSet &operator = (enum_type flag)
	{
		store(flag);
		return *this;
	}

	MPT_FORCEINLINE FlagSet &operator = (FlagSet flags)
	{
		store(flags.load());
		return *this;
	}

	MPT_FORCEINLINE FlagSet &operator &= (value_type flags)
	{
		store(load() & flags);
		return *this;
	}

	MPT_FORCEINLINE FlagSet &operator |= (value_type flags)
	{
		store(load() | flags);
		return *this;
	}

	MPT_FORCEINLINE FlagSet &operator ^= (value_type flags)
	{
		store(load() ^ flags);
		return *this;
	}
	
	friend MPT_FORCEINLINE bool operator == (self_type a, self_type b) { return a.load() == b.load(); }
	friend MPT_FORCEINLINE bool operator != (self_type a, self_type b) { return a.load() != b.load(); }

	friend MPT_FORCEINLINE bool operator == (self_type a, value_type b) { return a.load() == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (self_type a, value_type b) { return a.load() != value_type(b); }

	friend MPT_FORCEINLINE bool operator == (value_type a, self_type b) { return value_type(a) == b.load(); }
	friend MPT_FORCEINLINE bool operator != (value_type a, self_type b) { return value_type(a) != b.load(); }

	friend MPT_FORCEINLINE bool operator == (self_type a, enum_type b) { return a.load() == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (self_type a, enum_type b) { return a.load() != value_type(b); }

	friend MPT_FORCEINLINE bool operator == (enum_type a, self_type b) { return value_type(a) == b.load(); }
	friend MPT_FORCEINLINE bool operator != (enum_type a, self_type b) { return value_type(a) != b.load(); }

	friend MPT_FORCEINLINE bool operator == (self_type a, Enum<enum_type> b) { return a.load() == value_type(b); }
	friend MPT_FORCEINLINE bool operator != (self_type a, Enum<enum_type> b) { return a.load() != value_type(b); }

	friend MPT_FORCEINLINE bool operator == (Enum<enum_type> a, self_type b) { return value_type(a) == b.load(); }
	friend MPT_FORCEINLINE bool operator != (Enum<enum_type> a, self_type b) { return value_type(a) != b.load(); }

	friend MPT_FORCEINLINE const value_type operator | (self_type a, self_type b) { return a.load() | b.load(); }
	friend MPT_FORCEINLINE const value_type operator & (self_type a, self_type b) { return a.load() & b.load(); }
	friend MPT_FORCEINLINE const value_type operator ^ (self_type a, self_type b) { return a.load() ^ b.load(); }

	friend MPT_FORCEINLINE const value_type operator | (self_type a, value_type b) { return a.load() | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (self_type a, value_type b) { return a.load() & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (self_type a, value_type b) { return a.load() ^ value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (value_type a, self_type b) { return value_type(a) | b.load(); }
	friend MPT_FORCEINLINE const value_type operator & (value_type a, self_type b) { return value_type(a) & b.load(); }
	friend MPT_FORCEINLINE const value_type operator ^ (value_type a, self_type b) { return value_type(a) ^ b.load(); }

	friend MPT_FORCEINLINE const value_type operator | (self_type a, enum_type b) { return a.load() | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (self_type a, enum_type b) { return a.load() & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (self_type a, enum_type b) { return a.load() ^ value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (enum_type a, self_type b) { return value_type(a) | b.load(); }
	friend MPT_FORCEINLINE const value_type operator & (enum_type a, self_type b) { return value_type(a) & b.load(); }
	friend MPT_FORCEINLINE const value_type operator ^ (enum_type a, self_type b) { return value_type(a) ^ b.load(); }

	friend MPT_FORCEINLINE const value_type operator | (self_type a, Enum<enum_type> b) { return a.load() | value_type(b); }
	friend MPT_FORCEINLINE const value_type operator & (self_type a, Enum<enum_type> b) { return a.load() & value_type(b); }
	friend MPT_FORCEINLINE const value_type operator ^ (self_type a, Enum<enum_type> b) { return a.load() ^ value_type(b); }

	friend MPT_FORCEINLINE const value_type operator | (Enum<enum_type> a, self_type b) { return value_type(a) | b.load(); }
	friend MPT_FORCEINLINE const value_type operator & (Enum<enum_type> a, self_type b) { return value_type(a) & b.load(); }
	friend MPT_FORCEINLINE const value_type operator ^ (Enum<enum_type> a, self_type b) { return value_type(a) ^ b.load(); }

};


// Declare typesafe logical operators for enum_t
#define MPT_DECLARE_ENUM(enum_t) \
	MPT_FORCEINLINE enum_value_type<enum_t> operator | (enum_t a, enum_t b) { return enum_value_type<enum_t>(a) | enum_value_type<enum_t>(b); } \
	MPT_FORCEINLINE enum_value_type<enum_t> operator & (enum_t a, enum_t b) { return enum_value_type<enum_t>(a) & enum_value_type<enum_t>(b); } \
	MPT_FORCEINLINE enum_value_type<enum_t> operator ^ (enum_t a, enum_t b) { return enum_value_type<enum_t>(a) ^ enum_value_type<enum_t>(b); } \
	MPT_FORCEINLINE enum_value_type<enum_t> operator ~ (enum_t a) { return ~enum_value_type<enum_t>(a); } \
/**/

// backwards compatibility
#define DECLARE_FLAGSET MPT_DECLARE_ENUM


OPENMPT_NAMESPACE_END
