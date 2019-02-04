/*
 * MoveFXSlotDialog.h
 * ------------------
 * Purpose: Implementationof OpenMPT's move plugin dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "MoveFXSlotDialog.h"


OPENMPT_NAMESPACE_BEGIN


void CMoveFXSlotDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CbnEmptySlots);
}


CMoveFXSlotDialog::CMoveFXSlotDialog(CWnd *pParent, PLUGINDEX currentSlot, const std::vector<PLUGINDEX> &emptySlots, PLUGINDEX defaultIndex, bool clone, bool hasChain) :
	CDialog(CMoveFXSlotDialog::IDD, pParent),
	m_EmptySlots(emptySlots),
	m_nDefaultSlot(defaultIndex),
	moveChain(hasChain)
{
	if(clone)
	{
		m_csPrompt.Format(_T("Clone plugin in slot %d to the following empty slot:"), currentSlot + 1);
		m_csTitle = _T("Clone To Slot...");
		m_csChain = _T("&Clone follow-up plugin chain if possible");
	} else
	{
		m_csPrompt.Format(_T("Move plugin in slot %d to the following empty slot:"), currentSlot + 1);
		m_csTitle = _T("Move To Slot...");
		m_csChain = _T("&Move follow-up plugin chain if possible");
	}
}


BOOL CMoveFXSlotDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetDlgItemText(IDC_STATIC1, m_csPrompt);
	SetDlgItemText(IDC_CHECK1, m_csChain);
	SetWindowText(m_csTitle);

	if(m_EmptySlots.empty())
	{
		Reporting::Error("No empty plugin slots are availabe.");
		OnCancel();
		return TRUE;
	}

	CString slotText;
	std::size_t defaultSlot = 0;
	bool foundDefault = false;
	for(size_t nSlot = 0; nSlot < m_EmptySlots.size(); nSlot++)
	{
		slotText.Format(_T("FX%d"), m_EmptySlots[nSlot] + 1);
		m_CbnEmptySlots.SetItemData(m_CbnEmptySlots.AddString(slotText), nSlot);
		if(m_EmptySlots[nSlot] >= m_nDefaultSlot && !foundDefault)
		{
			defaultSlot = nSlot;
			foundDefault = true;
		}
	}
	m_CbnEmptySlots.SetCurSel(static_cast<int>(defaultSlot));

	GetDlgItem(IDC_CHECK1)->EnableWindow(moveChain ? TRUE : FALSE);
	CheckDlgButton(IDC_CHECK1, moveChain ? BST_CHECKED : BST_UNCHECKED);

	return TRUE;
}


void CMoveFXSlotDialog::OnOK()
{
	m_nToSlot = m_CbnEmptySlots.GetItemData(m_CbnEmptySlots.GetCurSel());
	moveChain = IsDlgButtonChecked(IDC_CHECK1) != BST_UNCHECKED;
	CDialog::OnOK();
}


OPENMPT_NAMESPACE_END
