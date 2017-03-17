/*
 * tuningCollection.cpp
 * --------------------
 * Purpose: Alternative sample tuning collection class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "tuningcollection.h"
#include "../common/mptIO.h"
#include "../common/serialization_utils.h"
#include <algorithm>
#include <bitset>
#include "../common/mptFileIO.h"
#include "Loaders.h"


OPENMPT_NAMESPACE_BEGIN


/*
Version history:
	2->3: Serialization revamp(August 2007)
	1->2: Sizetypes of string serialisation from size_t(uint32)
		  to uint8. (March 2007)
*/

/*
TODOS:
-Handle const-status better(e.g. status check in unserialization)
*/

const char CTuningCollection::s_FileExtension[4] = ".tc";

namespace CTuningS11n
{
	void WriteNoteMap(std::ostream& oStrm, const CTuning::NOTENAMEMAP& m);
	void ReadStr(std::istream& iStrm, std::string& str, const size_t);

	void ReadNoteMap(std::istream& iStrm, CTuning::NOTENAMEMAP& m, const size_t);
	void ReadRatioTable(std::istream& iStrm, std::vector<CTuningRTI::RATIOTYPE>& v, const size_t);
	void WriteStr(std::ostream& oStrm, const std::string& str);

	void ReadTuning(std::istream& iStrm, CTuningCollection& Tc, const size_t) {Tc.AddTuning(iStrm, true);}
	void WriteTuning(std::ostream& oStrm, const CTuning& t) {t.Serialize(oStrm);}
} // namespace CTuningS11n

using namespace CTuningS11n;


CTuningCollection::CTuningCollection(const std::string& name) : m_Name(name), m_EditMask(EM_ALLOWALL)
//---------------------------------------------------------------------------------------------------
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

CTuning* CTuningCollection::FindTuning(const std::string& name) const
//-------------------------------------------------------------------
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


CTuning* CTuningCollection::GetTuning(const std::string& name)
//------------------------------------------------------------
{
	return FindTuning(name);
}

const CTuning* CTuningCollection::GetTuning(const std::string& name) const
//------------------------------------------------------------------------
{
	return FindTuning(name);
}


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Serialize(std::ostream& oStrm) const
//--------------------------------------------------------------------------------------------------
{
	srlztn::SsbWrite ssb(oStrm);
	ssb.BeginWrite("TC", s_SerializationVersion);
	ssb.WriteItem(m_Name, "0", &WriteStr);
	ssb.WriteItem(m_EditMask, "1");

	const size_t tcount = m_Tunings.size();
	for(size_t i = 0; i<tcount; i++)
		ssb.WriteItem(*m_Tunings[i], "2", &WriteTuning);
	ssb.FinishWrite();
		
	if(ssb.GetStatus() & srlztn::SNT_FAILURE)
		return true;
	else
		return false;
}


#ifndef MODPLUG_NO_FILESAVE

CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Serialize() const
//-------------------------------------------------------------------------------
{
	if(m_SavefilePath.empty())
		return SERIALIZATION_FAILURE;
	mpt::ofstream fout(m_SavefilePath, std::ios::binary);
	if(!fout.good())
		return SERIALIZATION_FAILURE;

	if(Serialize(fout) == SERIALIZATION_FAILURE)
		return SERIALIZATION_FAILURE;

	return SERIALIZATION_SUCCESS;
}

CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Deserialize()
//---------------------------------------------------------------------------
{
	if(m_SavefilePath.empty())
		return SERIALIZATION_FAILURE;
	mpt::ifstream fin(m_SavefilePath, std::ios::binary);
	if(!fin.good())
		return SERIALIZATION_FAILURE;

	if(Deserialize(fin) == SERIALIZATION_FAILURE)
		return SERIALIZATION_FAILURE;

	return SERIALIZATION_SUCCESS;
}

#endif // MODPLUG_NO_FILESAVE


CTuningCollection::SERIALIZATION_RETURN_TYPE CTuningCollection::Deserialize(std::istream& iStrm)
//----------------------------------------------------------------------------------------------
{
	std::istream::pos_type startpos = iStrm.tellg();
	bool oldLoadingSuccess = false;

	if(DeserializeOLD(iStrm, oldLoadingSuccess))
	{	// An old version was not recognised - trying new version.
		iStrm.clear();
		iStrm.seekg(startpos);
		srlztn::SsbRead ssb(iStrm);
		ssb.BeginRead("TC", s_SerializationVersion);

		const srlztn::SsbRead::ReadIterator iterBeg = ssb.GetReadBegin();
		const srlztn::SsbRead::ReadIterator iterEnd = ssb.GetReadEnd();
		for(srlztn::SsbRead::ReadIterator iter = iterBeg; iter != iterEnd; iter++)
		{
			if (ssb.CompareId(iter, "0") == srlztn::SsbRead::IdMatch)
				ssb.ReadIterItem(iter, m_Name, &ReadStr);
			else if (ssb.CompareId(iter, "1") == srlztn::SsbRead::IdMatch)
				ssb.ReadIterItem(iter, m_EditMask);
			else if (ssb.CompareId(iter, "2") == srlztn::SsbRead::IdMatch)
				ssb.ReadIterItem(iter, *this, &ReadTuning);
		}

		if(ssb.GetStatus() & srlztn::SNT_FAILURE)
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
bool CTuningCollection::DeserializeOLD(std::istream& inStrm, bool& loadingSuccessful)
//-----------------------------------------------------------------------------------
{
	loadingSuccessful = false;

	//1. begin marker:
	int32 beginMarker = 0;
	mpt::IO::ReadIntLE<int32>(inStrm, beginMarker);
	if(beginMarker != MAGIC4BE('T','C','S','H')) return true;

	//2. version
	int32 version = 0;
	mpt::IO::ReadIntLE<int32>(inStrm, version);
	if(version > 2 || version < 1)
		return false;

	//3. Name
	if(version < 2)
	{
		if(!mpt::IO::ReadSizedStringLE<uint32>(inStrm, m_Name, 256))
			return false;
	}
	else
	{
		if(!mpt::IO::ReadSizedStringLE<uint8>(inStrm, m_Name))
			return false;
	}

	//4. Editmask
	int16 em = 0;
	mpt::IO::ReadIntLE<int16>(inStrm, em);
	//Not assigning the value yet, for if it sets some property const,
	//further loading might fail.

	//5. Tunings
	{
		uint32 s = 0;
		mpt::IO::ReadIntLE<uint32>(inStrm, s);
		if(s > 50) return false;
		for(size_t i = 0; i<s; i++)
		{
			if(AddTuning(inStrm))
				return false;
		}
	}

	//6. End marker
	int32 endMarker = 0;
	mpt::IO::ReadIntLE<int32>(inStrm, endMarker);
	if(endMarker != MAGIC4BE('T','C','S','F')) return false;

	m_EditMask = em;

	loadingSuccessful = true;

	return false;
}



bool CTuningCollection::Remove(const CTuning* pT)
//-----------------------------------------------
{
	TITER iter = find(m_Tunings.begin(), m_Tunings.end(), pT);
	if(iter != m_Tunings.end())
		return Remove(iter);
	else
		return true;
}

bool CTuningCollection::Remove(TITER removable, bool moveToTrashBin)
//------------------------------------------------------------------
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
	if((m_EditMask & EM_ADD) == 0 || m_Tunings.size() >= s_nMaxTuningCount)
		return true;

	if(pT == NULL)
		return true;

	m_Tunings.push_back(pT);

	return false;
}


bool CTuningCollection::AddTuning(std::istream& inStrm, const bool ignoreEditmask)
//--------------------------------------------------------------------------------
{
	if((!ignoreEditmask && (m_EditMask & EM_ADD) == 0) || m_Tunings.size() >= s_nMaxTuningCount)
		return true;

	if(!inStrm.good()) return true;

	CTuning* pT = CTuningRTI::DeserializeOLD(inStrm);
	if(pT == 0) pT = CTuningRTI::Deserialize(inStrm);

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

std::string CTuningCollection::GetEditMaskString() const
//------------------------------------------------------
{
	std::bitset<16> mask(m_EditMask);
	return mask.to_string<char, std::char_traits<char>, std::allocator<char> >();
}


OPENMPT_NAMESPACE_END
