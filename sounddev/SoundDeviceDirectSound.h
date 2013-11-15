/*
 * SoundDeviceDirectSound.h
 * ------------------------
 * Purpose: DirectSound sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDevices.h"

#ifndef NO_DSOUND
#include <dsound.h>
#endif

////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound device
//

#ifndef NO_DSOUND

//================================================
class CDSoundDevice: public CSoundDeviceWithThread
//================================================
{
protected:
	IDirectSound *m_piDS;
	IDirectSoundBuffer *m_pPrimary, *m_pMixBuffer;
	ULONG m_nDSoundBufferSize;
	BOOL m_bMixRunning;
	DWORD m_dwWritePos, m_dwLatency;

public:
	CDSoundDevice(SoundDeviceID id, const std::wstring &internalID);
	~CDSoundDevice();

public:
	bool InternalOpen();
	bool InternalClose();
	void FillAudioBuffer();
	void StartFromSoundThread();
	void StopFromSoundThread();
	bool InternalIsOpen() const { return (m_pMixBuffer != NULL); }
	double GetCurrentRealLatency() const { return 1.0 * m_dwLatency / m_Settings.GetBytesPerSecond(); }
	SoundDeviceCaps GetDeviceCaps(const std::vector<uint32> &baseSampleRates);

protected:
	DWORD LockBuffer(DWORD dwBytes, LPVOID *lpBuf1, LPDWORD lpSize1, LPVOID *lpBuf2, LPDWORD lpSize2);
	BOOL UnlockBuffer(LPVOID lpBuf1, DWORD dwSize1, LPVOID lpBuf2, DWORD dwSize2);

public:
	static std::vector<SoundDeviceInfo> EnumerateDevices();
};

#endif // NO_DIRECTSOUND

