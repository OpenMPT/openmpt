/*
 * ModDoc.h
 * --------
 * Purpose: Converting between various module formats.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "Sndfile.h"
#include "../common/misc_util.h"
#include "Undo.h"
#include "Notification.h"
#include "UpdateHints.h"
#include <time.h>

OPENMPT_NAMESPACE_BEGIN

class EncoderFactoryBase;
class CChildFrame;

/////////////////////////////////////////////////////////////////////////
// Split Keyboard Settings (pattern editor)

//==========================
struct SplitKeyboardSettings
//==========================
{
	enum
	{
		splitOctaveRange = 9,
	};

	bool IsSplitActive() const { return (octaveLink && (octaveModifier != 0)) || (splitInstrument > 0) || (splitVolume != 0); }

	int octaveModifier;	// determines by how many octaves the notes should be transposed up or down
	ModCommand::NOTE splitNote;
	ModCommand::INSTR splitInstrument;
	ModCommand::VOL splitVolume;
	bool octaveLink;	// apply octaveModifier

	SplitKeyboardSettings()
	//---------------------
	{
		splitInstrument = 0;
		splitNote = NOTE_MIDDLEC - 1;
		splitVolume = 0;
		octaveModifier = 0;
		octaveLink = false;
	}
};

enum InputTargetContext;


struct LogEntry
{
	LogLevel level;
	mpt::ustring message;
	LogEntry() : level(LogInformation) {}
	LogEntry(LogLevel l, const mpt::ustring &m) : level(l), message(m) {}
};


enum LogMode
{
	LogModeInstantReporting,
	LogModeGather,
};


class ScopedLogCapturer
{
private:
	CModDoc &m_modDoc;
	LogMode m_oldLogMode;
	std::string m_title;
	CWnd *m_pParent;
	bool m_showLog;
public:
	ScopedLogCapturer(CModDoc &modDoc, const std::string &title = "", CWnd *parent = nullptr, bool showLog = true);
	~ScopedLogCapturer();
	void ShowLog(bool force = false);
	void ShowLog(const std::string &preamble, bool force = false);
	void ShowLog(const std::wstring &preamble, bool force = false);
};


//=============================
class CModDoc: public CDocument
//=============================
{
protected:
	friend ScopedLogCapturer;
	mutable std::vector<LogEntry> m_Log;
	LogMode m_LogMode;
	CSoundFile m_SndFile;

	HWND m_hWndFollow;
	FlagSet<Notification::Type, uint16> m_notifyType;
	Notification::Item m_notifyItem;
	CSize m_szOldPatternScrollbarsPos;

	CPatternUndo m_PatternUndo;
	CSampleUndo m_SampleUndo;
	SplitKeyboardSettings m_SplitKeyboardSettings;	// this is maybe not the best place to keep them, but it should do the job
	time_t m_creationTime;

	bool bModifiedAutosave; // Modified since last autosave?
	bool m_ShowSavedialog;
public:
	bool m_bHasValidPath; //becomes true if document is loaded or saved.

protected:
	std::bitset<MAX_BASECHANNELS> m_bsMultiRecordMask;
	std::bitset<MAX_BASECHANNELS> m_bsMultiSplitRecordMask;

protected: // create from serialization only
	CModDoc();
	DECLARE_SERIAL(CModDoc)

// public members
public:
	CSoundFile *GetSoundFile() { return &m_SndFile; }
	const CSoundFile *GetSoundFile() const { return &m_SndFile; }
	CSoundFile &GetrSoundFile() { return m_SndFile; }
	const CSoundFile &GetrSoundFile() const { return m_SndFile; }

	void SetModified(BOOL bModified=TRUE) { SetModifiedFlag(bModified); bModifiedAutosave = (bModified != FALSE); }
	bool ModifiedSinceLastAutosave() { bool bRetval = bModifiedAutosave; bModifiedAutosave = false; return bRetval; } // return "IsModified" value and reset it until the next SetModified() (as this is only used for polling)
	void SetShowSaveDialog(bool b) {m_ShowSavedialog = b;}
	void PostMessageToAllViews(UINT uMsg, WPARAM wParam=0, LPARAM lParam=0);
	void SendMessageToActiveViews(UINT uMsg, WPARAM wParam=0, LPARAM lParam=0);
	MODTYPE GetModType() const { return m_SndFile.m_nType; }
	INSTRUMENTINDEX GetNumInstruments() const { return m_SndFile.m_nInstruments; }
	SAMPLEINDEX GetNumSamples() const { return m_SndFile.m_nSamples; }

	// Logging for general progress and error events.
	void AddToLog(LogLevel level, const mpt::ustring &text) const;
	/*MPT_DEPRECATED*/ void AddToLog(const std::string &text) const { AddToLog(LogInformation, mpt::ToUnicode(mpt::CharsetLocale, text)); }

	const std::vector<LogEntry> & GetLog() const { return m_Log; }
	mpt::ustring GetLogString() const;
	LogLevel GetMaxLogLevel() const;
protected:
	LogMode GetLogMode() const { return m_LogMode; }
	void SetLogMode(LogMode mode) { m_LogMode = mode; }
	void ClearLog();
	UINT ShowLog(const std::string &preamble, const std::string &title = "", CWnd *parent = nullptr);
	UINT ShowLog(const std::wstring &preamble, const std::wstring &title = std::wstring(), CWnd *parent = nullptr);
	UINT ShowLog(const std::string &title = "", CWnd *parent = nullptr) { return ShowLog("", title, parent); }

public:

	void ClearFilePath() { m_strPathName.Empty(); }

	void ViewPattern(UINT nPat, UINT nOrd);
	void ViewSample(UINT nSmp);
	void ViewInstrument(UINT nIns);
	HWND GetFollowWnd() const { return m_hWndFollow; }
	void SetFollowWnd(HWND hwnd);

	void SetNotifications(FlagSet<Notification::Type> type, Notification::Item item = 0) { m_notifyType = type; m_notifyItem = item; }
	FlagSet<Notification::Type, uint16> GetNotificationType() const { return m_notifyType; }
	Notification::Item GetNotificationItem() const { return m_notifyItem; }

	void ActivateWindow();

	void OnSongProperties();

	void PrepareUndoForAllPatterns(bool storeChannelInfo = false, const char *description = "");
	CPatternUndo &GetPatternUndo() { return m_PatternUndo; }
	CSampleUndo &GetSampleUndo() { return m_SampleUndo; }
	SplitKeyboardSettings &GetSplitKeyboardSettings() { return m_SplitKeyboardSettings; }

	time_t GetCreationTime() const { return m_creationTime; }

// operations
public:
	bool ChangeModType(MODTYPE wType);

	bool ChangeNumChannels(CHANNELINDEX nNewChannels, const bool showCancelInRemoveDlg = true);
	bool RemoveChannels(const std::vector<bool> &keepMask);
	CHANNELINDEX ReArrangeChannels(const std::vector<CHANNELINDEX> &fromToArray, const bool createUndoPoint = true);
	void CheckUsedChannels(std::vector<bool> &usedMask, CHANNELINDEX maxRemoveCount = MAX_BASECHANNELS) const;

	SAMPLEINDEX ReArrangeSamples(const std::vector<SAMPLEINDEX> &newOrder);

	INSTRUMENTINDEX ReArrangeInstruments(const std::vector<INSTRUMENTINDEX> &newOrder, deleteInstrumentSamples removeSamples = doNoDeleteAssociatedSamples);

	bool ConvertInstrumentsToSamples();
	bool ConvertSamplesToInstruments();
	PLUGINDEX RemovePlugs(const std::vector<bool> &keepMask);

	void ClonePlugin(SNDMIXPLUGIN &target, const SNDMIXPLUGIN &source);
	void AppendModule(const CSoundFile &source);

	PATTERNINDEX InsertPattern(ORDERINDEX nOrd = ORDERINDEX_INVALID, ROWINDEX nRows = 64);
	SAMPLEINDEX InsertSample(bool bLimit = false);
	INSTRUMENTINDEX InsertInstrument(SAMPLEINDEX lSample = SAMPLEINDEX_INVALID, INSTRUMENTINDEX lDuplicate = INSTRUMENTINDEX_INVALID);
	INSTRUMENTINDEX InsertInstrumentForPlugin(PLUGINDEX plug);
	INSTRUMENTINDEX HasInstrumentForPlugin(PLUGINDEX plug) const;
	void InitializeInstrument(ModInstrument *pIns);
	bool RemoveOrder(SEQUENCEINDEX nSeq, ORDERINDEX nOrd);
	bool RemovePattern(PATTERNINDEX nPat);
	bool RemoveSample(SAMPLEINDEX nSmp);
	bool RemoveInstrument(INSTRUMENTINDEX nIns);

	void ProcessMIDI(uint32 midiData, INSTRUMENTINDEX ins, IMixPlugin *plugin, InputTargetContext ctx);
	CHANNELINDEX PlayNote(UINT note, INSTRUMENTINDEX nins, SAMPLEINDEX nsmp, bool pause, LONG nVol=-1, SmpLength loopStart = 0, SmpLength loopEnd = 0, CHANNELINDEX nCurrentChn = CHANNELINDEX_INVALID, const SmpLength sampleOffset = 0);
	bool NoteOff(UINT note, bool fade = false, INSTRUMENTINDEX ins = INSTRUMENTINDEX_INVALID, CHANNELINDEX currentChn = CHANNELINDEX_INVALID, CHANNELINDEX stopChn = CHANNELINDEX_INVALID); //rewbs.vstiLive: add params
	void CheckNNA(ModCommand::NOTE note, INSTRUMENTINDEX ins, std::bitset<128> &playingNotes);

	bool IsNotePlaying(UINT note, SAMPLEINDEX nsmp = 0, INSTRUMENTINDEX nins = 0);
	bool MuteChannel(CHANNELINDEX nChn, bool bMute);
	bool UpdateChannelMuteStatus(CHANNELINDEX nChn);
	bool MuteSample(SAMPLEINDEX nSample, bool bMute);
	bool MuteInstrument(INSTRUMENTINDEX nInstr, bool bMute);

	bool SoloChannel(CHANNELINDEX nChn, bool bSolo);
	bool IsChannelSolo(CHANNELINDEX nChn) const;

	bool SurroundChannel(CHANNELINDEX nChn, bool bSurround);
	bool SetChannelGlobalVolume(CHANNELINDEX nChn, uint16 nVolume);
	bool SetChannelDefaultPan(CHANNELINDEX nChn, uint16 nPan);
	bool IsChannelMuted(CHANNELINDEX nChn) const;
	bool IsSampleMuted(SAMPLEINDEX nSample) const;
	bool IsInstrumentMuted(INSTRUMENTINDEX nInstr) const;

	bool NoFxChannel(CHANNELINDEX nChn, bool bNoFx, bool updateMix = true);
	bool IsChannelNoFx(CHANNELINDEX nChn) const;
	bool IsChannelRecord1(CHANNELINDEX channel) const;
	bool IsChannelRecord2(CHANNELINDEX channel) const;
	BYTE IsChannelRecord(CHANNELINDEX channel) const;
	void Record1Channel(CHANNELINDEX channel, bool select = true);
	void Record2Channel(CHANNELINDEX channel, bool select = true);
	void ReinitRecordState(bool unselect = true);

	CHANNELINDEX GetNumChannels() const { return m_SndFile.m_nChannels; }
	UINT GetPatternSize(PATTERNINDEX nPat) const;
	bool IsChildSample(INSTRUMENTINDEX nIns, SAMPLEINDEX nSmp) const;
	INSTRUMENTINDEX FindSampleParent(SAMPLEINDEX sample) const;
	SAMPLEINDEX FindInstrumentChild(INSTRUMENTINDEX nIns) const;
	bool MoveOrder(ORDERINDEX nSourceNdx, ORDERINDEX nDestNdx, bool bUpdate = true, bool bCopy = false, SEQUENCEINDEX nSourceSeq = SEQUENCEINDEX_INVALID, SEQUENCEINDEX nDestSeq = SEQUENCEINDEX_INVALID);
	BOOL ExpandPattern(PATTERNINDEX nPattern);
	BOOL ShrinkPattern(PATTERNINDEX nPattern);

	bool CopyEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv);
	bool SaveEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv, const mpt::PathString &fileName);
	bool PasteEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv);
	bool LoadEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv, const mpt::PathString &fileName);

	LRESULT ActivateView(UINT nIdView, DWORD dwParam);
	void UpdateAllViews(CView *pSender, UpdateHint hint, CObject *pHint=NULL);
	HWND GetEditPosition(ROWINDEX &row, PATTERNINDEX &pat, ORDERINDEX &ord);
	LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	void TogglePluginEditor(UINT m_nCurrentPlugin, bool onlyThisEditor = false);
	void RecordParamChange(PLUGINDEX slot, PlugParamIndex param);
	void LearnMacro(int macro, PlugParamIndex param);
	void SetElapsedTime(ORDERINDEX nOrd, ROWINDEX nRow, bool setSamplePos);

	// Global settings to pattern effect conversion
	bool GlobalVolumeToPattern();

	bool HasMPTHacks(const bool autofix = false);

	void FixNullStrings();

// Fix: save pattern scrollbar position when switching to other tab
	CSize GetOldPatternScrollbarsPos() const { return m_szOldPatternScrollbarsPos; };
	void SetOldPatternScrollbarsPos( CSize s ){ m_szOldPatternScrollbarsPos = s; };

	void OnFileMP3Convert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder, bool showWarning = false);
	void OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder);
	void OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder, const std::vector<EncoderFactoryBase*> &encFactories);

	// Returns formatted ModInstrument name.
	// [in] bEmptyInsteadOfNoName: In case of unnamed instrument string, "(no name)" is returned unless this
	//                             parameter is true is case which an empty name is returned.
	// [in] bIncludeIndex: True to include instrument index in front of the instrument name, false otherwise.
	CString GetPatternViewInstrumentName(INSTRUMENTINDEX nInstr, bool bEmptyInsteadOfNoName = false, bool bIncludeIndex = true) const;

	// Check if a given channel contains data.
	bool IsChannelUnused(CHANNELINDEX nChn) const;

// protected members
protected:

	BOOL InitializeMod();
	CChildFrame *GetChildFrame(); //rewbs.customKeys

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModDoc)
	public:
	virtual BOOL OnNewDocument();
	MPT_DEPRECATED_PATH virtual BOOL OnOpenDocument(LPCTSTR lpszPathName)
	{
		return OnOpenDocument(lpszPathName ? mpt::PathString::TunnelOutofCString(lpszPathName) : mpt::PathString());
	}
	virtual BOOL OnOpenDocument(const mpt::PathString &filename);
	MPT_DEPRECATED_PATH virtual BOOL OnSaveDocument(LPCTSTR lpszPathName)
	{
		return OnSaveDocument(lpszPathName ? mpt::PathString::TunnelOutofCString(lpszPathName) : mpt::PathString(), false);
	}
	BOOL OnSaveDocument(const mpt::PathString &filename)
	{
		return OnSaveDocument(filename, false);
	}
	virtual void OnCloseDocument();
	void SafeFileClose();
	BOOL OnSaveDocument(const mpt::PathString &filename, const bool bTemplateFile);

	MPT_DEPRECATED_PATH virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE)
	{
		return SetPathName(lpszPathName ? mpt::PathString::TunnelOutofCString(lpszPathName) : mpt::PathString(), bAddToMRU);
	}
	virtual void SetPathName(const mpt::PathString &filename, BOOL bAddToMRU = TRUE)
	{
		CDocument::SetPathName(mpt::PathString::TunnelIntoCString(filename), bAddToMRU);
		#ifndef UNICODE
			// As paths are faked into utf8 when !UNICODE,
			// explicitly set the title in locale again.
			// This replaces non-ANSI characters in the title
			// with replacement character but overall the
			// unicode handling is sane and consistent this
			// way.
			SetTitle(mpt::ToCString((filename.GetFileName() + filename.GetFileExt()).ToWide()));
		#endif
	}
	MPT_DEPRECATED_PATH const CString& GetPathName() const
	{
		return CDocument::GetPathName();
	}
	mpt::PathString GetPathNameMpt() const
	{
		return mpt::PathString::TunnelOutofCString(CDocument::GetPathName());
	}

	virtual BOOL SaveModified();
	bool SaveAllSamples();
	bool SaveSample(SAMPLEINDEX smp);

#ifndef UNICODE
	// MFC checks for writeable filename in there and issues SaveAs dialog if not.
	// This fails for our utf8-in-CString hack.
	virtual BOOL DoFileSave();
#endif

	MPT_DEPRECATED_PATH virtual BOOL DoSave(LPCSTR lpszPathName, BOOL bSaveAs=TRUE)
	{
		return DoSave(lpszPathName ? mpt::PathString::TunnelOutofCString(lpszPathName) : mpt::PathString(), bSaveAs);
	}
	virtual BOOL DoSave(const mpt::PathString &filename, BOOL bSaveAs=TRUE);
	virtual void DeleteContents();
	virtual void SetModifiedFlag(BOOL bModified=TRUE);
	//}}AFX_VIRTUAL

	uint8 GetPlaybackMidiChannel(const ModInstrument *pIns, CHANNELINDEX nChn) const;

	// Get the sample index for the current pattern cell (resolves instrument note maps, etc)
	SAMPLEINDEX GetSampleIndex(const ModCommand &m) const;

	// Convert a linear volume property to decibels
	static CString LinearToDecibels(double value, double valueAtZeroDB);
	// Convert a panning value to a more readable string
	static CString PanningToString(int32 value, int32 valueAtCenter);

	void SerializeViews() const;
	void DeserializeViews();

	// View MIDI Mapping dialog for given plugin and parameter combination.
	void ViewMIDIMapping(PLUGINDEX plugin = PLUGINDEX_INVALID, PlugParamIndex param = 0);

// Implementation
public:
	virtual ~CModDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
public:
	//{{AFX_MSG(CModDoc)
	afx_msg void OnFileWaveConvert();
	afx_msg void OnFileMP3Convert();
	afx_msg void OnFileMidiConvert();
	afx_msg void OnFileCompatibilitySave();
	afx_msg void OnPlayerPlay();
	afx_msg void OnPlayerStop();
	afx_msg void OnPlayerPause();
	afx_msg void OnPlayerPlayFromStart();
	afx_msg void OnPanic();
	afx_msg void OnEditGlobals();
	afx_msg void OnEditPatterns();
	afx_msg void OnEditSamples();
	afx_msg void OnEditInstruments();
	afx_msg void OnEditComments();
	afx_msg void OnInsertPattern();
	afx_msg void OnInsertSample();
	afx_msg void OnInsertInstrument();
	afx_msg void OnShowCleanup();
	afx_msg void OnSetupZxxMacros();
	afx_msg void OnEstimateSongLength();
	afx_msg void OnApproximateBPM();
	afx_msg void OnUpdateXMITMPTOnly(CCmdUI *p);
	afx_msg void OnUpdateITMPTOnly(CCmdUI *p);
	afx_msg void OnUpdateHasMIDIMappings(CCmdUI *p);
	afx_msg void OnUpdateCompatExportableOnly(CCmdUI *p);
	afx_msg void OnPatternRestart() { OnPatternRestart(true); } //rewbs.customKeys
	afx_msg void OnPatternRestart(bool loop); //rewbs.customKeys
	afx_msg void OnPatternPlay(); //rewbs.customKeys
	afx_msg void OnPatternPlayNoLoop(); //rewbs.customKeys
	afx_msg void OnViewEditHistory();
	afx_msg void OnViewMPTHacks();
	afx_msg void OnSaveTemplateModule();
	afx_msg void OnAppendModule();
	afx_msg void OnViewMIDIMapping() { ViewMIDIMapping(); }
	afx_msg void OnChannelManager();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:

	void ChangeFileExtension(MODTYPE nNewType);
	CHANNELINDEX FindAvailableChannel() const;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


OPENMPT_NAMESPACE_END
