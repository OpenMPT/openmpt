/*
 * load_j2b.cpp
 * ------------
 * Purpose: Load RIFF AM and RIFF AMFF modules (Galaxy Sound System).
 * Notes  : J2B is a compressed variant of RIFF AM and RIFF AMFF files used in Jazz Jackrabbit 2.
 *          It seems like no other game used the AM(FF) format.
 *          RIFF AM is the newer version of the format, generally following the RIFF standard closely.
 * Authors: Johannes Schultz (OpenMPT port, reverse engineering + loader implementation of the instrument format)
 *          Chris Moeller (foo_dumb - this is almost a complete port of his code, thanks)
 *
 */

#include "stdafx.h"
#include "Loaders.h"
#ifndef ZLIB_WINAPI
#define ZLIB_WINAPI
#endif // ZLIB_WINAPI
#include "../zlib/zlib.h"

// 32-Bit J2B header identifiers
#define J2BHEAD_MUSE     0x4553554D
#define J2BHEAD_DEADBEAF 0xAFBEADDE
#define J2BHEAD_DEADBABE 0xBEBAADDE

// 32-Bit chunk identifiers
#define AMCHUNKID_RIFF 0x46464952
#define AMCHUNKID_AMFF 0x46464D41
#define AMCHUNKID_AM__ 0x20204D41
#define AMCHUNKID_MAIN 0x4E49414D
#define AMCHUNKID_INIT 0x54494E49
#define AMCHUNKID_ORDR 0x5244524F
#define AMCHUNKID_PATT 0x54544150
#define AMCHUNKID_INST 0x54534E49
#define AMCHUNKID_SAMP 0x504D4153
#define AMCHUNKID_AI__ 0x20204941
#define AMCHUNKID_AS__ 0x20205341

// Header flags
#define AMHEAD_LINEAR	0x01

// Envelope flags
#define AMENV_ENABLED	0x01
#define AMENV_SUSTAIN	0x02
#define AMENV_LOOP		0x04

// Sample flags
#define AMSMP_16BIT		0x04
#define AMSMP_LOOP		0x08
#define AMSMP_PINGPONG	0x10
#define AMSMP_PANNING	0x20
#define AMSMP_EXISTS	0x80
// some flags are still missing... what is f.e. 0x8000?


#pragma pack(1)

// header for compressed j2b files
struct J2BHEADER
{
	uint32 signature;		// MUSE
	uint32 deadbeaf;		// 0xDEADBEAF (AM) or 0xDEADBABE (AMFF)
	uint32 j2blength;		// complete filesize
	uint32 crc32;			// checksum of the compressed data block
	uint32 packed_length;	// length of the compressed data block
	uint32 unpacked_length;	// length of the decompressed module
};

// am(ff) stuff

struct AMFF_RIFFCHUNK
{
	uint32 signature;	// "RIFF"
	uint32 chunksize;	// chunk size without header
};

// this header is used for both AM's "INIT" as well as AMFF's "MAIN" chunk
struct AMFFCHUNK_MAIN
{
	char   songname[64];
	uint8  flags;
	uint8  channels;
	uint8  speed;
	uint8  tempo;
	uint32 unknown;		// 0x16078035 if original file was MOD, 0xC50100FF for everything else? it's 0xFF00FFFF in Carrotus.j2b (AMFF version)
	uint8  globalvolume;
};

// AMFF instrument envelope point
struct AMFFINST_ENVPOINT
{
	uint16 tick;
	uint8  pointval;	// 0...64
};

// AMFF instrument header
struct AMFFCHUNK_INSTRUMENT
{
	uint8  unknown;				// 0x00
	uint8  index;				// actual instrument number
	char   name[28];
	uint8  numsamples;
	uint8  samplemap[120];
	uint8  autovib_type;
	uint16 autovib_sweep;
	uint16 autovib_depth;
	uint16 autovib_rate;
	uint8  envflags;			// high nibble = pan env flags, low nibble = vol env flags (both nibbles work the same way)
	uint8  envnumpoints;		// high nibble = pan env length, low nibble = vol env length
	uint8  envsustainpoints;	// you guessed it... high nibble = pan env sustain point, low nibble = vol env sustain point
	uint8  envloopstarts;		// i guess you know the pattern now.
	uint8  envloopends;			// same here.
	AMFFINST_ENVPOINT volenv[10];
	AMFFINST_ENVPOINT panenv[10];
	uint16 fadeout;
};

// AMFF sample header
struct AMFFCHUNK_SAMPLE
{
	uint32 signature;	// "SAMP"
	uint32 chunksize;	// header + sample size
	char   name[28];
	uint8  pan;
	uint8  volume;
	uint16 flags;
	uint32 length;
	uint32 loopstart;
	uint32 loopend;
	uint32 samplerate;
	uint32 reserved1;
	uint32 reserved2;
};

// AM instrument envelope point
struct AMINST_ENVPOINT
{
	uint16 tick;
	uint16 pointval;
};

// AM instrument envelope
struct AMINST_ENVELOPE
{
	uint16 flags;
	uint8  numpoints;	// actually, it's num. points - 1, and 0xFF if there is no envelope
	uint8  suslooppoint;
	uint8  loopstart;
	uint8  loopend;
	AMINST_ENVPOINT values[10];
	uint16 fadeout;		// why is this here? it's only needed for the volume envelope...
};

// AM instrument header
struct AMCHUNK_INSTRUMENT
{
	uint8  unknown1;	// 0x00
	uint8  index;		// actual instrument number
	char   name[32];
	uint8  samplemap[128];
	uint8  autovib_type;
	uint16 autovib_sweep;
	uint16 autovib_depth;
	uint16 autovib_rate;
	uint8  unknown2[7];
	AMINST_ENVELOPE volenv;
	AMINST_ENVELOPE pitchenv;
	AMINST_ENVELOPE panenv;
	uint16 numsamples;
};

// AM sample header
struct AMCHUNK_SAMPLE
{
	uint32 signature;	// "SAMP"
	uint32 chunksize;	// header + sample size
	uint32 headsize;	// header size
	char   name[32];
	uint16 pan;
	uint16 volume;
	uint16 flags;
	uint16 unkown;		// 0x0000 / 0x0080?
	uint32 length;
	uint32 loopstart;
	uint32 loopend;
	uint32 samplerate;
};

#pragma pack()


// And here are some nice lookup tables!

static uint8 riffam_efftrans[] =
{
	CMD_ARPEGGIO, CMD_PORTAMENTOUP, CMD_PORTAMENTODOWN, CMD_TONEPORTAMENTO,
	CMD_VIBRATO, CMD_TONEPORTAVOL, CMD_VIBRATOVOL, CMD_TREMOLO,
	CMD_PANNING8, CMD_OFFSET, CMD_VOLUMESLIDE, CMD_POSITIONJUMP,
	CMD_VOLUME, CMD_PATTERNBREAK, CMD_MODCMDEX, CMD_TEMPO,
	CMD_GLOBALVOLUME, CMD_GLOBALVOLSLIDE, CMD_KEYOFF, CMD_SETENVPOSITION,
	CMD_CHANNELVOLUME, CMD_CHANNELVOLSLIDE, CMD_PANNINGSLIDE, CMD_RETRIG,
	CMD_TREMOR, CMD_XFINEPORTAUPDOWN,
};


static uint8 riffam_autovibtrans[] = 
{
	VIB_SINE, VIB_SQUARE, VIB_RAMP_UP, VIB_RAMP_DOWN, VIB_RANDOM,
};


// Convert RIFF AM(FF) pattern data to MPT pattern data.
bool Convert_RIFF_AM_Pattern(const PATTERNINDEX nPat, const LPCBYTE lpStream, const DWORD dwMemLength, const bool bIsAM, CSoundFile *pSndFile)
//--------------------------------------------------------------------------------------------------------------------------------------------
{
	// version false = AMFF, true = AM
	
	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(1);

	ROWINDEX nRows = CLAMP(lpStream[0] + 1, 1, MAX_PATTERN_ROWS);

	if(pSndFile == nullptr || pSndFile->Patterns.Insert(nPat, nRows))
		return false;

	dwMemPos++;

	const CHANNELINDEX nChannels = pSndFile->GetNumChannels();
	if(nChannels == 0)
		return false;

	MODCOMMAND *mrow = pSndFile->Patterns[nPat];
	MODCOMMAND *m = mrow;
	ROWINDEX nRow = 0;

	while((nRow < nRows) && (dwMemPos < dwMemLength))
	{
		ASSERT_CAN_READ(1);
		const uint8 flags = lpStream[dwMemPos];
		dwMemPos++;
		if (flags == 0)
		{
			nRow++;
			m = mrow = pSndFile->Patterns[nPat] + nRow * nChannels;
			continue;
		}

		m = mrow + min((flags & 0x1F), nChannels - 1);

		if(flags & 0xE0)
		{
			if (flags & 0x80) // effect
			{
				ASSERT_CAN_READ(2);
				m->command = lpStream[dwMemPos + 1];
				m->param = lpStream[dwMemPos];
				dwMemPos += 2;

				if(m->command < CountOf(riffam_efftrans))
				{
					// command translation
					m->command = riffam_efftrans[m->command];
					
					// handling special commands
					switch(m->command)
					{
					case CMD_ARPEGGIO:
						if(m->param == 0) m->command = CMD_NONE;
						break;
					case CMD_VOLUME:
						if(m->volcmd == VOLCMD_NONE)
						{
							m->volcmd = VOLCMD_VOLUME;
							m->vol = CLAMP(m->param, 0, 64);
							m->command = CMD_NONE;
							m->param = 0;
						}
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
						CSoundFile::MODExx2S3MSxx(m);
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
#endif // DEBUG
					m->command = CMD_NONE;
				}
			}

			if (flags & 0x40) // note + ins
			{
				ASSERT_CAN_READ(2);
				m->instr = lpStream[dwMemPos];
				m->note = lpStream[dwMemPos + 1];
				if(m->note == 0x80) m->note = NOTE_KEYOFF;
				else if(m->note > 0x80) m->note = NOTE_FADE;	// I guess the support for IT "note fade" notes was not intended in mod2j2b, but hey, it works! :-D
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

}


// Convert envelope data from a RIFF AMFF module (old format) to MPT envelope data.
void Convert_RIFF_AMFF_Envelope(const uint8 flags, const uint8 numpoints, const uint8 sustainpoint, const uint8 loopstart, const uint8 loopend, const AMFFINST_ENVPOINT *envelope, INSTRUMENTENVELOPE *pMPTEnv)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	if(envelope == nullptr || pMPTEnv == nullptr)
		return;

	pMPTEnv->dwFlags = (flags & AMENV_ENABLED) ? ENV_ENABLED : 0;
	if(flags & AMENV_SUSTAIN) pMPTEnv->dwFlags |= ENV_SUSTAIN;
	if(flags & AMENV_LOOP) pMPTEnv->dwFlags |= ENV_LOOP;

	pMPTEnv->nNodes = min(numpoints, 10);	// the buggy mod2j2b converter will actually NOT limit this to 10 points if the envelope is longer.

	pMPTEnv->nSustainStart = pMPTEnv->nSustainEnd = sustainpoint;
	if(pMPTEnv->nSustainStart > pMPTEnv->nNodes)
		pMPTEnv->dwFlags &= ~ENV_SUSTAIN;

	pMPTEnv->nLoopStart = loopstart;
	pMPTEnv->nLoopEnd = loopend;
	if(pMPTEnv->nLoopStart > pMPTEnv->nLoopEnd || pMPTEnv->nLoopStart > pMPTEnv->nNodes)
		pMPTEnv->dwFlags &= ~ENV_LOOP;

	for(size_t i = 0; i < 10; i++)
	{
		pMPTEnv->Ticks[i] = LittleEndianW(envelope[i].tick) >> 4;
		if(i == 0)
			pMPTEnv->Ticks[0] = 0;
		else if(pMPTEnv->Ticks[i] < pMPTEnv->Ticks[i - 1])
			pMPTEnv->Ticks[i] = pMPTEnv->Ticks[i - 1] + 1;

		pMPTEnv->Values[i] = CLAMP(envelope[i].pointval, 0, 0x40);
	}

}


// Convert envelope data from a RIFF AM module (new format) to MPT envelope data.
void Convert_RIFF_AM_Envelope(const AMINST_ENVELOPE *pAMEnv, INSTRUMENTENVELOPE *pMPTEnv, const enmEnvelopeTypes env)
//-------------------------------------------------------------------------------------------------------------------
{
	if(pAMEnv == nullptr || pMPTEnv == nullptr)
		return;

	if(pAMEnv->numpoints == 0xFF || pAMEnv->numpoints == 0x00)
		return;

	uint16 flags = LittleEndianW(pAMEnv->flags);
	pMPTEnv->dwFlags = (flags & AMENV_ENABLED) ? ENV_ENABLED : 0;
	if(flags & AMENV_SUSTAIN) pMPTEnv->dwFlags |= ENV_SUSTAIN;
	if(flags & AMENV_LOOP) pMPTEnv->dwFlags |= ENV_LOOP;

	pMPTEnv->nNodes = min(pAMEnv->numpoints + 1, 10);

	pMPTEnv->nSustainStart = pMPTEnv->nSustainEnd = pAMEnv->suslooppoint;
	if(pMPTEnv->nSustainStart > pMPTEnv->nNodes)
		pMPTEnv->dwFlags &= ~ENV_SUSTAIN;

	pMPTEnv->nLoopStart = pAMEnv->loopstart;
	pMPTEnv->nLoopEnd = pAMEnv->loopend;
	if(pMPTEnv->nLoopStart > pMPTEnv->nLoopEnd || pMPTEnv->nLoopStart > pMPTEnv->nNodes)
		pMPTEnv->dwFlags &= ~ENV_LOOP;

	for(size_t i = 0; i < 10; i++)
	{
		pMPTEnv->Ticks[i] = LittleEndianW(pAMEnv->values[i].tick >> 4);
		if(i == 0)
			pMPTEnv->Ticks[i] = 0;
		else if(pMPTEnv->Ticks[i] < pMPTEnv->Ticks[i - 1])
			pMPTEnv->Ticks[i] = pMPTEnv->Ticks[i - 1] + 1;

		const uint16 val = LittleEndianW(pAMEnv->values[i].pointval);
		switch(env)
		{
		case ENV_VOLUME:	// 0....32767
			pMPTEnv->Values[i] = (BYTE)((val + 1) >> 9);
			break;
		case ENV_PITCH:		// -4096....4096
			pMPTEnv->Values[i] = (BYTE)((((int16)val) + 0x1001) >> 7);
			break;
		case ENV_PANNING:	// -32768...32767
			pMPTEnv->Values[i] = (BYTE)((((int16)val) + 0x8001) >> 10);
			break;
		}
		pMPTEnv->Values[i] = CLAMP(pMPTEnv->Values[i], ENVELOPE_MIN, ENVELOPE_MAX);
	}
}


bool CSoundFile::ReadAM(const LPCBYTE lpStream, const DWORD dwMemLength)
//----------------------------------------------------------------------
{
	#define ASSERT_CAN_READ_CHUNK(x) ASSERT_CAN_READ_PROTOTYPE(dwMemPos, dwChunkEnd, x, break);

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(sizeof(AMFF_RIFFCHUNK));
	AMFF_RIFFCHUNK *chunkheader = (AMFF_RIFFCHUNK *)lpStream;

	if(LittleEndian(chunkheader->signature) != AMCHUNKID_RIFF // "RIFF"
		|| LittleEndian(chunkheader->chunksize) != dwMemLength - sizeof(AMFF_RIFFCHUNK)
		) return false;

	dwMemPos += sizeof(AMFF_RIFFCHUNK);

	bool bIsAM; // false: AMFF, true: AM

	ASSERT_CAN_READ(4);
	if(LittleEndian(*(uint32 *)(lpStream + dwMemPos)) == AMCHUNKID_AMFF) bIsAM = false; // "AMFF"
	else if(LittleEndian(*(uint32 *)(lpStream + dwMemPos)) == AMCHUNKID_AM__) bIsAM = true; // "AM  "
	else return false;
	dwMemPos += 4;
	m_nChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
	
	// go through all chunks now
	while(dwMemPos < dwMemLength)
	{
		ASSERT_CAN_READ(sizeof(AMFF_RIFFCHUNK));
		chunkheader = (AMFF_RIFFCHUNK *)(lpStream + dwMemPos);
		dwMemPos += sizeof(AMFF_RIFFCHUNK);
		ASSERT_CAN_READ(LittleEndian(chunkheader->chunksize));

		const DWORD dwChunkEnd = dwMemPos + LittleEndian(chunkheader->chunksize);

		switch(LittleEndian(chunkheader->signature))
		{
		case AMCHUNKID_MAIN: // "MAIN" - Song info (AMFF)
		case AMCHUNKID_INIT: // "INIT" - Song info (AM)
			if((LittleEndian(chunkheader->signature) == AMCHUNKID_MAIN && !bIsAM) || (LittleEndian(chunkheader->signature) == AMCHUNKID_INIT && bIsAM))
			{
				ASSERT_CAN_READ_CHUNK(sizeof(AMFFCHUNK_MAIN));
				AMFFCHUNK_MAIN *mainchunk = (AMFFCHUNK_MAIN *)(lpStream + dwMemPos);
				dwMemPos += sizeof(AMFFCHUNK_MAIN);

				ASSERT_CAN_READ_CHUNK(mainchunk->channels);

				memcpy(m_szNames[0], mainchunk->songname, 32);
				StringFixer::SpaceToNullStringFixed<31>(m_szNames[0]);
				m_dwSongFlags = SONG_ITOLDEFFECTS | SONG_ITCOMPATGXX;
				if(!(mainchunk->flags & AMHEAD_LINEAR)) m_dwSongFlags |= SONG_LINEARSLIDES;
				if(mainchunk->channels < 1) return false;
				m_nChannels = min(mainchunk->channels, MAX_BASECHANNELS);
				m_nDefaultSpeed = mainchunk->speed;
				m_nDefaultTempo = mainchunk->tempo;
				m_nDefaultGlobalVolume = mainchunk->globalvolume << 1;
				m_nSamplePreAmp = m_nVSTiVolume = 48;
				m_nType = MOD_TYPE_J2B;
				ASSERT(LittleEndian(mainchunk->unknown) == 0xFF0001C5 || LittleEndian(mainchunk->unknown) == 0x35800716 || LittleEndian(mainchunk->unknown) == 0xFF00FFFF);

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

		case AMCHUNKID_ORDR: // "ORDR" - Order list
			ASSERT_CAN_READ_CHUNK(1);
			Order.ReadAsByte(&lpStream[dwMemPos + 1], lpStream[dwMemPos] + 1, dwChunkEnd - (dwMemPos + 1));
			break;

		case AMCHUNKID_PATT: // "PATT" - Pattern data for one pattern
			ASSERT_CAN_READ_CHUNK(5);
			Convert_RIFF_AM_Pattern(lpStream[dwMemPos], (LPCBYTE)(lpStream + dwMemPos + 5), LittleEndian(*(DWORD *)(lpStream + dwMemPos + 1)), bIsAM, this);
			break;

		case AMCHUNKID_INST: // "INST" - Instrument (only in RIFF AMFF)
			if(!bIsAM)
			{
				ASSERT_CAN_READ_CHUNK(sizeof(AMFFCHUNK_INSTRUMENT));
				AMFFCHUNK_INSTRUMENT *instheader = (AMFFCHUNK_INSTRUMENT *)(lpStream + dwMemPos);
				dwMemPos += sizeof(AMFFCHUNK_INSTRUMENT);

				const INSTRUMENTINDEX nIns = instheader->index + 1;
				if(nIns >= MAX_INSTRUMENTS)
					break;

				if(Instruments[nIns] != nullptr)
					delete Instruments[nIns];

				try
				{
					Instruments[nIns] = new MODINSTRUMENT();
				} catch(MPTMemoryException)
				{
					break;
				}
				MODINSTRUMENT *pIns = Instruments[nIns];

				m_nInstruments = max(m_nInstruments, nIns);

				memcpy(pIns->name, instheader->name, 28);
				StringFixer::SpaceToNullStringFixed<28>(pIns->name);

				for(BYTE i = 0; i < 128; i++)
				{
					pIns->Keyboard[i] = instheader->samplemap[i] + m_nSamples + 1;
				}

				pIns->nFadeOut = LittleEndianW(instheader->fadeout) << 5;

				// interleaved envelope data... meh. gotta split it up here and decode it separately.
				// note: mod2j2b is BUGGY and always writes ($original_num_points & 0x0F) in the header,
				// but just has room for 10 envelope points. That means that long (>= 16 points)
				// envelopes are cut off, and envelopes have to be trimmed to 10 points, even if
				// the header claims that they are longer.
				Convert_RIFF_AMFF_Envelope(instheader->envflags & 0x0F, instheader->envnumpoints & 0x0F, instheader->envsustainpoints & 0x0F, instheader->envloopstarts & 0x0F, instheader->envloopends & 0x0F, instheader->volenv, &pIns->VolEnv);
				Convert_RIFF_AMFF_Envelope(instheader->envflags >> 4, instheader->envnumpoints >> 4, instheader->envsustainpoints >> 4, instheader->envloopstarts >> 4, instheader->envloopends >> 4, instheader->panenv, &pIns->PanEnv);

				const size_t nTotalSmps = LittleEndianW(instheader->numsamples);

				// read sample sub-chunks - this is a rather "flat" format compared to RIFF AM and has no nested RIFF chunks.
				for(size_t nSmpCnt = 0; nSmpCnt < nTotalSmps; nSmpCnt++)
				{

					if(m_nSamples + 1 >= MAX_SAMPLES)
						break;

					const SAMPLEINDEX nSmp = ++m_nSamples;

					MemsetZero(Samples[nSmp]);

					ASSERT_CAN_READ_CHUNK(sizeof(AMFFCHUNK_SAMPLE));
					AMFFCHUNK_SAMPLE *smpchunk = (AMFFCHUNK_SAMPLE *)(lpStream + dwMemPos);
					dwMemPos += sizeof(AMFFCHUNK_SAMPLE);

					if(smpchunk->signature != AMCHUNKID_SAMP) break; // SAMP

					memcpy(m_szNames[nSmp], smpchunk->name, 28);
					StringFixer::SpaceToNullStringFixed<28>(m_szNames[nSmp]);

					Samples[nSmp].nPan = smpchunk->pan << 2;
					Samples[nSmp].nVolume = smpchunk->volume << 2;
					Samples[nSmp].nGlobalVol = 64;
					Samples[nSmp].nLength = min(LittleEndian(smpchunk->length), MAX_SAMPLE_LENGTH);
					Samples[nSmp].nLoopStart = LittleEndian(smpchunk->loopstart);
					Samples[nSmp].nLoopEnd = LittleEndian(smpchunk->loopend);
					Samples[nSmp].nC5Speed = LittleEndian(smpchunk->samplerate);

					if(instheader->autovib_type < CountOf(riffam_autovibtrans))
						Samples[nSmp].nVibType = riffam_autovibtrans[instheader->autovib_type];
					Samples[nSmp].nVibSweep = (BYTE)(LittleEndianW(instheader->autovib_sweep));
					Samples[nSmp].nVibRate = (BYTE)(LittleEndianW(instheader->autovib_rate) >> 4);
					Samples[nSmp].nVibDepth = (BYTE)(LittleEndianW(instheader->autovib_depth) >> 2);

					const uint16 flags = LittleEndianW(smpchunk->flags);
					if(flags & AMSMP_16BIT)
						Samples[nSmp].uFlags |= CHN_16BIT;
					if(flags & AMSMP_LOOP)
						Samples[nSmp].uFlags |= CHN_LOOP;
					if(flags & AMSMP_PINGPONG)
						Samples[nSmp].uFlags |= CHN_PINGPONGLOOP;
					if(flags & AMSMP_PANNING)
						Samples[nSmp].uFlags |= CHN_PANNING;

					dwMemPos += ReadSample(&Samples[nSmp], (flags & 0x04) ? RS_PCM16S : RS_PCM8S, (LPCSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
				}
			}
			break;

		case AMCHUNKID_RIFF: // "RIFF" - Instrument (only in RIFF AM)
			if(bIsAM)
			{
				ASSERT_CAN_READ_CHUNK(4);
				if(LittleEndian(*(uint32 *)(lpStream + dwMemPos)) != AMCHUNKID_AI__) break; // "AI  "
				dwMemPos += 4;

				ASSERT_CAN_READ_CHUNK(sizeof(AMFF_RIFFCHUNK));
				const AMFF_RIFFCHUNK *instchunk = (AMFF_RIFFCHUNK *)(lpStream + dwMemPos);
				dwMemPos += sizeof(AMFF_RIFFCHUNK);
				ASSERT_CAN_READ_CHUNK(LittleEndian(instchunk->chunksize));
				if(LittleEndian(instchunk->signature) != AMCHUNKID_INST) break; // "INST"

				ASSERT_CAN_READ_CHUNK(4);
				const DWORD dwHeadlen = LittleEndian(*(uint32 *)(lpStream + dwMemPos));
				ASSERT(dwHeadlen == sizeof(AMCHUNK_INSTRUMENT));
				dwMemPos += 4;

				ASSERT_CAN_READ_CHUNK(sizeof(AMCHUNK_INSTRUMENT));
				const AMCHUNK_INSTRUMENT *instheader = (AMCHUNK_INSTRUMENT *)(lpStream + dwMemPos);
				dwMemPos += sizeof(AMCHUNK_INSTRUMENT);

				const INSTRUMENTINDEX nIns = instheader->index + 1;
				if(nIns >= MAX_INSTRUMENTS)
					break;

				if(Instruments[nIns] != nullptr)
					delete Instruments[nIns];

				try
				{
					Instruments[nIns] = new MODINSTRUMENT();
				} catch(MPTMemoryException)
				{
					break;
				}
				MODINSTRUMENT *pIns = Instruments[nIns];

				m_nInstruments = max(m_nInstruments, nIns);

				memcpy(pIns->name, instheader->name, 32);
				StringFixer::SpaceToNullStringFixed<31>(pIns->name);

				for(BYTE i = 0; i < 128; i++)
				{
					pIns->Keyboard[i] = instheader->samplemap[i] + m_nSamples + 1;
				}

				pIns->nFadeOut = LittleEndianW(instheader->volenv.fadeout) << 5;

				Convert_RIFF_AM_Envelope(&instheader->volenv, &pIns->VolEnv, ENV_VOLUME);
				Convert_RIFF_AM_Envelope(&instheader->pitchenv, &pIns->PitchEnv, ENV_PITCH);
				Convert_RIFF_AM_Envelope(&instheader->panenv, &pIns->PanEnv, ENV_PANNING);

				const size_t nTotalSmps = LittleEndianW(instheader->numsamples);

				if(nTotalSmps == 0)
				{
					MemsetZero(pIns->Keyboard);
				}

				DWORD dwChunkPos;

				// read sample sub-chunks (RIFF nesting ftw)
				for(size_t nSmpCnt = 0; nSmpCnt < nTotalSmps; nSmpCnt++)
				{
					dwChunkPos = dwMemPos;

					ASSERT_CAN_READ_CHUNK(sizeof(AMFF_RIFFCHUNK));
					instchunk = (AMFF_RIFFCHUNK *)(lpStream + dwChunkPos);
					dwChunkPos += sizeof(AMFF_RIFFCHUNK);
					ASSERT_CAN_READ_CHUNK(LittleEndian(instchunk->chunksize));
					if(LittleEndian(instchunk->signature) != AMCHUNKID_RIFF) break;

					ASSERT_CAN_READ_CHUNK(4);
					if(LittleEndian(*(uint32 *)(lpStream + dwChunkPos)) != AMCHUNKID_AS__) break;
					dwChunkPos += 4;

					// Moved this stuff here (was below the next ASSERT_CAN_READ_CHUNK) because of instrument 12 in Carrotus.j2b
					if(m_nSamples + 1 >= MAX_SAMPLES)
						break;
					const SAMPLEINDEX nSmp = ++m_nSamples;

					ASSERT_CAN_READ_CHUNK(sizeof(AMCHUNK_SAMPLE));
					const AMCHUNK_SAMPLE *smpchunk = (AMCHUNK_SAMPLE *)(lpStream + dwChunkPos);

					if(smpchunk->signature != AMCHUNKID_SAMP) break; // SAMP

					MemsetZero(Samples[nSmp]);

					memcpy(m_szNames[nSmp], smpchunk->name, 32);
					StringFixer::SpaceToNullStringFixed<31>(m_szNames[nSmp]);

					if(LittleEndianW(smpchunk->pan) > 0x7FFF || LittleEndianW(smpchunk->volume) > 0x7FFF)
						break;

					Samples[nSmp].nPan = LittleEndianW(smpchunk->pan) * 256 / 32767;
					Samples[nSmp].nVolume = LittleEndianW(smpchunk->volume) * 256 / 32767;
					Samples[nSmp].nGlobalVol = 64;
					Samples[nSmp].nLength = min(LittleEndian(smpchunk->length), MAX_SAMPLE_LENGTH);
					Samples[nSmp].nLoopStart = LittleEndian(smpchunk->loopstart);
					Samples[nSmp].nLoopEnd = LittleEndian(smpchunk->loopend);
					Samples[nSmp].nC5Speed = LittleEndian(smpchunk->samplerate);

					if(instheader->autovib_type < CountOf(riffam_autovibtrans))
						Samples[nSmp].nVibType = riffam_autovibtrans[instheader->autovib_type];
					Samples[nSmp].nVibSweep = (BYTE)(LittleEndianW(instheader->autovib_sweep));
					Samples[nSmp].nVibRate = (BYTE)(LittleEndianW(instheader->autovib_rate) >> 4);
					Samples[nSmp].nVibDepth = (BYTE)(LittleEndianW(instheader->autovib_depth) >> 2);

					const uint16 flags = LittleEndianW(smpchunk->flags);
					if(flags & AMSMP_16BIT)
						Samples[nSmp].uFlags |= CHN_16BIT;
					if(flags & AMSMP_LOOP)
						Samples[nSmp].uFlags |= CHN_LOOP;
					if(flags & AMSMP_PINGPONG)
						Samples[nSmp].uFlags |= CHN_PINGPONGLOOP;
					if(flags & AMSMP_PANNING)
						Samples[nSmp].uFlags |= CHN_PANNING;

					dwChunkPos += LittleEndian(smpchunk->headsize) + 12; // doesn't include the 3 first DWORDs
					dwChunkPos += ReadSample(&Samples[nSmp], (flags & 0x04) ? RS_PCM16S : RS_PCM8S, (LPCSTR)(lpStream + dwChunkPos), dwMemLength - dwChunkPos);

					dwMemPos += LittleEndian(instchunk->chunksize) + sizeof(AMFF_RIFFCHUNK);
					// RIFF AM has a padding byte so that all chunks have an even size.
					if(LittleEndian(instchunk->chunksize) & 1)
						dwMemPos++;

				}
			}
			break;

		}
		dwMemPos = dwChunkEnd;
		// RIFF AM has a padding byte so that all chunks have an even size.
		if(bIsAM && (LittleEndian(chunkheader->chunksize) & 1))
			dwMemPos++;
	}

	return true;

	#undef ASSERT_CAN_READ_CHUNK
}

bool CSoundFile::ReadJ2B(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(sizeof(J2BHEADER));
	J2BHEADER *header = (J2BHEADER *)lpStream;

	if(LittleEndian(header->signature) != J2BHEAD_MUSE // "MUSE"
		|| (LittleEndian(header->deadbeaf) != J2BHEAD_DEADBEAF // 0xDEADBEAF (RIFF AM)
		&& LittleEndian(header->deadbeaf) != J2BHEAD_DEADBABE) // 0xDEADBABE (RIFF AMFF)
		|| LittleEndian(header->j2blength) != dwMemLength
		|| LittleEndian(header->packed_length) != dwMemLength - sizeof(J2BHEADER)
		|| LittleEndian(header->packed_length) == 0
		|| LittleEndian(header->crc32) != crc32(0, (lpStream + sizeof(J2BHEADER)), dwMemLength - sizeof(J2BHEADER))
		) return false;

	dwMemPos += sizeof(J2BHEADER);

	// header is valid, now unpack the RIFF AM file using inflate
	DWORD destSize = LittleEndian(header->unpacked_length);
	Bytef *bOutput = new Bytef[destSize];
	if(bOutput == nullptr)
		return false;
	int nRetVal = uncompress(bOutput, &destSize, &lpStream[dwMemPos], LittleEndian(header->packed_length));

	bool bResult = false;

	if(destSize == LittleEndian(header->unpacked_length) && nRetVal == Z_OK)
	{
		// Success, now load the RIFF AM(FF) module.
		bResult = ReadAM(bOutput, destSize);
	}
	delete[] bOutput;

	return bResult;
}
