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
		, m_fileNameBuf(65536)
		, doPreview(preview)
		, played(false)
	{
		// MFC's filename buffer is way too small for multi-selections of a large number of files.
		_tcsncpy(m_fileNameBuf.data(), lpszFileName, m_fileNameBuf.size());
		m_fileNameBuf.back() = '\0';
		m_ofn.lpstrFile = m_fileNameBuf.data();
		m_ofn.nMaxFile = mpt::saturate_cast<DWORD>(m_fileNameBuf.size());
	}

	~CFileDialogEx()
	{
		if(played)
		{
			CMainFrame::GetMainFrame()->StopPreview();
		}
	}

#if NTDDI_VERSION >= NTDDI_VISTA
	// MFC's AddPlace() is declared as throw() but can in fact throw if any of the COM calls fail, e.g. because the place does not exist.
	// Avoid this by re-implementing our own version which doesn't throw.
	void AddPlace(const mpt::PathString &path)
	{
		if(m_bVistaStyle && path.IsDirectory())
		{
			CComPtr<IShellItem> shellItem;
			HRESULT hr = SHCreateItemFromParsingName(path.ToWide().c_str(), nullptr, IID_IShellItem, reinterpret_cast<void **>(&shellItem));
			if(SUCCEEDED(hr))
			{
				static_cast<IFileDialog*>(m_pIFileDialog)->AddPlace(shellItem, FDAP_TOP);
			}
		}
	}
#endif

protected:
	std::vector<TCHAR> m_fileNameBuf;
	CString oldName;
	bool doPreview, played;

	void OnFileNameChange() override
	{
		if(doPreview)
		{
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
		CFileDialog::OnFileNameChange();
	}
};


// Display the file dialog.
bool FileDialog::Show(CWnd *parent)
{
	m_filenames.clear();

	// First, set up the dialog...
	CFileDialogEx dlg(m_load,
		m_defaultExtension.empty() ? nullptr : m_defaultExtension.c_str(),
		m_defaultFilename.c_str(),
		OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (m_multiSelect ? OFN_ALLOWMULTISELECT : 0) | (m_load ? 0 : (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN)),
		m_extFilter.c_str(),
		parent != nullptr ? parent : CMainFrame::GetMainFrame(),
		0,
		(mpt::OS::Windows::IsWine() || mpt::OS::Windows::Version::Current().IsBefore(mpt::OS::Windows::Version::WinVista)) ? FALSE : TRUE,
		m_preview && TrackerSettings::Instance().previewInFileDialogs);
	OPENFILENAME &ofn = dlg.GetOFN();
	ofn.nFilterIndex = m_filterIndex != nullptr ? *m_filterIndex : 0;
	if(!m_workingDirectory.empty())
	{
		ofn.lpstrInitialDir = m_workingDirectory.c_str();
	}
#if NTDDI_VERSION >= NTDDI_VISTA
	const auto places =
	{
		&TrackerSettings::Instance().PathPluginPresets,
		&TrackerSettings::Instance().PathPlugins,
		&TrackerSettings::Instance().PathSamples,
		&TrackerSettings::Instance().PathInstruments,
		&TrackerSettings::Instance().PathSongs,
	};
	for(const auto place : places)
	{
		dlg.AddPlace(place->GetDefaultDir());
	}
	for(const auto &place : m_places)
	{
		dlg.AddPlace(place);
	}
#endif

	// Do it!
	BypassInputHandler bih;
	if(dlg.DoModal() != IDOK)
	{
		return false;
	}

	// Retrieve variables
	if(m_filterIndex != nullptr)
		*m_filterIndex = ofn.nFilterIndex;

	if(m_multiSelect)
	{
#if NTDDI_VERSION >= NTDDI_VISTA
		// Multiple files might have been selected
		if(CComPtr<IShellItemArray> shellItems = dlg.GetResults(); shellItems != nullptr)
		{
			// Using the old-style GetNextPathName doesn't work properly when the user performs a search and selects files from different folders.
			// Hence we use that only as a fallback.
			DWORD numItems = 0;
			shellItems->GetCount(&numItems);
			for(DWORD i = 0; i < numItems; i++)
			{
				CComPtr<IShellItem> shellItem;
				shellItems->GetItemAt(i, &shellItem);

				LPWSTR filePath = nullptr;
				if(HRESULT hr = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath); SUCCEEDED(hr))
				{
					m_filenames.push_back(mpt::PathString::FromWide(filePath));
					::CoTaskMemFree(filePath);
				}
			}
		} else
#endif
		{
			POSITION pos = dlg.GetStartPosition();
			while(pos != nullptr)
			{
				m_filenames.push_back(mpt::PathString::FromCString(dlg.GetNextPathName(pos)));
			}
		}
	} else
	{
		// Only one file
		m_filenames.push_back(mpt::PathString::FromCString(dlg.GetPathName()));
	}

	if(m_filenames.empty())
	{
		return false;
	}

	m_workingDirectory = m_filenames.front().AsNative().substr(0, ofn.nFileOffset);
	m_extension = m_filenames.front().AsNative().substr(ofn.nFileExtension);

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
	// Note: MFC's CFolderPickerDialog won't work on pre-Vista systems, as it tries to use OPENFILENAME.
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
	LPITEMIDLIST pid = SHBrowseForFolder(&bi);
	bool success = pid != nullptr && SHGetPathFromIDList(pid, path);
	CoTaskMemFree(pid);
	if(success)
	{
		m_workingDirectory = mpt::PathString::FromNative(path);
	}
	return success;
}


OPENMPT_NAMESPACE_END
