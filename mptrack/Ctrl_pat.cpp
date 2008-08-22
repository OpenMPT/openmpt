#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "dlg_misc.h"
#include "ctrl_pat.h"
#include "view_pat.h"
#include "ChannelManagerDlg.h"
#include ".\ctrl_pat.h"

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
	ON_COMMAND(IDC_PATTERN_LOOP,			OnChangeLoopStatus)
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
	ON_EN_KILLFOCUS(IDC_EDIT_ORDERLIST_MARGINS, OnOrderListMarginsChanged)
	ON_UPDATE_COMMAND_UI(IDC_PATTERN_RECORD,OnUpdateRecord)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	//}}AFX_MSG_MAP
	ON_WM_MOUSEWHEEL()
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
	DDX_Control(pDX, IDC_EDIT_ORDERLIST_MARGINS,m_EditOrderListMargins);
	DDX_Control(pDX, IDC_EDIT_PATTERNNAME,		m_EditPatName);
	DDX_Control(pDX, IDC_SPIN_SPACING,			m_SpinSpacing);
	DDX_Control(pDX, IDC_SPIN_ORDERLIST_MARGINS,m_SpinOrderListMargins);
	DDX_Control(pDX, IDC_SPIN_INSTRUMENT,		m_SpinInstrument);
	DDX_Control(pDX, IDC_TOOLBAR1,				m_ToolBar);
	//}}AFX_DATA_MAP
}


CCtrlPatterns::CCtrlPatterns()
//----------------------------
{
	m_nInstrument = 0;
	
	m_bVUMeters = CMainFrame::gbPatternVUMeters;
	m_bPluginNames = CMainFrame::gbPatternPluginNames;	 	//rewbs.patPlugNames
	m_bRecord = CMainFrame::gbPatternRecord;
	m_nDetailLevel = 4;
}


BOOL CCtrlPatterns::OnInitDialog()
//--------------------------------
{
	CWnd::EnableToolTips(true);
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
	m_EditPatName.SetLimitText(MAX_PATTERNNAME);
	m_EditOrderListMargins.SetParent(this);
	m_EditOrderListMargins.SetLimitText(3);
	// Spin controls
	m_SpinSpacing.SetRange(0, 16);
	m_SpinSpacing.SetPos(CMainFrame::gnPatternSpacing);

	m_SpinInstrument.SetRange(-1, 1);
	m_SpinInstrument.SetPos(0);

	m_SpinOrderListMargins.SetRange(0, m_OrderList.GetShownOrdersMax());
	m_SpinOrderListMargins.SetPos(m_OrderList.GetOrderlistMargins());

	SetDlgItemInt(IDC_EDIT_SPACING, CMainFrame::gnPatternSpacing);
	SetDlgItemInt(IDC_EDIT_ORDERLIST_MARGINS, m_OrderList.GetOrderlistMargins());
	CheckDlgButton(IDC_PATTERN_FOLLOWSONG, !(CMainFrame::m_dwPatternSetup & PATTERN_FOLLOWSONGOFF));		//rewbs.noFollow - set to unchecked
	m_OrderList.SetFocus(); 

	UpdateView(HINT_MODTYPE|HINT_PATNAMES, NULL);
	RecalcLayout();

	

// -> CODE#0012
// -> DESC="midi keyboard split"
	//rewbs.merge: fix buffer overrun:
	//CHAR s[8];
	CHAR s[10];
	
	AppendNotesToControl(m_CbnSplitNote, 0, NOTE_MAX - 1);
	
	m_nSplitInstrument = 0;
	m_nSplitNote = 60;
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
		OnOrderListMarginsChanged();
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
			m_CbnSplitInstrument.SetRedraw(FALSE);
			m_CbnSplitInstrument.ResetContent();
			m_CbnSplitInstrument.SetItemData(m_CbnSplitInstrument.AddString(" None"), 0);
			if (m_pSndFile->m_nInstruments)	{
				for (UINT i=1; i<=m_pSndFile->m_nInstruments; i++) {
					if (m_pSndFile->Headers[i] == NULL) {
						continue;
					}

					CString displayName = m_pSndFile->GetPatternViewInstrumentName(i);
					UINT n = m_CbnInstrument.AddString(displayName);
					if (n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
					m_CbnSplitInstrument.AddString(displayName);
					m_CbnSplitInstrument.SetItemData(n, i);

				}

			} else {
				UINT nmax = m_pSndFile->m_nSamples;
				while ((nmax > 1) && (m_pSndFile->Ins[nmax].pSample == NULL) && (!m_pSndFile->m_szNames[nmax][0])) nmax--;
				for (UINT i=1; i<=nmax; i++) if ((m_pSndFile->m_szNames[i][0]) || (m_pSndFile->Ins[i].pSample)) 	{
					wsprintf(s, "%02d: %s", i, m_pSndFile->m_szNames[i]);
					UINT n = m_CbnInstrument.AddString(s);
					if (n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
					m_CbnSplitInstrument.AddString(s);
					m_CbnSplitInstrument.SetItemData(n, i);
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
			UINT nPat;
			if (dwHintMask&HINT_PATNAMES) {
				nPat = (dwHintMask >> HINT_SHIFT_PAT);
			} else {
				nPat = SendViewMessage(VIEWMSG_GETCURRENTPATTERN);
			}
			m_pSndFile->GetPatternName(nPat, s, sizeof(s));
			m_EditPatName.SetWindowText(s);
			BOOL bXMIT = (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
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
		UpdateView((lParam << HINT_SHIFT_PAT) | HINT_PATNAMES, NULL);
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
		//Set orderlist selection and refresh GUI if change successful
		m_OrderList.SetCurSel(lParam, FALSE);
		break;

	case CTRLMSG_FORCEREFRESH:
		//refresh GUI
		m_OrderList.InvalidateRect(NULL, FALSE);
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
		{
			SendViewMessage(VIEWMSG_FOLLOWSONG, IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG));
			if (m_pSndFile) {
				SendViewMessage(VIEWMSG_PATTERNLOOP, (SONG_PATTERNLOOP & m_pSndFile->m_dwSongFlags));
			}
			OnSpacingChanged();
			OnOrderListMarginsChanged();
			
			SendViewMessage(VIEWMSG_SETSPLITINSTRUMENT, m_nSplitInstrument);
			SendViewMessage(VIEWMSG_SETSPLITNOTE, m_nSplitNote);
			SendViewMessage(VIEWMSG_SETOCTAVEMODIFIER, m_nOctaveModifier);
			SendViewMessage(VIEWMSG_SETOCTAVELINK, m_nOctaveLink);
			SendViewMessage(VIEWMSG_SETSPLITVOLUME, m_nSplitVolume);	
			SendViewMessage(VIEWMSG_SETDETAIL, m_nDetailLevel);
			SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
			SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
			SendViewMessage(VIEWMSG_SETPLUGINNAMES, m_bPluginNames);
		}
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

	//rewbs.customKeys
	case CTRLMSG_PAT_FOLLOWSONG:
		CheckDlgButton(IDC_PATTERN_FOLLOWSONG, !IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG));
		OnFollowSong();
		break;

	case CTRLMSG_PAT_LOOP:
		{
			if (!m_pSndFile) {
				break;
			}

		    bool setLoop = false;
			if (lParam == -1) {
				//Toggle loop state
				setLoop = !(m_pSndFile->m_dwSongFlags&SONG_PATTERNLOOP);
			} else {
				setLoop = (lParam != 0);
			}
				
			if (setLoop) {
				m_pSndFile->m_dwSongFlags |= SONG_PATTERNLOOP;
				CheckDlgButton(IDC_PATTERN_LOOP, BST_CHECKED);
			} else {
				m_pSndFile->m_dwSongFlags &= ~SONG_PATTERNLOOP;
				CheckDlgButton(IDC_PATTERN_LOOP, BST_UNCHECKED);	
			}

			break;
		}
	case CTRLMSG_PAT_NEWPATTERN:
		OnPatternNew();
		break;

	case CTRLMSG_SETUPMACROS:
		OnSetupZxxMacros();
		break;

	//end rewbs.customKeys
	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


void CCtrlPatterns::SetCurrentPattern(UINT nPat)
//----------------------------------------------
{
	SendViewMessage(VIEWMSG_SETCURRENTPATTERN, nPat);
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
	CSoundFile* pSndFile = pModDoc ? pModDoc->GetSoundFile() : NULL;

	if ((pModDoc) && (m_pParent))
	{
		int nIns = m_pParent->GetInstrumentChange();
		if (nIns > 0)
		{
			SetCurrentInstrument(nIns);
		}
		m_pParent->InstrumentChanged(-1);
	}
	if ((lParam >= 0) && (lParam < m_pSndFile->Patterns.Size()))
	{
		if (pSndFile)
		{
			for (UINT i=0; i<pSndFile->Order.size(); i++)
			{
				if (pSndFile->Order[i] == (UINT)lParam)
				{
					m_OrderList.SetCurSel(i, TRUE);
					break;
				}
				if (pSndFile->Order[i] == pSndFile->Order.GetInvalidPatIndex()) break;
			}
		}
		SetCurrentPattern(lParam);
	} 
	else if ((lParam >= 0x8000) && (lParam < pSndFile->Order.size() + 0x8000)) 
	{
		if (pSndFile)
		{
			lParam &= 0x7FFF;
			m_OrderList.SetCurSel(lParam);
			SetCurrentPattern(pSndFile->Order[lParam]);
		}
	}
	if (m_hWndView)
	{
		OnSpacingChanged();
		OnOrderListMarginsChanged();
		if (m_bRecord) SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
		
		//Restore all save pattern state, except pattern number which we might have just set.
		PATTERNVIEWSTATE* patternViewState = pFrame->GetPatternViewState();
		patternViewState->nPattern = SendViewMessage(VIEWMSG_GETCURRENTPATTERN);
		if (pFrame) SendViewMessage(VIEWMSG_LOADSTATE, (LPARAM)patternViewState);
		
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
//------------------------------------
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


void CCtrlPatterns::OnOrderListMarginsChanged()
//---------------------------------------------
{
	BYTE i;
	BYTE maxOrders = m_OrderList.GetShownOrdersMax();
	if((m_EditOrderListMargins.m_hWnd) && (m_EditOrderListMargins.GetWindowTextLength() > 0))
	{
		i = m_OrderList.SetOrderlistMargins(GetDlgItemInt(IDC_EDIT_ORDERLIST_MARGINS));
	}
	else
	{
		i = m_OrderList.GetOrderlistMargins();
	}

	m_SpinOrderListMargins.SetRange(0, maxOrders);
	SetDlgItemInt(IDC_EDIT_ORDERLIST_MARGINS, i);

}


void CCtrlPatterns::OnSpacingChanged()
//------------------------------------
{
	if ((m_EditSpacing.m_hWnd) && (m_EditSpacing.GetWindowTextLength() > 0))
	{
		CMainFrame::gnPatternSpacing = GetDlgItemInt(IDC_EDIT_SPACING);
		if (CMainFrame::gnPatternSpacing > 16) CMainFrame::gnPatternSpacing = 16;
		SendViewMessage(VIEWMSG_SETSPACING, CMainFrame::gnPatternSpacing);
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
//--------------------------------------
{
	m_nSplitNote = m_CbnSplitNote.GetCurSel();
	SendViewMessage(VIEWMSG_SETSPLITNOTE, m_nSplitNote);
	SwitchToView();
}
void CCtrlPatterns::OnOctaveModifierChanged()
//-------------------------------------------
{
	m_nOctaveModifier = m_CbnOctaveModifier.GetCurSel();
	SendViewMessage(VIEWMSG_SETOCTAVEMODIFIER, m_nOctaveModifier);
	SwitchToView();
}
void CCtrlPatterns::OnOctaveLink()
//--------------------------------
{
	m_nOctaveLink = IsDlgButtonChecked(IDC_PATTERN_OCTAVELINK);
	SendViewMessage(VIEWMSG_SETOCTAVELINK, m_nOctaveLink);
	SwitchToView();
}
void CCtrlPatterns::OnSplitVolumeChanged()
//----------------------------------------
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
		if ((pat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[pat]) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)))
		{
			rows = pSndFile->PatternSize[pat];
			if (rows < 32) rows = 32;
		}
		LONG nNewPat = m_pModDoc->InsertPattern(nCurOrd+1, rows);
		if ((nNewPat >= 0) && (nNewPat < pSndFile->Patterns.Size()))
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
		if (nCurPat < pSndFile->Patterns.Size())
		{
			if ((pSndFile->Patterns[nCurPat]) && (pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)))
			{
				rows = pSndFile->PatternSize[nCurPat];
				if (rows < pSndFile->GetModSpecifications().patternRowsMin) rows = pSndFile->GetModSpecifications().patternRowsMin;
			}
			LONG nNewPat = m_pModDoc->InsertPattern(nCurOrd+1, rows);
			if ((nNewPat >= 0) && (nNewPat < pSndFile->Patterns.Size()))
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
	if ((pMainFrm) && (pModDoc)) {
		pModDoc->OnPatternPlay();
	}
	SwitchToView();
}

//rewbs.playSongFromCursor
void CCtrlPatterns::OnPatternPlayNoLoop()
//---------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((pMainFrm) && (pModDoc)) {
		pModDoc->OnPatternPlayNoLoop();
	}
	SwitchToView();
}
//end rewbs.playSongFromCursor

void CCtrlPatterns::OnPatternPlayFromStart()
//------------------------------------------
{
	CModDoc *pModDoc = GetDocument();
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if ((pMainFrm) && (pModDoc)) {
		pModDoc->OnPatternRestart();
	}
	SwitchToView();
}


void CCtrlPatterns::OnPatternRecord()
//-----------------------------------
{
	UINT nState = m_ToolBar.GetState(IDC_PATTERN_RECORD);
	m_bRecord = ((nState & TBSTATE_CHECKED) != 0);
	CMainFrame::gbPatternRecord=m_bRecord;
	SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
	SwitchToView();
}


void CCtrlPatterns::OnPatternVUMeters()
//-------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_PATTERN_VUMETERS);
	m_bVUMeters = ((nState & TBSTATE_CHECKED) != 0);
	CMainFrame::gbPatternVUMeters = m_bVUMeters;
	SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
	SwitchToView();
}

//rewbs.patPlugName
void CCtrlPatterns::OnPatternViewPlugNames()
//------------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_VIEWPLUGNAMES);
	m_bPluginNames = ((nState & TBSTATE_CHECKED) != 0);
	CMainFrame::gbPatternPluginNames = m_bPluginNames;
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


void CCtrlPatterns::OnChangeLoopStatus()
//--------------------------------------
{
	OnModCtrlMsg(CTRLMSG_PAT_LOOP, IsDlgButtonChecked(IDC_PATTERN_LOOP));
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
			 if (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) m_pModDoc->SetModified();
			 m_pModDoc->UpdateAllViews(NULL, (nPat << HINT_SHIFT_PAT) | HINT_PATNAMES, this);
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
//--------------------------------------
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
//------------------------------------------
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


BOOL CCtrlPatterns::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//--------------------------------------------------------------------
{
	// TODO: Add your message handler code here and/or call default

	if (nFlags==0) {
		PostViewMessage(VIEWMSG_DOSCROLL, zDelta);
	}
	return CModControlDlg::OnMouseWheel(nFlags, zDelta, pt);
}

BOOL CCtrlPatterns::OnToolTip(UINT /*id*/, NMHDR *pNMHDR, LRESULT* /*pResult*/) 
//---------------------------------------------------------------------
{
    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
    UINT nID =pNMHDR->idFrom;
    if (pTTT->uFlags & TTF_IDISHWND)
    {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID((HWND)nID);
        if(nID)
        {
            pTTT->lpszText = MAKEINTRESOURCE(nID);
            pTTT->hinst = AfxGetResourceHandle();
            return(TRUE);
        }
    }

	return FALSE;
}

