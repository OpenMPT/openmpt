/*
 * Purpose: Load OKT (Oktalyzer) modules
 * Authors: Storlek (Original author - http://schismtracker.org/)
 *			Johannes Schultz (OpenMPT Port, tweaks)
 *
 * Thanks to Storlek for allowing me to use this code!
 */

//////////////////////////////////////////////
// Oktalyzer (OKT) module loader            //
//////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

// IFF chunk names
#define OKTCHUNKID_CMOD 0x434D4F44
#define OKTCHUNKID_SAMP 0x53414D50
#define OKTCHUNKID_SPEE 0x53504545
#define OKTCHUNKID_SLEN 0x534C454E
#define OKTCHUNKID_PLEN 0x504C454E
#define OKTCHUNKID_PATT 0x50415454
#define OKTCHUNKID_PBOD 0x50424F44
#define OKTCHUNKID_SBOD 0x53424F44

#pragma pack(1)

struct OKT_IFFCHUNK
{
	uint32 signature;	// IFF chunk name
	uint32 chunksize;	// chunk size without header
};

struct OKT_SAMPLE
{
	char   name[20];
	uint32 length;		// length in bytes
	uint16 loopstart;	// *2 for real value
	uint16 looplen;		// dito
	uint16 volume;		// default volume
	uint16 type;		// 7-/8-bit sample
};

STATIC_ASSERT(sizeof(OKT_SAMPLE) == 32);

#pragma pack()


// just for keeping track of offsets and stuff...
struct OKT_SAMPLEINFO
{
	DWORD start;	// start position of the IFF block
	DWORD length;	// length of the IFF block
};


// Parse the sample header block
void Read_OKT_Samples(const BYTE *lpStream, const DWORD dwMemLength, vector<bool> &sample7bit, CSoundFile *pSndFile)
//------------------------------------------------------------------------------------------------------------------
{
	pSndFile->m_nSamples = min((SAMPLEINDEX)(dwMemLength / 32), MAX_SAMPLES - 1);	// typically 36
	sample7bit.resize(pSndFile->GetNumSamples());

	for(SAMPLEINDEX nSmp = 1; nSmp <= pSndFile->GetNumSamples(); nSmp++)
	{
		MODSAMPLE *pSmp = &pSndFile->Samples[nSmp];
		OKT_SAMPLE oktsmp;
		memcpy(&oktsmp, lpStream + (nSmp - 1) * 32, sizeof(OKT_SAMPLE));

		oktsmp.length = min(BigEndian(oktsmp.length), MAX_SAMPLE_LENGTH);
		oktsmp.loopstart = BigEndianW(oktsmp.loopstart) * 2;
		oktsmp.looplen = BigEndianW(oktsmp.looplen) * 2;
		oktsmp.volume = BigEndianW(oktsmp.volume);
		oktsmp.type = BigEndianW(oktsmp.type);

		MemsetZero(*pSmp);
		strncpy(pSndFile->m_szNames[nSmp], oktsmp.name, 20);
		SpaceToNullStringFixed(pSndFile->m_szNames[nSmp], 20);

		pSmp->nC5Speed = 8287;
		pSmp->nGlobalVol = 64;
		pSmp->nVolume = min(oktsmp.volume, 64) * 4;
		pSmp->nLength = oktsmp.length & ~1;	// round down
		// parse loops
		if (oktsmp.looplen > 2 && ((UINT)oktsmp.loopstart) + ((UINT)oktsmp.looplen) <= pSmp->nLength)
		{
			pSmp->nSustainStart = oktsmp.loopstart;
			pSmp->nSustainEnd = oktsmp.loopstart + oktsmp.looplen;
			if (pSmp->nSustainStart < pSmp->nLength && pSmp->nSustainEnd <= pSmp->nLength)
				pSmp->uFlags |= CHN_SUSTAINLOOP;
			else
				pSmp->nSustainStart = pSmp->nSustainEnd = 0;
		}
		sample7bit[nSmp - 1] = (oktsmp.type == 0 || oktsmp.type == 2) ? true : false;
	}
}


// Parse a pattern block
void Read_OKT_Pattern(const BYTE *lpStream, const DWORD dwMemLength, const PATTERNINDEX nPat, CSoundFile *pSndFile)
//-----------------------------------------------------------------------------------------------------------------
{
	#define ASSERT_CAN_READ(x) \
	if( dwMemPos > dwMemLength || x > dwMemLength - dwMemPos ) return;

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(2);
	ROWINDEX nRows = CLAMP(BigEndianW(*(uint16 *)(lpStream + dwMemPos)), 1, MAX_PATTERN_ROWS);
	dwMemPos += 2;

	if(pSndFile->Patterns.Insert(nPat, nRows))
		return;

	const CHANNELINDEX nChns = pSndFile->GetNumChannels();
	MODCOMMAND *mrow = pSndFile->Patterns[nPat], *m;

	for(ROWINDEX nRow = 0; nRow < nRows; nRow++, mrow += nChns)
	{
		m = mrow;
		for(CHANNELINDEX nChn = 0; nChn < nChns; nChn++, m++)
		{
			ASSERT_CAN_READ(4);
			m->note = lpStream[dwMemPos++];
			m->instr = lpStream[dwMemPos++];
			int8 fxcmd = lpStream[dwMemPos++];
			m->param = lpStream[dwMemPos++];

			if(m->note > 0 && m->note <= 36)
			{
				m->note += 48;
				m->instr++;
			} else
			{
				m->instr = 0;
			}

			switch(fxcmd)
			{
			case 0:	// Nothing
				m->param = 0;
				break;

			case 1: // 1 Portamento Down (Period)
				m->command = CMD_PORTAMENTODOWN;
				m->param &= 0x0F;
				break;
			case 2: // 2 Portamento Up (Period)
				m->command = CMD_PORTAMENTOUP;
				m->param &= 0x0F;
				break;

#if 0
			/* these aren't like Jxx: "down" means to *subtract* the offset from the note.
			For now I'm going to leave these unimplemented. */
			case 10: // A Arpeggio 1 (down, orig, up)
			case 11: // B Arpeggio 2 (orig, up, orig, down)
				if (note->param)
					note->command = CMD_ARPEGGIO;
                    break;
#endif
			// This one is close enough to "standard" arpeggio -- I think!
			case 12: // C Arpeggio 3 (up, up, orig)
				if (m->param)
					m->command = CMD_ARPEGGIO;
				break;

			case 13: // D Slide Down (Notes)
				if (m->param)
				{
					m->command = CMD_NOTESLIDEDOWN;
					m->param = 0x10 | min(0x0F, m->param);
				}
				break;
			case 30: // U Slide Up (Notes)
				if (m->param)
				{
					m->command = CMD_NOTESLIDEUP;
					m->param = 0x10 | min(0x0F, m->param);
				}
				break;
			/* We don't have fine note slide, but this is supposed to happen once
			per row. Sliding every 5 (non-note) ticks kind of works (at least at
			speed 6), but implementing fine slides would of course be better. */
			case 21: // L Slide Down Once (Notes)
				if (m->param)
				{
					m->command = CMD_NOTESLIDEDOWN;
					m->param = 0x50 | min(0x0F, m->param);
				}
				break;
			case 17: // H Slide Up Once (Notes)
				if (m->param)
				{
					m->command = CMD_NOTESLIDEUP;
					m->param = 0x50 | min(0x0F, m->param);
				}
				break;

			case 15: // F Set Filter <>00:ON
				// Not implemented, but let's import it anyway...
				m->command = CMD_MODCMDEX;
				m->param = !!m->param;
				break;

			case 25: // P Pos Jump
				m->command = CMD_POSITIONJUMP;
				break;

			case 27: // R Release sample (apparently not listed in the help!)
				m->Clear();
				m->note = NOTE_KEYOFF;
				break;

			case 28: // S Speed
				m->command = CMD_SPEED; // or tempo?
				break;

			case 31: // V Volume
				m->command = CMD_VOLUMESLIDE;
				switch (m->param >> 4)
				{
				case 4:
					if (m->param != 0x40)
					{
						m->param &= 0x0F; // D0x
						break;
					}
					// 0x40 is set volume -- fall through
				case 0: case 1: case 2: case 3:
					m->volcmd = VOLCMD_VOLUME;
					m->vol = m->param;
					m->command = CMD_NONE;
					m->param = 0;
					break;
				case 5:
					m->param = (m->param & 0x0F) << 4; // Dx0
					break;
				case 6:
					m->param = 0xF0 | min(m->param & 0x0F, 0x0E); // DFx
					break;
				case 7:
					m->param = (min(m->param & 0x0F, 0x0E) << 4) | 0x0F; // DxF
					break;
				default:
					// Junk.
					m->command = m->param = 0;
					break;
				}
				break;

#if 0
			case 24: // O Old Volume (???)
				m->command = CMD_VOLUMESLIDE;
				m->param = 0;
				break;
#endif

			default:
				m->command = m->param = 0;
				//ASSERT(false);
				break;
			}
		}
	}

	#undef ASSERT_CAN_READ
}


bool CSoundFile::ReadOKT(const BYTE *lpStream, const DWORD dwMemLength)
//---------------------------------------------------------------------
{
	#define ASSERT_CAN_READ(x) \
	if( dwMemPos > dwMemLength || x > dwMemLength - dwMemPos ) return false;

	DWORD dwMemPos = 0;

	ASSERT_CAN_READ(8);
	if (memcmp(lpStream, "OKTASONG", 8) != 0)
		return false;
	dwMemPos += 8;

	OKT_IFFCHUNK iffHead;
	// prepare some arrays to store offsets etc.
	vector<DWORD> patternOffsets;
	vector<bool> sample7bit;	// 7-/8-bit sample
	vector<OKT_SAMPLEINFO> samplePos;
	PATTERNINDEX nPatterns = 0;
	ORDERINDEX nOrders = 0;

	MemsetZero(m_szNames);
	m_nChannels = 0;
	m_nSamples = 0;

	// Go through IFF chunks...
	while(dwMemPos < dwMemLength)
	{
		ASSERT_CAN_READ(sizeof(OKT_IFFCHUNK));
		memcpy(&iffHead, lpStream + dwMemPos, sizeof(OKT_IFFCHUNK));
		iffHead.signature = BigEndian(iffHead.signature);
		iffHead.chunksize = BigEndian(iffHead.chunksize);
		dwMemPos += sizeof(OKT_IFFCHUNK);

		switch(iffHead.signature)
		{
		case OKTCHUNKID_CMOD:
			// read that weird channel setup table
			if(m_nChannels > 0)
				break;
			ASSERT_CAN_READ(8);
			for(CHANNELINDEX nChn = 0; nChn < 4; nChn++)
			{
				if(lpStream[dwMemPos + nChn * 2] || lpStream[dwMemPos + nChn * 2 + 1])
				{
					ChnSettings[m_nChannels++].nPan = (((nChn & 3) == 1) || ((nChn & 3) == 2)) ? 0xC0 : 0x40;
				}
				ChnSettings[m_nChannels++].nPan = (((nChn & 3) == 1) || ((nChn & 3) == 2)) ? 0xC0 : 0x40;
			}
			break;

		case OKTCHUNKID_SAMP:
			// convert sample headers
			if(m_nSamples > 0)
				break;
			ASSERT_CAN_READ(iffHead.chunksize);
			Read_OKT_Samples(lpStream + dwMemPos, iffHead.chunksize, sample7bit, this);
			break;

		case OKTCHUNKID_SPEE:
			// read default speed
			{
				ASSERT_CAN_READ(2);
				uint16 defspeed = BigEndianW(*((uint16 *)(lpStream + dwMemPos)));
				m_nDefaultSpeed = CLAMP(defspeed, 1, 255);
			}
			break;

		case OKTCHUNKID_SLEN:
			// number of patterns, we don't need this.
			break;

		case OKTCHUNKID_PLEN:
			// read number of valid orders
			ASSERT_CAN_READ(2);
			nOrders = BigEndianW(*((uint16 *)(lpStream + dwMemPos)));
			break;

		case OKTCHUNKID_PATT:
			// read the orderlist
			ASSERT_CAN_READ(iffHead.chunksize);
			Order.ReadAsByte(lpStream + dwMemPos, min(iffHead.chunksize, MAX_ORDERS), iffHead.chunksize);
			break;

		case OKTCHUNKID_PBOD:
			// don't read patterns for now, as the number of channels might be unknown at this point.
			if(nPatterns < MAX_PATTERNS)
			{
				if(iffHead.chunksize > 0)
					patternOffsets.push_back(dwMemPos);
				nPatterns++;
			}
			break;

		case OKTCHUNKID_SBOD:
			// sample data - same as with patterns, as we need to know the sample format / length
			if(samplePos.size() < MAX_SAMPLES - 1)
			{
				ASSERT_CAN_READ(iffHead.chunksize);
				if(iffHead.chunksize)
				{
					OKT_SAMPLEINFO sinfo;
					sinfo.start = dwMemPos;
					sinfo.length = iffHead.chunksize;
					samplePos.push_back(sinfo);
				}
			}
			break;
		}

		dwMemPos += iffHead.chunksize;
	}

	// If there wasn't even a CMOD chunk, we can't really load this.
	if(m_nChannels == 0)
		return false;

	m_nDefaultTempo = 125;
	m_nDefaultGlobalVolume = 256;
	m_nSamplePreAmp = m_nVSTiVolume = 48;
	m_nType = MOD_TYPE_OKT;
	m_nMinPeriod = 0x71 << 2;
	m_nMaxPeriod = 0x358 << 2;

	// Fix orderlist
	for(ORDERINDEX nOrd = nOrders; nOrd < Order.GetLengthTailTrimmed(); nOrd++)
	{
		Order[nOrd] = Order.GetInvalidPatIndex();
	}

	// Read patterns
	for(PATTERNINDEX nPat = 0; nPat < nPatterns; nPat++)
	{
		if(patternOffsets[nPat] > 0)
			Read_OKT_Pattern(lpStream + patternOffsets[nPat], dwMemLength - patternOffsets[nPat], nPat, this);
		else
			Patterns.Insert(nPat, 64);	// invent empty pattern
	}

	// Read samples
	size_t nFileSmp = 0;
	for(SAMPLEINDEX nSmp = 1; nSmp < m_nSamples; nSmp++)
	{
		if(nFileSmp >= samplePos.size())
			break;

		MODSAMPLE *pSmp = &Samples[nSmp];
		if(pSmp->nLength == 0)
			continue;

		// weird stuff?
		if(pSmp->nLength != samplePos[nFileSmp].length)
		{
			pSmp->nLength = min(pSmp->nLength, samplePos[nFileSmp].length);
		}

		ReadSample(pSmp, RS_PCM8S, (LPCSTR)(lpStream + samplePos[nFileSmp].start), dwMemLength - samplePos[nFileSmp].start);

		// 7-bit to 8-bit hack
		if(sample7bit[nSmp - 1] && pSmp->pSample)
		{
			for(size_t i = 0; i < pSmp->nLength; i++)
				pSmp->pSample[i] = CLAMP(pSmp->pSample[i] * 2, -128, 127);
		}

		nFileSmp++;
	}

	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	return true;

	#undef ASSERT_CAN_READ
}
