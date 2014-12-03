/*
 * UpdateHints.h
 * -------------
 * Purpose: Hint type and abstraction class for passing around hints between module views.
 * Notes  : Please read the note in enum HintMode if you plan to add more hint types.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/FlagSet.h"

// Bit Mask for updating view (hints of what changed)
enum HintType
{
	HINT_NONE			= 0x00000,
	HINT_MODTYPE		= 0x00001,
	HINT_MODCOMMENTS	= 0x00002,
	HINT_MODGENERAL		= 0x00004,
	HINT_MODSEQUENCE	= 0x00008,
	HINT_MODCHANNELS	= 0x00010,
	HINT_PATTERNDATA	= 0x00020,
	HINT_PATTERNROW		= 0x00040,
	HINT_PATNAMES		= 0x00080,
	HINT_MPTOPTIONS		= 0x00100,
	HINT_SAMPLEINFO		= 0x00400,
	HINT_SAMPLEDATA		= 0x00800,
	HINT_INSTRUMENT		= 0x01000,
	HINT_ENVELOPE		= 0x02000,
	HINT_SMPNAMES		= 0x04000,
	HINT_INSNAMES		= 0x08000,
	HINT_UNDO			= 0x10000,
	HINT_MIXPLUGINS		= 0x20000,
	HINT_SEQNAMES		= 0x40000,
	// NOTE: Hint type and hint data need to fit together into one 32-bit integer.
	// At the moment, this means that there are only 14 bits left for hint data!
	// This means that it will be difficult to add hints in the current scheme,
	// which is only really required because hints are passed through LPARAMs in MFC.
	// There are two ways to fix this:
	// 1) Override MFC's mechanism to pass around UpdateHints instead (Rewrite OnUpdate/etc)
	// 2) Reduce number of bits required for hints. This could for example be done by
	//    finding mutually exclusive flags that can only be used if a certain "context"
	//    is set (e.g. have instrument, sample and pattern contexts and then use overlapping
	//    flags for them)

	HINT_MAXHINTFLAG = HINT_SEQNAMES
};
DECLARE_FLAGSET(HintType)


struct UpdateHint
{
protected:
	uint32_t data;

	template<int v>
	static int PowerOf2Exponent()
	{
		if(v <= 1) return 0;
		else return 1 + PowerOf2Exponent<v / 2>();
	}

	void Set(FlagSet<HintType> type, uint16_t item = 0)
	{
		data = type.GetRaw() | item << PowerOf2Exponent<HINT_MAXHINTFLAG>();
	}

public:
	UpdateHint(FlagSet<HintType>::value_type type, uint16_t item = 0)
	{
		Set(type, item);
	}

	UpdateHint(FlagSet<HintType> type, uint16_t item = 0)
	{
		Set(type, item);
	}

	UpdateHint(HintType type, uint16_t item = 0)
	{
		Set(type, item);
	}

	explicit UpdateHint(LPARAM data) : data(static_cast<uint32_t>(data)) { }

	FlagSet<HintType> GetType() const { return FlagSet<HintType>(static_cast<FlagSet<HintType>::store_type>(data & (HINT_MAXHINTFLAG | (HINT_MAXHINTFLAG - 1)))); }
	uint16_t GetData() const { return static_cast<uint16_t>(data >> PowerOf2Exponent<HINT_MAXHINTFLAG>()); }

	LPARAM AsLPARAM() const { return data; }
};

static_assert(sizeof(UpdateHint) <= sizeof(LPARAM), "Update hints are currently tunnelled through LPARAMs in MFC");

struct SampleHint : public UpdateHint
{
	SampleHint(FlagSet<HintType> type, SAMPLEINDEX item) : UpdateHint(type, item) { }
};

struct InstrumentHint : public UpdateHint
{
	InstrumentHint(FlagSet<HintType> type, INSTRUMENTINDEX item) : UpdateHint(type, item) { }
};

struct PatternHint : public UpdateHint
{
	PatternHint(FlagSet<HintType> type, PATTERNINDEX item) : UpdateHint(type, item) { }
};

struct RowHint : public UpdateHint
{
	RowHint(FlagSet<HintType> type, ROWINDEX item) : UpdateHint(type, static_cast<uint16_t>(item)) { }
};

struct SequenceHint : public UpdateHint
{
	SequenceHint(FlagSet<HintType> type, SEQUENCEINDEX item) : UpdateHint(type, item) { }
};

struct ChannelTabHint : public UpdateHint
{
	ChannelTabHint(FlagSet<HintType> type, int item) : UpdateHint(type, static_cast<uint16_t>(item)) { }
};
