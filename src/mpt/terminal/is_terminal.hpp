/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TERMINAL_IS_TERMINAL_HPP
#define MPT_TERMINAL_IS_TERMINAL_HPP

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/terminal/base.hpp"

#include <optional>

#if MPT_OS_WINDOWS
#include <io.h>
#endif

#if MPT_OS_WINDOWS
#include <windows.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace terminal {



inline bool is_terminal(stdio_fd e) {
#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
	if (!_isatty(detail::get_fd(e))) {
		return false;
	}
	std::optional<HANDLE> handle = detail::get_HANDLE(e);
	if (!handle) {
		return false;
	}
	DWORD mode = 0;
	return (GetConsoleMode(handle.value(), &mode) != FALSE);
#else
	return isatty(detail::get_fd(e)) ? true : false;
#endif
}



} // namespace terminal



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TERMINAL_IS_TERMINAL_HPP
