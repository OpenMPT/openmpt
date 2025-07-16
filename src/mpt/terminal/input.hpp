/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TERMINAL_INPUT_HPP
#define MPT_TERMINAL_INPUT_HPP

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"

#include <optional>

#if MPT_OS_DJGPP
#include <conio.h>
#include <termios.h>
#include <unistd.h>
#elif MPT_OS_WINDOWS
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/poll.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace terminal {



#if MPT_OS_WINDOWS

class input_guard {
public:
	input_guard() = default;
	input_guard(const input_guard &) = delete;
	input_guard(input_guard &&) = delete;
	input_guard & operator=(const input_guard &) = delete;
	input_guard & operator=(input_guard &&) = delete;
	~input_guard() = default;
};

#else

class input_guard {
private:
	bool changed = false;
	termios saved_attributes;
public:
	input_guard() {
		if (!isatty(STDIN_FILENO)) {
			return;
		}
		tcgetattr(STDIN_FILENO, &saved_attributes);
		termios tattr = saved_attributes;
		tattr.c_lflag &= ~(ICANON | ECHO);
		tattr.c_cc[VMIN] = 1;
		tattr.c_cc[VTIME] = 0;
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
		changed = true;
	}
	input_guard(const input_guard &) = delete;
	input_guard(input_guard &&) = delete;
	input_guard & operator=(const input_guard &) = delete;
	input_guard & operator=(input_guard &&) = delete;
	~input_guard() {
		if (changed) {
			tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
		}
	}
};

#endif



class input {
public:
	static inline bool is_input_available() {
#if MPT_OS_DJGPP
		return kbhit() ? true : false;
#elif MPT_OS_WINDOWS && defined(UNICODE) && !MPT_OS_WINDOWS_WINRT
		return _kbhit() ? true : false;
#elif MPT_OS_WINDOWS
		return _kbhit() ? true : false;
#else
		pollfd pollfds;
		pollfds.fd = STDIN_FILENO;
		pollfds.events = POLLIN;
		poll(&pollfds, 1, 0);
		if (!(pollfds.revents & POLLIN)) {
			return false;
		}
		return true;
#endif
	}
	static inline std::optional<int> read_input_char() {
#if MPT_OS_DJGPP
		int c = getch();
		return static_cast<int>(c);
#elif MPT_OS_WINDOWS && defined(UNICODE) && !MPT_OS_WINDOWS_WINRT
		wint_t c = _getwch();
		return static_cast<int>(c);
#elif MPT_OS_WINDOWS
		int c = _getch();
		return static_cast<int>(c);
#else
		char c = 0;
		if (read(STDIN_FILENO, &c, 1) != 1) {
			return std::nullopt;
		}
		return static_cast<int>(c);
#endif
	}
};



} // namespace terminal



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TERMINAL_INPUT_HPP
