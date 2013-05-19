/*
 * tuningbase.h
 * ------------
 * Purpose: Alternative sample tuning.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

//#define BUILD_TUNINGBASE_AS_TEMPLATE

#if defined(TUNINGBASE_CPP) && defined(BUILD_TUNINGBASE_AS_TEMPLATE)
	//Not including this file for tuningbase.cpp when building as template.
#else

#ifndef TUNINGBASE_H
#define TUNINGBASE_H


#include <string>
#include <vector>
#include <cmath>
#include <istream>
#include <ostream>
#include <map>
#include <limits>
#include "../common/misc_util.h"
#include "../common/typedefs.h"
using std::string;
using std::vector;
using std::istream;
using std::ostream;
using std::map;

namespace srlztn {class Ssb;};


#ifdef BUILD_TUNINGBASE_AS_TEMPLATE
	#define CLASSTEMPLATEDEC template<class TNOTEINDEXTYPE = int16, class TUNOTEINDEXTYPE = uint16, class TRATIOTYPE = float32, class TSTEPINDEXTYPE = int32, class TUSTEPINDEXTYPE = uint32> 
	#define TEMPLATEDEC template<class A, class B, class C, class D, class E>
	#define TYPENAME typename
	#define CTUNINGBASE CTuningBase<A, B, C, D, E>
#else
	#define CLASSTEMPLATEDEC
	typedef int16 TNOTEINDEXTYPE;
	typedef uint16 TUNOTEINDEXTYPE;
	typedef float32 TRATIOTYPE;
	typedef int32 TSTEPINDEXTYPE;
	typedef uint32 TUSTEPINDEXTYPE;
	#define TYPENAME
	#define TEMPLATEDEC
	#define CTUNINGBASE CTuningBase
#endif

#ifndef BYTE
	typedef unsigned char BYTE;
#endif


//Tuning baseclass; basic functionality is to map note to ratio.
CLASSTEMPLATEDEC
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
	typedef TNOTEINDEXTYPE NOTEINDEXTYPE;
	typedef TUNOTEINDEXTYPE UNOTEINDEXTYPE;
	typedef TRATIOTYPE RATIOTYPE;
	typedef TSTEPINDEXTYPE STEPINDEXTYPE;
	typedef TUSTEPINDEXTYPE USTEPINDEXTYPE;
	typedef void (*MESSAGEHANDLER)(const char*, const char*);

	typedef int16 SERIALIZATION_VERSION;
	typedef bool SERIALIZATION_RETURN_TYPE;

	//Validity Range PAIR.
	typedef std::pair<NOTEINDEXTYPE, NOTEINDEXTYPE> VRPAIR;

	typedef uint16 EDITMASK;

	typedef uint16 TUNINGTYPE;

	typedef std::string NOTESTR;
	typedef std::map<NOTEINDEXTYPE, NOTESTR> NOTENAMEMAP;
	typedef TYPENAME NOTENAMEMAP::iterator NNM_ITER;
	typedef TYPENAME NOTENAMEMAP::const_iterator NNM_CITER;

//END TYPEDEFS


//BEGIN PUBLIC STATICS
	static const SERIALIZATION_VERSION s_SerializationVersion;

	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_SUCCESS;
	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_FAILURE;

	static const TCHAR s_FileExtension[5];

	static const EDITMASK EM_RATIOS;
	static const EDITMASK EM_NOTENAME;
	static const EDITMASK EM_TYPE;
	static const EDITMASK EM_NAME;
	static const EDITMASK EM_FINETUNE;
	static const EDITMASK EM_EDITMASK;
	static const EDITMASK EM_VALIDITYRANGE;

	static const EDITMASK EM_ALLOWALL;
	static const EDITMASK EM_CONST;
	static const EDITMASK EM_CONST_STRICT; //This won't allow even changing const status
	//NOTE: When adding editmasks, check definition of EDITMASK -
	//might need to be modified.

	static const TUNINGTYPE TT_GENERAL;
	static const TUNINGTYPE TT_GROUPGEOMETRIC;
	static const TUNINGTYPE TT_GEOMETRIC;

//END PUBLIC STATICS


public:
//BEGIN TUNING INTERFACE

	//To return ratio of certain note.
	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE&) const {return 0;}

	//To return ratio from a 'step'(noteindex + stepindex)
	virtual RATIOTYPE GetRatio(const NOTEINDEXTYPE& s, const STEPINDEXTYPE&) const {return GetRatio(s);}

	//To return (fine)stepcount between two consecutive mainsteps.
	virtual USTEPINDEXTYPE GetFineStepCount() const {return m_FineStepCount;}

	//To return 'directed distance' between given notes.
	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE& /*from*/, const NOTEINDEXTYPE& /*to*/) const {return 0;}

	//To return 'directed distance' between given steps.
	virtual STEPINDEXTYPE GetStepDistance(const NOTEINDEXTYPE&, const STEPINDEXTYPE&, const NOTEINDEXTYPE&, const STEPINDEXTYPE&) const {return 0;}


	//To set finestepcount between two consecutive mainsteps and
	//return GetFineStepCount(). This might not be the same as
	//parameter fs if something fails. Finestep count == 0 means that
	//stepdistances become the same as note distances.
	USTEPINDEXTYPE SetFineStepCount(const USTEPINDEXTYPE& fs);

	//Multiply all ratios by given number.
	virtual bool Multiply(const RATIOTYPE&);

	//Create GroupGeometric tuning of *this using virtual ProCreateGroupGeometric.
	bool CreateGroupGeometric(const vector<RATIOTYPE>&, const RATIOTYPE&, const VRPAIR vr, const NOTEINDEXTYPE ratiostartpos);

	//Create GroupGeometric of *this using ratios from 'itself' and ratios starting from 
	//position given as third argument.
	bool CreateGroupGeometric(const NOTEINDEXTYPE&, const RATIOTYPE&, const NOTEINDEXTYPE&);

	//Create geometric tuning of *this using ratio(0) = 1.
	bool CreateGeometric(const UNOTEINDEXTYPE& p, const RATIOTYPE& r) {return CreateGeometric(p,r,GetValidityRange());}
	bool CreateGeometric(const UNOTEINDEXTYPE&, const RATIOTYPE&, const VRPAIR vr);

	virtual SERIALIZATION_RETURN_TYPE Serialize(ostream& /*out*/) const {return false;}

	NOTESTR GetNoteName(const NOTEINDEXTYPE& x) const;

	void SetName(const string& s);
	
	string GetName() const {return m_TuningName;}

	bool SetNoteName(const NOTEINDEXTYPE&, const string&);

	bool ClearNoteName(const NOTEINDEXTYPE& n, const bool clearAll = false);

	bool SetRatio(const NOTEINDEXTYPE& s, const RATIOTYPE& r);

	TUNINGTYPE GetTuningType() const {return m_TuningType;}

	static string GetTuningTypeStr(const TUNINGTYPE& tt);
	static TUNINGTYPE GetTuningType(const char* str);

	bool IsOfType(const TUNINGTYPE& type) const;

	bool ChangeGroupsize(const NOTEINDEXTYPE&);
	bool ChangeGroupRatio(const RATIOTYPE&);

	static uint32 GetVersion() {return s_SerializationVersion;}

	virtual UNOTEINDEXTYPE GetGroupSize() const {return 0;}
	virtual RATIOTYPE GetGroupRatio() const {return 0;}


	//Tuning might not be valid for arbitrarily large range,
	//so this can be used to ask where it is valid. Tells the lowest and highest
	//note that are valid.
	virtual VRPAIR GetValidityRange() const {return VRPAIR(0,0);}


	//To try to set validity range to given range; returns
	//the range valid after trying to set the new range.
	VRPAIR SetValidityRange(const VRPAIR& vrp);

	//Return true if note is within validity range - false otherwise.
	bool IsValidNote(const NOTEINDEXTYPE n) const {return (n >= GetValidityRange().first && n <= GetValidityRange().second);}

	//Checking that step distances can be presented with
	//value range of STEPINDEXTYPE with given finestepcount ja validityrange.
	bool IsStepCountRangeSufficient(USTEPINDEXTYPE fs, VRPAIR vrp);
	
	virtual const char* GetTuningTypeDescription() const;

	static const char* GetTuningTypeDescription(const TUNINGTYPE&);

	bool MayEdit(const EDITMASK& em) const {return (em & m_EditMask) != 0;}

	bool SetEditMask(const EDITMASK& em);

	EDITMASK GetEditMask() const {return m_EditMask;}

	bool DeserializeOLD(istream&);

	virtual ~CTuningBase() {};

//END TUNING INTERFACE

//BEGIN PROTECTED VIRTUALS
protected:
	//Return value: true if change was not done, and false otherwise, in case which
	//tuningtype is automatically changed to general.
	virtual bool ProSetRatio(const NOTEINDEXTYPE&, const RATIOTYPE&) {return true;}

	virtual NOTESTR ProGetNoteName(const NOTEINDEXTYPE&) const;

	//The two methods below return false if action was done, true otherwise.
	virtual bool ProCreateGroupGeometric(const vector<RATIOTYPE>&, const RATIOTYPE&, const VRPAIR&, const NOTEINDEXTYPE /*ratiostartpos*/) {return true;}
	virtual bool ProCreateGeometric(const UNOTEINDEXTYPE&, const RATIOTYPE&, const VRPAIR&) {return true;}
	
	virtual VRPAIR ProSetValidityRange(const VRPAIR&) {return GetValidityRange();}

	virtual void ProSetFineStepCount(const USTEPINDEXTYPE&) {}

	virtual NOTEINDEXTYPE ProSetGroupSize(const UNOTEINDEXTYPE&) {return 0;}
	virtual RATIOTYPE ProSetGroupRatio(const RATIOTYPE&) {return 0;}

	virtual uint32 GetClassVersion() const = 0;

//END PROTECTED VIRTUALS



//PROTECTED INTERFACE
protected:
	static void TuningCopy(CTuningBase& to, const CTuningBase& from, const bool allowExactnamecopy = false);

	TUNINGTYPE GetType() const {return m_TuningType;}

	//This is appended to baseclassID in serialization with which objects are identified when loading.
	virtual const string& GetDerivedClassID() const = 0;

	//Return true if data loading failed, false otherwise.
	virtual bool ProProcessUnserializationdata() = 0;


//END PROTECTED INTERFACE


//BEGIN PRIVATE METHODS
private:
	bool SetType(const TUNINGTYPE& tt);
//END PRIVATE METHODS


//BEGIN: DATA MEMBERS
protected:
	string m_TuningName;
	EDITMASK m_EditMask; //Behavior: true <~> allow modification
	TUNINGTYPE m_TuningType;
	NOTENAMEMAP m_NoteNameMap;
	USTEPINDEXTYPE m_FineStepCount;
	//NOTE: If adding new members, TuningCopy might need to be modified.

//END DATA MEMBERS

protected:
	CTuningBase(const string name = "Unnamed") :
		m_TuningName(name),
		m_EditMask(uint16_max), //All bits to true - allow all by default.
		m_TuningType(TT_GENERAL), //Unspecific tuning by default.
		m_FineStepCount(0)
		{}
private:
	CTuningBase(CTuningBase&) {}
	CTuningBase& operator=(const CTuningBase&) {return *this;}
	static void ReadNotenamemapPair(istream& iStrm, TYPENAME NOTENAMEMAP::value_type& val, const size_t);
	static void WriteNotenamemappair(ostream& oStrm, const TYPENAME NOTENAMEMAP::value_type& val, const size_t);

public:
	static const char* s_TuningDescriptionGeneral;
	static const char* s_TuningDescriptionGroupGeometric;
	static const char* s_TuningDescriptionGeometric;
	static const char* s_TuningTypeStrGeneral;
	static const char* s_TuningTypeStrGroupGeometric;
	static const char* s_TuningTypeStrGeometric;

private:
	static void DefaultMessageHandler(const char*, const char*) {}
};

#define NOTEINDEXTYPE_MIN (std::numeric_limits<NOTEINDEXTYPE>::min)()
#define NOTEINDEXTYPE_MAX (std::numeric_limits<NOTEINDEXTYPE>::max)()
#define UNOTEINDEXTYPE_MAX (std::numeric_limits<UNOTEINDEXTYPE>::max)()
#define STEPINDEXTYPE_MIN (std::numeric_limits<STEPINDEXTYPE>::min)()
#define STEPINDEXTYPE_MAX (std::numeric_limits<STEPINDEXTYPE>::max)()
#define USTEPINDEXTYPE_MAX (std::numeric_limits<USTEPINDEXTYPE>::max)()


TEMPLATEDEC
inline const char* CTUNINGBASE::GetTuningTypeDescription() const
//----------------------------------------------------------------------
{
	return GetTuningTypeDescription(GetType());
}

TEMPLATEDEC
inline void CTUNINGBASE::SetName(const string& s)
//-----------------------------------------------
{
	if(MayEdit(EM_NAME)) m_TuningName = s;
}

TEMPLATEDEC
inline bool CTUNINGBASE::IsStepCountRangeSufficient(USTEPINDEXTYPE fs, VRPAIR vrp)
//------------------------------------------------------------------
{
	if(vrp.first == STEPINDEXTYPE_MIN && vrp.second == STEPINDEXTYPE_MAX) return true;
	if(fs > static_cast<USTEPINDEXTYPE>(STEPINDEXTYPE_MAX) / (vrp.second - vrp.first + 1)) return false;
	else return true;
}

TEMPLATEDEC
inline bool CTUNINGBASE::SetEditMask(const EDITMASK& em)
//------------------------------------------------------
{
	if(MayEdit(EM_EDITMASK))
		{m_EditMask = em; return false;}
	else
		return true;
}



#if defined(BUILD_TUNINGBASE_AS_TEMPLATE)
	#include "tuningbase.cpp"
#endif

#endif
#endif

