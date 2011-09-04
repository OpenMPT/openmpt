/*
 *
 * Reporting.h
 * -----------
 * Purpose: Header file for reporting class.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#pragma once
#ifndef REPORTING_H
#define REPORTING_H

enum ConfirmAnswer
{
	cnfYes,
	cnfNo,
	cnfCancel,
};

//=============
class Reporting
//=============
{

protected:

	static UINT ShowNotification(const char *text, const char *caption, UINT flags, const CWnd *parent);

public:

#ifdef MODPLUG_TRACKER
	// TODO Quick'n'dirty workaround for MsgBox(). Shouldn't be public.
	static UINT CustomNotification(const char *text, const char *caption, UINT flags, const CWnd *parent) { return ShowNotification(text, caption, flags, parent); };
#endif // MODPLUG_TRACKER

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

};

#endif // REPORTING_H
