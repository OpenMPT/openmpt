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

	//If changing this, see whether serialization should be 
	//modified as well.

	typedef std::vector<CTuning*> TUNINGVECTOR;
	typedef TUNINGVECTOR::iterator TITER; //Tuning ITERator.
	typedef TUNINGVECTOR::const_iterator CTITER;

	typedef bool SERIALIZATION_RETURN_TYPE;

//END TYPEDEFS

//BEGIN PUBLIC STATIC CONSTS
public:
	enum 
	{
		SERIALIZATION_SUCCESS = false,
		SERIALIZATION_FAILURE = true
	};

	static const char s_FileExtension[4];

	// OpenMPT <= 1.26 had to following limits:
	//  *  255 built-in tunings (only 2 were ever actually provided)
	//  *  255 local tunings
	//  *  255 tune-specific tunings
	// As 1.27 copies all used tunings into the module, the limit of 255 is no
	// longer sufficient. In the worst case scenario, the module contains 255
	// unused tunings and uses 255 local ones. In addition to that, allow the
	// user to additionally import both built-in tunings.
	// Older OpenMPT versions will silently skip loading tunings beyond index
	// 255.
	static const size_t s_nMaxTuningCount = 255 + 255 + 2;

//END PUBLIC STATIC CONSTS

//BEGIN INTERFACE:
public:
	CTuningCollection(const std::string& name = "");
	~CTuningCollection();
	
	//Note: Given pointer is deleted by CTuningCollection
	//at some point.
	bool AddTuning(CTuning* const pT);
	bool AddTuning(std::istream& inStrm);
	
	bool Remove(const size_t i);
	bool Remove(const CTuning*);

	CTuning& GetTuning(size_t i) {return *m_Tunings.at(i);}
	const CTuning& GetTuning(size_t i) const {return *m_Tunings.at(i);}
	CTuning* GetTuning(const std::string& name);
	const CTuning* GetTuning(const std::string& name) const;

	size_t GetNumTunings() const {return m_Tunings.size();}

	std::string GetName() const { return m_Name; }
	void SetName(const std::string name) { m_Name = name; }

#ifndef MODPLUG_NO_FILESAVE
	void SetSavefilePath(const mpt::PathString &psz) {m_SavefilePath = psz;}
	mpt::PathString GetSaveFilePath() const {return m_SavefilePath;}
#endif // MODPLUG_NO_FILESAVE

	size_t GetNameLengthMax() const {return 256;}

	//Serialization/unserialisation
	bool Serialize(std::ostream&) const;
	bool Deserialize(std::istream&);
#ifndef MODPLUG_NO_FILESAVE
	bool Serialize() const;
	bool Deserialize();
#endif // MODPLUG_NO_FILESAVE

//END INTERFACE
	

//BEGIN: DATA MEMBERS
private:
	//BEGIN: SERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_Tunings; //The actual tuningobjects are stored as deletable pointers here.
	std::string m_Name;
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

	bool Remove(TITER removable, bool moveToTrashBin = true);

	//Hiding default operators because default meaning might not work right.
	CTuningCollection& operator=(const CTuningCollection&) {return *this;}
	CTuningCollection(const CTuningCollection&) {}

	bool DeserializeOLD(std::istream&, bool& loadingSuccessful);

//END PRIVATE METHODS.
};


#ifdef MODPLUG_TRACKER
bool UnpackTuningCollection(const mpt::PathString &filename, mpt::PathString dest = mpt::PathString());
bool UnpackTuningCollection(const CTuningCollection &tc, mpt::PathString dest = mpt::PathString());
#endif

OPENMPT_NAMESPACE_END
