/*
 * ModEdit.cpp
 * -----------
 * Purpose: Song (pattern, samples, instruments) editing functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "dlg_misc.h"
#include "Dlsbank.h"
#include "../soundlib/modsmp_ctrl.h"
#include "../soundlib/mod_specifications.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"
#include "../common/mptFileIO.h"
#include "../common/mptBufferIO.h"
// Plugin cloning
#include "../soundlib/plugins/PluginManager.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "VstPresets.h"
#include "../common/FileReader.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


OPENMPT_NAMESPACE_BEGIN


#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"


	// Change the number of channels.
// Return true on success.
bool CModDoc::ChangeNumChannels(CHANNELINDEX nNewChannels, const bool showCancelInRemoveDlg)
//------------------------------------------------------------------------------------------
{
	const CHANNELINDEX maxChans = m_SndFile.GetModSpecifications().channelsMax;

	if (nNewChannels > maxChans)
	{
		CString error;
		error.Format("Error: Max number of channels for this file type is %d", maxChans);
		Reporting::Warning(error);
		return false;
	}

	if (nNewChannels == GetNumChannels()) return false;

	if (nNewChannels < GetNumChannels())
	{
		// Remove channels
		UINT nChnToRemove = 0;
		CHANNELINDEX nFound = 0;

		//nNewChannels = 0 means user can choose how many channels to remove
		if(nNewChannels > 0)
		{
			nChnToRemove = GetNumChannels() - nNewChannels;
			nFound = nChnToRemove;
		} else
		{
			nChnToRemove = 0;
			nFound = GetNumChannels();
		}
		
		CRemoveChannelsDlg rem(m_SndFile, nChnToRemove, showCancelInRemoveDlg);
		CheckUsedChannels(rem.m_bKeepMask, nFound);
		if (rem.DoModal() != IDOK) return false;

		// Removing selected channels
		return RemoveChannels(rem.m_bKeepMask);
	} else
	{
		// Increasing number of channels
		BeginWaitCursor();
		std::vector<CHANNELINDEX> channels(nNewChannels, CHANNELINDEX_INVALID);
		for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
		{
			channels[nChn] = nChn;
		}

		const bool success = (ReArrangeChannels(channels) == nNewChannels);
		if(success)
		{
			SetModified();
			UpdateAllViews(nullptr, UpdateHint().ModType());
		}
		EndWaitCursor();
		return success;
	}
}


// To remove all channels whose index corresponds to false in the keepMask vector.
// Return true on success.
bool CModDoc::RemoveChannels(const std::vector<bool> &keepMask)
//-------------------------------------------------------------
{
	CHANNELINDEX nRemainingChannels = 0;
	//First calculating how many channels are to be left
	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		if(keepMask[nChn]) nRemainingChannels++;
	}
	if(nRemainingChannels == GetNumChannels() || nRemainingChannels < m_SndFile.GetModSpecifications().channelsMin)
	{
		CString str;
		if(nRemainingChannels == GetNumChannels())
			str = "No channels chosen to be removed.";
		else
			str = "No removal done - channel number is already at minimum.";
		Reporting::Information(str, "Remove Channels");
		return false;
	}

	BeginWaitCursor();
	// Create new channel order, with only channels from m_bChnMask left.
	std::vector<CHANNELINDEX> channels(nRemainingChannels, 0);
	CHANNELINDEX i = 0;
	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		if(keepMask[nChn])
		{
			channels[i++] = nChn;
		}
	}
	const bool success = (ReArrangeChannels(channels) == nRemainingChannels);
	if(success)
	{
		SetModified();
		UpdateAllViews(nullptr, UpdateHint().ModType());
	}
	EndWaitCursor();
	return success;

}


// Base code for adding, removing, moving and duplicating channels. Returns new number of channels on success, CHANNELINDEX_INVALID otherwise.
// The new channel vector can contain CHANNELINDEX_INVALID for adding new (empty) channels.
CHANNELINDEX CModDoc::ReArrangeChannels(const std::vector<CHANNELINDEX> &newOrder, const bool createUndoPoint)
//------------------------------------------------------------------------------------------------------------
{
	//newOrder[i] tells which current channel should be placed to i:th position in
	//the new order, or if i is not an index of current channels, then new channel is
	//added to position i. If index of some current channel is missing from the
	//newOrder-vector, then the channel gets removed.

	const CHANNELINDEX nRemainingChannels = static_cast<CHANNELINDEX>(newOrder.size());

	if(nRemainingChannels > m_SndFile.GetModSpecifications().channelsMax || nRemainingChannels < m_SndFile.GetModSpecifications().channelsMin)
	{
		CString str;
		str.Format(_T("Can't apply change: Number of channels should be between %u and %u."), m_SndFile.GetModSpecifications().channelsMin, m_SndFile.GetModSpecifications().channelsMax);
		Reporting::Error(str , "Rearrange Channels");
		return CHANNELINDEX_INVALID;
	}

	if(m_SndFile.Patterns.Size() == 0)
	{
		// Nothing to do
		return GetNumChannels();
	}

	CriticalSection cs;
	if(createUndoPoint)
	{
		PrepareUndoForAllPatterns(true, "Rearrange Channels");
	}

	for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++) 
	{
		if(m_SndFile.Patterns.IsValidPat(nPat))
		{
			ModCommand *oldPatData = m_SndFile.Patterns[nPat];
			ModCommand *newPatData = CPattern::AllocatePattern(m_SndFile.Patterns[nPat].GetNumRows(), nRemainingChannels);
			if(!newPatData)
			{
				cs.Leave();
				Reporting::Error("ERROR: Pattern allocation failed in ReArrangeChannels(...)");
				return CHANNELINDEX_INVALID;
			}
			ModCommand *tmpdest = newPatData;
			for(ROWINDEX nRow = 0; nRow < m_SndFile.Patterns[nPat].GetNumRows(); nRow++) //Scrolling rows
			{
				for(CHANNELINDEX nChn = 0; nChn < nRemainingChannels; nChn++, tmpdest++) //Scrolling channels.
				{
					if(newOrder[nChn] < GetNumChannels()) //Case: getting old channel to the new channel order.
						*tmpdest = *m_SndFile.Patterns[nPat].GetpModCommand(nRow, newOrder[nChn]);
					else //Case: figure newOrder[k] is not the index of any current channel, so adding a new channel.
						*tmpdest = ModCommand::Empty();

				}
			}
			m_SndFile.Patterns[nPat] = newPatData;
			CPattern::FreePattern(oldPatData);
		}
	}

	std::vector<ModChannel> chns(MAX_CHANNELS);
	std::vector<ModChannelSettings> settings(MAX_BASECHANNELS);
	std::vector<BYTE> recordStates(GetNumChannels(), 0);
	std::vector<bool> chnMutePendings(GetNumChannels(), false);

	for(CHANNELINDEX nChn = 0; nChn < GetNumChannels(); nChn++)
	{
		settings[nChn] = m_SndFile.ChnSettings[nChn];
		chns[nChn] = m_SndFile.m_PlayState.Chn[nChn];
		recordStates[nChn] = IsChannelRecord(nChn);
		chnMutePendings[nChn] = m_SndFile.m_bChannelMuteTogglePending[nChn];
	}

	ReinitRecordState();

	for(CHANNELINDEX nChn = 0; nChn < nRemainingChannels; nChn++)
	{
		if(newOrder[nChn] < GetNumChannels())
		{
			m_SndFile.ChnSettings[nChn] = settings[newOrder[nChn]];
			m_SndFile.m_PlayState.Chn[nChn] = chns[newOrder[nChn]];
			if(chns[newOrder[nChn]].nMasterChn > 0 && chns[newOrder[nChn]].nMasterChn <= nRemainingChannels)
			{
				m_SndFile.m_PlayState.Chn[nChn].nMasterChn = newOrder[chns[newOrder[nChn]].nMasterChn - 1] + 1;
			}
			if(recordStates[newOrder[nChn]] == 1) Record1Channel(nChn, true);
			if(recordStates[newOrder[nChn]] == 2) Record2Channel(nChn, true);
			m_SndFile.m_bChannelMuteTogglePending[nChn] = chnMutePendings[newOrder[nChn]];
		} else
		{
			m_SndFile.InitChannel(nChn);
		}
	}
	// Reset MOD panning (won't affect other module formats)
	m_SndFile.SetupMODPanning();

	m_SndFile.m_nChannels = nRemainingChannels;

	// Reset removed channels. Most notably, clear the channel name.
	for(CHANNELINDEX nChn = GetNumChannels(); nChn < MAX_BASECHANNELS; nChn++)
	{
		m_SndFile.InitChannel(nChn);
	}

	return GetNumChannels();
}


// Functor for rewriting instrument numbers in patterns.
struct RewriteInstrumentReferencesInPatterns
//==========================================
{
	RewriteInstrumentReferencesInPatterns(const std::vector<ModCommand::INSTR> &indices) : instrumentIndices(indices)
	{
	}

	void operator()(ModCommand& m)
	{
		if(!m.IsPcNote() && m.instr < instrumentIndices.size())
		{
			m.instr = instrumentIndices[m.instr];
		}
	}

	const std::vector<ModCommand::INSTR> &instrumentIndices;
};


// Base code for adding, removing, moving and duplicating samples. Returns new number of samples on success, SAMPLEINDEX_INVALID otherwise.
// The new sample vector can contain SAMPLEINDEX_INVALID for adding new (empty) samples.
// newOrder indices are zero-based, i.e. newOrder[0] will define the contents of the first sample slot.
SAMPLEINDEX CModDoc::ReArrangeSamples(const std::vector<SAMPLEINDEX> &newOrder)
//-----------------------------------------------------------------------------
{
	if(newOrder.size() > m_SndFile.GetModSpecifications().samplesMax)
	{
		return SAMPLEINDEX_INVALID;
	}

	CriticalSection cs;

	const SAMPLEINDEX oldNumSamples = m_SndFile.GetNumSamples(), newNumSamples = static_cast<SAMPLEINDEX>(newOrder.size());

	std::vector<int> sampleCount(oldNumSamples + 1, 0);
	std::vector<ModSample> sampleHeaders(oldNumSamples + 1);
	std::vector<SAMPLEINDEX> newIndex(oldNumSamples + 1, 0);	// One of the new indexes for the old sample
	std::vector<std::string> sampleNames(oldNumSamples + 1);
	std::vector<mpt::PathString> samplePaths(oldNumSamples + 1);

	for(SAMPLEINDEX i = 0; i < newNumSamples; i++)
	{
		const SAMPLEINDEX origSlot = newOrder[i];
		if(origSlot > 0 && origSlot <= oldNumSamples)
		{
			sampleCount[origSlot]++;
			sampleHeaders[origSlot] = m_SndFile.GetSample(origSlot);
			if(!newIndex[origSlot]) newIndex[origSlot] = i + 1;
		}
	}

	// First, delete all samples that will be removed anyway.
	for(SAMPLEINDEX i = 1; i < sampleCount.size(); i++)
	{
		if(sampleCount[i] == 0)
		{
			m_SndFile.DestroySample(i);
			GetSampleUndo().ClearUndo(i);
		}
		sampleNames[i] = m_SndFile.m_szNames[i];
		samplePaths[i] = m_SndFile.GetSamplePath(i);
	}

	// Remove sample data references from now unused slots.
	for(SAMPLEINDEX i = newNumSamples + 1; i <= oldNumSamples; i++)
	{
		m_SndFile.GetSample(i).pSample = nullptr;
		m_SndFile.GetSample(i).nLength = 0;
		strcpy(m_SndFile.m_szNames[i], "");
	}

	// Now, create new sample list.
	m_SndFile.m_nSamples = std::max(m_SndFile.m_nSamples, newNumSamples);	// Avoid assertions when using GetSample()...
	for(SAMPLEINDEX i = 0; i < newNumSamples; i++)
	{
		const SAMPLEINDEX origSlot = newOrder[i];
		ModSample &target = m_SndFile.GetSample(i + 1);
		if(origSlot > 0 && origSlot <= oldNumSamples)
		{
			// Copy an original sample.
			target = sampleHeaders[origSlot];
			if(--sampleCount[origSlot] > 0 && sampleHeaders[origSlot].pSample != nullptr)
			{
				// This sample slot is referenced multiple times, so we have to copy the actual sample.
				target.pSample = ModSample::AllocateSample(target.nLength, target.GetBytesPerSample());
				if(target.pSample != nullptr)
				{
					memcpy(target.pSample, sampleHeaders[origSlot].pSample, target.GetSampleSizeInBytes());
					target.PrecomputeLoops(m_SndFile, false);
				} else
				{
					Reporting::Error("Cannot duplicate sample - out of memory!");
				}
			}
			strcpy(m_SndFile.m_szNames[i + 1], sampleNames[origSlot].c_str());
			m_SndFile.SetSamplePath(i + 1, samplePaths[origSlot]);
		} else
		{
			// Invalid sample reference.
			target.Initialize(m_SndFile.GetType());
			target.pSample = nullptr;
			strcpy(m_SndFile.m_szNames[i + 1], "");
			m_SndFile.ResetSamplePath(i + 1);
		}
	}

	GetSampleUndo().RearrangeSamples(newIndex);

	for(CHANNELINDEX c = 0; c < CountOf(m_SndFile.m_PlayState.Chn); c++)
	{
		ModChannel &chn = m_SndFile.m_PlayState.Chn[c];
		for(SAMPLEINDEX i = 1; i <= oldNumSamples; i++)
		{
			if(chn.pModSample == &m_SndFile.GetSample(i))
			{
				chn.pModSample = &m_SndFile.GetSample(newIndex[i]);
				if(i == 0 || i > newNumSamples)
				{
					chn.Reset(ModChannel::resetTotal, m_SndFile, c);
				}
				break;
			}
		}
	}

	m_SndFile.m_nSamples = newNumSamples;

	if(m_SndFile.GetNumInstruments())
	{
		// Instrument mode: Update sample maps.
		for(INSTRUMENTINDEX i = 0; i <= m_SndFile.GetNumInstruments(); i++)
		{
			ModInstrument *ins = m_SndFile.Instruments[i];
			if(ins == nullptr)
			{
				continue;
			}
			for(size_t note = 0; note < CountOf(ins->Keyboard); note++)
			{
				if(ins->Keyboard[note] > 0 && ins->Keyboard[note] <= oldNumSamples)
				{
					ins->Keyboard[note] = newIndex[ins->Keyboard[note]];
				} else
				{
					ins->Keyboard[note] = 0;
				}
			}
		}
	} else
	{
		PrepareUndoForAllPatterns(false, "Rearrange Samples");

		std::vector<ModCommand::INSTR> indices(newIndex.size(), 0);
		for(size_t i = 0; i < newIndex.size(); i++)
		{
			indices[i] = newIndex[i];
		}
		m_SndFile.Patterns.ForEachModCommand(RewriteInstrumentReferencesInPatterns(indices));
	}

	return GetNumSamples();
}


// Base code for adding, removing, moving and duplicating instruments. Returns new number of instruments on success, INSTRUMENTINDEX_INVALID otherwise.
// The new instrument vector can contain INSTRUMENTINDEX_INVALID for adding new (empty) instruments.
// newOrder indices are zero-based, i.e. newOrder[0] will define the contents of the first instrument slot.
INSTRUMENTINDEX CModDoc::ReArrangeInstruments(const std::vector<INSTRUMENTINDEX> &newOrder, deleteInstrumentSamples removeSamples)
//--------------------------------------------------------------------------------------------------------------------------------
{
	if(newOrder.size() > m_SndFile.GetModSpecifications().instrumentsMax || GetNumInstruments() == 0)
	{
		return INSTRUMENTINDEX_INVALID;
	}

	CriticalSection cs;

	const INSTRUMENTINDEX oldNumInstruments = m_SndFile.GetNumInstruments(), newNumInstruments = static_cast<INSTRUMENTINDEX>(newOrder.size());

	std::vector<ModInstrument> instrumentHeaders(oldNumInstruments + 1);
	std::vector<INSTRUMENTINDEX> newIndex(oldNumInstruments + 1, 0);	// One of the new indexes for the old instrument
	for(INSTRUMENTINDEX i = 0; i < newNumInstruments; i++)
	{
		const INSTRUMENTINDEX origSlot = newOrder[i];
		if(origSlot > 0 && origSlot <= oldNumInstruments)
		{
			if(m_SndFile.Instruments[origSlot] != nullptr)
				instrumentHeaders[origSlot] = *m_SndFile.Instruments[origSlot];
			newIndex[origSlot] = i + 1;
		}
	}

	// Delete unused instruments first.
	for(INSTRUMENTINDEX i = 1; i <= oldNumInstruments; i++)
	{
		if(newIndex[i] == 0)
		{
			m_SndFile.DestroyInstrument(i, removeSamples);
		}
	}

	m_SndFile.m_nInstruments = newNumInstruments;

	// Now, create new instrument list.
	for(INSTRUMENTINDEX i = 0; i < newNumInstruments; i++)
	{
		ModInstrument *ins = m_SndFile.AllocateInstrument(i + 1);
		if(ins == nullptr)
		{
			continue;
		}

		const INSTRUMENTINDEX origSlot = newOrder[i];
		if(origSlot > 0 && origSlot <= oldNumInstruments)
		{
			// Copy an original instrument.
			*ins = instrumentHeaders[origSlot];
		}
	}

	// Free unused instruments
	for(INSTRUMENTINDEX i = newNumInstruments + 1; i <= oldNumInstruments; i++)
	{
		m_SndFile.DestroyInstrument(i, doNoDeleteAssociatedSamples);
	}

	PrepareUndoForAllPatterns(false, "Rearrange Instrumens");

	std::vector<ModCommand::INSTR> indices(newIndex.size(), 0);
	for(size_t i = 0; i < newIndex.size(); i++)
	{
		indices[i] = newIndex[i];
	}
	m_SndFile.Patterns.ForEachModCommand(RewriteInstrumentReferencesInPatterns(indices));

	return GetNumInstruments();
}


// Functor for converting instrument numbers to sample numbers in the patterns
struct ConvertInstrumentsToSamplesInPatterns
//==========================================
{
	ConvertInstrumentsToSamplesInPatterns(CSoundFile *pSndFile)
	{
		this->pSndFile = pSndFile;
	}

	void operator()(ModCommand& m)
	{
		if(m.instr && !m.IsPcNote())
		{
			ModCommand::INSTR instr = m.instr, newinstr = 0;
			ModCommand::NOTE note = m.note, newnote = note;
			if(ModCommand::IsNote(note))
				note = note - NOTE_MIN;
			else
				note = NOTE_MIDDLEC - NOTE_MIN;

			if((instr < MAX_INSTRUMENTS) && (pSndFile->Instruments[instr]))
			{
				const ModInstrument *pIns = pSndFile->Instruments[instr];
				newinstr = pIns->Keyboard[note];
				newnote = pIns->NoteMap[note];
				if(pIns->Keyboard[note] > Util::MaxValueOfType(m.instr)) newinstr = 0;
			}
			m.instr = newinstr;
			if(m.IsNote())
			{
				m.note = newnote;
			}
		}
	}

	CSoundFile *pSndFile;
};


bool CModDoc::ConvertInstrumentsToSamples()
//-----------------------------------------
{
	if (!m_SndFile.GetNumInstruments()) return false;
	m_SndFile.Patterns.ForEachModCommand(ConvertInstrumentsToSamplesInPatterns(&m_SndFile));
	return true;
}


bool CModDoc::ConvertSamplesToInstruments()
//-----------------------------------------
{
	const INSTRUMENTINDEX instrumentMax = m_SndFile.GetModSpecifications().instrumentsMax;
	if(GetNumInstruments() > 0 || instrumentMax == 0) return false;

	// If there is no actual sample data, don't bother creating any instruments
	bool anySamples = false;
	for(SAMPLEINDEX smp = 1; smp <= m_SndFile.m_nSamples; smp++)
	{
		if(m_SndFile.GetSample(smp).HasSampleData())
		{
			anySamples = true;
			break;
		}
	}
	if(!anySamples) return true;

	m_SndFile.m_nInstruments = std::min(m_SndFile.GetNumSamples(), instrumentMax);
	for(SAMPLEINDEX smp = 1; smp <= m_SndFile.m_nInstruments; smp++)
	{
		const bool muted = IsSampleMuted(smp);
		MuteSample(smp, false);

		ModInstrument *instrument = m_SndFile.AllocateInstrument(smp, smp);
		if(instrument == nullptr)
		{
			ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
			return false;
		}

		InitializeInstrument(instrument);
		mpt::String::Copy(instrument->name, m_SndFile.m_szNames[smp]);
		MuteInstrument(smp, muted);
	}

	return true;

}


PLUGINDEX CModDoc::RemovePlugs(const std::vector<bool> &keepMask)
//---------------------------------------------------------------
{
	//Remove all plugins whose keepMask[plugindex] is false.
	PLUGINDEX nRemoved = 0;
	const PLUGINDEX maxPlug = MIN(MAX_MIXPLUGINS, keepMask.size());

	for(PLUGINDEX nPlug = 0; nPlug < maxPlug; nPlug++)
	{
		SNDMIXPLUGIN &plug = m_SndFile.m_MixPlugins[nPlug];
		if(keepMask[nPlug])
		{
			continue;
		}

		if(plug.pMixPlugin || plug.IsValidPlugin())
		{
			nRemoved++;
		}

		delete[] plug.pPluginData;
		plug.pPluginData = nullptr;

		if(plug.pMixPlugin)
		{
			plug.pMixPlugin->Release();
			plug.pMixPlugin = nullptr;
		}
		MemsetZero(plug.Info);
		plug.nPluginDataSize = 0;
		plug.fDryRatio = 0;
		plug.defaultProgram = 0;
	}

	return nRemoved;
}


// Clone a plugin slot (source does not necessarily have to be from the current module)
void CModDoc::ClonePlugin(SNDMIXPLUGIN &target, const SNDMIXPLUGIN &source)
//-------------------------------------------------------------------------
{
	IMixPlugin *srcVstPlug = source.pMixPlugin;
	target.Destroy();
	target = source;
	// Don't want this stuff to be accidentally erased again...
	target.pMixPlugin = nullptr;
	target.pPluginData = nullptr;
	target.nPluginDataSize = 0;
	if(target.editorX != int32_min)
	{
		// Move target editor a bit to visually distinguish it from the original editor
		int addPixels = Util::ScalePixels(16, CMainFrame::GetMainFrame()->m_hWnd);
		target.editorX += addPixels;
		target.editorY += addPixels;
	}
#ifndef NO_PLUGINS
	if(theApp.GetPluginManager()->CreateMixPlugin(target, GetrSoundFile()))
	{
		IMixPlugin *newVstPlug = target.pMixPlugin;
		newVstPlug->SetCurrentProgram(srcVstPlug->GetCurrentProgram());

		mpt::ostringstream f(std::ios::out | std::ios::binary);
		if(VSTPresets::SaveFile(f, *srcVstPlug, false))
		{
			const std::string data = f.str();
			FileReader file(data.c_str(), data.length());
			VSTPresets::LoadFile(file, *newVstPlug);
		}
	}
#endif // !NO_PLUGINS
}


PATTERNINDEX CModDoc::InsertPattern(ORDERINDEX nOrd, ROWINDEX nRows)
//------------------------------------------------------------------
{
	const PATTERNINDEX i = m_SndFile.Patterns.Insert(nRows);
	if(i == PATTERNINDEX_INVALID)
		return i;

	//Increasing orderlist size if given order is beyond current limit,
	//or if the last order already has a pattern.
	if((nOrd == m_SndFile.Order.size() ||
		m_SndFile.Order.Last() < m_SndFile.Patterns.Size() ) &&
		m_SndFile.Order.GetLength() < m_SndFile.GetModSpecifications().ordersMax)
	{
		m_SndFile.Order.Append();
	}

	for (ORDERINDEX j = 0; j < m_SndFile.Order.size(); j++)
	{
		if (m_SndFile.Order[j] == i) break;
		if (m_SndFile.Order[j] == m_SndFile.Order.GetInvalidPatIndex() && nOrd == ORDERINDEX_INVALID)
		{
			m_SndFile.Order[j] = i;
			break;
		}
		if (j == nOrd)
		{
			for (ORDERINDEX k = m_SndFile.Order.size() - 1; k > j; k--)
			{
				m_SndFile.Order[k] = m_SndFile.Order[k - 1];
			}
			m_SndFile.Order[j] = i;
			break;
		}
	}

	SetModified();
	return i;
}


SAMPLEINDEX CModDoc::InsertSample(bool bLimit)
//--------------------------------------------
{
	SAMPLEINDEX i = m_SndFile.GetNextFreeSample();

	if ((bLimit && i >= 200 && !m_SndFile.GetNumInstruments()) || i == SAMPLEINDEX_INVALID)
	{
		ErrorBox(IDS_ERR_TOOMANYSMP, CMainFrame::GetMainFrame());
		return SAMPLEINDEX_INVALID;
	}
	const bool newSlot = (i > m_SndFile.GetNumSamples());
	if(newSlot || !m_SndFile.m_szNames[i][0]) strcpy(m_SndFile.m_szNames[i], "untitled");
	if(newSlot) m_SndFile.m_nSamples = i;
	m_SndFile.GetSample(i).Initialize(m_SndFile.GetType());
	
	m_SndFile.ResetSamplePath(i);

	SetModified();
	return i;
}


// Insert a new instrument assigned to sample nSample or duplicate instrument nDuplicate.
// If nSample is invalid, an appropriate sample slot is selected. 0 means "no sample".
INSTRUMENTINDEX CModDoc::InsertInstrument(SAMPLEINDEX nSample, INSTRUMENTINDEX nDuplicate)
//----------------------------------------------------------------------------------------
{
	if (m_SndFile.GetModSpecifications().instrumentsMax == 0) return INSTRUMENTINDEX_INVALID;

	ModInstrument *pDup = nullptr;

	if ((nDuplicate > 0) && (nDuplicate <= m_SndFile.m_nInstruments))
	{
		pDup = m_SndFile.Instruments[nDuplicate];
	}
	if ((!m_SndFile.GetNumInstruments()) && ((m_SndFile.GetNumSamples() > 1) || (m_SndFile.GetSample(1).pSample)))
	{
		if (pDup) return INSTRUMENTINDEX_INVALID;
		ConfirmAnswer result = Reporting::Confirm("Convert existing samples to instruments first?", true);
		if (result == cnfCancel)
		{
			return INSTRUMENTINDEX_INVALID;
		}
		if (result == cnfYes)
		{
			if(!ConvertSamplesToInstruments())
			{
				return INSTRUMENTINDEX_INVALID;
			}
		}
	}
	
	const INSTRUMENTINDEX newins = m_SndFile.GetNextFreeInstrument();
	if(newins == INSTRUMENTINDEX_INVALID)
	{
		ErrorBox(IDS_ERR_TOOMANYINS, CMainFrame::GetMainFrame());
		return INSTRUMENTINDEX_INVALID;
	} else if(newins > m_SndFile.GetNumInstruments())
	{
		m_SndFile.m_nInstruments = newins;
	}

	// Determine which sample slot to use
	SAMPLEINDEX newsmp = 0;
	if (nSample < m_SndFile.GetModSpecifications().samplesMax)
	{
		// Use specified slot
		newsmp = nSample;
	} else if (!pDup)
	{
		newsmp = m_SndFile.GetNextFreeSample(newins);
		if (newsmp > m_SndFile.GetNumSamples())
		{
			// Add a new sample
			const SAMPLEINDEX inssmp = InsertSample();
			if (inssmp != SAMPLEINDEX_INVALID) newsmp = inssmp;
		}
	}

	CriticalSection cs;

	ModInstrument *pIns = m_SndFile.AllocateInstrument(newins, newsmp);
	if(pIns == nullptr)
	{
		cs.Leave();
		ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
		return INSTRUMENTINDEX_INVALID;
	}
	InitializeInstrument(pIns);

	if (pDup)
	{
		*pIns = *pDup;
	}

	SetModified();

	return newins;
}


// Load default instrument values for inserting new instrument during editing
void CModDoc::InitializeInstrument(ModInstrument *pIns)
//-----------------------------------------------------
{
	pIns->nPluginVolumeHandling = TrackerSettings::Instance().DefaultPlugVolumeHandling;
}


// Try to set up a new instrument that is linked to a given plugin
INSTRUMENTINDEX CModDoc::InsertInstrumentForPlugin(PLUGINDEX plug)
//----------------------------------------------------------------
{
#ifndef NO_PLUGINS
	const bool first = (GetNumInstruments() == 0);
	if(first && !ConvertSamplesToInstruments()) return INSTRUMENTINDEX_INVALID;

	INSTRUMENTINDEX instr = m_SndFile.GetNextFreeInstrument();
	if(instr == INSTRUMENTINDEX_INVALID) return INSTRUMENTINDEX_INVALID;

	ModInstrument *ins = m_SndFile.AllocateInstrument(instr, 0);
	if(ins == nullptr) return INSTRUMENTINDEX_INVALID;
	InitializeInstrument(ins);

	_snprintf(ins->name, CountOf(ins->name) - 1, _T("%u: %s"), plug + 1, m_SndFile.m_MixPlugins[plug].GetName());
	mpt::String::Copy(ins->filename, mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, m_SndFile.m_MixPlugins[plug].GetLibraryName()));
	ins->nMixPlug = plug + 1;
	ins->nMidiChannel = 1;
	// People will forget to change this anyway, so the following lines can lead to some bad surprises after re-opening the module.
	//pIns->wMidiBank = (WORD)((m_pVstPlugin->GetCurrentProgram() >> 7) + 1);
	//pIns->nMidiProgram = (BYTE)((m_pVstPlugin->GetCurrentProgram() & 0x7F) + 1);

	if(instr > m_SndFile.m_nInstruments) m_SndFile.m_nInstruments = instr;

	InstrumentHint hint = InstrumentHint(instr).Info().Envelope().Names();
	if(first) hint.ModType();
	UpdateAllViews(nullptr, hint);
	if(m_SndFile.GetModSpecifications().supportsPlugins)
	{
		SetModified();
	}

	return instr;
#else
	return INSTRUMENTINDEX_INVALID;
#endif
}


INSTRUMENTINDEX CModDoc::HasInstrumentForPlugin(PLUGINDEX plug) const
//-------------------------------------------------------------------
{
	for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
	{
		if(m_SndFile.Instruments[i] != nullptr && m_SndFile.Instruments[i]->nMixPlug == plug + 1)
		{
			return i;
		}
	}
	return INSTRUMENTINDEX_INVALID;
}


bool CModDoc::RemoveOrder(SEQUENCEINDEX nSeq, ORDERINDEX nOrd)
//------------------------------------------------------------
{
	if (nSeq >= m_SndFile.Order.GetNumSequences() || nOrd >= m_SndFile.Order.GetSequence(nSeq).size())
		return false;

	CriticalSection cs;

	SEQUENCEINDEX nOldSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	m_SndFile.Order.SetSequence(nSeq);
	for (ORDERINDEX i = nOrd; i < m_SndFile.Order.GetSequence(nSeq).size() - 1; i++)
	{
		m_SndFile.Order[i] = m_SndFile.Order[i + 1];
	}
	m_SndFile.Order[m_SndFile.Order.GetLastIndex()] = m_SndFile.Order.GetInvalidPatIndex();
	m_SndFile.Order.SetSequence(nOldSeq);
	SetModified();

	return true;
}



bool CModDoc::RemovePattern(PATTERNINDEX nPat)
//--------------------------------------------
{
	if ((nPat < m_SndFile.Patterns.Size()) && (m_SndFile.Patterns[nPat]))
	{
		CriticalSection cs;

		m_SndFile.Patterns.Remove(nPat);
		SetModified();

		return true;
	}
	return false;
}


bool CModDoc::RemoveSample(SAMPLEINDEX nSmp)
//------------------------------------------
{
	if ((nSmp) && (nSmp <= m_SndFile.GetNumSamples()))
	{
		CriticalSection cs;

		m_SndFile.DestroySample(nSmp);
		m_SndFile.m_szNames[nSmp][0] = 0;
		while ((m_SndFile.GetNumSamples() > 1)
			&& (!m_SndFile.m_szNames[m_SndFile.GetNumSamples()][0])
			&& (!m_SndFile.GetSample(m_SndFile.GetNumSamples()).pSample))
		{
			m_SndFile.m_nSamples--;
		}
		SetModified();

		return true;
	}
	return false;
}


bool CModDoc::RemoveInstrument(INSTRUMENTINDEX nIns)
//--------------------------------------------------
{
	if ((nIns) && (nIns <= m_SndFile.GetNumInstruments()) && (m_SndFile.Instruments[nIns]))
	{
		bool instrumentsLeft = false;
		ConfirmAnswer result = cnfNo;
		if(!m_SndFile.Instruments[nIns]->GetSamples().empty()) result = Reporting::Confirm("Remove samples associated with an instrument if they are unused?", "Removing instrument", true);
		if(result == cnfCancel)
		{
			return false;
		}
		if(m_SndFile.DestroyInstrument(nIns, (result == cnfYes) ? deleteAssociatedSamples : doNoDeleteAssociatedSamples))
		{
			CriticalSection cs;
			if (nIns == m_SndFile.m_nInstruments) m_SndFile.m_nInstruments--;
			for (UINT i=1; i<MAX_INSTRUMENTS; i++) if (m_SndFile.Instruments[i]) instrumentsLeft = true;
			if (!instrumentsLeft) m_SndFile.m_nInstruments = 0;
			SetModified();

			return true;
		}
	}
	return false;
}


bool CModDoc::MoveOrder(ORDERINDEX nSourceNdx, ORDERINDEX nDestNdx, bool bUpdate, bool bCopy, SEQUENCEINDEX nSourceSeq, SEQUENCEINDEX nDestSeq)
//---------------------------------------------------------------------------------------------------------------------------------------------
{
	if (MAX(nSourceNdx, nDestNdx) >= m_SndFile.Order.size()) return false;
	if (nDestNdx >= m_SndFile.GetModSpecifications().ordersMax) return false;

	if(nSourceSeq == SEQUENCEINDEX_INVALID) nSourceSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	if(nDestSeq == SEQUENCEINDEX_INVALID) nDestSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	if (MAX(nSourceSeq, nDestSeq) >= m_SndFile.Order.GetNumSequences()) return false;
	PATTERNINDEX nSourcePat = m_SndFile.Order.GetSequence(nSourceSeq)[nSourceNdx];

	// save current working sequence
	SEQUENCEINDEX nWorkingSeq = m_SndFile.Order.GetCurrentSequenceIndex();

	// Delete source
	if (!bCopy)
	{
		m_SndFile.Order.SetSequence(nSourceSeq);
		m_SndFile.Order.Remove(nSourceNdx, nSourceNdx);
		if (nSourceNdx < nDestNdx && nSourceSeq == nDestSeq) nDestNdx--;
	}
	// Insert at dest
	m_SndFile.Order.SetSequence(nDestSeq);
	m_SndFile.Order.Insert(nDestNdx, 1, nSourcePat);

	if (bUpdate)
	{
		UpdateAllViews(nullptr, SequenceHint().Data());
	}

	m_SndFile.Order.SetSequence(nWorkingSeq);
	return true;
}


BOOL CModDoc::ExpandPattern(PATTERNINDEX nPattern)
//------------------------------------------------
{
	ROWINDEX numRows;

	if(!m_SndFile.Patterns.IsValidPat(nPattern)
		|| (numRows = m_SndFile.Patterns[nPattern].GetNumRows()) > m_SndFile.GetModSpecifications().patternRowsMax / 2)
	{
		return false;
	}

	BeginWaitCursor();
	CriticalSection cs;
	GetPatternUndo().PrepareUndo(nPattern, 0, 0, GetNumChannels(), numRows, "Expand Pattern");
	bool success = m_SndFile.Patterns[nPattern].Expand();
	cs.Leave();
	EndWaitCursor();

	if(success)
	{
		SetModified();
		UpdateAllViews(NULL, PatternHint(nPattern).Data(), NULL);
	} else
	{
		GetPatternUndo().RemoveLastUndoStep();
	}
	return success;
}


BOOL CModDoc::ShrinkPattern(PATTERNINDEX nPattern)
//------------------------------------------------
{
	ROWINDEX numRows;

	if(!m_SndFile.Patterns.IsValidPat(nPattern)
		|| (numRows = m_SndFile.Patterns[nPattern].GetNumRows()) < m_SndFile.GetModSpecifications().patternRowsMin * 2)
	{
		return false;
	}

	BeginWaitCursor();
	CriticalSection cs;
	GetPatternUndo().PrepareUndo(nPattern, 0, 0, GetNumChannels(), numRows, "Shrink Pattern");
	bool success = m_SndFile.Patterns[nPattern].Shrink();
	cs.Leave();
	EndWaitCursor();

	if(success)
	{
		SetModified();
		UpdateAllViews(NULL, PatternHint(nPattern).Data(), NULL);
	} else
	{
		GetPatternUndo().RemoveLastUndoStep();
	}
	return success;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Copy/Paste envelope

static const CHAR *pszEnvHdr = "ModPlug Tracker Envelope\r\n";
static const CHAR *pszEnvFmt = "%d,%d,%d,%d,%d,%d,%d,%d\r\n";

static bool EnvelopeToString(CStringA &s, const InstrumentEnvelope &env)
//----------------------------------------------------------------------
{
	// We don't want to copy empty envelopes
	if(env.nNodes == 0)
	{
		return false;
	}

	s.Preallocate(2048);
	s = pszEnvHdr;
	s.AppendFormat(pszEnvFmt, env.nNodes, env.nSustainStart, env.nSustainEnd, env.nLoopStart, env.nLoopEnd,
		env.dwFlags[ENV_SUSTAIN] ? 1 : 0, env.dwFlags[ENV_LOOP] ? 1 : 0, env.dwFlags[ENV_CARRY] ? 1 : 0);
	for(uint32 i = 0; i < env.nNodes; i++)
	{
		s.AppendFormat("%d,%d\r\n", env.Ticks[i], env.Values[i]);
	}

	// Writing release node
	s.AppendFormat("%u\r\n", env.nReleaseNode);
	return true;
}


static bool StringToEnvelope(const std::string &s, InstrumentEnvelope &env, const CModSpecifications &specs)
//----------------------------------------------------------------------------------------------------------
{
	uint32 susBegin = 0, susEnd = 0, loopBegin = 0, loopEnd = 0, bSus = 0, bLoop = 0, bCarry = 0, nPoints = 0, releaseNode = ENV_RELEASE_NODE_UNSET;
	size_t length = s.size(), pos = strlen(pszEnvHdr);
	if (length <= pos || mpt::strnicmp(s.c_str(), pszEnvHdr, pos - 2))
	{
		return false;
	}
	sscanf(&s[pos], pszEnvFmt, &nPoints, &susBegin, &susEnd, &loopBegin, &loopEnd, &bSus, &bLoop, &bCarry);
	while (pos < length && s[pos] != '\r' && s[pos] != '\n') pos++;

	nPoints = MIN(nPoints, specs.envelopePointsMax);
	if (susEnd >= nPoints) susEnd = 0;
	if (susBegin > susEnd) susBegin = susEnd;
	if (loopEnd >= nPoints) loopEnd = 0;
	if (loopBegin > loopEnd) loopBegin = loopEnd;

	env.nNodes = nPoints;
	env.nSustainStart = susBegin;
	env.nSustainEnd = susEnd;
	env.nLoopStart = loopBegin;
	env.nLoopEnd = loopEnd;
	env.nReleaseNode = releaseNode;
	env.dwFlags.set(ENV_LOOP, bLoop != 0);
	env.dwFlags.set(ENV_SUSTAIN, bSus != 0);
	env.dwFlags.set(ENV_CARRY, bCarry != 0);
	env.dwFlags.set(ENV_ENABLED, nPoints > 0);

	int oldn = 0;
	for (uint32 i = 0; i < nPoints; i++)
	{
		while (pos < length && (s[pos] < '0' || s[pos] > '9')) pos++;
		if (pos >= length) break;
		int n1 = atoi(&s[pos]);
		while (pos < length && s[pos] != ',') pos++;
		while (pos < length && (s[pos] < '0' || s[pos] > '9')) pos++;
		if (pos >= length) break;
		int n2 = atoi(&s[pos]);
		if (n1 < oldn) n1 = oldn + 1;
		Limit(n2, ENVELOPE_MIN, ENVELOPE_MAX);
		env.Ticks[i] = (uint16)n1;
		env.Values[i] = (uint8)n2;
		oldn = n1;
		while (pos < length && s[pos] != '\r' && s[pos] != '\n') pos++;
		if (pos >= length) break;
	}
	env.Ticks[0] = 0;

	// Read release node information.
	env.nReleaseNode = ENV_RELEASE_NODE_UNSET;
	if(pos < length)
	{
		uint8 r = static_cast<uint8>(atoi(&s[pos]));
		if(r == 0 || r >= nPoints || !specs.hasReleaseNode)
			r = ENV_RELEASE_NODE_UNSET;
		env.nReleaseNode = r;
	}
	return true;
}


bool CModDoc::CopyEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv)
//-----------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	HANDLE hCpy;
	DWORD dwMemSize;

	if ((nIns < 1) || (nIns > m_SndFile.m_nInstruments) || (!m_SndFile.Instruments[nIns]) || (!pMainFrm)) return false;
	BeginWaitCursor();
	const ModInstrument *pIns = m_SndFile.Instruments[nIns];
	if(pIns == nullptr) return false;
	
	CStringA s;
	EnvelopeToString(s, pIns->GetEnvelope(nEnv));

	dwMemSize = s.GetLength() + 1;
	if ((pMainFrm->OpenClipboard()) && ((hCpy = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, dwMemSize))!=NULL))
	{
		EmptyClipboard();
		LPBYTE p = (LPBYTE)GlobalLock(hCpy);
		if(p != nullptr)
		{
			memcpy(p, s.GetString(), dwMemSize);
		}
		GlobalUnlock(hCpy);
		SetClipboardData (CF_TEXT, (HANDLE)hCpy);
		CloseClipboard();
	}
	EndWaitCursor();
	return true;
}


bool CModDoc::SaveEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv, const mpt::PathString &fileName)
//--------------------------------------------------------------------------------------------------
{
	if (nIns < 1 || nIns > m_SndFile.m_nInstruments || !m_SndFile.Instruments[nIns]) return false;
	BeginWaitCursor();
	const ModInstrument *pIns = m_SndFile.Instruments[nIns];
	if(pIns == nullptr) return false;
	
	CStringA s;
	EnvelopeToString(s, pIns->GetEnvelope(nEnv));

	FILE *f = mpt_fopen(fileName, "wb");
	if(f == nullptr)
	{
		EndWaitCursor();
		return false;
	}
	fwrite(s.GetString(), s.GetLength(), 1, f);
	fclose(f);
	EndWaitCursor();
	return true;
}


bool CModDoc::PasteEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv)
//----------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (nIns < 1 || nIns > m_SndFile.m_nInstruments || !m_SndFile.Instruments[nIns] || !pMainFrm) return false;
	BeginWaitCursor();
	if (!pMainFrm->OpenClipboard())
	{
		EndWaitCursor();
		return false;
	}
	HGLOBAL hCpy = ::GetClipboardData(CF_TEXT);
	LPCSTR p;
	bool result = false;
	if ((hCpy) && ((p = (LPSTR)GlobalLock(hCpy)) != nullptr))
	{
		std::string data(p, p + GlobalSize(hCpy));
		GlobalUnlock(hCpy);
		CloseClipboard();

		result = StringToEnvelope(data, m_SndFile.Instruments[nIns]->GetEnvelope(nEnv), m_SndFile.GetModSpecifications());
	}
	EndWaitCursor();
	return result;
}


bool CModDoc::LoadEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv, const mpt::PathString &fileName)
//--------------------------------------------------------------------------------------------------
{
	InputFile f(fileName);
	if (nIns < 1 || nIns > m_SndFile.m_nInstruments || !m_SndFile.Instruments[nIns] || !f.IsValid()) return false;
	BeginWaitCursor();
	FileReader file = GetFileReader(f);
	std::string data;
	file.ReadNullString(data, 1 << 16);
	bool result = StringToEnvelope(data, m_SndFile.Instruments[nIns]->GetEnvelope(nEnv), m_SndFile.GetModSpecifications());
	EndWaitCursor();
	return result;
}


// Check which channels contain note data. maxRemoveCount specified how many empty channels are reported at max.
void CModDoc::CheckUsedChannels(std::vector<bool> &usedMask, CHANNELINDEX maxRemoveCount) const
//---------------------------------------------------------------------------------------------
{
	// Checking for unused channels
	const int nChannels = GetNumChannels();
	usedMask.resize(nChannels);
	for(int iRst = nChannels - 1; iRst >= 0; iRst--)
	{
		usedMask[iRst] = !IsChannelUnused(iRst);
		if(!usedMask[iRst])
		{
			// Found enough empty channels yet?
			if((--maxRemoveCount) == 0) break;
		}
	}
}


// Check if a given channel contains note data.
bool CModDoc::IsChannelUnused(CHANNELINDEX nChn) const
//----------------------------------------------------
{
	const CHANNELINDEX nChannels = GetNumChannels();
	if(nChn >= nChannels)
	{
		return true;
	}
	for(PATTERNINDEX nPat = 0; nPat < m_SndFile.Patterns.Size(); nPat++)
	{
		if(m_SndFile.Patterns.IsValidPat(nPat))
		{
			const ModCommand *p = m_SndFile.Patterns[nPat] + nChn;
			for(ROWINDEX nRow = m_SndFile.Patterns[nPat].GetNumRows(); nRow > 0; nRow--, p += nChannels)
			{
				if(!p->IsEmpty())
				{
					return false;
				}
			}
		}
	}
	return true;
}


// Convert module's default global volume to a pattern command.
bool CModDoc::GlobalVolumeToPattern()
//-----------------------------------
{
	bool result = false;
	if(m_SndFile.GetModSpecifications().HasCommand(CMD_GLOBALVOLUME))
	{
		for(ORDERINDEX i = 0; i < m_SndFile.Order.GetLength(); i++)
		{
			if(m_SndFile.Patterns[m_SndFile.Order[i]].WriteEffect(EffectWriter(CMD_GLOBALVOLUME, m_SndFile.m_nDefaultGlobalVolume * 64 / MAX_GLOBAL_VOLUME).Retry(EffectWriter::rmTryNextRow)))
			{
				result = true;
				break;
			}
		}
	}

	m_SndFile.m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	return result;
}


OPENMPT_NAMESPACE_END
