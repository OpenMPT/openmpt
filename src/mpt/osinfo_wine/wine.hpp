/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_WINE_WINE_HPP
#define MPT_OSINFO_WINE_WINE_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/format/join.hpp"
#include "mpt/format/simple.hpp"
#include "mpt/library/library.hpp"
#include "mpt/osinfo/class.hpp"
#include "mpt/osinfo/windows_wine_version.hpp"
#include "mpt/path/native_path.hpp"
#include "mpt/parse/split.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

namespace windows {

namespace wine {



class VersionContext {
protected:
	mpt::osinfo::windows::wine::version m_Version;
	mpt::osinfo::osclass m_HostClass;
	std::string m_RawVersion;
	std::string m_RawBuildID;
	std::string m_RawHostSysName;
	std::string m_RawHostRelease;
public:
	mpt::osinfo::windows::wine::version Version() const {
		return m_Version;
	}
	std::string VersionAsString() const {
		return VersionAsString(m_Version);
	}
	mpt::osinfo::osclass HostClass() const {
		return m_HostClass;
	}
	std::string RawVersion() const {
		return m_RawVersion;
	}
	std::string RawBuildID() const {
		return m_RawBuildID;
	}
	std::string RawHostSysName() const {
		return m_RawHostSysName;
	}
	std::string RawHostRelease() const {
		return m_RawHostRelease;
	}
public:
	bool IsValid() const {
		return m_Version.is_valid();
	}
	bool IsBefore(mpt::osinfo::windows::wine::version other) const {
		return m_Version.is_before(other);
	}
	bool IsAtLeast(mpt::osinfo::windows::wine::version other) const {
		return m_Version.is_atleast(other);
	}
public:
	static std::string VersionAsString(mpt::osinfo::windows::wine::version version) {
		return mpt::format<std::string>::dec(version.major) + std::string(".") + mpt::format<std::string>::dec(version.minor) + std::string(".") + mpt::format<std::string>::dec(version.update);
	}
	static mpt::osinfo::windows::wine::version ParseVersion(const std::string & rawVersion) {
		if (rawVersion.empty()) {
			return mpt::osinfo::windows::wine::version{0, 0, 0};
		}
		std::vector<uint8> version = mpt::split_parse<uint8>(rawVersion, std::string("."));
		if (version.size() < 2) {
			return mpt::osinfo::windows::wine::version{0, 0, 0};
		}
		std::string parsedVersion = mpt::join_format(version, std::string("."));
		std::size_t len = std::min(parsedVersion.length(), rawVersion.length());
		if (len == 0) {
			return mpt::osinfo::windows::wine::version{0, 0, 0};
		}
		if (parsedVersion.substr(0, len) != rawVersion.substr(0, len)) {
			return mpt::osinfo::windows::wine::version{0, 0, 0};
		}
		return mpt::osinfo::windows::wine::version{version[0], version[1], (version.size() >= 3) ? version[2] : static_cast<uint8>(0)};
	}
private:
	VersionContext(mpt::osinfo::windows::wine::version Version, mpt::osinfo::osclass HostClass, std::string RawVersion, std::string RawBuildID, std::string RawHostSysName, std::string RawHostRelease)
		: m_Version(Version)
		, m_HostClass(HostClass)
		, m_RawVersion(RawVersion)
		, m_RawBuildID(RawBuildID)
		, m_RawHostSysName(RawHostSysName)
		, m_RawHostRelease(RawHostRelease) {
		return;
	}
public:
	static std::optional<VersionContext> Current() {
#if MPT_OS_WINDOWS
		std::optional<mpt::library> NTDLL = mpt::library::load_optional({mpt::library::path_search::system, mpt::library::path_prefix::none, MPT_NATIVE_PATH("ntdll.dll"), mpt::library::path_suffix::none});
		mpt::library::optional_function<const char * __cdecl(void)> wine_get_version{NTDLL, "wine_get_version"};
		mpt::library::optional_function<const char * __cdecl(void)> wine_get_build_id{NTDLL, "wine_get_build_id"};
		mpt::library::optional_function<void __cdecl(const char **, const char **)> wine_get_host_version{NTDLL, "wine_get_host_version"};
		if (!wine_get_version) {
			return std::nullopt;
		}
		const char * wine_version = nullptr;
		const char * wine_build_id = nullptr;
		const char * wine_host_sysname = nullptr;
		const char * wine_host_release = nullptr;
		wine_version = wine_get_version().value_or("");
		wine_build_id = wine_get_build_id().value_or("");
		wine_get_host_version(&wine_host_sysname, &wine_host_release);
		std::string RawVersion = wine_version ? wine_version : "";
		std::string RawBuildID = wine_build_id ? wine_build_id : "";
		std::string RawHostSysName = wine_host_sysname ? wine_host_sysname : "";
		std::string RawHostRelease = wine_host_release ? wine_host_release : "";
		mpt::osinfo::windows::wine::version Version = ParseVersion(RawVersion);
		mpt::osinfo::osclass HostClass = mpt::osinfo::get_class_from_sysname(RawHostSysName);
		return VersionContext{Version, HostClass, RawVersion, RawBuildID, RawHostSysName, RawHostRelease};
#else  // !MPT_OS_WINDOWS
		return std::nullopt;
#endif // MPT_OS_WINDOWS
	}
};

inline std::optional<mpt::osinfo::windows::wine::VersionContext> Current() {
	return mpt::osinfo::windows::wine::VersionContext::Current();
}



} // namespace wine

} // namespace windows

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_WINE_WINE_HPP
