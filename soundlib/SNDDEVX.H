/*
 * snddevx.h
 * ---------
 * Purpose: Actual sound device driver classes.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <mmsystem.h>
#include "snddev.h"

#ifndef NO_DSOUND
#include <dsound.h>
#endif

#ifndef NO_ASIO
#include <iasiodrv.h>
#define ASIO_LOG
#endif

#ifndef NO_PORTAUDIO
#include "portaudio/include/portaudio.h"
#endif

////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut device
//

#define WAVEOUT_MAXBUFFERS           256
#define WAVEOUT_MINBUFFERSIZE        1024
#define WAVEOUT_MAXBUFFERSIZE        65536

//====================================
class CWaveDevice: public ISoundDevice
//====================================
{
protected:
	HWAVEOUT m_hWaveOut;
	ULONG m_nWaveBufferSize;
	ULONG m_nPreparedHeaders;
	ULONG m_nWriteBuffer;
	LONG m_nBuffersPending;
	ULONG m_nBytesPerSec;
	ULONG m_BytesPerSample;
	LPWAVEHDR m_WaveBuffers[WAVEOUT_MAXBUFFERS];

public:
	CWaveDevice();
	~CWaveDevice();

public:
	UINT GetDeviceType() { return SNDDEV_WAVEOUT; }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	void FillAudioBuffer(ISoundSource *pSource);
	void Reset();
	void Start();
	void Stop();
	bool IsOpen() const { return (m_hWaveOut != NULL); }
	UINT GetNumBuffers() { return m_nPreparedHeaders; }
	float GetCurrentRealLatencyMS() { return InterlockedExchangeAdd(&m_nBuffersPending, 0) * m_nWaveBufferSize * 1000.0f / m_nBytesPerSec; }
	bool HasGetStreamPosition() const { return true; }
	int64 GetStreamPositionSamples() const;

public:
	static VOID CALLBACK WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR, DWORD_PTR dw1, DWORD_PTR dw2);
	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize);
};


////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound device
//

#ifndef NO_DSOUND

#define DSOUND_MINBUFFERSIZE 1024
#define DSOUND_MAXBUFFERSIZE SNDDEV_MAXBUFFERSIZE

//======================================
class CDSoundDevice: public ISoundDevice
//======================================
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
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	void FillAudioBuffer(ISoundSource *pSource);
	void Reset();
	void Start();
	void Stop();
	bool IsOpen() const { return (m_pMixBuffer != NULL); }
	UINT GetNumBuffers() { return 1; } // meaning 1 ring buffer
	float GetCurrentRealLatencyMS() { return m_dwLatency * 1000.0f / m_nBytesPerSec; }

protected:
	DWORD LockBuffer(DWORD dwBytes, LPVOID *lpBuf1, LPDWORD lpSize1, LPVOID *lpBuf2, LPDWORD lpSize2);
	BOOL UnlockBuffer(LPVOID lpBuf1, DWORD dwSize1, LPVOID lpBuf2, DWORD dwSize2);

public:
	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize);
};

#endif // NO_DIRECTSOUND


////////////////////////////////////////////////////////////////////////////////////
//
// ASIO device
//

#ifndef NO_ASIO

//====================================
class CASIODevice: public ISoundDevice
//====================================
{
	enum { ASIO_MAX_CHANNELS=4 };
	enum { ASIO_BLOCK_LEN=1024 };
protected:
	IASIO *m_pAsioDrv;
	UINT m_nChannels, m_nBitsPerSample, m_nAsioBufferLen, m_nAsioSampleSize;
	BOOL m_bMixRunning;
	BOOL m_bPostOutput;
	UINT m_nCurrentDevice;
	ULONG m_nSamplesPerSec;
	LONG m_RenderSilence;
	ASIOCallbacks m_Callbacks;
	ASIOChannelInfo m_ChannelInfo[ASIO_MAX_CHANNELS];
	ASIOBufferInfo m_BufferInfo[ASIO_MAX_CHANNELS];
	int m_FrameBuffer[ASIO_BLOCK_LEN];
public:
	static int baseChannel;

public:
	CASIODevice();
	~CASIODevice();

public:
	UINT GetDeviceType() { return SNDDEV_ASIO; }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	void FillAudioBuffer(ISoundSource *pSource);
	void Reset();
	void Start();
	void Stop();
	bool IsOpen() const { return (m_pAsioDrv != NULL); }
	BOOL Directcallback() { return TRUE; }
	UINT HasFixedBitsPerSample() { return m_nBitsPerSample; }
	UINT GetNumBuffers() { return 2; }
	float GetCurrentRealLatencyMS() { return m_nAsioBufferLen * 2 * 1000.0f / m_nSamplesPerSec;; }

	bool CanSampleRate(UINT nDevice, vector<UINT> &samplerates, vector<bool> &result);
	UINT GetCurrentSampleRate(UINT nDevice);

public:
	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize);

protected:
	void OpenDevice(UINT nDevice);
	void CloseDevice();

protected:
	static CASIODevice *gpCurrentAsio;
	static LONG gnFillBuffers;
	static void BufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	static void SampleRateDidChange(ASIOSampleRate sRate);
	static long AsioMessage(long selector, long value, void* message, double* opt);
	static ASIOTime* BufferSwitchTimeInfo(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
	static void EndianSwap64(void *pbuffer, UINT nSamples);
	static void EndianSwap32(void *pbuffer, UINT nSamples);
	static void Cvt16To16(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static void Cvt16To16msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static void Cvt32To16(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static void Cvt32To16msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static void Cvt32To24(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static void Cvt32To24msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static void Cvt32To32(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples, UINT nShift);
	static void Cvt32To32msb(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples, UINT nShift);
	static void Cvt32To32f(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static void Cvt32To64f(void *pdst, void *psrc, UINT nSampleSize, UINT nSamples);
	static BOOL ReportASIOException(LPCSTR format,...);
};

#endif // NO_ASIO


////////////////////////////////////////////////////////////////////////////////////
//
// Protaudio device
//

#ifndef NO_PORTAUDIO

class CPortaudioDevice: public ISoundDevice
	//====================================
{
protected:
	PaHostApiIndex m_HostApi;
	PaStreamParameters m_StreamParameters;
	PaStream * m_Stream;
	void * m_CurrentFrameBuffer;
	unsigned long m_CurrentFrameCount;

	float m_CurrentRealLatencyMS;

public:

public:
	CPortaudioDevice(PaHostApiIndex hostapi);
	~CPortaudioDevice();

public:
	UINT GetDeviceType() { return HostApiToSndDevType(m_HostApi); }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	void FillAudioBuffer(ISoundSource *pSource);
	void Reset();
	void Start();
	void Stop();
	bool IsOpen() const { return m_Stream ? true : false; }
	UINT GetNumBuffers() { return 1; }
	float GetCurrentRealLatencyMS();
	BOOL Directcallback() { return true; }
	bool HasGetStreamPosition() const { return false; }
	int64 GetStreamPositionSamples() const;

	int StreamCallback(
		const void *input, void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags
		);

public:
	static int StreamCallbackWrapper(
		const void *input, void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData
		);

	static PaDeviceIndex HostApiOutputIndexToGlobalDeviceIndex(int hostapideviceindex, PaHostApiIndex hostapi);
	static int HostApiToSndDevType(PaHostApiIndex hostapi);
	static PaHostApiIndex SndDevTypeToHostApi(int snddevtype);

	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize, PaHostApiIndex hostapi);

};

#endif // NO_PORTAUDIO
