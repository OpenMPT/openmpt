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

//==============================================
class CWaveDevice: public CSoundDeviceWithThread
//==============================================
{
protected:
	HANDLE m_ThreadWakeupEvent;
	HWAVEOUT m_hWaveOut;
	ULONG m_nWaveBufferSize;
	bool m_JustStarted;
	ULONG m_nPreparedHeaders;
	ULONG m_nWriteBuffer;
	mutable LONG m_nBuffersPending;
	std::vector<WAVEHDR> m_WaveBuffers;
	std::vector<std::vector<char> > m_WaveBuffersData;

public:
	CWaveDevice(SoundDeviceID id, const std::wstring &internalID);
	~CWaveDevice();

public:
	bool InternalOpen();
	bool InternalClose();
	void InternalFillAudioBuffer();
	void StartFromSoundThread();
	void StopFromSoundThread();
	bool InternalIsOpen() const { return (m_hWaveOut != NULL); }
	double GetCurrentLatency() const { return InterlockedExchangeAdd(&m_nBuffersPending, 0) * m_nWaveBufferSize * 1.0 / m_Settings.GetBytesPerSecond(); }
	bool InternalHasGetStreamPosition() const { return true; }
	int64 InternalGetStreamPositionFrames() const;

public:
	static void CALLBACK WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR, DWORD_PTR dw1, DWORD_PTR dw2);
	static std::vector<SoundDeviceInfo> EnumerateDevices();
};
