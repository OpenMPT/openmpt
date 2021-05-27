
#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "NativeSoundDevice.h"

#include "openmpt/sounddevice/SoundDevice.hpp"

#ifdef MPT_WITH_NLOHMANNJSON

// https://github.com/nlohmann/json/issues/1204
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant
#endif // MPT_COMPILER_MSVC
#include "mpt/json/json.hpp"
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

#endif // MPT_WITH_NLOHMANNJSON



OPENMPT_NAMESPACE_BEGIN



#ifdef MPT_WITH_NLOHMANNJSON

inline void to_json(nlohmann::json &j, const SampleFormat &val)
{
	j = SampleFormat::ToInt(val);
}
inline void from_json(const nlohmann::json &j, SampleFormat &val)
{
	val = SampleFormat::FromInt(j);
}

namespace SoundDevice
{

	inline void to_json(nlohmann::json &j, const SoundDevice::ChannelMapping &val)
	{
		j = val.ToUString();
	}
	inline void from_json(const nlohmann::json &j, SoundDevice::ChannelMapping &val)
	{
		val = SoundDevice::ChannelMapping::FromString(j);
	}

	inline void to_json(nlohmann::json &j, const SoundDevice::Info::Default &val)
	{
		j = static_cast<int>(val);
	}
	inline void from_json(const nlohmann::json &j, SoundDevice::Info::Default &val)
	{
		val = static_cast<SoundDevice::Info::Default>(static_cast<int>(j));
	}
} // namespace SoundDevice



namespace SoundDevice
{

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::Info::ManagerFlags
		,defaultFor
	)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::Info::Flags
		,usability
		,level
		,compatible
		,api
		,io
		,mixing
		,implementor
	)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::Info
		,type
		,internalID
		,name
		,apiName
		,apiPath
		,default_
		,useNameAsIdentifier
		,managerFlags
		,flags
		,extraData
	)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::AppInfo
		,Name
		,BoostedThreadPriorityXP
		,BoostedThreadMMCSSClassVista
		,BoostedThreadRealtimePosix
		,BoostedThreadNicenessPosix
		,BoostedThreadRtprioPosix
		,MaskDriverCrashes
		,AllowDeferredProcessing
	)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::Settings
		,Latency
		,UpdateInterval
		,Samplerate
		,Channels
		,InputChannels
		,sampleFormat
		,ExclusiveMode
		,BoostThreadPriority
		,KeepDeviceRunning
		,UseHardwareTiming
		,DitherType
		,InputSourceID
	)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::Caps
		,Available
		,CanUpdateInterval
		,CanSampleFormat
		,CanExclusiveMode
		,CanBoostThreadPriority
		,CanKeepDeviceRunning
		,CanUseHardwareTiming
		,CanChannelMapping
		,CanInput
		,HasNamedInputSources
		,CanDriverPanel
		,HasInternalDither
		,ExclusiveModeDescription
		,LatencyMin
		,LatencyMax
		,UpdateIntervalMin
		,UpdateIntervalMax
		,DefaultSettings
	)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::DynamicCaps
		,currentSampleRate
		,supportedSampleRates
		,supportedExclusiveSampleRates
		,supportedSampleFormats
		,supportedExclusiveModeSampleFormats
		,channelNames
		,inputSourceNames
	)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SoundDevice::Statistics
		,InstantaneousLatency
		,LastUpdateInterval
		,text
	)

} // namespace SoundDevice



template <typename Tdst, typename Tsrc>
struct json_cast_impl
{
	Tdst operator () (const Tsrc &src);
};


template <typename Tdst, typename Tsrc>
Tdst json_cast(const Tsrc &src)
{
	return json_cast_impl<Tdst, Tsrc>()(src);
}


template <typename Tsrc>
struct json_cast_impl<nlohmann::json, Tsrc>
{
	nlohmann::json operator () (const Tsrc &src)
	{
		return static_cast<nlohmann::json>(src);
	}
};

template <typename Tdst>
struct json_cast_impl<Tdst, nlohmann::json>
{
	Tdst operator () (const nlohmann::json &src)
	{
		return src.get<Tdst>();
	}
};

template <typename Tsrc>
struct json_cast_impl<std::string, Tsrc>
{
	std::string operator () (const Tsrc &src)
	{
		return json_cast<nlohmann::json>(src).dump(4);
	}
};

template <typename Tdst>
struct json_cast_impl<Tdst, std::string>
{
	Tdst operator () (const std::string &str)
	{
		return json_cast<Tdst>(nlohmann::json::parse(str));
	}
};

#endif // MPT_WITH_NLOHMANNJSON



namespace C {

static_assert(sizeof(OpenMPT_SoundDevice_StreamPosition) % 8 == 0);
inline OpenMPT_SoundDevice_StreamPosition encode(SoundDevice::StreamPosition src) {
	OpenMPT_SoundDevice_StreamPosition dst;
	MemsetZero(dst);
	dst.Frames = src.Frames;
	dst.Seconds = src.Seconds;
	return dst;
}
inline SoundDevice::StreamPosition decode(OpenMPT_SoundDevice_StreamPosition src) {
	SoundDevice::StreamPosition dst;
	dst.Frames = src.Frames;
	dst.Seconds = src.Seconds;
	return dst;
}

static_assert(sizeof(OpenMPT_SoundDevice_TimeInfo) % 8 == 0);
inline OpenMPT_SoundDevice_TimeInfo encode(SoundDevice::TimeInfo src) {
	OpenMPT_SoundDevice_TimeInfo dst;
	MemsetZero(dst);
	dst.SyncPointStreamFrames = src.SyncPointStreamFrames;
	dst.SyncPointSystemTimestamp = src.SyncPointSystemTimestamp;
	dst.Speed = src.Speed;
	dst.RenderStreamPositionBefore = encode(src.RenderStreamPositionBefore);
	dst.RenderStreamPositionAfter = encode(src.RenderStreamPositionAfter);
	dst.Latency = src.Latency;
	return dst;
}
inline SoundDevice::TimeInfo decode(OpenMPT_SoundDevice_TimeInfo src) {
	SoundDevice::TimeInfo dst;
	dst.SyncPointStreamFrames = src.SyncPointStreamFrames;
	dst.SyncPointSystemTimestamp = src.SyncPointSystemTimestamp;
	dst.Speed = src.Speed;
	dst.RenderStreamPositionBefore = decode(src.RenderStreamPositionBefore);
	dst.RenderStreamPositionAfter = decode(src.RenderStreamPositionAfter);
	dst.Latency = src.Latency;
	return dst;
}

static_assert(sizeof(OpenMPT_SoundDevice_Flags) % 8 == 0);
inline OpenMPT_SoundDevice_Flags encode(SoundDevice::Flags src) {
	OpenMPT_SoundDevice_Flags dst;
	MemsetZero(dst);
	dst.WantsClippedOutput = src.WantsClippedOutput;
	return dst;
}
inline SoundDevice::Flags decode(OpenMPT_SoundDevice_Flags src) {
	SoundDevice::Flags dst;
	dst.WantsClippedOutput = src.WantsClippedOutput;
	return dst;
}

static_assert(sizeof(OpenMPT_SoundDevice_BufferFormat) % 8 == 0);
inline OpenMPT_SoundDevice_BufferFormat encode(SoundDevice::BufferFormat src) {
	OpenMPT_SoundDevice_BufferFormat dst;
	MemsetZero(dst);
	dst.Samplerate = src.Samplerate;
	dst.Channels = src.Channels;
	dst.InputChannels = src.InputChannels;
	dst.sampleFormat = SampleFormat::ToInt(src.sampleFormat);
	dst.WantsClippedOutput = src.WantsClippedOutput;
	dst.DitherType = src.DitherType;
	return dst;
}
inline SoundDevice::BufferFormat decode(OpenMPT_SoundDevice_BufferFormat src) {
	SoundDevice::BufferFormat dst;
	dst.Samplerate = src.Samplerate;
	dst.Channels = src.Channels;
	dst.InputChannels = src.InputChannels;
	dst.sampleFormat = SampleFormat::FromInt(src.sampleFormat);
	dst.WantsClippedOutput = src.WantsClippedOutput;
	dst.DitherType = src.DitherType;
	return dst;
}

static_assert(sizeof(OpenMPT_SoundDevice_BufferAttributes) % 8 == 0);
inline OpenMPT_SoundDevice_BufferAttributes encode(SoundDevice::BufferAttributes src) {
	OpenMPT_SoundDevice_BufferAttributes dst;
	MemsetZero(dst);
	dst.Latency = src.Latency;
	dst.UpdateInterval = src.UpdateInterval;
	dst.NumBuffers = src.NumBuffers;
	return dst;
}
inline SoundDevice::BufferAttributes decode(OpenMPT_SoundDevice_BufferAttributes src) {
	SoundDevice::BufferAttributes dst;
	dst.Latency = src.Latency;
	dst.UpdateInterval = src.UpdateInterval;
	dst.NumBuffers = src.NumBuffers;
	return dst;
}

static_assert(sizeof(OpenMPT_SoundDevice_RequestFlags) % 8 == 0);
inline OpenMPT_SoundDevice_RequestFlags encode(FlagSet<SoundDevice::RequestFlags> src) {
	OpenMPT_SoundDevice_RequestFlags dst;
	MemsetZero(dst);
	dst.RequestFlags = src.GetRaw();
	return dst;
}
inline FlagSet<SoundDevice::RequestFlags> decode(OpenMPT_SoundDevice_RequestFlags src) {
	FlagSet<SoundDevice::RequestFlags> dst;
	dst.SetRaw(src.RequestFlags);
	return dst;
}

} // namespace C



OPENMPT_NAMESPACE_END
