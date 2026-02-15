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

#include "mpt/base/bit.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/saturate_cast.hpp"
#include "mpt/base/utility.hpp"
#include "mpt/binary/hex.hpp"
#include "mpt/format/message_macros.hpp"
#include "mpt/parse/parse.hpp"
#include "mpt/string/types.hpp"
#include "mpt/textfile/textfile.hpp"

#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <cstddef>



OPENMPT_NAMESPACE_BEGIN



static mpt::ustring DecodeText(mpt::textfile::encoding encoding, mpt::const_byte_span filedata, const mpt::PathString &filename)
{
	switch(encoding.type)
	{
		case mpt::textfile::encoding::type::utf32be:
			if(((filedata.size() - encoding.get_text_offset()) % sizeof(char32_t)) != 0)
			{
				MPT_LOG_GLOBAL(LogError, "Settings", MPT_UFORMAT_MESSAGE("{}: {}")(filename, U_("UTF32 encoding detected, but file size is not a multiple of 4.")));
			}
			break;
		case mpt::textfile::encoding::type::utf32le:
			if(((filedata.size() - encoding.get_text_offset()) % sizeof(char32_t)) != 0)
			{
				MPT_LOG_GLOBAL(LogError, "Settings", MPT_UFORMAT_MESSAGE("{}: {}")(filename, U_("UTF32 encoding detected, but file size is not a multiple of 4.")));
			}
			break;
		case mpt::textfile::encoding::type::utf16be:
			if(((filedata.size() - encoding.get_text_offset()) % sizeof(char16_t)) != 0)
			{
				MPT_LOG_GLOBAL(LogError, "Settings", MPT_UFORMAT_MESSAGE("{}: {}")(filename, U_("UTF16 encoding detected, but file size is not a multiple of 2.")));
			}
			break;
		case mpt::textfile::encoding::type::utf16le:
			if(((filedata.size() - encoding.get_text_offset()) % sizeof(char16_t)) != 0)
			{
				MPT_LOG_GLOBAL(LogError, "Settings", MPT_UFORMAT_MESSAGE("{}: {}")(filename, U_("UTF16 encoding detected, but file size is not a multiple of 2.")));
			}
			break;
		case mpt::textfile::encoding::type::utf8:
			break;
		case mpt::textfile::encoding::type::locale:
			break;
	}
	return mpt::textfile::decode(encoding, filedata);
}

static mpt::ustring DecodeText(mpt::const_byte_span filedata, const mpt::PathString &filename)
{
	return DecodeText(mpt::textfile::probe_encoding(filedata), filedata, filename);
}



IniFileBase::IniFileBase(mpt::PathString filename_, std::optional<Caching> sync_hint)
	: filename(std::move(filename_))
	, file(filename)
	, sync_default(sync_hint)
{
	return;
}

const mpt::PathString &IniFileBase::Filename() const
{
	return filename;
}

mpt::PathString IniFileBase::GetFilename() const
{
	return filename;
}

void IniFileBase::MakeBackup(const mpt::PathString &tag, std::optional<Caching> sync_hint)
{
	MPT_UNUSED(sync_hint);
	const mpt::PathString backupFilename = filename + P_(".") + tag + P_(".bak");
	CopyFile(mpt::support_long_path(filename.AsNative()).c_str(), mpt::support_long_path(backupFilename.AsNative()).c_str(), FALSE);
}



WindowsIniFileBase::WindowsIniFileBase(mpt::PathString filename_, std::optional<Caching> sync_hint)
	: IniFileBase(std::move(filename_), sync_hint)
{
	return;
}

void WindowsIniFileBase::ConvertToUnicode(std::optional<Caching> sync_hint)
{
	// Force ini file to be encoded in UTF16LEBOM or UTF8BOM (on Wine).
	// This causes WINAPI ini file functions to keep it in Unicode encoding
	// and thus support storing unicode strings uncorrupted.
	// This is backwards compatible because even ANSI WINAPI behaves the
	// same way in this case.
	// Do not convert when running on Windows 2000 or earlier
	// because of missing Unicode support on Win9x,
	{
		std::shared_lock l{file};
		const std::vector<std::byte> filedata = file.read();
		const mpt::textfile::encoding desired_encoding = mpt::textfile::get_preferred_encoding();
		const mpt::textfile::encoding current_encoding = mpt::textfile::probe_encoding(filedata);
		if(current_encoding == desired_encoding)
		{
			return;
		}
		mpt::PathString backupTag{};
		switch(current_encoding.type)
		{
			case mpt::textfile::encoding::type::utf32be:
				backupTag = P_("utf32be");
				break;
			case mpt::textfile::encoding::type::utf32le:
				backupTag = P_("utf32le");
				break;
			case mpt::textfile::encoding::type::utf16be:
				backupTag = P_("utf16be");
				break;
			case mpt::textfile::encoding::type::utf16le:
				backupTag = P_("utf16le");
				break;
			case mpt::textfile::encoding::type::utf8:
				backupTag = P_("utf8");
				break;
			case mpt::textfile::encoding::type::locale:
				backupTag = P_("locale");
				break;
		}
		MakeBackup(backupTag, sync_hint);
	}
	{
		std::unique_lock l{file};
		const std::vector<std::byte> filedata = file.read();
		const mpt::textfile::encoding desired_encoding = mpt::textfile::get_preferred_encoding();
		static_assert(sizeof(wchar_t) == 2);
		MPT_ASSERT(mpt::endian_is_little());
		file.write(mpt::textfile::encode(desired_encoding, DecodeText(filedata, filename)), sync_hint.value_or(sync_default.value_or(Caching::WriteBack)) == Caching::WriteThrough);
	}
}

mpt::winstring WindowsIniFileBase::GetSection(const SettingPath &path)
{
	return mpt::ToWin(path.GetSection());
}

mpt::winstring WindowsIniFileBase::GetKey(const SettingPath &path)
{
	return mpt::ToWin(path.GetKey());
}



IniVersion IniFileHelpers::ProbeVersion(const std::vector<mpt::ustring> &lines)
{
	IniVersion result;
	if(!((lines.size() > 0) && (lines[0] == MPT_ULITERAL("[!Type]"))))
	{
		return result;
	}
	if(!((lines.size() > 1) && (lines[1] == MPT_ULITERAL("!Format=org.openmpt.fileformat.ini"))))
	{
		return result;
	}
	// assume our version from now
	result.major = 4;
	result.minor = 0;
	result.patch = 0;
	if(!(lines.size() > 4))
	{
		return result;
	}
	std::vector<mpt::ustring> linemajor = mpt::split<mpt::ustring>(lines[2], U_("="));
	std::vector<mpt::ustring> lineminor = mpt::split<mpt::ustring>(lines[3], U_("="));
	std::vector<mpt::ustring> linepatch = mpt::split<mpt::ustring>(lines[4], U_("="));
	if((linemajor.size() != 2) || (linemajor[0] != U_("!VersionMajor")))
	{
		return result;
	}
	if((lineminor.size() != 2) || (lineminor[0] != U_("!VersionMinor")))
	{
		return result;
	}
	if((linepatch.size() != 2) || (linepatch[0] != U_("!VersionPatch")))
	{
		return result;
	}
	result.major = mpt::parse<uint8>(linemajor[1]);
	result.minor = mpt::parse<uint8>(lineminor[1]);
	result.patch = mpt::parse<uint8>(linepatch[1]);
	return result;
}

std::list<std::pair<SettingPath, SettingValue>> IniFileHelpers::CreateIniHeader(IniVersion version)
{
	std::list<std::pair<SettingPath, SettingValue>> result;
	switch(version.major)
	{
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			result.emplace_back(std::make_pair(SettingPath{U_("!Type"), U_("!Format")}, U_("org.openmpt.fileformat.ini")));
			result.emplace_back(std::make_pair(SettingPath{U_("!Type"), U_("!VersionMajor")}, static_cast<int32>(version.major)));
			result.emplace_back(std::make_pair(SettingPath{U_("!Type"), U_("!VersionMinor")}, static_cast<int32>(version.minor)));
			result.emplace_back(std::make_pair(SettingPath{U_("!Type"), U_("!VersionPatch")}, static_cast<int32>(version.patch)));
			break;
	}
	return result;
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

ImmediateWindowsIniFileSettingsBackend::ImmediateWindowsIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint)
	: WindowsIniFileBase(std::move(filename_), sync_hint)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	ConvertToUnicode(sync_hint);
}

ImmediateWindowsIniFileSettingsBackend::~ImmediateWindowsIniFileSettingsBackend()
{
	return;
}

CaseSensitivity ImmediateWindowsIniFileSettingsBackend::GetCaseSensitivity() const
{
	return CaseSensitivity::Insensitive;
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

void ImmediateWindowsIniFileSettingsBackend::Sync(std::optional<Caching> sync_hint)
{
	if(sync_hint.value_or(sync_default.value_or(Caching::WriteThrough)) == Caching::WriteThrough)
	{
		file.sync();
	}
}



void BatchedWindowsIniFileSettingsBackend::RemoveSectionRaw(const mpt::ustring &section)
{
	::WritePrivateProfileString(mpt::ToWin(section).c_str(), NULL, NULL, filename.AsNative().c_str());
}

BatchedWindowsIniFileSettingsBackend::BatchedWindowsIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint)
	: WindowsIniFileBase(std::move(filename_), sync_hint)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	ConvertToUnicode(sync_hint);
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

CaseSensitivity BatchedWindowsIniFileSettingsBackend::GetCaseSensitivity() const
{
	return CaseSensitivity::Sensitive;
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
			} else if(val[0] == _T('\'') && val[val.length() - 1] == _T('\''))
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

void BatchedWindowsIniFileSettingsBackend::Sync(std::optional<Caching> sync_hint)
{
	if(sync_hint.value_or(sync_default.value_or(Caching::WriteThrough)) == Caching::WriteThrough)
	{
		file.sync();
	}
}


#if MPT_USTRING_MODE_UTF8

// We cannot escape natively in UTF8 because ^x00 escapes would be different.
// This has a 1.5x cost in ustring UTF8 mode, but luckily we do not care
// much at the moment.

static inline bool is_hex(char32_t c)
{
	return (U'0' <= c && c <= U'9') || (U'a' <= c && c <= U'f') || (U'A' <= c && c <= U'F');
}

static mpt::ustring escape(mpt::ustring_view text_)
{
	std::u32string text = mpt::transcode<std::u32string>(text_);
	std::u32string result;
	const std::size_t len = text.length();
	result.reserve(len);
	for(std::size_t pos = 0; pos < len; ++pos)
	{
		const char32_t c = text[pos];
		if((c == U'^') || (c == U';') || (c == U'[') || (c == U']') || (c == U'=') || (c == U'\"') || (c == U'\''))
		{
			result.push_back(U'^');
			result.push_back(c);
		} else if(mpt::char_value(c) < 32)
		{
			result.push_back(U'^');
			result.push_back(U'x');
			result.append(mpt::fmtT<std::u32string>::hex0<2>(mpt::char_value(c)));
		} else if((0x80 <= mpt::char_value(c)) && (mpt::char_value(c) <= 0x9f))
		{
			result.push_back(U'^');
			result.push_back(U'x');
			result.append(mpt::fmtT<std::u32string>::hex0<2>(mpt::char_value(c)));
		} else
		{
			result.push_back(c);
		}
	}
	return mpt::transcode<mpt::ustring>(result);
}

static mpt::ustring unescape(mpt::ustring_view text_)
{
	std::u32string text = mpt::transcode<std::u32string>(text_);
	std::u32string result;
	const std::size_t len = text.length();
	result.reserve(len);
	for(std::size_t pos = 0; pos < len; ++pos)
	{
		std::size_t rem = len - pos;
		if(text[pos] == U'^')
		{
			if((rem >= 2) && (text[pos+1] == U'^'))
			{
				result.push_back(U'^');
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == U';'))
			{
				result.push_back(U';');
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == U'['))
			{
				result.push_back(U'[');
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == U']'))
			{
				result.push_back(U']');
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == U'='))
			{
				result.push_back(U'=');
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == U'\"'))
			{
				result.push_back(U'\"');
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == U'\''))
			{
				result.push_back(U'\'');
				pos += 1;
			} else if((rem >= 4) && (text[pos+1] == U'x') && is_hex(text[pos+2]) && is_hex(text[pos+3]))
			{
				result.append(std::u32string(1, static_cast<char32_t>(mpt::parse_hex<uint8>(std::u32string{text.substr(pos + 2, 4)}))));
				pos += 3;
			} else if((rem >= 6) && (text[pos+1] == U'u') && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]))
			{
				result.append(std::u32string(1, static_cast<char32_t>(mpt::parse_hex<uint16>(std::u32string{text.substr(pos + 2, 4)}))));
				pos += 5;
			} else if((rem >= 10) && (text[pos+1] == U'U') && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]) && is_hex(text[pos+6]) && is_hex(text[pos+7]) && is_hex(text[pos+8]) && is_hex(text[pos+9]))
			{
				result.append(std::u32string(1, static_cast<char32_t>(mpt::parse_hex<uint32>(std::u32string{text.substr(pos + 2, 8)}))));
				pos += 9;
			} else
			{
				result.push_back(text[pos]);
			}
		} else
		{
			result.push_back(text[pos]);
		}
	}
	return mpt::transcode<mpt::ustring>(result);
}

#else

static inline bool is_hex(mpt::uchar c)
{
	return (MPT_UCHAR('0') <= c && c <= MPT_UCHAR('9')) || (MPT_UCHAR('a') <= c && c <= MPT_UCHAR('f')) || (MPT_UCHAR('A') <= c && c <= MPT_UCHAR('F'));
}

static mpt::ustring escape(mpt::ustring_view text)
{
	mpt::ustring result;
	const std::size_t len = text.length();
	result.reserve(len);
	for(std::size_t pos = 0; pos < len; ++pos)
	{
		const mpt::uchar c = text[pos];
		if((c == MPT_UCHAR('^')) || (c == MPT_UCHAR(';')) || (c == MPT_UCHAR('[')) || (c == MPT_UCHAR(']')) || (c == MPT_UCHAR('=')) || (c == MPT_UCHAR('\"')) || (c == MPT_UCHAR('\'')))
		{
			result.push_back(MPT_UCHAR('^'));
			result.push_back(c);
		} else if(mpt::char_value(c) < 32)
		{
			result.push_back(MPT_UCHAR('^'));
			result.push_back(MPT_UCHAR('x'));
			result.append(mpt::ufmt::hex0<2>(mpt::char_value(c)));
		} else if((0x80 <= mpt::char_value(c)) && (mpt::char_value(c) <= 0x9f))
		{
			result.push_back(MPT_UCHAR('^'));
			result.push_back(MPT_UCHAR('x'));
			result.append(mpt::ufmt::hex0<2>(mpt::char_value(c)));
		} else
		{
			result.push_back(c);
		}
	}
	return result;
}

static mpt::ustring unescape(mpt::ustring_view text)
{
	mpt::ustring result;
	const std::size_t len = text.length();
	result.reserve(len);
	for(std::size_t pos = 0; pos < len; ++pos)
	{
		std::size_t rem = len - pos;
		if(text[pos] == MPT_UCHAR('^'))
		{
			if((rem >= 2) && (text[pos+1] == MPT_UCHAR('^')))
			{
				result.push_back(MPT_UCHAR('^'));
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR(';')))
			{
				result.push_back(MPT_UCHAR(';'));
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('[')))
			{
				result.push_back(MPT_UCHAR('['));
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR(']')))
			{
				result.push_back(MPT_UCHAR(']'));
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('=')))
			{
				result.push_back(MPT_UCHAR('='));
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('\"')))
			{
				result.push_back(MPT_UCHAR('\"'));
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('\'')))
			{
				result.push_back(MPT_UCHAR('\''));
				pos += 1;
			} else if((rem >= 4) && (text[pos+1] == MPT_UCHAR('x')) && is_hex(text[pos+2]) && is_hex(text[pos+3]))
			{
				result.push_back(static_cast<mpt::uchar>(mpt::parse_hex<uint8>(mpt::ustring{text.substr(pos + 2, 2)})));
				pos += 3;
			} else if((rem >= 6) && (text[pos+1] == MPT_UCHAR('u')) && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]))
			{
				result.append(mpt::transcode<mpt::ustring>(std::u16string(1, static_cast<char16_t>(mpt::parse_hex<uint16>(mpt::ustring{text.substr(pos + 2, 4)})))));
				pos += 5;
			} else if((rem >= 10) && (text[pos+1] == MPT_UCHAR('U')) && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]) && is_hex(text[pos+6]) && is_hex(text[pos+7]) && is_hex(text[pos+8]) && is_hex(text[pos+9]))
			{
				result.append(mpt::transcode<mpt::ustring>(std::u32string(1, static_cast<char32_t>(mpt::parse_hex<uint32>(mpt::ustring{text.substr(pos + 2, 8)})))));
				pos += 9;
			} else
			{
				result.push_back(text[pos]);
			}
		} else
		{
			result.push_back(text[pos]);
		}
	}
	return result;
}

#endif

static inline std::size_t find_unescaped_first(mpt::ustring_view text, mpt::uchar c)
{
	const std::size_t len = text.length();
	for(std::size_t pos = 0; pos < len; ++pos)
	{
		if(text[pos] == c)
		{
			return pos;
		}
		std::size_t rem = len - pos;
		if(text[pos] == MPT_UCHAR('^'))
		{
			if((rem >= 2) && (text[pos+1] == MPT_UCHAR('^')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR(';')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('[')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR(']')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('=')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('\"')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('\'')))
			{
				pos += 1;
			} else if((rem >= 4) && (text[pos+1] == MPT_UCHAR('x')) && is_hex(text[pos+2]) && is_hex(text[pos+3]))
			{
				pos += 3;
			} else if((rem >= 6) && (text[pos+1] == MPT_UCHAR('u')) && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]))
			{
				pos += 5;
			} else if((rem >= 10) && (text[pos+1] == MPT_UCHAR('U')) && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]) && is_hex(text[pos+6]) && is_hex(text[pos+7]) && is_hex(text[pos+8]) && is_hex(text[pos+9]))
			{
				pos += 9;
			}
		}
	}
	return mpt::ustring_view::npos;
}

static inline std::size_t find_unescaped_last(mpt::ustring_view text, mpt::uchar c)
{
	std::size_t result = mpt::ustring_view::npos;
	const std::size_t len = text.length();
	for(std::size_t pos = 0; pos < len; ++pos)
	{
		if(text[pos] == c)
		{
			result = pos;
		}
		std::size_t rem = len - pos;
		if(text[pos] == MPT_UCHAR('^'))
		{
			if((rem >= 2) && (text[pos+1] == MPT_UCHAR('^')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR(';')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('[')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR(']')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('=')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('\"')))
			{
				pos += 1;
			} else if((rem >= 2) && (text[pos+1] == MPT_UCHAR('\'')))
			{
				pos += 1;
			} else if((rem >= 4) && (text[pos+1] == MPT_UCHAR('x')) && is_hex(text[pos+2]) && is_hex(text[pos+3]))
			{
				pos += 3;
			} else if((rem >= 6) && (text[pos+1] == MPT_UCHAR('u')) && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]))
			{
				pos += 5;
			} else if((rem >= 10) && (text[pos+1] == MPT_UCHAR('U')) && is_hex(text[pos+2]) && is_hex(text[pos+3]) && is_hex(text[pos+4]) && is_hex(text[pos+5]) && is_hex(text[pos+6]) && is_hex(text[pos+7]) && is_hex(text[pos+8]) && is_hex(text[pos+9]))
			{
				pos += 9;
			}
		}
	}
	return result;
}

struct line_endings {
#if MPT_CXX_BEFORE(20)
	uint8 CRLF = 1;  // (Atari, DOS, OS/2, Windows)
	uint8 LFCR = 1;  // (Acorn)
	uint8 LF   = 1;  // U+0000'000A LINE FEED (Amiga, POSIX, Unix)
	uint8 VT   = 0;  // U+0000'000B VERTICAL TAB
	uint8 FF   = 0;  // U+0000'000C FORM FEED
	uint8 CR   = 1;  // U+0000'000D CARRIAGE RETURN (C64, Mac OS Classic)
	uint8 RS   = 0;  // U+0000'001E RECORD SEPARATOR (QNX < v4)
	uint8 NEL  = 0;  // U+0000'0085 NEXT LINE (z/OS)
	uint8 LS   = 0;  // U+0000'2028 LINE SEPARATOR
	uint8 PS   = 0;  // U+0000'2029 PARAGRAPH SEPARATOR
#else
	uint8 CRLF : 1 = 1;  // (Atari, DOS, OS/2, Windows)
	uint8 LFCR : 1 = 1;  // (Acorn)
	uint8 LF   : 1 = 1;  // U+0000'000A LINE FEED (Amiga, POSIX, Unix)
	uint8 VT   : 1 = 0;  // U+0000'000B VERTICAL TAB
	uint8 FF   : 1 = 0;  // U+0000'000C FORM FEED
	uint8 CR   : 1 = 1;  // U+0000'000D CARRIAGE RETURN (C64, Mac OS Classic)
	uint8 RS   : 1 = 0;  // U+0000'001E RECORD SEPARATOR (QNX < v4)
	uint8 NEL  : 1 = 0;  // U+0000'0085 NEXT LINE (z/OS)
	uint8 LS   : 1 = 0;  // U+0000'2028 LINE SEPARATOR
	uint8 PS   : 1 = 0;  // U+0000'2029 PARAGRAPH SEPARATOR
#endif
	[[nodiscard]] inline constexpr static line_endings Windows() noexcept
	{
		return {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	}
	[[nodiscard]] inline constexpr static line_endings Acorn() noexcept
	{
		return {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	}
	[[nodiscard]] inline constexpr static line_endings Unix() noexcept
	{
		return {0, 0, 0, 0, 0, 1, 0, 0, 0, 0};
	}
	[[nodiscard]] inline constexpr static line_endings Mac() noexcept
	{
		return {0, 0, 0, 0, 0, 1, 0, 0, 0, 0};
	}
	[[nodiscard]] inline constexpr static line_endings AnyASCII() noexcept
	{
		return {1, 1, 1, 0, 0, 1, 0, 0, 0, 0};
	}
	[[nodiscard]] inline constexpr static line_endings AnyUnicode() noexcept
	{
		return {1, 1, 1, 1, 1, 1, 0, 1, 1, 1};
	}
	[[nodiscard]] inline constexpr static line_endings EBCDIC() noexcept
	{
		return {0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
	}
	[[nodiscard]] inline constexpr static line_endings QNX() noexcept
	{
		return {0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
	}
	[[nodiscard]] friend inline constexpr line_endings operator|(line_endings a, line_endings b) noexcept
	{
		return {
			static_cast<uint8>(a.CRLF | b.CRLF),
			static_cast<uint8>(a.LFCR | b.LFCR),
			static_cast<uint8>(a.LF   | b.LF  ),
			static_cast<uint8>(a.VT   | b.VT  ),
			static_cast<uint8>(a.FF   | b.FF  ),
			static_cast<uint8>(a.CR   | b.CR  ),
			static_cast<uint8>(a.RS   | b.RS  ),
			static_cast<uint8>(a.NEL  | b.NEL ),
			static_cast<uint8>(a.LS   | b.LS  ),
			static_cast<uint8>(a.PS   | b.PS  )
		};
	}
};

static inline std::vector<mpt::ustring> split_lines(mpt::ustring_view text, line_endings sep = line_endings::AnyASCII())
{
	std::vector<mpt::ustring> result;
	const std::size_t len = text.length();
	std::size_t beg = 0;
	for(std::size_t pos = 0; pos < len; ++pos)
	{
		std::size_t rem = len - pos;
		if(sep.CRLF && (rem >= 2) && (text[pos+0] == MPT_UCHAR('\r')) && (text[pos+1] == MPT_UCHAR('\n')))
		{
			result.emplace_back(text.substr(beg, pos - beg));
			pos += 2;
			beg = pos;
		} else if(sep.LFCR && (rem >= 2) && (text[pos+0] == MPT_UCHAR('\n')) && (text[pos+1] == MPT_UCHAR('\r')))
		{
			result.emplace_back(text.substr(beg, pos - beg));
			pos += 2;
			beg = pos;
		} else if(sep.LF && (rem >= 1) && text[pos+0] == MPT_UCHAR('\n'))
		{ 
			result.emplace_back(text.substr(beg, pos - beg));
			pos += 1;
			beg = pos;
		} else if(sep.VT && (rem >= 1) && text[pos+0] == MPT_UCHAR('\v'))
		{ 
			result.emplace_back(text.substr(beg, pos - beg));
			pos += 1;
			beg = pos;
		} else if(sep.FF && (rem >= 1) && text[pos+0] == MPT_UCHAR('\f'))
		{ 
			result.emplace_back(text.substr(beg, pos - beg));
			pos += 1;
			beg = pos;
		} else if(sep.CR && (rem >= 1) && text[pos+0] == MPT_UCHAR('\r'))
		{ 
			result.emplace_back(text.substr(beg, pos - beg));
			pos += 1;
			beg = pos;
		} else if(sep.RS && (rem >= 1) && text[pos+0] == MPT_UCHAR('\x1E'))
		{ 
			result.emplace_back(text.substr(beg, pos - beg));
			pos += 1;
			beg = pos;
		} else
		{
#if MPT_USTRING_MODE_UTF8
			if(sep.NEL && (rem >= 1) && text.find(MPT_UTF8("\xc2\x85"), 0) == 0)
			{ 
				result.emplace_back(text.substr(beg, pos - beg));
				pos += 1;
				beg = pos;
			} else if(sep.LS && (rem >= 1) && text.find(MPT_UTF8("\xe2\x80\xa8"), 0) == 0)
			{ 
				result.emplace_back(text.substr(beg, pos - beg));
				pos += 1;
				beg = pos;
			} else if(sep.PS && (rem >= 1) && text.find(MPT_UTF8("\xe2\x80\xa9"), 0) == 0)
			{ 
				result.emplace_back(text.substr(beg, pos - beg));
				pos += 1;
				beg = pos;
			}
#else
			if constexpr(sizeof(mpt::uchar) >= 4)
			{
				if(sep.NEL && (rem >= 1) && text[pos+0] == MPT_UCHAR('\U00000085'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 1;
					beg = pos;
				} else if(sep.LS && (rem >= 1) && text[pos+0] == MPT_UCHAR('\U00002028'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 1;
					beg = pos;
				} else if(sep.PS && (rem >= 1) && text[pos+0] == MPT_UCHAR('\U00002029'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 1;
					beg = pos;
				}
			} else if constexpr(sizeof(mpt::uchar) >= 2)
			{
				if(sep.NEL && (rem >= 1) && text[pos+0] == MPT_UCHAR('\u0085'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 1;
					beg = pos;
				} else if(sep.LS && (rem >= 1) && text[pos+0] == MPT_UCHAR('\u2028'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 1;
					beg = pos;
				} else if(sep.PS && (rem >= 1) && text[pos+0] == MPT_UCHAR('\u2029'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 1;
					beg = pos;
				}
			} else
			{
				if(sep.NEL && (rem >= 2) && text[pos+0] == MPT_UCHAR('\xc2') && text[pos+1] == MPT_UCHAR('\x85'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 2;
					beg = pos;
				} else if(sep.LS && (rem >= 3) && text[pos+0] == MPT_UCHAR('\xe2') && text[pos+1] == MPT_UCHAR('\x80') && text[pos+3] == MPT_UCHAR('\xa8'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 3;
					beg = pos;
				} else if(sep.PS && (rem >= 3) && text[pos+0] == MPT_UCHAR('\xe2') && text[pos+1] == MPT_UCHAR('\x80') && text[pos+2] == MPT_UCHAR('\xa9'))
				{ 
					result.emplace_back(text.substr(beg, pos - beg));
					pos += 3;
					beg = pos;
				}
			}
#endif
		}
	}
	if(!result.empty() || ((len - beg) > 0))
	{
		result.emplace_back(text.substr(beg, len - beg));
	}
	return result;
}

static inline mpt::ustring_view trim_whitespace(mpt::ustring_view text)
{
	mpt::ustring_view::size_type beg_pos = text.find_first_not_of(MPT_ULITERAL(" \t\r\n"));
	if(beg_pos != mpt::ustring_view::npos)
	{
		text = text.substr(beg_pos);
	} else
	{
		text = text.substr(text.length(), 0);
	}
	mpt::ustring_view::size_type end_pos = text.find_last_not_of(MPT_ULITERAL(" \t\r\n"));
	if(end_pos != mpt::ustring_view::npos)
	{
		text = text.substr(0, end_pos + 1);
	} else
	{
		text = text.substr(0, 0);
	}
	return text;
}

static inline mpt::ustring_view remove_quotes(mpt::ustring_view text)
{
	if(text.length() <= 1)
	{
		return text;
	}
	{
		mpt::ustring_view::size_type beg_quote = find_unescaped_first(text, MPT_UCHAR('\"'));
		mpt::ustring_view::size_type end_quote = find_unescaped_last(text, MPT_UCHAR('\"'));
		if((beg_quote == 0) && (end_quote == (text.length() - 1)))
		{
			return text.substr(1, text.length() - 2);
		}
	}
	{
		mpt::ustring_view::size_type beg_quote = find_unescaped_first(text, MPT_UCHAR('\''));
		mpt::ustring_view::size_type end_quote = find_unescaped_last(text, MPT_UCHAR('\''));
		if((beg_quote == 0) && (end_quote == (text.length() - 1)))
		{
			return text.substr(1, text.length() - 2);
		}
	}
	return text;
}

#if MPT_SETTINGS_INI_CASE_INSENSITIVE
static mpt::ustring lowercase(mpt::ustring_view text)
{
	return mpt::ToLowerCaseAscii(text);
}
#endif

void CachedIniFileSettingsBackend::ReadFileIntoCache()
{
	ReadLinesIntoCache(ReadFileAsLines());
}

std::vector<mpt::ustring> CachedIniFileSettingsBackend::ReadFileAsLines()
{
	return split_lines(DecodeText(file.read(), filename));;
}

void CachedIniFileSettingsBackend::ReadLinesIntoCache(const std::vector<mpt::ustring> &lines)
{
	ReadLinesIntoCache(ProbeVersion(lines), lines);
}

void CachedIniFileSettingsBackend::ReadLinesIntoCache(IniVersion version, const std::vector<mpt::ustring> &lines)
{
	cache.clear();
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	if(case_sensitivity == CaseSensitivity::Insensitive)
	{
		casemap.clear();
	}
#endif
	mpt::ustring current_section;
	mpt::ustring last_key;
	bool empty_line_after_last_key = false;
	std::vector<mpt::ustring> running_comments;
	std::size_t line_number = 0;
	bool last_line_empty = false;
	const bool quirk_no_unescape = (version.major < 4);
	auto store_previous_comments = [&]()
		{
			if(!empty_line_after_last_key && (running_comments.size() > 0))
			{
				comments[std::make_pair(current_section, last_key)].after = std::move(running_comments);
				running_comments = {};
			}
		};
	for(const auto &raw_line : lines)
	{
		line_number += 1;
		last_line_empty = (raw_line.length() == 0);
		mpt::ustring_view line = trim_whitespace(raw_line);
		std::size_t line_len = line.length();
		if(line_len == 0)
		{ // empty line
			store_previous_comments();
			empty_line_after_last_key = true;
			continue;
		}
		if(find_unescaped_first(line, MPT_UCHAR(';')) == 0)
		{ // comment
			running_comments.push_back(mpt::ustring{trim_whitespace(line.substr(1))});
			continue;
		}
		if(find_unescaped_first(line, MPT_UCHAR('[')) == 0)
		{ // section start
			mpt::ustring_view::size_type opening_pos = 0;
			mpt::ustring_view::size_type closing_pos = find_unescaped_first(line, MPT_UCHAR(']'));
			if(closing_pos == mpt::ustring_view::npos)
			{ // syntax error
				store_previous_comments();
				empty_line_after_last_key = true;
				MPT_LOG_GLOBAL(LogError, "Settings", MPT_UFORMAT_MESSAGE("{}({}): No closing bracket in section header. ('{}').")(filename, line_number, raw_line));
				continue;
			}
			mpt::ustring_view remainder = line.substr(closing_pos + 1);
			if(trim_whitespace(remainder).length() > 0)
			{ // syntax error
				MPT_LOG_GLOBAL(LogWarning, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Junk after section header. ('{}').")(filename, line_number, raw_line));
			}
			mpt::ustring_view escaped_section = trim_whitespace(line.substr(opening_pos + 1, closing_pos - opening_pos - 1));
			mpt::ustring section = (quirk_no_unescape ? mpt::ustring{escaped_section} : unescape(escaped_section));
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
			if(case_sensitivity == CaseSensitivity::Insensitive)
			{
				if(casemap.find(std::make_pair(lowercase(section), mpt::ustring{})) != casemap.end())
				{
					store_previous_comments();
					empty_line_after_last_key = true;
					current_section.clear();
					MPT_LOG_GLOBAL(LogWarning, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Ignoring duplicate casefolded section '{}'. ('{}').")(filename, line_number, escape(section), raw_line));
					continue;
				}
				casemap.emplace(std::make_pair(lowercase(section), mpt::ustring{}), std::make_pair(section, mpt::ustring{}));
			}
#endif
			if(cache.find(section) != cache.end())
			{ // semantic error
				store_previous_comments();
				empty_line_after_last_key = true;
				current_section.clear();
				MPT_LOG_GLOBAL(LogWarning, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Ignoring duplicate section '{}'. ('{}').")(filename, line_number, escape(section), raw_line));
				continue;
			}
			MPT_LOG_GLOBAL(LogInformation, "Settings", MPT_UFORMAT_MESSAGE("{}({}): [{}]")(filename, line_number, escape(section)));
			comments[std::make_pair(section, mpt::ustring{})].before = std::move(running_comments);
			running_comments = {};
			cache[section].emplace();
			current_section = std::move(section);
			last_key.clear();
			empty_line_after_last_key = false;
			continue;
		}
		mpt::ustring_view::size_type equal_pos = find_unescaped_first(line, MPT_UCHAR('='));
		if(equal_pos == mpt::ustring_view::npos)
		{ // syntax error
			store_previous_comments();
			empty_line_after_last_key = true;
			MPT_LOG_GLOBAL(LogError, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Invalid token. ('{}').")(filename, line_number, raw_line));
			continue;
		}
		// key value
		if(current_section.empty())
		{ // syntax error
			store_previous_comments();
			empty_line_after_last_key = true;
			MPT_LOG_GLOBAL(LogWarning, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Ignoring key-value without section. ('{}').")(filename, line_number, raw_line));
			continue;
		}
		mpt::ustring_view escaped_key = trim_whitespace(line.substr(0, equal_pos));
		mpt::ustring_view escaped_val = remove_quotes(trim_whitespace(line.substr(equal_pos + 1)));
		if(escaped_key.empty())
		{ // semantic error
			store_previous_comments();
			empty_line_after_last_key = true;
			MPT_LOG_GLOBAL(LogError, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Empty key. ('{}').")(filename, line_number, raw_line));
			continue;
		}
		mpt::ustring key = (quirk_no_unescape ? mpt::ustring{escaped_key} : unescape(escaped_key));
		mpt::ustring val = (quirk_no_unescape ? mpt::ustring{escaped_val} : unescape(escaped_val));
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
		if(case_sensitivity == CaseSensitivity::Insensitive)
		{
			if(casemap.find(std::make_pair(lowercase(current_section), lowercase(key))) != casemap.end())
			{
				store_previous_comments();
				empty_line_after_last_key = true;
				MPT_LOG_GLOBAL(LogWarning, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Ignoring duplicate casefolded key '{}'. ('{}').")(filename, line_number, escape(key), raw_line));
				continue;
			}
		}
#endif
		if(cache[current_section].value().find(key) != cache[current_section].value().end())
		{ // semantic error
			store_previous_comments();
			empty_line_after_last_key = true;
			MPT_LOG_GLOBAL(LogWarning, "Settings", MPT_UFORMAT_MESSAGE("{}({}): Ignoring duplicate key '{}'. ('{}').")(filename, line_number, escape(key), raw_line));
			continue;
		}
		MPT_LOG_GLOBAL(LogInformation, "Settings", MPT_UFORMAT_MESSAGE("{}({}): {}={}")(filename, line_number, escape(key), escape(val)));
		comments[std::make_pair(current_section, key)].before = std::move(running_comments);
		running_comments = {};
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
		if(case_sensitivity == CaseSensitivity::Insensitive)
		{
			casemap.emplace(std::make_pair(lowercase(current_section), lowercase(key)), std::make_pair(current_section, key));
		}
#endif
		cache[current_section].value()[key] = std::make_optional(std::move(val));
		last_key = std::move(key);
		empty_line_after_last_key = false;
	}
	store_previous_comments();
	if(!last_line_empty)
	{
		MPT_LOG_GLOBAL(LogNotification, "Settings", MPT_UFORMAT_MESSAGE("{}({}): {}")(filename, line_number, U_("No newline after last line.")));
	}
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	if(case_sensitivity == CaseSensitivity::Insensitive)
	{
		VerifyCasemap();
	}
#endif
}

void CachedIniFileSettingsBackend::MergeSettingsIntoCache(const std::set<mpt::ustring> &removeSections, const std::map<SettingPath, std::optional<SettingValue>> &settings)
{
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	if(case_sensitivity == CaseSensitivity::Insensitive)
	{
		for(const auto &section : removeSections)
		{
			const auto mapping = casemap.find(std::make_pair(lowercase(section), mpt::ustring{}));
			if(mapping != casemap.end())
			{
				if(mapping->second.first != section)
				{
					const auto entry = cache.find(mapping->second.first);
					if(entry != cache.end())
					{
						auto node = cache.extract(entry);
						node.key() = section;
						cache.insert(std::move(node));
						auto casenode = casemap.extract(mapping);
						mapping->second.first = section;
					}
				}
			}
		}
	}
#endif
	for(const auto &section : removeSections)
	{
		cache.erase(section);
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
		if(case_sensitivity == CaseSensitivity::Insensitive)
		{
			casemap.erase(std::make_pair(lowercase(section), mpt::ustring{}));
		}
#endif
	}
	std::map<mpt::ustring, std::map<SettingPath, std::optional<SettingValue>>> sectionssettings;
	for(const auto &[path, value] : settings)
	{
		sectionssettings[path.GetRefSection()][path] = value;
	}
	for(const auto &[section, sectionsettings] : sectionssettings)
	{
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
		if(case_sensitivity == CaseSensitivity::Insensitive)
		{
			const auto mapping = casemap.find(std::make_pair(lowercase(section), mpt::ustring{}));
			if(mapping == casemap.end())
			{
				casemap.emplace(std::make_pair(lowercase(section), mpt::ustring{}), std::make_pair(section, mpt::ustring{}));
			} else
			{
				if(mapping->second.first != section)
				{
					const auto entry = cache.find(mapping->second.first);
					if(entry != cache.end())
					{
						auto node = cache.extract(entry);
						node.key() = section;
						cache.insert(std::move(node));
						auto casenode = casemap.extract(mapping);
						mapping->second.first = section;
					}
				}
			}
		}
#endif
		if(!cache[section].has_value())
		{
			cache[section].emplace();
		}
		std::map<mpt::ustring, std::optional<mpt::ustring>> &sectioncache = cache[section].value();
		for(const auto &[path, value] : sectionsettings)
		{
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
			if(case_sensitivity == CaseSensitivity::Insensitive)
			{
				const auto mapping = casemap.find(std::make_pair(lowercase(section), lowercase(path.GetRefKey())));
				if(mapping == casemap.end())
				{
					casemap.emplace(std::make_pair(lowercase(section), lowercase(path.GetRefKey())), std::make_pair(section, path.GetRefKey()));
				} else
				{
					if(mapping->second.second != path.GetRefKey())
					{
						const auto entry = sectioncache.find(mapping->second.second);
						if(entry != sectioncache.end())
						{
							auto node = sectioncache.extract(entry);
							node.key() = path.GetRefKey();
							sectioncache.insert(std::move(node));
							auto casenode = casemap.extract(mapping);
							mapping->second.second = path.GetRefKey();
						}
					}
				}
			}
#endif
			if(value.has_value())
			{
				sectioncache[path.GetRefKey()] = FormatValueAsIni(value.value());
			} else
			{
				sectioncache.erase(path.GetRefKey());
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
				if(case_sensitivity == CaseSensitivity::Insensitive)
				{
					casemap.erase(std::make_pair(lowercase(section), lowercase(path.GetRefKey())));
				}
#endif
			}
		}
	}
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	if(case_sensitivity == CaseSensitivity::Insensitive)
	{
		VerifyCasemap();
	}
#endif
}

void CachedIniFileSettingsBackend::WriteCacheIntoFile(std::optional<Caching> sync_hint)
{
	mpt::ustring filetext;
	const std::list<std::pair<SettingPath, SettingValue>> header = CreateIniHeader(IniVersion{4, 0, 0});
	for(const auto &path : header)
	{
		cache.erase(path.first.GetRefSection());
	}
#if MPT_CXX_BEFORE(20) && MPT_USTRING_MODE_UTF8
	const mpt::ustring newline = MPT_USTRING("\r\n");
#else
	const mpt::uchar *newline = MPT_ULITERAL("\r\n");
#endif
	mpt::ustring last_section;
	for(const auto &[path, value] : header)
	{
		if(path.GetRefSection() != last_section)
		{
			last_section = path.GetRefSection();
			filetext += MPT_UFORMAT_MESSAGE("[{}]{}")(escape(path.GetRefSection()), newline);
		}
		filetext += MPT_UFORMAT_MESSAGE("{}={}{}")(escape(path.GetRefKey()), escape(FormatValueAsIni(value)), newline);
	}
	for(const auto &[section, sectionsettings] : cache)
	{
		filetext += newline;
		auto sectioncomments = comments.find(std::make_pair(section, mpt::ustring{}));
		if(sectioncomments != comments.end())
		{
			for(const auto &comment : sectioncomments->second.before)
			{
				filetext += MPT_UFORMAT_MESSAGE(";{}{}")(comment, newline);
			}
		}
		filetext += MPT_UFORMAT_MESSAGE("[{}]{}")(escape(section), newline);
		if(sectioncomments != comments.end())
		{
			for(const auto &comment : sectioncomments->second.after)
			{
				filetext += MPT_UFORMAT_MESSAGE(";{}{}")(comment, newline);
			}
			if(sectioncomments->second.after.size() > 0)
			{
				filetext += newline;
			}
		}
		if(sectionsettings.has_value())
		{
			for(const auto &[key, value] : sectionsettings.value())
			{
				if(value.has_value())
				{
					auto keycomments = comments.find(std::make_pair(section, key));
					if(keycomments != comments.end())
					{
						for(const auto &comment : keycomments->second.before)
						{
							filetext += MPT_UFORMAT_MESSAGE(";{}{}")(comment, newline);
						}
					}
					const mpt::ustring escaped_value = escape(value.value());
					if(escaped_value != trim_whitespace(escaped_value))
					{
						filetext += MPT_UFORMAT_MESSAGE("{}=\"{}\"{}")(escape(key), escaped_value, newline);
					} else
					{
						filetext += MPT_UFORMAT_MESSAGE("{}={}{}")(escape(key), escaped_value, newline);
					}
					if(keycomments != comments.end())
					{
						for(const auto &comment : keycomments->second.after)
						{
							filetext += MPT_UFORMAT_MESSAGE(";{}{}")(comment, newline);
						}
						if(keycomments->second.after.size() > 0)
						{
							filetext += newline;
						}
					}
				}
			}
		}
	}
	file.write(mpt::textfile::encode(mpt::textfile::get_preferred_encoding(), filetext), sync_hint.value_or(sync_default.value_or(Caching::WriteBack)) == Caching::WriteThrough);
}

#if MPT_SETTINGS_INI_CASE_INSENSITIVE
void CachedIniFileSettingsBackend::VerifyCasemap() const
{
#ifndef NDEBUG
	if(case_sensitivity == CaseSensitivity::Insensitive)
	{
		for(const auto &[section, optkeyvalues] : cache)
		{
			MPT_ASSERT(casemap.find(std::make_pair(lowercase(section), mpt::ustring{})) != casemap.end());
			MPT_ASSERT(casemap.find(std::make_pair(lowercase(section), mpt::ustring{}))->second == std::make_pair(section, mpt::ustring{}));
			if(optkeyvalues.has_value())
			{
				const auto &sectioncache = optkeyvalues.value();
				for(const auto &[key, value] : sectioncache)
				{
					MPT_ASSERT(casemap.find(std::make_pair(lowercase(section), lowercase(key))) != casemap.end());
					MPT_ASSERT(casemap.find(std::make_pair(lowercase(section), lowercase(key)))->second == std::make_pair(section, key));
				}
			}
		}
		for(const auto &[folded, unfolded] : casemap)
		{
			MPT_ASSERT(folded.first == lowercase(unfolded.first));
			MPT_ASSERT(folded.second == lowercase(unfolded.second));
			MPT_ASSERT(cache.find(unfolded.first) != cache.end());
			if(folded.second != mpt::ustring{})
			{
				MPT_ASSERT(cache.find(unfolded.first)->second.has_value());
				MPT_ASSERT(cache.find(unfolded.first)->second.value().find(unfolded.second) != cache.find(unfolded.first)->second.value().end());
			}
		}
	}
#endif
}
#endif

void CachedIniFileSettingsBackend::InvalidateCache()
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	std::shared_lock l(file);
	ReadFileIntoCache();
}

SettingValue CachedIniFileSettingsBackend::ReadSetting(const SettingPath &path, const SettingValue &def) const
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	if(case_sensitivity == CaseSensitivity::Insensitive)
	{
		const auto mapping = casemap.find(std::make_pair(lowercase(path.GetRefSection()), lowercase(path.GetRefKey())));
		if(mapping == casemap.end())
		{
			return def;
		}
		const SettingPath path2 = SettingPath{mapping->second.first, mapping->second.second};
		const auto sectionit = cache.find(path2.GetRefSection());
		if(sectionit == cache.end())
		{
			return def;
		}
		if(!sectionit->second.has_value())
		{
			return def;
		}
		const std::map<mpt::ustring, std::optional<mpt::ustring>> &section = sectionit->second.value();
		const auto it = section.find(path2.GetRefKey());
		if(it == section.end())
		{
			return def;
		}
		if(!it->second.has_value())
		{
			return def;
		}
		return ParseValueFromIni(it->second.value(), def);
	}
#endif
	const auto sectionit = cache.find(path.GetRefSection());
	if(sectionit == cache.end())
	{
		return def;
	}
	if(!sectionit->second.has_value())
	{
		return def;
	}
	const std::map<mpt::ustring, std::optional<mpt::ustring>> &section = sectionit->second.value();
	const auto it = section.find(path.GetRefKey());
	if(it == section.end())
	{
		return def;
	}
	if(!it->second.has_value())
	{
		return def;
	}
	return ParseValueFromIni(it->second.value(), def);
}

void CachedIniFileSettingsBackend::WriteAllSettings(const std::set<mpt::ustring> &removeSections, const std::map<SettingPath, std::optional<SettingValue>> &settings, std::optional<Caching> sync_hint)
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	std::unique_lock l(file);
	ReadFileIntoCache();
	MergeSettingsIntoCache(removeSections, settings);
	WriteCacheIntoFile(sync_hint);
}

void CachedIniFileSettingsBackend::Init()
{
	OPENMPT_PROFILE_FUNCTION(Profiler::Settings);
	std::shared_lock l(file);
	const std::vector<mpt::ustring> lines = ReadFileAsLines();
	const IniVersion version = ProbeVersion(lines);
	if(version.major < 4)
	{
		MakeBackup(P_("win32"), sync_default);
	}
	ReadLinesIntoCache(version, lines);
}

#if MPT_SETTINGS_INI_CASE_INSENSITIVE
CachedIniFileSettingsBackend::CachedIniFileSettingsBackend(mpt::PathString filename_)
	: IniFileBase(std::move(filename_), std::nullopt)
	, case_sensitivity(CaseSensitivity::Insensitive)
{
	Init();
}
CachedIniFileSettingsBackend::CachedIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint)
	: IniFileBase(std::move(filename_), sync_hint)
	, case_sensitivity(CaseSensitivity::Insensitive)
{
	Init();
}
CachedIniFileSettingsBackend::CachedIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint, CaseSensitivity case_sensitivity_)
	: IniFileBase(std::move(filename_), sync_hint)
	, case_sensitivity(case_sensitivity_)
{
	Init();
}
CachedIniFileSettingsBackend::CachedIniFileSettingsBackend(mpt::PathString filename_, CaseSensitivity case_sensitivity_)
	: IniFileBase(std::move(filename_), std::nullopt)
	, case_sensitivity(case_sensitivity_)
{
	Init();
}
CachedIniFileSettingsBackend::CachedIniFileSettingsBackend(mpt::PathString filename_, CaseSensitivity case_sensitivity_, std::optional<Caching> sync_hint)
	: IniFileBase(std::move(filename_), sync_hint)
	, case_sensitivity(case_sensitivity_)
{
	Init();
}
#else
CachedIniFileSettingsBackend::CachedIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint)
	: IniFileBase(std::move(filename_), sync_hint)
{
	Init();
}
#endif

CachedIniFileSettingsBackend::~CachedIniFileSettingsBackend()
{
	return;
}

CaseSensitivity CachedIniFileSettingsBackend::GetCaseSensitivity() const
{
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	return case_sensitivity;
#else
	return CaseSensitivity::Sensitive;
#endif
}



OPENMPT_NAMESPACE_END
