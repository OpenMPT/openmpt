/*
 * snddev.h
 * --------
 * Purpose: Sound device interface class.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <vector>
using std::vector;

////////////////////////////////////////////////////////////////////////////////////
//
// ISoundSource: provides streaming audio data to a device
//

//================
class ISoundSource
//================
{
public:
	virtual ULONG AudioRead(PVOID pData, ULONG MaxSamples) = 0; // returns number of valid samples read, the remaining space has to be filled with zeroes by the callee,
	                                                            // if return value != MaxSamples then end_of_stream = true
	virtual VOID AudioDone(ULONG SamplesWritten, ULONG SamplesLatency, bool end_of_stream) = 0; // all in samples
};


////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice Interface
//

// Sound Device Types
enum
{
	SNDDEV_WAVEOUT = 0,
	SNDDEV_DSOUND,
	SNDDEV_ASIO,
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

#define SNDDEV_OLD_MINBUFFERLEN			1    // 1ms
#define SNDDEV_OLD_MAXBUFFERLEN			1000 // 1sec

#define SNDDEV_OPTIONS_SECONDARY	0x01	// Use secondary buffers (if available)


//================
class ISoundDevice
//================
{
protected:
	ULONG m_LatencyMS;
	ULONG m_UpdateIntervalMS;
	ULONG m_fulCfgOptions;
	HWND m_hWnd;

	float m_RealLatencyMS;
	float m_RealUpdateIntervalMS;

public:
	ISoundDevice();
	virtual ~ISoundDevice();

public:
	VOID Configure(HWND hwnd, UINT LatencyMS, UINT UpdateIntervalMS, DWORD fdwCfgOptions);
	float GetRealLatencyMS() const  { return m_RealLatencyMS; }
	float GetRealUpdateIntervalMS() const { return m_RealUpdateIntervalMS; }

public:
	virtual UINT GetDeviceType() = 0;
	virtual BOOL Open(UINT nDevice, LPWAVEFORMATEX pwfx) = 0;	// Open a device
	virtual BOOL Close() = 0;				// Close the currently open device
	virtual void FillAudioBuffer(ISoundSource *pSource) = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void Reset() = 0;
	virtual BOOL Directcallback() { return FALSE; }
	virtual UINT HasFixedBitsPerSample() { return 0; }
	virtual bool IsOpen() const = 0;
	virtual UINT GetNumBuffers() { return 0; }
	virtual float GetCurrentRealLatencyMS() { return GetRealLatencyMS(); }
	virtual bool HasGetStreamPosition() const { return false; }
	virtual int64 GetStreamPositionSamples() const { return 0; }
	virtual UINT GetCurrentSampleRate(UINT nDevice) { UNREFERENCED_PARAMETER(nDevice); return 0; }
	// Return which samplerates are actually supported by the device. Currently only implemented properly for ASIO.
	virtual bool CanSampleRate(UINT nDevice, vector<UINT> &samplerates, vector<bool> &result) { UNREFERENCED_PARAMETER(nDevice); result.assign(samplerates.size(), true); return true; } ;
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
extern void SoundDeviceCallback();

////////////////////////////////////////////////////////////////////////////////////
