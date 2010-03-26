#ifndef MOD_SEQUENCE_H
#define MOD_SEQUENCE_H

#include <vector>

class CSoundFile;
class ModSequenceSet;

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
	ModSequence(const CSoundFile& rSf, ORDERINDEX nSize);
	ModSequence(const CSoundFile& rSf, PATTERNINDEX* pArray, ORDERINDEX nSize, ORDERINDEX nCapacity, bool bDeletableArray);

	// Initialize default sized sequence.
	void Init();

	PATTERNINDEX& operator[](const size_t i) {ASSERT(i < m_nSize); return m_pArray[i];}
	const PATTERNINDEX& operator[](const size_t i) const {ASSERT(i < m_nSize); return m_pArray[i];}

	PATTERNINDEX& Last() {ASSERT(m_nSize > 0); return m_pArray[m_nSize-1];}
	const PATTERNINDEX& Last() const {ASSERT(m_nSize > 0); return m_pArray[m_nSize-1];}

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

	void clear();
	void resize(ORDERINDEX nNewSize) {resize(nNewSize, GetInvalidPatIndex());}
	void resize(ORDERINDEX nNewSize, PATTERNINDEX nFill);
	
	// Replaces all occurences of nOld with nNew.
	void Replace(PATTERNINDEX nOld, PATTERNINDEX nNew) {std::replace(begin(), end(), nOld, nNew);}

	void OnModTypeChanged(const MODTYPE oldtype);

	ORDERINDEX size() const {return GetLength();}
	ORDERINDEX GetLength() const {return m_nSize;}

	// Returns length of sequence without counting trailing '---' items.
	ORDERINDEX GetLengthTailTrimmed() const;

	// Returns length of sequence stopping counting on first '---' (or at the end of sequence).
	ORDERINDEX GetLengthFirstEmpty() const;

	PATTERNINDEX GetInvalidPatIndex() const {return m_nInvalidIndex;} //To correspond 0xFF
	static PATTERNINDEX GetInvalidPatIndex(const MODTYPE type);

	PATTERNINDEX GetIgnoreIndex() const {return m_nIgnoreIndex;} //To correspond 0xFE
	static PATTERNINDEX GetIgnoreIndex(const MODTYPE type);

	// Returns the previous/next order ignoring skip indeces(+++).
	// If no previous/next order exists, return first/last order, and zero
	// when orderlist is empty.
	ORDERINDEX GetPreviousOrderIgnoringSkips(const ORDERINDEX start) const;
	ORDERINDEX GetNextOrderIgnoringSkips(const ORDERINDEX start) const;
	
	ModSequence& operator=(const ModSequence& seq);

	// Read/write.
	size_t WriteAsByte(FILE* f, const uint16 count);
	size_t WriteToByteArray(BYTE* dest, const UINT numOfBytes, const UINT destSize);
	bool ReadAsByte(const BYTE* pFrom, const int howMany, const int memLength);

	// Deprecated function used for MPTm's created in 1.17.02.46 - 1.17.02.48.
	DWORD Deserialize(const BYTE* const src, const DWORD memLength);

	//Returns true if the IT orderlist datafield is not sufficient to store orderlist information.
	bool NeedsExtraDatafield() const;

protected:
	iterator begin() {return m_pArray;}
	const_iterator begin() const {return m_pArray;}
	iterator end() {return m_pArray + m_nSize;}
	const_iterator end() const {return m_pArray + m_nSize;}

public:
	CString m_sName;				// Sequence name.

protected:
	PATTERNINDEX* m_pArray;			// Pointer to sequence array.
	ORDERINDEX m_nSize;				// Sequence length.
	ORDERINDEX m_nCapacity;			// Capacity in m_pArray.
	PATTERNINDEX m_nInvalidIndex;	// Invalid pat index.
	PATTERNINDEX m_nIgnoreIndex;	// Ignore pat index.
	bool m_bDeletableArray;			// True if m_pArray points the deletable(with delete[]) array.
	const CSoundFile* m_pSndFile;	// Pointer to associated CSoundFile.

	static const bool NoArrayDelete = false;
};


inline PATTERNINDEX ModSequence::GetInvalidPatIndex(const MODTYPE type) {return type == MOD_TYPE_MPT ?  uint16_max : 0xFF;}
inline PATTERNINDEX ModSequence::GetIgnoreIndex(const MODTYPE type) {return type == MOD_TYPE_MPT ? uint16_max - 1 : 0xFE;}


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
	ModSequenceSet(const CSoundFile& sndFile);

	const ModSequence& GetSequence() {return GetSequence(GetCurrentSequenceIndex());}
	const ModSequence& GetSequence(SEQUENCEINDEX nSeq);
	SEQUENCEINDEX GetNumSequences() const {return static_cast<SEQUENCEINDEX>(m_Sequences.size());}
	void SetSequence(SEQUENCEINDEX);			// Sets working sequence.
	SEQUENCEINDEX AddSequence(bool bDuplicate = true);	// Adds new sequence. If bDuplicate is true, new sequence is a duplicate of the old one. Returns the ID of the new sequence.
	void RemoveSequence() {RemoveSequence(GetCurrentSequenceIndex());}
	void RemoveSequence(SEQUENCEINDEX);		// Removes given sequence
	SEQUENCEINDEX GetCurrentSequenceIndex() const {return m_nCurrentSeq;}

	ModSequenceSet& operator=(const ModSequence& seq) {ModSequence::operator=(seq); return *this;}

	static const ORDERINDEX s_nCacheSize = MAX_ORDERS;

private:
	void CopyCacheToStorage();
	void CopyStorageToCache();

	PATTERNINDEX m_Cache[s_nCacheSize];		// Local cache array.
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


#endif

