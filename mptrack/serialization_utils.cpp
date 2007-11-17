#include "stdafx.h"
#include "serialization_utils.h"
#include <algorithm>
#include <ctime>
#include <list>
using std::list;

using namespace srlztn;



uint8 WriteAdaptive1234(OUTSTREAM& oStrm, const uint32 num)
//------------------------------------------------------
{
	const uint8 bc = GetNeededBytecount1234(num);
	const uint32 sizeInstruction = (num << 2) |  (bc - 1);
	Binarywrite<uint32>(oStrm, sizeInstruction, bc);
	return bc;
}

uint8 WriteAdaptive1248(OUTSTREAM& oStrm, const uint64& num)
//------------------------------------------------------
{
	const uint8 bc = GetNeededBytecount1248(num);
	const uint64 sizeInstruction = (num << 2) |  Log2(bc);
	Binarywrite<uint64>(oStrm, sizeInstruction, bc);
	return bc;
}



uint8 ReadAdaptive1234(INSTREAM& iStrm, uint32& val)
//------------------------------------------------
{
	val = 0;
	Binaryread<uint32>(iStrm, val, 1);
	const uint8 bc = 1 + static_cast<uint8>(val & 3);
	if(bc > 1) iStrm.read(reinterpret_cast<char*>(&val)+1, bc-1);
	val >>= 2;
	return bc;
}

uint8 ReadAdaptive1248(INSTREAM& iStrm, uint64& val)
//------------------------------------------------
{
	val = 0;
	Binaryread<uint64>(iStrm, val, 1);
	const uint8 bc = Pow2xSmall(static_cast<uint8>(val & 3));
	if(bc > 1) iStrm.read(reinterpret_cast<char*>(&val)+1, bc-1);
	val >>= 2;
	return bc;
}


//SNR <-> Serialization Note Read
const CReadNotification SNR_BADGIVEN_STREAM(1 | SNT_FAILURE, "Bad stream given for reading");
const CReadNotification SNR_BADSTREAM_AFTER_MAPHEADERSEEK(1<<1 | SNT_FAILURE, "Stream was bad after seeking map");
const CReadNotification SNR_BAD_MAPBEGINPOS(1<<2 | SNT_FAILURE, "Bad map begin position");
const CReadNotification SNR_NO_READING_ALLOWED_BY_INSTRUCTIONS(1<<3 | SNT_FAILURE, "Instructions do not allow reading");
const CReadNotification SNR_FRAMEWORKID_MISMATCH(1<<4 | SNT_FAILURE, "FrameworkID mismatch");
const CReadNotification SNR_FAULTY_HEADERSIZEINDICATOR(1<<5 | SNT_FAILURE, "Faulty headersize indicator");
const CReadNotification SNR_BADSTREAM_AT_MAP_READ(1<<6 | SNT_FAILURE, "Bad stream when reading map");
const CReadNotification SNR_MAPINFO_SIZE_ERROR(1<<7 | SNT_WARNING, "Read entrycount != mapcontainer size");
const CReadNotification SNR_ZEROENTRYCOUNT(1<<8 | SNT_NOTE, "Entrycount is informed to be zero");
const CReadNotification SNR_INSUFFICIENT_STREAM_OFFTYPE(1<<9 | SNT_FAILURE, "Map contains larger position than what OFFTYPE can handle");
const CReadNotification SNR_OBJECTCLASS_IDMISMATCH(1<<10 | SNT_FAILURE, "Objectclass ID mismatch");
const CReadNotification SNR_NO_ENTRYIDS_WITH_CUSTOMID_DEFINED(1<<11 | SNT_NOTE, "CustomID headerdata entry exists, but instructs to use no entryIDs");
const CReadNotification SNR_IGNORED_HEADERDATA(1<<12 | SNT_NOTE, "Ignored headerdata");
const CReadNotification SNR_LOADING_OBJECT_WITH_LARGER_VERSION(1<<13 | SNT_NOTE, "Object version > Instruction version");
const CReadNotification SNR_UNKNOWN_DATAENTRY(1<<14 | SNT_NOTE, "Unknown dataentry");
const CReadNotification SNR_TOO_MANY_ENTRIES_TO_READ(1 << 15 | SNT_FAILURE, "Object informs to contain more entries than is allowed to be read.");
const CReadNotification SNR_READ_TERMINATED_AFTER_READING_VERSIONNUMERIC(1 << 16 | SNT_NOTE, "Reading terminated after reading versionnumeric.");
const CReadNotification SNR_READ_TERMINATED_AFTER_READING_VERSIONSTRING(1 << 17 | SNT_NOTE, "Reading terminated after reading versionstring.");


const CWriteNotification SNW_BADGIVEN_STREAM(1 | SNT_FAILURE, "Bad stream given for writing");
const CWriteNotification SNW_DATASIZETYPE_OVERFLOW(1<<1 | SNT_FAILURE, "Datasize for a entry is larger than what can be handled by datasizetype.");
//1 << 2 unused
const CWriteNotification SNW_WRITING_NOT_ALLOWED(1 << 3 | SNT_FAILURE, "Instructions do not allow writing");
const CWriteNotification SNW_INSUFFICIENT_FIXEDSIZE(1<<4 | SNT_FAILURE, "An entry data was longer than fixedsize parameter");
const CWriteNotification SNW_WRITECOUNT_STREAMPOSINCREMENT_INCONSISTENCY(1<<5 | SNT_FAILURE, "Streamer informed it wrote number of bytes which was different from streamsize increment");
const CWriteNotification SNW_CHANGING_IDSIZE_WITH_FIXED_IDSIZESETTING(1<<6 | SNT_FAILURE, "IDsize changes while it should be fixed");
const CWriteNotification SNW_BADSTREAM_AT_END(1<<7 | SNT_FAILURE, "Stream was bad at the end.");
const CWriteNotification SNW_ZERO_IDINSTRUCTION_BYTE(1<<8 | SNT_WARNING, "Custom ID instruction byte is zero");


const char CSSBSerialization::s_EntryID[3] = {'2','2','8'};
const uint8 CSSBSerialization::s_DefaultFlagbyte = 0;
int32 CSSBSerialization::s_DefaultReadLogMask = static_cast<int32>(uint32_max); //Enable all notes
int32 CSSBSerialization::s_DefaultWriteLogMask = static_cast<int32>(uint32_max); //Enable all notes

const ABCSerializationStreamer::READINFO ABCSerializationStreamer::s_InfoNoReadimplementation(SNT_WARNING, "No readmethod implementation");
const ABCSerializationStreamer::WRITEINFO ABCSerializationStreamer::s_InfoNoWriteimplementation(SNT_WARNING, "No writemethod implementation");

string CSerializationNotification::ToString() const
//-------------------------------------------------
{
	string str;
	str.reserve(50);
	if(ID & SNT_PROGRESS) str += "Progress: ";
	if(ID & SNT_FAILURE) str += "Error: ";
	if(ID & SNT_NOTE) str += "Note: ";
	if(ID & SNT_WARNING) str += "Warning: ";
	if(str.size() == 0) str += "Unknown: ";
	str += "ID " + Stringify(ID) + "; ";
	str += description;
	return str;
}


bool CSSBSerialization::AddReadNote(const CReadNotification& note, const bool addToLog)
//-------------------------------------------------------------------------------------
{
	m_Readnotes.ID |= note.ID;
	if(addToLog && (note & m_Readlogmask))
	{
		AddToLog(note.ToString().c_str());
	}
	return (note & SNT_FAILURE) ? true : false;
}


bool CSSBSerialization::AddReadNote(const CReadNotification& note, const CMappinginfo* pMi)
//----------------------------------------------------------------------------------------
{
	if(note == SNR_UNKNOWN_DATAENTRY)
	{
		m_Readnotes.ID |= note.ID;
		if(note.ID & m_Readlogmask)
		{
			string logmsg = "Note: Unknown dataentry found; ";

			if(pMi == 0)
				logmsg += "No map entry found.";
			else
			{
				//Adding ID only if it does not contain null-character.
				if(std::find(pMi->id.begin(), pMi->id.end(), 0) == pMi->id.end())
				{
					logmsg += "id = ";
					const size_t s = logmsg.length();
					logmsg.resize(s + pMi->id.size(), '.');
					std::copy(pMi->id.begin(), pMi->id.end(), logmsg.begin()+s);
				}

				if(pMi->description.length() > 0)
					logmsg += "; entry description: " + pMi->description;
				else
					logmsg += "; there's no description for the entry";

				AddToLog(logmsg.c_str());
			}
		}
		return false;
	}
	else
	{
		CReadNotification n = note;
		if(pMi)
		{
			n.description += string("; ") + pMi->ToString();
		}
		return AddReadNote(n);
	}

}


bool CSSBSerialization::AddWriteNote(const CWriteNotification& note, const bool addToLog)
//------------------------------------------------
{
	m_Writenotes.ID |= note.ID;
	if(addToLog && (note & m_Writelogmask))
	{
		AddToLog(note.ToString().c_str());
	}

	return (note & SNT_FAILURE) ? true : false;
}

void CSSBSerialization::ResetReadstatus()
//---------------------------------------
{
	m_Readnotes.ID = 0;
	m_Readnotes.description.clear();
}

void CSSBSerialization::ResetWritestatus()
//----------------------------------------
{
	m_Writenotes.ID = 0;
	m_Writenotes.description.clear();
}



const CWriteNotification& CSSBSerialization::Serialize(OUTSTREAM& oStrm)
//-------------------------------------------------
{
	ResetWritestatus();

	if(!m_pSi->AllowWriting())
	{
		AddWriteNote(SNW_WRITING_NOT_ALLOWED);
		return m_Writenotes;
	}

	if(!oStrm.good() && AddWriteNote(SNW_BADGIVEN_STREAM))
		return m_Writenotes;

	const POSTYPE startpos = oStrm.tellp();

	//Framework identifier.
	oStrm.write(s_EntryID, sizeof(s_EntryID));

	//Objectclass('filetype') identifier
	{
	uint8 idsize = static_cast<uint8>(m_pSi->GetObjectClassID().size());
	Binarywrite<uint8>(oStrm, idsize);
	if(idsize > 0) oStrm.write(&m_pSi->GetObjectClassID()[0], idsize);
	}

	//Forming header.
	uint8 header = 0;

	//0,1 : Bytes per IDtype, 0,1,2,4
	header = (m_pSi->GetBytecount_ID() != 4) ? (m_pSi->GetBytecount_ID() & 3) : 3;

	//2   : Startpos in map?
	Setbit(header, 2, m_pSi->GetIncludeStartposInMap());

	//3   : Datasize in map?
	Setbit(header, 3, m_pSi->GetIncludeDatasizeEntryInMap());

	//4   : Has version numeric field?
	Setbit(header, 4, m_pSi->GetUseVersion());

	//5   : Has version string field?
	Setbit(header, 5, m_pSi->GetUseVersionstring());

	//6   : Bytes per descriptioncharacter(1,2)
	Setbit(header, 6, m_pSi->GetUseTwobyteDescriptionChar());

	//7   : Entrydescriptions in map?
	Setbit(header, 7, m_pSi->GetIncludeEntrydescriptionsInMap());

	uint16 IDbytecount = m_pSi->GetBytecount_ID();

	//Writing header
	Binarywrite<uint8>(oStrm, header);

	//Various flags to modify default settings(with default settings its not needed).
	uint8 tempU8 = 0;
	Setbit(tempU8, 0, m_pSi->GetCustomIDInstructionbyte() != 0);
	Setbit(tempU8, 1, m_pSi->GetHasFixedsizeentries() != 0);
	Setbit(tempU8, 2, m_pSi->GetObjectDescription().size() != 0);
	Setbit(tempU8, 3, m_pSi->GetUseTimestamp());
	Setbit(tempU8, 4, m_pSi->GetIgnoreLeadingNullInIDComparison());

	const uint8 flags = tempU8;
	if(flags != s_DefaultFlagbyte)
	{
		WriteAdaptive1234(oStrm, 2); //Headersize - now it is 2.
        Binarywrite<uint8>(oStrm, HEADERID_FLAGBYTE);
		Binarywrite<uint8>(oStrm, flags);
	}
	else
		WriteAdaptive1234(oStrm, 0);

	//Version(numeric)?
	if(Testbit(header, 4))
		WriteAdaptive1248(oStrm, m_pSi->GetInstructionVersion());

	//Version(string)?
	if(Testbit(header, 5))
	{
		Binarywrite<size_t>(oStrm, m_pSi->GetVersionString().size(), 1);
		oStrm.write(m_pSi->GetVersionString().c_str(), static_cast<uint8>(m_pSi->GetVersionString().size()));
	}

	//Custom IDbytecount?
	if(Testbit(flags, 0))
	{
		Binarywrite<uint8>(oStrm, m_pSi->GetCustomIDInstructionbyte());
		IDbytecount = m_pSi->GetCustomIDInstructionbyte() >> 1;
		if(IDbytecount == 0)
		{
			if(m_pSi->GetCustomIDInstructionbyte() & 1) IDbytecount = uint16_max; //Variablesize ID.
			else AddWriteNote(SNW_ZERO_IDINSTRUCTION_BYTE);
		}
	}

	//Fixedsize entries
	const uint32 fixedsizeentries = m_pSi->GetHasFixedsizeentries();
	if(Testbit(flags, 1))
		WriteAdaptive1234(oStrm, m_pSi->GetHasFixedsizeentries());

	//Object description?
	if(Testbit(flags, 2))
		WriteAdaptive12String(oStrm, m_pSi->GetObjectDescription());

	//Timestamp?
	if(Testbit(flags, 3))
		Binarywrite<uint64>(oStrm, time(0), 5);


	const bool hasDatastartposEntryInMap = m_pSi->GetIncludeStartposInMap() && fixedsizeentries == 0;
	const bool hasDatasizeEntryInMap = m_pSi->GetIncludeDatasizeEntryInMap() && fixedsizeentries == 0;


	//Entrycount
	WriteAdaptive1248(oStrm, m_pSi->GetEntrycount());

	//Mapping begin pos(reserve space - actual value is written after writing data)
	const POSTYPE mappingpositionindicatorfieldpos = oStrm.tellp();

	//Writing always 4 bytes even though the size is adaptive(changing this requires modifications elsewhere
	//as well.)
	Binarywrite<uint32>(oStrm, OFFTYPE_MAX);

	const bool hasMap = (IDbytecount != 0 || hasDatastartposEntryInMap || hasDatasizeEntryInMap || m_pSi->GetIncludeEntrydescriptionsInMap());

	std::vector<CMappinginfo> mappings;
	if(hasMap)
		mappings.resize(m_pSi->GetEntrycount(), CMappinginfo());

	//Writing data
	size_t counter = 0;
	const CSerializationentry* pEntry = 0;
	while(m_pSi->SetNextentry(pEntry))
	{
		const POSTYPE entrystartposInStream = oStrm.tellp();
		const POSTYPE entrystartposInObject = oStrm.tellp() - startpos;
		OFFTYPE bytecount = pEntry->Write(oStrm);
		const uint64 datasize = bytecount;
		if(static_cast<uint64>(oStrm.tellp()-entrystartposInStream) != bytecount && AddWriteNote(SNW_WRITECOUNT_STREAMPOSINCREMENT_INCONSISTENCY))
			return m_Writenotes;

		if(pEntry->GetStreamer().GetLastWriteinfo().description.size())
			AddWriteNote(pEntry->GetStreamer().GetLastWriteinfo());

		if(hasDatasizeEntryInMap && datasize > (uint32_max >> 2) && AddWriteNote(SNW_DATASIZETYPE_OVERFLOW))
			return m_Writenotes;

		if(fixedsizeentries)
		{
			if(static_cast<uint32>(bytecount) <= fixedsizeentries)
			{
				char fillc = 0;
				for(uint32 i = 0; i<fixedsizeentries-bytecount; i++)
					oStrm.write(&fillc, 1);
			}
			else
				if(AddWriteNote(SNW_INSUFFICIENT_FIXEDSIZE))
					return m_Writenotes;
		}
		if(!hasMap)
			continue;

		mappings[counter] = CMappinginfo(pEntry->GetID(), entrystartposInObject, datasize, pEntry->GetDescription());

		counter++;
	}

	//Mapping data
	const uint64 mapstartposRelative = oStrm.tellp() - startpos;
	if(hasMap) //Write map
	{
		vector<CMappinginfo>::const_iterator mapiter = mappings.begin();
		vector<CMappinginfo>::const_iterator mapiterEnd = mappings.end();
		for(mapiter; mapiter != mapiterEnd; mapiter++)
		{
			if((m_pSi->GetCustomIDInstructionbyte() & 3) == 0 && mapiter->id.size() != IDbytecount
				&& AddWriteNote(SNW_CHANGING_IDSIZE_WITH_FIXED_IDSIZESETTING))
				return m_Writenotes;

			if(IDbytecount > 0)
			{
				if((m_pSi->GetCustomIDInstructionbyte() & 1) != 0) //Variablesize ID?
					WriteAdaptive12(oStrm, static_cast<uint16>(mapiter->id.size()));

				if(mapiter->id.size() > 0)
					oStrm.write(&mapiter->id[0], static_cast<STREAMSIZE>(mapiter->id.size()));
			}

			if(hasDatastartposEntryInMap) //Startpos
				WriteAdaptive1248(oStrm, mapiter->startpos);
			if(hasDatasizeEntryInMap) //Entrysize
				WriteAdaptive1248(oStrm, mapiter->entrysize);

			if(Testbit(header, 7)) //Entry descriptions?
			{
				WriteAdaptive12String(oStrm, mapiter->description);
			}
		}
	}
	//Writing mappingstartposition.
	const POSTYPE endposStrm = oStrm.tellp();
	oStrm.seekp(mappingpositionindicatorfieldpos);
	Binarywrite<uint64>(oStrm, mapstartposRelative << 2 | 2, 4);
	oStrm.seekp(endposStrm); //Setting endpos to end of stream.
	if(oStrm.fail()) AddWriteNote(SNW_BADSTREAM_AT_END);
	return m_Writenotes;
}

const CReadNotification& CSSBSerialization::Unserialize(INSTREAM& iStrm)
//-------------------------------------------------
{
	ResetReadstatus();

	if(!m_pSi->AllowReading())
		{AddReadNote(SNR_NO_READING_ALLOWED_BY_INSTRUCTIONS); return m_Readnotes;}

	if(!iStrm.good() && AddReadNote(SNR_BADGIVEN_STREAM))
		return m_Readnotes;

	const POSTYPE startpos = iStrm.tellg();

	//Framework entryidentifier.
	{
	char temp[sizeof(s_EntryID)];
	Binaryread<char[sizeof(s_EntryID)]>(iStrm, temp);
	if(memcmp(temp, s_EntryID, sizeof(s_EntryID)) && AddReadNote(SNR_FRAMEWORKID_MISMATCH))
		return m_Readnotes;
	}

	//Filetype identifier
	{
	uint8 tempU8 = 0;
	Binaryread<uint8>(iStrm, tempU8);
	vector<char> buf(tempU8, 0);
	if(tempU8) iStrm.read(&buf[0], tempU8);
	if(m_pSi->CompareObjectClassID(buf) && AddReadNote(SNR_OBJECTCLASS_IDMISMATCH))
		return m_Readnotes;
	}

	//Header
	uint8 tempU8;
	Binaryread<uint8>(iStrm, tempU8);
	const uint8 header = tempU8;
	const uint8 bytesPerID = ((header & 3) == 3) ? 4 : (header & 3);
	const bool hasVersionnumeric = Testbit(header, 4);
	const bool hasVersionstring = Testbit(header, 5);
	const uint8 bytesPerDescchar = 1 + Testbit(header, 6);

	//Read headerdata size
	uint32 tempU32 = 0;
	ReadAdaptive1234(iStrm, tempU32);
	const uint32 headerdatasize = tempU32;

	//If headerdatasize != 0, read known headerdata and ignore rest.
	uint8 flagbyte = s_DefaultFlagbyte;
	if(headerdatasize > 0)
	{
		uint32 headerbytesread = 0;
		while(headerbytesread < headerdatasize)
		{
			headerbytesread += Binaryread<uint8>(iStrm, tempU8);
			if(tempU8 == HEADERID_FLAGBYTE)
			{
				headerbytesread += Binaryread<uint8>(iStrm, tempU8);
				flagbyte = tempU8;
				continue;
			}

			iStrm.ignore(headerdatasize - headerbytesread);
			headerbytesread = headerdatasize;
			AddReadNote(SNR_IGNORED_HEADERDATA);
			break;
		}
		if(headerdatasize != headerbytesread && AddReadNote(SNR_FAULTY_HEADERSIZEINDICATOR))
			return m_Readnotes;
	}

	uint64 tempU64 = 0;

	//Read version numeric if available.
	if(hasVersionnumeric)
	{
		ReadAdaptive1248(iStrm, tempU64);
		if(m_pSi->SetReadVersion(tempU64))
		{
			AddReadNote(SNR_READ_TERMINATED_AFTER_READING_VERSIONNUMERIC);
			return m_Readnotes;
		}
		if(tempU64 > m_pSi->GetInstructionVersion())
			AddReadNote(SNR_LOADING_OBJECT_WITH_LARGER_VERSION);
	}

	//Read version string if available.
	if(hasVersionstring)
	{
        Binaryread<uint8>(iStrm, tempU8);
		string temp;
		ReadBytes(iStrm, temp, tempU8);
		if(m_pSi->SetVersionstring(temp))
		{
			AddReadNote(SNR_READ_TERMINATED_AFTER_READING_VERSIONSTRING);
			return m_Readnotes;
		}
	}

	std::pair<bool, uint8> IDsize(0,bytesPerID); //First: Variable ID size?, second value is the size of fixed size ID - ignored if first entry is true.
	if(Testbit(flagbyte, 0)) //Custom ID?
	{
		Binaryread<uint8>(iStrm, tempU8);
		IDsize.first = (tempU8 & 1) != 0;
		IDsize.second = tempU8 >> 1;
		if(IDsize.first == 0 && IDsize.second == 0)
			AddReadNote(SNR_NO_ENTRYIDS_WITH_CUSTOMID_DEFINED);
	}

	uint32 fixedsizeentries = 0; //If != 0, means that corresponding fixed size entries expected.
	if(Testbit(flagbyte, 1)) //Fixedsize entries?
		ReadAdaptive1234(iStrm, fixedsizeentries);


	if(Testbit(flagbyte, 2)) //Object description?
	{
		uint16 size = 0;
		ReadAdaptive12(iStrm, size);
		if(bytesPerDescchar == 1)
		{
			string temp;
			ReadBytes(iStrm, temp, size);
			m_pSi->SetObjectDescription(temp);
		}
		else
		{
            iStrm.ignore(size * bytesPerDescchar);
		}
	}

    if(Testbit(flagbyte, 3)) //Simple timestamp
	{
		Binaryread<uint64>(iStrm, tempU64, 5);
		m_pSi->SetTimestamp(tempU64);
	}

	//Read entrycount
	ReadAdaptive1248(iStrm, tempU64);
	const uint64 entrycount = tempU64;
	if(entrycount == 0)
		AddReadNote(SNR_ZEROENTRYCOUNT);

	if(entrycount > m_pSi->GetMaxReadEntrycount())
	{
		AddReadNote(SNR_TOO_MANY_ENTRIES_TO_READ);
		return m_Readnotes;
	}

	//Read map position start
	ReadAdaptive1248(iStrm, tempU64);
	if(tempU64 > OFFTYPE_MAX && AddReadNote(SNR_INSUFFICIENT_STREAM_OFFTYPE))
		return m_Readnotes;
	const uint64 mapbeginpos = tempU64;
	if(mapbeginpos < iStrm.tellg() - startpos && AddReadNote(SNR_BAD_MAPBEGINPOS))
		return m_Readnotes;

	const POSTYPE databeginpos = iStrm.tellg();


	std::list<CMappinginfo> mapinfos;
	const bool hasStartposInMap = Testbit(header, 2) && fixedsizeentries == 0;
	const bool hasSizedataInMap = Testbit(header, 3) && fixedsizeentries == 0;
	const bool hasIDsInMap = (IDsize.first == true || IDsize.second != 0);
	const bool hasEntrydescriptionsInMap = Testbit(header, 7);
	const bool hasMap = hasIDsInMap || hasStartposInMap || hasSizedataInMap || hasEntrydescriptionsInMap;

	//Goto map if it exists
	if(hasMap)
	{
		Seekg(iStrm, startpos, mapbeginpos);

		if(iStrm.fail() && AddReadNote(SNR_BADSTREAM_AFTER_MAPHEADERSEEK))
			return m_Readnotes;

		//Read map
		for(uint64 i = 0; i<entrycount; i++)
		{
			if(iStrm.fail() && AddReadNote(SNR_BADSTREAM_AT_MAP_READ))
				return m_Readnotes;

			CMappinginfo mi;
			if(IDsize.first == true) //Variablesize ID
			{
				uint16 idsize;
				ReadAdaptive12(iStrm, idsize);
				mi.id.resize(idsize);
				iStrm.read(&mi.id[0], idsize);
			}
			else //Fixed size ID, or not id at all.
			{
				if(IDsize.second != 0)
				{
					mi.id.resize(IDsize.second);
					iStrm.read(&mi.id[0], IDsize.second);
				}
			}

			if(hasStartposInMap)
			{
				ReadAdaptive1248(iStrm, tempU64);
				mi.startpos = tempU64;
				if(mi.startpos > OFFTYPE_MAX && AddReadNote(SNR_INSUFFICIENT_STREAM_OFFTYPE))
					return m_Readnotes;

			}
			if(hasSizedataInMap) //Using datasize field in map?
			{
				ReadAdaptive1248(iStrm, tempU64);
				mi.entrysize = tempU64;
			}


			if(hasEntrydescriptionsInMap) //Map has entrydescriptions?
			{
				uint16 size = 0;
				ReadAdaptive12(iStrm, size);

				if(bytesPerDescchar != 1)
				{
					iStrm.ignore(size * bytesPerDescchar); //Ignoring.
					mi.description = "Note: Entrydescription had twobytecharacters - ignored.";
				}
				else
				{
					string temp;
					ReadBytes(iStrm, temp, size);
					mi.description = temp;
				}
			}

			mapinfos.push_back(mi);
		}
	}
	if(hasMap && mapinfos.size() != entrycount && AddReadNote(SNR_MAPINFO_SIZE_ERROR))
		return m_Readnotes;

	const uint64 mapendposRelative = (hasMap) ? iStrm.tellg()-startpos : mapbeginpos;
	//If no map exists, tells the position of first byte after the data(i.e. outside
	//the object).

	//Goto data and read it
	list<CMappinginfo>::iterator iter = mapinfos.begin();
	const list<CMappinginfo>::iterator iterend = mapinfos.end();
	iStrm.seekg(databeginpos);
	const bool ignoreleadingnullsInIDComparison = Testbit(flagbyte, 4);
	for(uint32 counter = 0; counter < entrycount; counter++)
	{
		POSTYPE dataitemStartposStrm = iStrm.tellg();
		if(!hasStartposInMap && !hasSizedataInMap && fixedsizeentries == 0) //No dataposition hints in map - trying plain 'in order'-read.
		{
			const CSerializationentry* pE = m_pSi->Read(iStrm, invalidDatasize, hasMap ? iter->id : vector<char>(), ignoreleadingnullsInIDComparison, hasIDsInMap, counter);
			if(pE && pE->GetReadnote().size() > 0 && m_Readlogmask & SNT_NOTE) AddToLog(pE->GetReadnote().c_str());
			if(pE)
				AddReadNote(pE->GetStreamer().GetLastReadinfo(), (hasMap) ? &(*iter) : 0);
			else
				AddReadNote(SNR_UNKNOWN_DATAENTRY, (hasMap) ? &(*iter) : 0);

			if(hasMap) iter++;
			continue;
		}

		uint64 datasize = invalidDatasize;
		if(fixedsizeentries > 0)
		{
			Seekg(iStrm, startpos + databeginpos, counter * fixedsizeentries);
			datasize = fixedsizeentries;
		}
		else
		{
			if(hasMap && iter->startpos != 0) Seekg(iStrm, startpos, iter->startpos);
			if(hasMap) datasize = iter->entrysize;
		}

		const CSerializationentry* pE = 0;
		if(!hasMap)
		{
			pE = m_pSi->Read(iStrm, datasize, vector<char>(), ignoreleadingnullsInIDComparison, hasIDsInMap, counter);
		}
		else
		{
			pE = m_pSi->Read(iStrm, datasize, iter->id, ignoreleadingnullsInIDComparison, hasIDsInMap, counter);
		}

		if(pE && pE->GetReadnote().size() > 0 && m_Readlogmask & SNT_NOTE) AddToLog(("Entry note: " + pE->GetReadnote()).c_str());
		if(pE) AddReadNote(pE->GetStreamer().GetLastReadinfo(), (hasMap) ? &(*iter) : 0);
		else AddReadNote(SNR_UNKNOWN_DATAENTRY, (hasMap) ? &(*iter) : 0);

		iStrm.clear();

		if(!hasStartposInMap && hasSizedataInMap)
			Seekg(iStrm, dataitemStartposStrm, iter->entrysize);

		if(hasMap) iter++;


	}
	Seekg(iStrm, startpos, mapendposRelative); //Returning stream to the end of the objectstream.
	return m_Readnotes;
}

void CSSBSerialization::AddToLog(const char* str)
//-------------------------------------------
{
	if(m_pLogstring) *m_pLogstring += str + string("\n");
}


void CSSBSerialization::Seekg(INSTREAM& iStrm, const POSTYPE& startpos, int64 offset)
//--------------------------------------------------------------------------------------------------
{
	int64 curoff = startpos - iStrm.tellg() + offset;
	if(curoff <= OFFTYPE_MAX && curoff >= OFFTYPE_MIN)
	{
		iStrm.seekg(static_cast<OFFTYPE>(curoff), CURPOS);
	}
	else
	{
		if(offset >= 0)
		{
			while(offset > OFFTYPE_MAX) {iStrm.seekg(OFFTYPE_MAX, CURPOS); offset -= OFFTYPE_MAX;}
			iStrm.seekg(static_cast<OFFTYPE>(offset), CURPOS);
		}
		else
		{
			while(offset < OFFTYPE_MIN) {iStrm.seekg(OFFTYPE_MIN, CURPOS); offset -= OFFTYPE_MIN;}
			iStrm.seekg(static_cast<OFFTYPE>(offset), CURPOS);
		}
	}
}



ABCSerializationInstructions::ABCSerializationInstructions(const char* objectClassID,
								const size_t size, const uint64 version, const uint8 flags) :
	m_CustomIDInstructionbyte(0),
	m_BytesPerIDtype(1),
	m_IncludeEntrydescriptionsInMap(true),
	m_Usetimestamp(false),
	m_UseVersion(true),
	m_StartposInMap(false),
	m_IncludeDatasizeentryInMap(true),
	m_IgnoreLeadingNulls(false),
	m_VersionRead(0),
	m_VersionInstruction(version),
	m_Fixedsizeentries(0),
	m_Flags(flags),
	m_Timestamp(0),
	m_MaxReadCount(1000)
//------------------------------------------------------------------
{
	m_ObjectClassID.resize(size);
	std::copy(objectClassID, objectClassID+size, m_ObjectClassID.begin());
}


ABCSerializationInstructions::~ABCSerializationInstructions() {}
//------------------------------------------------------------------



const srlztn::CSerializationentry* ABCSerializationInstructions::Read(INSTREAM& iStrm,
							const uint64 size, const vector<char>& id,
							const bool ignoreLeadingnulls, const bool mapHasIDs,
							const uint64 entrycounter)
//---------------------------------------------------------------
{
	CSerializationentry* p = 0;
	for(size_t i = 0; SetNextentry(p); i++)
	{
		if(!mapHasIDs) //Trying reading in order if no map ids.
		{
			if(i == entrycounter)
			{
				p->Read(iStrm, size);
				return p;
			}
			else continue;
		}

		if(!ignoreLeadingnulls)
		{
			if(p->GetID() == id)
			{
				p->Read(iStrm, size);
				return p;
			}
		}
		else //Ignore leading nulls when comparing.
		{
			const vector<char>& largerID = (p->GetID().size() > id.size()) ? p->GetID() : id;
			const vector<char>& smallerID = (p->GetID().size() > id.size()) ? id : p->GetID();
			bool gotonext = false;
			for(size_t j = smallerID.size(); j<largerID.size(); j++)
			{
				if(largerID[j] != 0)
					{gotonext = true; break;}
			}
			if(gotonext) continue;
			for(size_t j = 0; j<smallerID.size(); j++)
			{
				if(largerID[j] != smallerID[j])
					{gotonext = true; break;}
			}
			if(gotonext) continue;
			p->Read(iStrm, size);
			return p;
		}
	}
	return 0;
}

void ABCSerializationInstructions::AddEntry(CSerializationentry entry)
//----------------------------------------------------------
{
	if(AreWriteInstructions() && entry.GetDescription().size() > 0) SetWritepropIncludeEntrydescriptionsInMap(true);
	CSerializationentry* p = 0;
	SetNextentry(p);
	if(p == 0) //First entry?
	{
		if(m_Flags & OUTFLAG)
		{
			SetWritePropIDtypebytes((entry.GetID().size() > uint8_max) ? 255 : static_cast<uint8>(entry.GetID().size()));
			if(entry.GetDescription().size() == 0) SetWritepropIncludeEntrydescriptionsInMap(false);
		}
		ProAddEntry(entry);
		return;
	}

	if(m_Flags & OUTFLAG)
	{
		uint16 maxIDbytes = static_cast<uint16>(p->m_Id.size());

		bool bytecountvaries = false;
		while(SetNextentry(p))
		{
			if(!bytecountvaries && p->m_Id.size() != maxIDbytes)
			{
				bytecountvaries = true;
			}
			maxIDbytes = max(maxIDbytes, static_cast<uint16>(p->m_Id.size()));
		}
		if(entry.GetID().size() != maxIDbytes)
		{
			if(entry.GetID().size() > maxIDbytes) maxIDbytes = static_cast<uint16>(entry.GetID().size());
			bytecountvaries = true;
		}
		if(bytecountvaries)
		{
			m_CustomIDInstructionbyte = 0;
			if(maxIDbytes <= (uint16_max >> 1))
				m_CustomIDInstructionbyte = 1;
			else
				return; //Should never reach here
		}
		else
			SetWritePropIDtypebytes((maxIDbytes > uint8_max) ? 255 : static_cast<uint8>(maxIDbytes));
	}
	ProAddEntry(entry);
}






void ABCSerializationInstructions::SetWritePropIDtypebytes(const uint8 bc)
//-------------------------------------------------------------
{
	m_BytesPerIDtype = bc;
	if(m_BytesPerIDtype > 2 && m_BytesPerIDtype != 4) //Using custom IDbytecount.
	{
		if(bc >> 7 != 0) m_CustomIDInstructionbyte = 1; //Too large fixed size ID - setting variable IDsize
		else m_CustomIDInstructionbyte = bc << 1;
	}

}

CSerializationInstructions::CSerializationInstructions(const string& objectClass,
			const uint64 version, const uint8 flags, const size_t entrycounthint)
	: ABCSerializationInstructions(objectClass.c_str(), objectClass.size(), version, flags)
//-----------------------------------------------------------------------------------------
{
	m_Entries.reserve(entrycounthint);
}





CSerializationentry* CSerializationInstructions::SetNextentry(CSerializationentry*& p)
//---------------------------------------------------------------------
{
	if(m_Entries.size() == 0) return 0;
	if(p == 0) return p = &m_Entries[0];

	if(static_cast<size_t>(++p - &m_Entries[0]) < m_Entries.size())
		return p;
	else
		return 0;
}


const CSerializationentry* CSerializationInstructions::SetNextentry(const CSerializationentry*& p) const
//--------------------------------------------------------------------------------------------
{
	if(m_Entries.size() == 0) return 0;
	if(p == 0) return p = &m_Entries[0];

	if(static_cast<size_t>(++p - &m_Entries[0]) < m_Entries.size())
		return p;
	else
		return 0;
}


