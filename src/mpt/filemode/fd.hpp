/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_FD_HPP
#define MPT_FILEMODE_FD_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/exception/runtime_error.hpp"
#include "mpt/filemode/filemode.hpp"
#include "mpt/string/types.hpp"

#if MPT_OS_DJGPP
#include <fcntl.h>
#include <io.h>
#elif MPT_OS_WINDOWS && MPT_LIBC_MS
#include <fcntl.h>
#include <io.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



#if MPT_OS_DJGPP

class fd_guard {
private:
	int fd;
	int old_mode;
public:
	fd_guard(int fd, mpt::filemode::mode new_mode)
		: fd(fd)
		, old_mode(-1) {
		switch (new_mode) {
			case mpt::filemode::mode::text:
				old_mode = setmode(fd, O_TEXT);
				if (old_mode == -1) {
					throw mpt::runtime_error(MPT_USTRING("failed to set TEXT mode on file descriptor"));
				}
				break;
			case mpt::filemode::mode::binary:
				old_mode = setmode(fd, O_BINARY);
				if (old_mode == -1) {
					throw mpt::runtime_error(MPT_USTRING("failed to set BINARY mode on file descriptor"));
				}
				break;
		}
	}
	fd_guard(const fd_guard &) = delete;
	fd_guard(fd_guard &&) = default;
	fd_guard & operator=(const fd_guard &) = delete;
	fd_guard & operator=(fd_guard &&) = default;
	~fd_guard() {
		if (old_mode != -1) {
			old_mode = setmode(fd, old_mode);
		}
	}
};

#elif MPT_OS_WINDOWS && MPT_LIBC_MS

class fd_guard {
private:
	int fd;
	int old_mode;
public:
	fd_guard(int fd, mpt::filemode::mode new_mode)
		: fd(fd)
		, old_mode(-1) {
		switch (new_mode) {
			case mpt::filemode::mode::text:
#if defined(UNICODE) && MPT_LIBC_MS_AT_LEAST(MPT_LIBC_MS_VER_UCRT)
				old_mode = _setmode(fd, _O_U8TEXT);
#else
				old_mode = _setmode(fd, _O_TEXT);
#endif
				if (old_mode == -1) {
					throw mpt::runtime_error(MPT_USTRING("failed to set TEXT mode on file descriptor"));
				}
				break;
			case mpt::filemode::mode::binary:
				old_mode = _setmode(fd, _O_BINARY);
				if (old_mode == -1) {
					throw mpt::runtime_error(MPT_USTRING("failed to set BINARY mode on file descriptor"));
				}
				break;
		}
	}
	fd_guard(const fd_guard &) = delete;
	fd_guard(fd_guard &&) = default;
	fd_guard & operator=(const fd_guard &) = delete;
	fd_guard & operator=(fd_guard &&) = default;
	~fd_guard() {
		if (old_mode != -1) {
			old_mode = _setmode(fd, old_mode);
		}
	}
};

#else

class fd_guard {
public:
	fd_guard(int * /* fd */, mpt::filemode::mode /* new_mode */) {
		return;
	}
	fd_guard(const fd_guard &) = delete;
	fd_guard(fd_guard &&) = default;
	fd_guard & operator=(const fd_guard &) = delete;
	fd_guard & operator=(fd_guard &&) = default;
	~fd_guard() = default;
};

#endif



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_FD_HPP
