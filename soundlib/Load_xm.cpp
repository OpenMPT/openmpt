/*
 * Copied to OpenMPT from libmodplug.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
 *			OpenMPT dev(s)	(miscellaneous modifications)
*/

#include "stdafx.h"
#include "sndfile.h"
#include "../mptrack/version.h"
#include "../mptrack/misc_util.h"

////////////////////////////////////////////////////////
// FastTracker II XM file support

#pragma warning(disable:4244) //conversion from 'type1' to 'type2', possible loss of data

#define str_MBtitle				(GetStrI18N((_TEXT("Saving XM"))))
#define str_tooMuchPatternData	(GetStrI18N((_TEXT("Warning: File format limit was reached. Some pattern data may not get written to file."))))
#define str_pattern				(GetStrI18N((_TEXT("pattern"))))


#pragma pack(1)
typedef struct tagXMFILEHEADER
{
	DWORD size;
	WORD norder;
	WORD restartpos;
	WORD channels;
	WORD patterns;
	WORD instruments;
	WORD flags;
	WORD speed;
	WORD tempo;
	BYTE order[256];
} XMFILEHEADER;


typedef struct tagXMINSTRUMENTHEADER
{
	DWORD size; // size of XMINSTRUMENTHEADER + XMSAMPLEHEADER
	CHAR name[22];
	BYTE type; // should always be 0
	WORD samples;
} XMINSTRUMENTHEADER;


typedef struct tagXMSAMPLEHEADER
{
	DWORD shsize; // size of XMSAMPLESTRUCT
	BYTE snum[96];
	WORD venv[24];
	WORD penv[24];
	BYTE vnum, pnum;
	BYTE vsustain, vloops, vloope, psustain, ploops, ploope;
	BYTE vtype, ptype;
	BYTE vibtype, vibsweep, vibdepth, vibrate;
	WORD volfade;
	WORD res;
	BYTE reserved1[20];
} XMSAMPLEHEADER;

typedef struct tagXMSAMPLESTRUCT
{
	DWORD samplen;
	DWORD loopstart;
	DWORD looplen;
	BYTE vol;
	signed char finetune;
	BYTE type;
	BYTE pan;
	signed char relnote;
	BYTE res;
	char name[22];
} XMSAMPLESTRUCT;
#pragma pack()


bool CSoundFile::ReadXM(const BYTE *lpStream, DWORD dwMemLength)
//--------------------------------------------------------------
{
	XMSAMPLEHEADER xmsh;
	XMSAMPLESTRUCT xmss;
	DWORD dwMemPos, dwHdrSize;
	WORD norders=0, restartpos=0, channels=0, patterns=0, instruments=0;
	WORD xmflags=0;
	BYTE InstUsed[256];
// -> CODE#0006
// -> DESC="misc quantity changes"
//	BYTE channels_used[MAX_CHANNELS];
	BYTE channels_used[MAX_BASECHANNELS];
// -! BEHAVIOUR_CHANGE#0006
	BYTE pattern_map[256];
	BYTE samples_used[(MAX_SAMPLES+7)/8];
	UINT unused_samples;

	bool bMadeWithModPlug = false, bProbablyMadeWithModPlug = false;
	// set this here already because XMs compressed with BoobieSqueezer will exit the function early
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	m_nChannels = 0;
	if ((!lpStream) || (dwMemLength < 0xAA)) return false; // the smallest XM I know is 174 Bytes
	if (_strnicmp((LPCSTR)lpStream, "Extended Module", 15)) return false;

	memcpy(m_szNames[0], lpStream + 17, 20);
	// look for null-terminated song name - that's most likely a tune made with modplug
	for(int i = 0; i < 20; i++)
		if(lpStream[17 + i] == 0) bProbablyMadeWithModPlug = true;

	dwHdrSize = LittleEndian(*((DWORD *)(lpStream+60)));
	norders = LittleEndianW(*((WORD *)(lpStream+64)));
	if ((!norders) || (norders > MAX_ORDERS)) return false;
	restartpos = LittleEndianW(*((WORD *)(lpStream+66)));
	channels = LittleEndianW(*((WORD *)(lpStream+68)));
// -> CODE#0006
// -> DESC="misc quantity changes"
//	if ((!channels) || (channels > 64)) return false;
	if ((!channels) || (channels > MAX_BASECHANNELS)) return false;
// -! BEHAVIOUR_CHANGE#0006

	m_nType = MOD_TYPE_XM;
	m_nMinPeriod = 27;
	m_nMaxPeriod = 54784;
	m_nChannels = channels;
	if (restartpos < norders) m_nRestartPos = restartpos;
	patterns = CLAMP(LittleEndianW(*((WORD *)(lpStream+70))), 0, 256);
	instruments = LittleEndianW(*((WORD *)(lpStream+72)));
	if (instruments >= MAX_INSTRUMENTS) instruments = MAX_INSTRUMENTS-1;
	m_nInstruments = instruments;
	m_nSamples = 0;
	memcpy(&xmflags, lpStream + 74, 2);
	xmflags = LittleEndianW(xmflags);
	if (xmflags & 1) m_dwSongFlags |= SONG_LINEARSLIDES;
	if (xmflags & 0x1000) m_dwSongFlags |= SONG_EXFILTERRANGE;
	m_nDefaultSpeed = CLAMP(LittleEndianW(*((WORD *)(lpStream+76))), 1, 31);
// -> CODE#0016
// -> DESC="default tempo update"
	m_nDefaultTempo = CLAMP(LittleEndianW(*((WORD *)(lpStream+78))), 32, 512);
// -! BEHAVIOUR_CHANGE#0016
	Order.ReadAsByte(lpStream+80, norders, dwMemLength-80);
	memset(InstUsed, 0, sizeof(InstUsed));
	if (patterns > MAX_PATTERNS)
	{
		UINT i, j;
		for (i=0; i<norders; i++)
		{
			if (Order[i] < patterns) InstUsed[Order[i]] = true;
		}
		j = 0;
		for (i=0; i<256; i++)
		{
			if (InstUsed[i]) pattern_map[i] = j++;
		}
		for (i=0; i<256; i++)
		{
			if (!InstUsed[i])
			{
				pattern_map[i] = (j < MAX_PATTERNS) ? j : 0xFE;
				j++;
			}
		}
		for (i=0; i<norders; i++)
		{
			Order[i] = pattern_map[Order[i]];
		}
	} else
	{
		for (UINT i=0; i<256; i++) pattern_map[i] = i;
	}
	memset(InstUsed, 0, sizeof(InstUsed));
	dwMemPos = dwHdrSize + 60;
	if (dwMemPos + 8 >= dwMemLength) return true;
	// Reading patterns
	memset(channels_used, 0, sizeof(channels_used));
	for (UINT ipat=0; ipat<patterns; ipat++)
	{
		UINT ipatmap = pattern_map[ipat];
		DWORD dwSize = 0;
		WORD rows=64, packsize=0;
		dwSize = LittleEndian(*((DWORD *)(lpStream + dwMemPos)));
		/*while ((dwMemPos + dwSize >= dwMemLength) || (dwSize & 0xFFFFFF00))
		{
			if (dwMemPos + 4 >= dwMemLength) break;
			dwMemPos++;
			dwSize = LittleEndian(*((DWORD *)(lpStream+dwMemPos)));
		}*/
		rows = LittleEndianW(*((WORD *)(lpStream + dwMemPos + 5)));
// -> CODE#0008
// -> DESC="#define to set pattern size"
//		if ((!rows) || (rows > 256)) rows = 64;
		if ((!rows) || (rows > MAX_PATTERN_ROWS)) rows = 64;
// -> BEHAVIOUR_CHANGE#0008
		packsize = LittleEndianW(*((WORD *)(lpStream + dwMemPos + 7)));
		if (dwMemPos + dwSize + 4 > dwMemLength) return true;
		dwMemPos += dwSize;
		if (dwMemPos + packsize > dwMemLength) return true;
		MODCOMMAND *p;
		if (ipatmap < MAX_PATTERNS)
		{
			if(Patterns.Insert(ipatmap, rows))
				return true;

			if (!packsize) continue;
			p = Patterns[ipatmap];
		} else p = NULL;
		const BYTE *src = lpStream+dwMemPos;
		UINT j=0;
		for (UINT row=0; row<rows; row++)
		{
			for (UINT chn=0; chn<m_nChannels; chn++)
			{
				if ((p) && (j < packsize))
				{
					BYTE b = src[j++];
					UINT vol = 0;
					if (b & 0x80)
					{
						if (b & 1) p->note = src[j++];
						if (b & 2) p->instr = src[j++];
						if (b & 4) vol = src[j++];
						if (b & 8) p->command = src[j++];
						if (b & 16) p->param = src[j++];
					} else
					{
						p->note = b;
						p->instr = src[j++];
						vol = src[j++];
						p->command = src[j++];
						p->param = src[j++];
					}
					if (p->note == 97) p->note = 0xFF; else
					if ((p->note) && (p->note < 97)) p->note += 12;
					if (p->note) channels_used[chn] = 1;
					if (p->command | p->param) ConvertModCommand(p);
					if (p->instr == 0xff) p->instr = 0;
					if (p->instr) InstUsed[p->instr] = TRUE;
					if ((vol >= 0x10) && (vol <= 0x50))
					{
						p->volcmd = VOLCMD_VOLUME;
						p->vol = vol - 0x10;
					} else
					if (vol >= 0x60)
					{
						UINT v = vol & 0xF0;
						vol &= 0x0F;
						p->vol = vol;
						switch(v)
						{
						// 60-6F: Volume Slide Down
						case 0x60:	p->volcmd = VOLCMD_VOLSLIDEDOWN; break;
						// 70-7F: Volume Slide Up:
						case 0x70:	p->volcmd = VOLCMD_VOLSLIDEUP; break;
						// 80-8F: Fine Volume Slide Down
						case 0x80:	p->volcmd = VOLCMD_FINEVOLDOWN; break;
						// 90-9F: Fine Volume Slide Up
						case 0x90:	p->volcmd = VOLCMD_FINEVOLUP; break;
						// A0-AF: Set Vibrato Speed
						case 0xA0:	p->volcmd = VOLCMD_VIBRATOSPEED; break;
						// B0-BF: Vibrato
						case 0xB0:	p->volcmd = VOLCMD_VIBRATODEPTH; break;
						// C0-CF: Set Panning
						case 0xC0:	p->volcmd = VOLCMD_PANNING; p->vol = (vol << 2) + 2; break;
						// D0-DF: Panning Slide Left
						case 0xD0:	p->volcmd = VOLCMD_PANSLIDELEFT; break;
						// E0-EF: Panning Slide Right
						case 0xE0:	p->volcmd = VOLCMD_PANSLIDERIGHT; break;
						// F0-FF: Tone Portamento
						case 0xF0:	p->volcmd = VOLCMD_TONEPORTAMENTO; break;
						}
					}
					p++;
				} else
				if (j < packsize)
				{
					BYTE b = src[j++];
					if (b & 0x80)
					{
						if (b & 1) j++;
						if (b & 2) j++;
						if (b & 4) j++;
						if (b & 8) j++;
						if (b & 16) j++;
					} else j += 4;
				} else break;
			}
		}
		dwMemPos += packsize;
	}
	// Wrong offset check
	while (dwMemPos + 4 < dwMemLength)
	{
		DWORD d = LittleEndian(*((DWORD *)(lpStream+dwMemPos)));
		if (d < 0x300) break;
		dwMemPos++;
	}
	memset(samples_used, 0, sizeof(samples_used));
	unused_samples = 0;
	// Reading instruments
	for (UINT iIns=1; iIns<=instruments; iIns++)
	{
		XMINSTRUMENTHEADER *pih;
		BYTE flags[32];
		DWORD samplesize[32];
		UINT samplemap[32];
		WORD nsamples;
				
		if (dwMemPos + sizeof(DWORD) >= dwMemLength) return true;
		DWORD ihsize = LittleEndian(*((DWORD *)(lpStream + dwMemPos)));
		if (dwMemPos + ihsize >= dwMemLength) return true;

		pih = (XMINSTRUMENTHEADER *)(lpStream + dwMemPos);
		if (dwMemPos + LittleEndian(pih->size) > dwMemLength) return true;
		if ((Instruments[iIns] = new MODINSTRUMENT) == NULL) continue;
		memset(Instruments[iIns], 0, sizeof(MODINSTRUMENT));
		Instruments[iIns]->pTuning = m_defaultInstrument.pTuning;
		Instruments[iIns]->nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
		Instruments[iIns]->nPluginVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;

		memcpy(Instruments[iIns]->name, pih->name, 22);
		// look for null-terminated instr name - that's most likely a tune made with modplug
		for(int i = 0; i < 22; i++)
			if(pih->name[i] == 0) bProbablyMadeWithModPlug = true;

		if ((nsamples = pih->samples) > 0)
		{
			/* we have samples, so let's read the rest of this instrument
			   the header that is being read here is not the sample header, though,
			   it's rather the instrument settings. */

			if (dwMemPos + ihsize >= dwMemLength)
				return true;

			memset(&xmsh, 0, sizeof(XMSAMPLEHEADER));
			memcpy(&xmsh,
				lpStream + dwMemPos + sizeof(XMINSTRUMENTHEADER),
				min(ihsize - sizeof(XMINSTRUMENTHEADER), sizeof(XMSAMPLEHEADER)));

			xmsh.shsize = LittleEndian(xmsh.shsize);
			if(xmsh.shsize == 0 && bProbablyMadeWithModPlug) bMadeWithModPlug = true;

			for (int i = 0; i < 24; ++i) {
				xmsh.venv[i] = LittleEndianW(xmsh.venv[i]);
				xmsh.penv[i] = LittleEndianW(xmsh.penv[i]);
			}
			xmsh.volfade = LittleEndianW(xmsh.volfade);
			xmsh.res = LittleEndianW(xmsh.res);
			dwMemPos += LittleEndian(pih->size);
		} else
		{
			if (LittleEndian(pih->size)) dwMemPos += LittleEndian(pih->size);
			else dwMemPos += sizeof(XMINSTRUMENTHEADER);
			continue;
		}
		memset(samplemap, 0, sizeof(samplemap));
		if (nsamples > 32) return true;
		UINT newsamples = m_nSamples;
		for (UINT nmap=0; nmap<nsamples; nmap++)
		{
			UINT n = m_nSamples+nmap+1;
			if (n >= MAX_SAMPLES)
			{
				n = m_nSamples;
				while (n > 0)
				{
					if (!Samples[n].pSample)
					{
						for (UINT xmapchk=0; xmapchk < nmap; xmapchk++)
						{
							if (samplemap[xmapchk] == n) goto alreadymapped;
						}
						for (UINT clrs=1; clrs<iIns; clrs++) if (Instruments[clrs])
						{
							MODINSTRUMENT *pks = Instruments[clrs];
							for (UINT ks=0; ks<128; ks++)
							{
								if (pks->Keyboard[ks] == n) pks->Keyboard[ks] = 0;
							}
						}
						break;
					}
				alreadymapped:
					n--;
				}
#ifndef FASTSOUNDLIB
				// Damn! more than 200 samples: look for duplicates
				if (!n)
				{
					if (!unused_samples)
					{
						unused_samples = DetectUnusedSamples(samples_used);
						if (!unused_samples) unused_samples = 0xFFFF;
					}
					if ((unused_samples) && (unused_samples != 0xFFFF))
					{
						for (UINT iext=m_nSamples; iext>=1; iext--) if (0 == (samples_used[iext>>3] & (1<<(iext&7))))
						{
							unused_samples--;
							samples_used[iext>>3] |= (1<<(iext&7));
							DestroySample(iext);
							n = iext;
							for (UINT mapchk=0; mapchk<nmap; mapchk++)
							{
								if (samplemap[mapchk] == n) samplemap[mapchk] = 0;
							}
							for (UINT clrs=1; clrs<iIns; clrs++) if (Instruments[clrs])
							{
								MODINSTRUMENT *pks = Instruments[clrs];
								for (UINT ks=0; ks<128; ks++)
								{
									if (pks->Keyboard[ks] == n) pks->Keyboard[ks] = 0;
								}
							}
							memset(&Samples[n], 0, sizeof(Samples[0]));
							break;
						}
					}
				}
#endif // FASTSOUNDLIB
			}
			if (newsamples < n) newsamples = n;
			samplemap[nmap] = n;
		}
		m_nSamples = newsamples;
		// Reading Volume Envelope
		MODINSTRUMENT *pIns = Instruments[iIns];
		pIns->nMidiProgram = pih->type;
		pIns->nFadeOut = xmsh.volfade;
		pIns->nPan = 128;
		pIns->nPPC = 5*12;
		SetDefaultInstrumentValues(pIns);
		pIns->nPluginVelocityHandling = PLUGIN_VELOCITYHANDLING_CHANNEL;
		pIns->nPluginVolumeHandling = PLUGIN_VOLUMEHANDLING_IGNORE;
		if (xmsh.vtype & 1) pIns->dwFlags |= ENV_VOLUME;
		if (xmsh.vtype & 2) pIns->dwFlags |= ENV_VOLSUSTAIN;
		if (xmsh.vtype & 4) pIns->dwFlags |= ENV_VOLLOOP;
		if (xmsh.ptype & 1) pIns->dwFlags |= ENV_PANNING;
		if (xmsh.ptype & 2) pIns->dwFlags |= ENV_PANSUSTAIN;
		if (xmsh.ptype & 4) pIns->dwFlags |= ENV_PANLOOP;
		if (xmsh.vnum > 12) xmsh.vnum = 12;
		if (xmsh.pnum > 12) xmsh.pnum = 12;
		pIns->VolEnv.nNodes = xmsh.vnum;
		if (!xmsh.vnum) pIns->dwFlags &= ~ENV_VOLUME;
		if (!xmsh.pnum) pIns->dwFlags &= ~ENV_PANNING;
		pIns->PanEnv.nNodes = xmsh.pnum;
		pIns->VolEnv.nSustainStart = pIns->VolEnv.nSustainEnd = xmsh.vsustain;
		if (xmsh.vsustain >= 12) pIns->dwFlags &= ~ENV_VOLSUSTAIN;
		pIns->VolEnv.nLoopStart = xmsh.vloops;
		pIns->VolEnv.nLoopEnd = xmsh.vloope;
		if (pIns->VolEnv.nLoopEnd >= 12) pIns->VolEnv.nLoopEnd = 0;
		if (pIns->VolEnv.nLoopStart >= pIns->VolEnv.nLoopEnd) pIns->dwFlags &= ~ENV_VOLLOOP;
		pIns->PanEnv.nSustainStart = pIns->PanEnv.nSustainEnd = xmsh.psustain;
		if (xmsh.psustain >= 12) pIns->dwFlags &= ~ENV_PANSUSTAIN;
		pIns->PanEnv.nLoopStart = xmsh.ploops;
		pIns->PanEnv.nLoopEnd = xmsh.ploope;
		if (pIns->PanEnv.nLoopEnd >= 12) pIns->PanEnv.nLoopEnd = 0;
		if (pIns->PanEnv.nLoopStart >= pIns->PanEnv.nLoopEnd) pIns->dwFlags &= ~ENV_PANLOOP;
		pIns->nGlobalVol = 64;
		for (UINT ienv=0; ienv<12; ienv++)
		{
			pIns->VolEnv.Ticks[ienv] = (WORD)xmsh.venv[ienv*2];
			pIns->VolEnv.Values[ienv] = (BYTE)xmsh.venv[ienv*2+1];
			pIns->PanEnv.Ticks[ienv] = (WORD)xmsh.penv[ienv*2];
			pIns->PanEnv.Values[ienv] = (BYTE)xmsh.penv[ienv*2+1];
			if (ienv)
			{
				if (pIns->VolEnv.Ticks[ienv] < pIns->VolEnv.Ticks[ienv-1])
				{
					pIns->VolEnv.Ticks[ienv] &= 0xFF;
					pIns->VolEnv.Ticks[ienv] += pIns->VolEnv.Ticks[ienv-1] & 0xFF00;
					if (pIns->VolEnv.Ticks[ienv] < pIns->VolEnv.Ticks[ienv-1]) pIns->VolEnv.Ticks[ienv] += 0x100;
				}
				if (pIns->PanEnv.Ticks[ienv] < pIns->PanEnv.Ticks[ienv-1])
				{
					pIns->PanEnv.Ticks[ienv] &= 0xFF;
					pIns->PanEnv.Ticks[ienv] += pIns->PanEnv.Ticks[ienv-1] & 0xFF00;
					if (pIns->PanEnv.Ticks[ienv] < pIns->PanEnv.Ticks[ienv-1]) pIns->PanEnv.Ticks[ienv] += 0x100;
				}
			}
		}
		for (UINT j=0; j<96; j++)
		{
			pIns->NoteMap[j+12] = j+1+12;
			if (xmsh.snum[j] < nsamples)
				pIns->Keyboard[j+12] = samplemap[xmsh.snum[j]];
		}
		// Reading samples
		for (UINT ins=0; ins<nsamples; ins++)
		{
			if ((dwMemPos + sizeof(xmss) > dwMemLength)
			 || (dwMemPos + xmsh.shsize > dwMemLength)) return true;
			memcpy(&xmss, lpStream+dwMemPos, sizeof(xmss));
			xmss.samplen = LittleEndian(xmss.samplen);
			xmss.loopstart = LittleEndian(xmss.loopstart);
			xmss.looplen = LittleEndian(xmss.looplen);
			dwMemPos += xmsh.shsize;
			flags[ins] = (xmss.type & 0x10) ? RS_PCM16D : RS_PCM8D;
			if (xmss.type & 0x20) flags[ins] = (xmss.type & 0x10) ? RS_STPCM16D : RS_STPCM8D;
			samplesize[ins] = xmss.samplen;
			if (!samplemap[ins]) continue;
			if (xmss.type & 0x10)
			{
				xmss.looplen >>= 1;
				xmss.loopstart >>= 1;
				xmss.samplen >>= 1;
			}
			if (xmss.type & 0x20)
			{
				xmss.looplen >>= 1;
				xmss.loopstart >>= 1;
				xmss.samplen >>= 1;
			}
			if (xmss.samplen > MAX_SAMPLE_LENGTH) xmss.samplen = MAX_SAMPLE_LENGTH;
			if (xmss.loopstart >= xmss.samplen) xmss.type &= ~3;
			xmss.looplen += xmss.loopstart;
			if (xmss.looplen > xmss.samplen) xmss.looplen = xmss.samplen;
			if (!xmss.looplen) xmss.type &= ~3;
			UINT imapsmp = samplemap[ins];
			memcpy(m_szNames[imapsmp], xmss.name, 22);
			m_szNames[imapsmp][22] = 0;
			MODSAMPLE *pSmp = &Samples[imapsmp];
			pSmp->nLength = (xmss.samplen > MAX_SAMPLE_LENGTH) ? MAX_SAMPLE_LENGTH : xmss.samplen;
			pSmp->nLoopStart = xmss.loopstart;
			pSmp->nLoopEnd = xmss.looplen;
			if (pSmp->nLoopEnd > pSmp->nLength) pSmp->nLoopEnd = pSmp->nLength;
			if (pSmp->nLoopStart >= pSmp->nLoopEnd)
			{
				pSmp->nLoopStart = pSmp->nLoopEnd = 0;
			}
			if (xmss.type & 3) pSmp->uFlags |= CHN_LOOP;
			if (xmss.type & 2) pSmp->uFlags |= CHN_PINGPONGLOOP;
			pSmp->nVolume = xmss.vol << 2;
			if (pSmp->nVolume > 256) pSmp->nVolume = 256;
			pSmp->nGlobalVol = 64;
			if ((xmss.res == 0xAD) && (!(xmss.type & 0x30)))
			{
				flags[ins] = RS_ADPCM4;
				samplesize[ins] = (samplesize[ins]+1)/2 + 16;
			}
			pSmp->nFineTune = xmss.finetune;
			pSmp->RelativeTone = (int)xmss.relnote;
			pSmp->nPan = xmss.pan;
			pSmp->uFlags |= CHN_PANNING;
			pSmp->nVibType = xmsh.vibtype;
			pSmp->nVibSweep = xmsh.vibsweep;
			pSmp->nVibDepth = xmsh.vibdepth;
			pSmp->nVibRate = xmsh.vibrate;
			memcpy(pSmp->filename, xmss.name, 22);
			pSmp->filename[21] = 0;
			// look for null-terminated sample name - that's most likely a tune made with modplug
			for(int i = 0; i < 22; i++)
				if(xmss.name[i] == 0) bProbablyMadeWithModPlug = true;
		}
#if 0
		if ((xmsh.reserved2 > nsamples) && (xmsh.reserved2 <= 16))
		{
			dwMemPos += (((UINT)xmsh.reserved2) - nsamples) * xmsh.shsize;
		}
#endif
		for (UINT ismpd=0; ismpd<nsamples; ismpd++)
		{
			if ((samplemap[ismpd]) && (samplesize[ismpd]) && (dwMemPos < dwMemLength))
			{
				ReadSample(&Samples[samplemap[ismpd]], flags[ismpd], (LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
			}
			dwMemPos += samplesize[ismpd];
			if (dwMemPos >= dwMemLength) break;
		}
	}
	// Read song comments: "TEXT"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream+dwMemPos))) == 0x74786574))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len < 16384))
		{
			m_lpszSongComments = new char[len+1];
			if (m_lpszSongComments)
			{
				memcpy(m_lpszSongComments, lpStream+dwMemPos, len);
				m_lpszSongComments[len] = 0;
			}
			dwMemPos += len;
		}
		bMadeWithModPlug = true;
	}
	// Read midi config: "MIDI"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream+dwMemPos))) == 0x4944494D))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if (len == sizeof(MODMIDICFG))
		{
			memcpy(&m_MidiCfg, lpStream+dwMemPos, len);
			m_dwSongFlags |= SONG_EMBEDMIDICFG;
			dwMemPos += len;	//rewbs.fix36946
		}
		bMadeWithModPlug = true;
	}
	// Read pattern names: "PNAM"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream+dwMemPos))) == 0x4d414e50))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= MAX_PATTERNS*MAX_PATTERNNAME) && (len >= MAX_PATTERNNAME))
		{
			m_lpszPatternNames = new char[len];
			if (m_lpszPatternNames)
			{
				m_nPatternNames = len / MAX_PATTERNNAME;
				memcpy(m_lpszPatternNames, lpStream+dwMemPos, len);
			}
			dwMemPos += len;
		}
		bMadeWithModPlug = true;
	}
	// Read channel names: "CNAM"
	if ((dwMemPos + 8 < dwMemLength) && (LittleEndian(*((DWORD *)(lpStream+dwMemPos))) == 0x4d414e43))
	{
		UINT len = *((DWORD *)(lpStream+dwMemPos+4));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= MAX_BASECHANNELS*MAX_CHANNELNAME))
		{
			UINT n = len / MAX_CHANNELNAME;
			for (UINT i=0; i<n; i++)
			{
				memcpy(ChnSettings[i].szName, (lpStream+dwMemPos+i*MAX_CHANNELNAME), MAX_CHANNELNAME);
				ChnSettings[i].szName[MAX_CHANNELNAME-1] = 0;
			}
			dwMemPos += len;
		}
		bMadeWithModPlug = true;
	}
	// Read mix plugins information
	if (dwMemPos + 8 < dwMemLength) 
	{
		dwMemPos += LoadMixPlugins(lpStream+dwMemPos, dwMemLength-dwMemPos);
		bMadeWithModPlug = true;
	}

	// Check various things to find out whether this has been made with MPT.
	// Null chars in names -> most likely made with MPT, which disguises as FT2
	if (!memcmp((LPCSTR)lpStream + 0x26, "FastTracker v2.00   ", 20) && bProbablyMadeWithModPlug) bMadeWithModPlug = true;
	if (!memcmp((LPCSTR)lpStream + 0x26, "FastTracker v 2.00  ", 20))
	{
		// Early MPT 1.0 alpha/beta versions
		bMadeWithModPlug = true;
		m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 00, 00, 00);
	}

	if (!memcmp((LPCSTR)lpStream + 0x26, "OpenMPT ", 8))
	{
		//bMadeWithModPlug = true; // Don't set it - it's also used by compatibility export
		CHAR sVersion[13];
		memcpy(sVersion, lpStream + 0x26 + 8, 12);
		sVersion[12] = 0;
		m_dwLastSavedWithVersion = MptVersion::ToNum(sVersion);
	}

	if(bMadeWithModPlug)
	{
		SetModFlag(MSF_COMPATIBLE_PLAY, false);
		if(!m_dwLastSavedWithVersion) m_dwLastSavedWithVersion = MAKE_VERSION_NUMERIC(1, 16, 00, 00);
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Leave if no extra instrument settings are available (end of file reached)
	if(dwMemPos >= dwMemLength) return true;

	bool bInterpretOpenMPTMade = false; // specific for OpenMPT 1.17+ (bMadeWithModPlug is also for MPT 1.16)
	LPCBYTE ptr = lpStream + dwMemPos;
	if(m_nInstruments)
		ptr = LoadExtendedInstrumentProperties(ptr, lpStream+dwMemLength, &bInterpretOpenMPTMade);

	LoadExtendedSongProperties(GetType(), ptr, lpStream, dwMemLength, &bInterpretOpenMPTMade);

	if(bInterpretOpenMPTMade && m_dwLastSavedWithVersion < MAKE_VERSION_NUMERIC(1, 17, 2, 50))
		SetModFlag(MSF_MIDICC_BUGEMULATION, true);

	return true;
}


#ifndef MODPLUG_NO_FILESAVE

bool CSoundFile::SaveXM(LPCSTR lpszFileName, UINT nPacking, const bool bCompatibilityExport)
//------------------------------------------------------------------------------------------
{
	#define ASSERT_CAN_WRITE(x) \
	if(len > s.size() - x) /*Buffer running out? Make it larger.*/ \
		s.resize(s.size() + 10*1024, 0); \
	\
	if(len > uint16_max - (UINT)x) /*Reaching the limits of file format?*/ \
	{ \
		CString str; str.Format("%s (%s %u)", str_tooMuchPatternData, str_pattern, i); \
		MessageBox(0, str, str_MBtitle, MB_ICONWARNING); \
		break; \
	}

	//BYTE s[64*64*5];
	vector<BYTE> s(64*64*5, 0);
	XMFILEHEADER header;
	XMINSTRUMENTHEADER xmih;
	XMSAMPLEHEADER xmsh;
	XMSAMPLESTRUCT xmss;
	BYTE xmph[9];
	FILE *f;
	int i;
	BOOL bAddChannel = false; // avoid odd channel count for FT2 compatibility 

	if(bCompatibilityExport) nPacking = false;

	if ((!m_nChannels) || (!lpszFileName)) return false;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	fwrite("Extended Module: ", 17, 1, f);
	fwrite(m_szNames[0], 20, 1, f);
	s[0] = 0x1A;
	lstrcpy((LPSTR)&s[1], (nPacking) ? "MOD Plugin packed   " : "OpenMPT " MPT_VERSION_STR "  ");
	s[21] = 0x04; // Version number
	s[22] = 0x01; // XM Format v1.04
	fwrite(&s[0], 23, 1, f);
	// Writing song header
	memset(&header, 0, sizeof(header));
	header.size = sizeof(XMFILEHEADER);
	header.norder = 0;
	header.restartpos = m_nRestartPos;

	header.channels = (m_nChannels + 1) & 0xFFFE; // avoid odd channel count for FT2 compatibility
	if(m_nChannels & 1) bAddChannel = true;
	if(bCompatibilityExport && header.channels > 32)
		header.channels = 32;

	header.patterns = 0;
  /*for (i=0; i<MAX_ORDERS; i++) {
		header.norder++;
		if ((Order[i] >= header.patterns) && (Order[i] < MAX_PATTERNS)) header.patterns = Order[i]+1;
	}*/
	if(Order.GetCount() < MAX_ORDERS)
		Order.resize(MAX_ORDERS, Order.GetInvalidPatIndex());

	for (i=MAX_ORDERS-1; i>=0; i--) { // walk backwards over orderlist
		if ((Order[i]!=0xFF) && (header.norder==0)) {
			header.norder=i+1;	//find last used order
		}
		if ((Order[i] >= header.patterns) && (Order[i] < MAX_PATTERNS)) {
			header.patterns = Order[i]+1;	//find last pattern
		}
	}

	header.instruments = m_nInstruments;
	if (!header.instruments) header.instruments = m_nSamples;
	header.flags = (m_dwSongFlags & SONG_LINEARSLIDES) ? 0x01 : 0x00;
	if (m_dwSongFlags & SONG_EXFILTERRANGE) header.flags |= 0x1000;
	if(bCompatibilityExport)
	{	
		header.tempo = CLAMP(m_nDefaultTempo, 32, 255);
	}
	else
	{
		header.tempo = m_nDefaultTempo;
	}
	header.speed = CLAMP(m_nDefaultSpeed, 1, 31);
	Order.WriteToByteArray(header.order, header.norder, 256);

	fwrite(&header, 1, sizeof(header), f);
	// Writing patterns
	for (i=0; i<header.patterns; i++) if (Patterns[i])
	{
		MODCOMMAND *p = Patterns[i];
		UINT len = 0;

		memset(&xmph, 0, sizeof(xmph));
		xmph[0] = 9;
		xmph[5] = (BYTE)(PatternSize[i] & 0xFF);
		xmph[6] = (BYTE)(PatternSize[i] >> 8);
		for (UINT j = m_nChannels * PatternSize[i]; j > 0; j--, p++)
		{
			// Don't write more than 32 channels
			if(bCompatibilityExport && m_nChannels - ((j - 1) % m_nChannels) > 32) continue;

			UINT note = p->note;
			UINT param = ModSaveCommand(p, true, bCompatibilityExport);
			UINT command = param >> 8;
			param &= 0xFF;
			if (note >= 0xFE) note = 97; else
			if ((note <= 12) || (note > 96+12)) note = 0; else
			note -= 12;
			UINT vol = 0;
			if (p->volcmd)
			{
				UINT volcmd = p->volcmd;
				switch(volcmd)
				{
				case VOLCMD_VOLUME:			vol = 0x10 + p->vol; break;
				case VOLCMD_VOLSLIDEDOWN:	vol = 0x60 + (p->vol & 0x0F); break;
				case VOLCMD_VOLSLIDEUP:		vol = 0x70 + (p->vol & 0x0F); break;
				case VOLCMD_FINEVOLDOWN:	vol = 0x80 + (p->vol & 0x0F); break;
				case VOLCMD_FINEVOLUP:		vol = 0x90 + (p->vol & 0x0F); break;
				case VOLCMD_VIBRATOSPEED:	vol = 0xA0 + (p->vol & 0x0F); break;
				case VOLCMD_VIBRATODEPTH:	vol = 0xB0 + (p->vol & 0x0F); break;
				case VOLCMD_PANNING:		vol = 0xC0 + (p->vol >> 2); if (vol > 0xCF) vol = 0xCF; break;
				case VOLCMD_PANSLIDELEFT:	vol = 0xD0 + (p->vol & 0x0F); break;
				case VOLCMD_PANSLIDERIGHT:	vol = 0xE0 + (p->vol & 0x0F); break;
				case VOLCMD_TONEPORTAMENTO:	vol = 0xF0 + (p->vol & 0x0F); break;
				}
			}
			if ((note) && (p->instr) && (vol > 0x0F) && (command) && (param))
			{
				s[len++] = note;
				s[len++] = p->instr;
				s[len++] = vol;
				s[len++] = command;
				s[len++] = param;
			} else
			{
				BYTE b = 0x80;
				if (note) b |= 0x01;
				if (p->instr) b |= 0x02;
				if (vol >= 0x10) b |= 0x04;
				if (command) b |= 0x08;
				if (param) b |= 0x10;
				s[len++] = b;
				if (b & 1) s[len++] = note;
				if (b & 2) s[len++] = p->instr;
				if (b & 4) s[len++] = vol;
				if (b & 8) s[len++] = command;
				if (b & 16) s[len++] = param;
			}

			if(bAddChannel && (j % m_nChannels == 1 || m_nChannels == 1))
			{
				ASSERT_CAN_WRITE(1);
				s[len++] = 0x80;
			}

			ASSERT_CAN_WRITE(5);

		}
		xmph[7] = (BYTE)(len & 0xFF);
		xmph[8] = (BYTE)(len >> 8);
		fwrite(xmph, 1, 9, f);
		fwrite(&s[0], 1, len, f);
	} else
	{
		memset(&xmph, 0, sizeof(xmph));
		xmph[0] = 9;
		xmph[5] = (BYTE)(PatternSize[i] & 0xFF);
		xmph[6] = (BYTE)(PatternSize[i] >> 8);
		fwrite(xmph, 1, 9, f);
	}
	// Writing instruments
	for (i=1; i<=header.instruments; i++)
	{
		MODSAMPLE *pSmp;
		WORD smptable[32];
		BYTE flags[32];

		memset(&smptable, 0, sizeof(smptable));
		memset(&xmih, 0, sizeof(xmih));
		memset(&xmsh, 0, sizeof(xmsh));
		xmih.size = sizeof(xmih) + sizeof(xmsh);
		memcpy(xmih.name, m_szNames[i], 22);
		xmih.type = 0;
		xmih.samples = 0;
		if (m_nInstruments)
		{
			MODINSTRUMENT *pIns = Instruments[i];
			if (pIns)
			{
				memcpy(xmih.name, pIns->name, 22);
				xmih.type = pIns->nMidiProgram;
				xmsh.volfade = min(pIns->nFadeOut, 0xFFF); // FFF is maximum in FT2
				xmsh.vnum = (BYTE)pIns->VolEnv.nNodes;
				xmsh.pnum = (BYTE)pIns->PanEnv.nNodes;
				if (xmsh.vnum > 12) xmsh.vnum = 12;
				if (xmsh.pnum > 12) xmsh.pnum = 12;
				for (UINT ienv=0; ienv<12; ienv++)
				{
					xmsh.venv[ienv*2] = pIns->VolEnv.Ticks[ienv];
					xmsh.venv[ienv*2+1] = pIns->VolEnv.Values[ienv];
					xmsh.penv[ienv*2] = pIns->PanEnv.Ticks[ienv];
					xmsh.penv[ienv*2+1] = pIns->PanEnv.Values[ienv];
				}
				if (pIns->dwFlags & ENV_VOLUME) xmsh.vtype |= 1;
				if (pIns->dwFlags & ENV_VOLSUSTAIN) xmsh.vtype |= 2;
				if (pIns->dwFlags & ENV_VOLLOOP) xmsh.vtype |= 4;
				if (pIns->dwFlags & ENV_PANNING) xmsh.ptype |= 1;
				if (pIns->dwFlags & ENV_PANSUSTAIN) xmsh.ptype |= 2;
				if (pIns->dwFlags & ENV_PANLOOP) xmsh.ptype |= 4;
				xmsh.vsustain = (BYTE)pIns->VolEnv.nSustainStart;
				xmsh.vloops = (BYTE)pIns->VolEnv.nLoopStart;
				xmsh.vloope = (BYTE)pIns->VolEnv.nLoopEnd;
				xmsh.psustain = (BYTE)pIns->PanEnv.nSustainStart;
				xmsh.ploops = (BYTE)pIns->PanEnv.nLoopStart;
				xmsh.ploope = (BYTE)pIns->PanEnv.nLoopEnd;
				for (UINT j=0; j<96; j++) if (pIns->Keyboard[j+12]) // for all notes
				{
					UINT k;
					UINT sample = pIns->Keyboard[j+12];

					// Check to see if sample mapped to this note is already accounted for in this instrument
					for (k=0; k<xmih.samples; k++)	{
						if (smptable[k] == sample) {
							break;
						}
					}
				    
					if (k == xmih.samples) { //we got to the end of the loop: sample unnaccounted for.
						smptable[xmih.samples++] = sample; //record in instrument's sample table
					}
					
					if (xmih.samples >= 32) break;
					xmsh.snum[j] = k;	//record sample table offset in instrument's note map
				}
//				xmsh.reserved2 = xmih.samples;
			}
		} else
		{
			xmih.samples = 1;
//			xmsh.reserved2 = 1;
			smptable[0] = i;
		}
		xmsh.shsize = (xmih.samples) ? 40 : 0;
		fwrite(&xmih, 1, sizeof(xmih), f);
		if (smptable[0])
		{
			MODSAMPLE *pvib = &Samples[smptable[0]];
			xmsh.vibtype = pvib->nVibType;
			xmsh.vibsweep = pvib->nVibSweep;
			xmsh.vibdepth = pvib->nVibDepth;
			xmsh.vibrate = pvib->nVibRate;
		}
		fwrite(&xmsh, 1, xmih.size - sizeof(xmih), f);
		if (!xmih.samples) continue;
		for (UINT ins=0; ins<xmih.samples; ins++)
		{
			memset(&xmss, 0, sizeof(xmss));
			if (smptable[ins]) memcpy(xmss.name, m_szNames[smptable[ins]], 22);
			pSmp = &Samples[smptable[ins]];
			xmss.samplen = pSmp->nLength;
			xmss.loopstart = pSmp->nLoopStart;
			xmss.looplen = pSmp->nLoopEnd - pSmp->nLoopStart;
			xmss.vol = pSmp->nVolume / 4;
			xmss.finetune = (char)pSmp->nFineTune;
			xmss.type = 0;
			if (pSmp->uFlags & CHN_LOOP) xmss.type = (pSmp->uFlags & CHN_PINGPONGLOOP) ? 2 : 1;
			flags[ins] = RS_PCM8D;
#ifndef NO_PACKING
			if (nPacking)
			{
				if ((!(pSmp->uFlags & (CHN_16BIT|CHN_STEREO)))
				 && (CanPackSample(pSmp->pSample, pSmp->nLength, nPacking)))
				{
					flags[ins] = RS_ADPCM4;
					xmss.res = 0xAD;
				}
			} else
#endif
			{
				if (pSmp->uFlags & CHN_16BIT)
				{
					flags[ins] = RS_PCM16D;
					xmss.type |= 0x10;
					xmss.looplen *= 2;
					xmss.loopstart *= 2;
					xmss.samplen *= 2;
				}
				if (pSmp->uFlags & CHN_STEREO && !bCompatibilityExport)
				{
					flags[ins] = (pSmp->uFlags & CHN_16BIT) ? RS_STPCM16D : RS_STPCM8D;
					xmss.type |= 0x20;
					xmss.looplen *= 2;
					xmss.loopstart *= 2;
					xmss.samplen *= 2;
				}
			}
			xmss.pan = 255;
			if (pSmp->nPan < 256) xmss.pan = (BYTE)pSmp->nPan;
			xmss.relnote = (signed char)pSmp->RelativeTone;
			fwrite(&xmss, 1, xmsh.shsize, f);
		}
		for (UINT ismpd=0; ismpd<xmih.samples; ismpd++)
		{
			pSmp = &Samples[smptable[ismpd]];
			if (pSmp->pSample)
			{
#ifndef NO_PACKING
				if ((flags[ismpd] == RS_ADPCM4) && (xmih.samples>1)) CanPackSample(pSmp->pSample, pSmp->nLength, nPacking);
#endif // NO_PACKING
				WriteSample(f, pSmp, flags[ismpd]);
			}
		}
	}

	if(!bCompatibilityExport)
	{
		// Writing song comments
		if ((m_lpszSongComments) && (m_lpszSongComments[0]))
		{
			DWORD d = 0x74786574;
			fwrite(&d, 1, 4, f);
			d = strlen(m_lpszSongComments);
			fwrite(&d, 1, 4, f);
			fwrite(m_lpszSongComments, 1, d, f);
		}
		// Writing midi cfg
		if (m_dwSongFlags & SONG_EMBEDMIDICFG)
		{
			DWORD d = 0x4944494D;
			fwrite(&d, 1, 4, f);
			d = sizeof(MODMIDICFG);
			fwrite(&d, 1, 4, f);
			fwrite(&m_MidiCfg, 1, sizeof(MODMIDICFG), f);
		}
		// Writing Pattern Names
		if ((m_nPatternNames) && (m_lpszPatternNames))
		{
			DWORD dwLen = m_nPatternNames * MAX_PATTERNNAME;
			while ((dwLen >= MAX_PATTERNNAME) && (!m_lpszPatternNames[dwLen-MAX_PATTERNNAME])) dwLen -= MAX_PATTERNNAME;
			if (dwLen >= MAX_PATTERNNAME)
			{
				DWORD d = 0x4d414e50;
				fwrite(&d, 1, 4, f);
				fwrite(&dwLen, 1, 4, f);
				fwrite(m_lpszPatternNames, 1, dwLen, f);
			}
		}
		// Writing Channel Names
		{
			UINT nChnNames = 0;
			for (UINT inam=0; inam<m_nChannels; inam++)
			{
				if (ChnSettings[inam].szName[0]) nChnNames = inam+1;
			}
			// Do it!
			if (nChnNames)
			{
				DWORD dwLen = nChnNames * MAX_CHANNELNAME;
				DWORD d = 0x4d414e43;
				fwrite(&d, 1, 4, f);
				fwrite(&dwLen, 1, 4, f);
				for (UINT inam=0; inam<nChnNames; inam++)
				{
					fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
				}
			}
		}

		//Save hacked-on extra info
		SaveMixPlugins(f);
		SaveExtendedInstrumentProperties(Instruments, header.instruments, f);
		SaveExtendedSongProperties(f);
	}

	fclose(f);
	return true;
}

#endif // MODPLUG_NO_FILESAVE
