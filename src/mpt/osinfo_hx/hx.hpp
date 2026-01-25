/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_OSINFO_HX_HX_HPP
#define MPT_OSINFO_HX_HX_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/osinfo/windows_hx_version.hpp"
#include "mpt/string/types.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace osinfo {

namespace windows {

namespace hx {



#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4996)  // 'GetVersionExW': was declared deprecated
#pragma warning(disable : 28159) // Consider using 'IsWindows*' instead of 'GetVersionExW'. Reason: Deprecated. Use VerifyVersionInfo* or IsWindows* macros from VersionHelpers.
#endif
#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

inline std::optional<version> current() {
#if MPT_OS_WINDOWS && !MPT_OS_WINDOWS_WINRT && MPT_ARCH_X86
	// <https://www.japheth.de/HX/DKRNL32.TXT>
	HMODULE hDKRNL32 = ::GetModuleHandleA("DKRNL32.DLL");
	if (!hDKRNL32) {
		return std::nullopt;
	}
	using PGetDKrnl32Version = DWORD (WINAPI *)(void);
	PGetDKrnl32Version pGetDKrnl32Version = (PGetDKrnl32Version)::GetProcAddress(hDKRNL32, "GetDKrnl32Version");
	if (!pGetDKrnl32Version) {
		OSVERSIONINFOA versioninfo{};
		versioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		::GetVersionExA(&versioninfo);
		if ((LOWORD(versioninfo.dwBuildNumber) == 2222) && (versioninfo.szCSDVersion[0] == '\0')) {
			return version{2, 0, 0}; // guess
		} else {
			return version{1, 0, 0}; // guess
		}
	}
	DWORD dwVersion = pGetDKrnl32Version();
	return version{LOBYTE(LOWORD(dwVersion)), HIBYTE(LOWORD(dwVersion)), HIWORD(dwVersion)};
#else
	return std::nullopt;
#endif
}

#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif



} // namespace hx

} // namespace windows

} // namespace osinfo



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_OSINFO_HX_HX_HPP
