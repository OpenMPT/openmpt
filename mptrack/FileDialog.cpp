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
#include "../common/StringFixer.h"


// Display the file dialog.
bool FileDialog::Show()
//---------------------
{
	filenames.clear();

	// Convert filter representation to WinAPI style.
	for(size_t i = 0; i < extFilter.length(); i++)
		if(extFilter[i] == '|') extFilter[i] = 0;
	extFilter.push_back(0);

	// Prepare filename buffer.
	std::vector<TCHAR> filenameBuffer(uint16_max, 0);
	filenameBuffer.insert(filenameBuffer.begin(), defaultFilename.begin(), defaultFilename.end());
	filenameBuffer.push_back(0);

	// First, set up the dialog...
	OPENFILENAME ofn;
	MemsetZero(ofn);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = theApp.m_pMainWnd->GetSafeHwnd();
	ofn.hInstance = theApp.m_hInstance;
	ofn.lpstrFilter = extFilter.c_str();
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = filterIndex != nullptr ? *filterIndex : 0;
	ofn.lpstrFile = &filenameBuffer[0];
	ofn.nMaxFile = filenameBuffer.size();
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = workingDirectory.empty() ? NULL : workingDirectory.c_str();
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (multiSelect ? OFN_ALLOWMULTISELECT : 0) | (load ? 0 : (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN));
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = defaultExtension.empty() ? NULL : defaultExtension.c_str();
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	ofn.pvReserved = NULL;
	ofn.dwReserved = 0;
	ofn.FlagsEx = 0;

	// Do it!
	CMainFrame::GetInputHandler()->Bypass(true);
	BOOL result = load ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn);
	CMainFrame::GetInputHandler()->Bypass(false);

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
		int pos = ofn.nFileOffset;
		const TCHAR *currentFile = ofn.lpstrFile + pos;
		TCHAR filePath[MAX_PATH + 1];
		lstrcpy(filePath, ofn.lpstrFile);
		lstrcat(filePath, "\\");

		while(currentFile[0] != 0)
		{
			lstrcpy(&filePath[ofn.nFileOffset], currentFile);
			currentFile += lstrlen(currentFile) + 1;
			filenames.push_back(filePath);
		}
	} else
	{
		// Only one file
		filenames.push_back(ofn.lpstrFile);
	}

	if(filenames.empty())
	{
		return false;
	}

	workingDirectory = filenames.front().substr(0, ofn.nFileOffset);
	defaultExtension = filenames.front().substr(ofn.nFileExtension);

	return true;
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
bool BrowseForFolder::Show()
//--------------------------
{
	WCHAR path[MAX_PATH];

	BROWSEINFOW bi;
	MemsetZero(bi);
	bi.hwndOwner = theApp.m_pMainWnd->GetSafeHwnd();
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
