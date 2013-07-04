/*
 * Load_ult.cpp
 * ------------
 * Purpose: ULT (UltraTracker) module loader
 * Notes  : (currently none)
 * Authors: Storlek (Original author - http://schismtracker.org/ - code ported with permission)
 *			Johannes Schultz (OpenMPT Port, tweaks)
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED UltFileHeader
{
	char  signature[14];		// "MAS_UTrack_V00"
	uint8 version;				// '1'...'4'
	char  songName[32];			// Song Name, not guaranteed to be null-terminated
	uint8 messageLength;		// Number of Lines
};

STATIC_ASSERT(sizeof(UltFileHeader) == 48);


struct PACKED UltSample
{
	enum UltSampleFlags
	{
		ULT_16BIT = 4,
		ULT_LOOP  = 8,
		ULT_PINGPONGLOOP = 16,
	};

	char   name[32];
	char   filename[12];
	uint32 loopStart;
	uint32 loopEnd;
	uint32 sizeStart;
	uint32 sizeEnd;
	uint8  volume;		// 0-255, apparently prior to 1.4 this was logarithmic?
	uint8  flags;		// above
	uint16 speed;		// only exists for 1.4+
	int16  finetune;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sizeStart);
		SwapBytesLE(sizeEnd);
		SwapBytesLE(speed);
		SwapBytesLE(finetune);
	}

	// Convert an ULT sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();

		mpt::String::Read<mpt::String::maybeNullTerminated>(mptSmp.filename, filename);

		if(sizeEnd <= sizeStart)
		{
			return;
		}

		mptSmp.nLength = sizeEnd - sizeStart;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = std::min(static_cast<SmpLength>(loopEnd), mptSmp.nLength);
		mptSmp.nVolume = volume;
		mptSmp.nGlobalVol = 64;

		// mikmod does some weird integer math here, but it didn't really work for me
		mptSmp.nC5Speed = speed;
		if(finetune)
		{
			mptSmp.nC5Speed = static_cast<uint32>((static_cast<double>(mptSmp.nC5Speed)) * pow(2.0, ((static_cast<double>(finetune)) / (12.0 * 32768))));
		}

		if(flags & ULT_LOOP)
			mptSmp.uFlags.set(CHN_LOOP);
		if(flags & ULT_PINGPONGLOOP)
			mptSmp.uFlags.set(CHN_PINGPONGLOOP);
		if(flags & ULT_16BIT)
		{
			mptSmp.uFlags.set(CHN_16BIT);
			mptSmp.nLoopStart /= 2;
			mptSmp.nLoopEnd /= 2;
		}
		
	}
};

STATIC_ASSERT(sizeof(UltSample) == 66);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

/* Unhandled effects:
5x1 - do not loop sample (x is unused)
5xC - end loop and finish sample
9xx - set sample offset to xx * 1024
    with 9yy: set sample offset to xxyy * 4
E0x - set vibrato strength (2 is normal)

The logarithmic volume scale used in older format versions here, or pretty
much anywhere for that matter. I don't even think Ultra Tracker tries to
convert them. */


static void TranslateULTCommands(uint8 *pe, uint8 *pp)
//----------------------------------------------------
{

	static const uint8 ultEffTrans[] =
	{
		CMD_ARPEGGIO,
		CMD_PORTAMENTOUP,
		CMD_PORTAMENTODOWN,
		CMD_TONEPORTAMENTO,
		CMD_VIBRATO,
		CMD_NONE,
		CMD_NONE,
		CMD_TREMOLO,
		CMD_NONE,
		CMD_OFFSET,
		CMD_VOLUMESLIDE,
		CMD_PANNING8,
		CMD_VOLUME,
		CMD_PATTERNBREAK,
		CMD_NONE, // extended effects, processed separately
		CMD_SPEED,
	};


	uint8 e = *pe & 0x0F;
	uint8 p = *pp;

	*pe = ultEffTrans[e];

	switch (e)
	{
	case 0x00:
		if(!p)
			*pe = CMD_NONE;
		break;
	case 0x05:
		// play backwards
		if((p & 0x0F) == 0x02)
		{
			*pe = CMD_S3MCMDEX;
			p = 0x9F;
		}
		break;
	case 0x0A:
		// blah, this sucks
		if(p & 0xF0)
			p &= 0xF0;
		break;
	case 0x0B:
		// mikmod does this wrong, resulting in values 0-225 instead of 0-255
		p = (p & 0x0F) * 0x11;
		break;
	case 0x0C: // volume
		p >>= 2;
		break;
	case 0x0D: // pattern break
		p = 10 * (p >> 4) + (p & 0x0F);
	case 0x0E: // special
		switch(p >> 4)
		{
		case 0x01:
			*pe = CMD_PORTAMENTOUP;
			p = 0xF0 | (p & 0x0F);
			break;
		case 0x02:
			*pe = CMD_PORTAMENTODOWN;
			p = 0xF0 | (p & 0x0F);
			break;
		case 0x08:
			*pe = CMD_S3MCMDEX;
			p = 0x60 | (p & 0x0F);
			break;
		case 0x09:
			*pe = CMD_RETRIG;
			p &= 0x0F;
			break;
		case 0x0A:
			*pe = CMD_VOLUMESLIDE;
			p = ((p & 0x0F) << 4) | 0x0F;
			break;
		case 0x0B:
			*pe = CMD_VOLUMESLIDE;
			p = 0xF0 | (p & 0x0F);
			break;
		case 0x0C: case 0x0D:
			*pe = CMD_S3MCMDEX;
			break;
		}
		break;
	case 0x0F:
		if(p > 0x2F)
			*pe = CMD_TEMPO;
		break;
	}

	*pp = p;
}


static int ReadULTEvent(ModCommand &m, FileReader &file)
//------------------------------------------------------
{
	uint8 b, repeat = 1;
	uint8 cmd1, cmd2;	// 1 = vol col, 2 = fx col in the original schismtracker code
	uint8 param1, param2;

	b = file.ReadUint8();
	if (b == 0xFC)	// repeat event
	{
		repeat = file.ReadUint8();
		b = file.ReadUint8();
	}

	m.note = (b > 0 && b < 61) ? b + 36 : NOTE_NONE;
	m.instr = file.ReadUint8();
	b = file.ReadUint8();
	cmd1 = b & 0x0F;
	cmd2 = b >> 4;
	param1 = file.ReadUint8();
	param2 = file.ReadUint8();
	TranslateULTCommands(&cmd1, &param1);
	TranslateULTCommands(&cmd2, &param2);

	// sample offset -- this is even more special than digitrakker's
	if(cmd1 == CMD_OFFSET && cmd2 == CMD_OFFSET)
	{
		uint32 off = ((param1 << 8) | param2) >> 6;
		cmd1 = CMD_NONE;
		param1 = (uint8)MIN(off, 0xFF);
	} else if(cmd1 == CMD_OFFSET)
	{
		uint32 off = param1 * 4;
		param1 = (uint8)MIN(off, 0xFF);
	} else if(cmd2 == CMD_OFFSET)
	{
		uint32 off = param2 * 4;
		param2 = (uint8)MIN(off, 0xFF);
	} else if(cmd1 == cmd2)
	{
		// don't try to figure out how ultratracker does this, it's quite random
		cmd2 = CMD_NONE;
	}
	if(cmd2 == CMD_VOLUME || (cmd2 == CMD_NONE && cmd1 != CMD_VOLUME))
	{
		// swap commands
		std::swap(cmd1, cmd2);
		std::swap(param1, param2);
	}

	// Combine slide commands, if possible
	ModCommand::CombineEffects(cmd2, param2, cmd1, param1);

	// Do that dance.
	// Maybe I should quit rewriting this everywhere and make a generic version :P
	int n;
	for(n = 0; n < 4; n++)
	{
		if(ModCommand::ConvertVolEffect(cmd1, param1, (n >> 1) != 0))
		{
			n = 5;
			break;
		}
		std::swap(cmd1, cmd2);
		std::swap(param1, param2);
	}
	if(n < 5)
	{
		if (ModCommand::GetEffectWeight((ModCommand::COMMAND)cmd1) > ModCommand::GetEffectWeight((ModCommand::COMMAND)cmd2))
		{
			std::swap(cmd1, cmd2);
			std::swap(param1, param2);
		}
		cmd1 = CMD_NONE;
	}
	if(cmd1 == CMD_NONE)
		param1 = 0;
	if(cmd2 == CMD_NONE)
		param2 = 0;

	m.volcmd = cmd1;
	m.vol = param1;
	m.command = cmd2;
	m.param = param2;

	return repeat;
}


// Functor for postfixing ULT patterns (this is easier than just remembering everything WHILE we're reading the pattern events)
struct PostFixUltCommands
//=======================
{
	PostFixUltCommands(CHANNELINDEX numChannels)
	{
		this->numChannels = numChannels;
		curChannel = 0;
		writeT125 = false;
		isPortaActive.resize(numChannels, false);
	}

	void operator()(ModCommand& m)
	{
		// Attempt to fix portamentos.
		// UltraTracker will slide until the destination note is reached or 300 is encountered.

		// Stop porta?
		if(m.command == CMD_TONEPORTAMENTO && m.param == 0)
		{
			isPortaActive[curChannel] = false;
			m.command = CMD_NONE;
		}
		if(m.volcmd == VOLCMD_TONEPORTAMENTO && m.vol == 0)
		{
			isPortaActive[curChannel] = false;
			m.volcmd = VOLCMD_NONE;
		}

		// Apply porta?
		if(m.note == NOTE_NONE && isPortaActive[curChannel])
		{
			if(m.command == CMD_NONE && m.vol != VOLCMD_TONEPORTAMENTO)
			{
				m.command = CMD_TONEPORTAMENTO;
				m.param = 0;
			} else if(m.volcmd == VOLCMD_NONE && m.command != CMD_TONEPORTAMENTO)
			{
				m.volcmd = VOLCMD_TONEPORTAMENTO;
				m.vol = 0;
			}
		} else	// new note -> stop porta (or initialize again)
		{
			isPortaActive[curChannel] = (m.command == CMD_TONEPORTAMENTO || m.volcmd == VOLCMD_TONEPORTAMENTO);
		}

		// attempt to fix F00 (reset to tempo 125, speed 6)
		if(writeT125 && m.command == CMD_NONE)
		{
			m.command = CMD_TEMPO;
			m.param = 125;
		}
		if(m.command == CMD_SPEED && m.param == 0)
		{
			m.param = 6;
			writeT125 = true;
		}
		if(m.command == CMD_TEMPO)	// don't try to fix this anymore if the tempo has already changed.
		{
			writeT125 = false;
		}
		curChannel = (curChannel + 1) % numChannels;
	}

	std::vector<bool> isPortaActive;
	bool writeT125;
	CHANNELINDEX numChannels, curChannel;
};


bool CSoundFile::ReadUlt(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();
	UltFileHeader fileHeader;

	// Tracker ID
	if(!file.Read(fileHeader)
		|| fileHeader.version < '1'
		|| fileHeader.version > '4'
		|| memcmp(fileHeader.signature, "MAS_UTrack_V00", sizeof(fileHeader.signature)) != 0)
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();
	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, fileHeader.songName);

	const char *versions[] = {"<1.4", "1.4", "1.5", "1.6"};
	madeWithTracker = "UltraTracker ";
	madeWithTracker += versions[fileHeader.version - '1'];

	m_nType = MOD_TYPE_ULT;
	m_SongFlags = SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS;	// this will be converted to IT format by MPT.
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	// read "messageLength" lines, each containing 32 characters.
	songMessage.ReadFixedLineLength(file, fileHeader.messageLength * 32, 32, 0);

	m_nSamples = static_cast<SAMPLEINDEX>(file.ReadUint8());
	if(GetNumSamples() >= MAX_SAMPLES)
		return false;

	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		UltSample sampleHeader;

		// Annoying: v4 added a field before the end of the struct
		if(fileHeader.version >= '4')
		{
			file.ReadConvertEndianness(sampleHeader);
		} else
		{
			file.ReadStructPartial(sampleHeader, 64);
			sampleHeader.ConvertEndianness();
			sampleHeader.finetune = sampleHeader.speed;
			sampleHeader.speed = 8363;
		}

		sampleHeader.ConvertToMPT(Samples[smp]);
		mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[smp], sampleHeader.name);
	}

	// ult just so happens to use 255 for its end mark, so there's no need to fiddle with this
	Order.ReadAsByte(file, 256);

	m_nChannels = file.ReadUint8() + 1;
	PATTERNINDEX numPats = file.ReadUint8() + 1;

	if(GetNumChannels() > MAX_BASECHANNELS || numPats > MAX_PATTERNS)
		return false;

	for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
	{
		ChnSettings[chn].Reset();
		if(fileHeader.version >= '3')
			ChnSettings[chn].nPan = ((file.ReadUint8() & 0x0F) << 4) + 8;
		else
			ChnSettings[chn].nPan = (chn & 1) ? 192 : 64;
	}

	for(PATTERNINDEX pat = 0; pat < numPats; pat++)
	{
		if(Patterns.Insert(pat, 64))
			return false;
	}

	for(CHANNELINDEX chn = 0; chn < m_nChannels; chn++)
	{
		ModCommand evnote;
		ModCommand *note;
		evnote.Clear();

		for(PATTERNINDEX pat = 0; pat < numPats; pat++)
		{
			note = Patterns[pat] + chn;
			ROWINDEX row = 0;
			while(row < 64)
			{
				int repeat = ReadULTEvent(evnote, file);
				if(repeat + row > 64)
					repeat = 64 - row;
				if(repeat == 0) break;
				while(repeat--)
				{
					*note = evnote;
					note += GetNumChannels();
					row++;
				}
			}
		}
	}

	// Post-fix some effects.
	Patterns.ForEachModCommand(PostFixUltCommands(GetNumChannels()));

	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			SampleIO(
				Samples[smp].uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::signedPCM)
				.ReadSample(Samples[smp], file);
		}
	}
	return true;
}