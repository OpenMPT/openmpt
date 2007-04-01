#ifndef SERIALIZATION_UTILS_H
#define SERIALIZATION_UTILS_H

#include <list>
#include <string>
#include <map>
#include <fstream>
#include "misc_util.h"



using namespace std;

const size_t MAX_TUNING_NUM_DEFAULT = 1000;

template<class TUNNUMTYPE, class STRSIZETYPE>
bool ReadTuningMap(istream& fin, map<WORD, string>& shortToTNameMap, const size_t maxNum = MAX_TUNING_NUM_DEFAULT)
//----------------------------------------------------------------------------------------
{
	//In first versions: SIZETYPE1 == SIZETYPE2 == size_t == uint64.
	typedef map<WORD, string> MAP;
	typedef MAP::iterator MAP_ITER;
	TUNNUMTYPE numTuning = 0;
	fin.read(reinterpret_cast<char*>(&numTuning), sizeof(numTuning));
	if(numTuning > maxNum)
	{
		fin.setstate(ios::failbit);
		return true;
	}

	for(size_t i = 0; i<numTuning; i++)
	{
		string temp;
		uint16 ui;
		if(StringFromBinaryStream<STRSIZETYPE>(fin, temp))
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


enum //Serialization Message
{
	SM_UNKNOWN = 0,
	SM_NOTFATAL = 1,
};

//==============================================
class CCharBufferStreamIn : public std::streambuf
//==============================================
{
private:
	DWORD GetStrmSize()
	{
		DWORD rv = egptr() - eback();
		return rv;
	}
public:
	CCharBufferStreamIn() {}
protected:
	
	streambuf* setbuf(char_type* buffer, std::streamsize count)
	{
		setg(buffer, buffer, buffer + count);
		return this;
	}
	
	//Gets called on seekg
	pos_type seekpos(pos_type sp, ios_base::openmode) // which = ios_base::in | ios_base::binary)
	{
		DWORD offset = static_cast<DWORD>(sp);
		if(offset >= 0 && offset < GetStrmSize())
		{
			setg(eback(), eback()+sp, egptr());
			return sp;
		}
		else 
			return -1;
	}

	//Gets called on tellg
	pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode) // which = ios_base::in | ios_base::binary)
	{
		DWORD beginOffset = 0;
		if(way == ios::cur)
			beginOffset = gptr() - eback();
		if(way == ios::beg)
			beginOffset = 0;
		if(way == ios::end)
			beginOffset = GetStrmSize();
		
		const int resultOff = static_cast<int>(beginOffset) + off;
		if(resultOff >= 0 && resultOff < static_cast<int>(GetStrmSize()))
			return resultOff;
		else 
			return -1;
	}
};



//==========================
class CSerializationMessager
//==========================
{
private:
	class CMessage
	{
	public:
		BYTE m_Type; //(minor, major, note...)
		string m_Message;
		CMessage(const string& str, const BYTE type = SM_UNKNOWN) : m_Type(type), m_Message(str) {}
	};

	list<CMessage> m_Messages;


public:
	void Add(const string& str)
	{
		m_Messages.push_back(CMessage(str));
	}

	void Add(const string& str, const BYTE type)
	{
		m_Messages.push_back(CMessage(str, type));
	}

	size_t GetCount () const {return m_Messages.size();}
};

#endif
