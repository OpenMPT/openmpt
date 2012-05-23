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

	PATTERNUNDOBUFFER sUndo;
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

	const bool bUpdate = !CanUndo(); // update undo status?

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
		sUndo.channelInfo = new PATTERNUNDOINFO;
		sUndo.channelInfo->oldNumChannels = pSndFile->GetNumChannels();
		sUndo.channelInfo->settings = new ModChannelSettings[pSndFile->GetNumChannels()];
		memcpy(sUndo.channelInfo->settings, pSndFile->ChnSettings, sizeof(ModChannelSettings) * pSndFile->GetNumChannels());
	} else
	{
		sUndo.channelInfo = nullptr;
	}

	UndoBuffer.push_back(sUndo);

	if (bUpdate) m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);
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
	const PATTERNUNDOBUFFER *pUndo = &UndoBuffer.back();

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
			pSndFile->Patterns[nPattern].Resize(nRows, false);
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
bool CPatternUndo::CanUndo()
//--------------------------
{
	return (UndoBuffer.size() > 0);
}


// Delete a given undo step.
void CPatternUndo::DeleteUndoStep(UINT nStep)
//-------------------------------------------
{
	if(nStep >= UndoBuffer.size()) return;
	if(UndoBuffer[nStep].pbuffer) delete[] UndoBuffer[nStep].pbuffer;
	if(UndoBuffer[nStep].channelInfo)
	{
		delete[] UndoBuffer[nStep].channelInfo->settings;
		delete UndoBuffer[nStep].channelInfo;
	}
	UndoBuffer.erase(UndoBuffer.begin() + nStep);
}


// Public helper function to remove the most recent undo point.
void CPatternUndo::RemoveLastUndoStep()
//-------------------------------------
{
	if(CanUndo() == false) return;
	DeleteUndoStep(UndoBuffer.size() - 1);
}


/////////////////////////////////////////////////////////////////////////////////////////
// Sample Undo Functions


// Remove all undo steps for all samples.
void CSampleUndo::ClearUndo()
//---------------------------
{
	for(SAMPLEINDEX nSmp = 1; nSmp <= MAX_SAMPLES; nSmp++)
	{
		ClearUndo(nSmp);
	}
	UndoBuffer.clear();
}


// Remove all undo steps of a given sample.
void CSampleUndo::ClearUndo(const SAMPLEINDEX nSmp)
//-------------------------------------------------
{
	if(!SampleBufferExists(nSmp, false)) return;

	while(UndoBuffer[nSmp - 1].size() > 0)
	{
		DeleteUndoStep(nSmp, 0);
	}
}


// Create undo point for given sample.
// The main program has to tell what kind of changes are going to be made to the sample.
// That way, a lot of RAM can be saved, because some actions don't even require an undo sample buffer.
bool CSampleUndo::PrepareUndo(const SAMPLEINDEX nSmp, sampleUndoTypes nChangeType, UINT nChangeStart, UINT nChangeEnd)
//--------------------------------------------------------------------------------------------------------------------
{	
	if(m_pModDoc == nullptr || !SampleBufferExists(nSmp)) return false;

	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;
	
	// Remove an undo step if there are too many.
	while(UndoBuffer[nSmp - 1].size() >= MAX_UNDO_LEVEL)
	{
		DeleteUndoStep(nSmp, 0);
	}
	
	// Restrict amount of memory that's being used
	RestrictBufferSize();

	// Create new undo slot
	SAMPLEUNDOBUFFER sUndo;

	const ModSample &oldsample = pSndFile->GetSample(nSmp);

	// Save old sample header
	MemCopy(sUndo.OldSample, oldsample);
	MemCopy(sUndo.szOldName, pSndFile->m_szNames[nSmp]);
	sUndo.nChangeType = nChangeType;

	if(nChangeType == sundo_replace)
	{
		// ensure that size information is correct here.
		nChangeStart = 0;
		nChangeEnd = oldsample.nLength;
	} else if(nChangeType == sundo_none)
	{
		// we do nothing...
		nChangeStart = nChangeEnd = 0;
	}

	sUndo.nChangeStart = nChangeStart;
	sUndo.nChangeEnd = nChangeEnd;
	sUndo.SamplePtr = nullptr;

	switch(nChangeType)
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
		{
			UINT nBytesPerSample = oldsample.GetBytesPerSample();
			UINT nChangeLen = nChangeEnd - nChangeStart;

			sUndo.SamplePtr = pSndFile->AllocateSample(nChangeLen * nBytesPerSample + 4 * nBytesPerSample);
			if(sUndo.SamplePtr == nullptr) return false;
			memcpy(sUndo.SamplePtr, oldsample.pSample + nChangeStart * nBytesPerSample, nChangeLen * nBytesPerSample);

#ifdef DEBUG
			char s[64];
			const UINT nSize = (GetUndoBufferCapacity() + nChangeLen * nBytesPerSample) >> 10;
			wsprintf(s, "Sample undo buffer size is now %u.%u MB\n", nSize >> 10, (nSize & 1023) * 100 / 1024);
			Log(s);
#endif

		}
		break;

	default:
		ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
		return false;
	}

	UndoBuffer[nSmp - 1].push_back(sUndo);

	m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);

	return true;
}


// Restore undo point for given sample
bool CSampleUndo::Undo(const SAMPLEINDEX nSmp)
//--------------------------------------------
{
	if(m_pModDoc == nullptr || !CanUndo(nSmp)) return false;

	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	// Select most recent undo slot
	SAMPLEUNDOBUFFER &undo = UndoBuffer[nSmp - 1].back();

	ModSample &sample = pSndFile->GetSample(nSmp);
	LPSTR pCurrentSample = sample.pSample;
	LPSTR pNewSample = nullptr;	// a new sample is possibly going to be allocated, depending on what's going to be undone.

	UINT nBytesPerSample = undo.OldSample.GetBytesPerSample();
	UINT nChangeLen = undo.nChangeEnd - undo.nChangeStart;

	switch(undo.nChangeType)
	{
	case sundo_none:
		break;

	case sundo_invert:
		// invert again
		ctrlSmp::InvertSample(sample, undo.nChangeStart, undo.nChangeEnd, pSndFile);
		break;

	case sundo_reverse:
		// reverse again
		ctrlSmp::ReverseSample(sample, undo.nChangeStart, undo.nChangeEnd, pSndFile);
		break;

	case sundo_unsign:
		// unsign again
		ctrlSmp::UnsignSample(sample, undo.nChangeStart, undo.nChangeEnd, pSndFile);
		break;

	case sundo_insert:
		// delete inserted data
		ASSERT(nChangeLen == sample.nLength - undo.OldSample.nLength);
		memcpy(pCurrentSample + undo.nChangeStart * nBytesPerSample, pCurrentSample + undo.nChangeEnd * nBytesPerSample, (sample.nLength - undo.nChangeEnd) * nBytesPerSample);
		// also clean the sample end
		memset(pCurrentSample + undo.OldSample.nLength * nBytesPerSample, 0, (sample.nLength - undo.OldSample.nLength) * nBytesPerSample);
		break;

	case sundo_update:
		// simply replace what has been updated.
		if(sample.nLength < undo.nChangeEnd) return false;
		memcpy(pCurrentSample + undo.nChangeStart * nBytesPerSample, undo.SamplePtr, nChangeLen * nBytesPerSample);
		break;

	case sundo_delete:
		// insert deleted data
		pNewSample = pSndFile->AllocateSample(undo.OldSample.GetSampleSizeInBytes() + 4 * nBytesPerSample);
		if(pNewSample == nullptr) return false;
		memcpy(pNewSample, pCurrentSample, undo.nChangeStart * nBytesPerSample);
		memcpy(pNewSample + undo.nChangeStart * nBytesPerSample, undo.SamplePtr, nChangeLen * nBytesPerSample);
		memcpy(pNewSample + undo.nChangeEnd * nBytesPerSample, pCurrentSample + undo.nChangeStart * nBytesPerSample, (undo.OldSample.nLength - undo.nChangeEnd) * nBytesPerSample);
		break;

	case sundo_replace:
		// simply exchange sample pointer
		pNewSample = undo.SamplePtr;
		undo.SamplePtr = nullptr; // prevent sample from being deleted
		break;

	default:
		ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
		return false;
	}

	// Restore old sample header
	sample = undo.OldSample;
	sample.pSample = pCurrentSample; // select the "correct" old sample
	MemCopy(pSndFile->m_szNames[nSmp], undo.szOldName);

	if(pNewSample != nullptr)
	{
		ctrlSmp::ReplaceSample(sample, pNewSample, undo.OldSample.nLength, pSndFile);
	}
	ctrlSmp::AdjustEndOfSample(sample, pSndFile);

	RemoveLastUndoStep(nSmp);

	if (CanUndo(nSmp) == false) m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);
	m_pModDoc->SetModified();

	return true;
}


// Check if given sample has a valid undo buffer
bool CSampleUndo::CanUndo(const SAMPLEINDEX nSmp)
//-----------------------------------------------
{
	if(!SampleBufferExists(nSmp, false) || UndoBuffer[nSmp - 1].size() == 0) return false;
	return true;
}


// Delete a given undo step of a sample.
void CSampleUndo::DeleteUndoStep(const SAMPLEINDEX nSmp, const UINT nStep)
//------------------------------------------------------------------------
{
	if(!SampleBufferExists(nSmp, false) || nStep >= UndoBuffer[nSmp - 1].size()) return;
	CSoundFile::FreeSample(UndoBuffer[nSmp - 1][nStep].SamplePtr);
	UndoBuffer[nSmp - 1].erase(UndoBuffer[nSmp - 1].begin() + nStep);
}


// Public helper function to remove the most recent undo point.
void CSampleUndo::RemoveLastUndoStep(const SAMPLEINDEX nSmp)
//----------------------------------------------------------
{
	if(CanUndo(nSmp) == false) return;
	DeleteUndoStep(nSmp, UndoBuffer[nSmp - 1].size() - 1);
}


// Restrict undo buffer size so it won't grow too large.
// This is done in FIFO style, equally distributed over all sample slots (very simple).
void CSampleUndo::RestrictBufferSize()
//------------------------------------
{
	UINT nCapacity = GetUndoBufferCapacity();
	while(nCapacity > CMainFrame::GetSettings().m_nSampleUndoMaxBuffer)
	{
		for(SAMPLEINDEX smp = 1; smp <= UndoBuffer.size(); smp++)
		{
			if(UndoBuffer[smp - 1].size() != 0 && UndoBuffer[smp - 1][0].SamplePtr != nullptr)
			{
				nCapacity -= (UndoBuffer[smp - 1][0].nChangeEnd - UndoBuffer[smp - 1][0].nChangeStart) * UndoBuffer[smp - 1][0].OldSample.GetBytesPerSample();
				DeleteUndoStep(smp, 0);
			}
			if(nCapacity <= CMainFrame::GetSettings().m_nSampleUndoMaxBuffer) return;
		}
	}
}


// Return total amount of bytes used by the sample undo buffer.
UINT CSampleUndo::GetUndoBufferCapacity()
//---------------------------------------
{
	UINT nSum = 0;
	for(size_t nSmp = 0; nSmp < UndoBuffer.size(); nSmp++)
	{
		for(size_t nStep = 0; nStep < UndoBuffer[nSmp].size(); nStep++)
		{
			if(UndoBuffer[nSmp][nStep].SamplePtr != nullptr)
			{
				nSum += (UndoBuffer[nSmp][nStep].nChangeEnd - UndoBuffer[nSmp][nStep].nChangeStart)
					* UndoBuffer[nSmp][nStep].OldSample.GetBytesPerSample();
			}
		}
	}
	return nSum;
}


// Ensure that the undo buffer is big enough for a given sample number
bool CSampleUndo::SampleBufferExists(const SAMPLEINDEX nSmp, bool bForceCreation)
//-------------------------------------------------------------------------------
{
	if(nSmp == 0 || nSmp > MAX_SAMPLES) return false;

	// Sample slot exists already
	if(nSmp <= UndoBuffer.size()) return true;

	// Sample slot doesn't exist, don't create it.
	if(bForceCreation == false) return false;

	// Sample slot doesn't exist, so create it.
	UndoBuffer.resize(nSmp);
	return true;
}