/*
 * Mainfrm.h
 * ---------
 * Purpose: Implementation of OpenMPT's main window code.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "CImageListEx.h"
#include "Mainbar.h"
#include "Notification.h"
#include "openmpt/soundbase/Dither.hpp"
#include "Settings.h"
#include "../common/Dither.h"
#include "../soundlib/AudioCriticalSection.h"
#include "../soundlib/Sndfile.h"
#include "mpt/audio/span.hpp"
#include "mpt/mutex/mutex.hpp"
#include "openmpt/sounddevice/SoundDevice.hpp"
#include "openmpt/sounddevice/SoundDeviceBuffer.hpp"

#include <Msctf.h>

OPENMPT_NAMESPACE_BEGIN

class CAutoSaver;
class CDLSBank;
class CInputHandler;
class CModDoc;
class QuickStartDlg;
struct UpdateCheckResult;
struct UpdateHint;
struct MODPLUGDIB;
enum class MidiSetup: int32;
enum class MainToolBarItem : uint8;
enum SoundDeviceStopMode : int;
namespace SoundDevice {
class Base;
class ICallback;
} // namerspace SoundDevice

#define MAINFRAME_TITLE _T("Open ModPlug Tracker")


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
	OPTIONS_PAGE_WINE,
};


/////////////////////////////////////////////////////////////////////////
// Player position notification

#define MAX_UPDATE_HISTORY		2000 // 2 seconds with 1 ms updates

template<> inline SettingValue ToSettingValue(const WINDOWPLACEMENT &val)
{
	return SettingValue(EncodeBinarySetting<WINDOWPLACEMENT>(val), "WINDOWPLACEMENT");
}
template<> inline WINDOWPLACEMENT FromSettingValue(const SettingValue &val)
{
	MPT_ASSERT(val.GetTypeTag() == "WINDOWPLACEMENT");
	return DecodeBinarySetting<WINDOWPLACEMENT>(val.as<std::vector<std::byte> >());
}


class VUMeter
	: public IMonitorInput
	, public IMonitorOutput
{
public:
	static constexpr std::size_t maxChannels = 4;
	static const float dynamicRange; // corresponds to the current implementation of the UI widget diplaying the result
	struct Channel
	{
		int32 peak = 0;
		bool clipped = false;
	};
private:
	Channel channels[maxChannels];
	int32 decayParam;
	void Process(Channel &channel, MixSampleInt sample);
	void Process(Channel &channel, MixSampleFloat sample);
public:
	VUMeter() : decayParam(0) { SetDecaySpeedDecibelPerSecond(88.0f); }
	void SetDecaySpeedDecibelPerSecond(float decibelPerSecond);
public:
	const Channel & operator [] (std::size_t channel) const { return channels[channel]; }
	void Process(mpt::audio_span_interleaved<const MixSampleInt> buffer);
	void Process(mpt::audio_span_planar<const MixSampleInt> buffer);
	void Process(mpt::audio_span_interleaved<const MixSampleFloat> buffer);
	void Process(mpt::audio_span_planar<const MixSampleFloat> buffer);
	void Decay(int32 secondsNum, int32 secondsDen);
	void ResetClipped();
};


class TfLanguageProfileNotifySink : public ITfLanguageProfileNotifySink
{
public:
	TfLanguageProfileNotifySink();
	~TfLanguageProfileNotifySink();

	HRESULT STDMETHODCALLTYPE OnLanguageChange(LANGID langid, __RPC__out BOOL *pfAccept) override;
	HRESULT STDMETHODCALLTYPE OnLanguageChanged() override;
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;

protected:
	ITfInputProcessorProfiles *m_pProfiles = nullptr;
	ITfSource *m_pSource = nullptr;
	DWORD m_dwCookie = TF_INVALID_COOKIE;
};


class CMainFrame
	: public CMDIFrameWnd
	, public SoundDevice::CallbackBufferHandler<DithersOpenMPT>
	, public SoundDevice::IMessageReceiver
	, public TfLanguageProfileNotifySink
{
	DECLARE_DYNAMIC(CMainFrame)
	// static data
public:

	// Globals
	static OptionsPage m_nLastOptionsPage;
	static HHOOK g_focusHook;

	// GDI
	CFont m_hCommentsFont;
	static CFont m_hGUIFont;
	static HICON m_hIcon;
	static HPEN penDarkGray, penHalfDarkGray, penGray99;
	static HCURSOR curDragging, curNoDrop, curArrow, curNoDrop2, curVSplit;
	static MODPLUGDIB *bmpNotes;
	static COLORREF gcolrefVuMeter[NUM_VUMETER_PENS * 2];	// General tab VU meters

public:

	// Low-Level Audio
	CriticalSection m_SoundDeviceFillBufferCriticalSection;
	Util::MultimediaClock m_SoundDeviceClock;
	SoundDevice::IBase *gpSoundDevice = nullptr;
	UINT_PTR m_NotifyTimer = 0;
	VUMeter m_VUMeterInput;
	VUMeter m_VUMeterOutput;

	DWORD m_AudioThreadId = 0;
	bool m_InNotifyHandler = false;

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
	CSoundFile *m_pSndFile = nullptr; // != NULL only when currently playing or rendering
	HWND m_hWndMidi = nullptr;
	samplecount_t m_dwTimeSec = 0;
	UINT_PTR m_nTimer = 0;
	UINT m_nAvgMixChn = 0, m_nMixChn = 0;
	uint32 m_currentSpeed = 0;
	// Misc
	class COptionsSoundcard *m_SoundCardOptionsDialog = nullptr;
#if defined(MPT_ENABLE_UPDATE)
	class CUpdateSetupDlg *m_UpdateOptionsDialog = nullptr;
	std::unique_ptr<UpdateCheckResult> m_updateCheckResult;
	bool m_cancelUpdateCheck = false;
#endif // MPT_ENABLE_UPDATE
	DWORD m_helpCookie = 0;
	bool m_bOptionsLocked = false;

	// Notification Buffer
	mpt::mutex m_NotificationBufferMutex; // to avoid deadlocks, this mutex should only be taken as a innermost lock, i.e. do not block on anything while holding this mutex
	Util::fixed_size_queue<Notification, MAX_UPDATE_HISTORY> m_NotifyBuffer;

	// Instrument preview in tree view
	CSoundFile m_WaveFile;
	ModSample m_metronomeMeasure{}, m_metronomeBeat{};

	TCHAR m_szUserText[512], m_szInfoText[512], m_szXInfoText[512];

	mpt::heap_value<CAutoSaver> m_AutoSaver;
	mpt::heap_value<CInputHandler> m_InputHandler;

public:
	bool m_bModTreeHasFocus = false;

public:
	CMainFrame();
	void Initialize();


// Low-Level Audio
public:
	static void UpdateDspEffects(CSoundFile &sndFile, bool reset=false);
	static void UpdateAudioParameters(CSoundFile &sndFile, bool reset=false);

	// from SoundDevice::IBufferHandler
	uint64 SoundCallbackGetReferenceClockNowNanoseconds() const override;
	void SoundCallbackPreStart() override;
	void SoundCallbackPostStop() override;
	bool SoundCallbackIsLockedByCurrentThread() const override;
	void SoundCallbackLock() override;
	uint64 SoundCallbackLockedGetReferenceClockNowNanoseconds() const override;
	void SoundCallbackLockedProcessPrepare(SoundDevice::TimeInfo timeInfo) override;
	void SoundCallbackLockedCallback(SoundDevice::CallbackBuffer<DithersOpenMPT> &buffer) override;
	void SoundCallbackLockedProcessDone(SoundDevice::TimeInfo timeInfo) override;
	void SoundCallbackUnlock() override;

	// from SoundDevice::IMessageReceiver
	void SoundDeviceMessage(LogLevel level, const mpt::ustring &str) override;

	bool InGuiThread() const noexcept;
	bool InAudioThread() const noexcept { return GetCurrentThreadId() == m_AudioThreadId; }
	bool InNotifyHandler() const noexcept { return m_InNotifyHandler; }

	bool audioOpenDevice();
	void audioCloseDevice();
	bool IsAudioDeviceOpen() const;
	bool DoNotification(DWORD dwSamplesRead, int64 streamPosition);

// Midi Input Functions
public:
	bool midiOpenDevice(bool showSettings = true);
	void midiCloseDevice();
	void SetMidiRecordWnd(HWND hwnd) { m_hWndMidi = hwnd; }
	HWND GetMidiRecordWnd() const { return m_hWndMidi; }
	void LoadMetronomeSamples();
	void UpdateMetronomeSamples();
	void UpdateMetronomeVolume();

	static int ApplyVolumeRelatedSettings(const DWORD &dwParam1, const uint8 midivolume);

// static functions
public:
	static CMainFrame *GetMainFrame() noexcept;
	static void UpdateColors();
	static HICON GetModIcon() { return m_hIcon; }
	static HFONT GetGUIFont() { return m_hGUIFont; }
	static LRESULT CALLBACK FocusChangeProc(int code, WPARAM wParam, LPARAM lParam);

	// Misc functions
public:
	CFont &GetCommentsFont() { return m_hCommentsFont; }

	void SetUserText(LPCTSTR lpszText);
	void SetInfoText(LPCTSTR lpszText);
	void SetXInfoText(LPCTSTR lpszText);
	void SetHelpText(LPCTSTR lpszText);
	CString GetHelpText() const;
	UINT GetBaseOctave() const;
	CModDoc *GetActiveDoc() const;
	CView *GetActiveView() const;
	void OnDocumentCreated(CModDoc *pModDoc);
	void OnDocumentClosed(CModDoc *pModDoc);
	void UpdateTree(CModDoc *pModDoc, UpdateHint hint, CObject *pHint = nullptr);
	void RefreshDlsBanks();
	static CInputHandler *GetInputHandler();
	void SetElapsedTime(double t) { m_dwTimeSec = mpt::saturate_trunc<samplecount_t>(t * 10.0); }

#if defined(MPT_ENABLE_UPDATE)
	bool ShowUpdateIndicator(const UpdateCheckResult &result, const CString &releaseVersion, const CString &infoURL, bool showHighlight);
#endif // MPT_ENABLE_UPDATE

	CModTree *GetUpperTreeview() { return m_wndTree.m_pModTree; }
	CModTree *GetLowerTreeview() { return m_wndTree.m_pModTreeData; }
	bool SetTreeSoundfile(FileReader &file) { return m_wndTree.SetTreeSoundfile(file); }

	void CreateExampleModulesMenu();
	void CreateTemplateModulesMenu();
	static std::pair<CMenu *, int> FindMenuItemByCommand(CMenu &menu, UINT commandID);

	// Creates submenu whose items are filenames of files in both
	// AppDirectory\folderName\ (usually C:\Program Files\OpenMPT\folderName\)
	// and
	// ConfigDirectory\folderName (usually %appdata%\OpenMPT\folderName\)
	// [in] maxCount: Maximum number of items allowed in the menu
	// [out] paths: Receives the full paths of the files added to the menu.
	// [in] folderName: Name of the folder
	// [in] idRangeBegin: First ID for the menu item.
	static HMENU CreateFileMenu(const size_t maxCount, std::vector<mpt::PathString> &paths, const mpt::PathString &folderName, const uint16 idRangeBegin);

// Player functions
public:

	// High-level synchronous playback functions, do not hold AudioCriticalSection while calling these
	bool PreparePlayback();
	bool StartPlayback();
	void StopPlayback();
	bool RestartPlayback();
	bool PausePlayback();
	static bool IsValidSoundFile(CSoundFile &sndFile) { return sndFile.GetType() != MOD_TYPE_NONE; }
	static bool IsValidSoundFile(CSoundFile *pSndFile) { return pSndFile && pSndFile->GetType(); }
	void SetPlaybackSoundFile(CSoundFile *pSndFile);
	void UnsetPlaybackSoundFile();
	void GenerateStopNotification();

	bool PlayMod(CModDoc *);
	bool StopMod(CModDoc *pDoc = nullptr);
	bool PauseMod(CModDoc *pDoc = nullptr);

	bool StopSoundFile(CSoundFile *);
	bool PlaySoundFile(CSoundFile *);
	bool PlaySoundFile(const mpt::PathString &filename, ModCommand::NOTE note, int volume = -1);
	bool PlaySoundFile(CSoundFile &sndFile, INSTRUMENTINDEX nInstrument, SAMPLEINDEX nSample, ModCommand::NOTE note, int volume = -1);
	bool PlayDLSInstrument(const CDLSBank &bank, UINT instr, UINT region, ModCommand::NOTE note, int volume = -1);

	void InitPreview();
	void PreparePreview(ModCommand::NOTE note, int volume);
	void StopPreview() { StopSoundFile(&m_WaveFile); }
	void PlayPreview() { PlaySoundFile(&m_WaveFile); }

	inline bool IsPlaying() const { return m_pSndFile != nullptr; }
	// Return currently playing module (nullptr if none is playing)
	inline CModDoc *GetModPlaying() const { return m_pSndFile ? m_pSndFile->GetpModDoc() : nullptr; }
	// Return currently playing module (nullptr if none is playing)
	inline CSoundFile *GetSoundFilePlaying() const { return m_pSndFile; }
	void InitRenderer(CSoundFile *);
	void StopRenderer(CSoundFile *);
	void SwitchToActiveView();

	void IdleHandlerSounddevice();

	void ResetSoundCard();
	void SetupSoundCard(SoundDevice::Settings deviceSettings, SoundDevice::Identifier deviceIdentifier, SoundDeviceStopMode stoppedMode, bool forceReset = false);
	void SetupMiscOptions();
	void SetupPlayer();

	void SetupMidi(FlagSet<MidiSetup> d, UINT n);
	HWND GetFollowSong() const;
	HWND GetFollowSong(const CModDoc *pDoc) const { return (pDoc == GetModPlaying()) ? GetFollowSong() : nullptr; }
	void ResetNotificationBuffer();

	// Notify accessbility software that it should read out updated UI elements
	void NotifyAccessibilityUpdate(CWnd &source);

	void UpdateDocumentCount();

// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	BOOL PreCreateWindow(CREATESTRUCT &cs) override;
	BOOL PreTranslateMessage(MSG *pMsg) override;
	BOOL DestroyWindow() override;
	void OnUpdateFrameTitle(BOOL bAddToTitle) override;
	//}}AFX_VIRTUAL

	/// Opens either template or example menu item.
	void OpenMenuItemFile(const UINT nId, const bool isTemplateFile);

	void ShowToolbarMenu(CPoint screenPt);
	void AddToolBarMenuEntries(CMenu &menu) const;

	void RecreateImageLists();
	void SetupStatusBarSizes();

public:
	void UpdateMRUList();

// Implementation
public:
	~CMainFrame() override;
	void RecalcLayout(BOOL notify = TRUE) override;
#ifdef _DEBUG
	void AssertValid() const override;
	void Dump(CDumpContext& dc) const override;
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
	afx_msg LRESULT OnDPIChanged(WPARAM, LPARAM);

	afx_msg void OnPluginManager();
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
	afx_msg LRESULT OnUpdateViews(WPARAM modDoc, LPARAM hint);
	afx_msg LRESULT OnSetModified(WPARAM modDoc, LPARAM);
	afx_msg void OnOpenTemplateModule(UINT nId);
	afx_msg void OnExampleSong(UINT nId);
	afx_msg void OnOpenMRUItem(UINT nId);
	afx_msg void OnUpdateMRUItem(CCmdUI *cmd);
	afx_msg LRESULT OnInvalidatePatterns(WPARAM, LPARAM);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	afx_msg void OnInternetUpdate();
	afx_msg void OnUpdateAvailable();
	afx_msg void OnShowSettingsFolder();
#if defined(MPT_ENABLE_UPDATE)
	afx_msg LRESULT OnUpdateCheckStart(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnUpdateCheckProgress(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnUpdateCheckCanceled(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnUpdateCheckFailure(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnUpdateCheckSuccess(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnToolbarUpdateIndicatorClick(WPARAM wparam, LPARAM lparam);
#endif // MPT_ENABLE_UPDATE
	afx_msg void OnHelp();
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnActivateApp(BOOL active, DWORD threadID);
	afx_msg void OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized);

	afx_msg void OnToggleMainBarShowOctave();
	afx_msg void OnToggleMainBarShowTempo();
	afx_msg void OnToggleMainBarShowSpeed();
	afx_msg void OnToggleMainBarShowRowsPerBeat();
	afx_msg void OnToggleMainBarShowGlobalVolume();
	afx_msg void OnToggleMainBarShowVUMeter();
	afx_msg void OnToggleMainBarItem(MainToolBarItem item, UINT menuID);
	afx_msg void OnToggleTreeViewOnLeft();

	afx_msg void OnCreateMixerDump();
	afx_msg void OnVerifyMixerDump();
	afx_msg void OnConvertMixerDumpToText();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	bool UpdateEffectKeys(const CModDoc *modDoc);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	// Defines maximum number of items in example modules menu.
	static constexpr size_t nMaxItemsInExampleModulesMenu = 50;
	static constexpr size_t nMaxItemsInTemplateModulesMenu = 50;

private:
	/// Array of paths of example modules that are available from help menu.
	std::vector<mpt::PathString> m_ExampleModulePaths;
	/// Array of paths of template modules that are available from file menu.
	std::vector<mpt::PathString> m_TemplateModulePaths;

	std::unique_ptr<QuickStartDlg> m_quickStartDlg;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


OPENMPT_NAMESPACE_END
