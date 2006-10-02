#include "stdafx.h"
#include "tuning.h"
#include <string>

const CTuning::STEPTYPE CTuningRTI::s_StepMinDefault(-128);
const CTuning::STEPTYPE CTuningRTI::s_RatioTableSizeDefault(256);

//CTuningRTi-statics
const CTuning::SERIALIZATION_MARKER CTuningRTI::s_SerializationBeginMarker("CTRTI_B.");
const CTuning::SERIALIZATION_MARKER CTuningRTI::s_SerializationEndMarker("CTRTI_E.");
const CTuning::SERIALIZATION_VERSION CTuningRTI::s_SerializationVersion(2);

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/*
TODOS:
-Handling of const status could be improved(e.g. case one can
 change ratios, but not type)
-Finetune improvements
*/

CTuningRTI::CTuningRTI(const CTuning*& pTun)
//------------------------------------------
{
	SetDummyValues();
	if(pTun)
	{
		CTuning::operator=(*pTun);	
	}	
}

void CTuningRTI::SetDummyValues()
//---------------------------------
{
	if(MayEdit(EM_MAINRATIOS))
	{
		m_RatioTable.clear();
		m_StepMin = m_StepsInPeriod = 0;
		m_PeriodRatio = 0;
		m_RatioTableFine.clear();
		BelowRatios = AboveRatios = DefaultBARFUNC;
	}
}


bool CTuningRTI::CreateRatioTableRP(const vector<RATIOTYPE>& v, const RATIOTYPE r)
//------------------------------------------------------------------------------
{
	if(!MayEdit(EM_MAINRATIOS) || 
		(!MayEdit(EM_TYPE) && GetType() != TT_RATIOPERIODIC))
		return true;

	if(v.size() == 0 || r <= 0) return true;

	m_StepMin = s_StepMinDefault;
	m_StepsInPeriod = static_cast<STEPTYPE>(v.size());
	m_PeriodRatio = r;
	BelowRatios = AboveRatios = DefaultBARFUNC;

	m_RatioTable.clear();
	m_RatioTable.reserve(s_RatioTableSizeDefault);
	for(STEPTYPE i = 0; i<s_RatioTableSizeDefault; i++)
	{
		const STEPTYPE fromCentre = i + m_StepMin;
		if(fromCentre < 0)
		{
			m_RatioTable.push_back(
			v[((fromCentre % m_StepsInPeriod) + m_StepsInPeriod)%m_StepsInPeriod] * pow(r, (fromCentre+1)/m_StepsInPeriod - 1)
			);
		}
		else
		{
			m_RatioTable.push_back(
			v[abs(fromCentre%m_StepsInPeriod)] * pow(r, abs(fromCentre)/m_StepsInPeriod)
			);
		}
		
	}
	return false;
}


bool CTuningRTI::ProCreateRatioPeriodic(const vector<RATIOTYPE>& v, const RATIOTYPE& r)
//---------------------------------------------------------------------
{
	if(CreateRatioTableRP(v, r)) return true;
	else return false;
}


bool CTuningRTI::ProCreateTET(const STEPTYPE& s, const RATIOTYPE& r)
//---------------------------------------------------------------
{
	if(s <= 0 || r <= 0) return true;
	SetDummyValues();
	m_StepMin = s_StepMinDefault;
	m_StepsInPeriod = s;
	m_PeriodRatio = r;
	BelowRatios = AboveRatios = DefaultBARFUNC;
	const RATIOTYPE stepRatio = pow(r, static_cast<RATIOTYPE>(1)/s);

	m_RatioTable.clear();
	m_RatioTable.reserve(s_RatioTableSizeDefault);
	for(STEPTYPE i = 0; i<s_RatioTableSizeDefault; i++)
	{
		m_RatioTable.push_back(pow(stepRatio, i + m_StepMin));
	}
	return false;
}

string CTuningRTI::GetNoteName(const STEPTYPE& x) const
//-----------------------------------------------------
{
	if(GetPeriod() < 1)
	{
		return CTuning::GetNoteName(x);
	}
	else //Tuning periodic.
	{
		//First checking whether note is not within the 
		//first period and whether it exists in the map - always
		//using specifically given notename instead of possible
		//tuning specific naming style.
		if(x >= GetPeriod() || x < 0)
		{
			NNM_CITER nmi = m_NoteNameMap.find(x);
			if(nmi != m_NoteNameMap.end())
				return nmi->second;
		}

		const STEPTYPE pos = ((x % m_StepsInPeriod) + m_StepsInPeriod) % m_StepsInPeriod;
		const STEPTYPE middlePeriodNumber = 5;
		string rValue;
		NNM_CITER nmi = m_NoteNameMap.find(pos);
		if(nmi != m_NoteNameMap.end())
		{
			rValue = nmi->second;
			(x >= 0) ? rValue += Stringify(middlePeriodNumber + x/m_StepsInPeriod) : rValue += Stringify(middlePeriodNumber + (x+1)/m_StepsInPeriod - 1);
		}
		else 
		{
			//By default, using notation nnP for notes; nn <-> note character starting
			//from 'A' with char ':' as fill char, and P is period integer. For example:
			//C:5, D:3, R:7
			rValue = Stringify(static_cast<char>(pos + 65)); //char 65 == A

			rValue += ":";

			(x >= 0) ? rValue += Stringify(middlePeriodNumber + x/m_StepsInPeriod) : rValue += Stringify(middlePeriodNumber + (x+1)/m_StepsInPeriod - 1);
		}
		
		return rValue;	
	}
}



CTuning::SERIALIZATION_RETURN_TYPE CTuningRTI::SerializeBinary(ostream& outStrm, const int mode) const
//-------------------------------------------------------
{
	if(!outStrm.good())
		return SERIALIZATION_FAILURE;

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		outStrm.write(reinterpret_cast<const char*>(&s_SerializationBeginMarker), sizeof(s_SerializationBeginMarker));
	}

	//Version
	outStrm.write(reinterpret_cast<const char*>(&s_SerializationVersion), sizeof(s_SerializationVersion));

	//Baseclass serialization
	if(CTuning::SerializeBinary(outStrm) == SERIALIZATION_FAILURE) return SERIALIZATION_FAILURE;
	
	//Main Ratios
	if(VectorToBinaryStream(outStrm, m_RatioTable))
		return SERIALIZATION_FAILURE;

	//Fine ratios
	if(VectorToBinaryStream(outStrm, m_RatioTableFine))
		return SERIALIZATION_FAILURE;
	
	//m_StepMin
	outStrm.write(reinterpret_cast<const char*>(&m_StepMin), sizeof(m_StepMin));

	//m_StepsInPeriod
	outStrm.write(reinterpret_cast<const char*>(&m_StepsInPeriod), sizeof(m_StepsInPeriod));

	//m_PeriodRatio
	outStrm.write(reinterpret_cast<const char*>(&m_PeriodRatio), sizeof(m_PeriodRatio));

	//TODO: BARFUNC serialization.

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		outStrm.write(reinterpret_cast<const char*>(&s_SerializationEndMarker), sizeof(s_SerializationEndMarker));
	}

	if(outStrm.good())
		return SERIALIZATION_SUCCESS;
	else 
		return SERIALIZATION_FAILURE;
}


CTuning::SERIALIZATION_RETURN_TYPE CTuningRTI::UnSerializeBinary(istream& inStrm, const int mode)
//-------------------------------------------------------------------------------
{
	if(!inStrm.good())
		return SERIALIZATION_FAILURE;

	SERIALIZATION_MARKER begin;
	SERIALIZATION_MARKER end;
	SERIALIZATION_VERSION version;

	const long startPos = inStrm.tellg();

	//First checking is there expected (optional) begin sequence.
	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		inStrm.read(reinterpret_cast<char*>(&begin), sizeof(begin));
		if(begin != s_SerializationBeginMarker)
		{
			inStrm.seekg(startPos);
			//Returning stream position if beginmarker was not found.

			return SERIALIZATION_FAILURE;
		}
	}
	
	//Version
	inStrm.read(reinterpret_cast<char*>(&version), sizeof(version));
	if(version != 1 && version != s_SerializationVersion)
		return SERIALIZATION_FAILURE;
	
	//Baseclass Unserialization
	if(CTuning::UnSerializeBinary(inStrm) == SERIALIZATION_FAILURE)
		return SERIALIZATION_FAILURE;

	//Ratiotable
	if(VectorFromBinaryStream(inStrm, m_RatioTable))
		return SERIALIZATION_FAILURE;

	//Ratiotable fine
	if(version > 1)
	{
		if(VectorFromBinaryStream(inStrm, m_RatioTableFine))
			return SERIALIZATION_FAILURE;
	}
	

	//m_StepMin
	inStrm.read(reinterpret_cast<char*>(&m_StepMin), sizeof(m_StepMin));

	//m_StepsInPeriod
	inStrm.read(reinterpret_cast<char*>(&m_StepsInPeriod), sizeof(m_StepsInPeriod));

	//m_PeriodRatio
	inStrm.read(reinterpret_cast<char*>(&m_PeriodRatio), sizeof(m_PeriodRatio));

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		inStrm.read(reinterpret_cast<char*>(&end), sizeof(end));
		if(end != s_SerializationEndMarker) return SERIALIZATION_FAILURE;
	}
	

	if(inStrm.good()) return SERIALIZATION_SUCCESS;
	else return SERIALIZATION_FAILURE;

}


CTuningRTI::RATIOTYPE CTuningRTI::DefaultBARFUNC(const STEPTYPE&, const STEPTYPE&)
//---------------------------------------------------------------
{
	return 1;
}

//Without finetune
CTuning::RATIOTYPE CTuningRTI::GetFrequencyRatio(const STEPTYPE& stepsFromCentre) const
//-------------------------------------------------------------------------------------
{
	if(stepsFromCentre < m_StepMin) return BelowRatios(stepsFromCentre,0);
	if(stepsFromCentre >= m_StepMin + static_cast<RATIOTYPE>(m_RatioTable.size())) return AboveRatios(stepsFromCentre,0);
	return m_RatioTable[stepsFromCentre - m_StepMin];
}


//With finetune
CTuning::RATIOTYPE CTuningRTI::GetFrequencyRatio(const STEPTYPE& baseStep, const FINESTEPTYPE& baseFineStep) const
//----------------------------------------------------------------------------------------------------
{
	if(GetFineStepCount() < 1)
		return GetFrequencyRatio(baseStep);

	//If finestep count is more than the number of finesteps between notes,
	//step is increased. So first figuring out what step and fineStep values to
	//actually use. Interpreting finestep -1 on note x so that it is the same as 
	//finestep GetFineStepCount()-1 on note x-1.
	STEPTYPE stepsFromCentre;
	FINESTEPTYPE fineStep;
	if(baseFineStep >= 0)
	{
		stepsFromCentre = baseStep + baseFineStep / GetFineStepCount();
		fineStep = baseFineStep % GetFineStepCount();
	}
	else
	{
		stepsFromCentre = baseStep + (baseFineStep + 1) / GetFineStepCount() - 1;
		fineStep = (GetFineStepCount() + baseFineStep % GetFineStepCount()) % GetFineStepCount();
	}
	
    if(stepsFromCentre < m_StepMin) return BelowRatios(stepsFromCentre, fineStep);
	if(stepsFromCentre >= m_StepMin + static_cast<RATIOTYPE>(m_RatioTable.size())) return AboveRatios(stepsFromCentre, fineStep);

	return m_RatioTable[stepsFromCentre - m_StepMin] * GetFineStepRatio(fineStep);
}


bool CTuningRTI::ProSetRatio(const STEPTYPE& s, const RATIOTYPE& r)
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


CTuningRTI::VRPAIR CTuningRTI::SetValidityRange(const VRPAIR&)
//---------------------------------------------------------------
{
	//For now setting default regardless of demand:
	//TODO: Make it work properly.
	m_StepMin = s_StepMinDefault;
	m_RatioTable.resize(s_RatioTableSizeDefault, 1);
	return GetValidityRange();
}

CTuningRTI::FINESTEPTYPE CTuningRTI::ProSetFineStepCount(const STEPTYPE& s)
//-----------------------------------------------------
{
	STEPTYPE fineSteps = s;
	if(fineSteps < 0) //Interpreting this as 'use default'
		fineSteps = 16; //16 is the default here.

	if(fineSteps == 0)
	{
		m_RatioTableFine.clear();
		return GetFineStepCount();
	}

	if(DoesTypeInclude(TT_RATIOPERIODIC))
	{
		m_RatioTableFine.resize(fineSteps);
		const RATIOTYPE rFineStep = pow(GetFrequencyRatio(1) / GetFrequencyRatio(0), static_cast<RATIOTYPE>(1)/fineSteps);
		for(STEPTYPE i = 0; i<fineSteps; i++)
			m_RatioTableFine[i] = pow(rFineStep, i);
		return GetFineStepCount();
		//NOTE: Above is actually intented for TET only, and thus may be totally
		//senseless for many Ratioperiodics, but that'll do for now - functionality
		//for better finetune support(e.g. own ratios for every step and for every
		//finestep) for non-TET-tunings is something that could be implemented some
		//day.
	}

	m_RatioTableFine.clear();
	return GetFineStepCount();
}


