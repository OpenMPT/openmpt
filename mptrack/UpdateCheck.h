/*
 * UpdateCheck.h
 * -------------
 * Purpose: Class for easy software update check.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "mpt/uuid/uuid.hpp"

#include "../common/mptTime.h"

#include <atomic>

#include "Settings.h"

OPENMPT_NAMESPACE_BEGIN


#if defined(MPT_ENABLE_UPDATE)


namespace HTTP {
class InternetSession;
}

enum UpdateChannel : uint32
{
	UpdateChannelRelease = 1,
	UpdateChannelNext = 2,
	UpdateChannelDevelopment = 3,
};

struct UpdateCheckResult
{
	mpt::Date::Unix CheckTime = mpt::Date::Unix{};
	std::vector<std::byte> json;
	bool IsFromCache() const noexcept
	{
		return CheckTime == mpt::Date::Unix{};
	}
};

class CUpdateCheck
{

private:

	static std::atomic<int32> s_InstanceCount;

public:

	static mpt::ustring GetStatisticsUserInformation(bool shortText);

	static std::vector<mpt::ustring> GetDefaultUpdateSigningKeysRootAnchors();
	static mpt::ustring GetDefaultAPIURL();
	
	static int32 GetNumCurrentRunningInstances();

	static bool IsSuitableUpdateMoment();

	static void DoAutoUpdateCheck() { StartUpdateCheckAsync(true); }
	static void DoManualUpdateCheck() { StartUpdateCheckAsync(false); }

	static void WaitForUpdateCheckFinished();

public:

	struct Context
	{
		CWnd *window;
		UINT msgStart;
		UINT msgProgress;
		UINT msgSuccess;
		UINT msgFailure;
		UINT msgCanceled;
		bool autoUpdate;
		bool loadPersisted;
		std::string statistics;
	};

	struct Settings
	{
		Version previousVersion;
		int32 periodDays;
		UpdateChannel channel;
		mpt::PathString persistencePath;
		mpt::ustring apiURL;
		bool sendStatistics;
		mpt::UUID statisticsUUID;
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

	class Cancel
		: public std::exception
	{
	public:
		Cancel();
	};

	static bool IsAutoUpdateFromMessage(WPARAM wparam, LPARAM lparam);

	static const UpdateCheckResult &MessageAsResult(WPARAM wparam, LPARAM lparam);
	static const CUpdateCheck::Error &MessageAsError(WPARAM wparam, LPARAM lparam);

	static void AcknowledgeSuccess(const UpdateCheckResult &result);

	static void ShowSuccessGUI(const bool autoUpdate, const UpdateCheckResult &result);
	static void ShowFailureGUI(const bool autoUpdate, const CUpdateCheck::Error &error);

public:

	// v3
	static std::string GetStatisticsDataV3(const Settings &settings);  // UTF8

	static mpt::PathString GetUpdateTempDirectory(bool portable);

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

	static UpdateCheckResult SearchUpdate(const CUpdateCheck::Context &context, const CUpdateCheck::Settings &settings, const std::string &statistics); // may throw

	static void CleanOldUpdates(mpt::PathString dirTemp);

	static void CleanOldUpdates(const CUpdateCheck::Settings &settings, const CUpdateCheck::Context &context);

	static void SendStatistics(HTTP::InternetSession &internet, const CUpdateCheck::Settings &settings, const std::string &statistics); // may throw

	static UpdateCheckResult SearchUpdateModern(HTTP::InternetSession &internet, const CUpdateCheck::Settings &settings); // may throw

};


class CUpdateSetupDlg: public CPropertyPage
	, public ISettingChanged
{
public:
	CUpdateSetupDlg();

public:
	void SettingChanged(const SettingPath &changedPath) override;

protected:
	void DoDataExchange(CDataExchange *pDX) override;
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;

	afx_msg void OnSettingsChanged();
	afx_msg void OnCheckNow();
	afx_msg void OnShowStatisticsData(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/);
	void EnableDisableDialog();
	DECLARE_MESSAGE_MAP()

private:
	SettingChangedNotifyGuard m_SettingChangedNotifyGuard;
	CComboBox m_CbnUpdateFrequency;
};


#endif // MPT_ENABLE_UPDATE


OPENMPT_NAMESPACE_END
