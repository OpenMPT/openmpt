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
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "TrackerSettings.h"
#include "mpt/fs/fs.hpp"


OPENMPT_NAMESPACE_BEGIN

enum class FileDialogType
{
	OpenFile,
	SaveFile,
	BrowseForFolder,
};

class CFileDialogEx : public CFileDialog
{
public:
	CFileDialogEx(FileDialogType type,
		LPCTSTR lpszDefExt,
		LPCTSTR lpszFileName,
		DWORD dwFlags,
		LPCTSTR lpszFilter,
		CWnd *pParentWnd,
		DWORD dwSize,
		BOOL bVistaStyle,
		bool preview)
		: CFileDialog((type != FileDialogType::SaveFile) ? TRUE : FALSE, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
		, m_fileNameBuf(65536)
		, doPreview(preview)
	{
		// MFC's filename buffer is way too small for multi-selections of a large number of files.
		_tcsncpy(m_fileNameBuf.data(), lpszFileName, m_fileNameBuf.size());
		m_fileNameBuf.back() = '\0';
		m_ofn.lpstrFile = m_fileNameBuf.data();
		m_ofn.nMaxFile = mpt::saturate_cast<DWORD>(m_fileNameBuf.size());

		if(type == FileDialogType::BrowseForFolder)
		{
			m_bPickFoldersMode = TRUE;
			m_bPickNonFileSysFoldersMode = FALSE;
		}

#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
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
			AddPlace(place->GetDefaultDir());
		}
#endif
	}

	~CFileDialogEx()
	{
		if(played)
		{
			CMainFrame::GetMainFrame()->StopPreview();
		}
	}

#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	// MFC's AddPlace() is declared as throw() but can in fact throw if any of the COM calls fail, e.g. because the place does not exist.
	// Avoid this by re-implementing our own version which doesn't throw.
	void AddPlace(const mpt::PathString &path)
	{
		if(m_bVistaStyle && mpt::native_fs{}.is_directory(path))
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

	static bool CanUseModernStyle()
	{
		return !mpt::OS::Windows::IsWine() && mpt::osinfo::windows::Version::Current().IsAtLeast(mpt::osinfo::windows::Version::WinVista);
	}

protected:
	std::vector<TCHAR> m_fileNameBuf;
	CString oldName;
	const bool doPreview;
	bool played = false;

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
	CFileDialogEx dlg(m_load ? FileDialogType::OpenFile : FileDialogType::SaveFile,
		m_defaultExtension.empty() ? nullptr : m_defaultExtension.c_str(),
		m_defaultFilename.c_str(),
		OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (m_multiSelect ? OFN_ALLOWMULTISELECT : 0) | (m_load ? 0 : (OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN)),
		m_extFilter.c_str(),
		parent != nullptr ? parent : CMainFrame::GetMainFrame(),
		0,
		CFileDialogEx::CanUseModernStyle() ? TRUE : FALSE,
		m_preview && TrackerSettings::Instance().previewInFileDialogs);
	OPENFILENAME &ofn = dlg.GetOFN();
	ofn.nFilterIndex = m_filterIndex != nullptr ? *m_filterIndex : 0;
	if(!m_workingDirectory.empty())
	{
		ofn.lpstrInitialDir = m_workingDirectory.c_str();
	}
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
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
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
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

	// Don't rely on nFileOffset / nFileExtension - it was observed on Windows 10 / 11 that when opening a .lnk file,
	// those offsets are in terms of the .lnk file, while lpstrFile contains the resolved target of that link.
	m_workingDirectory = m_filenames.front().GetDirectoryWithDrive();
	m_extension = m_filenames.front().GetFilenameExtension();
	if(!m_extension.empty())
		m_extension.erase(0, 1);

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

	// Note: MFC's CFolderPickerDialog won't work on pre-Vista systems, as it tries to use OPENFILENAME unconditionally.
	if(CFileDialogEx::CanUseModernStyle() && !TrackerSettings::Instance().useOldStyleFolderBrowser)
	{
		CFileDialogEx dlg{FileDialogType::BrowseForFolder, nullptr, m_workingDirectory.AsNative().c_str(), 0, nullptr, parent, 0, TRUE, false};
		dlg.m_ofn.lpstrTitle = m_caption;
		if(dlg.DoModal() != IDOK)
			return false;
		m_workingDirectory = mpt::PathString::FromCString(dlg.GetPathName());
		return true;
	}

	TCHAR path[MAX_PATH];
	BROWSEINFO bi{};
	bi.hwndOwner = (parent != nullptr ? parent : theApp.m_pMainWnd)->m_hWnd;
	if(!m_caption.IsEmpty())
		bi.lpszTitle = m_caption;
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
