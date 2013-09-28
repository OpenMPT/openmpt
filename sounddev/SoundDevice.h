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

#include "../common/mutex.h"
#include "../soundlib/SampleFormat.h"

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
	virtual void AudioDone(ULONG NumSamples, int64 streamPosition) = 0; // in samples
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


struct SoundDeviceSettings
{
	HWND hWnd;
	ULONG LatencyMS;
	ULONG UpdateIntervalMS;
	ULONG fulCfgOptions;
	uint32 Samplerate;
	uint8 Channels;
	SampleFormat sampleFormat;
	SoundDeviceSettings()
		: hWnd(NULL)
		, LatencyMS(SNDDEV_DEFAULT_LATENCY_MS)
		, UpdateIntervalMS(SNDDEV_DEFAULT_UPDATEINTERVAL_MS)
		, fulCfgOptions(0)
		, Samplerate(48000)
		, Channels(2)
		, sampleFormat(SampleFormatInt16)
	{
		return;
	}
};


class SoundDevicesManager;


//=============================================
class ISoundDevice : protected IFillAudioBuffer
//=============================================
{
	friend class SoundDevicesManager;

private:
	ISoundSource *m_Source;

protected:

	UINT m_Index;
	std::wstring m_InternalID;

	SoundDeviceSettings m_Settings;

	float m_RealLatencyMS;
	float m_RealUpdateIntervalMS;

	bool m_IsPlaying;

	mutable Util::mutex m_SamplesRenderedMutex;
	int64 m_SamplesRendered;

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
protected:
	void SetDevice(UINT index, const std::wstring &internalID);
public:
	UINT GetDeviceIndex() const { return m_Index; }
	UINT GetDeviceID() const { return SNDDEV_BUILD_ID(GetDeviceIndex(), GetDeviceType()); }

	std::wstring GetDeviceInternalID() const { return m_InternalID; }

public:
	float GetRealLatencyMS() const  { return m_RealLatencyMS; }
	float GetRealUpdateIntervalMS() const { return m_RealUpdateIntervalMS; }
	bool IsPlaying() const { return m_IsPlaying; }

protected:
	bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat);

protected:
	virtual bool InternalOpen() = 0;
	virtual void InternalStart() = 0;
	virtual void InternalStop() = 0;
	virtual void InternalReset() = 0;
	virtual bool InternalClose() = 0;
	virtual bool InternalHasGetStreamPosition() const { return false; }
	virtual int64 InternalGetStreamPositionSamples() const { return 0; }

public:
	virtual UINT GetDeviceType() const = 0;
	bool Open(const SoundDeviceSettings &settings);	// Open a device
	bool Close();				// Close the currently open device
	void Start();
	void Stop();
	void Reset();
	int64 GetStreamPositionSamples() const;
	virtual SampleFormat HasFixedSampleFormat() { return SampleFormatInvalid; }
	virtual bool IsOpen() const = 0;
	virtual UINT GetNumBuffers() { return 0; }
	virtual float GetCurrentRealLatencyMS() { return GetRealLatencyMS(); }
	virtual UINT GetCurrentSampleRate() { return 0; }
	// Return which samplerates are actually supported by the device. Currently only implemented properly for ASIO.
	virtual bool CanSampleRate(const std::vector<uint32> &samplerates, std::vector<bool> &result) { result.assign(samplerates.size(), true); return true; } ;
};


struct SoundDeviceInfo
{
	UINT type;
	UINT index;
	std::wstring name;
	std::wstring internalID;
	SoundDeviceInfo()
		: type(0)
		, index(0)
	{
		return;
	}
	SoundDeviceInfo(UINT type, UINT index, const std::wstring &name, const std::wstring &id = std::wstring())
		: type(type)
		, index(index)
		, name(name)
		, internalID(id)
	{
		return;
	}
};


//=======================
class SoundDevicesManager
//=======================
{
private:
	std::vector<SoundDeviceInfo> m_SoundDevices;

public:
	SoundDevicesManager();
	~SoundDevicesManager();

public:

	void ReEnumerate();

	std::vector<SoundDeviceInfo>::const_iterator begin() const { return m_SoundDevices.begin(); }
	std::vector<SoundDeviceInfo>::const_iterator end() const { return m_SoundDevices.end(); }
	const std::vector<SoundDeviceInfo> & GetDeviceInfos() const { return m_SoundDevices; }

	const SoundDeviceInfo * FindDeviceInfo(UINT type, UINT index) const;
	const SoundDeviceInfo * FindDeviceInfo(UINT id) const { return FindDeviceInfo(SNDDEV_GET_TYPE(id), SNDDEV_GET_NUMBER(id)); }

	ISoundDevice * CreateSoundDevice(UINT type, UINT index);
	ISoundDevice * CreateSoundDevice(UINT id) { return CreateSoundDevice(SNDDEV_GET_TYPE(id), SNDDEV_GET_NUMBER(id)); }

};
