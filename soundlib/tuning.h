/*
 * tuning.h
 * --------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "tuningbase.h"


OPENMPT_NAMESPACE_BEGIN


//===================================
class CTuningRTI : public CTuningBase //RTI <-> Ratio Table Implementation
//===================================
{

public:
//BEGIN STATIC CONST MEMBERS:
	static const RATIOTYPE s_DefaultFallbackRatio;
	static const NOTEINDEXTYPE s_StepMinDefault = -64;
	static const UNOTEINDEXTYPE s_RatioTableSizeDefault = 128;
	static const USTEPINDEXTYPE s_RatioTableFineSizeMaxDefault = 1000;
//END STATIC CONST MEMBERS


public:
//BEGIN TUNING INTERFACE METHODS:
	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE& stepsFromCentre) const;

	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE& stepsFromCentre, const STEPINDEXTYPE& fineSteps) const;

	virtual UNOTEINDEXTYPE GetRatioTableSize() const {return static_cast<UNOTEINDEXTYPE>(m_RatioTable.size());}

	virtual NOTEINDEXTYPE GetRatioTableBeginNote() const {return m_StepMin;}

	VRPAIR GetValidityRange() const {return VRPAIR(m_StepMin, static_cast<NOTEINDEXTYPE>(m_StepMin + static_cast<NOTEINDEXTYPE>(m_RatioTable.size()) - 1));}

	virtual UNOTEINDEXTYPE GetGroupSize() const {return m_GroupSize;}

	virtual RATIOTYPE GetGroupRatio() const {return m_GroupRatio;}

	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& from, const NOTEINDEXTYPE& to) const
		{return (to - from)*(static_cast<NOTEINDEXTYPE>(GetFineStepCount())+1);}

	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& noteFrom, const STEPINDEXTYPE& stepDistFrom, const NOTEINDEXTYPE& noteTo, const STEPINDEXTYPE& stepDistTo) const
		{return GetStepDistance(noteFrom, noteTo) + stepDistTo - stepDistFrom;}

	static CTuningRTI* Deserialize(std::istream& inStrm);

	//Try to read old version (v.3) and return pointer to new instance if succesfull, else nullptr.
	static CTuningRTI* DeserializeOLD(std::istream&);

	SERIALIZATION_RETURN_TYPE Serialize(std::ostream& out) const;

#ifdef MODPLUG_TRACKER
	bool WriteSCL(std::ostream &f, const mpt::PathString &filename) const;
#endif

	bool UpdateRatioGroupGeometric(NOTEINDEXTYPE s, RATIOTYPE r);

public:
	//PUBLIC CONSTRUCTORS/DESTRUCTORS:

	CTuningRTI() {SetDummyValues();}

	virtual ~CTuningRTI() {}

//BEGIN PROTECTED VIRTUALS:
protected:
	virtual bool ProSetRatio(const NOTEINDEXTYPE&, const RATIOTYPE&);
	virtual bool ProCreateGroupGeometric(const std::vector<RATIOTYPE>&, const RATIOTYPE&, const VRPAIR&, const NOTEINDEXTYPE ratiostartpos);
	virtual bool ProCreateGeometric(const UNOTEINDEXTYPE&, const RATIOTYPE&, const VRPAIR&);
	virtual void ProSetFineStepCount(const USTEPINDEXTYPE&);

	virtual NOTESTR ProGetNoteName(const NOTEINDEXTYPE& xi, bool addOctave) const;

	//Note: Groupsize is restricted to interval [0, NOTEINDEXTYPE_MAX]
	NOTEINDEXTYPE ProSetGroupSize(const UNOTEINDEXTYPE& p) {return m_GroupSize = (p<=static_cast<UNOTEINDEXTYPE>(NOTEINDEXTYPE_MAX)) ? static_cast<NOTEINDEXTYPE>(p) : NOTEINDEXTYPE_MAX;}
	RATIOTYPE ProSetGroupRatio(const RATIOTYPE& pr) {return m_GroupRatio = (pr >= 0) ? pr : -pr;}

	virtual bool ProProcessUnserializationdata(UNOTEINDEXTYPE ratiotableSize);


//END PROTECTED VIRTUALS

protected:
//BEGIN PROTECTED CLASS SPECIFIC METHODS:
	//GroupGeometric.
	bool CreateRatioTableGG(const std::vector<RATIOTYPE>&, const RATIOTYPE, const VRPAIR& vr, const NOTEINDEXTYPE ratiostartpos);

	//Note: Stepdiff should be in range [1, finestepcount]
	virtual RATIOTYPE GetRatioFine(const NOTEINDEXTYPE& note, USTEPINDEXTYPE stepDiff) const;

	//GroupPeriodic-specific.
	//Get the corresponding note in [0, period-1].
	//For example GetRefNote(-1) is to return note :'groupsize-1'.
	NOTEINDEXTYPE GetRefNote(NOTEINDEXTYPE note) const;

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
	std::vector<RATIOTYPE> m_RatioTable;

	//'Fineratios'
	std::vector<RATIOTYPE> m_RatioTableFine;

	//The lowest index of note in the table
	NOTEINDEXTYPE m_StepMin; // this should REALLY be called 'm_NoteMin' renaming was missed in r192

	//For groupgeometric tunings, tells the 'group size' and 'group ratio'
	//m_GroupSize should always be >= 0.
	NOTEINDEXTYPE m_GroupSize;
	RATIOTYPE m_GroupRatio;

	//<----Actual data members

}; //End: CTuningRTI declaration.


typedef CTuningRTI CTuning;


OPENMPT_NAMESPACE_END
