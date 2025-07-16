/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TERMINAL_OUTPUT_HPP
#define MPT_TERMINAL_OUTPUT_HPP

#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/format/simple.hpp"
#include "mpt/string/types.hpp"
#include "mpt/string_transcode/transcode.hpp"
#include "mpt/terminal/transliterate.hpp"

#include <iostream>

#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
#include <windows.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace terminal {



template <typename Tstring>
struct concat_stream {
	virtual concat_stream & append(Tstring str) = 0;
	virtual ~concat_stream() = default;
	inline concat_stream<Tstring> & operator<<(concat_stream<Tstring> & (*func)(concat_stream<Tstring> & s)) {
		return func(*this);
	}
};

template <typename T, typename Tstring>
inline concat_stream<Tstring> & operator<<(concat_stream<Tstring> & s, const T & val) {
	return s.append(mpt::default_formatter::template format<Tstring>(val));
}

template <typename Tstring>
struct string_concat_stream
	: public concat_stream<Tstring> {
private:
	Tstring m_str;
public:
	inline void str(Tstring s) {
		m_str = std::move(s);
	}
	inline concat_stream<Tstring> & append(Tstring s) override {
		m_str += std::move(s);
		return *this;
	}
	inline Tstring str() const {
		return m_str;
	}
	~string_concat_stream() override = default;
};



class textout : public string_concat_stream<mpt::ustring> {
protected:
	textout() = default;
public:
	virtual ~textout() = default;
protected:
	mpt::ustring pop() {
		mpt::ustring text = str();
		str(mpt::ustring());
		return text;
	}
public:
	virtual void writeout() = 0;
	virtual void cursor_up(std::size_t lines) = 0;
};



class textout_dummy : public textout {
public:
	textout_dummy() = default;
	~textout_dummy() override {
		static_cast<void>(pop());
	}
public:
	void writeout() override {
		static_cast<void>(pop());
	}
	void cursor_up(std::size_t lines) override {
		static_cast<void>(lines);
	}
};



enum class textout_destination {
	destination_stdout,
	destination_stderr,
	destination_stdlog,
};



class textout_backend {
protected:
	textout_backend() = default;
public:
	virtual ~textout_backend() = default;
public:
	virtual void write(const mpt::ustring & text) = 0;
	virtual void cursor_up(std::size_t lines) = 0;
};



class textout_ostream : public textout_backend {
private:
	std::ostream & s;
	void write_raw(const std::string & chars) {
		if (chars.size() > 0) {
			s.write(chars.data(), chars.size());
		}
	}
#if MPT_OS_DJGPP
	mpt::common_encoding codepage;
#endif
public:
	textout_ostream(std::ostream & s_)
		: s(s_)
#if MPT_OS_DJGPP
		, codepage(mpt::common_encoding::cp437)
#endif
	{
#if MPT_OS_DJGPP
		codepage = mpt::djgpp_get_locale_encoding();
#endif
		return;
	}
	~textout_ostream() override = default;
public:
	void write(const mpt::ustring & text) override {
		if (text.length() > 0) {
#if MPT_OS_DJGPP
			write_raw(mpt::transcode<std::string>(codepage, transliterate_control_codes_multiline(text, {transliterate_c0::ascii, transliterate_del::ascii, transliterate_c1::ascii_replacement})));
#elif MPT_OS_EMSCRIPTEN
			write_raw(mpt::transcode<std::string>(mpt::common_encoding::utf8, transliterate_control_codes_multiline(text, {transliterate_c0::unicode, transliterate_del::unicode, transliterate_c1::unicode_replacement})));
#else
			write_raw(mpt::transcode<std::string>(mpt::logical_encoding::locale, transliterate_control_codes_multiline(text, {transliterate_c0::unicode, transliterate_del::unicode, transliterate_c1::unicode_replacement})));
#endif
			s.flush();
		}
	}
	void cursor_up(std::size_t lines) override {
		s.flush();
		for (std::size_t line = 0; line < lines; ++line) {
			write_raw(std::string("\x1b[1A"));
		}
	}
};

#if MPT_OS_WINDOWS && defined(UNICODE)

class textout_wostream : public textout_backend {
private:
	std::wostream & s;
	void write_raw(const std::wstring & wchars) {
		if (wchars.size() > 0) {
			s.write(wchars.data(), wchars.size());
		}
	}
public:
	textout_wostream(std::wostream & s_)
		: s(s_) {
		return;
	}
	~textout_wostream() override = default;
public:
	void write(const mpt::ustring & text) override {
		if (text.length() > 0) {
			write_raw(mpt::transcode<std::wstring>(transliterate_control_codes_multiline(text, {transliterate_c0::unicode, transliterate_del::unicode, transliterate_c1::unicode_replacement})));
			s.flush();
		}
	}
	void cursor_up(std::size_t lines) override {
		s.flush();
		for (std::size_t line = 0; line < lines; ++line) {
			write_raw(std::wstring(L"\x1b[1A"));
		}
	}
};

#endif // MPT_OS_WINDOWS && UNICODE

#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)

class textout_ostream_console : public textout_backend {
private:
#if defined(UNICODE)
	std::wostream & s;
#else
	std::ostream & s;
#endif
	void write_raw(const mpt::winstring & wchars) {
		if (wchars.size() > 0) {
			s.write(wchars.data(), wchars.size());
		}
	}
	HANDLE handle;
	bool console;
#if MPT_WIN_AT_LEAST(MPT_WIN_10_1809)
	DWORD mode;
#endif
public:
#if defined(UNICODE)
	textout_ostream_console(std::wostream & s_, stdio_fd e)
#else
	textout_ostream_console(std::ostream & s_, stdio_fd e)
#endif
		: s(s_)
		, handle(detail::get_HANDLE(e).value_or(static_cast<HANDLE>(NULL)))
		, console(is_terminal(e)) {
		s.flush();
#if MPT_WIN_AT_LEAST(MPT_WIN_10_1809)
		if (GetConsoleMode(handle, &mode) == FALSE) {
			mode = 0;
		}
#endif
		return;
	}
	~textout_ostream_console() override = default;
public:
	void write(const mpt::ustring & text) override {
		if (text.length() > 0) {
			if (console) {
				DWORD chars_written = 0;
				mpt::winstring wtext = mpt::transcode<mpt::winstring>(transliterate_control_codes_multiline(text, {transliterate_c0::unicode, transliterate_del::unicode, transliterate_c1::unicode_replacement}));
				WriteConsole(handle, wtext.data(), static_cast<DWORD>(wtext.size()), &chars_written, NULL);
			} else {
				write_raw(mpt::transcode<mpt::winstring>(transliterate_control_codes_multiline(text, {transliterate_c0::unicode, transliterate_del::unicode, transliterate_c1::unicode_replacement})));
				s.flush();
			}
		}
	}
	void cursor_up(std::size_t lines) override {
		if (console) {
#if MPT_WIN_AT_LEAST(MPT_WIN_10_1809)
			if ((mode & (ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) == (ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
				for (std::size_t line = 0; line < lines; ++line) {
					DWORD chars_written = 0;
					mpt::winstring wtext = mpt::winstring(TEXT("\x1b[A"));
					WriteConsole(handle, wtext.data(), static_cast<DWORD>(wtext.size()), &chars_written, NULL);
				}
				return;
			}
#endif
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			ZeroMemory(&csbi, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
			COORD coord_cursor = COORD();
			if (GetConsoleScreenBufferInfo(handle, &csbi) != FALSE) {
				coord_cursor = csbi.dwCursorPosition;
				coord_cursor.X = 1;
				coord_cursor.Y -= static_cast<SHORT>(lines);
				SetConsoleCursorPosition(handle, coord_cursor);
			}
		} else {
			s.flush();
			for (std::size_t line = 0; line < lines; ++line) {
				write_raw(mpt::winstring(TEXT("\x1b[1A")));
			}
		}
	}
};

#endif // MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)



template <textout_destination dest>
class textout_wrapper : public textout {
private:
#if MPT_OS_WINDOWS && !MPT_WINRT_BEFORE(MPT_WIN_10)
#if defined(UNICODE)
	textout_ostream_console out{(dest == textout_destination::destination_stdout) ? std::wcout : (dest == textout_destination::destination_stderr) ? std::wcerr
																																				   : std::wclog,
								(dest == textout_destination::destination_stdout) ? stdio_fd::out : stdio_fd::err};
#else
	textout_ostream_console out{(dest == textout_destination::destination_stdout) ? std::cout : (dest == textout_destination::destination_stderr) ? std::cerr
																																				  : std::clog,
								(dest == textout_destination::destination_stdout) ? stdio_fd::out : stdio_fd::err};
#endif
#elif MPT_OS_WINDOWS
#if defined(UNICODE)
	textout_wostream out{(dest == textout_destination::destination_stdout) ? std::wcout : (dest == textout_destination::destination_stderr) ? std::wcerr
																																			: std::wclog};
#else
	textout_ostream out{(dest == textout_destination::destination_stdout) ? std::cout : (dest == textout_destination::destination_stderr) ? std::cerr
																																		  : std::clog};
#endif
#else
	textout_ostream out{(dest == textout_destination::destination_stdout) ? std::cout : (dest == textout_destination::destination_stderr) ? std::cerr
																																		  : std::clog};
#endif
public:
	textout_wrapper() = default;
	~textout_wrapper() override {
		out.write(pop());
	}
public:
	void writeout() override {
		out.write(pop());
	}
	void cursor_up(std::size_t lines) override {
		out.cursor_up(lines);
	}
};



} // namespace terminal



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TERMINAL_OUTPUT_HPP
