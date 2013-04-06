/*
 * Notification.h
 * --------------
 * Purpose: GUI update notification struct
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

// struct Notification requires working copy constructor / copy assignment, keep in mind when extending
struct Notification
{
	enum Type
	{
		None		= 0x00,
		Position	= 0x01,	// Pattern playback position
		Sample		= 0x02,	// pos[i] contains sample position on this channel
		VolEnv		= 0x04,	// pos[i] contains volume envelope position
		PanEnv		= 0x08,	// pos[i] contains panning envelope position
		PitchEnv	= 0x10,	// pos[i] contains pitch envelope position
		VUMeters	= 0x20,	// pos[i] contains VU meter for this channel
		EOS			= 0x40,	// End of stream reached, the GUI should stop the audio device
		Stop		= 0x80,	// Audio device has been stopped -> reset GUI

		Default		= Position,
	};

	typedef uint16 Item;

	static const SmpLength PosInvalid = -1;	// pos[i] is not valid (if it contains sample or envelope position)

	/*
		timestampSamples is kind of confusing at the moment:
		If gpSoundDevice->HasGetStreamPosition(),
			then it contains the sample timestamp as when it was generated and the output stream is later queried when this exact timestamp has actually reached the speakers.
		 If !gpSoundDevice->HasGetStreamPosition(),
			then it contains a sample timestamp in the future, incremented by the current latency estimation of the sound buffers. It is later checked against the total number of rendered samples at that time.
	*/
	int64 timestampSamples;
	FlagSet<Type, uint16> type;
	Item item;						// Sample or instrument number, depending on type
	ROWINDEX row;					// Always valid
	uint32 tick;					// dito
	ORDERINDEX order;				// dito
	PATTERNINDEX pattern;			// dito
	uint32 masterVu[2];				// dito
	SmpLength pos[MAX_CHANNELS];	// Sample / envelope pos for each channel if != PosInvalid, or pattern channel VUs
};

DECLARE_FLAGSET(Notification::Type);
