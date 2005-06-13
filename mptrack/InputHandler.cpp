#include "stdafx.h"
#include "afxtempl.h"
#include "CommandSet.h"
#include "inputhandler.h"
#include "Resource.h"
//#include "KeyboardSettings.h"
#include "mptrack.h"
#include "mainfrm.h"
#include <direct.h>
#include ".\inputhandler.h"

#define TRANSITIONBIT 0x8000
#define REPEATBIT 0x4000

//--------------------------------------------------------------
CInputHandler::CInputHandler(CWnd *mainframe)
{
	m_pMainFrm = mainframe;
		
	//Init CommandSet and Load defaults
	activeCommandSet = new CCommandSet();
	
	CString workingDir;
	char wd[255];
	_getdcwd(_getdrive(), wd, 255);
	workingDir = wd;
	if (!CMainFrame::m_szKbdFile[0]) {
		strcpy(CMainFrame::m_szKbdFile, workingDir + "\\default.mkb");
	}
	if (!(activeCommandSet->LoadFile(CMainFrame::m_szKbdFile))) {
		if (!(activeCommandSet->LoadFile(workingDir + "\\default.mkb"))) {
			AfxMessageBox("Warning! OpenMPT has not been able to locate a keymap file. Please locate one in the settings.\r\nUntil you do so, the keyboard will not work in OpenMPT.");
		}
	}

	//Get Keymap 
	activeCommandSet->GenKeyMap(keyMap);
	
	m_bDistinguishControls = false; 
	m_bDistinguishShifts = false;
	m_bDistinguishAlts = false;
	m_bBypass = false;
	modifierMask=0;
	m_bNoAltMenu = true;
	m_bAutoSave = true;


}
CInputHandler::~CInputHandler(void)
{
	delete activeCommandSet;
}



//--------------------------------------------------------------


CommandID CInputHandler::GeneralKeyEvent(InputTargetContext context, int code, WPARAM wParam , LPARAM lParam)

{
	CommandID executeCommand = kcNull;	
	KeyEventType keyEventType;

	if (code == HC_ACTION)
	{
		//Get the KeyEventType (key up, key down, key repeat)
		DWORD scancode = lParam >> 16;
		if		((scancode & 0xC000) == 0xC000)		keyEventType = kKeyEventUp;
		else if ((scancode & 0xC000) == 0x0000)		keyEventType = kKeyEventDown;
		else										keyEventType = kKeyEventRepeat;

		//Catch modifier change (ctrl, alt, shift) - Only check on keyDown or keyUp.
		if (keyEventType == kKeyEventUp || keyEventType == kKeyEventDown)
		{
			scancode =(lParam >> 16) & 0x1FF;
			CatchModifierChange(wParam, keyEventType, scancode);
		}

		if (keyEventType == kKeyEventUp)
			keyEventType=kKeyEventUp;

		if (!Bypass())	// If bypassing, we want to catch modifiers but not execute commands
			executeCommand = keyMap[context][modifierMask][wParam][keyEventType];
	}

	if (m_pMainFrm && executeCommand != kcNull)
	{
		m_pMainFrm->PostMessage(WM_MOD_KEYCOMMAND, executeCommand, wParam);
	}

	return executeCommand;
}

//--------------------------------------------------------------
CommandID CInputHandler::KeyEvent(InputTargetContext context, UINT &nChar, UINT &nRepCnt, UINT &nFlags, KeyEventType keyEventType, CWnd* pSourceWnd)
{
	
	CommandID executeCommand = keyMap[context][modifierMask][nChar][keyEventType];

/*		if (keyEventType == kKeyEventUp)
			keyEventType=kKeyEventUp;
*/
	if (pSourceWnd == NULL) {
		pSourceWnd = m_pMainFrm;	//by default, send command message to main frame.
	}
	if (pSourceWnd && (executeCommand != kcNull)) 
	{
		pSourceWnd->PostMessage(WM_MOD_KEYCOMMAND, executeCommand, nChar);
	}

	return executeCommand;
}


//--------------------------------------------------------------
//Deal with Modifier keypresses. Private surouting used above.
bool CInputHandler::CatchModifierChange(WPARAM wParam, KeyEventType keyEventType, int scancode)
{
	UINT tempModifierMask = 0;
	switch(wParam)
	{	
		case VK_CONTROL: 
/*			if (m_bDistinguishControls)
			{
				if		(scancode == SC_LCONTROL)	tempModifierMask |= LControl;
				else if (scancode == SC_RCONTROL)	tempModifierMask |= RControl;
				break;
			}
			else
			{
*/				tempModifierMask |= HOTKEYF_CONTROL; 
				break;	
//			}

		case VK_SHIFT:
/*			if (m_bDistinguishShifts)
			{
				if		(scancode == SC_LSHIFT)		tempModifierMask |= LShift;
				else if (scancode == SC_RSHIFT)		tempModifierMask |= RShift;
				break;
			}
			else
			{
*/				tempModifierMask |= HOTKEYF_SHIFT; 
				break;
//			}
			
		case VK_MENU:  
/*			if (m_bDistinguishAlts)
			{
				if		(scancode == SC_LALT)		{tempModifierMask |= LAlt;	break;}
				else if	(scancode == SC_RALT)		{tempModifierMask |= RAlt;	break;}
			}
			else
			{
*/				tempModifierMask |= HOTKEYF_ALT; 
				break;
//			}
	}

	if (tempModifierMask)	//This keypress just changed the modifier mask
	{
		if (keyEventType == kKeyEventDown)
		{
			modifierMask |= tempModifierMask;
			LogModifiers(modifierMask);
		}
		if (keyEventType == kKeyEventUp)
			modifierMask &= ~tempModifierMask;

		return true;
	}
	
	return false;
}

//--------------------------------------------------------------
DWORD CInputHandler::GetKey(CommandID c)
{
	return 0;// command[c].kc.code command[c].kc.mod;
}

//-------------------------------------------------------------
//----------------------- Command Access
//--------------------------------------------------------------


int CInputHandler::SetCommand(InputTargetContext context, CommandID cmd, UINT modifierMask, UINT actionKey, UINT keyEventType)
{
	int deletedCommand = -1;
	
	KeyCombination curKc;
	curKc.code=actionKey;
	curKc.ctx=context;
	curKc.event=(KeyEventType)keyEventType;
	curKc.mod=modifierMask;

	activeCommandSet->Add(curKc, cmd, true);

	return deletedCommand;
}

KeyCombination CInputHandler::GetKey(CommandID cmd, UINT key)
{
	return activeCommandSet->GetKey(cmd, key);	
}


int CInputHandler::GetKeyListSize(CommandID cmd)
{
	return activeCommandSet->GetKeyListSize(cmd);
}

CString CInputHandler::GetCommandText(CommandID cmd)
{
	return activeCommandSet->GetCommandText(cmd);
}


//-------------------------------------------------------------
//----------------------- Misc
//--------------------------------------------------------------



/*
void CInputHandler::GenCommandList(const KeyMap &km, CommandStruct coms[], )
{

}
*/

void CInputHandler::LogModifiers(UINT mask)
{
	Log("----------------------------------\n");
	if (mask & HOTKEYF_CONTROL) Log("Ctrl On"); else Log("Ctrl --");
	if (mask & HOTKEYF_SHIFT)   Log("\tShft On"); else Log("\tShft --");
	if (mask & HOTKEYF_ALT)     Log("\tAlt  On\n"); else Log("\tAlt  --\n");
}


KeyEventType CInputHandler::GetKeyEventType(UINT nFlags)
{	
	if (nFlags & TRANSITIONBIT)		//Key released
		return kKeyEventUp;
	else						
		if (nFlags & REPEATBIT)		//key repeated
			return kKeyEventRepeat;
		else						//new key down
			return kKeyEventDown;		
}

bool CInputHandler::ShiftPressed(void)
{
	return (modifierMask & HOTKEYF_SHIFT);
}

bool CInputHandler::CtrlPressed(void)
{
	return (modifierMask & HOTKEYF_CONTROL);
}

bool CInputHandler::AltPressed(void)
{
	return (modifierMask & HOTKEYF_ALT);
}

void CInputHandler::Bypass(bool b)
{
	m_bBypass=b;
}

bool CInputHandler::Bypass()
{
	return m_bBypass;
}

WORD CInputHandler::GetModifierMask()
{
	return (WORD)modifierMask;
}

void CInputHandler::SetModifierMask(WORD mask)
{
	modifierMask=mask;
}

CString CInputHandler::GetCurModifierText()
{
	return activeCommandSet->GetModifierText(modifierMask);
}

CString CInputHandler::GetKeyTextFromCommand(CommandID c)
{
	return activeCommandSet->GetKeyTextFromCommand(c, 0);
}

#define FILENEW 1
#define MAINVIEW 59392

CString CInputHandler::GetMenuText(UINT id)
{
	CString s;
	CommandID c;

	switch(id)
	{
		case FILENEW:				s="New\t"; c=kcFileNew; break;
		case ID_FILE_OPEN:			s="Open...\t"; c=kcFileOpen;  break;
		case ID_FILE_CLOSE:			s="Close\t"; c=kcFileClose; break;
		case ID_FILE_SAVE:			s="Save\t"; c=kcFileSave; break;
		case ID_FILE_SAVE_AS:		s="Save As...\t"; c=kcFileSaveAs; break;
		case ID_FILE_SAVEASWAVE:	s="Save as Wave...\t"; c=kcFileSaveAsWave; break;
		case ID_FILE_SAVEASMP3:		s="Save as MP3...\t"; c=kcFileSaveAsMP3; break;
		case ID_FILE_SAVEMIDI:		s="Export as Midi...\t"; c=kcFileSaveMidi; break;
		case ID_IMPORT_MIDILIB:		s="Import Midi Library...\t"; c=kcFileImportMidiLib; break;
		case ID_ADD_SOUNDBANK:		s="Add Sound Bank...\t"; c=kcFileAddSoundBank; break;

		case ID_PLAYER_PLAY:		s="Pause/Resume\t"; c= kcPlayPauseSong; break;
		case ID_PLAYER_PLAYFROMSTART:	s="Play from start\t"; c=kcPlaySongFromStart; break;
		case ID_PLAYER_STOP:		s="Stop\t"; c=kcStopSong; break;
		case ID_PLAYER_PAUSE:		s="Pause\t"; c=kcPauseSong; break;
		case ID_MIDI_RECORD:		s="Midi Record\t"; c=kcMidiRecord; break;
		case ID_ESTIMATESONGLENGTH:	s="Estimate Song Length\t"; c=kcEstimateSongLength; break;
		case ID_APPROX_BPM:			s="Approx. real BPM\t"; c=kcApproxRealBPM; break;

		case ID_EDIT_UNDO:			s="Undo\t"; c=kcEditUndo; break;
		case ID_EDIT_CUT:			s="Cut\t"; c=kcEditCut; break;
		case ID_EDIT_COPY:			s="Copy\t"; c=kcEditCopy; break;
		case ID_EDIT_PASTE:			s="Paste\t"; c=kcEditPaste; break;
		case ID_EDIT_PASTE_SPECIAL:	s="Mix Paste\t"; c=kcEditMixPaste; break;
		case ID_EDIT_SELECT_ALL:	s="Select All\t"; c=kcEditSelectAll; break;
		case ID_EDIT_FIND:			s="Find\t"; c=kcEditFind; break;
		case ID_EDIT_FINDNEXT:		s="Find Next\t"; c=kcEditFindNext; break;

		case ID_VIEW_GLOBALS:		s="Globals\t"; c=kcViewGeneral; break;
		case ID_VIEW_SAMPLES:		s="Samples\t"; c=kcViewSamples; break;
		case ID_VIEW_PATTERNS:		s="Patterns\t"; c=kcViewPattern; break;
		case ID_VIEW_INSTRUMENTS:	s="Instruments\t"; c=kcViewInstruments; break;
		case ID_VIEW_COMMENTS:		s="Comments\t"; c=kcViewComments; break;
		case MAINVIEW:				s="Main\t"; c=kcViewMain; break;
		case IDD_TREEVIEW:			s="Tree\t"; c=kcViewTree; break;
		case ID_VIEW_OPTIONS:		s="Setup...\t"; c=kcViewOptions; break;
		case ID_HELP:				s="Contents (todo)"; c=kcHelp; break;
		case ID_PLUGIN_SETUP:		s="Plugin Setup...\t"; c=kcViewAddPlugin; break;
		case ID_CHANNEL_MANAGER:	s="Channel Manager...\t"; c=kcViewChannelManager; break;
/*	
		case ID_WINDOW_NEW:			s="New Window\t"; c=kcWindowNew; break;
		case ID_WINDOW_CASCADE:		s="Cascade\t"; c=kcWindowCascade; break;
		case ID_WINDOW_TILE_HORZ:	s="Tile Horizontal\t"; c=kcWindowTileHorz; break;
		case ID_WINDOW_TILE_VERT:	s="Tile Vertical\t"; c=kcWindowTileVert; break;
*/
		default: return "Unknown Item.";
	}

	s+=activeCommandSet->GetKeyTextFromCommand(c,0);

	return s;
}

void CInputHandler::UpdateMainMenu()
{	
	CMenu *pMenu = (CMainFrame::GetMainFrame())->GetMenu();
	if (!pMenu) return;

	pMenu->GetSubMenu(0)->ModifyMenu(0, MF_BYPOSITION | MF_STRING, 0, GetMenuText(FILENEW));
	pMenu->ModifyMenu(ID_FILE_OPEN, MF_BYCOMMAND | MF_STRING, ID_FILE_OPEN, GetMenuText(ID_FILE_OPEN));
	pMenu->ModifyMenu(ID_FILE_CLOSE, MF_BYCOMMAND | MF_STRING, ID_FILE_CLOSE, GetMenuText(ID_FILE_CLOSE));
	pMenu->ModifyMenu(ID_FILE_SAVE, MF_BYCOMMAND | MF_STRING, ID_FILE_SAVE, GetMenuText(ID_FILE_SAVE));
	pMenu->ModifyMenu(ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_STRING, ID_FILE_SAVE_AS, GetMenuText(ID_FILE_SAVE_AS));
	pMenu->ModifyMenu(ID_FILE_SAVEASWAVE, MF_BYCOMMAND | MF_STRING, ID_FILE_SAVEASWAVE, GetMenuText(ID_FILE_SAVEASWAVE));
	pMenu->ModifyMenu(ID_FILE_SAVEASMP3, MF_BYCOMMAND | MF_STRING, ID_FILE_SAVEASMP3, GetMenuText(ID_FILE_SAVEASMP3));
	pMenu->ModifyMenu(ID_FILE_SAVEMIDI, MF_BYCOMMAND | MF_STRING, ID_FILE_SAVEMIDI, GetMenuText(ID_FILE_SAVEMIDI));
	pMenu->ModifyMenu(ID_IMPORT_MIDILIB, MF_BYCOMMAND | MF_STRING, ID_IMPORT_MIDILIB, GetMenuText(ID_IMPORT_MIDILIB));
	pMenu->ModifyMenu(ID_ADD_SOUNDBANK, MF_BYCOMMAND | MF_STRING, ID_ADD_SOUNDBANK, GetMenuText(ID_ADD_SOUNDBANK));

	pMenu->ModifyMenu(ID_PLAYER_PLAY, MF_BYCOMMAND | MF_STRING, ID_PLAYER_PLAY, GetMenuText(ID_PLAYER_PLAY));
	pMenu->ModifyMenu(ID_PLAYER_PLAYFROMSTART, MF_BYCOMMAND | MF_STRING, ID_PLAYER_PLAYFROMSTART, GetMenuText(ID_PLAYER_PLAYFROMSTART));
	pMenu->ModifyMenu(ID_PLAYER_STOP, MF_BYCOMMAND | MF_STRING, ID_PLAYER_STOP, GetMenuText(ID_PLAYER_STOP));
	pMenu->ModifyMenu(ID_PLAYER_PAUSE, MF_BYCOMMAND | MF_STRING, ID_PLAYER_PAUSE, GetMenuText(ID_PLAYER_PAUSE));
	pMenu->ModifyMenu(ID_MIDI_RECORD, MF_BYCOMMAND | MF_STRING, ID_MIDI_RECORD, GetMenuText(ID_MIDI_RECORD));
	pMenu->ModifyMenu(ID_ESTIMATESONGLENGTH, MF_BYCOMMAND | MF_STRING, ID_ESTIMATESONGLENGTH, GetMenuText(ID_ESTIMATESONGLENGTH));
	pMenu->ModifyMenu(ID_APPROX_BPM, MF_BYCOMMAND | MF_STRING, ID_APPROX_BPM,  GetMenuText(ID_APPROX_BPM));


	pMenu->ModifyMenu(ID_EDIT_UNDO, MF_BYCOMMAND | MF_STRING, ID_EDIT_UNDO, GetMenuText(ID_EDIT_UNDO));
	pMenu->ModifyMenu(ID_EDIT_CUT, MF_BYCOMMAND | MF_STRING, ID_EDIT_CUT, GetMenuText(ID_EDIT_CUT));
	pMenu->ModifyMenu(ID_EDIT_COPY, MF_BYCOMMAND | MF_STRING, ID_EDIT_COPY, GetMenuText(ID_EDIT_COPY));
	pMenu->ModifyMenu(ID_EDIT_PASTE, MF_BYCOMMAND | MF_STRING, ID_EDIT_PASTE, GetMenuText(ID_EDIT_PASTE));
	pMenu->ModifyMenu(ID_EDIT_PASTE_SPECIAL, MF_BYCOMMAND | MF_STRING, ID_EDIT_PASTE_SPECIAL, GetMenuText(ID_EDIT_PASTE_SPECIAL));
	pMenu->ModifyMenu(ID_EDIT_SELECT_ALL, MF_BYCOMMAND | MF_STRING, ID_EDIT_SELECT_ALL, GetMenuText(ID_EDIT_SELECT_ALL));
	pMenu->ModifyMenu(ID_EDIT_FIND, MF_BYCOMMAND | MF_STRING, ID_EDIT_FIND, GetMenuText(ID_EDIT_FIND));
	pMenu->ModifyMenu(ID_EDIT_FINDNEXT, MF_BYCOMMAND | MF_STRING, ID_EDIT_FINDNEXT, GetMenuText(ID_EDIT_FINDNEXT));
	
	pMenu->ModifyMenu(ID_VIEW_GLOBALS, MF_BYCOMMAND | MF_STRING, ID_VIEW_GLOBALS, GetMenuText(ID_VIEW_GLOBALS));
	pMenu->ModifyMenu(ID_VIEW_SAMPLES, MF_BYCOMMAND | MF_STRING, ID_VIEW_SAMPLES, GetMenuText(ID_VIEW_SAMPLES));
	pMenu->ModifyMenu(ID_VIEW_PATTERNS, MF_BYCOMMAND | MF_STRING, ID_VIEW_PATTERNS, GetMenuText(ID_VIEW_PATTERNS));
	pMenu->ModifyMenu(ID_VIEW_INSTRUMENTS, MF_BYCOMMAND | MF_STRING, ID_VIEW_INSTRUMENTS, GetMenuText(ID_VIEW_INSTRUMENTS));
	pMenu->ModifyMenu(ID_VIEW_COMMENTS, MF_BYCOMMAND | MF_STRING, ID_VIEW_COMMENTS, GetMenuText(ID_VIEW_COMMENTS));
	pMenu->ModifyMenu(MAINVIEW, MF_BYCOMMAND | MF_STRING, MAINVIEW, GetMenuText(MAINVIEW));
	pMenu->ModifyMenu(IDD_TREEVIEW, MF_BYCOMMAND | MF_STRING, IDD_TREEVIEW, GetMenuText(IDD_TREEVIEW));
	pMenu->ModifyMenu(ID_VIEW_OPTIONS, MF_BYCOMMAND | MF_STRING, ID_VIEW_OPTIONS, GetMenuText(ID_VIEW_OPTIONS));
	pMenu->ModifyMenu(ID_PLUGIN_SETUP, MF_BYCOMMAND | MF_STRING, ID_PLUGIN_SETUP, GetMenuText(ID_PLUGIN_SETUP));
	pMenu->ModifyMenu(ID_CHANNEL_MANAGER, MF_BYCOMMAND | MF_STRING, ID_CHANNEL_MANAGER, GetMenuText(ID_CHANNEL_MANAGER));
	pMenu->ModifyMenu(ID_HELP, MF_BYCOMMAND | MF_STRING, ID_HELP, GetMenuText(ID_HELP));
/*	
	pMenu->ModifyMenu(ID_WINDOW_NEW, MF_BYCOMMAND | MF_STRING, ID_WINDOW_NEW, GetMenuText(ID_WINDOW_NEW));
	pMenu->ModifyMenu(ID_WINDOW_CASCADE, MF_BYCOMMAND | MF_STRING, ID_WINDOW_CASCADE, GetMenuText(ID_WINDOW_CASCADE));
	pMenu->ModifyMenu(ID_WINDOW_TILE_HORZ, MF_BYCOMMAND | MF_STRING, ID_WINDOW_TILE_HORZ, GetMenuText(ID_WINDOW_TILE_HORZ));
	pMenu->ModifyMenu(ID_WINDOW_TILE_VERT, MF_BYCOMMAND | MF_STRING, ID_WINDOW_TILE_VERT, GetMenuText(ID_WINDOW_TILE_VERT));
*/

	
}


void CInputHandler::SetNewCommandSet(CCommandSet *newSet)
{
	activeCommandSet->Copy(newSet);
	activeCommandSet->GenKeyMap(keyMap);
	UpdateMainMenu();
}

bool CInputHandler::noAltMenu()
{
	return m_bNoAltMenu;
}



bool CInputHandler::SetITEffects(void)
{
	Log("Setting command set to IT.\n");
	bool retval = activeCommandSet->QuickChange_SetEffectsIT();
	activeCommandSet->GenKeyMap(keyMap);
	return retval;
}


bool CInputHandler::SetXMEffects(void)
{
	Log("Setting command set to XM.\n");
	bool retval = activeCommandSet->QuickChange_SetEffectsXM();
	activeCommandSet->GenKeyMap(keyMap);
	return retval;
}


bool CInputHandler::isKeyPressHandledByTextBox(DWORD key) 
{

	//Alpha-numerics (only shift or no modifier):
	if (!CtrlPressed() &&  !AltPressed() && 
        ((key>='A'&&key<='Z') || (key>='0'&&key<='9') || 
		 key==VK_DIVIDE  || key==VK_MULTIPLY || key==VK_SPACE || key==VK_RETURN ||
		 key==VK_CAPITAL || (key>=VK_OEM_1 && key<=VK_OEM_3) || (key>=VK_OEM_4 && key<=VK_OEM_8)))
		return true;
	
	//navigation (any modifier):
	if (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN || 
		key == VK_HOME || key == VK_END || key == VK_DELETE || key == VK_INSERT || key == VK_BACK)
		return true;
	
	//Copy paste etc..
	if (CMainFrame::GetInputHandler()->GetModifierMask()==HOTKEYF_CONTROL && 
		(key == 'Y' || key == 'Z' || key == 'X' ||  key == 'C' || key == 'V'))
		return true;

	return false;
}