/*
 * FolderScanner.cpp
 * -----------------
 * Purpose: Class for finding files in folders.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "FolderScanner.h"

OPENMPT_NAMESPACE_BEGIN

FolderScanner::FolderScanner(const mpt::PathString &path, bool findInSubDirs) : paths(1, path), findInSubDirs(findInSubDirs), hFind(INVALID_HANDLE_VALUE)
//-------------------------------------------------------------------------------------------------------------------------------------------------------
{
	MemsetZero(wfd);
}


FolderScanner::~FolderScanner()
//-----------------------------
{
	FindClose(hFind);
}


// Return one file at a time in parameter file. Returns true if a file was found (file parameter is valid), false if no more files can be found.
bool FolderScanner::NextFile(mpt::PathString &file)
//-------------------------------------------------
{
	bool foundFile = false;
	do
	{
		if(hFind == INVALID_HANDLE_VALUE)
		{
			if(paths.empty())
			{
				return false;
			}

			currentPath = paths.back();
			paths.pop_back();
			currentPath.EnsureTrailingSlash();
			hFind = FindFirstFileW((currentPath + MPT_PATHSTRING("*.*")).AsNative().c_str(), &wfd);
		}

		BOOL nextFile = FALSE;
		if(hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				file = currentPath + mpt::PathString::FromNative(wfd.cFileName);
				if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(findInSubDirs && wcscmp(wfd.cFileName, L"..") && wcscmp(wfd.cFileName, L"."))
					{
						// Add sub directory
						paths.push_back(file);
					}
				} else
				{
					foundFile = true;
				}
			} while((nextFile = FindNextFileW(hFind, &wfd)) != FALSE && !foundFile);
		}
		if(nextFile == FALSE)
		{
			// Done with this directory, advance to next
			if(hFind != INVALID_HANDLE_VALUE)
			{
				FindClose(hFind);
			}
			hFind = INVALID_HANDLE_VALUE;
		}
	} while(!foundFile);
	return true;
}

OPENMPT_NAMESPACE_END
