#ifndef CTUNING_H
#define CTUNING_H

#include "tuningbase.h"

#ifdef BUILD_TUNINGBASE_AS_TEMPLATE
	typedef CTuningBase<int16, uint16, float32, int32, uint32> CTuning;
	//If changing RATIOTYPE, serialization methods may need modifications.
#else
	typedef CTuningBase CTuning;
#endif



//================================
class CTuningRTI : public CTuning //RTI <-> Ratio Table Implementation
//================================
{
public: 
//BEGIN TYPEDEFS:
	typedef RATIOTYPE (BARFUNC)(const NOTEINDEXTYPE&, const STEPINDEXTYPE&);
	//BARFUNC <-> Beyond Array Range FUNCtion.
//END TYPEDEFS

public:
//BEGIN STATIC CONST MEMBERS:
	static RATIOTYPE DefaultBARFUNC(const NOTEINDEXTYPE&, const STEPINDEXTYPE&);
	static const NOTEINDEXTYPE s_StepMinDefault;
	static const UNOTEINDEXTYPE s_RatioTableSizeDefault;
	static const STEPINDEXTYPE s_RatioTableFineSizeMaxDefault;
	static const SERIALIZATION_VERSION s_SerializationVersion;
//END STATIC CONST MEMBERS


public:
//BEGIN TUNING INTERFACE METHODS:
	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE& stepsFromCentre) const;

	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE& stepsFromCentre, const STEPINDEXTYPE& fineSteps) const;

	virtual UNOTEINDEXTYPE GetRatioTableSize() const {return static_cast<UNOTEINDEXTYPE>(m_RatioTable.size());}

	virtual NOTEINDEXTYPE GetRatioTableBeginNote() const {return m_StepMin;}

	VRPAIR GetValidityRange() const {return VRPAIR(m_StepMin, m_StepMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size()) - 1);}

	UNOTEINDEXTYPE GetGroupSize() const {return m_GroupSize;}

	RATIOTYPE GetGroupRatio() const {return m_GroupRatio;}

	void ProAddSI(srlztn::ABCSerializationInstructions&, const BYTE[3]) const;

	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& from, const NOTEINDEXTYPE& to) const
		{return (to - from)*(static_cast<NOTEINDEXTYPE>(GetFineStepCount())+1);}
	
	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& noteFrom, const STEPINDEXTYPE& stepDistFrom, const NOTEINDEXTYPE& noteTo, const STEPINDEXTYPE& stepDistTo) const
		{return GetStepDistance(noteFrom, noteTo) + stepDistTo - stepDistFrom;}
	
	static CTuning* CreateRTITuning(const string&, srlztn::ABCSerializationInstructions&);

	static uint32 GetVersion() {return s_SerializationVersion;}

	//Try to read old version (v.3) and return pointer to new instance if succesfull, else 0.
	static CTuningRTI* UnserializeOLD(istream& iStrm);
	

public:
	//PUBLIC CONSTRUCTORS/DESTRUCTORS:
	CTuningRTI(const vector<RATIOTYPE>& ratios,
				const NOTEINDEXTYPE& stepMin = s_StepMinDefault,
				const string& name = "")
				: CTuning(name)
	{
		SetDummyValues();
		m_StepMin = stepMin;
		m_RatioTable = ratios;
	}

	//Copy tuning.
	CTuningRTI(const CTuning* const pTun);
	
	CTuningRTI() {SetDummyValues();}

	CTuningRTI(const string& name) : CTuning(name) {SetDummyValues();}
	
	CTuningRTI(const NOTEINDEXTYPE& stepMin, const string& name) : CTuning(name)
	{
		SetDummyValues();
		m_StepMin = stepMin;
	}

	virtual ~CTuningRTI() {}

//BEGIN PROTECTED VIRTUALS:
protected:
	bool ProSetRatio(const NOTEINDEXTYPE&, const RATIOTYPE&);
	bool ProCreateGroupGeometric(const vector<RATIOTYPE>&, const RATIOTYPE&, const VRPAIR&, const NOTEINDEXTYPE ratiostartpos);
	bool ProCreateGeometric(const UNOTEINDEXTYPE&, const RATIOTYPE&, const VRPAIR&);
	void ProSetFineStepCount(const USTEPINDEXTYPE&);

	virtual NOTESTR ProGetNoteName(const NOTEINDEXTYPE& x) const;

	//Not implemented.
	VRPAIR ProSetValidityRange(const VRPAIR&);

	//Note: Groupsize is restricted to interval [0, NOTEINDEXTYPE_MAX]
	NOTEINDEXTYPE ProSetGroupSize(const UNOTEINDEXTYPE& p) {return m_GroupSize = (p<=static_cast<UNOTEINDEXTYPE>(NOTEINDEXTYPE_MAX)) ? static_cast<NOTEINDEXTYPE>(p) : NOTEINDEXTYPE_MAX;}
	RATIOTYPE ProSetGroupRatio(const RATIOTYPE& pr) {return m_GroupRatio = (pr >= 0) ? pr : -pr;}

	virtual uint32 GetClassVersion() const {return GetVersion();}

	virtual bool ProProcessUnserializationdata();
	

//END PROTECTED VIRTUALS

protected:
//BEGIN PROTECTED CLASS SPECIFIC METHODS:
	//GroupGeometric.
	bool CreateRatioTableGG(const vector<RATIOTYPE>&, const RATIOTYPE, const VRPAIR& vr, const NOTEINDEXTYPE ratiostartpos);

	//Note: Stepdiff should be in range [1, finestepcount]
	virtual RATIOTYPE GetRatioFine(const NOTEINDEXTYPE& note, USTEPINDEXTYPE stepDiff) const;

	//GroupPeriodic-specific. 
	//Get the corresponding note in [0, period-1].
	//For example GetRefNote(-1) is to return note :'groupsize-1'.
	NOTEINDEXTYPE GetRefNote(NOTEINDEXTYPE note) const;

	virtual const string& GetDerivedClassID() const {return s_DerivedclassID;}

private:
	//PRIVATE METHODS:

	//Sets dummy values for *this.
	void SetDummyValues();
    
	bool IsNoteInTable(const NOTEINDEXTYPE& s) const
	{
		if(s < m_StepMin || s >= m_StepMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size()))
			return false;
		else
			return true;
	}

private:
	//ACTUAL DATA MEMBERS
	//NOTE: Update SetDummyValues when adding members.

	//Noteratios
	vector<RATIOTYPE> m_RatioTable;
	
	//'Fineratios'
	vector<RATIOTYPE> m_RatioTableFine;
	
	//The lowest index of note in the table
	NOTEINDEXTYPE m_StepMin;
	
	//For groupgeometric tunings, tells the 'group size' and 'group ratio'
	//m_GroupSize should always be >= 0.
	NOTEINDEXTYPE m_GroupSize;
	RATIOTYPE m_GroupRatio;


	BARFUNC* BelowRatios;
	BARFUNC* AboveRatios;
	//Defines the ratio to return if the ratio table runs out.
	
	//<----Actual data members

	mutable UNOTEINDEXTYPE m_SerHelperRatiotableSize;

	static const string s_DerivedclassID;
	

}; //End: CTuningRTI declaration.






#endif
