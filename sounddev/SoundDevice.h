/*
 * SoundDevice.h
 * -------------
 * Purpose: Sound device interfaces.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/FlagSet.h"
#include "../common/mptOS.h"
#include "../soundlib/SampleFormat.h"

#include <map>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


//====================
class IMessageReceiver
//====================
{
public:
	virtual void SoundDeviceMessage(LogLevel level, const mpt::ustring &str) = 0;
};


struct StreamPosition
{
	int64 Frames; // relative to Start()
	double Seconds; // relative to Start()
	StreamPosition() : Frames(0), Seconds(0.0) { }
	StreamPosition(int64 frames, double seconds) : Frames(frames), Seconds(seconds) { }
};


struct TimeInfo
{
	
	int64 SyncPointStreamFrames;
	uint64 SyncPointSystemTimestamp;
	double Speed;

	SoundDevice::StreamPosition RenderStreamPositionBefore;
	SoundDevice::StreamPosition RenderStreamPositionAfter;
	// int64 chunkSize = After - Before
	
	TimeInfo()
		: SyncPointStreamFrames(0)
		, SyncPointSystemTimestamp(0)
		, Speed(1.0)
	{
		return;
	}

};


struct Settings;
struct Flags;
struct BufferFormat;
struct BufferAttributes;


//===========
class ISource
//===========
{
public:
	// main thread
	virtual uint64 SoundSourceGetReferenceClockNowNanoseconds() const = 0; // timeGetTime()*1000000 on Windows
	virtual void SoundSourcePreStartCallback() = 0;
	virtual void SoundSourcePostStopCallback() = 0;
	virtual bool SoundSourceIsLockedByCurrentThread() const = 0;
	// audio thread
	virtual void SoundSourceLock() = 0;
	virtual uint64 SoundSourceLockedGetReferenceClockNowNanoseconds() const = 0; // timeGetTime()*1000000 on Windows
	virtual void SoundSourceLockedRead(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo, std::size_t numFrames, void *buffer, const void *inputBuffer) = 0;
	virtual void SoundSourceLockedDone(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo) = 0;
	virtual void SoundSourceUnlock() = 0;
public:
	class Guard
	{
	private:
		ISource &m_Source;
	public:
		Guard(ISource &source)
			: m_Source(source)
		{
			m_Source.SoundSourceLock();
		}
		~Guard()
		{
			m_Source.SoundSourceUnlock();
		}
	};
};


static const MPT_UCHAR_TYPE TypeWAVEOUT          [] = MPT_ULITERAL("WaveOut");
static const MPT_UCHAR_TYPE TypeDSOUND           [] = MPT_ULITERAL("DirectSound");
static const MPT_UCHAR_TYPE TypeASIO             [] = MPT_ULITERAL("ASIO");
static const MPT_UCHAR_TYPE TypePORTAUDIO_WASAPI [] = MPT_ULITERAL("WASAPI");
static const MPT_UCHAR_TYPE TypePORTAUDIO_WDMKS  [] = MPT_ULITERAL("WDM-KS");
static const MPT_UCHAR_TYPE TypePORTAUDIO_WMME   [] = MPT_ULITERAL("MME");
static const MPT_UCHAR_TYPE TypePORTAUDIO_DS     [] = MPT_ULITERAL("DS");

typedef mpt::ustring Type;


typedef mpt::ustring Identifier;

SoundDevice::Type ParseType(const SoundDevice::Identifier &identifier);

struct Info
{
	SoundDevice::Type type;
	mpt::ustring internalID;
	mpt::ustring name; // user visible (and configuration key if useNameAsIdentifier)
	mpt::ustring apiName; // user visible
	std::vector<mpt::ustring> apiPath; // i.e. Wine-support, PortAudio
	bool isDefault;
	bool useNameAsIdentifier;
	std::map<mpt::ustring, mpt::ustring> extraData; // user visible (hidden by default)
	Info() : isDefault(false), useNameAsIdentifier(false) { }
	bool IsValid() const
	{
		return !type.empty() && !internalID.empty();
	}
	SoundDevice::Identifier GetIdentifier() const
	{
		if(!IsValid())
		{
			return mpt::ustring();
		}
		mpt::ustring result = mpt::ustring();
		result += type;
		result += MPT_USTRING("_");
		if(useNameAsIdentifier)
		{
			// UTF8-encode the name and convert the utf8 to hex.
			// This ensures that no special characters are contained in the configuration key.
			std::string utf8String = mpt::ToCharset(mpt::CharsetUTF8, name);
			mpt::ustring hexString = Util::BinToHex(std::vector<char>(utf8String.begin(), utf8String.end()));
			result += hexString;
		} else
		{
			result += internalID; // safe to not contain special characters
		}
		return result;
	}
};


struct ChannelMapping
{

private:

	std::vector<int32> ChannelToDeviceChannel;

public:

	static const int32 MaxDeviceChannel = 32000;

public:

	// Construct default identity mapping
	ChannelMapping(uint32 numHostChannels = 2);

	// Construct mapping from given vector.
	// Silently fall back to identity mapping if mapping is invalid.
	ChannelMapping(const std::vector<int32> &mapping);

	// Construct mapping for #channels with a baseChannel offset.
	static ChannelMapping BaseChannel(uint32 channels, int32 baseChannel);

private:

	// check that the channel mapping is actually a 1:1 mapping
	static bool IsValid(const std::vector<int32> &mapping);

public:

	operator int () const
	{
		return GetNumHostChannels();
	}

	ChannelMapping & operator = (int channels)
	{
		return (*this = ChannelMapping(channels));
	}

	bool operator == (const SoundDevice::ChannelMapping &cmp) const
	{
		return (ChannelToDeviceChannel == cmp.ChannelToDeviceChannel);
	}
	
	uint32 GetNumHostChannels() const
	{
		return static_cast<uint32>(ChannelToDeviceChannel.size());
	}
	
	// Get the number of required device channels for this mapping. Derived from the maximum mapped-to channel number.
	int32 GetRequiredDeviceChannels() const
	{
		if(ChannelToDeviceChannel.empty())
		{
			return 0;
		}
		int32 maxChannel = 0;
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
	int32 ToDevice(uint32 channel) const
	{
		if(channel >= ChannelToDeviceChannel.size())
		{
			return channel;
		}
		return ChannelToDeviceChannel[channel];
	}

	mpt::ustring ToString() const;

	static SoundDevice::ChannelMapping FromString(const mpt::ustring &str);

};


struct SysInfo
{
	mpt::Windows::Version WindowsVersion;
	bool IsWine;
	bool WineHostIsLinux;
	mpt::Wine::Version WineVersion;
public:
	static SysInfo Current();
private:
	SysInfo();
};


struct AppInfo
{
	mpt::ustring Name;
	uintptr_t UIHandle; // HWND on Windows
	AppInfo()
		: UIHandle(0)
	{
		return;
	}
	AppInfo &SetName(const mpt::ustring &name) { Name = name; return *this; }
	mpt::ustring GetName() const { return Name; }
#if MPT_OS_WINDOWS
	AppInfo &SetHWND(HWND hwnd) { UIHandle = reinterpret_cast<uintptr_t>(hwnd); return *this; }
	HWND GetHWND() const { return reinterpret_cast<HWND>(UIHandle); }
#endif // MPT_OS_WINDOWS
};


struct Settings
{
	double Latency; // seconds
	double UpdateInterval; // seconds
	uint32 Samplerate;
	SoundDevice::ChannelMapping Channels;
	uint8 InputChannels;
	SampleFormat sampleFormat;
	bool ExclusiveMode; // Use hardware buffers directly
	bool BoostThreadPriority; // Boost thread priority for glitch-free audio rendering
	bool KeepDeviceRunning;
	bool UseHardwareTiming;
	int DitherType;
	uint32 InputSourceID;
	Settings()
		: Latency(0.1)
		, UpdateInterval(0.005)
		, Samplerate(48000)
		, Channels(2)
		, InputChannels(0)
		, sampleFormat(SampleFormatFloat32)
		, ExclusiveMode(false)
		, BoostThreadPriority(true)
		, KeepDeviceRunning(true)
		, UseHardwareTiming(false)
		, DitherType(1)
		, InputSourceID(0)
	{
		return;
	}
	bool operator == (const SoundDevice::Settings &cmp) const
	{
		return true
			&& Util::Round<int64>(Latency * 1000000000.0) == Util::Round<int64>(cmp.Latency * 1000000000.0) // compare in nanoseconds
			&& Util::Round<int64>(UpdateInterval * 1000000000.0) == Util::Round<int64>(cmp.UpdateInterval * 1000000000.0) // compare in nanoseconds
			&& Samplerate == cmp.Samplerate
			&& Channels == cmp.Channels
			&& InputChannels == cmp.InputChannels
			&& sampleFormat == cmp.sampleFormat
			&& ExclusiveMode == cmp.ExclusiveMode
			&& BoostThreadPriority == cmp.BoostThreadPriority
			&& KeepDeviceRunning == cmp.KeepDeviceRunning
			&& UseHardwareTiming == cmp.UseHardwareTiming
			&& DitherType == cmp.DitherType
			&& InputSourceID == cmp.InputSourceID
			;
	}
	bool operator != (const SoundDevice::Settings &cmp) const
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
	uint32 GetTotalChannels() const
	{
		return InputChannels + Channels;
	}
};


struct Flags
{
	// Windows since Vista has a limiter/compressor in the audio path that kicks
	// in as soon as there are samples > 0dBFs (i.e. the absolute float value >
	// 1.0). This happens for all APIs that get processed through the system-
	// wide audio engine. It does not happen for exclusive mode WASAPI streams
	// or direct WaveRT (labeled WDM-KS in PortAudio) streams. As there is no
	// known way to disable this annoying behavior, avoid unclipped samples on
	// affected windows versions and clip them ourselves before handing them to
	// the APIs.
	bool NeedsClippedFloat;
	Flags()
		: NeedsClippedFloat(false)
	{
		return;
	}
};


struct Caps
{
	bool Available;
	bool CanUpdateInterval;
	bool CanSampleFormat;
	bool CanExclusiveMode;
	bool CanBoostThreadPriority;
	bool CanKeepDeviceRunning;
	bool CanUseHardwareTiming;
	bool CanChannelMapping;
	bool CanInput;
	bool HasNamedInputSources;
	bool CanDriverPanel;
	bool HasInternalDither;
	mpt::ustring ExclusiveModeDescription;
	double LatencyMin;
	double LatencyMax;
	double UpdateIntervalMin;
	double UpdateIntervalMax;
	SoundDevice::Settings DefaultSettings;
	Caps()
		: Available(false)
		, CanUpdateInterval(true)
		, CanSampleFormat(true)
		, CanExclusiveMode(false)
		, CanBoostThreadPriority(true)
		, CanKeepDeviceRunning(false)
		, CanUseHardwareTiming(false)
		, CanChannelMapping(false)
		, CanInput(false)
		, HasNamedInputSources(false)
		, CanDriverPanel(false)
		, HasInternalDither(false)
		, ExclusiveModeDescription(MPT_USTRING("Use device exclusively"))
		, LatencyMin(0.002) // 2ms
		, LatencyMax(0.5) // 500ms
		, UpdateIntervalMin(0.001) // 1ms
		, UpdateIntervalMax(0.2) // 200ms
	{
		return;
	}
};


struct DynamicCaps
{
	uint32 currentSampleRate;
	std::vector<uint32> supportedSampleRates;
	std::vector<uint32> supportedExclusiveSampleRates;
	std::vector<mpt::ustring> channelNames;
	std::vector<std::pair<uint32, mpt::ustring> > inputSourceNames;
	DynamicCaps()
		: currentSampleRate(0)
	{
		return;
	}
};


struct BufferFormat
{
	uint32 Samplerate;
	uint32 Channels;
	uint8 InputChannels;
	SampleFormat sampleFormat;
	bool NeedsClippedFloat;
	int DitherType;
};


struct BufferAttributes
{
	double Latency; // seconds
	double UpdateInterval; // seconds
	int NumBuffers;
	BufferAttributes()
		: Latency(0.0)
		, UpdateInterval(0.0)
		, NumBuffers(0)
	{
		return;
	}
};


enum RequestFlags
{
	RequestFlagClose   = 1<<0,
	RequestFlagReset   = 1<<1,
	RequestFlagRestart = 1<<2,
};
} // namespace SoundDevice
template <> struct enum_traits<SoundDevice::RequestFlags> { typedef uint32 store_type; };
namespace SoundDevice {
MPT_DECLARE_ENUM(RequestFlags)


struct Statistics
{
	double InstantaneousLatency;
	double LastUpdateInterval;
	mpt::ustring text;
	Statistics()
		: InstantaneousLatency(0.0)
		, LastUpdateInterval(0.0)
	{
		return;
	}
};


//=========
class IBase
//=========
{

protected:

	IBase() { }

public:

	virtual ~IBase() { }

public:

	virtual void SetSource(SoundDevice::ISource *source) = 0;
	virtual void SetMessageReceiver(SoundDevice::IMessageReceiver *receiver) = 0;

	virtual SoundDevice::Info GetDeviceInfo() const = 0;

	virtual SoundDevice::Caps GetDeviceCaps() const = 0;
	virtual SoundDevice::DynamicCaps GetDeviceDynamicCaps(const std::vector<uint32> &baseSampleRates) = 0;

	virtual bool Init(const SoundDevice::AppInfo &appInfo) = 0;
	virtual bool Open(const SoundDevice::Settings &settings) = 0;
	virtual bool Close() = 0;
	virtual bool Start() = 0;
	virtual void Stop(bool force = false) = 0;

	virtual FlagSet<RequestFlags> GetRequestFlags() const = 0;

	virtual bool IsInited() const = 0;
	virtual bool IsOpen() const = 0;
	virtual bool IsAvailable() const = 0;
	virtual bool IsPlaying() const = 0;

	virtual bool OnIdle() = 0; // return true if any work has been done

	virtual SoundDevice::Settings GetSettings() const = 0;
	virtual SampleFormat GetActualSampleFormat() const = 0;
	virtual SoundDevice::BufferAttributes GetEffectiveBufferAttributes() const = 0;

	virtual SoundDevice::TimeInfo GetTimeInfo() const = 0;
	virtual SoundDevice::StreamPosition GetStreamPosition() const = 0;

	// Debugging aids in case of a crash
	virtual bool DebugIsFragileDevice() const = 0;
	virtual bool DebugInRealtimeCallback() const = 0;

	// Informational only, do not use for timing.
	// Use GetStreamPositionFrames() for timing
	virtual SoundDevice::Statistics GetStatistics() const = 0;

	virtual bool OpenDriverSettings() = 0;

};


namespace Legacy
{
typedef uint16 ID;
static const SoundDevice::Legacy::ID MaskType = 0xff00;
static const SoundDevice::Legacy::ID MaskIndex = 0x00ff;
static const SoundDevice::Legacy::ID TypeWAVEOUT          = 0;
static const SoundDevice::Legacy::ID TypeDSOUND           = 1;
static const SoundDevice::Legacy::ID TypeASIO             = 2;
static const SoundDevice::Legacy::ID TypePORTAUDIO_WASAPI = 3;
static const SoundDevice::Legacy::ID TypePORTAUDIO_WDMKS  = 4;
static const SoundDevice::Legacy::ID TypePORTAUDIO_WMME   = 5;
static const SoundDevice::Legacy::ID TypePORTAUDIO_DS     = 6;
#ifdef MPT_WITH_DSOUND
mpt::ustring GetDirectSoundDefaultDeviceIdentifierPre_1_25_00_04();
mpt::ustring GetDirectSoundDefaultDeviceIdentifier_1_25_00_04();
#endif // MPT_WITH_DSOUND
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
