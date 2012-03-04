/*
 * Pattern.cpp
 * -----------
 * Purpose: Module Pattern header class
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "pattern.h"
#include "patternContainer.h"
#include "../mptrack/mainfrm.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/serialization_utils.h"
#include "../mptrack/version.h"
#include "ITTools.h"

CSoundFile& CPattern::GetSoundFile() {return m_rPatternContainer.GetSoundFile();}
const CSoundFile& CPattern::GetSoundFile() const {return m_rPatternContainer.GetSoundFile();}


CHANNELINDEX CPattern::GetNumChannels() const
//-------------------------------------------
{
	return GetSoundFile().GetNumChannels();
}


bool CPattern::SetSignature(const ROWINDEX rowsPerBeat, const ROWINDEX rowsPerMeasure)
//------------------------------------------------------------------------------------
{
	if(rowsPerBeat < GetSoundFile().GetModSpecifications().patternRowsMin || rowsPerBeat > GetSoundFile().GetModSpecifications().patternRowsMax
		|| rowsPerMeasure < rowsPerBeat || rowsPerMeasure > GetSoundFile().GetModSpecifications().patternRowsMax
		/*|| rowsPerBeat > m_Rows || rowsPerMeasure > m_Rows*/)
		return false;
	m_RowsPerBeat = rowsPerBeat;
	m_RowsPerMeasure = rowsPerMeasure;
	return true;
}


bool CPattern::Resize(const ROWINDEX newRowCount, const bool showDataLossWarning)
//-------------------------------------------------------------------------------
{
	if(m_ModCommands == nullptr)
	{
		//For mimicing old behavior of setting patternsize before even having the
		//actual pattern allocated.
		m_Rows = newRowCount; 
		return false;
	}


	CSoundFile& sndFile = m_rPatternContainer.GetSoundFile();
	const CModSpecifications& specs = sndFile.GetModSpecifications();
	if(sndFile.m_pModDoc == nullptr) return true;
	CModDoc& rModDoc = *sndFile.m_pModDoc;
	if(newRowCount > specs.patternRowsMax || newRowCount < specs.patternRowsMin)
		return true;

	if (newRowCount == m_Rows) return false;
	rModDoc.BeginWaitCursor();

	CriticalSection cs;

	if (newRowCount > m_Rows)
	{
		ModCommand *p = AllocatePattern(newRowCount, sndFile.GetNumChannels());
		if (p)
		{
			memcpy(p, m_ModCommands, sndFile.GetNumChannels() * m_Rows * sizeof(ModCommand));
			FreePattern(m_ModCommands);
			m_ModCommands = p;
			m_Rows = newRowCount;
		}
	} else
	{
		bool bOk = true;

#ifdef MODPLUG_TRACKER
		if(showDataLossWarning)
		{
			// Check if any non-empty pattern cells would be lost when truncating rows at the bottom.
			const ModCommand *m = GetpModCommand(newRowCount, 0);
			for(size_t numCommands = (m_Rows - newRowCount) * sndFile.GetNumChannels(); numCommands != 0; numCommands--, m++)
			{
				if(!m->IsEmpty())
				{
					bOk = false;
					break;
				}
			}

			if (!bOk)
			{
				cs.Leave();
				rModDoc.EndWaitCursor();
				bOk = (Reporting::Confirm("Data at the end of the pattern will be lost.\nDo you want to continue?", "Shrink Pattern") == cnfYes);
				rModDoc.BeginWaitCursor();
				cs.Enter();
			}
		}
#endif // MODPLUG_TRACKER

		if (bOk)
		{
			ModCommand *pnew = AllocatePattern(newRowCount, sndFile.GetNumChannels());
			if (pnew)
			{
				memcpy(pnew, m_ModCommands, sndFile.GetNumChannels() * newRowCount * sizeof(ModCommand));
				FreePattern(m_ModCommands);
				m_ModCommands = pnew;
				m_Rows = newRowCount;
			}
		}
	}

	cs.Leave();

	rModDoc.EndWaitCursor();
	rModDoc.SetModified();

	return (newRowCount == m_Rows) ? false : true;
}


void CPattern::ClearCommands()
//----------------------------
{
	if (m_ModCommands != nullptr)
		memset(m_ModCommands, 0, GetNumRows() * GetNumChannels() * sizeof(ModCommand));
}


bool CPattern::AllocatePattern(ROWINDEX rows)
//-------------------------------------------
{
	ModCommand *m = AllocatePattern(rows, GetNumChannels());
	if(m != nullptr)
	{
		Deallocate();
		m_ModCommands = m;
		m_Rows = rows;
		m_RowsPerBeat = m_RowsPerMeasure = 0;
		return true;
	} else
	{
		return false;
	}

}

void CPattern::Deallocate()
//-------------------------
{
	// Removed critical section as it can cause problems when destroying patterns in the CSoundFile destructor.
	//BEGIN_CRITICAL();
	m_Rows = m_RowsPerBeat = m_RowsPerMeasure = 0;
	FreePattern(m_ModCommands);
	m_ModCommands = nullptr;
	m_PatternName.Empty();
	//END_CRITICAL();
}

bool CPattern::Expand()
//---------------------
{
	ModCommand *newPattern, *oldPattern;

	CSoundFile& sndFile = m_rPatternContainer.GetSoundFile();
	if(sndFile.m_pModDoc == nullptr) return true;

	CModDoc& rModDoc = *sndFile.m_pModDoc;

	if ((!m_ModCommands) || (m_Rows > sndFile.GetModSpecifications().patternRowsMax / 2)) return true;

	rModDoc.BeginWaitCursor();
	const ROWINDEX nRows = m_Rows;
	const CHANNELINDEX nChns = sndFile.m_nChannels;
	newPattern = AllocatePattern(nRows * 2, nChns);
	if (!newPattern) return true;

	const PATTERNINDEX nPattern = m_rPatternContainer.GetIndex(this);
	rModDoc.GetPatternUndo().PrepareUndo(nPattern, 0, 0, nChns, nRows);
	oldPattern = m_ModCommands;
	for (ROWINDEX y = 0; y < nRows; y++)
	{
		memcpy(newPattern + y * 2 * nChns, oldPattern + y * nChns, nChns * sizeof(ModCommand));
	}
	m_ModCommands = newPattern;
	m_Rows = nRows * 2;
	FreePattern(oldPattern); oldPattern = nullptr;
	rModDoc.SetModified();
	rModDoc.UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << HINT_SHIFT_PAT), NULL);
	rModDoc.EndWaitCursor();
	return false;
}

bool CPattern::Shrink()
//---------------------
{
	CSoundFile& sndFile = m_rPatternContainer.GetSoundFile();
	if(sndFile.m_pModDoc == NULL) return true;

	CModDoc& rModDoc = *sndFile.m_pModDoc;

	if (!m_ModCommands || m_Rows < sndFile.GetModSpecifications().patternRowsMin * 2) return true;

	rModDoc.BeginWaitCursor();
	ROWINDEX nRows = m_Rows;
	const CHANNELINDEX nChns = sndFile.m_nChannels;
	const PATTERNINDEX nPattern = m_rPatternContainer.GetIndex(this);
	rModDoc.GetPatternUndo().PrepareUndo(nPattern, 0, 0, nChns, nRows);
	nRows /= 2;
	for (ROWINDEX y = 0; y < nRows; y++)
	{
		ModCommand *psrc = sndFile.Patterns[nPattern] + (y * 2 * nChns);
		ModCommand *pdest = sndFile.Patterns[nPattern] + (y * nChns);
		for (CHANNELINDEX x = 0; x < nChns; x++)
		{
			pdest[x] = psrc[x];
			if ((!pdest[x].note) && (!pdest[x].instr))
			{
				pdest[x].note = psrc[x+nChns].note;
				pdest[x].instr = psrc[x+nChns].instr;
				if (psrc[x+nChns].volcmd)
				{
					pdest[x].volcmd = psrc[x+nChns].volcmd;
					pdest[x].vol = psrc[x+nChns].vol;
				}
				if (!pdest[x].command)
				{
					pdest[x].command = psrc[x+nChns].command;
					pdest[x].param = psrc[x+nChns].param;
				}
			}
		}
	}
	m_Rows = nRows;
	rModDoc.SetModified();
	rModDoc.UpdateAllViews(NULL, HINT_PATTERNDATA | (nPattern << HINT_SHIFT_PAT), NULL);
	rModDoc.EndWaitCursor();
	return false;
}


bool CPattern::SetName(const char *newName, size_t maxChars)
//----------------------------------------------------------
{
	if(newName == nullptr || maxChars == 0)
	{
		return false;
	}
	m_PatternName.SetString(newName, strnlen(newName, maxChars));
	return true;
}


bool CPattern::GetName(char *buffer, size_t maxChars) const
//---------------------------------------------------------
{
	if(buffer == nullptr || maxChars == 0)
	{
		return false;
	}
	strncpy(buffer, m_PatternName, maxChars - 1);
	buffer[maxChars - 1] = '\0';
	return true;
}


////////////////////////////////////////////////////////////////////////
//
//	Static allocation / deallocation methods
//
////////////////////////////////////////////////////////////////////////


ModCommand *CPattern::AllocatePattern(ROWINDEX rows, CHANNELINDEX nchns)
//----------------------------------------------------------------------
{
	try
	{
		ModCommand *p = new ModCommand[rows * nchns];
		memset(p, 0, rows * nchns * sizeof(ModCommand));
		return p;
	} catch(MPTMemoryException)
	{
		return nullptr;
	}
}


void CPattern::FreePattern(ModCommand *pat)
//-----------------------------------------
{
	if (pat) delete[] pat;
}


////////////////////////////////////////////////////////////////////////
//
//	ITP functions
//
////////////////////////////////////////////////////////////////////////


bool CPattern::WriteITPdata(FILE* f) const
//----------------------------------------
{
	for(ROWINDEX r = 0; r < GetNumRows(); r++)
	{
		for(CHANNELINDEX c = 0; c < GetNumChannels(); c++)
		{
			ModCommand mc = GetModCommand(r,c);
			fwrite(&mc, sizeof(ModCommand), 1, f);
		}
	}
    return false;
}

bool CPattern::ReadITPdata(const BYTE* const lpStream, DWORD& streamPos, const DWORD datasize, const DWORD dwMemLength)
//-----------------------------------------------------------------------------------------------
{
	if(streamPos > dwMemLength || datasize >= dwMemLength - streamPos || datasize < sizeof(MODCOMMAND_ORIGINAL))
		return true;

	const DWORD startPos = streamPos;
	size_t counter = 0;
	while(streamPos - startPos + sizeof(MODCOMMAND_ORIGINAL) <= datasize)
	{
		MODCOMMAND_ORIGINAL temp;
		memcpy(&temp, lpStream + streamPos, sizeof(MODCOMMAND_ORIGINAL));
		ModCommand& mc = GetModCommand(counter);
		mc.command = temp.command;
		mc.instr = temp.instr;
		mc.note = temp.note;
		mc.param = temp.param;
		mc.vol = temp.vol;
		mc.volcmd = temp.volcmd;
		streamPos += sizeof(MODCOMMAND_ORIGINAL);
		counter++;
	}
	if(streamPos != startPos + datasize)
	{
		ASSERT(false);
		return true;
	}
	else
		return false;
}




////////////////////////////////////////////////////////////////////////
//
//	Pattern serialization functions
//
////////////////////////////////////////////////////////////////////////


static enum maskbits
{
	noteBit			= (1 << 0),
	instrBit		= (1 << 1),
	volcmdBit		= (1 << 2),
	volBit			= (1 << 3),
	commandBit		= (1 << 4),
	effectParamBit	= (1 << 5),
	extraData		= (1 << 6)
};

void WriteData(std::ostream& oStrm, const CPattern& pat);
void ReadData(std::istream& iStrm, CPattern& pat, const size_t nSize = 0);

void WriteModPattern(std::ostream& oStrm, const CPattern& pat)
//------------------------------------------------------------
{
	srlztn::Ssb ssb(oStrm);
	ssb.BeginWrite(FileIdPattern, MptVersion::num);
	ssb.WriteItem(pat, "data", strlen("data"), &WriteData);
	// pattern time signature
	if(pat.GetOverrideSignature())
	{
		ssb.WriteItem<uint32>(pat.GetRowsPerBeat(), "RPB.");
		ssb.WriteItem<uint32>(pat.GetRowsPerMeasure(), "RPM.");
	}
	ssb.FinishWrite();
}


void ReadModPattern(std::istream& iStrm, CPattern& pat, const size_t)
//-------------------------------------------------------------------
{
	srlztn::Ssb ssb(iStrm);
	ssb.BeginRead(FileIdPattern, MptVersion::num);
	if ((ssb.m_Status & srlztn::SNT_FAILURE) != 0)
		return;
	ssb.ReadItem(pat, "data", strlen("data"), &ReadData);
	// pattern time signature
	uint32 nRPB = 0, nRPM = 0;
	ssb.ReadItem<uint32>(nRPB, "RPB.");
	ssb.ReadItem<uint32>(nRPM, "RPM.");
	pat.SetSignature(nRPB, nRPM);
}


uint8 CreateDiffMask(ModCommand chnMC, ModCommand newMC)
//------------------------------------------------------
{
	uint8 mask = 0;
	if(chnMC.note != newMC.note)
		mask |= noteBit;
	if(chnMC.instr != newMC.instr)
		mask |= instrBit;
	if(chnMC.volcmd != newMC.volcmd)
		mask |= volcmdBit;
	if(chnMC.vol != newMC.vol)
		mask |= volBit;
	if(chnMC.command != newMC.command)
		mask |= commandBit;
	if(chnMC.param != newMC.param)
		mask |= effectParamBit;
	return mask;
}

using srlztn::Binarywrite;
using srlztn::Binaryread;

// Writes pattern data. Adapted from SaveIT.
void WriteData(std::ostream& oStrm, const CPattern& pat)
//------------------------------------------------------
{
	if(!pat)
		return;

	const ROWINDEX rows = pat.GetNumRows();
	const CHANNELINDEX chns = pat.GetNumChannels();
	vector<ModCommand> lastChnMC(chns);

	for(ROWINDEX r = 0; r<rows; r++)
	{
		for(CHANNELINDEX c = 0; c<chns; c++)
		{
			const ModCommand m = *pat.GetpModCommand(r, c);
			// Writing only commands not written in IT-pattern writing:
			// For now this means only NOTE_PC and NOTE_PCS.
			if(!m.IsPcNote())
				continue;
			uint8 diffmask = CreateDiffMask(lastChnMC[c], m);
			uint8 chval = static_cast<uint8>(c+1);
			if(diffmask != 0)
				chval |= IT_bitmask_patternChanEnabled_c;

			Binarywrite<uint8>(oStrm, chval);
				
			if(diffmask)
			{
				lastChnMC[c] = m;
				Binarywrite<uint8>(oStrm, diffmask);
				if(diffmask & noteBit) Binarywrite<uint8>(oStrm, m.note);
				if(diffmask & instrBit) Binarywrite<uint8>(oStrm, m.instr);
				if(diffmask & volcmdBit) Binarywrite<uint8>(oStrm, m.volcmd);
				if(diffmask & volBit) Binarywrite<uint8>(oStrm, m.vol);
				if(diffmask & commandBit) Binarywrite<uint8>(oStrm, m.command);
				if(diffmask & effectParamBit) Binarywrite<uint8>(oStrm, m.param);
			}
		}
		Binarywrite<uint8>(oStrm, 0); // Write end of row marker.
	}
}


#define READITEM(itembit,id)		\
if(diffmask & itembit)				\
{									\
	Binaryread<uint8>(iStrm, temp);	\
	if(ch < chns)					\
		lastChnMC[ch].id = temp;	\
}									\
if(ch < chns)						\
	m.id = lastChnMC[ch].id;


void ReadData(std::istream& iStrm, CPattern& pat, const size_t)
//-------------------------------------------------------------
{
	if (!pat) // Expecting patterns to be allocated and resized properly.
		return;

	const CHANNELINDEX chns = pat.GetNumChannels();
	const ROWINDEX rows = pat.GetNumRows();

	vector<ModCommand> lastChnMC(chns);

	ROWINDEX row = 0;
	while(row < rows && iStrm.good())
	{
		uint8 t = 0;
		Binaryread<uint8>(iStrm, t);
		if(t == 0)
		{
			row++;
			continue;
		}

		CHANNELINDEX ch = (t & IT_bitmask_patternChanField_c); 
		if(ch > 0)
			ch--;

		uint8 diffmask = 0;
		if((t & IT_bitmask_patternChanEnabled_c) != 0)
			Binaryread<uint8>(iStrm, diffmask);
		uint8 temp = 0;

		ModCommand dummy;
		ModCommand& m = (ch < chns) ? *pat.GetpModCommand(row, ch) : dummy;

		READITEM(noteBit, note);
		READITEM(instrBit, instr);
		READITEM(volcmdBit, volcmd);
		READITEM(volBit, vol);
		READITEM(commandBit, command);
		READITEM(effectParamBit, param);
		if(diffmask & extraData)
		{
			//Ignore additional data.
			uint8 temp;
			Binaryread<uint8>(iStrm, temp);
			iStrm.ignore(temp);
		}
	}
}

#undef READITEM
