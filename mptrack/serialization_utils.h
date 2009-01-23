#ifndef SERIALIZATION_UTILS_H
#define SERIALIZATION_UTILS_H

#include <string>
#include <fstream>
#include <istream>
#include <ostream>
#include <strstream>
#include <vector>
#include <algorithm>
#include "misc_util.h"
#include "typedefs.h"
#include <limits>

using std::numeric_limits;
using std::ostringstream;
using std::istream;
using std::ostream;
using std::vector;
using std::string;

namespace srlztn //SeRiaLiZaTioN
{


using std::string;
using std::vector;


inline uint64 Pow2x(const uint8& exp)
//--------------------------------
{
	if(exp == 0) return 1;
	return 2 << ((exp-1));
}

inline uint8 Pow2xSmall(const uint8& exp) {return static_cast<uint8>(Pow2x(exp));}

//Note: Works only for arguments 1,2,4,8
inline uint8 Log2(const uint16& val)
//---------------------------------
{
	if(val == 1) return 0;
	if(val == 2) return 1;
	if(val == 4) return 2;
	if(val == 8) return 3;
	return 255;
}


inline uint8 GetNeededBytecount1248(const uint64 size)
//------------------------------------------------
{
	if((size >> 6) == 0) return 1;
	if((size >> (1*8+6)) == 0) return 2;
	if((size >> (3*8+6)) == 0) return 4;
	return 8;
}

inline uint8 GetNeededBytecount1234(const uint32 num)
//---------------------------------------------
{
	if((num >> 6) == 0) return 1;
	if((num >> (1*8+6)) == 0) return 2;
	if((num >> (2*8+6)) == 0) return 3;
	return 4;
}

//Note: Indexing starts from 0
inline bool Testbit(uint8 val, uint8 bitindex) {return ((val & (1 << bitindex)) != 0);}
//----------------------------------------------------------------------------------------
inline void Setbit(uint8& val, uint8 bitindex, bool newval)
//----------------------------------------------------------
{
	if(newval) val |= (1 << bitindex);
	else val &= ~(1 << bitindex);
}


class ABCSerializationInstructions;
class ABCSerializationStreamer;

typedef std::ostream OUTSTREAM;
typedef std::istream INSTREAM;
typedef std::istrstream ISTRSTREAM;
typedef std::ostrstream OSTRSTREAM;
typedef std::ios::off_type OFFTYPE;
typedef std::ios::pos_type POSTYPE;
typedef std::streamsize STREAMSIZE;
const uint8 OUTFLAG = std::ios::out;
const uint8 INFLAG = std::ios::in;
const uint8 CURPOS = std::ios::cur;

const uint64 invalidDatasize = uint64_max;
const OFFTYPE OFFTYPE_MIN = (numeric_limits<OFFTYPE>::min)();
const OFFTYPE OFFTYPE_MAX = (numeric_limits<OFFTYPE>::max)();

enum
{
	//SNT <-> SerializationNotificationType
	SNT_PROGRESS = 1 << 31,
	SNT_FAILURE = 1 << 30,
	SNT_NOTE = 1 << 29,
	SNT_WARNING = 1 << 28
};

//==============================
class CSerializationNotification
//==============================
{
public:
	CSerializationNotification(int32 i, const string& d) :
			ID(i), description(d) {}
	int32 ID;
	string description;
	bool operator==(const CSerializationNotification& cn) const {return ID == cn.ID;}
	operator int32() const {return ID;}
	string ToString() const;
};

class CReadNotification : public CSerializationNotification
//=============================================================
{
public:
	CReadNotification(int32 id = 0, const string& desc = "") : CSerializationNotification(id,desc) {}
};

class CWriteNotification : public CSerializationNotification
//=============================================================
{
public:
	CWriteNotification(int32 id = 0, const string& desc = "") : CSerializationNotification(id,desc) {}
};




class CSerializationentry
//=======================
{
public:
	friend class ABCSerializationInstructions;

	//Streamer pointer must not be null; usually the parameter can simply be 'new SomeStreamClass' -
	//deleting is handled by this object.
	CSerializationentry(const string& id, ABCSerializationStreamer* streamer,
		const string& description = "", const string readnote = "", bool deletestreamer = true)
		: m_Description(description), m_pStreamer(streamer), m_Readnote(readnote),
		  m_Deletestreamer(deletestreamer)
	{
		m_Id.resize(id.length());
		for(size_t i = 0; i<m_Id.size(); i++) {m_Id[i] = id[i];}
	}

	const ABCSerializationStreamer& GetStreamer() const {return *m_pStreamer;}

	~CSerializationentry();

	OFFTYPE Write(OUTSTREAM& oStrm) const;
	OFFTYPE Read(INSTREAM& iStrm, const uint64 datasize);

	const vector<char>& GetID() const {return m_Id;}
	const string& GetDescription() const {return m_Description;}
	const string& GetReadnote() const {return m_Readnote;}

	//Transferring ownership of streamer to new object when copying objects - copying enabled
	//so that arrays can be constructed.
	CSerializationentry(const CSerializationentry& e) {*this = e;}
	CSerializationentry& operator=(const CSerializationentry& e)
	{
		m_Id = e.m_Id;
		m_Description = e.m_Description;
		m_pStreamer = e.m_pStreamer;
		m_Deletestreamer = e.m_Deletestreamer;
		m_Readnote = e.m_Readnote;
		e.m_Deletestreamer = false;
		return *this;
	}

private:
	vector<char> m_Id;
	string m_Description;
	ABCSerializationStreamer* m_pStreamer;
	mutable bool m_Deletestreamer;
	string m_Readnote; //Note to be shown when reading this entry.
	//Note: Remember to check whether operator= needs updating if adding/removing stuff.
};


class CSSBSerialization //SSB <-> Simple Structured Binary
//=====================
{
private:
	class CMappinginfo
	{
	public:
		CMappinginfo(const vector<char>& i, uint64 start, uint64 size, const string& str) : id(i), startpos(start), entrysize(size), description(str) {}
		CMappinginfo() : startpos(0), entrysize(invalidDatasize) {}
		vector<char> id;
		uint64 startpos;
		uint64 entrysize;
		string description;
		string ToString() const
		{
			string temp; temp.reserve(id.size() + 60);
			temp = "Mapinfo: ";
			if(std::find(id.begin(), id.end(), 0) == id.end())
				std::copy(id.begin(), id.end(), std::back_inserter(temp));
			temp += ", " + Stringify(startpos) + ", " + Stringify(entrysize) + ", " + string(description);
			return temp;
		}
	};

public:

	CSSBSerialization(ABCSerializationInstructions& si) :
		m_pSi(&si),
		m_pLogstring(0),
		m_Readlogmask(s_DefaultReadLogMask),
		m_Writelogmask(s_DefaultWriteLogMask)
		{
		}

	//NOTES
	//	-The method is to fail if the given stream is opened with ios::app flag.
	//	-If an object contains this type of object(CSSBSerialization) in data,
	//   this method may get called recursively.
	const CWriteNotification& Serialize(OUTSTREAM& oStrm);

	//NOTES:
	//	-Given stream should be opened in binary mode.
	//	-If an object contains this type of object in data, this method may get called recursively.
	//	-Its uncertain whether reading objects(files) with size > OFFTYPE_MAX works.
	const CReadNotification& Unserialize(INSTREAM& iStrm);

	void SetLogstring(string& str) {m_pLogstring = &str;}

	static const char s_EntryID[3];

private:
	//Return true if reading should be terminated.
	bool AddReadNote(const CReadNotification& newStatus, const bool addToLog = true);
	bool AddReadNote(const CReadNotification& newStatus, const CMappinginfo* pMi);

	//Return true if writing should be terminated.
	bool AddWriteNote(const CWriteNotification& note, const bool addToLog = true);

	void AddToLog(const char*);
	void AddToLog(const CReadNotification& rn, const CMappinginfo* pMi);
	void ResetReadstatus();
	void ResetWritestatus();

	static void Seekg(INSTREAM& iStrm, const POSTYPE& startpos, int64 offset);

private:
	ABCSerializationInstructions* m_pSi;
	CReadNotification m_Readnotes;
	CWriteNotification m_Writenotes;
	int32 m_Readlogmask;
	int32 m_Writelogmask;
	string* m_pLogstring;


public:
	static const uint8 s_DefaultFlagbyte;
	static int32 s_DefaultReadLogMask;
	static int32 s_DefaultWriteLogMask;

private:

	enum
	{
		HEADERID_FLAGBYTE = 0
	};

};



//Returns the number of bytes that were tried to write.
template<class T>
inline OFFTYPE Binarywrite(OUTSTREAM& oStrm, const T& data)
//-------------------------------------------------------------
{
	oStrm.write(reinterpret_cast<const char*>(&data), sizeof(data));
	return sizeof(data);
}

//Write only given number of bytes from the beginning.
template<class T>
inline OFFTYPE Binarywrite(OUTSTREAM& oStrm, const T& data, const OFFTYPE bytecount)
//-------------------------------------------------------------------------------
{
	if(bytecount <= 0) return 0;
	const size_t bc = min(bytecount, sizeof(data));
	oStrm.write(reinterpret_cast<const char*>(&data), bc);
	return bc;
}

inline uint8 WriteAdaptive12(OUTSTREAM& oStrm, const uint16 num)
//------------------------------------------------------
{
	if(num >> 7 == 0)
	{
		Binarywrite<uint16>(oStrm, num << 1, 1);
		return 1;
	}
	else
	{
		Binarywrite<uint16>(oStrm, (num << 1) | 1);
		return 2;
	}
}


//Format: First bit tells whether the size indicator is 1 or 2 bytes.
inline OFFTYPE WriteAdaptive12String(OUTSTREAM& oStrm, const string& str)
//-----------------------------------------------------------------------
{
	const uint16 s =(str.size() <= 32767) ? static_cast<uint16>(str.size()) : 32767;
	WriteAdaptive12(oStrm, s);
	oStrm.write(str.c_str(), s);
	return (s > 127) ? s+2 : s+1;
}

//Returns the number of bytes that were tried to read.
template<class T>
inline OFFTYPE Binaryread(INSTREAM& iStrm, T& data)
//-----------------------------------------------------------------------
{
	iStrm.read(reinterpret_cast<char*>(&data), sizeof(T));
	return sizeof(T);
}

//Read only given number of bytes to the beginning of data; data bytes is memset to 0 before reading.
template<class T>
inline OFFTYPE Binaryread(INSTREAM& iStrm, T& data, const OFFTYPE bytecount)
//----------------------------------------------------------------------------------
{
	if(bytecount <= 0) return 0;
	const size_t bc = min(bytecount, sizeof(data));
	memset(&data, 0, sizeof(data));
	iStrm.read(reinterpret_cast<char*>(&data), bc);
	return bc;
}

//Read given number of bytes to string object.
inline OFFTYPE ReadBytes(INSTREAM& iStrm, string& str, const OFFTYPE bytecount)
//---------------------------------------------------------------------------------------------------------
{
	str.resize(bytecount);
	for(OFFTYPE i = 0; i<bytecount; i++)
		iStrm.read(&str[i], 1);
	return bytecount;
}

inline uint8 ReadAdaptive12(INSTREAM& iStrm, uint16& val)
//------------------------------------------------------
{
	val = 0;
	Binaryread<uint16>(iStrm, val, 1);
	if(val & 1) iStrm.read(reinterpret_cast<char*>(&val) + 1, 1);
	val >>= 1;
	return (val > 127) ? 2 : 1;
}



class ABCSerializationStreamer
//=============================
{
public:
	typedef CReadNotification READINFO;
	typedef CWriteNotification WRITEINFO;
	ABCSerializationStreamer(int flags = INFLAG|OUTFLAG) : m_Flags(static_cast<uint8>(flags)) {}
	virtual ~ABCSerializationStreamer() {}

	//Returns streampos increment which should be the same as bytes written.
	OFFTYPE Write(OUTSTREAM& oStrm) const
	{
		const POSTYPE startpos = oStrm.tellp();
		if(m_Flags & OUTFLAG) ProWrite(oStrm);
		return oStrm.tellp() - startpos;
	}

	//Returns stream increment. If entrysize is known, its given as argument 'datasize', else
	//the argument will be invalidDatasize.
	virtual OFFTYPE Read(INSTREAM& iStrm, const uint64 datasize)
	{
		m_LastReadinfo = READINFO(SNT_PROGRESS, "");
		const POSTYPE startpos = iStrm.tellg();
		if(m_Flags & INFLAG) ProRead(iStrm, datasize);
		else m_LastReadinfo = CReadNotification(SNT_WARNING, "Disabled readflag - no reading done.");
		return iStrm.tellg() - startpos;
	}

	READINFO GetLastReadinfo() const {return m_LastReadinfo;}
	WRITEINFO GetLastWriteinfo() const {return m_LastWriteinfo;}

protected:
	//NOTE: Write method must leave stream to position one byte after the last byte written.
	//(otherwise data will probably be overwritten)
	virtual void ProWrite(OUTSTREAM&) const = 0;
	virtual void ProRead(INSTREAM&, const uint64) = 0;

protected:
	uint8 m_Flags;
	READINFO m_LastReadinfo;
	mutable WRITEINFO m_LastWriteinfo;

	static const READINFO s_InfoNoReadimplementation;
	static const WRITEINFO s_InfoNoWriteimplementation;
};


template<class T>
class CBinarystreamer : public ABCSerializationStreamer
//=====================================================
{
public:
	CBinarystreamer(const T& obj, STREAMSIZE tr = sizeof(T), STREAMSIZE tw = sizeof(T)) :
		m_rData((T&)obj),
		m_BytesToRead(tr),
		m_BytesToWrite(tw)
		{
			if(m_BytesToRead < 0) m_BytesToRead = 0;
			if(m_BytesToWrite < 0) m_BytesToWrite = 0;
		}

	void ProWrite(OUTSTREAM& outStrm) const
	//-------------------------------------
	{
		if(m_BytesToWrite == 0) {m_LastWriteinfo = WRITEINFO(SNT_WARNING, "Bytes to write == 0"); return;}
		const STREAMSIZE bc = (m_BytesToWrite > sizeof(T)) ? sizeof(T) : m_BytesToWrite;
		outStrm.write(reinterpret_cast<const char*>(&m_rData), bc);
		if(m_BytesToWrite > sizeof(T))
			{for(size_t i = 0; i<m_BytesToWrite - sizeof(T); i++) outStrm.put(0);}
	}
	void ProRead(INSTREAM& inStrm, const uint64 datasize)
	//---------------------------------------------------------
	{
		if(m_BytesToRead == 0) {m_LastReadinfo = READINFO(SNT_WARNING, "Bytes to read == 0"); return;}
		if(datasize < m_BytesToRead) {m_LastReadinfo = READINFO(SNT_WARNING, "Datasize less than bytes to read; no reading done."); return;}
		if(std::numeric_limits<T>::is_integer)
			ReadAsInteger(inStrm);
		else
			Read<T>(inStrm);
	}

private:
	void DefaultRead(INSTREAM& inStrm)
	//--------------------------------
	{
		const STREAMSIZE bc = (m_BytesToRead > sizeof(T)) ? sizeof(T) : m_BytesToRead;
		if(m_BytesToRead < sizeof(T))
			memset(&m_rData, 0, sizeof(T));
		inStrm.read(reinterpret_cast<char*>(&m_rData), bc);
		if(m_BytesToRead > bc)
		{
			ostringstream oss;
			oss << "Read " << bc << " bytes instead of instructed " << m_BytesToRead << " bytes"
				<< "; possible loss of data.";
			m_LastReadinfo = READINFO(SNT_NOTE, oss.str());
		}
	}

	template<class C>
	void Read(INSTREAM& inStrm)
	//-------------------------
	{
		DefaultRead(inStrm);
	}

	template<>
	void Read<float>(INSTREAM& inStrm)
	//--------------------------------
	{
		if(m_BytesToRead == 8)
		{
			double temp;
			inStrm.read(reinterpret_cast<char*>(&temp), sizeof(double));
			m_rData = static_cast<float>(temp);
			m_LastReadinfo = READINFO(SNT_NOTE, "Read double to float");
		}
		else
			DefaultRead(inStrm);
	}

	template<>
	void Read<double>(INSTREAM& inStrm)
	//---------------------------------
	{
		if(m_BytesToRead == 4)
		{
			float temp;
			inStrm.read(reinterpret_cast<char*>(&temp), sizeof(float));
			m_rData = temp;
			m_LastReadinfo = READINFO(SNT_NOTE, "Read float to double");
		}
		else
			DefaultRead(inStrm);
	}

	void ReadAsInteger(INSTREAM& inStrm)
	//----------------------------------
	{
		if(m_BytesToRead > 8) {m_LastReadinfo = READINFO(SNT_WARNING, "No integer reading done; bytes to read > 8."); return;}

		const uint8 bc = (m_BytesToRead > sizeof(T)) ? sizeof(T) : static_cast<uint8>(m_BytesToRead);
		if(numeric_limits<T>::is_signed)
		{
			#pragma warning(disable:4146) //"unary minus operator applied to unsigned type, result still unsigned"
			int64 temp = 0;
			inStrm.read(reinterpret_cast<char*>(&temp) + 8-bc, bc);
			const bool isNegative = (temp < 0);
			if(isNegative && bc != 8) temp = -temp;
			if(bc != 8)
			{
				if(temp < 0)
				{
					temp = -Pow2x(8*bc-1);
					m_rData = static_cast<T>(temp);
				}
				else
				{
					temp >>= (8-bc)*8;
					m_rData = static_cast<T>(temp);
					if(isNegative && m_rData > 0) m_rData = -m_rData;
				}
			}
			else
				m_rData = static_cast<T>(temp);

			if(temp > (numeric_limits<T>::max)() || temp < (numeric_limits<T>::min)())
			{
				m_LastReadinfo = READINFO(SNT_WARNING, "Integer value was beyond range - read value may not be reasonable.");
			}
			#pragma warning(default:4146)
		}
		else
		{
			uint64 temp = 0;
			inStrm.read(reinterpret_cast<char*>(&temp), bc);
			m_rData = static_cast<T>(temp);
			if(temp > (numeric_limits<T>::max)())
				m_LastReadinfo = READINFO(SNT_WARNING, "Integer value was beyond range - read value may not be reasonable.");

		}
	}

protected:
	STREAMSIZE m_BytesToWrite;
	STREAMSIZE m_BytesToRead;
private:
	T& m_rData;

};


#define size_t_MAX (std::numeric_limits<size_t>::max)()


//Generic container streamer.
template<class TCONT>
class CContainerstreamer : public ABCSerializationStreamer
//========================================================
{
public:
	typedef TCONT CONT;
	typedef typename CONT::value_type DATATYPE;
	typedef typename CONT::size_type SIZETYPE;
	typedef void (*VALUETYPEWRITER)(OUTSTREAM&, const DATATYPE&, const size_t);
	typedef void (*VALUETYPEREADER)(INSTREAM&, DATATYPE&, const size_t);

	static CContainerstreamer* NewCustomWriter(const CONT& cont, VALUETYPEWRITER writer, SIZETYPE writecountlimit = size_t_MAX)
	//---------------------------------------------------------------
	{
		return new CContainerstreamer(cont, 0, writecountlimit, (writer != 0) ? writer : DefaultValuetypeWriter,
								DefaultValuetypeReader, sizeof(DATATYPE), sizeof(DATATYPE), OUTFLAG);

	}

	static CContainerstreamer* NewDefaultWriter(const CONT& cont, SIZETYPE writecountlimit = size_t_MAX, SIZETYPE bytesToWritePerElem = sizeof(DATATYPE))
	//----------------------------------------------------------------
	{
		return new CContainerstreamer(cont, 0, writecountlimit, DefaultValuetypeWriter,
								DefaultValuetypeReader, sizeof(DATATYPE), bytesToWritePerElem, OUTFLAG);
	}

	static CContainerstreamer* NewDefaultReader(CONT& cont, SIZETYPE maxReadCount, SIZETYPE bytesToReadPerElem = sizeof(DATATYPE))
	//---------------------------------------------------------------
	{
		return new CContainerstreamer(cont, maxReadCount, 0, DefaultValuetypeWriter, DefaultValuetypeReader, bytesToReadPerElem,
							0, INFLAG);
	}

	static CContainerstreamer* NewCustomReader(CONT& cont, SIZETYPE maxReadCount, VALUETYPEREADER reader)
	//---------------------------------------------------------------
	{
		return new CContainerstreamer(cont, maxReadCount, 0, DefaultValuetypeWriter, (reader != 0) ? reader : DefaultValuetypeReader, sizeof(DATATYPE),
							0, INFLAG);
	}

	static CContainerstreamer* NewDefaultReadWrite(CONT& cont, SIZETYPE maxReadCount, SIZETYPE writecountlimit, SIZETYPE bytesToReadPerElem = sizeof(DATATYPE), SIZETYPE bytesToWritePerElem = sizeof(DATATYPE))
	//---------------------------------------------------------------
	{
		return new CContainerstreamer(cont, maxReadCount, writecountlimit, DefaultValuetypeWriter, DefaultValuetypeReader, bytesToReadPerElem, bytesToWritePerElem, INFLAG|OUTFLAG);
	}

	static CContainerstreamer* NewCustomReadWrite(CONT& cont, SIZETYPE maxReadCount, SIZETYPE writecountlimit, VALUETYPEWRITER writer, VALUETYPEREADER reader)
	//---------------------------------------------------------------
	{
		return new CContainerstreamer(cont, maxReadCount, writecountlimit, (writer != 0) ? writer : DefaultValuetypeWriter, (reader != 0) ? reader : DefaultValuetypeReader, sizeof(DATATYPE), sizeof(DATATYPE), INFLAG|OUTFLAG);
	}


private:
	CContainerstreamer(const CONT& cont,
						SIZETYPE maxreadcount,
						SIZETYPE writecountlimit,
						VALUETYPEWRITER writer,
						VALUETYPEREADER reader,
						SIZETYPE bytesToReadPerElem,
						SIZETYPE bytesToWritePerElem,
						int flag)
						:
		ABCSerializationStreamer(flag),
		WriteValuetype(writer), ReadValuetype(reader), m_BytesToRead(bytesToReadPerElem),
			m_BytesToWrite(bytesToWritePerElem), m_WritecountLimit(writecountlimit), m_rCont((CONT&)cont),
			m_MaxReadCount(maxreadcount) {}



	virtual void ProWrite(OUTSTREAM& oStrm) const
	//-------------------------------------------
	{
		size_t size = min(m_WritecountLimit, m_rCont.size());
		const uint8 bc = GetNeededBytecount1248(size);
		//Two first bits in the sizedata tell how many bytes it is (1,2,4,8).
		const size_t sizeInstruction = (size << 2) |  Log2(bc);
		oStrm.write(reinterpret_cast<const char*>(&sizeInstruction), bc);
		typename CONT::const_iterator citer = m_rCont.begin();
		size_t counter = 0;
		for(;citer != m_rCont.end() && counter < size; citer++, counter++)
		{
			WriteValuetype(oStrm, *citer, m_BytesToWrite);
		}
	}

	virtual void ProRead(INSTREAM& iStrm, const uint64 datasize)
	//----------------------------------------------------------
	{
		if(datasize < 1)
		{
			m_LastReadinfo = READINFO(SNT_WARNING, "No container reading done: entrysize < 1");
			return;
		}
		size_t size = 0;
		iStrm.read(reinterpret_cast<char*>(&size), 1);
		const uint8 bc = Pow2xSmall(static_cast<uint8>(size & 3));
		if(bc > 1) iStrm.read(reinterpret_cast<char*>(&size)+1, min(bc-1, sizeof(size_t)-1));
		if(bc > sizeof(size_t))
		{
			iStrm.ignore(bc - sizeof(size_t));
			m_LastReadinfo = READINFO(SNT_WARNING, "Container sizetype is eight bytes, but only 4 bytes is read; possible loss of data");
		}
		size >>= 2;
		if(size > GetMaxReadCount())
		{
			m_LastReadinfo = READINFO(SNT_WARNING, "Container contains more elements than is allowed to be read; no reading done");
			return;
		}

		m_rCont.clear();
		Reserve(m_rCont, size); //Reserve memory if it is possible with given container.
		for(size_t i = 0; i<size; i++)
		{
			DATATYPE temp;
			ReadValuetype(iStrm, temp, m_BytesToRead);
			m_rCont.insert(m_rCont.end(), temp);
		}
	}

private:
	//Default memory reserver - does nothing.
	template <template<class A> class TTCONT, class TTDATA>
	void Reserve(TTCONT<TTDATA>&, const size_t) {}

	//For vector, reserves memory.
	template<class TTDATA>
	void Reserve(vector<TTDATA>& cont, const size_t s) {cont.reserve(s);}

	//For string, reserves memory.
	template<class TTDATA>
	void Reserve(std::basic_string<TTDATA>& str, const size_t s) {str.reserve(s);}

	static void DefaultValuetypeWriter(OUTSTREAM& oStrm, const DATATYPE& data, const size_t bytesToWrite)
		{if(bytesToWrite <= sizeof(DATATYPE)) oStrm.write(reinterpret_cast<const char*>(&data), static_cast<STREAMSIZE>(bytesToWrite));}

	static void DefaultValuetypeReader(INSTREAM& iStrm, DATATYPE& data, const size_t bytesToRead)
	{
		if(bytesToRead <= sizeof(DATATYPE))
		{
			if(bytesToRead < sizeof(DATATYPE)) memset(&data, 0, sizeof(DATATYPE));
			iStrm.read(reinterpret_cast<char*>(&data), static_cast<STREAMSIZE>(bytesToRead));
		}
	}

	size_t GetMaxReadCount() const {return m_MaxReadCount;}
	//------------------------------------------------------

private:
	VALUETYPEWRITER WriteValuetype;
	VALUETYPEREADER ReadValuetype;
	SIZETYPE m_BytesToRead; //To tell whether to read less/more bytes to data than its size
	SIZETYPE m_BytesToWrite;
	SIZETYPE m_WritecountLimit;  //To tell how many elements to write at max.
	SIZETYPE m_MaxReadCount;
	CONT& m_rCont;
};


inline CSerializationentry::~CSerializationentry() {if(m_Deletestreamer) delete m_pStreamer;}
//-------------------------------------------------------------------------------------

inline OFFTYPE CSerializationentry::Write(OUTSTREAM& oStrm) const {return m_pStreamer->Write(oStrm);}
//-------------------------------------------------------------------------------------------

inline OFFTYPE CSerializationentry::Read(INSTREAM& iStrm, const uint64 datasize) {return m_pStreamer->Read(iStrm, datasize);}
//--------------------------------------------------------------------------------------------


//Baseclass for objects implementing 'serializationinstructions' for an object.
//Read and write instructions are in the same class.
class ABCSerializationInstructions
//========================================
{
public:
	//-->Mainly UI methods
	//-------------
	public:
		ABCSerializationInstructions(const char* objectClassID, const size_t size, const uint64 version, const uint8 flags);
		virtual ~ABCSerializationInstructions();

		uint64 GetReadVersion() const {return m_VersionRead;}
		uint64 GetTimestamp() const {return m_Timestamp;}

		void AddEntry(CSerializationentry entry);

		void SetWritepropIncludeStartposInMap(const bool val) {m_StartposInMap = val;}
		void SetWritepropIncludeDatasizeentryInMap(const bool val) {m_IncludeDatasizeentryInMap = val;}
		void SetWritepropIncludeEntrydescriptionsInMap(const bool val) {m_IncludeEntrydescriptionsInMap = val;}
		void SetWritepropIncludeVersionNumeric(const bool val) {m_UseVersion = val;}
		void SetWritepropIgnoreLeadingNullsInIDComparison(const bool val) {m_IgnoreLeadingNulls = val;}
		void SetWritepropSetFixedsizeentry(const uint32 s) {m_Fixedsizeentries = s;}
		void SetWritepropObjectDescription(const string& str) {m_Objectdescription = str;}
		void SetWritepropUseTimestamp(bool newval) {m_Usetimestamp = newval;}

		void SetInstructionsVersion(uint64 newVersion) {m_VersionInstruction = newVersion;}
		bool AreReadInstructions() const {return (m_Flags & INFLAG) && !(m_Flags & OUTFLAG);}
		bool AreWriteInstructions() const {return (m_Flags & OUTFLAG) && !(m_Flags & INFLAG);}
	//<<- UI methods


	//-->Methods called (mainly) by serialisation when writing
		virtual size_t GetEntrycount() const = 0;
		bool GetIncludeEntrydescriptionsInMap() const {return m_IncludeEntrydescriptionsInMap;}
		bool GetUseVersion() const {return m_UseVersion;}
		bool GetUseVersionstring() const {return m_VersionStr.size() != 0;}
		virtual CSerializationentry* SetNextentry(CSerializationentry*&) = 0;
		virtual const CSerializationentry* SetNextentry(const CSerializationentry*&) const = 0;
		uint8 GetCustomIDInstructionbyte() const {return m_CustomIDInstructionbyte;}
		bool GetIncludeStartposInMap() const {return m_StartposInMap;}
		uint32 GetHasFixedsizeentries() const {return m_Fixedsizeentries;}
		bool GetUseTimestamp() const {return m_Usetimestamp;}
		bool GetIgnoreLeadingNullInIDComparison() const {return m_IgnoreLeadingNulls;}
		bool GetUseTwobyteDescriptionChar() {return false;} //Use one byte per character(for now all strings used are char strings so this is not modifiable).
		uint8 GetBytecount_ID() const {return m_BytesPerIDtype;}
		bool GetIncludeDatasizeEntryInMap() const {return m_IncludeDatasizeentryInMap;}
		const vector<char>& GetObjectClassID() const {return m_ObjectClassID;}
		bool AllowWriting() const {return (m_Flags & OUTFLAG) != 0;}
	//<--Methods called by serialisation when writing

	const string& GetVersionString() const {return m_VersionStr;}
	const string& GetObjectDescription() const {return m_Objectdescription;}


	//-->Methods called (mainly) by serialisation when reading
	public:
		//Return false if IDs match(at least to an extent that reading should continue), true otherwise.
		virtual bool CompareObjectClassID(const vector<char>& objectClassID) {return objectClassID != m_ObjectClassID;}

		//Return true if reading should be terminated.
		virtual bool SetReadVersion(uint64 v) {m_VersionRead = v; return false;}
		virtual bool SetVersionstring(const string& temp) {m_VersionStr = temp; return false;}

		void SetObjectDescription(const string& str) {m_Objectdescription = str;}

		//Returns pointer to entry if found, else returns null.
		//Ignoreleadingnulls instructs to ignore leading nulls if compared IDs do not have the same size.
		const CSerializationentry* Read(INSTREAM& iStrm, const uint64 size, const vector<char>& id,
			const bool ignoreLeadingnulls, const bool mapHasIDs,
			const uint64 entrycounter);

		uint64 GetMaxReadEntrycount() const {return m_MaxReadCount;}
		void SetTimestamp(uint64 ts) {m_Timestamp = ts;}
		bool AllowReading() const {return (m_Flags & INFLAG) != 0;}

	//<--Methods called by serialisation when reading

	//-->Methods called on both(reading/writing)
	uint64 GetInstructionVersion() const {return m_VersionInstruction;}



protected:
	//-->Pure virtuals.
	virtual CSerializationentry* GetpBeginentry() = 0;
	virtual const CSerializationentry* GetpBeginentry() const = 0;
	virtual void ProAddEntry(CSerializationentry entry) = 0;
	//<--Pure virtuals



private:
	void SetWritePropIDtypebytes(const uint8 bc);

protected:
	uint8 m_BytesPerIDtype;
	uint8 m_CustomIDInstructionbyte;
	bool m_IncludeEntrydescriptionsInMap;
	bool m_Usetimestamp;
	bool m_UseVersion;
	bool m_StartposInMap;
	bool m_IgnoreLeadingNulls;
	bool m_IncludeDatasizeentryInMap;
	uint64 m_VersionRead; //The version read in unserialization.
	uint64 m_VersionInstruction; //The version for serialization(or version comparison when reading)

	uint64 m_MaxReadCount;
	uint32 m_Fixedsizeentries; //If != 0, intructions to used fixed size entries whose size is given by the value.

	uint8 m_Flags; //To tells whether to allow reading(INFLAG) and writing(OUTFLAG)

	string m_Objectdescription; //When writing, writing this if chosen to write object description.
								//When reading, reading object description here if it exists.
	string m_VersionStr;

	vector<char> m_ObjectClassID;
	uint64 m_Timestamp;

};


class CSerializationInstructions : public ABCSerializationInstructions
//====================================================================
{
public:
	CSerializationInstructions(const string& objectClass, const uint64 version, const uint8 flags = INFLAG|OUTFLAG,
		const size_t entrycounthint = 5);

	size_t GetEntrycount() const {return m_Entries.size();}
	CSerializationentry* GetpBeginentry() {return (m_Entries.size() != 0) ? &m_Entries[0] : 0;}
	const CSerializationentry* GetpBeginentry() const {return (m_Entries.size() != 0) ? &m_Entries[0] : 0;}
	CSerializationentry* SetNextentry(CSerializationentry*& p);
	const CSerializationentry* SetNextentry(const CSerializationentry*& p) const;

protected:
	void ProAddEntry(CSerializationentry entry) {m_Entries.push_back(entry);}

private:
	vector<CSerializationentry> m_Entries;
};


inline void FloatToDoubleReader(INSTREAM& iStrm, double& data, const size_t)
//----------------------------------------------------------------------------------------
{
	float temp;
	iStrm.read(reinterpret_cast<char*>(&temp), sizeof(temp));
	data = temp;
}

inline void DoubleToFloatReader(INSTREAM& iStrm, float& data, const size_t)
//----------------------------------------------------------------------------------------
{
	double temp;
	iStrm.read(reinterpret_cast<char*>(&temp), sizeof(temp));
	data = static_cast<float>(temp);
}


} //namespace srlztn.


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
	outStream.write(str.c_str(), size);
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



#endif
