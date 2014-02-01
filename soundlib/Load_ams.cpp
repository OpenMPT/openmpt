/*
 * Load_ams.cpp
 * ------------
 * Purpose: AMS (Extreme's Tracker / Velvet Studio) module loader
 * Notes  : Extreme was renamed to Velvet Development at some point,
 *          and thus they also renamed their tracker from
 *          "Extreme's Tracker" to "Velvet Studio".
 *          While the two programs look rather similiar, the structure of both
 *          programs' "AMS" format is significantly different in some places -
 *          Velvet Studio is a rather advanced tracker in comparison to Extreme's Tracker.
 *          The source code of Velvet Studio has been released into the
 *          public domain in 2013: https://github.com/Patosc/VelvetStudio/commits/master
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#include <iterator>

/////////////////////////////////////////////////////////////////////
// Common code for both AMS formats

// Read variable-length AMS string (we ignore the maximum text length specified by the AMS specs and accept any length).
template<size_t destSize>
static bool ReadAMSString(char (&destBuffer)[destSize], FileReader &file)
//-----------------------------------------------------------------------
{
	const size_t length = file.ReadUint8();
	return file.ReadString<mpt::String::spacePadded>(destBuffer, length);
}

// Read variable-length AMS string (we ignore the maximum text length specified by the AMS specs and accept any length).
static bool ReadAMSString(std::string &dest, FileReader &file)
//------------------------------------------------------------
{
	const size_t length = file.ReadUint8();
	return file.ReadString<mpt::String::spacePadded>(dest, length);
}


// Read AMS or AMS2 (newVersion = true) pattern. At least this part of the format is more or less identical between the two trackers...
static void ReadAMSPattern(CPattern &pattern, bool newVersion, FileReader &patternChunk, CSoundFile &sndFile)
//-----------------------------------------------------------------------------------------------------------
{
	enum
	{
		emptyRow		= 0xFF,	// No commands on row
		endOfRowMask	= 0x80,	// If set, no more commands on this row
		noteMask		= 0x40,	// If set, no note+instr in this command
		channelMask		= 0x1F,	// Mask for extracting channel

		// Note flags
		readNextCmd		= 0x80,	// One more command follows
		noteDataMask	= 0x7F,	// Extract note

		// Command flags
		volCommand		= 0x40,	// Effect is compressed volume command
		commandMask		= 0x3F,	// Command or volume mask
	};

	// Effect translation table for extended (non-Protracker) effects
	static const ModCommand::COMMAND effTrans[] =
	{
		CMD_S3MCMDEX,		// Forward / Backward
		CMD_PORTAMENTOUP,	// Extra fine slide up
		CMD_PORTAMENTODOWN,	// Extra fine slide up
		CMD_RETRIG,			// Retrigger
		CMD_NONE,
		CMD_TONEPORTAVOL,	// Toneporta with fine volume slide
		CMD_VIBRATOVOL,		// Vibrato with fine volume slide
		CMD_NONE,
		CMD_PANNINGSLIDE,
		CMD_NONE,
		CMD_VOLUMESLIDE,	// Two times finder volume slide than Axx
		CMD_NONE,
		CMD_CHANNELVOLUME,	// Channel volume (0...127)
		CMD_PATTERNBREAK,	// Long pattern break (in hex)
		CMD_S3MCMDEX,		// Fine slide commands
		CMD_NONE,			// Fractional BPM
		CMD_KEYOFF,			// Key off at tick xx
		CMD_PORTAMENTOUP,	// Porta up, but uses all octaves (?)
		CMD_PORTAMENTODOWN,	// Porta down, but uses all octaves (?)
		CMD_NONE,
		CMD_NONE,
		CMD_NONE,
		CMD_NONE,
		CMD_NONE,
		CMD_NONE,
		CMD_NONE,
		CMD_GLOBALVOLSLIDE,	// Global volume slide
		CMD_NONE,
		CMD_GLOBALVOLUME,	// Global volume (0... 127)
	};

	static ModCommand dummy;

	for(ROWINDEX row = 0; row < pattern.GetNumRows(); row++)
	{
		PatternRow baseRow = pattern.GetRow(row);
		while(patternChunk.AreBytesLeft())
		{
			const uint8 flags = patternChunk.ReadUint8();
			if(flags == emptyRow)
			{
				break;
			}

			const CHANNELINDEX chn = (flags & channelMask);
			ModCommand &m = chn < pattern.GetNumChannels() ? baseRow[chn] : dummy;
			bool moreCommands = true;
			if(!(flags & noteMask))
			{
				// Read note + instr
				uint8 note = patternChunk.ReadUint8();
				moreCommands = (note & readNextCmd) != 0;
				note &= noteDataMask;

				if(note == 1)
				{
					m.note = NOTE_KEYOFF;
				} else if(note >= 2 && note <= 121 && newVersion)
				{
					m.note = note - 2 + NOTE_MIN;
				} else if(note >= 12 && note <= 108 && !newVersion)
				{
					m.note = note + 12 + NOTE_MIN;
				}
				m.instr = patternChunk.ReadUint8();
			}

			while(moreCommands)
			{
				// Read one more effect command
				ModCommand origCmd = m;
				const uint8 command = patternChunk.ReadUint8(), effect = (command & commandMask);
				moreCommands = (command & readNextCmd) != 0;

				if(command & volCommand)
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = effect;
				} else
				{
					m.param = patternChunk.ReadUint8();

					if(effect < 0x10)
					{
						// PT commands
						m.command = effect;
						sndFile.ConvertModCommand(m);

						// Post-fix some commands
						switch(m.command)
						{
						case CMD_PANNING8:
							// 4-Bit panning
							m.command = CMD_PANNING8;
							m.param = (m.param & 0x0F) * 0x11;
							break;

						case CMD_VOLUME:
							m.command = CMD_NONE;
							m.volcmd = VOLCMD_VOLUME;
							m.vol = static_cast<ModCommand::VOL>(std::min((m.param + 1) / 2, 64));
							break;

						case CMD_MODCMDEX:
							if(m.param == 0x80)
							{
								// Break sample loop (cut after loop)
								m.command = CMD_NONE;
							} else
							{
								m.ExtendedMODtoS3MEffect();
							}
							break;
						}
					} else if(effect - 0x10 < (int)CountOf(effTrans))
					{
						// Extended commands
						m.command = effTrans[effect - 0x10];

						// Post-fix some commands
						switch(effect)
						{
						case 0x10:
							// Play sample forwards / backwards
							if(m.param <= 0x01)
							{
								m.param |= 0x9E;
							} else
							{
								m.command = CMD_NONE;
							}
							break;

						case 0x11:
						case 0x12:
							// Extra fine slides
							m.param = static_cast<ModCommand::PARAM>(std::min(uint8(0x0F), m.param) | 0xE0);
							break;

						case 0x15:
						case 0x16:
							// Fine slides
							m.param = static_cast<ModCommand::PARAM>((std::min(0x10, m.param + 1) / 2) | 0xF0);
							break;

						case 0x1E:
							// More fine slides
							switch(m.param >> 4)
							{
							case 0x1:
								// Fine porta up
								m.command = CMD_PORTAMENTOUP;
								m.param |= 0xF0;
								break;
							case 0x2:
								// Fine porta down
								m.command = CMD_PORTAMENTODOWN;
								m.param |= 0xF0;
								break;
							case 0xA:
								// Extra fine volume slide up
								m.command = CMD_VOLUMESLIDE;
								m.param = ((((m.param & 0x0F) + 1) / 2) << 4) | 0x0F;
								break;
							case 0xB:
								// Extra fine volume slide down
								m.command = CMD_VOLUMESLIDE;
								m.param = (((m.param & 0x0F) + 1) / 2) | 0xF0;
								break;
							default:
								m.command = CMD_NONE;
								break;
							}
							break;

						case 0x1C:
							// Adjust channel volume range
							m.param = static_cast<ModCommand::PARAM>(std::min((m.param + 1) / 2, 64));
							break;
						}
					}

					// Try merging commands first
					ModCommand::CombineEffects(m.command, m.param, origCmd.command, origCmd.param);

					if(ModCommand::GetEffectWeight(origCmd.command) > ModCommand::GetEffectWeight(m.command))
					{
						if(m.volcmd == VOLCMD_NONE && ModCommand::ConvertVolEffect(m.command, m.param, true))
						{
							// Volume column to the rescue!
							m.volcmd = m.command;
							m.vol = m.param;
						}

						m.command = origCmd.command;
						m.param = origCmd.param;
					}
				}
			}

			if(flags & endOfRowMask)
			{
				// End of row
				break;
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////
// AMS (Extreme's Tracker) 1.x loader

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// AMS File Header
struct PACKED AMSFileHeader
{
	uint8  versionLow;
	uint8  versionHigh;
	uint8  channelConfig;
	uint8  numSamps;
	uint16 numPats;
	uint16 numOrds;
	uint8  midiChannels;
	uint16 extraSize;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numPats);
		SwapBytesLE(numOrds);
		SwapBytesLE(extraSize);
	}
};

STATIC_ASSERT(sizeof(AMSFileHeader) == 11);


// AMS Sample Header
struct PACKED AMSSampleHeader
{
	enum SampleFlags
	{
		smp16BitOld	= 0x04,	// AMS 1.0 (at least according to docs, I yet have to find such a file)
		smp16Bit	= 0x80,	// AMS 1.1+
		smpPacked	= 0x03,
	};

	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	uint8  panFinetune;		// High nibble = pan position, low nibble = finetune value
	uint16 sampleRate;
	uint8  volume;			// 0...127
	uint8  flags;			// See SampleFlags

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sampleRate);
	}

	// Convert sample header to OpenMPT's internal format.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();

		mptSmp.nLength = length;
		mptSmp.nLoopStart = std::min(loopStart, length);
		mptSmp.nLoopEnd = std::min(loopEnd, length);

		mptSmp.nVolume = (std::min(uint8(127), volume) * 256 + 64) / 127;
		if(panFinetune & 0xF0)
		{
			mptSmp.nPan = (panFinetune & 0xF0);
			mptSmp.uFlags = CHN_PANNING;
		}

		mptSmp.nC5Speed = 2 * sampleRate;
		if(sampleRate == 0)
		{
			mptSmp.nC5Speed = 2 * 8363;
		}

		uint32 newC4speed = ModSample::TransposeToFrequency(0, MOD2XMFineTune(panFinetune & 0x0F));
		mptSmp.nC5Speed = (mptSmp.nC5Speed * newC4speed) / 8363;

		if(mptSmp.nLoopStart < mptSmp.nLoopEnd)
		{
			mptSmp.uFlags.set(CHN_LOOP);
		}

		if((flags & smp16Bit) || (flags & smp16BitOld))
		{
			mptSmp.uFlags.set(CHN_16BIT);
		}
	}
};

STATIC_ASSERT(sizeof(AMSSampleHeader) == 17);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadAMS(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	AMSFileHeader fileHeader;
	if(!file.ReadMagic("Extreme")
		|| !file.ReadConvertEndianness(fileHeader)
		|| !file.Skip(fileHeader.extraSize)
		|| !file.CanRead(fileHeader.numSamps * sizeof(AMSSampleHeader))
		|| fileHeader.versionHigh != 0x01)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();

	m_nType = MOD_TYPE_AMS;
	m_SongFlags = SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS;
	m_nChannels = (fileHeader.channelConfig & 0x1F) + 1;
	m_nSamples = fileHeader.numSamps;
	SetModFlag(MSF_COMPATIBLE_PLAY, true);
	SetupMODPanning(true);
	madeWithTracker = mpt::String::Print("Extreme's Tracker %1.%2", fileHeader.versionHigh, fileHeader.versionLow);

	std::vector<bool> packSample(fileHeader.numSamps);

	STATIC_ASSERT(MAX_SAMPLES > 255);
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		AMSSampleHeader sampleHeader;
		file.ReadConvertEndianness(sampleHeader);
		sampleHeader.ConvertToMPT(Samples[smp]);
		packSample[smp - 1] = (sampleHeader.flags & AMSSampleHeader::smpPacked) != 0;
	}

	// Texts
	ReadAMSString(songName, file);

	// Read sample names
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		ReadAMSString(m_szNames[smp], file);
	}

	// Read channel names
	for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
	{
		ChnSettings[chn].Reset();
		ReadAMSString(ChnSettings[chn].szName, file);
	}

	// Read pattern names
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPats; pat++)
	{
		char name[11];
		ReadAMSString(name, file);
		// Create pattern now, so name won't be reset later.
		if(!Patterns.Insert(pat, 64))
		{
			Patterns[pat].SetName(name);
		}
	}

	// Read packed song message
	const uint16 packedLength = file.ReadUint16LE();
	if(packedLength)
	{
		std::vector<uint8> textIn, textOut;
		file.ReadVector(textIn, packedLength);
		textOut.reserve(packedLength);

		for(std::vector<uint8>::iterator c = textIn.begin(); c != textIn.end(); c++)
		{
			if(*c & 0x80)
			{
				textOut.insert(textOut.end(), (*c & 0x7F), ' ');
			} else
			{
				textOut.push_back(*c);
			}
		}

		std::string str;
		std::copy(textOut.begin(), textOut.end(), std::back_inserter(str));
		str = mpt::To(mpt::CharsetCP437, mpt::CharsetCP437AMS, str);

		// Packed text doesn't include any line breaks!
		songMessage.ReadFixedLineLength(str.c_str(), str.length(), 76, 0);
	}

	// Read Order List
	std::vector<uint16> orders;
	if(file.ReadVectorLE(orders, fileHeader.numOrds))
	{
		Order.resize(fileHeader.numOrds);
		for(size_t i = 0; i < fileHeader.numOrds; i++)
		{
			Order[i] = orders[i];
		}
	}

	// Read patterns
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPats; pat++)
	{
		uint32 patLength = file.ReadUint32LE();
		FileReader patternChunk = file.GetChunk(patLength);

		if(loadFlags & loadPatternData)
		{
			ReadAMSPattern(Patterns[pat], false, patternChunk, *this);
		}
	}

	if(loadFlags & loadSampleData)
	{
		// Read Samples
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			SampleIO(
				Samples[smp].uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				packSample[smp - 1] ? SampleIO::AMS : SampleIO::signedPCM)
				.ReadSample(Samples[smp], file);
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////////////
// AMS (Velvet Studio) 2.0 - 2.02 loader


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// AMS2 File Header
struct PACKED AMS2FileHeader
{
	enum FileFlags
	{
		linearSlides	= 0x40,
	};

	uint8  versionLow;		// Version of format (Hi = MainVer, Low = SubVer e.g. 0202 = 2.02)
	uint8  versionHigh;		// ditto
	uint8  numIns;			// Nr of Instruments (0-255)
	uint16 numPats;			// Nr of Patterns (1-1024)
	uint16 numOrds;			// Nr of Positions (1-65535)
	// Rest of header differs between format revision 2.01 and 2.02

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numPats);
		SwapBytesLE(numOrds);
	};
};

STATIC_ASSERT(sizeof(AMS2FileHeader) == 7);


// AMS2 Instument Envelope 
struct PACKED AMS2Envelope
{
	uint8 speed;		// Envelope speed (currently not supported, always the same as current BPM)
	uint8 sustainPoint;	// Envelope sustain point
	uint8 loopStart;	// Envelope loop Start
	uint8 loopEnd;		// Envelope loop End
	uint8 numPoints;	// Envelope length

	// Read envelope and do partial conversion.
	void ConvertToMPT(InstrumentEnvelope &mptEnv, FileReader &file)
	{
		file.Read(*this);

		// Read envelope points
		uint8 data[64][3];
		file.ReadStructPartial(data, numPoints * 3);

		if(numPoints <= 1)
		{
			// This is not an envelope.
			return;
		}

		STATIC_ASSERT(MAX_ENVPOINTS >= CountOf(data));
		mptEnv.nNodes = std::min(numPoints, uint8(CountOf(data)));
		mptEnv.nLoopStart = loopStart;
		mptEnv.nLoopEnd = loopEnd;
		mptEnv.nSustainStart = mptEnv.nSustainEnd = sustainPoint;

		for(size_t i = 0; i < mptEnv.nNodes; i++)
		{
			if(i != 0)
			{
				mptEnv.Ticks[i] = mptEnv.Ticks[i - 1] + static_cast<uint16>(std::max(1, data[i][0] | ((data[i][1] & 0x01) << 8)));
			}
			mptEnv.Values[i] = data[i][2];
		}
	}
};

STATIC_ASSERT(sizeof(AMS2Envelope) == 5);


// AMS2 Instrument Data
struct PACKED AMS2Instrument
{
	enum EnvelopeFlags
	{
		envLoop		= 0x01,
		envSustain	= 0x02,
		envEnabled	= 0x04,
		
		// Flag shift amounts
		volEnvShift	= 0,
		panEnvShift	= 1,
		vibEnvShift	= 2,

		vibAmpMask	= 0x3000,
		vibAmpShift	= 12,
		fadeOutMask	= 0xFFF,
	};

	uint8  shadowInstr;		// Shadow Instrument. If non-zero, the value=the shadowed inst.
	uint16 vibampFadeout;	// Vib.Amplify + Volume fadeout in one variable!
	uint16 envFlags;		// See EnvelopeFlags

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(vibampFadeout);
		SwapBytesLE(envFlags);
	}

	void ApplyFlags(InstrumentEnvelope &mptEnv, EnvelopeFlags shift) const
	{
		const int flags = envFlags >> (shift * 3);
		mptEnv.dwFlags.set(ENV_ENABLED, (flags & envEnabled) != 0);
		mptEnv.dwFlags.set(ENV_LOOP, (flags & envLoop) != 0);
		mptEnv.dwFlags.set(ENV_SUSTAIN, (flags & envSustain) != 0);

		// "Break envelope" should stop the envelope loop when encountering a note-off... We can only use the sustain loop to emulate this behaviour.
		if(!(flags & envSustain) && (flags & envLoop) != 0 && (flags & (1 << (9 - shift * 2))) != 0)
		{
			mptEnv.nSustainStart = mptEnv.nLoopStart;
			mptEnv.nSustainEnd = mptEnv.nLoopEnd;
			mptEnv.dwFlags.set(ENV_SUSTAIN);
			mptEnv.dwFlags.reset(ENV_LOOP);
		}
	}

};

STATIC_ASSERT(sizeof(AMS2Instrument) == 5);


// AMS2 Sample Header
struct PACKED AMS2SampleHeader
{
	enum SampleFlags
	{
		smpPacked	= 0x03,
		smp16Bit	= 0x04,
		smpLoop		= 0x08,
		smpBidiLoop	= 0x10,
		smpReverse	= 0x40,
	};

	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	uint16 sampledRate;		// Whyyyy?
	uint8  panFinetune;		// High nibble = pan position, low nibble = finetune value
	uint16 c4speed;			// Why is all of this so redundant?
	int8   relativeTone;	// q.e.d.
	uint8  volume;			// 0...127
	uint8  flags;			// See SampleFlags

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sampledRate);
		SwapBytesLE(c4speed);
	}

	// Convert sample header to OpenMPT's internal format.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();

		mptSmp.nLength = length;
		mptSmp.nLoopStart = std::min(loopStart, length);
		mptSmp.nLoopEnd = std::min(loopEnd, length);

		mptSmp.nC5Speed = c4speed * 2;
		if(c4speed == 0)
		{
			mptSmp.nC5Speed = 8363 * 2;
		}
		// Why, oh why, does this format need a c5speed and transpose/finetune at the same time...
		uint32 newC4speed = ModSample::TransposeToFrequency(relativeTone, MOD2XMFineTune(panFinetune & 0x0F));
		mptSmp.nC5Speed = (mptSmp.nC5Speed * newC4speed) / 8363;

		mptSmp.nVolume = (std::min(uint8(127), volume) * 256 + 64) / 127;
		if(panFinetune & 0xF0)
		{
			mptSmp.nPan = (panFinetune & 0xF0);
			mptSmp.uFlags = CHN_PANNING;
		}

		if(flags & smp16Bit)
		{
			mptSmp.uFlags.set(CHN_16BIT);
		}
		if((flags & smpLoop) && mptSmp.nLoopStart < mptSmp.nLoopEnd)
		{
			mptSmp.uFlags.set(CHN_LOOP);
			if(flags & smpBidiLoop)
			{
				mptSmp.uFlags.set(CHN_PINGPONGLOOP);
			}
			if(flags & smpReverse)
			{
				mptSmp.uFlags.set(CHN_REVRSE);
			}
		}
	}
};

STATIC_ASSERT(sizeof(AMS2SampleHeader) == 20);


// AMS2 Song Description Header
struct PACKED AMS2Description
{
	uint32 packedLen;		// Including header
	uint32 unpackedLen;
	uint8  packRoutine;		// 01
	uint8  preProcessing;	// None!
	uint8  packingMethod;	// RLE

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(packedLen);
		SwapBytesLE(unpackedLen);
	}
};

STATIC_ASSERT(sizeof(AMS2Description) == 11);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadAMS2(FileReader &file, ModLoadingFlags loadFlags)
//--------------------------------------------------------------------
{
	file.Rewind();

	AMS2FileHeader fileHeader;
	if(!file.ReadMagic("AMShdr\x1A")
		|| !ReadAMSString(songName, file)
		|| !file.ReadConvertEndianness(fileHeader)
		|| fileHeader.versionHigh != 2 || fileHeader.versionLow > 2)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}
	
	InitializeGlobals();

	m_nType = MOD_TYPE_AMS2;
	m_nInstruments = fileHeader.numIns;
	m_nChannels = 32;
	SetModFlag(MSF_COMPATIBLE_PLAY, true);
	SetupMODPanning(true);
	madeWithTracker = mpt::String::Print("Velvet Studio %1.%2", fileHeader.versionHigh, mpt::fmt::dec0<2>(fileHeader.versionLow));

	uint16 headerFlags;
	if(fileHeader.versionLow >= 2)
	{
		m_nDefaultTempo = std::max(uint8(32), static_cast<uint8>(file.ReadUint16LE() >> 8));	// 16.16 Tempo
		m_nDefaultSpeed = std::max(uint8(1), file.ReadUint8());
		file.Skip(3);	// Default values for pattern editor
		headerFlags = file.ReadUint16LE();
	} else
	{
		m_nDefaultTempo = std::max(uint8(32), file.ReadUint8());
		m_nDefaultSpeed = std::max(uint8(1), file.ReadUint8());
		headerFlags = file.ReadUint8();
	}

	m_SongFlags = SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS | ((headerFlags & AMS2FileHeader::linearSlides) ? SONG_LINEARSLIDES : SongFlags(0));

	// Instruments
	std::vector<SAMPLEINDEX> firstSample;	// First sample of instrument
	std::vector<uint16> sampleSettings;		// Shadow sample map... Lo byte = Instrument, Hi byte, lo nibble = Sample index in instrument, Hi byte, hi nibble = Sample pack status
	enum
	{
		instrIndexMask		= 0xFF,		// Shadow instrument
		sampleIndexMask		= 0x7F00,	// Sample index in instrument
		sampleIndexShift	= 8,
		packStatusMask		= 0x8000,	// If bit is set, sample is packed
	};

	STATIC_ASSERT(MAX_INSTRUMENTS > 255);
	for(INSTRUMENTINDEX ins = 1; ins <= m_nInstruments; ins++)
	{
		ModInstrument *instrument = AllocateInstrument(ins);
		if(instrument == nullptr
			|| !ReadAMSString(instrument->name, file))
		{
			return false;
		}

		uint8 numSamples = file.ReadUint8();
		uint8 sampleAssignment[120];
		MemsetZero(sampleAssignment);	// Only really needed for v2.0, where the lowest and highest octave aren't cleared.

		if(numSamples == 0
			|| (fileHeader.versionLow > 0 && !file.ReadArray(sampleAssignment))	// v2.01+: 120 Notes
			|| (fileHeader.versionLow == 0 && !file.ReadArray(reinterpret_cast<uint8 (&) [96]>(sampleAssignment[12]))))	// v2.0: 96 Notes
		{
			continue;
		}

		STATIC_ASSERT(CountOf(instrument->Keyboard) >= CountOf(sampleAssignment));
		for(size_t i = 0; i < 120; i++)
		{
			instrument->Keyboard[i] = sampleAssignment[i] + GetNumSamples() + 1;
		}

		AMS2Envelope volEnv, panEnv, vibratoEnv;
		volEnv.ConvertToMPT(instrument->VolEnv, file);
		panEnv.ConvertToMPT(instrument->PanEnv, file);
		vibratoEnv.ConvertToMPT(instrument->PitchEnv, file);

		AMS2Instrument instrHeader;
		file.ReadConvertEndianness(instrHeader);
		instrument->nFadeOut = (instrHeader.vibampFadeout & AMS2Instrument::fadeOutMask);
		const int16 vibAmp = 1 << ((instrHeader.vibampFadeout & AMS2Instrument::vibAmpMask) >> AMS2Instrument::vibAmpShift);

		instrHeader.ApplyFlags(instrument->VolEnv, AMS2Instrument::volEnvShift);
		instrHeader.ApplyFlags(instrument->PanEnv, AMS2Instrument::panEnvShift);
		instrHeader.ApplyFlags(instrument->PitchEnv, AMS2Instrument::vibEnvShift);

		// Scale envelopes to correct range
		for(size_t i = 0; i < MAX_ENVPOINTS; i++)
		{
			instrument->VolEnv.Values[i] = std::min(uint8(ENVELOPE_MAX), static_cast<uint8>((instrument->VolEnv.Values[i] * ENVELOPE_MAX + 64u) / 127u));
			instrument->PanEnv.Values[i] = std::min(uint8(ENVELOPE_MAX), static_cast<uint8>((instrument->PanEnv.Values[i] * ENVELOPE_MAX + 128u) / 255u));
#ifdef MODPLUG_TRACKER
			instrument->PitchEnv.Values[i] = std::min(uint8(ENVELOPE_MAX), static_cast<uint8>(32 + Util::muldivrfloor(static_cast<int8>(instrument->PitchEnv.Values[i] - 128), vibAmp, 255)));
#else
			// Try to keep as much precision as possible... divide by 8 since that's the highest possible vibAmp factor.
			instrument->PitchEnv.Values[i] = static_cast<uint8>(128 + Util::muldivrfloor(static_cast<int8>(instrument->PitchEnv.Values[i] - 128), vibAmp, 8));
#endif
		}

		// Sample headers - we will have to read them even for shadow samples, and we will have to load them several times,
		// as it is possible that shadow samples use different sample settings like base frequency or panning.
		const SAMPLEINDEX firstSmp = GetNumSamples() + 1;
		for(SAMPLEINDEX smp = 0; smp < numSamples; smp++)
		{
			if(firstSmp + smp >= MAX_SAMPLES)
			{
				file.Skip(sizeof(AMS2SampleHeader));
				break;
			}
			ReadAMSString(m_szNames[firstSmp + smp], file);

			AMS2SampleHeader sampleHeader;
			file.ReadConvertEndianness(sampleHeader);
			sampleHeader.ConvertToMPT(Samples[firstSmp + smp]);

			uint16 settings = (instrHeader.shadowInstr & instrIndexMask)
				| ((smp << sampleIndexShift) & sampleIndexMask)
				| ((sampleHeader.flags & AMS2SampleHeader::smpPacked) ? packStatusMask : 0);
			sampleSettings.push_back(settings);
		}

		firstSample.push_back(firstSmp);
		m_nSamples = static_cast<SAMPLEINDEX>(std::min(MAX_SAMPLES - 1, GetNumSamples() + numSamples));
	}

	// Text

	// Read composer name
	uint8 composerLength = file.ReadUint8();
	if(composerLength)
	{
		std::string str;
		file.ReadString<mpt::String::spacePadded>(str, composerLength);
		str = mpt::To(mpt::CharsetCP437, mpt::CharsetCP437AMS2, str);
		songMessage.Read(str.c_str(), str.length(), SongMessage::leAutodetect);
		songArtist = str;
	}

	// Channel names
	for(CHANNELINDEX chn = 0; chn < 32; chn++)
	{
		ChnSettings[chn].Reset();
		ReadAMSString(ChnSettings[chn].szName, file);
	}

	// RLE-Packed description text
	AMS2Description descriptionHeader;
	if(!file.ReadConvertEndianness(descriptionHeader))
	{
		return true;
	}
	if(descriptionHeader.packedLen)
	{
		const size_t textLength = descriptionHeader.packedLen - sizeof(descriptionHeader);
		std::vector<uint8> textIn, textOut(descriptionHeader.unpackedLen);
		file.ReadVector(textIn, textLength);

		size_t readLen = 0, writeLen = 0;
		while(readLen < textLength && writeLen < descriptionHeader.unpackedLen)
		{
			uint8 c = textIn[readLen++];
			if(c == 0xFF && textLength - readLen >= 2)
			{
				c = textIn[readLen++];
				uint32 count = textIn[readLen++];
				for(size_t i = std::min<size_t>(descriptionHeader.unpackedLen - writeLen, count); i != 0; i--)
				{
					textOut[writeLen++] = c;
				}
			} else
			{
				textOut[writeLen++] = c;
			}
		}
		std::string str;
		std::copy(textOut.begin(), textOut.begin() + descriptionHeader.unpackedLen, std::back_inserter(str));
		str = mpt::To(mpt::CharsetCP437, mpt::CharsetCP437AMS2, str);
		// Packed text doesn't include any line breaks!
		songMessage.ReadFixedLineLength(str.c_str(), str.length(), 74, 0);
	}

	// Read Order List
	std::vector<uint16> orders;
	if(file.ReadVectorLE(orders, fileHeader.numOrds))
	{
		Order.resize(fileHeader.numOrds);
		for(size_t i = 0; i < fileHeader.numOrds; i++)
		{
			Order[i] = orders[i];
		}
	}

	// Read Patterns
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPats; pat++)
	{
		uint32 patLength = file.ReadUint32LE();
		FileReader patternChunk = file.GetChunk(patLength);

		if(loadFlags & loadPatternData)
		{
			const ROWINDEX numRows = patternChunk.ReadUint8() + 1;
			// We don't need to know the number of channels or commands.
			patternChunk.Skip(1);

			if(Patterns.Insert(pat, numRows))
			{
				continue;
			}

			char patternName[11];
			ReadAMSString(patternName, patternChunk);
			Patterns[pat].SetName(patternName);

			ReadAMSPattern(Patterns[pat], true, patternChunk, *this);
		}
	}

	if(!(loadFlags & loadSampleData))
	{
		return true;
	}

	// Read Samples
	for(SAMPLEINDEX smp = 0; smp < GetNumSamples(); smp++)
	{
		if((sampleSettings[smp] & instrIndexMask) == 0)
		{
			// Only load samples that aren't part of a shadow instrument
			SampleIO(
				(Samples[smp + 1].uFlags & CHN_16BIT) ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				(sampleSettings[smp] & packStatusMask) ? SampleIO::AMS : SampleIO::signedPCM)
				.ReadSample(Samples[smp + 1], file);
		}
	}

	// Copy shadow samples
	for(SAMPLEINDEX smp = 0; smp < GetNumSamples(); smp++)
	{
		INSTRUMENTINDEX sourceInstr = (sampleSettings[smp] & instrIndexMask);
		if(sourceInstr == 0
			|| --sourceInstr >= firstSample.size())
		{
				continue;
		}

		SAMPLEINDEX sourceSample = ((sampleSettings[smp] & sampleIndexMask) >> sampleIndexShift) + firstSample[sourceInstr];
		if(sourceSample > GetNumSamples())
		{
			continue;
		}

		// Copy over original sample
		ModSample &sample = Samples[smp + 1];
		ModSample &source = Samples[sourceSample];
		if(source.uFlags & CHN_16BIT)
		{
			sample.uFlags |= CHN_16BIT;
		} else
		{
			sample.uFlags &= ~CHN_16BIT;
		}
		sample.nLength = source.nLength;
		if(sample.AllocateSample())
		{
			memcpy(sample.pSample, source.pSample, source.GetSampleSizeInBytes());
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////////////
// AMS Sample unpacking

void AMSUnpack(const int8 * const source, size_t sourceSize, void * const dest, const size_t destSize, char packCharacter)
//------------------------------------------------------------------------------------------------------------------------
{
	int8 *tempBuf = new (std::nothrow) int8[destSize];
	if(tempBuf == nullptr)
	{
		return;
	}

	// Unpack Loop
	{
		const int8 *in = source;
		int8 *out = tempBuf;

		size_t i = sourceSize, j = destSize;
		while(i != 0 && j != 0)
		{
			int8 ch = *(in++);
			if(--i != 0 && ch == packCharacter)
			{
				uint8 repCount = *(in++);
				repCount = static_cast<uint8>(std::min(static_cast<size_t>(repCount), j));
				if(--i != 0 && repCount)
				{
					ch = *(in++);
					i--;
					while(repCount-- != 0)
					{
						*(out++) = ch;
						j--;
					}
				} else
				{
					*(out++) = packCharacter;
					j--;
				}
			} else
			{
				*(out++) = ch;
				j--;
			}
		}
	}

	// Bit Unpack Loop
	{
		int8 *out = tempBuf;
		uint16 bitcount = 0x80;
		size_t k = 0;
		uint8 *dst = static_cast<uint8 *>(dest);
		for(size_t i = 0; i < destSize; i++)
		{
			uint8 al = *out++;
			uint16 dh = 0;
			for(uint16 count = 0; count < 8; count++)
			{
				uint16 bl = al & bitcount;
				bl = ((bl | (bl << 8)) >> ((dh + 8 - count) & 7)) & 0xFF;
				bitcount = ((bitcount | (bitcount << 8)) >> 1) & 0xFF;
				dst[k++] |= bl;
				if(k >= destSize)
				{
					k = 0;
					dh++;
				}
			}
			bitcount = ((bitcount | (bitcount << 8)) >> dh) & 0xFF;
		}
	}

	// Delta Unpack
	{
		int8 old = 0;
		int8 *out = static_cast<int8 *>(dest);
		for(size_t i = destSize; i != 0; i--)
		{
			int pos = *reinterpret_cast<uint8 *>(out);
			if(pos != 128 && (pos & 0x80) != 0)
			{
				pos = -(pos & 0x7F);
			}
			old -= static_cast<int8>(pos);
			*(out++) = old;
		}
	}

	delete[] tempBuf;
}
