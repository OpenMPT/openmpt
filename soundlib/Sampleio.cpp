/*
 * SampleIO.cpp
 * ------------
 * Purpose: Code for loading various more or less common sample and instrument formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "sndfile.h"
#include "wavConverter.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Moddoc.h"
#endif //MODPLUG_TRACKER
#include "../mptrack/Mainfrm.h" // For CriticalSection
#include "Wav.h"
#include "ITTools.h"
#include "XMTools.h"
#include "../common/StringFixer.h"
#include "../common/Reporting.h"
#include "../mptrack/version.h"
#include "ChunkReader.h"

#pragma warning(disable:4244)

bool CSoundFile::ReadSampleFromFile(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength)
//--------------------------------------------------------------------------------------------------
{
	FileReader file(reinterpret_cast<const char *>(lpMemFile), dwFileLength);
	if(!nSample || nSample >= MAX_SAMPLES) return false;
	if(!ReadWAVSample(nSample, lpMemFile, dwFileLength)
		&& !ReadXISample(nSample, lpMemFile, dwFileLength)
		&& !ReadAIFFSample(nSample, file)
		&& !ReadITSSample(nSample, lpMemFile, dwFileLength)
		&& !ReadPATSample(nSample, lpMemFile, dwFileLength)
		&& !Read8SVXSample(nSample, lpMemFile, dwFileLength)
		&& !ReadS3ISample(nSample, lpMemFile, dwFileLength))
	{
		return false;
	}
	return true;
}


bool CSoundFile::ReadInstrumentFromFile(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------------------
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


bool CSoundFile::ReadSampleAsInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------------------
{
	const uint32 *psig = reinterpret_cast<const uint32 *>(lpMemFile);
	if ((!lpMemFile) || (dwFileLength < 80)) return false;
	if (((psig[0] == LittleEndian(0x46464952)) && (psig[2] == LittleEndian(0x45564157)))	// RIFF....WAVE signature
	 || ((psig[0] == LittleEndian(0x5453494C)) && (psig[2] == LittleEndian(0x65766177)))	// LIST....wave
	 || (psig[76/4] == LittleEndian(0x53524353))											// S3I signature
	 || ((psig[0] == LittleEndian(0x4D524F46)) && (psig[2] == LittleEndian(0x46464941)))	// AIFF signature
	 || ((psig[0] == LittleEndian(0x4D524F46)) && (psig[2] == LittleEndian(0x58565338)))	// 8SVX signature
	 || (psig[0] == LittleEndian(ITSample::magic))											// ITS signature
	)
	{
		// Loading Instrument

		ModInstrument *pIns;

		try
		{
			pIns = new ModInstrument();
		} catch(MPTMemoryException)
		{
			return false;
		}
		 
		DestroyInstrument(nInstr, deleteAssociatedSamples);
		Instruments[nInstr] = pIns;

		// Scanning free sample
		SAMPLEINDEX nSample = GetNextFreeSample(nInstr);
		if(nSample == SAMPLEINDEX_INVALID)
		{
			return false;
		} else if(nSample > GetNumSamples())
		{
			m_nSamples = nSample;
		}

		Instruments[nInstr]->AssignSample(nSample);

		ReadSampleFromFile(nSample, lpMemFile, dwFileLength);
		return true;
	}
	return false;
}


bool CSoundFile::DestroyInstrument(INSTRUMENTINDEX nInstr, deleteInstrumentSamples removeSamples)
//-----------------------------------------------------------------------------------------------
{
	if ((!nInstr) || (nInstr > m_nInstruments)) return false;
	if (!Instruments[nInstr]) return true;

#ifdef MODPLUG_TRACKER
	if(removeSamples == askDeleteAssociatedSamples)
	{
		ConfirmAnswer result = Reporting::Confirm("Remove samples associated with an instrument if they are unused?", "Removing instrument", true);
		if(result == cnfCancel)
		{
			return false;
		}
		removeSamples = (result == cnfYes) ? deleteAssociatedSamples : doNoDeleteAssociatedSamples;
	}
#endif // MODPLUG_TRACKER
	if(removeSamples == deleteAssociatedSamples)
	{
		RemoveInstrumentSamples(nInstr);
	}

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

	CriticalSection cs;

	ModInstrument *pIns = Instruments[nInstr];
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
	if(Instruments[nInstr] == nullptr)
	{
		return false;
	}

	vector<bool> keepSamples(GetNumSamples() + 1, true);

	// Check which samples are used by the instrument we are going to nuke.
	std::set<SAMPLEINDEX> referencedSamples = Instruments[nInstr]->GetSamples();
	for(std::set<SAMPLEINDEX>::const_iterator sample = referencedSamples.begin(); sample != referencedSamples.end(); sample++)
	{
		if((*sample) <= GetNumSamples())
			keepSamples[*sample] = false;
	}

	// Check if any of those samples are referenced by other instruments as well, in which case we want to keep them of course.
	for(INSTRUMENTINDEX nIns = 1; nIns <= GetNumInstruments(); nIns++) if (Instruments[nIns] != nullptr && nIns != nInstr)
	{
		Instruments[nIns]->GetSamples(keepSamples);
	}

	// Now nuke the selected samples.
	RemoveSelectedSamples(keepSamples);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// I/O From another song
//

bool CSoundFile::ReadInstrumentFromSong(INSTRUMENTINDEX targetInstr, const CSoundFile *pSrcSong, INSTRUMENTINDEX sourceInstr)
//---------------------------------------------------------------------------------------------------------------------------
{
	if ((!pSrcSong) || (!sourceInstr) || (sourceInstr > pSrcSong->GetNumInstruments())
		|| (targetInstr >= MAX_INSTRUMENTS) || (!pSrcSong->Instruments[sourceInstr]))
	{
		return false;
	}
	if (m_nInstruments < targetInstr) m_nInstruments = targetInstr;

	ModInstrument *pIns;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}
		
	DestroyInstrument(targetInstr, deleteAssociatedSamples);

	Instruments[targetInstr] = pIns;
	*pIns = *pSrcSong->Instruments[sourceInstr];

	vector<SAMPLEINDEX> sourceSample;	// Sample index in source song
	vector<SAMPLEINDEX> targetSample;	// Sample index in target song
	SAMPLEINDEX targetIndex = 0;		// Next index for inserting sample

	for(size_t i = 0; i < CountOf(pIns->Keyboard); i++)
	{
		const SAMPLEINDEX sourceIndex = pIns->Keyboard[i];
		if(sourceIndex > 0 && sourceIndex <= pSrcSong->GetNumSamples())
		{
			const vector<SAMPLEINDEX>::const_iterator entry = std::find(sourceSample.begin(), sourceSample.end(), sourceIndex);
			if(entry == sourceSample.end())
			{
				// Didn't consider this sample yet, so add it to our map.
				targetIndex = GetNextFreeSample(targetInstr, targetIndex + 1);
				if(targetIndex <= GetModSpecifications().samplesMax)
				{
					sourceSample.push_back(sourceIndex);
					targetSample.push_back(targetIndex);
					pIns->Keyboard[i] = targetIndex;
				} else
				{
					pIns->Keyboard[i] = 0;
				}
			} else
			{
				// Sample reference has already been created, so only need to update the sample map.
				pIns->Keyboard[i] = *(entry - sourceSample.begin() + targetSample.begin());
			}
		} else
		{
			// Invalid or no source sample
			pIns->Keyboard[i] = 0;
		}
	}

	pIns->Convert(pSrcSong->GetType(), GetType());

	// Copy all referenced samples over
	for(size_t i = 0; i < targetSample.size(); i++)
	{
		ReadSampleFromSong(targetSample[i], pSrcSong, sourceSample[i]);
	}

	return true;
}


bool CSoundFile::ReadSampleFromSong(SAMPLEINDEX targetSample, const CSoundFile *pSrcSong, SAMPLEINDEX sourceSample)
//-----------------------------------------------------------------------------------------------------------------
{
	if ((!pSrcSong) || (!sourceSample) || (sourceSample > pSrcSong->m_nSamples) || (targetSample >= GetModSpecifications().samplesMax))
	{
		return false;
	}

	const ModSample *pSourceSample = &pSrcSong->Samples[sourceSample];

	if (m_nSamples < targetSample) m_nSamples = targetSample;
	if (Samples[targetSample].pSample)
	{
		Samples[targetSample].nLength = 0;
		FreeSample(Samples[targetSample].pSample);
	}
	Samples[targetSample] = *pSourceSample;
	if (pSourceSample->pSample)
	{
		UINT nSize = pSourceSample->GetSampleSizeInBytes();
		Samples[targetSample].pSample = AllocateSample(nSize + 8);
		if (Samples[targetSample].pSample)
		{
			memcpy(Samples[targetSample].pSample, pSourceSample->pSample, nSize);
			AdjustSampleLoop(&Samples[targetSample]);
		}
	}

	Samples[targetSample].Convert(pSrcSong->GetType(), GetType());

	return true;
}


////////////////////////////////////////////////////////////////////////////////
// WAV Open

#define IFFID_pcm	0x206d6370
#define IFFID_fact	0x74636166

extern BOOL IMAADPCMUnpack16(signed short *pdest, UINT nLen, LPBYTE psrc, DWORD dwBytes, UINT pkBlkAlign);

bool CSoundFile::ReadWAVSample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength, DWORD *pdwWSMPOffset)
//-------------------------------------------------------------------------------------------------------------------
{
	DWORD dwMemPos = 0, dwDataPos;
	WAVEFILEHEADER *phdr = (WAVEFILEHEADER *)lpMemFile;
	WAVEFORMATHEADER *pfmt, *pfmtpk;
	WAVEDATAHEADER *pdata;
	WAVESMPLHEADER *psh;
	WAVEEXTRAHEADER *pxh;
	DWORD dwInfoList, dwFact, dwSamplesPerBlock;
	
	if (!nSample || !lpMemFile || (dwFileLength < sizeof(WAVEFILEHEADER) + sizeof(WAVEFORMATHEADER)))
		return false;
	if ((phdr->id_RIFF != LittleEndian(IFFID_RIFF) && phdr->id_RIFF != LittleEndian(IFFID_LIST))
		|| (phdr->id_WAVE != LittleEndian(IFFID_WAVE) && phdr->id_WAVE != LittleEndian(IFFID_wave)))
		return false;

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
		uint32 dwIFFID = LittleEndian(*reinterpret_cast<uint32 *>(lpMemFile + dwMemPos));
		uint32 dwLen = LittleEndian(*reinterpret_cast<uint32 *>(lpMemFile + dwMemPos + 4));
		if ((dwLen > dwFileLength) || (dwMemPos + 8 + dwLen > dwFileLength))
			break;

		switch(dwIFFID)
		{
		// "fmt "
		case IFFID_fmt:
			if (pfmt) break;
			if (dwLen + 8 >= sizeof(WAVEFORMATHEADER))
			{
				pfmt = (WAVEFORMATHEADER *)(lpMemFile + dwMemPos);
				if (dwLen + 8 >= sizeof(WAVEFORMATHEADER)+4)
				{
					dwSamplesPerBlock = LittleEndianW(*reinterpret_cast<uint16 *>(lpMemFile + dwMemPos + sizeof(WAVEFORMATHEADER) + 2));
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
			if (!dwFact) dwFact = LittleEndian(*reinterpret_cast<uint32 *>(lpMemFile + dwMemPos + 8));
			break;
		// "data"
		case IFFID_data:
			if(dwLen + 8 >= sizeof(WAVEDATAHEADER) && !pdata)
			{
				pdata = (WAVEDATAHEADER *)(lpMemFile + dwMemPos);
				dwDataPos = dwMemPos + 8;
			}
			break;
		// "xtra"
		case 0x61727478:
			if(dwLen + 8 >= sizeof(WAVEEXTRAHEADER))
			{
				pxh = (WAVEEXTRAHEADER *)(lpMemFile + dwMemPos);
				if(LittleEndian(pxh->xtra_len) + 8 > dwFileLength - dwMemPos)
				{
					pxh = nullptr;
				}
			}
			break;
		// "smpl"
		case 0x6C706D73:
			if(dwLen + 8 >= sizeof(WAVESMPLHEADER))
				psh = (WAVESMPLHEADER *)(lpMemFile + dwMemPos);
			break;
		// "LIST"."info"
		case IFFID_LIST:
			if(*reinterpret_cast<uint32 *>(lpMemFile + dwMemPos + 8) == LittleEndian(IFFID_INFO))	// "INFO"
				dwInfoList = dwMemPos;
			break;
		// "wsmp":
		case IFFID_wsmp:
			if (pdwWSMPOffset) *pdwWSMPOffset = dwMemPos;
			break;
		}
		dwMemPos += dwLen + 8;

		if((dwLen % 2) != 0 && dwMemPos < dwFileLength)
		{
			// Ignore the padding byte. Problem: Old versions of OpenMPT didn't write padding. -_-
			// We will just do a heuristic check here: As OpenMPT always writes the "smpl" chunk after the "data" chunk
			// (which happens to be the only chunk that has to be padded in OpenMPT's case), we just check if we can read "smpl"
			// without incrementing the position.
			if((dwMemPos + 8 < dwFileLength) && LittleEndian(*reinterpret_cast<uint32 *>(lpMemFile + dwMemPos)) == 0x6C706D73)
			{
				continue;
			}
			dwMemPos++;
		}
	}
	if (!pdata || !pfmt || pdata->length < 4)
		return false;

	if ((pfmtpk) && (pfmt))
	{
		if (pfmt->format != 1)
		{
			WAVEFORMATHEADER *tmp = pfmt;
			pfmt = pfmtpk;
			pfmtpk = tmp;
			if(pfmtpk->format != 0x11 || pfmtpk->bitspersample != 4
				|| pfmtpk->channels != 1)
				return false;
		} else pfmtpk = NULL;
	}

	uint16 format = LittleEndianW(pfmt->format);
	uint16 channels = LittleEndianW(pfmt->channels);
	uint16 bitsPerSmp = LittleEndianW(pfmt->bitspersample);

	// WAVE_FORMAT_PCM, WAVE_FORMAT_IEEE_FLOAT, WAVE_FORMAT_EXTENSIBLE
	if (((format != 1 && format != 0xFFFE)
	 && (format != 3 || bitsPerSmp != 32)) //Microsoft IEEE FLOAT
	 || channels > 2
	 || channels == 0
	 || (bitsPerSmp & 7) != 0
	 || bitsPerSmp == 0
	 || bitsPerSmp > 32
	) return false;

	DestroySample(nSample);
	UINT nType = RS_PCM8U;
	if(channels == 1)
	{
		if(bitsPerSmp == 24)
			nType = RS_PCM24S; 
		else if(bitsPerSmp == 32)
			nType = RS_PCM32S; 
		else if(bitsPerSmp == 16)
			nType = RS_PCM16S;
		else
			nType = RS_PCM8U;
	} else
	{
		if(bitsPerSmp == 24)
			nType = RS_STIPCM24S; 
		else if(bitsPerSmp == 32)
			nType = RS_STIPCM32S; 
		else if(bitsPerSmp == 16)
			nType = RS_STIPCM16S;
		else
			nType = RS_STIPCM8U;
	}

	UINT samplesize = channels * (bitsPerSmp / 8);

	ModSample *pSmp = &Samples[nSample];
	if (pSmp->pSample)
	{
		FreeSample(pSmp->pSample);
		pSmp->pSample = nullptr;
		pSmp->nLength = 0;
	}
	pSmp->nLength = LittleEndian(pdata->length) / samplesize;
	pSmp->nLoopStart = pSmp->nLoopEnd = 0;
	pSmp->nSustainStart = pSmp->nSustainEnd = 0;
	pSmp->nC5Speed = LittleEndian(pfmt->freqHz);
	pSmp->nPan = 128;
	pSmp->nVolume = 256;
	pSmp->nGlobalVol = 64;
	pSmp->uFlags = (bitsPerSmp > 8) ? CHN_16BIT : 0;
	pSmp->RelativeTone = 0;
	pSmp->nFineTune = 0;
	pSmp->nVibType = pSmp->nVibSweep = pSmp->nVibDepth = pSmp->nVibRate = 0;
	pSmp->filename[0] = 0;
	MemsetZero(m_szNames[nSample]);
	if (pSmp->nLength > MAX_SAMPLE_LENGTH) pSmp->nLength = MAX_SAMPLE_LENGTH;

	// IMA ADPCM 4:1
	if (pfmtpk)
	{
		if (dwFact < 4) dwFact = LittleEndian(pdata->length) * 2;
		pSmp->nLength = dwFact;
		pSmp->pSample = AllocateSample(pSmp->nLength * 2 + 16);
		IMAADPCMUnpack16((signed short *)pSmp->pSample, pSmp->nLength,
						 (LPBYTE)(lpMemFile + dwDataPos), dwFileLength - dwDataPos, LittleEndianW(pfmtpk->samplesize));
		AdjustSampleLoop(pSmp);
	} else
	{
		ReadSample(pSmp, nType, (LPSTR)(lpMemFile + dwDataPos), dwFileLength - dwDataPos, format);
	}
	
	bool fixSampleLoops = false;

	// LIST field
	if (dwInfoList)
	{
		uint32 dwLSize = LittleEndian(*reinterpret_cast<uint32 *>(lpMemFile + dwInfoList + 4)) + 8;
		uint32 d = 12;
		while (d + 8 < dwLSize)
		{
			if (!lpMemFile[dwInfoList + d]) d++;
			uint32 id = LittleEndian(*reinterpret_cast<uint32 *>(lpMemFile + dwInfoList + d));
			uint32 len = LittleEndian(*reinterpret_cast<uint32 *>(lpMemFile + dwInfoList + d + 4));

			if ((dwInfoList + d + 8 + len <= dwFileLength) && (len))
			{
				switch(id)
				{
				case IFFID_INAM:
					StringFixer::ReadString<StringFixer::nullTerminated>(m_szNames[nSample], reinterpret_cast<const char *>(lpMemFile + dwInfoList + d + 8), len);
					if (phdr->id_RIFF != LittleEndian(IFFID_RIFF))
					{
						// DLS sample -> sample filename
						StringFixer::ReadString<StringFixer::nullTerminated>(pSmp->filename, reinterpret_cast<const char *>(lpMemFile + dwInfoList + d + 8), len);
					}
					break;

				case IFFID_ISFT:
					{
						// MPT / old versions of OpenMPT stored the sample loop information incorrectly:
						// In the RIFF standard, the loop end point is *inclusive*, in OpenMPT it's *exclusive*.
						char *softwareId = reinterpret_cast<char *>(lpMemFile + dwInfoList + d + 8);
						fixSampleLoops = !strncmp(softwareId, "Modplug Tracker", min(len, 16));
					}
					break;
				}
			}
			d += 8 + len;
		}
	}

	// smpl field
	if (psh)
	{
		pSmp->nLoopStart = pSmp->nLoopEnd = 0;
		int numLoops = LittleEndian(psh->dwSampleLoops);
		if(numLoops && (sizeof(WAVESMPLHEADER) + numLoops * sizeof(SAMPLELOOPSTRUCT) <= LittleEndian(psh->smpl_len) + 8))
		{
			SAMPLELOOPSTRUCT *psl = (SAMPLELOOPSTRUCT *)(&psh[1]);
			if(numLoops > 1)
			{
				pSmp->uFlags |= (CHN_LOOP | CHN_SUSTAINLOOP);
				if(LittleEndian(psl[0].dwLoopType)) pSmp->uFlags |= CHN_PINGPONGSUSTAIN;
				if(LittleEndian(psl[1].dwLoopType)) pSmp->uFlags |= CHN_PINGPONGLOOP;
				pSmp->nSustainStart = LittleEndian(psl[0].dwLoopStart);
				pSmp->nSustainEnd = LittleEndian(psl[0].dwLoopEnd);
				pSmp->nLoopStart = LittleEndian(psl[1].dwLoopStart);
				pSmp->nLoopEnd = LittleEndian(psl[1].dwLoopEnd);
			} else
			{
				pSmp->uFlags |= CHN_LOOP;
				if(LittleEndian(psl->dwLoopType)) pSmp->uFlags |= CHN_PINGPONGLOOP;
				pSmp->nLoopStart = LittleEndian(psl->dwLoopStart);
				pSmp->nLoopEnd = LittleEndian(psl->dwLoopEnd);
			}

			if(!fixSampleLoops)
			{
				// RIFF loop end points are inclusive
				if(pSmp->nLoopEnd < pSmp->nLength && (pSmp->uFlags & CHN_LOOP))
				{
					pSmp->nLoopEnd++;
				}

				if(pSmp->nSustainEnd < pSmp->nLength && (pSmp->uFlags & CHN_SUSTAINLOOP))
				{
					pSmp->nSustainEnd++;
				}
			}

			if (pSmp->nLoopStart >= pSmp->nLoopEnd) pSmp->uFlags &= ~(CHN_LOOP|CHN_PINGPONGLOOP);
			if (pSmp->nSustainStart >= pSmp->nSustainEnd) pSmp->uFlags &= ~(CHN_PINGPONGLOOP|CHN_PINGPONGSUSTAIN);
		}
	}

	// xtra field
	if (pxh)
	{
		DWORD extFlags = LittleEndian(pxh->dwFlags);
		if (extFlags & CHN_PINGPONGLOOP) pSmp->uFlags |= CHN_PINGPONGLOOP;
		if (extFlags & CHN_SUSTAINLOOP) pSmp->uFlags |= CHN_SUSTAINLOOP;
		if (extFlags & CHN_PINGPONGSUSTAIN) pSmp->uFlags |= CHN_PINGPONGSUSTAIN;
		if (extFlags & CHN_PANNING) pSmp->uFlags |= CHN_PANNING;

		pSmp->nPan = LittleEndianW(pxh->wPan);
		pSmp->nVolume = LittleEndianW(pxh->wVolume);
		pSmp->nGlobalVol = LittleEndianW(pxh->wGlobalVol);
		pSmp->nVibType = pxh->nVibType;
		pSmp->nVibSweep = pxh->nVibSweep;
		pSmp->nVibDepth = pxh->nVibDepth;
		pSmp->nVibRate = pxh->nVibRate;
		// Name present (clipboard only)
		UINT xtrabytes = LittleEndian(pxh->xtra_len) - (sizeof(WAVEEXTRAHEADER) - 8);
		LPSTR pszTextEx = (LPSTR)(pxh + 1); 
		if (xtrabytes >= MAX_SAMPLENAME)
		{
			StringFixer::ReadString<StringFixer::nullTerminated>(m_szNames[nSample], pszTextEx, MAX_SAMPLENAME);
			pszTextEx += MAX_SAMPLENAME;
			xtrabytes -= MAX_SAMPLENAME;
			if(xtrabytes > 0)
			{
				StringFixer::ReadString<StringFixer::nullTerminated>(pSmp->filename, pszTextEx, xtrabytes);
			}
		}
	}
	pSmp->Convert(MOD_TYPE_IT, GetType());
	return true;
}


///////////////////////////////////////////////////////////////
// Save WAV


bool CSoundFile::SaveWAVSample(UINT nSample, const LPCSTR lpszFileName) const
//---------------------------------------------------------------------------
{
	char *softwareId = "OpenMPT " MPT_VERSION_STR;
	size_t softwareIdLength = strlen(softwareId) + 1;

	WAVEFILEHEADER header;
	WAVEFORMATHEADER format;
	WAVEDATAHEADER data;
	WAVESAMPLERINFO smpl;
	WAVELISTHEADER list;
	WAVEEXTRAHEADER extra;
	const ModSample &sample = Samples[nSample];
	FILE *f;

	if ((f = fopen(lpszFileName, "wb")) == NULL) return false;
	MemsetZero(extra);
	MemsetZero(smpl);
	header.id_RIFF = LittleEndian(IFFID_RIFF);
	header.filesize = sizeof(header) - 8 + sizeof(format) + sizeof(data) + sizeof(extra)
		+ sizeof(list) + 8 + softwareIdLength + 8 + 32; // LIST(INAM, ISFT)
	header.id_WAVE = LittleEndian(IFFID_WAVE);
	format.id_fmt = LittleEndian(IFFID_fmt);
	format.hdrlen = LittleEndian(16);
	format.format = LittleEndianW(1);
	if(!(GetType() & (MOD_TYPE_MOD | MOD_TYPE_XM)))
		format.freqHz = LittleEndian(sample.nC5Speed);
	else
		format.freqHz = LittleEndian(TransposeToFrequency(sample.RelativeTone, sample.nFineTune));
	format.channels = LittleEndianW(sample.GetNumChannels());
	format.bitspersample = LittleEndianW(sample.GetElementarySampleSize() * 8);
	format.samplesize = LittleEndianW(sample.GetBytesPerSample());
	format.bytessec = LittleEndian(format.freqHz * format.samplesize);

	data.id_data = LittleEndian(IFFID_data);
	UINT nType;
	data.length = sample.GetSampleSizeInBytes();
	if (sample.uFlags & CHN_STEREO)
	{
		nType = (sample.uFlags & CHN_16BIT) ? RS_STIPCM16S : RS_STIPCM8U;
	} else
	{
		nType = (sample.uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8U;
	}

	header.filesize += data.length;
	if((data.length % 2) != 0)
	{
		// Write padding byte if sample size is odd.
		header.filesize++;
	}

	// "smpl" field
	smpl.wsiHdr.smpl_id = LittleEndian(IFFID_smpl);
	smpl.wsiHdr.smpl_len = sizeof(WAVESMPLHEADER) - 8;
	if (sample.nC5Speed >= 256)
		smpl.wsiHdr.dwSamplePeriod = LittleEndian(1000000000 / sample.nC5Speed);
	else
		smpl.wsiHdr.dwSamplePeriod = LittleEndian(22675);	// 44100 Hz

	smpl.wsiHdr.dwBaseNote = LittleEndian(NOTE_MIDDLEC - NOTE_MIN);

	// Write loops
	if((sample.uFlags & CHN_SUSTAINLOOP) != 0)
	{
		smpl.wsiLoops[smpl.wsiHdr.dwSampleLoops++].SetLoop(sample.nSustainStart, sample.nSustainEnd, (sample.uFlags & CHN_PINGPONGSUSTAIN) != 0);
		smpl.wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT);
	}
	if((sample.uFlags & CHN_LOOP) != 0)
	{
		smpl.wsiLoops[smpl.wsiHdr.dwSampleLoops++].SetLoop(sample.nLoopStart, sample.nLoopEnd, (sample.uFlags & CHN_PINGPONGLOOP) != 0);
		smpl.wsiHdr.smpl_len += sizeof(SAMPLELOOPSTRUCT);
	}

	// Update file length in header
	header.filesize += smpl.wsiHdr.smpl_len + 8;

	header.filesize = LittleEndian(header.filesize);
	data.length = LittleEndian(data.length);
	smpl.wsiHdr.smpl_len = LittleEndian(smpl.wsiHdr.smpl_len);
	fwrite(&header, 1, sizeof(header), f);
	fwrite(&format, 1, sizeof(format), f);
	fwrite(&data, 1, sizeof(data), f);
	WriteSample(f, &sample, nType);
	if((data.length % 2) != 0)
	{
		// Write padding byte if sample size is odd.
		int8 padding = 0;
		fwrite(&padding, 1, 1, f);
	}
	fwrite(&smpl, 1, smpl.wsiHdr.smpl_len + 8, f);

	// "LIST" field
	list.list_id = LittleEndian(IFFID_LIST);
	list.list_len = LittleEndian(sizeof(list) - 8	// LIST
					+ 8 + 32						// "INAM".dwLen.szSampleName
					+ 8 + softwareIdLength);		// "ISFT".dwLen.softwareId.0
	list.info = LittleEndian(IFFID_INFO);
	fwrite(&list, 1, sizeof(list), f);

	list.list_id = LittleEndian(IFFID_INAM);		// "INAM"
	list.list_len = LittleEndian(32);
	fwrite(&list, 1, 8, f);
	fwrite(m_szNames[nSample], 1, 32, f);

	list.list_id = LittleEndian(IFFID_ISFT);		// "ISFT"
	list.list_len = LittleEndian(softwareIdLength);
	fwrite(&list, 1, 8, f);
	fwrite(softwareId, 1, list.list_len, f);

	// "xtra" field
	extra.xtra_id = LittleEndian(IFFID_xtra);
	extra.xtra_len = LittleEndian(sizeof(extra) - 8);

	extra.dwFlags = LittleEndian(sample.uFlags);
	extra.wPan = LittleEndianW(sample.nPan);
	extra.wVolume = LittleEndianW(sample.nVolume);
	extra.wGlobalVol = LittleEndianW(sample.nGlobalVol);
	extra.wReserved = 0;

	extra.nVibType = sample.nVibType;
	extra.nVibSweep = sample.nVibSweep;
	extra.nVibDepth = sample.nVibDepth;
	extra.nVibRate = sample.nVibRate;
	if((GetType() & MOD_TYPE_XM) && (extra.nVibDepth | extra.nVibRate))
	{
		// XM vibrato is upside down
		extra.nVibSweep = 255 - extra.nVibSweep;
	}

	fwrite(&extra, 1, sizeof(extra), f);
	fclose(f);
	return true;
}

///////////////////////////////////////////////////////////////
// Save RAW

bool CSoundFile::SaveRAWSample(UINT nSample, const LPCSTR lpszFileName) const
//---------------------------------------------------------------------------
{
	const ModSample *pSmp = &Samples[nSample];
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

#pragma pack(push, 1)

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
//	It can be represented like this (the envelope is totally bogus, it is
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
// bit 6: off/on envelopes
// bit 7: off/on clamped release (6th point, env)

typedef struct GF1LAYER
{
	BYTE previous;			// If !=0 the wavesample to use is from the previous layer. The waveheader is still needed
	BYTE id;				// Layer id: 0-3
	DWORD size;				// data size in bytes in the layer, without the header. to skip to next layer for example:
	BYTE samples;			// number of wavesamples
	BYTE reserved[40];
} GF1LAYER;

#pragma pack(pop)

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


void PatchToSample(CSoundFile *that, UINT nSample, LPBYTE lpStream, DWORD dwMemLength)
//------------------------------------------------------------------------------------
{
	ModSample &sample = that->GetSample(nSample);
	DWORD dwMemPos = sizeof(GF1SAMPLEHEADER);
	GF1SAMPLEHEADER *psh = (GF1SAMPLEHEADER *)(lpStream);
	UINT nSmpType;

	if (dwMemLength < sizeof(GF1SAMPLEHEADER)) return;
	if (psh->name[0])
	{
		memcpy(that->m_szNames[nSample], psh->name, 7);
		that->m_szNames[nSample][7] = 0;
	}
	sample.filename[0] = 0;
	sample.nGlobalVol = 64;
	sample.uFlags = (psh->flags & 1) ? CHN_16BIT : 0;
	if (psh->flags & 4) sample.uFlags |= CHN_LOOP;
	if (psh->flags & 8) sample.uFlags |= CHN_PINGPONGLOOP;
	sample.nLength = psh->length;
	sample.nLoopStart = psh->loopstart;
	sample.nLoopEnd = psh->loopend;
	sample.nC5Speed = psh->freq;
	sample.RelativeTone = 0;
	sample.nFineTune = 0;
	sample.nVolume = 256;
	sample.nPan = (psh->balance << 4) + 8;
	if (sample.nPan > 256) sample.nPan = 128;
	sample.nVibType = 0;
	sample.nVibSweep = psh->vibrato_sweep;
	sample.nVibDepth = psh->vibrato_depth;
	sample.nVibRate = psh->vibrato_rate/4;
	that->FrequencyToTranspose(&sample);
	sample.RelativeTone += 84 - PatchFreqToNote(psh->root_freq);
	if (psh->scale_factor) sample.RelativeTone -= psh->scale_frequency - 60;
	sample.nC5Speed = that->TransposeToFrequency(sample.RelativeTone, sample.nFineTune);
	if (sample.uFlags & CHN_16BIT)
	{
		nSmpType = (psh->flags & 2) ? RS_PCM16U : RS_PCM16S;
		sample.nLength >>= 1;
		sample.nLoopStart >>= 1;
		sample.nLoopEnd >>= 1;
	} else
	{
		nSmpType = (psh->flags & 2) ? RS_PCM8U : RS_PCM8S;
	}
	that->ReadSample(&sample, nSmpType, (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
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
	ModInstrument *pIns;
	DWORD dwMemPos = sizeof(GF1PATCHFILEHEADER)+sizeof(GF1INSTRUMENT)+sizeof(GF1LAYER);
	UINT nSamples;

	if ((!lpStream) || (dwMemLength < 512)
	 || (phdr->gf1p != 0x50314647) || (phdr->atch != 0x48435441)
	 || (phdr->version[3] != 0) || (phdr->id[9] != 0)
	 || (phdr->instrum < 1) || (!phdr->samples)
	 || (!pih->layers) || (!plh->samples)) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);

	Instruments[nInstr] = pIns;
	nSamples = plh->samples;
	if (nSamples > 16) nSamples = 16;
	memcpy(pIns->name, pih->name, 16);
	pIns->name[16] = 0;
	pIns->nFadeOut = 2048;
	if (GetType() & (MOD_TYPE_IT|MOD_TYPE_MPT))
	{
		pIns->nNNA = NNA_NOTEOFF;
		pIns->nDNA = DNA_NOTEFADE;
	}
	UINT nFreeSmp = 0;
	UINT nMinSmpNote = 0xff;
	UINT nMinSmp = 0;
	for (UINT iSmp=0; iSmp<nSamples; iSmp++)
	{
		// Find a free sample
		nFreeSmp = GetNextFreeSample(nInstr, nFreeSmp + 1);
		if (nFreeSmp == SAMPLEINDEX_INVALID) break;
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
				if(!psh->scale_factor)
					pIns->NoteMap[k] = NOTE_MIDDLEC;

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

bool CSoundFile::ReadS3ISample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------
{
	S3ISAMPLESTRUCT *pss = (S3ISAMPLESTRUCT *)lpMemFile;
	ModSample *pSmp = &Samples[nSample];
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

	pSmp->Convert(MOD_TYPE_S3M, GetType());

	if (pss->flags & 0x01) pSmp->uFlags |= CHN_LOOP;
	flags = (pss->flags & 0x04) ? RS_PCM16U : RS_PCM8U;
	if (pss->flags & 0x02) flags |= RSF_STEREO;
	ReadSample(pSmp, flags, (LPSTR)(lpMemFile+dwMemPos), dwFileLength-dwMemPos);
	return true;
}


/////////////////////////////////////////////////////////////
// XI Instruments


bool CSoundFile::ReadXIInstrument(INSTRUMENTINDEX nInstr, const LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------------------------
{
	if(dwFileLength < sizeof(XIInstrumentHeader))
	{
		return false;
	}

	XIInstrumentHeader header = *reinterpret_cast<const XIInstrumentHeader *>(lpMemFile);
	header.ConvertEndianness();

	DWORD dwMemPos = sizeof(XIInstrumentHeader);
	
	if(memcmp(header.signature, "Extended Instrument: ", 21)
		|| header.version != 0x102
		|| header.eof != 0x1A
		|| header.numSamples > 32)
	{
		return false;
	}

	ModInstrument *pIns;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);
	if(nInstr > m_nInstruments) m_nInstruments = nInstr;
	Instruments[nInstr] = pIns;

	header.ConvertToMPT(*pIns);

	// Translate sample map and find available sample slots
	vector<SAMPLEINDEX> sampleMap(header.numSamples);
	SAMPLEINDEX maxSmp = 0;

	for(size_t i = 0 + 12; i < 96 + 12; i++)
	{
		if(pIns->Keyboard[i] >= header.numSamples)
		{
			continue;
		}

		if(sampleMap[pIns->Keyboard[i]] == 0)
		{
			// Find slot for this sample
			maxSmp = GetNextFreeSample(nInstr, maxSmp + 1);
			if(maxSmp != SAMPLEINDEX_INVALID)
			{
				sampleMap[pIns->Keyboard[i]] = maxSmp;
			}
		}
		pIns->Keyboard[i] = sampleMap[pIns->Keyboard[i]];
	}

	if(m_nSamples < maxSmp)
	{
		m_nSamples = maxSmp;
	}

	vector<UINT> sampleFlags(header.numSamples);

	// Read sample headers
	for(SAMPLEINDEX i = 0; i < header.numSamples; i++)
	{
		if (dwMemPos + sizeof(XMSample) > dwFileLength)
		{
			break;
		}

		const XMSample *xmSample= reinterpret_cast<const XMSample *>(lpMemFile + dwMemPos);
		dwMemPos += sizeof(XMSample);

		if(!sampleMap[i])
		{
			continue;
		}

		ModSample &mptSample = Samples[sampleMap[i]];
		xmSample->ConvertToMPT(mptSample);
		header.instrument.ApplyAutoVibratoToMPT(mptSample);
		mptSample.Convert(MOD_TYPE_XM, GetType());

		StringFixer::ReadString<StringFixer::spacePadded>(mptSample.filename, xmSample->name);
		StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[sampleMap[i]], xmSample->name);

		sampleFlags[i] = xmSample->GetSampleFormat();
	}

	// Read sample data
	for(SAMPLEINDEX i = 0; i < header.numSamples; i++)
	{
		if(dwMemPos < dwFileLength && sampleMap[i])
		{
			dwMemPos += ReadSample(&Samples[sampleMap[i]], sampleFlags[i], (LPSTR)(lpMemFile + dwMemPos), dwFileLength - dwMemPos);
		}
	}

	pIns->Convert(MOD_TYPE_XM, GetType());

	// -> CODE#0027
	// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Leave if no extra instrument settings are available (end of file reached)
	if(dwMemPos < dwFileLength)
	{
		ReadExtendedInstrumentProperties(pIns, lpMemFile + dwMemPos, dwFileLength - dwMemPos);
	}
	// -! NEW_FEATURE#0027

	return true;
}


bool CSoundFile::SaveXIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName) const
//----------------------------------------------------------------------------------------
{
	ModInstrument *pIns = Instruments[nInstr];
	if(pIns == nullptr || lpszFileName == nullptr)
	{
		return false;
	}

	FILE *f;
	if((f = fopen(lpszFileName, "wb")) == nullptr)
	{
		return false;
	}

	// Create file header
	XIInstrumentHeader header;
	header.ConvertToXM(*pIns, false);

	const vector<SAMPLEINDEX> samples = header.instrument.GetSampleList(*pIns, false);
	if(samples.size() > 0 && samples[0] <= GetNumSamples())
	{
		// Copy over auto-vibrato settings of first sample
		header.instrument.ApplyAutoVibratoToXM(Samples[samples[0]], GetType());
	}

	fwrite(&header, 1, sizeof(XIInstrumentHeader), f);

	vector<UINT> sampleFlags(samples.size());

	// XI Sample Headers
	for(SAMPLEINDEX i = 0; i < samples.size(); i++)
	{
		XMSample xmSample;
		if(samples[i] <= GetNumSamples())
		{
			xmSample.ConvertToXM(Samples[samples[i]], GetType(), false);
		} else
		{
			MemsetZero(xmSample);
		}
		sampleFlags[i] = xmSample.GetSampleFormat();

		StringFixer::WriteString<StringFixer::spacePadded>(xmSample.name, m_szNames[samples[i]]);

		fwrite(&xmSample, 1, sizeof(xmSample), f);
	}

	// XI Sample Data
	for(SAMPLEINDEX i = 0; i < samples.size(); i++)
	{
		if(samples[i] <= GetNumSamples())
		{
			WriteSample(f, &Samples[samples[i]], sampleFlags[i]);
		}
	}

	int32 code = 'MPTX';
	fwrite(&code, 1, sizeof(int32), f);		// Write extension tag
	WriteInstrumentHeaderStruct(pIns, f);	// Write full extended header.

	fclose(f);
	return true;
}


// Read first sample from XI file into a sample slot
bool CSoundFile::ReadXISample(SAMPLEINDEX nSample, const LPBYTE lpMemFile, DWORD dwFileLength)
//--------------------------------------------------------------------------------------------
{
	if(dwFileLength < sizeof(XIInstrumentHeader))
	{
		return false;
	}

	XIInstrumentHeader header = *reinterpret_cast<const XIInstrumentHeader *>(lpMemFile);
	header.ConvertEndianness();

	DWORD dwMemPos = sizeof(XIInstrumentHeader);

	if(memcmp(header.signature, "Extended Instrument: ", 21)
		|| header.version != 0x102
		|| header.eof != 0x1A
		|| header.numSamples > 32)
	{
		return false;
	}

	if(m_nSamples < nSample)
	{
		m_nSamples = nSample;
	}

	// Read sample header
	if (dwMemPos + sizeof(XMSample) > dwFileLength)
	{
		return false;
	}

	const XMSample *xmSample= reinterpret_cast<const XMSample *>(lpMemFile + dwMemPos);
	// Gotta skip 'em all!
	dwMemPos += sizeof(XMSample) * header.numSamples;

	ModSample &mptSample = Samples[nSample];
	xmSample->ConvertToMPT(mptSample);
	if(GetType() != MOD_TYPE_XM)
	{
		// No need to pan that single sample, thank you...
		mptSample.uFlags &= ~CHN_PANNING;
	}
	header.instrument.ApplyAutoVibratoToMPT(mptSample);
	mptSample.Convert(MOD_TYPE_XM, GetType());

	StringFixer::ReadString<StringFixer::spacePadded>(mptSample.filename, xmSample->name);
	StringFixer::ReadString<StringFixer::spacePadded>(m_szNames[nSample], xmSample->name);

	// Read sample data
	if(dwMemPos < dwFileLength)
	{
		ReadSample(&Samples[nSample], xmSample->GetSampleFormat(), (LPSTR)(lpMemFile + dwMemPos), dwFileLength - dwMemPos);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// AIFF File I/O


#pragma pack(push, 1)

// AIFF header
struct AIFFHeader
{
	// 32-Bit chunk identifiers
	enum AIFFMagic
	{
		idFORM	= 0x464F524D,
		idAIFF	= 0x41494646,
		idAIFC	= 0x41494643,
	};

	uint32 magic;	// FORM
	uint32 length;	// Size of the file, not including magic an length
	uint32 type;	// AIFF or AIFC

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(magic);
		SwapBytesBE(type);
	}
};


// General IFF Chunk header
struct AIFFChunk
{
	// 32-Bit chunk identifiers
	enum ChunkIdentifiers
	{
		idCOMM	= 0x434F4D4D,
		idSSND	= 0x53534E44,
		idINST	= 0x494E5354,
		idMARK	= 0x4D41524B,
		idNAME	= 0x4E414D45,
	};

	uint32 id;		// See ChunkIdentifiers
	uint32 length;	// Chunk size without header

	size_t GetLength() const
	{
		uint32 l = length;
		return SwapBytesBE(l);
	}

	ChunkIdentifiers GetID() const
	{
		uint32 i = id;
		return static_cast<ChunkIdentifiers>(SwapBytesBE(i));
	}
};


// "Common" chunk (in AIFC, a compression ID and compression name follows this header, but apart from that it's identical)
struct AIFFCommonChunk
{
	uint16 numChannels;
	uint32 numSampleFrames;
	uint16 sampleSize;
	uint8  sampleRate[10];		// Sample rate in 80-Bit floating point

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(numChannels);
		SwapBytesBE(numSampleFrames);
		SwapBytesBE(sampleSize);
	}

	// Convert sample rate to integer
	uint32 GetSampleRate() const
	{
		uint32 mantissa, last = 0;
		uint8 exp;

		mantissa = SwapBytesBE(static_cast<uint32 >(*reinterpret_cast<const uint32 *>(sampleRate + 2)));
		exp = 30 - sampleRate[1];
		while(exp--)
		{
			last = mantissa;
			mantissa >>= 1;
		}
		if(last & 1) mantissa++;
		return mantissa;
	}
};


// Sound chunk
struct AIFFSoundChunk
{
	uint32 offset;
	uint32 blockSize;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(offset);
		SwapBytesBE(blockSize);
	}
};


// Marker
struct AIFFMarker
{
	uint16 id;
	uint32 position;		// Position in sample
	uint8  nameLength;		// Not counting eventually existing padding byte in name string

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(id);
		SwapBytesBE(position);
	}};


// Instrument loop
struct AIFFInstrumentLoop
{
	enum PlayModes
	{
		noLoop		= 0,
		loopNormal	= 1,
		loopBidi	= 2,
	};

	uint16 playMode;
	uint16 beginLoop;		// Marker index
	uint16 endLoop;			// Marker index

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesBE(playMode);
		SwapBytesBE(beginLoop);
		SwapBytesBE(endLoop);
	}
};


struct AIFFInstrumentChunk
{
	uint8  baseNote;
	uint8  detune;
	uint8  lowNote;
	uint8  highNote;
	uint8  lowVelocity;
	uint8  highVelocity;
	uint16 gain;
	AIFFInstrumentLoop sustainLoop;
	AIFFInstrumentLoop releaseLoop;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		sustainLoop.ConvertEndianness();
		releaseLoop.ConvertEndianness();
	}
};

#pragma pack(pop)


bool CSoundFile::ReadAIFFSample(SAMPLEINDEX nSample, FileReader file)
//-------------------------------------------------------------------
{
	ChunkReader chunkFile(file);

	// Verify header
	AIFFHeader fileHeader;
	if(!chunkFile.ReadConvertEndianness(fileHeader)
		|| fileHeader.magic != AIFFHeader::idFORM
		|| (fileHeader.type != AIFFHeader::idAIFF && fileHeader.type != AIFFHeader::idAIFC))
	{
		return false;
	}

	ChunkReader::ChunkList<AIFFChunk> chunks = chunkFile.ReadChunks<AIFFChunk>(2);

	// Read COMM chunk
	FileReader commChunk(chunks.GetChunk(AIFFChunk::idCOMM));
	AIFFCommonChunk sampleInfo;
	if(!commChunk.IsValid()
		|| !commChunk.ReadConvertEndianness(sampleInfo))
	{
		return false;
	}

	// Is this a proper sample?
	if(sampleInfo.numSampleFrames == 0
		|| sampleInfo.numChannels < 1 || sampleInfo.numChannels > 2
		|| sampleInfo.sampleSize < 1 || sampleInfo.sampleSize > 32)
	{
		return false;
	}

	// Read compression type in AIFF-C files.
	char compression[4] = { 'N', 'O', 'N', 'E' };
	bool littleEndian = false;
	if(fileHeader.type == AIFFHeader::idAIFC)
	{
		if(!commChunk.ReadArray(compression))
		{
			return false;
		}
		littleEndian = !memcmp(compression, "twos", 4);
	}

	// Read SSND chunk
	FileReader soundChunk(chunks.GetChunk(AIFFChunk::idSSND));
	AIFFSoundChunk sampleHeader;
	if(!soundChunk.IsValid()
		|| !soundChunk.ReadConvertEndianness(sampleHeader)
		|| soundChunk.BytesLeft() <= sampleHeader.offset)
	{
		return false;
	}

	UINT format, wavFormat = 1;
	if(sampleInfo.numChannels == 1)
	{
		if(sampleInfo.sampleSize <= 8)
		{
			format = RS_PCM8S;
		} else if(sampleInfo.sampleSize <= 16)
		{
			format = littleEndian ? RS_PCM16S : RS_PCM16M;
		} else if(sampleInfo.sampleSize <= 24)
		{
			format = littleEndian ? RS_PCM24S : RS_PCM24S;	// FIXME: No big-endian support!
		} else if(sampleInfo.sampleSize <= 32)
		{
			format = littleEndian ? RS_PCM32S : RS_PCM32S;	// FIXME: No big-endian support!
		} else
		{
			return false;
		}
	} else
	{
		if(sampleInfo.sampleSize <= 8)
		{
			format = RS_STIPCM8S;
		} else if(sampleInfo.sampleSize <= 16)
		{
			format = littleEndian ? RS_STIPCM16S : RS_STIPCM16M;
		} else if(sampleInfo.sampleSize <= 24)
		{
			format = littleEndian ? RS_STIPCM24S : RS_STIPCM24S;	// FIXME: No big-endian support!
		} else if(sampleInfo.sampleSize <= 32)
		{
			format = littleEndian ? RS_STIPCM32S : RS_STIPCM32S;	// FIXME: No big-endian support!
		} else
		{
			return false;
		}
	}
	if(!memcmp(compression, "fl32", 4) || !memcmp(compression, "FL32", 4))
	{
		// FIXME: No big-endian support!
		wavFormat = 3;
	}

	soundChunk.Skip(sampleHeader.offset);

	ModSample &mptSample = Samples[nSample];
	DestroySample(nSample);
	mptSample.Initialize();
	mptSample.nLength = sampleInfo.numSampleFrames;
	mptSample.nC5Speed = sampleInfo.GetSampleRate();
	LimitMax(mptSample.nLength, MAX_SAMPLE_LENGTH);

	ReadSample(&mptSample, format, soundChunk, wavFormat);

	// Read MARK and INST chunk to extract sample loops
	FileReader markerChunk(chunks.GetChunk(AIFFChunk::idMARK));
	FileReader instChunk(chunks.GetChunk(AIFFChunk::idINST));
	AIFFInstrumentChunk instrHeader;
	if(markerChunk.IsValid() && instChunk.IsValid() && instChunk.ReadConvertEndianness(instrHeader))
	{
		size_t numMarkers = markerChunk.ReadUint16BE();

		vector<AIFFMarker> markers;
		for(size_t i = 0; i < numMarkers; i++)
		{
			AIFFMarker marker;
			if(!markerChunk.ReadConvertEndianness(marker))
			{
				break;
			}
			markers.push_back(marker);
			markerChunk.Skip(marker.nameLength + ((marker.nameLength % 2) == 0 ? 1 : 0));
		}

		if(instrHeader.sustainLoop.playMode != AIFFInstrumentLoop::noLoop)
		{
			mptSample.uFlags |= CHN_SUSTAINLOOP | (instrHeader.sustainLoop.playMode == AIFFInstrumentLoop::loopBidi ? CHN_PINGPONGSUSTAIN : 0);
		}

		if(instrHeader.releaseLoop.playMode != AIFFInstrumentLoop::noLoop)
		{
			mptSample.uFlags |= CHN_LOOP | (instrHeader.releaseLoop.playMode == AIFFInstrumentLoop::loopBidi ? CHN_PINGPONGLOOP : 0);
		}

		// Read markers
		for(vector<AIFFMarker>::iterator iter = markers.begin(); iter != markers.end(); iter++)
		{
			if(iter->id == instrHeader.sustainLoop.beginLoop)
			{
				mptSample.nSustainStart = iter->position;
			}
			if(iter->id == instrHeader.sustainLoop.endLoop)
			{
				mptSample.nSustainEnd = iter->position;
			}
			if(iter->id == instrHeader.releaseLoop.beginLoop)
			{
				mptSample.nLoopStart = iter->position;
			}
			if(iter->id == instrHeader.releaseLoop.endLoop)
			{
				mptSample.nLoopEnd = iter->position;
			}
		}
		// Sanitize loops
		LimitMax(mptSample.nSustainEnd, mptSample.nLength);
		LimitMax(mptSample.nLoopEnd, mptSample.nLength);
		if(mptSample.nSustainStart >= mptSample.nSustainEnd)
		{
			mptSample.nSustainStart = mptSample.nSustainEnd = 0;
			mptSample.uFlags &= ~CHN_SUSTAINLOOP;
		}
		if(mptSample.nLoopStart >= mptSample.nLoopEnd)
		{
			mptSample.nLoopStart = mptSample.nLoopEnd = 0;
			mptSample.uFlags &= ~CHN_LOOP;
		}
	}

	// Extract sample name
	FileReader nameChunk(chunks.GetChunk(AIFFChunk::idNAME));
	if(nameChunk.IsValid())
	{
		nameChunk.ReadString<StringFixer::spacePadded>(m_szNames[nSample], nameChunk.GetLength());
	} else
	{
		strcpy(m_szNames[nSample], "");
	}

	mptSample.Convert(MOD_TYPE_IT, GetType());
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
	ITSample *pis = (ITSample *)lpMemFile;
	DWORD dwMemPos;

	if ((!lpMemFile) || (dwFileLength < sizeof(ITSample))
		|| (pis->id != LittleEndian(ITSample::magic)))
	{
		return 0;
	}

	DestroySample(nSample);

	dwMemPos = pis->ConvertToMPT(Samples[nSample]) - dwOffset;

	StringFixer::ReadString<StringFixer::spacePaddedNull>(m_szNames[nSample], pis->name);

	if(dwMemPos > dwFileLength)
	{
		return 0;
	}

	Samples[nSample].Convert(MOD_TYPE_IT, GetType());

	return ReadSample(&Samples[nSample], pis->GetSampleFormat(), (LPSTR)(lpMemFile + dwMemPos), dwFileLength + dwOffset - dwMemPos);
}


bool CSoundFile::ReadITIInstrument(INSTRUMENTINDEX nInstr, LPBYTE lpMemFile, DWORD dwFileLength)
//----------------------------------------------------------------------------------------------
{
	ITInstrument *pinstr = (ITInstrument *)lpMemFile;
	UINT nsmp = 0, nsamples;

	if ((!lpMemFile) || (dwFileLength < sizeof(ITInstrument))
		|| (pinstr->id != LittleEndian(ITInstrument::magic))) return false;
	if (nInstr > m_nInstruments) m_nInstruments = nInstr;

	ModInstrument *pIns;

	try
	{
		pIns = new ModInstrument();
	} catch(MPTMemoryException)
	{
		return false;
	}

	DestroyInstrument(nInstr, deleteAssociatedSamples);

	Instruments[nInstr] = pIns;
	DWORD dwMemPos = ITInstrToMPT(pinstr, pIns, pinstr->trkvers, dwFileLength);
	nsamples = pinstr->nos;

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
	// In order to properly compute the position, in file, of eventual extended settings
	// such as "attack" we need to keep the "real" size of the last sample as those extra
	// setting will follow this sample in the file
	UINT lastSampleSize = 0;
// -! NEW_FEATURE#0027

	// Reading Samples
	vector<SAMPLEINDEX> samplemap(nsamples, 0);
	for (UINT i=0; i<nsamples; i++)
	{
		nsmp = GetNextFreeSample(nInstr, nsmp + 1);
		if (nsmp == SAMPLEINDEX_INVALID) break;
		samplemap[i] = nsmp;
// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"
//		ReadITSSample(nsmp, lpMemFile+dwMemPos, dwFileLength-dwMemPos, dwMemPos);
		lastSampleSize = ReadITSSample(nsmp, lpMemFile + dwMemPos, dwFileLength - dwMemPos, dwMemPos);
// -! NEW_FEATURE#0027
		dwMemPos += sizeof(ITSample);
	}
	if (m_nSamples < nsmp) m_nSamples = nsmp;

	for(size_t j = 0; j < CountOf(pIns->Keyboard); j++)
	{
		if(pIns->Keyboard[j] && pIns->Keyboard[j] <= nsamples)
		{
			pIns->Keyboard[j] = samplemap[pIns->Keyboard[j] - 1];
		}
	}

// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

	// Rewind file pointer offset (dwMemPos) to last sample header position
	dwMemPos -= sizeof(ITSample);
	BYTE *ptr = (BYTE *)(lpMemFile + dwMemPos);

	// Update file pointer offset (dwMemPos) to match the end of the sample datas
	ITSample *pis = (ITSample *)ptr;
	dwMemPos += pis->samplepointer - dwMemPos + lastSampleSize;
	// Leave if no extra instrument settings are available (end of file reached)
	if(dwMemPos >= dwFileLength) return true;

	pIns->Convert(MOD_TYPE_IT, GetType());

	ReadExtendedInstrumentProperties(pIns, lpMemFile + dwMemPos, dwFileLength - dwMemPos);

// -! NEW_FEATURE#0027

	return true;
}


bool CSoundFile::SaveITIInstrument(INSTRUMENTINDEX nInstr, const LPCSTR lpszFileName) const
//-----------------------------------------------------------------------------------------
{
	ITInstrumentEx iti;
	ModInstrument *pIns = Instruments[nInstr];
	DWORD dwPos;
	FILE *f;

	if((!pIns) || (!lpszFileName)) return false;
	if((f = fopen(lpszFileName, "wb")) == NULL) return false;
	
	size_t instSize = iti.ConvertToIT(*pIns, false, *this);

	// Create sample assignment table
	vector<SAMPLEINDEX> smptable;
	vector<SAMPLEINDEX> smpmap(GetNumSamples(), 0);
	for(size_t i = 0; i < NOTE_MAX; i++)
	{
		const SAMPLEINDEX smp = pIns->Keyboard[i];
		if(smp && smp <= GetNumSamples())
		{
			if(!smpmap[smp - 1])
			{
				// We haven't considered this sample yet.
				smptable.push_back(smp);
				smpmap[smp - 1] = smptable.size();
			}
			iti.iti.keyboard[i * 2 + 1] = smpmap[smp - 1];
		} else
		{
			iti.iti.keyboard[i * 2 + 1] = 0;
		}
	}
	smpmap.clear();

	dwPos = instSize;
	fwrite(&iti, 1, instSize, f);

	dwPos += smptable.size() * sizeof(ITSample);

	// Writing sample headers
	for(vector<SAMPLEINDEX>::iterator iter = smptable.begin(); iter != smptable.end(); iter++)
	{
		ITSample itss;
		itss.ConvertToIT(Samples[*iter], GetType());

		StringFixer::WriteString<StringFixer::nullTerminated>(itss.name, m_szNames[*iter]);

		itss.samplepointer = LittleEndian(dwPos);

		fwrite(&itss, 1, sizeof(itss), f);
		dwPos += Samples[*iter].GetSampleSizeInBytes();
	}

	// Writing Sample Data

	for(vector<SAMPLEINDEX>::iterator iter = smptable.begin(); iter != smptable.end(); iter++)
	{
		const ModSample &sample = Samples[*iter];
		// TODO we should actually use ITSample::GetSampleFormat() instead of re-computing the flags here.
		UINT smpflags = (sample.uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
		if (sample.uFlags & CHN_STEREO) smpflags = (sample.uFlags & CHN_16BIT) ? RS_STPCM16S : RS_STPCM8S;
		WriteSample(f, &sample, smpflags);
	}

	int32 code = 'MPTX';
	fwrite(&code, 1, sizeof(int32), f);		// Write extension tag
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


void ReadInstrumentExtensionField(ModInstrument* pIns, LPCBYTE& ptr, const int32 code, const int16 size)
//------------------------------------------------------------------------------------------------------
{
	// get field's address in instrument's header
	BYTE* fadr = GetInstrumentHeaderFieldPointer(pIns, code, size);
	 
	if(fadr && code != 'K[..')	// copy field data in instrument's header
		memcpy(fadr,ptr,size);  // (except for keyboard mapping)
	ptr += size;				// jump field

	if (code == 'dF..' && fadr != nullptr) // 'dF..' field requires additional processing.
		ConvertReadExtendedFlags(pIns);
}


void ReadExtendedInstrumentProperty(ModInstrument* pIns, const int32 code, LPCBYTE& pData, const LPCBYTE pEnd)
//------------------------------------------------------------------------------------------------------------
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


void ReadExtendedInstrumentProperties(ModInstrument* pIns, const LPCBYTE pDataStart, const size_t nMemLength)
//-----------------------------------------------------------------------------------------------------------
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


void ConvertReadExtendedFlags(ModInstrument *pIns)
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

#pragma pack(push, 1)

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


#pragma pack(pop)



bool CSoundFile::Read8SVXSample(UINT nSample, LPBYTE lpMemFile, DWORD dwFileLength)
//---------------------------------------------------------------------------------
{
	IFF8SVXFILEHEADER *pfh = (IFF8SVXFILEHEADER *)lpMemFile;
	IFFVHDR *pvh = (IFFVHDR *)(lpMemFile + 12);
	ModSample *pSmp = &Samples[nSample];
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
	if (GetType() & MOD_TYPE_XM) FrequencyToTranspose(pSmp);
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
