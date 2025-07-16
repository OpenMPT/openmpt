/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TERMINAL_SIZE_HPP
#define MPT_TERMINAL_SIZE_HPP

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#if !(MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10))
#include "mpt/parse/parse.hpp"
#endif

#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
#include <algorithm>
#endif

#if !(MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10))
#include <cstdlib>
#endif

#if MPT_OS_DJGPP
#include <pc.h>
#endif
#if !MPT_OS_WINDOWS
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
#include <windows.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace terminal {



inline void query_size(int & terminal_width, int & terminal_height) {
#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
	HANDLE hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if ((hStdOutput != NULL) && (hStdOutput != INVALID_HANDLE_VALUE)) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		ZeroMemory(&csbi, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
		if (GetConsoleScreenBufferInfo(hStdOutput, &csbi) != FALSE) {
			if (terminal_width <= 0) {
				terminal_width = std::min(static_cast<int>(1 + csbi.srWindow.Right - csbi.srWindow.Left), static_cast<int>(csbi.dwSize.X));
			}
			if (terminal_height <= 0) {
				terminal_height = std::min(static_cast<int>(1 + csbi.srWindow.Bottom - csbi.srWindow.Top), static_cast<int>(csbi.dwSize.Y));
			}
		}
	}
#else // !(MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10))
	if (isatty(STDERR_FILENO)) {
		if (terminal_width <= 0) {
			const char * env_columns = std::getenv("COLUMNS");
			if (env_columns) {
				int tmp = mpt::parse_or<int>(env_columns, 0);
				if (tmp > 0) {
					terminal_width = tmp;
				}
			}
		}
		if (terminal_height <= 0) {
			const char * env_rows = std::getenv("ROWS");
			if (env_rows) {
				int tmp = mpt::parse_or<int>(env_rows, 0);
				if (tmp > 0) {
					terminal_height = tmp;
				}
			}
		}
#if defined(TIOCGWINSZ)
		struct winsize ts;
		if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ts) >= 0) {
			if (terminal_width <= 0) {
				terminal_width = ts.ws_col;
			}
			if (terminal_height <= 0) {
				terminal_height = ts.ws_row;
			}
		}
#elif defined(TIOCGSIZE)
		struct ttysize ts;
		if (ioctl(STDERR_FILENO, TIOCGSIZE, &ts) >= 0) {
			if (terminal_width <= 0) {
				terminal_width = ts.ts_cols;
			}
			if (terminal_height <= 0) {
				terminal_height = ts.ts_rows;
			}
		}
#endif
	}
#endif // MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
#if MPT_OS_DJGPP
	if (terminal_width <= 0) {
		terminal_width = ScreenCols();
	}
	if (terminal_height <= 0) {
		terminal_height = ScreenRows();
	}
#endif
	if (terminal_width <= 0) {
		terminal_width = 72;
	}
	if (terminal_height <= 0) {
		terminal_height = 23;
	}
}



} // namespace terminal



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TERMINAL_SIZE_HPP
