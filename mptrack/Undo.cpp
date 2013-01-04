/*
 * Undo.cpp
 * --------
 * Purpose: Editor undo buffer functionality.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "moddoc.h"
#include "MainFrm.h"
#include "modsmp_ctrl.h"
#include "Undo.h"

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////////////////
// Pattern Undo Functions


// Remove all undo steps.
void CPatternUndo::ClearUndo()
//----------------------------
{
	while(UndoBuffer.size() > 0)
	{
		DeleteUndoStep(0);
	}
}


// Create undo point.
//   Parameter list:
//   - pattern: Pattern of which an undo step should be created from.
//   - firstChn: first channel, 0-based.
//   - firstRow: first row, 0-based.
//   - numChns: width
//   - numRows: height
//   - linkToPrevious: Don't create a separate undo step, but link this to the previous undo event. Useful for commands that modify several patterns at once.
//   - storeChannelInfo: Also store current channel header information (pan / volume / etc. settings) and number of channels in this undo point.
bool CPatternUndo::PrepareUndo(PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, bool linkToPrevious, bool storeChannelInfo)
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(m_pModDoc == nullptr) return false;
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	UndoInfo sUndo;
	ModCommand *pUndoData, *pPattern;
	ROWINDEX nRows;

	if (!pSndFile->Patterns.IsValidPat(pattern)) return false;
	nRows = pSndFile->Patterns[pattern].GetNumRows();
	pPattern = pSndFile->Patterns[pattern];
	if ((firstRow >= nRows) || (numChns < 1) || (numRows < 1) || (firstChn >= pSndFile->GetNumChannels())) return false;
	if (firstRow + numRows >= nRows) numRows = nRows - firstRow;
	if (firstChn + numChns >= pSndFile->GetNumChannels()) numChns = pSndFile->GetNumChannels() - firstChn;

	pUndoData = CPattern::AllocatePattern(numRows, numChns);
	if (!pUndoData) return false;

	const bool updateView = !CanUndo(); // update undo status?

	// Remove an undo step if there are too many.
	while(UndoBuffer.size() >= MAX_UNDO_LEVEL)
	{
		DeleteUndoStep(0);
	}

	sUndo.pattern = pattern;
	sUndo.patternsize = pSndFile->Patterns[pattern].GetNumRows();
	sUndo.firstChannel = firstChn;
	sUndo.firstRow = firstRow;
	sUndo.numChannels = numChns;
	sUndo.numRows = numRows;
	sUndo.pbuffer = pUndoData;
	sUndo.linkToPrevious = linkToPrevious;
	pPattern += firstChn + firstRow * pSndFile->GetNumChannels();
	for(ROWINDEX iy = 0; iy < numRows; iy++)
	{
		memcpy(pUndoData, pPattern, numChns * sizeof(ModCommand));
		pUndoData += numChns;
		pPattern += pSndFile->GetNumChannels();
	}

	if(storeChannelInfo)
	{
		sUndo.channelInfo = new ChannelInfo(pSndFile->GetNumChannels());
		memcpy(sUndo.channelInfo->settings, pSndFile->ChnSettings, sizeof(ModChannelSettings) * pSndFile->GetNumChannels());
	} else
	{
		sUndo.channelInfo = nullptr;
	}

	UndoBuffer.push_back(sUndo);

	if(updateView) m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);
	return true;
}


// Restore an undo point. Returns which pattern has been modified.
PATTERNINDEX CPatternUndo::Undo()
//-------------------------------
{
	return Undo(false);
}


// Restore an undo point. Returns which pattern has been modified.
// linkedFromPrevious is true if a connected undo event is going to be deleted (can only be called internally).
PATTERNINDEX CPatternUndo::Undo(bool linkedFromPrevious)
//------------------------------------------------------
{
	if(m_pModDoc == nullptr) return PATTERNINDEX_INVALID;
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return PATTERNINDEX_INVALID;

	ModCommand *pUndoData, *pPattern;
	PATTERNINDEX nPattern;
	ROWINDEX nRows;
	bool linkToPrevious = false;

	if (CanUndo() == false) return PATTERNINDEX_INVALID;

	// If the most recent undo step is invalid, trash it.
	while(UndoBuffer.back().pattern >= pSndFile->Patterns.Size())
	{
		RemoveLastUndoStep();
		// The command which was connect to this command is no more valid, so don't search for the next command.
		if(linkedFromPrevious)
			return PATTERNINDEX_INVALID;
	}

	// Select most recent undo slot
	const UndoInfo *pUndo = &UndoBuffer.back();

	if(pUndo->channelInfo != nullptr)
	{
		if(pUndo->channelInfo->oldNumChannels > pSndFile->GetNumChannels())
		{
			// First add some channels again...
			vector<CHANNELINDEX> channels(pUndo->channelInfo->oldNumChannels, CHANNELINDEX_INVALID);
			for(CHANNELINDEX i = 0; i < pSndFile->GetNumChannels(); i++)
			{
				channels[i] = i;
			}
			m_pModDoc->ReArrangeChannels(channels, false);
		} else if(pUndo->channelInfo->oldNumChannels < pSndFile->GetNumChannels())
		{
			// ... or remove newly added channels
			vector<CHANNELINDEX> channels(pUndo->channelInfo->oldNumChannels);
			for(CHANNELINDEX i = 0; i < pUndo->channelInfo->oldNumChannels; i++)
			{
				channels[i] = i;
			}
			m_pModDoc->ReArrangeChannels(channels, false);
		}
		memcpy(pSndFile->ChnSettings, pUndo->channelInfo->settings, sizeof(ModChannelSettings) * pUndo->channelInfo->oldNumChannels);

		// Channel mute status might have changed...
		for(CHANNELINDEX i = 0; i < pSndFile->GetNumChannels(); i++)
		{
			m_pModDoc->UpdateChannelMuteStatus(i);
		}

	}

	nPattern = pUndo->pattern;
	nRows = pUndo->patternsize;
	if(pUndo->firstChannel + pUndo->numChannels <= pSndFile->GetNumChannels())
	{
		if(!pSndFile->Patterns[nPattern])
		{
			if(!pSndFile->Patterns[nPattern].AllocatePattern(nRows))
			{
				return PATTERNINDEX_INVALID;
			}
		} else if(pSndFile->Patterns[nPattern].GetNumRows() != nRows)
		{
			pSndFile->Patterns[nPattern].Resize(nRows);
		}

		linkToPrevious = pUndo->linkToPrevious;
		pUndoData = pUndo->pbuffer;
		pPattern = pSndFile->Patterns[nPattern];
		if (!pSndFile->Patterns[nPattern]) return PATTERNINDEX_INVALID;
		pPattern += pUndo->firstChannel + (pUndo->firstRow * pSndFile->GetNumChannels());
		for(ROWINDEX iy = 0; iy < pUndo->numRows; iy++)
		{
			memcpy(pPattern, pUndoData, pUndo->numChannels * sizeof(ModCommand));
			pPattern += pSndFile->GetNumChannels();
			pUndoData += pUndo->numChannels;
		}
	}		

	RemoveLastUndoStep();

	if(CanUndo() == false) m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);

	if(linkToPrevious)
	{
		nPattern = Undo(true);
	}

	return nPattern;
}


// Check if an undo buffer actually exists.
bool CPatternUndo::CanUndo() const
//--------------------------------
{
	return (UndoBuffer.size() > 0);
}


// Delete a given undo step.
void CPatternUndo::DeleteUndoStep(size_t step)
//--------------------------------------------
{
	if(step >= UndoBuffer.size()) return;
	if(UndoBuffer[step].pbuffer) delete[] UndoBuffer[step].pbuffer;
	if(UndoBuffer[step].channelInfo)
	{
		delete UndoBuffer[step].channelInfo;
	}
	UndoBuffer.erase(UndoBuffer.begin() + step);
}


// Public helper function to remove the most recent undo point.
void CPatternUndo::RemoveLastUndoStep()
//-------------------------------------
{
	if(!CanUndo()) return;
	DeleteUndoStep(UndoBuffer.size() - 1);
}


/////////////////////////////////////////////////////////////////////////////////////////
// Sample Undo Functions


// Remove all undo steps for all samples.
void CSampleUndo::ClearUndo()
//---------------------------
{
	for(SAMPLEINDEX smp = 1; smp <= MAX_SAMPLES; smp++)
	{
		ClearUndo(smp);
	}
	UndoBuffer.clear();
}


// Remove all undo steps of a given sample.
void CSampleUndo::ClearUndo(const SAMPLEINDEX smp)
//------------------------------------------------
{
	if(!SampleBufferExists(smp, false)) return;

	while(UndoBuffer[smp - 1].size() > 0)
	{
		DeleteUndoStep(smp, 0);
	}
}


// Create undo point for given sample.
// The main program has to tell what kind of changes are going to be made to the sample.
// That way, a lot of RAM can be saved, because some actions don't even require an undo sample buffer.
bool CSampleUndo::PrepareUndo(const SAMPLEINDEX smp, sampleUndoTypes changeType, SmpLength changeStart, SmpLength changeEnd)
//--------------------------------------------------------------------------------------------------------------------------
{	
	if(m_pModDoc == nullptr || !SampleBufferExists(smp)) return false;

	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;
	
	// Remove an undo step if there are too many.
	while(UndoBuffer[smp - 1].size() >= MAX_UNDO_LEVEL)
	{
		DeleteUndoStep(smp, 0);
	}
	
	// Restrict amount of memory that's being used
	RestrictBufferSize();

	// Create new undo slot
	UndoInfo undo;

	const ModSample &oldSample = pSndFile->GetSample(smp);

	// Save old sample header
	undo.OldSample = oldSample;
	MemCopy(undo.oldName, pSndFile->m_szNames[smp]);
	undo.changeType = changeType;

	if(changeType == sundo_replace)
	{
		// ensure that size information is correct here.
		changeStart = 0;
		changeEnd = oldSample.nLength;
	} else if(changeType == sundo_none)
	{
		// we do nothing...
		changeStart = changeEnd = 0;
	}

	if(changeStart > oldSample.nLength || changeStart > changeEnd)
	{
		// Something is surely screwed up.
		ASSERT(false);
		return false;
	}

	undo.changeStart = changeStart;
	undo.changeEnd = changeEnd;
	undo.samplePtr = nullptr;

	switch(changeType)
	{
	case sundo_none:	// we are done, no sample changes here.
	case sundo_invert:	// no action necessary, since those effects can be applied again to be undone.
	case sundo_reverse:	// dito
	case sundo_unsign:	// dito
	case sundo_insert:	// no action necessary, we already have stored the variables that are necessary.
		break;

	case sundo_update:
	case sundo_delete:
	case sundo_replace:
		if(oldSample.pSample != nullptr)
		{
			const size_t bytesPerSample = oldSample.GetBytesPerSample();
			const size_t changeLen = changeEnd - changeStart;

			undo.samplePtr = pSndFile->AllocateSample((changeLen + 4) * bytesPerSample);
			if(undo.samplePtr == nullptr) return false;
			memcpy(undo.samplePtr, oldSample.pSample + changeStart * bytesPerSample, changeLen * bytesPerSample);

#ifdef _DEBUG
			char s[64];
			const size_t nSize = (GetUndoBufferCapacity() + changeLen * bytesPerSample) >> 10;
			wsprintf(s, "Sample undo buffer size is now %u.%u MB\n", nSize >> 10, (nSize & 1023) * 100 / 1024);
			Log(s);
#endif

		}
		break;

	default:
		ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
		return false;
	}

	UndoBuffer[smp - 1].push_back(undo);

	m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);

	return true;
}


// Restore undo point for given sample
bool CSampleUndo::Undo(const SAMPLEINDEX smp)
//-------------------------------------------
{
	if(m_pModDoc == nullptr || !CanUndo(smp)) return false;

	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	// Select most recent undo slot
	UndoInfo &undo = UndoBuffer[smp - 1].back();

	ModSample &sample = pSndFile->GetSample(smp);
	LPSTR pCurrentSample = sample.pSample;
	LPSTR pNewSample = nullptr;	// a new sample is possibly going to be allocated, depending on what's going to be undone.
	bool replace = false;

	uint8 bytesPerSample = undo.OldSample.GetBytesPerSample();
	SmpLength changeLen = undo.changeEnd - undo.changeStart;

	switch(undo.changeType)
	{
	case sundo_none:
		break;

	case sundo_invert:
		// invert again
		ctrlSmp::InvertSample(sample, undo.changeStart, undo.changeEnd, pSndFile);
		break;

	case sundo_reverse:
		// reverse again
		ctrlSmp::ReverseSample(sample, undo.changeStart, undo.changeEnd, pSndFile);
		break;

	case sundo_unsign:
		// unsign again
		ctrlSmp::UnsignSample(sample, undo.changeStart, undo.changeEnd, pSndFile);
		break;

	case sundo_insert:
		// delete inserted data
		ASSERT(changeLen == sample.nLength - undo.OldSample.nLength);
		memcpy(pCurrentSample + undo.changeStart * bytesPerSample, pCurrentSample + undo.changeEnd * bytesPerSample, (sample.nLength - undo.changeEnd) * bytesPerSample);
		// also clean the sample end
		memset(pCurrentSample + undo.OldSample.nLength * bytesPerSample, 0, (sample.nLength - undo.OldSample.nLength) * bytesPerSample);
		break;

	case sundo_update:
		// simply replace what has been updated.
		if(sample.nLength < undo.changeEnd) return false;
		memcpy(pCurrentSample + undo.changeStart * bytesPerSample, undo.samplePtr, changeLen * bytesPerSample);
		break;

	case sundo_delete:
		// insert deleted data
		pNewSample = pSndFile->AllocateSample(undo.OldSample.GetSampleSizeInBytes() + 4 * bytesPerSample);
		if(pNewSample == nullptr) return false;
		replace = true;
		memcpy(pNewSample, pCurrentSample, undo.changeStart * bytesPerSample);
		memcpy(pNewSample + undo.changeStart * bytesPerSample, undo.samplePtr, changeLen * bytesPerSample);
		memcpy(pNewSample + undo.changeEnd * bytesPerSample, pCurrentSample + undo.changeStart * bytesPerSample, (undo.OldSample.nLength - undo.changeEnd) * bytesPerSample);
		break;

	case sundo_replace:
		// simply exchange sample pointer
		pNewSample = undo.samplePtr;
		undo.samplePtr = nullptr; // prevent sample from being deleted
		replace = true;
		break;

	default:
		ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
		return false;
	}

	// Restore old sample header
	sample = undo.OldSample;
	sample.pSample = pCurrentSample; // select the "correct" old sample
	MemCopy(pSndFile->m_szNames[smp], undo.oldName);

	if(replace)
	{
		ctrlSmp::ReplaceSample(sample, pNewSample, undo.OldSample.nLength, pSndFile);
	}
	ctrlSmp::AdjustEndOfSample(sample, pSndFile);

	RemoveLastUndoStep(smp);

	if(CanUndo(smp) == false) m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);
	m_pModDoc->SetModified();

	return true;
}


// Check if given sample has a valid undo buffer
bool CSampleUndo::CanUndo(const SAMPLEINDEX smp)
//----------------------------------------------
{
	if(!SampleBufferExists(smp, false) || UndoBuffer[smp - 1].size() == 0) return false;
	return true;
}


// Delete a given undo step of a sample.
void CSampleUndo::DeleteUndoStep(const SAMPLEINDEX smp, const size_t step)
//------------------------------------------------------------------------
{
	if(!SampleBufferExists(smp, false) || step >= UndoBuffer[smp - 1].size()) return;
	CSoundFile::FreeSample(UndoBuffer[smp - 1][step].samplePtr);
	UndoBuffer[smp - 1].erase(UndoBuffer[smp - 1].begin() + step);
}


// Public helper function to remove the most recent undo point.
void CSampleUndo::RemoveLastUndoStep(const SAMPLEINDEX smp)
//---------------------------------------------------------
{
	if(CanUndo(smp) == false) return;
	DeleteUndoStep(smp, UndoBuffer[smp - 1].size() - 1);
}


// Restrict undo buffer size so it won't grow too large.
// This is done in FIFO style, equally distributed over all sample slots (very simple).
void CSampleUndo::RestrictBufferSize()
//------------------------------------
{
	size_t capacity = GetUndoBufferCapacity();
	while(capacity > CMainFrame::GetSettings().m_nSampleUndoMaxBuffer)
	{
		for(SAMPLEINDEX smp = 1; smp <= UndoBuffer.size(); smp++)
		{
			for(size_t i = 0; i < UndoBuffer[smp - 1].size(); i++)
			{
				if(UndoBuffer[smp - 1][i].samplePtr != nullptr)
				{
					capacity -= (UndoBuffer[smp - 1][i].changeEnd - UndoBuffer[smp - 1][i].changeStart) * UndoBuffer[smp - 1][i].OldSample.GetBytesPerSample();
					for(size_t j = 0; j <= i; j++)
					{
						DeleteUndoStep(smp, j);
					}
					// Try to evenly spread out the restriction, i.e. move on to other samples before deleting another step for this sample.
					break;
				}
			}
			if(capacity <= CMainFrame::GetSettings().m_nSampleUndoMaxBuffer) return;
		}
	}
}


// Return total amount of bytes used by the sample undo buffer.
size_t CSampleUndo::GetUndoBufferCapacity()
//-----------------------------------------
{
	size_t sum = 0;
	for(size_t smp = 0; smp < UndoBuffer.size(); smp++)
	{
		for(size_t nStep = 0; nStep < UndoBuffer[smp].size(); nStep++)
		{
			if(UndoBuffer[smp][nStep].samplePtr != nullptr)
			{
				sum += (UndoBuffer[smp][nStep].changeEnd - UndoBuffer[smp][nStep].changeStart)
					* UndoBuffer[smp][nStep].OldSample.GetBytesPerSample();
			}
		}
	}
	return sum;
}


// Ensure that the undo buffer is big enough for a given sample number
bool CSampleUndo::SampleBufferExists(const SAMPLEINDEX smp, bool forceCreation)
//-----------------------------------------------------------------------------
{
	if(smp == 0 || smp > MAX_SAMPLES) return false;

	// Sample slot exists already
	if(smp <= UndoBuffer.size()) return true;

	// Sample slot doesn't exist, don't create it.
	if(forceCreation == false) return false;

	// Sample slot doesn't exist, so create it.
	UndoBuffer.resize(smp);
	return true;
}
