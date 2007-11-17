#include "stdafx.h"
#include "sndfile.h"
#include "ordertopatterntable.h"

const WORD COrderToPatternTable::s_Version = 0;

DWORD COrderToPatternTable::UnSerialize(const BYTE* const src, const int memLength)
//-------------------------------------------------------------------------
{
	if(memLength < sizeof(s_Version) + sizeof(size_t)) return 0;
	WORD version = 0;
	uint32 s = 0;
	DWORD memPos = 0;
	memcpy(&version, src, sizeof(version));
	memPos += sizeof(version);
	if(version != 0)
	{
		ASSERT(false);
		return memPos;
	}
	memcpy(&s, src+memPos, sizeof(s));
	memPos += sizeof(s);
	const UINT type = m_rSndFile.m_nType;
	if(type == MOD_TYPE_MPT && s > MPTM_SPECS.ordersMax)
		return true;
	if(type != MOD_TYPE_MPT && s > MAX_ORDERS)
		return true;
	if(static_cast<UINT>(memLength) < memPos+s*sizeof(UINT))
	{
		ASSERT(false);
		return memPos;
	}
	resize(s);
	ASSERT(size() == s);
	for(size_t i = 0; i<s; i++)
	{
		memcpy(&(*this)[i], src+memPos, sizeof(UINT));
		memPos += sizeof(UINT);
	}
	return memPos;
}


UINT COrderToPatternTable::GetSerializationByteNum() const
//----------------------------------
{
	return sizeof(s_Version) + sizeof(size_t) + size()*sizeof(UINT);
}

bool COrderToPatternTable::Serialize(FILE* f) const
//-------------------------------------------------------------------------
{
	//NOTE: If changing this method, see if 
	//GetSerializationByteNum() should be changed as well.

	//Version
	fwrite(&s_Version, sizeof(s_Version), 1, f);
	const uint32 s = size();
	//Size
	fwrite(&s, sizeof(s), 1, f);
	//Values
	for(size_t i = 0; i<s; i++)
	{
		fwrite(&(*this)[i], sizeof(UINT), 1, f);
	}
	return false;
}

size_t COrderToPatternTable::WriteToByteArray(BYTE* dest, const UINT numOfBytes, const UINT destSize)
//-----------------------------------------------------------------------------
{
	if(numOfBytes > destSize) return true;
	if(size() < numOfBytes) resize(numOfBytes, 0xFF);
	UINT i = 0;
	for(i = 0; i<numOfBytes; i++)
		dest[i] = static_cast<BYTE>((*this)[i]);
	ASSERT(i == numOfBytes);
	return i; //Returns the number of bytes written.
}


size_t COrderToPatternTable::WriteAsByte(FILE* f, const UINT count)
//---------------------------------------------------------------
{
	if(size() < count) resize(count, 0xFF);

	size_t i = 0;
	
	for(i = 0; i<count; i++)
	{
		const BYTE temp = static_cast<BYTE>((*this)[i]);
		fwrite(&temp, 1, 1, f);
	}
	ASSERT(i == count);
	return i; //Returns the number of bytes written.
}

bool COrderToPatternTable::ReadAsByte(const BYTE* pFrom, const int howMany, const int memLength)
//-------------------------------------------------------------------------
{
	if(howMany < 0 || howMany > memLength || howMany > MAX_ORDERS) return true;
	
	if(size() < MAX_ORDERS)
	{
		resize(MAX_ORDERS, 0xFF);
		ASSERT(false);
	}
	
	for(int i = 0; i<howMany; i++, pFrom++)
		(*this)[i] = *pFrom;
	return false;
}

UINT COrderToPatternTable::GetOrderNumberLimitMax() const
//-----------------------------------------------------------
{
	return (m_rSndFile.m_nType == MOD_TYPE_MPT) ? MPTM_SPECS.ordersMax : MAX_ORDERS;
}
