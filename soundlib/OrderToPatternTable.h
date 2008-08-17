#ifndef ORDERTOPATTERNTABLE_H
#define ORDERTOPATTERNTABLE_H

#include "../mptrack/serialization_utils.h"
#include <vector>
using std::vector;

class CSoundFile;
class COrderToPatternTable;



#pragma warning(disable:4244) //conversion from 'type1' to 'type2', possible loss of data

class COrderSerialization : public srlztn::ABCSerializationStreamer
//=========================================================
{
public:
	COrderSerialization(COrderToPatternTable& ordertable) : m_rOrders(ordertable) {}
	virtual void ProWrite(srlztn::OUTSTREAM& ostrm) const;
	virtual void ProRead(srlztn::INSTREAM& istrm, const uint64 /*datasize*/);
private:
	COrderToPatternTable& m_rOrders;
};

//==============================================
class COrderToPatternTable : public vector<PATTERNINDEX>
//==============================================
{
public:
	COrderToPatternTable(const CSoundFile& sndFile) : m_rSndFile(sndFile) {}

	bool ReadAsByte(const BYTE* pFrom, const int howMany, const int memLength);

	size_t WriteAsByte(FILE* f, const UINT count);

	size_t WriteToByteArray(BYTE* dest, const UINT numOfBytes, const UINT destSize);

	ORDERINDEX GetCount() const {return size();}

	//Deprecated function used for MPTm's created in 1.17.02.46 - 1.17.02.48.
	DWORD Unserialize(const BYTE* const src, const DWORD memLength);
	
	//Returns true if the IT orderlist datafield is not sufficient to store orderlist information.
	bool NeedsExtraDatafield() const;

	void OnModTypeChanged(const MODTYPE oldtype);

	PATTERNINDEX GetInvalidPatIndex() const; //To correspond 0xFF
	static PATTERNINDEX GetInvalidPatIndex(const MODTYPE type);

	PATTERNINDEX GetIgnoreIndex() const; //To correspond 0xFE
	static PATTERNINDEX GetIgnoreIndex(const MODTYPE type);

	//Returns the previous/next order ignoring skip indeces(+++).
	//If no previous/next order exists, return first/last order, and zero
	//when orderlist is empty.
	ORDERINDEX GetNextOrderIgnoringSkips(const ORDERINDEX start) const;
	ORDERINDEX GetPreviousOrderIgnoringSkips(const ORDERINDEX start) const;

	COrderSerialization* NewReadWriteObject() {return new COrderSerialization(*this);}

private:
	const CSoundFile& m_rSndFile;
};

#pragma warning(default:4244) 

#endif

