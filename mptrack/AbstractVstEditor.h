/*
 * AbstractVstEditor.h
 * -------------------
 * Purpose: Common plugin editor interface class. This code is shared between custom and default plugin user interfaces.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifndef NO_VST

class CAbstractVstEditor: public CDialog
{

protected:
	//{{AFX_VIRTUAL(CNoteMapWnd)
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL


public:
	CVstPlugin &m_VstPlugin;
	int m_nCurProg;
	CAbstractVstEditor(CVstPlugin &plugin);
	virtual ~CAbstractVstEditor();
	void SetupMenu(bool force = false);
	void SetTitle();
	void SetLearnMacro(int inMacro);
	int GetLearnMacro();

	void UpdatePresetField();
	bool CreateInstrument();

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
	afx_msg void OnCreateInstrument();
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hMenu);
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys

	//Overridden methods:
	virtual void OnOK() = 0;
	virtual void OnCancel() = 0;
	virtual bool OpenEditor(CWnd *parent) = 0;
	virtual void DoClose() = 0;
	virtual void UpdateParamDisplays() = 0;
	virtual afx_msg void OnClose() = 0;

	virtual bool IsResizable() const = 0;
	virtual bool SetSize(int contentWidth, int contentHeight) = 0;

	DECLARE_MESSAGE_MAP()

private:
	CMenu *m_pMenu;
	CMenu *m_pPresetMenu;
	vector<CMenu *> m_pPresetMenuGroup;
	CMenu *m_pInputMenu;
	CMenu *m_pOutputMenu;
	CMenu *m_pMacroMenu;
	CMenu *m_pOptionsMenu;
	static UINT clipboardFormat;

	void FillPresetMenu();
	void UpdatePresetMenu(bool force = false);
	void UpdateInputMenu();
	void UpdateOutputMenu();
	void UpdateMacroMenu();
	void UpdateOptionsMenu();
	INSTRUMENTINDEX GetBestInstrumentCandidate();
	bool CheckInstrument(INSTRUMENTINDEX ins);
	bool ValidateCurrentInstrument();
	INSTRUMENTINDEX m_nInstrument;
	int m_nLearnMacro;
	
	void OnToggleEditor(UINT nID);
	void OnSetInputInstrument(UINT nID);
	afx_msg void OnInitMenu(CMenu* pMenu);
	void PrepareToLearnMacro(UINT nID);

};
//end rewbs.defaultPlugGUI

#endif // NO_VST


