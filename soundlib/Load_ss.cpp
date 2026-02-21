/*
 * Load_ss.cpp
 * -----------
 * Purpose: Apple IIgs SoundSmith / MegaTracker module loader
 * Notes  : Partially based on deMODifier by Bret Victor and Javascript SoundSmith Player by mrkite
 *          MegaTracker effects (portamento / vibrato) are not supported (and not widely used).
 *          The deMODifier readme is refreshingly honest: "...this software inspired the creation of
 *          countless numbers of IIgs-specific tunes, several of which were actually worth listening to."
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "Loaders.h"

OPENMPT_NAMESPACE_BEGIN

struct SSInstrument
{
	uint8    nameLength;
	char     name[21];
	char     reserved[2];  // Usually contains random garbage
	uint16le volume;       // 0...255 (high byte is ignored)
	uint16le midiProgram;
	uint16le midiVelocity;
};

MPT_BINARY_STRUCT(SSInstrument, 30)


struct SSFooter
{
	std::array<uint16le, 14> trackPanning;
	uint16le useTrackPanning;  // 0: use instrument panning (which appears to not be stored anywhere in the file - loading a song just reverts every instrument to being panned to the right)
};

MPT_BINARY_STRUCT(SSFooter, 30)


struct SSFileHeader
{
	char         magic[6];    // "SONGOK" (or "IAN9OK" / "IAN92a" for MegaTracker files)
	uint16le     patBufSize;
	uint16le     speed;
	char         reserved[10];
	SSInstrument instruments[15];
	uint16le     numOrders;
	uint8        orders[128];

	bool IsValid() const
	{
		return (!std::memcmp(magic, "SONGOK", 6) || !std::memcmp(magic, "IAN9OK", 6) || !std::memcmp(magic, "IAN92a", 6))
			&& patBufSize >= 14 * 64
			&& ((patBufSize % 14 * 64) == 0 || patBufSize == 0x8000)
			&& speed <= 15
			&& (numOrders & 0xFF) <= 128;
	}

	uint32 GetHeaderMinimumAdditionalSize() const
	{
		return patBufSize * 3u;
	}
};

MPT_BINARY_STRUCT(SSFileHeader, 600)


struct ASIFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idINST = MagicBE("INST"),
		idWAVE = MagicBE("WAVE"),
	};

	uint32be id;      // See ChunkIdentifiers
	uint32be length;  // Chunk size without header

	size_t GetLength() const
	{
		return length;
	}

	ChunkIdentifiers GetID() const
	{
		return static_cast<ChunkIdentifiers>(id.get());
	}
};

MPT_BINARY_STRUCT(ASIFChunk, 8)


struct ASIFWaveLists
{
	struct WaveList
	{
		uint8   topKey;  // Supported in theory, could not find any tunes making use of it
		uint8   waveAddress;
		uint8   waveSize;
		uint8   docMode;
		int16le relPitch;  // Ignored by SoundSmith
	};

	WaveList oscA;
	WaveList oscB;

	void ConvertToMPT(const mpt::const_byte_span waveData, ModSample &mptSmp) const
	{
		// The Ensoniq ES-5503-DOC stops playing a sample as soon as it encounters a 0 byte.
		const size_t trimmedLength = std::distance(waveData.begin(), std::find(waveData.begin() + waveData.size() / 2, waveData.end(), std::byte{0}));
		mptSmp.nLength = mpt::saturate_cast<SmpLength>(trimmedLength);
		if(!mptSmp.nLength)
			return;

		const uint8 modeA = (oscA.docMode >> 1) & 0x03, modeB = (oscB.docMode >> 1) & 0x03;
		if(modeA == 0                      // Oscillator A in free run mode
		   || (modeA == 1 && modeB == 3)   // Oscillator A in one-shot mode, B in swap mode
		   || (modeA == 3 && modeB != 2))  // Oscillator A in swap mode, B in free run, one-shot or swap mode
		{
			// In theory oscillator A and B could point at two different waveforms, with oscillator A
			// stopping mid-sample (using a 0 byte) to allow for arbitrary loop start points.
			// Luckily, I could not find any module making use of that in the wild, so we can greatly simplify the code here.
			mptSmp.nLoopStart = (oscB.waveAddress - oscA.waveAddress) * 256;
			mptSmp.nLoopEnd = mptSmp.nLength;
			mptSmp.uFlags.set(CHN_LOOP);
		}

		FileReader smpFile{waveData.subspan(0, trimmedLength)};
		SampleIO{SampleIO::_8bit, SampleIO::mono, SampleIO::littleEndian, SampleIO::unsignedPCM}
			.ReadSample(mptSmp, smpFile);
	}
};

MPT_BINARY_STRUCT(ASIFWaveLists, 12)


#if defined(MPT_EXTERNAL_SAMPLES) || defined(MPT_BUILD_FUZZER)

static bool LoadDOCRAMFile(FileReader &file, mpt::span<ModSample> samples)
{
	file.Rewind();
	if(!file.CanRead(65538 + 92))
		return false;
	
	const uint16 numWaves = file.ReadUint16LE();
	if(numWaves == 0 || numWaves > samples.size())
		return false;

	std::vector<std::byte> waveData;
	for(uint16 i = 0; i < numWaves; i++)
	{
		// Each instrument is comprised of 32 bytes of envelope and other stuff (same as in ASIF file), followed by 5x2 WaveLists (for multisampling)
		if(!file.Seek(65538 + i * 92 + 32))
			return false;
		ASIFWaveLists waveLists;
		if(!file.ReadStruct(waveLists))
			return false;

		if(!file.Seek(2 + waveLists.oscA.waveAddress * 256))
			return false;
		const uint32 waveSize = 1u << ((waveLists.oscA.waveSize & 7) + 8);
		if(file.ReadVector(waveData, waveSize))
			waveLists.ConvertToMPT(waveData, samples[i]);
	}
	return true;
}


static bool LoadASIFFile(ChunkReader file, ModSample &mptSmp)
{
	file.Rewind();
	if(!file.ReadMagic("FORM"))
		return false;
	if(!file.Skip(4))
		return false;
	if(!file.ReadMagic("ASIF"))
		return false;

	ASIFWaveLists waveLists;
	const auto chunks = file.ReadChunks<ASIFChunk>(2);
	if(auto chunk = chunks.GetChunk(ASIFChunk::idINST); chunk.IsValid())
	{
		const uint8 nameLen = chunk.ReadUint8();
		if(!chunk.Skip(nameLen))
			return false;

		chunk.Skip(2 + 24 + 6);  // SampleNum + Envelope + ReleaseSegment, PriorityIncrement, PitchBendRange, VibratoDepth, VibratoSpeed, UpdateRate
		const auto [aWaveCount, bWaveCount] = chunk.ReadArray<uint8, 2>();
		if(aWaveCount != 1 || bWaveCount != 1)
			return false;
		if(!chunk.ReadStruct(waveLists))
			return false;
	} else
	{
		return false;
	}

	if(auto chunk = chunks.GetChunk(ASIFChunk::idWAVE); chunk.IsValid())
	{
		const uint8 nameLen = chunk.ReadUint8();
		if(!chunk.Skip(nameLen))
			return false;

		const auto [waveSize, numSamples, offset] = chunk.ReadArray<uint16le, 3>();
		if(numSamples != 1)
			return false;

		std::vector<std::byte> waveData;
		if(offset > 8)
			chunk.Seek(offset - 8);
		else
			chunk.Skip(2 + 4 + 4);  // Size + OrigFreq + SampRate
		if(chunk.ReadVector(waveData, waveSize))
			waveLists.ConvertToMPT(waveData, mptSmp);
		return true;
	}
	return false;
}

#endif


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSS(MemoryFileReader file, const uint64 *pfilesize)
{
#if defined(MPT_EXTERNAL_SAMPLES) || defined(MPT_BUILD_FUZZER)
	SSFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
		return ProbeWantMoreData;
	if(!fileHeader.IsValid())
		return ProbeFailure;
	return ProbeAdditionalSize(file, pfilesize, fileHeader.GetHeaderMinimumAdditionalSize());
#else
	MPT_UNUSED(file);
	MPT_UNUSED(pfilesize);
	return ProbeFailure;
#endif
}


bool CSoundFile::ReadSS(FileReader &file, ModLoadingFlags loadFlags)
{
#if defined(MPT_EXTERNAL_SAMPLES) || defined(MPT_BUILD_FUZZER)
	SSFileHeader fileHeader;
	file.Rewind();
	if(!file.ReadStruct(fileHeader) || !fileHeader.IsValid())
		return false;
	if(!file.CanRead(fileHeader.GetHeaderMinimumAdditionalSize()))
		return false;
	if(loadFlags == onlyVerifyHeader)
		return true;

	const uint16 patBufSize = fileHeader.patBufSize;
	FileReader noteBuffer = file.ReadChunk(patBufSize);
	FileReader instrEffectBuffer = file.ReadChunk(patBufSize);
	FileReader paramBuffer = file.ReadChunk(patBufSize);

	// Some files miss the footer entirely
	SSFooter footer;
	file.ReadStruct(footer);

	InitializeGlobals(MOD_TYPE_MOD, 14);
	m_SongFlags.set(SONG_IMPORTED | SONG_FORMAT_NO_VOLCOL);
	m_SongFlags.reset(SONG_ISAMIGA);
	Order().SetDefaultTempoInt(125);
	Order().SetDefaultSpeed(fileHeader.speed);
	ReadOrderFromArray(Order(), fileHeader.orders, fileHeader.numOrders & 0xFF);

	const bool isSoundSmith = !std::memcmp(fileHeader.magic, "SONG", 4);
	m_modFormat.formatName = UL_("SoundSmith / Mega Tracker");
	m_modFormat.type = isSoundSmith ? UL_("ss") : UL_("mtp");  // Not that anyone is using these consistently...
	m_modFormat.madeWithTracker = isSoundSmith ? UL_("SoundSmith") : UL_("MegaTracker");
	m_modFormat.charset = mpt::Charset::ASCII;

	// Some files just have all channels panned to the left. Fall back to mono in that case.
	const bool hasStereoInfo = std::adjacent_find(footer.trackPanning.begin(), footer.trackPanning.end(), std::not_equal_to<>{}) != footer.trackPanning.end();
	if(footer.useTrackPanning && hasStereoInfo)
	{
		for(CHANNELINDEX chn = 0; chn < 14; chn++)
		{
			ChnSettings[chn].nPan = (footer.trackPanning[chn] == 0) ? 64 : 192;
		}
	}

	// Reading patterns
	const PATTERNINDEX numPatterns = patBufSize / (14u * 64u);
	if(loadFlags & loadPatternData)
		Patterns.ResizeArray(numPatterns);
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(!(loadFlags & loadPatternData) || !Patterns.Insert(pat, 64))
			continue;

		for(ROWINDEX row = 0; row < 64; row++)
		{
			const auto notes = noteBuffer.ReadArray<uint8, 14>();
			const auto instrEffect = instrEffectBuffer.ReadArray<uint8, 14>();
			const auto params = paramBuffer.ReadArray<uint8, 14>();
			for(CHANNELINDEX chn = 0; chn < 14; chn++)
			{
				ModCommand &m = *Patterns[pat].GetpModCommand(row, chn);
				const uint8 note = notes[chn];
				if(note > 0 && note <= 96)  // In practice, only octaves 2 through 7 can be entered in SoundSmith
					m.note = static_cast<ModCommand::NOTE>(NOTE_MIDDLEC - 36 + note);
				else if(note == 128)  // STP
					m.note = NOTE_NOTECUT;
				else if(note == 129)  // NXT
					m.SetEffectCommand(CMD_PATTERNBREAK, 0);

				m.instr = instrEffect[chn] >> 4;

				const uint8 param = params[chn];
				switch(instrEffect[chn] & 0x0F)
				{
				case 0x00:
					if(param)
						m.SetEffectCommand(CMD_ARPEGGIO, param);
					break;
				case 0x03:
					m.SetEffectCommand(CMD_VOLUME8, param);
					break;
				case 0x05:
					m.SetEffectCommand(CMD_MODCMDEX, static_cast<ModCommand::PARAM>(0xA0 | std::min(static_cast<uint8>(param + 3u / 4u), uint8(15))));
					break;
				case 0x06:
					m.SetEffectCommand(CMD_MODCMDEX, static_cast<ModCommand::PARAM>(0xB0 | std::min(static_cast<uint8>(param + 3u / 4u), uint8(15))));
					break;
				case 0x0F:
					m.SetEffectCommand(CMD_SPEED, std::min(param, uint8(15)));
					break;
				}
			}
		}
	}

	// Reading samples
	m_nSamples = 15;
	for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
	{
		const SSInstrument &ssInstr = fileHeader.instruments[smp - 1];
		Samples[smp].Initialize(MOD_TYPE_MOD);
		// Sometimes, default volume is completely ignored. I couldn't find a pattern to it, except maybe being tied to non-looping samples?! Could also be an emulator bug.
		Samples[smp].nVolume = (ssInstr.volume & 0xFF) + 1;
		if(ssInstr.nameLength)
			m_szNames[smp] = mpt::String::ReadBuf(mpt::String::spacePadded, ssInstr.name, std::min(static_cast<size_t>(ssInstr.nameLength), sizeof(ssInstr.name)));
	}

#ifdef MPT_EXTERNAL_SAMPLES
	if(!(loadFlags & loadSampleData))
		return true;

	if(!file.GetOptionalFileName())
	{
		AddToLog(LogWarning, U_("Module path is not known (compressed file?), samples cannot be loaded."));
		return true;
	}

	const mpt::PathString modFilePath = file.GetOptionalFileName().value();
	const mpt::PathString baseDir = modFilePath.GetDirectoryWithDrive();

	// Try corresponding DOC RAM file first, then generic DOC.DATA
	const mpt::RawPathString docNames[] = {PL_(".D"), PL_(".d"), PL_(".W"), PL_(".w"), PL_(".DOC"), PL_(".doc"), PL_(".WB"), PL_(".wb"), PL_("DOC.DATA"), PL_("DOC.Data")};
	for(const auto &name : docNames)
	{
		mpt::PathString docPath;
		if(name[0] == PC_('.'))
		{
			docPath = modFilePath + name;  // Case: "song.without.extension" + "song.without.extension.w"
			if(!mpt::native_fs{}.is_file(docPath))
				docPath = modFilePath.ReplaceExtension(name);  // Case: "with extension.ss" + "with extension.w"
		} else
		{
			docPath = baseDir + name;
		}
		if(!mpt::native_fs{}.is_file(docPath))
			continue;
			
		mpt::IO::InputFile f{docPath, SettingCacheCompleteFileBeforeLoading()};
		if(f.IsValid())
		{
			FileReader docFile = GetFileReader(f);
			if(LoadDOCRAMFile(docFile, mpt::as_span(Samples).subspan(1, GetNumSamples())))
				return true;
		}
	}

	// Try individual ASIF files if no DOC RAM was found
	bool ok = false;
	for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
	{
		if(m_szNames[smp].empty())
			continue;

		const mpt::PathString instrPath = baseDir + mpt::PathString::FromUnicode(mpt::ToUnicode(m_modFormat.charset, m_szNames[smp]));
		if(!mpt::native_fs{}.is_file(instrPath))
			continue;

		mpt::IO::InputFile f{instrPath, SettingCacheCompleteFileBeforeLoading()};
		if(f.IsValid())
		{
			FileReader asifFile = GetFileReader(f);
			ok |= LoadASIFFile(asifFile, Samples[smp]);
		}
	}

	if(!ok)
		AddToLog(LogWarning, U_("Could not find accompanying DOC RAM file or individual sample files for this module; samples will not play."));
#elif defined(MPT_BUILD_FUZZER)
	// Fuzz DOC RAM and ASIF loaders
	FileReader fuzzChunk = file.ReadChunk(file.BytesLeft());
	LoadDOCRAMFile(fuzzChunk, mpt::as_span(Samples).subspan(1, GetNumSamples()));
	LoadASIFFile(fuzzChunk, Samples[1]);
#endif  // MPT_EXTERNAL_SAMPLES

	return true;
#else
	MPT_UNUSED(file);
	MPT_UNUSED(loadFlags);
	return false;
#endif
}

OPENMPT_NAMESPACE_END
