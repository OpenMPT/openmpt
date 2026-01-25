/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_WINDOWS_WINE_VERSION_HPP
#define MPT_OSINFO_WINDOWS_WINE_VERSION_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/osinfo/windows_version.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

namespace windows {

namespace wine {



#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT

namespace detail {

[[nodiscard]] inline bool current_is_wine_impl() noexcept {
	HMODULE hNTDLL = ::GetModuleHandle(TEXT("ntdll.dll"));
	if (!hNTDLL) {
		return false;
	}
	return (::GetProcAddress(hNTDLL, "wine_get_version") != NULL);
}

} // namespace detail

[[nodiscard]] inline bool current_is_wine() noexcept {
	static bool s_cached_current_is_wine = detail::current_is_wine_impl();
	return s_cached_current_is_wine;
}

#else

[[nodiscard]] inline constexpr bool current_is_wine() noexcept {
	return false;
}

#endif



struct version {
	uint8 major = 0;
	uint8 minor = 0;
	uint8 update = 0;
	[[nodiscard]] constexpr uint32 as_integer() const noexcept {
		uint32 version = 0;
		version |= static_cast<uint32>(major) << 16;
		version |= static_cast<uint32>(minor) << 8;
		version |= static_cast<uint32>(update) << 0;
		return version;
	}
	[[nodiscard]] constexpr bool is_valid() const noexcept {
		return (as_integer() != 0u);
	}
	[[nodiscard]] constexpr bool is_before(mpt::osinfo::windows::wine::version other) const noexcept {
		return as_integer() < other.as_integer();
	}
	[[nodiscard]] constexpr bool is_atleast(mpt::osinfo::windows::wine::version other) const noexcept {
		return as_integer() >= other.as_integer();
	}
};



} // namespace wine

} // namespace windows

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_WINDOWS_WINE_VERSION_HPP
