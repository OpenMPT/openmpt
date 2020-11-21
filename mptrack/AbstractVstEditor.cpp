/*
 * AbstractVstEditor.cpp
 * ---------------------
 * Purpose: Common plugin editor interface class. This code is shared between custom and default plugin user interfaces.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Clipboard.h"
#include "../soundlib/Sndfile.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/plugins/PlugInterface.h"
#include "../soundlib/plugins/PluginManager.h"
#include "Vstplug.h"
#include "dlg_misc.h"
#include "AbstractVstEditor.h"
#include "../common/mptStringBuffer.h"
#include "MIDIMacros.h"
#include "VstPresets.h"
#include "../common/FileReader.h"
#include "InputHandler.h"
#include "dlg_misc.h"
#include <sstream>
#include "Globals.h"


OPENMPT_NAMESPACE_BEGIN


#ifndef NO_PLUGINS

CAbstractVstEditor::WindowSizeAdjuster::WindowSizeAdjuster(CWnd &wnd)
	: m_wnd(wnd)
{
	MENUBARINFO mbi = { sizeof(mbi) };
	if(GetMenuBarInfo(m_wnd, OBJID_MENU, 0, &mbi))
		m_menuHeight = (mbi.rcBar.bottom - mbi.rcBar.top);
}

CAbstractVstEditor::WindowSizeAdjuster::~WindowSizeAdjuster()
{
	// Extend window height by the menu size if it changed
	MENUBARINFO mbi = { sizeof(mbi) };
	if(GetMenuBarInfo(m_wnd, OBJID_MENU, 0, &mbi))
	{
		CRect windowRect;
		m_wnd.GetWindowRect(&windowRect);
		windowRect.bottom += (mbi.rcBar.bottom - mbi.rcBar.top) - m_menuHeight;
		m_wnd.SetWindowPos(nullptr, 0, 0,
			windowRect.Width(), windowRect.Height(),
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}
}

#define PRESETS_PER_COLUMN 32
#define PRESETS_PER_GROUP 128

UINT CAbstractVstEditor::m_clipboardFormat = RegisterClipboardFormat(_T("VST Preset Data"));

BEGIN_MESSAGE_MAP(CAbstractVstEditor, CDialog)
	ON_WM_CLOSE()
	ON_WM_INITMENU()
	ON_WM_MENUSELECT()
	ON_WM_ACTIVATE()
	ON_WM_DROPFILES()
	ON_WM_MOVE()
	ON_WM_NCLBUTTONDBLCLK()
	ON_COMMAND(ID_EDIT_COPY,			&CAbstractVstEditor::OnCopyParameters)
	ON_COMMAND(ID_EDIT_PASTE,			&CAbstractVstEditor::OnPasteParameters)
	ON_COMMAND(ID_PRESET_LOAD,			&CAbstractVstEditor::OnLoadPreset)
	ON_COMMAND(ID_PLUG_BYPASS,			&CAbstractVstEditor::OnBypassPlug)
	ON_COMMAND(ID_PLUG_RECORDAUTOMATION,&CAbstractVstEditor::OnRecordAutomation)
	ON_COMMAND(ID_PLUG_RECORD_MIDIOUT,  &CAbstractVstEditor::OnRecordMIDIOut)
	ON_COMMAND(ID_PLUG_PASSKEYS,		&CAbstractVstEditor::OnPassKeypressesToPlug)
	ON_COMMAND(ID_PRESET_SAVE,			&CAbstractVstEditor::OnSavePreset)
	ON_COMMAND(ID_PRESET_RANDOM,		&CAbstractVstEditor::OnRandomizePreset)
	ON_COMMAND(ID_PREVIOUSVSTPRESET,	&CAbstractVstEditor::OnSetPreviousVSTPreset)
	ON_COMMAND(ID_NEXTVSTPRESET,		&CAbstractVstEditor::OnSetNextVSTPreset)
	ON_COMMAND(ID_VSTPRESETBACKWARDJUMP,&CAbstractVstEditor::OnVSTPresetBackwardJump)
	ON_COMMAND(ID_VSTPRESETFORWARDJUMP,	&CAbstractVstEditor::OnVSTPresetForwardJump)
	ON_COMMAND(ID_VSTPRESETNAME,		&CAbstractVstEditor::OnVSTPresetRename)
	ON_COMMAND(ID_PLUGINTOINSTRUMENT,	&CAbstractVstEditor::OnCreateInstrument)
	ON_COMMAND_RANGE(ID_PRESET_SET, ID_PRESET_SET + PRESETS_PER_GROUP, &CAbstractVstEditor::OnSetPreset)
	ON_MESSAGE(WM_MOD_MIDIMSG,		&CAbstractVstEditor::OnMidiMsg)
	ON_MESSAGE(WM_MOD_KEYCOMMAND,	&CAbstractVstEditor::OnCustomKeyMsg) //rewbs.customKeys
	ON_COMMAND_RANGE(ID_PLUGSELECT, ID_PLUGSELECT + MAX_MIXPLUGINS, &CAbstractVstEditor::OnToggleEditor) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_SELECTINST, ID_SELECTINST + MAX_INSTRUMENTS, &CAbstractVstEditor::OnSetInputInstrument) //rewbs.patPlugName
	ON_COMMAND_RANGE(ID_LEARN_MACRO_FROM_PLUGGUI, ID_LEARN_MACRO_FROM_PLUGGUI + NUM_MACROS, &CAbstractVstEditor::PrepareToLearnMacro)
END_MESSAGE_MAP()


CAbstractVstEditor::CAbstractVstEditor(IMixPlugin &plugin)
	: m_VstPlugin(plugin)
{
	m_Menu.LoadMenu(IDR_VSTMENU);
	m_nInstrument = GetBestInstrumentCandidate();
}


CAbstractVstEditor::~CAbstractVstEditor()
{
	m_VstPlugin.m_pEditor = nullptr;
}


void CAbstractVstEditor::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}


void CAbstractVstEditor::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	CDialog::OnNcLButtonDblClk(nHitTest, point);
	// Double click on title bar = reduce plugin window to non-client area
	if(nHitTest == HTCAPTION)
	{
		CRect rcWnd, rcClient;
		GetWindowRect(&rcWnd);
		if(!m_isMinimized)
		{
			// When minimizing, remove the client area
			GetClientRect(&rcClient);
			m_clientHeight = rcClient.Height();
		}
		m_isMinimized = !m_isMinimized;
		m_clientHeight = -m_clientHeight;
		int rcHeight = rcWnd.Height() + m_clientHeight;

		SetWindowPos(NULL, 0, 0,
			rcWnd.Width(), rcHeight,
			SWP_NOZORDER | SWP_NOMOVE);
	}
}


void CAbstractVstEditor::OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	if(nState != WA_INACTIVE) CMainFrame::GetMainFrame()->SetMidiRecordWnd(GetSafeHwnd());
}


LRESULT CAbstractVstEditor::OnMidiMsg(WPARAM midiData, LPARAM sender)
{
	CModDoc *modDoc = m_VstPlugin.GetModDoc();
	if(modDoc != nullptr && sender != reinterpret_cast<LPARAM>(&m_VstPlugin))
	{
		if(!CheckInstrument(m_nInstrument))
			m_nInstrument = GetBestInstrumentCandidate();
		modDoc->ProcessMIDI((uint32)midiData, m_nInstrument, &m_VstPlugin, kCtxVSTGUI);
		return 1;
	}
	return 0;
}


// Drop files from Windows
void CAbstractVstEditor::OnDropFiles(HDROP hDropInfo)
{
	const UINT nFiles = ::DragQueryFileW(hDropInfo, (UINT)-1, NULL, 0);
	CMainFrame::GetMainFrame()->SetForegroundWindow();
	for(UINT f = 0; f < nFiles; f++)
	{
		UINT size = ::DragQueryFile(hDropInfo, f, nullptr, 0) + 1;
		std::vector<TCHAR> fileName(size, _T('\0'));
		if(::DragQueryFile(hDropInfo, f, fileName.data(), size))
		{
			m_VstPlugin.LoadProgram(mpt::PathString::FromNative(fileName.data()));
		}
	}
	::DragFinish(hDropInfo);
}


void CAbstractVstEditor::OnLoadPreset()
{
	if(m_VstPlugin.LoadProgram())
	{
		UpdatePresetMenu(true);
		UpdatePresetField();
	}
}


void CAbstractVstEditor::OnSavePreset()
{
	m_VstPlugin.SaveProgram();
}


void CAbstractVstEditor::OnCopyParameters()
{
	if(CMainFrame::GetMainFrame() == nullptr) return;

	BeginWaitCursor();
	std::ostringstream f(std::ios::out | std::ios::binary);
	if(VSTPresets::SaveFile(f, m_VstPlugin, false))
	{
		const std::string data = f.str();
		Clipboard clipboard(m_clipboardFormat, data.length());
		if(auto dst = clipboard.As<char>())
		{
			memcpy(dst, data.data(), data.length());
		}
	}
	EndWaitCursor();
}


void CAbstractVstEditor::OnPasteParameters()
{
	if(CMainFrame::GetMainFrame() == nullptr) return;

	BeginWaitCursor();
	Clipboard clipboard(m_clipboardFormat);
	if(auto data = clipboard.Get(); data.data())
	{
		FileReader file(data);
		VSTPresets::ErrorCode error = VSTPresets::LoadFile(file, m_VstPlugin);
		clipboard.Close();

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
	EndWaitCursor();
}


void CAbstractVstEditor::OnRandomizePreset()
{
	static double randomFactor = 10.0;
	CInputDlg dlg(this, _T("Input parameter randomization amount (0 = no change, 100 = completely random)"), 0.0, 100.0, randomFactor);
	if(dlg.DoModal() == IDOK)
	{
		randomFactor = dlg.resultAsDouble;
		PlugParamValue factor = PlugParamValue(randomFactor / 100.0);
		PlugParamIndex numParams = m_VstPlugin.GetNumParameters();
		for(PlugParamIndex p = 0; p < numParams; p++)
		{
			PlugParamValue val = m_VstPlugin.GetParameter(p);
			val += mpt::random(theApp.PRNG(), PlugParamValue(-1.0), PlugParamValue(1.0)) * factor;
			Limit(val, 0.0f, 1.0f);
			m_VstPlugin.SetParameter(p, val);
		}
		UpdateParamDisplays();
	}
}


bool CAbstractVstEditor::OpenEditor(CWnd *)
{
	ModifyStyleEx(0, WS_EX_ACCEPTFILES);
	RestoreWindowPos();
	SetTitle();
	SetupMenu();
	ShowWindow(SW_SHOW);
	return true;
}


void CAbstractVstEditor::DoClose()
{
	StoreWindowPos();
	m_presetMenuGroup.clear();
	DestroyWindow();
}


void CAbstractVstEditor::SetupMenu(bool force)
{
	WindowSizeAdjuster adjuster(*this);
	SetMenu(&m_Menu);
	UpdatePresetMenu(force);
	UpdateInputMenu();
	UpdateOutputMenu();
	UpdateMacroMenu();
	UpdateOptionsMenu();
	UpdatePresetField();
}


void CAbstractVstEditor::UpdatePresetField()
{
	if(m_VstPlugin.GetNumPrograms() > 0)
	{
		if(m_Menu.GetMenuItemCount() < 5)
		{
			m_Menu.AppendMenu(MF_BYPOSITION, ID_VSTPRESETBACKWARDJUMP, _T("<<"));
			m_Menu.AppendMenu(MF_BYPOSITION, ID_PREVIOUSVSTPRESET, _T("<"));
			m_Menu.AppendMenu(MF_BYPOSITION, ID_NEXTVSTPRESET, _T(">"));
			m_Menu.AppendMenu(MF_BYPOSITION, ID_VSTPRESETFORWARDJUMP, _T(">>"));
			m_Menu.AppendMenu(MF_BYPOSITION|MF_DISABLED, ID_VSTPRESETNAME, _T(""));
		}

		CString programName = m_VstPlugin.GetFormattedProgramName(m_VstPlugin.GetCurrentProgram());
		programName.Replace(_T("&"), _T("&&"));
		m_Menu.ModifyMenu(8, MF_BYPOSITION, ID_VSTPRESETNAME, programName);
	}

	DrawMenuBar();

}


void CAbstractVstEditor::OnSetPreset(UINT nID)
{
	SetPreset(nID - ID_PRESET_SET + m_currentPresetMenu * PRESETS_PER_GROUP);
}


void CAbstractVstEditor::OnSetPreviousVSTPreset()
{
	SetPreset(m_VstPlugin.GetCurrentProgram() - 1);
}


void CAbstractVstEditor::OnSetNextVSTPreset()
{
	SetPreset(m_VstPlugin.GetCurrentProgram() + 1);
}


void CAbstractVstEditor::OnVSTPresetBackwardJump()
{
	SetPreset(std::max(0, m_VstPlugin.GetCurrentProgram() - 10));
}


void CAbstractVstEditor::OnVSTPresetForwardJump()
{
	SetPreset(std::min(m_VstPlugin.GetCurrentProgram() + 10, m_VstPlugin.GetNumPrograms() - 1));
}


void CAbstractVstEditor::SetPreset(int32 preset)
{
	if(preset >= 0 && preset < m_VstPlugin.GetNumPrograms())
	{
		m_VstPlugin.SetCurrentProgram(preset);
		WindowSizeAdjuster adjuster(*this);
		UpdatePresetField();

		if(m_VstPlugin.GetSoundFile().GetModSpecifications().supportsPlugins)
		{
			m_VstPlugin.GetModDoc()->SetModified();
		}
	}
}


void CAbstractVstEditor::OnVSTPresetRename()
{
	auto currentName = m_VstPlugin.GetCurrentProgramName();
	CInputDlg dlg(this, _T("New program name:"), currentName);
	if(dlg.DoModal() == IDOK)
	{
		m_VstPlugin.SetCurrentProgramName(dlg.resultAsString);
		if(m_VstPlugin.GetCurrentProgramName() != currentName)
		{
			m_VstPlugin.SetModified();
			WindowSizeAdjuster adjuster(*this);
			UpdatePresetField();
			UpdatePresetMenu(true);
		}
	}
}


void CAbstractVstEditor::OnBypassPlug()
{
	m_VstPlugin.ToggleBypass();
	if(m_VstPlugin.GetSoundFile().GetModSpecifications().supportsPlugins)
	{
		m_VstPlugin.GetModDoc()->SetModified();
	}
}


void CAbstractVstEditor::OnRecordAutomation()
{
	m_VstPlugin.m_recordAutomation = !m_VstPlugin.m_recordAutomation;
}


void CAbstractVstEditor::OnRecordMIDIOut()
{
	m_VstPlugin.m_recordMIDIOut = !m_VstPlugin.m_recordMIDIOut;
}


void CAbstractVstEditor::OnPassKeypressesToPlug()
{
	m_VstPlugin.m_passKeypressesToPlug  = !m_VstPlugin.m_passKeypressesToPlug;
}


BOOL CAbstractVstEditor::PreTranslateMessage(MSG *msg)
{
	if(msg && HandleKeyMessage(*msg))
		return TRUE;

	return CDialog::PreTranslateMessage(msg);
}


bool CAbstractVstEditor::HandleKeyMessage(MSG &msg)
{
	if(m_VstPlugin.m_passKeypressesToPlug)
		return false;
	if(msg.message != WM_SYSKEYUP && msg.message != WM_KEYUP && msg.message != WM_SYSKEYDOWN && msg.message != WM_KEYDOWN)
		return false;

	CInputHandler *ih = CMainFrame::GetInputHandler();
	if(ih->IsKeyPressHandledByTextBox(static_cast<DWORD>(msg.wParam), ::GetFocus()))
		return false;

	// Translate message manually
	UINT nChar = (UINT)msg.wParam;
	UINT nRepCnt = LOWORD(msg.lParam);
	UINT nFlags = HIWORD(msg.lParam);
	KeyEventType kT = ih->GetKeyEventType(nFlags);

	// If we successfully mapped to a command and plug does not listen for keypresses, no need to pass message on.
	if(ih->KeyEvent(kCtxVSTGUI, nChar, nRepCnt, nFlags, kT, this) != kcNull)
		return true;

	// Don't forward key repeats if plug does not listen for keypresses
	// (avoids system beeps on note hold)
	if(kT == kKeyEventRepeat)
		return true;
	
	return false;
}


void CAbstractVstEditor::SetTitle()
{
	if(m_VstPlugin.m_pMixStruct)
	{
		CString title = MPT_CFORMAT("FX {}: ")(mpt::cfmt::dec0<2>(m_VstPlugin.m_nSlot + 1));

		bool hasCustomName = (m_VstPlugin.m_pMixStruct->GetName() != U_("")) && (m_VstPlugin.m_pMixStruct->GetName() != m_VstPlugin.m_pMixStruct->GetLibraryName());
		if(hasCustomName)
			title += mpt::ToCString(m_VstPlugin.m_pMixStruct->GetName()) + _T(" (");
		title += mpt::ToCString(m_VstPlugin.m_pMixStruct->GetLibraryName());
		if(hasCustomName)
			title += _T(")");

#ifndef NO_VST
		const CVstPlugin *vstPlugin = dynamic_cast<CVstPlugin *>(&m_VstPlugin);
		if(vstPlugin != nullptr && vstPlugin->isBridged)
			title += MPT_CFORMAT(" ({} Bridged)")(m_VstPlugin.GetPluginFactory().GetDllArchNameUser());
#endif // NO_VST

		SetWindowText(title);
	}
}


LRESULT CAbstractVstEditor::OnCustomKeyMsg(WPARAM wParam, LPARAM /*lParam*/)
{
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
			const ModCommand::NOTE note = static_cast<ModCommand::NOTE>(wParam - kcVSTGUIStartNotes + NOTE_MIN + pMainFrm->GetBaseOctave() * 12);
			if(ModCommand::IsNote(note))
			{
				pModDoc->PlayNote(PlayNoteParam(note).Instrument(m_nInstrument), &m_noteChannel);
			}
		}
		return wParam;
	}
	if (wParam >= kcVSTGUIStartNoteStops && wParam <= kcVSTGUIEndNoteStops)
	{
		if(ValidateCurrentInstrument())
		{
			CModDoc* pModDoc = m_VstPlugin.GetModDoc();
			CMainFrame *pMainFrm = CMainFrame::GetMainFrame();
			const ModCommand::NOTE note = static_cast<ModCommand::NOTE>(wParam - kcVSTGUIStartNoteStops + NOTE_MIN + pMainFrm->GetBaseOctave() * 12);
			if(ModCommand::IsNote(note))
			{
				pModDoc->NoteOff(note, false, m_nInstrument, m_noteChannel[note - NOTE_MIN]);
			}
		}
		return wParam;
	}

	return kcNull;
}


// When trying to play a note using this plugin, but no instrument is assigned to it,
// the user is asked whether a new instrument should be added.
bool CAbstractVstEditor::ValidateCurrentInstrument()
{
	if(!CheckInstrument(m_nInstrument))
		m_nInstrument = GetBestInstrumentCandidate();

	//only show messagebox if plug is able to process notes.
	if(m_nInstrument == INSTRUMENTINDEX_INVALID)
	{
		if(m_VstPlugin.CanRecieveMidiEvents())
		{
			// We might need to steal the focus from the plugin bridge. This is going to work
			// as the plugin bridge will call AllowSetForegroundWindow on key messages.
			SetForegroundWindow();
			if(!m_VstPlugin.IsInstrument() || m_VstPlugin.GetSoundFile().GetModSpecifications().instrumentsMax == 0 ||
				Reporting::Confirm(_T("You need to assign an instrument to this plugin before you can play notes from here.\nCreate a new instrument and assign this plugin to the instrument?"), false, false, this) == cnfNo)
			{
				return false;
			} else
			{
				OnCreateInstrument();
				// Return true since we don't want to trigger the note for which the instrument has been validated yet.
				// Otherwise, the note might hang forever because the key-up event will go missing.
				return false;
			}
		} else
		{
			// Can't process notes
			return false;
		}
	}
	return true;

}


static int GetNumSubMenus(int32 numProgs) { return (numProgs + (PRESETS_PER_GROUP - 1)) / PRESETS_PER_GROUP; }


void CAbstractVstEditor::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	if(!(nFlags & MF_POPUP))
	{
		return;
	}
	if(hSysMenu == m_Menu.m_hMenu)
	{
		// Main menu
		switch(nItemID)
		{
		case 0:
			// Grey out paste menu item.
			m_Menu.EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND | (IsClipboardFormatAvailable(m_clipboardFormat) ? 0 : MF_GRAYED));
			break;
		case 1:
			// If there would be only one sub menu, we add presets directly to the factory menu
			{
				const int32 numProgs = m_VstPlugin.GetNumPrograms();
				if(GetNumSubMenus(numProgs) <= 1)
				{
					GeneratePresetMenu(0, m_PresetMenu);
				}
			}
			break;
		}
	} else if(hSysMenu == m_Menu.GetSubMenu(1)->m_hMenu)
	{
		// Preset menu
		m_currentPresetMenu = nItemID;
		GeneratePresetMenu(nItemID * PRESETS_PER_GROUP, *m_presetMenuGroup[nItemID]);
	}
}


void CAbstractVstEditor::UpdatePresetMenu(bool force)
{
	const int32 numProgs = m_VstPlugin.GetNumPrograms();
	const int32 curProg  = m_VstPlugin.GetCurrentProgram();

	if(m_PresetMenu.m_hMenu) // We rebuild the menu from scratch, so remove any exiting menus...
	{
		if(curProg == m_nCurProg && !force) // ... unless menu exists and is accurate, in which case we are done.
			return;

		m_presetMenuGroup.clear();

		// If there were no preset groups, delete the remaining content so that it can be refilled dynamically
		while(m_PresetMenu.GetMenuItemCount() > 0)
			m_PresetMenu.RemoveMenu(0, MF_BYPOSITION);
	} else
	{
		// Create Factory preset menu
		m_PresetMenu.CreatePopupMenu();
		m_Menu.InsertMenu(1, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_PresetMenu.m_hMenu), _T("&Presets"));
	}

	m_Menu.EnableMenuItem(1, MF_BYPOSITION | (numProgs ? 0 : MF_GRAYED));

	const int numSubMenus = GetNumSubMenus(numProgs);

	if(numSubMenus > 1)
	{
		// Depending on the plugin and its number of presets, filling the sub menus can take quite a while (e.g. Synth1),
		// so we fill the menus only on demand (when they  are opened), so that the editor GUI creation doesn't take forever.
		m_presetMenuGroup.resize(numSubMenus);
		for(int bank = 0, prog = 0; bank < numSubMenus; bank++, prog += PRESETS_PER_GROUP)
		{
			m_presetMenuGroup[bank] = std::make_unique<CMenu>();
			m_presetMenuGroup[bank]->CreatePopupMenu();

			CString label;
			label.Format(_T("Bank %d (%d-%d)"), bank + 1, prog + 1, std::min(prog + PRESETS_PER_GROUP, numProgs));
			m_PresetMenu.AppendMenu(MF_POPUP
				| (bank % 32 == 0 ? MF_MENUBREAK : 0)
				| (curProg >= prog && curProg < prog + PRESETS_PER_GROUP ? MF_CHECKED : MF_UNCHECKED),
				reinterpret_cast<UINT_PTR>(m_presetMenuGroup[bank]->m_hMenu), label);
		}
	}

	m_currentPresetMenu = 0;
	m_nCurProg = curProg;
}


void CAbstractVstEditor::GeneratePresetMenu(int32 offset, CMenu &parent)
{
	const int32 numProgs = m_VstPlugin.GetNumPrograms();
	const int32 curProg  = m_VstPlugin.GetCurrentProgram();
	const int32 endProg = std::min(offset + PRESETS_PER_GROUP, numProgs);

	if(parent.GetMenuItemCount() != 0)
	{
		// Already generated.
		return;
	}

	m_VstPlugin.CacheProgramNames(offset, endProg);
	for(int32 p = offset, row = 0, id = 0; p < endProg; p++, row++, id++)
	{
		CString programName = m_VstPlugin.GetFormattedProgramName(p);
		programName.Replace(_T("&"), _T("&&"));
		UINT splitMenuFlag = 0;

		if(row == PRESETS_PER_COLUMN)
		{
			// Advance to next menu column
			row = 0;
			splitMenuFlag = MF_MENUBARBREAK;
		}

		parent.AppendMenu(MF_STRING | (p == curProg ? MF_CHECKED : MF_UNCHECKED) | splitMenuFlag, ID_PRESET_SET + id, programName);
	}
}


void CAbstractVstEditor::UpdateInputMenu()
{
	CMenu *pInfoMenu = m_Menu.GetSubMenu(2);
	pInfoMenu->DeleteMenu(0, MF_BYPOSITION);

	const CSoundFile &sndFile = m_VstPlugin.GetSoundFile();

	if(m_InputMenu.m_hMenu)
	{
		m_InputMenu.DestroyMenu();
	}
	if(!m_InputMenu.m_hMenu)
	{
		m_InputMenu.CreatePopupMenu();
	}

	std::vector<IMixPlugin *> inputPlugs;
	m_VstPlugin.GetInputPlugList(inputPlugs);
	for(auto plug : inputPlugs)
	{
		CString name = MPT_CFORMAT("FX{}: {}")(mpt::cfmt::dec0<2>(plug->m_nSlot + 1), mpt::ToCString(plug->m_pMixStruct->GetName()));
		m_InputMenu.AppendMenu(MF_STRING, ID_PLUGSELECT + plug->m_nSlot, name);
	}

	std::vector<CHANNELINDEX> inputChannels;
	m_VstPlugin.GetInputChannelList(inputChannels);
	bool addSeparator = !inputPlugs.empty();
	for(auto chn : inputChannels)
	{
		if(addSeparator)
		{
			m_InputMenu.AppendMenu(MF_SEPARATOR);
			addSeparator = false;
		}
		CString name = MPT_CFORMAT("Chn{}: {}")(mpt::cfmt::dec0<2>(chn + 1), mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.ChnSettings[chn].szName));
		m_InputMenu.AppendMenu(MF_STRING, NULL, name);
	}

	std::vector<INSTRUMENTINDEX> inputInstruments;
	m_VstPlugin.GetInputInstrumentList(inputInstruments);
	addSeparator = !inputPlugs.empty() || !inputChannels.empty();
	for(auto ins : inputInstruments)
	{
		if(addSeparator)
		{
			m_InputMenu.AppendMenu(MF_SEPARATOR);
			addSeparator = false;
		}
		CString name = MPT_CFORMAT("Ins{}: {}")(mpt::cfmt::dec0<2>(ins), mpt::ToCString(sndFile.GetCharsetInternal(), sndFile.GetInstrumentName(ins)));
		m_InputMenu.AppendMenu(MF_STRING | ((ins == m_nInstrument) ? MF_CHECKED : 0), ID_SELECTINST + ins, name);
	}

	if(inputPlugs.empty() && inputChannels.empty() && inputInstruments.empty())
	{
		m_InputMenu.AppendMenu(MF_STRING | MF_GRAYED, NULL, _T("None"));
	}

	pInfoMenu->InsertMenu(0, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_InputMenu.m_hMenu), _T("I&nputs"));
}


void CAbstractVstEditor::UpdateOutputMenu()
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

	std::vector<IMixPlugin *> outputPlugs;
	m_VstPlugin.GetOutputPlugList(outputPlugs);
	CString name;

	for(auto plug : outputPlugs)
	{
		if(plug != nullptr)
		{
			name.Format(_T("FX%02d: "), plug->m_nSlot + 1);
			name += mpt::ToCString(plug->m_pMixStruct->GetName());
			m_OutputMenu.AppendMenu(MF_STRING, ID_PLUGSELECT + plug->m_nSlot, name);
		} else
		{
			name = _T("Master Output");
			m_OutputMenu.AppendMenu(MF_STRING | MF_GRAYED, NULL, name);
		}
	}
	pInfoMenu->InsertMenu(1, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_OutputMenu.m_hMenu), _T("Ou&tputs"));
}


void CAbstractVstEditor::UpdateMacroMenu()
{
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

	CString label, macroName;
	for(int nMacro = 0; nMacro < NUM_MACROS; nMacro++)
	{
		int action = 0;
		UINT greyed = MF_GRAYED;

		const MIDIMacroConfig &midiCfg = m_VstPlugin.GetSoundFile().m_MidiCfg;

		const ParameteredMacro macroType = midiCfg.GetParameteredMacroType(nMacro);

		if(macroType == kSFxUnused)
		{
			macroName = _T("Unused. Learn Param...");
			action= ID_LEARN_MACRO_FROM_PLUGGUI + nMacro;
			greyed = 0;
		} else
		{
			macroName = midiCfg.GetParameteredMacroName(nMacro, &m_VstPlugin);
			if(macroType != kSFxPlugParam || macroName.Left(3) != _T("N/A"))
			{
				greyed = 0;
			}
		}

		label.Format(_T("SF%X: "), nMacro);
		label += macroName;
		m_MacroMenu.AppendMenu(MF_STRING | greyed, action, label);
	}

	pInfoMenu->InsertMenu(2, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_MacroMenu.m_hMenu), _T("&Macros"));
}

void CAbstractVstEditor::UpdateOptionsMenu()
{

	if(m_OptionsMenu.m_hMenu)
		m_OptionsMenu.DestroyMenu();

	CInputHandler *ih = CMainFrame::GetInputHandler();

	m_OptionsMenu.CreatePopupMenu();

	//Bypass
	m_OptionsMenu.AppendMenu(MF_STRING | (m_VstPlugin.IsBypassed() ? MF_CHECKED : 0),
							   ID_PLUG_BYPASS, ih->GetKeyTextFromCommand(kcVSTGUIBypassPlug, _T("&Bypass Plugin")));
	//Record Params
	m_OptionsMenu.AppendMenu(MF_STRING | (m_VstPlugin.m_recordAutomation ? MF_CHECKED : 0),
							   ID_PLUG_RECORDAUTOMATION, ih->GetKeyTextFromCommand(kcVSTGUIToggleRecordParams, _T("Record &Parameter Changes")));
	//Record MIDI Out
	m_OptionsMenu.AppendMenu(MF_STRING | (m_VstPlugin.m_recordMIDIOut ? MF_CHECKED : 0),
							   ID_PLUG_RECORD_MIDIOUT, ih->GetKeyTextFromCommand(kcVSTGUIToggleRecordMIDIOut, _T("Record &MIDI Out to Pattern Editor")));
	//Pass on keypresses
	m_OptionsMenu.AppendMenu(MF_STRING | (m_VstPlugin.m_passKeypressesToPlug ? MF_CHECKED : 0),
							   ID_PLUG_PASSKEYS, ih->GetKeyTextFromCommand(kcVSTGUIToggleSendKeysToPlug, _T("Pass &Keys to Plugin")));


	m_Menu.DeleteMenu(3, MF_BYPOSITION);
	m_Menu.InsertMenu(3, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(m_OptionsMenu.m_hMenu), _T("&Options"));

}


void CAbstractVstEditor::OnToggleEditor(UINT nID)
{
	CModDoc *pModDoc = m_VstPlugin.GetModDoc();

	if(pModDoc)
	{
		pModDoc->TogglePluginEditor(nID - ID_PLUGSELECT);
	}
}


void CAbstractVstEditor::OnInitMenu(CMenu* /*pMenu*/)
{
	SetupMenu();
}


bool CAbstractVstEditor::CheckInstrument(INSTRUMENTINDEX ins) const
{
	const CSoundFile &sndFile = m_VstPlugin.GetSoundFile();

	if(ins != INSTRUMENTINDEX_INVALID && ins < MAX_INSTRUMENTS && sndFile.Instruments[ins] != nullptr)
	{
		return (sndFile.Instruments[ins]->nMixPlug) == (m_VstPlugin.m_nSlot + 1);
	}
	return false;
}


INSTRUMENTINDEX CAbstractVstEditor::GetBestInstrumentCandidate() const
{
	// First try current instrument:
	const CModDoc *modDoc = m_VstPlugin.GetModDoc();
	POSITION pos = modDoc->GetFirstViewPosition();
	while(pos != NULL)
	{
		CModControlView *pView = dynamic_cast<CModControlView *>(modDoc->GetNextView(pos));
		if(pView != nullptr && pView->GetDocument() == modDoc)
		{
			INSTRUMENTINDEX ins = static_cast<INSTRUMENTINDEX>(pView->GetInstrumentChange());
			if(CheckInstrument(ins))
				return ins;
		}
	}

	// Just take the first instrument that points to this plug..
	return modDoc->HasInstrumentForPlugin(m_VstPlugin.m_nSlot);
}


void CAbstractVstEditor::OnSetInputInstrument(UINT nID)
{
	m_nInstrument = static_cast<INSTRUMENTINDEX>(nID - ID_SELECTINST);
}


void CAbstractVstEditor::OnCreateInstrument()
{
	if(m_VstPlugin.GetModDoc() != nullptr)
	{
		INSTRUMENTINDEX instr = m_VstPlugin.GetModDoc()->InsertInstrumentForPlugin(m_VstPlugin.GetSlot());
		if(instr != INSTRUMENTINDEX_INVALID) m_nInstrument  = instr;
	}
}


void CAbstractVstEditor::PrepareToLearnMacro(UINT nID)
{
	m_nLearnMacro = (nID-ID_LEARN_MACRO_FROM_PLUGGUI);
	//Now we wait for a param to be touched. We'll get the message from the VST Plug Manager.
	//Then pModDoc->LearnMacro(macro, param) is called
}

void CAbstractVstEditor::SetLearnMacro(int inMacro)
{
	if (inMacro < NUM_MACROS)
	{
		m_nLearnMacro=inMacro;
	}
}

int CAbstractVstEditor::GetLearnMacro()
{
	return m_nLearnMacro;
}


void CAbstractVstEditor::OnMove(int, int)
{
	if(IsWindowVisible())
	{
		StoreWindowPos();
	}
}


void CAbstractVstEditor::StoreWindowPos()
{
	if(m_hWnd)
	{
		WINDOWPLACEMENT wnd;
		wnd.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(&wnd);
		m_VstPlugin.SetEditorPos(wnd.rcNormalPosition.left, wnd.rcNormalPosition.top);
	}
}


void CAbstractVstEditor::RestoreWindowPos()
{
	// Restore previous editor position
	int32 editorX, editorY;
	m_VstPlugin.GetEditorPos(editorX, editorY);

	if(editorX != int32_min && editorY != int32_min)
	{
		WINDOWPLACEMENT wnd;
		wnd.length = sizeof(wnd);
		GetWindowPlacement(&wnd);
		wnd.showCmd = SW_SHOWNOACTIVATE;
		CRect rect = wnd.rcNormalPosition;
		rect.MoveToXY(editorX, editorY);
		wnd.rcNormalPosition = rect;
		SetWindowPlacement(&wnd);
	}
}


#endif // NO_PLUGINS


OPENMPT_NAMESPACE_END
