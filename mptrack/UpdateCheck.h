/*
 * UpdateCheck.h
 * -------------
 * Purpose: Header for easy software update check.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#pragma once
#ifndef UPDATECHECK_H
#define UPDATECHECK_H

#include <WinInet.h>
#include <time.h>

// Setup dialog stuff
#include "Mainfrm.h"
#include "resource.h"
#include "Moptions.h"


#define DOWNLOAD_BUFFER_SIZE 256

//================
class CUpdateCheck
//================
{
public:

	static const CString defaultUpdateURL;

	// Force creation via "new" as we're using "delete this".
	static CUpdateCheck *Create(bool autoUpdate) { return new CUpdateCheck(autoUpdate); };
	void DoUpdateCheck();

	static time_t GetLastUpdateCheck() { return lastUpdateCheck; };
	static int GetUpdateCheckPeriod() { return updateCheckPeriod; };
	static CString GetUpdateURL() { return updateBaseURL; };
	static bool GetSendGUID() { return sendGUID; }
	static bool GetShowUpdateHint() { return showUpdateHint; };
	static void SetUpdateSettings(time_t last, int period, CString url, bool sendID, bool showHint)
		{ lastUpdateCheck = last; updateCheckPeriod = period; updateBaseURL = url; sendGUID = sendID; showUpdateHint = showHint; };

protected:

	// Static configuration variables
	static time_t lastUpdateCheck;	// Time of last successful update check
	static int updateCheckPeriod;	// Check for updates every x days
	static CString updateBaseURL;	// URL where the version check should be made.
	static bool sendGUID;			// Send GUID to collect basic stats
	static bool showUpdateHint;		// Show hint on first automatic update

	bool isAutoUpdate;	// Are we running an automatic update check?

	// Runtime resource handles
	HANDLE threadHandle;
	HINTERNET internetHandle, connectionHandle;

	CUpdateCheck(const bool showErrors);
	~CUpdateCheck();

	static DWORD WINAPI UpdateThread(LPVOID param);
	void Die(CString errorMessage);
	void Die(CString errorMessage, DWORD errorCode);
	void Terminate();
};


//=========================================
class CUpdateSetupDlg: public CPropertyPage
//=========================================
{
public:
	CUpdateSetupDlg():CPropertyPage(IDD_OPTIONS_UPDATE)
	{ };

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnCheckNow();
	afx_msg void OnResetURL();
	DECLARE_MESSAGE_MAP()
};

#endif // UPDATECHECK_H

