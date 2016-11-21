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
#include "../common/StringFixer.h"
#include "Mptrack.h"
#include "Mainfrm.h"

#include <algorithm>
#include "../common/mptFileIO.h"


OPENMPT_NAMESPACE_BEGIN


mpt::ustring SettingValue::FormatTypeAsString() const
{
	if(GetType() == SettingTypeNone)
	{
		return MPT_USTRING("nil");
	}
	mpt::ustring result;
	switch(GetType())
	{
		case SettingTypeBool:
			result += MPT_USTRING("bool");
			break;
		case SettingTypeInt:
			result += MPT_USTRING("int");
			break;
		case SettingTypeFloat:
			result += MPT_USTRING("float");
			break;
		case SettingTypeString:
			result += MPT_USTRING("string");
			break;
		case SettingTypeBinary:
			result += MPT_USTRING("binary");
			break;
		case SettingTypeNone:
		default:
			result += MPT_USTRING("nil");
			break;
	}
	if(HasTypeTag() && !GetTypeTag().empty())
	{
		result += MPT_USTRING(":") + mpt::ToUnicode(mpt::CharsetASCII, GetTypeTag());
	}
	return result;
}


mpt::ustring SettingValue::FormatValueAsString() const
{
	switch(GetType())
	{
		case SettingTypeBool:
			return mpt::ToUString(valueBool);
			break;
		case SettingTypeInt:
			return mpt::ToUString(valueInt);
			break;
		case SettingTypeFloat:
			return mpt::ToUString(valueFloat);
			break;
		case SettingTypeString:
			return valueString;
			break;
		case SettingTypeBinary:
			return Util::BinToHex(mpt::as_span(valueBinary));
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
			valueBool = ConvertStrTo<bool>(newVal);
			break;
		case SettingTypeInt:
			valueInt = ConvertStrTo<int32>(newVal);
			break;
		case SettingTypeFloat:
			valueFloat = ConvertStrTo<double>(newVal);
			break;
		case SettingTypeString:
			valueString = newVal;
			break;
		case SettingTypeBinary:
			valueBinary = Util::HexToBin(newVal);
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
	for(auto &i : map)
	{
		if(i.second.IsDirty())
		{
			BackendsWriteSetting(i.first, i.second);
			i.second.Clean();
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
	, immediateFlush(false)
{
	ASSERT(backend);
}




std::vector<mpt::byte> IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, const std::vector<mpt::byte> &def) const
{
	std::vector<mpt::byte> result = def;
	if(!Util::TypeCanHoldValue<UINT>(result.size()))
	{
		return result;
	}
	::GetPrivateProfileStructW(GetSection(path).c_str(), GetKey(path).c_str(), result.data(), static_cast<UINT>(result.size()), filename.AsNative().c_str());
	return result;
}

std::wstring IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, const std::wstring &def) const
{
	std::vector<WCHAR> buf(128);
	while(::GetPrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), def.c_str(), buf.data(), static_cast<DWORD>(buf.size()), filename.AsNative().c_str()) == buf.size() - 1)
	{
		if(buf.size() == std::numeric_limits<DWORD>::max())
		{
			return def;
		}
		buf.resize(Util::ExponentialGrow(buf.size(), std::numeric_limits<DWORD>::max()));
	}
	return buf.data();
}

double IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, double def) const
{
	std::vector<WCHAR> buf(128);
	while(::GetPrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWString(def).c_str(), buf.data(), static_cast<DWORD>(buf.size()), filename.AsNative().c_str()) == buf.size() - 1)
	{
		if(buf.size() == std::numeric_limits<DWORD>::max())
		{
			return def;
		}
		buf.resize(Util::ExponentialGrow(buf.size(), std::numeric_limits<DWORD>::max()));
	}
	return ConvertStrTo<double>(std::wstring(buf.data()));
}

int32 IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, int32 def) const
{
	return (int32)::GetPrivateProfileIntW(GetSection(path).c_str(), GetKey(path).c_str(), (UINT)def, filename.AsNative().c_str());
}

bool IniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, bool def) const
{
	return ::GetPrivateProfileIntW(GetSection(path).c_str(), GetKey(path).c_str(), def?1:0, filename.AsNative().c_str()) ? true : false;
}


void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, const std::vector<mpt::byte> &val)
{
	MPT_ASSERT(Util::TypeCanHoldValue<UINT>(val.size()));
	::WritePrivateProfileStructW(GetSection(path).c_str(), GetKey(path).c_str(), (LPVOID)val.data(), static_cast<UINT>(val.size()), filename.AsNative().c_str());
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, const std::wstring &val)
{
	::WritePrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), val.c_str(), filename.AsNative().c_str());

	if(mpt::ToWide(mpt::CharsetLocale, mpt::ToCharset(mpt::CharsetLocale, val)) != val) // explicit round-trip
	{
		// Value is not representable in ANSI CP.
		// Now check if the string got stored correctly.
		if(ReadSettingRaw(path, std::wstring()) != val)
		{
			// The ini file is probably ANSI encoded.
			ConvertToUnicode();
			// Re-write non-ansi-representable value.
			::WritePrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), val.c_str(), filename.AsNative().c_str());
		}
	}
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, double val)
{
	::WritePrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWString(val).c_str(), filename.AsNative().c_str());
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, int32 val)
{
	::WritePrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWString(val).c_str(), filename.AsNative().c_str());
}

void IniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, bool val)
{
	::WritePrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWString(val).c_str(), filename.AsNative().c_str());
}

void IniFileSettingsBackend::RemoveSettingRaw(const SettingPath &path)
{
	::WritePrivateProfileStringW(GetSection(path).c_str(), GetKey(path).c_str(), NULL, filename.AsNative().c_str());
}


std::wstring IniFileSettingsBackend::GetSection(const SettingPath &path)
{
	return mpt::ToWide(path.GetSection());
}
std::wstring IniFileSettingsBackend::GetKey(const SettingPath &path)
{
	return mpt::ToWide(path.GetKey());
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
	STATIC_ASSERT(sizeof(wchar_t) == 2);
	mpt::ofstream inifile(filename, std::ios::binary | std::ios::trunc);
	const uint8 UTF16LE_BOM[] = { 0xff, 0xfe };
	inifile.write(reinterpret_cast<const char*>(UTF16LE_BOM), 2);
	inifile.write(reinterpret_cast<const char*>(str.c_str()), str.length() * sizeof(std::wstring::value_type));
	inifile.flush();
	inifile.close();
}

void IniFileSettingsBackend::ConvertToUnicode(const std::wstring &backupTag)
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
	const mpt::PathString backupFilename = filename + mpt::PathString::FromWide(backupTag.empty() ? L".ansi.bak" : L".ansi." + backupTag + L".bak");
	CopyFileW(filename.AsNative().c_str(), backupFilename.AsNative().c_str(), FALSE);
	WriteFileUTF16LE(filename, mpt::ToWide(mpt::CharsetLocale, std::string(data.data(), data.data() + data.size())));
}

SettingValue IniFileSettingsBackend::ReadSetting(const SettingPath &path, const SettingValue &def) const
{
	switch(def.GetType())
	{
	case SettingTypeBool: return SettingValue(ReadSettingRaw(path, def.as<bool>()), def.GetTypeTag()); break;
	case SettingTypeInt: return SettingValue(ReadSettingRaw(path, def.as<int32>()), def.GetTypeTag()); break;
	case SettingTypeFloat: return SettingValue(ReadSettingRaw(path, def.as<double>()), def.GetTypeTag()); break;
	case SettingTypeString: return SettingValue(ReadSettingRaw(path, def.as<std::wstring>()), def.GetTypeTag()); break;
	case SettingTypeBinary: return SettingValue(ReadSettingRaw(path, def.as<std::vector<mpt::byte> >()), def.GetTypeTag()); break;
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
	case SettingTypeString: WriteSettingRaw(path, val.as<std::wstring>()); break;
	case SettingTypeBinary: WriteSettingRaw(path, val.as<std::vector<mpt::byte> >()); break;
	default: break;
	}
}

void IniFileSettingsBackend::RemoveSetting(const SettingPath &path)
{
	RemoveSettingRaw(path);
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
