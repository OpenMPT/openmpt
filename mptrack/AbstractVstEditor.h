/*
 * AbstractVstEditor.h
 * -------------------
 * Purpose: Common plugin editor interface class. This code is shared between custom and default plugin user interfaces.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_PLUGINS

#include <vector>
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class IMixPlugin;

class CAbstractVstEditor: public CDialog
{
protected:
	CMenu m_Menu;
	CMenu m_PresetMenu;
	std::vector<CMenu *> m_pPresetMenuGroup;
	CMenu m_InputMenu;
	CMenu m_OutputMenu;
	CMenu m_MacroMenu;
	CMenu m_OptionsMenu;
	static UINT m_clipboardFormat;
	int32 m_currentPresetMenu;
	int32 m_clientHeight;
	int m_nLearnMacro;
	INSTRUMENTINDEX m_nInstrument;
	bool m_isMinimized : 1;
	bool m_updateDisplay : 1;

public:
	IMixPlugin &m_VstPlugin;
	int m_nCurProg;

	CAbstractVstEditor(IMixPlugin &plugin);
	virtual ~CAbstractVstEditor();
	void SetupMenu(bool force = false);
	void SetTitle();
	void SetLearnMacro(int inMacro);
	int GetLearnMacro();

	void SetPreset(int32 preset);
	void UpdatePresetField();

	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnLoadPreset();
	afx_msg void OnSavePreset();
	afx_msg void OnCopyParameters();
	afx_msg void OnPasteParameters();
	afx_msg void OnRandomizePreset();
	afx_msg void OnSetPreset(UINT nID);
	afx_msg void OnBypassPlug();
	afx_msg void OnRecordAutomation();
	afx_msg void OnRecordMIDIOut();
	afx_msg void OnPassKeypressesToPlug();
	afx_msg void OnSetPreviousVSTPreset();
	afx_msg void OnSetNextVSTPreset();
	afx_msg void OnVSTPresetBackwardJump();
	afx_msg void OnVSTPresetForwardJump();
	afx_msg void OnVSTPresetRename();
	afx_msg void OnCreateInstrument();
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hMenu);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys
	afx_msg LRESULT OnMidiMsg(WPARAM, LPARAM);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnMove(int x, int y);

	// Overridden methods:
	virtual void OnOK() { DoClose(); }
	virtual void OnCancel() { DoClose(); }
	virtual void OnClose() { DoClose(); }

	virtual bool OpenEditor(CWnd *parent);
	virtual void DoClose();
	virtual void UpdateParamDisplays() { if(m_updateDisplay) { SetupMenu(true); m_updateDisplay = false; } }
	virtual void OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized);
	virtual void PostNcDestroy();

	virtual bool IsResizable() const = 0;
	virtual bool SetSize(int contentWidth, int contentHeight) = 0;

	void UpdateDisplay() { m_updateDisplay = true; }

	DECLARE_MESSAGE_MAP()

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void UpdatePresetMenu(bool force = false);
	void GeneratePresetMenu(int32 offset, CMenu &parent);
	void UpdateInputMenu();
	void UpdateOutputMenu();
	void UpdateMacroMenu();
	void UpdateOptionsMenu();
	INSTRUMENTINDEX GetBestInstrumentCandidate() const;
	bool CheckInstrument(INSTRUMENTINDEX ins) const;
	bool ValidateCurrentInstrument();

	void OnToggleEditor(UINT nID);
	void OnSetInputInstrument(UINT nID);
	afx_msg void OnInitMenu(CMenu* pMenu);
	void PrepareToLearnMacro(UINT nID);

	void StoreWindowPos();
	void RestoreWindowPos();

};

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
