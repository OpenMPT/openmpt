/*
 * AutoSaver.h
 * -----------
 * Purpose: Class for automatically saving open modules at a specified interval.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

class CAutoSaver
{
public:
	CAutoSaver();
	
	bool DoSave(DWORD curTime);

	bool IsEnabled() const;
	bool GetUseOriginalPath() const;
	mpt::PathString GetPath() const;
	uint32 GetHistoryDepth() const;
	uint32 GetSaveInterval() const;
	uint32 GetSaveIntervalMilliseconds() const
	{
		return Clamp(GetSaveInterval(), 0u, (1u << 30) / 60u / 1000u) * 60 * 1000;
	}

private:
	bool SaveSingleFile(CModDoc &modDoc);
	mpt::PathString GetBasePath(const CModDoc &modDoc, bool createPath) const;
	mpt::PathString GetBaseName(const CModDoc &modDoc) const;
	mpt::PathString BuildFileName(const CModDoc &modDoc) const;
	void CleanUpBackups(const CModDoc &modDoc) const;
	bool CheckTimer(DWORD curTime) const;
	
	DWORD m_lastSave = 0;
	//Flag to prevent autosave from starting new saving if previous is still in progress.
	bool m_saveInProgress = false;

};


OPENMPT_NAMESPACE_END
