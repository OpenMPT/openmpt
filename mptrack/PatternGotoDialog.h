/*
 * PatternGotoDialog.h
 * -------------------
 * Purpose: Implementation of pattern "go to" dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;

// CPatternGotoDialog dialog

class CPatternGotoDialog : public CDialog
{
	CSoundFile &m_SndFile;
	CSpinButtonCtrl m_SpinRow, m_SpinChannel, m_SpinPattern, m_SpinOrder;

public:
	ROWINDEX m_nRow;
	CHANNELINDEX m_nChannel;
	PATTERNINDEX m_nPattern;
	ORDERINDEX m_nOrder, m_nActiveOrder;

public:
	CPatternGotoDialog(CWnd *pParent, ROWINDEX row, CHANNELINDEX chan, PATTERNINDEX pat, ORDERINDEX ord, CSoundFile &sndFile);
	BOOL OnInitDialog() override;

protected:
	bool m_controlLock = true;

	inline bool ControlsLocked() const { return m_controlLock; }
	inline void LockControls() { m_controlLock = true; }
	inline void UnlockControls() { m_controlLock = false; }

	void UpdateTime();

	void DoDataExchange(CDataExchange* pDX) override;
	void OnOK() override;

	afx_msg void OnPatternChanged();
	afx_msg void OnOrderChanged();
	afx_msg void OnRowChanged();
	afx_msg void OnTimeChanged();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
