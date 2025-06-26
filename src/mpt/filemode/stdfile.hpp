/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FILEMODE_STDFILE_HPP
#define MPT_FILEMODE_STDFILE_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/filemode/fd.hpp"
#include "mpt/filemode/filemode.hpp"

#include <optional>

#include <cstdio>

#if MPT_OS_DJGPP
#include <stdio.h>
#elif MPT_OS_WINDOWS && MPT_LIBC_MS
#include <stdio.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace filemode {



#if MPT_OS_DJGPP

class FILE_guard {
private:
	std::FILE * file;
	std::optional<mpt::filemode::fd_guard> guard;
public:
	FILE_guard( std::FILE * file, mpt::filemode::mode new_mode )
		: file(file)
	{
		std::fflush( file );
		guard = std::make_optional<mpt::filemode::fd_guard>( fileno( file ), new_mode );
	}
	FILE_guard( const FILE_guard & ) = delete;
	FILE_guard( FILE_guard && ) = default;
	FILE_guard & operator=( const FILE_guard & ) = delete;
	FILE_guard & operator=( FILE_guard && ) = default;
	~FILE_guard() {
		std::fflush( file );
		guard = std::nullopt;
	}
};

#elif MPT_OS_WINDOWS && MPT_LIBC_MS

class FILE_guard {
private:
	std::FILE * file;
	std::optional<mpt::filemode::fd_guard> guard;
public:
	FILE_guard( std::FILE * file, mpt::filemode::mode new_mode )
		: file(file)
	{
		std::fflush( file );
		guard = std::make_optional<mpt::filemode::fd_guard>( _fileno( file ), new_mode );
	}
	FILE_guard( const FILE_guard & ) = delete;
	FILE_guard( FILE_guard && ) = default;
	FILE_guard & operator=( const FILE_guard & ) = delete;
	FILE_guard & operator=( FILE_guard && ) = default;
	~FILE_guard() {
		std::fflush( file );
		guard = std::nullopt;
	}
};

#else

class FILE_guard {
public:
	FILE_guard( std::FILE * /* file */, mpt::filemode::mode /* new_mode */ ) {
		return;
	}
	FILE_guard( const FILE_guard & ) = delete;
	FILE_guard( FILE_guard && ) = default;
	FILE_guard & operator=( const FILE_guard & ) = delete;
	FILE_guard & operator=( FILE_guard && ) = default;
	~FILE_guard() = default;
};

#endif



} // namespace filemode



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FILEMODE_STDFILE_HPP
