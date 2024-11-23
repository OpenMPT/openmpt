/*
 * InputHandler.cpp
 * ----------------
 * Purpose: Implementation of keyboard input handling, keymap loading, ...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "InputHandler.h"
#include "CommandSet.h"
#include "Mainfrm.h"
#include "Mptrack.h"
#include "resource.h"
#include "TrackerSettings.h"
#include "WindowMessages.h"
#include "../soundlib/MIDIEvents.h"
#include "mpt/fs/fs.hpp"


OPENMPT_NAMESPACE_BEGIN


#define TRANSITIONBIT 0x8000
#define REPEATBIT 0x4000

CInputHandler::CInputHandler(CWnd *mainframe)
{
	m_pMainFrm = mainframe;

	//Init CommandSet and Load defaults
	m_activeCommandSet = std::make_unique<CCommandSet>();
	m_lastCommands.fill(kcNull);

	mpt::PathString defaultPath = theApp.GetConfigPath() + P_("Keybindings.mkb");

	const bool noExistingKbdFileSetting = TrackerSettings::Instance().m_szKbdFile.empty();

	// 1. Try to load keybindings from the path saved in the settings.
	// 2. If the setting doesn't exist or the loading fails, try to load from default location.
	// 3. If neither one of these worked, load default keybindings from resources.
	// 4. If there were no keybinding setting already, create a keybinding file to default location
	//    and set its path to settings.

	if(noExistingKbdFileSetting || !m_activeCommandSet->LoadFile(TrackerSettings::Instance().m_szKbdFile))
	{
		if(!mpt::native_fs{}.is_file(defaultPath) || !m_activeCommandSet->LoadFile(defaultPath))
		{
			// Load keybindings from resources.
			MPT_LOG_GLOBAL(LogDebug, "InputHandler", U_("Loading keybindings from resources\n"));
			m_activeCommandSet->LoadDefaultKeymap();
			if (noExistingKbdFileSetting)
				m_activeCommandSet->SaveFile(defaultPath);
		}
	}
	// We will only overwrite the default Keybindings.mkb file from now on.
	TrackerSettings::Instance().m_szKbdFile = defaultPath;

	//Get Keymap
	m_activeCommandSet->GenKeyMap(m_keyMap);
	SetupSpecialKeyInterception(); // Feature: use Windows keys as modifier keys, intercept special keys
}


CommandID CInputHandler::SendCommands(CWnd *wnd, const KeyMapRange &cmd)
{
	CommandID executeCommand = kcNull;
	if(wnd != nullptr)
	{
		// Some commands (e.g. open/close/document switching) may invalidate the key map and thus its iterators.
		// To avoid this problem, copy over the elements we are interested in before sending commands.
		std::vector<KeyMap::value_type> commands;
		commands.reserve(std::distance(cmd.first, cmd.second));
		for(auto i = cmd.first; i != cmd.second; i++)
		{
			commands.push_back(*i);
		}
		for(const auto &i : commands)
		{
			m_lastCommands[m_lastCommandPos] = i.second;
			m_lastCommandPos = (m_lastCommandPos + 1) % m_lastCommands.size();
			if(wnd->SendMessage(WM_MOD_KEYCOMMAND, i.second, i.first.AsLPARAM()) != kcNull)
			{
				// Command was handled, no need to let the OS handle the key
				executeCommand = i.second;
			}
		}
	}
	return executeCommand;
}


CommandID CInputHandler::KeyEvent(const InputTargetContext context, const KeyboardEvent &event, CWnd *pSourceWnd)
{
	if(IsKeyPressHandledByTextBox(event.key, ::GetFocus()))
		return kcNull;

	KeyMapRange cmd = m_keyMap.equal_range(KeyCombination(context, GetModifierMask(), event.key, event.keyEventType));
	if(cmd.first != cmd.second && InterceptSpecialKeys(event))
		return kcDummyShortcut;

	if(pSourceWnd == nullptr)
		pSourceWnd = m_pMainFrm;	// By default, send command message to main frame.
	return SendCommands(pSourceWnd, cmd);
}


// Feature: use Windows keys as modifier keys, intercept special keys
bool CInputHandler::InterceptSpecialKeys(const KeyboardEvent &event)
{
	enum { VK_NonExistentKey = VK_F24+1 };

	if(event.key == VK_NonExistentKey)
	{
		return true;
	} else if(m_bInterceptWindowsKeys && (event.key == VK_LWIN || event.key == VK_RWIN))
	{
		if(event.keyEventType == kKeyEventDown)
		{
			INPUT inp[2];
			inp[0].type = inp[1].type = INPUT_KEYBOARD;
			inp[0].ki.time = inp[1].ki.time = 0;
			inp[0].ki.dwExtraInfo = inp[1].ki.dwExtraInfo = 0;
			inp[0].ki.wVk = inp[1].ki.wVk = VK_NonExistentKey;
			inp[0].ki.wScan = inp[1].ki.wScan = 0;
			inp[0].ki.dwFlags = 0;
			inp[1].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(2, inp, sizeof(INPUT));
		}
	}

	if((event.key == VK_NUMLOCK && m_bInterceptNumLock)
		|| (event.key == VK_CAPITAL && m_bInterceptCapsLock)
		|| (event.key == VK_SCROLL && m_bInterceptScrollLock))
	{
		if(GetMessageExtraInfo() == 0xC0FFEE)
		{
			SetMessageExtraInfo(0);
			return true;
		} else if(event.keyEventType == kKeyEventDown)
		{
			// Prevent keys from lighting up by simulating a second press.
			INPUT inp[2];
			inp[0].type = inp[1].type = INPUT_KEYBOARD;
			inp[0].ki.time = inp[1].ki.time = 0;
			inp[0].ki.dwExtraInfo = inp[1].ki.dwExtraInfo = 0xC0FFEE;
			inp[0].ki.wVk = inp[1].ki.wVk = static_cast<WORD>(event.key);
			inp[0].ki.wScan = inp[1].ki.wScan = 0;
			inp[0].ki.dwFlags = KEYEVENTF_KEYUP;
			inp[1].ki.dwFlags = 0;
			SendInput(2, inp, sizeof(INPUT));
		}
	}
	return false;
};


void CInputHandler::SetupSpecialKeyInterception()
{
	m_bInterceptWindowsKeys = m_bInterceptNumLock = m_bInterceptCapsLock = m_bInterceptScrollLock = false;
	for(const auto &i : m_keyMap)
	{
		ASSERT(i.second != kcNull);
		if(i.first.Modifier() == ModWin)
			m_bInterceptWindowsKeys = true;
		if(i.first.KeyCode() == VK_NUMLOCK)
			m_bInterceptNumLock = true;
		if(i.first.KeyCode() == VK_CAPITAL)
			m_bInterceptCapsLock = true;
		if(i.first.KeyCode() == VK_SCROLL)
			m_bInterceptScrollLock = true;
	}
};


// Translate MIDI messages to shortcut commands
CommandID CInputHandler::HandleMIDIMessage(InputTargetContext context, uint32 message)
{
	KeyMapRange cmd = { m_keyMap.end(), m_keyMap.end() };
	auto byte1 = MIDIEvents::GetDataByte1FromEvent(message), byte2 = MIDIEvents::GetDataByte2FromEvent(message);
	switch(MIDIEvents::GetTypeFromEvent(message))
	{
	case MIDIEvents::evControllerChange:
		if(byte2 != 0)
		{
			// Only capture MIDI CCs for now. Some controllers constantly send some MIDI CCs with value 0
			// (e.g. the Roland D-50 sends CC123 whenenver all notes have been released), so we will ignore those.
			cmd = m_keyMap.equal_range(KeyCombination(context, ModMidi, byte1, kKeyEventDown));
			return SendCommands(m_pMainFrm, cmd);
		}
		break;

	case MIDIEvents::evNoteOff:
		byte2 = 0;
		[[fallthrough]];
	case MIDIEvents::evNoteOn:
		if(byte2 != 0)
		{
			cmd = m_keyMap.equal_range(KeyCombination(context, ModMidi, byte1 | 0x80, kKeyEventDown));
			return SendCommands(m_pMainFrm, cmd);
		} else
		{
			cmd = m_keyMap.equal_range(KeyCombination(context, ModMidi, byte1 | 0x80, kKeyEventUp));
			// If the key-down triggered a note, we still want that note to be stopped. So we always pretend that no key was assigned to this event
			SendCommands(m_pMainFrm, cmd);
		}
		break;

	default:
		break;
	}

	return kcNull;
}


int CInputHandler::GetKeyListSize(CommandID cmd) const
{
	return m_activeCommandSet->GetKeyListSize(cmd);
}


CInputHandler::KeyboardEvent CInputHandler::Translate(const MSG &msg)
{
	MPT_ASSERT(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP);

	uint32 key = static_cast<uint32>(msg.wParam);
	if(key == VK_PACKET)
	{
		// This message was sent by something other than a physical keyboard.
		// This behaviour can be observed when using Microsoft's RDP client for iOS.

		// VK_PACKET is not the real virtual key code, and the message does not contain a real scancode.
		// To get at the virtual key code, we must first force the WM_CHAR message to get posted to the queue.
		::TranslateMessage(&msg);

		// Now remove that WM_CHAR message to get the virtual key code.  (The WM_CHAR message will get
		// re-posted by the main message pump after this call stack unwinds to it.)
		MSG msgChar{};
		if(::PeekMessage(&msgChar, msgChar.hwnd, WM_CHAR, WM_CHAR, PM_REMOVE | PM_QS_POSTMESSAGE))
		{
			key = VkKeyScanW(static_cast<WCHAR>(msgChar.wParam & 0xffffu));
		}
	}

	const auto repCnt = LOWORD(msg.lParam);
	const auto flags = HIWORD(msg.lParam);
	return {key, repCnt, flags, GetKeyEventType(flags)};
}


KeyEventType CInputHandler::GetKeyEventType(const MSG &msg)
{
	return GetKeyEventType(HIWORD(msg.lParam));
}


KeyEventType CInputHandler::GetKeyEventType(UINT nFlags)
{
	if (nFlags & TRANSITIONBIT)
	{
		// Key released
		return kKeyEventUp;
	} else if (nFlags & REPEATBIT)
	{
		// Key repeated
		return kKeyEventRepeat;
	} else
	{
		// New key down
		return kKeyEventDown;
	}
}


bool CInputHandler::SelectionPressed() const
{
	int nSelectionKeys = m_activeCommandSet->GetKeyListSize(kcSelect);
	KeyCombination key;

	const auto modifierMask = GetModifierMask();
	for (int k=0; k<nSelectionKeys; k++)
	{
		key = m_activeCommandSet->GetKey(kcSelect, k);
		if(modifierMask & key.Modifier())
			return true;
	}
	return false;
}


bool CInputHandler::ShiftPressed()
{
	return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
}


bool CInputHandler::CtrlPressed()
{
	return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
}


bool CInputHandler::AltPressed()
{
	return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
}


OrderTransitionMode CInputHandler::ModifierKeysToTransitionMode()
{
	const bool shift = ShiftPressed();
	const bool alt = AltPressed();
	if(shift && alt)
		return OrderTransitionMode::AtRowEnd;
	else if(!shift && alt)
		return OrderTransitionMode::AtBeatEnd;
	else if(shift && !alt)
		return OrderTransitionMode::AtMeasureEnd;
	else
		return OrderTransitionMode::AtPatternEnd;
}


void CInputHandler::Bypass(bool b)
{
	if(b)
		m_bypassCount++;
	else
		m_bypassCount--;
	MPT_ASSERT(m_bypassCount >= 0);
}


bool CInputHandler::IsBypassed() const
{
	return m_bypassCount > 0;
}


FlagSet<Modifiers> CInputHandler::GetModifierMask()
{
	BYTE keyStates[256];
	FlagSet<Modifiers> modifierMask = ModNone;
	if(GetActiveWindow() == nullptr)
		return modifierMask;
	if(!GetKeyboardState(keyStates))
		return modifierMask;

	const bool distinguishModifiers = TrackerSettings::Instance().MiscDistinguishModifiers;
	if(keyStates[VK_LCONTROL] & 0x80)
		modifierMask.set(ModCtrl);
	if(keyStates[VK_RCONTROL] & 0x80)
		modifierMask.set(distinguishModifiers ? ModRCtrl : ModCtrl);
	if(keyStates[VK_LSHIFT] & 0x80)
		modifierMask.set(ModShift);
	if(keyStates[VK_RSHIFT] & 0x80)
		modifierMask.set(distinguishModifiers ? ModRShift : ModShift);
	if(keyStates[VK_LMENU] & 0x80)
		modifierMask.set(ModAlt);
	if(keyStates[VK_RMENU] & 0x80)
		modifierMask.set(distinguishModifiers ? ModRAlt : ModAlt);
	if(keyStates[VK_LWIN] & 0x80)
		modifierMask.set(ModWin);
	if(keyStates[VK_RWIN] & 0x80)
		modifierMask.set(ModWin);
	return modifierMask;
}


CString CInputHandler::GetKeyTextFromCommand(CommandID c, const TCHAR *prependText) const
{
	CString s;
	if(prependText != nullptr)
	{
		s = prependText;
		s.AppendChar(_T('\t'));
	}
	s += m_activeCommandSet->GetKeyTextFromCommand(c, 0);
	return s;
}


CString CInputHandler::GetMenuText(UINT id) const
{
	static constexpr std::tuple<UINT, CommandID, const TCHAR *> MenuItems[] =
	{
		{ ID_FILE_NEW,                      kcFileNew,           _T("&New") },
		{ ID_FILE_OPEN,                     kcFileOpen,          _T("&Open...") },
		{ ID_FILE_OPENTEMPLATE,             kcNull,              _T("Open &Template") },
		{ ID_FILE_OPENTEMPLATE_LASTINRANGE, kcFileOpenTemplate,  _T("&Browse...") },
		{ ID_FILE_CLOSE,                    kcFileClose,         _T("&Close") },
		{ ID_FILE_CLOSEALL,                 kcFileCloseAll,      _T("C&lose All") },
		{ ID_FILE_APPENDMODULE,             kcFileAppend,        _T("Appen&d Module...") },
		{ ID_FILE_SAVE,                     kcFileSave,          _T("&Save") },
		{ ID_FILE_SAVE_AS,                  kcFileSaveAs,        _T("Save &As...") },
		{ ID_FILE_SAVE_COPY,                kcFileSaveCopy,      _T("Save Cop&y...") },
		{ ID_FILE_SAVEASTEMPLATE,           kcFileSaveTemplate,  _T("Sa&ve as Template") },
		{ ID_FILE_SAVEASWAVE,               kcFileSaveAsWave,    _T("Stream Export (&WAV, FLAC, MP3, etc.)...") },
		{ ID_FILE_SAVEMIDI,                 kcFileSaveMidi,      _T("Export as M&IDI...") },
		{ ID_FILE_SAVEOPL,                  kcFileSaveOPL,       _T("Export O&PL Register Dump...") },
		{ ID_FILE_SAVECOMPAT,               kcFileExportCompat,  _T("Compatibility &Export...") },
		{ ID_IMPORT_MIDILIB,                kcFileImportMidiLib, _T("Import &MIDI Library...") },
		{ ID_ADD_SOUNDBANK,                 kcFileAddSoundBank,  _T("Add Sound &Bank...") },

		{ ID_PLAYER_PLAY,          kcPlayPauseSong,      _T("Pause / &Resume") },
		{ ID_PLAYER_PLAYFROMSTART, kcPlaySongFromStart,  _T("&Play from Start") },
		{ ID_PLAYER_STOP,          kcStopSong,           _T("&Stop") },
		{ ID_PLAYER_PAUSE,         kcPauseSong,          _T("P&ause") },
		{ ID_MIDI_RECORD,          kcMidiRecord,         _T("&MIDI Record") },
		{ ID_ESTIMATESONGLENGTH,   kcEstimateSongLength, _T("&Estimate Song Length") },
		{ ID_APPROX_BPM,           kcApproxRealBPM,      _T("Approximate Real &BPM") },

		{ ID_EDIT_UNDO,                  kcEditUndo,      _T("&Undo") },
		{ ID_EDIT_REDO,                  kcEditRedo,      _T("&Redo") },
		{ ID_EDIT_CUT,                   kcEditCut,       _T("Cu&t") },
		{ ID_EDIT_COPY,                  kcEditCopy,      _T("&Copy") },
		{ ID_EDIT_PASTE,                 kcEditPaste,     _T("&Paste") },
		{ ID_EDIT_SELECT_ALL,            kcEditSelectAll, _T("Select &All") },
		{ ID_EDIT_CLEANUP,               kcNull,          _T("C&leanup") },
		{ ID_EDIT_FIND,                  kcEditFind,      _T("&Find / Replace") },
		{ ID_EDIT_FINDNEXT,              kcEditFindNext,  _T("Find &Next") },
		{ ID_EDIT_GOTO_MENU,             kcPatternGoto,   _T("&Goto") },
		{ ID_EDIT_SPLITKEYBOARDSETTINGS, kcShowSplitKeyboardSettings, _T("Split &Keyboard Settings") },
		// "Paste Special" sub menu
		{ ID_EDIT_PASTE_SPECIAL,    kcEditMixPaste,         _T("&Mix Paste") },
		{ ID_EDIT_MIXPASTE_ITSTYLE, kcEditMixPasteITStyle,  _T("M&ix Paste (IT Style)") },
		{ ID_EDIT_PASTEFLOOD,       kcEditPasteFlood,       _T("Paste Fl&ood") },
		{ ID_EDIT_PUSHFORWARDPASTE, kcEditPushForwardPaste, _T("&Push Forward Paste (Insert)") },

		{ ID_VIEW_GLOBALS,        kcViewGeneral,            _T("&General") },
		{ ID_VIEW_SAMPLES,        kcViewSamples,            _T("&Samples") },
		{ ID_VIEW_PATTERNS,       kcViewPattern,            _T("&Patterns") },
		{ ID_VIEW_INSTRUMENTS,    kcViewInstruments,        _T("&Instruments") },
		{ ID_VIEW_COMMENTS,       kcViewComments,           _T("&Comments") },
		{ ID_VIEW_OPTIONS,        kcViewOptions,            _T("S&etup") },
		{ ID_VIEW_TOOLBAR,        kcViewMain,               _T("Show &Main Toolbar") },
		{ IDD_TREEVIEW,           kcViewTree,               _T("Show &Tree View") },
		{ ID_PLUGIN_SETUP,        kcViewAddPlugin,          _T("Pl&ugin Manager") },
		{ ID_CHANNEL_MANAGER,     kcViewChannelManager,     _T("Ch&annel Manager") },
		{ ID_CLIPBOARD_MANAGER,   kcToggleClipboardManager, _T("C&lipboard Manager") },
		{ ID_VIEW_SONGPROPERTIES, kcViewSongProperties,     _T("Song P&roperties") },
		{ ID_PATTERN_MIDIMACRO,   kcShowMacroConfig,        _T("&Zxx Macro Configuration") },
		{ ID_VIEW_MIDIMAPPING,    kcViewMIDImapping,        _T("&MIDI Mapping") },
		{ ID_VIEW_EDITHISTORY,    kcViewEditHistory,        _T("Edit &History") },
		// Help submenu
		{ ID_HELPSHOW,        kcHelp, _T("&Help") },
		{ ID_EXAMPLE_MODULES, kcNull, _T("&Example Modules") },
	};

	for(const auto & [cmdID, command, text] : MenuItems)
	{
		if(id == cmdID)
		{
			if(command != kcNull)
				return GetKeyTextFromCommand(command, text);
			else
				return text;
		}
	}
	MPT_ASSERT_NOTREACHED();
	return _T("Unknown Item");
}


void CInputHandler::UpdateMainMenu()
{
	CMenu *pMenu = m_pMainFrm->GetMenu();
	if (!pMenu) return;

	pMenu->GetSubMenu(0)->ModifyMenu(0, MF_BYPOSITION | MF_STRING, 0, GetMenuText(ID_FILE_NEW));
	static constexpr int MenuItems[] =
	{
		ID_FILE_OPEN,
		ID_FILE_OPENTEMPLATE_LASTINRANGE,
		ID_FILE_APPENDMODULE,
		ID_FILE_CLOSE,
		ID_FILE_CLOSEALL,
		ID_FILE_SAVE,
		ID_FILE_SAVE_AS,
		ID_FILE_SAVEASWAVE,
		ID_FILE_SAVEMIDI,
		ID_FILE_SAVECOMPAT,
		ID_IMPORT_MIDILIB,
		ID_ADD_SOUNDBANK,

		ID_PLAYER_PLAY,
		ID_PLAYER_PLAYFROMSTART,
		ID_PLAYER_STOP,
		ID_PLAYER_PAUSE,
		ID_MIDI_RECORD,
		ID_ESTIMATESONGLENGTH,
		ID_APPROX_BPM,

		ID_EDIT_UNDO,
		ID_EDIT_REDO,
		ID_EDIT_CUT,
		ID_EDIT_COPY,
		ID_EDIT_PASTE,
		ID_EDIT_PASTE_SPECIAL,
		ID_EDIT_MIXPASTE_ITSTYLE,
		ID_EDIT_PASTEFLOOD,
		ID_EDIT_PUSHFORWARDPASTE,
		ID_EDIT_SELECT_ALL,
		ID_EDIT_FIND,
		ID_EDIT_FINDNEXT,
		ID_EDIT_GOTO_MENU,
		ID_EDIT_SPLITKEYBOARDSETTINGS,

		ID_VIEW_GLOBALS,
		ID_VIEW_SAMPLES,
		ID_VIEW_PATTERNS,
		ID_VIEW_INSTRUMENTS,
		ID_VIEW_COMMENTS,
		ID_VIEW_TOOLBAR,
		IDD_TREEVIEW,
		ID_VIEW_OPTIONS,
		ID_PLUGIN_SETUP,
		ID_CHANNEL_MANAGER,
		ID_CLIPBOARD_MANAGER,
		ID_VIEW_SONGPROPERTIES,
		ID_VIEW_SONGPROPERTIES,
		ID_PATTERN_MIDIMACRO,
		ID_VIEW_EDITHISTORY,
		ID_HELPSHOW,
	};
	for(const auto id : MenuItems)
	{
		pMenu->ModifyMenu(id, MF_BYCOMMAND | MF_STRING, id, GetMenuText(id));
	}
}


void CInputHandler::SetNewCommandSet(const CCommandSet &newSet)
{
	m_activeCommandSet->Copy(newSet);
	m_activeCommandSet->GenKeyMap(m_keyMap);
	SetupSpecialKeyInterception(); // Feature: use Windows keys as modifier keys, intercept special keys
	UpdateMainMenu();
}


bool CInputHandler::SetEffectLetters(const CModSpecifications &modSpecs)
{
	MPT_LOG_GLOBAL(LogDebug, "InputHandler", U_("Changing command set."));
	bool retval = m_activeCommandSet->QuickChange_SetEffects(modSpecs);
	if(retval) m_activeCommandSet->GenKeyMap(m_keyMap);
	return retval;
}


bool CInputHandler::IsKeyPressHandledByTextBox(DWORD key, HWND hWnd) const
{
	if(hWnd == nullptr)
		return false;
	
	TCHAR activeWindowClassName[6];
	GetClassName(hWnd, activeWindowClassName, mpt::saturate_cast<int>(std::size(activeWindowClassName)));
	const bool textboxHasFocus = _tcsicmp(activeWindowClassName, _T("Edit")) == 0;
	if(!textboxHasFocus)
		return false;

	// Alpha-numerics (only shift or no modifier):
	const auto modifierMask = GetModifierMask();
	if(!modifierMask.test_any_except(ModShift))
	{
		if((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')
		   || (key >= VK_MULTIPLY && key <= VK_DIVIDE) || key == VK_SPACE || key == VK_CAPITAL
		   || (key >= VK_OEM_1 && key <= VK_OEM_3) || (key >= VK_OEM_4 && key <= VK_OEM_8))
			return true;
		if((key >= VK_NUMPAD0 && key <= VK_NUMPAD9) && modifierMask == ModNone)
			return true;
		if(key == VK_RETURN && (GetWindowLong(hWnd, GWL_STYLE) & ES_MULTILINE))
			return true;
	}

	// Navigation (any modifier except Alt without any other modifiers):
	if(modifierMask != ModAlt)
	{
		if(key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN
		   || key == VK_HOME || key == VK_END || key == VK_DELETE || key == VK_INSERT || key == VK_BACK)
			return true;
	}

	// Copy paste etc..
	if(modifierMask == ModCtrl)
	{
		if(key == 'Y' || key == 'Z' || key == 'X' ||  key == 'C' || key == 'V' || key == 'A')
			return true;
	}

	return false;
}


BypassInputHandler::BypassInputHandler()
{
	if(CMainFrame::GetInputHandler())
	{
		bypassed = true;
		CMainFrame::GetInputHandler()->Bypass(true);
	}
}


BypassInputHandler::~BypassInputHandler()
{
	if(bypassed)
	{
		CMainFrame::GetInputHandler()->Bypass(false);
		bypassed = false;
	}
}

OPENMPT_NAMESPACE_END
