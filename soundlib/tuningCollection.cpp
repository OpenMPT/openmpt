#include "stdafx.h"
#include "tuningCollection.h"

//Serializations statics:
const CTuningCollection::SERIALIZATION_MARKER CTuningCollection::s_SerializationBeginMarker = 0x54435348; //ascii of TCSH(TuningCollectionSerialisationHeader) in hex.
const CTuningCollection::SERIALIZATION_MARKER CTuningCollection::s_SerializationEndMarker = 0x54435346; //ascii of TCSF(TuningCollectionSerialisationFooter) in hex.
const CTuningCollection::SERIALIZATION_MARKER CTuningCollection::s_SerializationVersion = 1;

const CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::SERIALIZATION_SUCCESS = false;
const CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::SERIALIZATION_FAILURE = true;

const string CTuningCollection::s_FileExtension = ".tc";

//BUG(?): These are not called before constructor for certain
//CTuningCollection objects - not good.
const CTuningCollection::CEDITMASK CTuningCollection::EM_ADD = 1; //0..01
const CTuningCollection::CEDITMASK CTuningCollection::EM_REMOVE = 2; //0..010
const CTuningCollection::CEDITMASK CTuningCollection::EM_ALLOWALL = 0xFFFF;
const CTuningCollection::CEDITMASK CTuningCollection::EM_CONST = 0;

/*
TODOS:
-Handle const-status better(e.g. status check in unserialization)
*/



CTuningCollection::CTuningCollection(const string& name) : m_Name(name)
//------------------------------------
{
	m_EditMask.set();
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

CTuning* CTuningCollection::GetTuning(const string& name)
//----------------------------------------------
{
	return FindTuning(name);
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::WriteTuningVector(ostream& outStrm) const
//------------------------------------------------------
{
	if(!outStrm.good())
		return SERIALIZATION_FAILURE;
	
	const size_t vs = m_Tunings.size();
	outStrm.write(reinterpret_cast<const char*>(&vs), sizeof(vs));
	for(CTITER i = m_Tunings.begin(); i != m_Tunings.end(); i++)
	{
		if((*i)->SerializeBinary(outStrm) == CTuning::SERIALIZATION_FAILURE)
			return SERIALIZATION_FAILURE;
	}

	if(outStrm.good())
		return SERIALIZATION_SUCCESS;
	else
		return SERIALIZATION_FAILURE;
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::ReadTuningVector(istream& inStrm)
//-------------------------------------
{
	if(!inStrm.good())
		return SERIALIZATION_FAILURE;
	size_t s = 0;
	inStrm.read(reinterpret_cast<char*>(&s), sizeof(s));

	//Todo: Handle the size limit better
	if(s > 1000)
		return SERIALIZATION_FAILURE;

	for(size_t i = 0; i<s; i++)
	{
		if(AddTuning(inStrm))
			return SERIALIZATION_FAILURE;
	}
	
	return SERIALIZATION_SUCCESS;
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::SerializeBinary(ostream& outStrm) const
//--------------------------------------------------------------
{
	//1. Serialization begin marker:
	outStrm.write(reinterpret_cast<const char*>(&s_SerializationBeginMarker), sizeof(s_SerializationBeginMarker));

	//2. Serialization version
	outStrm.write(reinterpret_cast<const char*>(&s_SerializationVersion), sizeof(s_SerializationVersion));

	//3. Name
	if(StringToBinaryStream(outStrm, m_Name))
		return SERIALIZATION_FAILURE;
	
	//4. Edit mask
	__int16 em = static_cast<__int16>(m_EditMask.to_ulong());
	outStrm.write(reinterpret_cast<const char*>(&em), sizeof(em));

	//5. Tunings
	WriteTuningVector(outStrm);

	//6. Serialization end marker
	outStrm.write(reinterpret_cast<const char*>(&s_SerializationEndMarker), sizeof(s_SerializationEndMarker));


	if(outStrm.good())
		return SERIALIZATION_SUCCESS;
	else
		return SERIALIZATION_FAILURE;
}

CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::SerializeBinary() const
//-------------------------------------------------------------------------------------
{
	if(m_SavefilePath.length() < 1)
		return SERIALIZATION_FAILURE;
	ofstream fout(m_SavefilePath.c_str(), ios::binary);
	if(!fout.good())
		return SERIALIZATION_FAILURE;

	if(SerializeBinary(fout) == SERIALIZATION_FAILURE)
	{
		fout.close();
		return SERIALIZATION_FAILURE;
	}
	fout.close();
	return SERIALIZATION_SUCCESS;
}

CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::UnSerializeBinary()
//-------------------------------------------------------------------------------------
{
	if(m_SavefilePath.length() < 1)
		return SERIALIZATION_FAILURE;
	ifstream fin(m_SavefilePath.c_str(), ios::binary);
	if(!fin.good())
		return SERIALIZATION_FAILURE;

	if(UnSerializeBinary(fin) == SERIALIZATION_FAILURE)
	{
		fin.close();
		return SERIALIZATION_FAILURE;
	}
	fin.close();
	return SERIALIZATION_SUCCESS;
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::UnSerializeBinary(istream& inStrm)
//---------------------------------------------------------
{
	SERIALIZATION_MARKER beginMarker = 1;
	SERIALIZATION_MARKER version = 1;
	SERIALIZATION_MARKER endMarker = 1;

    //1. Serialization begin marker:
	inStrm.read(reinterpret_cast<char*>(&beginMarker), sizeof(beginMarker));
	if(beginMarker != s_SerializationBeginMarker) return SERIALIZATION_FAILURE;

	//2. Serialization version
	inStrm.read(reinterpret_cast<char*>(&version), sizeof(version));
	if(version != s_SerializationVersion) return SERIALIZATION_FAILURE;

	//3. Name
	if(StringFromBinaryStream(inStrm, m_Name))
		return SERIALIZATION_FAILURE;

	//4. Editmask
	__int16 em = 0;
	inStrm.read(reinterpret_cast<char*>(&em), sizeof(em));
	//Not assigning the value yet, for if it is sets some aspect const,
	//further loading might fail.

    //5. Tunings
	if(ReadTuningVector(inStrm) == SERIALIZATION_FAILURE)
		return SERIALIZATION_FAILURE;

	//6. Serialization end marker
	inStrm.read(reinterpret_cast<char*>(&endMarker), sizeof(endMarker));
	if(endMarker != s_SerializationEndMarker) return SERIALIZATION_FAILURE;

	m_EditMask = em;

	if(inStrm.good()) return SERIALIZATION_SUCCESS;
	else return SERIALIZATION_FAILURE;
}	


bool CTuningCollection::Remove(const size_t i)
//--------------------------------------------
{
	//Behavior:
	//Moves tuning to carbage bin(m_DeletedTunings) so that
	//it gets deleted in destructor. This way
	//the tuning address remains valid until the destruction.
	
	if((m_EditMask & EM_REMOVE).any())
	{
		if(i >= m_Tunings.size())
			return true;

		m_DeletedTunings.push_back(m_Tunings[i]);
		m_Tunings.erase(m_Tunings.begin() + i);
		SerializeBinary();
		return false;
	}
	
	return true;
}


bool CTuningCollection::AddTuning(CTuning* const pT)
//--------------------------------------------------
{
	if((m_EditMask & EM_ADD).none())
		return true;

	if(pT == NULL)
		return true;

	m_Tunings.push_back(pT);

	if(m_SavefilePath.length() > 0)
	{
		ofstream fout(m_SavefilePath.c_str(), ios::binary);
		SerializeBinary(fout);
	}
	return false;
}


bool CTuningCollection::AddTuning(istream& inStrm)
//-------------------------------------------------
{
	if((m_EditMask & EM_ADD).none())
		return true;

	if(!inStrm.good()) return true;

	DWORD beginPos = inStrm.tellg();

	CTuning::SERIALIZATION_MARKER sbm;
	inStrm.read(reinterpret_cast<char*>(&sbm), sizeof(sbm));

	if(sbm == CTuningRTI::s_SerializationBeginMarker)
	{
		inStrm.seekg(beginPos);
		//Returning stream to point so that serialization marker is there
		//in tuning unserialization.

		CTuning* p = new CTuningRTI;
		if(p->UnSerializeBinary(inStrm) == CTuning::SERIALIZATION_FAILURE)
		{
			delete p; return true;
		}
		m_Tunings.push_back(p);
		return false;
	}

	if(!inStrm.good()) return true;
	else return false;
}
