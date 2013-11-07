/*
 * mptFstream.h
 * ------------
 * Purpose: A wrapper around std::fstream, fixing VS2008 charset conversion braindamage.
 * Notes  : You should only ever use these wrappers instead of plain std::fstream classes.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include <fstream>

#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
#include "../common/mptString.h"
#endif

namespace mpt
{

#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)

// VS2008 converts char filenames with CRT mbcs string conversion functions to wchar_t filenames.
// This is totally wrong for Win32 GUI applications because the C locale does not necessarily match the current windows ANSI codepage (CP_ACP).
// Work around this insanity by using our own string conversions for the std::fstream filenames.

class fstream
	: public std::fstream
{
private:
	void mptopen(const std::wstring & filename, std::ios_base::openmode mode)
	{
		std::fstream::open(filename.c_str(), mode);
	}
	void mptopen(const std::string & filename, std::ios_base::openmode mode)
	{
		mptopen(mpt::String::Decode(filename, mpt::CharsetLocale).c_str(), mode);
	}
	void mptopen(const char * filename, std::ios_base::openmode mode)
	{
		mptopen(filename ? std::string(filename) : std::string(), mode);
	}
public:
	fstream() {}
	fstream(const char * filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	fstream(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	fstream(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	fstream(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	void open(const char * filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	{
		mptopen(filename, mode);
	}
};

class ifstream
	: public std::ifstream
{
private:
	void mptopen(const std::wstring & filename, std::ios_base::openmode mode)
	{
		std::ifstream::open(filename.c_str(), mode);
	}
	void mptopen(const std::string & filename, std::ios_base::openmode mode)
	{
		mptopen(mpt::String::Decode(filename, mpt::CharsetLocale).c_str(), mode);
	}
	void mptopen(const char * filename, std::ios_base::openmode mode)
	{
		mptopen(filename ? std::string(filename) : std::string(), mode);
	}
public:
	ifstream() {}
	ifstream(const char * filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		mptopen(filename, mode);
	}
	ifstream(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		mptopen(filename, mode);
	}
	ifstream(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		mptopen(filename, mode);
	}
	ifstream(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		mptopen(filename, mode);
	}
	void open(const char * filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		mptopen(filename, mode);
	}
	void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::in)
	{
		mptopen(filename, mode);
	}
};

class ofstream
	: public std::ofstream
{
private:
	void mptopen(const std::wstring & filename, std::ios_base::openmode mode)
	{
		std::ofstream::open(filename.c_str(), mode);
	}
	void mptopen(const std::string & filename, std::ios_base::openmode mode)
	{
		mptopen(mpt::String::Decode(filename, mpt::CharsetLocale).c_str(), mode);
	}
	void mptopen(const char * filename, std::ios_base::openmode mode)
	{
		mptopen(filename ? std::string(filename) : std::string(), mode);
	}
public:
	ofstream() {}
	ofstream(const char * filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	ofstream(const std::string & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	ofstream(const wchar_t * filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	ofstream(const std::wstring & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	void open(const char * filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		mptopen(filename, mode);
	}
	void open(const std::string & filename, std::ios_base::openmode mode = std::ios_base::out)
	{
		mptopen(filename, mode);
	}
};

#else

typedef std::fstream fstream;
typedef std::ifstream ifstream;
typedef std::ofstream ofstream;

#endif

} // namespace mpt
