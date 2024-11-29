/*
 * AutoSaver.cpp
 * -------------
 * Purpose: Class for automatically saving open modules at a specified interval.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "AutoSaver.h"
#include "FileDialog.h"
#include "FolderScanner.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "Reporting.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "../soundlib/mod_specifications.h"
#include "mpt/fs/fs.hpp"

#include <algorithm>
#include <filesystem>


OPENMPT_NAMESPACE_BEGIN


CAutoSaver::CAutoSaver()
	: m_lastSave{timeGetTime()}
{
}


bool CAutoSaver::IsEnabled() const
{
	return TrackerSettings::Instance().AutosaveEnabled;
}

bool CAutoSaver::GetUseOriginalPath() const
{
	return TrackerSettings::Instance().AutosaveUseOriginalPath;
}

mpt::PathString CAutoSaver::GetPath() const
{
	return TrackerSettings::Instance().AutosavePath.GetDefaultDir();
}

uint32 CAutoSaver::GetHistoryDepth() const
{
	return TrackerSettings::Instance().AutosaveHistoryDepth;
}

std::chrono::minutes CAutoSaver::GetSaveInterval() const
{
	return std::chrono::minutes{TrackerSettings::Instance().AutosaveIntervalMinutes.Get()};
}


mpt::chrono::days CAutoSaver::GetRetentionTime() const
{
	return mpt::chrono::days{TrackerSettings::Instance().AutosaveRetentionTimeDays.Get()};
}


bool CAutoSaver::DoSave()
{
	// Do nothing if we are already saving, or if time to save has not been reached yet.
	if(m_saveInProgress || !CheckTimer(timeGetTime()))
		return true;
		
	bool success = true, clearStatus = false;
	m_saveInProgress = true;

	theApp.BeginWaitCursor();  // Display hour glass

	for(auto &modDoc : theApp.GetOpenDocuments())
	{
		if(modDoc->ModifiedSinceLastAutosave())
		{
			clearStatus = true;
			CMainFrame::GetMainFrame()->SetHelpText(MPT_CFORMAT("Auto-saving {}...")(modDoc->GetPathNameMpt().GetFilename()));
			if(SaveSingleFile(*modDoc))
			{
				CleanUpAutosaves(*modDoc);
			} else
			{
				TrackerSettings::Instance().AutosaveEnabled = false;
				Reporting::Warning("Warning: Auto Save failed and has been disabled. Please:\n- Review your Auto Save paths\n- Check available disk space and filesystem access rights");
				success = false;
			}
		}
	}
	CleanUpAutosaves();

	m_lastSave = timeGetTime();
	theApp.EndWaitCursor();  // End display hour glass
	m_saveInProgress = false;

	if(clearStatus)
		CMainFrame::GetMainFrame()->SetHelpText(_T(""));
	
	return success;
}


bool CAutoSaver::CheckTimer(DWORD curTime) const
{
	return std::chrono::milliseconds{curTime - m_lastSave} >= GetSaveInterval();
}


mpt::PathString CAutoSaver::GetBasePath(const CModDoc &modDoc, bool createPath) const
{
	mpt::PathString path;
	if(GetUseOriginalPath())
	{
		if(modDoc.m_bHasValidPath && !(path = modDoc.GetPathNameMpt()).empty())
		{
			// File has a user-chosen path - remove filename
			path = path.GetDirectoryWithDrive();
		} else
		{
			// If it doesn't, fall back to default
			path = TrackerSettings::GetDefaultAutosavePath();
		}
	} else
	{
		path = GetPath();
	}
	std::error_code ec;
	if(createPath)
		std::filesystem::create_directories(mpt::support_long_path(path.AsNative()), ec);
	if(!mpt::native_fs{}.is_directory(path))
		path = theApp.GetConfigPath();

	return path.WithTrailingSlash();
}


mpt::PathString CAutoSaver::GetBaseName(const CModDoc &modDoc) const
{
	return mpt::PathString::FromCString(modDoc.GetTitle()).AsSanitizedComponent();
}


mpt::PathString CAutoSaver::BuildFileName(const CModDoc &modDoc) const
{
	mpt::PathString name = GetBasePath(modDoc, true) + GetBaseName(modDoc);
	const CString timeStamp = CTime::GetCurrentTime().Format(_T(".AutoSave.%Y%m%d.%H%M%S."));
	name += mpt::PathString::FromCString(timeStamp);  // Append backtup tag + timestamp
	name += mpt::PathString::FromUnicode(modDoc.GetSoundFile().GetModSpecifications().GetFileExtension());
	return name;
}


bool CAutoSaver::SaveSingleFile(CModDoc &modDoc)
{
	mpt::PathString fileName = BuildFileName(modDoc);

	// We are actually not going to show the log for autosaved files.
	ScopedLogCapturer logcapturer(modDoc, _T(""), nullptr, false);
	return modDoc.SaveFile(fileName, GetUseOriginalPath());
}


void CAutoSaver::CleanUpAutosaves(const CModDoc &modDoc) const
{
	// Find all autosave files for this document, and delete the oldest ones if there are more than the user wants.
	std::vector<mpt::PathString> foundfiles;
	FolderScanner scanner(GetBasePath(modDoc, false), FolderScanner::kOnlyFiles, GetBaseName(modDoc) + P_(".AutoSave.*.*.*"));
	mpt::PathString fileName;
	while(scanner.Next(fileName))
	{
		foundfiles.push_back(std::move(fileName));
	}
	std::sort(foundfiles.begin(), foundfiles.end());
	size_t filesToDelete = std::max(static_cast<size_t>(GetHistoryDepth()), foundfiles.size()) - GetHistoryDepth();
	const bool deletePermanently = TrackerSettings::Instance().AutosaveDeletePermanently;
	for(size_t i = 0; i < filesToDelete; i++)
	{
		DeleteAutosave(foundfiles[i], deletePermanently);
	}
}


// Find all autosave files in the autosave folder and delete all of those older than X days
void CAutoSaver::CleanUpAutosaves() const
{
	if(GetUseOriginalPath() || !GetRetentionTime().count())
		return;
	auto path = GetPath();
	if(!mpt::native_fs{}.is_directory(path))
		return;
	const std::chrono::seconds maxAge = GetRetentionTime();
	const bool deletePermanently = TrackerSettings::Instance().AutosaveDeletePermanently;
	FILETIME sysFileTime;
	GetSystemTimeAsFileTime(&sysFileTime);
	const int64 currentTime = static_cast<int64>(mpt::bit_cast<ULARGE_INTEGER>(sysFileTime).QuadPart);
	FolderScanner scanner(std::move(path), FolderScanner::kOnlyFiles, P_("*.AutoSave.*.*.*"));
	mpt::PathString fileName;
	WIN32_FIND_DATA fileInfo;
	while(scanner.Next(fileName, &fileInfo))
	{
		const auto fileTime = static_cast<int64>(mpt::bit_cast<ULARGE_INTEGER>(fileInfo.ftLastWriteTime).QuadPart);
		const auto timeDiff = std::chrono::seconds{(currentTime - fileTime) / 10000000};
		if(timeDiff >= maxAge)
			DeleteAutosave(fileName, deletePermanently);
	}
}


void CAutoSaver::DeleteAutosave(const mpt::PathString &fileName, bool deletePermanently)
{
	// Create double-null-terminated path list
	// Note: We could supply multiple filenames here. However, if the files in total are too big for the recycling bin,
	// this will cause all of them to be deleted permanently. If we supply them one by one, at least some of them
	// will go to the recycling bin until it is full.
	mpt::winstring path = fileName.AsNative() + _T('\0');
	SHFILEOPSTRUCT fos{};
	fos.hwnd = CMainFrame::GetMainFrame()->m_hWnd;
	fos.wFunc = FO_DELETE;
	fos.pFrom = path.c_str();
	fos.fFlags = (deletePermanently ? 0 : FOF_ALLOWUNDO) | FOF_NO_UI;
	SHFileOperation(&fos);
}

OPENMPT_NAMESPACE_END
