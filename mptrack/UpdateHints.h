/*
 * UpdateHints.h
 * -------------
 * Purpose: Hint type and abstraction class for passing around hints between module views.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "openmpt/base/FlagSet.hpp"
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

// Mutually exclusive hint categories
enum HintCategory
{
	HINTCAT_GLOBAL		= 0,	// Not a real category, since all other categories can be combined with this
	HINTCAT_GENERAL		= 0,
	HINTCAT_PATTERNS	= 1,
	HINTCAT_SAMPLES		= 2,
	HINTCAT_INSTRUMENTS	= 3,
	HINTCAT_SEQUENCE	= 4,
	HINTCAT_PLUGINS		= 5,
	HINTCAT_COMMENTS	= 6,

	NUM_HINTCATS
};

enum HintType
{
	// Hints that can be combined with any other hints (no parameter)
	HINT_NONE			= 0x00,	// No specific hint
	HINT_MODTYPE		= 0x01,	// Module type has changed. Generally this will force most things to update.
	HINT_MPTOPTIONS		= 0x02,	// Some OpenMPT options (e.g. colours) have changed which might require stuff to be redrawn.
	HINT_UNDO			= 0x04,	// Undo state information has changed

	HINT_ALLGLOBAL = HINT_MODTYPE | HINT_MPTOPTIONS | HINT_UNDO,

	// From here: Mutually exclusive hint categories

	// General module setting hints (GeneralHint)
	HINT_MODGENERAL		= 0x10,	// General global module settings have changed
	HINT_MODCHANNELS	= 0x20,	// Module channel settings have changed (e.g. channel volume). Parameter: Channel ID
	HINT_TUNINGS		= 0x40,	// Tuning collection was updated
	// Pattern-specific hints (PatternHint)
	HINT_PATTERNDATA	= 0x10,	// Pattern data has changed. Parameter: Pattern ID (0 = all patterns)
	HINT_PATTERNROW		= 0x20,	// A row of the currently edited pattern has changed. Parameter: Row number
	HINT_PATNAMES		= 0x40,	// Pattern names have changed. Parameter: Pattern ID (0 = all patterns)
	// Sample-specific hints (SampleHint)
	HINT_SAMPLEINFO		= 0x10,	// Sample properties have changed. Parameter: Sample ID (0 = all samples)
	HINT_SAMPLEDATA		= 0x20,	// Sample waveform has changed. Parameter: Sample ID (0 = all samples)
	HINT_SMPNAMES		= 0x40,	// Sample name has changed. Parameter: Sample ID (0 = all samples)
	// Instrument-specific hints (InstrumentHint)
	HINT_INSTRUMENT		= 0x10,	// Instrument properties have changed. Parameter: Instrument ID (0 = all instruments)
	HINT_ENVELOPE		= 0x20,	// An instrument envelope has changed. Parameter: Instrument ID (0 = all instruments)
	HINT_INSNAMES		= 0x40,	// Instrument name has changed. Parameter: Instrument ID (0 = all instruments)
	// Sequence-specific hints (SequenceHint)
	HINT_MODSEQUENCE	= 0x10,	// The pattern sequence has changed.
	HINT_SEQNAMES		= 0x20,	// Sequence names have changed. Parameter: Sequence ID (0 = all sequences)
	HINT_RESTARTPOS		= 0x40,	// Restart position has changed. Parameter: Sequence ID (0 = all sequences)
	// Plugin-specific hints (PluginHint)
	HINT_MIXPLUGINS		= 0x10,	// Plugin properties have changed. Parameter: Plugin ID (0 = all plugins, 1 = first plugin)
	HINT_PLUGINNAMES	= 0x20,	// Plugin names have changed. Parameter: Plugin ID (0 = all plugins, 1 = first plugin)
	HINT_PLUGINPARAM	= 0x40,	// Plugin parameter has changed. Parameter: Plugin ID (0 = all plugins, 1 = first plugin)
	// Comment text hints (CommentHint)
	HINT_MODCOMMENTS	= 0x10,	// Module comment text has changed
};
DECLARE_FLAGSET(HintType)

struct UpdateHint
{
protected:
	using store_t = uint32;
	union
	{
		struct
		{
			store_t type     : 7;	// All HintType flags must fit into this.
			store_t category : 3;	// All HintCategory types must fit into this.
			store_t item     : 22;
		};
		store_t rawData;
	};
	
	UpdateHint(HintCategory category_, store_t item_ = 0) : type(HINT_NONE), category(category_), item(item_)
	{
		static_assert(sizeof(UpdateHint) == sizeof(store_t), "Internal UpdateHint size inconsistency");
		static_assert(sizeof(UpdateHint) <= sizeof(LPARAM), "Update hints are currently tunnelled through LPARAMs in MFC");
		MPT_ASSERT(static_cast<HintCategory>(category) == category_);
		MPT_ASSERT(UpdateHint::item == item);
	}

	template<typename T>
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE T GetData() const { return static_cast<T>(item); }

public:
	UpdateHint() : type(HINT_NONE), category(HINTCAT_GLOBAL), item(0) { }

	template<typename T>
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE UpdateHint &SetData(T i) { item = i; MPT_ASSERT(item == i); return *this; }

	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE HintCategory GetCategory() const { return static_cast<HintCategory>(category); }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE FlagSet<HintType> GetType() const { return FlagSet<HintType>(static_cast<FlagSet<HintType>::store_type>(type)); }

	// CModDoc hint tunnelling
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static UpdateHint FromLPARAM(LPARAM rawData) { UpdateHint hint; hint.rawData = static_cast<store_t>(rawData); return hint; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE LPARAM AsLPARAM() const { return rawData; }

	// Discard any hints that don't belong to class T.
	template<typename T>
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE T ToType() const
	{
		T hint = static_cast<const T &>(*this);
		if(T::classCategory != static_cast<HintCategory>(category))
		{
			hint.type &= HINT_ALLGLOBAL;
			hint.item = 0;
		}
		return hint;
	}

	// Set global hint flags
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE UpdateHint &ModType() { type |= HINT_MODTYPE; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE UpdateHint &MPTOptions() { type |= HINT_MPTOPTIONS; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE UpdateHint &Undo() { type |= HINT_UNDO; return *this; }
};

struct GeneralHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_GENERAL;
	GeneralHint() : UpdateHint(classCategory, 0) { }
	GeneralHint(CHANNELINDEX channel) : UpdateHint(classCategory, 1 + channel) { }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE GeneralHint &General() { type |= HINT_MODGENERAL; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE GeneralHint &Channels() { type |= HINT_MODCHANNELS; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE GeneralHint &Tunings() { type |= HINT_TUNINGS; return *this; }

	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE CHANNELINDEX GetChannel() const { return item ? static_cast<CHANNELINDEX>(item - 1) : CHANNELINDEX_INVALID; }
};

struct PatternHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_PATTERNS;
	PatternHint(PATTERNINDEX item = 0) : UpdateHint(classCategory, item) { }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE PatternHint &Data() { type |= HINT_PATTERNDATA; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE PatternHint &Names() { type |= HINT_PATNAMES; return *this; }

	PATTERNINDEX GetPattern() const { return GetData<PATTERNINDEX>(); }
};

struct RowHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_PATTERNS;
	RowHint(ROWINDEX item = 0) : UpdateHint(classCategory, item) { type = HINT_PATTERNROW; }

	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE ROWINDEX GetRow() const { return GetData<ROWINDEX>(); }
};

struct SampleHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_SAMPLES;
	SampleHint(SAMPLEINDEX item = 0) : UpdateHint(classCategory, item) { }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SampleHint &Info() { type |= HINT_SAMPLEINFO; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SampleHint &Data() { type |= HINT_SAMPLEDATA; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SampleHint &Names() { type |= HINT_SMPNAMES; return *this; }

	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SAMPLEINDEX GetSample() const { return GetData<SAMPLEINDEX>(); }
};

struct InstrumentHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_INSTRUMENTS;
	InstrumentHint(INSTRUMENTINDEX item = 0) : UpdateHint(classCategory, item) { }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE InstrumentHint &Info() { type |= HINT_INSTRUMENT; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE InstrumentHint &Envelope() { type |= HINT_ENVELOPE; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE InstrumentHint &Names() { type |= HINT_INSNAMES; return *this; }

	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE INSTRUMENTINDEX GetInstrument() const { return GetData<INSTRUMENTINDEX>(); }
};

struct SequenceHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_SEQUENCE;
	SequenceHint(SEQUENCEINDEX item = 0) : UpdateHint(classCategory, item) { }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SequenceHint &Data() { type |= HINT_MODSEQUENCE; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SequenceHint &Names() { type |= HINT_SEQNAMES; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SequenceHint &RestartPos() { type |= HINT_RESTARTPOS; return *this; }

	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE SEQUENCEINDEX GetSequence() const { return GetData<SEQUENCEINDEX>(); }
};

struct PluginHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_PLUGINS;
	PluginHint(PLUGINDEX item = 0) : UpdateHint(classCategory, item) { }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE PluginHint &Info() { type |= HINT_MIXPLUGINS; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE PluginHint &Names() { type |= HINT_PLUGINNAMES; return *this; }
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE PluginHint &Parameter() { type |= HINT_PLUGINPARAM; return *this; }

	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE PLUGINDEX GetPlugin() const { return GetData<PLUGINDEX>(); }
};

struct CommentHint : public UpdateHint
{
	static constexpr HintCategory classCategory = HINTCAT_COMMENTS;
	CommentHint() : UpdateHint(classCategory) { type = HINT_MODCOMMENTS; }
};

OPENMPT_NAMESPACE_END
