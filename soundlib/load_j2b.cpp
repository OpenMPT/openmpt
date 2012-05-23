/*
 * load_j2b.cpp
 * ------------
 * Purpose: RIFF AM and RIFF AMFF (Galaxy Sound System) module loader
 * Notes  : J2B is a compressed variant of RIFF AM and RIFF AMFF files used in Jazz Jackrabbit 2.
 *          It seems like no other game used the AM(FF) format.
 *          RIFF AM is the newer version of the format, generally following the RIFF "standard" closely.
 * Authors: Johannes Schultz (OpenMPT port, reverse engineering + loader implementation of the instrument format)
 *          Chris Moeller (foo_dumb - this is almost a complete port of his code, thanks)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#ifndef ZLIB_WINAPI
#define ZLIB_WINAPI
#endif // ZLIB_WINAPI
#include "../zlib/zlib.h"


// First off, a nice vibrato translation LUT.
static const uint8 j2bAutoVibratoTrans[] =
{
	VIB_SINE, VIB_SQUARE, VIB_RAMP_UP, VIB_RAMP_DOWN, VIB_RANDOM,
};

#pragma pack(push, 1)

// header for compressed j2b files
struct J2BFileHeader
{
	// Magic Bytes
	enum J2BMagic
	{
		// 32-Bit J2B header identifiers
		magicMUSE		= 0x4553554D,
		magicDEADBEAF	= 0xAFBEADDE,
		magicDEADBABE	= 0xBEBAADDE,
	};

	uint32 signature;		// MUSE
	uint32 deadbeaf;		// 0xDEADBEAF (AM) or 0xDEADBABE (AMFF)
	uint32 fileLength;		// complete filesize
	uint32 crc32;			// checksum of the compressed data block
	uint32 packedLength;	// length of the compressed data block
	uint32 unpackedLength;	// length of the decompressed module

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(signature);
		SwapBytesLE(deadbeaf);
		SwapBytesLE(fileLength);
		SwapBytesLE(crc32);
		SwapBytesLE(packedLength);
		SwapBytesLE(unpackedLength);
	}
};

// AM(FF) stuff

struct AMFFRiffChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idRIFF	= 0x46464952,
		idAMFF	= 0x46464D41,
		idAM__	= 0x20204D41,
		idMAIN	= 0x4E49414D,
		idINIT	= 0x54494E49,
		idORDR	= 0x5244524F,
		idPATT	= 0x54544150,
		idINST	= 0x54534E49,
		idSAMP	= 0x504D4153,
		idAI__	= 0x20204941,
		idAS__	= 0x20205341,
	};

	uint32 id;		// See ChunkIdentifiers
	uint32 length;	// Chunk size without header

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(length);
	}
};


// This header is used for both AM's "INIT" as well as AMFF's "MAIN" chunk
struct AMFFMainChunk
{
	// Main Chunk flags
	enum MainFlags
	{
		amigaSlides = 0x01,
	};

	char   songname[64];
	uint8  flags;
	uint8  channels;
	uint8  speed;
	uint8  tempo;
	uint32 unknown;		// 0x16078035 if original file was MOD, 0xC50100FF for everything else? it's 0xFF00FFFF in Carrotus.j2b (AMFF version)
	uint8  globalvolume;
};


// AMFF instrument envelope point (old format)
struct AMFFEnvelopePoint
{
	uint16 tick;
	uint8  value;	// 0...64

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(tick);
	}
};


// AMFF instrument envelope (old format)
struct AMFFEnvelope
{
	// Envelope flags (also used for RIFF AM)
	enum EnvelopeFlags
	{
		envEnabled	= 0x01,
		envSustain	= 0x02,
		envLoop		= 0x04,
	};

	uint8  envFlags;			// high nibble = pan env flags, low nibble = vol env flags (both nibbles work the same way)
	uint8  envNumPoints;		// high nibble = pan env length, low nibble = vol env length
	uint8  envSustainPoints;	// you guessed it... high nibble = pan env sustain point, low nibble = vol env sustain point
	uint8  envLoopStarts;		// i guess you know the pattern now.
	uint8  envLoopEnds;			// same here.
	AMFFEnvelopePoint volEnv[10];
	AMFFEnvelopePoint panEnv[10];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		for(size_t i = 0; i < CountOf(volEnv); i++)
		{
			volEnv[i].ConvertEndianness();
			panEnv[i].ConvertEndianness();
		}
	}

	// Convert weird envelope data to OpenMPT's internal format.
	void ConvertEnvelope(uint8 flags, uint8 numPoints, uint8 sustainPoint, uint8 loopStart, uint8 loopEnd, const AMFFEnvelopePoint *points, InstrumentEnvelope &mptEnv) const
	{
		mptEnv.dwFlags = (flags & AMFFEnvelope::envEnabled) ? ENV_ENABLED : 0;
		if(flags & AMFFEnvelope::envSustain) mptEnv.dwFlags |= ENV_SUSTAIN;
		if(flags & AMFFEnvelope::envLoop) mptEnv.dwFlags |= ENV_LOOP;

		// The buggy mod2j2b converter will actually NOT limit this to 10 points if the envelope is longer.
		mptEnv.nNodes = Util::Min(numPoints, static_cast<uint8>(10));

		mptEnv.nSustainStart = mptEnv.nSustainEnd = sustainPoint;
		if(mptEnv.nSustainStart > mptEnv.nNodes)
			mptEnv.dwFlags &= ~ENV_SUSTAIN;

		mptEnv.nLoopStart = loopStart;
		mptEnv.nLoopEnd = loopEnd;
		if(mptEnv.nLoopStart > mptEnv.nLoopEnd || mptEnv.nLoopStart > mptEnv.nNodes)
		{
			mptEnv.dwFlags &= ~ENV_LOOP;
		}

		for(size_t i = 0; i < 10; i++)
		{
			mptEnv.Ticks[i] = points[i].tick >> 4;
			if(i == 0)
				mptEnv.Ticks[0] = 0;
			else if(mptEnv.Ticks[i] < mptEnv.Ticks[i - 1])
				mptEnv.Ticks[i] = mptEnv.Ticks[i - 1] + 1;

			mptEnv.Values[i] = Clamp(points[i].value, uint8(0), uint8(0x40));
		}
	}

	void ConvertToMPT(ModInstrument &mptIns) const
	{
		// interleaved envelope data... meh. gotta split it up here and decode it separately.
		// note: mod2j2b is BUGGY and always writes ($original_num_points & 0x0F) in the header,
		// but just has room for 10 envelope points. That means that long (>= 16 points)
		// envelopes are cut off, and envelopes have to be trimmed to 10 points, even if
		// the header claims that they are longer.
		ConvertEnvelope(envFlags & 0x0F, envNumPoints & 0x0F, envSustainPoints & 0x0F, envLoopStarts & 0x0F, envLoopEnds & 0x0F, volEnv, mptIns.VolEnv);
		ConvertEnvelope(envFlags >> 4, envNumPoints >> 4, envSustainPoints >> 4, envLoopStarts >> 4, envLoopEnds >> 4, panEnv, mptIns.PanEnv);
	}
};


// AMFF instrument header (old format)
struct AMFFInstrumentHeader
{
	uint8  unknown;				// 0x00
	uint8  index;				// actual instrument number
	char   name[28];
	uint8  numSamples;
	uint8  sampleMap[120];
	uint8  vibratoType;
	uint16 vibratoSweep;
	uint16 vibratoDepth;
	uint16 vibratoRate;
	AMFFEnvelope envelopes;
	uint16 fadeout;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(vibratoSweep);
		SwapBytesLE(vibratoDepth);
		SwapBytesLE(vibratoRate);
		envelopes.ConvertEndianness();
		SwapBytesLE(fadeout);
	}

	// Convert instrument data to OpenMPT's internal format.
	void ConvertToMPT(ModInstrument &mptIns, SAMPLEINDEX baseSample)
	{
		StringFixer::ReadString<StringFixer::maybeNullTerminated>(mptIns.name, name);

		STATIC_ASSERT(CountOf(sampleMap) <= CountOf(mptIns.Keyboard));
		for(size_t i = 0; i < CountOf(sampleMap); i++)
		{
			mptIns.Keyboard[i] = sampleMap[i] + baseSample + 1;
		}

		mptIns.nFadeOut = fadeout << 5;
		envelopes.ConvertToMPT(mptIns);
	}

};


// AMFF sample header (old format)
struct AMFFSampleHeader
{
	// Sample flags (also used for RIFF AM)
	enum SampleFlags
	{
		smp16Bit	= 0x04,
		smpLoop		= 0x08,
		smpPingPong	= 0x10,
		smpPanning	= 0x20,
		smpExists	= 0x80,
		// some flags are still missing... what is e.g. 0x8000?
	};

	uint32 id;	// "SAMP"
	uint32 chunkSize;	// header + sample size
	char   name[28];
	uint8  pan;
	uint8  volume;
	uint16 flags;
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	uint32 sampleRate;
	uint32 reserved1;
	uint32 reserved2;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(id);
		SwapBytesLE(chunkSize);
		SwapBytesLE(flags);
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sampleRate);
	}

	// Convert sample header to OpenMPT's internal format.
	void ConvertToMPT(AMFFInstrumentHeader &instrHeader, ModSample &mptSmp) const
	{
		mptSmp.nPan = pan * 4;
		mptSmp.nVolume = volume * 4;
		mptSmp.nGlobalVol = 64;
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		mptSmp.nC5Speed = sampleRate;

		if(instrHeader.vibratoType < CountOf(j2bAutoVibratoTrans))
			mptSmp.nVibType = j2bAutoVibratoTrans[instrHeader.vibratoType];
		mptSmp.nVibSweep = static_cast<uint8>(instrHeader.vibratoSweep);
		mptSmp.nVibRate = static_cast<uint8>(instrHeader.vibratoRate / 16);
		mptSmp.nVibDepth = static_cast<uint8>(instrHeader.vibratoDepth / 4);
		if((mptSmp.nVibRate | mptSmp.nVibDepth) != 0)
		{
			// Convert XM-style vibrato sweep to IT
			mptSmp.nVibSweep = 255 - mptSmp.nVibSweep;
		}

		if(flags & AMFFSampleHeader::smp16Bit)
			mptSmp.uFlags |= CHN_16BIT;
		if(flags & AMFFSampleHeader::smpLoop)
			mptSmp.uFlags |= CHN_LOOP;
		if(flags & AMFFSampleHeader::smpPingPong)
			mptSmp.uFlags |= CHN_PINGPONGLOOP;
		if(flags & AMFFSampleHeader::smpPanning)
			mptSmp.uFlags |= CHN_PANNING;
	}

	// Retrieve the internal sample format flags for this sample.
	SampleIO GetSampleFormat() const
	{
		return SampleIO(
			(flags & AMFFSampleHeader::smp16Bit) ? SampleIO::_16bit : SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);
	}
};


// AM instrument envelope point (new format)
struct AMEnvelopePoint
{
	uint16 tick;
	uint16 value;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(tick);
		SwapBytesLE(value);
	}
};


// AM instrument envelope (new format)
struct AMEnvelope
{
	uint16 flags;
	uint8  numPoints;	// actually, it's num. points - 1, and 0xFF if there is no envelope
	uint8  sustainPoint;
	uint8  loopStart;
	uint8  loopEnd;
	AMEnvelopePoint values[10];
	uint16 fadeout;		// why is this here? it's only needed for the volume envelope...

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(flags);
		for(size_t i = 0; i < CountOf(values); i++)
		{
			values[i].ConvertEndianness();
		}
		SwapBytesLE(fadeout);
	}

	// Convert envelope data to OpenMPT's internal format.
	void ConvertToMPT(InstrumentEnvelope &mptEnv, enmEnvelopeTypes envType) const
	{
		if(numPoints == 0xFF || numPoints == 0)
			return;

		mptEnv.dwFlags = (flags & AMFFEnvelope::envEnabled) ? ENV_ENABLED : 0;
		if(flags & AMFFEnvelope::envSustain) mptEnv.dwFlags |= ENV_SUSTAIN;
		if(flags & AMFFEnvelope::envLoop) mptEnv.dwFlags |= ENV_LOOP;

		mptEnv.nNodes = Util::Min(numPoints + 1, 10);

		mptEnv.nSustainStart = mptEnv.nSustainEnd = sustainPoint;
		if(mptEnv.nSustainStart > mptEnv.nNodes)
			mptEnv.dwFlags &= ~ENV_SUSTAIN;

		mptEnv.nLoopStart = loopStart;
		mptEnv.nLoopEnd = loopEnd;
		if(mptEnv.nLoopStart > mptEnv.nLoopEnd || mptEnv.nLoopStart > mptEnv.nNodes)
			mptEnv.dwFlags &= ~ENV_LOOP;

		for(size_t i = 0; i < 10; i++)
		{
			mptEnv.Ticks[i] = values[i].tick >> 4;
			if(i == 0)
				mptEnv.Ticks[i] = 0;
			else if(mptEnv.Ticks[i] < mptEnv.Ticks[i - 1])
				mptEnv.Ticks[i] = mptEnv.Ticks[i - 1] + 1;

			const uint16 val = values[i].value;
			switch(envType)
			{
			case ENV_VOLUME:	// 0....32767
				mptEnv.Values[i] = (BYTE)((val + 1) >> 9);
				break;
			case ENV_PITCH:		// -4096....4096
				mptEnv.Values[i] = (BYTE)((((int16)val) + 0x1001) >> 7);
				break;
			case ENV_PANNING:	// -32768...32767
				mptEnv.Values[i] = (BYTE)((((int16)val) + 0x8001) >> 10);
				break;
			}
			Limit(mptEnv.Values[i], BYTE(ENVELOPE_MIN), BYTE(ENVELOPE_MAX));
		}
	}
};


// AM instrument header (new format)
struct AMInstrumentHeader
{
	uint32 headSize;	// Header size (i.e. the size of this struct)
	uint8  unknown1;	// 0x00
	uint8  index;		// Actual instrument number
	char   name[32];
	uint8  sampleMap[128];
	uint8  vibratoType;
	uint16 vibratoSweep;
	uint16 vibratoDepth;
	uint16 vibratoRate;
	uint8  unknown2[7];
	AMEnvelope volEnv;
	AMEnvelope pitchEnv;
	AMEnvelope panEnv;
	uint16 numSamples;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(headSize);
		SwapBytesLE(vibratoSweep);
		SwapBytesLE(vibratoDepth);
		SwapBytesLE(vibratoRate);
		volEnv.ConvertEndianness();
		pitchEnv.ConvertEndianness();
		panEnv.ConvertEndianness();
		SwapBytesLE(numSamples);
	}

	// Convert instrument data to OpenMPT's internal format.
	void ConvertToMPT(ModInstrument &mptIns, SAMPLEINDEX baseSample)
	{
		StringFixer::ReadString<StringFixer::maybeNullTerminated>(mptIns.name, name);

		STATIC_ASSERT(CountOf(sampleMap) <= CountOf(mptIns.Keyboard));
		for(BYTE i = 0; i < CountOf(sampleMap); i++)
		{
			mptIns.Keyboard[i] = sampleMap[i] + baseSample + 1;
		}

		mptIns.nFadeOut = volEnv.fadeout << 5;

		volEnv.ConvertToMPT(mptIns.VolEnv, ENV_VOLUME);
		pitchEnv.ConvertToMPT(mptIns.PitchEnv, ENV_PITCH);
		panEnv.ConvertToMPT(mptIns.PanEnv, ENV_PANNING);

		if(numSamples == 0)
		{
			MemsetZero(mptIns.Keyboard);
		}
	}
};


// AM sample header (new format)
struct AMSampleHeader
{
	uint32 headSize;	// Header size (i.e. the size of this struct), apparently not including headSize.
	char   name[32];
	uint16 pan;
	uint16 volume;
	uint16 flags;
	uint16 unkown;		// 0x0000 / 0x0080?
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	uint32 sampleRate;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(headSize);
		SwapBytesLE(pan);
		SwapBytesLE(volume);
		SwapBytesLE(flags);
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sampleRate);
	}

	// Convert sample header to OpenMPT's internal format.
	void ConvertToMPT(AMInstrumentHeader &instrHeader, ModSample &mptSmp) const
	{
		mptSmp.nPan = Util::Min(pan, static_cast<uint16>(32767)) * 256 / 32767;
		mptSmp.nVolume = Util::Min(volume, static_cast<uint16>(32767)) * 256 / 32767;
		mptSmp.nGlobalVol = 64;
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;
		mptSmp.nC5Speed = sampleRate;

		if(instrHeader.vibratoType < CountOf(j2bAutoVibratoTrans))
			mptSmp.nVibType = j2bAutoVibratoTrans[instrHeader.vibratoType];
		mptSmp.nVibSweep = static_cast<uint8>(instrHeader.vibratoSweep);
		mptSmp.nVibRate = static_cast<uint8>(instrHeader.vibratoRate / 16);
		mptSmp.nVibDepth = static_cast<uint8>(instrHeader.vibratoDepth / 4);
		if((mptSmp.nVibRate | mptSmp.nVibDepth) != 0)
		{
			// Convert XM-style vibrato sweep to IT
			mptSmp.nVibSweep = 255 - mptSmp.nVibSweep;
		}

		if(flags & AMFFSampleHeader::smp16Bit)
			mptSmp.uFlags |= CHN_16BIT;
		if(flags & AMFFSampleHeader::smpLoop)
			mptSmp.uFlags |= CHN_LOOP;
		if(flags & AMFFSampleHeader::smpPingPong)
			mptSmp.uFlags |= CHN_PINGPONGLOOP;
		if(flags & AMFFSampleHeader::smpPanning)
			mptSmp.uFlags |= CHN_PANNING;
	}

	// Retrieve the internal sample format flags for this sample.
	SampleIO GetSampleFormat() const
	{
		return SampleIO(
			(flags & AMFFSampleHeader::smp16Bit) ? SampleIO::_16bit : SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);
	}
};

#pragma pack(pop)


// Convert RIFF AM(FF) pattern data to MPT pattern data.
bool ConvertAMPattern(FileReader &chunk, PATTERNINDEX pat, bool isAM, CSoundFile &sndFile)
//----------------------------------------------------------------------------------------
{
	// Effect translation LUT
	static const uint8 amEffTrans[] =
	{
		CMD_ARPEGGIO, CMD_PORTAMENTOUP, CMD_PORTAMENTODOWN, CMD_TONEPORTAMENTO,
		CMD_VIBRATO, CMD_TONEPORTAVOL, CMD_VIBRATOVOL, CMD_TREMOLO,
		CMD_PANNING8, CMD_OFFSET, CMD_VOLUMESLIDE, CMD_POSITIONJUMP,
		CMD_VOLUME, CMD_PATTERNBREAK, CMD_MODCMDEX, CMD_TEMPO,
		CMD_GLOBALVOLUME, CMD_GLOBALVOLSLIDE, CMD_KEYOFF, CMD_SETENVPOSITION,
		CMD_CHANNELVOLUME, CMD_CHANNELVOLSLIDE, CMD_PANNINGSLIDE, CMD_RETRIG,
		CMD_TREMOR, CMD_XFINEPORTAUPDOWN,
	};

	enum
	{
		rowDone		= 0,		// Advance to next row
		channelMask	= 0x1F,		// Mask for retrieving channel information
		volFlag		= 0x20,		// Volume effect present
		noteFlag	= 0x40,		// Note + instr present
		effectFlag	= 0x80,		// Effect information present
		dataFlag	= 0xE0,		// Channel data present
	};

	if(!chunk.BytesLeft())
	{
		return false;
	}

	ROWINDEX numRows = Clamp(static_cast<ROWINDEX>(chunk.ReadUint8()) + 1, ROWINDEX(1), MAX_PATTERN_ROWS);

	if(sndFile.Patterns.Insert(pat, numRows))
		return false;

	const CHANNELINDEX channels = sndFile.GetNumChannels();
	if(channels == 0)
		return false;

	PatternRow rowBase = sndFile.Patterns[pat].GetRow(0);
	ROWINDEX row = 0;

	while(row < numRows && chunk.BytesLeft())
	{
		const uint8 flags = chunk.ReadUint8();

		if(flags == rowDone)
		{
			row++;
			rowBase = sndFile.Patterns[pat].GetRow(row);
			continue;
		}

		ModCommand &m = rowBase[min((flags & channelMask), channels - 1)];

		if(flags & dataFlag)
		{
			if(flags & effectFlag) // effect
			{
				m.param = chunk.ReadUint8();
				m.command = chunk.ReadUint8();

				if(m.command < CountOf(amEffTrans))
				{
					// command translation
					m.command = amEffTrans[m.command];
				} else
				{
#ifdef DEBUG
					CHAR s[64];
					wsprintf(s, "J2B: Unknown command: 0x%X, param 0x%X", m.command, m.param);
					Log(s);
#endif // DEBUG
					m.command = CMD_NONE;
				}

				// Handling special commands
				switch(m.command)
				{
				case CMD_ARPEGGIO:
					if(m.param == 0) m.command = CMD_NONE;
					break;
				case CMD_VOLUME:
					if(m.volcmd == VOLCMD_NONE)
					{
						m.volcmd = VOLCMD_VOLUME;
						m.vol = Clamp(m.param, BYTE(0), BYTE(64));
						m.command = CMD_NONE;
						m.param = 0;
					}
					break;
				case CMD_TONEPORTAVOL:
				case CMD_VIBRATOVOL:
				case CMD_VOLUMESLIDE:
				case CMD_GLOBALVOLSLIDE:
				case CMD_PANNINGSLIDE:
					if (m.param & 0xF0) m.param &= 0xF0;
					break;
				case CMD_PANNING8:
					if(m.param <= 0x80) m.param = min(m.param << 1, 0xFF);
					else if(m.param == 0xA4) {m.command = CMD_S3MCMDEX; m.param = 0x91;}
					break;
				case CMD_PATTERNBREAK:
					m.param = ((m.param >> 4) * 10) + (m.param & 0x0F);
					break;
				case CMD_MODCMDEX:
					m.ExtendedMODtoS3MEffect();
					break;
				case CMD_TEMPO:
					if(m.param <= 0x1F) m.command = CMD_SPEED;
					break;
				case CMD_XFINEPORTAUPDOWN:
					switch(m.param & 0xF0)
					{
					case 0x10:
						m.command = CMD_PORTAMENTOUP;
						break;
					case 0x20:
						m.command = CMD_PORTAMENTODOWN;
						break;
					}
					m.param = (m.param & 0x0F) | 0xE0;
					break;
				}
			}

			if (flags & noteFlag) // note + ins
			{
				m.instr = chunk.ReadUint8();
				m.note = chunk.ReadUint8();
				if(m.note == 0x80) m.note = NOTE_KEYOFF;
				else if(m.note > 0x80) m.note = NOTE_FADE;	// I guess the support for IT "note fade" notes was not intended in mod2j2b, but hey, it works! :-D
			}

			if (flags & volFlag) // volume
			{
				m.volcmd = VOLCMD_VOLUME;
				m.vol = chunk.ReadUint8();
				if(isAM)
				{
					m.vol = m.vol * 64 / 127;
				}
			}
		}
	}

	return true;
}


bool CSoundFile::ReadAM(FileReader &file)
//---------------------------------------
{
	file.Rewind();
	AMFFRiffChunk fileHeader;
	if(!file.ReadConvertEndianness(fileHeader))
	{
		return false;
	}

	if(fileHeader.id != AMFFRiffChunk::idRIFF)
	{
		return false;
	}

	bool isAM; // false: AMFF, true: AM

	uint32 format = file.ReadUint32LE();
	if(format == AMFFRiffChunk::idAMFF)
		isAM = false; // "AMFF"
	else if(format == AMFFRiffChunk::idAM__)
		isAM = true; // "AM  "
	else
		return false;

	m_nChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;

	// go through all chunks now
	while(file.BytesLeft())
	{
		AMFFRiffChunk chunkHeader;
		if(!file.ReadConvertEndianness(chunkHeader))
		{
			break;
		}

		FileReader chunk = file.GetChunk(chunkHeader.length);
		if(!chunk.IsValid())
		{
			continue;
		}

		switch(chunkHeader.id)
		{
		case AMFFRiffChunk::idMAIN: // "MAIN" - Song info (AMFF)
		case AMFFRiffChunk::idINIT: // "INIT" - Song info (AM)
			if((chunkHeader.id == AMFFRiffChunk::idMAIN && !isAM) || (chunkHeader.id == AMFFRiffChunk::idINIT && isAM))
			{
				AMFFMainChunk mainChunk;
				if(!chunk.Read(mainChunk))
				{
					break;
				}

				StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[0], mainChunk.songname);

				m_dwSongFlags = SONG_ITOLDEFFECTS | SONG_ITCOMPATGXX;
				if(!(mainChunk.flags & AMFFMainChunk::amigaSlides))
				{
					m_dwSongFlags |= SONG_LINEARSLIDES;
				}
				if(mainChunk.channels < 1 || !chunk.CanRead(mainChunk.channels))
				{
					return false;
				}
				m_nChannels = Util::Min(static_cast<CHANNELINDEX>(mainChunk.channels), MAX_BASECHANNELS);
				m_nDefaultSpeed = mainChunk.speed;
				m_nDefaultTempo = mainChunk.tempo;
				m_nDefaultGlobalVolume = mainChunk.globalvolume * 2;
				m_nSamplePreAmp = m_nVSTiVolume = 48;
				m_nType = MOD_TYPE_J2B;

				ASSERT(mainChunk.unknown == LittleEndian(0xFF0001C5) || mainChunk.unknown == LittleEndian(0x35800716) || mainChunk.unknown == LittleEndian(0xFF00FFFF));

				// It seems like there's no way to differentiate between
				// Muted and Surround channels (they're all 0xA0) - might
				// be a limitation in mod2j2b.
				for(CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++)
				{
					ChnSettings[nChn].nVolume = 64;
					ChnSettings[nChn].nPan = 128;

					uint8 pan = chunk.ReadUint8();

					if(isAM)
					{
						if(pan > 128)
							ChnSettings[nChn].dwFlags = CHN_MUTE;
						else
							ChnSettings[nChn].nPan = pan * 2;
					} else
					{
						if(pan >= 128)
							ChnSettings[nChn].dwFlags = CHN_MUTE;
						else
							ChnSettings[nChn].nPan = pan * 4;
					}
				}
			}
			break;

		case AMFFRiffChunk::idORDR: // "ORDR" - Order list
			{
				uint8 numOrders = chunk.ReadUint8() + 1;
				Order.ReadAsByte(chunk, numOrders);
			}
			break;

		case AMFFRiffChunk::idPATT: // "PATT" - Pattern data for one pattern
			{
				PATTERNINDEX pat = chunk.ReadUint8();
				size_t patternSize = chunk.ReadUint32LE();
				FileReader patternChunk = chunk.GetChunk(patternSize);
				ConvertAMPattern(patternChunk, pat, isAM, *this);
			}
			break;

		case AMFFRiffChunk::idINST: // "INST" - Instrument (only in RIFF AMFF)
			if(!isAM)
			{
				AMFFInstrumentHeader instrHeader;
				if(!chunk.ReadConvertEndianness(instrHeader))
				{
					break;
				}

				const INSTRUMENTINDEX nIns = instrHeader.index + 1;
				if(nIns >= MAX_INSTRUMENTS)
					break;

				if(Instruments[nIns] != nullptr)
					delete Instruments[nIns];

				try
				{
					Instruments[nIns] = new ModInstrument();
				} catch(MPTMemoryException)
				{
					break;
				}
				ModInstrument *pIns = Instruments[nIns];

				m_nInstruments = max(m_nInstruments, nIns);

				instrHeader.ConvertToMPT(*pIns, m_nSamples);

				// read sample sub-chunks - this is a rather "flat" format compared to RIFF AM and has no nested RIFF chunks.
				for(size_t samples = 0; samples < instrHeader.numSamples; samples++)
				{
					AMFFSampleHeader sampleHeader;

					if(m_nSamples + 1 >= MAX_SAMPLES || !chunk.ReadConvertEndianness(sampleHeader))
					{
						break;
					}

					const SAMPLEINDEX smp = ++m_nSamples;

					MemsetZero(Samples[smp]);

					if(sampleHeader.id != AMFFRiffChunk::idSAMP)
					{
						break;
					}

					StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[smp], sampleHeader.name);
					sampleHeader.ConvertToMPT(instrHeader, Samples[smp]);
					sampleHeader.GetSampleFormat().ReadSample(Samples[smp], chunk);
				}
			}
			break;

		case AMFFRiffChunk::idRIFF: // "RIFF" - Instrument (only in RIFF AM)
			if(isAM)
			{
				if(chunk.ReadUint32LE() != AMFFRiffChunk::idAI__)
				{
					break;
				}

				AMFFRiffChunk instChunk;
				if(!chunk.ReadConvertEndianness(instChunk) || instChunk.id != AMFFRiffChunk::idINST)
				{
					break;
				}

				AMInstrumentHeader instrHeader;
				if(!chunk.ReadConvertEndianness(instrHeader))
				{
					break;
				}
				ASSERT(instrHeader.headSize + 4 == sizeof(instrHeader));

				const INSTRUMENTINDEX instr = instrHeader.index + 1;
				if(instr >= MAX_INSTRUMENTS)
					break;

				if(Instruments[instr] != nullptr)
					delete Instruments[instr];

				try
				{
					Instruments[instr] = new ModInstrument();
				} catch(MPTMemoryException)
				{
					break;
				}
				ModInstrument *pIns = Instruments[instr];
				m_nInstruments = max(m_nInstruments, instr);

				instrHeader.ConvertToMPT(*pIns, m_nSamples);

				// Read sample sub-chunks (RIFF nesting ftw)
				for(size_t nSmpCnt = 0; nSmpCnt < instrHeader.numSamples; nSmpCnt++)
				{
					AMFFRiffChunk sampleChunk;
					if(!chunk.ReadConvertEndianness(sampleChunk) || sampleChunk.id != AMFFRiffChunk::idRIFF)
					{
						break;
					}

					if(chunk.ReadUint32LE() != AMFFRiffChunk::idAS__ || m_nSamples + 1 >= MAX_SAMPLES)
					{
						break;
					}

					const SAMPLEINDEX smp = ++m_nSamples;

					// Aaand even more nested chunks! Great, innit?
					if(!chunk.ReadConvertEndianness(sampleChunk) || sampleChunk.id != AMFFRiffChunk::idSAMP)
					{
						break;
					}

					FileReader sampleFileChunk = chunk.GetChunk(sampleChunk.length);

					AMSampleHeader sampleHeader;
					if(!sampleFileChunk.ReadConvertEndianness(sampleHeader))
					{
						break;
					}

					MemsetZero(Samples[smp]);

					StringFixer::ReadString<StringFixer::maybeNullTerminated>(m_szNames[smp], sampleHeader.name);

					sampleHeader.ConvertToMPT(instrHeader, Samples[smp]);

					sampleFileChunk.Seek(sampleHeader.headSize + 4);
					sampleHeader.GetSampleFormat().ReadSample(Samples[smp], sampleFileChunk);

					// RIFF AM has a padding byte so that all chunks have an even size.
					if((sampleChunk.length % 2) != 0)
					{
						chunk.Skip(1);
					}
				}
			}
			break;
		}

		// RIFF AM has a padding byte so that all chunks have an even size.
		if(isAM && (chunkHeader.length % 2) != 0)
		{
			file.Skip(1);
		}
	}

	return true;
}

bool CSoundFile::ReadJ2B(FileReader &file)
//----------------------------------------
{
	file.Rewind();
	J2BFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader))
	{
		return false;
	}

	if(fileHeader.signature != J2BFileHeader::magicMUSE // "MUSE"
		|| (fileHeader.deadbeaf != J2BFileHeader::magicDEADBEAF // 0xDEADBEAF (RIFF AM)
			&& fileHeader.deadbeaf != J2BFileHeader::magicDEADBABE) // 0xDEADBABE (RIFF AMFF)
		|| fileHeader.fileLength != file.GetLength()
		|| fileHeader.packedLength != file.BytesLeft()
		|| fileHeader.packedLength == 0
		|| fileHeader.crc32 != crc32(0, reinterpret_cast<const Bytef *>(file.GetRawData()), file.BytesLeft())
		)
	{
		return false;
	}

	// Header is valid, now unpack the RIFF AM file using inflate
	Bytef *amFile;
	uLongf destSize = fileHeader.unpackedLength;
	try
	{
		amFile = new Bytef[destSize];
	} catch(MPTMemoryException)
	{
		return false;
	}

	int retVal = uncompress(amFile, &destSize, reinterpret_cast<const Bytef *>(file.GetRawData()), fileHeader.packedLength);

	bool result = false;

	if(destSize == fileHeader.unpackedLength && retVal == Z_OK)
	{
		// Success, now load the RIFF AM(FF) module.
		FileReader amFile(reinterpret_cast<const char *>(amFile), destSize);
		result = ReadAM(amFile);
	}
	delete[] amFile;

	return result;
}