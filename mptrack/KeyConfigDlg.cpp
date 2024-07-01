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
			if(!m_isDummy)
				m_pOptKeyDlg->OnSetKeyChoice();
		}
		break;

	case MIDIEvents::evNoteOn:
	case MIDIEvents::evNoteOff:
		SetKey(ModMidi, byte1 | 0x80);
		if(!m_isDummy)
			m_pOptKeyDlg->OnSetKeyChoice();
		break;

	default:
		break;
	}

	return 1;
}


BOOL CCustEdit::PreTranslateMessage(MSG *pMsg)
{
	if(pMsg)
	{
		if(pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN)
		{
			SetKey(CMainFrame::GetInputHandler()->GetModifierMask(), static_cast<UINT>(pMsg->wParam));
			return -1;  // Keypress handled, don't pass on message.
		} else if(pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP)
		{
			//if a key has been released but custom edit box is empty, we have probably just
			//navigated into the box with TAB or SHIFT-TAB. No need to set keychoice.
			if(code != 0 && !m_isDummy)
				m_pOptKeyDlg->OnSetKeyChoice();
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
}


//***************************************************************************************//
// COptionsKeyboard:
//
//***************************************************************************************//

// Initialisation

BEGIN_MESSAGE_MAP(COptionsKeyboard, CPropertyPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_COMMAND_LIST, &COptionsKeyboard::OnCommandKeySelChanged)

	ON_LBN_SELCHANGE(IDC_CHOICECOMBO,     &COptionsKeyboard::OnKeyChoiceSelect)
	ON_LBN_SELCHANGE(IDC_KEYCATEGORY,     &COptionsKeyboard::OnCategorySelChanged)
	ON_EN_UPDATE(IDC_CHORDDETECTWAITTIME, &COptionsKeyboard::OnChordWaitTimeChanged)
	ON_COMMAND(IDC_DELETE,                &COptionsKeyboard::OnDeleteKeyChoice)
	ON_COMMAND(IDC_RESTORE,               &COptionsKeyboard::OnRestoreKeyChoice)
	ON_COMMAND(IDC_LOAD,                  &COptionsKeyboard::OnLoad)
	ON_COMMAND(IDC_SAVE,                  &COptionsKeyboard::OnSave)
	ON_COMMAND(IDC_CHECKKEYDOWN,          &COptionsKeyboard::OnCheck)
	ON_COMMAND(IDC_CHECKKEYHOLD,          &COptionsKeyboard::OnCheck)
	ON_COMMAND(IDC_CHECKKEYUP,            &COptionsKeyboard::OnCheck)
	ON_COMMAND(IDC_NOTESREPEAT,           &COptionsKeyboard::OnNotesRepeat)
	ON_COMMAND(IDC_NONOTESREPEAT,         &COptionsKeyboard::OnNoNotesRepeat)
	ON_COMMAND(IDC_CLEARLOG,              &COptionsKeyboard::OnClearLog)
	ON_COMMAND(IDC_RESTORE_KEYMAP,        &COptionsKeyboard::OnRestoreDefaultKeymap)
	ON_EN_CHANGE(IDC_FIND,                &COptionsKeyboard::OnSearchTermChanged)
	ON_EN_CHANGE(IDC_FINDHOTKEY,          &COptionsKeyboard::OnFindHotKey)
	ON_EN_SETFOCUS(IDC_FINDHOTKEY,        &COptionsKeyboard::OnClearHotKey)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void COptionsKeyboard::DoDataExchange(CDataExchange *pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_KEYCATEGORY, m_cmbCategory);
	DDX_Control(pDX, IDC_COMMAND_LIST, m_lbnCommandKeys);
	DDX_Control(pDX, IDC_CHOICECOMBO, m_cmbKeyChoice);
	DDX_Control(pDX, IDC_CHORDDETECTWAITTIME, m_eChordWaitTime);
	DDX_Control(pDX, IDC_KEYREPORT, m_eReport);
	DDX_Control(pDX, IDC_CUSTHOTKEY, m_eCustHotKey);
	DDX_Control(pDX, IDC_FINDHOTKEY, m_eFindHotKey);
	DDX_Control(pDX, IDC_CHECKKEYDOWN, m_bKeyDown);
	DDX_Control(pDX, IDC_CHECKKEYHOLD, m_bKeyHold);
	DDX_Control(pDX, IDC_CHECKKEYUP, m_bKeyUp);
	DDX_Control(pDX, IDC_FIND, m_eFind);
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
	m_localCmdSet->Copy(CMainFrame::GetInputHandler()->m_activeCommandSet.get());

	m_lbnCommandKeys.SetExtendedStyle(m_lbnCommandKeys.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
	m_listGrouped = CListCtrlEx::EnableGroupView(m_lbnCommandKeys);

	//Fill category combo and automatically selects first category
	DefineCommandCategories();
	for(size_t c = 0; c < commandCategories.size(); c++)
	{
		if(commandCategories[c].name && !commandCategories[c].commands.empty())
			m_cmbCategory.SetItemData(m_cmbCategory.AddString(commandCategories[c].name), c);
	}
	m_cmbCategory.SetCurSel(0);
	UpdateDialog();

	m_eCustHotKey.SetParent(m_hWnd, IDC_CUSTHOTKEY, this);
	m_eFindHotKey.SetParent(m_hWnd, IDC_FINDHOTKEY, this);
	m_eReport.FmtLines(TRUE);
	m_eReport.SetWindowText(_T(""));

	m_eChordWaitTime.SetWindowText(mpt::cfmt::val(TrackerSettings::Instance().gnAutoChordWaitTime.Get()));
	return TRUE;
}


void CommandCategory::AddCommands(CommandID first, CommandID last, bool addSeparatorAtEnd)
{
	int count = last - first + 1, val = first;
	commands.insert(commands.end(), count, kcNull);
	std::generate(commands.end() - count, commands.end(), [&val] { return static_cast<CommandID>(val++); });
	if(addSeparatorAtEnd)
		separators.push_back(last);
}


// Filter commands: We only need user to see a select set off commands
// for each category
void COptionsKeyboard::DefineCommandCategories()
{
	{
		CommandCategory newCat(_T("Global keys"), kCtxAllContexts);

		newCat.AddCommands(kcStartFile, kcEndFile, true);
		newCat.AddCommands(kcStartPlayCommands, kcEndPlayCommands, true);
		newCat.AddCommands(kcStartEditCommands, kcEndEditCommands, true);
		newCat.AddCommands(kcStartView, kcEndView, true);
		newCat.AddCommands(kcStartMisc, kcEndMisc, true);
		newCat.commands.push_back(kcDummyShortcut);

		commandCategories.push_back(newCat);
	}

	commandCategories.emplace_back(_T("  General [Top]"), kCtxCtrlGeneral);
	commandCategories.emplace_back(_T("  General [Bottom]"), kCtxViewGeneral);
	commandCategories.emplace_back(_T("  Pattern Editor [Top]"), kCtxCtrlPatterns);

	{
		CommandCategory newCat(_T("  Pattern Editor - Order List"), kCtxCtrlOrderlist);

		newCat.AddCommands(kcStartOrderlistCommands, kcEndOrderlistCommands);
		newCat.separators.push_back(kcEndOrderlistNavigation);
		newCat.separators.push_back(kcEndOrderlistEdit);
		newCat.separators.push_back(kcEndOrderlistNum);

		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("  Pattern Editor - Quick Channel Settings"), kCtxChannelSettings);
		newCat.AddCommands(kcStartChnSettingsCommands, kcEndChnSettingsCommands);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("    Pattern Editor - General"), kCtxViewPatterns);

		newCat.AddCommands(kcStartPlainNavigate, kcEndPlainNavigate, true);
		newCat.AddCommands(kcStartJumpSnap, kcEndJumpSnap, true);
		newCat.AddCommands(kcStartHomeEnd, kcEndHomeEnd, true);
		newCat.AddCommands(kcStartGotoColumn, kcEndGotoColumn, true);
		newCat.AddCommands(kcPrevPattern, kcNextSequence, true);
		newCat.AddCommands(kcStartPatternScrolling, kcEndPatternScrolling, true);
		newCat.AddCommands(kcStartSelect, kcEndSelect, true);
		newCat.AddCommands(kcStartPatternClipboard, kcEndPatternClipboard, true);
		newCat.AddCommands(kcClearRow, kcInsertWholeRowGlobal, true);
		newCat.AddCommands(kcStartChannelKeys, kcEndChannelKeys, true);
		newCat.AddCommands(kcBeginTranspose, kcEndTranspose, true);
		newCat.AddCommands(kcPatternAmplify, kcPatternShrinkSelection, true);
		newCat.AddCommands(kcStartPatternEditMisc, kcEndPatternEditMisc, true);

		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("        Pattern Editor - Note Column"), kCtxViewPatternsNote);

		newCat.AddCommands(kcVPStartNotes, kcVPEndNotes, true);
		newCat.AddCommands(kcSetOctave0, kcSetOctave9, true);
		newCat.AddCommands(kcStartNoteMisc, kcEndNoteMisc);

		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("        Pattern Editor - Instrument Column"), kCtxViewPatternsIns);
		newCat.AddCommands(kcSetIns0, kcSetIns9);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("        Pattern Editor - Volume Column"), kCtxViewPatternsVol);
		newCat.AddCommands(kcSetVolumeStart, kcSetVolumeEnd);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("        Pattern Editor - Effect Column"), kCtxViewPatternsFX);
		newCat.AddCommands(kcSetFXStart, kcSetFXEnd);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("        Pattern Editor - Effect Parameter Column"), kCtxViewPatternsFXparam);
		newCat.AddCommands(kcSetFXParam0, kcSetFXParamF);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("  Sample [Top]"), kCtxCtrlSamples);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("    Sample Editor"), kCtxViewSamples);

		newCat.AddCommands(kcStartSampleEditing, kcEndSampleEditing, true);
		newCat.AddCommands(kcStartSampleMisc, kcEndSampleMisc, true);
		newCat.AddCommands(kcStartSampleCues, kcEndSampleCueGroup);

		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("  Instrument Editor"), kCtxCtrlInstruments);
		newCat.AddCommands(kcStartInstrumentCtrlMisc, kcEndInstrumentCtrlMisc);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("    Envelope Editor"), kCtxViewInstruments);
		newCat.AddCommands(kcStartInstrumentMisc, kcEndInstrumentMisc);
		commandCategories.push_back(newCat);
	}

	commandCategories.emplace_back(_T("  Comments [Top]"), kCtxCtrlComments);

	{
		CommandCategory newCat(_T("  Comments [Bottom]"), kCtxViewComments);
		newCat.AddCommands(kcStartCommentsCommands, kcEndCommentsCommands);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("  Plugin Editor"), kCtxVSTGUI);
		newCat.AddCommands(kcStartVSTGUICommands, kcEndVSTGUICommands);
		commandCategories.push_back(newCat);
	}

	{
		CommandCategory newCat(_T("  Tree View"), kCtxViewTree);
		newCat.AddCommands(kcStartTreeViewCommands, kcEndTreeViewCommands);
		commandCategories.push_back(newCat);
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


void COptionsKeyboard::OnFindHotKey()
{
	if(IsLocked())
		return;
	
	if(m_eFindHotKey.code == 0)
	{
		UpdateCategory();
	}
	UpdateShortcutList(m_eFindHotKey.code == 0 ? m_curCategory : -1);
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

	const bool searchByName = !findString.IsEmpty(), searchByKey = (m_eFindHotKey.code != 0);
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
	m_lbnCommandKeys.DeleteColumn(0);
	if(m_listGrouped)
		ListView_RemoveAllGroups(m_lbnCommandKeys);
	m_lbnCommandKeys.InsertColumn(0, _T(""));
	m_lbnCommandKeys.DeleteAllItems();

	int currentGroup = -1;
	int itemID = -1;

	for(int cat = firstCat; cat <= lastCat; cat++)
	{
		// When searching, we also add the category names to the list.
		bool addCategoryName = (firstCat != lastCat);

		for(size_t cmd = 0; cmd < commandCategories[cat].commands.size(); cmd++)
		{
			CommandID com = (CommandID)commandCategories[cat].commands[cmd];

			CString cmdText = m_localCmdSet->GetCommandText(com);
			bool addKey = true;

			if(searchByKey)
			{
				addKey = false;
				int numChoices = m_localCmdSet->GetKeyListSize(com);
				for(int choice = 0; choice < numChoices; choice++)
				{
					const KeyCombination &kc = m_localCmdSet->GetKey(com, choice);
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

			if(addKey)
			{
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

				if(!m_localCmdSet->isHidden(com))
				{
					if(doSearch && addCategoryName)
					{
						if(m_listGrouped)
						{
							InsertGroup(commandCategories[cat].name.Trim(), ++currentGroup);
						} else
						{
							const CString catName = _T("------ ") + commandCategories[cat].name.Trim() + _T(" ------");
							lvi.iItem = ++itemID;
							lvi.lParam = LPARAM(-1);
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

					if(curCommand == com)
					{
						// Keep selection on previously selected string
						m_lbnCommandKeys.SetSelectionMark(itemID);
					}
				}

				if(commandCategories[cat].SeparatorAt(com))
				{
					if(m_listGrouped)
					{
						InsertGroup(_T(""), ++currentGroup);

					} else
					{
						lvi.iItem = ++itemID;
						lvi.lParam = LPARAM(-1);
						lvi.pszText = const_cast<TCHAR *>(_T("------------------------------------------------------"));
						m_lbnCommandKeys.InsertItem(&lvi);
					}
				}
			}
		}
	}

	if(m_lbnCommandKeys.GetSelectionMark() == -1)
	{
		m_lbnCommandKeys.SetSelectionMark(0);
	}
	m_lbnCommandKeys.SetColumnWidth(0, LVSCW_AUTOSIZE);
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

	//Separator
	if(cmd == kcNull)
	{
		m_cmbKeyChoice.SetWindowText(_T(""));
		m_cmbKeyChoice.EnableWindow(FALSE);
		m_eCustHotKey.SetWindowText(_T(""));
		m_eCustHotKey.EnableWindow(FALSE);
		m_bKeyDown.SetCheck(0);
		m_bKeyDown.EnableWindow(FALSE);
		m_bKeyHold.SetCheck(0);
		m_bKeyHold.EnableWindow(FALSE);
		m_bKeyUp.SetCheck(0);
		m_bKeyUp.EnableWindow(FALSE);
		m_curCommand = kcNull;
	}

	//Fill "choice" list
	else if((cmd >= kcFirst && cmd != m_curCommand) || m_forceUpdate)  // Have we changed command?
	{
		m_forceUpdate = false;

		m_cmbKeyChoice.EnableWindow(TRUE);
		m_eCustHotKey.EnableWindow(TRUE);
		m_bKeyDown.EnableWindow(TRUE);
		m_bKeyHold.EnableWindow(TRUE);
		m_bKeyUp.EnableWindow(TRUE);

		m_curCommand = cmd;
		m_curCategory = GetCategoryFromCommandID(cmd);

		m_cmbKeyChoice.ResetContent();
		int numChoices = m_localCmdSet->GetKeyListSize(cmd);
		if((cmd < kcNumCommands) && (numChoices > 0))
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

//Fills or clears key choice info
void COptionsKeyboard::OnKeyChoiceSelect()
{
	int choice = static_cast<int>(m_cmbKeyChoice.GetItemData(m_cmbKeyChoice.GetCurSel()));
	CommandID cmd = m_curCommand;

	//If nothing there, clear
	if(cmd == kcNull || choice >= m_localCmdSet->GetKeyListSize(cmd) || choice < 0)
	{
		m_curKeyChoice = choice;
		m_forceUpdate = true;
		m_eCustHotKey.SetKey(ModNone, 0);
		m_bKeyDown.SetCheck(0);
		m_bKeyHold.SetCheck(0);
		m_bKeyUp.SetCheck(0);
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
	KeyCombination kc;
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
		for(int i = 0; i < defaultSet->GetKeyListSize(cmd); i++)
		{
			m_localCmdSet->Add(defaultSet->GetKey(cmd, i), cmd, true, m_curKeyChoice);
		}
	} else
	{
		// Restore current key combination choice for currently selected command.
		kc = ih->m_activeCommandSet->GetKey(cmd, m_curKeyChoice);
		m_localCmdSet->Remove(m_curKeyChoice, cmd);
		m_localCmdSet->Add(kc, cmd, true, m_curKeyChoice);
	}

	ForceUpdateGUI();
	return;
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
	return;
}


void COptionsKeyboard::OnSetKeyChoice()
{
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

	KeyCombination kc((commandCategories[m_curCategory]).id, m_eCustHotKey.mod, m_eCustHotKey.code, event);
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

	bool add = true;
	std::pair<CommandID, KeyCombination> conflictCmd;
	if((conflictCmd = m_localCmdSet->IsConflicting(kc, cmd)).first != kcNull
	   && conflictCmd.first != cmd
	   && !m_localCmdSet->IsCrossContextConflict(kc, conflictCmd.second))
	{
		ConfirmAnswer delOld = Reporting::Confirm(_T("New shortcut (") + kc.GetKeyText() + _T(") has the same key combination as ") + m_localCmdSet->GetCommandText(conflictCmd.first) + _T(" in ") + conflictCmd.second.GetContextText() + _T(".\nDo you want to delete the other shortcut, only keeping the new one?"), _T("Shortcut Conflict"), true, false, this);
		if(delOld == cnfYes)
		{
			m_localCmdSet->Remove(conflictCmd.second, conflictCmd.first);
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
		CString report, reportHistory;
		//process valid input
		m_localCmdSet->Remove(m_curKeyChoice, cmd);
		report = m_localCmdSet->Add(kc, cmd, true, m_curKeyChoice);

		//Update log
		m_eReport.GetWindowText(reportHistory);
		m_eReport.SetWindowText(report + reportHistory);
		ForceUpdateGUI();
	}
}


void COptionsKeyboard::OnOK()
{
	CMainFrame::GetInputHandler()->SetNewCommandSet(m_localCmdSet.get());

	CString cs;
	m_eChordWaitTime.GetWindowText(cs);
	TrackerSettings::Instance().gnAutoChordWaitTime = _tstoi(cs);

	CPropertyPage::OnOK();
}


void COptionsKeyboard::OnDestroy()
{
	CPropertyPage::OnDestroy();
	m_localCmdSet = nullptr;
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
	ForceUpdateGUI();
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


void COptionsKeyboard::OnNotesRepeat()
{
	m_localCmdSet->QuickChange_NotesRepeat(true);
	ForceUpdateGUI();
}


void COptionsKeyboard::OnNoNotesRepeat()
{
	m_localCmdSet->QuickChange_NotesRepeat(false);
	ForceUpdateGUI();
}


void COptionsKeyboard::ForceUpdateGUI()
{
	m_forceUpdate = true;                 // m_nCurKeyChoice and m_nCurHotKey haven't changed, yet we still want to update.
	int ntmpChoice = m_curKeyChoice;      // next call will overwrite m_nCurKeyChoice
	OnCommandKeySelChanged();              // update keychoice list
	m_cmbKeyChoice.SetCurSel(ntmpChoice);  // select fresh keychoice (thus restoring m_nCurKeyChoice)
	OnKeyChoiceSelect();                   // update key data
	OnSettingsChanged();                   // Enable "apply" button
}


void COptionsKeyboard::OnClearLog()
{
	m_eReport.SetWindowText(_T(""));
	ForceUpdateGUI();
}


void COptionsKeyboard::OnRestoreDefaultKeymap()
{
	if(Reporting::Confirm("Discard all custom changes and restore default key configuration?", false, true, this) == cnfYes)
	{
		m_localCmdSet->LoadDefaultKeymap();
		ForceUpdateGUI();
	}
}


int COptionsKeyboard::GetCategoryFromCommandID(CommandID command) const
{
	for(size_t cat = 0; cat < commandCategories.size(); cat++)
	{
		const auto &cmds = commandCategories[cat].commands;
		if(mpt::contains(cmds, command))
			return static_cast<int>(cat);
	}
	return -1;
}


OPENMPT_NAMESPACE_END
