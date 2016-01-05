/*
 * UpdateCheck.h
 * -------------
 * Purpose: Class for easy software update check.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <WinInet.h>
#include <time.h>

#include "../common/mptAtomic.h"
#include "resource.h"
#include "Settings.h"

OPENMPT_NAMESPACE_BEGIN

#define DOWNLOAD_BUFFER_SIZE 4096

//================
class CUpdateCheck
//================
{

private:

	static mpt::atomic_int32_t s_InstanceCount;

public:

	static const TCHAR *const defaultUpdateURL;

	int32 GetNumCurrentRunningInstances();

	static void DoAutoUpdateCheck() { StartUpdateCheckAsync(true); }
	static void DoManualUpdateCheck() { StartUpdateCheckAsync(false); }

public:

	struct Settings
	{
		CWnd *window;
		UINT msgProgress;
		UINT msgSuccess;
		UINT msgFailure;
		bool autoUpdate;
		CString updateBaseURL;  // URL where the version check should be made.
		CString guidString;     // Send GUID to collect basic stats or "anonymous"
		bool suggestDifferentBuilds;
	};

	class Error
		: public std::runtime_error
	{
	public:
		Error(CString errorMessage);
		Error(CString errorMessage, DWORD errorCode);
	protected:
		static CString FormatErrorCode(CString errorMessage, DWORD errorCode);
	};

	struct Result
	{
		time_t CheckTime;
		bool UpdateAvailable;
		CString Version;
		CString Date;
		CString URL;
		Result()
			: CheckTime(time_t())
			, UpdateAvailable(false)
		{
			return;
		}
	};

	static CUpdateCheck::Result ResultFromMessage(WPARAM wparam, LPARAM lparam);
	static CUpdateCheck::Error ErrorFromMessage(WPARAM wparam, LPARAM lparam);

	static void ShowSuccessGUI(WPARAM wparam, LPARAM lparam);
	static void ShowFailureGUI(WPARAM wparam, LPARAM lparam);

protected:

	static void StartUpdateCheckAsync(bool autoUpdate);

	struct ThreadFunc
	{
		CUpdateCheck::Settings settings;
		ThreadFunc(const CUpdateCheck::Settings &settings);
		void operator () ();
	};

	// Runtime resource handles
	HINTERNET internetHandle, connectionHandle, requestHandle;

	CUpdateCheck();
	
	void CheckForUpdate(const CUpdateCheck::Settings &settings);

	CUpdateCheck::Result SearchUpdate(const CUpdateCheck::Settings &settings); // may throw
	
	~CUpdateCheck();

};


//=========================================
class CUpdateSetupDlg: public CPropertyPage
//=========================================
	, public ISettingChanged
{
public:
	CUpdateSetupDlg();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnCheckNow();
	afx_msg void OnResetURL();
	virtual void SettingChanged(const SettingPath &changedPath);
	DECLARE_MESSAGE_MAP()

private:
	SettingChangedNotifyGuard m_SettingChangedNotifyGuard;
};


OPENMPT_NAMESPACE_END
