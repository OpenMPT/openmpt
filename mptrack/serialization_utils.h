#ifndef SERIALIZATION_UTILS_H
#define SERIALIZATION_UTILS_H

#include <list>
#include <string>
#include <map>
#include <fstream>

using namespace std;

const size_t MAX_TUNING_NUM_DEFAULT = 1000;
bool ReadTuningMap(istream&, map<WORD, string>&, const size_t maxNum = MAX_TUNING_NUM_DEFAULT);


enum //Serialization Message
{
	SM_UNKNOWN = 0,
	SM_NOTFATAL = 1,
};

//==============================================
class CCharStreamBufFrom : public std::streambuf
//==============================================
{
private:
	DWORD GetStrmSize()
	{
		DWORD rv = egptr() - eback();
		return rv;
	}
public:
	CCharStreamBufFrom() {}
protected:
	
	streambuf* setbuf(char_type* buffer, std::streamsize count)
	{
		setg(buffer, buffer, buffer + count);
		return this;
	}
	
	//Gets called on seekg
	pos_type seekpos(pos_type sp, ios_base::openmode which = ios_base::in | ios_base::binary)
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
	pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode which = ios_base::in | ios_base::binary)
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
