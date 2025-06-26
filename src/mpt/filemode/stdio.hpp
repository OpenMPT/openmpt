/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_STDIO_HPP
#define MPT_FILEMODE_STDIO_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/filemode.hpp"
#include "mpt/filemode/iostream.hpp"
#include "mpt/exception/runtime_error.hpp"
#include "mpt/string/types.hpp"

#include <iostream>
#include <variant>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



class stdio_guard {
private:
#if MPT_OS_WINDOWS && defined(UNICODE)
	std::variant<std::monostate, mpt::filemode::istream_guard, mpt::filemode::ostream_guard, mpt::filemode::wistream_guard, mpt::filemode::wostream_guard> guard;
#else
	std::variant<std::monostate, mpt::filemode::istream_guard, mpt::filemode::ostream_guard> guard;
#endif
public:
	stdio_guard(stdio which, mpt::filemode::mode mode) {
#if MPT_OS_WINDOWS && defined(UNICODE)
		switch (which) {
			case mpt::filemode::stdio::input:
				if (mode == mpt::filemode::mode::text) {
					guard = mpt::filemode::wistream_guard(std::wcin, mode);
				} else {
					guard = mpt::filemode::istream_guard(std::cin, mode);
				}
				break;
			case mpt::filemode::stdio::output:
				if (mode == mpt::filemode::mode::text) {
					guard = mpt::filemode::wostream_guard(std::wcout, mode);
				} else {
					guard = mpt::filemode::ostream_guard(std::cout, mode);
				}
				break;
			case mpt::filemode::stdio::error:
				if (mode == mpt::filemode::mode::text) {
					guard = mpt::filemode::wostream_guard(std::wcerr, mode);
				} else {
					guard = mpt::filemode::ostream_guard(std::cerr, mode);
				}
				break;
			case mpt::filemode::stdio::log:
				if (mode == mpt::filemode::mode::text) {
					guard = mpt::filemode::wostream_guard(std::wclog, mode);
				} else {
					guard = mpt::filemode::ostream_guard(std::clog, mode);
				}
				break;
			default:
				throw mpt::runtime_error(MPT_USTRING("invalid stdio"));
				break;
#else
		switch (which) {
			case mpt::filemode::stdio::input:
				guard = mpt::filemode::istream_guard(std::cin, mode);
				break;
			case mpt::filemode::stdio::output:
				guard = mpt::filemode::ostream_guard(std::cout, mode);
				break;
			case mpt::filemode::stdio::error:
				guard = mpt::filemode::ostream_guard(std::cerr, mode);
				break;
			case mpt::filemode::stdio::log:
				guard = mpt::filemode::ostream_guard(std::clog, mode);
				break;
			default:
				throw mpt::runtime_error(MPT_USTRING("invalid stdio"));
				break;
#endif
		}
	}
	stdio_guard(const stdio_guard &) = delete;
	stdio_guard(stdio_guard &&) = default;
	stdio_guard & operator=(const stdio_guard &) = delete;
	stdio_guard & operator=(stdio_guard &&) = default;
	~stdio_guard() {
		guard = std::monostate{};
	}
};



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_STDIO_HPP
