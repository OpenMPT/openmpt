//rewbs.defaultPlugGUI
#pragma once

class CAbstractVstEditor: public CDialog
{

protected:
	//{{AFX_VIRTUAL(CNoteMapWnd)
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL


public:
	CVstPlugin *m_pVstPlugin;
	int m_nCurProg;
	CAbstractVstEditor(CVstPlugin *pPlugin);
	virtual ~CAbstractVstEditor();
	void SetupMenu();
	void SetTitle();
	void SetLearnMacro(int inMacro);
	int GetLearnMacro();

	afx_msg void OnLoadPreset();
	afx_msg void OnSavePreset();
	afx_msg void OnRandomizePreset();
	afx_msg void OnSetPreset(UINT nID);
	afx_msg void OnMacroInfo();
	afx_msg void OnInputInfo();
	afx_msg void OnBypassPlug();
	afx_msg void OnRecordAutomation();
	afx_msg LRESULT OnCustomKeyMsg(WPARAM, LPARAM); //rewbs.customKeys

	//Overridden methods:
	virtual VOID OnOK()=0;
	virtual VOID OnCancel()=0;
	virtual BOOL OpenEditor(CWnd *parent)=0;
	virtual VOID DoClose()=0;
	virtual void UpdateParamDisplays()=0;
	virtual afx_msg void OnClose()=0;
	DECLARE_MESSAGE_MAP()

private:
	CMenu *m_pMenu;
	CMenu *m_pPresetMenu;
	CArray<CMenu*,CMenu*> m_pPresetMenuGroup;
	CMenu *m_pInputMenu;
	CMenu *m_pOutputMenu;
	CMenu *m_pMacroMenu;

	void UpdatePresetMenu();
	void UpdateInputMenu();
	void UpdateOutputMenu();
	void UpdateMacroMenu();
	int GetBestInstrumentCandidate();
	bool CheckInstrument(int instrument);
	int m_nInstrument;
	int m_nLearnMacro;
	
	void OnToggleEditor(UINT nID);
	void OnSetInputInstrument(UINT nID);
	afx_msg void OnInitMenu(CMenu* pMenu);
	void PrepareToLearnMacro(UINT nID);
};
//end rewbs.defaultPlugGUI