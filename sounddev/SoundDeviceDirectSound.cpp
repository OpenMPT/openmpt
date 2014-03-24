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
#include "SoundDeviceThread.h"

#include "SoundDeviceDirectSound.h"

#include "../common/misc_util.h"
#include "../common/StringFixer.h"

#include <mmreg.h>


bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat, const SoundDeviceSettings &m_Settings);


////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound device
//

#ifndef NO_DSOUND


static BOOL WINAPI DSEnumCallbackW(GUID * lpGuid, LPCWSTR lpstrDescription, LPCWSTR, LPVOID lpContext)
//----------------------------------------------------------------------------------------------------
{
	std::vector<SoundDeviceInfo> &devices = *(std::vector<SoundDeviceInfo>*)lpContext;
	if(!lpstrDescription)
	{
		return TRUE;
	}
	if(!SoundDeviceIndexIsValid(devices.size()))
	{
		return FALSE;
	}
	SoundDeviceInfo info;
	info.id = SoundDeviceID(SNDDEV_DSOUND, static_cast<SoundDeviceIndex>(devices.size()));
	info.name = lpstrDescription;
	info.apiName = SoundDeviceTypeToString(SNDDEV_DSOUND);
	if(lpGuid)
	{
		info.internalID = Util::GUIDToString(*lpGuid);
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


CDSoundDevice::CDSoundDevice(SoundDeviceID id, const std::wstring &internalID)
//----------------------------------------------------------------------------
	: CSoundDeviceWithThread(id, internalID)
	, m_piDS(NULL)
	, m_pPrimary(NULL)
	, m_pMixBuffer(NULL)
	, m_nDSoundBufferSize(0)
	, m_bMixRunning(FALSE)
	, m_dwWritePos(0)
	, m_dwLatency(0)
{
	return;
}


CDSoundDevice::~CDSoundDevice()
//-----------------------------
{
	Close();
}


SoundDeviceCaps CDSoundDevice::GetDeviceCaps()
//--------------------------------------------
{
	SoundDeviceCaps caps;
	caps.CanUpdateInterval = true;
	caps.CanSampleFormat = true;
	caps.CanExclusiveMode = true;
	caps.CanBoostThreadPriority = true;
	caps.CanUseHardwareTiming = false;
	caps.CanChannelMapping = false;
	caps.CanDriverPanel = false;
	caps.ExclusiveModeDescription = L"Use primary buffer";
	return caps;
}


SoundDeviceDynamicCaps CDSoundDevice::GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates)
//----------------------------------------------------------------------------------------------------
{
	SoundDeviceDynamicCaps caps;
	IDirectSound *dummy = nullptr;
	IDirectSound *ds = nullptr;
	if(m_piDS)
	{
		ds = m_piDS;
	} else
	{
		const std::wstring internalID = GetDeviceInternalID();
		GUID guid = internalID.empty() ? GUID() : Util::StringToGUID(internalID);
		if(DirectSoundCreate(internalID.empty() ? NULL : &guid, &dummy, NULL) != DS_OK)
		{
			return caps;
		}
		if(!dummy)
		{
			return caps;
		}
		ds = dummy;
	}
	DSCAPS dscaps;
	MemsetZero(dscaps);
	dscaps.dwSize = sizeof(dscaps);
	if(DS_OK != ds->GetCaps(&dscaps))
	{
		// nothing known about supported sample rates
	} else if(dscaps.dwMaxSecondarySampleRate == 0)
	{
		// nothing known about supported sample rates
	} else
	{
		for(std::size_t i = 0; i < baseSampleRates.size(); ++i)
		{
			if(dscaps.dwMinSecondarySampleRate <= baseSampleRates[i] && baseSampleRates[i] <= dscaps.dwMaxSecondarySampleRate)
			{
				caps.supportedSampleRates.push_back(baseSampleRates[i]);
			}
		}
	}
	if(dummy)
	{
		dummy->Release();
		dummy = nullptr;
	}
	ds = nullptr;
	return caps;
}


bool CDSoundDevice::InternalOpen()
//--------------------------------
{
	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext, m_Settings)) return false;
	WAVEFORMATEX *pwfx = &wfext.Format;

	const std::size_t bytesPerFrame = m_Settings.GetBytesPerFrame();

	DSBUFFERDESC dsbd;
	DSBCAPS dsc;

	if(m_piDS) return true;
	const std::wstring internalID = GetDeviceInternalID();
	GUID guid = internalID.empty() ? GUID() : Util::StringToGUID(internalID);
	if(DirectSoundCreate(internalID.empty() ? NULL : &guid, &m_piDS, NULL) != DS_OK) return false;
	if(!m_piDS) return false;
	m_piDS->SetCooperativeLevel(m_Settings.hWnd, m_Settings.ExclusiveMode ? DSSCL_WRITEPRIMARY : DSSCL_PRIORITY);
	m_bMixRunning = FALSE;
	m_nDSoundBufferSize = (m_Settings.LatencyMS * pwfx->nAvgBytesPerSec) / 1000;
	m_nDSoundBufferSize = (m_nDSoundBufferSize + (bytesPerFrame-1)) / bytesPerFrame * bytesPerFrame; // round up to full frame
	m_nDSoundBufferSize = Clamp(m_nDSoundBufferSize, (DWORD)DSBSIZE_MIN, (DWORD)DSBSIZE_MAX);
	if(!m_Settings.ExclusiveMode)
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
	m_dwWritePos = 0xFFFFFFFF;
	SetWakeupInterval(std::min(m_Settings.UpdateIntervalMS / 1000.0, m_nDSoundBufferSize / (2.0 * m_Settings.GetBytesPerSecond())));
	m_Flags.NeedsClippedFloat = (mpt::Windows::GetWinNTVersion() >= mpt::Windows::VerWinVista);
	SoundBufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_nDSoundBufferSize * 1.0 / m_Settings.GetBytesPerSecond();
	bufferAttributes.UpdateInterval = std::min(m_Settings.UpdateIntervalMS / 1000.0, m_nDSoundBufferSize / (2.0 * m_Settings.GetBytesPerSecond()));
	bufferAttributes.NumBuffers = 1;
	UpdateBufferAttributes(bufferAttributes);
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
	// done in InternalFillAudioBuffer
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


void CDSoundDevice::InternalFillAudioBuffer()
//-------------------------------------------
{
	if(!m_pMixBuffer)
	{
		RequestClose();
		return;
	}
	if(m_nDSoundBufferSize == 0)
	{
		RequestClose();
		return;
	}

	for(int refillCount = 0; refillCount < 2; ++refillCount)
	{
		// Refill the buffer at most twice so we actually sleep some time when CPU is overloaded.
		
		const std::size_t bytesPerFrame = m_Settings.GetBytesPerFrame();

		DWORD dwPlay = 0;
		DWORD dwWrite = 0;
		if(m_pMixBuffer->GetCurrentPosition(&dwPlay, &dwWrite) != DS_OK)
		{
			RequestClose();
			return;
		}

		DWORD dwBytes = m_nDSoundBufferSize/2;
		if(!m_bMixRunning)
		{
			// startup
			m_dwWritePos = dwWrite;
			m_dwLatency = 0;
		} else
		{
			// running
			m_dwLatency = (m_dwWritePos - dwPlay + m_nDSoundBufferSize) % m_nDSoundBufferSize;
			m_dwLatency = (m_dwLatency + m_nDSoundBufferSize - 1) % m_nDSoundBufferSize + 1;
			dwBytes = (dwPlay - m_dwWritePos + m_nDSoundBufferSize) % m_nDSoundBufferSize;
			dwBytes = Clamp(dwBytes, (DWORD)0u, m_nDSoundBufferSize/2); // limit refill amount to half the buffer size
		}
		dwBytes = dwBytes / bytesPerFrame * bytesPerFrame; // truncate to full frame
		if(dwBytes < bytesPerFrame)
		{
			// ok, nothing to do
			return;
		}

		void *buf1 = nullptr;
		void *buf2 = nullptr;
		DWORD dwSize1 = 0;
		DWORD dwSize2 = 0;
		HRESULT hr = m_pMixBuffer->Lock(m_dwWritePos, dwBytes, &buf1, &dwSize1, &buf2, &dwSize2, 0);
		if(hr == DSERR_BUFFERLOST)
		{
			// buffer lost, restore buffer and try again, fail if it fails again
			if(m_pMixBuffer->Restore() != DS_OK)
			{
				RequestClose();
				return;
			}
			if(m_pMixBuffer->Lock(m_dwWritePos, dwBytes, &buf1, &dwSize1, &buf2, &dwSize2, 0) != DS_OK)
			{
				RequestClose();
				return;
			}
		} else if(hr != DS_OK)
		{
			RequestClose();
			return;
		}

		SourceAudioPreRead(dwSize1/bytesPerFrame + dwSize2/bytesPerFrame);

		SourceAudioRead(buf1, dwSize1/bytesPerFrame);
		SourceAudioRead(buf2, dwSize2/bytesPerFrame);

		if(m_pMixBuffer->Unlock(buf1, dwSize1, buf2, dwSize2) != DS_OK)
		{
			RequestClose();
			return;
		}
		m_dwWritePos += dwSize1 + dwSize2;
		m_dwWritePos %= m_nDSoundBufferSize;

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
				if(m_pMixBuffer->Restore() != DS_OK)
				{
					RequestClose();
					return;
				}
				if(m_pMixBuffer->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
				{
					RequestClose();
					return;
				}
			} else if(hr != DS_OK)
			{
				RequestClose();
				return;
			}
			m_bMixRunning = TRUE; 
		}

		SourceAudioDone(dwSize1/bytesPerFrame + dwSize2/bytesPerFrame, m_dwLatency/bytesPerFrame);

		if(dwBytes < m_nDSoundBufferSize/2)
		{
			// Sleep again if we did fill less than half the buffer size.
			// Otherwise it's a better idea to refill again right away.
			break;
		}

	}

}


#endif // NO_DIRECTSOUND

