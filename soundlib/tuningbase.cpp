/*
 * tuningbase.cpp
 * --------------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#define TUNINGBASE_CPP
#include "tuningbase.h"
#include "../mptrack/serialization_utils.h"

#ifdef TUNINGBASE_H


TEMPLATEDEC
const char* CTUNINGBASE::s_TuningDescriptionGeneral = "No ratio restrictions";

TEMPLATEDEC
const char* CTUNINGBASE::s_TuningDescriptionGroupGeometric = "Ratio of ratios with distance of 'groupsize' is constant.";

TEMPLATEDEC
const char* CTUNINGBASE::s_TuningDescriptionGeometric = "Ratio of successive ratios is constant.";

TEMPLATEDEC
const char* CTUNINGBASE::s_TuningTypeStrGeneral = "\"General\"";

TEMPLATEDEC
const char* CTUNINGBASE::s_TuningTypeStrGroupGeometric = "\"GroupGeometric\"";

TEMPLATEDEC
const char* CTUNINGBASE::s_TuningTypeStrGeometric = "\"Geometric\"";

TEMPLATEDEC
const TYPENAME CTUNINGBASE::SERIALIZATION_VERSION CTUNINGBASE::s_SerializationVersion(5);
/*
Version history:
	4->5: Lots of changes, finestep interpretation revamp, fileformat revamp.
	3->4: Changed sizetypes in serialisation from size_t(uint32) to
			smaller types (uint8, USTEPTYPE) (March 2007)
*/


TEMPLATEDEC
TYPENAME CTUNINGBASE::MESSAGEHANDLER CTUNINGBASE::MessageHandler = &(CTUNINGBASE::DefaultMessageHandler);

TEMPLATEDEC
const TYPENAME CTUNINGBASE::SERIALIZATION_RETURN_TYPE CTUNINGBASE::SERIALIZATION_SUCCESS = false;

TEMPLATEDEC
const TYPENAME CTUNINGBASE::SERIALIZATION_RETURN_TYPE CTUNINGBASE::SERIALIZATION_FAILURE = true;

TEMPLATEDEC
const TCHAR CTUNINGBASE::s_FileExtension[5] = TEXT(".tun");

TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_RATIOS = 1; //1b
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_NOTENAME = 1 << 1; //10b
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_TYPE = 1 << 2; //100b
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_NAME = 1 << 3; //1000b
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_FINETUNE = 1 << 4; //10000b
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_VALIDITYRANGE = 1 << 5; //100000b

TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_ALLOWALL = 0xFFFF; //All editing allowed.
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_EDITMASK = 0x8000; //Whether to allow modifications to editmask.
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_CONST = 0x8000;  //All editing except changing const status disable.
TEMPLATEDEC
const TYPENAME CTUNINGBASE::EDITMASK CTUNINGBASE::EM_CONST_STRICT = 0; //All bits are zero - even the const status can't be changed.


TEMPLATEDEC
const TYPENAME CTUNINGBASE::TUNINGTYPE CTUNINGBASE::TT_GENERAL = 0; //0...00b
TEMPLATEDEC
const TYPENAME CTUNINGBASE::TUNINGTYPE CTUNINGBASE::TT_GROUPGEOMETRIC = 1; //0...10b
TEMPLATEDEC
const TYPENAME CTUNINGBASE::TUNINGTYPE CTUNINGBASE::TT_GEOMETRIC = 3; //0...11b


TEMPLATEDEC
void CTUNINGBASE::TuningCopy(CTuningBase& to, const CTuningBase& from, const bool allowExactnamecopy)
//------------------------------------------------------------------------------------
{
	if(!to.MayEdit(EM_ALLOWALL))
		return;

	if(allowExactnamecopy)
		to.m_TuningName = from.m_TuningName;
	else
		to.m_TuningName = string("Copy of ") + from.m_TuningName;

	to.m_NoteNameMap = from.m_NoteNameMap;
	to.m_EditMask = from.m_EditMask;
	to.m_EditMask |= EM_EDITMASK; //Not copying possible strict-const-status.

	to.m_TuningType = from.m_TuningType;

	//Copying ratios.
	const VRPAIR rp = to.ProSetValidityRange(from.GetValidityRange());

	//Copying ratios
	for(NOTEINDEXTYPE i = rp.first; i<=rp.second; i++)
	{
		to.ProSetRatio(i, from.GetRatio(i));
	}
	to.ProSetGroupSize(from.GetGroupSize());
	to.ProSetGroupRatio(from.GetGroupRatio());
	to.ProSetFineStepCount(from.GetFineStepCount());
}


TEMPLATEDEC
bool CTUNINGBASE::SetRatio(const NOTEINDEXTYPE& s, const RATIOTYPE& r)
//-----------------------------------------------------------------
{
	if(MayEdit(EM_RATIOS))
	{
		if(ProSetRatio(s, r))
			return true;
		else
			SetType(TT_GENERAL);

		return false;
	}
	return true;

}

TEMPLATEDEC
TYPENAME CTUNINGBASE::USTEPINDEXTYPE CTUNINGBASE::SetFineStepCount(const USTEPINDEXTYPE& fs)
//-------------------------------------------------------
{
	VRPAIR vrp = GetValidityRange();

	if( (vrp.first > vrp.second)
		||
		(!IsStepCountRangeSufficient(fs, vrp))
		||
		(!MayEdit(EM_FINETUNE))
	  ) return GetFineStepCount();
	else
	{
		ProSetFineStepCount(fs);
		return GetFineStepCount();
	}
}

TEMPLATEDEC
TYPENAME CTUNINGBASE::TUNINGTYPE CTUNINGBASE::GetTuningType(const char* str)
//--------------------------------------------------------------------------
{
	if(!strcmp(str, s_TuningTypeStrGroupGeometric))
		return TT_GROUPGEOMETRIC;
	if(!strcmp(str, s_TuningTypeStrGeometric))
		return TT_GEOMETRIC;

	return TT_GENERAL;
}

TEMPLATEDEC
string CTUNINGBASE::GetTuningTypeStr(const TUNINGTYPE& tt)
//----------------------------------------------------------------
{
	if(tt == TT_GENERAL)
		return s_TuningTypeStrGeneral;
	if(tt == TT_GROUPGEOMETRIC)
		return s_TuningTypeStrGroupGeometric;
	if(tt == TT_GEOMETRIC)
		return s_TuningTypeStrGeometric;
	return "Unknown";
}



TEMPLATEDEC
TYPENAME CTUNINGBASE::NOTESTR CTUNINGBASE::GetNoteName(const NOTEINDEXTYPE& x) const
//-----------------------------------------------------------------------
{
	if(!IsValidNote(x)) return "";
	else return ProGetNoteName(x);
}

TEMPLATEDEC
TYPENAME CTUNINGBASE::NOTESTR CTUNINGBASE::ProGetNoteName(const NOTEINDEXTYPE& x) const
//-------------------------------------------------------------------------------------
{
	NNM_CITER i = m_NoteNameMap.find(x);
	if(i != m_NoteNameMap.end())
		return i->second;
	else
		return Stringify(x);
}


TEMPLATEDEC
bool CTUNINGBASE::IsOfType(const TUNINGTYPE& type) const
//----------------------------------------------------
{
	if(type == TT_GENERAL)
		return true;
	//Using interpretation that every tuning is also
	//a general tuning.

	//Note: Here type != TT_GENERAL
	if(m_TuningType == TT_GENERAL)
		return false;
	//Every non-general tuning should not include all tunings.

	if( (m_TuningType & type) == type)
		return true;
	else
		return false;
}

TEMPLATEDEC
bool CTUNINGBASE::SetNoteName(const NOTEINDEXTYPE& n, const string& str)
//-----------------------------------------------------------------------
{
	if(MayEdit(EM_NOTENAME))
	{
		m_NoteNameMap[n] = str;
		return false;
	}
	return true;
}



TEMPLATEDEC
bool CTUNINGBASE::ClearNoteName(const NOTEINDEXTYPE& n, const bool eraseAll)
//-------------------------------------------------------
{
	if(MayEdit(EM_NOTENAME))
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
	return true;
}


TEMPLATEDEC
bool CTUNINGBASE::Multiply(const RATIOTYPE& r)
//---------------------------------------------------
{
	if(r <= 0 || !MayEdit(EM_RATIOS))
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

TEMPLATEDEC
bool CTUNINGBASE::CreateGroupGeometric(const NOTEINDEXTYPE& s, const RATIOTYPE& r, const NOTEINDEXTYPE& startindex)
//-------------------------------------------------------------
{
	if(s < 1 || r <= 0 || startindex < GetValidityRange().first)
		return true;

	vector<RATIOTYPE> v;
	v.reserve(s);
	for(NOTEINDEXTYPE i = startindex; i<startindex+s; i++)
		v.push_back(GetRatio(i));
	return CreateGroupGeometric(v, r, GetValidityRange(), startindex);
}

TEMPLATEDEC
bool CTUNINGBASE::CreateGroupGeometric(const vector<RATIOTYPE>& v, const RATIOTYPE& r, const VRPAIR vr, const NOTEINDEXTYPE ratiostartpos)
//------------------------------------------------------------------------------------------
{
	if(MayEdit(EM_RATIOS) &&
		(MayEdit(EM_TYPE) || GetType() == TT_GROUPGEOMETRIC))
	{
		if(vr.first > vr.second || v.size() == 0) return true;
		if(ratiostartpos < vr.first || vr.second < ratiostartpos || static_cast<UNOTEINDEXTYPE>(vr.second - ratiostartpos) < static_cast<UNOTEINDEXTYPE>(v.size() - 1)) return true;
		if(!IsStepCountRangeSufficient(GetFineStepCount(), vr)) return true;
		for(size_t i = 0; i<v.size(); i++) {if(v[i] < 0) return true;}
		if(ProCreateGroupGeometric(v,r, vr, ratiostartpos))
			return true;
		else
		{
			SetType(TT_GROUPGEOMETRIC);
			ProSetFineStepCount(GetFineStepCount());
			return false;
		}
	}
	else
		return true;
}


TEMPLATEDEC
bool CTUNINGBASE::CreateGeometric(const UNOTEINDEXTYPE& s, const RATIOTYPE& r, const VRPAIR vr)
//-------------------------------------------------------------------
{
	if(MayEdit(EM_RATIOS) &&
	  (MayEdit(EM_TYPE) || GetType() == TT_GEOMETRIC))
	{
		if(vr.first > vr.second) return true;
		if(s < 1 || r <= 0) return true;
		if(ProCreateGeometric(s,r,vr))
			return true;
		else
		{
			SetType(TT_GEOMETRIC);
			ProSetFineStepCount(GetFineStepCount());
			return false;
		}
	}
	else
		return true;
}



TEMPLATEDEC
bool CTUNINGBASE::ChangeGroupsize(const NOTEINDEXTYPE& s)
//---------------------------------------------------
{
	if(!MayEdit(EM_RATIOS) || s < 1)
		return true;

	if(m_TuningType == TT_GROUPGEOMETRIC)
		return CreateGroupGeometric(s, GetGroupRatio(), 0);

	if(m_TuningType == TT_GEOMETRIC)
		return CreateGeometric(s, GetGroupRatio());

	return true;
}


TEMPLATEDEC
bool CTUNINGBASE::ChangeGroupRatio(const RATIOTYPE& r)
//---------------------------------------------------
{
	if(!MayEdit(EM_RATIOS) || r <= 0)
		return true;

	if(m_TuningType == TT_GROUPGEOMETRIC)
		return CreateGroupGeometric(GetGroupSize(), r, 0);

	if(m_TuningType == TT_GEOMETRIC)
		return CreateGeometric(GetGroupSize(), r);

	return true;
}


TEMPLATEDEC
const char* CTUNINGBASE::GetTuningTypeDescription(const TUNINGTYPE& type)
//---------------------------------------------------------------------------------------
{
	if(type == TT_GENERAL)
		return s_TuningDescriptionGeneral;
	if(type == TT_GROUPGEOMETRIC)
		return s_TuningDescriptionGroupGeometric;
	if(type == TT_GEOMETRIC)
		return s_TuningDescriptionGeometric;
	return "Unknown";
}

TEMPLATEDEC
TYPENAME CTUNINGBASE::VRPAIR CTUNINGBASE::SetValidityRange(const VRPAIR& vrp)
//----------------------------------------------------------------------------------------
{
	if(vrp.second < vrp.first) return GetValidityRange();
	if(IsStepCountRangeSufficient(GetFineStepCount(), vrp)
		&&
		MayEdit(EM_VALIDITYRANGE)
	   )
		return ProSetValidityRange(vrp);
	else
		return GetValidityRange();
}


TEMPLATEDEC
bool CTUNINGBASE::SetType(const TUNINGTYPE& tt)
//----------------------------------------------
{
	//Note: This doesn't check whether the tuning ratios
	//are consistent with given type.
	if(MayEdit(EM_TYPE))
	{
		m_TuningType = tt;

		if(m_TuningType == TT_GENERAL)
		{
			ProSetGroupSize(0);
			ProSetGroupRatio(0);
		}

		return false;
	}
	else
		return true;
}


TEMPLATEDEC
bool CTUNINGBASE::DeserializeOLD(istream& inStrm)
//------------------------------------------------
{
	char begin[8];
	int16 version;

	inStrm.read(begin, sizeof(begin));
	if(memcmp(begin, "CT<sfs>B", 8)) return SERIALIZATION_FAILURE;

	inStrm.read(reinterpret_cast<char*>(&version), sizeof(version));
	if(version != 4) return SERIALIZATION_FAILURE;

	//Tuning name
	if(StringFromBinaryStream<uint8>(inStrm, m_TuningName))
		return SERIALIZATION_FAILURE;

	//Const mask
	int16 em = 0;
	inStrm.read(reinterpret_cast<char*>(&em), sizeof(em));
	m_EditMask = em;

	//Tuning type
	int16 tt = 0;
	inStrm.read(reinterpret_cast<char*>(&tt), sizeof(tt));
	m_TuningType = tt;

	//Notemap
	UNOTEINDEXTYPE size;
	inStrm.read(reinterpret_cast<char*>(&size), sizeof(size));
	for(size_t i = 0; i<size; i++)
	{
		NOTEINDEXTYPE n;
		string str;
		inStrm.read(reinterpret_cast<char*>(&n), sizeof(n));
		if(StringFromBinaryStream<uint8>(inStrm, str))
			return SERIALIZATION_FAILURE;
		m_NoteNameMap[n] = str;
	}

	//End marker
	char end[8];
	inStrm.read(end, sizeof(end));
	if(memcmp(end, "CT<sfs>E", 8)) return SERIALIZATION_FAILURE;

	return SERIALIZATION_SUCCESS;
}


#endif

