#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#include <vector>
#include <sstream>
#include <string>
#include <limits>

using namespace std;

#include "typedefs.h"

template<class T, class SIZETYPE>
bool VectorToBinaryStream(ostream& outStrm, const vector<T>& v)
//------------------------------------------------------------
{
	if(!outStrm.good()) return true;
	if((std::numeric_limits<SIZETYPE>::max)() < v.size()) return true;
	
	SIZETYPE s = static_cast<SIZETYPE>(v.size());
	outStrm.write(reinterpret_cast<const char*>(&s), sizeof(s));
	vector<T>::const_iterator iter = v.begin();
	for(iter; iter != v.end(); iter++)
		outStrm.write(reinterpret_cast<const char*>(&(*iter)), sizeof(T));

	if(outStrm.good())
		return false;
	else 
		return true;
}


template<class T, class SIZETYPE>
bool VectorFromBinaryStream(istream& inStrm, vector<T>& v, const SIZETYPE maxSize = (std::numeric_limits<SIZETYPE>::max)())
//---------------------------------------------------------
{
	if(!inStrm.good()) return true;

	SIZETYPE size;
	inStrm.read(reinterpret_cast<char*>(&size), sizeof(size));

	if(size > maxSize)
		return true;

	v.resize(size);
	for(size_t i = 0; i<size; i++)
	{
		inStrm.read(reinterpret_cast<char*>(&v[i]), sizeof(T));
	}
	if(inStrm.good()) 
		return false;
	else
		return true;
}

template<class SIZETYPE>
bool StringToBinaryStream(ostream& outStream, const string& str)
//--------------------------------------------------------------
{
	if(!outStream.good()) return true;
	if((std::numeric_limits<SIZETYPE>::max)() < str.size()) return true;

	SIZETYPE size = static_cast<SIZETYPE>(str.size());

	outStream.write(reinterpret_cast<char*>(&size), sizeof(size));
	outStream << str;
	if(outStream.good())
		return false;
	else 
		return true;
}


template<class SIZETYPE>
bool StringFromBinaryStream(istream& inStrm, string& str, const SIZETYPE maxSize = (std::numeric_limits<SIZETYPE>::max)())
//--------------------------------------------------------------------------------------------
{
	if(!inStrm.good()) return true;

	SIZETYPE strSize;
	inStrm.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
	
	if(strSize > maxSize)
		return true;

	str.resize(strSize);

	//Copying string from stream one character at a time.
	for(SIZETYPE i = 0; i<strSize; i++)
		inStrm.read(&str[i], 1);

	if(inStrm.good()) 
		return false;
	else 
		return true;
}


template<class T>
inline string Stringify(const T& x)
//--------------------------
{
	std::ostringstream o;
	if(!(o << x)) return "STRINGIFY() FAILURE";
	else return o.str();
}


#endif
