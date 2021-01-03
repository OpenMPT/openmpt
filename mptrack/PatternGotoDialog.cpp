/*
 * PatternGotoDialog.cpp
 * ---------------------
 * Purpose: Implementation of pattern "go to" dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Mptrack.h"
#include "PatternGotoDialog.h"
#include "Sndfile.h"


OPENMPT_NAMESPACE_BEGIN


// CPatternGotoDialog dialog

CPatternGotoDialog::CPatternGotoDialog(CWnd *pParent, ROWINDEX row, CHANNELINDEX chan, PATTERNINDEX pat, ORDERINDEX ord, CSoundFile &sndFile)
	: CDialog(IDD_EDIT_GOTO, pParent)
	, m_SndFile(sndFile)
	, m_nRow(row)
	, m_nChannel(chan)
	, m_nPattern(pat)
	, m_nOrder(ord)
	, m_nActiveOrder(ord)
{ }


void CPatternGotoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SPIN1, m_SpinRow);
	DDX_Control(pDX, IDC_SPIN2, m_SpinChannel);
	DDX_Control(pDX, IDC_SPIN3, m_SpinPattern);
	DDX_Control(pDX, IDC_SPIN4, m_SpinOrder);
}


BEGIN_MESSAGE_MAP(CPatternGotoDialog, CDialog)
	ON_EN_CHANGE(IDC_GOTO_PAT, &CPatternGotoDialog::OnPatternChanged)
	ON_EN_CHANGE(IDC_GOTO_ORD, &CPatternGotoDialog::OnOrderChanged)
	ON_EN_CHANGE(IDC_GOTO_ROW, &CPatternGotoDialog::OnRowChanged)
	ON_EN_CHANGE(IDC_EDIT5,    &CPatternGotoDialog::OnTimeChanged)
	ON_EN_CHANGE(IDC_EDIT6,    &CPatternGotoDialog::OnTimeChanged)
END_MESSAGE_MAP()


BOOL CPatternGotoDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_SpinRow.SetRange32(0, MAX_PATTERN_ROWS - 1);
	m_SpinChannel.SetRange32(1, MAX_BASECHANNELS);
	m_SpinPattern.SetRange32(0, std::max(m_SndFile.Patterns.GetNumPatterns(), PATTERNINDEX(1)) - 1);
	m_SpinOrder.SetRange32(0, std::max(m_SndFile.Order().GetLengthTailTrimmed(), ORDERINDEX(1)) - 1);
	SetDlgItemInt(IDC_GOTO_ROW, m_nRow);
	SetDlgItemInt(IDC_GOTO_CHAN, m_nChannel);
	SetDlgItemInt(IDC_GOTO_PAT, m_nPattern);
	SetDlgItemInt(IDC_GOTO_ORD, m_nOrder);
	UpdateTime();
	UnlockControls();
	return TRUE;
}

void CPatternGotoDialog::OnOK()
{
	const auto &Order = m_SndFile.Order();
	m_nRow = mpt::saturate_cast<ROWINDEX>(GetDlgItemInt(IDC_GOTO_ROW));
	m_nChannel = mpt::saturate_cast<CHANNELINDEX>(GetDlgItemInt(IDC_GOTO_CHAN));

	// Valid order item?
	if(m_nOrder >= Order.size())
	{
		MessageBeep(MB_ICONWARNING);
		GetDlgItem(IDC_GOTO_ORD)->SetFocus();
		return;
	}
	// Does order match pattern? Does pattern exist?
	if(Order[m_nOrder] != m_nPattern || !Order.IsValidPat(m_nOrder))
	{
		MessageBeep(MB_ICONWARNING);
		GetDlgItem(IDC_GOTO_PAT)->SetFocus();
		return;
	}

	LimitMax(m_nRow, m_SndFile.Patterns[m_nPattern].GetNumRows() - ROWINDEX(1));
	Limit(m_nChannel, CHANNELINDEX(1), m_SndFile.GetNumChannels());

	CDialog::OnOK();
}

void CPatternGotoDialog::OnPatternChanged()
{
	if(ControlsLocked())
		return;  // The change to textbox did not come from user.

	m_nPattern = mpt::saturate_cast<PATTERNINDEX>(GetDlgItemInt(IDC_GOTO_PAT));
	m_nOrder = m_SndFile.Order().FindOrder(m_nPattern, m_nActiveOrder);

	if(m_nOrder == ORDERINDEX_INVALID)
	{
		m_nOrder = 0;
	}

	LockControls();
	SetDlgItemInt(IDC_GOTO_ORD, m_nOrder);
	UpdateTime();
	UnlockControls();
}


void CPatternGotoDialog::OnOrderChanged()
{
	if(ControlsLocked())
		return;  // The change to textbox did not come from user.

	m_nOrder = mpt::saturate_cast<ORDERINDEX>(GetDlgItemInt(IDC_GOTO_ORD));
	if(m_SndFile.Order().IsValidPat(m_nOrder))
	{
		m_nPattern = m_SndFile.Order()[m_nOrder];
	}

	LockControls();
	SetDlgItemInt(IDC_GOTO_PAT, m_nPattern);
	UpdateTime();
	UnlockControls();
}


void CPatternGotoDialog::OnRowChanged()
{
	if(ControlsLocked())
		return;  // The change to textbox did not come from user.

	m_nRow = mpt::saturate_cast<ROWINDEX>(GetDlgItemInt(IDC_GOTO_ROW));
	UpdateTime();
}


void CPatternGotoDialog::OnTimeChanged()
{
	if(ControlsLocked())
		return;  // The change to textbox did not come from user.

	BOOL success = TRUE;
	auto minutes = GetDlgItemInt(IDC_EDIT5, &success);
	if(!success)
		return;
	auto seconds = GetDlgItemInt(IDC_EDIT6, &success);
	if(!success)
		return;

	auto result = m_SndFile.GetLength(eNoAdjust, GetLengthTarget(minutes * 60.0 + seconds)).back();
	if(!result.targetReached)
		return;

	m_nOrder = result.lastOrder;
	m_nRow = result.lastRow;
	if(m_SndFile.Order().IsValidPat(m_nOrder))
		m_nPattern = m_SndFile.Order()[m_nOrder];

	LockControls();
	SetDlgItemInt(IDC_GOTO_ORD, m_nOrder);
	SetDlgItemInt(IDC_GOTO_ROW, m_nRow);
	SetDlgItemInt(IDC_GOTO_PAT, m_nPattern);
	UnlockControls();
}


void CPatternGotoDialog::UpdateTime()
{
	const double length = m_SndFile.GetPlaybackTimeAt(m_nOrder, m_nRow, false, false);
	if(length < 0.0)
		return;
	const double minutes = std::floor(length / 60.0), seconds = std::fmod(length, 60.0);
	LockControls();
	SetDlgItemInt(IDC_EDIT5, static_cast<int>(minutes));
	SetDlgItemInt(IDC_EDIT6, static_cast<int>(seconds));
	UnlockControls();
}


OPENMPT_NAMESPACE_END
