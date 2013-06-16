/*
 * SoundDevice.h
 * -------------
 * Purpose: Sound device interface class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <vector>


class ISoundDevice;
class IFillAudioBuffer;
class ISoundSource;


////////////////////////////////////////////////////////////////////////////////////
//
// ISoundSource: provides streaming audio data to a device
//


//====================
class IFillAudioBuffer
//====================
{
public:
	virtual void FillAudioBuffer() = 0;
};


//================
class ISoundSource
//================
{
public:
	virtual void FillAudioBufferLocked(IFillAudioBuffer &callback) = 0; // take any locks needed while rendering audio and then call FillAudioBuffer
	virtual void AudioRead(void* pData, ULONG NumSamples) = 0;
	virtual void AudioDone(ULONG NumSamples, ULONG SamplesLatency) = 0; // all in samples
	virtual void AudioDone(ULONG NumSamples) = 0; // all in samples
};


////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice Interface
//

// Sound Device Types
enum // do not change old values, these get saved to the ini
{
	SNDDEV_WAVEOUT          = 0,
	SNDDEV_DSOUND           = 1,
	SNDDEV_ASIO             = 2,
	SNDDEV_PORTAUDIO_WASAPI = 3,
	SNDDEV_PORTAUDIO_WDMKS  = 4,
	SNDDEV_PORTAUDIO_WMME   = 5,
	SNDDEV_PORTAUDIO_DS     = 6,
	SNDDEV_PORTAUDIO_ASIO   = 7,
	SNDDEV_NUM_DEVTYPES
};

#define SNDDEV_DEVICE_MASK				0xFF	// Mask for getting the device number
#define SNDDEV_DEVICE_SHIFT				8		// Shift amount for getting the device type
#define SNDDEV_GET_NUMBER(x)			(x & SNDDEV_DEVICE_MASK)	// Use this for getting the device number
#define SNDDEV_GET_TYPE(x)				(x >> SNDDEV_DEVICE_SHIFT)	// ...and this for getting the device type
#define SNDDEV_BUILD_ID(number, type)	((number & SNDDEV_DEVICE_MASK) | (type << SNDDEV_DEVICE_SHIFT)) // Build a sound device ID from device number and device type.

#define SNDDEV_DEFAULT_LATENCY_MS        100
#define SNDDEV_DEFAULT_UPDATEINTERVAL_MS   5

#define SNDDEV_MAXBUFFERS	       	  256
#define SNDDEV_MAXBUFFERSIZE        (1024*1024)
#define SNDDEV_MINLATENCY_MS        1
#define SNDDEV_MAXLATENCY_MS        500
#define SNDDEV_MINUPDATEINTERVAL_MS 1
#define SNDDEV_MAXUPDATEINTERVAL_MS 200

#define SNDDEV_OPTIONS_EXCLUSIVE           0x01 // Use hardware buffers directly
#define SNDDEV_OPTIONS_BOOSTTHREADPRIORITY 0x02 // Boost thread priority for glitch-free audio rendering


//=============================================
class ISoundDevice : protected IFillAudioBuffer
//=============================================
{
private:
	ISoundSource *m_Source;

protected:
	ULONG m_LatencyMS;
	ULONG m_UpdateIntervalMS;
	ULONG m_fulCfgOptions;
	HWND m_hWnd;

	float m_RealLatencyMS;
	float m_RealUpdateIntervalMS;

	bool m_IsPlaying;

protected:
	virtual void FillAudioBuffer() = 0;
	void SourceFillAudioBufferLocked();
	void SourceAudioRead(void* pData, ULONG NumSamples);
	void SourceAudioDone(ULONG NumSamples, ULONG SamplesLatency);

public:
	ISoundDevice();
	virtual ~ISoundDevice();
	void SetSource(ISoundSource *source) { m_Source = source; }
	ISoundSource *GetSource() const { return m_Source; }

public:
	void Configure(HWND hwnd, UINT LatencyMS, UINT UpdateIntervalMS, DWORD fdwCfgOptions);
	float GetRealLatencyMS() const  { return m_RealLatencyMS; }
	float GetRealUpdateIntervalMS() const { return m_RealUpdateIntervalMS; }
	bool IsPlaying() const { return m_IsPlaying; }

protected:
	virtual void InternalStart() = 0;
	virtual void InternalStop() = 0;
	virtual void InternalReset() = 0;

public:
	virtual UINT GetDeviceType() = 0;
	virtual BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx) = 0;	// Open a device
	virtual BOOL Close() = 0;				// Close the currently open device
	void Start();
	void Stop();
	void Reset();
	virtual UINT HasFixedBitsPerSample() { return 0; }
	virtual bool IsOpen() const = 0;
	virtual UINT GetNumBuffers() { return 0; }
	virtual float GetCurrentRealLatencyMS() { return GetRealLatencyMS(); }
	virtual bool HasGetStreamPosition() const { return false; }
	virtual int64 GetStreamPositionSamples() const { return 0; }
	virtual UINT GetCurrentSampleRate(UINT nDevice) { UNREFERENCED_PARAMETER(nDevice); return 0; }
	// Return which samplerates are actually supported by the device. Currently only implemented properly for ASIO.
	virtual bool CanSampleRate(UINT nDevice, std::vector<UINT> &samplerates, std::vector<bool> &result) { UNREFERENCED_PARAMETER(nDevice); result.assign(samplerates.size(), true); return true; } ;
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
ISoundDevice *CreateSoundDevice(UINT nType);

////////////////////////////////////////////////////////////////////////////////////
