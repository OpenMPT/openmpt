/*
 * VSTEditor.cpp
 * -------------
 * Purpose: Implementation of the custom plugin editor window that is used if a plugin provides an own editor GUI.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "resource.h"
#include "Vstplug.h"
#include "VSTEditor.h"


OPENMPT_NAMESPACE_BEGIN


#ifndef NO_VST

BEGIN_MESSAGE_MAP(COwnerVstEditor, CAbstractVstEditor)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()


void COwnerVstEditor::OnPaint()
{
	CAbstractVstEditor::OnPaint();
	CVstPlugin &plugin = static_cast<CVstPlugin &>(m_VstPlugin);
	if(plugin.isBridged)
	{
		// Force redrawing for the plugin window in the bridged process.
		// Otherwise, bridged plugin GUIs will not always be refreshed properly.
		CRect rect;
		if(m_plugWindow.GetUpdateRect(&rect, FALSE))
		{
			CWnd *child = m_plugWindow.GetWindow(GW_CHILD | GW_HWNDFIRST);
			if(child) child->RedrawWindow(&rect, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
		}
	} else
	{
		// For plugins that change their size without telling the host through audioMasterSizeWindow, e.g. Roland D-50
		CRect rect;
		m_plugWindow.GetClientRect(rect);
		if(rect.Width() != m_width || rect.Height() != m_height)
		{
			SetSize(rect.Width(), rect.Height());
		}
	}
}


void COwnerVstEditor::UpdateParamDisplays()
{
	CAbstractVstEditor::UpdateParamDisplays();
	// We trust that the plugin GUI can update its display with a bit of idle time.
	static_cast<CVstPlugin &>(m_VstPlugin).Dispatch(Vst::effEditIdle, 0, 0, nullptr, 0.0f);
}

bool COwnerVstEditor::OpenEditor(CWnd *parent)
{
	Create(IDD_PLUGINEDITOR, parent);

	// Some plugins (e.g. ProteusVX) need to be planted into another control or else they will break our window proc, making the window unusable.
	m_plugWindow.Create(nullptr, WS_CHILD | WS_VISIBLE, CRect(0, 0, 100, 100), this);

	// Set editor window size
	Vst::ERect rect{};
	Vst::ERect *pRect = nullptr;
	CVstPlugin &vstPlug = static_cast<CVstPlugin &>(m_VstPlugin);
	vstPlug.Dispatch(Vst::effEditGetRect, 0, 0, &pRect, 0);
	if(pRect) rect = *pRect;
	vstPlug.Dispatch(Vst::effEditOpen, 0, 0, m_plugWindow.m_hWnd, 0);
	vstPlug.Dispatch(Vst::effEditGetRect, 0, 0, &pRect, 0);
	if(pRect) rect = *pRect;
	if(rect.right > rect.left && rect.bottom > rect.top)
	{
		// Plugin provided valid window size.
		SetSize(rect.Width(), rect.Height());
	}

	vstPlug.Dispatch(Vst::effEditTop, 0,0, nullptr, 0.0f);
	vstPlug.Dispatch(Vst::effEditIdle, 0,0, nullptr, 0.0f);

	// Set knob mode to linear (2) instead of circular (0) for those plugins that support it (e.g. Steinberg VB-1)
	vstPlug.Dispatch(Vst::effSetEditKnobMode, 0, 2, nullptr, 0.0f);

	return CAbstractVstEditor::OpenEditor(parent);
}


void COwnerVstEditor::DoClose()
{
	// Prevent some plugins from storing a bogus window size (e.g. Electri-Q)
	ShowWindow(SW_HIDE);
	if(m_isMinimized) OnNcLButtonDblClk(HTCAPTION, CPoint(0, 0));
	static_cast<CVstPlugin &>(m_VstPlugin).Dispatch(Vst::effEditClose, 0, 0, nullptr, 0.0f);
	CAbstractVstEditor::DoClose();
}


bool COwnerVstEditor::SetSize(int contentWidth, int contentHeight)
{
	if(contentWidth < 0 || contentHeight < 0 || !m_hWnd)
	{
		return false;
	}
	m_width = contentWidth;
	m_height = contentHeight;

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
	m_plugWindow.SetWindowPos(NULL, 0, 0,
		contentWidth, contentHeight,
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


OPENMPT_NAMESPACE_END
