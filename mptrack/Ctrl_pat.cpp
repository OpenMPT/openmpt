/*
 * ctrl_pat.cpp
 * ------------
 * Purpose: Pattern tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "InputHandler.h"
#include "ImageLists.h"
#include "Childfrm.h"
#include "Moddoc.h"
#include "../soundlib/mod_specifications.h"
#include "Globals.h"
#include "Ctrl_pat.h"
#include "View_pat.h"
#include "PatternEditorDialogs.h"
#include "ChannelManagerDlg.h"
#include "../common/StringFixer.h"


OPENMPT_NAMESPACE_BEGIN


//////////////////////////////////////////////////////////////
// CCtrlPatterns


BEGIN_MESSAGE_MAP(CCtrlPatterns, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlPatterns)
	ON_WM_KEYDOWN()
	ON_WM_VSCROLL()
	ON_WM_XBUTTONUP()
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
	ON_COMMAND(ID_PATTERN_CHANNELMANAGER,	OnChannelManager)
	ON_COMMAND(ID_PATTERN_VUMETERS,			OnPatternVUMeters)
	ON_COMMAND(ID_VIEWPLUGNAMES,			OnPatternViewPlugNames)
	ON_COMMAND(ID_NEXTINSTRUMENT,			OnNextInstrument)
	ON_COMMAND(ID_PREVINSTRUMENT,			OnPrevInstrument)
	ON_COMMAND(IDC_PATTERN_FOLLOWSONG,		OnFollowSong)
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
	ON_UPDATE_COMMAND_UI(IDC_PATTERN_RECORD,OnUpdateRecord)
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipText)
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
	DDX_Control(pDX, IDC_EDIT_PATTERNNAME,		m_EditPatName);
	DDX_Control(pDX, IDC_EDIT_SEQNUM,			m_EditSequence);
	DDX_Control(pDX, IDC_SPIN_SPACING,			m_SpinSpacing);
	DDX_Control(pDX, IDC_SPIN_INSTRUMENT,		m_SpinInstrument);
	DDX_Control(pDX, IDC_SPIN_SEQNUM,			m_SpinSequence);
	DDX_Control(pDX, IDC_TOOLBAR1,				m_ToolBar);
	//}}AFX_DATA_MAP
}


CCtrlPatterns::CCtrlPatterns(CModControlView &parent, CModDoc &document) : CModControlDlg(parent, document), m_OrderList(*this, document)
//---------------------------------------------------------------------------------------------------------------------------------------
{
	m_nInstrument = 0;

	m_bVUMeters = TrackerSettings::Instance().gbPatternVUMeters;
	m_bPluginNames = TrackerSettings::Instance().gbPatternPluginNames;
	m_bRecord = TrackerSettings::Instance().gbPatternRecord;
	m_nDetailLevel = PatternCursor::lastColumn;
}


BOOL CCtrlPatterns::OnInitDialog()
//--------------------------------
{
	CWnd::EnableToolTips(true);
	CRect rect, rcOrderList;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModControlDlg::OnInitDialog();

	if(!pMainFrm) return TRUE;
	SetRedraw(FALSE);
	LockControls();
	// Order List
	m_BtnNext.GetWindowRect(&rect);
	ScreenToClient(&rect);
	rcOrderList.left = rect.right + 4;
	rcOrderList.top = rect.top;
	rcOrderList.bottom = rect.bottom + GetSystemMetrics(SM_CYHSCROLL);
	GetClientRect(&rect);
	rcOrderList.right = rect.right - 4;
	m_OrderList.Init(rcOrderList, pMainFrm->GetGUIFont());
	// Toolbar buttons
	m_ToolBar.Init(CMainFrame::GetMainFrame()->m_PatternIcons,CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
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
	m_ToolBar.AddButton(ID_OVERFLOWPASTE, TIMAGE_PATTERN_OVERFLOWPASTE, TBSTYLE_CHECK, ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);

	// Special edit controls -> tab switch to view
	m_EditSequence.SetParent(this);
	m_EditSpacing.SetParent(this);
	m_EditPatName.SetParent(this);
	m_EditPatName.SetLimitText(MAX_PATTERNNAME - 1);
	// Spin controls
	m_SpinSpacing.SetRange(0, MAX_SPACING);
	m_SpinSpacing.SetPos(TrackerSettings::Instance().gnPatternSpacing);

	m_SpinInstrument.SetRange(-1, 1);
	m_SpinInstrument.SetPos(0);

	SetDlgItemInt(IDC_EDIT_SPACING, TrackerSettings::Instance().gnPatternSpacing);
	CheckDlgButton(IDC_PATTERN_FOLLOWSONG, !(TrackerSettings::Instance().m_dwPatternSetup & PATTERN_FOLLOWSONGOFF));

	m_SpinSequence.SetRange(0, m_sndFile.Order.GetNumSequences() - 1);
	m_SpinSequence.SetPos(m_sndFile.Order.GetCurrentSequenceIndex());
	SetDlgItemText(IDC_EDIT_SEQUENCE_NAME, m_sndFile.Order.GetName().c_str());

	m_OrderList.SetFocus();

	UpdateView(PatternHint().Names().ModType(), NULL);
	RecalcLayout();

	m_bInitialized = TRUE;
	UnlockControls();

	SetRedraw(TRUE);
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


void CCtrlPatterns::UpdateView(UpdateHint hint, CObject *pObj)
//------------------------------------------------------------
{
	m_OrderList.UpdateView(hint, pObj);
	FlagSet<HintType> hintType = hint.GetType();

	const bool updateAll = hintType[HINT_MODTYPE];
	const bool updateSeq = hint.GetCategory() == HINTCAT_SEQUENCE && hintType[HINT_MODSEQUENCE];
	const bool updatePlug = hint.GetCategory() == HINTCAT_PLUGINS && hintType[HINT_MIXPLUGINS];
	const PatternHint patternHint = hint.ToType<PatternHint>();

	if(updateAll || updateSeq)
	{
		SetDlgItemText(IDC_EDIT_SEQUENCE_NAME, m_sndFile.Order.GetName().c_str());

		m_SpinSequence.SetRange(0, m_sndFile.Order.GetNumSequences() - 1);
		m_SpinSequence.SetPos(m_sndFile.Order.GetCurrentSequenceIndex());

		// Enable/disable multisequence controls according the current modtype.
		const BOOL isMultiSeqAvail = (m_sndFile.GetModSpecifications().sequencesMax > 1 || m_sndFile.Order.GetNumSequences() > 1) ? TRUE : FALSE;
		GetDlgItem(IDC_STATIC_SEQUENCE_NAME)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_EDIT_SEQUENCE_NAME)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_EDIT_SEQNUM)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_SPIN_SEQNUM)->EnableWindow(isMultiSeqAvail);
	}

	if(updateAll || updatePlug)
	{
		GetDlgItem(IDC_PATINSTROPLUGGUI)->EnableWindow(HasValidPlug(m_nInstrument));
	}

	if(updateAll)
	{
		// Enable/disable pattern names
		const BOOL isPatNameAvail = m_sndFile.GetModSpecifications().hasPatternNames ? TRUE : FALSE;
		GetDlgItem(IDC_STATIC_PATTERNNAME)->EnableWindow(isPatNameAvail);
		GetDlgItem(IDC_EDIT_PATTERNNAME)->EnableWindow(isPatNameAvail);
	}

	if(hintType[HINT_MPTOPTIONS])
	{
		m_ToolBar.UpdateStyle();
		m_ToolBar.SetState(ID_OVERFLOWPASTE, ((TrackerSettings::Instance().m_dwPatternSetup & PATTERN_OVERFLOWPASTE) ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);
	}

	const bool updatePatNames = patternHint.GetType()[HINT_PATNAMES];
	const bool updateSmpNames = hint.GetCategory() == HINTCAT_SAMPLES && hintType[HINT_SMPNAMES];
	const bool updateInsNames = hint.GetCategory() == HINTCAT_INSTRUMENTS && hintType[HINT_INSNAMES];
	if(updateAll || updatePatNames || updateSmpNames || updateInsNames)
	{
		LockControls();
		CHAR s[256];
		if(updateAll || updateSmpNames || updateInsNames)
		{
			static const TCHAR szSplitFormat[] = _T("%02u %s %02u: %s/%s");
			UINT nPos = 0;
			m_CbnInstrument.SetRedraw(FALSE);
			m_CbnInstrument.ResetContent();
			m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(" No Instrument"), 0);
			const INSTRUMENTINDEX nSplitIns = m_modDoc.GetSplitKeyboardSettings().splitInstrument;
			const ModCommand::NOTE noteSplit = 1 + m_modDoc.GetSplitKeyboardSettings().splitNote;
			const CString sSplitInsName = m_modDoc.GetPatternViewInstrumentName(nSplitIns, true, false);
			if(m_sndFile.GetNumInstruments())
			{
				// Show instrument names
				for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
				{
					if(m_sndFile.Instruments[i] == nullptr)
						continue;

					CString sDisplayName;
					if (m_modDoc.GetSplitKeyboardSettings().IsSplitActive())
					{
						wsprintf(s, szSplitFormat, nSplitIns, m_sndFile.GetNoteName(noteSplit, nSplitIns).c_str(), i,
								 (LPCTSTR)sSplitInsName, (LPCTSTR)m_modDoc.GetPatternViewInstrumentName(i, true, false));
						sDisplayName = s;
					}
					else
						sDisplayName = m_modDoc.GetPatternViewInstrumentName(i);

					UINT n = m_CbnInstrument.AddString(sDisplayName);
					if(n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
				}
			} else
			{
				// Show sample names
				SAMPLEINDEX nmax = m_sndFile.GetNumSamples();
				for(SAMPLEINDEX i = 1; i <= nmax; i++) if (m_sndFile.GetSample(i).pSample)
				{
					if (m_modDoc.GetSplitKeyboardSettings().IsSplitActive())
						wsprintf(s, szSplitFormat, nSplitIns, m_sndFile.GetNoteName(noteSplit, nSplitIns).c_str(), i, m_sndFile.m_szNames[nSplitIns], m_sndFile.m_szNames[i]);
					else
						wsprintf(s, "%02u: %s", i, m_sndFile.m_szNames[i]);

					UINT n = m_CbnInstrument.AddString(s);
					if(n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
				}
			}
			m_CbnInstrument.SetCurSel(nPos);
			m_CbnInstrument.SetRedraw(TRUE);
			m_CbnInstrument.Invalidate(FALSE);
		}
		if(updateAll || updatePatNames)
		{
			PATTERNINDEX nPat;
			if(patternHint.GetType()[HINT_PATNAMES])
				nPat = patternHint.GetPattern();
			else
				nPat = (PATTERNINDEX)SendViewMessage(VIEWMSG_GETCURRENTPATTERN);
			if(m_sndFile.Patterns.IsValidIndex(nPat))
			{
				m_sndFile.Patterns[nPat].GetName(s);
				m_EditPatName.SetWindowText(s);
			}

			BOOL bXMIT = (m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) ? TRUE : FALSE;
			m_ToolBar.EnableButton(ID_PATTERN_MIDIMACRO, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_PROPERTIES, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_EXPAND, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_SHRINK, bXMIT);
		}
		UnlockControls();
	}
	if (hintType[HINT_MODTYPE | HINT_UNDO])
	{
		m_ToolBar.EnableButton(ID_EDIT_UNDO, m_modDoc.GetPatternUndo().CanUndo());
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
		UpdateView(PatternHint(static_cast<PATTERNINDEX>(lParam)).Names());
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

	case CTRLMSG_NOTIFYCURRENTORDER:
		if(m_OrderList.GetCurSel(false).GetSelCount() > 1 || m_OrderList.m_bDragging)
		{
			// Only update play cursor in case there's a selection
			m_OrderList.Invalidate(FALSE);
			break;
		}
		// Otherwise, just act the same as a normal selection change
		MPT_FALLTHROUGH;
	case CTRLMSG_SETCURRENTORDER:
		//Set orderlist selection and refresh GUI if change successful
		m_OrderList.SetCurSel((ORDERINDEX)lParam, false);
		break;

	case CTRLMSG_FORCEREFRESH:
		//refresh GUI
		m_OrderList.InvalidateRect(NULL, FALSE);
		break;

	case CTRLMSG_GETCURRENTORDER:
		return m_OrderList.GetCurSel(true).firstOrd;

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
			SendViewMessage(VIEWMSG_PATTERNLOOP, (m_sndFile.m_SongFlags & SONG_PATTERNLOOP) ? TRUE : FALSE);
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
		GetParentFrame()->SetActiveView(&m_parent);
		m_OrderList.SetFocus();
		break;

	case CTRLMSG_SETRECORD:
		if (lParam >= 0) m_bRecord = (BOOL)(lParam); else m_bRecord = !m_bRecord;
		m_ToolBar.SetState(IDC_PATTERN_RECORD, ((m_bRecord) ? TBSTATE_CHECKED : 0)|TBSTATE_ENABLED);
		TrackerSettings::Instance().gbPatternRecord = (m_bRecord != 0);
		SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		break;

	case CTRLMSG_PREVORDER:
		m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).firstOrd - 1, true);
		break;

	case CTRLMSG_NEXTORDER:
		m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).firstOrd + 1, true);
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
			bool setLoop = false;
			if (lParam == -1)
			{
				//Toggle loop state
				setLoop = !m_sndFile.m_SongFlags[SONG_PATTERNLOOP];
			} else
			{
				setLoop = (lParam != 0);
			}

			if (setLoop)
			{
				m_sndFile.m_SongFlags.set(SONG_PATTERNLOOP);
				CheckDlgButton(IDC_PATTERN_LOOP, BST_CHECKED);
			} else
			{
				m_sndFile.m_SongFlags.reset(SONG_PATTERNLOOP);
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
	return FALSE;
}


////////////////////////////////////////////////////////////
// CCtrlPatterns messages

void CCtrlPatterns::OnActivatePage(LPARAM lParam)
//-----------------------------------------------
{
	int nIns = m_parent.GetInstrumentChange();
	if (nIns > 0)
	{
		SetCurrentInstrument(nIns);
	}
	m_parent.InstrumentChanged(-1);

	if (!(lParam & 0x80000000))
	{
		// Pattern item
		PATTERNINDEX nPat = (PATTERNINDEX)(lParam & 0x7FFF);
		if(m_sndFile.Patterns.IsValidIndex(nPat))
		{
			for (SEQUENCEINDEX nSeq = 0; nSeq < m_sndFile.Order.GetNumSequences(); nSeq++)
			{
				for (ORDERINDEX nOrd = 0; nOrd < m_sndFile.Order.GetSequence(nSeq).GetLengthTailTrimmed(); nOrd++)
				{
					if (m_sndFile.Order.GetSequence(nSeq)[nOrd] == nPat)
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
	else if ((lParam & 0x80000000))
	{
		// Order item
		ORDERINDEX nOrd = (ORDERINDEX)(lParam & 0xFFFF);
		SEQUENCEINDEX nSeq = (SEQUENCEINDEX)((lParam >> 16) & 0x7FFF);
		if(nSeq < m_sndFile.Order.GetNumSequences())
		{
			m_OrderList.SelectSequence(nSeq);
			if((nOrd < m_sndFile.Order.GetSequence(nSeq).size()))
			{
				m_OrderList.SetCurSel(nOrd);
				SetCurrentPattern(m_sndFile.Order[nOrd]);
			}
		}
	}
	if (m_hWndView)
	{
		OnSpacingChanged();
		if (m_bRecord) SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		CChildFrame *pFrame = (CChildFrame *)GetParentFrame();

		//Restore all save pattern state, except pattern number which we might have just set.
		PATTERNVIEWSTATE &patternViewState = pFrame->GetPatternViewState();
		if(patternViewState.initialOrder != ORDERINDEX_INVALID)
		{
			if(CMainFrame::GetMainFrame()->GetModPlaying() != &m_modDoc)
				m_OrderList.SetCurSel(patternViewState.initialOrder);
			patternViewState.initialOrder = ORDERINDEX_INVALID;
		}

		patternViewState.nPattern = static_cast<PATTERNINDEX>(SendViewMessage(VIEWMSG_GETCURRENTPATTERN));
		if (pFrame) SendViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&patternViewState);

		SwitchToView();
	}

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlPatterns::OnDeactivatePage()
//------------------------------------
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if ((pFrame) && (m_hWndView)) SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&pFrame->GetPatternViewState());
}


void CCtrlPatterns::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
//----------------------------------------------------------------------------
{
	CModControlDlg::OnVScroll(nSBCode, nPos, pScrollBar);
	short int pos = (short int)m_SpinInstrument.GetPos();
	if (pos)
	{
		m_SpinInstrument.SetPos(0);
		if(pos < 0)
			OnPrevInstrument();
		else
			OnNextInstrument();
	}
}


void CCtrlPatterns::OnSequencePrev()
//----------------------------------
{
	m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).firstOrd - 1);
	m_OrderList.SetFocus();
}


void CCtrlPatterns::OnSequenceNext()
//----------------------------------
{
	m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).firstOrd + 1);
	m_OrderList.SetFocus();
}


void CCtrlPatterns::OnChannelManager()
//------------------------------------
{
	m_modDoc.OnChannelManager();
}


void CCtrlPatterns::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//------------------------------------------------------------------
{
	CModControlDlg::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CCtrlPatterns::OnSpacingChanged()
//------------------------------------
{
	if ((m_EditSpacing.m_hWnd) && (m_EditSpacing.GetWindowTextLength() > 0))
	{
		TrackerSettings::Instance().gnPatternSpacing = GetDlgItemInt(IDC_EDIT_SPACING);
		if (TrackerSettings::Instance().gnPatternSpacing > MAX_SPACING)
		{
			TrackerSettings::Instance().gnPatternSpacing = MAX_SPACING;
			SetDlgItemInt(IDC_EDIT_SPACING, TrackerSettings::Instance().gnPatternSpacing, FALSE);
		}
		SendViewMessage(VIEWMSG_SETSPACING, TrackerSettings::Instance().gnPatternSpacing);
	}
}


void CCtrlPatterns::OnInstrumentChanged()
//---------------------------------------
{
	int n = m_CbnInstrument.GetCurSel();
	if (n >= 0)
	{
		n = m_CbnInstrument.GetItemData(n);
		int nmax = (m_sndFile.m_nInstruments) ? m_sndFile.m_nInstruments : m_sndFile.m_nSamples;
		if ((n >= 0) && (n <= nmax) && (n != (int)m_nInstrument))
		{
			m_nInstrument = static_cast<INSTRUMENTINDEX>(n);
			m_parent.InstrumentChanged(m_nInstrument);
		}
		SwitchToView();
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), HasValidPlug(m_nInstrument));
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
	ORDERINDEX curOrd = m_OrderList.GetCurSel(true).firstOrd;
	PATTERNINDEX curPat = m_sndFile.Order[curOrd];
	ROWINDEX rows = 64;
	if(m_sndFile.Patterns.IsValidPat(curPat))
	{
		// Only if the current oder is already occupied, create a new pattern at the next position.
		curOrd++;
	} else
	{
		// Use currently edited pattern for new pattern length
		curPat = (PATTERNINDEX)SendViewMessage(VIEWMSG_GETCURRENTPATTERN);
	}
	if(m_sndFile.Patterns.IsValidPat(curPat))
	{
		rows = m_sndFile.Patterns[curPat].GetNumRows();
	}
	rows = Clamp(rows, m_sndFile.GetModSpecifications().patternRowsMin, m_sndFile.GetModSpecifications().patternRowsMax);
	PATTERNINDEX newPat = m_modDoc.InsertPattern(curOrd, rows);
	if(m_sndFile.Patterns.IsValidPat(newPat))
	{
		// update time signature
		if(m_sndFile.Patterns.IsValidIndex(curPat))
		{
			if(m_sndFile.Patterns[curPat].GetOverrideSignature())
				m_sndFile.Patterns[newPat].SetSignature(m_sndFile.Patterns[curPat].GetRowsPerBeat(), m_sndFile.Patterns[curPat].GetRowsPerMeasure());
			if(m_sndFile.Patterns[curPat].HasTempoSwing())
				m_sndFile.Patterns[newPat].SetTempoSwing(m_sndFile.Patterns[curPat].GetTempoSwing());
		}
		// move to new pattern
		m_OrderList.SetCurSel(curOrd);
		m_OrderList.InvalidateRect(NULL, FALSE);
		SetCurrentPattern(newPat);
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(NULL, PatternHint(newPat).Names(), this);
		m_modDoc.UpdateAllViews(NULL, SequenceHint().Data(), this);
	}
	SwitchToView();
}


// Duplicates one or more patterns.
void CCtrlPatterns::OnPatternDuplicate()
//--------------------------------------
{
	OrdSelection selection = m_OrderList.GetCurSel(false);
	const ORDERINDEX insertFrom = selection.firstOrd;
	const ORDERINDEX insertWhere = selection.lastOrd + 1u;
	if(insertWhere >= m_sndFile.GetModSpecifications().ordersMax)
		return;
	const ORDERINDEX insertCount = std::min<ORDERINDEX>(selection.lastOrd - selection.firstOrd + 1u, m_sndFile.GetModSpecifications().ordersMax - insertWhere);
	if(!insertCount)
		return;

	bool success = false;
	// Has this pattern been duplicated already? (for multiselect)
	std::vector<PATTERNINDEX> patReplaceIndex(m_sndFile.Patterns.Size(), PATTERNINDEX_INVALID);

	for(ORDERINDEX i = 0; i < insertCount; i++)
	{
		PATTERNINDEX curPat = m_sndFile.Order[insertFrom + i];
		if(curPat < patReplaceIndex.size() && patReplaceIndex[curPat] == PATTERNINDEX_INVALID)
		{
			PATTERNINDEX newPat = m_sndFile.Patterns.Duplicate(curPat, true);
			if(newPat != PATTERNINDEX_INVALID)
			{
				m_sndFile.Order.Insert(insertWhere + i, 1, newPat);
				success = true;
				// Mark as duplicated, so if this pattern is to be duplicated again, the same new pattern number is inserted into the order list.
				patReplaceIndex[curPat] = newPat;
			} else
			{
				continue;
			}
		} else
		{
			// Invalid pattern, or it has been duplicated before (multiselect)
			PATTERNINDEX newPat;
			if(curPat < patReplaceIndex.size() && patReplaceIndex[curPat] != PATTERNINDEX_INVALID)
			{
				// Take care of patterns that have been duplicated before
				newPat = patReplaceIndex[curPat];
			} else
			{
				newPat = m_sndFile.Order[insertFrom + i];
			}

			m_sndFile.Order.Insert(insertWhere + i, 1, newPat);

			success = true;
		}
	}
	if(success)
	{
		Util::InsertItem(selection.firstOrd, selection.lastOrd, m_sndFile.m_PlayState.m_nNextOrder);
		if(m_sndFile.m_PlayState.m_nSeqOverride != ORDERINDEX_INVALID)
		{
			Util::InsertItem(selection.firstOrd, selection.lastOrd, m_sndFile.m_PlayState.m_nSeqOverride);
		}
		// Adjust order lock position
		if(m_sndFile.m_lockOrderStart != ORDERINDEX_INVALID)
		{
			Util::InsertRange(selection.firstOrd, selection.lastOrd, m_sndFile.m_lockOrderStart, m_sndFile.m_lockOrderEnd);
		}

		m_OrderList.InvalidateRect(NULL, FALSE);
		m_OrderList.SetCurSel(insertWhere, true, false, true);

		// If the first duplicated order is e.g. a +++ item, we need to move the pattern display on or else we'll still edit the previously shown pattern.
		ORDERINDEX showPattern = std::min(insertWhere, m_sndFile.Order.GetLastIndex());
		while(!m_sndFile.Order.IsValidPat(showPattern) && showPattern < m_sndFile.Order.GetLastIndex())
		{
			showPattern++;
		}
		SetCurrentPattern(m_sndFile.Order[showPattern]);

		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(NULL, SequenceHint().Data(), this);
		m_modDoc.UpdateAllViews(NULL, PatternHint(PATTERNINDEX_INVALID).Names(), this);
		if(selection.lastOrd != selection.firstOrd)
			m_OrderList.m_nScrollPos2nd = insertWhere + insertCount - 1u;
	}
	SwitchToView();
}


void CCtrlPatterns::OnPatternStop()
//---------------------------------
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if (pMainFrm) pMainFrm->PauseMod(&m_modDoc);
	m_sndFile.ResetChannels();
	SwitchToView();
}


void CCtrlPatterns::OnPatternPlay()
//---------------------------------
{
	m_modDoc.OnPatternPlay();
	SwitchToView();
}

//rewbs.playSongFromCursor
void CCtrlPatterns::OnPatternPlayNoLoop()
//---------------------------------
{
	m_modDoc.OnPatternPlayNoLoop();
	SwitchToView();
}
//end rewbs.playSongFromCursor

void CCtrlPatterns::OnPatternPlayFromStart()
//------------------------------------------
{
	m_modDoc.OnPatternRestart();
	SwitchToView();
}


void CCtrlPatterns::OnPatternRecord()
//-----------------------------------
{
	UINT nState = m_ToolBar.GetState(IDC_PATTERN_RECORD);
	m_bRecord = ((nState & TBSTATE_CHECKED) != 0);
	TrackerSettings::Instance().gbPatternRecord = (m_bRecord != 0);
	SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
	SwitchToView();
}


void CCtrlPatterns::OnPatternVUMeters()
//-------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_PATTERN_VUMETERS);
	m_bVUMeters = ((nState & TBSTATE_CHECKED) != 0);
	TrackerSettings::Instance().gbPatternVUMeters = (m_bVUMeters != 0);
	SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
	SwitchToView();
}

//rewbs.patPlugName
void CCtrlPatterns::OnPatternViewPlugNames()
//------------------------------------------
{
	UINT nState = m_ToolBar.GetState(ID_VIEWPLUGNAMES);
	m_bPluginNames = ((nState & TBSTATE_CHECKED) != 0);
	TrackerSettings::Instance().gbPatternPluginNames = (m_bPluginNames != 0);
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
	if(!IsLocked())
	{
		const PATTERNINDEX nPat = (PATTERNINDEX)SendViewMessage(VIEWMSG_GETCURRENTPATTERN);

		CHAR s[MAX_PATTERNNAME];
		m_EditPatName.GetWindowText(s, CountOf(s));
		mpt::String::SetNullTerminator(s);

		if(m_sndFile.Patterns[nPat].GetName() != s)
		{
			if(m_sndFile.Patterns[nPat].SetName(s))
			{
				if(m_sndFile.GetType() & (MOD_TYPE_XM|MOD_TYPE_IT|MOD_TYPE_MPT)) m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(NULL, PatternHint(nPat).Names(), this);
			}
		}
	}
}


void CCtrlPatterns::OnSequenceNameChanged()
//-----------------------------------------
{
	CString str;
	GetDlgItemText(IDC_EDIT_SEQUENCE_NAME, str);
	if(str != m_sndFile.Order.GetName().c_str())
	{
		m_sndFile.Order.SetName(str.GetString());
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(NULL, SequenceHint(m_sndFile.Order.GetCurrentSequenceIndex()).Names(), this);
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
	if (m_nDetailLevel != PatternCursor::instrColumn)
	{
		m_nDetailLevel = PatternCursor::instrColumn;
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
	if (m_nDetailLevel != PatternCursor::volumeColumn)
	{
		m_nDetailLevel = PatternCursor::volumeColumn;
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
	if (m_nDetailLevel != PatternCursor::lastColumn)
	{
		m_nDetailLevel = PatternCursor::lastColumn;
		m_ToolBar.SetState(ID_PATTERNDETAIL_LO, TBSTATE_ENABLED);
		m_ToolBar.SetState(ID_PATTERNDETAIL_MED, TBSTATE_ENABLED);
		SendViewMessage(VIEWMSG_SETDETAIL, m_nDetailLevel);
	}
	SwitchToView();
}

void CCtrlPatterns::OnToggleOverflowPaste()
//-----------------------------------------
{
	TrackerSettings::Instance().m_dwPatternSetup ^= PATTERN_OVERFLOWPASTE;
	UpdateView(UpdateHint().MPTOptions());
	SwitchToView();
}


void CCtrlPatterns::TogglePluginEditor()
//--------------------------------------
{
	if(m_sndFile.GetInstrumentPlugin(m_nInstrument) != nullptr)
	{
		m_modDoc.TogglePluginEditor(m_sndFile.Instruments[m_nInstrument]->nMixPlug - 1, CMainFrame::GetInputHandler()->ShiftPressed());
	}
}


bool CCtrlPatterns::HasValidPlug(INSTRUMENTINDEX instr)
//-----------------------------------------------------
{
	return m_sndFile.GetInstrumentPlugin(instr) != nullptr;
}


BOOL CCtrlPatterns::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
//--------------------------------------------------------------------
{
	if (nFlags==0)
	{
		PostViewMessage(VIEWMSG_DOSCROLL, zDelta);
	}
	return CModControlDlg::OnMouseWheel(nFlags, zDelta, pt);
}


void CCtrlPatterns::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
//----------------------------------------------------------------------
{
	if(nButton == XBUTTON1) OnModCtrlMsg(CTRLMSG_PREVORDER, 0);
	else if(nButton == XBUTTON2) OnModCtrlMsg(CTRLMSG_NEXTORDER, 0);
	CModControlDlg::OnXButtonUp(nFlags, nButton, point);
}


BOOL CCtrlPatterns::OnToolTip(UINT /*id*/, NMHDR *pNMHDR, LRESULT* /*pResult*/)
//---------------------------------------------------------------------
{
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
	UINT_PTR nID = pNMHDR->idFrom;
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
		if(newSeq == m_sndFile.Order.GetCurrentSequenceIndex())
			return;

		if (newSeq >= MAX_SEQUENCES)
		{
			newSeq = MAX_SEQUENCES - 1;
			SetDlgItemInt(IDC_EDIT_SEQNUM, MAX_SEQUENCES - 1, FALSE);
		}
		m_OrderList.SelectSequence(newSeq);
	}
}

OPENMPT_NAMESPACE_END
