/*
 * Reporting.cpp
 * -------------
 * Purpose: A class for showing notifications, prompts, etc...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Reporting.h"
#include "../mptrack/Mainfrm.h"
#include "../mptrack/InputHandler.h"


OPENMPT_NAMESPACE_BEGIN


static inline UINT LogLevelToFlags(LogLevel level)
{
	switch(level)
	{
	case LogNotification: return MB_OK; break;
	case LogInformation: return MB_OK | MB_ICONINFORMATION; break;
	case LogWarning: return MB_OK | MB_ICONWARNING; break;
	case LogError: return MB_OK | MB_ICONERROR; break;
	}
	return MB_OK;
}


static CString GetTitle()
{
	return MAINFRAME_TITLE;
}


static CString FillEmptyCaption(const CString &caption, LogLevel level)
{
	CString result = caption;
	if(result.IsEmpty())
	{
		result = GetTitle() + _T(" - ");
		switch(level)
		{
		case LogDebug: result += _T("Debug"); break;
		case LogNotification: result += _T("Notification"); break;
		case LogInformation: result += _T("Information"); break;
		case LogWarning: result += _T("Warning"); break;
		case LogError: result += _T("Error"); break;
		}
	}
	return result;
}


static CString FillEmptyCaption(const CString &caption)
{
	CString result = caption;
	if(result.IsEmpty())
	{
		result = GetTitle();
	}
	return result;
}


static UINT ShowNotificationImpl(const CString &text, const CString &caption, UINT flags, const CWnd *parent)
{
	if(parent == nullptr)
	{
		parent = CMainFrame::GetActiveWindow();
	}
	BypassInputHandler bih;
	UINT result = ::MessageBox(parent->GetSafeHwnd(), text, caption.IsEmpty() ? CString(MAINFRAME_TITLE) : caption, flags);
	return result;
}


UINT Reporting::CustomNotification(const AnyStringLocale &text, const AnyStringLocale &caption, UINT flags, const CWnd *parent)
{
	return ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption)), flags, parent);
}


void Reporting::Notification(const AnyStringLocale &text, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(CString(), LogNotification), LogLevelToFlags(LogNotification), parent);
}
void Reporting::Notification(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption), LogNotification), LogLevelToFlags(LogNotification), parent);
}


void Reporting::Information(const AnyStringLocale &text, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(CString(), LogInformation), LogLevelToFlags(LogInformation), parent);
}
void Reporting::Information(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption), LogInformation), LogLevelToFlags(LogInformation), parent);
}


void Reporting::Warning(const AnyStringLocale &text, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(CString(), LogWarning), LogLevelToFlags(LogWarning), parent);
}
void Reporting::Warning(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption), LogWarning), LogLevelToFlags(LogWarning), parent);
}


void Reporting::Error(const AnyStringLocale &text, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(CString(), LogError), LogLevelToFlags(LogError), parent);
}
void Reporting::Error(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption), LogError), LogLevelToFlags(LogError), parent);
}


void Reporting::Message(LogLevel level, const AnyStringLocale &text, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(CString(), level), LogLevelToFlags(level), parent);
}
void Reporting::Message(LogLevel level, const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
{
	ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption), level), LogLevelToFlags(level), parent);
}


ConfirmAnswer Reporting::Confirm(const AnyStringLocale &text, bool showCancel, bool defaultNo, const CWnd *parent)
{
	return Confirm(mpt::ToCString(text), GetTitle() + _T(" - Confirmation"), showCancel, defaultNo, parent);
}


ConfirmAnswer Reporting::Confirm(const AnyStringLocale &text, const AnyStringLocale &caption, bool showCancel, bool defaultNo, const CWnd *parent)
{
	UINT result = ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption)), (showCancel ? MB_YESNOCANCEL : MB_YESNO) | MB_ICONQUESTION | (defaultNo ? MB_DEFBUTTON2 : 0), parent);
	switch(result)
	{
	case IDYES:
		return cnfYes;
	case IDNO:
		return cnfNo;
	default:
	case IDCANCEL:
		return cnfCancel;
	}
}


RetryAnswer Reporting::RetryCancel(const AnyStringLocale &text, const CWnd *parent)
{
	return RetryCancel(mpt::ToCString(text), GetTitle(), parent);
}


RetryAnswer Reporting::RetryCancel(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
{
	UINT result = ShowNotificationImpl(mpt::ToCString(text), FillEmptyCaption(mpt::ToCString(caption)), MB_RETRYCANCEL, parent);
	switch(result)
	{
	case IDRETRY:
		return rtyRetry;
	default:
	case IDCANCEL:
		return rtyCancel;
	}
}


OPENMPT_NAMESPACE_END
