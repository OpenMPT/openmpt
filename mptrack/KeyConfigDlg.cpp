/*
 * KeyConfigDlg.cpp
 * ----------------
 * Purpose: Implementation of OpenMPT's keyboard configuration dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "KeyConfigDlg.h"
#include "CListCtrl.h"
#include "FileDialog.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "InputHandler.h"
#include "Reporting.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "WindowMessages.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/MIDIEvents.h"


OPENMPT_NAMESPACE_BEGIN


//***************************************************************************************//
// CCustEdit: customised CEdit control to catch keypresses.
// (does what CHotKeyCtrl does,but better)
//***************************************************************************************//

BEGIN_MESSAGE_MAP(CCustEdit, CEdit)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_MESSAGE(WM_MOD_MIDIMSG, &CCustEdit::OnMidiMsg)
END_MESSAGE_MAP()


LRESULT CCustEdit::OnMidiMsg(WPARAM dwMidiDataParam, LPARAM)
{
	if(!m_isFocussed)
		return 1;

	uint32 midiData = static_cast<uint32>(dwMidiDataParam);
	const auto byte1 = MIDIEvents::GetDataByte1FromEvent(midiData), byte2 = MIDIEvents::GetDataByte2FromEvent(midiData);
	switch(MIDIEvents::GetTypeFromEvent(midiData))
	{
	case MIDIEvents::evControllerChange:
		if(byte2 != 0)
		{
			SetKey(ModMidi, byte1);
			m_pOptKeyDlg->OnSetKeyChoice(this);
		}
		break;

	case MIDIEvents::evNoteOn:
	case MIDIEvents::evNoteOff:
		SetKey(ModMidi, byte1 | 0x80);
		m_pOptKeyDlg->OnSetKeyChoice(this);
		break;

	default:
		break;
	}

	return 1;
}


BOOL CCustEdit::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg && !m_bypassed)
	{
		if(pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
		{
			SetKey(CInputHandler::GetModifierMask(), static_cast<UINT>(pMsg->wParam));
			return -1;  // Keypress handled, don't pass on message.
		} else if(pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP)
		{
			//if a key has been released but custom edit box is empty, we have probably just
			//navigated into the box with TAB or SHIFT-TAB. No need to set keychoice.
			if(code != 0)
				m_pOptKeyDlg->OnSetKeyChoice(this);
		}
	}
	return CEdit::PreTranslateMessage(pMsg);
}


void CCustEdit::SetKey(FlagSet<Modifiers> inMod, UINT inCode)
{
	mod = inMod;
	code = inCode;
	//Setup display
	SetWindowText(KeyCombination::GetKeyText(mod, code));
}


void CCustEdit::OnSetFocus(CWnd *pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);
	// Lock the input handler
	CMainFrame::GetInputHandler()->Bypass(true);
	// Accept MIDI input
	CMainFrame::GetMainFrame()->SetMidiRecordWnd(m_hWnd);

	m_isFocussed = true;
}


void CCustEdit::OnKillFocus(CWnd *pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	//unlock the input handler
	CMainFrame::GetInputHandler()->Bypass(false);
	m_isFocussed = false;
	m_pOptKeyDlg->OnCancelKeyChoice(this);
}


//***************************************************************************************//
// COptionsKeyboard:
//
//***************************************************************************************//

static constexpr CListCtrlEx::Header KeyListHeaders[] =
{
	{_T("Shortcut"),      274, LVCFMT_LEFT},
	{_T("Assigned Keys"), 174, LVCFMT_LEFT},
};

BEGIN_MESSAGE_MAP(COptionsKeyboard, CPropertyPage)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &COptionsKeyboard::OnDPIChangedAfterParent)
	ON_LBN_SELCHANGE(IDC_CHOICECOMBO,     &COptionsKeyboard::OnKeyChoiceSelect)
	ON_LBN_SELCHANGE(IDC_KEYCATEGORY,     &COptionsKeyboard::OnCategorySelChanged)
	ON_EN_UPDATE(IDC_CHORDDETECTWAITTIME, &COptionsKeyboard::OnChordWaitTimeChanged)
	ON_COMMAND(IDC_BUTTON3,               &COptionsKeyboard::OnClearSearch)
	ON_COMMAND(IDC_BUTTON2,               &COptionsKeyboard::OnEnableFindHotKey)
	ON_COMMAND(IDC_BUTTON1,               &COptionsKeyboard::OnListenForKeys)
	ON_COMMAND(IDC_DELETE,                &COptionsKeyboard::OnDeleteKeyChoice)
	ON_COMMAND(IDC_RESTORE,               &COptionsKeyboard::OnRestoreKeyChoice)
	ON_COMMAND(IDC_LOAD,                  &COptionsKeyboard::OnLoad)
	ON_COMMAND(IDC_SAVE,                  &COptionsKeyboard::OnSave)
	ON_COMMAND(IDC_CHECKKEYDOWN,          &COptionsKeyboard::OnCheck)
	ON_COMMAND(IDC_CHECKKEYHOLD,          &COptionsKeyboard::OnCheck)
	ON_COMMAND(IDC_CHECKKEYUP,            &COptionsKeyboard::OnCheck)
	ON_COMMAND(IDC_NOTESREPEAT,           &COptionsKeyboard::OnToggleNotesRepeat)
	ON_COMMAND(IDC_RESTORE_KEYMAP,        &COptionsKeyboard::OnRestoreDefaultKeymap)
	ON_COMMAND(ID_KEYPRESET_MPT,          &COptionsKeyboard::OnRestoreMPTKeymap)
	ON_COMMAND(ID_KEYPRESET_IT,           &COptionsKeyboard::OnRestoreITKeymap)
	ON_COMMAND(ID_KEYPRESET_FT2,          &COptionsKeyboard::OnRestoreFT2Keymap)
	ON_EN_CHANGE(IDC_FIND,                &COptionsKeyboard::OnSearchTermChanged)
	ON_EN_SETFOCUS(IDC_FINDHOTKEY,        &COptionsKeyboard::OnClearHotKey)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_COMMAND_LIST, &COptionsKeyboard::OnCommandKeySelChanged)
	ON_NOTIFY(NM_DBLCLK,       IDC_COMMAND_LIST, &COptionsKeyboard::OnListenForKeysFromList)
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	ON_NOTIFY(BCN_DROPDOWN, IDC_RESTORE_KEYMAP, &COptionsKeyboard::OnRestoreKeymapDropdown)
#endif
END_MESSAGE_MAP()


void COptionsKeyboard::DoDataExchange(CDataExchange *pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_KEYCATEGORY, m_cmbCategory);
	DDX_Control(pDX, IDC_COMMAND_LIST, m_lbnCommandKeys);
	DDX_Control(pDX, IDC_CHOICECOMBO, m_cmbKeyChoice);
	DDX_Control(pDX, IDC_CHORDDETECTWAITTIME, m_eChordWaitTime);
	DDX_Control(pDX, IDC_CUSTHOTKEY, m_eCustHotKey);
	DDX_Control(pDX, IDC_FINDHOTKEY, m_eFindHotKey);
	DDX_Control(pDX, IDC_CHECKKEYDOWN, m_bKeyDown);
	DDX_Control(pDX, IDC_CHECKKEYHOLD, m_bKeyHold);
	DDX_Control(pDX, IDC_CHECKKEYUP, m_bKeyUp);
	DDX_Control(pDX, IDC_FIND, m_eFind);
	DDX_Control(pDX, IDC_STATIC1, m_warnIconCtl);
	DDX_Control(pDX, IDC_KEYREPORT, m_warnText);
	DDX_Control(pDX, IDC_RESTORE_KEYMAP, m_restoreDefaultButton);
}


COptionsKeyboard::COptionsKeyboard() : CPropertyPage{IDD_OPTIONS_KEYBOARD} { }


BOOL COptionsKeyboard::OnSetActive()
{
	CMainFrame::m_nLastOptionsPage = OPTIONS_PAGE_KEYBOARD;
	return CPropertyPage::OnSetActive();
}


BOOL COptionsKeyboard::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	m_fullPathName = TrackerSettings::Instance().m_szKbdFile;

	m_localCmdSet = std::make_unique<CCommandSet>();
	m_localCmdSet->Copy(*CMainFrame::GetInputHandler()->m_activeCommandSet);

	m_lbnCommandKeys.SetExtendedStyle(m_lbnCommandKeys.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_listGrouped = m_lbnCommandKeys.EnableGroupView();
	m_lbnCommandKeys.SetHeaders(KeyListHeaders);

	//Fill category combo and automatically selects first category
	DefineCommandCategories();
	for(size_t c = 0; c < commandCategories.size(); c++)
	{
		if(commandCategories[c].name && !commandCategories[c].commandRanges.empty())
			m_cmbCategory.SetItemData(m_cmbCategory.AddString(commandCategories[c].name), c);
	}
	m_cmbCategory.SetCurSel(0);
	UpdateDialog();

#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	m_restoreDefaultButton.ModifyStyle(0, BS_SPLITBUTTON);
#endif

	m_eCustHotKey.SetOwner(*this);
	m_eFindHotKey.SetOwner(*this);

	EnableKeyChoice(false);

	m_eChordWaitTime.SetWindowText(mpt::cfmt::val(TrackerSettings::Instance().gnAutoChordWaitTime.Get()));
	return TRUE;
}


LRESULT COptionsKeyboard::OnDPIChangedAfterParent(WPARAM, LPARAM)
{
	auto result = Default();

	m_lbnCommandKeys.SetColumnWidths(KeyListHeaders);

	if(m_warnIcon)
	{
		DestroyIcon(m_warnIcon);
		m_warnIcon = nullptr;
	}
	if(m_infoIcon)
	{
		DestroyIcon(m_infoIcon);
		m_infoIcon = nullptr;
	}
	UpdateWarning(m_lastWarning);

	return result;
}


void COptionsKeyboard::DefineCommandCategories()
{
	{
		auto &commands = commandCategories.emplace_back(_T("Global"), kCtxAllContexts).commandRanges;
		commands.emplace_back(kcStartFile, kcEndFile, _T("File"));
		commands.emplace_back(kcStartPlayCommands, kcEndPlayCommands, _T("Player"));
		commands.emplace_back(kcStartEditCommands, kcEndEditCommands, _T("Edit"));
		commands.emplace_back(kcStartView, kcEndView, _T("View"));
		commands.emplace_back(kcStartMisc, kcEndMisc, _T("Miscellaneous"));
		commands.emplace_back(kcDummyShortcut, kcDummyShortcut, _T(""));
	}

	commandCategories.emplace_back(_T("  General [Top]"), kCtxCtrlGeneral);
	commandCategories.emplace_back(_T("  General [Bottom]"), kCtxViewGeneral);
	commandCategories.emplace_back(_T("  Pattern Editor [Top]"), kCtxCtrlPatterns);

	{
		auto &commands = commandCategories.emplace_back(_T("  Pattern Editor - Order List"), kCtxCtrlOrderlist).commandRanges;
		commands.emplace_back(kcStartOrderlistEdit, kcEndOrderlistEdit, _T("Edit"));
		commands.emplace_back(kcStartOrderlistNavigation, kcEndOrderlistNavigation, _T("Navigation"));
		commands.emplace_back(kcStartOrderlistNum, kcEndOrderlistNum, _T("Pattern Entry"));
		commands.emplace_back(kcStartOrderlistMisc, kcEndOrderlistMisc, _T("Miscellaneous"));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("  Pattern Editor - Quick Channel Settings"), kCtxChannelSettings).commandRanges;
		commands.emplace_back(kcStartChnSettingsCommands, kcEndChnSettingsCommands, _T(""));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("    Pattern Editor - General"), kCtxViewPatterns).commandRanges;
		commands.emplace_back(kcStartPlainNavigate, kcEndPlainNavigate, _T("Navigation"));
		commands.emplace_back(kcStartJumpSnap, kcEndJumpSnap, _T("Jump"));
		commands.emplace_back(kcStartHomeEnd, kcEndHomeEnd, _T("Go To"));
		commands.emplace_back(kcStartGotoColumn, kcEndGotoColumn, _T("Go To Column"));
		commands.emplace_back(kcPrevPattern, kcNextSequence, _T("Order List Navigation"));
		commands.emplace_back(kcStartPatternScrolling, kcEndPatternScrolling, _T("Scrolling"));
		commands.emplace_back(kcStartSelect, kcEndSelect, _T("Selection"));
		commands.emplace_back(kcStartPatternClipboard, kcEndPatternClipboard, _T("Clipboard"));
		commands.emplace_back(kcClearRow, kcInsertWholeRowGlobal, _T("Clear / Insert"));
		commands.emplace_back(kcStartChannelKeys, kcEndChannelKeys, _T("Channels"));
		commands.emplace_back(kcBeginTranspose, kcEndTranspose, _T("Transpose"));
		commands.emplace_back(kcStartPatternEditMisc, kcEndPatternEditMisc, _T("Edit"));
		commands.emplace_back(kcStartPatternMisc, kcEndPatternMisc, _T("Miscellaneous"));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("        Pattern Editor - Note Column"), kCtxViewPatternsNote).commandRanges;
		commands.emplace_back(kcVPStartNotes, kcVPEndNotes, _T("Note Entry"));
		commands.emplace_back(kcSetOctave0, kcSetOctave9, _T("Octave Entry"));
		commands.emplace_back(kcStartNoteMisc, kcEndNoteMisc, _T("Miscellaneous"));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("        Pattern Editor - Instrument Column"), kCtxViewPatternsIns).commandRanges;
		commands.emplace_back(kcSetIns0, kcSetIns9, _T("Instrument Entry"));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("        Pattern Editor - Volume Column"), kCtxViewPatternsVol).commandRanges;
		commands.emplace_back(kcStartVolumeDigits, kcEndVolumeDigits, _T("Volume Entry"));
		commands.emplace_back(kcStartVolumeCommands, kcEndVolumeCommands, _T("Volume Command Entry"));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("        Pattern Editor - Effect Column"), kCtxViewPatternsFX).commandRanges;
		commands.emplace_back(kcSetFXStart, kcSetFXEnd, _T("Effect Command Entry"));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("        Pattern Editor - Effect Parameter Column"), kCtxViewPatternsFXparam).commandRanges;
		commands.emplace_back(kcSetFXParam0, kcSetFXParamF, _T("Parameter Digit Entry"));
	}

	commandCategories.emplace_back(_T("  Sample [Top]"), kCtxCtrlSamples);

	{
		auto &commands = commandCategories.emplace_back(_T("    Sample Editor"), kCtxViewSamples).commandRanges;
		commands.emplace_back(kcStartSampleEditing, kcEndSampleEditing, _T("Edit"));
		commands.emplace_back(kcStartSampleMisc, kcEndSampleMisc, _T("Miscellaneous"));
		commands.emplace_back(kcStartSampleCues, kcEndSampleCueGroup, _T("Sample Cues"));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("  Instrument Editor"), kCtxCtrlInstruments).commandRanges;
		commands.emplace_back(kcStartInstrumentCtrl, kcEndInstrumentCtrl, _T(""));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("    Envelope Editor"), kCtxViewInstruments).commandRanges;
		commands.emplace_back(kcStartInsEnvelopeEdit, kcEndInsEnvelopeEdit, _T(""));
	}

	commandCategories.emplace_back(_T("  Comments [Top]"), kCtxCtrlComments);

	{
		auto &commands = commandCategories.emplace_back(_T("  Comments [Bottom]"), kCtxViewComments).commandRanges;
		commands.emplace_back(kcStartCommentsCommands, kcEndCommentsCommands, _T(""));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("  Plugin Editor"), kCtxVSTGUI).commandRanges;
		commands.emplace_back(kcStartVSTGUICommands, kcEndVSTGUICommands, _T(""));
	}

	{
		auto &commands = commandCategories.emplace_back(_T("  Tree View"), kCtxViewTree).commandRanges;
		commands.emplace_back(kcStartTreeViewCommands, kcEndTreeViewCommands, _T(""));
	}
}


// Pure GUI methods

void COptionsKeyboard::UpdateDialog()
{
	OnCategorySelChanged();    // Fills command list and automatically selects first command.
	OnCommandKeySelChanged();  // Fills command key choice list for that command and automatically selects first choice.
}


void COptionsKeyboard::OnKeyboardChanged()
{
	OnSettingsChanged();
	UpdateDialog();
}


void COptionsKeyboard::OnCategorySelChanged()
{
	LockControls();
	int cat = static_cast<int>(m_cmbCategory.GetItemData(m_cmbCategory.GetCurSel()));
	if(cat < 0)
		return;

	const bool refresh = cat != m_curCategory || m_eFind.GetWindowTextLength() > 0 || m_eFindHotKey.GetWindowTextLength() > 0;
	m_eFind.SetWindowText(_T(""));
	OnClearHotKey();
	if(refresh)
	{
		// Changed category
		UpdateShortcutList(cat);
	}
	UnlockControls();
}


// Force last active category to be selected in dropdown menu.
void COptionsKeyboard::UpdateCategory()
{
	for(int i = 0; i < m_cmbCategory.GetCount(); i++)
	{
		if(static_cast<int>(m_cmbCategory.GetItemData(i)) == m_curCategory)
		{
			m_cmbCategory.SetCurSel(i);
			break;
		}
	}
}

void COptionsKeyboard::OnSearchTermChanged()
{
	if(IsLocked())
		return;

	CString findString;
	m_eFind.GetWindowText(findString);

	if(findString.IsEmpty())
	{
		UpdateCategory();
	}
	UpdateShortcutList(findString.IsEmpty() ? m_curCategory : -1);
}


void COptionsKeyboard::OnClearSearch()
{
	m_eFindHotKey.SetKey(ModNone, 0);
	m_eFind.SetWindowText(_T(""));
}


void COptionsKeyboard::OnEnableFindHotKey()
{
	OnClearHotKey();
	GetDlgItem(IDC_BUTTON2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_FINDHOTKEY_LABEL)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_FINDHOTKEY)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_FINDHOTKEY)->SetFocus();
}


void COptionsKeyboard::OnFindHotKey()
{
	GetDlgItem(IDC_BUTTON2)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_FINDHOTKEY_LABEL)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_FINDHOTKEY)->ShowWindow(SW_HIDE);
	const bool hasKey = m_eFindHotKey.HasKey();
	if(!hasKey)
		UpdateCategory();
	UpdateShortcutList(hasKey ? -1 : m_curCategory);
	m_lbnCommandKeys.SetFocus();
}


void COptionsKeyboard::OnClearHotKey()
{
	// Focus key search: Clear input
	m_eFindHotKey.SetKey(ModNone, 0);
}


void COptionsKeyboard::InsertGroup(const TCHAR *title, int groupId)
{
	LVGROUP group{};
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	group.cbSize = LVGROUP_V5_SIZE;
#else
	group.cbSize = sizeof(group);
#endif
	group.mask = LVGF_HEADER | LVGF_GROUPID;
#if defined(UNICODE)
	group.pszHeader = const_cast<TCHAR *>(title);
#else
	std::wstring titlew = mpt::ToWide(mpt::winstring(title));
	group.pszHeader = const_cast<WCHAR *>(titlew.c_str());
#endif
	group.cchHeader = 0;
	group.pszFooter = nullptr;
	group.cchFooter = 0;
	group.iGroupId = groupId;
	group.stateMask = 0;
	group.state = 0;
	group.uAlign = LVGA_HEADER_LEFT;
	ListView_InsertGroup(m_lbnCommandKeys.m_hWnd, -1, &group);
	ListView_SetGroupState(m_lbnCommandKeys.m_hWnd, group.iGroupId, LVGS_COLLAPSIBLE, LVGS_COLLAPSIBLE);
}


// Fills command list and automatically selects first command.
void COptionsKeyboard::UpdateShortcutList(int category)
{
	CString findString;
	m_eFind.GetWindowText(findString);
	findString.MakeLower();

	const bool searchByName = !findString.IsEmpty(), searchByKey = m_eFindHotKey.HasKey();
	const bool doSearch = (searchByName || searchByKey);

	int firstCat = category, lastCat = category;
	if(category == -1)
	{
		// We will search in all categories
		firstCat = 0;
		lastCat = static_cast<int>(commandCategories.size()) - 1;
	}

	const auto curSelection = m_lbnCommandKeys.GetSelectionMark();
	CommandID curCommand = (curSelection >= 0) ? static_cast<CommandID>(m_lbnCommandKeys.GetItemData(curSelection)) : kcNull;
	m_lbnCommandKeys.SetRedraw(FALSE);
	m_lbnCommandKeys.DeleteAllItems();
	if(m_listGrouped)
	{
		ListView_RemoveAllGroups(m_lbnCommandKeys);
		m_lbnCommandKeys.EnableGroupView();
	}

	int currentGroup = -1;
	int itemID = -1;

	for(int cat = firstCat; cat <= lastCat; cat++)
	{
		// When searching, we also add the category names to the list.
		bool addCategoryName = (firstCat != lastCat);

		for(const auto &range : commandCategories[cat].commandRanges)
		{
			for(CommandID com = range.first; com <= range.last; com = static_cast<CommandID>(com + 1))
			{
				CString cmdText = m_localCmdSet->GetCommandText(com);
				bool addKey = true;

				if(searchByKey)
				{
					addKey = false;
					for(const KeyCombination &kc : m_localCmdSet->GetKeyChoices(com))
					{
						if(kc.KeyCode() == m_eFindHotKey.code && kc.Modifier() == m_eFindHotKey.mod)
						{
							addKey = true;
							break;
						}
					}
				}
				if(searchByName && addKey)
				{
					addKey = (cmdText.MakeLower().Find(findString) >= 0);
				}

				if(!addKey)
					continue;

				m_curCategory = cat;

				LVITEM lvi;
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				if(m_listGrouped)
					lvi.mask |= LVIF_GROUPID;
				lvi.iSubItem = 0;
				lvi.state = 0;
				lvi.stateMask = 0;
				lvi.cchTextMax = 0;
				lvi.iImage = 0;
				lvi.iIndent = 0;
				lvi.iGroupId = 0;

				if(com == range.first && !doSearch)
				{
					if(m_listGrouped)
					{
						InsertGroup(range.name, ++currentGroup);
					} else
					{
						lvi.iItem = ++itemID;
						lvi.lParam = kcNull;
						CString catName;
						if(range.name.IsEmpty())
							catName = const_cast<TCHAR *>(_T("------------------------------------------------------"));
						else
							catName = _T("------ ") + range.name + _T(" ------");
						lvi.pszText = const_cast<TCHAR *>(catName.GetString());
						m_lbnCommandKeys.InsertItem(&lvi);
					}
				}

				if(!m_localCmdSet->IsHidden(com))
				{
					if(doSearch && addCategoryName)
					{
						if(m_listGrouped)
						{
							InsertGroup(CString{commandCategories[cat].name}.TrimLeft(), ++currentGroup);
						} else
						{
							const CString catName = _T("------ ") + CString{commandCategories[cat].name}.TrimLeft() + _T(" ------");
							lvi.iItem = ++itemID;
							lvi.lParam = kcNull;
							lvi.pszText = const_cast<TCHAR *>(catName.GetString());
							m_lbnCommandKeys.InsertItem(&lvi);
						}
						addCategoryName = false;
					} else if(currentGroup == -1 && m_listGrouped)
					{
						InsertGroup(_T(""), ++currentGroup);
					}

					const CString text = m_localCmdSet->GetCommandText(com);
					lvi.iItem = ++itemID;
					lvi.lParam = static_cast<LPARAM>(com);
					lvi.pszText = const_cast<TCHAR *>(text.GetString());
					lvi.iGroupId = currentGroup;
					m_lbnCommandKeys.InsertItem(&lvi);
					m_lbnCommandKeys.SetItemText(itemID, 1, m_localCmdSet->GetKeyTextFromCommand(com));

					if(curCommand == com)
					{
						// Keep selection on previously selected string
						m_lbnCommandKeys.SetSelectionMark(itemID);
					}
				}
			}
		}
	}

	if(m_lbnCommandKeys.GetSelectionMark() == -1)
	{
		m_lbnCommandKeys.SetSelectionMark(0);
	}
	m_lbnCommandKeys.SetRedraw(TRUE);
	OnCommandKeySelChanged();
}


// Fills  key choice list and automatically selects first key choice
void COptionsKeyboard::OnCommandKeySelChanged(NMHDR *pNMHDR, LRESULT *)
{
	int selectedItem;
	if(pNMHDR)
	{
		auto hdr = reinterpret_cast<const NMLISTVIEW *>(pNMHDR);
		if(!(hdr->uNewState & LVIS_SELECTED))
			return;
		selectedItem = hdr->iItem;
	} else
	{
		selectedItem = m_lbnCommandKeys.GetSelectionMark();
	}

	const CommandID cmd = (selectedItem >= 0) ? static_cast<CommandID>(m_lbnCommandKeys.GetItemData(selectedItem)) : kcNull;
	CString str;

	EnableKeyChoice(false);

	BOOL enableButton = (cmd != kcNull) ? TRUE : FALSE;
	BOOL enableCheckBoxes = (cmd != kcNull && m_localCmdSet->GetKeyListSize(cmd) > 0) ? TRUE : FALSE;
	GetDlgItem(IDC_BUTTON1)->EnableWindow(enableButton);
	GetDlgItem(IDC_DELETE)->EnableWindow(enableButton);
	GetDlgItem(IDC_RESTORE)->EnableWindow(enableButton);
	m_cmbKeyChoice.EnableWindow(enableButton);
	m_bKeyDown.EnableWindow(enableCheckBoxes);
	m_bKeyHold.EnableWindow(enableCheckBoxes);
	m_bKeyUp.EnableWindow(enableCheckBoxes);

	//Separator
	if(cmd == kcNull)
	{
		m_cmbKeyChoice.SetWindowText(_T(""));
		m_eCustHotKey.SetWindowText(_T(""));
		m_bKeyDown.SetCheck(BST_UNCHECKED);
		m_bKeyHold.SetCheck(BST_UNCHECKED);
		m_bKeyUp.SetCheck(BST_UNCHECKED);
		m_curCommand = kcNull;

		GetDlgItem(IDC_GROUPBOX_KEYSETUP)->SetWindowText(_T("&Key setup for selected command"));
		UpdateWarning();
	}

	//Fill "choice" list
	else if((cmd >= kcFirst && cmd != m_curCommand) || m_forceUpdate)  // Have we changed command?
	{
		GetDlgItem(IDC_GROUPBOX_KEYSETUP)->SetWindowText(_T("&Key setup for ") + m_localCmdSet->GetCommandText(cmd));

		m_forceUpdate = false;

		m_curCommand = cmd;
		m_curCategory = GetCategoryFromCommandID(cmd);

		m_cmbKeyChoice.ResetContent();
		int numChoices = m_localCmdSet->GetKeyListSize(cmd);
		if(cmd >= kcFirst && cmd < kcNumCommands && numChoices > 0)
		{
			for(int i = 0; i < numChoices; i++)
			{
				CString s = MPT_CFORMAT("Choice {} (of {})")(i + 1, numChoices);
				m_cmbKeyChoice.SetItemData(m_cmbKeyChoice.AddString(s), i);
			}
		}
		m_cmbKeyChoice.SetItemData(m_cmbKeyChoice.AddString(_T("<new>")), numChoices);
		m_cmbKeyChoice.SetCurSel(0);
		m_curKeyChoice = -1;
		OnKeyChoiceSelect();
	}
}


void COptionsKeyboard::OnListenForKeysFromList(NMHDR *pNMHDR, LRESULT *)
{
	auto hdr = reinterpret_cast<const NMLISTVIEW *>(pNMHDR);
	if(m_curCommand != kcNull && hdr->iSubItem == 1)
		OnListenForKeys();
}


//Fills or clears key choice info
void COptionsKeyboard::OnKeyChoiceSelect()
{
	EnableKeyChoice(false);

	int choice = static_cast<int>(m_cmbKeyChoice.GetItemData(m_cmbKeyChoice.GetCurSel()));
	CommandID cmd = m_curCommand;

	//If nothing there, clear
	if(cmd == kcNull || choice >= m_localCmdSet->GetKeyListSize(cmd) || choice < 0)
	{
		UpdateWarning();
		m_curKeyChoice = choice;
		m_forceUpdate = true;
		m_eCustHotKey.SetKey(ModNone, 0);
		m_bKeyDown.SetCheck(BST_UNCHECKED);
		m_bKeyHold.SetCheck(BST_UNCHECKED);
		m_bKeyUp.SetCheck(BST_UNCHECKED);
		return;
	}

	//else, if changed, Fill
	if(choice != m_curKeyChoice || m_forceUpdate)
	{
		m_curKeyChoice = choice;
		m_forceUpdate = false;
		KeyCombination kc = m_localCmdSet->GetKey(cmd, choice);
		m_eCustHotKey.SetKey(kc.Modifier(), kc.KeyCode());

		m_bKeyDown.SetCheck((kc.EventType() & kKeyEventDown) ? BST_CHECKED : BST_UNCHECKED);
		m_bKeyHold.SetCheck((kc.EventType() & kKeyEventRepeat) ? BST_CHECKED : BST_UNCHECKED);
		m_bKeyUp.SetCheck((kc.EventType() & kKeyEventUp) ? BST_CHECKED : BST_UNCHECKED);

		if(auto conflictCmd = m_localCmdSet->IsConflicting(kc, cmd, true, false); conflictCmd.first != kcNull
		   && conflictCmd.first != cmd)
		{
			UpdateWarning(m_localCmdSet->FormatConflict(kc, conflictCmd.first, conflictCmd.second));
		} else
		{
			UpdateWarning();
		}
	}
}

void COptionsKeyboard::OnChordWaitTimeChanged()
{
	CString s;
	UINT val;
	m_eChordWaitTime.GetWindowText(s);
	val = _tstoi(s);
	if(val > 5000)
	{
		val = 5000;
		m_eChordWaitTime.SetWindowText(_T("5000"));
	}
	OnSettingsChanged();
}

// Change handling

void COptionsKeyboard::OnRestoreKeyChoice()
{
	CommandID cmd = m_curCommand;

	CInputHandler *ih = CMainFrame::GetInputHandler();

	// Do nothing if there's nothing to restore
	if(cmd == kcNull || ((m_curKeyChoice < 0 || m_curKeyChoice >= ih->GetKeyListSize(cmd)) && ih->GetKeyListSize(cmd) != 0))
	{
		::MessageBeep(MB_ICONWARNING);
		return;
	}

	if(ih->GetKeyListSize(cmd) == 0)
	{
		// Restore the defaults for this key
		mpt::heap_value<CCommandSet> defaultSet;
		defaultSet->LoadDefaultKeymap();
		for(const KeyCombination &kc : defaultSet->GetKeyChoices(cmd))
		{
			m_localCmdSet->Add(kc, cmd, true, m_curKeyChoice);
		}
	} else
	{
		// Restore current key combination choice for currently selected command.
		KeyCombination kc = ih->m_activeCommandSet->GetKey(cmd, m_curKeyChoice);
		m_localCmdSet->Remove(m_curKeyChoice, cmd);
		UpdateWarning(m_localCmdSet->Add(kc, cmd, true, m_curKeyChoice));
	}

	ForceUpdateGUI();
	return;
}


void COptionsKeyboard::OnLButtonDblClk(UINT flags, CPoint point)
{
	ClientToScreen(&point);
	CRect rect;
	m_eCustHotKey.GetWindowRect(rect);
	if(m_curCommand != kcNull && m_eCustHotKey.IsBypassed() && rect.PtInRect(point))
		EnableKeyChoice(true);
	else
		CPropertyPage::OnLButtonDblClk(flags, point);
}


void COptionsKeyboard::OnListenForKeys()
{
	EnableKeyChoice(m_eCustHotKey.IsBypassed());
}


void COptionsKeyboard::EnableKeyChoice(bool enable)
{
	if(!enable && GetFocus() == &m_eCustHotKey)
		GetDlgItem(IDC_BUTTON1)->SetFocus();
	m_eCustHotKey.Bypass(!enable);
	GetDlgItem(IDC_BUTTON1)->SetWindowText(enable ? _T("Cancel") : _T("&Set"));
	if(enable)
		m_eCustHotKey.SetFocus();
}


void COptionsKeyboard::OnDeleteKeyChoice()
{
	CommandID cmd = m_curCommand;

	// Do nothing if there's no key defined for this slot.
	if(m_curCommand == kcNull || m_curKeyChoice < 0 || m_curKeyChoice >= m_localCmdSet->GetKeyListSize(cmd))
	{
		::MessageBeep(MB_ICONWARNING);
		return;
	}

	// Delete current key combination choice for currently selected command.
	m_localCmdSet->Remove(m_curKeyChoice, cmd);

	ForceUpdateGUI();
	UpdateWarning();
}


void COptionsKeyboard::OnCancelKeyChoice(const CWnd *source)
{
	if(source == &m_eFindHotKey)
	{
		GetDlgItem(IDC_BUTTON2)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_FINDHOTKEY_LABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_FINDHOTKEY)->ShowWindow(SW_HIDE);
	} else
	{
		EnableKeyChoice(false);
	}
}


void COptionsKeyboard::OnSetKeyChoice(const CWnd *source)
{
	if(source == &m_eFindHotKey)
	{
		OnFindHotKey();
		return;
	}

	EnableKeyChoice(false);

	CommandID cmd = m_curCommand;
	if(cmd == kcNull)
	{
		Reporting::Warning("Invalid slot.", "Invalid key data", this);
		return;
	}

	FlagSet<KeyEventType> event = kKeyEventNone;
	if(m_bKeyDown.GetCheck() != BST_UNCHECKED)
		event |= kKeyEventDown;
	if(m_bKeyHold.GetCheck() != BST_UNCHECKED)
		event |= kKeyEventRepeat;
	if(m_bKeyUp.GetCheck() != BST_UNCHECKED)
		event |= kKeyEventUp;

	KeyCombination kc(CCommandSet::ContextFromCommand(cmd), m_eCustHotKey.mod, m_eCustHotKey.code, event);
	//detect invalid input
	if(!kc.KeyCode())
	{
		Reporting::Warning("You need to say to which key you'd like to map this command to.", "Invalid key data", this);
		return;
	}
	if(!kc.EventType())
	{
		::MessageBeep(MB_ICONWARNING);
		kc.EventType(kKeyEventDown);
	}

	bool add = true, updateAll = false;
	std::pair<CommandID, KeyCombination> conflictCmd;
	if(CCommandSet::MustBeModifierKey(cmd) && !kc.IsModifierCombination())
	{
		KeyCombination origKc = m_localCmdSet->GetKey(cmd, m_curKeyChoice);
		m_eCustHotKey.SetKey(origKc.Modifier(), origKc.KeyCode());
		UpdateWarning(m_localCmdSet->GetCommandText(cmd) + _T(" must be a modifier (Shift/Ctrl/Alt), but you chose ") + kc.GetKeyText(), true);
		add = false;
	} else if((conflictCmd = m_localCmdSet->IsConflicting(kc, cmd)).first != kcNull && conflictCmd.first != cmd && !m_localCmdSet->IsCrossContextConflict(kc, conflictCmd.second))
	{
		ConfirmAnswer delOld = Reporting::Confirm(_T("New shortcut (") + kc.GetKeyText() + _T(") has the same key combination as ") + m_localCmdSet->GetCommandText(conflictCmd.first) + _T(" in ") + conflictCmd.second.GetContextText() + _T(".\nDo you want to delete the other shortcut, only keeping the new one?"), _T("Shortcut Conflict"), true, false, this);
		if(delOld == cnfYes)
		{
			m_localCmdSet->Remove(conflictCmd.second, conflictCmd.first);
			updateAll = true;
		} else if(delOld == cnfCancel)
		{
			// Cancel altogther; restore original choice
			add = false;
			if(m_curKeyChoice >= 0 && m_curKeyChoice < m_localCmdSet->GetKeyListSize(cmd))
			{
				KeyCombination origKc = m_localCmdSet->GetKey(cmd, m_curKeyChoice);
				m_eCustHotKey.SetKey(origKc.Modifier(), origKc.KeyCode());
			} else
			{
				m_eCustHotKey.SetWindowText(_T(""));
			}
		}
	}
	
	if(add)
	{
		//process valid input
		m_localCmdSet->Remove(m_curKeyChoice, cmd);
		UpdateWarning(m_localCmdSet->Add(kc, cmd, true, m_curKeyChoice), true);
		ForceUpdateGUI(updateAll);
	}
}


static HICON LoadScaledIcon(const TCHAR *resourceName, int iconSize)
{
	mpt::Library comctl32(mpt::LibraryPath::System(P_("Comctl32")));
	if(comctl32.IsValid())
	{
		using PLOADICONWITHSCALEDOWN = HRESULT(WINAPI *)(HINSTANCE, PCWSTR, int, int, HICON *);
		PLOADICONWITHSCALEDOWN LoadIconWithScaleDown = nullptr;
		HICON icon = nullptr;
		if(comctl32.Bind(LoadIconWithScaleDown, "LoadIconWithScaleDown"))
			LoadIconWithScaleDown(NULL, MAKEINTRESOURCEW(reinterpret_cast<uintptr_t>(resourceName)), iconSize, iconSize, &icon);
		if(icon)
			return icon;
	}
	return static_cast<HICON>(::LoadImage(nullptr, resourceName, IMAGE_ICON, iconSize, iconSize, LR_SHARED));
}


void COptionsKeyboard::UpdateWarning(CString text, bool notify)
{
	const int iconSize = HighDPISupport::ScalePixels(16, m_hWnd);
	HICON icon = nullptr;
	if(text.IsEmpty())
	{
		m_warnText.SetWindowText(_T("No conflicts found."));
		if(!m_infoIcon)
			m_infoIcon = LoadScaledIcon(IDI_INFORMATION, iconSize);
		icon = m_infoIcon;
	} else
	{
		if(notify)
			::MessageBeep(MB_ICONWARNING);

		m_warnText.SetWindowText(text);
		if(!m_warnIcon)
			m_warnIcon = LoadScaledIcon(IDI_EXCLAMATION, iconSize);
		icon = m_warnIcon;
	}
	m_warnIconCtl.SetWindowPos(nullptr, 0, 0, iconSize, iconSize, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOMOVE | SWP_NOACTIVATE);
	m_warnIconCtl.SetIcon(icon);
	m_lastWarning = std::move(text);
}


void COptionsKeyboard::OnOK()
{
	CMainFrame::GetInputHandler()->SetNewCommandSet(*m_localCmdSet);

	CString cs;
	m_eChordWaitTime.GetWindowText(cs);
	TrackerSettings::Instance().gnAutoChordWaitTime = _tstoi(cs);

	CPropertyPage::OnOK();
}


void COptionsKeyboard::OnDestroy()
{
	if(m_warnIcon)
	{
		DestroyIcon(m_warnIcon);
		m_warnIcon = nullptr;
	}
	if(m_infoIcon)
	{
		DestroyIcon(m_infoIcon);
		m_infoIcon = nullptr;
	}
	CPropertyPage::OnDestroy();
	m_localCmdSet.reset();
}


void COptionsKeyboard::OnLoad()
{
	auto dlg = OpenFileDialog()
	               .DefaultExtension(U_("mkb"))
	               .DefaultFilename(m_fullPathName)
	               .ExtensionFilter(U_("OpenMPT Key Bindings (*.mkb)|*.mkb||"))
	               .AddPlace(theApp.GetInstallPkgPath() + P_("extraKeymaps"))
	               .WorkingDirectory(TrackerSettings::Instance().m_szKbdFile);
	if(!dlg.Show(this))
		return;

	m_fullPathName = dlg.GetFirstFile();
	m_localCmdSet->LoadFile(m_fullPathName);
	ForceUpdateGUI(true);
	UpdateWarning();
}


void COptionsKeyboard::OnSave()
{
	auto dlg = SaveFileDialog()
	               .DefaultExtension(U_("mkb"))
	               .DefaultFilename(m_fullPathName)
	               .ExtensionFilter(U_("OpenMPT Key Bindings (*.mkb)|*.mkb||"))
	               .WorkingDirectory(TrackerSettings::Instance().m_szKbdFile);
	if(!dlg.Show(this))
		return;

	m_fullPathName = dlg.GetFirstFile();
	m_localCmdSet->SaveFile(m_fullPathName);
}


void COptionsKeyboard::OnToggleNotesRepeat()
{
	m_localCmdSet->QuickChange_NotesRepeat(IsDlgButtonChecked(IDC_NOTESREPEAT) != BST_CHECKED);
	ForceUpdateGUI(true);
}


void COptionsKeyboard::ForceUpdateGUI(bool updateAllKeys)
{
	m_forceUpdate = true;                  // m_curKeyChoice and m_curCommand haven't changed, yet we still want to update.
	int curChoice = m_curKeyChoice;        // next call will overwrite m_curKeyChoice
	OnCommandKeySelChanged();              // update keychoice list
	m_cmbKeyChoice.SetCurSel(curChoice);   // select fresh keychoice (thus restoring m_curKeyChoice)
	OnKeyChoiceSelect();                   // update key data
	OnSettingsChanged();                   // Enable "apply" button

	if(updateAllKeys)
	{
		const int numItems = m_lbnCommandKeys.GetItemCount();
		for(int i = 0; i < numItems; i++)
		{
			if(const auto cmd = static_cast<CommandID>(m_lbnCommandKeys.GetItemData(i)); cmd != kcNull)
				m_lbnCommandKeys.SetItemText(i, 1, m_localCmdSet->GetKeyTextFromCommand(cmd));
		}
		UpdateNoteRepeatCheckbox();
	} else if(m_curCommand != kcNull)
	{
		m_lbnCommandKeys.SetItemText(m_lbnCommandKeys.GetSelectionMark(), 1, m_localCmdSet->GetKeyTextFromCommand(m_curCommand));
		if(mpt::is_in_range(m_curCommand, kcVPStartNotes, kcVPEndNotes))
			UpdateNoteRepeatCheckbox();
	}
}


void COptionsKeyboard::UpdateNoteRepeatCheckbox()
{
	UINT state = uint32_max;
	for(CommandID cmd = kcVPStartNotes; cmd <= kcVPEndNotes && state != BST_INDETERMINATE; cmd = static_cast<CommandID>(cmd + 1))
	{
		for(auto &kc : m_localCmdSet->GetKeyChoices(cmd))
		{
			const bool repeat = (kc.EventType() & kKeyEventRepeat);
			if(repeat && (state == uint32_max || state == BST_CHECKED))
				state = BST_CHECKED;
			else if(!repeat && (state == uint32_max || state == BST_UNCHECKED))
				state = BST_UNCHECKED;
			else
				state = BST_INDETERMINATE;
		}
	}
	CheckDlgButton(IDC_NOTESREPEAT, state);
}


void COptionsKeyboard::OnRestoreKeymapDropdown(NMHDR *, LRESULT *result)
{
	ShowRestoreKeymapMenu();
	*result = 0;
}

void COptionsKeyboard::OnRestoreDefaultKeymap()
{
#if MPT_WINNT_AT_LEAST(MPT_WIN_VISTA)
	if((m_restoreDefaultButton.GetStyle() & BS_SPLITBUTTON) == BS_SPLITBUTTON)
	{
		OnRestoreMPTKeymap();
		return;
	}
#endif
	ShowRestoreKeymapMenu();
}


void COptionsKeyboard::ShowRestoreKeymapMenu()
{
	CRect rect;
	m_restoreDefaultButton.GetWindowRect(rect);

	TPMPARAMS tpmParams{};
	tpmParams.cbSize = sizeof(TPMPARAMS);
	tpmParams.rcExclude = rect;

	CMenu menu;
	if(!menu.CreatePopupMenu())
		return;
	menu.AppendMenu(MF_STRING, ID_KEYPRESET_MPT, _T("&OpenMPT style"));
	menu.AppendMenu(MF_STRING, ID_KEYPRESET_IT, _T("&Impulse Tracker style"));
	menu.AppendMenu(MF_STRING, ID_KEYPRESET_FT2, _T("&Fast Tracker style"));
	menu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, rect.left, rect.bottom, this, &tpmParams);
}


void COptionsKeyboard::RestoreKeymap(KeyboardPreset preset)
{
	if(Reporting::Confirm("Discard all custom changes and restore default key configuration?", false, true, this) == cnfYes)
	{
		m_localCmdSet->LoadDefaultKeymap(preset);
		ForceUpdateGUI(true);
	}
}


int COptionsKeyboard::GetCategoryFromCommandID(CommandID command) const
{
	const InputTargetContext context = CCommandSet::ContextFromCommand(command);
	for(size_t cat = 0; cat < commandCategories.size(); cat++)
	{
		if(commandCategories[cat].id == context)
			return static_cast<int>(cat);
	}
	return -1;
}


OPENMPT_NAMESPACE_END
