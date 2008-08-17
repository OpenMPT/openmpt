#include "stdafx.h"
#include "sndfile.h"
#include "ordertopatterntable.h"

DWORD COrderToPatternTable::Unserialize(const BYTE* const src, const DWORD memLength)
//-------------------------------------------------------------------------
{
	if(memLength < 2 + 4) return 0;
	uint16 version = 0;
	uint32 s = 0;
	DWORD memPos = 0;
	memcpy(&version, src, sizeof(version));
	memPos += sizeof(version);
	if(version != 0) return memPos;
    memcpy(&s, src+memPos, sizeof(s));
	memPos += sizeof(s);
	if(s > 65000) return true;
	if(memLength < memPos+s*4) return memPos;

	resize(s);
	for(size_t i = 0; i<s; i++, memPos +=4 )
	{
		uint32 temp;
		memcpy(&temp, src+memPos, 4);
		(*this)[i] = static_cast<PATTERNINDEX>(temp);
	}
	return memPos;
}


size_t COrderToPatternTable::WriteToByteArray(BYTE* dest, const UINT numOfBytes, const UINT destSize)
//-----------------------------------------------------------------------------
{
	if(numOfBytes > destSize) return true;
	if(size() < numOfBytes) resize(numOfBytes, 0xFF);
	UINT i = 0;
	for(i = 0; i<numOfBytes; i++)
	{
		dest[i] = static_cast<BYTE>((*this)[i]);
	}
	return i; //Returns the number of bytes written.
}


size_t COrderToPatternTable::WriteAsByte(FILE* f, const UINT count)
//---------------------------------------------------------------
{
	if(size() < count) resize(count, GetInvalidPatIndex());

	size_t i = 0;
	
	for(i = 0; i<count; i++)
	{
		const PATTERNINDEX pat = (*this)[i];
		BYTE temp = static_cast<BYTE>((*this)[i]);

		if(pat > 0xFD)
		{
			if(pat == GetInvalidPatIndex()) temp = 0xFF;
			else temp = 0xFE;
		}
		fwrite(&temp, 1, 1, f);
	}
	return i; //Returns the number of bytes written.
}

bool COrderToPatternTable::ReadAsByte(const BYTE* pFrom, const int howMany, const int memLength)
//-------------------------------------------------------------------------
{
	if(howMany < 0 || howMany > memLength) return true;
	if(m_rSndFile.GetType() != MOD_TYPE_MPT && howMany > MAX_ORDERS) return true;
	
	if(size() < static_cast<size_t>(howMany))
		resize(howMany, GetInvalidPatIndex());
	
	for(int i = 0; i<howMany; i++, pFrom++)
		(*this)[i] = *pFrom;
	return false;
}


bool COrderToPatternTable::NeedsExtraDatafield() const
//----------------------------------------------
{
	if(m_rSndFile.GetType() == MOD_TYPE_MPT && m_rSndFile.Patterns.Size() > 0xFD)
		return true;
	else
		return false;
}


void COrderToPatternTable::OnModTypeChanged(const MODTYPE oldtype)
//----------------------------------------------------------------
{
	const CModSpecifications specs = m_rSndFile.GetModSpecifications();

	//Resize orderlist if needed. Because old orderlist had MAX_ORDERS(256) elements, not making it
	//smaller than that even if the modtype doesn't support that many orders.
	if(specs.ordersMax < GetCount()) 
	{
		resize(max(MAX_ORDERS, specs.ordersMax));
		for(ORDERINDEX i = GetCount(); i>specs.ordersMax; --i) (*this)[i-1] = GetInvalidPatIndex();
	}

	//Replace items used to denote end of song/skip order.
	replace(begin(), end(), GetInvalidPatIndex(oldtype), GetInvalidPatIndex());
	replace(begin(), end(), GetIgnoreIndex(oldtype), GetIgnoreIndex());
}


ORDERINDEX COrderToPatternTable::GetNextOrderIgnoringSkips(const ORDERINDEX start) const
//-------------------------------------------------------------------------------------
{
	const ORDERINDEX count = GetCount();
	if(count == 0) return 0;
	ORDERINDEX next = min(count-1, start+1);
	while(next+1 < count && (*this)[next] == GetIgnoreIndex()) next++;
	return next;
}

ORDERINDEX COrderToPatternTable::GetPreviousOrderIgnoringSkips(const ORDERINDEX start) const
//-------------------------------------------------------------------------------------
{
	const ORDERINDEX count = GetCount();
	if(start == 0 || count == 0) return 0;
	ORDERINDEX prev = min(start-1, count-1);
	while(prev > 0 && (*this)[prev] == GetIgnoreIndex()) prev--;
	return prev;
}


PATTERNINDEX COrderToPatternTable::GetInvalidPatIndex(const MODTYPE type) {return type == MOD_TYPE_MPT ?  65535 : 0xFF;}
PATTERNINDEX COrderToPatternTable::GetIgnoreIndex(const MODTYPE type) {return type == MOD_TYPE_MPT ? 65534 : 0xFE;}

PATTERNINDEX COrderToPatternTable::GetInvalidPatIndex() const {return GetInvalidPatIndex(m_rSndFile.GetType());}
PATTERNINDEX COrderToPatternTable::GetIgnoreIndex() const {return GetIgnoreIndex(m_rSndFile.GetType());}




//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

using namespace srlztn;

void COrderSerialization::ProWrite(OUTSTREAM& ostrm) const
//--------------------------------------------------------
{	
    uint16 size = static_cast<uint16>(m_rOrders.size());
	ostrm.write(reinterpret_cast<const char*>(&size), 2);
	const COrderToPatternTable::const_iterator endIter = m_rOrders.end();
	for(COrderToPatternTable::const_iterator citer = m_rOrders.begin(); citer != endIter; citer++)
	{
		const uint16 temp = static_cast<uint16>(*citer);
		ostrm.write(reinterpret_cast<const char*>(&temp), 2);
	}
}


void COrderSerialization::ProRead(INSTREAM& istrm, const uint64 /*datasize*/)
//---------------------------------------------------------------------------
{
	uint16 size;
	istrm.read(reinterpret_cast<char*>(&size), 2);
	if(size > MPTM_SPECS.ordersMax) size = MPTM_SPECS.ordersMax;
	m_rOrders.resize(size);
	if(size == 0) {m_rOrders.assign(MAX_ORDERS, m_rOrders.GetInvalidPatIndex()); return;}

	const COrderToPatternTable::const_iterator endIter = m_rOrders.end();
	for(COrderToPatternTable::iterator iter = m_rOrders.begin(); iter != endIter; iter++)
	{
		uint16 temp;
		istrm.read(reinterpret_cast<char*>(&temp), 2);
		*iter = temp;
	}
}



