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
#include <vector>

class CModDoc;
class CVstPluginManager;
class SoundDevicesManager;
class CDLSBank;
class TrackerDirectories;
class TrackerSettings;

/////////////////////////////////////////////////////////////////////////////
// 16-colors DIB
typedef struct MODPLUGDIB
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[16];
	LPBYTE lpDibBits;
} MODPLUGDIB, *LPMODPLUGDIB;


/////////////////////////////////////////////////////////////////////////////
// Midi Library

struct MIDILIBSTRUCT
{
	mpt::PathString MidiMap[128*2];	// 128 instruments + 128 percussions
};


//////////////////////////////////////////////////////////////////////////
// Dragon Droppings

struct DRAGONDROP
{
	CModDoc *pModDoc;
	DWORD dwDropType;
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

enum {
	DRAGONDROP_NOTHING=0,	// |------< Drop Type >-------------|--< dwDropItem >---|--< lDropParam >---|
	DRAGONDROP_DLS,			// | Instrument from a DLS bank		|	  DLS Bank #	|	DLS Instrument	|
	DRAGONDROP_SAMPLE,		// | Sample from a song				|     Sample #		|	    NULL		|
	DRAGONDROP_INSTRUMENT,	// | Instrument from a song			|	  Instrument #	|	    NULL		|
	DRAGONDROP_SOUNDFILE,	// | File from instrument library	|		?			|	pszFileName		|
	DRAGONDROP_MIDIINSTR,	// | File from midi library			| Midi Program/Perc	|	pszFileName		|
	DRAGONDROP_PATTERN,		// | Pattern from a song			|      Pattern #    |       NULL        |
	DRAGONDROP_ORDER,		// | Pattern index in a song		|       Order #     |       NULL        |
	DRAGONDROP_SONG,		// | Song file (mod/s3m/xm/it)		|		0			|	pszFileName		|
	DRAGONDROP_SEQUENCE		// | Sequence (a set of orders)		|    Sequence #     |       NULL        |
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

	// inherited members, overload them all
	#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
		MPT_DEPRECATED_PATH virtual CDocument* OpenDocumentFile(LPCTSTR path, BOOL makeVisible = TRUE)
		{
			return OpenDocumentFile(path ? mpt::PathString::TunnelOutofCString(path) : mpt::PathString(), TRUE, makeVisible);
		}
	#else
		MPT_DEPRECATED_PATH virtual CDocument* OpenDocumentFile(LPCTSTR path, BOOL addToMru = TRUE, BOOL makeVisible = TRUE)
		{
			return OpenDocumentFile(path ? mpt::PathString::TunnelOutofCString(path) : mpt::PathString(), addToMru, makeVisible);
		}
	#endif
};


/////////////////////////////////////////////////////////////////////////////
// CTrackApp:
// See mptrack.cpp for the implementation of this class
//

//=============================
class CTrackApp: public CWinApp
//=============================
{
	friend class CMainFrame;
// static data
protected:
	static MODTYPE m_nDefaultDocType;
	static BOOL m_nProject;
	static MIDILIBSTRUCT midiLibrary;

public:
	static std::vector<CDLSBank *> gpDLSBanks;

#if MPT_COMPILER_MSVC && MPT_MSVC_BEFORE(2010,0)
	MPT_DEPRECATED_PATH virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU = TRUE)
	{
		return OpenDocumentFile(lpszFileName ? mpt::PathString::TunnelOutofCString(lpszFileName) : mpt::PathString(), bAddToMRU);
	}
	virtual CDocument* OpenDocumentFile(const mpt::PathString &filename, BOOL bAddToMRU = TRUE)
	{
		CDocument* pDoc = CWinApp::OpenDocumentFile(filename.empty() ? NULL : mpt::PathString::TunnelIntoCString(filename).GetString());
		if (pDoc && bAddToMRU != TRUE)
			RemoveMruItem(0); // This doesn't result to the same behaviour as not adding to MRU 
							  // (if the new item got added, it might have already dropped the last item out)
		return pDoc;
	}
#else
	MPT_DEPRECATED_PATH virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU = TRUE)
	{
		return CWinApp::OpenDocumentFile(lpszFileName, bAddToMRU);
	}
	virtual CDocument* OpenDocumentFile(const mpt::PathString &filename, BOOL bAddToMRU = TRUE)
	{
		return CWinApp::OpenDocumentFile(filename.empty() ? NULL : mpt::PathString::TunnelIntoCString(filename).GetString(), bAddToMRU);
	}
#endif

	MPT_DEPRECATED_PATH virtual void AddToRecentFileList(LPCTSTR lpszPathName);

protected:

	DWORD m_GuiThreadId;

	TrackerDirectories *m_pTrackerDirectories;
	IniFileSettingsBackend *m_pSettingsIniFile;
	SettingsContainer *m_pSettings;
	TrackerSettings *m_pTrackerSettings;
	IniFileSettingsContainer *m_pPluginCache;
	CModDocTemplate *m_pModTemplate;
	CVstPluginManager *m_pPluginManager;
	SoundDevicesManager *m_pSoundDevicesManager;
	BOOL m_bInitialized;
	DWORD m_dwTimeStarted, m_dwLastPluginIdleCall;
	// Default macro configuration
	MIDIMacroConfig m_MidiCfg;
	static mpt::PathString m_szExePath;
	mpt::PathString m_szConfigDirectory;
	mpt::PathString m_szConfigFileName;
	mpt::PathString m_szPluginCacheFileName;
	bool m_bPortableMode;

public:
	CTrackApp();

public:

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	static BOOL IsProject() { return m_nProject; }
	static VOID SetAsProject(BOOL n) { m_nProject = n; }
// -! NEW_FEATURE#0023

	static mpt::PathString GetAppDirPath() {return m_szExePath;} // Returns '\'-ended executable directory path.
	static MODTYPE GetDefaultDocType() { return m_nDefaultDocType; }
	static void SetDefaultDocType(MODTYPE n) { m_nDefaultDocType = n; }
	static MIDILIBSTRUCT &GetMidiLibrary() { return midiLibrary; }
	static BOOL ImportMidiConfig(const mpt::PathString &filename, BOOL bNoWarning=FALSE);
	static BOOL ExportMidiConfig(const mpt::PathString &filename);
	static BOOL ImportMidiConfig(SettingsContainer &file, bool forgetSettings = false);
	static BOOL ExportMidiConfig(SettingsContainer &file);
	static void RegisterExtensions();
	static BOOL LoadDefaultDLSBanks();
	static BOOL SaveDefaultDLSBanks();
	static BOOL RemoveDLSBank(UINT nBank);
	static BOOL AddDLSBank(const mpt::PathString &filename);
	static bool OpenURL(const char *url);
	static bool OpenURL(const std::string &url);
	static bool OpenURL(const CString &url);
	static bool OpenURL(const mpt::PathString &lpszURL);
	static bool OpenFile(const mpt::PathString &file) { return OpenURL(file); };
	static bool OpenDirectory(const mpt::PathString &directory) { return OpenURL(directory); };

	int GetOpenDocumentCount() const;
	std::vector<CModDoc *>GetOpenDocuments() const;

public:
	bool InGuiThread() const { return GetCurrentThreadId() == m_GuiThreadId; }
	CModDocTemplate *GetModDocTemplate() const { return m_pModTemplate; }
	CVstPluginManager *GetPluginManager() const { return m_pPluginManager; }
	SoundDevicesManager *GetSoundDevicesManager() const { return m_pSoundDevicesManager; }
	void GetDefaultMidiMacro(MIDIMacroConfig &cfg) const { cfg = m_MidiCfg; }
	void SetDefaultMidiMacro(const MIDIMacroConfig &cfg) { m_MidiCfg = cfg; }
	mpt::PathString GetConfigFileName() const { return m_szConfigFileName; }
	TrackerDirectories & GetTrackerDirectories()
	{
		ASSERT(m_pTrackerDirectories);
		return *m_pTrackerDirectories;
	}
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

	/// Returns path to config folder including trailing '\'.
	mpt::PathString GetConfigPath() const { return m_szConfigDirectory; }
	void SetupPaths(bool overridePortable);

	// Relative / absolute paths conversion
	mpt::PathString AbsolutePathToRelative(const mpt::PathString &path) { return path.AbsolutePathToRelative(GetAppDirPath()); }
	mpt::PathString RelativePathToAbsolute(const mpt::PathString &path) { return path.RelativePathToAbsolute(GetAppDirPath()); }

	/// Removes item from MRU-list; most recent item has index zero.
	void RemoveMruItem(const int nItem);

// Splash Screen
protected:
	void StartSplashScreen();
	void StopSplashScreen();

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
// -> CODE#0023
// -> DESC="IT project files (.itp)"
	afx_msg void OnFileNewITProject();
// -! NEW_FEATURE#0023

	afx_msg void OnFileOpen();
	afx_msg void OnAppAbout();

	afx_msg void OnFileCloseAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	static void LoadRegistryDLS();

protected:
	BOOL InitializeDXPlugins();
	BOOL UninitializeDXPlugins();


#ifdef WIN32	// Legacy stuff
	bool MoveConfigFile(mpt::PathString sFileName, mpt::PathString sSubDir = mpt::PathString(), mpt::PathString sNewFileName = mpt::PathString());
#endif // WIN32

};


extern CTrackApp theApp;


//////////////////////////////////////////////////////////////////
// More Bitmap Helpers

//#define FASTBMP_XSHIFT		12	// 4K pixels
#define FASTBMP_XSHIFT			13	// 8K pixels
#define FASTBMP_MAXWIDTH		(1 << FASTBMP_XSHIFT)
#define FASTBMP_MAXHEIGHT		16

struct MODPLUGFASTDIB
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
	BYTE DibBits[FASTBMP_MAXWIDTH*FASTBMP_MAXHEIGHT];
};

//===============
class CFastBitmap
//===============
{
protected:
	MODPLUGFASTDIB m_Dib;
	UINT m_nTextColor, m_nBkColor;
	LPMODPLUGDIB m_pTextDib;
	BYTE m_nBlendOffset;
	BYTE m_n4BitPalette[16];

public:
	CFastBitmap() {}

public:
	void Init(LPMODPLUGDIB lpTextDib=NULL);
	void Blit(HDC hdc, int x, int y, int cx, int cy);
	void Blit(HDC hdc, LPCRECT lprc) { Blit(hdc, lprc->left, lprc->top, lprc->right-lprc->left, lprc->bottom-lprc->top); }
	void SetTextColor(int nText, int nBk=-1) { m_nTextColor = nText; if (nBk >= 0) m_nBkColor = nBk; }
	void SetTextBkColor(UINT nBk) { m_nBkColor = nBk; }
	void SetColor(UINT nIndex, COLORREF cr);
	void SetAllColors(UINT nBaseIndex, UINT nColors, COLORREF *pcr);
	void TextBlt(int x, int y, int cx, int cy, int srcx, int srcy, LPMODPLUGDIB lpdib=NULL);
	void SetBlendMode(BYTE nBlendOfs) { m_nBlendOffset = nBlendOfs; }
	void SetBlendColor(COLORREF cr);
};


///////////////////////////////////////////////////
// 4-bit DIB Drawing functions
void DibBlt(HDC hdc, int x, int y, int sizex, int sizey, int srcx, int srcy, LPMODPLUGDIB lpdib);
LPMODPLUGDIB LoadDib(LPCSTR lpszName);
RGBQUAD rgb2quad(COLORREF c);

// Other bitmap functions
void DrawButtonRect(HDC hdc, LPRECT lpRect, LPCSTR lpszText=NULL, BOOL bDisabled=FALSE, BOOL bPushed=FALSE, DWORD dwFlags=(DT_CENTER|DT_VCENTER));

// Misc functions
class CVstPlugin;
UINT MsgBox(UINT nStringID, CWnd *p=NULL, LPCSTR lpszTitle=NULL, UINT n=MB_OK);
void ErrorBox(UINT nStringID, CWnd*p=NULL);

// Helper function declarations.
struct SNDMIXPLUGIN;
class CVstPlugin;
void AddPluginNamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN* plugarray, const bool librarynames = false);
void AddPluginParameternamesToCombobox(CComboBox& CBox, SNDMIXPLUGIN& plugarray);
void AddPluginParameternamesToCombobox(CComboBox& CBox, CVstPlugin& plug);

// Append note names in range [noteStart, noteEnd] to given combobox. Index starts from 0.
void AppendNotesToControl(CComboBox& combobox, const ModCommand::NOTE noteStart, const ModCommand::NOTE noteEnd);

// Append note names to combobox. If pSndFile != nullprt, appends only notes that are 
// available in the module type. If nInstr is given, instrument specific note names are used instead of
// default note names.
void AppendNotesToControlEx(CComboBox& combobox, const CSoundFile* const pSndFile = nullptr, const INSTRUMENTINDEX nInstr = MAX_INSTRUMENTS);

// Returns note name(such as "C-5") of given note. Regular notes are in range [1,MAX_NOTE].
LPCTSTR GetNoteStr(const ModCommand::NOTE);

///////////////////////////////////////////////////
// Tables

//const LPCTSTR szSpecialNoteNames[] = {TEXT("PCs"), TEXT("PC"), TEXT("~~"), TEXT("^^"), TEXT("==")};
const LPCTSTR szSpecialNoteNames[] = {TEXT("PCs"), TEXT("PC"), TEXT("~~ (Note Fade)"), TEXT("^^ (Note Cut)"), TEXT("== (Note Off)")};
const LPCTSTR szSpecialNoteShortDesc[] = {TEXT("Param Control (Smooth)"), TEXT("Param Control"), TEXT("Note Fade"), TEXT("Note Cut"), TEXT("Note Off")};

// Make sure that special note arrays include string for every note.
STATIC_ASSERT(NOTE_MAX_SPECIAL - NOTE_MIN_SPECIAL + 1 == CountOf(szSpecialNoteNames)); 
STATIC_ASSERT(CountOf(szSpecialNoteShortDesc) == CountOf(szSpecialNoteNames)); 

const LPCSTR szHexChar = "0123456789ABCDEF";

// Defined in load_mid.cpp
extern const char *szMidiProgramNames[128];
extern const char *szMidiPercussionNames[61];	// notes 25..85
extern const char *szMidiGroupNames[17];		// 16 groups + Percussions

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
