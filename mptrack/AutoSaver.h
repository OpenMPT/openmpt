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
	CAutoSaver(bool enabled=true, int saveInterval=10, int backupHistory=3,
			   bool useOriginalPath=true, mpt::PathString path=mpt::PathString());
	
//Work
	bool DoSave(DWORD curTime);

//Member access
	void SetEnabled(bool enabled) { m_bEnabled = enabled; }
	bool IsEnabled() const { return m_bEnabled; }
	void SetUseOriginalPath(bool useOrgPath) { m_bUseOriginalPath = useOrgPath; }
	bool GetUseOriginalPath() const { return m_bUseOriginalPath; }
	void SetPath(const mpt::PathString &path) { m_csPath = path; }
	mpt::PathString GetPath() const { return m_csPath; }
	void SetHistoryDepth(int history) { m_nBackupHistory = Clamp(history, 1, 1000); }
	int GetHistoryDepth() const { return m_nBackupHistory; }
	void SetSaveInterval(int minutes)
	{
		m_nSaveInterval = Clamp(minutes, 1, 10000) * 60 * 1000; //minutes to milliseconds
	}
	int GetSaveInterval() const { return m_nSaveInterval / 60 / 1000; }

//internal implementation
private: 
	bool SaveSingleFile(CModDoc &modDoc);
	mpt::PathString BuildFileName(CModDoc &modDoc);
	void CleanUpBackups(CModDoc &modDoc);
	bool CheckTimer(DWORD curTime);
	
//internal implementation members
private:

	//Flag to prevent autosave from starting new saving if previous is still in progress.
	bool m_bSaveInProgress; 

	bool m_bEnabled;
	DWORD m_nLastSave;
	DWORD m_nSaveInterval;
	size_t m_nBackupHistory;
	bool m_bUseOriginalPath;
	mpt::PathString m_csPath;

};


// CAutoSaverGUI dialog

class CAutoSaverGUI : public CPropertyPage
{
	DECLARE_DYNAMIC(CAutoSaverGUI)

public:
	CAutoSaverGUI(CAutoSaver* pAutoSaver);

// Dialog Data
	enum { IDD = IDD_OPTIONS_AUTOSAVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual void OnOK();
	virtual BOOL OnKillActive();

private:
	CAutoSaver *m_pAutoSaver;

public:
	afx_msg void OnBnClickedAutosaveBrowse();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedAutosaveEnable();
	afx_msg void OnBnClickedAutosaveUseorigdir();
	void OnSettingsChanged();
	BOOL OnSetActive();
	
};


OPENMPT_NAMESPACE_END
