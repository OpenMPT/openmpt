#include "stdafx.h"
#include "patternContainer.h"
#include "sndfile.h"
#include "../mptrack/mainfrm.h"
#include "../mptrack/serialization_utils.h"
#include "../mptrack/version.h"

PATTERNINDEX CPatternContainer::Insert(const ROWINDEX rows)
//---------------------------------------------
{
	PATTERNINDEX i = 0;
	for(i = 0; i<m_Patterns.size(); i++)
		if(!m_Patterns[i]) break;
	if(Insert(i, rows))
		return PATTERNINDEX_INVALID;
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
	m_Patterns[ipat].Deallocate();
	return false;
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


void CPatternContainer::Init()
//----------------------------
{
	for(PATTERNINDEX i = 0; i < Size(); i++)
	{
		Remove(i);
	}

	ResizeArray(MAX_PATTERNS);
}


void WriteModPatterns(std::ostream& oStrm, const CPatternContainer& patc)
//----------------------------------------------------------------------
{
	srlztn::Ssb ssb(oStrm);
	ssb.BeginWrite(FileIdPatterns, MptVersion::num);
	const PATTERNINDEX nPatterns = patc.Size();
	uint16 nCount = 0;
	for(uint16 i = 0; i < nPatterns; i++) if (patc[i])
	{
		ssb.WriteItem(patc[i], &i, sizeof(i), &WriteModPattern);
		nCount++;
	}
	ssb.WriteItem<uint16>(nCount, "num");
	ssb.FinishWrite();
}


void ReadModPatterns(std::istream& iStrm, CPatternContainer& patc, const size_t)
//--------------------------------------------------------------------------------
{
	srlztn::Ssb ssb(iStrm);
	ssb.BeginRead(FileIdPatterns, MptVersion::num);
	if ((ssb.m_Status & srlztn::SNT_FAILURE) != 0)
		return;
	PATTERNINDEX nPatterns = patc.Size();
	uint16 nCount = uint16_max;
	if (ssb.ReadItem(nCount, "num") != srlztn::Ssb::EntryNotFound)
		nPatterns = nCount;
	LimitMax(nPatterns, ModSpecs::mptm.patternsMax);
	if (nPatterns > patc.Size())
		patc.ResizeArray(nPatterns);	
	for(uint16 i = 0; i < nPatterns; i++)
	{
		ssb.ReadItem(patc[i], &i, sizeof(i), &ReadModPattern);
	}
}

