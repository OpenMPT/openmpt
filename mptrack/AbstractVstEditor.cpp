/*
 * AbstractVstEditor.cpp
 * ---------------------
 * Purpose: Common plugin editor interface class. This code is shared between custom and default plugin user interfaces.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "moddoc.h"
#include "mainfrm.h"
#include "sndfile.h"
#include "vstplug.h"
#include "fxp.h"
#include "dlg_misc.h"
#include "AbstractVstEditor.h"
#include "../common/StringFixer.h"
#include "MIDIMacros.h"

#ifndef NO_VST

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
	ON_COMMAND(ID_PLUGINTOINSTRUMENT,	OnCreateInstrument)
	ON_COMMAND_RANGE(ID_PRESET_SET, ID_PRESET_SET + MAX_PLUGPRESETS, OnSetPreset)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	OnCustomKeyMsg) //rewbs.customKeys
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT + MAX_MIXPLUGINS, OnToggleEditor) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_SELECTINST, ID_SELECTINST + MAX_INSTRUMENTS, OnSetInputInstrument) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_LEARN_MACRO_FROM_PLUGGUI, ID_LEARN_MACRO_FROM_PLUGGUI + NUM_MACROS, PrepareToLearnMacro)
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

		for (int i=0; i<m_pPresetMenuGroup.GetSize(); i++)
		{
			if (m_pPresetMenuGroup[i]->m_hMenu)
			{
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
		CMainFrame::GetSettings().GetWorkingDirectory(DIR_PLUGINPRESETS));
	if(files.abort) return;

	CMainFrame::GetSettings().SetWorkingDirectory(files.workingDirectory.c_str(), DIR_PLUGINPRESETS, true);

	//TODO: exception handling to distinguish errors at this level.
	if (m_pVstPlugin->LoadProgram(files.first_file.c_str()))
	{
		if(m_pVstPlugin->GetModDoc() != nullptr)
			m_pVstPlugin->GetModDoc()->SetModified();
	} else
	{
		Reporting::Error("Error loading preset. Are you sure it is for this plugin?");
	}
}

VOID CAbstractVstEditor::OnSavePreset()
//-------------------------------------
{
	if(!m_pVstPlugin) return;

	FileDlgResult files = CTrackApp::ShowOpenSaveFileDialog(false, "fxp", "",
		"VST Program (*.fxp)|*.fxp||",
		CMainFrame::GetSettings().GetWorkingDirectory(DIR_PLUGINPRESETS));
	if(files.abort) return;

	CMainFrame::GetSettings().SetWorkingDirectory(files.workingDirectory.c_str(), DIR_PLUGINPRESETS, true);

	//TODO: exception handling
	if (!(m_pVstPlugin->SaveProgram(files.first_file.c_str())))
		Reporting::Error("Error saving preset.");

}

VOID CAbstractVstEditor::OnRandomizePreset()
//-----------------------------------------
{
	if (m_pVstPlugin)
	{
		if (Reporting::Confirm("Are you sure you want to randomize parameters?\nYou will lose current parameter values.") == cnfYes)
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
	if(m_pVstPlugin->GetNumPrograms() > 0)
	{
		if(m_pMenu->GetMenuItemCount() < 5)
		{
			m_pMenu->AppendMenu(MF_BYPOSITION, ID_VSTPRESETBACKWARDJUMP, TEXT("<<"));
			m_pMenu->AppendMenu(MF_BYPOSITION, ID_PREVIOUSVSTPRESET, TEXT("<"));
			m_pMenu->AppendMenu(MF_BYPOSITION, ID_NEXTVSTPRESET, TEXT(">"));
			m_pMenu->AppendMenu(MF_BYPOSITION, ID_VSTPRESETFORWARDJUMP, TEXT(">>"));
			m_pMenu->AppendMenu(MF_BYPOSITION|MF_DISABLED, 0, TEXT(""));
		}

		m_pMenu->ModifyMenu(8, MF_BYPOSITION, 0, m_pVstPlugin->GetFormattedProgramName(m_pVstPlugin->GetCurrentProgram(), true));
	}
	
	DrawMenuBar();
	
}


void CAbstractVstEditor::OnSetPreset(UINT nID)
//--------------------------------------------
{
	int nIndex = nID - ID_PRESET_SET;
	if (nIndex >= 0)
	{
		m_pVstPlugin->SetCurrentProgram(nIndex);
		UpdatePresetField();
		
		//SetupMenu();
	}
}

void CAbstractVstEditor::OnBypassPlug()
//-------------------------------------
{
	if (m_pVstPlugin)
	{
		m_pVstPlugin->ToggleBypass();
	}
}

void CAbstractVstEditor::OnRecordAutomation()
//-------------------------------------------
{
	if (m_pVstPlugin)
	{
		m_pVstPlugin->m_bRecordAutomation = !m_pVstPlugin->m_bRecordAutomation;
	}
}

void CAbstractVstEditor::OnPassKeypressesToPlug()
//-----------------------------------------------
{
	if (m_pVstPlugin)
	{
		m_pVstPlugin->m_bPassKeypressesToPlug  = !m_pVstPlugin->m_bPassKeypressesToPlug;
	}
}

void CAbstractVstEditor::OnMacroInfo()
{ //TODO	
/*
	for (UINT m=0; m<NUM_MACROS; m++)
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
			 (pMsg->message == WM_SYSKEYDOWN) || (pMsg->message == WM_KEYDOWN)) )
		{

			CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();
			
			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);
			InputTargetContext ctx = (InputTargetContext)(kCtxVSTGUI);
			
			// If we successfully mapped to a command and plug does not listen for keypresses, no need to pass message on.
			if (ih->KeyEvent(ctx, nChar, nRepCnt, nFlags, kT, (CWnd*)this) != kcNull)
			{
				return true; 
			}
			
			// Don't forward key repeats if plug does not listen for keypresses
		    // (avoids system beeps on note hold)
			if (kT == kKeyEventRepeat)
			{
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
		Title.Format("FX %02d: ", m_pVstPlugin->m_nSlot + 1);

		if (strcmp(m_pVstPlugin->m_pMixStruct->GetName(), ""))
			Title.Append(m_pVstPlugin->m_pMixStruct->GetName());
		else
			Title.Append(m_pVstPlugin->m_pMixStruct->GetLibraryName());

		SetWindowText(Title);
	}
}

LRESULT CAbstractVstEditor::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//--------------------------------------------------------------------------
{
	if (wParam == kcNull)
		return NULL;
	
//	CMainFrame *pMainFrm = CMainFrame::GetMainFrame();

	switch(wParam)
	{
		case kcVSTGUIPrevPreset:			OnSetPreviousVSTPreset(); return wParam;
		case kcVSTGUIPrevPresetJump:		OnVSTPresetBackwardJump(); return wParam;
		case kcVSTGUINextPreset:			OnSetNextVSTPreset(); return wParam;
		case kcVSTGUINextPresetJump:		OnVSTPresetForwardJump(); return wParam;
		case kcVSTGUIRandParams:			OnRandomizePreset() ; return wParam;
		case kcVSTGUIToggleRecordParams:	OnRecordAutomation(); return wParam;
		case kcVSTGUIToggleSendKeysToPlug:	OnPassKeypressesToPlug(); return wParam;
		case kcVSTGUIBypassPlug:			OnBypassPlug(); return wParam;
	}
	if (wParam >= kcVSTGUIStartNotes && wParam <= kcVSTGUIEndNotes)
	{
		if(ValidateCurrentInstrument())
		{
			CModDoc* pModDoc     = m_pVstPlugin->GetModDoc();
			CMainFrame* pMainFrm = CMainFrame::GetMainFrame();
			pModDoc->PlayNote(wParam - kcVSTGUIStartNotes + 1 + pMainFrm->GetBaseOctave() * 12, m_nInstrument, 0, false);
		}
		return wParam;
	}
	if (wParam >= kcVSTGUIStartNoteStops && wParam <= kcVSTGUIEndNoteStops)
	{
		if(ValidateCurrentInstrument())
		{
			CModDoc* pModDoc     = m_pVstPlugin->GetModDoc();
			CMainFrame* pMainFrm = CMainFrame::GetMainFrame();
			pModDoc->NoteOff(wParam - kcVSTGUIStartNoteStops + 1 + pMainFrm->GetBaseOctave() * 12, false, m_nInstrument);
		}
		return wParam;
	}

	return NULL;
}


// When trying to play a note using this plugin, but no instrument is assigned to it,
// the user is asked whether a new instrument should be added.
bool CAbstractVstEditor::ValidateCurrentInstrument()
//--------------------------------------------------
{
	if (!CheckInstrument(m_nInstrument))
		m_nInstrument = GetBestInstrumentCandidate();

	//only show messagebox if plug is able to process notes.
	if(m_nInstrument < 0)
	{
		if(m_pVstPlugin->CanRecieveMidiEvents())
		{
			CModDoc *pModDoc = m_pVstPlugin->GetModDoc();
			if(pModDoc == nullptr)
				return false;

			if(!m_pVstPlugin->isInstrument() || pModDoc->GetSoundFile()->GetModSpecifications().instrumentsMax == 0 ||
				Reporting::Confirm(_T("You need to assign an instrument to this plugin before you can play notes from here.\nCreate a new instrument and assign this plugin to the instrument?"), false, false, this) == cnfNo)
			{
				return false;
			} else
			{
				return CreateInstrument();
			}
		} else
		{
			// Can't process notes
			return false;
		}
	} else
	{
		return true;
	}
	
}

#define PRESETS_PER_COLUMN 32
#define PRESETS_PER_GROUP 128

void CAbstractVstEditor::UpdatePresetMenu()
//-----------------------------------------
{
	long numProgs = m_pVstPlugin->GetNumPrograms();
	long curProg  = m_pVstPlugin->GetCurrentProgram();

	if (m_pPresetMenu->m_hMenu)						// We rebuild menu from scratch
	{												// So remove any exiting menus...	
		if (curProg == m_nCurProg)					//.. unless menu exists and is accurate,	
			return;									//in which case we are done.

		for (int i=0; i<m_pPresetMenuGroup.GetSize(); i++)
		{
			//Destroy any submenus
			if (m_pPresetMenuGroup[i]->m_hMenu)
			{
				m_pPresetMenuGroup[i]->DestroyMenu();
				delete m_pPresetMenuGroup[i];
			}
		}
		m_pPresetMenuGroup.RemoveAll();
		m_pPresetMenuGroup.SetSize(0);

		m_pPresetMenu->DestroyMenu();							//Destroy Factory preset menu
		m_pMenu->DeleteMenu(1, MF_BYPOSITION);

	}
	if (!m_pPresetMenu->m_hMenu)
	{
		// Create Factory preset menu
		m_pPresetMenu->CreatePopupMenu();
	}

	const int numSubMenus = ((numProgs - 1) / PRESETS_PER_GROUP) + 1;
	if(numSubMenus > 1)
	{
		// Create sub menus if necessary
		m_pPresetMenuGroup.SetSize(numSubMenus);
		for(int bank = 0, prog = 1; bank < numSubMenus; bank++, prog += PRESETS_PER_GROUP)
		{
			m_pPresetMenuGroup[bank] = new CMenu();
			m_pPresetMenuGroup[bank]->CreatePopupMenu();

			CString label;
			label.Format("Bank %d (%d-%d)", bank + 1, prog, min(prog + PRESETS_PER_GROUP - 1, numProgs));
			m_pPresetMenu->AppendMenu(MF_POPUP | (bank % 32 == 0 ? MF_MENUBREAK : 0), (UINT) m_pPresetMenuGroup[bank]->m_hMenu, label);
		}
	}

	int subMenuIndex = 0;
	int entryInThisMenu = 0;
	int entryInThisColumn = 0;

	// If there would be only one sub menu, we add directly to factory menu
	CMenu *targetMenu = (numProgs > PRESETS_PER_GROUP) ? m_pPresetMenuGroup[subMenuIndex] : m_pPresetMenu;

	for (long p = 0; p < numProgs; p++)
	{
		CString programName = m_pVstPlugin->GetFormattedProgramName(p, p == curProg);
		UINT splitMenuFlag = 0;

		if(entryInThisMenu++ == PRESETS_PER_GROUP)
		{
			// Advance to next preset group (sub menu)
			subMenuIndex++;
			targetMenu = m_pPresetMenuGroup[subMenuIndex];
			entryInThisMenu = 1;
			entryInThisColumn = 1;
		} else if(entryInThisColumn++ == PRESETS_PER_COLUMN)
		{
			// Advance to next menu column
			entryInThisColumn = 1;
			splitMenuFlag = MF_MENUBARBREAK;
		}
		
		targetMenu->AppendMenu(MF_STRING | (p == curProg ? MF_CHECKED : MF_UNCHECKED) | splitMenuFlag, ID_PRESET_SET + p, programName);
	}

	// Add Factory menu to main menu
	m_pMenu->InsertMenu(1, MF_BYPOSITION | MF_POPUP | (numProgs ? 0 : MF_GRAYED), (UINT) m_pPresetMenu->m_hMenu, (LPCTSTR)"&Presets");

	m_nCurProg=curProg;
}

void CAbstractVstEditor::UpdateInputMenu()
//----------------------------------------
{
 	CMenu* pInfoMenu = m_pMenu->GetSubMenu(2);
	pInfoMenu->DeleteMenu(0, MF_BYPOSITION);

	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();
	CSoundFile* pSndFile = pModDoc->GetSoundFile();

	if (m_pInputMenu->m_hMenu)
	{
		m_pInputMenu->DestroyMenu();
	}
	if (!m_pInputMenu->m_hMenu)
	{
		m_pInputMenu->CreatePopupMenu();
	}

	CString name;

	CArray<CVstPlugin*, CVstPlugin*> inputPlugs;
	m_pVstPlugin->GetInputPlugList(inputPlugs);
	for (int nPlug=0; nPlug<inputPlugs.GetSize(); nPlug++)
	{
		name.Format("FX%02d: %s", inputPlugs[nPlug]->m_nSlot + 1, inputPlugs[nPlug]->m_pMixStruct->GetName());
		m_pInputMenu->AppendMenu(MF_STRING, ID_PLUGSELECT + inputPlugs[nPlug]->m_nSlot, name);
	}

	CArray<UINT, UINT> inputChannels;
	m_pVstPlugin->GetInputChannelList(inputChannels);
	for (int nChn=0; nChn<inputChannels.GetSize(); nChn++)
	{
		if (nChn==0 && inputPlugs.GetSize())
		{ 
			m_pInputMenu->AppendMenu(MF_SEPARATOR);
		}
		name.Format("Chn%02d: %s", inputChannels[nChn]+1, pSndFile->ChnSettings[inputChannels[nChn]].szName);
		m_pInputMenu->AppendMenu(MF_STRING, NULL, name);
	}

	CArray<UINT, UINT> inputInstruments;
	m_pVstPlugin->GetInputInstrumentList(inputInstruments);
	bool checked;
	for (int nIns=0; nIns<inputInstruments.GetSize(); nIns++)
	{
		checked=false;
		if (nIns==0 && (inputPlugs.GetSize() || inputChannels.GetSize()))
		{ 
			m_pInputMenu->AppendMenu(MF_SEPARATOR);
		}
		name.Format("Ins%02d: %s", inputInstruments[nIns], (LPCTSTR)pSndFile->GetInstrumentName(inputInstruments[nIns]));
		if (inputInstruments[nIns] == (UINT)m_nInstrument)	checked=true;
		m_pInputMenu->AppendMenu(MF_STRING|(checked?MF_CHECKED:0), ID_SELECTINST+inputInstruments[nIns], name);
	}

	if ((inputPlugs.GetSize() == 0) &&
		(inputChannels.GetSize() == 0) &&
		(inputInstruments.GetSize() == 0))
	{
		m_pInputMenu->AppendMenu(MF_STRING|MF_GRAYED, NULL, "None");
	}

	pInfoMenu->InsertMenu(0, MF_BYPOSITION|MF_POPUP, (UINT)m_pInputMenu->m_hMenu, "I&nputs");
}

void CAbstractVstEditor::UpdateOutputMenu()
//-----------------------------------------
{
 	CMenu* pInfoMenu = m_pMenu->GetSubMenu(2);
	pInfoMenu->DeleteMenu(1, MF_BYPOSITION);

	if (m_pOutputMenu->m_hMenu)
	{
		m_pOutputMenu->DestroyMenu();
	}
	if (!m_pOutputMenu->m_hMenu)
	{
		m_pOutputMenu->CreatePopupMenu();
	}

	CArray<CVstPlugin*, CVstPlugin*> outputPlugs;
	m_pVstPlugin->GetOutputPlugList(outputPlugs);
	CString name;

	for (int nPlug = 0; nPlug < outputPlugs.GetSize(); nPlug++)
	{
		if (outputPlugs[nPlug] != nullptr)
		{
			name.Format("FX%02d: %s", outputPlugs[nPlug]->m_nSlot + 1,
									outputPlugs[nPlug]->m_pMixStruct->GetName());
			m_pOutputMenu->AppendMenu(MF_STRING, ID_PLUGSELECT + outputPlugs[nPlug]->m_nSlot, name);
		} else
		{
			name = "Master Output";
			m_pOutputMenu->AppendMenu(MF_STRING|MF_GRAYED, NULL, name);
		}
		
	}
	pInfoMenu->InsertMenu(1, MF_BYPOSITION|MF_POPUP, (UINT)m_pOutputMenu->m_hMenu, "Ou&tputs");
}

void CAbstractVstEditor::UpdateMacroMenu()
//----------------------------------------
{
	CString label, macroName;
	bool greyed;
	int action;

	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();
	if (!pModDoc)
	{
		return;
	}

 	CMenu* pInfoMenu = m_pMenu->GetSubMenu(2);
	pInfoMenu->DeleteMenu(2, MF_BYPOSITION);	

	if (m_pMacroMenu->m_hMenu)
	{
		m_pMacroMenu->DestroyMenu();
	}
	if (!m_pMacroMenu->m_hMenu)
	{
		m_pMacroMenu->CreatePopupMenu();
	}

	for (int nMacro = 0; nMacro < NUM_MACROS; nMacro++)
	{
		action = NULL;
		greyed = true;

		const MIDIMacroConfig &midiCfg = pModDoc->GetSoundFile()->m_MidiCfg;

 		const parameteredMacroType macroType = midiCfg.GetParameteredMacroType(nMacro);

		if(macroType == sfx_unused)
		{
			macroName = "Unused. Learn Param..."; 
			action= ID_LEARN_MACRO_FROM_PLUGGUI + nMacro;
			greyed = false; 
		} else
		{
			macroName = midiCfg.GetParameteredMacroName(nMacro, m_pVstPlugin->GetSlot(), *pModDoc->GetSoundFile());
			if(macroType != sfx_plug || macroName.Left(3) != "N/A")
			{
				greyed = false;
			}
		}

		label.Format("SF%X: %s", nMacro, macroName);
		m_pMacroMenu->AppendMenu(MF_STRING | (greyed ? MF_GRAYED : 0), action, label);
	}

	pInfoMenu->InsertMenu(2, MF_BYPOSITION | MF_POPUP, (UINT)m_pMacroMenu->m_hMenu, "&Macros");
}

void CAbstractVstEditor::UpdateOptionsMenu()
//------------------------------------------
{

	if (m_pOptionsMenu->m_hMenu)
		m_pOptionsMenu->DestroyMenu();	

	CInputHandler* ih = (CMainFrame::GetMainFrame())->GetInputHandler();

	m_pOptionsMenu->CreatePopupMenu();
	
	//Bypass
	m_pOptionsMenu->AppendMenu(MF_STRING | m_pVstPlugin->IsBypassed() ? MF_CHECKED : 0,
							   ID_PLUG_BYPASS, "&Bypass Plugin\t" + ih->GetKeyTextFromCommand(kcVSTGUIBypassPlug));
	//Record Params
	m_pOptionsMenu->AppendMenu(MF_STRING | m_pVstPlugin->m_bRecordAutomation ? MF_CHECKED : 0,
							   ID_PLUG_RECORDAUTOMATION, "Record &Parameter Changes\t" + ih->GetKeyTextFromCommand(kcVSTGUIToggleRecordParams));
	//Pass on keypresses
	m_pOptionsMenu->AppendMenu(MF_STRING | m_pVstPlugin->m_bPassKeypressesToPlug ? MF_CHECKED : 0,
							   ID_PLUG_PASSKEYS, "Pass &Keys to Plugin\t" + ih->GetKeyTextFromCommand(kcVSTGUIToggleSendKeysToPlug));


	m_pMenu->DeleteMenu(3, MF_BYPOSITION);
	m_pMenu->InsertMenu(3, MF_BYPOSITION|MF_POPUP, (UINT)m_pOptionsMenu->m_hMenu, "&Options");

}


void CAbstractVstEditor::OnToggleEditor(UINT nID)
//-----------------------------------------------
{
	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();

	if (pModDoc)
	{
		pModDoc->TogglePluginEditor(nID-ID_PLUGSELECT);
	}
}


void CAbstractVstEditor::OnInitMenu(CMenu* /*pMenu*/)
//---------------------------------------------------
{
	SetupMenu();
}


bool CAbstractVstEditor::CheckInstrument(int instrument)
//------------------------------------------------------
{
	CSoundFile* pSndFile = m_pVstPlugin->GetSoundFile();
	
	if (instrument >= 0 && instrument<MAX_INSTRUMENTS && pSndFile->Instruments[instrument])
	{
		return (pSndFile->Instruments[instrument]->nMixPlug) == (m_pVstPlugin->m_nSlot + 1);
	}
	return false;
}


int CAbstractVstEditor::GetBestInstrumentCandidate() 
//--------------------------------------------------
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
	if (plugInstrumentList.GetSize())
	{
		return plugInstrumentList[0];
	}

	//No instrument in the entire track points to this plug.
	return -1;
}


void CAbstractVstEditor::OnSetInputInstrument(UINT nID)
//-----------------------------------------------------
{
	m_nInstrument = (nID - ID_SELECTINST);
}


void CAbstractVstEditor::OnSetPreviousVSTPreset()
//-----------------------------------------------
{
	OnSetPreset(ID_PRESET_SET + m_pVstPlugin->GetCurrentProgram() - 1); 
}


void CAbstractVstEditor::OnSetNextVSTPreset()
//-------------------------------------------
{
	OnSetPreset(ID_PRESET_SET + m_pVstPlugin->GetCurrentProgram() + 1);
}


void CAbstractVstEditor::OnVSTPresetBackwardJump()
//------------------------------------------------
{
	OnSetPreset(max(ID_PRESET_SET + m_pVstPlugin->GetCurrentProgram() - 10, ID_PRESET_SET));
}


void CAbstractVstEditor::OnVSTPresetForwardJump()
//-----------------------------------------------
{
	OnSetPreset(min(ID_PRESET_SET + m_pVstPlugin->GetCurrentProgram() + 10, ID_PRESET_SET + m_pVstPlugin->GetNumPrograms() - 1));
}


void CAbstractVstEditor::OnCreateInstrument()
//-------------------------------------------
{
	CreateInstrument();
}


// Try to set up a new instrument that is linked to the current plugin.
bool CAbstractVstEditor::CreateInstrument()
//-----------------------------------------
{
	CModDoc *pModDoc = m_pVstPlugin->GetModDoc();
	CSoundFile *pSndFile = m_pVstPlugin->GetSoundFile();
	if(pModDoc == nullptr || pSndFile == nullptr)
	{
		return false;
	}

	bool bFirst = (pSndFile->GetNumInstruments() == 0);
	INSTRUMENTINDEX nIns = pModDoc->InsertInstrument(0); 
	if(nIns == INSTRUMENTINDEX_INVALID)
	{
		return false;
	}

	ModInstrument *pIns = pSndFile->Instruments[nIns];
	m_nInstrument = nIns;

	_snprintf(pIns->name, CountOf(pIns->name) - 1, _T("%d: %s"), m_pVstPlugin->GetSlot() + 1, pSndFile->m_MixPlugins[m_pVstPlugin->GetSlot()].GetName());
	strncpy(pIns->filename, pSndFile->m_MixPlugins[m_pVstPlugin->GetSlot()].GetLibraryName(), CountOf(pIns->filename) - 1);
	pIns->nMixPlug = (PLUGINDEX)m_pVstPlugin->GetSlot() + 1;
	pIns->nMidiChannel = 1;
	// People will forget to change this anyway, so the following lines can lead to some bad surprises after re-opening the module.
	//pIns->wMidiBank = (WORD)((m_pVstPlugin->GetCurrentProgram() >> 7) + 1);
	//pIns->nMidiProgram = (BYTE)((m_pVstPlugin->GetCurrentProgram() & 0x7F) + 1);

	pModDoc->UpdateAllViews(NULL, (nIns << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_INSNAMES | HINT_ENVELOPE | (bFirst ? HINT_MODTYPE : 0));
	if(pSndFile->GetModSpecifications().supportsPlugins)
	{
		pModDoc->SetModified();
	}

	return true;
}


void CAbstractVstEditor::PrepareToLearnMacro(UINT nID)
//----------------------------------------------------
{
	m_nLearnMacro = (nID-ID_LEARN_MACRO_FROM_PLUGGUI);
	//Now we wait for a param to be touched. We'll get the message from the VST Plug Manager.
	//Then pModDoc->LearnMacro(macro, param) is called
}

void CAbstractVstEditor::SetLearnMacro(int inMacro)
//-------------------------------------------------
{
	if (inMacro < NUM_MACROS)
	{
		m_nLearnMacro=inMacro;
	}
}

int CAbstractVstEditor::GetLearnMacro()
//-------------------------------------
{
	return m_nLearnMacro;
}

#endif // NO_VST

