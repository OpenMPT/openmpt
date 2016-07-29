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
#include "SoundDeviceUtilities.h"

#include "SoundDeviceDirectSound.h"

#include "../common/misc_util.h"
#include "../common/StringFixer.h"


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


#ifdef MPT_WITH_DSOUND


MPT_REGISTERED_COMPONENT(ComponentDirectSound, "DirectSound")



namespace Legacy
{
static BOOL WINAPI DSEnumCallbackGetDefaultName(GUID * lpGuid, LPCWSTR lpstrDescription, LPCWSTR, LPVOID lpContext)
{
	mpt::ustring & name = *reinterpret_cast<mpt::ustring*>(lpContext);
	if(!lpGuid)
	{
		if(lpstrDescription)
		{
			name = mpt::ToUnicode(std::wstring(lpstrDescription));
			return FALSE;
		}
	}
	return TRUE;
}
mpt::ustring GetDirectSoundDefaultDeviceIdentifierPre_1_25_00_04()
{
	mpt::ustring name = mpt::ustring();
	ComponentHandle<ComponentDirectSound> drectSound;
	if(!IsComponentAvailable(drectSound))
	{
		return name;
	}
	DirectSoundEnumerateW(DSEnumCallbackGetDefaultName, &name);
	if(name.empty())
	{
		return name;
	}
	std::string utf8String = mpt::ToCharset(mpt::CharsetUTF8, name);
	mpt::ustring hexString = Util::BinToHex(std::vector<char>(utf8String.begin(), utf8String.end()));
	return MPT_USTRING("DirectSound") + MPT_USTRING("_") + hexString;
}
mpt::ustring GetDirectSoundDefaultDeviceIdentifier_1_25_00_04()
{
	return MPT_USTRING("DirectSound_{00000000-0000-0000-0000-000000000000}");
}
}


static BOOL WINAPI DSEnumCallbackW(GUID * lpGuid, LPCWSTR lpstrDescription, LPCWSTR lpstrDriver, LPVOID lpContext)
//----------------------------------------------------------------------------------------------------------------
{
	std::vector<SoundDevice::Info> &devices = *(std::vector<SoundDevice::Info>*)lpContext;
	if(!lpstrDescription)
	{
		return TRUE;
	}
	GUID guid = (lpGuid ? *lpGuid : GUID());
	SoundDevice::Info info;
	info.type = TypeDSOUND;
	info.isDefault = (!lpGuid);
	info.internalID = mpt::ToUnicode(Util::GUIDToString(guid));
	info.name = mpt::ToUnicode(lpstrDescription);
	if(lpstrDriver)
	{
		info.extraData[MPT_USTRING("DriverName")] = mpt::ToUnicode(lpstrDriver);
	}
	if(lpGuid)
	{
		info.extraData[MPT_USTRING("UUID")] = mpt::UUID(guid).ToUString();
	}
	info.apiName = MPT_USTRING("DirectSound");
	info.useNameAsIdentifier = false;
	devices.push_back(info);
	return TRUE;
}


std::vector<SoundDevice::Info> CDSoundDevice::EnumerateDevices(SoundDevice::SysInfo /* sysInfo */ )
//-------------------------------------------------------------------------------------------------
{
	std::vector<SoundDevice::Info> devices;
	DirectSoundEnumerateW(DSEnumCallbackW, &devices);
	return devices;
}


CDSoundDevice::CDSoundDevice(SoundDevice::Info info, SoundDevice::SysInfo sysInfo)
//--------------------------------------------------------------------------------
	: CSoundDeviceWithThread(info, sysInfo)
	, m_piDS(NULL)
	, m_pPrimary(NULL)
	, m_pMixBuffer(NULL)
	, m_nDSoundBufferSize(0)
	, m_bMixRunning(FALSE)
	, m_dwWritePos(0)
	, m_StatisticLatencyFrames(0)
	, m_StatisticPeriodFrames(0)
{
	return;
}


CDSoundDevice::~CDSoundDevice()
//-----------------------------
{
	Close();
}


SoundDevice::Caps CDSoundDevice::InternalGetDeviceCaps()
//----------------------------------------------------
{
	SoundDevice::Caps caps;
	caps.Available = true;
	caps.CanUpdateInterval = true;
	caps.CanSampleFormat = true;
	caps.CanExclusiveMode = false;
	caps.CanBoostThreadPriority = true;
	caps.CanUseHardwareTiming = false;
	caps.CanChannelMapping = false;
	caps.CanInput = false;
	caps.HasNamedInputSources = false;
	caps.CanDriverPanel = false;
	caps.ExclusiveModeDescription = MPT_USTRING("Use primary buffer");
	caps.DefaultSettings.sampleFormat = SampleFormatInt16;
	IDirectSound *dummy = nullptr;
	IDirectSound *ds = nullptr;
	if(m_piDS)
	{
		ds = m_piDS;
	} else
	{
		GUID guid = Util::StringToGUID(mpt::ToWide(GetDeviceInternalID()));
		if(DirectSoundCreate(Util::IsValid(guid) ? &guid : NULL, &dummy, NULL) != DS_OK)
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
		if(!(dscaps.dwFlags & DSCAPS_EMULDRIVER))
		{
			caps.CanExclusiveMode = true;
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


SoundDevice::DynamicCaps CDSoundDevice::GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates)
//----------------------------------------------------------------------------------------------------
{
	SoundDevice::DynamicCaps caps;
	IDirectSound *dummy = nullptr;
	IDirectSound *ds = nullptr;
	if(m_piDS)
	{
		ds = m_piDS;
	} else
	{
		GUID guid = Util::StringToGUID(mpt::ToWide(GetDeviceInternalID()));
		if(DirectSoundCreate(Util::IsValid(guid) ? &guid : NULL, &dummy, NULL) != DS_OK)
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
				caps.supportedExclusiveSampleRates.push_back(baseSampleRates[i]);
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
	if(m_Settings.InputChannels > 0) return false;

	WAVEFORMATEXTENSIBLE wfext;
	if(!FillWaveFormatExtensible(wfext, m_Settings)) return false;
	WAVEFORMATEX *pwfx = &wfext.Format;

	const uint32 bytesPerFrame = static_cast<uint32>(m_Settings.GetBytesPerFrame());

	DSBUFFERDESC dsbd;
	DSBCAPS dsc;

	if(m_piDS) return true;
	GUID guid = Util::StringToGUID(mpt::ToWide(GetDeviceInternalID()));
	if(DirectSoundCreate(Util::IsValid(guid) ? &guid : NULL, &m_piDS, NULL) != DS_OK) return false;
	if(!m_piDS) return false;
	if(m_piDS->SetCooperativeLevel(m_AppInfo.GetHWND(), m_Settings.ExclusiveMode ? DSSCL_WRITEPRIMARY : DSSCL_PRIORITY) != DS_OK)
	{
		Close();
		return false;
	}
	m_bMixRunning = FALSE;
	m_nDSoundBufferSize = Util::Round<int32>(m_Settings.Latency * pwfx->nAvgBytesPerSec);
	m_nDSoundBufferSize = Util::AlignUp<uint32>(m_nDSoundBufferSize, bytesPerFrame);
	m_nDSoundBufferSize = Clamp(m_nDSoundBufferSize, Util::AlignUp<uint32>(DSBSIZE_MIN, bytesPerFrame), Util::AlignDown<uint32>(DSBSIZE_MAX, bytesPerFrame));
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
		if(m_pPrimary->SetFormat(pwfx) != DS_OK)
		{
			Close();
			return false;
		}
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
	SetWakeupInterval(std::min(m_Settings.UpdateInterval, m_nDSoundBufferSize / (2.0 * m_Settings.GetBytesPerSecond())));
	m_Flags.NeedsClippedFloat = GetSysInfo().WindowsVersion.IsAtLeast(mpt::Windows::Version::WinVista);
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

	DWORD dwLatency = 0;

	for(int refillCount = 0; refillCount < 2; ++refillCount)
	{
		// Refill the buffer at most twice so we actually sleep some time when CPU is overloaded.
		
		const uint32 bytesPerFrame = static_cast<uint32>(m_Settings.GetBytesPerFrame());

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
			dwLatency = 0;
		} else
		{
			// running
			dwLatency = (m_dwWritePos - dwPlay + m_nDSoundBufferSize) % m_nDSoundBufferSize;
			dwLatency = (dwLatency + m_nDSoundBufferSize - 1) % m_nDSoundBufferSize + 1;
			dwBytes = (dwPlay - m_dwWritePos + m_nDSoundBufferSize) % m_nDSoundBufferSize;
			dwBytes = Clamp(dwBytes, DWORD(0), m_nDSoundBufferSize/2); // limit refill amount to half the buffer size
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

		SourceLockedAudioPreRead(dwSize1/bytesPerFrame + dwSize2/bytesPerFrame, dwLatency/bytesPerFrame);

		SourceLockedAudioRead(buf1, nullptr, dwSize1/bytesPerFrame);
		SourceLockedAudioRead(buf2, nullptr, dwSize2/bytesPerFrame);

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

		m_StatisticLatencyFrames.store(dwLatency/bytesPerFrame);
		m_StatisticPeriodFrames.store(dwSize1/bytesPerFrame + dwSize2/bytesPerFrame);
		SourceLockedAudioDone();

		if(dwBytes < m_nDSoundBufferSize/2)
		{
			// Sleep again if we did fill less than half the buffer size.
			// Otherwise it's a better idea to refill again right away.
			break;
		}

	}

}


SoundDevice::BufferAttributes CDSoundDevice::InternalGetEffectiveBufferAttributes() const
//---------------------------------------------------------------------------------------
{
	SoundDevice::BufferAttributes bufferAttributes;
	bufferAttributes.Latency = m_nDSoundBufferSize * 1.0 / m_Settings.GetBytesPerSecond();
	bufferAttributes.UpdateInterval = std::min(m_Settings.UpdateInterval, m_nDSoundBufferSize / (2.0 * m_Settings.GetBytesPerSecond()));
	bufferAttributes.NumBuffers = 1;
	return bufferAttributes;
}


SoundDevice::Statistics CDSoundDevice::GetStatistics() const
//----------------------------------------------------------
{
	MPT_TRACE();
	SoundDevice::Statistics result;
	result.InstantaneousLatency = 1.0 * m_StatisticLatencyFrames.load() / m_Settings.Samplerate;
	result.LastUpdateInterval = 1.0 * m_StatisticPeriodFrames.load() / m_Settings.Samplerate;
	result.text = mpt::ustring();
	return result;
}


#endif // NO_DIRECTSOUND


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
