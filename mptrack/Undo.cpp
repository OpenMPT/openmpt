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
#include "Moddoc.h"
#include "Mainfrm.h"
#include "Undo.h"
#include "../common/mptStringBuffer.h"
#include "../tracklib/SampleEdit.h"
#include "../soundlib/modsmp_ctrl.h"


OPENMPT_NAMESPACE_BEGIN


/////////////////////////////////////////////////////////////////////////////////////////
// Pattern Undo Functions


// Remove all undo steps.
void CPatternUndo::ClearUndo()
{
	UndoBuffer.clear();
	RedoBuffer.clear();
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
{
	if(PrepareBuffer(UndoBuffer, pattern, firstChn, firstRow, numChns, numRows, description, linkToPrevious, storeChannelInfo))
	{
		RedoBuffer.clear();
		return true;
	}
	return false;
}


bool CPatternUndo::PrepareChannelUndo(CHANNELINDEX firstChn, CHANNELINDEX numChns, const char* description)
{
	return PrepareUndo(PATTERNINDEX_INVALID, firstChn, 0, numChns, 0, description, false, true);
}


bool CPatternUndo::PrepareBuffer(undobuf_t &buffer, PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, const char *description, bool linkToPrevious, bool storeChannelInfo) const
{
	const CSoundFile &sndFile = modDoc.GetSoundFile();
	const bool onlyChannelInfo = storeChannelInfo && numRows < 1;

	ROWINDEX patRows = 0;
	if(sndFile.Patterns.IsValidPat(pattern))
	{
		patRows = sndFile.Patterns[pattern].GetNumRows();
		if((firstRow >= patRows) || (firstChn >= sndFile.GetNumChannels()))
			return false;
		if(numChns < 1 || numRows < 1)
			return false;
		if(firstRow + numRows >= patRows)
			numRows = patRows - firstRow;
		if(firstChn + numChns >= sndFile.GetNumChannels())
			numChns = sndFile.GetNumChannels() - firstChn;
	} else if(!onlyChannelInfo)
	{
		return false;
	}

	// Remove an undo step if there are too many.
	if(buffer.size() >= MAX_UNDO_LEVEL)
	{
		buffer.erase(buffer.begin(), buffer.begin() + (buffer.size() - MAX_UNDO_LEVEL + 1));
	}

	UndoInfo undo;
	undo.pattern = pattern;
	undo.numPatternRows = patRows;
	undo.firstChannel = firstChn;
	undo.firstRow = firstRow;
	undo.numChannels = numChns;
	undo.numRows = numRows;
	undo.linkToPrevious = linkToPrevious;
	undo.description = description;

	if(!onlyChannelInfo)
	{
		try
		{
			undo.content.resize(numRows * numChns);
		} catch(mpt::out_of_memory e)
		{
			mpt::delete_out_of_memory(e);
			return false;
		}
		const ModCommand *pPattern = sndFile.Patterns[pattern].GetpModCommand(firstRow, firstChn);
		auto pUndoData = undo.content.begin();
		for(ROWINDEX iy = 0; iy < numRows; iy++)
		{
			std::copy(pPattern, pPattern + numChns, pUndoData);
			pUndoData += numChns;
			pPattern += sndFile.GetNumChannels();
		}
	}

	if(storeChannelInfo)
	{
		undo.channelInfo.assign(std::begin(sndFile.ChnSettings) + firstChn, std::begin(sndFile.ChnSettings) + firstChn + numChns);
	}

	buffer.push_back(std::move(undo));

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());
	return true;
}


// Restore an undo point. Returns which pattern has been modified.
PATTERNINDEX CPatternUndo::Undo()
{
	return Undo(UndoBuffer, RedoBuffer, false);
}


// Restore an undo point. Returns which pattern has been modified.
PATTERNINDEX CPatternUndo::Redo()
{
	return Undo(RedoBuffer, UndoBuffer, false);
}


// Restore an undo point. Returns which pattern has been modified.
// linkedFromPrevious is true if a connected undo event is going to be deleted (can only be called internally).
PATTERNINDEX CPatternUndo::Undo(undobuf_t &fromBuf, undobuf_t &toBuf, bool linkedFromPrevious)
{
	CSoundFile &sndFile = modDoc.GetSoundFile();

	bool linkToPrevious = false;

	if(fromBuf.empty())
		return PATTERNINDEX_INVALID;

	// Select most recent undo slot
	const UndoInfo &undo = fromBuf.back();
	const bool onlyChannelSettings = undo.OnlyChannelSettings();
	const bool deletePattern = (undo.numPatternRows == DELETE_PATTERN) && !onlyChannelSettings;

	// Add this action to redo buffer if the pattern exists; otherwise add a special deletion redo step later
	const bool patternExists = sndFile.Patterns.IsValidPat(undo.pattern);
	if(patternExists || onlyChannelSettings)
		PrepareBuffer(toBuf, undo.pattern, undo.firstChannel, undo.firstRow, undo.numChannels, undo.numRows, undo.description, linkedFromPrevious, !undo.channelInfo.empty());

	const bool modifyChannels = !undo.channelInfo.empty();
	const CHANNELINDEX updateChannel = (undo.numChannels == 1) ? undo.firstChannel : CHANNELINDEX_INVALID;
	if(modifyChannels)
	{
		const bool modifyChannelCount =
		    (!onlyChannelSettings && undo.channelInfo.size() != sndFile.GetNumChannels())
		    || (onlyChannelSettings && (undo.firstChannel + undo.channelInfo.size()) > sndFile.GetNumChannels());
		if(modifyChannelCount)
		{
			// Add or remove channels
			std::vector<CHANNELINDEX> channels(undo.channelInfo.size(), CHANNELINDEX_INVALID);
			const CHANNELINDEX copyCount = std::min(sndFile.GetNumChannels(), static_cast<CHANNELINDEX>(undo.channelInfo.size()));
			std::iota(channels.begin(), channels.begin() + copyCount, CHANNELINDEX(0));
			modDoc.ReArrangeChannels(channels, false);
		}
		if(undo.firstChannel + undo.channelInfo.size() <= sndFile.GetNumChannels())
		{
			std::move(undo.channelInfo.cbegin(), undo.channelInfo.cend(), std::begin(sndFile.ChnSettings) + undo.firstChannel);
		}

		// Channel mute status might have changed...
		for(CHANNELINDEX i = undo.firstChannel; i < sndFile.GetNumChannels(); i++)
		{
			modDoc.UpdateChannelMuteStatus(i);
		}
	}

	PATTERNINDEX pat = undo.pattern;
	if(deletePattern)
	{
		sndFile.Patterns.Remove(pat);
	} else if(undo.firstChannel + undo.numChannels <= sndFile.GetNumChannels() && !onlyChannelSettings)
	{
		if(!patternExists)
		{
			if(!sndFile.Patterns.Insert(pat, undo.numPatternRows))
			{
				fromBuf.pop_back();
				return PATTERNINDEX_INVALID;
			}
		} else if(sndFile.Patterns[pat].GetNumRows() != undo.numPatternRows)
		{
			sndFile.Patterns[pat].Resize(undo.numPatternRows);
		}

		linkToPrevious = undo.linkToPrevious;
		auto pUndoData = undo.content.cbegin();
		CPattern &pattern = sndFile.Patterns[pat];
		ModCommand *m = pattern.GetpModCommand(undo.firstRow, undo.firstChannel);
		const ROWINDEX numRows = std::min(undo.numRows, pattern.GetNumRows());
		for(ROWINDEX iy = 0; iy < numRows; iy++)
		{
			std::move(pUndoData, pUndoData + undo.numChannels, m);
			m += sndFile.GetNumChannels();
			pUndoData += undo.numChannels;
		}
	}

	if(!patternExists && !onlyChannelSettings)
	{
		// Redo a deletion
		auto &redo = fromBuf.back();
		redo.content.clear();
		redo.numPatternRows = DELETE_PATTERN;
		toBuf.push_back(std::move(redo));
	}
	fromBuf.pop_back();

	if(patternExists != sndFile.Patterns.IsValidPat(pat))
	{
		modDoc.UpdateAllViews(nullptr, PatternHint(pat).Names().Undo());
		modDoc.UpdateAllViews(nullptr, SequenceHint().Data());  // Pattern color will change in sequence
	} else
	{
		modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());
	}
	if(modifyChannels)
		modDoc.UpdateAllViews(nullptr, GeneralHint(updateChannel).Channels());
	modDoc.SetModified();

	if(linkToPrevious)
	{
		pat = Undo(fromBuf, toBuf, true);
	}

	return pat;
}


// Public helper function to remove the most recent undo point.
void CPatternUndo::RemoveLastUndoStep()
{
	if(!UndoBuffer.empty())
		UndoBuffer.pop_back();
}


CString CPatternUndo::GetName(const undobuf_t &buffer)
{
	if(buffer.empty())
		return CString();
	
	const UndoInfo &info = buffer.back();
	CString desc = mpt::ToCString(mpt::Charset::Locale, info.description);
	if(info.linkToPrevious)
		desc += _T(" (Multiple Patterns)");
	else if(info.OnlyChannelSettings() && info.numChannels > 1)
		desc += _T(" (Multiple Channels)");
	else if(info.OnlyChannelSettings())
		desc += MPT_CFORMAT(" (Channel {})")(info.firstChannel + 1);
	else
		desc += MPT_CFORMAT(" (Pat {} Row {} Chn {})")(info.pattern, info.firstRow, info.firstChannel + 1);
	return desc;
}


void CPatternUndo::RearrangePatterns(undobuf_t &buffer, const std::vector<PATTERNINDEX> &newIndex)
{
	for(auto &step : buffer)
	{
		if(step.pattern < newIndex.size())
			step.pattern = newIndex[step.pattern];
	}
}


void CPatternUndo::RearrangePatterns(const std::vector<PATTERNINDEX> &newIndex)
{
	RearrangePatterns(UndoBuffer, newIndex);
	RearrangePatterns(RedoBuffer, newIndex);
}


/////////////////////////////////////////////////////////////////////////////////////////
// Sample Undo Functions


// Remove all undo steps for all samples.
void CSampleUndo::ClearUndo()
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
{
	if(PrepareBuffer(UndoBuffer, smp, changeType, description, changeStart, changeEnd))
	{
		ClearUndo(RedoBuffer, smp);
		return true;
	}
	return false;
}


bool CSampleUndo::PrepareBuffer(undobuf_t &buffer, const SAMPLEINDEX smp, sampleUndoTypes changeType, const char *description, SmpLength changeStart, SmpLength changeEnd)
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

	const CSoundFile &sndFile = modDoc.GetSoundFile();
	const ModSample &oldSample = sndFile.GetSample(smp);

	// Save old sample header
	undo.OldSample = oldSample;
	undo.oldName = sndFile.m_szNames[smp];
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
		if(oldSample.HasSampleData())
		{
			const uint8 bytesPerSample = oldSample.GetBytesPerSample();
			const SmpLength changeLen = changeEnd - changeStart;

			undo.samplePtr = ModSample::AllocateSample(changeLen, bytesPerSample);
			if(undo.samplePtr == nullptr) return false;
			memcpy(undo.samplePtr, oldSample.sampleb() + changeStart * bytesPerSample, changeLen * bytesPerSample);

#ifdef MPT_ALL_LOGGING
			const size_t nSize = (GetBufferCapacity(UndoBuffer) + GetBufferCapacity(RedoBuffer) + changeLen * bytesPerSample) >> 10;
			MPT_LOG(LogDebug, "Undo", MPT_UFORMAT("Sample undo/redo buffer size is now {}.{} MB")(nSize >> 10, (nSize & 1023) * 100 / 1024));
#endif

		}
		break;

	default:
		MPT_ASSERT(false); // whoops, what's this? someone forgot to implement it, some code is obviously missing here!
		return false;
	}

	buffer[smp - 1].push_back(std::move(undo));

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());

	return true;
}


// Restore undo point for given sample
bool CSampleUndo::Undo(const SAMPLEINDEX smp)
{
	return Undo(UndoBuffer, RedoBuffer, smp);
}


// Restore redo point for given sample
bool CSampleUndo::Redo(const SAMPLEINDEX smp)
{
	return Undo(RedoBuffer, UndoBuffer, smp);
}


// Restore undo/redo point for given sample
bool CSampleUndo::Undo(undobuf_t &fromBuf, undobuf_t &toBuf, const SAMPLEINDEX smp)
{
	if(!SampleBufferExists(fromBuf, smp) || fromBuf[smp - 1].empty()) return false;

	CSoundFile &sndFile = modDoc.GetSoundFile();

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
	std::byte *pCurrentSample = mpt::void_cast<std::byte*>(sample.samplev());
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
		SampleEdit::InvertSample(sample, undo.changeStart, undo.changeEnd, sndFile);
		break;

	case sundo_reverse:
		// reverse again
		SampleEdit::ReverseSample(sample, undo.changeStart, undo.changeEnd, sndFile);
		break;

	case sundo_unsign:
		// unsign again
		SampleEdit::UnsignSample(sample, undo.changeStart, undo.changeEnd, sndFile);
		break;

	case sundo_insert:
		// delete inserted data
		MPT_ASSERT(changeLen == sample.nLength - undo.OldSample.nLength);
		if(undo.OldSample.nLength > 0)
		{
			memcpy(pCurrentSample + undo.changeStart * bytesPerSample, pCurrentSample + undo.changeEnd * bytesPerSample, (sample.nLength - undo.changeEnd) * bytesPerSample);
			// also clean the sample end
			memset(pCurrentSample + undo.OldSample.nLength * bytesPerSample, 0, (sample.nLength - undo.OldSample.nLength) * bytesPerSample);
		} else
		{
			replace = true;
		}
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
	sample.pData.pSample = mpt::void_cast<void*>(pCurrentSample); // select the "correct" old sample
	sndFile.m_szNames[smp] = undo.oldName;

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

	fromBuf[smp - 1].push_back(std::move(undo));
	DeleteStep(fromBuf, smp, fromBuf[smp - 1].size() - 1);

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());
	modDoc.SetModified();

	return true;
}


// Delete a given undo / redo step of a sample.
void CSampleUndo::DeleteStep(undobuf_t &buffer, const SAMPLEINDEX smp, const size_t step)
{
	if(!SampleBufferExists(buffer, smp) || step >= buffer[smp - 1].size()) return;
	ModSample::FreeSample(buffer[smp - 1][step].samplePtr);
	buffer[smp - 1].erase(buffer[smp - 1].begin() + step);
}


// Public helper function to remove the most recent undo point.
void CSampleUndo::RemoveLastUndoStep(const SAMPLEINDEX smp)
{
	if(!CanUndo(smp)) return;
	DeleteStep(UndoBuffer, smp, UndoBuffer[smp - 1].size() - 1);
}


// Restrict undo buffer size so it won't grow too large.
// This is done in FIFO style, equally distributed over all sample slots (very simple).
void CSampleUndo::RestrictBufferSize()
{
	size_t capacity = GetBufferCapacity(UndoBuffer) + GetBufferCapacity(RedoBuffer);
	while(capacity > TrackerSettings::Instance().m_SampleUndoBufferSize.Get().GetSizeInBytes())
	{
		RestrictBufferSize(UndoBuffer, capacity);
		RestrictBufferSize(RedoBuffer, capacity);
	}
}


void CSampleUndo::RestrictBufferSize(undobuf_t &buffer, size_t &capacity)
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
{
	size_t sum = 0;
	for(auto &smp : buffer)
	{
		for(auto &step : smp)
		{
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
{
	if(smp == 0 || smp >= MAX_SAMPLES) return false;
	if(smp <= buffer.size()) return true;
	return false;
}

// Get name of next undo item
const char *CSampleUndo::GetUndoName(const SAMPLEINDEX smp) const
{
	if(!CanUndo(smp))
	{
		return "";
	}
	return UndoBuffer[smp - 1].back().description;
}


// Get name of next redo item
const char *CSampleUndo::GetRedoName(const SAMPLEINDEX smp) const
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
{
	UndoBuffer.clear();
	RedoBuffer.clear();
}


// Remove all undo steps of a given instrument.
void CInstrumentUndo::ClearUndo(undobuf_t &buffer, const INSTRUMENTINDEX ins)
{
	if(!InstrumentBufferExists(buffer, ins)) return;
	buffer[ins - 1].clear();
}


// Create undo point for given Instrument.
// The main program has to tell what kind of changes are going to be made to the Instrument.
// That way, a lot of RAM can be saved, because some actions don't even require an undo Instrument buffer.
bool CInstrumentUndo::PrepareUndo(const INSTRUMENTINDEX ins, const char *description, EnvelopeType envType)
{
	if(PrepareBuffer(UndoBuffer, ins, description, envType))
	{
		ClearUndo(RedoBuffer, ins);
		return true;
	}
	return false;
}


bool CInstrumentUndo::PrepareBuffer(undobuf_t &buffer, const INSTRUMENTINDEX ins, const char *description, EnvelopeType envType)
{
	if(ins == 0 || ins >= MAX_INSTRUMENTS || modDoc.GetSoundFile().Instruments[ins] == nullptr) return false;
	if(ins > buffer.size())
	{
		buffer.resize(ins);
	}
	auto &insBuffer = buffer[ins - 1];

	// Remove undo steps if there are too many.
	if(insBuffer.size() >= MAX_UNDO_LEVEL)
	{
		insBuffer.erase(insBuffer.begin(), insBuffer.begin() + (insBuffer.size() - MAX_UNDO_LEVEL + 1));
	}
	
	// Create new undo slot
	UndoInfo undo;

	const CSoundFile &sndFile = modDoc.GetSoundFile();
	undo.description = description;
	undo.editedEnvelope = envType;
	if(envType < ENV_MAXTYPES)
	{
		undo.instr.GetEnvelope(envType) = sndFile.Instruments[ins]->GetEnvelope(envType);
	} else
	{
		undo.instr = *sndFile.Instruments[ins];
	}
	// cppcheck false-positive
	// cppcheck-suppress uninitStructMember
	insBuffer.push_back(std::move(undo));

	modDoc.UpdateAllViews(nullptr, UpdateHint().Undo());

	return true;
}


// Restore undo point for given Instrument
bool CInstrumentUndo::Undo(const INSTRUMENTINDEX ins)
{
	return Undo(UndoBuffer, RedoBuffer, ins);
}


// Restore redo point for given Instrument
bool CInstrumentUndo::Redo(const INSTRUMENTINDEX ins)
{
	return Undo(RedoBuffer, UndoBuffer, ins);
}


// Restore undo/redo point for given Instrument
bool CInstrumentUndo::Undo(undobuf_t &fromBuf, undobuf_t &toBuf, const INSTRUMENTINDEX ins)
{
	CSoundFile &sndFile = modDoc.GetSoundFile();
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
{
	if(!InstrumentBufferExists(buffer, ins) || step >= buffer[ins - 1].size()) return;
	buffer[ins - 1].erase(buffer[ins - 1].begin() + step);
}


// Public helper function to remove the most recent undo point.
void CInstrumentUndo::RemoveLastUndoStep(const INSTRUMENTINDEX ins)
{
	if(!CanUndo(ins)) return;
	DeleteStep(UndoBuffer, ins, UndoBuffer[ins - 1].size() - 1);
}


// Update undo buffer when using rearrange instruments functionality.
// newIndex contains one new index for each old index. newIndex[1] represents the first instrument.
void CInstrumentUndo::RearrangeInstruments(undobuf_t &buffer, const std::vector<INSTRUMENTINDEX> &newIndex)
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
{
	const CSoundFile &sndFile = modDoc.GetSoundFile();
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
{
	if(ins == 0 || ins >= MAX_INSTRUMENTS) return false;
	if(ins <= buffer.size()) return true;
	return false;
}


// Get name of next undo item
const char *CInstrumentUndo::GetUndoName(const INSTRUMENTINDEX ins) const
{
	if(!CanUndo(ins))
	{
		return "";
	}
	return UndoBuffer[ins - 1].back().description;
}


// Get name of next redo item
const char *CInstrumentUndo::GetRedoName(const INSTRUMENTINDEX ins) const
{
	if(!CanRedo(ins))
	{
		return "";
	}
	return RedoBuffer[ins - 1].back().description;
}


OPENMPT_NAMESPACE_END
