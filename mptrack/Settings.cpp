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


SettingsBatching SettingsContainer::BackendsSettingsBatching() const
{
	if(std::holds_alternative<ISettingsBackend<SettingsBatching::Single>*>(backend))
	{
		return SettingsBatching::Single;
	} else if(std::holds_alternative<ISettingsBackend<SettingsBatching::Section>*>(backend))
	{
		return SettingsBatching::Section;
	} else if(std::holds_alternative<ISettingsBackend<SettingsBatching::All>*>(backend))
	{
		return SettingsBatching::All;
	} else
	{
		MPT_ASSERT_NOTREACHED();
		throw std::bad_variant_access{};
	}
}

void SettingsContainer::BackendsInvalidateCache()
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Single || batching == SettingsBatching::Section || batching == SettingsBatching::All);
	if(batching == SettingsBatching::All)
	{
		std::get<ISettingsBackend<SettingsBatching::All>*>(backend)->InvalidateCache();
		return;
	} else if(batching == SettingsBatching::Section)
	{
		std::get<ISettingsBackend<SettingsBatching::Section>*>(backend)->InvalidateCache();
		return;
	}
	MPT_ASSERT_NOTREACHED();
}

SettingValue SettingsContainer::BackendsReadSetting(const SettingPath &path, const SettingValue &def) const
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Single || batching == SettingsBatching::Section || batching == SettingsBatching::All);
	if(batching == SettingsBatching::All)
	{
		return std::get<ISettingsBackend<SettingsBatching::All>*>(backend)->ReadSetting(path, def);
	} else if(batching == SettingsBatching::Section)
	{
		return std::get<ISettingsBackend<SettingsBatching::Section>*>(backend)->ReadSetting(path, def);
	} else if(batching == SettingsBatching::Single)
	{
		return std::get<ISettingsBackend<SettingsBatching::Single>*>(backend)->ReadSetting(path, def);
	}
	MPT_ASSERT_NOTREACHED();
	throw std::bad_variant_access{};
}

void SettingsContainer::BackendsWriteSetting(const SettingPath &path, const SettingValue &val)
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Single);
	if(batching == SettingsBatching::Single)
	{
		std::get<ISettingsBackend<SettingsBatching::Single>*>(backend)->WriteSetting(path, val);
		return;
	}
	MPT_ASSERT_NOTREACHED();
}

void SettingsContainer::BackendsRemoveSetting(const SettingPath &path)
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Single);
	if(batching == SettingsBatching::Single)
	{
		std::get<ISettingsBackend<SettingsBatching::Single>*>(backend)->RemoveSetting(path);
		return;
	}
	MPT_ASSERT_NOTREACHED();
}

void SettingsContainer::BackendsRemoveSection(const mpt::ustring &section)
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Single);
	if(batching == SettingsBatching::Single)
	{
		std::get<ISettingsBackend<SettingsBatching::Single>*>(backend)->RemoveSection(section);
		return;
	}
	MPT_ASSERT_NOTREACHED();
}

void SettingsContainer::BackendsWriteRemovedSections(const std::set<mpt::ustring> &removeSections)
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Section);
	if(batching == SettingsBatching::Section)
	{
		std::get<ISettingsBackend<SettingsBatching::Section>*>(backend)->WriteRemovedSections(removeSections);
		return;
	}
	MPT_ASSERT_NOTREACHED();
}

void SettingsContainer::BackendsWriteMultipleSettings(const std::map<SettingPath, std::optional<SettingValue>> &settings)
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Section);
	if(batching == SettingsBatching::Section)
	{
		std::get<ISettingsBackend<SettingsBatching::Section>*>(backend)->WriteMultipleSettings(settings);
		return;
	}
	MPT_ASSERT_NOTREACHED();
}

void SettingsContainer::BackendsWriteAllSettings(const std::set<mpt::ustring> &removeSections, const std::map<SettingPath, std::optional<SettingValue>> &settings, std::optional<Caching> sync_hint)
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::All);
	if(batching == SettingsBatching::All)
	{
		std::get<ISettingsBackend<SettingsBatching::All>*>(backend)->WriteAllSettings(removeSections, settings, sync_hint.has_value() ? sync_hint : sync_fallback);
		return;
	}
	MPT_ASSERT_NOTREACHED();
}

void SettingsContainer::BackendsSync(std::optional<Caching> sync_hint)
{
	const SettingsBatching batching = BackendsSettingsBatching();
	ASSERT(batching == SettingsBatching::Single || batching == SettingsBatching::Section);
	if(batching == SettingsBatching::Section)
	{
		return std::get<ISettingsBackend<SettingsBatching::Section>*>(backend)->Sync(sync_hint.has_value() ? sync_hint : sync_fallback);
	} else if(batching == SettingsBatching::Single)
	{
		return std::get<ISettingsBackend<SettingsBatching::Single>*>(backend)->Sync(sync_hint.has_value() ? sync_hint : sync_fallback);
	}
	MPT_ASSERT_NOTREACHED();
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
	if(!entry->second.has_value())
	{
		entry->second.emplace(SettingState(def).assign(BackendsReadSetting(path, def), false));
	}
	return entry->second.value();
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
	if(!entry->second.has_value())
	{
		return true;
	}
	return entry->second.value().IsDefault();
}

void SettingsContainer::WriteSetting(const SettingPath &path, const SettingValue &val)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	auto entry = map.find(path);
	if(entry == map.end())
	{
		map[path] = SettingValue();
		entry = map.find(path);
	}
	if(!entry->second.has_value())
	{
		entry->second = SettingValue();
	}
	entry->second = val;
	NotifyListeners(path);
}

void SettingsContainer::ForgetSetting(const SettingPath &path)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	map.erase(path);
}

void SettingsContainer::InvalidateCache()
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	map.clear();
	if(BackendsSettingsBatching() == SettingsBatching::Section || BackendsSettingsBatching() == SettingsBatching::All)
	{
		BackendsInvalidateCache();
	}
}

void SettingsContainer::RemoveSetting(const SettingPath &path)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	if(BackendsSettingsBatching() == SettingsBatching::Single)
	{
		map.erase(path);
		BackendsRemoveSetting(path);
	} else
	{
		map[path] = std::nullopt;
	}
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
	if(BackendsSettingsBatching() == SettingsBatching::Single)
	{
		BackendsRemoveSection(section);
	} else
	{
		removedSections.insert(section);
	}
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

void SettingsContainer::WriteSettings(std::optional<Caching> sync_hint)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	if(BackendsSettingsBatching() == SettingsBatching::Single)
	{
		std::set<SettingPath> removedsettings;
		for(auto &[path, value] : map)
		{
			if(value.has_value())
			{
				if(value.value().IsDirty())
				{
					BackendsWriteSetting(path, value.value());
					value.value().Clean();
				}
			} else
			{
				removedsettings.insert(path);
				BackendsRemoveSetting(path);
			}
		}
		for(const auto & path : removedsettings)
		{
			map.erase(path);
		}
		BackendsSync(sync_hint);
	} else if(BackendsSettingsBatching() == SettingsBatching::Section || BackendsSettingsBatching() == SettingsBatching::All)
	{
		std::map<SettingPath, std::optional<SettingValue>> settings;
		for(auto &[path, value] : map)
		{
			if(value.has_value())
			{
				if(value.value().IsDirty())
				{
					settings.insert(std::make_pair(path, std::make_optional(value.value().GetRefValue())));
				}
			} else
			{
				settings.insert(std::make_pair(path, std::nullopt));
			}
		}
		if(BackendsSettingsBatching() == SettingsBatching::Section)
		{
			BackendsWriteRemovedSections(removedSections);
			BackendsWriteMultipleSettings(settings);
			BackendsSync(sync_hint);
		} else if(BackendsSettingsBatching() == SettingsBatching::All)
		{
			BackendsWriteAllSettings(removedSections, settings, sync_hint);
		}
		std::set<SettingPath> removedsettings;
		for(auto &[path, value] : map)
		{
			if(value.has_value())
			{
				if(value.value().IsDirty())
				{
					value.value().Clean();
				}
			} else
			{
				removedsettings.insert(path);
			}
		}
		for(const auto & path : removedsettings)
		{
			map.erase(path);
		}
		removedSections.clear();
	}
}

void SettingsContainer::SetSync(std::optional<Caching> sync_hint)
{
	sync_fallback = sync_hint;
}

void SettingsContainer::Flush(std::optional<Caching> sync_hint)
{
	ASSERT(theApp.InGuiThread());
	ASSERT(!CMainFrame::GetMainFrame() || (CMainFrame::GetMainFrame() && !CMainFrame::GetMainFrame()->InNotifyHandler())); // This is a slow path, use CachedSetting for stuff that is accessed in notify handler.
	WriteSettings(sync_hint);
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
	// cppcheck complains about potentially throwing std::bad_variant_access in dtor,
	// however that cannot happen in practive here.
	// cppcheck-suppress throwInNoexceptFunction
	WriteSettings(sync_fallback);
}


SettingsContainer::SettingsContainer(ISettingsBackend<SettingsBatching::Single> *backend, std::optional<Caching> sync_hint)
	: backend(backend)
	, sync_fallback(sync_hint)
{
	MPT_ASSERT(backend);
}

SettingsContainer::SettingsContainer(ISettingsBackend<SettingsBatching::Section> *backend, std::optional<Caching> sync_hint)
	: backend(backend)
	, sync_fallback(sync_hint)
{
	MPT_ASSERT(backend);
}

SettingsContainer::SettingsContainer(ISettingsBackend<SettingsBatching::All> *backend, std::optional<Caching> sync_hint)
	: backend(backend)
	, sync_fallback(sync_hint)
{
	MPT_ASSERT(backend);
}



OPENMPT_NAMESPACE_END
