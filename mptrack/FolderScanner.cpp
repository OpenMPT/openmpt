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
#include <tchar.h>

OPENMPT_NAMESPACE_BEGIN

FolderScanner::FolderScanner(const mpt::PathString &path, FlagSet<ScanType> type, mpt::PathString filter)
	: m_paths(1, path)
	, m_filter(std::move(filter))
	, m_hFind(INVALID_HANDLE_VALUE)
	, m_type(type)
{
	MemsetZero(m_wfd);
}


FolderScanner::~FolderScanner()
{
	FindClose(m_hFind);
}


bool FolderScanner::Next(mpt::PathString &file)
{
	bool found = false;
	do
	{
		if(m_hFind == INVALID_HANDLE_VALUE)
		{
			if(m_paths.empty())
			{
				return false;
			}

			m_currentPath = m_paths.back();
			m_paths.pop_back();
			m_currentPath.EnsureTrailingSlash();
			m_hFind = FindFirstFile((m_currentPath + m_filter).AsNative().c_str(), &m_wfd);
		}

		BOOL nextFile = FALSE;
		if(m_hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				file = m_currentPath + mpt::PathString::FromNative(m_wfd.cFileName);
				if(m_wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(_tcscmp(m_wfd.cFileName, _T("..")) && _tcscmp(m_wfd.cFileName, _T(".")))
					{
						if(m_type[kFindInSubDirectories])
						{
							// Add sub directory
							m_paths.push_back(file);
						}
						if(m_type[kOnlyDirectories])
						{
							found = true;
						}
					}
				} else if(m_type[kOnlyFiles])
				{
					found = true;
				}
			} while((nextFile = FindNextFile(m_hFind, &m_wfd)) != FALSE && !found);
		}
		if(nextFile == FALSE)
		{
			// Done with this directory, advance to next
			if(m_hFind != INVALID_HANDLE_VALUE)
			{
				FindClose(m_hFind);
			}
			m_hFind = INVALID_HANDLE_VALUE;
		}
	} while(!found);
	return true;
}

OPENMPT_NAMESPACE_END
