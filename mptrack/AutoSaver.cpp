/*
 * AutoSaver.cpp
 * -------------
 * Purpose: Class for automatically saving open modules at a specified interval.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "AutoSaver.h"
#include "moptions.h"
#include "FileDialog.h"
#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////////
// AutoSaver.cpp : implementation file
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////
// Construction/Destruction
///////////////////////////

CAutoSaver::CAutoSaver(bool enabled, uint32_t saveInterval, uint32_t backupHistory, bool useOriginalPath, mpt::PathString path)
//-----------------------------------------------------------------------------------------------------------------------------
	: m_bSaveInProgress(false)
	, m_nLastSave(timeGetTime())
	, m_bEnabled(enabled)
	, m_nSaveInterval(saveInterval * 60 * 1000) //minutes to milliseconds
	, m_nBackupHistory(backupHistory)
	, m_bUseOriginalPath(useOriginalPath)
	, m_csPath(path)
{
}


//////////////
// Entry Point
//////////////


bool CAutoSaver::DoSave(DWORD curTime)
//------------------------------------
{
	bool success = true;

	//If time to save and not already having save in progress.
	if (CheckTimer(curTime) && !m_bSaveInProgress)
	{ 
		m_bSaveInProgress = true;

		theApp.BeginWaitCursor(); //display hour glass

		std::vector<CModDoc *> docs = theApp.GetOpenDocuments();
		for(std::vector<CModDoc *>::iterator doc = docs.begin(); doc != docs.end(); doc++)
		{
			CModDoc &modDoc = **doc;
			if(modDoc.ModifiedSinceLastAutosave())
			{
				if(SaveSingleFile(modDoc))
				{
					CleanUpBackups(modDoc);
				} else
				{
					m_bEnabled = false;
					Reporting::Warning("Warning: Auto Save failed and has been disabled. Please:\n- Review your Auto Save paths\n- Check available disk space and filesystem access rights");
					success = false;
				}
			}
		}

		m_nLastSave = timeGetTime();
		theApp.EndWaitCursor(); //end display hour glass
		m_bSaveInProgress = false;
	}
	
	return success;
}


///////////////////////////
// Implementation internals
///////////////////////////


bool CAutoSaver::CheckTimer(DWORD curTime)
//----------------------------------------
{
	DWORD curInterval = curTime - m_nLastSave;
	return (curInterval >= m_nSaveInterval);
}


mpt::PathString CAutoSaver::BuildFileName(CModDoc &modDoc)
//--------------------------------------------------------
{
	mpt::PathString name;
	
	if(m_bUseOriginalPath)
	{
		if(modDoc.m_bHasValidPath && !(name = modDoc.GetPathNameMpt()).empty())
		{
			// File has a user-chosen path - no change required
		} else
		{
			// if it doesn't, put it in settings dir
			name = theApp.GetConfigPath() + MPT_PATHSTRING("Autosave\\");
			if(!CreateDirectoryW(name.AsNative().c_str(), nullptr) && GetLastError() == ERROR_PATH_NOT_FOUND)
			{
				name = theApp.GetConfigPath();
			}
			name += mpt::PathString::FromCStringSilent(modDoc.GetTitle()).SanitizeComponent();
		}
	} else
	{
		name = m_csPath + mpt::PathString::FromCStringSilent(modDoc.GetTitle()).SanitizeComponent();
	}
	
	const mpt::ustring timeStamp = mpt::ToUnicode((CTime::GetCurrentTime()).Format(_T(".AutoSave.%Y%m%d.%H%M%S.")));
	name += mpt::PathString::FromUnicode(timeStamp);			//append backtup tag + timestamp
	name += mpt::PathString::FromUTF8(modDoc.GetrSoundFile().GetModSpecifications().fileExtension);

	return name;
}


bool CAutoSaver::SaveSingleFile(CModDoc &modDoc)
//----------------------------------------------
{
	// We do not call CModDoc::DoSave as this populates the Recent Files
	// list with backups... hence we have duplicated code.. :(
	CSoundFile &sndFile = modDoc.GetrSoundFile(); 
	
	mpt::PathString fileName = BuildFileName(modDoc);

	// We are acutally not going to show the log for autosaved files.
	ScopedLogCapturer logcapturer(modDoc, "", nullptr, false);

	bool success = false;
	switch(modDoc.GetrSoundFile().GetBestSaveFormat())
	{
	case MOD_TYPE_MOD:
		success = sndFile.SaveMod(fileName);
		break;

	case MOD_TYPE_S3M:
		success = sndFile.SaveS3M(fileName);
		break;

	case MOD_TYPE_XM:
		success = sndFile.SaveXM(fileName);
		break;

	case MOD_TYPE_IT:
		success = sndFile.SaveIT(fileName);
		break;

	case MOD_TYPE_MPT:
		//Using IT save function also for MPT.
		success = sndFile.SaveIT(fileName);
		break;
	}

	return success;
}


void CAutoSaver::CleanUpBackups(const CModDoc &modDoc)
//----------------------------------------------------
{
	mpt::PathString path;
	
	if (m_bUseOriginalPath)
	{
		if (modDoc.m_bHasValidPath && !(path = modDoc.GetPathNameMpt()).empty())
		{
			// File has a user-chosen path - remove filename
			path = path.GetPath();
		} else
		{
			// if it doesn't, put it in settings dir
			path = theApp.GetConfigPath() + MPT_PATHSTRING("Autosave\\");
		}
	} else
	{
		path = m_csPath;
	}

	std::vector<mpt::PathString> foundfiles;
	mpt::PathString searchPattern = path + mpt::PathString::FromCStringSilent(modDoc.GetTitle()).SanitizeComponent() + MPT_PATHSTRING(".AutoSave.*");

	WIN32_FIND_DATAW findData;
	MemsetZero(findData);
	HANDLE hFindFile = FindFirstFileW(searchPattern.AsNative().c_str(), &findData);
	if(hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				foundfiles.push_back(path + mpt::PathString::FromNative(findData.cFileName));
			}
		} while(FindNextFileW(hFindFile, &findData));
		FindClose(hFindFile);
		hFindFile = INVALID_HANDLE_VALUE;
	}
	std::sort(foundfiles.begin(), foundfiles.end());
	
	while(foundfiles.size() > (size_t)m_nBackupHistory)
	{
		DeleteFileW(foundfiles[0].AsNative().c_str());
		foundfiles.erase(foundfiles.begin());
	}
	
}


///////////////////////////////////////////////////////////////////////////////////////
// CAutoSaverGUI dialog : AutoSaver GUI
///////////////////////////////////////////////////////////////////////////////////////


IMPLEMENT_DYNAMIC(CAutoSaverGUI, CPropertyPage)
CAutoSaverGUI::CAutoSaverGUI(CAutoSaver* pAutoSaver)
	: CPropertyPage(CAutoSaverGUI::IDD)
{
	m_pAutoSaver = pAutoSaver;
}


void CAutoSaverGUI::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAutoSaverGUI, CPropertyPage)
	ON_BN_CLICKED(IDC_AUTOSAVE_BROWSE, OnBnClickedAutosaveBrowse)
	ON_BN_CLICKED(IDC_AUTOSAVE_ENABLE, OnBnClickedAutosaveEnable)
	ON_BN_CLICKED(IDC_AUTOSAVE_USEORIGDIR, OnBnClickedAutosaveUseorigdir)
	ON_BN_CLICKED(IDC_AUTOSAVE_USECUSTOMDIR, OnBnClickedAutosaveUseorigdir)
	ON_EN_UPDATE(IDC_AUTOSAVE_PATH, OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_HISTORY, OnSettingsChanged)
	ON_EN_UPDATE(IDC_AUTOSAVE_INTERVAL, OnSettingsChanged)
END_MESSAGE_MAP()


// CAutoSaverGUI message handlers

BOOL CAutoSaverGUI::OnInitDialog()
//--------------------------------
{
	CPropertyPage::OnInitDialog();

	CheckDlgButton(IDC_AUTOSAVE_ENABLE, m_pAutoSaver->IsEnabled()?BST_CHECKED:BST_UNCHECKED);
	//SetDlgItemText(IDC_AUTOSAVE_FNTEMPLATE, m_pAutoSaver->GetFilenameTemplate());
	SetDlgItemInt(IDC_AUTOSAVE_HISTORY, m_pAutoSaver->GetHistoryDepth()); //TODO
	::SetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, m_pAutoSaver->GetPath().AsNative().c_str());
	SetDlgItemInt(IDC_AUTOSAVE_INTERVAL, m_pAutoSaver->GetSaveInterval());
	CheckDlgButton(IDC_AUTOSAVE_USEORIGDIR, m_pAutoSaver->GetUseOriginalPath()?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_AUTOSAVE_USECUSTOMDIR, m_pAutoSaver->GetUseOriginalPath()?BST_UNCHECKED:BST_CHECKED);

	//enable/disable stuff as appropriate
	OnBnClickedAutosaveEnable();
	OnBnClickedAutosaveUseorigdir();

	return TRUE;
}


void CAutoSaverGUI::OnOK()
//------------------------
{
	WCHAR tempPath[MAX_PATH];
	m_pAutoSaver->SetEnabled(IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) != BST_UNCHECKED);
	m_pAutoSaver->SetHistoryDepth(GetDlgItemInt(IDC_AUTOSAVE_HISTORY));
	m_pAutoSaver->SetSaveInterval(GetDlgItemInt(IDC_AUTOSAVE_INTERVAL));
	m_pAutoSaver->SetUseOriginalPath(IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR) == BST_CHECKED);
	::GetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, tempPath, CountOf(tempPath));
	mpt::PathString path = mpt::PathString::FromNative(tempPath);
	if(!path.empty() && !path.HasTrailingSlash())
	{
		path += MPT_PATHSTRING("\\");
	}
	m_pAutoSaver->SetPath(path);

	CPropertyPage::OnOK();
}

void CAutoSaverGUI::OnBnClickedAutosaveBrowse()
//---------------------------------------------
{
	WCHAR szPath[MAX_PATH] = L"";
	::GetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, szPath, CountOf(szPath));

	BrowseForFolder dlg(mpt::PathString::FromNative(szPath), TEXT("Select a folder to store autosaved files in..."));
	if(dlg.Show(this))
	{
		::SetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, dlg.GetDirectory().AsNative().c_str());
		OnSettingsChanged();
	}
}


void CAutoSaverGUI::OnBnClickedAutosaveEnable()
//---------------------------------------------
{
	BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE);
	GetDlgItem(IDC_AUTOSAVE_INTERVAL)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_HISTORY)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_USEORIGDIR)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_USECUSTOMDIR)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_PATH)->EnableWindow(enabled);
	GetDlgItem(IDC_AUTOSAVE_BROWSE)->EnableWindow(enabled);
	OnSettingsChanged();
	return;
}

void CAutoSaverGUI::OnBnClickedAutosaveUseorigdir()
//-------------------------------------------------
{
	if (IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE))
	{
		BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR);
		GetDlgItem(IDC_AUTOSAVE_PATH)->EnableWindow(!enabled);
		GetDlgItem(IDC_AUTOSAVE_BROWSE)->EnableWindow(!enabled);
		OnSettingsChanged();
	}
	return;
}

void CAutoSaverGUI::OnSettingsChanged()
//-------------------------------------
{
	SetModified(TRUE);
}

BOOL CAutoSaverGUI::OnSetActive()
//--------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_AUTOSAVE;
	return CPropertyPage::OnSetActive();
}

BOOL CAutoSaverGUI::OnKillActive()
//--------------------------------
{
	WCHAR szPath[MAX_PATH] = L"";
	::GetDlgItemTextW(m_hWnd, IDC_AUTOSAVE_PATH, szPath, CountOf(szPath));

	if (!::PathIsDirectoryW(szPath) && IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) && !IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR))
	{
		Reporting::Error("Backup path does not exist.");
		GetDlgItem(IDC_AUTOSAVE_PATH)->SetFocus();
		return 0;
	}

	return CPropertyPage::OnKillActive();
}


OPENMPT_NAMESPACE_END
