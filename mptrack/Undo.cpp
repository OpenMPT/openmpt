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


OPENMPT_NAMESPACE_BEGIN


/////////////////////////////////////////////////////////////////////////////////////////
// Pattern Undo Functions


// Remove all undo steps.
void CPatternUndo::ClearUndo()
//----------------------------
{
	ClearBuffer(UndoBuffer);
	ClearBuffer(RedoBuffer);
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
bool CPatternUndo::PrepareUndo(PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, const char *description, bool linkToPrevious, bool storeChannelInfo)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(PrepareBuffer(UndoBuffer, pattern, firstChn, firstRow, numChns, numRows, description, linkToPrevious, storeChannelInfo))
	{
		ClearBuffer(RedoBuffer);
		return true;
	}
	return false;
}


bool CPatternUndo::PrepareBuffer(undobuf_t &buffer, PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, const char *description, bool linkToPrevious, bool storeChannelInfo)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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
		DeleteStep(buffer, 0);
	}

	UndoInfo undo;
	undo.pattern = pattern;
	undo.numPatternRows = sndFile.Patterns[pattern].GetNumRows();
	undo.firstChannel = firstChn;
	undo.firstRow = firstRow;
	undo.numChannels = numChns;
	undo.numRows = numRows;
	undo.pbuffer = pUndoData;
	undo.linkToPrevious = linkToPrevious;
	undo.description = description;
	const ModCommand *pPattern = sndFile.Patterns[pattern].GetpModCommand(firstRow, firstChn);
	for(ROWINDEX iy = 0; iy < numRows; iy++)
	{
		memcpy(pUndoData, pPattern, numChns * sizeof(ModCommand));
		pUndoData += numChns;
		pPattern += sndFile.GetNumChannels();
	}

	if(storeChannelInfo)
	{
		undo.channelInfo = new UndoInfo::ChannelInfo(sndFile.GetNumChannels());
		memcpy(undo.channelInfo->settings, sndFile.ChnSettings, sizeof(ModChannelSettings) * sndFile.GetNumChannels());
	} else
	{
		undo.channelInfo = nullptr;
	}

	buffer.push_back(undo);

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());
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

	PrepareBuffer(toBuf, undo.pattern, undo.firstChannel, undo.firstRow, undo.numChannels, undo.numRows, undo.description, linkedFromPrevious, undo.channelInfo != nullptr);

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
			if(!sndFile.Patterns.Insert(nPattern, undo.numPatternRows))
			{
				DeleteStep(fromBuf, fromBuf.size() - 1);
				return PATTERNINDEX_INVALID;
			}
		} else if(sndFile.Patterns[nPattern].GetNumRows() != undo.numPatternRows)
		{
			sndFile.Patterns[nPattern].Resize(undo.numPatternRows);
		}

		linkToPrevious = undo.linkToPrevious;
		const ModCommand *pUndoData = undo.pbuffer;
		CPattern &pattern = sndFile.Patterns[nPattern];
		ModCommand *m = pattern.GetpModCommand(undo.firstRow, undo.firstChannel);
		const ROWINDEX numRows = std::min(undo.numRows, pattern.GetNumRows());
		for(ROWINDEX iy = 0; iy < numRows; iy++)
		{
			memcpy(m, pUndoData, undo.numChannels * sizeof(ModCommand));
			m += sndFile.GetNumChannels();
			pUndoData += undo.numChannels;
		}
	}

	DeleteStep(fromBuf, fromBuf.size() - 1);

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());

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
	while(!buffer.empty())
	{
		DeleteStep(buffer, buffer.size() - 1);
	}
}


// Delete a given undo / redo step.
void CPatternUndo::DeleteStep(undobuf_t &buffer, size_t step)
//-----------------------------------------------------------
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
	DeleteStep(UndoBuffer, UndoBuffer.size() - 1);
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
		ClearUndo(UndoBuffer, smp);
		ClearUndo(RedoBuffer, smp);
	}
	UndoBuffer.clear();
	RedoBuffer.clear();
}


// Remove all undo steps of a given sample.
void CSampleUndo::ClearUndo(undobuf_t &buffer, const SAMPLEINDEX smp)
//-------------------------------------------------------------------
{
	if(!SampleBufferExists(buffer, smp)) return;

	while(!buffer[smp - 1].empty())
	{
		DeleteStep(buffer, smp, 0);
	}
}


// Create undo point for given sample.
// The main program has to tell what kind of changes are going to be made to the sample.
// That way, a lot of RAM can be saved, because some actions don't even require an undo sample buffer.
bool CSampleUndo::PrepareUndo(const SAMPLEINDEX smp, sampleUndoTypes changeType, const char *description, SmpLength changeStart, SmpLength changeEnd)
//---------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(PrepareBuffer(UndoBuffer, smp, changeType, description, changeStart, changeEnd))
	{
		ClearUndo(RedoBuffer, smp);
		return true;
	}
	return false;
}


bool CSampleUndo::PrepareBuffer(undobuf_t &buffer, const SAMPLEINDEX smp, sampleUndoTypes changeType, const char *description, SmpLength changeStart, SmpLength changeEnd)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(smp == 0 || smp >= MAX_SAMPLES) return false;
	if(!TrackerSettings::Instance().m_SampleUndoBufferSize.Get().GetSizeInBytes())
	{
		// Undo/Redo is disabled
		return false;
	}
	if(smp > buffer.size())
	{
		buffer.resize(smp);
	}

	// Remove an undo step if there are too many.
	while(buffer[smp - 1].size() >= MAX_UNDO_LEVEL)
	{
		DeleteStep(buffer, smp, 0);
	}
	
	// Create new undo slot
	UndoInfo undo;

	const CSoundFile &sndFile = modDoc.GetrSoundFile();
	const ModSample &oldSample = sndFile.GetSample(smp);

	// Save old sample header
	undo.OldSample = oldSample;
	mpt::String::Copy(undo.oldName, sndFile.m_szNames[smp]);
	undo.changeType = changeType;
	undo.description = description;

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
		MPT_ASSERT(false);
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
	case sundo_reverse:	// ditto
	case sundo_unsign:	// ditto
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
			memcpy(undo.samplePtr, oldSample.pSample8 + changeStart * bytesPerSample, changeLen * bytesPerSample);

#ifdef _DEBUG
			char s[64];
			const size_t nSize = (GetBufferCapacity(UndoBuffer) + GetBufferCapacity(RedoBuffer) + changeLen * bytesPerSample) >> 10;
			wsprintf(s, "Sample undo/redo buffer size is now %u.%u MB\n", nSize >> 10, (nSize & 1023) * 100 / 1024);
			Log(s);
#endif

		}
		break;

	default:
		MPT_ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
		return false;
	}

	buffer[smp - 1].push_back(undo);

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());

	return true;
}


// Restore undo point for given sample
bool CSampleUndo::Undo(const SAMPLEINDEX smp)
//-------------------------------------------
{
	return Undo(UndoBuffer, RedoBuffer, smp);
}


// Restore redo point for given sample
bool CSampleUndo::Redo(const SAMPLEINDEX smp)
//-------------------------------------------
{
	return Undo(RedoBuffer, UndoBuffer, smp);
}


// Restore undo/redo point for given sample
bool CSampleUndo::Undo(undobuf_t &fromBuf, undobuf_t &toBuf, const SAMPLEINDEX smp)
//---------------------------------------------------------------------------------
{
	if(!SampleBufferExists(fromBuf, smp) || fromBuf[smp - 1].empty()) return false;

	CSoundFile &sndFile = modDoc.GetrSoundFile();

	// Select most recent undo slot and temporarily remove it from the buffer so that it won't get deleted by possible buffer size restrictions in PrepareBuffer()
	UndoInfo undo = fromBuf[smp - 1].back();
	fromBuf[smp - 1].pop_back();

	// When turning an undo point into a redo point (and vice versa), some action types need to be adjusted.
	sampleUndoTypes redoType = undo.changeType;
	if(redoType == sundo_delete)
		redoType = sundo_insert;
	else if(redoType == sundo_insert)
		redoType = sundo_delete;
	PrepareBuffer(toBuf, smp, redoType, undo.description, undo.changeStart, undo.changeEnd);

	ModSample &sample = sndFile.GetSample(smp);
	int8 *pCurrentSample = sample.pSample8;
	int8 *pNewSample = nullptr;	// a new sample is possibly going to be allocated, depending on what's going to be undone.
	bool keepOnDisk = sample.uFlags[SMP_KEEPONDISK];
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
		MPT_ASSERT(changeLen == sample.nLength - undo.OldSample.nLength);
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
		pNewSample = static_cast<int8 *>(ModSample::AllocateSample(undo.OldSample.nLength, bytesPerSample));
		if(pNewSample == nullptr) return false;
		replace = true;
		memcpy(pNewSample, pCurrentSample, undo.changeStart * bytesPerSample);
		memcpy(pNewSample + undo.changeStart * bytesPerSample, undo.samplePtr, changeLen * bytesPerSample);
		memcpy(pNewSample + undo.changeEnd * bytesPerSample, pCurrentSample + undo.changeStart * bytesPerSample, (undo.OldSample.nLength - undo.changeEnd) * bytesPerSample);
		break;

	case sundo_replace:
		// simply exchange sample pointer
		pNewSample = static_cast<int8 *>(undo.samplePtr);
		undo.samplePtr = nullptr; // prevent sample from being deleted
		replace = true;
		break;

	default:
		MPT_ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
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

	if(undo.changeType != sundo_none)
	{
		sample.uFlags.set(SMP_MODIFIED);
	}
	if(!keepOnDisk)
	{
		// Never re-enable the keep on disk flag after it was disabled.
		// This can lead to quite some dangerous situations when replacing samples.
		sample.uFlags.reset(SMP_KEEPONDISK);
	}

	fromBuf[smp - 1].push_back(undo);
	DeleteStep(fromBuf, smp, fromBuf[smp - 1].size() - 1);

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());
	modDoc.SetModified();

	return true;
}


// Delete a given undo / redo step of a sample.
void CSampleUndo::DeleteStep(undobuf_t &buffer, const SAMPLEINDEX smp, const size_t step)
//---------------------------------------------------------------------------------------
{
	if(!SampleBufferExists(buffer, smp) || step >= buffer[smp - 1].size()) return;
	ModSample::FreeSample(buffer[smp - 1][step].samplePtr);
	buffer[smp - 1].erase(buffer[smp - 1].begin() + step);
}


// Public helper function to remove the most recent undo point.
void CSampleUndo::RemoveLastUndoStep(const SAMPLEINDEX smp)
//---------------------------------------------------------
{
	if(!CanUndo(smp)) return;
	DeleteStep(UndoBuffer, smp, UndoBuffer[smp - 1].size() - 1);
}


// Restrict undo buffer size so it won't grow too large.
// This is done in FIFO style, equally distributed over all sample slots (very simple).
void CSampleUndo::RestrictBufferSize()
//------------------------------------
{
	size_t capacity = GetBufferCapacity(UndoBuffer) + GetBufferCapacity(RedoBuffer);
	while(capacity > TrackerSettings::Instance().m_SampleUndoBufferSize.Get().GetSizeInBytes())
	{
		RestrictBufferSize(UndoBuffer, capacity);
		RestrictBufferSize(RedoBuffer, capacity);
	}
}


void CSampleUndo::RestrictBufferSize(undobuf_t &buffer, size_t &capacity)
//-----------------------------------------------------------------------
{
	for(SAMPLEINDEX smp = 1; smp <= buffer.size(); smp++)
	{
		if(capacity <= TrackerSettings::Instance().m_SampleUndoBufferSize.Get().GetSizeInBytes()) return;
		for(size_t i = 0; i < buffer[smp - 1].size(); i++)
		{
			if(buffer[smp - 1][i].samplePtr != nullptr)
			{
				capacity -= (buffer[smp - 1][i].changeEnd - buffer[smp - 1][i].changeStart) * buffer[smp - 1][i].OldSample.GetBytesPerSample();
				for(size_t j = 0; j <= i; j++)
				{
					DeleteStep(buffer, smp, 0);
				}
				// Try to evenly spread out the restriction, i.e. move on to other samples before deleting another step for this sample.
				break;
			}
		}
	}
}


// Update undo buffer when using rearrange sample functionality.
// newIndex contains one new index for each old index. newIndex[1] represents the first sample.
void CSampleUndo::RearrangeSamples(undobuf_t &buffer, const std::vector<SAMPLEINDEX> &newIndex)
//---------------------------------------------------------------------------------------------
{
	undobuf_t newBuf(modDoc.GetNumSamples());

	const SAMPLEINDEX newSize = static_cast<SAMPLEINDEX>(newIndex.size());
	const SAMPLEINDEX oldSize = static_cast<SAMPLEINDEX>(buffer.size());
	for(SAMPLEINDEX smp = 1; smp <= oldSize; smp++)
	{
		MPT_ASSERT(smp >= newSize || newIndex[smp] <= modDoc.GetNumSamples());
		if(smp < newSize && newIndex[smp] > 0 && newIndex[smp] <= modDoc.GetNumSamples())
		{
			newBuf[newIndex[smp] - 1] = buffer[smp - 1];
		} else
		{
			ClearUndo(smp);
		}
	}
#ifdef _DEBUG
	for(size_t i = 0; i < oldSize; i++)
	{
		if(i + 1 < newIndex.size() && newIndex[i + 1] != 0)
			MPT_ASSERT(newBuf[newIndex[i + 1] - 1].size() == buffer[i].size());
		else
			MPT_ASSERT(buffer[i].empty());
	}
#endif
	buffer = newBuf;
}


// Return total amount of bytes used by the sample undo buffer.
size_t CSampleUndo::GetBufferCapacity(const undobuf_t &buffer) const
//------------------------------------------------------------------
{
	size_t sum = 0;
	for(auto smp = buffer.begin(); smp != buffer.end(); smp++)
	{
		for(size_t nStep = 0; nStep < smp->size(); nStep++)
		{
			auto &step = smp->at(nStep);
			if(step.samplePtr != nullptr)
			{
				sum += (step.changeEnd - step.changeStart) * step.OldSample.GetBytesPerSample();
			}
		}
	}
	return sum;
}


// Ensure that the undo buffer is big enough for a given sample number
bool CSampleUndo::SampleBufferExists(const undobuf_t &buffer, const SAMPLEINDEX smp) const
//----------------------------------------------------------------------------------------
{
	if(smp == 0 || smp >= MAX_SAMPLES) return false;
	if(smp <= buffer.size()) return true;
	return false;
}

// Get name of next undo item
const char *CSampleUndo::GetUndoName(const SAMPLEINDEX smp) const
//---------------------------------------------------------------
{
	if(!CanUndo(smp))
	{
		return "";
	}
	return UndoBuffer[smp - 1].back().description;
}


// Get name of next redo item
const char *CSampleUndo::GetRedoName(const SAMPLEINDEX smp) const
//---------------------------------------------------------------
{
	if(!CanRedo(smp))
	{
		return "";
	}
	return RedoBuffer[smp - 1].back().description;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Instrument Undo Functions


// Remove all undo steps for all instruments.
void CInstrumentUndo::ClearUndo()
//-------------------------------
{
	UndoBuffer.clear();
	RedoBuffer.clear();
}


// Remove all undo steps of a given instrument.
void CInstrumentUndo::ClearUndo(undobuf_t &buffer, const INSTRUMENTINDEX ins)
//---------------------------------------------------------------------------
{
	if(!InstrumentBufferExists(buffer, ins)) return;
	buffer[ins - 1].clear();
}


// Create undo point for given Instrument.
// The main program has to tell what kind of changes are going to be made to the Instrument.
// That way, a lot of RAM can be saved, because some actions don't even require an undo Instrument buffer.
bool CInstrumentUndo::PrepareUndo(const INSTRUMENTINDEX ins, const char *description, EnvelopeType envType)
//---------------------------------------------------------------------------------------------------------
{
	if(PrepareBuffer(UndoBuffer, ins, description, envType))
	{
		ClearUndo(RedoBuffer, ins);
		return true;
	}
	return false;
}


bool CInstrumentUndo::PrepareBuffer(undobuf_t &buffer, const INSTRUMENTINDEX ins, const char *description, EnvelopeType envType)
//------------------------------------------------------------------------------------------------------------------------------
{
	if(ins == 0 || ins >= MAX_INSTRUMENTS || modDoc.GetrSoundFile().Instruments[ins] == nullptr) return false;
	if(ins > buffer.size())
	{
		buffer.resize(ins);
	}

	// Remove undo steps if there are too many.
	if(buffer[ins - 1].size() >= MAX_UNDO_LEVEL)
	{
		buffer.erase(buffer.begin(), buffer.begin() + (buffer[ins - 1].size() - MAX_UNDO_LEVEL + 1));
	}
	
	// Create new undo slot
	UndoInfo undo;

	const CSoundFile &sndFile = modDoc.GetrSoundFile();
	undo.description = description;
	undo.editedEnvelope = envType;
	if(envType < ENV_MAXTYPES)
	{
		undo.instr.GetEnvelope(envType) = sndFile.Instruments[ins]->GetEnvelope(envType);
	} else
	{
		undo.instr = *sndFile.Instruments[ins];
	}
	buffer[ins - 1].push_back(undo);

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());

	return true;
}


// Restore undo point for given Instrument
bool CInstrumentUndo::Undo(const INSTRUMENTINDEX ins)
//---------------------------------------------------
{
	return Undo(UndoBuffer, RedoBuffer, ins);
}


// Restore redo point for given Instrument
bool CInstrumentUndo::Redo(const INSTRUMENTINDEX ins)
//---------------------------------------------------
{
	return Undo(RedoBuffer, UndoBuffer, ins);
}


// Restore undo/redo point for given Instrument
bool CInstrumentUndo::Undo(undobuf_t &fromBuf, undobuf_t &toBuf, const INSTRUMENTINDEX ins)
//-----------------------------------------------------------------------------------------
{
	CSoundFile &sndFile = modDoc.GetrSoundFile();
	if(sndFile.Instruments[ins] == nullptr || !InstrumentBufferExists(fromBuf, ins) || fromBuf[ins - 1].empty()) return false;

	// Select most recent undo slot
	const UndoInfo &undo = fromBuf[ins - 1].back();

	PrepareBuffer(toBuf, ins, undo.description, undo.editedEnvelope);

	// When turning an undo point into a redo point (and vice versa), some action types need to be adjusted.
	ModInstrument *instr = sndFile.Instruments[ins];
	if(undo.editedEnvelope < ENV_MAXTYPES)
	{
		instr->GetEnvelope(undo.editedEnvelope) = undo.instr.GetEnvelope(undo.editedEnvelope);
	} else
	{
		*instr = undo.instr;
	}

	DeleteStep(fromBuf, ins, fromBuf[ins - 1].size() - 1);

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());
	modDoc.SetModified();

	return true;
}


// Delete a given undo / redo step of a Instrument.
void CInstrumentUndo::DeleteStep(undobuf_t &buffer, const INSTRUMENTINDEX ins, const size_t step)
//---------------------------------------------------------------------------------------
{
	if(!InstrumentBufferExists(buffer, ins) || step >= buffer[ins - 1].size()) return;
	buffer[ins - 1].erase(buffer[ins - 1].begin() + step);
}


// Public helper function to remove the most recent undo point.
void CInstrumentUndo::RemoveLastUndoStep(const INSTRUMENTINDEX ins)
//---------------------------------------------------------
{
	if(!CanUndo(ins)) return;
	DeleteStep(UndoBuffer, ins, UndoBuffer[ins - 1].size() - 1);
}


// Update undo buffer when using rearrange instruments functionality.
// newIndex contains one new index for each old index. newIndex[1] represents the first instrument.
void CInstrumentUndo::RearrangeInstruments(undobuf_t &buffer, const std::vector<INSTRUMENTINDEX> &newIndex)
//---------------------------------------------------------------------------------------------------------
{
	undobuf_t newBuf(modDoc.GetNumInstruments());

	const INSTRUMENTINDEX newSize = static_cast<INSTRUMENTINDEX>(newIndex.size());
	const INSTRUMENTINDEX oldSize = static_cast<INSTRUMENTINDEX>(buffer.size());
	for(INSTRUMENTINDEX ins = 1; ins <= oldSize; ins++)
	{
		MPT_ASSERT(ins >= newSize || newIndex[ins] <= modDoc.GetNumInstruments());
		if(ins < newSize && newIndex[ins] > 0 && newIndex[ins] <= modDoc.GetNumInstruments())
		{
			newBuf[newIndex[ins] - 1] = buffer[ins - 1];
		} else
		{
			ClearUndo(ins);
		}
	}
#ifdef _DEBUG
	for(size_t i = 0; i < oldSize; i++)
	{
		if(i + 1 < newIndex.size() && newIndex[i + 1] != 0)
			MPT_ASSERT(newBuf[newIndex[i + 1] - 1].size() == buffer[i].size());
		else
			MPT_ASSERT(buffer[i].empty());
	}
#endif
	buffer = newBuf;
}


// Update undo buffer when using rearrange samples functionality.
// newIndex contains one new index for each old index. newIndex[1] represents the first sample.
void CInstrumentUndo::RearrangeSamples(undobuf_t &buffer, const INSTRUMENTINDEX ins, std::vector<SAMPLEINDEX> &newIndex)
//----------------------------------------------------------------------------------------------------------------------
{
	const CSoundFile &sndFile = modDoc.GetrSoundFile();
	if(sndFile.Instruments[ins] == nullptr || !InstrumentBufferExists(buffer, ins) || buffer[ins - 1].empty()) return;

	for(auto &i : buffer[ins - 1]) if(i.editedEnvelope >= ENV_MAXTYPES)
	{
		for(auto &sample : i.instr.Keyboard)
		{
			if(sample < newIndex.size())
				sample = newIndex[sample];
			else
				sample = 0;
		}
	}
}


// Ensure that the undo buffer is big enough for a given Instrument number
bool CInstrumentUndo::InstrumentBufferExists(const undobuf_t &buffer, const INSTRUMENTINDEX ins) const
//----------------------------------------------------------------------------------------------------
{
	if(ins == 0 || ins >= MAX_INSTRUMENTS) return false;
	if(ins <= buffer.size()) return true;
	return false;
}


// Get name of next undo item
const char *CInstrumentUndo::GetUndoName(const INSTRUMENTINDEX ins) const
//-----------------------------------------------------------------------
{
	if(!CanUndo(ins))
	{
		return "";
	}
	return UndoBuffer[ins - 1].back().description;
}


// Get name of next redo item
const char *CInstrumentUndo::GetRedoName(const INSTRUMENTINDEX ins) const
//-----------------------------------------------------------------------
{
	if(!CanRedo(ins))
	{
		return "";
	}
	return RedoBuffer[ins - 1].back().description;
}


OPENMPT_NAMESPACE_END
