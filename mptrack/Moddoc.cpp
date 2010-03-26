// moddoc.cpp : implementation of the CModDoc class
//

#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "moddoc.h"
#include "childfrm.h"
#include "mpdlgs.h"
#include "dlg_misc.h"
#include "dlsbank.h"
#include "mod2wave.h"
#include "mod2midi.h"
#include "vstplug.h"
#include "version.h"
#include "modsmp_ctrl.h"
#include "CleanupSong.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR FileFilterMOD[]	= _T("ProTracker Modules (*.mod)|*.mod||");
const TCHAR FileFilterXM[]	= _T("FastTracker Modules (*.xm)|*.xm||");
const TCHAR FileFilterS3M[] = _T("ScreamTracker Modules (*.s3m)|*.s3m||");
const TCHAR FileFilterIT[]	= _T("Impulse Tracker Modules (*.it)|*.it||");
const TCHAR FileFilterITP[] = _T("Impulse Tracker Projects (*.itp)|*.itp||");
const TCHAR FileFilterMPT[] = _T("OpenMPT Modules (*.mptm)|*.mptm||");

/////////////////////////////////////////////////////////////////////////////
// CModDoc

IMPLEMENT_SERIAL(CModDoc, CDocument, 0 /* schema number*/ )

BEGIN_MESSAGE_MAP(CModDoc, CDocument)
	//{{AFX_MSG_MAP(CModDoc)
	ON_COMMAND(ID_FILE_SAVEASWAVE,		OnFileWaveConvert)
	ON_COMMAND(ID_FILE_SAVEASMP3,		OnFileMP3Convert)
	ON_COMMAND(ID_FILE_SAVEMIDI,		OnFileMidiConvert)
	ON_COMMAND(ID_FILE_SAVECOMPAT,		OnFileCompatibilitySave)
	ON_COMMAND(ID_PLAYER_PLAY,			OnPlayerPlay)
	ON_COMMAND(ID_PLAYER_PAUSE,			OnPlayerPause)
	ON_COMMAND(ID_PLAYER_STOP,			OnPlayerStop)
	ON_COMMAND(ID_PLAYER_PLAYFROMSTART,	OnPlayerPlayFromStart)
	ON_COMMAND(ID_VIEW_GLOBALS,			OnEditGlobals)
	ON_COMMAND(ID_VIEW_PATTERNS,		OnEditPatterns)
	ON_COMMAND(ID_VIEW_SAMPLES,			OnEditSamples)
	ON_COMMAND(ID_VIEW_INSTRUMENTS,		OnEditInstruments)
	ON_COMMAND(ID_VIEW_COMMENTS,		OnEditComments)
	ON_COMMAND(ID_VIEW_GRAPH,			OnEditGraph) //rewbs.graph
	ON_COMMAND(ID_INSERT_PATTERN,		OnInsertPattern)
	ON_COMMAND(ID_INSERT_SAMPLE,		OnInsertSample)
	ON_COMMAND(ID_INSERT_INSTRUMENT,	OnInsertInstrument)
	ON_COMMAND(ID_EDIT_CLEANUP,			OnShowCleanup)

	ON_COMMAND(ID_ESTIMATESONGLENGTH,	OnEstimateSongLength)
	ON_COMMAND(ID_APPROX_BPM,			OnApproximateBPM)
	ON_COMMAND(ID_PATTERN_PLAY,			OnPatternPlay)				//rewbs.patPlayAllViews
	ON_COMMAND(ID_PATTERN_PLAYNOLOOP,	OnPatternPlayNoLoop)		//rewbs.patPlayAllViews
	ON_COMMAND(ID_PATTERN_RESTART,		OnPatternRestart)			//rewbs.patPlayAllViews
	ON_UPDATE_COMMAND_UI(ID_INSERT_INSTRUMENT,		OnUpdateXMITMPTOnly)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INSTRUMENTS,		OnUpdateXMITMPTOnly)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COMMENTS,			OnUpdateXMITMPTOnly)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MIDIMAPPING,		OnUpdateHasMIDIMappings)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEASMP3,			OnUpdateMP3Encode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////
// CModDoc diagnostics

#ifdef _DEBUG
void CModDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CModDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CModDoc construction/destruction

CModDoc::CModDoc()
//----------------
{
	m_bHasValidPath=false;
	m_bPaused = TRUE;
	m_lpszLog = NULL;
	m_hWndFollow = NULL;

	m_PatternUndo.SetParent(this);
	m_SampleUndo.SetParent(this);

	m_SplitKeyboardSettings.splitInstrument = 0;
	m_SplitKeyboardSettings.splitNote = NOTE_MIDDLEC - 1;
	m_SplitKeyboardSettings.splitVolume = 0;
	m_SplitKeyboardSettings.octaveModifier = 0;
	m_SplitKeyboardSettings.octaveLink = false;

#ifdef _DEBUG
	MODCHANNEL *p = m_SndFile.Chn;
	if (((DWORD)p) & 7) Log("MODCHANNEL is not aligned (0x%08X)\n", p);
#endif
// Fix: save pattern scrollbar position when switching to other tab
	m_szOldPatternScrollbarsPos = CSize(-10,-10);
// -> CODE#0015
// -> DESC="channels management dlg"
	ReinitRecordState();
// -! NEW_FEATURE#0015
	m_ShowSavedialog = false;
}


CModDoc::~CModDoc()
//-----------------
{
	ClearLog();
}


void CModDoc::SetModifiedFlag(BOOL bModified)
//-------------------------------------------
{
	BOOL bChanged = (bModified != IsModified());
	CDocument::SetModifiedFlag(bModified);
	if (bChanged) UpdateFrameCounts();
}


BOOL CModDoc::OnNewDocument()
//---------------------------
{
	if (!CDocument::OnNewDocument()) return FALSE;

	m_SndFile.Create(NULL, this, 0);
	m_SndFile.ChangeModTypeTo(CTrackApp::GetDefaultDocType());

	if(CTrackApp::IsProject()) {
		m_SndFile.m_dwSongFlags |= SONG_ITPROJECT;
	}

	if (m_SndFile.m_nType & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)) {
		m_SndFile.m_dwSongFlags |= SONG_LINEARSLIDES;
		m_SndFile.m_dwSongFlags |= SONG_EMBEDMIDICFG;
	}


	// Refresh mix levels now that the correct mod type has been set
	m_SndFile.m_nMixLevels = m_SndFile.GetModSpecifications().defaultMixLevels;
	m_SndFile.m_pConfig->SetMixLevels(m_SndFile.m_nMixLevels);
	// ...and the order length
	m_SndFile.Order.resize(min(ModSequenceSet::s_nCacheSize, m_SndFile.GetModSpecifications().ordersMax));

	theApp.GetDefaultMidiMacro(&m_SndFile.m_MidiCfg);
	ReinitRecordState();
	InitializeMod();
	SetModifiedFlag(FALSE);
	return TRUE;
}


BOOL CModDoc::OnOpenDocument(LPCTSTR lpszPathName)
//------------------------------------------------
{
	BOOL bModified = TRUE;
	CMappedFile f;

	if (!lpszPathName) return OnNewDocument();
	BeginWaitCursor();
	if (f.Open(lpszPathName))
	{
		DWORD dwLen = f.GetLength();
		if (dwLen)
		{
			LPBYTE lpStream = f.Lock();
			if (lpStream)
			{
				m_SndFile.Create(lpStream, this, dwLen);
				f.Unlock();
			}
		}
		f.Close();
	}

	EndWaitCursor();
	if ((m_SndFile.m_nType == MOD_TYPE_NONE) || (!m_SndFile.m_nChannels)) return FALSE;
	// Midi Import
	if (m_SndFile.m_nType == MOD_TYPE_MID)
	{
		CDLSBank *pCachedBank = NULL, *pEmbeddedBank = NULL;
		CHAR szCachedBankFile[_MAX_PATH] = "";

		if (CDLSBank::IsDLSBank(lpszPathName))
		{
			pEmbeddedBank = new CDLSBank();
			pEmbeddedBank->Open(lpszPathName);
		}
		m_SndFile.ChangeModTypeTo(MOD_TYPE_IT);
		BeginWaitCursor();
		LPMIDILIBSTRUCT lpMidiLib = CTrackApp::GetMidiLibrary();
		// Scan Instruments
		if (lpMidiLib) for (INSTRUMENTINDEX nIns = 1; nIns <= m_SndFile.m_nInstruments; nIns++) if (m_SndFile.Instruments[nIns])
		{
			LPCSTR pszMidiMapName;
			MODINSTRUMENT *pIns = m_SndFile.Instruments[nIns];
			UINT nMidiCode;
			BOOL bEmbedded = FALSE;

			if (pIns->nMidiChannel == 10)
				nMidiCode = 0x80 | (pIns->nMidiDrumKey & 0x7F);
			else
				nMidiCode = pIns->nMidiProgram & 0x7F;
			pszMidiMapName = lpMidiLib->MidiMap[nMidiCode];
			if (pEmbeddedBank)
			{
				UINT nDlsIns = 0, nDrumRgn = 0;
				UINT nProgram = pIns->nMidiProgram;
				UINT dwKey = (nMidiCode < 128) ? 0xFF : (nMidiCode & 0x7F);
				if ((pEmbeddedBank->FindInstrument(	(nMidiCode >= 128),
													(pIns->wMidiBank & 0x3FFF),
													nProgram, dwKey, &nDlsIns))
				 || (pEmbeddedBank->FindInstrument(	(nMidiCode >= 128),	0xFFFF,
													(nMidiCode >= 128) ? 0xFF : nProgram,
													dwKey, &nDlsIns)))
				{
					if (dwKey < 0x80) nDrumRgn = pEmbeddedBank->GetRegionFromKey(nDlsIns, dwKey);
					if (pEmbeddedBank->ExtractInstrument(&m_SndFile, nIns, nDlsIns, nDrumRgn))
					{
						if ((dwKey >= 24) && (dwKey < 100))
						{
							lstrcpyn(pIns->name, szMidiPercussionNames[dwKey-24], sizeof(pIns->name));
						}
						bEmbedded = TRUE;
					}
				}
			}
			if ((pszMidiMapName) && (pszMidiMapName[0]) && (!bEmbedded))
			{
				// Load From DLS Bank
				if (CDLSBank::IsDLSBank(pszMidiMapName))
				{
					CDLSBank *pDLSBank = NULL;
					
					if ((pCachedBank) && (!lstrcmpi(szCachedBankFile, pszMidiMapName)))
					{
						pDLSBank = pCachedBank;
					} else
					{
						if (pCachedBank) delete pCachedBank;
						pCachedBank = new CDLSBank;
						strcpy(szCachedBankFile, pszMidiMapName);
						if (pCachedBank->Open(pszMidiMapName)) pDLSBank = pCachedBank;
					}
					if (pDLSBank)
					{
						UINT nDlsIns = 0, nDrumRgn = 0;
						UINT nProgram = pIns->nMidiProgram;
						UINT dwKey = (nMidiCode < 128) ? 0xFF : (nMidiCode & 0x7F);
						if ((pDLSBank->FindInstrument(	(nMidiCode >= 128),
														(pIns->wMidiBank & 0x3FFF),
														nProgram, dwKey, &nDlsIns))
						 || (pDLSBank->FindInstrument(	(nMidiCode >= 128), 0xFFFF,
														(nMidiCode >= 128) ? 0xFF : nProgram,
														dwKey, &nDlsIns)))
						{
							if (dwKey < 0x80) nDrumRgn = pDLSBank->GetRegionFromKey(nDlsIns, dwKey);
							pDLSBank->ExtractInstrument(&m_SndFile, nIns, nDlsIns, nDrumRgn);
							if ((dwKey >= 24) && (dwKey < 24+61))
							{
								lstrcpyn(pIns->name, szMidiPercussionNames[dwKey-24], sizeof(pIns->name));
							}
						}
					}
				} else
				{
					// Load from Instrument or Sample file
					CHAR szName[_MAX_FNAME], szExt[_MAX_EXT];
					CFile f;

					if (f.Open(pszMidiMapName, CFile::modeRead))
					{
						DWORD len = static_cast<DWORD>(f.GetLength());
						LPBYTE lpFile;
						if ((len) && ((lpFile = (LPBYTE)GlobalAllocPtr(GHND, len)) != NULL))
						{
							f.Read(lpFile, len);
							m_SndFile.ReadInstrumentFromFile(nIns, lpFile, len);
							_splitpath(pszMidiMapName, NULL, NULL, szName, szExt);
							strncat(szName, szExt, sizeof(szName));
							pIns = m_SndFile.Instruments[nIns];
							if (!pIns->filename[0]) lstrcpyn(pIns->filename, szName, sizeof(pIns->filename));
							if (!pIns->name[0])
							{
								if (nMidiCode < 128)
								{
									lstrcpyn(pIns->name, szMidiProgramNames[nMidiCode], sizeof(pIns->name));
								} else
								{
									UINT nKey = nMidiCode & 0x7F;
									if (nKey >= 24)
										lstrcpyn(pIns->name, szMidiPercussionNames[nKey-24], sizeof(pIns->name));
								}
							}
						}
						f.Close();
					}
				}
			}
		}
		if (pCachedBank) delete pCachedBank;
		if (pEmbeddedBank) delete pEmbeddedBank;
		EndWaitCursor();
	}
	// Convert to MOD/S3M/XM/IT
	switch(m_SndFile.m_nType)
	{
	case MOD_TYPE_MOD:
	case MOD_TYPE_S3M:
	case MOD_TYPE_XM:
	case MOD_TYPE_IT:
	case MOD_TYPE_MPT:
		bModified = FALSE;
		break;
	case MOD_TYPE_AMF0:
	case MOD_TYPE_MTM:
	case MOD_TYPE_669:
		m_SndFile.ChangeModTypeTo(MOD_TYPE_MOD);
		break;
	case MOD_TYPE_MED:
	case MOD_TYPE_OKT:
	case MOD_TYPE_AMS:
		m_SndFile.ChangeModTypeTo(MOD_TYPE_XM);
		if ((m_SndFile.m_nDefaultTempo == 125) && (m_SndFile.m_nDefaultSpeed == 6) && (!m_SndFile.m_nInstruments))
		{
			m_SndFile.m_nType = MOD_TYPE_MOD;
			for (UINT i=0; i<m_SndFile.Patterns.Size(); i++)
				if ((m_SndFile.Patterns[i]) && (m_SndFile.PatternSize[i] != 64))
					m_SndFile.m_nType = MOD_TYPE_XM;
		}
		break;
	case MOD_TYPE_MT2:
		m_SndFile.ChangeModTypeTo(MOD_TYPE_IT);
		break;
	case MOD_TYPE_FAR:
	case MOD_TYPE_PTM:
	case MOD_TYPE_STM:
	case MOD_TYPE_DSM:
	case MOD_TYPE_AMF:
		m_SndFile.ChangeModTypeTo(MOD_TYPE_S3M);
		break;
	case MOD_TYPE_PSM:
	case MOD_TYPE_ULT:
	default:
		m_SndFile.ChangeModTypeTo(MOD_TYPE_IT);
	}

// -> CODE#0015
// -> DESC="channels management dlg"
	ReinitRecordState();
// -! NEW_FEATURE#0015
	if (m_SndFile.m_dwLastSavedWithVersion > MptVersion::num) {
		char s[256];
		wsprintf(s, "Warning: this song was last saved with a more recent version of OpenMPT.\r\nSong saved with: v%s. Current version: v%s.\r\n", 
			(LPCTSTR)MptVersion::ToStr(m_SndFile.m_dwLastSavedWithVersion),
			MptVersion::str);
		::AfxMessageBox(s);
	}

	SetModifiedFlag(FALSE); // (bModified);
	m_bHasValidPath=true;

	return TRUE;
}


BOOL CModDoc::OnSaveDocument(LPCTSTR lpszPathName)
//------------------------------------------------
{
	static int greccount = 0;
	TCHAR fext[_MAX_EXT]="";
	UINT nType = m_SndFile.m_nType, dwPacking = 0;
	BOOL bOk = FALSE;
	m_SndFile.m_dwLastSavedWithVersion = MptVersion::num;
	if (!lpszPathName) return FALSE;
	_tsplitpath(lpszPathName, NULL, NULL, NULL, fext);
	if (!lstrcmpi(fext, ".mod")) nType = MOD_TYPE_MOD; else
	if (!lstrcmpi(fext, ".s3m")) nType = MOD_TYPE_S3M; else
	if (!lstrcmpi(fext, ".xm")) nType = MOD_TYPE_XM; else
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//	if (!lstrcmpi(fext, ".it")) nType = MOD_TYPE_IT; else
	if (!lstrcmpi(fext, ".it") || !lstrcmpi(fext, ".itp")) nType = MOD_TYPE_IT; else
	if (!lstrcmpi(fext, ".mptm")) nType = MOD_TYPE_MPT; else
// -! NEW_FEATURE#0023
	if (!greccount)
	{
		greccount++;
		bOk = DoSave(NULL, TRUE);
		greccount--;
        return bOk;
	}
	BeginWaitCursor();
	switch(nType)
	{
	case MOD_TYPE_MOD:	bOk = m_SndFile.SaveMod(lpszPathName, dwPacking); break;
	case MOD_TYPE_S3M:	bOk = m_SndFile.SaveS3M(lpszPathName, dwPacking); break;
	case MOD_TYPE_XM:	bOk = m_SndFile.SaveXM(lpszPathName, dwPacking); break;
	case MOD_TYPE_IT:	bOk = (m_SndFile.m_dwSongFlags & SONG_ITPROJECT || !lstrcmpi(fext, ".itp")) ? m_SndFile.SaveITProject(lpszPathName) : m_SndFile.SaveIT(lpszPathName, dwPacking); break;
	case MOD_TYPE_MPT:	bOk = m_SndFile.SaveIT(lpszPathName, dwPacking); break;
	}
	EndWaitCursor();
	if (bOk)
	{
		if (nType == m_SndFile.m_nType) SetPathName(lpszPathName);
	} else
	{
		if(nType == MOD_TYPE_IT && m_SndFile.m_dwSongFlags & SONG_ITPROJECT) ::MessageBox(NULL,"ITP projects need to have a path set for each instrument...",NULL,MB_ICONERROR | MB_OK);
		else ErrorBox(IDS_ERR_SAVESONG, CMainFrame::GetMainFrame());
	}
	return bOk;
}


// -> CODE#0023
// -> DESC="IT project files (.itp)"
BOOL CModDoc::SaveModified()
//--------------------------
{
	if((m_SndFile.m_nType & MOD_TYPE_IT) && m_SndFile.m_dwSongFlags & SONG_ITPROJECT && !(m_SndFile.m_dwSongFlags & SONG_ITPEMBEDIH)){

		BOOL unsavedInstrument = FALSE;

		for(UINT i = 0 ; i < m_SndFile.m_nInstruments ; i++){
			if(m_SndFile.instrumentModified[i]) { unsavedInstrument = TRUE; break; }
		}

		if(unsavedInstrument && ::MessageBox(NULL,"Do you want to save modified instruments ?",NULL,MB_ICONQUESTION | MB_YESNO | MB_APPLMODAL) == IDYES){

			for(INSTRUMENTINDEX i = 0 ; i < m_SndFile.m_nInstruments ; i++){
				if(m_SndFile.m_szInstrumentPath[i][0] != '\0'){
					int size = strlen(m_SndFile.m_szInstrumentPath[i]);
					BOOL iti = _stricmp(&m_SndFile.m_szInstrumentPath[i][size-3],"iti") == 0;
					BOOL xi  = _stricmp(&m_SndFile.m_szInstrumentPath[i][size-2],"xi") == 0;

					if(iti || (!iti && !xi  && m_SndFile.m_nType & (MOD_TYPE_IT|MOD_TYPE_MPT)))
						m_SndFile.SaveITIInstrument(i+1, m_SndFile.m_szInstrumentPath[i]);
					if(xi  || (!xi  && !iti && m_SndFile.m_nType == MOD_TYPE_XM))
						m_SndFile.SaveXIInstrument(i+1, m_SndFile.m_szInstrumentPath[i]);
				}
			}
		}
	}

	return CDocument::SaveModified();
}
// -! NEW_FEATURE#0023


void CModDoc::OnCloseDocument()
//-----------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->OnDocumentClosed(this);
	CDocument::OnCloseDocument();
}


void CModDoc::DeleteContents()
//----------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->StopMod(this);
	m_SndFile.Destroy();
// -> CODE#0015
// -> DESC="channels management dlg"
	ReinitRecordState();
// -! NEW_FEATURE#0015
}


BOOL CModDoc::DoSave(LPCSTR lpszPathName, BOOL)
//---------------------------------------------
{
	CHAR s[_MAX_PATH] = "";
	CHAR path[_MAX_PATH]="", drive[_MAX_DRIVE]="";
	CHAR fname[_MAX_FNAME]="", fext[_MAX_EXT]="";
	std::string extFilter = "", defaultExtension = "";
	
	switch(m_SndFile.GetType())
	{
	case MOD_TYPE_MOD:
		MsgBoxHidable(ModCompatibilityExportTip);
		defaultExtension = "mod";
		extFilter = FileFilterMOD;
		strcpy(fext, ".mod");
		break;
	case MOD_TYPE_S3M:
		defaultExtension = "s3m";
		extFilter = FileFilterS3M;
		strcpy(fext, ".s3m");
		break;
	case MOD_TYPE_XM:
		MsgBoxHidable(XMCompatibilityExportTip);
		defaultExtension = "xm";
		extFilter = FileFilterXM;
		strcpy(fext, ".xm");
		break;
	case MOD_TYPE_IT:
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//		lpszDefExt = "it";
//		lpszFilter = "Impulse Tracker Modules (*.it)|*.it||";
//		strcpy(fext, ".it");
		if(m_SndFile.m_dwSongFlags & SONG_ITPROJECT){
			defaultExtension = "itp";
			extFilter = FileFilterITP;
			strcpy(fext, ".itp");
		}
		else
		{
			MsgBoxHidable(ItCompatibilityExportTip);
			defaultExtension = "it";
			extFilter = FileFilterIT;
			strcpy(fext, ".it");
		}
// -! NEW_FEATURE#0023
		break;
	case MOD_TYPE_MPT:
			defaultExtension = "mptm";
			extFilter = FileFilterMPT;
			strcpy(fext, ".mptm");
		break;
	default:	
		ErrorBox(IDS_ERR_SAVESONG, CMainFrame::GetMainFrame());
		return FALSE;
	}
	if ((!lpszPathName) || (!lpszPathName[0]) || m_ShowSavedialog)
	{
		_splitpath(m_strPathName, drive, path, fname, NULL);
		if (!fname[0]) strcpy(fname, m_strTitle);
		strcpy(s, drive);
		strcat(s, path);
		strcat(s, fname);
		strcat(s, fext);
		
		FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, defaultExtension, s,
			extFilter,
			CMainFrame::GetWorkingDirectory(DIR_MODS));
		if(files.abort) return FALSE;

		CMainFrame::SetWorkingDirectory(files.workingDirectory.c_str(), DIR_MODS, true);

		strcpy(s, files.first_file.c_str());
		_splitpath(s, drive, path, fname, fext);
	} else
	{
		_splitpath(lpszPathName, drive, path, fname, NULL);
		strcpy(s, drive);
		strcat(s, path);
		strcat(s, fname);
		strcat(s, fext);
	}
	// Do we need to create a backup file ?
	if ((CMainFrame::m_dwPatternSetup & PATTERN_CREATEBACKUP)
	 && (IsModified()) && (!lstrcmpi(s, m_strPathName)))
	{
		CFileStatus rStatus;
		if (CFile::GetStatus(s, rStatus))
		{
			CHAR bkname[_MAX_PATH] = "";
			_splitpath(s, bkname, path, fname, NULL);
			strcat(bkname, path);
			strcat(bkname, fname);
			strcat(bkname, ".bak");
			if (CFile::GetStatus(bkname, rStatus))
			{
				DeleteFile(bkname);
			}
			MoveFile(s, bkname);
		}
	}
	if (OnSaveDocument(s))
	{
		SetModified(FALSE);
		m_bHasValidPath=true;
		m_ShowSavedialog = false;
		UpdateAllViews(NULL, HINT_MODGENERAL); // Update treeview (e.g. filename might have changed).
		return TRUE;
	} else
	{
// -> CODE#0023
// -> DESC="IT project files (.itp)"
//		ErrorBox(IDS_ERR_SAVESONG, CMainFrame::GetMainFrame());	// done in OnSaveDocument()
// -! NEW_FEATURE#0023
		return FALSE;
	}
}


void CModDoc::InitPlayer()
//------------------------
{
	m_SndFile.ResetChannels();
}


BOOL CModDoc::InitializeMod()
//---------------------------
{
	// New module ?
	if (!m_SndFile.m_nChannels)
	{
		switch(GetModType())
		{
		case MOD_TYPE_MOD:
			m_SndFile.m_nChannels = 4;
			break;
		case MOD_TYPE_S3M:
			m_SndFile.m_nChannels = 16;
			break;
		default:
			m_SndFile.m_nChannels = 32;
			break;
		}

		if (m_SndFile.Order[0] >= m_SndFile.Patterns.Size())
			m_SndFile.Order[0] = 0;

		if (!m_SndFile.Patterns[0])
		{
			m_SndFile.Patterns.Insert(0, 64);
		}

		memset(m_SndFile.m_szNames, 0, sizeof(m_SndFile.m_szNames));
		strcpy(m_SndFile.m_szNames[0], "untitled");

		m_SndFile.m_nMusicTempo = m_SndFile.m_nDefaultTempo = 125;
		m_SndFile.m_nMusicSpeed = m_SndFile.m_nDefaultSpeed = 6;

		if(m_SndFile.m_nMixLevels == mixLevels_original)
		{
			m_SndFile.m_nGlobalVolume = m_SndFile.m_nDefaultGlobalVolume = 256;
			m_SndFile.m_nSamplePreAmp = m_SndFile.m_nVSTiVolume = 48;
		}
		else
		{
			m_SndFile.m_nGlobalVolume = m_SndFile.m_nDefaultGlobalVolume = 128;
			m_SndFile.m_nSamplePreAmp = m_SndFile.m_nVSTiVolume = 128;
		}

		for (CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
		{
			m_SndFile.ChnSettings[nChn].dwFlags = 0;
			m_SndFile.ChnSettings[nChn].nVolume = 64;
			m_SndFile.ChnSettings[nChn].nPan = 128;
			m_SndFile.Chn[nChn].nGlobalVol = 64;
		}
		// Setup LRRL panning scheme for MODs
		m_SndFile.SetupMODPanning();
	}
	if (!m_SndFile.m_nSamples)
	{
		strcpy(m_SndFile.m_szNames[1], "untitled");
		m_SndFile.m_nSamples = (GetModType() == MOD_TYPE_MOD) ? 31 : 1;

		ctrlSmp::ResetSamples(m_SndFile, ctrlSmp::SmpResetInit);

		if ((!m_SndFile.m_nInstruments) && (m_SndFile.m_nType & MOD_TYPE_XM))
		{
			m_SndFile.m_nInstruments = 1;
			m_SndFile.Instruments[1] = new MODINSTRUMENT;
			InitializeInstrument(m_SndFile.Instruments[1], 1);
		}
		if (m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT|MOD_TYPE_XM))
		{
			m_SndFile.m_dwSongFlags |= SONG_LINEARSLIDES;
		}
	}
	m_SndFile.SetCurrentPos(0);

	return TRUE;
}


void CModDoc::PostMessageToAllViews(UINT uMsg, WPARAM wParam, LPARAM lParam)
//--------------------------------------------------------------------------
{
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView* pView = GetNextView(pos);
		if (pView) pView->PostMessage(uMsg, wParam, lParam);
	} 
}


void CModDoc::SendMessageToActiveViews(UINT uMsg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		CMDIChildWnd *pMDI = pMainFrm->MDIGetActive();
		if (pMDI) pMDI->SendMessageToDescendants(uMsg, wParam, lParam);
	}
}


void CModDoc::ViewPattern(UINT nPat, UINT nOrd)
//---------------------------------------------
{
	SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_PATTERNS, ((nPat+1) << 16) | nOrd);
}


void CModDoc::ViewSample(UINT nSmp)
//---------------------------------
{
	SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_SAMPLES, nSmp);
}


void CModDoc::ViewInstrument(UINT nIns)
//-------------------------------------
{
	SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_INSTRUMENTS, nIns);
}


BOOL CModDoc::AddToLog(LPCSTR lpszLog)
//------------------------------------
{
	UINT len;
	if ((!lpszLog) || (!lpszLog[0])) return FALSE;
	len = strlen(lpszLog);
	if (m_lpszLog)
	{
		LPSTR p = new CHAR[strlen(m_lpszLog)+len+2];
		if (p)
		{
			strcpy(p, m_lpszLog);
			strcat(p, lpszLog);
			delete[] m_lpszLog;
			m_lpszLog = p;
		}
	} else
	{
		m_lpszLog = new CHAR[len+2];
		if (m_lpszLog) strcpy(m_lpszLog, lpszLog);
	}
	return TRUE;
}


BOOL CModDoc::ClearLog()
//----------------------
{
	if (m_lpszLog)
	{
		delete[] m_lpszLog;
		m_lpszLog = NULL;
	}
	return TRUE;
}


UINT CModDoc::ShowLog(LPCSTR lpszTitle, CWnd *parent)
//---------------------------------------------------
{
	if (!lpszTitle) lpszTitle = MAINFRAME_TITLE;
	if (m_lpszLog)
	{
#if 0
		CShowLogDlg dlg(parent);
		return dlg.ShowLog(m_lpszLog, lpszTitle);
#else
		return ::MessageBox((parent) ? parent->m_hWnd : NULL, m_lpszLog, lpszTitle, MB_OK|MB_ICONINFORMATION);
#endif
	}
	return IDCANCEL;
}

UINT CModDoc::PlayNote(UINT note, UINT nins, UINT nsmp, BOOL bpause, LONG nVol, LONG loopstart, LONG loopend, int nCurrentChn, const uint32 nStartPos) //rewbs.vstiLive: added current chan param
//-----------------------------------------------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	UINT nChn = m_SndFile.m_nChannels;
	
	if ((!pMainFrm) || (!note)) return FALSE;
	if (nVol > 256) nVol = 256;
	if (note <= NOTE_MAX)
	{
		BEGIN_CRITICAL();
		
		//kill notes if required.
		if ( (bpause) || (m_SndFile.IsPaused()) || pMainFrm->GetModPlaying() != this) { 
			//OnPlayerPause();				  // pause song - pausing VSTis is too slow
			pMainFrm->SetLastMixActiveTime(); // mark activity

			// All notes off
			for (UINT i=0; i<MAX_CHANNELS; i++)	{
				if ((i < m_SndFile.m_nChannels) || (m_SndFile.Chn[i].nMasterChn)) {
					m_SndFile.Chn[i].dwFlags |= CHN_KEYOFF | CHN_NOTEFADE;
					m_SndFile.Chn[i].nFadeOutVol = 0;
				}
			}
		}

		//find a channel if required
		//if (nCurrentChn<0) { 
			nChn = FindAvailableChannel();
		//}

		MODCHANNEL *pChn = &m_SndFile.Chn[nChn];
		
		//stop channel, just in case.
		if (pChn->nLength)	{
			pChn->nPos = pChn->nPosLo = pChn->nLength = 0;
		}

		//reset channel properties; in theory the chan is completely unused anyway.
		pChn->dwFlags &= 0xFF;
		pChn->dwFlags &= ~(CHN_MUTE);
		pChn->nGlobalVol = 64;
		pChn->nInsVol = 64;
		pChn->nPan = 128;
		pChn->nRightVol = pChn->nLeftVol = 0;
		pChn->nROfs = pChn->nLOfs = 0;
		pChn->nCutOff = 0x7F;
		pChn->nResonance = 0;
		pChn->nVolume = 256;
		pChn->nMasterChn = 0;	//remove NNA association
		pChn->nNewNote = static_cast<BYTE>(note);

		if (nins) {									//Set instrument
			m_SndFile.resetEnvelopes(pChn);
			m_SndFile.InstrumentChange(pChn, nins);
		} 
		else if ((nsmp) && (nsmp < MAX_SAMPLES)) {	//Or set sample
			MODSAMPLE *pSmp = &m_SndFile.Samples[nsmp];
			pChn->pCurrentSample = pSmp->pSample;
			pChn->pModInstrument = nullptr;
			pChn->pModSample = pSmp;
			pChn->pSample = pSmp->pSample;
			pChn->nFineTune = pSmp->nFineTune;
			pChn->nC5Speed = pSmp->nC5Speed;
			pChn->nPos = pChn->nPosLo = pChn->nLength = 0;
			pChn->nLoopStart = pSmp->nLoopStart;
			pChn->nLoopEnd = pSmp->nLoopEnd;
			pChn->dwFlags = pSmp->uFlags & (0xFF & ~CHN_MUTE);
			pChn->nPan = 128;
			if (pSmp->uFlags & CHN_PANNING) pChn->nPan = pSmp->nPan;
			pChn->nInsVol = pSmp->nGlobalVol;
			pChn->nFadeOutVol = 0x10000;
		}

		m_SndFile.NoteChange(nChn, note, false, true, true);
		if (nVol >= 0) pChn->nVolume = nVol;
		
		// handle sample looping.
		// Fix: Bug report 1700.
		//if ((loopstart + 16 < loopend) && (loopstart >= 0) && (loopend <= (LONG)pChn->nLength)) 	{
		if ((loopstart + 16 < loopend) && (loopstart >= 0) && (pChn->pModSample != nullptr))
		{
			pChn->nPos = loopstart;
			pChn->nPosLo = 0;
			pChn->nLoopStart = loopstart;
			pChn->nLoopEnd = loopend;
			pChn->nLength = min(UINT(loopend), pChn->pModSample->nLength);
		}

		// handle extra-loud flag
		if ((!(CMainFrame::m_dwPatternSetup & PATTERN_NOEXTRALOUD)) && (nsmp)) {
			pChn->dwFlags |= CHN_EXTRALOUD;
		} else {
			pChn->dwFlags &= ~CHN_EXTRALOUD;
		}

		// Handle custom start position
		if(nStartPos != uint32_max && pChn->pModSample)
		{
			pChn->nPos = nStartPos;
			// If start position is after loop end, set loop end to sample end so that the sample starts 
			// playing.
			if(pChn->nLoopEnd < nStartPos) 
				pChn->nLength = pChn->nLoopEnd = pChn->pModSample->nLength;
		}

		/*
		if (bpause) {   
			if ((loopstart + 16 < loopend) && (loopstart >= 0) && (loopend <= (LONG)pChn->nLength)) 	{
				pChn->nPos = loopstart;
				pChn->nPosLo = 0;
				pChn->nLoopStart = loopstart;
				pChn->nLoopEnd = loopend;
				pChn->nLength = loopend;
			}
			m_SndFile.m_nBufferCount = 0;
			m_SndFile.m_dwSongFlags |= SONG_PAUSED;
			if ((!(CMainFrame::m_dwPatternSetup & PATTERN_NOEXTRALOUD)) && (nsmp)) pChn->dwFlags |= CHN_EXTRALOUD;
		} else pChn->dwFlags &= ~CHN_EXTRALOUD;
		*/

		//rewbs.vstiLive
		if (nins <= m_SndFile.m_nInstruments)
		{
			MODINSTRUMENT *pIns = m_SndFile.Instruments[nins];
			if (pIns && pIns->nMidiChannel > 0 && pIns->nMidiChannel < 17) // instro sends to a midi chan
			{
				// UINT nPlugin = m_SndFile.GetBestPlugin(nChn, PRIORITISE_INSTRUMENT, EVEN_IF_MUTED);
				 
				UINT nPlugin = 0;
				if (pChn->pModInstrument) 
					nPlugin = pChn->pModInstrument->nMixPlug;  					// first try intrument VST
				if ((!nPlugin) || (nPlugin > MAX_MIXPLUGINS) && (nCurrentChn >=0))
					nPlugin = m_SndFile.ChnSettings[nCurrentChn].nMixPlugin; // Then try Channel VST
				
   				if ((nPlugin) && (nPlugin <= MAX_MIXPLUGINS))
				{
					IMixPlugin *pPlugin =  m_SndFile.m_MixPlugins[nPlugin-1].pMixPlugin;
					if (pPlugin) pPlugin->MidiCommand(pIns->nMidiChannel, pIns->nMidiProgram, pIns->wMidiBank, pIns->NoteMap[note - 1], pChn->nVolume, MAX_BASECHANNELS);
					//if (pPlugin) pPlugin->MidiCommand(pIns->nMidiChannel, pIns->nMidiProgram, pIns->wMidiBank, note, pChn->GetVSTVolume(), MAX_BASECHANNELS);
				}
			}
		}
		//end rewbs.vstiLive

		END_CRITICAL();

		if (pMainFrm->GetModPlaying() != this)
		{
			m_SndFile.m_dwSongFlags |= SONG_PAUSED;
			pMainFrm->PlayMod(this, m_hWndFollow, m_dwNotifyType);
		}
		CMainFrame::EnableLowLatencyMode();
	} else
	{
		BEGIN_CRITICAL();
		m_SndFile.NoteChange(nChn, note);
		if (bpause) m_SndFile.m_dwSongFlags |= SONG_PAUSED;
		END_CRITICAL();
	}
	return nChn;
}


BOOL CModDoc::NoteOff(UINT note, BOOL bFade, UINT nins, UINT nCurrentChn) //rewbs.vstiLive: added chan and nins
//-----------------------------------------------------------------------
{
	BEGIN_CRITICAL();

	//rewbs.vstiLive
	if((nins > 0) && (nins <= m_SndFile.m_nInstruments) && (note >= NOTE_MIN) && (note <= NOTE_MAX))
	{

		MODINSTRUMENT *pIns = m_SndFile.Instruments[nins];
		if (pIns && pIns->nMidiChannel > 0 && pIns->nMidiChannel < 17) // instro sends to a midi chan
		{

			UINT nPlugin = pIns->nMixPlug;  		// First try intrument VST
			if (((!nPlugin) || (nPlugin > MAX_MIXPLUGINS)) && //no good plug yet
				(nCurrentChn<MAX_CHANNELS)) // chan OK
				nPlugin = m_SndFile.ChnSettings[nCurrentChn].nMixPlugin;// Then try Channel VST
			
			if ((nPlugin) && (nPlugin <= MAX_MIXPLUGINS))
			{
				IMixPlugin *pPlugin =  m_SndFile.m_MixPlugins[nPlugin-1].pMixPlugin;
				if (pPlugin) pPlugin->MidiCommand(pIns->nMidiChannel, pIns->nMidiProgram, pIns->wMidiBank, pIns->NoteMap[note - 1] + NOTE_KEYOFF, 0, MAX_BASECHANNELS);

			}
		}
	}
	//end rewbs.vstiLive

	MODCHANNEL *pChn = &m_SndFile.Chn[m_SndFile.m_nChannels];
	for (UINT i=m_SndFile.m_nChannels; i<MAX_CHANNELS; i++, pChn++) if (!pChn->nMasterChn)
	{
		DWORD mask = (bFade) ? CHN_NOTEFADE : (CHN_NOTEFADE|CHN_KEYOFF);

		// Fade all channels > m_nChannels which are playing this note. 
		// Could conflict with NNAs.
		if ((!(pChn->dwFlags & mask)) && (pChn->nLength) && ((note == pChn->nNewNote) || (!note)))
		{
			m_SndFile.KeyOff(i);
			if (!m_SndFile.m_nInstruments) pChn->dwFlags &= ~CHN_LOOP;
			if (bFade) pChn->dwFlags |= CHN_NOTEFADE;
			if (note) break;
		}
	}
	END_CRITICAL();
	return TRUE;
}


BOOL CModDoc::IsNotePlaying(UINT note, UINT nsmp, UINT nins)
//----------------------------------------------------------
{
	MODCHANNEL *pChn = &m_SndFile.Chn[m_SndFile.m_nChannels];
	for (UINT i=m_SndFile.m_nChannels; i<MAX_CHANNELS; i++, pChn++) if (!pChn->nMasterChn)
	{
		if ((pChn->nLength) && (!(pChn->dwFlags & (CHN_NOTEFADE|CHN_KEYOFF|CHN_MUTE)))
		 && ((note == pChn->nNewNote) || (!note))
		 && ((pChn->pModSample == &m_SndFile.Samples[nsmp]) || (!nsmp))
		 && ((pChn->pModInstrument == m_SndFile.Instruments[nins]) || (!nins))) return TRUE;
	}
	return FALSE;
}


bool CModDoc::MuteChannel(CHANNELINDEX nChn, bool doMute)
//-------------------------------------------------------
{
	DWORD muteType = (CMainFrame::m_dwPatternSetup&PATTERN_SYNCMUTE)? CHN_SYNCMUTE:CHN_MUTE;

	if (nChn >= m_SndFile.m_nChannels) {
		return false;
	}

	//Mark channel as muted in channel settings
	if (doMute)	{
		m_SndFile.ChnSettings[nChn].dwFlags |= CHN_MUTE;
	} else {
		m_SndFile.ChnSettings[nChn].dwFlags &= ~CHN_MUTE;
	}
	
	//Mute pattern channel
	if (doMute) {
		m_SndFile.Chn[nChn].dwFlags |= muteType;
		//Kill VSTi notes on muted channel.
		UINT nPlug = m_SndFile.GetBestPlugin(nChn, PRIORITISE_INSTRUMENT, EVEN_IF_MUTED);
		if ((nPlug) && (nPlug<=MAX_MIXPLUGINS))	{
			CVstPlugin *pPlug = (CVstPlugin*)m_SndFile.m_MixPlugins[nPlug-1].pMixPlugin;
			MODINSTRUMENT* pIns = m_SndFile.Chn[nChn].pModInstrument;
			if (pPlug && pIns) {
				pPlug->MidiCommand(pIns->nMidiChannel, pIns->nMidiProgram, pIns->wMidiBank, NOTE_KEYOFF, 0, nChn);
			}
		}
	} else {
		//on unmute alway cater for both mute types - this way there's no probs if user changes mute mode.
		m_SndFile.Chn[nChn].dwFlags &= ~(CHN_SYNCMUTE|CHN_MUTE);
	}

	//mute any NNA'd channels
	for (UINT i=m_SndFile.m_nChannels; i<MAX_CHANNELS; i++) {
		if (m_SndFile.Chn[i].nMasterChn == nChn + 1u)	{
			if (doMute) { 
				m_SndFile.Chn[i].dwFlags |= muteType;
			} else {
				//on unmute alway cater for both mute types - this way there's no probs if user changes mute mode.
				m_SndFile.Chn[i].dwFlags &= ~(CHN_SYNCMUTE|CHN_MUTE);
			}
		}
	}

	//Mark IT/MPTM/S3M as modified
	if (m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT | MOD_TYPE_S3M)) {
		CMainFrame::GetMainFrame()->ThreadSafeSetModified(this);
	}

	return true;
}

// -> CODE#0012
// -> DESC="midi keyboard split"
bool CModDoc::IsChannelSolo(CHANNELINDEX nChn) const
//--------------------------------------------------
{
	if (nChn >= m_SndFile.m_nChannels) return true;
	return (m_SndFile.ChnSettings[nChn].dwFlags & CHN_SOLO) ? true : false;
}

bool CModDoc::SoloChannel(CHANNELINDEX nChn, bool bSolo)
//------------------------------------------------------
{
	if (nChn >= m_SndFile.m_nChannels) return false;
	if (m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();
	if (bSolo)	m_SndFile.ChnSettings[nChn].dwFlags |= CHN_SOLO;
	else		m_SndFile.ChnSettings[nChn].dwFlags &= ~CHN_SOLO;
	return true;
}
// -! NEW_FEATURE#0012


// -> CODE#0015
// -> DESC="channels management dlg"
bool CModDoc::IsChannelNoFx(CHANNELINDEX nChn) const
//------------------------------------------
{
	if (nChn >= m_SndFile.m_nChannels) return true;
	return (m_SndFile.ChnSettings[nChn].dwFlags & CHN_NOFX) ? true : false;
}

bool CModDoc::NoFxChannel(CHANNELINDEX nChn, bool bNoFx, bool updateMix)
//----------------------------------------------------------------------
{
	if (nChn >= m_SndFile.m_nChannels) return false;
	if (m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();
	if (bNoFx){
		m_SndFile.ChnSettings[nChn].dwFlags |= CHN_NOFX;
		if(updateMix) m_SndFile.Chn[nChn].dwFlags |= CHN_NOFX;
	}
	else{
		m_SndFile.ChnSettings[nChn].dwFlags &= ~CHN_NOFX;
		if(updateMix) m_SndFile.Chn[nChn].dwFlags &= ~CHN_NOFX;
	}
	return true;
}


bool CModDoc::IsChannelRecord1(CHANNELINDEX channel) const
//--------------------------------------------------------
{
	return m_bsMultiRecordMask[channel];
}


bool CModDoc::IsChannelRecord2(CHANNELINDEX channel) const
//--------------------------------------------------------
{
	return m_bsMultiSplitRecordMask[channel];
}

BYTE CModDoc::IsChannelRecord(CHANNELINDEX channel) const
//-------------------------------------------------------
{
	if(IsChannelRecord1(channel)) return 1;
	if(IsChannelRecord2(channel)) return 2;
	return 0;
}

void CModDoc::Record1Channel(CHANNELINDEX channel, bool select)
//-------------------------------------------------------------
{
	if (!select)
	{
		m_bsMultiRecordMask.reset(channel);
		m_bsMultiSplitRecordMask.reset(channel);
	}
	else
	{
		m_bsMultiRecordMask.flip(channel);
		m_bsMultiSplitRecordMask.reset(channel);
	}
}

void CModDoc::Record2Channel(CHANNELINDEX channel, bool select)
//-------------------------------------------------------------
{
	if (!select)
	{
		m_bsMultiRecordMask.reset(channel);
		m_bsMultiSplitRecordMask.reset(channel);
	}
	else
	{
		m_bsMultiSplitRecordMask.flip(channel);
		m_bsMultiRecordMask.reset(channel);
	}
}

void CModDoc::ReinitRecordState(bool unselect)
//--------------------------------------------
{
	if (unselect)
	{
		m_bsMultiRecordMask.reset();
		m_bsMultiSplitRecordMask.reset();
	}
	else
	{
		m_bsMultiRecordMask.set();
		m_bsMultiSplitRecordMask.set();
	}
}
// -! NEW_FEATURE#0015


bool CModDoc::MuteSample(SAMPLEINDEX nSample, bool bMute)
//-------------------------------------------------------
{
	if ((nSample < 1) || (nSample > m_SndFile.m_nSamples)) return false;
	if (bMute) m_SndFile.Samples[nSample].uFlags |= CHN_MUTE;
	else m_SndFile.Samples[nSample].uFlags &= ~CHN_MUTE;
	return true;
}

bool CModDoc::MuteInstrument(INSTRUMENTINDEX nInstr, bool bMute)
//--------------------------------------------------------------
{
	if ((nInstr < 1) || (nInstr > m_SndFile.m_nInstruments) || (!m_SndFile.Instruments[nInstr])) return false;
	if (bMute) m_SndFile.Instruments[nInstr]->dwFlags |= INS_MUTE;
	else m_SndFile.Instruments[nInstr]->dwFlags &= ~INS_MUTE;
	return true;
}


bool CModDoc::SurroundChannel(CHANNELINDEX nChn, bool bSurround)
//--------------------------------------------------------------
{
	DWORD d = (bSurround) ? CHN_SURROUND : 0;
	
	if (nChn >= m_SndFile.m_nChannels) return false;
	if (!(m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT))) d = 0;
	if (d != (m_SndFile.ChnSettings[nChn].dwFlags & CHN_SURROUND))
	{
		if (m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();
		if (d)	m_SndFile.ChnSettings[nChn].dwFlags |= CHN_SURROUND;
		else	m_SndFile.ChnSettings[nChn].dwFlags &= ~CHN_SURROUND;
		
	}
	if (d)	m_SndFile.Chn[nChn].dwFlags |= CHN_SURROUND;
	else	m_SndFile.Chn[nChn].dwFlags &= ~CHN_SURROUND;
	return true;
}


bool CModDoc::SetChannelGlobalVolume(CHANNELINDEX nChn, UINT nVolume)
//-------------------------------------------------------------------
{
	bool bOk = false;
	if ((nChn >= m_SndFile.m_nChannels) || (nVolume > 64)) return false;
	if (m_SndFile.ChnSettings[nChn].nVolume != nVolume)
	{
		m_SndFile.ChnSettings[nChn].nVolume = nVolume;
		if (m_SndFile.m_nType & (MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();
		bOk = true;
	}
	m_SndFile.Chn[nChn].nGlobalVol = nVolume;
	return bOk;
}


bool CModDoc::SetChannelDefaultPan(CHANNELINDEX nChn, UINT nPan)
//--------------------------------------------------------------
{
	bool bOk = false;
	if ((nChn >= m_SndFile.m_nChannels) || (nPan > 256)) return false;
	if (m_SndFile.ChnSettings[nChn].nPan != nPan)
	{
		m_SndFile.ChnSettings[nChn].nPan = nPan;
		if (m_SndFile.m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT | MOD_TYPE_MPT)) SetModified();
		bOk = true;
	}
	m_SndFile.Chn[nChn].nPan = nPan;
	return bOk;
}


bool CModDoc::IsChannelMuted(CHANNELINDEX nChn) const
//---------------------------------------------------
{
	if (nChn >= m_SndFile.m_nChannels) return true;
	return (m_SndFile.ChnSettings[nChn].dwFlags & CHN_MUTE) ? true : false;
}


bool CModDoc::IsSampleMuted(SAMPLEINDEX nSample) const
//----------------------------------------------------
{
	if ((!nSample) || (nSample > m_SndFile.m_nSamples)) return false;
	return (m_SndFile.Samples[nSample].uFlags & CHN_MUTE) ? true : false;
}


bool CModDoc::IsInstrumentMuted(INSTRUMENTINDEX nInstr) const
//-----------------------------------------------------------
{
	if ((!nInstr) || (nInstr > m_SndFile.m_nInstruments) || (!m_SndFile.Instruments[nInstr])) return false;
	return (m_SndFile.Instruments[nInstr]->dwFlags & INS_MUTE) ? true : false;
}


UINT CModDoc::GetPatternSize(PATTERNINDEX nPat) const
//---------------------------------------------------
{
	if ((nPat < m_SndFile.Patterns.Size()) && (m_SndFile.Patterns[nPat])) return m_SndFile.PatternSize[nPat];
	return 0;
}


void CModDoc::SetFollowWnd(HWND hwnd, DWORD dwType)
//-------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	m_hWndFollow = hwnd;
	m_dwNotifyType = dwType;
	if (pMainFrm) pMainFrm->SetFollowSong(this, m_hWndFollow, TRUE, m_dwNotifyType);
}


BOOL CModDoc::IsChildSample(UINT nIns, UINT nSmp) const
//-----------------------------------------------------
{
	MODINSTRUMENT *pIns;
	if ((nIns < 1) || (nIns > m_SndFile.m_nInstruments)) return FALSE;
	pIns = m_SndFile.Instruments[nIns];
	if (pIns)
	{
		for (UINT i=0; i<NOTE_MAX; i++)
		{
			if (pIns->Keyboard[i] == nSmp) return TRUE;
		}
	}
	return FALSE;
}


UINT CModDoc::FindSampleParent(UINT nSmp) const
//---------------------------------------------
{
	if ((!m_SndFile.m_nInstruments) || (!nSmp)) return 0;
	for (UINT i=1; i<=m_SndFile.m_nInstruments; i++)
	{
		MODINSTRUMENT *pIns = m_SndFile.Instruments[i];
		if (pIns)
		{
			for (UINT j=0; j<NOTE_MAX; j++)
			{
				if (pIns->Keyboard[j] == nSmp) return i;
			}
		}
	}
	return 0;
}


UINT CModDoc::FindInstrumentChild(UINT nIns) const
//------------------------------------------------
{
	MODINSTRUMENT *pIns;
	if ((!nIns) || (nIns > m_SndFile.m_nInstruments)) return 0;
	pIns = m_SndFile.Instruments[nIns];
	if (pIns)
	{
		for (UINT i=0; i<NOTE_MAX; i++)
		{
			UINT n = pIns->Keyboard[i];
			if ((n) && (n <= m_SndFile.m_nSamples)) return n;
		}
	}
	return 0;
}


LRESULT CModDoc::ActivateView(UINT nIdView, DWORD dwParam)
//--------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (!pMainFrm) return 0;
	CMDIChildWnd *pMDIActive = pMainFrm->MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if ((pView) && (pView->GetDocument() == this))
		{
			return ((CChildFrame *)pMDIActive)->ActivateView(nIdView, dwParam);
		}
	}
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pView = GetNextView(pos);
		if ((pView) && (pView->GetDocument() == this))
		{
			CChildFrame *pChildFrm = (CChildFrame *)pView->GetParentFrame();
			pChildFrm->MDIActivate();
			return pChildFrm->ActivateView(nIdView, dwParam);
		}
	}
	return 0;
}


// Activate document's window.
void CModDoc::ActivateWindow()
//----------------------------
{
	
	CChildFrame *pChildFrm = (CChildFrame *)GetChildFrame();
	if(pChildFrm) pChildFrm->MDIActivate();
}


void CModDoc::UpdateAllViews(CView *pSender, LPARAM lHint, CObject *pHint)
//------------------------------------------------------------------------
{
	CDocument::UpdateAllViews(pSender, lHint, pHint);
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->UpdateTree(this, lHint, pHint);
}


/////////////////////////////////////////////////////////////////////////////
// CModDoc commands

void CModDoc::OnFileWaveConvert()
//-------------------------------
{
	OnFileWaveConvert(ORDERINDEX_INVALID, ORDERINDEX_INVALID);
}

void CModDoc::OnFileWaveConvert(ORDERINDEX nMinOrder, ORDERINDEX nMaxOrder)
//-------------------------------------------------------------------------
{
	TCHAR fname[_MAX_FNAME]="";
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if ((!pMainFrm) || (!m_SndFile.GetType())) return;
	_splitpath(GetPathName(), NULL, NULL, fname, NULL);

	CWaveConvert wsdlg(pMainFrm, nMinOrder, nMaxOrder);
	if (wsdlg.DoModal() != IDOK) return;

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "wav", fname,
		"Wave Files (*.wav)|*.wav||",
		CMainFrame::GetWorkingDirectory(DIR_EXPORT));
	if(files.abort) return;

	// will set default dir here because there's no setup option for export dir yet (feel free to add one...)
	CMainFrame::SetDefaultDirectory(files.workingDirectory.c_str(), DIR_EXPORT, true);

	TCHAR s[_MAX_PATH];
	strcpy(s, files.first_file.c_str());

	// Saving as wave file
	UINT p = 0, n = 1;
	DWORD flags[MAX_BASECHANNELS];
	CHAR channel[MAX_CHANNELNAME+10];

	// Channel mode : save song in multiple wav files (one for each enabled channels)
	if(wsdlg.m_bChannelMode){
		n = m_SndFile.m_nChannels;
		for(UINT i = 0 ; i < n ; i++){
			// Save channels' flags
			flags[i] = m_SndFile.ChnSettings[i].dwFlags;
			// Mute each channel
			m_SndFile.ChnSettings[i].dwFlags |= CHN_MUTE;
		}
		// Keep position of the caracter just before ".wav" in path string
		p = strlen(s) - 4;
	}

	CDoWaveConvert dwcdlg(&m_SndFile, s, &wsdlg.WaveFormat.Format, wsdlg.m_bNormalize, pMainFrm);
	dwcdlg.m_dwFileLimit = static_cast<DWORD>(wsdlg.m_dwFileLimit);
	dwcdlg.m_bGivePlugsIdleTime = wsdlg.m_bGivePlugsIdleTime;
	dwcdlg.m_dwSongLimit = wsdlg.m_dwSongLimit;
	dwcdlg.m_nMaxPatterns = (wsdlg.m_bSelectPlay) ? wsdlg.m_nMaxOrder - wsdlg.m_nMinOrder + 1 : 0;
	//if(wsdlg.m_bHighQuality) CSoundFile::SetResamplingMode(SRCMODE_POLYPHASE);

	BOOL bplaying = FALSE;
	UINT pos = m_SndFile.GetCurrentPos();
	bplaying = TRUE;
	pMainFrm->PauseMod();

	for(UINT i = 0 ; i < n ; i++){

		// Channel mode
		if(wsdlg.m_bChannelMode){
			// Add channel number & name (if available) to path string
			if(m_SndFile.ChnSettings[i].szName[0] >= 0x20)
				wsprintf(channel, "-%03d_%s.wav", i+1,m_SndFile.ChnSettings[i].szName);
			else
				wsprintf(channel, "-%03d.wav", i+1);
			s[p] = '\0';
			strcat(s,channel);
			// Unmute channel to process
			m_SndFile.ChnSettings[i].dwFlags &= ~CHN_MUTE;
		}

		// Render song (or current channel if channel mode and channel not initially disabled)
		if(!wsdlg.m_bChannelMode || !(flags[i] & CHN_MUTE)){
			// rewbs.fix3239
			m_SndFile.SetCurrentPos(0);
			m_SndFile.m_dwSongFlags &= ~SONG_PATTERNLOOP;
			if (wsdlg.m_bSelectPlay) {
				m_SndFile.SetCurrentOrder(wsdlg.m_nMinOrder);
				m_SndFile.m_nCurrentPattern = wsdlg.m_nMinOrder;
				m_SndFile.GetLength(TRUE, FALSE);
				m_SndFile.m_nMaxOrderPosition = wsdlg.m_nMaxOrder + 1;
			}
			//end rewbs.fix3239
			if( dwcdlg.DoModal() != IDOK ) break;	// UPDATE#03
		}

		// Re-mute processed channel
		if(wsdlg.m_bChannelMode) m_SndFile.ChnSettings[i].dwFlags |= CHN_MUTE;
	}

	// Restore channels' flags
	if(wsdlg.m_bChannelMode){
		for(UINT i = 0 ; i < n ; i++) m_SndFile.ChnSettings[i].dwFlags = flags[i];
	}

	m_SndFile.SetCurrentPos(pos);
	m_SndFile.GetLength(TRUE);
	CMainFrame::UpdateAudioParameters(TRUE);
}


void CModDoc::OnFileMP3Convert()
//------------------------------
{
	int nFilterIndex = 0;
	TCHAR sFName[_MAX_FNAME] = "";
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if ((!pMainFrm) || (!m_SndFile.GetType())) return;
	_splitpath(GetPathName(), NULL, NULL, sFName, NULL);

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "mp3", sFName,
		"MPEG Layer III Files (*.mp3)|*.mp3|Layer3 Wave Files (*.wav)|*.wav||",
		CMainFrame::GetWorkingDirectory(DIR_EXPORT),
		false,
		&nFilterIndex);
	if(files.abort) return;

	MPEGLAYER3WAVEFORMAT wfx;
	HACMDRIVERID hadid;

	// will set default dir here because there's no setup option for export dir yet (feel free to add one...)
	pMainFrm->SetDefaultDirectory(files.workingDirectory.c_str(), DIR_EXPORT, true);

	TCHAR s[_MAX_PATH], fext[_MAX_EXT];
	strcpy(s, files.first_file.c_str());
	_splitpath(s, NULL, NULL, NULL, fext);
	if (strlen(fext) <= 1)
	{
		int l = strlen(s) - 1;
		if ((l >= 0) && (s[l] == '.')) s[l] = 0;
		strcpy(fext, (nFilterIndex == 2) ? ".wav" : ".mp3");
		strcat(s, fext);
	}
	CLayer3Convert wsdlg(&m_SndFile, pMainFrm);
	if (m_SndFile.m_szNames[0][0]) wsdlg.m_bSaveInfoField = TRUE;
	if (wsdlg.DoModal() != IDOK) return;
	wsdlg.GetFormat(&wfx, &hadid);
	// Saving as mpeg file
	BOOL bplaying = FALSE;
	UINT pos = m_SndFile.GetCurrentPos();
	bplaying = TRUE;
	pMainFrm->PauseMod();
	m_SndFile.SetCurrentPos(0);

	m_SndFile.m_dwSongFlags &= ~SONG_PATTERNLOOP;

	// Saving file
	CFileTagging *pTag = (wsdlg.m_bSaveInfoField) ? &wsdlg.m_FileTags : NULL;
	CDoAcmConvert dwcdlg(&m_SndFile, s, &wfx.wfx, hadid, pTag, pMainFrm);
	dwcdlg.m_dwFileLimit = wsdlg.m_dwFileLimit;
	dwcdlg.m_dwSongLimit = wsdlg.m_dwSongLimit;
	dwcdlg.DoModal();
	m_SndFile.SetCurrentPos(pos);
	m_SndFile.GetLength(TRUE);
	CMainFrame::UpdateAudioParameters(TRUE);
}


void CModDoc::OnFileMidiConvert()
//-------------------------------------
{
	CHAR path[_MAX_PATH]="", drive[_MAX_DRIVE]="";
	CHAR s[_MAX_PATH], fname[_MAX_FNAME]="";
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	if ((!pMainFrm) || (!m_SndFile.GetType())) return;
	_splitpath(GetPathName(), drive, path, fname, NULL);
	strcpy(s, drive);
	strcat(s, path);
	strcat(s, fname);
	strcat(s, ".mid");

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "mid", s,
		"Midi Files (*.mid,*.rmi)|*.mid;*.rmi||");
	if(files.abort) return;

	CModToMidi mididlg(files.first_file.c_str(), &m_SndFile, pMainFrm);
	if (mididlg.DoModal() == IDOK)
	{
		BeginWaitCursor();
		mididlg.DoConvert();
		EndWaitCursor();
	}
}

//HACK: This is a quick fix. Needs to be better integrated into player and GUI.
void CModDoc::OnFileCompatibilitySave()
//-------------------------------------
{
	CHAR path[_MAX_PATH]="", drive[_MAX_DRIVE]="";
	CHAR fname[_MAX_FNAME]="";
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	std::string ext, pattern, filename;
	
	UINT type = m_SndFile.GetType();

	if ((!pMainFrm) || (!m_SndFile.GetType())) return;
	switch (type)
	{
		/*case MOD_TYPE_XM:
			ext = "xm";
			pattern = "Fast Tracker Files (*.xm)|*.xm||";
			break;*/
		case MOD_TYPE_MOD:
			ext = ModSpecs::mod.fileExtension;
			pattern = FileFilterMOD;
			if( AfxMessageBox(GetStrI18N(TEXT(
				"Compared to regular MOD save, compatibility export makes "
				"small adjustments to the save file in order to make the file compatible with "
				"ProTracker and other Amiga-based trackers. Note that this feature is not complete and the "
				"file is not guaranteed to be free of MPT-specific features.\n\n "
				"Important: beginning of some samples may be adjusted in the process. Proceed?")), MB_ICONINFORMATION|MB_YESNO) != IDYES
				)
				return;
			break;
		case MOD_TYPE_IT:
			ext = ModSpecs::it.fileExtension;
			pattern = FileFilterIT;
			::MessageBox(NULL,"Warning: the exported file will not contain any of MPT's file-format hacks.", "Compatibility export warning.",MB_ICONINFORMATION | MB_OK);
			break;
		case MOD_TYPE_XM:
			ext = ModSpecs::xm.fileExtension;
			pattern = FileFilterXM;
			::MessageBox(NULL,"Warning: the exported file will not contain any of MPT's file-format hacks.", "Compatibility export warning.",MB_ICONINFORMATION | MB_OK);
			break;
		default:
			::MessageBox(NULL,"Compatibility export is currently only available for MOD, XM and IT modules.", "Can't do compatibility export.",MB_ICONINFORMATION | MB_OK);
			return;
	}

	_splitpath(GetPathName(), drive, path, fname, NULL);
	filename = drive;
	filename += path;
	filename += fname;
	if (!strstr(fname, "compat"))
		filename += ".compat.";
	else
		filename += ".";
	filename += ext;

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, ext, filename, pattern, CMainFrame::GetWorkingDirectory(DIR_MODS));
	if(files.abort) return;

	switch (type)
	{
		case MOD_TYPE_MOD:
			m_SndFile.SaveMod(files.first_file.c_str(), 0, true);
			SetModified(); // Compatibility save may adjust samples so set modified...
			m_ShowSavedialog = true;	// ...and force save dialog to appear when saving.
			break;
		case MOD_TYPE_XM:
			m_SndFile.SaveXM(files.first_file.c_str(), 0, true);
			break;
		case MOD_TYPE_IT:
			m_SndFile.SaveCompatIT(files.first_file.c_str());
			break;
	}
}


void CModDoc::OnPlayerPlay()
//--------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		CChildFrame *pChildFrm = (CChildFrame *) GetChildFrame();
 		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play song command: set loop pattern checkbox to false.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 0);
		}
                 
		BOOL bPlaying = (pMainFrm->GetModPlaying() == this) ? TRUE : FALSE;
		if ((bPlaying) && (!(m_SndFile.m_dwSongFlags & (SONG_PAUSED|SONG_STEP/*|SONG_PATTERNLOOP*/))))
		{
			OnPlayerPause();
			return;
		}
		BEGIN_CRITICAL();
		for (UINT i=m_SndFile.m_nChannels; i<MAX_CHANNELS; i++) if (!m_SndFile.Chn[i].nMasterChn)
		{
			m_SndFile.Chn[i].dwFlags |= (CHN_NOTEFADE|CHN_KEYOFF);
			if (!bPlaying) m_SndFile.Chn[i].nLength = 0;
		}
		if (bPlaying) {
			m_SndFile.StopAllVsti();
		} else {
			m_SndFile.ResumePlugins();
		}
		END_CRITICAL();
		//m_SndFile.m_dwSongFlags &= ~(SONG_STEP|SONG_PAUSED);
		m_SndFile.m_dwSongFlags &= ~(SONG_STEP|SONG_PAUSED|SONG_PATTERNLOOP);
		pMainFrm->PlayMod(this, m_hWndFollow, m_dwNotifyType);
	}
}


void CModDoc::OnPlayerPause()
//---------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		if (pMainFrm->GetModPlaying() == this)
		{
			BOOL bLoop = (m_SndFile.m_dwSongFlags & SONG_PATTERNLOOP) ? TRUE : FALSE;
			UINT nPat = m_SndFile.m_nPattern;
			UINT nRow = m_SndFile.m_nRow;
			UINT nNextRow = m_SndFile.m_nNextRow;
			pMainFrm->PauseMod();
			BEGIN_CRITICAL();
			if ((bLoop) && (nPat < m_SndFile.Patterns.Size()))
			{
				if ((m_SndFile.m_nCurrentPattern < m_SndFile.Order.size()) && (m_SndFile.Order[m_SndFile.m_nCurrentPattern] == nPat))
				{
					m_SndFile.m_nNextPattern = m_SndFile.m_nCurrentPattern;
					m_SndFile.m_nNextRow = nNextRow;
					m_SndFile.m_nRow = nRow;
				} else
				{
					for (ORDERINDEX nOrd = 0; nOrd < m_SndFile.Order.size(); nOrd++)
					{
						if (m_SndFile.Order[nOrd] == m_SndFile.Order.GetInvalidPatIndex()) break;
						if (m_SndFile.Order[nOrd] == nPat)
						{
							m_SndFile.m_nCurrentPattern = nOrd;
							m_SndFile.m_nNextPattern = nOrd;
							m_SndFile.m_nNextRow = nNextRow;
							m_SndFile.m_nRow = nRow;
							break;
						}
					}
				}
			}
			END_CRITICAL();
		} else
		{
			pMainFrm->PauseMod();
		}
	}
}


void CModDoc::OnPlayerStop()
//--------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->StopMod();
}


void CModDoc::OnPlayerPlayFromStart()
//-----------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		CChildFrame *pChildFrm = (CChildFrame *) GetChildFrame();
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play song command: set loop pattern checkbox to false.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 0);
		}
		

		pMainFrm->PauseMod();
		//m_SndFile.m_dwSongFlags &= ~SONG_STEP;
		m_SndFile.m_dwSongFlags &= ~(SONG_STEP|SONG_PATTERNLOOP);
		m_SndFile.SetCurrentPos(0);
		pMainFrm->ResetElapsedTime();
		BEGIN_CRITICAL();
		m_SndFile.ResumePlugins();
		END_CRITICAL();
		pMainFrm->PlayMod(this, m_hWndFollow, m_dwNotifyType);
		CMainFrame::EnableLowLatencyMode(FALSE);
	}
}
/*
void CModDoc::OnPlayerPlayFromPos(UINT pos)
//-----------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm)
	{
		pMainFrm->PauseMod();
		m_SndFile.m_dwSongFlags &= ~SONG_STEP;
		m_SndFile.SetCurrentPos(pos);
		pMainFrm->ResetElapsedTime();
		pMainFrm->PlayMod(this, m_hWndFollow, m_dwNotifyType);
		CMainFrame::EnableLowLatencyMode(FALSE);
	}
}
*/

void CModDoc::OnEditGlobals()
//---------------------------
{
	SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW,	IDD_CONTROL_GLOBALS);
}


void CModDoc::OnEditPatterns()
//----------------------------
{
	SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_PATTERNS, -1);
}


void CModDoc::OnEditSamples()
//---------------------------
{
	SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_SAMPLES);
}


void CModDoc::OnEditInstruments()
//-------------------------------
{
	//if (m_SndFile.m_nInstruments) rewbs.cosmetic: allow keyboard access to instruments even with no instruments
	SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_INSTRUMENTS);
}


void CModDoc::OnEditComments()
//----------------------------
{
	if (m_SndFile.m_nType & (MOD_TYPE_XM|MOD_TYPE_IT | MOD_TYPE_MPT)) SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_COMMENTS);
}

//rewbs.graph
void CModDoc::OnEditGraph()
//-------------------------
{
	if (m_SndFile.m_nType & (MOD_TYPE_XM|MOD_TYPE_IT | MOD_TYPE_MPT)) SendMessageToActiveViews(WM_MOD_ACTIVATEVIEW, IDD_CONTROL_GRAPH);
}
//end rewbs.graph


void CModDoc::OnShowCleanup()
//---------------------------
{
	CModCleanupDlg dlg(this, CMainFrame::GetMainFrame());
	dlg.DoModal();
}


void CModDoc::OnUpdateHasMIDIMappings(CCmdUI *p)
//----------------------------------------------
{
	if(!p) return;
	if(m_SndFile.GetModSpecifications().MIDIMappingDirectivesMax > 0)
		p->Enable();
	else
		p->Enable(FALSE);
}


void CModDoc::OnUpdateXMITMPTOnly(CCmdUI *p)
//---------------------------------------
{
	if (p)
		p->Enable((m_SndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE);
}


void CModDoc::OnUpdateMP3Encode(CCmdUI *p)
//----------------------------------------
{
	if (p) p->Enable(theApp.CanEncodeLayer3());
}


void CModDoc::OnInsertPattern()
//-----------------------------
{
	LONG pat = InsertPattern();
	if (pat >= 0)
	{
		UINT ord = 0;
		for (UINT i=0; i<m_SndFile.Order.size(); i++)
		{
			if (m_SndFile.Order[i] == pat) ord = i;
			if (m_SndFile.Order[i] == m_SndFile.Order.GetInvalidPatIndex()) break;
		}
		ViewPattern(pat, ord);
	}
}


void CModDoc::OnInsertSample()
//----------------------------
{
	LONG smp = InsertSample();
	if (smp != SAMPLEINDEX_INVALID) ViewSample(smp);
}


void CModDoc::OnInsertInstrument()
//--------------------------------
{
	LONG ins = InsertInstrument();
	if (ins != INSTRUMENTINDEX_INVALID) ViewInstrument(ins);
}



void CModDoc::OnEstimateSongLength()
//----------------------------------
{
	CHAR s[256];
	DWORD dwSongLength = m_SndFile.GetSongTime();
	wsprintf(s, "Approximate song length: %dmn%02ds", dwSongLength/60, dwSongLength%60);
	CMainFrame::GetMainFrame()->MessageBox(s, NULL, MB_OK|MB_ICONINFORMATION);
}

void CModDoc::OnApproximateBPM()
//----------------------------------
{
	//Convert BPM to string:
	CString Message;
	double bpm = CMainFrame::GetMainFrame()->GetApproxBPM();


	switch(m_SndFile.m_nTempoMode) {
		case tempo_mode_alternative: 
			Message.Format("Using alternative tempo interpretation.\n\nAssuming:\n. %d ticks per second\n. %d ticks per row\n. %d rows per beat\nthe tempo is approximately: %.20g BPM",
			m_SndFile.m_nMusicTempo, m_SndFile.m_nMusicSpeed, m_SndFile.m_nRowsPerBeat, bpm); 
			break;

		case tempo_mode_modern: 
			Message.Format("Using modern tempo interpretation.\n\nThe tempo is: %.20g BPM", bpm); 
			break;

		case tempo_mode_classic: 
		default:
			Message.Format("Using standard tempo interpretation.\n\nAssuming:\n. A mod tempo (tick duration factor) of %d\n. %d ticks per row\n. %d rows per beat\nthe tempo is approximately: %.20g BPM",
			m_SndFile.m_nMusicTempo, m_SndFile.m_nMusicSpeed, m_SndFile.m_nRowsPerBeat, bpm); 
			break;
	}

	CMainFrame::GetMainFrame()->MessageBox(Message, NULL, MB_OK|MB_ICONINFORMATION);
}



///////////////////////////////////////////////////////////////////////////
// Effects description

typedef struct MPTEFFECTINFO
{
	DWORD dwEffect;		// CMD_XXXX
	DWORD dwParamMask;	// 0 = default
	DWORD dwParamValue;	// 0 = default
	DWORD dwFlags;		// FXINFO_XXXX
	DWORD dwFormats;	// MOD_TYPE_XXX combo
	LPCSTR pszName;		// ie "Tone Portamento"
} MPTEFFECTINFO;

#define MOD_TYPE_MODXM	(MOD_TYPE_MOD|MOD_TYPE_XM)
#define MOD_TYPE_S3MIT	(MOD_TYPE_S3M|MOD_TYPE_IT)
#define MOD_TYPE_S3MITMPT (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_MPT)
#define MOD_TYPE_NOMOD	(MOD_TYPE_S3M|MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)
#define MOD_TYPE_XMIT	(MOD_TYPE_XM|MOD_TYPE_IT)
#define MOD_TYPE_XMITMPT (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)
#define MOD_TYPE_ITMPT (MOD_TYPE_IT|MOD_TYPE_MPT)
#define MAX_FXINFO		69


const MPTEFFECTINFO gFXInfo[MAX_FXINFO] =
{
	{CMD_ARPEGGIO,		0,0,		0,	0xFFFFFFFF,		"Arpeggio"},
	{CMD_PORTAMENTOUP,	0,0,		0,	0xFFFFFFFF,		"Portamento Up"},
	{CMD_PORTAMENTODOWN,0,0,		0,	0xFFFFFFFF,		"Portamento Down"},
	{CMD_TONEPORTAMENTO,0,0,		0,	0xFFFFFFFF,		"Tone portamento"},
	{CMD_VIBRATO,		0,0,		0,	0xFFFFFFFF,		"Vibrato"},
	{CMD_TONEPORTAVOL,	0,0,		0,	0xFFFFFFFF,		"Volslide+Toneporta"},
	{CMD_VIBRATOVOL,	0,0,		0,	0xFFFFFFFF,		"VolSlide+Vibrato"},
	{CMD_TREMOLO,		0,0,		0,	0xFFFFFFFF,		"Tremolo"},
	{CMD_PANNING8,		0,0,		0,	0xFFFFFFFF,		"Set Panning"},
	{CMD_OFFSET,		0,0,		0,	0xFFFFFFFF,		"Set Offset"},
	{CMD_VOLUMESLIDE,	0,0,		0,	0xFFFFFFFF,		"Volume Slide"},
	{CMD_POSITIONJUMP,	0,0,		0,	0xFFFFFFFF,		"Position Jump"},
	{CMD_VOLUME,		0,0,		0,	MOD_TYPE_MODXM,	"Set Volume"},
	{CMD_PATTERNBREAK,	0,0,		0,	0xFFFFFFFF,		"Pattern Break"},
	{CMD_RETRIG,		0,0,		0,	MOD_TYPE_NOMOD,	"Retrigger Note"},
	{CMD_SPEED,			0,0,		0,	0xFFFFFFFF,		"Set Speed"},
	{CMD_TEMPO,			0,0,		0,	0xFFFFFFFF,		"Set Tempo"},
	{CMD_TREMOR,		0,0,		0,	MOD_TYPE_NOMOD,	"Tremor"},
	{CMD_CHANNELVOLUME,	0,0,		0,	MOD_TYPE_S3MITMPT,	"Set channel volume"},
	{CMD_CHANNELVOLSLIDE,0,0,		0,	MOD_TYPE_S3MITMPT,	"Channel volslide"},
	{CMD_GLOBALVOLUME,	0,0,		0,	MOD_TYPE_NOMOD,	"Set global volume"},
	{CMD_GLOBALVOLSLIDE,0,0,		0,	MOD_TYPE_NOMOD,	"Global volume slide"},
	{CMD_KEYOFF,		0,0,		0,	MOD_TYPE_XM,	"Key off"},
	{CMD_FINEVIBRATO,	0,0,		0,	MOD_TYPE_S3MITMPT,	"Fine vibrato"},
	{CMD_PANBRELLO,		0,0,		0,	MOD_TYPE_NOMOD,	"Panbrello"},
	{CMD_PANNINGSLIDE,	0,0,		0,	MOD_TYPE_NOMOD,	"Panning slide"},
	{CMD_SETENVPOSITION,0,0,		0,	MOD_TYPE_XM,	"Envelope position"},
	{CMD_MIDI,			0,0,		0x7F,	MOD_TYPE_NOMOD,	"Midi macro"},
	{CMD_SMOOTHMIDI,	0,0,		0x7F,	MOD_TYPE_NOMOD,	"Smooth Midi macro"},	//rewbs.smoothVST
	// Extended MOD/XM effects
	{CMD_MODCMDEX,		0xF0,0x10,	0,	MOD_TYPE_MODXM,	"Fine porta up"},
	{CMD_MODCMDEX,		0xF0,0x20,	0,	MOD_TYPE_MODXM,	"Fine porta down"},
	{CMD_MODCMDEX,		0xF0,0x30,	0,	MOD_TYPE_MODXM,	"Glissando Control"},
	{CMD_MODCMDEX,		0xF0,0x40,	0,	MOD_TYPE_MODXM,	"Vibrato waveform"},
	{CMD_MODCMDEX,		0xF0,0x50,	0,	MOD_TYPE_MOD,	"Set finetune"},
	{CMD_MODCMDEX,		0xF0,0x60,	0,	MOD_TYPE_MODXM,	"Pattern loop"},
	{CMD_MODCMDEX,		0xF0,0x70,	0,	MOD_TYPE_MODXM,	"Tremolo waveform"},
	{CMD_MODCMDEX,		0xF0,0x80,	0,	MOD_TYPE_MODXM,	"Set panning"},
	{CMD_MODCMDEX,		0xF0,0x90,	0,	MOD_TYPE_MODXM,	"Retrigger note"},
	{CMD_MODCMDEX,		0xF0,0xA0,	0,	MOD_TYPE_MODXM,	"Fine volslide up"},
	{CMD_MODCMDEX,		0xF0,0xB0,	0,	MOD_TYPE_MODXM,	"Fine volslide down"},
	{CMD_MODCMDEX,		0xF0,0xC0,	0,	MOD_TYPE_MODXM,	"Note cut"},
	{CMD_MODCMDEX,		0xF0,0xD0,	0,	MOD_TYPE_MODXM,	"Note delay"},
	{CMD_MODCMDEX,		0xF0,0xE0,	0,	MOD_TYPE_MODXM,	"Pattern delay"},
	{CMD_MODCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_XM,	"Set active macro"},
	{CMD_MODCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_MOD,	"Invert Loop"},
	// Extended S3M/IT effects
	{CMD_S3MCMDEX,		0xF0,0x10,	0,	MOD_TYPE_S3MITMPT,	"Glissando control"},
	{CMD_S3MCMDEX,		0xF0,0x20,	0,	MOD_TYPE_S3M,	"Set finetune"},
	{CMD_S3MCMDEX,		0xF0,0x30,	0,	MOD_TYPE_S3MITMPT,	"Vibrato waveform"},
	{CMD_S3MCMDEX,		0xF0,0x40,	0,	MOD_TYPE_S3MITMPT,	"Tremolo waveform"},
	{CMD_S3MCMDEX,		0xF0,0x50,	0,	MOD_TYPE_S3MITMPT,	"Panbrello waveform"},
	{CMD_S3MCMDEX,		0xF0,0x60,	0,	MOD_TYPE_S3MITMPT,	"Fine pattern delay"},
	{CMD_S3MCMDEX,		0xF0,0x80,	0,	MOD_TYPE_S3MITMPT,	"Set panning"},
	{CMD_S3MCMDEX,		0xF0,0xA0,	0,	MOD_TYPE_S3MITMPT,	"Set high offset"},
	{CMD_S3MCMDEX,		0xF0,0xB0,	0,	MOD_TYPE_S3MITMPT,	"Pattern loop"},
	{CMD_S3MCMDEX,		0xF0,0xC0,	0,	MOD_TYPE_S3MITMPT,	"Note cut"},
	{CMD_S3MCMDEX,		0xF0,0xD0,	0,	MOD_TYPE_S3MITMPT,	"Note delay"},
	{CMD_S3MCMDEX,		0xF0,0xE0,	0,	MOD_TYPE_S3MITMPT,	"Pattern delay"},
	{CMD_S3MCMDEX,		0xF0,0xF0,	0,	MOD_TYPE_ITMPT,	"Set active macro"},
	// MPT XM extensions and special effects
	{CMD_XFINEPORTAUPDOWN,0xF0,0x10,0,	MOD_TYPE_XM,	"Extra fine porta up"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x20,0,	MOD_TYPE_XM,	"Extra fine porta down"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x50,0,	MOD_TYPE_XM,	"Panbrello waveform"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x60,0,	MOD_TYPE_XM,	"Fine pattern delay"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0x90,0,	MOD_TYPE_XM,	"Sound control"},
	{CMD_XFINEPORTAUPDOWN,0xF0,0xA0,0,	MOD_TYPE_XM,	"Set high offset"},
	// MPT IT extensions and special effects
	{CMD_S3MCMDEX,		0xF0,0x90,	0,	MOD_TYPE_S3MITMPT,	"Sound control"},
	{CMD_S3MCMDEX,		0xF0,0x70,	0,	MOD_TYPE_ITMPT,	"Instr. control"},
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
	{CMD_XPARAM,		0x00,0x00,	0,	MOD_TYPE_XMITMPT,	"X param"},
// -! NEW_FEATURE#0010
	{CMD_NOTESLIDEUP,	0x00,0x00,	0,	0,	"Note Slide Up"}, // .IMF effect
	{CMD_NOTESLIDEDOWN,	0x00,0x00,	0,	0,	"Note Slide Down"}, // .IMF effect
};


UINT CModDoc::GetNumEffects() const
//---------------------------------
{
	return MAX_FXINFO;
}


BOOL CModDoc::IsExtendedEffect(UINT ndx) const
//--------------------------------------------
{
	return ((ndx < MAX_FXINFO) && (gFXInfo[ndx].dwParamMask));
}


bool CModDoc::GetEffectName(LPSTR pszDescription, UINT command, UINT param, bool bXX, CHANNELINDEX nChn) //rewbs.xinfo: added chan arg
//------------------------------------------------------------------------------------------------------
{
	bool bSupported;
	UINT fxndx = MAX_FXINFO;
	pszDescription[0] = 0;
	for (UINT i = 0; i < MAX_FXINFO; i++)
	{
		if ((command == gFXInfo[i].dwEffect) // Effect
		 && ((param & gFXInfo[i].dwParamMask) == gFXInfo[i].dwParamValue)) // Value
		{
			fxndx = i;
			// if format is compatible, everything is fine. if not, let's still search
			// for another command. this fixes searching for the EFx command, which
			// does different things in MOD format.
			if((m_SndFile.m_nType & gFXInfo[i].dwFormats) != 0)
				break;
		}
	}
	if (fxndx == MAX_FXINFO) return false;
	bSupported = ((m_SndFile.m_nType & gFXInfo[fxndx].dwFormats) != 0);
	if (gFXInfo[fxndx].pszName)
	{
		if ((bXX) && (bSupported))
		{
			strcpy(pszDescription, " xx: ");
			LPCSTR pszCmd = (m_SndFile.m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) ? gszModCommands : gszS3mCommands;
			pszDescription[0] = pszCmd[command];
			if ((gFXInfo[fxndx].dwParamMask & 0xF0) == 0xF0) pszDescription[1] = szHexChar[gFXInfo[fxndx].dwParamValue >> 4];
			if ((gFXInfo[fxndx].dwParamMask & 0x0F) == 0x0F) pszDescription[2] = szHexChar[gFXInfo[fxndx].dwParamValue & 0x0F];
		}
		strcat(pszDescription, gFXInfo[fxndx].pszName);
		//rewbs.xinfo
		//Get channel specific info
		if (nChn < m_SndFile.m_nChannels)
		{
			CString chanSpec = "";
			CString macroText= "_no macro_";
			switch (command)
			{
			case CMD_MODCMDEX:
			case CMD_S3MCMDEX:
				if ((param & 0xF0) == 0xF0 && !(m_SndFile.m_nType & MOD_TYPE_MOD))	//Set Macro
				{
					macroText = &m_SndFile.m_MidiCfg.szMidiSFXExt[(param & 0x0F) * 32];
					chanSpec.Format(" to %d: ", param & 0x0F);
				}
				break;
			case CMD_MIDI:
			case CMD_SMOOTHMIDI:
				if (param < 0x80 && nChn != CHANNELINDEX_INVALID)
				{
					macroText = &m_SndFile.m_MidiCfg.szMidiSFXExt[m_SndFile.Chn[nChn].nActiveMacro*32];
					chanSpec.Format(": currently %d: ", m_SndFile.Chn[nChn].nActiveMacro);
				}
				else
				{
					chanSpec = " (Fixed)";
				}
				break;
			}
			
			if (macroText != "_no macro_")
			{
				switch (GetMacroType(macroText))
				{
				case sfx_unused: chanSpec.Append("Unused"); break;
				case sfx_cutoff: chanSpec.Append("Cutoff"); break;
				case sfx_reso: chanSpec.Append("Resonance"); break;
				case sfx_mode: chanSpec.Append("Filter Mode"); break;
				case sfx_drywet: chanSpec.Append("Plug wet/dry ratio"); break;
				case sfx_cc: {
					int nCC = MacroToMidiCC(macroText);
					CString temp; 
					temp.Format("MidiCC %d", nCC);
					chanSpec.Append(temp); 
					break;
				}
				case sfx_plug: {
					int nParam = MacroToPlugParam(macroText);
					char paramName[128];
					memset(paramName, 0, sizeof(paramName));				
					UINT nPlug = m_SndFile.GetBestPlugin(nChn, PRIORITISE_CHANNEL, EVEN_IF_MUTED);
					if ((nPlug) && (nPlug<=MAX_MIXPLUGINS)) {
						CVstPlugin *pPlug = (CVstPlugin*)m_SndFile.m_MixPlugins[nPlug-1].pMixPlugin;
						if (pPlug) 
							pPlug->GetParamName(nParam, paramName, sizeof(paramName));
						if (paramName[0] == 0)
							strcpy(paramName, "N/A");
					}
					if (paramName[0] == 0)
						strcpy(paramName, "N/A - no plug");
					CString temp; 
					temp.Format("param %d (%s)", nParam, paramName);
					chanSpec.Append(temp); 
					break; }
				case sfx_custom: 
				default: chanSpec.Append("Custom");
				}
			}
			if (chanSpec != "") {
				strcat(pszDescription, chanSpec);
			}

		}
		//end rewbs.xinfo
	}
	return bSupported;
}


LONG CModDoc::GetIndexFromEffect(UINT command, UINT param)
//--------------------------------------------------------
{
	UINT ndx = MAX_FXINFO;
	for (UINT i = 0; i < MAX_FXINFO; i++)
	{
		if ((command == gFXInfo[i].dwEffect) // Effect
		 && ((param & gFXInfo[i].dwParamMask) == gFXInfo[i].dwParamValue)) // Value
		{
			ndx = i;
			if((m_SndFile.m_nType & gFXInfo[i].dwFormats) != 0)
				break; // found fitting format; this is correct for sure
		}
	}
	return ndx;
}


//Returns command and corrects parameter refParam if necessary
UINT CModDoc::GetEffectFromIndex(UINT ndx, int &refParam)
//-------------------------------------------------------
{
	//if (pParam) *pParam = -1;
	if (ndx >= MAX_FXINFO) {
		refParam = 0;
		return 0;
	}

	//Cap parameter to match FX if necessary.
	if (gFXInfo[ndx].dwParamMask) {
		if (refParam < static_cast<int>(gFXInfo[ndx].dwParamValue)) {
			refParam = gFXInfo[ndx].dwParamValue;	 // for example: delay with param < D0 becomes SD0
		} else if (refParam > static_cast<int>(gFXInfo[ndx].dwParamValue)+15) {
			refParam = gFXInfo[ndx].dwParamValue+15; // for example: delay with param > DF becomes SDF
		}
	}
	if (gFXInfo[ndx].dwFlags) {
		if (refParam > static_cast<int>(gFXInfo[ndx].dwFlags)) {
			refParam = gFXInfo[ndx].dwFlags;	//used for Zxx macro control: limit to 7F max.
		}
	}

	return gFXInfo[ndx].dwEffect;
}


UINT CModDoc::GetEffectFromIndex(UINT ndx)
//----------------------------------------
{
	if (ndx >= MAX_FXINFO) {
		return 0;
	}

	return gFXInfo[ndx].dwEffect;
}

UINT CModDoc::GetEffectMaskFromIndex(UINT ndx)
//-------------------------------------------------------
{
	if (ndx >= MAX_FXINFO) {
		return 0;
	}

	return gFXInfo[ndx].dwParamValue;

}

bool CModDoc::GetEffectInfo(UINT ndx, LPSTR s, bool bXX, DWORD *prangeMin, DWORD *prangeMax)
//------------------------------------------------------------------------------------------
{
	if (s) s[0] = 0;
	if (prangeMin) *prangeMin = 0;
	if (prangeMax) *prangeMax = 0;
	if ((ndx >= MAX_FXINFO) || (!(m_SndFile.m_nType & gFXInfo[ndx].dwFormats))) return FALSE;
	if (s) GetEffectName(s, gFXInfo[ndx].dwEffect, gFXInfo[ndx].dwParamValue, bXX);
	if ((prangeMin) && (prangeMax))
	{
		UINT nmin = 0, nmax = 0xFF, nType = m_SndFile.m_nType;
		if (gFXInfo[ndx].dwParamMask == 0xF0)
		{
			nmin = gFXInfo[ndx].dwParamValue;
			nmax = nmin | 0x0F;
		}
		switch(gFXInfo[ndx].dwEffect)
		{
		case CMD_ARPEGGIO:
			if (nType & (MOD_TYPE_MOD|MOD_TYPE_XM)) nmin = 1;
			break;
		case CMD_VOLUME:
		case CMD_CHANNELVOLUME:
			nmax = 0x40;
			break;
		case CMD_SPEED:
			nmin = 1;
			nmax = 0x7F;
			if (nType & MOD_TYPE_MOD) nmax = 0x20; else
			if (nType & MOD_TYPE_XM) nmax = 0x1F;
			break;
		case CMD_TEMPO:
			nmin = 0x20;
			if (nType & MOD_TYPE_MOD) nmin = 0x21; else
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
//			if (nType & MOD_TYPE_S3MIT) nmin = 1;
			if (nType & MOD_TYPE_S3MITMPT) nmin = 0;
// -! NEW_FEATURE#0010
			break;
		case CMD_VOLUMESLIDE:
		case CMD_TONEPORTAVOL:
		case CMD_VIBRATOVOL:
		case CMD_GLOBALVOLSLIDE:
		case CMD_CHANNELVOLSLIDE:
		case CMD_PANNINGSLIDE:
			nmax = (nType & MOD_TYPE_S3MITMPT) ? 58 : 30;
			break;
		case CMD_PANNING8:
			if (nType & (MOD_TYPE_S3M)) nmax = 0x81;
			else nmax = 0xFF;
			break;
		case CMD_GLOBALVOLUME:
			nmax = (nType & (MOD_TYPE_IT|MOD_TYPE_MPT)) ? 128 : 64;
			break;

		case CMD_MODCMDEX:
			// adjust waveform types for XM/MOD
			if(gFXInfo[ndx].dwParamValue == 0x40 || gFXInfo[ndx].dwParamValue == 0x70) nmax = gFXInfo[ndx].dwParamValue | 0x07;
			break;
		case CMD_S3MCMDEX:
			// adjust waveform types for IT/S3M
			if(gFXInfo[ndx].dwParamValue >= 0x30 && gFXInfo[ndx].dwParamValue <= 0x50) nmax = gFXInfo[ndx].dwParamValue | (m_SndFile.IsCompatibleMode(TRK_IMPULSETRACKER | TRK_SCREAMTRACKER) ? 0x03 : 0x07);
			break;
		}
		*prangeMin = nmin;
		*prangeMax = nmax;
	}
	return TRUE;
}


UINT CModDoc::MapValueToPos(UINT ndx, UINT param)
//-----------------------------------------------
{
	UINT pos;
	
	if (ndx >= MAX_FXINFO) return 0;
	pos = param;
	if (gFXInfo[ndx].dwParamMask == 0xF0)
	{
		pos &= 0x0F;
		pos |= gFXInfo[ndx].dwParamValue;
	}
	switch(gFXInfo[ndx].dwEffect)
	{
	case CMD_VOLUMESLIDE:
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
	case CMD_GLOBALVOLSLIDE:
	case CMD_CHANNELVOLSLIDE:
	case CMD_PANNINGSLIDE:
		if (m_SndFile.GetType() & MOD_TYPE_S3MITMPT)
		{
			if (!param) pos = 29; else
			if (((param & 0x0F) == 0x0F) && (param & 0xF0))
				pos = 29 + (param >> 4); else
			if (((param & 0xF0) == 0xF0) && (param & 0x0F))
				pos = 29 - (param & 0x0F); else
			if (param & 0x0F)
				pos = 15 - (param & 0x0F);
			else
				pos = (param >> 4) + 44;
		} else
		{
			if (param & 0x0F) pos = 15 - (param & 0x0F);
			else pos = (param >> 4) + 15;
		}
		break;
	case CMD_PANNING8:
		if(m_SndFile.GetType() == MOD_TYPE_S3M)
		{
			pos = CLAMP(param, 0, 0x80);
			if(param == 0xA4)
				pos = 0x81;
		}
		break;
	}
	return pos;
}


UINT CModDoc::MapPosToValue(UINT ndx, UINT pos)
//---------------------------------------------
{
	UINT param;
	
	if (ndx >= MAX_FXINFO) return 0;
	param = pos;
	if (gFXInfo[ndx].dwParamMask == 0xF0) param |= gFXInfo[ndx].dwParamValue;
	switch(gFXInfo[ndx].dwEffect)
	{
	case CMD_VOLUMESLIDE:
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
	case CMD_GLOBALVOLSLIDE:
	case CMD_CHANNELVOLSLIDE:
	case CMD_PANNINGSLIDE:
		if (m_SndFile.GetType() & MOD_TYPE_S3MITMPT)
		{
			if (pos < 15) param = 15-pos; else
			if (pos < 29) param = (29-pos) | 0xF0; else
			if (pos == 29) param = 0; else
			if (pos < 44) param = ((pos - 29) << 4) | 0x0F; else
			if (pos < 59) param = (pos-43) << 4;
		} else
		{
			if (pos < 15) param = 15 - pos; else
			param = (pos - 15) << 4;
		}
		break;
	case CMD_PANNING8:
		if(m_SndFile.GetType() == MOD_TYPE_S3M)
			param = (pos <= 0x80) ? pos : 0xA4;
		break;
	}
	return param;
}


bool CModDoc::GetEffectNameEx(LPSTR pszName, UINT ndx, UINT param)
//----------------------------------------------------------------
{
	char s[64];
	char szContinueOrIgnore[16];

	if (pszName) pszName[0] = 0;
	if ((!pszName) || (ndx >= MAX_FXINFO) || (!gFXInfo[ndx].pszName)) return false;
	wsprintf(pszName, "%s: ", gFXInfo[ndx].pszName);
	s[0] = 0;

	// for effects that don't have effect memory in MOD format.
	if(m_SndFile.GetType() == MOD_TYPE_MOD)
		strcpy(szContinueOrIgnore, "ignore");
	else
		strcpy(szContinueOrIgnore, "continue");

	std::string sPlusChar = "+", sMinusChar = "-";

	switch(gFXInfo[ndx].dwEffect)
	{
	case CMD_ARPEGGIO:
		if(m_SndFile.GetType() == MOD_TYPE_XM)	// XM also ignores this!
			strcpy(szContinueOrIgnore, "ignore");

		if (param)
			wsprintf(s, "note+%d note+%d", param >> 4, param & 0x0F);
		else
			strcpy(s, szContinueOrIgnore);
		break;

	case CMD_PORTAMENTOUP:
	case CMD_PORTAMENTODOWN:
		if(param)
		{
			char sign[2];
			strcpy(sign, (gFXInfo[ndx].dwEffect == CMD_PORTAMENTOUP) ? "+" : "-");
			if((m_SndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xF0))
				wsprintf(s, "fine %s%d", sign, (param & 0x0F));
			else if((m_SndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xE0))
				wsprintf(s, "extra fine %s%d", sign, (param & 0x0F));
			else
				wsprintf(s, "%s%d", sign, param);
		}
		else
		{
			strcpy(s, szContinueOrIgnore);
		}
		break;

	case CMD_TONEPORTAMENTO:
		if (param)
			wsprintf(s, "speed %d", param);
		else
			strcpy(s, szContinueOrIgnore);
		break;

	case CMD_VIBRATO:
	case CMD_TREMOLO:
	case CMD_PANBRELLO:
	case CMD_FINEVIBRATO:
		if (param)
			wsprintf(s, "speed=%d depth=%d", param >> 4, param & 0x0F);
		else
			strcpy(s, szContinueOrIgnore);
		break;

	case CMD_SPEED:
		wsprintf(s, "%d frames/row", param);
		break;

	case CMD_TEMPO:
		if (param < 0x10)
			wsprintf(s, "-%d bpm (slower)", param & 0x0F);
		else if (param < 0x20)
			wsprintf(s, "+%d bpm (faster)", param & 0x0F);
		else
			wsprintf(s, "%d bpm", param);
		break;

	case CMD_PANNING8:
		wsprintf(s, "%d", param);
		if(m_SndFile.m_nType & MOD_TYPE_S3M)
		{
			if(param == 0xA4)
				strcpy(s, "Surround");
		}
		break;

	case CMD_RETRIG:
		switch(param >> 4) {
			case  0: strcpy(s, "continue"); break;
			case  1: strcpy(s, "vol -1"); break;
			case  2: strcpy(s, "vol -2"); break;
			case  3: strcpy(s, "vol -4"); break;
			case  4: strcpy(s, "vol -8"); break;
			case  5: strcpy(s, "vol -16"); break;
			case  6: strcpy(s, "vol *0.66"); break;
			case  7: strcpy(s, "vol *0.5"); break;
			case  8: strcpy(s, "vol *1"); break;
			case  9: strcpy(s, "vol +1"); break;
			case 10: strcpy(s, "vol +2"); break;
			case 11: strcpy(s, "vol +4"); break;
			case 12: strcpy(s, "vol +8"); break;
			case 13: strcpy(s, "vol +16"); break;
			case 14: strcpy(s, "vol *1.5"); break;
			case 15: strcpy(s, "vol *2"); break;
		}
		char spd[10];
		wsprintf(spd, " speed %d", param & 0x0F);
		strcat(s, spd);
		break;

	case CMD_VOLUMESLIDE:
	case CMD_TONEPORTAVOL:
	case CMD_VIBRATOVOL:
	case CMD_GLOBALVOLSLIDE:
	case CMD_CHANNELVOLSLIDE:
	case CMD_PANNINGSLIDE:
		if(gFXInfo[ndx].dwEffect == CMD_PANNINGSLIDE)
		{
			if(m_SndFile.GetType() == MOD_TYPE_XM)
			{
				sPlusChar = "-> ";
				sMinusChar = "<- ";
			}
			else
			{
				sPlusChar = "<- ";
				sMinusChar = "-> ";
			}
		}

		if (!param)
		{
			wsprintf(s, "continue");
		} else
		if ((m_SndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0x0F) == 0x0F) && (param & 0xF0))
		{
			wsprintf(s, "fine %s%d", sPlusChar.c_str(), param >> 4);
		} else
		if ((m_SndFile.GetType() & MOD_TYPE_S3MITMPT) && ((param & 0xF0) == 0xF0) && (param & 0x0F))
		{
			wsprintf(s, "fine %s%d", sMinusChar.c_str(), param & 0x0F);
		} else
		if ((param & 0x0F) != param && (param & 0xF0) != param)	// both nibbles are set.
		{
			strcpy(s, "undefined");
		} else
		if (param & 0x0F)
		{
			wsprintf(s, "%s%d", sMinusChar.c_str(), param & 0x0F);
		} else
		{
			wsprintf(s, "%s%d", sPlusChar.c_str(), param >> 4);
		}
		break;

	case CMD_PATTERNBREAK:
		wsprintf(pszName, "Break to row %d", param);
		break;

	case CMD_POSITIONJUMP:
		wsprintf(pszName, "Jump to position %d", param);
		break;

	case CMD_OFFSET:
		if (param)
// -> CODE#0010
// -> DESC="add extended parameter mechanism to pattern effects"
//			wsprintf(pszName, "Set Offset to %u", param << 8);
			wsprintf(pszName, "Set Offset to %u", param);
// -! NEW_FEATURE#0010
		else
			strcpy(s, "continue");
		break;

	case CMD_TREMOR:
		if(param)
		{
			BYTE ontime = (BYTE)(param >> 4), offtime = (BYTE)(param & 0x0F);
			if(m_SndFile.m_dwSongFlags & SONG_ITOLDEFFECTS)
			{
				ontime++;
				offtime++;
			}
			else
			{
				if(ontime == 0) ontime = 1;
				if(offtime == 0) offtime = 1;
			}
			wsprintf(pszName, "ontime %d, offtime %d", ontime, offtime);
		}
		else
		{
			strcpy(s, "continue");
		}
		break;

	case CMD_MIDI:
		if (param < 0x80)
		{
			wsprintf(pszName, "SFx macro: z=%02X (%d)", param, param);
		} else
		{
			wsprintf(pszName, "Fixed Macro Z%02X", param);
		}
		break;
	
	//rewbs.smoothVST
	case CMD_SMOOTHMIDI:
		if (param < 0x80)
		{
			wsprintf(pszName, "SFx macro: z=%02X (%d)", param, param);
		} else
		{
			wsprintf(pszName, "Fixed Macro Z%02X", param);
		}
		break;
	//end rewbs.smoothVST

	default:
		if (gFXInfo[ndx].dwParamMask == 0xF0)
		{
			// Sound control names
			if (((gFXInfo[ndx].dwEffect == CMD_XFINEPORTAUPDOWN) || (gFXInfo[ndx].dwEffect == CMD_S3MCMDEX))
			 && ((gFXInfo[ndx].dwParamValue & 0xF0) == 0x90) && ((param & 0xF0) == 0x90))
			{
				switch(param & 0x0F)
				{
				case 0x00:	strcpy(s, "90: Surround Off"); break;
				case 0x01:	strcpy(s, "91: Surround On"); break;
				case 0x08:	strcpy(s, "98: Reverb Off"); break;
				case 0x09:	strcpy(s, "99: Reverb On"); break;
				case 0x0A:	strcpy(s, "9A: Center surround"); break;
				case 0x0B:	strcpy(s, "9B: Quad surround"); break;
				case 0x0C:	strcpy(s, "9C: Global filters"); break;
				case 0x0D:	strcpy(s, "9D: Local filters"); break;
				case 0x0E:	strcpy(s, "9E: Play Forward"); break;
				case 0x0F:	strcpy(s, "9F: Play Backward"); break;
				default:	wsprintf(s, "%02X: undefined", param);
				}
			} else
			if (((gFXInfo[ndx].dwEffect == CMD_XFINEPORTAUPDOWN) || (gFXInfo[ndx].dwEffect == CMD_S3MCMDEX))
			 && ((gFXInfo[ndx].dwParamValue & 0xF0) == 0x70) && ((param & 0xF0) == 0x70))
			{
				switch(param & 0x0F)
				{
				case 0x00:	strcpy(s, "70: Past note cut"); break;
				case 0x01:	strcpy(s, "71: Past note off"); break;
				case 0x02:	strcpy(s, "72: Past note fade"); break;
				case 0x03:	strcpy(s, "73: NNA note cut"); break;
				case 0x04:	strcpy(s, "74: NNA continue"); break;
				case 0x05:	strcpy(s, "75: NNA note off"); break;
				case 0x06:	strcpy(s, "76: NNA note fade"); break;
				case 0x07:	strcpy(s, "77: Volume Env Off"); break;
				case 0x08:	strcpy(s, "78: Volume Env On"); break;
				case 0x09:	strcpy(s, "79: Pan Env Off"); break;
				case 0x0A:	strcpy(s, "7A: Pan Env On"); break;
				case 0x0B:	strcpy(s, "7B: Pitch Env Off"); break;
				case 0x0C:	strcpy(s, "7C: Pitch Env On"); break;
					// intentional fall-through follows
				case 0x0D:	if(m_SndFile.GetType() == MOD_TYPE_MPT) { strcpy(s, "7D: Force Pitch Env"); break; }
				case 0x0E:	if(m_SndFile.GetType() == MOD_TYPE_MPT) { strcpy(s, "7E: Force Filter Env"); break; }
				default:	wsprintf(s, "%02X: undefined", param); break;
				}
			} else
			{
				wsprintf(s, "%d", param & 0x0F);
				if(gFXInfo[ndx].dwEffect == CMD_S3MCMDEX)
				{
					switch(param & 0xF0)
					{
					case 0x10: // glissando control
						if((param & 0x0F) == 0)
							strcpy(s, "smooth");
						else
							strcpy(s, "semitones");
						break;					
					case 0x30: // vibrato waveform
					case 0x40: // tremolo waveform
					case 0x50: // panbrello waveform
						if(((param & 0x0F) > 0x03) && m_SndFile.IsCompatibleMode(TRK_IMPULSETRACKER))
						{
							strcpy(s, "ignore");
							break;
						}
						switch(param & 0x0F)
						{
							case 0x00: strcpy(s, "sine wave"); break;
							case 0x01: strcpy(s, "ramp down"); break;
							case 0x02: strcpy(s, "square wave"); break;
							case 0x03: strcpy(s, "random"); break;
							case 0x04: strcpy(s, "sine wave (cont.)"); break;
							case 0x05: strcpy(s, "ramp down (cont.)"); break;
							case 0x06: strcpy(s, "square wave (cont.)"); break;
							case 0x07: strcpy(s, "random (cont.)"); break;
							default: strcpy(s, "ignore"); break;
						}
						break;

					case 0x60: // fine pattern delay (ticks)
						strcat(s, " rows");
						break;

					case 0xA0: // high offset
						wsprintf(s, "+ %u samples", 0x10000 * (param & 0x0F));
						break;

					case 0xB0: // pattern loop
						if((param & 0x0F) == 0x00)
							strcpy(s, "loop start");
						else
							strcat(s, " times");
						break;
					case 0xC0: // note cut
					case 0xD0: // note delay
						//IT compatibility 22. SD0 == SD1, SC0 == SC1
						if(((param & 0x0F) == 1) || ((param & 0x0F) == 0 && m_SndFile.IsCompatibleMode(TRK_IMPULSETRACKER)))
							strcpy(s, "1 frame");
						else
							strcat(s, " frames");
						break;
					case 0xE0: // pattern delay (rows)
						strcat(s, " rows");
						break;
					case 0xF0: // macro
						if(m_SndFile.GetType() != MOD_TYPE_MOD)
							wsprintf(s, "SF%X", param & 0x0F);
						break;
					default:
						break;
					}
				}
				if(gFXInfo[ndx].dwEffect == CMD_MODCMDEX)
				{
					switch(param & 0xF0)
					{
					case 0x30: // glissando control
						if((param & 0x0F) == 0)
							strcpy(s, "smooth");
						else
							strcpy(s, "semitones");
						break;					
					case 0x40: // vibrato waveform
					case 0x70: // tremolo waveform
						switch(param & 0x0F)
						{
							case 0x00: case 0x08: strcpy(s, "sine wave"); break;
							case 0x01: case 0x09: strcpy(s, "ramp down"); break;
							case 0x02: case 0x0A: strcpy(s, "square wave"); break;
							case 0x03: case 0x0B: strcpy(s, "square wave"); break;

							case 0x04: case 0x0C: strcpy(s, "sine wave (cont.)"); break;
							case 0x05: case 0x0D: strcpy(s, "ramp down (cont.)"); break;
							case 0x06: case 0x0E: strcpy(s, "square wave (cont.)"); break;
							case 0x07: case 0x0F: strcpy(s, "square wave (cont.)"); break;
						}
						break;
					case 0x60: // pattern loop
						if((param & 0x0F) == 0x00)
							strcpy(s, "loop start");
						else
							strcat(s, " times");
						break;
					case 0x90: // retrigger
						wsprintf(s, "speed %d", param & 0x0F);
						break;
					case 0xC0: // note cut
					case 0xD0: // note delay
						strcat(s, " frames");
						break;
					case 0xE0: // pattern delay (rows)
						strcat(s, " rows");
						break;
					case 0xF0:
						if(m_SndFile.GetType() == MOD_TYPE_MOD) // invert loop
						{
							if((param & 0x0F) == 0)
								strcpy(s, "Stop");
							else
								wsprintf(s, "Speed %d", param & 0x0F); 
						}
						else // macro
						{
							wsprintf(s, "SF%X", param & 0x0F);
						}
						break;
					default:
						break;
					}
				}
			}
			
		} else
		{
			wsprintf(s, "%d", param);
		}
	}
	strcat(pszName, s);
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////
// Volume column effects description

typedef struct MPTVOLCMDINFO
{
	DWORD dwVolCmd;		// VOLCMD_XXXX
	DWORD dwFormats;	// MOD_TYPE_XXX combo
	LPCSTR pszName;		// ie "Set Volume"
} MPTVOLCMDINFO;

#define MAX_VOLINFO		15			//rewbs.volOff & rewbs.velocity: increased from 13


const MPTVOLCMDINFO gVolCmdInfo[MAX_VOLINFO] =
{
	{VOLCMD_VOLUME,			MOD_TYPE_NOMOD,		"v: Set Volume"},
	{VOLCMD_PANNING,		MOD_TYPE_NOMOD,		"p: Set Panning"},
	{VOLCMD_VOLSLIDEUP,		MOD_TYPE_XMITMPT,	"c: Volume slide up"},
	{VOLCMD_VOLSLIDEDOWN,	MOD_TYPE_XMITMPT,	"d: Volume slide down"},
	{VOLCMD_FINEVOLUP,		MOD_TYPE_XMITMPT,	"a: Fine volume up"},
	{VOLCMD_FINEVOLDOWN,	MOD_TYPE_XMITMPT,	"b: Fine volume down"},
	{VOLCMD_VIBRATOSPEED,	MOD_TYPE_XM,		"u: Vibrato speed"},
	{VOLCMD_VIBRATODEPTH,	MOD_TYPE_XMITMPT,	"h: Vibrato depth"},
	{VOLCMD_PANSLIDELEFT,	MOD_TYPE_XM,		"l: Pan slide left"},
	{VOLCMD_PANSLIDERIGHT,	MOD_TYPE_XM,		"r: Pan slide right"},
	{VOLCMD_TONEPORTAMENTO,	MOD_TYPE_XMITMPT,	"g: Tone portamento"},
	{VOLCMD_PORTAUP,		MOD_TYPE_ITMPT,		"f: Portamento up"},
	{VOLCMD_PORTADOWN,		MOD_TYPE_ITMPT,		"e: Portamento down"},
	{VOLCMD_VELOCITY,		MOD_TYPE_ITMPT,		":: velocity"},		//rewbs.velocity
	{VOLCMD_OFFSET,			MOD_TYPE_ITMPT,		"o: offset"},		//rewbs.volOff
};


UINT CModDoc::GetNumVolCmds() const
//---------------------------------
{
	return MAX_VOLINFO;
}


LONG CModDoc::GetIndexFromVolCmd(UINT volcmd)
//-------------------------------------------
{
	for (UINT i=0; i<MAX_VOLINFO; i++)
	{
		if (gVolCmdInfo[i].dwVolCmd == volcmd) return i;
	}
	return -1;
}


UINT CModDoc::GetVolCmdFromIndex(UINT ndx)
//----------------------------------------
{
	return (ndx < MAX_VOLINFO) ? gVolCmdInfo[ndx].dwVolCmd : 0;
}


BOOL CModDoc::GetVolCmdInfo(UINT ndx, LPSTR s, DWORD *prangeMin, DWORD *prangeMax)
//--------------------------------------------------------------------------------
{
	if (s) s[0] = 0;
	if (prangeMin) *prangeMin = 0;
	if (prangeMax) *prangeMax = 0;
	if (ndx >= MAX_VOLINFO) return FALSE;
	if (s) strcpy(s, gVolCmdInfo[ndx].pszName);
	if ((prangeMin) && (prangeMax))
	{
		switch(gVolCmdInfo[ndx].dwVolCmd)
		{
		case VOLCMD_VOLUME:
			*prangeMax = 64;
			break;

		case VOLCMD_PANNING:
			*prangeMax = 64;
			if (m_SndFile.m_nType & MOD_TYPE_XM)
			{
				*prangeMin = 2;		// 0*4+2
				*prangeMax = 62;	// 15*4+2
			}
			break;

		default:
			*prangeMax = (m_SndFile.m_nType & MOD_TYPE_XM) ? 15 : 9;
		}
	}
	return (gVolCmdInfo[ndx].dwFormats & m_SndFile.m_nType) ? TRUE : FALSE;
}


//rewbs.customKeys
void* CModDoc::GetChildFrame()
//----------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (!pMainFrm) return 0;
	CMDIChildWnd *pMDIActive = pMainFrm->MDIGetActive();
	if (pMDIActive)
	{
		CView *pView = pMDIActive->GetActiveView();
		if ((pView) && (pView->GetDocument() == this))
			return ((CChildFrame *)pMDIActive);
	}
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView *pView = GetNextView(pos);
		if ((pView) && (pView->GetDocument() == this))
			return (CChildFrame *)pView->GetParentFrame();
	}

	return NULL;
}

HWND CModDoc::GetEditPosition(ROWINDEX &row, PATTERNINDEX &pat, ORDERINDEX &ord)
//------------------------------------------------------------------------------
{
	HWND followSonghWnd;
	PATTERNVIEWSTATE *patternViewState;
	CChildFrame *pChildFrm = (CChildFrame *) GetChildFrame();
	CSoundFile *pSndFile = GetSoundFile();

	if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0) // dirty HACK
	{
		followSonghWnd = pChildFrm->GetHwndView();
		PATTERNVIEWSTATE patternViewState;
		pChildFrm->SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)(&patternViewState));
		
		pat = patternViewState.nPattern;
		row = patternViewState.nRow;
		ord = patternViewState.nOrder;	
	}
	else	//patern editor object does not exist (i.e. is not active)  - use saved state.
	{
		followSonghWnd = NULL;
		patternViewState = pChildFrm->GetPatternViewState();

		pat = patternViewState->nPattern;
		row = patternViewState->nRow;
		ord = patternViewState->nOrder;
	}
	//rewbs.fix3185: if position is invalid, go to start of song.
	if (ord >= m_SndFile.Order.size()) {
		ord = 0;
		pat = pSndFile->Order[ord];
	}
	if (pat >= m_SndFile.Patterns.Size()) {
		pat=0;
	}
	if (row >= pSndFile->PatternSize[pat]) {
		row=0;
	}
	//end rewbs.fix3185

	//ensure order correlates with pattern.
	if (pSndFile->Order[ord]!=pat) {
		ORDERINDEX tentativeOrder = pSndFile->FindOrder(pat);
		if (tentativeOrder != ORDERINDEX_INVALID) {	//ensure a valid order exists.
			ord = tentativeOrder;
		}
	}


	return followSonghWnd;

}

int CModDoc::GetMacroType(CString value)
//--------------------------------------
{
	if (value.Compare("")==0) return sfx_unused;
	if (value.Compare("F0F000z")==0) return sfx_cutoff;
	if (value.Compare("F0F001z")==0) return sfx_reso;
	if (value.Compare("F0F002z")==0) return sfx_mode;
	if (value.Compare("F0F003z")==0) return sfx_drywet;
	if (value.Compare("BK00z")>=0 && value.Compare("BKFFz")<=0 && value.GetLength()==5)
		return sfx_cc;
	if (value.Compare("F0F079z")>0 && value.Compare("F0F1G")<0 && value.GetLength()==7)
		return sfx_plug; 
	return sfx_custom; //custom/unknown
}

int CModDoc::MacroToPlugParam(CString macro)
//------------------------------------------
{
	int code=0;
	char* param = (char *) (LPCTSTR) macro;
	param +=4;
	if ((param[0] >= '0') && (param[0] <= '9')) code = (param[0] - '0') << 4; else
	if ((param[0] >= 'A') && (param[0] <= 'F')) code = (param[0] - 'A' + 0x0A) << 4;
	if ((param[1] >= '0') && (param[1] <= '9')) code += (param[1] - '0'); else
	if ((param[1] >= 'A') && (param[1] <= 'F')) code += (param[1] - 'A' + 0x0A);

	if (macro.GetLength()>=4 && macro.GetAt(3)=='0') {
		return (code-128);
	} else {
		return (code+128);
	}
}
int CModDoc::MacroToMidiCC(CString macro)
//---------------------------------------
{
	int code=0;
	char* param = (char *) (LPCTSTR) macro;
	param +=2;
	if ((param[0] >= '0') && (param[0] <= '9')) code = (param[0] - '0') << 4; else
	if ((param[0] >= 'A') && (param[0] <= 'F')) code = (param[0] - 'A' + 0x0A) << 4;
	if ((param[1] >= '0') && (param[1] <= '9')) code += (param[1] - '0'); else
	if ((param[1] >= 'A') && (param[1] <= 'F')) code += (param[1] - 'A' + 0x0A);

	return code;
}

int CModDoc::FindMacroForParam(long param)
//----------------------------------------
{
	for (int macro=0; macro<16; macro++) {	//what's the named_const for num macros?? :D
		CString macroString = &(GetSoundFile()->m_MidiCfg.szMidiSFXExt[macro*32]);
		if (GetMacroType(macroString) == sfx_plug &&  MacroToPlugParam(macroString) == param) {
			return macro;
		}
	}

	return -1;
}

// Retrieve Zxx (Z80-ZFF) type from current macro configuration
int CModDoc::GetZxxType(const CHAR (&szMidiZXXExt)[128 * 32])
//-----------------------------------------------------------
{
	// Compare with all possible preset patterns
	for(int i = 1; i <= 5; i++)
	{
		// Prepare pattern to compare
		CHAR szPatterns[128 * 32];
		CreateZxxFromType(szPatterns, i);

		bool bFound = true;
		for(int j = 0; j < 128; j++)
		{
			if(strncmp(&szPatterns[j * 32], &szMidiZXXExt[j * 32], 32))
				bFound = false;
		}
		if(bFound) return i;
	}
	return 0; // Type 0 - Custom setup
}

// Create Zxx (Z80 - ZFF) from one out of five presets
void CModDoc::CreateZxxFromType(CHAR (&szMidiZXXExt)[128 * 32], int iZxxType)
//---------------------------------------------------------------------------
{
	for(int i = 0; i < 128; i++)
	{
		switch(iZxxType)
		{
		case 1:
			// Type 1 - Z80 - Z8F controls resonance
			if (i < 16) wsprintf(&szMidiZXXExt[i * 32], "F0F001%02X", i * 8);
			else szMidiZXXExt[i * 32] = 0;
			break;

		case 2:
			// Type 2 - Z80 - ZFF controls resonance
			wsprintf(&szMidiZXXExt[i * 32], "F0F001%02X", i);
			break;

		case 3:
			// Type 3 - Z80 - ZFF controls cutoff
			wsprintf(&szMidiZXXExt[i * 32], "F0F000%02X", i);
			break;

		case 4:
			// Type 4 - Z80 - ZFF controls filter mode
			wsprintf(&szMidiZXXExt[i * 32], "F0F002%02X", i);
			break;

		case 5:
			// Type 5 - Z80 - Z9F controls resonance + filter mode
			if (i < 16) wsprintf(&szMidiZXXExt[i * 32], "F0F001%02X", i * 8);
			else if (i < 32) wsprintf(&szMidiZXXExt[i * 32], "F0F002%02X", (i - 16) * 8);
			else szMidiZXXExt[i * 32] = 0;
			break;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Playback


void CModDoc::OnPatternRestart()
//------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CChildFrame *pChildFrm = (CChildFrame *) GetChildFrame();

	if ((pMainFrm) && (pChildFrm))
	{
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play pattern command: set loop pattern checkbox to true.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 1);
		}
                   
		CSoundFile *pSndFile = GetSoundFile();

		ROWINDEX nRow;
		PATTERNINDEX nPat;
		ORDERINDEX nOrd;

		HWND followSonghWnd;

		followSonghWnd = GetEditPosition(nRow, nPat, nOrd);
		CModDoc *pModPlaying = pMainFrm->GetModPlaying();
		
		BEGIN_CRITICAL();

		// Cut instruments/samples
		for (UINT i=0; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].nPatternLoopCount = 0;
			pSndFile->Chn[i].nPatternLoop = 0;
			pSndFile->Chn[i].nFadeOutVol = 0;
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		if ((nOrd < m_SndFile.Order.size()) && (pSndFile->Order[nOrd] == nPat)) pSndFile->m_nCurrentPattern = pSndFile->m_nNextPattern = nOrd;
		pSndFile->m_dwSongFlags &= ~(SONG_PAUSED|SONG_STEP);
		pSndFile->LoopPattern(nPat);
		pSndFile->m_nNextRow = 0;
		pSndFile->ResetTotalTickCount();
		//rewbs.vstCompliance
		if (pModPlaying == this) {
			pSndFile->StopAllVsti();
		} else {
			pSndFile->ResumePlugins();
		}
		//end rewbs.vstCompliance
		END_CRITICAL();
		
		// set playback timer in the status bar
		SetElapsedTime(nOrd, nRow, true);

		if (pModPlaying != this)
		{
			pMainFrm->PlayMod(this, followSonghWnd, m_dwNotifyType|MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS); //rewbs.fix2977
		}
	}
	//SwitchToView();
}

void CModDoc::OnPatternPlay()
//---------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CChildFrame *pChildFrm = (CChildFrame *) GetChildFrame();

	if ((pMainFrm) && (pChildFrm))
	{
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play pattern command: set loop pattern checkbox to true.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 1);
		}
                   
		CSoundFile *pSndFile = GetSoundFile();

		ROWINDEX nRow;
		PATTERNINDEX nPat;
		ORDERINDEX nOrd;

		HWND followSonghWnd;

		followSonghWnd = GetEditPosition(nRow,nPat,nOrd);
		CModDoc *pModPlaying = pMainFrm->GetModPlaying();
	
		BEGIN_CRITICAL();
		// Cut instruments/samples
		for (UINT i=pSndFile->m_nChannels; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		if ((nOrd < m_SndFile.Order.size()) && (pSndFile->Order[nOrd] == nPat)) pSndFile->m_nCurrentPattern = pSndFile->m_nNextPattern = nOrd;
		pSndFile->m_dwSongFlags &= ~(SONG_PAUSED|SONG_STEP);
		pSndFile->LoopPattern(nPat);
		pSndFile->m_nNextRow = nRow;
		//rewbs.VSTCompliance		
		if (pModPlaying == this) {
			pSndFile->StopAllVsti();
		} else {
			pSndFile->ResumePlugins();
		}
		//end rewbs.VSTCompliance
		END_CRITICAL();

		// set playback timer in the status bar
		SetElapsedTime(nOrd, nRow, true);

		if (pModPlaying != this) {
			pMainFrm->PlayMod(this, followSonghWnd, m_dwNotifyType|MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);  //rewbs.fix2977
		}
	}
	//SwitchToView();

}

void CModDoc::OnPatternPlayNoLoop()
//---------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CChildFrame *pChildFrm = (CChildFrame *) GetChildFrame();

	if ((pMainFrm) && (pChildFrm))
	{
		if (strcmp("CViewPattern", pChildFrm->GetCurrentViewClassName()) == 0)
		{
			//User has sent play song command: set loop pattern checkbox to false.
			pChildFrm->SendViewMessage(VIEWMSG_PATTERNLOOP, 0);
		}
                   
		CSoundFile *pSndFile = GetSoundFile();

		ROWINDEX nRow;
		PATTERNINDEX nPat;
		ORDERINDEX nOrd;

		HWND followSonghWnd;

		followSonghWnd = GetEditPosition(nRow,nPat,nOrd);
		CModDoc *pModPlaying = pMainFrm->GetModPlaying();

		BEGIN_CRITICAL();
		// Cut instruments/samples
		for (UINT i=pSndFile->m_nChannels; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		pSndFile->m_dwSongFlags &= ~(SONG_PAUSED|SONG_STEP);
		pSndFile->SetCurrentOrder(nOrd);
		if (pSndFile->Order[nOrd] == nPat)
			pSndFile->DontLoopPattern(nPat, nRow);
		else
			pSndFile->LoopPattern(nPat);
		pSndFile->m_nNextRow = nRow;
		//end rewbs.VSTCompliance
		if (pModPlaying == this) {
			pSndFile->StopAllVsti();	
		} else {
			pSndFile->ResumePlugins();
		}
		//rewbs.VSTCompliance
		END_CRITICAL();

		// set playback timer in the status bar
		SetElapsedTime(nOrd, nRow, true);
		
		if (pModPlaying != this)	{
			pMainFrm->PlayMod(this, followSonghWnd, m_dwNotifyType|MPTNOTIFY_POSITION|MPTNOTIFY_VUMETERS);  //rewbs.fix2977
		}
	}
	//SwitchToView();
}

LRESULT CModDoc::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;

	switch(wParam)
	{
		case kcViewGeneral: OnEditGlobals(); break;
		case kcViewPattern: OnEditPatterns(); break;
		case kcViewSamples: OnEditSamples(); break;
		case kcViewInstruments: OnEditInstruments(); break;
		case kcViewComments: OnEditComments(); break;
		case kcViewGraph: OnEditGraph(); break; //rewbs.graph
		case kcViewSongProperties: SongProperties(); break;

		case kcFileSaveAsWave:	OnFileWaveConvert(); break;
		case kcFileSaveAsMP3:	OnFileMP3Convert(); break;
		case kcFileSaveMidi:	OnFileMidiConvert(); break;
		case kcFileExportCompat:  OnFileCompatibilitySave(); break;
		case kcEstimateSongLength: OnEstimateSongLength(); break;
		case kcApproxRealBPM:	OnApproximateBPM(); break;
		case kcFileSave:		DoSave(m_strPathName, 0); break;
		case kcFileSaveAs:		DoSave(NULL, 1); break;
		case kcFileClose:		SafeFileClose(); break;

		case kcPlayPatternFromCursor: OnPatternPlay(); break;
		case kcPlayPatternFromStart: OnPatternRestart(); break;
		case kcPlaySongFromCursor: OnPatternPlayNoLoop(); break;
		case kcPlaySongFromStart: OnPlayerPlayFromStart(); break;
		case kcPlayPauseSong: OnPlayerPlay(); break;
		case kcStopSong: OnPlayerStop(); break;
//		case kcPauseSong: OnPlayerPause(); break;
			

	}

	return wParam;
}
//end rewbs.customKeys

void CModDoc::TogglePluginEditor(UINT m_nCurrentPlugin)
{
	PSNDMIXPLUGIN pPlugin;

	pPlugin = &m_SndFile.m_MixPlugins[m_nCurrentPlugin];
	if (m_nCurrentPlugin<MAX_MIXPLUGINS && pPlugin && pPlugin->pMixPlugin)
	{
		CVstPlugin *pVstPlugin = (CVstPlugin *)pPlugin->pMixPlugin;
		pVstPlugin->ToggleEditor();
	}

	return;
}

void CModDoc::ChangeFileExtension(UINT nNewType)
//----------------------------------------------
{
	//Not making path if path is empty(case only(?) for new file)
	if(GetPathName().GetLength() > 0)
	{
		CHAR path[_MAX_PATH], drive[_MAX_PATH], fname[_MAX_FNAME], ext[_MAX_EXT];
		_splitpath(GetPathName(), drive, path, fname, ext);

		CString newPath = drive;
		newPath += path;

		//Catch case where we don't have a filename yet.
		if (fname[0] == 0) {
			newPath += GetTitle();
		} else {
			newPath += fname;
		}

		switch(nNewType)
		{
		case MOD_TYPE_XM:  newPath += ".xm"; break;
		case MOD_TYPE_IT:  m_SndFile.m_dwSongFlags & SONG_ITPROJECT ? newPath+=".itp" : newPath+=".it"; break;
		case MOD_TYPE_MPT: newPath += ".mptm"; break;
		case MOD_TYPE_S3M: newPath += ".s3m"; break;
		case MOD_TYPE_MOD: newPath += ".mod"; break;
		default: ASSERT(false);		
		}
	
		if(nNewType != MOD_TYPE_IT ||
			(nNewType == MOD_TYPE_IT &&
				(
					(!strcmp(ext, ".it") && (m_SndFile.m_dwSongFlags & SONG_ITPROJECT)) ||
					(!strcmp(ext, ".itp") && !(m_SndFile.m_dwSongFlags & SONG_ITPROJECT))
				)
			)
		  ) 
			m_ShowSavedialog = true;
			//Forcing savedialog to appear after extension change - otherwise
			//unnotified file overwriting may occur.

		SetPathName(newPath, FALSE);

		
		
	}

	UpdateAllViews(NULL, HINT_MODTYPE);
}



UINT CModDoc::FindAvailableChannel()
//-------------------------------------------
{
	// Search for available channel
	for (UINT j=m_SndFile.m_nChannels; j<MAX_CHANNELS; j++)	{
		MODCHANNEL *p = &m_SndFile.Chn[j];
		if (!p->nLength) {
			return j;
		}
	}

	// Not found: look for one that's stopped
	for (UINT j=m_SndFile.m_nChannels; j<MAX_CHANNELS; j++)	{
		MODCHANNEL *p = &m_SndFile.Chn[j];
		if (p->dwFlags & CHN_NOTEFADE) {
			return j;
		}
	}
	
	//Last resort: go for first virutal channel.
	return m_SndFile.m_nChannels;
}

void CModDoc::RecordParamChange(int plugSlot, long paramIndex) 
//------------------------------------------------------
{
	SendMessageToActiveViews(WM_MOD_RECORDPARAM, plugSlot, paramIndex);
}

void CModDoc::LearnMacro(int macroToSet, long paramToUse)
//-------------------------------------------------------
{
	if (macroToSet<0 || macroToSet>NMACROS) {
		return;
	}
	
	//if macro already exists for this param, alert user and return
	for (int checkMacro=0; checkMacro<NMACROS; checkMacro++)	{
		CString macroText = &(GetSoundFile()->m_MidiCfg.szMidiSFXExt[checkMacro*32]);
 		int macroType = GetMacroType(macroText);
		
		if (macroType==sfx_plug && MacroToPlugParam(macroText)==paramToUse) {
			CString message;
			message.Format("Param %d can already be controlled with macro %X", paramToUse, checkMacro);
			CMainFrame::GetMainFrame()->MessageBox(message, "Macro exists for this param",MB_ICONINFORMATION | MB_OK);
			return;
		}
	}

	//set new macro
	CHAR *pMacroToSet = &(GetSoundFile()->m_MidiCfg.szMidiSFXExt[macroToSet*32]);
	if (paramToUse<128) {
		wsprintf(pMacroToSet, "F0F0%Xz",paramToUse+128);
	} else if (paramToUse<384) {
		wsprintf(pMacroToSet, "F0F1%Xz",paramToUse-128);
	} else {
		CString message;
		message.Format("Param %d beyond controllable range.", paramToUse);
		::MessageBox(NULL,message, "Macro not assigned for this param",MB_ICONINFORMATION | MB_OK);
		return;
	}

	CString message;
	message.Format("Param %d can now be controlled with macro %X", paramToUse, macroToSet);
	::MessageBox(NULL,message, "Macro assigned for this param",MB_ICONINFORMATION | MB_OK);
	
	return;
}

void CModDoc::SongProperties()
//----------------------------
{
	CModTypeDlg dlg(GetSoundFile(), CMainFrame::GetMainFrame());
	if (dlg.DoModal() == IDOK)
	{	
		BOOL bShowLog = FALSE;
		ClearLog();
		if(dlg.m_nType)
		{
			if (!ChangeModType(dlg.m_nType)) return;
			bShowLog = TRUE;
		}
		
		UINT nNewChannels = CLAMP(dlg.m_nChannels, m_SndFile.GetModSpecifications().channelsMin, m_SndFile.GetModSpecifications().channelsMax);

		if (nNewChannels != GetSoundFile()->m_nChannels)
		{
			const bool showCancelInRemoveDlg = m_SndFile.GetModSpecifications().channelsMax >= m_SndFile.GetNumChannels();
			if(ChangeNumChannels(nNewChannels, showCancelInRemoveDlg)) bShowLog = TRUE;
		}

		if (bShowLog) ShowLog("Conversion Status", CMainFrame::GetMainFrame());
		SetModified();
	}
}


// Sets playback timer to playback time at given position. If 'bReset' is true,
// timer is reset if playback position timer is not enabled.
void CModDoc::SetElapsedTime(ORDERINDEX nOrd, ROWINDEX nRow, bool bReset)
//-----------------------------------------------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm == NULL)
		return;

	if(CMainFrame::m_dwPatternSetup & PATTERN_POSITIONAWARETIMER)
	{
		double dPatternPlaytime = max(0, m_SndFile.GetPlaybackTimeAt(nOrd, nRow));
		pMainFrm->SetElapsedTime((DWORD) (dPatternPlaytime * 1000));
	}
	else if(bReset)
		pMainFrm->ResetElapsedTime();
}


CString CModDoc::GetPatternViewInstrumentName(UINT nInstr,
											  bool bEmptyInsteadOfNoName /* = false*/,
											  bool bIncludeIndex /* = true*/) const
//-----------------------------------------------------------------------------------
{
	if(nInstr >= MAX_INSTRUMENTS || m_SndFile.GetNumInstruments() == 0 || m_SndFile.Instruments[nInstr] == nullptr)
		return TEXT("");

	CString displayName, instrumentName, pluginName;
					
	// Get instrument name.
	instrumentName = m_SndFile.GetInstrumentName(nInstr);

	// If instrument name is empty, use name of the sample mapped to C-5.
	if (instrumentName.IsEmpty())
	{
		const SAMPLEINDEX nSmp = m_SndFile.Instruments[nInstr]->Keyboard[60];
		if (nSmp < ARRAYELEMCOUNT(m_SndFile.Samples) && m_SndFile.Samples[nSmp].pSample)
			instrumentName.Format(TEXT("s: %s"), m_SndFile.GetSampleName(nSmp)); //60 is C-5
	}

	// Get plugin name.
	const PLUGINDEX nPlug = m_SndFile.Instruments[nInstr]->nMixPlug;
	if (nPlug > 0 && nPlug < MAX_MIXPLUGINS)
		pluginName = m_SndFile.m_MixPlugins[nPlug-1].GetName();

	if (pluginName.IsEmpty())
	{
		if(bEmptyInsteadOfNoName && instrumentName.IsEmpty())
			return TEXT("");
		if(instrumentName.IsEmpty())
			instrumentName = TEXT("(no name)");
		if (bIncludeIndex)
			displayName.Format(TEXT("%02d: %s"), nInstr, (LPCTSTR)instrumentName);
		else
			displayName.Format(TEXT("%s"), (LPCTSTR)instrumentName);
	} else
	{
		if (bIncludeIndex)
			displayName.Format(TEXT("%02d: %s (%s)"), nInstr, (LPCTSTR)instrumentName, (LPCTSTR)pluginName);
		else
			displayName.Format(TEXT("%s (%s)"), (LPCTSTR)instrumentName, (LPCTSTR)pluginName);
	}
	return displayName;
}

void CModDoc::SafeFileClose()
//--------------------------
{
	// Verify that the main window has the focus. This saves us a lot of trouble because active dialogs normally don't check if their pSndFile pointers are still valid.
	if(GetActiveWindow() == CMainFrame::GetMainFrame()->m_hWnd)
		OnFileClose();
}