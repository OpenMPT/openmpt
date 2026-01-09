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

#include "mpt/io_file_atomic/atomic_file.hpp"

#include <map>
#include <optional>
#include <set>
#include <vector>

#include <cstddef>


OPENMPT_NAMESPACE_BEGIN



class IniFileHelpers
{
protected:
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


using IniFileSettingsBackend = ImmediateWindowsIniFileSettingsBackend;

using IniFileSettingsContainer = FileSettingsContainer<ImmediateWindowsIniFileSettingsBackend>;


OPENMPT_NAMESPACE_END
