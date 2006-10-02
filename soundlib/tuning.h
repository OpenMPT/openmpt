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
	virtual RATIOTYPE GetFrequencyRatio(const STEPTYPE& stepsFromCentre) const;

	virtual RATIOTYPE GetFrequencyRatio(const STEPTYPE& stepsFromCentre, const FINESTEPTYPE& fineSteps) const;

	virtual RATIOTYPE GetFrequencyRatioFine(const FINESTEPTYPE&) {return 1;}

	virtual STEPTYPE GetRatioTableSize() const {return static_cast<STEPTYPE>(m_RatioTable.size());}

	virtual STEPTYPE GetRatioTableBeginNote() const {return m_StepMin;}

	VRPAIR GetValidityRange() const {return VRPAIR(static_cast<STEPTYPE>(m_StepMin), static_cast<STEPTYPE>(m_StepMin + m_RatioTable.size()));}

	virtual string GetNoteName(const STEPTYPE& x) const;

	STEPTYPE GetPeriod() const {return m_StepsInPeriod;}

	RATIOTYPE GetPeriodRatio() const {return m_PeriodRatio;}

	virtual SERIALIZATION_RETURN_TYPE SerializeBinary(ostream&, const int mode = 0) const;

	virtual SERIALIZATION_RETURN_TYPE UnSerializeBinary(istream&, const int mode = 0);

	FINESTEPTYPE GetFineStepCount() const {return static_cast<FINESTEPTYPE>(m_RatioTableFine.size());}
	//To be improved.

	FINESTEPTYPE GetFineStepCount(const STEPTYPE& from, const STEPTYPE& to) const
	{return (to - from)*GetFineStepCount();}
	//To be improved.

	virtual FINESTEPTYPE GetFineStepCount(const STEPTYPE& fromStep, const FINESTEPTYPE& fromFineSteps, const STEPTYPE& toStep, const FINESTEPTYPE& toFineSteps) const
	{return GetFineStepCount(fromStep, toStep) + toFineSteps - fromFineSteps;}

	VRPAIR SetValidityRange(const VRPAIR& rp);
	

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

//BEGIN PROTECTED VIRTUALS:
protected:
	bool ProSetRatio(const STEPTYPE&, const RATIOTYPE&);
	bool ProCreateRatioPeriodic(const vector<RATIOTYPE>&, const RATIOTYPE&);
	bool ProCreateTET(const STEPTYPE&, const RATIOTYPE&);
	FINESTEPTYPE ProSetFineStepCount(const STEPTYPE&);
	STEPTYPE ProSetPeriod(const STEPTYPE& p) {return m_StepsInPeriod = (p>=0) ? p : -p;}
	RATIOTYPE ProSetPeriodRatio(const RATIOTYPE& pr) {return m_PeriodRatio = (pr >= 0) ? pr : -pr;}

	virtual RATIOTYPE ProSetRatioFine(const FINESTEPTYPE&, const RATIOTYPE&) const {return 0;}
	//For now finestepcount defines the fineratios, so
	//this method doesn't modify the fineratios.

//END PROTECTED VIRTUALS

protected:
//BEGIN PROTECTED CLASS SPECIFIC METHODS:
	bool CreateRatioTableRP(const vector<RATIOTYPE>&, const RATIOTYPE);
	//Ratioperiodic.
	

private:
	//PRIVATE METHODS:
	void SetDummyValues();
    //Sets dummy values for *this.

	RATIOTYPE GetFineStepRatio(const FINESTEPTYPE& s) const
	{
		if(GetFineStepCount() == 0)
			return 1;
		if(s < 0) return m_RatioTableFine[0];
		if(s >= GetFineStepCount()) return m_RatioTableFine.back();
		return m_RatioTableFine[s];
	}

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
	//Array tells (main) tuning ratios. Centre is given by s_StepMin(see its explanation).

	vector<RATIOTYPE> m_RatioTableFine;
	//To contain fineratios.

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
