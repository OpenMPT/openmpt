#ifndef PATTERN_H
#define PATTERN_H

#include <vector>
#include "modcommand.h"

using std::vector;

class CPatternContainer;
class CSoundFile;

typedef MODCOMMAND* PatternRow;

//============
class CPattern
//============
{
	friend class CPatternContainer;
	
public:
//BEGIN: OPERATORS
	//To mimic MODCOMMAND*
	operator MODCOMMAND*() {return m_ModCommands;}
	operator const MODCOMMAND*() const {return m_ModCommands;}
	CPattern& operator=(MODCOMMAND* const p) {m_ModCommands = p; return *this;}
	CPattern& operator=(const CPattern& pat)
	{
		m_ModCommands = pat.m_ModCommands;
		m_Rows = pat.m_Rows;
		return *this;
	}
//END: OPERATORS

//BEGIN: INTERFACE METHODS
public:
	MODCOMMAND* GetpModCommand(const ROWINDEX r, const CHANNELINDEX c) {return &m_ModCommands[r*GetNumChannels()+c];}
	const MODCOMMAND* GetpModCommand(const ROWINDEX r, const CHANNELINDEX c) const {return &m_ModCommands[r*GetNumChannels()+c];}
	
	ROWINDEX GetNumRows() const {return m_Rows;}

	// Return true if modcommand can be accessed from given row, false otherwise.
	bool IsValidRow(const ROWINDEX iRow) const {return (iRow < GetNumRows());}

	// Return PatternRow object which has operator[] defined so that MODCOMMAND
	// at (iRow, iChn) can be accessed with GetRow(iRow)[iChn].
	PatternRow GetRow(const ROWINDEX iRow) {return GetpModCommand(iRow, 0);}

	CHANNELINDEX GetNumChannels() const;

	bool Resize(const ROWINDEX newRowCount, const bool showDataLossWarning = true);

	// Deallocates pattern data. 
	void Deallocate();

	// Removes all modcommands from the pattern.
	void ClearCommands();

	// Returns associated soundfile.
	CSoundFile& GetSoundFile();
	const CSoundFile& GetSoundFile() const;

	bool SetData(MODCOMMAND* p, const ROWINDEX rows) {m_ModCommands = p; m_Rows = rows; return false;}

	bool Expand();

	bool Shrink();

	bool WriteITPdata(FILE* f) const;
	bool ReadITPdata(const BYTE* const lpStream, DWORD& streamPos, const DWORD datasize, const DWORD dwMemLength);
	//Parameters:
	//1. Pointer to the beginning of the stream
	//2. Tells where to start(lpStream+streamPos) and number of bytes read is added to it.
	//3. How many bytes to read
	//4. Length of the stream.
	//Returns true on error.

//END: INTERFACE METHODS

	typedef MODCOMMAND* iterator;
	typedef const MODCOMMAND *const_iterator;

	iterator Begin() {return m_ModCommands;}
	const_iterator Begin() const {return m_ModCommands;}

	iterator End() {return (m_ModCommands != nullptr) ? m_ModCommands + m_Rows * GetNumChannels() : nullptr;}
	const_iterator End() const {return (m_ModCommands != nullptr) ? m_ModCommands + m_Rows * GetNumChannels() : nullptr;}

	CPattern(CPatternContainer& patCont) : m_ModCommands(0), m_Rows(64), m_rPatternContainer(patCont) {}

private:
	MODCOMMAND& GetModCommand(ROWINDEX i) {return m_ModCommands[i];}
	//Returns modcommand from (floor[i/channelCount], i%channelCount) 

	MODCOMMAND& GetModCommand(ROWINDEX r, CHANNELINDEX c) {return m_ModCommands[r*GetNumChannels()+c];}
	const MODCOMMAND& GetModCommand(ROWINDEX r, CHANNELINDEX c) const {return m_ModCommands[r*GetNumChannels()+c];}


//BEGIN: DATA
private:
	MODCOMMAND* m_ModCommands;
	ROWINDEX m_Rows;
	CPatternContainer& m_rPatternContainer;
//END: DATA
};


const char FileIdPattern[] = "mptP";

void ReadModPattern(std::istream& iStrm, CPattern& patc, const size_t nSize = 0);
void WriteModPattern(std::ostream& oStrm, const CPattern& patc);


#endif
