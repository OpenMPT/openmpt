/*
 * AutoSaver.h
 * -----------
 * Purpose: Class for automatically saving open modules at a specified interval.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

class CAutoSaver
{
public:
//Cons/Destr
	CAutoSaver();
	
//Work
	bool DoSave(DWORD curTime);

//Member access
	bool IsEnabled() const;
	bool GetUseOriginalPath() const;
	mpt::PathString GetPath() const;
	uint32 GetHistoryDepth() const;
	uint32 GetSaveInterval() const;
	uint32 GetSaveIntervalMilliseconds() const
	{
		return Clamp(GetSaveInterval(), 0u, (1u<<30)/60u/1000u) * 60 * 1000;
	}

//internal implementation
private: 
	bool SaveSingleFile(CModDoc &modDoc);
	mpt::PathString BuildFileName(CModDoc &modDoc);
	void CleanUpBackups(const CModDoc &modDoc);
	bool CheckTimer(DWORD curTime);
	
//internal implementation members
private:

	//Flag to prevent autosave from starting new saving if previous is still in progress.
	bool m_bSaveInProgress; 

	DWORD m_nLastSave;

};


OPENMPT_NAMESPACE_END
