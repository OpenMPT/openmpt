/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_WINDOWS_HX_VERSION_HPP
#define MPT_OSINFO_WINDOWS_HX_VERSION_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

namespace windows {

namespace hx {



#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT && MPT_ARCH_X86

namespace detail {

[[nodiscard]] inline bool current_is_hx_impl() noexcept {
	HMODULE hDKRNL32 = ::GetModuleHandleA("DKRNL32.DLL");
	if (!hDKRNL32) {
		return false;
	}
	return (::GetProcAddress(hDKRNL32, "GetDKrnl32Version") != NULL);
}

} // namespace detail

[[nodiscard]] inline bool current_is_hx() noexcept {
	static bool s_cached_current_is_hx = detail::current_is_hx_impl();
	return s_cached_current_is_hx;
}

#else

[[nodiscard]] inline constexpr bool current_is_hx() noexcept {
	return false;
}

#endif



struct version {
	uint8 major = 0;
	uint8 minor = 0;
	uint16 patch = 0;
};



} // namespace hx

} // namespace windows

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_WINDOWS_HX_VERSION_HPP
