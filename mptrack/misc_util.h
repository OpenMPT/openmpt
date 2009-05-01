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
{
	STATIC_ASSERT(size > 0);
	buffer[size-1] = 0;
}


#endif
