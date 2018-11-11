/*
 * UpdateCheck.cpp
 * ---------------
 * Purpose: Class for easy software update check.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "UpdateCheck.h"
#include "BuildVariants.h"
#include "../common/version.h"
#include "../common/misc_util.h"
#include "../common/mptStringBuffer.h"
#include "Mptrack.h"
#include "TrackerSettings.h"
// Setup dialog stuff
#include "Mainfrm.h"
#include "../common/mptThread.h"
#include "HTTP.h"
#include "../misc/JSON.h"
#include "dlg_misc.h"


OPENMPT_NAMESPACE_BEGIN

// Update notification dialog
class UpdateDialog : public CDialog
{
protected:
	const CString &m_releaseVersion;
	const CString &m_releaseDate;
	const CString &m_releaseURL;
	CFont m_boldFont;
public:
	bool ignoreVersion;

public:
	UpdateDialog(const CString &releaseVersion, const CString &releaseDate, const CString &releaseURL)
		: CDialog(IDD_UPDATE)
		, m_releaseVersion(releaseVersion)
		, m_releaseDate(releaseDate)
		, m_releaseURL(releaseURL)
		, ignoreVersion(false)
	{ }

	BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();

		CFont *font = GetDlgItem(IDC_VERSION2)->GetFont();
		LOGFONT lf;
		font->GetLogFont(&lf);
		lf.lfWeight = FW_BOLD;
		m_boldFont.CreateFontIndirect(&lf);
		GetDlgItem(IDC_VERSION2)->SetFont(&m_boldFont);

		SetDlgItemText(IDC_VERSION1, mpt::cfmt::val(Version::Current()));
		SetDlgItemText(IDC_VERSION2, m_releaseVersion);
		SetDlgItemText(IDC_DATE, m_releaseDate);
		SetDlgItemText(IDC_SYSLINK1, _T("More information about this build:\n<a href=\"") + m_releaseURL + _T("\">") + m_releaseURL + _T("</a>"));
		CheckDlgButton(IDC_CHECK1, (TrackerSettings::Instance().UpdateIgnoreVersion == m_releaseVersion) ? BST_CHECKED : BST_UNCHECKED);
		return FALSE;
	}

	void OnClose()
	{
		m_boldFont.DeleteObject();
		CDialog::OnClose();
		TrackerSettings::Instance().UpdateIgnoreVersion = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED ? m_releaseVersion : CString(_T(""));
	}

	void OnClickURL(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/)
	{
		CTrackApp::OpenURL(m_releaseURL);
	}

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(UpdateDialog, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK1, &UpdateDialog::OnClickURL)
END_MESSAGE_MAP()



mpt::ustring CUpdateCheck::GetStatisticsUserInformation(bool shortText)
{
	if(shortText)
	{
		return MPT_USTRING("A randomized user ID is created and transmitted alongside. This ID can only be linked to you or your computer by this very ID, which is stored solely on your computer. OpenMPT will use this information to gather usage statistics and to plan system support for future OpenMPT versions.");
	} else
	{
		return MPT_USTRING("")
			+ MPT_USTRING("When checking for updates, OpenMPT can additionally collect some basic statistical information.") + MPT_USTRING(" ")
			+ MPT_USTRING("A randomized user ID is created and transmitted alongside the update check. This ID can only be linked to you or your computer by this very ID, which is stored solely on your computer.") + MPT_USTRING(" ")
			+ MPT_USTRING("OpenMPT will use this information to gather usage statistics and to plan system support for future OpenMPT versions.") + MPT_USTRING(" ")
			+ MPT_USTRING("Without this statistical information, the OpenMPT developers would be blind with respect to what systems are used to run OpenMPT. This makes deciding where to focus development plain guesswork.") + MPT_USTRING("\n")
			+ MPT_USTRING("OpenMPT would collect the following statistical data points: OpenMPT version, Windows version, type of CPU, amount of RAM, configured update check frequency of OpenMPT.")
			;
	}
}


mpt::ustring CUpdateCheck::GetDefaultChannelReleaseURL()
{
	return MPT_USTRING("https://update.openmpt.org/check/$VERSION/$GUID");
}

mpt::ustring CUpdateCheck::GetDefaultChannelNextURL()
{
	return MPT_USTRING("https://update.openmpt.org/check/next/$VERSION/$GUID");
}

mpt::ustring CUpdateCheck::GetDefaultChannelDevelopmentURL()
{
	return MPT_USTRING("https://update.openmpt.org/check/testing/$VERSION/$GUID");
}


mpt::ustring CUpdateCheck::GetDefaultAPIURL()
{
	return MPT_USTRING("https://update.openmpt.org/api/v3/");
}


std::atomic<int32> CUpdateCheck::s_InstanceCount(0);


int32 CUpdateCheck::GetNumCurrentRunningInstances()
{
	return s_InstanceCount.load();
}


// Start update check
void CUpdateCheck::StartUpdateCheckAsync(bool isAutoUpdate)
{
	if(isAutoUpdate)
	{
		if(!TrackerSettings::Instance().UpdateEnabled)
		{
			return;
		}
		int updateCheckPeriod = TrackerSettings::Instance().UpdateIntervalDays;
		if(updateCheckPeriod < 0)
		{
			return;
		}
		// Do we actually need to run the update check right now?
		const time_t now = time(nullptr);
		const time_t lastCheck = TrackerSettings::Instance().UpdateLastUpdateCheck.Get();
		// Check update interval. Note that we always check for updates when the system time had gone backwards (i.e. when the last update check supposedly happened in the future).
		if(difftime(now, lastCheck) > 0.0)
		{
			if(difftime(now, lastCheck) < static_cast<double>(updateCheckPeriod * 86400))
			{
				return;
			}
		}

		// Never ran update checks before, so we notify the user of automatic update checks.
		if(TrackerSettings::Instance().UpdateShowUpdateHint)
		{
			TrackerSettings::Instance().UpdateShowUpdateHint = false;
			CString msg;
			msg.Format(_T("OpenMPT would like to check for updates now, proceed?\n\nNote: In the future, OpenMPT will check for updates every %u days. If you do not want this, you can disable update checks in the setup."), TrackerSettings::Instance().UpdateIntervalDays.Get());
			if(Reporting::Confirm(msg, _T("OpenMPT Internet Update")) == cnfNo)
			{
				TrackerSettings::Instance().UpdateLastUpdateCheck = mpt::Date::Unix(now);
				return;
			}
		}
	} else
	{
		if(!TrackerSettings::Instance().UpdateEnabled)
		{
			if(Reporting::Confirm(_T("Update Check is disabled. Do you want to check anyway?"), _T("OpenMPT Internet Update")) != cnfYes)
			{
				return;
			}
		}
	}
	TrackerSettings::Instance().UpdateShowUpdateHint = false;

	// ask if user wants to contribute system statistics
	if(!TrackerSettings::Instance().UpdateStatisticsConsentAsked)
	{
		TrackerSettings::Instance().UpdateStatistics = (ConfirmAnswer::cnfYes == Reporting::Confirm(
			MPT_USTRING("Do you want to contribute to OpenMPT by providing system statistics?\r\n") +
			MPT_USTRING("\r\n") +
			mpt::String::Replace(CUpdateCheck::GetStatisticsUserInformation(false), MPT_USTRING("\n"), MPT_USTRING("\r\n")) + MPT_USTRING("\r\n") +
			MPT_USTRING("\r\n") +
			mpt::format(MPT_USTRING("This option was previously %1 on your system.\r\n"))(TrackerSettings::Instance().UpdateStatistics ? MPT_USTRING("enabled") : MPT_USTRING("disabled")),
			false, !TrackerSettings::Instance().UpdateStatistics.Get()));
		TrackerSettings::Instance().UpdateStatisticsConsentAsked = true;
	}

	int32 expected = 0;
	if(!s_InstanceCount.compare_exchange_strong(expected, 1))
	{
		return;
	}

	CUpdateCheck::Context context;
	context.window = CMainFrame::GetMainFrame();
	context.msgProgress = MPT_WM_APP_UPDATECHECK_PROGRESS;
	context.msgSuccess = MPT_WM_APP_UPDATECHECK_SUCCESS;
	context.msgFailure = MPT_WM_APP_UPDATECHECK_FAILURE;
	context.autoUpdate = isAutoUpdate;
	std::thread(CUpdateCheck::ThreadFunc(CUpdateCheck::Settings(), context)).detach();
}


CUpdateCheck::Settings::Settings()
	: periodDays(TrackerSettings::Instance().UpdateIntervalDays)
	, channel(TrackerSettings::Instance().UpdateChannel)
	, channelReleaseURL(TrackerSettings::Instance().UpdateChannelReleaseURL)
	, channelNextURL(TrackerSettings::Instance().UpdateChannelNextURL)
	, channelDevelopmentURL(TrackerSettings::Instance().UpdateChannelDevelopmentURL)
	, apiURL(TrackerSettings::Instance().UpdateAPIURL)
	, sendStatistics(TrackerSettings::Instance().UpdateStatistics)
	, statisticsUUID(TrackerSettings::Instance().VersionInstallGUID)
	, suggestDifferentBuilds(TrackerSettings::Instance().UpdateSuggestDifferentBuildVariant)
{
}


CUpdateCheck::ThreadFunc::ThreadFunc(const CUpdateCheck::Settings &settings, const CUpdateCheck::Context &context)
	: settings(settings)
	, context(context)
{
	return;
}


void CUpdateCheck::ThreadFunc::operator () ()
{
	mpt::SetCurrentThreadPriority(context.autoUpdate ? mpt::ThreadPriorityLower : mpt::ThreadPriorityNormal);
	CUpdateCheck::CheckForUpdate(settings, context);
}


std::string CUpdateCheck::GetStatisticsDataV3(const Settings &settings)
{
	JSON::value j;
	j["OpenMPT"]["Version"] = mpt::ufmt::val(Version::Current());
	j["OpenMPT"]["BuildVariant"] = BuildVariants().GuessCurrentBuildName();
	j["OpenMPT"]["Architecture"] = mpt::Windows::Name(mpt::Windows::GetProcessArchitecture());
	j["Update"]["PeriodDays"] = settings.periodDays;
	j["System"]["Windows"]["Version"]["Name"] = mpt::Windows::Version::Current().GetName();
	j["System"]["Windows"]["Version"]["Major"] = mpt::Windows::Version::Current().GetSystem().Major;
	j["System"]["Windows"]["Version"]["Minor"] = mpt::Windows::Version::Current().GetSystem().Minor;
	j["System"]["Windows"]["ServicePack"]["Major"] = mpt::Windows::Version::Current().GetServicePack().Major;
	j["System"]["Windows"]["ServicePack"]["Minor"] = mpt::Windows::Version::Current().GetServicePack().Minor;
	j["System"]["Windows"]["Build"] = mpt::Windows::Version::Current().GetBuild();
	j["System"]["Windows"]["Architecture"] = mpt::Windows::Name(mpt::Windows::GetHostArchitecture());
	j["System"]["Windows"]["IsWine"] = mpt::Windows::IsWine();
	std::vector<mpt::Windows::Architecture> architectures = mpt::Windows::GetSupportedProcessArchitectures(mpt::Windows::GetHostArchitecture());
	for(const auto & arch : architectures)
	{
		j["System"]["Windows"]["ProcessArchitectures"][mpt::ToCharset(mpt::CharsetUTF8, mpt::Windows::Name(arch))] = true;
	}
	j["System"]["Memory"] = mpt::Windows::GetSystemMemorySize() / 1024 / 1024;  // MB
	if(mpt::Windows::IsWine())
	{
		mpt::Wine::VersionContext v;
		j["System"]["Windows"]["Wine"]["Version"]["Raw"] = v.RawVersion();
		if(v.Version().IsValid())
		{
			j["System"]["Windows"]["Wine"]["Version"]["Major"] = v.Version().GetMajor();
			j["System"]["Windows"]["Wine"]["Version"]["Minor"] = v.Version().GetMinor();
			j["System"]["Windows"]["Wine"]["Version"]["Update"] = v.Version().GetUpdate();
		}
		j["System"]["Windows"]["Wine"]["HostSysName"] = v.RawHostSysName();
	}
	#ifdef ENABLE_ASM
		j["System"]["Processor"]["Vendor"] = std::string(mpt::String::ReadAutoBuf(ProcVendorID));
		j["System"]["Processor"]["Brand"] = std::string(mpt::String::ReadAutoBuf(ProcBrandID));
		j["System"]["Processor"]["Id"]["Family"] = ProcFamily;
		j["System"]["Processor"]["Id"]["Model"] = ProcModel;
		j["System"]["Processor"]["Id"]["Stepping"] = ProcStepping;
		j["System"]["Processor"]["Features"]["lm"] = ((GetRealProcSupport() & PROCSUPPORT_LM) ? true : false);
		j["System"]["Processor"]["Features"]["cmov"] = ((GetRealProcSupport() & PROCSUPPORT_CMOV) ? true : false);
		j["System"]["Processor"]["Features"]["mmx"] = ((GetRealProcSupport() & PROCSUPPORT_MMX) ? true : false);
		j["System"]["Processor"]["Features"]["mmxext"] = ((GetRealProcSupport() & PROCSUPPORT_AMD_MMXEXT) ? true : false);
		j["System"]["Processor"]["Features"]["3dnow"] = ((GetRealProcSupport() & PROCSUPPORT_AMD_3DNOW) ? true : false);
		j["System"]["Processor"]["Features"]["3dnowext"] = ((GetRealProcSupport() & PROCSUPPORT_AMD_3DNOWEXT) ? true : false);
		j["System"]["Processor"]["Features"]["sse"] = ((GetRealProcSupport() & PROCSUPPORT_SSE) ? true : false);
		j["System"]["Processor"]["Features"]["sse2"] = ((GetRealProcSupport() & PROCSUPPORT_SSE2) ? true : false);
		j["System"]["Processor"]["Features"]["sse3"] = ((GetRealProcSupport() & PROCSUPPORT_SSE3) ? true : false);
		j["System"]["Processor"]["Features"]["ssse3"] = ((GetRealProcSupport() & PROCSUPPORT_SSSE3) ? true : false);
		j["System"]["Processor"]["Features"]["sse4_1"] = ((GetRealProcSupport() & PROCSUPPORT_SSE4_1) ? true : false);
		j["System"]["Processor"]["Features"]["sse4_2"] = ((GetRealProcSupport() & PROCSUPPORT_SSE4_2) ? true : false);
	#endif
	return j.dump(1, '\t');
}


mpt::ustring CUpdateCheck::GetUpdateURLV2(const CUpdateCheck::Settings &settings)
{
	mpt::ustring updateURL;
	if(settings.channel == UpdateChannelRelease)
	{
		updateURL = settings.channelReleaseURL;
		if(updateURL.empty())
		{
			updateURL = GetDefaultChannelReleaseURL();
		}
	}	else if(settings.channel == UpdateChannelNext)
	{
		updateURL = settings.channelNextURL;
		if(updateURL.empty())
		{
			updateURL = GetDefaultChannelNextURL();
		}
	}	else if(settings.channel == UpdateChannelDevelopment)
	{
		updateURL = settings.channelDevelopmentURL;
		if(updateURL.empty())
		{
			updateURL = GetDefaultChannelDevelopmentURL();
		}
	}	else
	{
		updateURL = settings.channelReleaseURL;
		if(updateURL.empty())
		{
			updateURL = GetDefaultChannelReleaseURL();
		}
	}
	if(updateURL.find(MPT_USTRING("://")) == mpt::ustring::npos)
	{
		updateURL = MPT_USTRING("https://") + updateURL;
	}
	// Build update URL
	updateURL = mpt::String::Replace(updateURL, MPT_USTRING("$VERSION"), mpt::uformat(MPT_USTRING("%1-%2-%3"))
		( Version::Current()
		, BuildVariants().GuessCurrentBuildName()
		, settings.sendStatistics ? mpt::Windows::Version::Current().GetNameShort() : MPT_USTRING("unknown")
		));
	updateURL = mpt::String::Replace(updateURL, MPT_USTRING("$GUID"), settings.sendStatistics ? mpt::ufmt::val(settings.statisticsUUID) : MPT_USTRING("anonymous"));
	return updateURL;
}


// Run update check (independent thread)
CUpdateCheck::Result CUpdateCheck::SearchUpdate(const CUpdateCheck::Settings &settings)
{
	
	HTTP::InternetSession internet(Version::Current().GetOpenMPTVersionString());

	// Establish a connection.
	HTTP::Request request;
	request.SetURI(ParseURI(GetUpdateURLV2(settings)));
	request.method = HTTP::Method::Get;
	request.flags = HTTP::NoCache;

	HTTP::Result resultHTTP = internet(request.InsecureTLSDowngradeWindowsXP());

	if(settings.sendStatistics)
	{
		HTTP::Request requestStatistics;
		if(settings.statisticsUUID.IsValid())
		{
			requestStatistics.SetURI(ParseURI(settings.apiURL + mpt::format(MPT_USTRING("statistics/%1"))(settings.statisticsUUID)));
			requestStatistics.method = HTTP::Method::Put;
		} else
		{
			requestStatistics.SetURI(ParseURI(settings.apiURL + MPT_USTRING("statistics/")));
			requestStatistics.method = HTTP::Method::Post;
		}
		requestStatistics.dataMimeType = HTTP::MimeType::JSON();
		requestStatistics.acceptMimeTypes = HTTP::MimeTypes::JSON();
		std::string jsondata = GetStatisticsDataV3(settings);
		MPT_LOG(LogInformation, "Update", mpt::ToUnicode(mpt::CharsetUTF8, jsondata));
		requestStatistics.data = mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(jsondata));
		internet(requestStatistics.InsecureTLSDowngradeWindowsXP());
	}

	// Retrieve HTTP status code.
	if(resultHTTP.Status >= 400)
	{
		throw CUpdateCheck::Error(mpt::cformat(_T("Version information could not be found on the server (HTTP status code %1). Maybe your version of OpenMPT is too old!"))(resultHTTP.Status));
	}

	// Now, evaluate the downloaded data.
	CUpdateCheck::Result result;
	result.UpdateAvailable = false;
	result.CheckTime = time(nullptr);
	CString resultData = mpt::ToCString(mpt::CharsetUTF8, resultHTTP.Data);
	if(resultData.CompareNoCase(_T("noupdate")) != 0)
	{
		CString token;
		int parseStep = 0, parsePos = 0;
		while((token = resultData.Tokenize(_T("\n"), parsePos)) != "")
		{
			token.Trim();
			switch(parseStep++)
			{
			case 0:
				if(token.CompareNoCase(_T("update")) != 0)
				{
					throw CUpdateCheck::Error(_T("Could not understand server response. Maybe your version of OpenMPT is too old!"));
				}
				break;
			case 1:
				result.Version = token;
				break;
			case 2:
				result.Date = token;
				break;
			case 3:
				result.URL = token;
				break;
			}
		}
		if(parseStep < 4)
		{
			throw CUpdateCheck::Error(_T("Could not understand server response. Maybe your version of OpenMPT is too old!"));
		}
		result.UpdateAvailable = true;
	}
	return result;
}


void CUpdateCheck::CheckForUpdate(const CUpdateCheck::Settings &settings, const CUpdateCheck::Context &context)
{
	// íncremented before starting the thread
	MPT_ASSERT(s_InstanceCount.load() >= 1);
	CUpdateCheck::Result result;
	context.window->SendMessage(context.msgProgress, context.autoUpdate ? 1 : 0, s_InstanceCount.load());
	try
	{
		try
		{
			result = SearchUpdate(settings);
		} catch(const bad_uri &e)
		{
			throw CUpdateCheck::Error(mpt::cformat(_T("Error parsing update URL: %1"))(mpt::get_exception_text<CString>(e)));
		} catch(const HTTP::exception &e)
		{
			throw CUpdateCheck::Error(CString(_T("HTTP error: ")) + mpt::ToCString(e.GetMessage()));
		}
	} catch(const CUpdateCheck::Error &e)
	{
		context.window->SendMessage(context.msgFailure, context.autoUpdate ? 1 : 0, reinterpret_cast<LPARAM>(&e));
		s_InstanceCount.fetch_sub(1);
		MPT_ASSERT(s_InstanceCount.load() >= 0);
		return;
	}
	context.window->SendMessage(context.msgSuccess, context.autoUpdate ? 1 : 0, reinterpret_cast<LPARAM>(&result));
	s_InstanceCount.fetch_sub(1);
	MPT_ASSERT(s_InstanceCount.load() >= 0);
}


CUpdateCheck::Result CUpdateCheck::ResultFromMessage(WPARAM /*wparam*/ , LPARAM lparam)
{
	const CUpdateCheck::Result &result = *reinterpret_cast<CUpdateCheck::Result*>(lparam);
	return result;
}


CUpdateCheck::Error CUpdateCheck::ErrorFromMessage(WPARAM /*wparam*/ , LPARAM lparam)
{
	const CUpdateCheck::Error &error = *reinterpret_cast<CUpdateCheck::Error*>(lparam);
	return error;
}


void CUpdateCheck::ShowSuccessGUI(WPARAM wparam, LPARAM lparam)
{
	const CUpdateCheck::Result &result = *reinterpret_cast<CUpdateCheck::Result*>(lparam);
	bool autoUpdate = wparam != 0;
	if(result.UpdateAvailable && (!autoUpdate || result.Version != TrackerSettings::Instance().UpdateIgnoreVersion))
	{
		UpdateDialog dlg(result.Version, result.Date, result.URL);
		if(dlg.DoModal() == IDOK)
		{
			CTrackApp::OpenURL(result.URL);
		}
	} else if(!result.UpdateAvailable && !autoUpdate)
	{
		Reporting::Information(MPT_USTRING("You already have the latest version of OpenMPT installed."), MPT_USTRING("OpenMPT Internet Update"));
	}
}


void CUpdateCheck::ShowFailureGUI(WPARAM wparam, LPARAM lparam)
{
	const CUpdateCheck::Error &error = *reinterpret_cast<CUpdateCheck::Error*>(lparam);
	bool autoUpdate = wparam != 0;
	if(!autoUpdate)
	{
		Reporting::Error(mpt::get_exception_text<mpt::ustring>(error), MPT_USTRING("OpenMPT Internet Update Error"));
	}
}


CUpdateCheck::Error::Error(CString errorMessage)
	: std::runtime_error(mpt::ToCharset(mpt::CharsetUTF8, errorMessage))
{
	return;
}


CUpdateCheck::Error::Error(CString errorMessage, DWORD errorCode)
	: std::runtime_error(mpt::ToCharset(mpt::CharsetUTF8, FormatErrorCode(errorMessage, errorCode)))
{
	return;
}


CString CUpdateCheck::Error::FormatErrorCode(CString errorMessage, DWORD errorCode)
{
	void *lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		GetModuleHandle(TEXT("wininet.dll")), errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	errorMessage.Append((LPTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return errorMessage;
}


/////////////////////////////////////////////////////////////
// CUpdateSetupDlg

BEGIN_MESSAGE_MAP(CUpdateSetupDlg, CPropertyPage)
	ON_COMMAND(IDC_CHECK_UPDATEENABLED,         &CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO1,                      &CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,                      &CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,                      &CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_BUTTON1,                     &CUpdateSetupDlg::OnCheckNow)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATEFREQUENCY, &CUpdateSetupDlg::OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,                      &CUpdateSetupDlg::OnSettingsChanged)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK1,           &CUpdateSetupDlg::OnShowStatisticsData)
END_MESSAGE_MAP()


CUpdateSetupDlg::CUpdateSetupDlg()
	: CPropertyPage(IDD_OPTIONS_UPDATE)
	, m_SettingChangedNotifyGuard(theApp.GetSettings(), TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath())
{
	return;
}


void CUpdateSetupDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_UPDATEFREQUENCY, m_CbnUpdateFrequency);
}


BOOL CUpdateSetupDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CheckDlgButton(IDC_CHECK_UPDATEENABLED, TrackerSettings::Instance().UpdateEnabled ? BST_CHECKED : BST_UNCHECKED);

	int radioID = 0;
	int updateChannel = TrackerSettings::Instance().UpdateChannel;
	if(updateChannel == UpdateChannelRelease)
	{
		radioID = IDC_RADIO1;
	} else if(updateChannel == UpdateChannelNext)
	{
		radioID = IDC_RADIO2;
	} else if(updateChannel == UpdateChannelDevelopment)
	{
		radioID = IDC_RADIO3;
	} else
	{
		radioID = IDC_RADIO1;
	}
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, radioID);

	int32 periodDays = TrackerSettings::Instance().UpdateIntervalDays;
	int ndx;

	ndx = m_CbnUpdateFrequency.AddString(_T("always"));
	m_CbnUpdateFrequency.SetItemData(ndx, 0);
	if(periodDays >= 0)
	{
		m_CbnUpdateFrequency.SetCurSel(ndx);
	}

	ndx = m_CbnUpdateFrequency.AddString(_T("daily"));
	m_CbnUpdateFrequency.SetItemData(ndx, 1);
	if(periodDays >= 1)
	{
		m_CbnUpdateFrequency.SetCurSel(ndx);
	}

	ndx = m_CbnUpdateFrequency.AddString(_T("weekly"));
	m_CbnUpdateFrequency.SetItemData(ndx, 7);
	if(periodDays >= 7)
	{
		m_CbnUpdateFrequency.SetCurSel(ndx);
	}

	ndx = m_CbnUpdateFrequency.AddString(_T("monthly"));
	m_CbnUpdateFrequency.SetItemData(ndx, 30);
	if(periodDays >= 30)
	{
		m_CbnUpdateFrequency.SetCurSel(ndx);
	}

	ndx = m_CbnUpdateFrequency.AddString(_T("never"));
	m_CbnUpdateFrequency.SetItemData(ndx, ~(DWORD_PTR)0);
	if(periodDays < 0)
	{		
		m_CbnUpdateFrequency.SetCurSel(ndx);
	}

	CheckDlgButton(IDC_CHECK1, TrackerSettings::Instance().UpdateStatistics ? BST_CHECKED : BST_UNCHECKED);

	GetDlgItem(IDC_STATIC_UPDATEPRIVACYTEXT)->SetWindowText(mpt::ToCString(CUpdateCheck::GetStatisticsUserInformation(true)));

	EnableDisableDialog();

	m_SettingChangedNotifyGuard.Register(this);
	SettingChanged(TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath());

	return TRUE;
}


void CUpdateSetupDlg::OnShowStatisticsData(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/)
{
	CUpdateCheck::Settings settings;

	int updateChannel = TrackerSettings::Instance().UpdateChannel;
	if(IsDlgButtonChecked(IDC_RADIO1)) updateChannel = UpdateChannelRelease;
	if(IsDlgButtonChecked(IDC_RADIO2)) updateChannel = UpdateChannelNext;
	if(IsDlgButtonChecked(IDC_RADIO3)) updateChannel = UpdateChannelDevelopment;

	int updateCheckPeriod = (m_CbnUpdateFrequency.GetItemData(m_CbnUpdateFrequency.GetCurSel()) == ~(DWORD_PTR)0) ? -1 : static_cast<int>(m_CbnUpdateFrequency.GetItemData(m_CbnUpdateFrequency.GetCurSel()));

	settings.periodDays = updateCheckPeriod;
	settings.channel = updateChannel;
	settings.sendStatistics = (IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);

	mpt::ustring statistics;
	statistics += MPT_USTRING("GET ") + CUpdateCheck::GetUpdateURLV2(settings) + MPT_ULITERAL("\n");
	statistics += MPT_ULITERAL("\n");
	if(settings.sendStatistics)
	{
		if(settings.statisticsUUID.IsValid())
		{
			statistics += MPT_USTRING("PUT ") + settings.apiURL + mpt::format(MPT_USTRING("statistics/%1"))(settings.statisticsUUID) + MPT_ULITERAL("\n");
		} else
		{
			statistics += MPT_USTRING("POST ") + settings.apiURL + MPT_USTRING("statistics/") + MPT_ULITERAL("\n");
		}
		statistics += mpt::String::Replace(mpt::ToUnicode(mpt::CharsetUTF8, CUpdateCheck::GetStatisticsDataV3(settings)), MPT_USTRING("\t"), MPT_USTRING("    "));
	}

	InfoDialog dlg(this);
	dlg.SetCaption(_T("Update Statistics Data"));
	dlg.SetContent(mpt::ToWin(mpt::String::Replace(statistics, MPT_USTRING("\n"), MPT_USTRING("\r\n"))));
	dlg.DoModal();
}


void CUpdateSetupDlg::SettingChanged(const SettingPath &changedPath)
{
	if(changedPath == TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath())
	{
		CString updateText;
		const time_t t = TrackerSettings::Instance().UpdateLastUpdateCheck.Get();
		if(t > 0)
		{
			const tm* const lastUpdate = localtime(&t);
			if(lastUpdate != nullptr)
			{
				updateText.Format(_T("The last successful update check was run on %04d-%02d-%02d, %02d:%02d."), lastUpdate->tm_year + 1900, lastUpdate->tm_mon + 1, lastUpdate->tm_mday, lastUpdate->tm_hour, lastUpdate->tm_min);
			}
		}
		updateText += _T("\r\n");
		if(TrackerSettings::Instance().UpdateSuggestDifferentBuildVariant)
		{
			updateText += theApp.SuggestModernBuildText();
		}
		SetDlgItemText(IDC_LASTUPDATE, updateText);
	}
}


void CUpdateSetupDlg::EnableDisableDialog()
{

	BOOL status = ((IsDlgButtonChecked(IDC_CHECK_UPDATEENABLED) != BST_UNCHECKED) ? TRUE : FALSE);

	GetDlgItem(IDC_STATIC_UDATECHANNEL)->EnableWindow(status);
	GetDlgItem(IDC_RADIO1)->EnableWindow(status);
	GetDlgItem(IDC_RADIO2)->EnableWindow(status);
	GetDlgItem(IDC_RADIO3)->EnableWindow(status);

	GetDlgItem(IDC_STATIC_UPDATECHECK)->EnableWindow(status);
	GetDlgItem(IDC_STATIC_UPDATEFREQUENCY)->EnableWindow(status);
	GetDlgItem(IDC_COMBO_UPDATEFREQUENCY)->EnableWindow(status);
	GetDlgItem(IDC_BUTTON1)->EnableWindow(status);
	GetDlgItem(IDC_LASTUPDATE)->EnableWindow(status);

	GetDlgItem(IDC_STATIC_UPDATEPRIVACY)->EnableWindow(status);
	GetDlgItem(IDC_CHECK1)->EnableWindow(status);
	GetDlgItem(IDC_STATIC_UPDATEPRIVACYTEXT)->EnableWindow(status);
	GetDlgItem(IDC_SYSLINK1)->EnableWindow(status);

	// disabled features
	GetDlgItem(IDC_RADIO2)->EnableWindow(FALSE);

}


void CUpdateSetupDlg::OnSettingsChanged()
{
	EnableDisableDialog();
	SetModified(TRUE);
}


void CUpdateSetupDlg::OnOK()
{
	int updateChannel = TrackerSettings::Instance().UpdateChannel;
	if(IsDlgButtonChecked(IDC_RADIO1)) updateChannel = UpdateChannelRelease;
	if(IsDlgButtonChecked(IDC_RADIO2)) updateChannel = UpdateChannelNext;
	if(IsDlgButtonChecked(IDC_RADIO3)) updateChannel = UpdateChannelDevelopment;

	int updateCheckPeriod = (m_CbnUpdateFrequency.GetItemData(m_CbnUpdateFrequency.GetCurSel()) == ~(DWORD_PTR)0) ? -1 : static_cast<int>(m_CbnUpdateFrequency.GetItemData(m_CbnUpdateFrequency.GetCurSel()));
	
	CString updateURL;
	GetDlgItemText(IDC_EDIT1, updateURL);

	if(GetDlgItem(IDC_CHECK_UPDATEENABLED)->IsWindowEnabled() != FALSE)
	{
		TrackerSettings::Instance().UpdateEnabled = (IsDlgButtonChecked(IDC_CHECK_UPDATEENABLED) != BST_UNCHECKED);
	}
	TrackerSettings::Instance().UpdateIntervalDays = updateCheckPeriod;
	TrackerSettings::Instance().UpdateChannel = updateChannel;
	TrackerSettings::Instance().UpdateStatistics = (IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);
	
	CPropertyPage::OnOK();
}


BOOL CUpdateSetupDlg::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_UPDATE;
	return CPropertyPage::OnSetActive();
}


void CUpdateSetupDlg::OnCheckNow()
{
	CMainFrame::GetMainFrame()->PostMessage(WM_COMMAND, ID_INTERNETUPDATE);
}


OPENMPT_NAMESPACE_END
