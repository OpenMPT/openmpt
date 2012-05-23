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
#include "typedefs.h"
#if _HAS_TR1
	#include <type_traits>
#endif
#include <io.h> // for _taccess

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
inline T ConvertStrTo(LPCSTR psz)
//-------------------------------
{
	#if _HAS_TR1
		static_assert(std::tr1::is_const<T>::value == false && std::tr1::is_volatile<T>::value == false, "Const and volatile types are not handled correctly.");
	#endif
	if(std::numeric_limits<T>::is_integer)
		return static_cast<T>(atoi(psz));
	else
		return static_cast<T>(atof(psz));
}

template<> inline uint32 ConvertStrTo(LPCSTR psz) {return strtoul(psz, nullptr, 10);}
template<> inline int64 ConvertStrTo(LPCSTR psz) {return _strtoi64(psz, nullptr, 10);}
template<> inline uint64 ConvertStrTo(LPCSTR psz) {return _strtoui64(psz, nullptr, 10);}


// Memset given object to zero.
template <class T>
inline void MemsetZero(T &a)
//--------------------------
{
#if _HAS_TR1
	static_assert(std::tr1::is_pointer<T>::value == false, "Won't memset pointers.");
	static_assert(std::tr1::is_pod<T>::value == true, "Won't memset non-pods.");
#endif
	memset(&a, 0, sizeof(T));
}


// Copy given object to other location.
template <class T>
inline T &MemCopy(T &destination, const T &source)
//------------------------------------------------
{
#if _HAS_TR1
	static_assert(std::tr1::is_pointer<T>::value == false, "Won't copy pointers.");
	static_assert(std::tr1::is_pod<T>::value == true, "Won't copy non-pods.");
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
#define CLAMP(number, low, high) min(high, max(low, number))
#endif

 
LPCCH LoadResource(LPCTSTR lpName, LPCTSTR lpType, LPCCH& pData, size_t& nSize, HGLOBAL& hglob);
CString GetErrorMessage(DWORD nErrorCode);

namespace utilImpl
{
	template <bool bMemcpy>
	struct ArrayCopyImpl {};

	template <>
	struct ArrayCopyImpl<true>
	{
		template <class T>
		static void Do(T* pDst, const T* pSrc, const size_t n) {memcpy(pDst, pSrc, sizeof(T) * n);}
	};

	template <>
	struct ArrayCopyImpl<false>
	{
		template <class T>
		static void Do(T* pDst, const T* pSrc, const size_t n) {std::copy(pSrc, pSrc + n, pDst);}
	};
} // namespace utilImpl


// Copies n elements from array pSrc to array pDst.
// If the source and destination arrays overlap, behaviour is undefined.
template <class T>
void ArrayCopy(T* pDst, const T* pSrc, const size_t n)
//----------------------------------------------------
{
	utilImpl::ArrayCopyImpl<std::tr1::has_trivial_assign<T>::value>::Do(pDst, pSrc, n);
}


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
			for(size_t j = i + 1; j < size; j++)
			{
				buffer[j - 1] = buffer[j];
			}
			buffer[size - 1] = 0;
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
	// Like std::max, but avoids conflict with max-macro.
	template <class T> inline const T& Max(const T& a, const T& b) {return (std::max)(a, b);}

	// Like std::min, but avoids conflict with min-macro.
	template <class T> inline const T& Min(const T& a, const T& b) {return (std::min)(a, b);}

	// Minimum of 3 values
	template <class T> inline const T& Min(const T& a, const T& b, const T& c) {return Min(Min(a, b), c);}

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

namespace Util { namespace sdTime
{
	// Returns string containing date and time ended with newline.
	std::basic_string<TCHAR> GetDateTimeStr();

	time_t MakeGmTime(tm& timeUtc);

}}; // namespace Util::sdTime

namespace Util { namespace sdOs
{
	/// Checks whether file or folder exists and whether it has the given mode.
	enum FileMode {FileModeExists = 0, FileModeRead = 4, FileModeWrite = 2, FileModeReadWrite = 6};
	inline bool IsPathFileAvailable(LPCTSTR pszFilePath, FileMode fm) {return (_taccess(pszFilePath, fm) == 0);}

} } // namespace Util::sdOs
