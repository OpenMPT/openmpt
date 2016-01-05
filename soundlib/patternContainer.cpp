/*
 * PatternContainer.cpp
 * --------------------
 * Purpose: Container class for managing patterns.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "patternContainer.h"
#include "Sndfile.h"
#include "mod_specifications.h"
#include "../common/serialization_utils.h"
#include "../common/version.h"


OPENMPT_NAMESPACE_BEGIN


void CPatternContainer::ClearPatterns()
//-------------------------------------
{
	DestroyPatterns();
	m_Patterns.assign(m_Patterns.size(), MODPATTERN(*this));
}


void CPatternContainer::DestroyPatterns()
//---------------------------------------
{
	for(PATTERNINDEX i = 0; i < m_Patterns.size(); i++)
	{
		Remove(i);
	}
}


PATTERNINDEX CPatternContainer::Duplicate(PATTERNINDEX from)
//----------------------------------------------------------
{
	if(!IsValidPat(from))
	{
		return PATTERNINDEX_INVALID;
	}

	const CPattern oldPat = m_Patterns[from];
	PATTERNINDEX newPatIndex = Insert(oldPat.GetNumRows());

	if(newPatIndex != PATTERNINDEX_INVALID)
	{
		CPattern &newPat = m_Patterns[newPatIndex];
		memcpy(newPat.m_ModCommands, oldPat.m_ModCommands, newPat.GetNumChannels() * newPat.GetNumRows() * sizeof(ModCommand));
		newPat.m_Rows = oldPat.m_Rows;
		newPat.m_RowsPerBeat = oldPat.m_RowsPerBeat;
		newPat.m_RowsPerMeasure = oldPat.m_RowsPerMeasure;
		newPat.m_PatternName = oldPat.m_PatternName;
	}
	return newPatIndex;
}


PATTERNINDEX CPatternContainer::Insert(const ROWINDEX rows)
//---------------------------------------------------------
{
	PATTERNINDEX i = 0;
	for(i = 0; i < m_Patterns.size(); i++)
		if(!m_Patterns[i]) break;
	if(!Insert(i, rows))
		return PATTERNINDEX_INVALID;
	else return i;

}


bool CPatternContainer::Insert(const PATTERNINDEX index, const ROWINDEX rows)
//---------------------------------------------------------------------------
{
	if(rows > MAX_PATTERN_ROWS || rows == 0)
		return false;
	if(index < m_Patterns.size() && m_Patterns[index])
		return false;

	if(index >= m_Patterns.size())
	{
		m_Patterns.resize(index + 1, MODPATTERN(*this));
	}

	m_Patterns[index].AllocatePattern(rows);
	m_Patterns[index].RemoveSignature();
	m_Patterns[index].SetName("");

	return m_Patterns[index] != nullptr;
}


void CPatternContainer::Remove(const PATTERNINDEX ipat)
//-----------------------------------------------------
{
	if(ipat < m_Patterns.size()) m_Patterns[ipat].Deallocate();
}


bool CPatternContainer::IsPatternEmpty(const PATTERNINDEX nPat) const
//-------------------------------------------------------------------
{
	if(!IsValidPat(nPat))
		return false;
	
	const ModCommand *m = m_Patterns[nPat].m_ModCommands;
	for(const ModCommand *mEnd = m + m_Patterns[nPat].GetNumChannels() * m_Patterns[nPat].GetNumRows(); m != mEnd; m++)
	{
		if(!m->IsEmpty())
			return false;
	}
	return true;
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
//-----------------------------------------------------------------
{
	const CModSpecifications specs = m_rSndFile.GetModSpecifications();
	if(specs.patternsMax < Size())
		ResizeArray(MAX(MAX_PATTERNS, specs.patternsMax));
	else if(Size() < MAX_PATTERNS)
		ResizeArray(MAX_PATTERNS);

	// remove pattern time signatures
	if(!specs.hasPatternSignatures)
	{
		for(PATTERNINDEX nPat = 0; nPat < m_Patterns.size(); nPat++)
		{
			m_Patterns[nPat].RemoveSignature();
			m_Patterns[nPat].RemoveTempoSwing();
		}
	}
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


PATTERNINDEX CPatternContainer::GetNumPatterns() const
//----------------------------------------------------
{
	if(Size() == 0)
	{
		return 0;
	}
	for(PATTERNINDEX nPat = Size(); nPat > 0; nPat--)
	{
		if(IsValidPat(nPat - 1))
		{
			return nPat;
		}
	}
	return 0;
}


PATTERNINDEX CPatternContainer::GetNumNamedPatterns() const
//---------------------------------------------------------
{
	if(Size() == 0)
	{
		return 0;
	}
	for(PATTERNINDEX nPat = Size(); nPat > 0; nPat--)
	{
		if(!m_Patterns[nPat - 1].GetName().empty())
		{
			return nPat;
		}
	}
	return 0;
}



void WriteModPatterns(std::ostream& oStrm, const CPatternContainer& patc)
//----------------------------------------------------------------------
{
	srlztn::SsbWrite ssb(oStrm);
	ssb.BeginWrite(FileIdPatterns, MptVersion::num);
	const PATTERNINDEX nPatterns = patc.Size();
	uint16 nCount = 0;
	for(uint16 i = 0; i < nPatterns; i++) if (patc[i])
	{
		ssb.WriteItem(patc[i], srlztn::ID::FromInt<uint16>(i), &WriteModPattern);
		nCount = i + 1;
	}
	ssb.WriteItem<uint16>(nCount, "num"); // Index of last pattern + 1.
	ssb.FinishWrite();
}


void ReadModPatterns(std::istream& iStrm, CPatternContainer& patc, const size_t)
//--------------------------------------------------------------------------------
{
	srlztn::SsbRead ssb(iStrm);
	ssb.BeginRead(FileIdPatterns, MptVersion::num);
	if ((ssb.GetStatus() & srlztn::SNT_FAILURE) != 0)
		return;
	PATTERNINDEX nPatterns = patc.Size();
	uint16 nCount = uint16_max;
	if (ssb.ReadItem(nCount, "num") != srlztn::SsbRead::EntryNotFound)
		nPatterns = nCount;
	LimitMax(nPatterns, ModSpecs::mptm.patternsMax);
	if (nPatterns > patc.Size())
		patc.ResizeArray(nPatterns);
	for(uint16 i = 0; i < nPatterns; i++)
	{
		ssb.ReadItem(patc[i], srlztn::ID::FromInt<uint16>(i), &ReadModPattern);
	}
}


OPENMPT_NAMESPACE_END
