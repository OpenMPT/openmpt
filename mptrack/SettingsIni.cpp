/*
 * SettingsIni.cpp
 * ---------------
 * Purpose: Application settings in ini format.
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "SettingsIni.h"

#include "mpt/binary/hex.hpp"
#include "mpt/io_file/fstream.hpp"
#include "mpt/io_file/outputfile.hpp"
#include "mpt/parse/parse.hpp"

#include "../common/misc_util.h"
#include "../common/mptFileIO.h"
#include "../common/mptStringBuffer.h"

#include <algorithm>
#include <utility>



OPENMPT_NAMESPACE_BEGIN



WindowsIniFileBase::WindowsIniFileBase(mpt::PathString filename_)
	: filename(std::move(filename_))
	, file(filename)
{
	return;
}

void WindowsIniFileBase::ConvertToUnicode(const mpt::ustring &backupTag)
{
	// Force ini file to be encoded in UTF16.
	// This causes WINAPI ini file functions to keep it in UTF16 encoding
	// and thus support storing unicode strings uncorrupted.
	// This is backwards compatible because even ANSI WINAPI behaves the
	// same way in this case.
	// Do not convert when not runing a UNICODE build.
	// Do not convert when running on Windows 2000 or earlier
	// because of missing Unicode support on Win9x,
#if defined(UNICODE)
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 26110)
#pragma warning(disable : 26117)
#endif // MPT_COMPILER_MSVC
	if(mpt::osinfo::windows::Version::Current().IsBefore(mpt::osinfo::windows::Version::WinXP))
	{
		return;
	}
	{
		std::lock_guard l{file};
		const std::vector<std::byte> data = file.read();
		if(!data.empty() && IsTextUnicode(data.data(), mpt::saturate_cast<int>(data.size()), NULL))
		{
			return;
		}
		{
			const mpt::PathString backupFilename = filename + mpt::PathString::FromUnicode(backupTag.empty() ? U_(".ansi.bak") : U_(".ansi.") + backupTag + U_(".bak"));
			mpt::IO::atomic_shared_file_ref backup{backupFilename};
			// cppcheck bogus warning
			// cppcheck-suppress localMutex
			std::lock_guard bl{backup};
			backup.write(mpt::as_span(data));
		}
		static_assert(sizeof(wchar_t) == 2);
		std::ostringstream inifile{std::ios::binary};
		inifile.imbue(std::locale::classic());
		const uint8 UTF16LE_BOM[] = { 0xff, 0xfe };
		inifile.write(reinterpret_cast<const char*>(UTF16LE_BOM), 2);
		const std::wstring str = mpt::ToWide(mpt::Charset::Locale, mpt::buffer_cast<std::string>(data));
		inifile.write(reinterpret_cast<const char*>(str.c_str()), str.length() * sizeof(std::wstring::value_type));
		file.write(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(inifile.str())));
	}
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC
#else
	MPT_UNUSED(backupTag);
#endif
}

const mpt::PathString &WindowsIniFileBase::Filename() const
{
	return filename;
}

mpt::PathString WindowsIniFileBase::GetFilename() const
{
	return filename;
}



mpt::winstring IniFileHelpers::GetSection(const SettingPath &path)
{
	return mpt::ToWin(path.GetSection());
}

mpt::winstring IniFileHelpers::GetKey(const SettingPath &path)
{
	return mpt::ToWin(path.GetKey());
}

mpt::ustring IniFileHelpers::FormatValueAsIni(const SettingValue &value)
{
	switch(value.GetType())
	{
		case SettingTypeBool:
			return mpt::ufmt::val(value.as<bool>());
			break;
		case SettingTypeInt:
			return mpt::ufmt::val(value.as<int32>());
			break;
		case SettingTypeFloat:
			return mpt::ufmt::val(value.as<double>());
			break;
		case SettingTypeString:
			return value.as<mpt::ustring>();
			break;
		case SettingTypeBinary:
			{
				std::vector<std::byte> data = value.as<std::vector<std::byte>>();
				uint8 checksum = 0;
				for(const std::byte x : data) {
					checksum += mpt::byte_cast<uint8>(x);
				}
				return mpt::encode_hex(mpt::as_span(data)) + mpt::ufmt::HEX0<2>(checksum);
			}
			break;
		case SettingTypeNone:
		default:
			return mpt::ustring();
			break;
	}
}

SettingValue IniFileHelpers::ParseValueFromIni(const mpt::ustring &str, const SettingValue &def)
{
	switch(def.GetType())
	{
		case SettingTypeBool:
			return SettingValue(mpt::parse_or<bool>(mpt::trim(str), def.as<bool>()), def.GetTypeTag());
			break;
		case SettingTypeInt:
			return SettingValue(mpt::parse_or<int32>(mpt::trim(str), def.as<int32>()), def.GetTypeTag());
			break;
		case SettingTypeFloat:
			return SettingValue(mpt::parse_or<double>(mpt::trim(str), def.as<double>()), def.GetTypeTag());
			break;
		case SettingTypeString:
			return SettingValue(str, def.GetTypeTag());
			break;
		case SettingTypeBinary:
			{
				if((str.length() % 2) != 0)
				{
					return SettingValue(def.as<std::vector<std::byte>>(), def.GetTypeTag());
				}
				std::vector<std::byte> data = mpt::decode_hex(str);
				if(data.size() < 1)
				{
					return SettingValue(def.as<std::vector<std::byte>>(), def.GetTypeTag());
				}
				const uint8 storedchecksum = mpt::byte_cast<uint8>(data[data.size() - 1]);
				data.resize(data.size() - 1);
				uint8 calculatedchecksum = 0;
				for(const std::byte x : data) {
					calculatedchecksum += mpt::byte_cast<uint8>(x);
				}
				if(calculatedchecksum != storedchecksum)
				{
					return SettingValue(def.as<std::vector<std::byte>>(), def.GetTypeTag());
				}
				return SettingValue(data, def.GetTypeTag());
			}
			break;
		default:
			return SettingValue();
			break;
	}
}



std::vector<std::byte> ImmediateWindowsIniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, const std::vector<std::byte> &def) const
{
	std::vector<std::byte> result = def;
	if(!mpt::in_range<UINT>(result.size()))
	{
		return result;
	}
	::GetPrivateProfileStruct(GetSection(path).c_str(), GetKey(path).c_str(), result.data(), static_cast<UINT>(result.size()), filename.AsNative().c_str());
	return result;
}

mpt::ustring ImmediateWindowsIniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, const mpt::ustring &def) const
{
	std::vector<TCHAR> buf(128);
	while(::GetPrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::ToWin(def).c_str(), buf.data(), static_cast<DWORD>(buf.size()), filename.AsNative().c_str()) == buf.size() - 1)
	{
		if(buf.size() == std::numeric_limits<DWORD>::max())
		{
			return def;
		}
		buf.resize(mpt::exponential_grow(buf.size(), std::numeric_limits<DWORD>::max()));
	}
	return mpt::ToUnicode(mpt::winstring(buf.data()));
}

double ImmediateWindowsIniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, double def) const
{
	std::vector<TCHAR> buf(128);
	while(::GetPrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(def).c_str(), buf.data(), static_cast<DWORD>(buf.size()), filename.AsNative().c_str()) == buf.size() - 1)
	{
		if(buf.size() == std::numeric_limits<DWORD>::max())
		{
			return def;
		}
		buf.resize(mpt::exponential_grow(buf.size(), std::numeric_limits<DWORD>::max()));
	}
	return mpt::parse<double>(mpt::winstring(buf.data()));
}

int32 ImmediateWindowsIniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, int32 def) const
{
	return (int32)::GetPrivateProfileInt(GetSection(path).c_str(), GetKey(path).c_str(), (UINT)def, filename.AsNative().c_str());
}

bool ImmediateWindowsIniFileSettingsBackend::ReadSettingRaw(const SettingPath &path, bool def) const
{
	return ::GetPrivateProfileInt(GetSection(path).c_str(), GetKey(path).c_str(), def?1:0, filename.AsNative().c_str()) ? true : false;
}

void ImmediateWindowsIniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, const std::vector<std::byte> &val)
{
	MPT_ASSERT(mpt::in_range<UINT>(val.size()));
	::WritePrivateProfileStruct(GetSection(path).c_str(), GetKey(path).c_str(), (LPVOID)val.data(), static_cast<UINT>(val.size()), filename.AsNative().c_str());
}

void ImmediateWindowsIniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, const mpt::ustring &val)
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

void ImmediateWindowsIniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, double val)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(val).c_str(), filename.AsNative().c_str());
}

void ImmediateWindowsIniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, int32 val)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(val).c_str(), filename.AsNative().c_str());
}

void ImmediateWindowsIniFileSettingsBackend::WriteSettingRaw(const SettingPath &path, bool val)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), mpt::tfmt::val(val).c_str(), filename.AsNative().c_str());
}

void ImmediateWindowsIniFileSettingsBackend::RemoveSettingRaw(const SettingPath &path)
{
	::WritePrivateProfileString(GetSection(path).c_str(), GetKey(path).c_str(), NULL, filename.AsNative().c_str());
}

void ImmediateWindowsIniFileSettingsBackend::RemoveSectionRaw(const mpt::ustring &section)
{
	::WritePrivateProfileString(mpt::ToWin(section).c_str(), NULL, NULL, filename.AsNative().c_str());
}

ImmediateWindowsIniFileSettingsBackend::ImmediateWindowsIniFileSettingsBackend(mpt::PathString filename_)
	: WindowsIniFileBase(std::move(filename_))
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	return;
}

ImmediateWindowsIniFileSettingsBackend::~ImmediateWindowsIniFileSettingsBackend()
{
	return;
}

SettingValue ImmediateWindowsIniFileSettingsBackend::ReadSetting(const SettingPath &path, const SettingValue &def) const
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
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

void ImmediateWindowsIniFileSettingsBackend::WriteSetting(const SettingPath &path, const SettingValue &val)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
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

void ImmediateWindowsIniFileSettingsBackend::RemoveSetting(const SettingPath &path)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	RemoveSettingRaw(path);
}

void ImmediateWindowsIniFileSettingsBackend::RemoveSection(const mpt::ustring &section)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	RemoveSectionRaw(section);
}



void BatchedWindowsIniFileSettingsBackend::RemoveSectionRaw(const mpt::ustring &section)
{
	::WritePrivateProfileString(mpt::ToWin(section).c_str(), NULL, NULL, filename.AsNative().c_str());
}

BatchedWindowsIniFileSettingsBackend::BatchedWindowsIniFileSettingsBackend(mpt::PathString filename_)
	: WindowsIniFileBase(std::move(filename_))
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	ConvertToUnicode();
	cache = ReadAllSectionsRaw();
}

void BatchedWindowsIniFileSettingsBackend::InvalidateCache()
{
	cache = ReadAllSectionsRaw();
}

BatchedWindowsIniFileSettingsBackend::~BatchedWindowsIniFileSettingsBackend()
{
	return;
}

SettingValue BatchedWindowsIniFileSettingsBackend::ReadSetting(const SettingPath &path, const SettingValue &def) const
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	const auto sectionit = cache.find(path.GetRefSection());
	if(sectionit != cache.end())
	{
		if(sectionit->second.has_value())
		{
			const std::map<mpt::ustring, std::optional<mpt::ustring>> &section = sectionit->second.value();
			const auto it = section.find(path.GetRefKey());
			if(it != section.end())
			{
				if(it->second.has_value())
				{
					return ParseValueFromIni(it->second.value(), def);
				} else
				{
					return def;
				}
			} else
			{
				return def;
			}
		} else
		{
			return def;
		}
	} else
	{
		return def;
	}
}

std::set<mpt::ustring> BatchedWindowsIniFileSettingsBackend::ReadSectionNamesRaw() const
{
	std::set<mpt::ustring> result;
	const std::vector<TCHAR> sectionsstr = [&]()
		{
			std::vector<TCHAR> buf;
			buf.resize(mpt::IO::BUFFERSIZE_SMALL);
			while(true)
			{
				DWORD bufused = ::GetPrivateProfileSectionNames(buf.data(), mpt::saturate_cast<DWORD>(buf.size()), filename.AsNative().c_str());
				if(bufused >= (buf.size() - 2))
				{
					std::size_t newsize = mpt::exponential_grow(buf.size());
					buf.resize(0);
					buf.resize(newsize);
					continue;
				}
				if(bufused >= 1)
				{
					bufused -= 1;  // terminating \0
				}
				buf.resize(bufused);
				break;
			};
			return buf;
		}();
	const std::vector<mpt::winstring> sections = mpt::split(mpt::winstring(sectionsstr.data(), sectionsstr.size()), mpt::winstring(_T("\0"), 1));
	for(const auto &section : sections)
	{
		result.insert(mpt::ToUnicode(section));
	}
	return result;
}

std::map<mpt::ustring, std::optional<mpt::ustring>> BatchedWindowsIniFileSettingsBackend::ReadNamedSectionRaw(const mpt::ustring &section) const
{
	std::map<mpt::ustring, std::optional<mpt::ustring>> result;
	const std::vector<TCHAR> keyvalsstr = [&]()
		{
			std::vector<TCHAR> buf;
			buf.resize(mpt::IO::BUFFERSIZE_SMALL);
			while(true)
			{
				DWORD bufused = ::GetPrivateProfileSection(mpt::ToWin(section).c_str(), buf.data(), mpt::saturate_cast<DWORD>(buf.size()), filename.AsNative().c_str());
				if(bufused >= (buf.size() - 2))
				{
					std::size_t newsize = mpt::exponential_grow(buf.size());
					buf.resize(0);
					buf.resize(newsize);
					continue;
				}
				if(bufused >= 1)
				{
					bufused -= 1;  // terminating \0
				}
				buf.resize(bufused);
				break;
			};
			return buf;
		}();
	const std::vector<mpt::winstring> keyvals = mpt::split(mpt::winstring(keyvalsstr.data(), keyvalsstr.size()), mpt::winstring(_T("\0"), 1));
	for(const auto &keyval : keyvals)
	{
		const auto equalpos = keyval.find(_T("="));
		if(equalpos == mpt::winstring::npos)
		{
			continue;
		}
		if(equalpos == 0)
		{
			continue;
		}
		mpt::winstring key = keyval.substr(0, equalpos);
		mpt::winstring val = keyval.substr(equalpos + 1);
		if(val.length() >= 2)
		{
			if(val[0] == _T('\"') && val[val.length() - 1] == _T('\"'))
			{
				val = val.substr(1, val.length() - 2);
			}
		}
		result.insert(std::make_pair(mpt::ToUnicode(key), std::make_optional(mpt::ToUnicode(val))));
	}
	return result;
}

std::map<mpt::ustring, std::optional<std::map<mpt::ustring, std::optional<mpt::ustring>>>> BatchedWindowsIniFileSettingsBackend::ReadAllSectionsRaw() const
{
	std::map<mpt::ustring, std::optional<std::map<mpt::ustring, std::optional<mpt::ustring>>>> result;
	const std::set<mpt::ustring> sectionnames = ReadSectionNamesRaw();
	for(const mpt::ustring &sectionname : sectionnames)
	{
		result.insert(std::make_pair(sectionname, std::make_optional(ReadNamedSectionRaw(sectionname))));
	}
	return result;
}

void BatchedWindowsIniFileSettingsBackend::WriteSectionRaw(const mpt::ustring &section, const std::map<mpt::ustring, std::optional<mpt::ustring>> &keyvalues)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	mpt::winstring keyvals;
	for(const auto &[key, val] : keyvalues)
	{
		if(val.has_value())
		{
			keyvals.append(mpt::ToWin(key));
			keyvals.append(_T("="));
			keyvals.append(mpt::ToWin(val.value()));
			keyvals.append(mpt::winstring(_T("\0"), 1));
		}
	}
	keyvals.append(mpt::winstring(_T("\0"), 1));
	::WritePrivateProfileSection(mpt::ToWin(section).c_str(), keyvals.c_str(), filename.AsNative().c_str());
}

void BatchedWindowsIniFileSettingsBackend::WriteRemovedSections(const std::set<mpt::ustring> &removeSections)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	for(const auto &section : removeSections)
	{
		cache[section] = std::nullopt;
	}
	for(const auto &[section, keyvalues] : cache)
	{
		if(!keyvalues.has_value())
		{
			RemoveSectionRaw(section);
		}
	}
	for(const auto &section : removeSections)
	{
		cache.erase(section);
	}
}

void BatchedWindowsIniFileSettingsBackend::WriteMultipleSettings(const std::map<SettingPath, std::optional<SettingValue>> &settings)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	std::map<mpt::ustring, std::map<SettingPath, std::optional<SettingValue>>> sectionssettings;
	for(const auto &[path, value] : settings)
	{
		sectionssettings[path.GetRefSection()][path] = value;
	}
	for(const auto &[section, sectionsettings] : sectionssettings)
	{
		if(!cache[section].has_value())
		{
			cache[section].emplace();
		}
		std::map<mpt::ustring, std::optional<mpt::ustring>> &workingsectionsettings = cache[section].value();
		for(const auto &[path, value] : sectionsettings)
		{
			if(value.has_value())
			{
				workingsectionsettings[path.GetRefKey()] = FormatValueAsIni(value.value());
			} else
			{
				workingsectionsettings.erase(path.GetRefKey());
			}
		}
		WriteSectionRaw(section, workingsectionsettings);
	}
}



OPENMPT_NAMESPACE_END
