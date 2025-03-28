/*
 * Ctrl_pat.cpp
 * ------------
 * Purpose: Pattern tab, upper panel.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Ctrl_pat.h"
#include "ChannelManagerDlg.h"
#include "Childfrm.h"
#include "Globals.h"
#include "HighDPISupport.h"
#include "ImageLists.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "PatternCursor.h"
#include "PatternEditorDialogs.h"
#include "Reporting.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "View_pat.h"
#include "WindowMessages.h"
#include "../common/mptStringBuffer.h"
#include "../soundlib/mod_specifications.h"


OPENMPT_NAMESPACE_BEGIN


//////////////////////////////////////////////////////////////
// CCtrlPatterns


BEGIN_MESSAGE_MAP(CCtrlPatterns, CModControlDlg)
	//{{AFX_MSG_MAP(CCtrlPatterns)
	ON_WM_KEYDOWN()
	ON_WM_VSCROLL()
	ON_WM_XBUTTONUP()
	ON_COMMAND(IDC_BUTTON1,                   &CCtrlPatterns::OnSequenceNext)
	ON_COMMAND(IDC_BUTTON2,                   &CCtrlPatterns::OnSequencePrev)
	ON_COMMAND(ID_PLAYER_PAUSE,               &CCtrlPatterns::OnPlayerPause)
	ON_COMMAND(IDC_PATTERN_NEW,               &CCtrlPatterns::OnPatternNew)
	ON_COMMAND(IDC_PATTERN_STOP,              &CCtrlPatterns::OnPatternStop)
	ON_COMMAND(IDC_PATTERN_PLAY,              &CCtrlPatterns::OnPatternPlay)
	ON_COMMAND(IDC_PATTERN_PLAYFROMSTART,     &CCtrlPatterns::OnPatternPlayFromStart)
	ON_COMMAND(IDC_PATTERN_RECORD,            &CCtrlPatterns::OnPatternRecord)
	ON_COMMAND(IDC_METRONOME,                 &CCtrlPatterns::OnToggleMetronome)
	ON_COMMAND(ID_METRONOME_SETTINGS,         &CCtrlPatterns::OnMetronomeSettings)
	ON_COMMAND(IDC_PATTERN_LOOP,              &CCtrlPatterns::OnChangeLoopStatus)
	ON_COMMAND(ID_PATTERN_PLAYROW,            &CCtrlPatterns::OnPatternPlayRow)
	ON_COMMAND(ID_PATTERN_CHANNELMANAGER,     &CCtrlPatterns::OnChannelManager)
	ON_COMMAND(ID_PATTERN_VUMETERS,           &CCtrlPatterns::OnPatternVUMeters)
	ON_COMMAND(ID_VIEWPLUGNAMES,              &CCtrlPatterns::OnPatternViewPlugNames)
	ON_COMMAND(ID_NEXTINSTRUMENT,             &CCtrlPatterns::OnNextInstrument)
	ON_COMMAND(ID_PREVINSTRUMENT,             &CCtrlPatterns::OnPrevInstrument)
	ON_COMMAND(IDC_PATTERN_FOLLOWSONG,        &CCtrlPatterns::OnFollowSong)
	ON_COMMAND(ID_PATTERN_CHORDEDIT,          &CCtrlPatterns::OnChordEditor)
	ON_COMMAND(ID_PATTERN_PROPERTIES,         &CCtrlPatterns::OnPatternProperties)
	ON_COMMAND(ID_PATTERN_EXPAND,             &CCtrlPatterns::OnPatternExpand)
	ON_COMMAND(ID_PATTERN_SHRINK,             &CCtrlPatterns::OnPatternShrink)
	ON_COMMAND(ID_PATTERN_AMPLIFY,            &CCtrlPatterns::OnPatternAmplify)
	ON_COMMAND(ID_ORDERLIST_NEW,              &CCtrlPatterns::OnPatternNew)
	ON_COMMAND(ID_ORDERLIST_COPY,             &CCtrlPatterns::OnPatternDuplicate)
	ON_COMMAND(ID_ORDERLIST_MERGE,            &CCtrlPatterns::OnPatternMerge)
	ON_COMMAND(ID_PATTERNCOPY,                &CCtrlPatterns::OnPatternCopy)
	ON_COMMAND(ID_PATTERNPASTE,               &CCtrlPatterns::OnPatternPaste)
	ON_COMMAND(ID_EDIT_UNDO,                  &CCtrlPatterns::OnEditUndo)
	ON_COMMAND(ID_PATTERNDETAIL_DROPDOWN,     &CCtrlPatterns::OnDetailSwitch)
	ON_COMMAND(ID_PATTERNDETAIL_INSTR,        &CCtrlPatterns::OnDetailInstr)
	ON_COMMAND(ID_PATTERNDETAIL_VOLUME,       &CCtrlPatterns::OnDetailVolume)
	ON_COMMAND(ID_PATTERNDETAIL_EFFECT,       &CCtrlPatterns::OnDetailEffect)
	ON_COMMAND(ID_OVERFLOWPASTE,              &CCtrlPatterns::OnToggleOverflowPaste)
	ON_CBN_DROPDOWN(IDC_COMBO_INSTRUMENT,     &CCtrlPatterns::OnOpenInstrumentDropdown)
	ON_CBN_SELENDCANCEL(IDC_COMBO_INSTRUMENT, &CCtrlPatterns::OnCancelInstrumentDropdown)
	ON_CBN_SELENDOK(IDC_COMBO_INSTRUMENT,     &CCtrlPatterns::OnInstrumentChanged)
	ON_COMMAND(IDC_PATINSTROPLUGGUI,          &CCtrlPatterns::TogglePluginEditor)
	ON_EN_CHANGE(IDC_EDIT_SPACING,            &CCtrlPatterns::OnSpacingChanged)
	ON_EN_CHANGE(IDC_EDIT_PATTERNNAME,        &CCtrlPatterns::OnPatternNameChanged)
	ON_EN_CHANGE(IDC_EDIT_SEQUENCE_NAME,      &CCtrlPatterns::OnSequenceNameChanged)
	ON_EN_CHANGE(IDC_EDIT_SEQNUM,             &CCtrlPatterns::OnSequenceNumChanged)
	ON_NOTIFY(TBN_DROPDOWN, IDC_TOOLBAR1,     &CCtrlPatterns::OnTbnDropDownToolBar)
	ON_UPDATE_COMMAND_UI(IDC_PATTERN_RECORD,  &CCtrlPatterns::OnUpdateRecord)
	//}}AFX_MSG_MAP
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

void CCtrlPatterns::DoDataExchange(CDataExchange *pDX)
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


const ModSequence &CCtrlPatterns::Order() const { return m_sndFile.Order(); }
ModSequence &CCtrlPatterns::Order() { return m_sndFile.Order(); }


CCtrlPatterns::CCtrlPatterns(CModControlView &parent, CModDoc &document)
	: CModControlDlg{parent, document}
	, m_OrderList{*this, document}
{
	m_BtnPrev.SetAccessibleText(_T("Select Previous Order"));
	m_BtnNext.SetAccessibleText(_T("Select Next Order"));
	m_bVUMeters = TrackerSettings::Instance().gbPatternVUMeters;
	m_bPluginNames = TrackerSettings::Instance().gbPatternPluginNames;
	m_bRecord = TrackerSettings::Instance().gbPatternRecord;
}


BOOL CCtrlPatterns::OnInitDialog()
{
	CRect rect, rcOrderList;
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	CModControlDlg::OnInitDialog();

	if(!pMainFrm)
		return TRUE;
	SetRedraw(FALSE);
	LockControls();
	// Order List
	const int cyHScroll = HighDPISupport::GetSystemMetrics(SM_CYHSCROLL, GetDPI());
	m_BtnNext.GetWindowRect(&rect);
	ScreenToClient(&rect);
	auto margins = HighDPISupport::ScalePixels(4, m_hWnd);
	rcOrderList.left = rect.right + margins;
	rcOrderList.top = rect.top;
	rcOrderList.bottom = rect.bottom + cyHScroll;
	GetClientRect(&rect);
	rcOrderList.right = rect.right - margins;
	m_OrderList.Init(rcOrderList);
	// Toolbar buttons
	m_ToolBar.Init(CMainFrame::GetMainFrame()->m_PatternIcons, CMainFrame::GetMainFrame()->m_PatternIconsDisabled);
	m_ToolBar.SetExtendedStyle(m_ToolBar.GetExtendedStyle() | TBSTYLE_EX_DRAWDDARROWS);
	m_ToolBar.AddButton(IDC_PATTERN_NEW, TIMAGE_PATTERN_NEW, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar.AddButton(IDC_PATTERN_PLAY, TIMAGE_PATTERN_PLAY);
	m_ToolBar.AddButton(IDC_PATTERN_PLAYFROMSTART, TIMAGE_PATTERN_RESTART);
	m_ToolBar.AddButton(IDC_PATTERN_STOP, TIMAGE_PATTERN_STOP);
	m_ToolBar.AddButton(ID_PATTERN_PLAYROW, TIMAGE_PATTERN_PLAYROW);
	m_ToolBar.AddButton(IDC_PATTERN_RECORD, TIMAGE_PATTERN_RECORD, TBSTYLE_CHECK, (m_bRecord ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);
	m_ToolBar.AddButton(IDC_METRONOME, TIMAGE_METRONOME, TBSTYLE_CHECK | TBSTYLE_DROPDOWN, (TrackerSettings::Instance().metronomeEnabled ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_PATTERN_VUMETERS, TIMAGE_PATTERN_VUMETERS, TBSTYLE_CHECK, (m_bVUMeters ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);
	m_ToolBar.AddButton(ID_VIEWPLUGNAMES, TIMAGE_PATTERN_PLUGINS, TBSTYLE_CHECK, (m_bPluginNames ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);
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
	m_ToolBar.AddButton(ID_PATTERNDETAIL_DROPDOWN, TIMAGE_PATTERN_DETAIL, TBSTYLE_BUTTON | TBSTYLE_DROPDOWN);
	m_ToolBar.AddButton(ID_SEPARATOR, 0, TBSTYLE_SEP);
	m_ToolBar.AddButton(ID_OVERFLOWPASTE, TIMAGE_PATTERN_OVERFLOWPASTE, TBSTYLE_CHECK, ((TrackerSettings::Instance().patternSetup & PatternSetup::OverflowPaste) ? TBSTATE_CHECKED : 0) | TBSTATE_ENABLED);

	m_EditPatName.SetLimitText(MAX_PATTERNNAME - 1);
	// Spin controls
	m_SpinSpacing.SetRange32(0, MAX_SPACING);
	m_SpinSpacing.SetPos(TrackerSettings::Instance().gnPatternSpacing);

	m_SpinInstrument.SetRange32(-1, 1);
	m_SpinInstrument.SetPos(0);

	SetDlgItemInt(IDC_EDIT_SPACING, TrackerSettings::Instance().gnPatternSpacing);
	CheckDlgButton(IDC_PATTERN_FOLLOWSONG, (TrackerSettings::Instance().patternSetup & PatternSetup::FollowSongOffByDefault) ? BST_UNCHECKED : BST_CHECKED);

	m_SpinSequence.SetRange32(1, m_sndFile.Order.GetNumSequences());
	m_SpinSequence.SetPos(m_sndFile.Order.GetCurrentSequenceIndex() + 1);
	SetDlgItemText(IDC_EDIT_SEQUENCE_NAME, mpt::ToCString(Order().GetName()));

	m_OrderList.SetFocus();

	UpdateView(PatternHint().Names().ModType(), nullptr);
	RecalcLayout();

	m_initialized = true;
	UnlockControls();

	SetRedraw(TRUE);
	return FALSE;
}


void CCtrlPatterns::OnDPIChanged()
{
	m_ToolBar.OnDPIChanged();
	CModControlDlg::OnDPIChanged();
}


Setting<LONG> &CCtrlPatterns::GetSplitPosRef() { return TrackerSettings::Instance().glPatternWindowHeight; }


void CCtrlPatterns::RecalcLayout()
{
	// Update Order List Position
	if(m_OrderList.m_hWnd)
	{
		CRect rect;
		int cx, cy, cellcx;
		int cyHScroll = HighDPISupport::GetSystemMetrics(SM_CYHSCROLL, GetDPI());
		m_BtnNext.GetWindowRect(&rect);
		ScreenToClient(&rect);
		cx = -(rect.right + 4);
		cy = rect.bottom - rect.top + cyHScroll;
		GetClientRect(&rect);
		cx += rect.right - 8;
		cellcx = m_OrderList.GetFontWidth();
		if(cellcx > 0)
			cx -= (cx % cellcx);
		cx += 2;
		if((cx > 0) && (cy > 0))
		{
			m_OrderList.SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_DRAWFRAME);
		}
	}
}


void CCtrlPatterns::UpdateView(UpdateHint hint, CObject *pObj)
{
	m_OrderList.UpdateView(hint, pObj);
	FlagSet<HintType> hintType = hint.GetType();

	const bool updateAll = hintType[HINT_MODTYPE];
	const bool updateSeq = hint.GetCategory() == HINTCAT_SEQUENCE;
	const bool updatePlug = hint.GetCategory() == HINTCAT_PLUGINS && hintType[HINT_MIXPLUGINS];
	const PatternHint patternHint = hint.ToType<PatternHint>();

	if(updateAll || (updateSeq && hintType[HINT_SEQNAMES]))
	{
		SetDlgItemText(IDC_EDIT_SEQUENCE_NAME, mpt::ToCString(Order().GetName()));
	}

	if(updateAll || (updateSeq && hintType[HINT_MODSEQUENCE]))
	{
		m_SpinSequence.SetRange(1, m_sndFile.Order.GetNumSequences());
		m_SpinSequence.SetPos(m_sndFile.Order.GetCurrentSequenceIndex() + 1);

		// Enable/disable multisequence controls according the current modtype.
		const BOOL isMultiSeqAvail = (m_sndFile.GetModSpecifications().sequencesMax > 1 || m_sndFile.Order.GetNumSequences() > 1) ? TRUE : FALSE;
		GetDlgItem(IDC_STATIC_SEQUENCE_NAME)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_EDIT_SEQUENCE_NAME)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_EDIT_SEQNUM)->EnableWindow(isMultiSeqAvail);
		GetDlgItem(IDC_SPIN_SEQNUM)->EnableWindow(isMultiSeqAvail);
	}

	if(updateAll || updatePlug || (hint.GetCategory() == HINTCAT_INSTRUMENTS && hintType[HINT_INSTRUMENT]))
	{
		GetDlgItem(IDC_PATINSTROPLUGGUI)->EnableWindow(HasValidPlug(m_nInstrument) ? TRUE : FALSE);
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
	}

	bool instrPluginsChanged = false;
	if(hint.GetCategory() == HINTCAT_PLUGINS && hintType[HINT_PLUGINNAMES])
	{
		const auto changedPlug = hint.ToType<PluginHint>().GetPlugin();
		for(INSTRUMENTINDEX i = 1; i <= m_sndFile.GetNumInstruments(); i++)
		{
			const auto ins = m_sndFile.Instruments[i];
			if(!ins)
				continue;
			if((!changedPlug && ins->nMixPlug != 0) || (changedPlug && ins->nMixPlug == changedPlug))
			{
				instrPluginsChanged = true;
				break;
			}
		}
	}

	const bool updatePatNames = patternHint.GetType()[HINT_PATNAMES];
	const bool updateSmpNames = hint.GetCategory() == HINTCAT_SAMPLES && hintType[HINT_SMPNAMES];
	const bool updateInsNames = (hint.GetCategory() == HINTCAT_INSTRUMENTS && hintType[HINT_INSNAMES]) || instrPluginsChanged;
	if(updateAll || updatePatNames || updateSmpNames || updateInsNames)
	{
		LockControls();
		CString s;
		if(updateAll || updateSmpNames || updateInsNames)
		{
			constexpr TCHAR szSplitFormat[] = _T("%02u %s %02u: %s/%s");
			UINT nPos = 0;
			m_CbnInstrument.SetRedraw(FALSE);
			m_CbnInstrument.ResetContent();
			m_CbnInstrument.SetItemData(m_CbnInstrument.AddString(_T(" No Instrument")), 0);
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
					if(m_modDoc.GetSplitKeyboardSettings().IsSplitActive())
					{
						s.Format(szSplitFormat,
							nSplitIns,
							mpt::ToCString(m_sndFile.GetNoteName(noteSplit, nSplitIns)).GetString(),
							i,
							sSplitInsName.GetString(),
							m_modDoc.GetPatternViewInstrumentName(i, true, false).GetString());
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
				for(SAMPLEINDEX i = 1; i <= nmax; i++) if (m_sndFile.GetSample(i).HasSampleData() || m_sndFile.GetSample(i).uFlags[CHN_ADLIB])
				{
					if (m_modDoc.GetSplitKeyboardSettings().IsSplitActive())
						s.Format(szSplitFormat,
							nSplitIns,
							mpt::ToCString(m_sndFile.GetNoteName(noteSplit, nSplitIns)).GetString(),
							i,
							mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.m_szNames[nSplitIns]).GetString(),
							mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.m_szNames[i]).GetString());
					else
						s.Format(_T("%02u: %s"),
							i,
							mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.m_szNames[i]).GetString());

					UINT n = m_CbnInstrument.AddString(s);
					if(n == m_nInstrument) nPos = n;
					m_CbnInstrument.SetItemData(n, i);
				}
			}
			m_CbnInstrument.SetCurSel(nPos);
			if(nPos == 0)
				SetCurrentInstrument(0);
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
				m_EditPatName.SetWindowText(mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.Patterns[nPat].GetName()));
			}

			BOOL bXMIT = (m_sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT)) ? TRUE : FALSE;
			m_ToolBar.EnableButton(ID_PATTERN_MIDIMACRO, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_PROPERTIES, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_EXPAND, bXMIT);
			m_ToolBar.EnableButton(ID_PATTERN_SHRINK, bXMIT);
		}
		UnlockControls();
	}
	if(hintType[HINT_MODTYPE | HINT_UNDO])
	{
		m_ToolBar.EnableButton(ID_EDIT_UNDO, m_modDoc.GetPatternUndo().CanUndo());
	}
}


CRuntimeClass *CCtrlPatterns::GetAssociatedViewClass()
{
	return RUNTIME_CLASS(CViewPattern);
}


LRESULT CCtrlPatterns::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
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

	case CTRLMSG_NOTIFYCURRENTORDER:
		if(m_OrderList.GetCurSel().GetSelCount() > 1 || m_OrderList.m_bDragging)
		{
			// Only update play cursor in case there's a selection
			m_OrderList.Invalidate(FALSE);
			break;
		}
		// Otherwise, just act the same as a normal selection change
		[[fallthrough]];
	case CTRLMSG_SETCURRENTORDER:
		// Set order list selection and refresh GUI if change successful
		m_OrderList.SetCurSel(static_cast<ORDERINDEX>(lParam), false, false, true);
		break;

	case CTRLMSG_FORCEREFRESH:
		//refresh GUI
		m_OrderList.InvalidateRect(nullptr, FALSE);
		break;

	case CTRLMSG_GETCURRENTORDER:
		return m_OrderList.GetCurSel(true).firstOrd;

	case CTRLMSG_SETCURRENTINSTRUMENT:
	case CTRLMSG_PAT_SETINSTRUMENT:
		return SetCurrentInstrument(static_cast<uint32>(lParam));

	case CTRLMSG_SETVIEWWND:
		{
			SendViewMessage(VIEWMSG_FOLLOWSONG, IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG));
			SendViewMessage(VIEWMSG_PATTERNLOOP, (m_sndFile.m_PlayState.m_flags[SONG_PATTERNLOOP]) ? TRUE : FALSE);
			OnSpacingChanged();
			SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
			SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
			SendViewMessage(VIEWMSG_SETPLUGINNAMES, m_bPluginNames);
		}
		break;

	case CTRLMSG_SETSPACING:
		SetDlgItemInt(IDC_EDIT_SPACING, static_cast<UINT>(lParam));
		break;

	case CTRLMSG_PAT_SETORDERLISTFOCUS:
		GetParentFrame()->SetActiveView(&m_parent);
		m_OrderList.SetFocus();
		break;

	case CTRLMSG_SETRECORD:
		if(lParam >= 0)
			m_bRecord = lParam != 0;
		else
			m_bRecord = !m_bRecord;
		m_ToolBar.CheckButton(IDC_PATTERN_RECORD, m_bRecord ? TRUE : FALSE);
		TrackerSettings::Instance().gbPatternRecord = m_bRecord;
		SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		break;

	case CTRLMSG_TOGGLE_METRONOME:
		m_ToolBar.CheckButton(IDC_METRONOME, m_ToolBar.IsButtonChecked(IDC_METRONOME) ? FALSE : TRUE);
		OnToggleMetronome();
		break;

	case CTRLMSG_TOGGLE_OVERFLOW_PASTE:
		m_ToolBar.CheckButton(ID_OVERFLOWPASTE, m_ToolBar.IsButtonChecked(ID_OVERFLOWPASTE) ? FALSE : TRUE);
		OnToggleOverflowPaste();
		break;

	case CTRLMSG_PREVORDER:
		m_OrderList.SetCurSel(Order().GetPreviousOrderIgnoringSkips(m_OrderList.GetCurSel(true).firstOrd), true);
		break;

	case CTRLMSG_NEXTORDER:
		m_OrderList.SetCurSel(Order().GetNextOrderIgnoringSkips(m_OrderList.GetCurSel(true).firstOrd), true);
		break;

	case CTRLMSG_PAT_FOLLOWSONG:
		// parameters: 0 = turn off, 1 = toggle
		{
			UINT state = BST_UNCHECKED;
			if(lParam == 1)	// toggle
			{
				state = (IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG) == BST_UNCHECKED) ? BST_CHECKED : BST_UNCHECKED;
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
				setLoop = !m_sndFile.m_PlayState.m_flags[SONG_PATTERNLOOP];
			} else
			{
				setLoop = (lParam != 0);
			}

			m_sndFile.m_PlayState.m_flags.set(SONG_PATTERNLOOP, setLoop);
			CheckDlgButton(IDC_PATTERN_LOOP, setLoop ? BST_CHECKED : BST_UNCHECKED);
			break;
		}
	case CTRLMSG_PAT_NEWPATTERN:
		OnPatternNew();
		break;

	case CTRLMSG_PAT_DUPPATTERN:
		OnPatternDuplicate();
		break;

	case CTRLMSG_PAT_SETSEQUENCE:
		m_OrderList.SelectSequence(static_cast<SEQUENCEINDEX>(lParam));
		UpdateView(SequenceHint(static_cast<SEQUENCEINDEX>(lParam)).Names(), nullptr);
		break;

	case CTRLMSG_PAT_UPDATE_TOOLBAR:
		m_ToolBar.CheckButton(ID_OVERFLOWPASTE, (TrackerSettings::Instance().patternSetup & PatternSetup::OverflowPaste) ? TRUE : FALSE);
		m_ToolBar.CheckButton(IDC_METRONOME, TrackerSettings::Instance().metronomeEnabled ? TRUE : FALSE);
		break;

	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}


void CCtrlPatterns::SetCurrentPattern(PATTERNINDEX nPat)
{
	SendViewMessage(VIEWMSG_SETCURRENTPATTERN, (LPARAM)nPat);
}


BOOL CCtrlPatterns::SetCurrentInstrument(UINT nIns)
{
	if(nIns == m_nInstrument)
		return TRUE;
	int n = m_CbnInstrument.GetCount();
	for(int i = 0; i < n; i++)
	{
		if(m_CbnInstrument.GetItemData(i) == nIns)
		{
			m_CbnInstrument.SetCurSel(i);
			m_nInstrument = static_cast<INSTRUMENTINDEX>(nIns);
			m_parent.InstrumentChanged(m_nInstrument);
			GetDlgItem(IDC_PATINSTROPLUGGUI)->EnableWindow(HasValidPlug(m_nInstrument) ? TRUE : FALSE);
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CCtrlPatterns::GetFollowSong() const
{
	return IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG);
}


BOOL CCtrlPatterns::GetLoopPattern() const
{
	return IsDlgButtonChecked(IDC_PATTERN_LOOP);
}



////////////////////////////////////////////////////////////
// CCtrlPatterns messages

void CCtrlPatterns::OnActivatePage(LPARAM lParam)
{
	int nIns = m_parent.GetInstrumentChange();
	if(nIns > 0)
	{
		SetCurrentInstrument(nIns);
	}

	if(!(lParam & 0x80000000))
	{
		// Pattern item
		auto pat = static_cast<PATTERNINDEX>(lParam & 0xFFFF);
		if(m_sndFile.Patterns.IsValidIndex(pat))
		{
			for(SEQUENCEINDEX seq = 0; seq < m_sndFile.Order.GetNumSequences(); seq++)
			{
				if(ORDERINDEX ord = m_sndFile.Order(seq).FindOrder(pat); ord != ORDERINDEX_INVALID)
				{
					m_OrderList.SelectSequence(seq);
					m_OrderList.SetCurSel(ord, true);
					UpdateView(SequenceHint(seq).Names(), nullptr);
					break;
				}
			}
		}
		SetCurrentPattern(pat);
	} else if((lParam & 0x80000000))
	{
		// Order item
		auto ord = static_cast<ORDERINDEX>(lParam & 0xFFFF);
		auto seq = static_cast<SEQUENCEINDEX>((lParam >> 16) & 0x7FFF);
		if(seq < m_sndFile.Order.GetNumSequences())
		{
			m_OrderList.SelectSequence(seq);
			const auto &order = Order();
			if(ord < order.size())
			{
				m_OrderList.SetCurSel(ord);
				SetCurrentPattern(order[ord]);
			}
			UpdateView(SequenceHint(static_cast<SEQUENCEINDEX>(seq)).Names(), nullptr);
		}
	}
	if(m_hWndView)
	{
		OnSpacingChanged();
		if(m_bRecord)
			SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
		CChildFrame *pFrame = (CChildFrame *)GetParentFrame();

		// Restore all save pattern state, except pattern number which we might have just set.
		PatternViewState &patternViewState = pFrame->GetPatternViewState();
		if(patternViewState.initialOrder != ORDERINDEX_INVALID)
		{
			if(CMainFrame::GetMainFrame()->GetModPlaying() != &m_modDoc)
				m_OrderList.SetCurSel(patternViewState.initialOrder);
			patternViewState.initialOrder = ORDERINDEX_INVALID;
		}

		patternViewState.nPattern = static_cast<PATTERNINDEX>(SendViewMessage(VIEWMSG_GETCURRENTPATTERN));
		SendViewMessage(VIEWMSG_LOADSTATE, (LPARAM)&patternViewState);

		SwitchToView();
	}

	// Combo boxes randomly disappear without this... why?
	Invalidate();
}


void CCtrlPatterns::OnDeactivatePage()
{
	CChildFrame *pFrame = (CChildFrame *)GetParentFrame();
	if((pFrame) && (m_hWndView))
		SendViewMessage(VIEWMSG_SAVESTATE, (LPARAM)&pFrame->GetPatternViewState());
}


void CCtrlPatterns::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	CModControlDlg::OnVScroll(nSBCode, nPos, pScrollBar);
	short int pos = (short int)m_SpinInstrument.GetPos();
	if(pos)
	{
		m_SpinInstrument.SetPos(0);
		if(pos < 0)
			OnPrevInstrument();
		else
			OnNextInstrument();
	}
}


void CCtrlPatterns::OnSequencePrev()
{
	m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).firstOrd - 1);
	m_OrderList.SetFocus();
}


void CCtrlPatterns::OnSequenceNext()
{
	m_OrderList.SetCurSel(m_OrderList.GetCurSel(true).firstOrd + 1);
	m_OrderList.SetFocus();
}


void CCtrlPatterns::OnChannelManager()
{
	m_modDoc.OnChannelManager();
}


void CCtrlPatterns::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CModControlDlg::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CCtrlPatterns::OnSpacingChanged()
{
	if((m_EditSpacing.m_hWnd) && (m_EditSpacing.GetWindowTextLength() > 0))
	{
		TrackerSettings::Instance().gnPatternSpacing = GetDlgItemInt(IDC_EDIT_SPACING);
		if(TrackerSettings::Instance().gnPatternSpacing > MAX_SPACING)
		{
			TrackerSettings::Instance().gnPatternSpacing = MAX_SPACING;
			SetDlgItemInt(IDC_EDIT_SPACING, TrackerSettings::Instance().gnPatternSpacing, FALSE);
		}
		SendViewMessage(VIEWMSG_SETSPACING, TrackerSettings::Instance().gnPatternSpacing);
	}
}


void CCtrlPatterns::OnInstrumentChanged()
{
	int n = m_CbnInstrument.GetCurSel();
	if(n >= 0)
	{
		n = static_cast<int>(m_CbnInstrument.GetItemData(n));
		int nmax = m_sndFile.GetNumInstruments() ? m_sndFile.GetNumInstruments() : m_sndFile.GetNumSamples();
		if((n >= 0) && (n <= nmax) && (n != (int)m_nInstrument))
		{
			m_nInstrument = static_cast<INSTRUMENTINDEX>(n);
			m_parent.InstrumentChanged(m_nInstrument);
		}
		if(m_instrDropdownOpen)
		{
			m_instrDropdownOpen = false;
			SwitchToView();
		}
		::EnableWindow(::GetDlgItem(m_hWnd, IDC_PATINSTROPLUGGUI), HasValidPlug(m_nInstrument));
	}
}


void CCtrlPatterns::OnPrevInstrument()
{
	int n = m_CbnInstrument.GetCount();
	if(n > 0)
	{
		int pos = m_CbnInstrument.GetCurSel();
		if(pos > 0)
			pos--;
		else
			pos = n - 1;
		m_CbnInstrument.SetCurSel(pos);
		OnInstrumentChanged();
		SwitchToView();
	}
}


void CCtrlPatterns::OnNextInstrument()
{
	int n = m_CbnInstrument.GetCount();
	if(n > 0)
	{
		int pos = m_CbnInstrument.GetCurSel() + 1;
		if(pos >= n)
			pos = 0;
		m_CbnInstrument.SetCurSel(pos);
		OnInstrumentChanged();
		SwitchToView();
	}
}


void CCtrlPatterns::OnPlayerPause()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm)
		pMainFrm->PauseMod();
}


void CCtrlPatterns::OnPatternNew()
{
	const auto &order = Order();
	ORDERINDEX curOrd = m_OrderList.GetCurSel(true).firstOrd;
	PATTERNINDEX curPat = (curOrd < order.size()) ? order[curOrd] : 0;
	ROWINDEX rows = 64;
	if(m_sndFile.Patterns.IsValidPat(curPat))
	{
		// Only if the current oder is already occupied, create a new pattern at the next position.
		curOrd++;
	} else
	{
		// Use currently edited pattern for new pattern length
		curPat = static_cast<PATTERNINDEX>(SendViewMessage(VIEWMSG_GETCURRENTPATTERN));
	}
	if(m_sndFile.Patterns.IsValidPat(curPat))
	{
		rows = m_sndFile.Patterns[curPat].GetNumRows();
	}
	rows = Clamp(rows, m_sndFile.GetModSpecifications().patternRowsMin, m_sndFile.GetModSpecifications().patternRowsMax);
	const PATTERNINDEX newPat = m_modDoc.InsertPattern(rows, curOrd);
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
		m_OrderList.Invalidate(FALSE);
		SetCurrentPattern(newPat);
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, PatternHint(newPat).Names(), this);
		m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
		SwitchToView();
	}
}


// Duplicates one or more patterns.
void CCtrlPatterns::OnPatternDuplicate()
{
	OrdSelection selection = m_OrderList.GetCurSel();
	const ORDERINDEX insertFrom = selection.firstOrd;
	const ORDERINDEX insertWhere = selection.lastOrd + 1u;
	if(insertWhere >= m_sndFile.GetModSpecifications().ordersMax)
		return;
	const ORDERINDEX insertCount = std::min(selection.GetSelCount(), static_cast<ORDERINDEX>(m_sndFile.GetModSpecifications().ordersMax - insertWhere));
	if(!insertCount)
		return;

	bool success = false, outOfPatterns = false;
	// Has this pattern been duplicated already? (for multiselect)
	std::vector<PATTERNINDEX> patReplaceIndex(m_sndFile.Patterns.Size(), PATTERNINDEX_INVALID);

	ModSequence &order = Order();
	for(ORDERINDEX i = 0; i < insertCount; i++)
	{
		PATTERNINDEX curPat = order[insertFrom + i];
		if(curPat < patReplaceIndex.size() && patReplaceIndex[curPat] == PATTERNINDEX_INVALID)
		{
			PATTERNINDEX newPat = m_sndFile.Patterns.Duplicate(curPat, true);
			if(newPat != PATTERNINDEX_INVALID)
			{
				order.insert(insertWhere + i, 1, newPat);
				success = true;
				// Mark as duplicated, so if this pattern is to be duplicated again, the same new pattern number is inserted into the order list.
				patReplaceIndex[curPat] = newPat;
			} else
			{
				if(m_sndFile.Patterns.IsValidPat(curPat))
					outOfPatterns = true;
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
				newPat = order[insertFrom + i];
			}

			order.insert(insertWhere + i, 1, newPat);

			success = true;
		}
	}
	if(success)
	{
		m_OrderList.InsertUpdatePlaystate(selection.firstOrd, selection.lastOrd);

		m_OrderList.Invalidate(FALSE);
		m_OrderList.SetCurSel(insertWhere, true, false, true);

		// If the first duplicated order is e.g. a +++ item, we need to move the pattern display on or else we'll still edit the previously shown pattern.
		ORDERINDEX showPattern = std::min(insertWhere, order.GetLastIndex());
		while(!order.IsValidPat(showPattern) && showPattern < order.GetLastIndex())
		{
			showPattern++;
		}
		SetCurrentPattern(order[showPattern]);

		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
		m_modDoc.UpdateAllViews(nullptr, PatternHint(PATTERNINDEX_INVALID).Names(), this);
		if(selection.lastOrd != selection.firstOrd)
			m_OrderList.m_nScrollPos2nd = insertWhere + insertCount - 1u;
	}
	if(outOfPatterns)
	{
		const auto &specs = m_sndFile.GetModSpecifications();
		Reporting::Error(MPT_UFORMAT("Pattern limit of the {} format ({} patterns) has been reached.")(specs.GetFileExtensionUpper(), specs.patternsMax), U_("Duplicate Patterns"));
	}
	SwitchToView();
}


// Merges one or more patterns into a single pattern
void CCtrlPatterns::OnPatternMerge()
{
	const OrdSelection selection = m_OrderList.GetCurSel();
	const ORDERINDEX firstOrder = selection.firstOrd;
	const ORDERINDEX numOrders = selection.GetSelCount();

	// Get the total number of lines to be merged
	std::vector<ModCommand> originalData;
	ROWINDEX numRows = 0, minPatternSize = MAX_PATTERN_ROWS, maxPatternSize = 0;
	ModSequence &order = Order();
	for(ORDERINDEX ord = selection.firstOrd; ord <= selection.lastOrd; ord++)
	{
		const CPattern *pattern = order.PatternAt(ord);
		if(!pattern)
			continue;
		numRows += pattern->GetNumRows();
		minPatternSize = std::min(minPatternSize, pattern->GetNumRows());
		maxPatternSize = std::max(maxPatternSize, pattern->GetNumRows());
		originalData.insert(originalData.end(), pattern->cbegin(), pattern->cend());
	}
	if(!numRows || !numOrders)
	{
		MessageBeep(MB_ICONWARNING);
		SwitchToView();
		return;
	}

	const auto &specs = m_sndFile.GetModSpecifications();
	const mpt::ustring format = specs.GetFileExtensionUpper();

	const ORDERINDEX remainingOrders = order.GetRemainingCapacity(selection.lastOrd + 1) + numOrders;
	ROWINDEX minRows = (numRows + (remainingOrders - 1)) / remainingOrders;
	if(minRows > specs.patternRowsMax)
	{
		Reporting::Error(U_("There are not enough empty orders left to merge the selected patterns into."), U_("Merge Patterns"));
		SwitchToView();
		return;
	}

	const PATTERNINDEX remainingPatterns = m_sndFile.Patterns.GetRemainingCapacity();
	if(!remainingPatterns)
	{
		Reporting::Error(MPT_UFORMAT("Pattern limit of the {} format ({} patterns) has been reached.")(format, specs.patternsMax), U_("Merge Patterns"));
		SwitchToView();
		return;
	}
	minRows = std::max(minRows, (numRows + (remainingPatterns - 1)) / remainingPatterns);
	if(minRows > specs.patternRowsMax)
	{
		Reporting::Error(U_("There are not enough empty patterns left to merge the selected patterns into."), U_("Merge Patterns"));
		SwitchToView();
		return;
	}

	ROWINDEX patternSize = minRows;
	if(minRows != specs.patternRowsMax)
	{
		CInputDlg dlg(this, _T("New pattern length:"), static_cast<int32>(minRows), static_cast<int32>(specs.patternRowsMax), static_cast<int32>(std::clamp(numRows, minRows, specs.patternRowsMax)));
		if(dlg.DoModal() != IDOK)
		{
			SwitchToView();
			return;
		}
		patternSize = static_cast<ROWINDEX>(dlg.resultAsInt);
	}
	if(minPatternSize == maxPatternSize && minPatternSize == patternSize)
	{
		MessageBeep(MB_ICONWARNING);
		SwitchToView();
		return;
	}

	CriticalSection cs;

	const PATTERNINDEX patternsRequired = static_cast<PATTERNINDEX>((numRows + (patternSize - 1)) / patternSize);
	// Double-check if we *still* have enough patterns and orders (right now it should not be possible that this number changed, but in the future it might due to scripting / etc.)
	if(m_sndFile.Patterns.GetRemainingCapacity() < patternsRequired || (order.GetRemainingCapacity(selection.lastOrd + 1) + numOrders) < patternsRequired)
	{
		cs.Leave();
		Reporting::Error(U_("There are not enough empty patterns or order lists for this operation."), U_("Merge Patterns"));
		SwitchToView();
		return;
	}

	order.Remove(selection.firstOrd, selection.lastOrd);
	m_OrderList.DeleteUpdatePlaystate(selection.firstOrd, selection.lastOrd);

	order.insert(firstOrder, patternsRequired, PATTERNINDEX_INVALID);
	m_OrderList.InsertUpdatePlaystate(firstOrder, firstOrder + patternsRequired);

	PATTERNINDEX patternsInserted = 0;
	auto sourceData = originalData.cbegin();
	while(sourceData != originalData.end())
	{
		const ROWINDEX thisPatternSize = std::min(numRows, patternSize);

		const PATTERNINDEX newPat = m_sndFile.Patterns.InsertAny(std::max(thisPatternSize, specs.patternRowsMin), true);
		if(newPat == PATTERNINDEX_INVALID)
			break;

		auto &pattern = m_sndFile.Patterns[newPat];
		auto sourceEnd = sourceData + thisPatternSize * m_sndFile.GetNumChannels();
		std::copy(sourceData, sourceEnd, pattern.begin());
		sourceData = sourceEnd;

		if(pattern.GetNumRows() > thisPatternSize)
			pattern.WriteEffect(EffectWriter(CMD_PATTERNBREAK, 0).Row(thisPatternSize - 1).RetryNextRow());

		order[firstOrder + patternsInserted] = newPat;

		numRows -= thisPatternSize;
		patternsInserted++;
	}

	m_OrderList.Invalidate(FALSE);
	m_OrderList.SetSelection(firstOrder, firstOrder + patternsInserted);
	SetCurrentPattern(order[firstOrder]);

	m_modDoc.SetModified();
	m_modDoc.UpdateAllViews(nullptr, SequenceHint().Data(), this);
	m_modDoc.UpdateAllViews(nullptr, PatternHint(PATTERNINDEX_INVALID).Names(), this);

	SwitchToView();
}


void CCtrlPatterns::OnPatternStop()
{
	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
	if(pMainFrm)
		pMainFrm->PauseMod(&m_modDoc);
	m_sndFile.ResetChannels();
	SwitchToView();
}


void CCtrlPatterns::OnPatternPlay()
{
	if(CMainFrame::GetMainFrame()->GetInputHandler()->ShiftPressed())
		m_modDoc.OnPatternPlayNoLoop();
	else
		m_modDoc.OnPatternPlay();
	SwitchToView();
}


void CCtrlPatterns::OnPatternPlayFromStart()
{
	if(CMainFrame::GetMainFrame()->GetInputHandler()->ShiftPressed())
		m_modDoc.OnPatternRestart(false);
	else
		m_modDoc.OnPatternRestart();
	SwitchToView();
}


void CCtrlPatterns::OnPatternRecord()
{
	m_bRecord = !m_bRecord;
	TrackerSettings::Instance().gbPatternRecord = m_bRecord;
	m_ToolBar.CheckButton(IDC_PATTERN_RECORD, m_bRecord ? TRUE : FALSE);
	SendViewMessage(VIEWMSG_SETRECORD, m_bRecord);
	SwitchToView();
}


void CCtrlPatterns::OnPatternVUMeters()
{
	m_bVUMeters = !m_bVUMeters;
	TrackerSettings::Instance().gbPatternVUMeters = m_bVUMeters;
	m_ToolBar.CheckButton(ID_PATTERN_VUMETERS, m_bVUMeters ? TRUE : FALSE);
	SendViewMessage(VIEWMSG_SETVUMETERS, m_bVUMeters);
	SwitchToView();
}


void CCtrlPatterns::OnPatternViewPlugNames()
{
	m_bPluginNames = !m_bPluginNames;
	TrackerSettings::Instance().gbPatternPluginNames = m_bPluginNames;
	m_ToolBar.CheckButton(ID_VIEWPLUGNAMES, m_bPluginNames ? TRUE : FALSE);
	SendViewMessage(VIEWMSG_SETPLUGINNAMES, m_bPluginNames);
	SwitchToView();
}


void CCtrlPatterns::OnToggleOverflowPaste()
{
	TrackerSettings::Instance().patternSetup ^= PatternSetup::OverflowPaste;
	theApp.PostMessageToAllViews(WM_MOD_CTRLMSG, CTRLMSG_PAT_UPDATE_TOOLBAR);
	SwitchToView();
}


void CCtrlPatterns::OnToggleMetronome()
{
	TrackerSettings::Instance().metronomeEnabled = !TrackerSettings::Instance().metronomeEnabled;
	CMainFrame::GetMainFrame()->UpdateMetronomeSamples();
	theApp.PostMessageToAllViews(WM_MOD_CTRLMSG, CTRLMSG_PAT_UPDATE_TOOLBAR);
	SwitchToView();
}


void CCtrlPatterns::OnMetronomeSettings()
{
	MetronomeSettingsDlg dlg{this};
	dlg.DoModal();
	SwitchToView();
}


void CCtrlPatterns::OnPatternProperties()
{
	SendViewMessage(VIEWMSG_PATTERNPROPERTIES, PATTERNINDEX_INVALID);
	SwitchToView();
}


void CCtrlPatterns::OnPatternExpand()
{
	SendViewMessage(VIEWMSG_EXPANDPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternCopy()
{
	SendViewMessage(VIEWMSG_COPYPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternPaste()
{
	SendViewMessage(VIEWMSG_PASTEPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternShrink()
{
	SendViewMessage(VIEWMSG_SHRINKPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternAmplify()
{
	SendViewMessage(VIEWMSG_AMPLIFYPATTERN);
	SwitchToView();
}


void CCtrlPatterns::OnPatternPlayRow()
{
	::SendMessage(m_hWndView, WM_COMMAND, ID_PATTERN_PLAYROW, 0);
	SwitchToView();
}


void CCtrlPatterns::OnUpdateRecord(CCmdUI *pCmdUI)
{
	if(pCmdUI)
		pCmdUI->SetCheck((m_bRecord) ? TRUE : FALSE);
}


void CCtrlPatterns::OnFollowSong()
{
	SendViewMessage(VIEWMSG_FOLLOWSONG, IsDlgButtonChecked(IDC_PATTERN_FOLLOWSONG));
	SwitchToView();
}


void CCtrlPatterns::OnChangeLoopStatus()
{
	OnModCtrlMsg(CTRLMSG_PAT_LOOP, IsDlgButtonChecked(IDC_PATTERN_LOOP));
	SwitchToView();
}


void CCtrlPatterns::OnEditUndo()
{
	if(m_hWndView)
		::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_UNDO, 0);
	SwitchToView();
}


// cppcheck-suppress duplInheritedMember
void CCtrlPatterns::OnSwitchToView()
{
	PostViewMessage(VIEWMSG_SETFOCUS);
}


void CCtrlPatterns::OnPatternNameChanged()
{
	if(!IsLocked())
	{
		const PATTERNINDEX nPat = (PATTERNINDEX)SendViewMessage(VIEWMSG_GETCURRENTPATTERN);

		CString tmp;
		m_EditPatName.GetWindowText(tmp);
		const std::string s = mpt::ToCharset(m_sndFile.GetCharsetInternal(), tmp);

		if(m_sndFile.Patterns[nPat].GetName() != s)
		{
			if(m_sndFile.Patterns[nPat].SetName(s))
			{
				if(m_sndFile.GetType() & (MOD_TYPE_XM | MOD_TYPE_IT | MOD_TYPE_MPT))
					m_modDoc.SetModified();
				m_modDoc.UpdateAllViews(nullptr, PatternHint(nPat).Names(), this);
			}
		}
	}
}


void CCtrlPatterns::OnSequenceNameChanged()
{
	CString tmp;
	GetDlgItemText(IDC_EDIT_SEQUENCE_NAME, tmp);
	const mpt::ustring str = mpt::ToUnicode(tmp);
	auto &order = Order();
	if(str != order.GetName())
	{
		order.SetName(str);
		m_modDoc.SetModified();
		m_modDoc.UpdateAllViews(nullptr, SequenceHint(m_sndFile.Order.GetCurrentSequenceIndex()).Names(), this);
	}
}


void CCtrlPatterns::OnChordEditor()
{
	CChordEditor dlg(this);
	dlg.DoModal();
	SwitchToView();
}


void CCtrlPatterns::OnTbnDropDownToolBar(NMHDR *pNMHDR, LRESULT *pResult)
{
	CInputHandler *ih = CMainFrame::GetInputHandler();
	NMTOOLBAR *pToolBar = reinterpret_cast<NMTOOLBAR *>(pNMHDR);
	ClientToScreen(&(pToolBar->rcButton));
	const int offset = HighDPISupport::ScalePixels(4, m_hWnd);  // Compared to the main toolbar, the offset seems to be a bit wrong here...?
	int x = pToolBar->rcButton.left + offset, y = pToolBar->rcButton.bottom + offset;
	const auto visibleColumns = std::bitset<PatternCursor::numColumns>{static_cast<unsigned long>(SendViewMessage(VIEWMSG_GETDETAIL))};
	CMenu menu;
	switch(pToolBar->iItem)
	{
	case IDC_PATTERN_NEW:
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, ID_ORDERLIST_COPY, ih->GetKeyTextFromCommand(kcDuplicatePattern, _T("&Duplicate Pattern")));
		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
		menu.DestroyMenu();
		break;

	case IDC_METRONOME:
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, ID_METRONOME_SETTINGS, _T("&Metronome Settings"));
		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
		menu.DestroyMenu();
		break;

	case ID_PATTERNDETAIL_DROPDOWN:
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING | (visibleColumns[PatternCursor::instrColumn] ? MF_CHECKED : 0), ID_PATTERNDETAIL_INSTR, ih->GetKeyTextFromCommand(kcToggleVisibilityInstrColumn, _T("Show &Instrument Column")));
		menu.AppendMenu(MF_STRING | (visibleColumns[PatternCursor::volumeColumn] ? MF_CHECKED : 0), ID_PATTERNDETAIL_VOLUME, ih->GetKeyTextFromCommand(kcToggleVisibilityVolumeColumn, _T("Show &Volume Column")));
		menu.AppendMenu(MF_STRING | (visibleColumns[PatternCursor::effectColumn] ? MF_CHECKED : 0), ID_PATTERNDETAIL_EFFECT, ih->GetKeyTextFromCommand(kcToggleVisibilityEffectColumn, _T("Show &Effect Column")));
		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, this);
		menu.DestroyMenu();
		break;
	}
	*pResult = 0;
}


void CCtrlPatterns::OnDetailSwitch()
{
	// Cycle through all bit combinations
	auto visibleColumns = std::bitset<PatternCursor::numColumns>{static_cast<unsigned long>(SendViewMessage(VIEWMSG_GETDETAIL) + 1)};
	visibleColumns.set(PatternCursor::noteColumn);
	visibleColumns.set(PatternCursor::paramColumn, visibleColumns[PatternCursor::effectColumn]);
	SendViewMessage(VIEWMSG_SETDETAIL, visibleColumns.to_ulong());
	SwitchToView();
}


void CCtrlPatterns::OnDetailInstr()
{
	auto visibleColumns = std::bitset<PatternCursor::numColumns>{static_cast<unsigned long>(SendViewMessage(VIEWMSG_GETDETAIL))};
	visibleColumns.flip(PatternCursor::instrColumn);
	SendViewMessage(VIEWMSG_SETDETAIL, visibleColumns.to_ulong());
	SwitchToView();
}


void CCtrlPatterns::OnDetailVolume()
{
	auto visibleColumns = std::bitset<PatternCursor::numColumns>{static_cast<unsigned long>(SendViewMessage(VIEWMSG_GETDETAIL))};
	visibleColumns.flip(PatternCursor::volumeColumn);
	SendViewMessage(VIEWMSG_SETDETAIL, visibleColumns.to_ulong());
	SwitchToView();
}


void CCtrlPatterns::OnDetailEffect()
{
	auto visibleColumns = std::bitset<PatternCursor::numColumns>{static_cast<unsigned long>(SendViewMessage(VIEWMSG_GETDETAIL))};
	visibleColumns.flip(PatternCursor::effectColumn);
	visibleColumns.flip(PatternCursor::paramColumn);
	SendViewMessage(VIEWMSG_SETDETAIL, visibleColumns.to_ulong());
	SwitchToView();
}


void CCtrlPatterns::TogglePluginEditor()
{
	if(m_sndFile.GetInstrumentPlugin(m_nInstrument) != nullptr)
	{
		m_modDoc.TogglePluginEditor(m_sndFile.Instruments[m_nInstrument]->nMixPlug - 1, CInputHandler::ShiftPressed());
	}
}


bool CCtrlPatterns::HasValidPlug(INSTRUMENTINDEX instr) const
{
	return m_sndFile.GetInstrumentPlugin(instr) != nullptr;
}


BOOL CCtrlPatterns::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if(nFlags == 0)
	{
		PostViewMessage(VIEWMSG_DOSCROLL, zDelta);
	}
	return CModControlDlg::OnMouseWheel(nFlags, zDelta, pt);
}


void CCtrlPatterns::OnXButtonUp(UINT nFlags, UINT nButton, CPoint point)
{
	if(nButton == XBUTTON1)
		OnModCtrlMsg(CTRLMSG_PREVORDER, 0);
	else if(nButton == XBUTTON2)
		OnModCtrlMsg(CTRLMSG_NEXTORDER, 0);
	CModControlDlg::OnXButtonUp(nFlags, nButton, point);
}


CString CCtrlPatterns::GetToolTipText(UINT id, HWND) const
{
	CString s;
	CommandID cmd = kcNull;
	switch(id)
	{
	case IDC_PATTERN_NEW: s = _T("Insert Pattern"); cmd = kcNewPattern; break;
	case IDC_PATTERN_PLAY: s = _T("Play Pattern (Shift-click to play song from cursor)"); cmd = kcPlayPatternFromCursor; break;
	case IDC_PATTERN_PLAYFROMSTART: s = _T("Replay Pattern (Shift-click to play song from start of pattern)"); cmd = kcPlayPatternFromStart; break;
	case IDC_PATTERN_STOP: s = _T("Stop"); cmd = kcPauseSong; break;
	case ID_PATTERN_PLAYROW: s = _T("Play Row"); cmd = kcPatternPlayRow; break;
	case IDC_PATTERN_RECORD: s = _T("Record"); cmd = kcPatternRecord; break;
	case IDC_METRONOME: s = _T("Metronome"); cmd = kcToggleMetronome; break;
	case ID_PATTERN_VUMETERS: s = _T("VU-Meters"); break;
	case ID_VIEWPLUGNAMES: s = _T("Show Plugins"); break;
	case ID_PATTERN_CHANNELMANAGER: s = _T("Channel Manager"); cmd = kcViewChannelManager; break;
	case ID_PATTERN_MIDIMACRO: s = _T("Zxx Macro Configuration"); cmd = kcShowMacroConfig; break;
	case ID_PATTERN_CHORDEDIT: s = _T("Chord Editor"); cmd = kcChordEditor; break;
	case ID_EDIT_UNDO:
		s = _T("Undo");
		if(m_modDoc.GetPatternUndo().CanUndo())
			s += _T(" ") + m_modDoc.GetPatternUndo().GetUndoName();
		cmd = kcEditUndo;
		break;
	case ID_PATTERN_PROPERTIES: s = _T("Pattern Properties"); cmd = kcShowPatternProperties; break;
	case ID_PATTERN_EXPAND: s = _T("Expand Pattern"); break;
	case ID_PATTERN_SHRINK: s = _T("Shrink Pattern"); break;
	case ID_PATTERNDETAIL_DROPDOWN: s = _T("Change Pattern Detail Level"); break;
	case ID_OVERFLOWPASTE: s = _T("Toggle Overflow Paste"); cmd = kcToggleOverflowPaste; break;
	case IDC_PATTERN_LOOP: s = _T("Toggle Loop Pattern"); cmd = kcChangeLoopStatus; break;
	case IDC_PATTERN_FOLLOWSONG: s = _T("Toggle Follow Song"); cmd = kcToggleFollowSong; break;

	case IDC_SPIN_SEQNUM:
	case IDC_EDIT_SEQNUM:
	case IDC_EDIT_SEQUENCE_NAME:
		if(!GetDlgItem(id)->IsWindowEnabled())
			s = _T("Multiple sequences are only supported in the MPTM format.");
		break;
	}

	if(cmd != kcNull)
	{
		auto keyText = CMainFrame::GetInputHandler()->m_activeCommandSet->GetKeyTextFromCommand(cmd, 0);
		if(!keyText.IsEmpty())
			s += MPT_CFORMAT(" ({})")(keyText);
	}
	return s;
}


void CCtrlPatterns::OnSequenceNumChanged()
{
	if((m_EditSequence.m_hWnd) && (m_EditSequence.GetWindowTextLength() > 0))
	{
		SEQUENCEINDEX newSeq = static_cast<SEQUENCEINDEX>(GetDlgItemInt(IDC_EDIT_SEQNUM) - 1);

		if(newSeq == m_sndFile.Order.GetCurrentSequenceIndex())
			return;

		if(newSeq >= m_sndFile.Order.GetNumSequences())
		{
			newSeq = m_sndFile.Order.GetNumSequences() - 1;
			SetDlgItemInt(IDC_EDIT_SEQNUM, newSeq + 1, FALSE);
		}
		m_OrderList.SelectSequence(newSeq);
		UpdateView(SequenceHint(newSeq).Names(), nullptr);
	}
}

OPENMPT_NAMESPACE_END
