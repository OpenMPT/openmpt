#ifndef TUNING_TEMPLATE_H
#define TUNING_TEMPLATE_H

#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <utility> //Pair-struct from here
#include <bitset>
#include <map>
#include "../mptrack/misc_util.h"
using std::string;
using std::vector;
using std::istream;
using std::ostream;
using std::bitset;
using std::map;

#ifndef BYTE
	typedef unsigned char BYTE;
#endif

//This is just a class for the data which is written in serializations as a ID.
template<int SIZE>
class CSerializationID
{
public:
	char m_ID[SIZE];
	bool operator==(const CSerializationID& a) const
	{
		for(int i = 0; i<SIZE; i++)
		{
			if(m_ID[i] != a.m_ID[i]) return false;
		}
		return true;
	}

	CSerializationID& operator=(const CSerializationID& a)
	{
		for(int i = 0; i<SIZE; i++) m_ID[i] = a.m_ID[i];
		return *this;
	}

	CSerializationID(const CSerializationID& a) {*this = a;}

	bool operator!=(const CSerializationID& a) const
	{
		return !(*this == a);
	}

	CSerializationID(const string& a = "")
	{
		int i = 0;
		for(i = 0; i < std::min<int>(SIZE, static_cast<int>(a.size())); i++)
		{
			m_ID[i] = a.at(i);
		}
		for(i; i<SIZE; i++) m_ID[i] = '.';


	}
};

/*
TODO: Make certain methods pure virtual?
TODO: Finetune to be notespecific?
TODO: Better return values(e.g. NOTHING_DONE, ERROR etc.)?
*/

//Class defining tuning which is fundamentally based on discrete steps.
template<class TSTEPTYPE = short int, class TUSTEPTYPE = unsigned short int,
		 class TRATIOTYPE = float,
		 class TFINESTEPTYPE = TSTEPTYPE, class TUFINESTEPTYPE = TUSTEPTYPE>
class CTuningBase
{
	//STEPTYPE: Some type that has properties that of 'ordinary' signed and discrete figures.
	//RATIOTYPE: Some 'real figure' type which is able to present ratios.

	//NOTE: Tuning can, to some extent, be thought simply as 'alias' for
	//      function from set defined by STEPTYPE to positive figures 
	//		in set defined by RATIOTYPE(similar to f: Z -> R+)

public:
//BEING TYPEDEFS:
	typedef TSTEPTYPE STEPTYPE;
	typedef TUSTEPTYPE USTEPTYPE; //Unsigned steptype
	typedef TRATIOTYPE RATIOTYPE;
	typedef TFINESTEPTYPE FINESTEPTYPE;
	typedef TUFINESTEPTYPE UFINESTEPTYPE; //Unsigned finesteptype

	typedef std::exception TUNINGEXCEPTION;

	typedef CSerializationID<8> SERIALIZATION_MARKER;
	typedef __int16 SERIALIZATION_VERSION;
	typedef bool SERIALIZATION_RETURN_TYPE;

	typedef std::pair<STEPTYPE, STEPTYPE> VRPAIR;
	//Validity Range PAIR

	typedef bitset<16> CEDITMASK;
	//If making bitset larger, see whether serialization
	//works correctly after that.

	typedef bitset<16> CTUNINGTYPE;
	//If making bitset larger, see whether serialization
	//works correctly after that.

	typedef std::string NOTESTR;
	typedef std::map<STEPTYPE, NOTESTR> NOTENAMEMAP;
	typedef USTEPTYPE SIZETYPE;
	typedef typename NOTENAMEMAP::iterator NNM_ITER;
	typedef typename NOTENAMEMAP::const_iterator NNM_CITER;

//END TYPEDEFS
	

//BEGIN PUBLIC STATICS
	static const SERIALIZATION_MARKER s_SerializationBeginMarker;
	static const SERIALIZATION_MARKER s_SerializationEndMarker;
	static const SERIALIZATION_VERSION s_SerializationVersion;

	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_SUCCESS;
	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_FAILURE;

	static const string s_FileExtension;

	static const CEDITMASK EM_MAINRATIOS;
	static const CEDITMASK EM_NOTENAME;
	static const CEDITMASK EM_TYPE;
	static const CEDITMASK EM_NAME;
	static const CEDITMASK EM_FINETUNE;
	static const CEDITMASK EM_EDITMASK;

	static const CEDITMASK EM_ALLOWALL;
	static const CEDITMASK EM_CONST;
	static const CEDITMASK EM_CONST_STRICT; //This won't allow changing const status
	//Add declarations of editmasks here.
	//NOTE: When adding, check definition of CEDITMASK - 
	//might need to be modified.

	static const CTUNINGTYPE TT_GENERAL;
	static const CTUNINGTYPE TT_RATIOPERIODIC;
	static const CTUNINGTYPE TT_TET;
	//Add declarations of tuning type specifiers here.
	//NOTE: When adding, check definition of CTUNINGTYPE - 
	//might need to be modified.

//END PUBLIC STATICS

	
public:
//BEGIN TUNING INTERFACE
	virtual RATIOTYPE GetFrequencyRatio(const STEPTYPE&) const {return 0;};
	//Returned value is to be such that
	//returnvalue * ReferenceFrequency == Frequency_of_note_stepsFromCenter_away_from_reference.
	//E.g. in 12-TET it is 2^(stepFromCenter/12)

	virtual RATIOTYPE GetFrequencyRatio(const STEPTYPE& s, const FINESTEPTYPE&) const {return GetFrequencyRatio(s);};
	//Like previous but here having additional finetune parameter - by default calling
	//previous method.

	virtual RATIOTYPE GetFrequencyRatioFine(const FINESTEPTYPE&) const {return 1;}

	virtual FINESTEPTYPE GetFineStepCount() const {return 0;}
	//To return the number of finesteps between main steps. It is not defined 
	//what to return in case of tuning in which this number is dependent on 
	//on the chosen notes.
	
	virtual FINESTEPTYPE GetFineStepCount(const STEPTYPE&, const STEPTYPE&) const {return 0;}
	//To return the number of finesteps between given notes.
	//First parameter is 'from' and second 'to'.

	virtual FINESTEPTYPE GetFineStepCount(const STEPTYPE&, const FINESTEPTYPE&, const STEPTYPE&, const FINESTEPTYPE&) const {return 0;}
	//To return the number of finesteps between given notes including finesteps.

	FINESTEPTYPE SetFineStepCount(const FINESTEPTYPE& s) {if(MayEdit(EM_FINETUNE)) return ProSetFineStepCount(s); else return GetFineStepCount();}
	//Returns the number that was set.

	virtual bool Multiply(const RATIOTYPE&);
	//Multiply all ratios by given figure.
    
	bool CreateRatioPeriodic(const vector<RATIOTYPE>&, const RATIOTYPE&);
	//Create ratioperiodic tuning of *this using 
	//virtual ProCreateRatioPeriodic.

	bool CreateRatioPeriodic(const STEPTYPE&, const RATIOTYPE&);
	//Create ratioperiodic of *this using ratios from 'itself'.

	bool CreateTET(const STEPTYPE&, const RATIOTYPE&);
	//Create TET tuning of *this. Actually this doesn't really
	//need two parameters, but with this one also has to define
	//some period.

	virtual SERIALIZATION_RETURN_TYPE SerializeBinary(ostream& out, const int mode = 0) const;
	//Purpose: This should write such binary stream, that when that stream is
	//read with UnserialiseBinary, the resulted object is identical to that 
	//which was written with this method. Parameter 'mode' can be used for
	//example for saying: "don't write serializatons id's" etc.

	virtual SERIALIZATION_RETURN_TYPE UnSerializeBinary(istream& in, const int mode = 0);
	//Purpose: See SerialiseBinary(...).

	virtual NOTESTR GetNoteName(const STEPTYPE& x) const;

	void SetName(const string& s)
	{
		if(MayEdit(EM_NAME))
			m_TuningName = s;
	}

	string GetName() const {return m_TuningName;}

	bool SetNoteName(const STEPTYPE&, const string&);

	bool ClearNoteName(const STEPTYPE& n, const bool clearAll = false);
	
	bool SetRatio(const STEPTYPE& s, const RATIOTYPE& r);
	
	CTUNINGTYPE GetTuningType() const {return m_TuningType;}

	static string GetTuningTypeStr(const CTUNINGTYPE& tt);

	bool DoesTypeInclude(const CTUNINGTYPE& type) const;

	bool ChangePeriod(const STEPTYPE&);
	bool ChangePeriodRatio(const RATIOTYPE&);

	virtual STEPTYPE GetPeriod() const {return 0;}
	virtual RATIOTYPE GetPeriodRatio() const {return 0;}
	//These are to return the period and corresponding ratioperiod - if they don't exist,
	//or not sure whether those exist, returning 0.
	//NOTE: Definition of period is not unambiguous. For example TET12
	//can be for example period = 1, periodratio = 2^(1/12), or
	//period = 12, periodratio = 2(and lots of other choices as well)

	virtual VRPAIR GetValidityRange() const {return VRPAIR(0,0);}
	//Tunings might not be valid for arbitrarily large range,
	//so this can be used to ask where it is valid. 

	virtual VRPAIR SetValidityRange(const VRPAIR&) {return GetValidityRange();}
	//To try to set validity range to given range, returns
	//the range that could be set.

	virtual void GeneralMessenger(const string&, ...) {}
	//Maybe somelike this might be needed for possible need to do something
	//specific with tunings(tweak parameters etc.)

	bool MayEdit(const CEDITMASK& em) const {return (em & m_EditMask).any();}

	bool SetEditMask(const CEDITMASK& em)
	{
		if(MayEdit(EM_EDITMASK))
			{m_EditMask = em; return false;}
		else
			return true;
	}

	CEDITMASK GetEditMask() const {return m_EditMask;}


	virtual ~CTuningBase() {};

//END TUNING INTERFACE

//BEGIN PROTECTED VIRTUALS
protected:
	virtual bool ProSetRatio(const STEPTYPE&, const RATIOTYPE&) {return true;}
	virtual bool ProCreateRatioPeriodic(const vector<RATIOTYPE>&, const RATIOTYPE&) {return true;}
	virtual bool ProCreateTET(const STEPTYPE&, const RATIOTYPE&) {return true;}
	//Above methods return false if action was done, true otherwise.
	virtual FINESTEPTYPE ProSetFineStepCount(const STEPTYPE&) {return GetFineStepCount();}
	virtual RATIOTYPE ProSetRatioFine(const FINESTEPTYPE&, const RATIOTYPE&) {return 0;}
	virtual STEPTYPE ProSetPeriod(const STEPTYPE&) {return 0;}
	virtual RATIOTYPE ProSetPeriodRatio(const RATIOTYPE&) {return 0;}

//END PROTECTED VIRTUALS



//PROTECTED INTERFACE
protected:
	CTuningBase& operator=(const CTuningBase&);
	CTuningBase(const CTuningBase&);
	//When copying tunings, the name must not be exact copy
	//since it is to be unique for every tuning, or maybe some
	//better identification could be introduced.

	CTUNINGTYPE GetType() const {return m_TuningType;}

	
//END PROTECTED INTERFACE


//BEGIN PRIVATE METHODS
private:
	SERIALIZATION_RETURN_TYPE NotenameMapToBinary(ostream&) const;
	SERIALIZATION_RETURN_TYPE NotenameMapFromBinary(istream&, const SERIALIZATION_VERSION);

	bool SetType(const CTUNINGTYPE& tt)
	{
		//Note: This doesn't check whether the tuning ratios
		//are consistent with given type.
		if(MayEdit(EM_TYPE))
		{
			m_TuningType = tt;

			if(m_TuningType == TT_GENERAL)
			{
				ProSetPeriod(0);
				ProSetPeriodRatio(0);
			}

			return false;
		}
		else
			return true;
	}
//END PRIVATE METHODS


//BEGIN: DATA MEMBERS
private:
	string m_TuningName;
	CEDITMASK m_EditMask; //Behavior: true <~> allow modification
	CTUNINGTYPE m_TuningType;
protected:
	NOTENAMEMAP m_NoteNameMap;
	//NOTE: Update operator = (and copy constructor) when
	//adding new members.

//END DATA MEMBERS

protected:
	CTuningBase(const string name = "Unnamed") :
		m_TuningName(name),
		m_TuningType(0) //Unspecific tuning by default.
		{
			m_EditMask.set(); //Allow all by default.
		}
	
protected:
	static enum
	{
		NO_SERIALIZATION_MARKERS = 1
	};
	
};


//Specialising tuning for ompt.
typedef CTuningBase<int16, uint16, float32, int16, uint16> CTuning;

const CTuning::SERIALIZATION_MARKER CTuning::s_SerializationBeginMarker("CT<sfs>B");
const CTuning::SERIALIZATION_MARKER CTuning::s_SerializationEndMarker("CT<sfs>E");
//<sfs> <-> Short Float Short

template<class A, class B, class C, class D, class E>
const CTuningBase<>::SERIALIZATION_MARKER CTuningBase<A, B, C, D, E>::s_SerializationBeginMarker("CTB<ABC>");

template<class A, class B, class C, class D, class E>
const CTuningBase<>::SERIALIZATION_MARKER CTuningBase<A, B, C, D, E>::s_SerializationEndMarker("CTB<ABC>");

template<class A, class B, class C, class D, class E>
const CTuningBase<>::SERIALIZATION_VERSION CTuningBase<A, B, C, D, E>::s_SerializationVersion(4);
/*
Version history:
	3->4: Changed sizetypes in serialisation from size_t(uint64) to
			smaller types (uint8, USTEPTYPE) (March 2007)
*/

template<class A, class B, class C, class D, class E>
const CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<A, B, C, D, E>::SERIALIZATION_SUCCESS = false;

template<class A, class B, class C, class D, class E>
const CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<A, B, C, D, E>::SERIALIZATION_FAILURE = true;

template<class A, class B, class C, class D, class E>
const string CTuningBase<A, B, C, D, E>::s_FileExtension = ".tun";

template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_MAINRATIOS = 0x1; //1b
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_NOTENAME = 0x2; //10b
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_TYPE = 0x4; //100b
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_NAME = 0x8; //1000b
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_FINETUNE = 0x10; //10000b
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_ALLOWALL = 0xFFFF; //All editing allowed.
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_EDITMASK = 0x8000; //Whether to allow modifications to editmask.
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_CONST = 0x8000;  //All editing except changing const status disable.
template<class A, class B, class C, class D, class E>
const CTuning::CEDITMASK CTuningBase<A, B, C, D, E>::EM_CONST_STRICT = 0; //All bits are zero.


template<class A, class B, class C, class D, class E>
const CTuning::CTUNINGTYPE CTuningBase<A, B, C, D, E>::TT_GENERAL = 0; //0...00b
template<class A, class B, class C, class D, class E>
const CTuning::CTUNINGTYPE CTuningBase<A, B, C, D, E>::TT_RATIOPERIODIC = 1; //0...10b
template<class A, class B, class C, class D, class E>
const CTuning::CTUNINGTYPE CTuningBase<A, B, C, D, E>::TT_TET = 3; //0...11b

template<class A, class B, class C, class D, class E>
CTuningBase<A,B,C,D,E>& CTuningBase<A,B,C,D,E>::operator =(const CTuningBase& pt)
//-----------------------------------------------------------------------
{
	if(!MayEdit(EM_ALLOWALL))
		return *this;

	m_TuningName = string("Copy of ") + pt.m_TuningName;
	m_NoteNameMap = pt.m_NoteNameMap;
	m_EditMask = pt.m_EditMask;
	m_EditMask |= EM_EDITMASK; //Not copying possible strict-const-status.

	m_TuningType = pt.m_TuningType;

	
	//TODO: Copying mode flags to be added(e.g. if tuning doesn't want the ratios
	//to be copied here)?

	//Copying ratios(Maybe inefficient, but general nevertheless).
	const VRPAIR rp = SetValidityRange(pt.GetValidityRange());
	//Returns best range the tuning can set. Ideally should be 
	//the same as validityrange of pt.

	//Copying ratios
	for(STEPTYPE i = rp.first; i<=rp.second; i++)
	{
		ProSetRatio(i, pt.GetFrequencyRatio(i));
	}
	ProSetPeriod(pt.GetPeriod());
	ProSetPeriodRatio(pt.GetPeriodRatio());

	//Copying finetune.
    ProSetFineStepCount(pt.GetFineStepCount());
	for(FINESTEPTYPE i = 0; i<GetFineStepCount(); i++)
		ProSetRatioFine(i, pt.GetFrequencyRatioFine(i));
	

	return *this;
}

template<class A, class B, class C, class D, class E>
CTuningBase<A,B,C,D,E>::CTuningBase(const CTuningBase& pt)
//-----------------------------------------------------------------------
{
	*this = pt;
}

template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::SetRatio(const STEPTYPE& s, const RATIOTYPE& r)
//-----------------------------------------------------------------
{
	if(MayEdit(EM_MAINRATIOS))
	{
		if(ProSetRatio(s, r))
			return true;
		else
			SetType(TT_GENERAL);

		return false;
	}
	return true;
	
}

template<class A, class B, class C, class D, class E>
string CTuningBase<A,B,C,D,E>::GetTuningTypeStr(const CTUNINGTYPE& tt)
//----------------------------------------------------------------
{
	if(tt == TT_GENERAL)
		return "General";
	if(tt == TT_RATIOPERIODIC)
		return "Ratio periodic";
	if(tt == TT_TET)
		return "TET";
	return "Unknown";
}



template<class A, class B, class C, class D, class E>
CTuningBase<>::NOTESTR CTuningBase<A,B,C,D,E>::GetNoteName(const STEPTYPE& x) const
//-----------------------------------------------------------------------
{
	NNM_CITER i = m_NoteNameMap.find(x);
	if(i != m_NoteNameMap.end())
		return i->second;
	else
		return Stringify(x);
}


template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::DoesTypeInclude(const CTUNINGTYPE& type) const
//-----------------------------------------------------------------------------
{
	if(type == TT_GENERAL)
		return true;
	//Using interpretation that every tuning is also
	//a general tuning.

	//Note: Here newType != TT_GENERAL
	if(m_TuningType == TT_GENERAL)
		return false;
	//Every non-general tuning should not include all tunings.

	if( (m_TuningType & type) == type)
		return true;
	else
		return false;
}

template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::SetNoteName(const STEPTYPE& n, const string& str)
//-----------------------------------------------------------------------
{
	if(MayEdit(EM_NOTENAME))
	{
		m_NoteNameMap[n] = str;
		return false;
	}
	return true;
}

template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::ClearNoteName(const STEPTYPE& n, const bool eraseAll)
//-------------------------------------------------------
{
	if(MayEdit(EM_NOTENAME))
	{
		if(eraseAll)
		{
			m_NoteNameMap.clear();
			return false;
		}

		NNM_ITER iter = m_NoteNameMap.find(n);
		if(iter != m_NoteNameMap.end())
		{
			m_NoteNameMap.erase(iter);
			return false;
		}
		else
			return true;
	}
	return true;
}


template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::Multiply(const RATIOTYPE& r)
//---------------------------------------------------
{
	if(r <= 0 || !MayEdit(EM_MAINRATIOS))
		return true;

	//Note: Multiplying ratios by constant doesn't 
	//change, e.g. TETness or ratioperiodic'ness status.
	VRPAIR vrp = GetValidityRange();
	for(STEPTYPE i = vrp.first; i<vrp.second; i++)
	{
		if(ProSetRatio(i, r*GetFrequencyRatio(i)))
			return true;
	}
	return false;
}

template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::CreateRatioPeriodic(const STEPTYPE& s, const RATIOTYPE& r)
//-------------------------------------------------------------
{
	if(s < 1 || r <= 0)
		return true;

	vector<RATIOTYPE> v;
	v.reserve(s);
	for(STEPTYPE i = 0; i<s; i++)
		v.push_back(GetFrequencyRatio(i));
	return CreateRatioPeriodic(v, r);
}

template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::CreateRatioPeriodic(const vector<RATIOTYPE>& v, const RATIOTYPE& r)
//------------------------------------------------------------------------------------------
{
	if(MayEdit(EM_MAINRATIOS) &&
		(MayEdit(EM_TYPE) || GetType() == TT_RATIOPERIODIC))
	{
		if(ProCreateRatioPeriodic(v,r))
			return true;
		else
		{
			SetType(TT_RATIOPERIODIC);
			if(MayEdit(EM_FINETUNE))
				ProSetFineStepCount(GetFineStepCount());
			return false;
		}
	}
	else 
		return true;
}


template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::CreateTET(const STEPTYPE& s, const RATIOTYPE& r)
//-------------------------------------------------------------------
{
	if(MayEdit(EM_MAINRATIOS) &&
	  (MayEdit(EM_TYPE) || GetType() == TT_TET))
	{
		if(ProCreateTET(s,r))
			return true;
		else
		{
			SetType(TT_TET);
			if(MayEdit(EM_FINETUNE))
				ProSetFineStepCount(GetFineStepCount());
			return false;
		}
	}
	else
		return true;
}


template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::ChangePeriod(const STEPTYPE& s)
//---------------------------------------------------
{
	if(!MayEdit(EM_MAINRATIOS) || s < 1)
		return true;

	if(m_TuningType == TT_RATIOPERIODIC)
		return CreateRatioPeriodic(s, GetPeriodRatio());

	if(m_TuningType == TT_TET)
		return CreateTET(s, GetPeriodRatio());

	return true;
}	


template<class A, class B, class C, class D, class E>
bool CTuningBase<A,B,C,D,E>::ChangePeriodRatio(const RATIOTYPE& r)
//---------------------------------------------------
{
	if(!MayEdit(EM_MAINRATIOS) || r <= 0)
		return true;

	if(m_TuningType == TT_RATIOPERIODIC)
		return CreateRatioPeriodic(GetPeriod(), r);

	if(m_TuningType == TT_TET)
		return CreateTET(GetPeriod(), r);

	return true;
}	


template<class A, class B, class C, class D, class E>
CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<A, B, C, D, E>::SerializeBinary(ostream& outStrm, const int mode) const
//------------------------------------------------------------------------------------------------------------------------------
{
	//Writing the tuning name here.
	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		outStrm.write(reinterpret_cast<const char*>(&s_SerializationBeginMarker), sizeof(s_SerializationBeginMarker));
	}
	outStrm.write(reinterpret_cast<const char*>(&s_SerializationVersion), sizeof(s_SerializationVersion));

	//Tuning name
	if(StringToBinaryStream<uint8>(outStrm, m_TuningName)) return SERIALIZATION_FAILURE;

	//Const mask
	const __int16 cm = static_cast<__int16>(m_EditMask.to_ulong());
	outStrm.write(reinterpret_cast<const char*>(&cm), sizeof(cm));

	//Tuning type
	const __int16 tt = static_cast<__int16>(m_TuningType.to_ulong());
	outStrm.write(reinterpret_cast<const char*>(&tt), sizeof(tt));
	
	//Notename map.
	if(NotenameMapToBinary(outStrm))
		return SERIALIZATION_FAILURE;

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		outStrm.write(reinterpret_cast<const char*>(&s_SerializationEndMarker), sizeof(s_SerializationEndMarker));
	}

	if(outStrm.good())
		return SERIALIZATION_SUCCESS;
	else 
		return SERIALIZATION_FAILURE;
}


template<class A, class B, class C, class D, class E>
CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<A,B,C,D,E>::UnSerializeBinary(istream& inStrm, const int mode) 
//----------------------------------------------------------------------------------------------------------------------------
{
	if(!inStrm.good() || !MayEdit(EM_ALLOWALL))
		return SERIALIZATION_FAILURE;

	SERIALIZATION_MARKER begin;
	SERIALIZATION_MARKER end;
	SERIALIZATION_VERSION version;

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		inStrm.read(reinterpret_cast<char*>(&begin), sizeof(begin));
		if(begin != s_SerializationBeginMarker) return SERIALIZATION_FAILURE;
	}

	inStrm.read(reinterpret_cast<char*>(&version), sizeof(version));
	if(version > s_SerializationVersion) 
		return SERIALIZATION_FAILURE;

	//Tuning name
	if(version < 4)
	{
		if(StringFromBinaryStream<uint64>(inStrm, m_TuningName))
			return SERIALIZATION_FAILURE;	
	}
	else
	{
		if(StringFromBinaryStream<uint8>(inStrm, m_TuningName))
			return SERIALIZATION_FAILURE;
	}

	//Const mask
	__int16 em = 0;
	inStrm.read(reinterpret_cast<char*>(&em), sizeof(em));
	m_EditMask = em;

	//Tuning type
	__int16 tt = 0;
	inStrm.read(reinterpret_cast<char*>(&tt), sizeof(tt));
	m_TuningType = tt;

	//Notemap
	if(NotenameMapFromBinary(inStrm, version))
		return SERIALIZATION_FAILURE;

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		inStrm.read(reinterpret_cast<char*>(&end), sizeof(end));
		if(end != s_SerializationEndMarker) return SERIALIZATION_FAILURE;
	}
	
	if(inStrm.good()) return SERIALIZATION_SUCCESS;
	else return SERIALIZATION_FAILURE;
}


template<class A, class B, class C, class D, class E>
CTuningBase<>::SERIALIZATION_RETURN_TYPE
CTuningBase<A,B,C,D,E>::NotenameMapToBinary(ostream& outStrm) const
//-----------------------------------------------------------------------------------------------
{
	if(!outStrm.good())
		return SERIALIZATION_FAILURE;
	if(m_NoteNameMap.size() > (std::numeric_limits<SIZETYPE>::max)())
		return SERIALIZATION_FAILURE;

	//Size.
	const SIZETYPE size = static_cast<SIZETYPE>(m_NoteNameMap.size());
	outStrm.write(reinterpret_cast<const char*>(&size), sizeof(size));

	for(NNM_CITER i = m_NoteNameMap.begin(); i != m_NoteNameMap.end(); i++)
	{
		const STEPTYPE n = i->first;
		outStrm.write(reinterpret_cast<const char*>(&n), sizeof(n));
		if(StringToBinaryStream<uint8>(outStrm, i->second))
			return SERIALIZATION_FAILURE;
	}

	if(outStrm.good())
		return SERIALIZATION_SUCCESS;
	else
		return SERIALIZATION_FAILURE;
}

template<class A, class B, class C, class D, class E>
CTuningBase<>::SERIALIZATION_RETURN_TYPE
CTuningBase<A,B,C,D,E>::NotenameMapFromBinary(istream& inStrm, const SERIALIZATION_VERSION version)
//--------------------------------------------------
{
	if(!inStrm.good())
		return SERIALIZATION_FAILURE;

	//Size
	SIZETYPE size;
	if(version < 4)
	{
		uint64 s = 0;
        inStrm.read(reinterpret_cast<char*>(&s), sizeof(s));
		size = static_cast<SIZETYPE>(s);
	}
	else 
		inStrm.read(reinterpret_cast<char*>(&size), sizeof(size));
	


	for(size_t i = 0; i<size; i++)
	{
		STEPTYPE n;
		string str;
		inStrm.read(reinterpret_cast<char*>(&n), sizeof(n));
		if(version < 4)
		{
			if(StringFromBinaryStream<uint64>(inStrm, str))
				return SERIALIZATION_FAILURE;	
		}
		else
		{
			if(StringFromBinaryStream<uint8>(inStrm, str))
				return SERIALIZATION_FAILURE;	
		}
		m_NoteNameMap[n] = str;
	}

	if(inStrm.good())
		return SERIALIZATION_SUCCESS;
	else
		return SERIALIZATION_FAILURE;
}

#endif
