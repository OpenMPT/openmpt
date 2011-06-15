// MoveFXSlotDialog.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "afxtempl.h"
#include "MoveFXSlotDialog.h"
#include ".\movefxslotdialog.h"


// CMoveFXSlotDialog dialog

IMPLEMENT_DYNAMIC(CMoveFXSlotDialog, CDialog)
CMoveFXSlotDialog::CMoveFXSlotDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMoveFXSlotDialog::IDD, pParent)
{
}

CMoveFXSlotDialog::~CMoveFXSlotDialog()
//-------------------------------------
{
}

void CMoveFXSlotDialog::DoDataExchange(CDataExchange* pDX)
//--------------------------------------------------------
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CbnEmptySlots);
	DDX_Control(pDX, IDC_EDIT1, m_EditPrompt);
}


BEGIN_MESSAGE_MAP(CMoveFXSlotDialog, CDialog)
END_MESSAGE_MAP()


// CMoveFXSlotDialog message handlers

void CMoveFXSlotDialog::OnOK()
//----------------------------
{
	m_nToSlot = static_cast<PLUGINDEX>(m_CbnEmptySlots.GetItemData(m_CbnEmptySlots.GetCurSel()));
	CDialog::OnOK();
}

void CMoveFXSlotDialog::SetupMove(PLUGINDEX currentSlot, CArray<UINT, UINT> &emptySlots, PLUGINDEX defaultIndex)
//--------------------------------------------------------------------------------------------------------------
{	
	m_nDefaultSlot = defaultIndex;
	m_csPrompt.Format("Move plugin in slot %d to the following empty slot:", currentSlot + 1);
	m_csTitle.Format("Move To Slot..");
	m_EmptySlots.Copy(emptySlots);
}

BOOL CMoveFXSlotDialog::OnInitDialog()
//------------------------------------
{
	CDialog::OnInitDialog();
	m_EditPrompt.SetWindowText(m_csPrompt);
	SetWindowText(m_csTitle);

 	CString slotText;
	int defaultSlot = 0;
	bool foundDefault = false;
	for (int nSlot=0; nSlot<m_EmptySlots.GetSize(); nSlot++)
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
