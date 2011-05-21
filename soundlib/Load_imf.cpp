/*
 * Purpose: Load IMF (Imago Orpheus) modules
 * Authors: Storlek (Original author - http://schismtracker.org/)
 *			Johannes Schultz (OpenMPT Port, tweaks)
 *
 * Thanks to Storlek for allowing me to use this code!
 */

#include "stdafx.h"
#include "Loaders.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/moddoc.h"
#endif // MODPLUG_TRACKER

#pragma pack(1)

struct IMFCHANNEL
{
	char name[12];	// Channel name (ASCIIZ-String, max 11 chars)
	uint8 chorus;	// Default chorus
	uint8 reverb;	// Default reverb
	uint8 panning;	// Pan positions 00-FF
	uint8 status;	// Channel status: 0 = enabled, 1 = mute, 2 = disabled (ignore effects!)
};

struct IMFHEADER
{
	char title[32];				// Songname (ASCIIZ-String, max. 31 chars)
	uint16 ordnum;				// Number of orders saved
	uint16 patnum;				// Number of patterns saved
	uint16 insnum;				// Number of instruments saved
	uint16 flags;				// Module flags (&1 => linear)
	uint8 unused1[8];
	uint8 tempo;				// Default tempo (Axx, 1..255)
	uint8 bpm;					// Default beats per minute (BPM) (Txx, 32..255)
	uint8 master;				// Default mastervolume (Vxx, 0..64)
	uint8 amp;					// Amplification factor (mixing volume, 4..127)
	uint8 unused2[8];
	char im10[4];				// 'IM10'
	IMFCHANNEL channels[32];	// Channel settings
	uint8 orderlist[256];		// Order list (0xff = +++; blank out anything beyond ordnum)
};

enum
{
	IMF_ENV_VOL = 0,
	IMF_ENV_PAN = 1,
	IMF_ENV_FILTER = 2,
};

struct IMFENVELOPE
{
	uint8 points;		// Number of envelope points
	uint8 sustain;		// Envelope sustain point
	uint8 loop_start;	// Envelope loop start point
	uint8 loop_end;		// Envelope loop end point
	uint8 flags;		// Envelope flags
	uint8 unused[3];
};

struct IMFENVNODES
{
	uint16 tick;
	uint16 value;
};

struct IMFINSTRUMENT
{
	char name[32];		// Inst. name (ASCIIZ-String, max. 31 chars)
	uint8 map[120];		// Multisample settings
	uint8 unused[8];
	IMFENVNODES nodes[3][16];
	IMFENVELOPE env[3];
	uint16 fadeout;		// Fadeout rate (0...0FFFH)
	uint16 smpnum;		// Number of samples in instrument
	char ii10[4];		// 'II10'
};

struct IMFSAMPLE
{
	char filename[13];	// Sample filename (12345678.ABC) */
	uint8 unused1[3];
	uint32 length;		// Length
	uint32 loop_start;	// Loop start
	uint32 loop_end;	// Loop end
	uint32 C5Speed;		// Samplerate
	uint8 volume;		// Default volume (0...64)
	uint8 panning;		// Default pan (0...255)
	uint8 unused2[14];
	uint8 flags;		// Sample flags
	uint8 unused3[5];
	uint16 ems;			// Reserved for internal usage
	uint32 dram;		// Reserved for internal usage
	char is10[4];		// 'IS10'
};
#pragma pack()

static BYTE imf_efftrans[] =
{
	CMD_NONE,
	CMD_SPEED,			// 0x01 1xx Set Tempo
	CMD_TEMPO,			// 0x02 2xx Set BPM
	CMD_TONEPORTAMENTO, // 0x03 3xx Tone Portamento
	CMD_TONEPORTAVOL,	// 0x04 4xy Tone Portamento + Volume Slide
	CMD_VIBRATO,		// 0x05 5xy Vibrato
	CMD_VIBRATOVOL,		// 0x06 6xy Vibrato + Volume Slide
	CMD_FINEVIBRATO,	// 0x07 7xy Fine Vibrato
	CMD_TREMOLO,		// 0x08 8xy Tremolo
	CMD_ARPEGGIO,		// 0x09 9xy Arpeggio
	CMD_PANNING8,		// 0x0A Axx Set Pan Position
	CMD_PANNINGSLIDE,	// 0x0B Bxy Pan Slide
	CMD_VOLUME,			// 0x0C Cxx Set Volume
	CMD_VOLUMESLIDE,	// 0x0D Dxy Volume Slide
	CMD_VOLUMESLIDE,	// 0x0E Exy Fine Volume Slide
	CMD_S3MCMDEX,		// 0x0F Fxx Set Finetune
	CMD_NOTESLIDEUP,	// 0x10 Gxy Note Slide Up
	CMD_NOTESLIDEDOWN,	// 0x11 Hxy Note Slide Down
	CMD_PORTAMENTOUP,	// 0x12 Ixx Slide Up
	CMD_PORTAMENTODOWN,	// 0x13 Jxx Slide Down
	CMD_PORTAMENTOUP,	// 0x14 Kxx Fine Slide Up
	CMD_PORTAMENTODOWN,	// 0x15 Lxx Fine Slide Down
	CMD_MIDI,			// 0x16 Mxx Set Filter Cutoff - XXX
	CMD_NONE,			// 0x17 Nxy Filter Slide + Resonance - XXX
	CMD_OFFSET,			// 0x18 Oxx Set Sample Offset
	CMD_NONE,			// 0x19 Pxx Set Fine Sample Offset - XXX
	CMD_KEYOFF,			// 0x1A Qxx Key Off
	CMD_RETRIG,			// 0x1B Rxy Retrig
	CMD_TREMOR,			// 0x1C Sxy Tremor
	CMD_POSITIONJUMP,	// 0x1D Txx Position Jump
	CMD_PATTERNBREAK,	// 0x1E Uxx Pattern Break
	CMD_GLOBALVOLUME,	// 0x1F Vxx Set Mastervolume
	CMD_GLOBALVOLSLIDE,	// 0x20 Wxy Mastervolume Slide
	CMD_S3MCMDEX,		// 0x21 Xxx Extended Effect
							// X1x Set Filter
							// X3x Glissando
							// X5x Vibrato Waveform
							// X8x Tremolo Waveform
							// XAx Pattern Loop
							// XBx Pattern Delay
							// XCx Note Cut
							// XDx Note Delay
							// XEx Ignore Envelope
							// XFx Invert Loop
	CMD_NONE,			// 0x22 Yxx Chorus - XXX
	CMD_NONE,			// 0x23 Zxx Reverb - XXX
};

static void import_imf_effect(MODCOMMAND *note)
//---------------------------------------------
{
	uint8 n;
	// fix some of them
	switch (note->command)
	{
	case 0xe: // fine volslide
		// hackaround to get almost-right behavior for fine slides (i think!)
		if (note->param == 0)
			/* nothing */;
		else if (note->param == 0xf0)
			note->param = 0xef;
		else if (note->param == 0x0f)
			note->param = 0xfe;
		else if (note->param & 0xf0)
			note->param |= 0xf;
		else
			note->param |= 0xf0;
		break;
	case 0xf: // set finetune
		// we don't implement this, but let's at least import the value
		note->param = 0x20 | min(note->param >> 4, 0xf);
		break;
	case 0x14: // fine slide up
	case 0x15: // fine slide down
		// this is about as close as we can do...
		if (note->param >> 4)
			note->param = 0xf0 | min(note->param >> 4, 0xf);
		else
			note->param |= 0xe0;
		break;
	case 0x16: // cutoff
		note->param >>= 1;
		break;
	case 0x1f: // set global volume
		note->param = min(note->param << 1, 0xff);
		break;
	case 0x21:
		n = 0;
		switch (note->param >> 4)
		{
		case 0:
			/* undefined, but since S0x does nothing in IT anyway, we won't care.
			this is here to allow S00 to pick up the previous value (assuming IMF
			even does that -- I haven't actually tried it) */
			break;
		default: // undefined
		case 0x1: // set filter
		case 0xf: // invert loop
			note->command = CMD_NONE;
			break;
		case 0x3: // glissando
			n = 0x20;
			break;
		case 0x5: // vibrato waveform
			n = 0x30;
			break;
		case 0x8: // tremolo waveform
			n = 0x40;
			break;
		case 0xa: // pattern loop
			n = 0xb0;
			break;
		case 0xb: // pattern delay
			n = 0xe0;
			break;
		case 0xc: // note cut
		case 0xd: // note delay
			// no change
			break;
		case 0xe: // ignore envelope
			/* predicament: we can only disable one envelope at a time.
			volume is probably most noticeable, so let's go with that.
			(... actually, orpheus doesn't even seem to implement this at all) */
			note->param = 0x77;
			break;
		case 0x18: // sample offset
			// O00 doesn't pick up the previous value
			if (!note->param)
				note->command = CMD_NONE;
			break;
		}
		if (n)
			note->param = n | (note->param & 0xf);
		break;
	}
	note->command = (note->command < 0x24) ? imf_efftrans[note->command] : CMD_NONE;
	if (note->command == CMD_VOLUME && note->volcmd == VOLCMD_NONE)
	{
		note->volcmd = VOLCMD_VOLUME;
		note->vol = note->param;
		note->command = CMD_NONE;
		note->param = 0;
	}
}

static void load_imf_envelope(INSTRUMENTENVELOPE *env, const IMFINSTRUMENT *imfins, const int e)
//----------------------------------------------------------------------------------------------
{
	UINT min = 0; // minimum tick value for next node
	const int shift = (e == IMF_ENV_VOL) ? 0 : 2;

	env->dwFlags = ((imfins->env[e].flags & 1) ? ENV_ENABLED : 0) | ((imfins->env[e].flags & 2) ? ENV_SUSTAIN : 0) | ((imfins->env[e].flags & 4) ? ENV_LOOP : 0);
	env->nNodes = CLAMP(imfins->env[e].points, 2, 25);
	env->nLoopStart = imfins->env[e].loop_start;
	env->nLoopEnd = imfins->env[e].loop_end;
	env->nSustainStart = env->nSustainEnd = imfins->env[e].sustain;
	env->nReleaseNode = ENV_RELEASE_NODE_UNSET;

	for(UINT n = 0; n < env->nNodes; n++)
	{
		uint16 nTick, nValue;
		nTick = LittleEndianW(imfins->nodes[e][n].tick);
		nValue = LittleEndianW(imfins->nodes[e][n].value) >> shift;
		env->Ticks[n] = (WORD)max(min, nTick);
		env->Values[n] = (BYTE)min(nValue, ENVELOPE_MAX);
		min = nTick + 1;
	}
}

bool CSoundFile::ReadIMF(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------------------
{
	DWORD dwMemPos = 0;
	IMFHEADER hdr;
	MODSAMPLE *pSample = Samples + 1;
	WORD firstsample = 1; // first pSample for the current instrument
	uint32 ignore_channels = 0; // bit set for each channel that's completely disabled

	ASSERT_CAN_READ(sizeof(IMFHEADER));
	memset(&hdr, 0, sizeof(IMFHEADER));
	memcpy(&hdr, lpStream, sizeof(IMFHEADER));
	dwMemPos = sizeof(IMFHEADER);

	hdr.ordnum = LittleEndianW(hdr.ordnum);
	hdr.patnum = LittleEndianW(hdr.patnum);
	hdr.insnum = LittleEndianW(hdr.insnum);
	hdr.flags = LittleEndianW(hdr.flags);

	if (memcmp(hdr.im10, "IM10", 4) != 0)
		return false;

	m_nType = MOD_TYPE_IMF;
	SetModFlag(MSF_COMPATIBLE_PLAY, true);

	// song name
	memset(m_szNames, 0, sizeof(m_szNames));
	memcpy(m_szNames[0], hdr.title, 31);
	SpaceToNullStringFixed<31>(m_szNames[0]);

	if (hdr.flags & 1)
		m_dwSongFlags |= SONG_LINEARSLIDES;
	m_nDefaultSpeed = hdr.tempo;
	m_nDefaultTempo = hdr.bpm;
	m_nDefaultGlobalVolume = CLAMP(hdr.master, 0, 64) << 2;
	m_nSamplePreAmp = CLAMP(hdr.amp, 4, 127);

	m_nSamples = 0; // Will be incremented later
	m_nInstruments = 0;
	
	m_nChannels = 0;
	for(CHANNELINDEX nChn = 0; nChn < 32; nChn++)
	{
		ChnSettings[nChn].nPan = hdr.channels[nChn].panning * 64 / 255;
		ChnSettings[nChn].nPan *= 4;

		memcpy(ChnSettings[nChn].szName, hdr.channels[nChn].name, 12);
		SpaceToNullStringFixed<12>(ChnSettings[nChn].szName);

		// TODO: reverb/chorus?
		switch(hdr.channels[nChn].status)
		{
		case 0: // enabled; don't worry about it
			m_nChannels = nChn + 1;
			break;
		case 1: // mute
			ChnSettings[nChn].dwFlags |= CHN_MUTE;
			m_nChannels = nChn + 1;
			break;
		case 2: // disabled
			ChnSettings[nChn].dwFlags |= CHN_MUTE;
			ignore_channels |= (1 << nChn);
			break;
		default: // uhhhh.... freak out
			//fprintf(stderr, "imf: channel %d has unknown status %d\n", n, hdr.channels[n].status);
			return false;
		}
	}
	if(!m_nChannels) return false;

	//From mikmod: work around an Orpheus bug
	if (hdr.channels[0].status == 0)
	{
		CHANNELINDEX nChn;
		for(nChn = 1; nChn < 16; nChn++)
			if(hdr.channels[nChn].status != 1)
				break;
		if (nChn == 16)
			for(nChn = 1; nChn < 16; nChn++)
				ChnSettings[nChn].dwFlags &= ~CHN_MUTE;
	}

	Order.resize(hdr.ordnum);
	for(ORDERINDEX nOrd = 0; nOrd < hdr.ordnum; nOrd++)
		Order[nOrd] = ((hdr.orderlist[nOrd] == 0xff) ? Order.GetIgnoreIndex() : (PATTERNINDEX)hdr.orderlist[nOrd]);
	
	// read patterns
	for(PATTERNINDEX nPat = 0; nPat < hdr.patnum; nPat++)
	{
		uint16 length, nrows;
		BYTE mask, channel;
		int row;
		unsigned int lostfx = 0;
		MODCOMMAND *row_data, *note, junk_note;

		ASSERT_CAN_READ(4);
		length = LittleEndianW(*((uint16 *)(lpStream + dwMemPos)));
		nrows = LittleEndianW(*((uint16 *)(lpStream + dwMemPos + 2)));
		dwMemPos += 4;

		if(Patterns.Insert(nPat, nrows))
			break;

		row_data = Patterns[nPat];

		row = 0;
		while(row < nrows)
		{
			ASSERT_CAN_READ(1);
			mask = *((BYTE *)(lpStream + dwMemPos));
			dwMemPos += 1;
			if (mask == 0) {
				row++;
				row_data += m_nChannels;
				continue;
			}

			channel = mask & 0x1f;

			if(ignore_channels & (1 << channel))
			{
				/* should do this better, i.e. not go through the whole process of deciding
				what to do with the effects since they're just being thrown out */
				//printf("disabled channel %d contains data\n", channel + 1);
				note = &junk_note;
			} else
			{
				note = row_data + channel;
			}

			if(mask & 0x20)
			{
				// read note/instrument
				ASSERT_CAN_READ(2);
				note->note = *((BYTE *)(lpStream + dwMemPos));
				note->instr = *((BYTE *)(lpStream + dwMemPos + 1));
				dwMemPos += 2;

				if (note->note == 160)
				{
					note->note = NOTE_KEYOFF; /* ??? */
				} else if (note->note == 255)
				{
					note->note = NOTE_NONE; /* ??? */
				} else
				{
					note->note = (note->note >> 4) * 12 + (note->note & 0xf) + 12 + 1;
					if(note->note > NOTE_MAX)
					{
						/*printf("%d.%d.%d: funny note 0x%02x\n",
							nPat, row, channel, fp->data[fp->pos - 1]);*/
						note->note = NOTE_NONE;
					}
				}
			}
			if((mask & 0xc0) == 0xc0)
			{
				uint8 e1c, e1d, e2c, e2d;

				// read both effects and figure out what to do with them
				ASSERT_CAN_READ(4);
				e1c = *((uint8 *)(lpStream + dwMemPos));
				e1d = *((uint8 *)(lpStream + dwMemPos + 1));
				e2c = *((uint8 *)(lpStream + dwMemPos + 2));
				e2d = *((uint8 *)(lpStream + dwMemPos + 3));
				dwMemPos += 4;

				if (e1c == 0xc)
				{
					note->vol = min(e1d, 0x40);
					note->volcmd = VOLCMD_VOLUME;
					note->command = e2c;
					note->param = e2d;
				} else if (e2c == 0xc)
				{
					note->vol = min(e2d, 0x40);
					note->volcmd = VOLCMD_VOLUME;
					note->command = e1c;
					note->param = e1d;
				} else if (e1c == 0xa)
				{
					note->vol = e1d * 64 / 255;
					note->volcmd = VOLCMD_PANNING;
					note->command = e2c;
					note->param = e2d;
				} else if (e2c == 0xa)
				{
					note->vol = e2d * 64 / 255;
					note->volcmd = VOLCMD_PANNING;
					note->command = e1c;
					note->param = e1d;
				} else
				{
					/* check if one of the effects is a 'global' effect
					-- if so, put it in some unused channel instead.
					otherwise pick the most important effect. */
					lostfx++;
					note->command = e2c;
					note->param = e2d;
				}
			} else if(mask & 0xc0)
			{
				// there's one effect, just stick it in the effect column
				ASSERT_CAN_READ(2);
				note->command = *((BYTE *)(lpStream + dwMemPos));
				note->param = *((BYTE *)(lpStream + dwMemPos + 1));
				dwMemPos += 2;
			}
			if(note->command)
				import_imf_effect(note);
		}
	}

	// read instruments
	for (INSTRUMENTINDEX nIns = 0; nIns < hdr.insnum; nIns++)
	{
		IMFINSTRUMENT imfins;
		MODINSTRUMENT *pIns;
		ASSERT_CAN_READ(sizeof(IMFINSTRUMENT));
		memset(&imfins, 0, sizeof(IMFINSTRUMENT));
		memcpy(&imfins, lpStream + dwMemPos, sizeof(IMFINSTRUMENT));
		dwMemPos += sizeof(IMFINSTRUMENT);
		m_nInstruments++;

		imfins.smpnum = LittleEndianW(imfins.smpnum);
		imfins.fadeout = LittleEndianW(imfins.fadeout);

		// Orpheus does not check this!
		//if(memcmp(imfins.ii10, "II10", 4) != 0)
		//	return false;
		
		pIns = new MODINSTRUMENT;
		if(!pIns)
			continue;
		Instruments[nIns + 1] = pIns;
		memset(pIns, 0, sizeof(MODINSTRUMENT));
		pIns->nPPC = 5 * 12;
		SetDefaultInstrumentValues(pIns);

		memcpy(pIns->name, imfins.name, 31);
		SpaceToNullStringFixed<31>(pIns->name);

		if(imfins.smpnum)
		{
			for(BYTE cNote = 0; cNote < 120; cNote++)
			{
				pIns->NoteMap[cNote] = cNote + 1;
				pIns->Keyboard[cNote] = firstsample + imfins.map[cNote];
			}
		}

		pIns->nFadeOut = imfins.fadeout;
		pIns->nGlobalVol = 64;

		load_imf_envelope(&pIns->VolEnv, &imfins, IMF_ENV_VOL);
		load_imf_envelope(&pIns->PanEnv, &imfins, IMF_ENV_PAN);
		load_imf_envelope(&pIns->PitchEnv, &imfins, IMF_ENV_FILTER);
		if((pIns->PitchEnv.dwFlags & ENV_ENABLED) != 0)
			pIns->PitchEnv.dwFlags |= ENV_FILTER;

		// hack to get === to stop notes (from modplug's xm loader)
		if(!(pIns->VolEnv.dwFlags & ENV_ENABLED) && !pIns->nFadeOut)
			pIns->nFadeOut = 8192;

		// read this instrument's samples
		for(SAMPLEINDEX nSmp = 0; nSmp < imfins.smpnum; nSmp++)
		{
			IMFSAMPLE imfsmp;
			uint32 blen;
			ASSERT_CAN_READ(sizeof(IMFSAMPLE));
			memset(&imfsmp, 0, sizeof(IMFSAMPLE));
			memcpy(&imfsmp, lpStream + dwMemPos, sizeof(IMFSAMPLE));
			dwMemPos += sizeof(IMFSAMPLE);
			m_nSamples++;

			if(memcmp(imfsmp.is10, "IS10", 4) != 0)
				return false;
			
			memcpy(pSample->filename, imfsmp.filename, 12);
			SpaceToNullStringFixed<12>(pSample->filename);
			strcpy(m_szNames[m_nSamples], pSample->filename);

			blen = pSample->nLength = LittleEndian(imfsmp.length);
			pSample->nLoopStart = LittleEndian(imfsmp.loop_start);
			pSample->nLoopEnd = LittleEndian(imfsmp.loop_end);
			pSample->nC5Speed = LittleEndian(imfsmp.C5Speed);
			pSample->nVolume = imfsmp.volume * 4;
			pSample->nGlobalVol = 256;
			pSample->nPan = imfsmp.panning;
			if (imfsmp.flags & 1)
				pSample->uFlags |= CHN_LOOP;
			if (imfsmp.flags & 2)
				pSample->uFlags |= CHN_PINGPONGLOOP;
			if (imfsmp.flags & 4)
			{
				pSample->uFlags |= CHN_16BIT;
				pSample->nLength >>= 1;
				pSample->nLoopStart >>= 1;
				pSample->nLoopEnd >>= 1;
			}
			if (imfsmp.flags & 8)
				pSample->uFlags |= CHN_PANNING;
			
			if(blen)
			{
				ASSERT_CAN_READ(blen);
				ReadSample(pSample, (imfsmp.flags & 4) ? RS_PCM16S : RS_PCM8S, reinterpret_cast<LPCSTR>(lpStream + dwMemPos), blen);
			}

			dwMemPos += blen;
			pSample++;
		}
		firstsample += imfins.smpnum;
	}
	
	return true;
}