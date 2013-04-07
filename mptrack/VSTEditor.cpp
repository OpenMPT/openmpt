/*
 * VSTEditor.cpp
 * -------------
 * Purpose: Implementation of the custom plugin editor window that is used if a plugin provides an own editor GUI.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "vsteditor.h"

#ifndef NO_VST

COwnerVstEditor::COwnerVstEditor(CVstPlugin &plugin) : CAbstractVstEditor(plugin)
//-------------------------------------------------------------------------------
{

}


COwnerVstEditor::~COwnerVstEditor()
//---------------------------------
{

}


bool COwnerVstEditor::OpenEditor(CWnd *parent)
//--------------------------------------------
{
	Create(IDD_PLUGINEDITOR, parent);

	SetupMenu();

	// Set editor window size
	ERect *pRect = nullptr;
	m_VstPlugin.Dispatch(effEditGetRect, 0, 0, &pRect, 0);
	m_VstPlugin.Dispatch(effEditOpen, 0, 0, m_hWnd, 0);
	m_VstPlugin.Dispatch(effEditGetRect, 0, 0, &pRect, 0);
	if((pRect) && (pRect->right > pRect->left) && (pRect->bottom > pRect->top))
	{
		// Plugin provided valid window size.
		SetSize(pRect->right - pRect->left, pRect->bottom - pRect->top);
	}

	// Restore previous editor position
	int editorX, editorY;
	m_VstPlugin.GetEditorPos(editorX, editorY);

	if((editorX >= 0) && (editorY >= 0))
	{
		int cxScreen = GetSystemMetrics(SM_CXSCREEN);
		int cyScreen = GetSystemMetrics(SM_CYSCREEN);
		if((editorX + 8 < cxScreen) && (editorY + 8 < cyScreen))
		{
			SetWindowPos(NULL, editorX, editorY, 0, 0,
				SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}
	SetTitle();

	m_VstPlugin.Dispatch(effEditTop, 0,0, NULL, 0);
	m_VstPlugin.Dispatch(effEditIdle, 0,0, NULL, 0);

	// Set knob mode to linear (2) instead of circular (0) for those plugins that support it (e.g. Steinberg VB-1)
	m_VstPlugin.Dispatch(effSetEditKnobMode, 0, 2, nullptr, 0.0f);

	ShowWindow(SW_SHOW);
	return TRUE;
}


void COwnerVstEditor::OnClose()
//-----------------------------
{
	DoClose();
}


void COwnerVstEditor::OnOK()
//--------------------------
{
	OnClose();
}


void COwnerVstEditor::OnCancel()
//------------------------------
{
	OnClose();
}


void COwnerVstEditor::DoClose()
//-----------------------------
{
	if(m_hWnd)
	{
		CRect rect;
		GetWindowRect(&rect);
		m_VstPlugin.SetEditorPos(rect.left, rect.top);
	}
	m_VstPlugin.Dispatch(effEditClose, 0, 0, NULL, 0);
	if(m_hWnd)
	{
		DestroyWindow();
	}
}


bool COwnerVstEditor::SetSize(int contentWidth, int contentHeight)
//----------------------------------------------------------------
{
	if(contentWidth < 0 || contentHeight < 0 || !m_hWnd)
	{
		return false;
	}

	CRect rcWnd, rcClient;

	// Get border / menu size.
	GetWindowRect(&rcWnd);
	GetClientRect(&rcClient);

	MENUBARINFO mbi;
	MemsetZero(mbi);
	mbi.cbSize = sizeof(mbi);
	GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);
	int menuHeight = mbi.rcBar.bottom - mbi.rcBar.top;

	// Preliminary setup, which might have to be adjusted for small (narrow) plugin GUIs again,
	// since the menu might be two lines high...
	const int windowWidth = rcWnd.Width() - rcClient.Width() + contentWidth;
	const int windowHeight = rcWnd.Height() - rcClient.Height() + contentHeight;
	SetWindowPos(NULL, 0, 0,
		windowWidth, windowHeight,
		SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

	// Check if the height of the menu bar has changed.
	GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

	const int menuHeightDiff = (mbi.rcBar.bottom - mbi.rcBar.top) - menuHeight;

	if(menuHeightDiff != 0)
	{
		// Menu height changed, resize window so that the whole content area can be viewed again.
		SetWindowPos(NULL, 0, 0,
			windowWidth, windowHeight + menuHeightDiff,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}

	return true;
}


#endif // NO_VST
