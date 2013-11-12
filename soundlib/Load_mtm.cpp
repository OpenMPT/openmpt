/*
 * Load_mtm.cpp
 * ------------
 * Purpose: MTM (MultiTracker) module loader
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

// File Header
struct PACKED MTMFileHeader
{
	char   id[3];			// MTM file marker
	uint8  version;			// Tracker version
	char   songName[20];	// ASCIIZ songname
	uint16 numTracks;		// Number of tracks saved
	uint8  lastPattern;		// Last pattern number saved
	uint8  lastOrder;		// Last order number to play (songlength-1)
	uint16 commentSize;		// Length of comment field
	uint8  numSamples;		// Number of samples saved
	uint8  attribute;		// Attribute byte (unused)
	uint8  beatsPerTrack;	// Numbers of rows in every pattern (MultiTracker itself does not seem to support values != 64)
	uint8  numChannels;		// Number of channels used
	uint8  panPos[32];		// Channel pan positions

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numTracks);
		SwapBytesLE(commentSize);
	}
};

STATIC_ASSERT(sizeof(MTMFileHeader) == 66);


// Sample Header
struct PACKED MTMSampleHeader
{
	char   samplename[22];
	uint32 length;
	uint32 loopStart;
	uint32 loopEnd;
	int8   finetune;
	uint8  volume;
	uint8  attribute;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(length);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
	}

	// Convert an MTM sample header to OpenMPT's internal sample header.
	void ConvertToMPT(ModSample &mptSmp) const
	{
		mptSmp.Initialize();
		mptSmp.nVolume = std::min(uint16(volume * 4), uint16(256));
		if(length > 2)
		{
			mptSmp.nLength = length;
			mptSmp.nLoopStart = loopStart;
			mptSmp.nLoopEnd = loopEnd;
			LimitMax(mptSmp.nLoopEnd, mptSmp.nLength);
			if(mptSmp.nLoopStart + 4 >= mptSmp.nLoopEnd) mptSmp.nLoopStart = mptSmp.nLoopEnd = 0;
			if(mptSmp.nLoopEnd) mptSmp.uFlags.set(CHN_LOOP);
			mptSmp.nFineTune = MOD2XMFineTune(finetune);
			mptSmp.nC5Speed = ModSample::TransposeToFrequency(0, mptSmp.nFineTune);

			if(attribute & 0x01)
			{
				mptSmp.uFlags.set(CHN_16BIT);
				mptSmp.nLength /= 2;
				mptSmp.nLoopStart /= 2;
				mptSmp.nLoopEnd /= 2;
			}
		}
	}
};

STATIC_ASSERT(sizeof(MTMSampleHeader) == 37);


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadMTM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();
	MTMFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.id, "MTM", 3)
		|| fileHeader.lastOrder > 127
		|| fileHeader.numChannels > 32
		|| fileHeader.numChannels == 0
		|| fileHeader.numSamples >= MAX_SAMPLES
		|| fileHeader.lastPattern >= MAX_PATTERNS
		|| fileHeader.beatsPerTrack == 0
		|| !file.CanRead(sizeof(MTMSampleHeader) * fileHeader.numSamples + 128 + 192 * fileHeader.numTracks + 64 * (fileHeader.lastPattern + 1) + fileHeader.commentSize))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();
	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, fileHeader.songName);
	m_nType = MOD_TYPE_MTM;
	m_nSamples = fileHeader.numSamples;
	m_nChannels = fileHeader.numChannels;
	madeWithTracker = mpt::String::Print("MultiTracker %1.%2", fileHeader.version >> 4, fileHeader.version & 0x0F);

	// Reading instruments
	for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
	{
		MTMSampleHeader sampleHeader;
		file.ReadConvertEndianness(sampleHeader);
		sampleHeader.ConvertToMPT(Samples[smp]);
		mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[smp], sampleHeader.samplename);
	}

	// Setting Channel Pan Position
	for(CHANNELINDEX chn = 0; chn < GetNumChannels(); chn++)
	{
		ChnSettings[chn].Reset();
		ChnSettings[chn].nPan = ((fileHeader.panPos[chn] & 0x0F) << 4) + 8;
	}

	// Reading pattern order
	const ORDERINDEX readOrders = fileHeader.lastOrder + 1;
	Order.ReadAsByte(file, 128, readOrders);

	// Reading Patterns
	const ROWINDEX rowsPerPat = std::min(ROWINDEX(fileHeader.beatsPerTrack), MAX_PATTERN_ROWS);
	FileReader tracks = file.GetChunk(192 * fileHeader.numTracks);

	for(PATTERNINDEX pat = 0; pat <= fileHeader.lastPattern; pat++)
	{
		if(!(loadFlags & loadPatternData) || Patterns.Insert(pat, rowsPerPat))
		{
			break;
		}

		for(CHANNELINDEX chn = 0; chn < 32; chn++)
		{
			uint16 track = file.ReadUint16LE();
			if(track == 0 || track > fileHeader.numTracks || chn >= GetNumChannels())
			{
				continue;
			}

			tracks.Seek(192 * (track - 1));

			ModCommand *m = Patterns[pat].GetpModCommand(0, chn);
			for(ROWINDEX row = 0; row < rowsPerPat; row++, m += GetNumChannels())
			{
				uint8 data[3];
				tracks.ReadArray(data);

				if(data[0] & 0xFC) m->note = (data[0] >> 2) + 36 + NOTE_MIN;
				m->instr = ((data[0] & 0x03) << 4) | (data[1] >> 4);
				uint8 cmd = data[1] & 0x0F;
				uint8 param = data[2];
				if(cmd == 0x0A)
				{
					if(param & 0xF0) param &= 0xF0; else param &= 0x0F;
				}
				m->command = cmd;
				m->param = param;
				if(cmd != 0 || param != 0)
				{
					ConvertModCommand(*m);
					m->Convert(MOD_TYPE_MOD, MOD_TYPE_S3M);
				}
			}
		}
	}

	if(fileHeader.commentSize != 0)
	{
		// Read message with a fixed line length of 40 characters
		// (actually the last character is always null, so make that 39 + 1 padding byte)
		songMessage.ReadFixedLineLength(file, fileHeader.commentSize, 39, 1);
	}

	// Reading Samples
	if(loadFlags & loadSampleData)
	{
		for(SAMPLEINDEX smp = 1; smp <= GetNumSamples(); smp++)
		{
			SampleIO(
				Samples[smp].uFlags[CHN_16BIT] ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::littleEndian,
				SampleIO::unsignedPCM)
				.ReadSample(Samples[smp], file);
		}
	}

	m_nMinPeriod = 64;
	m_nMaxPeriod = 32767;
	return true;
}
