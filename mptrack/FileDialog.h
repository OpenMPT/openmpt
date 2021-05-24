/*
 * FileDialog.h
 * ------------
 * Purpose: File and folder selection dialogs implementation.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include <string>
#include <vector>

OPENMPT_NAMESPACE_BEGIN

// Generic open / save file dialog. Cannot be instanced by the user, use OpenFileDialog / SaveFileDialog instead.
class FileDialog
{
public:
	using PathList = std::vector<mpt::PathString>;

protected:
	mpt::RawPathString m_defaultExtension;
	mpt::RawPathString m_defaultFilename;
	mpt::RawPathString m_extFilter;
	mpt::RawPathString m_lastPreviewFile;
	mpt::RawPathString m_workingDirectory;
	mpt::RawPathString m_extension;
	PathList m_filenames;
	PathList m_places;
	int *m_filterIndex = nullptr;
	bool m_load;
	bool m_multiSelect = false;
	bool m_preview = false;

protected:
	FileDialog(bool load) : m_load(load) { }

public:
	// Default extension to use if none is specified.
	FileDialog &DefaultExtension(const mpt::PathString &ext) { m_defaultExtension = ext.AsNative(); return *this; }
	FileDialog &DefaultExtension(const AnyStringLocale &ext) { m_defaultExtension = mpt::ToWin(ext); return *this; }
	// Default suggested filename.
	FileDialog &DefaultFilename(const mpt::PathString &name) { m_defaultFilename = name.AsNative(); return *this; }
	FileDialog &DefaultFilename(const AnyStringLocale &name) { m_defaultFilename = mpt::ToWin(name); return *this; }
	// List of possible extensions. Format: "description|extensions|...|description|extensions||"
	FileDialog &ExtensionFilter(const mpt::PathString &filter) { m_extFilter = filter.AsNative(); return *this; }
	FileDialog &ExtensionFilter(const AnyStringLocale &filter) { m_extFilter = mpt::ToWin(filter); return *this; }
	// Default directory of the dialog.
	FileDialog &WorkingDirectory(const mpt::PathString &dir) { m_workingDirectory = dir.AsNative(); return *this; }
	// Pointer to a variable holding the index of the last extension filter to use. Holds the selected filter after the dialog has been closed.
	FileDialog &FilterIndex(int *index) { m_filterIndex = index; return *this; }
	// Enable preview of instrument files (if globally enabled).
	FileDialog &EnableAudioPreview() { m_preview = true; return *this; }
	// Add a directory to the application-specific quick-access directories in the file dialog
	FileDialog &AddPlace(mpt::PathString path) { m_places.push_back(std::move(path)); return *this; }

	// Show the file selection dialog.
	bool Show(CWnd *parent = nullptr);

	// Get some selected file. Mostly useful when only one selected file is possible anyway.
	mpt::PathString GetFirstFile() const
	{
		if(!m_filenames.empty())
			return m_filenames.front();
		else
			return {};
	}
	// Gets a reference to all selected filenames.
	const PathList &GetFilenames() const { return m_filenames; }
	// Gets directory in which the selected files are placed.
	mpt::PathString GetWorkingDirectory() const { return mpt::PathString::FromNative(m_workingDirectory); }
	// Gets the extension of the first selected file, without dot.
	mpt::PathString GetExtension() const { return mpt::PathString::FromNative(m_extension); }
};


// Dialog for opening files
class OpenFileDialog : public FileDialog
{
public:
	OpenFileDialog() : FileDialog(true) { }

	// Enable selection of multiple files
	OpenFileDialog &AllowMultiSelect() { m_multiSelect = true; return *this; }
};


// Dialog for saving files
class SaveFileDialog : public FileDialog
{
public:
	SaveFileDialog() : FileDialog(false) { }
};


// Folder browser.
class BrowseForFolder
{
protected:
	mpt::PathString m_workingDirectory;
	CString m_caption;

public:
	BrowseForFolder(const mpt::PathString &dir, const CString &caption) : m_workingDirectory(dir), m_caption(caption) { }

	// Show the folder selection dialog.
	bool Show(CWnd *parent = nullptr);

	// Gets selected directory.
	mpt::PathString GetDirectory() const { return m_workingDirectory; }

protected:
	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
};

OPENMPT_NAMESPACE_END
