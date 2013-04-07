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
#include "dlg_misc.h"
#include "AbstractVstEditor.h"
#include "../common/StringFixer.h"
#include "MIDIMacros.h"
#include "VstPresets.h"

#ifndef NO_VST

UINT CAbstractVstEditor::clipboardFormat = RegisterClipboardFormat("VST Preset Data");

BEGIN_MESSAGE_MAP(CAbstractVstEditor, CDialog)
	ON_WM_CLOSE()
	ON_WM_INITMENU()
	ON_WM_MENUSELECT()
	ON_COMMAND(ID_EDIT_COPY,			OnCopyParameters)
	ON_COMMAND(ID_EDIT_PASTE,			OnPasteParameters)
	ON_COMMAND(ID_PRESET_LOAD,			OnLoadPreset)
	ON_COMMAND(ID_PLUG_BYPASS,			OnBypassPlug)
	ON_COMMAND(ID_PLUG_RECORDAUTOMATION,OnRecordAutomation)
	ON_COMMAND(ID_PLUG_RECORD_MIDIOUT,  OnRecordMIDIOut)
	ON_COMMAND(ID_PLUG_PASSKEYS,		OnPassKeypressesToPlug)
	ON_COMMAND(ID_PRESET_SAVE,			OnSavePreset)
	ON_COMMAND(ID_PRESET_RANDOM,		OnRandomizePreset)
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

CAbstractVstEditor::CAbstractVstEditor(CVstPlugin &plugin) : m_VstPlugin(plugin)
{
	m_nCurProg = -1;

	m_Menu.LoadMenu(IDR_VSTMENU);
	m_nInstrument = GetBestInstrumentCandidate();
	m_nLearnMacro = -1;
}

CAbstractVstEditor::~CAbstractVstEditor()
{
#ifdef VST_LOG
	Log("~CVstEditor()\n");
#endif
	m_Menu.DestroyMenu();
	m_PresetMenu.DestroyMenu();
	m_InputMenu.DestroyMenu();
	m_OutputMenu.DestroyMenu();
	m_MacroMenu.DestroyMenu();

	m_OptionsMenu.DestroyMenu();

	for(size_t i = 0; i < m_pPresetMenuGroup.size(); i++)
	{
		if(m_pPresetMenuGroup[i]->m_hMenu)
		{
			m_pPresetMenuGroup[i]->DestroyMenu();
			delete m_pPresetMenuGroup[i];
		}
	}
	m_pPresetMenuGroup.clear();

	m_VstPlugin.m_pEditor = nullptr;
}


void CAbstractVstEditor::OnLoadPreset()
//-------------------------------------
{
	if(m_VstPlugin.LoadProgram())
	{
		UpdatePresetMenu(true);
		UpdatePresetField();
	}
}


void CAbstractVstEditor::OnSavePreset()
//-------------------------------------
{
	m_VstPlugin.SaveProgram();
}


void CAbstractVstEditor::OnCopyParameters()
//-----------------------------------------
{
	if(CMainFrame::GetMainFrame() == nullptr) return;

	BeginWaitCursor();
	std::ostringstream f(std::ios::out | std::ios::binary);
	if(VSTPresets::SaveFile(f, m_VstPlugin, false))
	{
		const std::string data = f.str();

		HGLOBAL hCpy;
		if(CMainFrame::GetMainFrame()->OpenClipboard() && (hCpy = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, data.length())) != nullptr)
		{
			EmptyClipboard();
			LPSTR p = (LPSTR)GlobalLock(hCpy);
			if(p)
			{
				memcpy(p, &data[0], data.length());
			}
			GlobalUnlock(hCpy);
			SetClipboardData(clipboardFormat, (HANDLE) hCpy);
			CloseClipboard();
		}
	}
	EndWaitCursor();
}


void CAbstractVstEditor::OnPasteParameters()
//------------------------------------------
{
	if(CMainFrame::GetMainFrame() == nullptr) return;

	BeginWaitCursor();
	if(CMainFrame::GetMainFrame()->OpenClipboard())
	{
		HGLOBAL hCpy = ::GetClipboardData(clipboardFormat);
		const char *p;

		if(hCpy != nullptr && (p = static_cast<const char *>(GlobalLock(hCpy))) != nullptr)
		{
			FileReader file(p, GlobalSize(hCpy));
			VSTPresets::ErrorCode error = VSTPresets::LoadFile(file, m_VstPlugin);
			GlobalUnlock(hCpy);

			if(error == VSTPresets::noError)
			{
				const CSoundFile &sndFile = m_VstPlugin.GetSoundFile();
				CModDoc *pModDoc;
				if(sndFile.GetModSpecifications().supportsPlugins && (pModDoc = sndFile.GetpModDoc()) != nullptr)
				{
					pModDoc->SetModified();
				}
				UpdatePresetField();
			} else
			{
				Reporting::Error(VSTPresets::GetErrorMessage(error));
			}
		}
		CloseClipboard();
	}
	EndWaitCursor();
}


VOID CAbstractVstEditor::OnRandomizePreset()
//-----------------------------------------
{
	if(Reporting::Confirm("Are you sure you want to randomize parameters?\nYou will lose current parameter values.", false, false, this) == cnfYes)
	{
		m_VstPlugin.RandomizeParams();
		UpdateParamDisplays();
	}
}


void CAbstractVstEditor::SetupMenu(bool force)
//--------------------------------------------
{
	//TODO: create menus on click so they are only updated when required
	UpdatePresetMenu(force);
	UpdateInputMenu();
	UpdateOutputMenu();
	UpdateMacroMenu();
	UpdateOptionsMenu();
	UpdatePresetField();
	::SetMenu(m_hWnd, m_Menu.m_hMenu);
	return;
}


void CAbstractVstEditor::UpdatePresetField()
//------------------------------------------
{
	if(m_VstPlugin.GetNumPrograms() > 0)
	{
		if(m_Menu.GetMenuItemCount() < 5)
		{
			m_Menu.AppendMenu(MF_BYPOSITION, ID_VSTPRESETBACKWARDJUMP, TEXT("<<"));
			m_Menu.AppendMenu(MF_BYPOSITION, ID_PREVIOUSVSTPRESET, TEXT("<"));
			m_Menu.AppendMenu(MF_BYPOSITION, ID_NEXTVSTPRESET, TEXT(">"));
			m_Menu.AppendMenu(MF_BYPOSITION, ID_VSTPRESETFORWARDJUMP, TEXT(">>"));
			m_Menu.AppendMenu(MF_BYPOSITION|MF_DISABLED, 0, TEXT(""));
		}

		m_Menu.ModifyMenu(8, MF_BYPOSITION, 0, m_VstPlugin.GetFormattedProgramName(m_VstPlugin.GetCurrentProgram()));
	}

	DrawMenuBar();

}


void CAbstractVstEditor::OnSetPreset(UINT nID)
//--------------------------------------------
{
	int nIndex = nID - ID_PRESET_SET;
	if(nIndex >= 0)
	{
		m_VstPlugin.SetCurrentProgram(nIndex);
		UpdatePresetField();

		if(m_VstPlugin.GetSoundFile().GetModSpecifications().supportsPlugins)
		{
			m_VstPlugin.GetModDoc()->SetModified();
		}
	}
}


void CAbstractVstEditor::OnBypassPlug()
//-------------------------------------
{
	m_VstPlugin.ToggleBypass();
	if(m_VstPlugin.GetSoundFile().GetModSpecifications().supportsPlugins)
	{
		m_VstPlugin.GetModDoc()->SetModified();
	}
}


void CAbstractVstEditor::OnRecordAutomation()
//-------------------------------------------
{
	m_VstPlugin.m_bRecordAutomation = !m_VstPlugin.m_bRecordAutomation;
}


void CAbstractVstEditor::OnRecordMIDIOut()
//----------------------------------------
{
	m_VstPlugin.m_bRecordMIDIOut = !m_VstPlugin.m_bRecordMIDIOut;
}


void CAbstractVstEditor::OnPassKeypressesToPlug()
//-----------------------------------------------
{
	m_VstPlugin.m_bPassKeypressesToPlug  = !m_VstPlugin.m_bPassKeypressesToPlug;
}


BOOL CAbstractVstEditor::PreTranslateMessage(MSG* pMsg)
//-----------------------------------------------------
{
	if (pMsg)
	{
		//We handle keypresses before Windows has a chance to handle them (for alt etc..)
		if(!m_VstPlugin.m_bPassKeypressesToPlug &&
			(pMsg->message == WM_SYSKEYUP   || pMsg->message == WM_KEYUP ||
			 pMsg->message == WM_SYSKEYDOWN || pMsg->message == WM_KEYDOWN) )
		{

			CInputHandler *ih = (CMainFrame::GetMainFrame())->GetInputHandler();

			//Translate message manually
			UINT nChar = pMsg->wParam;
			UINT nRepCnt = LOWORD(pMsg->lParam);
			UINT nFlags = HIWORD(pMsg->lParam);
			KeyEventType kT = ih->GetKeyEventType(nFlags);

			// If we successfully mapped to a command and plug does not listen for keypresses, no need to pass message on.
			if(ih->KeyEvent(kCtxVSTGUI, nChar, nRepCnt, nFlags, kT, (CWnd*)this) != kcNull)
			{
				return true;
			}

			// Don't forward key repeats if plug does not listen for keypresses
			// (avoids system beeps on note hold)
			if(kT == kKeyEventRepeat)
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
	if(m_VstPlugin.m_pMixStruct)
	{
		CString Title;
		Title.Format("FX %02d: ", m_VstPlugin.m_nSlot + 1);

		if(strcmp(m_VstPlugin.m_pMixStruct->GetName(), ""))
			Title.Append(m_VstPlugin.m_pMixStruct->GetName());
		else
			Title.Append(m_VstPlugin.m_pMixStruct->GetLibraryName());

		SetWindowText(Title);
	}
}


LRESULT CAbstractVstEditor::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
//--------------------------------------------------------------------------
{
	if(wParam == kcNull)
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
			CModDoc* pModDoc = m_VstPlugin.GetModDoc();
			CMainFrame* pMainFrm = CMainFrame::GetMainFrame();
			pModDoc->PlayNote(wParam - kcVSTGUIStartNotes + 1 + pMainFrm->GetBaseOctave() * 12, m_nInstrument, 0, false);
		}
		return wParam;
	}
	if (wParam >= kcVSTGUIStartNoteStops && wParam <= kcVSTGUIEndNoteStops)
	{
		if(ValidateCurrentInstrument())
		{
			CModDoc* pModDoc = m_VstPlugin.GetModDoc();
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
	if(!CheckInstrument(m_nInstrument))
		m_nInstrument = GetBestInstrumentCandidate();

	//only show messagebox if plug is able to process notes.
	if(m_nInstrument == INSTRUMENTINDEX_INVALID)
	{
		if(m_VstPlugin.CanRecieveMidiEvents())
		{
			CModDoc *pModDoc = m_VstPlugin.GetModDoc();
			if(pModDoc == nullptr)
				return false;

			if(!m_VstPlugin.isInstrument() || pModDoc->GetSoundFile()->GetModSpecifications().instrumentsMax == 0 ||
				Reporting::Confirm(_T("You need to assign an instrument to this plugin before you can play notes from here.\nCreate a new instrument and assign this plugin to the instrument?"), false, false) == cnfNo)
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


void CAbstractVstEditor::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU)
//---------------------------------------------------------------------
{
	if(!(nFlags & MF_POPUP))
	{
		return;
	}
	switch(nItemID)
	{
	case 0:
		// Grey out paste menu item.
		m_Menu.EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND | (IsClipboardFormatAvailable(clipboardFormat) ? 0 : MF_GRAYED));
		break;
	case 1:
		// Generate preset menu on click.
		FillPresetMenu();
		break;
	}
}


#define PRESETS_PER_COLUMN 32
#define PRESETS_PER_GROUP 128


void CAbstractVstEditor::FillPresetMenu()
//---------------------------------------
{
	if(m_PresetMenu.GetMenuItemCount() != 0)
	{
		// Already filled...
		return;
	}

	const VstInt32 numProgs = m_VstPlugin.GetNumPrograms();
	const VstInt32 curProg  = m_VstPlugin.GetCurrentProgram();

	const int numSubMenus = ((numProgs - 1) / PRESETS_PER_GROUP) + 1;
	if(numSubMenus > 1)
	{
		// Create sub menus if necessary
		m_pPresetMenuGroup.resize(numSubMenus);
		for(int bank = 0, prog = 1; bank < numSubMenus; bank++, prog += PRESETS_PER_GROUP)
		{
			m_pPresetMenuGroup[bank] = new CMenu();
			m_pPresetMenuGroup[bank]->CreatePopupMenu();

			CString label;
			label.Format("Bank %d (%d-%d)", bank + 1, prog, Util::Min(prog + PRESETS_PER_GROUP - 1, numProgs));
			m_PresetMenu.AppendMenu(MF_POPUP | (bank % 32 == 0 ? MF_MENUBREAK : 0), reinterpret_cast<UINT_PTR>(m_pPresetMenuGroup[bank]->m_hMenu), label);
		}
	}

	int subMenuIndex = 0;
	int entryInThisMenu = 0;
	int entryInThisColumn = 0;

	// If there would be only one sub menu, we add directly to factory menu
	CMenu *targetMenu = (numProgs > PRESETS_PER_GROUP) ? m_pPresetMenuGroup[subMenuIndex] : &m_PresetMenu;

	for(VstInt32 p = 0; p < numProgs; p++)
	{
		CString programName = m_VstPlugin.GetFormattedProgramName(p);
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

	m_nCurProg = curProg;
}


void CAbstractVstEditor::UpdatePresetMenu(bool force)
//---------------------------------------------------
{
	const VstInt32 numProgs = m_VstPlugin.GetNumPrograms();
	const VstInt32 curProg  = m_VstPlugin.GetCurrentProgram();

	if(m_PresetMenu.m_hMenu)						// We rebuild menu from scratch
	{												// So remove any exiting menus...
		if(curProg == m_nCurProg && !force)			// ... unless menu exists and is accurate,
			return;									// in which case we are done.

		for(size_t i = 0; i < m_pPresetMenuGroup.size(); i++)
		{
			// Destroy any submenus
			if (m_pPresetMenuGroup[i]->m_hMenu)
			{
				m_pPresetMenuGroup[i]->DestroyMenu();
				delete m_pPresetMenuGroup[i];
			}
		}
		m_pPresetMenuGroup.clear();

		m_PresetMenu.DestroyMenu();				// Destroy Factory preset menu
		m_Menu.DeleteMenu(1, MF_BYPOSITION);

	}
	if(!m_PresetMenu.m_hMenu)
	{
		// Create Factory preset menu
		m_PresetMenu.CreatePopupMenu();
	}

	// Add Factory menu to main menu
	m_Menu.InsertMenu(1, MF_BYPOSITION | MF_POPUP | (numProgs ? 0 : MF_GRAYED), reinterpret_cast<UINT_PTR>(m_PresetMenu.m_hMenu), "&Presets");

	// Depending on the plugin and its number of presets, creating the menu entries can take quite a while (e.g. Synth1),
	// so we fill the menu only on demand (when it is clicked), so that the editor GUI creation doesn't take forever.
}


void CAbstractVstEditor::UpdateInputMenu()
//----------------------------------------
{
	CMenu *pInfoMenu = m_Menu.GetSubMenu(2);
	pInfoMenu->DeleteMenu(0, MF_BYPOSITION);

	CModDoc* pModDoc = m_VstPlugin.GetModDoc();
	CSoundFile* pSndFile = pModDoc->GetSoundFile();

	if(m_InputMenu.m_hMenu)
	{
		m_InputMenu.DestroyMenu();
	}
	if(!m_InputMenu.m_hMenu)
	{
		m_InputMenu.CreatePopupMenu();
	}

	CString name;

	vector<CVstPlugin *> inputPlugs;
	m_VstPlugin.GetInputPlugList(inputPlugs);
	for(size_t nPlug=0; nPlug < inputPlugs.size(); nPlug++)
	{
		name.Format("FX%02d: %s", inputPlugs[nPlug]->m_nSlot + 1, inputPlugs[nPlug]->m_pMixStruct->GetName());
		m_InputMenu.AppendMenu(MF_STRING, ID_PLUGSELECT + inputPlugs[nPlug]->m_nSlot, name);
	}

	vector<CHANNELINDEX> inputChannels;
	m_VstPlugin.GetInputChannelList(inputChannels);
	for(size_t nChn=0; nChn<inputChannels.size(); nChn++)
	{
		if(nChn == 0 && inputPlugs.size())
		{
			m_InputMenu.AppendMenu(MF_SEPARATOR);
		}
		name.Format("Chn%02d: %s", inputChannels[nChn] + 1, pSndFile->ChnSettings[inputChannels[nChn]].szName);
		m_InputMenu.AppendMenu(MF_STRING, NULL, name);
	}

	vector<INSTRUMENTINDEX> inputInstruments;
	m_VstPlugin.GetInputInstrumentList(inputInstruments);
	for(size_t nIns = 0; nIns<inputInstruments.size(); nIns++)
	{
		bool checked = false;
		if(nIns == 0 && (inputPlugs.size() || inputChannels.size()))
		{
			m_InputMenu.AppendMenu(MF_SEPARATOR);
		}
		name.Format("Ins%02d: %s", inputInstruments[nIns], pSndFile->GetInstrumentName(inputInstruments[nIns]));
		if(inputInstruments[nIns] == m_nInstrument)	checked = true;
		m_InputMenu.AppendMenu(MF_STRING | (checked ? MF_CHECKED : 0), ID_SELECTINST + inputInstruments[nIns], name);
	}

	if(inputPlugs.size() == 0 &&
		inputChannels.size() == 0 &&
		inputInstruments.size() == 0)
	{
		m_InputMenu.AppendMenu(MF_STRING | MF_GRAYED, NULL, "None");
	}

	pInfoMenu->InsertMenu(0, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_InputMenu.m_hMenu), "I&nputs");
}


void CAbstractVstEditor::UpdateOutputMenu()
//-----------------------------------------
{
	CMenu *pInfoMenu = m_Menu.GetSubMenu(2);
	pInfoMenu->DeleteMenu(1, MF_BYPOSITION);

	if(m_OutputMenu.m_hMenu)
	{
		m_OutputMenu.DestroyMenu();
	}
	if(!m_OutputMenu.m_hMenu)
	{
		m_OutputMenu.CreatePopupMenu();
	}

	std::vector<CVstPlugin *> outputPlugs;
	m_VstPlugin.GetOutputPlugList(outputPlugs);
	CString name;

	for(size_t nPlug = 0; nPlug < outputPlugs.size(); nPlug++)
	{
		if(outputPlugs[nPlug] != nullptr)
		{
			name.Format("FX%02d: %s", outputPlugs[nPlug]->m_nSlot + 1,
									outputPlugs[nPlug]->m_pMixStruct->GetName());
			m_OutputMenu.AppendMenu(MF_STRING, ID_PLUGSELECT + outputPlugs[nPlug]->m_nSlot, name);
		} else
		{
			name = "Master Output";
			m_OutputMenu.AppendMenu(MF_STRING | MF_GRAYED, NULL, name);
		}

	}
	pInfoMenu->InsertMenu(1, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_OutputMenu.m_hMenu), "Ou&tputs");
}


void CAbstractVstEditor::UpdateMacroMenu()
//----------------------------------------
{
	CString label, macroName;
	bool greyed;
	int action;

	CModDoc *pModDoc = m_VstPlugin.GetModDoc();
	if(!pModDoc)
	{
		return;
	}

	CMenu *pInfoMenu = m_Menu.GetSubMenu(2);
	pInfoMenu->DeleteMenu(2, MF_BYPOSITION);

	if(m_MacroMenu.m_hMenu)
	{
		m_MacroMenu.DestroyMenu();
	}
	if(!m_MacroMenu.m_hMenu)
	{
		m_MacroMenu.CreatePopupMenu();
	}

	for(int nMacro = 0; nMacro < NUM_MACROS; nMacro++)
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
			macroName = midiCfg.GetParameteredMacroName(nMacro, m_VstPlugin.GetSlot(), *pModDoc->GetSoundFile());
			if(macroType != sfx_plug || macroName.Left(3) != "N/A")
			{
				greyed = false;
			}
		}

		label.Format("SF%X: %s", nMacro, macroName);
		m_MacroMenu.AppendMenu(MF_STRING | (greyed ? MF_GRAYED : 0), action, label);
	}

	pInfoMenu->InsertMenu(2, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_MacroMenu.m_hMenu), "&Macros");
}

void CAbstractVstEditor::UpdateOptionsMenu()
//------------------------------------------
{

	if(m_OptionsMenu.m_hMenu)
		m_OptionsMenu.DestroyMenu();

	CInputHandler *ih = (CMainFrame::GetMainFrame())->GetInputHandler();

	m_OptionsMenu.CreatePopupMenu();

	//Bypass
	m_OptionsMenu.AppendMenu(MF_STRING | m_VstPlugin.IsBypassed() ? MF_CHECKED : 0,
							   ID_PLUG_BYPASS, "&Bypass Plugin\t" + ih->GetKeyTextFromCommand(kcVSTGUIBypassPlug));
	//Record Params
	m_OptionsMenu.AppendMenu(MF_STRING | m_VstPlugin.m_bRecordAutomation ? MF_CHECKED : 0,
							   ID_PLUG_RECORDAUTOMATION, "Record &Parameter Changes\t" + ih->GetKeyTextFromCommand(kcVSTGUIToggleRecordParams));
	//Record MIDI Out
	m_OptionsMenu.AppendMenu(MF_STRING | m_VstPlugin.m_bRecordMIDIOut ? MF_CHECKED : 0,
							   ID_PLUG_RECORD_MIDIOUT, "Record &MIDI Out to Pattern Editor\t" + ih->GetKeyTextFromCommand(kcVSTGUIToggleRecordMIDIOut));
	//Pass on keypresses
	m_OptionsMenu.AppendMenu(MF_STRING | m_VstPlugin.m_bPassKeypressesToPlug ? MF_CHECKED : 0,
							   ID_PLUG_PASSKEYS, "Pass &Keys to Plugin\t" + ih->GetKeyTextFromCommand(kcVSTGUIToggleSendKeysToPlug));


	m_Menu.DeleteMenu(3, MF_BYPOSITION);
	m_Menu.InsertMenu(3, MF_BYPOSITION|MF_POPUP, reinterpret_cast<UINT_PTR>(m_OptionsMenu.m_hMenu), "&Options");

}


void CAbstractVstEditor::OnToggleEditor(UINT nID)
//-----------------------------------------------
{
	CModDoc *pModDoc = m_VstPlugin.GetModDoc();

	if(pModDoc)
	{
		pModDoc->TogglePluginEditor(nID - ID_PLUGSELECT);
	}
}


void CAbstractVstEditor::OnInitMenu(CMenu* /*pMenu*/)
//---------------------------------------------------
{
	SetupMenu();
}


bool CAbstractVstEditor::CheckInstrument(INSTRUMENTINDEX ins)
//-----------------------------------------------------------
{
	const CSoundFile &sndFile = m_VstPlugin.GetSoundFile();

	if(ins != INSTRUMENTINDEX_INVALID && ins < MAX_INSTRUMENTS && sndFile.Instruments[ins] != nullptr)
	{
		return (sndFile.Instruments[ins]->nMixPlug) == (m_VstPlugin.m_nSlot + 1);
	}
	return false;
}


INSTRUMENTINDEX CAbstractVstEditor::GetBestInstrumentCandidate()
//--------------------------------------------------------------
{
	//First try current instrument:
/*	CModDoc* pModDoc = m_pVstPlugin->GetModDoc();
	int currentInst =	//HOW DO WE DO THIS??
	if (CheckInstrument(currentInst)) {
		return currentInst;
	}
*/
	//Then just take the first instrument that points to this plug..
	std::vector<INSTRUMENTINDEX> plugInstrumentList;
	m_VstPlugin.GetInputInstrumentList(plugInstrumentList);
	if(plugInstrumentList.size())
	{
		return static_cast<INSTRUMENTINDEX>(plugInstrumentList[0]);
	}

	//No instrument in the entire track points to this plug.
	return INSTRUMENTINDEX_INVALID;
}


void CAbstractVstEditor::OnSetInputInstrument(UINT nID)
//-----------------------------------------------------
{
	m_nInstrument = static_cast<INSTRUMENTINDEX>(nID - ID_SELECTINST);
}


void CAbstractVstEditor::OnSetPreviousVSTPreset()
//-----------------------------------------------
{
	OnSetPreset(ID_PRESET_SET + m_VstPlugin.GetCurrentProgram() - 1);
}


void CAbstractVstEditor::OnSetNextVSTPreset()
//-------------------------------------------
{
	OnSetPreset(ID_PRESET_SET + m_VstPlugin.GetCurrentProgram() + 1);
}


void CAbstractVstEditor::OnVSTPresetBackwardJump()
//------------------------------------------------
{
	OnSetPreset(max(ID_PRESET_SET + m_VstPlugin.GetCurrentProgram() - 10, ID_PRESET_SET));
}


void CAbstractVstEditor::OnVSTPresetForwardJump()
//-----------------------------------------------
{
	OnSetPreset(min(ID_PRESET_SET + m_VstPlugin.GetCurrentProgram() + 10, ID_PRESET_SET + m_VstPlugin.GetNumPrograms() - 1));
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
	CModDoc *pModDoc = m_VstPlugin.GetModDoc();
	CSoundFile &sndFile = m_VstPlugin.GetSoundFile();
	if(pModDoc == nullptr)
	{
		return false;
	}

	const bool first = (sndFile.GetNumInstruments() == 0);
	INSTRUMENTINDEX nIns = pModDoc->InsertInstrument(0);
	if(nIns == INSTRUMENTINDEX_INVALID)
	{
		return false;
	}

	ModInstrument *pIns = sndFile.Instruments[nIns];
	m_nInstrument = nIns;

	_snprintf(pIns->name, CountOf(pIns->name) - 1, _T("%d: %s"), m_VstPlugin.GetSlot() + 1, sndFile.m_MixPlugins[m_VstPlugin.GetSlot()].GetName());
	StringFixer::CopyN(pIns->filename, sndFile.m_MixPlugins[m_VstPlugin.GetSlot()].GetLibraryName());
	pIns->nMixPlug = (PLUGINDEX)m_VstPlugin.GetSlot() + 1;
	pIns->nMidiChannel = 1;
	// People will forget to change this anyway, so the following lines can lead to some bad surprises after re-opening the module.
	//pIns->wMidiBank = (WORD)((m_pVstPlugin->GetCurrentProgram() >> 7) + 1);
	//pIns->nMidiProgram = (BYTE)((m_pVstPlugin->GetCurrentProgram() & 0x7F) + 1);

	pModDoc->UpdateAllViews(NULL, (nIns << HINT_SHIFT_INS) | HINT_INSTRUMENT | HINT_INSNAMES | HINT_ENVELOPE | (first ? HINT_MODTYPE : 0));
	if(sndFile.GetModSpecifications().supportsPlugins)
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