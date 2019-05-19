/*
 * FolderScanner.h
 * ---------------
 * Purpose: Class for finding files in folders.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

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

protected:
	std::vector<mpt::PathString> m_paths;
	mpt::PathString m_currentPath;
	mpt::PathString m_filter;
	HANDLE m_hFind;
	WIN32_FIND_DATA m_wfd;
	FlagSet<ScanType> m_type;

public:
	FolderScanner(const mpt::PathString &path, FlagSet<ScanType> type, mpt::PathString filter = MPT_PATHSTRING("*.*"));
	~FolderScanner();

	// Return one file or directory at a time in parameter file. Returns true if a file was found (file parameter is valid), false if no more files can be found (file parameter is not touched).
	bool Next(mpt::PathString &file);
};

MPT_DECLARE_ENUM(FolderScanner::ScanType)

OPENMPT_NAMESPACE_END
