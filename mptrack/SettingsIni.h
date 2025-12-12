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
#include <set>
#include <vector>


OPENMPT_NAMESPACE_BEGIN


class IniFileHelpers
{
protected:
	static mpt::winstring GetSection(const SettingPath &path);
	static mpt::winstring GetKey(const SettingPath &path);
};


class WindowsIniFileBase
{
protected:
	const mpt::PathString filename;
	mpt::IO::atomic_shared_file_ref file;
protected:
	WindowsIniFileBase(mpt::PathString filename_);
	~WindowsIniFileBase() = default;
public:
	void ConvertToUnicode(const mpt::ustring &backupTag = mpt::ustring());
public:
	const mpt::PathString &Filename() const;
	mpt::PathString GetFilename() const;
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
	ImmediateWindowsIniFileSettingsBackend(mpt::PathString filename_);
	~ImmediateWindowsIniFileSettingsBackend() override;
public:
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const override;
	virtual void RemoveSection(const mpt::ustring &section) override;
	virtual void RemoveSetting(const SettingPath &path) override;
	virtual void WriteSetting(const SettingPath &path, const SettingValue &val) override;
};


#ifndef IniFileSettingsBackend
#define IniFileSettingsBackend ImmediateWindowsIniFileSettingsBackend
#endif


#ifndef IniFileSettingsContainer
#define IniFileSettingsContainer FileSettingsContainer<ImmediateWindowsIniFileSettingsBackend>
#endif


OPENMPT_NAMESPACE_END
