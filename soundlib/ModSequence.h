/*
 * ModSequence.h
 * -------------
 * Purpose: Order and sequence handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <algorithm>
#include <vector>

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
class ModSequenceSet;
class FileReader;

class ModSequence
//===============
{
public:
	friend class ModSequenceSet;
	typedef PATTERNINDEX* iterator;
	typedef const PATTERNINDEX* const_iterator;
	
	friend void WriteModSequence(std::ostream& oStrm, const ModSequence& seq);
	friend void ReadModSequence(std::istream& iStrm, ModSequence& seq, const size_t);

	virtual ~ModSequence() {if (m_bDeletableArray) delete[] m_pArray;}
	ModSequence(const ModSequence&);
	ModSequence(CSoundFile& rSf, ORDERINDEX nSize);
	ModSequence(CSoundFile& rSf, PATTERNINDEX* pArray, ORDERINDEX nSize, ORDERINDEX nCapacity, ORDERINDEX restartPos, bool bDeletableArray);

	// Initialize default sized sequence.
	void Init();

	PATTERNINDEX& operator[](const size_t i) {MPT_ASSERT(i < m_nSize); return m_pArray[i];}
	const PATTERNINDEX& operator[](const size_t i) const {MPT_ASSERT(i < m_nSize); return m_pArray[i];}
	PATTERNINDEX& At(const size_t i) {return (*this)[i];}
	const PATTERNINDEX& At(const size_t i) const {return (*this)[i];}

	PATTERNINDEX& Last() {MPT_ASSERT(m_nSize > 0); return m_pArray[m_nSize-1];}
	const PATTERNINDEX& Last() const {MPT_ASSERT(m_nSize > 0); return m_pArray[m_nSize-1];}

	// Returns last accessible index, i.e. GetLength() - 1. Behaviour is undefined if length is zero.
	ORDERINDEX GetLastIndex() const {return m_nSize - 1;}

	void Append() {Append(GetInvalidPatIndex());}	// Appends InvalidPatIndex.
	void Append(PATTERNINDEX nPat);					// Appends given patindex.

	// Inserts nCount orders starting from nPos using nFill as the pattern index for all inserted orders.
	// Sequence will automatically grow if needed and if it can't grow enough, some tail 
	// orders will be discarded.
	// Return: Number of orders inserted.
	ORDERINDEX Insert(ORDERINDEX nPos, ORDERINDEX nCount) {return Insert(nPos, nCount, GetInvalidPatIndex());}
	ORDERINDEX Insert(ORDERINDEX nPos, ORDERINDEX nCount, PATTERNINDEX nFill);

	// Removes orders from range [nPosBegin, nPosEnd].
	void Remove(ORDERINDEX nPosBegin, ORDERINDEX nPosEnd);

	// Remove all references to a given pattern index from the order list. Jump commands are updated accordingly.
	void RemovePattern(PATTERNINDEX which);

	// Check if pattern at sequence position ord is valid.
	bool IsValidPat(ORDERINDEX ord);

	void clear();
	void resize(ORDERINDEX nNewSize) {resize(nNewSize, GetInvalidPatIndex());}
	void resize(ORDERINDEX nNewSize, PATTERNINDEX nFill);
	
	// Replaces all occurences of nOld with nNew.
	void Replace(PATTERNINDEX nOld, PATTERNINDEX nNew) {if (nOld != nNew) std::replace(begin(), end(), nOld, nNew);}

	void AdjustToNewModType(const MODTYPE oldtype);

	ORDERINDEX size() const {return GetLength();}
	ORDERINDEX GetLength() const {return m_nSize;}

	// Returns length of sequence without counting trailing '---' items.
	ORDERINDEX GetLengthTailTrimmed() const;

	// Returns length of sequence stopping counting on first '---' (or at the end of sequence).
	ORDERINDEX GetLengthFirstEmpty() const;

	// Returns the internal representation of a stop '---' index
	static PATTERNINDEX GetInvalidPatIndex() { return uint16_max; }
	// Returns the internal representation of an ignore '+++' index
	static PATTERNINDEX GetIgnoreIndex() {return uint16_max - 1; }

	// Returns the previous/next order ignoring skip indices(+++).
	// If no previous/next order exists, return first/last order, and zero
	// when orderlist is empty.
	ORDERINDEX GetPreviousOrderIgnoringSkips(const ORDERINDEX start) const;
	ORDERINDEX GetNextOrderIgnoringSkips(const ORDERINDEX start) const;

	// Find an order item that contains a given pattern number.
	ORDERINDEX FindOrder(PATTERNINDEX nPat, ORDERINDEX startFromOrder = 0, bool searchForward = true) const;

	ModSequence& operator=(const ModSequence& seq);

	// Write order items as bytes. '---' is written as stopIndex, '+++' is written as ignoreIndex
	size_t WriteAsByte(FILE* f, const ORDERINDEX count, uint8 stopIndex = 0xFF, uint8 ignoreIndex = 0xFE) const;
	// Read order items as bytes from a file. 'howMany' bytes are read from the file, optionally only the first 'readEntries' of them are actually processed.
	// 'stopIndex' is treated as '---', 'ignoreIndex' is treated as '+++'. If the format doesn't support such indices, just pass a parameter that does not fit into a byte.
	bool ReadAsByte(FileReader &file, size_t howMany, size_t readEntries = ORDERINDEX_MAX, uint16 stopIndex = ORDERINDEX_INVALID, uint16 ignoreIndex = ORDERINDEX_INVALID);
	template<typename T, size_t arraySize>
	// Read order items from an array. Only the first 'howMany' entries are actually processed.
	// 'stopIndex' is treated as '---', 'ignoreIndex' is treated as '+++'. If the format doesn't support such indices, just pass ORDERINDEX_INVALID.
	bool ReadFromArray(const T (&orders)[arraySize], size_t howMany = arraySize, uint16 stopIndex = ORDERINDEX_INVALID, uint16 ignoreIndex = ORDERINDEX_INVALID);

	// Deprecated function used for MPTm files created with OpenMPT 1.17.02.46 - 1.17.02.48.
	bool Deserialize(FileReader &file);

	// Returns true if the IT orderlist datafield is not sufficient to store orderlist information.
	bool NeedsExtraDatafield() const;

#ifdef MODPLUG_TRACKER
	// Check if a playback position is currently locked (inaccessible)
	bool IsPositionLocked(ORDERINDEX position) const;
#endif // MODPLUG_TRACKER

	// Sequence name setter / getter
	inline void SetName(const std::string &newName) { m_sName = newName;}
	inline std::string GetName() const { return m_sName; }

	// Restart position setter / getter
	inline void SetRestartPos(ORDERINDEX restartPos) { m_restartPos = restartPos; }
	inline ORDERINDEX GetRestartPos() const { return m_restartPos; }


protected:
	iterator begin() {return m_pArray;}
	const_iterator begin() const {return m_pArray;}
	iterator end() {return m_pArray + m_nSize;}
	const_iterator end() const {return m_pArray + m_nSize;}

protected:
	std::string m_sName;			// Sequence name.

	CSoundFile &m_sndFile;			// Pointer to associated CSoundFile.
	PATTERNINDEX *m_pArray;			// Pointer to sequence array.
	ORDERINDEX m_nSize;				// Sequence length.
	ORDERINDEX m_nCapacity;			// Capacity in m_pArray.
	ORDERINDEX m_restartPos;		// Restart position when playback of this order ended
	bool m_bDeletableArray;			// True if m_pArray points the deletable(with delete[]) array.
};


template<typename T, size_t arraySize>
bool ModSequence::ReadFromArray(const T (&orders)[arraySize], size_t howMany, uint16 stopIndex, uint16 ignoreIndex)
//-----------------------------------------------------------------------------------------------------------------
{
	STATIC_ASSERT(arraySize < ORDERINDEX_MAX);
	LimitMax(howMany, arraySize);
	ORDERINDEX readEntries = static_cast<ORDERINDEX>(howMany);

	if(GetLength() < readEntries)
	{
		resize(readEntries);
	}

	for(int i = 0; i < readEntries; i++)
	{
		PATTERNINDEX pat = static_cast<PATTERNINDEX>(orders[i]);
		if(pat == stopIndex) pat = GetInvalidPatIndex();
		if(pat == ignoreIndex) pat = GetIgnoreIndex();
		(*this)[i] = pat;
	}
	return true;
}

//=======================================
class ModSequenceSet : public ModSequence
//=======================================
{
	friend void WriteModSequenceOld(std::ostream& oStrm, const ModSequenceSet& seq);
	friend void ReadModSequenceOld(std::istream& iStrm, ModSequenceSet& seq, const size_t);
	friend void WriteModSequences(std::ostream& oStrm, const ModSequenceSet& seq);
	friend void ReadModSequences(std::istream& iStrm, ModSequenceSet& seq, const size_t);
	friend void ReadModSequence(std::istream& iStrm, ModSequence& seq, const size_t);

public:
	ModSequenceSet(CSoundFile& sndFile);

	const ModSequence& GetSequence() {return GetSequence(GetCurrentSequenceIndex());}
	const ModSequence& GetSequence(SEQUENCEINDEX nSeq) const;
	ModSequence& GetSequence(SEQUENCEINDEX nSeq);
	SEQUENCEINDEX GetNumSequences() const {return static_cast<SEQUENCEINDEX>(m_Sequences.size());}
	void SetSequence(SEQUENCEINDEX);			// Sets working sequence.
	SEQUENCEINDEX AddSequence(bool bDuplicate = true);	// Adds new sequence. If bDuplicate is true, new sequence is a duplicate of the old one. Returns the ID of the new sequence.
	void RemoveSequence() {RemoveSequence(GetCurrentSequenceIndex());}
	void RemoveSequence(SEQUENCEINDEX);		// Removes given sequence
	SEQUENCEINDEX GetCurrentSequenceIndex() const {return m_nCurrentSeq;}

	ModSequenceSet& operator=(const ModSequence& seq) {ModSequence::operator=(seq); return *this;}

#ifdef MODPLUG_TRACKER
	// Adjust sequence when converting between module formats
	void OnModTypeChanged(const MODTYPE oldtype);
	// If there are subsongs (separated by "---" or "+++" patterns) in the module,
	// asks user whether to convert these into multiple sequences (given that the 
	// modformat supports multiple sequences).
	// Returns true if sequences were modified, false otherwise.
	bool ConvertSubsongsToMultipleSequences();

	// Convert the sequence's restart position information to a pattern command.
	bool RestartPosToPattern(SEQUENCEINDEX seq);
	// Merges multiple sequences into one and destroys all other sequences.
	// Returns false if there were no sequences to merge, true otherwise.
	bool MergeSequences();
#endif // MODPLUG_TRACKER

	static const ORDERINDEX s_nCacheSize;

private:
	void CopyCacheToStorage();
	void CopyStorageToCache();

	// Array size should be s_nCacheSize.
	// s_nCacheSize is defined out of line, because taking references to
	// static const members is problematic for some clang and GCC versions
	// otherwise.
	// Defining s_nCacheSize out of line makes it unavailable for declaring
	// an array.
	PATTERNINDEX m_Cache[MAX_ORDERS];		// Local cache array.
	std::vector<ModSequence> m_Sequences;	// Array of sequences.
	SEQUENCEINDEX m_nCurrentSeq;			// Index of current sequence.
};


const char FileIdSequences[] = "mptSeqC";
const char FileIdSequence[] = "mptSeq";

void WriteModSequences(std::ostream& oStrm, const ModSequenceSet& seq);
void ReadModSequences(std::istream& iStrm, ModSequenceSet& seq, const size_t nSize = 0);

void WriteModSequence(std::ostream& oStrm, const ModSequence& seq);
void ReadModSequence(std::istream& iStrm, ModSequence& seq, const size_t);

void WriteModSequenceOld(std::ostream& oStrm, const ModSequenceSet& seq);
void ReadModSequenceOld(std::istream& iStrm, ModSequenceSet& seq, const size_t);


OPENMPT_NAMESPACE_END
