/*
 * FolderScanner.h
 * ---------------
 * Purpose: Class for finding files in folders.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "../common/mptPathString.h"

OPENMPT_NAMESPACE_BEGIN

class FolderScanner
{
public:
	enum ScanType
	{
		kOnlyFiles = 0x01,
		kOnlyDirectories = 0x02,
		kFilesAndDirectories = kOnlyFiles | kOnlyDirectories,
		kFindInSubDirectories = 0x04,
	};

	FolderScanner(mpt::PathString path, FlagSet<ScanType> type, mpt::PathString filter = P_("*.*"));
	~FolderScanner();

	// Return one file or directory at a time in parameter file. Returns true if a file was found (file parameter is valid), false if no more files can be found (file parameter is not touched).
	bool Next(mpt::PathString &file, WIN32_FIND_DATA *fileInfo = nullptr);

protected:
	std::vector<mpt::PathString> m_paths;
	mpt::PathString m_currentPath;
	const mpt::PathString m_filter;
	HANDLE m_hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA m_wfd{};
	const FlagSet<ScanType> m_type;
};

MPT_DECLARE_ENUM(FolderScanner::ScanType)

OPENMPT_NAMESPACE_END
