/*
 * Purpose: Load new PSM (ProTracker Studio) modules
 * Authors: Johannes Schultz
 *
 * This is partly based on http://www.shikadi.net/moddingwiki/ProTracker_Studio_Module
 * and partly reverse-engineered.
 *
 * What's playing?
 *  - Epic Pinball - Seems to be working perfectly with linear freq slides enabled
 *  - Extreme Pinball - Default tempo / speed / restart position of subtunes is missing.
 *    I'm using the last default values, restart position is still completely missing
 *  - Jazz Jackrabbit - Hmm, I don't like some of the portas (BONUS.PSM - the guitar portamento is too "deep"), but apart from that it's great.
 *  - One Must Fall! - Perfect! (I modelled the volume slide and portamento conversion after this, as I got the original MTM files)
 *  - Silverball - Currently not supported (old PSM16 format)
 *
 * Effect conversion should be about right, however I have my doubts wheter linear freq slides are used or not.
 */

#include "stdafx.h"
#include "sndfile.h"

#pragma pack(1)

struct PSMHEADER
{
	DWORD formatID;			// "PSM " (new format)
	DWORD fileSize;			// Filesize - 12
	DWORD fileInfoID;		// "FILE" Start of file info
};

struct PSMSONGHEADER
{
	CHAR songType[9];		// Mostly "MAINSONG " (But not in Extreme Pinball!)
	BYTE compression;		// 1 - uncompressed
	BYTE numChannels;		// Number of channels, usually 4

};

struct PSMSAMPLEHEADERTEST // apparently, this is wrong.
{
	CHAR sampleFormat[9];
	DWORD sampleID;			// INS0...INS9 (only last digit of sample ID, i.e. sample 1 and sample 11 are equal)
	CHAR sampleName[33];
	CHAR unknown1[6];		// 00 00 00 00 00 FF
	WORD sampleNumber;
	DWORD loopStart;
	CHAR loop;				// 1 = loop
	CHAR unknown2[3];		// 00 00 00 00 00 FF
	DWORD loopEnd;			// FF FF FF FF = end of sample
	WORD unknown3;
	CHAR defaultVolume;
	DWORD unknown4;
	WORD C5Freq;
	CHAR padding[21];		// 00 ... 00
};

struct PSMSAMPLEHEADER // Use this instead
{
	BYTE flags;
	CHAR fileName[8];
	DWORD sampleID;			// INS0...INS9 (only last digit of sample ID, i.e. sample 1 and sample 11 are equal)
	CHAR sampleName[33];
	CHAR unknown1[6];		// 00 00 00 00 00 FF
	WORD sampleNumber;
	DWORD sampleLength;
	DWORD loopStart;
	DWORD loopEnd;			// FF FF FF FF = end of sample
	WORD unknown3;
	CHAR defaultVolume;
	DWORD unknown4;
	WORD C5Freq;
	CHAR unknown5[21];		// 00 ... 00
};

#pragma pack()

bool CSoundFile::ReadPSM(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	#define ASSERT_CAN_READ(x) \
	if( dwMemPos > dwMemLength || x > dwMemLength - dwMemPos ) return false;

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(12);
	PSMHEADER shdr;
	memcpy(&shdr, lpStream, sizeof(PSMHEADER));
	if(LittleEndian(shdr.formatID) != 0x204d5350 // "PSM "
		|| LittleEndian(shdr.fileSize) != dwMemLength - 12
		|| LittleEndian(shdr.fileInfoID) != 0x454C4946 // "FILE"
		) return false;

	// Yep, this seems to be a valid file.
	m_nType = MOD_TYPE_PSM;
	m_dwSongFlags |= SONG_LINEARSLIDES; // Seems to be correct for Epic Pinball and Jazz Jackrabbit?
	m_nChannels = 0;

	dwMemPos += 12;

	memset(m_szNames, 0, sizeof(m_szNames));

	m_nVSTiVolume = m_nSamplePreAmp = 48; // not supported in this format, so use a good default value

	// pattern offset and identifier
	PATTERNINDEX numPatterns = 0;
	vector<DWORD> patternOffsets;
	vector<DWORD> patternIDs;
	patternOffsets.clear();
	patternIDs.clear();
	Order.clear();

	std::string sComment; // we will store some information about the tune here

	while(dwMemPos + 8 < dwMemLength)
	{
		// Skip through the chunks
		ASSERT_CAN_READ(8);
		DWORD chunkID = LittleEndian(*(DWORD *)(lpStream + dwMemPos));
		DWORD chunkSize = LittleEndian(*(DWORD *)(lpStream + dwMemPos + 4));
		dwMemPos += 8;

		ASSERT_CAN_READ(chunkSize);

		switch(chunkID)
		{
		case 0x4C544954: // "TITL" - Song Title
			memcpy(m_szNames[0], lpStream + dwMemPos, (chunkSize < 31) ? chunkSize : 31);
			m_szNames[0][31] = 0;
			break;

		case 0x54464453: // "SDFT" - Format info (song data starts here)
			if(chunkSize != 8 || memcmp(lpStream + dwMemPos, "MAINSONG", 8)) return false;
			break;

		case 0x444F4250: // "PBOD" - Pattern data of a single pattern
			if(chunkSize < 4 || chunkSize != LittleEndian(*(DWORD *)(lpStream + dwMemPos))) return false; // same value twice

			// Pattern ID (something like "P0  " or "P13 ") follows
			if(memcmp(lpStream + dwMemPos + 4, "P", 1)) return false;
			char patternID[4];
			memcpy(patternID, lpStream + dwMemPos + 5, 3);
			patternID[3] = 0;
			patternOffsets.push_back(dwMemPos + 8);
			patternIDs.push_back(atoi(patternID));
			numPatterns++;

			// Convert later as we have to know how many channels there are.
			break;

		case 0x474E4F53: // "SONG" - Information about this file (channel count etc)
			// We will not check for the "song type", because it is not ALWAYS "MAINSONG " (Extreme Pinball seems to use other song types for the sub songs)
			// However, the last char always seems to be a space char.
			{
				if(chunkSize < sizeof(PSMSONGHEADER)) return false;
				PSMSONGHEADER *pSong = (PSMSONGHEADER *)(lpStream + dwMemPos);
				if(pSong->compression != 0x01) return false;
				m_nChannels = max(m_nChannels, pSong->numChannels); // subsongs *might* have different channel count

				CHAR cSongName[10];
				memcpy(cSongName, &pSong->songType, 9);
				cSongName[9] = 0;
				sComment += "\r\nSubsong: ";
				sComment += cSongName;

				DWORD dwChunkPos = dwMemPos + sizeof(PSMSONGHEADER);

				// Sub chunks
				while(dwChunkPos + 8 < dwMemPos + chunkSize)
				{
					DWORD subChunkID = LittleEndian(*(DWORD *)(lpStream + dwChunkPos));
					DWORD subChunkSize = LittleEndian(*(DWORD *)(lpStream + dwChunkPos + 4));
					dwChunkPos += 8;

					switch(subChunkID)
					{
					case 0x45544144: // "DATE" - Song date (YYMMDD)
						if(subChunkSize < 6) break;

						CHAR cDate[7];
						memcpy(cDate, lpStream + dwChunkPos, 6);
						cDate[6] = 0;
						sComment += "\r\nDate: ";
						sComment += cDate;
						break;

					case 0x484C504F: // "OPLH" - Order list, channel + module settings
						{
							if(subChunkSize < 9) return false;
							// First byte = "Memory alloc, roughly ordlen + 10, if too small, song freezes"
							if(LittleEndian(*(DWORD *)(lpStream + dwChunkPos + 1)) != 0xFF000C00 // "Something"
								|| LittleEndian(*(DWORD *)(lpStream + dwChunkPos + 5)) != 0x00010000) // "Something"
								return false;
							
							// Now, the interesting part begins!
							DWORD dwSettingsOffset = dwChunkPos + 9;
							while(dwSettingsOffset - dwChunkPos + 1 < subChunkSize)
							{
								switch(lpStream[dwSettingsOffset])
								{
								case 0x00: // Seems to be the End item
									dwSettingsOffset += 1;
									break;

								case 0x01: // Order list item
									if(dwSettingsOffset - dwChunkPos + 5 > subChunkSize) return false;
									// Pattern name follows - find pattern (this is the orderlist)
									for(PATTERNINDEX i = 0; i < patternIDs.size(); i++)
									{
										char patternID[4];
										memcpy(patternID, lpStream + dwSettingsOffset + 2, 3);
										patternID[3] = 0;
										DWORD nPattern = atoi(patternID);
										if(patternIDs[i] == nPattern)
										{
											Order.push_back(i);
											break;
										}
									}
									dwSettingsOffset += 5;
									break;

								case 0x04:
									/* It looks like the 2nd number of this chunk could be the restart position,
									   where position = ((number < 15) ? 0 : (number - 15)) */
									#ifdef DEBUG
									{
										char s[32];
										wsprintf(s, " - restart %d", (lpStream[dwSettingsOffset + 1] < 15) ? 0 : lpStream[dwSettingsOffset + 1] - 15);
										sComment += s;
										dwSettingsOffset += 3;
									}
									#endif
									break;

								case 0x07: // Default Speed
									if(dwSettingsOffset - dwChunkPos + 2 > subChunkSize) break;
									// NOTE: This should not be global! (Extreme Pinball!!!)
									m_nDefaultSpeed =  lpStream[dwSettingsOffset + 1];
									dwSettingsOffset += 2;
									break;

								case 0x08: // Default Tempo
									if(dwSettingsOffset - dwChunkPos + 2 > subChunkSize) break;
									// Same note as above!
									m_nDefaultTempo =  lpStream[dwSettingsOffset + 1];
									dwSettingsOffset += 2;
									break;

								case 0x0D: // This is a channel header
									if(dwSettingsOffset - dwChunkPos + 4 > subChunkSize) break;
									ChnSettings[min(lpStream[dwSettingsOffset + 1], pSong->numChannels)].nPan = lpStream[dwSettingsOffset + 2];
									// Last byte: "either 0, 2 or 4 (Some sort of priority?)"
									dwSettingsOffset += 4;
									break;

								case 0x0E: // Unknown - "0E <ChnID> FF"
									if(dwSettingsOffset - dwChunkPos + 3 > subChunkSize) break;
									dwSettingsOffset += 3;
									break;
								
								default: // How the hell should this happen? I've listened through all existing (original) PSM files. :)
									CString s;
									s.Format("Please report to the OpenMPT team: Unknown chunk %d found at position %d (in the OPLH chunk of this PSM file)", lpStream[dwSettingsOffset], dwSettingsOffset);
									MessageBox(NULL, s, TEXT("OpenMPT PSM import"), MB_ICONERROR);
									return false;
									break;

								}
							}
							Order.push_back(Order.GetInvalidPatIndex());
						}
						break;

					case 0x54544150: // PATT - Pattern list
						// We don't really need this.
						break;

					case 0x4D415344: // DSAM - Sample list
						// We don't need this either.
						break;

					default:
						break;

					}

					dwChunkPos += subChunkSize;
				}
			}

			break;

		case 0x504D5344: // DSMP - Samples

			{
				if(chunkSize < sizeof(PSMSAMPLEHEADER)) return false;
				PSMSAMPLEHEADER *pSample = (PSMSAMPLEHEADER *)(lpStream + dwMemPos);
				SAMPLEINDEX smp = (SAMPLEINDEX)(LittleEndianW(pSample->sampleNumber) + 1);
				m_nSamples = max(m_nSamples, smp);
				memcpy(m_szNames[smp], pSample->sampleName, 31);
				m_szNames[smp][31] = 0;
				memcpy(Samples[smp].filename, pSample->fileName, 8);
				Samples[smp].filename[8] = 0;

				Samples[smp].nGlobalVol = 0x40;
				Samples[smp].nC5Speed = LittleEndianW(pSample->C5Freq);
				Samples[smp].nLength = LittleEndian(pSample->sampleLength);
				Samples[smp].nLoopStart = LittleEndian(pSample->loopStart);
				Samples[smp].nLoopEnd = LittleEndian(pSample->loopEnd);
				Samples[smp].nPan = 128;
				Samples[smp].nVolume = (pSample->defaultVolume + 1) << 1;
				Samples[smp].uFlags = (pSample->flags & 0x80) ? CHN_LOOP : 0;
				if(Samples[smp].nLoopStart > 0) Samples[smp].nLoopStart--;
				if(Samples[smp].nLoopEnd == 0xFFFFFF) Samples[smp].nLoopEnd = Samples[smp].nLength;

				// Delta-encoded samples
				ReadSample(&Samples[smp], RS_PCM8D, (LPCSTR)(lpStream + dwMemPos + sizeof(PSMSAMPLEHEADER)), Samples[smp].nLength);
			}

			break;

		default:
			break;

		}

		dwMemPos += chunkSize;
	}

	if(m_nChannels == 0)
		return false;
	// Now that we know the number of channels, we can go through all the patterns.
	for(PATTERNINDEX i = 0; i < numPatterns; i++)
	{
		DWORD dwPatternOffset = patternOffsets[i];
		if(dwPatternOffset + 2 > dwMemLength) return false;
		WORD patternSize = LittleEndianW(*(WORD *)(lpStream + dwPatternOffset));
		dwPatternOffset += 2;

		if(Patterns.Insert(i, patternSize))
		{
			CString s;
			s.Format(TEXT("Allocating patterns failed starting from pattern %u"), i);
			MessageBox(NULL, s, TEXT("OpenMPT PSM import"), MB_ICONERROR);
			break;
		}

		// Read pattern.
		MODCOMMAND *row_data;
		row_data = Patterns[i];

		for(int nRow = 0; nRow < patternSize; nRow++)
		{
			if(dwPatternOffset + 2 > dwMemLength) return false;
			WORD rowSize = LittleEndianW(*(WORD *)(lpStream + dwPatternOffset));
			
			DWORD dwRowOffset = dwPatternOffset + 2;

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
					if(bNote == 0xFF)
						bNote = NOTE_NOTECUT;
					else
						bNote = (bNote & 0x0F) + 12 * (bNote >> 4) + 13;
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
					BYTE command = lpStream[dwRowOffset], param = lpStream[dwRowOffset + 1];

					switch(command)
					{
					// Volslides
					case 0x01: // fine volslide up
						command = CMD_VOLUMESLIDE;
						param = (param << 3) | 0x0F;
						break;
					case 0x02: // volslide up
						command = CMD_VOLUMESLIDE;
						param = (param << 3);
						break;
					case 0x03: // fine volslide down
						command = CMD_VOLUMESLIDE;
						param = 0xF0 | (param >> 1);
						break;
					case 0x04: // volslide down
						command = CMD_VOLUMESLIDE;
						param >>= 1;
						break;

					// Portamento
					case 0x0B: // fine portamento up
						command = CMD_PORTAMENTOUP;
						param = 0xF0 | (param >> 1);
						break;
					case 0x0C: // portamento up
						command = CMD_PORTAMENTOUP;
						param >>= 1;
						break;
					case 0x0D: // fine portamento down
						command = CMD_PORTAMENTODOWN;
						param = 0xF0 | (param >> 1);
						break;
					case 0x0E: // portamento down
						command = CMD_PORTAMENTODOWN;
						param >>= 1;
						break;					
					case 0x0F: // tone portamento
						command = CMD_TONEPORTAMENTO;
						param >>= 2;
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
						ASSERT(false);
						//command = CMD_NONE;
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

	}

	if(!sComment.empty())
	{
		m_lpszSongComments = new char[sComment.length() + 1];
		if (m_lpszSongComments)
		{
			memset(m_lpszSongComments, 0, sComment.length() + 1);
			memcpy(m_lpszSongComments, sComment.c_str(), sComment.length());
		}
	}

	return true;

	#undef ASSERT_CAN_READ
}
