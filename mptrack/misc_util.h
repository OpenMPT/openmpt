#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#include <sstream>
#include <string>
#include <limits>

#define ARRAYELEMCOUNT(x) (sizeof(x)/sizeof(x[0]))

//Compile time assert. 
#define STATIC_ASSERT(expr) C_ASSERT(expr)

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
inline T ConvertStrTo(const char* str)
//----------------------------------
{
	//Is implemented like this because the commented code below caused link errors.
	if(std::numeric_limits<T>::is_integer)
		return static_cast<T>(atoi(str));
	else
		return static_cast<T>(atof(str));
	
	/*
	std::istringstream i(str);
	T t;
	i >> t;
	return t;
	*/
}

// Sets last character to null in given char array.
// Size of the array must be known at compile time.
template <size_t size>
inline void SetNullTerminator(char (&buffer)[size])
//-------------------------------------------------
{
	STATIC_ASSERT(size > 0);
	buffer[size-1] = 0;
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
{
	utilImpl::ArrayCopyImpl<std::tr1::has_trivial_assign<T>::value>::Do(pDst, pSrc, n);
}

// Sanitize a filename (remove special chars)
template <size_t size>
inline void SanitizeFilename(char (&buffer)[size])
{
	STATIC_ASSERT(size > 0);
	for(int i = 0; i < size; i++)
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
			for(int j = i + 1; j < size; j++)
			{
				buffer[j - 1] = buffer[j];
			}
			buffer[size - 1] = 0;
		}
	}
}

#endif
