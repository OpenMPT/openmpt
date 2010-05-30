/*
 * Undo.cpp
 * --------
 * Purpose: Undo functions for pattern and sample editor
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 */


#include "stdafx.h"
#include "moddoc.h"
#include "MainFrm.h"
#include "modsmp_ctrl.h"
#include "Undo.h"


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
//   - x: first channel
//   - y: first row
//   - cx: width
//   - cy: height
//   - linkToPrevious: Don't create a separate undo step, but link this to the previous undo event.
bool CPatternUndo::PrepareUndo(PATTERNINDEX pattern, UINT x, UINT y, UINT cx, UINT cy, bool linkToPrevious)
//---------------------------------------------------------------------------------------------------------
{
	if(m_pModDoc == nullptr) return false;
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	PATTERNUNDOBUFFER sUndo;
	MODCOMMAND *pUndoData, *pPattern;
	UINT nRows;

	if (!pSndFile->Patterns.IsValidPat(pattern)) return false;
	nRows = pSndFile->Patterns[pattern].GetNumRows();
	pPattern = pSndFile->Patterns[pattern];
	if ((y >= nRows) || (cx < 1) || (cy < 1) || (x >= pSndFile->m_nChannels)) return false;
	if (y + cy >= nRows) cy = nRows - y;
	if (x + cx >= pSndFile->m_nChannels) cx = pSndFile->m_nChannels - x;

	pUndoData = new MODCOMMAND[cx * cy];
	if (!pUndoData) return false;

	const bool bUpdate = !CanUndo(); // update undo status?

	// Remove an undo step if there are too many.
	while(UndoBuffer.size() >= MAX_UNDO_LEVEL)
	{
		DeleteUndoStep(0);
	}

	sUndo.pattern = pattern;
	sUndo.patternsize = pSndFile->Patterns[pattern].GetNumRows();
	sUndo.column = x;
	sUndo.row = y;
	sUndo.cx = cx;
	sUndo.cy = cy;
	sUndo.pbuffer = pUndoData;
	sUndo.linkToPrevious = linkToPrevious;
	pPattern += x + y * pSndFile->m_nChannels;
	for (UINT iy = 0; iy < cy; iy++)
	{
		memcpy(pUndoData, pPattern, cx * sizeof(MODCOMMAND));
		pUndoData += cx;
		pPattern += pSndFile->m_nChannels;
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
// linkedFromPrevious is true if a connected undo even is going to be deleted.
PATTERNINDEX CPatternUndo::Undo(bool linkedFromPrevious)
//------------------------------------------------------
{
	if(m_pModDoc == nullptr) return PATTERNINDEX_INVALID;
	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return PATTERNINDEX_INVALID;

	MODCOMMAND *pUndoData, *pPattern;
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

	nPattern = pUndo->pattern;
	nRows = pUndo->patternsize;
	if(pUndo->column + pUndo->cx <= pSndFile->m_nChannels)
	{
		if((!pSndFile->Patterns[nPattern]) || (pSndFile->Patterns[nPattern].GetNumRows() < nRows))
		{
			MODCOMMAND *newPattern = CSoundFile::AllocatePattern(nRows, pSndFile->m_nChannels);
			MODCOMMAND *oldPattern = pSndFile->Patterns[nPattern];
			if (!newPattern) return PATTERNINDEX_INVALID;
			const ROWINDEX nOldRowCount = pSndFile->Patterns[nPattern].GetNumRows();
			pSndFile->Patterns[nPattern].SetData(newPattern, nRows);
			if(oldPattern)
			{
				memcpy(newPattern, oldPattern, pSndFile->m_nChannels * nOldRowCount * sizeof(MODCOMMAND));
				CSoundFile::FreePattern(oldPattern);
			}
		}
		linkToPrevious = pUndo->linkToPrevious;
		pUndoData = pUndo->pbuffer;
		pPattern = pSndFile->Patterns[nPattern];
		if (!pSndFile->Patterns[nPattern]) return PATTERNINDEX_INVALID;
		pPattern += pUndo->column + (pUndo->row * pSndFile->m_nChannels);
		for(UINT iy = 0; iy < pUndo->cy; iy++)
		{
			memcpy(pPattern, pUndoData, pUndo->cx * sizeof(MODCOMMAND));
			pPattern += pSndFile->m_nChannels;
			pUndoData += pUndo->cx;
		}
	}		

	RemoveLastUndoStep();

	if (CanUndo() == false) m_pModDoc->UpdateAllViews(NULL, HINT_UNDO);
	if(linkToPrevious)
	{
		return Undo(true);		
	} else
	{
		return nPattern;
	}
}


// Check if an undo buffer actually exists.
bool CPatternUndo::CanUndo()
//--------------------------
{
	return (UndoBuffer.size() > 0) ? true : false;
}


// Delete a given undo step.
void CPatternUndo::DeleteUndoStep(UINT nStep)
//-------------------------------------------
{
	if(nStep >= UndoBuffer.size()) return;
	if (UndoBuffer[nStep].pbuffer) delete[] UndoBuffer[nStep].pbuffer;
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

	// Save old sample header
	memcpy(&sUndo.OldSample, &pSndFile->Samples[nSmp], sizeof(MODSAMPLE));
	sUndo.nChangeType = nChangeType;

	if(nChangeType == sundo_replace)
	{
		// ensure that size information is correct here.
		nChangeStart = 0;
		nChangeEnd = pSndFile->Samples[nSmp].nLength;
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
			UINT nBytesPerSample = pSndFile->Samples[nSmp].GetBytesPerSample();
			UINT nChangeLen = nChangeEnd - nChangeStart;

			sUndo.SamplePtr = pSndFile->AllocateSample(nChangeLen * nBytesPerSample + 4 * nBytesPerSample);
			if(sUndo.SamplePtr == nullptr) return false;
			memcpy(sUndo.SamplePtr, pSndFile->Samples[nSmp].pSample + nChangeStart * nBytesPerSample, nChangeLen * nBytesPerSample);

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
	if(m_pModDoc == nullptr || CanUndo(nSmp) == false) return false;

	CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
	if(pSndFile == nullptr) return false;

	// Select most recent undo slot
	SAMPLEUNDOBUFFER *pUndo = &UndoBuffer[nSmp - 1].back();

	LPSTR pCurrentSample = pSndFile->Samples[nSmp].pSample;
	LPSTR pNewSample = nullptr;	// a new sample is possibly going to be allocated, depending on what's going to be undone.

	UINT nBytesPerSample = pUndo->OldSample.GetBytesPerSample();
	UINT nChangeLen = pUndo->nChangeEnd - pUndo->nChangeStart;

	switch(pUndo->nChangeType)
	{
	case sundo_none:
		break;

	case sundo_invert:
		// invert again
		ctrlSmp::InvertSample(&pSndFile->Samples[nSmp], pUndo->nChangeStart, pUndo->nChangeEnd, pSndFile);
		break;

	case sundo_reverse:
		// reverse again
		ctrlSmp::ReverseSample(&pSndFile->Samples[nSmp], pUndo->nChangeStart, pUndo->nChangeEnd, pSndFile);
		break;

	case sundo_unsign:
		// unsign again
		ctrlSmp::UnsignSample(&pSndFile->Samples[nSmp], pUndo->nChangeStart, pUndo->nChangeEnd, pSndFile);
		break;

	case sundo_insert:
		// delete inserted data
		ASSERT(nChangeLen == pSndFile->Samples[nSmp].nLength - pUndo->OldSample.nLength);
		memcpy(pCurrentSample + pUndo->nChangeStart * nBytesPerSample, pCurrentSample + pUndo->nChangeEnd * nBytesPerSample, (pSndFile->Samples[nSmp].nLength - pUndo->nChangeEnd) * nBytesPerSample);
		// also clean the sample end
		memset(pCurrentSample + pUndo->OldSample.nLength * nBytesPerSample, 0, (pSndFile->Samples[nSmp].nLength - pUndo->OldSample.nLength) * nBytesPerSample);
		break;

	case sundo_update:
		// simply replace what has been updated.
		if(pSndFile->Samples[nSmp].nLength < pUndo->nChangeEnd) return false;
		memcpy(pCurrentSample + pUndo->nChangeStart * nBytesPerSample, pUndo->SamplePtr, nChangeLen * nBytesPerSample);
		break;

	case sundo_delete:
		// insert deleted data
		pNewSample = pSndFile->AllocateSample(pUndo->OldSample.GetSampleSizeInBytes() + 4 * nBytesPerSample);
		if(pNewSample == nullptr) return false;
		memcpy(pNewSample, pCurrentSample, pUndo->nChangeStart * nBytesPerSample);
		memcpy(pNewSample + pUndo->nChangeStart * nBytesPerSample, pUndo->SamplePtr, nChangeLen * nBytesPerSample);
		memcpy(pNewSample + pUndo->nChangeEnd * nBytesPerSample, pCurrentSample + pUndo->nChangeStart * nBytesPerSample, (pUndo->OldSample.nLength - pUndo->nChangeEnd) * nBytesPerSample);
		break;

	case sundo_replace:
		// simply exchange sample pointer
		pNewSample = pUndo->SamplePtr;
		pUndo->SamplePtr = nullptr; // prevent sample from being deleted
		break;

	default:
		ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
		return false;
	}

	// Restore old sample header
	memcpy(&pSndFile->Samples[nSmp], &pUndo->OldSample, sizeof(MODSAMPLE));
	pSndFile->Samples[nSmp].pSample = pCurrentSample; // select the "correct" old sample

	if(pNewSample != nullptr)
	{
		ctrlSmp::ReplaceSample(pSndFile->Samples[nSmp], pNewSample, pUndo->OldSample.nLength, pSndFile);
	}
	ctrlSmp::AdjustEndOfSample(pSndFile->Samples[nSmp], pSndFile);

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
	while(nCapacity > CMainFrame::m_nSampleUndoMaxBuffer)
	{
		for(SAMPLEINDEX nSmp = 1; nSmp <= UndoBuffer.size(); nSmp++)
		{
			if(UndoBuffer[nSmp - 1][0].SamplePtr != nullptr)
			{
				nCapacity -= (UndoBuffer[nSmp - 1][0].nChangeEnd - UndoBuffer[nSmp - 1][0].nChangeStart) * UndoBuffer[nSmp - 1][0].OldSample.GetBytesPerSample();
				DeleteUndoStep(nSmp, 0);
			}
			if(nCapacity <= CMainFrame::m_nSampleUndoMaxBuffer) return;
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