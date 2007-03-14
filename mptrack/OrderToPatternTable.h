#ifndef ORDERTOPATTERNTABLE_H
#define ORDERTOPATTERNTABLE_H

#include <vector>
using std::vector;

class CSoundFile;

//==============================================
class COrderToPatternTable : public vector<UINT>
//==============================================
{
public:
	COrderToPatternTable(const CSoundFile& sndFile) : m_rSndFile(sndFile) {}

	bool ReadAsByte(const BYTE* pFrom, const int howMany, const int memLength);

	UINT GetOrderNumberLimitMax() const;

	size_t WriteAsByte(FILE* f, const UINT count);

	size_t WriteToByteArray(BYTE* dest, const UINT numOfBytes, const UINT destSize);

	bool Serialize(FILE* f) const;

	UINT GetSerializationByteNum() const;
	//To return the number of bytes to write when Serialize
	//is called.
	
	DWORD UnSerialize(const BYTE* const src, const int memLength);
	//Return the number of bytes read.

private:
	const CSoundFile& m_rSndFile;
	static const WORD s_Version;
};



#endif

