#include "stdafx.h"
#include "patternContainer.h"
#include "sndfile.h"
#include "mainfrm.h"

int CPatternContainer::Insert(const ROWINDEX rows)
//---------------------------------------------
{
	size_t i = 0;
	for(i = 0; i<m_Patterns.size(); i++)
		if(!m_Patterns[i]) break;
	if(Insert(static_cast<WORD>(i), rows))
		return -1;
	else return i;

}


bool CPatternContainer::Insert(const PATTERNINDEX index, const ROWINDEX rows)
//---------------------------------------------------------------
{
	if(index > m_Patterns.size() || rows > m_rSndFile.GetRowMax())
		return true;
	if(index < m_Patterns.size() && m_Patterns[index])
		return true;

	if(index == m_Patterns.size())
	{
		if(index < GetPatternNumberLimitMax())
			m_Patterns.push_back(MODPATTERN(*this));
		else 
		{
			ErrorBox(IDS_ERR_TOOMANYPAT, CMainFrame::GetMainFrame());
			return true;
		}
	}

	m_Patterns[index] = CSoundFile::AllocatePattern(rows, m_rSndFile.m_nChannels);
	m_Patterns[index].m_Rows = rows;

	return false;
}

bool CPatternContainer::Remove(const PATTERNINDEX ipat)
//----------------------------------------------
{
	if(ipat >= m_Patterns.size())
		return true;

	return m_Patterns[ipat].ClearData();
}

UINT CPatternContainer::GetIndex(const MODPATTERN* const pPat) const
//------------------------------------------------------------------
{
	for(UINT i = 0; i<m_Patterns.size(); i++)
		if(&m_Patterns[i] == pPat) return i;

	return m_Patterns.size();
}

//NOTE: Various methods around the code are, unsurprisingly, bounded to the way things
//were handled with 0xFE and 0xFF, so changing these return values might cause problems.
UINT CPatternContainer::GetInvalidIndex() const {return m_rSndFile.m_nType == MOD_TYPE_MPT ?  LIMIT_UINT_MAX : 0xFF;}
UINT CPatternContainer::GetIgnoreIndex() const {return m_rSndFile.m_nType == MOD_TYPE_MPT ? LIMIT_UINT_MAX-1 : 0xFE;}

UINT CPatternContainer::GetPatternNumberLimitMax() const
//-------------------------------------------------------
{
	const UINT type = m_rSndFile.m_nType;
	if(type == MOD_TYPE_MPT) return MPTM_SPECS.patternsMax;
	if(type == MOD_TYPE_MOD) return 128;
	return MAX_PATTERNS;
}

