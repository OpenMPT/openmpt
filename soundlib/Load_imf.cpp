/*
 * Purpose: Load IMF (Imago Orpheus) modules
 * Authors: Storlek (http://schismtracker.org/)
 *			Johannes Schultz (OpenMPT Port)
 *
 * Thanks to Storlek for allowing me to use this code!
 */

#include "stdafx.h"
#include "sndfile.h"

#pragma pack(1)

struct IMFCHANNEL {
	char name[12];	// Channelname (ASCIIZ-String, max 11 chars)
	BYTE chorus;	// Default chorus
	BYTE reverb;	// Default reverb
	BYTE panning;	// Pan positions 00-FF
	BYTE status;	// Channel status: 0 = enabled, 1 = mute, 2 = disabled (ignore effects!)
};

struct IMFHEADER {
	char title[32];			// Songname (ASCIIZ-String, max. 31 chars)
	UINT16 ordnum;			// Number of orders saved
	UINT16 patnum;			// Number of patterns saved
	UINT16 insnum;			// Number of instruments saved
	UINT16 flags;			// Module flags (&1 => linear)
	BYTE unused1[8];
	BYTE tempo;				// Default tempo (Axx, 1..255)
	BYTE bpm;				// Default beats per minute (BPM) (Txx, 32..255)
	BYTE master;			// Default mastervolume (Vxx, 0..64)
	BYTE amp;				// Amplification factor (mixing volume, 4..127)
	BYTE unused2[8];
	char im10[4];			// 'IM10'
	struct IMFCHANNEL channels[32]; // Channel settings
	BYTE orderlist[256];	// Order list (0xff = +++; blank out anything beyond ordnum)
};

enum {
	IMF_ENV_VOL = 0,
	IMF_ENV_PAN = 1,
	IMF_ENV_FILTER = 2,
};

struct IMFENVELOPE {
	BYTE points;		// Number of envelope points
	BYTE sustain;		// Envelope sustain point
	BYTE loop_start;	// Envelope loop start point
	BYTE loop_end;		// Envelope loop end point
	BYTE flags;			// Envelope flags
	BYTE unused[3];
};

struct IMFENVNODES {
	UINT16 tick;
	UINT16 value;
};

struct IMFINSTRUMENT {
	char name[32];		// Inst. name (ASCIIZ-String, max. 31 chars)
	BYTE map[120];		// Multisample settings
	BYTE unused[8];
	struct IMFENVNODES nodes[3][16];
	struct IMFENVELOPE env[3];
	UINT16 fadeout;		// Fadeout rate (0...0FFFH)
	UINT16 smpnum;		// Number of samples in instrument
	char ii10[4];		// 'II10'
};

struct IMFSAMPLE {
	char filename[13];		// Sample filename (12345678.ABC) */
	BYTE unused1[3];
	UINT32 length;		// Length
	UINT32 loop_start;	// Loop start
	UINT32 loop_end;	// Loop end
	UINT32 C5Speed;		// Samplerate
	BYTE volume;		// Default volume (0..64)
	BYTE panning;		// Default pan (00h = Left / 80h = Middle)
	BYTE unused2[14];
	BYTE flags;			// Sample flags
	BYTE unused3[5];
	UINT16 ems;			// Reserved for internal usage
	UINT32 dram;		// Reserved for internal usage
	char is10[4];		// 'IS10'
};
#pragma pack()


static BYTE imf_efftrans[] = {
	CMD_NONE,
	CMD_SPEED, // 0x01 1xx Set Tempo
	CMD_TEMPO, // 0x02 2xx Set BPM
	CMD_TONEPORTAMENTO, // 0x03 3xx Tone Portamento
	CMD_TONEPORTAVOL, // 0x04 4xy Tone Portamento + Volume Slide
	CMD_VIBRATO, // 0x05 5xy Vibrato
	CMD_VIBRATOVOL, // 0x06 6xy Vibrato + Volume Slide
	CMD_FINEVIBRATO, // 0x07 7xy Fine Vibrato
	CMD_TREMOLO, // 0x08 8xy Tremolo
	CMD_ARPEGGIO, // 0x09 9xy Arpeggio
	CMD_PANNING8, // 0x0A Axx Set Pan Position
	CMD_PANNINGSLIDE, // 0x0B Bxy Pan Slide
	CMD_VOLUME, // 0x0C Cxx Set Volume
	CMD_VOLUMESLIDE, // 0x0D Dxy Volume Slide
	CMD_VOLUMESLIDE, // 0x0E Exy Fine Volume Slide
	CMD_S3MCMDEX, // 0x0F Fxx Set Finetune
	CMD_NOTESLIDEUP, // 0x10 Gxy Note Slide Up
	CMD_NOTESLIDEDOWN, // 0x11 Hxy Note Slide Down
	CMD_PORTAMENTOUP, // 0x12 Ixx Slide Up
	CMD_PORTAMENTODOWN, // 0x13 Jxx Slide Down
	CMD_PORTAMENTOUP, // 0x14 Kxx Fine Slide Up
	CMD_PORTAMENTODOWN, // 0x15 Lxx Fine Slide Down
	CMD_MIDI, // 0x16 Mxx Set Filter Cutoff - XXX
	CMD_NONE, // 0x17 Nxy Filter Slide + Resonance - XXX
	CMD_OFFSET, // 0x18 Oxx Set Sample Offset
	CMD_NONE, // 0x19 Pxx Set Fine Sample Offset - XXX
	CMD_KEYOFF, // 0x1A Qxx Key Off
	CMD_RETRIG, // 0x1B Rxy Retrig
	CMD_TREMOR, // 0x1C Sxy Tremor
	CMD_POSITIONJUMP, // 0x1D Txx Position Jump
	CMD_PATTERNBREAK, // 0x1E Uxx Pattern Break
	CMD_GLOBALVOLUME, // 0x1F Vxx Set Mastervolume
	CMD_GLOBALVOLSLIDE, // 0x20 Wxy Mastervolume Slide
	CMD_S3MCMDEX, // 0x21 Xxx Extended Effect
	//      X1x Set Filter
	//      X3x Glissando
	//      X5x Vibrato Waveform
	//      X8x Tremolo Waveform
	//      XAx Pattern Loop
	//      XBx Pattern Delay
	//      XCx Note Cut
	//      XDx Note Delay
	//      XEx Ignore Envelope
	//      XFx Invert Loop
	CMD_NONE, // 0x22 Yxx Chorus - XXX
	CMD_NONE, // 0x23 Zxx Reverb - XXX
};

static void import_imf_effect(MODCOMMAND *note)
{
	BYTE n;
	// fix some of them
	switch (note->command) {
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
	case 0x1f: // set global volume
		note->param = min(note->param << 1, 0xff);
		break;
	case 0x21:
		n = 0;
		switch (note->param >> 4) {
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
	if (note->command == CMD_VOLUME && note->volcmd == VOLCMD_NONE) {
		note->volcmd = VOLCMD_VOLUME;
		note->vol = note->param;
		note->command = CMD_NONE;
		note->param = 0;
	}
}

#ifdef _IMF_SUPPORT_FINISHED_

static void load_imf_pattern(CSoundFile *csf, int pat, UINT32 ignore_channels, slurp_t *fp)
{
	UINT16 length, nrows;
	BYTE mask, channel;
	int row, startpos;
	unsigned int lostfx = 0;
	MODCOMMAND *row_data, *note, junk_note;
	
	startpos = slurp_tell(fp);
	
	slurp_read(fp, &length, 2);
	length = LittleEndianW(length);
	slurp_read(fp, &nrows, 2);
	nrows = LittleEndianW(nrows);

	csf->Patterns.Insert(pat, nrows);
	//row_data = Patterns[pat] = csf_allocate_pattern(nrows, 64);
	//PatternSize[pat] = PatternAllocSize[pat] = nrows;
	row_data = csf->Patterns[pat];

	row = 0;
	while (row < nrows) {
		mask = slurp_getc(fp);
		if (mask == 0) {
			row++;
			row_data += MAX_CHANNELS;
			continue;
		}
		
		channel = mask & 0x1f;
		
		if (ignore_channels & (1 << channel)) {
			/* should do this better, i.e. not go through the whole process of deciding
			what to do with the effects since they're just being thrown out */
			//printf("disabled channel %d contains data\n", channel + 1);
			note = &junk_note;
		} else {
			note = row_data + channel;
		}
		
		if (mask & 0x20) {
			/* read note/instrument */
			note->note = slurp_getc(fp);
			note->instr = slurp_getc(fp);
			if (note->note == 160) {
				note->note = NOTE_KEYOFF; /* ??? */
			} else if (note->note == 255) {
				note->note = NOTE_NONE; /* ??? */
			} else {
				note->note = (note->note >> 4) * 12 + (note->note & 0xf) + 12 + 1;
				if (note->note > NOTE_MAX) {
					//printf("%d.%d.%d: funny note 0x%02x\n",
					//	pat, row, channel, fp->data[fp->pos - 1]);
					note->note = NOTE_NONE;
				}
			}
		}
		if ((mask & 0xc0) == 0xc0) {
			BYTE e1c, e1d, e2c, e2d;
			
			/* read both effects and figure out what to do with them */
			e1c = slurp_getc(fp);
			e1d = slurp_getc(fp);
			e2c = slurp_getc(fp);
			e2d = slurp_getc(fp);
			if (e1c == 0xc) {
				note->vol = min(e1d, 0x40);
				note->volcmd = VOLCMD_VOLUME;
				note->command = e2c;
				note->param = e2d;
			} else if (e2c == 0xc) {
				note->vol = min(e2d, 0x40);
				note->volcmd = VOLCMD_VOLUME;
				note->command = e1c;
				note->param = e1d;
			} else if (e1c == 0xa) {
				note->vol = e1d * 64 / 255;
				note->volcmd = VOLCMD_PANNING;
				note->command = e2c;
				note->param = e2d;
			} else if (e2c == 0xa) {
				note->vol = e2d * 64 / 255;
				note->volcmd = VOLCMD_PANNING;
				note->command = e1c;
				note->param = e1d;
			} else {
				/* check if one of the effects is a 'global' effect
				-- if so, put it in some unused channel instead.
				otherwise pick the most important effect. */
				lostfx++;
				note->command = e2c;
				note->param = e2d;
			}
		} else if (mask & 0xc0) {
			/* there's one effect, just stick it in the effect column */
			note->command = slurp_getc(fp);
			note->param = slurp_getc(fp);
		}
		if (note->command)
			import_imf_effect(note);
	}
	
	/*if (lostfx)
		log_appendf(2, "Pattern %d: %d effect%s skipped!\n", pat, lostfx, lostfx == 1 ? "" : "s");*/
}


static unsigned int envflags[3][3] = {
	{ENV_VOLUME,             ENV_VOLSUSTAIN,   ENV_VOLLOOP},
	{ENV_PANNING,            ENV_PANSUSTAIN,   ENV_PANLOOP},
	{ENV_PITCH | ENV_FILTER, ENV_PITCHSUSTAIN, ENV_PITCHLOOP},
};

static void load_imf_envelope(MODINSTRUMENT *ins, INSTRUMENTENVELOPE *env, IMFINSTRUMENT *imfins, int e)
{
	int n, t, v;
	int min = 0; // minimum tick value for next node
	int shift = (e == IMF_ENV_VOL ? 0 : 2);

	env->nNodes = CLAMP(imfins->env[e].points, 2, 25);
	env->nLoopStart = imfins->env[e].loop_start;
	env->nLoopEnd = imfins->env[e].loop_end;
	env->nSustainStart = env->nSustainEnd = imfins->env[e].sustain;

	for (n = 0; n < env->nNodes; n++) {
		t = LittleEndianW(imfins->nodes[e][n].tick);
		v = LittleEndianW(imfins->nodes[e][n].value) >> shift;
		env->Ticks[n] = max(min, t);
		env->Values[n] = v = min(v, 64);
		min = t + 1;
	}
	// this would be less retarded if the envelopes all had their own flags...
	if (imfins->env[e].flags & 1)
		ins->dwFlags |= envflags[e][0];
	if (imfins->env[e].flags & 2)
		ins->dwFlags |= envflags[e][1];
	if (imfins->env[e].flags & 4)
		ins->dwFlags |= envflags[e][2];
}

//int fmt_imf_load_song(CSoundFile *song, slurp_t *fp, UNUSED unsigned int lflags)
bool CSoundFile::ReadIMF(const LPCBYTE lpStream, const DWORD dwMemLength)
{
	DWORD dwMemPos;
	IMFHEADER hdr;
	int n, s;
	MODSAMPLE *pSample = Samples + 1;
	int firstsample = 1; // first pSample for the current instrument
	UINT32 ignore_channels = 0; /* bit set for each channel that's completely disabled */

	//slurp_read(fp, &hdr, sizeof(hdr));
	if(sizeof(IMFHEADER) > dwMemLength) return false;
	memset(hdr, 0, sizeof(IMFHEADER));
	memcpy(hdr, lpStream, sizeof(IMFHEADER))
	hdr.ordnum = LittleEndianW(hdr.ordnum);
	hdr.patnum = LittleEndianW(hdr.patnum);
	hdr.insnum = LittleEndianW(hdr.insnum);
	hdr.flags = LittleEndianW(hdr.flags);

	if (memcmp(hdr.im10, "IM10", 4) != 0)
		return false;

	// song name
	memset(m_szNames, 0, sizeof(m_szNames));
	memcpy(m_szNames[0], hdr.title, 25);
	m_szNames[0][25] = 0;
	SetNullTerminator(m_szNames[0]);

	if (hdr.flags & 1)
		m_dwSongFlags |= SONG_LINEARSLIDES;
	//m_dwSongFlags |= SONG_INSTRUMENTMODE;
	m_nDefaultSpeed = hdr.tempo;
	m_nDefaultTempo = hdr.bpm;
	m_nDefaultGlobalVolume = hdr.master << 1;
	m_nSamplePreAmp = hdr.amp;
	m_nVSTiVolume = 48; // not supported
	
	for (n = 0; n < 32; n++) {
		Chn[n].nPan = hdr.channels[n].panning * 64 / 255;
		Chn[n].nPan *= 4; //mphack
		/* TODO: reverb/chorus??? */
		switch (hdr.channels[n].status) {
		case 0: /* enabled; don't worry about it */
			break;
		case 1: /* mute */
			Chn[n].dwFlags |= CHN_MUTE;
			break;
		case 2: /* disabled */
			Chn[n].dwFlags |= CHN_MUTE;
			ignore_channels |= (1 << n);
			break;
		default: /* uhhhh.... freak out */
			//fprintf(stderr, "imf: channel %d has unknown status %d\n", n, hdr.channels[n].status);
			return false;
		}
	}
	for (; n < MAX_CHANNELS; n++)
		Chn[n].dwFlags |= CHN_MUTE;
	
	for (n = 0; n < hdr.ordnum; n++)
		Order[n] = ((hdr.orderlist[n] == 0xff) ? Order.GetIgnoreIndex() : hdr.orderlist[n]);
	
	for (n = 0; n < hdr.patnum; n++)
		load_imf_pattern(this, n, ignore_channels, fp);
	
	dwMemPos = sizeof(IMFHEADER);

	for (n = 0; n < hdr.insnum; n++) {
		// read the ins header
		struct IMFINSTRUMENT imfins;
		MODINSTRUMENT *ins;
		slurp_read(fp, &imfins, sizeof(imfins));

		imfins.smpnum = LittleEndianW(imfins.smpnum);
		imfins.fadeout = LittleEndianW(imfins.fadeout);

		if (memcmp(imfins.ii10, "II10", 4) != 0) {
			//printf("ii10 says %02x %02x %02x %02x!\n",
			//	imfins.ii10[0], imfins.ii10[1], imfins.ii10[2], imfins.ii10[3]);
			return false;
		}
		
		ins = new MODINSTRUMENT;
		if (!ins) continue;
		Instruments[n + 1] = ins;
		memset(ins, 0, sizeof(MODINSTRUMENT));

		strncpy(ins->name, imfins.name, 25);
		ins->name[25] = 0;

		if (imfins.smpnum) {
			for (s = 0; s < 120; s++) {
				ins->NoteMap[s] = s + 1;
				ins->Keyboard[s] = firstsample + imfins.map[s];
			}
		}

		/* Fadeout:
		IT1 - 64
		IT2 - 256
		FT2 - 4095
		IMF - 4095
		MPT - god knows what, all the loaders are inconsistent
		Schism - 128 presented (!); 8192? internal

		IMF and XM have the same range and modplug's XM loader doesn't do any bit shifting with it,
		so I'll do the same here for now. I suppose I should get this nonsense straightened
		out at some point, though. */
		ins->nFadeOut = imfins.fadeout;
		ins->nGlobalVol = 128;

		load_imf_envelope(ins, &ins->VolEnv, &imfins, IMF_ENV_VOL);
		load_imf_envelope(ins, &ins->PanEnv, &imfins, IMF_ENV_PAN);
		load_imf_envelope(ins, &ins->PitchEnv, &imfins, IMF_ENV_FILTER);

		// hack to get === to stop notes (from modplug's xm loader)
		if (!(ins->dwFlags & ENV_VOLUME) && !ins->nFadeOut)
			ins->nFadeOut = 8192;

		for (s = 0; s < imfins.smpnum; s++) {
			IMFSAMPLE imfsmp;
			UINT32 blen;
			slurp_read(fp, &imfsmp, sizeof(imfsmp));
			
			if (memcmp(imfsmp.is10, "IS10", 4) != 0) {
				//printf("is10 says %02x %02x %02x %02x!\n",
				//	imfsmp.is10[0], imfsmp.is10[1], imfsmp.is10[2], imfsmp.is10[3]);
				return false;
			}
			
			strncpy(pSample->filename, imfsmp.filename, 12);
			pSample->filename[12] = 0;
			strcpy(m_szNames[s + 1], pSample->filename);
			blen = pSample->nLength = LittleEndian(imfsmp.length);
			pSample->nLoopStart = LittleEndian(imfsmp.loop_start);
			pSample->nLoopEnd = LittleEndian(imfsmp.loop_end);
			pSample->nC5Speed = LittleEndian(imfsmp.C5Speed);
			pSample->nVolume = imfsmp.volume * 4; //mphack
			pSample->nPan = imfsmp.panning; //mphack (IT uses 0-64, IMF uses the full 0-255)
			if (imfsmp.flags & 1)
				pSample->uFlags |= CHN_LOOP;
			if (imfsmp.flags & 2)
				pSample->uFlags |= CHN_PINGPONGLOOP;
			if (imfsmp.flags & 4) {
				pSample->uFlags |= CHN_16BIT;
				pSample->nLength >>= 1;
				pSample->nLoopStart >>= 1;
				pSample->nLoopEnd >>= 1;
			}
			if (imfsmp.flags & 8)
				pSample->uFlags |= CHN_PANNING;
			
			if (!blen) {
				/* leave it blank */
			/*} else if (lflags & LOAD_NOSAMPLES) {
				slurp_seek(fp, blen, SEEK_CUR);*/
			} else {
				ReadSample(pSample, (imfsmp.flags & 4) ? RS_PCM8U : RS_PCM16U, reinterpret_cast<LPCSTR>(lpStream + iSampleOffset), blen);
			}

			pSample++;
		}
		firstsample += imfins.smpnum;
	}
	
	return true;
}
#endif