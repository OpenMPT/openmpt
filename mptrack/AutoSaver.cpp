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
#include "resource.h"
#include "../soundlib/mod_specifications.h"
#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////////
// AutoSaver.cpp : implementation file
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////
// Construction/Destruction
///////////////////////////

CAutoSaver::CAutoSaver()
//----------------------
	: m_bSaveInProgress(false)
	, m_nLastSave(timeGetTime())
{
}


bool CAutoSaver::IsEnabled() const
//--------------------------------
{
	return TrackerSettings::Instance().AutosaveEnabled;
}

bool CAutoSaver::GetUseOriginalPath() const
//-----------------------------------------
{
	return TrackerSettings::Instance().AutosaveUseOriginalPath;
}

mpt::PathString CAutoSaver::GetPath() const
//-----------------------------------------
{
	return TrackerSettings::Instance().AutosavePath.GetDefaultDir();
}

uint32 CAutoSaver::GetHistoryDepth() const
//----------------------------------------
{
	return TrackerSettings::Instance().AutosaveHistoryDepth;
}

uint32 CAutoSaver::GetSaveInterval() const
//----------------------------------------
{
	return TrackerSettings::Instance().AutosaveIntervalMinutes;
}


//////////////
// Entry Point
//////////////


bool CAutoSaver::DoSave(DWORD curTime)
//------------------------------------
{
	bool success = true;

	//If time to save and not already having save in progress.
	if (CheckTimer(curTime) && !m_bSaveInProgress)
	{ 
		m_bSaveInProgress = true;

		theApp.BeginWaitCursor(); //display hour glass

		auto docs = theApp.GetOpenDocuments();
		for(auto doc = docs.begin(); doc != docs.end(); doc++)
		{
			CModDoc &modDoc = **doc;
			if(modDoc.ModifiedSinceLastAutosave())
			{
				if(SaveSingleFile(modDoc))
				{
					CleanUpBackups(modDoc);
				} else
				{
					TrackerSettings::Instance().AutosaveEnabled = false;
					Reporting::Warning("Warning: Auto Save failed and has been disabled. Please:\n- Review your Auto Save paths\n- Check available disk space and filesystem access rights");
					success = false;
				}
			}
		}

		m_nLastSave = timeGetTime();
		theApp.EndWaitCursor(); //end display hour glass
		m_bSaveInProgress = false;
	}
	
	return success;
}


///////////////////////////
// Implementation internals
///////////////////////////


bool CAutoSaver::CheckTimer(DWORD curTime)
//----------------------------------------
{
	DWORD curInterval = curTime - m_nLastSave;
	return (curInterval >= GetSaveIntervalMilliseconds());
}


mpt::PathString CAutoSaver::BuildFileName(CModDoc &modDoc)
//--------------------------------------------------------
{
	mpt::PathString name;
	
	if(GetUseOriginalPath())
	{
		if(modDoc.m_bHasValidPath && !(name = modDoc.GetPathNameMpt()).empty())
		{
			// File has a user-chosen path - no change required
		} else
		{
			// if it doesn't, put it in settings dir
			name = theApp.GetConfigPath() + MPT_PATHSTRING("Autosave\\");
			if(!CreateDirectoryW(name.AsNative().c_str(), nullptr) && GetLastError() == ERROR_PATH_NOT_FOUND)
			{
				name = theApp.GetConfigPath();
			}
			name += mpt::PathString::FromCStringSilent(modDoc.GetTitle()).SanitizeComponent();
		}
	} else
	{
		name = GetPath() + mpt::PathString::FromCStringSilent(modDoc.GetTitle()).SanitizeComponent();
	}
	
	const mpt::ustring timeStamp = mpt::ToUnicode((CTime::GetCurrentTime()).Format(_T(".AutoSave.%Y%m%d.%H%M%S.")));
	name += mpt::PathString::FromUnicode(timeStamp);			//append backtup tag + timestamp
	name += mpt::PathString::FromUTF8(modDoc.GetrSoundFile().GetModSpecifications().fileExtension);

	return name;
}


bool CAutoSaver::SaveSingleFile(CModDoc &modDoc)
//----------------------------------------------
{
	// We do not call CModDoc::DoSave as this populates the Recent Files
	// list with backups... hence we have duplicated code.. :(
	CSoundFile &sndFile = modDoc.GetrSoundFile(); 
	
	mpt::PathString fileName = BuildFileName(modDoc);

	// We are acutally not going to show the log for autosaved files.
	ScopedLogCapturer logcapturer(modDoc, "", nullptr, false);

	bool success = false;
	switch(modDoc.GetrSoundFile().GetBestSaveFormat())
	{
	case MOD_TYPE_MOD:
		success = sndFile.SaveMod(fileName);
		break;

	case MOD_TYPE_S3M:
		success = sndFile.SaveS3M(fileName);
		break;

	case MOD_TYPE_XM:
		success = sndFile.SaveXM(fileName);
		break;

	case MOD_TYPE_IT:
		success = sndFile.SaveIT(fileName);
		break;

	case MOD_TYPE_MPT:
		//Using IT save function also for MPT.
		success = sndFile.SaveIT(fileName);
		break;
	}

	return success;
}


void CAutoSaver::CleanUpBackups(const CModDoc &modDoc)
//----------------------------------------------------
{
	mpt::PathString path;
	
	if(GetUseOriginalPath())
	{
		if (modDoc.m_bHasValidPath && !(path = modDoc.GetPathNameMpt()).empty())
		{
			// File has a user-chosen path - remove filename
			path = path.GetPath();
		} else
		{
			// if it doesn't, put it in settings dir
			path = theApp.GetConfigPath() + MPT_PATHSTRING("Autosave\\");
		}
	} else
	{
		path = GetPath();
	}

	std::vector<mpt::PathString> foundfiles;
	mpt::PathString searchPattern = path + mpt::PathString::FromCStringSilent(modDoc.GetTitle()).SanitizeComponent() + MPT_PATHSTRING(".AutoSave.*");

	WIN32_FIND_DATAW findData;
	MemsetZero(findData);
	HANDLE hFindFile = FindFirstFileW(searchPattern.AsNative().c_str(), &findData);
	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				foundfiles.push_back(path + mpt::PathString::FromNative(findData.cFileName));
			}
		} while(FindNextFileW(hFindFile, &findData));
		FindClose(hFindFile);
		hFindFile = INVALID_HANDLE_VALUE;
	}
	std::sort(foundfiles.begin(), foundfiles.end());
	
	while(foundfiles.size() > static_cast<size_t>(GetHistoryDepth()))
	{
		DeleteFileW(foundfiles[0].AsNative().c_str());
		foundfiles.erase(foundfiles.begin());
	}
	
}


OPENMPT_NAMESPACE_END
