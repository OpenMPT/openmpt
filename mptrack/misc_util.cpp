#include "stdafx.h"
#include "misc_util.h"


bool StringToBinaryStream(ostream& outStream, const string& str)
//----------------------------------------------------
{
	if(!outStream.good()) return true;
	size_t size = str.size();
	outStream.write(reinterpret_cast<char*>(&size), sizeof(size));
	outStream << str;
	if(outStream.good())
		return false;
	else 
		return true;

}

bool StringFromBinaryStream(istream& inStrm, string& str, const size_t maxSize)
//---------------------------------------------------------
{
	if(!inStrm.good()) return true;
	size_t strSize;
	inStrm.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
	
	if(strSize > maxSize)
		return true;

	str.resize(strSize);

	//Inefficiently reading to temporary buffer first and
	//then setting that to the string.
	char* buffer = new char[strSize+1];
	inStrm.read(buffer, strSize);
	buffer[strSize] = '\0';
	str = buffer;
	delete[] buffer; buffer = 0;

	
	if(inStrm.good()) 
		return false;
	else 
		return true;
}
