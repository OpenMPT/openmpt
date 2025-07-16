/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TERMINAL_BASE_HPP
#define MPT_TERMINAL_BASE_HPP

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"

#include <optional>

#include <cstddef>
#if MPT_OS_WINDOWS
#include <cstdio>
#endif

#if MPT_OS_WINDOWS
#include <stdio.h>
#endif

#if !MPT_OS_WINDOWS
#include <unistd.h>
#endif

#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
#include <windows.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace terminal {



enum class stdio_fd {
	in,
	out,
	err,
};



namespace detail {



inline int get_fd(stdio_fd e) {
	int fd = -1;
	switch (e) {
		case stdio_fd::in:
#if MPT_OS_WINDOWS
			fd = _fileno(stdin);
#else
			fd = STDIN_FILENO;
#endif
			break;
		case stdio_fd::out:
#if MPT_OS_WINDOWS
			fd = _fileno(stdout);
#else
			fd = STDOUT_FILENO;
#endif
			break;
		case stdio_fd::err:
#if MPT_OS_WINDOWS
			fd = _fileno(stderr);
#else
			fd = STDERR_FILENO;
#endif
			break;
	}
	return fd;
}



#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
inline std::optional<HANDLE> get_HANDLE(stdio_fd e) {
	std::optional<HANDLE> handle;
	switch (e) {
		case stdio_fd::in:
			handle = GetStdHandle(STD_INPUT_HANDLE);
			break;
		case stdio_fd::out:
			handle = GetStdHandle(STD_OUTPUT_HANDLE);
			break;
		case stdio_fd::err:
			handle = GetStdHandle(STD_ERROR_HANDLE);
			break;
	}
	if (handle.has_value() && ((handle.value() == NULL) || (handle.value() == INVALID_HANDLE_VALUE))) {
		handle = std::nullopt;
	}
	return handle;
}
#endif



} // namespace detail



} // namespace terminal



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TERMINAL_BASE_HPP
