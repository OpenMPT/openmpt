/*
 * Reporting.cpp
 * -------------
 * Purpose: A class for showing notifications, prompts, etc...
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "Stdafx.h"
#include "Reporting.h"
#include "../mptrack/Mainfrm.h"
#include "../mptrack/InputHandler.h"


OPENMPT_NAMESPACE_BEGIN


static inline UINT LogLevelToFlags(LogLevel level)
//------------------------------------------------
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


static std::wstring GetTitle()
//----------------------------
{
	return MAINFRAME_TITLEW;
}


static std::wstring FillEmptyCaption(const std::wstring &caption, LogLevel level)
//-------------------------------------------------------------------------------
{
	std::wstring result = mpt::ToWide(caption);
	if(result.empty())
	{
		result = GetTitle() + std::wstring(L" - ");
		switch(level)
		{
		case LogDebug: result += L"Debug"; break;
		case LogNotification: result += L"Notification"; break;
		case LogInformation: result += L"Information"; break;
		case LogWarning: result += L"Warning"; break;
		case LogError: result += L"Error"; break;
		}
	}
	return result;
}


static std::wstring FillEmptyCaption(const std::wstring &caption)
//---------------------------------------------------------------
{
	std::wstring result = mpt::ToWide(caption);
	if(result.empty())
	{
		result = GetTitle();
	}
	return result;
}


static UINT ShowNotificationImpl(const std::wstring &text, const std::wstring &caption, UINT flags, const CWnd *parent)
//---------------------------------------------------------------------------------------------------------------------
{
	if(parent == nullptr)
	{
		parent = CMainFrame::GetActiveWindow();
	}
	BypassInputHandler bih;
	UINT result = ::MessageBoxW(parent->GetSafeHwnd(), text.c_str(), caption.empty() ? MAINFRAME_TITLEW : caption.c_str(), flags);

	return result;
}


UINT Reporting::CustomNotification(const AnyStringLocale &text, const AnyStringLocale &caption, UINT flags, const CWnd *parent)
//-----------------------------------------------------------------------------------------------------------------------------
{
	return ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption)), flags, parent);
}


void Reporting::Notification(const AnyStringLocale &text, const CWnd *parent)
//---------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(std::wstring(), LogNotification), LogLevelToFlags(LogNotification), parent);
}
void Reporting::Notification(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
//-----------------------------------------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption), LogNotification), LogLevelToFlags(LogNotification), parent);
}


void Reporting::Information(const AnyStringLocale &text, const CWnd *parent)
//--------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(std::wstring(), LogInformation), LogLevelToFlags(LogInformation), parent);
}
void Reporting::Information(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
//----------------------------------------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption), LogInformation), LogLevelToFlags(LogInformation), parent);
}


void Reporting::Warning(const AnyStringLocale &text, const CWnd *parent)
//----------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(std::wstring(), LogWarning), LogLevelToFlags(LogWarning), parent);
}
void Reporting::Warning(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
//------------------------------------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption), LogWarning), LogLevelToFlags(LogWarning), parent);
}


void Reporting::Error(const AnyStringLocale &text, const CWnd *parent)
//--------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(std::wstring(), LogError), LogLevelToFlags(LogError), parent);
}
void Reporting::Error(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
//----------------------------------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption), LogError), LogLevelToFlags(LogError), parent);
}


void Reporting::Message(LogLevel level, const AnyStringLocale &text, const CWnd *parent)
//--------------------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(std::wstring(), level), LogLevelToFlags(level), parent);
}
void Reporting::Message(LogLevel level, const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
//----------------------------------------------------------------------------------------------------------------------
{
	ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption), level), LogLevelToFlags(level), parent);
}


ConfirmAnswer Reporting::Confirm(const AnyStringLocale &text, bool showCancel, bool defaultNo, const CWnd *parent)
//----------------------------------------------------------------------------------------------------------------
{
	return Confirm(mpt::ToWide(text), GetTitle() + L" - Confirmation", showCancel, defaultNo, parent);
}


ConfirmAnswer Reporting::Confirm(const AnyStringLocale &text, const AnyStringLocale &caption, bool showCancel, bool defaultNo, const CWnd *parent)
//------------------------------------------------------------------------------------------------------------------------------------------------
{
	UINT result = ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption)), (showCancel ? MB_YESNOCANCEL : MB_YESNO) | MB_ICONQUESTION | (defaultNo ? MB_DEFBUTTON2 : 0), parent);
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
//---------------------------------------------------------------------------------
{
	return RetryCancel(mpt::ToWide(text), GetTitle(), parent);
}


RetryAnswer Reporting::RetryCancel(const AnyStringLocale &text, const AnyStringLocale &caption, const CWnd *parent)
//-----------------------------------------------------------------------------------------------------------------
{
	UINT result = ShowNotificationImpl(mpt::ToWide(text), FillEmptyCaption(mpt::ToWide(caption)), MB_RETRYCANCEL, parent);
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
