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
			   bool useOriginalPath=true, mpt::PathString path=mpt::PathString(), mpt::PathString fileNameTemplate=mpt::PathString());
	~CAutoSaver();
	
//Work
	bool DoSave(DWORD curTime);

//Member access
	void Enable();
	void Disable();
	void SetEnabled(bool enabled) { if(enabled) Enable(); else Disable(); }
	bool IsEnabled();
	void SetUseOriginalPath(bool useOrgPath);
	bool GetUseOriginalPath();
	void SetPath(mpt::PathString path);
	mpt::PathString GetPath();
	void SetFilenameTemplate(mpt::PathString path);
	mpt::PathString GetFilenameTemplate();
	void SetHistoryDepth(int);
	int GetHistoryDepth();
	void SetSaveInterval(int minutes);
	int GetSaveInterval();

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
	mpt::PathString m_csFileNameTemplate;

};


// CAutoSaverGUI dialog

class CAutoSaverGUI : public CPropertyPage
{
	DECLARE_DYNAMIC(CAutoSaverGUI)

public:
	CAutoSaverGUI(CAutoSaver* pAutoSaver);
	virtual ~CAutoSaverGUI();

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
