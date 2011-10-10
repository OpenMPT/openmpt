#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "childfrm.h"
#include "moddoc.h"
#include "globals.h"
#include "PatternEditorDialogs.h"
#include "ctrl_pat.h"
#include "view_pat.h"
#include "ChannelManagerDlg.h"
#include "../common/StringFixer.h"


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
	ON_COMMAND(ID_OVERFLOWPASTE,			OnToggleOverflowPaste)
	ON_CBN_SELCHANGE(IDC_COMBO_INSTRUMENT,	OnInstrumentChanged)
	ON_COMMAND(IDC_PATINSTROPLUGGUI,		TogglePluginEditor) //rewbs.instroVST
	ON_EN_CHANGE(IDC_EDIT_SPACING,			OnSpacingChanged)
	ON_EN_CHANGE(IDC_EDIT_PATTERNNAME,		OnPatternNameChanged)
	ON_EN_CHANGE(IDC_EDIT_SEQUENCE_NAME,	OnSequenceNameChanged)
	ON_EN_CHANGE(IDC_EDIT_SEQNUM,			OnSequenceNumChanged)
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
	DDX_Control(pDX, IDC_EDIT_SPACING,			m_EditSpacing);
	DDX_Control(pDX, IDC_EDIT_ORDERLIST_MARGINS,m_EditOrderListMargins);
	DDX_Control(pDX, IDC_EDIT_PATTERNNAME,		m_EditPatName);
	DDX_Control(pDX, IDC_EDIT_SEQNUM,			m_EditSequence);
	DDX_Control(pDX, IDC_SPIN_SPACING,			m_SpinSpacing);
	DDX_Control(pDX, IDC_SPIN_ORDERLIST_MARGINS,m_SpinOrderListMargins);
	DDX_Control(pDX, IDC_SPIN_INSTRUMENT,		m_SpinInstrument);
	DDX_Control(pDX, IDC_SPIN_SEQNUM,			m_SpinSequence);
	DDX_Control(pDX, IDC_TOOLBAR1,				m_ToolBar);
	//}}AFX_DATA_MAP
}


CCtrlPatterns::CCtrlPatterns()
//----------------------------
{
	m_nInstrument = 0;
	
	m_bVUMeters = CMainFrame::GetSettings().gbPatternVUMeters;
	m_bPluginNames = CMainFrame::GetSettings().gbPatternPluginNames;	 	//rewbs.patPlugNames
	m_bRecord = CMainFrame::GetSettings().gbPatternRecord;
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
	m_ToolBar.AddButton(IDC_PATTERN_NEW, TIMAGE_PATTERN_NEW);
	m_ToolBar.AddButton(IDC_PATTERN_PLAY, TIMAGE_PATTERN_PLAY);
	m_ToolBar.AddButton(IDC_PATTERN_PLAYFROMSTART, TIMAGE_PATTERN_RESTART);
	m_ToolBar.AddButton(IDC_PATTERN_STOP, TIMAGE_PATTERN_STOP);
	m_ToolBar.AddButton(ID_PATTERN_PLAYROW, TIMAGE_PATTERN_PLAYROW);
	m_ToolBar.AddButton(IDC_PATTERN_RECORD, TIMAGE_PATTERN_RECORD, TBSTYLE_CHECK, ((m_bRecord) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_PATTERN_VUMETERS, TIMAGE_PATTERN_VUMETERS, TBSTYLE_CHECK, ((m_bVUMeters) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_VIEWPLUGNAMES, TIMAGE_PATTERN_PLUGINS, TBSTYLE_CHECK, ((m_bPluginNames) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED); //rewbs.patPlugNames
	m_ToolBar.AddButton(ID_PATTERN_CHANNELMANAGER, TIMAGE_CHANNELMANAGER);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_PATTERN_MIDIMACRO, TIMAGE_MACROEDITOR);
	m_ToolBar.AddButton(ID_PATTERN_CHORDEDIT, TIMAGE_CHORDEDITOR);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_EDIT_UNDO, TIMAGE_UNDO);
	m_ToolBar.AddButton(ID_PATTERN_PROPERTIES, TIMAGE_PATTERN_PROPERTIES);
	m_ToolBar.AddButton(ID_PATTERN_EXPAND, TIMAGE_PATTERN_EXPAND);
	m_ToolBar.AddButton(ID_PATTERN_SHRINK, TIMAGE_PATTERN_SHRINK);
	//	m_ToolBar.AddButton(ID_PATTERN_AMPLIFY, TIMAGE_SAMPLE_AMPLIFY);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_PATTERNDETAIL_LO, TIMAGE_PATTERN_DETAIL_LO, TBSTYLE_CHECK, TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_PATTERNDETAIL_MED, TIMAGE_PATTERN_DETAIL_MED, TBSTYLE_CHECK, TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_PATTERNDETAIL_HI, TIMAGE_PATTERN_DETAIL_HI, TBSTYLE_CHECK, TBSTATE_ENABLED|TBSTATE_CHECKED);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_OVERFLOWPASTE, TIMAGE_PATTERN_OVERFLOWPASTE, TBSTYLE_CHECK, ((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);

	// Special edit controls -> tab switch to view
	m_EditSequence.SetParent(this);
	m_EditSpacing.SetParent(this);
	m_EditPatName.SetParent(this);
	m_EditPatName.SetLimitText(MAX_PATTERNNAME - 1);
	m_EditOrderListMargins.SetParent(this);
	m_EditOrderListMargins.SetLimitText(3);
	// Spin controls
	m_SpinSpacing.SetRange(0, MAX_SPACING);
	m_SpinSpacing.SetPos(CMainFrame::GetSettings().gnPatternSpacing);

	m_SpinInstrument.SetRange(-1, 1);
	m_SpinInstrument.SetPos(0);

	if(CMainFrame::GetSettings().gbShowHackControls == true)
	{
		m_SpinOrderListMargins.ShowWindow(SW_SHOW);
		m_EditOrderListMargins.ShowWindow(SW_SHOW);
		m_SpinOrderListMargins.SetRange(0, m_OrderList.GetMarginsMax());
		m_SpinOrderListMargins.SetPos(m_OrderList.GetMargins());
	}
	else
	{
		m_SpinOrderListMargins.ShowWindow(SW_HIDE);
		m_EditOrderListMargins.ShowWindow(SW_HIDE);
	}

	SetDlgItemInt(IDC_EDIT_SPACING, CMainFrame::GetSettings().gnPatternSpacing);
	SetDlgItemInt(IDC_EDIT_ORDERLIST_MARGINS, m_OrderList.GetMargins());
	CheckDlgButton(IDC_PATTERN_FOLLOWSONG, !(CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_FOLLOWSONGOFF));		//rewbs.noFollow - set to unchecked

	m_SpinSequence.SetRange(0, m_pSndFile->Order.GetNumSequences() - 1);
	m_SpinSequence.SetPos(m_pSndFile->Order.GetCurrentSequenceIndex());
	SetDlgItemText(IDC_EDIT_SEQUENCE_NAME, m_pSndFile->Order.m_sName);

	m_OrderList.SetFocus(); 

	UpdateView(HINT_MODTYPE|HINT_PATNAMES, NULL);
	RecalcLayout();

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
		SetDlgItemInt(IDC_EDIT_ORDERLIST_MARGINS, m_OrderList.GetMargins());
		m_SpinOrderListMargins.SetRange(0, m_OrderList.GetMarginsMax());
		m_SpinOrderListMargins.SetPos(m_OrderList.GetMargins());
	}
}


void CCtrlPatterns::UpdateView(DWORD dwHintMask, CObject *pObj)
//-------------------------------------------------------------
{
	CHAR s[256];
	m_OrderList.UpdateView(dwHintMask, pObj);
	if (!m_pSndFile) return;

	if (dwHintMask & HINT_MODSEQUENCE)
	{
		SetDlgItemText(IDC_EDIT_SEQUENCE_NAME, m_pSndFile->Order.m_sName);
	}
	if (dwHintMask & (HINT_MODSEQUENCE|HINT_MODTYPE))
	{
		m_SpinSequence.SetRange(0, m_pSndFile->Order.GetNumSequences() - 1);
		m_SpinSequence.SetPos(m_pSndFile->Order.GetCurrentSequenceIndex());
	}

	//rewbs.instroVST
	if (dwHintMask & (HINT_MIXPLUGINS|HINT_MODTYPE))
	{
		if (HasValidPlug(m_nInstrument))
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), true);
		else
			::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), false);

		// Enable/disable multisequence controls according the current modtype.
		BOOL isMultiSeqAvail = (m_pSndFile->GetType() == MOD_TYPE_MPT) ? TRUE : FALSE;
		GetDlgItem(IDC_STATIC_SEQUENCE_NAME)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_EDIT_SEQUENCE_NAME)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_EDIT_SEQNUM)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_SPIN_SEQNUM)->EnableWindow(isMultiSeqAvail);

		// Enable/disable pattern names
		BOOL isPatNameAvail = (m_pSndFile->GetType() & (MOD_TYPE_MPT|MOD_TYPE_IT|MOD_TYPE_XM)) ? TRUE : FALSE;
		GetDlgItem(IDC_STATIC_PATTERNNAME)->EnableWindow(isPatNameAvail);
		GetDlgItem(IDC_EDIT_PATTERNNAME)->EnableWindow(isPatNameAvail);
	}
	//end rewbs.instroVST
	if (dwHintMask & HINT_MPTOPTIONS)
	{
		m_ToolBar.UpdateStyle();
// -> CODE#0007
// -> DESC="uncheck follow song checkbox by default"
		//CheckDlgButton(IDC_PATTERN_FOLLOWSONG, (CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_FOLLOWSONGOFF) ? MF_UNCHECKED : MF_CHECKED);
		m_ToolBar.SetState(ID_OVERFLOWPASTE, ((CMainFrame::GetSettings().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);
// -! BEHAVIOUR_CHANGE#0007
	}
	if (dwHintMask & (HINT_MODTYPE|HINT_INSNAMES|HINT_SMPNAMES|HINT_PATNAMES))
	{
		LockControls();
		if (dwHintMask & (HINT_MODTYPE|HINT_INSNAMES|HINT_SMPNAMES))
		{
			static const TCHAR szSplitFormat[] = TEXT("%02u %s %02u: %s/%s");
			UINT nPos = 0;
			m_CbnInstrument.SetRedraw(FALSE);
			m_CbnInstrument.ResetContent();
			m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(" No Instrument"), 0);
			const INSTRUMENTINDEX nSplitIns = m_pModDoc->GetSplitKeyboardSettings()->splitInstrument;
			const MODCOMMAND::NOTE noteSplit = 1 + m_pModDoc->GetSplitKeyboardSettings()->splitNote;
			const CString sSplitInsName = m_pModDoc->GetPatternViewInstrumentName(nSplitIns, true, false);
			if (m_pSndFile->GetNumInstruments())
			{
				// Show instrument names
				for (INSTRUMENTINDEX i = 1; i <= m_pSndFile->GetNumInstruments(); i++)
				{
					if (m_pSndFile->Instruments[i] == nullptr)
						continue;

					CString sDisplayName;
					if (m_pModDoc->GetSplitKeyboardSettings()->IsSplitActive())
					{
						wsprintf(s, szSplitFormat, nSplitIns, GetNoteStr(noteSplit), i,
								 (LPCTSTR)sSplitInsName, (LPCTSTR)m_pModDoc->GetPatternViewInstrumentName(i, true, false));
						sDisplayName = s;
					}
					else
						sDisplayName = m_pModDoc->GetPatternViewInstrumentName(i);

					UINT n = m_CbnInstrument.AddString(sDisplayName);
					if (n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
				}
			} else
			{
				// Show sample names
				SAMPLEINDEX nmax = m_pSndFile->GetNumSamples();
				for (SAMPLEINDEX i = 1; i <= nmax; i++) if (m_pSndFile->GetSample(i).pSample)
				{
					if (m_pModDoc->GetSplitKeyboardSettings()->IsSplitActive() && nSplitIns < ARRAYELEMCOUNT(m_pSndFile->m_szNames))
						wsprintf(s, szSplitFormat, nSplitIns, GetNoteStr(noteSplit), i, m_pSndFile->m_szNames[nSplitIns], m_pSndFile->m_szNames[i]);
					else
						wsprintf(s, "%02u: %s", i, m_pSndFile->m_szNames[i]);

					UINT n = m_CbnInstrument.AddString(s);
					if (n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
				}
			}
			m_CbnInstrument.SetCurSel(nPos);
			m_CbnInstrument.SetRedraw(TRUE);
		}
		if (dwHintMask & (HINT_MODTYPE|HINT_PATNAMES))
		{
			PATTERNINDEX nPat;
			if (dwHintMask & HINT_PATNAMES)
				nPat = (PATTERNINDEX)(dwHintMask >> HINT_SHIFT_PAT);
			else
				nPat = (PATTERNINDEX)SendViewMessage(VIEWMSG_GETCURRENTPATTERN);
			m_pSndFile->Patterns[nPat].GetName(s, CountOf(s));
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
		m_ToolBar.EnableButton(ID_EDIT_UNDO, m_pModDoc->GetPatternUndo()->CanUndo());
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
		SetCurrentPattern((PATTERNINDEX)lParam);
		break;

	case CTRLMSG_SETCURRENTORDER:
		//Set orderlist selection and refresh GUI if change successful
		m_OrderList.SetCurSel((ORDERINDEX)lParam, FALSE);
		break;

	case CTRLMSG_FORCEREFRESH:
		//refresh GUI
		m_OrderList.InvalidateRect(NULL, FALSE);
		break;

	case CTRLMSG_GETCURRENTORDER:
		return m_OrderList.GetCurSel(true).nOrdLo;

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
		CMainFrame::GetSettings().gbPatternRecord = m_bRecord;
		SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		break;

	case CTRLMSG_PREVORDER:
		m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).nOrdLo - 1, TRUE);
		break;
	
	case CTRLMSG_NEXTORDER:
		m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).nOrdLo + 1, TRUE);
		break;

	//rewbs.customKeys
	case CTRLMSG_PAT_FOLLOWSONG:
		// parameters: 0 = turn off, 1 = toggle
		{
			UINT state = FALSE;
			if(lParam == 1)	// toggle
			{
				state = !IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG);
			}
			CheckDlgButton(IDC_PATTERN_FOLLOWSONG, state);
			OnFollowSong();
		}
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

	case CTRLMSG_PAT_DUPPATTERN:
		OnPatternDuplicate();
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


void CCtrlPatterns::SetCurrentPattern(PATTERNINDEX nPat)
//------------------------------------------------------
{
	SendViewMessage(VIEWMSG_SETCURRENTPATTERN, (LPARAM)nPat);
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
				m_nInstrument = static_cast<INSTRUMENTINDEX>(nIns);
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
	if ((m_pModDoc) && (m_pParent))
	{
		int nIns = m_pParent->GetInstrumentChange();
		if (nIns > 0)
		{
			SetCurrentInstrument(nIns);
		}
		m_pParent->InstrumentChanged(-1);
	}
	if (!(lParam & 0x8000) && m_pSndFile)
	{
		// Pattern item
		PATTERNINDEX nPat = (PATTERNINDEX)(lParam & 0x7FFF);
		if(m_pSndFile->Patterns.IsValidIndex(nPat))
		{
			for (SEQUENCEINDEX nSeq = 0; nSeq < m_pSndFile->Order.GetNumSequences(); nSeq++)
			{
				for (ORDERINDEX nOrd = 0; nOrd < m_pSndFile->Order.GetSequence(nSeq).GetLengthTailTrimmed(); nOrd++)
				{
					if (m_pSndFile->Order.GetSequence(nSeq)[nOrd] == nPat)
					{
						m_OrderList.SelectSequence(nSeq);
						m_OrderList.SetCurSel(nOrd, true);
						break;
					}
				}
			}
		}
		SetCurrentPattern(nPat);
	} 
	else if ((lParam & 0x8000) && m_pSndFile)
	{
		// Order item
		ORDERINDEX nOrd = (ORDERINDEX)(lParam & 0x7FFF);
		SEQUENCEINDEX nSeq = (SEQUENCEINDEX)(lParam >> 16);
		if(nSeq < m_pSndFile->Order.GetNumSequences())
		{
			m_OrderList.SelectSequence(nSeq);
			if((nOrd < m_pSndFile->Order.GetSequence(nSeq).size()))
			{
				m_OrderList.SetCurSel(nOrd);
				SetCurrentPattern(m_pSndFile->Order[nOrd]);
			}
		}
	}
	if (m_hWndView)
	{
		OnSpacingChanged();
		if (m_bRecord) SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
		
		//Restore all save pattern state, except pattern number which we might have just set.
		PATTERNVIEWSTATE* patternViewState = pFrame->GetPatternViewState();
		patternViewState->nPattern = static_cast<PATTERNINDEX>(SendViewMessage(VIEWMSG_GETCURRENTPATTERN));
		if (pFrame) SendViewMessage(VIEWMSG_LOADSTATE, (LPARAM)patternViewState);
		
		SwitchToView();
	}

	// Combo boxes randomly disappear without this... why?
	Invalidate();
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
	m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).nOrdLo - 1);
	m_OrderList.SetFocus();
}


void CCtrlPatterns::OnSequenceNext()
//----------------------------------
{
	m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).nOrdLo + 1);
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
	CModControlDlg::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CCtrlPatterns::OnOrderListMarginsChanged()
//---------------------------------------------
{
	BYTE i;
	if((m_EditOrderListMargins.m_hWnd) && (m_EditOrderListMargins.IsWindowVisible()) && (m_EditOrderListMargins.GetWindowTextLength() > 0))
	{
		i = m_OrderList.SetMargins(GetDlgItemInt(IDC_EDIT_ORDERLIST_MARGINS));
	}
	else
	{
		i = m_OrderList.GetMargins();
	}

	m_SpinOrderListMargins.SetRange(0, m_OrderList.GetMarginsMax());
	SetDlgItemInt(IDC_EDIT_ORDERLIST_MARGINS, i);

}


void CCtrlPatterns::OnSpacingChanged()
//------------------------------------
{
	if ((m_EditSpacing.m_hWnd) && (m_EditSpacing.GetWindowTextLength() > 0))
	{
		CMainFrame::GetSettings().gnPatternSpacing = GetDlgItemInt(IDC_EDIT_SPACING);
		if (CMainFrame::GetSettings().gnPatternSpacing > MAX_SPACING) 
		{
			CMainFrame::GetSettings().gnPatternSpacing = MAX_SPACING;
			SetDlgItemInt(IDC_EDIT_SPACING, CMainFrame::GetSettings().gnPatternSpacing, FALSE);
		}
		SendViewMessage(VIEWMSG_SETSPACING, CMainFrame::GetSettings().gnPatternSpacing);
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
			m_nInstrument = static_cast<INSTRUMENTINDEX>(n);
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
		ORDERINDEX nCurOrd = m_OrderList.GetCurSel(true).nOrdLo;
		PATTERNINDEX nCurPat = pSndFile->Order[nCurOrd];
		ROWINDEX rows = 64;
		if(pSndFile->Patterns.IsValidPat(nCurPat))
		{
			nCurOrd++;	// only if the current oder is already occupied, create a new pattern at the next position.
			rows = pSndFile->Patterns[nCurPat].GetNumRows();
			rows = CLAMP(rows, pSndFile->GetModSpecifications().patternRowsMin, pSndFile->GetModSpecifications().patternRowsMax);
		}
		PATTERNINDEX nNewPat = m_pModDoc->InsertPattern(nCurOrd, rows);
		if ((nNewPat != PATTERNINDEX_INVALID) && (nNewPat < pSndFile->Patterns.Size()))
		{
			// update time signature
			if(pSndFile->Patterns.IsValidIndex(nCurPat) && pSndFile->Patterns[nCurPat].GetOverrideSignature())
			{
				pSndFile->Patterns[nNewPat].SetSignature(pSndFile->Patterns[nCurPat].GetRowsPerBeat(), pSndFile->Patterns[nCurPat].GetRowsPerMeasure());
			}
			// move to new pattern
			m_OrderList.SetCurSel(nCurOrd);
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
	// duplicates one or more patterns.
	if (m_pModDoc)
	{
		CSoundFile *pSndFile = m_pModDoc->GetSoundFile();

		ORD_SELECTION selection = m_OrderList.GetCurSel(false);
		ORDERINDEX nInsertCount = selection.nOrdHi - selection.nOrdLo;
		ORDERINDEX nInsertWhere = selection.nOrdLo + nInsertCount + 1;
		if (nInsertWhere >= pSndFile->GetModSpecifications().ordersMax)
			return;
		bool bSuccess = false;
		// has this pattern been duplicated already? (for multiselect)
		vector<PATTERNINDEX> pReplaceIndex;
		pReplaceIndex.resize(pSndFile->Patterns.Size(), PATTERNINDEX_INVALID);

		for(ORDERINDEX i = 0; i <= nInsertCount; i++)
		{
			PATTERNINDEX nCurPat = pSndFile->Order[selection.nOrdLo + i];
			ROWINDEX rows = 64;
			if (pSndFile->Patterns.IsValidIndex(nCurPat) && pReplaceIndex[nCurPat] == PATTERNINDEX_INVALID)
			{
				rows = pSndFile->Patterns[nCurPat].GetNumRows();
				rows = CLAMP(rows, pSndFile->GetModSpecifications().patternRowsMin, pSndFile->GetModSpecifications().patternRowsMax);

				PATTERNINDEX nNewPat = m_pModDoc->InsertPattern(nInsertWhere + i, rows);
				if ((nNewPat != PATTERNINDEX_INVALID) && (nNewPat < pSndFile->Patterns.Size()) && (pSndFile->Patterns[nCurPat] != nullptr))
				{
					// update time signature
					if(pSndFile->Patterns[nCurPat].GetOverrideSignature())
					{
						pSndFile->Patterns[nNewPat].SetSignature(pSndFile->Patterns[nCurPat].GetRowsPerBeat(), pSndFile->Patterns[nCurPat].GetRowsPerMeasure());
					}
					// copy pattern data
					MODCOMMAND *pSrc = pSndFile->Patterns[nCurPat];
					MODCOMMAND *pDest = pSndFile->Patterns[nNewPat];
					UINT n = pSndFile->Patterns[nCurPat].GetNumRows();
					if (pSndFile->Patterns[nNewPat].GetNumRows() < n) n = pSndFile->Patterns[nNewPat].GetNumRows();
					n *= pSndFile->m_nChannels;
					if (n) memcpy(pDest, pSrc, n * sizeof(MODCOMMAND));
					bSuccess = true;
					pReplaceIndex[nCurPat] = nNewPat; // mark as duplicated
				}
				else
				{
					break;
				}
			}
			else
			{
				// invalid pattern, or it has been duplicated before (multiselect)
				for (int j = pSndFile->Order.size() - 1; j > selection.nOrdLo + i + nInsertCount + 1; j--) pSndFile->Order[j] = pSndFile->Order[j - 1];
				PATTERNINDEX nNewPat; 
				if(nCurPat < pSndFile->Patterns.Size() && pReplaceIndex[nCurPat] != PATTERNINDEX_INVALID)
					nNewPat = pReplaceIndex[nCurPat]; // take care of patterns that have been duplicated before
				else
					nNewPat= pSndFile->Order[selection.nOrdLo + i];
				if (selection.nOrdLo + i + nInsertCount + 1 < pSndFile->Order.GetLength())
					pSndFile->Order[selection.nOrdLo + i + nInsertCount + 1] = nNewPat;
			}
		}
		if(bSuccess)
		{
			m_OrderList.InvalidateRect(NULL, FALSE);
			m_OrderList.SetCurSel(nInsertWhere);
			SetCurrentPattern(pSndFile->Order[min(nInsertWhere, pSndFile->Order.GetLastIndex())]);
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, HINT_MODSEQUENCE|HINT_PATNAMES, this);
			if(selection.nOrdHi != selection.nOrdLo) m_OrderList.m_nScrollPos2nd = nInsertWhere + nInsertCount;
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
		if(pSndFile)
			pSndFile->ResetChannels();
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
	CMainFrame::GetSettings().gbPatternRecord = m_bRecord;
	SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
	SwitchToView();
}


void CCtrlPatterns::OnPatternVUMeters()
//-------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_PATTERN_VUMETERS);
	m_bVUMeters = ((nState & TBSTATE_CHECKED) != 0);
	CMainFrame::GetSettings().gbPatternVUMeters = m_bVUMeters;
	SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
	SwitchToView();
}

//rewbs.patPlugName
void CCtrlPatterns::OnPatternViewPlugNames()
//------------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_VIEWPLUGNAMES);
	m_bPluginNames = ((nState & TBSTATE_CHECKED) != 0);
	CMainFrame::GetSettings().gbPatternPluginNames = m_bPluginNames;
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
		const PATTERNINDEX nPat = (PATTERNINDEX)SendViewMessage(VIEWMSG_GETCURRENTPATTERN);

		CHAR s[MAX_PATTERNNAME];
		m_EditPatName.GetWindowText(s, CountOf(s));
		StringFixer::SetNullTerminator(s);
		
		if (m_pSndFile->Patterns[nPat].GetName().Compare(s))
		{
			 if(m_pSndFile->Patterns[nPat].SetName(s))
			 {
				 if (m_pSndFile->m_nType & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) m_pModDoc->SetModified();
				 m_pModDoc->UpdateAllViews(NULL, (nPat << HINT_SHIFT_PAT) | HINT_PATNAMES, this);
			 }
		}
	}
}


void CCtrlPatterns::OnSequenceNameChanged()
//-----------------------------------------
{
	if (m_pSndFile)
	{
		CString str;
		GetDlgItemText(IDC_EDIT_SEQUENCE_NAME, str);
		if (str != m_pSndFile->Order.m_sName)
		{
			m_pSndFile->Order.m_sName = str;
			m_pModDoc->SetModified();
			m_pModDoc->UpdateAllViews(NULL, (m_pSndFile->Order.GetCurrentSequenceIndex() << HINT_SHIFT_SEQUENCE) | HINT_SEQNAMES, this);
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

				// If this macro is not the default IT macro, display a warning.
				if(!m_pModDoc->IsMacroDefaultSetupUsed())
				{
					if(Reporting::Confirm(_T("You have chosen not to embed MIDI macros. However, the current macro configuration differs from the default macro configuration that is assumed when loading a file that has no macros embedded. This can result in data loss and broken playback.\nWould you like to embed MIDI macros now?")) == cnfYes)
					{
						m_pSndFile->m_dwSongFlags |= SONG_EMBEDMIDICFG;
						m_pModDoc->SetModified();
					}
				}	
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

void CCtrlPatterns::OnToggleOverflowPaste()
//-------------------------------------
{
	CMainFrame::GetSettings().m_dwPatternSetup ^= PATTERN_OVERFLOWPASTE;
	UpdateView(HINT_MPTOPTIONS, NULL);
	SwitchToView();
}


void CCtrlPatterns::TogglePluginEditor()
//--------------------------------------
{
	if(m_nInstrument && m_pModDoc && m_pSndFile && m_pSndFile->Instruments[m_nInstrument])
	{
		UINT nPlug = m_pSndFile->Instruments[m_nInstrument]->nMixPlug;
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


bool CCtrlPatterns::HasValidPlug(UINT instr)
//------------------------------------------
{
	if ((instr) && (instr<MAX_INSTRUMENTS) && (m_pSndFile) && m_pSndFile->Instruments[instr])
	{
		UINT nPlug = m_pSndFile->Instruments[instr]->nMixPlug;
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
	if (nFlags==0)
	{
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

void CCtrlPatterns::OnSequenceNumChanged()
//----------------------------------------
{
	if ((m_EditSequence.m_hWnd) && (m_EditSequence.GetWindowTextLength() > 0))
	{
		SEQUENCEINDEX newSeq = (SEQUENCEINDEX)GetDlgItemInt(IDC_EDIT_SEQNUM);

		// avoid reloading the order list and thus setting the document modified
		if(newSeq == m_pSndFile->Order.GetCurrentSequenceIndex())
			return;
		
		if (newSeq >= MAX_SEQUENCES)
		{
			newSeq = MAX_SEQUENCES - 1;
			SetDlgItemInt(IDC_EDIT_SEQNUM, MAX_SEQUENCES - 1, FALSE);
		}
		m_OrderList.SelectSequence(newSeq);
	}
}

