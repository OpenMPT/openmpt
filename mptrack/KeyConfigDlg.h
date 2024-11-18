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
#include "CListCtrl.h"
#include "CommandSet.h"

OPENMPT_NAMESPACE_BEGIN

class COptionsKeyboard;

class CommandCategory
{
public:
	CommandCategory(const TCHAR *n, InputTargetContext ctx) : name{n}, id{ctx} {}

	const CString name;
	const InputTargetContext id;
	
	struct Range
	{
		Range(CommandID f, CommandID l, const TCHAR *n) : first{f}, last{l}, name{n} {}
		const CommandID first, last;
		const CString name;
	};
	std::vector<Range> commandRanges;
};


class CCustEdit: public CEdit
{
protected:
	COptionsKeyboard *m_pOptKeyDlg = nullptr;
	bool m_isFocussed = false;
	bool m_bypassed = false;

public:
	FlagSet<Modifiers> mod = ModNone;
	UINT code = 0;

	void SetOwner(COptionsKeyboard &dlg) { m_pOptKeyDlg = &dlg; }
	void SetKey(FlagSet<Modifiers> mod, UINT code);
	bool HasKey() const noexcept { return mod || code; }

	void Bypass(bool bypass) { m_bypassed = bypass; EnableWindow(bypass ? FALSE : TRUE); }
	bool IsBypassed() const { return m_bypassed; }
	
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
	CListCtrlEx m_lbnCommandKeys;
	CComboBox m_cmbKeyChoice;
	CComboBox m_cmbCategory;
	CButton m_bKeyDown, m_bKeyHold, m_bKeyUp;
	CButton m_bnReset;
	CCustEdit m_eCustHotKey, m_eFindHotKey;
	CStatic m_warnIconCtl, m_warnText;
	CEdit m_eFind;
	CEdit m_eChordWaitTime;
	CButton m_restoreDefaultButton;
	HICON m_infoIcon = nullptr, m_warnIcon = nullptr;
	
	CString m_lastWarning;
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
	void ForceUpdateGUI(bool updateAllKeys = false);
	void UpdateNoteRepeatCheckbox();
	void InsertGroup(const TCHAR *title, int groupId);
	void UpdateShortcutList(int category = -1);
	void UpdateCategory();
	int GetCategoryFromCommandID(CommandID command) const;
	void OnCancelKeyChoice(const CWnd *source);
	void OnSetKeyChoice(const CWnd *source);

	void LockControls() { m_lockCount++; }
	void UnlockControls() { m_lockCount--; MPT_ASSERT(m_lockCount >= 0); }
	bool IsLocked() const noexcept { return m_lockCount != 0; }

	void EnableKeyChoice(bool enable);

	void UpdateWarning(CString text = {}, bool notify = false);
	void ShowRestoreKeymapMenu();
	void RestoreKeymap(KeyboardPreset preset);

	afx_msg LRESULT OnDPIChangedAfterParent(WPARAM, LPARAM);
	afx_msg void UpdateDialog();
	afx_msg void OnKeyboardChanged();
	afx_msg void OnKeyChoiceSelect();
	afx_msg void OnCommandKeySelChanged(NMHDR *pNMHDR = nullptr, LRESULT *pResult = nullptr);
	afx_msg void OnListenForKeysFromList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCategorySelChanged();
	afx_msg void OnSearchTermChanged();
	afx_msg void OnChordWaitTimeChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnCheck() { OnSetKeyChoice(&m_eCustHotKey); };
	afx_msg void OnToggleNotesRepeat();
	afx_msg void OnListenForKeys();
	afx_msg void OnDeleteKeyChoice();
	afx_msg void OnRestoreKeyChoice();
	afx_msg void OnLoad();
	afx_msg void OnSave();
	afx_msg void OnRestoreDefaultKeymap();
	afx_msg void OnRestoreKeymapDropdown(NMHDR *, LRESULT *result);
	afx_msg void OnRestoreMPTKeymap() { RestoreKeymap(KeyboardPreset::MPT); }
	afx_msg void OnRestoreITKeymap() { RestoreKeymap(KeyboardPreset::IT); }
	afx_msg void OnRestoreFT2Keymap() { RestoreKeymap(KeyboardPreset::FT2); }
	afx_msg void OnClearHotKey();
	afx_msg void OnClearSearch();
	afx_msg void OnEnableFindHotKey();
	afx_msg void OnFindHotKey();
	afx_msg void OnLButtonDblClk(UINT flags, CPoint point);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
