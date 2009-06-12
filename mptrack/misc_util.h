#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#include <sstream>
#include <string>

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


#endif
