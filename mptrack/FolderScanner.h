/*
 * FolderScanner.h
 * ---------------
 * Purpose: Class for finding files in folders.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/mptPathString.h"

OPENMPT_NAMESPACE_BEGIN

class FolderScanner
{
protected:
	std::vector<mpt::PathString> paths;
	mpt::PathString currentPath;
	HANDLE hFind;
	WIN32_FIND_DATAW wfd;
	bool findInSubDirs;

public:
	FolderScanner(const mpt::PathString &path, bool findInSubDirs);
	~FolderScanner();
	bool NextFile(mpt::PathString &file);
};

OPENMPT_NAMESPACE_END
