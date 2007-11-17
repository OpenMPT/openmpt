#include "stdafx.h"
#include "tuningCollection.h"
#include <algorithm>
#include <bitset>

//Serializations statics:
const CTuningCollection::SERIALIZATION_VERSION CTuningCollection::s_SerializationVersion = 3;

/*
Version history:
	2->3: Serialization revamp(August 2007)
	1->2: Sizetypes of string serialisation from size_t(uint32)
		  to uint8. (March 2007)
*/

using namespace std;


const CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::SERIALIZATION_SUCCESS = false;
const CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::SERIALIZATION_FAILURE = true;

const string CTuningCollection::s_FileExtension = ".tc";

//BUG(?): These might not be called before constructor for certain
//CTuningCollection objects - not good.
const CTuningCollection::EDITMASK CTuningCollection::EM_ADD = 1; //0..01
const CTuningCollection::EDITMASK CTuningCollection::EM_REMOVE = 2; //0..010
const CTuningCollection::EDITMASK CTuningCollection::EM_ALLOWALL = 0xFFFF;
const CTuningCollection::EDITMASK CTuningCollection::EM_CONST = 0;

/*
TODOS:
-Handle const-status better(e.g. status check in unserialization)
*/



CTuningCollection::CTuningCollection(const string& name) : m_Name(name), m_EditMask(0xFFFF)
//------------------------------------
{
	if(m_Name.size() > GetNameLengthMax()) m_Name.resize(GetNameLengthMax());
}


CTuningCollection::~CTuningCollection()
//-------------------------------------
{
	for(TITER i = m_Tunings.begin(); i != m_Tunings.end(); i++)
	{
		delete *i;
	}
	m_Tunings.clear();

	for(TITER i = m_DeletedTunings.begin(); i != m_DeletedTunings.end(); i++)
	{
		delete *i;
	}
	m_DeletedTunings.clear();
}

CTuning* CTuningCollection::FindTuning(const string& name) const
//------------------------------------------------------
{
	for(size_t i = 0; i<m_Tunings.size(); i++)
	{
		if(m_Tunings[i]->GetName() == name) return m_Tunings[i];
	}
	return NULL;
}

size_t CTuningCollection::FindTuning(const CTuning* const pT) const
//-----------------------------------------------------------------
{
	CTITER citer = find(m_Tunings.begin(), m_Tunings.end(), pT);
		return citer - m_Tunings.begin();
}


CTuning* CTuningCollection::GetTuning(const string& name)
//----------------------------------------------
{
	return FindTuning(name);
}

const CTuning* CTuningCollection::GetTuning(const string& name) const
//-------------------------------------------------------------------
{
	return FindTuning(name);
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Serialize(ostream& outStrm) const
//--------------------------------------------------------------
{
	using namespace srlztn;
	const size_t tcount = m_Tunings.size();
	CSerializationInstructions si("TC", s_SerializationVersion, OUTFLAG, 2+tcount);
	si.AddEntry(CSerializationentry("0", CContainerstreamer<string>::NewDefaultWriter(m_Name, uint8_max), ""));
	si.AddEntry(CSerializationentry("1", new CBinarystreamer<uint16>(m_EditMask), ""));
	for(size_t i = 0; i<tcount; i++)
		si.AddEntry(CSerializationentry("2", new CTuningstreamer(*m_Tunings[i]), m_Tunings[i]->GetName()));

	si.SetWritepropIncludeStartposInMap(false);

	string logstr;
	CSSBSerialization ms(si);
	const CWriteNotification& wn = ms.Serialize(outStrm);
	if(wn.description.length() > 0)
		MessageBox(0, wn.description.c_str(), "Tuningcollection writemessages", MB_ICONINFORMATION);
	if(wn & SNT_FAILURE)
		return true;
	else
		return false;
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Serialize() const
//-------------------------------------------------------------------------------------
{
	if(m_SavefilePath.length() < 1)
		return SERIALIZATION_FAILURE;
	ofstream fout(m_SavefilePath.c_str(), ios::binary);
	if(!fout.good())
		return SERIALIZATION_FAILURE;

	if(Serialize(fout) == SERIALIZATION_FAILURE)
		return SERIALIZATION_FAILURE;

	return SERIALIZATION_SUCCESS;
}

CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Unserialize()
//-------------------------------------------------------------------------------------
{
	if(m_SavefilePath.length() < 1)
		return SERIALIZATION_FAILURE;
	ifstream fin(m_SavefilePath.c_str(), ios::binary);
	if(!fin.good())
		return SERIALIZATION_FAILURE;

	if(Unserialize(fin) == SERIALIZATION_FAILURE)
		return SERIALIZATION_FAILURE;

	return SERIALIZATION_SUCCESS;
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Unserialize(istream& inStrm)
//---------------------------------------------------------
{
	istream::pos_type startpos = inStrm.tellg();
	bool oldLoadingSuccess = false;
	if(UnserializeOLD(inStrm, oldLoadingSuccess))
	{
		//An old version was not recognised - trying new version.
		inStrm.clear();
		inStrm.seekg(startpos);
		using namespace srlztn;
		CSerializationInstructions si("TC", s_SerializationVersion, INFLAG, 3);
		si.AddEntry(CSerializationentry("0", CContainerstreamer<string>::NewDefaultReader(m_Name, uint8_max), ""));
		si.AddEntry(CSerializationentry("1", new CBinarystreamer<uint16>(m_EditMask), ""));
		si.AddEntry(CSerializationentry("2", new CTuningstreamer(*this), ""));
		CSSBSerialization ms(si);
		string logstr;
		ms.SetLogstring(logstr);
		const CReadNotification& rn = ms.Unserialize(inStrm);
		if(logstr.length() > 0)
			MessageBox(0, logstr.c_str(), "Tuningcollection loadmessages", MB_ICONINFORMATION);
		if(rn & SNT_FAILURE)
			return true;
		else
			return false;
	}
	else
	{
		if(oldLoadingSuccess)
			return false;
		else
			return true;
	}
}

//Returns false if stream content was recognised to be right kind of file(by beginmarker),
//else true, and sets bool parameter to true if loading was successful
bool CTuningCollection::UnserializeOLD(istream& inStrm, bool& loadingSuccessful)
//------------------------------------------------------------------------------
{
	//s_SerializationBeginMarker = 0x54435348;  //ascii of TCSH
	//s_SerializationEndMarker = 0x54435346; //ascii of TCSF(TuningCollectionSerialisationFooter) in hex.

	int32 beginMarker, endMarker, version;

	loadingSuccessful = false;

	//1. begin marker:
	inStrm.read(reinterpret_cast<char*>(&beginMarker), sizeof(beginMarker));
	if(beginMarker != 0x54435348) return true;

	//2. version
	inStrm.read(reinterpret_cast<char*>(&version), sizeof(version));
	if(version > 2 || version < 1)
		return false;

	//3. Name
	if(version < 2)
	{
		if(StringFromBinaryStream<uint32>(inStrm, m_Name, 256))
			return false;
	}
	else
	{
        if(StringFromBinaryStream<uint8>(inStrm, m_Name))
			return false;
	}

	//4. Editmask
	int16 em = 0;
	inStrm.read(reinterpret_cast<char*>(&em), sizeof(em));
	//Not assigning the value yet, for if it sets some property const,
	//further loading might fail.

    //5. Tunings
	{
		size_t s = 0;
		inStrm.read(reinterpret_cast<char*>(&s), sizeof(s));
		if(s > 50) return false;
		for(size_t i = 0; i<s; i++)
		{
			if(AddTuning(inStrm))
				return false;
		}
	}

	//6. End marker
	inStrm.read(reinterpret_cast<char*>(&endMarker), sizeof(endMarker));
	if(endMarker != 0x54435346) return false;

	m_EditMask = em;

	loadingSuccessful = true;

	return false;
}



bool CTuningCollection::Remove(const CTuning* pT)
//--------------------------------------------
{
	TITER iter = find(m_Tunings.begin(), m_Tunings.end(), pT);
	if(iter != m_Tunings.end())
		return Remove(iter);
	else
		return true;
}

bool CTuningCollection::Remove(TITER removable, bool moveToTrashBin)
//---------------------------------------------
{
	//Behavior:
	//By default, moves tuning to carbage bin(m_DeletedTunings) so that
	//it gets deleted in destructor. This way
	//the tuning address remains valid until the destruction of the collection.
	//Optinally only removing the pointer without deleting or moving
	//it to trashbin(e.g. when transferring tuning to other collection)
	if((m_EditMask & EM_REMOVE) != 0)
	{
		if(moveToTrashBin) m_DeletedTunings.push_back(*removable);
		m_Tunings.erase(removable);
		return false;
	}
	else
		return true;
}

bool CTuningCollection::Remove(const size_t i)
//--------------------------------------------
{
	if(i >= m_Tunings.size())
			return true;

	return Remove(m_Tunings.begin()+i);
}


bool CTuningCollection::AddTuning(CTuning* const pT)
//--------------------------------------------------
{
	if((m_EditMask & EM_ADD) == 0)
		return true;

	if(pT == NULL)
		return true;

	m_Tunings.push_back(pT);

	return false;
}


bool CTuningCollection::AddTuning(istream& inStrm, const bool ignoreEditmask)
//-------------------------------------------------
{
	if(!ignoreEditmask && (m_EditMask & EM_ADD) == 0)
		return true;

	if(!inStrm.good()) return true;

	CTuning* pT = CTuningRTI::UnserializeOLD(inStrm);
	if(pT == 0) pT = CTuning::Unserialize(inStrm);

	if(pT == 0)
		return true;
	else
	{
		m_Tunings.push_back(pT);
		return false;
	}
}

//Static
bool CTuningCollection::TransferTuning(CTuningCollection* pTCsrc, CTuningCollection* pTCdest, CTuning* pT)
//--------------------------------------------------------------------------------------------------------
{
	if(pTCsrc == NULL || pTCdest == NULL || pT == NULL)
		return true;

	if(pTCsrc == pTCdest)
		return true;

	size_t i = pTCsrc->FindTuning(pT);
	if(i >= pTCsrc->m_Tunings.size()) //Tuning not found?
		return true;

	if(pTCdest->AddTuning(pTCsrc->m_Tunings[i]))
		return true;

	if(pTCsrc->Remove(pTCsrc->m_Tunings.begin()+i, false))
		return true;

	return false;

}

string CTuningCollection::GetEditMaskString() const
//-------------------------------------------------
{
	std::bitset<16> mask(m_EditMask);
	return mask.to_string<char, char_traits<char>, allocator<char> >();
}
