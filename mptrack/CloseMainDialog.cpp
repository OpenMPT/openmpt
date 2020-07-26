/*
 * CloseMainDialog.cpp
 * -------------------
 * Purpose: Dialog showing a list of unsaved documents, with the ability to choose which documents should be saved or not.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "CloseMainDialog.h"


OPENMPT_NAMESPACE_BEGIN


BEGIN_MESSAGE_MAP(CloseMainDialog, ResizableDialog)
	ON_COMMAND(IDC_BUTTON1,			&CloseMainDialog::OnSaveAll)
	ON_COMMAND(IDC_BUTTON2,			&CloseMainDialog::OnSaveNone)
	ON_COMMAND(IDC_CHECK1,			&CloseMainDialog::OnSwitchFullPaths)
END_MESSAGE_MAP()


void CloseMainDialog::DoDataExchange(CDataExchange* pDX)
{
	ResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DoDataExchange)
	DDX_Control(pDX, IDC_LIST1,		m_List);
	//}}AFX_DATA_MAP
}


CloseMainDialog::CloseMainDialog() : ResizableDialog(IDD_CLOSEDOCUMENTS)
{
};


CString CloseMainDialog::FormatTitle(const CModDoc *modDoc, bool fullPath)
{
	return MPT_CFORMAT("{} ({})")
		(mpt::ToCString(modDoc->GetSoundFile().GetCharsetInternal(), modDoc->GetSoundFile().GetTitle()),
		(!fullPath || modDoc->GetPathNameMpt().empty()) ? modDoc->GetTitle() : modDoc->GetPathNameMpt().ToCString());
}


BOOL CloseMainDialog::OnInitDialog()
{
	ResizableDialog::OnInitDialog();

	// Create list of unsaved documents
	m_List.ResetContent();

	CheckDlgButton(IDC_CHECK1, BST_CHECKED);

	m_List.SetRedraw(FALSE);
	for(const auto &modDoc : theApp.GetOpenDocuments())
	{
		if(modDoc->IsModified())
		{
			int item = m_List.AddString(FormatTitle(modDoc, true));
			m_List.SetItemDataPtr(item, modDoc);
			m_List.SetSel(item, TRUE);
		}
	}
	m_List.SetRedraw(TRUE);

	if(m_List.GetCount() == 0)
	{
		// No modified documents...
		OnOK();
	}

	return TRUE;
}


void CloseMainDialog::OnOK()
{
	const int count = m_List.GetCount();
	for(int i = 0; i < count; i++)
	{
		CModDoc *modDoc = static_cast<CModDoc *>(m_List.GetItemDataPtr(i));
		MPT_ASSERT(modDoc != nullptr);
		if(m_List.GetSel(i))
		{
			modDoc->ActivateWindow();
			if(modDoc->DoFileSave() == FALSE)
			{
				// If something went wrong, or if the user decided to cancel saving (when using "Save As"), we'll better not proceed...
				OnCancel();
				return;
			}
		} else
		{
			modDoc->SetModified(FALSE);
		}
	}

	ResizableDialog::OnOK();
}


void CloseMainDialog::OnSaveAll()
{
	if(m_List.GetCount() == 1)
		m_List.SetSel(0, TRUE);	// SelItemRange can't select one item: https://jeffpar.github.io/kbarchive/kb/129/Q129428/
	else
		m_List.SelItemRange(TRUE, 0, m_List.GetCount() - 1);
	OnOK();
}


void CloseMainDialog::OnSaveNone()
{
	if(m_List.GetCount() == 1)
		m_List.SetSel(0, FALSE);	// SelItemRange can't select one item: https://jeffpar.github.io/kbarchive/kb/129/Q129428/
	else
		m_List.SelItemRange(FALSE, 0, m_List.GetCount() - 1);
	OnOK();
}


// Switch between full path / filename only display
void CloseMainDialog::OnSwitchFullPaths()
{
	const int count = m_List.GetCount();
	const bool fullPath = (IsDlgButtonChecked(IDC_CHECK1) == BST_CHECKED);
	m_List.SetRedraw(FALSE);
	for(int i = 0; i < count; i++)
	{
		CModDoc *modDoc = static_cast<CModDoc *>(m_List.GetItemDataPtr(i));
		int item = m_List.InsertString(i + 1, FormatTitle(modDoc, fullPath));
		m_List.SetItemDataPtr(item, modDoc);
		m_List.SetSel(item, m_List.GetSel(i));
		m_List.DeleteString(i);
	}
	m_List.SetRedraw(TRUE);
}

OPENMPT_NAMESPACE_END
