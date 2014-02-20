/*
 * MoveFXSlotDialog.h
 * ------------------
 * Purpose: Implementationof OpenMPT's move plugin dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

class CMoveFXSlotDialog : public CDialog
{
protected:
	const std::vector<PLUGINDEX> &m_EmptySlots;
	CString m_csPrompt, m_csTitle;
	CEdit m_EditPrompt;
	PLUGINDEX m_nDefaultSlot, m_nToSlot;

	CComboBox m_CbnEmptySlots;

	enum { IDD = IDD_MOVEFXSLOT };

public:
	CMoveFXSlotDialog(CWnd *pParent, PLUGINDEX currentSlot, const std::vector<PLUGINDEX> &emptySlots, PLUGINDEX defaultIndex, bool clone);
	PLUGINDEX GetSlot() const { return m_nToSlot; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual void OnOK();
	virtual BOOL OnInitDialog();
};
