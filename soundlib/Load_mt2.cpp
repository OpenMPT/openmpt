/*
 * Load_mt2.cpp
 * ------------
 * Purpose: MT2 (MadTracker 2) module loader
 * Notes  : Plugins are currently not imported. Better rewrite this crap.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"

//#define MT2DEBUG

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

typedef struct PACKED _MT2FILEHEADER
{
	DWORD dwMT20;	// 0x3032544D "MT20"
	DWORD dwSpecial;
	WORD wVersion;
	char szTrackerName[32];	// "MadTracker 2.0"
	char szSongName[64];
	WORD nOrders;
	WORD wRestart;
	WORD wPatterns;
	WORD wChannels;
	WORD wSamplesPerTick;
	BYTE bTicksPerLine;
	BYTE bLinesPerBeat;
	DWORD fulFlags; // b0=packed patterns
	WORD wInstruments;
	WORD wSamples;
	BYTE Orders[256];
} MT2FILEHEADER;

STATIC_ASSERT(sizeof(MT2FILEHEADER) == 382);

typedef struct PACKED _MT2PATTERN
{
	WORD wLines;
	DWORD wDataLen;
} MT2PATTERN;

STATIC_ASSERT(sizeof(MT2PATTERN) == 6);

typedef struct PACKED _MT2COMMAND
{
	BYTE note;	// 0=nothing, 97=note off
	BYTE instr;
	BYTE vol;
	BYTE pan;
	BYTE fxcmd;
	BYTE fxparam1;
	BYTE fxparam2;
} MT2COMMAND;

STATIC_ASSERT(sizeof(MT2COMMAND) == 7);

typedef struct PACKED _MT2DRUMSDATA
{
	WORD wDrumPatterns;
	WORD wDrumSamples[8];
	BYTE DrumPatternOrder[256];
} MT2DRUMSDATA;

STATIC_ASSERT(sizeof(MT2DRUMSDATA) == 274);

typedef struct PACKED _MT2AUTOMATION
{
	DWORD dwFlags;
	DWORD dwEffectId;
	DWORD nEnvPoints;
} MT2AUTOMATION;

STATIC_ASSERT(sizeof(MT2AUTOMATION) == 12);

typedef struct PACKED _MT2INSTRUMENT
{
	char szName[32];
	DWORD dwDataLen;
	WORD wSamples;
	BYTE GroupsMapping[96];
	BYTE bVibType;
	BYTE bVibSweep;
	BYTE bVibDepth;
	BYTE bVibRate;
	WORD wFadeOut;
	WORD wNNA;
	WORD wInstrFlags;
	WORD wEnvFlags1;
	WORD wEnvFlags2;
} MT2INSTRUMENT;

STATIC_ASSERT(sizeof(MT2INSTRUMENT) == 148);

typedef struct PACKED _MT2ENVELOPE
{
	BYTE nFlags;
	BYTE nPoints;
	BYTE nSustainPos;
	BYTE nLoopStart;
	BYTE nLoopEnd;
	BYTE bReserved[3];
	BYTE EnvData[64];
} MT2ENVELOPE;

STATIC_ASSERT(sizeof(MT2ENVELOPE) == 72);

typedef struct PACKED _MT2SYNTH
{
	BYTE nSynthId;
	BYTE nFxId;
	WORD wCutOff;
	BYTE nResonance;
	BYTE nAttack;
	BYTE nDecay;
	BYTE bReserved[25];
} MT2SYNTH;

STATIC_ASSERT(sizeof(MT2SYNTH) == 32);

typedef struct PACKED _MT2SAMPLE
{
	char szName[32];
	DWORD dwDataLen;
	DWORD dwLength;
	DWORD dwFrequency;
	BYTE nQuality;
	BYTE nChannels;
	BYTE nFlags;	// 8 = no interpolation
	BYTE nLoop;
	DWORD dwLoopStart;
	DWORD dwLoopEnd;
	WORD wVolume;
	BYTE nPan;
	BYTE nBaseNote;
	WORD wSamplesPerBeat;
} MT2SAMPLE;

STATIC_ASSERT(sizeof(MT2SAMPLE) == 62);

typedef struct PACKED _MT2GROUP
{
	uint8 nSmpNo;
	uint8 nVolume;	// 0-128
	int8  nFinePitch;
	int8  Reserved[5];
} MT2GROUP;

STATIC_ASSERT(sizeof(MT2GROUP) == 8);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


static void ConvertMT2Command(CSoundFile *that, ModCommand *m, MT2COMMAND *p)
//---------------------------------------------------------------------------
{
	// Note
	m->note = NOTE_NONE;
	if (p->note) m->note = (p->note > 96) ? 0xFF : p->note+12;
	// Instrument
	m->instr = p->instr;
	// Volume Column
	if ((p->vol >= 0x10) && (p->vol <= 0x90))
	{
		m->volcmd = VOLCMD_VOLUME;
		m->vol = (p->vol - 0x10) >> 1;
	} else
	if ((p->vol >= 0xA0) && (p->vol <= 0xAF))
	{
		m->volcmd = VOLCMD_VOLSLIDEDOWN;
		m->vol = (p->vol & 0x0f);
	} else
	if ((p->vol >= 0xB0) && (p->vol <= 0xBF))
	{
		m->volcmd = VOLCMD_VOLSLIDEUP;
		m->vol = (p->vol & 0x0f);
	} else
	if ((p->vol >= 0xC0) && (p->vol <= 0xCF))
	{
		m->volcmd = VOLCMD_FINEVOLDOWN;
		m->vol = (p->vol & 0x0f);
	} else
	if ((p->vol >= 0xD0) && (p->vol <= 0xDF))
	{
		m->volcmd = VOLCMD_FINEVOLUP;
		m->vol = (p->vol & 0x0f);
	} else
	{
		m->volcmd = 0;
		m->vol = 0;
	}
	// Effects
	m->command = 0;
	m->param = 0;
	if ((p->fxcmd) || (p->fxparam1) || (p->fxparam2))
	{
		if (!p->fxcmd)
		{
			m->command = p->fxparam2;
			m->param = p->fxparam1;
			that->ConvertModCommand(*m);
#ifdef MODPLUG_TRACKER
			m->Convert(MOD_TYPE_XM, MOD_TYPE_IT);
#endif // MODPLUG_TRACKER
		} else
		{
			// TODO: MT2 Effects
		}
	}
}


bool CSoundFile::ReadMT2(LPCBYTE lpStream, DWORD dwMemLength, ModLoadingFlags loadFlags)
//--------------------------------------------------------------------------------------
{
	MT2FILEHEADER *pfh = (MT2FILEHEADER *)lpStream;
	DWORD dwMemPos, dwDrumDataPos, dwExtraDataPos;
	UINT nDrumDataLen, nExtraDataLen;
	MT2DRUMSDATA *pdd;
	MT2INSTRUMENT *InstrMap[255];
	MT2SAMPLE *SampleMap[256];

	if ((!lpStream) || (dwMemLength < sizeof(MT2FILEHEADER))
	 || (pfh->dwMT20 != 0x3032544D)
	 || (pfh->wVersion < 0x0200) || (pfh->wVersion >= 0x0300)
	 || (pfh->wChannels < 1) || (pfh->wChannels > MAX_BASECHANNELS))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}
	pdd = NULL;

	InitializeGlobals();
	InitializeChannels();
	mpt::String::Read<mpt::String::maybeNullTerminated>(madeWithTracker, pfh->szTrackerName);
	m_nType = MOD_TYPE_MT2;
	m_nChannels = pfh->wChannels;
	m_nRestartPos = pfh->wRestart;
	m_nDefaultSpeed = pfh->bTicksPerLine;
	m_nDefaultTempo = 125;
	m_SongFlags = (SONG_LINEARSLIDES | SONG_ITCOMPATGXX | SONG_EXFILTERRANGE);
	m_nDefaultRowsPerBeat = pfh->bLinesPerBeat;
	m_nDefaultRowsPerMeasure = m_nDefaultRowsPerBeat * 4;
	if ((pfh->wSamplesPerTick > 100) && (pfh->wSamplesPerTick < 5000))
	{
		m_nDefaultTempo = 110250 / pfh->wSamplesPerTick;
	}
	Order.resize(pfh->nOrders, Order.GetInvalidPatIndex());
	for (UINT iOrd=0; iOrd < pfh->nOrders; iOrd++)
	{
		Order[iOrd] = (PATTERNINDEX)pfh->Orders[iOrd];
	}

	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, pfh->szSongName);

	dwMemPos = sizeof(MT2FILEHEADER);
	nDrumDataLen = *(WORD *)(lpStream + dwMemPos);
	dwDrumDataPos = dwMemPos + 2;
	if (nDrumDataLen >= 2) pdd = (MT2DRUMSDATA *)(lpStream+dwDrumDataPos);
	dwMemPos += 2 + nDrumDataLen;
#ifdef MT2DEBUG
	Log("MT2 v%03X: \"%s\" (flags=%04X)\n", pfh->wVersion, songName.c_str(), pfh->fulFlags);
	Log("%d Channels, %d Patterns, %d Instruments, %d Samples\n", pfh->wChannels, pfh->wPatterns, pfh->wInstruments, pfh->wSamples);
	Log("Drum Data: %d bytes @%04X\n", nDrumDataLen, dwDrumDataPos);
#endif
	if (dwMemPos >= dwMemLength-12) return true;
	if (!*(DWORD *)(lpStream+dwMemPos)) dwMemPos += 4;
	if (!*(DWORD *)(lpStream+dwMemPos)) dwMemPos += 4;
	nExtraDataLen = *(DWORD *)(lpStream+dwMemPos);
	dwExtraDataPos = dwMemPos + 4;
	dwMemPos += 4;
#ifdef MT2DEBUG
	Log("Extra Data: %d bytes @%04X\n", nExtraDataLen, dwExtraDataPos);
#endif
	if (dwMemPos + nExtraDataLen >= dwMemLength) return true;
	while (dwMemPos+8 < dwExtraDataPos + nExtraDataLen)
	{
		DWORD dwId = *(DWORD *)(lpStream+dwMemPos);
		DWORD dwLen = *(DWORD *)(lpStream+dwMemPos+4);
		dwMemPos += 8;
		if (dwMemPos + dwLen > dwMemLength) return true;
#ifdef MT2DEBUG
		CHAR s[5];
		memcpy(s, &dwId, 4);
		s[4] = 0;
		Log("pos=0x%04X: %s: %d bytes\n", dwMemPos-8, s, dwLen);
#endif
		switch(dwId)
		{
		// MSG
		case MAGIC4LE('M','S','G','\0'):
			if (dwLen > 3)
			{
				DWORD nTxtLen = dwLen;
				if (nTxtLen > 32000) nTxtLen = 32000;
				songMessage.Read(lpStream + dwMemPos + 1, nTxtLen - 1, SongMessage::leCRLF);
			}
			break;
		case MAGIC4LE('T','R','K','S'):
			if (dwLen >= 2)
			{
				m_nSamplePreAmp = LittleEndianW(*(uint16 *)(lpStream+dwMemPos)) >> 9;
				dwMemPos += 2;
			}
			for(CHANNELINDEX c = 0; c < GetNumChannels(); c++)
			{
				ChnSettings[c].Reset();
				if(dwMemPos + 1030 < dwMemLength)
				{
					ChnSettings[c].nVolume = LittleEndianW(*(uint16 *)(lpStream+dwMemPos)) >> 9;
					LimitMax(ChnSettings[c].nVolume, uint16(64));
					dwMemPos += 1030;
				}
			}
			break;
		// SUM -> author name (or "Unregistered")
		// TMAP
		}
		dwMemPos += dwLen;
	}
	// Load Patterns
	dwMemPos = dwExtraDataPos + nExtraDataLen;
	for (PATTERNINDEX iPat=0; iPat < pfh->wPatterns; iPat++) if (dwMemPos < dwMemLength - 6)
	{
		MT2PATTERN *pmp = (MT2PATTERN *)(lpStream+dwMemPos);
		UINT wDataLen = (pmp->wDataLen + 1) & ~1;
		dwMemPos += 6;
		if (dwMemPos + wDataLen > dwMemLength) break;
		UINT nLines = pmp->wLines;
		if ((iPat < MAX_PATTERNS) && (nLines > 0) && (nLines <= MAX_PATTERN_ROWS) && (loadFlags & loadPatternData))
		{
	#ifdef MT2DEBUG
			Log("Pattern #%d @%04X: %d lines, %d bytes\n", iPat, dwMemPos-6, nLines, pmp->wDataLen);
	#endif
			Patterns.Insert(iPat, nLines);
			if (!Patterns[iPat]) return true;
			ModCommand *m = Patterns[iPat];
			UINT len = wDataLen;
			if (pfh->fulFlags & 1) // Packed Patterns
			{
				BYTE *p = (BYTE *)(lpStream+dwMemPos);
				UINT pos = 0, row=0, ch=0;
				while (pos < len)
				{
					MT2COMMAND cmd;
					UINT infobyte = p[pos++];
					UINT rptcount = 0;
					if (infobyte == 0xff)
					{
						rptcount = p[pos++];
						infobyte = p[pos++];
				#if 0
						Log("(%d.%d) FF(%02X).%02X\n", row, ch, rptcount, infobyte);
					} else
					{
						Log("(%d.%d) %02X\n", row, ch, infobyte);
				#endif
					}
					if (infobyte & 0x7f)
					{
						UINT patpos = row*m_nChannels+ch;
						cmd.note = cmd.instr = cmd.vol = cmd.pan = cmd.fxcmd = cmd.fxparam1 = cmd.fxparam2 = 0;
						if (infobyte & 1) cmd.note = p[pos++];
						if (infobyte & 2) cmd.instr = p[pos++];
						if (infobyte & 4) cmd.vol = p[pos++];
						if (infobyte & 8) cmd.pan = p[pos++];
						if (infobyte & 16) cmd.fxcmd = p[pos++];
						if (infobyte & 32) cmd.fxparam1 = p[pos++];
						if (infobyte & 64) cmd.fxparam2 = p[pos++];
					#ifdef MT2DEBUG
						if (cmd.fxcmd)
						{
							Log("(%d.%d) MT2 FX=%02X.%02X.%02X\n", row, ch, cmd.fxcmd, cmd.fxparam1, cmd.fxparam2);
						}
					#endif
						ConvertMT2Command(this, &m[patpos], &cmd);
						const ModCommand &orig = m[patpos];
						const ROWINDEX fillRows = std::min(rptcount, nLines - (row + 1));
						for(ROWINDEX r = 0; r < fillRows; r++)
						{
							patpos += GetNumChannels();
							m[patpos] = orig;
						}
					}
					row += rptcount+1;
					while (row >= nLines) { row-=nLines; ch++; }
					if (ch >= m_nChannels) break;
				}
			} else
			{
				MT2COMMAND *p = (MT2COMMAND *)(lpStream+dwMemPos);
				UINT n = 0;
				while ((len > sizeof(MT2COMMAND)) && (n < m_nChannels*nLines))
				{
					ConvertMT2Command(this, m, p);
					len -= sizeof(MT2COMMAND);
					n++;
					p++;
					m++;
				}
			}
		}
		dwMemPos += wDataLen;
	}
	// Skip Drum Patterns
	if (pdd)
	{
	#ifdef MT2DEBUG
		Log("%d Drum Patterns at offset 0x%08X\n", pdd->wDrumPatterns, dwMemPos);
	#endif
		for (UINT iDrm=0; iDrm<pdd->wDrumPatterns; iDrm++)
		{
			if (dwMemPos > dwMemLength-2) return true;
			UINT nLines = *(WORD *)(lpStream+dwMemPos);
		#ifdef MT2DEBUG
			if (nLines != 64) Log("Drum Pattern %d: %d Lines @%04X\n", iDrm, nLines, dwMemPos);
		#endif
			dwMemPos += 2 + nLines * 32;
		}
	}
	// Automation
	if (pfh->fulFlags & 2)
	{
	#ifdef MT2DEBUG
		Log("Automation at offset 0x%08X\n", dwMemPos);
	#endif
		UINT nAutoCount = m_nChannels;
		if (pfh->fulFlags & 0x10) nAutoCount++; // Master Automation
		if ((pfh->fulFlags & 0x08) && (pdd)) nAutoCount += 8; // Drums Automation
		nAutoCount *= pfh->wPatterns;
		for (UINT iAuto=0; iAuto<nAutoCount; iAuto++)
		{
			if (dwMemPos+12 >= dwMemLength) return true;
			MT2AUTOMATION *pma = (MT2AUTOMATION *)(lpStream+dwMemPos);
			dwMemPos += (pfh->wVersion <= 0x201) ? 4 : 8;
			for (UINT iEnv=0; iEnv<14; iEnv++)
			{
				if (pma->dwFlags & (1 << iEnv))
				{
				#ifdef MT2DEBUG
					UINT nPoints = *(DWORD *)(lpStream+dwMemPos);
					Log("  Env[%d/%d] %04X @%04X: %d points\n", iAuto, nAutoCount, 1 << iEnv, dwMemPos-8, nPoints);
				#endif
					dwMemPos += 260;
				}
			}
		}
	}
	// Load Instruments
#ifdef MT2DEBUG
	Log("Loading instruments at offset 0x%08X\n", dwMemPos);
#endif
	MemsetZero(InstrMap);
	m_nInstruments = (pfh->wInstruments < MAX_INSTRUMENTS) ? pfh->wInstruments : MAX_INSTRUMENTS-1;
	for(INSTRUMENTINDEX iIns = 1; iIns <= 255; iIns++)
	{
		if (dwMemPos+36 > dwMemLength) return true;
		MT2INSTRUMENT *pmi = (MT2INSTRUMENT *)(lpStream+dwMemPos);
		ModInstrument *pIns = NULL;
		if (iIns <= m_nInstruments)
		{
			pIns = AllocateInstrument(iIns);
			if(pIns != nullptr)
			{
				mpt::String::Read<mpt::String::maybeNullTerminated>(pIns->name, pmi->szName);
			}
		}
	#ifdef MT2DEBUG
		if (iIns <= pfh->wInstruments) Log("  Instrument #%d at offset %04X: %d bytes\n", iIns, dwMemPos, pmi->dwDataLen);
	#endif
		if (((int)pmi->dwDataLen > 0) && (dwMemPos <= dwMemLength - 40) && (pmi->dwDataLen <= dwMemLength - (dwMemPos + 40)))
		{
			InstrMap[iIns-1] = pmi;
			if (pIns)
			{
				pIns->nFadeOut = pmi->wFadeOut;
				pIns->nNNA = pmi->wNNA & 3;
				pIns->nDCT = (pmi->wNNA>>8) & 3;
				pIns->nDNA = (pmi->wNNA>>12) & 3;
				MT2ENVELOPE *pehdr[4];
				WORD *pedata[4];

				if (pfh->wVersion <= 0x201)
				{
					DWORD dwEnvPos = dwMemPos + sizeof(MT2INSTRUMENT) - 4;
					pehdr[0] = (MT2ENVELOPE *)(lpStream+dwEnvPos);
					pehdr[1] = (MT2ENVELOPE *)(lpStream+dwEnvPos+8);
					pehdr[2] = pehdr[3] = NULL;
					pedata[0] = (WORD *)(lpStream+dwEnvPos+16);
					pedata[1] = (WORD *)(lpStream+dwEnvPos+16+64);
					pedata[2] = pedata[3] = NULL;
				} else
				{
					DWORD dwEnvPos = dwMemPos + sizeof(MT2INSTRUMENT);
					for (UINT i=0; i<4; i++)
					{
						if (pmi->wEnvFlags1 & (1<<i))
						{
							pehdr[i] = (MT2ENVELOPE *)(lpStream+dwEnvPos);
							pedata[i] = (WORD *)pehdr[i]->EnvData;
							dwEnvPos += sizeof(MT2ENVELOPE);
						} else
						{
							pehdr[i] = NULL;
							pedata[i] = NULL;
						}
					}
				}
				// Load envelopes
				for (UINT iEnv=0; iEnv<4; iEnv++) if (pehdr[iEnv])
				{
					MT2ENVELOPE *pme = pehdr[iEnv];
					InstrumentEnvelope *pEnv;
				#ifdef MT2DEBUG
					Log("  Env %d.%d @%04X: %d points\n", iIns, iEnv, (UINT)(((BYTE *)pme)-lpStream), pme->nPoints);
				#endif

					switch(iEnv)
					{
					case 0: // Volume Envelope
						pEnv = &pIns->VolEnv;
						break;
					case 1: // Panning Envelope
						pEnv = &pIns->PanEnv;
						break;
					default: // Pitch/Filter envelope
						pEnv = &pIns->PitchEnv;
						pIns->PitchEnv.dwFlags.set(ENV_FILTER, (pme->nFlags & 1) && iEnv == 3);
					}

					pEnv->dwFlags.set(ENV_ENABLED, (pme->nFlags & 1) != 0);
					pEnv->dwFlags.set(ENV_SUSTAIN, (pme->nFlags & 2) != 0);
					pEnv->dwFlags.set(ENV_LOOP, (pme->nFlags & 4) != 0);
					pEnv->nNodes = (pme->nPoints > 16) ? 16 : pme->nPoints;
					pEnv->nSustainStart = pEnv->nSustainEnd = pme->nSustainPos;
					pEnv->nLoopStart = pme->nLoopStart;
					pEnv->nLoopEnd = pme->nLoopEnd;

					// Envelope data
					if (pedata[iEnv])
					{
						WORD *psrc = pedata[iEnv];
						for (UINT i=0; i<16; i++)
						{
							pEnv->Ticks[i] = psrc[i*2];
							pEnv->Values[i] = (BYTE)psrc[i*2+1];
						}
					}
				}
			}
			dwMemPos += pmi->dwDataLen + 36;
			if (pfh->wVersion > 0x201) dwMemPos += 4; // ?
		} else
		{
			dwMemPos += 36;
		}
	}
#ifdef MT2DEBUG
	Log("Loading samples at offset 0x%08X\n", dwMemPos);
#endif
	memset(SampleMap, 0, sizeof(SampleMap));
	m_nSamples = (pfh->wSamples < MAX_SAMPLES) ? pfh->wSamples : MAX_SAMPLES-1;
	for (UINT iSmp=1; iSmp<=256; iSmp++)
	{
		if (dwMemPos+36 > dwMemLength) return true;
		MT2SAMPLE *pms = (MT2SAMPLE *)(lpStream+dwMemPos);
	#ifdef MT2DEBUG
		if (iSmp <= m_nSamples) Log("  Sample #%d at offset %04X: %d bytes\n", iSmp, dwMemPos, pms->dwDataLen);
	#endif
		if (iSmp < MAX_SAMPLES)
		{
			mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[iSmp], pms->szName);
		}
		if (pms->dwDataLen > 0)
		{
			SampleMap[iSmp-1] = pms;
			if (iSmp < MAX_SAMPLES)
			{
				ModSample *psmp = &Samples[iSmp];
				psmp->Initialize(MOD_TYPE_XM);
				psmp->nGlobalVol = 64;
				psmp->nVolume = (pms->wVolume >> 7);
				psmp->nPan = (pms->nPan == 0x80) ? 128 : (pms->nPan^0x80);
				psmp->nLength = pms->dwLength;
				psmp->nC5Speed = pms->dwFrequency;
				psmp->nLoopStart = pms->dwLoopStart;
				psmp->nLoopEnd = pms->dwLoopEnd;
				if (pms->nQuality == 2) { psmp->uFlags |= CHN_16BIT; psmp->nLength >>= 1; }
				if (pms->nChannels == 2) { psmp->nLength >>= 1; }
				if (pms->nLoop == 1) psmp->uFlags |= CHN_LOOP;
				if (pms->nLoop == 2) psmp->uFlags |= CHN_LOOP|CHN_PINGPONGLOOP;
			}
			dwMemPos += pms->dwDataLen + 36;
		} else
		{
			dwMemPos += 36;
		}
	}
#ifdef MT2DEBUG
	Log("Loading groups at offset 0x%08X\n", dwMemPos);
#endif
	for (UINT iMap=0; iMap<255; iMap++) if (InstrMap[iMap])
	{
		if (dwMemPos+8 > dwMemLength) return true;
		MT2INSTRUMENT *pmi = InstrMap[iMap];
		ModInstrument *pIns = NULL;
		if (iMap<m_nInstruments) pIns = Instruments[iMap+1];
		for (UINT iGrp=0; iGrp<pmi->wSamples; iGrp++)
		{
			if (pIns)
			{
				MT2GROUP *pmg = (MT2GROUP *)(lpStream+dwMemPos);
				for (UINT i=0; i<96; i++)
				{
					if (pmi->GroupsMapping[i] == iGrp)
					{
						UINT nSmp = pmg->nSmpNo+1;
						pIns->Keyboard[i + 12] = (SAMPLEINDEX)nSmp;
						if (nSmp <= m_nSamples && SampleMap[nSmp - 1] != nullptr)
						{
							Samples[nSmp].nVibType = pmi->bVibType;
							Samples[nSmp].nVibSweep = pmi->bVibSweep;
							Samples[nSmp].nVibDepth = pmi->bVibDepth;
							Samples[nSmp].nVibRate = pmi->bVibRate;
							Samples[nSmp].nC5Speed = Util::Round<uint32>(SampleMap[nSmp - 1]->dwFrequency * pow(2, -(SampleMap[nSmp - 1]->nBaseNote - 49 - (pmg->nFinePitch / 128.0f)) / 12.0f));
						}
					}
				}
			}
			dwMemPos += 8;
		}
	}
#ifdef MT2DEBUG
	Log("Loading sample data at offset 0x%08X\n", dwMemPos);
#endif
	if(!(loadFlags & loadSampleData))
	{
		return true;
	}
	for (UINT iData=0; iData<256; iData++) if ((iData < m_nSamples) && (SampleMap[iData]))
	{
		MT2SAMPLE *pms = SampleMap[iData];
		ModSample &sample = Samples[iData+1];
		if(!(pms->nFlags & 5))
		{
			if(sample.nLength > 0)
			{
			#ifdef MT2DEBUG
				Log("  Reading sample #%d at offset 0x%04X (len=%d)\n", iData+1, dwMemPos, psmp->nLength);
			#endif

				FileReader chunk(lpStream + dwMemPos, dwMemLength - dwMemPos);
				dwMemPos += SampleIO(
					(sample.uFlags & CHN_16BIT) ? SampleIO::_16bit : SampleIO::_8bit,
					(pms->nChannels == 2) ? SampleIO::stereoSplit : SampleIO::mono,
					SampleIO::littleEndian,
					SampleIO::MT2)
					.ReadSample(sample, chunk);
			}
		} else
		if (dwMemPos+4 < dwMemLength)
		{
			UINT nNameLen = *(DWORD *)(lpStream+dwMemPos);
			dwMemPos += nNameLen + 16;
		}
		if (dwMemPos+4 >= dwMemLength) break;
	}
	return true;
}