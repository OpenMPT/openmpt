/*
 * UpdateCheck.cpp
 * ---------------
 * Purpose: Class for easy software update check.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */

#include "stdafx.h"
#include "UpdateCheck.h"
#include "version.h"
#include "misc_util.h"
#include "Mptrack.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Static configuration variables
time_t CUpdateCheck::lastUpdateCheck = 0;
int CUpdateCheck::updateCheckPeriod = 7;
CString CUpdateCheck::updateBaseURL = "http://update.openmpt.org/check/%s";
bool CUpdateCheck::showUpdateHint = true;


CUpdateCheck::CUpdateCheck(const bool autoUpdate)
//-----------------------------------------------
{
	isAutoUpdate = autoUpdate;
	threadHandle = NULL;
}


CUpdateCheck::~CUpdateCheck()
//---------------------------
{
	if(threadHandle != NULL)
	{
		CloseHandle(threadHandle);
	}
}


// Start update check
void CUpdateCheck::DoUpdateCheck()
//--------------------------------
{
	internetHandle = NULL;
	connectionHandle = NULL;

	DWORD dummy;	// For Win9x
	threadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&CUpdateCheck::UpdateThread, (LPVOID)this, 0, &dummy);
	if(isAutoUpdate)
	{
		SetThreadPriority(threadHandle, THREAD_PRIORITY_BELOW_NORMAL);
	}
}


// Run update check (independent thread)
DWORD WINAPI CUpdateCheck::UpdateThread(LPVOID param)
//---------------------------------------------------
{
	CUpdateCheck *caller = (CUpdateCheck *)param;

	const time_t now = time(nullptr);

	if(caller->isAutoUpdate)
	{
		// Do we actually need to run the update check right now?
		if(CUpdateCheck::updateCheckPeriod == 0 || difftime(now, CUpdateCheck::lastUpdateCheck) < (double)(CUpdateCheck::updateCheckPeriod * 86400))
		{
			caller->Terminate();
			return 0;
		}

		// Never ran update checks before, so we notify the user of automatic update checks.
		if(CUpdateCheck::showUpdateHint)
		{
			CString msg;
			msg.Format("OpenMPT would like to check for updates now, proceed?\n\nNote: In the future, OpenMPT will check for updates every %d days. If you do not want this, you can disable update checks in the setup.", CUpdateCheck::updateCheckPeriod);
			if(::MessageBox(0, msg, "OpenMPT Internet Update", MB_YESNO | MB_ICONQUESTION) == IDNO)
			{
				CUpdateCheck::showUpdateHint = false;
				caller->Terminate();
				return 0;
			}
			
		}
	}
	CUpdateCheck::showUpdateHint = false;

	const CString userAgent = CString("OpenMPT ") + MptVersion::str;
	CString updateURL;
	updateURL.Format(CUpdateCheck::updateBaseURL, MptVersion::str);

	/*if (CMainFrame::gcsInstallGUID == "")
	{
		//No GUID found in INI file - generate one.
		GUID guid;
		CoCreateGuid(&guid);
		BYTE* Str;
		UuidToString((UUID*)&guid, &Str);
		CMainFrame::gcsInstallGUID.Format("%s", (LPTSTR)Str);
		RpcStringFree(&Str);
	}*/

	// Establish a connection.
	caller->internetHandle = InternetOpen(userAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(caller->internetHandle == NULL)
	{
		caller->Die("Could not start update check:\n", GetLastError());
		return 0;
	}
	caller->connectionHandle = InternetOpenUrl(caller->internetHandle, updateURL, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);
	if(caller->connectionHandle == NULL)
	{
		caller->Die("Could not establish connection:\n", GetLastError());
		return 0;
	}

	// Retrieve HTTP status code.
	DWORD statusCodeHTTP = 0;
	DWORD length = sizeof(statusCodeHTTP);
	if(HttpQueryInfo(caller->connectionHandle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&statusCodeHTTP, &length, NULL) == FALSE)
	{
		caller->Die("Could not retrieve HTTP header information:\n", GetLastError());
		return 0;
	}
	if(statusCodeHTTP >= 400)
	{
		CString error;
		error.Format("Version information could not be found on the server (HTTP status code %d). Maybe your version of OpenMPT is too old!", statusCodeHTTP);
		caller->Die(error);
		return 0;
	}

	// Download data.
	CString resultData = "";
	char *downloadBuffer = new char[DOWNLOAD_BUFFER_SIZE];
    DWORD availableSize, bytesRead;
	do
	{
		// Query number of available bytes to download
		if(InternetQueryDataAvailable(caller->connectionHandle, &availableSize, 0, NULL) == FALSE)
		{
			caller->Die("Error while downloading update information data:\n", GetLastError());
		}

		LimitMax(availableSize, (DWORD)DOWNLOAD_BUFFER_SIZE);

		// Put downloaded bytes into our buffer
		if(InternetReadFile(caller->connectionHandle, downloadBuffer, availableSize, &bytesRead) == FALSE)
		{
			caller->Die("Error while downloading update information data:\n", GetLastError());
		}

		resultData.Append(downloadBuffer, availableSize);
		Sleep(1);

	} while(bytesRead != 0);
	delete[] downloadBuffer;
	
	// Now, evaluate the downloaded data.
	if(!resultData.CompareNoCase("noupdate"))
	{
		if(!caller->isAutoUpdate)
		{
			::MessageBox(0, "You already have the latest version of OpenMPT.", "OpenMPT Internet Update", MB_OK | MB_ICONINFORMATION);
		}
	} else
	{
		CString releaseVersion, releaseDate, releaseURL;
		CString token;
		int parseStep = 0, parsePos = 0;
		while((token = resultData.Tokenize("\n", parsePos)) != "")
		{
			token.Trim();
			switch(parseStep++)
			{
			case 0:
				if(token.CompareNoCase("update") != 0)
				{
					caller->Die("Could not understand server response. Maybe your version of OpenMPT is too old!");
					return 0;
				}
				break;
			case 1:
				releaseVersion = token;
				break;
			case 2:
				releaseDate = token;
				break;
			case 3:
				releaseURL = token;
				break;
			}
		}
		if(parseStep >= 4)
		{
			resultData.Format("A new version is available!\nOpenMPT %s has been released on %s. Would you like to visit %s for more information?", releaseVersion, releaseDate, releaseURL);
			if(::MessageBox(0, resultData, "OpenMPT Internet Update", MB_YESNO | MB_ICONINFORMATION) == IDYES)
			{
				CTrackApp::OpenURL(releaseURL);
			}
		} else
		{
			caller->Die("Could not understand server response. Maybe your version of OpenMPT is too old!");
			return 0;
		}
	}

	CUpdateCheck::lastUpdateCheck = now;

	caller->Terminate();
	return 0;
}


// Die with error message
void CUpdateCheck::Die(CString errorMessage)
//------------------------------------------
{
	if(!isAutoUpdate)
	{
		::MessageBox(0, errorMessage, "OpenMPT Internet Update Error", MB_OK | MB_ICONERROR);
	}
	Terminate();
}


// Die with WinINet error message
void CUpdateCheck::Die(CString errorMessage, DWORD errorCode)
//-----------------------------------------------------------
{
	if(!isAutoUpdate)
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			GetModuleHandle(TEXT("wininet.dll")), errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		errorMessage.Append((LPTSTR)lpMsgBuf);
		LocalFree(lpMsgBuf);
	}
	Die(errorMessage);
}


// Kill update object
void CUpdateCheck::Terminate()
//----------------------------
{
	if(connectionHandle != NULL)
	{
		InternetCloseHandle(connectionHandle);
		connectionHandle = NULL;
	}
	if(internetHandle != NULL)
	{
		InternetCloseHandle(internetHandle);
		internetHandle = NULL;
	}
	threadHandle = NULL;	// If we got here, the thread asked for termination, so don't use CloseHandle()
	delete this;
}


/////////////////////////////////////////////////////////////
// CUpdateSetupDlg

BEGIN_MESSAGE_MAP(CUpdateSetupDlg, CPropertyPage)
	ON_COMMAND(IDC_BUTTON1,			OnCheckNow)
	ON_COMMAND(IDC_RADIO1,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO2,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO3,			OnSettingsChanged)
	ON_COMMAND(IDC_RADIO4,			OnSettingsChanged)
	ON_EN_CHANGE(IDC_EDIT1,			OnSettingsChanged)
END_MESSAGE_MAP()


BOOL CUpdateSetupDlg::OnInitDialog()
//----------------------------------
{
	CPropertyPage::OnInitDialog();

	int radioID = 0;
	switch(CUpdateCheck::GetUpdateCheckPeriod())
	{
	case 0:		radioID = IDC_RADIO1; break;
	case 1:		radioID = IDC_RADIO2; break;
	case 7:		radioID = IDC_RADIO3; break;
	case 31:	radioID = IDC_RADIO4; break;
	}
	CheckRadioButton(IDC_RADIO1, IDC_RADIO4, radioID);
	SetDlgItemText(IDC_EDIT1, CUpdateCheck::GetUpdateURL());

	const time_t t = CUpdateCheck::GetLastUpdateCheck();
	if(t > 0)
	{
		CString updateText;
		const tm* const lastUpdate = localtime(&t);
		if(lastUpdate != nullptr)
		{
			updateText.Format("The last successful update check was run on %04d-%02d-%02d, %02d:%02d.", lastUpdate->tm_year + 1900, lastUpdate->tm_mon + 1, lastUpdate->tm_mday, lastUpdate->tm_hour, lastUpdate->tm_min);
			SetDlgItemText(IDC_LASTUPDATE, updateText);
		}
	}

	return TRUE;
}


void CUpdateSetupDlg::OnOK()
//--------------------------
{
	int updateCheckPeriod = CUpdateCheck::GetUpdateCheckPeriod();
	if(IsDlgButtonChecked(IDC_RADIO1)) updateCheckPeriod = 0;
	if(IsDlgButtonChecked(IDC_RADIO2)) updateCheckPeriod = 1;
	if(IsDlgButtonChecked(IDC_RADIO3)) updateCheckPeriod = 7;
	if(IsDlgButtonChecked(IDC_RADIO4)) updateCheckPeriod = 31;

	CString updateURL;
	GetDlgItemText(IDC_EDIT1, updateURL);
	CUpdateCheck::SetUpdateSettings(CUpdateCheck::GetLastUpdateCheck(), updateCheckPeriod, updateURL, CUpdateCheck::GetShowUpdateHint());
	
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

