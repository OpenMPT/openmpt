/*
 * ModSequence.cpp
 * ---------------
 * Purpose: Order and sequence handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "sndfile.h"
#include "ModSequence.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif // MODPLUG_TRACKER
#include "../common/version.h"
#include "../common/serialization_utils.h"
#include "../common/Reporting.h"
#include <functional>

#define str_SequenceTruncationNote (GetStrI18N((_TEXT("Module has sequence of length %u; it will be truncated to maximum supported length, %u."))))

#define new DEBUG_NEW


ModSequence::ModSequence(CSoundFile& rSf,
						 PATTERNINDEX* pArray,
						 ORDERINDEX nSize,
						 ORDERINDEX nCapacity, 
						 const bool bDeletableArray) :
		m_pSndFile(&rSf),
		m_pArray(pArray),
		m_nSize(nSize),
		m_nCapacity(nCapacity),
		m_bDeletableArray(bDeletableArray),
		m_nInvalidIndex(0xFF),
		m_nIgnoreIndex(0xFE)
//-------------------------------------------------------
{}


ModSequence::ModSequence(CSoundFile& rSf, ORDERINDEX nSize) :
	m_pSndFile(&rSf),
	m_bDeletableArray(true),
	m_nInvalidIndex(GetInvalidPatIndex(MOD_TYPE_MPT)),
	m_nIgnoreIndex(GetIgnoreIndex(MOD_TYPE_MPT))
//-------------------------------------------------------------------
{
	m_nSize = nSize;
	m_nCapacity = m_nSize;
	m_pArray = new PATTERNINDEX[m_nCapacity];
	std::fill(begin(), end(), GetInvalidPatIndex(MOD_TYPE_MPT));
}


ModSequence::ModSequence(const ModSequence& seq) :
	m_pSndFile(seq.m_pSndFile),
	m_bDeletableArray(false),
	m_nInvalidIndex(0xFF),
	m_nIgnoreIndex(0xFE),
	m_nSize(0),
	m_nCapacity(0),
	m_pArray(nullptr)
//------------------------------------------
{
	*this = seq;
}


bool ModSequence::NeedsExtraDatafield() const
//-------------------------------------------
{
	if(m_pSndFile->GetType() == MOD_TYPE_MPT && m_pSndFile->Patterns.Size() > 0xFD)
		return true;
	else
		return false;
}

namespace
{
	// Functor for detecting non-valid patterns from sequence.
	struct IsNotValidPat
	{
		IsNotValidPat(CSoundFile& sndFile) : rSndFile(sndFile) {}
		bool operator()(PATTERNINDEX i) {return !rSndFile.Patterns.IsValidPat(i);}
		CSoundFile& rSndFile;
	};
}


void ModSequence::AdjustToNewModType(const MODTYPE oldtype)
//---------------------------------------------------------
{
	const CModSpecifications specs = m_pSndFile->GetModSpecifications();
	const MODTYPE newtype = m_pSndFile->GetType();

	m_nInvalidIndex = GetInvalidPatIndex(newtype);
	m_nIgnoreIndex = GetIgnoreIndex(newtype);

	// If not supported, remove "+++" separator order items.
	if (specs.hasIgnoreIndex == false)
	{
		if (oldtype != MOD_TYPE_NONE)
		{
			iterator i = std::remove_if(begin(), end(), std::bind2nd(std::equal_to<PATTERNINDEX>(), GetIgnoreIndex(oldtype)));
			std::fill(i, end(), GetInvalidPatIndex());
		}
	}
	else
		Replace(GetIgnoreIndex(oldtype), GetIgnoreIndex());

	//Resize orderlist if needed.
	if (specs.ordersMax < GetLength())
	{
		// Order list too long? -> remove unnecessary order items first.
		if (oldtype != MOD_TYPE_NONE && specs.ordersMax < GetLengthTailTrimmed())
		{
			iterator iter = std::remove_if(begin(), end(), IsNotValidPat(*m_pSndFile));
			std::fill(iter, end(), GetInvalidPatIndex());
			if(GetLengthTailTrimmed() > specs.ordersMax)
			{
				m_pSndFile->AddToLog("WARNING: Order list has been trimmed!");
			}
		}
		resize(specs.ordersMax);
	}
		
	// Replace items used to denote end of song order.
	Replace(GetInvalidPatIndex(oldtype), GetInvalidPatIndex());
}


ORDERINDEX ModSequence::GetLengthTailTrimmed() const
//--------------------------------------------------
{
	ORDERINDEX nEnd = GetLength();
	if(nEnd == 0) return 0;
	nEnd--;
	const PATTERNINDEX iInvalid = GetInvalidPatIndex();
	while(nEnd > 0 && (*this)[nEnd] == iInvalid)
		nEnd--;
	return ((*this)[nEnd] == iInvalid) ? 0 : nEnd+1;
}


ORDERINDEX ModSequence::GetLengthFirstEmpty() const
//-------------------------------------------------
{
	return static_cast<ORDERINDEX>(std::find(begin(), end(), GetInvalidPatIndex()) - begin());
}


ORDERINDEX ModSequence::GetNextOrderIgnoringSkips(const ORDERINDEX start) const
//-----------------------------------------------------------------------------
{
	const ORDERINDEX nLength = GetLength();
	if(nLength == 0) return 0;
	ORDERINDEX next = Util::Min(ORDERINDEX(nLength - 1), ORDERINDEX(start + 1));
	while(next+1 < nLength && (*this)[next] == GetIgnoreIndex()) next++;
	return next;
}


ORDERINDEX ModSequence::GetPreviousOrderIgnoringSkips(const ORDERINDEX start) const
//---------------------------------------------------------------------------------
{
	const ORDERINDEX nLength = GetLength();
	if(start == 0 || nLength == 0) return 0;
	ORDERINDEX prev = Util::Min(ORDERINDEX(start - 1), ORDERINDEX(nLength - 1));
	while(prev > 0 && (*this)[prev] == GetIgnoreIndex()) prev--;
	return prev;
}


void ModSequence::Init()
//----------------------
{
	resize(MAX_ORDERS);
	std::fill(begin(), end(), GetInvalidPatIndex());
}


void ModSequence::Remove(ORDERINDEX nPosBegin, ORDERINDEX nPosEnd)
//----------------------------------------------------------------
{
	const ORDERINDEX nLengthTt = GetLengthTailTrimmed();
	if (nPosEnd < nPosBegin || nPosEnd >= nLengthTt)
		return;
	const ORDERINDEX nMoveCount = nLengthTt - (nPosEnd + 1);
	// Move orders left.
	if (nMoveCount > 0)
		memmove(m_pArray + nPosBegin, m_pArray + nPosEnd + 1, sizeof(PATTERNINDEX) * nMoveCount);
	// Clear tail orders.
	std::fill(m_pArray + nPosBegin + nMoveCount, m_pArray + nLengthTt, GetInvalidPatIndex());
}


ORDERINDEX ModSequence::Insert(ORDERINDEX nPos, ORDERINDEX nCount, PATTERNINDEX nFill)
//------------------------------------------------------------------------------------
{
	if (nPos >= m_pSndFile->GetModSpecifications().ordersMax || nCount == 0)
		return (nCount = 0);
	const ORDERINDEX nLengthTt = GetLengthTailTrimmed();
	// Limit number of orders to be inserted.
	LimitMax(nCount, ORDERINDEX(m_pSndFile->GetModSpecifications().ordersMax - nPos));
	// Calculate new length.
	const ORDERINDEX nNewLength = min(nLengthTt + nCount, m_pSndFile->GetModSpecifications().ordersMax);
	// Resize if needed.
	if (nNewLength > GetLength())
		resize(nNewLength);
	// Calculate how many orders would need to be moved(nNeededSpace) and how many can actually
	// be moved(nFreeSpace).
	const ORDERINDEX nNeededSpace = nLengthTt - nPos;
	const ORDERINDEX nFreeSpace = GetLength() - (nPos + nCount);
	// Move orders nCount steps right starting from nPos.
	if (nPos < nLengthTt)
		memmove(m_pArray + nPos + nCount, m_pArray + nPos, min(nFreeSpace, nNeededSpace) * sizeof(PATTERNINDEX));
	// Set nFill to new orders.
	std::fill(begin() + nPos, begin() + nPos + nCount, nFill);
	return nCount;
}


void ModSequence::Append(PATTERNINDEX nPat)
//-----------------------------------------
{
	resize(m_nSize + 1, nPat);
}


void ModSequence::resize(ORDERINDEX nNewSize, PATTERNINDEX nFill)
//---------------------------------------------------------------
{
	if (nNewSize == m_nSize) return;
	if (nNewSize <= m_nCapacity)
	{
		if (nNewSize > m_nSize)
			std::fill(begin() + m_nSize, begin() + nNewSize, nFill);
		m_nSize = nNewSize;
	}
	else
	{
		const PATTERNINDEX* const pOld = m_pArray;
		m_nCapacity = nNewSize + 100;
		m_pArray = new PATTERNINDEX[m_nCapacity];
		std::copy(pOld, pOld+m_nSize, m_pArray);
		std::fill(m_pArray + m_nSize, m_pArray + nNewSize, nFill);
		m_nSize = nNewSize;
		if (m_bDeletableArray)
			delete[] pOld;
		m_bDeletableArray = true;
	}
}


void ModSequence::clear()
//-----------------------
{
	m_nSize = 0;
}


ModSequence& ModSequence::operator=(const ModSequence& seq)
//---------------------------------------------------------
{
	if (&seq == this)
		return *this;
	m_nIgnoreIndex = seq.m_nIgnoreIndex;
	m_nInvalidIndex = seq.m_nInvalidIndex;
	resize(seq.GetLength());
	std::copy(seq.begin(), seq.end(), begin());
	m_sName = seq.m_sName;
	return *this;
}


ORDERINDEX ModSequence::FindOrder(PATTERNINDEX nPat, ORDERINDEX startFromOrder, bool searchForward) const
//-------------------------------------------------------------------------------------------------------
{
	const ORDERINDEX maxOrder = GetLength();
	ORDERINDEX foundAtOrder = ORDERINDEX_INVALID;
	ORDERINDEX candidateOrder = 0;

	for (ORDERINDEX p = 0; p < maxOrder; p++)
	{
		if (searchForward)
		{
			candidateOrder = (startFromOrder + p) % maxOrder;				// wrap around MAX_ORDERS
		} else
		{
			candidateOrder = (startFromOrder - p + maxOrder) % maxOrder;	// wrap around 0 and MAX_ORDERS
		}
		if ((*this)[candidateOrder] == nPat)
		{
			foundAtOrder = candidateOrder;
			break;
		}
	}

	return foundAtOrder;
}


/////////////////////////////////////
// ModSequenceSet
/////////////////////////////////////


ModSequenceSet::ModSequenceSet(CSoundFile& sndFile)
	: ModSequence(sndFile, m_Cache, s_nCacheSize, s_nCacheSize, false),
	  m_nCurrentSeq(0)
//--------------------------------------------------------------
{
	m_Sequences.push_back(ModSequence(sndFile, s_nCacheSize));
}


const ModSequence& ModSequenceSet::GetSequence(SEQUENCEINDEX nSeq) const
//----------------------------------------------------------------------
{
	if(nSeq == GetCurrentSequenceIndex())
		return *this;
	else
		return m_Sequences[nSeq];
}


ModSequence& ModSequenceSet::GetSequence(SEQUENCEINDEX nSeq)
//----------------------------------------------------------
{
	if(nSeq == GetCurrentSequenceIndex())
		return *this;
	else
		return m_Sequences[nSeq];
}


void ModSequenceSet::CopyCacheToStorage()
//---------------------------------------
{
	m_Sequences[m_nCurrentSeq] = *this;
}


void ModSequenceSet::CopyStorageToCache()
//---------------------------------------
{
	const ModSequence& rSeq = m_Sequences[m_nCurrentSeq];
	if(rSeq.GetLength() <= s_nCacheSize)
	{
		PATTERNINDEX* pOld = m_pArray;
		m_pArray = m_Cache;
		m_nSize = rSeq.GetLength();
		m_nCapacity = s_nCacheSize;
		m_sName = rSeq.m_sName;
		std::copy(rSeq.m_pArray, rSeq.m_pArray+m_nSize, m_pArray);
		if (m_bDeletableArray)
			delete[] pOld;
		m_bDeletableArray = false;
	}
	else
		ModSequence::operator=(rSeq);
}


void ModSequenceSet::SetSequence(SEQUENCEINDEX n)
//-----------------------------------------------
{
	CopyCacheToStorage();
	m_nCurrentSeq = n;
	CopyStorageToCache();
}


SEQUENCEINDEX ModSequenceSet::AddSequence(bool bDuplicate)
//--------------------------------------------------------
{
	if(GetNumSequences() == MAX_SEQUENCES)
		return SEQUENCEINDEX_INVALID;
	m_Sequences.push_back(ModSequence(*m_pSndFile, s_nCacheSize)); 
	if (bDuplicate)
	{
		m_Sequences.back() = *this;
		m_Sequences.back().m_sName = ""; // Don't copy sequence name.
	}
	SetSequence(GetNumSequences() - 1);
	return GetNumSequences() - 1;
}


void ModSequenceSet::RemoveSequence(SEQUENCEINDEX i)
//--------------------------------------------------
{
	// Do nothing if index is invalid or if there's only one sequence left.
	if(i >= m_Sequences.size() || m_Sequences.size() <= 1) 
		return;
	const bool bSequenceChanges = (i == m_nCurrentSeq);
	m_Sequences.erase(m_Sequences.begin() + i);
	if(i < m_nCurrentSeq || m_nCurrentSeq >= GetNumSequences())
		m_nCurrentSeq--;
	if(bSequenceChanges)
		CopyStorageToCache();
}


void ModSequenceSet::OnModTypeChanged(const MODTYPE oldtype)
//----------------------------------------------------------
{
	const MODTYPE newtype = m_pSndFile->GetType();
	const SEQUENCEINDEX nSeqs = GetNumSequences();
	for(SEQUENCEINDEX n = 0; n < nSeqs; n++)
	{
		GetSequence(n).AdjustToNewModType(oldtype);
	}
	// Multisequences not suppported by other formats
	if(oldtype != MOD_TYPE_NONE && newtype != MOD_TYPE_MPT)
		MergeSequences();

	// Convert sequence with separator patterns into multiple sequences?
	if(oldtype != MOD_TYPE_NONE && newtype == MOD_TYPE_MPT && GetNumSequences() == 1)
		ConvertSubsongsToMultipleSequences();
}


bool ModSequenceSet::ConvertSubsongsToMultipleSequences()
//-------------------------------------------------------
{
	// Allow conversion only if there's only one sequence.
	if(GetNumSequences() != 1 || m_pSndFile->GetType() != MOD_TYPE_MPT)
		return false;

	bool hasSepPatterns = false;
	const ORDERINDEX nLengthTt = GetLengthTailTrimmed();
	for(ORDERINDEX nOrd = 0; nOrd < nLengthTt; nOrd++)
	{
		if(!m_pSndFile->Patterns.IsValidPat(At(nOrd)) && At(nOrd) != GetIgnoreIndex())
		{
			hasSepPatterns = true;
			break;
		}
	}
	bool modified = false;

	if(hasSepPatterns &&
		Reporting::Confirm("The order list contains separator items.\nThe new format supports multiple sequences, do you want to convert those separate tracks into multiple song sequences?",
		"Order list conversion", false, true) == cnfYes)
	{

		SetSequence(0);
		for(ORDERINDEX nOrd = 0; nOrd < GetLengthTailTrimmed(); nOrd++)
		{
			// end of subsong?
			if(!m_pSndFile->Patterns.IsValidPat(At(nOrd)) && At(nOrd) != GetIgnoreIndex())
			{
				ORDERINDEX oldLength = GetLengthTailTrimmed();
				// remove all separator patterns between current and next subsong first
				while(nOrd < oldLength && (!m_pSndFile->Patterns.IsValidIndex(At(nOrd))))
				{
					At(nOrd) = GetInvalidPatIndex();
					nOrd++;
					modified = true;
				}
				if(nOrd >= oldLength) break;
				ORDERINDEX startOrd = nOrd;
				modified = true;

				SEQUENCEINDEX newSeq = AddSequence(false);
				SetSequence(newSeq);

				// resize new seqeuence if necessary
				if(GetLength() < oldLength - startOrd)
				{
					resize(oldLength - startOrd);
				}

				// now, move all following orders to the new sequence
				while(nOrd < oldLength)
				{
					PATTERNINDEX copyPat = GetSequence(newSeq - 1)[nOrd];
					At(nOrd - startOrd) = copyPat;
					nOrd++;

					// is this a valid pattern? adjust pattern jump commands, if necessary.
					if(m_pSndFile->Patterns.IsValidPat(copyPat))
					{
						ModCommand *m = m_pSndFile->Patterns[copyPat];
						for(size_t len = m_pSndFile->Patterns[copyPat].GetNumRows() * m_pSndFile->m_nChannels; len; m++, len--)
						{
							if(m->command == CMD_POSITIONJUMP && m->param >= startOrd)
							{
								m->param = static_cast<BYTE>(m->param - startOrd);
							}
						}
					}
				}
				SetSequence(newSeq - 1);
				Remove(startOrd, oldLength - 1);
				SetSequence(newSeq);
				// start from beginning...
				nOrd = 0;
				if(GetLengthTailTrimmed() == 0 || !m_pSndFile->Patterns.IsValidIndex(At(nOrd))) break;
			}
		}
		SetSequence(0);
	}
	return modified;
}

bool ModSequenceSet::MergeSequences()
//-----------------------------------
{
	if(GetNumSequences() <= 1)
		return false;

	CHAR s[256];
	SetSequence(0);
	resize(GetLengthTailTrimmed());
	SEQUENCEINDEX removedSequences = 0; // sequence count
	vector <SEQUENCEINDEX> patternsFixed; // pattern fixed by other sequence already?
	patternsFixed.resize(m_pSndFile->Patterns.Size(), SEQUENCEINDEX_INVALID);
	// Set up vector
	for(ORDERINDEX nOrd = 0; nOrd < GetLengthTailTrimmed(); nOrd++)
	{
		PATTERNINDEX nPat = At(nOrd);
		if(!m_pSndFile->Patterns.IsValidPat(nPat)) continue;
		patternsFixed[nPat] = 0;
	}

	while(GetNumSequences() > 1)
	{
		removedSequences++;
		const ORDERINDEX nFirstOrder = GetLengthTailTrimmed() + 1; // +1 for separator item
		if(nFirstOrder + GetSequence(1).GetLengthTailTrimmed() > m_pSndFile->GetModSpecifications().ordersMax)
		{
			wsprintf(s, "WARNING: Cannot merge Sequence %d (too long!)", removedSequences);
			m_pSndFile->AddToLog(s);
			RemoveSequence(1);
			continue;
		}
		Append(GetInvalidPatIndex()); // Separator item
		for(ORDERINDEX nOrd = 0; nOrd < GetSequence(1).GetLengthTailTrimmed(); nOrd++)
		{
			PATTERNINDEX nPat = GetSequence(1)[nOrd];
			Append(nPat);

			// Try to fix patterns (Bxx commands)
			if(!m_pSndFile->Patterns.IsValidPat(nPat)) continue;

			ModCommand *m = m_pSndFile->Patterns[nPat];
			for(size_t len = 0; len < m_pSndFile->Patterns[nPat].GetNumRows() * m_pSndFile->m_nChannels; m++, len++)
			{
				if(m->command == CMD_POSITIONJUMP)
				{
					if(patternsFixed[nPat] != SEQUENCEINDEX_INVALID && patternsFixed[nPat] != removedSequences)
					{
						// Oops, some other sequence uses this pattern already.
						const PATTERNINDEX nNewPat = m_pSndFile->Patterns.Insert(m_pSndFile->Patterns[nPat].GetNumRows());
						if(nNewPat != SEQUENCEINDEX_INVALID)
						{
							// could create new pattern - copy data over and continue from here.
							At(nFirstOrder + nOrd) = nNewPat;
							ModCommand *pSrc = m_pSndFile->Patterns[nPat];
							ModCommand *pDest = m_pSndFile->Patterns[nNewPat];
							memcpy(pDest, pSrc, m_pSndFile->Patterns[nPat].GetNumRows() * m_pSndFile->m_nChannels * sizeof(ModCommand));
							m = pDest + len;
							patternsFixed.resize(max(nNewPat + 1, (PATTERNINDEX)patternsFixed.size()), SEQUENCEINDEX_INVALID);
							nPat = nNewPat;
						} else
						{
							// cannot create new pattern: notify the user
							wsprintf(s, "CONFLICT: Pattern break commands in Pattern %d might be broken since it has been used in several sequences!", nPat);
							m_pSndFile->AddToLog(s);
						}
					}
					m->param  = static_cast<BYTE>(m->param + nFirstOrder);
					patternsFixed[nPat] = removedSequences;
				}
			}

		}
		RemoveSequence(1);
	}
	// Remove order name + fill up with empty patterns.
	m_sName = "";
	const ORDERINDEX nMinLength = Util::Min(ModSequenceSet::s_nCacheSize, m_pSndFile->GetModSpecifications().ordersMax);
	if(GetLength() < nMinLength)
		resize(nMinLength);
	return true;
}


#ifdef MODPLUG_TRACKER
// Check if a playback position is currently locked (inaccessible)
bool ModSequence::IsPositionLocked(ORDERINDEX position)
{
	return(m_pSndFile->m_lockOrderStart != ORDERINDEX_INVALID
		&& (position < m_pSndFile->m_lockOrderStart || position > m_pSndFile->m_lockOrderEnd));
}
#endif // MODPLUG_TRACKER


/////////////////////////////////////
// Read/Write
/////////////////////////////////////


bool ModSequence::Deserialize(FileReader &file)
//---------------------------------------------
{
	if(file.BytesLeft() < 2 + 4) return false;

	uint16 version = file.ReadUint16LE();
	if(version != 0) return false;

	uint32 s = file.ReadUint32LE();
	if(s > 65000 || !file.CanRead(s * 4)) return false;

	const FileReader::off_t endPos = file.GetPosition() + s * 4;
	LimitMax(s, ModSpecs::mptm.ordersMax);

	resize(static_cast<ORDERINDEX>(s));
	for(size_t i = 0; i < s; i++)
	{
		(*this)[i] = static_cast<PATTERNINDEX>(file.ReadUint32LE());
	}
	file.Seek(endPos);
	return true;
}


size_t ModSequence::WriteAsByte(FILE* f, const uint16 count) const
//----------------------------------------------------------------
{
	const size_t limit = min(count, GetLength());

	size_t i = 0;
	
	for(i = 0; i < limit; i++)
	{
		const PATTERNINDEX pat = (*this)[i];
		BYTE temp = static_cast<BYTE>((*this)[i]);

		if(pat > 0xFD)
		{
			if(pat == GetInvalidPatIndex()) temp = 0xFF;
			else temp = 0xFE;
		}
		fwrite(&temp, 1, 1, f);
	}
	// Fill non-existing order items with stop indices
	for(i = limit; i < count; i++)
	{
		BYTE temp = 0xFF;
		fwrite(&temp, 1, 1, f);
	}
	return i; //Returns the number of bytes written.
}


bool ModSequence::ReadAsByte(const BYTE* pFrom, const int howMany, const int memLength)
//-------------------------------------------------------------------------------------
{
	if(howMany < 0 || howMany > memLength) return false;
	if(m_pSndFile->GetType() != MOD_TYPE_MPT && howMany > MAX_ORDERS) return false;
	
	if(GetLength() < static_cast<size_t>(howMany))
		resize(ORDERINDEX(howMany));
	
	for(int i = 0; i<howMany; i++, pFrom++)
		(*this)[i] = *pFrom;
	return true;
}


bool ModSequence::ReadAsByte(FileReader &file, size_t howMany)
//------------------------------------------------------------
{
	if(!file.CanRead(howMany))
	{
		return false;
	}
	ORDERINDEX readEntries = static_cast<ORDERINDEX>(howMany);
	if(!(m_pSndFile->GetType() & MOD_TYPE_MPT))
	{
		LimitMax(readEntries, MAX_ORDERS);
	}

	if(GetLength() < readEntries)
	{
		resize(readEntries);
	}

	for(int i = 0; i < readEntries; i++)
	{
		(*this)[i] = file.ReadUint8();
	}
	file.Skip(howMany - readEntries);
	return true;
}


void ReadModSequenceOld(std::istream& iStrm, ModSequenceSet& seq, const size_t)
//-----------------------------------------------------------------------------
{
	uint16 size;
	srlztn::Binaryread<uint16>(iStrm, size);
	if(size > ModSpecs::mptm.ordersMax)
	{
		// Hack: Show message here if trying to load longer sequence than what is supported.
		mpt::String str; str.Format(str_SequenceTruncationNote, size, ModSpecs::mptm.ordersMax);
		Reporting::Warning(str);
		size = ModSpecs::mptm.ordersMax;
	}
	seq.resize(max(size, MAX_ORDERS));
	if(size == 0)
		{ seq.Init(); return; }

	for(size_t i = 0; i < size; i++)
	{
		uint16 temp;
		srlztn::Binaryread<uint16>(iStrm, temp);
		seq[i] = temp;
	}
}


void WriteModSequenceOld(std::ostream& oStrm, const ModSequenceSet& seq)
//-------------------------------------------------------------------------
{
	const uint16 size = seq.GetLength();
	srlztn::Binarywrite<uint16>(oStrm, size);
	const ModSequenceSet::const_iterator endIter = seq.end();
	for(ModSequenceSet::const_iterator citer = seq.begin(); citer != endIter; citer++)
	{
		const uint16 temp = static_cast<uint16>(*citer);
		srlztn::Binarywrite<uint16>(oStrm, temp);
	}
}


void WriteModSequence(std::ostream& oStrm, const ModSequence& seq)
//----------------------------------------------------------------
{
	srlztn::Ssb ssb(oStrm);
	ssb.BeginWrite(FileIdSequence, MptVersion::num);
	ssb.WriteItem((const char*)seq.m_sName, "n");
	const uint16 nLength = seq.GetLengthTailTrimmed();
	ssb.WriteItem<uint16>(nLength, "l");
	ssb.WriteItem(seq.m_pArray, "a", 1, srlztn::ArrayWriter<uint16>(nLength));
	ssb.FinishWrite();
}


void ReadModSequence(std::istream& iStrm, ModSequence& seq, const size_t)
//-----------------------------------------------------------------------
{
	srlztn::Ssb ssb(iStrm);
	ssb.BeginRead(FileIdSequence, MptVersion::num);
	if ((ssb.m_Status & srlztn::SNT_FAILURE) != 0)
		return;
	std::string str;
	ssb.ReadItem(str, "n");
	seq.m_sName = str.c_str();
	uint16 nSize = MAX_ORDERS;
	ssb.ReadItem<uint16>(nSize, "l");
	LimitMax(nSize, ModSpecs::mptm.ordersMax);
	seq.resize(max(nSize, ModSequenceSet::s_nCacheSize));
	ssb.ReadItem(seq.m_pArray, "a", 1, srlztn::ArrayReader<uint16>(nSize));
}


void WriteModSequences(std::ostream& oStrm, const ModSequenceSet& seq)
//--------------------------------------------------------------------
{
	srlztn::Ssb ssb(oStrm);
	ssb.BeginWrite(FileIdSequences, MptVersion::num);
	const uint8 nSeqs = seq.GetNumSequences();
	const uint8 nCurrent = seq.GetCurrentSequenceIndex();
	ssb.WriteItem(nSeqs, "n");
	ssb.WriteItem(nCurrent, "c");
	for(uint8 i = 0; i < nSeqs; i++)
	{
		if (i == seq.GetCurrentSequenceIndex())
			ssb.WriteItem(seq, &i, sizeof(i), &WriteModSequence);
		else
			ssb.WriteItem(seq.m_Sequences[i], &i, sizeof(i), &WriteModSequence);
	}
	ssb.FinishWrite();
}


void ReadModSequences(std::istream& iStrm, ModSequenceSet& seq, const size_t)
//---------------------------------------------------------------------------
{
	srlztn::Ssb ssb(iStrm);
	ssb.BeginRead(FileIdSequences, MptVersion::num);
	if ((ssb.m_Status & srlztn::SNT_FAILURE) != 0)
		return;
	uint8 nSeqs;
	uint8 nCurrent;
	ssb.ReadItem(nSeqs, "n");
	if (nSeqs == 0)
		return;
	LimitMax(nSeqs, MAX_SEQUENCES);
	ssb.ReadItem(nCurrent, "c");
	if (seq.GetNumSequences() < nSeqs)
		seq.m_Sequences.resize(nSeqs, ModSequence(*seq.m_pSndFile, seq.s_nCacheSize));

	for(uint8 i = 0; i < nSeqs; i++)
		ssb.ReadItem(seq.m_Sequences[i], &i, sizeof(i), &ReadModSequence);
	seq.m_nCurrentSeq = (nCurrent < seq.GetNumSequences()) ? nCurrent : 0;
	seq.CopyStorageToCache();
}

