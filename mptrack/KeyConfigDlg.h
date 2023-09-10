/*
 * KeyConfigDlg.h
 * --------------
 * Purpose: Implementation of OpenMPT's keyboard configuration dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"
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
	COptionsKeyboard *m_pOptKeyDlg = nullptr;
	HWND m_hParent = nullptr;
	UINT m_nCtrlId = 0;
	bool m_isFocussed = false, m_isDummy = false;

public:
	FlagSet<Modifiers> mod = ModNone;
	UINT code = 0;

	explicit CCustEdit(bool dummyField) : m_isDummy(dummyField) { }
	void SetParent(HWND h, UINT nID, COptionsKeyboard *pOKD)
	{
		m_hParent = h;
		m_nCtrlId = nID;
		m_pOptKeyDlg = pOKD;
	}
	void SetKey(FlagSet<Modifiers> mod, UINT code);
	
protected:
	BOOL PreTranslateMessage(MSG *pMsg) override;
	
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnKillFocus(CWnd *pNewWnd);
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	
	DECLARE_MESSAGE_MAP()
};

class COptionsKeyboard: public CPropertyPage
{
	friend class CCustEdit;

protected:
	CListBox m_lbnHotKeys;
	CListCtrl m_lbnCommandKeys;
	CComboBox m_cmbKeyChoice;
	CComboBox m_cmbCategory;
	CButton m_bKeyDown, m_bKeyHold, m_bKeyUp;
	CButton m_bnReset;
	CCustEdit m_eCustHotKey{false}, m_eFindHotKey{true};
	CEdit m_eFind;
	CEdit m_eReport, m_eChordWaitTime;
	
	std::vector<CommandCategory> commandCategories;
	std::unique_ptr<CCommandSet> m_localCmdSet;
	mpt::PathString m_fullPathName;
	CommandID m_curCommand = kcNull;
	int m_curCategory = -1, m_curKeyChoice = -1;
	int m_lockCount = 0;
	bool m_forceUpdate = false;
	bool m_listGrouped = false;

public:
	COptionsKeyboard();

protected:
	BOOL OnInitDialog() override;
	void OnOK() override;
	BOOL OnSetActive() override;
	void DoDataExchange(CDataExchange* pDX) override;

	void DefineCommandCategories();
	void ForceUpdateGUI();
	void InsertGroup(const TCHAR *title, int groupId);
	void UpdateShortcutList(int category = -1);
	void UpdateCategory();
	int GetCategoryFromCommandID(CommandID command) const;
	void OnSetKeyChoice();

	void LockControls() { m_lockCount++; }
	void UnlockControls() { m_lockCount--; MPT_ASSERT(m_lockCount >= 0); }
	bool IsLocked() const noexcept { return m_lockCount != 0; }

	afx_msg void UpdateDialog();
	afx_msg void OnKeyboardChanged();
	afx_msg void OnKeyChoiceSelect();
	afx_msg void OnCommandKeySelChanged(NMHDR *pNMHDR = nullptr, LRESULT *pResult = nullptr);
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
