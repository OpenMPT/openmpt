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

#define DSOUND_MINBUFFERSIZE 1024
#define DSOUND_MAXBUFFERSIZE SNDDEV_MAXBUFFERSIZE

//================================================
class CDSoundDevice: public CSoundDeviceWithThread
//================================================
{
protected:
	IDirectSound *m_piDS;
	IDirectSoundBuffer *m_pPrimary, *m_pMixBuffer;
	ULONG m_nDSoundBufferSize;
	ULONG m_nBytesPerSec;
	ULONG m_BytesPerSample;
	BOOL m_bMixRunning;
	DWORD m_dwWritePos, m_dwLatency;

public:
	CDSoundDevice();
	~CDSoundDevice();

public:
	UINT GetDeviceType() { return SNDDEV_DSOUND; }
	bool InternalOpen(UINT nDevice);
	bool InternalClose();
	void FillAudioBuffer();
	void ResetFromOutsideSoundThread();
	void StartFromSoundThread();
	void StopFromSoundThread();
	bool IsOpen() const { return (m_pMixBuffer != NULL); }
	UINT GetNumBuffers() { return 1; } // meaning 1 ring buffer
	float GetCurrentRealLatencyMS() { return m_dwLatency * 1000.0f / m_nBytesPerSec; }

protected:
	DWORD LockBuffer(DWORD dwBytes, LPVOID *lpBuf1, LPDWORD lpSize1, LPVOID *lpBuf2, LPDWORD lpSize2);
	BOOL UnlockBuffer(LPVOID lpBuf1, DWORD dwSize1, LPVOID lpBuf2, DWORD dwSize2);

public:
	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize);
};

BOOL SndDevDSoundInitialize();

BOOL SndDevDSoundUninitialize();

#endif // NO_DIRECTSOUND

