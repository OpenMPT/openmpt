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
#include "../soundlib/Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

struct CModSpecifications;

class CInputHandler
{
protected:
	CWnd *m_pMainFrm;
	KeyMap m_keyMap;
	int m_bypassCount = 0;
	bool m_bInterceptWindowsKeys : 1, m_bInterceptNumLock : 1, m_bInterceptCapsLock : 1, m_bInterceptScrollLock : 1;

public:
	std::unique_ptr<CCommandSet> m_activeCommandSet;

	std::array<CommandID, 10> m_lastCommands;
	size_t m_lastCommandPos = 0;

	struct KeyboardEvent
	{
		uint32 key;
		uint16 repeatCount;
		uint16 flags;
		KeyEventType keyEventType;
	};

public:
	CInputHandler(CWnd *mainframe);
	CommandID KeyEvent(const InputTargetContext context, const KeyboardEvent &event, CWnd *pSourceWnd = nullptr);
	static KeyboardEvent Translate(const MSG &msg);
	static KeyEventType GetKeyEventType(const MSG &msg);
	static KeyEventType GetKeyEventType(UINT nFlags);
	bool IsKeyPressHandledByTextBox(DWORD wparam, HWND hWnd) const;
	CommandID HandleMIDIMessage(InputTargetContext context, uint32 message);

	int GetKeyListSize(CommandID cmd) const;

protected:
	bool InterceptSpecialKeys(const KeyboardEvent &event);
	void SetupSpecialKeyInterception();
	CommandID SendCommands(CWnd *wnd, const KeyMapRange &cmd);

public:
	bool SelectionPressed() const;
	static bool ShiftPressed();
	static bool CtrlPressed();
	static bool AltPressed();
	OrderTransitionMode ModifierKeysToTransitionMode();
	bool IsBypassed() const;
	void Bypass(bool);
	static FlagSet<Modifiers> GetModifierMask();
	CString GetKeyTextFromCommand(CommandID c, const TCHAR *prependText = nullptr) const;
	CString GetMenuText(UINT id) const;
	void UpdateMainMenu();
	void SetNewCommandSet(const CCommandSet &newSet);
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
