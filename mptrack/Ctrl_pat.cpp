#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "dlg_misc.h"
#include "ctrl_pat.h"
#include "view_pat.h"
#include ".\ctrl_pat.h"


//////////////////////////////////////////////////////////////
// Load/Save pattern settings in the registry

static UINT gnPatternSpacing = 0;
static BOOL gbPatternVUMeters = FALSE;
static BOOL gbPatternPluginNames = TRUE;	//rewbs.patPlugNames

void MPT_LoadPatternState(LPCSTR pszSection)
//------------------------------------------
{
	gnPatternSpacing = theApp.GetProfileInt(pszSection, "Spacing", 0);
	gbPatternVUMeters = theApp.GetProfileInt(pszSection, "VU-Meters", 0);
	gbPatternPluginNames = theApp.GetProfileInt(pszSection, "Plugin-Names", 1);	//rewbs.patPlugNames
}

void MPT_SavePatternState(LPCSTR pszSection)
//------------------------------------------
{
	theApp.WriteProfileInt(pszSection, "Spacing", gnPatternSpacing);
	theApp.WriteProfileInt(pszSection, "VU-Meters", gbPatternVUMeters);
	theApp.WriteProfileInt(pszSection, "Plugin-Names", gbPatternPluginNames);	//rewbs.patPlugNames
}

//////////////////////////////////////////////////////////////
// CCtrlPatterns

BEGIN_MESSAGE_MAP(CCtrlPatterns, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlPatterns)
	ON_WM_KEYDOWN()
	ON_WM_VSCROLL()
	ON_COMMAND(IDC_BUTTON1,					OnSequenceNext)
	ON_COMMAND(IDC_BUTTON2,					OnSequencePrev)
	ON_COMMAND(ID_PLAYER_PAUSE,				OnPlayerPause)
	ON_COMMAND(IDC_PATTERN_NEW,				OnPatternNew)
	ON_COMMAND(IDC_PATTERN_STOP,			OnPatternStop)
	ON_COMMAND(IDC_PATTERN_PLAY,			OnPatternPlay)
	ON_COMMAND(IDC_PATTERN_PLAYFROMSTART,	OnPatternPlayFromStart)
	ON_COMMAND(IDC_PATTERN_RECORD,			OnPatternRecord)
	ON_COMMAND(ID_PATTERN_PLAYROW,			OnPatternPlayRow)
// -> CODE#0015
// -> DESC="channels management dlg"
	ON_COMMAND(ID_PATTERN_CHANNELMANAGER,	OnChannelManager)
// -! NEW_FEATURE#0015
	ON_COMMAND(ID_PATTERN_VUMETERS,			OnPatternVUMeters)
	ON_COMMAND(ID_VIEWPLUGNAMES,			OnPatternViewPlugNames)	//rewbs.patPlugNames
	ON_COMMAND(ID_NEXTINSTRUMENT,			OnNextInstrument)
	ON_COMMAND(ID_PREVINSTRUMENT,			OnPrevInstrument)
	ON_COMMAND(ID_CONTROLTAB,				OnSwitchToView)
	ON_COMMAND(IDC_PATTERN_FOLLOWSONG,		OnFollowSong)
	ON_COMMAND(ID_PATTERN_MIDIMACRO,		OnSetupZxxMacros)
	ON_COMMAND(ID_PATTERN_CHORDEDIT,		OnChordEditor)
	ON_COMMAND(ID_PATTERN_PROPERTIES,		OnPatternProperties)
	ON_COMMAND(ID_PATTERN_EXPAND,			OnPatternExpand)
	ON_COMMAND(ID_PATTERN_SHRINK,			OnPatternShrink)
	ON_COMMAND(ID_PATTERN_AMPLIFY,			OnPatternAmplify)
	ON_COMMAND(ID_ORDERLIST_NEW,			OnPatternNew)
	ON_COMMAND(ID_ORDERLIST_COPY,			OnPatternDuplicate)
	ON_COMMAND(ID_PATTERNCOPY,				OnPatternCopy)
	ON_COMMAND(ID_PATTERNPASTE,				OnPatternPaste)
	ON_COMMAND(ID_EDIT_UNDO,				OnEditUndo)
	ON_COMMAND(ID_PATTERNDETAIL_LO,			OnDetailLo)
	ON_COMMAND(ID_PATTERNDETAIL_MED,		OnDetailMed)
	ON_COMMAND(ID_PATTERNDETAIL_HI,			OnDetailHi)
	ON_CBN_SELCHANGE(IDC_COMBO_INSTRUMENT,	OnInstrumentChanged)
// -> CODE#0012
// -> DESC="midi keyboard split"
	ON_CBN_SELCHANGE(IDC_COMBO_SPLITINSTRUMENT,	OnSplitInstrumentChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLITNOTE,		OnSplitNoteChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_OCTAVEMODIFIER,	OnOctaveModifierChanged)
	ON_COMMAND(IDC_PATTERN_OCTAVELINK,			OnOctaveLink)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLITVOLUME,		OnSplitVolumeChanged)
// -! NEW_FEATURE#0012
	ON_COMMAND(IDC_PATINSTROPLUGGUI,		TogglePluginEditor) //rewbs.instroVST
	ON_COMMAND(IDC_PATINSTROPLUGGUI2,		ToggleSplitPluginEditor) //rewbs.instroVST
	ON_EN_CHANGE(IDC_EDIT_SPACING,			OnSpacingChanged)
	ON_EN_CHANGE(IDC_EDIT_PATTERNNAME,		OnPatternNameChanged)
	ON_UPDATE_COMMAND_UI(IDC_PATTERN_RECORD,OnUpdateRecord)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCtrlPatterns::DoDataExchange(CDataExchange* pDX)
//----------------------------------------------------
{
	CModControlDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCtrlPatterns)
	DDX_Control(pDX, IDC_BUTTON1,				m_BtnNext);
	DDX_Control(pDX, IDC_BUTTON2,				m_BtnPrev);
	DDX_Control(pDX, IDC_COMBO_INSTRUMENT,		m_CbnInstrument);
// -> CODE#0012
// -> DESC="midi keyboard split"
    DDX_Control(pDX, IDC_COMBO_SPLITINSTRUMENT,	m_CbnSplitInstrument);
	DDX_Control(pDX, IDC_COMBO_SPLITNOTE,		m_CbnSplitNote);
	DDX_Control(pDX, IDC_COMBO_OCTAVEMODIFIER,	m_CbnOctaveModifier);
	DDX_Control(pDX, IDC_COMBO_SPLITVOLUME,		m_CbnSplitVolume);
	DDX_Control(pDX, IDC_EDIT_SPACING,			m_EditSpacing);
	DDX_Control(pDX, IDC_EDIT_PATTERNNAME,		m_EditPatName);
	DDX_Control(pDX, IDC_SPIN_SPACING,			m_SpinSpacing);
	DDX_Control(pDX, IDC_SPIN_INSTRUMENT,		m_SpinInstrument);
	DDX_Control(pDX, IDC_TOOLBAR1,				m_ToolBar);
	//}}AFX_DATA_MAP
}


CCtrlPatterns::CCtrlPatterns()
//----------------------------
{
	m_nInstrument = 0;
	m_bRecord = TRUE;
	m_bVUMeters = gbPatternVUMeters;
	m_bPluginNames = gbPatternPluginNames;	 	//rewbs.patPlugNames
	m_nDetailLevel = 4;
}


BOOL CCtrlPatterns::OnInitDialog()
//--------------------------------
{
	CRect rect, rcOrderList;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModControlDlg::OnInitDialog();

	if ((!m_pModDoc) || (!m_pSndFile) || (!pMainFrm)) return TRUE;
	LockControls();
	// Order List
	m_BtnNext.GetWindowRect(&rect);
	ScreenToClient(&rect);
	rcOrderList.left = rect.right + 4;
	rcOrderList.top = rect.top;
	rcOrderList.bottom = rect.bottom + GetSystemMetrics(SM_CYHSCROLL);
	GetClientRect(&rect);
	rcOrderList.right = rect.right - 4;
	m_OrderList.Init(rcOrderList, this, m_pModDoc, pMainFrm->GetGUIFont());
	// Toolbar buttons
	m_ToolBar.Init();
	m_ToolBar.AddButton(IDC_PATTERN_NEW, 0);
	m_ToolBar.AddButton(IDC_PATTERN_PLAY, 2);
	m_ToolBar.AddButton(IDC_PATTERN_PLAYFROMSTART, 3);
	m_ToolBar.AddButton(IDC_PATTERN_STOP, 1);
	m_ToolBar.AddButton(ID_PATTERN_PLAYROW, 28);
	m_ToolBar.AddButton(IDC_PATTERN_RECORD, 4, TBSTYLE_CHECK, ((m_bRecord) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_PATTERN_VUMETERS, 16, TBSTYLE_CHECK, ((m_bVUMeters) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_VIEWPLUGNAMES, 33, TBSTYLE_CHECK, ((m_bPluginNames) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED); //rewbs.patPlugNames
// -> CODE#0015
// -> DESC="channels management dlg"
	m_ToolBar.AddButton(ID_PATTERN_CHANNELMANAGER, 34);
// -! NEW_FEATURE#0015
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_PATTERN_MIDIMACRO, 17);
	m_ToolBar.AddButton(ID_PATTERN_CHORDEDIT, 18);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_EDIT_UNDO, 26);
	m_ToolBar.AddButton(ID_PATTERN_PROPERTIES, 19);
	m_ToolBar.AddButton(ID_PATTERN_EXPAND, 20);
	m_ToolBar.AddButton(ID_PATTERN_SHRINK, 21);
//	m_ToolBar.AddButton(ID_PATTERN_AMPLIFY, 9);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_PATTERNDETAIL_LO, 30, TBSTYLE_CHECK, TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_PATTERNDETAIL_MED, 31, TBSTYLE_CHECK, TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_PATTERNDETAIL_HI, 32, TBSTYLE_CHECK, TBSTATE_ENABLED|TBSTATE_CHECKED);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);

	// Special edit controls -> tab switch to view
	m_EditSpacing.SetParent(this);
	m_EditPatName.SetParent(this);
	// Spin controls
	m_SpinSpacing.SetRange(0, 16);
	m_SpinInstrument.SetRange(-1, 1);
	m_SpinInstrument.SetPos(0);
	m_SpinSpacing.SetPos(gnPatternSpacing);
	m_EditPatName.SetLimitText(MAX_PATTERNNAME);
	SetDlgItemInt(IDC_EDIT_SPACING, gnPatternSpacing);
	CheckDlgButton(IDC_PATTERN_FOLLOWSONG, !(CMainFrame::m_dwPatternSetup & PATTERN_FOLLOWSONGOFF));		//rewbs.noFollow - set to unchecked
	m_OrderList.SetFocus(); 

	UpdateView(HINT_MODTYPE|HINT_PATNAMES, NULL);
	RecalcLayout();

// -> CODE#0012
// -> DESC="midi keyboard split"
	//rewbs.merge: fix buffer overrun:
	//CHAR s[8];
	CHAR s[10];
	
	for(int i = 0 ; i < 120 ; i++){
		wsprintf(s, "%s%d", szNoteNames[i % 12], i/12);
		int n = m_CbnSplitNote.AddString(s);
		m_CbnSplitNote.SetItemData(n, i);
	}
	m_nSplitInstrument = 0;
	m_nSplitNote = 0;
	m_CbnSplitNote.SetCurSel(m_nSplitNote);

	
	for(int i = -9 ; i < 10 ; i++){
		wsprintf(s,i < 0 ? "Oct.  - %d" : i > 0 ? "Oct. + %d" : "Oct. + 0", abs(i));
		int n = m_CbnOctaveModifier.AddString(s);
		m_CbnOctaveModifier.SetItemData(n, i);
	}
	
	m_nOctaveModifier = 9;
	m_CbnOctaveModifier.SetCurSel(m_nOctaveModifier);
	m_nOctaveLink = TRUE;
	CheckDlgButton(IDC_PATTERN_OCTAVELINK, m_nOctaveLink ? MF_CHECKED : MF_UNCHECKED);

	m_CbnSplitVolume.AddString("--");
	m_CbnSplitVolume.SetItemData(0, 0);
	for(int i = 1; i<65 ; i++){
		wsprintf(s,"%d",i);
		int n = m_CbnSplitVolume.AddString(s);
		m_CbnSplitVolume.SetItemData(n, i);
	}

	m_nSplitVolume = 0;
	m_CbnSplitVolume.SetCurSel(m_nSplitVolume);



// -! NEW_FEATURE#0012

	m_bInitialized = TRUE;
	UnlockControls();
	return FALSE;
}


void CCtrlPatterns::RecalcLayout()
//--------------------------------
{
	// Update Order List Position
	if (m_OrderList.m_hWnd)
	{
		CRect rect;
		int cx, cy, cellcx;

		m_BtnNext.GetWindowRect(&rect);
		ScreenToClient(&rect);
		cx = - (rect.right + 4);
		cy = rect.bottom - rect.top + GetSystemMetrics(SM_CYHSCROLL);
		GetClientRect(&rect);
		cx += rect.right - 8;
		cellcx = m_OrderList.GetFontWidth();
		if (cellcx > 0) cx -= (cx % cellcx);
		cx += 2;
		if ((cx > 0) && (cy > 0))
		{
			m_OrderList.SetWindowPos(NULL, 0,0, cx, cy, SWP_NOMOVE|SWP_NOZORDER|SWP_DRAWFRAME);
		}
	}
}


void CCtrlPatterns::UpdateView(DWORD dwHintMask, CObject *pObj)
//-------------------------------------------------------------
{
	CHAR s[256];
	m_OrderList.UpdateView(dwHintMask, pObj);
	if (!m_pSndFile) return;

	//rewbs.instroVST
	if (dwHintMask & (HINT_MIXPLUGINS|HINT_MODTYPE))
	{
		if (HasValidPlug(m_nInstrument))
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), true);
		else
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), false);
		if (HasValidPlug(m_nSplitInstrument))
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI2), true);
		else
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI2), false);
	}
	//end rewbs.instroVST
	if (dwHintMask & HINT_MPTOPTIONS)
	{
		m_ToolBar.UpdateStyle();
// -> CODE#0007
// -> DESC="uncheck follow song checkbox by default"
		CheckDlgButton(IDC_PATTERN_FOLLOWSONG, (CMainFrame::m_dwPatternSetup & PATTERN_FOLLOWSONGOFF) ? MF_UNCHECKED : MF_CHECKED);
// -! BEHAVIOUR_CHANGE#0007
	}
	if (dwHintMask & (HINT_MODTYPE|HINT_INSNAMES|HINT_SMPNAMES|HINT_PATNAMES))
	{
		LockControls();
		if (dwHintMask & (HINT_MODTYPE|HINT_INSNAMES|HINT_SMPNAMES))
		{
			UINT nPos = 0;
			m_CbnInstrument.SetRedraw(FALSE);
			m_CbnInstrument.ResetContent();
			m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(" None"), 0);
// -> CODE#0012
// -> DESC="midi keyboard split"
			m_CbnSplitInstrument.SetRedraw(FALSE);
			m_CbnSplitInstrument.ResetContent();
			m_CbnSplitInstrument.SetItemData(m_CbnSplitInstrument.AddString(" None"), 0);
// -! NEW_FEATURE#0012
			if (m_pSndFile->m_nInstruments)
			{
				for (UINT i=1; i<=m_pSndFile->m_nInstruments; i++) if (m_pSndFile->Headers[i])
				{
					wsprintf(s, "%02d: %s", i, (m_pSndFile->Headers[i]) ? m_pSndFile->Headers[i]->name : "");
					UINT n = m_CbnInstrument.AddString(s);
					if (n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
// -> CODE#0012
// -> DESC="midi keyboard split"
					m_CbnSplitInstrument.AddString(s);
					m_CbnSplitInstrument.SetItemData(n, i);
// -! NEW_FEATURE#0012
				}
			} else
			{
				UINT nmax = m_pSndFile->m_nSamples;
				while ((nmax > 1) && (m_pSndFile->Ins[nmax].pSample == NULL) && (!m_pSndFile->m_szNames[nmax][0])) nmax--;
				for (UINT i=1; i<=nmax; i++) if ((m_pSndFile->m_szNames[i][0]) || (m_pSndFile->Ins[i].pSample))
				{
					wsprintf(s, "%02d: %s", i, m_pSndFile->m_szNames[i]);
					UINT n = m_CbnInstrument.AddString(s);
					if (n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
// -> CODE#0012
// -> DESC="midi keyboard split"
					m_CbnSplitInstrument.AddString(s);
					m_CbnSplitInstrument.SetItemData(n, i);
// -! NEW_FEATURE#0012
				}
			}
			m_CbnInstrument.SetCurSel(nPos);
			m_CbnInstrument.SetRedraw(TRUE);
// -> CODE#0012
// -> DESC="midi keyboard split"
			m_CbnSplitInstrument.SetCurSel(nPos);
			m_CbnSplitInstrument.SetRedraw(TRUE);

		
// -! NEW_FEATURE#0012
		}
		if (dwHintMask & (HINT_MODTYPE|HINT_PATNAMES))
		{
			UINT nPat = SendViewMessage(VIEWMSG_GETCURRENTPATTERN);
			m_pSndFile->GetPatternName(nPat, s, sizeof(s));
			m_EditPatName.SetWindowText(s);
			BOOL bXMIT = (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) ? TRUE : FALSE;
			m_ToolBar.EnableButton(ID_PATTERN_MIDIMACRO, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_PROPERTIES, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_EXPAND, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_SHRINK, bXMIT);
		}
		UnlockControls();
	}
	if (dwHintMask & (HINT_MODTYPE|HINT_UNDO))
	{
		m_ToolBar.EnableButton(ID_EDIT_UNDO, m_pModDoc->CanUndo());
	}
}


CRuntimeClass *CCtrlPatterns::GetAssociatedViewClass()
//----------------------------------------------------
{
	return RUNTIME_CLASS(CViewPattern);
}


LRESULT CCtrlPatterns::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
//---------------------------------------------------------------
{
	switch(wParam)
	{
	case CTRLMSG_GETCURRENTINSTRUMENT:
		return m_nInstrument;

	case CTRLMSG_GETCURRENTPATTERN:
		return m_OrderList.GetCurrentPattern();

	case CTRLMSG_PATTERNCHANGED:
		UpdateView((lParam << 24) | HINT_PATNAMES, NULL);
		break;

	case CTRLMSG_PAT_PREVINSTRUMENT:
		OnPrevInstrument();
		break;

	case CTRLMSG_PAT_NEXTINSTRUMENT:
		OnNextInstrument();
		break;

	case CTRLMSG_SETCURRENTPATTERN:
		SetCurrentPattern(lParam);
		break;

	case CTRLMSG_SETCURRENTORDER:
		m_OrderList.SetCurSel(lParam, FALSE);
		break;

	case CTRLMSG_GETCURRENTORDER:
		return m_OrderList.GetCurSel();

	case CTRLMSG_SETCURRENTINSTRUMENT:
	case CTRLMSG_PAT_SETINSTRUMENT:
		return SetCurrentInstrument(lParam);

	case CTRLMSG_PLAYPATTERN:
		switch(lParam)
		{
			case -2: OnPatternPlayNoLoop();	break;		//rewbs.playSongFromCursor
			case -1: OnPatternPlayFromStart();	break;
			default: OnPatternPlay();
		}

	case CTRLMSG_SETVIEWWND:
		SendViewMessage(VIEWMSG_FOLLOWSONG, IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG));
		OnSpacingChanged();
		
		//rewbs.merge
		SendViewMessage(VIEWMSG_SETSPLITINSTRUMENT, m_nSplitInstrument);
		SendViewMessage(VIEWMSG_SETSPLITNOTE, m_nSplitNote);
		SendViewMessage(VIEWMSG_SETOCTAVEMODIFIER, m_nOctaveModifier);
		SendViewMessage(VIEWMSG_SETOCTAVELINK, m_nOctaveLink);
		SendViewMessage(VIEWMSG_SETSPLITVOLUME, m_nSplitVolume);	
		//end rewbs.merge

		SendViewMessage(VIEWMSG_SETDETAIL, m_nDetailLevel);
		SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
		SendViewMessage(VIEWMSG_SETPLUGINNAMES, m_bPluginNames); //rewbs.patPlugNames
		break;

	case CTRLMSG_GETSPACING:
		return GetDlgItemInt(IDC_EDIT_SPACING);

	case CTRLMSG_SETSPACING:
		SetDlgItemInt(IDC_EDIT_SPACING, lParam);
		break;

	case CTRLMSG_ISRECORDING:
		return m_bRecord;

	case CTRLMSG_SETFOCUS:
		if (m_pParent) GetParentFrame()->SetActiveView(m_pParent);
		m_OrderList.SetFocus();
		break;

	case CTRLMSG_SETRECORD:
		if (lParam >= 0) m_bRecord = (BOOL)(lParam); else m_bRecord = !m_bRecord;
		m_ToolBar.SetState(IDC_PATTERN_RECORD, ((m_bRecord) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		break;

	case CTRLMSG_PREVORDER:
		m_OrderList.SetCurSel(m_OrderList.GetCurSel()-1, TRUE);
		break;
	
	case CTRLMSG_NEXTORDER:
		m_OrderList.SetCurSel(m_OrderList.GetCurSel()+1, TRUE);
		break;

	//rewbs.followSongCustomKey
	case CTRLMSG_PAT_FOLLOWSONG:
		CheckDlgButton(IDC_PATTERN_FOLLOWSONG, !IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG));
		OnFollowSong();
		break;
	//end rewbs.followSongCustomKey


	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


void CCtrlPatterns::SetCurrentPattern(UINT nPat)
//----------------------------------------------
{
	PostViewMessage(VIEWMSG_SETCURRENTPATTERN, nPat);
}


BOOL CCtrlPatterns::SetCurrentInstrument(UINT nIns)
//-------------------------------------------------
{
	if (nIns == m_nInstrument) return TRUE;
	if (m_pSndFile)
	{
		int n = m_CbnInstrument.GetCount();
		if (nIns > (UINT)n) return FALSE;
		for (int i=0; i<n; i++)
		{
			if (m_CbnInstrument.GetItemData(i) == nIns)
			{
				m_CbnInstrument.SetCurSel(i);
				m_nInstrument = nIns;
				//rewbs.instroVST
				if (HasValidPlug(m_nInstrument))
					::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), true);
				else
					::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), false);
				//end rewbs.instroVST
				return TRUE;
			}
		}
	}
	return FALSE;
}


////////////////////////////////////////////////////////////
// CCtrlPatterns messages

void CCtrlPatterns::OnActivatePage(LPARAM lParam)
//-----------------------------------------------
{
	CModDoc *pModDoc = GetDocument();

	if ((pModDoc) && (m_pParent))
	{
		int nIns = m_pParent->GetInstrumentChange();
		if (nIns > 0)
		{
			SetCurrentInstrument(nIns);
		}
		m_pParent->InstrumentChanged(-1);
	}
	if ((lParam >= 0) && (lParam < MAX_PATTERNS))
	{
		if (pModDoc)
		{
			CSoundFile *pSndFile = pModDoc->GetSoundFile();
			for (UINT i=0; i<MAX_ORDERS; i++)
			{
				if (pSndFile->Order[i] == (UINT)lParam)
				{
					m_OrderList.SetCurSel(i, TRUE);
					break;
				}
				if (pSndFile->Order[i] == 0xFF) break;
			}
		}
		SetCurrentPattern(lParam);
	} else
	if ((lParam >= 0x8000) && (lParam < MAX_ORDERS + 0x8000))
	{
		if (pModDoc)
		{
			CSoundFile *pSndFile = pModDoc->GetSoundFile();
			lParam &= 0x7FFF;
			m_OrderList.SetCurSel(lParam);
			SetCurrentPattern(pSndFile->Order[lParam]);
		}
	}
	if (m_hWndView)
	{
		OnSpacingChanged();
		if (m_bRecord) SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
		if (pFrame) PostViewMessage(VIEWMSG_LOADSTATE, (LPARAM)pFrame->GetPatternViewState());
		SwitchToView();
	}
}


void CCtrlPatterns::OnDeactivatePage()
//------------------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)pFrame->GetPatternViewState());
}


void CCtrlPatterns::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//----------------------------------------------------------------------------
{
	CModControlDlg::OnVScroll(nSBCode, nPos, pScrollBar);
	short int pos = (short int)m_SpinInstrument.GetPos();
	if (pos)
	{
		m_SpinInstrument.SetPos(0);
		int nmax = m_CbnInstrument.GetCount();
		int nins = m_CbnInstrument.GetCurSel() - pos;
		if (nins < 0) nins = nmax-1;
		if (nins >= nmax) nins = 0;
		m_CbnInstrument.SetCurSel(nins);
		OnInstrumentChanged();
	}
	if ((nSBCode == SB_ENDSCROLL) && (m_hWndView))
	{
		SwitchToView();
	}
}


void CCtrlPatterns::OnSequencePrev()
//----------------------------------
{
	m_OrderList.SetCurSel(m_OrderList.GetCurSel()-1);
	m_OrderList.SetFocus();
}


void CCtrlPatterns::OnSequenceNext()
//----------------------------------
{
	m_OrderList.SetCurSel(m_OrderList.GetCurSel()+1);
	m_OrderList.SetFocus();
}

// -> CODE#0015
// -> DESC="channels management dlg"
void CCtrlPatterns::OnChannelManager()
{
	if(CChannelManagerDlg::sharedInstance()){
		if(CChannelManagerDlg::sharedInstance()->IsDisplayed())
			CChannelManagerDlg::sharedInstance()->Hide();
		else{
			CChannelManagerDlg::sharedInstance()->SetDocument(NULL);
			CChannelManagerDlg::sharedInstance()->Show();
		}
	}
}
// -! NEW_FEATURE#0015


void CCtrlPatterns::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//------------------------------------------------------------------
{
	if (!m_OrderList.ProcessKeyDown(nChar))
	{
		CModControlDlg::OnKeyDown(nChar, nRepCnt, nFlags);
	}
}


void CCtrlPatterns::OnSpacingChanged()
//------------------------------------
{
	if ((m_EditSpacing.m_hWnd) && (m_EditSpacing.GetWindowTextLength() > 0))
	{
		gnPatternSpacing = GetDlgItemInt(IDC_EDIT_SPACING);
		if (gnPatternSpacing > 16) gnPatternSpacing = 16;
		SendViewMessage(VIEWMSG_SETSPACING, gnPatternSpacing);
	}
}


void CCtrlPatterns::OnInstrumentChanged()
//---------------------------------------
{
	int n = m_CbnInstrument.GetCurSel();
	if ((m_pSndFile) && (n >= 0))
	{
		n = m_CbnInstrument.GetItemData(n);
		int nmax = (m_pSndFile->m_nInstruments) ? m_pSndFile->m_nInstruments : m_pSndFile->m_nSamples;
		if ((n >= 0) && (n <= nmax) && (n != (int)m_nInstrument))
		{
			m_nInstrument = n;
			if (m_pParent) m_pParent->InstrumentChanged(m_nInstrument);
		}
		SwitchToView();
		//rewbs.instroVST
		if (HasValidPlug(m_nInstrument))
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), true);
		else
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), false);
		//rewbs.instroVST
	}
}

// -> CODE#0012
// -> DESC="midi keyboard split"
void CCtrlPatterns::OnSplitInstrumentChanged()
//---------------------------------------
{
	int n = m_CbnSplitInstrument.GetCurSel();
	if ((m_pSndFile) && (n >= 0))
	{
		n = m_CbnSplitInstrument.GetItemData(n);
		int nmax = (m_pSndFile->m_nInstruments) ? m_pSndFile->m_nInstruments : m_pSndFile->m_nSamples;
		if ((n >= 0) && (n <= nmax) && (n != (int)m_nSplitInstrument))
		{
			m_nSplitInstrument = n;
		}
		SendViewMessage(VIEWMSG_SETSPLITINSTRUMENT, m_nSplitInstrument);
		if (HasValidPlug(m_nSplitInstrument))
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI2), true);
		else
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI2), false);
		SwitchToView();
	}
}
void CCtrlPatterns::OnSplitNoteChanged()
{
	m_nSplitNote = m_CbnSplitNote.GetCurSel();
	SendViewMessage(VIEWMSG_SETSPLITNOTE, m_nSplitNote);
	SwitchToView();
}
void CCtrlPatterns::OnOctaveModifierChanged()
{
	m_nOctaveModifier = m_CbnOctaveModifier.GetCurSel();
	SendViewMessage(VIEWMSG_SETOCTAVEMODIFIER, m_nOctaveModifier);
	SwitchToView();
}
void CCtrlPatterns::OnOctaveLink()
{
	m_nOctaveLink = IsDlgButtonChecked(IDC_PATTERN_OCTAVELINK);
	SendViewMessage(VIEWMSG_SETOCTAVELINK, m_nOctaveLink);
	SwitchToView();
}
void CCtrlPatterns::OnSplitVolumeChanged()
{
	m_nSplitVolume = m_CbnSplitVolume.GetCurSel();
	SendViewMessage(VIEWMSG_SETSPLITVOLUME, m_nSplitVolume);
	SwitchToView();
}
// -! NEW_FEATURE#0012


void CCtrlPatterns::OnPrevInstrument()
//------------------------------------
{
	int n = m_CbnInstrument.GetCount();
	if (n > 0)
	{
		int pos = m_CbnInstrument.GetCurSel();
		if (pos > 0) pos--; else pos = n-1;
		m_CbnInstrument.SetCurSel(pos);
		OnInstrumentChanged();
	}
}


void CCtrlPatterns::OnNextInstrument()
//------------------------------------
{
	int n = m_CbnInstrument.GetCount();
	if (n > 0)
	{
		int pos = m_CbnInstrument.GetCurSel() + 1;
		if (pos >= n) pos = 0;
		m_CbnInstrument.SetCurSel(pos);
		OnInstrumentChanged();
	}
}


void CCtrlPatterns::OnPlayerPause()
//---------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->PauseMod();
}


void CCtrlPatterns::OnPatternNew()
//--------------------------------
{
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		UINT nCurOrd = m_OrderList.GetCurSel();
		UINT pat = pSndFile->Order[nCurOrd];
		UINT rows = 64;
		if ((pat < MAX_PATTERNS) && (pSndFile->Patterns[pat]) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)))
		{
			rows = pSndFile->PatternSize[pat];
			if (rows < 32) rows = 32;
		}
		LONG nNewPat = m_pModDoc->InsertPattern(nCurOrd+1, rows);
		if ((nNewPat >= 0) && (nNewPat < MAX_PATTERNS))
		{
			m_OrderList.SetCurSel(nCurOrd+1);
			m_OrderList.InvalidateRect(NULL, FALSE);
			SetCurrentPattern(nNewPat);
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE|HINT_PATNAMES, this);
		}
	}
	SwitchToView();
}


void CCtrlPatterns::OnPatternDuplicate()
//--------------------------------------
{
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		UINT nCurOrd = m_OrderList.GetCurSel();
		UINT nCurPat = pSndFile->Order[nCurOrd];
		UINT rows = 64;
		if (nCurPat < MAX_PATTERNS)
		{
			if ((pSndFile->Patterns[nCurPat]) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)))
			{
				rows = pSndFile->PatternSize[nCurPat];
				if (rows < 16) rows = 16;
			}
			LONG nNewPat = m_pModDoc->InsertPattern(nCurOrd+1, rows);
			if ((nNewPat >= 0) && (nNewPat < MAX_PATTERNS))
			{
				MODCOMMAND *pSrc = pSndFile->Patterns[nCurPat];
				MODCOMMAND *pDest = pSndFile->Patterns[nNewPat];
				UINT n = pSndFile->PatternSize[nCurPat];
				if (pSndFile->PatternSize[nNewPat] < n) n = pSndFile->PatternSize[nNewPat];
				n *= pSndFile->m_nChannels;
				if (n) memcpy(pDest, pSrc, n * sizeof(MODCOMMAND));
				m_OrderList.SetCurSel(nCurOrd+1);
				m_OrderList.InvalidateRect(NULL, FALSE);
				SetCurrentPattern(nNewPat);
				m_pModDoc->SetModified();
				m_pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE|HINT_PATNAMES, this);
			}
		}
	}
	SwitchToView();
}


void CCtrlPatterns::OnPatternStop()
//---------------------------------
{
	if (m_pModDoc)
	{
		CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
		if (pMainFrm) pMainFrm->PauseMod(m_pModDoc);
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();
		for (UINT i=0; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].nLength = 0;
			pSndFile->Chn[i].nROfs = 0;
			pSndFile->Chn[i].nLOfs = 0;
		}
	}
	SwitchToView();
}


void CCtrlPatterns::OnPatternPlay()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((pMainFrm) && (pModDoc))
	{
		DWORD dwPos = SendViewMessage(VIEWMSG_GETCURRENTPOS);
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		BEGIN_CRITICAL();
		// Cut instruments/samples
		for (UINT i=pSndFile->m_nChannels; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		pSndFile->m_dwSongFlags &= ~(SONG_PAUSED|SONG_STEP);
		pSndFile->LoopPattern(HIWORD(dwPos));
		pSndFile->m_nNextRow = LOWORD(dwPos);
		END_CRITICAL();
		if (pMainFrm->GetModPlaying() != pModDoc)
		{
			pSndFile->ResumePlugins();		//rewbs.VSTcompliance
			pMainFrm->PlayMod(pModDoc, m_hWndView, MPTNOTIFY_POSITION);
		}
		else
		{
			pSndFile->SuspendPlugins();		//rewbs.VSTcompliance
			pSndFile->ResumePlugins();		//rewbs.VSTcompliance
		}
	}
	SwitchToView();
}

//rewbs.playSongFromCursor
void CCtrlPatterns::OnPatternPlayNoLoop()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	
	if ((pMainFrm) && (pModDoc))
	{
		DWORD dwPos = SendViewMessage(VIEWMSG_GETCURRENTPOS);
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		int order = pSndFile->FindOrder(HIWORD(dwPos));
		if  (order < 0)
			return;			//we can't play song from a pat that's not in the orderlist.

		BEGIN_CRITICAL();
		// Cut instruments/samples
		for (UINT i=pSndFile->m_nChannels; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		pSndFile->m_dwSongFlags &= ~(SONG_PAUSED|SONG_STEP);
		pSndFile->SetCurrentOrder(order);
		pSndFile->DontLoopPattern(order, LOWORD(dwPos));
		pSndFile->m_nNextRow = LOWORD(dwPos);
		END_CRITICAL();
		if (pMainFrm->GetModPlaying() != pModDoc)
		{
			pSndFile->ResumePlugins();
			pMainFrm->PlayMod(pModDoc, m_hWndView, MPTNOTIFY_POSITION);
		}
		else
		{
			pSndFile->SuspendPlugins();
			pSndFile->ResumePlugins();
		}
	}
	SwitchToView();
}
//end rewbs.playSongFromCursor

void CCtrlPatterns::OnPatternPlayFromStart()
//------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((pMainFrm) && (pModDoc))
	{
		DWORD dwPos = SendViewMessage(VIEWMSG_GETCURRENTPOS);
		CSoundFile *pSndFile = pModDoc->GetSoundFile();
		BEGIN_CRITICAL();
		// Cut instruments/samples
		for (UINT i=0; i<MAX_CHANNELS; i++)
		{
			pSndFile->Chn[i].nPatternLoopCount = 0;
			pSndFile->Chn[i].nPatternLoop = 0;
			pSndFile->Chn[i].nFadeOutVol = 0;
			pSndFile->Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
		}
		UINT nPat = HIWORD(dwPos);
		UINT nOrd = m_OrderList.GetCurSel();
		if ((nOrd < MAX_PATTERNS) && (pSndFile->Order[nOrd] == nPat)) pSndFile->m_nCurrentPattern = pSndFile->m_nNextPattern = nOrd;
		pSndFile->m_dwSongFlags &= ~(SONG_PAUSED|SONG_STEP);
		pSndFile->LoopPattern(nPat);
		pSndFile->m_nNextRow = 0;
		pSndFile->ResetTotalTickCount();
		END_CRITICAL();
		pMainFrm->ResetElapsedTime();
		if (pMainFrm->GetModPlaying() != pModDoc)
		{
			pSndFile->ResumePlugins();	//rewbs.VSTcompliance
			pMainFrm->PlayMod(pModDoc, m_hWndView, MPTNOTIFY_POSITION);
		}
		else
		{
			pSndFile->SuspendPlugins();	//rewbs.VSTcompliance
			pSndFile->ResumePlugins();	//rewbs.VSTcompliance
		}
	}
	SwitchToView();
}


void CCtrlPatterns::OnPatternRecord()
//-----------------------------------
{
	UINT nState = m_ToolBar.GetState(IDC_PATTERN_RECORD);
	m_bRecord = ((nState & TBSTATE_CHECKED) != 0);
	SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
	SwitchToView();
}


void CCtrlPatterns::OnPatternVUMeters()
//-------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_PATTERN_VUMETERS);
	m_bVUMeters = ((nState & TBSTATE_CHECKED) != 0);
	gbPatternVUMeters = m_bVUMeters;
	SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
	SwitchToView();
}

//rewbs.patPlugName
void CCtrlPatterns::OnPatternViewPlugNames()
//-------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_VIEWPLUGNAMES);
	m_bPluginNames = ((nState & TBSTATE_CHECKED) != 0);
	gbPatternPluginNames = m_bPluginNames;
	SendViewMessage(VIEWMSG_SETPLUGINNAMES, m_bPluginNames);
	SwitchToView();
}
//end rewbs.patPlugName

void CCtrlPatterns::OnPatternProperties()
//---------------------------------------
{
	SendViewMessage(VIEWMSG_PATTERNPROPERTIES);
	SwitchToView();
}


void CCtrlPatterns::OnPatternExpand()
//-----------------------------------
{
	SendViewMessage(VIEWMSG_EXPANDPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternCopy()
//---------------------------------
{
	SendViewMessage(VIEWMSG_COPYPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternPaste()
//----------------------------------
{
	SendViewMessage(VIEWMSG_PASTEPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternShrink()
//-----------------------------------
{
	SendViewMessage(VIEWMSG_SHRINKPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternAmplify()
//------------------------------------
{
	SendViewMessage(VIEWMSG_AMPLIFYPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternPlayRow()
//------------------------------------
{
	::SendMessage(m_hWndView, WM_COMMAND, ID_PATTERN_PLAYROW, 0);
	SwitchToView();
}


void CCtrlPatterns::OnUpdateRecord(CCmdUI *pCmdUI)
//------------------------------------------------
{
	if (pCmdUI) pCmdUI->SetCheck((m_bRecord) ? TRUE : FALSE);
}


void CCtrlPatterns::OnFollowSong()
//--------------------------------
{
	SendViewMessage(VIEWMSG_FOLLOWSONG, IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG));
	SwitchToView();
}


void CCtrlPatterns::OnEditUndo()
//------------------------------
{
	if (m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_UNDO, 0);
	SwitchToView();
}


void CCtrlPatterns::OnSwitchToView()
//----------------------------------
{
	PostViewMessage(VIEWMSG_SETFOCUS);
}


void CCtrlPatterns::OnPatternNameChanged()
//----------------------------------------
{
	if ((m_pSndFile) && (m_pModDoc) && (!IsLocked()))
	{
		CHAR s[256], sold[128] = "";
		UINT nPat = SendViewMessage(VIEWMSG_GETCURRENTPATTERN);

		m_EditPatName.GetWindowText(s, MAX_PATTERNNAME);
		s[MAX_PATTERNNAME-1] = 0;
		m_pSndFile->GetPatternName(nPat, sold, sizeof(sold));
		if (strcmp(s, sold))
		{
			 m_pSndFile->SetPatternName(nPat, s);
			 if (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT)) m_pModDoc->SetModified();
			 m_pModDoc->UpdateAllViews(NULL, (nPat << 24) | HINT_PATNAMES, this);
		}
	}
}


void CCtrlPatterns::OnSetupZxxMacros()
//------------------------------------
{
	if ((m_pSndFile) && (m_pModDoc))
	{
		CMidiMacroSetup dlg(&m_pSndFile->m_MidiCfg, (m_pSndFile->m_dwSongFlags & SONG_EMBEDMIDICFG), this);
		if (dlg.DoModal() == IDOK)
		{
			m_pSndFile->m_MidiCfg = dlg.m_MidiCfg;
			if (dlg.m_bEmbed)
			{
				m_pSndFile->m_dwSongFlags |= SONG_EMBEDMIDICFG;
				m_pModDoc->SetModified();
			} else
			{
				if (m_pSndFile->m_dwSongFlags & SONG_EMBEDMIDICFG) m_pModDoc->SetModified();
				m_pSndFile->m_dwSongFlags &= ~SONG_EMBEDMIDICFG;
			}
		}
	}
}


void CCtrlPatterns::OnChordEditor()
//---------------------------------
{
	CChordEditor dlg(this);
	dlg.DoModal();
	SwitchToView();
}


void CCtrlPatterns::OnDetailLo()
//------------------------------
{
	m_ToolBar.SetState(ID_PATTERNDETAIL_LO, TBSTATE_CHECKED|TBSTATE_ENABLED);
	if (m_nDetailLevel != 1)
	{
		m_nDetailLevel = 1;
		m_ToolBar.SetState(ID_PATTERNDETAIL_MED, TBSTATE_ENABLED);
		m_ToolBar.SetState(ID_PATTERNDETAIL_HI, TBSTATE_ENABLED);
		SendViewMessage(VIEWMSG_SETDETAIL, m_nDetailLevel);
	}
	SwitchToView();
}


void CCtrlPatterns::OnDetailMed()
//-------------------------------
{
	m_ToolBar.SetState(ID_PATTERNDETAIL_MED, TBSTATE_CHECKED|TBSTATE_ENABLED);
	if (m_nDetailLevel != 2)
	{
		m_nDetailLevel = 2;
		m_ToolBar.SetState(ID_PATTERNDETAIL_LO, TBSTATE_ENABLED);
		m_ToolBar.SetState(ID_PATTERNDETAIL_HI, TBSTATE_ENABLED);
		SendViewMessage(VIEWMSG_SETDETAIL, m_nDetailLevel);
	}
	SwitchToView();
}


void CCtrlPatterns::OnDetailHi()
//------------------------------
{
	m_ToolBar.SetState(ID_PATTERNDETAIL_HI, TBSTATE_CHECKED|TBSTATE_ENABLED);
	if (m_nDetailLevel != 4)
	{
		m_nDetailLevel = 4;
		m_ToolBar.SetState(ID_PATTERNDETAIL_LO, TBSTATE_ENABLED);
		m_ToolBar.SetState(ID_PATTERNDETAIL_MED, TBSTATE_ENABLED);
		SendViewMessage(VIEWMSG_SETDETAIL, m_nDetailLevel);
	}
	SwitchToView();
}


//rewbs.introVST
void CCtrlPatterns::TogglePluginEditor()
{
	TogglePluginEditor(false);
}

void CCtrlPatterns::TogglePluginEditor(bool split)
//----------------------------------------
{
	if ((m_nInstrument) && (m_pModDoc))
	{
		UINT nPlug = m_pSndFile->Headers[(split?m_nSplitInstrument:m_nInstrument)]->nMixPlug;
		if (nPlug) //if not no plugin
		{
			PSNDMIXPLUGIN pPlug = &(m_pSndFile->m_MixPlugins[nPlug-1]);
			if (pPlug && pPlug->pMixPlugin) //if has valid plugin
			{
				m_pModDoc->TogglePluginEditor(nPlug-1);
			}
		}
	}
}

void CCtrlPatterns::ToggleSplitPluginEditor()
//----------------------------------------
{
	TogglePluginEditor(true);
}

bool CCtrlPatterns::HasValidPlug(UINT instr)
{
	if ((instr) && (instr<MAX_INSTRUMENTS) && (m_pSndFile) && m_pSndFile->Headers[instr])
	{
		UINT nPlug = m_pSndFile->Headers[instr]->nMixPlug;
		if (nPlug) //if not no plugin
		{
			PSNDMIXPLUGIN pPlug = &(m_pSndFile->m_MixPlugins[nPlug-1]);
			if (pPlug && pPlug->pMixPlugin) //if has valid plugin
			{
				return true ;
			}
		}
	}
	return false;
}

//end rewbs.instroVST

// -> CODE#0015
// -> DESC="channels management dlg"

///////////////////////////////////////////////////////////
// CChannelManagerDlg

BEGIN_MESSAGE_MAP(CChannelManagerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOVE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
	//{{AFX_MSG_MAP(CRemoveChannelsDlg)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	ON_COMMAND(IDC_BUTTON1,	OnApply)
	ON_COMMAND(IDC_BUTTON2,	OnClose)
	ON_COMMAND(IDC_BUTTON3,	OnSelectAll)
	ON_COMMAND(IDC_BUTTON4,	OnInvert)
	ON_COMMAND(IDC_BUTTON5,	OnAction1)
	ON_COMMAND(IDC_BUTTON6,	OnAction2)
	ON_COMMAND(IDC_BUTTON7,	OnStore)
	ON_COMMAND(IDC_BUTTON9,	OnRestore)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1,	OnTabSelchange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CChannelManagerDlg * CChannelManagerDlg::sharedInstance_ = NULL;

CChannelManagerDlg * CChannelManagerDlg::sharedInstance(BOOL autoCreate)
{
	if(CChannelManagerDlg::sharedInstance_ == NULL && autoCreate) CChannelManagerDlg::sharedInstance_ = new CChannelManagerDlg();
	return CChannelManagerDlg::sharedInstance_;
}

void CChannelManagerDlg::SetDocument(void * parent)
{
	if(parent && parentCtrl != parent){

		EnterCriticalSection(&applying);

		parentCtrl = parent;
		nChannelsOld = 0;

		LeaveCriticalSection(&applying);
		InvalidateRect(NULL, FALSE);
	}
}

BOOL CChannelManagerDlg::IsOwner(void * ctrl)
{
	return ctrl == parentCtrl;
}

BOOL CChannelManagerDlg::IsDisplayed(void)
{
	return show;
}

void CChannelManagerDlg::Update(void)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);
	nChannelsOld = 0;
	LeaveCriticalSection(&applying);

	InvalidateRect(NULL, FALSE);
}

BOOL CChannelManagerDlg::Show(void)
{
	if(this->m_hWnd != NULL && show == FALSE){
		ShowWindow(SW_SHOW);
		show = TRUE;
	}

	return show;
}

BOOL CChannelManagerDlg::Hide(void)
{
	if(this->m_hWnd != NULL && show == TRUE){
		ResetState(TRUE,TRUE,TRUE,TRUE,TRUE);
		ShowWindow(SW_HIDE);
		show = FALSE;
	}

	return show;
}

CChannelManagerDlg::CChannelManagerDlg(void)
{
	InitializeCriticalSection(&applying);
	mouseTracking = FALSE;
	rightButton = FALSE;
	leftButton = FALSE;
	parentCtrl = NULL;
	moveRect = FALSE;
	nChannelsOld = 0;
	bkgnd = NULL;
	show = FALSE;

	Create(IDD_CHANNELMANAGER, NULL);
	ShowWindow(SW_HIDE);
}

CChannelManagerDlg::~CChannelManagerDlg(void)
{
	if(this == CChannelManagerDlg::sharedInstance_) CChannelManagerDlg::sharedInstance_ = NULL;
	DeleteCriticalSection(&applying);
	if(bkgnd) DeleteObject(bkgnd);
}

BOOL CChannelManagerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	HWND menu = ::GetDlgItem(m_hWnd,IDC_TAB1);

    TCITEM tie; 
    tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 
    tie.pszText = "Solo/Mute"; 
    TabCtrl_InsertItem(menu, 0, &tie); 
    tie.pszText = "Record select"; 
    TabCtrl_InsertItem(menu, 1, &tie); 
    tie.pszText = "Fx plugins"; 
    TabCtrl_InsertItem(menu, 2, &tie); 
    tie.pszText = "Reorder/Remove"; 
    TabCtrl_InsertItem(menu, 3, &tie); 
	currentTab = 0;

	for(int i = 0 ; i < MAX_BASECHANNELS ; i++){
		pattern[i] = i;
		removed[i] = FALSE;
		select[i] = FALSE;
		state[i] = FALSE;
		memory[0][i] = 0;
		memory[1][i] = 0;
		memory[2][i] = 0;
		memory[3][i] = i;
	}

	::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);

	return TRUE;
}

void CChannelManagerDlg::OnApply()
{
	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(!m_pSndFile || !pModDoc) return;

	// Stop player
	CModDoc *pActiveMod = NULL;
	HWND followSong = pMainFrm->GetFollowSong(pModDoc);
	if(pMainFrm->IsPlaying()){
		if((m_pSndFile) && (!m_pSndFile->IsPaused())) pActiveMod = pMainFrm->GetModPlaying();
		pMainFrm->PauseMod();
	}

	EnterCriticalSection(&applying);

	MODCOMMAND *p,*newp;
	MODCHANNELSETTINGS settings[MAX_BASECHANNELS];
	UINT i,j,k,nChannels,newpat[MAX_BASECHANNELS],newMemory[4][MAX_BASECHANNELS];

	// Count new number of channels , copy pattern pointers & manager internal store memory
	nChannels = 0;
	for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
		if(!removed[pattern[i]]){
			newMemory[0][nChannels] = memory[0][nChannels];
			newMemory[1][nChannels] = memory[1][nChannels];
			newMemory[2][nChannels] = memory[2][nChannels];
			newpat[nChannels++] = pattern[i];
		}
	}

	BeginWaitCursor();
	BEGIN_CRITICAL();

	// Rearrange patterns content
	for(i = 0 ; i < MAX_PATTERNS ; i++){

		// Allocate a new empty pattern to replace current pattern at i'th position in pattern array
		p = m_pSndFile->Patterns[i];
		if(p) newp = CSoundFile::AllocatePattern(m_pSndFile->PatternSize[i], nChannels);

		if(p && !newp){
			END_CRITICAL();
			EndWaitCursor();
			LeaveCriticalSection(&applying);
			::MessageBox(NULL, "Pattern Data is corrupted!!!", "ERROR: Not enough memory to rearrange channels!", MB_ICONERROR | MB_OK);
			return;
		}

		// Copy data from old pattern taking care of new channel reodering
		if(p != NULL){
			for(j = 0 ; j < m_pSndFile->PatternSize[i] ; j++){
				for(k = 0 ; k < nChannels ; k++)
					memcpy(&newp[j*nChannels + k],&p[j*m_pSndFile->m_nChannels + newpat[k]],sizeof(MODCOMMAND));
			}
			// Set new pattern in pattern array & free previous pattern
			m_pSndFile->Patterns[i] = newp;
			CSoundFile::FreePattern(p);
		}
	}

	// Copy channel settings
	for(i = 0 ; i < m_pSndFile->m_nChannels ; i++) settings[i] = m_pSndFile->ChnSettings[i];

	// Redistribute channel setting & update manager internal store memory
	for(i = 0 ; i < nChannels ; i++){
		if(i != newpat[i]){
			m_pSndFile->ChnSettings[i] = settings[newpat[i]];
			memory[0][i] = newMemory[0][newpat[i]];
			memory[1][i] = newMemory[1][newpat[i]];
			memory[2][i] = newMemory[2][newpat[i]];
		}
		memory[3][i] = i;
	}

	// Also update record states (unfortunetely they are not part of channel settings)
	for(i = 0 ; i < nChannels ; i++) newMemory[1][i] = pModDoc->IsChannelRecord(i);

	pModDoc->ReinitRecordState();
	for(i = 0 ; i < nChannels ; i++){
		if(newMemory[1][newpat[i]] == 1) pModDoc->Record1Channel(i,TRUE);
		if(newMemory[1][newpat[i]] == 2) pModDoc->Record2Channel(i,TRUE);
	}

	// Update new number of channels
	m_pSndFile->m_nChannels = nChannels;
	if(pActiveMod == pModDoc){
		i = m_pSndFile->GetCurrentPos();
		m_pSndFile->m_dwSongFlags &= ~SONG_STEP;
		m_pSndFile->SetCurrentPos(0);
		m_pSndFile->SetCurrentPos(i);
	}

	END_CRITICAL();
	EndWaitCursor();

	ResetState(TRUE,TRUE,TRUE,TRUE,TRUE);
	LeaveCriticalSection(&applying);

	// Update document & player
	pModDoc->SetModified();
	pModDoc->UpdateAllViews(NULL,0xff,NULL);
	pMainFrm->PlayMod(pActiveMod, followSong, MPTNOTIFY_POSITION);

	// Redraw channel manager window
	InvalidateRect(NULL,TRUE);
}

void CChannelManagerDlg::OnClose()
{
	EnterCriticalSection(&applying);

	if(bkgnd) DeleteObject((HBITMAP)bkgnd);
	ResetState(TRUE,TRUE,TRUE,TRUE,TRUE);
	bkgnd = NULL;
	show = FALSE;

	LeaveCriticalSection(&applying);

	CDialog::OnCancel();
}

void CChannelManagerDlg::OnSelectAll()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(m_pSndFile) for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) select[i] = TRUE;

	LeaveCriticalSection(&applying);
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnInvert()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(m_pSndFile) for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) select[i] = !select[i];

	LeaveCriticalSection(&applying);
	InvalidateRect(NULL, FALSE);
}

void CChannelManagerDlg::OnAction1()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile){
		
		UINT ii,i,r;
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab){
			case 0:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						if(select[ii] && pModDoc->IsChannelSolo(ii)) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]){
						if(pModDoc->IsChannelMuted(ii)) pModDoc->MuteChannel(ii,FALSE);
						if(nbSelect == nbOk) pModDoc->SoloChannel(ii,!pModDoc->IsChannelSolo(ii));
						else pModDoc->SoloChannel(ii,TRUE);
					}
					else if(!pModDoc->IsChannelSolo(ii)) pModDoc->MuteChannel(ii,TRUE);
				}
				break;
			case 1:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						r = pModDoc->IsChannelRecord(ii);
						if(select[ii] && r == 1) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii] && select[ii]){
						if(select[ii] && nbSelect != nbOk && pModDoc->IsChannelRecord(ii) != 1) pModDoc->Record1Channel(ii);
						else if(nbSelect == nbOk) pModDoc->Record1Channel(ii,FALSE);
					}
				}
				break;
			case 2:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]) pModDoc->NoFxChannel(ii,FALSE);
				}
				break;
			case 3:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii]) removed[ii] = !removed[ii];
				}
				break;
			default:
				break;
		}


		ResetState();
		LeaveCriticalSection(&applying);

		pModDoc->UpdateAllViews(NULL,0xff,NULL);
		InvalidateRect(NULL,FALSE);
	}
	else LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnAction2()
{
	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile){
		
		UINT i,ii,r;
		int nbOk = 0, nbSelect = 0;
		
		switch(currentTab){
			case 0:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						if(select[ii] && pModDoc->IsChannelMuted(ii)) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]){
						if(pModDoc->IsChannelSolo(ii)) pModDoc->SoloChannel(ii,FALSE);
						if(nbSelect == nbOk) pModDoc->MuteChannel(ii,!pModDoc->IsChannelMuted(ii));
						else pModDoc->MuteChannel(ii,TRUE);
					}
				}
				break;
			case 1:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii]){
						if(select[ii]) nbSelect++;
						r = pModDoc->IsChannelRecord(ii);
						if(select[ii] && r == 2) nbOk++;
					}
				}
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(!removed[ii] && select[ii]){
						if(select[ii] && nbSelect != nbOk && pModDoc->IsChannelRecord(ii) != 2) pModDoc->Record2Channel(ii);
						else if(nbSelect == nbOk) pModDoc->Record2Channel(ii,FALSE);
					}
				}
				break;
			case 2:
				for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
					ii = pattern[i];
					if(select[ii] && !removed[ii]) pModDoc->NoFxChannel(ii,TRUE);
				}
				break;
			case 3:
				ResetState(FALSE,FALSE,FALSE,FALSE,TRUE);
				break;
			default:
				break;
		}

		if(currentTab !=3) ResetState();
		LeaveCriticalSection(&applying);

		pModDoc->UpdateAllViews(NULL,0xff,NULL);
		InvalidateRect(NULL,FALSE);
	}
	else LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnStore(void)
{
	if(!show) return;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	switch(currentTab){
		case 0:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				UINT ii = pattern[i];
				memory[0][i] = 0;
				if(pModDoc->IsChannelMuted(ii)) memory[0][i] |= 1;
				if(pModDoc->IsChannelSolo(ii))  memory[0][i] |= 2;
			}
			break;
		case 1:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) memory[1][i] = pModDoc->IsChannelRecord(pattern[i]);
			break;
		case 2:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) memory[2][i] = pModDoc->IsChannelNoFx(pattern[i]);
			break;
		case 3:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) memory[3][i] = pattern[i];
			break;
		default:
			break;
	}
}

void CChannelManagerDlg::OnRestore(void)
{
	if(!show) return;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	EnterCriticalSection(&applying);

	switch(currentTab){
		case 0:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				UINT ii = pattern[i];
				pModDoc->MuteChannel(ii,memory[0][i] & 1);
				pModDoc->SoloChannel(ii,memory[0][i] & 2);
			}
			break;
		case 1:
			pModDoc->ReinitRecordState(TRUE);
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				if(memory[1][i] != 2) pModDoc->Record1Channel(pattern[i],memory[1][i] == 1);
				if(memory[1][i] != 1) pModDoc->Record2Channel(pattern[i],memory[1][i] == 2);
			}
			break;
		case 2:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) pModDoc->NoFxChannel(pattern[i],memory[2][i]);
			break;
		case 3:
			for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) pattern[i] = memory[3][i];
			ResetState(FALSE,FALSE,FALSE,FALSE,TRUE);
			break;
		default:
			break;
	}

	if(currentTab !=3) ResetState();
	LeaveCriticalSection(&applying);

	pModDoc->UpdateAllViews(NULL,0xff,NULL);
	InvalidateRect(NULL,FALSE);
}

void CChannelManagerDlg::OnTabSelchange(NMHDR* header, LRESULT* pResult)
{
	if(!show) return;

	int sel = TabCtrl_GetCurFocus(::GetDlgItem(m_hWnd,IDC_TAB1));
	currentTab = sel;

	switch(currentTab){
		case 0:
			SetDlgItemText(IDC_BUTTON5, "Solo");
			SetDlgItemText(IDC_BUTTON6, "Mute");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case 1:
			SetDlgItemText(IDC_BUTTON5, "Instrument 1");
			SetDlgItemText(IDC_BUTTON6, "Instrument 2");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case 2:
			SetDlgItemText(IDC_BUTTON5, "Enable fx");
			SetDlgItemText(IDC_BUTTON6, "Disable fx");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_HIDE);
			break;
		case 3:
			SetDlgItemText(IDC_BUTTON5, "Remove");
			SetDlgItemText(IDC_BUTTON6, "Cancel all");
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON5),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON6),SW_SHOW);
			::ShowWindow(::GetDlgItem(m_hWnd, IDC_BUTTON1),SW_SHOW);
			break;
		default:
			break;
	}

	InvalidateRect(NULL, FALSE);
}

void DrawChannelButton(HDC hdc, LPRECT lpRect, LPCSTR lpszText, BOOL bActivate, BOOL bEnable, DWORD dwFlags, HBRUSH markBrush)
{
	RECT rect;
	rect = (*lpRect);
	::FillRect(hdc,&rect,bActivate ? CMainFrame::brushWindow : (bEnable ? CMainFrame::brushGray : CMainFrame::brushHighLight));

	if(bEnable){
		HGDIOBJ oldpen = ::SelectObject(hdc, CMainFrame::penLightGray);
		::MoveToEx(hdc, lpRect->left, lpRect->bottom-1, NULL);
		::LineTo(hdc, lpRect->left, lpRect->top);
		::LineTo(hdc, lpRect->right-1, lpRect->top);

		::MoveToEx(hdc, lpRect->left+1, lpRect->bottom-1, NULL);
		::LineTo(hdc, lpRect->left+1, lpRect->top+1);
		::LineTo(hdc, lpRect->right-1, lpRect->top+1);

		::SelectObject(hdc, CMainFrame::penBlack);
		::MoveToEx(hdc, lpRect->right-1, lpRect->top, NULL);
		::LineTo(hdc, lpRect->right-1, lpRect->bottom-1);
		::LineTo(hdc, lpRect->left, lpRect->bottom-1);

		::SelectObject(hdc, CMainFrame::penDarkGray);
		::MoveToEx(hdc, lpRect->right-2, lpRect->top+1, NULL);
		::LineTo(hdc, lpRect->right-2, lpRect->bottom-2);
		::LineTo(hdc, lpRect->left+1, lpRect->bottom-2);

		::SelectObject(hdc, oldpen);
	}
	else ::FrameRect(hdc,&rect,CMainFrame::brushBlack);

	if ((lpszText) && (lpszText[0])){
		rect.left += 13;
		rect.right -= 5;

		::SetBkMode(hdc, TRANSPARENT);
		HGDIOBJ oldfont = ::SelectObject(hdc, CMainFrame::GetGUIFont());

		::SetTextColor(hdc, GetSysColor(bEnable || !bActivate ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
		::DrawText(hdc, lpszText, -1, &rect, dwFlags | DT_SINGLELINE);
		::SelectObject(hdc, oldfont);
	}
}

void CChannelManagerDlg::OnSize(UINT nType,int cx,int cy)
{
	CWnd::OnSize(nType,cx,cy);
	if(!m_hWnd || show == FALSE) return;

	CWnd * button;
	CRect wnd,btn;
	GetWindowRect(&wnd);

	if(button = GetDlgItem(IDC_BUTTON1)){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if(button = GetDlgItem(IDC_BUTTON2)){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if(button = GetDlgItem(IDC_BUTTON3)){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if(button = GetDlgItem(IDC_BUTTON4)){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if(button = GetDlgItem(IDC_BUTTON5)){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}
	if(button = GetDlgItem(IDC_BUTTON6)){
		button->GetWindowRect(&btn);
		button->SetWindowPos(NULL,btn.left-wnd.left-3,wnd.Height()-btn.Height()*2-6,0,0,SWP_NOSIZE | SWP_NOZORDER);
	}

	GetClientRect(&wnd);
	wnd.SetRect(wnd.left + 10,wnd.top + 38,wnd.right - 8,wnd.bottom - 30);
	if(bkgnd) DeleteObject(bkgnd);
	bkgnd = ::CreateCompatibleBitmap(::GetDC(m_hWnd),wnd.Width(),wnd.Height());
	if(!moveRect && bkgnd){
		HDC bdc = ::CreateCompatibleDC(::GetDC(m_hWnd));
		::SelectObject(bdc,bkgnd);
		::BitBlt(bdc,0,0,wnd.Width(),wnd.Height(),::GetDC(m_hWnd),wnd.left,wnd.top,SRCCOPY);
		::SelectObject(bdc,(HBITMAP)NULL);
		::DeleteDC(bdc);
	}

	nChannelsOld = 0;
	InvalidateRect(NULL,FALSE);
}

void CChannelManagerDlg::OnActivate(UINT nState,CWnd* pWndOther,BOOL bMinimized)
{
	CDialog::OnActivate(nState,pWndOther,bMinimized);

	if(show && !bMinimized){
		ResetState(TRUE,TRUE,TRUE,TRUE,FALSE);
		EnterCriticalSection(&applying);
		nChannelsOld = 0;
		LeaveCriticalSection(&applying);
		InvalidateRect(NULL,TRUE);
	}
}

BOOL CChannelManagerDlg::OnEraseBkgnd(CDC* pDC)
{
	nChannelsOld = 0;
	return CDialog::OnEraseBkgnd(pDC);
}

void CChannelManagerDlg::OnPaint()
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	PAINTSTRUCT pDC;
	::BeginPaint(m_hWnd,&pDC);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(!pModDoc || !m_pSndFile){
		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		return;
	}

	UINT i,ii,c=0,l=0;
	UINT nColns = CM_NB_COLS;
	UINT nChannels = m_pSndFile->m_nChannels;
	UINT nLines = nChannels / nColns + (nChannels % nColns ? 1 : 0);
	CRect client,btn;

	GetWindowRect(&btn);
	GetClientRect(&client);
	client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);

	UINT chnSizeX = (client.right - client.left) / (int)nColns;
	UINT chnSizeY = (client.bottom - client.top) / (int)nLines;

	if(chnSizeY != CM_BT_HEIGHT){
		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		CWnd::SetWindowPos(NULL,0,0,btn.Width(),btn.Height()+(CM_BT_HEIGHT-chnSizeY)*nLines,SWP_NOMOVE | SWP_NOZORDER);
		return;
	}
	chnSizeY = CM_BT_HEIGHT;

	if(currentTab == 3 && moveRect && bkgnd){

		HDC bdc = ::CreateCompatibleDC(pDC.hdc);
		::SelectObject(bdc,bkgnd);
		::BitBlt(pDC.hdc,client.left,client.top,client.Width(),client.Height(),bdc,0,0,SRCCOPY);
		::SelectObject(bdc,(HBITMAP)NULL);
		::DeleteDC(bdc);

		for(i = 0 ; i < nChannels ; i++){
			ii = pattern[i];
			if(select[ii]){
				btn = move[ii];
				btn.left += mx - omx + 2;
				btn.right += mx - omx + 1;
				btn.top += my - omy + 2;
				btn.bottom += my - omy + 1;
				FrameRect(pDC.hdc,&btn,CMainFrame::brushBlack);
			}
		}

		::EndPaint(m_hWnd,&pDC);
		LeaveCriticalSection(&applying);
		return;
	}

	GetClientRect(&client);
	client.SetRect(client.left + 2,client.top + 32,client.right - 2,client.bottom - 24);

	CRect intersection;
	BOOL ok = intersection.IntersectRect(&pDC.rcPaint,&client);

	if(pDC.fErase || (ok && nChannelsOld != nChannels)){
		FillRect(pDC.hdc,&intersection,CMainFrame::brushHighLight);
		FrameRect(pDC.hdc,&client,CMainFrame::brushBlack);
		nChannelsOld = nChannels;
	}

	client.SetRect(client.left + 8,client.top + 6,client.right - 6,client.bottom - 6);

	HBRUSH red = CreateSolidBrush(RGB(192,96,96));
	HBRUSH green = CreateSolidBrush(RGB(96,192,96));

	CHAR s[256];

	for(i = 0 ; i < nChannels ; i++){

		ii = pattern[i];

		if(m_pSndFile->ChnSettings[ii].szName[0] > 0x20)
			wsprintf(s, "%s", m_pSndFile->ChnSettings[ii].szName);
		else
			wsprintf(s, "Channel %d", ii+1);

		btn.left = client.left + c * chnSizeX + 3;
		btn.right = btn.left + chnSizeX - 3;
		btn.top = client.top + l * chnSizeY + 3;
		btn.bottom = btn.top + chnSizeY - 3;

		ok = intersection.IntersectRect(&pDC.rcPaint,&client);
		if(ok) DrawChannelButton(pDC.hdc, &btn, s, select[ii], removed[ii] ? FALSE : TRUE, DT_RIGHT | DT_VCENTER, NULL);

		btn.right = btn.left + chnSizeX / 7;

		btn.left += 3;
		btn.right -= 3;
		btn.top += 3;
		btn.bottom -= 3;

		switch(currentTab){
			case 0:
				if(m_pSndFile->ChnSettings[ii].dwFlags & CHN_MUTE) FillRect(pDC.hdc,&btn,red);
				else if(m_pSndFile->ChnSettings[ii].dwFlags & CHN_SOLO) FillRect(pDC.hdc,&btn,green);
				else FillRect(pDC.hdc,&btn,CMainFrame::brushHighLight);
				break;
			case 1:
				UINT r;
				if(pModDoc) r = pModDoc->IsChannelRecord(ii);
				else r = 0;
				if(r == 1) FillRect(pDC.hdc,&btn,green);
				else if(r == 2) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,CMainFrame::brushHighLight);
				break;
			case 2:
				if(m_pSndFile->ChnSettings[ii].dwFlags & CHN_NOFX) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,green);
				break;
			case 3:
				if(removed[ii]) FillRect(pDC.hdc,&btn,red);
				else FillRect(pDC.hdc,&btn,green);
				break;
		}

		if(!removed[ii]){
			HGDIOBJ oldpen = ::SelectObject(pDC.hdc, CMainFrame::penLightGray);
			::MoveToEx(pDC.hdc, btn.right, btn.top-2, NULL);
			::LineTo(pDC.hdc, btn.right, btn.bottom+1);
			::SelectObject(pDC.hdc, oldpen);
		}
		FrameRect(pDC.hdc,&btn,CMainFrame::brushBlack);

		c++;
		if(c >= nColns) { c = 0; l++; }
	}

	DeleteObject((HBRUSH)green);
	DeleteObject((HBRUSH)red);

	::EndPaint(m_hWnd,&pDC);
	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnMove(int x, int y)
{
	CWnd::OnMove(x,y);
	nChannelsOld = 0;
}

BOOL CChannelManagerDlg::ButtonHit(CPoint point, UINT * id, CRect * invalidate)
{
	CRect client;
	GetClientRect(&client);
	client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);

	if(!PtInRect(client,point)) return FALSE;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(pModDoc && m_pSndFile){
		UINT nChannels = m_pSndFile->m_nChannels;
		UINT nColns = CM_NB_COLS;
		UINT nLines = nChannels / nColns + (nChannels % nColns ? 1 : 0);

		int x = point.x - client.left;
		int y = point.y - client.top;

		int dx = (client.right - client.left) / (int)nColns;
		int dy = CM_BT_HEIGHT;

		x = x / dx;
		y = y / dy;
		int n = y * nColns + x;
		if(n >= 0 && n < (int)m_pSndFile->m_nChannels){
			if(id) *id = n;
			if(invalidate){
				invalidate->left = client.left + x * dx;
				invalidate->right = invalidate->left + dx;
				invalidate->top = client.top + y * dy;
				invalidate->bottom = invalidate->top + dy;
			}
			return TRUE;
		}
	}
	return FALSE;
}

void CChannelManagerDlg::ResetState(BOOL selection, BOOL move, BOOL button, BOOL internal, BOOL order)
{
	for(int i = 0 ; i < MAX_BASECHANNELS ; i++){
		if(selection)
			select[pattern[i]] = FALSE;
		if(button)
			state[pattern[i]]  = FALSE;
		if(order){
			pattern[i] = i;
			removed[i] = FALSE;
		}
	}
	if(move || internal){
		leftButton = FALSE;
		rightButton = FALSE;
	}
	if(move) moveRect = FALSE;
	if(internal) mouseTracking = FALSE;

	if(order) nChannelsOld = 0;
}

LRESULT CChannelManagerDlg::OnMouseLeave(WPARAM wparam, LPARAM lparam)
{
	if(!m_hWnd || show == FALSE) return 0;

	EnterCriticalSection(&applying);
	mouseTracking = FALSE;
	ResetState(FALSE,TRUE,FALSE,TRUE);
	LeaveCriticalSection(&applying);

	return 0;
}

LRESULT CChannelManagerDlg::OnMouseHover(WPARAM wparam, LPARAM lparam)
{
	if(!m_hWnd || show == FALSE) return 0;

	EnterCriticalSection(&applying);
	mouseTracking = FALSE;
	LeaveCriticalSection(&applying);

	return 0;
}

void CChannelManagerDlg::OnMouseMove(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	if(!mouseTracking){
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(tme);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE|TME_HOVER;
		tme.dwHoverTime = 1;
		mouseTracking = _TrackMouseEvent(&tme); 
	}

	if(!leftButton && !rightButton){
		mx = point.x;
		my = point.y;
		LeaveCriticalSection(&applying);
		return;
	}
	MouseEvent(nFlags,point,moveRect ? 0 : (leftButton ? CM_BT_LEFT : CM_BT_RIGHT));

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnLButtonUp(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(moveRect && m_pSndFile){
		UINT n,i,k,newpat[MAX_BASECHANNELS];

		k = 0xffff;
		BOOL hit = ButtonHit(point,&n,NULL);
		for(i = 0 ; i < m_pSndFile->m_nChannels ; i++) if(k == 0xffff && select[pattern[i]]) k = i;

		if(hit && m_pSndFile && k != 0xffff){
			i = 0;
			k = 0;
			while(i < n){
				while(i < n && select[pattern[i]]) i++;
				if(i < n && !select[pattern[i]]){
					newpat[k] = pattern[i];
					pattern[i] = 0xffff;
					k++;
					i++;
				}
			}
			for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				if(pattern[i] != 0xffff && select[pattern[i]]){
					newpat[k] = pattern[i];
					pattern[i] = 0xffff;
					k++;
				}
			}
			i = 0;
			while(i < m_pSndFile->m_nChannels){
				while(i < m_pSndFile->m_nChannels && pattern[i] == 0xffff) i++;
				if(i < m_pSndFile->m_nChannels && pattern[i] != 0xffff){
					newpat[k] = pattern[i];
					k++;
					i++;
				}
			}
			for(i = 0 ; i < m_pSndFile->m_nChannels ; i++){
				pattern[i] = newpat[i];
				select[i] = FALSE;
			}
		}

		moveRect = FALSE;
		nChannelsOld = 0;
		InvalidateRect(NULL,FALSE);
	}

	leftButton = FALSE;

	for(int i = 0 ; i < MAX_BASECHANNELS ; i++) state[pattern[i]] = FALSE;
	if(pModDoc) pModDoc->UpdateAllViews(NULL,0xff,NULL);

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnLButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	if(!ButtonHit(point,NULL,NULL)) ResetState(TRUE,FALSE,FALSE,FALSE);

	leftButton = TRUE;
	MouseEvent(nFlags,point,CM_BT_LEFT);
	omx = point.x;
	omy = point.y;

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnRButtonUp(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	ResetState(FALSE,FALSE,TRUE,FALSE);

	rightButton = FALSE;
	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm->GetActiveDoc();
	if(pModDoc) pModDoc->UpdateAllViews(NULL,0xff,NULL);

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::OnRButtonDown(UINT nFlags,CPoint point)
{
	if(!m_hWnd || show == FALSE) return;

	EnterCriticalSection(&applying);

	if(moveRect){
		ResetState(TRUE,TRUE,FALSE,FALSE,FALSE);
		nChannelsOld = 0;
		InvalidateRect(NULL, FALSE);
		rightButton = TRUE;
	}
	else{
		rightButton = TRUE;
		MouseEvent(nFlags,point,CM_BT_RIGHT);
		omx = point.x;
		omy = point.y;
	}

	LeaveCriticalSection(&applying);
}

void CChannelManagerDlg::MouseEvent(UINT nFlags,CPoint point,BYTE button)
{
	UINT n;
	CRect client,invalidate;
	BOOL hit = ButtonHit(point,&n,&invalidate);
	if(hit) n = pattern[n];

	mx = point.x;
	my = point.y;

	CMainFrame * pMainFrm = CMainFrame::GetMainFrame();
	CModDoc *pModDoc = pMainFrm ? pMainFrm->GetActiveDoc() : NULL;
	CSoundFile * m_pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if(!pModDoc || !m_pSndFile) return;

	if(hit && !state[n] && button){
		if(nFlags & MK_CONTROL){
			if(button == CM_BT_LEFT){
				if(!select[n] && !removed[n]) move[n] = invalidate;
				select[n] = TRUE;
			}
			else if(button == CM_BT_RIGHT) select[n] = FALSE;
		}
		else if(!removed[n] || currentTab == 3){
			switch(currentTab){
				case 0:
					if(button == CM_BT_LEFT){
						if(pModDoc->IsChannelMuted(n)) pModDoc->MuteChannel(n,FALSE);
						if(!pModDoc->IsChannelSolo(n)){
							GetClientRect(&client);
							pModDoc->SoloChannel(n,TRUE);
							for(UINT i = 0 ; i < m_pSndFile->m_nChannels ; i++) if(!pModDoc->IsChannelSolo(i)) pModDoc->MuteChannel(i,TRUE);
							client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);
							invalidate = client;
						}
						else pModDoc->SoloChannel(n,FALSE);
					}
					else{
						if(pModDoc->IsChannelSolo(n)) pModDoc->SoloChannel(n,FALSE);
						pModDoc->MuteChannel(n,!pModDoc->IsChannelMuted(n));
					}
					pModDoc->SetModified();
					pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | (n << 24));
					break;
				case 1:
					UINT r;
					r = pModDoc->IsChannelRecord(n);
					if(!r || r != (button == CM_BT_LEFT ? 1 : 2)){
						if(button == CM_BT_LEFT) pModDoc->Record1Channel(n);
						else pModDoc->Record2Channel(n);
					}
					else{
						pModDoc->Record1Channel(n,FALSE);
						pModDoc->Record2Channel(n,FALSE);
					}
					break;
				case 2:
					if(button == CM_BT_LEFT) pModDoc->NoFxChannel(n,FALSE);
					else pModDoc->NoFxChannel(n,TRUE);
					pModDoc->SetModified();
					pModDoc->UpdateAllViews(NULL, HINT_MODCHANNELS | (n << 24));
					break;
				case 3:
					if(button == CM_BT_LEFT){
						move[n] = invalidate;
						select[n] = TRUE;
					}
					if(button == CM_BT_RIGHT){
						select[n] = FALSE;
						removed[n] = !removed[n];
					}

					if(select[n] || button == 0){
						GetClientRect(&client);
						client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);
						if(!bkgnd) bkgnd = ::CreateCompatibleBitmap(::GetDC(m_hWnd),client.Width(),client.Height());
						if(!moveRect && bkgnd){
							HDC bdc = ::CreateCompatibleDC(::GetDC(m_hWnd));
							::SelectObject(bdc,bkgnd);
							::BitBlt(bdc,0,0,client.Width(),client.Height(),::GetDC(m_hWnd),client.left,client.top,SRCCOPY);
							::SelectObject(bdc,(HBITMAP)NULL);
							::DeleteDC(bdc);
						}
						invalidate = client;
						moveRect = TRUE;
					}
					break;
			}
		}

		state[n] = TRUE;
		InvalidateRect(&invalidate, FALSE);
	}
	else{
		GetClientRect(&client);
		client.SetRect(client.left + 10,client.top + 38,client.right - 8,client.bottom - 30);
		InvalidateRect(&client, FALSE);
	}
}

// -! NEW_FEATURE#0015