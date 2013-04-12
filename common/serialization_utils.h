/*
 * serialization_utils.h
 * ---------------------
 * Purpose: Serializing data to and from MPTM files.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <string>
#include <strstream>
#include <vector>
#include <bitset>
#include "../common/misc_util.h"
#include "../common/typedefs.h"
#include <limits>
#ifdef HAS_TYPE_TRAITS
#include <type_traits>
#endif
#include <algorithm>

namespace srlztn //SeRiaLiZaTioN
{

typedef std::ostream OutStream;
typedef std::istream InStream;
typedef std::iostream IoStream;
typedef std::istrstream IstrStream;
typedef std::ostrstream OstrStream;
typedef OutStream::off_type Offtype;
typedef Offtype Postype;
typedef std::streamsize Streamsize;

typedef uintptr_t	DataSize;	// Data size type.
typedef uintptr_t	RposType;	// Relative position type.
typedef uintptr_t	NumType;	// Entry count type.

const DataSize invalidDatasize = DataSize(-1);

typedef std::basic_string<TCHAR> String;

enum 
{
	SNT_PROGRESS =		0x80000000, // = 1 << 31
	SNT_FAILURE =		0x40000000, // = 1 << 30
	SNT_NOTE =			0x20000000, // = 1 << 29
	SNT_WARNING =		0x10000000, // = 1 << 28
	SNT_NONE = 0,
	SNT_ALL_ENABLED = -1,
	SNT_DEFAULT_MASK = SNT_FAILURE | SNT_WARNING,

	SNRW_BADGIVEN_STREAM =								1	| SNT_FAILURE,

	// Read failures.
	SNR_BADSTREAM_AFTER_MAPHEADERSEEK =					2	| SNT_FAILURE,
	SNR_STARTBYTE_MISMATCH =							3	| SNT_FAILURE,
	SNR_BADSTREAM_AT_MAP_READ =							4	| SNT_FAILURE,
	SNR_INSUFFICIENT_STREAM_OFFTYPE =					5	| SNT_FAILURE,
	SNR_OBJECTCLASS_IDMISMATCH =						6	| SNT_FAILURE,
	SNR_TOO_MANY_ENTRIES_TO_READ =						7	| SNT_FAILURE,
	SNR_INSUFFICIENT_RPOSTYPE =							8	| SNT_FAILURE,

	// Read notes and warnings.
	SNR_ZEROENTRYCOUNT =								0x80	| SNT_NOTE, // 0x80 == 1 << 7
	SNR_NO_ENTRYIDS_WITH_CUSTOMID_DEFINED =				0x100	| SNT_NOTE,
	SNR_LOADING_OBJECT_WITH_LARGER_VERSION =			0x200	| SNT_NOTE,
	
	// Write failures.
	SNW_INSUFFICIENT_FIXEDSIZE =						(0x10)	| SNT_FAILURE,
	SNW_CHANGING_IDSIZE_WITH_FIXED_IDSIZESETTING =		(0x11)	| SNT_FAILURE,
	SNW_INSUFFICIENT_MAPSIZE =							(0x12)	| SNT_FAILURE,
	SNW_DATASIZETYPE_OVERFLOW =							(0x13)	| SNT_FAILURE,
	SNW_MAX_WRITE_COUNT_REACHED =						(0x14)	| SNT_FAILURE,
	SNW_SUBENTRY_FAILURE =								(0x15)	| SNT_FAILURE,
	SNW_INSUFFICIENT_DATASIZETYPE =						(0x16)	| SNT_FAILURE,
};

bool IsPrintableId(const void* pvId, const size_t nLength); // Return true if given id is printable, false otherwise. 
void ReadAdaptive1248(InStream& iStrm, uint64& val);
void WriteAdaptive1248(OutStream& oStrm, const uint64& val);

enum
{
	IdSizeVariable = uint16_max,
	IdSizeMaxFixedSize = (uint8_max >> 1)
};

typedef int32 SsbStatus;


struct ReadEntry
//==============
{
	ReadEntry() : nIdpos(0), rposStart(0), nSize(invalidDatasize), nIdLength(0) {}

	uintptr_t nIdpos;	// Index of id start in ID array.
	RposType rposStart;	// Entry start position.
	DataSize nSize;		// Entry size.
	uint16 nIdLength;	// Length of id.
};


enum Rwf
{
	RwfWMapStartPosEntry,	// Write. True to include data start pos entry to map.
	RwfWMapSizeEntry,		// Write. True to include data size entry to map.
	RwfWMapDescEntry,		// Write. True to include description entry to map.
	RwfWVersionNum,			// Write. True to include version numeric.
	RwfRPartialIdMatch,		// Read. True to allow partial ID match.
	RwfRMapCached,			// Read. True if map has been cached.
	RwfRMapHasId,			// Read. True if map has IDs
	RwfRMapHasStartpos,		// Read. True if map data start pos.
	RwfRMapHasSize,			// Read. True if map has entry size.
	RwfRMapHasDesc,			// Read. True if map has entry description.
	RwfRTwoBytesDescChar,	// Read. True if map description characters are two bytes.
	RwfRHeaderIsRead,		// Read. True when header is read.
	RwfRwHasMap,			// Read/write. True if map exists.
	RwfNumFlags
};


class Ssb
//=======
{
public:
	typedef void (*fpLogFunc_t)(const TCHAR*, ...);

	enum ReadRv // Read return value.
	{
		EntryRead,
		EntryNotFound
	};
	enum IdMatchStatus
	{
		IdMatch, IdMismatch
	};
	typedef std::vector<ReadEntry>::const_iterator ReadIterator;

	Ssb(InStream* pIstrm, OutStream* pOstrm);
	Ssb(IoStream& ioStrm);
	Ssb(OutStream& oStrm);
	Ssb(InStream& iStrm);

	~Ssb() {delete m_pSubEntry;}

	// Sets map ID size in writing.
	void SetIdSize(uint16 idSize);

	// Write header
	void BeginWrite(const void* pId, const size_t nIdSize, const uint64& nVersion);
	void BeginWrite(const LPCSTR pszId, const uint64& nVersion) {BeginWrite(pszId, strlen(pszId), nVersion);}
	
	// Call this to begin reading: must be called before other read functions.
	void BeginRead(const void* pId, const size_t nLength, const uint64& nVersion);
	void BeginRead(const LPCSTR pszId, const uint64& nVersion) {return BeginRead(pszId, strlen(pszId), nVersion);}

	// Reserves space for map to current position. Call after BeginWrite and before writing any entries.
	void ReserveMapSize(uint32 nSize);

	// Creates subentry for writing. Use SubEntry() to access the subentry and 
	// when done, call ReleaseSubEntry. Don't call WriteItem() for 'this' while 
	// subentry is active.
	void CreateWriteSubEntry();

	// Returns current write/read subentry. CreateWriteSubEntry/CreateReadSubEntry
	// must be called before calling this.
	Ssb& SubEntry() {return *m_pSubEntry;}

	// Releases write subentry and writes corresponding map information.
	void ReleaseWriteSubEntry(const void* pId, const size_t nIdLength);
	void ReleaseWriteSubEntry(const LPCSTR pszId) {ReleaseWriteSubEntry(pszId, strlen(pszId));}

	// If ID was found, returns pointer to Ssb object, nullptr if not found.
	// Note: All reading on subentry must be done before calling ReadItem with 'this'.
	Ssb* CreateReadSubEntry(const void* pId, const size_t nLength);
	Ssb* CreateReadSubEntry(const LPCSTR pszId) {return CreateReadSubEntry(pszId, strlen(pszId));}

	// After calling BeginRead(), this returns number of entries in the file.
	NumType GetNumEntries() const {return m_nReadEntrycount;}

	// Returns read iterator to the beginning of entries.
	// The behaviour of read iterators is undefined if map doesn't
    // contain entry ids or data begin positions.
	ReadIterator GetReadBegin();

	// Returns read iterator to the end(one past last) of entries.
	ReadIterator GetReadEnd();

	// Compares given id with read entry id 
	IdMatchStatus CompareId(const ReadIterator& iter, LPCSTR pszId) {return CompareId(iter, pszId, strlen(pszId));}
	IdMatchStatus CompareId(const ReadIterator& iter, const void* pId, const size_t nIdSize);

	// When writing, returns the number of entries written.
	// When reading, returns the number of entries read not including unrecognized entries.
	NumType GetCounter() const {return m_nCounter;}

	uint64 GetReadVersion() {return m_nReadVersion;}

	// Read item using default read implementation.
	template <class T>
	ReadRv ReadItem(T& obj, const LPCSTR pszId) {return ReadItem(obj, pszId, strlen(pszId), srlztn::ReadItem<T>);}

	template <class T>
	ReadRv ReadItem(T& obj, const void* pId, const size_t nIdSize) {return ReadItem(obj, pId, nIdSize, srlztn::ReadItem<T>);}

	// Read item using given function.
	template <class T, class FuncObj>
	ReadRv ReadItem(T& obj, const void* pId, const size_t nIdSize, FuncObj);

	// Read item using read iterator.
	template <class T>
	ReadRv ReadItem(const ReadIterator& iter, T& obj) {return ReadItem(iter, obj, srlztn::ReadItem<T>);}
	template <class T, class FuncObj>
	ReadRv ReadItem(const ReadIterator& iter, T& obj, FuncObj func);

	// Write item using default write implementation.
	template <class T>
	void WriteItem(const T& obj, const LPCSTR pszId) {WriteItem(obj, pszId, strlen(pszId), &srlztn::WriteItem<T>);}

	template <class T>
	void WriteItem(const T& obj, const void* pId, const size_t nIdSize) {WriteItem(obj, pId, nIdSize, &srlztn::WriteItem<T>);}

	// Write item using given function.
	template <class T, class FuncObj>
	void WriteItem(const T& obj, const void* pId, const size_t nIdSize, FuncObj);

	// Writes mapping.
	void FinishWrite();

	void SetFlag(Rwf flag, bool val) {m_Flags.set(flag, val);}
	bool GetFlag(Rwf flag) const {return m_Flags[flag];}

	// Write given string to log if log func is defined.
	void Log(LPCTSTR psz) {if (m_fpLogFunc) m_fpLogFunc(psz);}

	SsbStatus m_Status;
	uint32 m_nFixedEntrySize;			// Read/write: If > 0, data entries have given fixed size.
	fpLogFunc_t m_fpLogFunc;			// Pointer to log function.

private:
	// Reads map to cache.
	void CacheMap();

	// Compares ID in file with expected ID.
	void CompareId(InStream& iStrm, const void* pId, const size_t nLength);

	// Searches for entry with given ID. If found, returns pointer to corresponding entry, else
	// returns nullptr.
	const ReadEntry* Find(const void* pId, const size_t nLength);
	const ReadEntry* Find(const LPCSTR pszId) {return Find(pszId, strlen(pszId));}

	// Called after reading an object.
	ReadRv OnReadEntry(const ReadEntry* pE, const void* pId, const size_t nIdSize, const Postype& posReadBegin);

	// Called after writing an item.
	void OnWroteItem(const void* pId, const size_t nIdSize, const Postype& posBeforeWrite);
	
	void AddNote(const SsbStatus s, const SsbStatus mask, const TCHAR* sz);

	void AddReadNote(const SsbStatus s);

	// Called after reading entry. pRe is a pointer to associated map entry if exists.
	void AddReadNote(const ReadEntry* const pRe, const NumType nNum);

	void AddWriteNote(const SsbStatus s);
	void AddWriteNote(const void* pId,
					  const size_t nIdLength,
					  const NumType nEntryNum,
					  const DataSize nBytecount,
					  const RposType rposStart);

	// Writes mapping item to mapstream.
	void WriteMapItem(const void* pId, 
				  const size_t nIdSize,
				  const RposType& rposDataStart,
				  const DataSize& nDatasize,
				  const TCHAR* pszDesc);

	void ResetReadstatus();
	void ResetWritestatus() {m_Status = SNT_NONE;}

	void IncrementWriteCounter();

private:
	SsbStatus m_Readlogmask;			// Read: Controls which read messages will be written to log.
	SsbStatus m_Writelogmask;			// Write: Controls which write messages will be written to log.
	
	std::vector<char> m_Idarray;		// Read: Holds entry ids.
	
	InStream* m_pIstrm;					// Read: Pointer to read stream.
	OutStream* m_pOstrm;				// Write: Pointer to write stream.

	Postype m_posStart;					// Read/write: Stream position at the beginning of object.
	std::vector<ReadEntry> mapData;		// Read: Contains map information.
	uint64 m_nReadVersion;				// Read: Version is placed here when reading.
	NumType m_nMaxReadEntryCount;		// Read: Limits the number of entries allowed to be read.
	RposType m_rposMapBegin;			// Read: If map exists, rpos of map begin, else m_rposEndofHdrData.
	Postype m_posMapEnd;				// Read: If map exists, map end position, else pos of end of hdrData.
	Postype m_posDataBegin;				// Read: Data begin position.
	RposType m_rposEndofHdrData;		// Read: rpos of end of header data.
	NumType m_nReadEntrycount;			// Read: Number of entries.

	uint16 m_nIdbytes;					// Read/Write: Tells map ID entry size in bytes. If size is variable, value is IdSizeVariable.
	NumType m_nCounter;					// Read/write: Keeps count of entries written/read.
	NumType m_nNextReadHint;			// Read: Hint where to start looking for the next read entry.
	std::bitset<RwfNumFlags> m_Flags;	// Read/write: Various flags.

	Ssb* m_pSubEntry;					// Read/Write: Pointer to SubEntry.
	Postype m_posSubEntryStart;			// Write: Holds data position where SubEntry started.
	uint32 m_nMapReserveSize;			// Write: Number of bytes to reserve for map if writing it before data.			
	Postype m_posEntrycount;			// Write: Pos of entrycount field. 
	Postype m_posMapPosField;			// Write: Pos of map position field.
	Postype m_posMapStart;				// Write: Pos of map start.
	OstrStream m_MapStream;				// Write: Map stream.

private:
	static const uint8 HeaderId_FlagByte = 0;
public:
	static const uint8 s_DefaultFlagbyte = 0;
	static int32 s_DefaultReadLogMask;
	static int32 s_DefaultWriteLogMask;
	static fpLogFunc_t s_DefaultLogFunc;
	static const char s_EntryID[3];
	static const int32 s_DefaultFlags = (1 << RwfWMapStartPosEntry) +
										 (1 << RwfWMapSizeEntry) + (1 << RwfWVersionNum) +
										 (1 << RwfRPartialIdMatch);
};


template <class T, class FuncObj>
void Ssb::WriteItem(const T& obj, const void* pId, const size_t nIdSize, FuncObj Func)
//------------------------------------------------------------------------------------
{
	const Postype pos = m_pOstrm->tellp();
	Func(*m_pOstrm, obj);
	OnWroteItem(pId, nIdSize, pos);
}

template <class T, class FuncObj>
Ssb::ReadRv Ssb::ReadItem(T& obj, const void* pId, const size_t nIdSize, FuncObj Func)
//------------------------------------------------------------------------------------
{
	const ReadEntry* pE = Find(pId, nIdSize);
	const Postype pos = m_pIstrm->tellg();
	if (pE != nullptr || GetFlag(RwfRMapHasId) == false)
		Func(*m_pIstrm, obj, (pE) ? (pE->nSize) : invalidDatasize);
	return OnReadEntry(pE, pId, nIdSize, pos);
}


template <class T, class FuncObj>
Ssb::ReadRv Ssb::ReadItem(const ReadIterator& iter, T& obj, FuncObj func)
//-----------------------------------------------------------------------
{
	m_pIstrm->clear();
	if (iter->rposStart != 0)
		m_pIstrm->seekg(m_posStart + Postype(iter->rposStart));
	const Postype pos = m_pIstrm->tellg();
	func(*m_pIstrm, obj, iter->nSize);
	return OnReadEntry(&(*iter), &m_Idarray[iter->nIdpos], iter->nIdLength, pos);
}


inline Ssb::IdMatchStatus Ssb::CompareId(const ReadIterator& iter, const void* pId, const size_t nIdSize)
//-------------------------------------------------------------------------------------------------------
{
	if (nIdSize == iter->nIdLength && memcmp(&m_Idarray[iter->nIdpos], pId, iter->nIdLength) == 0)
		return IdMatch;
	else
		return IdMismatch;
}


inline Ssb::ReadIterator Ssb::GetReadBegin()
//------------------------------------------
{
	ASSERT(GetFlag(RwfRMapHasId) && (GetFlag(RwfRMapHasStartpos) || GetFlag(RwfRMapHasSize) || m_nFixedEntrySize > 0));
	if (GetFlag(RwfRMapCached) == false)
		CacheMap();
	return mapData.begin();
}


inline Ssb::ReadIterator Ssb::GetReadEnd()
//----------------------------------------
{
	if (GetFlag(RwfRMapCached) == false)
		CacheMap();
	return mapData.end();
}


template<class T>
inline void Binarywrite(OutStream& oStrm, const T& data)
//------------------------------------------------------
{
	oStrm.write(reinterpret_cast<const char*>(&data), sizeof(data));
}

//Write only given number of bytes from the beginning.
template<class T>
inline void Binarywrite(OutStream& oStrm, const T& data, const Offtype bytecount)
//--------------------------------------------------------------------------
{
	oStrm.write(reinterpret_cast<const char*>(&data), MIN(bytecount, sizeof(data)));
}

template <class T>
inline void WriteItem(OutStream& oStrm, const T& data)
//----------------------------------------------------
{
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	Binarywrite(oStrm, data);
}

void WriteItemString(OutStream& oStrm, const char* const pStr, const size_t nSize);

template <>
inline void WriteItem<std::string>(OutStream& oStrm, const std::string& str) {WriteItemString(oStrm, str.c_str(), str.length());}

template <>
inline void WriteItem<LPCSTR>(OutStream& oStrm, const LPCSTR& psz) {WriteItemString(oStrm, psz, strlen(psz));}

template<class T>
inline void Binaryread(InStream& iStrm, T& data)
//----------------------------------------------
{
	iStrm.read(reinterpret_cast<char*>(&data), sizeof(T));
}

//Read only given number of bytes to the beginning of data; data bytes are memset to 0 before reading.
template <class T>
inline void Binaryread(InStream& iStrm, T& data, const Offtype bytecount)
//-----------------------------------------------------------------------
{
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	memset(&data, 0, sizeof(data));
	iStrm.read(reinterpret_cast<char*>(&data), (std::min)((size_t)bytecount, sizeof(data)));
}


template <class T>
inline void ReadItem(InStream& iStrm, T& data, const DataSize nSize)
//------------------------------------------------------------------
{
	#ifdef HAS_TYPE_TRAITS
		static_assert(std::is_trivial<T>::value == true, "");
	#endif
	if (nSize == sizeof(T) || nSize == invalidDatasize)
		Binaryread(iStrm, data);
	else
		Binaryread(iStrm, data, nSize);
}

// Read specialization for float. If data size is 8, read double and assign it to given float.
template <>
inline void ReadItem<float>(InStream& iStrm, float& f, const DataSize nSize)
//--------------------------------------------------------------------------
{
	if (nSize == 8)
	{
		double d;
		Binaryread(iStrm, d);
		f = static_cast<float>(d);
	}
	else
		Binaryread(iStrm, f);
}

// Read specialization for double. If data size is 4, read float and assign it to given double.
template <>
inline void ReadItem<double>(InStream& iStrm, double& d, const DataSize nSize)
//----------------------------------------------------------------------------
{
	if (nSize == 4)
	{
		float f;
		Binaryread(iStrm, f);
		d = f;
	}
	else
		Binaryread(iStrm, d);
}

void ReadItemString(InStream& iStrm, std::string& str, const DataSize); 

template <>
inline void ReadItem<std::string>(InStream& iStrm, std::string& str, const DataSize nSize)
//----------------------------------------------------------------------------------------
{
	ReadItemString(iStrm, str, nSize);
}


template <class T>
struct ArrayWriter
//================
{
	ArrayWriter(size_t nCount) : m_nCount(nCount) {}
	void operator()(srlztn::OutStream& oStrm, const T* pData) {oStrm.write(reinterpret_cast<const char*>(pData), m_nCount * sizeof(T));} 
	size_t m_nCount;
};

template <class T>
struct ArrayReader
//================
{
	ArrayReader(size_t nCount) : m_nCount(nCount) {}
	void operator()(srlztn::InStream& iStrm, T* pData, const size_t) {iStrm.read(reinterpret_cast<char*>(pData), m_nCount * sizeof(T));} 
	size_t m_nCount;
};

} //namespace srlztn.


template<class SIZETYPE>
bool StringToBinaryStream(std::ostream& oStrm, const std::string& str)
//--------------------------------------------------------------------
{
	if(!oStrm.good()) return true;
	if((std::numeric_limits<SIZETYPE>::max)() < str.size()) return true;
	SIZETYPE size = static_cast<SIZETYPE>(str.size());
	oStrm.write(reinterpret_cast<char*>(&size), sizeof(size));
	oStrm.write(str.c_str(), size);
	if(oStrm.good()) return false;
	else return true;
}


template<class SIZETYPE>
bool StringFromBinaryStream(std::istream& iStrm, std::string& str, const SIZETYPE maxSize = (std::numeric_limits<SIZETYPE>::max)())
//---------------------------------------------------------------------------------------------------------------------------------
{
	if(!iStrm.good()) return true;
	SIZETYPE strSize;
	iStrm.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
	if(strSize > maxSize)
		return true;
	str.resize(strSize);
	for(SIZETYPE i = 0; i<strSize; i++)
		iStrm.read(&str[i], 1);
	if(iStrm.good()) return false;
	else return true;
}
