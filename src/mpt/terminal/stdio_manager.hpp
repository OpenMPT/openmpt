/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_TERMINAL_STDIO_MANAGER_HPP
#define MPT_TERMINAL_STDIO_MANAGER_HPP

#include "mpt/base/detect.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/filemode.hpp"
#include "mpt/filemode/stdio.hpp"
#include "mpt/terminal/input.hpp"
#include "mpt/terminal/output.hpp"

#include <iostream>
#include <memory>
#include <optional>

#include <cassert>
#include <cstdio>

#include <stdio.h>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace terminal {



class stdio_manager final {
public:
	enum class api {
		unused,
		os,
		crt,
		posix,
		stdfile,
		stdiostream,
	};
	enum class stdin_mode {
		unused,
		text,
		binary,
		terminal,
	};
	enum class stdout_mode {
		unused,
		text,
		binary,
	};
	enum class stderr_mode {
		unused,
		text,
	};
private:
	api in_api;
	api out_api;
	api err_api;
	stdin_mode in_mode;
	stdout_mode out_mode;
	stderr_mode err_mode;
	std::optional<mpt::filemode::stdio_guard<mpt::filemode::stdio::input>> stdin_guard;
	std::optional<mpt::filemode::stdio_guard<mpt::filemode::stdio::output>> stdout_guard;
	std::optional<mpt::filemode::stdio_guard<mpt::filemode::stdio::error>> stderr_guard;
	std::optional<mpt::filemode::stdio_guard<mpt::filemode::stdio::log>> stdlog_guard;
	std::optional<input_guard> stdin_terminal_guard;
	std::optional<std::unique_ptr<textout_wrapper<textout_destination::destination_stdout>>> std_out;
	std::optional<std::unique_ptr<textout_wrapper<textout_destination::destination_stderr>>> std_err;
	std::optional<std::unique_ptr<textout_wrapper<textout_destination::destination_stdlog>>> std_log;
	textout_dummy std_nul;
	constexpr static mpt::filemode::api get_filemode_api(api api_) {
		mpt::filemode::api result = mpt::filemode::api::none;
		switch (api_) {
			case api::stdiostream:
				result = mpt::filemode::api::iostream;
				break;
			case api::stdfile:
				result = mpt::filemode::api::file;
				break;
			case api::posix:
				result = mpt::filemode::api::fd;
				break;
			case api::crt:
				result = mpt::filemode::api::fd;
				break;
			case api::os:
				result = mpt::filemode::api::none;
				break;
			case api::unused:
				result = mpt::filemode::api::none;
				break;
		}
		return result;
	}
public:
	stdio_manager(mpt::main::stdin_token token_in, api in_api_, stdin_mode in_mode_, mpt::main::stdout_token token_out, api out_api_, stdout_mode out_mode_, mpt::main::stderr_token token_err, api err_api_, stderr_mode err_mode_)
		: in_api(in_api_)
		, out_api(out_api_)
		, err_api(err_api_)
		, in_mode(in_mode_)
		, out_mode(out_mode_)
		, err_mode(err_mode_) {
		MPT_UNUSED(token_in);
		MPT_UNUSED(token_out);
		MPT_UNUSED(token_err);
		switch (in_mode) {
			case stdin_mode::unused:
				// nothing
				break;
			case stdin_mode::text:
				stdin_guard.emplace(get_filemode_api(in_api), mpt::filemode::mode::text);
				break;
			case stdin_mode::binary:
				stdin_guard.emplace(get_filemode_api(in_api), mpt::filemode::mode::binary);
				break;
			case stdin_mode::terminal:
				stdin_guard.emplace(get_filemode_api(in_api), mpt::filemode::mode::text);
				break;
		}
		switch (out_mode) {
			case stdout_mode::unused:
				// nothing
				break;
			case stdout_mode::text:
				stdout_guard.emplace(get_filemode_api(out_api), mpt::filemode::mode::text);
				break;
			case stdout_mode::binary:
				stdout_guard.emplace(get_filemode_api(out_api), mpt::filemode::mode::binary);
				break;
		}
		switch (err_mode) {
			case stderr_mode::unused:
				// nothing
				break;
			case stderr_mode::text:
				stderr_guard.emplace(get_filemode_api(err_api), mpt::filemode::mode::text);
				stdlog_guard.emplace(get_filemode_api(err_api), mpt::filemode::mode::text);
				break;
		}
		if (in_mode == stdin_mode::terminal) {
			stdin_terminal_guard.emplace();
		}
		if (out_mode == stdout_mode::text) {
			std_out = std::make_unique<textout_wrapper<textout_destination::destination_stdout>>();
		}
		if (err_mode == stderr_mode::text) {
			std_err = std::make_unique<textout_wrapper<textout_destination::destination_stderr>>();
		}
		// untie stdin and stdout
		if ((in_mode == stdin_mode::binary) || (in_mode == stdin_mode::unused) || (out_mode == stdout_mode::binary) || (out_mode == stdout_mode::unused)) {
			if (in_api == api::stdiostream) {
				std::cin.tie(nullptr);
#if MPT_OS_WINDOWS && defined(UNICODE)
				std::wcin.tie(nullptr);
#endif
			}
		}
		// untie stderr and stdout
		if ((out_mode == stdout_mode::binary) || (out_mode == stdout_mode::unused)) {
			if (err_api == api::stdiostream) {
				std::cerr.tie(nullptr);
#if MPT_OS_WINDOWS && defined(UNICODE)
				std::wcerr.tie(nullptr);
#endif
			}
		}
	}
#if MPT_OS_WINDOWS && defined(UNICODE)
	std::wistream & stdin_text() {
		assert(in_mode == stdin_mode::text);
		return std::wcin;
	}
#else
	std::istream & stdin_text() {
		assert(in_mode == stdin_mode::text);
		return std::cin;
	}
#endif
	int stdin_fd_data() {
		assert((in_api == api::posix) || (in_api == api::crt));
		assert(in_mode == stdin_mode::binary);
#if MPT_OS_WINDOWS
		return _fileno(stdin);
#else
		return STDIN_FILENO;
#endif
	}
	std::FILE * stdin_file_data() {
		assert(in_api == api::stdfile);
		assert(in_mode == stdin_mode::binary);
		return stdin;
	}
	std::istream & stdin_stream_data() {
		assert(in_api == api::stdiostream);
		assert(in_mode == stdin_mode::binary);
		return std::cin;
	}
	int stdout_fd_data() {
		assert((out_api == api::posix) || (out_api == api::crt));
		assert(out_mode == stdout_mode::binary);
#if MPT_OS_WINDOWS
		return _fileno(stdout);
#else
		return STDOUT_FILENO;
#endif
	}
	std::FILE * stdout_file_data() {
		assert(out_api == api::stdfile);
		assert(out_mode == stdout_mode::binary);
		return stdout;
	}
	std::ostream & stdout_stream_data() {
		assert(out_api == api::stdiostream);
		assert(out_mode == stdout_mode::binary);
		return std::cout;
	}
	textout & output_text() {
		if (out_mode == stdout_mode::text) {
			return *std_out.value().get();
		} else if (out_mode == stdout_mode::binary) {
			if (err_mode == stderr_mode::text) {
				return *std_log.value().get();
			}
		}
		if (err_mode == stderr_mode::text) {
			return *std_err.value().get();
		}
		return std_nul;
	}
	textout & error_text() {
		if (err_mode == stderr_mode::text) {
			if (out_mode == stdout_mode::binary) {
				return *std_log.value().get();
			} else {
				return *std_err.value().get();
			}
		} else if (out_mode == stdout_mode::text) {
			return *std_out.value().get();
		}
		return std_nul;
	}
	textout & silent_text() {
		return std_nul;
	}
	~stdio_manager() = default;
};



} // namespace terminal



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_TERMINAL_STDIO_MANAGER_HPP
