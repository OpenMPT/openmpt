/*
 * Load_stm.cpp
 * ------------
 * Purpose: STM (ScreamTracker 2) module loader
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#if MPT_COMPILER_MSVC
#pragma warning(disable:4244) //"conversion from 'type1' to 'type2', possible loss of data"
#endif

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif


// STM sample header struct
struct PACKED STMSampleHeader
{
	char   filename[12];	// Can't have long comments - just filename comments :)
	uint8  zero;
	uint8  disk;			// A blast from the past
	uint8  offset[2];		// ISA in memory when in ST 2
	uint16 length;			// Sample length
	uint16 loopStart;		// Loop start point
	uint16 loopEnd;			// Loop end point
	uint8  volume;			// Volume
	uint8  reserved2;
	uint16 sampleRate;
	uint8  reserved3[6];	// Yet more of PSi's reserved crap

	// Convert an STM sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mpt::String::Read<mpt::String::nullTerminated>(mptSmp.filename, filename);

		mptSmp.nC5Speed = sampleRate;
		mptSmp.nVolume = std::min<uint8>(volume, 64) * 4;
		mptSmp.nLength = length;
		mptSmp.nLoopStart = loopStart;
		mptSmp.nLoopEnd = loopEnd;

		if(mptSmp.nLength < 2 || volume == 0)
		{
			// As WTF as the above condition might sound, it seems to make sense.
			// The zero-volume samples in acidlamb.stm shouldn't be loaded, but they have an actual sample length.
			mptSmp.nLength = 0;
		}

		if(mptSmp.nLoopStart < mptSmp.nLength
			&& mptSmp.nLoopEnd > mptSmp.nLoopStart
			&& mptSmp.nLoopEnd != 0xFFFF)
		{
			mptSmp.uFlags = CHN_LOOP;
			mptSmp.nLoopEnd = std::min(mptSmp.nLoopEnd, mptSmp.nLength);
		}
	}

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(sampleRate);
	}
};

STATIC_ASSERT(sizeof(STMSampleHeader) == 32);


// STM file header
struct PACKED STMFileHeader
{
	char  songname[20];
	char  trackername[8];			// !SCREAM! for ST 2.xx
	uint8 dosEof;					// 0x1A
	uint8 filetype;					// 1=song, 2=module (only 2 is supported, of course) :)
	uint8 verMajor;					// Like 2
	uint8 verMinor;					// "ditto"
	uint8 initTempo;				// initspeed= stm inittempo>>4
	uint8 numPatterns;				// number of patterns
	uint8 globalVolume;				// <- WoW! a RiGHT TRiANGLE =8*)
	uint8 reserved[13];				// More of PSi's internal crap
	STMSampleHeader samples[31];	// Sample headers
	uint8 order[128];				// Order list
};

STATIC_ASSERT(sizeof(STMFileHeader) == 1168);


// Pattern note entry
struct PACKED STMPatternEntry
{
	uint8 note;
	uint8 insvol;
	uint8 volcmd;
	uint8 cmdinf;
};

STATIC_ASSERT(sizeof(STMPatternEntry) == 4);


struct PACKED STMPatternData
{
	STMPatternEntry entry[64 * 4];
	void ConvertEndianness()
	{
		// nothing
	}
};

STATIC_ASSERT(sizeof(STMPatternData) == 4*64*4);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadSTM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	STMFileHeader fileHeader;
	if(!file.Read(fileHeader)
		|| fileHeader.filetype != 2
		|| fileHeader.dosEof != 0x1A
		|| (mpt::strnicmp(fileHeader.trackername, "!SCREAM!", 8)
			&& mpt::strnicmp(fileHeader.trackername, "BMOD2STM", 8)))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, fileHeader.songname);

	// Read STM header
	InitializeGlobals();
	madeWithTracker = mpt::String::Print("Scream Tracker %1.%2", fileHeader.verMajor, mpt::fmt::hex0<2>(fileHeader.verMinor));
	m_nType = MOD_TYPE_STM;
	m_nSamples = 31;
	m_nChannels = 4;
	m_nMinPeriod = 64;
	m_nMaxPeriod = 0x7FFF;
#ifdef MODPLUG_TRACKER
	m_nDefaultTempo = 125;
	m_nDefaultSpeed = fileHeader.initTempo >> 4;
#else
	m_nDefaultTempo = 125 * 16;
	m_nDefaultSpeed = fileHeader.initTempo;
#endif // MODPLUG_TRACKER
	if(m_nDefaultSpeed < 1) m_nDefaultSpeed = 1;
	m_nDefaultGlobalVolume = std::min<uint8>(fileHeader.globalVolume, 64) * 4;

	// Setting up channels
	for(CHANNELINDEX chn = 0; chn < 4; chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].nPan = (chn & 1) ? 0x40 : 0xC0;
	}

	// Read samples
	for(SAMPLEINDEX smp = 0; smp < 31; smp++)
	{
		fileHeader.samples[smp].ConvertEndianness();
		fileHeader.samples[smp].ConvertToMPT(Samples[smp + 1]);
		mpt::String::Read<mpt::String::nullTerminated>(m_szNames[smp + 1], fileHeader.samples[smp].filename);
	}

	// Read order list
	Order.ReadFromArray(fileHeader.order);
	for(ORDERINDEX ord = 0; ord < 128; ord++)
	{
		if(Order[ord] >= 99)
		{
			Order[ord] = Order.GetInvalidPatIndex();
		}
	}

	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		STMPatternData patternData;

		if(!(loadFlags & loadPatternData) || Patterns.Insert(pat, 64) || !file.ReadConvertEndianness(patternData))
		{
			file.Skip(sizeof(patternData));
			continue;
		}

		ModCommand *m = Patterns[pat];
		ORDERINDEX breakPos = ORDERINDEX_INVALID;
		ROWINDEX breakRow = 63;	// Candidate row for inserting pattern break
	
		for(size_t n = 0; n < 64 * 4; n++, m++)
		{
			const STMPatternEntry &entry = patternData.entry[n];

			if(entry.note == 0xFE || entry.note == 0xFC)
			{
				m->note = NOTE_NOTECUT;
			} else if(entry.note < 0xFC)
			{
				m->note = (entry.note >> 4) * 12 + (entry.note & 0x0F) + 36 + NOTE_MIN;
			}

			m->instr = entry.insvol >> 3;
			if(m->instr > 31)
			{
				m->instr = 0;
			}
			
			int8 vol = (entry.insvol & 0x07) | (entry.volcmd >> 1);
			if(vol <= 64)
			{
				m->volcmd = VOLCMD_VOLUME;
				m->vol = vol;
			}

			static const uint8 stmEffects[] =
			{
				CMD_NONE, CMD_SPEED, CMD_POSITIONJUMP, CMD_PATTERNBREAK,					// .ABC
				CMD_VOLUMESLIDE, CMD_PORTAMENTODOWN, CMD_PORTAMENTOUP, CMD_TONEPORTAMENTO,	// DEFG
				CMD_VIBRATO, CMD_TREMOR, CMD_ARPEGGIO, CMD_NONE,							// HIJK
				CMD_NONE, CMD_NONE, CMD_NONE, CMD_NONE,										// LMNO
				// KLMNO can be entered in the editor but don't do anything
			};

			m->command = stmEffects[entry.volcmd & 0x0F];
			m->param = entry.cmdinf;

			switch(m->command)
			{
			case CMD_PATTERNBREAK:
				m->param = (m->param & 0xF0) * 10 + (m->param & 0x0F);
				if(breakRow > m->param)
				{
					breakRow = m->param;
				}
				break;

			case CMD_POSITIONJUMP:
				// This effect is also very weird.
				// Bxx doesn't appear to cause an immediate break -- it merely
				// sets the next order for when the pattern ends (either by
				// playing it all the way through, or via Cxx effect)
				breakPos = m->param;
				breakRow = 63;
				m->command = CMD_NONE;
				break;

			case CMD_TREMOR:
				// this actually does something with zero values, and has no
				// effect memory. which makes SENSE for old-effects tremor,
				// but ST3 went and screwed it all up by adding an effect
				// memory and IT followed that, and those are much more popular
				// than STM so we kind of have to live with this effect being
				// broken... oh well. not a big loss.
				break;

#ifdef MODPLUG_TRACKER
			case CMD_SPEED:
				// ST2 assumes that the tempo is 125 * 16 BPM (or in other words: ticks are
				// 16 times as precise as in ProTracker), and effects are updated on every
				// 16th tick of a row. This is pretty hard to handle in the tracker when not
				// natively supporting STM editing, so we just assume the tempo is 125 and
				// divide the speed by 16 instead. Parameters below 10 might behave weird.
				m->param >>= 4;
				MPT_FALLTHROUGH;
#endif // MODPLUG_TRACKER

			default:
				// Anything not listed above is a no-op if there's no value.
				// (ST2 doesn't have effect memory)
				if(!m->param)
				{
					m->command = CMD_NONE;
				}
				break;
			}
		}

		if(breakPos != ORDERINDEX_INVALID)
		{
			Patterns[pat].WriteEffect(EffectWriter(CMD_POSITIONJUMP, breakPos).Row(breakRow).Retry(EffectWriter::rmTryPreviousRow));
		}
	}

	// Reading Samples
	if(loadFlags & loadSampleData)
	{
		const SampleIO sampleIO(
			SampleIO::_8bit,
			SampleIO::mono,
			SampleIO::littleEndian,
			SampleIO::signedPCM);

		for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
		{
			ModSample &sample = Samples[smp];
			if(sample.nLength)
			{
				//size_t sampleOffset = fileHeader.samples[smp - 1].offset << 4;
				//if(sampleOffset > sizeof(STMPatternEntry) && sampleOffset < file.GetLength())
				//{
				//	file.Seek(sampleOffset);
				//} else
				{
					file.Seek((file.GetPosition() + 15) & (~15));
				}
				sampleIO.ReadSample(sample, file);
			}
		}
	}

	return true;
}
