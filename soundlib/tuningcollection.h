#ifndef TUNINGCOLLECTION_H
#define TUNINGCOLLECTION_H

#include "tuning.h"
#include "../mptrack/serialization_utils.h"
#include <vector>
#include <string>


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

	typedef vector<CTuning* const> TUNINGVECTOR;
	typedef TUNINGVECTOR::iterator TITER; //Tuning ITERator.
	typedef TUNINGVECTOR::const_iterator CTITER;

	typedef uint16 SERIALIZATION_VERSION;

	typedef bool SERIALIZATION_RETURN_TYPE;

//END TYPEDEFS

//BEGIN PUBLIC STATIC CONSTS
public:
	static const EDITMASK EM_ADD; //true <~> allowed
	static const EDITMASK EM_REMOVE;
	static const EDITMASK EM_ALLOWALL;
	static const EDITMASK EM_CONST;

	static const SERIALIZATION_VERSION s_SerializationVersion;

	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_SUCCESS;
	static const SERIALIZATION_RETURN_TYPE SERIALIZATION_FAILURE;

	static const string s_FileExtension;

//END PUBLIC STATIC CONSTS

//BEGIN INTERFACE:
public:
	CTuningCollection(const string& name = "");
	~CTuningCollection();
	
	//Note: Given pointer is deleted by CTuningCollection
	//at some point.
	bool AddTuning(CTuning* const pT);
	bool AddTuning(istream& inStrm) {return AddTuning(inStrm, false);}
	
	bool Remove(const size_t i);
	bool Remove(const CTuning*);

	bool CanEdit(const EDITMASK& em) const {return (m_EditMask & em) != 0;}

	void SetConstStatus(const EDITMASK& em) {m_EditMask = em;}

	const EDITMASK& GetEditMask() const {return m_EditMask;}

	string GetEditMaskString() const;

	CTuning& GetTuning(size_t i) {return *m_Tunings.at(i);}
	const CTuning& GetTuning(size_t i) const {return *m_Tunings.at(i);}
	CTuning* GetTuning(const string& name);
	const CTuning* GetTuning(const string& name) const;

	size_t GetNumTunings() const {return m_Tunings.size();}

	const string& GetName() const {return m_Name;}

	void SetSavefilePath(const string& str) {m_SavefilePath = str;}
	const string& GetSaveFilePath() const {return m_SavefilePath;}

	string GetVersionString() const {return Stringify(s_SerializationVersion);}

	size_t GetNameLengthMax() const {return 256;}

	//Serialization/unserialisation
	bool Serialize(ostream&) const;
	bool Serialize() const;
	bool Unserialize(istream&);
	bool Unserialize();

	//Transfer tuning pT from pTCsrc to pTCdest
	static bool TransferTuning(CTuningCollection* pTCsrc, CTuningCollection* pTCdest, CTuning* pT);
	

//END INTERFACE
	

//BEGIN: DATA MEMBERS
private:
	//BEGIN: SERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_Tunings; //The actual tuningobjects are stored as deletable pointers here.
	string m_Name;
	EDITMASK m_EditMask;
	//END: SERIALIZABLE DATA MEMBERS

	//BEGIN: NONSERIALIZABLE DATA MEMBERS
	TUNINGVECTOR m_DeletedTunings; //See Remove()-method for explanation of this.
	string m_SavefilePath;
	//END: NONSERIALIZABLE DATA MEMBERS
	
//END: DATA MEMBERS

//BEGIN PRIVATE METHODS
private:
	CTuning* FindTuning(const string& name) const;
	size_t FindTuning(const CTuning* const) const;

	bool AddTuning(istream& inStrm, const bool ignoreEditmask);

	bool Remove(TITER removable, bool moveToTrashBin = true);

	//Hiding default operators because default meaning might not work right.
	CTuningCollection& operator=(const CTuningCollection&) {}
	CTuningCollection(const CTuningCollection&) {}

	bool UnserializeOLD(istream&, bool& loadingSuccessful);

//END PRIVATE METHODS.
};

using srlztn::INSTREAM;
using srlztn::OUTSTREAM;
using srlztn::OUTFLAG;
using srlztn::INFLAG;
using srlztn::ABCSerializationStreamer;
using srlztn::SNT_NOTE;
using srlztn::SNT_PROGRESS;

class CTuningstreamer : public ABCSerializationStreamer
//=====================================================
{
public:
	CTuningstreamer(const CTuning& t) : ABCSerializationStreamer(OUTFLAG), m_pTuning(&t), m_pTC(0) {}
	CTuningstreamer(CTuningCollection& rTC) : ABCSerializationStreamer(INFLAG), m_pTuning(0), m_pTC(&rTC) {}
		
	void ProWrite(OUTSTREAM& oStrm) const
	{
		m_pTuning->Serialize(oStrm);
	}
	void ProRead(INSTREAM& iStrm, const uint64)
	{
		if(m_pTC->AddTuning(iStrm, true))
			m_LastReadinfo = READINFO(SNT_NOTE, "Loading tuning failed");
		else
			m_LastReadinfo = READINFO(SNT_PROGRESS, "Done reading tuning " + m_pTC->GetTuning(m_pTC->GetNumTunings()-1).GetName());
	}
private:
	const CTuning* m_pTuning;
	CTuningCollection* m_pTC;
};

class CTuningCollectionStreamer : public ABCSerializationStreamer
//=====================================================
{
public:
	CTuningCollectionStreamer(CTuningCollection& tc) : ABCSerializationStreamer(INFLAG|OUTFLAG), m_rTC(tc) {}
		
	void ProWrite(OUTSTREAM& oStrm) const
	{
		if(m_rTC.Serialize(oStrm))
			m_LastWriteinfo = WRITEINFO(srlztn::SNT_WARNING, "Writing tuningcollection failed");
	}
	void ProRead(INSTREAM& iStrm, const uint64)
	{
		if(m_rTC.Unserialize(iStrm))
			m_LastReadinfo = READINFO(srlztn::SNT_WARNING, "Reading tuningcollection failed");

	}
private:
	CTuningCollection& m_rTC;
};


#endif
