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

//This is just class for the data which is written in serializations as a ID.
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
		for(i = 0; i < min<int>(SIZE, static_cast<int>(a.size())); i++)
		{
			m_ID[i] = a.at(i);
		}
		for(i; i<SIZE; i++) m_ID[i] = '.';


	}
};

//Class defining tuning which is fundamentally based on discrete steps.
template<class TSTEPTYPE = short int, class TRATIOTYPE = float, class TFINETUNESTEP = TSTEPTYPE>
class CTuningBase
{
	//STEPTYPE: Some type that has properties that of 'ordinary' signed and discrete figures.
	//RATIOTYPE: Some 'real figure' type which is able to present ratios.

public:
//BEING TYPEDEFS:
	typedef TSTEPTYPE STEPTYPE;
	typedef TRATIOTYPE RATIOTYPE;
	typedef TFINETUNESTEP FINETUNESTEP;

	typedef std::exception TUNINGEXCEPTION;

	typedef CSerializationID<8> SERIALIZATION_MARKER;
	typedef __int16 SERIALIZATION_VERSION;
	typedef bool SERIALIZATION_RETURN_TYPE;

	typedef std::pair<STEPTYPE, STEPTYPE> VRPAIR;

	typedef bitset<16> CEDITMASK;
	//If making bitset larger, see whether serialization
	//works correctly after that.

	typedef bitset<16> CTUNINGTYPE;
	//If making bitset larger, see whether serialization
	//works correctly after that.

	typedef std::string NOTESTR;
	typedef std::map<STEPTYPE, NOTESTR> NOTENAMEMAP;
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

	static const CEDITMASK EM_RATIOS;
	static const CEDITMASK EM_NOTENAME;
	static const CEDITMASK EM_TYPE;
	static const CEDITMASK EM_NAME;
	static const CEDITMASK EM_ALLOWALL;
	static const CEDITMASK EM_CONST;
	//Add declarations of editmasks here.
	//NOTE: When adding, check definition of CEDITMASK - 
	//might need to be modified.

	static const CTUNINGTYPE TT_GENERAL;
	static const CTUNINGTYPE TT_RATIOPERIODIC;
	static const CTUNINGTYPE TT_TET;
	//Add declarations of tuning typee specifiers here.
	//NOTE: When adding, check definition of CTUNINGTYPE - 
	//might need to be modified.

//END PUBLIC STATICS

	
public:
//BEGIN TUNING INTERFACE
	virtual RATIOTYPE GetFrequencyRatio(const STEPTYPE&, const STEPTYPE&) const {return 0;};
	//Neglecting possible use of fineSteps, returned value is to be such that
	//returnvalue * ReferenceFrequency == Frequency_of_note_stepsFromCenter_away_from_reference.
	//E.g. in 12-TET it is 2^(stepFromCenter/12)

	virtual NOTESTR GetNoteName(const STEPTYPE& x) const;

	virtual bool CreateRatioPeriodic(const vector<RATIOTYPE>&, const RATIOTYPE&) {return true;}
	//Create ratioperiodic tuning of *this.

	virtual bool CreateTET(const STEPTYPE&, const RATIOTYPE&) {return true;}
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

	void SetName(const string& s)
	{
		if(MayEdit(EM_NAME))
			m_TuningName = s;
	}

	string GetName() const {return m_TuningName;}

	bool SetNoteName(const STEPTYPE&, const string&);

	bool ClearNoteName(const STEPTYPE& n, const bool clearAll = false);
	
	virtual bool SetRatio(const STEPTYPE&, const RATIOTYPE&) {return true;}
	
	CTUNINGTYPE GetTuningType() const {return m_TuningType;}

	static string GetTuningTypeStr(const CTUNINGTYPE& tt);

	bool DoesTypeInclude(const CTUNINGTYPE& type) const;

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

	virtual void GeneralMessenger(const string&, ...) {}
	//Maybe somelike this might be needed for possible need to do something
	//specific with tunings(tweak parameters etc.)

	virtual ~CTuningBase() {};

//END TUNING INTERFACE



//PROTECTED INTERFACE
protected:
	CTuningBase& operator=(const CTuningBase&);
	CTuningBase(const CTuningBase&);
	//When copying tunings, the name must not be exact copy
	//since it is to be unique for every tuning.

	bool MayEdit(const CEDITMASK& em) {return (em & m_ConstMask).any();}

	CTUNINGTYPE GetType() const {return m_TuningType;}

	bool SetType(const CTUNINGTYPE& tt)
	{
		//Note: This doesn't check whether the tuning ratios
		//are consistent with given type.
		if(MayEdit(EM_TYPE))
		{
			m_TuningType = tt;
			return false;
		}
		else
			return true;
	}

//END PROTECTED INTERFACE


//BEGIN PRIVATE METHODS
private:
	SERIALIZATION_RETURN_TYPE NotenameMapToBinary(ostream&) const;
	SERIALIZATION_RETURN_TYPE NotenameMapFromBinary(istream&);
//END PRIVATE METHODS


//BEGIN: DATA MEMBERS
private:
	string m_TuningName;
	CEDITMASK m_ConstMask; //Behavior: true <~> allow modification
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
			m_ConstMask.set(); //Allow all by default.
		}
	
protected:
	static enum
	{
		NO_SERIALIZATION_MARKERS = 1
	};
	
};


//Specialising tuning for ompt.
typedef short int MPT_TUNING_STEPTYPE;
typedef float MPT_TUNING_RATIOTYPE;
typedef MPT_TUNING_STEPTYPE MPT_TUNING_FINESTEPTYPE;

typedef CTuningBase<MPT_TUNING_STEPTYPE, MPT_TUNING_RATIOTYPE, MPT_TUNING_FINESTEPTYPE> CTuning;

const CTuning::SERIALIZATION_MARKER CTuning::s_SerializationBeginMarker("CT<sfs>B");
const CTuning::SERIALIZATION_MARKER CTuning::s_SerializationEndMarker("CT<sfs>E");
//<sfs> <-> Short Float Short

template<class A, class B, class C>
const CTuningBase<>::SERIALIZATION_MARKER CTuningBase<A, B, C>::s_SerializationBeginMarker("CTB<ABC>");

template<class A, class B, class C>
const CTuningBase<>::SERIALIZATION_MARKER CTuningBase<A, B, C>::s_SerializationEndMarker("CTB<ABC>");

template<class A, class B, class C>
const CTuningBase<>::SERIALIZATION_VERSION CTuningBase<A, B, C>::s_SerializationVersion(3);

template<class A, class B, class C>
const CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<A, B, C>::SERIALIZATION_SUCCESS = false;

template<class A, class B, class C>
const CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<A, B, C>::SERIALIZATION_FAILURE = true;

template<class A, class B, class C>
const string CTuningBase<A, B, C>::s_FileExtension = ".mptt";

template<class A, class B, class C>
const CTuning::CEDITMASK CTuningBase<A, B, C>::EM_RATIOS = 1; //1b
template<class A, class B, class C>
const CTuning::CEDITMASK CTuningBase<A, B, C>::EM_NOTENAME = 2; //10b
template<class A, class B, class C>
const CTuning::CEDITMASK CTuningBase<A, B, C>::EM_TYPE = 4; //100b
template<class A, class B, class C>
const CTuning::CEDITMASK CTuningBase<A, B, C>::EM_NAME = 8; //1000b
template<class A, class B, class C>
const CTuning::CEDITMASK CTuningBase<A, B, C>::EM_ALLOWALL = 0xFFFF; //
template<class A, class B, class C>
const CTuning::CEDITMASK CTuningBase<A, B, C>::EM_CONST = 0; //All bits are zero.


template<class A, class B, class C>
const CTuning::CTUNINGTYPE CTuningBase<A, B, C>::TT_GENERAL = 0; //0...00b
template<class A, class B, class C>
const CTuning::CTUNINGTYPE CTuningBase<A, B, C>::TT_RATIOPERIODIC = 1; //0...10b
template<class A, class B, class C>
const CTuning::CTUNINGTYPE CTuningBase<A, B, C>::TT_TET = 3; //0...11b

template<class A, class B, class C>
CTuningBase<A,B,C>& CTuningBase<A,B,C>::operator =(const CTuningBase& pt)
//-----------------------------------------------------------------------
{
	if(!MayEdit(EM_ALLOWALL))
		return *this;

	m_TuningName = string("Copy of ") + pt.m_TuningName;
	m_NoteNameMap = pt.m_NoteNameMap;
	m_ConstMask.set(); //NOTE: Not copying const status.
	m_TuningType = pt.m_TuningType;
	return *this;
}

template<class A, class B, class C>
CTuningBase<A,B,C>::CTuningBase(const CTuningBase& pt)
//-----------------------------------------------------------------------
{
	*this = pt;
}

template<class A, class B, class C>
string CTuningBase<A,B,C>::GetTuningTypeStr(const CTUNINGTYPE& tt)
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



template<class A, class B, class C>
CTuningBase<>::NOTESTR CTuningBase<A,B,C>::GetNoteName(const STEPTYPE& x) const
//-----------------------------------------------------------------------
{
	NNM_CITER i = m_NoteNameMap.find(x);
	if(i != m_NoteNameMap.end())
		return i->second;
	else
		return Stringify(x);
}


template<class A, class B, class C>
bool CTuningBase<A,B,C>::DoesTypeInclude(const CTUNINGTYPE& type) const
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

template<class A, class B, class C>
bool CTuningBase<A,B,C>::SetNoteName(const STEPTYPE& n, const string& str)
//-----------------------------------------------------------------------
{
	if(MayEdit(EM_NOTENAME))
	{
		m_NoteNameMap[n] = str;
		return false;
	}
	return true;
}

template<class A, class B, class C>
bool CTuningBase<A,B,C>::ClearNoteName(const STEPTYPE& n, const bool eraseAll)
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


template<class TSTEPTYPE, class TRATIOTYPE, class TFINETUNESTEP>
CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<TSTEPTYPE, TRATIOTYPE, TFINETUNESTEP>::SerializeBinary(ostream& outStrm, const int mode) const
//------------------------------------------------------------------------------------------------------------------------------
{
	//Writing the tuning name here.
	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		outStrm.write(reinterpret_cast<const char*>(&s_SerializationBeginMarker), sizeof(s_SerializationBeginMarker));
	}
	outStrm.write(reinterpret_cast<const char*>(&s_SerializationVersion), sizeof(s_SerializationVersion));

	//Tuning name
	if(StringToBinaryStream(outStrm, m_TuningName)) return SERIALIZATION_FAILURE;

	//Const mask
	const __int16 cm = static_cast<__int16>(m_ConstMask.to_ulong());
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


template<class TSTEPTYPE, class TRATIOTYPE, class TFINETUNESTEP>
CTuningBase<>::SERIALIZATION_RETURN_TYPE CTuningBase<TSTEPTYPE, TRATIOTYPE, TFINETUNESTEP>::UnSerializeBinary(istream& inStrm, const int mode) 
//----------------------------------------------------------------------------------------------------------------------------
{
	if(inStrm.fail())
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
	if(version != s_SerializationVersion) 
		return SERIALIZATION_FAILURE;

	//Tuning name
	if(StringFromBinaryStream(inStrm, m_TuningName))
		return SERIALIZATION_FAILURE;

	//Const mask
	__int16 cm = 0;
	inStrm.read(reinterpret_cast<char*>(&cm), sizeof(cm));
	m_ConstMask = cm;

	//Tuning type
	__int16 tt = 0;
	inStrm.read(reinterpret_cast<char*>(&tt), sizeof(tt));
	m_TuningType = tt;

	//Notemap
	if(NotenameMapFromBinary(inStrm))
		return SERIALIZATION_FAILURE;

	if(!(mode & NO_SERIALIZATION_MARKERS))
	{
		inStrm.read(reinterpret_cast<char*>(&end), sizeof(end));
		if(end != s_SerializationEndMarker) return SERIALIZATION_FAILURE;
	}
	
	if(inStrm.good()) return SERIALIZATION_SUCCESS;
	else return SERIALIZATION_FAILURE;
}


template<class A, class B, class C>
CTuningBase<>::SERIALIZATION_RETURN_TYPE
CTuningBase<A,B,C>::NotenameMapToBinary(ostream& outStrm) const
//-----------------------------------------------------------------------------------------------
{
	if(outStrm.fail())
		return SERIALIZATION_FAILURE;

	//Size.
	const size_t size = m_NoteNameMap.size();
	outStrm.write(reinterpret_cast<const char*>(&size), sizeof(size));

	for(NNM_CITER i = m_NoteNameMap.begin(); i != m_NoteNameMap.end(); i++)
	{
		const STEPTYPE n = i->first;
		outStrm.write(reinterpret_cast<const char*>(&n), sizeof(n));
		if(StringToBinaryStream(outStrm, i->second))
			return SERIALIZATION_FAILURE;
	}

	if(outStrm.good())
		return SERIALIZATION_SUCCESS;
	else
		return SERIALIZATION_FAILURE;
}

template<class A, class B, class C>
CTuningBase<>::SERIALIZATION_RETURN_TYPE
CTuningBase<A,B,C>::NotenameMapFromBinary(istream& inStrm)
//--------------------------------------------------
{
	if(inStrm.fail())
		return SERIALIZATION_FAILURE;

	//Size
	size_t size;
	inStrm.read(reinterpret_cast<char*>(&size), sizeof(size));

	//Todo: Handle this better.
	if(size > 1000)
		return SERIALIZATION_FAILURE;


	for(size_t i = 0; i<size; i++)
	{
		STEPTYPE n;
		string str;
		inStrm.read(reinterpret_cast<char*>(&n), sizeof(n));
		if(StringFromBinaryStream(inStrm, str))
			return SERIALIZATION_FAILURE;
		m_NoteNameMap[n] = str;
	}

	if(inStrm.good())
		return SERIALIZATION_SUCCESS;
	else
		return SERIALIZATION_FAILURE;
}

#endif
