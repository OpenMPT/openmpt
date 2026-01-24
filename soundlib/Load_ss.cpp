/*
 * Load_ss.cpp
 * -----------
 * Purpose: SoundSmith (Apple IIgs) module loader
 * Notes  : Based on deMODifier by Bret Victor
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "Loaders.h"

OPENMPT_NAMESPACE_BEGIN

// SoundSmith file structures (Apple IIgs format)
struct SSInstrument
{
	uint8  nameLength;
	char   name[21];
	uint8  junk[2];
	uint8  volume[2];      // Little-endian
	uint8  midiProgram[2]; // Little-endian
	uint8  midiVelocity[2];// Little-endian

	uint16 GetVolume() const { return volume[0] | (volume[1] << 8); }
};

MPT_BINARY_STRUCT(SSInstrument, 30)

struct SSFileHeader
{
	char         magicWord[6];     // Should end with "OK"
	uint16le     bufSize;          // Total size of one pattern buffer
	uint8        tempo;
	uint8        junk[11];
	SSInstrument instruments[15];
	uint16le     numOrders;
	uint8        orders[128];

	bool IsValid() const
	{
		return magicWord[0] == 'S' && magicWord[1] == 'O'
			&& magicWord[2] == 'N' && magicWord[3] == 'G'
			&& magicWord[4] == 'O' && magicWord[5] == 'K'
			&& bufSize > 0 && bufSize <= 0x8000;
	}

	uint32 GetHeaderMinimumAdditionalSize() const
	{
		return bufSize * 3 + 30;  // 3 pattern buffers + footer
	}
};

MPT_BINARY_STRUCT(SSFileHeader, 600)

struct SSFooter
{
	uint16le stereo[14];       // Stereo position for each channel
	uint16le useChannelData;   // Whether footer contains valid data
};

MPT_BINARY_STRUCT(SSFooter, 30)

// ASIF (Apple II Sound Instrument Format) structures
struct ASIFWaveList
{
	uint8    topKey;
	uint8    waveAddress;
	uint8    waveSize;
	uint8    docMode;
	uint16le relPitch;
};

MPT_BINARY_STRUCT(ASIFWaveList, 6)

struct ASIFWaveListPair
{
	ASIFWaveList a;
	ASIFWaveList b;

	// Check if this wavelist pair indicates a looping sample (from deMODifier)
	bool IsLooping() const
	{
		uint8 modeA = a.docMode;
		uint8 modeB = b.docMode;
		return ((modeA == 6) && (modeB == 6)) ||
		       ((modeA == 6) && (modeB == 7)) ||
		       ((modeA == 6) && (modeB == 0)) ||
		       ((modeA == 2) && (modeB == 6)) ||
		       ((modeA == 2) && (modeB == 7)) ||
		       ((modeA == 0) && (modeB == 0));
	}
};

MPT_BINARY_STRUCT(ASIFWaveListPair, 12)


// File format probe function
CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSS(MemoryFileReader file, const uint64 *pfilesize)
{
	SSFileHeader fileHeader;
	if(!file.ReadStruct(fileHeader))
		return ProbeWantMoreData;
	if(!fileHeader.IsValid())
		return ProbeFailure;
	return ProbeAdditionalSize(file, pfilesize, fileHeader.GetHeaderMinimumAdditionalSize());
}


#ifdef MPT_EXTERNAL_SAMPLES

// Helper: Find actual wave size by looking for zero terminator
static uint32 FindWaveSize(const std::vector<std::byte> &waveData, uint32 maxSize)
{
	for(uint32 i = 0; i < maxSize && i < waveData.size(); i++)
	{
		if(waveData[i] == std::byte{0})
			return i;
	}
	return std::min(maxSize, static_cast<uint32>(waveData.size()));
}

// Structure to hold loaded DOC RAM data
struct DOCRAMData
{
	std::vector<ASIFWaveListPair> wavelists;
	std::vector<std::vector<std::byte>> waves;
	std::vector<uint32> waveSizes;
	uint8 numWaves = 0;
};

// Load DOC RAM sound bank file - returns data, doesn't modify CSoundFile
static bool LoadDOCRAMData(FileReader &docFile, DOCRAMData &data)
{
	uint8 numWaves = 0, padding = 0;
	if(!docFile.Read(numWaves) || !docFile.Read(padding))
		return false;

	if(numWaves == 0 || numWaves > 15 || padding != 0)
		return false;

	data.numWaves = numWaves;
	data.wavelists.resize(numWaves);
	data.waves.resize(numWaves);
	data.waveSizes.resize(numWaves);

	// Load wavelists (at offset 0x10022 in DOC RAM file)
	if(!docFile.Seek(0x10022))
		return false;

	for(uint8 i = 0; i < numWaves; i++)
	{
		if(!docFile.ReadStruct(data.wavelists[i]))
			return false;
		// Skip remaining ASIF chunk data (0x5c bytes per entry)
		docFile.Skip(0x5c - sizeof(ASIFWaveListPair));
	}

	// Load wave data
	for(uint8 i = 0; i < numWaves; i++)
	{
		const uint8 waveSizeCode = data.wavelists[i].a.waveSize & 7;
		const uint32 maxSize = 1u << (waveSizeCode + 8);  // Size in bytes
		const uint8 waveAddr = data.wavelists[i].a.waveAddress;

		// Seek to wave data (page 0x100 * address + 2 byte header)
		if(!docFile.Seek(2 + 0x100 * waveAddr))
			return false;

		data.waves[i].resize(maxSize);
		docFile.ReadRaw(mpt::as_span(data.waves[i]));

		// Find actual size by looking for zero terminator
		data.waveSizes[i] = FindWaveSize(data.waves[i], maxSize);
	}

	return true;
}

// Structure to hold loaded ASIF data
struct ASIFData
{
	ASIFWaveListPair wavelist{};
	std::vector<std::byte> waveData;
};

// Load individual ASIF instrument file - returns data, doesn't modify ModSample
static bool LoadASIFData(FileReader &asifFile, ASIFData &data)
{
	// Read ASIF header (IFF-style: "FORM" + size + "ASIF")
	char formID[4], asifID[4];
	uint32be fileSize;

	if(!asifFile.ReadArray(formID) || std::memcmp(formID, "FORM", 4) != 0)
		return false;
	if(!asifFile.Read(fileSize))
		return false;
	if(!asifFile.ReadArray(asifID) || std::memcmp(asifID, "ASIF", 4) != 0)
		return false;

	bool foundINST = false;
	bool foundWAVE = false;

	// Search for INST and WAVE chunks
	while(asifFile.CanRead(8) && (!foundINST || !foundWAVE))
	{
		char chunkID[4];
		uint32be chunkSize;

		if(!asifFile.ReadArray(chunkID) || !asifFile.Read(chunkSize))
			break;

		FileReader chunk = asifFile.ReadChunk(chunkSize);

		if(std::memcmp(chunkID, "INST", 4) == 0 && !foundINST)
		{
			// INST chunk: skip name, envelope, and other params to get to wavelist
			uint8 nameLen = 0;
			if(!chunk.Read(nameLen))
				continue;
			chunk.Skip(nameLen);

			// Skip: samplenum(2) + envelope(24) + params before wave counts
			chunk.Skip(2 + 24 + 6);

			uint8 aWaveCount = 0, bWaveCount = 0;
			if(!chunk.Read(aWaveCount) || !chunk.Read(bWaveCount))
				continue;

			// Only support single wavelist pairs
			if(aWaveCount != 1 || bWaveCount != 1)
				continue;

			if(chunk.ReadStruct(data.wavelist))
				foundINST = true;
		}
		else if(std::memcmp(chunkID, "WAVE", 4) == 0 && !foundWAVE)
		{
			// WAVE chunk: skip name to get to wave data
			uint8 nameLen = 0;
			if(!chunk.Read(nameLen))
				continue;
			chunk.Skip(nameLen);

			uint16le waveSize, numSamples;
			if(!chunk.Read(waveSize) || !chunk.Read(numSamples))
				continue;

			// Only support single samples
			if(numSamples != 1)
				continue;

			// Skip: location(2) + size(2) + origFreq(4) + sampRate(4)
			chunk.Skip(2 + 2 + 4 + 4);

			// Read wave data
			data.waveData.resize(waveSize);
			chunk.ReadRaw(mpt::as_span(data.waveData));
			foundWAVE = true;
		}
	}

	return foundINST && foundWAVE && !data.waveData.empty();
}

#endif // MPT_EXTERNAL_SAMPLES


// Main loader function
bool CSoundFile::ReadSS(FileReader &file, ModLoadingFlags loadFlags)
{
	SSFileHeader fileHeader;
	file.Rewind();
	if(!file.ReadStruct(fileHeader) || !fileHeader.IsValid())
		return false;
	if(!file.CanRead(fileHeader.GetHeaderMinimumAdditionalSize()))
		return false;

	// Calculate pattern count
	const uint8 numPatterns = static_cast<uint8>(fileHeader.bufSize / (14 * 64));
	const uint8 numOrders = fileHeader.numOrders & 0x7F;

	if(numPatterns == 0 || numOrders == 0)
		return false;

	if(loadFlags == onlyVerifyHeader)
		return true;

	// Read pattern data (3 buffers)
	const uint32 patternBufSize = fileHeader.bufSize;
	FileReader patternBuf1 = file.ReadChunk(patternBufSize);
	FileReader patternBuf2 = file.ReadChunk(patternBufSize);
	FileReader patternBuf3 = file.ReadChunk(patternBufSize);

	if(!patternBuf1.IsValid() || !patternBuf2.IsValid() || !patternBuf3.IsValid())
		return false;

	// Read footer (at end of file)
	SSFooter footer;
	file.Seek(file.GetLength() - sizeof(SSFooter));
	if(!file.ReadStruct(footer))
		return false;

	// Check if stereo info is valid
	bool hasStereoInfo = false;
	for(int i = 0; i < 14; i++)
	{
		if(footer.stereo[i] != 0)
		{
			hasStereoInfo = true;
			break;
		}
	}

	// If no stereo info, assign default alternating panning
	if(!hasStereoInfo)
	{
		for(int i = 0; i < 14; i++)
			footer.stereo[i] = (i & 1);
	}

	// Initialize module
	InitializeGlobals(MOD_TYPE_MOD, 14);
	m_SongFlags.set(SONG_IMPORTED);
	Order().SetDefaultSpeed(fileHeader.tempo + 1);
	Order().SetDefaultTempoInt(125);

	// Set format metadata
	m_modFormat.formatName = UL_("SoundSmith");
	m_modFormat.type = UL_("ss");
	m_modFormat.madeWithTracker = UL_("SoundSmith");
	m_modFormat.charset = mpt::Charset::ASCII;

	// Set song name from first instrument with a name
	m_songName.clear();
	for(int i = 0; i < 15 && m_songName.empty(); i++)
	{
		if(fileHeader.instruments[i].nameLength > 0)
		{
			m_songName = mpt::String::ReadBuf(mpt::String::spacePadded,
				fileHeader.instruments[i].name,
				std::min<size_t>(fileHeader.instruments[i].nameLength, 21));
		}
	}

	// Create order list
	ReadOrderFromArray(Order(), fileHeader.orders, numOrders);

	// Track last instrument per channel for filling in instrument 0
	// SoundSmith uses instrument 0 to mean "repeat last instrument on this channel"
	uint8 lastInstrument[14] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

	// Allocate patterns
	Patterns.ResizeArray(numPatterns);

	// First pass: convert patterns in order list order for proper instrument 0 handling
	for(ORDERINDEX ord = 0; ord < numOrders; ord++)
	{
		PATTERNINDEX pat = fileHeader.orders[ord];
		if(pat >= numPatterns)
			continue;

		// Skip if pattern already converted
		if(Patterns.IsValidPat(pat))
			continue;

		if(!Patterns.Insert(pat, 64))
			continue;

		// Convert pattern data
		for(ROWINDEX row = 0; row < 64; row++)
		{
			for(CHANNELINDEX chn = 0; chn < 14; chn++)
			{
				const uint32 offset = pat * 14 * 64 + row * 14 + chn;

				uint8 noteVal = 0, instEffect = 0, param = 0;
				patternBuf1.Seek(offset);
				patternBuf2.Seek(offset);
				patternBuf3.Seek(offset);
				patternBuf1.Read(noteVal);
				patternBuf2.Read(instEffect);
				patternBuf3.Read(param);

				ModCommand &m = *Patterns[pat].GetpModCommand(row, chn);

				uint8 inst = instEffect >> 4;
				uint8 effect = instEffect & 0x0F;

				// Handle instrument 0 (repeat last instrument)
				if(inst == 0)
					inst = lastInstrument[chn];
				else
					lastInstrument[chn] = inst;

				// Convert note
				if(noteVal == 129)  // NXT (pattern break)
				{
					m.command = CMD_PATTERNBREAK;
					m.param = 0;
				}
				else if(noteVal == 128)  // STP (key off)
				{
					m.note = NOTE_KEYOFF;
					m.instr = inst;
				}
				else if(noteVal > 0 && noteVal < 128)
				{
					m.note = static_cast<ModCommand::NOTE>(NOTE_MIN + noteVal - 1);
					m.instr = inst;
				}

				// Convert effects
				if(effect == 3)  // Volume
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = static_cast<ModCommand::VOL>(((param >> 1) + 1) >> 1);
				}
				else if(effect == 15)  // Tempo
				{
					m.command = CMD_SPEED;
					m.param = param + 1;
				}
			}
		}
	}

	// Second pass: convert any unreferenced patterns
	for(PATTERNINDEX pat = 0; pat < numPatterns; pat++)
	{
		if(Patterns.IsValidPat(pat))
			continue;

		if(!Patterns.Insert(pat, 64))
			continue;

		for(ROWINDEX row = 0; row < 64; row++)
		{
			for(CHANNELINDEX chn = 0; chn < 14; chn++)
			{
				const uint32 offset = pat * 14 * 64 + row * 14 + chn;

				uint8 noteVal = 0, instEffect = 0, param = 0;
				patternBuf1.Seek(offset);
				patternBuf2.Seek(offset);
				patternBuf3.Seek(offset);
				patternBuf1.Read(noteVal);
				patternBuf2.Read(instEffect);
				patternBuf3.Read(param);

				ModCommand &m = *Patterns[pat].GetpModCommand(row, chn);

				uint8 inst = instEffect >> 4;
				uint8 effect = instEffect & 0x0F;

				// For unreferenced patterns, default instrument 0 to 1
				if(inst == 0)
					inst = 1;

				// Convert note
				if(noteVal == 129)  // NXT (pattern break)
				{
					m.command = CMD_PATTERNBREAK;
					m.param = 0;
				}
				else if(noteVal == 128)  // STP (key off)
				{
					m.note = NOTE_KEYOFF;
					m.instr = inst;
				}
				else if(noteVal > 0 && noteVal < 128)
				{
					m.note = static_cast<ModCommand::NOTE>(NOTE_MIN + noteVal - 1);
					m.instr = inst;
				}

				// Convert effects
				if(effect == 3)  // Volume
				{
					m.volcmd = VOLCMD_VOLUME;
					m.vol = static_cast<ModCommand::VOL>(((param >> 1) + 1) >> 1);
				}
				else if(effect == 15)  // Tempo
				{
					m.command = CMD_SPEED;
					m.param = param + 1;
				}
			}
		}
	}

	// Set channel panning based on stereo info
	for(CHANNELINDEX chn = 0; chn < 14; chn++)
	{
		if(footer.stereo[chn] == 0)
			ChnSettings[chn].nPan = 64;   // Left
		else
			ChnSettings[chn].nPan = 192;  // Right
	}

	// Load samples
	bool loadedSamples = false;

#ifdef MPT_EXTERNAL_SAMPLES
	if((loadFlags & loadSampleData) && file.GetOptionalFileName())
	{
#if defined(MPT_LIBCXX_QUIRK_NO_OPTIONAL_VALUE)
		mpt::PathString basePath = *(file.GetOptionalFileName());
#else
		mpt::PathString basePath = file.GetOptionalFileName().value();
#endif
		mpt::PathString baseDir = basePath.GetDirectoryWithDrive();
		mpt::PathString baseName = basePath.GetFilenameBase();

		// Try to load DOC RAM file first (various extensions)
		DOCRAMData docData;
		const mpt::PathString docExts[] = {P_(".D"), P_(".d"), P_(".W"), P_(".w"), P_(".DOC"), P_(".doc")};
		for(const auto &ext : docExts)
		{
			mpt::PathString docPath = baseDir + baseName + ext;
			if(mpt::native_fs{}.is_file(docPath))
			{
				mpt::IO::InputFile docInputFile(docPath, SettingCacheCompleteFileBeforeLoading());
				if(docInputFile.IsValid())
				{
					FileReader docFile = GetFileReader(docInputFile);
					if(LoadDOCRAMData(docFile, docData))
					{
						loadedSamples = true;
						break;
					}
				}
			}
		}

		// Also try DOC.DATA in same directory
		if(!loadedSamples)
		{
			mpt::PathString docDataPath = baseDir + P_("DOC.DATA");
			if(mpt::native_fs{}.is_file(docDataPath))
			{
				mpt::IO::InputFile docInputFile(docDataPath, SettingCacheCompleteFileBeforeLoading());
				if(docInputFile.IsValid())
				{
					FileReader docFile = GetFileReader(docInputFile);
					if(LoadDOCRAMData(docFile, docData))
					{
						loadedSamples = true;
					}
				}
			}
		}

		// If DOC RAM was loaded, create samples from the data
		if(loadedSamples)
		{
			m_nSamples = 0;
			for(uint8 i = 0; i < docData.numWaves && i < 15; i++)
			{
				SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(i + 1);
				const SSInstrument &ssInst = fileHeader.instruments[i];

				Samples[smp].Initialize();
				Samples[smp].nLength = docData.waveSizes[i];
				Samples[smp].nC5Speed = 8363;  // Default sample rate

				// Set volume from instrument header
				Samples[smp].nVolume = static_cast<uint16>(((ssInst.GetVolume() >> 1) + 1) >> 1) * 4;
				if(Samples[smp].nVolume > 256)
					Samples[smp].nVolume = 256;

				// Set loop info based on DOC mode
				if(docData.wavelists[i].IsLooping() && docData.waveSizes[i] > 0)
				{
					Samples[smp].uFlags.set(CHN_LOOP);
					Samples[smp].nLoopStart = 0;
					Samples[smp].nLoopEnd = docData.waveSizes[i];
				}

				// Allocate and copy sample data (8-bit unsigned)
				if(docData.waveSizes[i] > 0 && Samples[smp].AllocateSample())
				{
					std::memcpy(Samples[smp].samplev(), docData.waves[i].data(), docData.waveSizes[i]);
					m_nSamples = smp;
				}

				// Set sample name from instrument header
				if(ssInst.nameLength > 0)
				{
					m_szNames[smp] = mpt::String::ReadBuf(mpt::String::spacePadded,
						ssInst.name, std::min<size_t>(ssInst.nameLength, 21));
				}
			}
		}

		// If no DOC RAM, try loading individual ASIF files
		if(!loadedSamples)
		{
			m_nSamples = 0;
			for(uint8 i = 0; i < 15; i++)
			{
				const SSInstrument &ssInst = fileHeader.instruments[i];
				if(ssInst.nameLength == 0)
					continue;

				// Extract instrument filename from header
				std::string instName(ssInst.name, std::min<size_t>(ssInst.nameLength, 21));
				// Trim trailing spaces
				while(!instName.empty() && instName.back() == ' ')
					instName.pop_back();

				if(instName.empty())
					continue;

				mpt::PathString instPath = baseDir + mpt::PathString::FromUTF8(instName);

				if(mpt::native_fs{}.is_file(instPath))
				{
					mpt::IO::InputFile asifInputFile(instPath, SettingCacheCompleteFileBeforeLoading());
					if(asifInputFile.IsValid())
					{
						FileReader asifFile = GetFileReader(asifInputFile);
						ASIFData asifData;

						if(LoadASIFData(asifFile, asifData))
						{
							SAMPLEINDEX smp = static_cast<SAMPLEINDEX>(i + 1);

							Samples[smp].Initialize();
							Samples[smp].nLength = static_cast<SmpLength>(asifData.waveData.size());
							Samples[smp].nC5Speed = 8363;

							// Set volume from instrument header
							Samples[smp].nVolume = static_cast<uint16>(((ssInst.GetVolume() >> 1) + 1) >> 1) * 4;
							if(Samples[smp].nVolume > 256)
								Samples[smp].nVolume = 256;

							// Set loop info based on DOC mode
							if(asifData.wavelist.IsLooping() && !asifData.waveData.empty())
							{
								Samples[smp].uFlags.set(CHN_LOOP);
								Samples[smp].nLoopStart = 0;
								Samples[smp].nLoopEnd = static_cast<SmpLength>(asifData.waveData.size());
							}

							// Allocate and copy sample data
							if(!asifData.waveData.empty() && Samples[smp].AllocateSample())
							{
								// Convert the data from GS format to standard PCM, during copy
								for (int idx = 0; idx < asifData.waveData.size(); ++idx)
								{
									int8 sample = (int8)asifData.waveData[ idx ];

									if (sample != 0)
									{
										sample+=0x80;
									}

									Samples[smp].sample8()[ idx ] = sample;
								}

								m_nSamples = std::max(m_nSamples, smp);
								loadedSamples = true;
							}

							// Set sample name
							m_szNames[smp] = mpt::String::ReadBuf(mpt::String::spacePadded,
								ssInst.name, std::min<size_t>(ssInst.nameLength, 21));
						}
					}
				}
			}
		}
	}
#endif // MPT_EXTERNAL_SAMPLES

	// If no samples were loaded, at least set up sample names
	if(!loadedSamples)
	{
		m_nSamples = 0;
		for(SAMPLEINDEX smp = 1; smp <= 15; smp++)
		{
			const SSInstrument &ssInst = fileHeader.instruments[smp - 1];
			if(ssInst.nameLength == 0)
				continue;

			m_nSamples = smp;
			m_szNames[smp] = mpt::String::ReadBuf(mpt::String::spacePadded,
				ssInst.name, std::min<size_t>(ssInst.nameLength, 21));

			// Initialize empty sample with correct volume
			Samples[smp].Initialize();
			Samples[smp].nVolume = static_cast<uint16>(((ssInst.GetVolume() >> 1) + 1) >> 1) * 4;
			if(Samples[smp].nVolume > 256)
				Samples[smp].nVolume = 256;
		}
	}

	return true;
}

OPENMPT_NAMESPACE_END
