#pragma once

#define END_VIEWPATTERNS     200
#define END_CTRLPATTERNS     400
#define END_VIEWSAMPLES      600
#define END_CTRLSAMPLES      800
#define END_VIEWINSTRUMENTS 1000
#define END_CTRLINSTRUMENTS 1200
#define END_VIEWCOMMENTS    1400
#define END_CTRLCOMMENTS    1800
#define END_VIEWGENERAL     2000
#define END_CTRLGENERAL     2200
#define END_MODDOCUMENT     2400
#define END_MAINFRAME       2600

//not sure why I shoved message IDs here anymore. Might want to move em. -rewbs
enum {
	WM_MOD_UPDATEPOSITION	=	(WM_USER+1973),
	WM_MOD_DESTROYMODDOC,
	WM_MOD_DESTROYPATEDIT,
	WM_MOD_UPDATESOUNDBUFFER,
	WM_MOD_INVALIDATEPATTERNS,
	WM_MOD_DESTROYSAMPLE,
	WM_MOD_DESTROYINSTRUMENT,
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
};


class CInputHandler
{

public:
	CInputHandler(CWnd *mainframe);
	~CInputHandler(void);
	CommandID GeneralKeyEvent(InputTargetContext context, int code, WPARAM wParam , LPARAM lParam);
	CommandID KeyEvent(InputTargetContext context, UINT &nChar, UINT &nRepCnt, UINT &nFlags, KeyEventType keyEventType, CWnd* pSourceWnd=NULL);
	int SetCommand(InputTargetContext context, CommandID command, UINT modifierMask, UINT actionKey, UINT keyEventType);
	KeyEventType GetKeyEventType(UINT nFlags);
	DWORD GetKey(CommandID);
	bool isKeyPressHandledByTextBox(DWORD wparam);

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

public:
	CCommandSet *activeCommandSet;
	bool ShiftPressed(void);
	bool SelectionPressed(void);
	bool CtrlPressed(void);
	bool AltPressed(void);
	bool Bypass();
	void Bypass(bool);
	WORD GetModifierMask();
	void SetModifierMask(WORD mask);
	CString GetCurModifierText();
	CString GetKeyTextFromCommand(CommandID c);
	CString GetMenuText(UINT id);
	void UpdateMainMenu();
	void SetNewCommandSet(CCommandSet * newSet);
	bool noAltMenu();
	bool m_bAutoSave;
	bool SetXMEffects(void);
	bool SetITEffects(void);
};

