/*
 * AutoSaver.h
 * -----------
 * Purpose: Class for automatically saving open modules at a specified interval.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "resource.h"

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

class CAutoSaver
{
public:
//Cons/Destr
	CAutoSaver(bool enabled = true, uint32_t saveInterval = 10, uint32_t backupHistory = 3,
			   bool useOriginalPath = true, mpt::PathString path = mpt::PathString());
	
//Work
	bool DoSave(DWORD curTime);

//Member access
	void SetEnabled(bool enabled) { m_bEnabled = enabled; }
	bool IsEnabled() const { return m_bEnabled; }
	void SetUseOriginalPath(bool useOrgPath) { m_bUseOriginalPath = useOrgPath; }
	bool GetUseOriginalPath() const { return m_bUseOriginalPath; }
	void SetPath(const mpt::PathString &path) { m_csPath = path; }
	mpt::PathString GetPath() const { return m_csPath; }
	void SetHistoryDepth(uint32_t history) { m_nBackupHistory = Clamp(history, 1u, 1000u); }
	uint32_t GetHistoryDepth() const { return m_nBackupHistory; }
	void SetSaveInterval(uint32_t minutes)
	{
		m_nSaveInterval = Clamp(minutes, 1u, 10000u) * 60u * 1000u; //minutes to milliseconds
	}
	uint32_t GetSaveInterval() const { return m_nSaveInterval / 60u / 1000u; }

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

	bool m_bEnabled;
	DWORD m_nLastSave;
	uint32_t m_nSaveInterval;
	uint32_t m_nBackupHistory;
	bool m_bUseOriginalPath;
	mpt::PathString m_csPath;

};


OPENMPT_NAMESPACE_END
