/*
 * Notification.h
 * --------------
 * Purpose: GUI update notification struct
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

// struct Notification requires working copy constructor / copy assignment, keep in mind when extending
struct Notification
{
	enum Type
	{
		GlobalVU	= 0x00,	// Global VU meters (always enabled)
		Position	= 0x01,	// Pattern playback position
		Sample		= 0x02,	// pos[i] contains sample position on this channel
		VolEnv		= 0x04,	// pos[i] contains volume envelope position
		PanEnv		= 0x08,	// pos[i] contains panning envelope position
		PitchEnv	= 0x10,	// pos[i] contains pitch envelope position
		VUMeters	= 0x20,	// pos[i] contains pattern VU meter for this channel
		EOS			= 0x40,	// End of stream reached, the GUI should stop the audio device
		Stop		= 0x80,	// Audio device has been stopped -> reset GUI

		Default		= GlobalVU,
	};

	typedef uint16 Item;

	static constexpr SmpLength PosInvalid = SmpLength(-1);	// pos[i] is not valid (if it contains sample or envelope position)
	static constexpr uint32 ClipVU = 0x80000000;			// Master VU clip indicator bit (sound output has previously clipped)

	int64 timestampSamples;
	FlagSet<Notification::Type> type;
	Item item;                               // Sample or instrument number, depending on type
	ROWINDEX row;                            // Always valid
	uint32 tick, ticksOnRow;                 // ditto
	ORDERINDEX order;                        // ditto
	PATTERNINDEX pattern;                    // ditto
	uint32 mixedChannels;                    // ditto
	std::array<uint32, 4> masterVUin;        // ditto
	std::array<uint32, 4> masterVUout;       // ditto
	uint8 masterVUinChannels;                // ditto
	uint8 masterVUoutChannels;               // ditto
	std::array<SmpLength, MAX_CHANNELS> pos; // Sample / envelope pos for each channel if != PosInvalid, or pattern channel VUs

	Notification(FlagSet<Notification::Type> t = Default, Item i = 0, int64 s = 0, ROWINDEX r = 0, uint32 ti = 0, uint32 tir = 0, ORDERINDEX o = 0, PATTERNINDEX p = 0, uint32 x = 0, uint8 outChannels = 0, uint8 inChannels = 0) : timestampSamples(s), type(t), item(i), row(r), tick(ti), ticksOnRow(tir), order(o), pattern(p), mixedChannels(x), masterVUinChannels(inChannels), masterVUoutChannels(outChannels)
	{
		masterVUin.fill(0);
		masterVUout.fill(0);
		pos.fill(0);
	}
};

DECLARE_FLAGSET(Notification::Type);

OPENMPT_NAMESPACE_END
