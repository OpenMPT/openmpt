/*
 * Purpose: Load RIFF AM and RIFF AMFF modules (Galaxy Sound System).
 * Note:    J2B is a compressed variant of RIFF AM and RIFF AMFF files used in Jazz Jackrabbit 2.
 *          It seems like no other game used the AM(FF) format.
 * Authors: Johannes Schultz (OpenMPT port)
 *          Chris Moeller (foo_dumb - this is almost a complete port of his code, thanks)
 *
 */

#include "stdafx.h"
#include "sndfile.h"
#define ZLIB_WINAPI
#include "../zlib/zlib.h"

#pragma pack(1)

// header for compressed j2b files
struct J2BHEADER
{
	DWORD signature;		// MUSE
	DWORD deadbeaf;			// 0xDEADBEAF (AM) or 0xDEADBABE (AMFF)
	DWORD j2blength;		// complete filesize
	DWORD crc32;			// checksum of the compressed data block
	DWORD packed_length;	// length of the compressed data block
	DWORD unpacked_length;	// length of the decompressed module
};

// am(ff) stuff

struct RIFFCHUNK
{
	DWORD signature;	// "RIFF"
	DWORD chunksize;	// chunk size without header
};

// this header is used for both AM's "INIT" as well as AMFF's "MAIN" chunk
struct AMFFCHUNK_MAIN
{
	char  songname[64];
	BYTE  flags;
	BYTE  channels;
	BYTE  speed;
	BYTE  tempo;
	DWORD unknown;
	BYTE  globalvolume;
};

struct AMFFCHUNK_INSTRUMENT
{
	BYTE unknown;		// 0x00
	BYTE sample;		// sample number
	char name[28];
	char stuff[195];	// lots of NULs?
};

struct AMFFCHUNK_SAMPLE
{
	DWORD signature;	// "SAMP"
	DWORD chunksize;	// header + sample size
	char  name[28];
	BYTE  pan;
	BYTE  volume;
	WORD  flags;
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	DWORD samplerate;
	DWORD reserved1;
	DWORD reserved2;
};

struct AMCHUNK_INSTRUMENT
{
	BYTE  unknown;		// 0x00
	BYTE  sample;		// sample number
	char  name[32];
};

struct AMCHUNK_SAMPLE
{
	DWORD signature;	// "SAMP"
	DWORD chunksize;	// header + sample size
	DWORD headsize;		// header size
	char  name[32];
	WORD  pan;
	WORD  volume;
	WORD  flags;
	WORD  unkown;
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	DWORD samplerate;
};

#pragma pack()

static BYTE riffam_efftrans[26] =
{
	CMD_ARPEGGIO, CMD_PORTAMENTOUP, CMD_PORTAMENTODOWN, CMD_TONEPORTAMENTO,
	CMD_VIBRATO, CMD_TONEPORTAVOL, CMD_VIBRATOVOL, CMD_TREMOLO,
	CMD_PANNING8, CMD_OFFSET, CMD_VOLUMESLIDE, CMD_POSITIONJUMP,
	CMD_VOLUME, CMD_PATTERNBREAK, CMD_MODCMDEX, CMD_TEMPO,
	CMD_GLOBALVOLUME, CMD_GLOBALVOLSLIDE, CMD_KEYOFF, CMD_SETENVPOSITION,
	CMD_CHANNELVOLUME, CMD_CHANNELVOLSLIDE, CMD_PANNINGSLIDE, CMD_RETRIG,
	CMD_TREMOR, CMD_XFINEPORTAUPDOWN,
};

bool CSoundFile::Convert_RIFF_AM_Pattern(PATTERNINDEX nPat, const LPCBYTE lpStream, DWORD dwMemLength, bool bIsAM)
//----------------------------------------------------------------------------------------------------------------
{
	// version false = AMFF, true = AM
	#define ASSERT_CAN_READ(x) \
	if( dwMemPos > dwMemLength || x > dwMemLength - dwMemPos ) return false;

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(1);

	ROWINDEX nRows = lpStream[0] + 1;

	if(Patterns.Insert(nPat, nRows))
		return false;

	dwMemPos++;

	MODCOMMAND *mrow = Patterns[nPat];
	MODCOMMAND *m = mrow;
	ROWINDEX nRow = 0;
	BYTE flags;

	while((nRow < nRows) && (dwMemPos < dwMemLength))
	{
		ASSERT_CAN_READ(1);
		flags = lpStream[dwMemPos];
		dwMemPos++;
		if (flags == 0)
		{
			nRow++;
			m = mrow = Patterns[nPat] + nRow * m_nChannels;
			continue;
		}

		m = mrow + min((flags & 0x1F), m_nChannels - 1);

		if(flags & 0xE0)
		{
			if (flags & 0x80) // effect
			{
				ASSERT_CAN_READ(2);
				m->command = lpStream[dwMemPos + 1];
				m->param = lpStream[dwMemPos];
				dwMemPos += 2;

				if(m->command <= 25)
				{
					// command translation
					m->command = riffam_efftrans[m->command];
					
					// handling special commands
					switch(m->command)
					{
					case CMD_ARPEGGIO:
						if(m->param == 0) m->command = CMD_NONE;
						break;
					case CMD_TONEPORTAVOL:
					case CMD_VIBRATOVOL:
					case CMD_VOLUMESLIDE:
					case CMD_GLOBALVOLSLIDE:
					case CMD_PANNINGSLIDE:
						if (m->param & 0xF0) m->param &= 0xF0;
						break;
					case CMD_PANNING8:
						if(m->param <= 0x80) m->param = min(m->param << 1, 0xFF);
						else if(m->param == 0xA4) {m->command = CMD_S3MCMDEX; m->param = 0x91;}
						break;
					case CMD_PATTERNBREAK:
						m->param = ((m->param >> 4) * 10) + (m->param & 0x0F);
						break;
					case CMD_MODCMDEX:
						MODExx2S3MSxx(m);
						break;
					case CMD_TEMPO:
						if(m->param <= 0x1F) m->command = CMD_SPEED;
						break;
					case CMD_XFINEPORTAUPDOWN:
						switch(m->param & 0xF0)
						{
						case 0x10:
							m->command = CMD_PORTAMENTOUP;
							break;
						case 0x20:
							m->command = CMD_PORTAMENTODOWN;
							break;
						}
						m->param = (m->param & 0x0F) | 0xE0;
						break;
					}
				} else
				{
#ifdef DEBUG
					{
						CHAR s[64];
						wsprintf(s, "J2B: Unknown command: 0x%X, param 0x%X", m->command, m->param);
						Log(s);
					}
#endif
					m->command = CMD_NONE;
				}
			}

			if (flags & 0x40) // note + ins
			{
				ASSERT_CAN_READ(2);
				m->instr = lpStream[dwMemPos];
				m->note = lpStream[dwMemPos + 1];
				if(m->note > NOTE_MAX) m->note = NOTE_NOTECUT;
				dwMemPos += 2;
			}

			if (flags & 0x20) // volume
			{
				ASSERT_CAN_READ(1);
				m->volcmd = VOLCMD_VOLUME;
				if(!bIsAM)
					m->vol = lpStream[dwMemPos];
				else
					m->vol = lpStream[dwMemPos] * 64 / 127;
				dwMemPos++;
			}
		}
	}

	return true;

	#undef ASSERT_CAN_READ
}

bool CSoundFile::ReadAM(const LPCBYTE lpStream, const DWORD dwMemLength)
//------------------------------------------------------------------------
{
	#define ASSERT_CAN_READ(x) \
	if( dwMemPos > dwMemLength || x > dwMemLength - dwMemPos ) return false;
	#define ASSERT_CAN_READ_CHUNK(x) \
	if( dwMemPos > dwChunkEnd || x > dwChunkEnd - dwMemPos ) break;

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(sizeof(RIFFCHUNK));
	RIFFCHUNK *chunkheader = (RIFFCHUNK *)lpStream;

	if(LittleEndian(chunkheader->signature) != 0x46464952 // "RIFF"
		|| LittleEndian(chunkheader->chunksize) != dwMemLength - sizeof(RIFFCHUNK)
		) return false;

	dwMemPos += sizeof(RIFFCHUNK);

	bool bIsAM; // false: AMFF, true: AM

	ASSERT_CAN_READ(4);
	if(LittleEndian(*(DWORD *)(lpStream + dwMemPos)) == 0x46464D41) bIsAM = false; // "AMFF"
	else if(LittleEndian(*(DWORD *)(lpStream + dwMemPos)) == 0x20204D41) bIsAM = true; // "AM  "
	else return false;
	dwMemPos += 4;
	
	// go through all chunks now
	while(dwMemPos < dwMemLength)
	{
		ASSERT_CAN_READ(sizeof(RIFFCHUNK));
		chunkheader = (RIFFCHUNK *)(lpStream + dwMemPos);
		dwMemPos += sizeof(RIFFCHUNK);
		ASSERT_CAN_READ(LittleEndian(chunkheader->chunksize));

		DWORD dwChunkEnd = dwMemPos + LittleEndian(chunkheader->chunksize);

		switch(LittleEndian(chunkheader->signature))
		{
		case 0x4E49414D: // "MAIN" - Song info (AMFF)
		case 0x54494E49: // "INIT" - Song info (AM)
			if((LittleEndian(chunkheader->signature) == 0x4E49414D && !bIsAM) || (LittleEndian(chunkheader->signature) == 0x54494E49 && bIsAM))
			{
				ASSERT_CAN_READ_CHUNK(sizeof(AMFFCHUNK_MAIN));
				AMFFCHUNK_MAIN *mainchunk = (AMFFCHUNK_MAIN *)(lpStream + dwMemPos);
				dwMemPos += sizeof(AMFFCHUNK_MAIN);

				ASSERT_CAN_READ_CHUNK(mainchunk->channels);

				memcpy(m_szNames[0], mainchunk->songname, 32);
				SpaceToNullStringFixed(m_szNames[0], 31);
				m_dwSongFlags = SONG_ITOLDEFFECTS | SONG_ITCOMPATMODE;
				if(!(mainchunk->flags & 0x01)) m_dwSongFlags |= SONG_LINEARSLIDES;
				if(mainchunk->channels < 1) return false;
				m_nChannels = min(mainchunk->channels, MAX_BASECHANNELS);
				m_nDefaultSpeed = mainchunk->speed;
				m_nDefaultTempo = mainchunk->tempo;
				m_nDefaultGlobalVolume = mainchunk->globalvolume << 1;
				m_nSamplePreAmp = m_nVSTiVolume = 48;
				m_nType = MOD_TYPE_IT;

				// It seems like there's no way to differentiate between
				// Muted and Surround channels (they're all 0xA0) - might
				// be a limitation in mod2j2b.
				for(CHANNELINDEX nChn = 0; nChn < m_nChannels; nChn++)
				{
					ChnSettings[nChn].nVolume = 64;
					ChnSettings[nChn].nPan = 128;
					if(bIsAM)
					{
						if(lpStream[dwMemPos + nChn] > 128)
							ChnSettings[nChn].dwFlags = CHN_MUTE;
						else
							ChnSettings[nChn].nPan = lpStream[dwMemPos + nChn] << 1;
					} else
					{
						if(lpStream[dwMemPos + nChn] >= 128)
							ChnSettings[nChn].dwFlags = CHN_MUTE;
						else
							ChnSettings[nChn].nPan = lpStream[dwMemPos + nChn] << 2;
					}
				}
				dwMemPos += mainchunk->channels;
			}
			break;

		case 0x5244524F: // "ORDR" - Order list
			ASSERT_CAN_READ_CHUNK(1);
			Order.ReadAsByte(&lpStream[dwMemPos + 1], lpStream[dwMemPos] + 1, dwChunkEnd - (dwMemPos + 1));
			break;

		case 0x54544150: // "PATT" - Pattern data for one pattern
			ASSERT_CAN_READ_CHUNK(5);
			Convert_RIFF_AM_Pattern(lpStream[dwMemPos], (LPCBYTE)(lpStream + dwMemPos + 5), LittleEndian(*(DWORD *)(lpStream + dwMemPos + 1)), bIsAM);
			break;

		case 0x54534E49: // "INST" - Instrument (only in RIFF AMFF)
			if(!bIsAM)
			{
				ASSERT_CAN_READ_CHUNK(sizeof(AMFFCHUNK_INSTRUMENT));
				AMFFCHUNK_INSTRUMENT *inschunk = (AMFFCHUNK_INSTRUMENT *)(lpStream + dwMemPos);
				dwMemPos += sizeof(AMFFCHUNK_INSTRUMENT);

				SAMPLEINDEX nSmp = inschunk->sample + 1;
				if(nSmp > MAX_SAMPLES)
					break;
				m_nSamples = max(m_nSamples, nSmp);

				memcpy(m_szNames[nSmp], inschunk->name, 28);
				SpaceToNullStringFixed(m_szNames[nSmp], 28);

				ASSERT_CAN_READ_CHUNK(sizeof(AMFFCHUNK_SAMPLE));
				AMFFCHUNK_SAMPLE *smpchunk = (AMFFCHUNK_SAMPLE *)(lpStream + dwMemPos);
				dwMemPos += sizeof(AMFFCHUNK_SAMPLE);

				if(smpchunk->signature != 0x504D4153) break; // SAMP

				memcpy(m_szNames[nSmp], smpchunk->name, 28);
				SpaceToNullStringFixed(m_szNames[nSmp], 28);
				
				Samples[nSmp].nPan = smpchunk->pan << 2;
				Samples[nSmp].nVolume = smpchunk->volume << 2;
				Samples[nSmp].nGlobalVol = 64;
				Samples[nSmp].nLength = LittleEndian(smpchunk->length);
				Samples[nSmp].nLoopStart = LittleEndian(smpchunk->loopstart);
				Samples[nSmp].nLoopEnd = LittleEndian(smpchunk->loopend);
				Samples[nSmp].nC5Speed = LittleEndian(smpchunk->samplerate);

				if(LittleEndianW(smpchunk->flags) & 0x04)
					Samples[nSmp].uFlags |= CHN_16BIT;
				if(LittleEndianW(smpchunk->flags) & 0x08)
					Samples[nSmp].uFlags |= CHN_LOOP;
				if(LittleEndianW(smpchunk->flags) & 0x10)
					Samples[nSmp].uFlags |= CHN_PINGPONGLOOP;
				if(LittleEndianW(smpchunk->flags) & 0x20)
					Samples[nSmp].uFlags |= CHN_PANNING;

				dwMemPos += ReadSample(&Samples[nSmp], (LittleEndianW(smpchunk->flags) & 0x04) ? RS_PCM16S : RS_PCM8S, (LPCSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
			}
			break;

		case 0x46464952: // "RIFF" - Instrument (only in RIFF AM)
			if(bIsAM)
			{
				ASSERT_CAN_READ_CHUNK(4);
				if(LittleEndian(*(DWORD *)(lpStream + dwMemPos)) != 0x20204941) break; // "AI  "
				dwMemPos += 4;

				ASSERT_CAN_READ_CHUNK(sizeof(RIFFCHUNK));
				RIFFCHUNK *instchunk = (RIFFCHUNK *)(lpStream + dwMemPos);
				dwMemPos += sizeof(RIFFCHUNK);
				ASSERT_CAN_READ_CHUNK(LittleEndian(instchunk->chunksize));
				if(LittleEndian(instchunk->signature) != 0x54534E49) break; // "INST"

				ASSERT_CAN_READ_CHUNK(4);
				DWORD dwHeadlen = LittleEndian(*(DWORD *)(lpStream + dwMemPos));
				dwMemPos +=4 ;

				ASSERT_CAN_READ_CHUNK(sizeof(AMCHUNK_INSTRUMENT));
				AMCHUNK_INSTRUMENT *instheadchunk = (AMCHUNK_INSTRUMENT *)(lpStream + dwMemPos);
				dwMemPos += dwHeadlen;

				SAMPLEINDEX nSmp = instheadchunk->sample + 1;
				if(nSmp > MAX_SAMPLES)
					break;
				m_nSamples = max(m_nSamples, nSmp);

				memcpy(m_szNames[nSmp], instheadchunk->name, 32);
				SpaceToNullStringFixed(m_szNames[nSmp], 31);

				ASSERT_CAN_READ_CHUNK(sizeof(RIFFCHUNK));
				instchunk = (RIFFCHUNK *)(lpStream + dwMemPos);
				dwMemPos += sizeof(RIFFCHUNK);
				ASSERT_CAN_READ_CHUNK(LittleEndian(instchunk->chunksize));
				if(LittleEndian(instchunk->signature) != 0x46464952) break; // yet another "RIFF"...

				ASSERT_CAN_READ_CHUNK(4);
				if(LittleEndian(*(DWORD *)(lpStream + dwMemPos)) != 0x20205341) break; // "AS  " (ain't this boring?)
				dwMemPos += 4;

				ASSERT_CAN_READ_CHUNK(sizeof(AMCHUNK_SAMPLE));
				AMCHUNK_SAMPLE *smpchunk = (AMCHUNK_SAMPLE *)(lpStream + dwMemPos);

				if(smpchunk->signature != 0x504D4153) break; // SAMP

				memcpy(m_szNames[nSmp], smpchunk->name, 32);
				SpaceToNullStringFixed(m_szNames[nSmp], 31);

				if(LittleEndianW(smpchunk->pan) > 0x7FFF || LittleEndianW(smpchunk->volume) > 0x7FFF)
					break;

				Samples[nSmp].nPan = LittleEndianW(smpchunk->pan) * 256 / 32767;
				Samples[nSmp].nVolume = LittleEndianW(smpchunk->volume) * 256 / 32767;
				Samples[nSmp].nGlobalVol = 64;
				Samples[nSmp].nLength = LittleEndian(smpchunk->length);
				Samples[nSmp].nLoopStart = LittleEndian(smpchunk->loopstart);
				Samples[nSmp].nLoopEnd = LittleEndian(smpchunk->loopend);
				Samples[nSmp].nC5Speed = LittleEndian(smpchunk->samplerate);

				if(LittleEndianW(smpchunk->flags) & 0x04)
					Samples[nSmp].uFlags |= CHN_16BIT;
				if(LittleEndianW(smpchunk->flags) & 0x08)
					Samples[nSmp].uFlags |= CHN_LOOP;
				if(LittleEndianW(smpchunk->flags) & 0x10)
					Samples[nSmp].uFlags |= CHN_PINGPONGLOOP;
				if(LittleEndianW(smpchunk->flags) & 0x20)
					Samples[nSmp].uFlags |= CHN_PANNING;

				dwMemPos += LittleEndian(smpchunk->headsize) + 12; // doesn't include the 3 first DWORDs
				dwMemPos += ReadSample(&Samples[nSmp], (LittleEndianW(smpchunk->flags) & 0x04) ? RS_PCM16S : RS_PCM8S, (LPCSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
			}
			break;

		}
		dwMemPos = dwChunkEnd;
		// RIFF AM has a padding byte
		if(bIsAM && (LittleEndian(chunkheader->chunksize) & 1)) dwMemPos++;
	}

	return true;

	#undef ASSERT_CAN_READ
	#undef ASSERT_CAN_READ_CHUNK
}

bool CSoundFile::ReadJ2B(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	#define ASSERT_CAN_READ(x) \
	if( dwMemPos > dwMemLength || x > dwMemLength - dwMemPos ) return false;

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(sizeof(J2BHEADER));
	J2BHEADER *header = (J2BHEADER *)lpStream;

	if(LittleEndian(header->signature) != 0x4553554D // "MUSE"
		|| (LittleEndian(header->deadbeaf) != 0xAFBEADDE // 0xDEADBEAF (RIFF AM)
		&& LittleEndian(header->deadbeaf) != 0xBEBAADDE) // 0xDEADBABE (RIFF AMFF)
		|| LittleEndian(header->j2blength) != dwMemLength
		|| LittleEndian(header->packed_length) != dwMemLength - sizeof(J2BHEADER)
		|| LittleEndian(header->packed_length) == 0
		|| LittleEndian(header->crc32) != crc32(0, (lpStream + sizeof(J2BHEADER)), dwMemLength - sizeof(J2BHEADER))
		) return false;

	dwMemPos += sizeof(J2BHEADER);

	// header is valid, now unpack the RIFF AM file using inflate
	DWORD destSize = LittleEndian(header->unpacked_length);
	Bytef *bOutput = new Bytef[destSize];
	int nRetVal = uncompress(bOutput, &destSize, &lpStream[dwMemPos], LittleEndian(header->packed_length));

	bool bResult = false;

	if(destSize == LittleEndian(header->unpacked_length) && nRetVal == Z_OK)
	{
		// Success, now load the RIFF AM(FF) module.
		bResult = ReadAM(bOutput, destSize);
	}
	delete[] bOutput;

	return bResult;

	#undef ASSERT_CAN_READ
}
