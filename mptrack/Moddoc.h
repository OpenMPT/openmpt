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

// Undo
#define MAX_UNDO_LEVEL		10

typedef struct PATTERNUNDOBUFFER
{
	UINT pattern, patternsize;
	UINT column, row;
	UINT cx, cy;
	MODCOMMAND *pbuffer;
} PATTERNUNDOBUFFER, *PPATTERNUNDOBUFFER;


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
	BOOL GetEffectName(LPSTR s, UINT command, UINT param, BOOL bXX=FALSE); // bXX: Nxx: ...
	UINT GetNumEffects() const;
	BOOL GetEffectInfo(UINT ndx, LPSTR s, BOOL bXX=FALSE, DWORD *prangeMin=NULL, DWORD *prangeMax=NULL);
	LONG GetIndexFromEffect(UINT command, UINT param);
	UINT GetEffectFromIndex(UINT ndx, int *pParam=NULL);
	BOOL GetEffectNameEx(LPSTR pszName, UINT ndx, UINT param);
	BOOL IsExtendedEffect(UINT ndx) const;
	UINT MapValueToPos(UINT ndx, UINT param);
	UINT MapPosToValue(UINT ndx, UINT pos);
	// Volume column effects description
	UINT GetNumVolCmds() const;
	LONG GetIndexFromVolCmd(UINT volcmd);
	UINT GetVolCmdFromIndex(UINT ndx);
	BOOL GetVolCmdInfo(UINT ndx, LPSTR s, DWORD *prangeMin=NULL, DWORD *prangeMax=NULL);

// operations
public:
	BOOL ChangeModType(UINT nNewType);
	BOOL ChangeNumChannels(UINT nNewChannels);
	BOOL ResizePattern(UINT nPattern, UINT nRows);
	BOOL ConvertInstrumentsToSamples();;
	BOOL RemoveUnusedSamples();
	BOOL RemoveUnusedInstruments();
	BOOL RemoveUnusedPatterns(BOOL bRemove=TRUE);
	LONG InsertPattern(LONG nOrd=-1, UINT nRows=64);
	LONG InsertSample(BOOL bLimit=FALSE);
	LONG InsertInstrument(LONG lSample=0, LONG lDuplicate=0);
	void InitializeInstrument(INSTRUMENTHEADER *penv, UINT nsample=0);
	BOOL RemoveOrder(UINT n);
	BOOL RemovePattern(UINT n);
	BOOL RemoveSample(UINT n);
	BOOL RemoveInstrument(UINT n);
	BOOL PlayNote(UINT note, UINT nins, UINT nsmp, BOOL bpause, LONG nVol=-1, LONG loopstart=0, LONG loopend=0);
	BOOL NoteOff(UINT note, BOOL bFade=FALSE);
	BOOL IsNotePlaying(UINT note, UINT nsmp=0, UINT nins=0);
	BOOL MuteChannel(UINT nChn, BOOL bMute);
	BOOL MuteSample(UINT nSample, BOOL bMute);
	BOOL MuteInstrument(UINT nInstr, BOOL bMute);
	BOOL SurroundChannel(UINT nChn, BOOL bSurround);
	BOOL SetChannelGlobalVolume(UINT nChn, UINT nVolume);
	BOOL SetChannelDefaultPan(UINT nChn, UINT nPan);
	BOOL IsChannelMuted(UINT nChn) const;
	BOOL IsSampleMuted(UINT nSample) const;
	BOOL IsInstrumentMuted(UINT nInstr) const;
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
	BOOL PastePattern(UINT nPattern, DWORD dwBeginSel);
	BOOL CopyEnvelope(UINT nIns, UINT nEnv);
	BOOL PasteEnvelope(UINT nIns, UINT nEnv);
	BOOL ClearUndo();
	BOOL PrepareUndo(UINT pattern, UINT x, UINT y, UINT cx, UINT cy);
	UINT DoUndo();
	BOOL CanUndo();
	LRESULT ActivateView(UINT nIdView, DWORD dwParam);
	void UpdateAllViews(CView *pSender, LPARAM lHint=0L, CObject *pHint=NULL);

// protected members
protected:
	BOOL InitializeMod();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
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
	afx_msg void OnPlayerPlay();
	afx_msg void OnPlayerStop();
	afx_msg void OnPlayerPause();
	afx_msg void OnPlayerPlayFromStart();
	afx_msg void OnEditGlobals();
	afx_msg void OnEditPatterns();
	afx_msg void OnEditSamples();
	afx_msg void OnEditInstruments();
	afx_msg void OnEditComments();
	afx_msg void OnInsertPattern();
	afx_msg void OnInsertSample();
	afx_msg void OnInsertInstrument();
	afx_msg void OnCleanupSamples();
	afx_msg void OnCleanupInstruments();
	afx_msg void OnCleanupPatterns();
	afx_msg void OnCleanupSong();
	afx_msg void OnRearrangePatterns();
	afx_msg void OnRemoveAllInstruments();
	afx_msg void OnEstimateSongLength();
	afx_msg void OnUpdateXMITOnly(CCmdUI *p);
	afx_msg void OnUpdateInstrumentOnly(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMP3Encode(CCmdUI *pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODDOC_H__AE144DCC_DD0B_11D1_AF24_444553540000__INCLUDED_)
