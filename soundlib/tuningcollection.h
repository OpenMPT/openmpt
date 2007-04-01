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
	//Note: By default, given pointer is deleted by CTuningCollection
	//at some point.
	//TODO: Make possible to add tuning which won't be deleted.

	bool Remove(const size_t i);
	bool Remove(const CTuning*);

	bool CanEdit(const CEDITMASK& em) const {return (m_EditMask & em).any();}

	void SetConstStatus(const CEDITMASK& em) {m_EditMask = em;}

	const CEDITMASK& GetEditMask() const {return m_EditMask;}

	string GetEditMaskString() const {return m_EditMask.to_string<char, 
		char_traits<char>, allocator<char> >();}

	const CTuning& GetTuning(size_t i) const {return *m_Tunings.at(i);}

	CTuning& GetTuning(size_t i) {return *m_Tunings.at(i);}
	CTuning* GetTuning(const string& name);
	const CTuning* GetTuning(const string& name) const;

	size_t GetNumTunings() const {return m_Tunings.size();}

	const string& GetName() const {return m_Name;}

	void SetSavefilePath(const string& str) {m_SavefilePath = str;}
	const string GetSaveFilePath() const {return m_SavefilePath;}

	string GetVersionString() const {return Stringify(s_SerializationVersion);}

	//Stringsize-data is saved as byte so maxlength is 256.
	size_t GetNameLengthMax() const {return 256;}

	//Serialization/unserialisation
	bool SerializeBinary(ostream&) const;
	bool SerializeBinary() const;
	bool UnSerializeBinary(istream&);
	bool UnSerializeBinary();

	static bool TransferTuning(CTuningCollection* pTCsrc, CTuningCollection* pTCdest, CTuning* pT);
	//Transfer tuning pT from pTCsrc to pTCdest

//END INTERFACE
	

//BEGIN: DATA MEMBERS
private:
	//BEGIN: SERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_Tunings; //The actual tunings are collected as deletable pointers in vector m_Tunings.
	string m_Name;
	CEDITMASK m_EditMask;
	//TODO: TO_BE_ADDED: TUNINGCOLLECTIONVECTOR m_TuningCollections;
	//A tuningcollection to be able to contain tuningcollections(compare
	//treeview)
	//END: SERIALIZABLE DATA MEMBERS

	//BEGIN: NONSERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_DeletedTunings; //See Remove()-method for explanation of this.
	string m_SavefilePath;
	//END: NOTSERIALIZABLE DATA MEMBERS
	
//END: DATA MEMBERS

//BEGIN PRIVATE METHODS
private:
	CTuning* FindTuning(const string& name) const;
	size_t FindTuning(const CTuning* const) const;

	bool Remove(TITER removable, bool moveToTrashBin = true);

	//Hiding default operators because default meaning might not work right.
	CTuningCollection& operator=(const CTuningCollection&) {}
	CTuningCollection(const CTuningCollection&) {}

	SERIALIZATION_RETURN_TYPE WriteTuningVector(ostream&) const;
	SERIALIZATION_RETURN_TYPE ReadTuningVector(istream&);
//END PRIVATE METHODS.
};


#endif
