/*
 * KeyConfigDlg.h
 * --------------
 * Purpose: Implementation of OpenMPT's keyboard configuration dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once
#include "mptrack.h"
#include "MainFrm.h"
#include "mpdlgs.h"
#include "moptions.h"

// Might promote to class so we can add rules 
// (eg automatically do note off stuff, generate chord keybindings from notes based just on modifier.
// Would need GUI rules too as options would be different for each category
class CommandCategory 
{
public:
	CommandCategory(CString n, InputTargetContext d)
	{
		CommandCategory();
		name=n;
		id=d;
	}
	

	//copy constructor; necessary if we want to use arrays of this class
	CommandCategory(const CommandCategory &copy )
	{
		name = copy.name;
		id = copy.id;
		commands.Copy(copy.commands);
		separators.Copy(copy.separators);
	}
	void operator=(const CommandCategory& copy)
	{
		name = copy.name;
		id = copy.id;
		commands.Copy(copy.commands);
		separators.Copy(copy.separators);
	}
	bool separatorAt(int c)
	{
		for (int p=0; p<separators.GetSize(); p++)
		{
			if (separators[p]==c)
				return true;
		}
		return false;
	}


	CommandCategory() {};
	~CommandCategory() {};
	CString name;
	InputTargetContext id;
	CArray<int> separators;
	CArray<int> commands;
};


//===========================
class CCustEdit: public CEdit
//===========================
{
protected:
	COptionsKeyboard *m_pOptKeyDlg;
	HWND m_hParent;
	UINT m_nCtrlId;
	bool isFocussed, isDummy;

public:
	UINT mod;
	UINT code;

	CCustEdit(bool dummyField) : m_hParent(nullptr), isFocussed(false), isDummy(dummyField), mod(0), code(0) { }
	VOID SetParent(HWND h, UINT nID, COptionsKeyboard* pOKD) { m_hParent = h; m_nCtrlId = nID; m_pOptKeyDlg = pOKD;}
	void SetKey(UINT mod, UINT code);
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
};

//==========================================
class COptionsKeyboard: public CPropertyPage
//==========================================
{
protected:
	CListBox m_lbnHotKeys;
	CListBox m_lbnCommandKeys;		//rewbs.keys
	CComboBox m_cmbKeyChoice;		//rewbs.keys
	CComboBox m_cmbCategory;
	CButton m_bKeyDown, m_bKeyHold, m_bKeyUp;
	CButton m_bnReset;
	CCustEdit m_eCustHotKey, m_eFindHotKey;
	CEdit m_eFind;
	CEdit m_eReport, m_eChordWaitTime;
	UINT m_nKeyboardCfg;
	int m_nCurHotKey, m_nCurCategory, m_nCurKeyChoice;
	CString m_sFullPathName;
	CCommandSet *plocalCmdSet;
	bool m_bForceUpdate;
	bool m_bModified;
	bool m_bChoiceModified;

	void ForceUpdateGUI();
	void UpdateShortcutList(int category = -1);
	void UpdateCategory();
	int GetCategoryFromCommandID(CommandID command) const;

public:
	COptionsKeyboard() : CPropertyPage(IDD_OPTIONS_KEYBOARD), m_eCustHotKey(false), m_eFindHotKey(true), m_nKeyboardCfg(0) { }
	BOOL SetKey(UINT nId, UINT nChar, UINT nFlags);
	CArray<CommandCategory, CommandCategory> commandCategories;
	void DefineCommandCategories();


public:
	afx_msg void OnSetKeyChoice();
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void UpdateDialog();
	afx_msg void OnKeyboardChanged();
	afx_msg void OnKeyChoiceSelect();
	afx_msg void OnCommandKeySelChanged();
	afx_msg void OnCategorySelChanged();
	afx_msg void OnSearchTermChanged();
	afx_msg void OnChordWaitTimeChanged(); //rewbs.autochord
	afx_msg void OnHotKeyChanged();
	afx_msg void OnSettingsChanged() { SetModified(TRUE); }
	afx_msg void OnCheck() { OnSetKeyChoice(); };
	afx_msg void OnNotesRepeat();
	afx_msg void OnNoNotesRepeat();
	afx_msg void OnSetITEffects();
	afx_msg void OnSetXMEffects();
	afx_msg void OnDeleteKeyChoice();
	afx_msg void OnRestoreKeyChoice();
	afx_msg void OnLoad();
	afx_msg void OnSave();
	afx_msg void OnClearLog();
	afx_msg void OnRestoreDefaultKeymap();
	afx_msg void OnClearHotKey();
	afx_msg void OnFindHotKey();
	DECLARE_MESSAGE_MAP();
public:
	afx_msg void OnDestroy();
};
