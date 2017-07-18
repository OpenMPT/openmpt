/*
 * tuningbase.h
 * ------------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


#include <string>
#include <vector>
#include <cmath>
#include <iosfwd>
#include <map>
#include <limits>
#include "../common/typedefs.h"


OPENMPT_NAMESPACE_BEGIN


namespace srlztn {class Ssb;}


//Tuning baseclass; basic functionality is to map note to ratio.
class CTuningBase
//===============
{
	//NOTEINDEXTYPE: Some signed integer-type.
	//UNOTEINDEXTYPE: Unsigned NOTEINDEXTYPE
	//RATIOTYPE: Some 'real figure' type able to present ratios.
	//STEPINDEXTYPE: Counter of steps between notes. If there is no 'finetune'(finestepcount == 0),
			//then 'step difference' between notes is the
			//same as differences in NOTEINDEXTYPE. In a way similar to ticks and rows in pattern -
			//ticks <-> STEPINDEX, rows <-> NOTEINDEX

public:
//BEGIN TYPEDEFS:
	typedef int16 NOTEINDEXTYPE;
	typedef uint16 UNOTEINDEXTYPE;
	typedef float32 RATIOTYPE; //If changing RATIOTYPE, serialization methods may need modifications.
	typedef int32 STEPINDEXTYPE;
	typedef uint32 USTEPINDEXTYPE;

	typedef bool SERIALIZATION_RETURN_TYPE;

	//Validity Range PAIR.
	typedef std::pair<NOTEINDEXTYPE, NOTEINDEXTYPE> VRPAIR;

	typedef uint16 TUNINGTYPE;

	typedef std::string NOTESTR;
	typedef std::map<NOTEINDEXTYPE, NOTESTR> NOTENAMEMAP;
	typedef NOTENAMEMAP::iterator NNM_ITER;
	typedef NOTENAMEMAP::const_iterator NNM_CITER;

//END TYPEDEFS


//BEGIN PUBLIC STATICS

	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_SUCCESS;
	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_FAILURE;

	static const char s_FileExtension[5];

	static const TUNINGTYPE TT_GENERAL;
	static const TUNINGTYPE TT_GROUPGEOMETRIC;
	static const TUNINGTYPE TT_GEOMETRIC;

//END PUBLIC STATICS


public:
//BEGIN TUNING INTERFACE

	//To return ratio of certain note.
	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE&) const = 0;

	//To return ratio from a 'step'(noteindex + stepindex)
	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE& s, const STEPINDEXTYPE&) const = 0;

	//To return (fine)stepcount between two consecutive mainsteps.
	virtual USTEPINDEXTYPE GetFineStepCount() const {return m_FineStepCount;}

	//To return 'directed distance' between given notes.
	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& /*from*/, const NOTEINDEXTYPE& /*to*/) const = 0;

	//To return 'directed distance' between given steps.
	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE&, const STEPINDEXTYPE&, const NOTEINDEXTYPE&, const STEPINDEXTYPE&) const = 0;


	//To set finestepcount between two consecutive mainsteps and
	//return GetFineStepCount(). This might not be the same as
	//parameter fs if something fails. Finestep count == 0 means that
	//stepdistances become the same as note distances.
	USTEPINDEXTYPE SetFineStepCount(const USTEPINDEXTYPE& fs);

	//Multiply all ratios by given number.
	virtual bool Multiply(const RATIOTYPE&);

	//Create GroupGeometric tuning of *this using virtual ProCreateGroupGeometric.
	bool CreateGroupGeometric(const std::vector<RATIOTYPE>&, const RATIOTYPE&, const VRPAIR vr, const NOTEINDEXTYPE ratiostartpos);

	//Create GroupGeometric of *this using ratios from 'itself' and ratios starting from
	//position given as third argument.
	bool CreateGroupGeometric(const NOTEINDEXTYPE&, const RATIOTYPE&, const NOTEINDEXTYPE&);

	//Create geometric tuning of *this using ratio(0) = 1.
	bool CreateGeometric(const UNOTEINDEXTYPE& p, const RATIOTYPE& r) {return CreateGeometric(p,r,GetValidityRange());}
	bool CreateGeometric(const UNOTEINDEXTYPE&, const RATIOTYPE&, const VRPAIR vr);

	virtual SERIALIZATION_RETURN_TYPE Serialize(std::ostream& /*out*/) const = 0;

	NOTESTR GetNoteName(const NOTEINDEXTYPE& x, bool addOctave = true) const;

	void SetName(const std::string& s);

	std::string GetName() const {return m_TuningName;}

	bool SetNoteName(const NOTEINDEXTYPE&, const std::string&);

	bool ClearNoteName(const NOTEINDEXTYPE& n, const bool clearAll = false);

	bool SetRatio(const NOTEINDEXTYPE& s, const RATIOTYPE& r);

	TUNINGTYPE GetType() const {return m_TuningType;}

	bool ChangeGroupsize(const NOTEINDEXTYPE&);
	bool ChangeGroupRatio(const RATIOTYPE&);

	virtual UNOTEINDEXTYPE GetGroupSize() const = 0;
	virtual RATIOTYPE GetGroupRatio() const = 0;


	//Tuning might not be valid for arbitrarily large range,
	//so this can be used to ask where it is valid. Tells the lowest and highest
	//note that are valid.
	virtual VRPAIR GetValidityRange() const = 0;


	//Return true if note is within validity range - false otherwise.
	bool IsValidNote(const NOTEINDEXTYPE n) const {return (n >= GetValidityRange().first && n <= GetValidityRange().second);}

	//Checking that step distances can be presented with
	//value range of STEPINDEXTYPE with given finestepcount and validityrange.
	bool IsStepCountRangeSufficient(USTEPINDEXTYPE fs, VRPAIR vrp);

	bool DeserializeOLD(std::istream&);

	virtual ~CTuningBase() {};

//END TUNING INTERFACE

//BEGIN PROTECTED VIRTUALS
protected:
	//Return value: true if change was not done, and false otherwise, in case which
	//tuningtype is automatically changed to general.
	virtual bool ProSetRatio(const NOTEINDEXTYPE&, const RATIOTYPE&) = 0;

	virtual NOTESTR ProGetNoteName(const NOTEINDEXTYPE&, bool) const;

	//The two methods below return false if action was done, true otherwise.
	virtual bool ProCreateGroupGeometric(const std::vector<RATIOTYPE>&, const RATIOTYPE&, const VRPAIR&, const NOTEINDEXTYPE /*ratiostartpos*/) = 0;
	virtual bool ProCreateGeometric(const UNOTEINDEXTYPE&, const RATIOTYPE&, const VRPAIR&) = 0;
	
	virtual void ProSetFineStepCount(const USTEPINDEXTYPE&) = 0;

	virtual NOTEINDEXTYPE ProSetGroupSize(const UNOTEINDEXTYPE&) = 0;
	virtual RATIOTYPE ProSetGroupRatio(const RATIOTYPE&) = 0;

//END PROTECTED VIRTUALS



//PROTECTED INTERFACE

protected:

	//Return true if data loading failed, false otherwise.
	virtual bool ProProcessUnserializationdata(UNOTEINDEXTYPE ratiotableSize) = 0;

//END PROTECTED INTERFACE


//BEGIN: DATA MEMBERS
protected:
	std::string m_TuningName;
	TUNINGTYPE m_TuningType;
	NOTENAMEMAP m_NoteNameMap;
	USTEPINDEXTYPE m_FineStepCount;

//END DATA MEMBERS

protected:
	CTuningBase(const std::string name = "Unnamed") :
		m_TuningName(name),
		m_TuningType(TT_GENERAL), //Unspecific tuning by default.
		m_FineStepCount(0)
		{}
private:
	static void ReadNotenamemapPair(std::istream& iStrm, NOTENAMEMAP::value_type& val, const size_t);
	static void WriteNotenamemappair(std::ostream& oStrm, const NOTENAMEMAP::value_type& val, const size_t);

};

#define NOTEINDEXTYPE_MIN (std::numeric_limits<NOTEINDEXTYPE>::min)()
#define NOTEINDEXTYPE_MAX (std::numeric_limits<NOTEINDEXTYPE>::max)()
#define UNOTEINDEXTYPE_MAX (std::numeric_limits<UNOTEINDEXTYPE>::max)()
#define STEPINDEXTYPE_MIN (std::numeric_limits<STEPINDEXTYPE>::min)()
#define STEPINDEXTYPE_MAX (std::numeric_limits<STEPINDEXTYPE>::max)()
#define USTEPINDEXTYPE_MAX (std::numeric_limits<USTEPINDEXTYPE>::max)()



inline void CTuningBase::SetName(const std::string& s)
//-----------------------------------------------
{
	m_TuningName = s;
}


inline bool CTuningBase::IsStepCountRangeSufficient(USTEPINDEXTYPE fs, VRPAIR vrp)
//--------------------------------------------------------------------------------
{
	{ // avoid integer overload
		//if(vrp.first == STEPINDEXTYPE_MIN && vrp.second == STEPINDEXTYPE_MAX) return true;
		MPT_ASSERT(NOTEINDEXTYPE_MIN / 2 < vrp.first && vrp.second < NOTEINDEXTYPE_MAX / 2);
		if(NOTEINDEXTYPE_MIN / 2 >= vrp.first || vrp.second >= NOTEINDEXTYPE_MAX / 2) return true;
	}
	if(fs > static_cast<USTEPINDEXTYPE>(STEPINDEXTYPE_MAX) / (vrp.second - vrp.first + 1)) return false;
	else return true;
}


OPENMPT_NAMESPACE_END
