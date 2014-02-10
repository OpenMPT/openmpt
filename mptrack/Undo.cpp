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
#include "../common/StringFixer.h"

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////////////////
// Pattern Undo Functions


// Remove all undo steps.
void CPatternUndo::ClearUndo()
//----------------------------
{
	ClearBuffer(UndoBuffer);
	ClearBuffer(RedoBuffer);
}


bool CPatternUndo::PrepareUndo(PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, const char *description, bool linkToPrevious, bool storeChannelInfo)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(PrepareUndo(UndoBuffer, pattern, firstChn, firstRow, numChns, numRows, description, linkToPrevious, storeChannelInfo))
	{
		ClearBuffer(RedoBuffer);
		return true;
	}
	return false;
}


// Create undo point.
//   Parameter list:
//   - pattern: Pattern of which an undo step should be created from.
//   - firstChn: first channel, 0-based.
//   - firstRow: first row, 0-based.
//   - numChns: width
//   - numRows: height
//   - description: Short description text of action for undo menu.
//   - linkToPrevious: Don't create a separate undo step, but link this to the previous undo event. Use this for commands that modify several patterns at once.
//   - storeChannelInfo: Also store current channel header information (pan / volume / etc. settings) and number of channels in this undo point.
bool CPatternUndo::PrepareUndo(undobuf_t &buffer, PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, const char *description, bool linkToPrevious, bool storeChannelInfo)
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	const CSoundFile &sndFile = modDoc.GetrSoundFile();

	if (!sndFile.Patterns.IsValidPat(pattern)) return false;
	ROWINDEX nRows = sndFile.Patterns[pattern].GetNumRows();
	if ((firstRow >= nRows) || (numChns < 1) || (numRows < 1) || (firstChn >= sndFile.GetNumChannels())) return false;
	if (firstRow + numRows >= nRows) numRows = nRows - firstRow;
	if (firstChn + numChns >= sndFile.GetNumChannels()) numChns = sndFile.GetNumChannels() - firstChn;

	ModCommand *pUndoData = CPattern::AllocatePattern(numRows, numChns);
	if (!pUndoData) return false;

	// Remove an undo step if there are too many.
	while(buffer.size() >= MAX_UNDO_LEVEL)
	{
		DeleteUndoStep(buffer, 0);
	}

	UndoInfo sUndo;
	sUndo.pattern = pattern;
	sUndo.numPatternRows = sndFile.Patterns[pattern].GetNumRows();
	sUndo.firstChannel = firstChn;
	sUndo.firstRow = firstRow;
	sUndo.numChannels = numChns;
	sUndo.numRows = numRows;
	sUndo.pbuffer = pUndoData;
	sUndo.linkToPrevious = linkToPrevious;
	sUndo.description = description;
	const ModCommand *pPattern = sndFile.Patterns[pattern].GetpModCommand(firstRow, firstChn);
	for(ROWINDEX iy = 0; iy < numRows; iy++)
	{
		memcpy(pUndoData, pPattern, numChns * sizeof(ModCommand));
		pUndoData += numChns;
		pPattern += sndFile.GetNumChannels();
	}

	if(storeChannelInfo)
	{
		sUndo.channelInfo = new UndoInfo::ChannelInfo(sndFile.GetNumChannels());
		memcpy(sUndo.channelInfo->settings, sndFile.ChnSettings, sizeof(ModChannelSettings) * sndFile.GetNumChannels());
	} else
	{
		sUndo.channelInfo = nullptr;
	}

	buffer.push_back(sUndo);

	modDoc.UpdateAllViews(NULL, HINT_UNDO);
	return true;
}


// Restore an undo point. Returns which pattern has been modified.
PATTERNINDEX CPatternUndo::Undo()
//-------------------------------
{
	return Undo(UndoBuffer, RedoBuffer, false);
}


// Restore an undo point. Returns which pattern has been modified.
PATTERNINDEX CPatternUndo::Redo()
//-------------------------------
{
	return Undo(RedoBuffer, UndoBuffer, false);
}


// Restore an undo point. Returns which pattern has been modified.
// linkedFromPrevious is true if a connected undo event is going to be deleted (can only be called internally).
PATTERNINDEX CPatternUndo::Undo(undobuf_t &fromBuf, undobuf_t &toBuf, bool linkedFromPrevious)
//--------------------------------------------------------------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();

	bool linkToPrevious = false;

	if(fromBuf.empty()) return PATTERNINDEX_INVALID;

	// Select most recent undo slot
	const UndoInfo &undo = fromBuf.back();

	PrepareUndo(toBuf, undo.pattern, undo.firstChannel, undo.firstRow, undo.numChannels, undo.numRows, undo.description, linkedFromPrevious, undo.channelInfo != nullptr);

	if(undo.channelInfo != nullptr)
	{
		if(undo.channelInfo->oldNumChannels != sndFile.GetNumChannels())
		{
			// Add or remove channels
			std::vector<CHANNELINDEX> channels(undo.channelInfo->oldNumChannels, CHANNELINDEX_INVALID);
			const CHANNELINDEX copyCount = std::min(sndFile.GetNumChannels(), undo.channelInfo->oldNumChannels);
			for(CHANNELINDEX i = 0; i < copyCount; i++)
			{
				channels[i] = i;
			}
			modDoc.ReArrangeChannels(channels, false);
		}
		memcpy(sndFile.ChnSettings, undo.channelInfo->settings, sizeof(ModChannelSettings) * undo.channelInfo->oldNumChannels);

		// Channel mute status might have changed...
		for(CHANNELINDEX i = 0; i < sndFile.GetNumChannels(); i++)
		{
			modDoc.UpdateChannelMuteStatus(i);
		}

	}

	PATTERNINDEX nPattern = undo.pattern;
	if(undo.firstChannel + undo.numChannels <= sndFile.GetNumChannels())
	{
		if(!sndFile.Patterns.IsValidPat(nPattern))
		{
			if(!sndFile.Patterns[nPattern].AllocatePattern(undo.numPatternRows))
			{
				DeleteUndoStep(fromBuf, fromBuf.size() - 1);
				return PATTERNINDEX_INVALID;
			}
		} else if(sndFile.Patterns[nPattern].GetNumRows() != undo.numPatternRows)
		{
			sndFile.Patterns[nPattern].Resize(undo.numPatternRows);
		}

		linkToPrevious = undo.linkToPrevious;
		const ModCommand *pUndoData = undo.pbuffer;
		ModCommand *pPattern = sndFile.Patterns[nPattern].GetpModCommand(undo.firstRow, undo.firstChannel);
		for(ROWINDEX iy = 0; iy < undo.numRows; iy++)
		{
			memcpy(pPattern, pUndoData, undo.numChannels * sizeof(ModCommand));
			pPattern += sndFile.GetNumChannels();
			pUndoData += undo.numChannels;
		}
	}

	DeleteUndoStep(fromBuf, fromBuf.size() - 1);

	if(fromBuf.empty()) modDoc.UpdateAllViews(NULL, HINT_UNDO);

	if(linkToPrevious)
	{
		nPattern = Undo(fromBuf, toBuf, true);
	}

	return nPattern;
}


// Remove all undo or redo steps
void CPatternUndo::ClearBuffer(undobuf_t &buffer)
//-----------------------------------------------
{
	while(buffer.size() > 0)
	{
		DeleteUndoStep(buffer, buffer.size() - 1);
	}
}


// Delete a given undo / redo step.
void CPatternUndo::DeleteUndoStep(undobuf_t &buffer, size_t step)
//---------------------------------------------------------------
{
	if(step >= buffer.size()) return;
	delete[] buffer[step].pbuffer;
	delete buffer[step].channelInfo;
	buffer.erase(buffer.begin() + step);
}


// Public helper function to remove the most recent undo point.
void CPatternUndo::RemoveLastUndoStep()
//-------------------------------------
{
	if(!CanUndo()) return;
	DeleteUndoStep(UndoBuffer, UndoBuffer.size() - 1);
}


const char *CPatternUndo::GetUndoName() const
//-------------------------------------------
{
	if(!CanUndo())
	{
		return "";
	}
	return UndoBuffer.back().description;
}


const char *CPatternUndo::GetRedoName() const
//-------------------------------------------
{
	if(!CanRedo())
	{
		return "";
	}
	return RedoBuffer.back().description;
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
	if(!SampleBufferExists(smp)) return false;

	// Remove an undo step if there are too many.
	while(UndoBuffer[smp - 1].size() >= MAX_UNDO_LEVEL)
	{
		DeleteUndoStep(smp, 0);
	}
	
	// Create new undo slot
	UndoInfo undo;

	CSoundFile &sndFile = modDoc.GetrSoundFile();
	const ModSample &oldSample = sndFile.GetSample(smp);

	// Save old sample header
	undo.OldSample = oldSample;
	mpt::String::Copy(undo.oldName, sndFile.m_szNames[smp]);
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

	// Restrict amount of memory that's being used
	RestrictBufferSize();

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

			undo.samplePtr = ModSample::AllocateSample(changeLen, bytesPerSample);
			if(undo.samplePtr == nullptr) return false;
			memcpy(undo.samplePtr, static_cast<const char *>(oldSample.pSample) + changeStart * bytesPerSample, changeLen * bytesPerSample);

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

	modDoc.UpdateAllViews(NULL, HINT_UNDO);

	return true;
}


// Restore undo point for given sample
bool CSampleUndo::Undo(const SAMPLEINDEX smp)
//-------------------------------------------
{
	if(!CanUndo(smp)) return false;

	CSoundFile &sndFile = modDoc.GetrSoundFile();

	// Select most recent undo slot
	UndoInfo &undo = UndoBuffer[smp - 1].back();

	ModSample &sample = sndFile.GetSample(smp);
	char *pCurrentSample = static_cast<char *>(sample.pSample);
	char *pNewSample = nullptr;	// a new sample is possibly going to be allocated, depending on what's going to be undone.
	bool replace = false;

	uint8 bytesPerSample = undo.OldSample.GetBytesPerSample();
	SmpLength changeLen = undo.changeEnd - undo.changeStart;

	switch(undo.changeType)
	{
	case sundo_none:
		break;

	case sundo_invert:
		// invert again
		ctrlSmp::InvertSample(sample, undo.changeStart, undo.changeEnd, sndFile);
		break;

	case sundo_reverse:
		// reverse again
		ctrlSmp::ReverseSample(sample, undo.changeStart, undo.changeEnd, sndFile);
		break;

	case sundo_unsign:
		// unsign again
		ctrlSmp::UnsignSample(sample, undo.changeStart, undo.changeEnd, sndFile);
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
		pNewSample = static_cast<char *>(ModSample::AllocateSample(undo.OldSample.nLength, bytesPerSample));
		if(pNewSample == nullptr) return false;
		replace = true;
		memcpy(pNewSample, pCurrentSample, undo.changeStart * bytesPerSample);
		memcpy(pNewSample + undo.changeStart * bytesPerSample, undo.samplePtr, changeLen * bytesPerSample);
		memcpy(pNewSample + undo.changeEnd * bytesPerSample, pCurrentSample + undo.changeStart * bytesPerSample, (undo.OldSample.nLength - undo.changeEnd) * bytesPerSample);
		break;

	case sundo_replace:
		// simply exchange sample pointer
		pNewSample = static_cast<char *>(undo.samplePtr);
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
	MemCopy(sndFile.m_szNames[smp], undo.oldName);

	if(replace)
	{
		ctrlSmp::ReplaceSample(sample, pNewSample, undo.OldSample.nLength, sndFile);
	}
	sample.PrecomputeLoops(sndFile, true);

	RemoveLastUndoStep(smp);

	if(CanUndo(smp) == false) modDoc.UpdateAllViews(NULL, HINT_UNDO);
	modDoc.SetModified();

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
	ModSample::FreeSample(UndoBuffer[smp - 1][step].samplePtr);
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
	while(capacity > TrackerSettings::Instance().GetSampleUndoBufferSize())
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
			if(capacity <= TrackerSettings::Instance().GetSampleUndoBufferSize()) return;
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
