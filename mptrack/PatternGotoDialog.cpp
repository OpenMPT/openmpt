/*
 * PatternGotoDialog.cpp
 * ---------------------
 * Purpose: Implementation of pattern "go to" dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "mptrack.h"
#include "PatternGotoDialog.h"
#include ".\patterngotodialog.h"
#include "Sndfile.h"


OPENMPT_NAMESPACE_BEGIN


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
	UINT temp;
	temp = m_nRow; DDX_Text(pDX, IDC_GOTO_ROW, temp); m_nRow = static_cast<ROWINDEX>(temp);
	temp = m_nChannel; DDX_Text(pDX, IDC_GOTO_CHAN, temp); m_nChannel = static_cast<CHANNELINDEX>(temp);
	temp = m_nPattern; DDX_Text(pDX, IDC_GOTO_PAT, temp); m_nPattern = static_cast<PATTERNINDEX>(temp);
	temp = m_nOrder; DDX_Text(pDX, IDC_GOTO_ORD, temp); m_nOrder = static_cast<ORDERINDEX>(temp);
}


BEGIN_MESSAGE_MAP(CPatternGotoDialog, CDialog)
	ON_EN_CHANGE(IDC_GOTO_PAT, OnEnChangeGotoPat)
	ON_EN_CHANGE(IDC_GOTO_ORD, OnEnChangeGotoOrd)
END_MESSAGE_MAP()


// CPatternGotoDialog message handlers

void CPatternGotoDialog::UpdatePos(ROWINDEX row, CHANNELINDEX chan, PATTERNINDEX pat, ORDERINDEX ord, CSoundFile &sndFile)
{
	m_nRow = row;
	m_nChannel = chan;
	m_nPattern = pat;
	m_nActiveOrder = ord;
	m_nOrder = ord;
	m_pSndFile = &sndFile;
}

void CPatternGotoDialog::OnOK()
{
	UpdateData();
	
	bool validated = true;
	
	// Does pattern exist?
	if(validated && !m_pSndFile->Patterns.IsValidPat(static_cast<PATTERNINDEX>(m_nPattern)))
	{
		validated = false;
	}
	
	// Does order match pattern?
	if(validated && m_pSndFile->Order[m_nOrder] != m_nPattern)
	{
		validated = false;
	}

	// Does pattern have enough rows?
	if(validated && m_pSndFile->Patterns[m_nPattern].GetNumRows() <= m_nRow)
	{
		validated = false;
	}
	
	// Does track have enough channels?
	if(validated && m_pSndFile->m_nChannels < m_nChannel)
	{
		validated = false;
	}


	if (validated)
	{
		CDialog::OnOK();
	} else
	{
		CDialog::OnCancel();
	}


}

void CPatternGotoDialog::OnEnChangeGotoPat()
//------------------------------------------
{
	if(ControlsLocked())
	{
		return;				//the change to textbox did not come from user.
	}
		
	UpdateData();
	m_nOrder = m_pSndFile->Order.FindOrder(static_cast<PATTERNINDEX>(m_nPattern), static_cast<ORDERINDEX>(m_nActiveOrder));

	if(m_nOrder == ORDERINDEX_INVALID)
	{
		m_nOrder = 0;
	}

	LockControls();
	UpdateData(FALSE);
	UnlockControls();
}

void CPatternGotoDialog::OnEnChangeGotoOrd()
{
	if(ControlsLocked())
	{
		return;				//the change to textbox did not come from user.
	}

	UpdateData();

	if(m_nOrder<m_pSndFile->Order.size())
	{
		PATTERNINDEX candidatePattern = m_pSndFile->Order[m_nOrder];
		if(candidatePattern < m_pSndFile->Patterns.Size() && m_pSndFile->Patterns[candidatePattern])
		{
			m_nPattern = candidatePattern;
		}
	}

	LockControls();
	UpdateData(FALSE);
	UnlockControls();
}


OPENMPT_NAMESPACE_END
