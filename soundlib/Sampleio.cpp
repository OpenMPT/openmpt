/*
 * OpenMPT
 *
 * Sampleio.cpp
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          OpenMPT devs
*/

#include "stdafx.h"
#include "sndfile.h"
#include "it_defs.h"
#include "wavConverter.h"

#pragma warning(disable:4244)

BOOL CSoundFile::ReadSampleFromFile(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//-------------------------------------------------------------------------------------
{
	if ((!nSample) || (nSample >= MAX_SAMPLES)) return FALSE;
	if ((!ReadWAVSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadXISample(nSample, lpMemFile, dwFileLength))
	 && (!ReadAIFFSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadITSSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadPATSample(nSample, lpMemFile, dwFileLength))
	 && (!Read8SVXSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadS3ISample(nSample, lpMemFile, dwFileLength)))
		return FALSE;
	return TRUE;
}


BOOL CSoundFile::ReadInstrumentFromFile(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//----------------------------------------------------------------------------------------
{
	if ((!nInstr) || (nInstr >= MAX_INSTRUMENTS)) return FALSE;
	if ((!ReadXIInstrument(nInstr, lpMemFile, dwFileLength))
	 && (!ReadPATInstrument(nInstr, lpMemFile, dwFileLength))
	 && (!ReadITIInstrument(nInstr, lpMemFile, dwFileLength))
	// Generic read
	 && (!ReadSampleAsInstrument(nInstr, lpMemFile, dwFileLength))) return FALSE;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
	return TRUE;
}


BOOL CSoundFile::ReadSampleAsInstrument(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//----------------------------------------------------------------------------------------
{
	LPDWORD psig = (LPDWORD)lpMemFile;
	if ((!lpMemFile) || (dwFileLength < 128)) return FALSE;
	if (((psig[0] == 0x46464952) && (psig[2] == 0x45564157))	// RIFF....WAVE signature
	 || ((psig[0] == 0x5453494C) && (psig[2] == 0x65766177))	// LIST....wave
	 || (psig[76/4] == 0x53524353)								// S3I signature
	 || ((psig[0] == 0x4D524F46) && (psig[2] == 0x46464941))	// AIFF signature
	 || ((psig[0] == 0x4D524F46) && (psig[2] == 0x58565338))	// 8SVX signature
	 || (psig[0] == 0x53504D49)									// ITS signature
	)
	{
		// Loading Instrument
		INSTRUMENTHEADER *penv = new INSTRUMENTHEADER;
		if (!penv) return FALSE;
		memset(penv, 0, sizeof(INSTRUMENTHEADER));
		penv->pTuning = penv->s_DefaultTuning;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//		RemoveInstrumentSamples(nInstr);
		DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
		Headers[nInstr] = penv;
		// Scanning free sample
		UINT nSample = 0;
		for (UINT iscan=1; iscan<MAX_SAMPLES; iscan++)
		{
			if ((!Ins[iscan].pSample) && (!m_szNames[iscan][0]))
			{
				nSample = iscan;
				if (nSample > m_nSamples) m_nSamples = nSample;
				break;
			}
		}
		// Default values
		penv->nFadeOut = 1024;
		penv->nGlobalVol = 64;
		penv->nPan = 128;
		penv->nPPC = 5*12;
		SetDefaultInstrumentValues(penv);
		for (UINT iinit=0; iinit<128; iinit++)
		{
			penv->Keyboard[iinit] = nSample;
			penv->NoteMap[iinit] = iinit+1;
		}
		if (nSample) ReadSampleFromFile(nSample, lpMemFile, dwFileLength);
		return TRUE;
	}
	return FALSE;
}


// -> CODE#0003
// -> DESC="remove instrument's samples"
// BOOL CSoundFile::DestroyInstrument(UINT nInstr)
BOOL CSoundFile::DestroyInstrument(UINT nInstr, char removeSamples)
// -! BEHAVIOUR_CHANGE#0003
//---------------------------------------------
{
	if ((!nInstr) || (nInstr > m_nInstruments)) return FALSE;
	if (!Headers[nInstr]) return TRUE;

// -> CODE#0003
// -> DESC="remove instrument's samples"
	//rewbs: changed message
	if(removeSamples > 0 || (removeSamples == 0 && ::MessageBox(NULL, "Remove samples associated with an instrument if they are unused?", "Removing instrument", MB_YESNO | MB_ICONQUESTION) == IDYES))
		RemoveInstrumentSamples(nInstr);
// -! BEHAVIOUR_CHANGE#0003

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_szInstrumentPath[nInstr-1][0] = '\0';
	instrumentModified[nInstr-1] = FALSE;
// -! NEW_FEATURE#0023

	INSTRUMENTHEADER *penv = Headers[nInstr];
	Headers[nInstr] = NULL;
	for (UINT i=0; i<MAX_CHANNELS; i++)
	{
		if (Chn[i].pHeader == penv)
		{
			Chn[i].pHeader = NULL;
		}
	}
	delete penv;
	return TRUE;
}


BOOL CSoundFile::IsSampleUsed(UINT nSample)
//-----------------------------------------
{
	if ((!nSample) || (nSample > m_nSamples)) return FALSE;
	if (m_nInstruments)
	{
		for (UINT i=1; i<=m_nInstruments; i++) if (Headers[i])
		{
			INSTRUMENTHEADER *penv = Headers[i];
			for (UINT j=0; j<128; j++)
			{
				if (penv->Keyboard[j] == nSample) return TRUE;
			}
		}
	} else
	{
		for (UINT i=0; i<Patterns.Size(); i++) if (Patterns[i])
		{
			MODCOMMAND *m = Patterns[i];
			for (UINT j=m_nChannels*PatternSize[i]; j; m++, j--)
			{
				if (m->instr == nSample) return TRUE;
			}
		}
	}
	return FALSE;
}


BOOL CSoundFile::IsInstrumentUsed(UINT nInstr)
//--------------------------------------------
{
	if ((!nInstr) || (nInstr > m_nInstruments) || (!Headers[nInstr])) return FALSE;
	for (UINT i=0; i<Patterns.Size(); i++) if (Patterns[i])
	{
		MODCOMMAND *m = Patterns[i];
		for (UINT j=m_nChannels*PatternSize[i]; j; m++, j--)
		{
			if (m->instr == nInstr) return TRUE;
		}
	}
	return FALSE;
}


// Removing all unused samples
BOOL CSoundFile::RemoveInstrumentSamples(UINT nInstr)
//---------------------------------------------------
{
	BYTE sampleused[MAX_SAMPLES/8];
	
	memset(sampleused, 0, sizeof(sampleused));
	if (Headers[nInstr])
	{
		INSTRUMENTHEADER *p = Headers[nInstr];
		for (UINT r=0; r<128; r++)
		{
			UINT n = p->Keyboard[r];
			if (n < MAX_SAMPLES) sampleused[n>>3] |= (1<<(n&7));
		}
		for (UINT smp=1; smp<MAX_INSTRUMENTS; smp++) if ((Headers[smp]) && (smp != nInstr))
		{
			p = Headers[smp];
			for (UINT r=0; r<128; r++)
			{
				UINT n = p->Keyboard[r];
				if (n < MAX_SAMPLES) sampleused[n>>3] &= ~(1<<(n&7));
			}
		}
		for (UINT d=1; d<=m_nSamples; d++) if (sampleused[d>>3] & (1<<(d&7)))
		{
			DestroySample(d);
			m_szNames[d][0] = 0;
		}
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// I/O From another song
//

BOOL CSoundFile::ReadInstrumentFromSong(UINT nInstr, CSoundFile *pSrcSong, UINT nSrcInstr)
//----------------------------------------------------------------------------------------
{
	if ((!pSrcSong) || (!nSrcInstr) || (nSrcInstr > pSrcSong->m_nInstruments)
	 || (nInstr >= MAX_INSTRUMENTS) || (!pSrcSong->Headers[nSrcInstr])) return FALSE;
	if (m_nInstruments < nInstr) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
	if (!Headers[nInstr]) Headers[nInstr] = new INSTRUMENTHEADER;
	INSTRUMENTHEADER *penv = Headers[nInstr];
	if (penv)
	{
		WORD samplemap[32];
		WORD samplesrc[32];
		UINT nSamples = 0;
		UINT nsmp = 1;
		*penv = *pSrcSong->Headers[nSrcInstr];
		for (UINT i=0; i<128; i++)
		{
			UINT n = penv->Keyboard[i];
			if ((n) && (n <= pSrcSong->m_nSamples) && (i < NOTE_MAX))
			{
				UINT j = 0;
				for (j=0; j<nSamples; j++)
				{
					if (samplesrc[j] == n) break;
				}
				if (j >= nSamples)
				{
					while ((nsmp < MAX_SAMPLES) && ((Ins[nsmp].pSample) || (m_szNames[nsmp][0]))) nsmp++;
					if ((nSamples < 32) && (nsmp < MAX_SAMPLES))
					{
						samplesrc[nSamples] = (WORD)n;
						samplemap[nSamples] = (WORD)nsmp;
						nSamples++;
						penv->Keyboard[i] = (WORD)nsmp;
						if (m_nSamples < nsmp) m_nSamples = nsmp;
						nsmp++;
					} else
					{
						penv->Keyboard[i] = 0;
					}
				} else
				{
					penv->Keyboard[i] = samplemap[j];
				}
			} else
			{
				penv->Keyboard[i] = 0;
			}
		}
		// Load Samples
		for (UINT k=0; k<nSamples; k++)
		{
			ReadSampleFromSong(samplemap[k], pSrcSong, samplesrc[k]);
		}
		return TRUE;
	}
	return FALSE;
}


BOOL CSoundFile::ReadSampleFromSong(UINT nSample, CSoundFile *pSrcSong, UINT nSrcSample)
//--------------------------------------------------------------------------------------
{
	if ((!pSrcSong) || (!nSrcSample) || (nSrcSample > pSrcSong->m_nSamples) || (nSample >= MAX_SAMPLES)) return FALSE;
	MODINSTRUMENT *psmp = &pSrcSong->Ins[nSrcSample];
	UINT nSize = psmp->nLength;
	if (psmp->uFlags & CHN_16BIT) nSize *= 2;
	if (psmp->uFlags & CHN_STEREO) nSize *= 2;
	if (m_nSamples < nSample) m_nSamples = nSample;
	if (Ins[nSample].pSample)
	{
		Ins[nSample].nLength = 0;
		FreeSample(Ins[nSample].pSample);
	}
	Ins[nSample] = *psmp;
	if (psmp->pSample)
	{
		Ins[nSample].pSample = AllocateSample(nSize+8);
		if (Ins[nSample].pSample)
		{
			memcpy(Ins[nSample].pSample, psmp->pSample, nSize);
			AdjustSampleLoop(&Ins[nSample]);
		}
	}
	if ((!(m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))) && (pSrcSong->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)))
	{
		MODINSTRUMENT *pins = &Ins[nSample];
		pins->nC4Speed = TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
		pins->RelativeTone = 0;
		pins->nFineTune = 0;
	} else
	if ((m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) && (!(pSrcSong->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))))
	{
		FrequencyToTranspose(&Ins[nSample]);
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
// WAV Open

#define IFFID_pcm	0x206d6370
#define IFFID_fact	0x74636166

extern BOOL IMAADPCMUnpack16(signed short *pdest, UINT nLen, LPBYTE psrc, DWORD dwBytes, UINT pkBlkAlign);

BOOL CSoundFile::ReadWAVSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD *pdwWSMPOffset)
//------------------------------------------------------------------------------------------------------
{
	DWORD dwMemPos = 0, dwDataPos;
	WAVEFILEHEADER *phdr = (WAVEFILEHEADER *)lpMemFile;
	WAVEFORMATHEADER *pfmt, *pfmtpk;
	WAVEDATAHEADER *pdata;
	WAVESMPLHEADER *psh;
	WAVEEXTRAHEADER *pxh;
	DWORD dwInfoList, dwFact, dwSamplesPerBlock;
	
	if ((!nSample) || (!lpMemFile) || (dwFileLength < (DWORD)(sizeof(WAVEFILEHEADER)+sizeof(WAVEFORMATHEADER)))) return FALSE;
	if (((phdr->id_RIFF != IFFID_RIFF) && (phdr->id_RIFF != IFFID_LIST))
	 || ((phdr->id_WAVE != IFFID_WAVE) && (phdr->id_WAVE != IFFID_wave))) return FALSE;
	dwMemPos = sizeof(WAVEFILEHEADER);
	dwDataPos = 0;
	pfmt = NULL;
	pfmtpk = NULL;
	pdata = NULL;
	psh = NULL;
	pxh = NULL;
	dwSamplesPerBlock = 0;
	dwInfoList = 0;
	dwFact = 0;
	while ((dwMemPos + 8 < dwFileLength) && (dwMemPos < phdr->filesize))
	{
		DWORD dwLen = *((LPDWORD)(lpMemFile+dwMemPos+4));
		DWORD dwIFFID = *((LPDWORD)(lpMemFile+dwMemPos));
		if ((dwLen > dwFileLength) || (dwMemPos+8+dwLen > dwFileLength)) break;
		switch(dwIFFID)
		{
		// "fmt "
		case IFFID_fmt:
			if (pfmt) break;
			if (dwLen+8 >= sizeof(WAVEFORMATHEADER))
			{
				pfmt = (WAVEFORMATHEADER *)(lpMemFile + dwMemPos);
				if (dwLen+8 >= sizeof(WAVEFORMATHEADER)+4)
				{
					dwSamplesPerBlock = *((WORD *)(lpMemFile+dwMemPos+sizeof(WAVEFORMATHEADER)+2));
				}
			}
			break;
		// "pcm "
		case IFFID_pcm:
			if (pfmtpk) break;
			if (dwLen+8 >= sizeof(WAVEFORMATHEADER)) pfmtpk = (WAVEFORMATHEADER *)(lpMemFile + dwMemPos);
			break;
		// "fact"
		case IFFID_fact:
			if (!dwFact) dwFact = *((LPDWORD)(lpMemFile+dwMemPos+8));
			break;
		// "data"
		case IFFID_data:
			if ((dwLen+8 >= sizeof(WAVEDATAHEADER)) && (!pdata))
			{
				pdata = (WAVEDATAHEADER *)(lpMemFile + dwMemPos);
				dwDataPos = dwMemPos + 8;
			}
			break;
		// "xtra"
		case 0x61727478:
			if (dwLen+8 >= sizeof(WAVEEXTRAHEADER)) pxh = (WAVEEXTRAHEADER *)(lpMemFile + dwMemPos);
			break;
		// "smpl"
		case 0x6C706D73:
			if (dwLen+8 >= sizeof(WAVESMPLHEADER)) psh = (WAVESMPLHEADER *)(lpMemFile + dwMemPos);
			break;
		// "LIST"."info"
		case IFFID_LIST:
			if (*((LPDWORD)(lpMemFile+dwMemPos+8)) == 0x4F464E49)	// "INFO"
				dwInfoList = dwMemPos;
			break;
		// "wsmp":
		case IFFID_wsmp:
			if (pdwWSMPOffset) *pdwWSMPOffset = dwMemPos;
			break;
		}
		dwMemPos += dwLen + 8;
	}
	if ((!pdata) || (!pfmt) || (pdata->length < 4)) return FALSE;
	if ((pfmtpk) && (pfmt))
	{
		if (pfmt->format != 1)
		{
			WAVEFORMATHEADER *tmp = pfmt;
			pfmt = pfmtpk;
			pfmtpk = tmp;
			if ((pfmtpk->format != 0x11) || (pfmtpk->bitspersample != 4)
			 || (pfmtpk->channels != 1)) return FALSE;
		} else pfmtpk = NULL;
	}
	// WAVE_FORMAT_PCM, WAVE_FORMAT_IEEE_FLOAT, WAVE_FORMAT_EXTENSIBLE
	if ((((pfmt->format != 1) && (pfmt->format != 0xFFFE))
	 && (pfmt->format != 3 || pfmt->bitspersample != 32)) //Microsoft IEEE FLOAT
	 || (pfmt->channels > 2)
	 || (!pfmt->channels)
	 || (pfmt->bitspersample & 7)
	 || (!pfmt->bitspersample)
	 || (pfmt->bitspersample > 32)
	) return FALSE;

	DestroySample(nSample);
	UINT nType = RS_PCM8U;
	if (pfmt->channels == 1)
	{
		if (pfmt->bitspersample == 24) nType = RS_PCM24S; 
		else if (pfmt->bitspersample == 32) nType = RS_PCM32S; 
			else nType = (pfmt->bitspersample == 16) ? RS_PCM16S : RS_PCM8U;
	} else
	{
		if (pfmt->bitspersample == 24) nType = RS_STIPCM24S; 
		else if (pfmt->bitspersample == 32) nType = RS_STIPCM32S; 
			else nType = (pfmt->bitspersample == 16) ? RS_STIPCM16S : RS_STIPCM8U;
	}
	UINT samplesize = pfmt->channels * (pfmt->bitspersample >> 3);
	MODINSTRUMENT *pins = &Ins[nSample];
	if (pins->pSample)
	{
		FreeSample(pins->pSample);
		pins->pSample = NULL;
		pins->nLength = 0;
	}
	pins->nLength = pdata->length / samplesize;
	pins->nLoopStart = pins->nLoopEnd = 0;
	pins->nSustainStart = pins->nSustainEnd = 0;
	pins->nC4Speed = pfmt->freqHz;
	pins->nPan = 128;
	pins->nVolume = 256;
	pins->nGlobalVol = 64;
	pins->uFlags = (pfmt->bitspersample > 8) ? CHN_16BIT : 0;
	if (m_nType & MOD_TYPE_XM) pins->uFlags |= CHN_PANNING;
	pins->RelativeTone = 0;
	pins->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pins);
	pins->nVibType = pins->nVibSweep = pins->nVibDepth = pins->nVibRate = 0;
	pins->name[0] = 0;
	memset(m_szNames[nSample], 0, 32);
	if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
	// IMA ADPCM 4:1
	if (pfmtpk)
	{
		if (dwFact < 4) dwFact = pdata->length * 2;
		pins->nLength = dwFact;
		pins->pSample = AllocateSample(pins->nLength*2+16);
		IMAADPCMUnpack16((signed short *)pins->pSample, pins->nLength,
						 (LPBYTE)(lpMemFile+dwDataPos), dwFileLength-dwDataPos, pfmtpk->samplesize);
		AdjustSampleLoop(pins);
	} else
	{
		ReadSample(pins, nType, (LPSTR)(lpMemFile+dwDataPos), dwFileLength-dwDataPos, pfmt->format);
	}
	// smpl field
	if (psh)
	{
		pins->nLoopStart = pins->nLoopEnd = 0;
		if ((psh->dwSampleLoops) && (sizeof(WAVESMPLHEADER) + psh->dwSampleLoops * sizeof(SAMPLELOOPSTRUCT) <= psh->smpl_len + 8))
		{
			SAMPLELOOPSTRUCT *psl = (SAMPLELOOPSTRUCT *)(&psh[1]);
			if (psh->dwSampleLoops > 1)
			{
				pins->uFlags |= CHN_LOOP | CHN_SUSTAINLOOP;
				if (psl[0].dwLoopType) pins->uFlags |= CHN_PINGPONGSUSTAIN;
				if (psl[1].dwLoopType) pins->uFlags |= CHN_PINGPONGLOOP;
				pins->nSustainStart = psl[0].dwLoopStart;
				pins->nSustainEnd = psl[0].dwLoopEnd;
				pins->nLoopStart = psl[1].dwLoopStart;
				pins->nLoopEnd = psl[1].dwLoopEnd;
			} else
			{
				pins->uFlags |= CHN_LOOP;
				if (psl->dwLoopType) pins->uFlags |= CHN_PINGPONGLOOP;
				pins->nLoopStart = psl->dwLoopStart;
				pins->nLoopEnd = psl->dwLoopEnd;
			}
			if (pins->nLoopStart >= pins->nLoopEnd) pins->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP);
			if (pins->nSustainStart >= pins->nSustainEnd) pins->uFlags &= ~(CHN_PINGPONGLOOP|CHN_PINGPONGSUSTAIN);
		}
	}
	// LIST field
	if (dwInfoList)
	{
		DWORD dwLSize = *((DWORD *)(lpMemFile+dwInfoList+4)) + 8;
		DWORD d = 12;
		while (d+8 < dwLSize)
		{
			if (!lpMemFile[dwInfoList+d]) d++;
			DWORD id = *((DWORD *)(lpMemFile+dwInfoList+d));
			DWORD len = *((DWORD *)(lpMemFile+dwInfoList+d+4));
			if (id == 0x4D414E49) // "INAM"
			{
				if ((dwInfoList+d+8+len <= dwFileLength) && (len))
				{
					DWORD dwNameLen = len;
					if (dwNameLen > 31) dwNameLen = 31;
					memcpy(m_szNames[nSample], lpMemFile+dwInfoList+d+8, dwNameLen);
					if (phdr->id_RIFF != 0x46464952)
					{
						// DLS sample -> sample filename
						if (dwNameLen > 21) dwNameLen = 21;
						memcpy(pins->name, lpMemFile+dwInfoList+d+8, dwNameLen);
					}
				}
				break;
			}
			d += 8 + len;
		}
	}
	// xtra field
	if (pxh)
	{
		if (m_nType > MOD_TYPE_S3M)
		{
			if (pxh->dwFlags & CHN_PINGPONGLOOP) pins->uFlags |= CHN_PINGPONGLOOP;
			if (pxh->dwFlags & CHN_SUSTAINLOOP) pins->uFlags |= CHN_SUSTAINLOOP;
			if (pxh->dwFlags & CHN_PINGPONGSUSTAIN) pins->uFlags |= CHN_PINGPONGSUSTAIN;
			if (pxh->dwFlags & CHN_PANNING) pins->uFlags |= CHN_PANNING;
		}
		pins->nPan = pxh->wPan;
		pins->nVolume = pxh->wVolume;
		pins->nGlobalVol = pxh->wGlobalVol;
		pins->nVibType = pxh->nVibType;
		pins->nVibSweep = pxh->nVibSweep;
		pins->nVibDepth = pxh->nVibDepth;
		pins->nVibRate = pxh->nVibRate;
		// Name present (clipboard only)
		UINT xtrabytes = pxh->xtra_len + 8 - sizeof(WAVEEXTRAHEADER);
		LPSTR pszTextEx = (LPSTR)(pxh+1); 
		if (xtrabytes >= 32)
		{
			memcpy(m_szNames[nSample], pszTextEx, 31);
			pszTextEx += 32;
			xtrabytes -= 32;
			if (xtrabytes >= 22)
			{
				memcpy(pins->name, pszTextEx, 22);
				xtrabytes -= 22;
			}
		}
	}
	return TRUE;
}


///////////////////////////////////////////////////////////////
// Save WAV

BOOL CSoundFile::SaveWAVSample(UINT nSample, LPCSTR lpszFileName)
//---------------------------------------------------------------
{
	LPCSTR lpszMPT = "Modplug Tracker\0";
	WAVEFILEHEADER header;
	WAVEFORMATHEADER format;
	WAVEDATAHEADER data;
	WAVESAMPLERINFO smpl;
	WAVELISTHEADER list;
	WAVEEXTRAHEADER extra;
	MODINSTRUMENT *pins = &Ins[nSample];
	FILE *f;

	if ((f = fopen(lpszFileName, "wb")) == NULL) return FALSE;
	memset(&extra, 0, sizeof(extra));
	memset(&smpl, 0, sizeof(smpl));
	header.id_RIFF = IFFID_RIFF;
	header.filesize = sizeof(header)+sizeof(format)+sizeof(data)+sizeof(extra)-8;
	header.filesize += 12+8+16+8+32; // LIST(INAM, ISFT)
	header.id_WAVE = IFFID_WAVE;
	format.id_fmt = IFFID_fmt;
	format.hdrlen = 16;
	format.format = 1;
	format.freqHz = pins->nC4Speed;
	if (m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) format.freqHz = TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
	format.channels = (pins->uFlags & CHN_STEREO) ? 2 : 1;
	format.bitspersample = (pins->uFlags & CHN_16BIT) ? 16 : 8;
	format.samplesize = (format.channels*format.bitspersample)>>3;
	format.bytessec = format.freqHz*format.samplesize;
	data.id_data = IFFID_data;
	UINT nType;
	data.length = pins->nLength;
	if (pins->uFlags & CHN_STEREO)
	{
		nType = (pins->uFlags & CHN_16BIT) ? RS_STIPCM16S : RS_STIPCM8U;
		data.length *= 2;
	} else
	{
		nType = (pins->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8U;
	}
	if (pins->uFlags & CHN_16BIT) data.length *= 2;
	header.filesize += data.length;
	fwrite(&header, 1, sizeof(header), f);
	fwrite(&format, 1, sizeof(format), f);
	fwrite(&data, 1, sizeof(data), f);
	WriteSample(f, pins, nType);
	// "smpl" field
	smpl.wsiHdr.smpl_id = 0x6C706D73;
	smpl.wsiHdr.smpl_len = sizeof(WAVESMPLHEADER) - 8;
	smpl.wsiHdr.dwSamplePeriod = 22675;
	if (pins->nC4Speed >= 256) smpl.wsiHdr.dwSamplePeriod = 1000000000 / pins->nC4Speed;
	smpl.wsiHdr.dwBaseNote = 60;
	if (pins->uFlags & (CHN_LOOP|CHN_SUSTAINLOOP))
	{
		if (pins->uFlags & CHN_SUSTAINLOOP)
		{
			smpl.wsiHdr.dwSampleLoops = 2;
			smpl.wsiLoops[0].dwLoopType = (pins->uFlags & CHN_PINGPONGSUSTAIN) ? 1 : 0;
			smpl.wsiLoops[0].dwLoopStart = pins->nSustainStart;
			smpl.wsiLoops[0].dwLoopEnd = pins->nSustainEnd;
			smpl.wsiLoops[1].dwLoopType = (pins->uFlags & CHN_PINGPONGLOOP) ? 1 : 0;
			smpl.wsiLoops[1].dwLoopStart = pins->nLoopStart;
			smpl.wsiLoops[1].dwLoopEnd = pins->nLoopEnd;
		} else
		{
			smpl.wsiHdr.dwSampleLoops = 1;
			smpl.wsiLoops[0].dwLoopType = (pins->uFlags & CHN_PINGPONGLOOP) ? 1 : 0;
			smpl.wsiLoops[0].dwLoopStart = pins->nLoopStart;
			smpl.wsiLoops[0].dwLoopEnd = pins->nLoopEnd;
		}
		smpl.wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT) * smpl.wsiHdr.dwSampleLoops;
	}
	fwrite(&smpl, 1, smpl.wsiHdr.smpl_len + 8, f);
	// "LIST" field
	list.list_id = IFFID_LIST;
	list.list_len = sizeof(list) - 8	// LIST
					+ 8 + 32			// "INAM".dwLen.szSampleName
					+ 8 + 16;			// "ISFT".dwLen."ModPlug Tracker".0
	list.info = IFFID_INFO;
	fwrite(&list, 1, sizeof(list), f);
	list.list_id = IFFID_INAM;			// "INAM"
	list.list_len = 32;
	fwrite(&list, 1, 8, f);
	fwrite(m_szNames[nSample], 1, 32, f);
	list.list_id = IFFID_ISFT;			// "ISFT"
	list.list_len = 16;
	fwrite(&list, 1, 8, f);
	fwrite(lpszMPT, 1, list.list_len, f);
	// "xtra" field
	extra.xtra_id = IFFID_xtra;
	extra.xtra_len = sizeof(extra) - 8;
	extra.dwFlags = pins->uFlags;
	extra.wPan = pins->nPan;
	extra.wVolume = pins->nVolume;
	extra.wGlobalVol = pins->nGlobalVol;
	extra.wReserved = 0;
	extra.nVibType = pins->nVibType;
	extra.nVibSweep = pins->nVibSweep;
	extra.nVibDepth = pins->nVibDepth;
	extra.nVibRate = pins->nVibRate;
	fwrite(&extra, 1, sizeof(extra), f);
	fclose(f);
	return TRUE;
}

///////////////////////////////////////////////////////////////
// Save RAW

BOOL CSoundFile::SaveRAWSample(UINT nSample, LPCSTR lpszFileName)
//---------------------------------------------------------------
{
	MODINSTRUMENT *pins = &Ins[nSample];
	FILE *f;

	if ((f = fopen(lpszFileName, "wb")) == NULL) return FALSE;

	UINT nType;
	if (pins->uFlags & CHN_STEREO)
		nType = (pins->uFlags & CHN_16BIT) ? RS_STIPCM16S : RS_STIPCM8S;
	else
		nType = (pins->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
	WriteSample(f, pins, nType);
	fclose(f);
	return TRUE;
}

/////////////////////////////////////////////////////////////
// GUS Patches

#pragma pack(1)

typedef struct GF1PATCHFILEHEADER
{
	DWORD gf1p;				// "GF1P"
	DWORD atch;				// "ATCH"
	CHAR version[4];		// "100", or "110"
	CHAR id[10];			// "ID#000002"
	CHAR copyright[60];		// Copyright
	BYTE instrum;			// Number of instruments in patch
	BYTE voices;			// Number of voices, usually 14
	BYTE channels;			// Number of wav channels that can be played concurently to the patch
	WORD samples;			// Total number of waveforms for all the .PAT
	WORD volume;			// Master volume
	DWORD data_size;
	BYTE reserved2[36];
} GF1PATCHFILEHEADER;


typedef struct GF1INSTRUMENT
{
	WORD id;				// Instrument id: 0-65535
	CHAR name[16];			// Name of instrument. Gravis doesn't seem to use it
	DWORD size;				// Number of bytes for the instrument with header. (To skip to next instrument)
	BYTE layers;			// Number of layers in instrument: 1-4
	BYTE reserved[40];
} GF1INSTRUMENT;


typedef struct GF1SAMPLEHEADER
{
	CHAR name[7];			// null terminated string. name of the wave.
	BYTE fractions;			// Start loop point fraction in 4 bits + End loop point fraction in the 4 other bits.
	DWORD length;			// total size of wavesample. limited to 65535 now by the drivers, not the card.
	DWORD loopstart;		// start loop position in the wavesample
	DWORD loopend;			// end loop position in the wavesample
	WORD freq;				// Rate at which the wavesample has been sampled
	DWORD low_freq, high_freq, root_freq;	// check note.h for the correspondance.
	SHORT finetune;			// fine tune. -512 to +512, EXCLUDING 0 cause it is a multiplier. 512 is one octave off, and 1 is a neutral value
	BYTE balance;			// Balance: 0-15. 0=full left, 15 = full right
	BYTE env_rate[6];		// attack rates
	BYTE env_volume[6];		// attack volumes
	BYTE tremolo_sweep, tremolo_rate, tremolo_depth;
	BYTE vibrato_sweep, vibrato_rate, vibrato_depth;
	BYTE flags;
	SHORT scale_frequency;
	WORD scale_factor;
	BYTE reserved[36];
} GF1SAMPLEHEADER;

// -- GF1 Envelopes --
//
//	It can be represented like this (the enveloppe is totally bogus, it is
//	just to show the concept):
//						  
//	|                               
//	|           /----`               | |
//	|   /------/      `\         | | | | |
//	|  /                 \       | | | | |
//	| /                    \     | | | | |
//	|/                       \   | | | | |
//	---------------------------- | | | | | |
//	<---> attack rate 0          0 1 2 3 4 5 amplitudes
//	     <----> attack rate 1
//		     <> attack rate 2
//			 <--> attack rate 3
//			     <> attack rate 4
//				 <-----> attack rate 5
//
// -- GF1 Flags --
//
// bit 0: 8/16 bit
// bit 1: Signed/Unsigned
// bit 2: off/on looping
// bit 3: off/on bidirectionnal looping
// bit 4: off/on backward looping
// bit 5: off/on sustaining (3rd point in env.)
// bit 6: off/on enveloppes
// bit 7: off/on clamped release (6th point, env)

typedef struct GF1LAYER
{
	BYTE previous;			// If !=0 the wavesample to use is from the previous layer. The waveheader is still needed
	BYTE id;				// Layer id: 0-3
	DWORD size;				// data size in bytes in the layer, without the header. to skip to next layer for example:
	BYTE samples;			// number of wavesamples
	BYTE reserved[40];
} GF1LAYER;

#pragma pack()

// returns 12*Log2(nFreq/2044)
LONG PatchFreqToNote(ULONG nFreq)
//-------------------------------
{
	const float k_base = 1.0f / 2044.0f;
	const float k_12 = 12;
	LONG result;
	if (nFreq < 1) return 0;
	_asm {
	fld k_12
	fild nFreq
	fld k_base
	fmulp ST(1), ST(0)
	fyl2x
	fistp result
	}
	return result;
}


VOID PatchToSample(CSoundFile *that, UINT nSample, LPBYTE lpStream, DWORD dwMemLength)
//------------------------------------------------------------------------------------
{
	MODINSTRUMENT *pIns = &that->Ins[nSample];
	DWORD dwMemPos = sizeof(GF1SAMPLEHEADER);
	GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream);
	UINT nSmpType;
	
	if (dwMemLength < sizeof(GF1SAMPLEHEADER)) return;
	if (psh->name[0])
	{
		memcpy(that->m_szNames[nSample], psh->name, 7);
		that->m_szNames[nSample][7] = 0;
	}
	pIns->name[0] = 0;
	pIns->nGlobalVol = 64;
	pIns->uFlags = (psh->flags & 1) ? CHN_16BIT : 0;
	if (psh->flags & 4) pIns->uFlags |= CHN_LOOP;
	if (psh->flags & 8) pIns->uFlags |= CHN_PINGPONGLOOP;
	pIns->nLength = psh->length;
	pIns->nLoopStart = psh->loopstart;
	pIns->nLoopEnd = psh->loopend;
	pIns->nC4Speed = psh->freq;
	pIns->RelativeTone = 0;
	pIns->nFineTune = 0;
	pIns->nVolume = 256;
	pIns->nPan = (psh->balance << 4) + 8;
	if (pIns->nPan > 256) pIns->nPan = 128;
	pIns->nVibType = 0;
	pIns->nVibSweep = psh->vibrato_sweep;
	pIns->nVibDepth = psh->vibrato_depth;
	pIns->nVibRate = psh->vibrato_rate/4;
	that->FrequencyToTranspose(pIns);
	pIns->RelativeTone += 84 - PatchFreqToNote(psh->root_freq);
	if (psh->scale_factor) pIns->RelativeTone -= psh->scale_frequency - 60;
	pIns->nC4Speed = that->TransposeToFrequency(pIns->RelativeTone, pIns->nFineTune);
	if (pIns->uFlags & CHN_16BIT)
	{
		nSmpType = (psh->flags & 2) ? RS_PCM16U : RS_PCM16S;
		pIns->nLength >>= 1;
		pIns->nLoopStart >>= 1;
		pIns->nLoopEnd >>= 1;
	} else
	{
		nSmpType = (psh->flags & 2) ? RS_PCM8U : RS_PCM8S;
	}
	that->ReadSample(pIns, nSmpType, (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
}


BOOL CSoundFile::ReadPATSample(UINT nSample, LPBYTE lpStream, DWORD dwMemLength)
//------------------------------------------------------------------------------
{
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pinshdr = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));

	if ((!lpStream) || (dwMemLength < 512)
	 || (phdr->gf1p != 0x50314647) || (phdr->atch != 0x48435441)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0) || (phdr->instrum < 1)
	 || (!phdr->samples) || (!pinshdr->layers)) return FALSE;
	DestroySample(nSample);
	PatchToSample(this, nSample, lpStream+dwMemPos, dwMemLength-dwMemPos);
	if (pinshdr->name[0] > ' ')
	{
		memcpy(m_szNames[nSample], pinshdr->name, 16);
		m_szNames[nSample][16] = 0;
	}
	return TRUE;
}


// PAT Instrument
BOOL CSoundFile::ReadPATInstrument(UINT nInstr, LPBYTE lpStream, DWORD dwMemLength)
//---------------------------------------------------------------------------------
{
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pih = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));
	GF1LAYER *plh = (GF1LAYER *)(lpStream+sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT));
	INSTRUMENTHEADER *penv;
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	UINT nSamples;

	if ((!lpStream) || (dwMemLength < 512)
	 || (phdr->gf1p != 0x50314647) || (phdr->atch != 0x48435441)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0)
	 || (phdr->instrum < 1) || (!phdr->samples)
	 || (!pih->layers) || (!plh->samples)) return FALSE;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
	penv = new INSTRUMENTHEADER;
	if (!penv) return FALSE;
	memset(penv, 0, sizeof(INSTRUMENTHEADER));
	penv->pTuning = penv->s_DefaultTuning;
	Headers[nInstr] = penv;
	nSamples = plh->samples;
	if (nSamples > 16) nSamples = 16;
	memcpy(penv->name, pih->name, 16);
	penv->name[16] = 0;
	penv->nFadeOut = 2048;
	penv->nGlobalVol = 64;
	penv->nPan = 128;
	penv->nPPC = 60;
	penv->nResampling = SRCMODE_DEFAULT;
	penv->nFilterMode = FLTMODE_UNCHANGED;
	if (m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		penv->nNNA = NNA_NOTEOFF;
		penv->nDNA = DNA_NOTEFADE;
	}
	UINT nFreeSmp = 1;
	UINT nMinSmpNote = 0xff;
	UINT nMinSmp = 0;
	for (UINT iSmp=0; iSmp<nSamples; iSmp++)
	{
		// Find a free sample
		while ((nFreeSmp < MAX_SAMPLES) && ((Ins[nFreeSmp].pSample) || (m_szNames[nFreeSmp][0]))) nFreeSmp++;
		if (nFreeSmp >= MAX_SAMPLES) break;
		if (m_nSamples < nFreeSmp) m_nSamples = nFreeSmp;
		if (!nMinSmp) nMinSmp = nFreeSmp;
		// Load it
		GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream+dwMemPos);
		PatchToSample(this, nFreeSmp, lpStream+dwMemPos, dwMemLength-dwMemPos);
		LONG nMinNote = (psh->low_freq > 100) ? PatchFreqToNote(psh->low_freq) : 0;
		LONG nMaxNote = (psh->high_freq > 100) ? PatchFreqToNote(psh->high_freq) : NOTE_MAX;
		LONG nBaseNote = (psh->root_freq > 100) ? PatchFreqToNote(psh->root_freq) : -1;
		if ((!psh->scale_factor) && (nSamples == 1)) { nMinNote = 0; nMaxNote = NOTE_MAX; }
		// Fill Note Map
		for (UINT k=0; k<NOTE_MAX; k++)
		{
			if (((LONG)k == nBaseNote)
			 || ((!penv->Keyboard[k])
			  && ((LONG)k >= nMinNote)
			  && ((LONG)k <= nMaxNote)))
			{
				if (psh->scale_factor)
					penv->NoteMap[k] = (BYTE)(k+1);
				else
					penv->NoteMap[k] = 5*12+1;
				penv->Keyboard[k] = nFreeSmp;
				if (k < nMinSmpNote)
				{
					nMinSmpNote = k;
					nMinSmp = nFreeSmp;
				}
			}
		}
	/*
		// Create dummy envelope
		if (!iSmp)
		{
			penv->dwFlags |= ENV_VOLUME;
			if (psh->flags & 32) penv->dwFlags |= ENV_VOLSUSTAIN;
			penv->VolEnv[0] = 64;
			penv->VolPoints[0] = 0;
			penv->VolEnv[1] = 64;
			penv->VolPoints[1] = 1;
			penv->VolEnv[2] = 32;
			penv->VolPoints[2] = 20;
			penv->VolEnv[3] = 0;
			penv->VolPoints[3] = 100;
			penv->nVolEnv = 4;
		}
	*/
		// Skip to next sample
		dwMemPos += sizeof(GF1SAMPLEHEADER)+psh->length;
		if (dwMemPos + sizeof(GF1SAMPLEHEADER) >= dwMemLength) break;
	}
	if (nMinSmp)
	{
		// Fill note map and missing samples
		for (UINT k=0; k<NOTE_MAX; k++)
		{
			if (!penv->NoteMap[k]) penv->NoteMap[k] = (BYTE)(k+1);
			if (!penv->Keyboard[k])
			{
				penv->Keyboard[k] = nMinSmp;
			} else
			{
				nMinSmp = penv->Keyboard[k];
			}
		}
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////
// S3I Samples

typedef struct S3ISAMPLESTRUCT
{
	BYTE id;
	CHAR filename[12];
	BYTE reserved1;
	WORD offset;
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	BYTE volume;
	BYTE reserved2;
	BYTE pack;
	BYTE flags;
	DWORD C4Speed;
	DWORD reserved3;
	DWORD reserved4;
	DWORD date;
	CHAR name[28];
	DWORD scrs;
} S3ISAMPLESTRUCT;

BOOL CSoundFile::ReadS3ISample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//--------------------------------------------------------------------------------
{
	S3ISAMPLESTRUCT *pss = (S3ISAMPLESTRUCT *)lpMemFile;
	MODINSTRUMENT *pins = &Ins[nSample];
	DWORD dwMemPos;
	UINT flags;

	if ((!lpMemFile) || (dwFileLength < sizeof(S3ISAMPLESTRUCT))
	 || (pss->id != 0x01) || (((DWORD)pss->offset << 4) >= dwFileLength)
	 || (pss->scrs != 0x53524353)) return FALSE;
	DestroySample(nSample);
	dwMemPos = pss->offset << 4;
	memcpy(pins->name, pss->filename, 12);
	memcpy(m_szNames[nSample], pss->name, 28);
	m_szNames[nSample][28] = 0;
	pins->nLength = pss->length;
	pins->nLoopStart = pss->loopstart;
	pins->nLoopEnd = pss->loopend;
	pins->nGlobalVol = 64;
	pins->nVolume = pss->volume << 2;
	pins->uFlags = 0;
	pins->nPan = 128;
	pins->nC4Speed = pss->C4Speed;
	pins->RelativeTone = 0;
	pins->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pins);
	if (pss->flags & 0x01) pins->uFlags |= CHN_LOOP;
	flags = (pss->flags & 0x04) ? RS_PCM16U : RS_PCM8U;
	if (pss->flags & 0x02) flags |= RSF_STEREO;
	ReadSample(pins, flags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
	return TRUE;
}


/////////////////////////////////////////////////////////////
// XI Instruments

typedef struct XIFILEHEADER
{
	CHAR extxi[21];		// Extended Instrument:
	CHAR name[23];		// Name, 1Ah
	CHAR trkname[20];	// FastTracker v2.00
	WORD shsize;		// 0x0102
} XIFILEHEADER;


typedef struct XIINSTRUMENTHEADER
{
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
	WORD reserved2;
} XIINSTRUMENTHEADER;


typedef struct XISAMPLEHEADER
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
} XISAMPLEHEADER;




BOOL CSoundFile::ReadXIInstrument(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//----------------------------------------------------------------------------------
{
	XIFILEHEADER *pxh = (XIFILEHEADER *)lpMemFile;
	XIINSTRUMENTHEADER *pih = (XIINSTRUMENTHEADER *)(lpMemFile+sizeof(XIFILEHEADER));
	UINT samplemap[32];
	UINT sampleflags[32];
	DWORD samplesize[32];
	DWORD dwMemPos = sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER);
	UINT nsamples;

	if ((!lpMemFile) || (dwFileLength < sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER))) return FALSE;
	if (dwMemPos + pxh->shsize - 0x102 >= dwFileLength) return FALSE;
	if (memcmp(pxh->extxi, "Extended Instrument", 19)) return FALSE;
	dwMemPos += pxh->shsize - 0x102;
	if ((dwMemPos < sizeof(XIFILEHEADER)) || (dwMemPos >= dwFileLength)) return FALSE;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
	Headers[nInstr] = new INSTRUMENTHEADER;
	INSTRUMENTHEADER *penv = Headers[nInstr];
	if (!penv) return FALSE;
	memset(penv, 0, sizeof(INSTRUMENTHEADER));
	penv->pTuning = penv->s_DefaultTuning;
	memcpy(penv->name, pxh->name, 22);
	nsamples = 0;
	for (UINT i=0; i<96; i++)
	{
		penv->NoteMap[i+12] = i+1+12;
		if (pih->snum[i] > nsamples) nsamples = pih->snum[i];
	}
	nsamples++;
	if (nsamples > 32) nsamples = 32;
	// Allocate samples
	memset(samplemap, 0, sizeof(samplemap));
	UINT nsmp = 1;
	for (UINT j=0; j<nsamples; j++)
	{
		while ((nsmp < MAX_SAMPLES) && ((Ins[nsmp].pSample) || (m_szNames[nsmp][0]))) nsmp++;
		if (nsmp >= MAX_SAMPLES) break;
		samplemap[j] = nsmp;
		if (m_nSamples < nsmp) m_nSamples = nsmp;
		nsmp++;
	}
	// Setting up instrument header
	for (UINT k=0; k<96; k++)
	{
		UINT n = pih->snum[k];
		if (n < nsamples) penv->Keyboard[k+12] = samplemap[n];
	}
	penv->nFadeOut = pih->volfade;
	if (pih->vtype & 1) penv->dwFlags |= ENV_VOLUME;
	if (pih->vtype & 2) penv->dwFlags |= ENV_VOLSUSTAIN;
	if (pih->vtype & 4) penv->dwFlags |= ENV_VOLLOOP;
	if (pih->ptype & 1) penv->dwFlags |= ENV_PANNING;
	if (pih->ptype & 2) penv->dwFlags |= ENV_PANSUSTAIN;
	if (pih->ptype & 4) penv->dwFlags |= ENV_PANLOOP;
	penv->nVolEnv = pih->vnum;
	penv->nPanEnv = pih->pnum;
	if (penv->nVolEnv > 12) penv->nVolEnv = 12;
	if (penv->nPanEnv > 12) penv->nPanEnv = 12;
	if (!penv->nVolEnv) penv->dwFlags &= ~ENV_VOLUME;
	if (!penv->nPanEnv) penv->dwFlags &= ~ENV_PANNING;
	penv->nVolSustainBegin = pih->vsustain;
	penv->nVolSustainEnd = pih->vsustain;
	if (pih->vsustain >= 12) penv->dwFlags &= ~ENV_VOLSUSTAIN;
	penv->nVolLoopStart = pih->vloops;
	penv->nVolLoopEnd = pih->vloope;
	if (penv->nVolLoopEnd >= 12) penv->nVolLoopEnd = 0;
	if (penv->nVolLoopStart >= penv->nVolLoopEnd) penv->dwFlags &= ~ENV_VOLLOOP;
	penv->nPanSustainBegin = pih->psustain;
	penv->nPanSustainEnd = pih->psustain;
	if (pih->psustain >= 12) penv->dwFlags &= ~ENV_PANSUSTAIN;
	penv->nPanLoopStart = pih->ploops;
	penv->nPanLoopEnd = pih->ploope;
	if (penv->nPanLoopEnd >= 12) penv->nPanLoopEnd = 0;
	if (penv->nPanLoopStart >= penv->nPanLoopEnd) penv->dwFlags &= ~ENV_PANLOOP;
	penv->nGlobalVol = 64;
	penv->nPPC = 5*12;
	SetDefaultInstrumentValues(penv);
	for (UINT ienv=0; ienv<12; ienv++)
	{
		penv->VolPoints[ienv] = (WORD)pih->venv[ienv*2];
		penv->VolEnv[ienv] = (BYTE)pih->venv[ienv*2+1];
		penv->PanPoints[ienv] = (WORD)pih->penv[ienv*2];
		penv->PanEnv[ienv] = (BYTE)pih->penv[ienv*2+1];
		if (ienv)
		{
			if (penv->VolPoints[ienv] < penv->VolPoints[ienv-1])
			{
				penv->VolPoints[ienv] &= 0xFF;
				penv->VolPoints[ienv] += penv->VolPoints[ienv-1] & 0xFF00;
				if (penv->VolPoints[ienv] < penv->VolPoints[ienv-1]) penv->VolPoints[ienv] += 0x100;
			}
			if (penv->PanPoints[ienv] < penv->PanPoints[ienv-1])
			{
				penv->PanPoints[ienv] &= 0xFF;
				penv->PanPoints[ienv] += penv->PanPoints[ienv-1] & 0xFF00;
				if (penv->PanPoints[ienv] < penv->PanPoints[ienv-1]) penv->PanPoints[ienv] += 0x100;
			}
		}
	}
	// Reading samples
	memset(sampleflags, 0, sizeof(sampleflags));
	memset(samplesize, 0, sizeof(samplesize));
	UINT maxsmp = nsamples;
	if ((pih->reserved2 > maxsmp) && (pih->reserved2 <= 32)) maxsmp = pih->reserved2;
	for (UINT ismp=0; ismp<maxsmp; ismp++)
	{
		if (dwMemPos + sizeof(XISAMPLEHEADER) > dwFileLength) break;
		XISAMPLEHEADER *psh = (XISAMPLEHEADER *)(lpMemFile+dwMemPos);
		dwMemPos += sizeof(XISAMPLEHEADER);
		if (ismp >= nsamples) continue;
		sampleflags[ismp] = RS_PCM8S;
		samplesize[ismp] = psh->samplen;
		if (!samplemap[ismp]) continue;
		MODINSTRUMENT *pins = &Ins[samplemap[ismp]];
		pins->uFlags = 0;
		pins->nLength = psh->samplen;
		pins->nLoopStart = psh->loopstart;
		pins->nLoopEnd = psh->loopstart + psh->looplen;
		if (psh->type & 0x10)
		{
			pins->nLength /= 2;
			pins->nLoopStart /= 2;
			pins->nLoopEnd /= 2;
		}
		if (psh->type & 0x20)
		{
			pins->nLength /= 2;
			pins->nLoopStart /= 2;
			pins->nLoopEnd /= 2;
		}
		if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
		if (psh->type & 3) pins->uFlags |= CHN_LOOP;
		if (psh->type & 2) pins->uFlags |= CHN_PINGPONGLOOP;
		if (pins->nLoopEnd > pins->nLength) pins->nLoopEnd = pins->nLength;
		if (pins->nLoopStart >= pins->nLoopEnd)
		{
			pins->uFlags &= ~CHN_LOOP;
			pins->nLoopStart = 0;
		}
		pins->nVolume = psh->vol << 2;
		if (pins->nVolume > 256) pins->nVolume = 256;
		pins->nGlobalVol = 64;
		sampleflags[ismp] = (psh->type & 0x10) ? RS_PCM16D : RS_PCM8D;
		if (psh->type & 0x20) sampleflags[ismp] = (psh->type & 0x10) ? RS_STPCM16D : RS_STPCM8D;
		pins->nFineTune = psh->finetune;
		pins->nC4Speed = 8363;
		pins->RelativeTone = (int)psh->relnote;
		if (m_nType != MOD_TYPE_XM)
		{
			pins->nC4Speed = TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
			pins->RelativeTone = 0;
			pins->nFineTune = 0;
		}
		pins->nPan = psh->pan;
		pins->uFlags |= CHN_PANNING;
		pins->nVibType = pih->vibtype;
		pins->nVibSweep = pih->vibsweep;
		pins->nVibDepth = pih->vibdepth;
		pins->nVibRate = pih->vibrate;
		memset(m_szNames[samplemap[ismp]], 0, 32);
		memcpy(m_szNames[samplemap[ismp]], psh->name, 22);
		memcpy(pins->name, psh->name, 22);
		pins->name[21] = 0;
	}
	// Reading sample data
	for (UINT dsmp=0; dsmp<nsamples; dsmp++)
	{
		if (dwMemPos >= dwFileLength) break;
		if (samplemap[dsmp])
			ReadSample(&Ins[samplemap[dsmp]], sampleflags[dsmp], (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
		dwMemPos += samplesize[dsmp];
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Leave if no extra instrument settings are available (end of file reached)
	if(dwMemPos >= dwFileLength) return TRUE;

	ReadExtendedInstrumentProperties(penv, lpMemFile + dwMemPos, dwFileLength - dwMemPos);

// -! NEW_FEATURE#0027

	return TRUE;
}


BOOL CSoundFile::SaveXIInstrument(UINT nInstr, LPCSTR lpszFileName)
//-----------------------------------------------------------------
{
	XIFILEHEADER xfh;
	XIINSTRUMENTHEADER xih;
	XISAMPLEHEADER xsh;
	INSTRUMENTHEADER *penv = Headers[nInstr];
	UINT smptable[32];
	UINT nsamples;
	FILE *f;

	if ((!penv) || (!lpszFileName)) return FALSE;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return FALSE;
	// XI File Header
	memset(&xfh, 0, sizeof(xfh));
	memset(&xih, 0, sizeof(xih));
	memcpy(xfh.extxi, "Extended Instrument: ", 21);
	memcpy(xfh.name, penv->name, 22);
	xfh.name[22] = 0x1A;
	memcpy(xfh.trkname, "FastTracker v2.00   ", 20);
	xfh.shsize = 0x102;
	fwrite(&xfh, 1, sizeof(xfh), f);
	// XI Instrument Header
	xih.volfade = penv->nFadeOut;
	xih.vnum = penv->nVolEnv;
	xih.pnum = penv->nPanEnv;
	if (xih.vnum > 12) xih.vnum = 12;
	if (xih.pnum > 12) xih.pnum = 12;
	for (UINT ienv=0; ienv<12; ienv++)
	{
		xih.venv[ienv*2] = (BYTE)penv->VolPoints[ienv];
		xih.venv[ienv*2+1] = penv->VolEnv[ienv];
		xih.penv[ienv*2] = (BYTE)penv->PanPoints[ienv];
		xih.penv[ienv*2+1] = penv->PanEnv[ienv];
	}
	if (penv->dwFlags & ENV_VOLUME) xih.vtype |= 1;
	if (penv->dwFlags & ENV_VOLSUSTAIN) xih.vtype |= 2;
	if (penv->dwFlags & ENV_VOLLOOP) xih.vtype |= 4;
	if (penv->dwFlags & ENV_PANNING) xih.ptype |= 1;
	if (penv->dwFlags & ENV_PANSUSTAIN) xih.ptype |= 2;
	if (penv->dwFlags & ENV_PANLOOP) xih.ptype |= 4;
	xih.vsustain = (BYTE)penv->nVolSustainBegin;
	xih.vloops = (BYTE)penv->nVolLoopStart;
	xih.vloope = (BYTE)penv->nVolLoopEnd;
	xih.psustain = (BYTE)penv->nPanSustainBegin;
	xih.ploops = (BYTE)penv->nPanLoopStart;
	xih.ploope = (BYTE)penv->nPanLoopEnd;
	nsamples = 0;
	for (UINT j=0; j<96; j++) if (penv->Keyboard[j+12])
	{
		UINT n = penv->Keyboard[j+12];
		UINT k = 0;
		for (k=0; k<nsamples; k++)	if (smptable[k] == n) break;
		if (k == nsamples)
		{
			if (!k)
			{
				xih.vibtype = Ins[n].nVibType;
				xih.vibsweep = Ins[n].nVibSweep;
				xih.vibdepth = Ins[n].nVibDepth;
				xih.vibrate = Ins[n].nVibRate;
			}
			if (nsamples < 32) smptable[nsamples++] = n;
			k = nsamples - 1;
		}
		xih.snum[j] = k;
	}
	xih.reserved2 = nsamples;
	fwrite(&xih, 1, sizeof(xih), f);
	// XI Sample Headers
	for (UINT ismp=0; ismp<nsamples; ismp++)
	{
		MODINSTRUMENT *pins = &Ins[smptable[ismp]];
		xsh.samplen = pins->nLength;
		xsh.loopstart = pins->nLoopStart;
		xsh.looplen = pins->nLoopEnd - pins->nLoopStart;
		xsh.vol = pins->nVolume >> 2;
		xsh.finetune = (signed char)pins->nFineTune;
		xsh.type = 0;
		if (pins->uFlags & CHN_16BIT)
		{
			xsh.type |= 0x10;
			xsh.samplen *= 2;
			xsh.loopstart *= 2;
			xsh.looplen *= 2;
		}
		if (pins->uFlags & CHN_STEREO)
		{
			xsh.type |= 0x20;
			xsh.samplen *= 2;
			xsh.loopstart *= 2;
			xsh.looplen *= 2;
		}
		if (pins->uFlags & CHN_LOOP)
		{
			xsh.type |= (pins->uFlags & CHN_PINGPONGLOOP) ? 0x02 : 0x01;
		}
		xsh.pan = (BYTE)pins->nPan;
		if (pins->nPan > 0xFF) xsh.pan = 0xFF;
		if ((m_nType & MOD_TYPE_XM) || (!pins->nC4Speed))
			xsh.relnote = (signed char) pins->RelativeTone;
		else
		{
			int f2t = FrequencyToTranspose(pins->nC4Speed);
			xsh.relnote = (signed char)(f2t >> 7);
			xsh.finetune = (signed char)(f2t & 0x7F);
		}
		xsh.res = 0;
		memcpy(xsh.name, pins->name, 22);
		fwrite(&xsh, 1, sizeof(xsh), f);
	}
	// XI Sample Data
	for (UINT dsmp=0; dsmp<nsamples; dsmp++)
	{
		MODINSTRUMENT *pins = &Ins[smptable[dsmp]];
		UINT smpflags = (pins->uFlags & CHN_16BIT) ? RS_PCM16D : RS_PCM8D;
		if (pins->uFlags & CHN_STEREO) smpflags = (pins->uFlags & CHN_16BIT) ? RS_STPCM16D : RS_STPCM8D;
		WriteSample(f, pins, smpflags);
	}

	__int32 code = 'MPTX';
	fwrite(&code, 1, sizeof(__int32), f);	// Write extension tag
	WriteInstrumentHeaderStruct(penv, f);	// Write full extended header.


	fclose(f);
	return TRUE;
}


BOOL CSoundFile::ReadXISample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//-------------------------------------------------------------------------------
{
	XIFILEHEADER *pxh = (XIFILEHEADER *)lpMemFile;
	XIINSTRUMENTHEADER *pih = (XIINSTRUMENTHEADER *)(lpMemFile+sizeof(XIFILEHEADER));
	UINT sampleflags = 0;
	DWORD dwMemPos = sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER);
	MODINSTRUMENT *pins = &Ins[nSample];
	UINT nsamples;

	if ((!lpMemFile) || (dwFileLength < sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER))) return FALSE;
	if (memcmp(pxh->extxi, "Extended Instrument", 19)) return FALSE;
	dwMemPos += pxh->shsize - 0x102;
	if ((dwMemPos < sizeof(XIFILEHEADER)) || (dwMemPos >= dwFileLength)) return FALSE;
	DestroySample(nSample);
	nsamples = 0;
	for (UINT i=0; i<96; i++)
	{
		if (pih->snum[i] > nsamples) nsamples = pih->snum[i];
	}
	nsamples++;
	memcpy(m_szNames[nSample], pxh->name, 22);
	pins->uFlags = 0;
	pins->nGlobalVol = 64;
	// Reading sample
	UINT maxsmp = nsamples;
	if ((pih->reserved2 <= 16) && (pih->reserved2 > maxsmp)) maxsmp = pih->reserved2;
	for (UINT ismp=0; ismp<maxsmp; ismp++)
	{
		if (dwMemPos + sizeof(XISAMPLEHEADER) > dwFileLength) break;
		XISAMPLEHEADER *psh = (XISAMPLEHEADER *)(lpMemFile+dwMemPos);
		dwMemPos += sizeof(XISAMPLEHEADER);
		if (ismp) continue;
		sampleflags = RS_PCM8S;
		pins->nLength = psh->samplen;
		pins->nLoopStart = psh->loopstart;
		pins->nLoopEnd = psh->loopstart + psh->looplen;
		if (psh->type & 0x10)
		{
			pins->nLength /= 2;
			pins->nLoopStart /= 2;
			pins->nLoopEnd /= 2;
		}
		if (psh->type & 0x20)
		{
			pins->nLength /= 2;
			pins->nLoopStart /= 2;
			pins->nLoopEnd /= 2;
		}
		if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
		if (psh->type & 3) pins->uFlags |= CHN_LOOP;
		if (psh->type & 2) pins->uFlags |= CHN_PINGPONGLOOP;
		if (pins->nLoopEnd > pins->nLength) pins->nLoopEnd = pins->nLength;
		if (pins->nLoopStart >= pins->nLoopEnd)
		{
			pins->uFlags &= ~CHN_LOOP;
			pins->nLoopStart = 0;
		}
		pins->nVolume = psh->vol << 2;
		if (pins->nVolume > 256) pins->nVolume = 256;
		pins->nGlobalVol = 64;
		sampleflags = (psh->type & 0x10) ? RS_PCM16D : RS_PCM8D;
		if (psh->type & 0x20) sampleflags = (psh->type & 0x10) ? RS_STPCM16D : RS_STPCM8D;
		pins->nFineTune = psh->finetune;
		pins->nC4Speed = 8363;
		pins->RelativeTone = (int)psh->relnote;
		if (m_nType != MOD_TYPE_XM)
		{
			pins->nC4Speed = TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
			pins->RelativeTone = 0;
			pins->nFineTune = 0;
		}
		pins->nPan = psh->pan;
		pins->uFlags |= CHN_PANNING;
		pins->nVibType = pih->vibtype;
		pins->nVibSweep = pih->vibsweep;
		pins->nVibDepth = pih->vibdepth;
		pins->nVibRate = pih->vibrate;
		memcpy(pins->name, psh->name, 22);
		pins->name[21] = 0;
	}
	if (dwMemPos >= dwFileLength) return TRUE;
	ReadSample(pins, sampleflags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////
// AIFF File I/O

typedef struct AIFFFILEHEADER
{
	DWORD dwFORM;	// "FORM" -> 0x4D524F46
	DWORD dwLen;
	DWORD dwAIFF;	// "AIFF" -> 0x46464941
} AIFFFILEHEADER;

typedef struct AIFFCOMM
{
	DWORD dwCOMM;	// "COMM" -> 0x4D4D4F43
	DWORD dwLen;
	WORD wChannels;
	WORD wFramesHi;	// Align!
	WORD wFramesLo;
	WORD wSampleSize;
	BYTE xSampleRate[10];
} AIFFCOMM;

typedef struct AIFFSSND
{
	DWORD dwSSND;	// "SSND" -> 0x444E5353
	DWORD dwLen;
	DWORD dwOffset;
	DWORD dwBlkSize;
} AIFFSSND;


static DWORD FetchLong(LPBYTE p)
{
	DWORD d = p[0];
	d = (d << 8) | p[1];
	d = (d << 8) | p[2];
	d = (d << 8) | p[3];
	return d;
}


static DWORD Ext2Long(LPBYTE p)
{
	DWORD mantissa, last=0;
	BYTE exp;

	mantissa = FetchLong(p+2);
	exp = 30 - p[1];
	while (exp--)
	{
		last = mantissa;
		mantissa >>= 1;
	}
	if (last & 1) mantissa++;
	return mantissa;
}



BOOL CSoundFile::ReadAIFFSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------
{
	DWORD dwMemPos = sizeof(AIFFFILEHEADER);
	DWORD dwFORMLen, dwCOMMLen, dwSSNDLen;
	AIFFFILEHEADER *phdr = (AIFFFILEHEADER *)lpMemFile;
	AIFFCOMM *pcomm;
	AIFFSSND *psnd;
	UINT nType;

	if ((!lpMemFile) || (dwFileLength < (DWORD)sizeof(AIFFFILEHEADER))) return FALSE;
	dwFORMLen = BigEndian(phdr->dwLen);
	if ((phdr->dwFORM != 0x4D524F46) || (phdr->dwAIFF != 0x46464941)
	 || (dwFORMLen > dwFileLength) || (dwFORMLen < (DWORD)sizeof(AIFFCOMM))) return FALSE;
	pcomm = (AIFFCOMM *)(lpMemFile+dwMemPos);
	dwCOMMLen = BigEndian(pcomm->dwLen);
	if ((pcomm->dwCOMM != 0x4D4D4F43) || (dwCOMMLen < 0x12) || (dwCOMMLen >= dwFileLength)) return FALSE;
	if ((pcomm->wChannels != 0x0100) && (pcomm->wChannels != 0x0200)) return FALSE;
	if ((pcomm->wSampleSize != 0x0800) && (pcomm->wSampleSize != 0x1000)) return FALSE;
	dwMemPos += dwCOMMLen + 8;
	if (dwMemPos + sizeof(AIFFSSND) >= dwFileLength) return FALSE;
	psnd = (AIFFSSND *)(lpMemFile+dwMemPos);
	dwSSNDLen = BigEndian(psnd->dwLen);
	if ((psnd->dwSSND != 0x444E5353) || (dwSSNDLen >= dwFileLength) || (dwSSNDLen < 8)) return FALSE;
	dwMemPos += sizeof(AIFFSSND);
	if (dwMemPos >= dwFileLength) return FALSE;
	DestroySample(nSample);
	if (pcomm->wChannels == 0x0100)
	{
		nType = (pcomm->wSampleSize == 0x1000) ? RS_PCM16M : RS_PCM8S;
	} else
	{
		nType = (pcomm->wSampleSize == 0x1000) ? RS_STIPCM16M : RS_STIPCM8S;
	}
	UINT samplesize = (pcomm->wSampleSize >> 11) * (pcomm->wChannels >> 8);
	if (!samplesize) samplesize = 1;
	MODINSTRUMENT *pins = &Ins[nSample];
	if (pins->pSample)
	{
		FreeSample(pins->pSample);
		pins->pSample = NULL;
		pins->nLength = 0;
	}
	pins->nLength = dwSSNDLen / samplesize;
	pins->nLoopStart = pins->nLoopEnd = 0;
	pins->nSustainStart = pins->nSustainEnd = 0;
	pins->nC4Speed = Ext2Long(pcomm->xSampleRate);
	pins->nPan = 128;
	pins->nVolume = 256;
	pins->nGlobalVol = 64;
	pins->uFlags = (pcomm->wSampleSize > 0x0800) ? CHN_16BIT : 0;
	if (pcomm->wChannels >= 0x0200) pins->uFlags |= CHN_STEREO;
	if (m_nType & MOD_TYPE_XM) pins->uFlags |= CHN_PANNING;
	pins->RelativeTone = 0;
	pins->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pins);
	pins->nVibType = pins->nVibSweep = pins->nVibDepth = pins->nVibRate = 0;
	pins->name[0] = 0;
	m_szNames[nSample][0] = 0;
	if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
	ReadSample(pins, nType, (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITS Samples

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//BOOL CSoundFile::ReadITSSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset)
UINT CSoundFile::ReadITSSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset)
// -! NEW_FEATURE#0027
//------------------------------------------------------------------------------------------------
{
	ITSAMPLESTRUCT *pis = (ITSAMPLESTRUCT *)lpMemFile;
	MODINSTRUMENT *pins = &Ins[nSample];
	DWORD dwMemPos;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//	if ((!lpMemFile) || (dwFileLength < sizeof(ITSAMPLESTRUCT))
//	 || (pis->id != 0x53504D49) || (((DWORD)pis->samplepointer) >= dwFileLength + dwOffset)) return FALSE;
	if ((!lpMemFile) || (dwFileLength < sizeof(ITSAMPLESTRUCT))
	 || (pis->id != 0x53504D49) || (((DWORD)pis->samplepointer) >= dwFileLength + dwOffset)) return 0;
// -! NEW_FEATURE#0027
	DestroySample(nSample);
	dwMemPos = pis->samplepointer - dwOffset;
	memcpy(pins->name, pis->filename, 12);
	memcpy(m_szNames[nSample], pis->name, 26);
	m_szNames[nSample][26] = 0;
	pins->nLength = pis->length;
	if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
	pins->nLoopStart = pis->loopbegin;
	pins->nLoopEnd = pis->loopend;
	pins->nSustainStart = pis->susloopbegin;
	pins->nSustainEnd = pis->susloopend;
	pins->nC4Speed = pis->C5Speed;
	if (!pins->nC4Speed) pins->nC4Speed = 8363;
	if (pis->C5Speed < 256) pins->nC4Speed = 256;
	pins->RelativeTone = 0;
	pins->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pins);
	pins->nVolume = pis->vol << 2;
	if (pins->nVolume > 256) pins->nVolume = 256;
	pins->nGlobalVol = pis->gvl;
	if (pins->nGlobalVol > 64) pins->nGlobalVol = 64;
	if (pis->flags & 0x10) pins->uFlags |= CHN_LOOP;
	if (pis->flags & 0x20) pins->uFlags |= CHN_SUSTAINLOOP;
	if (pis->flags & 0x40) pins->uFlags |= CHN_PINGPONGLOOP;
	if (pis->flags & 0x80) pins->uFlags |= CHN_PINGPONGSUSTAIN;
	pins->nPan = (pis->dfp & 0x7F) << 2;
	if (pins->nPan > 256) pins->nPan = 256;
	if (pis->dfp & 0x80) pins->uFlags |= CHN_PANNING;
	pins->nVibType = autovibit2xm[pis->vit & 7];
	pins->nVibSweep = (pis->vir + 3) / 4;
	pins->nVibDepth = pis->vid;
	pins->nVibRate = pis->vis;
	UINT flags = (pis->cvt & 1) ? RS_PCM8S : RS_PCM8U;
	if (pis->flags & 2)
	{
		flags += 5;
		if (pis->flags & 4) {
			flags |= RSF_STEREO;
// -> CODE#0001
// -> DESC="enable saving stereo ITI"
			pins->uFlags |= CHN_STEREO;
// -! BUG_FIX#0001
		}
		pins->uFlags |= CHN_16BIT;
		// IT 2.14 16-bit packed sample ?
		if (pis->flags & 8) flags = RS_IT21416;
	} else
	{
		if (pis->flags & 4) flags |= RSF_STEREO;
		if (pis->cvt == 0xFF) flags = RS_ADPCM4; else
		// IT 2.14 8-bit packed sample ?
		if (pis->flags & 8)	flags =	RS_IT2148;
	}
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//	ReadSample(pins, flags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength + dwOffset - dwMemPos);
//	return TRUE;
	return ReadSample(pins, flags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength + dwOffset - dwMemPos);
// -! NEW_FEATURE#0027
}


BOOL CSoundFile::ReadITIInstrument(UINT nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//-----------------------------------------------------------------------------------
{
	ITINSTRUMENT *pinstr = (ITINSTRUMENT *)lpMemFile;
	WORD samplemap[NOTE_MAX];	//rewbs.noSamplePerInstroLimit (120 was 64)
	DWORD dwMemPos;
	UINT nsmp, nsamples;

	if ((!lpMemFile) || (dwFileLength < sizeof(ITINSTRUMENT))
	 || (pinstr->id != 0x49504D49)) return FALSE;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
	Headers[nInstr] = new INSTRUMENTHEADER;
	INSTRUMENTHEADER *penv = Headers[nInstr];
	if (!penv) return FALSE;
	memset(penv, 0, sizeof(INSTRUMENTHEADER));
	penv->pTuning = penv->s_DefaultTuning;
	memset(samplemap, 0, sizeof(samplemap));
	dwMemPos = 554;
	dwMemPos += ITInstrToMPT(pinstr, penv, pinstr->trkvers);
	nsamples = pinstr->nos;
// -> CODE#0019
// -> DESC="correctly load ITI & XI instruments sample note map"
//	if (nsamples >= 64) nsamples = 64;
// -! BUG_FIX#0019
	nsmp = 1;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	UINT lastSampleSize = 0;
// -! NEW_FEATURE#0027

	// Reading Samples
	for (UINT i=0; i<nsamples; i++)
	{
		while ((nsmp < MAX_SAMPLES) && ((Ins[nsmp].pSample) || (m_szNames[nsmp][0]))) nsmp++;
		if (nsmp >= MAX_SAMPLES) break;
		if (m_nSamples < nsmp) m_nSamples = nsmp;
		samplemap[i] = nsmp;
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//		ReadITSSample(nsmp, lpMemFile+dwMemPos, dwFileLength-dwMemPos, dwMemPos);
		lastSampleSize = ReadITSSample(nsmp, lpMemFile+dwMemPos, dwFileLength-dwMemPos, dwMemPos);
// -! NEW_FEATURE#0027
		dwMemPos += sizeof(ITSAMPLESTRUCT);
		nsmp++;
	}
	for (UINT j=0; j<128; j++)
	{
// -> CODE#0019
// -> DESC="correctly load ITI & XI instruments sample note map"
//		if ((penv->Keyboard[j]) && (penv->Keyboard[j] <= 64))
		if (penv->Keyboard[j])
// -! BUG_FIX#0019
		{
			penv->Keyboard[j] = samplemap[penv->Keyboard[j]-1];
		}
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Rewind file pointer offset (dwMemPos) to last sample header position
	dwMemPos -= sizeof(ITSAMPLESTRUCT);
	BYTE * ptr = (BYTE *)(lpMemFile+dwMemPos);

	// Update file pointer offset (dwMemPos) to match the end of the sample datas
	ITSAMPLESTRUCT *pis = (ITSAMPLESTRUCT *)ptr;
	dwMemPos += pis->samplepointer - dwMemPos + lastSampleSize;
	// Leave if no extra instrument settings are available (end of file reached)
	if(dwMemPos >= dwFileLength) return TRUE;

	ReadExtendedInstrumentProperties(penv, lpMemFile + dwMemPos, dwFileLength - dwMemPos);

// -! NEW_FEATURE#0027

	return TRUE;
}


BOOL CSoundFile::SaveITIInstrument(UINT nInstr, LPCSTR lpszFileName)
//------------------------------------------------------------------
{
	BYTE buffer[554];
	ITINSTRUMENT *iti = (ITINSTRUMENT *)buffer;
	ITSAMPLESTRUCT itss;
	INSTRUMENTHEADER *penv = Headers[nInstr];
	UINT smpcount[MAX_SAMPLES], smptable[MAX_SAMPLES], smpmap[MAX_SAMPLES];
	DWORD dwPos;
	FILE *f;

	if ((!penv) || (!lpszFileName)) return FALSE;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return FALSE;
	memset(buffer, 0, sizeof(buffer));
	memset(smpcount, 0, sizeof(smpcount));
	memset(smptable, 0, sizeof(smptable));
	memset(smpmap, 0, sizeof(smpmap));
	iti->id = 0x49504D49;	// "IMPI"
	memcpy(iti->filename, penv->filename, 12);
	memcpy(iti->name, penv->name, 26);
	SetNullTerminator(iti->name);
	iti->mpr = penv->nMidiProgram;
	iti->mch = penv->nMidiChannel;
	iti->mbank = penv->wMidiBank; //rewbs.MidiBank
	iti->nna = penv->nNNA;
	iti->dct = penv->nDCT;
	iti->dca = penv->nDNA;
	iti->fadeout = penv->nFadeOut >> 5;
	iti->pps = penv->nPPS;
	iti->ppc = penv->nPPC;
	iti->gbv = (BYTE)(penv->nGlobalVol << 1);
	iti->dfp = (BYTE)penv->nPan >> 2;
	if (!(penv->dwFlags & ENV_SETPANNING)) iti->dfp |= 0x80;
	iti->rv = penv->nVolSwing;
	iti->rp = penv->nPanSwing;
	iti->ifc = penv->nIFC;
	iti->ifr = penv->nIFR;
	//iti->trkvers = 0x202;
	iti->trkvers =	0x220;	 //rewbs.ITVersion (was 0x202)
	iti->nos = 0;
	for (UINT i=0; i<NOTE_MAX; i++) if (penv->Keyboard[i] < MAX_SAMPLES)
	{
		UINT smp = penv->Keyboard[i];
		if ((smp) && (!smpcount[smp]))
		{
			smpcount[smp] = 1;
			smptable[iti->nos] = smp;
			smpmap[smp] = iti->nos;
			iti->nos++;
		}
		iti->keyboard[i*2] = penv->NoteMap[i] - 1;
// -> CODE#0019
// -> DESC="correctly load ITI & XI instruments sample note map"
//		iti->keyboard[i*2+1] = smpmap[smp] + 1;
		iti->keyboard[i*2+1] = smp ? smpmap[smp] + 1 : 0;
// -! BUG_FIX#0019
	}
	// Writing Volume envelope
	if (penv->dwFlags & ENV_VOLUME) iti->volenv.flags |= 0x01;
	if (penv->dwFlags & ENV_VOLLOOP) iti->volenv.flags |= 0x02;
	if (penv->dwFlags & ENV_VOLSUSTAIN) iti->volenv.flags |= 0x04;
	if (penv->dwFlags & ENV_VOLCARRY) iti->volenv.flags |= 0x08;
	iti->volenv.num = (BYTE)penv->nVolEnv;
	iti->volenv.lpb = (BYTE)penv->nVolLoopStart;
	iti->volenv.lpe = (BYTE)penv->nVolLoopEnd;
	iti->volenv.slb = penv->nVolSustainBegin;
	iti->volenv.sle = penv->nVolSustainEnd;
	// Writing Panning envelope
	if (penv->dwFlags & ENV_PANNING) iti->panenv.flags |= 0x01;
	if (penv->dwFlags & ENV_PANLOOP) iti->panenv.flags |= 0x02;
	if (penv->dwFlags & ENV_PANSUSTAIN) iti->panenv.flags |= 0x04;
	if (penv->dwFlags & ENV_PANCARRY) iti->panenv.flags |= 0x08;
	iti->panenv.num = (BYTE)penv->nPanEnv;
	iti->panenv.lpb = (BYTE)penv->nPanLoopStart;
	iti->panenv.lpe = (BYTE)penv->nPanLoopEnd;
	iti->panenv.slb = penv->nPanSustainBegin;
	iti->panenv.sle = penv->nPanSustainEnd;
	// Writing Pitch Envelope
	if (penv->dwFlags & ENV_PITCH) iti->pitchenv.flags |= 0x01;
	if (penv->dwFlags & ENV_PITCHLOOP) iti->pitchenv.flags |= 0x02;
	if (penv->dwFlags & ENV_PITCHSUSTAIN) iti->pitchenv.flags |= 0x04;
	if (penv->dwFlags & ENV_PITCHCARRY) iti->pitchenv.flags |= 0x08;
	if (penv->dwFlags & ENV_FILTER) iti->pitchenv.flags |= 0x80;
	iti->pitchenv.num = (BYTE)penv->nPitchEnv;
	iti->pitchenv.lpb = (BYTE)penv->nPitchLoopStart;
	iti->pitchenv.lpe = (BYTE)penv->nPitchLoopEnd;
	iti->pitchenv.slb = (BYTE)penv->nPitchSustainBegin;
	iti->pitchenv.sle = (BYTE)penv->nPitchSustainEnd;
	// Writing Envelopes data
	for (UINT ev=0; ev<25; ev++)
	{
		iti->volenv.data[ev*3] = penv->VolEnv[ev];
		iti->volenv.data[ev*3+1] = penv->VolPoints[ev] & 0xFF;
		iti->volenv.data[ev*3+2] = penv->VolPoints[ev] >> 8;
		iti->panenv.data[ev*3] = penv->PanEnv[ev] - 32;
		iti->panenv.data[ev*3+1] = penv->PanPoints[ev] & 0xFF;
		iti->panenv.data[ev*3+2] = penv->PanPoints[ev] >> 8;
		iti->pitchenv.data[ev*3] = penv->PitchEnv[ev] - 32;
		iti->pitchenv.data[ev*3+1] = penv->PitchPoints[ev] & 0xFF;
		iti->pitchenv.data[ev*3+2] = penv->PitchPoints[ev] >> 8;
	}
	dwPos = 554;
	fwrite(buffer, 1, dwPos, f);
	dwPos += iti->nos * sizeof(ITSAMPLESTRUCT);

	// Writing sample headers
	for (UINT j=0; j<iti->nos; j++) if (smptable[j])
	{
		UINT smpsize = 0;
		UINT nsmp = smptable[j];
		memset(&itss, 0, sizeof(itss));
		MODINSTRUMENT *psmp = &Ins[nsmp];
		itss.id = 0x53504D49;
		memcpy(itss.filename, psmp->name, 12);
		memcpy(itss.name, m_szNames[nsmp], 26);
		itss.gvl = (BYTE)psmp->nGlobalVol;
		itss.flags = 0x01;
		if (psmp->uFlags & CHN_LOOP) itss.flags |= 0x10;
		if (psmp->uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
		if (psmp->uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
		if (psmp->uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
		itss.C5Speed = psmp->nC4Speed;
		if (!itss.C5Speed) itss.C5Speed = 8363;
		itss.length = psmp->nLength;
		itss.loopbegin = psmp->nLoopStart;
		itss.loopend = psmp->nLoopEnd;
		itss.susloopbegin = psmp->nSustainStart;
		itss.susloopend = psmp->nSustainEnd;
		itss.vol = psmp->nVolume >> 2;
		itss.dfp = psmp->nPan >> 2;
		itss.vit = autovibxm2it[psmp->nVibType & 7];
		itss.vir = (psmp->nVibSweep < 64) ? psmp->nVibSweep*4 : 255;
		itss.vid = psmp->nVibDepth;
		itss.vis = psmp->nVibRate;
		if (psmp->uFlags & CHN_PANNING) itss.dfp |= 0x80;
		itss.cvt = 0x01;
		smpsize = psmp->nLength;
		if (psmp->uFlags & CHN_16BIT)
		{
			itss.flags |= 0x02; 
			smpsize <<= 1;
		} else
		{
			itss.flags &= ~(0x02);
		}
		//rewbs.enableStereoITI
		if (psmp->uFlags & CHN_STEREO)
		{
			itss.flags |= 0x04;
			smpsize <<= 1; 
		} else
		{
			itss.flags &= ~(0x04);
		}
		//end rewbs.enableStereoITI
		itss.samplepointer = dwPos;
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
		dwPos += smpsize;
	}
	// Writing Sample Data
	//rewbs.enableStereoITI
	WORD sampleType=0;
	if (itss.flags | 0x02) sampleType=RS_PCM16S; else sampleType=RS_PCM8S;	//8 or 16 bit signed
	if (itss.flags | 0x04) sampleType |= RSF_STEREO;						//mono or stereo
	for (UINT k=0; k<iti->nos; k++)
	{
//rewbs.enableStereoITI - using eric's code as it is better here.
// -> CODE#0001
// -> DESC="enable saving stereo ITI"
		//UINT nsmp = smptable[k];
		//MODINSTRUMENT *psmp = &Ins[nsmp];
		//WriteSample(f, psmp, (psmp->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S);
		MODINSTRUMENT *pins = &Ins[smptable[k]];
		UINT smpflags = (pins->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
		if (pins->uFlags & CHN_STEREO) smpflags = (pins->uFlags & CHN_16BIT) ? RS_STPCM16S : RS_STPCM8S;
		WriteSample(f, pins, smpflags);
// -! BUG_FIX#0001
	}

	__int32 code = 'MPTX';
	fwrite(&code, 1, sizeof(__int32), f);	// Write extension tag
	WriteInstrumentHeaderStruct(penv, f);	// Write full extended header.

	fclose(f);
	return TRUE;
}



bool IsValidSizeField(const LPCBYTE pData, const LPCBYTE pEnd, const int16 size)
//------------------------------------------------------------------------------
{
	if(size < 0 || (uintptr_t)(pEnd - pData) < (uintptr_t)size)
		return false;
	else 
		return true;
}


void ReadInstrumentExtensionField(INSTRUMENTHEADER* penv, LPCBYTE& ptr, const int32 code, const int16 size)
//------------------------------------------------------------------------------------------------------------
{
	// get field's address in instrument's header
	BYTE* fadr = GetInstrumentHeaderFieldPointer(penv, code, size);
	 
	if(fadr && code != 'K[..')	// copy field data in instrument's header
		memcpy(fadr,ptr,size);  // (except for keyboard mapping)
	ptr += size;				// jump field
}


void ReadExtendedInstrumentProperty(INSTRUMENTHEADER* penv, const int32 code, LPCBYTE& pData, const LPCBYTE pEnd)
//---------------------------------------------------------------------------------------------------------------
{
	if(pEnd < pData || uintptr_t(pEnd - pData) < 2)
		return;

	int16 size;
	memcpy(&size, pData, sizeof(size)); // read field size
	pData += sizeof(size);				// jump field size

	if(IsValidSizeField(pData, pEnd, size) == false)
		return;

	ReadInstrumentExtensionField(penv, pData, code, size);
}


void ReadExtendedInstrumentProperties(INSTRUMENTHEADER* penv, const LPCBYTE pDataStart, const size_t nMemLength)
//--------------------------------------------------------------------------------------------------------------
{
	if(penv == 0 || pDataStart == 0 || nMemLength < 4)
		return;

	LPCBYTE pData = pDataStart;
	const LPCBYTE pEnd = pDataStart + nMemLength;

	int32 code;
	memcpy(&code, pData, sizeof(code));

	// Seek for supported extended settings header
	if( code == 'MPTX' )
	{
		pData += sizeof(code); // jump extension header code

		while( (uintptr_t)(pData - pDataStart) <= nMemLength - 4)
		{
			memcpy(&code, pData, sizeof(code)); // read field code
			pData += sizeof(code);				 // jump field code
			ReadExtendedInstrumentProperty(penv, code, pData, pEnd);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// 8SVX Samples

#define IFFID_8SVX	0x58565338
#define IFFID_VHDR	0x52444856
#define IFFID_BODY	0x59444f42
#define IFFID_NAME	0x454d414e
#define IFFID_ANNO	0x4f4e4e41

#pragma pack(1)

typedef struct IFF8SVXFILEHEADER
{
	DWORD dwFORM;	// "FORM"
	DWORD dwSize;
	DWORD dw8SVX;	// "8SVX"
} IFF8SVXFILEHEADER;

typedef struct IFFVHDR
{
	DWORD dwVHDR;	// "VHDR"
	DWORD dwSize;
	ULONG oneShotHiSamples,			/* # samples in the high octave 1-shot part */
			repeatHiSamples,		/* # samples in the high octave repeat part */
			samplesPerHiCycle;		/* # samples/cycle in high octave, else 0 */
	WORD samplesPerSec;				/* data sampling rate */
    BYTE	ctOctave,				/* # octaves of waveforms */
			sCompression;			/* data compression technique used */
	DWORD Volume;
} IFFVHDR;

typedef struct IFFBODY
{
	DWORD dwBody;
	DWORD dwSize;
} IFFBODY;


#pragma pack()



BOOL CSoundFile::Read8SVXSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//--------------------------------------------------------------------------------
{
	IFF8SVXFILEHEADER *pfh = (IFF8SVXFILEHEADER *)lpMemFile;
	IFFVHDR *pvh = (IFFVHDR *)(lpMemFile + 12);
	MODINSTRUMENT *pins = &Ins[nSample];
	DWORD dwMemPos = 12;
	
	if ((!lpMemFile) || (dwFileLength < sizeof(IFFVHDR)+12) || (pfh->dwFORM != IFFID_FORM)
	 || (pfh->dw8SVX != IFFID_8SVX) || (BigEndian(pfh->dwSize) >= dwFileLength)
	 || (pvh->dwVHDR != IFFID_VHDR) || (BigEndian(pvh->dwSize) >= dwFileLength)) return FALSE;
	DestroySample(nSample);
	// Default values
	pins->nGlobalVol = 64;
	pins->nPan = 128;
	pins->nLength = 0;
	pins->nLoopStart = BigEndian(pvh->oneShotHiSamples);
	pins->nLoopEnd = pins->nLoopStart + BigEndian(pvh->repeatHiSamples);
	pins->nSustainStart = 0;
	pins->nSustainEnd = 0;
	pins->uFlags = 0;
	pins->nVolume = (WORD)(BigEndianW((WORD)pvh->Volume) >> 8);
	pins->nC4Speed = BigEndianW(pvh->samplesPerSec);
	pins->name[0] = 0;
	if ((!pins->nVolume) || (pins->nVolume > 256)) pins->nVolume = 256;
	if (!pins->nC4Speed) pins->nC4Speed = 22050;
	pins->RelativeTone = 0;
	pins->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pins);
	dwMemPos += BigEndian(pvh->dwSize) + 8;
	while (dwMemPos + 8 < dwFileLength)
	{
		DWORD dwChunkId = *((LPDWORD)(lpMemFile+dwMemPos));
		DWORD dwChunkLen = BigEndian(*((LPDWORD)(lpMemFile+dwMemPos+4)));
		LPBYTE pChunkData = (LPBYTE)(lpMemFile+dwMemPos+8);
		if (dwChunkLen > dwFileLength - dwMemPos) break;
		switch(dwChunkId)
		{
		case IFFID_NAME:
			{
				UINT len = dwChunkLen;
				if (len > 31) len = 31;
				memset(m_szNames[nSample], 0, 32);
				memcpy(m_szNames[nSample], pChunkData, len);
			}
			break;
		case IFFID_BODY:
			if (!pins->pSample)
			{
				UINT len = dwChunkLen;
				if (len > dwFileLength - dwMemPos - 8) len = dwFileLength - dwMemPos - 8;
				if (len > 4)
				{
					pins->nLength = len;
					if ((pins->nLoopStart + 4 < pins->nLoopEnd) && (pins->nLoopEnd < pins->nLength)) pins->uFlags |= CHN_LOOP;
					ReadSample(pins, RS_PCM8S, (LPSTR)(pChunkData), len);
				}
			}
			break;
		}
		dwMemPos += dwChunkLen + 8;
	}
	return TRUE;
}





