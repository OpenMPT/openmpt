/*
 * FlagSet.h
 * ---------
 * Purpose: A flexible and typesafe flag set class.
 * Notes  : Mostly taken from http://stackoverflow.com/questions/4226960/type-safer-bitflags-in-c
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include <string>

template <typename enum_t, typename store_t = enum_t>
class FlagSet
{
public:
	// Default constructor (no flags set)
	FlagSet() : flags(store_t(0))
	{

	}

	// Value constructor
	explicit FlagSet(enum_t value) : flags(static_cast<store_t>(value))
	{

	}

	// Explicit conversion operator
	operator enum_t() const
	{
		return static_cast<enum_t>(flags);
	}

	operator std::string() const
	{
		return to_string();
	}

	// Test if one or more flags are set. Returns true if at least one of the given flags is set.
	bool operator[] (enum_t flag) const
	{
		return test(flag);
	}

	// String representation of flag set
	std::string to_string() const
	{
		std::string str(size(), '0');

		for(size_t x = 0; x < size(); ++x)
		{
			str[size() - x - 1] = (flags & (1 << x) ? '1' : '0');
		}

		return str;
	}
	
	// Set one or more flags.
	FlagSet &set(enum_t flag)
	{
		flags = (flags | static_cast<store_t>(flag));
		return *this;
	}

	// Set or clear one or more flags.
	FlagSet &set(enum_t flag, bool val)
	{
		flags = (val ? (flags | static_cast<store_t>(flag)) : (flags & ~static_cast<store_t>(flag)));
		return *this;
	}

	// Clear or flags.
	FlagSet &reset()
	{
		flags = store_t(0);
		return *this;
	}

	// Clear one or more flags.
	FlagSet &reset(enum_t flag)
	{
		flags &= ~static_cast<store_t>(flag);
		return *this;
	}

	// Toggle all flags.
	FlagSet &flip()
	{
		flags = ~flags;
		return *this;
	}

	// Toggle one or more flags.
	FlagSet &flip(enum_t flag)
	{
		flags ^= static_cast<store_t>(flag);
		return *this;
	}

	// Returns the size of the flag set in bits
	size_t size() const
	{
		return sizeof(enum_t) * 8;
	}

	// Test if one or more flags are set. Returns true if at least one of the given flags is set.
	bool test(enum_t flag) const
	{
		return (flags & static_cast<store_t>(flag)) > 0;
	}

	// Test if any flag is set.
	bool any() const
	{
		return flags > 0;
	}

	// Test if no flags are set.
	bool none() const
	{
		return flags == 0;
	}

	FlagSet<enum_t, store_t> &operator = (const enum_t other)
	{
		flags = static_cast<store_t>(other);
		return *this;
	}

	FlagSet<enum_t, store_t> &operator &= (const enum_t other)
	{
		flags &= static_cast<store_t>(other);
		return *this;
	}

	FlagSet<enum_t, store_t> &operator |= (const enum_t other)
	{
		flags |= static_cast<store_t>(other);
		return *this;
	}

private:
	store_t flags;

};


// Declaration of a typesafe flag set enum.
// Usage: FLAGSET(enumName) { foo = 1, bar = 2, ... }
#define FLAGSET(enum_t) \
	enum enum_t; \
	/* Declare typesafe logical operators for flag set */ \
	inline enum_t operator | (enum_t a, enum_t b) { return static_cast<enum_t>(+a | +b); } \
	inline enum_t operator & (enum_t a, enum_t b) { return static_cast<enum_t>(+a & +b); } \
	inline enum_t &operator &= (enum_t &a, enum_t b) { a = (a & b); return a; } \
	inline enum_t &operator |= (enum_t &a, enum_t b) { a = (a | b); return a; } \
	inline enum_t operator ~ (enum_t a) { return static_cast<enum_t>(~(+a)); } \
	enum enum_t
