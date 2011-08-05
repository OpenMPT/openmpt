/*
 *
 * CloseMainDialog.cpp
 * -------------------
 * Purpose: Code for displaying a dialog with a list of unsaved documents, and the ability to choose which documents should be saved or not.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "CloseMainDialog.h"


BEGIN_MESSAGE_MAP(CloseMainDialog, CDialog)
	ON_LBN_SELCHANGE(IDC_LIST1,		OnSelectionChanged)
	ON_COMMAND(IDC_BUTTON1,			OnSwitchSelection)
	ON_COMMAND(IDC_CHECK1,			OnSwitchFullPaths)
END_MESSAGE_MAP()


void CloseMainDialog::DoDataExchange(CDataExchange* pDX)
//------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DoDataExchange)
	DDX_Control(pDX, IDC_LIST1,		m_List);
	//}}AFX_DATA_MAP
}


// Format a list entry string
CString CloseMainDialog::FormatTitle(const CModDoc *pModDoc, bool fullPath)
//-------------------------------------------------------------------------
{
	const CString &path = (!fullPath || pModDoc->GetPathName().IsEmpty()) ? pModDoc->GetTitle() :  pModDoc->GetPathName();
	return pModDoc->GetSoundFile()->GetTitle() + CString(" (") + path + CString(")");
}


BOOL CloseMainDialog::OnInitDialog()
//----------------------------------
{
	CDialog::OnInitDialog();

	CMainFrame::GetInputHandler()->Bypass(true);

	// Create list of unsaved documents
	m_List.ResetContent();

	CheckDlgButton(IDC_CHECK1, BST_CHECKED);

	CDocTemplate *pDocTmpl = theApp.GetModDocTemplate();
	if(pDocTmpl)
	{
		POSITION pos = pDocTmpl->GetFirstDocPosition();
		CDocument *pDoc;
		while((pos != NULL) && ((pDoc = pDocTmpl->GetNextDoc(pos)) != NULL))
		{
			CModDoc *pModDoc = (CModDoc *)pDoc;
			if(pModDoc->IsModified())
			{
				int item = m_List.AddString(FormatTitle(pModDoc, true));
				m_List.SetItemDataPtr(item, pModDoc);
				m_List.SetSel(item, TRUE);
			}
		}
	}

	if(m_List.GetCount() == 0)
	{
		// No modified documents...
		OnOK();
	} else
	{
		UpdateSwitchButtonState();
	}

	return TRUE;
}


void CloseMainDialog::OnOK()
//--------------------------
{
	const int count = m_List.GetCount();
	for(int i = 0; i < count; i++)
	{
		CModDoc *pModDoc = (CModDoc *)m_List.GetItemDataPtr(i);
		ASSERT(pModDoc != nullptr);
		if(m_List.GetSel(i))
		{
			pModDoc->ActivateWindow();
			if(pModDoc->DoFileSave() == FALSE)
			{
				// If something went wrong, or if the user decided to cancel saving (when using "Save As"), we'll better not proceed...
				OnCancel();
				return;
			}
		} else
		{
			pModDoc->SetModified(FALSE);
		}
	}

	CDialog::OnOK();
	CMainFrame::GetInputHandler()->Bypass(false);

}


void CloseMainDialog::OnCancel()
//------------------------------
{
	CDialog::OnCancel();
	CMainFrame::GetInputHandler()->Bypass(false);
}


void CloseMainDialog::OnSelectionChanged()
//----------------------------------------
{
	UpdateSwitchButtonState();
}


// Switch between save all/none
void CloseMainDialog::OnSwitchSelection()
//---------------------------------------
{
	const int count = m_List.GetCount();
	// If all items are selected, deselect them all; Else, select all items.
	const BOOL action = (m_List.GetSelCount() == count) ? FALSE : TRUE;
	for(int i = 0; i < count; i++)
	{
		m_List.SetSel(i, action);
	}
	UpdateSwitchButtonState();
}


// Update Select none/all button
void CloseMainDialog::UpdateSwitchButtonState()
//---------------------------------------------
{
	CString text = (m_List.GetSelCount() == m_List.GetCount()) ? "Se&lect none" : "Se&lect all";
	((CButton *)GetDlgItem(IDC_BUTTON1))->SetWindowText(text);
	text = (m_List.GetSelCount() > 0) ? "&Save selected" : "Cl&ose";
	((CButton *)GetDlgItem(IDOK))->SetWindowText(text);
}


// Switch between full path / filename only display
void CloseMainDialog::OnSwitchFullPaths()
//---------------------------------------
{
	const int count = m_List.GetCount();
	const bool fullPath = (IsDlgButtonChecked(IDC_CHECK1) == BST_CHECKED);
	for(int i = 0; i < count; i++)
	{
		CModDoc *pModDoc = (CModDoc *)m_List.GetItemDataPtr(i);
		int item = m_List.InsertString(i + 1, FormatTitle(pModDoc, fullPath));
		m_List.SetItemDataPtr(item, pModDoc);
		m_List.SetSel(item, m_List.GetSel(i));
		m_List.DeleteString(i);
	}
}
