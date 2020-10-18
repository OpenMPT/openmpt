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

#include "../common/misc_util.h"
#include "../common/mptStringBuffer.h"
#include "Mptrack.h"
#include "Mainfrm.h"

#include <algorithm>
#include "../common/mptFileIO.h"


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
			return Util::BinToHex(mpt::as_span(as<std::vector<std::byte>>()));
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
			value = ConvertStrTo<bool>(newVal);
			break;
		case SettingTypeInt:
			value = ConvertStrTo<int32>(newVal);
			break;
		case SettingTypeFloat:
			value = ConvertStrTo<double>(newVal);
			break;
		case SettingTypeString:
			value = newVal;
			break;
		case SettingTypeBinary:
			value = Util::HexToBin(newVal);
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

void SettingsContainer::WriteSetting(const SettingPath &path, const SettingValue &val, SettingFlushMode flushMode)
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
	if(immediateFlush || flushMode == SettingWriteThrough)
	{
		BackendsWriteSetting(path, val);
		entry->second.Clean();
	}
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

void SettingsContainer::SetImmediateFlush(bool newImmediateFlush)
{
	if(newImmediateFlush)
	{
		Flush();
	}
	immediateFlush = newImmediateFlush;
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




std::vector<std::byte> IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, const std::vector<std::byte> &def) const
{
	std::vector<std::byte> result = def;
	if(!mpt::in_range<UINT>(result.size()))
	{
		return result;
	}
	::GetPrivateProfileStruct(GetSection(path).c_str(), GetKey(path).c_str(), result.data(), static_cast<UINT>(result.size()), filename.AsNative().c_str());
	return result;
}

mpt::ustring IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, const mpt::ustring &def) const
{
	std::vector<TCHAR> buf(128);
	while(::GetPrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWin(def).c_str(), buf.data(), static_cast<DWORD>(buf.size()), filename.AsNative().c_str()) == buf.size() - 1)
	{
		if(buf.size() == std::numeric_limits<DWORD>::max())
		{
			return def;
		}
		buf.resize(Util::ExponentialGrow(buf.size(), std::numeric_limits<DWORD>::max()));
	}
	return mpt::ToUnicode(mpt::winstring(buf.data()));
}

double IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, double def) const
{
	std::vector<TCHAR> buf(128);
	while(::GetPrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(def).c_str(), buf.data(), static_cast<DWORD>(buf.size()), filename.AsNative().c_str()) == buf.size() - 1)
	{
		if(buf.size() == std::numeric_limits<DWORD>::max())
		{
			return def;
		}
		buf.resize(Util::ExponentialGrow(buf.size(), std::numeric_limits<DWORD>::max()));
	}
	return ConvertStrTo<double>(mpt::winstring(buf.data()));
}

int32 IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, int32 def) const
{
	return (int32)::GetPrivateProfileInt(GetSection(path).c_str(), GetKey(path).c_str(), (UINT)def, filename.AsNative().c_str());
}

bool IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, bool def) const
{
	return ::GetPrivateProfileInt(GetSection(path).c_str(), GetKey(path).c_str(), def?1:0, filename.AsNative().c_str()) ? true : false;
}


void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, const std::vector<std::byte> &val)
{
	MPT_ASSERT(mpt::in_range<UINT>(val.size()));
	::WritePrivateProfileStruct(GetSection(path).c_str(), GetKey(path).c_str(), (LPVOID)val.data(), static_cast<UINT>(val.size()), filename.AsNative().c_str());
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, const mpt::ustring &val)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWin(val).c_str(), filename.AsNative().c_str());

	if(mpt::ToUnicode(mpt::Charset::Locale, mpt::ToCharset(mpt::Charset::Locale, val)) != val) // explicit round-trip
	{
		// Value is not representable in ANSI CP.
		// Now check if the string got stored correctly.
		if(ReadSettingRaw(path, mpt::ustring()) != val)
		{
			// The ini file is probably ANSI encoded.
			ConvertToUnicode();
			// Re-write non-ansi-representable value.
			::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWin(val).c_str(), filename.AsNative().c_str());
		}
	}
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, double val)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(val).c_str(), filename.AsNative().c_str());
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, int32 val)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(val).c_str(), filename.AsNative().c_str());
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, bool val)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(val).c_str(), filename.AsNative().c_str());
}

void IniFileSettingsBackend::RemoveSettingRaw(const SettingPath &path)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), NULL, filename.AsNative().c_str());
}

void IniFileSettingsBackend::RemoveSectionRaw(const mpt::ustring &section)
{
	::WritePrivateProfileSection(mpt::ToWin(section).c_str(), _T("\0"), filename.AsNative().c_str());
}


mpt::winstring IniFileSettingsBackend::GetSection(const SettingPath &path)
{
	return mpt::ToWin(path.GetSection());
}
mpt::winstring IniFileSettingsBackend::GetKey(const SettingPath &path)
{
	return mpt::ToWin(path.GetKey());
}



IniFileSettingsBackend::IniFileSettingsBackend(const mpt::PathString &filename)
	: filename(filename)
{
	return;
}

IniFileSettingsBackend::~IniFileSettingsBackend()
{
	return;
}

static std::vector<char> ReadFile(const mpt::PathString &filename)
{
	mpt::ifstream s(filename, std::ios::binary);
	std::vector<char> result;
	while(s)
	{
		char buf[4096];
		s.read(buf, 4096);
		std::streamsize count = s.gcount();
		result.insert(result.end(), buf, buf + count);
	}
	return result;
}

static void WriteFileUTF16LE(const mpt::PathString &filename, const std::wstring &str)
{
	static_assert(sizeof(wchar_t) == 2);
	mpt::SafeOutputFile sinifile(filename, std::ios::binary, mpt::FlushMode::Full);
	mpt::ofstream& inifile = sinifile;
	const uint8 UTF16LE_BOM[] = { 0xff, 0xfe };
	inifile.write(reinterpret_cast<const char*>(UTF16LE_BOM), 2);
	inifile.write(reinterpret_cast<const char*>(str.c_str()), str.length() * sizeof(std::wstring::value_type));
}

void IniFileSettingsBackend::ConvertToUnicode(const mpt::ustring &backupTag)
{
	// Force ini file to be encoded in UTF16.
	// This causes WINAPI ini file functions to keep it in UTF16 encoding
	// and thus support storing unicode strings uncorrupted.
	// This is backwards compatible because even ANSI WINAPI behaves the
	// same way in this case.
	const std::vector<char> data = ReadFile(filename);
	if(!data.empty() && IsTextUnicode(data.data(), mpt::saturate_cast<int>(data.size()), NULL))
	{
		return;
	}
	const mpt::PathString backupFilename = filename + mpt::PathString::FromUnicode(backupTag.empty() ? U_(".ansi.bak") : U_(".ansi.") + backupTag + U_(".bak"));
	CopyFile(filename.AsNative().c_str(), backupFilename.AsNative().c_str(), FALSE);
	WriteFileUTF16LE(filename, mpt::ToWide(mpt::Charset::Locale, mpt::buffer_cast<std::string>(data)));
}

SettingValue IniFileSettingsBackend::ReadSetting(const SettingPath &path, const SettingValue &def) const
{
	switch(def.GetType())
	{
	case SettingTypeBool: return SettingValue(ReadSettingRaw(path, def.as<bool>()), def.GetTypeTag()); break;
	case SettingTypeInt: return SettingValue(ReadSettingRaw(path, def.as<int32>()), def.GetTypeTag()); break;
	case SettingTypeFloat: return SettingValue(ReadSettingRaw(path, def.as<double>()), def.GetTypeTag()); break;
	case SettingTypeString: return SettingValue(ReadSettingRaw(path, def.as<mpt::ustring>()), def.GetTypeTag()); break;
	case SettingTypeBinary: return SettingValue(ReadSettingRaw(path, def.as<std::vector<std::byte> >()), def.GetTypeTag()); break;
	default: return SettingValue(); break;
	}
}

void IniFileSettingsBackend::WriteSetting(const SettingPath &path, const SettingValue &val)
{
	ASSERT(val.GetType() != SettingTypeNone);
	switch(val.GetType())
	{
	case SettingTypeBool: WriteSettingRaw(path, val.as<bool>()); break;
	case SettingTypeInt: WriteSettingRaw(path, val.as<int32>()); break;
	case SettingTypeFloat: WriteSettingRaw(path, val.as<double>()); break;
	case SettingTypeString: WriteSettingRaw(path, val.as<mpt::ustring>()); break;
	case SettingTypeBinary: WriteSettingRaw(path, val.as<std::vector<std::byte> >()); break;
	default: break;
	}
}

void IniFileSettingsBackend::RemoveSetting(const SettingPath &path)
{
	RemoveSettingRaw(path);
}

void IniFileSettingsBackend::RemoveSection(const mpt::ustring &section)
{
	RemoveSectionRaw(section);
}





IniFileSettingsContainer::IniFileSettingsContainer(const mpt::PathString &filename)
	: IniFileSettingsBackend(filename)
	, SettingsContainer(this)
{
	return;
}

IniFileSettingsContainer::~IniFileSettingsContainer()
{
	return;
}



DefaultSettingsContainer::DefaultSettingsContainer()
	: IniFileSettingsContainer(theApp.GetConfigFileName())
{
	return;
}

DefaultSettingsContainer::~DefaultSettingsContainer()
{
	return;
}


OPENMPT_NAMESPACE_END
