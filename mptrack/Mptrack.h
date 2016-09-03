/*
 * MPTrack.h
 * ---------
 * Purpose: OpenMPT core application class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "resource.h"       // main symbols
#include "Settings.h"
#include <windows.h>
#include "../mptrack/MpTrackUtil.h"
#include "../mptrack/Reporting.h"
#include "../soundlib/MIDIMacros.h"
#include "../soundlib/modcommand.h"
#include "../common/ComponentManager.h"
#include "../common/mptMutex.h"
#include "../common/mptRandom.h"
#include <vector>

OPENMPT_NAMESPACE_BEGIN

class CModDoc;
class CVstPluginManager;
namespace SoundDevice {
class Manager;
} // namespace SoundDevice
class CDLSBank;
class DebugSettings;
class TrackerSettings;
class ComponentManagerSettings;
namespace mpt { namespace Wine {
class VersionContext;
} } // namespace mpt::Wine


/////////////////////////////////////////////////////////////////////////////
// 16-colors DIB
typedef struct MODPLUGDIB
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[16];
	LPBYTE lpDibBits;
} MODPLUGDIB;


/////////////////////////////////////////////////////////////////////////////
// Midi Library

struct MIDILIBSTRUCT
{
	mpt::PathString MidiMap[128*2];	// 128 instruments + 128 percussions
};


//////////////////////////////////////////////////////////////////////////
// Dragon Droppings

enum DragonDropType
{
	DRAGONDROP_NOTHING=0,	// |------< Drop Type >-------------|--< dwDropItem >---|--< lDropParam >---|
	DRAGONDROP_DLS,			// | Instrument from a DLS bank     |     DLS Bank #    |   DLS Instrument  |
	DRAGONDROP_SAMPLE,		// | Sample from a song             |     Sample #      |       NULL        |
	DRAGONDROP_INSTRUMENT,	// | Instrument from a song         |     Instrument #  |       NULL        |
	DRAGONDROP_SOUNDFILE,	// | File from instrument library   |        ?          |     File Name     |
	DRAGONDROP_MIDIINSTR,	// | File from midi library         | Midi Program/Perc |     File Name     |
	DRAGONDROP_PATTERN,		// | Pattern from a song            |      Pattern #    |       NULL        |
	DRAGONDROP_ORDER,		// | Pattern index in a song        |       Order #     |       NULL        |
	DRAGONDROP_SONG,		// | Song file (mod/s3m/xm/it)      |       0           |     File Name     |
	DRAGONDROP_SEQUENCE		// | Sequence (a set of orders)     |    Sequence #     |       NULL        |
};

struct DRAGONDROP
{
	CModDoc *pModDoc;
	DragonDropType dwDropType;
	DWORD dwDropItem;
	LPARAM lDropParam;

	mpt::PathString GetPath() const
	{
		const mpt::PathString *const pPath = reinterpret_cast<const mpt::PathString *>(lDropParam);
		ASSERT(pPath);
		if(!pPath)
		{
			return mpt::PathString();
		}
		return *pPath;
	}
};


/////////////////////////////////////////////////////////////////////////////
// Document Template

// OK, this is a really dirty hack for ANSI builds (which are still used because the code is not yet UNICODE compatible).
// To support wide path names even in ansi builds, we use mpt::PathString everywhere.
// Except, there is one set of cases, where MFC dictates TCHAR/CString on us: The filename handling for MDI documents.
// Here, if in ANSI build, we encode the wide string in utf8 and pass it around through MFC. When MFC calls us again,
// we unpack it again. This works surprisingly well for the hackish nature this has.
// Rough edges:
//  - CDocument::GetTitle is still ANSI encoded and returns replacement chars for non-representable chars.
//  - CWinApp::AddToRecentFileList chokes on filenames it cannot access. We simply avoid passing non-ansi-representable filenames there.
// Modified MFC functionality:
//  CTrackApp:
//   CWinApp::OpenDocument
//   CWinApp::AddToRecentFileList
//  CModDocManager:
//   CDocManager::OpenDocumentFile
//  CModDocTemplate:
//   CMultiDocTemplate::OpenDocumentFile
//  CModDoc:
//   CDocument::GetPathName
//   CDocument::SetPathName
//   CDocument::DoFileSave
//   CDocument::DoSave
//   CDocument::OnOpenDocument
//   CDocument::OnSaveDocument

//=============================================
class CModDocTemplate: public CMultiDocTemplate
//=============================================
{
public:
	CModDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass):
		CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}

	CDocument* OpenDocumentFile(const mpt::PathString &filename, BOOL addToMru = TRUE, BOOL makeVisible = TRUE);
	CDocument* OpenTemplateFile(const mpt::PathString &filename, bool isExampleTune = false);

	// inherited members, overload them all
	MPT_DEPRECATED_PATH virtual CDocument* OpenDocumentFile(LPCTSTR path, BOOL addToMru = TRUE, BOOL makeVisible = TRUE)
	{
		return OpenDocumentFile(path ? mpt::PathString::TunnelOutofCString(path) : mpt::PathString(), addToMru, makeVisible);
	}
};


/////////////////////////////////////////////////////////////////////////////
// CTrackApp:
// See mptrack.cpp for the implementation of this class
//

class ComponentUXTheme : public ComponentSystemDLL
{
	MPT_DECLARE_COMPONENT_MEMBERS
public:
	ComponentUXTheme() : ComponentSystemDLL(MPT_PATHSTRING("uxtheme")) { }
};

//=============================
class CTrackApp: public CWinApp
//=============================
{
	friend class CMainFrame;
// static data
protected:
	static MODTYPE m_nDefaultDocType;
	static MIDILIBSTRUCT midiLibrary;

public:
	static std::vector<CDLSBank *> gpDLSBanks;

protected:

	mpt::recursive_mutex_with_lock_count m_GlobalMutex;

	DWORD m_GuiThreadId;

	mpt::scoped_ptr<mpt::random_device> m_RD;
	mpt::scoped_ptr<mpt::thread_safe_prng<mpt::best_prng> > m_BestPRNG;
	mpt::scoped_ptr<mpt::thread_safe_prng<mpt::prng> > m_PRNG;

	std::shared_ptr<mpt::Wine::VersionContext> m_WineVersion;

	IniFileSettingsBackend *m_pSettingsIniFile;
	SettingsContainer *m_pSettings;
	DebugSettings *m_pDebugSettings;
	TrackerSettings *m_pTrackerSettings;
	IniFileSettingsBackend *m_pSongSettingsIniFile;
	SettingsContainer *m_pSongSettings;
	ComponentManagerSettings *m_pComponentManagerSettings;
	IniFileSettingsContainer *m_pPluginCache;
	CModDocTemplate *m_pModTemplate;
	CVstPluginManager *m_pPluginManager;
	SoundDevice::Manager *m_pSoundDevicesManager;
	mpt::PathString m_szExePath;
	mpt::PathString m_szConfigDirectory;
	mpt::PathString m_szConfigFileName;
	mpt::PathString m_szPluginCacheFileName;
	// Default macro configuration
	MIDIMacroConfig m_MidiCfg;
	DWORD m_dwLastPluginIdleCall;
	bool m_bPortableMode;

public:
	CTrackApp();

	MPT_DEPRECATED_PATH virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU = TRUE)
	{
		return CWinApp::OpenDocumentFile(lpszFileName, bAddToMRU);
	}
	virtual CDocument* OpenDocumentFile(const mpt::PathString &filename, BOOL bAddToMRU = TRUE)
	{
		return CWinApp::OpenDocumentFile(filename.empty() ? NULL : mpt::PathString::TunnelIntoCString(filename).GetString(), bAddToMRU);
	}

	MPT_DEPRECATED_PATH virtual void AddToRecentFileList(LPCTSTR lpszPathName);
	void AddToRecentFileList(const mpt::PathString path);
	/// Removes item from MRU-list; most recent item has index zero.
	void RemoveMruItem(const size_t item);
	void RemoveMruItem(const mpt::PathString &path);

public:

	mpt::PathString GetAppDirPath() {return m_szExePath;} // Returns '\'-ended executable directory path.
	static MODTYPE GetDefaultDocType() { return m_nDefaultDocType; }
	static void SetDefaultDocType(MODTYPE n) { m_nDefaultDocType = n; }
	static MIDILIBSTRUCT &GetMidiLibrary() { return midiLibrary; }
	static BOOL ImportMidiConfig(const mpt::PathString &filename, BOOL bNoWarning=FALSE);
	static BOOL ExportMidiConfig(const mpt::PathString &filename);
	static BOOL ImportMidiConfig(SettingsContainer &file, bool forgetSettings = false);
	static BOOL ExportMidiConfig(SettingsContainer &file);
	static BOOL LoadDefaultDLSBanks();
	static BOOL SaveDefaultDLSBanks();
	static BOOL RemoveDLSBank(UINT nBank);
	static BOOL AddDLSBank(const mpt::PathString &filename);
	static bool OpenURL(const char *url); // UTF8
	static bool OpenURL(const std::string &url); // UTF8
	static bool OpenURL(const CString &url);
	static bool OpenURL(const mpt::ustring &url);
	static bool OpenURL(const mpt::PathString &lpszURL);
	static bool OpenFile(const mpt::PathString &file) { return OpenURL(file); };
	static bool OpenDirectory(const mpt::PathString &directory) { return OpenURL(directory); };

	int GetOpenDocumentCount() const;
	std::vector<CModDoc *> GetOpenDocuments() const;

public:
	inline mpt::recursive_mutex_with_lock_count & GetGlobalMutexRef() { return m_GlobalMutex; }
	bool InGuiThread() const { return GetCurrentThreadId() == m_GuiThreadId; }
	mpt::random_device & RandomDevice() { return *m_RD; }
	mpt::thread_safe_prng<mpt::best_prng> & BestPRNG() { return *m_BestPRNG; }
	mpt::thread_safe_prng<mpt::prng> & PRNG() { return *m_PRNG; }
	CModDocTemplate *GetModDocTemplate() const { return m_pModTemplate; }
	CVstPluginManager *GetPluginManager() const { return m_pPluginManager; }
	SoundDevice::Manager *GetSoundDevicesManager() const { return m_pSoundDevicesManager; }
	void GetDefaultMidiMacro(MIDIMacroConfig &cfg) const { cfg = m_MidiCfg; }
	void SetDefaultMidiMacro(const MIDIMacroConfig &cfg) { m_MidiCfg = cfg; }
	mpt::PathString GetConfigFileName() const { return m_szConfigFileName; }
	SettingsContainer & GetSettings()
	{
		ASSERT(m_pSettings);
		return *m_pSettings;
	}
	TrackerSettings & GetTrackerSettings()
	{
		ASSERT(m_pTrackerSettings);
		return *m_pTrackerSettings;
	}
	bool IsPortableMode() { return m_bPortableMode; }
	SettingsContainer & GetPluginCache()
	{
		ASSERT(m_pPluginCache);
		return *m_pPluginCache;
	}

	SettingsContainer & GetSongSettings()
	{
		ASSERT(m_pSongSettings);
		return *m_pSongSettings;
	}
	const mpt::PathString& GetSongSettingsFilename() const
	{
		return m_pSongSettingsIniFile->GetFilename();
	}

	void SetWineVersion(std::shared_ptr<mpt::Wine::VersionContext> wineVersion)
	{
		m_WineVersion = wineVersion;
	}
	std::shared_ptr<mpt::Wine::VersionContext> GetWineVersion() const
	{
		return m_WineVersion;
	}

	/// Returns path to config folder including trailing '\'.
	mpt::PathString GetConfigPath() const { return m_szConfigDirectory; }
	void SetupPaths(bool overridePortable);

	CString SuggestModernBuildText();
	bool CheckSystemSupport();

	// Relative / absolute paths conversion
	mpt::PathString AbsolutePathToRelative(const mpt::PathString &path) { return path.AbsolutePathToRelative(GetAppDirPath()); }
	mpt::PathString RelativePathToAbsolute(const mpt::PathString &path) { return path.RelativePathToAbsolute(GetAppDirPath()); }

	static void OpenModulesDialog(std::vector<mpt::PathString> &files, const mpt::PathString &overridePath = mpt::PathString());

public:
	// Get name of resampling mode. addTaps = true also adds the number of taps the filter uses.
	static const TCHAR *GetResamplingModeName(ResamplingMode mode, bool addTaps);

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrackApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTrackApp)
	afx_msg void OnFileNew();
	afx_msg void OnFileNewMOD();
	afx_msg void OnFileNewS3M();
	afx_msg void OnFileNewXM();
	afx_msg void OnFileNewIT();
	afx_msg void OnFileNewMPT();

	afx_msg void OnFileOpen();
	afx_msg void OnAppAbout();

	afx_msg void OnFileCloseAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	BOOL InitializeDXPlugins();
	BOOL UninitializeDXPlugins();


	bool MoveConfigFile(mpt::PathString sFileName, mpt::PathString sSubDir = mpt::PathString(), mpt::PathString sNewFileName = mpt::PathString());

};


extern CTrackApp theApp;


//////////////////////////////////////////////////////////////////
// More Bitmap Helpers

//===============
class CFastBitmap
//===============
{
protected:
	struct MODPLUGFASTDIB
	{
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[256];
		std::vector<uint8_t> DibBits;
	};

	MODPLUGFASTDIB m_Dib;
	UINT m_nTextColor, m_nBkColor;
	MODPLUGDIB *m_pTextDib;
	uint8_t m_nBlendOffset;
	uint8_t m_n4BitPalette[16];
	uint8_t m_nXShiftFactor;

public:
	CFastBitmap() {}

public:
	void Init(MODPLUGDIB *lpTextDib = nullptr);
	void Blit(HDC hdc, int x, int y, int cx, int cy);
	void Blit(HDC hdc, LPCRECT lprc) { Blit(hdc, lprc->left, lprc->top, lprc->right-lprc->left, lprc->bottom-lprc->top); }
	void SetTextColor(int nText, int nBk=-1) { m_nTextColor = nText; if (nBk >= 0) m_nBkColor = nBk; }
	void SetTextBkColor(UINT nBk) { m_nBkColor = nBk; }
	void SetColor(UINT nIndex, COLORREF cr);
	void SetAllColors(UINT nBaseIndex, UINT nColors, COLORREF *pcr);
	void TextBlt(int x, int y, int cx, int cy, int srcx, int srcy, MODPLUGDIB *lpdib = nullptr);
	void SetBlendMode(BYTE nBlendOfs) { m_nBlendOffset = nBlendOfs; }
	void SetBlendColor(COLORREF cr);
	void SetSize(int x, int y);
	int GetWidth() const { return m_Dib.bmiHeader.biWidth; }
};


///////////////////////////////////////////////////
// 4-bit DIB Drawing functions
void DibBlt(HDC hdc, int x, int y, int sizex, int sizey, int srcx, int srcy, MODPLUGDIB *lpdib);
MODPLUGDIB *LoadDib(LPCTSTR lpszName);
RGBQUAD rgb2quad(COLORREF c);

// Other bitmap functions
void DrawButtonRect(HDC hdc, LPRECT lpRect, LPCSTR lpszText=NULL, BOOL bDisabled=FALSE, BOOL bPushed=FALSE, DWORD dwFlags=(DT_CENTER|DT_VCENTER));

// Misc functions
UINT MsgBox(UINT nStringID, CWnd *p=NULL, LPCSTR lpszTitle=NULL, UINT n=MB_OK);
void ErrorBox(UINT nStringID, CWnd*p=NULL);

// Helper function declarations.
struct SNDMIXPLUGIN;
class IMixPlugin;
void AddPluginNamesToCombobox(CComboBox& CBox, const SNDMIXPLUGIN *plugarray, const bool librarynames = false);
void AddPluginParameternamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN& plugarray);
void AddPluginParameternamesToCombobox(CComboBox& CBox, IMixPlugin& plug);

// Append note names in range [noteStart, noteEnd] to given combobox. Index starts from 0.
void AppendNotesToControl(CComboBox& combobox, ModCommand::NOTE noteStart, ModCommand::NOTE noteEnd);

// Append note names to combo box.
// If nInstr is given, instrument-specific note names are used instead of default note names.
// A custom note range may also be specified using the noteStart and noteEnd parameters.
// If they are left out, only notes that are available in the module type, plus any supported "special notes" are added.
void AppendNotesToControlEx(CComboBox& combobox, const CSoundFile &sndFile, INSTRUMENTINDEX nInstr = MAX_INSTRUMENTS, ModCommand::NOTE noteStart = 0, ModCommand::NOTE noteEnd = 0);

// Get window text (e.g. edit box conent) as a wide string
std::wstring GetWindowTextW(CWnd &wnd);

///////////////////////////////////////////////////
// Tables

extern const TCHAR *szSpecialNoteNamesMPT[];
extern const TCHAR *szSpecialNoteShortDesc[];
extern const char *szHexChar;

// Defined in load_mid.cpp
extern const char *szMidiProgramNames[128];
extern const char *szMidiPercussionNames[61];	// notes 25..85
extern const char *szMidiGroupNames[17];		// 16 groups + Percussions

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


OPENMPT_NAMESPACE_END
