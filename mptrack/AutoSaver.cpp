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
#include <algorithm>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>


///////////////////////////////////////////////////////////////////////////////////////
// AutoSaver.cpp : implementation file
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////
// Construction/Destruction
///////////////////////////

CAutoSaver::CAutoSaver() : m_bSaveInProgress(false)
{
}

CAutoSaver::CAutoSaver(bool enabled, int saveInterval, int backupHistory,
					   bool useOriginalPath, CString path, CString fileNameTemplate) :
	m_bSaveInProgress(false)
{
	m_nLastSave			 = timeGetTime();
	m_bEnabled			 = enabled;
	m_nSaveInterval		 = saveInterval*60*1000; //minutes to milliseconds
	m_nBackupHistory	 = backupHistory;
	m_bUseOriginalPath	 = useOriginalPath;
	m_csPath			 = path;
	m_csFileNameTemplate = fileNameTemplate;
}

CAutoSaver::~CAutoSaver(void)
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
					Reporting::Warning("Warning: Autosave failed and has been disabled. Please:\n- Review your autosave paths\n- Check available diskspace & filesystem access rights\n- If you are using the ITP format, ensure all instruments exist as independant .iti files");
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


////////////////
// Member access
////////////////


void CAutoSaver::Enable()
//-----------------------
{
	m_bEnabled = true;
}


void CAutoSaver::Disable()
//------------------------
{
	m_bEnabled = false;
}


bool CAutoSaver::IsEnabled()
//--------------------------
{
	return m_bEnabled;
}


void CAutoSaver::SetUseOriginalPath(bool useOrgPath)
//--------------------------------------------------
{
	m_bUseOriginalPath=useOrgPath;
}


bool CAutoSaver::GetUseOriginalPath()
//-----------------------------------
{
	return m_bUseOriginalPath;
}


void CAutoSaver::SetPath(CString path)
//------------------------------------
{
	m_csPath = path;
}


CString CAutoSaver::GetPath()
//---------------------------
{
	return m_csPath;
}


void CAutoSaver::SetFilenameTemplate(CString fnTemplate)
//------------------------------------------------------
{
	m_csFileNameTemplate = fnTemplate;
}


CString CAutoSaver::GetFilenameTemplate()
//---------------------------------------
{
	return m_csFileNameTemplate;
}


void CAutoSaver::SetHistoryDepth(int history)
//-------------------------------------------
{
	Limit(history, 1, 100);
	m_nBackupHistory = history;
}


int CAutoSaver::GetHistoryDepth()
//_------------------------------
{
	return m_nBackupHistory;
}


void CAutoSaver::SetSaveInterval(int minutes)
//-------------------------------------------
{
	Limit(minutes, 1, 10000);
	
	m_nSaveInterval=minutes * 60 * 1000; //minutes to milliseconds
}


int CAutoSaver::GetSaveInterval()
//-------------------------------
{
	return m_nSaveInterval/60/1000;
}


///////////////////////////
// Implementation internals
///////////////////////////


bool CAutoSaver::CheckTimer(DWORD curTime) 
//----------------------------------------
{
	DWORD curInterval = curTime-m_nLastSave;
	return (curInterval >= m_nSaveInterval);
}


CString CAutoSaver::BuildFileName(CModDoc &modDoc)
//------------------------------------------------
{
	CString timeStamp = (CTime::GetCurrentTime()).Format("%Y%m%d.%H%M%S");
	CString name;
	
	if(m_bUseOriginalPath)
	{
		if(modDoc.m_bHasValidPath)
		{
			// Check that the file has a user-chosen path
			name = modDoc.GetPathName(); 
		} else
		{
			// if it doesnt, put it in settings dir
			name = theApp.GetConfigPath() + modDoc.GetTitle();
		}
	} else
	{
		name = m_csPath+modDoc.GetTitle();
	}
	
	name.Append(".AutoSave.");					//append backup tag
	name.Append(timeStamp);						//append timestamp
	name.Append(".");							//append extension
	if(modDoc.GetrSoundFile().m_SongFlags[SONG_ITPROJECT])
	{
		name.Append("itp");
	} else
	{
		name.Append(modDoc.GetrSoundFile().GetModSpecifications().fileExtension);
	}

	return name;
}


bool CAutoSaver::SaveSingleFile(CModDoc &modDoc)
//----------------------------------------------
{
	// We do not call CModDoc::DoSave as this populates the Recent Files
	// list with backups... hence we have duplicated code.. :(
	CSoundFile &sndFile = modDoc.GetrSoundFile(); 
	
	CString fileName = BuildFileName(modDoc);

	// We are acutally not going to show the log for autosaved files.
	ScopedLogCapturer logcapturer(modDoc, "", nullptr, false);

	bool success = false;
	switch(modDoc.GetModType())
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
		success = sndFile.m_SongFlags[SONG_ITPROJECT] ? 
			sndFile.SaveITProject(fileName) :
			sndFile.SaveIT(fileName); 
		break;

	case MOD_TYPE_MPT:
		//Using IT save function also for MPT.
		success = sndFile.SaveIT(fileName);
		break;
	}

	return success;
}


void CAutoSaver::CleanUpBackups(CModDoc &modDoc)
//----------------------------------------------
{
	CString path;
	
	if (m_bUseOriginalPath)
	{
		if (modDoc.m_bHasValidPath)	// Check that the file has a user-chosen path
		{
			CString fullPath = modDoc.GetPathName();
			path = fullPath.Left(fullPath.GetLength() - modDoc.GetTitle().GetLength()); //remove file name if necessary
		} else
		{
			path = theApp.GetConfigPath();
		}
	} else
	{
		path = m_csPath;
	}

	CString searchPattern = path + modDoc.GetTitle() + ".AutoSave.*";

	CFileFind finder;
	BOOL bResult = finder.FindFile(searchPattern);
	vector<CString> foundfiles;
	
	while(bResult)
	{
		bResult = finder.FindNextFile();
		foundfiles.push_back(path + finder.GetFileName());
	}
	finder.Close();
	
	std::sort(foundfiles.begin(), foundfiles.end());
	while(foundfiles.size() > m_nBackupHistory)
	{
		try
		{
			CString toRemove = foundfiles[0];
			CFile::Remove(toRemove);
		} catch (CFileException* /*pEx*/){}
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

CAutoSaverGUI::~CAutoSaverGUI()
{
}

void CAutoSaverGUI::DoDataExchange(CDataExchange* pDX)
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
{
	CPropertyPage::OnInitDialog();

	CheckDlgButton(IDC_AUTOSAVE_ENABLE, m_pAutoSaver->IsEnabled()?BST_CHECKED:BST_UNCHECKED);
	//SetDlgItemText(IDC_AUTOSAVE_FNTEMPLATE, m_pAutoSaver->GetFilenameTemplate());
	SetDlgItemInt(IDC_AUTOSAVE_HISTORY, m_pAutoSaver->GetHistoryDepth()); //TODO
	SetDlgItemText(IDC_AUTOSAVE_PATH, m_pAutoSaver->GetPath());
	SetDlgItemInt(IDC_AUTOSAVE_INTERVAL, m_pAutoSaver->GetSaveInterval());
	CheckDlgButton(IDC_AUTOSAVE_USEORIGDIR, m_pAutoSaver->GetUseOriginalPath()?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_AUTOSAVE_USECUSTOMDIR, m_pAutoSaver->GetUseOriginalPath()?BST_UNCHECKED:BST_CHECKED);

	//enable/disable stuff as appropriate
	OnBnClickedAutosaveEnable();
	OnBnClickedAutosaveUseorigdir();

	return TRUE;
}


void CAutoSaverGUI::OnOK()
{
	CString tempPath;
	IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) ? m_pAutoSaver->Enable() : m_pAutoSaver->Disable();
	m_pAutoSaver->SetFilenameTemplate(""); //TODO
	m_pAutoSaver->SetHistoryDepth(GetDlgItemInt(IDC_AUTOSAVE_HISTORY));
	m_pAutoSaver->SetSaveInterval(GetDlgItemInt(IDC_AUTOSAVE_INTERVAL));
	m_pAutoSaver->SetUseOriginalPath(IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR) == BST_CHECKED);
	GetDlgItemText(IDC_AUTOSAVE_PATH, tempPath);
	if (!tempPath.IsEmpty() && (tempPath.Right(1)!="\\"))
		tempPath.Append("\\");
	m_pAutoSaver->SetPath(tempPath);

	CPropertyPage::OnOK();
}

void CAutoSaverGUI::OnBnClickedAutosaveBrowse()
{
	CHAR szPath[_MAX_PATH] = "";
	BROWSEINFO bi;

	GetDlgItemText(IDC_AUTOSAVE_PATH, szPath, CountOf(szPath));
	MemsetZero(bi);
	bi.hwndOwner = m_hWnd;
	bi.lpszTitle = "Select a folder to store autosaved files in...";
	bi.pszDisplayName = szPath;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	LPITEMIDLIST pid = SHBrowseForFolder(&bi);
	if (pid != NULL)
	{
		SHGetPathFromIDList(pid, szPath);
		SetDlgItemText(IDC_AUTOSAVE_PATH, szPath);
		OnSettingsChanged();
	}
}


void CAutoSaverGUI::OnBnClickedAutosaveEnable()
{
	BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_INTERVAL), enabled);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_HISTORY), enabled);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_USEORIGDIR), enabled);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_USECUSTOMDIR), enabled);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_PATH), enabled);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_BROWSE), enabled);
	OnSettingsChanged();
	return;
}

void CAutoSaverGUI::OnBnClickedAutosaveUseorigdir()
{
	if (IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE)) {
		BOOL enabled = IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_PATH), !enabled);
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_BROWSE), !enabled);
		OnSettingsChanged();
	}
	return;
}

void CAutoSaverGUI::OnSettingsChanged() {
	SetModified(TRUE);
}

BOOL CAutoSaverGUI::OnSetActive()
//--------------------------------
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_AUTOSAVE;
	return CPropertyPage::OnSetActive();
}

BOOL CAutoSaverGUI::OnKillActive() 
//---------------------------------
{
	CString path;
	GetDlgItemText(IDC_AUTOSAVE_PATH, path);
	if (!path.IsEmpty() && (path.Right(1)!="\\")) {
		path.Append("\\");
	}
	bool pathIsOK = !_access(path, 0);

	if (!pathIsOK && IsDlgButtonChecked(IDC_AUTOSAVE_ENABLE) && !IsDlgButtonChecked(IDC_AUTOSAVE_USEORIGDIR))
	{
		Reporting::Error("Error: backup path does not exist.");
		::SetFocus(::GetDlgItem(m_hWnd, IDC_AUTOSAVE_PATH));
		return 0;
	}

	return CPropertyPage::OnKillActive();
}
