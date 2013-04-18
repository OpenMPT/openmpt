/*
 * Reporting.h
 * -----------
 * Purpose: A class for showing notifications, prompts, etc...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


enum ConfirmAnswer
{
	cnfYes,
	cnfNo,
	cnfCancel,
};


enum RetryAnswer
{
	rtyRetry,
	rtyCancel,
};


//=============
class Reporting
//=============
{

protected:

	static UINT ShowNotification(const char *text, const char *caption, UINT flags, const CWnd *parent);

public:

	// TODO Quick'n'dirty workaround for MsgBox(). Shouldn't be public.
	static UINT CustomNotification(const char *text, const char *caption, UINT flags, const CWnd *parent) { return ShowNotification(text, caption, flags, parent); };

	// Show a simple notification
	static void Notification(const char *text, const CWnd *parent = nullptr);
	static void Notification(const char *text, const char *caption, const CWnd *parent = nullptr);

	// Show a simple information
	static void Information(const char *text, const CWnd *parent = nullptr);
	static void Information(const char *text, const char *caption, const CWnd *parent = nullptr);

	// Show a simple warning
	static void Warning(const char *text, const CWnd *parent = nullptr);
	static void Warning(const char *text, const char *caption, const CWnd *parent = nullptr);

	// Show an error box.
	static void Error(const char *text, const CWnd *parent = nullptr);
	static void Error(const char *text, const char *caption, const CWnd *parent = nullptr);

	// Show a confirmation dialog.
	static ConfirmAnswer Confirm(const char *text, bool showCancel = false, bool defaultNo = false, const CWnd *parent = nullptr);
	static ConfirmAnswer Confirm(const char *text, const char *caption, bool showCancel = false, bool defaultNo = false, const CWnd *parent = nullptr);

	// Show a confirmation dialog.
	static RetryAnswer RetryCancel(const char *text, const CWnd *parent = nullptr);
	static RetryAnswer RetryCancel(const char *text, const char *caption, const CWnd *parent = nullptr);

};
