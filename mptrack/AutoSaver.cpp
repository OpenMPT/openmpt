/*
 * AutoSaver.cpp
 * -------------
 * Purpose: Class for automatically saving open modules at a specified interval.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "AutoSaver.h"
#include "FileDialog.h"
#include "FolderScanner.h"
#include "resource.h"
#include "../soundlib/mod_specifications.h"
#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


CAutoSaver::CAutoSaver()
	: m_lastSave(timeGetTime())
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

uint32 CAutoSaver::GetSaveInterval() const
{
	return TrackerSettings::Instance().AutosaveIntervalMinutes;
}


bool CAutoSaver::DoSave(DWORD curTime)
{
	bool success = true;

	//If time to save and not already having save in progress.
	if (CheckTimer(curTime) && !m_saveInProgress)
	{ 
		m_saveInProgress = true;

		theApp.BeginWaitCursor(); //display hour glass

		for(auto &modDoc : theApp.GetOpenDocuments())
		{
			if(modDoc->ModifiedSinceLastAutosave())
			{
				if(SaveSingleFile(*modDoc))
				{
					CleanUpBackups(*modDoc);
				} else
				{
					TrackerSettings::Instance().AutosaveEnabled = false;
					Reporting::Warning("Warning: Auto Save failed and has been disabled. Please:\n- Review your Auto Save paths\n- Check available disk space and filesystem access rights");
					success = false;
				}
			}
		}

		m_lastSave = timeGetTime();
		theApp.EndWaitCursor();  // End display hour glass
		m_saveInProgress = false;
	}
	
	return success;
}


bool CAutoSaver::CheckTimer(DWORD curTime) const
{
	return (curTime - m_lastSave) >= GetSaveIntervalMilliseconds();
}


mpt::PathString CAutoSaver::GetBasePath(const CModDoc &modDoc, bool createPath) const
{
	mpt::PathString path;
	if(GetUseOriginalPath())
	{
		if(modDoc.m_bHasValidPath && !(path = modDoc.GetPathNameMpt()).empty())
		{
			// File has a user-chosen path - remove filename
			path = path.GetPath();
		} else
		{
			// if it doesn't, put it in settings dir
			path = theApp.GetConfigPath() + P_("Autosave\\");
			if(createPath && !CreateDirectory(path.AsNative().c_str(), nullptr) && GetLastError() == ERROR_PATH_NOT_FOUND)
				path = theApp.GetConfigPath();
			else if(!createPath && !path.IsDirectory())
				path = theApp.GetConfigPath();
		}
	} else
	{
		path = GetPath();
	}
	return path.EnsureTrailingSlash();
}


mpt::PathString CAutoSaver::GetBaseName(const CModDoc &modDoc) const
{
	return mpt::PathString::FromCString(modDoc.GetTitle()).SanitizeComponent();
}


mpt::PathString CAutoSaver::BuildFileName(const CModDoc &modDoc) const
{
	mpt::PathString name = GetBasePath(modDoc, true) + GetBaseName(modDoc);
	const CString timeStamp = CTime::GetCurrentTime().Format(_T(".AutoSave.%Y%m%d.%H%M%S."));
	name += mpt::PathString::FromCString(timeStamp);  //append backtup tag + timestamp
	name += mpt::PathString::FromUTF8(modDoc.GetSoundFile().GetModSpecifications().fileExtension);
	return name;
}


bool CAutoSaver::SaveSingleFile(CModDoc &modDoc)
{
	// We do not call CModDoc::DoSave as this populates the Recent Files
	// list with backups... hence we have duplicated code.. :(
	CSoundFile &sndFile = modDoc.GetSoundFile();
	
	mpt::PathString fileName = BuildFileName(modDoc);

	// We are actually not going to show the log for autosaved files.
	ScopedLogCapturer logcapturer(modDoc, _T(""), nullptr, false);

	bool success = false;
	mpt::ofstream f(fileName, std::ios::binary);
	if(f)
	{
		switch(modDoc.GetSoundFile().GetBestSaveFormat())
		{
		case MOD_TYPE_MOD: success = sndFile.SaveMod(f); break;
		case MOD_TYPE_S3M: success = sndFile.SaveS3M(f); break;
		case MOD_TYPE_XM:  success = sndFile.SaveXM(f); break;
		case MOD_TYPE_IT:  success = sndFile.SaveIT(f, fileName); break;
		case MOD_TYPE_MPT: success = sndFile.SaveIT(f, fileName); break;
		}
	}

	return success;
}


void CAutoSaver::CleanUpBackups(const CModDoc &modDoc) const
{
	// Find all autosave files for this document, and delete the oldest ones if there are more than the user wants.
	std::vector<mpt::PathString> foundfiles;
	FolderScanner scanner(GetBasePath(modDoc, false), FolderScanner::kOnlyFiles, GetBaseName(modDoc) + P_(".AutoSave.*"));
	mpt::PathString fileName;
	while(scanner.Next(fileName))
	{
		foundfiles.push_back(std::move(fileName));
	}
	std::sort(foundfiles.begin(), foundfiles.end());
	size_t filesToDelete = std::max(static_cast<size_t>(GetHistoryDepth()), foundfiles.size()) - GetHistoryDepth();
	for(size_t i = 0; i < filesToDelete; i++)
	{
		DeleteFile(foundfiles[i].AsNative().c_str());
	}
}


OPENMPT_NAMESPACE_END
