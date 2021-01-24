/*
 * KeyConfigDlg.h
 * --------------
 * Purpose: Implementation of OpenMPT's keyboard configuration dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"
#include "Mainfrm.h"
#include "InputHandler.h"

OPENMPT_NAMESPACE_BEGIN

class COptionsKeyboard;

// Might promote to class so we can add rules
// (eg automatically do note off stuff, generate chord keybindings from notes based just on modifier.
// Would need GUI rules too as options would be different for each category
class CommandCategory
{
public:
	CommandCategory(const TCHAR *n, InputTargetContext d) : name(n), id(d) { }

	bool SeparatorAt(CommandID c) const
	{
		return mpt::contains(separators, c);
	}

	void AddCommands(CommandID first, CommandID last, bool addSeparatorAtEnd = false);

	CString name;
	InputTargetContext id;
	std::vector<CommandID> separators;
	std::vector<CommandID> commands;
};


class CCustEdit: public CEdit
{
protected:
	COptionsKeyboard *m_pOptKeyDlg;
	HWND m_hParent = nullptr;
	UINT m_nCtrlId = 0;
	bool m_isFocussed = false, m_isDummy = false;

public:
	FlagSet<Modifiers> mod = ModNone;
	UINT code = 0;

	CCustEdit(bool dummyField) : m_isDummy(dummyField) { }
	void SetParent(HWND h, UINT nID, COptionsKeyboard *pOKD)
	{
		m_hParent = h;
		m_nCtrlId = nID;
		m_pOptKeyDlg = pOKD;
	}
	void SetKey(FlagSet<Modifiers> mod, UINT code);
	
	BOOL PreTranslateMessage(MSG *pMsg) override;
	
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	
	DECLARE_MESSAGE_MAP()
};

class COptionsKeyboard: public CPropertyPage
{
protected:
	CListBox m_lbnHotKeys;
	CListBox m_lbnCommandKeys;
	CComboBox m_cmbKeyChoice;
	CComboBox m_cmbCategory;
	CButton m_bKeyDown, m_bKeyHold, m_bKeyUp;
	CButton m_bnReset;
	CCustEdit m_eCustHotKey, m_eFindHotKey;
	CEdit m_eFind;
	CEdit m_eReport, m_eChordWaitTime;
	CommandID m_curCommand = kcNull;
	int m_curCategory = -1, m_curKeyChoice = -1;
	mpt::PathString m_fullPathName;
	std::unique_ptr<CCommandSet> m_localCmdSet;
	bool m_forceUpdate = false;

	void ForceUpdateGUI();
	void UpdateShortcutList(int category = -1);
	void UpdateCategory();
	int GetCategoryFromCommandID(CommandID command) const;

public:
	COptionsKeyboard() : CPropertyPage(IDD_OPTIONS_KEYBOARD), m_eCustHotKey(false), m_eFindHotKey(true) { }
	std::vector<CommandCategory> commandCategories;
	void DefineCommandCategories();

	void OnSetKeyChoice();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;

	afx_msg void UpdateDialog();
	afx_msg void OnKeyboardChanged();
	afx_msg void OnKeyChoiceSelect();
	afx_msg void OnCommandKeySelChanged();
	afx_msg void OnCategorySelChanged();
	afx_msg void OnSearchTermChanged();
	afx_msg void OnChordWaitTimeChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnCheck() { OnSetKeyChoice(); };
	afx_msg void OnNotesRepeat();
	afx_msg void OnNoNotesRepeat();
	afx_msg void OnDeleteKeyChoice();
	afx_msg void OnRestoreKeyChoice();
	afx_msg void OnLoad();
	afx_msg void OnSave();
	afx_msg void OnClearLog();
	afx_msg void OnRestoreDefaultKeymap();
	afx_msg void OnClearHotKey();
	afx_msg void OnFindHotKey();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
