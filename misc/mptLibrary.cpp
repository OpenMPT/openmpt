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

#include "mpt/osinfo/windows_version.hpp"

#if MPT_OS_WINDOWS
#include <windows.h>
#elif MPT_OS_ANDROID
#include <dlfcn.h>
#elif defined(MPT_WITH_LTDL)
#include <ltdl.h>
#elif defined(MPT_WITH_DL)
#include <dlfcn.h>
#endif


OPENMPT_NAMESPACE_BEGIN


namespace mpt
{


#if MPT_OS_WINDOWS


// KB2533623 / Win8
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#endif
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif


class LibraryHandle
{

private:

	HMODULE hModule;

public:

	LibraryHandle(const mpt::LibraryPath &path)
		: hModule(NULL)
	{

#if MPT_OS_WINDOWS_WINRT

#if (_WIN32_WINNT < 0x0602)
		(void)path;
		hModule = NULL; // unsupported
#else
		switch(path.GetSearchPath())
		{
			case mpt::LibrarySearchPath::Default:
				hModule = LoadPackagedLibrary(path.GetFileName().AsNative().c_str(), 0);
				break;
			case mpt::LibrarySearchPath::Application:
				hModule = LoadPackagedLibrary(path.GetFileName().AsNative().c_str(), 0);
				break;
			case mpt::LibrarySearchPath::System:
				hModule = NULL; // Only application packaged libraries can be loaded dynamically in WinRT
				break;
			case mpt::LibrarySearchPath::FullPath:
				hModule = NULL; // Absolute path is not supported in WinRT
				break;
			case mpt::LibrarySearchPath::Invalid:
				MPT_ASSERT_NOTREACHED();
				break;
		}
#endif

#else // !MPT_OS_WINDOWS_WINRT

#if (_WIN32_WINNT >= 0x0602)
		bool hasKB2533623 = true;
#else
		// Check for KB2533623:
		bool hasKB2533623 = false;
		mpt::osinfo::windows::Version WindowsVersion = mpt::osinfo::windows::Version::Current();
		if(WindowsVersion.IsAtLeast(mpt::osinfo::windows::Version::Win8))
		{
			hasKB2533623 = true;
		} else if(WindowsVersion.IsAtLeast(mpt::osinfo::windows::Version::WinVista))
		{
			HMODULE hKernel32DLL = LoadLibrary(TEXT("kernel32.dll"));
			if(hKernel32DLL)
			{
				if(::GetProcAddress(hKernel32DLL, "SetDefaultDllDirectories") != nullptr)
				{
					hasKB2533623 = true;
				}
				FreeLibrary(hKernel32DLL);
				hKernel32DLL = NULL;
			}
		}
#endif

		MPT_MAYBE_CONSTANT_IF(hasKB2533623)
		{
			switch(path.GetSearchPath())
			{
				case mpt::LibrarySearchPath::Default:
					hModule = LoadLibraryEx(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
					break;
				case mpt::LibrarySearchPath::System:
					hModule = LoadLibraryEx(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
					break;
#if defined(MODPLUG_TRACKER)
					// Using restricted search paths applies to potential DLL dependencies
					// recursively.
					// This fails loading for e.g. Codec or Plugin DLLs in application
					// directory if they depend on the MSVC C or C++ runtime (which is
					// located in the system directory).
					// Just rely on the default search path here.
				case mpt::LibrarySearchPath::Application:
					{
						const mpt::PathString dllPath = mpt::GetExecutablePath();
						if(!dllPath.empty() && mpt::PathIsAbsolute(dllPath) && dllPath.IsDirectory())
						{
							hModule = LoadLibrary((dllPath + path.GetFileName()).AsNative().c_str());
						}
					}
					break;
				case mpt::LibrarySearchPath::FullPath:
					hModule = LoadLibrary(path.GetFileName().AsNative().c_str());
					break;
#else
					// For libopenmpt, do the safe thing.
				case mpt::LibrarySearchPath::Application:
					hModule = LoadLibraryEx(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
					break;
				case mpt::LibrarySearchPath::FullPath:
					hModule = LoadLibraryEx(path.GetFileName().AsNative().c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
					break;
#endif
				case mpt::LibrarySearchPath::Invalid:
					MPT_ASSERT_NOTREACHED();
					break;
			}
		} else
		{
			switch(path.GetSearchPath())
			{
				case mpt::LibrarySearchPath::Default:
					hModule = LoadLibrary(path.GetFileName().AsNative().c_str());
					break;
				case mpt::LibrarySearchPath::Application:
					{
						const mpt::PathString dllPath = mpt::GetExecutablePath();
						if(!dllPath.empty() && mpt::PathIsAbsolute(dllPath) && dllPath.IsDirectory())
						{
							hModule = LoadLibrary((dllPath + path.GetFileName()).AsNative().c_str());
						}
					}
					break;
				case mpt::LibrarySearchPath::System:
					{
						const mpt::PathString dllPath = mpt::GetSystemPath();
						if(!dllPath.empty() && mpt::PathIsAbsolute(dllPath) && dllPath.IsDirectory())
						{
							hModule = LoadLibrary((dllPath + path.GetFileName()).AsNative().c_str());
						}
					}
					break;
				case mpt::LibrarySearchPath::FullPath:
					hModule = LoadLibrary(path.GetFileName().AsNative().c_str());
					break;
				case mpt::LibrarySearchPath::Invalid:
					MPT_ASSERT_NOTREACHED();
					break;
			}
		}

#endif // MPT_OS_WINDOWS_WINRT

	}

	LibraryHandle(const LibraryHandle &) = delete;

	LibraryHandle & operator=(const LibraryHandle &) = delete;

	~LibraryHandle()
	{
		if(IsValid())
		{
			FreeLibrary(hModule);
		}
		hModule = NULL;
	}

public:

	bool IsValid() const
	{
		return (hModule != NULL);
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		if(!IsValid())
		{
			return nullptr;
		}
		return reinterpret_cast<FuncPtr>(::GetProcAddress(hModule, symbol.c_str()));
	}

};


#elif MPT_OS_ANDROID


// Fake implementation.
// Load shared objects from the JAVA side of things.
class LibraryHandle
{

public:

	LibraryHandle(const mpt::LibraryPath &path)
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
		return true;
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		if(!IsValid())
		{
			return nullptr;
		}
		return reinterpret_cast<FuncPtr>(dlsym(0, symbol.c_str()));
	}

};



#elif defined(MPT_WITH_LTDL)


class LibraryHandle
{

private:

	bool inited;
	lt_dlhandle handle;

public:

	LibraryHandle(const mpt::LibraryPath &path)
		: inited(false)
		, handle(0)
	{
		if(lt_dlinit() != 0)
		{
			return;
		}
		inited = true;
		handle = lt_dlopenext(path.GetFileName().AsNative().c_str());
	}

	LibraryHandle(const LibraryHandle &) = delete;

	LibraryHandle & operator=(const LibraryHandle &) = delete;

	~LibraryHandle()
	{
		if(IsValid())
		{
			lt_dlclose(handle);
		}
		handle = 0;
		if(inited)
		{
			lt_dlexit();
			inited = false;
		}
	}

public:

	bool IsValid() const
	{
		return handle != 0;
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		if(!IsValid())
		{
			return nullptr;
		}
		return reinterpret_cast<FuncPtr>(lt_dlsym(handle, symbol.c_str()));
	}

};


#elif defined(MPT_WITH_DL)


class LibraryHandle
{

private:

	void* handle;

public:

	LibraryHandle(const mpt::LibraryPath &path)
		: handle(NULL)
	{
		handle = dlopen(path.GetFileName().AsNative().c_str(), RTLD_NOW);
	}

	LibraryHandle(const LibraryHandle &) = delete;

	LibraryHandle & operator=(const LibraryHandle &) = delete;

	~LibraryHandle()
	{
		if(IsValid())
		{
			dlclose(handle);
		}
		handle = NULL;
	}

public:

	bool IsValid() const
	{
		return handle != NULL;
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		if(!IsValid())
		{
			return NULL;
		}
		return reinterpret_cast<FuncPtr>(dlsym(handle, symbol.c_str()));
	}

};


#else // MPT_OS


// dummy implementation

class LibraryHandle
{
public:

	LibraryHandle(const mpt::LibraryPath &path)
	{
		MPT_UNREFERENCED_PARAMETER(path);
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
		return false;
	}

	FuncPtr GetProcAddress(const std::string &symbol) const
	{
		MPT_UNREFERENCED_PARAMETER(symbol);
		if(!IsValid())
		{
			return nullptr;
		}
		return nullptr;
	}

};


#endif // MPT_OS


LibraryPath::LibraryPath(mpt::LibrarySearchPath searchPath, class mpt::PathString const &fileName)
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
