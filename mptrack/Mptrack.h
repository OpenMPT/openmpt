// mptrack.h : main header file for the MPTRACK application
//

#if !defined(AFX_MPTRACK_H__AE144DC4_DD0B_11D1_AF24_444553540000__INCLUDED_)
#define AFX_MPTRACK_H__AE144DC4_DD0B_11D1_AF24_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "sndfile.h"
#include <windows.h>

#ifdef UPDATECHECKENABLED
	#include <Specstrings.h>	// In VC2003, '__in' was undefined in winhttp.h
	#include <winhttp.h>
#endif

class CModDoc;
class CVstPluginManager;

/////////////////////////////////////////////////////////////////////////////
// ACM Functions (for dynamic linking)

typedef VOID (ACMAPI *PFNACMMETRICS)(HACMOBJ, UINT, LPVOID);
typedef MMRESULT (ACMAPI *PFNACMFORMATENUM)(HACMDRIVER, LPACMFORMATDETAILSA, ACMFORMATENUMCBA, DWORD dwInstance, DWORD fdwEnum);
typedef MMRESULT (ACMAPI *PFNACMDRIVEROPEN)(LPHACMDRIVER, HACMDRIVERID, DWORD);
typedef MMRESULT (ACMAPI *PFNACMDRIVERCLOSE)(HACMDRIVER, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMOPEN)(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER, DWORD, DWORD, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMCLOSE)(HACMSTREAM, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMSIZE)(HACMSTREAM, DWORD, LPDWORD, DWORD);
typedef MMRESULT (ACMAPI *PFNACMSTREAMCONVERT)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
typedef MMRESULT (ACMAPI *PFNACMDRIVERDETAILS)(HACMDRIVERID, LPACMDRIVERDETAILS, DWORD);


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
	LPSTR MidiMap[128*2];	// 128 instruments + 128 percussions
} MIDILIBSTRUCT, *LPMIDILIBSTRUCT;


/////////////////////////////////////////////////////////////////////////////
// DLS Sound Banks

#define MAX_DLS_BANKS	100 //rewbs.increaseMaxDLSBanks

class CDLSBank;


/////////////////////////////////////////////////////////////////////////
// Chords

typedef struct MPTCHORD
{
	BYTE key;
	BYTE notes[3];
} MPTCHORD, *PMPTCHORD;


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
};


/////////////////////////////////////////////////////////////////////////////
// Internet connection context

#ifdef UPDATECHECKENABLED
typedef struct REQUEST_CONTEXT {
	HINTERNET hSession; 
	HINTERNET hConnection;
	HINTERNET hRequest;
	LPSTR     lpBuffer;       // Buffer for storing read data
	LPSTR	  postData;
} REQUEST_CONTEXT;
#endif

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
	static UINT m_nDefaultDocType;
	static LPMIDILIBSTRUCT glpMidiLibrary;
	static BOOL m_nProject;

public:
	static MEMORYSTATUS gMemStatus;
	static CDLSBank *gpDLSBanks[MAX_DLS_BANKS];

protected:
	CMultiDocTemplate *m_pModTemplate;
	CVstPluginManager *m_pPluginManager;
	BOOL m_bInitialized, m_bLayer3Present, m_bExWaveSupport, m_bDebugMode;
	DWORD m_dwTimeStarted, m_dwLastPluginIdleCall;
	HANDLE m_hAlternateResourceHandle;
	// Default macro configuration
	MODMIDICFG m_MidiCfg;
	CHAR m_szConfigFileName[_MAX_PATH];
	CHAR m_szPluginCacheFileName[_MAX_PATH];
	CHAR m_szStringsFileName[_MAX_PATH];

	#ifdef UPDATECHECKENABLED
	// Internet request context
	REQUEST_CONTEXT *m_pRequestContext;
	#endif

public:
	CTrackApp();

public:

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	static BOOL IsProject() { return m_nProject; }
	static VOID SetAsProject(BOOL n) { m_nProject = n; }
// -! NEW_FEATURE#0023

	static UINT GetDefaultDocType() { return m_nDefaultDocType; }
	static VOID SetDefaultDocType(UINT n) { m_nDefaultDocType = n; }
	static LPMIDILIBSTRUCT GetMidiLibrary() { return glpMidiLibrary; }
	static BOOL ImportMidiConfig(LPCSTR lpszFileName, BOOL bNoWarning=FALSE);
	static BOOL ExportMidiConfig(LPCSTR lpszFileName);
	static void RegisterExtensions();
	static BOOL LoadDefaultDLSBanks();
	static BOOL SaveDefaultDLSBanks();
	static BOOL RemoveDLSBank(UINT nBank);
	static BOOL AddDLSBank(LPCSTR);
	static BOOL OpenURL(LPCSTR lpszURL);

public:
	CDocTemplate *GetModDocTemplate() const { return m_pModTemplate; }
	CVstPluginManager *GetPluginManager() const { return m_pPluginManager; }
	void GetDefaultMidiMacro(MODMIDICFG *pcfg) const { *pcfg = m_MidiCfg; }
	void SetDefaultMidiMacro(const MODMIDICFG *pcfg) { m_MidiCfg = *pcfg; }
	void LoadChords(PMPTCHORD pChords);
	void SaveChords(PMPTCHORD pChords);
	BOOL CanEncodeLayer3() const { return m_bLayer3Present; }
	BOOL IsWaveExEnabled() const { return m_bExWaveSupport; }
	BOOL IsDebug() const { return m_bDebugMode; }
	LPCSTR GetConfigFileName() const { return m_szConfigFileName; }
	LPCSTR GetPluginCacheFileName() const { return m_szPluginCacheFileName; }

// Splash Screen
protected:
	VOID StartSplashScreen();
	VOID StopSplashScreen();

	#ifdef UPDATECHECKENABLED
		VOID UpdateCheck();
		static void __stdcall InternetRequestCallback( HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus,
									  LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
		static void CleanupInternetRequest(REQUEST_CONTEXT *pRequestContext);
	#endif

// Localized strings
public:
	VOID ImportLocalizedStrings();
	BOOL GetLocalizedString(LPCSTR pszName, LPSTR pszStr, UINT cbSize);

// ACM and MPEG Layer3 support
protected:
	HINSTANCE m_hACMInst;
	HINSTANCE m_hBladeEnc, m_hLameEnc;
	PFNACMFORMATENUM m_pfnAcmFormatEnum;
	
public:
	BOOL InitializeACM(BOOL bNoAcm=FALSE);
	BOOL UninitializeACM();
	BOOL InitializeDXPlugins();
	BOOL UninitializeDXPlugins();
	static void AcmExceptionHandler();
	MMRESULT AcmFormatEnum(HACMDRIVER had, LPACMFORMATDETAILSA pafd, ACMFORMATENUMCBA fnCallback, DWORD dwInstance, DWORD fdwEnum);
	MMRESULT AcmDriverOpen(LPHACMDRIVER, HACMDRIVERID, DWORD);
	MMRESULT AcmDriverDetails(HACMDRIVERID hadid, LPACMDRIVERDETAILS padd, DWORD fdwDetails);
	MMRESULT AcmDriverClose(HACMDRIVER, DWORD);
	MMRESULT AcmStreamOpen(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER pwfltr, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
	MMRESULT AcmStreamClose(HACMSTREAM, DWORD);
	MMRESULT AcmStreamSize(HACMSTREAM has, DWORD cbInput, LPDWORD pdwOutputBytes, DWORD fdwSize);
	MMRESULT AcmStreamPrepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare);
	MMRESULT AcmStreamUnprepareHeader(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwUnprepare);
	MMRESULT AcmStreamConvert(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert);

protected:
	static BOOL CALLBACK AcmFormatEnumCB(HACMDRIVERID, LPACMFORMATDETAILS, DWORD, DWORD);

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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);

private:
	static void LoadRegistryDLS();
};


//=============================
class CButtonEx: public CButton
//=============================
{
protected:
	MODPLUGDIB m_Dib;
	RECT m_srcRect;
	BOOL m_bPushed;

public:
	CButtonEx() { m_Dib.lpDibBits = NULL; m_bPushed = FALSE; }
	BOOL Init(const LPMODPLUGDIB pDib, COLORREF colorkey=RGB(0,128,128));
	BOOL SetSourcePos(int x, int y=0, int cx=16, int cy=15);
	BOOL AlignButton(UINT nIdPrev, int dx=0);
	BOOL AlignButton(const CWnd &wnd, int dx=0) { return AlignButton(wnd.m_hWnd, dx); }
	BOOL AlignButton(HWND hwnd, int dx=0);
	BOOL GetPushState() const { return m_bPushed; }
	void SetPushState(BOOL bPushed);

protected:
	//{{AFX_VIRTUAL(CButtonEx)
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL
	//{{AFX_MSG(CButtonEx)
	afx_msg BOOL OnEraseBkgnd(CDC *) { return TRUE; }
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();
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

#define FASTBMP_XSHIFT			12	// 4K pixels
#define FASTBMP_MAXWIDTH		(1 << FASTBMP_XSHIFT)
#define FASTBMP_MAXHEIGHT		16

typedef struct MODPLUGFASTDIB
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
	BYTE DibBits[FASTBMP_MAXWIDTH*FASTBMP_MAXHEIGHT];
} MODPLUGFASTDIB, *LPMODPLUGFASTDIB;

//===============
class CFastBitmap
//===============
{
protected:
	MODPLUGFASTDIB m_Dib;
	UINT m_nTextColor, m_nBkColor;
	LPMODPLUGDIB m_pTextDib;
	BOOL m_nBlendOffset;
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
	void SetBlendMode(UINT nBlendOfs) { m_nBlendOffset = nBlendOfs; }
	void SetBlendColor(COLORREF cr);
};


///////////////////////////////////////////////////
// 4-bit DIB Drawing functions
void DibBlt(HDC hdc, int x, int y, int sizex, int sizey, int srcx, int srcy, LPMODPLUGDIB lpdib);
LPMODPLUGDIB LoadDib(LPCSTR lpszName);
RGBQUAD rgb2quad(COLORREF c);

// Other bitmap functions
void DrawBitmapButton(HDC hdc, LPRECT lpRect, LPMODPLUGDIB lpdib, int srcx, int srcy, BOOL bPushed);
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

// Append note names from zero-based range [noteStart, noteEnd] to given combobox.
void AppendNotesToControl(CComboBox& combobox, const MODCOMMAND::NOTE noteStart, const MODCOMMAND::NOTE noteEnd);

// Append note names to combobox. If pSndFile != nullPrt, appends only notes that are 
// available in the module type.
void AppendNotesToControl(CComboBox& combobox, const CSoundFile* const pSndFile = nullptr);

// Append note names of instrument iInstr in rSndFile to combobox. 
void AppendNotesToControl(CComboBox& combobox, const CSoundFile& rSndFile, const INSTRUMENTINDEX iInstr);

// Returns note name(such as "C-5") of given note using note indecing [1, NOTE_MAX].
const char* GetNoteStr(const MODCOMMAND::NOTE);

///////////////////////////////////////////////////
// Tables
#define MAX_EFFECTS		35	//rewbs.smoothVST & rewbs.velocity: increased from 32. Wonder what this will break...
							//+1 for eric's multiplier
#define MAX_VOLCMDS		16	//rewbs.voloff & rewbs.velocity: increased from 14

extern BYTE gEffectColors[MAX_EFFECTS];
extern LPCSTR szNoteNames[12];
extern LPCSTR szDefaultNoteNames[NOTE_MAX];
LPCSTR const szSpecialNoteNames[NOTE_MAX_SPECIAL-NOTE_MIN_SPECIAL + 1] = {"PCs", "PC", "^^", "=="};
LPCSTR const szSpecialNoteShortDesc[NOTE_MAX_SPECIAL-NOTE_MIN_SPECIAL + 1] = {"Param control(smooth)", "Param control", "Note Cut", "Note Off"};

// Make sure that szSpecialNoteNames-array includes note string for every special note.
STATIC_ASSERT(NOTE_MAX_SPECIAL - NOTE_MIN_SPECIAL + 1 == ARRAYELEMCOUNT(szSpecialNoteNames)); 
STATIC_ASSERT(ARRAYELEMCOUNT(szSpecialNoteShortDesc) == ARRAYELEMCOUNT(szSpecialNoteNames)); 

extern LPCSTR szHexChar;
extern LPCSTR gszModCommands;
extern LPCSTR gszS3mCommands;
extern LPCSTR gszVolCommands;

// Defined in load_mid.cpp
extern LPCSTR szMidiProgramNames[128];
extern LPCSTR szMidiPercussionNames[61]; // notes 25..85
extern LPCSTR szMidiGroupNames[17];		// 16 groups + Percussions

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MPTRACK_H__AE144DC4_DD0B_11D1_AF24_444553540000__INCLUDED_)
