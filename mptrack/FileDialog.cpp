/*
 * FileDialog.cpp
 * --------------
 * Purpose: File and folder selection dialogs implementation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "FileDialog.h"
#include "Mainfrm.h"
#include "InputHandler.h"


OPENMPT_NAMESPACE_BEGIN

class CFileDialogEx : public CFileDialog
{
public:
	CFileDialogEx(bool bOpenFileDialog,
		LPCTSTR lpszDefExt,
		LPCTSTR lpszFileName,
		DWORD dwFlags,
		LPCTSTR lpszFilter,
		CWnd *pParentWnd,
		DWORD dwSize,
		BOOL bVistaStyle,
		bool preview)
		: CFileDialog(bOpenFileDialog ? TRUE : FALSE, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
		, doPreview(preview)
		, played(false)
	{ }

	~CFileDialogEx()
	{
		if(played)
		{
			CMainFrame::GetMainFrame()->StopPreview();
		}
	}

protected:
	CString oldName;
	bool doPreview, played;

	void OnFileNameChange() override
	{
		if(!m_bOpenFileDialog || !doPreview)
		{
			CFileDialog::OnFileNameChange();
			return;
		}
		CString name = GetPathName();
		if(!name.IsEmpty() && name != oldName)
		{
			oldName = name;
			if(CMainFrame::GetMainFrame()->PlaySoundFile(mpt::PathString::FromCString(name), NOTE_MIDDLEC))
			{
				played = true;
			}
		}
	}
};


// Display the file dialog.
bool FileDialog::Show(CWnd *parent)
{
	filenames.clear();

	// First, set up the dialog...
	CFileDialogEx dlg(load,
		defaultExtension.empty() ? nullptr : defaultExtension.c_str(),
		defaultFilename.c_str(),
		OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (multiSelect ? OFN_ALLOWMULTISELECT : 0) | (load ? 0 : (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN)),
		extFilter.c_str(),
		parent != nullptr ? parent : CMainFrame::GetMainFrame(),
		0,
		TRUE,
		preview && TrackerSettings::Instance().previewInFileDialogs);
	OPENFILENAME &ofn = dlg.GetOFN();
	ofn.nFilterIndex = filterIndex != nullptr ? *filterIndex : 0;
	ofn.lpstrInitialDir = workingDirectory.empty() ? nullptr : workingDirectory.AsNative().c_str();

	// Do it!
	BypassInputHandler bih;
	if(dlg.DoModal() != IDOK)
	{
		return false;
	}

	// Retrieve variables
	if(filterIndex != nullptr)
		*filterIndex = ofn.nFilterIndex;

	if(multiSelect)
	{
		// Multiple files might have been selected
		POSITION pos = dlg.GetStartPosition();
		while(pos != nullptr)
		{
			filenames.push_back(mpt::PathString::FromCString(dlg.GetNextPathName(pos)));
		}
	} else
	{
		// Only one file
		filenames.push_back(mpt::PathString::FromCString(dlg.GetPathName()));
	}

	if(filenames.empty())
	{
		return false;
	}

	workingDirectory = mpt::PathString::FromNative(filenames.front().AsNative().substr(0, ofn.nFileOffset));
	extension = mpt::PathString::FromNative(filenames.front().AsNative().substr(ofn.nFileExtension));

	return true;
}


// Helper callback to set start path.
int CALLBACK BrowseForFolder::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
{
	if(uMsg == BFFM_INITIALIZED && lpData != NULL)
	{
		const BrowseForFolder *that = reinterpret_cast<BrowseForFolder *>(lpData);
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(that->m_workingDirectory.AsNative().c_str()));
	}
	return 0;
}


// Display the folder dialog.
bool BrowseForFolder::Show(CWnd *parent)
{
	BypassInputHandler bih;
	TCHAR path[MAX_PATH];
	BROWSEINFO bi;
	MemsetZero(bi);
	bi.hwndOwner = (parent != nullptr ? parent : theApp.m_pMainWnd)->m_hWnd;
	if(!m_caption.IsEmpty()) bi.lpszTitle = m_caption;
	bi.pszDisplayName = path;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = reinterpret_cast<LPARAM>(this);
	ITEMIDLIST *pid = SHBrowseForFolder(&bi);
	bool success = pid != nullptr && SHGetPathFromIDList(pid, path);
	CoTaskMemFree(pid);
	if(success)
	{
		m_workingDirectory = mpt::PathString::FromNative(path);
	}
	return success;
}


OPENMPT_NAMESPACE_END
