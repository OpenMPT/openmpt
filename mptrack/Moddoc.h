// moddoc.h : interface of the CModDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODDOC_H__AE144DCC_DD0B_11D1_AF24_444553540000__INCLUDED_)
#define AFX_MODDOC_H__AE144DCC_DD0B_11D1_AF24_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sndfile.h"

// C-5
#define NOTE_MIDDLEC	(5*12+1)
#define NOTE_KEYOFF		0xFF
#define NOTE_NOTECUT	0xFE


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bit Mask for updating view (hints of what changed)
#define HINT_MODTYPE		0x00001
#define HINT_MODCOMMENTS	0x00002
#define HINT_MODGENERAL		0x00004
#define HINT_MODSEQUENCE	0x00008
#define HINT_MODCHANNELS	0x00010
#define HINT_PATTERNDATA	0x00020
#define HINT_PATTERNROW		0x00040
#define HINT_PATNAMES		0x00080
#define HINT_MPTOPTIONS		0x00100
#define HINT_MPTSETUP		0x00200
#define HINT_SAMPLEINFO		0x00400
#define HINT_SAMPLEDATA		0x00800
#define HINT_INSTRUMENT		0x01000
#define HINT_ENVELOPE		0x02000
#define HINT_SMPNAMES		0x04000
#define HINT_INSNAMES		0x08000
#define HINT_UNDO			0x10000
#define HINT_MIXPLUGINS		0x20000
#define HINT_SPEEDCHANGE	0x40000	//rewbs.envRowGrid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE : be carefull when adding new flags !!!
// -------------------------------------------------------------------------------------------------------------------------
// those flags are passed through a 32bits parameter which can also contain instrument/sample/pattern row... number :
// HINT_SAMPLEINFO & HINT_SAMPLEDATA & HINT_SMPNAMES : can be used with a sample number 12bit coded (passed as bit 20 to 31)
// HINT_PATTERNROW : is used with a row number 10bit coded (passed as bit 22 to 31)
// HINT_INSTRUMENT & HINT_INSNAMES : can be used with an instrument number 8bit coded (passed as bit 24 to 31)
// new flags can be added BUT be carefull that they will not be used in a case they should aliased with, ie, a sample number
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Undo
#define MAX_UNDO_LEVEL		100

typedef struct PATTERNUNDOBUFFER
{
	UINT pattern, patternsize;
	UINT column, row;
	UINT cx, cy;
	MODCOMMAND *pbuffer;
} PATTERNUNDOBUFFER, *PPATTERNUNDOBUFFER;


//parametered macro presets:
enum
{
	sfx_unused=0,
	sfx_cutoff,
	sfx_reso,
	sfx_mode,
	sfx_drywet,
	sfx_plug,
	sfx_cc,
	sfx_custom
};


//=============================
class CModDoc: public CDocument
//=============================
{
protected:
	LPSTR m_lpszLog;
	CSoundFile m_SndFile;

	BOOL m_bPaused;
	HWND m_hWndFollow;
	DWORD m_dwNotifyType;
	PATTERNUNDOBUFFER PatternUndo[MAX_UNDO_LEVEL];
	vector<vector<BYTE> > OrderUndo;

// -> CODE#0015
// -> DESC="channels management dlg"
	BYTE MultiRecordMask[(MAX_CHANNELS+7)/8];
	BYTE MultiSplitRecordMask[(MAX_CHANNELS+7)/8];
// -! NEW_FEATURE#0015

protected: // create from serialization only
	CModDoc();
	DECLARE_SERIAL(CModDoc)

// public members
public:
	void InitPlayer();
	CSoundFile *GetSoundFile() { return &m_SndFile; }
	void SetPause(BOOL bPause) { m_bPaused = bPause; }
	void SetModified(BOOL bModified=TRUE) { SetModifiedFlag(bModified); }
	void PostMessageToAllViews(UINT uMsg, WPARAM wParam=0, LPARAM lParam=0);
	void SendMessageToActiveViews(UINT uMsg, WPARAM wParam=0, LPARAM lParam=0);
	UINT GetModType() const { return m_SndFile.m_nType; }
	BOOL GetNumInstruments() const { return m_SndFile.m_nInstruments; }
	BOOL GetNumSamples() const { return m_SndFile.m_nSamples; }
	BOOL AddToLog(LPCSTR lpszLog);
	LPCSTR GetLog() const { return m_lpszLog; }
	BOOL ClearLog();
	UINT ShowLog(LPCSTR lpszTitle=NULL, CWnd *parent=NULL);
	void ViewPattern(UINT nPat, UINT nOrd);
	void ViewSample(UINT nSmp);
	void ViewInstrument(UINT nIns);
	HWND GetFollowWnd() const { return m_hWndFollow; }
	void SetFollowWnd(HWND hwnd, DWORD dwType);
	// Effects Description
	BOOL GetEffectName(LPSTR s, UINT command, UINT param, BOOL bXX=FALSE, int nChn=-1); // bXX: Nxx: ...
	UINT GetNumEffects() const;
	BOOL GetEffectInfo(UINT ndx, LPSTR s, BOOL bXX=FALSE, DWORD *prangeMin=NULL, DWORD *prangeMax=NULL);
	LONG GetIndexFromEffect(UINT command, UINT param);
	UINT GetEffectFromIndex(UINT ndx, int &refParam);
	BOOL GetEffectNameEx(LPSTR pszName, UINT ndx, UINT param);
	BOOL IsExtendedEffect(UINT ndx) const;
	UINT MapValueToPos(UINT ndx, UINT param);
	UINT MapPosToValue(UINT ndx, UINT pos);
	// Volume column effects description
	UINT GetNumVolCmds() const;
	LONG GetIndexFromVolCmd(UINT volcmd);
	UINT GetVolCmdFromIndex(UINT ndx);
	BOOL GetVolCmdInfo(UINT ndx, LPSTR s, DWORD *prangeMin=NULL, DWORD *prangeMax=NULL);
	int GetMacroType(CString value); //rewbs.xinfo
	int MacroToPlugParam(CString value); //rewbs.xinfo
	int MacroToMidiCC(CString value);
	int FindMacroForParam(long param);
	void SongProperties();
// operations
public:
	BOOL ChangeModType(UINT nNewType);
	BOOL ChangeNumChannels(UINT nNewChannels);
	BOOL ConvertInstrumentsToSamples();;
	BOOL RemoveUnusedSamples();
	BOOL RemoveUnusedInstruments();
	BOOL RemoveUnusedPlugs();
	BOOL RemoveUnusedPatterns(BOOL bRemove=TRUE);
	LONG InsertPattern(LONG nOrd=-1, UINT nRows=64);
	LONG InsertSample(BOOL bLimit=FALSE);
	LONG InsertInstrument(LONG lSample=0, LONG lDuplicate=0);
	void InitializeInstrument(INSTRUMENTHEADER *penv, UINT nsample=0);
	BOOL RemoveOrder(UINT n);
	BOOL RemovePattern(UINT n);
	BOOL RemoveSample(UINT n);
	BOOL RemoveInstrument(UINT n);
	UINT PlayNote(UINT note, UINT nins, UINT nsmp, BOOL bpause, LONG nVol=-1, LONG loopstart=0, LONG loopend=0, int nCurrentChn=-1); //rewbs.vstiLive: added current chan param
	BOOL NoteOff(UINT note, BOOL bFade=FALSE, UINT nins=-1, UINT nCurrentChn=-1); //rewbs.vstiLive: add params

// -> CODE#0020
// -> DESC="rearrange sample list"
	void RearrangeSampleList(void);
// -! NEW_FEATURE#0020

	BOOL IsNotePlaying(UINT note, UINT nsmp=0, UINT nins=0);
	BOOL MuteChannel(UINT nChn, BOOL bMute);
	BOOL MuteSample(UINT nSample, BOOL bMute);
	BOOL MuteInstrument(UINT nInstr, BOOL bMute);
// -> CODE#0012
// -> DESC="midi keyboard split"
	BOOL SoloChannel(UINT nChn, BOOL bSolo);
	BOOL IsChannelSolo(UINT nChn) const;
// -! NEW_FEATURE#0012
	BOOL SurroundChannel(UINT nChn, BOOL bSurround);
	BOOL SetChannelGlobalVolume(UINT nChn, UINT nVolume);
	BOOL SetChannelDefaultPan(UINT nChn, UINT nPan);
	BOOL IsChannelMuted(UINT nChn) const;
	BOOL IsSampleMuted(UINT nSample) const;
	BOOL IsInstrumentMuted(UINT nInstr) const;
// -> CODE#0015
// -> DESC="channels management dlg"
	BOOL NoFxChannel(UINT nChn, BOOL bNoFx, BOOL updateMix = TRUE);
	BOOL IsChannelNoFx(UINT nChn) const;
	BOOL IsChannelRecord1(UINT channel);
	BOOL IsChannelRecord2(UINT channel);
	BYTE IsChannelRecord(UINT channel);
	void Record1Channel(UINT channel, BOOL select = TRUE);
	void Record2Channel(UINT channel, BOOL select = TRUE);
	void ReinitRecordState(BOOL unselect = TRUE);
// -! NEW_FEATURE#0015
	UINT GetNumChannels() const { return m_SndFile.m_nChannels; }
	UINT GetPatternSize(UINT nPat) const;
	BOOL AdjustEndOfSample(UINT nSample);
	BOOL IsChildSample(UINT nIns, UINT nSmp) const;
	UINT FindSampleParent(UINT nSmp) const;
	UINT FindInstrumentChild(UINT nIns) const;
	BOOL MoveOrder(UINT nSourceNdx, UINT nDestNdx, BOOL bUpdate=TRUE, BOOL bCopy=FALSE);
	BOOL ExpandPattern(UINT nPattern);
	BOOL ShrinkPattern(UINT nPattern);
	BOOL CopyPattern(UINT nPattern, DWORD dwBeginSel, DWORD dwEndSel);
	BOOL PastePattern(UINT nPattern, DWORD dwBeginSel, BOOL mix, BOOL ITStyleMix=FALSE);	//rewbs.mixpaste

	BOOL CopyEnvelope(UINT nIns, UINT nEnv);
	BOOL PasteEnvelope(UINT nIns, UINT nEnv);
	BOOL ClearUndo();
	BOOL PrepareUndo(UINT pattern, UINT x, UINT y, UINT cx, UINT cy);
	UINT DoUndo();
	BOOL CanUndo();
	LRESULT ActivateView(UINT nIdView, DWORD dwParam);
	void UpdateAllViews(CView *pSender, LPARAM lHint=0L, CObject *pHint=NULL);
	HWND GetEditPosition(UINT &row, UINT &pat, UINT &ord); //rewbs.customKeys
	LRESULT OnCustomKeyMsg(WPARAM, LPARAM);				   //rewbs.customKeys
	void TogglePluginEditor(UINT m_nCurrentPlugin);		   //rewbs.patPlugNames
	void RecordParamChange(int slot, long param);
	void LearnMacro(int macro, long param);

	BOOL RemoveChannels(BOOL bChnMask[MAX_CHANNELS]);

	bool m_bHasValidPath; //becomes true if document is loaded or saved.

// protected members
protected:
	BOOL InitializeMod();
	void* GetChildFrame(); //rewbs.customKeys

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();

// -> CODE#0023
// -> DESC="IT project files (.itp)"
	virtual BOOL SaveModified();
// -! NEW_FEATURE#0023

	virtual BOOL DoSave(LPCSTR lpszPathName, BOOL bSaveAs=TRUE);
	virtual void DeleteContents();
	virtual void SetModifiedFlag(BOOL bModified=TRUE);
	//}}AFX_VIRTUAL

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
	afx_msg void OnEditGlobals();
	afx_msg void OnEditPatterns();
	afx_msg void OnEditSamples();
	afx_msg void OnEditInstruments();
	afx_msg void OnEditComments();
	afx_msg void OnEditGraph();	//rewbs.Graph
	afx_msg void OnInsertPattern();
	afx_msg void OnInsertSample();
	afx_msg void OnInsertInstrument();
	afx_msg void OnCleanupSamples();
	afx_msg void OnCleanupInstruments();
	afx_msg void OnCleanupPlugs();
	afx_msg void OnCleanupPatterns();
	afx_msg void OnCleanupSong();
	afx_msg void OnRearrangePatterns();
	afx_msg void OnRemoveAllInstruments();
	afx_msg void OnEstimateSongLength();
	afx_msg void OnApproximateBPM();
	afx_msg void OnUpdateXMITOnly(CCmdUI *p);
	afx_msg void OnUpdateInstrumentOnly(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMP3Encode(CCmdUI *pCmdUI);
	afx_msg void OnPatternRestart(); //rewbs.customKeys
	afx_msg void OnPatternPlay(); //rewbs.customKeys
	afx_msg void OnPatternPlayNoLoop(); //rewbs.customKeys
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:

	void ChangeFileExtension(UINT nNewType);
	UINT FindAvailableChannel();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODDOC_H__AE144DCC_DD0B_11D1_AF24_444553540000__INCLUDED_)
