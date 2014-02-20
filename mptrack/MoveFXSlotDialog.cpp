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


CMoveFXSlotDialog::CMoveFXSlotDialog(CWnd *pParent, PLUGINDEX currentSlot, const std::vector<PLUGINDEX> &emptySlots, PLUGINDEX defaultIndex, bool clone) :
	CDialog(CMoveFXSlotDialog::IDD, pParent),
	m_nDefaultSlot(defaultIndex),
	m_EmptySlots(emptySlots)
{
	if(clone)
	{
		m_csPrompt.Format(_T("Clone plugin in slot %d to the following empty slot:"), currentSlot + 1);
		m_csTitle = _T("Clone To Slot...");
	} else
	{
		m_csPrompt.Format(_T("Move plugin in slot %d to the following empty slot:"), currentSlot + 1);
		m_csTitle = _T("Move To Slot...");
	}
}


void CMoveFXSlotDialog::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CbnEmptySlots);
	DDX_Control(pDX, IDC_EDIT1, m_EditPrompt);
}


void CMoveFXSlotDialog::OnOK()
//----------------------------
{
	m_nToSlot = static_cast<PLUGINDEX>(m_CbnEmptySlots.GetItemData(m_CbnEmptySlots.GetCurSel()));
	CDialog::OnOK();
}


BOOL CMoveFXSlotDialog::OnInitDialog()
//------------------------------------
{
	CDialog::OnInitDialog();
	m_EditPrompt.SetWindowText(m_csPrompt);
	SetWindowText(m_csTitle);

	if(m_EmptySlots.empty())
	{
		Reporting::Error("No empty plugin slots are availabe.");
		OnCancel();
		return TRUE;
	}

	CString slotText;
	int defaultSlot = 0;
	bool foundDefault = false;
	for(size_t nSlot = 0; nSlot < m_EmptySlots.size(); nSlot++)
	{
		slotText.Format("FX%d", m_EmptySlots[nSlot] + 1);
		m_CbnEmptySlots.SetItemData(m_CbnEmptySlots.AddString(slotText), m_EmptySlots[nSlot]);
		if(m_EmptySlots[nSlot] >= m_nDefaultSlot && !foundDefault)
		{
			defaultSlot = nSlot;
			foundDefault = true;
		}
	}
	m_CbnEmptySlots.SetCurSel(defaultSlot);

	return TRUE;
}
