/*
 * Undo.h
 * ------
 * Purpose: Editor undo buffer functionality.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../soundlib/ModChannel.h"
#include "../soundlib/modcommand.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
struct ModSample;

#define MAX_UNDO_LEVEL 100000	// 100,000 undo steps for each undo type!

/////////////////////////////////////////////////////////////////////////////////////////
// Pattern Undo


class CPatternUndo
{
protected:
	static constexpr auto DELETE_PATTERN = PATTERNINDEX_INVALID;

	struct UndoInfo
	{
		std::vector<ModChannelSettings> channelInfo;	// Optional old channel information (pan / volume / etc.)
		std::vector<ModCommand> content;	// Rescued pattern content
		const char *description;			// Name of this undo action
		ROWINDEX numPatternRows;			// Original number of pattern rows (in case of resize, DELETE_PATTERN in case of deletion)
		ROWINDEX firstRow, numRows;
		PATTERNINDEX pattern;
		CHANNELINDEX firstChannel, numChannels;
		bool linkToPrevious;			// This undo information is linked with the previous undo information

		bool OnlyChannelSettings() const noexcept
		{
			return !channelInfo.empty() && numRows < 1 && !linkToPrevious;
		}
	};

	using undobuf_t = std::vector<UndoInfo>;

	undobuf_t UndoBuffer;
	undobuf_t RedoBuffer;
	CModDoc &modDoc;

	// Pattern undo helper functions
	PATTERNINDEX Undo(undobuf_t &fromBuf, undobuf_t &toBuf, bool linkedFromPrevious);

	bool PrepareBuffer(undobuf_t &buffer, PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, const char *description, bool linkToPrevious, bool storeChannelInfo) const;

	static CString GetName(const undobuf_t &buffer);
	static void RearrangePatterns(undobuf_t &buffer, const std::vector<PATTERNINDEX> &newIndex);

public:

	// Removes all undo steps from the buffer.
	void ClearUndo();
	// Adds a new action to the undo buffer.
	bool PrepareUndo(PATTERNINDEX pattern, CHANNELINDEX firstChn, ROWINDEX firstRow, CHANNELINDEX numChns, ROWINDEX numRows, const char *description, bool linkToPrevious = false, bool storeChannelInfo = false);
	// Adds a new action only affecting channel data to the undo buffer.
	bool PrepareChannelUndo(CHANNELINDEX firstChn, CHANNELINDEX numChns, const char *description);
	// Undoes the most recent action.
	PATTERNINDEX Undo();
	// Redoes the most recent action.
	PATTERNINDEX Redo();
	// Returns true if any actions can currently be undone.
	bool CanUndo() const { return !UndoBuffer.empty(); }
	// Returns true if any actions can currently be redone.
	bool CanRedo() const { return !RedoBuffer.empty(); }
	// Returns true if a channel-specific action (no pattern data) can currently be undone.
	bool CanUndoChannelSettings() const { return !UndoBuffer.empty() && UndoBuffer.back().OnlyChannelSettings(); }
	// Returns true if a channel-specific action (no pattern data) actions can currently be redone.
	bool CanRedoChannelSettings() const { return !RedoBuffer.empty() && RedoBuffer.back().OnlyChannelSettings(); }
	// Remove the latest added undo step from the undo buffer
	void RemoveLastUndoStep();
	// Get name of next undo item
	CString GetUndoName() const { return GetName(UndoBuffer); }
	// Get name of next redo item
	CString GetRedoName() const { return GetName(RedoBuffer); }
	// Adjust undo buffers for rearranged patterns
	void RearrangePatterns(const std::vector<PATTERNINDEX> &newIndex);

	CPatternUndo(CModDoc &parent) : modDoc(parent) { }
};


/////////////////////////////////////////////////////////////////////////////////////////
// Sample Undo

// We will differentiate between different types of undo actions so that we don't have to copy the whole sample everytime.
enum sampleUndoTypes
{
	sundo_none,		// no changes to sample itself, e.g. loop point update
	sundo_update,	// silence, amplify, normalize, dc offset - update complete sample section
	sundo_delete,	// delete part of the sample
	sundo_invert,	// invert sample phase, apply again to undo
	sundo_reverse,	// reverse sample, ditto
	sundo_unsign,	// unsign sample, ditto
	sundo_insert,	// insert data, delete inserted data to undo
	sundo_replace,	// replace complete sample (16->8Bit, up/downsample, downmix to mono, pitch shifting / time stretching, trimming, pasting)
};


class CSampleUndo
{
protected:

	struct UndoInfo
	{
		ModSample OldSample;
		mpt::charbuf<MAX_SAMPLENAME> oldName;
		void *samplePtr = nullptr;
		const char *description = nullptr;
		SmpLength changeStart = 0, changeEnd = 0;
		sampleUndoTypes changeType = sundo_none;
	};

	using undobuf_t = std::vector<std::vector<UndoInfo>>;
	undobuf_t UndoBuffer;
	undobuf_t RedoBuffer;

	CModDoc &modDoc;

	// Sample undo helper functions
	void ClearUndo(undobuf_t &buffer, const SAMPLEINDEX smp);
	void DeleteStep(undobuf_t &buffer, const SAMPLEINDEX smp, const size_t step);
	bool SampleBufferExists(const undobuf_t &buffer, const SAMPLEINDEX smp) const;
	void RestrictBufferSize(undobuf_t &buffer, size_t &capacity);
	size_t GetBufferCapacity(const undobuf_t &buffer) const;
	void RearrangeSamples(undobuf_t &buffer, const std::vector<SAMPLEINDEX> &newIndex);

	bool PrepareBuffer(undobuf_t &buffer, const SAMPLEINDEX smp, sampleUndoTypes changeType, const char *description, SmpLength changeStart, SmpLength changeEnd);
	bool Undo(undobuf_t &fromBuf, undobuf_t &toBuf, const SAMPLEINDEX smp);

public:

	// Sample undo functions
	void ClearUndo();
	void ClearUndo(const SAMPLEINDEX smp) { ClearUndo(UndoBuffer, smp); ClearUndo(RedoBuffer, smp); }
	bool PrepareUndo(const SAMPLEINDEX smp, sampleUndoTypes changeType, const char *description, SmpLength changeStart = 0, SmpLength changeEnd = 0);
	bool Undo(const SAMPLEINDEX smp);
	bool Redo(const SAMPLEINDEX smp);
	bool CanUndo(const SAMPLEINDEX smp) const { return SampleBufferExists(UndoBuffer, smp) && !UndoBuffer[smp - 1].empty(); }
	bool CanRedo(const SAMPLEINDEX smp) const { return SampleBufferExists(RedoBuffer, smp) && !RedoBuffer[smp - 1].empty(); }
	void RemoveLastUndoStep(const SAMPLEINDEX smp);
	const char *GetUndoName(const SAMPLEINDEX smp) const;
	const char *GetRedoName(const SAMPLEINDEX smp) const;
	void RestrictBufferSize();
	void RearrangeSamples(const std::vector<SAMPLEINDEX> &newIndex) { RearrangeSamples(UndoBuffer, newIndex); RearrangeSamples(RedoBuffer, newIndex); }

	CSampleUndo(CModDoc &parent) : modDoc(parent) { }

	~CSampleUndo()
	{
		ClearUndo();
	};

};


/////////////////////////////////////////////////////////////////////////////////////////
// Instrument Undo


class CInstrumentUndo
{
protected:

	struct UndoInfo
	{
		ModInstrument instr;
		const char *description = nullptr;
		EnvelopeType editedEnvelope = ENV_MAXTYPES;
	};

	using undobuf_t = std::vector<std::vector<UndoInfo>>;
	undobuf_t UndoBuffer;
	undobuf_t RedoBuffer;

	CModDoc &modDoc;

	// Instrument undo helper functions
	void ClearUndo(undobuf_t &buffer, const INSTRUMENTINDEX ins);
	void DeleteStep(undobuf_t &buffer, const INSTRUMENTINDEX ins, const size_t step);
	bool InstrumentBufferExists(const undobuf_t &buffer, const INSTRUMENTINDEX ins) const;
	void RearrangeInstruments(undobuf_t &buffer, const std::vector<INSTRUMENTINDEX> &newIndex);
	void RearrangeSamples(undobuf_t &buffer, const INSTRUMENTINDEX ins, std::vector<SAMPLEINDEX> &newIndex);

	bool PrepareBuffer(undobuf_t &buffer, const INSTRUMENTINDEX ins, const char *description, EnvelopeType envType);
	bool Undo(undobuf_t &fromBuf, undobuf_t &toBuf, const INSTRUMENTINDEX ins);

public:

	// Instrument undo functions
	void ClearUndo();
	void ClearUndo(const INSTRUMENTINDEX ins) { ClearUndo(UndoBuffer, ins); ClearUndo(RedoBuffer, ins); }
	bool PrepareUndo(const INSTRUMENTINDEX ins, const char *description, EnvelopeType envType = ENV_MAXTYPES);
	bool Undo(const INSTRUMENTINDEX ins);
	bool Redo(const INSTRUMENTINDEX ins);
	bool CanUndo(const INSTRUMENTINDEX ins) const { return InstrumentBufferExists(UndoBuffer, ins) && !UndoBuffer[ins - 1].empty(); }
	bool CanRedo(const INSTRUMENTINDEX ins) const { return InstrumentBufferExists(RedoBuffer, ins) && !RedoBuffer[ins - 1].empty(); }
	void RemoveLastUndoStep(const INSTRUMENTINDEX ins);
	const char *GetUndoName(const INSTRUMENTINDEX ins) const;
	const char *GetRedoName(const INSTRUMENTINDEX ins) const;
	void RearrangeInstruments(const std::vector<INSTRUMENTINDEX> &newIndex) { RearrangeInstruments(UndoBuffer, newIndex); RearrangeInstruments(RedoBuffer, newIndex); }
	void RearrangeSamples(const INSTRUMENTINDEX ins, std::vector<SAMPLEINDEX> &newIndex) { RearrangeSamples(UndoBuffer, ins, newIndex); RearrangeSamples(RedoBuffer, ins, newIndex); }

	CInstrumentUndo(CModDoc &parent) : modDoc(parent) { }

	~CInstrumentUndo()
	{
		ClearUndo();
	};
};


OPENMPT_NAMESPACE_END
