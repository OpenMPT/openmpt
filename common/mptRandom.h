/*
 * mptRandom.h
 * -----------
 * Purpose: PRNG
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#if MPT_COMPILER_GENERIC || (MPT_COMPILER_MSVC && MPT_MSVC_AT_LEAST(2010,0)) || (MPT_COMPILER_GCC && MPT_GCC_AT_LEAST(4,5,0)) || (MPT_COMPILER_CLANG && MPT_CLANG_AT_LEAST(3,0,0))
#define MPT_STD_RANDOM 1
#else // MPT_COMPILER
#define MPT_STD_RANDOM 0
#endif // MPT_COMPILER

#include <limits>
#if MPT_STD_RANDOM
#include <random>
#endif // MPT_STD_RANDOM


OPENMPT_NAMESPACE_BEGIN


// NOTE:
//  We implement our own PRNG and distribution functions as the implementations
// of std::uniform_int_distribution is either wrong (not uniform in MSVC2010) or
// not guaranteed to be livelock-free for bad PRNGs (in GCC, Clang, boost).
// We resort to a simpler implementation with only power-of-2 result ranges for
// both the underlying PRNG and our interface function. This saves us from
// complicated code having to deal with partial bits of entropy.
//  Our interface still somewhat follows the mindset of C++11 <random> (with the
// addition of a simple wrapper function mpt::random which saves the caller from
// instatiating distribution objects for the common uniform distribution case.
//  We are still using std::random_device for initial seeding when avalable and
// after working around its set of problems.


namespace mpt
{


template <typename Trng> struct engine_traits
{
	static inline int entropy_bits()
	{
		return Trng::bits();
	}
	template<typename Trd>
	static inline Trng make(Trd & rd)
	{
		return Trng(rd);
	}
};


template <typename T, typename Trng>
inline T random(Trng & rng)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	typedef typename mpt::make_unsigned<T>::type unsigned_T;
	const unsigned int rng_bits = mpt::engine_traits<Trng>::entropy_bits();
	unsigned_T result = 0;
	for(std::size_t entropy = 0; entropy < (sizeof(T) * 8); entropy += rng_bits)
	{
		MPT_CONSTANT_IF(rng_bits < (sizeof(T) * 8))
		{
			result = (result << rng_bits) ^ static_cast<unsigned_T>(rng());
		} else
		{
			result = static_cast<unsigned_T>(rng());
		}
	}
	return static_cast<T>(result);
}

template <typename T, std::size_t required_entropy_bits, typename Trng>
inline T random(Trng & rng)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	typedef typename mpt::make_unsigned<T>::type unsigned_T;
	const unsigned int rng_bits = mpt::engine_traits<Trng>::entropy_bits();
	unsigned_T result = 0;
	for(std::size_t entropy = 0; entropy < std::min<std::size_t>(required_entropy_bits, sizeof(T) * 8); entropy += rng_bits)
	{
		if(rng_bits < (sizeof(T) * 8))
		{
			result = (result << rng_bits) ^ static_cast<unsigned_T>(rng());
		} else
		{
			result = static_cast<unsigned_T>(rng());
		}
	}
	MPT_CONSTANT_IF(required_entropy_bits >= (sizeof(T) * 8))
	{
		return static_cast<T>(result);
	} else
	{
		return static_cast<T>(result & ((static_cast<unsigned_T>(1) << required_entropy_bits) - static_cast<unsigned_T>(1)));
	}
}

template <typename T, typename Trng>
inline T random(Trng & rng, std::size_t required_entropy_bits)
{
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	typedef typename mpt::make_unsigned<T>::type unsigned_T;
	const unsigned int rng_bits = mpt::engine_traits<Trng>::entropy_bits();
	unsigned_T result = 0;
	for(std::size_t entropy = 0; entropy < std::min<std::size_t>(required_entropy_bits, sizeof(T) * 8); entropy += rng_bits)
	{
		if(rng_bits < (sizeof(T) * 8))
		{
			result = (result << rng_bits) ^ static_cast<unsigned_T>(rng());
		} else
		{
			result = static_cast<unsigned_T>(rng());
		}
	}
	if(required_entropy_bits >= (sizeof(T) * 8))
	{
		return static_cast<T>(result);
	} else
	{
		return static_cast<T>(result & ((static_cast<unsigned_T>(1) << required_entropy_bits) - static_cast<unsigned_T>(1)));
	}
}

template <typename Tf> struct float_traits { };
template <> struct float_traits<float> {
	typedef uint32 mantissa_uint_type;
	static const int mantissa_bits = 24;
};
template <> struct float_traits<double> {
	typedef uint64 mantissa_uint_type;
	static const int mantissa_bits = 53;
};
template <> struct float_traits<long double> {
	typedef uint64 mantissa_uint_type;
	static const int mantissa_bits = 63;
};

template <typename T>
struct uniform_real_distribution
{
private:
	T a;
	T b;
public:
	inline uniform_real_distribution(T a, T b)
		: a(a)
		, b(b)
	{
		return;
	}
	template <typename Trng>
	inline T operator()(Trng & rng) const
	{
		typedef typename float_traits<T>::mantissa_uint_type uint_type;
		const int bits = float_traits<T>::mantissa_bits;
		return ((b - a) * static_cast<T>(mpt::random<uint_type, bits>(rng)) / static_cast<T>((static_cast<uint_type>(1u) << bits) - 1u)) + a;
	}
};


template <typename T, typename Trng>
inline T random(Trng & rng, T min, T max)
{
	STATIC_ASSERT(!std::numeric_limits<T>::is_integer);
	typedef mpt::uniform_real_distribution<T> dis_type;
	dis_type dis(min, max);
	return static_cast<T>(dis(rng));
}


namespace rng
{

#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4724) // potential mod by 0
#endif // MPT_COMPILER_MSVC

template <typename Tstate, typename Tvalue, Tstate m, Tstate a, Tstate c, Tstate result_mask, int result_shift, int result_bits>
class lcg
{
public:
	typedef Tstate state_type;
	typedef Tvalue result_type;
private:
	state_type state;
public:
	template <typename Trng>
	explicit inline lcg(Trng & rd)
		: state(mpt::random<state_type>(rd))
	{
		random(); // we return results from the current state and update state after returning. results in better pipelining.
	}
	explicit inline lcg(state_type seed)
		: state(seed)
	{
		random(); // we return results from the current state and update state after returning. results in better pipelining.
	}
public:
	static inline result_type min()
	{
		return static_cast<result_type>(0);
	}
	static inline result_type max()
	{
		STATIC_ASSERT(((result_mask >> result_shift) << result_shift) == result_mask);
		return static_cast<result_type>(result_mask >> result_shift);
	}
	static inline int bits()
	{
		STATIC_ASSERT(((static_cast<Tstate>(1) << result_bits) - 1) == (result_mask >> result_shift));
		return result_bits;
	}
	inline result_type operator()()
	{
		return random();
	}
	inline result_type random()
	{
		// we return results from the current state and update state after returning. results in better pipelining.
		state_type s = state;
		result_type result = static_cast<result_type>((s & result_mask) >> result_shift);
		s = (m == 0) ? ((a * s) + c) : (((a * s) + c) % m);
		state = s;
		return result;
	}
	inline result_type random_bits()
	{
		return random();
	}
};

#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

typedef lcg<uint32, uint16, 0u, 214013u, 2531011u, 0x7fff0000u, 16, 15> lcg_msvc;
typedef lcg<uint32, uint16, 0x80000000u, 1103515245u, 12345u, 0x7fff0000u, 16, 15> lcg_c99;
typedef lcg<uint64, uint32, 0ull, 6364136223846793005ull, 1ull, 0xffffffff00000000ull, 32, 32> lcg_musl;

} // namespace rng


#ifdef MODPLUG_TRACKER

namespace rng
{

class crand
{
public:
	typedef void state_type;
	typedef int result_type;
private:
	static void reseed(uint32 seed);
public:
	template <typename Trd>
	static void reseed(Trd & rd)
	{
		reseed(mpt::random<uint32>(rd));
	}
public:
	crand() { }
	explicit crand(const std::string &) { }
public:
	static result_type min();
	static result_type max();
	static int bits();
	result_type random();
	result_type operator()();
};

} // namespace rng

#endif // MODPLUG_TRACKER


#if MPT_STD_RANDOM


//  C++11 std::random_device may be implemented as a deterministic PRNG.
//  There is no way to seed this PRNG and it is allowed to be seeded with the
// same value on each program invocation. This makes std::random_device
// completely useless even as a non-cryptographic entropy pool.
//  We fallback to time-seeded std::mt19937 if std::random_device::entropy() is
// 0 or less.
class sane_random_device
{
private:
	bool rd_reliable;
	std::random_device rd;
	MPT_SHARED_PTR<std::mt19937> rd_fallback;
public:
	typedef unsigned int result_type;
public:
	sane_random_device();
	sane_random_device(const std::string & token);
	static result_type min();
	static result_type max();
	static int bits();
	result_type operator()();
};


template <std::size_t N>
class seed_seq_values
{
private:
	unsigned int seeds[N];
public:
	template <typename Trd>
	explicit seed_seq_values(Trd & rd)
	{
		for(std::size_t i = 0; i < N; ++i)
		{
			seeds[i] = rd();
		}
	}
	const unsigned int * begin() const
	{
		return seeds + 0;
	}
	const unsigned int * end() const
	{
		return seeds + N;
	}
};


// C++11 random does not provide any sane way to determine the amount of entropy
// required to seed a particular engine. VERY STUPID.
// List the ones we are likely to use.

template <> struct engine_traits<std::mt19937> {
	static const std::size_t seed_bits = sizeof(std::mt19937::result_type) * 8 * std::mt19937::state_size;
	typedef std::mt19937 rng_type;
	static inline int entropy_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::mt19937_64> {
	static const std::size_t seed_bits = sizeof(std::mt19937_64::result_type) * 8 * std::mt19937_64::state_size;
	typedef std::mt19937_64 rng_type;
	static inline int entropy_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux24_base> {
	static const std::size_t seed_bits = std::ranlux24_base::word_size;
	typedef std::ranlux24_base rng_type;
	static inline int entropy_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux48_base> {
	static const std::size_t seed_bits = std::ranlux48_base::word_size;
	typedef std::ranlux48_base rng_type;
	static inline int entropy_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux24> {
	static const std::size_t seed_bits = std::ranlux24_base::word_size;
	typedef std::ranlux24 rng_type;
	static inline int entropy_bits() { return std::ranlux24_base::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux48> {
	static const std::size_t seed_bits = std::ranlux48_base::word_size;
	typedef std::ranlux48 rng_type;
	static inline int entropy_bits() { return std::ranlux48_base::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};


#else // !MPT_STD_RANDOM


class prng_random_device_seeder
{
private:
	uint8 generate_seed8();
	uint16 generate_seed16();
	uint32 generate_seed32();
	uint64 generate_seed64();
protected:
	template <typename T> inline T generate_seed();
protected:
	prng_random_device_seeder();
};

template <> inline uint8 prng_random_device_seeder::generate_seed() { return generate_seed8(); }
template <> inline uint16 prng_random_device_seeder::generate_seed() { return generate_seed16(); }
template <> inline uint32 prng_random_device_seeder::generate_seed() { return generate_seed32(); }
template <> inline uint64 prng_random_device_seeder::generate_seed() { return generate_seed64(); }

template <typename Trng = mpt::rng::lcg_musl>
class prng_random_device
	: public prng_random_device_seeder
{
public:
	typedef unsigned int result_type;
private:
	Trng rng;
public:
	prng_random_device()
		: rng(generate_seed<typename Trng::state_type>())
	{
		return;
	}
	prng_random_device(const std::string &)
		: rng(generate_seed<typename Trng::state_type>())
	{
		return;
	}
	static result_type min()
	{
		return std::numeric_limits<unsigned int>::min();
	}
	static result_type max()
	{
		return std::numeric_limits<unsigned int>::max();
	}
	static int bits()
	{
		return sizeof(unsigned int) * 8;
	}
	result_type operator()()
	{
		return mpt::random<unsigned int>(rng);
	}
	result_type random()
	{
		return operator()();
	}
	result_type random_bits()
	{
		return operator()();
	}
};


#endif // MPT_STD_RANDOM


#if MPT_STD_RANDOM

// mpt::random_device always generates 32 bits of entropy
typedef mpt::sane_random_device random_device;

// We cannot use std::minstd_rand here because it has not a power-of-2 sized
// output domain which we rely upon.
typedef mpt::rng::lcg_msvc fast_prng; // about 3 ALU operations, ~32bit of state, suited for inner loops
typedef std::mt19937       main_prng;
typedef std::ranlux48      best_prng;

#else // !MPT_STD_RANDOM

// easy to implement fallbacks, only used on very old compilers

typedef mpt::prng_random_device<mpt::rng::lcg_musl> random_device;

typedef mpt::rng::lcg_msvc fast_prng;
typedef mpt::rng::lcg_c99  main_prng;
typedef mpt::rng::lcg_musl best_prng;

#endif // MPT_STD_RANDOM


typedef mpt::main_prng default_prng;
typedef mpt::main_prng prng;


template <typename Trng, typename Trd>
inline Trng make_prng(Trd & rd)
{
	return mpt::engine_traits<Trng>::make(rd);
}


mpt::random_device & global_random_device();

#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)
void set_global_random_device(mpt::random_device *rd);
#endif // MODPLUG_TRACKER && !MPT_BUILD_WINESUPPORT


} // namespace mpt


OPENMPT_NAMESPACE_END
