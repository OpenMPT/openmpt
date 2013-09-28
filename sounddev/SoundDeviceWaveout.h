/*
 * SoundDeviceWaveout.h
 * --------------------
 * Purpose: Waveout sound device driver class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "SoundDevices.h"

#include <MMSystem.h>

////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut device
//

#define WAVEOUT_MAXBUFFERS           256
#define WAVEOUT_MINBUFFERSIZE        1024
#define WAVEOUT_MAXBUFFERSIZE        65536

//==============================================
class CWaveDevice: public CSoundDeviceWithThread
//==============================================
{
protected:
	HWAVEOUT m_hWaveOut;
	ULONG m_nWaveBufferSize;
	ULONG m_nPreparedHeaders;
	ULONG m_nWriteBuffer;
	LONG m_nBuffersPending;
	ULONG m_nBytesPerSec;
	ULONG m_BytesPerSample;
	std::vector<WAVEHDR> m_WaveBuffers;
	std::vector<std::vector<char> > m_WaveBuffersData;

public:
	CWaveDevice();
	~CWaveDevice();

public:
	UINT GetDeviceType() const { return SNDDEV_WAVEOUT; }
	bool InternalOpen();
	bool InternalClose();
	void FillAudioBuffer();
	void ResetFromOutsideSoundThread();
	void StartFromSoundThread();
	void StopFromSoundThread();
	bool IsOpen() const { return (m_hWaveOut != NULL); }
	UINT GetNumBuffers() { return m_nPreparedHeaders; }
	float GetCurrentRealLatencyMS() { return InterlockedExchangeAdd(&m_nBuffersPending, 0) * m_nWaveBufferSize * 1000.0f / m_nBytesPerSec; }
	bool InternalHasGetStreamPosition() const { return true; }
	int64 InternalGetStreamPositionSamples() const;

public:
	static void CALLBACK WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR, DWORD_PTR dw1, DWORD_PTR dw2);
	static std::vector<SoundDeviceInfo> EnumerateDevices();
};
