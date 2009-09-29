#ifndef PATTERNCONTAINER_H
#define PATTERNCONTAINER_H

#include "pattern.h"
#include "Snd_defs.h"

class CSoundFile;
typedef CPattern MODPATTERN;

//=====================
class CPatternContainer
//=====================
{
//BEGIN: TYPEDEFS
public:
	typedef vector<MODPATTERN> PATTERNVECTOR;
//END: TYPEDEFS


//BEGIN: OPERATORS
public:
	//To mimic old pattern == MODCOMMAND* behavior.
	MODPATTERN& operator[](const int pat) {return m_Patterns[pat];}
	const MODPATTERN& operator[](const int pat) const {return m_Patterns[pat];}
//END: OPERATORS

//BEGIN: INTERFACE METHODS
public:
	CPatternContainer(CSoundFile& sndFile) : m_rSndFile(sndFile) {m_Patterns.assign(MAX_PATTERNS, MODPATTERN(*this));}

	// Clears existing patterns and resizes array to default size.
	void Init();

	//Note: No memory handling here.
	void ClearPatterns() {m_Patterns.assign(m_Patterns.size(), MODPATTERN(*this));}
	
	//Insert (default)pattern to given position. If pattern already exists at that position,
	//ignoring request.
	bool Insert(const PATTERNINDEX index, const ROWINDEX rows);
	
	//Insert pattern to position with the lowest index, and return that index, PATTERNINDEX_INVALID
	//on failure.
	PATTERNINDEX Insert(const ROWINDEX rows);

	//Remove pattern from given position. Currently it actually makes the pattern
	//'invisible' - the pattern data is cleared but the actual pattern object won't get removed.
	bool Remove(const PATTERNINDEX index);

	PATTERNINDEX Size() const {return static_cast<PATTERNINDEX>(m_Patterns.size());}

	CSoundFile& GetSoundFile() {return m_rSndFile;}
	const CSoundFile& GetSoundFile() const {return m_rSndFile;}

	//Returns the index of given pattern, Size() if not found.
	PATTERNINDEX GetIndex(const MODPATTERN* const pPat) const;

	// Return true if pattern can be accessed with operator[](iPat), false otherwise.
	bool IsValidIndex(const PATTERNINDEX iPat) const {return (iPat < Size());}

	// Return true if IsValidIndex() is true and the corresponding pattern has allocated modcommand array, false otherwise.
	bool IsValidPat(const PATTERNINDEX iPat) const {return IsValidIndex(iPat) && (*this)[iPat];}
	
	void ResizeArray(const PATTERNINDEX newSize);

	void OnModTypeChanged(const MODTYPE oldtype);

//END: INTERFACE METHODS


//BEGIN: DATA MEMBERS
private:
	PATTERNVECTOR m_Patterns;
	CSoundFile& m_rSndFile;
//END: DATA MEMBERS

};


const char FileIdPatterns[] = "mptPc";

void ReadModPatterns(std::istream& iStrm, CPatternContainer& patc, const size_t nSize = 0);
void WriteModPatterns(std::ostream& oStrm, const CPatternContainer& patc);

#endif

