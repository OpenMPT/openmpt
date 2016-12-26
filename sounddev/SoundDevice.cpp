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

#include "../common/misc_util.h"

#include <map>


OPENMPT_NAMESPACE_BEGIN


namespace SoundDevice {


SysInfo::SysInfo()
//----------------
	: WindowsVersion(mpt::Windows::Version::Current())
	, IsWine(mpt::Windows::IsWine())
	, WineHostIsLinux(false)
	, WineVersion(mpt::Wine::Version())
{
	mpt::Wine::VersionContext wineVersionContext;
	WineHostIsLinux = wineVersionContext.HostIsLinux();
	WineVersion = wineVersionContext.Version();
}


SysInfo SysInfo::Current()
//------------------------
{
	return SysInfo();
}


SoundDevice::Type ParseType(const SoundDevice::Identifier &identifier)
//--------------------------------------------------------------------
{
	std::vector<mpt::ustring> tmp = mpt::String::Split<mpt::ustring>(identifier, MPT_USTRING("_"));
	if(tmp.size() == 0)
	{
		return SoundDevice::Type();
	}
	return tmp[0];
}


ChannelMapping::ChannelMapping(uint32 numHostChannels)
//----------------------------------------------------
{
	ChannelToDeviceChannel.resize(numHostChannels);
	for(uint32 channel = 0; channel < numHostChannels; ++channel)
	{
		ChannelToDeviceChannel[channel] = channel;
	}
}


ChannelMapping::ChannelMapping(const std::vector<int32> &mapping)
//---------------------------------------------------------------
{
	if(IsValid(mapping))
	{
		ChannelToDeviceChannel = mapping;
	}
}


ChannelMapping ChannelMapping::BaseChannel(uint32 channels, int32 baseChannel)
//----------------------------------------------------------------------------
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
//-------------------------------------------------------------
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


std::string ChannelMapping::ToString() const
//------------------------------------------
{
	return mpt::String::Combine<int32>(ChannelToDeviceChannel, std::string(","));
}


mpt::ustring ChannelMapping::ToUString() const
//--------------------------------------------
{
	return mpt::String::Combine<int32>(ChannelToDeviceChannel, MPT_USTRING(","));
}


ChannelMapping ChannelMapping::FromString(const std::string &str)
//---------------------------------------------------------------
{
	return SoundDevice::ChannelMapping(mpt::String::Split<int32>(str, std::string(",")));
}


ChannelMapping ChannelMapping::FromString(const mpt::ustring &str)
//----------------------------------------------------------------
{
	return SoundDevice::ChannelMapping(mpt::String::Split<int32>(str, MPT_USTRING(",")));
}


} // namespace SoundDevice


OPENMPT_NAMESPACE_END
