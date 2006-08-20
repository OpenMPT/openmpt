#include "stdafx.h"
#include "tuning.h"
#include <string>

const CTuning::STEPTYPE CTuningRTI::s_StepMinDefault(-128);
const CTuning::STEPTYPE CTuningRTI::s_RatioTableSizeDefault(256);

//CTuningRTi-statics
const CTuning::SERIALIZATION_MARKER CTuningRTI::s_SerializationBeginMarker("CTRTI_B.");
const CTuning::SERIALIZATION_MARKER CTuningRTI::s_SerializationEndMarker("CTRTI_E.");
const CTuning::SERIALIZATION_VERSION CTuningRTI::s_SerializationVersion(1);

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/*
TODOS:
-Handling of const status could be improved(e.g. case one can
 change ratios, but not type)
-The ratiovalue copying is general enough that it could be moved to base class.
-Check how tuning name handles things if ratio periodic/tet 
 tuning has note mapped outside first period.
*/

CTuningRTI::CTuningRTI(const CTuning*& pTun)
//------------------------------------------
{
	SetDummyValues();
	if(pTun)
	{
		//For now, only copying ratio values from *pTun
		//to *this without checking validity range and
		//not taking finetuning into account.
		//TODO: 
		m_StepMin = s_StepMinDefault;
		m_RatioTable.resize(s_RatioTableSizeDefault);
		for(STEPTYPE i = 0; i<m_RatioTable.size(); i++)
		{
            m_RatioTable[i]= pTun->GetFrequencyRatio(i+m_StepMin, 0);
		}
		m_StepsInPeriod = pTun->GetPeriod();
		m_PeriodRatio = pTun->GetPeriodRatio();
		CTuning::operator=(*pTun);
	}	
}

void CTuningRTI::SetDummyValues()
//---------------------------------
{
	if(MayEdit(EM_RATIOS))
	{
		m_RatioTable.clear();
		m_StepMin = m_StepsInPeriod = 0;
		m_PeriodRatio = 0;
		BelowRatios = AboveRatios = DefaultBARFUNC;
	}
}


bool CTuningRTI::CreateRatioTableRP(const vector<RATIOTYPE>& v, const RATIOTYPE r)
//------------------------------------------------------------------------------
{
	if(!MayEdit(EM_RATIOS) || 
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


bool CTuningRTI::CreateRatioPeriodic(const vector<RATIOTYPE>& v, const RATIOTYPE& r)
//---------------------------------------------------------------------
{
	if(MayEdit(EM_RATIOS) &&
		(MayEdit(EM_TYPE) || GetType() == TT_RATIOPERIODIC))
	{
		SetType(TT_RATIOPERIODIC);
		if(CreateRatioTableRP(v, r)) return true;
		return false;
	}
	else
		return true;
}


bool CTuningRTI::CreateTET(const STEPTYPE& s, const RATIOTYPE& r)
//---------------------------------------------------------------
{
	if(MayEdit(EM_RATIOS) &&
	  (MayEdit(EM_TYPE) || GetType() == TT_TET))
	{
		if(s <= 0 || r <= 0) return true;

		SetDummyValues();
		m_StepMin = s_StepMinDefault;
		m_StepsInPeriod = s;
		m_PeriodRatio = r;
		SetType(TT_TET);
		BelowRatios = AboveRatios = DefaultBARFUNC;

		const RATIOTYPE stepRatio = pow(r, 1.0F/s);

		m_RatioTable.clear();
		m_RatioTable.reserve(s_RatioTableSizeDefault);
		for(STEPTYPE i = 0; i<s_RatioTableSizeDefault; i++)
		{
			m_RatioTable.push_back(pow(stepRatio, i + m_StepMin));
		}
		return false;
	}
	return true;
}

string CTuningRTI::GetNoteName(const STEPTYPE& x) const
//-----------------------------------------------------
{
	if(m_StepsInPeriod < 1)
	{
		return CTuning::GetNoteName(x);
	}
	else
	{
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
	if(outStrm.fail())
		return SERIALIZATION_FAILURE;

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		outStrm.write(reinterpret_cast<const char*>(&s_SerializationBeginMarker), sizeof(s_SerializationBeginMarker));
	}

	//Version
	outStrm.write(reinterpret_cast<const char*>(&s_SerializationVersion), sizeof(s_SerializationVersion));

	//Baseclass serialization
	if(CTuning::SerializeBinary(outStrm) == SERIALIZATION_FAILURE) return SERIALIZATION_FAILURE;
	
	//Vector data
	if(VectorToBinaryStream(outStrm, m_RatioTable))
		return SERIALIZATION_FAILURE;
	
	//StepMin
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
	if(inStrm.fail())
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
    if(version != s_SerializationVersion) return SERIALIZATION_FAILURE;

	//Baseclass Unserialization
	if(CTuning::UnSerializeBinary(inStrm) == SERIALIZATION_FAILURE)
		return SERIALIZATION_FAILURE;

	//Ratiotable
	if(VectorFromBinaryStream(inStrm, m_RatioTable))
		return SERIALIZATION_FAILURE;

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


CTuning::RATIOTYPE CTuningRTI::GetFrequencyRatio(const STEPTYPE& stepsFromCentre, const STEPTYPE& finetune) const
//----------------------------------------------------------------------------------------------------
{
	if(stepsFromCentre < m_StepMin) return BelowRatios(stepsFromCentre, finetune);
	if(stepsFromCentre >= m_StepMin + static_cast<RATIOTYPE>(m_RatioTable.size())) return AboveRatios(stepsFromCentre, finetune);
	return m_RatioTable[stepsFromCentre - m_StepMin];
}

bool CTuningRTI::SetRatio(const STEPTYPE& s, const RATIOTYPE& r)
//--------------------------------------------------------------
{
	if(MayEdit(EM_RATIOS))
	{
		//Creating ratio table if doesnt exist.
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
		
		SetType(TT_GENERAL);
		//Not checking whether the tuning type changed;
		//simply setting it to general

		return false;
	}
	return true;
}

void CTuningRTI::SetType(const CTUNINGTYPE& ct)
//------------------------
{
	if(!MayEdit(EM_TYPE))
		return;

	if(ct == TT_GENERAL)
	{
		m_StepsInPeriod = 0;
		m_PeriodRatio = 0;
	}
	CTuning::SetType(ct);
}



