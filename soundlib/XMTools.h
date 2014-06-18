/*
 * XMTools.h
 * ---------
 * Purpose: Definition of XM file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


OPENMPT_NAMESPACE_BEGIN


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// XM File Header
struct PACKED XMFileHeader
{
	enum XMHeaderFlags
	{
		linearSlides			= 0x01,
		extendedFilterRange		= 0x1000,
	};

	char   signature[17];	// "Extended Module: "
	char   songName[20];	// Song Name, not null-terminated (any nulls are treated as spaces)
	uint8  eof;				// DOS EOF Character (0x1A)
	char   trackerName[20];	// Software that was used to create the XM file
	uint16 version;			// File version (1.02 - 1.04 are supported)
	uint32 size;			// Header Size
	uint16 orders;			// Number of Orders
	uint16 restartPos;		// Restart Position
	uint16 channels;		// Number of Channels
	uint16 patterns;		// Number of Patterns
	uint16 instruments;		// Number of Unstruments
	uint16 flags;			// Song Flags
	uint16 speed;			// Default Speed
	uint16 tempo;			// Default Tempo

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness();
};

STATIC_ASSERT(sizeof(XMFileHeader) == 80);


// XM Instrument Data
struct PACKED XMInstrument
{
	// Envelope Flags
	enum XMEnvelopeFlags
	{
		envEnabled	= 0x01,
		envSustain	= 0x02,
		envLoop		= 0x04,
	};

	uint8  sampleMap[96];	// Note -> Sample assignment
	uint16 volEnv[24];		// Volume envelope nodes / values (0...64)
	uint16 panEnv[24];		// Panning envelope nodes / values (0...63)
	uint8  volPoints;		// Volume envelope length
	uint8  panPoints;		// Panning envelope length
	uint8  volSustain;		// Volume envelope sustain point
	uint8  volLoopStart;	// Volume envelope loop start point
	uint8  volLoopEnd;		// Volume envelope loop end point
	uint8  panSustain;		// Panning envelope sustain point
	uint8  panLoopStart;	// Panning envelope loop start point
	uint8  panLoopEnd;		// Panning envelope loop end point
	uint8  volFlags;		// Volume envelope flags
	uint8  panFlags;		// Panning envelope flags
	uint8  vibType;			// Sample Auto-Vibrato Type
	uint8  vibSweep;		// Sample Auto-Vibrato Sweep
	uint8  vibDepth;		// Sample Auto-Vibrato Depth
	uint8  vibRate;			// Sample Auto-Vibrato Rate
	uint16 volFade;			// Volume Fade-Out
	uint8  midiEnabled;		// MIDI Out Enabled (0 / 1)
	uint8  midiChannel;		// MIDI Channel (0...15)
	uint16 midiProgram;		// MIDI Program (0...127)
	uint16 pitchWheelRange;	// MIDI Pitch Wheel Range (0...36 halftones)
	uint8  muteComputer;	// Mute instrument if MIDI is enabled (0 / 1)
	uint8  reserved[15];	// Reserved

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness();

	enum EnvType
	{
		EnvTypeVol,
		EnvTypePan,
	};
	// Convert OpenMPT's internal envelope representation to XM envelope data.
	void ConvertEnvelopeToXM(const InstrumentEnvelope &mptEnv, uint8 &numPoints, uint8 &flags, uint8 &sustain, uint8 &loopStart, uint8 &loopEnd, EnvType env);
	// Convert XM envelope data to an OpenMPT's internal envelope representation.
	void ConvertEnvelopeToMPT(InstrumentEnvelope &mptEnv, uint8 numPoints, uint8 flags, uint8 sustain, uint8 loopStart, uint8 loopEnd, EnvType env) const;

	// Convert OpenMPT's internal sample representation to an XMInstrument.
	uint16 ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport);
	// Convert an XMInstrument to OpenMPT's internal instrument representation.
	void ConvertToMPT(ModInstrument &mptIns) const;
	// Apply auto-vibrato settings from sample to file.
	void ApplyAutoVibratoToXM(const ModSample &mptSmp, MODTYPE fromType);
	// Apply auto-vibrato settings from file to a sample.
	void ApplyAutoVibratoToMPT(ModSample &mptSmp) const;

	// Get a list of samples that should be written to the file.
	std::vector<SAMPLEINDEX> GetSampleList(const ModInstrument &mptIns, bool compatibilityExport) const;
};

STATIC_ASSERT(sizeof(XMInstrument) == 230);


// XM Instrument Header
struct PACKED XMInstrumentHeader
{
	uint32 size;				// Size of XMInstrumentHeader + XMInstrument
	char   name[22];			// Instrument Name, not null-terminated (any nulls are treated as spaces)
	uint8  type;				// Instrument Type (Apparently FT2 writes some crap here, but it's the same crap for all instruments of the same module!)
	uint16 numSamples;			// Number of Samples associated with instrument
	uint32 sampleHeaderSize;	// Size of XMSample
	XMInstrument instrument;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness();

	// Write stuff to the header that's always necessary (also for empty instruments)
	void Finalise();

	// Convert OpenMPT's internal sample representation to an XMInstrument.
	void ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport);
	// Convert an XMInstrument to OpenMPT's internal instrument representation.
	void ConvertToMPT(ModInstrument &mptIns) const;
};

STATIC_ASSERT(sizeof(XMInstrumentHeader) == 263);


// XI Instrument Header
struct PACKED XIInstrumentHeader
{
	enum
	{
		fileVersion	= 0x102,
	};

	char   signature[21];		// "Extended Instrument: "
	char   name[22];			// Instrument Name, not null-terminated (any nulls are treated as spaces)
	uint8  eof;					// DOS EOF Character (0x1A)
	char   trackerName[20];		// Software that was used to create the XI file
	uint16 version;				// File Version (1.02)
	XMInstrument instrument;
	uint16 numSamples;			// Number of embedded sample headers + samples

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness();

	// Convert OpenMPT's internal sample representation to an XIInstrumentHeader.
	void ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport);
	// Convert an XIInstrumentHeader to OpenMPT's internal instrument representation.
	void ConvertToMPT(ModInstrument &mptIns) const;
};

STATIC_ASSERT(sizeof(XIInstrumentHeader) == 298);


// XM Sample Header
struct PACKED XMSample
{
	enum XMSampleFlags
	{
		sampleLoop			= 0x01,
		sampleBidiLoop		= 0x02,
		sample16Bit			= 0x10,
		sampleStereo		= 0x20,

		sampleADPCM			= 0xAD,		// MODPlugin :(
	};

	uint32 length;			// Sample Length (in bytes)
	uint32 loopStart;		// Loop Start (in bytes)
	uint32 loopLength;		// Loop Length (in bytes)
	uint8  vol;				// Default Volume
	int8   finetune;		// Sample Finetune
	uint8  flags;			// Sample Flags
	uint8  pan;				// Sample Panning
	int8   relnote;			// Sample Transpose
	uint8  reserved;		// Reserved (abused for ModPlug's ADPCM compression)
	char   name[22];		// Sample Name, not null-terminated (any nulls are treated as spaces)

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness();

	// Convert OpenMPT's internal sample representation to an XMSample.
	void ConvertToXM(const ModSample &mptSmp, MODTYPE fromType, bool compatibilityExport);
	// Convert an XMSample to OpenMPT's internal sample representation.
	void ConvertToMPT(ModSample &mptSmp) const;
	// Retrieve the internal sample format flags for this instrument.
	SampleIO GetSampleFormat() const;
};

STATIC_ASSERT(sizeof(XMSample) == 40);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


OPENMPT_NAMESPACE_END
