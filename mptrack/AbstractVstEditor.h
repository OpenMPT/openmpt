#pragma once

class CAbstractVstEditor: public CDialog
{
public:
	CVstPlugin *m_pVstPlugin;
	CMenu *m_pMenu;
	CMenu *m_pPresetMenu;

	CAbstractVstEditor(CVstPlugin *pPlugin);
	virtual ~CAbstractVstEditor();
	VOID SetupMenu();
	afx_msg void OnLoadPreset();
	afx_msg void OnSavePreset();
	afx_msg void OnRandomizePreset();


	//Overridden methods:
	virtual VOID OnOK()=0;
	virtual VOID OnCancel()=0;
	virtual BOOL OpenEditor(CWnd *parent)=0;
	virtual VOID DoClose()=0;
	virtual void UpdateAll()=0;
	virtual afx_msg void OnClose()=0;
	
	DECLARE_MESSAGE_MAP()

};