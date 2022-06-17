/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_FS_FS_HPP
#define MPT_FS_FS_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#if MPT_OS_WINDOWS
#include "mpt/base/saturate_cast.hpp"
#endif // MPT_OS_WINDOWS
#include "mpt/path/native_path.hpp"

#if MPT_OS_WINDOWS
#include <vector>
#endif // MPT_OS_WINDOWS

#if MPT_OS_WINDOWS
#include <windows.h>
#endif // MPT_OS_WINDOWS



namespace mpt {
inline namespace MPT_INLINE_NS {



template <typename Tpath>
class fs;



template <>
class fs<mpt::native_path> {



public:

	bool is_directory(const mpt::native_path & path) {
		// Using PathIsDirectoryW here instead would increase libopenmpt dependencies by shlwapi.dll.
		// GetFileAttributesW also does the job just fine.
#if MPT_OS_WINDOWS_WINRT
		WIN32_FILE_ATTRIBUTE_DATA data = {};
		if (::GetFileAttributesExW(path.AsNative().c_str(), GetFileExInfoStandard, &data) == 0) {
			return false;
		}
		DWORD dwAttrib = data.dwFileAttributes;
#else // !MPT_OS_WINDOWS_WINRT
		DWORD dwAttrib = ::GetFileAttributes(path.AsNative().c_str());
#endif // MPT_OS_WINDOWS_WINRT
		return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	bool is_file(const mpt::native_path & path) {
#if MPT_OS_WINDOWS_WINRT
		WIN32_FILE_ATTRIBUTE_DATA data = {};
		if (::GetFileAttributesExW(path.AsNative().c_str(), GetFileExInfoStandard, &data) == 0) {
			return false;
		}
		DWORD dwAttrib = data.dwFileAttributes;
#else // !MPT_OS_WINDOWS_WINRT
		DWORD dwAttrib = ::GetFileAttributes(path.AsNative().c_str());
#endif // MPT_OS_WINDOWS_WINRT
		return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	bool exists(const mpt::native_path & path) {
		return ::PathFileExists(path.AsNative().c_str()) != FALSE;
	}



public:

#if !(MPT_OS_WINDOWS_WINRT && (_WIN32_WINNT < 0x0a00))
	mpt::native_path absolute(const mpt::native_path & path) {
		DWORD size = ::GetFullPathName(path.AsNative().c_str(), 0, nullptr, nullptr);
		if (size == 0) {
			return path;
		}
		std::vector<TCHAR> fullPathName(size, TEXT('\0'));
		if (::GetFullPathName(path.AsNative().c_str(), size, fullPathName.data(), nullptr) == 0) {
			return path;
		}
		return mpt::native_path::FromNative(fullPathName.data());

	}
#endif // !MPT_OS_WINDOWS_WINRT

public:

	mpt::native_path get_application_directory() {
		std::vector<TCHAR> exeFileName(MAX_PATH);
		while (::GetModuleFileName(0, exeFileName.data(), mpt::saturate_cast<DWORD>(exeFileName.size())) >= exeFileName.size()) {
			if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				return mpt::native_path();
			}
			exeFileName.resize(exeFileName.size() * 2);
		}
		return mpt::fs<mpt::native_path>{}.absolute(mpt::native_path::FromNative(exeFileName.data()).GetDirectoryWithDrive());
	}

#if !MPT_OS_WINDOWS_WINRT
	mpt::native_path get_system_directory() {
		DWORD size = ::GetSystemDirectory(nullptr, 0);
		std::vector<TCHAR> path(size + 1);
		if (!::GetSystemDirectory(path.data(), size + 1)) {
			return mpt::native_path();
		}
		return mpt::native_path::FromNative(path.data()) + MPT_NATIVE_PATH("\\");
	}
#endif // !MPT_OS_WINDOWS_WINRT

	mpt::native_path get_temp_directory() {
		DWORD size = ::GetTempPath(0, nullptr);
		if (size) {
			std::vector<TCHAR> tempPath(size + 1);
			if (::GetTempPath(size + 1, tempPath.data())) {
				return mpt::native_path::FromNative(tempPath.data());
			}
		}
		// use app directory as fallback
		return mpt::fs<mpt::native_path>{}.get_application_directory();
	}



public:

	bool delete_tree(mpt::native_path path) {
		if (path.AsNative().empty()) {
			return false;
		}
		if (::PathIsRelative(path.AsNative().c_str()) == TRUE) {
			return false;
		}
		if (!mpt::fs<mpt::native_path>{}.exists(path)) {
			return true;
		}
		if (!mpt::fs<mpt::native_path>{}.is_directory(path)) {
			return false;
		}
		path = path.WithTrailingSlash();
		HANDLE hFind = NULL;
		WIN32_FIND_DATA wfd = {};
		hFind = ::FindFirstFile((path + MPT_NATIVE_PATH("*.*")).AsNative().c_str(), &wfd);
		if (hFind != NULL && hFind != INVALID_HANDLE_VALUE) {
			do {
				mpt::native_path filename = mpt::native_path::FromNative(wfd.cFileName);
				if (filename != MPT_NATIVE_PATH(".") && filename != MPT_NATIVE_PATH("..")) {
					filename = path + filename;
					if (mpt::fs<mpt::native_path>{}.is_directory(filename)) {
						if (!mpt::fs<mpt::native_path>{}.delete_tree(filename)) {
							return false;
						}
					} else if (mpt::fs<mpt::native_path>{}.is_file(filename)) {
						if (::DeleteFile(filename.AsNative().c_str()) == 0) {
							return false;
						}
					}
				}
			} while(::FindNextFile(hFind, &wfd));
			::FindClose(hFind);
		}
		if (::RemoveDirectory(path.AsNative().c_str()) == 0) {
			return false;
		}
		return true;
	}



};



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_FS_FS_HPP
