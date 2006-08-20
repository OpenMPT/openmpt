#ifndef CTUNING_H
#define CTUNING_H

#include "tuning_template.h"



//================================
class CTuningRTI : public CTuning //RTI <-> Ratio Table Implementation
//================================
{
	//Class description: General tuning class with
	//ratiotable implementation.

public: 
//BEGIN TYPEDEFS:
	typedef RATIOTYPE (BARFUNC)(const STEPTYPE&, const STEPTYPE&);
	//BARFUNC <-> Beyond Array Range FUNCtion.
//END TYPEDEFS

public:
//BEGIN STATIC CONST MEMBERS:
	static RATIOTYPE DefaultBARFUNC(const STEPTYPE&, const STEPTYPE&);
	static const STEPTYPE s_StepMinDefault;
	static const STEPTYPE s_RatioTableSizeDefault;
	static const SERIALIZATION_MARKER s_SerializationBeginMarker;
	static const SERIALIZATION_MARKER s_SerializationEndMarker;
	static const SERIALIZATION_VERSION s_SerializationVersion;
//END STATIC CONST MEMBERS


public:
//BEGIN TUNING INTERFACE METHODS:
	virtual RATIOTYPE GetFrequencyRatio(const STEPTYPE& stepsFromCentre, const STEPTYPE& fineSteps) const;

	virtual STEPTYPE GetRatioTableSize() const {return static_cast<STEPTYPE>(m_RatioTable.size());}

	virtual STEPTYPE GetRatioTableBeginNote() const {return m_StepMin;}

	VRPAIR GetValidityRange() const {return VRPAIR(static_cast<STEPTYPE>(m_StepMin), static_cast<STEPTYPE>(m_StepMin + m_RatioTable.size()));}

	virtual string GetNoteName(const STEPTYPE& x) const;

	STEPTYPE GetPeriod() const {return m_StepsInPeriod;}

	RATIOTYPE GetPeriodRatio() const {return m_PeriodRatio;}

	bool SetRatio(const STEPTYPE&, const RATIOTYPE&);

	virtual SERIALIZATION_RETURN_TYPE SerializeBinary(ostream&, const int mode = 0) const;

	virtual SERIALIZATION_RETURN_TYPE UnSerializeBinary(istream&, const int mode = 0);

	bool CreateRatioPeriodic(const vector<RATIOTYPE>&, const RATIOTYPE&);

	bool CreateTET(const STEPTYPE&, const RATIOTYPE&);

public:
	//PUBLIC CONSTRUCTORS/DESTRUCTORS:
	CTuningRTI(const vector<RATIOTYPE>& ratios,
				const STEPTYPE& stepMin = s_StepMinDefault,
				const string& name = "")
				: CTuning(name)
	{
		SetDummyValues();
		m_StepMin = stepMin;
		m_RatioTable = ratios;
	}


	CTuningRTI(const CTuning*& pTun);
	//Copy tuning.

	CTuningRTI() {SetDummyValues();}

	CTuningRTI(const string& name) : CTuning(name)
	{
		SetDummyValues();
	}
	
	CTuningRTI(const STEPTYPE& stepMin, const string& name) : CTuning(name)
	{
		SetDummyValues();
		m_StepMin = stepMin;
	}

	virtual ~CTuningRTI() {}


protected:
//BEGIN PROTECTED CLASS SPECIFIC METHODS:
	bool CreateRatioTableRP(const vector<RATIOTYPE>&, const RATIOTYPE);
	//Ratioperiodic.

	void SetType(const CTUNINGTYPE&);

private:
	//PRIVATE METHODS:
	void SetDummyValues();
    //Sets dummy values for *this.

	bool IsNoteInTable(const STEPTYPE& s) const
	{
		if(s < m_StepMin || s >= m_StepMin + static_cast<RATIOTYPE>(m_RatioTable.size()))
			return false;
		return true;
	}

private:
	//ACTUAL DATA MEMBERS
	//NOTE: Update SetDummyValues when adding members.
	vector<RATIOTYPE> m_RatioTable;
	//Array tells tuning ratios. Centre is given by s_StepMin(see its explanation).

	STEPTYPE m_StepMin;
	//When wanting to get ratio n steps below reference, the n is negative, but the index
	//cannot be negative when getting the ratio for the note. So simply choosing that 
	//the maximum deviation to 'negative side' corresponds to index 0 => the centre ratio
	//will be at m_RatioTable[m_MinStep].

	STEPTYPE m_StepsInPeriod;
	RATIOTYPE m_PeriodRatio;
	//If tuning is ratioperiodic, meaning that if exists such
	//n > 0 that ratio(x + n)/ratio(x) == c == constant, the tuning
	//is considered 'ratioperiodic' with m_StepsInPeriod
	//being n, and m_PeriodRatio = c. Are zero they tuning
	//is not ratioperiodic.

	BARFUNC* BelowRatios;
	BARFUNC* AboveRatios;
	//Defines the ratio to return if the ratio table runs out. Are on status
	//'just in case for possible use'.
	
	//<----Actual data members
}; //End: CTuningRTI declaration.




#endif
