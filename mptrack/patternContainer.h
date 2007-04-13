#ifdef __SNDFILE_H

#ifndef PATTERNCONTAINER_H
#define PATTERNCONTAINER_H

#include "pattern.h"
#include <limits>

const UINT LIMIT_UINT_MAX = (std::numeric_limits<WORD>::max)();

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

	void ClearPatterns() {m_Patterns.assign(m_Patterns.size(), MODPATTERN(*this));}
	//Note: No memory handling here(yet).

	bool Insert(const PATTERNINDEX index, const ROWINDEX rows);
	//Insert (default)pattern to given position. If pattern already exists at that position,
	//ignoring request.

	int Insert(const ROWINDEX rows);
	//Insert pattern to position with the lowest index, and return that index, -1
	//on failure.

	bool Remove(const PATTERNINDEX index);
	//Remove pattern from given position. Currently it actually makes the pattern
	//'invisible' - the pattern data is cleared but the actual pattern object won't get removed.

	size_t Size() const {return m_Patterns.size();}

	CSoundFile& GetSoundFile() {return m_rSndFile;}

	UINT GetIndex(const MODPATTERN* const pPat) const;
	//Returns the index of given pattern, Size() if not found.

	UINT GetInvalidIndex() const; //To correspond 0xFF
	UINT GetIgnoreIndex() const; //To correspond 0xFE

	UINT GetPatternNumberLimitMax() const;

//END: INTERFACE METHODS


//BEGIN: DATA MEMBERS
private:
	PATTERNVECTOR m_Patterns;
	CSoundFile& m_rSndFile;
//END: DATA MEMBERS

};

#endif
#endif
