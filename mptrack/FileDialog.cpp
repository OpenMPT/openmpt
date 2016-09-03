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
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "../common/StringFixer.h"


OPENMPT_NAMESPACE_BEGIN


// Display the file dialog.
bool FileDialog::Show(const CWnd *parent)
//---------------------------------------
{
	filenames.clear();

	// Convert filter representation to WinAPI style.
	for(size_t i = 0; i < extFilter.length(); i++)
		if(extFilter[i] == L'|') extFilter[i] = 0;
	extFilter.push_back(0);

	// Prepare filename buffer.
	std::vector<WCHAR> filenameBuffer(uint16_max, 0);
	wcscpy(filenameBuffer.data(), defaultFilename.c_str());

	preview = preview && TrackerSettings::Instance().previewInFileDialogs;
	const std::wstring workingDirectoryNative = workingDirectory.AsNative();

	// First, set up the dialog...
	OPENFILENAMEW ofn;
	MemsetZero(ofn);
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = (parent != nullptr ? parent : theApp.m_pMainWnd)->m_hWnd;
	ofn.hInstance = theApp.m_hInstance;
	ofn.lpstrFilter = extFilter.c_str();
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = filterIndex != nullptr ? *filterIndex : 0;
	ofn.lpstrFile = filenameBuffer.data();
	ofn.nMaxFile = filenameBuffer.size();
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = workingDirectory.empty() ? NULL : workingDirectoryNative.c_str();
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (multiSelect ? OFN_ALLOWMULTISELECT : 0) | (load ? 0 : (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN)) | (preview ? OFN_ENABLEHOOK : 0);
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = defaultExtension.empty() ? NULL : defaultExtension.c_str();
	ofn.lCustData = reinterpret_cast<LPARAM>(this);
	ofn.lpfnHook = OFNHookProc;
	ofn.lpTemplateName = NULL;
	ofn.pvReserved = NULL;
	ofn.dwReserved = 0;
	ofn.FlagsEx = 0;

	// Do it!
	CMainFrame::GetInputHandler()->Bypass(true);
	BOOL result = load ? GetOpenFileNameW(&ofn) : GetSaveFileNameW(&ofn);
	CMainFrame::GetInputHandler()->Bypass(false);

	if(stopPreview)
	{
		CMainFrame::GetMainFrame()->StopPreview();
	}

	if(result == FALSE)
	{
		return false;
	}

	// Retrieve variables
	if(filterIndex != nullptr)
		*filterIndex = ofn.nFilterIndex;

	if(multiSelect)
	{
		// Multiple files might have been selected
		const WCHAR *currentFile = ofn.lpstrFile + ofn.nFileOffset;
		std::wstring filePath = ofn.lpstrFile;
		filePath += L"\\";

		while(currentFile[0] != 0)
		{
			int len = lstrlenW(currentFile);
			filePath.resize(ofn.nFileOffset + len);
			lstrcpyW(&filePath[ofn.nFileOffset], currentFile);
			currentFile += len + 1;
			filenames.push_back(mpt::PathString::FromNative(filePath));
		}
	} else
	{
		// Only one file
		filenames.push_back(mpt::PathString::FromNative(ofn.lpstrFile));
	}

	if(filenames.empty())
	{
		return false;
	}

	workingDirectory = mpt::PathString::FromNative(filenames.front().AsNative().substr(0, ofn.nFileOffset));
	extension = mpt::PathString::FromNative(filenames.front().AsNative().substr(ofn.nFileExtension));

	return true;
}


// Callback function for instrument preview
UINT_PTR CALLBACK FileDialog::OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM /*wParam*/, LPARAM lParam)
//------------------------------------------------------------------------------------------------
{
	if(uiMsg == WM_NOTIFY)
	{
		OFNOTIFY *ofn = reinterpret_cast<OFNOTIFY *>(lParam);
		WCHAR path[MAX_PATH];
		if(ofn->hdr.code == CDN_SELCHANGE && CommDlg_OpenSave_GetFilePathW(GetParent(hdlg), path, CountOf(path)) > 0)
		{
			FileDialog *that = reinterpret_cast<FileDialog *>(ofn->lpOFN->lCustData);
			if(path[0] && that->lastPreviewFile != path)
			{
				that->lastPreviewFile = path;
				if(CMainFrame::GetMainFrame()->PlaySoundFile(mpt::PathString::FromNative(path), NOTE_MIDDLEC))
				{
					that->stopPreview = true;
				}
			}
		}
	}
	return 0;
}


// Helper callback to set start path.
int CALLBACK BrowseForFolder::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData)
//------------------------------------------------------------------------------------------------------
{
	if(uMsg == BFFM_INITIALIZED && lpData != NULL)
	{
		const BrowseForFolder *that = reinterpret_cast<BrowseForFolder *>(lpData);
		std::wstring startPath = that->workingDirectory.AsNative();
		SendMessage(hwnd, BFFM_SETSELECTIONW, TRUE, reinterpret_cast<LPARAM>(startPath.c_str()));
	}
	return 0;
}


// Display the folder dialog.
bool BrowseForFolder::Show(const CWnd *parent)
//--------------------------------------------
{
	WCHAR path[MAX_PATH];

	BROWSEINFOW bi;
	MemsetZero(bi);
	bi.hwndOwner = (parent != nullptr ? parent : theApp.m_pMainWnd)->m_hWnd;
	bi.lpszTitle = caption.empty() ? NULL : caption.c_str();
	bi.pszDisplayName = path;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = reinterpret_cast<LPARAM>(this);
	LPITEMIDLIST pid = SHBrowseForFolderW(&bi);
	if(pid != NULL && SHGetPathFromIDListW(pid, path))
	{
		workingDirectory = mpt::PathString::FromNative(path);
		return true;
	}
	return false;
}


OPENMPT_NAMESPACE_END
