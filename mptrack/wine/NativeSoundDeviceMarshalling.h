
#pragma once

#include "NativeSoundDevice.h"

#include "../../sounddev/SoundDevice.h"

#include "picojson/picojson.h"

OPENMPT_NAMESPACE_BEGIN

namespace C {

namespace JSON {

	template <typename T> struct Marshal;

	template <typename Tsrc>
	inline picojson::value encode(const Tsrc & src)
	{
		return picojson::value(JSON::Marshal<Tsrc>().enc(src));
	}

	template <typename Tdst>
	inline Tdst decode(picojson::value val)
	{
		return JSON::Marshal<Tdst>().dec(val);
	}

	template <typename Tdst>
	inline void decode(Tdst & dst, picojson::value val)
	{
		dst = decode<Tdst>(val);
	}

	template <> struct Marshal<bool>
	{
		picojson::value enc(bool src) const
		{
			return picojson::value(src);
		}
		bool dec(picojson::value val) const
		{
			return val.get<bool>();
		}
	};

	template <> struct Marshal<double>
	{
		picojson::value enc(double src) const
		{
			return picojson::value(src);
		}
		double dec(picojson::value val) const
		{
			return val.get<double>();
		}
	};

	template <> struct Marshal<int64>
	{
		picojson::value enc(int64_t src) const
		{
			return picojson::value(src);
		}
		int64_t dec(picojson::value val) const
		{
			return val.get<int64_t>();
		}
	};

	template <> struct Marshal<int>
	{
		picojson::value enc(int src) const
		{
			return picojson::value(static_cast<int64_t>(src));
		}
		int dec(picojson::value val) const
		{
			return static_cast<int>(val.get<int64_t>());
		}
	};

	template <> struct Marshal<uint8>
	{
		picojson::value enc(uint8 src) const
		{
			return picojson::value(static_cast<int64_t>(src));
		}
		uint8 dec(picojson::value val) const
		{
			return static_cast<uint8>(val.get<int64_t>());
		}
	};

	template <> struct Marshal<uint32>
	{
		picojson::value enc(uint32 src) const
		{
			return picojson::value(static_cast<int64_t>(src));
		}
		uint32 dec(picojson::value val) const
		{
			return static_cast<uint32>(val.get<int64_t>());
		}
	};

	template <> struct Marshal<mpt::ustring>
	{
		picojson::value enc(mpt::ustring src) const
		{
			return picojson::value(mpt::ToCharset(mpt::CharsetUTF8, src));
		}
		mpt::ustring dec(picojson::value val) const
		{
			return mpt::ToUnicode(mpt::CharsetUTF8, val.get<std::string>());
		}
	};

	template <typename Tval> struct Marshal<std::vector<Tval> >
	{
		picojson::value enc(std::vector<Tval> src) const
		{
			picojson::value result = picojson::value(picojson::array());
			for(typename std::vector<Tval>::const_iterator it = src.cbegin(); it != src.cend(); ++it)
			{
				result.get<picojson::array>().push_back(JSON::encode(*it));
			}
			return result;
		}
		std::vector<Tval> dec(picojson::value val) const
		{
			std::vector<Tval> result;
			for(picojson::array::const_iterator it = val.get<picojson::array>().cbegin(); it != val.get<picojson::array>().cend(); ++it)
			{
				result.push_back(JSON::decode<Tval>(*it));
			}
			return result;
		}
	};

	template <typename Tval> struct Marshal<std::map<mpt::ustring, Tval> >
	{
		picojson::value enc(std::map<mpt::ustring, Tval> src) const
		{
			picojson::value result = picojson::value(picojson::object());
			for(typename std::map<mpt::ustring, Tval>::const_iterator it = src.cbegin(); it != src.cend(); ++it)
			{
				result.get<picojson::object>().insert(std::make_pair(mpt::ToCharset(mpt::CharsetUTF8, (*it).first), JSON::encode((*it).second)));
			}
			return result;
		}
		std::map<mpt::ustring, Tval> dec(picojson::value val) const
		{
			std::map<mpt::ustring, Tval> result;
			for(picojson::object::const_iterator it = val.get<picojson::object>().cbegin(); it != val.get<picojson::object>().cend(); ++it)
			{
				result[mpt::ToUnicode(mpt::CharsetUTF8, (*it).first)] = JSON::decode<Tval>((*it).second);
			}
			return result;
		}
	};

	template <typename Ta, typename Tb> struct Marshal<std::pair<Ta, Tb> >
	{
		picojson::value enc(std::pair<Ta, Tb> src) const
		{
			picojson::value result = picojson::value(picojson::object());
			result.get<picojson::object>()["first"] = JSON::encode(src.first);
			result.get<picojson::object>()["second"] = JSON::encode(src.second);
			return result;
		}
		std::pair<Ta, Tb> dec(picojson::value val) const
		{
			std::pair<Ta, Tb> result;
			JSON::decode(result.first, val.get<picojson::object>()["first"]);
			JSON::decode(result.second, val.get<picojson::object>()["second"]);
			return result;
		}
	};

	template <> struct Marshal<SoundDevice::Info>
	{
		picojson::value enc(SoundDevice::Info src) const
		{
			picojson::value result = picojson::value(picojson::object());
			result.get<picojson::object>()["type"] = JSON::encode(src.type);
			result.get<picojson::object>()["internalID"] = JSON::encode(src.internalID);
			result.get<picojson::object>()["name"] = JSON::encode(src.name);
			result.get<picojson::object>()["apiName"] = JSON::encode(src.apiName);
			result.get<picojson::object>()["apiPath"] = JSON::encode(src.apiPath);
			result.get<picojson::object>()["isDefault"] = JSON::encode(src.isDefault);
			result.get<picojson::object>()["useNameAsIdentifier"] = JSON::encode(src.useNameAsIdentifier);
			result.get<picojson::object>()["extraData"] = JSON::encode(src.extraData);
			return result;
		}
		SoundDevice::Info dec(picojson::value val) const
		{
			SoundDevice::Info result;
			JSON::decode(result.type, val.get<picojson::object>()["type"]);
			JSON::decode(result.internalID, val.get<picojson::object>()["internalID"]);
			JSON::decode(result.name, val.get<picojson::object>()["name"]);
			JSON::decode(result.apiName, val.get<picojson::object>()["apiName"]);
			JSON::decode(result.apiPath, val.get<picojson::object>()["apiPath"]);
			JSON::decode(result.isDefault, val.get<picojson::object>()["isDefault"]);
			JSON::decode(result.useNameAsIdentifier, val.get<picojson::object>()["useNameAsIdentifier"]);
			JSON::decode(result.extraData, val.get<picojson::object>()["extraData"]);
			return result;
		}
	};

	template <> struct Marshal<SoundDevice::AppInfo>
	{
		picojson::value enc(SoundDevice::AppInfo src) const
		{
			picojson::value result = picojson::value(picojson::object());
			result.get<picojson::object>()["Name"] = JSON::encode(src.Name);
			result.get<picojson::object>()["BoostedThreadPriorityXP"] = JSON::encode(src.BoostedThreadPriorityXP);
			result.get<picojson::object>()["BoostedThreadMMCSSClassVista"] = JSON::encode(src.BoostedThreadMMCSSClassVista);
			result.get<picojson::object>()["BoostedThreadRealtimePosix"] = JSON::encode(src.BoostedThreadRealtimePosix);
			result.get<picojson::object>()["BoostedThreadNicenessPosix"] = JSON::encode(src.BoostedThreadNicenessPosix);
			result.get<picojson::object>()["BoostedThreadRtprioPosix"] = JSON::encode(src.BoostedThreadRtprioPosix);
			return result;
		}
		SoundDevice::AppInfo dec(picojson::value val) const
		{
			SoundDevice::AppInfo result;
			JSON::decode(result.Name, val.get<picojson::object>()["Name"]);
			JSON::decode(result.BoostedThreadPriorityXP, val.get<picojson::object>()["BoostedThreadPriorityXP"]);
			JSON::decode(result.BoostedThreadMMCSSClassVista, val.get<picojson::object>()["BoostedThreadMMCSSClassVista"]);
			JSON::decode(result.BoostedThreadRealtimePosix, val.get<picojson::object>()["BoostedThreadRealtimePosix"]);
			JSON::decode(result.BoostedThreadNicenessPosix, val.get<picojson::object>()["BoostedThreadNicenessPosix"]);
			JSON::decode(result.BoostedThreadRtprioPosix, val.get<picojson::object>()["BoostedThreadRtprioPosix"]);
			return result;
		}
	};

	template <> struct Marshal<SoundDevice::ChannelMapping>
	{
		picojson::value enc(SoundDevice::ChannelMapping src) const
		{
			mpt::ustring tmp = src.ToUString();
			return JSON::encode(tmp);
		}
		SoundDevice::ChannelMapping dec(picojson::value val) const
		{
			mpt::ustring tmp;
			JSON::decode(tmp, val);
			return SoundDevice::ChannelMapping::FromString(tmp);
		}
	};

	template <> struct Marshal<SampleFormat>
	{
		picojson::value enc(SampleFormat src) const
		{
			int tmp = src;
			return JSON::encode(tmp);
		}
		SampleFormat dec(picojson::value val) const
		{
			int tmp = 0;
			JSON::decode(tmp, val);
			return tmp;
		}
	};

	template <> struct Marshal<SoundDevice::Settings>
	{
		picojson::value enc(SoundDevice::Settings src) const
		{
			picojson::value result = picojson::value(picojson::object());
			result.get<picojson::object>()["Latency"] = JSON::encode(src.Latency);
			result.get<picojson::object>()["UpdateInterval"] = JSON::encode(src.UpdateInterval);
			result.get<picojson::object>()["Samplerate"] = JSON::encode(src.Samplerate);
			result.get<picojson::object>()["Channels"] = JSON::encode(src.Channels);
			result.get<picojson::object>()["InputChannels"] = JSON::encode(src.InputChannels);
			result.get<picojson::object>()["sampleFormat"] = JSON::encode(src.sampleFormat);
			result.get<picojson::object>()["ExclusiveMode"] = JSON::encode(src.ExclusiveMode);
			result.get<picojson::object>()["BoostThreadPriority"] = JSON::encode(src.BoostThreadPriority);
			result.get<picojson::object>()["KeepDeviceRunning"] = JSON::encode(src.KeepDeviceRunning);
			result.get<picojson::object>()["UseHardwareTiming"] = JSON::encode(src.UseHardwareTiming);
			result.get<picojson::object>()["DitherType"] = JSON::encode(src.DitherType);
			result.get<picojson::object>()["InputSourceID"] = JSON::encode(src.InputSourceID);
			return result;
		}
		SoundDevice::Settings dec(picojson::value val) const
		{
			SoundDevice::Settings result;
			JSON::decode(result.Latency, val.get<picojson::object>()["Latency"]);
			JSON::decode(result.UpdateInterval, val.get<picojson::object>()["UpdateInterval"]);
			JSON::decode(result.Samplerate, val.get<picojson::object>()["Samplerate"]);
			JSON::decode(result.Channels, val.get<picojson::object>()["Channels"]);
			JSON::decode(result.InputChannels, val.get<picojson::object>()["InputChannels"]);
			JSON::decode(result.sampleFormat, val.get<picojson::object>()["sampleFormat"]);
			JSON::decode(result.ExclusiveMode, val.get<picojson::object>()["ExclusiveMode"]);
			JSON::decode(result.BoostThreadPriority, val.get<picojson::object>()["BoostThreadPriority"]);
			JSON::decode(result.KeepDeviceRunning, val.get<picojson::object>()["KeepDeviceRunning"]);
			JSON::decode(result.UseHardwareTiming, val.get<picojson::object>()["UseHardwareTiming"]);
			JSON::decode(result.DitherType, val.get<picojson::object>()["DitherType"]);
			JSON::decode(result.InputSourceID, val.get<picojson::object>()["InputSourceID"]);
			return result;
		}
	};

	template <> struct Marshal<SoundDevice::Caps>
	{
		picojson::value enc(SoundDevice::Caps src) const
		{
			picojson::value result = picojson::value(picojson::object());
			result.get<picojson::object>()["Available"] = JSON::encode(src.Available);
			result.get<picojson::object>()["CanUpdateInterval"] = JSON::encode(src.CanUpdateInterval);
			result.get<picojson::object>()["CanSampleFormat"] = JSON::encode(src.CanSampleFormat);
			result.get<picojson::object>()["CanExclusiveMode"] = JSON::encode(src.CanExclusiveMode);
			result.get<picojson::object>()["CanBoostThreadPriority"] = JSON::encode(src.CanBoostThreadPriority);
			result.get<picojson::object>()["CanKeepDeviceRunning"] = JSON::encode(src.CanKeepDeviceRunning);
			result.get<picojson::object>()["CanUseHardwareTiming"] = JSON::encode(src.CanUseHardwareTiming);
			result.get<picojson::object>()["CanChannelMapping"] = JSON::encode(src.CanChannelMapping);
			result.get<picojson::object>()["CanInput"] = JSON::encode(src.CanInput);
			result.get<picojson::object>()["HasNamedInputSources"] = JSON::encode(src.HasNamedInputSources);
			result.get<picojson::object>()["CanDriverPanel"] = JSON::encode(src.CanDriverPanel);
			result.get<picojson::object>()["HasInternalDither"] = JSON::encode(src.HasInternalDither);
			result.get<picojson::object>()["ExclusiveModeDescription"] = JSON::encode(src.ExclusiveModeDescription);
			result.get<picojson::object>()["LatencyMin"] = JSON::encode(src.LatencyMin);
			result.get<picojson::object>()["LatencyMax"] = JSON::encode(src.LatencyMax);
			result.get<picojson::object>()["UpdateIntervalMin"] = JSON::encode(src.UpdateIntervalMin);
			result.get<picojson::object>()["UpdateIntervalMax"] = JSON::encode(src.UpdateIntervalMax);
			result.get<picojson::object>()["DefaultSettings"] = JSON::encode(src.DefaultSettings);
			return result;
		}
		SoundDevice::Caps dec(picojson::value val) const
		{
			SoundDevice::Caps result;
			JSON::decode(result.Available, val.get<picojson::object>()["Available"]);
			JSON::decode(result.CanUpdateInterval, val.get<picojson::object>()["CanUpdateInterval"]);
			JSON::decode(result.CanSampleFormat, val.get<picojson::object>()["CanSampleFormat"]);
			JSON::decode(result.CanExclusiveMode, val.get<picojson::object>()["CanExclusiveMode"]);
			JSON::decode(result.CanBoostThreadPriority, val.get<picojson::object>()["CanBoostThreadPriority"]);
			JSON::decode(result.CanKeepDeviceRunning, val.get<picojson::object>()["CanKeepDeviceRunning"]);
			JSON::decode(result.CanUseHardwareTiming, val.get<picojson::object>()["CanUseHardwareTiming"]);
			JSON::decode(result.CanChannelMapping, val.get<picojson::object>()["CanChannelMapping"]);
			JSON::decode(result.CanInput, val.get<picojson::object>()["CanInput"]);
			JSON::decode(result.HasNamedInputSources, val.get<picojson::object>()["HasNamedInputSources"]);
			JSON::decode(result.CanDriverPanel, val.get<picojson::object>()["CanDriverPanel"]);
			JSON::decode(result.HasInternalDither, val.get<picojson::object>()["HasInternalDither"]);
			JSON::decode(result.ExclusiveModeDescription, val.get<picojson::object>()["ExclusiveModeDescription"]);
			JSON::decode(result.LatencyMin, val.get<picojson::object>()["LatencyMin"]);
			JSON::decode(result.LatencyMax, val.get<picojson::object>()["LatencyMax"]);
			JSON::decode(result.UpdateIntervalMin, val.get<picojson::object>()["UpdateIntervalMin"]);
			JSON::decode(result.UpdateIntervalMax, val.get<picojson::object>()["UpdateIntervalMax"]);
			JSON::decode(result.DefaultSettings, val.get<picojson::object>()["DefaultSettings"]);
			return result;
		}
	};

	template <> struct Marshal<SoundDevice::DynamicCaps>
	{
		picojson::value enc(SoundDevice::DynamicCaps src) const
		{
			picojson::value result = picojson::value(picojson::object());
			result.get<picojson::object>()["currentSampleRate"] = JSON::encode(src.currentSampleRate);
			result.get<picojson::object>()["supportedSampleRates"] = JSON::encode(src.supportedSampleRates);
			result.get<picojson::object>()["supportedExclusiveSampleRates"] = JSON::encode(src.supportedExclusiveSampleRates);
			result.get<picojson::object>()["channelNames"] = JSON::encode(src.channelNames);
			result.get<picojson::object>()["inputSourceNames"] = JSON::encode(src.inputSourceNames);
			return result;
		}
		SoundDevice::DynamicCaps dec(picojson::value val) const
		{
			SoundDevice::DynamicCaps result;
			JSON::decode(result.currentSampleRate, val.get<picojson::object>()["currentSampleRate"]);
			JSON::decode(result.supportedSampleRates, val.get<picojson::object>()["supportedSampleRates"]);
			JSON::decode(result.supportedExclusiveSampleRates, val.get<picojson::object>()["supportedExclusiveSampleRates"]);
			JSON::decode(result.channelNames, val.get<picojson::object>()["channelNames"]);
			JSON::decode(result.inputSourceNames, val.get<picojson::object>()["inputSourceNames"]);
			return result;
		}
	};

	template <> struct Marshal<SoundDevice::Statistics>
	{
		picojson::value enc(SoundDevice::Statistics src) const
		{
			picojson::value result = picojson::value(picojson::object());
			result.get<picojson::object>()["InstantaneousLatency"] = JSON::encode(src.InstantaneousLatency);
			result.get<picojson::object>()["LastUpdateInterval"] = JSON::encode(src.LastUpdateInterval);
			result.get<picojson::object>()["text"] = JSON::encode(src.text);
			return result;
		}
		SoundDevice::Statistics dec(picojson::value val) const
		{
			SoundDevice::Statistics result;
			JSON::decode(result.InstantaneousLatency, val.get<picojson::object>()["InstantaneousLatency"]);
			JSON::decode(result.LastUpdateInterval, val.get<picojson::object>()["LastUpdateInterval"]);
			JSON::decode(result.text, val.get<picojson::object>()["text"]);
			return result;
		}
	};

} // namespace JSON


STATIC_ASSERT(sizeof(OpenMPT_SoundDevice_StreamPosition) % 8 == 0);
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

STATIC_ASSERT(sizeof(OpenMPT_SoundDevice_TimeInfo) % 8 == 0);
inline OpenMPT_SoundDevice_TimeInfo encode(SoundDevice::TimeInfo src) {
	OpenMPT_SoundDevice_TimeInfo dst;
	MemsetZero(dst);
	dst.SyncPointStreamFrames = src.SyncPointStreamFrames;
	dst.SyncPointSystemTimestamp = src.SyncPointSystemTimestamp;
	dst.Speed = src.Speed;
	dst.RenderStreamPositionBefore = encode(src.RenderStreamPositionBefore);
	dst.RenderStreamPositionAfter = encode(src.RenderStreamPositionAfter);
	return dst;
}
inline SoundDevice::TimeInfo decode(OpenMPT_SoundDevice_TimeInfo src) {
	SoundDevice::TimeInfo dst;
	dst.SyncPointStreamFrames = src.SyncPointStreamFrames;
	dst.SyncPointSystemTimestamp = src.SyncPointSystemTimestamp;
	dst.Speed = src.Speed;
	dst.RenderStreamPositionBefore = decode(src.RenderStreamPositionBefore);
	dst.RenderStreamPositionAfter = decode(src.RenderStreamPositionAfter);
	return dst;
}

STATIC_ASSERT(sizeof(OpenMPT_SoundDevice_Flags) % 8 == 0);
inline OpenMPT_SoundDevice_Flags encode(SoundDevice::Flags src) {
	OpenMPT_SoundDevice_Flags dst;
	MemsetZero(dst);
	dst.NeedsClippedFloat = src.NeedsClippedFloat;
	return dst;
}
inline SoundDevice::Flags decode(OpenMPT_SoundDevice_Flags src) {
	SoundDevice::Flags dst;
	dst.NeedsClippedFloat = src.NeedsClippedFloat;
	return dst;
}

STATIC_ASSERT(sizeof(OpenMPT_SoundDevice_BufferFormat) % 8 == 0);
inline OpenMPT_SoundDevice_BufferFormat encode(SoundDevice::BufferFormat src) {
	OpenMPT_SoundDevice_BufferFormat dst;
	MemsetZero(dst);
	dst.Samplerate = src.Samplerate;
	dst.Channels = src.Channels;
	dst.InputChannels = src.InputChannels;
	dst.sampleFormat = src.sampleFormat;
	dst.NeedsClippedFloat = src.NeedsClippedFloat;
	dst.DitherType = src.DitherType;
	return dst;
}
inline SoundDevice::BufferFormat decode(OpenMPT_SoundDevice_BufferFormat src) {
	SoundDevice::BufferFormat dst;
	dst.Samplerate = src.Samplerate;
	dst.Channels = src.Channels;
	dst.InputChannels = src.InputChannels;
	dst.sampleFormat = src.sampleFormat;
	dst.NeedsClippedFloat = src.NeedsClippedFloat;
	dst.DitherType = src.DitherType;
	return dst;
}

STATIC_ASSERT(sizeof(OpenMPT_SoundDevice_BufferAttributes) % 8 == 0);
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

STATIC_ASSERT(sizeof(OpenMPT_SoundDevice_RequestFlags) % 8 == 0);
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


template <typename Tdst, typename Tsrc>
struct json_cast_impl
{
	Tdst operator () (Tsrc src);
};


template <typename Tdst, typename Tsrc>
Tdst json_cast(Tsrc src)
{
	return json_cast_impl<Tdst, Tsrc>()(src);
}


template <typename Tsrc>
struct json_cast_impl<picojson::value, Tsrc>
{
	picojson::value operator () (Tsrc src)
	{
		return C::JSON::encode<Tsrc>(src);
	}
};

template <typename Tdst>
struct json_cast_impl<Tdst, picojson::value>
{
	Tdst operator () (picojson::value val)
	{
		return C::JSON::decode<Tdst>(val);
	}
};

template <typename Tsrc>
struct json_cast_impl<std::string, Tsrc>
{
	std::string operator () (Tsrc src)
	{
		picojson::value result = json_cast<picojson::value>(src);
		return result.serialize(true);
	}
};

template <typename Tdst>
struct json_cast_impl<Tdst, std::string>
{
	Tdst operator () (std::string str)
	{
		picojson::value val;
		picojson::parse(val, str);
		return json_cast<Tdst>(val);
	}
};

OPENMPT_NAMESPACE_END
