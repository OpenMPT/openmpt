/*
 * SettingsIni.h
 * -------------
 * Purpose: Header file for setting in ini format.
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "Settings.h"

#include "mpt/base/integer.hpp"
#include "mpt/base/span.hpp"
#include "mpt/io_file_atomic/atomic_file.hpp"
#include "mpt/string/types.hpp"

#include <list>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <cstddef>


OPENMPT_NAMESPACE_BEGIN


#define MPT_SETTINGS_INI_CASE_INSENSITIVE 1


// Version 1:
//  *  encoding: ANSI

// Version 2:
//  *  encoding: UTF-16LE with BOM

// Version 3:
//  *  encoding: UTF-8 with BOM (Wine-only, not supported by Windows)

// Version 1 / 2 / 3:
//  *  no header
//  *  no escaping
//  *  string quoting with " or ' to support whitespace
//  *  case-insensitive

// Version 4:
//  *  encoding: any Unicode encoding with BOM, ANSI/Locale without BOM
//  *  header:
//      * [!Type]
//        !Format=org.openmpt.fileformat.ini
//        !VersionMajor=4
//        !VersionMinor=0
//        !VersionPatch=0
//  *  escaping:
//      *  ^xff / ^uffff / ^Uffffffff
//         Unicode codepoint ([0x00..0x1f], [0x80..0x9f])
//      *  ^^ / ^; / ^[ / ^] / ^= / ^" / ^'
//         Significant syntax elements
//  *  string quoting with " or ' to support whitespace
//  *  optionally case-sensitive or case-insensitive
//  *  ; Comments on separate lines


struct IniVersion
{
	uint8 major = 0;
	uint8 minor = 0;
	uint8 patch = 0;
};

class IniFileHelpers
{
protected:
	static IniVersion ProbeVersion(const std::vector<mpt::ustring> &lines);
	static std::list<std::pair<SettingPath, SettingValue>> CreateIniHeader(IniVersion version);
	static mpt::ustring FormatValueAsIni(const SettingValue &value);
	static SettingValue ParseValueFromIni(const mpt::ustring &str, const SettingValue &def);
};


class IniFileBase
{
protected:
	const mpt::PathString filename;
	mpt::IO::atomic_shared_file_ref file;
	std::optional<Caching> sync_default;
protected:
	IniFileBase(mpt::PathString filename_, std::optional<Caching> sync_hint);
	~IniFileBase() = default;
public:
	const mpt::PathString &Filename() const;
	mpt::PathString GetFilename() const;
public:
	void MakeBackup(const mpt::PathString &tag, std::optional<Caching> sync_hint);
};


class WindowsIniFileBase
	: public IniFileBase
{
protected:
	WindowsIniFileBase(mpt::PathString filename_, std::optional<Caching> sync_hint);
	~WindowsIniFileBase() = default;
protected:
	static mpt::winstring GetSection(const SettingPath &path);
	static mpt::winstring GetKey(const SettingPath &path);
public:
	void ConvertToUnicode(std::optional<Caching> sync_hint);
};


class ImmediateWindowsIniFileSettingsBackend
	: public ISettingsBackend<SettingsBatching::Single>
	, public WindowsIniFileBase
	, protected IniFileHelpers
{
private:
	std::vector<std::byte> ReadSettingRaw(const SettingPath &path, const std::vector<std::byte> &def) const;
	mpt::ustring ReadSettingRaw(const SettingPath &path, const mpt::ustring &def) const;
	double ReadSettingRaw(const SettingPath &path, double def) const;
	int32 ReadSettingRaw(const SettingPath &path, int32 def) const;
	bool ReadSettingRaw(const SettingPath &path, bool def) const;
	void RemoveSectionRaw(const mpt::ustring &section);
	void RemoveSettingRaw(const SettingPath &path);
	void WriteSettingRaw(const SettingPath &path, const std::vector<std::byte> &val);
	void WriteSettingRaw(const SettingPath &path, const mpt::ustring &val);
	void WriteSettingRaw(const SettingPath &path, double val);
	void WriteSettingRaw(const SettingPath &path, int32 val);
	void WriteSettingRaw(const SettingPath &path, bool val);
public:
	ImmediateWindowsIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint = std::nullopt);
	~ImmediateWindowsIniFileSettingsBackend() override;
public:
	virtual CaseSensitivity GetCaseSensitivity() const override;
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const override;
	virtual void RemoveSection(const mpt::ustring &section) override;
	virtual void RemoveSetting(const SettingPath &path) override;
	virtual void WriteSetting(const SettingPath &path, const SettingValue &val) override;
	virtual void Sync(std::optional<Caching> sync_hint) override;
};


class BatchedWindowsIniFileSettingsBackend
	: public ISettingsBackend<SettingsBatching::Section>
	, public WindowsIniFileBase
	, protected IniFileHelpers
{
private:
	std::map<mpt::ustring, std::optional<std::map<mpt::ustring, std::optional<mpt::ustring>>>> cache;
private:
	std::set<mpt::ustring> ReadSectionNamesRaw() const;
	std::map<mpt::ustring, std::optional<mpt::ustring>> ReadNamedSectionRaw(const mpt::ustring &section) const;
	std::map<mpt::ustring, std::optional<std::map<mpt::ustring, std::optional<mpt::ustring>>>> ReadAllSectionsRaw() const;
	void RemoveSectionRaw(const mpt::ustring &section);
	void WriteSectionRaw(const mpt::ustring &section, const std::map<mpt::ustring, std::optional<mpt::ustring>> &keyvalues);
public:
	BatchedWindowsIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint = std::nullopt);
	~BatchedWindowsIniFileSettingsBackend() override;
public:
	virtual CaseSensitivity GetCaseSensitivity() const override;
	virtual void InvalidateCache() override;
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const override;
	virtual void WriteRemovedSections(const std::set<mpt::ustring> &removeSections) override;
	virtual void WriteMultipleSettings(const std::map<SettingPath, std::optional<SettingValue>> &settings) override;
	virtual void Sync(std::optional<Caching> sync_hint) override;
};


class CachedIniFileSettingsBackend
	: public ISettingsBackend<SettingsBatching::All>
	, public IniFileBase
	, protected IniFileHelpers
{
private:
	struct comments
	{
		std::vector<mpt::ustring> before;
		std::vector<mpt::ustring> after;
	};
private:
	std::map<mpt::ustring, std::optional<std::map<mpt::ustring, std::optional<mpt::ustring>>>> cache;
	std::map<std::pair<mpt::ustring, mpt::ustring>, comments> comments;
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	const CaseSensitivity case_sensitivity;
	std::map<std::pair<mpt::ustring, mpt::ustring>, std::pair<mpt::ustring, mpt::ustring>> casemap;
#endif
private:
	std::vector<mpt::ustring> ReadFileAsLines();
	void ReadLinesIntoCache(const std::vector<mpt::ustring> &lines);
	void ReadLinesIntoCache(IniVersion version, const std::vector<mpt::ustring> &lines);
	void ReadFileIntoCache();
	void MergeSettingsIntoCache(const std::set<mpt::ustring> &removeSections, const std::map<SettingPath, std::optional<SettingValue>> &settings);
	void WriteCacheIntoFile(std::optional<Caching> sync_hint);
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	void VerifyCasemap() const;
#endif
private:
	void Init();
public:
#if MPT_SETTINGS_INI_CASE_INSENSITIVE
	CachedIniFileSettingsBackend(mpt::PathString filename_);
	CachedIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint);
	CachedIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint, CaseSensitivity case_sensitivity_);
	CachedIniFileSettingsBackend(mpt::PathString filename_, CaseSensitivity case_sensitivity_);
	CachedIniFileSettingsBackend(mpt::PathString filename_, CaseSensitivity case_sensitivity_, std::optional<Caching> sync_hint);
#else
	CachedIniFileSettingsBackend(mpt::PathString filename_, std::optional<Caching> sync_hint = std::nullopt);
#endif
	~CachedIniFileSettingsBackend() override;
public:
	virtual CaseSensitivity GetCaseSensitivity() const override;
	virtual void InvalidateCache() override;
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const override;
	virtual void WriteAllSettings(const std::set<mpt::ustring> &removeSections, const std::map<SettingPath, std::optional<SettingValue>> &settings, std::optional<Caching> sync_hint) override;
};


using IniFileSettingsBackend = CachedIniFileSettingsBackend;

using IniFileSettingsContainer = FileSettingsContainer<CachedIniFileSettingsBackend>;


OPENMPT_NAMESPACE_END
