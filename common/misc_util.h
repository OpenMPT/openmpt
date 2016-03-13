/*
 * misc_util.h
 * -----------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "mptStringParse.h"
#include "mptCPU.h"
#include "mptOS.h"
#include "mptTime.h"
#include "mptUUID.h"
#include "mptLibrary.h"
#include "mptTypeTraits.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include <cmath>
#include <cstring>


OPENMPT_NAMESPACE_BEGIN


// cmath fixups
#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif



namespace mpt { namespace String {

// Combine a vector of values into a string, separated with the given separator.
// No escaping is performed.
template<typename T>
mpt::ustring Combine(const std::vector<T> &vals, const mpt::ustring &sep=MPT_USTRING(","))
//----------------------------------------------------------------------------------------
{
	mpt::ustring str;
	for(std::size_t i = 0; i < vals.size(); ++i)
	{
		if(i > 0)
		{
			str += sep;
		}
		str += mpt::ToUString(vals[i]);
	}
	return str;
}
template<typename T>
std::string Combine(const std::vector<T> &vals, const std::string &sep=std::string(","))
//--------------------------------------------------------------------------------------
{
	std::string str;
	for(std::size_t i = 0; i < vals.size(); ++i)
	{
		if(i > 0)
		{
			str += sep;
		}
		str += mpt::ToString(vals[i]);
	}
	return str;
}

// Split the given string at separator positions into individual values returned as a vector.
// An empty string results in an empty vector.
// Leading or trailing separators result in a default-constructed element being inserted before or after the other elements.
template<typename T>
std::vector<T> Split(const mpt::ustring &str, const mpt::ustring &sep=MPT_USTRING(","))
//-------------------------------------------------------------------------------------
{
	std::vector<T> vals;
	std::size_t pos = 0;
	while(str.find(sep, pos) != std::string::npos)
	{
		vals.push_back(ConvertStrTo<T>(str.substr(pos, str.find(sep, pos) - pos)));
		pos = str.find(sep, pos) + sep.length();
	}
	if(!vals.empty() || (str.substr(pos).length() > 0))
	{
		vals.push_back(ConvertStrTo<T>(str.substr(pos)));
	}
	return vals;
}
template<typename T>
std::vector<T> Split(const std::string &str, const std::string &sep=std::string(","))
//-----------------------------------------------------------------------------------
{
	std::vector<T> vals;
	std::size_t pos = 0;
	while(str.find(sep, pos) != std::string::npos)
	{
		vals.push_back(ConvertStrTo<T>(str.substr(pos, str.find(sep, pos) - pos)));
		pos = str.find(sep, pos) + sep.length();
	}
	if(!vals.empty() || (str.substr(pos).length() > 0))
	{
		vals.push_back(ConvertStrTo<T>(str.substr(pos)));
	}
	return vals;
}

} } // namespace mpt::String


// Memset given object to zero.
template <class T>
inline void MemsetZero(T &a)
//--------------------------
{
#if MPT_COMPILER_HAS_TYPE_TRAITS
	static_assert(std::is_pointer<T>::value == false, "Won't memset pointers.");
	static_assert(std::is_pod<T>::value == true, "Won't memset non-pods.");
#endif
	std::memset(&a, 0, sizeof(T));
}


// Copy given object to other location.
template <class T>
inline T &MemCopy(T &destination, const T &source)
//------------------------------------------------
{
#if MPT_COMPILER_HAS_TYPE_TRAITS
	static_assert(std::is_pointer<T>::value == false, "Won't copy pointers.");
	static_assert(std::is_pod<T>::value == true, "Won't copy non-pods.");
#endif
	return *static_cast<T *>(std::memcpy(&destination, &source, sizeof(T)));
}


namespace mpt {


template <typename Tdst, typename Tsrc>
struct byte_cast_impl
{
	inline Tdst operator () (Tsrc src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
#if MPT_COMPILER_HAS_TYPE_TRAITS
		STATIC_ASSERT(std::is_integral<Tsrc>::value);
		STATIC_ASSERT(std::is_integral<Tdst>::value);
#endif
		return static_cast<Tdst>(src);
	}
};
template <typename Tdst, typename Tsrc>
struct byte_cast_impl<Tdst*, Tsrc*>
{
	inline Tdst* operator () (Tsrc* src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
#if MPT_COMPILER_HAS_TYPE_TRAITS
		STATIC_ASSERT(std::is_integral<Tsrc>::value);
		STATIC_ASSERT(std::is_integral<Tdst>::value);
#endif
		return reinterpret_cast<Tdst*>(src);
	}
};
template <typename Tdst>
struct byte_cast_impl<Tdst*, void*>
{
	inline Tdst* operator () (void* src) const
	{
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
#if MPT_COMPILER_HAS_TYPE_TRAITS
		STATIC_ASSERT(std::is_integral<Tdst>::value);
#endif
		return reinterpret_cast<Tdst*>(src);
	}
};
template <typename Tdst>
struct byte_cast_impl<Tdst*, const void*>
{
	inline Tdst* operator () (const void* src) const
	{
		STATIC_ASSERT(sizeof(Tdst) == sizeof(mpt::byte));
#if MPT_COMPILER_HAS_TYPE_TRAITS
		STATIC_ASSERT(std::is_integral<Tdst>::value);
#endif
		return reinterpret_cast<Tdst*>(src);
	}
};
template <typename Tsrc>
struct byte_cast_impl<void*, Tsrc*>
{
	inline void* operator () (Tsrc* src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
#if MPT_COMPILER_HAS_TYPE_TRAITS
		STATIC_ASSERT(std::is_integral<Tsrc>::value);
#endif
		return reinterpret_cast<void*>(src);
	}
};
template <typename Tsrc>
struct byte_cast_impl<const void*, Tsrc*>
{
	inline const void* operator () (Tsrc* src) const
	{
		STATIC_ASSERT(sizeof(Tsrc) == sizeof(mpt::byte));
#if MPT_COMPILER_HAS_TYPE_TRAITS
		STATIC_ASSERT(std::is_integral<Tsrc>::value);
#endif
		return reinterpret_cast<const void*>(src);
	}
};

// casts between different byte (char) types or pointers to these types
template <typename Tdst, typename Tsrc>
inline Tdst byte_cast(Tsrc src)
{
	return byte_cast_impl<Tdst, Tsrc>()(src);
}


// Saturate the value of src to the domain of Tdst
template <typename Tdst, typename Tsrc>
inline Tdst saturate_cast(Tsrc src)
//---------------------------------
{
	// This code tries not only to obviously avoid overflows but also to avoid signed/unsigned comparison warnings and type truncation warnings (which in fact would be safe here) by explicit casting.
	STATIC_ASSERT(std::numeric_limits<Tdst>::is_integer);
	STATIC_ASSERT(std::numeric_limits<Tsrc>::is_integer);
	MPT_CONSTANT_IF(std::numeric_limits<Tdst>::is_signed && std::numeric_limits<Tsrc>::is_signed)
	{
		MPT_CONSTANT_IF(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		}
		return static_cast<Tdst>(std::max<Tsrc>(static_cast<Tsrc>(std::numeric_limits<Tdst>::min()), std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max()))));
	} else MPT_CONSTANT_IF(!std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		MPT_CONSTANT_IF(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		}
		return static_cast<Tdst>(std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
	} else MPT_CONSTANT_IF(std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		MPT_CONSTANT_IF(sizeof(Tdst) > sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		}
		MPT_CONSTANT_IF(sizeof(Tdst) == sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		}
		return static_cast<Tdst>(std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
	} else // Tdst unsigned, Tsrc signed
	{
		MPT_CONSTANT_IF(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::max<Tsrc>(0, src));
		}
		return static_cast<Tdst>(std::max<Tsrc>(0, std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max()))));
	}
}

template <typename Tdst>
inline Tdst saturate_cast(double src)
//-----------------------------------
{
	if(src >= std::numeric_limits<Tdst>::max())
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= std::numeric_limits<Tdst>::min())
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}

template <typename Tdst>
inline Tdst saturate_cast(float src)
//----------------------------------
{
	if(src >= std::numeric_limits<Tdst>::max())
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= std::numeric_limits<Tdst>::min())
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}


} // namespace mpt


#if MPT_MSVC_BEFORE(2010,0) || MPT_GCC_BEFORE(4,4,0)
// Compiler too old.
#ifndef MPT_MINMAX_MACROS
#define MPT_MINMAX_MACROS
#endif
#endif

#if defined(MODPLUG_TRACKER)
// Tracker code requires MIN/MAX to work in constexpr contexts.
// We could make MIN/MAX constexpr for supporting compilers,
// but that would just needlessly complicate the support matrix
// for now.
#ifndef MPT_MINMAX_MACROS
#define MPT_MINMAX_MACROS
#endif
#endif

#if MPT_COMPILER_MSVC
// MSVC disables a bunch of type conversion warnings once a macro is involved.
// Replacing the macro with a template thus spews a TON OF WARNINGS for now.
#ifndef MPT_MINMAX_MACROS
#define MPT_MINMAX_MACROS
#endif
#endif

#if defined(MPT_MINMAX_MACROS)

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#else

namespace mpt { namespace Legacy {

template <typename Ta, typename Tb>
forceinline auto MAX(const Ta &a, const Tb &b) -> decltype((a>b)?a:b)
{
	return (a > b) ? a : b;
}

template <typename Ta, typename Tb>
forceinline auto MIN(const Ta &a, const Tb &b) -> decltype((a<b)?a:b)
{
	return (a < b) ? a : b;
}

} } // namespace mpt::Legacy

using namespace mpt::Legacy;

#endif


namespace Util
{

// Returns true iff Tdst can represent the value val.
// Use as if(Util::TypeCanHoldValue<uint8>(-1)).
template <typename Tdst, typename Tsrc>
inline bool TypeCanHoldValue(Tsrc val)
{
	return (static_cast<Tsrc>(mpt::saturate_cast<Tdst>(val)) == val);
}

// Grows x with an exponential factor suitable for increasing buffer sizes.
// Clamps the result at limit.
// And avoids integer overflows while doing its business.
// The growth factor is 1.5, rounding down, execpt for the initial x==1 case.
template <typename T, typename Tlimit>
inline T ExponentialGrow(const T &x, const Tlimit &limit)
{
	MPT_ASSERT(x > 0);
	MPT_ASSERT(limit > 0);
	if(x == 1)
	{
		return 2;
	}
	T add = std::min<T>(x >> 1, std::numeric_limits<T>::max() - x);
	return std::min<T>(x + add, mpt::saturate_cast<T>(limit));
}
									
template <typename T>
inline T ExponentialGrow(const T &x)
{
	return Util::ExponentialGrow(x, std::numeric_limits<T>::max());
}
									
} //namespace Util


// Limits 'val' to given range. If 'val' is less than 'lowerLimit', 'val' is set to value 'lowerLimit'.
// Similarly if 'val' is greater than 'upperLimit', 'val' is set to value 'upperLimit'.
// If 'lowerLimit' > 'upperLimit', 'val' won't be modified.
template<class T, class C>
inline void Limit(T& val, const C lowerLimit, const C upperLimit)
//---------------------------------------------------------------
{
	if(lowerLimit > upperLimit) return;
	if(val < lowerLimit) val = lowerLimit;
	else if(val > upperLimit) val = upperLimit;
}


// Like Limit, but returns value
template<class T, class C>
inline T Clamp(T val, const C lowerLimit, const C upperLimit)
//-----------------------------------------------------------
{
	if(val < lowerLimit) return lowerLimit;
	else if(val > upperLimit) return upperLimit;
	else return val;
}

// Check if val is in [lo,hi] without causing compiler warnings
// if theses checks are always true due to the domain of T.
// GCC does not warn if the type is templated.
template<typename T, typename C>
inline bool IsInRange(T val, C lo, C hi)
//--------------------------------------
{
	return lo <= val && val <= hi;
}

// Like Limit, but with upperlimit only.
template<class T, class C>
inline void LimitMax(T& val, const C upperLimit)
//----------------------------------------------
{
	if(val > upperLimit)
		val = upperLimit;
}


// Returns sign of a number (-1 for negative numbers, 1 for positive numbers, 0 for 0)
template <class T>
int sgn(T value)
//--------------
{
	return (value > T(0)) - (value < T(0));
}


namespace Util
{

	// Minimum of 3 values
	template <class T> inline const T& Min(const T& a, const T& b, const T& c) {return std::min(std::min(a, b), c);}

	// Returns maximum value of given integer type.
	template <class T> inline T MaxValueOfType(const T&) {static_assert(std::numeric_limits<T>::is_integer == true, "Only integer types are allowed."); return (std::numeric_limits<T>::max)();}

	/// Returns value rounded to nearest integer.
#if (MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2013,0)) || (MPT_COMPILER_GCC && MPT_GCC_BEFORE(4,3,0)) || defined(ANDROID) || MPT_OS_EMSCRIPTEN
	// MSVC before 2013 does not support C99/C++11.
	// GCC before 4.3.0 does not support C++11.
	// Android has std::round just missing even though it should be available.
	// Certain emscripten versions and/or combinations with nodejs (at least the following combination: emscripten 1.34.8, clang 3.7.0, nodejs 0.10.38) fail assert(std::round(1.5)==2.0). The work-around always works.
	inline double Round(const double& val) {if(val >= 0.0) return std::floor(val + 0.5); else return std::ceil(val - 0.5);}
	inline float Round(const float& val) {if(val >= 0.0f) return std::floor(val + 0.5f); else return std::ceil(val - 0.5f);}
#else
	inline double Round(const double& val) { return std::round(val); }
	inline float Round(const float& val) { return std::round(val); }
#endif

	/// Rounds given double value to nearest integer value of type T.
	template <class T> inline T Round(const double& val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		const double valRounded = Round(val);
		MPT_ASSERT(valRounded >= (std::numeric_limits<T>::min)() && valRounded <= (std::numeric_limits<T>::max)());
		const T intval = static_cast<T>(valRounded);
		return intval;
	}

	template <class T> inline T Round(const float& val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		const float valRounded = Round(val);
		MPT_ASSERT(valRounded >= (std::numeric_limits<T>::min)() && valRounded <= (std::numeric_limits<T>::max)());
		const T intval = static_cast<T>(valRounded);
		return intval;
	}

	template<typename T>
	T Weight(T x)
	{
		STATIC_ASSERT(std::numeric_limits<T>::is_integer);
		T c;
		for(c = 0; x; x >>= 1)
		{
			c += x & 1;
		}
		return c;
	}
	
}

namespace Util {

	// Multiply two 32-bit integers, receive 64-bit result.
	// MSVC generates unnecessarily complicated code for the unoptimized variant using _allmul.
	forceinline int64 mul32to64(int32 a, int32 b)
	{
#if MPT_COMPILER_MSVC
		return __emul(a, b);
#else
		return static_cast<int64>(a) * b;
#endif
	}

	forceinline uint64 mul32to64_unsigned(uint32 a, uint32 b)
	{
#if MPT_COMPILER_MSVC
		return __emulu(a, b);
#else
		return static_cast<uint64>(a) * b;
#endif
	}

	forceinline int32 muldiv(int32 a, int32 b, int32 c)
	{
		return static_cast<int32>( mul32to64( a, b ) / c );
	}

	forceinline int32 muldivr(int32 a, int32 b, int32 c)
	{
		return static_cast<int32>( ( mul32to64( a, b ) + ( c / 2 ) ) / c );
	}

	// Do not use overloading because catching unsigned version by accident results in slower X86 code.
	forceinline uint32 muldiv_unsigned(uint32 a, uint32 b, uint32 c)
	{
		return static_cast<uint32>( mul32to64_unsigned( a, b ) / c );
	}
	forceinline uint32 muldivr_unsigned(uint32 a, uint32 b, uint32 c)
	{
		return static_cast<uint32>( ( mul32to64_unsigned( a, b ) + ( c / 2 ) ) / c );
	}

	forceinline int32 muldivrfloor(int64 a, uint32 b, uint32 c)
	{
		a *= b;
		a += c / 2;
		return (a >= 0) ? mpt::saturate_cast<int32>(a / c) : mpt::saturate_cast<int32>((a - (c - 1)) / c);
	}

	// rounds x up to multiples of target
	template <typename T>
	inline T AlignUp(T x, T target)
	{
		return ((x + (target - 1)) / target) * target;
	}

	// rounds x down to multiples of target
	template <typename T>
	inline T AlignDown(T x, T target)
	{
		return (x / target) * target;
	}

	// Insert a range of items [insStart,  insEnd], and possibly shift item fix to the left.
	template<typename T>
	void InsertItem(const T insStart, const T insEnd, T &fix)
	{
		MPT_ASSERT(insEnd >= insStart);
		if(fix >= insStart)
		{
			fix += (insEnd - insStart + 1);
		}
	}

	// Insert a range of items [insStart,  insEnd], and possibly shift items in range [fixStart, fixEnd] to the right.
	template<typename T>
	void InsertRange(const T insStart, const T insEnd, T &fixStart, T &fixEnd)
	{
		MPT_ASSERT(insEnd >= insStart);
		const T insLength = insEnd - insStart + 1;
		if(fixStart >= insEnd)
		{
			fixStart += insLength;
		}
		if(fixEnd >= insEnd)
		{
			fixEnd += insLength;
		}
	}

	// Delete a range of items [delStart,  delEnd], and possibly shift item fix to the left.
	template<typename T>
	void DeleteItem(const T delStart, const T delEnd, T &fix)
	{
		MPT_ASSERT(delEnd >= delStart);
		if(fix > delEnd)
		{
			fix -= (delEnd - delStart + 1);
		}
	}

	// Delete a range of items [delStart,  delEnd], and possibly shift items in range [fixStart, fixEnd] to the left.
	template<typename T>
	void DeleteRange(const T delStart, const T delEnd, T &fixStart, T &fixEnd)
	{
		MPT_ASSERT(delEnd >= delStart);
		const T delLength = delEnd - delStart + 1;
		if(delStart < fixStart  && delEnd < fixStart)
		{
			// cut part is before loop start
			fixStart -= delLength;
			fixEnd -= delLength;
		} else if(delStart < fixStart  && delEnd < fixEnd)
		{
			// cut part is partly before loop start
			fixStart = delStart;
			fixEnd -= delLength;
		} else if(delStart >= fixStart && delEnd < fixEnd)
		{
			// cut part is in the loop
			fixEnd -= delLength;
		} else if(delStart >= fixStart && delStart < fixEnd && delEnd > fixEnd)
		{
			// cut part is partly before loop end
			fixEnd = delStart;
		}
	}

	// Greatest Common Divisor. Always returns non-negative number.
	template <class T>
	T gcd(T a, T b)
	{
		if(a < 0)
			a = -a;
		if(b < 0)
			b = -b;
		for(;;)
		{
			if(a == 0)
				return b;
			b %= a;
			if(b == 0)
				return a;
			a %= b;
		}
	}

	// Least Common Multiple. Always returns non-negative number.
	template <class T>
	int lcm(T a, T b)
	{
		if(a < 0)
			a = -a;
		if(b < 0)
			b = -b;
		if((a | b) == 0)
			return 0;
		return a / gcd<T>(a, b) * b;
	}

	template<typename T, std::size_t n>
	class fixed_size_queue
	{
	private:
		T buffer[n+1];
		std::size_t read_position;
		std::size_t write_position;
	public:
		fixed_size_queue() : read_position(0), write_position(0)
		{
			return;
		}
		void clear()
		{
			read_position = 0;
			write_position = 0;
		}
		std::size_t read_size() const
		{
			if ( write_position > read_position )
			{
				return write_position - read_position;
			} else if ( write_position < read_position )
			{
				return write_position - read_position + n + 1;
			} else
			{
				return 0;
			}
		}
		std::size_t write_size() const
		{
			if ( write_position > read_position )
			{
				return read_position - write_position + n;
			} else if ( write_position < read_position )
			{
				return read_position - write_position - 1;
			} else
			{
				return n;
			}
		}
		bool push( const T & v )
		{
			if ( !write_size() )
			{
				return false;
			}
			buffer[write_position] = v;
			write_position = ( write_position + 1 ) % ( n + 1 );
			return true;
		}
		bool pop() {
			if ( !read_size() )
			{
				return false;
			}
			read_position = ( read_position + 1 ) % ( n + 1 );
			return true;
		}
		T peek() {
			if ( !read_size() )
			{
				return T();
			}
			return buffer[read_position];
		}
		const T * peek_p()
		{
			if ( !read_size() )
			{
				return nullptr;
			}
			return &(buffer[read_position]);
		}
		const T * peek_next_p()
		{
			if ( read_size() < 2 )
			{
				return nullptr;
			}
			return &(buffer[(read_position+1)%(n+1)]);
		}
	};

} // namespace Util


#ifdef MODPLUG_TRACKER

namespace Util
{


} // namespace Util

#endif // MODPLUG_TRACKER

namespace Util
{

mpt::ustring BinToHex(const std::vector<char> &src);
std::vector<char> HexToBin(const mpt::ustring &src);

} // namespace Util


namespace mpt
{

// Wrapper around std::getenv.
// Instead of returning null pointer if the environment variable is not set,
// this wrapper returns the provided default value.
std::string getenv(const std::string &env_var, const std::string &def = std::string());

} // namespace mpt


OPENMPT_NAMESPACE_END
