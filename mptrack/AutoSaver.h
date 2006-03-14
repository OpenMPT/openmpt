#pragma once
#include "resource.h"

class CModDoc;

class CAutoSaver
{
public:
//Cons/Destr
	CAutoSaver(void);
	CAutoSaver(bool enabled, int saveInterval, int backupHistory,
			   bool useOriginalPath, CString path, CString fileNameTemplate);
	~CAutoSaver(void);
	
//Work
	bool DoSave(DWORD curTime);

//Member access
	void Enable();
	void Disable();
	bool IsEnabled();
	void SetUseOriginalPath(bool useOrgPath);
	bool GetUseOriginalPath();
	void SetPath(CString path);
	CString GetPath();
	void SetFilenameTemplate(CString path);
	CString GetFilenameTemplate();
	void SetHistoryDepth(int);
	int GetHistoryDepth();
	void SetSaveInterval(int minutes);
	int GetSaveInterval();

//internal implementation
private: 
	bool SaveSingleFile(CModDoc *pModDoc);
	CString BuildFileName(CModDoc *pModDoc);
	void CleanUpBackups(CModDoc *pModDoc);
	bool CheckTimer(DWORD curTime);
	
//internal implementation members
private:
	bool m_bEnabled;
	DWORD m_nLastSave;
	DWORD m_nSaveInterval;
	int m_nBackupHistory;
	bool m_bUseOriginalPath;
	CString m_csPath;
	CString m_csFileNameTemplate;

};
#pragma once


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
