#ifndef TUNINGCOLLECTION_H
#define TUNINGCOLLECTION_H

#include "tuning.h"
#include <vector>
#include <fstream>
#include <string>
#include <bitset>

using namespace std;


//=====================
class CTuningCollection
//Purpose: To contain tuning modes.
//=====================
{
public:

//BEGIN TYPEDEFS
	typedef std::bitset<16> CEDITMASK;
	//If changing this, see whether serialization should be 
	//modified as well.

	typedef vector<CTuning* const> TUNINGVECTOR;
	typedef TUNINGVECTOR::iterator TITER; //Tuning ITERator.
	typedef TUNINGVECTOR::const_iterator CTITER;

	typedef __int32 SERIALIZATION_MARKER;

	typedef bool SERIALIZATION_RETURN_TYPE;

//END TYPEDEFS

//BEGIN PUBLIC STATIC CONSTS
public:
	static const CEDITMASK EM_ADD; //true <~> allowed
	static const CEDITMASK EM_REMOVE;
	static const CEDITMASK EM_ALLOWALL;
	static const CEDITMASK EM_CONST;

	static const SERIALIZATION_MARKER s_SerializationBeginMarker;
	static const SERIALIZATION_MARKER s_SerializationEndMarker;
	static const SERIALIZATION_MARKER s_SerializationVersion;
	//Begin- and endmarkers are used to make it more certain that the stream given to
	//Unserialization method really is in position which contain serialization of object
	//of this class.

	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_SUCCESS;
	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_FAILURE;

	static const string s_FileExtension;

//END PUBLIC STATIC CONSTS

//BEGIN INTERFACE:
public:
	CTuningCollection(const string& name = "");
	~CTuningCollection();
	
	bool AddTuning(istream& inStrm);
	bool AddTuning(CTuning* const pT);

	bool Remove(const size_t i);

	bool CanEdit(const CEDITMASK& em) const {return (m_EditMask & em).any();}

	void SetConstStatus(const CEDITMASK& em) {m_EditMask = em;}

	const CEDITMASK& GetEditMask() const {return m_EditMask;}
	
	const CTuning& GetTuning(unsigned short int i) const {return *m_Tunings.at(i);}

	CTuning& GetTuning(unsigned short int i) {return *m_Tunings.at(i);}
	CTuning* GetTuning(const string& name);

	size_t GetNumTunings() const {return m_Tunings.size();}

	const string& GetName() const {return m_Name;}

	void SetSavefilePath(const string& str) {m_SavefilePath = str + s_FileExtension;}

	//Serialization/unserialisation
	bool SerializeBinary(ostream&) const;
	bool SerializeBinary() const;
	bool UnSerializeBinary(istream&);
	bool UnSerializeBinary();

//END INTERFACE
	

//BEGIN: DATA MEMBERS
private:
	//BEGIN: SERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_Tunings; //The actual tunings are collected as deletable pointers in vector m_Tunings.
	string m_Name;
	CEDITMASK m_EditMask;
	//END: SERIALIZABLE DATA MEMBERS

	//BEGIN: NOTSERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_DeletedTunings; //See Remove()-method for explanation of this.
	string m_SavefilePath;
	//END: NOTSERIALIZABLE DATA MEMBERS
	
//END: DATA MEMBERS

//BEGIN PRIVATE METHODS
private:
	CTuning* FindTuning(const string& name) const;

	//Hiding default operators because default meaning doesn't work right.
	CTuningCollection& operator=(const CTuningCollection&) {}
	CTuningCollection(const CTuningCollection&) {}

	SERIALIZATION_RETURN_TYPE WriteTuningVector(ostream&) const;
	SERIALIZATION_RETURN_TYPE ReadTuningVector(istream&);
//END PRIVATE METHODS.
};


#endif
