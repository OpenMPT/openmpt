/*
 * tuningbase.cpp
 * --------------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "tuningbase.h"
#include "tuning.h"
#include "../common/mptIO.h"
#include "../common/serialization_utils.h"

#include <istream>
#include <ostream>


OPENMPT_NAMESPACE_BEGIN



/*
Version history:
	4->5: Lots of changes, finestep interpretation revamp, fileformat revamp.
	3->4: Changed sizetypes in serialisation from size_t(uint32) to
			smaller types (uint8, USTEPTYPE) (March 2007)
*/



const CTuningBase::SERIALIZATION_RETURN_TYPE CTuningBase::SERIALIZATION_SUCCESS = false;


const CTuningBase::SERIALIZATION_RETURN_TYPE CTuningBase::SERIALIZATION_FAILURE = true;


const char CTuningBase::s_FileExtension[5] = ".tun";


const CTuningBase::TUNINGTYPE CTuningBase::TT_GENERAL = 0; //0...00b

const CTuningBase::TUNINGTYPE CTuningBase::TT_GROUPGEOMETRIC = 1; //0...10b

const CTuningBase::TUNINGTYPE CTuningBase::TT_GEOMETRIC = 3; //0...11b



bool CTuningBase::SetRatio(const NOTEINDEXTYPE& s, const RATIOTYPE& r)
//--------------------------------------------------------------------
{
	if(GetType() != TT_GENERAL)
	{
		return true;
	}
	{
		if(ProSetRatio(s, r))
			return true;

		return false;
	}
}


CTuningBase::USTEPINDEXTYPE CTuningBase::SetFineStepCount(const USTEPINDEXTYPE& fs)
//---------------------------------------------------------------------------------
{
	VRPAIR vrp = GetValidityRange();

	if( (vrp.first > vrp.second)
		||
		(!IsStepCountRangeSufficient(fs, vrp))
	  ) return GetFineStepCount();
	else
	{
		ProSetFineStepCount(fs);
		return GetFineStepCount();
	}
}



CTuningBase::NOTESTR CTuningBase::GetNoteName(const NOTEINDEXTYPE& x, bool addOctave) const
//-----------------------------------------------------------------------------------------
{
	if(!IsValidNote(x)) return "";
	else return ProGetNoteName(x, addOctave);
}


CTuningBase::NOTESTR CTuningBase::ProGetNoteName(const NOTEINDEXTYPE& x, bool /*addOctave*/) const
//------------------------------------------------------------------------------------------------
{
	NNM_CITER i = m_NoteNameMap.find(x);
	if(i != m_NoteNameMap.end())
		return i->second;
	else
		return mpt::fmt::val(x);
}



bool CTuningBase::SetNoteName(const NOTEINDEXTYPE& n, const std::string& str)
//---------------------------------------------------------------------------
{
	{
		m_NoteNameMap[n] = str;
		return false;
	}
}




bool CTuningBase::ClearNoteName(const NOTEINDEXTYPE& n, const bool eraseAll)
//--------------------------------------------------------------------------
{
	{
		if(eraseAll)
		{
			m_NoteNameMap.clear();
			return false;
		}

		NNM_ITER iter = m_NoteNameMap.find(n);
		if(iter != m_NoteNameMap.end())
		{
			m_NoteNameMap.erase(iter);
			return false;
		}
		else
			return true;
	}
}



bool CTuningBase::Multiply(const RATIOTYPE& r)
//--------------------------------------------
{
	if(r <= 0)
		return true;

	//Note: Multiplying ratios by constant doesn't
	//change, e.g. 'geometricness' status.
	VRPAIR vrp = GetValidityRange();
	for(NOTEINDEXTYPE i = vrp.first; i<vrp.second; i++)
	{
		if(ProSetRatio(i, r*GetRatio(i)))
			return true;
	}
	return false;
}


bool CTuningBase::CreateGroupGeometric(const NOTEINDEXTYPE& s, const RATIOTYPE& r, const NOTEINDEXTYPE& startindex)
//-----------------------------------------------------------------------------------------------------------------
{
	if(s < 1 || r <= 0 || startindex < GetValidityRange().first)
		return true;

	std::vector<RATIOTYPE> v;
	v.reserve(s);
	for(NOTEINDEXTYPE i = startindex; i<startindex+s; i++)
		v.push_back(GetRatio(i));
	return CreateGroupGeometric(v, r, GetValidityRange(), startindex);
}


bool CTuningBase::CreateGroupGeometric(const std::vector<RATIOTYPE>& v, const RATIOTYPE& r, const VRPAIR vr, const NOTEINDEXTYPE ratiostartpos)
//---------------------------------------------------------------------------------------------------------------------------------------------
{
	{
		if(vr.first > vr.second || v.size() == 0) return true;
		if(ratiostartpos < vr.first || vr.second < ratiostartpos || static_cast<UNOTEINDEXTYPE>(vr.second - ratiostartpos) < static_cast<UNOTEINDEXTYPE>(v.size() - 1)) return true;
		if(!IsStepCountRangeSufficient(GetFineStepCount(), vr)) return true;
		for(size_t i = 0; i<v.size(); i++) {if(v[i] < 0) return true;}
		if(ProCreateGroupGeometric(v,r, vr, ratiostartpos))
			return true;
		else
		{
			m_TuningType = TT_GROUPGEOMETRIC;
			ProSetFineStepCount(GetFineStepCount());
			return false;
		}
	}
}



bool CTuningBase::CreateGeometric(const UNOTEINDEXTYPE& s, const RATIOTYPE& r, const VRPAIR vr)
//---------------------------------------------------------------------------------------------
{
	{
		if(vr.first > vr.second) return true;
		if(s < 1 || r <= 0) return true;
		if(ProCreateGeometric(s,r,vr))
			return true;
		else
		{
			m_TuningType = TT_GEOMETRIC;
			ProSetFineStepCount(GetFineStepCount());
			return false;
		}
	}
}




bool CTuningBase::ChangeGroupsize(const NOTEINDEXTYPE& s)
//-------------------------------------------------------
{
	if(s < 1)
		return true;

	if(m_TuningType == TT_GROUPGEOMETRIC)
		return CreateGroupGeometric(s, GetGroupRatio(), 0);

	if(m_TuningType == TT_GEOMETRIC)
		return CreateGeometric(s, GetGroupRatio());

	return true;
}



bool CTuningBase::ChangeGroupRatio(const RATIOTYPE& r)
//----------------------------------------------------
{
	if(r <= 0)
		return true;

	if(m_TuningType == TT_GROUPGEOMETRIC)
		return CreateGroupGeometric(GetGroupSize(), r, 0);

	if(m_TuningType == TT_GEOMETRIC)
		return CreateGeometric(GetGroupSize(), r);

	return true;
}



bool CTuningBase::DeserializeOLD(std::istream& inStrm)
//----------------------------------------------------
{
	char begin[8];
	MemsetZero(begin);
	inStrm.read(begin, sizeof(begin));
	if(std::memcmp(begin, "CT<sfs>B", 8)) return SERIALIZATION_FAILURE;

	int16 version = 0;
	mpt::IO::ReadIntLE<int16>(inStrm, version);
	if(version != 4) return SERIALIZATION_FAILURE;

	//Tuning name
	if(!mpt::IO::ReadSizedStringLE<uint8>(inStrm, m_TuningName))
		return SERIALIZATION_FAILURE;

	//Const mask
	int16 em = 0;
	mpt::IO::ReadIntLE<int16>(inStrm, em);

	//Tuning type
	int16 tt = 0;
	mpt::IO::ReadIntLE<int16>(inStrm, tt);
	m_TuningType = tt;

	//Notemap
	uint16 size = 0;
	mpt::IO::ReadIntLE<uint16>(inStrm, size);
	for(UNOTEINDEXTYPE i = 0; i<size; i++)
	{
		std::string str;
		int16 n = 0;
		mpt::IO::ReadIntLE<int16>(inStrm, n);
		if(!mpt::IO::ReadSizedStringLE<uint8>(inStrm, str))
			return SERIALIZATION_FAILURE;
		m_NoteNameMap[n] = str;
	}

	//End marker
	char end[8];
	MemsetZero(end);
	inStrm.read(end, sizeof(end));
	if(std::memcmp(end, "CT<sfs>E", 8)) return SERIALIZATION_FAILURE;

	return SERIALIZATION_SUCCESS;
}


OPENMPT_NAMESPACE_END
