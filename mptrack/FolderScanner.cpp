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

FolderScanner::FolderScanner(mpt::PathString path, FlagSet<ScanType> type, mpt::PathString filter)
	: m_paths{1, std::move(path)}
	, m_filter{std::move(filter)}
	, m_type{type}
{
}


FolderScanner::~FolderScanner()
{
	if(m_hFind != INVALID_HANDLE_VALUE)
		FindClose(m_hFind);
}

#if MPT_COMPILER_MSVC
// silence static analyzer false positive for FindFirstFile
#pragma warning(push)
#pragma warning(disable:6387) // 'HANDLE' could be '0'
#endif // MPT_COMPILER_MSVC
bool FolderScanner::Next(mpt::PathString &file, WIN32_FIND_DATA *fileInfo)
{
	bool found = false;
	do
	{
		if(m_hFind == INVALID_HANDLE_VALUE)
		{
			if(m_paths.empty())
				return false;

			m_currentPath = m_paths.back().WithTrailingSlash();
			m_paths.pop_back();
			m_hFind = FindFirstFile(mpt::support_long_path((m_currentPath + m_filter).AsNative()).c_str(), &m_wfd);
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
						// Add sub directory
						if(m_type[kFindInSubDirectories])
							m_paths.push_back(file);
						if(m_type[kOnlyDirectories])
							found = true;
					}
				} else if(m_type[kOnlyFiles])
				{
					found = true;
				}
				if(found && fileInfo)
					*fileInfo = m_wfd;
			} while((nextFile = FindNextFile(m_hFind, &m_wfd)) != FALSE && !found);
		}
		if(nextFile == FALSE)
		{
			// Done with this directory, advance to next
			if(m_hFind != INVALID_HANDLE_VALUE)
				FindClose(m_hFind);
			m_hFind = INVALID_HANDLE_VALUE;
		}
	} while(!found);
	return true;
}
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC

OPENMPT_NAMESPACE_END
