/*
 * tuningCollection.h
 * ------------------
 * Purpose: Alternative sample tuning collection class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "tuning.h"
#include <vector>
#include <string>


OPENMPT_NAMESPACE_BEGIN


class CTuningCollection;

namespace CTuningS11n
{
	void ReadTuning(std::istream& iStrm, CTuningCollection& Tc, const size_t);
	void WriteTuning(std::ostream& oStrm, const CTuning& t);
}


//=====================
class CTuningCollection //To contain tuning objects.
//=====================
{
	friend class CTuningstreamer;
public:

//BEGIN TYPEDEFS
	typedef uint16 EDITMASK;
	//If changing this, see whether serialization should be 
	//modified as well.

	typedef std::vector<CTuning*> TUNINGVECTOR;
	typedef TUNINGVECTOR::iterator TITER; //Tuning ITERator.
	typedef TUNINGVECTOR::const_iterator CTITER;

	typedef uint16 SERIALIZATION_VERSION;

	typedef bool SERIALIZATION_RETURN_TYPE;

//END TYPEDEFS

//BEGIN PUBLIC STATIC CONSTS
public:
	enum 
	{
		EM_ADD = 1, //true <~> allowed
		EM_REMOVE = 2,
		EM_ALLOWALL = 0xffff,
		EM_CONST = 0,

	    s_SerializationVersion = 3,

		SERIALIZATION_SUCCESS = false,
		SERIALIZATION_FAILURE = true
	};

	static const char s_FileExtension[4];
	static const size_t s_nMaxTuningCount = 255;

//END PUBLIC STATIC CONSTS

//BEGIN INTERFACE:
public:
	CTuningCollection(const std::string& name = "");
	~CTuningCollection();
	
	//Note: Given pointer is deleted by CTuningCollection
	//at some point.
	bool AddTuning(CTuning* const pT);
	bool AddTuning(std::istream& inStrm) {return AddTuning(inStrm, false);}
	
	bool Remove(const size_t i);
	bool Remove(const CTuning*);

	bool CanEdit(const EDITMASK& em) const {return (m_EditMask & em) != 0;}

	void SetConstStatus(const EDITMASK& em) {m_EditMask = em;}

	const EDITMASK& GetEditMask() const {return m_EditMask;}

	std::string GetEditMaskString() const;

	CTuning& GetTuning(size_t i) {return *m_Tunings.at(i);}
	const CTuning& GetTuning(size_t i) const {return *m_Tunings.at(i);}
	CTuning* GetTuning(const std::string& name);
	const CTuning* GetTuning(const std::string& name) const;

	size_t GetNumTunings() const {return m_Tunings.size();}

	std::string GetName() const {return m_Name;}

#ifndef MODPLUG_NO_FILESAVE
	void SetSavefilePath(const mpt::PathString &psz) {m_SavefilePath = psz;}
	mpt::PathString GetSaveFilePath() const {return m_SavefilePath;}
#endif // MODPLUG_NO_FILESAVE

	std::string GetVersionString() const {return mpt::ToString(static_cast<int>(s_SerializationVersion));}

	size_t GetNameLengthMax() const {return 256;}

	//Serialization/unserialisation
	bool Serialize(std::ostream&) const;
	bool Deserialize(std::istream&);
#ifndef MODPLUG_NO_FILESAVE
	bool Serialize() const;
	bool Deserialize();
#endif // MODPLUG_NO_FILESAVE

	//Transfer tuning pT from pTCsrc to pTCdest
	static bool TransferTuning(CTuningCollection* pTCsrc, CTuningCollection* pTCdest, CTuning* pT);
	

//END INTERFACE
	

//BEGIN: DATA MEMBERS
private:
	//BEGIN: SERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_Tunings; //The actual tuningobjects are stored as deletable pointers here.
	std::string m_Name;
	EDITMASK m_EditMask;
	//END: SERIALIZABLE DATA MEMBERS

	//BEGIN: NONSERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_DeletedTunings; //See Remove()-method for explanation of this.
#ifndef MODPLUG_NO_FILESAVE
	mpt::PathString m_SavefilePath;
#endif // MODPLUG_NO_FILESAVE
	//END: NONSERIALIZABLE DATA MEMBERS
	
//END: DATA MEMBERS

	friend void CTuningS11n::ReadTuning(std::istream& iStrm, CTuningCollection& Tc, const size_t);

//BEGIN PRIVATE METHODS
private:
	CTuning* FindTuning(const std::string& name) const;
	size_t FindTuning(const CTuning* const) const;

	bool AddTuning(std::istream& inStrm, const bool ignoreEditmask);

	bool Remove(TITER removable, bool moveToTrashBin = true);

	//Hiding default operators because default meaning might not work right.
	CTuningCollection& operator=(const CTuningCollection&) {return *this;}
	CTuningCollection(const CTuningCollection&) {}

	bool DeserializeOLD(std::istream&, bool& loadingSuccessful);

//END PRIVATE METHODS.
};


OPENMPT_NAMESPACE_END
