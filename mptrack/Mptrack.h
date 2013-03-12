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
#include "../soundlib/Sndfile.h"
#include "ACMConvert.h"
#include <windows.h>
#include "../common/Reporting.h"

class CModDoc;
class CVstPluginManager;
class CDLSBank;

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

typedef struct MIDILIBSTRUCT
{
	LPTSTR MidiMap[128*2];	// 128 instruments + 128 percussions
} MIDILIBSTRUCT, *LPMIDILIBSTRUCT;


//////////////////////////////////////////////////////////////////////////
// Dragon Droppings

typedef struct DRAGONDROP
{
	CModDoc *pModDoc;
	DWORD dwDropType;
	DWORD dwDropItem;
	LPARAM lDropParam;
} DRAGONDROP, *LPDRAGONDROP;

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
// File dialog (open/save) results
struct FileDlgResult
{
	std::string workingDirectory;			// working directory. will include filename, so beware.
	std::string first_file;					// for some convenience, this will keep the first filename of the filenames vector.
	std::vector <std::string> filenames;	// all selected filenames in one vector.
	std::string extension;					// extension used. beware of this when multiple files can be selected!
	bool abort;								// no selection has been made.
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
	static LPMIDILIBSTRUCT glpMidiLibrary;
	static BOOL m_nProject;

public:
	static MEMORYSTATUS gMemStatus;
	static vector<CDLSBank *> gpDLSBanks;

#if (_MSC_VER < MSVC_VER_2010)
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU = TRUE)
	{
		CDocument* pDoc = CWinApp::OpenDocumentFile(lpszFileName);
		if (pDoc && bAddToMRU != TRUE)
			RemoveMruItem(0); // This doesn't result to the same behaviour as not adding to MRU 
							  // (if the new item got added, it might have already dropped the last item out)
		return pDoc;
	}
#endif

protected:
	CMultiDocTemplate *m_pModTemplate;
	CVstPluginManager *m_pPluginManager;
	BOOL m_bInitialized, m_bExWaveSupport, m_bDebugMode;
	DWORD m_dwTimeStarted, m_dwLastPluginIdleCall;
	HANDLE m_hAlternateResourceHandle;
	// Default macro configuration
	MIDIMacroConfig m_MidiCfg;
	static TCHAR m_szExePath[_MAX_PATH];
	TCHAR m_szConfigDirectory[_MAX_PATH];
	TCHAR m_szConfigFileName[_MAX_PATH];
	TCHAR m_szPluginCacheFileName[_MAX_PATH];
	TCHAR m_szStringsFileName[_MAX_PATH];
	bool m_bPortableMode;
	ACMConvert acmConvert;

public:
	CTrackApp();

public:

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	static BOOL IsProject() { return m_nProject; }
	static VOID SetAsProject(BOOL n) { m_nProject = n; }
// -! NEW_FEATURE#0023

	static LPCTSTR GetAppDirPath() {return m_szExePath;} // Returns '\'-ended executable directory path.
	static MODTYPE GetDefaultDocType() { return m_nDefaultDocType; }
	static void SetDefaultDocType(MODTYPE n) { m_nDefaultDocType = n; }
	static LPMIDILIBSTRUCT GetMidiLibrary() { return glpMidiLibrary; }
	static BOOL ImportMidiConfig(LPCSTR lpszFileName, BOOL bNoWarning=FALSE);
	static BOOL ExportMidiConfig(LPCSTR lpszFileName);
	static void RegisterExtensions();
	static BOOL LoadDefaultDLSBanks();
	static BOOL SaveDefaultDLSBanks();
	static BOOL RemoveDLSBank(UINT nBank);
	static BOOL AddDLSBank(LPCSTR);
	static bool OpenURL(const LPCSTR lpszURL);
	static bool OpenFile(const LPCSTR file) { return OpenURL(file); };
	static bool OpenDirectory(const LPCSTR directory) { return OpenURL(directory); };

	static FileDlgResult ShowOpenSaveFileDialog(const bool load, const std::string defaultExtension, const std::string defaultFilename, const std::string extFilter, const std::string workingDirectory = "", const bool allowMultiSelect = false, int *filterIndex = nullptr);

	int GetOpenDocumentCount() const;
	vector<CModDoc *>GetOpenDocuments() const;

public:
	CDocTemplate *GetModDocTemplate() const { return m_pModTemplate; }
	CVstPluginManager *GetPluginManager() const { return m_pPluginManager; }
	void GetDefaultMidiMacro(MIDIMacroConfig *pcfg) const { *pcfg = m_MidiCfg; }
	void SetDefaultMidiMacro(const MIDIMacroConfig *pcfg) { m_MidiCfg = *pcfg; }
	BOOL CanEncodeLayer3() const { return acmConvert.IsLayer3Present(); }
	BOOL IsWaveExEnabled() const { return m_bExWaveSupport; }
	BOOL IsDebug() const { return m_bDebugMode; }
	LPCTSTR GetConfigFileName() const { return m_szConfigFileName; }
	bool IsPortableMode() { return m_bPortableMode; }
	LPCTSTR GetPluginCacheFileName() const { return m_szPluginCacheFileName; }

	/// Returns path to config folder including trailing '\'.
	LPCTSTR GetConfigPath() const { return m_szConfigDirectory; }
	void SetupPaths(bool overridePortable);
	// Relative / absolute paths conversion
	template <size_t nLength>
	void AbsolutePathToRelative(TCHAR (&szPath)[nLength]);
	template <size_t nLength>
	void RelativePathToAbsolute(TCHAR (&szPath)[nLength]);

	/// Removes item from MRU-list; most recent item has index zero.
	void RemoveMruItem(const int nItem);

	ACMConvert &GetACMConvert() { return acmConvert; };

// Splash Screen
protected:
	VOID StartSplashScreen();
	VOID StopSplashScreen();

// Localized strings
public:
	VOID ImportLocalizedStrings();
	BOOL GetLocalizedString(LPCSTR pszName, LPSTR pszStr, UINT cbSize);

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
	afx_msg void OnHelpSearch();

	afx_msg void OnFileCloseAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	static void LoadRegistryDLS();

protected:
	BOOL InitializeDXPlugins();
	BOOL UninitializeDXPlugins();


#ifdef WIN32	// Legacy stuff
	bool MoveConfigFile(TCHAR sFileName[_MAX_PATH], TCHAR sSubDir[_MAX_PATH] = _T(""), TCHAR sNewFileName[_MAX_PATH] = _T(""));
#endif // WIN32

};


extern CTrackApp theApp;


//////////////////////////////////////////////////////////////////
// File Mapping Class

//===============
class CMappedFile
//===============
{
protected:
	CFile m_File;
	HANDLE m_hFMap;
	LPVOID m_lpData;

public:
	CMappedFile();
	virtual ~CMappedFile();

public:
	BOOL Open(LPCSTR lpszFileName);
	void Close();
	DWORD GetLength();
	LPBYTE Lock(DWORD dwMaxLen=0);
	BOOL Unlock();
};


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
void Log(LPCSTR format,...);
UINT MsgBox(UINT nStringID, CWnd *p=NULL, LPCSTR lpszTitle=NULL, UINT n=MB_OK);
void ErrorBox(UINT nStringID, CWnd*p=NULL);

// Helper function declarations.
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

extern const LPCSTR szNoteNames[12];
extern const LPCTSTR szDefaultNoteNames[NOTE_MAX];
//const LPCTSTR szSpecialNoteNames[] = {TEXT("PCs"), TEXT("PC"), TEXT("~~"), TEXT("^^"), TEXT("==")};
const LPCTSTR szSpecialNoteNames[] = {TEXT("PCs"), TEXT("PC"), TEXT("~~ (Note Fade)"), TEXT("^^ (Note Cut)"), TEXT("== (Note Off)")};
const LPCTSTR szSpecialNoteShortDesc[] = {TEXT("Param Control (Smooth)"), TEXT("Param Control"), TEXT("Note Fade"), TEXT("Note Cut"), TEXT("Note Off")};

// Make sure that special note arrays include string for every note.
STATIC_ASSERT(NOTE_MAX_SPECIAL - NOTE_MIN_SPECIAL + 1 == CountOf(szSpecialNoteNames)); 
STATIC_ASSERT(CountOf(szSpecialNoteShortDesc) == CountOf(szSpecialNoteNames)); 

const LPCSTR szHexChar = "0123456789ABCDEF";
const TCHAR gszEmpty[] = TEXT("");

// Defined in load_mid.cpp
extern const LPCSTR szMidiProgramNames[128];
extern const LPCSTR szMidiPercussionNames[61]; // notes 25..85
extern const LPCSTR szMidiGroupNames[17];		// 16 groups + Percussions

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
