/*
 * InputHandler.h
 * --------------
 * Purpose: Implementation of keyboard input handling, keymap loading, ...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

#include "CommandSet.h"

OPENMPT_NAMESPACE_BEGIN

// Hook codes
enum
{
	HC_MIDI = 0x8000,
};

class CInputHandler
{
protected:
	CWnd *m_pMainFrm;
	KeyMap m_keyMap;
	FlagSet<Modifiers> m_modifierMask = ModNone;
	int m_bypassCount = 0;
	bool m_bInterceptWindowsKeys : 1, m_bInterceptNumLock : 1, m_bInterceptCapsLock : 1, m_bInterceptScrollLock : 1;

public:
	std::unique_ptr<CCommandSet> m_activeCommandSet;

	std::array<CommandID, 10> m_lastCommands;
	size_t m_lastCommandPos = 0;

public:
	CInputHandler(CWnd *mainframe);
	CommandID GeneralKeyEvent(InputTargetContext context, int code, WPARAM wParam , LPARAM lParam);
	CommandID KeyEvent(InputTargetContext context, UINT &nChar, UINT &nRepCnt, UINT &nFlags, KeyEventType keyEventType, CWnd *pSourceWnd = nullptr);
	static KeyEventType GetKeyEventType(UINT nFlags);
	bool IsKeyPressHandledByTextBox(DWORD wparam, HWND hWnd) const;
	CommandID HandleMIDIMessage(InputTargetContext context, uint32 message);

	int GetKeyListSize(CommandID cmd) const;

protected:
	void LogModifiers();
	bool CatchModifierChange(WPARAM wParam, KeyEventType keyEventType, int scancode);
	bool InterceptSpecialKeys(UINT nChar, UINT nFlags, bool generateMsg);
	void SetupSpecialKeyInterception();
	CommandID SendCommands(CWnd *wnd, const KeyMapRange &cmd);

public:
	bool ShiftPressed() const;
	bool SelectionPressed() const;
	bool CtrlPressed() const;
	bool AltPressed() const;
	bool IsBypassed() const;
	void Bypass(bool);
	FlagSet<Modifiers> GetModifierMask() const;
	void SetModifierMask(FlagSet<Modifiers> mask);
	CString GetKeyTextFromCommand(CommandID c, const TCHAR *prependText = nullptr) const;
	CString GetMenuText(UINT id) const;
	void UpdateMainMenu();
	void SetNewCommandSet(const CCommandSet *newSet);
	bool SetEffectLetters(const CModSpecifications &modSpecs);
};


// RAII object for temporarily bypassing the input handler
class BypassInputHandler
{
private:
	bool bypassed = false;
public:
	BypassInputHandler();
	~BypassInputHandler();
};

OPENMPT_NAMESPACE_END
