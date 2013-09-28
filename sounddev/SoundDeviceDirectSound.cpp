/*
 * SoundDeviceDirectSound.cpp
 * --------------------------
 * Purpose: DirectSound sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"
#include "SoundDevices.h"

#include "SoundDeviceDirectSound.h"

#include "../common/misc_util.h"
#include "../common/StringFixer.h"


////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound device
//

#ifndef NO_DSOUND


static std::wstring GuidToString(GUID guid)
//-----------------------------------------
{
	std::wstring str;
	LPOLESTR tmp = nullptr;
	StringFromIID(guid, &tmp);
	if(tmp)
	{
		str = tmp;
		CoTaskMemFree(tmp);
		tmp = nullptr;
	}
	return str;
}


static GUID StringToGuid(const std::wstring &str)
//-----------------------------------------------
{
	GUID guid = GUID();
	IIDFromString(str.c_str(), &guid);
	return guid;
}


static BOOL WINAPI DSEnumCallbackW(GUID * lpGuid, LPCWSTR lpstrDescription, LPCWSTR, LPVOID lpContext)
//----------------------------------------------------------------------------------------------------
{
	std::vector<SoundDeviceInfo> &devices = *(std::vector<SoundDeviceInfo>*)lpContext;
	if(!lpstrDescription)
	{
		return TRUE;
	}
	SoundDeviceInfo info;
	info.type = SNDDEV_DSOUND;
	info.index = devices.size();
	info.name = lpstrDescription;
	if(lpGuid)
	{
		info.internalID = GuidToString(*lpGuid);
	}
	devices.push_back(info);
	return TRUE;
}


std::vector<SoundDeviceInfo> CDSoundDevice::EnumerateDevices()
//------------------------------------------------------------
{
	std::vector<SoundDeviceInfo> devices;
	DirectSoundEnumerateW(DSEnumCallbackW, &devices);
	return devices;
}


CDSoundDevice::CDSoundDevice()
//----------------------------
{
	m_piDS = NULL;
	m_pPrimary = NULL;
	m_pMixBuffer = NULL;
	m_bMixRunning = FALSE;
	m_nBytesPerSec = 0;
	m_BytesPerSample = 0;
}


CDSoundDevice::~CDSoundDevice()
//-----------------------------
{
	Reset();
	Close();
}


bool CDSoundDevice::InternalOpen()
//--------------------------------
{
	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext)) return false;
	WAVEFORMATEX *pwfx = &wfext.Format;

	DSBUFFERDESC dsbd;
	DSBCAPS dsc;
	UINT nPriorityLevel = (m_Settings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE) ? DSSCL_WRITEPRIMARY : DSSCL_PRIORITY;

	if(m_piDS) return true;
	const std::wstring internalID = GetDeviceInternalID();
	GUID guid = internalID.empty() ? GUID() : StringToGuid(internalID);
	if(DirectSoundCreate(internalID.empty() ? NULL : &guid, &m_piDS, NULL) != DS_OK) return false;
	if(!m_piDS) return false;
	m_piDS->SetCooperativeLevel(m_Settings.hWnd, nPriorityLevel);
	m_bMixRunning = FALSE;
	m_nDSoundBufferSize = (m_Settings.LatencyMS * pwfx->nAvgBytesPerSec) / 1000;
	m_nDSoundBufferSize = (m_nDSoundBufferSize + 15) & ~15;
	if(m_nDSoundBufferSize < DSOUND_MINBUFFERSIZE) m_nDSoundBufferSize = DSOUND_MINBUFFERSIZE;
	if(m_nDSoundBufferSize > DSOUND_MAXBUFFERSIZE) m_nDSoundBufferSize = DSOUND_MAXBUFFERSIZE;
	m_nBytesPerSec = pwfx->nAvgBytesPerSec;
	m_BytesPerSample = (pwfx->wBitsPerSample/8) * pwfx->nChannels;
	if(!(m_Settings.fulCfgOptions & SNDDEV_OPTIONS_EXCLUSIVE))
	{
		// Set the format of the primary buffer
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
		dsbd.dwBufferBytes = 0;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = NULL;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pPrimary, NULL) != DS_OK)
		{
			Close();
			return false;
		}
		m_pPrimary->SetFormat(pwfx);
		///////////////////////////////////////////////////
		// Create the secondary buffer
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = m_nDSoundBufferSize;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = pwfx;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pMixBuffer, NULL) != DS_OK)
		{
			Close();
			return false;
		}
	} else
	{
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = 0;
		dsbd.dwReserved = 0;
		dsbd.lpwfxFormat = NULL;
		if (m_piDS->CreateSoundBuffer(&dsbd, &m_pPrimary, NULL) != DS_OK)
		{
			Close();
			return false;
		}
		if (m_pPrimary->SetFormat(pwfx) != DS_OK)
		{
			Close();
			return false;
		}
		dsc.dwSize = sizeof(dsc);
		if (m_pPrimary->GetCaps(&dsc) != DS_OK)
		{
			Close();
			return false;
		}
		m_nDSoundBufferSize = dsc.dwBufferBytes;
		m_pMixBuffer = m_pPrimary;
		m_pMixBuffer->AddRef();
	}
	LPVOID lpBuf1, lpBuf2;
	DWORD dwSize1, dwSize2;
	if (m_pMixBuffer->Lock(0, m_nDSoundBufferSize, &lpBuf1, &dwSize1, &lpBuf2, &dwSize2, 0) == DS_OK)
	{
		UINT zero = (pwfx->wBitsPerSample == 8) ? 0x80 : 0x00;
		if ((lpBuf1) && (dwSize1)) memset(lpBuf1, zero, dwSize1);
		if ((lpBuf2) && (dwSize2)) memset(lpBuf2, zero, dwSize2);
		m_pMixBuffer->Unlock(lpBuf1, dwSize1, lpBuf2, dwSize2);
	} else
	{
		DWORD dwStat = 0;
		m_pMixBuffer->GetStatus(&dwStat);
		if (dwStat & DSBSTATUS_BUFFERLOST) m_pMixBuffer->Restore();
	}
	m_RealLatencyMS = m_nDSoundBufferSize * 1000.0f / m_nBytesPerSec;
	m_RealUpdateIntervalMS = CLAMP(static_cast<float>(m_Settings.UpdateIntervalMS), 1.0f, m_nDSoundBufferSize * 1000.0f / ( 2.0f * m_nBytesPerSec ) );
	m_dwWritePos = 0xFFFFFFFF;
	return true;
}


bool CDSoundDevice::InternalClose()
//---------------------------------
{
	if (m_pMixBuffer)
	{
		m_pMixBuffer->Release();
		m_pMixBuffer = NULL;
	}
	if (m_pPrimary)
	{
		m_pPrimary->Release();
		m_pPrimary = NULL;
	}
	if (m_piDS)
	{
		m_piDS->Release();
		m_piDS = NULL;
	}
	m_bMixRunning = FALSE;
	return true;
}


void CDSoundDevice::StartFromSoundThread()
//----------------------------------------
{
	if(!m_pMixBuffer) return;
	// done in FillAudioBuffer for now
}


void CDSoundDevice::StopFromSoundThread()
//---------------------------------------
{
	if(m_pMixBuffer)
	{
		m_pMixBuffer->Stop();
	}
	m_bMixRunning = FALSE;
}


void CDSoundDevice::ResetFromOutsideSoundThread()
//-----------------------------------------------
{
	if(m_pMixBuffer)
	{
		m_pMixBuffer->Stop();
	}
	m_bMixRunning = FALSE;
}


DWORD CDSoundDevice::LockBuffer(DWORD dwBytes, LPVOID *lpBuf1, LPDWORD lpSize1, LPVOID *lpBuf2, LPDWORD lpSize2)
//--------------------------------------------------------------------------------------------------------------
{
	DWORD d, dwPlay = 0, dwWrite = 0;

	if ((!m_pMixBuffer) || (!m_nDSoundBufferSize)) return 0;
	if (m_pMixBuffer->GetCurrentPosition(&dwPlay, &dwWrite) != DS_OK) return 0;
	if ((m_dwWritePos >= m_nDSoundBufferSize) || (!m_bMixRunning))
	{
		m_dwWritePos = dwWrite;
		m_dwLatency = 0;
		d = m_nDSoundBufferSize/2;
	} else
	{
		dwWrite = m_dwWritePos;
		m_dwLatency = (m_nDSoundBufferSize + dwWrite - dwPlay) % m_nDSoundBufferSize;
		if (dwPlay >= dwWrite)
			d = dwPlay - dwWrite;
		else
			d = m_nDSoundBufferSize + dwPlay - dwWrite;
	}
	if (d > m_nDSoundBufferSize) return FALSE;
	if (d > m_nDSoundBufferSize/2) d = m_nDSoundBufferSize/2;
	if (dwBytes)
	{
		if (m_dwLatency > dwBytes) return 0;
		if (m_dwLatency+d > dwBytes) d = dwBytes - m_dwLatency;
	}
	d &= ~0x0f;
	if (d <= 16) return 0;
	HRESULT hr = m_pMixBuffer->Lock(m_dwWritePos, d, lpBuf1, lpSize1, lpBuf2, lpSize2, 0);
	if(hr == DSERR_BUFFERLOST)
	{
		// buffer lost, restore buffer and try again, fail if it fails again
		if(m_pMixBuffer->Restore() != DS_OK) return 0;
		if(m_pMixBuffer->Lock(m_dwWritePos, d, lpBuf1, lpSize1, lpBuf2, lpSize2, 0) != DS_OK) return 0;
		return d;
	} else if(hr != DS_OK)
	{
		return 0;
	}
	return d;
}


BOOL CDSoundDevice::UnlockBuffer(LPVOID lpBuf1, DWORD dwSize1, LPVOID lpBuf2, DWORD dwSize2)
//------------------------------------------------------------------------------------------
{
	if (!m_pMixBuffer) return FALSE;
	if (m_pMixBuffer->Unlock(lpBuf1, dwSize1, lpBuf2, dwSize2) == DS_OK)
	{
		m_dwWritePos += dwSize1 + dwSize2;
		if (m_dwWritePos >= m_nDSoundBufferSize) m_dwWritePos -= m_nDSoundBufferSize;
		return TRUE;
	}
	return FALSE;
}


void CDSoundDevice::FillAudioBuffer()
//-----------------------------------
{
	LPVOID lpBuf1=NULL, lpBuf2=NULL;
	DWORD dwSize1=0, dwSize2=0;
	DWORD dwBytes;

	if (!m_pMixBuffer) return;
	dwBytes = LockBuffer(m_nDSoundBufferSize, &lpBuf1, &dwSize1, &lpBuf2, &dwSize2);
	if (dwBytes)
	{
		if ((lpBuf1) && (dwSize1)) SourceAudioRead(lpBuf1, dwSize1/m_BytesPerSample);
		if ((lpBuf2) && (dwSize2)) SourceAudioRead(lpBuf2, dwSize2/m_BytesPerSample);
		UnlockBuffer(lpBuf1, dwSize1, lpBuf2, dwSize2);
		DWORD dwStatus = 0;
		m_pMixBuffer->GetStatus(&dwStatus);
		if(!m_bMixRunning || !(dwStatus & DSBSTATUS_PLAYING))
		{
			HRESULT hr;
			if(!(dwStatus & DSBSTATUS_BUFFERLOST))
			{
				// start playing
				hr = m_pMixBuffer->Play(0, 0, DSBPLAY_LOOPING);
			} else
			{
				// buffer lost flag is set, do not try start playing, we know it will fail with DSERR_BUFFERLOST.
				hr = DSERR_BUFFERLOST;
			}
			if(hr == DSERR_BUFFERLOST)
			{
				// buffer lost, restore buffer and try again, fail if it fails again
				if(m_pMixBuffer->Restore() != DS_OK) return;
				if(m_pMixBuffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK) return;
			} else if(hr != DS_OK)
			{
				return;
			}
			m_bMixRunning = TRUE; 
		}
		SourceAudioDone((dwSize1+dwSize2)/m_BytesPerSample, m_dwLatency/m_BytesPerSample);
	}
}


#endif // NO_DIRECTSOUND

