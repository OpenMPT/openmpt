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

#include <time.h>

#include <atomic>

#include "resource.h"
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

class CUpdateCheck
{

private:

	static std::atomic<int32> s_InstanceCount;

public:

	static mpt::ustring GetStatisticsUserInformation(bool shortText);

#if MPT_UPDATE_LEGACY
	static mpt::ustring GetDefaultChannelReleaseURL();
	static mpt::ustring GetDefaultChannelNextURL();
	static mpt::ustring GetDefaultChannelDevelopmentURL();
#endif // MPT_UPDATE_LEGACY

	static std::vector<mpt::ustring> GetDefaultUpdateSigningKeysRootAnchors();
	static mpt::ustring GetDefaultAPIURL();
	
	int32 GetNumCurrentRunningInstances();

	static bool IsSuitableUpdateMoment();

	static void DoAutoUpdateCheck() { StartUpdateCheckAsync(true); }
	static void DoManualUpdateCheck() { StartUpdateCheckAsync(false); }

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
		int32 periodDays;
		UpdateChannel channel;
		mpt::PathString persistencePath;
#if MPT_UPDATE_LEGACY
		bool modeLegacy;
		mpt::ustring channelReleaseURL;
		mpt::ustring channelNextURL;
		mpt::ustring channelDevelopmentURL;
#endif // MPT_UPDATE_LEGACY
		mpt::ustring apiURL;
		bool sendStatistics;
		mpt::UUID statisticsUUID;
		Settings();
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

	class Cancel
		: public std::exception
	{
	public:
		Cancel();
	};

	struct Result
	{
		time_t CheckTime = time_t{};
		std::vector<std::byte> json;
#if MPT_UPDATE_LEGACY
		bool UpdateAvailable = false;
		CString Version;
		CString Date;
		CString URL;
#endif // MPT_UPDATE_LEGACY
	};

	static bool IsAutoUpdateFromMessage(WPARAM wparam, LPARAM lparam);

	static CUpdateCheck::Result ResultFromMessage(WPARAM wparam, LPARAM lparam);
	static CUpdateCheck::Error ErrorFromMessage(WPARAM wparam, LPARAM lparam);

	static void AcknowledgeSuccess(WPARAM wparam, LPARAM lparam);

	static void ShowSuccessGUI(WPARAM wparam, LPARAM lparam);
	static void ShowFailureGUI(WPARAM wparam, LPARAM lparam);

	static mpt::ustring GetFailureMessage(WPARAM wparam, LPARAM lparam);

public:

#if MPT_UPDATE_LEGACY
	// v2
	static mpt::ustring GetUpdateURLV2(const Settings &settings);
#endif // MPT_UPDATE_LEGACY

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

	static CUpdateCheck::Result SearchUpdate(const CUpdateCheck::Context &context, const CUpdateCheck::Settings &settings, const std::string &statistics); // may throw

	static void CleanOldUpdates(const CUpdateCheck::Settings &settings, const CUpdateCheck::Context &context);

	static void SendStatistics(HTTP::InternetSession &internet, const CUpdateCheck::Settings &settings, const std::string &statistics); // may throw

#if MPT_UPDATE_LEGACY
	static CUpdateCheck::Result SearchUpdateLegacy(HTTP::InternetSession &internet, const CUpdateCheck::Settings &settings); // may throw
#endif // MPT_UPDATE_LEGACY
	static CUpdateCheck::Result SearchUpdateModern(HTTP::InternetSession &internet, const CUpdateCheck::Settings &settings); // may throw

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
