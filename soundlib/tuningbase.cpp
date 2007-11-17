#include "stdafx.h"
#define TUNINGBASE_CPP
#include "tuningbase.h"

#ifdef TUNINGBASE_H



//Utilities for reading/writing notenamemap.
static uint8 NotenamemapKeybytes = 0;
TEMPLATEDEC
void CTUNINGBASE::ReadNotenamemapPair(istream& iStrm, TYPENAME NOTENAMEMAP::value_type& val, const size_t)
//-------------------------------------------------------------------------------------------------
{
	uint64 key = 0;
	if(NotenamemapKeybytes > 8) return;
	iStrm.read(reinterpret_cast<char*>(&key), NotenamemapKeybytes);
	TYPENAME CTuningBase::NOTENAMEMAP::key_type* p = (TYPENAME CTuningBase::NOTENAMEMAP::key_type*)(&val.first);
	*p = static_cast<TYPENAME CTuningBase::NOTENAMEMAP::key_type>(key);
	StringFromBinaryStream<uint8>(iStrm, val.second);
}
TEMPLATEDEC
void CTUNINGBASE::WriteNotenamemappair(ostream& oStrm, const TYPENAME NOTENAMEMAP::value_type& val, const size_t)
//------------------------------------------------------------------------
{
	oStrm.write(reinterpret_cast<const char*>(&val.first), sizeof(TYPENAME CTUNINGBASE::NOTENAMEMAP::key_type));
	StringToBinaryStream<uint8>(oStrm, val.second);
}


//=======================================================================
TEMPLATEDEC
class CTuningReadInstructions : public srlztn::CSerializationInstructions
//=======================================================================
{
public:
	friend class CTUNINGBASE;

	CTuningReadInstructions(const uint64 version, const size_t entrycounthint) :
		CSerializationInstructions("", version, srlztn::INFLAG, entrycounthint),
		m_pTuning(0)
		{}

	virtual bool CompareObjectClassID(const vector<char>& objectClassID)
	//------------------------------------------------------------------
	{
		const size_t idsize = objectClassID.size();
		if(idsize < 6) return true;
		string str; str.resize(idsize);
		for(size_t i = 0; i<idsize; i++) {str[i] = objectClassID[i];}
		const size_t pos = str.find("CTB");
		if(pos == string::npos)
			return true;

		if(!isdigit(str[3]) || !isdigit(str[4]) || !isdigit(str[5]))
			return true;

		const BYTE bytesPerNotetype = str[3] - 48;
		const BYTE bytesPerRatiotype = str[4] - 48;
		const BYTE bytesPerSteptype = str[5] - 48;
		if(bytesPerNotetype == 0 || bytesPerRatiotype == 0 || bytesPerSteptype == 0
			|| bytesPerNotetype > 8 || bytesPerRatiotype > 8 || bytesPerSteptype > 8) return true;

		str.erase(pos, 6);

		const size_t numCreationfunctions = CTUNINGBASE::GetCreationfunctionarray().size();
		for(size_t i = 0; i<numCreationfunctions && !m_pTuning; i++)
		{
			m_pTuning = (CTUNINGBASE::GetCreationfunctionarray()[i])(str, *this);
		}
		if(m_pTuning)
		{
			BYTE sizes[3] = {bytesPerNotetype, bytesPerRatiotype, bytesPerSteptype};
			m_pTuning->AddSI(*this, sizes);
			return false;
		}
		else return true;
	}

private:
	CTUNINGBASE* m_pTuning;
};

#ifdef BUILD_TUNINGBASE_AS_TEMPLATE
	#define CTUNINGREADINSTRUCTIONS CTuningReadInstructions<A,B,C,D,E>
#else
	#define CTUNINGREADINSTRUCTIONS CTuningReadInstructions
#endif

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
const BYTE CTUNINGBASE::s_Typesizes[3] = {sizeof(CTUNINGBASE::NOTEINDEXTYPE), sizeof(CTUNINGBASE::RATIOTYPE), sizeof(CTUNINGBASE::STEPINDEXTYPE)};

TEMPLATEDEC
const string CTUNINGBASE::s_ClassID = string("CTB") + Stringify(sizeof(CTUNINGBASE::NOTEINDEXTYPE)) + Stringify(sizeof(CTUNINGBASE::RATIOTYPE)) + Stringify(sizeof(CTUNINGBASE::STEPINDEXTYPE));

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
const string CTUNINGBASE::s_FileExtension = ".tun";

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
std::vector<TYPENAME CTUNINGBASE::CREATIONFUNCTION> CTUNINGBASE::s_Creationfunctions;



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
		if(vr.first > vr.second) return true;
		if(ratiostartpos < vr.first || vr.second < ratiostartpos || static_cast<UNOTEINDEXTYPE>(vr.second - ratiostartpos) < static_cast<UNOTEINDEXTYPE>(v.size())) return true;
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
		return CreateGroupGeometric(s, GetGroupRatio(), GetValidityRange().first);

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
		return CreateGroupGeometric(GetGroupSize(), r, GetValidityRange().first);

	if(m_TuningType == TT_GEOMETRIC)
		return CreateGeometric(GetGroupSize(), r);

	return true;
}

TEMPLATEDEC
TYPENAME CTUNINGBASE::SERIALIZATION_RETURN_TYPE CTUNINGBASE::Serialize(ostream& outStrm) const
//--------------------------------------------------------------------------------------------
{
	using namespace srlztn;

	const string classID = CTuningBase::s_ClassID + GetDerivedClassID();
	CSerializationInstructions instructions(classID, (GetVersion() << 24) + GetClassVersion(), OUTFLAG, 4);
	instructions.SetWritepropUseTimestamp(false);
	instructions.SetWritepropIncludeStartposInMap(false);
	
	AddSI(instructions, s_Typesizes);
	string msg; msg.reserve(instructions.GetEntrycount() * 30);
	CSSBSerialization sm(instructions);
	sm.SetLogstring(msg);
	sm.Serialize(outStrm);
	if(msg.size() > 0) MessageHandler(msg.c_str(), "Tuning writemessages");

	return 0;
}


TEMPLATEDEC
CTUNINGBASE* CTUNINGBASE::Unserialize(istream& inStrm)
//-------------------------------------------------------------------
{
	if(inStrm.fail())
		return 0;

	using namespace srlztn;

	//Using 4 byte version in which 'largest' byte is for base class version, and
	//rest three for derived classes.
	CTUNINGREADINSTRUCTIONS instructions(CTuningBase::GetVersion() << 24, 4);
	string msg; msg.reserve(instructions.GetEntrycount() * 30);
	CSSBSerialization sm(instructions);
	sm.SetLogstring(msg);
	CTuningBase* pT = 0;
	if(sm.Unserialize(inStrm) & SNT_FAILURE)
		delete instructions.m_pTuning;
	else
		pT = instructions.m_pTuning;

	if(msg.size() > 0) MessageHandler(msg.c_str(), "Tuning readmessages");
	if(pT) 
	{
		EDITMASK temp = pT->GetEditMask();
		pT->m_EditMask = EM_ALLOWALL; //Allowing all while processing data.
		if(pT->ProProcessUnserializationdata())
		{
			MessageHandler(("Processing loaded data for tuning \"" + pT->GetName() + "\" failed.").c_str(), "Tuning load failure");
			delete pT; pT = 0;
		}
		else
		{ 
			USTEPINDEXTYPE fsTemp = pT->m_FineStepCount;
			pT->m_FineStepCount = 0;
			pT->SetFineStepCount(fsTemp);
			pT->SetEditMask(temp);
		}
	}
	return pT;
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
void CTUNINGBASE::AddSI(srlztn::ABCSerializationInstructions& instructions, const BYTE sizes[3]) const
//-------------------------------------------------------------------------------------------------
{
	using namespace srlztn;
	NotenamemapKeybytes = sizes[0];
	bool arwi = instructions.AreWriteInstructions();
	if(!arwi || m_TuningName.length() > 0) //Writing name to file only when there is one.
		instructions.AddEntry(CSerializationentry("0", CContainerstreamer<string>::NewDefaultReadWrite((string&)m_TuningName, uint8_max, uint8_max)));

	instructions.AddEntry(CSerializationentry("1", new CBinarystreamer<EDITMASK>(m_EditMask)));
	instructions.AddEntry(CSerializationentry("2", new CBinarystreamer<TUNINGTYPE>(m_TuningType)));
	if(!arwi || m_NoteNameMap.size() > 0)
		instructions.AddEntry(CSerializationentry("3", CContainerstreamer<NOTENAMEMAP>::NewCustomReadWrite((NOTENAMEMAP&)m_NoteNameMap, UNOTEINDEXTYPE_MAX, m_NoteNameMap.size(), &WriteNotenamemappair, &ReadNotenamemapPair), ""));
	if(!arwi || GetFineStepCount() > 0)
		instructions.AddEntry(CSerializationentry("4", new CBinarystreamer<USTEPINDEXTYPE>(m_FineStepCount, sizes[2]), ""));
	ProAddSI(instructions, sizes);
}


TEMPLATEDEC
bool CTUNINGBASE::UnserializeOLD(istream& inStrm)
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

