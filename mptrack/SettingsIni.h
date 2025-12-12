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

class ImmediateWindowsIniFileSettingsBackend
	: public ISettingsBackend<SettingsBatching::Single>
{
private:
	const mpt::PathString filename;
	mpt::IO::atomic_shared_file_ref file;
private:
	std::vector<std::byte> ReadSettingRaw(const SettingPath &path, const std::vector<std::byte> &def) const;
	mpt::ustring ReadSettingRaw(const SettingPath &path, const mpt::ustring &def) const;
	double ReadSettingRaw(const SettingPath &path, double def) const;
	int32 ReadSettingRaw(const SettingPath &path, int32 def) const;
	bool ReadSettingRaw(const SettingPath &path, bool def) const;
	void WriteSettingRaw(const SettingPath &path, const std::vector<std::byte> &val);
	void WriteSettingRaw(const SettingPath &path, const mpt::ustring &val);
	void WriteSettingRaw(const SettingPath &path, double val);
	void WriteSettingRaw(const SettingPath &path, int32 val);
	void WriteSettingRaw(const SettingPath &path, bool val);
	void RemoveSettingRaw(const SettingPath &path);
	void RemoveSectionRaw(const mpt::ustring &section);
	static mpt::winstring GetSection(const SettingPath &path);
	static mpt::winstring GetKey(const SettingPath &path);
public:
	ImmediateWindowsIniFileSettingsBackend(const mpt::PathString &filename);
	~ImmediateWindowsIniFileSettingsBackend() override;
	void ConvertToUnicode(const mpt::ustring &backupTag = mpt::ustring());
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const override;
	virtual void WriteSetting(const SettingPath &path, const SettingValue &val) override;
	virtual void RemoveSetting(const SettingPath &path) override;
	virtual void RemoveSection(const mpt::ustring &section) override;
	const mpt::PathString& GetFilename() const { return filename; }
};

#ifndef IniFileSettingsBackend
#define IniFileSettingsBackend ImmediateWindowsIniFileSettingsBackend
#endif

class IniFileSettingsContainer : private IniFileSettingsBackend, public SettingsContainer
{
public:
	IniFileSettingsContainer(const mpt::PathString &filename);
	~IniFileSettingsContainer() override;
};


OPENMPT_NAMESPACE_END
