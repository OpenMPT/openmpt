/*
 * mptFstream.h
 * ------------
 * Purpose: A wrapper around std::fstream, fixing VS2008 charset conversion braindamage, and enforcing usage of mpt::PathString.
 * Notes  : You should only ever use these wrappers instead of plain std::fstream classes.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <fstream>

#include "../common/mptString.h"
#include "../common/mptPathString.h"

namespace mpt
{

#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)

// VS2008 converts char filenames with CRT mbcs string conversion functions to wchar_t filenames.
// This is totally wrong for Win32 GUI applications because the C locale does not necessarily match the current windows ANSI codepage (CP_ACP).
// Work around this insanity by using our own string conversions for the std::fstream filenames.

#define MPT_FSTREAM_DO_CONVERSIONS

#elif MPT_COMPILER_GCC

#if defined(WIN32)
// GCC C++ library has no wchar_t overloads
#define MPT_FSTREAM_DO_CONVERSIONS
#define MPT_FSTREAM_DO_CONVERSIONS_ANSI
#endif

#endif

#ifdef MPT_FSTREAM_DO_CONVERSIONS
#define MPT_FSTREAM_OPEN(filename, mode) detail::fstream_open<Tbase>(*this, (filename), (mode))
#else
#define MPT_FSTREAM_OPEN(filename, mode) Tbase::open((filename), (mode))
#endif

namespace detail
{

template<typename Tbase>
inline void fstream_open(Tbase & base, const mpt::PathString & filename, std::ios_base::openmode mode)
{
#if defined( MPT_FSTREAM_DO_CONVERSIONS_ANSI)
	base.open(mpt::ToLocale(filename.AsNative()).c_str(), mode);
#else
	base.open(filename.AsNative().c_str(), mode);
#endif
}

#ifdef MPT_FSTREAM_DO_CONVERSIONS

template<typename Tbase>
inline void fstream_open(Tbase & base, const std::wstring & filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(filename), mode);
}

template<typename Tbase>
inline void fstream_open(Tbase & base, const wchar_t * filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(filename ? std::wstring(filename) : std::wstring()), mode);
}

template<typename Tbase>
inline void fstream_open(Tbase & base, const std::string & filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(mpt::ToWide(mpt::CharsetLocale, filename)), mode);
}

template<typename Tbase>
inline void fstream_open(Tbase & base, const char * filename, std::ios_base::openmode mode)
{
	detail::fstream_open<Tbase>(base, mpt::PathString::FromWide(mpt::ToWide(mpt::CharsetLocale, filename ? std::string(filename) : std::string())), mode);
}

#endif

} // namespace detail

class fstream
	: public std::fstream
{
private:
	typedef std::fstream Tbase;
public:
	fstream() {}
	fstream(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	void open(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const char * filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#if defined(WIN32)
	MPT_DEPRECATED_PATH void open(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#endif
};

class ifstream
	: public std::ifstream
{
private:
	typedef std::ifstream Tbase;
public:
	ifstream() {}
	ifstream(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	void open(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const char * filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#if defined(WIN32)
	MPT_DEPRECATED_PATH void open(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#endif
};

class ofstream
	: public std::ofstream
{
private:
	typedef std::ofstream Tbase;
public:
	ofstream() {}
	ofstream(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	void open(const mpt::PathString & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		detail::fstream_open<Tbase>(*this, filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const char * filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#if defined(WIN32)
	MPT_DEPRECATED_PATH void open(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename, mode);
	}
	MPT_DEPRECATED_PATH void open(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		MPT_FSTREAM_OPEN(filename.c_str(), mode);
	}
#endif
};

#undef MPT_FSTREAM_OPEN

} // namespace mpt
