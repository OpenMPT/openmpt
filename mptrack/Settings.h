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


#include "../common/misc_util.h"

#include <map>
#include <set>


OPENMPT_NAMESPACE_BEGIN


#define MPT_SETTINGS_CACHE
#define MPT_SETTINGS_CACHE_STORE_DEFAULTS


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
	bool valueBool;
	int32 valueInt;
	double valueFloat;
	mpt::ustring valueString;
	std::vector<char> valueBinary;
	SettingType type;
	std::string typeTag;
	void Init()
	{
		valueBool = false;
		valueInt = 0;
		valueFloat = 0.0;
		valueString = mpt::ustring();
		valueBinary.clear();
		type = SettingTypeNone;
		typeTag = std::string();
	}
public:
	bool operator == (const SettingValue &other) const
	{
		return type == other.type
			&& typeTag == other.typeTag
			&& valueBool == other.valueBool
			&& valueInt == other.valueInt
			&& valueFloat == other.valueFloat
			&& valueString == other.valueString
			&& valueBinary == other.valueBinary
			;
	}
	bool operator != (const SettingValue &other) const
	{
		return !(*this == other);
	}
	SettingValue()
	{
		Init();
	}
	SettingValue(const SettingValue &other)
	{
		Init();
		*this = other;
	}
	SettingValue & operator = (const SettingValue &other)
	{
		if(this == &other)
		{
			return *this;
		}
		MPT_ASSERT(type == SettingTypeNone || (type == other.type && typeTag == other.typeTag));
		type = other.type;
		valueBool = other.valueBool;
		valueInt = other.valueInt;
		valueFloat = other.valueFloat;
		valueString = other.valueString;
		valueBinary = other.valueBinary;
		typeTag = other.typeTag;
		return *this;
	}
	SettingValue(bool val)
	{
		Init();
		type = SettingTypeBool;
		valueBool = val;
	}
	SettingValue(int32 val)
	{
		Init();
		type = SettingTypeInt;
		valueInt = val;
	}
	SettingValue(double val)
	{
		Init();
		type = SettingTypeFloat;
		valueFloat = val;
	}
	SettingValue(const char *val)
	{
		Init();
		type = SettingTypeString;
		valueString = mpt::ToUnicode(mpt::CharsetLocale, val);
	}
	SettingValue(const std::string &val)
	{
		Init();
		type = SettingTypeString;
		valueString = mpt::ToUnicode(mpt::CharsetLocale, val);
	}
	SettingValue(const wchar_t *val)
	{
		Init();
		type = SettingTypeString;
		valueString = mpt::ToUnicode(val);
	}
	SettingValue(const std::wstring &val)
	{
		Init();
		type = SettingTypeString;
		valueString = mpt::ToUnicode(val);
	}
#if MPT_USTRING_MODE_UTF8
	SettingValue(const mpt::ustring &val)
	{
		Init();
		type = SettingTypeString;
		valueString = val;
	}
#endif
	SettingValue(const std::vector<char> &val)
	{
		Init();
		type = SettingTypeBinary;
		valueBinary =  val;
	}
	SettingValue(bool val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeBool;
		typeTag = typeTag_;
		valueBool = val;
	}
	SettingValue(int32 val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeInt;
		typeTag = typeTag_;
		valueInt = val;
	}
	SettingValue(double val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeFloat;
		typeTag = typeTag_;
		valueFloat = val;
	}
	SettingValue(const char *val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeString;
		typeTag = typeTag_;
		valueString = mpt::ToUnicode(mpt::CharsetLocale, val);
	}
	SettingValue(const std::string &val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeString;
		typeTag = typeTag_;
		valueString = mpt::ToUnicode(mpt::CharsetLocale, val);
	}
	SettingValue(const wchar_t *val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeString;
		typeTag = typeTag_;
		valueString = mpt::ToUnicode(val);
	}
	SettingValue(const std::wstring &val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeString;
		typeTag = typeTag_;
		valueString = mpt::ToUnicode(val);
	}
#if MPT_USTRING_MODE_UTF8
	SettingValue(const mpt::ustring &val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeString;
		typeTag = typeTag_;
		valueString = val;
	}
#endif
	SettingValue(const std::vector<char> &val, const std::string &typeTag_)
	{
		Init();
		type = SettingTypeBinary;
		typeTag = typeTag_;
		valueBinary =  val;
	}
	SettingType GetType() const
	{
		return type;
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
		return static_cast<T>(*this);
	}
	operator bool () const
	{
		MPT_ASSERT(type == SettingTypeBool);
		return valueBool;
	}
	operator int32 () const
	{
		MPT_ASSERT(type == SettingTypeInt);
		return valueInt;
	}
	operator double () const
	{
		MPT_ASSERT(type == SettingTypeFloat);
		return valueFloat;
	}
	operator std::string () const
	{
		MPT_ASSERT(type == SettingTypeString);
		return mpt::ToCharset(mpt::CharsetLocale, valueString);
	}
	operator std::wstring () const
	{
		MPT_ASSERT(type == SettingTypeString);
		return mpt::ToWide(valueString);
	}
#if MPT_USTRING_MODE_UTF8
	operator mpt::ustring () const
	{
		MPT_ASSERT(type == SettingTypeString);
		return valueString;
	}
#endif
	operator std::vector<char> () const
	{
		MPT_ASSERT(type == SettingTypeBinary);
		return valueBinary;
	}
	mpt::ustring FormatTypeAsString() const;
	mpt::ustring FormatValueAsString() const;
	void SetFromString(const AnyStringLocale &newVal);
};


template<typename T>
std::vector<char> EncodeBinarySetting(const T &val)
{
	std::vector<char> result(sizeof(T));
	std::memcpy(&result[0], &val, sizeof(T));
	return result;
}
template<typename T>
T DecodeBinarySetting(const std::vector<char> &val)
{
	T result = T();
	if(val.size() >= sizeof(T))
	{
		std::memcpy(&result, &val[0], sizeof(T));
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

template<> inline SettingValue ToSettingValue(const CString &val) { return SettingValue(mpt::ToWide(val)); }
template<> inline CString FromSettingValue(const SettingValue &val) { return mpt::ToCString(val.as<std::wstring>()); }

template<> inline SettingValue ToSettingValue(const mpt::PathString &val) { return SettingValue(val.AsNative()); }
template<> inline mpt::PathString FromSettingValue(const SettingValue &val) { return mpt::PathString::FromNative(val); }

template<> inline SettingValue ToSettingValue(const float &val) { return SettingValue(double(val)); }
template<> inline float FromSettingValue(const SettingValue &val) { return float(val.as<double>()); }

template<> inline SettingValue ToSettingValue(const int64 &val) { return SettingValue(mpt::ufmt::dec(val), "int64"); }
template<> inline int64 FromSettingValue(const SettingValue &val) { return ConvertStrTo<int64>(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const uint64 &val) { return SettingValue(mpt::ufmt::dec(val), "uint64"); }
template<> inline uint64 FromSettingValue(const SettingValue &val) { return ConvertStrTo<uint64>(val.as<mpt::ustring>()); }

template<> inline SettingValue ToSettingValue(const uint32 &val) { return SettingValue(int32(val)); }
template<> inline uint32 FromSettingValue(const SettingValue &val) { return uint32(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const uint16 &val) { return SettingValue(int32(val)); }
template<> inline uint16 FromSettingValue(const SettingValue &val) { return uint16(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const uint8 &val) { return SettingValue(int32(val)); }
template<> inline uint8 FromSettingValue(const SettingValue &val) { return uint8(val.as<int32>()); }

template<> inline SettingValue ToSettingValue(const LONG &val) { return SettingValue(int32(val)); }
template<> inline LONG FromSettingValue(const SettingValue &val) { return LONG(val.as<int32>()); }


#if defined(MPT_SETTINGS_CACHE)


// An instance of SetttingState represents the cached on-disk state of a certain SettingPath.
// The mapping is stored externally in SettingsContainer::map.
class SettingState
{
private:
	SettingValue value;
#if defined(MPT_SETTINGS_CACHE_STORE_DEFAULTS)
	const SettingValue defaultValue;
#endif // MPT_SETTINGS_CACHE_STORE_DEFAULTS
	bool dirty;
public:
	SettingState()
	{
		return;
	}
	SettingState(const SettingValue &def)
		: value(def)
#if defined(MPT_SETTINGS_CACHE_STORE_DEFAULTS)
		, defaultValue(def)
#endif // MPT_SETTINGS_CACHE_STORE_DEFAULTS
		, dirty(false)
	{
		return;
	}
	SettingState & assign(const SettingValue &other, bool setDirty = true)
	{
#if defined(MPT_SETTINGS_CACHE_STORE_DEFAULTS)
		MPT_ASSERT(defaultValue.GetType() == SettingTypeNone || (defaultValue.GetType() == other.GetType() && defaultValue.GetTypeTag() == other.GetTypeTag()));
#endif // MPT_SETTINGS_CACHE_STORE_DEFAULTS
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
#if defined(MPT_SETTINGS_CACHE_STORE_DEFAULTS)
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
#endif // MPT_SETTINGS_CACHE_STORE_DEFAULTS
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


#endif // MPT_SETTINGS_CACHE


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
	SettingPath(const AnyStringLocale &section_, const AnyStringLocale &key_)
		: section(section_)
		, key(key_)
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
		return section + MPT_USTRING(".") + key;
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
};


class ISettingChanged
{
public:
	virtual void SettingChanged(const SettingPath &changedPath) = 0;
};

enum SettingFlushMode
{
	SettingWriteBack    = 0,
	SettingWriteThrough = 1,
};

// SettingContainer basically represents a frontend to 1 or 2 backends (e.g. ini files or registry subtrees) for a collection of configuration settings.
// SettingContainer provides basic read/write access to individual setting. The values are cached and only flushed on destruction or explicit flushs.
class SettingsContainer
{

	#if defined(MPT_SETTINGS_CACHE)

		public:
			typedef std::map<SettingPath,SettingState> SettingsMap;
			typedef std::map<SettingPath,std::set<ISettingChanged*> > SettingsListenerMap;
			void WriteSettings();
		private:
			mutable SettingsMap map;
			mutable SettingsListenerMap mapListeners;

	#endif // MPT_SETTINGS_CACHE

private:
	ISettingsBackend *backend;
private:
	bool immediateFlush;
	SettingValue BackendsReadSetting(const SettingPath &path, const SettingValue &def) const;
	void BackendsWriteSetting(const SettingPath &path, const SettingValue &val);
	void BackendsRemoveSetting(const SettingPath &path);
	void NotifyListeners(const SettingPath &path);
	SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const;
	bool IsDefaultSetting(const SettingPath &path) const;
	void WriteSetting(const SettingPath &path, const SettingValue &val, SettingFlushMode flushMode);
	void ForgetSetting(const SettingPath &path);
	void RemoveSetting(const SettingPath &path);
private:
	SettingsContainer(const SettingsContainer &other); // disable
	SettingsContainer& operator = (const SettingsContainer &other); // disable
public:
	SettingsContainer(ISettingsBackend *backend);
	void SetImmediateFlush(bool newImmediateFlush);
	template <typename T>
	T Read(const SettingPath &path, const T &def = T()) const
	{
		return FromSettingValue<T>(ReadSetting(path, ToSettingValue<T>(def)));
	}
	template <typename T>
	T Read(const AnyStringLocale &section, const AnyStringLocale &key, const T &def = T()) const
	{
		return FromSettingValue<T>(ReadSetting(SettingPath(section, key), ToSettingValue<T>(def)));
	}
	bool IsDefault(const SettingPath &path) const
	{
		return IsDefaultSetting(path);
	}
	bool IsDefault(const AnyStringLocale &section, const AnyStringLocale &key) const
	{
		return IsDefaultSetting(SettingPath(section, key));
	}
	template <typename T>
	void Write(const SettingPath &path, const T &val, SettingFlushMode flushMode = SettingWriteBack)
	{
		WriteSetting(path, ToSettingValue<T>(val), flushMode);
	}
	template <typename T>
	void Write(const AnyStringLocale &section, const AnyStringLocale &key, const T &val, SettingFlushMode flushMode = SettingWriteBack)
	{
		WriteSetting(SettingPath(section, key), ToSettingValue<T>(val), flushMode);
	}
	void Forget(const SettingPath &path)
	{
		ForgetSetting(path);
	}
	void Forget(const AnyStringLocale &section, const AnyStringLocale &key)
	{
		ForgetSetting(SettingPath(section, key));
	}
	void ForgetAll();
	void Remove(const SettingPath &path)
	{
		RemoveSetting(path);
	}
	void Remove(const AnyStringLocale &section, const AnyStringLocale &key)
	{
		RemoveSetting(SettingPath(section, key));
	}
	void Flush();
	~SettingsContainer();

	#if defined(MPT_SETTINGS_CACHE)

		public:

			void Register(ISettingChanged *listener, const SettingPath &path);
			void UnRegister(ISettingChanged *listener, const SettingPath &path);

			SettingsMap::const_iterator begin() const { return map.begin(); }
			SettingsMap::const_iterator end() const { return map.end(); }
			SettingsMap::size_type size() const { return map.size(); }
			bool empty() const { return map.empty(); }
			const SettingsMap &GetMap() const { return map; }

	#endif // MPT_SETTINGS_CACHE

};

#if defined(MPT_SETTINGS_CACHE)

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
	Setting(const Setting &other)
		: conf(other.conf)
		, path(other.path)
	{
		return;
	}
public:
	Setting(SettingsContainer &conf_, const AnyStringLocale &section, const AnyStringLocale &key, const T&def)
		: conf(conf_)
		, path(section, key)
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
	operator const T () const
	{
		return conf.Read<T>(path);
	}
	const T Get() const
	{
		return conf.Read<T>(path);
	}
	bool IsDefault() const
	{
		conf.IsDefault(path);
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
	T value;
	SettingsContainer &conf;
	const SettingPath path;
public:
	CachedSetting(const CachedSetting &other)
		: value(other.value)
		, conf(other.conf)
		, path(other.path)
	{
		conf.Register(this, path);
	}
public:
	CachedSetting(SettingsContainer &conf_, const AnyStringLocale &section, const AnyStringLocale &key, const T&def)
		: value(def)
		, conf(conf_)
		, path(section, key)
	{
		value = conf.Read(path, def);
		conf.Register(this, path);
	}
	CachedSetting(SettingsContainer &conf_, const SettingPath &path_, const T&def)
		: value(def)
		, conf(conf_)
		, path(path_)
	{
		value = conf.Read(path, def);
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
		value = val;
		conf.Write(path, val);
		return *this;
	}
	operator const T & () const
	{
		return value;
	}
	const T & Get() const
	{
		return value;
	}
	bool IsDefault() const
	{
		conf.IsDefault(path);
	}
	CachedSetting & Update()
	{
		value = conf.Read<T>(path);
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

#else // !MPT_SETTINGS_CACHE

template <typename T>
class Setting
{
private:
	T value;
	SettingsContainer &conf;
	const SettingPath path;
public:
	Setting(const Setting &other)
		: value(other.value)
		, conf(other.conf)
		, path(other.path)
	{
		return;
	}
public:
	Setting(SettingsContainer &conf_, const AnyStringLocale &section, const AnyStringLocale &key, const T&def)
		: value(def)
		, conf(conf_)
		, path(section, key)
	{
		value = conf.Read(path, def);
	}
	Setting(SettingsContainer &conf_, const SettingPath &path_, const T&def)
		: value(def)
		, conf(conf_)
		, path(path_)
	{
		value = conf.Read(path, def);
	}
	~Setting()
	{
		conf.Write(path, val);
	}
	SettingPath GetPath() const
	{
		return path;
	}
	Setting & operator = (const T &val)
	{
		value = val;
		return *this;
	}
	operator const T & () const
	{
		return value;
	}
	const T & Get() const
	{
		return value;
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

#define CachedSetting Setting

#endif // MPT_SETTINGS_CACHE


class IniFileSettingsBackend : public ISettingsBackend
{
private:
	const mpt::PathString filename;
private:
	std::vector<char> ReadSettingRaw(const SettingPath &path, const std::vector<char> &def) const;
	std::wstring ReadSettingRaw(const SettingPath &path, const std::wstring &def) const;
	double ReadSettingRaw(const SettingPath &path, double def) const;
	int32 ReadSettingRaw(const SettingPath &path, int32 def) const;
	bool ReadSettingRaw(const SettingPath &path, bool def) const;
	void WriteSettingRaw(const SettingPath &path, const std::vector<char> &val);
	void WriteSettingRaw(const SettingPath &path, const std::wstring &val);
	void WriteSettingRaw(const SettingPath &path, double val);
	void WriteSettingRaw(const SettingPath &path, int32 val);
	void WriteSettingRaw(const SettingPath &path, bool val);
	void RemoveSettingRaw(const SettingPath &path);
	static std::wstring GetSection(const SettingPath &path);
	static std::wstring GetKey(const SettingPath &path);
public:
	IniFileSettingsBackend(const mpt::PathString &filename);
	~IniFileSettingsBackend();
	void ConvertToUnicode(const std::wstring &backupTag = std::wstring());
	virtual SettingValue ReadSetting(const SettingPath &path, const SettingValue &def) const;
	virtual void WriteSetting(const SettingPath &path, const SettingValue &val);
	virtual void RemoveSetting(const SettingPath &path);
	const mpt::PathString& GetFilename() const { return filename; }
};

class IniFileSettingsContainer : private IniFileSettingsBackend, public SettingsContainer
{
public:
	IniFileSettingsContainer(const mpt::PathString &filename);
	~IniFileSettingsContainer();
};

class DefaultSettingsContainer : public IniFileSettingsContainer
{
public:
	DefaultSettingsContainer();
	~DefaultSettingsContainer();
};


#if defined(MPT_SETTINGS_CACHE)

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

#endif // MPT_SETTINGS_CACHE


OPENMPT_NAMESPACE_END
