/*
 * SoundDevices.h
 * --------------
 * Purpose: Actual sound device driver classes.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <mmsystem.h>
#include "SoundDevice.h"

#ifndef NO_DSOUND
#include <dsound.h>
#endif

#ifndef NO_ASIO
#include <iasiodrv.h>
#define ASIO_LOG
#endif

#ifndef NO_PORTAUDIO
#include "portaudio/include/portaudio.h"
#include "portaudio/include/pa_win_wasapi.h"
#endif


class CSoundDeviceWithThread;

class CAudioThread
{
	friend class CPriorityBooster;
	friend class CPeriodicWaker;
private:
	CSoundDeviceWithThread & m_SoundDevice;

	bool m_HasXP;
	bool m_HasVista;
	HMODULE m_hKernel32DLL;
	HMODULE m_hAvRtDLL;
	typedef HANDLE (WINAPI *FCreateWaitableTimer)(LPSECURITY_ATTRIBUTES, BOOL, LPCTSTR);
	typedef BOOL (WINAPI *FSetWaitableTimer)(HANDLE, const LARGE_INTEGER *, LONG, PTIMERAPCROUTINE, LPVOID, BOOL);
	typedef BOOL (WINAPI *FCancelWaitableTimer)(HANDLE);
	typedef HANDLE (WINAPI *FAvSetMmThreadCharacteristics)(LPCTSTR, LPDWORD);
	typedef BOOL (WINAPI *FAvRevertMmThreadCharacteristics)(HANDLE);
	FCreateWaitableTimer pCreateWaitableTimer;
	FSetWaitableTimer pSetWaitableTimer;
	FCancelWaitableTimer pCancelWaitableTimer;
	FAvSetMmThreadCharacteristics pAvSetMmThreadCharacteristics;
	FAvRevertMmThreadCharacteristics pAvRevertMmThreadCharacteristics;

	HANDLE m_hAudioWakeUp;
	HANDLE m_hPlayThread;
	HANDLE m_hAudioThreadTerminateRequest;
	HANDLE m_hAudioThreadGoneIdle;
	DWORD m_dwPlayThreadId;
	LONG m_AudioThreadActive;
	static DWORD WINAPI AudioThreadWrapper(LPVOID user);
	DWORD AudioThread();
	bool IsActive() { return InterlockedExchangeAdd(&m_AudioThreadActive, 0)?true:false; }
public:
	CAudioThread(CSoundDeviceWithThread &SoundDevice);
	~CAudioThread();
	void Activate();
	void Deactivate();
};


class CSoundDeviceWithThread : public ISoundDevice
{
	friend class CAudioThread;
protected:
	CAudioThread m_AudioThread;
private:
	void FillAudioBufferLocked();
public:
	CSoundDeviceWithThread() : m_AudioThread(*this) {}
	virtual ~CSoundDeviceWithThread() {}
	void InternalStart();
	void InternalStop();
	void InternalReset();
	virtual void StartFromSoundThread() = 0;
	virtual void StopFromSoundThread() = 0;
	virtual void ResetFromOutsideSoundThread() = 0;
};


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
	UINT GetDeviceType() { return SNDDEV_WAVEOUT; }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	void FillAudioBuffer();
	void ResetFromOutsideSoundThread();
	void StartFromSoundThread();
	void StopFromSoundThread();
	bool IsOpen() const { return (m_hWaveOut != NULL); }
	UINT GetNumBuffers() { return m_nPreparedHeaders; }
	float GetCurrentRealLatencyMS() { return InterlockedExchangeAdd(&m_nBuffersPending, 0) * m_nWaveBufferSize * 1000.0f / m_nBytesPerSec; }
	bool HasGetStreamPosition() const { return true; }
	int64 GetStreamPositionSamples() const;

public:
	static void CALLBACK WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD_PTR, DWORD_PTR dw1, DWORD_PTR dw2);
	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize);
};


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
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
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
	LONG m_RenderingSilence;
	ASIOCallbacks m_Callbacks;
	ASIOChannelInfo m_ChannelInfo[ASIO_MAX_CHANNELS];
	ASIOBufferInfo m_BufferInfo[ASIO_MAX_CHANNELS];
	int m_FrameBuffer[ASIO_BLOCK_LEN];

private:
	void SetRenderSilence(bool silence, bool wait=false);

public:
	static int baseChannel;

public:
	CASIODevice();
	~CASIODevice();

public:
	UINT GetDeviceType() { return SNDDEV_ASIO; }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	void FillAudioBuffer();
	void InternalReset();
	void InternalStart();
	void InternalStop();
	bool IsOpen() const { return (m_pAsioDrv != NULL); }
	UINT HasFixedBitsPerSample() { return m_nBitsPerSample; }
	UINT GetNumBuffers() { return 2; }
	float GetCurrentRealLatencyMS() { return m_nAsioBufferLen * 2 * 1000.0f / m_nSamplesPerSec;; }

	bool CanSampleRate(UINT nDevice, std::vector<UINT> &samplerates, std::vector<bool> &result);
	UINT GetCurrentSampleRate(UINT nDevice);

public:
	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize);

protected:
	void OpenDevice(UINT nDevice);
	void CloseDevice();

protected:
	void BufferSwitch(long doubleBufferIndex);

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

//=========================================
class CPortaudioDevice: public ISoundDevice
//=========================================
{
protected:
	PaHostApiIndex m_HostApi;
	PaStreamParameters m_StreamParameters;
	PaWasapiStreamInfo m_WasapiStreamInfo;
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
	void FillAudioBuffer();
	void InternalReset();
	void InternalStart();
	void InternalStop();
	bool IsOpen() const { return m_Stream ? true : false; }
	UINT GetNumBuffers() { return 1; }
	float GetCurrentRealLatencyMS();
	bool HasGetStreamPosition() const { return false; }
	int64 GetStreamPositionSamples() const;
	bool CanSampleRate(UINT nDevice, std::vector<UINT> &samplerates, std::vector<bool> &result);

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

	static PaDeviceIndex HostApiOutputIndexToGlobalDeviceIndex(int hostapioutputdeviceindex, PaHostApiIndex hostapi);
	static int HostApiToSndDevType(PaHostApiIndex hostapi);
	static PaHostApiIndex SndDevTypeToHostApi(int snddevtype);

	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize, PaHostApiIndex hostapi);

};

#endif // NO_PORTAUDIO
