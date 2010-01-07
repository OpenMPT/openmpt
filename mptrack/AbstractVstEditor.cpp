//rewbs.defaultPlugGUI

#include "stdafx.h"
#include "mptrack.h"
#include "moddoc.h"
#include "mainfrm.h"
#include "sndfile.h"
#include "vstplug.h"
#include "fxp.h"
#include "dlg_misc.h"
#include "AbstractVstEditor.h"

#ifndef NO_VST

static void CreateVerifiedProgramName(const char* rawname, const size_t rnSize,
									  char* name, const size_t nSize,
									  const long p)
//-----------------------------------------------------------------------------
{
	if(rawname[0] < 32) 
	{
		wsprintf(name, "%02d - Program %d",p,p);
	} 
	else 
	{
		size_t k=0;
		while(k < rnSize-1 && rawname[k] != 0 && rawname[k] < 'A' && k<255) k++;
		wsprintf(name, "%02d - %s", p, &rawname[k]);
	}
	name[nSize-1] = 0;
}


BEGIN_MESSAGE_MAP(CAbstractVstEditor, CDialog)
	ON_WM_CLOSE()
	ON_WM_INITMENU()
	ON_COMMAND(ID_PRESET_LOAD,			OnLoadPreset)
	ON_COMMAND(ID_PLUG_BYPASS,			OnBypassPlug)
	ON_COMMAND(ID_PLUG_RECORDAUTOMATION,OnRecordAutomation)
	ON_COMMAND(ID_PLUG_PASSKEYS,		OnPassKeypressesToPlug)
	ON_COMMAND(ID_PRESET_SAVE,			OnSavePreset)
	ON_COMMAND(ID_PRESET_RANDOM,		OnRandomizePreset)
	ON_COMMAND(ID_VSTMACRO_INFO,		OnMacroInfo)
	ON_COMMAND(ID_VSTINPUT_INFO,		OnInputInfo)
	ON_COMMAND(ID_PREVIOUSVSTPRESET,	OnSetPreviousVSTPreset)
	ON_COMMAND(ID_NEXTVSTPRESET,		OnSetNextVSTPreset)
	ON_COMMAND(ID_VSTPRESETBACKWARDJUMP,OnVSTPresetBackwardJump)
	ON_COMMAND(ID_VSTPRESETFORWARDJUMP,	OnVSTPresetForwardJump)
	ON_COMMAND_RANGE(ID_PRESET_SET, ID_PRESET_SET+MAX_PLUGPRESETS, OnSetPreset)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg) //rewbs.customKeys
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT+MAX_MIXPLUGINS, OnToggleEditor) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_SELECTINST, ID_SELECTINST+MAX_INSTRUMENTS, OnSetInputInstrument) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_LEARN_MACRO_FROM_PLUGGUI, ID_LEARN_MACRO_FROM_PLUGGUI+NMACROS, PrepareToLearnMacro)
END_MESSAGE_MAP()

CAbstractVstEditor::CAbstractVstEditor(CVstPlugin *pPlugin)
{
	m_nCurProg = -1;
	m_pVstPlugin = pPlugin;
	m_pMenu = new CMenu();
	m_pInputMenu  = new CMenu();
	m_pOutputMenu = new CMenu();
	m_pMacroMenu  = new CMenu();

	m_pPresetMenu = new CMenu();
	m_pPresetMenuGroup.SetSize(0);

	m_pOptionsMenu  = new CMenu();

	m_pMenu->LoadMenu(IDR_VSTMENU);
	m_nInstrument = GetBestInstrumentCandidate();
	m_nLearnMacro = -1;
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
		m_pInputMenu->DestroyMenu();
		m_pOutputMenu->DestroyMenu();
		m_pMacroMenu->DestroyMenu();

		m_pOptionsMenu->DestroyMenu();

		delete m_pMenu;
		delete m_pPresetMenu;
		delete m_pInputMenu;
		delete m_pOutputMenu;
		delete m_pMacroMenu;
		delete m_pOptionsMenu;

		for (int i=0; i<m_pPresetMenuGroup.GetSize(); i++) {
			if (m_pPresetMenuGroup[i]->m_hMenu) {
				m_pPresetMenuGroup[i]->DestroyMenu();
				delete m_pPresetMenuGroup[i];
			}
		}
		m_pPresetMenuGroup.RemoveAll();

		m_pVstPlugin->m_pEditor = NULL;
		m_pVstPlugin = NULL;
	}
}

VOID CAbstractVstEditor::OnLoadPreset()
//-------------------------------------
{
	if(!m_pVstPlugin) return;

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(true, "fxp", "",
		"VST Program (*.fxp)|*.fxp||",
		CMainFrame::GetWorkingDirectory(DIR_PLUGINPRESETS));
	if(files.abort) return;

	CMainFrame::SetWorkingDirectory(files.workingDirectory.c_str(), DIR_PLUGINPRESETS, true);

	//TODO: exception handling to distinguish errors at this level.
	if (!(m_pVstPlugin->LoadProgram(files.first_file.c_str())))
		::AfxMessageBox("Error loading preset. Are you sure it is for this plugin?");
}

VOID CAbstractVstEditor::OnSavePreset()
//-------------------------------------
{
	if(!m_pVstPlugin) return;

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "fxp", "",
		"VST Program (*.fxp)|*.fxp||",
		CMainFrame::GetWorkingDirectory(DIR_PLUGINPRESETS));
	if(files.abort) return;

	CMainFrame::SetWorkingDirectory(files.workingDirectory.c_str(), DIR_PLUGINPRESETS, true);

	//TODO: exception handling
	if (!(m_pVstPlugin->SaveProgram(files.first_file.c_str())))
		::AfxMessageBox("Error saving preset.");
}

VOID CAbstractVstEditor::OnRandomizePreset()
//-----------------------------------------
{
	if (m_pVstPlugin)
	{
		if (::AfxMessageBox("Are you sure you want to randomize parameters?\nYou will lose current parameter values.", MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
			m_pVstPlugin->RandomizeParams();
		UpdateParamDisplays();
	}
}

VOID CAbstractVstEditor::SetupMenu()
//----------------------------------
{
	//TODO: create menus on click so they are only updated when required
	if (m_pVstPlugin)
	{
		UpdatePresetMenu();
		UpdateInputMenu();
		UpdateOutputMenu();
		UpdateMacroMenu();
		UpdateOptionsMenu();
		UpdatePresetField();
		::SetMenu(m_hWnd, m_pMenu->m_hMenu);
	}
	return;
}

void CAbstractVstEditor::UpdatePresetField()
//------------------------------------------
{
	
	if(m_pVstPlugin->GetNumPrograms() > 0 && m_pMenu->GetMenuItemCount() < 5)
	{
		m_pMenu->AppendMenu(MF_BYPOSITION, ID_VSTPRESETBACKWARDJUMP, TEXT("<<"));
		m_pMenu->AppendMenu(MF_BYPOSITION, ID_PREVIOUSVSTPRESET, TEXT("<"));
		m_pMenu->AppendMenu(MF_BYPOSITION, ID_NEXTVSTPRESET, TEXT(">"));
		m_pMenu->AppendMenu(MF_BYPOSITION, ID_VSTPRESETFORWARDJUMP, TEXT(">>"));
		m_pMenu->AppendMenu(MF_BYPOSITION|MF_DISABLED, 0, TEXT(""));
	}
	
	long index = m_pVstPlugin->GetCurrentProgram();
	char name[266];
	char rawname[256];
	m_pVstPlugin->GetProgramNameIndexed(index, -1, rawname);
	rawname[sizeof(rawname)-1] = 0;
	CreateVerifiedProgramName(rawname, sizeof(rawname), name, sizeof(name), index);

	m_pMenu->ModifyMenu(8, MF_BYPOSITION, 0, name);

	DrawMenuBar();
	
}


void CAbstractVstEditor::OnSetPreset(UINT nID)
//---------------------------------------------
{
	int nIndex=nID-ID_PRESET_SET;
	if (nIndex>=0)
	{
		m_pVstPlugin->SetCurrentProgram(nIndex);
		UpdatePresetField();
		
		//SetupMenu();
	}
}

void CAbstractVstEditor::OnBypassPlug()
//-------------------------------------
{
	if (m_pVstPlugin) {
		m_pVstPlugin->Bypass();
	}
}

void CAbstractVstEditor::OnRecordAutomation()
//-------------------------------------------
{
	if (m_pVstPlugin) {
		m_pVstPlugin->m_bRecordAutomation = !m_pVstPlugin->m_bRecordAutomation;
	}
}

void CAbstractVstEditor::OnPassKeypressesToPlug()
//-----------------------------------------------
{
	if (m_pVstPlugin) {
		m_pVstPlugin->m_bPassKeypressesToPlug  = !m_pVstPlugin->m_bPassKeypressesToPlug;
	}
}

void CAbstractVstEditor::OnMacroInfo()
{ //TODO	
/*
	for (UINT m=0; m<NMACROS; m++)
	{
	}
*/
}

void CAbstractVstEditor::OnInputInfo()
{ //TODO
}
//end rewbs.defaultPlugGUI

BOOL CAbstractVstEditor::PreTranslateMessage(MSG* pMsg)
//----------------------------------------------------
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if ( (!m_pVstPlugin->m_bPassKeypressesToPlug) &&  
			((pMsg->message == WM_SYSKEYUP)   || (pMsg->message == WM_KEYUP) || 
			 (pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN)) )	{

			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();
			
			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxVSTGUI);
			
			// If we successfully mapped to a command and plug does not listen for keypresses, no need to pass message on.
			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT, (CWnd*)this) != kcNull) {
				return true; 
			}
			
			// Don't forward key repeats if plug does not listen for keypresses
		    // (avoids system beeps on note hold)
			if (kT == kKeyEventRepeat) {
				return true;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);

}

void CAbstractVstEditor::SetTitle() 
//---------------------------------
{
	if (m_pVstPlugin && m_pVstPlugin->m_pMixStruct)
	{
		CString Title; 
		Title.Format("FX %02d:  ", m_pVstPlugin->m_nSlot+1);

		if (m_pVstPlugin->m_pMixStruct->Info.szName[0]) {
			Title.Append(m_pVstPlugin->m_pMixStruct->Info.szName);
		} else {
			Title.Append(m_pVstPlugin->m_pMixStruct->Info.szLibraryName);
		}

		SetWindowText(Title);
	}
}

LRESULT CAbstractVstEditor::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//----------------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
//	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	switch(wParam)
	{
		case kcVSTGUIPrevPreset:	OnSetPreviousVSTPreset(); return wParam;
		case kcVSTGUIPrevPresetJump:OnVSTPresetBackwardJump(); return wParam;
		case kcVSTGUINextPreset:	OnSetNextVSTPreset(); return wParam;
		case kcVSTGUINextPresetJump:OnVSTPresetForwardJump(); return wParam;
		case kcVSTGUIRandParams:	OnRandomizePreset() ; return wParam;
		
		
	}
	if (wParam>=kcVSTGUIStartNotes && wParam<=kcVSTGUIEndNotes)
	{
		if (!CheckInstrument(m_nInstrument)) {
			m_nInstrument = GetBestInstrumentCandidate();
		}
		
		if (m_nInstrument<0 && m_pVstPlugin->CanRecieveMidiEvents()) {	//only send warning if plug is able to process notes.
			AfxMessageBox("You need to assign an instrument to this plugin before you can play notes from here.");
		} else {
			CModDoc* pModDoc     = m_pVstPlugin->GetModDoc();
			CMainFrame* pMainFrm = CMainFrame::GetMainFrame();
			pModDoc->PlayNote(wParam-kcVSTGUIStartNotes+1+pMainFrm->GetBaseOctave()*12, m_nInstrument, 0, FALSE);
		}
		return wParam;
	}
	if (wParam>=kcVSTGUIStartNoteStops && wParam<=kcVSTGUIEndNoteStops)
	{
		if (!CheckInstrument(m_nInstrument)) {
			m_nInstrument = GetBestInstrumentCandidate();
		}
		if (m_nInstrument<0 && m_pVstPlugin->CanRecieveMidiEvents()) {	//only send warning if plug is able to process notes.
			AfxMessageBox("You need to assign an instrument to this plugin before you can play notes from here.");
		} else {	
			CModDoc* pModDoc     = m_pVstPlugin->GetModDoc();
			CMainFrame* pMainFrm = CMainFrame::GetMainFrame();
			pModDoc->NoteOff(wParam-kcVSTGUIStartNoteStops+1+pMainFrm->GetBaseOctave()*12, FALSE, m_nInstrument);
		}
		return wParam;
	}

	return NULL;
}

#define PRESETS_PER_COLUMN 25
#define PRESETS_PER_GROUP 100

void CAbstractVstEditor::UpdatePresetMenu()
{
	long numProgs = m_pVstPlugin->GetNumPrograms();
	long curProg  = m_pVstPlugin->GetCurrentProgram();
	char s[266];
	char sname[256];

	

	if (m_pPresetMenu->m_hMenu)						// We rebuild menu from scratch
	{												// So remove any exiting menus...	
		if (curProg == m_nCurProg)					//.. unless menu exists and is accurate,	
			return;									//in which case we are done.

		for (int i=0; i<m_pPresetMenuGroup.GetSize(); i++) {	//Destroy any submenus
			if (m_pPresetMenuGroup[i]->m_hMenu) {
				m_pPresetMenuGroup[i]->DestroyMenu();
				delete m_pPresetMenuGroup[i];
			}
		}
		m_pPresetMenuGroup.RemoveAll();
		m_pPresetMenuGroup.SetSize(0);

		m_pPresetMenu->DestroyMenu();							//Destroy Factory preset menu
		m_pMenu->DeleteMenu(1, MF_BYPOSITION);

	}
	if (!m_pPresetMenu->m_hMenu) {					// Create Factory preset menu
		m_pPresetMenu->CreatePopupMenu();
	}

	for (long p=0; p<numProgs; p++) {
		m_pVstPlugin->GetProgramNameIndexed(p, -1, sname);

		sname[sizeof(sname)-1] = 0;
		CreateVerifiedProgramName(sname, sizeof(sname), s, sizeof(s), p);

		// Get menu item properties
 		bool checkedItem = (p==curProg);			
		bool splitMenu   = (p%PRESETS_PER_COLUMN==0 && p%PRESETS_PER_GROUP!=0);
		int subMenuIndex = p/PRESETS_PER_GROUP;
		
		if (numProgs>PRESETS_PER_GROUP) { // If necessary, add to appropriate submenu.
			if (subMenuIndex >= m_pPresetMenuGroup.GetSize()) {	//create new submenu if required.
				m_pPresetMenuGroup.SetSize(m_pPresetMenuGroup.GetSize()+1);
				m_pPresetMenuGroup[subMenuIndex] = new CMenu();
				m_pPresetMenuGroup[subMenuIndex]->CreatePopupMenu();

				CString label;
				label.Format("Presets %d-%d", subMenuIndex*PRESETS_PER_GROUP, min((subMenuIndex+1)*PRESETS_PER_GROUP-1, numProgs));
				m_pPresetMenu->AppendMenu(MF_POPUP, (UINT) m_pPresetMenuGroup[subMenuIndex]->m_hMenu, label);
			}
			m_pPresetMenuGroup[subMenuIndex]->AppendMenu(MF_STRING|(checkedItem?MF_CHECKED:0)|(splitMenu?MF_MENUBARBREAK:0), ID_PRESET_SET+p, (LPCTSTR)s);
		} 
		else {							  // If there would only be 1 submenu, we add directly to factory menu
			m_pPresetMenu->AppendMenu(MF_STRING|(checkedItem?MF_CHECKED:0)|(splitMenu?MF_MENUBARBREAK:0), ID_PRESET_SET+p, (LPCTSTR)s);
		}
	}

	for (int i=0; i<m_pPresetMenuGroup.GetSize(); i++) {
	}

	//Add Factory menu to main menu
	if (numProgs) {
		m_pMenu->InsertMenu(1, MF_BYPOSITION|MF_POPUP, (UINT) m_pPresetMenu->m_hMenu, (LPCTSTR)"&Presets");
	} else {
		m_pMenu->InsertMenu(1, MF_BYPOSITION|MF_POPUP|MF_GRAYED, (UINT) m_pPresetMenu->m_hMenu, (LPCTSTR)"&Presets");
	}

	m_nCurProg=curProg;
}

void CAbstractVstEditor::UpdateInputMenu()
{
 	CMenu* pInfoMenu = m_pMenu->GetSubMenu(2);
	pInfoMenu->DeleteMenu(0, MF_BYPOSITION);

	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();
	CSoundFile* pSndFile = pModDoc->GetSoundFile();

	if (m_pInputMenu->m_hMenu)	{
		m_pInputMenu->DestroyMenu();
	}
	if (!m_pInputMenu->m_hMenu) {
		m_pInputMenu->CreatePopupMenu();
	}

	CString name;

	CArray<CVstPlugin*, CVstPlugin*> inputPlugs;
	m_pVstPlugin->GetInputPlugList(inputPlugs);
	for (int nPlug=0; nPlug<inputPlugs.GetSize(); nPlug++) {
		name.Format("FX%02d: %s", inputPlugs[nPlug]->m_nSlot+1, inputPlugs[nPlug]->m_pMixStruct->Info.szName);
		m_pInputMenu->AppendMenu(MF_STRING, ID_PLUGSELECT+inputPlugs[nPlug]->m_nSlot, name);
	}

	CArray<UINT, UINT> inputChannels;
	m_pVstPlugin->GetInputChannelList(inputChannels);
	for (int nChn=0; nChn<inputChannels.GetSize(); nChn++) {
		if (nChn==0 && inputPlugs.GetSize()) { 
			m_pInputMenu->AppendMenu(MF_SEPARATOR);
		}
		name.Format("Chn%02d: %s", inputChannels[nChn]+1, pSndFile->ChnSettings[inputChannels[nChn]].szName);
		m_pInputMenu->AppendMenu(MF_STRING, NULL, name);
	}

	CArray<UINT, UINT> inputInstruments;
	m_pVstPlugin->GetInputInstrumentList(inputInstruments);
	bool checked;
	for (int nIns=0; nIns<inputInstruments.GetSize(); nIns++) {
		checked=false;
		if (nIns==0 && (inputPlugs.GetSize() || inputChannels.GetSize())) { 
			m_pInputMenu->AppendMenu(MF_SEPARATOR);
		}
		name.Format("Ins%02d: %s", inputInstruments[nIns], (LPCTSTR)pSndFile->GetInstrumentName(inputInstruments[nIns]));
		if (inputInstruments[nIns] == (UINT)m_nInstrument)	checked=true;
		m_pInputMenu->AppendMenu(MF_STRING|(checked?MF_CHECKED:0), ID_SELECTINST+inputInstruments[nIns], name);
	}

	if ((inputPlugs.GetSize() == 0) &&
		(inputChannels.GetSize() == 0) &&
		(inputInstruments.GetSize() == 0)) {
		m_pInputMenu->AppendMenu(MF_STRING|MF_GRAYED, NULL, "None");
	}

	pInfoMenu->InsertMenu(0, MF_BYPOSITION|MF_POPUP, (UINT)m_pInputMenu->m_hMenu, "I&nputs");
}

void CAbstractVstEditor::UpdateOutputMenu()
{
 	CMenu* pInfoMenu = m_pMenu->GetSubMenu(2);
	pInfoMenu->DeleteMenu(1, MF_BYPOSITION);

	if (m_pOutputMenu->m_hMenu)	{
		m_pOutputMenu->DestroyMenu();
	}
	if (!m_pOutputMenu->m_hMenu) {
		m_pOutputMenu->CreatePopupMenu();
	}

	CArray<CVstPlugin*, CVstPlugin*> outputPlugs;
	m_pVstPlugin->GetOutputPlugList(outputPlugs);
	CString name;

	for (int nPlug=0; nPlug<outputPlugs.GetSize(); nPlug++) {
		if (outputPlugs[nPlug] != NULL) {
			name.Format("FX%02d: %s", outputPlugs[nPlug]->m_nSlot+1,
									outputPlugs[nPlug]->m_pMixStruct->Info.szName);
			m_pOutputMenu->AppendMenu(MF_STRING, ID_PLUGSELECT+outputPlugs[nPlug]->m_nSlot, name);
		} else {
			name = "Master Output";
			m_pOutputMenu->AppendMenu(MF_STRING|MF_GRAYED, NULL, name);
		}
		
	}
	pInfoMenu->InsertMenu(1, MF_BYPOSITION|MF_POPUP, (UINT)m_pOutputMenu->m_hMenu, "Ou&tputs");
}

void CAbstractVstEditor::UpdateMacroMenu() {
//------------------------------------------

	CString label, macroName, macroText;
	char paramName[128];
	bool greyed;
	int macroType,nParam,action;

	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();
	if (!pModDoc) {
		return;
	}

 	CMenu* pInfoMenu = m_pMenu->GetSubMenu(2);
	pInfoMenu->DeleteMenu(2, MF_BYPOSITION);	

	if (m_pMacroMenu->m_hMenu) {
		m_pMacroMenu->DestroyMenu();
	}
	if (!m_pMacroMenu->m_hMenu) {
		m_pMacroMenu->CreatePopupMenu();
	}

	for (int nMacro=0; nMacro<NMACROS; nMacro++)	{
		action=NULL;
		greyed=true;
		macroText = &(pModDoc->GetSoundFile()->m_MidiCfg.szMidiSFXExt[nMacro*32]);
 		macroType = pModDoc->GetMacroType(macroText);

		switch (macroType)	{
			case sfx_unused: 
				macroName = "Unused. Learn Param..."; 
				action=ID_LEARN_MACRO_FROM_PLUGGUI+nMacro;
				greyed=false; 
				break;
			case sfx_cutoff: 
				macroName = "Set Filter Cutoff"; 
				break;
			case sfx_reso: 
				macroName = "Set Filter Resonance"; 
				break;
			case sfx_mode: macroName = "Set Filter Mode"; 
				break;
			case sfx_drywet: 
				macroName = "Set plugin dry/wet ratio"; 
				greyed=false; 
				break;
			case sfx_plug: 
			{	
		 		nParam = pModDoc->MacroToPlugParam(macroText);
				m_pVstPlugin->GetParamName(nParam, paramName, sizeof(paramName));
				
				if (paramName[0] == 0) {
					strcpy(paramName, "N/A for this plug");
				} else {
					greyed=false;
				}
			
				macroName.Format("%d - %s", nParam, paramName); 
				break;
			}
			case sfx_custom: 
			default: 
				macroName.Format("Custom: %s", macroText);
				greyed=false;

		}
		label.Format("SF%X: %s", nMacro, macroName);
		m_pMacroMenu->AppendMenu(MF_STRING|(greyed?MF_GRAYED:0), action, label);
	}

	pInfoMenu->InsertMenu(2, MF_BYPOSITION|MF_POPUP, (UINT)m_pMacroMenu->m_hMenu, "&Macros");
}

void CAbstractVstEditor::UpdateOptionsMenu() {
//--------------------------------------------

	if (m_pOptionsMenu->m_hMenu) {
		m_pOptionsMenu->DestroyMenu();	
	}

	m_pOptionsMenu->CreatePopupMenu();
	
	//Bypass
	m_pOptionsMenu->AppendMenu(MF_STRING | m_pVstPlugin->IsBypassed()?MF_CHECKED:0,
							   ID_PLUG_BYPASS, "&Bypass");
	//Record Params
	m_pOptionsMenu->AppendMenu(MF_STRING | m_pVstPlugin->m_bRecordAutomation?MF_CHECKED:0,
							   ID_PLUG_RECORDAUTOMATION, "Record &Params");
	//Pass on keypresses
	m_pOptionsMenu->AppendMenu(MF_STRING | m_pVstPlugin->m_bPassKeypressesToPlug?MF_CHECKED:0,
							   ID_PLUG_PASSKEYS, "Pass &Keys to Plug");


	m_pMenu->DeleteMenu(3, MF_BYPOSITION);
	m_pMenu->InsertMenu(3, MF_BYPOSITION|MF_POPUP, (UINT)m_pOptionsMenu->m_hMenu, "&Options");

}

void CAbstractVstEditor::OnToggleEditor(UINT nID)
{
	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();

	if (pModDoc) {
		pModDoc->TogglePluginEditor(nID-ID_PLUGSELECT);
	}
}

void CAbstractVstEditor::OnInitMenu(CMenu* /*pMenu*/)
//---------------------------------------------------
{
	//AfxMessageBox("");
	SetupMenu();
}

bool CAbstractVstEditor::CheckInstrument(int instrument)
//------------------------------------------------------
{
	CSoundFile* pSndFile = m_pVstPlugin->GetSoundFile();
	
	if (instrument>=0 && instrument<MAX_INSTRUMENTS && pSndFile->Instruments[instrument]) {
		return (pSndFile->Instruments[instrument]->nMixPlug) == (m_pVstPlugin->m_nSlot+1);
	}
	return false;
}

int CAbstractVstEditor::GetBestInstrumentCandidate() 
{
	//First try current instrument:
/*	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();
	int currentInst =	//HOW DO WE DO THIS??
	if (CheckInstrument(currentInst)) {
		return currentInst;
	}
*/
	//Then just take the first instrument that points to this plug..
	CArray<UINT, UINT> plugInstrumentList;
	m_pVstPlugin->GetInputInstrumentList(plugInstrumentList);
	if (plugInstrumentList.GetSize()) {
		return plugInstrumentList[0];
	}

	//No instrument in the entire track points to this plug.
	return -1;
}

void CAbstractVstEditor::OnSetInputInstrument(UINT nID)
{
	m_nInstrument = (nID-ID_SELECTINST);
}

void CAbstractVstEditor::OnSetPreviousVSTPreset()
//--------------------------------------------
{
	OnSetPreset(-1+ID_PRESET_SET+m_pVstPlugin->GetCurrentProgram()); 
}

void CAbstractVstEditor::OnSetNextVSTPreset()
//----------------------------------------
{
	OnSetPreset(1+ID_PRESET_SET+m_pVstPlugin->GetCurrentProgram());
}

void CAbstractVstEditor::OnVSTPresetBackwardJump()
//------------------------------------------------
{
	OnSetPreset(max(ID_PRESET_SET+m_pVstPlugin->GetCurrentProgram()-10, ID_PRESET_SET));
}

void CAbstractVstEditor::OnVSTPresetForwardJump()
//----------------------------------------------------
{
	OnSetPreset(min(10+ID_PRESET_SET+m_pVstPlugin->GetCurrentProgram(), ID_PRESET_SET+m_pVstPlugin->GetNumPrograms()-1));
}

void CAbstractVstEditor::PrepareToLearnMacro(UINT nID)
{
	m_nLearnMacro = (nID-ID_LEARN_MACRO_FROM_PLUGGUI);
	//Now we wait for a param to be touched. We'll get the message from the VST Plug Manager.
	//Then pModDoc->LearnMacro(macro, param) is called
}

void CAbstractVstEditor::SetLearnMacro(int inMacro) {
	if (inMacro<NMACROS) {
		m_nLearnMacro=inMacro;
	}
}

int CAbstractVstEditor::GetLearnMacro() {
	return m_nLearnMacro;
}

#endif // NO_VST

