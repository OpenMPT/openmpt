/*
 * tuning.cpp
 * ----------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "tuning.h"
#include "../common/mptIO.h"
#include "../common/serialization_utils.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Reporting.h"
#endif
#include "../common/misc_util.h"
#include <string>

OPENMPT_NAMESPACE_BEGIN

typedef CTuningRTI::RATIOTYPE RATIOTYPE;
typedef CTuningRTI::NOTEINDEXTYPE NOTEINDEXTYPE;
typedef CTuningRTI::UNOTEINDEXTYPE UNOTEINDEXTYPE;
typedef CTuningRTI::STEPINDEXTYPE STEPINDEXTYPE;
typedef CTuningRTI::USTEPINDEXTYPE USTEPINDEXTYPE;

namespace CTuningS11n
{
	void ReadStr(std::istream& iStrm, std::string& str, const size_t);
	void ReadNoteMap(std::istream& iStrm, CTuning::NOTENAMEMAP& m, const size_t);
	void ReadRatioTable(std::istream& iStrm, std::vector<CTuningRTI::RATIOTYPE>& v, const size_t);

	void WriteNoteMap(std::ostream& oStrm, const CTuning::NOTENAMEMAP& m);
	void WriteStr(std::ostream& oStrm, const std::string& str);

	struct RatioWriter
	//================
	{
		RatioWriter(uint16 nWriteCount = s_nDefaultWriteCount) : m_nWriteCount(nWriteCount) {}

		void operator()(std::ostream& oStrm, const std::vector<float>& v);
		uint16 m_nWriteCount;
		static const uint16 s_nDefaultWriteCount = (uint16_max >> 2);
	};
}

using namespace CTuningS11n;


/*
Version changes:
	3->4: Finetune related internal structure and serialization revamp.
	2->3: The type for the size_type in the serialisation changed
		  from default(size_t, uint32) to unsigned STEPTYPE. (March 2007)
*/


static RATIOTYPE Pow(const RATIOTYPE r, const STEPINDEXTYPE s)
//------------------------------------------------------------
{
	if(s == 0) return 1;
	RATIOTYPE result = r;
	STEPINDEXTYPE absS = mpt::abs(s);
	for(STEPINDEXTYPE i = 1; i < absS; i++) result *= r;
	return (s > 0) ? result : 1/result;
}


CTuningRTI::CTuningRTI(const CTuning* const pTun)
//-----------------------------------------------
{
	SetDummyValues();
	if(pTun) TuningCopy(*this, *pTun);
}

void CTuningRTI::SetDummyValues()
//-------------------------------
{
	if(MayEdit(EM_RATIOS))
	{
		m_RatioTable.clear();
		m_StepMin = s_StepMinDefault;
		m_RatioTable.resize(s_RatioTableSizeDefault, 1);
		m_GroupSize = 0;
		m_GroupRatio = 0;
		m_RatioTableFine.clear();
	}
}


bool CTuningRTI::CreateRatioTableGG(const std::vector<RATIOTYPE>& v, const RATIOTYPE r, const VRPAIR& vr, const NOTEINDEXTYPE ratiostartpos)
//------------------------------------------------------------------------------------------------------------------------------------------
{
	if(v.size() == 0
		|| r <= 0
		|| vr.second < vr.first
		|| ratiostartpos < vr.first)
	{
		return true;
	}

	m_StepMin = vr.first;
	ProSetGroupSize(static_cast<UNOTEINDEXTYPE>(v.size()));
	ProSetGroupRatio(r);

	m_RatioTable.resize(vr.second-vr.first+1);
	std::copy(v.begin(), v.end(), m_RatioTable.begin() + (ratiostartpos - vr.first));

	for(int32 i = ratiostartpos-1; i>=m_StepMin && ratiostartpos > NOTEINDEXTYPE_MIN; i--)
	{
		m_RatioTable[i-m_StepMin] = m_RatioTable[i - m_StepMin + m_GroupSize] / m_GroupRatio;
	}
	for(int32 i = ratiostartpos+m_GroupSize; i<=vr.second && ratiostartpos <= (NOTEINDEXTYPE_MAX - m_GroupSize); i++)
	{
		m_RatioTable[i-m_StepMin] = m_GroupRatio * m_RatioTable[i - m_StepMin - m_GroupSize];
	}

	return false;
}


bool CTuningRTI::ProCreateGroupGeometric(const std::vector<RATIOTYPE>& v, const RATIOTYPE& r, const VRPAIR& vr, const NOTEINDEXTYPE ratiostartpos)
//------------------------------------------------------------------------------------------------------------------------------------------------
{
	//Note: Setting finestep is handled by base class when CreateGroupGeometric is called.
	if(CreateRatioTableGG(v, r, vr, ratiostartpos)) return true;
	else return false;
}


bool CTuningRTI::ProCreateGeometric(const UNOTEINDEXTYPE& s, const RATIOTYPE& r, const VRPAIR& vr)
//------------------------------------------------------------------------------------------------
{
	if(vr.second - vr.first + 1 > NOTEINDEXTYPE_MAX) return true;
	//Note: Setting finestep is handled by base class when CreateGeometric is called.
	SetDummyValues();
	m_StepMin = vr.first;
	ProSetGroupSize(s);
	ProSetGroupRatio(r);
	const RATIOTYPE stepRatio = pow(r, static_cast<RATIOTYPE>(1)/s);

	m_RatioTable.resize(vr.second - vr.first + 1);
	for(int32 i = vr.first; i<=vr.second; i++)
	{
		m_RatioTable[i-m_StepMin] = Pow(stepRatio, i);
	}
	return false;
}

CTuningRTI::NOTESTR CTuningRTI::ProGetNoteName(const NOTEINDEXTYPE& x, bool addOctave) const
//------------------------------------------------------------------------------------------
{
	if(GetGroupSize() < 1)
	{
		return CTuning::ProGetNoteName(x, addOctave);
	}
	else
	{
		const NOTEINDEXTYPE pos = ((x % m_GroupSize) + m_GroupSize) % m_GroupSize;
		const NOTEINDEXTYPE middlePeriodNumber = 5;
		std::string rValue;
		NNM_CITER nmi = m_NoteNameMap.find(pos);
		if(nmi != m_NoteNameMap.end())
		{
			rValue = nmi->second;
			if(addOctave)
			{
				if(x >= 0)
					rValue += mpt::ToString(middlePeriodNumber + x / m_GroupSize);
				else
					rValue += mpt::ToString(middlePeriodNumber + (x + 1) / m_GroupSize - 1);
			}
		}
		else
		{
			//By default, using notation nnP for notes; nn <-> note character starting
			//from 'A' with char ':' as fill char, and P is period integer. For example:
			//C:5, D:3, R:7
			rValue = std::string(1, static_cast<char>(pos + 'A'));

			rValue += ":";

			if(addOctave)
			{
				if(x >= 0)
					rValue += mpt::ToString(middlePeriodNumber + x/m_GroupSize);
				else
					rValue += mpt::ToString(middlePeriodNumber + (x+1)/m_GroupSize - 1);
			}
		}
		return rValue;
	}
}


const RATIOTYPE CTuningRTI::s_DefaultFallbackRatio = 1.0f;


//Without finetune
CTuning::RATIOTYPE CTuningRTI::GetRatio(const NOTEINDEXTYPE& stepsFromCentre) const
//---------------------------------------------------------------------------------
{
	if(stepsFromCentre < m_StepMin) return s_DefaultFallbackRatio;
	if(stepsFromCentre >= m_StepMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size())) return s_DefaultFallbackRatio;
	return m_RatioTable[stepsFromCentre - m_StepMin];
}


//With finetune
CTuning::RATIOTYPE CTuningRTI::GetRatio(const NOTEINDEXTYPE& baseNote, const STEPINDEXTYPE& baseStepDiff) const
//-------------------------------------------------------------------------------------------------------------
{
	const STEPINDEXTYPE fsCount = static_cast<STEPINDEXTYPE>(GetFineStepCount());
	if(fsCount == 0 || baseStepDiff == 0)
	{
		return GetRatio(static_cast<NOTEINDEXTYPE>(baseNote + baseStepDiff));
	}

	//If baseStepDiff is more than the number of finesteps between notes,
	//note is increased. So first figuring out what step and fineStep values to
	//actually use. Interpreting finestep -1 on note x so that it is the same as
	//finestep GetFineStepCount() on note x-1.
	//Note: If finestepcount is n, n+1 steps are needed to get to
	//next note.
	NOTEINDEXTYPE note;
	STEPINDEXTYPE fineStep;
	if(baseStepDiff >= 0)
	{
		note = static_cast<NOTEINDEXTYPE>(baseNote + baseStepDiff / (fsCount+1));
		fineStep = baseStepDiff % (fsCount+1);
	}
	else
	{
		note = static_cast<NOTEINDEXTYPE>(baseNote + ((baseStepDiff+1) / (fsCount+1)) - 1);
		fineStep = ((fsCount + 1) - (mpt::abs(baseStepDiff) % (fsCount+1))) % (fsCount+1);
	}

	if(note < m_StepMin) return s_DefaultFallbackRatio;
	if(note >= m_StepMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size())) return s_DefaultFallbackRatio;

	if(fineStep) return m_RatioTable[note - m_StepMin] * GetRatioFine(note, fineStep);
	else return m_RatioTable[note - m_StepMin];
}


CTuning::RATIOTYPE CTuningRTI::GetRatioFine(const NOTEINDEXTYPE& note, USTEPINDEXTYPE sd) const
//---------------------------------------------------------------------------------------------
{
	if(GetFineStepCount() <= 0)
		return 1;

	//Neither of these should happen.
	if(sd <= 0) sd = 1;
	if(sd > GetFineStepCount()) sd = GetFineStepCount();

	if(GetType() != TT_GENERAL && m_RatioTableFine.size() > 0) //Taking fineratio from table
	{
		if(GetType() == TT_GEOMETRIC)
		{
			return m_RatioTableFine[sd-1];
		}
		if(GetType() == TT_GROUPGEOMETRIC)
			return m_RatioTableFine[GetRefNote(note) * GetFineStepCount() + sd - 1];

		MPT_ASSERT_NOTREACHED();
		return m_RatioTableFine[0]; //Shouldn't happen.
	}
	else //Calculating ratio 'on the fly'.
	{
		//'Geometric finestepping'.
		return pow(GetRatio(note+1) / GetRatio(note), static_cast<RATIOTYPE>(sd)/(GetFineStepCount()+1));

	}

}


bool CTuningRTI::ProSetRatio(const NOTEINDEXTYPE& s, const RATIOTYPE& r)
//----------------------------------------------------------------------
{
	//Creating ratio table if doesn't exist.
	if(m_RatioTable.empty())
	{
		m_RatioTable.assign(s_RatioTableSizeDefault, 1);
		m_StepMin = s_StepMinDefault;
	}

	//If note is not within the table, at least for now
	//simply don't change anything.
	if(!IsNoteInTable(s))
		return true;

	m_RatioTable[s - m_StepMin] = fabs(r);
	return false;
}


CTuningRTI::VRPAIR CTuningRTI::ProSetValidityRange(const VRPAIR&)
//---------------------------------------------------------------
{
	//TODO: Implementation. Things to note:
	//		-If validity range is smaller than period, various methods such as ProSetFinestepcount
	//			might create wrong ratios.
	return GetValidityRange();
}


void CTuningRTI::ProSetFineStepCount(const USTEPINDEXTYPE& fs)
//------------------------------------------------------------
{
	if(fs <= 0)
	{
		m_FineStepCount = 0;
		m_RatioTableFine.clear();
		return;
	}

	m_FineStepCount = (fs > static_cast<UNOTEINDEXTYPE>(NOTEINDEXTYPE_MAX)) ? static_cast<UNOTEINDEXTYPE>(NOTEINDEXTYPE_MAX) : fs;

	if(GetType() == TT_GEOMETRIC)
	{
		if(m_FineStepCount > s_RatioTableFineSizeMaxDefault)
		{
			m_RatioTableFine.clear();
			return;
		}
		m_RatioTableFine.resize(m_FineStepCount);
		const RATIOTYPE q = GetRatio(GetValidityRange().first + 1) / GetRatio(GetValidityRange().first);
		const RATIOTYPE rFineStep = pow(q, static_cast<RATIOTYPE>(1)/(m_FineStepCount+1));
		for(USTEPINDEXTYPE i = 1; i<=m_FineStepCount; i++)
			m_RatioTableFine[i-1] = Pow(rFineStep, i);
		return;
	}
	if(GetType() == TT_GROUPGEOMETRIC)
	{
		const UNOTEINDEXTYPE p = GetGroupSize();
		if(p > s_RatioTableFineSizeMaxDefault / m_FineStepCount)
		{
			//In case fineratiotable would become too large, not using
			//table for it.
			m_RatioTableFine.clear();
			return;
		}
		else
		{
			//Creating 'geometric' finestepping between notes.
			m_RatioTableFine.resize(p * m_FineStepCount);
			const NOTEINDEXTYPE startnote = GetRefNote(GetValidityRange().first);
			for(UNOTEINDEXTYPE i = 0; i<p; i++)
			{
				const NOTEINDEXTYPE refnote = GetRefNote(startnote+i);
				const RATIOTYPE rFineStep = pow(GetRatio(refnote+1) / GetRatio(refnote), static_cast<RATIOTYPE>(1)/(m_FineStepCount+1));
				for(UNOTEINDEXTYPE j = 1; j<=m_FineStepCount; j++)
				{
					m_RatioTableFine[m_FineStepCount * refnote + (j-1)] = pow(rFineStep, static_cast<RATIOTYPE>(j));
				}
			}
			return;
		}

	}
	if(GetType() == TT_GENERAL)
	{
		//Not using table with tuning of type general.
		m_RatioTableFine.clear();
		return;
	}

	//Should not reach here.
	m_RatioTableFine.clear();
	m_FineStepCount = 0;
}


CTuningRTI::NOTEINDEXTYPE CTuningRTI::GetRefNote(const NOTEINDEXTYPE note) const
//------------------------------------------------------------------------------
{
	if(!IsOfType(TT_GROUPGEOMETRIC)) return 0;

	if(note >= 0) return note % GetGroupSize();
	else return (GetGroupSize() - (mpt::abs(static_cast<int>(note)) % GetGroupSize())) % GetGroupSize();
}


CTuning* CTuningRTI::Deserialize(std::istream& iStrm)
//---------------------------------------------------
{
	if(iStrm.fail())
		return nullptr;

	CTuningRTI* pTuning = new CTuningRTI;

	srlztn::SsbRead ssb(iStrm);
	ssb.BeginRead("CTB244RTI", (CTuning::GetVersion() << 24) + GetVersion());
	ssb.ReadItem(pTuning->m_TuningName, "0", ReadStr);
	ssb.ReadItem(pTuning->m_EditMask, "1");
	ssb.ReadItem(pTuning->m_TuningType, "2");
	ssb.ReadItem(pTuning->m_NoteNameMap, "3", ReadNoteMap);
	ssb.ReadItem(pTuning->m_FineStepCount, "4");

	// RTI entries.
	ssb.ReadItem(pTuning->m_RatioTable, "RTI0", ReadRatioTable);
	ssb.ReadItem(pTuning->m_StepMin, "RTI1");
	ssb.ReadItem(pTuning->m_GroupSize, "RTI2");
	ssb.ReadItem(pTuning->m_GroupRatio, "RTI3");
	UNOTEINDEXTYPE ratiotableSize = 0;
	ssb.ReadItem(ratiotableSize, "RTI4");

	// If reader status is ok and m_StepMin is somewhat reasonable, process data.
	if ((ssb.GetStatus() & srlztn::SNT_FAILURE) == 0 && pTuning->m_StepMin >= -300 && pTuning->m_StepMin <= 300)
	{
		EDITMASK temp = pTuning->GetEditMask();
		pTuning->m_EditMask = EM_ALLOWALL; //Allowing all while processing data.
		if (pTuning->ProProcessUnserializationdata(ratiotableSize))
		{
#ifdef MODPLUG_TRACKER
			Reporting::Error(("Processing loaded data for tuning \"" + pTuning->GetName() + "\" failed.").c_str(), "Tuning load failure");
#else
			MPT_LOG(LogError, "tuning", MPT_USTRING("Processing loaded data for tuning \"") + mpt::ToUnicode(mpt::CharsetISO8859_1, pTuning->GetName()) + MPT_USTRING("\" failed."));
#endif
			delete pTuning; pTuning = nullptr;
		}
		else
		{
			USTEPINDEXTYPE fsTemp = pTuning->m_FineStepCount;
			pTuning->m_FineStepCount = 0;
			pTuning->SetFineStepCount(fsTemp);
			pTuning->SetEditMask(temp);
		}
	}
	else
		{delete pTuning; pTuning = nullptr;}
	return pTuning;
}


bool CTuningRTI::ProProcessUnserializationdata(UNOTEINDEXTYPE ratiotableSize)
//---------------------------------------------------------------------------
{
	if (m_GroupSize < 0) {m_GroupSize = 0; return true;}
	if (m_RatioTable.size() > static_cast<size_t>(NOTEINDEXTYPE_MAX)) return true;
	if (IsOfType(TT_GROUPGEOMETRIC))
	{
		if (ratiotableSize < 1 || ratiotableSize > NOTEINDEXTYPE_MAX) return true;
		if (GetType() == TT_GEOMETRIC)
			return CTuning::CreateGeometric(GetGroupSize(), GetGroupRatio(), VRPAIR(m_StepMin, m_StepMin+ratiotableSize-1));
		else
		{
			return CreateGroupGeometric(m_RatioTable, GetGroupRatio(), VRPAIR(m_StepMin, m_StepMin+ratiotableSize-1), m_StepMin);
		}
	}
	return false;
}


template<class T, class SIZETYPE, class Tdst>
static bool VectorFromBinaryStream(std::istream& inStrm, std::vector<Tdst>& v, const SIZETYPE maxSize = (std::numeric_limits<SIZETYPE>::max)())
//---------------------------------------------------------------------------------------------------------------------------------------------
{
	if(!inStrm.good()) return true;

	SIZETYPE size = 0;
	mpt::IO::ReadIntLE<SIZETYPE>(inStrm, size);

	if(size > maxSize)
		return true;

	v.resize(size);
	for(std::size_t i = 0; i<size; i++)
	{
		T tmp = T();
		mpt::IO::Read(inStrm, tmp);
		v[i] = tmp;
	}
	if(inStrm.good())
		return false;
	else
		return true;
}


CTuningRTI* CTuningRTI::DeserializeOLD(std::istream& inStrm)
//----------------------------------------------------------
{
	if(!inStrm.good())
		return 0;

	const std::streamoff startPos = inStrm.tellg();

	//First checking is there expected begin sequence.
	char begin[8];
	MemsetZero(begin);
	inStrm.read(begin, sizeof(begin));
	if(std::memcmp(begin, "CTRTI_B.", 8))
	{
		//Returning stream position if beginmarker was not found.
		inStrm.seekg(startPos);
		return 0;
	}

	//Version
	int16 version = 0;
	mpt::IO::ReadIntLE<int16>(inStrm, version);
	if(version != 3)
		return 0;

	CTuningRTI* pT = new CTuningRTI;

	//Baseclass deserialization
	if(pT->CTuning::DeserializeOLD(inStrm) == SERIALIZATION_FAILURE)
	{
		delete pT;
		return 0;
	}

	//Ratiotable
	if(VectorFromBinaryStream<IEEE754binary32LE, uint16>(inStrm, pT->m_RatioTable))
	{
		delete pT;
		return 0;
	}

	//Fineratios
	if(VectorFromBinaryStream<IEEE754binary32LE, uint16>(inStrm, pT->m_RatioTableFine))
	{
		delete pT;
		return 0;
	}
	pT->m_FineStepCount = pT->m_RatioTableFine.size();

	//m_StepMin
	int16 stepmin = 0;
	mpt::IO::ReadIntLE<int16>(inStrm, stepmin);
	pT->m_StepMin = stepmin;
	if(pT->m_StepMin < -200 || pT->m_StepMin > 200)
	{
		delete pT;
		return nullptr;
	}

	//m_GroupSize
	int16 groupsize = 0;
	mpt::IO::ReadIntLE<int16>(inStrm, groupsize);
	pT->m_GroupSize = groupsize;
	if(pT->m_GroupSize < 0)
	{
		delete pT;
		return 0;
	}

	//m_GroupRatio
	IEEE754binary32LE groupratio = IEEE754binary32LE(0.0f);
	mpt::IO::Read(inStrm, groupratio);
	pT->m_GroupRatio = groupratio;
	if(pT->m_GroupRatio < 0)
	{
		delete pT;
		return 0;
	}

	if(pT->GetFineStepCount() > 0) pT->ProSetFineStepCount(pT->GetFineStepCount() - 1);

	char end[8];
	MemsetZero(end);
	inStrm.read(reinterpret_cast<char*>(&end), sizeof(end));
	if(std::memcmp(end, "CTRTI_E.", 8))
	{
		delete pT;
		return 0;
	}

	return pT;
}


CTuning::SERIALIZATION_RETURN_TYPE CTuningRTI::Serialize(std::ostream& outStrm) const
//-----------------------------------------------------------------------------------
{
	srlztn::SsbWrite ssb(outStrm);
	ssb.BeginWrite("CTB244RTI", (GetVersion() << 24) + GetClassVersion());
	if (m_TuningName.length() > 0)
		ssb.WriteItem(m_TuningName, "0", WriteStr);
	ssb.WriteItem(m_EditMask, "1");
	ssb.WriteItem(m_TuningType, "2");
	if (m_NoteNameMap.size() > 0)
		ssb.WriteItem(m_NoteNameMap, "3", WriteNoteMap);
	if (GetFineStepCount() > 0)
		ssb.WriteItem(m_FineStepCount, "4");

	const TUNINGTYPE tt = GetType();
	if (GetGroupRatio() > 0)
		ssb.WriteItem(m_GroupRatio, "RTI3");
	if (tt == TT_GROUPGEOMETRIC)
		ssb.WriteItem(m_RatioTable, "RTI0", RatioWriter(GetGroupSize()));
	if (tt == TT_GENERAL)
		ssb.WriteItem(m_RatioTable, "RTI0", RatioWriter());
	if (tt == TT_GEOMETRIC)
		ssb.WriteItem(m_GroupSize, "RTI2");

	if(tt == TT_GEOMETRIC || tt == TT_GROUPGEOMETRIC)
	{	//For Groupgeometric this data is the number of ratios in ratiotable.
		UNOTEINDEXTYPE ratiotableSize = static_cast<UNOTEINDEXTYPE>(m_RatioTable.size());
		ssb.WriteItem(ratiotableSize, "RTI4");
	}

	//m_StepMin
	ssb.WriteItem(m_StepMin, "RTI1");

	ssb.FinishWrite();

	return ((ssb.GetStatus() & srlztn::SNT_FAILURE) != 0) ? SERIALIZATION_FAILURE : SERIALIZATION_SUCCESS;
}


namespace CTuningS11n
{

void RatioWriter::operator()(std::ostream& oStrm, const std::vector<float>& v)
//----------------------------------------------------------------------------
{
	const size_t nWriteCount = MIN(v.size(), m_nWriteCount);
	mpt::IO::WriteAdaptiveInt64LE(oStrm, nWriteCount);
	for(size_t i = 0; i < nWriteCount; i++)
		mpt::IO::Write(oStrm, IEEE754binary32LE(v[i]));
}


void ReadNoteMap(std::istream& iStrm, CTuning::NOTENAMEMAP& m, const size_t)
//--------------------------------------------------------------------------
{
	uint64 val;
	mpt::IO::ReadAdaptiveInt64LE(iStrm, val);
	LimitMax(val, 256u); // Read 256 at max.
	for(size_t i = 0; i < val; i++)
	{
		int16 key;
		mpt::IO::ReadIntLE<int16>(iStrm, key);
		std::string str;
		mpt::IO::ReadSizedStringLE<uint8>(iStrm, str);
		m[key] = str;
	}
}


void ReadRatioTable(std::istream& iStrm, std::vector<CTuningRTI::RATIOTYPE>& v, const size_t)
//-------------------------------------------------------------------------------------------
{
	uint64 val;
	mpt::IO::ReadAdaptiveInt64LE(iStrm, val);
	v.resize( static_cast<size_t>(MIN(val, 256u))); // Read 256 vals at max.
	for(size_t i = 0; i < v.size(); i++)
	{
		IEEE754binary32LE tmp(0.0f);
		mpt::IO::Read(iStrm, tmp);
		v[i] = tmp;
	}
}


void ReadStr(std::istream& iStrm, std::string& str, const size_t)
//---------------------------------------------------------------
{
	uint64 val;
	mpt::IO::ReadAdaptiveInt64LE(iStrm, val);
	size_t nSize = (val > 255) ? 255 : static_cast<size_t>(val); // Read 255 characters at max.
	str.resize(nSize);
	for(size_t i = 0; i < nSize; i++)
		mpt::IO::ReadIntLE(iStrm, str[i]);
}


void WriteNoteMap(std::ostream& oStrm, const CTuning::NOTENAMEMAP& m)
//-------------------------------------------------------------------
{
	mpt::IO::WriteAdaptiveInt64LE(oStrm, m.size());
	CTuning::NNM_CITER iter = m.begin();
	CTuning::NNM_CITER end = m.end();
	for(; iter != end; iter++)
	{
		mpt::IO::WriteIntLE<int16>(oStrm, iter->first);
		mpt::IO::WriteSizedStringLE<uint8>(oStrm, iter->second);
	}
}


void WriteStr(std::ostream& oStrm, const std::string& str)
//--------------------------------------------------------
{
	mpt::IO::WriteAdaptiveInt64LE(oStrm, str.size());
	oStrm.write(str.c_str(), str.size());
}

} // namespace CTuningS11n.


OPENMPT_NAMESPACE_END
