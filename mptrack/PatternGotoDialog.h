/*
 * PatternGotoDialog.h
 * -------------------
 * Purpose: Implementation of pattern "go to" dialog.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;

// CPatternGotoDialog dialog

class CPatternGotoDialog : public CDialog
{
	const CSoundFile &m_SndFile;
public:
	ROWINDEX m_nRow;
	CHANNELINDEX m_nChannel;
	PATTERNINDEX m_nPattern;
	ORDERINDEX m_nOrder, m_nActiveOrder;

public:
	CPatternGotoDialog(CWnd *pParent, ROWINDEX row, CHANNELINDEX chan, PATTERNINDEX pat, ORDERINDEX ord, const CSoundFile &sndFile);

protected:
	bool m_bControlLock;
	inline bool ControlsLocked() { return m_bControlLock; }
	inline void LockControls() { m_bControlLock = true; }
	inline void UnlockControls() { m_bControlLock = false; }

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void OnOK();
	afx_msg void OnEnChangeGotoPat();
	afx_msg void OnEnChangeGotoOrd();

	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
