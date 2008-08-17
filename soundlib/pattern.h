#ifndef PATTERN_H
#define PATTERN_H

#include <vector>
#include "modcommand.h"

using std::vector;

class CPatternContainer;

#define MAX_PATTERNNAME		32
#define MAX_PATTERN_ROWS	1024

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

	CHANNELINDEX GetNumChannels() const;

	bool Resize(const ROWINDEX newRowCount, const bool showDataLossWarning = true);

	bool ClearData();

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



#endif
