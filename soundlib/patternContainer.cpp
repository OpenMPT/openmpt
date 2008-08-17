#include "stdafx.h"
#include "patternContainer.h"
#include "sndfile.h"
#include "../mptrack/mainfrm.h"

int CPatternContainer::Insert(const ROWINDEX rows)
//---------------------------------------------
{
	PATTERNINDEX i = 0;
	for(i = 0; i<m_Patterns.size(); i++)
		if(!m_Patterns[i]) break;
	if(Insert(i, rows))
		return -1;
	else return i;

}


bool CPatternContainer::Insert(const PATTERNINDEX index, const ROWINDEX rows)
//---------------------------------------------------------------
{
	const CModSpecifications& specs = m_rSndFile.GetModSpecifications();
	if(index >= specs.patternsMax || index > m_Patterns.size() || rows > specs.patternRowsMax)
		return true;
	if(index < m_Patterns.size() && m_Patterns[index])
		return true;

	if(index == m_Patterns.size())
	{
		if(index < specs.patternsMax)
			m_Patterns.push_back(MODPATTERN(*this));
		else 
		{
			ErrorBox(IDS_ERR_TOOMANYPAT, CMainFrame::GetMainFrame());
			return true;
		}
	}

	m_Patterns[index] = CSoundFile::AllocatePattern(rows, m_rSndFile.m_nChannels);
	m_Patterns[index].m_Rows = rows;

	if(!m_Patterns[index]) return true;

	return false;
}

bool CPatternContainer::Remove(const PATTERNINDEX ipat)
//----------------------------------------------
{
	if(ipat >= m_Patterns.size())
		return true;

	return m_Patterns[ipat].ClearData();
}

PATTERNINDEX CPatternContainer::GetIndex(const MODPATTERN* const pPat) const
//------------------------------------------------------------------
{
	const PATTERNINDEX endI = static_cast<PATTERNINDEX>(m_Patterns.size());
	for(PATTERNINDEX i = 0; i<endI; i++)
		if(&m_Patterns[i] == pPat) return i;

	return endI;
}


void CPatternContainer::ResizeArray(const PATTERNINDEX newSize)
//-------------------------------------------------------------
{
	if(Size() <= newSize)
		m_Patterns.resize(newSize, MODPATTERN(*this));
	else
	{
		for(PATTERNINDEX i = Size(); i > newSize; i--) Remove(i-1);
		m_Patterns.resize(newSize, MODPATTERN(*this));
	}
}


void CPatternContainer::OnModTypeChanged(const MODTYPE /*oldtype*/)
//----------------------------------------------------------
{
	const CModSpecifications specs = m_rSndFile.GetModSpecifications();
	if(specs.patternsMax < Size()) ResizeArray(max(MAX_PATTERNS, specs.patternsMax));
	else if(Size() < MAX_PATTERNS) ResizeArray(MAX_PATTERNS);
}

