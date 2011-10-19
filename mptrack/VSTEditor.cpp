#include "stdafx.h"
//#include "VstPlug.h"
#include "vsteditor.h"

//////////////////////////////////////////////////////////////////////////////
//
// COwnerVstEditor
//


#ifndef NO_VST

COwnerVstEditor::COwnerVstEditor(CVstPlugin *pPlugin) : CAbstractVstEditor(pPlugin)
//---------------------------------------------------------------------------------
{

}


COwnerVstEditor::~COwnerVstEditor()
//---------------------------------
{

}


BOOL COwnerVstEditor::OpenEditor(CWnd *parent)
//--------------------------------------------
{
	Create(IDD_PLUGINEDITOR, parent);
	SetupMenu();
	if(m_pVstPlugin)
	{
		// Set editor window size
		CRect rcWnd, rcClient;
		ERect *pRect;

		pRect = NULL;
		m_pVstPlugin->Dispatch(effEditGetRect, 0, 0, (LPRECT)&pRect, 0);
		m_pVstPlugin->Dispatch(effEditOpen, 0, 0, (void *)m_hWnd, 0);
		m_pVstPlugin->Dispatch(effEditGetRect, 0, 0, (LPRECT)&pRect, 0);
		if((pRect) && (pRect->right > pRect->left) && (pRect->bottom > pRect->top))
		{
			GetWindowRect(&rcWnd);
			GetClientRect(&rcClient);
			SetWindowPos(NULL, 0,0, 
				(rcWnd.Width()) - (rcClient.Width()) + (pRect->right - pRect->left),
				(rcWnd.Height()) - (rcClient.Height()) + (pRect->bottom - pRect->top),
				SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);
			
		}

		// Restore previous editor position
		int editorX, editorY;
		m_pVstPlugin->GetEditorPos(editorX, editorY);

		if((editorX >= 0) && (editorY >= 0))
		{
			int cxScreen = GetSystemMetrics(SM_CXSCREEN);
			int cyScreen = GetSystemMetrics(SM_CYSCREEN);
			if((editorX + 8 < cxScreen) && (editorY + 8 < cyScreen))
			{
				SetWindowPos(NULL, editorX, editorY, 0, 0,
					SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
			}
		}
		SetTitle();
		

		m_pVstPlugin->Dispatch(effEditTop, 0,0, NULL, 0);
		m_pVstPlugin->Dispatch(effEditIdle, 0,0, NULL, 0);
	}

	ShowWindow(SW_SHOW);
	return TRUE;
}


VOID COwnerVstEditor::OnClose()
//-----------------------------
{
	DoClose();
}


VOID COwnerVstEditor::OnOK()
//--------------------------
{
	OnClose();
}


VOID COwnerVstEditor::OnCancel()
//------------------------------
{
	OnClose();
}



VOID COwnerVstEditor::DoClose()
//-----------------------------
{
#ifdef VST_LOG
	Log("CVstEditor::DoClose()\n");
#endif // VST_LOG
	if ((m_pVstPlugin) && (m_hWnd))
	{
		CRect rect;
		GetWindowRect(&rect);
		m_pVstPlugin->SetEditorPos(rect.left, rect.top);
	}
	if (m_pVstPlugin)
	{
#ifdef VST_LOG
		Log("Dispatching effEditClose...\n");
#endif // VST_LOG
		m_pVstPlugin->Dispatch(effEditClose, 0, 0, NULL, 0);
	}
	if (m_hWnd)
	{
#ifdef VST_LOG
		Log("Destroying window...\n");
#endif // VST_LOG
		// Initially, this was called before the last Dispatch() call.
		// Now it's done after that call so that energyXT's GUI still works after re-opening the VST editor.
		// Let's hope that other plugins don't break...
		DestroyWindow();
	}
}
#endif // NO_VST

