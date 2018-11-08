/*
 * UpdateCheck.h
 * -------------
 * Purpose: Class for easy software update check.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "../common/mptUUID.h"

#include <time.h>

#include <atomic>

#include "resource.h"
#include "Settings.h"

OPENMPT_NAMESPACE_BEGIN


class CUpdateCheck
{

private:

	static std::atomic<int32> s_InstanceCount;

public:

	static mpt::ustring GetDefaultUpdateURL();
	
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
		mpt::ustring updateBaseURL;  // URL where the version check should be made.
		bool sendStatistics;
		mpt::UUID statisticsUUID;
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

	static void CheckForUpdate(const CUpdateCheck::Settings &settings);

	static CUpdateCheck::Result SearchUpdate(const CUpdateCheck::Settings &settings); // may throw
	
};


class CUpdateSetupDlg: public CPropertyPage
	, public ISettingChanged
{
public:
	CUpdateSetupDlg();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;

	void SettingChanged(const SettingPath &changedPath) override;

	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnCheckNow();
	afx_msg void OnResetURL();
	DECLARE_MESSAGE_MAP()

private:
	SettingChangedNotifyGuard m_SettingChangedNotifyGuard;
};


OPENMPT_NAMESPACE_END
