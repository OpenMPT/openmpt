#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#include <sstream>
#include <string>
#include <limits>
#if _HAS_TR1
	#include <type_traits>
#endif

//Convert object(typically number) to string
template<class T>
inline std::string Stringify(const T& x)
//--------------------------
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

// Sets last character to null in given char array.
// Size of the array must be known at compile time.
template <size_t size>
inline void SetNullTerminator(char (&buffer)[size])
//-------------------------------------------------
{
	STATIC_ASSERT(size > 0);
	buffer[size-1] = 0;
}


// Memset given object to zero.
template <class T>
inline void MemsetZero(T& a)
{
	#if _HAS_TR1
		static_assert(std::tr1::is_pointer<T>::value == false, "Won't memset pointers.");
		static_assert(std::tr1::is_pod<T>::value == true, "Won't memset non-pods.");
	#endif
	memset(&a, 0, sizeof(T));
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


// Convert a 0-terminated string to a space-padded string
template <size_t size>
void NullToSpaceString(char (&buffer)[size])
//------------------------------------------
{
	STATIC_ASSERT(size > 0);
	size_t pos = size;
	while (pos-- > 0)
		if (buffer[pos] == 0)
			buffer[pos] = 32;
	buffer[size - 1] = 0;
}


// Convert a space-padded string to a 0-terminated string
template <size_t size>
void SpaceToNullString(char (&buffer)[size])
//------------------------------------------
{
	STATIC_ASSERT(size > 0);
	// First, remove any Nulls
	NullToSpaceString(buffer);
	size_t pos = size;
	while (pos-- > 0)
	{
		if (buffer[pos] == 32)
			buffer[pos] = 0;
		else if(buffer[pos] != 0)
			break;
	}
	buffer[size - 1] = 0;
}


// Remove any chars after the first null char
template <size_t size>
void FixNullString(char (&buffer)[size])
//--------------------------------------
{
	STATIC_ASSERT(size > 0);
	size_t pos = 0;
	// Find the first null char.
	while(buffer[pos] != '\0' && pos < size)
	{
		pos++;
	}
	// Remove everything after the null char.
	while(pos < size)
	{
		buffer[pos++] = '\0';
	}
}


// Convert a space-padded string to a 0-terminated string. STATIC VERSION! (use this if the maximum string length is known)
// Additional template parameter to specifify the max length of the final string,
// not including null char (useful for e.g. mod loaders)
template <size_t length, size_t size>
void SpaceToNullStringFixed(char (&buffer)[size])
//------------------------------------------------
{
	STATIC_ASSERT(size > 0);
	STATIC_ASSERT(length < size);
	// Remove Nulls in string
	SpaceToNullString(buffer);
	// Overwrite trailing chars
	for(size_t pos = length; pos < size; pos++)
	{
		buffer[pos] = 0;
	}
}


// Convert a space-padded string to a 0-terminated string. DYNAMIC VERSION!
// Additional function parameter to specifify the max length of the final string,
// not including null char (useful for e.g. mod loaders)
template <size_t size>
void SpaceToNullStringFixed(char (&buffer)[size], size_t length)
//--------------------------------------------------------------
{
	STATIC_ASSERT(size > 0);
	ASSERT(length < size);
	// Remove Nulls in string
	SpaceToNullString(buffer);
	// Overwrite trailing chars
	for(size_t pos = length; pos < size; pos++)
	{
		buffer[pos] = 0;
	}
}

namespace Util
{
	// Like std::max, but avoids conflict with max-macro.
	template <class T> inline const T& Max(const T& a, const T& b) {return (std::max)(a, b);}

	// Returns maximum value of given integer type.
	template <class T> inline T MaxValueOfType(const T&) {static_assert(std::numeric_limits<T>::is_integer == true, "Only interger types are allowed."); return (std::numeric_limits<T>::max)();}

	// Returns string containing date and time ended with newline.
	std::basic_string<TCHAR> GetDateTimeStr();
};

#endif
