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
#include "Sndfile.h"


OPENMPT_NAMESPACE_BEGIN


// CPatternGotoDialog dialog

CPatternGotoDialog::CPatternGotoDialog(CWnd *pParent, ROWINDEX row, CHANNELINDEX chan, PATTERNINDEX pat, ORDERINDEX ord, const CSoundFile &sndFile)
	: CDialog(IDD_EDIT_GOTO, pParent)
	, m_SndFile(sndFile)
	, m_nRow(row)
	, m_nChannel(chan)
	, m_nPattern(pat)
	, m_nOrder(ord)
	, m_nActiveOrder(ord)
	, m_bControlLock(false)
{ }


void CPatternGotoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN1, m_SpinRow);
	DDX_Control(pDX, IDC_SPIN2, m_SpinChannel);
	DDX_Control(pDX, IDC_SPIN3, m_SpinPattern);
	DDX_Control(pDX, IDC_SPIN4, m_SpinOrder);

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


BOOL CPatternGotoDialog::OnInitDialog()
{
	m_SpinRow.SetRange32(0, MAX_PATTERN_ROWS - 1);
	m_SpinChannel.SetRange32(1, MAX_BASECHANNELS);
	m_SpinPattern.SetRange32(0, std::max(m_SndFile.Patterns.GetNumPatterns(), PATTERNINDEX(1)) - 1);
	m_SpinOrder.SetRange32(0, std::max(m_SndFile.Order().GetLengthTailTrimmed(), ORDERINDEX(1)) - 1);
	return TRUE;
}

void CPatternGotoDialog::OnOK()
{
	UpdateData();
	const auto &Order = m_SndFile.Order();

	bool validated = true;
	// Valid order item?
	if(m_nOrder >= Order.size())
		validated = false;
	// Does order match pattern?
	else if(Order[m_nOrder] != m_nPattern)
		validated = false;
	// Does pattern exist?
	if(!Order.IsValidPat(m_nOrder))
		validated = false;
	// Does pattern have enough rows?
	else if(m_SndFile.Patterns[m_nPattern].GetNumRows() <= m_nRow)
		validated = false;
	
	Limit(m_nChannel, CHANNELINDEX(1), m_SndFile.GetNumChannels());

	if(validated)
		CDialog::OnOK();
	else
		CDialog::OnCancel();


}

void CPatternGotoDialog::OnEnChangeGotoPat()
{
	if(ControlsLocked())
	{
		return;	// The change to textbox did not come from user.
	}
		
	UpdateData();
	m_nOrder = m_SndFile.Order().FindOrder(m_nPattern, m_nActiveOrder);

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
		return;	//the change to textbox did not come from user.
	}

	UpdateData();

	if(m_SndFile.Order().IsValidPat(m_nOrder))
	{
		m_nPattern = m_SndFile.Order()[m_nOrder];
	}

	LockControls();
	UpdateData(FALSE);
	UnlockControls();
}


OPENMPT_NAMESPACE_END
