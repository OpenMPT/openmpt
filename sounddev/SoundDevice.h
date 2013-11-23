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
#include "../common/misc_util.h"
#include "../soundlib/SampleFormat.h"

#include <map>
#include <vector>


class ISoundDevice;
class IFillAudioBuffer;
class ISoundSource;


////////////////////////////////////////////////////////////////////////////////////
//
// ISoundSource: provides streaming audio data to a device
//


struct SoundDeviceSettings;
struct SoundBufferAttributes;


//====================
class IFillAudioBuffer
//====================
{
public:
	virtual void FillAudioBuffer() = 0;
};


//=========================
class ISoundMessageReceiver
//=========================
{
public:
	virtual void AudioMessage(const std::string &str) = 0;
};


struct SoundTimeInfo
{
	int64 StreamFrames; // can actually be negative (e.g. when starting the stream)
	uint64 SystemTimestamp;
	double Speed;
	SoundTimeInfo()
		: StreamFrames(0)
		, SystemTimestamp(0)
		, Speed(1.0)
	{
		return;
	}
};


//================
class ISoundSource
//================
{
public:
	virtual void FillAudioBufferLocked(IFillAudioBuffer &callback) = 0; // take any locks needed while rendering audio and then call FillAudioBuffer
	virtual void AudioRead(const SoundDeviceSettings &settings, const SoundBufferAttributes &bufferAttributes, SoundTimeInfo timeInfo, std::size_t numFrames, void *buffer) = 0;
	virtual void AudioDone(const SoundDeviceSettings &settings, const SoundBufferAttributes &bufferAttributes, SoundTimeInfo timeInfo, std::size_t numFrames, int64 streamPosition) = 0; // in sample frames
};


////////////////////////////////////////////////////////////////////////////////////
//
// ISoundDevice Interface
//

enum SoundDeviceType
{
	// do not change old values, these get saved to the ini
	SNDDEV_INVALID          =-1,
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

typedef uint8 SoundDeviceIndex;

template<typename T>
bool SoundDeviceIndexIsValid(const T & x)
{
	return 0 <= x && x <= std::numeric_limits<SoundDeviceIndex>::max();
}

//=================
class SoundDeviceID
//=================
{
private:
	SoundDeviceType type;
	SoundDeviceIndex index;
public:
	SoundDeviceID() : type(SNDDEV_WAVEOUT), index(0) {}
	SoundDeviceID(SoundDeviceType type, SoundDeviceIndex index)
		: type(type)
		, index(index)
	{
		return;
	}
	bool IsValid() const
	{
		return (type > SNDDEV_INVALID);
	}
	SoundDeviceType GetType() const { return type; }
	SoundDeviceIndex GetIndex() const { return index; }
	bool operator == (const SoundDeviceID &cmp) const
	{
		return (type == cmp.type) && (index == cmp.index);
	}

	bool operator != (const SoundDeviceID &cmp) const
	{
		return (type != cmp.type) || (index != cmp.index);
	}
	bool operator < (const SoundDeviceID &cmp) const
	{
		return (type < cmp.type) || (type == cmp.type && index < cmp.index);
	}
public:
	// Do not change these. These functions are used to manipulate the value that gets stored in the settings.
	template<typename T>
	static SoundDeviceID FromIdRaw(T id_)
	{
		uint16 id = static_cast<uint16>(id_);
		return SoundDeviceID((SoundDeviceType)((id>>8)&0xff), (id>>0)&0xff);
	}
	uint16 GetIdRaw() const
	{
		return static_cast<uint16>(((int)type<<8) | (index<<0));
	}
};

#define SNDDEV_MAXBUFFERS	       	  256
#define SNDDEV_MAXBUFFERSIZE        (1024*1024)
#define SNDDEV_MINLATENCY_MS        1
#define SNDDEV_MAXLATENCY_MS        500
#define SNDDEV_MINUPDATEINTERVAL_MS 1
#define SNDDEV_MAXUPDATEINTERVAL_MS 200


struct SoundChannelMapping
{

private:

	std::vector<uint32> ChannelToDeviceChannel;

public:

	// Construct default identity mapping
	SoundChannelMapping();

	// Construct mapping from given vector.
	SoundChannelMapping(const std::vector<uint32> &mapping);

	// Construct mapping for #channels with a baseChannel offset.
	static SoundChannelMapping BaseChannel(uint32 channels, uint32 baseChannel);

public:

	bool operator == (const SoundChannelMapping &cmp) const
	{
		return (ChannelToDeviceChannel == cmp.ChannelToDeviceChannel);
	}

	// check that the channel mapping is actually a 1:1 mapping
	bool IsValid(uint32 channels) const;

	// Get the base channel offset. Deprecated because this has no defined semantics for more complex mappings.
	MPT_DEPRECATED uint32 GetBaseChannel() const
	{
		if(ChannelToDeviceChannel.empty())
		{
			return 0;
		}
		return ChannelToDeviceChannel[0];
	}

	// Get the number of required device channels for this mapping. Derived from the maximum mapped-to channel number.
	uint32 GetRequiredDeviceChannels() const
	{
		if(ChannelToDeviceChannel.empty())
		{
			return 0;
		}
		uint32 maxChannel = 0;
		for(uint32 channel = 0; channel < ChannelToDeviceChannel.size(); ++channel)
		{
			if(ChannelToDeviceChannel[channel] > maxChannel)
			{
				maxChannel = ChannelToDeviceChannel[channel];
			}
		}
		return maxChannel + 1;
	}
	
	// Convert OpenMPT channel number to the mapped device channel number.
	uint32 ToDevice(uint32 channel) const
	{
		if(ChannelToDeviceChannel.empty())
		{
			return channel;
		}
		if(channel >= ChannelToDeviceChannel.size())
		{
			return channel;
		}
		return ChannelToDeviceChannel[channel];
	}

	std::string ToString() const;

	static SoundChannelMapping FromString(const std::string &str);

};


struct SoundDeviceSettings
{
	HWND hWnd;
	uint32 LatencyMS;
	uint32 UpdateIntervalMS;
	uint32 Samplerate;
	uint8 Channels;
	SampleFormat sampleFormat;
	bool ExclusiveMode; // Use hardware buffers directly
	bool BoostThreadPriority; // Boost thread priority for glitch-free audio rendering
	bool UseHardwareTiming;
	SoundChannelMapping ChannelMapping;
	SoundDeviceSettings()
		: hWnd(NULL)
		, LatencyMS(100)
		, UpdateIntervalMS(5)
		, Samplerate(48000)
		, Channels(2)
		, sampleFormat(SampleFormatInt16)
		, ExclusiveMode(false)
		, BoostThreadPriority(true)
		, UseHardwareTiming(false)
	{
		return;
	}
	bool operator == (const SoundDeviceSettings &cmp) const
	{
		return true
			&& hWnd == cmp.hWnd
			&& LatencyMS == cmp.LatencyMS
			&& UpdateIntervalMS == cmp.UpdateIntervalMS
			&& Samplerate == cmp.Samplerate
			&& Channels == cmp.Channels
			&& sampleFormat == cmp.sampleFormat
			&& ExclusiveMode == cmp.ExclusiveMode
			&& BoostThreadPriority == cmp.BoostThreadPriority
			&& UseHardwareTiming == cmp.UseHardwareTiming
			&& ChannelMapping == cmp.ChannelMapping
			;
	}
	bool operator != (const SoundDeviceSettings &cmp) const
	{
		return !(*this == cmp);
	}
	std::size_t GetBytesPerFrame() const
	{
		return (sampleFormat.GetBitsPerSample()/8) * Channels;
	}
	std::size_t GetBytesPerSecond() const
	{
		return Samplerate * GetBytesPerFrame();
	}
};


struct SoundDeviceCaps
{
	uint32 currentSampleRate;
	std::vector<uint32> supportedSampleRates;	// Which samplerates are actually supported by the device. Currently only implemented properly for ASIO, DirectSound and PortAudio.
	std::vector<std::wstring> channelNames;
	SoundDeviceCaps()
		: currentSampleRate(0)
	{
		return;
	}
};


class SoundDevicesManager;


struct SoundBufferAttributes
{
	double Latency; // seconds
	double UpdateInterval; // seconds
	int NumBuffers;
	SoundBufferAttributes()
		: Latency(0.0)
		, UpdateInterval(0.0)
		, NumBuffers(0)
	{
		return;
	}
};


//=============================================
class ISoundDevice : protected IFillAudioBuffer
//=============================================
{
	friend class SoundDevicesManager;

private:

	ISoundSource *m_Source;
	ISoundMessageReceiver *m_MessageReceiver;

	const SoundDeviceID m_ID;

	std::wstring m_InternalID;

protected:

	SoundDeviceSettings m_Settings;

private:

	SoundBufferAttributes m_BufferAttributes;

	bool m_IsPlaying;

	Util::MultimediaClock m_Clock;
	SoundTimeInfo m_TimeInfo;

	mutable Util::mutex m_StreamPositionMutex;
	double m_CurrentUpdateInterval;
	int64 m_StreamPositionRenderFrames;
	int64 m_StreamPositionOutputFrames;

protected:

	virtual void InternalFillAudioBuffer() = 0;

	void FillAudioBuffer();

	void SourceFillAudioBufferLocked();
	void SourceAudioPreRead(std::size_t numFrames);
	void SourceAudioRead(void *buffer, std::size_t numFrames);
	void SourceAudioDone(std::size_t numFrames, int32 framesLatency);

	void AudioSendMessage(const std::string &str);

protected:

	bool FillWaveFormatExtensible(WAVEFORMATEXTENSIBLE &WaveFormat);

	const Util::MultimediaClock & Clock() const { return m_Clock; }

	void UpdateBufferAttributes(SoundBufferAttributes attributes);
	void UpdateTimeInfo(SoundTimeInfo timeInfo);

	virtual bool InternalHasTimeInfo() const { return false; }

	virtual bool InternalHasGetStreamPosition() const { return false; }
	virtual int64 InternalGetStreamPositionFrames() const { return 0; }

	virtual bool InternalIsOpen() const = 0;

	virtual bool InternalOpen() = 0;
	virtual bool InternalStart() = 0;
	virtual void InternalStop() = 0;
	virtual bool InternalClose() = 0;

public:

	ISoundDevice(SoundDeviceID id, const std::wstring &internalID);
	virtual ~ISoundDevice();

	void SetSource(ISoundSource *source) { m_Source = source; }
	ISoundSource *GetSource() const { return m_Source; }
	void SetMessageReceiver(ISoundMessageReceiver *receiver) { m_MessageReceiver = receiver; }
	ISoundMessageReceiver *GetMessageReceiver() const { return m_MessageReceiver; }

	SoundDeviceID GetDeviceID() const { return m_ID; }
	SoundDeviceType GetDeviceType() const { return m_ID.GetType(); }
	SoundDeviceIndex GetDeviceIndex() const { return m_ID.GetIndex(); }
	std::wstring GetDeviceInternalID() const { return m_InternalID; }

	virtual SoundDeviceCaps GetDeviceCaps(const std::vector<uint32> &baseSampleRates);

	bool Open(const SoundDeviceSettings &settings);
	bool Close();
	bool Start();
	void Stop();

	bool IsOpen() const { return InternalIsOpen(); }
	bool IsPlaying() const { return m_IsPlaying; }

	SoundDeviceSettings GetSettings() const { return m_Settings; }
	SampleFormat GetActualSampleFormat() const { return IsOpen() ? m_Settings.sampleFormat : SampleFormatInvalid; }

	SoundBufferAttributes GetBufferAttributes() const { return m_BufferAttributes; }
	SoundTimeInfo GetTimeInfo() const { return m_TimeInfo; }

	// Informational only, do not use for timing.
	// Use GetStreamPositionFrames() for timing
	virtual double GetCurrentLatency() const { return m_BufferAttributes.Latency; }
	double GetCurrentUpdateInterval() const;

	int64 GetStreamPositionFrames() const;

	virtual bool OpenDriverSettings() { return false; };
};


struct SoundDeviceInfo
{
	SoundDeviceID id;
	std::wstring name;
	std::wstring apiName;
	std::wstring internalID;
	bool isDefault;
	SoundDeviceInfo() : id(SNDDEV_INVALID, 0), isDefault(false) { }
	SoundDeviceInfo(SoundDeviceID id, const std::wstring &name, const std::wstring &apiName, const std::wstring &internalID = std::wstring())
		: id(id)
		, name(name)
		, apiName(apiName)
		, internalID(internalID)
		, isDefault(false)
	{
		return;
	}
	bool IsValid() const
	{
		return id.IsValid();
	}
};


//=======================
class SoundDevicesManager
//=======================
{
private:
	std::vector<SoundDeviceInfo> m_SoundDevices;
	std::map<SoundDeviceID, SoundDeviceCaps> m_DeviceCaps;

public:
	SoundDevicesManager();
	~SoundDevicesManager();

public:

	void ReEnumerate();

	std::vector<SoundDeviceInfo>::const_iterator begin() const { return m_SoundDevices.begin(); }
	std::vector<SoundDeviceInfo>::const_iterator end() const { return m_SoundDevices.end(); }
	const std::vector<SoundDeviceInfo> & GetDeviceInfos() const { return m_SoundDevices; }

	SoundDeviceInfo FindDeviceInfo(SoundDeviceID id) const;

	bool OpenDriverSettings(SoundDeviceID id, ISoundMessageReceiver *messageReceiver = nullptr, ISoundDevice *currentSoundDevice = nullptr);

	SoundDeviceCaps GetDeviceCaps(SoundDeviceID id, const std::vector<uint32> &baseSampleRates, ISoundMessageReceiver *messageReceiver = nullptr, ISoundDevice *currentSoundDevice = nullptr, bool update = false);

	ISoundDevice * CreateSoundDevice(SoundDeviceID id);

};
