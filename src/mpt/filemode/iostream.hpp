/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_IOSTREAM_HPP
#define MPT_FILEMODE_IOSTREAM_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/filemode.hpp"
#include "mpt/filemode/stdfile.hpp"
#include "mpt/exception/runtime_error.hpp"
#include "mpt/string/types.hpp"

#include <iostream>
#include <istream>
#include <optional>
#include <ostream>

#include <cstdio>
#include <cstddef>

#include <stdio.h>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



class istream_guard {
private:
	std::optional<mpt::filemode::FILE_guard> guard;
private:
	static std::FILE * get_FILE(std::istream & stream) {
		std::FILE * result = NULL;
		if (&stream == &std::cin) {
			result = stdin;
		} else {
			throw mpt::runtime_error(MPT_USTRING("invalid iostream"));
		}
		return result;
	}
public:
	istream_guard(std::istream & stream, mpt::filemode::mode mode) {
		guard = std::make_optional<mpt::filemode::FILE_guard>(get_FILE(stream), mode);
	}
	istream_guard(const istream_guard &) = delete;
	istream_guard(istream_guard &&) = default;
	istream_guard & operator=(const istream_guard &) = delete;
	istream_guard & operator=(istream_guard &&) = default;
	~istream_guard() {
		guard = std::nullopt;
	}
};



class ostream_guard {
private:
	std::ostream * stream;
	std::optional<mpt::filemode::FILE_guard> guard;
private:
	static std::FILE * get_FILE(std::ostream & stream) {
		std::FILE * result = NULL;
		if (&stream == &std::cout) {
			result = stdout;
		} else if (&stream == &std::cerr) {
			result = stderr;
		} else if (&stream == &std::clog) {
			result = stderr;
		} else {
			throw mpt::runtime_error(MPT_USTRING("invalid iostream"));
		}
		return result;
	}
public:
	ostream_guard(std::ostream & stream, mpt::filemode::mode mode)
		: stream(&stream) {
		guard = std::make_optional<mpt::filemode::FILE_guard>(get_FILE(stream), mode);
	}
	ostream_guard(const ostream_guard &) = delete;
	ostream_guard(ostream_guard &&) = default;
	ostream_guard & operator=(const ostream_guard &) = delete;
	ostream_guard & operator=(ostream_guard &&) = default;
	~ostream_guard() {
		stream->flush();
		guard = std::nullopt;
	}
};



#if !defined(MPT_COMPILER_QUIRK_NO_WCHAR)



class wistream_guard {
private:
	std::optional<mpt::filemode::FILE_guard> guard;
private:
	static std::FILE * get_FILE(std::wistream & stream) {
		std::FILE * result = NULL;
		if (&stream == &std::wcin) {
			result = stdin;
		} else {
			throw mpt::runtime_error(MPT_USTRING("invalid iostream"));
		}
		return result;
	}
public:
	wistream_guard(std::wistream & stream, mpt::filemode::mode mode) {
		guard = std::make_optional<mpt::filemode::FILE_guard>(get_FILE(stream), mode);
	}
	wistream_guard(const wistream_guard &) = delete;
	wistream_guard(wistream_guard &&) = default;
	wistream_guard & operator=(const wistream_guard &) = delete;
	wistream_guard & operator=(wistream_guard &&) = default;
	~wistream_guard() {
		guard = std::nullopt;
	}
};



class wostream_guard {
private:
	std::wostream * stream;
	std::optional<mpt::filemode::FILE_guard> guard;
private:
	static std::FILE * get_FILE(std::wostream & stream) {
		std::FILE * result = NULL;
		if (&stream == &std::wcout) {
			result = stdout;
		} else if (&stream == &std::wcerr) {
			result = stderr;
		} else if (&stream == &std::wclog) {
			result = stderr;
		} else {
			throw mpt::runtime_error(MPT_USTRING("invalid iostream"));
		}
		return result;
	}
public:
	wostream_guard(std::wostream & stream, mpt::filemode::mode mode)
		: stream(&stream) {
		guard = std::make_optional<mpt::filemode::FILE_guard>(get_FILE(stream), mode);
	}
	wostream_guard(const wostream_guard &) = delete;
	wostream_guard(wostream_guard &&) = default;
	wostream_guard & operator=(const wostream_guard &) = delete;
	wostream_guard & operator=(wostream_guard &&) = default;
	~wostream_guard() {
		stream->flush();
		guard = std::nullopt;
	}
};



#endif // !MPT_COMPILER_QUIRK_NO_WCHAR



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_IOSTREAM_HPP
