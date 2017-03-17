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

OPENMPT_NAMESPACE_BEGIN

// STM sample header struct
struct STMSampleHeader
{
	char     filename[12];	// Can't have long comments - just filename comments :)
	uint8le  zero;
	uint8le  disk;			// A blast from the past
	uint16le offset;		// 20-bit offset in file (lower 4 bits are zero)
	uint16le length;		// Sample length
	uint16le loopStart;		// Loop start point
	uint16le loopEnd;		// Loop end point
	uint8le  volume;		// Volume
	uint8le  reserved2;
	uint16le sampleRate;
	uint8le  reserved3[6];

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

		if(mptSmp.nLength < 2) mptSmp.nLength = 0;

		if(mptSmp.nLoopStart < mptSmp.nLength
			&& mptSmp.nLoopEnd > mptSmp.nLoopStart
			&& mptSmp.nLoopEnd != 0xFFFF)
		{
			mptSmp.uFlags = CHN_LOOP;
			mptSmp.nLoopEnd = std::min(mptSmp.nLoopEnd, mptSmp.nLength);
		}
	}
};

MPT_BINARY_STRUCT(STMSampleHeader, 32)


// STM file header
struct STMFileHeader
{
	char  songname[20];
	char  trackername[8];	// !SCREAM! for ST 2.xx
	uint8 dosEof;			// 0x1A
	uint8 filetype;			// 1=song, 2=module (only 2 is supported, of course) :)
	uint8 verMajor;
	uint8 verMinor;
	uint8 initTempo;		// Ticks per row. Keep in mind that effects are only updated on every 16th tick.
	uint8 numPatterns;		// number of patterns
	uint8 globalVolume;
	uint8 reserved[13];
};

MPT_BINARY_STRUCT(STMFileHeader, 48)


// Pattern note entry
struct STMPatternEntry
{
	uint8le note;
	uint8le insvol;
	uint8le volcmd;
	uint8le cmdinf;
};

MPT_BINARY_STRUCT(STMPatternEntry, 4)


struct STMPatternData
{
	STMPatternEntry entry[64 * 4];
};

MPT_BINARY_STRUCT(STMPatternData, 4*64*4)


bool CSoundFile::ReadSTM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	STMFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader)
		|| fileHeader.filetype != 2
		|| fileHeader.dosEof != 0x1A
		|| (mpt::CompareNoCaseAscii(fileHeader.trackername, "!SCREAM!", 8)
			&& mpt::CompareNoCaseAscii(fileHeader.trackername, "BMOD2STM", 8))
		|| !file.CanRead(31 * sizeof(STMSampleHeader) + 128))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals(MOD_TYPE_STM);

	mpt::String::Read<mpt::String::maybeNullTerminated>(m_songName, fileHeader.songname);

	// Read STM header
	m_madeWithTracker = mpt::String::Print("Scream Tracker %1.%2", fileHeader.verMajor, mpt::fmt::hex0<2>(fileHeader.verMinor));
	m_nSamples = 31;
	m_nChannels = 4;
	m_nMinPeriod = 64;
	m_nMaxPeriod = 0x7FFF;
#ifdef MODPLUG_TRACKER
	m_nDefaultTempo.Set(125);
	m_nDefaultSpeed = fileHeader.initTempo >> 4;
#else
	m_nDefaultTempo.Set(125 * 16);
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
	uint16 sampleOffsets[31];
	for(SAMPLEINDEX smp = 1; smp <= 31; smp++)
	{
		STMSampleHeader sampleHeader;
		file.ReadStruct(sampleHeader);
		sampleHeader.ConvertToMPT(Samples[smp]);
		mpt::String::Read<mpt::String::nullTerminated>(m_szNames[smp], sampleHeader.filename);
		sampleOffsets[smp - 1] = sampleHeader.offset;
	}

	// Read order list
	Order.ReadAsByte(file, 128);
	for(ORDERINDEX ord = 0; ord < 128; ord++)
	{
		if(Order[ord] >= 99)
		{
			Order[ord] = Order.GetInvalidPatIndex();
		}
	}

	if(loadFlags & loadPatternData)
		Patterns.ResizeArray(fileHeader.numPatterns);
	for(PATTERNINDEX pat = 0; pat < fileHeader.numPatterns; pat++)
	{
		STMPatternData patternData;

		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64) || !file.ReadStruct(patternData))
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
			
			int8 vol = (entry.insvol & 0x07) | ((entry.volcmd & 0xF0) >> 1);
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
			case CMD_VOLUMESLIDE:
				// Lower nibble always has precedence, and there are no fine slides.
				if(m->param & 0x0F) m->param &= 0x0F;
				else m->param &= 0xF0;
				break;

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
			Patterns[pat].WriteEffect(EffectWriter(CMD_POSITIONJUMP, static_cast<ModCommand::PARAM>(breakPos)).Row(breakRow).Retry(EffectWriter::rmTryPreviousRow));
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
			// ST2 just plays random noise for samples with a default volume of 0
			if(sample.nLength && sample.nVolume > 0)
			{
				FileReader::off_t sampleOffset = sampleOffsets[smp - 1] << 4;
				// acidlamb.stm has some bogus samples with sample offsets past EOF
				if(sampleOffset > sizeof(STMFileHeader) && file.Seek(sampleOffset))
				{
					sampleIO.ReadSample(sample, file);
				}
			}
		}
	}

	return true;
}


OPENMPT_NAMESPACE_END
