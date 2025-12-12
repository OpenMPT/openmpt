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
			std::lock_guard bl{backup};
			backup.write(mpt::as_span(data));
		}
		static_assert(sizeof(wchar_t) == 2);
		std::ostringstream inifile{std::ios::binary};
		inifile.imbue(std::locale::classic());
		const char UTF16LE_BOM[] = { static_cast<char>(0xff), static_cast<char>(0xfe) };
		inifile.write(UTF16LE_BOM, 2);
		const std::wstring str = mpt::ToWide(mpt::Charset::Locale, mpt::buffer_cast<std::string>(data));
		inifile.write(reinterpret_cast<const char*>(str.c_str()), str.length() * sizeof(std::wstring::value_type));
		file.write(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(inifile.str())));
	}
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



OPENMPT_NAMESPACE_END
