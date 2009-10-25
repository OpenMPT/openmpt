// moddoc.h : interface of the CModDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MODDOC_H__AE144DCC_DD0B_11D1_AF24_444553540000__INCLUDED_)
#define AFX_MODDOC_H__AE144DCC_DD0B_11D1_AF24_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sndfile.h"
#include "misc_util.h"



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
#define HINT_SEQNAMES		0x80000
#define HINT_MAXHINTFLAG	HINT_SEQNAMES
//Bits 0-19 are reserved.
#define HINT_MASK_FLAGS		(2*HINT_MAXHINTFLAG - 1) //When applied to hint parameter, should give the flag part.
#define HINT_MASK_ITEM		(~HINT_MASK_FLAGS) //To nullify update hintbits from hint parameter.
#define HintFlagPart(x)		((x) & HINT_MASK_FLAGS)

//If fails, hint flagbits|itembits does not enable all bits; 
//might be worthwhile to check the reason.
STATIC_ASSERT( (HINT_MASK_ITEM | HINT_MASK_FLAGS) == -1 ); 

//If fails, hint param flag and item parts overlap; might be a problem.
STATIC_ASSERT( (HINT_MASK_ITEM & HINT_MASK_FLAGS) == 0 );

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


//Updateview hints can, in addition to the actual hints, contain
//addition data such as pattern or instrument index. The
//values below define the number of bits used for these.
#define HINT_BITS_PATTERN	12
#define HINT_BITS_ROWS		10
#define HINT_BITS_SAMPLE	12
#define HINT_BITS_INST		8
#define HINT_BITS_CHNTAB	8
#define HINT_BITS_SEQUENCE	6

//Defines bit shift values used for setting/retrieving the additional hint data to/from hint parameter.
#define HINT_SHIFT_PAT		(32 - HINT_BITS_PATTERN)
#define HINT_SHIFT_ROW		(32 - HINT_BITS_ROWS)
#define HINT_SHIFT_SMP		(32 - HINT_BITS_SAMPLE)
#define HINT_SHIFT_INS		(32 - HINT_BITS_INST)
#define HINT_SHIFT_CHNTAB	(32 - HINT_BITS_CHNTAB)
#define HINT_SHIFT_SEQUENCE	(32 - HINT_BITS_SEQUENCE)

//Check that hint bit counts are not too large given the number of hint flags.
STATIC_ASSERT( ((-1 << HINT_SHIFT_PAT) & HINT_MASK_ITEM) == (-1 << HINT_SHIFT_PAT) ); 
STATIC_ASSERT( ((-1 << HINT_SHIFT_ROW) & HINT_MASK_ITEM) == (-1 << HINT_SHIFT_ROW) ); 
STATIC_ASSERT( ((-1 << HINT_SHIFT_SMP) & HINT_MASK_ITEM) == (-1 << HINT_SHIFT_SMP) ); 
STATIC_ASSERT( ((-1 << HINT_SHIFT_INS) & HINT_MASK_ITEM) == (-1 << HINT_SHIFT_INS) ); 
STATIC_ASSERT( ((-1 << HINT_SHIFT_CHNTAB) & HINT_MASK_ITEM) == (-1 << HINT_SHIFT_CHNTAB) ); 
STATIC_ASSERT( ((-1 << HINT_SHIFT_SEQUENCE) & HINT_MASK_ITEM) == (-1 << HINT_SHIFT_SEQUENCE) ); 



// Undo
#define MAX_UNDO_LEVEL		100

// Pattern Undo
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

	bool bModifiedAutosave; // Modified since last autosave?

	bool m_ShowSavedialog;

// -> CODE#0015
// -> DESC="channels management dlg"
	std::bitset<MAX_BASECHANNELS> m_bsMultiRecordMask;
	std::bitset<MAX_BASECHANNELS> m_bsMultiSplitRecordMask;
// -! NEW_FEATURE#0015

protected: // create from serialization only
	CModDoc();
	DECLARE_SERIAL(CModDoc)

// public members
public:
	void InitPlayer();
	CSoundFile *GetSoundFile() { return &m_SndFile; }
	void SetPause(BOOL bPause) { m_bPaused = bPause; }
	void SetModified(BOOL bModified=TRUE) { SetModifiedFlag(bModified); bModifiedAutosave = (bModified != FALSE); }
	BOOL ModifiedSinceLastAutosave() { bool bRetval = bModifiedAutosave; bModifiedAutosave = false; return bRetval; } // return "IsModified" value and reset it until the next SetModified() (as this is only used for polling)
	void SetShowSaveDialog(bool b) {m_ShowSavedialog = b;}
	void PostMessageToAllViews(UINT uMsg, WPARAM wParam=0, LPARAM lParam=0);
	void SendMessageToActiveViews(UINT uMsg, WPARAM wParam=0, LPARAM lParam=0);
	UINT GetModType() const { return m_SndFile.m_nType; }
	INSTRUMENTINDEX GetNumInstruments() const { return m_SndFile.m_nInstruments; }
	SAMPLEINDEX GetNumSamples() const { return m_SndFile.m_nSamples; }
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
	bool GetEffectName(LPSTR pszDescription, UINT command, UINT param, bool bXX = false, CHANNELINDEX nChn = CHANNELINDEX_INVALID); // bXX: Nxx: ...
	UINT GetNumEffects() const;
	bool GetEffectInfo(UINT ndx, LPSTR s, bool bXX = false, DWORD *prangeMin=NULL, DWORD *prangeMax=NULL);
	LONG GetIndexFromEffect(UINT command, UINT param);
	UINT GetEffectFromIndex(UINT ndx, int &refParam);
	UINT GetEffectMaskFromIndex(UINT ndx);
	bool GetEffectNameEx(LPSTR pszName, UINT ndx, UINT param);
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
	static int GetZxxType(const CHAR (&szMidiZXXExt)[128 * 32]);
	static void CreateZxxFromType(CHAR (&szMidiZXXExt)[128 * 32], int iZxxType);
	void SongProperties();
// operations
public:
	BOOL ChangeModType(MODTYPE wType);
	BOOL ChangeNumChannels(UINT nNewChannels, const bool showCancelInRemoveDlg = true);

	BOOL ConvertInstrumentsToSamples();
	UINT RemovePlugs(const bool (&keepMask)[MAX_MIXPLUGINS]);

	PATTERNINDEX InsertPattern(ORDERINDEX nOrd = -1, ROWINDEX nRows = 64);
	SAMPLEINDEX InsertSample(bool bLimit = false);
	INSTRUMENTINDEX InsertInstrument(LONG lSample = 0, LONG lDuplicate = 0);
	void InitializeInstrument(MODINSTRUMENT *pIns, UINT nsample=0);
	bool RemoveOrder(SEQUENCEINDEX nSeq, ORDERINDEX nOrd);
	bool RemovePattern(PATTERNINDEX nPat);
	bool RemoveSample(SAMPLEINDEX nSmp);
	bool RemoveInstrument(INSTRUMENTINDEX nIns);
	UINT PlayNote(UINT note, UINT nins, UINT nsmp, BOOL bpause, LONG nVol=-1, LONG loopstart=0, LONG loopend=0, int nCurrentChn=-1, const uint32 nStartPos = uint32_max); //rewbs.vstiLive: added current chan param
	BOOL NoteOff(UINT note, BOOL bFade=FALSE, UINT nins=-1, UINT nCurrentChn=-1); //rewbs.vstiLive: add params

	BOOL IsNotePlaying(UINT note, UINT nsmp=0, UINT nins=0);
	bool MuteChannel(CHANNELINDEX nChn, bool bMute);
	bool MuteSample(SAMPLEINDEX nSample, bool bMute);
	bool MuteInstrument(INSTRUMENTINDEX nInstr, bool bMute);
// -> CODE#0012
// -> DESC="midi keyboard split"
	bool SoloChannel(CHANNELINDEX nChn, bool bSolo);
	bool IsChannelSolo(CHANNELINDEX nChn) const;
// -! NEW_FEATURE#0012
	bool SurroundChannel(CHANNELINDEX nChn, bool bSurround);
	bool SetChannelGlobalVolume(CHANNELINDEX nChn, UINT nVolume);
	bool SetChannelDefaultPan(CHANNELINDEX nChn, UINT nPan);
	bool IsChannelMuted(CHANNELINDEX nChn) const;
	bool IsSampleMuted(SAMPLEINDEX nSample) const;
	bool IsInstrumentMuted(INSTRUMENTINDEX nInstr) const;
// -> CODE#0015
// -> DESC="channels management dlg"
	bool NoFxChannel(CHANNELINDEX nChn, bool bNoFx, bool updateMix = true);
	bool IsChannelNoFx(CHANNELINDEX nChn) const;
	bool IsChannelRecord1(CHANNELINDEX channel) const;
	bool IsChannelRecord2(CHANNELINDEX channel) const;
	BYTE IsChannelRecord(CHANNELINDEX channel) const;
	void Record1Channel(CHANNELINDEX channel, bool select = true);
	void Record2Channel(CHANNELINDEX channel, bool select = true);
	void ReinitRecordState(bool unselect = true);
// -! NEW_FEATURE#0015
	CHANNELINDEX GetNumChannels() const { return m_SndFile.m_nChannels; }
	UINT GetPatternSize(PATTERNINDEX nPat) const;
	BOOL AdjustEndOfSample(UINT nSample);
	BOOL IsChildSample(UINT nIns, UINT nSmp) const;
	UINT FindSampleParent(UINT nSmp) const;
	UINT FindInstrumentChild(UINT nIns) const;
	bool MoveOrder(ORDERINDEX nSourceNdx, ORDERINDEX nDestNdx, bool bUpdate = true, bool bCopy = false, SEQUENCEINDEX nSourceSeq = SEQUENCEINDEX_INVALID, SEQUENCEINDEX nDestSeq = SEQUENCEINDEX_INVALID);
	BOOL ExpandPattern(PATTERNINDEX nPattern);
	BOOL ShrinkPattern(PATTERNINDEX nPattern);
	BOOL CopyPattern(PATTERNINDEX nPattern, DWORD dwBeginSel, DWORD dwEndSel);
	BOOL PastePattern(PATTERNINDEX nPattern, DWORD dwBeginSel, BOOL mix, BOOL ITStyleMix=FALSE);	//rewbs.mixpaste

	BOOL CopyEnvelope(UINT nIns, UINT nEnv);
	BOOL PasteEnvelope(UINT nIns, UINT nEnv);
	BOOL ClearPatternUndo();
	BOOL PreparePatternUndo(UINT pattern, UINT x, UINT y, UINT cx, UINT cy);
	UINT DoPatternUndo();
	BOOL CanPatternUndo();
	LRESULT ActivateView(UINT nIdView, DWORD dwParam);
	void UpdateAllViews(CView *pSender, LPARAM lHint=0L, CObject *pHint=NULL);
	HWND GetEditPosition(ROWINDEX &row, PATTERNINDEX &pat, ORDERINDEX &ord); //rewbs.customKeys
	LRESULT OnCustomKeyMsg(WPARAM, LPARAM);				   //rewbs.customKeys
	void TogglePluginEditor(UINT m_nCurrentPlugin);		   //rewbs.patPlugNames
	void RecordParamChange(int slot, long param);
	void LearnMacro(int macro, long param);
	void SetElapsedTime(ORDERINDEX nOrd, ROWINDEX nRow, bool bReset = false);

	BOOL RemoveChannels(BOOL bChnMask[MAX_CHANNELS]);

	bool MergeSequences();

	bool m_bHasValidPath; //becomes true if document is loaded or saved.
// Fix: save pattern scrollbar position when switching to other tab
	CSize GetOldPatternScrollbarsPos() const { return m_szOldPatternScrollbarsPos; };
	void SetOldPatternScrollbarsPos( CSize s ){ m_szOldPatternScrollbarsPos = s; };

	void OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder);

// protected members
protected:
	CSize m_szOldPatternScrollbarsPos;

	BOOL InitializeMod();
	void* GetChildFrame(); //rewbs.customKeys

	void CheckUnusedChannels(BOOL mask[MAX_CHANNELS], CHANNELINDEX maxRemoveCount);

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

	// for mod conversion
	bool UpdateEnvelopes(INSTRUMENTENVELOPE *mptEnv);


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
	afx_msg void OnShowCleanup();
	afx_msg void OnEstimateSongLength();
	afx_msg void OnApproximateBPM();
	afx_msg void OnUpdateXMITMPTOnly(CCmdUI *p);
	afx_msg void OnUpdateHasMIDIMappings(CCmdUI *p);
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
