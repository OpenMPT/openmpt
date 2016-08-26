/*
 * InputHandler.h
 * --------------
 * Purpose: Implementation of keyboard input handling, keymap loading, ...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "CommandSet.h"

OPENMPT_NAMESPACE_BEGIN

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
	KeyEventType GetKeyEventType(UINT nFlags);
	bool isKeyPressHandledByTextBox(DWORD wparam);
	CommandID CInputHandler::HandleMIDIMessage(InputTargetContext context, uint32 message);

	int GetKeyListSize(CommandID cmd);

protected:
	CWnd *m_pMainFrm;
	KeyMap keyMap;
	void LogModifiers(UINT mask);
	UINT modifierMask;
	int bypassCount;
	bool m_bNoAltMenu;
	bool CatchModifierChange(WPARAM wParam, KeyEventType keyEventType, int scancode);
	bool m_bInterceptWindowsKeys, m_bInterceptNumLock, m_bInterceptCapsLock, m_bInterceptScrollLock;
	bool InterceptSpecialKeys(UINT nChar , UINT nFlags, bool generateMsg);
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
	CString GetKeyTextFromCommand(CommandID c);
	CString GetMenuText(UINT id);
	void UpdateMainMenu();
	void SetNewCommandSet(CCommandSet * newSet);
	bool noAltMenu() { return m_bNoAltMenu; };
	bool SetEffectLetters(const CModSpecifications &modSpecs);
};


// RAII object for temporarily bypassing the input handler
class BypassInputHandler
{
public:
	BypassInputHandler();
	~BypassInputHandler();
};

OPENMPT_NAMESPACE_END
