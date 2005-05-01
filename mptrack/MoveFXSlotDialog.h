#pragma once
#include "afxwin.h"

// CMoveFXSlotDialog dialog

class CMoveFXSlotDialog : public CDialog
{
	DECLARE_DYNAMIC(CMoveFXSlotDialog)

public:
	CMoveFXSlotDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMoveFXSlotDialog();
	void SetupMove(UINT currentSlot, CArray<UINT, UINT> &emptySlots);
	UINT m_nToSlot;
	


// Dialog Data
	enum { IDD = IDD_MOVEFXSLOT };

protected:
	CString m_csPrompt, m_csTitle;
	CEdit m_EditPrompt;
	CArray<UINT, UINT> m_EmptySlots;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CComboBox m_CbnEmptySlots;

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
public:
	
	virtual BOOL OnInitDialog();
};
