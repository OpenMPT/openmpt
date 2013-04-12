/*
 * misc_util.h
 * -----------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <sstream>
#include <string>
#include <limits>
#include <limits.h>
#include "typedefs.h"
#include <io.h> // for _taccess

#if defined(HAS_TYPE_TRAITS)
#include <type_traits>
#endif

//Convert object(typically number) to string
template<class T>
inline std::string Stringify(const T& x)
//--------------------------------------
{
	std::ostringstream o;
	if(!(o << x)) return "FAILURE";
	else return o.str();
}

//Convert string to number.
template<class T>
inline T ConvertStrTo(const char *str)
//------------------------------------
{
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_const<T>::value == false && std::is_volatile<T>::value == false, "Const and volatile types are not handled correctly.");
	#endif
	if(std::numeric_limits<T>::is_integer)
		return static_cast<T>(atoi(str));
	else
		return static_cast<T>(atof(str));
}

template<> inline uint32 ConvertStrTo(const char *str) {return strtoul(str, nullptr, 10);}
template<> inline int64 ConvertStrTo(const char *str) {return _strtoi64(str, nullptr, 10);}
template<> inline uint64 ConvertStrTo(const char *str) {return _strtoui64(str, nullptr, 10);}


// Memset given object to zero.
template <class T>
inline void MemsetZero(T &a)
//--------------------------
{
#ifdef HAS_TYPE_TRAITS
	static_assert(std::is_pointer<T>::value == false, "Won't memset pointers.");
	static_assert(std::is_pod<T>::value == true, "Won't memset non-pods.");
#endif
	memset(&a, 0, sizeof(T));
}


// Copy given object to other location.
template <class T>
inline T &MemCopy(T &destination, const T &source)
//------------------------------------------------
{
#ifdef HAS_TYPE_TRAITS
	static_assert(std::is_pointer<T>::value == false, "Won't copy pointers.");
	static_assert(std::is_pod<T>::value == true, "Won't copy non-pods.");
#endif
	return *static_cast<T *>(memcpy(&destination, &source, sizeof(T)));
}


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


// Like Limit, but with upperlimit only.
template<class T, class C>
inline void LimitMax(T& val, const C upperLimit)
//----------------------------------------------
{
	if(val > upperLimit)
		val = upperLimit;
}


// Like Limit, but returns value
#ifndef CLAMP
#define CLAMP(number, low, high) MIN(high, MAX(low, number))
#endif

 
#ifdef MODPLUG_TRACKER
LPCCH LoadResource(LPCTSTR lpName, LPCTSTR lpType, LPCCH& pData, size_t& nSize, HGLOBAL& hglob);
std::string GetErrorMessage(DWORD nErrorCode);
#endif // MODPLUG_TRACKER


// Sanitize a filename (remove special chars)
template <size_t size>
void SanitizeFilename(char (&buffer)[size])
//-----------------------------------------
{
	STATIC_ASSERT(size > 0);
	for(size_t i = 0; i < size; i++)
	{
		if(	buffer[i] == '\\' ||
			buffer[i] == '\"' ||
			buffer[i] == '/'  ||
			buffer[i] == ':'  ||
			buffer[i] == '?'  ||
			buffer[i] == '<'  ||
			buffer[i] == '>'  ||
			buffer[i] == '*')
		{
			buffer[i] = '_';
		}
	}
}


// Greatest Common Divisor.
template <class T>
T gcd(T a, T b)
//-------------
{
	if(a < 0)
		a = -a;
	if(b < 0)
		b = -b;
	do
	{
		if(a == 0)
			return b;
		b %= a;
		if(b == 0)
			return a;
		a %= b;
	}
}


// Returns sign of a number (-1 for negative numbers, 1 for positive numbers, 0 for 0)
template <class T>
int sgn(T value)
//--------------
{
	return (value > T(0)) - (value < T(0));
}


// Convert a variable-length MIDI integer <value> to a normal integer <result>.
// Function returns how many bytes have been read.
template <class TIn, class TOut>
size_t ConvertMIDI2Int(TOut &result, TIn value)
//---------------------------------------------
{
	return ConvertMIDI2Int(result, (uint8 *)(&value), sizeof(TIn));
}


// Convert a variable-length MIDI integer held in the byte buffer <value> to a normal integer <result>.
// maxLength bytes are read from the byte buffer at max.
// Function returns how many bytes have been read.
// TODO This should report overflow errors if TOut is too small!
template <class TOut>
size_t ConvertMIDI2Int(TOut &result, uint8 *value, size_t maxLength)
//------------------------------------------------------------------
{
	static_assert(std::numeric_limits<TOut>::is_integer == true, "Output type is a not an integer");

	if(maxLength <= 0)
	{
		result = 0;
		return 0;
	}
	size_t bytesUsed = 0;
	result = 0;
	uint8 b;
	do
	{
		b = *value;
		result <<= 7;
		result |= (b & 0x7F);
		value++;
	} while (++bytesUsed < maxLength && (b & 0x80) != 0);
	return bytesUsed;
}


// Convert an integer <value> to a variable-length MIDI integer, held in the byte buffer <result>.
// maxLength bytes are written to the byte buffer at max.
// Function returns how many bytes have been written.
template <class TIn>
size_t ConvertInt2MIDI(uint8 *result, size_t maxLength, TIn value)
//----------------------------------------------------------------
{
	static_assert(std::numeric_limits<TIn>::is_integer == true, "Input type is a not an integer");

	if(maxLength <= 0)
	{
		*result = 0;
		return 0;
	}
	size_t bytesUsed = 0;
	do
	{
		*result = (value & 0x7F);
		value >>= 7;
		if(value != 0)
		{
			*result |= 0x80;
		}
		result++;
	} while (++bytesUsed < maxLength && value != 0)
	return bytesUsed;
}


namespace Util
{
	// Numeric traits provide compile time values for integer min, max limits.
	template <class T> struct NumericTraits {};
	template <> struct NumericTraits<int8> {static const int8 maxValue = int8_max; static const int8 minValue = int8_min;};
	template <> struct NumericTraits<int16> {static const int16 maxValue = int16_max; static const int16 minValue = int16_min;};
	template <> struct NumericTraits<int32> {static const int32 maxValue = int32_max; static const int32 minValue = int32_min;};
	template <> struct NumericTraits<int64> {static const int64 maxValue = int64_max; static const int64 minValue = int64_min;};
	template <> struct NumericTraits<long> {static const long maxValue = LONG_MAX; static const long minValue = LONG_MIN;};

	template <> struct NumericTraits<uint8> {static const uint8 maxValue = uint8_max; static const uint8 minValue = 0;};
	template <> struct NumericTraits<uint16> {static const uint16 maxValue = uint16_max; static const uint16 minValue = 0;};
	template <> struct NumericTraits<uint32> {static const uint32 maxValue = uint32_max; static const uint32 minValue = 0;};
	template <> struct NumericTraits<uint64> {static const uint64 maxValue = uint64_max; static const uint64 minValue = 0;};
	template <> struct NumericTraits<unsigned long> {static const unsigned long maxValue = ULONG_MAX; static const unsigned long minValue = 0;};

	// Minimum of 3 values
	template <class T> inline const T& Min(const T& a, const T& b, const T& c) {return std::min(std::min(a, b), c);}

	// Returns maximum value of given integer type.
	template <class T> inline T MaxValueOfType(const T&) {static_assert(std::numeric_limits<T>::is_integer == true, "Only integer types are allowed."); return (std::numeric_limits<T>::max)();}

	/// Returns value rounded to nearest integer.
	inline double Round(const double& val) {return std::floor(val + 0.5);}
	inline float Round(const float& val) {return std::floor(val + 0.5f);}

	/// Rounds given double value to nearest integer value of type T.
	template <class T> inline T Round(const double& val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		static_assert(sizeof(T) <= 4, "Revise the implementation for integers > 32-bits.");
		const double valRounded = Round(val);
		ASSERT(valRounded >= (std::numeric_limits<T>::min)() && valRounded <= (std::numeric_limits<T>::max)());
		const T intval = static_cast<T>(valRounded);
		return intval;
	}

	template <class T> inline T Round(const float& val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		static_assert(sizeof(T) <= 4, "Revise the implementation for integers > 32-bits.");
		const float valRounded = Round(val);
		ASSERT(valRounded >= (std::numeric_limits<T>::min)() && valRounded <= (std::numeric_limits<T>::max)());
		const T intval = static_cast<T>(valRounded);
		return intval;
	}
	
};

#ifdef MODPLUG_TRACKER
namespace Util { namespace sdTime
{

	time_t MakeGmTime(tm& timeUtc);

}}; // namespace Util::sdTime

namespace Util { namespace sdOs
{
	/// Checks whether file or folder exists and whether it has the given mode.
	enum FileMode {FileModeExists = 0, FileModeRead = 4, FileModeWrite = 2, FileModeReadWrite = 6};
	inline bool IsPathFileAvailable(LPCTSTR pszFilePath, FileMode fm) {return (_taccess(pszFilePath, fm) == 0);}

} } // namespace Util::sdOs
#endif // MODPLUG_TRACKER

namespace Util {

	inline int32 muldiv(int32 a, int32 b, int32 c)
	{
		return static_cast<int32>( ( static_cast<int64>(a) * b ) / c );
	}

	inline int32 muldivr(int32 a, int32 b, int32 c)
	{
		return static_cast<int32>( ( static_cast<int64>(a) * b + ( c / 2 ) ) / c );
	}

	template<typename T, std::size_t n>
	class fixed_size_queue {
	private:
		T buffer[n+1];
		std::size_t read_position;
		std::size_t write_position;
	public:
		fixed_size_queue() : read_position(0), write_position(0) {
			return;
		}
		void clear() {
			read_position = 0;
			write_position = 0;
		}
		std::size_t read_size() const {
			if ( write_position > read_position ) {
				return write_position - read_position;
			} else if ( write_position < read_position ) {
				return write_position - read_position + n + 1;
			} else {
				return 0;
			}
		}
		std::size_t write_size() const {
			if ( write_position > read_position ) {
				return read_position - write_position + n;
			} else if ( write_position < read_position ) {
				return read_position - write_position - 1;
			} else {
				return n;
			}
		}
		bool push( const T & v ) {
			if ( !write_size() ) {
				return false;
			}
			buffer[write_position] = v;
			write_position = ( write_position + 1 ) % ( n + 1 );
			return true;
		}
		bool pop() {
			if ( !read_size() ) {
				return false;
			}
			read_position = ( read_position + 1 ) % ( n + 1 );
			return true;
		}
		T peek() {
			if ( !read_size() ) {
				return T();
			}
			return buffer[read_position];
		}
		const T * peek_p() {
			if ( !read_size() ) {
				return nullptr;
			}
			return &(buffer[read_position]);
		}
		const T * peek_next_p() {
			if ( read_size() < 2 ) {
				return nullptr;
			}
			return &(buffer[(read_position+1)%(n+1)]);
		}
	};

} // namespace Util
