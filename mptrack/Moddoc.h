/*
 * ModDoc.h
 * --------
 * Purpose: Converting between various module formats.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

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

struct SplitKeyboardSettings
{
	enum
	{
		splitOctaveRange = 9,
	};

	bool IsSplitActive() const { return (octaveLink && (octaveModifier != 0)) || (splitInstrument != 0) || (splitVolume != 0); }

	int octaveModifier = 0;	// determines by how many octaves the notes should be transposed up or down
	ModCommand::NOTE splitNote = NOTE_MIDDLEC - 1;
	ModCommand::INSTR splitInstrument = 0;
	ModCommand::VOL splitVolume = 0;
	bool octaveLink = false;	// apply octaveModifier
};

enum InputTargetContext : int8;


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
	CString m_title;
	CWnd *m_pParent;
	bool m_showLog;
public:
	ScopedLogCapturer(CModDoc &modDoc, const CString &title = {}, CWnd *parent = nullptr, bool showLog = true);
	~ScopedLogCapturer();
	void ShowLog(bool force = false);
	void ShowLog(const CString &preamble, bool force = false);
	[[deprecated]] void ShowLog(const std::string &preamble, bool force = false);
	void ShowLog(const mpt::ustring &preamble, bool force = false);
};


struct PlayNoteParam
{
	SmpLength m_loopStart = 0, m_loopEnd = 0, m_sampleOffset = 0;
	int32 m_volume = -1;
	SAMPLEINDEX m_sample = 0;
	INSTRUMENTINDEX m_instr = 0;
	CHANNELINDEX m_currentChannel = CHANNELINDEX_INVALID;
	ModCommand::NOTE m_note;

	PlayNoteParam(ModCommand::NOTE note) : m_note(note) { }

	PlayNoteParam& LoopStart(SmpLength loopStart) { m_loopStart = loopStart; return *this; }
	PlayNoteParam& LoopEnd(SmpLength loopEnd) { m_loopEnd = loopEnd; return *this; }
	PlayNoteParam& Offset(SmpLength sampleOffset) { m_sampleOffset = sampleOffset; return *this; }

	PlayNoteParam& Volume(int32 volume) { m_volume = volume; return *this; }
	PlayNoteParam& Sample(SAMPLEINDEX sample) { m_sample = sample; return *this; }
	PlayNoteParam& Instrument(INSTRUMENTINDEX instr) { m_instr = instr; return *this; }
	PlayNoteParam& Channel(CHANNELINDEX channel) { m_currentChannel = channel; return *this; }
};


enum class RecordGroup : uint8
{
	NoGroup = 0,
	Group1 = 1,
	Group2 = 2,
};


class CModDoc final : public CDocument
{
protected:
	friend ScopedLogCapturer;
	mutable std::vector<LogEntry> m_Log;
	LogMode m_LogMode = LogModeInstantReporting;
	CSoundFile m_SndFile;

	HWND m_hWndFollow = nullptr;
	FlagSet<Notification::Type, uint16> m_notifyType;
	Notification::Item m_notifyItem = 0;
	CSize m_szOldPatternScrollbarsPos = { -10, -10 };

	CPatternUndo m_PatternUndo;
	CSampleUndo m_SampleUndo;
	CInstrumentUndo m_InstrumentUndo;
	SplitKeyboardSettings m_SplitKeyboardSettings;	// this is maybe not the best place to keep them, but it should do the job
	time_t m_creationTime;

	std::atomic<bool> m_modifiedAutosave = false; // Modified since last autosave?

public:
	class NoteToChannelMap : public std::array<CHANNELINDEX, NOTE_MAX - NOTE_MIN + 1>
	{
	public:
		NoteToChannelMap() { fill(CHANNELINDEX_INVALID); }
	};
	NoteToChannelMap m_noteChannel;	// Note -> Preview channel assignment

	bool m_ShowSavedialog = false;
	bool m_bHasValidPath = false; //becomes true if document is loaded or saved.

protected:
	// Note-off event buffer for MIDI sustain pedal
	std::array<std::vector<uint32>, 16> m_midiSustainBuffer;
	std::array<std::bitset<128>, 16> m_midiPlayingNotes;
	std::bitset<16> m_midiSustainActive;

	std::bitset<MAX_BASECHANNELS> m_bsMultiRecordMask;
	std::bitset<MAX_BASECHANNELS> m_bsMultiSplitRecordMask;

protected: // create from serialization only
	CModDoc();
	DECLARE_DYNCREATE(CModDoc)

// public members
public:
	CSoundFile &GetSoundFile() { return m_SndFile; }
	const CSoundFile &GetSoundFile() const { return m_SndFile; }

#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif // MPT_COMPILER_CLANG
	bool IsModified() const { return m_bModified != FALSE; }	// Work-around: CDocument::IsModified() is not const...
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG
	void SetModified(bool modified = true);
	bool ModifiedSinceLastAutosave();
	void SetShowSaveDialog(bool b) { m_ShowSavedialog = b; }
	void PostMessageToAllViews(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
	void SendNotifyMessageToAllViews(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
	void SendMessageToActiveView(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
	MODTYPE GetModType() const { return m_SndFile.m_nType; }
	INSTRUMENTINDEX GetNumInstruments() const { return m_SndFile.m_nInstruments; }
	SAMPLEINDEX GetNumSamples() const { return m_SndFile.m_nSamples; }

	// Logging for general progress and error events.
	void AddToLog(LogLevel level, const mpt::ustring &text) const;
	/*[[deprecated]]*/ void AddToLog(const CString &text) const { AddToLog(LogInformation, mpt::ToUnicode(text)); }
	/*[[deprecated]]*/ void AddToLog(const std::string &text) const { AddToLog(LogInformation, mpt::ToUnicode(mpt::Charset::Locale, text)); }
	/*[[deprecated]]*/ void AddToLog(const char *text) const { AddToLog(LogInformation, mpt::ToUnicode(mpt::Charset::Locale, text ? text : "")); }

	const std::vector<LogEntry> & GetLog() const { return m_Log; }
	mpt::ustring GetLogString() const;
	LogLevel GetMaxLogLevel() const;
protected:
	LogMode GetLogMode() const { return m_LogMode; }
	void SetLogMode(LogMode mode) { m_LogMode = mode; }
	void ClearLog();
	UINT ShowLog(const CString &preamble, const CString &title = {}, CWnd *parent = nullptr);
	UINT ShowLog(const CString &title = {}, CWnd *parent = nullptr) { return ShowLog(_T(""), title, parent); }

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
	CInstrumentUndo &GetInstrumentUndo() { return m_InstrumentUndo; }
	SplitKeyboardSettings &GetSplitKeyboardSettings() { return m_SplitKeyboardSettings; }

	time_t GetCreationTime() const { return m_creationTime; }

// operations
public:
	bool ChangeModType(MODTYPE wType);

	bool ChangeNumChannels(CHANNELINDEX nNewChannels, const bool showCancelInRemoveDlg = true);
	bool RemoveChannels(const std::vector<bool> &keepMask, bool verbose = false);
	void CheckUsedChannels(std::vector<bool> &usedMask, CHANNELINDEX maxRemoveCount = MAX_BASECHANNELS) const;

	CHANNELINDEX ReArrangeChannels(const std::vector<CHANNELINDEX> &fromToArray, const bool createUndoPoint = true);
	SAMPLEINDEX ReArrangeSamples(const std::vector<SAMPLEINDEX> &newOrder);
	INSTRUMENTINDEX ReArrangeInstruments(const std::vector<INSTRUMENTINDEX> &newOrder, deleteInstrumentSamples removeSamples = doNoDeleteAssociatedSamples);
	SEQUENCEINDEX ReArrangeSequences(const std::vector<SEQUENCEINDEX> &newOrder);

	bool ConvertInstrumentsToSamples();
	bool ConvertSamplesToInstruments();
	PLUGINDEX RemovePlugs(const std::vector<bool> &keepMask);

	void ClonePlugin(SNDMIXPLUGIN &target, const SNDMIXPLUGIN &source);
	void AppendModule(const CSoundFile &source);

	// Create a new pattern and, if order position is specified, inserts it into the order list.
	PATTERNINDEX InsertPattern(ROWINDEX rows, ORDERINDEX ord = ORDERINDEX_INVALID);
	SAMPLEINDEX InsertSample();
	INSTRUMENTINDEX InsertInstrument(SAMPLEINDEX sample = SAMPLEINDEX_INVALID, INSTRUMENTINDEX duplicateSource = INSTRUMENTINDEX_INVALID, bool silent = false);
	INSTRUMENTINDEX InsertInstrumentForPlugin(PLUGINDEX plug);
	INSTRUMENTINDEX HasInstrumentForPlugin(PLUGINDEX plug) const;
	void InitializeInstrument(ModInstrument *pIns);
	bool RemoveOrder(SEQUENCEINDEX nSeq, ORDERINDEX nOrd);
	bool RemovePattern(PATTERNINDEX nPat);
	bool RemoveSample(SAMPLEINDEX nSmp);
	bool RemoveInstrument(INSTRUMENTINDEX nIns);

	void ProcessMIDI(uint32 midiData, INSTRUMENTINDEX ins, IMixPlugin *plugin, InputTargetContext ctx);
	CHANNELINDEX PlayNote(PlayNoteParam &params, NoteToChannelMap *noteChannel = nullptr);
	bool NoteOff(UINT note, bool fade = false, INSTRUMENTINDEX ins = INSTRUMENTINDEX_INVALID, CHANNELINDEX currentChn = CHANNELINDEX_INVALID);
	void CheckNNA(ModCommand::NOTE note, INSTRUMENTINDEX ins, std::bitset<128> &playingNotes);
	void UpdateOPLInstrument(SAMPLEINDEX smp);

	bool IsNotePlaying(UINT note, SAMPLEINDEX nsmp = 0, INSTRUMENTINDEX nins = 0);
	bool MuteChannel(CHANNELINDEX nChn, bool bMute);
	bool UpdateChannelMuteStatus(CHANNELINDEX nChn);
	bool MuteSample(SAMPLEINDEX nSample, bool bMute);
	bool MuteInstrument(INSTRUMENTINDEX nInstr, bool bMute);
	// Returns true if toggling the mute status of a channel should set the document as modified given the current module format and settings.
	bool MuteToggleModifiesDocument() const;

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

	RecordGroup GetChannelRecordGroup(CHANNELINDEX channel) const;
	void SetChannelRecordGroup(CHANNELINDEX channel, RecordGroup recordGroup);
	void ToggleChannelRecordGroup(CHANNELINDEX channel, RecordGroup recordGroup);
	void ReinitRecordState(bool unselect = true);

	CHANNELINDEX GetNumChannels() const { return m_SndFile.m_nChannels; }
	UINT GetPatternSize(PATTERNINDEX nPat) const;
	bool IsChildSample(INSTRUMENTINDEX nIns, SAMPLEINDEX nSmp) const;
	INSTRUMENTINDEX FindSampleParent(SAMPLEINDEX sample) const;
	SAMPLEINDEX FindInstrumentChild(INSTRUMENTINDEX nIns) const;
	bool MoveOrder(ORDERINDEX nSourceNdx, ORDERINDEX nDestNdx, bool bUpdate = true, bool bCopy = false, SEQUENCEINDEX nSourceSeq = SEQUENCEINDEX_INVALID, SEQUENCEINDEX nDestSeq = SEQUENCEINDEX_INVALID);
	BOOL ExpandPattern(PATTERNINDEX nPattern);
	BOOL ShrinkPattern(PATTERNINDEX nPattern);

	bool SetDefaultChannelColors();
	bool SupportsChannelColors() const { return GetModType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT); }

	bool CopyEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv);
	bool SaveEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv, const mpt::PathString &fileName);
	bool PasteEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv);
	bool LoadEnvelope(INSTRUMENTINDEX nIns, EnvelopeType nEnv, const mpt::PathString &fileName);

	LRESULT ActivateView(UINT nIdView, DWORD dwParam);
	// Notify all views of document updates (GUI thread only)
	void UpdateAllViews(CView *pSender, UpdateHint hint, CObject *pHint=NULL);
	// Notify all views of document updates (for non-GUI threads)
	void UpdateAllViews(UpdateHint hint);
	void GetEditPosition(ROWINDEX &row, PATTERNINDEX &pat, ORDERINDEX &ord);
	LRESULT OnCustomKeyMsg(WPARAM, LPARAM);
	void TogglePluginEditor(UINT m_nCurrentPlugin, bool onlyThisEditor = false);
	void RecordParamChange(PLUGINDEX slot, PlugParamIndex param);
	void LearnMacro(int macro, PlugParamIndex param);
	void SetElapsedTime(ORDERINDEX nOrd, ROWINDEX nRow, bool setSamplePos);
	void SetLoopSong(bool loop);

	// Global settings to pattern effect conversion
	bool GlobalVolumeToPattern();

	bool HasMPTHacks(const bool autofix = false);

	void FixNullStrings();

// Fix: save pattern scrollbar position when switching to other tab
	CSize GetOldPatternScrollbarsPos() const { return m_szOldPatternScrollbarsPos; };
	void SetOldPatternScrollbarsPos( CSize s ){ m_szOldPatternScrollbarsPos = s; };

	void OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder);
	void OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder, const std::vector<EncoderFactoryBase*> &encFactories);

	// Returns formatted ModInstrument name.
	// [in] bEmptyInsteadOfNoName: In case of unnamed instrument string, "(no name)" is returned unless this
	//                             parameter is true is case which an empty name is returned.
	// [in] bIncludeIndex: True to include instrument index in front of the instrument name, false otherwise.
	CString GetPatternViewInstrumentName(INSTRUMENTINDEX nInstr, bool bEmptyInsteadOfNoName = false, bool bIncludeIndex = true) const;

	// Check if a given channel contains data.
	bool IsChannelUnused(CHANNELINDEX nChn) const;
	// Check whether a sample is used.
	// In sample mode, the sample numbers in all patterns are checked.
	// In instrument mode, it is only checked if a sample is referenced by an instrument (but not if the sample is actually played anywhere)
	bool IsSampleUsed(SAMPLEINDEX sample, bool searchInMutedChannels = true) const;
	// Check whether an instrument is used (only for instrument mode).
	bool IsInstrumentUsed(INSTRUMENTINDEX instr, bool searchInMutedChannels = true) const;

// protected members
protected:

	void InitializeMod();

	CChildFrame *GetChildFrame(); //rewbs.customKeys

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModDoc)
	public:
	BOOL OnNewDocument() override;
	BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
	BOOL OnSaveDocument(LPCTSTR lpszPathName) override
	{
		return OnSaveDocument(lpszPathName ? mpt::PathString::FromCString(lpszPathName) : mpt::PathString()) ? TRUE : FALSE;
	}
	void OnCloseDocument() override;
	void SafeFileClose();
	bool OnSaveDocument(const mpt::PathString &filename, const bool setPath = true);

#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif // MPT_COMPILER_CLANG
	void SetPathName(const mpt::PathString &filename, BOOL bAddToMRU = TRUE)
	{
		CDocument::SetPathName(filename.ToCString(), bAddToMRU);
	}
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif // MPT_COMPILER_CLANG
	mpt::PathString GetPathNameMpt() const
	{
		return mpt::PathString::FromCString(GetPathName());
	}

	BOOL SaveModified() override;
	bool SaveAllSamples(bool showPrompt = true);
	bool SaveSample(SAMPLEINDEX smp);

	BOOL DoSave(LPCTSTR lpszPathName, BOOL /*bSaveAs*/ = TRUE) override
	{
		return DoSave(lpszPathName ? mpt::PathString::FromCString(lpszPathName) : mpt::PathString());
	}
	BOOL DoSave(const mpt::PathString &filename, bool setPath = true);
	void DeleteContents() override;
	//}}AFX_VIRTUAL

	// Get the sample index for the current pattern cell (resolves instrument note maps, etc)
	SAMPLEINDEX GetSampleIndex(const ModCommand &m, ModCommand::INSTR lastInstr = 0) const;
	// Get group (octave) size from given instrument (or sample in sample mode)
	int GetInstrumentGroupSize(INSTRUMENTINDEX instr) const;

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

// Generated message map functions
public:
	//{{AFX_MSG(CModDoc)
	afx_msg void OnFileWaveConvert();
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
	afx_msg void OnShowCleanup();
	afx_msg void OnShowSampleTrimmer();
	afx_msg void OnSetupZxxMacros();
	afx_msg void OnEstimateSongLength();
	afx_msg void OnApproximateBPM();
	afx_msg void OnUpdateXMITMPTOnly(CCmdUI *p);
	afx_msg void OnUpdateHasEditHistory(CCmdUI *p);
	afx_msg void OnUpdateHasMIDIMappings(CCmdUI *p);
	afx_msg void OnUpdateCompatExportableOnly(CCmdUI *p);
	afx_msg void OnPatternRestart() { OnPatternRestart(true); } //rewbs.customKeys
	afx_msg void OnPatternRestart(bool loop); //rewbs.customKeys
	afx_msg void OnPatternPlay(); //rewbs.customKeys
	afx_msg void OnPatternPlayNoLoop(); //rewbs.customKeys
	afx_msg void OnViewEditHistory();
	afx_msg void OnViewMPTHacks();
	afx_msg void OnViewTempoSwingSettings();
	afx_msg void OnSaveCopy();
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
