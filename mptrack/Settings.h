/*
 * Settings.h
 * ----------
 * Purpose: Header file for application setting handling framework.
 * Notes  : (currently none)
 * Authors: Joern Heusipp
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


#include "../common/misc_util.h"
#include "mpt/mutex/mutex.hpp"
#include "mpt/parse/parse.hpp"

#include <map>
#include <set>
#include <variant>


OPENMPT_NAMESPACE_BEGIN


enum SettingType
{
	SettingTypeNone,
	SettingTypeBool,
	SettingTypeInt,
	SettingTypeFloat,
	SettingTypeString,
	SettingTypeBinary,
};

// SettingValue is a variant type that stores any type that can natively be represented in a config backend.
// Any other type that should be stored must provide a matching ToSettingValue and FromSettingValue.
// Other types can optionally also set a type tag which would get checked in debug builds.
class SettingValue
{
private:
	std::variant<std::monostate, bool, int32, double, mpt::ustring, std::vector<std::byte>> value;
	std::string typeTag;
public:
	bool operator == (const SettingValue &other) const
	{
		return value == other.value && typeTag == other.typeTag;
	}
	bool operator != (const SettingValue &other) const
	{
		return !(*this == other);
	}
	SettingValue()
	{
	}
	SettingValue(const SettingValue &other)
	{
		*this = other;
	}
	SettingValue & operator = (const SettingValue &other)
	{
		if(this == &other)
		{
			return *this;
		}
		MPT_ASSERT(value.index() == 0 || (value.index() == other.value.index() && typeTag == other.typeTag));
		value = other.value;
		typeTag = other.typeTag;
		return *this;
	}
	SettingValue(bool val)
		: value(val)
	{
	}
	SettingValue(int32 val)
		: value(val)
	{
	}
	SettingValue(double val)
		: value(val)
	{
	}
	SettingValue(const mpt::ustring &val)
		: value(val)
	{
	}
	SettingValue(const std::vector<std::byte> &val)
		: value(val)
	{
	}
	SettingValue(bool val, const std::string &typeTag_)
		: value(val)
		, typeTag(typeTag_)
	{
	}
	SettingValue(int32 val, const std::string &typeTag_)
		: value(val)
		, typeTag(typeTag_)
	{
	}
	SettingValue(double val, const std::string &typeTag_)
		: value(val)
		, typeTag(typeTag_)
	{
	}
	SettingValue(const mpt::ustring &val, const std::string &typeTag_)
		: value(val)
		, typeTag(typeTag_)
	{
	}
	SettingValue(const std::vector<std::byte> &val, const std::string &typeTag_)
		: value(val)
		, typeTag(typeTag_)
	{
	}
	// these need to be explicitly deleted because otherwise the bool overload will catch the pointers
	SettingValue(const char *val) = delete;
	SettingValue(const wchar_t *val) = delete;
	SettingValue(const char *val, const std::string &typeTag_) = delete;
	SettingValue(const wchar_t *val, const std::string &typeTag_) = delete;
	SettingType GetType() const
	{
		SettingType result = SettingTypeNone;
		if(std::holds_alternative<bool>(value))
		{
			result = SettingTypeBool;
		}
		if(std::holds_alternative<int32>(value))
		{
			result = SettingTypeInt;
		}
		if(std::holds_alternative<double>(value))
		{
			result = SettingTypeFloat;
		}
		if(std::holds_alternative<mpt::ustring>(value))
		{
			result = SettingTypeString;
		}
		if(std::holds_alternative<std::vector<std::byte>>(value))
		{
			result = SettingTypeBinary;
		}
		return result;
	}
	bool HasTypeTag() const
	{
		return !typeTag.empty();
	}
	std::string GetTypeTag() const
	{
		return typeTag;
	}
	template <typename T>
	T as() const
	{
		return *this;
	}
	operator bool () const
	{
		MPT_ASSERT(std::holds_alternative<bool>(value));
		return std::get<bool>(value);			
	}
	operator int32 () const
	{
		MPT_ASSERT(std::holds_alternative<int32>(value));
		return std::get<int32>(value);			
	}
	operator double () const
	{
		MPT_ASSERT(std::holds_alternative<double>(value));
		return std::get<double>(value);			
	}
	operator mpt::ustring () const
	{
		MPT_ASSERT(std::holds_alternative<mpt::ustring>(value));
		return std::get<mpt::ustring>(value);			
	}
	operator std::vector<std::byte> () const
	{
		MPT_ASSERT(std::holds_alternative<std::vector<std::byte>>(value));
		return std::get<std::vector<std::byte>>(value);			
	}
	mpt::ustring FormatTypeAsString() const;
	mpt::ustring FormatValueAsString() const;
	void SetFromString(const AnyStringLocale &newVal);
};


template<typename T>
std::vector<std::byte> EncodeBinarySetting(const T &val)
{
	std::vector<std::byte> result(sizeof(T));
	std::memcpy(result.data(), &val, sizeof(T));
	return result;
}
template<typename T>
T DecodeBinarySetting(const std::vector<std::byte> &val)
{
	T result = T();
	if(val.size() >= sizeof(T))
	{
		std::memcpy(&result, val.data(), sizeof(T));
	}
	return result;
}


template<typename T>
inline SettingValue ToSettingValue(const T &val)
{
	return SettingValue(val);
}

template<typename T>
inline T FromSettingValue(const SettingValue &val)
{
	return val.as<T>();
}

// To support settings.Read<Tcustom> and settings.Write<Tcustom>,
// just provide specializations of ToSettingsValue<Tcustom> and FromSettingValue<Tcustom>.
// You may use the SettingValue(value, typeTag) constructor in ToSettingValue
// and check the typeTag FromSettingsValue to implement runtime type-checking for custom types.

template<> inline SettingValue ToSettingValue(const std::string &val) { return SettingValue(mpt::ToUnicode(mpt::Charset::Locale, val)); }
template<> inline std::string FromSettingValue(const SettingValue &val) { return mpt::ToCharset(mpt::Charset::Locale, val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const mpt::lstring &val) { return SettingValue(mpt::ToUnicode(val)); }
template<> inline mpt::lstring FromSettingValue(const SettingValue &val) { return mpt::ToLocale(val.as<mpt::ustring>()); }

#if !MPT_USTRING_MODE_WIDE
template<> inline SettingValue ToSettingValue(const std::wstring &val) { return SettingValue(mpt::ToUnicode(val)); }
template<> inline std::wstring FromSettingValue(const SettingValue &val) { return mpt::ToWide(val.as<mpt::ustring>()); }
#endif

template<> inline SettingValue ToSettingValue(const CString &val) { return SettingValue(mpt::ToUnicode(val)); }
template<> inline CString FromSettingValue(const SettingValue &val) { return mpt::ToCString(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const mpt::PathString &val) { return SettingValue(val.ToUnicode()); }
template<> inline mpt::PathString FromSettingValue(const SettingValue &val) { return mpt::PathString::FromUnicode(val); }

template<> inline SettingValue ToSettingValue(const float &val) { return SettingValue(double(val)); }
template<> inline float FromSettingValue(const SettingValue &val) { return float(val.as<double>()); }

template<> inline SettingValue ToSettingValue(const int64 &val) { return SettingValue(mpt::ufmt::dec(val), "int64"); }
template<> inline int64 FromSettingValue(const SettingValue &val) { return mpt::parse<int64>(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const uint64 &val) { return SettingValue(mpt::ufmt::dec(val), "uint64"); }
template<> inline uint64 FromSettingValue(const SettingValue &val) { return mpt::parse<uint64>(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const uint32 &val) { return SettingValue(int32(val)); }
template<> inline uint32 FromSettingValue(const SettingValue &val) { return uint32(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const uint16 &val) { return SettingValue(int32(val)); }
template<> inline uint16 FromSettingValue(const SettingValue &val) { return uint16(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const uint8 &val) { return SettingValue(int32(val)); }
template<> inline uint8 FromSettingValue(const SettingValue &val) { return uint8(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const LONG &val) { return SettingValue(int32(val)); }
template<> inline LONG FromSettingValue(const SettingValue &val) { return LONG(val.as<int32>()); }


// An instance of SetttingState represents the cached on-disk state of a certain SettingPath.
// The mapping is stored externally in SettingsContainer::map.
class SettingState
{
private:
	SettingValue value;
	const SettingValue defaultValue;
	bool dirty;
public:
	SettingState()
		: dirty(false)
	{
		return;
	}
	SettingState(const SettingValue &def)
		: value(def)
		, defaultValue(def)
		, dirty(false)
	{
		return;
	}
	SettingState & assign(const SettingValue &other, bool setDirty = true)
	{
		MPT_ASSERT(defaultValue.GetType() == SettingTypeNone || (defaultValue.GetType() == other.GetType() && defaultValue.GetTypeTag() == other.GetTypeTag()));
		if(setDirty)
		{
			if(value != other)
			{
				value = other;
				dirty = true;
			}
		} else
		{
			value = other;
		}
		return *this;
	}
	SettingState & operator = (const SettingValue &val)
	{
		assign(val);
		return *this;
	}

	SettingValue GetDefault() const
	{
		return defaultValue;
	}
	const SettingValue &GetRefDefault() const
	{
		return defaultValue;
	}
	bool IsDefault() const
	{
		return value == defaultValue;
	}

	bool IsDirty() const
	{
		return dirty;
	}
	void Clean()
	{
		dirty = false;
	}
	SettingValue GetValue() const
	{
		return value;
	}
	const SettingValue &GetRefValue() const
	{
		return value;
	}
	operator SettingValue () const
	{
		return value;
	}
};


// SettingPath represents the path in a config backend to a certain setting.
class SettingPath
{
private:
	mpt::ustring section;
	mpt::ustring key;
public:
	SettingPath()
	{
		return;
	}
	SettingPath(mpt::ustring section_, mpt::ustring key_)
		: section(std::move(section_))
		, key(std::move(key_))
	{
		return;
	}
	mpt::ustring GetSection() const
	{
		return section;
	}
	mpt::ustring GetKey() const
	{
		return key;
	}
	const mpt::ustring &GetRefSection() const
	{
		return section;
	}
	const mpt::ustring &GetRefKey() const
	{
		return key;
	}
	int compare(const SettingPath &other) const
	{
		int cmp_section = section.compare(other.section);
		if(cmp_section)
		{
			return cmp_section;
		}
		int cmp_key = key.compare(other.key);
		return cmp_key;
	}
	mpt::ustring FormatAsString() const
	{
		return section + U_(".") + key;
	}
};

inline bool operator <  (const SettingPath &left, const SettingPath &right) { return left.compare(right) <  0; }
inline bool operator <= (const SettingPath &left, const SettingPath &right) { return left.compare(right) <= 0; }
inline bool operator >  (const SettingPath &left, const SettingPath &right) { return left.compare(right) >  0; }
inline bool operator >= (const SettingPath &left, const SettingPath &right) { return left.compare(right) >= 0; }
inline bool operator == (const SettingPath &left, const SettingPath &right) { return left.compare(right) == 0; }
inline bool operator != (const SettingPath &left, const SettingPath &right) { return left.compare(right) != 0; }


class ISettingsBackend
{
public:
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const = 0;
	virtual void WriteSetting(const SettingPath &path, const SettingValue &val) = 0;
	virtual void RemoveSetting(const SettingPath &path) = 0;
	virtual void RemoveSection(const mpt::ustring &section) = 0;
protected:
	virtual ~ISettingsBackend() = default;
};


class ISettingChanged
{
public:
	virtual void SettingChanged(const SettingPath &changedPath) = 0;
protected:
	virtual ~ISettingChanged() = default;
};

// SettingContainer basically represents a frontend to 1 or 2 backends (e.g. ini files or registry subtrees) for a collection of configuration settings.
// SettingContainer provides basic read/write access to individual setting. The values are cached and only flushed on destruction or explicit flushs.
class SettingsContainer
{

public:
	using SettingsMap = std::map<SettingPath,SettingState>;
	using SettingsListenerMap = std::map<SettingPath,std::set<ISettingChanged*>>;
	void WriteSettings();
private:
	mutable SettingsMap map;
	mutable SettingsListenerMap mapListeners;

private:
	ISettingsBackend *backend;
private:
	SettingValue BackendsReadSetting(const SettingPath &path, const SettingValue &def) const;
	void BackendsWriteSetting(const SettingPath &path, const SettingValue &val);
	void BackendsRemoveSetting(const SettingPath &path);
	void BackendsRemoveSection(const mpt::ustring &section);
	void NotifyListeners(const SettingPath &path);
	SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const;
	bool IsDefaultSetting(const SettingPath &path) const;
	void WriteSetting(const SettingPath &path, const SettingValue &val);
	void ForgetSetting(const SettingPath &path);
	void RemoveSetting(const SettingPath &path);
	void RemoveSection(const mpt::ustring &section);
private:
	SettingsContainer(const SettingsContainer &other); // disable
	SettingsContainer& operator = (const SettingsContainer &other); // disable
public:
	SettingsContainer(ISettingsBackend *backend);
	template <typename T>
	T Read(const SettingPath &path, const T &def = T()) const
	{
		return FromSettingValue<T>(ReadSetting(path, ToSettingValue<T>(def)));
	}
	template <typename T>
	T Read(mpt::ustring section, mpt::ustring key, const T &def = T()) const
	{
		return FromSettingValue<T>(ReadSetting(SettingPath(std::move(section), std::move(key)), ToSettingValue<T>(def)));
	}
	bool IsDefault(const SettingPath &path) const
	{
		return IsDefaultSetting(path);
	}
	bool IsDefault(mpt::ustring section, mpt::ustring key) const
	{
		return IsDefaultSetting(SettingPath(std::move(section), std::move(key)));
	}
	template <typename T>
	void Write(const SettingPath &path, const T &val)
	{
		WriteSetting(path, ToSettingValue<T>(val));
	}
	template <typename T>
	void Write(mpt::ustring section, mpt::ustring key, const T &val)
	{
		WriteSetting(SettingPath(std::move(section), std::move(key)), ToSettingValue<T>(val));
	}
	void Forget(const SettingPath &path)
	{
		ForgetSetting(path);
	}
	void Forget(mpt::ustring section, mpt::ustring key)
	{
		ForgetSetting(SettingPath(std::move(section), std::move(key)));
	}
	void ForgetAll();
	void Remove(const SettingPath &path)
	{
		RemoveSetting(path);
	}
	void Remove(mpt::ustring section, mpt::ustring key)
	{
		RemoveSetting(SettingPath(std::move(section), std::move(key)));
	}
	void Remove(const mpt::ustring &section)
	{
		RemoveSection(section);
	}
	void Flush();
	~SettingsContainer();

public:

	void Register(ISettingChanged *listener, const SettingPath &path);
	void UnRegister(ISettingChanged *listener, const SettingPath &path);

	SettingsMap::const_iterator begin() const { return map.begin(); }
	SettingsMap::const_iterator end() const { return map.end(); }
	SettingsMap::size_type size() const { return map.size(); }
	bool empty() const { return map.empty(); }
	const SettingsMap &GetMap() const { return map; }

};


// Setting<T> and CachedSetting<T> are references to a SettingPath below a SettingConainer (both provided to the constructor).
// They should mostly behave like normal non-reference variables of type T. I.e., they can be assigned to and read from.
// As they have actual reference semantics, all Setting<T> or CachedSetting<T> that access the same path consistently have the same value.
// The difference between the 2 lies in the way this consistency is achieved:
//  Setting<T>: The actual value is not stored in an instance of Setting<T>.
//   Instead, it is read/written and converted on every access from the SettingContainer.
//   In the SettingContainer, each SettingPath is mapped to a single instance of SettingValue, so there cannot be any incoherence.
//  CachedSetting<T>: The value, readily converted to T, is stored directly in each instance of CachedSetting<T>.
//   A callback for its SettingPath is registered with SettingContainer, and on every change to this SettingPath, the value gets re-read and updated.
// Setting<T> implies some overhead on every access but is generally simpler to understand.
// CachedSetting<T> implies overhead in stored (the copy of T and the callback pointers).
// Except for the difference in runtime/space characteristics, Setting<T> and CachedSetting<T> behave exactly the same way.
// It is recommended to only use CachedSetting<T> for settings that get read frequently, i.e. during GUI updates (like in the pattern view).

template <typename T>
class Setting
{
private:
	SettingsContainer &conf;
	const SettingPath path;
public:
	Setting(const Setting &other) = delete;
	Setting & operator = (const Setting &other) = delete;
public:
	Setting(SettingsContainer &conf_, mpt::ustring section, mpt::ustring key, const T&def)
		: conf(conf_)
		, path(std::move(section), std::move(key))
	{
		conf.Read(path, def); // set default value
	}
	Setting(SettingsContainer &conf_, const SettingPath &path_, const T&def)
		: conf(conf_)
		, path(path_)
	{
		conf.Read(path, def); // set default value
	}
	SettingPath GetPath() const
	{
		return path;
	}
	Setting & operator = (const T &val)
	{
		conf.Write(path, val);
		return *this;
	}
	operator T () const
	{
		return conf.Read<T>(path);
	}
	T Get() const
	{
		return conf.Read<T>(path);
	}
	bool IsDefault() const
	{
		return conf.IsDefault(path);
	}
	template<typename Trhs> Setting & operator += (const Trhs &rhs) { T tmp = *this; tmp += rhs; *this = tmp; return *this; }
	template<typename Trhs> Setting & operator -= (const Trhs &rhs) { T tmp = *this; tmp -= rhs; *this = tmp; return *this; }
	template<typename Trhs> Setting & operator *= (const Trhs &rhs) { T tmp = *this; tmp *= rhs; *this = tmp; return *this; }
	template<typename Trhs> Setting & operator /= (const Trhs &rhs) { T tmp = *this; tmp /= rhs; *this = tmp; return *this; }
	template<typename Trhs> Setting & operator %= (const Trhs &rhs) { T tmp = *this; tmp %= rhs; *this = tmp; return *this; }
	template<typename Trhs> Setting & operator |= (const Trhs &rhs) { T tmp = *this; tmp |= rhs; *this = tmp; return *this; }
	template<typename Trhs> Setting & operator &= (const Trhs &rhs) { T tmp = *this; tmp &= rhs; *this = tmp; return *this; }
	template<typename Trhs> Setting & operator ^= (const Trhs &rhs) { T tmp = *this; tmp ^= rhs; *this = tmp; return *this; }
};

template <typename T>
class CachedSetting
	: public ISettingChanged
{
private:
	mutable mpt::mutex valueMutex;
	T value;
	SettingsContainer &conf;
	const SettingPath path;
public:
	CachedSetting(const CachedSetting &other) = delete;
	CachedSetting & operator = (const CachedSetting &other) = delete;
public:
	CachedSetting(SettingsContainer &conf_, mpt::ustring section, mpt::ustring key, const T&def)
		: value(def)
		, conf(conf_)
		, path(std::move(section), std::move(key))
	{
		{
			mpt::lock_guard<mpt::mutex> l(valueMutex);
			value = conf.Read(path, def);
		}
		conf.Register(this, path);
	}
	CachedSetting(SettingsContainer &conf_, const SettingPath &path_, const T&def)
		: value(def)
		, conf(conf_)
		, path(path_)
	{
		{
			mpt::lock_guard<mpt::mutex> l(valueMutex);
			value = conf.Read(path, def);
		}
		conf.Register(this, path);
	}
	~CachedSetting()
	{
		conf.UnRegister(this, path);
	}
	SettingPath GetPath() const
	{
		return path;
	}
	CachedSetting & operator = (const T &val)
	{
		{
			mpt::lock_guard<mpt::mutex> l(valueMutex);
			value = val;
		}
		conf.Write(path, val);
		return *this;
	}
	operator T () const
	{
		mpt::lock_guard<mpt::mutex> l(valueMutex);
		return value;
	}
	T Get() const
	{
		mpt::lock_guard<mpt::mutex> l(valueMutex);
		return value;
	}
	bool IsDefault() const
	{
		return conf.IsDefault(path);
	}
	CachedSetting & Update()
	{
		{
			mpt::lock_guard<mpt::mutex> l(valueMutex);
			value = conf.Read<T>(path);
		}
		return *this;
	}
	void SettingChanged(const SettingPath &changedPath)
	{
		MPT_UNREFERENCED_PARAMETER(changedPath);
		Update();
	}
	template<typename Trhs> CachedSetting & operator += (const Trhs &rhs) { T tmp = *this; tmp += rhs; *this = tmp; return *this; }
	template<typename Trhs> CachedSetting & operator -= (const Trhs &rhs) { T tmp = *this; tmp -= rhs; *this = tmp; return *this; }
	template<typename Trhs> CachedSetting & operator *= (const Trhs &rhs) { T tmp = *this; tmp *= rhs; *this = tmp; return *this; }
	template<typename Trhs> CachedSetting & operator /= (const Trhs &rhs) { T tmp = *this; tmp /= rhs; *this = tmp; return *this; }
	template<typename Trhs> CachedSetting & operator %= (const Trhs &rhs) { T tmp = *this; tmp %= rhs; *this = tmp; return *this; }
	template<typename Trhs> CachedSetting & operator |= (const Trhs &rhs) { T tmp = *this; tmp |= rhs; *this = tmp; return *this; }
	template<typename Trhs> CachedSetting & operator &= (const Trhs &rhs) { T tmp = *this; tmp &= rhs; *this = tmp; return *this; }
	template<typename Trhs> CachedSetting & operator ^= (const Trhs &rhs) { T tmp = *this; tmp ^= rhs; *this = tmp; return *this; }
};


class IniFileSettingsBackend : public ISettingsBackend
{
private:
	const mpt::PathString filename;
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
	IniFileSettingsBackend(const mpt::PathString &filename);
	~IniFileSettingsBackend() override;
	void ConvertToUnicode(const mpt::ustring &backupTag = mpt::ustring());
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const override;
	virtual void WriteSetting(const SettingPath &path, const SettingValue &val) override;
	virtual void RemoveSetting(const SettingPath &path) override;
	virtual void RemoveSection(const mpt::ustring &section) override;
	const mpt::PathString& GetFilename() const { return filename; }
};

class IniFileSettingsContainer : private IniFileSettingsBackend, public SettingsContainer
{
public:
	IniFileSettingsContainer(const mpt::PathString &filename);
	~IniFileSettingsContainer() override;
};

class DefaultSettingsContainer : public IniFileSettingsContainer
{
public:
	DefaultSettingsContainer();
	~DefaultSettingsContainer() override;
};


class SettingChangedNotifyGuard
{
private:
	SettingsContainer &conf;
	SettingPath m_Path;
	bool m_Registered;
	ISettingChanged *m_Handler;
public:
	SettingChangedNotifyGuard(SettingsContainer &conf, const SettingPath &path)
		: conf(conf)
		, m_Path(path)
		, m_Registered(false)
		, m_Handler(nullptr)
	{
		return;
	}
	void Register(ISettingChanged *handler)
	{
		if(m_Registered)
		{
			return;
		}
		m_Handler = handler;
		conf.Register(m_Handler, m_Path);
		m_Registered = true;
	}
	~SettingChangedNotifyGuard()
	{
		if(m_Registered)
		{
			conf.UnRegister(m_Handler, m_Path);
			m_Registered = false;
		}
	}
};


OPENMPT_NAMESPACE_END
