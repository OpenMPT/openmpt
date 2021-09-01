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


enum UpdateChannel
{
	UpdateChannelRelease = 1,
	UpdateChannelNext = 2,
	UpdateChannelDevelopment = 3,
};

class CUpdateCheck
{

private:

	static std::atomic<int32> s_InstanceCount;

public:

	static mpt::ustring GetStatisticsUserInformation(bool shortText);

	static mpt::ustring GetDefaultChannelReleaseURL();
	static mpt::ustring GetDefaultChannelNextURL();
	static mpt::ustring GetDefaultChannelDevelopmentURL();

	static mpt::ustring GetDefaultAPIURL();
	
	int32 GetNumCurrentRunningInstances();

	static void DoAutoUpdateCheck() { StartUpdateCheckAsync(true); }
	static void DoManualUpdateCheck() { StartUpdateCheckAsync(false); }

public:

	struct Context
	{
		CWnd *window;
		UINT msgProgress;
		UINT msgSuccess;
		UINT msgFailure;
		bool autoUpdate;
		std::string statistics;
	};

	struct Settings
	{
		int32 periodDays;
		uint32 channel;
		mpt::ustring channelReleaseURL;
		mpt::ustring channelNextURL;
		mpt::ustring channelDevelopmentURL;
		mpt::ustring apiURL;
		bool sendStatistics;
		mpt::UUID statisticsUUID;
		bool suggestDifferentBuilds;
		Settings();
	};

	class Error
		: public std::runtime_error
	{
	private:
		CString m_Message;
	public:
		Error(CString errorMessage);
		Error(CString errorMessage, DWORD errorCode);
		CString GetMessage() const;
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

public:

	// v2
	static mpt::ustring GetUpdateURLV2(const Settings &settings);

	// v3
	static std::string GetStatisticsDataV3(const Settings &settings);  // UTF8

protected:

	static void StartUpdateCheckAsync(bool autoUpdate);

	struct ThreadFunc
	{
		CUpdateCheck::Settings settings;
		CUpdateCheck::Context context;
		ThreadFunc(const CUpdateCheck::Settings &settings, const CUpdateCheck::Context &context);
		void operator () ();
	};

	static void CheckForUpdate(const CUpdateCheck::Settings &settings, const CUpdateCheck::Context &context);

	static CUpdateCheck::Result SearchUpdate(const CUpdateCheck::Settings &settings, const std::string &statistics); // may throw
	
};


class CUpdateSetupDlg: public CPropertyPage
	, public ISettingChanged
{
public:
	CUpdateSetupDlg();

protected:
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;

	void SettingChanged(const SettingPath &changedPath) override;

	afx_msg void OnSettingsChanged();
	afx_msg void OnCheckNow();
	afx_msg void OnShowStatisticsData(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/);
	void EnableDisableDialog();
	DECLARE_MESSAGE_MAP()

private:
	SettingChangedNotifyGuard m_SettingChangedNotifyGuard;
	CComboBox m_CbnUpdateFrequency;
};


OPENMPT_NAMESPACE_END
