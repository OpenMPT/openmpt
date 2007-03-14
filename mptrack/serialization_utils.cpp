#include "stdafx.h"
#include "serialization_utils.h"
#include "misc_util.h"

bool ReadTuningMap(istream& fin, map<WORD, string>& shortToTNameMap, const size_t maxNum)
//----------------------------------------
{
	typedef map<WORD, string> MAP;
	typedef MAP::iterator MAP_ITER;
	size_t numTuning = 0;
	fin.read(reinterpret_cast<char*>(&numTuning), sizeof(numTuning));
	if(numTuning > maxNum)
	{
		fin.setstate(ios::failbit);
		return true;
	}

	for(size_t i = 0; i<numTuning; i++)
	{
		string temp;
		unsigned short ui;
		if(StringFromBinaryStream(fin, temp))
		{
			fin.setstate(ios::failbit);
			return true;
		}

		fin.read(reinterpret_cast<char*>(&ui), sizeof(ui));
		shortToTNameMap[ui] = temp;
	}
	if(fin.good())
		return false;
	else
		return true;
}


