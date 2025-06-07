/*
 * mptLibrary.cpp
 * --------------
 * Purpose: Shared library handling.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptLibrary.h"

#include "mpt/fs/common_directories.hpp"
#include "mpt/library/library.hpp"

#include <optional>



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



class LibraryHandle
{

private:

	std::optional<mpt::library> lib;

private:

	static mpt::library::path convert_path(const mpt::LibraryPath &path)
	{
		mpt::library::path result{};
		result.prefix = mpt::library::path_prefix::none;
		result.suffix = mpt::library::path_suffix::none;
		switch(path.GetSearchPath())
		{
		case LibrarySearchPath::Default:
			result.search = mpt::library::path_search::default_;
			result.filename = path.GetFileName();
			break;
		case LibrarySearchPath::System:
			result.search = mpt::library::path_search::system;
			result.filename = path.GetFileName();
			break;
// Using restricted search paths applies to potential DLL dependencies
// recursively.
// This fails loading for e.g. Codec or Plugin DLLs in application
// directory if they depend on the MSVC C or C++ runtime (which is
// located in the system directory).
// Just rely on the default search path here.
#if MPT_OS_WINDOWS && defined(MODPLUG_TRACKER)
		case LibrarySearchPath::Application:
			if (!mpt::common_directories::get_application_directory().empty()) {
				result.search = mpt::library::path_search::unsafe;
				result.filename = mpt::common_directories::get_application_directory().WithTrailingSlash() + path.GetFileName();
			} else {
				result.search = mpt::library::path_search::invalid;
				result.filename = path.GetFileName();
			}
			break;
		case LibrarySearchPath::FullPath:
			result.search = mpt::library::path_search::unsafe;
			result.filename = path.GetFileName();
			break;
#else
		case LibrarySearchPath::Application:
			result.search = mpt::library::path_search::application;
			result.filename = path.GetFileName();
			break;
		case LibrarySearchPath::FullPath:
			result.search = mpt::library::path_search::none;
			result.filename = path.GetFileName();
			break;
#endif
		default:
			result.search = mpt::library::path_search::invalid;
			result.filename = path.GetFileName();
			break;
		}
		return result;
	}

public:

	LibraryHandle(const mpt::LibraryPath &path)
		: lib(mpt::library::load_optional(convert_path(path)))
	{
		return;
	}

	LibraryHandle(const LibraryHandle &) = delete;

	LibraryHandle & operator=(const LibraryHandle &) = delete;

	~LibraryHandle()
	{
		return;
	}

public:

	bool IsValid() const
	{
		return lib.has_value();
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		if(!IsValid())
		{
			return nullptr;
		}
		FuncPtr result = nullptr;
		lib->bind_function(result, symbol);
		return result;
	}

};



LibraryPath::LibraryPath(mpt::LibrarySearchPath searchPath, const mpt::PathString &fileName)
	: searchPath(searchPath)
	, fileName(fileName)
{
	return;
}


mpt::LibrarySearchPath LibraryPath::GetSearchPath() const
{
	return searchPath;
}


mpt::PathString LibraryPath::GetFileName() const
{
	return fileName;
}


mpt::PathString LibraryPath::GetDefaultPrefix()
{
	#if MPT_OS_WINDOWS
		return P_("");
	#elif MPT_OS_ANDROID
		return P_("lib");
	#elif defined(MPT_WITH_LTDL)
		return P_("lib");
	#elif defined(MPT_WITH_DL)
		return P_("lib");
	#else
		return P_("lib");
	#endif
}


mpt::PathString LibraryPath::GetDefaultSuffix()
{
	#if MPT_OS_WINDOWS
		return P_(".dll");
	#elif MPT_OS_ANDROID
		return P_(".so");
	#elif defined(MPT_WITH_LTDL)
		return P_("");  // handled by libltdl
	#elif defined(MPT_WITH_DL)
		return P_(".so");
	#else
		return mpt::PathString();
	#endif
}


LibraryPath LibraryPath::App(const mpt::PathString &basename)
{
	return LibraryPath(mpt::LibrarySearchPath::Application, GetDefaultPrefix() + basename + GetDefaultSuffix());
}


LibraryPath LibraryPath::AppFullName(const mpt::PathString &fullname)
{
	return LibraryPath(mpt::LibrarySearchPath::Application, fullname + GetDefaultSuffix());
}


LibraryPath LibraryPath::System(const mpt::PathString &basename)
{
	return LibraryPath(mpt::LibrarySearchPath::System, GetDefaultPrefix() + basename + GetDefaultSuffix());
}


LibraryPath LibraryPath::FullPath(const mpt::PathString &path)
{
	return LibraryPath(mpt::LibrarySearchPath::FullPath, path);
}



Library::Library()
{
	return;
}


Library::Library(const mpt::LibraryPath &path)
{
	if(path.GetSearchPath() == mpt::LibrarySearchPath::Invalid)
	{
		return;
	}
	if(path.GetFileName().empty())
	{
		return;
	}
	m_Handle = std::make_shared<LibraryHandle>(path);
}


void Library::Unload()
{
	*this = mpt::Library();
}


bool Library::IsValid() const
{
	return m_Handle && m_Handle->IsValid();
}


FuncPtr Library::GetProcAddress(const std::string &symbol) const
{
	if(!IsValid())
	{
		return nullptr;
	}
	return m_Handle->GetProcAddress(symbol);
}



} // namespace mpt



OPENMPT_NAMESPACE_END
