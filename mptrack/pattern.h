#ifndef __SNDFILE_H
#include "../soundlib/sndfile.h"
#endif

#ifndef PATTERN_H
#define PATTERN_H

#include <vector>

using std::vector;

class CPatternContainer;

typedef UINT ROWINDEX;

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
	MODCOMMAND* GetpModCommand(const UINT r, const UINT c) {return &m_ModCommands[r*c+c];}
	const MODCOMMAND* GetpModCommand(const UINT r, const UINT c) const {return &m_ModCommands[r*c+c];}

	ROWINDEX Rows() const {return m_Rows;}

	bool Resize(const ROWINDEX newRowCount, const bool showDataLossWarning = true);

	bool ClearData();

	bool SetData(MODCOMMAND* p, const ROWINDEX rows) {m_ModCommands = p; m_Rows = rows; return false;}

	bool Expand();

	bool Shrink();

//END: INTERFACE METHODS

	CPattern(CPatternContainer& patCont) : m_ModCommands(0), m_Rows(64), m_rPatternContainer(patCont) {}

//BEGIN: DATA
private:
	MODCOMMAND* m_ModCommands;
	ROWINDEX m_Rows;
	CPatternContainer& m_rPatternContainer;
//END: DATA
};



#endif
