#include "stdafx.h"
#include "mptrack.h"
#include "mainfrm.h"
#include "vstplug.h"
#include "fxp.h"
#include "AbstractVstEditor.h"

BEGIN_MESSAGE_MAP(CAbstractVstEditor, CDialog)
	ON_WM_CLOSE()
	ON_COMMAND(ID_PRESET_LOAD,			OnLoadPreset)
	ON_COMMAND(ID_PRESET_SAVE,			OnSavePreset)
	ON_COMMAND(ID_PRESET_RANDOM,		OnRandomizePreset)
END_MESSAGE_MAP()

CAbstractVstEditor::CAbstractVstEditor(CVstPlugin *pPlugin)
{
	m_pVstPlugin = pPlugin;
	m_pMenu = new CMenu();
	m_pPresetMenu = new CMenu();
	m_pMenu->LoadMenu(IDR_VSTMENU);
}

CAbstractVstEditor::~CAbstractVstEditor()
{
#ifdef VST_LOG
	Log("~CVstEditor()\n");
#endif
	if (m_pVstPlugin)
	{
		m_pMenu->DestroyMenu();
		m_pPresetMenu->DestroyMenu();
		m_pVstPlugin->m_pEditor = NULL;
		m_pVstPlugin = NULL;
	}
}

VOID CAbstractVstEditor::OnLoadPreset()
//-------------------------------------
{
	if (m_pVstPlugin)
	{
		CFileDialog dlg(TRUE, "fxp", NULL,
					OFN_HIDEREADONLY| OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_NOREADONLYRETURN,
					"VST FX Program (*.fxp)|*.fxp||",	theApp.m_pMainWnd);
		if (!(dlg.DoModal() == IDOK))	return;

		//TODO: exception handling
		Cfxp fxp(dlg.GetFileName());
		
		if ((fxp.params == NULL) || !(m_pVstPlugin->LoadProgram(&fxp)))
			::AfxMessageBox("Error loading preset. Are you sure it is for this plugin?");
	}
}

VOID CAbstractVstEditor::OnSavePreset()
//-------------------------
{
	if (m_pVstPlugin)
	{
		//Collect required data
		long numParams = m_pVstPlugin->GetNumParameters();
		long ID = m_pVstPlugin->GetUID();
		long plugVersion = m_pVstPlugin->GetVersion();
		float *params = new float[numParams]; 
		m_pVstPlugin->GetParams(params, 0, numParams);

		//Construct fxp
		Cfxp fxp(ID, plugVersion, numParams, params);

		CFileDialog dlg(FALSE, "fxp", NULL,
					OFN_HIDEREADONLY| OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_NOREADONLYRETURN,
					"VST FX Program (*.fxp)|*.fxp||",	theApp.m_pMainWnd);
		if (!(dlg.DoModal() == IDOK))
		{
			delete[] params;
			return;
		}

		fxp.Save(dlg.GetFileName());
		delete[] params;
	}
	return;
}

VOID CAbstractVstEditor::OnRandomizePreset()
//-----------------------------------------
{
	if (m_pVstPlugin)
	{
		if (::AfxMessageBox("Are you sure you want to randomize parameters?\nYou will lose current parameter values.", MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
			m_pVstPlugin->RandomizeParams();
		UpdateAll();
	}
}

VOID CAbstractVstEditor::SetupMenu()
//----------------------------------
{
	/*if (m_pVstPlugin)
	{
		long numProgs = m_pVstPlugin->GetNumPrograms();
		long curProg  = m_pVstPlugin->GetCurrentProgram();
		char s[256];

		m_pPresetMenu->CreatePopupMenu();
		
		for (long p=0; p<numProgs; p++)
		{
			m_pVstPlugin->GetProgramNameIndexed(p, -1, s);
			if (p==curProg)
				m_pPresetMenu->AppendMenu(MF_STRING|MF_CHECKED, 0, (LPCTSTR)s);
			else
				m_pPresetMenu->AppendMenu(MF_STRING, 0, (LPCTSTR)s);
		}

		m_pMenu->InsertMenu(-1, MF_BYPOSITION|MF_POPUP, (UINT) m_pPresetMenu->m_hMenu, (LPCTSTR)"&Programs");
	}*/
	::SetMenu(m_hWnd, m_pMenu->m_hMenu);
}