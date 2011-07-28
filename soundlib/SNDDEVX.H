#ifndef _SNDDEVX_H_
#define _SNDDEVX_H_

#include <mmsystem.h>
#include "snddev.h"

#ifndef NO_DSOUND
#include <dsound.h>
#endif

#ifndef NO_ASIO
#include <iasiodrv.h>
#define ASIO_LOG
#endif

////////////////////////////////////////////////////////////////////////////////////
//
// MMSYSTEM WaveOut device
//

#define WAVEOUT_MAXBUFFERS		16
#define WAVEOUT_MINBUFFERSIZE	1024
#define WAVEOUT_MAXBUFFERSIZE	32768

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
	LPWAVEHDR m_WaveBuffers[WAVEOUT_MAXBUFFERS];

public:
	CWaveDevice();
	~CWaveDevice();

public:
	UINT GetDeviceType() { return SNDDEV_WAVEOUT; }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	BOOL FillAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD);
	VOID Reset();
	VOID Start() {}
	BOOL IsOpen() { return (m_hWaveOut != NULL) ? TRUE : FALSE; }
	UINT GetCurrentLatency() { return m_nBuffersPending * m_nWaveBufferSize; }
	void SilenceAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD dwBuffer);

public:
	static VOID CALLBACK WaveOutCallBack(HWAVEOUT, UINT uMsg, DWORD, DWORD dw1, DWORD dw2);
	static BOOL EnumerateDevices(UINT nIndex, LPSTR pszDescription, UINT cbSize);
};


////////////////////////////////////////////////////////////////////////////////////
//
// DirectSound device
//

#ifndef NO_DSOUND

//======================================
class CDSoundDevice: public ISoundDevice
//======================================
{
protected:
	IDirectSound *m_piDS;
	IDirectSoundBuffer *m_pPrimary, *m_pMixBuffer;
	ULONG m_nDSoundBufferSize;
	BOOL m_bMixRunning;
	DWORD m_dwWritePos, m_dwLatency;

public:
	CDSoundDevice();
	~CDSoundDevice();

public:
	UINT GetDeviceType() { return SNDDEV_DSOUND; }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	BOOL FillAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD);
	VOID Reset();
	VOID Start() {}
	BOOL IsOpen() { return (m_pMixBuffer != NULL) ? TRUE : FALSE; }
	UINT GetCurrentLatency() { return m_dwLatency; }
	void SilenceAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD dwBuffer);

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
	ASIOCallbacks m_Callbacks;
	ASIOChannelInfo m_ChannelInfo[ASIO_MAX_CHANNELS];
	ASIOBufferInfo m_BufferInfo[ASIO_MAX_CHANNELS];
	int m_FrameBuffer[ASIO_BLOCK_LEN];

public:
	CASIODevice();
	~CASIODevice();

public:
	UINT GetDeviceType() { return SNDDEV_ASIO; }
	BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx);
	BOOL Close();
	BOOL FillAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD dwUser);
	VOID Reset();
	VOID Start();
	BOOL IsOpen() { return (m_pAsioDrv != NULL) ? TRUE : FALSE; }
	BOOL Directcallback() { return TRUE; }
	UINT HasFixedBitsPerSample() { return m_nBitsPerSample; }
	UINT GetCurrentLatency() { return m_nAsioBufferLen; }
	void SilenceAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD dwBuffer);

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

#endif // _SNDDEVX_H_
