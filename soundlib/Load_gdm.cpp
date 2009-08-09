/*
 * This source code is public domain.
 *
 * Purpose: Load GDM (BWSB Soundsystem) modules
 * Authors: Johannes Schultz
 *
 * This code is partly based on zilym's original code / specs (which are utterly wrong :P).
 * Thanks to the MenTaLguY for gdm.txt and ajs for gdm2s3m and some hints.
 *
 * Hint 1: Most (all?) of the unsupported features were not supported in 2GDM / BWSB either.
 * Hint 2: Files will be played like their original formats would be played in MPT, so no
 *         BWSB quirks including crashes and freezes are supported. :-P
 */

#include "stdafx.h"
#include "sndfile.h"

#pragma pack(1)

typedef struct _GDMHEADER
{
	DWORD ID;						// ID: 'GDMþ'
	CHAR SongTitle[32];				// Music's title
	CHAR SongMusician[32];			// Name of music's composer
	CHAR DOSEOF[3];					// 13, 10, 26
	DWORD ID2;						// ID: 'GMFS'
	BYTE FormMajorVer;				// Format major version
	BYTE FormMinorVer;				// Format minor version
	UINT16 TrackID;					// Composing Tracker ID code (00 = 2GDM)
	BYTE TrackMajorVer;				// Tracker's major version
	BYTE TrackMinorVer;				// Tracker's minor version
	BYTE PanMap[32];				// 0-Left to 15-Right, 255-N/U
	BYTE MastVol;					// Range: 0...64
	BYTE Tempo;						// Initial music tempo (6)
	BYTE BPM;							// Initial music BPM (125)
	UINT16 FormOrigin;				// Original format ID:
		// 1-MOD, 2-MTM, 3-S3M, 4-669, 5-FAR, 6-ULT, 7-STM, 8-MED
		// (versions of 2GDM prior to v1.15 won't set this correctly)

	UINT32 OrdOffset;
	BYTE NOO;						// Number of orders in module - 1
	UINT32 PatOffset;
	BYTE NOP;						// Number of patterns in module - 1
	UINT32 SamHeadOffset;
	UINT32 SamOffset;
	BYTE NOS;						// Number of samples in module - 1
	UINT32 MTOffset;				// Offset of song message
	UINT32 MTLength;
	UINT32 SSOffset;				// Offset of scrolly script (huh?)
	UINT16 SSLength;
	UINT32 TGOffset;				// Offset of text graphic (huh?)
	UINT16 TGLength;
} GDMHEADER, *PGDMHEADER;

typedef struct _GDMSAMPLEHEADER
{
	CHAR SamName[32];	// sample's name
	CHAR FileName[12];	// sample's filename
	BYTE EmsHandle;		// useless
	UINT32 Length;		// length in bytes
	UINT32 LoopBegin;	// loop start in samples
	UINT32 LoopEnd;		// loop end in samples
	BYTE Flags;			// misc. flags
	UINT16 C4Hertz;		// frequency
	BYTE Volume;		// default volume
	BYTE Pan;			// default pan
} GDMSAMPLEHEADER, *PGDMSAMPLEHEADER;

bool CSoundFile::ReadGDM(const LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------
{
	if ((!lpStream) || (dwMemLength < sizeof(GDMHEADER))) return false;

	const PGDMHEADER pHeader = (PGDMHEADER)lpStream;

	// is it a valid GDM file?
	if(	(LittleEndian(pHeader->ID) != 0xFE4D4447) || //GDMþ
		(pHeader->DOSEOF[0] != 13 || pHeader->DOSEOF[1] != 10 || pHeader->DOSEOF[2] != 26) || //CR+LF+EOF
		(LittleEndian(pHeader->ID2) != 0x53464D47)) return false; //GMFS

	// song name
	memset(m_szNames, 0, sizeof(m_szNames));
	memcpy(m_szNames[0], pHeader->SongTitle, 32);
	SetNullTerminator(m_szNames[0]);

	// there are no other format versions...
	if(pHeader->FormMajorVer != 1 || pHeader->FormMinorVer != 0)
	{
		::MessageBox(0, TEXT("GDM file seems to be valid, but this format version is currently not supported."), TEXT("OpenMPT GDM import"), MB_ICONERROR);
		return false;
	}

	// interesting question: Is TrackID, TrackMajorVer, TrackMinorVer relevant? The only TrackID should be 0 - 2GDM.exe, so the answer would be no.

	// read channel pan map... 0...15 = channel panning, 16 = surround channel, 255 = channel does not exist
	m_nChannels = 32;
	for(int i = 0; i < 32; i++)
	{
		if(pHeader->PanMap[i] < 16)
		{		
			ChnSettings[i].nPan = min((pHeader->PanMap[i] << 4) + 8, 256);
		}
		else if(pHeader->PanMap[i] == 16)
		{
			ChnSettings[i].nPan = 128;
			ChnSettings[i].dwFlags |= CHN_SURROUND;
		}
		else if(pHeader->PanMap[i] == 0xff)
		{
			m_nChannels = i;
			break;
		}
	}

	m_nDefaultGlobalVolume = min(pHeader->MastVol << 2, 256);
	m_nDefaultSpeed = pHeader->Tempo;
	m_nDefaultTempo = pHeader->BPM;
	m_nRestartPos = 0; // not supported in this format, so use the default value
	m_nSamplePreAmp = 48; // dito
	m_nVSTiVolume = 48; // dito

	// 1-MOD, 2-MTM, 3-S3M, 4-669, 5-FAR, 6-ULT, 7-STM, 8-MED

	switch(LittleEndianW(pHeader->FormOrigin))
	{
	case 1:
		m_nType = MOD_TYPE_MOD;
		break;
	case 2:
		m_nType = MOD_TYPE_MTM;
		break;
	case 3:
		m_nType = MOD_TYPE_S3M;
		break;
	case 4:
		m_nType = MOD_TYPE_669;
		break;
	case 5:
		m_nType = MOD_TYPE_FAR;
		break;
	case 6:
		m_nType = MOD_TYPE_ULT;
		break;
	case 7:
		m_nType = MOD_TYPE_STM;
		break;
	case 8:
		m_nType = MOD_TYPE_MED;
		break;
	default:
		::MessageBox(0, TEXT("GDM file seems to be valid, but the original format is currently not supported.\nThis should not happen."), TEXT("OpenMPT GDM import"), MB_ICONERROR);
		return false;
		break;
	}
	UINT32 iSampleOffset  = LittleEndian(pHeader->SamOffset),
		   iPatternsOffset = LittleEndian(pHeader->PatOffset);

	const UINT32 iOrdOffset = LittleEndian(pHeader->OrdOffset), iSamHeadOffset = LittleEndian(pHeader->SamHeadOffset), 
				 iMTOffset = LittleEndian(pHeader->MTOffset), iMTLength = LittleEndian(pHeader->MTLength),
				 iSSOffset = LittleEndian(pHeader->SSOffset), iSSLength = LittleEndianW(pHeader->SSLength),
				 iTGOffset = LittleEndian(pHeader->TGOffset), iTGLength = LittleEndianW(pHeader->TGLength);
	

	// check if offsets are valid. we won't read the scrolly text or text graphics, but invalid pointers would probably indicate a broken file...
	if(	   dwMemLength < iOrdOffset || dwMemLength - iOrdOffset < pHeader->NOO
		|| dwMemLength < iPatternsOffset
		|| dwMemLength < iSamHeadOffset || dwMemLength - iSamHeadOffset < (pHeader->NOS + 1) * sizeof(GDMSAMPLEHEADER)
		|| dwMemLength < iSampleOffset
		|| dwMemLength < iMTOffset || dwMemLength - iMTOffset < iMTLength
		|| dwMemLength < iSSOffset || dwMemLength - iSSOffset < iSSLength
		|| dwMemLength < iTGOffset || dwMemLength - iTGOffset < iTGLength)
		return false;

	// read orders
	Order.ReadAsByte(lpStream + iOrdOffset, pHeader->NOO + 1, dwMemLength - iOrdOffset);

	// read samples
	m_nSamples = pHeader->NOS + 1;
	
	int iLZWsamples = 0;

	for(UINT iSmp = 1; iSmp <= m_nSamples; iSmp++)
	{
		const PGDMSAMPLEHEADER pSample = (PGDMSAMPLEHEADER)(lpStream + iSamHeadOffset + (iSmp - 1) * sizeof(GDMSAMPLEHEADER));

		// sample header

		memcpy(m_szNames[iSmp], pSample->SamName, 32);
		SetNullTerminator(m_szNames[iSmp]);
		memcpy(Ins[iSmp].name, pSample->FileName, 12);

		Ins[iSmp].nC5Speed = LittleEndianW(pSample->C4Hertz);
		Ins[iSmp].nGlobalVol = 256; // not supported in this format
		Ins[iSmp].nLength = min(LittleEndian(pSample->Length), MAX_SAMPLE_LENGTH); // in bytes
		Ins[iSmp].nLoopStart = min(LittleEndian(pSample->LoopBegin), Ins[iSmp].nLength); // in samples
		Ins[iSmp].nLoopEnd = min(LittleEndian(pSample->LoopEnd) - 1, Ins[iSmp].nLength); // dito
		FrequencyToTranspose(&Ins[iSmp]); // set transpose + finetune for mod files

		// fix transpose + finetune for some rare cases where transpose is not C-5 (e.g. sample 4 in wander2.mod)
		if(m_nType == MOD_TYPE_MOD)
		{
			while(Ins[iSmp].RelativeTone != 0)
			{
				if(Ins[iSmp].RelativeTone > 0)
				{
					Ins[iSmp].RelativeTone -= 1;
					Ins[iSmp].nFineTune += 128;
				}
				else
				{
					Ins[iSmp].RelativeTone += 1;
					Ins[iSmp].nFineTune -= 128;
				}
			}
		}

		if(pSample->Flags & 0x01) Ins[iSmp].uFlags |= CHN_LOOP; // loop sample

		if(pSample->Flags & 0x04)
		{
			Ins[iSmp].nVolume = min(pSample->Volume << 2, 256); // 0...64, 255 = no default volume
		}
		else
		{
			Ins[iSmp].nVolume = 256; // default volume
		}

		if(pSample->Flags & 0x08) // default panning is used
		{
			Ins[iSmp].uFlags |= CHN_PANNING;
			Ins[iSmp].nPan = (pSample->Pan > 15) ? 128 : min((pSample->Pan << 4) + 8, 256); // 0...15, 16 = surround (not supported), 255 = no default panning
		}
		else
		{
			Ins[iSmp].nPan = 128;
		}

		/* note: apparently (and according to zilym), 2GDM doesn't handle 16 bit or stereo samples properly.
		   so those flags are pretty much meaningless and we will ignore them... in fact, samples won't load as expected if we don't! */
		
		UINT iSampleFormat;
		if(pSample->Flags & 0x02) // 16 bit
		{
			if(pSample->Flags & 0x20) // stereo
			{
				iSampleFormat = RS_PCM16U; // should be RS_STPCM16U but that breaks the sample reader
			}
			else
			{
				iSampleFormat = RS_PCM16U;
			}
		}
		else // 8 bit
		{
			if(pSample->Flags & 0x20) // stereo
			{
				iSampleFormat = RS_PCM8U; // should be RS_STPCM8U - dito
			}
			else
			{
				iSampleFormat = RS_PCM8U;
			}
		}

		// according to zilym, LZW support has never been finished, so this is also practically useless.
		if(pSample->Flags & 0x10)
			iLZWsamples++;

		// read sample data
		ReadSample(&Ins[iSmp], iSampleFormat, reinterpret_cast<LPCSTR>(lpStream + iSampleOffset), dwMemLength - iSampleOffset);
		iSampleOffset += min(LittleEndian(pSample->Length), dwMemLength - iSampleOffset);

	}

	// read patterns
	Patterns.ResizeArray(max(MAX_PATTERNS, pHeader->NOP + 1));

	BOOL bS3MCommandSet = (GetBestSaveFormat() & (MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_MPT));

	// we'll start at position iPatternsOffset and decode all patterns
	for (PATTERNINDEX iPat = 0; iPat < pHeader->NOP + 1; iPat++)
	{
		
		if(iPatternsOffset + 2 > dwMemLength) break;
		UINT16 iPatternLength = LittleEndianW(*(UINT16 *)(lpStream + iPatternsOffset)); // pattern length including the two "length" bytes
		if(iPatternLength > dwMemLength || iPatternsOffset > dwMemLength - iPatternLength) break;

		if(Patterns.Insert(iPat, 64)) 
		{
			CString s;
			s.Format(TEXT("Allocating patterns failed starting from pattern %u"), iPat);
			MessageBox(NULL, s, TEXT("OpenMPT GDM import"), MB_ICONERROR);
			break;
		}

		// position in THIS pattern
		DWORD iPatternPos = iPatternsOffset + 2;

		MODCOMMAND *p = Patterns[iPat];

		for(UINT iRow = 0; iRow < 64; iRow++)
		{
			do // zero byte = next row
			{
				if(iPatternPos + 1 > dwMemLength) break;

				BYTE bChannel = lpStream[iPatternPos++];

				if(bChannel == 0) break; // next row, please!

				UINT channel = bChannel & 0x1f;
				if(channel >= m_nChannels) break; // better safe than sorry!

				MODCOMMAND *m = &p[iRow * m_nChannels + channel];

				if(bChannel & 0x20)
				{
					// note and sample follows
					if(iPatternPos + 2 > dwMemLength) break;
					BYTE bNote = lpStream[iPatternPos++];
					BYTE bSample = lpStream[iPatternPos++];

					bNote = (bNote & 0x7F) - 1; // this format doesn't have note cuts
					if(bNote < 0xF0) bNote = (bNote & 0x0F) + 12 * (bNote >> 4) + 13;
					if(bNote == 0xFF) bNote = NOTE_NONE;
					m->note = bNote;
					m->instr = bSample;

				}

				if(bChannel & 0x40)
				{
					// effect(s) follow

					m->command = CMD_NONE;
					m->volcmd = CMD_NONE;

					do
					{
						if(iPatternPos + 2 > dwMemLength) break;
						BYTE bEffect = lpStream[iPatternPos++];
						BYTE bEffectData = lpStream[iPatternPos++];

						BYTE command = bEffect & 0x1F, param = bEffectData;
						BYTE volcommand = CMD_NONE, volparam = param;

						switch(command)
						{
						case 0x01: command = CMD_PORTAMENTOUP; if(param >= 0xE0) param = 0xDF; break;
						case 0x02: command = CMD_PORTAMENTODOWN; if(param >= 0xE0) param = 0xDF; break;
						case 0x03: command = CMD_TONEPORTAMENTO; break;
						case 0x04: command = CMD_VIBRATO; break;
						case 0x05: command = CMD_TONEPORTAVOL; if (param & 0xF0) param &= 0xF0; break;
						case 0x06: command = CMD_VIBRATOVOL; if (param & 0xF0) param &= 0xF0; break;
						case 0x07: command = CMD_TREMOLO; break;
						case 0x08: command = CMD_TREMOR; break;
						case 0x09: command = CMD_OFFSET; break;
						case 0x0A: command = CMD_VOLUMESLIDE; break;
						case 0x0B: command = CMD_POSITIONJUMP; break;
						case 0x0C: 
							if(bS3MCommandSet)
							{
								command = CMD_NONE;
								volcommand = VOLCMD_VOLUME;
								volparam = min(param, 64);
							}
							else
							{
								command = CMD_VOLUME;
								param = min(param, 64);
							}
							break;
						case 0x0D: command = CMD_PATTERNBREAK; break;
						case 0x0E: 
							if(bS3MCommandSet)
							{
								command = CMD_S3MCMDEX;
								// need to do some remapping
								switch(param >> 4)
								{
								case 0x0:
									// set filter
									command = CMD_MODCMDEX;
									break;
								case 0x1:
									// fine porta up
									command = CMD_PORTAMENTOUP;
									param = 0xF0 | (param & 0x0F);
									break;
								case 0x2:
									// fine porta down
									command = CMD_PORTAMENTODOWN;
									param = 0xF0 | (param & 0x0F);
									break;
								case 0x3:
									// glissando control
									param = 0x10 | (param & 0x0F);
									break;
								case 0x4:
									// vibrato waveform
									param = 0x30 | (param & 0x0F);
									break;
								case 0x5:
									// set finetune
									param = 0x20 | (param & 0x0F);
									break;
								case 0x6:
									// pattern loop
									param = 0xB0 | (param & 0x0F);
									break;
								case 0x7:
									// tremolo waveform
									param = 0x40 | (param & 0x0F);
									break;
								case 0x8:
									// extra fine porta up
									command = CMD_PORTAMENTOUP;
									param = 0xE0 | (param & 0x0F);
									break;
								case 0x9:
									// extra fine porta down
									command = CMD_PORTAMENTODOWN;
									param = 0xE0 | (param & 0x0F);
									break;
								case 0xA:
									// fine volume up
									command = CMD_VOLUMESLIDE;
									param = ((param & 0x0F) << 4) | 0x0F;
									break;
								case 0xB:
									// fine volume down
									command = CMD_VOLUMESLIDE;
									param = 0xF0 | (param & 0x0F);
									break;
								case 0xC:
									// note cut
									break;
								case 0xD:
									// note delay
									break;
								case 0xE:
									// pattern delay
									break;
								case 0xF:
									command = CMD_MODCMDEX;
									// invert loop / funk repeat
									break;
								}
							}
							else
							{
								command = CMD_MODCMDEX;
							}
							break;
						case 0x0F: command = CMD_SPEED; break;
						case 0x10: command = CMD_ARPEGGIO; break;
						case 0x11: command = CMD_NONE /* set internal flag */; break;
						case 0x12:
							if((!bS3MCommandSet) && ((param & 0xF0) == 0))
							{
								// retrig in "mod style"
								command = CMD_MODCMDEX;
								param = 0x90 | (param & 0x0F);
							}
							else
							{
								// either "s3m style" is required or this format is like s3m anyway
								command = CMD_RETRIG;
							}
							break;
						case 0x13: command = CMD_GLOBALVOLUME; break;
						case 0x14: command = CMD_FINEVIBRATO; break;
						case 0x1E:
							switch(param >> 4)
							{
							case 0x0:
								switch(param & 0x0F)
								{
								case 0x0: command = CMD_S3MCMDEX; param = 0x90; break;
								case 0x1: command = CMD_PANNING8; param = 0xA4; break;
								case 0x2: command = CMD_NONE /* set normal loop - not implemented in 2GDM */; break;
								case 0x3: command = CMD_NONE /* set bidi loop - dito */; break;
								case 0x4: command = CMD_S3MCMDEX; param = 0x9E; break;
								case 0x5: command = CMD_S3MCMDEX; param = 0x9F; break;
								case 0x6: command = CMD_NONE /* monaural sample - dito */; break;
								case 0x7: command = CMD_NONE /* stereo sample - dito */; break;
								case 0x8: command = CMD_NONE /* stop sample on end - dito */; break;
								case 0x9: command = CMD_NONE /* loop sample on end - dito */; break;
								default: command = CMD_NONE; break;
								}
								break;
							case 0x8:
								command = (bS3MCommandSet) ? CMD_S3MCMDEX : CMD_MODCMDEX;
								break;
							case 0xD:
								// adjust frequency (increment in hz) - not implemented in 2GDM
								command = CMD_NONE;
								break;
							default: command = CMD_NONE; break;
							}
							break;
						case 0x1F: command = CMD_TEMPO; break;
						default: command = CMD_NONE; break;
						}

						if(command != CMD_NONE)
						{
							// move pannings to volume column - should never happen
							if(m->command == CMD_S3MCMDEX && ((m->param >> 4) == 0x8) && volcommand == CMD_NONE)
							{
								volcommand = VOLCMD_PANNING;
								volparam = ((param & 0x0F) << 2) + 2;
							}

							m->command = command;
							m->param = param;
						}
						if(volcommand != CMD_NONE)
						{
							m->volcmd = volcommand;
							m->vol = volparam;
						}

						if(!(bEffect & 0x20)) break; // no other effect follows
					} while(1);

				}
				
			} while(1);
		}

		iPatternsOffset += iPatternLength;
	}

	// read song comments	
	if(iMTLength)
	{
		m_lpszSongComments = new char[iMTLength + 1];
		if (m_lpszSongComments)
		{
			memset(m_lpszSongComments, 0, iMTLength + 1);
			memcpy(m_lpszSongComments, lpStream + iMTOffset, iMTLength);
		}
	}

	if(iLZWsamples)
	{
		TCHAR s[128];
		wsprintf(s, TEXT("%d samples are LZW compressed. LZW compression is currently not supported.\nPlease send this file to the OpenMPT team."), iLZWsamples);
		::MessageBox(0, s, TEXT("OpenMPT GDM import"), MB_ICONWARNING);
	}
	
	return true;

}
