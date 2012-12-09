/*
 * WAVTools.cpp
 * ------------
 * Purpose: Definition of WAV file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "WAVTools.h"


///////////////////////////////////////////////////////////
// WAV Reading


WAVReader::WAVReader(FileReader &inputFile) : file(inputFile)
//-----------------------------------------------------------
{
	file.Rewind();

	RIFFHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| (fileHeader.magic != RIFFHeader::idRIFF && fileHeader.magic != RIFFHeader::idLIST)
		|| (fileHeader.type != RIFFHeader::idWAVE && fileHeader.type != RIFFHeader::idwave))
	{
		return;
	}

	isDLS = (fileHeader.magic == RIFFHeader::idLIST);

	ChunkReader::ChunkList<RIFFChunk> chunks = file.ReadChunks<RIFFChunk>(2);

	if(chunks.size() >= 4
		&& chunks[1].GetHeader().GetID() == RIFFChunk::iddata
		&& chunks[1].GetHeader().GetLength() % 2 != 0
		&& chunks[2].GetHeader().GetLength() == 0
		&& chunks[3].GetHeader().GetID() == RIFFChunk::id____)
	{
		// Houston, we have a problem: Old versions of (Open)MPT didn't write RIFF padding bytes. -_-
		// Luckily, the only RIFF chunk with an odd size those versions would ever write would be the "data" chunk
		// (which contains the sample data), and its size is only odd iff the sample has an odd length and is in
		// 8-Bit mono format. In all other cases, the sample size (and thus the chunk size) is even.

		// And we're even more lucky: The versions of (Open)MPT in question will always write a relatively small
		// (smaller than 256 bytes) "smpl" chunk after the "data" chunk. This means that after an unpadded sample,
		// we will always read "mpl?" (? being the length of the "smpl" chunk) as the next chunk magic. The first two
		// 32-Bit members of the "smpl" chunk are always zero in our case, so we are going to read a chunk length of 0
		// next and the next chunk magic, which will always consist of four zero bytes. Hooray! We just checked for those
		// four zero bytes and can be pretty confident that we should not have applied padding.
		file.Seek(sizeof(RIFFHeader));
		chunks = file.ReadChunks<RIFFChunk>(1);
	}

	// Read format chunk
	FileReader formatChunk = chunks.GetChunk(RIFFChunk::idfmt_);
	if(!formatChunk.ReadConvertEndianness(formatInfo))
	{
		return;
	}
	if(formatInfo.format == WAVFormatChunk::fmtExtensible)
	{
		WAVFormatChunkExtension extFormat;
		if(!formatChunk.ReadConvertEndianness(extFormat))
		{
			return;
		}
		formatInfo.format = extFormat.subFormat;
	}

	// Read sample data
	sampleData = chunks.GetChunk(RIFFChunk::iddata);

	if(!sampleData.IsValid())
	{
		// The old IMA ADPCM loader code looked for the "pcm " chunk instead of the "data" chunk...
		// Dunno why (Windows XP's audio recorder saves IMA ADPCM files with a "data" chunk), but we will just look for both.
		sampleData = chunks.GetChunk(RIFFChunk::idpcm_);
	}

	// "fact" chunk should contain sample length of compressed samples.
	sampleLength = chunks.GetChunk(RIFFChunk::idfact).ReadUint32LE();

	if((formatInfo.format != WAVFormatChunk::fmtIMA_ADPCM || sampleLength == 0) && GetSampleSize() != 0)
	{
		// Some samples have an incorrect blockAlign / sample size set (e.g. it's 8 in SQUARE.WAV while it should be 1), so let's better not trust this value.
		sampleLength = sampleData.GetLength() / GetSampleSize();
	}

	// Check for loop points, texts, etc...
	FindMetadataChunks(chunks);

	// DLS bank chunk
	wsmpChunk = chunks.GetChunk(RIFFChunk::idwsmp);
}


void WAVReader::FindMetadataChunks(ChunkReader::ChunkList<RIFFChunk> &chunks)
//---------------------------------------------------------------------------
{
	// Read sample loop points
	smplChunk = chunks.GetChunk(RIFFChunk::idsmpl);

	// Read text chunks
	ChunkReader listChunk = chunks.GetChunk(RIFFChunk::idLIST);
	if(listChunk.ReadMagic("INFO"))
	{
		infoChunk = listChunk.ReadChunks<RIFFChunk>(2);
	}

	// Read MPT sample information
	xtraChunk = chunks.GetChunk(RIFFChunk::idxtra);
}


void WAVReader::ApplySampleSettings(ModSample &sample, char (&sampleName)[MAX_SAMPLENAME])
//----------------------------------------------------------------------------------------
{
	// Read sample name
	FileReader textChunk = infoChunk.GetChunk(RIFFChunk::idINAM);
	if(textChunk.IsValid())
	{
		textChunk.ReadString<StringFixer::nullTerminated>(sampleName, textChunk.GetLength());
	}
	if(isDLS)
	{
		// DLS sample -> sample filename
		strncpy(sample.filename, sampleName, CountOf(sample.filename) - 1);
		StringFixer::SetNullTerminator(sample.filename);
	}

	// Read software name
	const bool isOldMPT = infoChunk.GetChunk(RIFFChunk::idISFT).ReadMagic("Modplug Tracker");
	
	// Convert loops
	WAVSampleInfoChunk sampleInfo;
	smplChunk.Rewind();
	if(smplChunk.ReadConvertEndianness(sampleInfo))
	{
		WAVSampleLoop loopData;
		if(sampleInfo.numLoops > 1 && smplChunk.ReadConvertEndianness(loopData))
		{
			// First loop: Sustain loop
			loopData.ApplyToSample(sample.nSustainStart, sample.nSustainEnd, sample.nLength, sample.uFlags, CHN_SUSTAINLOOP, CHN_PINGPONGSUSTAIN, isOldMPT);
		}
		// First loop (if only one loop is present) or second loop (if more than one loop is present): Normal sample loop
		if(smplChunk.ReadConvertEndianness(loopData))
		{
			loopData.ApplyToSample(sample.nLoopStart, sample.nLoopEnd, sample.nLength, sample.uFlags, CHN_LOOP, CHN_PINGPONGLOOP, isOldMPT);
		}
	}

	// Read MPT extra info
	WAVExtraChunk mptInfo;
	xtraChunk.Rewind();
	if(xtraChunk.ReadConvertEndianness(mptInfo))
	{
		if(mptInfo.flags & WAVExtraChunk::setPanning) sample.uFlags.set(CHN_PANNING);

		sample.nPan = Util::Min(mptInfo.defaultPan, uint16(256));
		sample.nVolume = Util::Min(mptInfo.defaultVolume, uint16(256));
		sample.nGlobalVol = Util::Min(mptInfo.globalVolume, uint16(64));
		sample.nVibType = mptInfo.vibratoType;
		sample.nVibSweep = mptInfo.vibratoSweep;
		sample.nVibDepth = mptInfo.vibratoDepth;
		sample.nVibRate = mptInfo.vibratoRate;

		if(xtraChunk.BytesLeft() >= MAX_SAMPLENAME)
		{
			// Name present (clipboard only)
			xtraChunk.ReadString<StringFixer::nullTerminated>(sampleName, MAX_SAMPLENAME);
			xtraChunk.ReadString<StringFixer::nullTerminated>(sample.filename, xtraChunk.BytesLeft());
		}
	}
}


void WAVSampleLoop::ApplyToSample(SmpLength &start, SmpLength &end, uint32 sampleLength, FlagSet<ChannelFlags, uint16> &flags, ChannelFlags enableFlag, ChannelFlags bidiFlag, bool mptLoopFix) const
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	start = Util::Min(static_cast<SmpLength>(loopStart), sampleLength);
	end = Clamp(static_cast<SmpLength>(loopEnd), start, sampleLength);
	if(!mptLoopFix && end < sampleLength)
	{
		// RIFF loop end points are inclusive - old versions of MPT didn't consider this.
		end++;
	}

	flags.set(enableFlag);
	if(loopType == loopBidi)
	{
		flags.set(bidiFlag);
	}
}
