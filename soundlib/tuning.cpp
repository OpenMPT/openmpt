#include "stdafx.h"

#include "tuning.h"
#include "../mptrack/serialization_utils.h"
#include <string>

typedef CTuningRTI::RATIOTYPE RATIOTYPE;
typedef CTuningRTI::NOTEINDEXTYPE NOTEINDEXTYPE;
typedef CTuningRTI::UNOTEINDEXTYPE UNOTEINDEXTYPE;
typedef CTuningRTI::STEPINDEXTYPE STEPINDEXTYPE;
typedef  CTuningRTI::USTEPINDEXTYPE USTEPINDEXTYPE;

const NOTEINDEXTYPE CTuningRTI::s_StepMinDefault(-64);
const UNOTEINDEXTYPE CTuningRTI::s_RatioTableSizeDefault(128);
const STEPINDEXTYPE CTuningRTI::s_RatioTableFineSizeMaxDefault(1000);

const CTuning::SERIALIZATION_VERSION CTuningRTI::s_SerializationVersion(4);

const string CTuningRTI::s_DerivedclassID = "RTI";



/*
Version changes:
	3->4: Finetune related internal structure and serialization revamp.
	2->3: The type for the size_type in the serialisation changed
		  from default(size_t, uint32) to unsigned STEPTYPE. (March 2007)
*/

template<class T>
static inline T Abs(T& val) {return (val >= 0) ? val : -val;}
//---------------------------------------------------------


static RATIOTYPE Pow(const RATIOTYPE r, const STEPINDEXTYPE s)
//-------------------------------------------------------------
{
	if(s == 0) return 1;
	RATIOTYPE result = r;
	STEPINDEXTYPE absS = (s > 0) ? s : -s;
	for(STEPINDEXTYPE i = 1; i<absS; i++) result *= r;
	return (s > 0) ? result : 1/result;
}


CTuningRTI::CTuningRTI(const CTuning* const pTun)
//------------------------------------------
{
	SetDummyValues();
	if(pTun) TuningCopy(*this, *pTun);
}

void CTuningRTI::SetDummyValues()
//---------------------------------
{
	if(MayEdit(EM_RATIOS))
	{
		m_RatioTable.clear();
		m_StepMin = s_StepMinDefault;
		m_RatioTable.resize(s_RatioTableSizeDefault, 1);
		m_GroupSize = 0;
		m_GroupRatio = 0;
		m_RatioTableFine.clear();
		BelowRatios = AboveRatios = DefaultBARFUNC;
	}
}


bool CTuningRTI::CreateRatioTableGG(const vector<RATIOTYPE>& v, const RATIOTYPE r, const VRPAIR& vr, const NOTEINDEXTYPE ratiostartpos)
//-------------------------------------------------------------------------------------------------
{
	if(v.size() == 0 || r <= 0) return true;

    m_StepMin = vr.first;
	ProSetGroupSize(static_cast<UNOTEINDEXTYPE>(v.size()));
	ProSetGroupRatio(r);
	BelowRatios = AboveRatios = DefaultBARFUNC;

	m_RatioTable.resize(vr.second-vr.first+1);
	std::copy(v.begin(), v.end(), m_RatioTable.begin() + (ratiostartpos - vr.first));

	for(NOTEINDEXTYPE i = ratiostartpos-1; i>=m_StepMin && ratiostartpos > NOTEINDEXTYPE_MIN; i--)
	{
		m_RatioTable[i-m_StepMin] = m_RatioTable[i - m_StepMin + m_GroupSize] / m_GroupRatio;
	}
	for(NOTEINDEXTYPE i = ratiostartpos+m_GroupSize; i<=vr.second && ratiostartpos <= (NOTEINDEXTYPE_MAX - m_GroupSize); i++)
	{
		m_RatioTable[i-m_StepMin] = m_GroupRatio * m_RatioTable[i - m_StepMin - m_GroupSize];
	}

	return false;
}


bool CTuningRTI::ProCreateGroupGeometric(const vector<RATIOTYPE>& v, const RATIOTYPE& r, const VRPAIR& vr, const NOTEINDEXTYPE ratiostartpos)
//---------------------------------------------------------------------
{
	//Note: Setting finestep is handled by base class when CreateGroupGeometric is called.
	if(CreateRatioTableGG(v, r, vr, ratiostartpos)) return true;
	else return false;
}


bool CTuningRTI::ProCreateGeometric(const UNOTEINDEXTYPE& s, const RATIOTYPE& r, const VRPAIR& vr)
//---------------------------------------------------------------
{
	if(vr.second - vr.first + 1 > NOTEINDEXTYPE_MAX) return true;
	//Note: Setting finestep is handled by base class when CreateGeometric is called.
	SetDummyValues();
	m_StepMin = vr.first;
	ProSetGroupSize(s);
	ProSetGroupRatio(r);
	BelowRatios = AboveRatios = DefaultBARFUNC;
	const RATIOTYPE stepRatio = pow(r, static_cast<RATIOTYPE>(1)/s);

	m_RatioTable.resize(vr.second - vr.first + 1);
	for(NOTEINDEXTYPE i = vr.first; i<=vr.second; i++)
	{
		m_RatioTable[i-m_StepMin] = Pow(stepRatio, i);
	}
	return false;
}

CTuningRTI::NOTESTR CTuningRTI::ProGetNoteName(const NOTEINDEXTYPE& x) const
//-----------------------------------------------------
{
	if(GetGroupSize() < 1)
	{
		return CTuning::ProGetNoteName(x);
	}
	else
	{
		const NOTEINDEXTYPE pos = ((x % m_GroupSize) + m_GroupSize) % m_GroupSize;
		const NOTEINDEXTYPE middlePeriodNumber = 5;
		string rValue;
		NNM_CITER nmi = m_NoteNameMap.find(pos);
		if(nmi != m_NoteNameMap.end())
		{
			rValue = nmi->second;
			(x >= 0) ? rValue += Stringify(middlePeriodNumber + x/m_GroupSize) : rValue += Stringify(middlePeriodNumber + (x+1)/m_GroupSize - 1);
		}
		else
		{
			//By default, using notation nnP for notes; nn <-> note character starting
			//from 'A' with char ':' as fill char, and P is period integer. For example:
			//C:5, D:3, R:7
			rValue = Stringify(static_cast<char>(pos + 65)); //char 65 == 'A'

			rValue += ":";

			(x >= 0) ? rValue += Stringify(middlePeriodNumber + x/m_GroupSize) : rValue += Stringify(middlePeriodNumber + (x+1)/m_GroupSize - 1);
		}
		return rValue;
	}
}


CTuningRTI::RATIOTYPE CTuningRTI::DefaultBARFUNC(const NOTEINDEXTYPE&, const STEPINDEXTYPE&)
//---------------------------------------------------------------
{
	return 1;
}

//Without finetune
CTuning::RATIOTYPE CTuningRTI::GetRatio(const NOTEINDEXTYPE& stepsFromCentre) const
//-------------------------------------------------------------------------------------
{
	if(stepsFromCentre < m_StepMin) return BelowRatios(stepsFromCentre,0);
	if(stepsFromCentre >= m_StepMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size())) return AboveRatios(stepsFromCentre,0);
	return m_RatioTable[stepsFromCentre - m_StepMin];
}


//With finetune
CTuning::RATIOTYPE CTuningRTI::GetRatio(const NOTEINDEXTYPE& baseNote, const STEPINDEXTYPE& baseStepDiff) const
//----------------------------------------------------------------------------------------------------
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
		fineStep = ((fsCount + 1) - (abs(baseStepDiff) % (fsCount+1))) % (fsCount+1);
	}

    if(note < m_StepMin) return BelowRatios(note, fineStep);
	if(note >= m_StepMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size())) return AboveRatios(note, fineStep);

	if(fineStep) return m_RatioTable[note - m_StepMin] * GetRatioFine(note, fineStep);
	else return m_RatioTable[note - m_StepMin];
}


CTuning::RATIOTYPE CTuningRTI::GetRatioFine(const NOTEINDEXTYPE& note, USTEPINDEXTYPE sd) const
//--------------------------------------------------------------
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

		ASSERT(false);
		return m_RatioTableFine[0]; //Shouldn't happen.
	}
	else //Calculating ratio 'on the fly'.
	{
		//'Geometric finestepping'.
		return pow(GetRatio(note+1) / GetRatio(note), static_cast<RATIOTYPE>(sd)/(GetFineStepCount()+1));

	}

}


bool CTuningRTI::ProSetRatio(const NOTEINDEXTYPE& s, const RATIOTYPE& r)
//--------------------------------------------------------------
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
//-----------------------------------------------------
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
//------------------------------------------------------
{
	if(!IsOfType(TT_GROUPGEOMETRIC)) return 0;

	if(note >= 0) return note % GetGroupSize();
	else return (GetGroupSize() -  (Abs(note) % GetGroupSize())) % GetGroupSize();
}

CTuning* CTuningRTI::CreateRTITuning(const string& id, srlztn::ABCSerializationInstructions&)
//----------------------------------------------------------------------------------------
{
	if(id == s_DerivedclassID)
		return new CTuningRTI;
	else
		return 0;

}

void CTuningRTI::ProAddSI(srlztn::ABCSerializationInstructions& instr, const BYTE typesizes[3]) const
//-----------------------------------------------------------------
{
	using namespace srlztn;
	if(instr.AreReadInstructions()) instr.SetInstructionsVersion(instr.GetInstructionVersion() + GetVersion());

	typedef vector<RATIOTYPE> RV;
	//Note ratios
	if(instr.AreReadInstructions())
	{
		if(typesizes[1] == 4)
			instr.AddEntry(CSerializationentry("RTI0", CContainerstreamer<RV>::NewDefaultReader((RV&)m_RatioTable, NOTEINDEXTYPE_MAX), ""));
		if(typesizes[1] == 8)
			instr.AddEntry(CSerializationentry("RTI0", CContainerstreamer<RV>::NewCustomReader((RV&)m_RatioTable, NOTEINDEXTYPE_MAX, &srlztn::DoubleToFloatReader), "", "File contains double ratios, but reading them to floats."));


		//Group size.
		instr.AddEntry(CSerializationentry("RTI2", new CBinarystreamer<NOTEINDEXTYPE>(m_GroupSize, typesizes[0]), "Groupsize"));

		//Group ratio
		instr.AddEntry(CSerializationentry("RTI3", new CBinarystreamer<RATIOTYPE>(m_GroupRatio, typesizes[1]), "Groupratio"));

		//Ratiotablesize
		m_SerHelperRatiotableSize = 0;
		instr.AddEntry(CSerializationentry("RTI4", new CBinarystreamer<UNOTEINDEXTYPE>(m_SerHelperRatiotableSize, typesizes[0]), "Ratiotable size"));
	}
	else //Writing instructions
	{
		const TUNINGTYPE tt = GetType();
		if(GetGroupRatio() > 0)
			instr.AddEntry(CSerializationentry("RTI3", new CBinarystreamer<RATIOTYPE>(m_GroupRatio, typesizes[1])));
		if(tt == TT_GROUPGEOMETRIC)
			instr.AddEntry(CSerializationentry("RTI0", CContainerstreamer<vector<RATIOTYPE> >::NewDefaultWriter(m_RatioTable, GetGroupSize())));
		if(tt == TT_GENERAL)
			instr.AddEntry(CSerializationentry("RTI0", CContainerstreamer<vector<RATIOTYPE> >::NewDefaultWriter(m_RatioTable)));
		if(tt == TT_GEOMETRIC)
			instr.AddEntry(CSerializationentry("RTI2", new CBinarystreamer<NOTEINDEXTYPE>(m_GroupSize, typesizes[0])));
			//For Groupgeometric this data is in the number of ratios in ratiotable.

		if(tt == TT_GEOMETRIC || tt == TT_GROUPGEOMETRIC)
		{
			m_SerHelperRatiotableSize = static_cast<UNOTEINDEXTYPE>(m_RatioTable.size());
			instr.AddEntry(CSerializationentry("RTI4", new CBinarystreamer<UNOTEINDEXTYPE>(m_SerHelperRatiotableSize, typesizes[0])));
		}
	}

	//m_StepMin
	instr.AddEntry(CSerializationentry("RTI1", new CBinarystreamer<NOTEINDEXTYPE>(m_StepMin, typesizes[0])));
}


bool CTuningRTI::ProProcessUnserializationdata()
//----------------------------------------------
{
	if(m_GroupSize < 0) {m_GroupSize = 0; return true;}
	if(m_RatioTable.size() > static_cast<size_t>(NOTEINDEXTYPE_MAX)) return true;
	if(IsOfType(TT_GROUPGEOMETRIC))
	{
		if(m_SerHelperRatiotableSize < 1 || m_SerHelperRatiotableSize > NOTEINDEXTYPE_MAX) return true;
		if(GetType() == TT_GEOMETRIC)
			return CTuning::CreateGeometric(GetGroupSize(), GetGroupRatio(), VRPAIR(m_StepMin, m_StepMin+m_SerHelperRatiotableSize-1));
		else
		{
			return CreateGroupGeometric(m_RatioTable, GetGroupRatio(), VRPAIR(m_StepMin, m_StepMin+m_SerHelperRatiotableSize-1), m_StepMin);
		}
	}
	return false;
}

CTuningRTI* CTuningRTI::UnserializeOLD(istream& inStrm)
//-----------------------------------------------------------
{
	if(!inStrm.good())
		return 0;


	char begin[8];
	char end[8];
	int16 version;

	const long startPos = inStrm.tellg();

	//First checking is there expected begin sequence.
	inStrm.read(reinterpret_cast<char*>(&begin), sizeof(begin));
	if(memcmp(begin, "CTRTI_B.", 8))
	{
		//Returning stream position if beginmarker was not found.
		inStrm.seekg(startPos);
		return 0;
	}

	//Version
	inStrm.read(reinterpret_cast<char*>(&version), sizeof(version));
	if(version != 3)
		return 0;

	CTuningRTI* pT = new CTuningRTI;

	//Baseclass Unserialization
	if(pT->CTuning::UnserializeOLD(inStrm) == SERIALIZATION_FAILURE)
	{
		delete pT;
		return 0;
	}

	//Ratiotable
	if(VectorFromBinaryStream<RATIOTYPE, uint16>(inStrm, pT->m_RatioTable))
	{
		delete pT;
		return 0;
	}

	//Fineratios
	if(VectorFromBinaryStream<RATIOTYPE, uint16>(inStrm, pT->m_RatioTableFine))
	{
		delete pT;
		return 0;
	}
	else
		pT->m_FineStepCount = pT->m_RatioTableFine.size();

	//m_StepMin
	inStrm.read(reinterpret_cast<char*>(&pT->m_StepMin), sizeof(pT->m_StepMin));

	//m_GroupSize
	inStrm.read(reinterpret_cast<char*>(&pT->m_GroupSize), sizeof(pT->m_GroupSize));
	if(pT->m_GroupSize < 0)
	{
		delete pT;
		return 0;
	}

	//m_GroupRatio
	inStrm.read(reinterpret_cast<char*>(&pT->m_GroupRatio), sizeof(pT->m_GroupRatio));
	if(pT->m_GroupRatio <= 0)
	{
		delete pT;
		return 0;
	}

	if(pT->GetFineStepCount() > 0) pT->ProSetFineStepCount(pT->GetFineStepCount() - 1);

	inStrm.read(reinterpret_cast<char*>(&end), sizeof(end));
	if(memcmp(end, "CTRTI_E.", 8))
	{
		delete pT;
		return 0;
	}


	return pT;
}



