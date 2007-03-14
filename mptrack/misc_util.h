#ifndef MISC_UTIL_H
#define MISC_UTIL_H

#include <vector>
#include <sstream>
#include <string>

using namespace std;

const UINT STRINGMAXSIZE = 1000;
const UINT VECTORMAXSIZE = 1000;
//Default sizelimits to string/vector load methods.
//Size limits are there to prevent corrupted streams from
//causing e.g. program to try to load of vector of size 2 000 000 000.

template<class T>
bool VectorToBinaryStream(ostream& outStrm, const vector<T>& v)
//------------------------------------------------------------
{
	if(!outStrm.good()) return true;
	size_t s = v.size();
	outStrm.write(reinterpret_cast<const char*>(&s), sizeof(s));
	vector<T>::const_iterator iter = v.begin();
	for(iter; iter != v.end(); iter++)
		outStrm.write(reinterpret_cast<const char*>(&(*iter)), sizeof(T));

	if(outStrm.good())
		return false;
	else 
		return true;
}

template<class T>
bool VectorFromBinaryStream(istream& inStrm, vector<T>& v, const size_t maxSize = VECTORMAXSIZE)
//---------------------------------------------------------
{
	if(!inStrm.good()) return true;
	size_t size;
	inStrm.read(reinterpret_cast<char*>(&size), sizeof(size));

	if(size > maxSize)
		return true;

	v.resize(size);
	ASSERT(v.size() == size);
	for(size_t i = 0; i<size; i++)
	{
		inStrm.read(reinterpret_cast<char*>(&v[i]), sizeof(T));
	}
	if(inStrm.good()) 
		return false;
	else
		return true;
}

bool StringToBinaryStream(ostream& outStream, const string& str);
bool StringFromBinaryStream(istream& inStrm, string& str, const size_t maxSize = STRINGMAXSIZE);


template<class T>
inline string Stringify(const T& x)
//--------------------------
{
	std::ostringstream o;
	if(!(o << x)) return "STRINGIFY() FAILURE";
	else return o.str();
}


#endif
