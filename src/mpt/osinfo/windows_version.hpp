/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_WINDOWS_VERSION_HPP
#define MPT_OSINFO_WINDOWS_VERSION_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"
#if MPT_OS_WINDOWS
#include "mpt/out_of_memory/out_of_memory.hpp"
#include "mpt/parse/parse.hpp"
#include "mpt/parse/split.hpp"
#include "mpt/string/types.hpp"
#include "mpt/system_error/system_error.hpp"
#endif // MPT_OS_WINDOWS

#if MPT_OS_WINDOWS
#include <algorithm>
#include <optional>
#include <vector>
#endif // MPT_OS_WINDOWS

#if MPT_OS_WINDOWS
#include <cstring>
#endif // MPT_OS_WINDOWS

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

namespace windows {



// clang-format off
enum class Epoch : int8 {
	unknown = -128,
	Win16   = -1,
	Win32s  =  0,
	Win9x   =  1,
	WinNT   =  2,
	modern  = WinNT,
};
// clang-format on

namespace detail {

constexpr uint64 VersionNumber(uint32 major, uint32 minor) noexcept {
	return (static_cast<uint64>(major) << 32) | (static_cast<uint64>(minor) << 0);
}

} // namespace detail

// clang-format off
enum class Number : uint64 {
	Win30      = detail::VersionNumber( 3,  0),
	Win31      = detail::VersionNumber( 3, 10),
	Win311     = detail::VersionNumber( 3, 11),
			
	Win95      = detail::VersionNumber( 4,  0),
	Win95OSR21 = detail::VersionNumber( 4,  1),
	Win95C     = detail::VersionNumber( 4,  3),
	Win98      = detail::VersionNumber( 4, 10),
	WinME      = detail::VersionNumber( 4, 90),

	WinNT31    = detail::VersionNumber( 3, 10),
	WinNT35    = detail::VersionNumber( 3, 50),
	WinNT351   = detail::VersionNumber( 3, 51),
	WinNT4     = detail::VersionNumber( 4,  0),
	Win2000    = detail::VersionNumber( 5,  0),
	WinXP      = detail::VersionNumber( 5,  1),
	WinXP64    = detail::VersionNumber( 5,  2),
	WinVista   = detail::VersionNumber( 6,  0),
	Win7       = detail::VersionNumber( 6,  1),
	Win8       = detail::VersionNumber( 6,  2),
	Win81      = detail::VersionNumber( 6,  3),

	Win10Pre   = detail::VersionNumber( 6,  4),
	Win10      = detail::VersionNumber(10,  0),
};
// clang-format on

struct System {
	uint32 Major = 0;
	uint32 Minor = 0;
	constexpr System() noexcept
		: Major(0)
		, Minor(0) {
	}
	constexpr System(Number number) noexcept
		: Major(static_cast<uint32>((static_cast<uint64>(number) >> 32) & 0xffffffffu))
		, Minor(static_cast<uint32>((static_cast<uint64>(number) >> 0) & 0xffffffffu)) {
	}
	explicit constexpr System(uint64 number) noexcept
		: Major(static_cast<uint32>((number >> 32) & 0xffffffffu))
		, Minor(static_cast<uint32>((number >> 0) & 0xffffffffu)) {
	}
	explicit constexpr System(uint32 major, uint32 minor) noexcept
		: Major(major)
		, Minor(minor) {
	}
	constexpr operator uint64() const noexcept {
		return (static_cast<uint64>(Major) << 32) | (static_cast<uint64>(Minor) << 0);
	}
};

struct ServicePack {
	uint16 Major = 0;
	uint16 Minor = 0;
	constexpr ServicePack() noexcept
		: Major(0)
		, Minor(0) {
	}
	explicit constexpr ServicePack(uint16 major, uint16 minor) noexcept
		: Major(major)
		, Minor(minor) {
	}
	constexpr bool HasServicePack() const noexcept {
		return Major != 0 || Minor != 0;
	}
	constexpr operator uint32() const noexcept {
		return (static_cast<uint32>(Major) << 16) | (static_cast<uint32>(Minor) << 0);
	}
};

struct InternetExplorer {
	uint8 Major = 0;
	uint16 Minor = 0;
	uint16 Build = 0;
	constexpr InternetExplorer() noexcept
		: Major(0)
		, Minor(0)
		, Build(0) {
	}
	explicit constexpr InternetExplorer(uint8 major, uint16 minor, uint16 build = 0) noexcept
		: Major(major)
		, Minor(minor)
		, Build(build) {
	}
	constexpr bool HasInternetExplorer() const noexcept {
		return Major != 0 || Minor != 0 || Build != 0;
	}
	constexpr operator uint64() const noexcept {
		return (static_cast<uint64>(Major) << 32) | (static_cast<uint64>(Minor) << 16) | (static_cast<uint64>(Build) << 0);
	}
};

using Build = uint32;

using Suite = uint16;
using NT = uint8;
using Product = uint32;



class Version {

public:

	// clang-format off
	static inline constexpr Number Win30      = mpt::osinfo::windows::Number::Win30;
	static inline constexpr Number Win31      = mpt::osinfo::windows::Number::Win31;
	static inline constexpr Number Win311     = mpt::osinfo::windows::Number::Win311;

	static inline constexpr Number Win95      = mpt::osinfo::windows::Number::Win95;
	static inline constexpr Number Win95OSR21 = mpt::osinfo::windows::Number::Win95OSR21;
	static inline constexpr Number Win95C     = mpt::osinfo::windows::Number::Win95C;
	static inline constexpr Number Win98      = mpt::osinfo::windows::Number::Win98;
	static inline constexpr Number WinME      = mpt::osinfo::windows::Number::WinME;

	static inline constexpr Number WinNT31    = mpt::osinfo::windows::Number::WinNT31;
	static inline constexpr Number WinNT35    = mpt::osinfo::windows::Number::WinNT35;
	static inline constexpr Number WinNT351   = mpt::osinfo::windows::Number::WinNT351;
	static inline constexpr Number WinNT4     = mpt::osinfo::windows::Number::WinNT4;
	static inline constexpr Number Win2000    = mpt::osinfo::windows::Number::Win2000;
	static inline constexpr Number WinXP      = mpt::osinfo::windows::Number::WinXP;
	static inline constexpr Number WinXP64    = mpt::osinfo::windows::Number::WinXP64;
	static inline constexpr Number WinVista   = mpt::osinfo::windows::Number::WinVista;
	static inline constexpr Number Win7       = mpt::osinfo::windows::Number::Win7;
	static inline constexpr Number Win8       = mpt::osinfo::windows::Number::Win8;
	static inline constexpr Number Win81      = mpt::osinfo::windows::Number::Win81;

	static inline constexpr Number Win10Pre   = mpt::osinfo::windows::Number::Win10Pre;
	static inline constexpr Number Win10      = mpt::osinfo::windows::Number::Win10;
	// clang-format on

private:
	bool m_SystemIsWindows = false;

	Epoch m_Epoch = Epoch::unknown;
	System m_System = System{0, 0};
	ServicePack m_ServicePack = ServicePack{0, 0};
	InternetExplorer m_InternetExplorer = InternetExplorer{0, 0};
	Build m_Build = 0;

	Suite m_Suite = 0x0000;
	NT m_NT = 0x00;
	Product m_Product = 000000000;

private:
	constexpr Version() noexcept
		: m_SystemIsWindows{false}
		, m_Epoch{Epoch::unknown}
		, m_System{0, 0}
		, m_ServicePack{0, 0}
		, m_InternetExplorer{0, 0}
		, m_Build{0} {
		return;
	}

public:
	static constexpr Version NoWindows() noexcept {
		return Version();
	}

	static constexpr Version AnyWindows() noexcept {
		Version result = Version();
		result.m_SystemIsWindows = true;
		return result;
	}

	constexpr Version(Epoch epoch, System system, ServicePack servicePack, InternetExplorer internetExplorer, Build build, Suite suite = 0, NT nt = 0, Product product = 0) noexcept
		: m_SystemIsWindows{true}
		, m_Epoch{epoch}
		, m_System{system}
		, m_ServicePack{servicePack}
		, m_InternetExplorer{internetExplorer}
		, m_Build{build}
		, m_Suite{suite}
		, m_NT{nt}
		, m_Product{product} {
		return;
	}



public:



#if MPT_OS_WINDOWS



	MPT_ATTR_ALWAYSINLINE MPT_CONSTEVAL static Version FromSDK() noexcept {
		// Initialize to used SDK version
#if 0

//#elif MPT_WINNT_AT_LEAST(MPT_WIN_11_25H2)
//		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplror{0, 0}, 26200);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_11_24H2)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 26100);
//#elif MPT_WINNT_AT_LEAST(MPT_WIN_11_23H2)
//		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 22631);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_11_22H2)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 22621);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_11) // 21H2
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 22000);

//#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_22H2)
//		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 19045);
//#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_21H2)
//		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 19044);
//#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_21H1)
//		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 19043);
//#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_20H2)
//		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 19042);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_2004)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 19041);
//#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1909)
//		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 18363);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1903)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 18362);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1809)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 17763);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1803)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 17134);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1709)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 16299);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1703)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 15063);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1607)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 14393);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_1511)
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 10586);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_10) // 1507
		return Version(Epoch::WinNT, Win10, ServicePack{0, 0}, InternetExplorer{0, 0}, 10240);

#elif MPT_WINNT_AT_LEAST(MPT_WIN_10_PRE)
		return Version(Epoch::WinNT, Win10Pre, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);

#elif MPT_WINNT_AT_LEAST(MPT_WIN_81)
		return Version(Epoch::WinNT, Win81, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_8)
		return Version(Epoch::WinNT, Win8, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_7)
		return Version(Epoch::WinNT, Win7, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
		return Version(Epoch::WinNT, WinVista, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);

#elif MPT_WINNT_AT_LEAST(MPT_WIN_XP64)
		return Version(Epoch::WinNT, WinXP64, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_XP)
		return Version(Epoch::WinNT, WinXP, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_2000)
		return Version(Epoch::WinNT, Win2000, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);

#elif MPT_WINNT_AT_LEAST(MPT_WIN_NT4)
		return Version(Epoch::WinNT, WinNT4, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);

#elif MPT_WINNT_AT_LEAST(MPT_WIN_NT351)
		return Version(Epoch::WinNT, WinNT351, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_NT35)
		return Version(Epoch::WinNT, WinNT35, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);
#elif MPT_WINNT_AT_LEAST(MPT_WIN_NT31)
		return Version(Epoch::WinNT, WinNT31, ServicePack{((NTDDI_VERSION & 0xffffu) >> 8) & 0xffu, ((NTDDI_VERSION & 0xffffu) >> 0) & 0xffu}, InternetExplorer{0, 0}, 0);

#elif MPT_OS_WINDOWS_WINNT
		return Version(Epoch::WinNT, System{(MPT_WIN_VERSION & 0xff000000u) >> 24, (MPT_WIN_VERSION & 0x00ff0000u) >> 16}, ServicePack{(MPT_WIN_VERSION & 0x0000ff00u) >> 8, (MPT_WIN_VERSION & 0x000000ffu) >> 0}, InternetExplorer{0, 0}, 0);

#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WINME_IE5)
		return Version(Epoch::Win9x, WinME, ServicePack{0, 0}, InternetExplorer{5, 0}, 3000);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WINME)
		return Version(Epoch::Win9x, WinME, ServicePack{0, 0}, InternetExplorer{0, 0}, 3000);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN98SE_IE5)
		return Version(Epoch::Win9x, Win98, ServicePack{0, 0}, InternetExplorer{5, 0}, 2222);
//#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN98SE)
//		return Version(Epoch::Win9x, Win98, ServicePack{0, 0}, InternetExplorer{0, 0}, 2222);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN98_IE4)
		return Version(Epoch::Win9x, Win98, ServicePack{0, 0}, InternetExplorer{4, 0}, 1998);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN98)
		return Version(Epoch::Win9x, Win98, ServicePack{0, 0}, InternetExplorer{0, 0}, 1998);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95C_IE4)
		return Version(Epoch::Win9x, Win95C, ServicePack{0, 0}, InternetExplorer{4, 0}, 1216);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95C)
		return Version(Epoch::Win9x, Win95C, ServicePack{0, 0}, InternetExplorer{0, 0}, 1216);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95OSR21_IE3)
		return Version(Epoch::Win9x, Win95OSR21, ServicePack{0, 0}, InternetExplorer{3, 0}, 1212);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95OSR21)
		return Version(Epoch::Win9x, Win95OSR21, ServicePack{0, 0}, InternetExplorer{0, 0}, 1212);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95OSR2_IE3)
		return Version(Epoch::Win9x, Win95, ServicePack{0, 0}, InternetExplorer{3, 0}, 1111);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95OSR1_IE2)
		return Version(Epoch::Win9x, Win95, ServicePack{0, 0}, InternetExplorer{2, 0}, 951);
//#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95PLUS_IE1)
//		return Version(Epoch::Win9x, Win95, ServicePack{0, 0}, InternetExplorer{1, 0}, 950);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95_IE1)
		return Version(Epoch::Win9x, Win95, ServicePack{0, 0}, InternetExplorer{1, 0}, 950);
#elif MPT_WIN9X_AT_LEAST(MPT_WIN_WIN95)
		return Version(Epoch::Win9x, Win95, ServicePack{0, 0}, InternetExplorer{0, 0}, 950);

#elif MPT_OS_WINDOWS_WIN9X
		return Version(Epoch::Win9x, System{(MPT_WIN_VERSION & 0xff000000u) >> 24, (MPT_WIN_VERSION & 0x00ff0000u) >> 16}, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);

#elif MPT_WIN32_AT_LEAST(MPT_WIN_WIN311)
		return Version(Epoch::Win32s, Win311, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);
#elif MPT_WIN32_AT_LEAST(MPT_WIN_WIN31)
		return Version(Epoch::Win32s, Win31, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);
#elif MPT_WIN32_AT_LEAST(MPT_WIN_WIN30)
		return Version(Epoch::Win32s, Win30, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);

#elif MPT_OS_WINDOWS_WIN32
		return Version(Epoch::Win32s, System{(MPT_WIN_VERSION & 0xff000000u) >> 24, (MPT_WIN_VERSION & 0x00ff0000u) >> 16}, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);

#elif defined(NTDDI_VERSION)
		return Version(Epoch::WinNT, System{(NTDDI_VERSION & 0xff000000u) >> 24, (NTDDI_VERSION & 0x00ff0000u) >> 16}, ServicePack{(NTDDI_VERSION & 0x0000ff00u) >> 8, (NTDDI_VERSION & 0x000000ffu) >> 0}, InternetExplorer{0, 0}, 0);
#elif defined(_WIN32_WINNT)
		return Version(Epoch::WinNT, System{(_WIN32_WINNT & 0xff00u) >> 8, (_WIN32_WINNT & 0x00ffu) >> 0}, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);
#elif defined(_WIN32_WINDOWS) && defined(_WIN32_IE)
		return Version(Epoch::Win9x, System{(_WIN32_WINDOWS & 0xff00u) >> 8, (_WIN32_WINDOWS & 0x00ffu) >> 0}, ServicePack{0, 0}, InternetExplorer{(_WIN32_IE & 0xff00u) >> 8, (_WIN32_IE & 0x00ffu) >> 0}, 0);
#elif defined(_WIN32_WINDOWS)
		return Version(Epoch::Win9x, System{(_WIN32_WINDOWS & 0xff00u) >> 8, (_WIN32_WINDOWS & 0x00ffu) >> 0}, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);
#elif defined(WINVER)
		return Version(Epoch::Win32s, System{(WINVER & 0xff00u) >> 8, (WINVER & 0x00ffu) >> 0}, ServicePack{0, 0}, InternetExplorer{0, 0}, 0);

#else
		return Version(Epoch::Win16, System{0, 0}, ServicePack{0, 0}, 0);

#endif
	}



#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4996)  // 'GetVersionExW': was declared deprecated
#pragma warning(disable : 28159) // Consider using 'IsWindows*' instead of 'GetVersionExW'. Reason: Deprecated. Use VerifyVersionInfo* or IsWindows* macros from VersionHelpers.
#endif
#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif


	// <https://en.wikipedia.org/wiki/Microsoft_Windows_version_history>
	// <https://en.wikipedia.org/wiki/List_of_Microsoft_Windows_versions>
	// <https://en.wikipedia.org/wiki/Windows_95>
	// <https://www.gaijin.at/de/infos/windows-versionsnummern>
	// <https://www.mdgx.com/ver.htm>
	// <https://gitlab.winehq.org/wine/wine/-/blob/master/dlls/ntdll/version.c>
	// <https://gitlab.winehq.org/wine/wine/-/blob/master/programs/winecfg/appdefaults.c>
	// VS6 MSDN sysinfo.chm GetVersion/GetVersionEx/OSVERSIONINFO/OSVERSIONINFOEX
	// VS6 MSDN win32.chm / Windows 9x / Release Notes / Detecting
	// MinGW w32api.h
	// MinGW sdkddkver.h


	// clang-format off

	template <typename TGetVersion>
	MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<Version> ParseGetVersion(TGetVersion pGetVersion) noexcept {
		DWORD version = pGetVersion();
		if ((version == 0x0000'0000u) || (version == 0x8000'0000u)) {
			return std::nullopt;
		}
		// ATTENTION: major in lobyte, minor in hibyte!!!
		uint32 versionmajor = LOBYTE(LOWORD(version));
		uint32 versionminor = HIBYTE(LOWORD(version));
		bool versionnt = (HIWORD(version) & 0x8000u) ? false : true;
		uint32 versionbuild = (versionnt || (versionmajor < 4)) ? (HIWORD(version) & 0x7fffu) : 0;
		if (!versionnt && (versionmajor <= 3)) {
			return Version{
				Epoch::Win32s,
				System{versionmajor, versionminor},
				ServicePack{0, 0},
				InternetExplorer{0, 0},
				versionbuild};
		}
		return Version{
			versionnt ? Epoch::WinNT : Epoch::Win9x,
			System{versionmajor, versionminor},
			ServicePack{0, 0},
			InternetExplorer{0, 0},
			versionbuild};
	}


	template <typename TGetVersionEx>
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<Version> ParseGetVersionExOSVERSIONINFO(TGetVersionEx pGetVersionEx) noexcept {
		OSVERSIONINFO versioninfo{};
		versioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (pGetVersionEx(&versioninfo) == FALSE) {
			return std::nullopt;
		}
		if ((versioninfo.dwPlatformId == VER_PLATFORM_WIN32s) || (versioninfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)) {
			// <https://winapi.freetechsecrets.com/win32/WIN32OSVERSIONINFO.htm>:
			// "Windows 95: Identifies the build number of the operating system in the low-order word.
			// The high-order word contains the major and minor version numbers."
			uint32 buildmajor = HIBYTE(HIWORD(versioninfo.dwBuildNumber));
			uint32 buildminor = LOBYTE(HIWORD(versioninfo.dwBuildNumber));
			uint32 buildbuild = LOWORD(versioninfo.dwBuildNumber);
			versioninfo.dwMajorVersion = std::max<uint32>(versioninfo.dwMajorVersion, buildmajor);
			versioninfo.dwMinorVersion = std::max<uint32>(versioninfo.dwMinorVersion, buildminor);
			versioninfo.dwBuildNumber = buildbuild;
			return Version{
				(versioninfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? Epoch::Win9x : Epoch::Win32s,
				System{versioninfo.dwMajorVersion, versioninfo.dwMinorVersion},
				ServicePack{0, 0},
				InternetExplorer{0, 0},
				versioninfo.dwBuildNumber};
		} else if (versioninfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			return Version{
				Epoch::WinNT,
				System{versioninfo.dwMajorVersion, versioninfo.dwMinorVersion},
				ServicePack{0, 0},
				InternetExplorer{0, 0},
				versioninfo.dwBuildNumber};
		}
		// fallback to modern WinNT for unknown epoch
		return Version{
			Epoch::modern,
			System{versioninfo.dwMajorVersion, versioninfo.dwMinorVersion},
			ServicePack{0, 0},
			InternetExplorer{0, 0},
			versioninfo.dwBuildNumber};
	}


	template <typename TGetVersionEx>
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<Version> ParseGetVersionExOSVERSIONINFOEX(TGetVersionEx pGetVersionEx) noexcept {
		OSVERSIONINFOEX versioninfoex{};
		versioninfoex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		if (pGetVersionEx((OSVERSIONINFO *)&versioninfoex) == FALSE) {
			return std::nullopt;
		}
		return Version{
			(versioninfoex.dwPlatformId == VER_PLATFORM_WIN32_NT) ? Epoch::WinNT :
			(versioninfoex.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? Epoch::Win9x :
			(versioninfoex.dwPlatformId == VER_PLATFORM_WIN32s) ? Epoch::Win32s :
			Epoch::modern,
			System{versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion},
			ServicePack{versioninfoex.wServicePackMajor, versioninfoex.wServicePackMinor},
			InternetExplorer{0, 0},
			versioninfoex.dwBuildNumber,
			versioninfoex.wSuiteMask,
			versioninfoex.wProductType};
	}


	template <typename TGetVersionEx>
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<Version> ParseGetVersionExOSVERSIONINFOEX(TGetVersionEx pGetVersionEx, std::optional<OSVERSIONINFOEX> & oversioninfoex) noexcept {
		OSVERSIONINFOEX versioninfoex{};
		versioninfoex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		if (pGetVersionEx((OSVERSIONINFO *)&versioninfoex) == FALSE) {
			oversioninfoex = std::nullopt;
			return std::nullopt;
		}
		oversioninfoex = versioninfoex;
		return Version{
			(versioninfoex.dwPlatformId == VER_PLATFORM_WIN32_NT) ? Epoch::WinNT :
			(versioninfoex.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ? Epoch::Win9x :
			(versioninfoex.dwPlatformId == VER_PLATFORM_WIN32s) ? Epoch::Win32s :
			Epoch::modern,
			System{versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion},
			ServicePack{versioninfoex.wServicePackMajor, versioninfoex.wServicePackMinor},
			InternetExplorer{0, 0},
			versioninfoex.dwBuildNumber,
			versioninfoex.wSuiteMask,
			versioninfoex.wProductType};
	}


	template <typename TCheckRegistryOutOfMemory>
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<InternetExplorer> ParseHKLMSOFTWAREMicrosoftInternetExplorer(TCheckRegistryOutOfMemory pCheckRegistryOutOfMemory) {
		std::optional<InternetExplorer> result;
		HKEY kInternetExplorer = NULL;
		if (pCheckRegistryOutOfMemory(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Internet Explorer"), 0, KEY_READ, &kInternetExplorer)) != ERROR_SUCCESS) {
			return result;
		}
		if (!kInternetExplorer) {
			return result;
		}
		mpt::winstring version;
		try {
			if (version.length() == 0) {
				DWORD dwType = 0;
				DWORD dwSize = 0;
				if (pCheckRegistryOutOfMemory(::RegQueryValueEx(kInternetExplorer, TEXT("svcVersion"), NULL, &dwType, NULL, &dwSize)) == ERROR_SUCCESS) {
					if ((dwType == REG_SZ) && (dwSize > 0)) {
						std::vector<BYTE> bbuf(dwSize);
						if (pCheckRegistryOutOfMemory(::RegQueryValueEx(kInternetExplorer, TEXT("svcVersion"), NULL, &dwType, bbuf.data(), &dwSize)) == ERROR_SUCCESS) {
							std::vector<TCHAR> cbuf(dwSize / sizeof(TCHAR));
							DWORD minsize = std::min(static_cast<DWORD>(bbuf.size()), dwSize);
							std::memcpy(cbuf.data(), bbuf.data(), minsize);
							version = mpt::winstring(cbuf.data(), cbuf.size()).c_str();
						}
					}
				}
			}
			if (version.length() == 0) {
				DWORD dwType = 0;
				DWORD dwSize = 0;
				if (pCheckRegistryOutOfMemory(::RegQueryValueEx(kInternetExplorer, TEXT("Version"), NULL, &dwType, NULL, &dwSize)) == ERROR_SUCCESS) {
					if ((dwType == REG_SZ) && (dwSize > 0)) {
						std::vector<BYTE> bbuf(dwSize);
						if (pCheckRegistryOutOfMemory(::RegQueryValueEx(kInternetExplorer, TEXT("Version"), NULL, &dwType, bbuf.data(), &dwSize)) == ERROR_SUCCESS) {
							std::vector<TCHAR> cbuf(dwSize / sizeof(TCHAR));
							DWORD minsize = std::min(static_cast<DWORD>(bbuf.size()), dwSize);
							std::memcpy(cbuf.data(), bbuf.data(), minsize);
							version = mpt::winstring(cbuf.data(), cbuf.size()).c_str();
						}
					}
				}
			}
		} catch (mpt::out_of_memory e) {
			pCheckRegistryOutOfMemory(::RegCloseKey(kInternetExplorer));
			mpt::rethrow_out_of_memory(e);
		}
		pCheckRegistryOutOfMemory(::RegCloseKey(kInternetExplorer));
		if (version.length() == 0) {
			return result;
		}
		std::vector<mpt::winstring> fields = mpt::split<mpt::winstring>(version, TEXT("."));
		uint8 major = 0;
		uint16 minor = 0;
		uint16 build = 0;
		if (fields.size() > 0) {
			major = mpt::parse<uint8>(fields[0]);
		}
		if (fields.size() > 1) {
			minor = mpt::parse<uint16>(fields[1]);
		}
		if (fields.size() > 2) {
			build = mpt::parse<uint16>(fields[2]);
		}
		return InternetExplorer{major, minor, build};
	}


	template <typename TGetProductInfo>
	[[nodiscard]] MPT_ATTR_ALWAYSINLINE MPT_INLINE_FORCE static std::optional<DWORD> ParseGetProductInfo(TGetProductInfo pGetProductInfo, const OSVERSIONINFOEX & versioninfoex) noexcept {
		DWORD dwProduct = 0;
		if (pGetProductInfo(versioninfoex.dwMajorVersion, versioninfoex.dwMinorVersion, versioninfoex.wServicePackMajor, versioninfoex.wServicePackMinor, &dwProduct) == FALSE) {
			return std::nullopt;
		}
		return dwProduct;
	}

	// clang-format on

	MPT_ATTR_NOINLINE MPT_DECL_NOINLINE static Version GatherWindowsVersionBaseOS() noexcept {

		Version result = FromSDK();

#if MPT_OS_WINDOWS_WINRT

		result = ParseGetVersionExOSVERSIONINFOEX(&::GetVersionEx).value_or(result);

#elif MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)

		std::optional<OSVERSIONINFOEX> oversioninfoex;
		result = ParseGetVersionExOSVERSIONINFOEX(&::GetVersionEx, oversioninfoex).value_or(result);

		if (!oversioninfoex) {
			return result;
		}

		result.m_Product = ParseGetProductInfo(&::GetProductInfo, oversioninfoex.value()).value_or(result.m_Product);

#elif MPT_WINNT_AT_LEAST(MPT_WIN_NT4SP6)

		std::optional<OSVERSIONINFOEX> oversioninfoex;
		result = ParseGetVersionExOSVERSIONINFOEX(&::GetVersionEx, oversioninfoex).value_or(result);

		if (!result.IsAtLeast(Epoch::WinNT, Version::WinVista, ServicePack{0, 0}, InternetExplorer{0, 0}, 0)) {
			return result;
		}

		if (!oversioninfoex) {
			return result;
		}

		HMODULE hKernel32DLL = ::GetModuleHandle(TEXT("kernel32.dll"));
		if (!hKernel32DLL) {
			return result;
		}

		using PGetProductInfo = BOOL(WINAPI *)(DWORD dwOSMajorVersion, DWORD dwOSMinorVersion, DWORD dwSpMajorVersion, DWORD dwSpMinorVersion, PDWORD pdwReturnedProductType);
		PGetProductInfo pGetProductInfo = (PGetProductInfo)::GetProcAddress(hKernel32DLL, "GetProductInfo");
		if (!pGetProductInfo) {
			return result;
		}
		result.m_Product = ParseGetProductInfo(pGetProductInfo, oversioninfoex.value()).value_or(result.m_Product);

#elif (MPT_OS_WINDOWS_WIN9X || MPT_WINNT_AT_LEAST(MPT_WIN_NT35))

		result = ParseGetVersionExOSVERSIONINFO(&::GetVersionEx).value_or(result);

		if (!result.IsAtLeast(Epoch::WinNT, Version::WinNT35, ServicePack{0, 0}, InternetExplorer{0, 0}, 0)) {
			return result;
		}
		std::optional<OSVERSIONINFOEX> oversioninfoex;
		result = ParseGetVersionExOSVERSIONINFOEX(&::GetVersionEx, oversioninfoex).value_or(result);

		if (!result.IsAtLeast(Epoch::WinNT, Version::WinVista, ServicePack{0, 0}, InternetExplorer{0, 0}, 0)) {
			return result;
		}

		if (!oversioninfoex) {
			return result;
		}

		HMODULE hKernel32DLL = ::GetModuleHandle(TEXT("kernel32.dll"));
		if (!hKernel32DLL) {
			return result;
		}

		using PGetProductInfo = BOOL(WINAPI *)(DWORD dwOSMajorVersion, DWORD dwOSMinorVersion, DWORD dwSpMajorVersion, DWORD dwSpMinorVersion, PDWORD pdwReturnedProductType);
		PGetProductInfo pGetProductInfo = (PGetProductInfo)::GetProcAddress(hKernel32DLL, "GetProductInfo");
		if (!pGetProductInfo) {
			return result;
		}
		result.m_Product = ParseGetProductInfo(pGetProductInfo, oversioninfoex.value()).value_or(result.m_Product);

#else

		result = ParseGetVersion(&::GetVersion).value_or(result);

		if (!(result.IsAtLeast(Epoch::Win9x, Version::Win95, ServicePack{0, 0}, InternetExplorer{0, 0}, 0) || result.IsAtLeast(Epoch::WinNT, Version::WinNT35, ServicePack{0, 0}, InternetExplorer{0, 0}, 0))) {
			return result;
		}


		HMODULE hKernel32DLL = ::GetModuleHandle(TEXT("kernel32.dll"));
		if (!hKernel32DLL) {
			return result;
		}

		using PGetVersionEx = BOOL(WINAPI *)(LPOSVERSIONINFO lpVersionInformation);
		PGetVersionEx pGetVersionEx = nullptr;
#if defined(UNICODE)
		pGetVersionEx = (PGetVersionEx)::GetProcAddress(hKernel32DLL, "GetVersionExW");
#else
		pGetVersionEx = (PGetVersionEx)::GetProcAddress(hKernel32DLL, "GetVersionExA");
#endif
		if (!pGetVersionEx) {
			return result;
		}

		result = ParseGetVersionExOSVERSIONINFO(pGetVersionEx).value_or(result);

		if (!result.IsAtLeast(Epoch::WinNT, Version::WinNT35, ServicePack{0, 0}, InternetExplorer{0, 0}, 0)) {
			return result;
		}
		std::optional<OSVERSIONINFOEX> oversioninfoex;
		result = ParseGetVersionExOSVERSIONINFOEX(pGetVersionEx, oversioninfoex).value_or(result);

		if (!result.IsAtLeast(Epoch::WinNT, Version::WinVista, ServicePack{0, 0}, InternetExplorer{0, 0}, 0)) {
			return result;
		}

		if (!oversioninfoex) {
			return result;
		}

		using PGetProductInfo = BOOL(WINAPI *)(DWORD dwOSMajorVersion, DWORD dwOSMinorVersion, DWORD dwSpMajorVersion, DWORD dwSpMinorVersion, PDWORD pdwReturnedProductType);
		PGetProductInfo pGetProductInfo = (PGetProductInfo)::GetProcAddress(hKernel32DLL, "GetProductInfo");
		if (!pGetProductInfo) {
			return result;
		}
		result.m_Product = ParseGetProductInfo(pGetProductInfo, oversioninfoex.value()).value_or(result.m_Product);

#endif

		return result;
	}


#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif



	static inline Version GatherWindowsVersion() {
		Version result = GatherWindowsVersionBaseOS();
#if !MPT_OS_WINDOWS_WINNT
		if (result.Is(Epoch::Win9x)) {
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
			result.m_InternetExplorer = ParseHKLMSOFTWAREMicrosoftInternetExplorer(&mpt::windows::CheckLSTATUSOutOfMemory).value_or(result.m_InternetExplorer);
#else
			result.m_InternetExplorer = ParseHKLMSOFTWAREMicrosoftInternetExplorer(&mpt::windows::CheckLLONGOutOfMemory).value_or(result.m_InternetExplorer);
#endif
		}
#endif
		return result;
	}



#endif // MPT_OS_WINDOWS



public:
	static inline mpt::osinfo::windows::Version Current() {
#if MPT_OS_WINDOWS
		static mpt::osinfo::windows::Version s_cachedVersion = GatherWindowsVersion();
		return s_cachedVersion;
#else  // !MPT_OS_WINDOWS
		return mpt::osinfo::windows::Version::NoWindows();
#endif // MPT_OS_WINDOWS
	}

public:
	bool IsWindows() const noexcept {
		return m_SystemIsWindows;
	}

	bool Is(Epoch epoch) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		return (m_Epoch == epoch);
	}

	bool Is(Version version) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_Epoch != version.GetEpoch()) {
			return false;
		}
		if (m_System != version.GetSystem()) {
			return false;
		}
		if (m_ServicePack != version.GetServicePack()) {
			return false;
		}
		return (m_Build >= version.GetBuild());
	}
	bool Is(Epoch epoch, System system, Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_Epoch != epoch) {
			return false;
		}
		if (m_System != system) {
			return false;
		}
		if ((m_Build == 0) || (build == 0)) {
			return true;
		}
		return (m_Build >= build);
	}

	bool IsBefore(System version) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		return m_System < version;
	}

	bool IsBefore(System version, ServicePack servicePack) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		return m_ServicePack < servicePack;
	}

	bool IsBefore(System version, InternetExplorer internetExplorer) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		return m_InternetExplorer < internetExplorer;
	}

	bool IsBefore(System version, Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		return m_Build < build;
	}

	bool IsBefore(System version, ServicePack servicePack, Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		if (m_ServicePack > servicePack) {
			return false;
		}
		if (m_ServicePack < servicePack) {
			return true;
		}
		return m_Build < build;
	}

	bool IsBefore(Epoch epoch, System version, ServicePack servicePack, InternetExplorer internetExplorer, Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_Epoch < epoch) {
			return true;
		}
		if (m_System > version) {
			return false;
		}
		if (m_System < version) {
			return true;
		}
		if (m_ServicePack > servicePack) {
			return false;
		}
		if (m_ServicePack < servicePack) {
			return true;
		}
		if (m_InternetExplorer > internetExplorer) {
			return false;
		}
		if (m_InternetExplorer < internetExplorer) {
			return true;
		}
		return m_Build < build;
	}

	bool IsBefore(mpt::osinfo::windows::Version version) const noexcept {
		return IsBefore(version.GetEpoch(), version.GetSystem(), version.GetServicePack(), version.GetInternetExplorer(), version.GetBuild());
	}

	bool IsAtLeast(System version) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		return m_System >= version;
	}

	bool IsAtLeast(System version, ServicePack servicePack) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		return m_ServicePack >= servicePack;
	}

	bool IsAtLeast(System version, InternetExplorer internetExplorer) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		return m_InternetExplorer >= internetExplorer;
	}

	bool IsAtLeast(System version, Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		return m_Build >= build;
	}

	bool IsAtLeast(System version, ServicePack servicePack, Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		if (m_ServicePack < servicePack) {
			return false;
		}
		if (m_ServicePack > servicePack) {
			return true;
		}
		return m_Build >= build;
	}

	bool IsAtLeast(Epoch epoch, System version, ServicePack servicePack, InternetExplorer internetExplorer, Build build) const noexcept {
		if (!m_SystemIsWindows) {
			return false;
		}
		if (m_Epoch < epoch) {
			return false;
		}
		if (m_System < version) {
			return false;
		}
		if (m_System > version) {
			return true;
		}
		if (m_ServicePack < servicePack) {
			return false;
		}
		if (m_ServicePack > servicePack) {
			return true;
		}
		if (m_InternetExplorer < internetExplorer) {
			return false;
		}
		if (m_InternetExplorer > internetExplorer) {
			return true;
		}
		return m_Build >= build;
	}

	bool IsAtLeast(mpt::osinfo::windows::Version version) const noexcept {
		return IsAtLeast(version.GetEpoch(), version.GetSystem(), version.GetServicePack(), version.GetInternetExplorer(), version.GetBuild());
	}

	mpt::osinfo::windows::Epoch GetEpoch() const noexcept {
		return m_Epoch;
	}

	mpt::osinfo::windows::System GetSystem() const noexcept {
		return m_System;
	}

	mpt::osinfo::windows::ServicePack GetServicePack() const noexcept {
		return m_ServicePack;
	}

	mpt::osinfo::windows::InternetExplorer GetInternetExplorer() const noexcept {
		return m_InternetExplorer;
	}

	mpt::osinfo::windows::Build GetBuild() const noexcept {
		return m_Build;
	}

	mpt::osinfo::windows::Suite GetSuite() const noexcept {
		return m_Suite;
	}

	mpt::osinfo::windows::NT GetNT() const noexcept {
		return m_NT;
	}

	mpt::osinfo::windows::Product GetProduct() const noexcept {
		return m_Product;
	}

}; // class Version



} // namespace windows

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_WINDOWS_VERSION_HPP
