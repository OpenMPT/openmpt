/*
 * mptRandom.h
 * -----------
 * Purpose: PRNG
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "mptMutex.h"

#include <limits>
#include <random>

#ifdef MODPLUG_TRACKER
#include <cstdlib>
#endif // MODPLUG_TRACKER


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
// instantiating distribution objects for the common uniform distribution case.
//  We are still using std::random_device for initial seeding when avalable and
// after working around its set of problems.


namespace mpt
{


inline constexpr uint32 DETERMINISTIC_RNG_SEED = 3141592653u; // pi


namespace detail
{

MPT_CONSTEXPRINLINE int lower_bound_entropy_bits(unsigned int x)
{
	return mpt::bit_width(x) == static_cast<unsigned int>(mpt::popcount(x)) ? mpt::bit_width(x) : mpt::bit_width(x) - 1;
}

}


template <typename Trng> struct engine_traits
{
	typedef typename Trng::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits()
	{
		return Trng::result_bits();
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
	static_assert(std::numeric_limits<T>::is_integer);
	typedef typename std::make_unsigned<T>::type unsigned_T;
	const unsigned int rng_bits = mpt::engine_traits<Trng>::result_bits();
	unsigned_T result = 0;
	for(std::size_t entropy = 0; entropy < (sizeof(T) * 8); entropy += rng_bits)
	{
		if constexpr(rng_bits < (sizeof(T) * 8))
		{
			constexpr unsigned int shift_bits = rng_bits % (sizeof(T) * 8); // silence utterly stupid MSVC and GCC warnings about shifting by too big amount (in which case this branch is not even taken however)
			result = (result << shift_bits) ^ static_cast<unsigned_T>(rng());
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
	static_assert(std::numeric_limits<T>::is_integer);
	typedef typename std::make_unsigned<T>::type unsigned_T;
	const unsigned int rng_bits = mpt::engine_traits<Trng>::result_bits();
	unsigned_T result = 0;
	for(std::size_t entropy = 0; entropy < std::min(required_entropy_bits, sizeof(T) * 8); entropy += rng_bits)
	{
		if constexpr(rng_bits < (sizeof(T) * 8))
		{
			constexpr unsigned int shift_bits = rng_bits % (sizeof(T) * 8); // silence utterly stupid MSVC and GCC warnings about shifting by too big amount (in which case this branch is not even taken however)
			result = (result << shift_bits) ^ static_cast<unsigned_T>(rng());
		} else
		{
			result = static_cast<unsigned_T>(rng());
		}
	}
	if constexpr(required_entropy_bits >= (sizeof(T) * 8))
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
	static_assert(std::numeric_limits<T>::is_integer);
	typedef typename std::make_unsigned<T>::type unsigned_T;
	const unsigned int rng_bits = mpt::engine_traits<Trng>::result_bits();
	unsigned_T result = 0;
	for(std::size_t entropy = 0; entropy < std::min(required_entropy_bits, sizeof(T) * 8); entropy += rng_bits)
	{
		if constexpr(rng_bits < (sizeof(T) * 8))
		{
			constexpr unsigned int shift_bits = rng_bits % (sizeof(T) * 8); // silence utterly stupid MSVC and GCC warnings about shifting by too big amount (in which case this branch is not even taken however)
			result = (result << shift_bits) ^ static_cast<unsigned_T>(rng());
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

template <typename T>
struct uniform_real_distribution
{
private:
	T a;
	T b;
public:
	inline uniform_real_distribution(T a_, T b_)
		: a(a_)
		, b(b_)
	{
		return;
	}
	template <typename Trng>
	inline T operator()(Trng & rng) const
	{
		const int mantissa_bits = std::numeric_limits<T>::digits;
		return ((b - a) * static_cast<T>(mpt::random<uint64, mantissa_bits>(rng)) / static_cast<T>((static_cast<uint64>(1u) << mantissa_bits))) + a;
	}
};


template <typename T, typename Trng>
inline T random(Trng & rng, T min, T max)
{
	static_assert(!std::numeric_limits<T>::is_integer);
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

template <typename Tstate, typename Tvalue, Tstate m, Tstate a, Tstate c, Tstate result_mask, int result_shift, int result_bits_>
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
		operator()(); // we return results from the current state and update state after returning. results in better pipelining.
	}
	explicit inline lcg(state_type seed)
		: state(seed)
	{
		operator()(); // we return results from the current state and update state after returning. results in better pipelining.
	}
public:
	static MPT_CONSTEXPRINLINE result_type min()
	{
		return static_cast<result_type>(0);
	}
	static MPT_CONSTEXPRINLINE result_type max()
	{
		static_assert(((result_mask >> result_shift) << result_shift) == result_mask);
		return static_cast<result_type>(result_mask >> result_shift);
	}
	static MPT_CONSTEXPRINLINE int result_bits()
	{
		static_assert(((static_cast<Tstate>(1) << result_bits_) - 1) == (result_mask >> result_shift));
		return result_bits_;
	}
	inline result_type operator()()
	{
		// we return results from the current state and update state after returning. results in better pipelining.
		state_type s = state;
		result_type result = static_cast<result_type>((s & result_mask) >> result_shift);
		s = Util::ModIfNotZero<state_type, m>((a * s) + c);
		state = s;
		return result;
	}
};

#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

typedef lcg<uint32, uint16, 0u, 214013u, 2531011u, 0x7fff0000u, 16, 15> lcg_msvc;
typedef lcg<uint32, uint16, 0x80000000u, 1103515245u, 12345u, 0x7fff0000u, 16, 15> lcg_c99;
typedef lcg<uint64, uint32, 0ull, 6364136223846793005ull, 1ull, 0xffffffff00000000ull, 32, 32> lcg_musl;

template <typename Tstate, typename Tvalue, Tstate x1, Tstate x2, Tstate x3, Tstate x4, int rol1, int rol2>
class modplug
{
public:
	typedef Tstate state_type;
	typedef Tvalue result_type;
private:
	state_type state1;
	state_type state2;
public:
	template <typename Trng>
	explicit inline modplug(Trng &rd)
		: state1(mpt::random<state_type>(rd))
	  , state2(mpt::random<state_type>(rd))
	{
	}
	explicit inline modplug(state_type seed1, state_type seed2)
		: state1(seed1)
		, state2(seed2)
	{
	}
public:
	static MPT_CONSTEXPRINLINE result_type min()
	{
		return static_cast<result_type>(0);
	}
	static MPT_CONSTEXPRINLINE result_type max()
	{
		return std::numeric_limits<result_type>::max();
	}
	static MPT_CONSTEXPRINLINE int result_bits()
	{
		static_assert(std::is_integral<result_type>::value);
		static_assert(std::is_unsigned<result_type>::value);
		return std::numeric_limits<result_type>::digits;
	}
	inline result_type operator()()
	{
		state_type a = state1;
		state_type b = state2;
		a = mpt::rotl(a, rol1);
		a ^= x1;
		a += x2 + (b * x3);
		b += mpt::rotl(a, rol2) * x4;
		state1 = a;
		state2 = b;
		result_type result = static_cast<result_type>(b);
		return result;
	}
};

typedef modplug<uint32, uint32, 0x10204080u, 0x78649E7Du, 4, 5, 1, 16> modplug_dither;

} // namespace rng


#ifdef MODPLUG_TRACKER

namespace rng
{

class crand
{
public:
	using state_type = void;
	using result_type = int;
private:
	static void reseed(uint32 seed)
	{
		std::srand(seed);
	}
public:
	template <typename Trd>
	static void reseed(Trd & rd)
	{
		reseed(mpt::random<uint32>(rd));
	}
public:
	crand() = default;
	explicit crand(const std::string &)
	{
		return;
	}
public:
	static MPT_CONSTEXPRINLINE result_type min()
	{
		return 0;
	}
	static MPT_CONSTEXPRINLINE result_type max()
	{
		return RAND_MAX;
	}
	static MPT_CONSTEXPRINLINE int result_bits()
	{
		return detail::lower_bound_entropy_bits(RAND_MAX);
	}
	result_type operator()()
	{
		return std::rand();
	}
};

} // namespace rng

#endif // MODPLUG_TRACKER


//  C++11 std::random_device may be implemented as a deterministic PRNG.
//  There is no way to seed this PRNG and it is allowed to be seeded with the
// same value on each program invocation. This makes std::random_device
// completely useless even as a non-cryptographic entropy pool.
//  We fallback to time-seeded std::mt19937 if std::random_device::entropy() is
// 0 or less.
class sane_random_device
{
private:
	mpt::mutex m;
	std::string token;
#if !defined(MPT_COMPILER_QUIRK_RANDOM_NO_RANDOM_DEVICE)
	std::unique_ptr<std::random_device> prd;
	bool rd_reliable;
#endif // !MPT_COMPILER_QUIRK_RANDOM_NO_RANDOM_DEVICE
	std::unique_ptr<std::mt19937> rd_fallback;
public:
	using result_type = unsigned int;
private:
	void init_fallback();
public:
	sane_random_device();
	sane_random_device(const std::string & token);
	static MPT_CONSTEXPRINLINE result_type min()
	{
		return std::numeric_limits<result_type>::min();
	}
	static MPT_CONSTEXPRINLINE result_type max()
	{
		return std::numeric_limits<result_type>::max();
	}
	static MPT_CONSTEXPRINLINE int result_bits()
	{
		return sizeof(result_type) * 8;
	}
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
	enum : std::size_t { seed_bits = sizeof(std::mt19937::result_type) * 8 * std::mt19937::state_size };
	typedef std::mt19937 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		std::unique_ptr<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>> values = std::make_unique<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>>(rd);
		std::seed_seq seed(values->begin(), values->end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::mt19937_64> {
	enum : std::size_t { seed_bits = sizeof(std::mt19937_64::result_type) * 8 * std::mt19937_64::state_size };
	typedef std::mt19937_64 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		std::unique_ptr<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>> values = std::make_unique<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>>(rd);
		std::seed_seq seed(values->begin(), values->end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux24_base> {
	enum : std::size_t { seed_bits = std::ranlux24_base::word_size };
	typedef std::ranlux24_base rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux48_base> {
	enum : std::size_t { seed_bits = std::ranlux48_base::word_size };
	typedef std::ranlux48_base rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() { return rng_type::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux24> {
	enum : std::size_t { seed_bits = std::ranlux24_base::word_size };
	typedef std::ranlux24 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() { return std::ranlux24_base::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <> struct engine_traits<std::ranlux48> {
	enum : std::size_t { seed_bits = std::ranlux48_base::word_size };
	typedef std::ranlux48 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() { return std::ranlux48_base::word_size; }
	template<typename Trd> static inline rng_type make(Trd & rd)
	{
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};


class prng_random_device_time_seeder
{
private:
	uint8 generate_seed8();
	uint16 generate_seed16();
	uint32 generate_seed32();
	uint64 generate_seed64();
protected:
	template <typename T> inline T generate_seed();
protected:
	prng_random_device_time_seeder() = default;
};

template <> inline uint8 prng_random_device_time_seeder::generate_seed() { return generate_seed8(); }
template <> inline uint16 prng_random_device_time_seeder::generate_seed() { return generate_seed16(); }
template <> inline uint32 prng_random_device_time_seeder::generate_seed() { return generate_seed32(); }
template <> inline uint64 prng_random_device_time_seeder::generate_seed() { return generate_seed64(); }

class prng_random_device_deterministic_seeder
{
protected:
	template <typename T> constexpr T generate_seed() noexcept
	{
		return static_cast<T>(mpt::DETERMINISTIC_RNG_SEED);
	}
protected:
	prng_random_device_deterministic_seeder() = default;
};

template <typename Trng = mpt::rng::lcg_musl, typename seeder = mpt::prng_random_device_time_seeder>
class prng_random_device
	: private seeder
{
public:
	using result_type = unsigned int;
private:
	mpt::mutex m;
	Trng rng;
public:
	prng_random_device()
		: rng(seeder::template generate_seed<typename Trng::state_type>())
	{
		return;
	}
	prng_random_device(const std::string &)
		: rng(seeder::template generate_seed<typename Trng::state_type>())
	{
		return;
	}
	static MPT_CONSTEXPRINLINE result_type min()
	{
		return std::numeric_limits<unsigned int>::min();
	}
	static MPT_CONSTEXPRINLINE result_type max()
	{
		return std::numeric_limits<unsigned int>::max();
	}
	static MPT_CONSTEXPRINLINE int result_bits()
	{
		return sizeof(unsigned int) * 8;
	}
	result_type operator()()
	{
		mpt::lock_guard<mpt::mutex> l(m);
		return mpt::random<unsigned int>(rng);
	}
};


using deterministc_random_device = mpt::prng_random_device<mpt::rng::lcg_musl>;

using deterministic_fast_prng = mpt::rng::lcg_msvc;
using deterministic_good_prng = mpt::rng::lcg_musl;


template <typename Trng, typename Trd>
inline Trng make_prng(Trd & rd)
{
	return mpt::engine_traits<Trng>::make(rd);
}


template <typename Trng>
class thread_safe_prng
	: private Trng
{
private:
	mpt::mutex m;
public:
	typedef typename Trng::result_type result_type;
public:
	template <typename Trd>
	explicit thread_safe_prng(Trd & rd)
		: Trng(mpt::make_prng<Trng>(rd))
	{
		return;
	}
	thread_safe_prng(Trng rng)
		: Trng(rng)
	{
		return;
	}
public:
	static MPT_CONSTEXPRINLINE typename engine_traits<Trng>::result_type min()
	{
		return Trng::min();
	}
	static MPT_CONSTEXPRINLINE typename engine_traits<Trng>::result_type max()
	{
		return Trng::max();
	}
	static MPT_CONSTEXPRINLINE int result_bits()
	{
		return engine_traits<Trng>::result_bits();
	}
public:
	typename engine_traits<Trng>::result_type operator()()
	{
		mpt::lock_guard<mpt::mutex> l(m);
		return Trng::operator()();
	}
};


#ifdef MPT_BUILD_FUZZER

//  1. Use deterministic seeding
using random_device = deterministc_random_device;

//  2. Use fast PRNGs in order to not waste time fuzzing more complex PRNG
//     implementations.
using fast_prng = deterministic_fast_prng;
using good_prng = deterministic_good_prng;

#else // !MPT_BUILD_FUZZER

// mpt::random_device always generates 32 bits of entropy
using random_device = mpt::sane_random_device;

// We cannot use std::minstd_rand here because it has not a power-of-2 sized
// output domain which we rely upon.
using fast_prng = mpt::rng::lcg_msvc; // about 3 ALU operations, ~32bit of state, suited for inner loops
using good_prng = std::ranlux48;

#endif // MPT_BUILD_FUZZER


using default_prng = mpt::good_prng;


mpt::random_device & global_random_device();
mpt::thread_safe_prng<mpt::default_prng> & global_prng();

#if defined(MODPLUG_TRACKER) && !defined(MPT_BUILD_WINESUPPORT)
void set_global_random_device(mpt::random_device *rd);
void set_global_prng(mpt::thread_safe_prng<mpt::default_prng> *rng);
#endif // MODPLUG_TRACKER && !MPT_BUILD_WINESUPPORT


} // namespace mpt


OPENMPT_NAMESPACE_END
