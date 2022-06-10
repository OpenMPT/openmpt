/*
 * mptFS.cpp
 * ---------
 * Purpose:
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "stdafx.h"
#include "mptFS.h"

#include "mptBaseMacros.h"

#if MPT_OS_WINDOWS
#include <windows.h>
#if defined(MODPLUG_TRACKER)
#include <shlwapi.h>
#endif
#endif



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{



#if defined(MODPLUG_TRACKER) && MPT_OS_WINDOWS



namespace FS
{



bool IsDirectory(const mpt::PathString &path)
{
	// Using PathIsDirectoryW here instead would increase libopenmpt dependencies by shlwapi.dll.
	// GetFileAttributesW also does the job just fine.
	#if MPT_OS_WINDOWS_WINRT
		WIN32_FILE_ATTRIBUTE_DATA data = {};
		if(::GetFileAttributesExW(path.AsNative().c_str(), GetFileExInfoStandard, &data) == 0)
		{
			return false;
		}
		DWORD dwAttrib = data.dwFileAttributes;
	#else // !MPT_OS_WINDOWS_WINRT
		DWORD dwAttrib = ::GetFileAttributes(path.AsNative().c_str());
	#endif // MPT_OS_WINDOWS_WINRT
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool IsFile(const mpt::PathString &path)
{
	#if MPT_OS_WINDOWS_WINRT
		WIN32_FILE_ATTRIBUTE_DATA data = {};
		if (::GetFileAttributesExW(path.AsNative().c_str(), GetFileExInfoStandard, &data) == 0)
		{
			return false;
		}
		DWORD dwAttrib = data.dwFileAttributes;
	#else // !MPT_OS_WINDOWS_WINRT
		DWORD dwAttrib = ::GetFileAttributes(path.AsNative().c_str());
	#endif // MPT_OS_WINDOWS_WINRT
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool PathExists(const mpt::PathString &path)
{
	return ::PathFileExists(path.AsNative().c_str()) != FALSE;
}



bool DeleteDirectoryTree(mpt::PathString path)
{
	if(path.AsNative().empty())
	{
		return false;
	}
	if(PathIsRelative(path.AsNative().c_str()) == TRUE)
	{
		return false;
	}
	if(!mpt::FS::PathExists(path))
	{
		return true;
	}
	if(!mpt::FS::IsDirectory(path))
	{
		return false;
	}
	path = path.WithTrailingSlash();
	HANDLE hFind = NULL;
	WIN32_FIND_DATA wfd = {};
	hFind = FindFirstFile((path + P_("*.*")).AsNative().c_str(), &wfd);
	if(hFind != NULL && hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			mpt::PathString filename = mpt::PathString::FromNative(wfd.cFileName);
			if(filename != P_(".") && filename != P_(".."))
			{
				filename = path + filename;
				if(mpt::FS::IsDirectory(filename))
				{
					if(!mpt::FS::DeleteDirectoryTree(filename))
					{
						return false;
					}
				} else if(mpt::FS::IsFile(filename))
				{
					if(DeleteFile(filename.AsNative().c_str()) == 0)
					{
						return false;
					}
				}
			}
		} while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	if(RemoveDirectory(path.AsNative().c_str()) == 0)
	{
		return false;
	}
	return true;
}



} // namespace FS



mpt::PathString GetExecutableDirectory()
{
	std::vector<TCHAR> exeFileName(MAX_PATH);
	while(GetModuleFileName(0, exeFileName.data(), mpt::saturate_cast<DWORD>(exeFileName.size())) >= exeFileName.size())
	{
		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			return mpt::PathString();
		}
		exeFileName.resize(exeFileName.size() * 2);
	}
	return mpt::GetAbsolutePath(mpt::PathString::FromNative(exeFileName.data()).GetDirectoryWithDrive());
}



#if !MPT_OS_WINDOWS_WINRT

mpt::PathString GetSystemDirectory()
{
	DWORD size = ::GetSystemDirectory(nullptr, 0);
	std::vector<TCHAR> path(size + 1);
	if(!::GetSystemDirectory(path.data(), size + 1))
	{
		return mpt::PathString();
	}
	return mpt::PathString::FromNative(path.data()) + P_("\\");
}

#endif // !MPT_OS_WINDOWS_WINRT



mpt::PathString GetTempDirectory()
{
	DWORD size = GetTempPath(0, nullptr);
	if(size)
	{
		std::vector<TCHAR> tempPath(size + 1);
		if(GetTempPath(size + 1, tempPath.data()))
		{
			return mpt::PathString::FromNative(tempPath.data());
		}
	}
	// use exe directory as fallback
	return mpt::GetExecutableDirectory();
}




#else
MPT_MSVC_WORKAROUND_LNK4221(mptFS)
#endif // MODPLUG_TRACKER && MPT_OS_WINDOWS



} // namespace mpt



OPENMPT_NAMESPACE_END
