/*
 * ACMConvert.cpp
 * --------------
 * Purpose: MPEG Layer-3 Functions through ACM.
 * Notes  : Access to LAMEenc and BLADEenc is emulated through the ACM interface.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include <bladeenc/bladedll.h>
#include "Mptrack.h"
#include "ACMConvert.h"


typedef struct BLADEENCSTREAMINFO
{
	BE_CONFIG beCfg;
	LONGLONG dummy;
	DWORD dwInputSamples;
	DWORD dwOutputSamples;
	HBE_STREAM hBeStream;
	SHORT *pPCMBuffer;
	DWORD dwReadPos;
} BLADEENCSTREAMINFO, *PBLADEENCSTREAMINFO;

static PBLADEENCSTREAMINFO gpbeBladeCfg = NULL;
static PBLADEENCSTREAMINFO gpbeLameCfg = NULL;
BOOL ACMConvert::layer3Present = FALSE;


BOOL ACMConvert::InitializeACM(BOOL bNoAcm)
//-----------------------------------------
{
	DWORD (ACMAPI *pfnAcmGetVersion)(void);
	DWORD dwVersion;
	UINT fuErrorMode;
	BOOL bOk = FALSE;

	fuErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	try {
		m_hBladeEnc = LoadLibrary(TEXT("BLADEENC.DLL"));
		m_hLameEnc = LoadLibrary(TEXT("LAME_ENC.DLL"));
	} catch(...) {}
	if (!bNoAcm)
	{
		try {
			m_hACMInst = LoadLibrary(TEXT("MSACM32.DLL"));
		} catch(...)
		{
			m_hACMInst = NULL;
		}
	}
	SetErrorMode(fuErrorMode);
	if (m_hBladeEnc != NULL)
	{
		BEVERSION pfnBeVersion = (BEVERSION)GetProcAddress(m_hBladeEnc, TEXT_BEVERSION);
		if (!pfnBeVersion)
		{
			FreeLibrary(m_hBladeEnc);
			m_hBladeEnc = NULL;
		} else
		{
			layer3Present = TRUE;
			bOk = TRUE;
		}
	}
	if (m_hLameEnc != NULL)
	{
		BEVERSION pfnBeVersion = (BEVERSION)GetProcAddress(m_hLameEnc, TEXT_BEVERSION);
		if (!pfnBeVersion)
		{
			FreeLibrary(m_hLameEnc);
			m_hLameEnc = NULL;
		} else
		{
			layer3Present = TRUE;
			bOk = TRUE;
		}
	}
	if ((m_hACMInst < (HINSTANCE)HINSTANCE_ERROR) || (!m_hACMInst))
	{
		m_hACMInst = NULL;
		return bOk;
	}
	try {
		*(FARPROC *)&pfnAcmGetVersion = GetProcAddress(m_hACMInst, "acmGetVersion");
		dwVersion = 0;
		if (pfnAcmGetVersion) dwVersion = pfnAcmGetVersion();
		if (HIWORD(dwVersion) < 0x0200)
		{
			FreeLibrary(m_hACMInst);
			m_hACMInst = NULL;
			return bOk;
		}
		// Load Function Pointers
		m_pfnAcmFormatEnum = (PFNACMFORMATENUM)GetProcAddress(m_hACMInst, "acmFormatEnumA");
		// Enumerate formats
		if (m_pfnAcmFormatEnum)
		{
			ACMFORMATDETAILS afd;
			BYTE wfx[256];
			WAVEFORMATEX *pwfx = (WAVEFORMATEX *)&wfx;

			MemsetZero(afd);
			MemsetZero(*pwfx);
			afd.cbStruct = sizeof(ACMFORMATDETAILS);
			afd.dwFormatTag = WAVE_FORMAT_PCM;
			afd.pwfx = pwfx;
			afd.cbwfx = sizeof(wfx);
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->nChannels = 2;
			pwfx->nSamplesPerSec = 44100;
			pwfx->wBitsPerSample = 16;
			pwfx->nBlockAlign = (WORD)((pwfx->nChannels * pwfx->wBitsPerSample) / 8);
			pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
			m_pfnAcmFormatEnum(NULL, &afd, AcmFormatEnumCB, NULL, ACM_FORMATENUMF_CONVERT);
			bOk = TRUE;
		}
	} catch(...)
	{
		// nothing
	}
	return bOk;
}


BOOL ACMConvert::UninitializeACM()
//--------------------------------
{
	if (m_hACMInst)
	{
		FreeLibrary(m_hACMInst);
		m_hACMInst = NULL;
	}
	if (m_hLameEnc)
	{
		FreeLibrary(m_hLameEnc);
		m_hLameEnc = NULL;
	}
	if (m_hBladeEnc)
	{
		FreeLibrary(m_hBladeEnc);
		m_hBladeEnc = NULL;
	}
	layer3Present = FALSE;
	return TRUE;
}


MMRESULT ACMConvert::AcmFormatEnum(HACMDRIVER had, LPACMFORMATDETAILSA pafd, ACMFORMATENUMCBA fnCallback, DWORD dwInstance, DWORD fdwEnum)
//----------------------------------------------------------------------------------------------------------------------------------------
{
	MMRESULT err = MMSYSERR_INVALPARAM;
	if ((m_hBladeEnc) || (m_hLameEnc))
	{
		HACMDRIVER hBlade = (HACMDRIVER)&m_hBladeEnc;
		HACMDRIVER hLame = (HACMDRIVER)&m_hLameEnc;
		if (((had == hBlade) || (had == hLame) || (had == NULL)) && (pafd) && (fnCallback)
			&& (fdwEnum & ACM_FORMATENUMF_CONVERT) && (pafd->dwFormatTag == WAVE_FORMAT_PCM))
		{
			ACMFORMATDETAILS afd;
			MPEGLAYER3WAVEFORMAT wfx;

			afd.dwFormatIndex = 0;
			for (UINT iFmt=0; iFmt<0x40; iFmt++)
			{
				afd.cbStruct = sizeof(afd);
				afd.dwFormatTag = WAVE_FORMAT_MPEGLAYER3;
				afd.fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
				afd.pwfx = (LPWAVEFORMATEX)&wfx;
				afd.cbwfx = sizeof(wfx);
				wfx.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
				wfx.wfx.nChannels = (WORD)((iFmt & 0x20) ? 1 : 2);
				wfx.wfx.nSamplesPerSec = 0;
				wfx.wfx.nBlockAlign = 1;
				wfx.wfx.wBitsPerSample = 0;
				wfx.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
				wfx.wID = MPEGLAYER3_ID_MPEG;
				wfx.fdwFlags = MPEGLAYER3_FLAG_PADDING_ISO;
				wfx.nBlockSize = 384;
				wfx.nFramesPerBlock = 1;
				wfx.nCodecDelay = 1000;
				switch((iFmt >> 3) & 0x03)
				{
				case 0x00: wfx.wfx.nSamplesPerSec = 48000; break;
				case 0x01: wfx.wfx.nSamplesPerSec = 44100; break;
				case 0x02: wfx.wfx.nSamplesPerSec = 32000; break;
				}
				switch(iFmt & 7)
				{
				case 5:	wfx.wfx.nAvgBytesPerSec = 320/8; break;
				case 4:	wfx.wfx.nAvgBytesPerSec = 64/8; break;
				case 3:	wfx.wfx.nAvgBytesPerSec = 96/8; break;
				case 2:	wfx.wfx.nAvgBytesPerSec = 128/8; break;
				case 1:	if (wfx.wfx.nChannels == 2) { wfx.wfx.nAvgBytesPerSec = 192/8; break; }
				case 0:	if (wfx.wfx.nChannels == 2) { wfx.wfx.nAvgBytesPerSec = 256/8; break; }
				default: wfx.wfx.nSamplesPerSec = 0;
				}
				wsprintf(afd.szFormat, "%dkbps, %dHz, %s", wfx.wfx.nAvgBytesPerSec*8, wfx.wfx.nSamplesPerSec, (wfx.wfx.nChannels == 2) ? "stereo" : "mono");
				if (wfx.wfx.nSamplesPerSec)
				{
					if (m_hBladeEnc) fnCallback((HACMDRIVERID)hBlade, &afd, dwInstance, ACMDRIVERDETAILS_SUPPORTF_CODEC);
					if (m_hLameEnc) fnCallback((HACMDRIVERID)hLame, &afd, dwInstance, ACMDRIVERDETAILS_SUPPORTF_CODEC);
					afd.dwFormatIndex++;
				}
			}
			err = MMSYSERR_NOERROR;
		}
	}
	if (m_pfnAcmFormatEnum)
	{
		err = m_pfnAcmFormatEnum(had, pafd, fnCallback, dwInstance, fdwEnum);
	}
	return err;
}


BOOL ACMConvert::AcmFormatEnumCB(HACMDRIVERID, LPACMFORMATDETAILS pafd, DWORD, DWORD fdwSupport)
//----------------------------------------------------------------------------------------------
{
	if ((pafd) && (fdwSupport & ACMDRIVERDETAILS_SUPPORTF_CODEC))
	{
		if (pafd->dwFormatTag == WAVE_FORMAT_MPEGLAYER3) layer3Present = TRUE;
	}
	return TRUE;
}


MMRESULT ACMConvert::AcmDriverOpen(LPHACMDRIVER phad, HACMDRIVERID hadid, DWORD fdwOpen)
//--------------------------------------------------------------------------------------
{
	if ((m_hBladeEnc) && (hadid == (HACMDRIVERID)&m_hBladeEnc))
	{
		*phad = (HACMDRIVER)&m_hBladeEnc;
		return MMSYSERR_NOERROR;
	}
	if ((m_hLameEnc) && (hadid == (HACMDRIVERID)&m_hLameEnc))
	{
		*phad = (HACMDRIVER)&m_hLameEnc;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMDRIVEROPEN pfnAcmDriverOpen = (PFNACMDRIVEROPEN)GetProcAddress(m_hACMInst, "acmDriverOpen");
		if (pfnAcmDriverOpen) return pfnAcmDriverOpen(phad, hadid, fdwOpen);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmDriverClose(HACMDRIVER had, DWORD fdwClose)
//-----------------------------------------------------------------
{
	if ((m_hBladeEnc) && (had == (HACMDRIVER)&m_hBladeEnc))
	{
		return MMSYSERR_NOERROR;
	}
	if ((m_hLameEnc) && (had == (HACMDRIVER)&m_hLameEnc))
	{
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMDRIVERCLOSE pfnAcmDriverClose = (PFNACMDRIVERCLOSE)GetProcAddress(m_hACMInst, "acmDriverClose");
		if (pfnAcmDriverClose) return pfnAcmDriverClose(had, fdwClose);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmDriverDetails(HACMDRIVERID hadid, LPACMDRIVERDETAILS padd, DWORD fdwDetails)
//--------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (hadid == (HACMDRIVERID)(&m_hBladeEnc)))
		|| ((m_hLameEnc) && (hadid == (HACMDRIVERID)(&m_hLameEnc))))
	{
		if (!padd) return MMSYSERR_INVALPARAM;
		padd->cbStruct = sizeof(ACMDRIVERDETAILS);
		padd->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
		padd->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
		padd->wMid = 0;
		padd->wPid = 0;
		padd->vdwACM = 0x04020000;
		padd->vdwDriver = 0x04020000;
		padd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
		padd->cFormatTags = 1;
		padd->cFilterTags = 0;
		padd->hicon = NULL;
		strcpy(padd->szShortName, (hadid == (HACMDRIVERID)(&m_hBladeEnc)) ? "BladeEnc MP3" : "LAME Encoder");
		strcpy(padd->szLongName, padd->szShortName);
		padd->szCopyright[0] = 0;
		padd->szLicensing[0] = 0;
		padd->szFeatures[0] = 0;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMDRIVERDETAILS pfnAcmDriverDetails = (PFNACMDRIVERDETAILS)GetProcAddress(m_hACMInst, "acmDriverDetailsA");
		if (pfnAcmDriverDetails) return pfnAcmDriverDetails(hadid, padd, fdwDetails);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmStreamOpen(
	LPHACMSTREAM            phas,       // pointer to stream handle
	HACMDRIVER              had,        // optional driver handle
	LPWAVEFORMATEX          pwfxSrc,    // source format to convert
	LPWAVEFORMATEX          pwfxDst,    // required destination format
	LPWAVEFILTER            pwfltr,     // optional filter
	DWORD                   dwCallback, // callback
	DWORD                   dwInstance, // callback instance data
	DWORD                   fdwOpen)    // ACM_STREAMOPENF_* and CALLBACK_*
//--------------------------------------------------------------------------
{
	PBLADEENCSTREAMINFO *ppbeCfg = NULL;
	HINSTANCE hLib = NULL;

	if ((m_hBladeEnc) && (had == (HACMDRIVER)&m_hBladeEnc))
	{
		ppbeCfg = &gpbeBladeCfg;
		hLib = m_hBladeEnc;
	}
	if ((m_hLameEnc) && (had == (HACMDRIVER)&m_hLameEnc))
	{
		ppbeCfg = &gpbeLameCfg;
		hLib = m_hLameEnc;
	}
	if ((ppbeCfg) && (hLib))
	{
		BEINITSTREAM pfnbeInitStream = (BEINITSTREAM)GetProcAddress(hLib, TEXT_BEINITSTREAM);
		if (!pfnbeInitStream) return MMSYSERR_INVALPARAM;
		PBLADEENCSTREAMINFO pbeCfg = new BLADEENCSTREAMINFO;
		if ((pbeCfg) && (pwfxSrc) && (pwfxDst) && (pwfxSrc->nChannels == pwfxDst->nChannels)
			&& (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM) && (pwfxDst->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
			&& (pwfxSrc->wBitsPerSample == 16))
		{
			pbeCfg->dwInputSamples = 2048;
			pbeCfg->dwOutputSamples = 2048;
			pbeCfg->beCfg.dwConfig = BE_CONFIG_MP3;
			pbeCfg->beCfg.format.mp3.dwSampleRate = pwfxDst->nSamplesPerSec; // 48000, 44100 and 32000 allowed
			pbeCfg->beCfg.format.mp3.byMode = (BYTE)((pwfxSrc->nChannels == 2) ? BE_MP3_MODE_STEREO : BE_MP3_MODE_MONO);
			pbeCfg->beCfg.format.mp3.wBitrate = (WORD)(pwfxDst->nAvgBytesPerSec * 8);	// 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256 and 320 allowed
			pbeCfg->beCfg.format.mp3.bPrivate = FALSE;
			pbeCfg->beCfg.format.mp3.bCRC = FALSE;
			pbeCfg->beCfg.format.mp3.bCopyright = FALSE;
			pbeCfg->beCfg.format.mp3.bOriginal = TRUE;
			pbeCfg->hBeStream = NULL;
			if (pfnbeInitStream(&pbeCfg->beCfg, &pbeCfg->dwInputSamples, &pbeCfg->dwOutputSamples, &pbeCfg->hBeStream) == BE_ERR_SUCCESSFUL)
			{
				*phas = (HACMSTREAM)had;
				*ppbeCfg = pbeCfg;
				pbeCfg->pPCMBuffer = (SHORT *)GlobalAllocPtr(GHND, pbeCfg->dwInputSamples * sizeof(SHORT));
				pbeCfg->dwReadPos = 0;
				return MMSYSERR_NOERROR;
			}
		}
		if (pbeCfg) delete pbeCfg;
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMOPEN pfnAcmStreamOpen = (PFNACMSTREAMOPEN)GetProcAddress(m_hACMInst, "acmStreamOpen");
		if (pfnAcmStreamOpen) return pfnAcmStreamOpen(phas, had, pwfxSrc, pwfxDst, pwfltr, dwCallback, dwInstance, fdwOpen);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmStreamClose(HACMSTREAM has, DWORD fdwClose)
//-----------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
		|| ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		HINSTANCE hLib = *(HINSTANCE *)has;
		PBLADEENCSTREAMINFO pbeCfg = (has == (HACMSTREAM)&m_hBladeEnc) ? gpbeBladeCfg : gpbeLameCfg;
		BECLOSESTREAM pfnbeCloseStream = (BECLOSESTREAM)GetProcAddress(hLib, TEXT_BECLOSESTREAM);
		if ((pbeCfg) && (pfnbeCloseStream))
		{
			pfnbeCloseStream(pbeCfg->hBeStream);
			if (pbeCfg->pPCMBuffer)
			{
				GlobalFreePtr(pbeCfg->pPCMBuffer);
				pbeCfg->pPCMBuffer = NULL;
			}
			delete pbeCfg;
			pbeCfg = NULL;
			return MMSYSERR_NOERROR;
		}
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCLOSE pfnAcmStreamClose = (PFNACMSTREAMCLOSE)GetProcAddress(m_hACMInst, "acmStreamClose");
		if (pfnAcmStreamClose) return pfnAcmStreamClose(has, fdwClose);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmStreamSize(HACMSTREAM has, DWORD cbInput, LPDWORD pdwOutputBytes, DWORD fdwSize)
//------------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
		|| ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		PBLADEENCSTREAMINFO pbeCfg = (has == (HACMSTREAM)&m_hBladeEnc) ? gpbeBladeCfg : gpbeLameCfg;

		if ((pbeCfg) && (pdwOutputBytes))
		{
			if (fdwSize & ACM_STREAMSIZEF_DESTINATION)
			{
				*pdwOutputBytes = pbeCfg->dwOutputSamples * sizeof(short);
			} else
				if (fdwSize & ACM_STREAMSIZEF_SOURCE)
				{
					*pdwOutputBytes = pbeCfg->dwInputSamples * sizeof(short);
				}
				return MMSYSERR_NOERROR;
		}
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		if (fdwSize & ACM_STREAMSIZEF_DESTINATION)	// Why does acmStreamSize return ACMERR_NOTPOSSIBLE in this case?
			return MMSYSERR_NOERROR;
		PFNACMSTREAMSIZE pfnAcmStreamSize = (PFNACMSTREAMSIZE)GetProcAddress(m_hACMInst, "acmStreamSize");
		if (pfnAcmStreamSize) return pfnAcmStreamSize(has, cbInput, pdwOutputBytes, fdwSize);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmStreamPrepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare)
//---------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
		|| ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		pash->fdwStatus = ACMSTREAMHEADER_STATUSF_PREPARED;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCONVERT pfnAcmStreamPrepareHeader = (PFNACMSTREAMCONVERT)GetProcAddress(m_hACMInst, "acmStreamPrepareHeader");
		if (pfnAcmStreamPrepareHeader) return pfnAcmStreamPrepareHeader(has, pash, fdwPrepare);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmStreamUnprepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwUnprepare)
//-------------------------------------------------------------------------------------------------------
{
	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
		|| ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		pash->fdwStatus &= ~ACMSTREAMHEADER_STATUSF_PREPARED;
		return MMSYSERR_NOERROR;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCONVERT pfnAcmStreamUnprepareHeader = (PFNACMSTREAMCONVERT)GetProcAddress(m_hACMInst, "acmStreamUnprepareHeader");
		if (pfnAcmStreamUnprepareHeader) return pfnAcmStreamUnprepareHeader(has, pash, fdwUnprepare);
	}
	return MMSYSERR_INVALPARAM;
}


MMRESULT ACMConvert::AcmStreamConvert(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert)
//---------------------------------------------------------------------------------------------
{
	static BEENCODECHUNK pfnbeEncodeChunk = NULL;
	static BEDEINITSTREAM pfnbeDeinitStream = NULL;
	static HINSTANCE hInstEncode = NULL;

	if (((m_hBladeEnc) && (has == (HACMSTREAM)&m_hBladeEnc))
		|| ((m_hLameEnc) && (has == (HACMSTREAM)&m_hLameEnc)))
	{
		PBLADEENCSTREAMINFO pbeCfg = (has == (HACMSTREAM)&m_hBladeEnc) ? gpbeBladeCfg : gpbeLameCfg;
		HINSTANCE hLib = *(HINSTANCE *)has;

		if (hInstEncode != hLib)
		{
			pfnbeEncodeChunk = (BEENCODECHUNK)GetProcAddress(hLib, TEXT_BEENCODECHUNK);
			pfnbeDeinitStream = (BEDEINITSTREAM)GetProcAddress(hLib, TEXT_BEDEINITSTREAM);
			hInstEncode = hLib;
		}
		pash->fdwStatus |= ACMSTREAMHEADER_STATUSF_DONE;
		if (fdwConvert & ACM_STREAMCONVERTF_END)
		{
			if ((pfnbeDeinitStream) && (pbeCfg))
			{
				pfnbeDeinitStream(pbeCfg->hBeStream, pash->pbDst, &pash->cbDstLengthUsed);
				return MMSYSERR_NOERROR;
			}
		} else
		{
			if ((pfnbeEncodeChunk) && (pbeCfg))
			{
				DWORD dwBytesLeft = pbeCfg->dwInputSamples*2 - pbeCfg->dwReadPos;
				if (!dwBytesLeft)
				{
					dwBytesLeft = pbeCfg->dwInputSamples*2;
					pbeCfg->dwReadPos = 0;
				}
				if (dwBytesLeft > pash->cbSrcLength) dwBytesLeft = pash->cbSrcLength;
				memcpy(&pbeCfg->pPCMBuffer[pbeCfg->dwReadPos/2], pash->pbSrc, dwBytesLeft);
				pbeCfg->dwReadPos += dwBytesLeft;
				pash->cbSrcLengthUsed = dwBytesLeft;
				pash->cbDstLengthUsed = 0;
				if (pbeCfg->dwReadPos == pbeCfg->dwInputSamples*2)
				{
					if (pfnbeEncodeChunk(pbeCfg->hBeStream, pbeCfg->dwInputSamples, pbeCfg->pPCMBuffer, pash->pbDst, &pash->cbDstLengthUsed) != 0) return MMSYSERR_INVALPARAM;
					pbeCfg->dwReadPos = 0;
				}
				return MMSYSERR_NOERROR;
			}
		}
		return MMSYSERR_INVALPARAM;
	}
	if (m_hACMInst)
	{
		PFNACMSTREAMCONVERT pfnAcmStreamConvert = (PFNACMSTREAMCONVERT)GetProcAddress(m_hACMInst, "acmStreamConvert");
		if (pfnAcmStreamConvert) return pfnAcmStreamConvert(has, pash, fdwConvert);
	}
	return MMSYSERR_INVALPARAM;
}
