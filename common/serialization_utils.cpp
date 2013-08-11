/*
 * serialization_utils.cpp
 * -----------------------
 * Purpose: Serializing data to and from MPTM files.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "serialization_utils.h"
#include "misc_util.h"
#include <algorithm>
#include <iterator> // for back_inserter

namespace srlztn
{

static const uint8 HeaderId_FlagByte = 0;

// Indexing starts from 0.
inline bool Testbit(uint8 val, uint8 bitindex) {return ((val & (1 << bitindex)) != 0);}

inline void Setbit(uint8& val, uint8 bitindex, bool newval)
//----------------------------------------------------------
{
	if(newval) val |= (1 << bitindex);
	else val &= ~(1 << bitindex);
}


bool IsPrintableId(const char* pId, const size_t nLength)
//--------------------------------------------------------
{
	for(size_t i = 0; i < nLength; i++)
	{
		if (pId[i] <= 0 || isprint(pId[i]) == 0)
			return false;
	}
	return true;
}


uint8 GetByteReq1248(const uint64 size)
//-------------------------------------
{
	if((size >> 6) == 0) return 1;
	if((size >> (1*8+6)) == 0) return 2;
	if((size >> (3*8+6)) == 0) return 4;
	return 8;
}


uint8 GetByteReq1234(const uint32 num)
//------------------------------------
{
	if((num >> 6) == 0) return 1;
	if((num >> (1*8+6)) == 0) return 2;
	if((num >> (2*8+6)) == 0) return 3;
	return 4;
}


void WriteAdaptive12(std::ostream& oStrm, const uint16 num)
//------------------------------------------------------
{
	if(num >> 7 == 0)
		Binarywrite<uint16>(oStrm, num << 1, 1);
	else
		Binarywrite<uint16>(oStrm, (num << 1) | 1);
}


void WriteAdaptive1234(std::ostream& oStrm, const uint32 num)
//--------------------------------------------------------
{
	const uint8 bc = GetByteReq1234(num);
	const uint32 sizeInstruction = (num << 2) | (bc - 1);
	Binarywrite<uint32>(oStrm, sizeInstruction, bc);
}


//Format: First bit tells whether the size indicator is 1 or 2 bytes.
void WriteAdaptive12String(std::ostream& oStrm, const std::string& str)
//------------------------------------------------------------------
{
	uint16 s = static_cast<uint16>(str.size());
	LimitMax(s, uint16(uint16_max / 2));
	WriteAdaptive12(oStrm, s);
	oStrm.write(str.c_str(), s);
}


// Works only for arguments 1,2,4,8
uint8 Log2(const uint8& val)
//--------------------------
{
	if(val == 1) return 0;
	else if(val == 2) return 1;
	else if(val == 4) return 2;
	else return 3;
}

void WriteAdaptive1248(std::ostream& oStrm, const uint64& num)
//---------------------------------------------------------
{
	const uint8 bc = GetByteReq1248(num);
	const uint64 sizeInstruction = (num << 2) | Log2(bc);
	Binarywrite<uint64>(oStrm, sizeInstruction, bc);
}


void ReadAdaptive12(std::istream& iStrm, uint16& val)
//-----------------------------------------------
{
	Binaryread<uint16>(iStrm, val, 1);
	if(val & 1)
	{
		uint8 hi = 0;
		Binaryread(iStrm, hi);
		val &= 0xff;
		val |= hi << 8;
	}
	val >>= 1;
}


void ReadAdaptive1234(std::istream& iStrm, uint32& val)
//-------------------------------------------------
{
	Binaryread<uint32>(iStrm, val, 1);
	const uint8 bc = 1 + static_cast<uint8>(val & 3);
	uint8 v2 = 0;
	uint8 v3 = 0;
	uint8 v4 = 0;
	if(bc >= 2) Binaryread(iStrm, v2);
	if(bc >= 3) Binaryread(iStrm, v3);
	if(bc >= 4) Binaryread(iStrm, v4);
	val &= 0xff;
	val |= (v2 << 8) | (v3 << 16) | (v4 << 24);
	val >>= 2;
}

const uint8 pow2xTable[] = {1, 2, 4, 8, 16, 32, 64, 128};

// Returns 2^n. n must be within {0,...,7}.
inline uint8 Pow2xSmall(const uint8& exp) {ASSERT(exp <= 7); return pow2xTable[exp];}

void ReadAdaptive1248(std::istream& iStrm, uint64& val)
//-------------------------------------------------
{
	Binaryread<uint64>(iStrm, val, 1);
	uint8 bc = Pow2xSmall(static_cast<uint8>(val & 3));
	int byte = 1;
	val &= 0xff;
	while(bc > 1)
	{
		uint8 v = 0;
		Binaryread(iStrm, v);
		if(byte < 8)
		{
			val |= uint64(v) << (byte*8);
		}
		byte++;
		bc--;
	}
	val >>= 2;
}


void WriteItemString(std::ostream& oStrm, const char* const pStr, const size_t nSize)
//--------------------------------------------------------------------------------
{
	uint32 id = (uint32)std::min<size_t>(nSize, (uint32_max >> 4)) << 4;
	id |= 12; // 12 == 1100b
	Binarywrite<uint32>(oStrm, id);
	id >>= 4;
	if(id > 0)
		oStrm.write(pStr, id);
}


void ReadItemString(std::istream& iStrm, std::string& str, const DataSize)
//--------------------------------------------------------------------
{
	// bits 0,1: Bytes per char type: 1,2,3,4.
	// bits 2,3: Bytes in size indicator, 1,2,3,4
	uint32 id = 0;
	Binaryread(iStrm, id, 1);
	const uint8 nSizeBytes = (id & 12) >> 2; // 12 == 1100b
	if (nSizeBytes > 0)
	{
		uint8 bytes = MIN(3, nSizeBytes);
		uint8 v2 = 0;
		uint8 v3 = 0;
		uint8 v4 = 0;
		if(bytes >= 1) Binaryread(iStrm, v2);
		if(bytes >= 2) Binaryread(iStrm, v3);
		if(bytes >= 3) Binaryread(iStrm, v4);
		id &= 0xff;
		id |= (v2 << 8) | (v3 << 16) | (v4 << 24);
	}
	// Limit to 1 MB.
	str.resize(MIN(id >> 4, 1000000));
	for(size_t i = 0; i < str.size(); i++)
		iStrm.read(&str[i], 1);

	id = (id >> 4) - (uint32)str.size();
	if(id > 0)
		iStrm.ignore(id);
}


std::string IdToString(const char* const pvId, const size_t nLength)
//-------------------------------------------------------------
{
	const char* pId = static_cast<const char*>(pvId);
	if (nLength == 0)
		return "";
	std::string str;
	if (IsPrintableId(pId, nLength))
		std::copy(pId, pId + nLength, std::back_inserter<std::string>(str));
	else if (nLength <= 4) // Interpret ID as integer value.
	{
		int32 val = 0;
		memcpy(&val, pId, nLength);
		str = Stringify(val);
	}
	return str;
}

const char Ssb::s_EntryID[3] = {'2','2','8'};
int32 Ssb::s_DefaultReadLogMask = SNT_DEFAULT_MASK;
int32 Ssb::s_DefaultWriteLogMask = SNT_DEFAULT_MASK;
Ssb::fpLogFunc_t Ssb::s_DefaultLogFunc = nullptr;

const TCHAR tstrWriteHeader[] = MPT_TEXT("Write header with ID = %s\n");
const TCHAR tstrWriteProgress[] = MPT_TEXT("Wrote entry: {num, id, rpos, size} = {%u, %s, %u, %u}\n");
const TCHAR tstrWritingMap[] = MPT_TEXT("Writing map to rpos: %u\n");
const TCHAR tstrMapEntryWrite[] = MPT_TEXT("Writing map entry: id=%s, rpos=%u, size=%u\n");
const TCHAR strWriteNote[] = MPT_TEXT("Write note: ");
const TCHAR tstrEndOfStream[] = MPT_TEXT("End of stream(rpos): %u\n");

const TCHAR tstrReadingHeader[] = MPT_TEXT("Read header with expected ID = %s\n");
const TCHAR strNoMapInFile[] = MPT_TEXT("No map in the file.\n");
const TCHAR strIdMismatch[] = MPT_TEXT("ID mismatch, terminating read.\n");
const TCHAR strIdMatch[] = MPT_TEXT("ID match, continuing reading.\n");
const TCHAR tstrReadingMap[] = MPT_TEXT("Reading map from rpos: %u\n");
const TCHAR tstrEndOfMap[] = MPT_TEXT("End of map(rpos): %u\n");
const TCHAR tstrReadProgress[] = MPT_TEXT("Read entry: {num, id, rpos, size, desc} = {%u, %s, %u, %s, %s}\n");
const TCHAR tstrNoEntryFound[] = MPT_TEXT("No entry with id %s found.\n");
const TCHAR tstrCantFindSubEntry[] = MPT_TEXT("Unable to find subentry with id=%s\n");
const TCHAR strReadNote[] = MPT_TEXT("Read note: ");


#define SSB_INITIALIZATION_LIST					\
	m_Status(SNT_NONE), \
	m_nFixedEntrySize(0),						\
	m_fpLogFunc(s_DefaultLogFunc),				\
	m_Readlogmask(s_DefaultReadLogMask),		\
	m_Writelogmask(s_DefaultWriteLogMask),		\
	m_posStart(0),								\
	m_nReadVersion(0),							\
	m_nMaxReadEntryCount(16000),				\
	m_rposMapBegin(0),							\
	m_posMapEnd(0),								\
	m_posDataBegin(0),							\
	m_rposEndofHdrData(0),						\
	m_nReadEntrycount(0),						\
	m_nIdbytes(IdSizeVariable),					\
	m_nCounter(0),								\
	m_nNextReadHint(0),							\
	m_Flags(s_DefaultFlags),					\
	m_pSubEntry(nullptr),						\
	m_posSubEntryStart(0),						\
	m_nMapReserveSize(0),						\
	m_posEntrycount(0),							\
	m_posMapPosField(0),						\
	m_posMapStart(0)							\


Ssb::Ssb(std::istream* pIstrm, std::ostream* pOstrm) :
		m_pOstrm(pOstrm),
		m_pIstrm(pIstrm),
		SSB_INITIALIZATION_LIST
//-----------------------------------------------
{}

Ssb::Ssb(std::iostream& ioStrm) :
		m_pOstrm(&ioStrm),
		m_pIstrm(&ioStrm),
		SSB_INITIALIZATION_LIST
//------------------------------
{}

Ssb::Ssb(std::ostream& oStrm) :
		m_pOstrm(&oStrm),
		m_pIstrm(nullptr),
		SSB_INITIALIZATION_LIST
//------------------------------
{}


Ssb::Ssb(std::istream& iStrm) :
		m_pOstrm(nullptr),
		m_pIstrm(&iStrm),
		SSB_INITIALIZATION_LIST
//------------------------------
{}

#undef SSB_INITIALIZATION_LIST

void Ssb::AddNote(const SsbStatus s, const SsbStatus mask, const TCHAR* sz)
//-------------------------------------------------------------------------
{
	m_Status |= s;
	if ((s & mask) != 0 && m_fpLogFunc)
		m_fpLogFunc("%s: 0x%x\n", sz, s);
}

void Ssb::AddWriteNote(const SsbStatus s) {AddNote(s, m_Writelogmask, strWriteNote);}
void Ssb::AddReadNote(const SsbStatus s) {AddNote(s, m_Readlogmask, strReadNote);}


void Ssb::AddReadNote(const ReadEntry* const pRe, const NumType nNum)
//-------------------------------------------------------------------
{
	m_Status |= SNT_PROGRESS;

	if ((m_Readlogmask & SNT_PROGRESS) != 0 && m_fpLogFunc)
	{
		m_fpLogFunc(
				 tstrReadProgress,
				 nNum,
				 (pRe && pRe->nIdLength < 30 && m_Idarray.size() > 0) ?  IdToString(&m_Idarray[pRe->nIdpos], pRe->nIdLength).c_str() : "",
				 (pRe) ? pRe->rposStart : 0,
				 (pRe && pRe->nSize != invalidDatasize) ? Stringify(pRe->nSize).c_str() : "",
				 "");
	}	
}


// Called after writing an entry.
void Ssb::AddWriteNote(const char* pId, const size_t nIdSize, const NumType nEntryNum, const DataSize nBytecount, const RposType rposStart)
//----------------------------------------------------------------------------
{
	m_Status |= SNT_PROGRESS;
	if ((m_Writelogmask & SNT_PROGRESS) != 0 && m_fpLogFunc)
	{
		if (nIdSize < 30)
		{
			m_fpLogFunc(tstrWriteProgress, nEntryNum, IdToString(pId, nIdSize).c_str(), rposStart, nBytecount);
		}
	}
}


void Ssb::ResetReadstatus()
//-------------------------
{
	m_Status = SNT_NONE;
	m_Idarray.reserve(32);
	m_Idarray.push_back(0);
}


void Ssb::WriteMapItem( const char* pId, 
						const size_t nIdSize,
						const RposType& rposDataStart,
						const DataSize& nDatasize,
						const TCHAR* pszDesc)
//----------------------------------------------
{
	if (m_fpLogFunc)
		m_fpLogFunc(tstrMapEntryWrite,
					(nIdSize > 0) ? IdToString(pId, nIdSize).c_str() : "",
					rposDataStart,
					nDatasize);

	if(m_nIdbytes > 0)
	{
		if (m_nIdbytes != IdSizeVariable && nIdSize != m_nIdbytes)
			{ AddWriteNote(SNW_CHANGING_IDSIZE_WITH_FIXED_IDSIZESETTING); return; }

		if (m_nIdbytes == IdSizeVariable) //Variablesize ID?
			WriteAdaptive12(m_MapStream, static_cast<uint16>(nIdSize));

		if(nIdSize > 0)
			m_MapStream.write(pId, nIdSize);
	}

	if (GetFlag(RwfWMapStartPosEntry)) //Startpos
		WriteAdaptive1248(m_MapStream, rposDataStart);
	if (GetFlag(RwfWMapSizeEntry)) //Entrysize
		WriteAdaptive1248(m_MapStream, nDatasize);
	if (GetFlag(RwfWMapDescEntry)) //Entry descriptions
		WriteAdaptive12String(m_MapStream, std::string(pszDesc));
}


void Ssb::ReserveMapSize(uint32 nSize)
//------------------------------------
{
	std::ostream& oStrm = *m_pOstrm;
	m_nMapReserveSize = nSize;
	if (nSize > 0)
	{
		m_posMapStart = oStrm.tellp();
		for(size_t i = 0; i < m_nMapReserveSize; i++)
			oStrm.put(0);
	}
}


void Ssb::SetIdSize(uint16 nSize)
//-------------------------------
{
	if (nSize == IdSizeVariable || nSize > IdSizeMaxFixedSize)
		m_nIdbytes = IdSizeVariable;
	else
		m_nIdbytes = nSize;
}


void Ssb::CreateWriteSubEntry()
//-----------------------------
{
	m_posSubEntryStart = m_pOstrm->tellp();
	delete m_pSubEntry;
	m_pSubEntry = new Ssb(*m_pOstrm);
	m_pSubEntry->m_fpLogFunc = m_fpLogFunc;
}


Ssb* Ssb::CreateReadSubEntry(const char* pId, const size_t nLength)
//-----------------------------------------------------------------
{
	const ReadEntry* pE = Find(pId, nLength);
	if (pE && pE->rposStart != 0)
	{
		m_nCounter++;
		delete m_pSubEntry;
		m_pSubEntry = new Ssb(*m_pIstrm);
		m_pSubEntry->m_fpLogFunc = m_fpLogFunc;
		m_pIstrm->seekg(m_posStart + Postype(pE->rposStart));
		return m_pSubEntry;
	}
	else if (m_fpLogFunc)
		m_fpLogFunc(tstrCantFindSubEntry, IdToString(pId, nLength).c_str());

	return nullptr;
}


void Ssb::IncrementWriteCounter()
//-------------------------------
{
	m_nCounter++;
	if (m_nCounter >= (uint16_max >> 2))
	{
		FinishWrite();
		AddWriteNote(SNW_MAX_WRITE_COUNT_REACHED);
	}
}


void Ssb::ReleaseWriteSubEntry(const char* pId, const size_t nIdLength)
//---------------------------------------------------------------------
{
	if ((m_pSubEntry->m_Status & SNT_FAILURE) != 0)
		m_Status |= SNW_SUBENTRY_FAILURE;

	delete m_pSubEntry; m_pSubEntry = nullptr;
	WriteMapItem(pId, nIdLength, static_cast<RposType>(m_posSubEntryStart - m_posStart), static_cast<DataSize>(m_pOstrm->tellp() - m_posSubEntryStart), "");
	IncrementWriteCounter();
}


void Ssb::BeginWrite(const char* pId, const size_t nIdSize, const uint64& nVersion)
//---------------------------------------------------------------------------------
{
	std::ostream& oStrm = *m_pOstrm;

	if (m_fpLogFunc)
		m_fpLogFunc(tstrWriteHeader, IdToString(pId, nIdSize).c_str());

	ResetWritestatus();

	if(!oStrm.good())
		{ AddWriteNote(SNRW_BADGIVEN_STREAM); return; }

	// Start bytes.
	oStrm.write(s_EntryID, sizeof(s_EntryID));

	m_posStart = oStrm.tellp() - Offtype(sizeof(s_EntryID));

	// Object ID.
	{
		uint8 idsize = static_cast<uint8>(nIdSize);
		Binarywrite<uint8>(oStrm, idsize);
		if(idsize > 0) oStrm.write(pId, nIdSize);
	}

	// Form header.
	uint8 header = 0;

	SetFlag(RwfWMapStartPosEntry, GetFlag(RwfWMapStartPosEntry) && m_nFixedEntrySize == 0);
	SetFlag(RwfWMapSizeEntry, GetFlag(RwfWMapSizeEntry) && m_nFixedEntrySize == 0);

	header = (m_nIdbytes != 4) ? (m_nIdbytes & 3) : 3;	//0,1 : Bytes per IDtype, 0,1,2,4
	Setbit(header, 2, GetFlag(RwfWMapStartPosEntry));	//2   : Startpos in map?
	Setbit(header, 3, GetFlag(RwfWMapSizeEntry));		//3   : Datasize in map?
	Setbit(header, 4, GetFlag(RwfWVersionNum));			//4   : Version numeric field?
	Setbit(header, 7, GetFlag(RwfWMapDescEntry));		//7   : Entrydescriptions in map?

	// Write header
	Binarywrite<uint8>(oStrm, header);

	// Additional options.
	uint8 tempU8 = 0;
	Setbit(tempU8, 0, (m_nIdbytes == IdSizeVariable) || (m_nIdbytes == 3) || (m_nIdbytes > 4));
	Setbit(tempU8, 1, m_nFixedEntrySize != 0);
	
	const uint8 flags = tempU8;
	if(flags != s_DefaultFlagbyte)
	{
		WriteAdaptive1234(oStrm, 2); //Headersize - now it is 2.
        Binarywrite<uint8>(oStrm, HeaderId_FlagByte);
		Binarywrite<uint8>(oStrm, flags);
	}
	else
		WriteAdaptive1234(oStrm, 0);

	if(Testbit(header, 4)) // Version(numeric)?
		WriteAdaptive1248(oStrm, nVersion);

	if(Testbit(flags, 0)) // Custom IDbytecount?
	{
		uint8 n = (m_nIdbytes == IdSizeVariable) ? 1 : static_cast<uint8>((m_nIdbytes << 1));
		Binarywrite<uint8>(oStrm, n);
	}

	if(Testbit(flags, 1)) // Fixedsize entries?
		WriteAdaptive1234(oStrm, m_nFixedEntrySize);

	//Entrycount. Reserve two bytes(max uint16_max / 4 entries), actual value is written after writing data.
	m_posEntrycount = oStrm.tellp();
	Binarywrite<uint16>(oStrm, 0);

	SetFlag(RwfRwHasMap, (m_nIdbytes != 0 || GetFlag(RwfWMapStartPosEntry) || GetFlag(RwfWMapSizeEntry) || GetFlag(RwfWMapDescEntry)));

	m_posMapPosField = oStrm.tellp();
	if (GetFlag(RwfRwHasMap)) //Mapping begin pos(reserve space - actual value is written after writing data)
		Binarywrite<uint64>(oStrm, 0);
}


Ssb::ReadRv Ssb::OnReadEntry(const ReadEntry* pE, const char* pId, const size_t nIdSize, const Postype& posReadBegin)
//-------------------------------------------------------------------------------------------------------------------
{
	if (pE != nullptr)
		AddReadNote(pE, m_nCounter);
	else if (GetFlag(RwfRMapHasId) == false) // Not ID's in map.
	{
		ReadEntry e;
		e.rposStart = static_cast<RposType>(posReadBegin - m_posStart);
		e.nSize = static_cast<DataSize>(m_pIstrm->tellg() - posReadBegin);
		AddReadNote(&e, m_nCounter);
	}
	else // Entry not found.
	{
		if (m_fpLogFunc)
			m_fpLogFunc(tstrNoEntryFound, IdToString(pId, nIdSize).c_str());
		return EntryNotFound;
	}
	m_nCounter++;
	return EntryRead;
}


void Ssb::OnWroteItem(const char* pId, const size_t nIdSize, const Postype& posBeforeWrite)
//-----------------------------------------------------------------------------------------
{
	const Offtype nRawEntrySize = m_pOstrm->tellp() - posBeforeWrite;

	if (nRawEntrySize > std::numeric_limits<DataSize>::max())
		{ AddWriteNote(SNW_INSUFFICIENT_DATASIZETYPE); return; }

	if(GetFlag(RwfRMapHasSize) && nRawEntrySize > (std::numeric_limits<DataSize>::max() >> 2))
		{ AddWriteNote(SNW_DATASIZETYPE_OVERFLOW); return; }

	DataSize nEntrySize = static_cast<DataSize>(nRawEntrySize);

	// Handle fixed size entries:
	if (m_nFixedEntrySize > 0)
	{
		if(nEntrySize <= m_nFixedEntrySize)
		{
			for(uint32 i = 0; i<m_nFixedEntrySize-nEntrySize; i++)
				m_pOstrm->put(0);
			nEntrySize = m_nFixedEntrySize;
		}
		else
			{ AddWriteNote(SNW_INSUFFICIENT_FIXEDSIZE); return; }
	}
	if (GetFlag(RwfRwHasMap))
		WriteMapItem(pId, nIdSize, static_cast<RposType>(posBeforeWrite - m_posStart), nEntrySize, "");

	if (m_fpLogFunc != nullptr)
		AddWriteNote(pId, nIdSize, m_nCounter, nEntrySize, static_cast<RposType>(posBeforeWrite - m_posStart));
	IncrementWriteCounter();
}


void Ssb::CompareId(std::istream& iStrm, const char* pId, const size_t nIdlength)
//---------------------------------------------------------------------------
{
	uint8 tempU8 = 0;
	Binaryread<uint8>(iStrm, tempU8);
	char buffer[256];
	if(tempU8)
		iStrm.read(buffer, tempU8);

	if (tempU8 == nIdlength && nIdlength > 0 && memcmp(buffer, pId, nIdlength) == 0)
		return; // Match.
	else if (GetFlag(RwfRPartialIdMatch) && tempU8 > nIdlength && nIdlength > 0
			&& memcmp(buffer, pId, nIdlength) == 0 && buffer[nIdlength] == 0)
		return; // Partial match.

	AddReadNote(SNR_OBJECTCLASS_IDMISMATCH);
}


void Ssb::BeginRead(const char* pId, const size_t nLength, const uint64& nVersion)
//---------------------------------------------------------------------------------
{
	std::istream& iStrm = *m_pIstrm;

	if (m_fpLogFunc)
		m_fpLogFunc(tstrReadingHeader, IdToString(pId, nLength).c_str());

	ResetReadstatus();

	if (!iStrm.good())
		{ AddReadNote(SNRW_BADGIVEN_STREAM); return; }

	m_posStart = iStrm.tellg();

	// Start bytes.
	{
		char temp[sizeof(s_EntryID)];
		ArrayReader<char>(sizeof(s_EntryID))(iStrm, temp, sizeof(s_EntryID));
		if (memcmp(temp, s_EntryID, sizeof(s_EntryID)))
		{
			AddReadNote(SNR_STARTBYTE_MISMATCH);
			return;
		}
	}
	
	// Compare IDs.
	CompareId(iStrm, pId, nLength);
	if ((m_Status & SNT_FAILURE) != 0)
	{
		if (m_fpLogFunc)
			m_fpLogFunc(strIdMismatch);
		return;
	}

	if (m_fpLogFunc)
		m_fpLogFunc(strIdMatch);
	
	// Header
	uint8 tempU8;
	Binaryread<uint8>(iStrm, tempU8);
	const uint8 header = tempU8;
	m_nIdbytes = ((header & 3) == 3) ? 4 : (header & 3);
	if (Testbit(header, 6))
		SetFlag(RwfRTwoBytesDescChar, true);

	// Read headerdata size
	uint32 tempU32 = 0;
	ReadAdaptive1234(iStrm, tempU32);
	const uint32 headerdatasize = tempU32;

	// If headerdatasize != 0, read known headerdata and ignore rest.
	uint8 flagbyte = s_DefaultFlagbyte;
	if(headerdatasize >= 2)
	{
		Binaryread<uint8>(iStrm, tempU8);
		if(tempU8 == HeaderId_FlagByte)
			Binaryread<uint8>(iStrm, flagbyte);

		iStrm.ignore( (tempU8 == HeaderId_FlagByte) ? headerdatasize - 2 : headerdatasize - 1);
	}

	uint64 tempU64 = 0;

	// Read version numeric if available.
	if (Testbit(header, 4))
	{
		ReadAdaptive1248(iStrm, tempU64);
		m_nReadVersion = tempU64;
		if(tempU64 > nVersion)
			AddReadNote(SNR_LOADING_OBJECT_WITH_LARGER_VERSION);
	}

	if (Testbit(header, 5))
	{
        Binaryread<uint8>(iStrm, tempU8);
		iStrm.ignore(tempU8);
	}

	if(Testbit(flagbyte, 0)) // Custom ID?
	{
		Binaryread<uint8>(iStrm, tempU8);
		if ((tempU8 & 1) != 0)
			m_nIdbytes = IdSizeVariable;
		else
			m_nIdbytes = (tempU8 >> 1);
		if(m_nIdbytes == 0)
			AddReadNote(SNR_NO_ENTRYIDS_WITH_CUSTOMID_DEFINED);
	}

	m_nFixedEntrySize = 0;
	if(Testbit(flagbyte, 1)) // Fixedsize entries?
		ReadAdaptive1234(iStrm, m_nFixedEntrySize);

	SetFlag(RwfRMapHasStartpos, Testbit(header, 2));
	SetFlag(RwfRMapHasSize, Testbit(header, 3));
	SetFlag(RwfRMapHasId, (m_nIdbytes > 0));
	SetFlag(RwfRMapHasDesc, Testbit(header, 7));
	SetFlag(RwfRwHasMap, GetFlag(RwfRMapHasId) || GetFlag(RwfRMapHasStartpos) || GetFlag(RwfRMapHasSize) || GetFlag(RwfRMapHasDesc));
	
	if (GetFlag(RwfRwHasMap) == false && m_fpLogFunc)
		m_fpLogFunc(strNoMapInFile);

	if (Testbit(flagbyte, 2)) // Object description?
	{
		uint16 size = 0;
		ReadAdaptive12(iStrm, size);
		iStrm.ignore(size * (GetFlag(RwfRTwoBytesDescChar) ? 2 : 1));
	}

    if(Testbit(flagbyte, 3))
		iStrm.ignore(5);

	// Read entrycount
	ReadAdaptive1248(iStrm, tempU64);
	if(tempU64 > m_nMaxReadEntryCount)
		{ AddReadNote(SNR_TOO_MANY_ENTRIES_TO_READ); return; }

	m_nReadEntrycount = static_cast<NumType>(tempU64);
	if(m_nReadEntrycount == 0)
		AddReadNote(SNR_ZEROENTRYCOUNT);

	// Read map rpos if map exists.
	if (GetFlag(RwfRwHasMap))
	{
		ReadAdaptive1248(iStrm, tempU64);
		if(tempU64 > static_cast<uint64>(std::numeric_limits<Offtype>::max()))
			{ AddReadNote(SNR_INSUFFICIENT_STREAM_OFFTYPE); return; }
	}

	const Offtype rawEndOfHdrData = iStrm.tellg() - m_posStart;

	if (rawEndOfHdrData < 0 || rawEndOfHdrData > std::numeric_limits<RposType>::max())
		{ AddReadNote(SNR_INSUFFICIENT_RPOSTYPE); return; }

	m_rposEndofHdrData = static_cast<RposType>(rawEndOfHdrData);
	m_rposMapBegin = (GetFlag(RwfRwHasMap)) ? static_cast<RposType>(tempU64) : m_rposEndofHdrData;

	if (GetFlag(RwfRwHasMap) == false)
		m_posMapEnd = m_posStart + m_rposEndofHdrData;

	SetFlag(RwfRHeaderIsRead, true);
}


void Ssb::CacheMap()
//------------------
{
	std::istream& iStrm = *m_pIstrm;
	if(GetFlag(RwfRwHasMap) || m_nFixedEntrySize > 0)
	{
		iStrm.seekg(m_posStart + m_rposMapBegin);

		if(iStrm.fail())
			{ AddReadNote(SNR_BADSTREAM_AFTER_MAPHEADERSEEK); return; }

		if (m_fpLogFunc)
			m_fpLogFunc(tstrReadingMap, m_rposMapBegin);

		mapData.resize(m_nReadEntrycount);
		m_Idarray.reserve(m_nReadEntrycount * 4);

		//Read map
		for(NumType i = 0; i<m_nReadEntrycount; i++)
		{
			if(iStrm.fail())
				{ AddReadNote(SNR_BADSTREAM_AT_MAP_READ); return; }

			// Read ID.
			uint16 nIdsize = m_nIdbytes;
			if(nIdsize == IdSizeVariable) //Variablesize ID
				ReadAdaptive12(iStrm, nIdsize);
			const size_t nOldEnd = m_Idarray.size();
			if (nIdsize > 0 && (Util::MaxValueOfType(nOldEnd) - nOldEnd >= nIdsize))
			{
				m_Idarray.resize(nOldEnd + nIdsize);
				iStrm.read(&m_Idarray[nOldEnd], nIdsize);
			}
			mapData[i].nIdLength = nIdsize;
			mapData[i].nIdpos = nOldEnd;

			// Read position.
			if(GetFlag(RwfRMapHasStartpos))
			{
				uint64 tempU64;
				ReadAdaptive1248(iStrm, tempU64);
				if(tempU64 > static_cast<uint64>(std::numeric_limits<Offtype>::max()))
					{ AddReadNote(SNR_INSUFFICIENT_STREAM_OFFTYPE); return; }
				mapData[i].rposStart = static_cast<RposType>(tempU64);
			}

			// Read entry size.
			if (m_nFixedEntrySize > 0)
				mapData[i].nSize = m_nFixedEntrySize;
			else if(GetFlag(RwfRMapHasSize)) // Map has datasize field.
			{
				uint64 tempU64;
				ReadAdaptive1248(iStrm, tempU64);
				if(tempU64 > static_cast<uint64>(std::numeric_limits<Offtype>::max()))
					{ AddReadNote(SNR_INSUFFICIENT_STREAM_OFFTYPE); return; }
				mapData[i].nSize = static_cast<DataSize>(tempU64);
			}

			// If there's no entry startpos in map, count start pos from datasizes.
			// Here readentry.rposStart is set to relative position from databegin.
			if (mapData[i].nSize != invalidDatasize && GetFlag(RwfRMapHasStartpos) == false)
				mapData[i].rposStart = (i > 0) ? mapData[i-1].rposStart + mapData[i-1].nSize : 0;

			if(GetFlag(RwfRMapHasDesc)) //Map has entrydescriptions?
			{
				uint16 size = 0;
				ReadAdaptive12(iStrm, size);
				if(GetFlag(RwfRTwoBytesDescChar))
					iStrm.ignore(size * 2);
				else
					iStrm.ignore(size);
			}
		}
		m_posMapEnd = iStrm.tellg();
		if (m_fpLogFunc)
			m_fpLogFunc(tstrEndOfMap, m_posMapEnd - m_posStart);
	}

	SetFlag(RwfRMapCached, true);
	m_posDataBegin = (m_rposMapBegin == m_rposEndofHdrData) ? m_posMapEnd : m_posStart + Postype(m_rposEndofHdrData);
	m_pIstrm->seekg(m_posDataBegin);

	// If there are no positions in the map but there are entry sizes, rposStart will
	// be relative to data start. Now that posDataBegin is known, make them relative to 
	// startpos.
	if (GetFlag(RwfRMapHasStartpos) == false && (GetFlag(RwfRMapHasSize) || m_nFixedEntrySize > 0))
	{
		const RposType offset = static_cast<RposType>(m_posDataBegin - m_posStart);
		for(size_t i = 0; i < m_nReadEntrycount; i++)
			mapData[i].rposStart += offset;
	}
}


const ReadEntry* Ssb::Find(const char* pId, const size_t nIdLength)
//-----------------------------------------------------------------
{
	m_pIstrm->clear();
	if (GetFlag(RwfRMapCached) == false)
		CacheMap();
	
	if (m_nFixedEntrySize > 0 && GetFlag(RwfRMapHasStartpos) == false && GetFlag(RwfRMapHasSize) == false)
		m_pIstrm->seekg(m_posDataBegin + Postype(m_nFixedEntrySize * m_nCounter));

	if (GetFlag(RwfRMapHasId) == true)
	{
		const size_t nEntries = mapData.size();
		for(size_t i0 = 0; i0 < nEntries; i0++)
		{
			const size_t i = (i0 + m_nNextReadHint) % nEntries;
			if (mapData[i].nIdLength == nIdLength && mapData[i].nIdpos < m_Idarray.size() &&
				memcmp(&m_Idarray[mapData[i].nIdpos], pId, mapData[i].nIdLength) == 0)
			{
				m_nNextReadHint = (i + 1) % nEntries;
				if (mapData[i].rposStart != 0)
					m_pIstrm->seekg(m_posStart + Postype(mapData[i].rposStart));
				return &mapData[i];
			}
		}
	}
	return nullptr;
}


void Ssb::FinishWrite()
//---------------------
{
	std::ostream& oStrm = *m_pOstrm;
	const Postype posDataEnd = oStrm.tellp();
	std::string mapStreamStr = m_MapStream.str();
	if (m_posMapStart != Postype(0) && ((uint32)mapStreamStr.length() > m_nMapReserveSize))
		{ AddWriteNote(SNW_INSUFFICIENT_MAPSIZE); return; }
		
	if (m_posMapStart < 1)
		m_posMapStart = oStrm.tellp();
	else
		oStrm.seekp(m_posMapStart);

	if (m_fpLogFunc)
		m_fpLogFunc(tstrWritingMap, uint32(m_posMapStart - m_posStart));

	if (GetFlag(RwfRwHasMap)) //Write map
	{
		oStrm.write(mapStreamStr.c_str(), mapStreamStr.length());
	}

	const Postype posMapEnd = oStrm.tellp();
	
	// Write entry count.
	oStrm.seekp(m_posEntrycount);
	Binarywrite<size_t>(oStrm, (m_nCounter << 2) | 1, 2);

	if (GetFlag(RwfRwHasMap))
	{	// Write map start position.
		oStrm.seekp(m_posMapPosField);
		const uint64 rposMap = m_posMapStart - m_posStart;
		Binarywrite<uint64>(oStrm, rposMap << 2 | 3);
	}

	// Seek to end.
	oStrm.seekp(MAX(posMapEnd, posDataEnd)); 

	if (m_fpLogFunc)
		m_fpLogFunc(tstrEndOfStream, uint32(oStrm.tellp() - m_posStart));
}

} // namespace srlztn 

