/*
 * SoundDevice.cpp
 * ---------------
 * Purpose: Sound device interfaces.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SoundDevice.h"

#include "../common/mptStringFormat.h"
#include "../common/misc_util.h"

#include <map>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


SysInfo::SysInfo()
	: SystemClass(mpt::OS::GetClass())
	, WindowsVersion(mpt::OS::Windows::Version::Current())
	, IsWine(mpt::OS::Windows::IsWine())
	, WineHostClass(mpt::OS::Class::Unknown)
	, WineVersion(mpt::OS::Wine::Version())
{
	mpt::OS::Wine::VersionContext wineVersionContext;
	WineHostClass = wineVersionContext.HostClass();
	WineVersion = wineVersionContext.Version();
}


SysInfo SysInfo::Current()
{
	return SysInfo();
}


SoundDevice::Type ParseType(const SoundDevice::Identifier &identifier)
{
	std::vector<mpt::ustring> tmp = mpt::String::Split<mpt::ustring>(identifier, U_("_"));
	if(tmp.size() == 0)
	{
		return SoundDevice::Type();
	}
	return tmp[0];
}


mpt::ustring Info::GetDisplayName() const
{
	mpt::ustring result = apiName + U_(" - ") + mpt::String::Trim(name);
	switch(flags.usability)
	{
	case SoundDevice::Info::Usability::Experimental:
		result += U_(" [experimental]");
		break;
	case SoundDevice::Info::Usability::Deprecated:
		result += U_(" [deprecated]");
		break;
	case SoundDevice::Info::Usability::Broken:
		result += U_(" [broken]");
		break;
	case SoundDevice::Info::Usability::NotAvailable:
		result += U_(" [alien]");
		break;
	default:
		// nothing
		break;
	}
	if(default_ == SoundDevice::Info::Default::Named)
	{
		result += U_(" [default]");
	}
	if(apiPath.size() > 0)
	{
		result += U_(" (") + mpt::String::Combine(apiPath, U_("/")) + U_(")");
	}
	return result;
}


ChannelMapping::ChannelMapping(uint32 numHostChannels)
{
	ChannelToDeviceChannel.resize(numHostChannels);
	for(uint32 channel = 0; channel < numHostChannels; ++channel)
	{
		ChannelToDeviceChannel[channel] = channel;
	}
}


ChannelMapping::ChannelMapping(const std::vector<int32> &mapping)
{
	if(IsValid(mapping))
	{
		ChannelToDeviceChannel = mapping;
	}
}


ChannelMapping ChannelMapping::BaseChannel(uint32 channels, int32 baseChannel)
{
	SoundDevice::ChannelMapping result;
	result.ChannelToDeviceChannel.resize(channels);
	for(uint32 channel = 0; channel < channels; ++channel)
	{
		result.ChannelToDeviceChannel[channel] = channel + baseChannel;
	}
	return result;
}


bool ChannelMapping::IsValid(const std::vector<int32> &mapping)
{
	if(mapping.empty())
	{
		return true;
	}
	std::map<int32, uint32> inverseMapping;
	for(uint32 hostChannel = 0; hostChannel < mapping.size(); ++hostChannel)
	{
		int32 deviceChannel = mapping[hostChannel];
		if(deviceChannel < 0)
		{
			return false;
		}
		if(deviceChannel > MaxDeviceChannel)
		{
			return false;
		}
		inverseMapping[deviceChannel] = hostChannel;
	}
	if(inverseMapping.size() != mapping.size())
	{
		return false;
	}
	return true;
}


mpt::ustring ChannelMapping::ToUString() const
{
	return mpt::String::Combine<int32>(ChannelToDeviceChannel, U_(","));
}


ChannelMapping ChannelMapping::FromString(const mpt::ustring &str)
{
	return SoundDevice::ChannelMapping(mpt::String::Split<int32>(str, U_(",")));
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
