/*
 * InputHandler.h
 * --------------
 * Purpose: Implementation of keyboard input handling, keymap loading, ...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

//not sure why I shoved message IDs here anymore. Might want to move em. -rewbs
enum
{
	WM_MOD_UPDATEPOSITION	=	(WM_USER+1973),
	WM_MOD_UPDATEPOSITIONTHREADED,
	WM_MOD_INVALIDATEPATTERNS,
	WM_MOD_ACTIVATEVIEW,
	WM_MOD_CHANGEVIEWCLASS,
	WM_MOD_UNLOCKCONTROLS,
	WM_MOD_CTRLMSG,
	WM_MOD_VIEWMSG,
	WM_MOD_TREEMSG,
	WM_MOD_MIDIMSG,
	WM_MOD_GETTOOLTIPTEXT,
	WM_MOD_DRAGONDROPPING,
	WM_MOD_SPECIALKEY,
	WM_MOD_KBDNOTIFY,
	WM_MOD_INSTRSELECTED,
	WM_MOD_KEYCOMMAND,
	WM_MOD_RECORDPARAM,
};

// Hook codes
enum
{
	HC_MIDI = 0x8000,
};


class CInputHandler
{

public:
	CInputHandler(CWnd *mainframe);
	~CInputHandler();
	CommandID GeneralKeyEvent(InputTargetContext context, int code, WPARAM wParam , LPARAM lParam);
	CommandID KeyEvent(InputTargetContext context, UINT &nChar, UINT &nRepCnt, UINT &nFlags, KeyEventType keyEventType, CWnd* pSourceWnd=NULL);
	int SetCommand(InputTargetContext context, CommandID command, UINT modifierMask, UINT actionKey, UINT keyEventType);
	KeyEventType GetKeyEventType(UINT nFlags);
	DWORD GetKey(CommandID);
	bool isKeyPressHandledByTextBox(DWORD wparam);
	CommandID CInputHandler::HandleMIDIMessage(InputTargetContext context, uint32 message);

	KeyCombination GetKey(CommandID cmd, UINT key);
	int GetKeyListSize(CommandID cmd);
	CString GetCommandText(CommandID cmd);

protected:
	CWnd *m_pMainFrm;
	int AsciiToScancode(char ch);
	KeyMap keyMap;
	void LogModifiers(UINT mask);	
	UINT modifierMask;
	bool m_bBypass;
	bool m_bNoAltMenu;
	bool m_bDistinguishControls, m_bDistinguishShifts, m_bDistinguishAlts;
	bool CatchModifierChange(WPARAM wParam, KeyEventType keyEventType, int scancode);
	bool m_bInterceptWindowsKeys, m_bInterceptNumLock, m_bInterceptCapsLock, m_bInterceptScrollLock;
	int m_nSkipGeneratedKeypresses;
	bool InterceptSpecialKeys( UINT nChar , UINT nFlags );
	void SetupSpecialKeyInterception();

public:
	CCommandSet *activeCommandSet;
	bool ShiftPressed();
	bool SelectionPressed();
	bool CtrlPressed();
	bool AltPressed();
	bool IsBypassed();
	void Bypass(bool);
	WORD GetModifierMask();
	void SetModifierMask(WORD mask);
	CString GetCurModifierText();
	CString GetKeyTextFromCommand(CommandID c);
	CString GetMenuText(UINT id);
	void UpdateMainMenu();
	void SetNewCommandSet(CCommandSet * newSet);
	bool noAltMenu() { return m_bNoAltMenu; };
	bool SetEffectLetters(const CModSpecifications &modSpecs);
};
