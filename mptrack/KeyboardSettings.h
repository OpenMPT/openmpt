#pragma once

class CKeyboardSettings
{
public:
	CKeyboardSettings(CInputHandler *ih);
    ~CKeyboardSettings(void);
	static void LoadDefaults(CInputHandler *inputHandler);
	static int AsciiToScancode(char ch);

	CInputHandler *pInputHandler;
};
