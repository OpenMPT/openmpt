// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_MAINFRM_H__AE144DC8_DD0B_11D1_AF24_444553540000__INCLUDED_)
#define AFX_MAINFRM_H__AE144DC8_DD0B_11D1_AF24_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sndfile.h"
#include "CommandSet.h"
#include "inputhandler.h"
#include "mptrack.h"

class CInputHandler;
class CMainFrame;
class CModDoc;
class CAutoSaver;
class ISoundDevice;
class ISoundSource;
class CPerformanceCounter;

#define NUM_AUDIO_BUFFERS			3
#define MIN_AUDIO_BUFFERSIZE		1024
#define MAX_AUDIO_BUFFERSIZE		32768	// 32K buffers max
#define MAINFRAME_TITLE				"Open Modplug Tracker"
#define INFORMAL_VERSION			"1.17RC3_"
#define INIBUFFERSIZE				MAX_PATH

enum {
	CTRLMSG_BASE=0,
	CTRLMSG_SETVIEWWND,
	CTRLMSG_ACTIVATEPAGE,
	CTRLMSG_DEACTIVATEPAGE,
	CTRLMSG_SETFOCUS,
	// Pattern-Specific
	CTRLMSG_SETCURRENTPATTERN,
	CTRLMSG_GETCURRENTPATTERN,
	CTRLMSG_SETCURRENTORDER,
	CTRLMSG_GETCURRENTORDER,
	CTRLMSG_FORCEREFRESH,
	CTRLMSG_PAT_PREVINSTRUMENT,
	CTRLMSG_PAT_NEXTINSTRUMENT,
	CTRLMSG_PAT_SETINSTRUMENT,
	CTRLMSG_PAT_FOLLOWSONG,		//rewbs.customKeys
	CTRLMSG_PAT_LOOP,
	CTRLMSG_PAT_NEWPATTERN,		//rewbs.customKeys
	CTRLMSG_SETUPMACROS,
	CTRLMSG_GETCURRENTINSTRUMENT,
	CTRLMSG_SETCURRENTINSTRUMENT,
	CTRLMSG_PLAYPATTERN,
	CTRLMSG_GETSPACING,
	CTRLMSG_SETSPACING,
	CTRLMSG_ISRECORDING,
	CTRLMSG_PATTERNCHANGED,
	CTRLMSG_PREVORDER,
	CTRLMSG_NEXTORDER,
	CTRLMSG_SETRECORD,
	// Sample-Specific
	CTRLMSG_SMP_PREVINSTRUMENT,
	CTRLMSG_SMP_NEXTINSTRUMENT,
	CTRLMSG_SMP_OPENFILE,
	CTRLMSG_SMP_SETZOOM,
	CTRLMSG_SMP_GETZOOM,
	CTRLMSG_SMP_SONGDROP,
	// Instrument-Specific
	CTRLMSG_INS_PREVINSTRUMENT,
	CTRLMSG_INS_NEXTINSTRUMENT,
	CTRLMSG_INS_OPENFILE,
	CTRLMSG_INS_NEWINSTRUMENT,
	CTRLMSG_INS_SONGDROP,
	CTRLMSG_INS_SAMPLEMAP,
};

enum {
	VIEWMSG_BASE=0,
	VIEWMSG_SETCTRLWND,
	VIEWMSG_SETACTIVE,
	VIEWMSG_SETFOCUS,
	VIEWMSG_SAVESTATE,
	VIEWMSG_LOADSTATE,
	// Pattern-Specific
	VIEWMSG_SETCURRENTPATTERN,
	VIEWMSG_GETCURRENTPATTERN,
	VIEWMSG_FOLLOWSONG,
	VIEWMSG_PATTERNLOOP,
	VIEWMSG_GETCURRENTPOS,
	VIEWMSG_SETRECORD,
	VIEWMSG_SETSPACING,
	VIEWMSG_PATTERNPROPERTIES,
	VIEWMSG_SETVUMETERS,
	VIEWMSG_SETPLUGINNAMES,	//rewbs.patPlugNames
	VIEWMSG_DOMIDISPACING,
	VIEWMSG_EXPANDPATTERN,
	VIEWMSG_SHRINKPATTERN,
	VIEWMSG_COPYPATTERN,
	VIEWMSG_PASTEPATTERN,
	VIEWMSG_AMPLIFYPATTERN,
	VIEWMSG_SETDETAIL,
	// Sample-Specific
	VIEWMSG_SETCURRENTSAMPLE,
	// Instrument-Specific
	VIEWMSG_SETCURRENTINSTRUMENT,
// -> CODE#0012
// -> DESC="midi keyboard split"
// rewbs.merge: swapped message direction
	VIEWMSG_SETSPLITINSTRUMENT,
	VIEWMSG_SETSPLITNOTE,
	VIEWMSG_SETOCTAVEMODIFIER,
	VIEWMSG_SETOCTAVELINK,
	VIEWMSG_SETSPLITVOLUME,
// -! CODE#0012
	VIEWMSG_DOSCROLL,

};


#define MODSTATUS_PLAYING		0x01
#define MODSTATUS_BUSY			0x02
#define MODSTATUS_RENDERING     0x04 //rewbs.VSTTimeInfo

#define SOUNDSETUP_ENABLEMMX	0x08
#define SOUNDSETUP_SOFTPANNING	0x10
#define SOUNDSETUP_STREVERSE	0x20
#define SOUNDSETUP_SECONDARY	0x40
#define SOUNDSETUP_RESTARTMASK	SOUNDSETUP_SECONDARY

#define QUALITY_NOISEREDUCTION	0x01
#define QUALITY_MEGABASS		0x02
#define QUALITY_SURROUND		0x08
#define QUALITY_REVERB			0x20
#define QUALITY_AGC				0x40
#define QUALITY_EQ				0x80


// User-defined colors
enum
{
	MODCOLOR_BACKNORMAL = 0,
	MODCOLOR_TEXTNORMAL,
	MODCOLOR_BACKCURROW,
	MODCOLOR_TEXTCURROW,
	MODCOLOR_BACKSELECTED,
	MODCOLOR_TEXTSELECTED,
	MODCOLOR_SAMPLE,
	MODCOLOR_BACKPLAYCURSOR,
	MODCOLOR_TEXTPLAYCURSOR,
	MODCOLOR_BACKHILIGHT,
	MODCOLOR_NOTE,
	MODCOLOR_INSTRUMENT,
	MODCOLOR_VOLUME,
	MODCOLOR_PANNING,
	MODCOLOR_PITCH,
	MODCOLOR_GLOBALS,
	MODCOLOR_ENVELOPES,
	MODCOLOR_VUMETER_LO,
	MODCOLOR_VUMETER_MED,
	MODCOLOR_VUMETER_HI,
	MAX_MODCOLORS,
	// Internal color codes
	MODSYSCOLOR_LO,
	MODSYSCOLOR_MED,
	MODSYSCOLOR_HI,
	MODCOLOR_2NDHIGHLIGHT,
	MAX_MODPALETTECOLORS
};

#define NUM_VUMETER_PENS		32

// Pattern Setup (contains also non-pattern related settings)
#define PATTERN_PLAYNEWNOTE		0x01
#define PATTERN_LARGECOMMENTS	0x02
#define PATTERN_STDHIGHLIGHT	0x04
#define PATTERN_SMALLFONT		0x08
#define PATTERN_CENTERROW		0x10
#define PATTERN_WRAP			0x20
#define PATTERN_EFFECTHILIGHT	0x40
#define PATTERN_HEXDISPLAY		0x80
#define PATTERN_FLATBUTTONS		0x100
#define PATTERN_CREATEBACKUP	0x200
#define PATTERN_SINGLEEXPAND	0x400
#define PATTERN_AUTOSPACEBAR	0x800
#define PATTERN_NOEXTRALOUD		0x1000
#define PATTERN_DRAGNDROPEDIT	0x2000
#define PATTERN_2NDHIGHLIGHT	0x4000
#define PATTERN_MUTECHNMODE		0x8000
#define PATTERN_SHOWPREVIOUS	0x10000
#define PATTERN_CONTSCROLL		0x20000
#define PATTERN_KBDNOTEOFF		0x40000
#define PATTERN_FOLLOWSONGOFF	0x80000 //rewbs.noFollow

// -> CODE#0017
// -> DESC="midi in record mode setup option"
#define PATTERN_MIDIRECORD		0x100000
// -! BEHAVIOUR_CHANGE#0017

// -> CODE#0022
// -> DESC="alternative BPM/Speed interpretation method"
#define	PATTERN_ALTERNTIVEBPMSPEED	0x200000
// rewbs: this options is now available under song settings. It is therefore saved with the song.
// -! NEW_FEATURE#0022

#define PATTERN_HILITETIMESIGS	0x400000

#define PATTERN_OLDCTXMENUSTYLE	0x800000
#define PATTERN_SYNCMUTE		0x1000000
#define PATTERN_AUTODELAY		0x2000000 
#define PATTERN_NOTEFADE		0x4000000


// Keyboard Setup
enum {
	KEYBOARD_CUSTOM=0,
	KEYBOARD_FT2,
	KEYBOARD_IT,
	KEYBOARD_MPT,
	KEYBOARD_MED
};

#define KEYBOARD_MASK		0x0F
#define KEYBOARD_FT2KEYS	0x10

// Midi Setup
#define MIDISETUP_RECORDVELOCITY			0x01
#define MIDISETUP_TRANSPOSEKEYBOARD			0x02
#define MIDISETUP_MIDITOPLUG				0x04
#define MIDISETUP_MIDIVOL_TO_NOTEVOL		0x08
#define MIDISETUP_RECORDNOTEOFF				0x10
#define MIDISETUP_RESPONDTOPLAYCONTROLMSGS	0x20
#define MIDISETUP_AMPLIFYVELOCITY			0x40
#define MIDISETUP_MIDIMACROCONTROL			0x80
#define MIDISETUP_PLAYPATTERNONMIDIIN       0x100



// Image List index
enum {
	IMAGE_COMMENTS=0,
	IMAGE_PATTERNS,
	IMAGE_SAMPLES,
	IMAGE_INSTRUMENTS,
	IMAGE_GENERAL,
	IMAGE_FOLDER,
	IMAGE_OPENFOLDER,
	IMAGE_PARTITION,
	IMAGE_NOSAMPLE,
	IMAGE_NOINSTRUMENT,
	IMAGE_NETWORKDRIVE,
	IMAGE_CDROMDRIVE,
	IMAGE_RAMDRIVE,
	IMAGE_FLOPPYDRIVE,
	IMAGE_REMOVABLEDRIVE,
	IMAGE_FIXEDDRIVE,
	IMAGE_FOLDERPARENT,
	IMAGE_FOLDERSONG,
	IMAGE_DIRECTX,
	IMAGE_WAVEOUT,
	IMAGE_ASIO,
	IMAGE_GRAPH,
};

//////////////////////////////////////////////////////////////////////////
// MPT Hot Keys

enum {
	MPTHOTKEY_TOGGLEEDIT=0,
	MPTHOTKEY_PLAYROW,
	MPTHOTKEY_CURSORCOPY,
	MPTHOTKEY_CURSORPASTE,
	MPTHOTKEY_MUTECHANNEL,
	MPTHOTKEY_SOLOCHANNEL,
	MPTHOTKEY_SELECTCOLUMN,
	MPTHOTKEY_TRANSPOSEUP,
	MPTHOTKEY_TRANSPOSEDOWN,
	MPTHOTKEY_TRANSPOSEOCTUP,
	MPTHOTKEY_TRANSPOSEOCTDOWN,
	MPTHOTKEY_PATTERNAMPLIFY,
	MPTHOTKEY_REPLACEINSTRUMENT,
	MPTHOTKEY_INTERPOLATEVOLUME,
	MPTHOTKEY_INTERPOLATEEFFECT,
	MPTHOTKEY_VISUALIZEEFFECT,		//rewbs.fxvis
	MAX_MPTHOTKEYS
};

typedef struct MPTHOTKEY
{
	UINT nID;			// WM_COMMAND Id - description is also loaded from the string id
	DWORD nMPTHotKey;	// Default hot key
	DWORD nFT2HotKey;
	DWORD nITHotKey;
	LPCSTR pszName;
} MPTHOTKEY, *PMPTHOTKEY;

#define MPTF_CTRL	(HOTKEYF_CONTROL<<16)
#define MPTF_ALT	(HOTKEYF_ALT<<16)
#define MPTF_SHIFT	(HOTKEYF_SHIFT<<16)
#define MPTF_EXT	(HOTKEYF_EXT<<16)

extern MPTHOTKEY gDefaultHotKeys[MAX_MPTHOTKEYS];


/////////////////////////////////////////////////////////////////////////
// Player position notification

#define MAX_UPDATE_HISTORY		8

#define MPTNOTIFY_TYPEMASK		0x00FF0000	// HiWord = type, LoWord = subtype (smp/instr #)
#define MPTNOTIFY_PENDING		0x01000000	// Being processed
#define MPTNOTIFY_DEFAULT		0x00010000
#define MPTNOTIFY_POSITION		0x00010000
#define MPTNOTIFY_SAMPLE		0x00020000
#define MPTNOTIFY_VOLENV		0x00040000
#define MPTNOTIFY_PANENV		0x00080000
#define MPTNOTIFY_PITCHENV		0x00100000
#define MPTNOTIFY_VUMETERS		0x00200000
#define MPTNOTIFY_MASTERVU		0x00400000
#define MPTNOTIFY_STOP			0x00800000
#define MPTNOTIFY_POSVALID		0x80000000	// dwPos[i] is valid

typedef struct MPTNOTIFICATION
{
	DWORD dwType;
	DWORD dwLatency;
	UINT nOrder, nPattern, nRow;			// Always valid
	DWORD dwPos[MAX_CHANNELS];	// sample/envelope pos for each channel if >= 0
} MPTNOTIFICATION, *PMPTNOTIFICATION;

/////////////////////////////////////////////////////////////////////////
// EQ Presets

typedef struct _EQPRESET
{
	CHAR szName[12];
	UINT Gains[MAX_EQ_BANDS];
	UINT Freqs[MAX_EQ_BANDS];
} EQPRESET, *PEQPRESET;


/////////////////////////////////////////////////////////////////////////
// Misc. Macros


#define DeleteGDIObject(h) if (h) { ::DeleteObject(h); h = NULL; }
#define UPDATEDSPEFFECTS() SetDspEffects(\
								m_dwQuality & QUALITY_SURROUND,\
								m_dwQuality & QUALITY_REVERB,\
								m_dwQuality & QUALITY_MEGABASS,\
								m_dwQuality & QUALITY_NOISEREDUCTION,\
								m_dwQuality & QUALITY_EQ)
#define BEGIN_CRITICAL()		EnterCriticalSection(&CMainFrame::m_csAudio)
#define END_CRITICAL()			LeaveCriticalSection(&CMainFrame::m_csAudio)

#include "mainbar.h"

//===================================
class CMainFrame: public CMDIFrameWnd
//===================================
{
	DECLARE_DYNAMIC(CMainFrame)
	// static data
public:
	CString m_csRegKey;
	CString m_csRegExt;
	CString m_csRegSettings;
	CString m_csRegWindow;
	static CString m_csExecutableDirectoryPath; //To contain path of executable directory
	// Globals
	static UINT m_nLastOptionsPage, m_nFilterIndex;
	static BOOL gbMdiMaximize;
	static bool gbShowHackControls;
	static LONG glCtrlWindowHeight, glTreeWindowWidth, glTreeSplitRatio;
	static LONG glGeneralWindowHeight, glPatternWindowHeight, glSampleWindowHeight, 
		        glInstrumentWindowHeight, glCommentsWindowHeight, glGraphWindowHeight; //rewbs.varWindowSize
    static HHOOK ghKbdHook;
	static DWORD gdwNotificationType;
	static CString gcsPreviousVersion;
	static CString gcsInstallGUID;
	static int gnCheckForUpdates;
	
	// Audio Setup
	static DWORD m_dwSoundSetup, m_dwRate, m_dwQuality, m_nSrcMode, m_nBitsPerSample, m_nPreAmp, gbLoopSong, m_nChannels;
	static LONG m_nWaveDevice, m_nMidiDevice;
	static DWORD m_nBufferLength;
	static EQPRESET m_EqSettings;
	// Pattern Setup
	static UINT gnPatternSpacing;
	static BOOL gbPatternVUMeters, gbPatternPluginNames, gbPatternRecord;
	static DWORD m_dwPatternSetup, m_dwMidiSetup, m_nRowSpacing, m_nRowSpacing2, m_nKeyboardCfg, gnHotKeyMask;
	static bool m_bHideUnavailableCtxMenuItems;
	// GDI
	static HICON m_hIcon;
	static HFONT m_hGUIFont, m_hFixedFont, m_hLargeFixedFont;
	static HBRUSH brushGray, brushBlack, brushWhite, brushHighLight, brushHighLightRed, brushWindow;
//	static CBrush *pbrushBlack, *pbrushWhite;
	static HPEN penBlack, penDarkGray, penLightGray, penWhite, penHalfDarkGray, penSample, penEnvelope, penEnvelopeHighlight, penSeparator, penScratch, penGray00, penGray33, penGray40, penGray55, penGray80, penGray99, penGraycc, penGrayff;
	static HCURSOR curDragging, curNoDrop, curArrow, curNoDrop2, curVSplit;
	static COLORREF rgbCustomColors[MAX_MODCOLORS];
	static LPMODPLUGDIB bmpPatterns, bmpNotes, bmpVUMeters, bmpVisNode;
	static HPEN gpenVuMeter[NUM_VUMETER_PENS*2];
	// Arrays
	static CHAR m_szModDir[_MAX_PATH], m_szSmpDir[_MAX_PATH], m_szInsDir[_MAX_PATH], m_szKbdFile[_MAX_PATH];
	static CHAR m_szCurModDir[_MAX_PATH], m_szCurSmpDir[_MAX_PATH], m_szCurInsDir[_MAX_PATH], m_szCurKbdDir[_MAX_PATH];

	// Low-Level Audio
public:

	static CRITICAL_SECTION m_csAudio;
	static ISoundDevice *gpSoundDevice;
	static HANDLE m_hAudioWakeUp, m_hNotifyWakeUp;
	static HANDLE m_hPlayThread, m_hNotifyThread;
	static DWORD m_dwPlayThreadId, m_dwNotifyThreadId;
	static LONG gnLVuMeter, gnRVuMeter;
	static UINT gdwIdleTime;
	static LONG slSampleSize, sdwSamplesPerSec, sdwAudioBufferSize;
	//rewbs.resamplerConf
	static double gdWFIRCutoff;
	static BYTE gbWFIRType;
	static long glVolumeRampSamples;
	//end rewbs.resamplerConf
	static UINT gnAutoChordWaitTime;

	static int gnPlugWindowX;
	static int gnPlugWindowY;
	static int gnPlugWindowWidth;
	static int gnPlugWindowHeight;
	static DWORD gnPlugWindowLast;

	static uint32 gnMsgBoxVisiblityFlags;

	// Midi Input
public:
	static HMIDIIN shMidiIn;


protected:
    CSoundFile m_WaveFile;
	CModTreeBar m_wndTree;
	CStatusBar m_wndStatusBar;
	CMainToolBar m_wndToolBar;
	CImageList m_ImageList;
	CModDoc *m_pModPlaying;
	CSoundFile *m_pSndFile;
	HWND m_hFollowSong, m_hWndMidi;
	DWORD m_dwStatus, m_dwElapsedTime, m_dwTimeSec, m_dwNotifyType;
	UINT m_nTimer, m_nAvgMixChn, m_nMixChn;
	CHAR m_szUserText[512], m_szInfoText[512], m_szXInfoText[512]; //rewbs.xinfo
	// Chords
	MPTCHORD Chords[3*12]; // 3 octaves
	// Notification Buffer
	MPTNOTIFICATION NotifyBuffer[MAX_UPDATE_HISTORY];
	// Misc
	CHAR m_szPluginsDir[_MAX_PATH];
	CHAR m_szExportDir[_MAX_PATH];
	bool m_bOptionsLocked; 	 	//rewbs.customKeys
	double m_dTotalCPU;
	CModDoc* m_pJustModifiedDoc;

public:
	CMainFrame(/*CString regKeyExtension*/);
	VOID Initialize();
	

// Low-Level Audio
public:
	static void UpdateAudioParameters(BOOL bReset=FALSE);
	static void EnableLowLatencyMode(BOOL bOn=TRUE);
	static void CalcStereoVuMeters(int *, unsigned long, unsigned long);
	static DWORD WINAPI AudioThread(LPVOID);
	static DWORD WINAPI NotifyThread(LPVOID);
	ULONG AudioRead(PVOID pData, ULONG cbSize);
	VOID AudioDone(ULONG nBytesWritten, ULONG nLatency);
	LONG audioTryOpeningDevice(UINT channels, UINT bits, UINT samplespersec);
	BOOL audioOpenDevice();
	void audioCloseDevice();
	BOOL audioFillBuffers();
	LRESULT OnWOMDone(WPARAM, LPARAM);
	BOOL dsoundFillBuffers(LPBYTE lpBuf, DWORD dwSize);
	BOOL DSoundDone(LPBYTE lpBuffer, DWORD dwBytes);
	BOOL DoNotification(DWORD dwSamplesRead, DWORD dwLatency);
	LPCSTR GetPluginsDir() const { return m_szPluginsDir; }
	VOID SetPluginsDir(LPCSTR pszDir) { lstrcpyn(m_szPluginsDir, pszDir, sizeof(m_szPluginsDir)); }
	LPCSTR GetExportDir() const { return m_szExportDir; }
	VOID SetExportDir(LPCSTR pszDir) { lstrcpyn(m_szExportDir, pszDir, sizeof(m_szExportDir)); }

// Midi Input Functions
public:
	BOOL midiOpenDevice();
	void midiCloseDevice();
	void midiReceive();
	void SetMidiRecordWnd(HWND hwnd) { m_hWndMidi = hwnd; }
	HWND GetMidiRecordWnd() const { return m_hWndMidi; }

// static functions
public:
	static CMainFrame *GetMainFrame() { return (CMainFrame *)theApp.m_pMainWnd; }
	static VOID UpdateColors();
	static HICON GetModIcon() { return m_hIcon; }
	static HFONT GetGUIFont() { return m_hGUIFont; }
	static HFONT GetFixedFont() { return m_hFixedFont; }
	static HFONT GetLargeFixedFont() { return m_hLargeFixedFont; }
	static void UpdateAllViews(DWORD dwHint, CObject *pHint=NULL);
	static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
	static void TranslateKeyboardMap(LPSTR pszKbd);
	static VOID GetKeyName(LONG lParam, LPSTR pszName, UINT cbSize);
	static CInputHandler *m_InputHandler; 	//rewbs.customKeys
	static CAutoSaver *m_pAutoSaver; 		//rewbs.customKeys
	static CPerformanceCounter *m_pPerfCounter;

	static bool WritePrivateProfileLong(const CString section, const CString key, const long value, const CString iniFile);
	static long GetPrivateProfileLong(const CString section, const CString key, const long defaultValue, const CString iniFile);
	static bool WritePrivateProfileDWord(const CString section, const CString key, const DWORD value, const CString iniFile);
	static DWORD GetPrivateProfileDWord(const CString section, const CString key, const DWORD defaultValue, const CString iniFile);
	static bool WritePrivateProfileCString(const CString section, const CString key, const CString value, const CString iniFile);
	static CString GetPrivateProfileCString(const CString section, const CString key, const CString defaultValue, const CString iniFile);
	

// Misc functions
public:
	VOID SetUserText(LPCSTR lpszText);
	VOID SetInfoText(LPCSTR lpszText);
	VOID SetXInfoText(LPCSTR lpszText); //rewbs.xinfo
	VOID SetHelpText(LPCSTR lpszText);
	UINT GetBaseOctave();
	UINT GetCurrentInstrument();
	CModDoc *GetActiveDoc();
	CView *GetActiveView();  	//rewbs.customKeys
	CImageList *GetImageList() { return &m_ImageList; }
	PMPTCHORD GetChords() { return Chords; }
	VOID OnDocumentCreated(CModDoc *pModDoc);
	VOID OnDocumentClosed(CModDoc *pModDoc);
	VOID UpdateTree(CModDoc *pModDoc, DWORD lHint=0, CObject *pHint=NULL);
	static CInputHandler* GetInputHandler() { return m_InputHandler; }  	//rewbs.customKeys
	bool m_bModTreeHasFocus;  	//rewbs.customKeys
	CWnd *m_pNoteMapHasFocus;  	//rewbs.customKeys
	long GetSampleRate();  		//rewbs.VSTTimeInfo
	long GetTotalSampleCount(); //rewbs.VSTTimeInfo
	double GetApproxBPM();		//rewbs.VSTTimeInfo
	void ThreadSafeSetModified(CModDoc* modified) {m_pJustModifiedDoc=modified;}

// Player functions
public:
	BOOL PlayMod(CModDoc *, HWND hPat=NULL, DWORD dwNotifyType=0);
	BOOL StopMod(CModDoc *pDoc=NULL);
	BOOL PauseMod(CModDoc *pDoc=NULL);
	BOOL PlaySoundFile(CSoundFile *);
	BOOL PlaySoundFile(LPCSTR lpszFileName, UINT nNote=0);
	BOOL PlaySoundFile(CSoundFile *pSong, UINT nInstrument, UINT nSample, UINT nNote=0);
	BOOL PlayDLSInstrument(UINT nDLSBank, UINT nIns, UINT nRgn);
	BOOL StopSoundFile(CSoundFile *);
	inline BOOL IsPlaying() const { return (m_dwStatus & MODSTATUS_PLAYING); 	}
	inline BOOL IsRendering() const { return (m_dwStatus & MODSTATUS_RENDERING); 	} //rewbs.VSTTimeInfo
	DWORD GetElapsedTime() const { return m_dwElapsedTime; }
	void ResetElapsedTime() { m_dwElapsedTime = 0; }
	inline CModDoc *GetModPlaying() const { return (IsPlaying()||IsRendering()) ? m_pModPlaying : NULL; }
	inline CSoundFile *GetSoundFilePlaying() const { return (IsPlaying()||IsRendering()) ? m_pSndFile : NULL; }  //rewbs.VSTTimeInfo
	BOOL InitRenderer(CSoundFile*);  //rewbs.VSTTimeInfo
	BOOL StopRenderer(CSoundFile*);  //rewbs.VSTTimeInfo
	void SwitchToActiveView();
	BOOL SetupSoundCard(DWORD q, DWORD rate, UINT nbits, UINT chns, UINT bufsize, LONG wd);
	BOOL SetupDirectories(LPCSTR s, LPCSTR s2, LPCSTR s3);
	BOOL SetupPlayer(DWORD, DWORD, BOOL bForceUpdate=FALSE);
	BOOL SetupMidi(DWORD d, LONG n);
	void SetPreAmp(UINT n);
	HWND GetFollowSong(const CModDoc *pDoc) const { return (pDoc == m_pModPlaying) ? m_hFollowSong : NULL; }
	BOOL SetFollowSong(CModDoc *, HWND hwnd, BOOL bFollowSong=TRUE, DWORD dwType=MPTNOTIFY_DEFAULT);
	BOOL ResetNotificationBuffer(HWND hwnd=NULL);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Message map functions
	//{{AFX_MSG(CMainFrame)
public:
	afx_msg void OnAddDlsBank();
	afx_msg void OnImportMidiLib();
	afx_msg void SetLastMixActiveTime(); //rewbs.VSTCompliance
	afx_msg void OnViewOptions();		 //rewbs.resamplerConf: made public so it's accessible from mod2wav gui :/
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT);
	afx_msg void OnSongProperties();

// -> CODE#0002
// -> DESC="list box to choose VST plugin presets (programs)"
	afx_msg void OnPluginManager();
// -! NEW_FEATURE#0002

	

// -> CODE#0015
// -> DESC="channels management dlg"
	afx_msg void OnChannelManager();
// -! NEW_FEATURE#0015

	afx_msg void OnUpdateTime(CCmdUI *pCmdUI);
	afx_msg void OnUpdateUser(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInfo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateXInfo(CCmdUI *pCmdUI); //rewbs.xinfo
	afx_msg void OnUpdateCPU(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMidiRecord(CCmdUI *pCmdUI);
	afx_msg void OnPlayerPause();
	afx_msg void OnMidiRecord();
	afx_msg void OnPrevOctave();
	afx_msg void OnNextOctave();
	afx_msg void OnOctaveChanged();
	afx_msg void OnReportBug();	//rewbs.customKeys
	afx_msg BOOL OnInternetLink(UINT nID);
	afx_msg LRESULT OnUpdatePosition(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnInvalidatePatterns(WPARAM, LPARAM);
	afx_msg LRESULT OnSpecialKey(WPARAM, LPARAM);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	afx_msg void OnViewMIDIMapping();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnInitMenu(CMenu* pMenu);
	bool UpdateEffectKeys(void); 
	bool UpdateHighlights(void);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

private:
	void LoadRegistrySettings();
	void LoadIniSettings();
	void SaveIniSettings();
};

const CHAR gszBuildDate[] = __DATE__ " " __TIME__;



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__AE144DC8_DD0B_11D1_AF24_444553540000__INCLUDED_)
