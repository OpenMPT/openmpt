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
#include "Clipboard.h"
#include "dlg_misc.h"
#include "Dlsbank.h"
#include "../soundlib/modsmp_ctrl.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/tuning.h"
#include "../soundlib/OPL.h"
#include "../common/misc_util.h"
#include "../common/mptStringBuffer.h"
#include "mpt/io_file/inputfile.hpp"
#include "mpt/io_file_read/inputfile_filecursor.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "../common/mptFileIO.h"
#include <sstream>
// Plugin cloning
#include "../soundlib/plugins/PluginManager.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "VstPresets.h"
#include "../common/FileReader.h"
#include "mpt/io/io.hpp"
#include "mpt/io/io_stdstream.hpp"


OPENMPT_NAMESPACE_BEGIN


// Change the number of channels.
// Return true on success.
bool CModDoc::ChangeNumChannels(CHANNELINDEX nNewChannels, const bool showCancelInRemoveDlg)
{
	const CHANNELINDEX maxChans = m_SndFile.GetModSpecifications().channelsMax;

	if(nNewChannels > maxChans)
	{
		CString error;
		error.Format(_T("Error: Max number of channels for this file type is %u"), maxChans);
		Reporting::Warning(error);
		return false;
	}

	if(nNewChannels == GetNumChannels())
		return false;

	if(nNewChannels < GetNumChannels())
	{
		// Remove channels
		CHANNELINDEX chnsToRemove = 0, maxRemoveCount = 0;

		//nNewChannels = 0 means user can choose how many channels to remove
		if(nNewChannels > 0)
		{
			chnsToRemove = GetNumChannels() - nNewChannels;
			maxRemoveCount = chnsToRemove;
		} else
		{
			chnsToRemove = 0;
			maxRemoveCount = GetNumChannels();
		}

		CRemoveChannelsDlg rem(m_SndFile, chnsToRemove, showCancelInRemoveDlg);
		CheckUsedChannels(rem.m_bKeepMask, maxRemoveCount);
		if(rem.DoModal() != IDOK)
			return false;

		// Removing selected channels
		return RemoveChannels(rem.m_bKeepMask, true);
	} else
	{
		// Increasing number of channels
		BeginWaitCursor();
		std::vector<CHANNELINDEX> channels(nNewChannels, CHANNELINDEX_INVALID);
		for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
		{
			channels[chn] = chn;
		}

		bool success = (ReArrangeChannels(channels) == nNewChannels);
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
bool CModDoc::RemoveChannels(const std::vector<bool> &keepMask, bool verbose)
{
	CHANNELINDEX nRemainingChannels = 0;
	//First calculating how many channels are to be left
	for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
	{
		if(keepMask[chn])
			nRemainingChannels++;
	}
	if(nRemainingChannels == GetNumChannels() || nRemainingChannels < m_SndFile.GetModSpecifications().channelsMin)
	{
		if(verbose)
		{
			CString str;
			if(nRemainingChannels == GetNumChannels())
				str = _T("No channels chosen to be removed.");
			else
				str = _T("No removal done - channel number is already at minimum.");
			Reporting::Information(str, _T("Remove Channels"));
		}
		return false;
	}

	BeginWaitCursor();
	// Create new channel order, with only channels from m_bChnMask left.
	std::vector<CHANNELINDEX> channels(nRemainingChannels, 0);
	CHANNELINDEX i = 0;
	for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
	{
		if(keepMask[chn])
		{
			channels[i++] = chn;
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
{
	//newOrder[i] tells which current channel should be placed to i:th position in
	//the new order, or if i is not an index of current channels, then new channel is
	//added to position i. If index of some current channel is missing from the
	//newOrder-vector, then the channel gets removed.

	const CHANNELINDEX newNumChannels = static_cast<CHANNELINDEX>(newOrder.size()), oldNumChannels = GetNumChannels();
	auto &specs = m_SndFile.GetModSpecifications();

	if(newNumChannels > specs.channelsMax || newNumChannels < specs.channelsMin)
	{
		CString str;
		str.Format(_T("Can't apply change: Number of channels should be between %u and %u."), specs.channelsMin, specs.channelsMax);
		Reporting::Error(str, _T("Rearrange Channels"));
		return CHANNELINDEX_INVALID;
	}

	if(createUndoPoint)
	{
		PrepareUndoForAllPatterns(true, "Rearrange Channels");
	}

	CriticalSection cs;
	if(oldNumChannels == newNumChannels)
	{
		// Optimization with no pattern re-allocation
		std::vector<ModCommand> oldRow(oldNumChannels);
		for(auto &pat : m_SndFile.Patterns)
		{
			auto m = pat.begin();
			for(ROWINDEX row = 0; row < pat.GetNumRows(); row++)
			{
				oldRow.assign(m, m + oldNumChannels);
				for(CHANNELINDEX chn = 0; chn < newNumChannels; chn++, m++)
				{
					if(newOrder[chn] < oldNumChannels)  // Case: getting old channel to the new channel order.
						*m = oldRow[newOrder[chn]];
					else
						*m = ModCommand::Empty();
				}
			}
		}
	} else
	{
		// Create all patterns first so that we can exit cleanly in case of OOM
		std::vector<std::vector<ModCommand>> newPatterns;
		try
		{
			newPatterns.resize(m_SndFile.Patterns.Size());
			for(PATTERNINDEX i = 0; i < m_SndFile.Patterns.Size(); i++)
			{
				newPatterns[i].resize(m_SndFile.Patterns[i].GetNumRows() * newNumChannels);
			}
		} catch(mpt::out_of_memory e)
		{
			mpt::delete_out_of_memory(e);
			Reporting::Error("Out of memory!", "Rearrange Channels");
			return oldNumChannels;
		}

		m_SndFile.m_nChannels = newNumChannels;
		for(PATTERNINDEX i = 0; i < m_SndFile.Patterns.Size(); i++)
		{
			CPattern &pat = m_SndFile.Patterns[i];
			if(pat.IsValid())
			{
				auto mNew = newPatterns[i].begin(), mOld = pat.begin();
				for(ROWINDEX row = 0; row < pat.GetNumRows(); row++, mOld += oldNumChannels)
				{
					for(CHANNELINDEX chn = 0; chn < newNumChannels; chn++, mNew++)
					{
						if(newOrder[chn] < oldNumChannels)  // Case: getting old channel to the new channel order.
							*mNew = mOld[newOrder[chn]];
					}
				}
				pat.SetData(std::move(newPatterns[i]));
			}
		}
	}

	// Reverse mapping: One of the new indices of an old channel
	std::vector<CHANNELINDEX> newIndex(oldNumChannels, CHANNELINDEX_INVALID);
	for(CHANNELINDEX chn = 0; chn < newNumChannels; chn++)
	{
		if(newOrder[chn] < oldNumChannels)
		{
			newIndex[newOrder[chn]] = chn;
		}
	}

	// Reassign NNA channels (note: if we increase the number of channels, the lowest-indexed NNA channels will still be lost)
	const auto muteFlag = CSoundFile::GetChannelMuteFlag();
	for(CHANNELINDEX chn = oldNumChannels; chn < MAX_CHANNELS; chn++)
	{
		auto &channel = m_SndFile.m_PlayState.Chn[chn];
		CHANNELINDEX masterChn = channel.nMasterChn, newMasterChn;
		if(masterChn > 0 && masterChn <= newIndex.size() && (newMasterChn = newIndex[masterChn - 1]) != CHANNELINDEX_INVALID)
			channel.nMasterChn = newMasterChn + 1;
		else
			channel.Reset(ModChannel::resetTotal, m_SndFile, chn, muteFlag);
	}

	std::vector<ModChannel> chns(std::begin(m_SndFile.m_PlayState.Chn), std::begin(m_SndFile.m_PlayState.Chn) + oldNumChannels);
	std::vector<ModChannelSettings> settings(std::begin(m_SndFile.ChnSettings), std::begin(m_SndFile.ChnSettings) + oldNumChannels);
	std::vector<RecordGroup> recordStates(oldNumChannels);
	auto chnMutePendings = m_SndFile.m_bChannelMuteTogglePending;
	for(CHANNELINDEX chn = 0; chn < oldNumChannels; chn++)
	{
		recordStates[chn] = GetChannelRecordGroup(chn);
	}
	ReinitRecordState();

	for(CHANNELINDEX chn = 0; chn < newNumChannels; chn++)
	{
		CHANNELINDEX srcChn = newOrder[chn];
		if(srcChn < oldNumChannels)
		{
			m_SndFile.ChnSettings[chn] = settings[srcChn];
			m_SndFile.m_PlayState.Chn[chn] = chns[srcChn];
			SetChannelRecordGroup(chn, recordStates[srcChn]);
			m_SndFile.m_bChannelMuteTogglePending[chn] = chnMutePendings[srcChn];
			if(m_SndFile.m_opl)
				m_SndFile.m_opl->MoveChannel(srcChn, chn);
		} else
		{
			m_SndFile.InitChannel(chn);
			SetDefaultChannelColors(chn);
		}
	}
	// Reset MOD panning (won't affect other module formats)
	m_SndFile.SetupMODPanning();
	m_SndFile.InitAmigaResampler();
	return newNumChannels;
}


// Base code for adding, removing, moving and duplicating samples. Returns new number of samples on success, SAMPLEINDEX_INVALID otherwise.
// The new sample vector can contain SAMPLEINDEX_INVALID for adding new (empty) samples.
// newOrder indices are zero-based, i.e. newOrder[0] will define the contents of the first sample slot.
SAMPLEINDEX CModDoc::ReArrangeSamples(const std::vector<SAMPLEINDEX> &newOrder)
{
	if(newOrder.size() > m_SndFile.GetModSpecifications().samplesMax)
	{
		return SAMPLEINDEX_INVALID;
	}

	CriticalSection cs;

	const SAMPLEINDEX oldNumSamples = m_SndFile.GetNumSamples(), newNumSamples = static_cast<SAMPLEINDEX>(newOrder.size());

	std::vector<int> sampleCount(oldNumSamples + 1, 0);
	std::vector<ModSample> sampleHeaders(oldNumSamples + 1);
	std::vector<SAMPLEINDEX> newIndex(oldNumSamples + 1, 0);  // One of the new indexes for the old sample
	std::vector<std::string> sampleNames(oldNumSamples + 1);
	std::vector<mpt::PathString> samplePaths(oldNumSamples + 1);

	for(SAMPLEINDEX i = 0; i < newNumSamples; i++)
	{
		const SAMPLEINDEX origSlot = newOrder[i];
		if(origSlot > 0 && origSlot <= oldNumSamples)
		{
			sampleCount[origSlot]++;
			sampleHeaders[origSlot] = m_SndFile.GetSample(origSlot);
			if(!newIndex[origSlot])
				newIndex[origSlot] = i + 1;
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
		m_SndFile.GetSample(i).pData.pSample = nullptr;
		m_SndFile.GetSample(i).nLength = 0;
		m_SndFile.m_szNames[i] = "";
	}

	// Now, create new sample list.
	m_SndFile.m_nSamples = std::max(m_SndFile.m_nSamples, newNumSamples);  // Avoid assertions when using GetSample()...
	for(SAMPLEINDEX i = 0; i < newNumSamples; i++)
	{
		const SAMPLEINDEX origSlot = newOrder[i];
		ModSample &target = m_SndFile.GetSample(i + 1);
		if(origSlot > 0 && origSlot <= oldNumSamples)
		{
			// Copy an original sample.
			target = sampleHeaders[origSlot];
			if(--sampleCount[origSlot] > 0 && sampleHeaders[origSlot].HasSampleData())
			{
				// This sample slot is referenced multiple times, so we have to copy the actual sample.
				if(target.CopyWaveform(sampleHeaders[origSlot]))
				{
					target.PrecomputeLoops(m_SndFile, false);
				} else
				{
					Reporting::Error("Cannot duplicate sample - out of memory!");
				}
			}
			m_SndFile.m_szNames[i + 1] = sampleNames[origSlot];
			m_SndFile.SetSamplePath(i + 1, samplePaths[origSlot]);
		} else
		{
			// Invalid sample reference.
			target.pData.pSample = nullptr;
			target.Initialize(m_SndFile.GetType());
			m_SndFile.m_szNames[i + 1] = "";
			m_SndFile.ResetSamplePath(i + 1);
		}
	}

	GetSampleUndo().RearrangeSamples(newIndex);

	const auto muteFlag = CSoundFile::GetChannelMuteFlag();
	for(CHANNELINDEX c = 0; c < std::size(m_SndFile.m_PlayState.Chn); c++)
	{
		ModChannel &chn = m_SndFile.m_PlayState.Chn[c];
		for(SAMPLEINDEX i = 1; i <= oldNumSamples; i++)
		{
			if(chn.pModSample == &m_SndFile.GetSample(i))
			{
				chn.pModSample = &m_SndFile.GetSample(newIndex[i]);
				if(i == 0 || i > newNumSamples)
				{
					chn.Reset(ModChannel::resetTotal, m_SndFile, c, muteFlag);
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
			GetInstrumentUndo().RearrangeSamples(i, newIndex);
			for(auto &sample : ins->Keyboard)
			{
				if(sample < newIndex.size())
					sample = newIndex[sample];
				else
					sample = 0;
			}
		}
	} else
	{
		PrepareUndoForAllPatterns(false, "Rearrange Samples");
		m_SndFile.Patterns.ForEachModCommand([&newIndex] (ModCommand &m)
		{
			if(!m.IsPcNote() && m.instr < newIndex.size())
			{
				m.instr = static_cast<ModCommand::INSTR>(newIndex[m.instr]);
			}
		});
	}

	return GetNumSamples();
}


// Base code for adding, removing, moving and duplicating instruments. Returns new number of instruments on success, INSTRUMENTINDEX_INVALID otherwise.
// The new instrument vector can contain INSTRUMENTINDEX_INVALID for adding new (empty) instruments.
// newOrder indices are zero-based, i.e. newOrder[0] will define the contents of the first instrument slot.
INSTRUMENTINDEX CModDoc::ReArrangeInstruments(const std::vector<INSTRUMENTINDEX> &newOrder, deleteInstrumentSamples removeSamples)
{
	if(newOrder.size() > m_SndFile.GetModSpecifications().instrumentsMax || GetNumInstruments() == 0)
	{
		return INSTRUMENTINDEX_INVALID;
	}

	CriticalSection cs;

	const INSTRUMENTINDEX oldNumInstruments = m_SndFile.GetNumInstruments(), newNumInstruments = static_cast<INSTRUMENTINDEX>(newOrder.size());

	std::vector<ModInstrument> instrumentHeaders(oldNumInstruments + 1);
	std::vector<INSTRUMENTINDEX> newIndex(oldNumInstruments + 1, 0);  // One of the new indexes for the old instrument
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

	PrepareUndoForAllPatterns(false, "Rearrange Instruments");
	GetInstrumentUndo().RearrangeInstruments(newIndex);
	m_SndFile.Patterns.ForEachModCommand([&newIndex] (ModCommand &m)
	{
		if(!m.IsPcNote() && m.instr < newIndex.size())
		{
			m.instr = static_cast<ModCommand::INSTR>(newIndex[m.instr]);
		}
	});

	return GetNumInstruments();
}


SEQUENCEINDEX CModDoc::ReArrangeSequences(const std::vector<SEQUENCEINDEX> &newOrder)
{
	CriticalSection cs;
	return m_SndFile.Order.Rearrange(newOrder) ? m_SndFile.Order.GetNumSequences() : SEQUENCEINDEX_INVALID;
}


bool CModDoc::ConvertInstrumentsToSamples()
{
	if(!m_SndFile.GetNumInstruments())
		return false;
	GetInstrumentUndo().ClearUndo();
	m_SndFile.Patterns.ForEachModCommand([&] (ModCommand &m)
	{
		if(m.instr && !m.IsPcNote())
		{
			ModCommand::INSTR instr = m.instr, newinstr = 0;
			ModCommand::NOTE note = m.note, newnote = note;
			if(ModCommand::IsNote(note))
				note = note - NOTE_MIN;
			else
				note = NOTE_MIDDLEC - NOTE_MIN;

			if((instr < MAX_INSTRUMENTS) && (m_SndFile.Instruments[instr]))
			{
				const ModInstrument *pIns = m_SndFile.Instruments[instr];
				newinstr = static_cast<ModCommand::INSTR>(pIns->Keyboard[note]);
				newnote = pIns->NoteMap[note];
				if(pIns->Keyboard[note] > Util::MaxValueOfType(m.instr))
					newinstr = 0;
			}
			m.instr = newinstr;
			if(m.IsNote())
			{
				m.note = newnote;
			}
		}
	});
	return true;
}


bool CModDoc::ConvertSamplesToInstruments()
{
	const INSTRUMENTINDEX instrumentMax = m_SndFile.GetModSpecifications().instrumentsMax;
	if(GetNumInstruments() > 0 || instrumentMax == 0)
		return false;

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
	if(!anySamples)
		return true;

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
		instrument->name = m_SndFile.m_szNames[smp];
		MuteInstrument(smp, muted);
	}

	return true;
}


PLUGINDEX CModDoc::RemovePlugs(const std::vector<bool> &keepMask)
{
	// Remove all plugins whose keepMask[plugindex] is false.
	PLUGINDEX nRemoved = 0;
	const PLUGINDEX maxPlug = std::min(MAX_MIXPLUGINS, static_cast<PLUGINDEX>(keepMask.size()));

	CriticalSection cs;
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

		plug.Destroy();
		plug = {};

		for(PLUGINDEX srcPlugSlot = 0; srcPlugSlot < nPlug; srcPlugSlot++)
		{
			SNDMIXPLUGIN &srcPlug = GetSoundFile().m_MixPlugins[srcPlugSlot];
			if(srcPlug.GetOutputPlugin() == nPlug)
			{
				srcPlug.SetOutputToMaster();
				UpdateAllViews(nullptr, PluginHint(static_cast<PLUGINDEX>(srcPlugSlot + 1)).Info());
			}
		}
		UpdateAllViews(nullptr, PluginHint(static_cast<PLUGINDEX>(nPlug + 1)).Info().Names());
	}

	if(nRemoved && m_SndFile.GetModSpecifications().supportsPlugins)
		SetModified();

	return nRemoved;
}


bool CModDoc::RemovePlugin(PLUGINDEX plugin)
{
	if(plugin >= MAX_MIXPLUGINS)
		return false;
	std::vector<bool> keepMask(MAX_MIXPLUGINS, true);
	keepMask[plugin] = false;
	return RemovePlugs(keepMask) == 1;
}


// Clone a plugin slot (source does not necessarily have to be from the current module)
void CModDoc::ClonePlugin(SNDMIXPLUGIN &target, const SNDMIXPLUGIN &source)
{
	IMixPlugin *srcVstPlug = source.pMixPlugin;
	target.Destroy();
	target = source;
	// Don't want this plugin to be accidentally erased again...
	target.pMixPlugin = nullptr;
	if(target.editorX != int32_min)
	{
		// Move target editor a bit to visually distinguish it from the original editor
		int addPixels = Util::ScalePixels(16, CMainFrame::GetMainFrame()->m_hWnd);
		target.editorX += addPixels;
		target.editorY += addPixels;
	}
#ifndef NO_PLUGINS
	if(theApp.GetPluginManager()->CreateMixPlugin(target, GetSoundFile()))
	{
		IMixPlugin *newVstPlug = target.pMixPlugin;
		newVstPlug->SetCurrentProgram(srcVstPlug->GetCurrentProgram());

		std::ostringstream f(std::ios::out | std::ios::binary);
		if(VSTPresets::SaveFile(f, *srcVstPlug, false))
		{
			const std::string data = f.str();
			FileReader file(mpt::as_span(data));
			VSTPresets::LoadFile(file, *newVstPlug);
		}
	}
#endif // !NO_PLUGINS
}


PATTERNINDEX CModDoc::InsertPattern(ROWINDEX rows, ORDERINDEX ord)
{
	if(ord != ORDERINDEX_INVALID && m_SndFile.Order().GetLengthTailTrimmed() >= m_SndFile.GetModSpecifications().ordersMax)
		return PATTERNINDEX_INVALID;

	PATTERNINDEX pat = m_SndFile.Patterns.InsertAny(rows, true);
	if(pat != PATTERNINDEX_INVALID)
	{
		if(ord != ORDERINDEX_INVALID)
		{
			m_SndFile.Order().insert(ord, 1, pat);
		}
		SetModified();
	}
	return pat;
}


SAMPLEINDEX CModDoc::InsertSample()
{
	SAMPLEINDEX i = m_SndFile.GetNextFreeSample();

	if((i > std::numeric_limits<ModCommand::INSTR>::max() && !m_SndFile.GetNumInstruments()) || i == SAMPLEINDEX_INVALID)
	{
		ErrorBox(IDS_ERR_TOOMANYSMP, CMainFrame::GetMainFrame());
		return SAMPLEINDEX_INVALID;
	}
	const bool newSlot = (i > m_SndFile.GetNumSamples());
	if(newSlot || !m_SndFile.m_szNames[i][0])
		m_SndFile.m_szNames[i] = "untitled";
	if(newSlot)
		m_SndFile.m_nSamples = i;
	m_SndFile.GetSample(i).Initialize(m_SndFile.GetType());

	m_SndFile.ResetSamplePath(i);

	SetModified();
	return i;
}


// Insert a new instrument assigned to sample nSample or duplicate instrument "duplicateSource".
// If "sample" is invalid, an appropriate sample slot is selected. 0 means "no sample".
INSTRUMENTINDEX CModDoc::InsertInstrument(SAMPLEINDEX sample, INSTRUMENTINDEX duplicateSource, bool silent)
{
	if(m_SndFile.GetModSpecifications().instrumentsMax == 0)
		return INSTRUMENTINDEX_INVALID;

	ModInstrument *pDup = nullptr;
	if(duplicateSource > 0 && duplicateSource <= m_SndFile.m_nInstruments)
	{
		pDup = m_SndFile.Instruments[duplicateSource];
	}

	if(!m_SndFile.GetNumInstruments() && (m_SndFile.GetNumSamples() > 1 || m_SndFile.GetSample(1).HasSampleData()))
	{
		bool doConvert = true;
		if(!silent)
		{
			ConfirmAnswer result = Reporting::Confirm("Convert existing samples to instruments first?", true);
			if(result == cnfCancel)
			{
				return INSTRUMENTINDEX_INVALID;
			}
			doConvert = (result == cnfYes);
		}
		if(doConvert)
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
		if(!silent)
		{
			ErrorBox(IDS_ERR_TOOMANYINS, CMainFrame::GetMainFrame());
		}
		return INSTRUMENTINDEX_INVALID;
	} else if(newins > m_SndFile.GetNumInstruments())
	{
		m_SndFile.m_nInstruments = newins;
	}

	// Determine which sample slot to use
	SAMPLEINDEX newsmp = 0;
	if(sample < m_SndFile.GetModSpecifications().samplesMax)
	{
		// Use specified slot
		newsmp = sample;
	} else if(!pDup)
	{
		newsmp = m_SndFile.GetNextFreeSample(newins);
		if(newsmp > m_SndFile.GetNumSamples())
		{
			// Add a new sample
			const SAMPLEINDEX inssmp = InsertSample();
			if(inssmp != SAMPLEINDEX_INVALID)
				newsmp = inssmp;
		}
	}

	CriticalSection cs;

	ModInstrument *pIns = m_SndFile.AllocateInstrument(newins, newsmp);
	if(pIns == nullptr)
	{
		if(!silent)
		{
			cs.Leave();
			ErrorBox(IDS_ERR_OUTOFMEMORY, CMainFrame::GetMainFrame());
		}
		return INSTRUMENTINDEX_INVALID;
	}
	InitializeInstrument(pIns);

	if(pDup)
	{
		*pIns = *pDup;
	}

	SetModified();

	return newins;
}


// Load default instrument values for inserting new instrument during editing
void CModDoc::InitializeInstrument(ModInstrument *pIns)
{
	pIns->pluginVolumeHandling = TrackerSettings::Instance().DefaultPlugVolumeHandling;
}


// Try to set up a new instrument that is linked to a given plugin
INSTRUMENTINDEX CModDoc::InsertInstrumentForPlugin(PLUGINDEX plug)
{
#ifndef NO_PLUGINS
	const bool first = (GetNumInstruments() == 0);
	INSTRUMENTINDEX instr = InsertInstrument(0, INSTRUMENTINDEX_INVALID, true);
	if(instr == INSTRUMENTINDEX_INVALID)
		return INSTRUMENTINDEX_INVALID;

	ModInstrument &ins = *m_SndFile.Instruments[instr];
	ins.name = mpt::ToCharset(m_SndFile.GetCharsetInternal(), MPT_UFORMAT("{}: {}")(plug + 1, m_SndFile.m_MixPlugins[plug].GetName()));
	ins.filename = mpt::ToCharset(m_SndFile.GetCharsetInternal(), m_SndFile.m_MixPlugins[plug].GetLibraryName());
	ins.nMixPlug = plug + 1;
	ins.nMidiChannel = 1;

	InstrumentHint hint = InstrumentHint(instr).Info().Envelope().Names();
	if(first)
		hint.ModType();
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
{
	if(nSeq >= m_SndFile.Order.GetNumSequences() || nOrd >= m_SndFile.Order(nSeq).size())
		return false;

	CriticalSection cs;
	m_SndFile.Order(nSeq).Remove(nOrd, nOrd);
	SetModified();

	return true;
}



bool CModDoc::RemovePattern(PATTERNINDEX nPat)
{
	if(m_SndFile.Patterns.IsValidPat(nPat))
	{
		CriticalSection cs;
		GetPatternUndo().PrepareUndo(nPat, 0, 0, GetNumChannels(), m_SndFile.Patterns[nPat].GetNumRows(), "Remove Pattern");
		m_SndFile.Patterns.Remove(nPat);
		SetModified();

		return true;
	}
	return false;
}


bool CModDoc::RemoveSample(SAMPLEINDEX nSmp)
{
	if((nSmp) && (nSmp <= m_SndFile.GetNumSamples()))
	{
		CriticalSection cs;

		m_SndFile.DestroySample(nSmp);
		m_SndFile.m_szNames[nSmp] = "";
		while((m_SndFile.GetNumSamples() > 1)
		      && (!m_SndFile.m_szNames[m_SndFile.GetNumSamples()][0])
		      && (!m_SndFile.GetSample(m_SndFile.GetNumSamples()).HasSampleData()))
		{
			m_SndFile.m_nSamples--;
		}
		SetModified();

		return true;
	}
	return false;
}


bool CModDoc::RemoveInstrument(INSTRUMENTINDEX nIns)
{
	if((nIns) && (nIns <= m_SndFile.GetNumInstruments()) && (m_SndFile.Instruments[nIns]))
	{
		ConfirmAnswer result = cnfNo;
		if(!m_SndFile.Instruments[nIns]->GetSamples().empty())
			result = Reporting::Confirm("Remove samples associated with an instrument if they are unused?", "Removing instrument", true);
		if(result == cnfCancel)
		{
			return false;
		}
		if(m_SndFile.DestroyInstrument(nIns, (result == cnfYes) ? deleteAssociatedSamples : doNoDeleteAssociatedSamples))
		{
			CriticalSection cs;
			if(nIns == m_SndFile.m_nInstruments)
				m_SndFile.m_nInstruments--;
			bool instrumentsLeft = std::find_if(std::begin(m_SndFile.Instruments), std::end(m_SndFile.Instruments), [](ModInstrument *ins) { return ins != nullptr; }) != std::end(m_SndFile.Instruments);
			if(!instrumentsLeft)
				m_SndFile.m_nInstruments = 0;
			SetModified();

			return true;
		}
	}
	return false;
}


bool CModDoc::MoveOrder(ORDERINDEX sourceOrd, ORDERINDEX destOrd, bool update, bool copy, SEQUENCEINDEX sourceSeq, SEQUENCEINDEX destSeq)
{
	if(sourceSeq == SEQUENCEINDEX_INVALID)
		sourceSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	if(destSeq == SEQUENCEINDEX_INVALID)
		destSeq = m_SndFile.Order.GetCurrentSequenceIndex();
	if(std::max(sourceSeq, destSeq) >= m_SndFile.Order.GetNumSequences())
		return false;

	const ORDERINDEX maxOrders = m_SndFile.GetModSpecifications().ordersMax;
	if(destOrd > maxOrders)
		return false;
	if(destOrd == maxOrders && (sourceSeq != destSeq || copy))
		return false;

	auto &sourceSequence = m_SndFile.Order(sourceSeq);
	const PATTERNINDEX sourcePat = sourceOrd < sourceSequence.size() ? sourceSequence[sourceOrd] : sourceSequence.GetInvalidPatIndex();

	// Delete source
	if(!copy)
	{
		sourceSequence.Remove(sourceOrd, sourceOrd);
		if(sourceOrd < destOrd && sourceSeq == destSeq)
			destOrd--;
	}
	// Insert at dest
	m_SndFile.Order(destSeq).insert(destOrd, 1, sourcePat);

	if(update)
	{
		UpdateAllViews(nullptr, SequenceHint().Data());
	}
	return true;
}


BOOL CModDoc::ExpandPattern(PATTERNINDEX nPattern)
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

static constexpr const char pszEnvHdr[] = "ModPlug Tracker Envelope\r\n";
static constexpr const char pszEnvFmt[] = "%d,%d,%d,%d,%d,%d,%d,%d\r\n";

static bool EnvelopeToString(CStringA &s, const InstrumentEnvelope &env)
{
	// We don't want to copy empty envelopes
	if(env.empty())
	{
		return false;
	}

	s.Preallocate(2048);
	s = pszEnvHdr;
	s.AppendFormat(pszEnvFmt, env.size(), env.nSustainStart, env.nSustainEnd, env.nLoopStart, env.nLoopEnd, env.dwFlags[ENV_SUSTAIN] ? 1 : 0, env.dwFlags[ENV_LOOP] ? 1 : 0, env.dwFlags[ENV_CARRY] ? 1 : 0);
	for(auto &p : env)
	{
		s.AppendFormat("%d,%d\r\n", p.tick, p.value);
	}

	// Writing release node
	s.AppendFormat("%u\r\n", env.nReleaseNode);
	return true;
}


static bool StringToEnvelope(const std::string_view &s, InstrumentEnvelope &env, const CModSpecifications &specs)
{
	uint32 susBegin = 0, susEnd = 0, loopBegin = 0, loopEnd = 0, bSus = 0, bLoop = 0, bCarry = 0, nPoints = 0, releaseNode = ENV_RELEASE_NODE_UNSET;
	size_t length = s.size(), pos = std::size(pszEnvHdr) - 1;
	if(length <= pos || mpt::CompareNoCaseAscii(s.data(), pszEnvHdr, pos - 2))
	{
		return false;
	}
	sscanf(&s[pos], pszEnvFmt, &nPoints, &susBegin, &susEnd, &loopBegin, &loopEnd, &bSus, &bLoop, &bCarry);
	while(pos < length && s[pos] != '\r' && s[pos] != '\n')
		pos++;

	nPoints = std::min(nPoints, static_cast<uint32>(specs.envelopePointsMax));
	if(susEnd >= nPoints)
		susEnd = 0;
	if(susBegin > susEnd)
		susBegin = susEnd;
	if(loopEnd >= nPoints)
		loopEnd = 0;
	if(loopBegin > loopEnd)
		loopBegin = loopEnd;

	try
	{
		env.resize(nPoints);
	} catch(mpt::out_of_memory e)
	{
		mpt::delete_out_of_memory(e);
		return false;
	}
	env.nSustainStart = static_cast<decltype(env.nSustainStart)>(susBegin);
	env.nSustainEnd = static_cast<decltype(env.nSustainEnd)>(susEnd);
	env.nLoopStart = static_cast<decltype(env.nLoopStart)>(loopBegin);
	env.nLoopEnd = static_cast<decltype(env.nLoopEnd)>(loopEnd);
	env.nReleaseNode = static_cast<decltype(env.nReleaseNode)>(releaseNode);
	env.dwFlags.set(ENV_LOOP, bLoop != 0);
	env.dwFlags.set(ENV_SUSTAIN, bSus != 0);
	env.dwFlags.set(ENV_CARRY, bCarry != 0);
	env.dwFlags.set(ENV_ENABLED, nPoints > 0);

	int oldn = 0;
	for(auto &p : env)
	{
		while(pos < length && (s[pos] < '0' || s[pos] > '9'))
			pos++;
		if(pos >= length)
			break;
		int n1 = atoi(&s[pos]);
		while(pos < length && s[pos] != ',')
			pos++;
		while(pos < length && (s[pos] < '0' || s[pos] > '9'))
			pos++;
		if(pos >= length)
			break;
		int n2 = atoi(&s[pos]);
		if(n1 < oldn)
			n1 = oldn + 1;
		Limit(n2, ENVELOPE_MIN, ENVELOPE_MAX);
		p.tick = (uint16)n1;
		p.value = (uint8)n2;
		oldn = n1;
		while(pos < length && s[pos] != '\r' && s[pos] != '\n')
			pos++;
		if(pos >= length)
			break;
	}
	env.Sanitize();

	// Read release node information.
	env.nReleaseNode = ENV_RELEASE_NODE_UNSET;
	if(pos < length)
	{
		auto r = static_cast<decltype(env.nReleaseNode)>(atoi(&s[pos]));
		if(r == 0 || r >= nPoints || !specs.hasReleaseNode)
			r = ENV_RELEASE_NODE_UNSET;
		env.nReleaseNode = r;
	}
	return true;
}


bool CModDoc::CopyEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if((nIns < 1) || (nIns > m_SndFile.m_nInstruments) || (!m_SndFile.Instruments[nIns]) || (!pMainFrm))
		return false;
	BeginWaitCursor();
	const ModInstrument *pIns = m_SndFile.Instruments[nIns];
	if(pIns == nullptr)
		return false;

	CStringA s;
	EnvelopeToString(s, pIns->GetEnvelope(nEnv));

	int memSize = s.GetLength() + 1;
	Clipboard clipboard(CF_TEXT, memSize);
	if(auto p = clipboard.As<char>())
	{
		memcpy(p, s.GetString(), memSize);
	}
	EndWaitCursor();
	return true;
}


bool CModDoc::SaveEnvelope(INSTRUMENTINDEX ins, EnvelopeType env, const mpt::PathString &fileName)
{
	if(ins < 1 || ins > m_SndFile.m_nInstruments || !m_SndFile.Instruments[ins])
		return false;
	BeginWaitCursor();
	const ModInstrument *pIns = m_SndFile.Instruments[ins];
	if(pIns == nullptr)
		return false;

	CStringA s;
	EnvelopeToString(s, pIns->GetEnvelope(env));

	bool ok = false;
	try
	{
		mpt::IO::SafeOutputFile sf(fileName, std::ios::binary, mpt::IO::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
		mpt::IO::ofstream &f = sf;
		f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);
		if(f)
			ok = mpt::IO::WriteRaw(f, s.GetString(), s.GetLength());
		else
			ok = false;
	} catch(const std::exception &)
	{
		ok = false;
	}
	EndWaitCursor();
	return ok;
}


bool CModDoc::PasteEnvelope(INSTRUMENTINDEX ins, EnvelopeType env)
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(ins < 1 || ins > m_SndFile.m_nInstruments || !m_SndFile.Instruments[ins] || !pMainFrm)
		return false;
	BeginWaitCursor();
	Clipboard clipboard(CF_TEXT);
	auto data = clipboard.GetString();
	if(!data.length())
	{
		EndWaitCursor();
		return false;
	}
	bool result = StringToEnvelope(data, m_SndFile.Instruments[ins]->GetEnvelope(env), m_SndFile.GetModSpecifications());
	EndWaitCursor();
	return result;
}


bool CModDoc::LoadEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv, const mpt::PathString &fileName)
{
	mpt::IO::InputFile f(fileName, TrackerSettings::Instance().MiscCacheCompleteFileBeforeLoading);
	if(nIns < 1 || nIns > m_SndFile.m_nInstruments || !m_SndFile.Instruments[nIns] || !f.IsValid())
		return false;
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
{
	// Checking for unused channels
	CHANNELINDEX chn = GetNumChannels();
	usedMask.assign(chn, true);
	while(chn-- > 0)
	{
		if(IsChannelUnused(chn))
		{
			usedMask[chn] = false;
			// Found enough empty channels yet?
			if((--maxRemoveCount) == 0)
				break;
		}
	}
}


// Check if a given channel contains note data or global effects.
bool CModDoc::IsChannelUnused(CHANNELINDEX nChn) const
{
	const CHANNELINDEX nChannels = GetNumChannels();
	if(nChn >= nChannels)
	{
		return true;
	}
	for(auto &pat : m_SndFile.Patterns)
	{
		if(pat.IsValid())
		{
			const ModCommand *p = pat.GetpModCommand(0, nChn);
			for(ROWINDEX row = pat.GetNumRows(); row > 0; row--, p += nChannels)
			{
				if(p->IsNote() || p->IsInstrPlug() || p->IsGlobalCommand())
					return false;
			}
		}
	}
	return true;
}


bool CModDoc::IsSampleUsed(SAMPLEINDEX sample, bool searchInMutedChannels) const
{
	if(!sample || sample > GetNumSamples())
		return false;
	if(GetNumInstruments())
	{
		for(INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			if(m_SndFile.IsSampleReferencedByInstrument(sample, i))
				return true;
		}
	} else
	{
		for(const auto &pattern : m_SndFile.Patterns)
		{
			if(!pattern.IsValid())
				continue;
			auto m = pattern.cbegin();
			for(ROWINDEX row = 0; row < pattern.GetNumRows(); row++)
			{
				for(CHANNELINDEX chn = 0; chn < pattern.GetNumChannels(); chn++, m++)
				{
					if(searchInMutedChannels || !m_SndFile.ChnSettings[chn].dwFlags[CHN_MUTE])
					{
						if(m->instr == sample && !m->IsPcNote())
							return true;
					}
				}
			}
		}
	}
	return false;
}


bool CModDoc::IsInstrumentUsed(INSTRUMENTINDEX instr, bool searchInMutedChannels) const
{
	if(instr < 1 || instr > GetNumInstruments())
		return false;
	for(const auto &pattern : m_SndFile.Patterns)
	{
		if(!pattern.IsValid())
			continue;
		auto m = pattern.cbegin();
		for(ROWINDEX row = 0; row < pattern.GetNumRows(); row++)
		{
			for(CHANNELINDEX chn = 0; chn < pattern.GetNumChannels(); chn++, m++)
			{
				if(searchInMutedChannels || !m_SndFile.ChnSettings[chn].dwFlags[CHN_MUTE])
				{
					if(m->instr == instr && !m->IsPcNote())
						return true;
				}
			}
		}
	}
	return false;
}


// Convert module's default global volume to a pattern command.
bool CModDoc::GlobalVolumeToPattern()
{
	bool result = false;
	if(m_SndFile.GetModSpecifications().HasCommand(CMD_GLOBALVOLUME))
	{
		for(PATTERNINDEX pat : m_SndFile.Order())
		{
			if(m_SndFile.Patterns[pat].WriteEffect(EffectWriter(CMD_GLOBALVOLUME, mpt::saturate_cast<ModCommand::PARAM>(m_SndFile.m_nDefaultGlobalVolume * 64 / MAX_GLOBAL_VOLUME)).RetryNextRow()))
			{
				result = true;
				break;
			}
		}
	}

	m_SndFile.m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	return result;
}


SAMPLEINDEX CModDoc::GetSampleIndex(const ModCommand &m, ModCommand::INSTR lastInstr) const
{
	if(m.IsPcNote())
		return 0;
	return m_SndFile.GetSampleIndex(m.note, m.instr > 0 ? m.instr : lastInstr);
}


int CModDoc::GetInstrumentGroupSize(INSTRUMENTINDEX instr) const
{
	const ModInstrument *ins;
	if(instr > 0 && instr <= GetNumInstruments()
	   && (ins = m_SndFile.Instruments[instr]) != nullptr && ins->pTuning != nullptr
	   && ins->pTuning->GetGroupSize() != 0)
	{
		return ins->pTuning->GetGroupSize();
	}
	return 12;
}


int CModDoc::GetBaseNote(INSTRUMENTINDEX instr) const
{
	// This may look a bit strange (using -12 and -4 instead of just -5 in the second part) but this is to keep custom tunings centered around middle-C on the keyboard.
	return NOTE_MIDDLEC - 12 + (CMainFrame::GetMainFrame()->GetBaseOctave() - 4) * GetInstrumentGroupSize(instr);
}


ModCommand::NOTE CModDoc::GetNoteWithBaseOctave(int noteOffset, INSTRUMENTINDEX instr) const
{
	return static_cast<ModCommand::NOTE>(Clamp(GetBaseNote(instr) + noteOffset, NOTE_MIN, NOTE_MAX));
}


INSTRUMENTINDEX CModDoc::GetParentInstrumentWithSameName(SAMPLEINDEX smp) const
{
	auto ins = FindSampleParent(smp);
	if(ins == INSTRUMENTINDEX_INVALID)
		return INSTRUMENTINDEX_INVALID;
	auto instr = m_SndFile.Instruments[ins];
	if(instr == nullptr)
		return INSTRUMENTINDEX_INVALID;
	if((!instr->name.empty() && instr->name != m_SndFile.m_szNames[smp]) || instr->GetSamples().size() != 1)
		return INSTRUMENTINDEX_INVALID;

	return ins;
}


OPENMPT_NAMESPACE_END
