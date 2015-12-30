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
#include "../common/version.h"
#include "../common/misc_util.h"
#include "Mptrack.h"
#include "TrackerSettings.h"
// Setup dialog stuff
#include "Mainfrm.h"
#include "../common/thread.h"


OPENMPT_NAMESPACE_BEGIN


const TCHAR *const CUpdateCheck::defaultUpdateURL = _T("http://update.openmpt.org/check/$VERSION/$GUID");

mpt::atomic_int32_t CUpdateCheck::s_InstanceCount;


int32 CUpdateCheck::GetNumCurrentRunningInstances()
//-------------------------------------------------
{
	return s_InstanceCount.load();
}


// Start update check
void CUpdateCheck::StartUpdateCheckAsync(bool isAutoUpdate)
//---------------------------------------------------------
{
	if(isAutoUpdate)
	{
		int updateCheckPeriod = TrackerSettings::Instance().UpdateUpdateCheckPeriod;
		if(updateCheckPeriod == 0)
		{
			return;
		}
		// Do we actually need to run the update check right now?
		const time_t now = time(nullptr);
		if(difftime(now, TrackerSettings::Instance().UpdateLastUpdateCheck.Get()) < (double)(updateCheckPeriod * 86400))
		{
			return;
		}

		// Never ran update checks before, so we notify the user of automatic update checks.
		if(TrackerSettings::Instance().UpdateShowUpdateHint)
		{
			TrackerSettings::Instance().UpdateShowUpdateHint = false;
			CString msg;
			msg.Format(_T("OpenMPT would like to check for updates now, proceed?\n\nNote: In the future, OpenMPT will check for updates every %d days. If you do not want this, you can disable update checks in the setup."), TrackerSettings::Instance().UpdateUpdateCheckPeriod.Get());
			if(Reporting::Confirm(msg, "OpenMPT Internet Update") == cnfNo)
			{
				TrackerSettings::Instance().UpdateLastUpdateCheck = mpt::Date::Unix(now);
				return;
			}
		}
	}
	TrackerSettings::Instance().UpdateShowUpdateHint = false;

	int32 expected = 0;
	if(!s_InstanceCount.compare_exchange_strong(expected, 1))
	{
		return;
	}

	CUpdateCheck::Settings settings;
	settings.window = CMainFrame::GetMainFrame();
	settings.msgProgress = MPT_WM_APP_UPDATECHECK_PROGRESS;
	settings.msgSuccess = MPT_WM_APP_UPDATECHECK_SUCCESS;
	settings.msgFailure = MPT_WM_APP_UPDATECHECK_FAILURE;
	settings.autoUpdate = isAutoUpdate;
	settings.updateBaseURL = TrackerSettings::Instance().UpdateUpdateURL;
	settings.guidString = (TrackerSettings::Instance().UpdateSendGUID ? mpt::ToCString(TrackerSettings::Instance().gcsInstallGUID.Get()) : _T("anonymous"));
	settings.suggestDifferentBuilds = TrackerSettings::Instance().UpdateSuggestDifferentBuildVariant;
	mpt::thread(CUpdateCheck::ThreadFunc(settings)).detach();
}


CUpdateCheck::ThreadFunc::ThreadFunc(const CUpdateCheck::Settings &settings)
//--------------------------------------------------------------------------
	: settings(settings)
{
	return;
}


void CUpdateCheck::ThreadFunc::operator () ()
//-------------------------------------------
{
	mpt::SetCurrentThreadPriority(settings.autoUpdate ? mpt::ThreadPriorityLower : mpt::ThreadPriorityNormal);
	CUpdateCheck().CheckForUpdate(settings);
}


CUpdateCheck::CUpdateCheck()
//--------------------------
	: internetHandle(nullptr)
	, connectionHandle(nullptr)
{
	return;
}


// Run update check (independent thread)
CUpdateCheck::Result CUpdateCheck::SearchUpdate(const CUpdateCheck::Settings &settings)
//-------------------------------------------------------------------------------------
{

	// Prepare UA / URL strings...
	const CString userAgent = CString(_T("OpenMPT ")) + MptVersion::str;
	CString updateURL = settings.updateBaseURL;
	CString versionStr = MptVersion::str;
#ifdef _WIN64
	versionStr.Append(_T("-win64"));
#elif defined(_WIN32)
	if(MptVersion::IsForOlderWindows())
	{
		versionStr.Append(_T("-win32old"));
	} else
	{
		versionStr.Append(_T("-win32"));
	}
#else
#error "Platform-specific identifier missing"
#endif
	updateURL.Replace(_T("$VERSION"), versionStr);
	updateURL.Replace(_T("$GUID"), settings.guidString);

	// Establish a connection.
	internetHandle = InternetOpen(userAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(internetHandle == NULL)
	{
		throw CUpdateCheck::Error("Could not start update check:\n", GetLastError());
	}
	connectionHandle = InternetOpenUrl(internetHandle, updateURL, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);
	if(connectionHandle == NULL)
	{
		throw CUpdateCheck::Error("Could not establish connection:\n", GetLastError());
	}

	// Retrieve HTTP status code.
	DWORD statusCodeHTTP = 0;
	DWORD length = sizeof(statusCodeHTTP);
	if(HttpQueryInfo(connectionHandle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&statusCodeHTTP, &length, NULL) == FALSE)
	{
		throw CUpdateCheck::Error("Could not retrieve HTTP header information:\n", GetLastError());
	}
	if(statusCodeHTTP >= 400)
	{
		CString error;
		error.Format(_T("Version information could not be found on the server (HTTP status code %d). Maybe your version of OpenMPT is too old!"), statusCodeHTTP);
		throw CUpdateCheck::Error(error);
	}

	// Download data.
	std::string resultBuffer = "";
	DWORD bytesRead = 0;
	do
	{
		// Query number of available bytes to download
		DWORD availableSize = 0;
		if(InternetQueryDataAvailable(connectionHandle, &availableSize, 0, NULL) == FALSE)
		{
			throw CUpdateCheck::Error("Error while downloading update information data:\n", GetLastError());
		}

		LimitMax(availableSize, (DWORD)DOWNLOAD_BUFFER_SIZE);

		// Put downloaded bytes into our buffer
		char downloadBuffer[DOWNLOAD_BUFFER_SIZE];
		if(InternetReadFile(connectionHandle, downloadBuffer, availableSize, &bytesRead) == FALSE)
		{
			throw CUpdateCheck::Error("Error while downloading update information data:\n", GetLastError());
		}
		resultBuffer.append(downloadBuffer, downloadBuffer + bytesRead);

		Sleep(1);
	} while(bytesRead != 0);
	
	// Now, evaluate the downloaded data.
	CUpdateCheck::Result result;
	result.UpdateAvailable = false;
	result.CheckTime = time(nullptr);
	CString resultData = mpt::ToCString(mpt::CharsetUTF8, resultBuffer);
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
					throw CUpdateCheck::Error("Could not understand server response. Maybe your version of OpenMPT is too old!");
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
			throw CUpdateCheck::Error("Could not understand server response. Maybe your version of OpenMPT is too old!");
		}
		result.UpdateAvailable = true;
	}
	return result;
}


void CUpdateCheck::CheckForUpdate(const CUpdateCheck::Settings &settings)
//-----------------------------------------------------------------------
{
	// íncremented before starting the thread
	MPT_ASSERT(s_InstanceCount.load() >= 1);
	CUpdateCheck::Result result;
	settings.window->SendMessage(settings.msgProgress, settings.autoUpdate ? 1 : 0, s_InstanceCount.load());
	try
	{
		result = SearchUpdate(settings);
	} catch(const CUpdateCheck::Error &e)
	{
		settings.window->SendMessage(settings.msgFailure, settings.autoUpdate ? 1 : 0, reinterpret_cast<LPARAM>(&e));
		s_InstanceCount.fetch_sub(1);
		MPT_ASSERT(s_InstanceCount.load() >= 0);
		return;
	}
	settings.window->SendMessage(settings.msgSuccess, settings.autoUpdate ? 1 : 0, reinterpret_cast<LPARAM>(&result));
	s_InstanceCount.fetch_sub(1);
	MPT_ASSERT(s_InstanceCount.load() >= 0);
}


CUpdateCheck::Result CUpdateCheck::ResultFromMessage(WPARAM /*wparam*/ , LPARAM lparam)
//-------------------------------------------------------------------------------------
{
	const CUpdateCheck::Result &result = *reinterpret_cast<CUpdateCheck::Result*>(lparam);
	return result;
}


CUpdateCheck::Error CUpdateCheck::ErrorFromMessage(WPARAM /*wparam*/ , LPARAM lparam)
//-----------------------------------------------------------------------------------
{
	const CUpdateCheck::Error &error = *reinterpret_cast<CUpdateCheck::Error*>(lparam);
	return error;
}


void CUpdateCheck::ShowSuccessGUI(WPARAM wparam, LPARAM lparam)
//-------------------------------------------------------------
{
	const CUpdateCheck::Result &result = *reinterpret_cast<CUpdateCheck::Result*>(lparam);
	bool autoUpdate = (wparam ? true : false);
	if(result.UpdateAvailable)
	{
		if(Reporting::Confirm(
			mpt::format(MPT_USTRING("A new version is available!\nOpenMPT %1 has been released on %2. Would you like to visit %3 for more information?"))
			 (result.Version, result.Date, result.URL)
			 , MPT_USTRING("OpenMPT Internet Update")) == cnfYes)
		{
			CTrackApp::OpenURL(result.URL);
		}
	} else if(!autoUpdate)
	{
		Reporting::Information(MPT_USTRING("You already have the latest version of OpenMPT installed."), MPT_USTRING("OpenMPT Internet Update"));
	}
}


void CUpdateCheck::ShowFailureGUI(WPARAM wparam, LPARAM lparam)
//-------------------------------------------------------------
{
	const CUpdateCheck::Error &error = *reinterpret_cast<CUpdateCheck::Error*>(lparam);
	bool autoUpdate = (wparam ? true : false);
	if(!autoUpdate)
	{
		Reporting::Error(mpt::ToUnicode(mpt::CharsetUTF8, error.what() ? std::string(error.what()) : std::string()), MPT_USTRING("OpenMPT Internet Update Error"));
	}
}


CUpdateCheck::Error::Error(CString errorMessage)
//----------------------------------------------
	: std::runtime_error(mpt::ToCharset(mpt::CharsetUTF8, errorMessage))
{
	return;
}


CUpdateCheck::Error::Error(CString errorMessage, DWORD errorCode)
//---------------------------------------------------------------
	: std::runtime_error(mpt::ToCharset(mpt::CharsetUTF8, FormatErrorCode(errorMessage, errorCode)))
{
	return;
}


CString CUpdateCheck::Error::FormatErrorCode(CString errorMessage, DWORD errorCode)
//---------------------------------------------------------------------------------
{
	void *lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		GetModuleHandle(TEXT("wininet.dll")), errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	errorMessage.Append((LPTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return errorMessage;
}


CUpdateCheck::~CUpdateCheck()
//---------------------------
{
	if(connectionHandle != nullptr)
	{
		InternetCloseHandle(connectionHandle);
		connectionHandle = nullptr;
	}
	if(internetHandle != nullptr)
	{
		InternetCloseHandle(internetHandle);
		internetHandle = nullptr;
	}
}


/////////////////////////////////////////////////////////////
// CUpdateSetupDlg

BEGIN_MESSAGE_MAP(CUpdateSetupDlg, CPropertyPage)
	ON_COMMAND(IDC_BUTTON1,			OnCheckNow)
	ON_COMMAND(IDC_BUTTON2,			OnResetURL)
	ON_COMMAND(IDC_RADIO1,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO4,			OnSettingsChanged)
	ON_COMMAND(IDC_CHECK1,			OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT1,			OnSettingsChanged)
END_MESSAGE_MAP()


CUpdateSetupDlg::CUpdateSetupDlg()
//--------------------------------
	: CPropertyPage(IDD_OPTIONS_UPDATE)
	, m_SettingChangedNotifyGuard(theApp.GetSettings(), TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath())
{
	return;
}


BOOL CUpdateSetupDlg::OnInitDialog()
//----------------------------------
{
	CPropertyPage::OnInitDialog();

	int radioID = 0;
	int periodDays = TrackerSettings::Instance().UpdateUpdateCheckPeriod;
	if(periodDays >= 30)
	{
		radioID = IDC_RADIO4;
	} else if(periodDays >= 7)
	{
		radioID = IDC_RADIO3;
	} else if(periodDays >= 1)
	{
		radioID = IDC_RADIO2;
	} else
	{
		radioID = IDC_RADIO1;
	}
	CheckRadioButton(IDC_RADIO1, IDC_RADIO4, radioID);
	CheckDlgButton(IDC_CHECK1, TrackerSettings::Instance().UpdateSendGUID ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(IDC_EDIT1, TrackerSettings::Instance().UpdateUpdateURL.Get());

	m_SettingChangedNotifyGuard.Register(this);
	SettingChanged(TrackerSettings::Instance().UpdateLastUpdateCheck.GetPath());

	return TRUE;
}


void CUpdateSetupDlg::SettingChanged(const SettingPath &changedPath)
//------------------------------------------------------------------
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
			const CString url = mpt::ToCString(MptVersion::GetDownloadURL());
			if(mpt::Windows::IsOriginal())
			{ // only do compatibility checks on non-emulated windows
				if(MptVersion::IsForOlderWindows())
				{
					if(theApp.SystemCanRunModernBuilds())
					{
						updateText += CString(_T("You are running a 'Win32old' build of OpenMPT.")) + _T("\r\n");
						updateText += CString(_T("However, OpenMPT detected that your system is capable of running the standard 'Win32' build as well, which provides better support for your system.")) + _T("\r\n");
						updateText += CString(_T("You may want to visit ")) + url + CString(_T(" and upgrade.")) + _T("\r\n");
					}
				}
			}
		}
		SetDlgItemText(IDC_LASTUPDATE, updateText);
	}
}


void CUpdateSetupDlg::OnOK()
//--------------------------
{
	int updateCheckPeriod = TrackerSettings::Instance().UpdateUpdateCheckPeriod;
	if(IsDlgButtonChecked(IDC_RADIO1)) updateCheckPeriod = 0;
	if(IsDlgButtonChecked(IDC_RADIO2)) updateCheckPeriod = 1;
	if(IsDlgButtonChecked(IDC_RADIO3)) updateCheckPeriod = 7;
	if(IsDlgButtonChecked(IDC_RADIO4)) updateCheckPeriod = 31;

	CString updateURL;
	GetDlgItemText(IDC_EDIT1, updateURL);

	TrackerSettings::Instance().UpdateUpdateCheckPeriod = updateCheckPeriod;
	TrackerSettings::Instance().UpdateUpdateURL = updateURL;
	TrackerSettings::Instance().UpdateSendGUID = (IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED);
	
	CPropertyPage::OnOK();
}


BOOL CUpdateSetupDlg::OnSetActive()
//---------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_UPDATE;
	return CPropertyPage::OnSetActive();
}


void CUpdateSetupDlg::OnCheckNow()
//--------------------------------
{
	CMainFrame::GetMainFrame()->PostMessage(WM_COMMAND, ID_INTERNETUPDATE);
}


void CUpdateSetupDlg::OnResetURL()
//--------------------------------
{
	SetDlgItemText(IDC_EDIT1, CUpdateCheck::defaultUpdateURL);
}


OPENMPT_NAMESPACE_END
