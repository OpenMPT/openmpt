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
#ifdef MODPLUG_TRACKER
#include "../mptrack/Moddoc.h"
#endif //MODPLUG_TRACKER
#include "Wav.h"

#pragma warning(disable:4244)

bool CSoundFile::ReadSampleFromFile(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//--------------------------------------------------------------------------------------------
{
	if ((!nSample) || (nSample >= MAX_SAMPLES)) return false;
	if ((!ReadWAVSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadXISample(nSample, lpMemFile, dwFileLength))
	 && (!ReadAIFFSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadITSSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadPATSample(nSample, lpMemFile, dwFileLength))
	 && (!Read8SVXSample(nSample, lpMemFile, dwFileLength))
	 && (!ReadS3ISample(nSample, lpMemFile, dwFileLength)))
		return false;
	return true;
}


bool CSoundFile::ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------------
{
	if ((!nInstr) || (nInstr >= MAX_INSTRUMENTS)) return false;
	if ((!ReadXIInstrument(nInstr, lpMemFile, dwFileLength))
	 && (!ReadPATInstrument(nInstr, lpMemFile, dwFileLength))
	 && (!ReadITIInstrument(nInstr, lpMemFile, dwFileLength))
	// Generic read
	 && (!ReadSampleAsInstrument(nInstr, lpMemFile, dwFileLength))) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
	return true;
}


bool CSoundFile::ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------------
{
	uint32 *psig = (uint32 *)lpMemFile;
	if ((!lpMemFile) || (dwFileLength < 128)) return false;
	if (((psig[0] == LittleEndian(0x46464952)) && (psig[2] == LittleEndian(0x45564157)))	// RIFF....WAVE signature
	 || ((psig[0] == LittleEndian(0x5453494C)) && (psig[2] == LittleEndian(0x65766177)))	// LIST....wave
	 || (psig[76/4] == LittleEndian(0x53524353))											// S3I signature
	 || ((psig[0] == LittleEndian(0x4D524F46)) && (psig[2] == LittleEndian(0x46464941)))	// AIFF signature
	 || ((psig[0] == LittleEndian(0x4D524F46)) && (psig[2] == LittleEndian(0x58565338)))	// 8SVX signature
	 || (psig[0] == LittleEndian(LittleEndian(IT_IMPS)))									// ITS signature
	)
	{
		// Loading Instrument
		MODINSTRUMENT *pIns = new MODINSTRUMENT;
		if (!pIns) return false;
		memset(pIns, 0, sizeof(MODINSTRUMENT));
		pIns->pTuning = pIns->s_DefaultTuning;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//		RemoveInstrumentSamples(nInstr);
		DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
		Instruments[nInstr] = pIns;
		// Scanning free sample
		UINT nSample = 0;
		for (UINT iscan=1; iscan<MAX_SAMPLES; iscan++)
		{
			if ((!Samples[iscan].pSample) && (!m_szNames[iscan][0]))
			{
				nSample = iscan;
				if (nSample > m_nSamples) m_nSamples = nSample;
				break;
			}
		}
		// Default values
		pIns->nFadeOut = 1024;
		pIns->nGlobalVol = 64;
		pIns->nPan = 128;
		pIns->nPPC = 5*12;
		SetDefaultInstrumentValues(pIns);
		for (UINT iinit=0; iinit<128; iinit++)
		{
			pIns->Keyboard[iinit] = nSample;
			pIns->NoteMap[iinit] = iinit+1;
		}
		if (nSample) ReadSampleFromFile(nSample, lpMemFile, dwFileLength);
		return true;
	}
	return false;
}


// -> CODE#0003
// -> DESC="remove instrument's samples"
// BOOL CSoundFile::DestroyInstrument(UINT nInstr)
bool CSoundFile::DestroyInstrument(INSTRUMENTINDEX nInstr, char removeSamples)
//----------------------------------------------------------------------------
{
	if ((!nInstr) || (nInstr > m_nInstruments)) return false;
	if (!Instruments[nInstr]) return true;

// -> CODE#0003
// -> DESC="remove instrument's samples"
	//rewbs: changed message
	if(removeSamples > 0 || (removeSamples == 0 && ::MessageBox(NULL, "Remove samples associated with an instrument if they are unused?", "Removing instrument", MB_YESNO | MB_ICONQUESTION) == IDYES))
	{
		RemoveInstrumentSamples(nInstr);
	}
// -! BEHAVIOUR_CHANGE#0003

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	m_szInstrumentPath[nInstr - 1][0] = '\0';
#ifdef MODPLUG_TRACKER
	if(GetpModDoc())
	{
		GetpModDoc()->m_bsInstrumentModified.reset(nInstr - 1);
	}
#endif // MODPLUG_TRACKER
// -! NEW_FEATURE#0023

	MODINSTRUMENT *pIns = Instruments[nInstr];
	Instruments[nInstr] = nullptr;
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		if (Chn[i].pModInstrument == pIns)
		{
			Chn[i].pModInstrument = nullptr;
		}
	}
	delete pIns;
	return true;
}


// Removing all unused samples
bool CSoundFile::RemoveInstrumentSamples(INSTRUMENTINDEX nInstr)
//--------------------------------------------------------------
{
	vector<bool> sampleused(GetNumSamples() + 1, false);

	if (Instruments[nInstr])
	{
		MODINSTRUMENT *p = Instruments[nInstr];
		for (UINT r=0; r<128; r++)
		{
			UINT n = p->Keyboard[r];
			if (n <= GetNumSamples()) sampleused[n] = true;
		}
		for (INSTRUMENTINDEX nIns = 1; nIns < MAX_INSTRUMENTS; nIns++) if ((Instruments[nIns]) && (nIns != nInstr))
		{
			p = Instruments[nIns];
			for (UINT r=0; r<128; r++)
			{
				UINT n = p->Keyboard[r];
				if (n <= GetNumSamples()) sampleused[n] = false;
			}
		}
		for (SAMPLEINDEX nSmp = 1; nSmp <= GetNumSamples(); nSmp++) if (sampleused[nSmp])
		{
			DestroySample(nSmp);
			strcpy(m_szNames[nSmp], "");
		}
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
//
// I/O From another song
//

bool CSoundFile::ReadInstrumentFromSong(INSTRUMENTINDEX nInstr, CSoundFile *pSrcSong, UINT nSrcInstr)
//---------------------------------------------------------------------------------------------------
{
	if ((!pSrcSong) || (!nSrcInstr) || (nSrcInstr > pSrcSong->m_nInstruments)
	 || (nInstr >= MAX_INSTRUMENTS) || (!pSrcSong->Instruments[nSrcInstr])) return false;
	if (m_nInstruments < nInstr) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr, 1);
// -! BEHAVIOUR_CHANGE#0003
	if (!Instruments[nInstr]) Instruments[nInstr] = new MODINSTRUMENT;
	MODINSTRUMENT *pIns = Instruments[nInstr];
	if (pIns)
	{
		WORD samplemap[32];
		WORD samplesrc[32];
		UINT nSamples = 0;
		UINT nsmp = 1;
		*pIns = *pSrcSong->Instruments[nSrcInstr];
		for (UINT i=0; i<128; i++)
		{
			UINT n = pIns->Keyboard[i];
			if ((n) && (n <= pSrcSong->m_nSamples) && (i < NOTE_MAX))
			{
				UINT j = 0;
				for (j=0; j<nSamples; j++)
				{
					if (samplesrc[j] == n) break;
				}
				if (j >= nSamples)
				{
					while ((nsmp < MAX_SAMPLES) && ((Samples[nsmp].pSample) || (m_szNames[nsmp][0]))) nsmp++;
					if ((nSamples < 32) && (nsmp < MAX_SAMPLES))
					{
						samplesrc[nSamples] = (WORD)n;
						samplemap[nSamples] = (WORD)nsmp;
						nSamples++;
						pIns->Keyboard[i] = (WORD)nsmp;
						if (m_nSamples < nsmp) m_nSamples = nsmp;
						nsmp++;
					} else
					{
						pIns->Keyboard[i] = 0;
					}
				} else
				{
					pIns->Keyboard[i] = samplemap[j];
				}
			} else
			{
				pIns->Keyboard[i] = 0;
			}
		}
		// Load Samples
		for (UINT k=0; k<nSamples; k++)
		{
			ReadSampleFromSong(samplemap[k], pSrcSong, samplesrc[k]);
		}
		return true;
	}
	return false;
}


bool CSoundFile::ReadSampleFromSong(SAMPLEINDEX nSample, CSoundFile *pSrcSong, UINT nSrcSample)
//---------------------------------------------------------------------------------------------
{
	if ((!pSrcSong) || (!nSrcSample) || (nSrcSample > pSrcSong->m_nSamples) || (nSample >= MAX_SAMPLES)) return false;
	MODSAMPLE *psmp = &pSrcSong->Samples[nSrcSample];
	UINT nSize = psmp->nLength;
	if (psmp->uFlags & CHN_16BIT) nSize *= 2;
	if (psmp->uFlags & CHN_STEREO) nSize *= 2;
	if (m_nSamples < nSample) m_nSamples = nSample;
	if (Samples[nSample].pSample)
	{
		Samples[nSample].nLength = 0;
		FreeSample(Samples[nSample].pSample);
	}
	Samples[nSample] = *psmp;
	if (psmp->pSample)
	{
		Samples[nSample].pSample = AllocateSample(nSize+8);
		if (Samples[nSample].pSample)
		{
			memcpy(Samples[nSample].pSample, psmp->pSample, nSize);
			AdjustSampleLoop(&Samples[nSample]);
		}
	}
	if ((!(m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))) && (pSrcSong->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)))
	{
		MODSAMPLE *pSmp = &Samples[nSample];
		pSmp->nC5Speed = TransposeToFrequency(pSmp->RelativeTone, pSmp->nFineTune);
		pSmp->RelativeTone = 0;
		pSmp->nFineTune = 0;
	} else
	if ((m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) && (!(pSrcSong->m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))))
	{
		FrequencyToTranspose(&Samples[nSample]);
	}
	return true;
}


////////////////////////////////////////////////////////////////////////////////
// WAV Open

#define IFFID_pcm	0x206d6370
#define IFFID_fact	0x74636166

extern BOOL IMAADPCMUnpack16(signed short *pdest, UINT nLen, LPBYTE psrc, DWORD dwBytes, UINT pkBlkAlign);

bool CSoundFile::ReadWAVSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD *pdwWSMPOffset)
//-------------------------------------------------------------------------------------------------------------
{
	DWORD dwMemPos = 0, dwDataPos;
	WAVEFILEHEADER *phdr = (WAVEFILEHEADER *)lpMemFile;
	WAVEFORMATHEADER *pfmt, *pfmtpk;
	WAVEDATAHEADER *pdata;
	WAVESMPLHEADER *psh;
	WAVEEXTRAHEADER *pxh;
	DWORD dwInfoList, dwFact, dwSamplesPerBlock;
	
	if ((!nSample) || (!lpMemFile) || (dwFileLength < (DWORD)(sizeof(WAVEFILEHEADER)+sizeof(WAVEFORMATHEADER)))) return false;
	if (((phdr->id_RIFF != IFFID_RIFF) && (phdr->id_RIFF != IFFID_LIST))
	 || ((phdr->id_WAVE != IFFID_WAVE) && (phdr->id_WAVE != IFFID_wave))) return false;
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
	if ((!pdata) || (!pfmt) || (pdata->length < 4)) return false;
	if ((pfmtpk) && (pfmt))
	{
		if (pfmt->format != 1)
		{
			WAVEFORMATHEADER *tmp = pfmt;
			pfmt = pfmtpk;
			pfmtpk = tmp;
			if ((pfmtpk->format != 0x11) || (pfmtpk->bitspersample != 4)
			 || (pfmtpk->channels != 1)) return false;
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
	) return false;

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
	MODSAMPLE *pSmp = &Samples[nSample];
	if (pSmp->pSample)
	{
		FreeSample(pSmp->pSample);
		pSmp->pSample = nullptr;
		pSmp->nLength = 0;
	}
	pSmp->nLength = pdata->length / samplesize;
	pSmp->nLoopStart = pSmp->nLoopEnd = 0;
	pSmp->nSustainStart = pSmp->nSustainEnd = 0;
	pSmp->nC5Speed = pfmt->freqHz;
	pSmp->nPan = 128;
	pSmp->nVolume = 256;
	pSmp->nGlobalVol = 64;
	pSmp->uFlags = (pfmt->bitspersample > 8) ? CHN_16BIT : 0;
	if (m_nType & MOD_TYPE_XM) pSmp->uFlags |= CHN_PANNING;
	pSmp->RelativeTone = 0;
	pSmp->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pSmp);
	pSmp->nVibType = pSmp->nVibSweep = pSmp->nVibDepth = pSmp->nVibRate = 0;
	pSmp->filename[0] = 0;
	memset(m_szNames[nSample], 0, 32);
	if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;
	// IMA ADPCM 4:1
	if (pfmtpk)
	{
		if (dwFact < 4) dwFact = pdata->length * 2;
		pSmp->nLength = dwFact;
		pSmp->pSample = AllocateSample(pSmp->nLength*2+16);
		IMAADPCMUnpack16((signed short *)pSmp->pSample, pSmp->nLength,
						 (LPBYTE)(lpMemFile+dwDataPos), dwFileLength-dwDataPos, pfmtpk->samplesize);
		AdjustSampleLoop(pSmp);
	} else
	{
		ReadSample(pSmp, nType, (LPSTR)(lpMemFile+dwDataPos), dwFileLength-dwDataPos, pfmt->format);
	}
	// smpl field
	if (psh)
	{
		pSmp->nLoopStart = pSmp->nLoopEnd = 0;
		if ((psh->dwSampleLoops) && (sizeof(WAVESMPLHEADER) + psh->dwSampleLoops * sizeof(SAMPLELOOPSTRUCT) <= psh->smpl_len + 8))
		{
			SAMPLELOOPSTRUCT *psl = (SAMPLELOOPSTRUCT *)(&psh[1]);
			if (psh->dwSampleLoops > 1)
			{
				pSmp->uFlags |= CHN_LOOP | CHN_SUSTAINLOOP;
				if (psl[0].dwLoopType) pSmp->uFlags |= CHN_PINGPONGSUSTAIN;
				if (psl[1].dwLoopType) pSmp->uFlags |= CHN_PINGPONGLOOP;
				pSmp->nSustainStart = psl[0].dwLoopStart;
				pSmp->nSustainEnd = psl[0].dwLoopEnd;
				pSmp->nLoopStart = psl[1].dwLoopStart;
				pSmp->nLoopEnd = psl[1].dwLoopEnd;
			} else
			{
				pSmp->uFlags |= CHN_LOOP;
				if (psl->dwLoopType) pSmp->uFlags |= CHN_PINGPONGLOOP;
				pSmp->nLoopStart = psl->dwLoopStart;
				pSmp->nLoopEnd = psl->dwLoopEnd;
			}
			if (pSmp->nLoopStart >= pSmp->nLoopEnd) pSmp->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP);
			if (pSmp->nSustainStart >= pSmp->nSustainEnd) pSmp->uFlags &= ~(CHN_PINGPONGLOOP|CHN_PINGPONGSUSTAIN);
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
						memcpy(pSmp->filename, lpMemFile+dwInfoList+d+8, dwNameLen);
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
		if (!(GetType() & (MOD_TYPE_MOD|MOD_TYPE_S3M)))
		{
			if (pxh->dwFlags & CHN_PINGPONGLOOP) pSmp->uFlags |= CHN_PINGPONGLOOP;
			if (pxh->dwFlags & CHN_SUSTAINLOOP) pSmp->uFlags |= CHN_SUSTAINLOOP;
			if (pxh->dwFlags & CHN_PINGPONGSUSTAIN) pSmp->uFlags |= CHN_PINGPONGSUSTAIN;
			if (pxh->dwFlags & CHN_PANNING) pSmp->uFlags |= CHN_PANNING;
		}
		pSmp->nPan = pxh->wPan;
		pSmp->nVolume = pxh->wVolume;
		pSmp->nGlobalVol = pxh->wGlobalVol;
		pSmp->nVibType = pxh->nVibType;
		pSmp->nVibSweep = pxh->nVibSweep;
		pSmp->nVibDepth = pxh->nVibDepth;
		pSmp->nVibRate = pxh->nVibRate;
		// Name present (clipboard only)
		UINT xtrabytes = pxh->xtra_len + 8 - sizeof(WAVEEXTRAHEADER);
		LPSTR pszTextEx = (LPSTR)(pxh+1); 
		if (xtrabytes >= MAX_SAMPLENAME)
		{
			memcpy(m_szNames[nSample], pszTextEx, MAX_SAMPLENAME - 1);
			pszTextEx += MAX_SAMPLENAME;
			xtrabytes -= MAX_SAMPLENAME;
			if (xtrabytes >= MAX_SAMPLEFILENAME)
			{
				memcpy(pSmp->filename, pszTextEx, MAX_SAMPLEFILENAME);
				xtrabytes -= MAX_SAMPLEFILENAME;
			}
		}
	}
	return true;
}


///////////////////////////////////////////////////////////////
// Save WAV

bool CSoundFile::SaveWAVSample(UINT nSample, LPCSTR lpszFileName)
//---------------------------------------------------------------
{
	LPCSTR lpszMPT = "Modplug Tracker\0";
	WAVEFILEHEADER header;
	WAVEFORMATHEADER format;
	WAVEDATAHEADER data;
	WAVESAMPLERINFO smpl;
	WAVELISTHEADER list;
	WAVEEXTRAHEADER extra;
	MODSAMPLE *pSmp = &Samples[nSample];
	FILE *f;

	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	memset(&extra, 0, sizeof(extra));
	memset(&smpl, 0, sizeof(smpl));
	header.id_RIFF = IFFID_RIFF;
	header.filesize = sizeof(header)+sizeof(format)+sizeof(data)+sizeof(extra)-8;
	header.filesize += 12+8+16+8+32; // LIST(INAM, ISFT)
	header.id_WAVE = IFFID_WAVE;
	format.id_fmt = IFFID_fmt;
	format.hdrlen = 16;
	format.format = 1;
	format.freqHz = pSmp->nC5Speed;
	if (m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) format.freqHz = TransposeToFrequency(pSmp->RelativeTone, pSmp->nFineTune);
	format.channels = (pSmp->uFlags & CHN_STEREO) ? 2 : 1;
	format.bitspersample = (pSmp->uFlags & CHN_16BIT) ? 16 : 8;
	format.samplesize = (format.channels*format.bitspersample)>>3;
	format.bytessec = format.freqHz*format.samplesize;
	data.id_data = IFFID_data;
	UINT nType;
	data.length = pSmp->nLength;
	if (pSmp->uFlags & CHN_STEREO)
	{
		nType = (pSmp->uFlags & CHN_16BIT) ? RS_STIPCM16S : RS_STIPCM8U;
		data.length *= 2;
	} else
	{
		nType = (pSmp->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8U;
	}
	if (pSmp->uFlags & CHN_16BIT) data.length *= 2;
	header.filesize += data.length;
	fwrite(&header, 1, sizeof(header), f);
	fwrite(&format, 1, sizeof(format), f);
	fwrite(&data, 1, sizeof(data), f);
	WriteSample(f, pSmp, nType);
	// "smpl" field
	smpl.wsiHdr.smpl_id = 0x6C706D73;
	smpl.wsiHdr.smpl_len = sizeof(WAVESMPLHEADER) - 8;
	smpl.wsiHdr.dwSamplePeriod = 22675;
	if (pSmp->nC5Speed >= 256) smpl.wsiHdr.dwSamplePeriod = 1000000000 / pSmp->nC5Speed;
	smpl.wsiHdr.dwBaseNote = 60;
	if (pSmp->uFlags & (CHN_LOOP|CHN_SUSTAINLOOP))
	{
		if (pSmp->uFlags & CHN_SUSTAINLOOP)
		{
			smpl.wsiHdr.dwSampleLoops = 2;
			smpl.wsiLoops[0].dwLoopType = (pSmp->uFlags & CHN_PINGPONGSUSTAIN) ? 1 : 0;
			smpl.wsiLoops[0].dwLoopStart = pSmp->nSustainStart;
			smpl.wsiLoops[0].dwLoopEnd = pSmp->nSustainEnd;
			smpl.wsiLoops[1].dwLoopType = (pSmp->uFlags & CHN_PINGPONGLOOP) ? 1 : 0;
			smpl.wsiLoops[1].dwLoopStart = pSmp->nLoopStart;
			smpl.wsiLoops[1].dwLoopEnd = pSmp->nLoopEnd;
		} else
		{
			smpl.wsiHdr.dwSampleLoops = 1;
			smpl.wsiLoops[0].dwLoopType = (pSmp->uFlags & CHN_PINGPONGLOOP) ? 1 : 0;
			smpl.wsiLoops[0].dwLoopStart = pSmp->nLoopStart;
			smpl.wsiLoops[0].dwLoopEnd = pSmp->nLoopEnd;
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
	extra.dwFlags = pSmp->uFlags;
	extra.wPan = pSmp->nPan;
	extra.wVolume = pSmp->nVolume;
	extra.wGlobalVol = pSmp->nGlobalVol;
	extra.wReserved = 0;
	extra.nVibType = pSmp->nVibType;
	extra.nVibSweep = pSmp->nVibSweep;
	extra.nVibDepth = pSmp->nVibDepth;
	extra.nVibRate = pSmp->nVibRate;
	fwrite(&extra, 1, sizeof(extra), f);
	fclose(f);
	return true;
}

///////////////////////////////////////////////////////////////
// Save RAW

bool CSoundFile::SaveRAWSample(UINT nSample, LPCSTR lpszFileName)
//---------------------------------------------------------------
{
	MODSAMPLE *pSmp = &Samples[nSample];
	FILE *f;

	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;

	UINT nType;
	if (pSmp->uFlags & CHN_STEREO)
		nType = (pSmp->uFlags & CHN_16BIT) ? RS_STIPCM16S : RS_STIPCM8S;
	else
		nType = (pSmp->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
	WriteSample(f, pSmp, nType);
	fclose(f);
	return true;
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
	MODSAMPLE *pIns = &that->Samples[nSample];
	DWORD dwMemPos = sizeof(GF1SAMPLEHEADER);
	GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream);
	UINT nSmpType;
	
	if (dwMemLength < sizeof(GF1SAMPLEHEADER)) return;
	if (psh->name[0])
	{
		memcpy(that->m_szNames[nSample], psh->name, 7);
		that->m_szNames[nSample][7] = 0;
	}
	pIns->filename[0] = 0;
	pIns->nGlobalVol = 64;
	pIns->uFlags = (psh->flags & 1) ? CHN_16BIT : 0;
	if (psh->flags & 4) pIns->uFlags |= CHN_LOOP;
	if (psh->flags & 8) pIns->uFlags |= CHN_PINGPONGLOOP;
	pIns->nLength = psh->length;
	pIns->nLoopStart = psh->loopstart;
	pIns->nLoopEnd = psh->loopend;
	pIns->nC5Speed = psh->freq;
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
	pIns->nC5Speed = that->TransposeToFrequency(pIns->RelativeTone, pIns->nFineTune);
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


bool CSoundFile::ReadPATSample(SAMPLEINDEX nSample, LPBYTE lpStream, DWORD dwMemLength)
//-------------------------------------------------------------------------------------
{
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pinshdr = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));

	if ((!lpStream) || (dwMemLength < 512)
	 || (phdr->gf1p != 0x50314647) || (phdr->atch != 0x48435441)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0) || (phdr->instrum < 1)
	 || (!phdr->samples) || (!pinshdr->layers)) return false;
	DestroySample(nSample);
	PatchToSample(this, nSample, lpStream+dwMemPos, dwMemLength-dwMemPos);
	if (pinshdr->name[0] > ' ')
	{
		memcpy(m_szNames[nSample], pinshdr->name, 16);
		m_szNames[nSample][16] = 0;
	}
	return true;
}


// PAT Instrument
bool CSoundFile::ReadPATInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpStream, DWORD dwMemLength)
//--------------------------------------------------------------------------------------------
{
	GF1PATCHFILEHEADER *phdr = (GF1PATCHFILEHEADER *)lpStream;
	GF1INSTRUMENT *pih = (GF1INSTRUMENT *)(lpStream+sizeof(GF1PATCHFILEHEADER));
	GF1LAYER *plh = (GF1LAYER *)(lpStream+sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT));
	MODINSTRUMENT *pIns;
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	UINT nSamples;

	if ((!lpStream) || (dwMemLength < 512)
	 || (phdr->gf1p != 0x50314647) || (phdr->atch != 0x48435441)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0)
	 || (phdr->instrum < 1) || (!phdr->samples)
	 || (!pih->layers) || (!plh->samples)) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
	pIns = new MODINSTRUMENT;
	if (!pIns) return false;
	memset(pIns, 0, sizeof(MODINSTRUMENT));
	pIns->pTuning = pIns->s_DefaultTuning;
	Instruments[nInstr] = pIns;
	nSamples = plh->samples;
	if (nSamples > 16) nSamples = 16;
	memcpy(pIns->name, pih->name, 16);
	pIns->name[16] = 0;
	pIns->nFadeOut = 2048;
	pIns->nGlobalVol = 64;
	pIns->nPan = 128;
	pIns->nPPC = 60;
	pIns->nResampling = SRCMODE_DEFAULT;
	pIns->nFilterMode = FLTMODE_UNCHANGED;
	if (m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		pIns->nNNA = NNA_NOTEOFF;
		pIns->nDNA = DNA_NOTEFADE;
	}
	UINT nFreeSmp = 1;
	UINT nMinSmpNote = 0xff;
	UINT nMinSmp = 0;
	for (UINT iSmp=0; iSmp<nSamples; iSmp++)
	{
		// Find a free sample
		while ((nFreeSmp < MAX_SAMPLES) && ((Samples[nFreeSmp].pSample) || (m_szNames[nFreeSmp][0]))) nFreeSmp++;
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
			 || ((!pIns->Keyboard[k])
			  && ((LONG)k >= nMinNote)
			  && ((LONG)k <= nMaxNote)))
			{
				if (psh->scale_factor)
					pIns->NoteMap[k] = (BYTE)(k+1);
				else
					pIns->NoteMap[k] = 5*12+1;
				pIns->Keyboard[k] = nFreeSmp;
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
			pIns->dwFlags |= ENV_VOLUME;
			if (psh->flags & 32) pIns->dwFlags |= ENV_VOLSUSTAIN;
			pIns->VolEnv.Values[0] = 64;
			pIns->VolEnv.Ticks[0] = 0;
			pIns->VolEnv.Values[1] = 64;
			pIns->VolEnv.Ticks[1] = 1;
			pIns->VolEnv.Values[2] = 32;
			pIns->VolEnv.Ticks[2] = 20;
			pIns->VolEnv.Values[3] = 0;
			pIns->VolEnv.Ticks[3] = 100;
			pIns->VolEnv.nNodes = 4;
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
			if (!pIns->NoteMap[k]) pIns->NoteMap[k] = (BYTE)(k+1);
			if (!pIns->Keyboard[k])
			{
				pIns->Keyboard[k] = nMinSmp;
			} else
			{
				nMinSmp = pIns->Keyboard[k];
			}
		}
	}
	return true;
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
	DWORD nC5Speed;
	DWORD reserved3;
	DWORD reserved4;
	DWORD date;
	CHAR name[28];
	DWORD scrs;
} S3ISAMPLESTRUCT;

bool CSoundFile::ReadS3ISample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------
{
	S3ISAMPLESTRUCT *pss = (S3ISAMPLESTRUCT *)lpMemFile;
	MODSAMPLE *pSmp = &Samples[nSample];
	DWORD dwMemPos;
	UINT flags;

	if ((!lpMemFile) || (dwFileLength < sizeof(S3ISAMPLESTRUCT))
	 || (pss->id != 0x01) || (((DWORD)pss->offset << 4) >= dwFileLength)
	 || (pss->scrs != 0x53524353)) return false;
	DestroySample(nSample);
	dwMemPos = pss->offset << 4;
	memcpy(pSmp->filename, pss->filename, 12);
	memcpy(m_szNames[nSample], pss->name, 28);
	m_szNames[nSample][28] = 0;
	pSmp->nLength = pss->length;
	pSmp->nLoopStart = pss->loopstart;
	pSmp->nLoopEnd = pss->loopend;
	pSmp->nGlobalVol = 64;
	pSmp->nVolume = pss->volume << 2;
	pSmp->uFlags = 0;
	pSmp->nPan = 128;
	pSmp->nC5Speed = pss->nC5Speed;
	pSmp->RelativeTone = 0;
	pSmp->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pSmp);
	if (pss->flags & 0x01) pSmp->uFlags |= CHN_LOOP;
	flags = (pss->flags & 0x04) ? RS_PCM16U : RS_PCM8U;
	if (pss->flags & 0x02) flags |= RSF_STEREO;
	ReadSample(pSmp, flags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
	return true;
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
	WORD pIns[24];
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




bool CSoundFile::ReadXIInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------
{
	XIFILEHEADER *pxh = (XIFILEHEADER *)lpMemFile;
	XIINSTRUMENTHEADER *pih = (XIINSTRUMENTHEADER *)(lpMemFile+sizeof(XIFILEHEADER));
	UINT samplemap[32];
	UINT sampleflags[32];
	DWORD samplesize[32];
	DWORD dwMemPos = sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER);
	UINT nsamples;

	if ((!lpMemFile) || (dwFileLength < sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER))) return false;
	if (dwMemPos + pxh->shsize - 0x102 >= dwFileLength) return false;
	if (memcmp(pxh->extxi, "Extended Instrument", 19)) return false;
	dwMemPos += pxh->shsize - 0x102;
	if ((dwMemPos < sizeof(XIFILEHEADER)) || (dwMemPos >= dwFileLength)) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
	Instruments[nInstr] = new MODINSTRUMENT;
	MODINSTRUMENT *pIns = Instruments[nInstr];
	if (!pIns) return false;
	memset(pIns, 0, sizeof(MODINSTRUMENT));
	pIns->pTuning = pIns->s_DefaultTuning;
	memcpy(pIns->name, pxh->name, 22);
	nsamples = 0;
	for (UINT i=0; i<96; i++)
	{
		pIns->NoteMap[i+12] = i+1+12;
		if (pih->snum[i] > nsamples) nsamples = pih->snum[i];
	}
	nsamples++;
	if (nsamples > 32) nsamples = 32;
	// Allocate samples
	memset(samplemap, 0, sizeof(samplemap));
	UINT nsmp = 1;
	for (UINT j=0; j<nsamples; j++)
	{
		while ((nsmp < MAX_SAMPLES) && ((Samples[nsmp].pSample) || (m_szNames[nsmp][0]))) nsmp++;
		if (nsmp >= MAX_SAMPLES) break;
		samplemap[j] = nsmp;
		if (m_nSamples < nsmp) m_nSamples = nsmp;
		nsmp++;
	}
	// Setting up instrument header
	for (UINT k=0; k<96; k++)
	{
		UINT n = pih->snum[k];
		if (n < nsamples) pIns->Keyboard[k+12] = samplemap[n];
	}
	pIns->nFadeOut = pih->volfade;
	if (pih->vtype & 1) pIns->VolEnv.dwFlags |= ENV_ENABLED;
	if (pih->vtype & 2) pIns->VolEnv.dwFlags |= ENV_SUSTAIN;
	if (pih->vtype & 4) pIns->VolEnv.dwFlags |= ENV_LOOP;
	if (pih->ptype & 1) pIns->PanEnv.dwFlags |= ENV_ENABLED;
	if (pih->ptype & 2) pIns->PanEnv.dwFlags |= ENV_SUSTAIN;
	if (pih->ptype & 4) pIns->PanEnv.dwFlags |= ENV_LOOP;
	pIns->VolEnv.nNodes = pih->vnum;
	pIns->PanEnv.nNodes = pih->pnum;
	if (pIns->VolEnv.nNodes > 12) pIns->VolEnv.nNodes = 12;
	if (pIns->PanEnv.nNodes > 12) pIns->PanEnv.nNodes = 12;
	if (!pIns->VolEnv.nNodes) pIns->VolEnv.dwFlags &= ~ENV_ENABLED;
	if (!pIns->PanEnv.nNodes) pIns->PanEnv.dwFlags &= ~ENV_ENABLED;
	pIns->VolEnv.nSustainStart = pih->vsustain;
	pIns->VolEnv.nSustainEnd = pih->vsustain;
	if (pih->vsustain >= 12) pIns->VolEnv.dwFlags &= ~ENV_SUSTAIN;
	pIns->VolEnv.nLoopStart = pih->vloops;
	pIns->VolEnv.nLoopEnd = pih->vloope;
	if (pIns->VolEnv.nLoopEnd >= 12) pIns->VolEnv.nLoopEnd = 0;
	if (pIns->VolEnv.nLoopStart >= pIns->VolEnv.nLoopEnd) pIns->VolEnv.dwFlags &= ~ENV_LOOP;
	pIns->PanEnv.nSustainStart = pih->psustain;
	pIns->PanEnv.nSustainEnd = pih->psustain;
	if (pih->psustain >= 12) pIns->PanEnv.dwFlags &= ~ENV_SUSTAIN;
	pIns->PanEnv.nLoopStart = pih->ploops;
	pIns->PanEnv.nLoopEnd = pih->ploope;
	if (pIns->PanEnv.nLoopEnd >= 12) pIns->PanEnv.nLoopEnd = 0;
	if (pIns->PanEnv.nLoopStart >= pIns->PanEnv.nLoopEnd) pIns->PanEnv.dwFlags &= ~ENV_LOOP;
	pIns->nGlobalVol = 64;
	pIns->nPPC = 5*12;
	SetDefaultInstrumentValues(pIns);
	for (UINT ienv=0; ienv<12; ienv++)
	{
		pIns->VolEnv.Ticks[ienv] = (WORD)pih->venv[ienv*2];
		pIns->VolEnv.Values[ienv] = (BYTE)pih->venv[ienv*2+1];
		pIns->PanEnv.Ticks[ienv] = (WORD)pih->pIns[ienv*2];
		pIns->PanEnv.Values[ienv] = (BYTE)pih->pIns[ienv*2+1];
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
		MODSAMPLE *pSmp = &Samples[samplemap[ismp]];
		pSmp->uFlags = 0;
		pSmp->nLength = psh->samplen;
		pSmp->nLoopStart = psh->loopstart;
		pSmp->nLoopEnd = psh->loopstart + psh->looplen;
		if (psh->type & 0x10)
		{
			pSmp->nLength /= 2;
			pSmp->nLoopStart /= 2;
			pSmp->nLoopEnd /= 2;
		}
		if (psh->type & 0x20)
		{
			pSmp->nLength /= 2;
			pSmp->nLoopStart /= 2;
			pSmp->nLoopEnd /= 2;
		}
		if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;
		if (psh->type & 3) pSmp->uFlags |= CHN_LOOP;
		if (psh->type & 2) pSmp->uFlags |= CHN_PINGPONGLOOP;
		if (pSmp->nLoopEnd > pSmp->nLength) pSmp->nLoopEnd = pSmp->nLength;
		if (pSmp->nLoopStart >= pSmp->nLoopEnd)
		{
			pSmp->uFlags &= ~CHN_LOOP;
			pSmp->nLoopStart = 0;
		}
		pSmp->nVolume = psh->vol << 2;
		if (pSmp->nVolume > 256) pSmp->nVolume = 256;
		pSmp->nGlobalVol = 64;
		sampleflags[ismp] = (psh->type & 0x10) ? RS_PCM16D : RS_PCM8D;
		if (psh->type & 0x20) sampleflags[ismp] = (psh->type & 0x10) ? RS_STPCM16D : RS_STPCM8D;
		pSmp->nFineTune = psh->finetune;
		pSmp->nC5Speed = 8363;
		pSmp->RelativeTone = (int)psh->relnote;
		if (m_nType != MOD_TYPE_XM)
		{
			pSmp->nC5Speed = TransposeToFrequency(pSmp->RelativeTone, pSmp->nFineTune);
			pSmp->RelativeTone = 0;
			pSmp->nFineTune = 0;
		}
		pSmp->nPan = psh->pan;
		pSmp->uFlags |= CHN_PANNING;
		pSmp->nVibType = pih->vibtype;
		pSmp->nVibSweep = pih->vibsweep;
		pSmp->nVibDepth = pih->vibdepth;
		pSmp->nVibRate = pih->vibrate;
		memset(m_szNames[samplemap[ismp]], 0, 32);
		memcpy(m_szNames[samplemap[ismp]], psh->name, 22);
		memcpy(pSmp->filename, psh->name, 22);
		pSmp->filename[21] = 0;
	}
	// Reading sample data
	for (UINT dsmp=0; dsmp<nsamples; dsmp++)
	{
		if (dwMemPos >= dwFileLength) break;
		if (samplemap[dsmp])
			ReadSample(&Samples[samplemap[dsmp]], sampleflags[dsmp], (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
		dwMemPos += samplesize[dsmp];
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Leave if no extra instrument settings are available (end of file reached)
	if(dwMemPos >= dwFileLength) return true;

	ReadExtendedInstrumentProperties(pIns, lpMemFile + dwMemPos, dwFileLength - dwMemPos);

// -! NEW_FEATURE#0027

	return true;
}


bool CSoundFile::SaveXIInstrument(INSTRUMENTINDEX nInstr, LPCSTR lpszFileName)
//----------------------------------------------------------------------------
{
	XIFILEHEADER xfh;
	XIINSTRUMENTHEADER xih;
	XISAMPLEHEADER xsh;
	MODINSTRUMENT *pIns = Instruments[nInstr];
	UINT smptable[32];
	UINT nsamples;
	FILE *f;

	if ((!pIns) || (!lpszFileName)) return false;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	// XI File Header
	memset(&xfh, 0, sizeof(xfh));
	memset(&xih, 0, sizeof(xih));
	memcpy(xfh.extxi, "Extended Instrument: ", 21);
	memcpy(xfh.name, pIns->name, 22);
	xfh.name[22] = 0x1A;
	memcpy(xfh.trkname, "Created by OpenMPT  ", 20);
	xfh.shsize = 0x102;
	fwrite(&xfh, 1, sizeof(xfh), f);
	// XI Instrument Header
	xih.volfade = pIns->nFadeOut;
	xih.vnum = pIns->VolEnv.nNodes;
	xih.pnum = pIns->PanEnv.nNodes;
	if (xih.vnum > 12) xih.vnum = 12;
	if (xih.pnum > 12) xih.pnum = 12;
	for (UINT ienv=0; ienv<12; ienv++)
	{
		xih.venv[ienv*2] = (BYTE)pIns->VolEnv.Ticks[ienv];
		xih.venv[ienv*2+1] = pIns->VolEnv.Values[ienv];
		xih.pIns[ienv*2] = (BYTE)pIns->PanEnv.Ticks[ienv];
		xih.pIns[ienv*2+1] = pIns->PanEnv.Values[ienv];
	}
	if (pIns->VolEnv.dwFlags & ENV_ENABLED) xih.vtype |= 1;
	if (pIns->VolEnv.dwFlags & ENV_SUSTAIN) xih.vtype |= 2;
	if (pIns->VolEnv.dwFlags & ENV_LOOP) xih.vtype |= 4;
	if (pIns->PanEnv.dwFlags & ENV_ENABLED) xih.ptype |= 1;
	if (pIns->PanEnv.dwFlags & ENV_SUSTAIN) xih.ptype |= 2;
	if (pIns->PanEnv.dwFlags & ENV_LOOP) xih.ptype |= 4;
	xih.vsustain = (BYTE)pIns->VolEnv.nSustainStart;
	xih.vloops = (BYTE)pIns->VolEnv.nLoopStart;
	xih.vloope = (BYTE)pIns->VolEnv.nLoopEnd;
	xih.psustain = (BYTE)pIns->PanEnv.nSustainStart;
	xih.ploops = (BYTE)pIns->PanEnv.nLoopStart;
	xih.ploope = (BYTE)pIns->PanEnv.nLoopEnd;
	nsamples = 0;
	for (UINT j=0; j<96; j++) if (pIns->Keyboard[j+12])
	{
		UINT n = pIns->Keyboard[j+12];
		UINT k = 0;
		for (k=0; k<nsamples; k++)	if (smptable[k] == n) break;
		if (k == nsamples)
		{
			if (!k)
			{
				xih.vibtype = Samples[n].nVibType;
				xih.vibsweep = min(Samples[n].nVibSweep, 255);
				xih.vibdepth = min(Samples[n].nVibDepth, 15);
				xih.vibrate = min(Samples[n].nVibRate, 63);
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
		MODSAMPLE *pSmp = &Samples[smptable[ismp]];
		xsh.samplen = pSmp->nLength;
		xsh.loopstart = pSmp->nLoopStart;
		xsh.looplen = pSmp->nLoopEnd - pSmp->nLoopStart;
		xsh.vol = pSmp->nVolume >> 2;
		xsh.finetune = (signed char)pSmp->nFineTune;
		xsh.type = 0;
		if (pSmp->uFlags & CHN_16BIT)
		{
			xsh.type |= 0x10;
			xsh.samplen *= 2;
			xsh.loopstart *= 2;
			xsh.looplen *= 2;
		}
		if (pSmp->uFlags & CHN_STEREO)
		{
			xsh.type |= 0x20;
			xsh.samplen *= 2;
			xsh.loopstart *= 2;
			xsh.looplen *= 2;
		}
		if (pSmp->uFlags & CHN_LOOP)
		{
			xsh.type |= (pSmp->uFlags & CHN_PINGPONGLOOP) ? 0x02 : 0x01;
		}
		xsh.pan = (BYTE)pSmp->nPan;
		if (pSmp->nPan > 0xFF) xsh.pan = 0xFF;
		if ((m_nType & MOD_TYPE_XM) || (!pSmp->nC5Speed))
			xsh.relnote = (signed char) pSmp->RelativeTone;
		else
		{
			int f2t = FrequencyToTranspose(pSmp->nC5Speed);
			xsh.relnote = (signed char)(f2t >> 7);
			xsh.finetune = (signed char)(f2t & 0x7F);
		}
		xsh.res = 0;
		memcpy(xsh.name, pSmp->filename, 22);
		fwrite(&xsh, 1, sizeof(xsh), f);
	}
	// XI Sample Data
	for (UINT dsmp=0; dsmp<nsamples; dsmp++)
	{
		MODSAMPLE *pSmp = &Samples[smptable[dsmp]];
		UINT smpflags = (pSmp->uFlags & CHN_16BIT) ? RS_PCM16D : RS_PCM8D;
		if (pSmp->uFlags & CHN_STEREO) smpflags = (pSmp->uFlags & CHN_16BIT) ? RS_STPCM16D : RS_STPCM8D;
		WriteSample(f, pSmp, smpflags);
	}

	__int32 code = 'MPTX';
	fwrite(&code, 1, sizeof(__int32), f);	// Write extension tag
	WriteInstrumentHeaderStruct(pIns, f);	// Write full extended header.


	fclose(f);
	return true;
}


bool CSoundFile::ReadXISample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//--------------------------------------------------------------------------------------
{
	XIFILEHEADER *pxh = (XIFILEHEADER *)lpMemFile;
	XIINSTRUMENTHEADER *pih = (XIINSTRUMENTHEADER *)(lpMemFile+sizeof(XIFILEHEADER));
	UINT sampleflags = 0;
	DWORD dwMemPos = sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER);
	MODSAMPLE *pSmp = &Samples[nSample];
	UINT nsamples;

	if ((!lpMemFile) || (dwFileLength < sizeof(XIFILEHEADER)+sizeof(XIINSTRUMENTHEADER))) return false;
	if (memcmp(pxh->extxi, "Extended Instrument", 19)) return false;
	dwMemPos += pxh->shsize - 0x102;
	if ((dwMemPos < sizeof(XIFILEHEADER)) || (dwMemPos >= dwFileLength)) return false;
	DestroySample(nSample);
	nsamples = 0;
	for (UINT i=0; i<96; i++)
	{
		if (pih->snum[i] > nsamples) nsamples = pih->snum[i];
	}
	nsamples++;
	memcpy(m_szNames[nSample], pxh->name, 22);
	pSmp->uFlags = 0;
	pSmp->nGlobalVol = 64;
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
		pSmp->nLength = psh->samplen;
		pSmp->nLoopStart = psh->loopstart;
		pSmp->nLoopEnd = psh->loopstart + psh->looplen;
		if (psh->type & 0x10)
		{
			pSmp->nLength /= 2;
			pSmp->nLoopStart /= 2;
			pSmp->nLoopEnd /= 2;
		}
		if (psh->type & 0x20)
		{
			pSmp->nLength /= 2;
			pSmp->nLoopStart /= 2;
			pSmp->nLoopEnd /= 2;
		}
		if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;
		if (psh->type & 3) pSmp->uFlags |= CHN_LOOP;
		if (psh->type & 2) pSmp->uFlags |= CHN_PINGPONGLOOP;
		if (pSmp->nLoopEnd > pSmp->nLength) pSmp->nLoopEnd = pSmp->nLength;
		if (pSmp->nLoopStart >= pSmp->nLoopEnd)
		{
			pSmp->uFlags &= ~CHN_LOOP;
			pSmp->nLoopStart = 0;
		}
		pSmp->nVolume = psh->vol << 2;
		if (pSmp->nVolume > 256) pSmp->nVolume = 256;
		pSmp->nGlobalVol = 64;
		sampleflags = (psh->type & 0x10) ? RS_PCM16D : RS_PCM8D;
		if (psh->type & 0x20) sampleflags = (psh->type & 0x10) ? RS_STPCM16D : RS_STPCM8D;
		pSmp->nFineTune = psh->finetune;
		pSmp->nC5Speed = 8363;
		pSmp->RelativeTone = (int)psh->relnote;
		if (m_nType != MOD_TYPE_XM)
		{
			pSmp->nC5Speed = TransposeToFrequency(pSmp->RelativeTone, pSmp->nFineTune);
			pSmp->RelativeTone = 0;
			pSmp->nFineTune = 0;
		}
		pSmp->nPan = psh->pan;
		pSmp->uFlags |= CHN_PANNING;
		pSmp->nVibType = pih->vibtype;
		pSmp->nVibSweep = pih->vibsweep;
		pSmp->nVibDepth = pih->vibdepth;
		pSmp->nVibRate = pih->vibrate;
		memcpy(pSmp->filename, psh->name, 22);
		pSmp->filename[21] = 0;
	}
	if (dwMemPos >= dwFileLength) return true;
	ReadSample(pSmp, sampleflags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
	return true;
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



bool CSoundFile::ReadAIFFSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//----------------------------------------------------------------------------------------
{
	DWORD dwMemPos = sizeof(AIFFFILEHEADER);
	DWORD dwFORMLen, dwCOMMLen, dwSSNDLen;
	AIFFFILEHEADER *phdr = (AIFFFILEHEADER *)lpMemFile;
	AIFFCOMM *pcomm;
	AIFFSSND *psnd;
	UINT nType;

	if ((!lpMemFile) || (dwFileLength < (DWORD)sizeof(AIFFFILEHEADER))) return false;
	dwFORMLen = BigEndian(phdr->dwLen);
	if ((phdr->dwFORM != 0x4D524F46) || (phdr->dwAIFF != 0x46464941)
	 || (dwFORMLen > dwFileLength) || (dwFORMLen < (DWORD)sizeof(AIFFCOMM))) return false;
	pcomm = (AIFFCOMM *)(lpMemFile+dwMemPos);
	dwCOMMLen = BigEndian(pcomm->dwLen);
	if ((pcomm->dwCOMM != 0x4D4D4F43) || (dwCOMMLen < 0x12) || (dwCOMMLen >= dwFileLength)) return false;
	if ((pcomm->wChannels != 0x0100) && (pcomm->wChannels != 0x0200)) return false;
	if ((pcomm->wSampleSize != 0x0800) && (pcomm->wSampleSize != 0x1000)) return false;
	dwMemPos += dwCOMMLen + 8;
	if (dwMemPos + sizeof(AIFFSSND) >= dwFileLength) return false;
	psnd = (AIFFSSND *)(lpMemFile+dwMemPos);
	dwSSNDLen = BigEndian(psnd->dwLen);
	if ((psnd->dwSSND != 0x444E5353) || (dwSSNDLen >= dwFileLength) || (dwSSNDLen < 8)) return false;
	dwMemPos += sizeof(AIFFSSND);
	if (dwMemPos >= dwFileLength) return false;
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
	MODSAMPLE *pSmp = &Samples[nSample];
	if (pSmp->pSample)
	{
		FreeSample(pSmp->pSample);
		pSmp->pSample = nullptr;
		pSmp->nLength = 0;
	}
	pSmp->nLength = dwSSNDLen / samplesize;
	pSmp->nLoopStart = pSmp->nLoopEnd = 0;
	pSmp->nSustainStart = pSmp->nSustainEnd = 0;
	pSmp->nC5Speed = Ext2Long(pcomm->xSampleRate);
	pSmp->nPan = 128;
	pSmp->nVolume = 256;
	pSmp->nGlobalVol = 64;
	pSmp->uFlags = (pcomm->wSampleSize > 0x0800) ? CHN_16BIT : 0;
	if (pcomm->wChannels >= 0x0200) pSmp->uFlags |= CHN_STEREO;
	if (m_nType & MOD_TYPE_XM) pSmp->uFlags |= CHN_PANNING;
	pSmp->RelativeTone = 0;
	pSmp->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pSmp);
	pSmp->nVibType = pSmp->nVibSweep = pSmp->nVibDepth = pSmp->nVibRate = 0;
	pSmp->filename[0] = 0;
	m_szNames[nSample][0] = 0;
	if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;
	ReadSample(pSmp, nType, (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// ITS Samples

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//BOOL CSoundFile::ReadITSSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset)
UINT CSoundFile::ReadITSSample(SAMPLEINDEX nSample, LPBYTE lpMemFile, DWORD dwFileLength, DWORD dwOffset)
//-------------------------------------------------------------------------------------------------------
{
	ITSAMPLESTRUCT *pis = (ITSAMPLESTRUCT *)lpMemFile;
	MODSAMPLE *pSmp = &Samples[nSample];
	DWORD dwMemPos;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//	if ((!lpMemFile) || (dwFileLength < sizeof(ITSAMPLESTRUCT))
//	 || (pis->id != LittleEndian(IT_IMPS)) || (((DWORD)pis->samplepointer) >= dwFileLength + dwOffset)) return FALSE;
	if ((!lpMemFile) || (dwFileLength < sizeof(ITSAMPLESTRUCT))
	 || (pis->id != LittleEndian(IT_IMPS)) || (((DWORD)pis->samplepointer) >= dwFileLength + dwOffset)) return 0;
// -! NEW_FEATURE#0027
	DestroySample(nSample);
	dwMemPos = pis->samplepointer - dwOffset;
	memcpy(pSmp->filename, pis->filename, 12);
	memcpy(m_szNames[nSample], pis->name, 26);
	m_szNames[nSample][26] = 0;
	pSmp->nLength = pis->length;
	if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;
	pSmp->nLoopStart = pis->loopbegin;
	pSmp->nLoopEnd = pis->loopend;
	pSmp->nSustainStart = pis->susloopbegin;
	pSmp->nSustainEnd = pis->susloopend;
	pSmp->nC5Speed = pis->C5Speed;
	if (!pSmp->nC5Speed) pSmp->nC5Speed = 8363;
	if (pis->C5Speed < 256) pSmp->nC5Speed = 256;
	pSmp->RelativeTone = 0;
	pSmp->nFineTune = 0;
	if (GetType() == MOD_TYPE_XM) FrequencyToTranspose(pSmp);
	pSmp->nVolume = pis->vol << 2;
	if (pSmp->nVolume > 256) pSmp->nVolume = 256;
	pSmp->nGlobalVol = pis->gvl;
	if (pSmp->nGlobalVol > 64) pSmp->nGlobalVol = 64;
	pSmp->uFlags = 0;
	if (pis->flags & 0x10) pSmp->uFlags |= CHN_LOOP;
	if (pis->flags & 0x20) pSmp->uFlags |= CHN_SUSTAINLOOP;
	if (pis->flags & 0x40) pSmp->uFlags |= CHN_PINGPONGLOOP;
	if (pis->flags & 0x80) pSmp->uFlags |= CHN_PINGPONGSUSTAIN;
	pSmp->nPan = (pis->dfp & 0x7F) << 2;
	if (pSmp->nPan > 256) pSmp->nPan = 256;
	if (pis->dfp & 0x80) pSmp->uFlags |= CHN_PANNING;
	pSmp->nVibType = autovibit2xm[pis->vit & 7];
	pSmp->nVibSweep = pis->vir;
	pSmp->nVibDepth = pis->vid;
	pSmp->nVibRate = pis->vis;
	UINT flags = (pis->cvt & 1) ? RS_PCM8S : RS_PCM8U;
	if (pis->flags & 2)
	{
		flags += 5;
		if (pis->flags & 4)
		{
			flags |= RSF_STEREO;
// -> CODE#0001
// -> DESC="enable saving stereo ITI"
			pSmp->uFlags |= CHN_STEREO;
// -! BUG_FIX#0001
		}
		pSmp->uFlags |= CHN_16BIT;
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
//	ReadSample(pSmp, flags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength + dwOffset - dwMemPos);
//	return TRUE;
	return ReadSample(pSmp, flags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength + dwOffset - dwMemPos);
// -! NEW_FEATURE#0027
}


bool CSoundFile::ReadITIInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//----------------------------------------------------------------------------------------------
{
	ITINSTRUMENT *pinstr = (ITINSTRUMENT *)lpMemFile;
	WORD samplemap[NOTE_MAX];	//rewbs.noSamplePerInstroLimit (120 was 64)
	DWORD dwMemPos;
	UINT nsmp, nsamples;

	if ((!lpMemFile) || (dwFileLength < sizeof(ITINSTRUMENT))
	 || (pinstr->id != LittleEndian(IT_IMPI))) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;
// -> CODE#0003
// -> DESC="remove instrument's samples"
//	RemoveInstrumentSamples(nInstr);
	DestroyInstrument(nInstr,1);
// -! BEHAVIOUR_CHANGE#0003
	Instruments[nInstr] = new MODINSTRUMENT;
	MODINSTRUMENT *pIns = Instruments[nInstr];
	if (!pIns) return false;
	memset(pIns, 0, sizeof(MODINSTRUMENT));
	pIns->pTuning = pIns->s_DefaultTuning;
	memset(samplemap, 0, sizeof(samplemap));
	dwMemPos = 554;
	dwMemPos += ITInstrToMPT(pinstr, pIns, pinstr->trkvers);
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
		while ((nsmp < MAX_SAMPLES) && ((Samples[nsmp].pSample) || (m_szNames[nsmp][0]))) nsmp++;
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
//		if ((pIns->Keyboard[j]) && (pIns->Keyboard[j] <= 64))
		if (pIns->Keyboard[j])
// -! BUG_FIX#0019
		{
			pIns->Keyboard[j] = samplemap[pIns->Keyboard[j]-1];
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
	if(dwMemPos >= dwFileLength) return true;

	ReadExtendedInstrumentProperties(pIns, lpMemFile + dwMemPos, dwFileLength - dwMemPos);

// -! NEW_FEATURE#0027

	return true;
}


bool CSoundFile::SaveITIInstrument(INSTRUMENTINDEX nInstr, LPCSTR lpszFileName)
//-----------------------------------------------------------------------------
{
	BYTE buffer[554];
	ITINSTRUMENT *iti = (ITINSTRUMENT *)buffer;
	ITSAMPLESTRUCT itss;
	MODINSTRUMENT *pIns = Instruments[nInstr];
	vector<bool> smpcount(GetNumSamples(), false);
	UINT smptable[MAX_SAMPLES], smpmap[MAX_SAMPLES];
	DWORD dwPos;
	FILE *f;

	if ((!pIns) || (!lpszFileName)) return false;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	memset(buffer, 0, sizeof(buffer));
	memset(smptable, 0, sizeof(smptable));
	memset(smpmap, 0, sizeof(smpmap));
	iti->id = LittleEndian(IT_IMPI);	// "IMPI"
	memcpy(iti->filename, pIns->filename, 12);
	memcpy(iti->name, pIns->name, 26);
	SetNullTerminator(iti->name);
	iti->mpr = pIns->nMidiProgram;
	iti->mch = pIns->nMidiChannel;
	iti->mbank = pIns->wMidiBank; //rewbs.MidiBank
	iti->nna = pIns->nNNA;
	iti->dct = pIns->nDCT;
	iti->dca = pIns->nDNA;
	iti->fadeout = pIns->nFadeOut >> 5;
	iti->pps = pIns->nPPS;
	iti->ppc = pIns->nPPC;
	iti->gbv = (BYTE)(pIns->nGlobalVol << 1);
	iti->dfp = (BYTE)pIns->nPan >> 2;
	if (!(pIns->dwFlags & INS_SETPANNING)) iti->dfp |= 0x80;
	iti->rv = pIns->nVolSwing;
	iti->rp = pIns->nPanSwing;
	iti->ifc = pIns->nIFC;
	iti->ifr = pIns->nIFR;
	//iti->trkvers = 0x202;
	iti->trkvers =	0x220;	 //rewbs.ITVersion (was 0x202)
	iti->nos = 0;
	for (UINT i=0; i<NOTE_MAX; i++) if (pIns->Keyboard[i] < MAX_SAMPLES)
	{
		const UINT smp = pIns->Keyboard[i];
		if (smp && smp <= GetNumSamples() && !smpcount[smp - 1])
		{
			smpcount[smp - 1] = true;
			smptable[iti->nos] = smp;
			smpmap[smp] = iti->nos;
			iti->nos++;
		}
		iti->keyboard[i*2] = pIns->NoteMap[i] - 1;
// -> CODE#0019
// -> DESC="correctly load ITI & XI instruments sample note map"
//		iti->keyboard[i*2+1] = smpmap[smp] + 1;
		iti->keyboard[i*2+1] = smp ? smpmap[smp] + 1 : 0;
// -! BUG_FIX#0019
	}
	// Writing Volume envelope
	if (pIns->VolEnv.dwFlags & ENV_ENABLED) iti->volenv.flags |= 0x01;
	if (pIns->VolEnv.dwFlags & ENV_LOOP) iti->volenv.flags |= 0x02;
	if (pIns->VolEnv.dwFlags & ENV_SUSTAIN) iti->volenv.flags |= 0x04;
	if (pIns->VolEnv.dwFlags & ENV_CARRY) iti->volenv.flags |= 0x08;
	iti->volenv.num = (BYTE)pIns->VolEnv.nNodes;
	iti->volenv.lpb = (BYTE)pIns->VolEnv.nLoopStart;
	iti->volenv.lpe = (BYTE)pIns->VolEnv.nLoopEnd;
	iti->volenv.slb = pIns->VolEnv.nSustainStart;
	iti->volenv.sle = pIns->VolEnv.nSustainEnd;
	// Writing Panning envelope
	if (pIns->PanEnv.dwFlags & ENV_ENABLED) iti->panenv.flags |= 0x01;
	if (pIns->PanEnv.dwFlags & ENV_LOOP) iti->panenv.flags |= 0x02;
	if (pIns->PanEnv.dwFlags & ENV_SUSTAIN) iti->panenv.flags |= 0x04;
	if (pIns->PanEnv.dwFlags & ENV_CARRY) iti->panenv.flags |= 0x08;
	iti->panenv.num = (BYTE)pIns->PanEnv.nNodes;
	iti->panenv.lpb = (BYTE)pIns->PanEnv.nLoopStart;
	iti->panenv.lpe = (BYTE)pIns->PanEnv.nLoopEnd;
	iti->panenv.slb = pIns->PanEnv.nSustainStart;
	iti->panenv.sle = pIns->PanEnv.nSustainEnd;
	// Writing Pitch Envelope
	if (pIns->PitchEnv.dwFlags & ENV_ENABLED) iti->pitchenv.flags |= 0x01;
	if (pIns->PitchEnv.dwFlags & ENV_LOOP) iti->pitchenv.flags |= 0x02;
	if (pIns->PitchEnv.dwFlags & ENV_SUSTAIN) iti->pitchenv.flags |= 0x04;
	if (pIns->PitchEnv.dwFlags & ENV_CARRY) iti->pitchenv.flags |= 0x08;
	if (pIns->PitchEnv.dwFlags & ENV_FILTER) iti->pitchenv.flags |= 0x80;
	iti->pitchenv.num = (BYTE)pIns->PitchEnv.nNodes;
	iti->pitchenv.lpb = (BYTE)pIns->PitchEnv.nLoopStart;
	iti->pitchenv.lpe = (BYTE)pIns->PitchEnv.nLoopEnd;
	iti->pitchenv.slb = (BYTE)pIns->PitchEnv.nSustainStart;
	iti->pitchenv.sle = (BYTE)pIns->PitchEnv.nSustainEnd;
	// Writing Envelopes data
	for (UINT ev=0; ev<25; ev++)
	{
		iti->volenv.data[ev*3] = pIns->VolEnv.Values[ev];
		iti->volenv.data[ev*3+1] = pIns->VolEnv.Ticks[ev] & 0xFF;
		iti->volenv.data[ev*3+2] = pIns->VolEnv.Ticks[ev] >> 8;
		iti->panenv.data[ev*3] = pIns->PanEnv.Values[ev] - 32;
		iti->panenv.data[ev*3+1] = pIns->PanEnv.Ticks[ev] & 0xFF;
		iti->panenv.data[ev*3+2] = pIns->PanEnv.Ticks[ev] >> 8;
		iti->pitchenv.data[ev*3] = pIns->PitchEnv.Values[ev] - 32;
		iti->pitchenv.data[ev*3+1] = pIns->PitchEnv.Ticks[ev] & 0xFF;
		iti->pitchenv.data[ev*3+2] = pIns->PitchEnv.Ticks[ev] >> 8;
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
		MODSAMPLE *psmp = &Samples[nsmp];
		itss.id = LittleEndian(IT_IMPS);
		memcpy(itss.filename, psmp->filename, 12);
		memcpy(itss.name, m_szNames[nsmp], 26);
		itss.gvl = (BYTE)psmp->nGlobalVol;
		itss.flags = 0x01;
		if (psmp->uFlags & CHN_LOOP) itss.flags |= 0x10;
		if (psmp->uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
		if (psmp->uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
		if (psmp->uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
		itss.C5Speed = psmp->nC5Speed;
		if (!itss.C5Speed) itss.C5Speed = 8363;
		itss.length = psmp->nLength;
		itss.loopbegin = psmp->nLoopStart;
		itss.loopend = psmp->nLoopEnd;
		itss.susloopbegin = psmp->nSustainStart;
		itss.susloopend = psmp->nSustainEnd;
		itss.vol = psmp->nVolume >> 2;
		itss.dfp = psmp->nPan >> 2;
		itss.vit = autovibxm2it[psmp->nVibType & 7];
		itss.vir = min(psmp->nVibSweep, 255);
		itss.vid = min(psmp->nVibDepth, 32);
		itss.vis = min(psmp->nVibRate, 64);
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
		MODSAMPLE *pSmp = &Samples[smptable[k]];
		UINT smpflags = (pSmp->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
		if (pSmp->uFlags & CHN_STEREO) smpflags = (pSmp->uFlags & CHN_16BIT) ? RS_STPCM16S : RS_STPCM8S;
		WriteSample(f, pSmp, smpflags);
// -! BUG_FIX#0001
	}

	__int32 code = 'MPTX';
	fwrite(&code, 1, sizeof(__int32), f);	// Write extension tag
	WriteInstrumentHeaderStruct(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}



bool IsValidSizeField(const LPCBYTE pData, const LPCBYTE pEnd, const int16 size)
//------------------------------------------------------------------------------
{
	if(size < 0 || (uintptr_t)(pEnd - pData) < (uintptr_t)size)
		return false;
	else 
		return true;
}


void ReadInstrumentExtensionField(MODINSTRUMENT* pIns, LPCBYTE& ptr, const int32 code, const int16 size)
//------------------------------------------------------------------------------------------------------------
{
	// get field's address in instrument's header
	BYTE* fadr = GetInstrumentHeaderFieldPointer(pIns, code, size);
	 
	if(fadr && code != 'K[..')	// copy field data in instrument's header
		memcpy(fadr,ptr,size);  // (except for keyboard mapping)
	ptr += size;				// jump field

	if (code == 'dF..' && fadr != nullptr) // 'dF..' field requires additional processing.
		ConvertReadExtendedFlags(pIns);
}


void ReadExtendedInstrumentProperty(MODINSTRUMENT* pIns, const int32 code, LPCBYTE& pData, const LPCBYTE pEnd)
//---------------------------------------------------------------------------------------------------------------
{
	if(pEnd < pData || uintptr_t(pEnd - pData) < 2)
		return;

	int16 size;
	memcpy(&size, pData, sizeof(size)); // read field size
	pData += sizeof(size);				// jump field size

	if(IsValidSizeField(pData, pEnd, size) == false)
		return;

	ReadInstrumentExtensionField(pIns, pData, code, size);
}


void ReadExtendedInstrumentProperties(MODINSTRUMENT* pIns, const LPCBYTE pDataStart, const size_t nMemLength)
//--------------------------------------------------------------------------------------------------------------
{
	if(pIns == 0 || pDataStart == 0 || nMemLength < 4)
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
			ReadExtendedInstrumentProperty(pIns, code, pData, pEnd);
		}
	}
}


void ConvertReadExtendedFlags(MODINSTRUMENT *pIns)
//------------------------------------------------
{
	const DWORD dwOldFlags = pIns->dwFlags;
	pIns->dwFlags = pIns->VolEnv.dwFlags = pIns->PanEnv.dwFlags = pIns->PitchEnv.dwFlags = 0;
	if(dwOldFlags & dFdd_VOLUME)		pIns->VolEnv.dwFlags |= ENV_ENABLED;
	if(dwOldFlags & dFdd_VOLSUSTAIN)	pIns->VolEnv.dwFlags |= ENV_SUSTAIN;
	if(dwOldFlags & dFdd_VOLLOOP)		pIns->VolEnv.dwFlags |= ENV_LOOP;
	if(dwOldFlags & dFdd_PANNING)		pIns->PanEnv.dwFlags |= ENV_ENABLED;
	if(dwOldFlags & dFdd_PANSUSTAIN)	pIns->PanEnv.dwFlags |= ENV_SUSTAIN;
	if(dwOldFlags & dFdd_PANLOOP)		pIns->PanEnv.dwFlags |= ENV_LOOP;
	if(dwOldFlags & dFdd_PITCH)			pIns->PitchEnv.dwFlags |= ENV_ENABLED;
	if(dwOldFlags & dFdd_PITCHSUSTAIN)	pIns->PitchEnv.dwFlags |= ENV_SUSTAIN;
	if(dwOldFlags & dFdd_PITCHLOOP)		pIns->PitchEnv.dwFlags |= ENV_LOOP;
	if(dwOldFlags & dFdd_SETPANNING)	pIns->dwFlags |= INS_SETPANNING;
	if(dwOldFlags & dFdd_FILTER)		pIns->PitchEnv.dwFlags |= ENV_FILTER;
	if(dwOldFlags & dFdd_VOLCARRY)		pIns->VolEnv.dwFlags |= ENV_CARRY;
	if(dwOldFlags & dFdd_PANCARRY)		pIns->PanEnv.dwFlags |= ENV_CARRY;
	if(dwOldFlags & dFdd_PITCHCARRY)	pIns->PitchEnv.dwFlags |= ENV_CARRY;
	if(dwOldFlags & dFdd_MUTE)			pIns->dwFlags |= INS_MUTE;
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



bool CSoundFile::Read8SVXSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------
{
	IFF8SVXFILEHEADER *pfh = (IFF8SVXFILEHEADER *)lpMemFile;
	IFFVHDR *pvh = (IFFVHDR *)(lpMemFile + 12);
	MODSAMPLE *pSmp = &Samples[nSample];
	DWORD dwMemPos = 12;
	
	if ((!lpMemFile) || (dwFileLength < sizeof(IFFVHDR)+12) || (pfh->dwFORM != IFFID_FORM)
	 || (pfh->dw8SVX != IFFID_8SVX) || (BigEndian(pfh->dwSize) >= dwFileLength)
	 || (pvh->dwVHDR != IFFID_VHDR) || (BigEndian(pvh->dwSize) >= dwFileLength)) return false;
	DestroySample(nSample);
	// Default values
	pSmp->nGlobalVol = 64;
	pSmp->nPan = 128;
	pSmp->nLength = 0;
	pSmp->nLoopStart = BigEndian(pvh->oneShotHiSamples);
	pSmp->nLoopEnd = pSmp->nLoopStart + BigEndian(pvh->repeatHiSamples);
	pSmp->nSustainStart = 0;
	pSmp->nSustainEnd = 0;
	pSmp->uFlags = 0;
	pSmp->nVolume = (WORD)(BigEndianW((WORD)pvh->Volume) >> 8);
	pSmp->nC5Speed = BigEndianW(pvh->samplesPerSec);
	pSmp->filename[0] = 0;
	if ((!pSmp->nVolume) || (pSmp->nVolume > 256)) pSmp->nVolume = 256;
	if (!pSmp->nC5Speed) pSmp->nC5Speed = 22050;
	pSmp->RelativeTone = 0;
	pSmp->nFineTune = 0;
	if (m_nType & MOD_TYPE_XM) FrequencyToTranspose(pSmp);
	dwMemPos += BigEndian(pvh->dwSize) + 8;
	while (dwMemPos + 8 < dwFileLength)
	{
		DWORD dwChunkId = *((LPDWORD)(lpMemFile+dwMemPos));
		DWORD dwChunkLen = BigEndian(*((LPDWORD)(lpMemFile+dwMemPos+4)));
		LPBYTE pChunkData = (LPBYTE)(lpMemFile+dwMemPos+8);
		// Hack for broken files: Trim claimed length if it's too long
		dwChunkLen = min(dwChunkLen, dwFileLength - dwMemPos);
		//if (dwChunkLen > dwFileLength - dwMemPos) break;
		switch(dwChunkId)
		{
		case IFFID_NAME:
			{
				const UINT len = min(dwChunkLen, MAX_SAMPLENAME - 1);
				MemsetZero(m_szNames[nSample]);
				memcpy(m_szNames[nSample], pChunkData, len);
			}
			break;
		case IFFID_BODY:
			if (!pSmp->pSample)
			{
				UINT len = dwChunkLen;
				if (len > dwFileLength - dwMemPos - 8) len = dwFileLength - dwMemPos - 8;
				if (len > 4)
				{
					pSmp->nLength = len;
					if ((pSmp->nLoopStart + 4 < pSmp->nLoopEnd) && (pSmp->nLoopEnd < pSmp->nLength)) pSmp->uFlags |= CHN_LOOP;
					ReadSample(pSmp, RS_PCM8S, (LPSTR)(pChunkData), len);
				}
			}
			break;
		}
		dwMemPos += dwChunkLen + 8;
	}
	return (pSmp->pSample != nullptr);
}

