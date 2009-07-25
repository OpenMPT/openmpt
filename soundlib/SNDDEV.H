#ifndef _SNDDEV_H_
#define _SNDDEV_H_

////////////////////////////////////////////////////////////////////////////////////
//
// ISoundSource: provides streaming audio data to a device
//

//================
class ISoundSource
//================
{
public:
	virtual ULONG AudioRead(PVOID pData, ULONG cbSize) = 0;
	virtual VOID AudioDone(ULONG nBytesWritten, ULONG nLatency) = 0; // All in bytes
};


////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice Interface
//

// Sound Device Types
enum {
	SNDDEV_WAVEOUT=0,
	SNDDEV_DSOUND,
	SNDDEV_ASIO,
	SNDDEV_NUM_DEVTYPES
};

#define SNDDEV_MINBUFFERS			2
#define SNDDEV_MAXBUFFERS			16
#define SNDDEV_MINBUFFERLEN			1		// 1ms
#define SNDDEV_MAXBUFFERLEN			1000	// 1sec

#define SNDDEV_OPTIONS_SECONDARY	0x01	// Use secondary buffers (if available)


//================
class ISoundDevice
//================
{
protected:
	ULONG m_RefCount;
	ULONG m_nBuffers;
	ULONG m_nBufferLen;
	ULONG m_fulCfgOptions;
	ULONG m_nMaxFillInterval;
	HWND m_hWnd;

public:
	ISoundDevice();
	virtual ~ISoundDevice();
	ULONG AddRef();
	ULONG Release();

public:
	VOID Configure(HWND hwnd, UINT nBuffers, UINT nBufferLen, DWORD fdwCfgOptions);
	ULONG GetMaxFillInterval() const { return m_nMaxFillInterval; }
	ULONG GetNumBuffers() const { return m_nBuffers; }
	ULONG GetBufferLength() const { return m_nBufferLen; }

public:
	virtual UINT GetDeviceType() = 0;
	virtual BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx) = 0;	// Open a device
	virtual BOOL Close() = 0;				// Close the currently open device
	virtual BOOL FillAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD dwUser=0) = 0;
	virtual VOID Start() = 0;
	virtual VOID Reset() = 0;
	virtual BOOL Directcallback() { return FALSE; }
	virtual UINT HasFixedBitsPerSample() { return 0; }
	virtual BOOL IsOpen() = 0;
	virtual UINT GetCurrentLatency() = 0;
	virtual void SilenceAudioBuffer(ISoundSource *pSource, ULONG nMaxLatency, DWORD dwUser=0) = 0;
};


////////////////////////////////////////////////////////////////////////////////////
//
// Global Functions
//

// Initialization
BOOL SndDevInitialize();
BOOL SndDevUninitialize();

// Enumerate devices for a specific type
BOOL EnumerateSoundDevices(UINT nType, UINT nIndex, LPSTR pszDescription, UINT cbSize);
BOOL CreateSoundDevice(UINT nType, ISoundDevice **ppsd);
extern BOOL SoundDeviceCallback(DWORD dwUser);

////////////////////////////////////////////////////////////////////////////////////

#endif // _SNDDEV_H_

