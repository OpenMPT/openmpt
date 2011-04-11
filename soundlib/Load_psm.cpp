/*
 * Purpose: Load PSM16 and new PSM (ProTracker Studio) modules
 * Authors: Johannes Schultz
 *
 * This is partly based on http://www.shikadi.net/moddingwiki/ProTracker_Studio_Module
 * and partly reverse-engineered. Also thanks to the author of foo_dumb, the source code
 * gave me a few clues. :)
 *
 * What's playing?
 *  - Epic Pinball - Perfect! (menu and order song are pitched up a bit in the PSM16 format for unknown reasons, but that shouldn't bother anyone)
 *  - Extreme Pinball - Perfect! (subtunes included!)
 *  - Jazz Jackrabbit - Perfect!
 *  - One Must Fall! - Perfect! (it helped a lot to have the original MTM files...)
 *  - Silverball - Seems to work (I don't have all tables so I can't compare)
 *  - Sinaria - Seems to work (never played the game, so I can't really tell...)
 *
 * Effect conversion should be about right...
 * If OpenMPT will ever support subtunes properly, the subtune crap should be rewritten completely.
 */

#include "stdafx.h"
#include "Loaders.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif // MODPLUG_TRACKER

#pragma pack(1)

////////////////////////////////////////////////////////////
//
//  New PSM support starts here. PSM16 structs are below.
//

// 32-Bit PSM header identifiers
#define PSM16HEAD_PSM_ 0xFE4D5350
#define PSMHEAD_PSM_   0x204D5350
#define PSMHEAD_FILE   0x454C4946

// 32-Bit chunk identifiers
#define PSMCHUNKID_TITL 0x4C544954
#define PSMCHUNKID_SDFT 0x54464453
#define PSMCHUNKID_PBOD 0x444F4250
#define PSMCHUNKID_SONG 0x474E4F53
#define PSMCHUNKID_DATE 0x45544144
#define PSMCHUNKID_OPLH 0x484C504F
#define PSMCHUNKID_PPAN 0x4E415050
#define PSMCHUNKID_PATT 0x54544150
#define PSMCHUNKID_DSAM 0x4D415344
#define PSMCHUNKID_DSMP 0x504D5344

struct PSMNEWHEADER
{
	uint32 formatID;			// "PSM " (new format)
	uint32 fileSize;			// Filesize - 12
	uint32 fileInfoID;			// "FILE" Start of file info
};

struct PSMSONGHEADER
{
	char  songType[9];		// Mostly "MAINSONG " (But not in Extreme Pinball!)
	uint8 compression;		// 1 - uncompressed
	uint8 numChannels;		// Number of channels, usually 4

};

struct PSMOLDSAMPLEHEADER // Regular sample header
{
	uint8  flags;
	char   fileName[8];		// Filename of the original module (without extension)
	uint32 sampleID;		// INS0...INS9 (only last digit of sample ID, i.e. sample 1 and sample 11 are equal)
	char   sampleName[33];
	uint8  unknown1[6];		// 00 00 00 00 00 FF
	uint16 sampleNumber;
	uint32 sampleLength;
	uint32 loopStart;
	uint32 loopEnd;			// FF FF FF FF = end of sample
	uint8  unknown3;
	uint8  defaulPan;		// unused?
	uint8  defaultVolume;
	uint32 unknown4;
	uint16 C5Freq;
	uint8  unknown5[21];	// 00 ... 00
};

struct PSMNEWSAMPLEHEADER // Sinaria sample header (and possibly other games)
{
	uint8  flags;
	char   fileName[8];		// Filename of the original module (without extension)
	char   sampleID[8];		// INS0...INS99999
	char   sampleName[33];
	uint8  unknown1[6];		// 00 00 00 00 00 FF
	uint16 sampleNumber;
	uint32 sampleLength;
	uint32 loopStart;
	uint32 loopEnd;
	uint16 unknown3;
	uint8  defaultPan;		// unused?
	uint8  defaultVolume;
	uint32 unknown4;
	uint16 C5Freq;
	char   unknown5[16];	// 00 ... 00
};
#pragma pack()

struct PSMSUBSONG // For internal use (pattern conversion)
{
	uint8 channelPanning[MAX_BASECHANNELS], channelVolume[MAX_BASECHANNELS];
	bool  channelSurround[MAX_BASECHANNELS];
	uint8 defaultTempo, defaultSpeed;
	char  songName[10];
	ORDERINDEX startOrder, endOrder, restartPos;

	PSMSUBSONG()
	{
		memset(channelPanning, 128, sizeof(channelPanning));
		memset(channelVolume, 64, sizeof(channelVolume));
		memset(channelSurround, false, sizeof(channelSurround));
		memset(songName, 0, sizeof(songName));
		defaultTempo = 125;
		defaultSpeed = 6;
		startOrder = endOrder = restartPos = ORDERINDEX_INVALID;
	}
};

// Portamento effect conversion (depending on format version)
inline BYTE convert_psm_porta(BYTE param, bool bNewFormat)
//--------------------------------------------------------
{
	return ((bNewFormat) ? (param) : ((param < 4) ? (param | 0xF0) : (param >> 2)));
}

bool CSoundFile::ReadPSM(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	DWORD dwMemPos = 0;
	bool bNewFormat = false; // The game "Sinaria" uses a slightly modified PSM structure

	ASSERT_CAN_READ(sizeof(PSMNEWHEADER));
	PSMNEWHEADER *shdr = (PSMNEWHEADER *)lpStream;

	if(LittleEndian(shdr->formatID) == PSM16HEAD_PSM_) // "PSMþ" - PSM16 format
		return ReadPSM16(lpStream, dwMemLength);

	// Check header
	if(LittleEndian(shdr->formatID) != PSMHEAD_PSM_ // "PSM "
		|| LittleEndian(shdr->fileSize) != dwMemLength - 12
		|| LittleEndian(shdr->fileInfoID) != PSMHEAD_FILE // "FILE"
		) return false;

	// Yep, this seems to be a valid file.
	m_nType = MOD_TYPE_PSM;
	m_dwSongFlags = SONG_ITOLDEFFECTS | SONG_ITCOMPATGXX;
	SetModFlag(MSF_COMPATIBLE_PLAY, true);
	m_nChannels = 0;

	dwMemPos += 12;

	memset(m_szNames, 0, sizeof(m_szNames));

	m_nVSTiVolume = m_nSamplePreAmp = 48; // not supported in this format, so use a good default value

	// pattern offset and identifier
	PATTERNINDEX numPatterns = 0; // used for setting up the orderlist - final pattern count
	vector<uint32> patternOffsets; // pattern offsets (sorted as they occour in the file)
	vector<uint32> patternIDs; // pattern IDs (sorted as they occour in the file)
	vector<uint32> orderOffsets; // combine the upper two vectors to get the offsets for each order item
	patternOffsets.clear();
	patternIDs.clear();
	orderOffsets.clear();
	Order.clear();
	// subsong setup
	vector<PSMSUBSONG> subsongs;
	bool bSubsongPanningDiffers = false; // do we have subsongs with different panning positions?

	while(dwMemPos + 8 < dwMemLength)
	{
		// Skip through the chunks
		ASSERT_CAN_READ(8);
		uint32 chunkID = LittleEndian(*(uint32 *)(lpStream + dwMemPos));
		uint32 chunkSize = LittleEndian(*(uint32 *)(lpStream + dwMemPos + 4));
		dwMemPos += 8;

		ASSERT_CAN_READ(chunkSize);

		switch(chunkID)
		{
		case PSMCHUNKID_TITL: // "TITL" - Song Title
			memcpy(m_szNames[0], lpStream + dwMemPos, (chunkSize < 31) ? chunkSize : 31);
			SpaceToNullStringFixed<31>(m_szNames[0]);
			break;

		case PSMCHUNKID_SDFT: // "SDFT" - Format info (song data starts here)
			if(chunkSize != 8 || memcmp(lpStream + dwMemPos, "MAINSONG", 8)) return false;
			break;

		case PSMCHUNKID_PBOD: // "PBOD" - Pattern data of a single pattern
			if(chunkSize < 8 || chunkSize != LittleEndian(*(uint32 *)(lpStream + dwMemPos))) return false; // same value twice

			// Pattern ID (something like "P0  " or "P13 ", or "PATT0   " in Sinaria) follows
			if(memcmp(lpStream + dwMemPos + 4, "P", 1)) return false;
			if(!memcmp(lpStream + dwMemPos + 4, "PATT", 4)) bNewFormat = true;
			if(bNewFormat && chunkSize < 12) return false; // 4 additional bytes

			char patternID[4];
			memcpy(patternID, lpStream + dwMemPos + 5 + (bNewFormat ? 3 : 0), 3);
			patternID[3] = 0;
			patternIDs.push_back(atoi(patternID));
			patternOffsets.push_back(dwMemPos + 8 + (bNewFormat ? 4 : 0));

			// Convert later as we have to know how many channels there are.
			break;

		case PSMCHUNKID_SONG: // "SONG" - Subsong information (channel count etc)
			{
				if(chunkSize < sizeof(PSMSONGHEADER)) return false;
				PSMSONGHEADER *pSong = (PSMSONGHEADER *)(lpStream + dwMemPos);
				if(pSong->compression != 0x01) return false; // no compression for PSM files
				m_nChannels = CLAMP(pSong->numChannels, m_nChannels, MAX_BASECHANNELS); // subsongs *might* have different channel count

				PSMSUBSONG subsong;
				subsong.restartPos = (ORDERINDEX)Order.size(); // restart order "offset": current orderlist length
				memcpy(subsong.songName, &pSong->songType, 9); // subsong name
				SpaceToNullStringFixed<9>(subsong.songName);

				DWORD dwChunkPos = dwMemPos + sizeof(PSMSONGHEADER);

				// "Sub sub chunks"
				while(dwChunkPos + 8 < dwMemPos + chunkSize)
				{
					uint32 subChunkID = LittleEndian(*(uint32 *)(lpStream + dwChunkPos));
					uint32 subChunkSize = LittleEndian(*(uint32 *)(lpStream + dwChunkPos + 4));
					dwChunkPos += 8;

					switch(subChunkID)
					{
					case PSMCHUNKID_DATE: // "DATE" - Conversion date (YYMMDD)
						if(subChunkSize != 6) break;

						{
							char cversion[7];
							memcpy(cversion, lpStream + dwChunkPos, 6);
							cversion[6] = 0;
							int version = atoi(cversion);
							// Sinaria song dates (just to go sure...)
							if(version == 800211 || version == 940902 || version == 940903 ||
								version == 940906 || version == 940914 || version == 941213)
								bNewFormat = true;
						}
						break;

					case PSMCHUNKID_OPLH: // "OPLH" - Order list, channel + module settings
						{
							if(subChunkSize < 9) return false;
							// First two bytes = Number of chunks that follow
							//uint16 nTotalChunks = LittleEndian(*(uint16 *)(lpStream + dwChunkPos));
							
							// Now, the interesting part begins!
							DWORD dwSettingsOffset = dwChunkPos + 2;
							uint16 nChunkCount = 0, nFirstOrderChunk = uint16_max;

							// "Sub sub sub chunks" (grrrr, silly format)
							while(dwSettingsOffset - dwChunkPos + 1 < subChunkSize)
							{
								switch(lpStream[dwSettingsOffset])
								{
								case 0x00: // End
									dwSettingsOffset += 1;
									break;

								case 0x01: // Order list item
									if(dwSettingsOffset - dwChunkPos + 5 > subChunkSize) return false;
									// Pattern name follows - find pattern (this is the orderlist)
									{
										char patternID[4]; // temporary
										memcpy(patternID, lpStream + dwSettingsOffset + 2 + (bNewFormat ? 3 : 0), 3);
										patternID[3] = 0;
										uint32 nPattern = atoi(patternID);

										// seek which pattern has this ID
										for(uint32 i = 0; i < patternIDs.size(); i++)
										{
											if(patternIDs[i] == nPattern)
											{
												// found the right pattern, copy offset + start / end positions.
												if(subsong.startOrder == ORDERINDEX_INVALID)
													subsong.startOrder = (ORDERINDEX)orderOffsets.size();
												subsong.endOrder = (ORDERINDEX)orderOffsets.size();

												// every pattern in the order will be unique, so store the pointer + pattern ID
												orderOffsets.push_back(patternOffsets[i]);
												Order.Append(numPatterns);
												numPatterns++;
												break;
											}
										}
									}
									// decide whether this is the first order chunk or not (for finding out the correct restart position)
									if(nFirstOrderChunk == uint16_max) nFirstOrderChunk = nChunkCount;
									dwSettingsOffset += 5 + (bNewFormat ? 4 : 0);
									break;

								case 0x04: // Restart position
									{
										uint16 nRestartChunk = LittleEndian(*(uint16 *)(lpStream + dwSettingsOffset + 1));
										ORDERINDEX nRestartPosition = 0;
										if(nRestartChunk >= nFirstOrderChunk) nRestartPosition = (ORDERINDEX)(nRestartChunk - nFirstOrderChunk);
										subsong.restartPos += nRestartPosition;
									}
									dwSettingsOffset += 3;
									break;

								case 0x07: // Default Speed
									if(dwSettingsOffset - dwChunkPos + 2 > subChunkSize) break;
									subsong.defaultSpeed = lpStream[dwSettingsOffset + 1];
									dwSettingsOffset += 2;
									break;

								case 0x08: // Default Tempo
									if(dwSettingsOffset - dwChunkPos + 2 > subChunkSize) break;
									subsong.defaultTempo =  lpStream[dwSettingsOffset + 1];
									dwSettingsOffset += 2;
									break;

								case 0x0C: // Sample map table (???)
									if(dwSettingsOffset - dwChunkPos + 7 > subChunkSize) break;

									// Never seems to be different, so...
									if (lpStream[dwSettingsOffset + 1] != 0x00 || lpStream[dwSettingsOffset + 2] != 0xFF ||
										lpStream[dwSettingsOffset + 3] != 0x00 || lpStream[dwSettingsOffset + 4] != 0x00 ||
										lpStream[dwSettingsOffset + 5] != 0x01 || lpStream[dwSettingsOffset + 6] != 0x00)
										return false;
									dwSettingsOffset += 7;
									break;

								case 0x0D: // Channel panning table
									if(dwSettingsOffset - dwChunkPos + 4 > subChunkSize) break;

									if(lpStream[dwSettingsOffset + 1] < MAX_BASECHANNELS)
									{
										CHANNELINDEX nChn = lpStream[dwSettingsOffset + 1];
										switch(lpStream[dwSettingsOffset + 3])
										{
										case 0: // use panning
											subsong.channelPanning[nChn] = lpStream[dwSettingsOffset + 2] ^ 128;
											subsong.channelSurround[nChn] = false;
											break;

										case 2: // surround
											subsong.channelPanning[nChn] = 128;
											subsong.channelSurround[nChn] = true;
											break;

										case 4: // center
											subsong.channelPanning[nChn] = 128;
											subsong.channelSurround[nChn] = false;
											break;

										}
										if(bSubsongPanningDiffers == false && subsongs.size() > 0)
										{
											if(subsongs.back().channelPanning[nChn] != subsong.channelPanning[nChn]
												|| subsongs.back().channelSurround[nChn] != subsong.channelSurround[nChn])
												bSubsongPanningDiffers = true;
										}
									}
									dwSettingsOffset += 4;
									break;

								case 0x0E: // Channel volume table (0...255) - apparently always 255
									if(dwSettingsOffset - dwChunkPos + 3 > subChunkSize) break;
									if(lpStream[dwSettingsOffset + 1] < MAX_BASECHANNELS)
										subsong.channelVolume[lpStream[dwSettingsOffset + 1]] = (lpStream[dwSettingsOffset + 2] >> 2) + 1;

									dwSettingsOffset += 3;
									break;
								
								default: // How the hell should this happen? I've listened through almost all existing (original) PSM files. :)
									// anyway, in such cases, we have to quit as we don't know how big the chunk really is.
									return false;
									break;

								}
								nChunkCount++;
							}
							// separate subsongs by "---" patterns
							orderOffsets.push_back(nullptr);
							Order.Append();
						}
						break;

					case PSMCHUNKID_PPAN: // PPAN - Channel panning table (used in Sinaria)
						if(subChunkSize & 1) return false;
						for(uint32 i = 0; i < subChunkSize; i += 2)
						{
							CHANNELINDEX nChn = (CHANNELINDEX)(i >> 1);
							if(nChn >= m_nChannels) break;
							switch(lpStream[dwChunkPos + i])
							{
							case 0: // use panning
								subsong.channelPanning[nChn] = lpStream[dwChunkPos + i + 1] ^ 128;
								subsong.channelSurround[nChn] = false;
								break;

							case 2: // surround
								subsong.channelPanning[nChn] = 128;
								subsong.channelSurround[nChn] = true;
								break;

							case 4: // center
								subsong.channelPanning[nChn] = 128;
								subsong.channelSurround[nChn] = false;
								break;
							}
						}
						break;
					
					case PSMCHUNKID_PATT: // PATT - Pattern list
						// We don't really need this.
						break;

					case PSMCHUNKID_DSAM: // DSAM - Sample list
						// We don't need this either.
						break;

					default:
						break;

					}

					dwChunkPos += subChunkSize;
				}

				// attach this subsong to the subsong list - finally, all "sub sub sub ..." chunks are parsed.
				subsongs.push_back(subsong);
			}

			break;

		case PSMCHUNKID_DSMP: // DSMP - Samples
			if(!bNewFormat)
			{
				// original header
				if(chunkSize < sizeof(PSMOLDSAMPLEHEADER)) return false;
				PSMOLDSAMPLEHEADER *pSample = (PSMOLDSAMPLEHEADER *)(lpStream + dwMemPos);
				SAMPLEINDEX smp = (SAMPLEINDEX)(LittleEndianW(pSample->sampleNumber) + 1);
				m_nSamples = max(m_nSamples, smp);
				memcpy(m_szNames[smp], pSample->sampleName, 31);
				SpaceToNullStringFixed<31>(m_szNames[smp]);
				memcpy(Samples[smp].filename, pSample->fileName, 8);
				SpaceToNullStringFixed<8>(Samples[smp].filename);

				Samples[smp].nGlobalVol = 0x40;
				Samples[smp].nC5Speed = LittleEndianW(pSample->C5Freq);
				Samples[smp].nLength = LittleEndian(pSample->sampleLength);
				Samples[smp].nLoopStart = LittleEndian(pSample->loopStart);
				Samples[smp].nLoopEnd = LittleEndian(pSample->loopEnd);
				Samples[smp].nPan = 128;
				Samples[smp].nVolume = (pSample->defaultVolume + 1) << 1;
				Samples[smp].uFlags = (pSample->flags & 0x80) ? CHN_LOOP : 0;
				if(Samples[smp].nLoopEnd == 0xFFFFFF) Samples[smp].nLoopEnd = Samples[smp].nLength;

				// Delta-encoded samples
				ReadSample(&Samples[smp], RS_PCM8D, (LPCSTR)(lpStream + dwMemPos + sizeof(PSMOLDSAMPLEHEADER)), Samples[smp].nLength);
			} else
			{
				// Sinaria uses a slightly different sample header
				if(chunkSize < sizeof(PSMNEWSAMPLEHEADER)) return false;
				PSMNEWSAMPLEHEADER *pSample = (PSMNEWSAMPLEHEADER *)(lpStream + dwMemPos);
				SAMPLEINDEX smp = (SAMPLEINDEX)(LittleEndianW(pSample->sampleNumber) + 1);
				m_nSamples = max(m_nSamples, smp);
				memcpy(m_szNames[smp], pSample->sampleName, 31);
				SpaceToNullStringFixed<31>(m_szNames[smp]);
				memcpy(Samples[smp].filename, pSample->fileName, 8);
				SpaceToNullStringFixed<8>(Samples[smp].filename);

				Samples[smp].nGlobalVol = 0x40;
				Samples[smp].nC5Speed = LittleEndianW(pSample->C5Freq);
				Samples[smp].nLength = LittleEndian(pSample->sampleLength);
				Samples[smp].nLoopStart = LittleEndian(pSample->loopStart);
				Samples[smp].nLoopEnd = LittleEndian(pSample->loopEnd);
				Samples[smp].nPan = 128;
				Samples[smp].nVolume = (pSample->defaultVolume + 1) << 1;
				Samples[smp].uFlags = (pSample->flags & 0x80) ? CHN_LOOP : 0;
				if(Samples[smp].nLoopEnd == 0xFFFFFF) Samples[smp].nLoopEnd = Samples[smp].nLength;

				// Delta-encoded samples
				ReadSample(&Samples[smp], RS_PCM8D, (LPCSTR)(lpStream + dwMemPos + sizeof(PSMNEWSAMPLEHEADER)), Samples[smp].nLength);
			}

			break;

		default:
			break;

		}

		dwMemPos += chunkSize;
	}

	if(m_nChannels == 0 || subsongs.size() == 0)
		return false;

	// Make the default variables of the first subsong global
	m_nDefaultSpeed = subsongs[0].defaultSpeed;
	m_nDefaultTempo = subsongs[0].defaultTempo;
	m_nRestartPos = subsongs[0].restartPos;
	for(CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++)
	{
		ChnSettings[nChn].nVolume = subsongs[0].channelVolume[nChn];
		ChnSettings[nChn].nPan = subsongs[0].channelPanning[nChn];
		if(subsongs[0].channelSurround[nChn])
			ChnSettings[nChn].dwFlags |= CHN_SURROUND;
		else
			ChnSettings[nChn].dwFlags &= ~CHN_SURROUND;
	}

	// Now that we know the number of channels, we can go through all the patterns.
	// This is a bit stupid since we will even read duplicate patterns twice, but hey, we do this just once... so who cares?
	PATTERNINDEX nPat = 0;
	for(ORDERINDEX nOrd = 0; nOrd < Order.size(); nOrd++)
	{
		if(orderOffsets[nOrd] == nullptr) continue;
		uint32 dwPatternOffset = orderOffsets[nOrd];
		if(dwPatternOffset + 2 > dwMemLength) return false;
		uint16 patternSize = LittleEndianW(*(uint16 *)(lpStream + dwPatternOffset));
		dwPatternOffset += 2;

		if(Patterns.Insert(nPat, patternSize))
			break;

		// Read pattern.
		MODCOMMAND *row_data;
		row_data = Patterns[nPat];

		for(int nRow = 0; nRow < patternSize; nRow++)
		{
			if(dwPatternOffset + 2 > dwMemLength) return false;
			uint16 rowSize = LittleEndianW(*(uint16 *)(lpStream + dwPatternOffset));
			
			uint32 dwRowOffset = dwPatternOffset + 2;

			while(dwRowOffset < dwPatternOffset + rowSize)
			{
				if(dwRowOffset + 1 > dwMemLength) return false;
				BYTE mask = lpStream[dwRowOffset];
				// Point to the correct channel
				MODCOMMAND *m = row_data + min(m_nChannels - 1, lpStream[dwRowOffset + 1]);
				dwRowOffset += 2;

				if(mask & 0x80)
				{
					if(dwRowOffset + 1 > dwMemLength) return false;
					// Note present
					BYTE bNote = lpStream[dwRowOffset];
					if(!bNewFormat)
					{
						if(bNote == 0xFF)
							bNote = NOTE_NOTECUT;
						else
							if(bNote < 129) bNote = (bNote & 0x0F) + 12 * (bNote >> 4) + 13;
					} else
					{
						if(bNote < 85) bNote += 36;
					}
					m->note = bNote;
					dwRowOffset++;
				}

				if(mask & 0x40)
				{
					if(dwRowOffset + 1 > dwMemLength) return false;
					// Instrument present
					m->instr = lpStream[dwRowOffset] + 1;
					dwRowOffset++;
				}

				if(mask & 0x20)
				{
					if(dwRowOffset + 1 > dwMemLength) return false;
					// Volume present
					m->volcmd = VOLCMD_VOLUME;
					m->vol = (min(lpStream[dwRowOffset], 127) + 1) >> 1;
					dwRowOffset++;
				}

				if(mask & 0x10)
				{
					// Effect present - convert
					if(dwRowOffset + 2 > dwMemLength) return false;
					uint8 command = lpStream[dwRowOffset], param = lpStream[dwRowOffset + 1];

					switch(command)
					{
					// Volslides
					case 0x01: // fine volslide up
						command = CMD_VOLUMESLIDE;
						if (bNewFormat) param = (param << 4) | 0x0F;
						else param = ((param & 0x1E) << 3) | 0x0F;
						break;
					case 0x02: // volslide up
						command = CMD_VOLUMESLIDE;
						if (bNewFormat) param = 0xF0 & (param << 4);
						else param = 0xF0 & (param << 3);
						break;
					case 0x03: // fine volslide down
						command = CMD_VOLUMESLIDE;
						if (bNewFormat) param |= 0xF0;
						else param = 0xF0 | (param >> 1);
						break;
					case 0x04: // volslide down
						command = CMD_VOLUMESLIDE;
						if (bNewFormat) param &= 0x0F;
						else if(param < 2) param |= 0xF0; else param = (param >> 1) & 0x0F;
						break;

					// Portamento
					case 0x0B: // fine portamento up
						command = CMD_PORTAMENTOUP;
						param = 0xF0 | convert_psm_porta(param, bNewFormat);
						break;
					case 0x0C: // portamento up
						command = CMD_PORTAMENTOUP;
						param = convert_psm_porta(param, bNewFormat);
						break;
					case 0x0D: // fine portamento down
						command = CMD_PORTAMENTODOWN;
						param = 0xF0 | convert_psm_porta(param, bNewFormat);
						break;
					case 0x0E: // portamento down
						command = CMD_PORTAMENTODOWN;
						param = convert_psm_porta(param, bNewFormat);
						break;					
					case 0x0F: // tone portamento
						command = CMD_TONEPORTAMENTO;
						if(!bNewFormat) param >>= 2;
						break;
					case 0x11: // glissando control
						command = CMD_S3MCMDEX;
						param = 0x10 | (param & 0x01);
						break;
					case 0x10: // tone portamento + volslide up
						command = CMD_TONEPORTAVOL;
						param = param & 0xF0;
						break;
					case 0x12: // tone portamento + volslide down
						command = CMD_TONEPORTAVOL;
						param = (param >> 4) & 0x0F;
						break;						

					// Vibrato
					case 0x15: // vibrato
						command = CMD_VIBRATO;
						break;
					case 0x16: // vibrato waveform
						command = CMD_S3MCMDEX;
						param = 0x30 | (param & 0x0F);
						break;
					case 0x17: // vibrato + volslide up
						command = CMD_VIBRATOVOL;
						param = 0xF0 | param;
						break;
					case 0x18: // vibrato + volslide down
						command = CMD_VIBRATOVOL;
						break;					

					// Tremolo
					case 0x1F: // tremolo
						command = CMD_TREMOLO;
						break;
					case 0x20: // tremolo waveform
						command = CMD_S3MCMDEX;
						param = 0x40 | (param & 0x0F);
						break;

					// Sample commands
					case 0x29: // 3-byte offset - we only support the middle byte.
						if(dwRowOffset + 4 > dwMemLength) return false;
						command = CMD_OFFSET;
						param = lpStream[dwRowOffset + 2];
						dwRowOffset += 2;
						break;
					case 0x2A: // retrigger
						command = CMD_RETRIG;
						break;
					case 0x2B: // note cut
						command = CMD_S3MCMDEX;
						param = 0xC0 | (param & 0x0F);
						break;
					case 0x2C: // note delay
						command = CMD_S3MCMDEX;
						param = 0xD0 | (param & 0x0F);
						break;

					// Position change
					case 0x33: // position jump
						command = CMD_POSITIONJUMP;
						param >>= 1;
						dwRowOffset += 1;
						break;
					case 0x34: // pattern break
						command = CMD_PATTERNBREAK;
						param >>= 1;
						break;
					case 0x35: // loop pattern
						command = CMD_S3MCMDEX;
						param = 0xB0 | (param & 0x0F);
						break;
					case 0x36: // pattern delay
						command = CMD_S3MCMDEX;
						param = 0xE0 | (param & 0x0F);
						break;

					// speed change
					case 0x3D: // set speed
						command = CMD_SPEED;
						break;
					case 0x3E: // set tempo
						command = CMD_TEMPO;
						break;

					// misc commands
					case 0x47: // arpeggio
						command = CMD_ARPEGGIO;
						break;
					case 0x48: // set finetune
						command = CMD_S3MCMDEX;
						param = 0x20 | (param & 0x0F);
						break;
					case 0x49: // set balance
						command = CMD_S3MCMDEX;
						param = 0x80 | (param & 0x0F);
						break;

					case CMD_MODCMDEX:
						// for some strange home-made tunes
						command = CMD_S3MCMDEX;
						break;

					default:
						command = CMD_NONE;
						break;

					}

					m->command = command;
					m->param = param;

					dwRowOffset += 2;
				}
			}

			row_data += m_nChannels;
			dwPatternOffset += rowSize;
		}
		nPat++;
	}

	if(subsongs.size() > 1)
	{
		// write subsong "configuration" to patterns (only if there are multiple subsongs)
		for(uint32 i = 0; i < subsongs.size(); i++)
		{
			PATTERNINDEX startPattern = Order[subsongs[i].startOrder], endPattern = Order[subsongs[i].endOrder];
			if(startPattern == PATTERNINDEX_INVALID || endPattern == PATTERNINDEX_INVALID) continue; // what, invalid subtune?

			// set the subsong name to all pattern names
			for(PATTERNINDEX nPat = startPattern; nPat <= endPattern; nPat++)
			{
				SetPatternName(nPat, subsongs[i].songName);
			}

			// subsongs with different panning setup -> write to pattern (MUSIC_C.PSM)
			if(bSubsongPanningDiffers)
			{
				for(CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++)
				{
					if(subsongs[i].channelSurround[nChn] == true)
						TryWriteEffect(startPattern, 0, CMD_S3MCMDEX, 0x91, false, nChn, false, weTryNextRow);
					else
						TryWriteEffect(startPattern, 0, CMD_PANNING8, subsongs[i].channelPanning[nChn], false, nChn, false, weTryNextRow);
				}
			}
			// write default tempo/speed to pattern
			TryWriteEffect(startPattern, 0, CMD_SPEED, subsongs[i].defaultSpeed, false, CHANNELINDEX_INVALID, false, weTryNextRow);
			TryWriteEffect(startPattern, 0, CMD_TEMPO, subsongs[i].defaultTempo, false, CHANNELINDEX_INVALID, false, weTryNextRow);

			// don't write channel volume for now, as it's always set to 100% anyway

			// there's a restart pos, so let's try to insert a Bxx command in the last pattern
			if(subsongs[i].restartPos != ORDERINDEX_INVALID)
			{
				ROWINDEX lastRow = Patterns[endPattern].GetNumRows() - 1;
				MODCOMMAND *row_data;
				row_data = Patterns[endPattern];
				for(uint32 nCell = 0; nCell < m_nChannels * Patterns[endPattern].GetNumRows(); nCell++, row_data++)
				{
					if(row_data->command == CMD_PATTERNBREAK || row_data->command == CMD_POSITIONJUMP)
					{
						lastRow = nCell / m_nChannels;
						break;
					}
				}
				TryWriteEffect(endPattern, lastRow, CMD_POSITIONJUMP, (BYTE)subsongs[i].restartPos, false, CHANNELINDEX_INVALID, false, weTryNextRow);
			}
		}
	}

	return true;
}

////////////////////////////////
//
//  PSM16 support starts here.
//

#pragma pack(1)

struct PSM16HEADER
{
	uint32 formatID;		// "PSMþ" (PSM16)
	char   songName[59];	// Song title, padded with nulls
	uint8  lineEnd;			// $1A
	uint8  songType;		// Song Type bitfield
	uint8  formatVersion;	// $10
	uint8  patternVersion;  // 0 or 1
	uint8  songSpeed;		//
	uint8  songTempo;		// 32 ... 255
	uint8  masterVolume;	// 0 ... 255
	uint16 songLength;		// 0 ... 255 (number of patterns to play in the song)
	uint16 songOrders;		// 0 ... 255 (same as previous value as no subsongs are present)
	uint16 numPatterns;		// 1 ... 255
	uint16 numSamples;		// 1 ... 255
	uint16 numChannelsPlay;	// 0 ... 32 (max. number of channels to play)
	uint16 numChannelsReal;	// 0 ... 32 (max. number of channels to process)
	uint32 orderOffset;
	uint32 panOffset;
	uint32 patOffset;
	uint32 smpOffset;
	uint32 commentsOffset;
	uint32 patSize;			// Size of all patterns
	uint8  filler[40];
};

struct PSM16SMPHEADER
{
	uint8 filename[13];	// null-terminated
	uint8 name[24];		// dito
	uint32 offset;		// in file
	uint32 memoffset;	// not used
	uint16 sampleNumber;// 1 ... 255
	uint8  flags;		// sample flag bitfield
	uint32 length;		// in bytes
	uint32 loopStart;	// in samples?
	uint32 loopEnd;		// in samples?
	int8   finetune;	// 0 ... 15 (useless? also, why is this almost always 70?)
	uint8  volume;		// default volume
	uint16 c2freq;
};

struct PSM16PATHEADER
{
	uint16 size;		// includes header bytes
	uint8  numRows;		// 1 ... 64
	uint8  numChans;	// 1 ... 31
};

#pragma pack()


bool CSoundFile::ReadPSM16(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(sizeof(PSM16HEADER));
	PSM16HEADER *shdr = (PSM16HEADER *)lpStream;

	// Check header
	if((LittleEndian(shdr->formatID) != PSM16HEAD_PSM_) // "PSMþ"
		|| (shdr->lineEnd != 0x1A)
		|| (shdr->formatVersion != 0x10 && shdr->formatVersion != 0x01) // why is this sometimes 0x01?
		|| (shdr->patternVersion != 0) // 255ch pattern version not supported (did anyone use this?)
		|| ((shdr->songType & 3) != 0)
		|| (min(shdr->numChannelsPlay, shdr->numChannelsReal) == 0)
		) return false;

	// Seems to be valid!

	m_nType = MOD_TYPE_S3M;
	m_nChannels = min(max(shdr->numChannelsPlay, shdr->numChannelsReal), MAX_BASECHANNELS);
	m_nMasterVolume = shdr->masterVolume;
	m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	m_nDefaultSpeed = shdr->songSpeed;
	m_nDefaultTempo = shdr->songTempo;

	memset(m_szNames, 0, sizeof(m_szNames));
	memcpy(m_szNames[0], shdr->songName, 31);
	SpaceToNullStringFixed<31>(m_szNames[0]);

	// Read orders
	dwMemPos = LittleEndian(shdr->orderOffset);
	ASSERT_CAN_READ((DWORD)LittleEndianW(shdr->songOrders) + 2);
	if(LittleEndian(shdr->orderOffset) > 4 && LittleEndian(*(uint32 *)(lpStream + dwMemPos - 4)) == 0x44524f50) // PORD
	{
		Order.ReadAsByte(lpStream + dwMemPos, LittleEndianW(shdr->songOrders), dwMemLength - dwMemPos);
	}

	// Read pan positions
	dwMemPos = LittleEndian(shdr->panOffset);
	ASSERT_CAN_READ(32);
	if(LittleEndian(shdr->panOffset) > 4 && LittleEndian(*(uint32 *)(lpStream + dwMemPos - 4)) == 0x4E415050) // PPAN
	{
		for(CHANNELINDEX i = 0; i < 32; i++)
		{
			ChnSettings[i].nPan = lpStream[dwMemPos + i] << 4;
			ChnSettings[i].nVolume = 64;
			ChnSettings[i].dwFlags = 0; // (i >= shdr->numChannelsPlay) ? CHN_MUTE : 0; // don't mute channels, as muted channels are completely ignored in S3M
		}
	}

	// Read samples
	dwMemPos = LittleEndian(shdr->smpOffset);
	ASSERT_CAN_READ(0);
	if(LittleEndian(shdr->smpOffset) > 4 && LittleEndian(*(uint32 *)(lpStream + dwMemPos - 4)) == 0x48415350) // PSAH
	{
		SAMPLEINDEX iSmpCount = 0;
		m_nSamples = LittleEndianW(shdr->numSamples);
		while(iSmpCount < LittleEndianW(shdr->numSamples))
		{
			ASSERT_CAN_READ(sizeof(PSM16SMPHEADER));
			PSM16SMPHEADER *smphdr = (PSM16SMPHEADER *)(lpStream + dwMemPos);
			dwMemPos += sizeof(PSM16SMPHEADER);

			SAMPLEINDEX iSmp = LittleEndianW(smphdr->sampleNumber);
			m_nSamples = max(m_nSamples, iSmp);

			memcpy(m_szNames[iSmp], smphdr->name, 24);
			SpaceToNullStringFixed<24>(m_szNames[iSmp]);
			memcpy(Samples[iSmp].filename, smphdr->filename, 13);
			SpaceToNullStringFixed<13>(Samples[iSmp].filename);

			Samples[iSmp].nLength = LittleEndian(smphdr->length);
			Samples[iSmp].nLoopStart = LittleEndian(smphdr->loopStart);
			Samples[iSmp].nLoopEnd = LittleEndian(smphdr->loopEnd);
			Samples[iSmp].nC5Speed = LittleEndianW(smphdr->c2freq);
			Samples[iSmp].nVolume = smphdr->volume << 2;
			Samples[iSmp].nGlobalVol = 256;

			UINT iSampleFormat = RS_PCM8S;
			if(smphdr->flags & 0x04) // 16-Bit
			{
				Samples[iSmp].uFlags |= CHN_16BIT;
				Samples[iSmp].nLength >>= 1;
				iSampleFormat = RS_PCM16S;
			}
			if(smphdr->flags & 0x08) // Signed/Unsigned
			{
				if(Samples[iSmp].uFlags & CHN_16BIT)
					iSampleFormat = RS_PCM16U;
				else
					iSampleFormat = RS_PCM8U;
			}
			if(smphdr->flags & 0x10) // Delta/Raw
			{
				if(Samples[iSmp].uFlags & CHN_16BIT)
					iSampleFormat = RS_PCM16D;
				else
					iSampleFormat = RS_PCM8D;
			}
			if(smphdr->flags & 0x20) // Bidi Loop
			{
				Samples[iSmp].uFlags |= CHN_PINGPONGLOOP;
			}
			if(smphdr->flags & 0x80) // Loop
			{
				Samples[iSmp].uFlags |= CHN_LOOP;
			}
			if((smphdr->flags & 0x7F) == 0)
				iSampleFormat = RS_PCM8D;

			ReadSample(&Samples[iSmp], iSampleFormat, reinterpret_cast<LPCSTR>(lpStream + LittleEndianW(smphdr->offset)), dwMemLength - LittleEndianW(smphdr->offset));

			iSmpCount++;
		}
	}

	// Read patterns
	dwMemPos = LittleEndian(shdr->patOffset);
	ASSERT_CAN_READ(LittleEndian(shdr->patSize));
	if(LittleEndian(shdr->patOffset) > 4 && LittleEndian(*(DWORD *)(lpStream + dwMemPos - 4)) == 0x54415050) // PPAT
	{
		DWORD dwPatEndPos = LittleEndian(shdr->patOffset) + LittleEndian(shdr->patSize);

		for(PATTERNINDEX nPat = 0; nPat < LittleEndianW(shdr->numPatterns); nPat++)
		{
			ASSERT_CAN_READ(sizeof(PSM16PATHEADER));
			PSM16PATHEADER *phdr = (PSM16PATHEADER *)(lpStream + dwMemPos);
			ASSERT_CAN_READ(LittleEndianW(phdr->size));

			DWORD dwNextPattern = dwMemPos + ((LittleEndianW(phdr->size) + 15) & ~15);
			dwMemPos += sizeof(PSM16PATHEADER);

			if(Patterns.Insert(nPat, phdr->numRows))
				break;

			MODCOMMAND *row_data;
			ROWINDEX iRow = 0;

			while(dwMemPos < dwNextPattern && iRow < phdr->numRows)
			{
				ASSERT_CAN_READ(1);
				BYTE bChnFlag = lpStream[dwMemPos++];
				if(bChnFlag == 0)
				{
					iRow++;
					continue;
				}

				row_data = Patterns[nPat] + iRow * m_nChannels + min(bChnFlag & 0x1F, m_nChannels - 1);

				if(bChnFlag & 0x80)
				{
					// note + instr present
					ASSERT_CAN_READ(2);
					row_data->note = lpStream[dwMemPos++] + 37;
					row_data->instr = lpStream[dwMemPos++];
				}
				if(bChnFlag & 0x40)
				{
					// volume present
					ASSERT_CAN_READ(1);
					row_data->volcmd = VOLCMD_VOLUME;
					row_data->vol = lpStream[dwMemPos++];
				}
				if(bChnFlag & 0x20)
				{
					// effect present - convert
					ASSERT_CAN_READ(2);
					BYTE command = lpStream[dwMemPos++], param = lpStream[dwMemPos++];

					switch(command)
					{
					// Volslides
					case 0x01: // fine volslide up
						command = CMD_VOLUMESLIDE;
						param = (param << 4) | 0x0F;
						break;
					case 0x02: // volslide up
						command = CMD_VOLUMESLIDE;
						param = (param << 4) & 0xF0;
						break;
					case 0x03: // fine voslide down
						command = CMD_VOLUMESLIDE;
						param = 0xF0 | param;
						break;
					case 0x04: // volslide down
						command = CMD_VOLUMESLIDE;
						param = param & 0x0F;
						break;

					// Portamento
					case 0x0A: // fine portamento up
						command = CMD_PORTAMENTOUP;
						param |= 0xF0;
						break;
					case 0x0B: // portamento down
						command = CMD_PORTAMENTOUP;
						break;
					case 0x0C: // fine portamento down
						command = CMD_PORTAMENTODOWN;
						param |= 0xF0;
						break;
					case 0x0D: // portamento down
						command = CMD_PORTAMENTODOWN;
						break;
					case 0x0E: // tone portamento
						command = CMD_TONEPORTAMENTO;
						break;
					case 0x0F: // glissando control
						command = CMD_S3MCMDEX;
						param |= 0x10;
						break;
					case 0x10: // tone portamento + volslide up
						command = CMD_TONEPORTAVOL;
						param <<= 4;
						break;
					case 0x11: // tone portamento + volslide down
						command = CMD_TONEPORTAVOL;
						param &= 0x0F;
						break;

					// Vibrato
					case 0x14: // vibrato
						command = CMD_VIBRATO;
						break;
					case 0x15: // vibrato waveform
						command = CMD_S3MCMDEX;
						param |= 0x30;
						break;
					case 0x16: // vibrato + volslide up
						command = CMD_VIBRATOVOL;
						param <<= 4;
						break;
					case 0x17: // vibrato + volslide down
						command = CMD_VIBRATOVOL;
						param &= 0x0F;
						break;

					// Tremolo
					case 0x1E: // tremolo
						command = CMD_TREMOLO;
						break;
					case 0x1F: // tremolo waveform
						command = CMD_S3MCMDEX;
						param |= 0x40;
						break;

					// Sample commands
					case 0x28: // 3-byte offset - we only support the middle byte.
						ASSERT_CAN_READ(2);
						command = CMD_OFFSET;
						param = lpStream[dwMemPos++];
						dwMemPos++;
						break;
					case 0x29: // retrigger
						command = CMD_RETRIG;
						param &= 0x0F;
						break;
					case 0x2A: // note cut
						command = CMD_S3MCMDEX;
						if(param == 0)	// in S3M mode, SC0 is ignored, so we convert it to a note cut.
						{
							if(row_data->note == NOTE_NONE)
							{
								row_data->note = NOTE_NOTECUT;
								command = CMD_NONE;
							} else
							{
								param = 1;
							}
						}
						param |= 0xC0;
						break;
					case 0x2B: // note delay
						command = CMD_S3MCMDEX;
						param |= 0xD0;
						break;

					// Position change
					case 0x32: // position jump
						command = CMD_POSITIONJUMP;
						break;
					case 0x33: // pattern break
						command = CMD_PATTERNBREAK;
						break;
					case 0x34: // loop pattern
						command = CMD_S3MCMDEX;
						param |= 0xB0;
						break;
					case 0x35: // pattern delay
						command = CMD_S3MCMDEX;
						param |= 0xE0;
						break;

					// speed change
					case 0x3C: // set speed
						command = CMD_SPEED;
						break;
					case 0x3D: // set tempo
						command = CMD_TEMPO;
						break;

					// misc commands
					case 0x46: // arpeggio
						command = CMD_ARPEGGIO;
						break;
					case 0x47: // set finetune
						command = CMD_S3MCMDEX;
						param |= 0x20;
						break;
					case 0x48: // set balance (panning?)
						command = CMD_PANNING8;
						param = (param << 4) + 8;
						break;

					default:
						command = CMD_NONE;
						break;
					}

					row_data->command = command;
					row_data->param = param;
				}
			}
			// Pattern break for short patterns (so saving the modules as S3M won't break it)
			if(phdr->numRows != 64)
				TryWriteEffect(nPat, phdr->numRows - 1, CMD_PATTERNBREAK, 0, false, CHANNELINDEX_INVALID, false, weTryNextRow);

			dwMemPos = dwNextPattern;
			if(dwMemPos > dwPatEndPos) break;
		}
	}

	return true;
}
