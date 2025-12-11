/*
 * Settings.cpp
 * ------------
 * Purpose: Application setting handling framework.
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "Settings.h"

#include "mpt/binary/hex.hpp"
#include "mpt/parse/parse.hpp"

#include "../common/misc_util.h"
#include "../common/mptStringBuffer.h"

#include "Mptrack.h"
#include "Mainfrm.h"

#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


mpt::ustring SettingValue::FormatTypeAsString() const
{
	if(GetType() == SettingTypeNone)
	{
		return U_("nil");
	}
	mpt::ustring result;
	switch(GetType())
	{
		case SettingTypeBool:
			result += U_("bool");
			break;
		case SettingTypeInt:
			result += U_("int");
			break;
		case SettingTypeFloat:
			result += U_("float");
			break;
		case SettingTypeString:
			result += U_("string");
			break;
		case SettingTypeBinary:
			result += U_("binary");
			break;
		case SettingTypeNone:
		default:
			result += U_("nil");
			break;
	}
	if(HasTypeTag() && !GetTypeTag().empty())
	{
		result += U_(":") + mpt::ToUnicode(mpt::Charset::ASCII, GetTypeTag());
	}
	return result;
}


mpt::ustring SettingValue::FormatValueAsString() const
{
	switch(GetType())
	{
		case SettingTypeBool:
			return mpt::ufmt::val(as<bool>());
			break;
		case SettingTypeInt:
			return mpt::ufmt::val(as<int32>());
			break;
		case SettingTypeFloat:
			return mpt::ufmt::val(as<double>());
			break;
		case SettingTypeString:
			return as<mpt::ustring>();
			break;
		case SettingTypeBinary:
			return mpt::encode_hex(mpt::as_span(as<std::vector<std::byte>>()));
			break;
		case SettingTypeNone:
		default:
			return mpt::ustring();
			break;
	}
}


void SettingValue::SetFromString(const AnyStringLocale &newVal)
{
	switch(GetType())
	{
		case SettingTypeBool:
			value = mpt::parse<bool, mpt::ustring>(newVal);
			break;
		case SettingTypeInt:
			value = mpt::parse<int32, mpt::ustring>(newVal);
			break;
		case SettingTypeFloat:
			value = mpt::parse<double, mpt::ustring>(newVal);
			break;
		case SettingTypeString:
			value = newVal;
			break;
		case SettingTypeBinary:
			value = mpt::decode_hex(newVal);
			break;
		case SettingTypeNone:
		default:
			break;
	}
}


SettingValue SettingsContainer::BackendsReadSetting(const SettingPath &path, const SettingValue &def) const
{
	return backend->ReadSetting(path, def);
}

void SettingsContainer::BackendsWriteSetting(const SettingPath &path, const SettingValue &val)
{
	backend->WriteSetting(path, val);
}

void SettingsContainer::BackendsRemoveSetting(const SettingPath &path)
{
	backend->RemoveSetting(path);
}

void SettingsContainer::BackendsRemoveSection(const mpt::ustring &section)
{
	backend->RemoveSection(section);
}

SettingValue SettingsContainer::ReadSetting(const SettingPath &path, const SettingValue &def) const
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	auto entry = map.find(path);
	if(entry == map.end())
	{
		entry = map.insert(map.begin(), std::make_pair(path, SettingState(def).assign(BackendsReadSetting(path, def), false)));
	}
	return entry->second;
}

bool SettingsContainer::IsDefaultSetting(const SettingPath &path) const
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	auto entry = map.find(path);
	if(entry == map.end())
	{
		return true;
	}
	return entry->second.IsDefault();
}

void SettingsContainer::WriteSetting(const SettingPath &path, const SettingValue &val)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	auto entry = map.find(path);
	if(entry == map.end())
	{
		map[path] = val;
		entry = map.find(path);
	} else
	{
		entry->second = val;
	}
	NotifyListeners(path);
}

void SettingsContainer::ForgetSetting(const SettingPath &path)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	map.erase(path);
}

void SettingsContainer::ForgetAll()
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	map.clear();
}

void SettingsContainer::RemoveSetting(const SettingPath &path)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	map.erase(path);
	BackendsRemoveSetting(path);
}

void SettingsContainer::RemoveSection(const mpt::ustring &section)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	std::vector<SettingPath> pathsToRemove;
	for(const auto &entry : map)
	{
		if(entry.first.GetSection() == section)
		{
			pathsToRemove.push_back(entry.first);
		}
	}
	for(const auto &path : pathsToRemove)
	{
		map.erase(path);
	}
	BackendsRemoveSection(section);
}

void SettingsContainer::NotifyListeners(const SettingPath &path)
{
	const auto entry = mapListeners.find(path);
	if(entry != mapListeners.end())
	{
		for(auto &it : entry->second)
		{
			it->SettingChanged(path);
		}
	}
}

void SettingsContainer::WriteSettings()
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	for(auto &[path, value] : map)
	{
		if(value.IsDirty())
		{
			BackendsWriteSetting(path, value);
			value.Clean();
		}
	}
}

void SettingsContainer::Flush()
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	WriteSettings();
}

void SettingsContainer::Register(ISettingChanged *listener, const SettingPath &path)
{
	mapListeners[path].insert(listener);
}

void SettingsContainer::UnRegister(ISettingChanged *listener, const SettingPath &path)
{
	mapListeners[path].erase(listener);
}

SettingsContainer::~SettingsContainer()
{
	WriteSettings();
}


SettingsContainer::SettingsContainer(ISettingsBackend *backend)
	: backend(backend)
{
	MPT_ASSERT(backend);
}



OPENMPT_NAMESPACE_END
