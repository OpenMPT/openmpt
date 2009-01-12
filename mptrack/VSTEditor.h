#pragma once
#include "mptrack.h"
#include "MainFrm.h"
#include "VstPlug.h"
#include "AbstractVstEditor.h"

#ifndef NO_VST

//==============================
class COwnerVstEditor: public CAbstractVstEditor
//==============================
{
protected:
/*	CVstPlugin *m_pVstPlugin;
	CMenu *m_pMenu;
	CMenu *m_pPresetMenu;
*/

public:
	COwnerVstEditor(CVstPlugin *pPlugin);
	virtual ~COwnerVstEditor();
	virtual VOID OnOK();
	virtual VOID OnCancel();
	afx_msg void OnLoadPreset();
	afx_msg void OnSavePreset();
	afx_msg void OnRandomizePreset();

	//Overridden:
	void UpdateParamDisplays() {m_pVstPlugin->Dispatch(effEditIdle, 0,0, NULL, 0); };	//we trust that the plugin GUI can update it's display with a bit of idle time.
	afx_msg void OnClose();
	BOOL OpenEditor(CWnd *parent);
	VOID DoClose();
};

#endif // NO_VST

