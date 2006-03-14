// PatternGotoDialog.cpp : implementation file
//

#include "stdafx.h"
#include "mptrack.h"
#include "PatternGotoDialog.h"
#include ".\patterngotodialog.h"
#include "sndfile.h"


// CPatternGotoDialog dialog

IMPLEMENT_DYNAMIC(CPatternGotoDialog, CDialog)
CPatternGotoDialog::CPatternGotoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPatternGotoDialog::IDD, pParent)
	, m_nRow(0)
	, m_nChannel(0)
	, m_nPattern(0)
	, m_nOrder(0)
{
	m_bControlLock=false;
	::CreateDialog(NULL, MAKEINTRESOURCE(IDD), pParent->m_hWnd, NULL); 
}

CPatternGotoDialog::~CPatternGotoDialog()
{
}

void CPatternGotoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_GOTO_ROW, m_nRow);
	DDX_Text(pDX, IDC_GOTO_CHAN, m_nChannel);
	DDX_Text(pDX, IDC_GOTO_PAT, m_nPattern);
	DDX_Text(pDX, IDC_GOTO_ORD, m_nOrder);
}


BEGIN_MESSAGE_MAP(CPatternGotoDialog, CDialog)
	ON_EN_CHANGE(IDC_GOTO_PAT, OnEnChangeGotoPat)
	ON_EN_CHANGE(IDC_GOTO_ORD, OnEnChangeGotoOrd)
END_MESSAGE_MAP()


// CPatternGotoDialog message handlers

void CPatternGotoDialog::UpdatePos(UINT row, UINT chan, UINT pat, UINT ord, CSoundFile* pSndFile)
{
	m_nRow = row;
	m_nChannel = chan;
	m_nPattern = pat;
	m_nActiveOrder = ord;
	m_nOrder = ord;
	m_pSndFile = pSndFile;
}

void CPatternGotoDialog::OnOK()
{
	UpdateData();
	
	bool validated=true;
	
	//is pattern number sensible?
	if (m_nPattern>=MAX_PATTERNS) {
		validated=false;
	}

	//Does pattern exist?
	if (validated && !(m_pSndFile->Patterns[m_nPattern])) {
		validated=false;
	}
	
	//Does order match pattern?
	if (validated && m_pSndFile->Order[m_nOrder] != m_nPattern) {
		validated=false;
	}

	//Does pattern have enough rows?
	if (validated && m_pSndFile->PatternSize[m_nPattern] <= m_nRow) {
		validated=false;
	}
	
	//Does track have enough channels?
	if (validated && m_pSndFile->m_nChannels < m_nChannel) {
		validated=false;
	}


	if (validated) {
		CDialog::OnOK();
	} else {
		CDialog::OnCancel();
	}


}

void CPatternGotoDialog::OnEnChangeGotoPat()
//------------------------------------------
{
	if (ControlsLocked()) {
		return;				//the change to textbox did not come from user.
	}
		
	UpdateData();
	m_nOrder = m_pSndFile->FindOrder(m_nPattern, m_nActiveOrder);

	if (m_nOrder>MAX_ORDERS) {
		m_nOrder=0;
	}

	LockControls();
	UpdateData(FALSE);
	UnlockControls();
}

void CPatternGotoDialog::OnEnChangeGotoOrd()
{
	if (ControlsLocked()) {
		return;				//the change to textbox did not come from user.
	}

	UpdateData();

	if (m_nOrder<MAX_ORDERS) {
		UINT candidatePattern = m_pSndFile->Order[m_nOrder];
		if (candidatePattern<MAX_PATTERNS && m_pSndFile->Patterns[candidatePattern]) {
			m_nPattern = candidatePattern;
		} 
	}

	LockControls();
	UpdateData(FALSE);
	UnlockControls();
}
