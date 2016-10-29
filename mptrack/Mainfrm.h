/*
 * MainFrm.h
 * ---------
 * Purpose: Implementation of OpenMPT's main window code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include <Msctf.h>
#include "Mptrack.h"
#include "AutoSaver.h"
#include "UpdateHints.h"
#include "../soundlib/AudioCriticalSection.h"
#include "../common/mptMutex.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/Dither.h"

OPENMPT_NAMESPACE_BEGIN

class CInputHandler;
class CModDoc;
class CAutoSaver;
namespace SoundDevice {
class Base;
class ISource;
} // namerspace SoundDevice

#define MAINFRAME_TITLE				"Open ModPlug Tracker"
#define MAINFRAME_TITLEW			L"Open ModPlug Tracker"

// Custom window messages
enum
{
	WM_MOD_UPDATEPOSITION	=	(WM_USER+1973),
	WM_MOD_INVALIDATEPATTERNS,
	WM_MOD_ACTIVATEVIEW,
	WM_MOD_CHANGEVIEWCLASS,
	WM_MOD_UNLOCKCONTROLS,
	WM_MOD_CTRLMSG,
	WM_MOD_VIEWMSG,
	WM_MOD_TREEMSG,
	WM_MOD_MIDIMSG,
	WM_MOD_GETTOOLTIPTEXT,
	WM_MOD_DRAGONDROPPING,
	WM_MOD_SPECIALKEY,
	WM_MOD_KBDNOTIFY,
	WM_MOD_INSTRSELECTED,
	WM_MOD_KEYCOMMAND,
	WM_MOD_RECORDPARAM,
	WM_MOD_PLUGPARAMAUTOMATE,
	WM_MOD_MIDIMAPPING,
};

enum
{
	MPT_WM_APP_UPDATECHECK_PROGRESS = WM_APP + 1,
	MPT_WM_APP_UPDATECHECK_SUCCESS = WM_APP + 2,
	MPT_WM_APP_UPDATECHECK_FAILURE = WM_APP + 3,
};

enum
{
	CTRLMSG_BASE=0,
	CTRLMSG_SETVIEWWND,
	CTRLMSG_ACTIVATEPAGE,
	CTRLMSG_DEACTIVATEPAGE,
	CTRLMSG_SETFOCUS,
	// Pattern-Specific
	CTRLMSG_SETCURRENTPATTERN,
	CTRLMSG_GETCURRENTPATTERN,
	CTRLMSG_NOTIFYCURRENTORDER,
	CTRLMSG_SETCURRENTORDER,
	CTRLMSG_GETCURRENTORDER,
	CTRLMSG_FORCEREFRESH,
	CTRLMSG_PAT_PREVINSTRUMENT,
	CTRLMSG_PAT_NEXTINSTRUMENT,
	CTRLMSG_PAT_SETINSTRUMENT,
	CTRLMSG_PAT_FOLLOWSONG,		//rewbs.customKeys
	CTRLMSG_PAT_LOOP,
	CTRLMSG_PAT_NEWPATTERN,		//rewbs.customKeys
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
	CTRLMSG_PAT_DUPPATTERN,
	// Sample-Specific
	CTRLMSG_SMP_PREVINSTRUMENT,
	CTRLMSG_SMP_NEXTINSTRUMENT,
	CTRLMSG_SMP_OPENFILE,
	CTRLMSG_SMP_OPENFILE_NEW,
	CTRLMSG_SMP_SETZOOM,
	CTRLMSG_SMP_GETZOOM,
	CTRLMSG_SMP_SONGDROP,
	// Instrument-Specific
	CTRLMSG_INS_PREVINSTRUMENT,
	CTRLMSG_INS_NEXTINSTRUMENT,
	CTRLMSG_INS_OPENFILE,
	CTRLMSG_INS_OPENFILE_NEW,
	CTRLMSG_INS_NEWINSTRUMENT,
	CTRLMSG_INS_SONGDROP,
	CTRLMSG_INS_SAMPLEMAP,
};

enum
{
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
	VIEWMSG_DOSCROLL,

};


#define NUM_VUMETER_PENS		32


// Tab Order
enum OptionsPage
{
	OPTIONS_PAGE_DEFAULT = 0,
	OPTIONS_PAGE_GENERAL = OPTIONS_PAGE_DEFAULT,
	OPTIONS_PAGE_SOUNDCARD,
	OPTIONS_PAGE_MIXER,
	OPTIONS_PAGE_PLAYER,
	OPTIONS_PAGE_SAMPLEDITOR,
	OPTIONS_PAGE_KEYBOARD,
	OPTIONS_PAGE_COLORS,
	OPTIONS_PAGE_MIDI,
	OPTIONS_PAGE_PATHS,
	OPTIONS_PAGE_UPDATE,
	OPTIONS_PAGE_ADVANCED,
};


/////////////////////////////////////////////////////////////////////////
// Player position notification

#define MAX_UPDATE_HISTORY		2000 // 2 seconds with 1 ms updates
OPENMPT_NAMESPACE_END
#include "Notification.h"
OPENMPT_NAMESPACE_BEGIN

#define TIMERID_GUI 1
#define TIMERID_NOTIFY 2

OPENMPT_NAMESPACE_END
#include "CImageListEx.h"
#include "Mainbar.h"
#include "TrackerSettings.h"
OPENMPT_NAMESPACE_BEGIN
struct MODPLUGDIB;

template<> inline SettingValue ToSettingValue(const WINDOWPLACEMENT &val)
{
	return SettingValue(EncodeBinarySetting<WINDOWPLACEMENT>(val), "WINDOWPLACEMENT");
}
template<> inline WINDOWPLACEMENT FromSettingValue(const SettingValue &val)
{
	ASSERT(val.GetTypeTag() == "WINDOWPLACEMENT");
	return DecodeBinarySetting<WINDOWPLACEMENT>(val.as<std::vector<mpt::byte> >());
}


class VUMeter
{
public:
	static const std::size_t maxChannels = 4;
	static const float dynamicRange; // corresponds to the current implementation of the UI widget diplaying the result
	struct Channel
	{
		int32 peak;
		bool clipped;
		Channel() : peak(0), clipped(false) { }
	};
private:
	Channel channels[maxChannels];
	int32 decayParam;
public:
	VUMeter() : decayParam(0) { SetDecaySpeedDecibelPerSecond(88.0f); }
	void SetDecaySpeedDecibelPerSecond(float decibelPerSecond);
public:
	const Channel & operator [] (std::size_t channel) const { return channels[channel]; }
	void Process(const int *mixbuffer, std::size_t numChannels, std::size_t numFrames); // mixbuffer is interleaved
	void Decay(int32 secondsNum, int32 secondsDen);
	void ResetClipped();
};


//=====================================================================
class TfLanguageProfileNotifySink : public ITfLanguageProfileNotifySink
//=====================================================================
{
public:
	TfLanguageProfileNotifySink();
	~TfLanguageProfileNotifySink();

	virtual HRESULT STDMETHODCALLTYPE OnLanguageChange(LANGID langid, __RPC__out BOOL *pfAccept);
	virtual HRESULT STDMETHODCALLTYPE OnLanguageChanged();
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

protected:
	ITfInputProcessorProfiles *m_pProfiles;
	ITfSource *m_pSource;
	DWORD m_dwCookie;
};


//==========================================================================================================================================
class CMainFrame: public CMDIFrameWnd, public SoundDevice::ISource, public SoundDevice::IMessageReceiver, public TfLanguageProfileNotifySink
//==========================================================================================================================================
{
	DECLARE_DYNAMIC(CMainFrame)
	// static data
public:

	// Globals
	static OptionsPage m_nLastOptionsPage;
	static HHOOK ghKbdHook;

	// GDI
	static HICON m_hIcon;
	static HFONT m_hGUIFont, m_hFixedFont;
	static HBRUSH brushGray, brushBlack, brushWhite, brushText, brushHighLight, brushHighLightRed, brushWindow, brushYellow;
	static HPEN penBlack, penDarkGray, penLightGray, penWhite, penHalfDarkGray, penSample, penEnvelope, penEnvelopeHighlight, penSeparator, penScratch, penGray00, penGray33, penGray40, penGray55, penGray80, penGray99, penGraycc, penGrayff;
	static HCURSOR curDragging, curNoDrop, curArrow, curNoDrop2, curVSplit;
	static MODPLUGDIB *bmpNotes, *bmpVUMeters, *bmpPluginVUMeters;
	static COLORREF gcolrefVuMeter[NUM_VUMETER_PENS * 2];	// General tab VU meters

public:

	// Low-Level Audio
	CriticalSection m_SoundDeviceFillBufferCriticalSection;
	Util::MultimediaClock m_SoundDeviceClock;
	SoundDevice::IBase *gpSoundDevice;
	UINT_PTR m_NotifyTimer;
	Dither m_Dither;
	VUMeter m_VUMeter;

	DWORD m_AudioThreadId;
	bool m_InNotifyHandler;

	// Midi Input
public:
	static HMIDIIN shMidiIn;

public:
	CImageListEx m_MiscIcons, m_MiscIconsDisabled;			// Misc Icons
	CImageListEx m_PatternIcons, m_PatternIconsDisabled;	// Pattern icons (includes some from sample editor as well...)
	CImageListEx m_EnvelopeIcons;							// Instrument editor icons
	CImageListEx m_SampleIcons;								// Sample editor icons

protected:

	CModTreeBar m_wndTree;
	CStatusBar m_wndStatusBar;
	CMainToolBar m_wndToolBar;
	CSoundFile *m_pSndFile; // != NULL only when currently playing or rendering
	HWND m_hWndMidi;
	CSoundFile::samplecount_t m_dwTimeSec;
	UINT_PTR m_nTimer;
	UINT m_nAvgMixChn, m_nMixChn;
	// Misc
	CModDoc* m_pJustModifiedDoc;
	class COptionsSoundcard *m_SoundCardOptionsDialog;
	DWORD helpCookie;
	bool m_bOptionsLocked;

	// Notification Buffer
	mpt::mutex m_NotificationBufferMutex; // to avoid deadlocks, this mutex should only be taken as a innermost lock, i.e. do not block on anything while holding this mutex
	Util::fixed_size_queue<Notification,MAX_UPDATE_HISTORY> m_NotifyBuffer;

	// Instrument preview in tree view
	CSoundFile m_WaveFile;

	CHAR m_szUserText[512], m_szInfoText[512], m_szXInfoText[512];

	CAutoSaver m_AutoSaver;

public:
	CMainFrame(/*CString regKeyExtension*/);
	void Initialize();


// Low-Level Audio
public:
	static void UpdateDspEffects(CSoundFile &sndFile, bool reset=false);
	static void UpdateAudioParameters(CSoundFile &sndFile, bool reset=false);

	// from SoundDevice::ISource
	uint64 SoundSourceGetReferenceClockNowNanoseconds() const;
	void SoundSourcePreStartCallback();
	void SoundSourcePostStopCallback();
	bool SoundSourceIsLockedByCurrentThread() const;
	void SoundSourceLock();
	uint64 SoundSourceLockedGetReferenceClockNowNanoseconds() const;
	void SoundSourceLockedRead(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo, std::size_t numFrames, void *buffer, const void *inputBuffer);
	void SoundSourceLockedDone(SoundDevice::BufferFormat bufferFormat, SoundDevice::BufferAttributes bufferAttributes, SoundDevice::TimeInfo timeInfo);
	void SoundSourceUnlock();

	// from SoundDevice::IMessageReceiver
	void SoundDeviceMessage(LogLevel level, const mpt::ustring &str);

	bool InGuiThread() const { return theApp.InGuiThread(); }
	bool InAudioThread() const { return GetCurrentThreadId() == m_AudioThreadId; }
	bool InNotifyHandler() const { return m_InNotifyHandler; }

	bool audioOpenDevice();
	void audioCloseDevice();
	bool IsAudioDeviceOpen() const;
	bool DoNotification(DWORD dwSamplesRead, int64 streamPosition);

// Midi Input Functions
public:
	bool midiOpenDevice(bool showSettings = true);
	void midiCloseDevice();
	void midiReceive();
	void SetMidiRecordWnd(HWND hwnd) { m_hWndMidi = hwnd; }
	HWND GetMidiRecordWnd() const { return m_hWndMidi; }

	static int ApplyVolumeRelatedSettings(const DWORD &dwParam1, const BYTE midivolume);

// static functions
public:
	static CMainFrame *GetMainFrame() { return (CMainFrame *)theApp.m_pMainWnd; }
	static void UpdateColors();
	static HICON GetModIcon() { return m_hIcon; }
	static HFONT GetGUIFont() { return m_hGUIFont; }
	static HFONT &GetCommentsFont() { return m_hFixedFont; }
	static void UpdateAllViews(UpdateHint hint, CObject *pHint=NULL);
	static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
	static CInputHandler *m_InputHandler;

	// Misc functions
public:
	void SetUserText(LPCSTR lpszText);
	void SetInfoText(LPCSTR lpszText);
	void SetXInfoText(LPCSTR lpszText);
	void SetHelpText(LPCSTR lpszText);
	UINT GetBaseOctave() const;
	CModDoc *GetActiveDoc();
	CView *GetActiveView();
	void OnDocumentCreated(CModDoc *pModDoc);
	void OnDocumentClosed(CModDoc *pModDoc);
	void UpdateTree(CModDoc *pModDoc, UpdateHint hint, CObject *pHint=NULL);
	static CInputHandler* GetInputHandler() { return m_InputHandler; }
	bool m_bModTreeHasFocus;
	CWnd *m_pNoteMapHasFocus;
	CWnd* m_pOrderlistHasFocus;
	void ThreadSafeSetModified(CModDoc* modified) { InterlockedExchangePointer(reinterpret_cast<void **>(&m_pJustModifiedDoc), modified); }
	void SetElapsedTime(double t) { m_dwTimeSec = static_cast<CSoundFile::samplecount_t>(t); }

	CModTree *GetUpperTreeview() { return m_wndTree.m_pModTree; }
	CModTree *GetLowerTreeview() { return m_wndTree.m_pModTreeData; }
	bool SetTreeSoundfile(FileReader &file) { return m_wndTree.SetTreeSoundfile(file); }

	void CreateExampleModulesMenu();
	void CreateTemplateModulesMenu();
	CMenu *GetFileMenu() const;

	/// Creates submenu whose items are filenames of files in both
	/// AppDirectory\pszFolderName\   (usually C:\program files\OpenMPT\pszFolderName\)
	/// and
	/// ConfigDirectory\pszFolderName  (usually %appdata%\OpenMPT\pszFolderName\)
	/// [in] nMaxCount: Maximum number of items allowed in the menu
	/// [out] vPaths: Receives the full paths of the files added to the menu.
	/// [in] pszFolderName: Name of the folder (should end with \)
	/// [in] nIdRangeBegin: First ID for the menu item.
	static HMENU CreateFileMenu(const size_t nMaxCount, std::vector<mpt::PathString>& vPaths, const mpt::PathString &pszFolderName, const uint16 nIdRangeBegin);

// Player functions
public:

	// high level synchronous playback functions, do not hold AudioCriticalSection while calling these
	bool PreparePlayback();
	bool StartPlayback();
	void StopPlayback();
	bool RestartPlayback();
	bool PausePlayback();
	static bool IsValidSoundFile(CSoundFile &sndFile) { return sndFile.GetType() ? true : false; }
	static bool IsValidSoundFile(CSoundFile *pSndFile) { return pSndFile && pSndFile->GetType(); }
	void SetPlaybackSoundFile(CSoundFile *pSndFile);
	void UnsetPlaybackSoundFile();
	void GenerateStopNotification();

	bool PlayMod(CModDoc *);
	bool StopMod(CModDoc *pDoc=NULL);
	bool PauseMod(CModDoc *pDoc=NULL);

	bool StopSoundFile(CSoundFile *);
	bool PlaySoundFile(CSoundFile *);
	BOOL PlaySoundFile(const mpt::PathString &filename, ModCommand::NOTE note);
	BOOL PlaySoundFile(CSoundFile &sndFile, INSTRUMENTINDEX nInstrument, SAMPLEINDEX nSample, ModCommand::NOTE note);
	BOOL PlayDLSInstrument(UINT nDLSBank, UINT nIns, UINT nRgn, ModCommand::NOTE note);

	void InitPreview();
	void PreparePreview(ModCommand::NOTE note);
	void StopPreview() { StopSoundFile(&m_WaveFile); }
	void PlayPreview() { PlaySoundFile(&m_WaveFile); }

	inline bool IsPlaying() const { return m_pSndFile != nullptr; }
	// Return currently playing module (nullptr if none is playing)
	inline CModDoc *GetModPlaying() const { return m_pSndFile ? m_pSndFile->GetpModDoc() : nullptr; }
	// Return currently playing module (nullptr if none is playing)
	inline CSoundFile *GetSoundFilePlaying() const { return m_pSndFile; }
	BOOL InitRenderer(CSoundFile*);
	BOOL StopRenderer(CSoundFile*);
	void SwitchToActiveView();

	void IdleHandlerSounddevice();

	BOOL ResetSoundCard();
	BOOL SetupSoundCard(SoundDevice::Settings deviceSettings, SoundDevice::Identifier deviceIdentifier, SoundDeviceStopMode stoppedMode, bool forceReset = false);
	BOOL SetupMiscOptions();
	BOOL SetupPlayer();

	BOOL SetupDirectories(const mpt::PathString &szModDir, const mpt::PathString &szSampleDir, const mpt::PathString &szInstrDir, const mpt::PathString &szVstDir, const mpt::PathString &szPresetDir);
	void SetupMidi(DWORD d, UINT_PTR n);
	HWND GetFollowSong() const;
	HWND GetFollowSong(const CModDoc *pDoc) const { return (pDoc == GetModPlaying()) ? GetFollowSong() : NULL; }
	void ResetNotificationBuffer();


// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	//}}AFX_VIRTUAL

	/// Opens either template or example menu item.
	void OpenMenuItemFile(const UINT nId, const bool isTemplateFile);

public:
	void UpdateMRUList();

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void OnTimerGUI();
	void OnTimerNotify();

// Message map functions
	//{{AFX_MSG(CMainFrame)
public:
	afx_msg void OnAddDlsBank();
	afx_msg void OnImportMidiLib();
	afx_msg void OnViewOptions();
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnRButtonDown(UINT, CPoint);
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR);

	afx_msg void OnPluginManager();

	afx_msg void OnChannelManager();
	afx_msg void OnClipboardManager();

	afx_msg LRESULT OnViewMIDIMapping(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateTime(CCmdUI *pCmdUI);
	afx_msg void OnUpdateUser(CCmdUI *pCmdUI);
	afx_msg void OnUpdateInfo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateXInfo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMidiRecord(CCmdUI *pCmdUI);
	afx_msg void OnPlayerPause();
	afx_msg void OnMidiRecord();
	afx_msg void OnPrevOctave();
	afx_msg void OnNextOctave();
	afx_msg void OnPanic();
	afx_msg void OnReportBug();
	afx_msg BOOL OnInternetLink(UINT nID);
	afx_msg LRESULT OnUpdatePosition(WPARAM, LPARAM lParam);
	afx_msg void OnOpenTemplateModule(UINT nId);
	afx_msg void OnExampleSong(UINT nId);
	afx_msg void OnOpenMRUItem(UINT nId);
	afx_msg void OnUpdateMRUItem(CCmdUI *cmd);
	afx_msg LRESULT OnInvalidatePatterns(WPARAM, LPARAM);
	afx_msg LRESULT OnSpecialKey(WPARAM, LPARAM);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	afx_msg void OnInternetUpdate();
	afx_msg void OnShowSettingsFolder();
	afx_msg LRESULT OnUpdateCheckProgress(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnUpdateCheckSuccess(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnUpdateCheckFailure(WPARAM wparam, LPARAM lparam);
	afx_msg void OnHelp();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnInitMenu(CMenu* pMenu);
	bool UpdateEffectKeys();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	// Defines maximum number of items in example modules menu.
	static const size_t nMaxItemsInExampleModulesMenu = 50;
	static const size_t nMaxItemsInTemplateModulesMenu = 50;

	/// Array of paths of example modules that are available from help menu.
	static std::vector<mpt::PathString> s_ExampleModulePaths;
	/// Array of paths of template modules that are available from file menu.
	static std::vector<mpt::PathString> s_TemplateModulePaths;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


OPENMPT_NAMESPACE_END
