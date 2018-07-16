/*
 * MoveFXSlotDialog.h
 * ------------------
 * Purpose: Implementationof OpenMPT's move plugin dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

OPENMPT_NAMESPACE_BEGIN

class CMoveFXSlotDialog : public CDialog
{
protected:
	const std::vector<PLUGINDEX> &m_EmptySlots;
	CString m_csPrompt, m_csTitle, m_csChain;
	size_t m_nToSlot;
	PLUGINDEX m_nDefaultSlot;
	bool moveChain;

	CComboBox m_CbnEmptySlots;

	enum { IDD = IDD_MOVEFXSLOT };

public:
	CMoveFXSlotDialog(CWnd *pParent, PLUGINDEX currentSlot, const std::vector<PLUGINDEX> &emptySlots, PLUGINDEX defaultIndex, bool clone, bool hasChain);
	PLUGINDEX GetSlot() const { return m_EmptySlots[m_nToSlot]; }
	size_t GetSlotIndex() const { return m_nToSlot; }
	bool DoMoveChain() const { return moveChain; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual void OnOK();
	virtual BOOL OnInitDialog();
};

OPENMPT_NAMESPACE_END
